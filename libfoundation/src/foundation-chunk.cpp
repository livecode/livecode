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
#include <foundation-locale.h>
#include <foundation-unicode.h>

#include "foundation-chunk.h"

////////////////////////////////////////////////////////////////////////////////

uinteger_t MCChunkCountByteChunkCallback(void *context, MCRange *p_range)
{
    uinteger_t t_length;
    t_length = MCDataGetLength(*(MCDataRef *)context);
    
    if (p_range == nil)
        return t_length;

    MCRange t_range;
    t_range = *p_range;
    
    return t_range . offset + t_range . length > t_length ? t_length - t_range . offset : t_range . length;
}

uinteger_t MCChunkCountCodeunitChunkCallback(void *context, MCRange *p_range)
{
    uinteger_t t_length;
    t_length = MCStringGetLength(*(MCStringRef *)context);
    
    if (p_range == nil)
        return t_length;
    
    MCRange t_range;
    t_range = *p_range;
    
    return t_range . offset + t_range . length > t_length ? t_length - t_range . offset : t_range . length;
}

uinteger_t MCChunkCountGraphemeChunkCallback(void *context, MCRange *p_range)
{
    MCRange t_range;
    if (p_range == nil)
        t_range = MCRangeMake(0, MCStringGetLength(*(MCStringRef *)context));
    else
        t_range = *p_range;
    
    MCRange t_grapheme_range;
    MCStringUnmapGraphemeIndices(*(MCStringRef *)context, t_range, t_grapheme_range);
    return t_grapheme_range . length;
}

uinteger_t MCChunkCountElementChunkCallback(void *context, MCRange *p_range)
{
    uinteger_t t_length;
    t_length = MCProperListGetLength(*(MCProperListRef *)context);
    
    if (p_range == nil)
        return t_length;
    
    MCRange t_range;
    t_range = *p_range;
    
    return t_range . offset + t_range . length > t_length ? t_length - t_range . offset : t_range . length;
}

////////////////////////////////////////////////////////////////////////////////

// AL-2015-02-10: [[ Bug 14532 ]] Allow chunk extents to be counted in a
// given range, to prevent substring copying in text chunk resolution.

// Note the returned r_first and r_chunk count *are* allowed to overrun
// the given range - MCStringsMarkTextChunkInRange does the work of
// ensuring the absolute indices are restricted accordingly.
bool MCChunkGetExtentsByRangeInRange(bool p_strict, bool p_boundary_start, bool p_boundary_end, integer_t p_first, integer_t p_last, MCChunkCountCallback p_callback, void *p_context, MCRange *p_range, uindex_t& r_first, uindex_t& r_chunk_count)
{
    int32_t t_chunk_count;

    /* Helper closure for making sure that the chunk counting callback
     * is called exactly once. */
    int32_t t_count = 0;
    bool t_counted = false;
    auto t_get_count = [&] {
        if (!t_counted)
        {
            t_count = MCMin<uinteger_t>(INT32_MAX, p_callback(p_context, p_range));
            t_counted = true;
        }
        return t_count;
    };

    // If the first index is negative, count chunks and adjust accordingly.
    // Resolved index should be the index *before* the target first chunk.
    if (p_first < 0)
    {
        p_first += t_get_count();
    }
    else
        p_first--;
    
    // If the last index is negative, count chunks and adjust accordingly.
    // Resolved index should be the index of the target last chunk.
    if (p_last < 0)
    {
        p_last += t_get_count() + 1;
    }
    
    t_chunk_count = p_last - p_first;
    
    if (p_first < 0)
        t_chunk_count += p_first;
    
    if (p_strict)
    {
        // If we counted back too far, the start index is out of range unless we are looking for a start
        // boundary, in which case p_first can be -1 (before the 0-indexed first chunk)
        if (p_first < -1 || (!p_boundary_start && p_first == -1))
            return false;
        
        // If there are no chunks in this range, the range was invalid
        if (t_chunk_count == 0)
            return false;
        
        // If the range extends beyond the number of chunks, the end index is out of range unless we are
        // looking for an end boundary, in which case it can exceed the end index by 1.
        if (p_first + t_chunk_count > t_get_count() + 1 ||
            (!p_boundary_end && p_first + t_chunk_count == t_get_count() + 1))
            return false;
    }
    
    if (p_first < 0)
        p_first = 0;
    
    if (t_chunk_count < 0)
        t_chunk_count = 0;
    
    r_chunk_count = (uindex_t)t_chunk_count;
    r_first = (uindex_t)p_first;
    return true;
}

// AL-2015-03-03: Add booleans to allow chunk ranges to be out of range by 1 in strict mode.
//  This is so that executing things like 'the offset of x after 0 in x' doesn't throw an error.
bool MCChunkGetExtentsByExpressionInRange(bool p_strict, bool p_boundary_start, bool p_boundary_end, integer_t p_first, MCChunkCountCallback p_callback, void *p_context, MCRange *p_range, uindex_t& r_first, uindex_t& r_chunk_count)
{
    int32_t t_chunk_count;
    t_chunk_count = 1;
    
    uinteger_t t_count;
    bool t_counted;
    t_counted = false;
    
    if (p_first < 0 || p_range != nil)
    {
        t_count = p_callback(p_context, p_range);
        t_counted = true;
        p_first += t_count;
    }
    else
        p_first--;
    
    if (p_strict)
    {
        // If we counted back too far, the start index is out of range unless we are looking for a start
        // boundary, in which case p_first can be -1 (before the 0-indexed first chunk)
        if (p_first < -1 || (!p_boundary_start && p_first == -1))
            return false;
        
        if (!t_counted)
            t_count = p_callback(p_context, p_range);
        
        // If the range extends beyond the number of chunks, the end index is out of range unless we are
        // looking for an end boundary, in which case it can exceed the end index by 1.
        if (p_first + t_chunk_count > t_count + 1 || (!p_boundary_end && p_first + t_chunk_count == t_count + 1))
            return false;
    }
    
    if (p_first < 0)
    {
        t_chunk_count = 0;
        p_first = 0;
    }
    
    r_first = p_first;
    r_chunk_count = t_chunk_count;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCChunkGetExtentsOfByteChunkByRangeInRange(MCDataRef p_data, MCRange *p_range, integer_t p_first, integer_t p_last, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkGetExtentsByRangeInRange(p_strict, p_boundary_start, p_boundary_end, p_first, p_last, MCChunkCountByteChunkCallback, &p_data, p_range, r_first, r_chunk_count);
}

bool MCChunkGetExtentsOfByteChunkByExpressionInRange(MCDataRef p_data, MCRange *p_range, integer_t p_first, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkGetExtentsByExpressionInRange(p_strict, p_boundary_start, p_boundary_end, p_first, MCChunkCountByteChunkCallback, &p_data, p_range, r_first, r_chunk_count);
}

bool MCChunkGetExtentsOfCodeunitChunkByRangeInRange(MCStringRef p_string, MCRange *p_range, integer_t p_first, integer_t p_last, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkGetExtentsByRangeInRange(p_strict, p_boundary_start, p_boundary_end, p_first, p_last, MCChunkCountCodeunitChunkCallback, &p_string, p_range, r_first, r_chunk_count);
}

bool MCChunkGetExtentsOfCodeunitChunkByExpressionInRange(MCStringRef p_string, MCRange *p_range, integer_t p_first, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkGetExtentsByExpressionInRange(p_strict, p_boundary_start, p_boundary_end, p_first, MCChunkCountCodeunitChunkCallback, &p_string, p_range, r_first, r_chunk_count);
}

bool MCChunkGetExtentsOfGraphemeChunkByRangeInRange(MCStringRef p_string, MCRange *p_range, integer_t p_first, integer_t p_last, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkGetExtentsByRangeInRange(p_strict, p_boundary_start, p_boundary_end, p_first, p_last, MCChunkCountGraphemeChunkCallback, &p_string, p_range, r_first, r_chunk_count);
}

bool MCChunkGetExtentsOfGraphemeChunkByExpressionInRange(MCStringRef p_string, MCRange *p_range, integer_t p_first, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkGetExtentsByExpressionInRange(p_strict, p_boundary_start, p_boundary_end, p_first, MCChunkCountGraphemeChunkCallback, &p_string, p_range, r_first, r_chunk_count);
}

bool MCChunkGetExtentsOfElementChunkByRangeInRange(MCProperListRef p_list, MCRange *p_range, integer_t p_first, integer_t p_last, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkGetExtentsByRangeInRange(p_strict, p_boundary_start, p_boundary_end, p_first, p_last, MCChunkCountElementChunkCallback, &p_list, p_range, r_first, r_chunk_count);
}

bool MCChunkGetExtentsOfElementChunkByExpressionInRange(MCProperListRef p_list, MCRange *p_range, integer_t p_first, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkGetExtentsByExpressionInRange(p_strict, p_boundary_start, p_boundary_end, p_first, MCChunkCountElementChunkCallback, &p_list, p_range, r_first, r_chunk_count);
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

void MCChunkSkipWord(MCStringRef p_string, MCStringRef p_line_delimiter, MCStringOptions p_options, bool p_skip_spaces, uindex_t& x_offset)
{
    uindex_t t_length = MCStringGetLength(p_string);
    uindex_t t_end_quote_offset = t_length;
    uindex_t t_end_line_offset = t_length;
    
    if (MCStringGetCharAtIndex(p_string, x_offset) == '"')
    {
        // then bump the offset up to the next quotation mark + 1, or the beginning of the next line
        // if neither of these are present then set offset to string length.
        if (!MCStringFirstIndexOfChar(p_string, '"', x_offset + 1, kMCStringOptionCompareExact, t_end_quote_offset))
            t_end_quote_offset = t_length;
        if (!MCStringFirstIndexOf(p_string, p_line_delimiter, x_offset + 1, p_options, t_end_line_offset))
            t_end_line_offset = t_length;
        
        if (t_end_quote_offset < t_end_line_offset)
            x_offset = t_end_quote_offset + 1;
        else if (t_end_line_offset < t_end_quote_offset)
            x_offset = t_end_line_offset + MCStringGetLength(p_line_delimiter);
        else
            x_offset = t_length;
    }
    else
    {
        while (!MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_string, x_offset)) && x_offset < t_length)
            x_offset++;
    }
    
    if (p_skip_spaces)
    {
        while (MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_string, x_offset)) && x_offset < t_length)
            x_offset++;
    }
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator::MCTextChunkIterator(MCStringRef p_text, MCChunkType p_chunk_type)
{
    m_text = MCValueRetain(p_text);
    m_length = MCStringGetLength(p_text);
    m_chunk_type = p_chunk_type;
    m_range = MCRangeMake(0, 0);
    // AL-2014-10-24: [[ Bug 13783 ]] Set exhausted to true if the string is immediately exhausted
    m_exhausted = MCStringIsEmpty(p_text);
    m_options = kMCStringOptionCompareCaseless;
}

// AL-2015-02-10: [[ Bug 14532 ]] Add text chunk iterator constructor for restricted range chunk operations.
MCTextChunkIterator::MCTextChunkIterator(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction)
{
    m_text = MCValueRetain(p_text);
    m_length = p_restriction . length == UINDEX_MAX ? MCStringGetLength(m_text) : p_restriction . offset + p_restriction . length;
    m_chunk_type = p_chunk_type;
    m_range = MCRangeMake(p_restriction . offset, 0);
    // AL-2014-10-24: [[ Bug 13783 ]] Set exhausted to true if the string is immediately exhausted
    m_exhausted = (p_restriction . length == 0 || p_restriction . offset>= MCStringGetLength(m_text));
    m_options = kMCStringOptionCompareCaseless;
}

MCTextChunkIterator::~MCTextChunkIterator()
{
    MCValueRelease(m_text);
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_Grapheme::MCTextChunkIterator_Grapheme(MCStringRef p_text, MCChunkType p_chunk_type) : MCTextChunkIterator(p_text, p_chunk_type)
{
    ;
}

MCTextChunkIterator_Grapheme::MCTextChunkIterator_Grapheme(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction) : MCTextChunkIterator(p_text, p_chunk_type, p_restriction)
{
    ;
}

MCTextChunkIterator_Grapheme::~MCTextChunkIterator_Grapheme()
{
    ;
}

bool MCTextChunkIterator_Grapheme::Next()
{
    m_range . offset = m_range . offset + m_range . length;
    
    if (m_range . offset >= m_length)
        return false;
    
    uindex_t t_next;
    t_next = MCStringGraphemeBreakIteratorAdvance(m_text, m_range . offset);
    
    if (t_next == kMCLocaleBreakIteratorDone)
    {
        m_exhausted = true;
        t_next = m_length;
    }
    
    m_range . length = t_next - m_range . offset;
    return true;
}

bool MCTextChunkIterator_Grapheme::IsAmong(MCStringRef p_needle)
{
    if (MCStringIsEmpty(p_needle))
        return false;
    
    while (Next())
        if (MCStringSubstringIsEqualTo(m_text, m_range, p_needle, m_options))
            return true;
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_Codepoint::MCTextChunkIterator_Codepoint(MCStringRef p_text, MCChunkType p_chunk_type) : MCTextChunkIterator(p_text, p_chunk_type)
{
    ;
}

MCTextChunkIterator_Codepoint::MCTextChunkIterator_Codepoint(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction) : MCTextChunkIterator(p_text, p_chunk_type, p_restriction)
{
    ;
}

MCTextChunkIterator_Codepoint::~MCTextChunkIterator_Codepoint()
{
    ;
}

bool MCTextChunkIterator_Codepoint::Next()
{
    m_range . offset = m_range . offset + m_range . length;
    
    if (m_range . offset >= m_length)
        return false;
    
    if (MCStringIsValidSurrogatePair(m_text, m_range . offset))
        m_range . length = 2;
    else
        m_range . length = 1;
    
    if (m_range . offset + m_range . length == m_length)
        m_exhausted = true;
    
    return true;
}

bool MCTextChunkIterator_Codepoint::IsAmong(MCStringRef p_needle)
{
    if (MCStringIsEmpty(p_needle))
        return false;
    
    while (Next())
        if (MCStringSubstringIsEqualTo(m_text, m_range, p_needle, m_options))
            return true;
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_Codeunit::MCTextChunkIterator_Codeunit(MCStringRef p_text, MCChunkType p_chunk_type) : MCTextChunkIterator(p_text, p_chunk_type)
{
    ;
}

MCTextChunkIterator_Codeunit::MCTextChunkIterator_Codeunit(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction) : MCTextChunkIterator(p_text, p_chunk_type, p_restriction)
{
    ;
}

MCTextChunkIterator_Codeunit::~MCTextChunkIterator_Codeunit()
{
    ;
}

bool MCTextChunkIterator_Codeunit::Next()
{
    m_range . offset = m_range . offset + m_range . length;
    if (m_range . offset == m_length - 1)
        m_exhausted = true;
    else if (m_range . offset >= m_length)
        return false;
    
    m_range . length = 1;
    return true;
}

bool MCTextChunkIterator_Codeunit::IsAmong(MCStringRef p_needle)
{
    if (MCStringIsEmpty(p_needle))
        return false;
    
    return MCStringFind(m_text, MCRangeMake(0, m_length), p_needle, m_options, nil);
}

uindex_t MCTextChunkIterator_Codeunit::ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset, uindex_t *p_end_offset, bool p_whole_matches)
{
    // AL-2015-07-20: [[ Bug 15618 ]] Search for the codeunit within the specified range
    MCRange t_in_range;
    t_in_range = MCRangeMake(p_start_offset, p_end_offset != nil ? *p_end_offset : m_length);
    
    MCRange t_range;
    if (MCStringFind(m_text, t_in_range, p_needle, m_options, &t_range))
    {
        if (!p_whole_matches || t_range . length == 1)
            return t_range . offset + 1;
    }
    
    return 0;
}

uindex_t MCTextChunkIterator_Codeunit::CountChunks()
{
    return m_length;
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_Delimited::MCTextChunkIterator_Delimited(MCStringRef p_text, MCChunkType p_chunk_type, MCStringRef p_delimiter) : MCTextChunkIterator(p_text, p_chunk_type)
{
    m_delimiter = MCValueRetain(p_delimiter);
    m_delimiter_length = MCStringGetLength(p_delimiter);
    m_first_chunk = true;
}

MCTextChunkIterator_Delimited::MCTextChunkIterator_Delimited(MCStringRef p_text, MCChunkType p_chunk_type, MCStringRef p_delimiter, MCRange p_restriction) : MCTextChunkIterator(p_text, p_chunk_type, p_restriction)
{
    m_delimiter = MCValueRetain(p_delimiter);
    m_delimiter_length = MCStringGetLength(p_delimiter);
    m_first_chunk = true;
}

MCTextChunkIterator_Delimited::~MCTextChunkIterator_Delimited()
{
    MCValueRelease(m_delimiter);
}

bool MCTextChunkIterator_Delimited::Next()
{
    uindex_t t_offset = m_range . offset + m_range . length;
    
    if (!m_first_chunk)
        t_offset += m_delimiter_length;
    
    if (t_offset >= m_length)
        return false;
    
    m_range . offset = t_offset;
    m_first_chunk = false;
            
    MCRange t_found_range;
    // calculate the length of the line / item
    // AL-2015-02-10: [[ Bug 14532 ]] Use restricted range for delimiter search
    if (!MCStringFind(m_text, MCRangeMakeMinMax(t_offset, m_length), m_delimiter, m_options, &t_found_range))
    {
        m_range . length = m_length - m_range . offset;
        m_exhausted = true;
    }
    else
    {
        m_range . length = t_found_range . offset - m_range . offset;
        // AL-2014-10-15: [[ Bug 13671 ]] Keep track of matched delimiter length to increment offset correctly
        m_delimiter_length = t_found_range . length;
        
        if (t_found_range . offset + t_found_range . length == m_length)
            m_exhausted = true;
    }

    return true;
}

bool MCTextChunkIterator_Delimited::IsAmong(MCStringRef p_needle)
{
    // if the pattern is empty, we use the default behavior -
    // i.e. go through chunk by chunk to find an empty one.
    if (!MCStringIsEmpty(p_needle))
    {
        // Otherwise we need to find p_needle and check to see if there is a delimiter either side.
        // This is because of the case where the delimiter is within p_needle - e.g.
        // "a,b" is among the items of "a,b,c,d" should return true.
        return MCChunkIsAmongTheChunksOfRange(p_needle, m_text, m_delimiter, m_options, MCRangeMakeMinMax(m_range . offset, m_length));
    }
    
    while (Next())
        if (MCStringSubstringIsEqualTo(m_text, m_range, p_needle, m_options))
            return true;
    
    // AL-2014-09-10: [[ Bug 13356 ]] If we were not 'exhausted', then there was a trailing delimiter
    //  which means empty is considered to be among the chunks.
    if (MCStringIsEmpty(p_needle) && !m_exhausted)
        return true;
    
    return false;
}

uindex_t MCTextChunkIterator_Delimited::ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset, uindex_t *p_end_offset, bool p_whole_matches)
{
    // Ensure that when no item is skipped, the offset starts from the first item - without skipping it
    uindex_t t_chunk_offset;
    t_chunk_offset = 1;
    
    // Skip ahead to the first (1-indexed) chunk of interest.
    p_start_offset += 1;
    while (p_start_offset)
    {
        if (!Next())
            break;
        p_start_offset--;
    }
    
    // If we skip past the last chunk, we are done.
    if (p_start_offset > 0)
        return 0;
    
    // MW-2013-01-21: item/line/paragraph offset do not currently operate on a 'split' basis.
    //   Instead, they return the index of the chunk in which p_chunk starts and if
    //   wholeMatches is true, then before and after the found range must be the del
    //   or eos. e.g.
    //     itemOffset("a,b", "aa,b,cc") => 1 if wholeMatches false, 0 otherwise
    //     itemOffset("b,c", "a,b,c") => 2
    
    // If we're looking for empty, then we have to iterate through the chunks.
    if (!MCStringIsEmpty(p_needle))
    {
        uindex_t t_found_offset;
        if (!MCChunkOffsetOfChunkInRange(m_text, p_needle, m_delimiter, p_whole_matches, m_options, MCRangeMakeMinMax(m_range . offset, m_length), t_found_offset))
            return 0;
        
        // Count the number of delimiters between the start of the first chunk
        // and the start of the found string.
        t_chunk_offset += MCStringCount(m_text, MCRangeMakeMinMax(m_range . offset, t_found_offset), m_delimiter, m_options);
        
        // AL-2015-07-20: [[ Bug 15618 ]] If the chunk found is outside the specified range, return 0 (not found)
        if (p_end_offset != nil && t_chunk_offset > *p_end_offset)
            return 0;
        
        return t_chunk_offset;
    }
    
    // Otherwise, just iterate through the chunks.
    do
    {
        // AL-2015-07-20: [[ Bug 15618 ]] If there is an end offset, don't exceed it.
        if (p_end_offset != nil && t_chunk_offset > *p_end_offset)
            break;
        
        if (p_whole_matches)
        {
            if (MCStringSubstringIsEqualTo(m_text, MCRangeMakeMinMax(m_range . offset, m_length), p_needle, m_options))
                return t_chunk_offset;
        }
        else
        {
            if (MCStringSubstringContains(m_text, MCRangeMakeMinMax(m_range . offset, m_length), p_needle, m_options))
                return t_chunk_offset;
        }
        t_chunk_offset++;
    }
    while (Next());
    
    // if not found then return 0.
    return 0;

}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_ICU::MCTextChunkIterator_ICU(MCStringRef p_text, MCChunkType p_chunk_type) : MCTextChunkIterator(p_text, p_chunk_type)
{
    m_break_position = 0;
    MCBreakIteratorRef break_iterator;
    break_iterator = nil;
    
    switch (p_chunk_type)
    {
        case kMCChunkTypeSentence:
        {
            MCRange t_range;
            uindex_t t_end;
            /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCLocaleBasic, kMCBreakIteratorTypeSentence, break_iterator);
            /* UNCHECKED */ MCLocaleBreakIteratorSetText(break_iterator, m_text);
            t_range . length = 0;
            t_range . offset = 0;
            
            while ((t_end = MCLocaleBreakIteratorAdvance(break_iterator)) != kMCLocaleBreakIteratorDone)
            {
                t_range . offset += t_range . length;
                t_range . length = t_end - t_range . offset;
                m_breaks . Push(t_range);
            }
        }
            break;
        case kMCChunkTypeTrueWord:
        {
            MCAutoArray<uindex_t> t_breaks;
            /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCLocaleBasic, kMCBreakIteratorTypeWord, break_iterator);
            /* UNCHECKED */ MCLocaleBreakIteratorSetText(break_iterator, m_text);
            MCRange t_range = MCRangeMake(0, 0);
            
            while (MCLocaleWordBreakIteratorAdvance(m_text, break_iterator, t_range)
                   && t_range . offset + t_range . length != kMCLocaleBreakIteratorDone)
            {
                m_breaks . Push(t_range);
            }
        }
            break;
        default:
            MCUnreachable();
    }
    
    if (break_iterator != nil)
        MCLocaleBreakIteratorRelease(break_iterator);
}

MCTextChunkIterator_ICU::MCTextChunkIterator_ICU(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction) : MCTextChunkIterator(p_text, p_chunk_type, p_restriction)
{
    m_break_position = 0;
    MCBreakIteratorRef break_iterator;
    break_iterator = nil;
    
    switch (p_chunk_type)
    {
        case kMCChunkTypeSentence:
        {
            MCAutoStringRef t_substring;
            MCStringCopySubstring(m_text, p_restriction, &t_substring);
            MCRange t_range;
            uindex_t t_end;
            /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCLocaleBasic, kMCBreakIteratorTypeSentence, break_iterator);
            /* UNCHECKED */ MCLocaleBreakIteratorSetText(break_iterator, *t_substring);
            // PM-2015-05-26: [[ Bug 15422 ]] Start with zero length to make sure the first trueWord is counted
            t_range . length = 0;
            t_range . offset = p_restriction . offset;
            
            while ((t_end = MCLocaleBreakIteratorAdvance(break_iterator)) != kMCLocaleBreakIteratorDone)
            {
                t_range . offset += t_range . length;
                t_range . length = t_end - t_range . offset;
                m_breaks . Push(t_range);
            }
        }
            break;
        case kMCChunkTypeTrueWord:
        {
            MCAutoStringRef t_substring;
            MCStringCopySubstring(m_text, p_restriction, &t_substring);
            MCAutoArray<uindex_t> t_breaks;
            /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCLocaleBasic, kMCBreakIteratorTypeWord, break_iterator);
            /* UNCHECKED */ MCLocaleBreakIteratorSetText(break_iterator, *t_substring);
            MCRange t_rel_range;
            t_rel_range = MCRangeMake(0, 0);

            while (MCLocaleWordBreakIteratorAdvance(*t_substring, break_iterator, t_rel_range)
                   && t_rel_range . offset + t_rel_range . length != kMCLocaleBreakIteratorDone)
            {
                m_breaks . Push(MCRangeMake(t_rel_range . offset + p_restriction . offset,
                                            t_rel_range . length + p_restriction . length));
            }
        }
        break;
        default:
            MCUnreachable();
    }
    
    if (break_iterator != nil)
        MCLocaleBreakIteratorRelease(break_iterator);
    
}
MCTextChunkIterator_ICU::~MCTextChunkIterator_ICU()
{

}

bool MCTextChunkIterator_ICU::Next()
{
    // We have a word, sentence or character delimiter, we just have to get the range stored from the constructor
    if (m_break_position < m_breaks . Size())
    {
        m_range = m_breaks[m_break_position++];
        
        if (m_break_position == m_breaks . Size())
            m_exhausted = true;
        
        return true;
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_Word::MCTextChunkIterator_Word(MCStringRef p_text, MCChunkType p_chunk_type, MCStringRef p_line_delimiter) : MCTextChunkIterator(p_text, p_chunk_type)
{
    m_line_delimiter = MCValueRetain(p_line_delimiter);
}

MCTextChunkIterator_Word::MCTextChunkIterator_Word(MCStringRef p_text, MCChunkType p_chunk_type, MCStringRef p_line_delimiter, MCRange p_range) : MCTextChunkIterator(p_text, p_chunk_type, p_range)
{
    m_line_delimiter = MCValueRetain(p_line_delimiter);
}

MCTextChunkIterator_Word::~MCTextChunkIterator_Word()
{
    MCValueRelease(m_line_delimiter);
}

bool MCTextChunkIterator_Word::Next()
{
    uindex_t t_offset = m_range . offset + m_range . length;
    
    if (t_offset >= m_length)
        return false;
    
    m_range . offset = t_offset;
    
    // if there are consecutive spaces at the beginning, skip them
    while (t_offset < m_length && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(m_text, t_offset)))
        t_offset++;
    
    if (t_offset >= m_length)
        return false;
    
    m_range . offset = t_offset;
    
    MCChunkSkipWord(m_text, m_line_delimiter, m_options, false, t_offset);
    
    // AL-2015-02-10: [[ Bug 14532 ]] Use restricted range for exhaustion check
    if (t_offset >= m_length)
        m_exhausted = true;
    
    m_range . length = t_offset - m_range . offset;

    return true;
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator *MCChunkCreateTextChunkIterator(MCStringRef p_text, MCRange *p_range, MCChunkType p_chunk_type, MCStringRef p_line_delimiter, MCStringRef p_item_delimiter, MCStringOptions p_options)
{
    p_chunk_type = MCChunkTypeSimplify(p_text, p_chunk_type);
    
    MCTextChunkIterator *t_iterator = nil;
    
    switch (p_chunk_type)
    {
        case kMCChunkTypeSentence:
        case kMCChunkTypeTrueWord:
            if (p_range != nil)
                t_iterator = new (nothrow) MCTextChunkIterator_ICU(p_text, p_chunk_type, *p_range);
            else
                t_iterator = new (nothrow) MCTextChunkIterator_ICU(p_text, p_chunk_type);
            break;
        case kMCChunkTypeLine:
        case kMCChunkTypeItem:
            MCStringRef t_delimiter;
            if (p_chunk_type == kMCChunkTypeLine)
                t_delimiter = p_line_delimiter;
            else
                t_delimiter = p_item_delimiter;
 
            if (p_range != nil)
                t_iterator = new (nothrow) MCTextChunkIterator_Delimited(p_text, p_chunk_type, t_delimiter, *p_range);
            else
                t_iterator = new (nothrow) MCTextChunkIterator_Delimited(p_text, p_chunk_type, t_delimiter);
            break;
            
        case kMCChunkTypeParagraph:
            if (p_range != nil)
                t_iterator = new (nothrow) MCTextChunkIterator_Delimited(p_text, p_chunk_type, MCSTR("\n"), *p_range);
            else
                t_iterator = new (nothrow) MCTextChunkIterator_Delimited(p_text, p_chunk_type, MCSTR("\n"));
            break;
            
        case kMCChunkTypeWord:
            // AL-2015-10-08: [[ Bug 16161 ]] Word chunk needs to be passed line delimiter
            //  as words are also delimited by line breaks.
            if (p_range != nil)
                t_iterator = new (nothrow) MCTextChunkIterator_Word(p_text, p_chunk_type, p_line_delimiter, *p_range);
            else
                t_iterator = new (nothrow) MCTextChunkIterator_Word(p_text, p_chunk_type, p_line_delimiter);
            break;
        case kMCChunkTypeCharacter:
            if (p_range != nil)
                t_iterator = new (nothrow) MCTextChunkIterator_Grapheme(p_text, p_chunk_type, *p_range);
            else
                t_iterator = new (nothrow) MCTextChunkIterator_Grapheme(p_text, p_chunk_type);
            break;
        case kMCChunkTypeCodepoint:
            if (p_range != nil)
                t_iterator = new (nothrow) MCTextChunkIterator_Codepoint(p_text, p_chunk_type, *p_range);
            else
                t_iterator = new (nothrow) MCTextChunkIterator_Codepoint(p_text, p_chunk_type);
            break;
        case kMCChunkTypeCodeunit:
            if (p_range != nil)
                t_iterator = new (nothrow) MCTextChunkIterator_Codeunit(p_text, p_chunk_type, *p_range);
            else
                t_iterator = new (nothrow) MCTextChunkIterator_Codeunit(p_text, p_chunk_type);
            break;
        default:
            MCAssert(false);
    }
    
    t_iterator -> SetOptions(p_options);
    
    return t_iterator;
}

////////////////////////////////////////////////////////////////////////////////

MCChunkType MCChunkTypeSimplify(MCStringRef p_string, MCChunkType p_type)
{
    switch (p_type)
    {
        case kMCChunkTypeCharacter:
        {
            if (MCStringIsTrivial(p_string))
                return kMCChunkTypeCodeunit;
            
            break;
        }
        case kMCChunkTypeCodepoint:
        {
            if (MCStringIsBasic(p_string))
                return kMCChunkTypeCodeunit;
            break;
        }
        default:
            break;
    }
    
    return p_type;
}

MCChunkType MCChunkTypeFromCharChunkType(MCCharChunkType p_char_type)
{
    switch (p_char_type)
    {
        case kMCCharChunkTypeCodeunit:
            return kMCChunkTypeCodeunit;
        case kMCCharChunkTypeCodepoint:
            return kMCChunkTypeCodepoint;
        case kMCCharChunkTypeGrapheme:
            return kMCChunkTypeCharacter;
        default:
            MCUnreachableReturn(kMCChunkTypeCharacter);
    }
}
