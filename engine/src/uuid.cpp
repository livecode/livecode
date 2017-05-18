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

#include "md5.h"
#include "sha1.h"
#include "util.h"

#include "uuid.h"

////////////////////////////////////////////////////////////////////////////////

static void MCUuidBrand(MCUuid& p_uuid, unsigned int p_version)
{
	// The version is stored in the top four bits of 'time_hi_and_version'.
	p_uuid . time_hi_and_version &= 0x0fff;
	p_uuid . time_hi_and_version |= p_version << 12;
	
	// The variant is stored in the top 2 bits of 'clock_seq_hi_and_reserved'.
	// It is always DCE 1.1 - 2.
	p_uuid . clock_seq_hi_and_reserved &= 0x3f;
	p_uuid . clock_seq_hi_and_reserved |= 0x02 << 6;
}

////////////////////////////////////////////////////////////////////////////////

bool MCUuidGenerateRandom(MCUuid& r_uuid)
{
	// Fill the UUID with random bytes (returns false if not enough random data
	// is available).
    MCAutoDataRef t_data;
	if (!MCSRandomData (sizeof(MCUuid), &t_data))
		return false;
    MCMemoryCopy(&r_uuid, MCDataGetBytePtr(*t_data), sizeof(MCUuid));
    
	// Now 'brand' the UUID with version 4.
	MCUuidBrand(r_uuid, 4);
	
	return true;
}

void MCUuidGenerateMD5(const MCUuid& p_namespace_id, MCStringRef p_name, MCUuid& r_uuid)
{
	// Initialize an md5 context.
	md5_state_t t_md5;
	md5_init(&t_md5);
	
	// Convert the namespace id to bytes.
	uint8_t t_namespace_bytes[16];
	MCUuidToBytes(p_namespace_id, t_namespace_bytes);
	
	// Append the namespace bytes to the md5 stream.
	md5_append(&t_md5, t_namespace_bytes, sizeof(t_namespace_bytes));
	
	// Append the name bytes to the md5 stream.
    MCAutoStringRefAsCString t_name;
    /* UNCHECKED */ t_name.Lock(p_name);
    md5_append(&t_md5, reinterpret_cast<const md5_byte_t*>(*t_name),
               t_name.Size());
	
	// Extract the resulting digest from the md5 stream.
	uint8_t t_uuid_bytes[16];
	md5_finish(&t_md5, t_uuid_bytes);
	
	// Now import the uuid bytes as a uuid.
	MCUuidFromBytes(t_uuid_bytes, r_uuid);
	
	// Finally brand the uuid with version 3.
	MCUuidBrand(r_uuid, 3);
}

void MCUuidGenerateSHA1(const MCUuid& p_namespace_id, MCStringRef p_name, MCUuid& r_uuid)
{
	// Initialize an sha1 context.
	sha1_state_t t_sha1;
	sha1_init(&t_sha1);
	
	// Convert the namespace id to bytes.
	uint8_t t_namespace_bytes[16];
	MCUuidToBytes(p_namespace_id, t_namespace_bytes);
	
	// Append the namespace bytes to the sha1 stream.
	sha1_append(&t_sha1, t_namespace_bytes, sizeof(t_namespace_bytes));
	
	// Append the name bytes to the sha1 stream.
    MCAutoStringRefAsCString t_name;
    /* UNCHECKED */ t_name.Lock(p_name);
    sha1_append(&t_sha1, *t_name, t_name.Size());
	
	// Extract the resulting digest from the sha1 stream.
	uint8_t t_uuid_bytes[20];
	sha1_finish(&t_sha1, t_uuid_bytes);
	
	// Now import the uuid bytes as a uuid. Note that UuidFromBytes will only use
	// the first 16 bytes of the sha1 digest - as required by the standard.
	MCUuidFromBytes(t_uuid_bytes, r_uuid);
	
	// Finally brand the uuid with version 5.
	MCUuidBrand(r_uuid, 5);
}

////////////////////////////////////////////////////////////////////////////////

void MCUuidToCString(const MCUuid& p_uuid, char r_uuid_string[kMCUuidCStringLength])
{
	sprintf(r_uuid_string,
			"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			(unsigned long)p_uuid . time_low,
			(unsigned int)p_uuid . time_mid,
			(unsigned int)p_uuid . time_hi_and_version,
			(unsigned int)p_uuid . clock_seq_hi_and_reserved,
			(unsigned int)p_uuid . clock_seq_low,
			(unsigned int)p_uuid . node[0],
			(unsigned int)p_uuid . node[1],
			(unsigned int)p_uuid . node[2],
			(unsigned int)p_uuid . node[3],
			(unsigned int)p_uuid . node[4],
			(unsigned int)p_uuid . node[5]);
}

bool MCUuidFromCString(const char *p_string, MCUuid& r_uuid)
{
	// Check the length of the string is correct.
	if (strlen(p_string) != kMCUuidStringLength)
		return false;
		
	// Check the structure of the string.
	for(int i = 0; i < kMCUuidStringLength; i++)
	{
		if (i == 8 || i == 13 || i == 18 || i == 23)
		{
			if (p_string[i] != '-')
				return false;
				
			continue;
		}
		
		if (!isxdigit(p_string[i]))
			return false;
	}
		
	// Parse the 'time' parts.
	r_uuid . time_low = (uint32_t)strtoul(p_string + 0, NULL, 16);
	r_uuid . time_mid = (uint32_t)strtoul(p_string + 9, NULL, 16);
	r_uuid . time_hi_and_version = (uint32_t)strtoul(p_string + 14, NULL, 16);
	
	// Parse the 'clock' parts.
	uint16_t t_clock;
	t_clock = (uint16_t)strtoul(p_string + 19, NULL, 16);
	r_uuid . clock_seq_low = (t_clock >> 0) & 0xff;
	r_uuid . clock_seq_hi_and_reserved = (t_clock >> 8) & 0xff;
	
	// Parse the 'node' part.
	for(size_t i = 0; i < sizeof(r_uuid . node); i++)
	{
		char t_hex_byte[3];
		t_hex_byte[0] = p_string[24 + i * 2];
		t_hex_byte[1] = p_string[24 + i * 2 + 1];
		t_hex_byte[2] = 0;
		r_uuid . node[i] = (uint8_t)strtoul(t_hex_byte, NULL, 16);
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCUuidToBytes(const MCUuid& p_uuid, uint8_t r_bytes[16])
{
	// Pack the 'time_low' field.
	r_bytes[3] = (uint8_t)((p_uuid . time_low >> 0) & 0xff);
	r_bytes[2] = (uint8_t)((p_uuid . time_low >> 8) & 0xff);
	r_bytes[1] = (uint8_t)((p_uuid . time_low >> 16) & 0xff);
	r_bytes[0] = (uint8_t)((p_uuid . time_low >> 24) & 0xff);
	
	// Pack the 'time_mid' field.
	r_bytes[5] = (uint8_t)((p_uuid . time_mid >> 0) & 0xff);
	r_bytes[4] = (uint8_t)((p_uuid . time_mid >> 8) & 0xff);
	
	// Pack the 'time_hi_and_version' field.
	r_bytes[7] = (uint8_t)((p_uuid . time_hi_and_version >> 0) & 0xff);
	r_bytes[6] = (uint8_t)((p_uuid . time_hi_and_version >> 8) & 0xff);
	
	// Pack the 'clock_seq_hi_and_reserved' field.
	r_bytes[8] = p_uuid . clock_seq_hi_and_reserved;

	// Pack the 'clock_seq_low' field.
	r_bytes[9] = p_uuid . clock_seq_low;
	
	// Pack the 'node' field.
	for(unsigned int i = 0; i < sizeof(p_uuid . node); i++)
		r_bytes[10 + i] = p_uuid . node[i];
}

void MCUuidFromBytes(uint8_t p_bytes[16], MCUuid& r_uuid)
{
	// Unpack the 'time_low' field.
	r_uuid . time_low = 
				(p_bytes[0] << 24) |
				(p_bytes[1] << 16) |
				(p_bytes[2] << 8) |
				(p_bytes[3] << 0);
				
				
	// Unpack the 'time_mid' field.
	r_uuid . time_mid =
				(p_bytes[4] << 8) |
				(p_bytes[5] << 0);
				
	// Unpack the 'time_hi_and_version' field.
	r_uuid . time_hi_and_version =
				(p_bytes[7] << 8) |
				(p_bytes[6] << 0);
				
	// Unpack the 'clock_seq_hi_and_reserved' field.
	r_uuid . clock_seq_hi_and_reserved =
				p_bytes[8];
	
	// Pack the 'clock_seq_low' field.
	r_uuid . clock_seq_low =
				p_bytes[9];
				
	// Pack the 'node' field.
	for(size_t i = 0; i < sizeof(r_uuid . node); i++)
		r_uuid . node[i] = p_bytes[10 + i];
}

////////////////////////////////////////////////////////////////////////////////
