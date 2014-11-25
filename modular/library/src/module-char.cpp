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

void MCCharEvalNumberOfCharsIn(MCStringRef p_target, index_t& r_output)
{
    r_output = MCStringGetLength(p_target);
}

#if 0
void MCCharEvalIsAmongTheCharsOf(MCHandlerContext& ctxt, MCStringRef p_needle, MCStringRef p_target, bool& r_output)
{
    // Error if there is more than one char in needle.
    if (MCStringGetLength(p_needle) == 1)
        return;
    
    uindex_t t_dummy;
    r_output = MCStringFirstIndexOfChar(p_target, MCStringGetCodepointAtIndex(p_needle, 0), 0, ctxt . GetStringComparisonOptions(), t_dummy);
}
#endif

void MCCharFetchCharRangeOf(index_t p_start, index_t p_finish, MCStringRef p_target, MCStringRef& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfCodepointChunkByRange(p_target, p_start, p_finish, t_start, t_count);
    if (!MCStringCopySubstring(p_target, MCRangeMake(t_start, t_count), r_output))
        return;
}

void MCCharStoreCharRangeOf(MCStringRef p_value, index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    uindex_t t_start, t_count;
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

void MCCharFetchCharOf(index_t p_index, MCStringRef p_target, MCStringRef& r_output)
{
    MCCharFetchCharRangeOf(p_index, p_index, p_target, r_output);
}

void MCCharStoreCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    MCCharStoreCharRangeOf(p_value, p_index, p_index, x_target);
}

void MCCharStoreAfterCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfCodepointChunkByRange(x_target, p_index, p_index, t_start, t_count);
    
    MCAutoStringRef t_data;
    if (!MCStringMutableCopy(x_target, &t_data))
        return;
    
    if (!MCStringInsert(*t_data, t_start + t_count, p_value))
        return;
    
    MCValueRelease(x_target);
    if (!MCStringCopy(*t_data, x_target))
        return;
}

void MCCharStoreBeforeCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfCodepointChunkByRange(x_target, p_index, p_index, t_start, t_count);
    
    MCAutoStringRef t_data;
    if (!MCStringMutableCopy(x_target, &t_data))
        return;
    
    if (!MCStringInsert(*t_data, t_start, p_value))
        return;
    
    MCValueRelease(x_target);
    if (!MCStringCopy(*t_data, x_target))
        return;
}

#if 0
void MCCharEvalOffsetOfCharsInRange(MCHandlerContext ctxt, MCStringRef p_needle, MCStringRef p_target, bool p_is_last, MCRange p_range, uindex_t& r_output)
{
    uindex_t t_offset;
    t_offset = 0;
    if (!MCStringIsEmpty(p_needle))
    {
        if (p_is_last)
            MCStringLastIndexOfStringInRange(p_target, p_needle, p_range, ctxt . GetStringComparisonOptions(), t_offset);
        else
            MCStringFirstIndexOfStringInRange(p_target, p_needle, p_range, ctxt . GetStringComparisonOptions(), t_offset);
        
        // correct output index
        t_offset++;
    }
    r_output = t_offset;
}

void MCCharEvalOffsetOfChars(MCHandlerContext ctxt, MCStringRef p_needle, MCStringRef p_target, bool p_is_last, uindex_t& r_output)
{
    MCCharEvalOffsetOfCharsInRange(ctxt, p_needle, MCRangeMake(0, UINDEX_MAX), p_target, p_is_last, r_output);
}

void MCCharEvalOffsetOfCharsAfter(MCHandlerContext ctxt, MCStringRef p_needle, uindex_t p_after, MCStringRef p_target, bool p_is_last, uindex_t& r_output)
{
    MCCharEvalOffsetOfCharsInRange(ctxt, p_needle, MCRangeMake(p_after, UINDEX_MAX), p_target, p_is_last, r_output);
}

void MCCharEvalOffsetOfCharsBefore(MCDataRef p_needle, MCDataRef p_target, uindex_t p_before, bool p_is_first, uindex_t& r_output)
{
    MCCharEvalOffsetOfCharsInRange(ctxt, p_needle, MCRangeMake(0, p_before), p_target, p_is_last, r_output);
}

void MCCharEvalBeginsWith(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef p_prefix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, ctxt . GetStringComparisonOptions());
}

void MCCharEvalEndsWith(MCHandlerContext& ctxt, MCStringRef p_source, MCStringRef p_suffix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, ctxt . GetStringComparisonOptions());
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("CHAR MODULE", test, result)
void MCCharRunTests()
{
    // Need handler context object to test
}