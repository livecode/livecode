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
#include "zlib.h"

#include "minizip.h"

////////////////////////////////////////////////////////////////////////////////

// The zip file format consists of:
//   --- START OF ARCHIVE
//   Local file header 1
//   File data 1
//   [ Local file footer 1 ]
//   Local file header 2
//   File data 2
//   [ Local file footer 2 ]
//   ...
//   Local file header n
//   [ Local file footer n ]
//   File data n
//   --- START OF CENTRAL DIRECTORY
//   File header 1
//   File header 2
//   ...
//   File header n
//   File footer
//   --- END OF FILE
//
// All fields are stored in little-endian, and location of all fields must be
// done via the 'central directory'. Indeed, the central directory must be
// located by looking for the 'footer' and then using that to locate the start
// of the file headers that constitute it. It is suggested that the scan for the
// footer occur from the end of the file.

enum
{
	kZipFlagFileHasFooter = 1 << 3
};

enum
{
	kZipCompressionStore = 0,
	kZipCompressionShrink = 1,
	kZipCompressionReduce1 = 2,
	kZipCompressionReduce2 = 3,
	kZipCompressionReduce3 = 4,
	kZipCompressionReduce4 = 5,
	kZipCompressionImplode = 6,
	kZipCompressionDeflate = 8,
	kZipCompressionDeflate64 = 9,
	kZipCompressionOldTerse = 10,
	kZipCompressionBZip2 = 12,
	kZipCompressionLZMA = 14,
	kZipCompressionNewTerse = 18,
	kZipCompressionLZ77 = 19,
	kZipCompressionWavPack = 97,
	kZipCompressionPPMdI1 = 98
};

// The local file header precedes the data for the file.
struct ZipLocalFileHeader
{
	// Local file header signature = 0x04034b50
	uint32_t signature;
	// Version needed to extract (minimum)
	uint16_t version;
	// General purpose flag bit
	uint16_t flags;
	// Compression method
	uint16_t compression;
	// File last modification time
	uint16_t last_modified_time;
	// File last modification date
	uint16_t last_modified_date;
	// PAD
	uint16_t __pad0;
	// CRC-32
	uint32_t checksum;
	// Compressed size
	uint32_t compressed_size;
	// Uncompressed size
	uint32_t uncompressed_size;
	// File name length
	uint16_t filename_length;
	// Extra field length
	uint16_t extra_length;
	// File name
	// uint8_t filename[filename_length]
	// Extra field
	// uint8_t extra[extra_length]
};

#define kZipLocalFileHeaderSignature "PK\x03\x04"
#define kZipLocalFileHeaderSize (sizeof(ZipLocalFileHeader) - sizeof(uint16_t))

// The local file footer is present if bit 3 of the 'flags' field is set. It
// contains the values for fields that can only be computed when the data has
// been processed. In this case the corresponding fields in the header are 0.
struct ZipLocalFileFooter
{
	uint32_t checksum;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
};

// The file header appears in the central directory for each file in the
// zip.
struct ZipFileHeader
{
	// Central directory file header signature = 0x02014b50
	uint32_t signature;
	// Version made by
	uint16_t creator_version;
	// Version needed to extract (minimum)
	uint16_t minimum_version;
	// General purpose bit flag
	uint16_t flags;
	// Compression method
	uint16_t compression;
	// File last modification time
	uint16_t last_modified_time;
	// File last modification date
	uint16_t last_modified_date;
	// CRC-32
	uint32_t checksum;
	// Compressed size
	uint32_t compressed_size;
	// Uncompressed size
	uint32_t uncompressed_size;
	// File name length
	uint16_t filename_length;
	// Extra field length
	uint16_t extra_length;
	// File comment length
	uint16_t comment_length;
	// Disk number where file starts
	uint16_t first_disk;
	// Internal file attributes
	uint16_t internal_attrs;
	// Ensure natural alignment (not present in file)
	uint16_t __pad0;
	// External file attributes
	uint32_t external_attrs;
	// Relative offset of local file header (from this header)
	uint32_t file_header_offset;
	// File name
	// uint8_t filename[filename_length];
	// Extra data
	// uint8_t extra[extra_length];
	// Comment data
	// uint8_t comment[comment_length];
};

#define kZipFileHeaderSignature "PK\x01\x02"

// Compute disk size of header - i.e. without padding
#define kZipFileHeaderSize (sizeof(ZipFileHeader) - sizeof(uint16_t))

// The file footer terminates the list of central directory file
// header entries.
struct ZipFileFooter
{
	// Central directory file header signature = 0x06054b50
	uint32_t signature;
	// Number of this disk
	uint16_t current_disk;
	// Disk where central directory starts
	uint16_t first_disk;
	// Number of central directory records on this disk
	uint16_t current_length;
	// Total number of central directory records
	uint16_t total_length;
	// Size of the central directory
	uint32_t size;
	// Offset to the central directory from start of archive
	uint32_t offset;
	// ZIP file comment length
	uint16_t comment_length;
	// ZIP file comment
	// uint8_t comment[comment_length];
};

#define kZipFileFooterSignature "PK\x05\x06"

// Compute disk size of footer - i.e. without padding
#define kZipFileFooterSize (sizeof(ZipFileFooter) - sizeof(uint16_t))

////////////////////////////////////////////////////////////////////////////////

struct MCMiniZipStream
{
	const uint8_t *data;
	uint32_t data_length;

	uint32_t pointer;
};

struct MCMiniZipItem
{
	MCStringRef name;
	uint32_t compression;
	uint32_t checksum;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint32_t header_offset;
};

struct MCMiniZip
{
	// The memory address of the archive and its length (we assume the archive
	// is mapped into memory/already loaded).
	const uint8_t *data;
	uint32_t data_size;

	// The array of items we have read from the central dir.
	uint32_t item_count;
	MCMiniZipItem *items;
};

static void decode_struct(const void *p_data, const char *p_format, void *p_struct);
static bool MCMiniZipReadCentralDir(MCMiniZipRef self);

////////////////////////////////////////////////////////////////////////////////

static MCMiniZipError s_error = kMCMiniZipErrorNone;

bool MCMiniZipOpen(const void *p_data, uint32_t p_data_size, MCMiniZipRef& r_minizip)
{
	bool t_success;
	t_success = true;

	MCMiniZipRef self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);

	if (t_success)
	{
		self -> data = (const uint8_t *)p_data;
		self -> data_size = p_data_size;
		t_success = MCMiniZipReadCentralDir(self);
	}

	if (t_success)
	{
		r_minizip = self;
	}
	else
		MCMiniZipClose(self);

	return t_success;
}

void MCMiniZipClose(MCMiniZipRef self)
{
	if (self == nil)
		return;

	for(uint32_t i = 0; i < self -> item_count; i++)
		MCValueRelease(self->items[i].name);

	MCMemoryDeleteArray(self -> items);
	MCMemoryDelete(self);
}

//////////

bool MCMiniZipListItems(MCMiniZipRef self, MCMiniZipListItemsCallback p_callback, void *p_context)
{
	for(uint32_t i = 0; i < self -> item_count; i++)
		if (!p_callback(p_context, self -> items[i] . name))
			return false;

	return true;
}

bool MCMiniZipDescribeItem(MCMiniZipRef self, MCStringRef p_item_name, MCMiniZipItemInfo& r_info)
{
	// Search our items for the requested one
	MCMiniZipItem *t_item;
	t_item = nil;
	for(uint32_t i = 0; i < self -> item_count; i++)
		if (MCStringIsEqualTo(p_item_name, self -> items[i] . name, kMCStringOptionCompareCaseless))
		{
			t_item = &self -> items[i];
			break;
		}

	// If it wasn't found, throw an error
	if (t_item == nil)
		return MCMiniZipThrow(kMCMiniZipErrorItemNotFound);

	r_info . uncompressed_size = t_item -> uncompressed_size;
	r_info . compressed_size = t_item -> compressed_size;
	r_info . checksum = t_item -> checksum;
	r_info . compression = 0;

	return true;
}

static bool extract_item_to_memory(void *context, const void *data, uint32_t data_length, uint32_t data_offset, uint32_t data_total)
{
	MCMemoryCopy((char *)context + data_offset, data, data_length);
	return true;
}

bool MCMiniZipExtractItemToMemory(MCMiniZipRef self, MCStringRef p_item, void*& r_bytes, uint32_t& r_byte_count)
{
	bool t_success;
	t_success = true;

	MCMiniZipItemInfo t_info;
	if (t_success)
		t_success = MCMiniZipDescribeItem(self, p_item, t_info);

	void *t_data;
	t_data = nil;
	if (t_success)
		t_success = MCMemoryAllocate(t_info . uncompressed_size, t_data);

	if (t_success)
		t_success = MCMiniZipExtractItem(self, p_item, extract_item_to_memory, t_data);

	if (t_success)
	{
		r_bytes = t_data;
		r_byte_count = t_info . uncompressed_size;
	}
	else
		MCMemoryDeallocate(t_data);

	return t_success;
}

bool MCMiniZipExtractItem(MCMiniZipRef self, MCStringRef p_item_name, MCMiniZipExtractItemCallback p_callback, void *p_context)
{
	// Search our items for the requested one
	MCMiniZipItem *t_item;
	t_item = nil;
	for(uint32_t i = 0; i < self -> item_count; i++)
		if (MCStringIsEqualTo(p_item_name, self -> items[i] . name, kMCStringOptionCompareCaseless))
		{
			t_item = &self -> items[i];
			break;
		}

	// If it wasn't found, throw an error
	if (t_item == nil)
		return MCMiniZipThrow(kMCMiniZipErrorItemNotFound);

	// Check that the item header has the correct signature
	if (!MCMemoryEqual(self -> data + t_item -> header_offset, kZipLocalFileHeaderSignature, 4))
		return MCMiniZipThrow(kMCMiniZipErrorItemMismatch);

	// Now decode the local file header
	ZipLocalFileHeader t_header;
	decode_struct(self -> data + t_item -> header_offset, "ISSSSSIIISS", &t_header);

	// Work out where the 'compressed' data is and that it is all there
	uint32_t t_data_offset;
	t_data_offset = t_item -> header_offset + kZipLocalFileHeaderSize + t_header . filename_length + t_header . extra_length;
	if (t_data_offset + t_item -> compressed_size > self -> data_size)
		return MCMiniZipThrow(kMCMiniZipErrorIncompleteItem);

	// Right, now we can extract and what we do depends on the compression
	// method.
	bool t_success;
	t_success = true;

	switch(t_item -> compression)
	{
	// 'Store' corresponds to no compression - the trivial case
	case kZipCompressionStore:
		{
			// First check the crc
			uint32_t t_checksum;
			t_checksum = crc32(0, self -> data + t_data_offset, t_item -> uncompressed_size);
			if (t_checksum != t_item -> checksum)
				t_success = MCMiniZipThrow(kMCMiniZipErrorChecksumFailed);
			if (t_success)
				t_success = p_callback(p_context, self -> data + t_data_offset, t_item -> uncompressed_size, 0, t_item -> uncompressed_size);
		}
		break;

	// 'Deflate' corresponds to zlib compression
	case kZipCompressionDeflate:
		{
			z_stream t_stream;
			MCMemoryClear(&t_stream, sizeof(z_stream));

			// Allocate a temporary buffer
			uint8_t *t_buffer;
			uint32_t t_buffer_size;
			t_buffer = nil;
			t_buffer_size = 65536;
			if (t_success && !MCMemoryNewArray(t_buffer_size, t_buffer))
				t_success = MCMiniZipThrow(kMCMiniZipErrorNoMemory);

			// Point the input buffer to the compressed data and init the
			// stream.
			if (t_success)
			{
				t_stream . next_in = (Bytef *)self -> data + t_data_offset;
				t_stream . avail_in = t_item -> compressed_size;
				if (inflateInit2(&t_stream, -MAX_WBITS) != Z_OK)
					t_success = MCMiniZipThrow(kMCMiniZipErrorInflateFailed);
			}

			// Now loop until we have decompressed everything
			uint32_t t_offset, t_checksum;
			t_offset = 0;
			t_checksum = 0;
			while(t_success)
			{
				// Point to the output buffer
				t_stream . next_out = t_buffer;
				t_stream . avail_out = 65536;
				
				// Inflate as much as we can
				int t_status;
				t_status = inflate(&t_stream, Z_SYNC_FLUSH);
				if (t_status != Z_OK && t_status != Z_STREAM_END)
					t_success = MCMiniZipThrow(kMCMiniZipErrorInflateFailed);

				// If we managed to, and there is data there then invoke
				// the callback
				uint32_t t_available;
				if (t_success)
				{
					t_available = t_buffer_size - t_stream . avail_out;
					if (t_available > 0)
						t_success = p_callback(p_context, t_buffer, t_available, t_offset, t_item -> uncompressed_size);
				}

				// Advance the offset, and upate the checksum
				if (t_success && t_available > 0)
				{
					t_checksum = crc32(t_checksum, t_buffer, t_available);
					t_offset += t_available;
				}

				// If we reached the end of the stream, we are done.
				if (t_success && t_status == Z_STREAM_END)
					break;
			}

			// Now check that the checksum is correct
			if (t_success && t_checksum != t_item -> checksum)
				t_success = MCMiniZipThrow(kMCMiniZipErrorChecksumFailed);

			// Cleanup the buffer and stream
			inflateEnd(&t_stream);
			MCMemoryDeleteArray(t_buffer);
		}
		break;
	}

	return t_success;
}

//////////

bool MCMiniZipThrow(MCMiniZipError p_error)
{
	s_error = p_error;
	return true;
}

MCMiniZipError MCMiniZipCatch(void)
{
	return s_error;
}

////////////////////////////////////////////////////////////////////////////////

static inline uint32_t swap_to_le(uint32_t v)
{
#ifdef __BIG_ENDIAN__
	return ((v & 0xff) << 24) | ((v & 0xff00) << 8) | ((v >> 8) & 0xff00) | ((v >> 24) & 0xff);
#else
	return v;
#endif
}

static inline uint16_t swap_to_le(uint16_t v)
{
#ifdef __BIG_ENDIAN__
	return ((v & 0xff) << 8) | ((v & 0xff00) >> 8);
#else
	return v;
#endif
}

// Decode a byte-stream into a struct. Note we assume that the struct has natural
// alignment for all fields.
static void decode_struct(const void *p_data, const char *p_format, void *p_struct)
{
	while(*p_format != '\0')
	{
		switch(*p_format++)
		{
		case 'i':
		case 'I': // little-endian uint32_t
		{
			uint32_t t_value;
			MCMemoryCopy(&t_value, p_data, sizeof(uint32_t));
			
			p_struct = (void *)(((uintptr_t)p_struct + 3) & ~3);
			*(uint32_t *)p_struct = swap_to_le(t_value);
			p_struct = (uint8_t *)p_struct + sizeof(uint32_t);

			p_data = (const uint8_t *)p_data + sizeof(uint32_t);
		}
		break;
		case 's':
		case 'S': // little-endian uint16_t
		{
			uint16_t t_value;
			MCMemoryCopy(&t_value, p_data, sizeof(uint16_t));

			p_struct = (void *)(((uintptr_t)p_struct + 1) & ~1);
			*(uint16_t *)p_struct = swap_to_le(t_value);
			p_struct = (uint8_t *)p_struct + sizeof(uint16_t);

			p_data = (const uint8_t *)p_data + sizeof(uint16_t);
		}
		break;
		}
	}
}

static bool MCMiniZipReadCentralDir(MCMiniZipRef self)
{
	// The first thing to do is to find the footer entry of the central dir. We
	// do this by repeatedly searching for footer signatures and then validating
	// that we have indeed found a footer, until we actually do find one.
	// Note that we assume the first footer we find which has integrity is the
	// real footer (we need do no more, since we control the input archives).

	if (self -> data_size == 0)
		return MCMiniZipThrow(kMCMiniZipErrorNoArchive);

	// The footer cannot start after 'data-length' - sizeof(ZipFileFooter), thus
	// this is where we start searching.
	uint32_t t_footer_offset;
	bool t_footer_found;
	ZipFileFooter t_footer;
	t_footer_offset = self -> data_size - kZipFileFooterSize;
	t_footer_found = false;
	for(;;)
	{
		// Check to see if the footer sig as at the current offset
		if (MCMemoryEqual(self -> data + t_footer_offset, kZipFileFooterSignature, 4))
		{
			// Parse the format structure (we can't read directly as might not
			// be word-aligned and might be a different endian).
			decode_struct(self -> data + t_footer_offset, "ISSSSIIS", &t_footer);

			// Check the following:
			//   1) There is room for the comment field
			//   2) The offset is sensible
			//   3) The size is sensible
			//   4) The total_length is greater or equal to the current_length
			//   5) The current_length is sensible
			if (t_footer_offset + kZipFileFooterSize + t_footer . comment_length <= self -> data_size &&
				t_footer . offset < t_footer_offset &&
				t_footer . size < self -> data_size &&
				t_footer . current_length <= t_footer . total_length &&
				t_footer . current_length * sizeof(ZipFileHeader) <= t_footer . size)
			{
				t_footer_found = true;
				break;
			}
		}

		// If we are already at 0 offset, we can do no more
		if (t_footer_offset == 0)
			break;

		// Otherwise skip back 1 byte and try again
		t_footer_offset -= 1;
	}

	// If we failed to find the footer, its an error
	if (!t_footer_found)
		return MCMiniZipThrow(kMCMiniZipErrorNoFooter);

	// Next allocate room for the central dir
	if (!MCMemoryNewArray(t_footer . total_length, self -> items))
		return MCMiniZipThrow(kMCMiniZipErrorNoMemory);

	// Now we must read in the central directory.
	uint32_t t_header_offset;
	t_header_offset = t_footer . offset;
	for(uint32_t i = 0; i < t_footer . current_length; i++)
	{
		// If there isn't room for another header, its an error
		if (t_header_offset + kZipFileHeaderSize > t_footer_offset)
			return MCMiniZipThrow(kMCMiniZipErrorMissingHeaders);

		// If the header doesn't start with the signature, its an error
		if (!MCMemoryEqual(self -> data + t_header_offset, kZipFileHeaderSignature, 4))
			return MCMiniZipThrow(kMCMiniZipErrorHeaderNotFound);

		// Otherwise we decode the next header
		ZipFileHeader t_header;
		decode_struct(self -> data + t_header_offset, "ISSSSSSIIISSSSSII", &t_header);

		// Check that there is room for filename, extra and comment fields
		if (t_header_offset + t_header . filename_length + t_header . extra_length + t_header . comment_length > t_footer_offset)
			return MCMiniZipThrow(kMCMiniZipErrorIncompleteHeader);

		// Check that offset makes sense
		if (t_header . file_header_offset > t_footer . offset)
			return MCMiniZipThrow(kMCMiniZipErrorMalformedHeader);

		// Check that the compressed size makes sense
		if (t_header . compressed_size > t_footer . offset)
			return MCMiniZipThrow(kMCMiniZipErrorMalformedHeader);

		// Otherwise read the item info we need to store
		self -> items[i] . compression = t_header . compression;
		self -> items[i] . checksum = t_header . checksum;
		self -> items[i] . compressed_size = t_header . compressed_size;
		self -> items[i] . uncompressed_size = t_header . uncompressed_size;
		self -> items[i] . header_offset = t_header . file_header_offset;

		// Clone the filename string
		if (!MCStringCreateWithBytes((const byte_t *)(self->data + t_header_offset + kZipFileHeaderSize), t_header.filename_length, kMCStringEncodingNative, false, self->items[i].name))
			return MCMiniZipThrow(kMCMiniZipErrorNoMemory);

		// Increment the number of items
		self -> item_count += 1;

		// Advance to the next header
		t_header_offset += kZipFileHeaderSize + t_header . filename_length + t_header . extra_length + t_header . comment_length;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
