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
#include <foundation-locale.h>

bool MCCharEvaluateChunk(MCStringRef p_target, MCRange p_grapheme_range, MCStringRef& r_output)
{
    MCRange t_range;
    MCStringMapGraphemeIndices(p_target, kMCLocaleBasic, p_grapheme_range, t_range);
    
    return MCStringCopySubstring(p_target, t_range, r_output);
}

bool MCCharStoreChunk(MCStringRef &x_target, MCStringRef p_value, MCRange p_grapheme_range, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(x_target, &t_string))
        return false;
    
    MCRange t_range;
    MCStringMapGraphemeIndices(x_target, kMCLocaleBasic, p_grapheme_range, t_range);
    
    if (!MCStringReplace(*t_string, MCRangeMake(t_range . offset, t_range . length), p_value))
        return false;
    
    MCAutoStringRef t_new_string;
    if (!MCStringCopy(*t_string, &t_new_string))
        return false;
    
    MCValueAssign(x_target, *t_new_string);
    return true;
}

extern "C" MC_DLLEXPORT void MCCharEvalNumberOfCharsIn(MCStringRef p_target, index_t& r_output)
{
    MCTextChunkIterator *tci;
    tci = MCChunkCreateTextChunkIterator(p_target, nil, kMCChunkTypeCharacter, nil, kMCStringOptionCompareExact);
    r_output = tci -> CountChunks();
}

extern "C" MC_DLLEXPORT void MCCharEvalIsAmongTheCharsOf(MCStringRef p_needle, MCStringRef p_target, bool& r_output)
{
    // Error if there is more than one char in needle.
    MCRange t_range;
    MCStringUnmapGraphemeIndices(p_needle, kMCLocaleBasic, MCRangeMake(0, UINDEX_MAX), t_range);
    if (t_range . length != 1)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("needle must be a single char"), nil);
        return;
    }
    
    MCTextChunkIterator *tci;
    tci = MCChunkCreateTextChunkIterator(p_target, nil, kMCChunkTypeCharacter, nil, kMCStringOptionCompareExact);
    r_output = tci -> IsAmong(p_needle);
}

extern "C" MC_DLLEXPORT void MCCharFetchCharRangeOf(index_t p_start, index_t p_finish, MCStringRef p_target, MCStringRef& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfGraphemeChunkByRangeInRange(p_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCCharEvaluateChunk(p_target, MCRangeMake(t_start, t_count), r_output);
}

extern "C" MC_DLLEXPORT void MCCharStoreCharRangeOf(MCStringRef p_value, index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfGraphemeChunkByRangeInRange(x_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCCharStoreChunk(x_target, p_value, MCRangeMake(t_start, t_count), p_value);
}

extern "C" MC_DLLEXPORT void MCCharFetchCharOf(index_t p_index, MCStringRef p_target, MCStringRef& r_output)
{
    MCCharFetchCharRangeOf(p_index, p_index, p_target, r_output);
}

extern "C" MC_DLLEXPORT void MCCharStoreCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    MCCharStoreCharRangeOf(p_value, p_index, p_index, x_target);
}

extern "C" MC_DLLEXPORT void MCCharEvalOffsetOfCharsInRange(bool p_is_last, MCStringRef p_needle, MCStringRef p_target, MCRange p_range, uindex_t& r_output)
{
    uindex_t t_offset;
    t_offset = 0;
    if (!MCStringIsEmpty(p_needle))
    {
        MCRange t_range;
        if (p_range . length == UINDEX_MAX)
        {
            MCStringMapGraphemeIndices(p_target, kMCLocaleBasic, MCRangeMake(p_range . offset, 1), t_range);
            t_range . length = UINDEX_MAX;
        }
        else
            MCStringMapGraphemeIndices(p_target, kMCLocaleBasic, p_range, t_range);
        
        bool t_found;
        if (p_is_last)
            t_found = MCStringLastIndexOfStringInRange(p_target, p_needle, t_range, kMCStringOptionCompareExact, t_offset);
        else
            t_found = MCStringFirstIndexOfStringInRange(p_target, p_needle, t_range, kMCStringOptionCompareExact, t_offset);
        
        // correct output index
        if (t_found)
        {
            t_offset -= p_range . offset;
            t_offset++;
        }
    }
    
    MCRange t_output_range;
    MCStringUnmapGraphemeIndices(p_target, kMCLocaleBasic, MCRangeMake(t_offset, 1), t_output_range);
    r_output = t_output_range . offset;
}

extern "C" MC_DLLEXPORT void MCCharEvalOffsetOfChars(bool p_is_last, MCStringRef p_needle, MCStringRef p_target, uindex_t& r_output)
{
    MCCharEvalOffsetOfCharsInRange(p_is_last, p_needle, p_target, MCRangeMake(0, UINDEX_MAX), r_output);
}

extern "C" MC_DLLEXPORT void MCCharEvalOffsetOfCharsAfter(bool p_is_last, MCStringRef p_needle, index_t p_after, MCStringRef p_target, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfGraphemeChunkByExpressionInRange(p_target, nil, p_after, true, true, false, t_start, t_count) && p_after != 0)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCCharEvalOffsetOfCharsInRange(p_is_last, p_needle, p_target, MCRangeMake(t_start + t_count, UINDEX_MAX), r_output);
}

extern "C" MC_DLLEXPORT void MCCharEvalOffsetOfCharsBefore(bool p_is_first, MCStringRef p_needle, index_t p_before, MCStringRef p_target, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfGraphemeChunkByExpressionInRange(p_target, nil, p_before, true, false, true, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }

    MCCharEvalOffsetOfCharsInRange(!p_is_first, p_needle, p_target, MCRangeMake(0, t_start), r_output);
}

extern "C" MC_DLLEXPORT void MCCharEvalContains(MCStringRef p_source, MCStringRef p_needle, bool& r_result)
{
    r_result = MCStringContains(p_source, p_needle, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT void MCCharEvalBeginsWith(MCStringRef p_source, MCStringRef p_prefix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT void MCCharEvalEndsWith(MCStringRef p_source, MCStringRef p_suffix, bool& r_result)
{
    r_result = MCStringEndsWith(p_source, p_suffix, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT void MCCharEvalNewlineCharacter(MCStringRef& r_output)
{
    MCStringFormat(r_output, "\n");
}

extern "C" MC_DLLEXPORT void MCCharFetchFirstCharOf(MCStringRef p_target, MCStringRef& r_output)
{
    MCCharFetchCharOf(1, p_target, r_output);
}

extern "C" MC_DLLEXPORT void MCCharStoreFirstCharOf(MCStringRef p_value, MCStringRef& x_target)
{
    MCCharStoreCharOf(p_value, 1, x_target);
}

extern "C" MC_DLLEXPORT void MCCharFetchLastCharOf(MCStringRef p_target, MCStringRef& r_output)
{
    MCCharFetchCharOf(-1, p_target, r_output);
}

extern "C" MC_DLLEXPORT void MCCharStoreLastCharOf(MCStringRef p_value, MCStringRef& x_target)
{
    MCCharStoreCharOf(p_value, -1, x_target);
}

extern "C" MC_DLLEXPORT void MCCharExecDeleteCharRangeOf(index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    MCCharStoreCharRangeOf(kMCEmptyString, p_start, p_finish, x_target);
}

extern "C" MC_DLLEXPORT void MCCharExecDeleteCharOf(index_t p_index, MCStringRef& x_target)
{
    MCCharStoreCharOf(kMCEmptyString, p_index, x_target);
}

extern "C" MC_DLLEXPORT void MCCharExecDeleteFirstCharOf(MCStringRef& x_target)
{
    MCCharExecDeleteCharOf(1, x_target);
}

extern "C" MC_DLLEXPORT void MCCharExecDeleteLastCharOf(MCStringRef& x_target)
{
    MCCharExecDeleteCharOf(-1, x_target);
}

// Iterate syntax methods have special calling convention at the moment:
//
// Post assignment of out / inout variables only occurs if the method returns true.
// If the method returns false, then it means iteration has finished *not* that an
// error has been thrown.
//
// This means that the iterand out binding will not be updated on the final test
// of the loop which means that:
//   repeat for each char tChar in tVar
//   end repeat
// Will result in tChar containing the value it had at the point of end repeat.
extern "C" MC_DLLEXPORT bool MCCharRepeatForEachChar(void*& x_iterator, MCStringRef& r_iterand, MCStringRef p_string)
{
    MCTextChunkIterator *t_iterator;
    bool t_first;
    t_first = false;
    
    if ((uintptr_t)x_iterator == 0)
    {
        t_first = true;
        t_iterator = MCChunkCreateTextChunkIterator(p_string, nil, kMCChunkTypeCharacter, nil, kMCStringOptionCompareExact);
    }
    else
        t_iterator = (MCTextChunkIterator *)x_iterator;
    
    if (t_iterator -> Next())
        t_iterator -> CopyString(r_iterand);
    else
    {
        delete t_iterator;
        return false;
    }
    
    x_iterator = (void *)(t_iterator);
    
    return true;
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
