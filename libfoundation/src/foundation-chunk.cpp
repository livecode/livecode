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

#include <foundation.h>

#include "foundation-chunk.h"

////////////////////////////////////////////////////////////////////////////////

uinteger_t MCChunkCountByteChunkCallback(void *context)
{
    MCChunkCountInRangeState *t_state = static_cast<MCChunkCountInRangeState *>(context);
    
    uinteger_t t_length;
    t_length = MCDataGetLength((MCDataRef)t_state -> value);
    
    if (t_state -> range == nil)
        return t_length;

    MCRange t_range;
    t_range = *t_state -> range;
    
    return t_range . offset + t_range . length > t_length ? t_length - t_range . offset : t_range . length;
}

uinteger_t MCChunkCountCodeunitChunkCallback(void *context)
{
    MCChunkCountInRangeState *t_state = static_cast<MCChunkCountInRangeState *>(context);
    
    uinteger_t t_length;
    t_length = MCStringGetLength((MCStringRef)t_state -> value);
    
    if (t_state -> range == nil)
        return t_length;
    
    MCRange t_range;
    t_range = *t_state -> range;
    
    return t_range . offset + t_range . length > t_length ? t_length - t_range . offset : t_range . length;
}

uinteger_t MCChunkCountElementChunkCallback(void *context)
{
    MCChunkCountInRangeState *t_state = static_cast<MCChunkCountInRangeState *>(context);
    
    uinteger_t t_length;
    t_length = MCProperListGetLength((MCProperListRef)t_state -> value);
    
    if (t_state -> range == nil)
        return t_length;
    
    MCRange t_range;
    t_range = *t_state -> range;
    
    return t_range . offset + t_range . length > t_length ? t_length - t_range . offset : t_range . length;
}

////////////////////////////////////////////////////////////////////////////////

// AL-2015-02-10: [[ Bug 14532 ]] Allow chunk extents to be counted in a given range, to prevent substring copying in text chunk resolution.
void MCChunkGetExtentsByRangeInRange(integer_t p_first, integer_t p_last, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count)
{
    int32_t t_chunk_count;
    
    if (p_first < 0 || p_last < 0)
    {
        uinteger_t t_count;
        t_count = p_callback(p_context);
        
        if (p_first < 0)
            p_first += t_count;
        else
            p_first--;
        
        if (p_last < 0)
            p_last += t_count + 1;
    }
    else
        p_first--;
    
    t_chunk_count = p_last - p_first;
    
    if (p_first < 0)
    {
        t_chunk_count += p_first;
        p_first = 0;
    }
    
    if (t_chunk_count < 0)
        t_chunk_count = 0;
    
    r_chunk_count = t_chunk_count;
    r_first = p_first;
}

void MCChunkGetExtentsByExpressionInRange(integer_t p_first, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count)
{
    r_chunk_count = 1;
    
    if (p_first < 0)
    {
        uinteger_t t_count;
        t_count = p_callback(p_context);
        p_first += t_count;
    }
    else
        p_first--;
    
    if (p_first < 0)
    {
        r_chunk_count = 0;
        p_first = 0;
    }
    
    r_first = p_first;
}

////////////////////////////////////////////////////////////////////////////////

void MCChunkGetExtentsOfByteChunkByRangeInRange(MCDataRef p_data, MCRange *p_range, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkCountInRangeState t_state;
    t_state . value = p_data;
    t_state . range = p_range;
    MCChunkGetExtentsByRangeInRange(p_first, p_last, MCChunkCountByteChunkCallback, &t_state, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfByteChunkByExpressionInRange(MCDataRef p_data, MCRange *p_range, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkCountInRangeState t_state;
    t_state . value = p_data;
    t_state . range = p_range;
    MCChunkGetExtentsByExpressionInRange(p_first, MCChunkCountByteChunkCallback, &t_state, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfCodeunitChunkByRangeInRange(MCStringRef p_string, MCRange *p_range, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkCountInRangeState t_state;
    t_state . value = p_string;
    t_state . range = p_range;
    
    MCChunkGetExtentsByRangeInRange(p_first, p_last, MCChunkCountCodeunitChunkCallback, &t_state, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfCodeunitChunkByExpressionInRange(MCStringRef p_string, MCRange *p_range, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkCountInRangeState t_state;
    t_state . value = p_string;
    t_state . range = p_range;
    
    MCChunkGetExtentsByExpressionInRange(p_first, MCChunkCountCodeunitChunkCallback, &t_state, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfElementChunkByRangeInRange(MCProperListRef p_list, MCRange *p_range, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkCountInRangeState t_state;
    t_state . value = p_list;
    t_state . range = p_range;
    
    MCChunkGetExtentsByRangeInRange(p_first, p_last, MCChunkCountElementChunkCallback, &t_state, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfElementChunkByExpressionInRange(MCProperListRef p_list, MCRange *p_range, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkCountInRangeState t_state;
    t_state . value = p_list;
    t_state . range = p_range;
    
    MCChunkGetExtentsByExpressionInRange(p_first, MCChunkCountElementChunkCallback, &t_state, r_first, r_chunk_count);
}

////////////////////////////////////////////////////////////////////////////////
