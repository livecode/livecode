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


#ifndef __MC_MINIZIP__
#define __MC_MINIZIP__

////////////////////////////////////////////////////////////////////////////////

enum MCMiniZipError
{
	kMCMiniZipErrorNone,

	kMCMiniZipErrorNoMemory,

	kMCMiniZipErrorNoArchive,
	kMCMiniZipErrorNoFooter,
	kMCMiniZipErrorMissingHeaders,
	kMCMiniZipErrorHeaderNotFound,
	kMCMiniZipErrorIncompleteHeader,
	kMCMiniZipErrorMalformedHeader,

	kMCMiniZipErrorItemNotFound,
	kMCMiniZipErrorItemMismatch,
	kMCMiniZipErrorIncompleteItem,

	kMCMiniZipErrorChecksumFailed,
	kMCMiniZipErrorInflateFailed,
};

bool MCMiniZipThrow(MCMiniZipError status);
MCMiniZipError MCMiniZipCatch(void);

const char *MCMiniZipErrorToString(MCMiniZipError p_error);

////////////////////////////////////////////////////////////////////////////////

typedef struct MCMiniZip *MCMiniZipRef;

typedef bool (*MCMiniZipListItemsCallback)(void *r_list, MCStringRef name);
typedef bool (*MCMiniZipExtractItemCallback)(void *context, const void *data, uint32_t data_length, uint32_t data_offset, uint32_t data_total);

struct MCMiniZipItemInfo
{
	uint32_t checksum;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint32_t compression;
};

bool MCMiniZipOpen(const void *p_data, uint32_t p_data_length, MCMiniZipRef& r_minizip);
void MCMiniZipClose(MCMiniZipRef self);

bool MCMiniZipListItems(MCMiniZipRef self, MCMiniZipListItemsCallback callback, void *context);
bool MCMiniZipDescribeItem(MCMiniZipRef self, MCStringRef p_item, MCMiniZipItemInfo& r_infO);
bool MCMiniZipExtractItem(MCMiniZipRef self, MCStringRef p_item, MCMiniZipExtractItemCallback callback, void *context);
bool MCMiniZipExtractItemToMemory(MCMiniZipRef self, MCStringRef p_item, void*& r_bytes, uint32_t& r_byte_count);

////////////////////////////////////////////////////////////////////////////////

#endif
