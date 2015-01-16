/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#ifndef __MC_FOUNDATION_CHUNK__
#define __MC_FOUNDATION_CHUNK__

#include "foundation.h"

typedef uinteger_t (MCChunkCountCallback(void *context));

uinteger_t MCChunkCountByteChunkCallback(void *context);
uinteger_t MCChunkCountCodepointChunkCallback(void *context);

uindex_t MCChunkCountChunkChunks(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options);

void MCChunkGetExtentsByRange(integer_t p_first, integer_t p_last, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsByExpression(integer_t p_first, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsOfByteChunkByRange(MCDataRef p_data, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count);
void MCChunkGetExtentsOfByteChunkByExpression(MCDataRef p_data, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsOfCodeunitChunkByRange(MCStringRef p_data, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count);
void MCChunkGetExtentsOfCodeunitChunkByExpression(MCStringRef p_data, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsOfElementChunkByRange(MCProperListRef p_string, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count);
void MCChunkGetExtentsOfElementChunkByExpression(MCProperListRef p_string, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsOfChunkChunkByRange(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count);
void MCChunkGetExtentsOfChunkChunkByExpression(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count);

bool MCChunkIsAmongTheChunksOfRange(MCStringRef p_chunk, MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, MCRange p_range);

bool MCChunkOffsetOfChunkInRange(MCStringRef p_string, MCStringRef p_needle, MCStringRef p_delimiter, bool p_whole_matches, MCStringOptions p_options, MCRange p_range, uindex_t& r_offset);

typedef bool (*MCChunkApplyCallback)(void *context, MCStringRef string, MCRange chunk_range);
bool MCChunkApply(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, MCChunkApplyCallback p_callback, void *context);

bool MCChunkIterate(MCRange& x_range, MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, bool p_first);

#endif // __MC_FOUNDATION_CHUNK__