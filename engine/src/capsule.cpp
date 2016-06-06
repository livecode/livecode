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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"


#include "dispatch.h"
#include "stack.h"
#include "globals.h"

#include "deploy.h"
#include "md5.h"
#include "capsule.h"

#include "stacksecurity.h"

#include <zlib.h>

////////////////////////////////////////////////////////////////////////////////

// The capsule bucket structure contains a single block of data that it has
// been provided with through one of the fill methods.
struct MCCapsuleBucket
{
	// We chain buckets together in a linked list.
	MCCapsuleBucket *next;

	// The amount of data we have in this bucket
	uint32_t length;

	// The buffer holding the data, if its a memory bucket
	void *data_buffer;

	// The file holding the data, if its a file bucket.
	IO_handle data_file;

	// If this is true, it means the capsule does not own the data or file
	// and must not free it.
	bool data_foreign;

	// The current offset into the data we are reading.
	uint32_t data_offset;
};

struct MCCapsule
{
	// The list of buckets we have to read data from.
	MCCapsuleBucket *buckets;

	// If this is truethe input data stream is complete.
	bool buckets_complete;

	// This is the total amount of data left in the buckets that we have
	// still to read.
	uint32_t buckets_available;

	// If this is true, then it means the next section is too big for internal
	// buffering and we must wait until we have all data before continuing.
	bool blocked;

	// The buffer we use when reading data - we need to demask incoming data
	// so always need to buffer. The frontier is the index of the first byte
	// that has yet to be decompressed.
	bool input_eof;
	uint8_t *input_buffer;
	uint32_t input_frontier;
	uint32_t input_capacity;

	// The buffer we use to decompress into when we have to buffer a whole
	// section. The frontier is the index of the first invalid output byte
	// (i.e. [0..frontier) holds the current bytes that can be processed).
	uint8_t *output_buffer;
	uint32_t output_frontier;
	uint32_t output_capacity;

	// The zlib decompress state
	z_stream decompress;

	// The md5 digest stream
	md5_state_t digest;

	// The md5 of the data up to before the current section header
	uint8_t last_digest[16];

	// The details of the current stream.
	bool stream_has_pushback;
	uint8_t stream_pushback;
	uint32_t stream_length;
	uint32_t stream_offset;

	// The user-defined callback to invoke on process
	MCCapsuleCallback callback;
	void *callback_state;
};

////////////////////////////////////////////////////////////////////////////////

static void MCCapsuleBucketDestroy(MCCapsuleBucket *self);

static bool MCCapsuleBucketCreate(uint32_t p_data_length, uint32_t p_data_offset, bool p_finished, bool p_foreign, const void *p_data, IO_handle p_file, MCCapsuleBucket*& r_self)
{
	bool t_success;
	t_success = true;

	MCCapsuleBucket *self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);

	if (t_success)
	{
		self -> length = p_data_length;
		self -> data_foreign = p_foreign;
		self -> data_file = p_file;
		self -> data_buffer = (void *)p_data;
		self -> data_offset = p_data_offset;
		r_self = self;
	}
	else
		MCCapsuleBucketDestroy(self);

	return t_success;
}

static void MCCapsuleBucketDestroy(MCCapsuleBucket *self)
{
	if (self == nil)
		return;

	if (self -> data_buffer != nil && !self -> data_foreign)
		MCMemoryDeallocate(self -> data_buffer);

	if (self -> data_file != nil && !self -> data_foreign)
		MCS_close(self -> data_file);

	MCMemoryDelete(self);
}

////////////////////////////////////////////////////////////////////////////////

bool MCCapsuleOpen(MCCapsuleCallback p_callback, void *p_callback_state, MCCapsuleRef& r_self)
{
	bool t_success;
	t_success = true;

	// Allocate the state record.
	MCCapsuleRef self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);

	// Allocate an initial input buffer of 4K in size
	if (t_success)
		t_success = MCMemoryAllocate(4096, self -> input_buffer);

	// Allocate an initial output buffer of 4K in size
	if (t_success)
		t_success = MCMemoryAllocate(4096, self -> output_buffer);

	// Initialize the inflate filter
	if (t_success)
	{
		self -> decompress . next_in = self -> input_buffer;
		self -> decompress . avail_in = 0;
		self -> decompress . next_out = self  -> output_buffer;
		self -> decompress . avail_out = 0;
		if (inflateInit2(&self -> decompress, -15) != Z_OK)
			t_success = false; //MCThrow(kMCErrorCapsuleInflateFailed);
	}

	// If successful, initialize as appropriate
	if (t_success)
	{
		md5_init(&self -> digest);

		self -> input_capacity = 4096;
		self -> output_capacity = 4096;

		self -> callback = p_callback;
		self -> callback_state = p_callback_state;

		r_self = self;
	}
	else
		MCCapsuleClose(self);

	return t_success;
}

void MCCapsuleClose(MCCapsuleRef self)
{
	if (self == nil)
		return;

	// Free the buckets
	while(self -> buckets != nil)
		MCCapsuleBucketDestroy(MCListPopFront(self -> buckets));

	inflateEnd(&self -> decompress);
	MCMemoryDeallocate(self -> output_buffer);
	MCMemoryDeallocate(self -> input_buffer);

	MCMemoryDelete(self);
}

bool MCCapsuleBlocked(MCCapsuleRef self)
{
	return self -> blocked;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCCapsuleFillCommon(MCCapsuleRef self, uint32_t p_data_length, uint32_t p_data_offset, bool p_finished, bool p_foreign, const void *p_data, IO_handle p_file)
{
	bool t_success;
	t_success = true;

	MCCapsuleBucket *t_bucket;
	t_bucket = nil;
	if (t_success)
		t_success = MCCapsuleBucketCreate(p_data_length, p_data_offset, p_finished, p_foreign, p_data, p_file, t_bucket);

	if (t_success)
	{
		// Append the bucket to the list
		MCListPushBack(self -> buckets, t_bucket);

		// The bucket list is complete if finished is true
		self -> buckets_complete = p_finished;

		// Increase the total amount of data available by the size of the bucket.
		self -> buckets_available += p_data_length;
	}
	else
		MCCapsuleBucketDestroy(t_bucket);

	return t_success;
}

bool MCCapsuleFill(MCCapsuleRef self, const void *p_data, uint32_t p_data_length, bool p_finished)
{
	MCAssert(self != nil);
	MCAssert(p_data != nil || p_data_length == 0);

	bool t_success;
	t_success = true;

	void *t_data;
	t_data = nil;
	if (t_success)
		t_success = MCMemoryAllocateCopy(p_data, p_data_length, t_data);

	if (t_success)
		t_success = MCCapsuleFillCommon(self, p_data_length, 0, p_finished, false, t_data, nil);

	if (!t_success)
		MCMemoryDeallocate(t_data);

	return t_success;
}

bool MCCapsuleFillNoCopy(MCCapsuleRef self, const void *p_data, uint32_t p_data_length, bool p_finished)
{
	MCAssert(self != nil);
	MCAssert(p_data != nil || p_data_length == 0);

	return MCCapsuleFillCommon(self, p_data_length, 0, p_finished, true, p_data, nil);
}

bool MCCapsuleFillFromFile(MCCapsuleRef self, MCStringRef p_path, uint32_t p_offset, bool p_finished)
{
	MCAssert(self != nil);
	MCAssert(p_path != nil);

	bool t_success;
	t_success = true;

	IO_handle t_stream;
	t_stream = nil;
	if (t_success)
	{
        t_stream = MCS_open(p_path, kMCOpenFileModeRead, True, False, 0);
		if (t_stream == nil)
			t_success = false;
	}

	// If the amount left is zero, then we should not add a new bucket, but if
	// we are 'done' then we *should* mark the capsule buckets as complete.
	uint32_t t_amount;
	t_amount = 0;
	if (t_success)
		t_amount = (uint32_t)MCS_fsize(t_stream) - p_offset;

	if (t_success)
	{
		if (t_amount > 0)
			t_success = MCCapsuleFillCommon(self, (uint32_t)MCS_fsize(t_stream) - p_offset, p_offset, p_finished, false, nil, t_stream);
		else if (p_finished)
			self -> buckets_complete = true;
	}

	if ((!t_success || t_amount == 0) && t_stream != nil)
		MCS_close(t_stream);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// Processing the input data is a little fiddly due to the need to:
//   1) Process a list of data sources sequentially as if they were one (the
//      buckets)
//   2) Unmask the input data on 32-bit boundaries (the XOR operation operates
//      only on that minimum data size)
//   3) Decompress the data using inflate.
// In general, data flows from the buckets into input_buffer where it is
// unmasked and then through deflate into an output buffer. This may or may not
// be output_buffer, depending if the stream is currently buffering a section
// before we have all the data available.

// The reason for the need to buffer complete sections before we have access
// to all the data is to simplify the higher-level loading code. If this
// buffering did not occur, the higher-level code would have to deal partial
// inputs. Done this way, it means that the higher-level code can always assume
// either it will have all the data it needs, or if it encounters an EOD (end-
// of-data) when it isn't expecting it, it is an error.

// This method reads data from the sequence of buckets, discarding them as they
// are used up. Note that buffer size must always by a multiple of 4, and the
// method will only ever return multiples of 4 bytes.
static bool MCCapsuleReadBuckets(MCCapsuleRef self, void *p_buffer, uint32_t p_buffer_size, uint32_t& r_filled)
{
	// If no data is asked for, we do nothing. Similarly, if there are no
	// buckets left to process, we do nothing
	if (p_buffer_size == 0 || self -> buckets == nil)
	{
		r_filled = 0;
		return true;
	}

	uint8_t *t_buffer;
	t_buffer = static_cast<uint8_t *>(p_buffer);

	// If we are asking for more than we have, make sure we only try and
	// return a nice round number.
	uint32_t t_total;
	t_total = p_buffer_size;
	if (t_total > self -> buckets_available)
		t_total = self -> buckets_available & ~3;

	// The amount we will fill on success is t_total
	uint32_t t_filled;
	t_filled = t_total;

	// Now loop until we've read as much as we can.
	while(t_total > 0)
	{
		// Work out how much we can read from the bucket
		uint32_t t_amount;
		t_amount = MCMin(t_total, self -> buckets -> length);

		// Read the data from the appropriate source, depending on the bucket type.
		if (self -> buckets -> data_buffer != nil)
			MCMemoryCopy(t_buffer, static_cast<uint8_t *>(self -> buckets -> data_buffer) + self -> buckets -> data_offset, t_amount);
		else
		{
			// Set the offset into the input stream appropriately
			uint32_t t_amount_read;
			t_amount_read = t_amount;
			if (MCS_seek_set(self -> buckets -> data_file, self -> buckets -> data_offset) == IO_ERROR)
				return false;

			// Read the data we require.
			if (MCS_readfixed(t_buffer, t_amount_read, self -> buckets -> data_file) == IO_ERROR)
				return false;
		}
		
		// Update the bucket data info
		self -> buckets -> length -= t_amount;
		self -> buckets -> data_offset += t_amount;
		self -> buckets_available -= t_amount;

		// Discard the bucket if its now empty
		if (self -> buckets -> length == 0)
			MCCapsuleBucketDestroy(MCListPopFront(self -> buckets));
		
		// Advance our output buffer
		t_buffer += t_amount;
		t_total -= t_amount;
	}

	r_filled = t_filled;

	return true;
}

// This method reads data from the buckets, unmasks and decompresses it into
// the target buffer. It will attempt to read as many bytes as possible, and
// return the number it managed to read in r_filled.
static bool MCCapsuleRead(MCCapsuleRef self, void *p_buffer, uint32_t p_buffer_size, uint32_t& r_filled)
{
	// If no data has been requested, then we do nothing :o)
	if (p_buffer_size == 0)
	{
		r_filled = 0;
		return true;
	}

	// Point the decompressor to the output buffer
	self -> decompress . next_out = (Bytef *)p_buffer;
	self -> decompress . avail_out = p_buffer_size;

	// Loop until there's no more room, or we have no more input to
	// decompress.
	for(;;)
	{
		// First attempt to inflate
		int t_result;
		t_result = inflate(&self -> decompress, Z_NO_FLUSH);

		// Any of these results in an error in the stream
		if (t_result == Z_DATA_ERROR || t_result == Z_MEM_ERROR || t_result == Z_STREAM_ERROR)
			return false;

		// If the result is Z_STREAM_END, the input is eofed
		if (t_result == Z_STREAM_END)
			self -> input_eof = true;

		// If we have all the output we need, do no more
		if (self -> decompress . avail_out == 0)
			break;

		// If we have reached the end of input, then we can do no more.
		if (self -> input_eof || self -> buckets_available == 0)
			break;

		// Similarly, if we have less than 4 bytes available at the moment
		// we must wait for more.
		if (self -> buckets_available < sizeof(uint32_t))
			break;

		// Otherwise, first make more room in the input buffer if there isn't
		// much data left there to process...
		if (self -> decompress . avail_in < 16)
		{
			uint32_t t_offset;
			t_offset = (self -> decompress . next_in - self -> input_buffer) & ~3;
			memmove(self -> input_buffer, self -> input_buffer + t_offset, self -> input_frontier - t_offset);
			self -> decompress . next_in -= t_offset;
			self -> input_frontier -= t_offset;
		}

		// And fill as much of it as we can from the buckets (note read buckets
		// only returns multiples of sizeof(uint32_t) bytes).
		uint32_t t_amount, t_amount_read;
		t_amount = MCMin(self -> input_capacity - self -> input_frontier, 4096U);
		if (!MCCapsuleReadBuckets(self, self -> input_buffer + self -> input_frontier, t_amount, t_amount_read))
			return false;

		// Now, if all data is available, and we didn't get as must as we asked
		// for, the input must be eof.
		if (self -> buckets_complete)
			self -> input_eof = (t_amount != t_amount_read);

		// Process any security features.
		MCStackSecurityProcessCapsule(self -> decompress . next_in + self -> decompress . avail_in, self -> input_buffer + self -> input_frontier + t_amount_read);

		// Adjust the data pointers
		self -> input_frontier += t_amount_read;
		self -> decompress . avail_in = (self -> input_buffer + self -> input_frontier - sizeof(uint32_t)) - (self -> decompress . next_in + self -> decompress . avail_in);
	}

	uint32_t t_filled;
	t_filled = self -> decompress . next_out - (Bytef *)p_buffer;

	// Add the output we just generated to the md5.
	md5_append(&self -> digest, (md5_byte_t *)p_buffer, t_filled);

	r_filled = t_filled;

	return true;
}

// This method attempts to ensure that there are a given number of valid bytes
// in the output buffer. Note that it is not an error if it cannot guarantee
// this and the caller must check how much is available by looking at the
// output frontier.
static bool MCCapsuleEnsure(MCCapsuleRef self, uint32_t p_amount)
{
	bool t_success;
	t_success = true;

	// If we already have enough, then return
	if (p_amount <= self -> output_frontier)
		return true;

	// First ensure we have enough space in the output buffer.
	if (p_amount > self -> output_capacity)
	{
		if (MCMemoryReallocate(self -> output_buffer, (p_amount + 4095) & ~4095, self -> output_buffer))
			self -> output_capacity = (p_amount + 4095) & ~4095;
		else
			t_success = false;
	}

	// Now try to read data into the buffer
	uint32_t t_read;
	if (t_success)
		t_success = MCCapsuleRead(self, self -> output_buffer + self -> output_frontier, p_amount - self -> output_frontier, t_read);

	// Update the frontier
	if (t_success)
		self -> output_frontier += t_read;

	return t_success;
}

// This method reads and decompresses data directly into the given buffer. It
// is used by the non-buffered custom stream implementation.
static IO_stat MCCapsuleStreamRead(void *p_state, void *p_buffer, uint32_t p_buffer_size, uint32_t& r_filled)
{
	MCCapsuleRef self;
	self = static_cast<MCCapsuleRef>(p_state);

	// Trivial case
	if (p_buffer_size == 0)
	{
		r_filled = 0;
		return IO_NORMAL;
	}

	// Work out the total amount left we can read
	uint32_t t_available;
	t_available = self -> stream_length - self -> stream_offset;

	// The count of how much we've filled
	uint32_t t_filled;
	t_filled = 0;

	// If there is push back, we can at least generate one byte
	if (self -> stream_has_pushback)
	{
		self -> stream_has_pushback = false;
		*(uint8_t *)p_buffer = self -> stream_pushback;
		t_filled += 1;
	}

	// Now we have some data in the output buffer lingering from buffering. We
	// use this up first.
	if (self -> stream_offset < self -> output_frontier)
	{
		uint32_t t_amount;
		t_amount = MCMin(p_buffer_size - t_filled, self -> output_frontier - self -> stream_offset);
		MCMemoryCopy(static_cast<uint8_t *>(p_buffer) + t_filled, self -> output_buffer + self -> stream_offset, t_amount);
		t_filled += t_amount;
	}

	// Now just do a read to fill as much of the rest as we can
	uint32_t t_to_read, t_amount_read;
	t_to_read = MCMin(t_available, p_buffer_size - t_filled);
	if (!MCCapsuleRead(self, static_cast<uint8_t *>(p_buffer) + t_filled, t_to_read, t_amount_read))
		return IO_ERROR;

	// Update filled
	t_filled += t_amount_read;

	// Store the last byte generated for pushback reasons
	self -> stream_pushback = static_cast<uint8_t *>(p_buffer)[t_filled - 1];

	// Update the stream offset
	self -> stream_offset += t_filled;

	// Return
	r_filled = t_filled;

	return IO_NORMAL;
}

static int64_t MCCapsuleStreamTell(void *p_state)
{
	MCCapsuleRef self;
	self = static_cast<MCCapsuleRef>(p_state);

	// This is simple - we just return the 'offset'
	return self -> stream_offset;
}

static IO_stat MCCapsuleStreamSeekSet(void *p_state, int64_t p_offset)
{
	MCCapsuleRef self;
	self = static_cast<MCCapsuleRef>(p_state);

	// If the new offset is before the current offset, it is an error (we can
	// only seek forwards)
	if (p_offset < self -> stream_offset)
		return IO_ERROR;

	// If we are seeking to the current position, then we are done.
	if (p_offset == self -> stream_offset)
		return IO_NORMAL;

	// If we are attempting to seek past the end of the stream, it is an error.
	if (p_offset > self -> stream_length)
		return IO_ERROR;

	// Otherwise we must read data until we reach the desired offset.
	uint32_t t_skip_amount;
	t_skip_amount = (uint32_t)p_offset - self -> stream_offset;
	
	// Adjust for pushback - we don't need to read the pushback byte from the
	// compress stream.
	if (self -> stream_has_pushback)
		t_skip_amount -= 1;

	// If bytes are in the output buffer, we should just skip those directly.
	if (self -> stream_offset < self -> output_frontier)
	{
		uint32_t t_amount;
		t_amount = MCMin(t_skip_amount, self -> output_frontier - self -> stream_offset);
		t_skip_amount -= t_amount;
		self -> stream_offset += t_amount;
	}

	// Now loop, reading chunks until we've reached our target. There is no
	// faster way to do this - skipped data must still be decompressed and
	// checksummed.
	while(t_skip_amount > 0)
	{
		// We use a temporary buffer of 4K in size.
		char t_buffer[4096];

		// Compute the amount to read
		uint32_t t_amount;
		t_amount = MCMin(t_skip_amount, 4096U);

		// Attempt to read. Note that if we don't read the amount we expect
		// the seek offset is invalid and so it must be an io error.
		uint32_t t_amount_read;
		if (!MCCapsuleRead(self, t_buffer, t_amount, t_amount_read))
			return IO_ERROR;

		if (t_amount_read != t_amount)
			return IO_ERROR;

		// Adjust what we have to do by how much we read.
		t_skip_amount -= t_amount;

		// Adjust the stream offset
		self -> stream_offset += t_amount;
	}

	return IO_NORMAL;
}

static IO_stat MCCapsuleStreamSeekCur(void *p_state, int64_t p_offset)
{
	MCCapsuleRef self;
	self = static_cast<MCCapsuleRef>(p_state);

	// We can only seek back one byte
	if (p_offset != -1)
		return IO_ERROR;

	// If we already have a push back byte, then we cannot seek another
	if (self -> stream_has_pushback)
		return IO_ERROR;

	// Set the pushback flag to true - note that the pushback byte is
	// already stored (by StreamRead).
	self -> stream_has_pushback = true;
	
	// Adjust the offset
	self -> stream_offset -= 1;

	return IO_NORMAL;
}

bool MCCapsuleProcess(MCCapsuleRef self)
{
	bool t_success;
	t_success = true;

	// If we are blocked, we do nothing unless buckets are complete.
	if (!self -> buckets_complete && self -> blocked)
		return true;

	while(t_success)
	{
		uint32_t t_header[2];

		// Ensure the first header word is available (if any).
		t_success = MCCapsuleEnsure(self, sizeof(uint32_t));

		// If there is not enough data then either we are done, or we just need to
		// wait for more input - either way, we break.
		if (t_success && self -> output_frontier < sizeof(uint32_t))
			break;

		// Fetch the first header word to see if we need to read more.
		if (t_success)
			t_header[0] = MCSwapInt32NetworkToHost(((uint32_t *)self -> output_buffer)[0]);

		// If the top bit of the first header word is set, it means that we
		// need to read another header word.
		if (t_success && (t_header[0] & (1U << 31)) != 0)
		{
			t_success = MCCapsuleEnsure(self, sizeof(uint32_t) * 2);
			if (self -> output_frontier < sizeof(uint32_t) * 2)
				break;

			if (t_success)
				t_header[1] = MCSwapInt32NetworkToHost(((uint32_t *)self -> output_buffer)[1]);
		}

		// Now we have the header compute its tag and size.
		uint32_t t_type, t_length, t_header_size;
		if (t_success)
		{
			t_type = (t_header[0] >> 24) & 0x7f;
			t_length = t_header[0] & 0xffffff;
			if ((t_header[0] & (1U << 31)) != 0)
			{
				t_type |= (t_header[1] & 0xffffff00) >> 1;
				t_length |= (t_header[1] & 0xff) << 24;
				t_header_size = 2 * sizeof(uint32_t);
			}
			else
				t_header_size = sizeof(uint32_t);
		}

		// At this point we have the full header, so what we do now depends on
		// whether we have all data or not. If buckets are not finished we see if
		// we can muster the data needed to process.
		if (t_success && !self -> buckets_complete)
		{
			// If the amount of data required is greater than the hard limit of
			// buffering, then block and return. (A megabyte seems reasonable for now).
			if (t_length > 1024 * 1024)
			{
				self -> blocked = true;
				break;
			}

			// Ensure the entire tag length + header size + padding
			// (if any).  The padding is computed up to the next 4-byte boundary.
			uint32_t t_required_length;
			t_required_length = (t_header_size + t_length + 3) & ~3;
			t_success = MCCapsuleEnsure(self, t_required_length);

			// If we don't have enough data, break as we can do no more for now.
			if (t_success && self -> output_frontier < t_required_length)
				break;
		}

		// At this point there are two possibilities - we have all the data of
		// the section buffered, or it is available in the buckets. (Only the
		// header has been buffered).
		IO_handle t_stream;
		t_stream = nil;
		if (t_success)
		{
			if (!self -> buckets_complete)
			{
				// If we haven't got all the data, then we have all data buffered
				// so can use a regular variety fake stream.
                t_stream = MCS_fakeopen((const char *)self -> output_buffer + t_header_size, t_length);
			}
			else
			{
				// If we have got all data at our finger tips we use a custom
				// stream that directly reads from the decompressed data stream.
				MCFakeOpenCallbacks t_callbacks;
				t_callbacks . read = MCCapsuleStreamRead;
				t_callbacks . tell = MCCapsuleStreamTell;
				t_callbacks . seek_set = MCCapsuleStreamSeekSet;
				t_callbacks . seek_cur = MCCapsuleStreamSeekCur;

				// If there is any data left in the output buffer, shift it back
				// to eliminate the header.
				MCMemoryMove(self -> output_buffer, self -> output_buffer + t_header_size, self -> output_frontier - t_header_size);
				self -> output_frontier -= t_header_size;

				self -> stream_has_pushback = false;
				self -> stream_pushback = 0;
				self -> stream_length = t_length;
				self -> stream_offset = 0;

				t_stream = MCS_fakeopencustom(&t_callbacks, self);
			}

			if (t_stream == nil)
				t_success = false;
		}

		// Now invoke the callback with the stream.
		if (t_success)
			t_success = self -> callback(self -> callback_state, self -> last_digest, (MCCapsuleSectionType)t_type, t_length, t_stream);

		// If this was a fake stream (due to complete buckets) seek first to
		// the end of the fake stream. Then seek the real stream round up
		// to the next pad boundary. We don't need to do this for a buffered
		// section as that's already done.
		if (t_success && self -> buckets_complete)
		{
			// First round up the length of the stream to the 32-bit boundary.
			// (We keep the byte length correct for the callback as the data
			// contained within a section is unaware of the rounding).
			self -> stream_length = (self -> stream_length + 3) & ~3;
			if (MCS_seek_set(t_stream, self -> stream_length) != IO_NORMAL)
				t_success = false;
		}

		// Now we store the digest we just calculated for passing to the next
		// section.
		if (t_success)
			md5_finish_copy(&self -> digest, (md5_byte_t *)self -> last_digest);

		// We've read a full section now, so reset the output frontier to 0
		if (t_success)
			self -> output_frontier = 0;

		// Finally, release the stream
		if (t_stream != nil)
			MCS_close(t_stream);
	}

	// If buckets have finished, and there is data available still then we have
	// ourselves an io error.
	if (self -> buckets_complete && (self -> output_frontier != 0 || self -> buckets != nil))
		return false;

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
