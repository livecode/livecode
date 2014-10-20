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

void MCBinaryEvalConcatenateBytes(MCDataRef p_left, MCDataRef p_right, MCDataRef& r_output)
{
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(p_left, &t_data))
        return;
    
    if (!MCDataAppend(*t_data, p_right))
        return;
    
    if (!MCDataCopy(*t_data, r_output))
        return;
}

void MCBinaryExecPutBytesBefore(MCDataRef p_source, MCDataRef& x_target)
{
    MCAutoDataRef t_data;
    MCBinaryEvalConcatenateBytes(p_source, x_target, &t_data);
    
    MCValueAssign(x_target, *t_data);
}

void MCBinaryExecPutBytesAfter(MCDataRef p_source, MCDataRef& x_target)
{
    MCAutoDataRef t_data;
    MCBinaryEvalConcatenateBytes(x_target, p_source, &t_data);
    
    MCValueAssign(x_target, *t_data);
}

void MCBinaryEvalNumberOfBytesIn(MCDataRef p_source, uindex_t& r_output)
{
    r_output = MCDataGetLength(p_source);
}

void MCBinaryEvalIsAmongTheBytesOf(MCDataRef p_needle, MCDataRef p_target, bool p_is_not, bool& r_output)
{
    bool t_found = MCDataContains(p_target, p_needle);
    
    if (p_is_not)
        t_found = !t_found;
    
    r_output = t_found;
}

void MCBinaryEvalOffsetOfBytesIn(MCDataRef p_needle, MCDataRef p_target, uindex_t& r_output)
{
    r_output = MCDataFirstIndexOf(p_target, p_needle, 0);
}

void MCBinaryEvalOffsetOfBytesAfterIndexIn(MCDataRef p_needle, MCDataRef p_target, uindex_t p_after, uindex_t& r_output)
{
    r_output = MCDataFirstIndexOf(p_target, p_needle, p_after);
}

void MCBinaryFetchByteRangeOf(index_t p_start, index_t p_finish, MCDataRef p_target, MCDataRef& r_output)
{
    integer_t t_start, t_count;
    MCChunkGetExtentsOfByteChunkByRange(p_target, p_start, p_finish, t_start, t_count);
    if (!MCDataCopyRange(p_target, MCRangeMake(t_start, t_count), r_output))
        return;
}

void MCBinaryStoreByteRangeOf(MCDataRef p_value, index_t p_start, index_t p_finish, MCDataRef& x_target)
{
    integer_t t_start, t_count;
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

void MCBinaryFetchByteOf(index_t p_index, MCDataRef p_target, MCDataRef& r_output)
{
    MCBinaryFetchByteRangeOf(p_index, p_index, p_target, r_output);
}

void MCBinaryStoreByteOf(MCDataRef p_value, index_t p_index, MCDataRef& x_target)
{
    MCBinaryStoreByteRangeOf(p_value, p_index, p_index, x_target);
}
