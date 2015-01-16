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

uinteger_t MCChunkCountCodeunitChunkCallback(void *context)
{
    return MCStringGetLength(*(MCStringRef *)context);
}

uinteger_t MCChunkCountElementChunkCallback(void *context)
{
    return MCProperListGetLength(*(MCProperListRef *)context);
}

struct MCChunkCountContext
{
    MCStringRef string;
    MCStringRef delimiter;
    MCStringOptions options;
};

static bool count_chunks(void *context, MCStringRef p_string, MCRange p_range)
{
    uindex_t count;
    count = *(uindex_t *)context;
    count++;
    
    return true;
}

uinteger_t MCChunkCountChunkChunkCallback(void *context)
{
    MCChunkCountContext *t_ctxt;
    t_ctxt = (MCChunkCountContext *)context;
    
    return MCChunkCountChunkChunks(t_ctxt -> string, t_ctxt -> delimiter, t_ctxt -> options);
}

////////////////////////////////////////////////////////////////////////////////

uindex_t MCChunkCountChunkChunks(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options)
{
    uindex_t t_count;
    t_count = 0;
    
    MCChunkApply(p_string, p_delimiter, p_options, count_chunks, &t_count);
    
    return t_count;
}

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

void MCChunkGetExtentsOfCodeunitChunkByRange(MCStringRef p_string, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByRange(p_first, p_last, MCChunkCountCodeunitChunkCallback, &p_string, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfCodeunitChunkByExpression(MCStringRef p_string, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByExpression(p_first, MCChunkCountCodeunitChunkCallback, &p_string, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfElementChunkByRange(MCProperListRef p_string, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByRange(p_first, p_last, MCChunkCountElementChunkCallback, &p_string, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfElementChunkByExpression(MCProperListRef p_string, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByExpression(p_first, MCChunkCountElementChunkCallback, &p_string, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfChunkChunkByRange(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkCountContext t_ctxt;
    t_ctxt . string = p_string;
    t_ctxt . delimiter = p_delimiter;
    t_ctxt . options = p_options;
    
    MCChunkGetExtentsByRange(p_first, p_last, MCChunkCountChunkChunkCallback, &t_ctxt, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfChunkChunkByExpression(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkCountContext t_ctxt;
    t_ctxt . string = p_string;
    t_ctxt . delimiter = p_delimiter;
    t_ctxt . options = p_options;
    
    MCChunkGetExtentsByExpression(p_first, MCChunkCountChunkChunkCallback, &t_ctxt, r_first, r_chunk_count);
}

////////////////////////////////////////////////////////////////////////////////

bool MCChunkIsAmongTheChunksOfRange(MCStringRef p_chunk, MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, MCRange p_range)
{
    MCRange t_range;
    if (!MCStringFind(p_string, p_range, p_chunk, p_options, &t_range))
        return false;
    
    uindex_t t_length;
    // if there is no delimiter to the left then continue searching the string.
    if (t_range . offset != 0 &&
        !MCStringSharedSuffix(p_string, MCRangeMake(0, t_range . offset), p_delimiter, p_options, t_length))
        return MCChunkIsAmongTheChunksOfRange(p_chunk, p_string, p_delimiter, p_options, MCRangeMake(t_range . offset + t_range . length, p_range . length));
    
    // if there is no delimiter to the right then continue searching the string.
    if (t_range . offset + t_range . length != MCStringGetLength(p_string) &&
        !MCStringSharedPrefix(p_string, MCRangeMake(t_range . offset + t_range . length, UINDEX_MAX), p_delimiter, p_options, t_length))
        return MCChunkIsAmongTheChunksOfRange(p_chunk, p_string, p_delimiter, p_options, MCRangeMake(t_range . offset + t_range . length, p_range . length));
    
    return true;
}

bool MCChunkOffsetOfChunkInRange(MCStringRef p_string, MCStringRef p_needle, MCStringRef p_delimiter, bool p_whole_matches, MCStringOptions p_options, MCRange p_range, uindex_t& r_offset)
{
    // If we can't find the chunk in the remainder of the string, we are done.
    MCRange t_range;
    if (!MCStringFind(p_string, p_range, p_needle, p_options, &t_range))
        return false;
    
    uindex_t t_length;
    // If we are in wholeMatches mode, ensure the delimiter is either side.
    if (p_whole_matches)
    {
        if (t_range . offset > 0 &&
            !MCStringSharedSuffix(p_string, MCRangeMake(0, t_range . offset), p_delimiter, p_options, t_length))
            return MCChunkOffsetOfChunkInRange(p_string, p_needle, p_delimiter, p_whole_matches, p_options, MCRangeMake(t_range . offset + t_range . length, p_range . length), r_offset);
        if (t_range . offset + t_range . length < MCStringGetLength(p_string) &&
            !MCStringSharedPrefix(p_string, MCRangeMake(t_range . offset + t_range . length, UINDEX_MAX), p_delimiter, p_options, t_length))
            return MCChunkOffsetOfChunkInRange(p_string, p_needle, p_delimiter, p_whole_matches, p_options, MCRangeMake(t_range . offset + t_range . length + 1, p_range . length), r_offset);
    }
    
    r_offset = t_range . offset;
    return true;
}

bool MCChunkApply(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, MCChunkApplyCallback p_callback, void *context)
{
    MCRange t_range;
    t_range = MCRangeMake(0, 0);
    
    bool t_first;
    t_first = true;
    
    while (MCChunkIterate(t_range, p_string, p_delimiter, p_options, t_first))
    {
        t_first = false;
        if (!p_callback(context, p_string, t_range))
            return false;
    }
    
    return true;
}

bool MCChunkIterate(MCRange& x_range, MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, bool p_first)
{
    // Currently assumes delimiter is 1 char long.
    uindex_t t_delimiter_offset;
    
    if (!p_first)
        x_range . offset = x_range . offset + x_range . length + 1;

    if (x_range . offset >= MCStringGetLength(p_string))
        return false;
    
        
    if (!MCStringFirstIndexOfChar(p_string, MCStringGetCodepointAtIndex(p_delimiter, 0), x_range . offset, p_options, t_delimiter_offset))
        t_delimiter_offset = MCStringGetLength(p_string);

    x_range . length = t_delimiter_offset - x_range . offset;
    
    return true;
}
