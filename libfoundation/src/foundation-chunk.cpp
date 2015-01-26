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
#include <foundation-locale.h>
#include <foundation-unicode.h>

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

uinteger_t MCChunkCountGraphemeChunkCallback(void *context)
{
    MCRange t_grapheme_range;
    MCStringUnmapGraphemeIndices(*(MCStringRef *)context, kMCLocaleBasic, MCRangeMake(0, MCStringGetLength(*(MCStringRef *)context)), t_grapheme_range);
    return t_grapheme_range . length;
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

bool MCChunkEnsureExtentsByRange(bool p_strict, integer_t p_first, integer_t p_last, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count)
{
    int32_t t_chunk_count;
    uinteger_t t_count;
    bool t_counted;
    t_counted = false;
    
    if (p_first < 0 || p_last < 0)
    {
        t_count = p_callback(p_context);
        t_counted = true;
        
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
    
    if (p_strict)
    {
        if (t_chunk_count == 0)
            return false;
        
        if (!t_counted)
            t_count = p_callback(p_context);
        
        if (p_first + t_chunk_count > t_count)
            return false;
    }
    
    r_chunk_count = t_chunk_count;
    r_first = p_first;
    return true;
}

void MCChunkGetExtentsByRange(integer_t p_first, integer_t p_last, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkEnsureExtentsByRange(false, p_first, p_last, p_callback, p_context, r_first, r_chunk_count);
}

bool MCChunkEnsureExtentsByExpression(bool p_strict, integer_t p_first, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count)
{
    int32_t t_chunk_count;
    t_chunk_count = 1;
    
    uinteger_t t_count;
    bool t_counted;
    t_counted = false;
    
    if (p_first < 0)
    {
        t_count = p_callback(p_context);
        t_counted = true;
        p_first += t_count;
    }
    else
        p_first--;
    
    if (p_first < 0)
    {
        t_chunk_count = 0;
        p_first = 0;
    }
    
    if (p_strict)
    {
        if (t_chunk_count == 0)
            return false;
        
        if (!t_counted)
            t_count = p_callback(p_context);
        
        if (p_first + t_chunk_count > t_count)
            return false;
    }
    
    r_first = p_first;
    r_chunk_count = t_chunk_count;
    return true;
}

void MCChunkGetExtentsByExpression(integer_t p_first, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkEnsureExtentsByExpression(false, p_first, p_callback, p_context, r_first, r_chunk_count);
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

bool MCChunkGetExtentsOfGraphemeChunkByRange(MCStringRef p_string, integer_t p_first, integer_t p_last, bool p_strict, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkEnsureExtentsByRange(p_strict, p_first, p_last, MCChunkCountGraphemeChunkCallback, &p_string, r_first, r_chunk_count);
}

bool MCChunkGetExtentsOfGraphemeChunkByExpression(MCStringRef p_string, integer_t p_first, bool p_strict, uindex_t& r_first, uindex_t& r_chunk_count)
{
    return MCChunkEnsureExtentsByExpression(p_strict, p_first, MCChunkCountGraphemeChunkCallback, &p_string, r_first, r_chunk_count);
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
    // Reimplement with MCTextChunkIterator_Delimited to accommodate arbitrary delimiters.
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

void MCChunkSkipWord(MCStringRef p_string, MCStringRef p_line_delimiter, MCStringOptions p_options, bool p_skip_spaces, uindex_t& x_offset)
{
    uindex_t t_space_offset;
    uindex_t t_length = MCStringGetLength(p_string);
    uindex_t t_end_quote_offset = t_length;
    uindex_t t_end_line_offset = t_length;
    
    if (MCStringGetCharAtIndex(p_string, x_offset) == '"')
    {
        // then bump the offset up to the next quotation mark + 1, or the beginning of the next line
        // if neither of these are present then set offset to string length.
        MCStringFirstIndexOfChar(p_string, '"', x_offset + 1, kMCStringOptionCompareExact, t_end_quote_offset);
        MCStringFirstIndexOf(p_string, p_line_delimiter, x_offset + 1, p_options, t_end_line_offset);
        
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
    m_exhausted = MCStringIsEmpty(p_text);
}

MCTextChunkIterator::~MCTextChunkIterator()
{
    MCValueRelease(m_text);
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_Codepoint::MCTextChunkIterator_Codepoint(MCStringRef p_text, MCChunkType p_chunk_type) : MCTextChunkIterator(p_text, p_chunk_type)
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
    
    m_range . length = MCStringIsValidSurrogatePair(m_text, m_range . offset) ? 2 : 1;
    
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

MCTextChunkIterator_Codeunit::~MCTextChunkIterator_Codeunit()
{
    ;
}

bool MCTextChunkIterator_Codeunit::Next()
{
    m_range . offset = m_range . offset + m_range . length;
    if (m_range . offset >= m_length)
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

uindex_t MCTextChunkIterator_Codeunit::ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset, bool p_whole_matches)
{
    MCRange t_range;
    if (MCStringFind(m_text, MCRangeMake(0, m_length), p_needle, m_options, &t_range))
    {
        if (!p_whole_matches || t_range . length == 1)
            return t_range . offset;
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
    
    // AL-2014-10-24: [[ Bug 13783 ]] Set exhausted to true if the string is immediately exhausted
    m_exhausted = MCStringIsEmpty(p_text);
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
    if (!MCStringFind(m_text, MCRangeMake(t_offset, UINDEX_MAX), m_delimiter, m_options, &t_found_range))
    {
        m_range . length = m_length - m_range . offset;
        m_exhausted = true;
    }
    else
    {
        m_range . length = t_found_range . offset - m_range . offset;
        // AL-2014-10-15: [[ Bug 13671 ]] Keep track of matched delimiter length to increment offset correctly
        m_delimiter_length = t_found_range . length;
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
        return MCChunkIsAmongTheChunksOfRange(p_needle, m_text, m_delimiter, m_options, MCRangeMake(0, m_length));
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

uindex_t MCTextChunkIterator_Delimited::ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset, bool p_whole_matches)
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
        if (!MCChunkOffsetOfChunkInRange(m_text, p_needle, m_delimiter, p_whole_matches, m_options, MCRangeMake(0, m_length), t_found_offset))
            return 0;
        
        // Count the number of delimiters between the start of the first chunk
        // and the start of the found string.
        t_chunk_offset += MCStringCount(m_text, MCRangeMake(m_range . offset, t_found_offset - m_range . offset), m_delimiter, m_options);
        return t_chunk_offset;
    }
    
    // Otherwise, just iterate through the chunks.
    do
    {
        if (p_whole_matches)
        {
            if (MCStringSubstringIsEqualTo(m_text, m_range, p_needle, m_options))
                return t_chunk_offset;
        }
        else
        {
            if (MCStringSubstringContains(m_text, m_range, p_needle, m_options))
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
    MCBreakIteratorRef break_iterator;
    break_iterator = nil;
    
    switch (p_chunk_type)
    {
        case kMCChunkTypeCharacter:
        case kMCChunkTypeSentence:
        {
            MCRange t_range;
            uindex_t t_end;
            /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCLocaleBasic, p_chunk_type == kMCChunkTypeSentence ? kMCBreakIteratorTypeSentence : kMCBreakIteratorTypeCharacter, break_iterator);
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
            break;
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
    
    if (t_offset == m_length)
        m_exhausted = true;
    
    m_range . length = t_offset - m_range . offset;

    return true;
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator *MCChunkCreateTextChunkIterator(MCStringRef p_text, MCChunkType p_chunk_type, MCStringRef p_delimiter, MCStringOptions p_options)
{
    if (p_chunk_type == kMCChunkTypeCharacter && (MCStringIsNative(p_text) || (MCStringIsSimple(p_text) && MCStringIsUncombined(p_text))))
        p_chunk_type = kMCChunkTypeCodeunit;
    
    MCTextChunkIterator *t_iterator = nil;
    
    switch (p_chunk_type)
    {
        case kMCChunkTypeCharacter:
        case kMCChunkTypeSentence:
        case kMCChunkTypeTrueWord:
            t_iterator = new MCTextChunkIterator_ICU(p_text, p_chunk_type);
            break;
        case kMCChunkTypeLine:
        case kMCChunkTypeItem:
            t_iterator = new MCTextChunkIterator_Delimited(p_text, p_chunk_type, p_delimiter);
            break;
        case kMCChunkTypeParagraph:
            t_iterator = new MCTextChunkIterator_Delimited(p_text, p_chunk_type, MCSTR("\n"));
            break;
        case kMCChunkTypeWord:
            t_iterator = new MCTextChunkIterator_Word(p_text, p_chunk_type, p_delimiter);
            break;
        case kMCChunkTypeCodepoint:
            t_iterator = new MCTextChunkIterator_Codepoint(p_text, p_chunk_type);
            break;
        case kMCChunkTypeCodeunit:
            t_iterator = new MCTextChunkIterator_Codeunit(p_text, p_chunk_type);
            break;
        default:
            MCAssert(false);
    }
    
    t_iterator -> SetOptions(p_options);
    
    return t_iterator;
}

////////////////////////////////////////////////////////////////////////////////
