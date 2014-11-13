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
    return MCDataGetLength(*(MCDataRef *)context);
}

uinteger_t MCChunkCountCodepointChunkCallback(void *context)
{
    return MCStringGetLength(*(MCStringRef *)context);
}

uinteger_t MCChunkCountElementChunkCallback(void *context)
{
    return MCProperListGetLength(*(MCProperListRef *)context);
}

////////////////////////////////////////////////////////////////////////////////

void MCChunkGetExtentsByRange(integer_t p_first, integer_t p_last, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count)
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

void MCChunkGetExtentsByExpression(integer_t p_first, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count)
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

void MCChunkGetExtentsOfByteChunkByRange(MCDataRef p_data, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByRange(p_first, p_last, MCChunkCountByteChunkCallback, &p_data, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfByteChunkByExpression(MCDataRef p_data, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByExpression(p_first, MCChunkCountByteChunkCallback, &p_data, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfCodepointChunkByRange(MCStringRef p_string, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByRange(p_first, p_last, MCChunkCountCodepointChunkCallback, &p_string, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfCodepointChunkByExpression(MCStringRef p_string, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByExpression(p_first, MCChunkCountCodepointChunkCallback, &p_string, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfElementChunkByRange(MCProperListRef p_string, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByRange(p_first, p_last, MCChunkCountElementChunkCallback, &p_string, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfElementChunkByExpression(MCProperListRef p_string, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByExpression(p_first, MCChunkCountElementChunkCallback, &p_string, r_first, r_chunk_count);
}

////////////////////////////////////////////////////////////////////////////////
