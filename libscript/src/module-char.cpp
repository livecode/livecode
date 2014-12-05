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

extern "C" void MCCharEvalNumberOfCharsIn(MCStringRef p_target, index_t& r_output)
{
    r_output = MCStringGetLength(p_target);
}

void MCCharEvalIsAmongTheCharsOf(MCStringRef p_needle, MCStringRef p_target, bool& r_output)
{
    // Error if there is more than one char in needle.
    if (MCStringGetLength(p_needle) == 1)
        return;
    
    uindex_t t_dummy;
    r_output = MCStringFirstIndexOfChar(p_target, MCStringGetCodepointAtIndex(p_needle, 0), 0, kMCStringOptionCompareExact, t_dummy);
}

extern "C" void MCCharFetchCharRangeOf(index_t p_start, index_t p_finish, MCStringRef p_target, MCStringRef& r_output)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfCodepointChunkByRange(p_target, p_start, p_finish, t_start, t_count);
    if (!MCStringCopySubstring(p_target, MCRangeMake(t_start, t_count), r_output))
        return;
}

extern "C" void MCCharStoreCharRangeOf(MCStringRef p_value, index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfCodepointChunkByRange(x_target, p_start, p_finish, t_start, t_count);
    
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(x_target, &t_string))
        return;
    
    if (!MCStringReplace(*t_string, MCRangeMake(t_start, t_count), p_value))
        return;
    
    MCAutoStringRef t_new_string;
    if (!MCStringCopy(*t_string, &t_new_string))
        return;
    
    MCValueAssign(x_target, *t_new_string);
}

extern "C" void MCCharFetchCharOf(index_t p_index, MCStringRef p_target, MCStringRef& r_output)
{
    MCCharFetchCharRangeOf(p_index, p_index, p_target, r_output);
}

extern "C" void MCCharStoreCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    MCCharStoreCharRangeOf(p_value, p_index, p_index, x_target);
}

extern "C" void MCCharStoreAfterCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfCodepointChunkByRange(x_target, p_index, p_index, t_start, t_count);
    t_start += t_count;
    
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(x_target, &t_string))
        return;
    
    if (!MCStringInsert(*t_string, t_start, p_value))
        return;
    
    MCAutoStringRef t_new_string;
    if (!MCStringCopy(*t_string, &t_new_string))
        return;
    
    MCValueAssign(x_target, *t_new_string);
}

extern "C" void MCCharStoreBeforeCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    uindex_t t_start, t_count;
    MCChunkGetExtentsOfCodepointChunkByRange(x_target, p_index, p_index, t_start, t_count);
    
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(x_target, &t_string))
        return;
    
    if (!MCStringInsert(*t_string, t_start, p_value))
        return;
    
    MCAutoStringRef t_new_string;
    if (!MCStringCopy(*t_string, &t_new_string))
        return;
    
    MCValueAssign(x_target, *t_new_string);
}

extern "C" void MCCharEvalOffsetOfCharsInRange(MCStringRef p_needle, MCStringRef p_target, bool p_is_last, MCRange p_range, uindex_t& r_output)
{
    uindex_t t_offset;
    t_offset = 0;
    if (!MCStringIsEmpty(p_needle))
    {
        if (p_is_last)
            MCStringLastIndexOfStringInRange(p_target, p_needle, p_range, kMCStringOptionCompareExact, t_offset);
        else
            MCStringFirstIndexOfStringInRange(p_target, p_needle, p_range, kMCStringOptionCompareExact, t_offset);
        
        // correct output index
        t_offset++;
    }
    r_output = t_offset;
}

extern "C" void MCCharEvalOffsetOfChars(MCStringRef p_needle, MCStringRef p_target, bool p_is_last, uindex_t& r_output)
{
    MCCharEvalOffsetOfCharsInRange(p_needle, p_target, p_is_last, MCRangeMake(0, UINDEX_MAX), r_output);
}

extern "C" void MCCharEvalOffsetOfCharsAfter(MCStringRef p_needle, uindex_t p_after, MCStringRef p_target, bool p_is_last, uindex_t& r_output)
{
    MCCharEvalOffsetOfCharsInRange(p_needle, p_target, p_is_last, MCRangeMake(p_after, UINDEX_MAX), r_output);
}

extern "C" void MCCharEvalOffsetOfCharsBefore(MCStringRef p_needle, MCStringRef p_target, uindex_t p_before, bool p_is_first, uindex_t& r_output)
{
    MCCharEvalOffsetOfCharsInRange(p_needle, p_target, !p_is_first, MCRangeMake(0, p_before), r_output);
}

extern "C" void MCCharEvalBeginsWith(MCStringRef p_source, MCStringRef p_prefix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, kMCStringOptionCompareExact);
}

extern "C" void MCCharEvalEndsWith(MCStringRef p_source, MCStringRef p_suffix, bool& r_result)
{
    r_result = MCStringEndsWith(p_source, p_suffix, kMCStringOptionCompareExact);
}

extern "C" void MCCharEvalNewlineCharacter(MCStringRef& r_output)
{
    MCStringFormat(r_output, "\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
extern void log(const char *module, const char *test, bool result);
#define log_result(test, result) log("CHAR MODULE", test, result)
void MCCharRunTests()
{
    // Need handler context object to test
}
#endif
