/* Copyright (C) 2003-2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"


#include "handler.h"
#include "scriptpt.h"
#include "variable.h"
#include "statemnt.h"
#include "globals.h"

#include "ide.h"
#include "deploy.h"
#include "capsule.h"
#include "md5.h"
#include "mode.h"
#include "license.h"

#include "deploysecurity.h"

#include <zlib.h>

////////////////////////////////////////////////////////////////////////////////

// This constant determines the maximum amount of data we put into the exe's
// project section before overflowing to an external file. It should be 4K -
// header size - magic size. i.e. Right now this is 4088. The intention here is
// that we don't create unnecessary padding. Block size in EXE's tends to be
// 4K. (Due to the fact that this is the 'natural' page size of many platforms).
#define kMCDeployCapsuleSpillLimit 4088

// An MCDeployCapsuleSection represents a section that will be written to a
// capsule.
struct MCDeployCapsuleSection
{
	// We link the capsule sections into a linked list.
	MCDeployCapsuleSection *next;

	// The type of section.
	MCCapsuleSectionType type;

	// The length of the section.
	uint32_t length;

	// The data and/or file containing the data - if data is nil, then
	// data_file must not be and vice-versa.
	void *buffer;
	MCDeployFileRef file;
};

// The state structure for the MCDeployCapsule opaque type. At present this is
// simply just a linked list of MCDeployCapsuleSection structures.
struct MCDeployCapsule
{
	MCDeployCapsuleSection *sections;
};

////////////////////////////////////////////////////////////////////////////////

static bool MCDeployCapsuleSectionCreate(MCCapsuleSectionType p_type, MCDeployCapsuleSection*& r_self);
static void MCDeployCapsuleSectionDestroy(MCDeployCapsuleSection *self);

static bool MCDeployCapsuleSectionCreate(MCCapsuleSectionType p_type, MCDeployCapsuleSection*& r_self)
{
	bool t_success;
	t_success = true;

	// Allocate the state structure
	MCDeployCapsuleSection *self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);

	// If successful, initialize the state
	if (t_success)
	{
		self -> type = p_type;
		r_self = self;
	}
	else
		MCDeployCapsuleSectionDestroy(self);
	
	return t_success;
}

static void MCDeployCapsuleSectionDestroy(MCDeployCapsuleSection *self)
{
	if (self == nil)
		return;

	// Delete the data
	MCMemoryDeallocate(self -> buffer);

	// Delete the state
	MCMemoryDelete(self);
}

////////////////////////////////////////////////////////////////////////////////

bool MCDeployCapsuleCreate(MCDeployCapsuleRef& r_self)
{
	bool t_success;
	t_success = true;

	// Allocate the state structure (initialized to zero)
	MCDeployCapsule *self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);

	// Process success
	if (t_success)
		r_self = self;
	
	return t_success;
}

void MCDeployCapsuleDestroy(MCDeployCapsuleRef self)
{
	if (self == nil)
		return;

	// Loop through all the sections and destroy them
	while(self -> sections != nil)
		MCDeployCapsuleSectionDestroy(MCListPopFront(self -> sections));

	// Delete the deploy capsule structure
	MCMemoryDelete(self);
}

////////////////////////////////////////////////////////////////////////////////

bool MCDeployCapsuleDefine(MCDeployCapsuleRef self, MCCapsuleSectionType p_type, const void *p_data, uint32_t p_data_size)
{
	MCAssert(self != nil);
	MCAssert(p_data != nil || p_data_size == 0);

	bool t_success;
	t_success = true;

	// Allocate a new section structure
	MCDeployCapsuleSection *t_section;
	t_section = nil;
	if (t_success)
		t_success = MCDeployCapsuleSectionCreate(p_type, t_section);

	// Allocate the data segment, if needed
	if (t_success && p_data != nil)
		t_success = MCMemoryAllocate(p_data_size, t_section -> buffer);

	// Process success, or destroy if failure
	if (t_success)
	{
		// Copy across data
		t_section -> length = p_data_size;
		MCMemoryCopy(t_section -> buffer, p_data, p_data_size);

		// Link into chain at end
		MCListPushBack(self -> sections, t_section);
	}
	else
		MCDeployCapsuleSectionDestroy(t_section);

	return t_success;
}

bool MCDeployCapsuleDefineString(MCDeployCapsuleRef self, MCCapsuleSectionType p_type, MCStringRef p_string)
{
    MCAutoStringRefAsCString t_auto_cstring;
    /* UNCHECKED */ t_auto_cstring . Lock(p_string);
    return MCDeployCapsuleDefine(self, p_type, *t_auto_cstring, strlen(*t_auto_cstring) + 1);
}

bool MCDeployCapsuleDefineFromFile(MCDeployCapsuleRef self, MCCapsuleSectionType p_type, MCDeployFileRef p_file)
{
	MCAssert(self != nil);
	MCAssert(p_file != nil);

	bool t_success;
	t_success = true;

	// Allocate a new section
	MCDeployCapsuleSection *t_section;
	t_section = nil;
	if (t_success)
		t_success = MCDeployCapsuleSectionCreate(p_type, t_section);

	// Now measure the length of the file
	if (t_success)
		t_success = MCDeployFileMeasure(p_file, t_section -> length);

	// Process success, or destroy if failure
	if (t_success)
	{
		// Link to the file
		t_section -> file = p_file;

		// Append to section list
		MCListPushBack(self -> sections, t_section);
	}
	else
		MCDeployCapsuleSectionDestroy(t_section);


	return t_success;
}

bool MCDeployCapsuleChecksum(MCDeployCapsuleRef self)
{
	MCAssert(self != nil);

	bool t_success;
	t_success = true;

	// Allocate a new section
	MCDeployCapsuleSection *t_section;
	t_section = nil;
	if (t_success)
		t_success = MCDeployCapsuleSectionCreate(kMCCapsuleSectionTypeDigest, t_section);

	// Link into the chain if successful, otherwise destroy
	if (t_success)
		MCListPushBack(self -> sections, t_section);
	else
		MCDeployCapsuleSectionDestroy(t_section);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// This holds state for a simple compressed data writing filter. The filter
// uses the zlib library to perform raw deflate. There is no need for anything
// beyong the raw level of compression since we know precisely what data we
// are expecting back *and* also digest/checksum the whole lot already.
//
struct MCDeployCapsuleFilterState
{
	// The target output file
	MCDeployFileRef file;

	// The target split file
	MCDeployFileRef spill_file;

	// The initial offset into the output file
	uint32_t start_offset;

	// The offset into the output file to write
	uint32_t offset;

	// The offset into the spill file we are
	uint32_t spill_offset;

	// The number of bytes we've written
	uint32_t amount;

	// The md5 state for computing the digest as we go along
	md5_state_t md5_stream;

	// The input/output buffers in use by zlib
	uint8_t *input;
	uint32_t input_capacity;

	uint8_t *output;
	uint32_t output_capacity;

	// The zlib state
	z_stream stream;
};

static void MCDeployCapsuleFilterInitialize(MCDeployCapsuleFilterState& self)
{
	memset(&self, 0, sizeof(MCDeployCapsuleFilterState));
}

static void MCDeployCapsuleFilterFinalize(MCDeployCapsuleFilterState& self)
{
	MCMemoryDeallocate(self . input);
	MCMemoryDeallocate(self . output);
	deflateEnd(&self . stream);
}

static bool MCDeployCapsuleFilterStart(MCDeployCapsuleFilterState& self, MCDeployFileRef p_output, MCDeployFileRef p_spill_output, uint32_t p_offset)
{
	memset(&self, 0, sizeof(MCDeployCapsuleFilterState));

	self . file = p_output;
	self . spill_file = p_spill_output;
	self . offset = p_offset;
	self . start_offset = p_offset;

	self . input_capacity = 65536;
	if (!MCMemoryAllocate(65536, self . input))
		return false;

	self . output_capacity = 65536;
	if (!MCMemoryAllocate(65536, self . output))
		return false;

	// Initialize the md5 stream
	md5_init(&self . md5_stream);

	// Initialize the deflate stream
	int t_result;
	self . stream . next_in = self . input;
	self . stream . avail_in = 0;
	self . stream . next_out = self . output;
	self . stream . avail_out = self . output_capacity;
	t_result = deflateInit2(&self . stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8,Z_DEFAULT_STRATEGY);
	if (t_result != Z_OK)
		return MCDeployThrow(kMCDeployErrorBadCompress);

	return true;
}

// This method outputs the current buffer into the output file/split output file.
// If we are splitting output, then the following happens:
//   1) We fill up to the first 4K - header size in the output file
//   2) If we overflow this, we move the last 2K into the split file and continue
//      output there.
//   3) When we are done, we shift the last 2K of data back into the exe.
// If this is the last block of data to output, p_last will be true.
//
static bool MCDeployCapsuleFilterOutput(MCDeployCapsuleFilterState& self, bool p_last)
{
	// Compute the amount of data we have
	uint32_t t_amount;
	t_amount = self . stream . next_out - self . output;

	// If we aren't splitting, this is easy
	if (self . spill_file == nil)
	{
		if (!MCDeployFileWriteAt(self . file, self . output, t_amount, self . offset))
			return false;
	
		// Update the offset
		self . offset += t_amount;

		// Update the amount written
		self . amount += t_amount;
	}
	else
	{
		if (!MCDeployFileWriteAt(self . spill_file, self . output, t_amount, self . spill_offset))
			return false;
		self . spill_offset += t_amount;
		self . amount += t_amount;
	}


	// Reset the stream
	self . stream . next_out = self . output;
	self . stream . avail_out = self . output_capacity;

	return true;
}

static bool MCDeployCapsuleFilterFlush(MCDeployCapsuleFilterState& self)
{
	// Now attempt to do the deflate
	int t_result;
	t_result = deflate(&self . stream, Z_NO_FLUSH);
	if (t_result == Z_STREAM_ERROR)
		return MCDeployThrow(kMCDeployErrorBadCompress);

	// First ensure we maximum space in the input buffer
	if (self . stream . next_in != self . input)
	{
		memmove(self . input, self . stream . next_in, self . stream . avail_in);
		self . stream . next_in = self . input;
	}

	// If a buf error occurred, we either need more space, or more input
	if (t_result == Z_BUF_ERROR)
	{
		// If the input buffer is not maxed out, return as we need more input
		if (self . stream . avail_in != self . input_capacity)
			return true;

		// Otherwise, we must need a bigger input buffer, so extend it.
		uint8_t *t_new_input;
		t_new_input = (uint8_t *)realloc(self . input, self . input_capacity * 2);
		if (t_new_input == NULL)
			return MCDeployThrow(kMCDeployErrorNoMemory);

		// Update the stream input pointer
		self . stream . next_in = t_new_input + (self . stream . next_in - self . input);

		// Set the new input buffer and size
		self . input = t_new_input;
		self . input_capacity *= 2;

		return true;
	}

	// Otherwise, first write out any produced output
	if (self . stream . avail_out != self . output_capacity)
	{
		// Write out the data.
		if (!MCDeployCapsuleFilterOutput(self, false))
			return false;
	}

	return true;
}

static bool MCDeployCapsuleFilterWrite(MCDeployCapsuleFilterState& self, void *p_buffer, uint32_t p_size)
{
	// Loop until we have no more input to write
	while(p_size > 0)
	{
		// Fill the input buffer as much as we can
		uint32_t t_amount;
		t_amount = MCU_min((unsigned)((self . input + self . input_capacity) - (self . stream . next_in + self . stream . avail_in)), p_size);
		memcpy(self . stream . next_in + self . stream . avail_in, p_buffer, t_amount);

		// Mix in the new data into the MD5 stream.
		md5_append(&self . md5_stream, self . stream . next_in + self . stream . avail_in, t_amount);

		// Update the amount that is available
		self . stream . avail_in += t_amount;

		// Adjust the input buffer/size
		p_size -= t_amount;
		p_buffer = (uint8_t *)p_buffer + t_amount;

		// Attempt to flush the data
		if (!MCDeployCapsuleFilterFlush(self))
			return false;
	}

	return true;
}

static bool MCDeployCapsuleFilterWriteFile(MCDeployCapsuleFilterState& self, MCDeployFileRef p_file, uint32_t p_from, uint32_t p_size)
{
	while(p_size > 0)
	{
		uint32_t t_amount;
		t_amount = MCU_min((unsigned)((self . input + self . input_capacity) - (self . stream . next_in + self . stream . avail_in)), p_size);
		if (!MCDeployFileReadAt(p_file, self . stream . next_in + self . stream . avail_in, t_amount, p_from))
			return false;

		// Mix in the new data into the MD5 stream.
		md5_append(&self . md5_stream, self . stream . next_in + self . stream . avail_in, t_amount);

		self . stream . avail_in += t_amount;

		p_size -= t_amount;
		p_from += t_amount;

		if (!MCDeployCapsuleFilterFlush(self))
			return false;
	}

	return true;
}

static bool MCDeployCapsuleFilterFinish(MCDeployCapsuleFilterState& self, uint32_t& r_offset, md5_byte_t r_digest[16])
{
	if (deflate(&self . stream, Z_FINISH) != Z_STREAM_END)
		return MCDeployThrow(kMCDeployErrorBadCompress);

	// Write out any remaining data
	if (!MCDeployCapsuleFilterOutput(self, true))
		return false;

	// Finish off the md5
	md5_finish(&self  . md5_stream, r_digest);

	r_offset = self . offset;

	return true;
}

bool MCDeployCapsuleGenerate(MCDeployCapsuleRef self, MCDeployFileRef p_file, MCDeployFileRef p_spill_file, uint32_t& x_offset)
{
	MCAssert(self != nil);
	MCAssert(p_file != nil);

	bool t_success;
	t_success = true;

	// Initialize the filter state structure.
	MCDeployCapsuleFilterState t_filter;
	MCDeployCapsuleFilterInitialize(t_filter);

	// Here we hold the number of bytes written before compression, and the starting file offset
	uint32_t t_generated;
	t_generated = 0;

	// Startup the filter
	if (t_success)
		t_success = MCDeployCapsuleFilterStart(t_filter, p_file, p_spill_file, x_offset);

	// Loop through the sections, generating them as needed.
	if (t_success)
		for(MCDeployCapsuleSection *t_section = self -> sections; t_section != nil && t_success; t_section = t_section -> next)
		{
			// If this is a digest section we generate the data and write
			if (t_section -> type == kMCCapsuleSectionTypeDigest)
			{
				// Compute the digest
				md5_byte_t t_digest[16];
				md5_finish_copy(&t_filter . md5_stream, t_digest);

				// Now construct the header and write out
				uint32_t t_header;
				t_header = (kMCCapsuleSectionTypeDigest << 24) | 16;
				MCDeployByteSwap32(true, t_header);

				if (t_success)
					t_success = MCDeployCapsuleFilterWrite(t_filter, &t_header, sizeof(uint32_t));
				if (t_success)
					t_success = MCDeployCapsuleFilterWrite(t_filter, t_digest, 16);
				if (t_success)
					t_generated += sizeof(uint32_t) + 16;

				continue;
			}
			
			// This is a section for which data has been supplied so first write
			// out the header.
			if (t_section -> length >= 1 << 24 || (integer_t)t_section -> type >= 128)
			{
				// MW-2009-07-14: Probably best to make sure we fill the *right* indices in the
				//   header array :o)
				uint32_t t_header[2];
				t_header[0] = (1U << 31) | ((t_section -> type & 0x7f) << 24) | (t_section -> length & 0xffffff);
				t_header[1] = ((t_section -> type & 0x7fffff80) << 1) | (t_section -> length >> 24);
				MCDeployByteSwapRecord(true, "ll", t_header, sizeof(t_header));
				t_success = MCDeployCapsuleFilterWrite(t_filter, t_header, sizeof(t_header));
				if (t_success)
					t_generated += sizeof(t_header);
			}
			else
			{
				uint32_t t_header;
				t_header = ((t_section -> type & 0x7f) << 24) | (t_section -> length & 0xffffff);
				MCDeployByteSwap32(true, t_header);
				t_success = MCDeployCapsuleFilterWrite(t_filter, &t_header, sizeof(uint32_t));
				if (t_success)
					t_generated += sizeof(uint32_t);
			}

			// Now write out the data
			if (t_success)
			{
				if (t_section -> buffer != nil)
					t_success = MCDeployCapsuleFilterWrite(t_filter, t_section -> buffer, t_section -> length);
				else if (t_section -> file != nil)
					t_success = MCDeployCapsuleFilterWriteFile(t_filter, t_section -> file, 0, t_section -> length);

				if (t_success)
					t_generated += t_section -> length;
			}

			// Finally write out any necessary padding
			if (t_success && (t_generated & 3) != 0)
			{
				uint32_t t_zero;
				t_zero = 0;
				t_success = MCDeployCapsuleFilterWrite(t_filter, &t_zero, 4 - (t_generated & 3));
				if (t_success)
					t_generated = (t_generated + 3) & ~3;
			}
		}

	// Now we've written out all the principal data, finish the filter
	uint32_t t_offset;
	md5_byte_t t_digest[16];
	if (t_success)
		t_success = MCDeployCapsuleFilterFinish(t_filter, t_offset, t_digest);

	// Now actually run the masking
	if (t_success)
	{
		if (t_filter . spill_file == NULL)
			t_success = MCDeploySecuritySecureStandalone(p_file, t_filter . start_offset, t_filter . amount, t_offset, t_digest);
		else
			t_success = MCDeploySecuritySecureStandalone(t_filter . spill_file, 0, t_filter . amount, t_filter . spill_offset, t_digest);
	}

	// Return the new offset if successful
	if (t_success)
		x_offset = t_offset;

	// Clean up
	MCDeployCapsuleFilterFinalize(t_filter);

	return t_success;
}
