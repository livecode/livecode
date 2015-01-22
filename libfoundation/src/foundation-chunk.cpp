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

void MCChunkGetExtentsOfGraphemeChunkByRange(MCStringRef p_string, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByRange(p_first, p_last, MCChunkCountCodeunitChunkCallback, &p_string, r_first, r_chunk_count);
}

void MCChunkGetExtentsOfGraphemeChunkByExpression(MCStringRef p_string, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count)
{
    MCChunkGetExtentsByExpression(p_first, MCChunkCountGraphemeChunkCallback, &p_string, r_first, r_chunk_count);
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

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_Codepoint::MCTextChunkIterator_Codepoint()
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
    while (Next())
        if (MCStringSubstringIsEqualTo(m_text, m_range, p_needle, m_options))
            return true;
}

uindex_t MCTextChunkIterator_Codepoint::ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset)
{
    
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_Codeunit::MCTextChunkIterator_Codeunit()
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
    
}

uindex_t MCTextChunkIterator_Codeunit::ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset)
{
    
}

////////////////////////////////////////////////////////////////////////////////








MCTextChunkIterator *MCChunkCreateTextChunkIterator(MCStringRef p_text, Chunk_term p_chunk_type)
{
    /* UNCHECKED */ MCStringCopy(p_text, m_text);
    m_chunk_type = p_chunk_type;
    
    if (m_chunk_type == CT_CHARACTER && (MCStringIsNative(m_text) || (MCStringIsSimple(m_text) && MCStringIsUncombined(m_text))))
        m_chunk_type = CT_CODEUNIT;
    
    MCBreakIteratorRef break_iterator;
    
    break_iterator = nil;
    sp = nil;
    range = MCRangeMake(0, 0);
    // AL-2014-10-24: [[ Bug 13783 ]] Set exhausted to true if the string is immediately exhausted
    exhausted = MCStringIsEmpty(p_text);
    length = MCStringGetLength(text);
    first_chunk = true;
    break_position = 0;
    delimiter_length = 0;
    
    switch (type)
    {
        case CT_TOKEN:
            sp = new MCScriptPoint(p_text);
            break;
        case CT_CHARACTER:
        case CT_SENTENCE:
        {
            MCRange t_range;
            uindex_t t_end;
            /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCBasicLocale, p_chunk_type == CT_SENTENCE ? kMCBreakIteratorTypeSentence : kMCBreakIteratorTypeCharacter, break_iterator);
            /* UNCHECKED */ MCLocaleBreakIteratorSetText(break_iterator, text);
            t_range . length = 0;
            t_range . offset = 0;
            
            while ((t_end = MCLocaleBreakIteratorAdvance(break_iterator)) != kMCLocaleBreakIteratorDone)
            {
                t_range . offset += t_range . length;
                t_range . length = t_end - t_range . offset;
                breaks . Push(t_range);
            }
        }
            break;
        case CT_TRUEWORD:
        {
            MCAutoArray<uindex_t> t_breaks;
            /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCBasicLocale, kMCBreakIteratorTypeWord, break_iterator);
            /* UNCHECKED */ MCLocaleBreakIteratorSetText(break_iterator, text);
            MCRange t_range = MCRangeMake(0, 0);
            
            while (MCLocaleWordBreakIteratorAdvance(text, break_iterator, t_range)
                   && t_range . offset + t_range . length != kMCLocaleBreakIteratorDone)
            {
                breaks . Push(t_range);
            }
        }
            break;
        case CT_LINE:
        case CT_ITEM:
        case CT_PARAGRAPH:
            // delimiter length may vary for line and item.
            delimiter_length = 1;
        default:
            break;
    }
    
    if (break_iterator != nil)
        MCLocaleBreakIteratorRelease(break_iterator);
}

////////////////////////////////////////////////////////////////////////////////

static bool MCStringsIsAmongTheChunksOfRange(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, Chunk_term p_chunk_type, MCRange p_range)
{
    MCStringRef t_delimiter;
    t_delimiter = p_chunk_type == CT_ITEM ? ctxt . GetItemDelimiter() : ctxt . GetLineDelimiter();
    
    return MCChunkIsAmongTheChunksOfRange(p_chunk, p_string, t_delimiter, ctxt . GetStringComparisonType(), p_range);
}

static bool MCStringsIsAmongTheParagraphsOfRange(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, MCStringOptions p_options, MCRange p_range)
{
    MCRange t_range;
    if (!MCStringFind(p_string, p_range, p_chunk, p_options, &t_range))
        return false;
    
    codepoint_t t_delimiter;
    // if there is no delimiter to the left then continue searching the string.
    if (t_range . offset != 0)
    {
        t_delimiter = MCStringGetCodepointAtIndex(p_string, t_range . offset - 1);
        // AL-2014-07-21: [[ Bug 12162 ]] Ignore PS when calculating paragraph chunk.
        if (t_delimiter != '\n' /*&& t_delimiter != 0x2029*/)
            return MCStringsIsAmongTheParagraphsOfRange(ctxt, p_chunk, p_string, p_options, MCRangeMake(t_range . offset + t_range . length, p_range . length));
    }
    
    // if there is no delimiter to the right then continue searching the string.
    if (t_range . offset + t_range . length != MCStringGetLength(p_string))
    {
        t_delimiter = MCStringGetCodepointAtIndex(p_string, t_range . offset + t_range . length);
        // AL-2014-07-21: [[ Bug 12162 ]] Ignore PS when calculating paragraph chunk.
        if (t_delimiter != '\n' /*&& t_delimiter != 0x2029*/)
            return MCStringsIsAmongTheParagraphsOfRange(ctxt, p_chunk, p_string, p_options, MCRangeMake(t_range . offset + t_range . length, p_range . length));
    }
    return true;
}

static bool MCStringsFindChunkInRange(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_needle, Chunk_term p_chunk_type, MCRange p_range, uindex_t& r_offset)
{
    // Work out the delimiter.
    MCStringRef t_delimiter;
    t_delimiter = p_chunk_type == CT_ITEM ? ctxt . GetItemDelimiter() : ctxt . GetLineDelimiter();
    
    return MCChunkOffsetOfChunkInRange(p_string, p_needle, t_delimiter, ctxt . GetWholeMatches(), ctxt . GetStringComparisonType(), p_range, r_offset);
}

static bool MCStringsFindParagraphInRange(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_needle, MCStringOptions p_options, MCRange p_range, uindex_t& r_offset)
{
    // If we can't find the chunk in the remainder of the string, we are done.
    MCRange t_range;
    if (!MCStringFind(p_string, p_range, p_needle, p_options, &t_range))
        return false;
    
    // If we are in wholeMatches mode, ensure the delimiter is either side.
    if (ctxt . GetWholeMatches())
    {
        codepoint_t t_delimiter;
        // if there is no delimiter to the left then continue searching the string.
        if (t_range . offset != 0)
        {
            t_delimiter = MCStringGetCodepointAtIndex(p_string, t_range . offset - 1);
            // AL-2014-07-21: [[ Bug 12162 ]] Ignore PS when calculating paragraph chunk.
            if (t_delimiter != '\n' /*&& t_delimiter != 0x2029*/)
                return MCStringsFindParagraphInRange(ctxt, p_string, p_needle, p_options, MCRangeMake(t_range . offset + t_range . length, p_range . length), r_offset);
        }
        
        // if there is no delimiter to the right then continue searching the string.
        if (t_range . offset + t_range . length != MCStringGetLength(p_string))
        {
            t_delimiter = MCStringGetCodepointAtIndex(p_string, t_range . offset + t_range . length);
            // AL-2014-07-21: [[ Bug 12162 ]] Ignore PS when calculating paragraph chunk.
            if (t_delimiter != '\n' /*&& t_delimiter != 0x2029*/)
                return MCStringsFindParagraphInRange(ctxt, p_string, p_needle, p_options, MCRangeMake(t_range . offset + t_range . length, p_range . length), r_offset);
        }
    }
    
    r_offset = t_range . offset;
    return true;
}

MCTextChunkIterator::MCTextChunkIterator(Chunk_term p_chunk_type, MCStringRef p_text)
{
    /* UNCHECKED */ MCStringCopy(p_text, text);
    type = p_chunk_type;
    
    if (type == CT_CHARACTER && (MCStringIsNative(text) || (MCStringIsSimple(text) && MCStringIsUncombined(text))))
        type = CT_CODEUNIT;
    
    MCBreakIteratorRef break_iterator;
    
    break_iterator = nil;
    sp = nil;
    range = MCRangeMake(0, 0);
    // AL-2014-10-24: [[ Bug 13783 ]] Set exhausted to true if the string is immediately exhausted
    exhausted = MCStringIsEmpty(p_text);
    length = MCStringGetLength(text);
    first_chunk = true;
    break_position = 0;
    delimiter_length = 0;
    
    switch (type)
    {
        case CT_TOKEN:
            sp = new MCScriptPoint(p_text);
            break;
        case CT_CHARACTER:
        case CT_SENTENCE:
        {
            MCRange t_range;
            uindex_t t_end;
            /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCBasicLocale, p_chunk_type == CT_SENTENCE ? kMCBreakIteratorTypeSentence : kMCBreakIteratorTypeCharacter, break_iterator);
            /* UNCHECKED */ MCLocaleBreakIteratorSetText(break_iterator, text);
            t_range . length = 0;
            t_range . offset = 0;
            
            while ((t_end = MCLocaleBreakIteratorAdvance(break_iterator)) != kMCLocaleBreakIteratorDone)
            {
                t_range . offset += t_range . length;
                t_range . length = t_end - t_range . offset;
                breaks . Push(t_range);
            }
        }
            break;
        case CT_TRUEWORD:
        {
            MCAutoArray<uindex_t> t_breaks;
            /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCBasicLocale, kMCBreakIteratorTypeWord, break_iterator);
            /* UNCHECKED */ MCLocaleBreakIteratorSetText(break_iterator, text);
            MCRange t_range = MCRangeMake(0, 0);
            
            while (MCLocaleWordBreakIteratorAdvance(text, break_iterator, t_range)
                   && t_range . offset + t_range . length != kMCLocaleBreakIteratorDone)
            {
                breaks . Push(t_range);
            }
        }
            break;
        case CT_LINE:
        case CT_ITEM:
        case CT_PARAGRAPH:
            // delimiter length may vary for line and item.
            delimiter_length = 1;
        default:
            break;
    }
    
    if (break_iterator != nil)
        MCLocaleBreakIteratorRelease(break_iterator);
}

MCTextChunkIterator::~MCTextChunkIterator()
{
    MCValueRelease(text);
    delete sp;
}

bool MCTextChunkIterator::next(MCExecContext& ctxt)
{
    if (type == CT_TRUEWORD || type == CT_SENTENCE || type == CT_CHARACTER)
    {
        // We have a word, sentence or character delimiter, we just have to get the range stored from the constructor
        if (break_position < breaks . Size())
        {
            range = breaks[break_position++];
            
            if (break_position == breaks . Size())
                exhausted = true;
            
            return true;
        }
        
        return false;
    }
    
    if (sp != nil)
    {
        MCerrorlock++;
        
        bool t_found = true;
        uint2 t_pos;
        Parse_stat ps = sp -> nexttoken();
        if (ps == PS_ERROR || ps == PS_EOF)
            t_found = false;
        
        if (t_found)
        {
            range . offset = sp -> getindex();
            range . length = MCStringGetLength(sp -> gettoken_stringref());
        }
        
        return t_found;
    }
    
    uindex_t t_offset = range . offset + range . length;
    
    if (!first_chunk)
        t_offset += delimiter_length;
    
    if (t_offset >= length)
        return false;
    
    range . offset = t_offset;
    first_chunk = false;
    
    switch (type)
    {
        case CT_LINE:
        case CT_ITEM:
        {
            MCStringRef t_line_delimiter = ctxt . GetLineDelimiter();
            MCStringRef t_item_delimiter = ctxt . GetItemDelimiter();
            
            MCStringRef t_delimiter = (type == CT_LINE) ? t_line_delimiter : t_item_delimiter;
            
            MCRange t_found_range;
            // calculate the length of the line / item
            if (!MCStringFind(text, MCRangeMake(t_offset, UINDEX_MAX), t_delimiter, ctxt . GetStringComparisonType(), &t_found_range))
            {
                range . length = length - range . offset;
                exhausted = true;
            }
            else
            {
                range . length = t_found_range . offset - range . offset;
                // AL-2014-10-15: [[ Bug 13671 ]] Keep track of matched delimiter length to increment offset correctly
                delimiter_length = t_found_range . length;
            }
            
        }
            return true;
            
        case CT_PARAGRAPH:
        {
            uindex_t t_pg_offset;
            bool t_newline_found, t_pg_found;
            
            t_pg_offset = t_offset;
            t_newline_found = MCStringFirstIndexOfChar(text, '\n', t_offset, kMCCompareExact, t_offset);
            // AL-2014-07-21: [[ Bug 12162 ]] Ignore PS when calculating paragraph chunk.
            t_pg_found = false; /*MCStringFirstIndexOfChar(text, 0x2029, t_pg_offset, kMCCompareExact, t_pg_offset);*/
            
            t_offset = MCU_min(t_newline_found ? t_offset : UINDEX_MAX, t_pg_found ? t_pg_offset : UINDEX_MAX);
            
            // calculate the length of the paragraph
            if (t_newline_found || t_pg_found)
                range . length = t_offset - range . offset;
            else
            {
                // AL-2014-03-20: [[ Bug 11945 ]] We've got a final paragraph if delimiters are not found
                range . length = length - t_offset;
                exhausted = true;
            }
        }
            return true;
            
        case CT_WORD:
        {
            // if there are consecutive spaces at the beginning, skip them
            while (t_offset < length && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(text, t_offset)))
                t_offset++;
            
            if (t_offset >= length)
                return false;
            
            range . offset = t_offset;
            
            MCStringsSkipWord(ctxt, text, false, t_offset);
            
            if (t_offset == length)
                exhausted = true;
            
            range . length = t_offset - range . offset;
        }
            return true;
            
        case CT_CODEPOINT:
            range . length = MCStringIsValidSurrogatePair(text, range . offset) ? 2 : 1;
            return true;
            
        case CT_CODEUNIT:
        case CT_BYTE:
            range . length = 1;
            
            if (t_offset == length - 1)
                exhausted = true;
            
            return true;
            
        default:
            assert(false);
    }
}

bool MCTextChunkIterator::isamong(MCExecContext& ctxt, MCStringRef p_needle)
{
    switch (type)
    {
        case CT_LINE:
        case CT_ITEM:
        case CT_PARAGRAPH:
        {
            // if the pattern is empty, we use the default behavior -
            // i.e. go through chunk by chunk to find an empty one.
            if (MCStringIsEmpty(p_needle))
                break;
            
            
            // Otherwise we need to find p_needle and check to see if there is a delimiter either side.
            // This is because of the case where the delimiter is within p_needle - e.g.
            // "a,b" is among the items of "a,b,c,d" should return true.
            
            if (type == CT_PARAGRAPH)
                return MCStringsIsAmongTheParagraphsOfRange(ctxt, p_needle, text, ctxt . GetStringComparisonType(), MCRangeMake(0, length));
            
            return MCStringsIsAmongTheChunksOfRange(ctxt, p_needle, text, type, MCRangeMake(0, length));
        }
        default:
            if (MCStringIsEmpty(p_needle))
                return false;
            break;
    }
    
    while (next(ctxt))
        if (MCStringSubstringIsEqualTo(text, range, p_needle, ctxt . GetStringComparisonType()))
            return true;
    
    // AL-2014-09-10: [[ Bug 13356 ]] If we were not 'exhausted', then there was a trailing delimiter
    //  which means empty is considered to be among the chunks.
    if (MCStringIsEmpty(p_needle) && !exhausted)
        return true;
    
    return false;
}

uindex_t MCTextChunkIterator::chunkoffset(MCExecContext& ctxt, MCStringRef p_needle, uindex_t p_start_offset)
{
    MCStringOptions t_options;
    t_options = ctxt.GetStringComparisonType();
    
    // Ensure that when no item is skipped, the offset starts from the first item - without skipping it
    uindex_t t_chunk_offset;
    t_chunk_offset = 1;
    
    // Skip ahead to the first (1-indexed) chunk of interest.
    p_start_offset += 1;
    while (p_start_offset)
    {
        if (!next(ctxt))
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
    
    switch (type)
    {
        case CT_ITEM:
        case CT_LINE:
        case CT_PARAGRAPH:
        {
            // If we're looking for empty, then we have to iterate through the chunks.
            if (MCStringIsEmpty(p_needle))
                break;
            
            uindex_t t_found_offset;
            if (type != CT_PARAGRAPH)
            {
                if (!MCStringsFindChunkInRange(ctxt, text, p_needle, type, MCRangeMake(range . offset, length - range . offset), t_found_offset))
                    return 0;
            }
            else
            {
                if (!MCStringsFindParagraphInRange(ctxt, text, p_needle, t_options, MCRangeMake(range . offset, length - range . offset), t_found_offset))
                    return 0;
            }
            
            MCStringRef t_delimiter;
            t_delimiter = type == CT_ITEM ? ctxt . GetItemDelimiter() : ctxt . GetLineDelimiter();
            
            // Count the number of delimiters between the start of the first chunk
            // and the start of the found string.
            
            // AL-2014-07-21: [[ Bug 12162 ]] Ignore PS when calculating paragraph chunk.
            if (type != CT_PARAGRAPH)
                t_chunk_offset += MCStringCount(text, MCRangeMake(range . offset, t_found_offset - range . offset), t_delimiter, t_options);
            else
                t_chunk_offset += MCStringCountChar(text, MCRangeMake(range . offset, t_found_offset - range . offset), '\n', t_options);
            
            return t_chunk_offset;
        }
        default:
            break;
    }
    
    // Otherwise, just iterate through the chunks.
    do
    {
        if (ctxt.GetWholeMatches())
        {
            if (MCStringSubstringIsEqualTo(text, range, p_needle, t_options))
                return t_chunk_offset;
        }
        else
        {
            if (MCStringSubstringContains(text, range, p_needle, t_options))
                return t_chunk_offset;
        }
        t_chunk_offset++;
    }
    while (next(ctxt));
    
    // if not found then return 0.
    return 0;
}
