/* Copyright (C) 2009-2015 LiveCode Ltd.

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

/*
 *  sserialize.cpp
 *  libcore
 *
 *  Created by Ian Macphail on 22/10/2009.
 *
 */

#include "sserialize.h"

#include "core.h"

bool serialize_bytes(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, const void *p_data, uint32_t p_data_size)
{
	if (p_data_size == 0)
		return true;
	if (r_offset + p_data_size > r_stream_size)
	{
		char *t_stream;
		uint32_t t_size;
		t_size = r_offset + p_data_size;
		if (!MCMemoryReallocate(r_stream, t_size, t_stream))
			return false;
		r_stream = t_stream;
		r_stream_size = t_size;
	}
	MCMemoryCopy(r_stream + r_offset, p_data, p_data_size);
	r_offset += p_data_size;

	return true;
}

bool deserialize_bytes(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, void *p_dest, uint32_t p_size)
{
	if (r_offset + p_size > p_stream_size)
		return false;

	MCMemoryCopy(p_dest, p_stream + r_offset, p_size);
	r_offset += p_size;
	return true;
}

bool serialize_uint32(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, uint32_t p_val)
{
	return serialize_bytes(r_stream, r_stream_size, r_offset, &p_val, sizeof(uint32_t));
}

bool deserialize_uint32(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, uint32_t &r_val)
{
	return deserialize_bytes(p_stream, p_stream_size, r_offset, &r_val, sizeof(uint32_t));
}

bool serialize_data(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, const void *p_data, uint32_t p_data_size)
{
	if (serialize_uint32(r_stream, r_stream_size, r_offset, p_data_size) &&
		serialize_bytes(r_stream, r_stream_size, r_offset, p_data, p_data_size))
		return true;
	return false;
}

bool deserialize_data(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, void *&r_data, uint32_t &r_size)
{
	uint32_t t_size = 0, t_data_size = 0;
	bool t_success = true;
	void *t_data = nil;
	t_success = deserialize_uint32(p_stream, p_stream_size, r_offset, t_data_size);
	if (t_success)
	{
		if (t_data_size == 0)
		{
			r_size = 0;
			return true;
		}
		if (r_data == nil)
		{
			t_size = t_data_size;
			MCMemoryAllocate(t_size, t_data);
		}
		else
		{
			t_size = r_size;
			t_data = r_data;
		}
		t_success = (t_data != nil && t_data_size <= t_size);
	}
	if (t_success)
	{
		MCMemoryCopy(t_data, p_stream + r_offset, t_data_size);
		r_data = t_data;
		r_size = t_size;
		r_offset += t_data_size;
	}
	return t_success;
}

