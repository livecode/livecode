/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

bool MCCharEvaluateChunk(MCStringRef p_target, MCRange p_grapheme_range, MCStringRef& r_output)
{
    MCRange t_range;
    MCStringMapGraphemeIndices(p_target, p_grapheme_range, t_range);
    
    return MCStringCopySubstring(p_target, t_range, r_output);
}

bool MCCharStoreChunk(MCStringRef &x_target, MCStringRef p_value, MCRange p_grapheme_range, MCStringRef& r_output)
{
    MCAutoStringRef t_string;
    if (!MCStringMutableCopy(x_target, &t_string))
        return false;
    
    MCRange t_range;
    MCStringMapGraphemeIndices(x_target, p_grapheme_range, t_range);
    
    if (!MCStringReplace(*t_string, MCRangeMake(t_range . offset, t_range . length), p_value))
        return false;
    
    MCAutoStringRef t_new_string;
    if (!MCStringCopy(*t_string, &t_new_string))
        return false;
    
    MCValueAssign(x_target, *t_new_string);
    return true;
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalNumberOfCharsIn(MCStringRef p_target, index_t& r_output)
{
    MCAutoPointer<MCTextChunkIterator> tci =
            MCChunkCreateTextChunkIterator(p_target, nil, kMCChunkTypeCharacter, nil, nil, kMCStringOptionCompareExact);
    r_output = tci -> CountChunks();
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalIsAmongTheCharsOf(MCStringRef p_needle, MCStringRef p_target, bool& r_output)
{
    // Error if there is more than one char in needle.
    MCRange t_range;
    // AL_2015-05-07: [[ Bug 15331 ]] Pass in correct code unit range as MCStringUnmapGraphemeIndices doesn't clamp.
    MCStringUnmapGraphemeIndices(p_needle, MCRangeMake(0, MCStringGetLength(p_needle)), t_range);
    if (t_range . length != 1)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("needle must be a single char"), nil);
        return;
    }
    
    MCAutoPointer<MCTextChunkIterator> tci =
            MCChunkCreateTextChunkIterator(p_target, nil, kMCChunkTypeCharacter, nil, nil, kMCStringOptionCompareExact);
    r_output = tci -> IsAmong(p_needle);
}

extern "C" MC_DLLEXPORT_DEF void MCCharFetchCharRangeOf(index_t p_start, index_t p_finish, MCStringRef p_target, MCStringRef& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfGraphemeChunkByRangeInRange(p_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCCharEvaluateChunk(p_target, MCRangeMake(t_start, t_count), r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCharStoreCharRangeOf(MCStringRef p_value, index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfGraphemeChunkByRangeInRange(x_target, nil, p_start, p_finish, true, false, false, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCCharStoreChunk(x_target, p_value, MCRangeMake(t_start, t_count), p_value);
}

extern "C" MC_DLLEXPORT_DEF void MCCharFetchCharOf(index_t p_index, MCStringRef p_target, MCStringRef& r_output)
{
    MCCharFetchCharRangeOf(p_index, p_index, p_target, r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCharStoreCharOf(MCStringRef p_value, index_t p_index, MCStringRef& x_target)
{
    MCCharStoreCharRangeOf(p_value, p_index, p_index, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalOffsetOfCharsInRange(bool p_is_last, MCStringRef p_needle, MCStringRef p_target, MCRange p_range, uindex_t& r_output)
{
    uindex_t t_offset;
    t_offset = 0;
	if (MCStringIsEmpty(p_needle))
	{
		r_output = 0;
		return;
	}

	MCRange t_range;
	if (p_range . length == UINDEX_MAX)
	{
		MCStringMapGraphemeIndices(p_target, MCRangeMake(p_range . offset, 1), t_range);
		t_range . length = UINDEX_MAX;
	}
	else
		MCStringMapGraphemeIndices(p_target, p_range, t_range);
        
	bool t_found;
	if (p_is_last)
		t_found = MCStringLastIndexOfStringInRange(p_target, p_needle, t_range, kMCStringOptionCompareExact, t_offset);
	else
		t_found = MCStringFirstIndexOfStringInRange(p_target, p_needle, t_range, kMCStringOptionCompareExact, t_offset);
        
	if (!t_found)
	{
		r_output = 0;
		return;
	}

	// correct output index
	t_offset -= t_range . offset;
	t_offset++;

    MCRange t_output_range;
    MCStringUnmapGraphemeIndices(p_target, MCRangeMake(t_offset, 1), t_output_range);

	r_output = t_output_range . offset + p_range . offset;
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalOffsetOfChars(bool p_is_last, MCStringRef p_needle, MCStringRef p_target, uindex_t& r_output)
{
    MCCharEvalOffsetOfCharsInRange(p_is_last, p_needle, p_target, MCRangeMake(0, UINDEX_MAX), r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalOffsetOfCharsAfter(bool p_is_last, MCStringRef p_needle, index_t p_after, MCStringRef p_target, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfGraphemeChunkByExpressionInRange(p_target, nil, p_after, true, true, false, t_start, t_count) && p_after != 0)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }
    
    MCCharEvalOffsetOfCharsInRange(p_is_last, p_needle, p_target, MCRangeMake(t_start + t_count, UINDEX_MAX), r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalOffsetOfCharsBefore(bool p_is_first, MCStringRef p_needle, index_t p_before, MCStringRef p_target, uindex_t& r_output)
{
    uindex_t t_start, t_count;
    if (!MCChunkGetExtentsOfGraphemeChunkByExpressionInRange(p_target, nil, p_before, true, false, true, t_start, t_count))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("chunk index out of range"), nil);
        return;
    }

    MCCharEvalOffsetOfCharsInRange(!p_is_first, p_needle, p_target, MCRangeMake(0, t_start), r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalContains(MCStringRef p_source, MCStringRef p_needle, bool& r_result)
{
    r_result = MCStringContains(p_source, p_needle, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalBeginsWith(MCStringRef p_source, MCStringRef p_prefix, bool& r_result)
{
    r_result = MCStringBeginsWith(p_source, p_prefix, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalEndsWith(MCStringRef p_source, MCStringRef p_suffix, bool& r_result)
{
    r_result = MCStringEndsWith(p_source, p_suffix, kMCStringOptionCompareExact);
}

extern "C" MC_DLLEXPORT_DEF void MCCharEvalNewlineCharacter(MCStringRef& r_output)
{
    MCStringFormat(r_output, "\n");
}

extern "C" MC_DLLEXPORT_DEF void MCCharFetchFirstCharOf(MCStringRef p_target, MCStringRef& r_output)
{
    MCCharFetchCharOf(1, p_target, r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCharStoreFirstCharOf(MCStringRef p_value, MCStringRef& x_target)
{
    MCCharStoreCharOf(p_value, 1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCharFetchLastCharOf(MCStringRef p_target, MCStringRef& r_output)
{
    MCCharFetchCharOf(-1, p_target, r_output);
}

extern "C" MC_DLLEXPORT_DEF void MCCharStoreLastCharOf(MCStringRef p_value, MCStringRef& x_target)
{
    MCCharStoreCharOf(p_value, -1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCharExecDeleteCharRangeOf(index_t p_start, index_t p_finish, MCStringRef& x_target)
{
    MCCharStoreCharRangeOf(kMCEmptyString, p_start, p_finish, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCharExecDeleteCharOf(index_t p_index, MCStringRef& x_target)
{
    MCCharStoreCharOf(kMCEmptyString, p_index, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCharExecDeleteFirstCharOf(MCStringRef& x_target)
{
    MCCharExecDeleteCharOf(1, x_target);
}

extern "C" MC_DLLEXPORT_DEF void MCCharExecDeleteLastCharOf(MCStringRef& x_target)
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
extern "C" MC_DLLEXPORT_DEF bool MCCharRepeatForEachChar(void*& x_iterator, MCStringRef& r_iterand, MCStringRef p_string)
{ 
    uindex_t t_offset;
    t_offset = (uindex_t)(uintptr_t)x_iterator;
    
    uindex_t t_length =
            MCStringGetLength(p_string);
    if (t_offset == t_length)
        return false;

    uindex_t t_next =
            MCStringGraphemeBreakIteratorAdvance(p_string,
                                                 t_offset);
    if (t_next == (uindex_t)-1)
        t_next = t_length;
    
    if (!MCStringCopySubstring(p_string,
                               MCRangeMake(t_offset, t_next - t_offset),
                               r_iterand))
    {
        return false;
    }
    
    x_iterator = (void *)(uintptr_t)t_next;
    
    return true;
}

extern "C" MC_DLLEXPORT_DEF void
MCStringExecReverseCharsOf(MCStringRef &x_string)
{
    MCStringRef t_reversed = nullptr;
    if (!MCStringCopyReversed(x_string, t_reversed))
        return;
    MCValueAssign(x_string, t_reversed);
}

////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void MCStringEvalCodeOfChar(MCStringRef p_string, uinteger_t& r_code)
{
    uindex_t t_length;
    t_length = MCStringGetLength(p_string);
    if (t_length == 0 || t_length > 2)
        goto notacodepoint_exit;
    
    codepoint_t t_code;
    t_code = MCStringGetCodepointAtIndex(p_string, 0);
    if (t_length > 1 &&
        t_code < 65536)
        goto notacodepoint_exit;
    
    r_code = t_code;
    
    return;
    
notacodepoint_exit:
    MCErrorThrowGeneric(MCSTR("not a single code character"));
}

extern "C" MC_DLLEXPORT_DEF void MCStringEvalCharWithCode(uinteger_t p_code, MCStringRef& r_string)
{
    if (p_code >= 1 << 21)
    {
        MCErrorThrowGeneric(MCSTR("code out of range"));
        return;
    }
    
    if (p_code >= 1 << 16)
    {
        unichar_t t_codeunits[2];
        t_codeunits[0] =  unichar_t((p_code - 0x10000) >> 10) + 0xD800;
        t_codeunits[1] = unichar_t((p_code - 0x10000) & 0x3FF) + 0xDC00;
        MCStringCreateWithChars(t_codeunits, 2, r_string);
        return;
    }
    
    unichar_t t_codeunit;
    t_codeunit = (unichar_t)p_code;
    MCStringCreateWithChars(&t_codeunit, 1, r_string);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_char_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_char_Finalize(void)
{
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
