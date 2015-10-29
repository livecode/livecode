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
 *  sserialize.h
 *  libcore
 *
 *  Created by Ian Macphail on 22/10/2009.
 *
 */

#ifndef __MC_SSERIALIZE_H__
#define __MC_SSERIALIZE_H__

#include "core.h"

extern bool serialize_bytes(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, const void *p_data, uint32_t p_data_size);
extern bool deserialize_bytes(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, void *p_dest, uint32_t p_size);

extern bool serialize_uint32(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, uint32_t p_val);
extern bool deserialize_uint32(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, uint32_t &r_val);

extern bool serialize_data(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, const void *p_data, uint32_t p_data_size);
extern bool deserialize_data(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, void *&r_data, uint32_t &r_size);

#endif
