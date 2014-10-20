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

void MCCharChunkEvalNumberOfCharsIn(MCStringRef p_target, index_t& r_output)
{
    r_output = MCStringGetLength(p_target);
}

void MCCharChunkEvalIsAmongTheCharsOf(MCHandlerContext& ctxt, MCStringRef p_needle, MCStringRef p_target, bool& r_output)
{
    bool t_output;
    t_output = false;
    
    if (MCStringGetLength(p_needle) == 1)
    {
        uindex_t t_dummy;
        t_output = MCStringFirstIndexOfChar(p_target, MCStringGetCodepointAtIndex(p_needle, 0), 0, ctxt . GetStringComparisonOptions(), t_dummy);
    }
    
    r_output = t_output;
}

void MCCharChunkFetchCharRangeOf(index_t p_start, index_t p_finish, MCStringRef p_target, MCStringRef& r_output)
{
    integer_t t_start, t_count;
    MCChunkGetExtentsOfCodepointChunkByRange(p_target, p_start, p_finish, t_start, t_count);
    if (!MCStringCopySubstring(p_target, MCRangeMake(t_start, t_count), r_output))
        return;
}

void MCCharChunkStoreCharRangeOf(MCStringRef p_value, index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    integer_t t_start, t_count;
    MCChunkGetExtentsOfCodepointChunkByRange(x_target, p_start, p_finish, t_start, t_count);
    
    MCAutoStringRef t_data;
    if (!MCStringMutableCopy(x_target, &t_data))
        return;
    
    if (!MCStringReplace(*t_data, MCRangeMake(t_start, t_count), p_value))
        return;
    
    MCValueRelease(x_target);
    if (!MCStringCopy(*t_data, x_target))
        return;
}

void MCCharChunkFetchCharOf(index_t p_index, MCStringRef p_target, MCStringRef& r_output)
{
    MCCharChunkFetchCharRangeOf(p_index, p_index, p_target, r_output);
}

void MCCharChunkStoreCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    MCCharChunkStoreCharRangeOf(p_value, p_index, p_index, x_target);
}