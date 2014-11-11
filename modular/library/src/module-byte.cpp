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
#include <foundation-auto.h>
#include <foundation-chunk.h>

void MCByteEvalNumberOfBytesIn(MCDataRef p_source, uindex_t& r_output)
{
    r_output = MCDataGetLength(p_source);
}

void MCByteEvalIsAmongTheBytesOf(MCDataRef p_needle, MCDataRef p_target, bool p_is_not, bool& r_output)
{
    // Error if there is more than one byte.
    if (MCDataGetLength(p_target) != 1)
        return;
    
    bool t_found = MCDataContains(p_target, p_needle);
    
    if (p_is_not)
        t_found = !t_found;
    
    r_output = t_found;
}

void MCByteEvalContainsBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
{
    r_output = MCDataContains(p_target, p_needle);
}

void MCByteEvalBeginsWithBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
{
    r_output = MCDataBeginsWith(p_target, p_needle);
}

void MCByteEvalEndsWithBytes(MCDataRef p_target, MCDataRef p_needle, bool& r_output)
{
    r_output = MCDataEndsWith(p_target, p_needle);
}

void MCByteEvalOffsetOfBytesInRange(MCDataRef p_needle, MCDataRef p_target, bool p_is_last, MCRange p_range, uindex_t& r_output)
{
    uindex_t t_offset;
    t_offset = 0;
    
    if (!MCDataIsEmpty(p_needle))
    {
        if (p_is_last)
            t_offset = MCDataFirstIndexOf(p_target, p_needle, p_range);
        else
            t_offset = MCDataLastIndexOf(p_target, p_needle, p_range);
        
        t_offset++;
    }
    
    r_output = t_offset;
}

void MCByteEvalOffsetOfBytes(MCDataRef p_needle, MCDataRef p_target, bool p_is_last, uindex_t& r_output)
{
    return MCByteEvalOffsetOfBytesInRange(p_needle, p_target, MCRangeMake(0, UINDEX_MAX), p_is_last, r_output);
}

void MCByteEvalOffsetOfBytesAfter(MCDataRef p_needle, MCDataRef p_target, uindex_t p_after, bool p_is_last, uindex_t& r_output)
{
    return MCByteEvalOffsetOfBytesInRange(p_needle, p_target, MCRangeMake(p_after, UINDEX_MAX), p_is_last, r_output);
}

void MCByteEvalOffsetOfBytesBefore(MCDataRef p_needle, MCDataRef p_target, uindex_t p_before, bool p_is_first, uindex_t& r_output)
{
    return MCByteEvalOffsetOfBytesInRange(p_needle, p_target, MCRangeMake(0, p_before), p_is_last, r_output);
}

void MCByteFetchByteRangeOf(index_t p_start, index_t p_finish, MCDataRef p_target, MCDataRef& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByRange(p_target, p_start, p_finish, t_start, t_count);
    if (!MCDataCopyRange(p_target, MCRangeMake(t_start, t_count), r_output))
        return;
}

void MCByteStoreByteRangeOf(MCDataRef p_value, index_t p_start, index_t p_finish, MCDataRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByRange(x_target, p_start, p_finish, t_start, t_count);
    
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(x_target, &t_data))
        return;
    
    if (!MCDataReplace(*t_data, MCRangeMake(t_start, t_count), p_value))
        return;
    
    MCValueRelease(x_target);
    if (!MCDataCopy(*t_data, x_target))
        return;
}

void MCByteFetchByteOf(index_t p_index, MCDataRef p_target, MCDataRef& r_output)
{
    MCBinaryFetchByteRangeOf(p_index, p_index, p_target, r_output);
}

void MCByteStoreByteOf(MCDataRef p_value, index_t p_index, MCDataRef& x_target)
{
    MCBinaryStoreByteRangeOf(p_value, p_index, p_index, x_target);
}

void MCByteStoreAfterCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    integer_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByRange(x_target, p_index, p_index, t_start, t_count);
    
    t_start += t_count;
    
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(x_target, &t_data))
        return;
    
    if (!MCDataInsert(*t_data, t_start, p_value))
        return;
    
    MCValueRelease(x_target);
    if (!MCDataCopy(*t_data, x_target))
        return;
}

void MCByteStoreBeforeCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    integer_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByRange(x_target, p_index, p_index, t_start, t_count);
    
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(x_target, &t_data))
        return;
    
    if (!MCDataInsert(*t_data, t_start, p_value))
        return;
    
    MCValueRelease(x_target);
    if (!MCDataCopy(*t_data, x_target))
        return;
}
