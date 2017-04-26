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

#ifndef __MC_UUID__
#define __MC_UUID__

#define kMCUuidCStringLength 37
#define kMCUuidStringLength 36

enum MCUuidType
{
	kMCUuidTypeUnknown,
	kMCUuidTypeTime,
	kMCUuidTypeDCE,
	kMCUuidTypeMD5,
	kMCUuidTypeRandom,
	kMCUuidTypeSHA1,
};

struct MCUuid
{
	uint32_t time_low;
	uint16_t time_mid;
	uint16_t time_hi_and_version;
	uint8_t clock_seq_hi_and_reserved;
	uint8_t clock_seq_low;
	uint8_t node[6];
};

// Generate a (random) type v4 UUID.
// This returns false if generation failed (due to lack of random bytes).
bool MCUuidGenerateRandom(MCUuid& r_uuid);

// Generate a (non-random) type v3 UUID using the given name and namespace
// id.
void MCUuidGenerateMD5(const MCUuid& namespace_id, MCStringRef name, MCUuid& r_uuid);

// Generate a (non-random) type v4 UUID using the given name and namespace
// id.
void MCUuidGenerateSHA1(const MCUuid& namespace_id, MCStringRef name, MCUuid& r_uuid);

// Convert a UUID to a string. The output buffer is expected to be at
// least 37 bytes long.
void MCUuidToCString(const MCUuid& uuid, char t_uuid_string[kMCUuidCStringLength]);

// Convert a UUID from a string. Returns false if the string is not a valid
// uuid.
bool MCUuidFromCString(const char *string, MCUuid& r_uuid);

// Encode a UUID as a endian-invariant sequence of bytes.
void MCUuidToBytes(const MCUuid& uuid, uint8_t r_bytes[16]);

// Decode a UUID from an endian-invariant sequence of bytes.
void MCUuidFromBytes(uint8_t bytes[16], MCUuid& r_uuid);

#endif
