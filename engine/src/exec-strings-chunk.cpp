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

#include "prefix.h"
#include "foundation-chunk.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "handler.h"

#include "scriptpt.h"
#include "util.h"
#include "chunk.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

struct MCChunkCountState
{
    MCStringRef string;
    Chunk_term chunk;
    MCExecContext *ctxt;
};

// AL-2015-02-10: [[ Bug 14532 ]] Allow chunks to be counted in a given range, to prevent substring copying in text chunk resolution.
void MCStringsCountChunksInRange(MCExecContext& ctxt, Chunk_term p_chunk_type, MCStringRef p_string, MCRange p_range, uinteger_t& r_count)
{
    if (p_range . length == 0 || p_range . offset > MCStringGetLength(p_string))
    {
        r_count = 0;
        return;
    }
    
    MCChunkType t_type;
    t_type = MCChunkTypeFromChunkTerm(p_chunk_type);
    
    // When the string doesn't contain combining characters or surrogate
    // pairs, we can shortcut.
    t_type = MCChunkTypeSimplify(p_string, t_type);
        
    if (t_type == kMCChunkTypeCodeunit)
    {
        // AL-2015-03-23: [[ Bug 15045 ]] Clamp range correctly
        r_count = MCU_min(MCStringGetLength(p_string) - p_range . offset, p_range . length);
        return;
    }

    MCAutoPointer<MCTextChunkIterator> tci;
    tci = MCStringsTextChunkIteratorCreateWithRange(ctxt, p_string, p_range, p_chunk_type);
    
    r_count = tci -> CountChunks();
    
    return;
}

void MCStringsCountChunks(MCExecContext& ctxt, Chunk_term p_chunk_type, MCStringRef p_string, uinteger_t& r_count)
{
    MCStringsCountChunksInRange(ctxt, p_chunk_type, p_string, MCRangeMake(0, MCStringGetLength(p_string)), r_count);
}

uinteger_t MCStringsCountChunkCallback(void *context, MCRange *p_range)
{
    MCChunkCountState *t_state = static_cast<MCChunkCountState *>(context);
    uinteger_t t_count;
    if (p_range != nil)
        MCStringsCountChunksInRange(*t_state -> ctxt, t_state -> chunk, t_state -> string, *p_range, t_count);
    else
        MCStringsCountChunks(*t_state -> ctxt, t_state -> chunk, t_state -> string, t_count);
    return t_count;
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsSkipWord(MCExecContext& ctxt, MCStringRef p_string, bool p_skip_spaces, uindex_t& x_offset)
{
    MCChunkSkipWord(p_string, ctxt . GetLineDelimiter(), ctxt . GetStringComparisonType(), p_skip_spaces, x_offset);
}

// AL-2015-02-10: [[ Bug 14532 ]] Allow chunk extents to be counted in a given range, to prevent substring copying in text chunk resolution.
void MCStringsGetExtentsByOrdinalInRange(MCExecContext& ctxt, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, MCValueRef p_string, MCRange *p_range, uinteger_t& r_first, uinteger_t& r_chunk_count)
{
    uinteger_t t_count = 0;
    switch (p_ordinal_type)
    {
        case CT_ANY:
        case CT_LAST:
        case CT_MIDDLE:
            if (MCValueGetTypeCode(p_string) == kMCValueTypeCodeData)
                t_count = p_range != nil ? p_range -> length : MCDataGetLength((MCDataRef)p_string);
            else
            {
                if (p_range != nil)
                    MCStringsCountChunksInRange(ctxt, p_chunk_type, (MCStringRef)p_string, *p_range, t_count);
                else
                    MCStringsCountChunks(ctxt, p_chunk_type, (MCStringRef)p_string, t_count);
            }
            
            // AL-2015-04-09: [[ Bug 15156 ]] Prevent underflow of r_first
            if (t_count == 0)
            {
                r_first = 0;
                r_chunk_count = 0;
                return;
            }
            
            if (p_ordinal_type == CT_ANY)
                r_first = MCU_any(t_count);
            else if (p_ordinal_type == CT_LAST)
                r_first = t_count - 1;
            else
                r_first = t_count / 2;
            break;
        case CT_FIRST:
        case CT_SECOND:
        case CT_THIRD:
        case CT_FOURTH:
        case CT_FIFTH:
        case CT_SIXTH:
        case CT_SEVENTH:
        case CT_EIGHTH:
        case CT_NINTH:
        case CT_TENTH:
            r_first = p_ordinal_type - CT_FIRST;
            break;
        default:
            // SN-2014-12-15: [[ Bug 14211 ]] bad extents shoudl throw an error. It was returning
            //  a non-initialised value in 6.7.
            fprintf(stderr, "MCChunk: ERROR bad extents\n");
            ctxt . LegacyThrow(EE_CHUNK_BADEXTENTS);
            return;
	}

    r_chunk_count = 1;
}

// AL-2015-02-10: [[ Bug 14532 ]] Allow chunk extents to be counted in a given range, to prevent substring copying in text chunk resolution.
void MCStringsGetExtentsByRangeInRange(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, MCValueRef p_string, MCRange *p_range, uinteger_t& r_first, uinteger_t& r_chunk_count)
{
    if (MCValueGetTypeCode(p_string) == kMCValueTypeCodeData)
        MCChunkGetExtentsOfByteChunkByRangeInRange((MCDataRef)p_string, p_range, p_first, p_last, false, false, false, r_first, r_chunk_count);
    else if (p_chunk_type == CT_CODEUNIT)
        MCChunkGetExtentsOfCodeunitChunkByRangeInRange((MCStringRef)p_string, p_range, p_first, p_last, false, false, false, r_first, r_chunk_count);
    else
    {
        MCChunkCountState t_state;
        t_state . string = (MCStringRef)p_string;
        t_state . chunk = p_chunk_type;
        t_state . ctxt = &ctxt;
        MCChunkGetExtentsByRangeInRange(false, false, false, p_first, p_last, MCStringsCountChunkCallback, &t_state, p_range, r_first, r_chunk_count);
    }
}

// AL-2015-02-10: [[ Bug 14532 ]] Allow chunk extents to be counted in a given range, to prevent substring copying in text chunk resolution.
void MCStringsGetExtentsByExpressionInRange(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, MCStringRef p_string, MCRange *p_range, uinteger_t& r_first, uinteger_t& r_chunk_count)
{
    if (MCValueGetTypeCode(p_string) == kMCValueTypeCodeData)
        MCChunkGetExtentsOfByteChunkByExpressionInRange((MCDataRef)p_string, p_range, p_first, false, false, false, r_first, r_chunk_count);
    else if (p_chunk_type == CT_CODEUNIT)
        MCChunkGetExtentsOfCodeunitChunkByExpressionInRange((MCStringRef)p_string, p_range, p_first, false, false, false, r_first, r_chunk_count);
    else
    {
        MCChunkCountState t_state;
        t_state . string = (MCStringRef)p_string;
        t_state . chunk = p_chunk_type;
        t_state . ctxt = &ctxt;
        MCChunkGetExtentsByExpressionInRange(false, false, false, p_first, MCStringsCountChunkCallback, &t_state, p_range, r_first, r_chunk_count);
    }
}

// AL-2015-02-10: [[ Bug 14532 ]] Allow chunk marking in a given range, to prevent substring copying in text chunk resolution.
void MCStringsMarkTextChunkInRange(MCExecContext& ctxt, MCStringRef p_string, MCRange p_range, Chunk_term p_chunk_type, integer_t p_first, integer_t p_count, integer_t& r_start, integer_t& r_end, bool p_whole_chunk, bool p_further_chunks, bool p_include_chars, integer_t& r_add)
{
    r_add = 0;
    if (p_count == 0 && p_chunk_type != CT_CHARACTER && p_chunk_type != CT_WORD)
    {
        r_start = 0;
        r_end = 0;
        return;
    }
    
    uindex_t t_string_length;
    t_string_length = MCStringGetLength(p_string);
    uindex_t t_length = p_range . offset + p_range . length > t_string_length ? t_string_length : p_range . offset + p_range . length;
    
    if (t_length == 0)
    {
        r_start = 0;
        r_end = 0;
        r_add = p_first;
        return;
    }
    
    uindex_t t_end_index = t_length - 1;
    uindex_t t_offset = p_range . offset;
    
    switch (p_chunk_type)
    {
        case CT_LINE:
        case CT_ITEM:
        {
            MCStringRef t_line_delimiter = ctxt . GetLineDelimiter();
            MCStringRef t_item_delimiter = ctxt . GetItemDelimiter();
            
            MCStringRef t_delimiter = (p_chunk_type == CT_LINE) ? t_line_delimiter : t_item_delimiter;
            MCRange t_found_range;
            
            // calculate the start of the (p_first)th line or item
            while (p_first && MCStringFind(p_string, MCRangeMakeMinMax(t_offset, t_length), t_delimiter, ctxt . GetStringComparisonType(), &t_found_range))
            {
                p_first--;
                t_offset = t_found_range . offset + t_found_range . length;
            }
            
            // if we couldn't find enough delimiters, set r_add to the number of
            // additional delimiters required and set the offset to the end
            if (p_first > 0)
            {
                t_offset = t_length;
                r_add = p_first;
            }
            
            r_start = t_offset;
            
            // calculate the length of the next p_count lines / items
            while (p_count--)
            {
                if (t_offset > t_end_index || !MCStringFind(p_string, MCRangeMakeMinMax(t_offset, t_length), t_delimiter, ctxt . GetStringComparisonType(), &t_found_range))
                {
                    r_end = t_length;
                    break;
                }
                if (p_count == 0)
                    r_end = t_found_range . offset;
                else
                    t_offset = t_found_range . offset + t_found_range . length;
            }
            
            if (p_whole_chunk && !p_further_chunks)
            {
                // AL-2014-10-15: [[ Bug 13680 ]] Make sure the previously found delimiter's length is used to adjust the string offsets.
                
                // Wholechunk operations need additional processing of mark indices to preserve the presence or otherwise of trailing delimiters.
                // If we found a trailing delimiter for this item or line, make sure it is included in the mark.
                // e.g. mark item 3 of a,b,c, -> a,b,(c,) so that delete item 3 of a,b,c, -> a,b,
                
	            if (r_end >= 0 && (uindex_t) r_end < t_length)
                    r_end += t_found_range . length;
                // If we didn't, and this operation does not force additional delimiters, then include the previous delimiter in the mark.
                // e.g. mark item 3 of a,b,c -> a,b(,c) so that delete item 3 of a,b,c -> a,b
                else if (r_start > p_range . offset && !r_add)
                    r_start -= t_found_range . length;
            }
        }
            break;
        
        case CT_PARAGRAPH:
        {
            uindex_t t_pg_offset;
            bool t_pg_found, t_newline_found;
            
            // calculate the start of the (p_first)th paragraph
            while (p_first)
            {
                t_pg_offset = t_offset;
                t_newline_found = MCStringFirstIndexOfCharInRange(p_string, '\n', MCRangeMakeMinMax(t_offset, t_length), kMCCompareExact, t_offset);
                // AL-2014-07-21: [[ Bug 12162 ]] Ignore PS when calculating paragraph chunk.                
                t_pg_found = false; /*MCStringFirstIndexOfChar(p_string, 0x2029, t_pg_offset, kMCCompareExact, t_pg_offset);*/
                
                if (!t_newline_found && !t_pg_found)
                    break;
                t_offset = MCU_min(t_newline_found ? t_offset : UINDEX_MAX, t_pg_found ? t_pg_offset : UINDEX_MAX);
                p_first--;
                t_offset++;
            }
            
            // if we couldn't find enough delimiters, set r_add to the number of
            // additional delimiters required and set the offset to the end
            if (p_first > 0)
            {
                t_offset = t_length;
                r_add = p_first;
            }
                
            r_start = t_offset;
            
            // calculate the length of the next p_count paragraphs
            while (p_count--)
            {
                t_pg_offset = t_offset;
                // AL-2014-05-26: [[ Bug 12527 ]] Make sure both newline and pg char are found if both present
                if (t_offset <= t_end_index)
                {
                    t_newline_found = MCStringFirstIndexOfCharInRange(p_string, '\n', MCRangeMakeMinMax(t_offset, t_length), kMCCompareExact, t_offset);
                    t_pg_found = MCStringFirstIndexOfCharInRange(p_string, 0x2029, MCRangeMakeMinMax(t_pg_offset, t_length), kMCCompareExact, t_pg_offset);
                }
                else
                {
                    t_newline_found = false;
                    t_pg_found = false;
                }
                
                if (!t_newline_found && !t_pg_found)
                {
                    r_end = t_length;
                    break;
                }
                
                t_offset = MCU_min(t_newline_found ? t_offset : UINDEX_MAX, t_pg_found ? t_pg_offset : UINDEX_MAX);
                
                if (p_count == 0)
                    r_end = t_offset;
                else
                    t_offset++;
            }
            
            if (p_whole_chunk && !p_further_chunks)
            {
	            if (r_end >= 0 && (uindex_t) r_end < t_length)
                    r_end++;
                else if (r_start > p_range . offset && !r_add)
                    r_start--;
            }
        }
            break;

        case CT_SENTENCE:
        case CT_TRUEWORD:
        {
            MCAutoStringRef t_string;
            if (t_offset > 0 || t_length < MCStringGetLength(p_string))
                /* UNCHECKED */ MCStringCopySubstring(p_string, p_range, &t_string);
            else
                t_string = p_string;
            
            // Resolve the indices
            MCRange t_range, t_cu_range;
            t_range = MCRangeMake(p_first, p_count);
            if (p_chunk_type == CT_SENTENCE)
                /* UNCHECKED */ MCStringMapSentenceIndices(*t_string, kMCBasicLocale, t_range, t_cu_range);
            else
                /* UNCHECKED */ MCStringMapTrueWordIndices(*t_string, kMCBasicLocale, t_range, t_cu_range);
                
            r_start = t_offset + t_cu_range.offset;
            r_end = t_offset + t_cu_range.offset + t_cu_range.length;
        }
            break;
            
        case CT_WORD:
        {

            // if there are consecutive spaces at the beginning, skip them
            while (MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_string, t_offset)))
                t_offset++;
            
            // calculate the start of the (p_first)th word
            while (p_first-- && t_offset < t_length)
            {
                MCStringsSkipWord(ctxt, p_string, true, t_offset);
            }
            
            // AL-2015-03-05: [[ Bug 14812 ]] Make sure t_offset doesn't overrun t_length
            r_start = MCMin(t_offset, t_length);
            
            while (p_count-- && t_offset < t_length)
            {
                MCStringsSkipWord(ctxt, p_string, p_count != 0, t_offset);
            }
            
            // AL-2015-03-05: [[ Bug 14812 ]] Make sure t_offset doesn't overrun t_length
            r_end = MCMin(t_offset, t_length);
            
            if (p_whole_chunk && !p_further_chunks)
            {
	            while (r_end >= 0 && (uindex_t) r_end < t_length && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_string, r_end)))
                    r_end++;
                // AL-2014-09-29: [[ Bug 13550 ]] Only delete preceding whitespace if wholechunks is true
                //  and word chunk range goes to the end of the string. 
	            if (r_end >= 0 && (uindex_t) r_end == t_length)
                {
                    while (r_start > p_range . offset && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_string, r_start - 1)))
                        r_start--;
                }
                return;
            }
            
            // ignore whitespace at the end
            while (r_end > r_start && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_string, r_end - 1)))
                r_end--;
        }
            break;
            
        case CT_TOKEN:
        {
            MCAutoStringRef t_string;
            if (t_offset > 0 || t_length < MCStringGetLength(p_string))
            /* UNCHECKED */ MCStringCopySubstring(p_string, p_range, &t_string);
            else
                t_string = p_string;
            
            MCScriptPoint sp(*t_string);
            MCerrorlock++;
            
            Parse_stat ps = sp.nexttoken();
            
            while (p_first-- && ps != PS_ERROR && ps != PS_EOF)
                ps = sp.nexttoken();

            // AL-2015-05-01: [[ Bug 15309 ]] r_start and r_end are absolute indices, so they
            //  need to be corrected by the initial string offset.
            r_start = sp . getindex() + t_offset;
            while (--p_count && ps != PS_ERROR && ps != PS_EOF)
                ps = sp.nexttoken();
            
            r_end = sp . getindex() + MCStringGetLength(sp.gettoken_stringref()) + t_offset;
            MCerrorlock--;
        }
            break;
            
        case CT_CHARACTER:
        case CT_CODEPOINT:
            if (p_include_chars)
            {
                MCAutoStringRef t_string;
                if (t_offset > 0 || t_length < MCStringGetLength(p_string))
                /* UNCHECKED */ MCStringCopySubstring(p_string, p_range, &t_string);
                else
                    t_string = p_string;
                
                // Resolve the indices
                MCRange t_cp_range, t_cu_range;
                t_cp_range = MCRangeMake(p_first, p_count);
                MCStringMapIndices(*t_string, p_chunk_type == CT_CHARACTER ? kMCCharChunkTypeGrapheme : kMCCharChunkTypeCodepoint, t_cp_range, t_cu_range);
        
                r_start = t_offset + t_cu_range.offset;
                r_end = t_offset + t_cu_range.offset + t_cu_range.length;
            }
            break;
        case CT_CODEUNIT:
        case CT_BYTE:
            if (p_include_chars)
            {
                r_start = p_first + t_offset;
                r_end = p_first + t_offset + p_count;
            }
            break;
        default:
            MCAssert(false);
    }
    
    // Ensure the indices are in range
    MCAssert(r_start >= 0 && r_end >= 0);
    r_start = MCU_min(r_start, integer_t(t_length));
    r_end = MCU_min(r_end, integer_t(t_length));
}

void MCStringsMarkTextChunk(MCExecContext& ctxt, MCStringRef p_string, Chunk_term p_chunk_type, integer_t p_first, integer_t p_count, integer_t& r_start, integer_t& r_end, bool p_whole_chunk, bool p_further_chunks, bool p_include_chars, integer_t& r_add)
{
    MCStringsMarkTextChunkInRange(ctxt, p_string, MCRangeMake(0, MCStringGetLength(p_string)), p_chunk_type, p_first, p_count, r_start, r_end, p_whole_chunk, p_further_chunks, p_include_chars, r_add);
}

void MCStringsGetTextChunk(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, integer_t p_count, bool p_eval_mutable, MCStringRef& r_result)
{
    // if there are no chunks then the string should be empty.
    if (p_count == 0)
    {
        r_result = MCValueRetain(kMCEmptyString);
        return;
    }
    
    // otherwise, calculate the substring.
    integer_t t_start;
    integer_t t_end;
    integer_t t_add;
    MCStringsMarkTextChunk(ctxt, p_source, p_chunk_type, p_first, p_count, t_start, t_end, false, false, true, t_add);
    
    if (!p_eval_mutable)
    {
        MCStringCopySubstring(p_source, MCRangeMakeMinMax(t_start, t_end), r_result);
    }
    else
    {
        MCStringMutableCopySubstring(p_source, MCRangeMakeMinMax(t_start, t_end), r_result);
    }
}

void MCStringsSetTextChunk(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, integer_t p_first, integer_t p_count, MCStringRef& x_target)
{
    integer_t t_start;
    integer_t t_end;
    integer_t t_add;
    
    MCStringsMarkTextChunk(ctxt, x_target, p_chunk_type, p_first, p_count, t_start, t_end, false, false, true, t_add);
    
    if (t_add && (p_chunk_type == CT_ITEM || p_chunk_type == CT_LINE))
    {
        MCStringRef t_delimiter;
        t_delimiter = p_chunk_type == CT_LINE ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter();
        while (t_add--)
        {
            MCStringPrepend(x_target, t_delimiter);
        }
    }
    
    switch (p_type)
    {
        case PT_BEFORE:
            MCStringInsert(x_target, t_start, p_source);
            break;
        case PT_INTO:
            MCStringReplace(x_target, MCRangeMakeMinMax(t_start, t_end), p_source);
            break;
        case PT_AFTER:
            MCStringInsert(x_target, t_end, p_source);
            break;
        default:
            fprintf(stderr, "MCChunk: ERROR bad prep in gets\n");
            break;
    }
    
}

void MCStringsEvalTextChunkByRange(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, bool p_eval_mutable, MCStringRef& r_result)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByRangeInRange(ctxt, p_chunk_type, p_first, p_last, p_source, nil, t_first, t_chunk_count);
    
    MCStringsGetTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, p_eval_mutable, r_result);
}

void MCStringsEvalTextChunkByExpression(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, bool p_eval_mutable, MCStringRef& r_result)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByExpressionInRange(ctxt, p_chunk_type, p_first, p_source, nil, t_first, t_chunk_count);
    
    MCStringsGetTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, p_eval_mutable, r_result);
}

void MCStringsEvalTextChunkByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, bool p_eval_mutable, MCStringRef& r_result)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByOrdinalInRange(ctxt, p_chunk_type, p_ordinal_type, p_source, nil, t_first, t_chunk_count);
    
    MCStringsGetTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, p_eval_mutable, r_result);
}

void MCStringsSetTextChunkByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, MCStringRef& x_target)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByRangeInRange(ctxt, p_chunk_type, p_first, p_last, x_target, nil, t_first, t_chunk_count);
    MCStringsSetTextChunk(ctxt, p_source, p_type, p_chunk_type, t_first, t_chunk_count, x_target);
}

void MCStringsSetTextChunkByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, integer_t p_first, MCStringRef& x_target)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByExpressionInRange(ctxt, p_chunk_type, p_first, x_target, nil, t_first, t_chunk_count);
    MCStringsSetTextChunk(ctxt, p_source, p_type, p_chunk_type, t_first, t_chunk_count, x_target);
}

void MCStringsSetTextChunkByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, MCStringRef& x_target)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByOrdinalInRange(ctxt, p_chunk_type, p_ordinal_type, x_target, nil, t_first, t_chunk_count);
    MCStringsSetTextChunk(ctxt, p_source, p_type, p_chunk_type, t_first, t_chunk_count, x_target);
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalMutableLinesOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByRange(ctxt, p_source, CT_LINE, p_first, p_last, true, r_result);
}

void MCStringsEvalMutableLinesOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_line, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByExpression(ctxt, p_source, CT_LINE, p_line, true, r_result);
}

void MCStringsEvalMutableLinesOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByOrdinal(ctxt, p_source, CT_LINE, p_ordinal_type, true, r_result);
}

void MCStringsEvalMutableItemsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByRange(ctxt, p_source, CT_ITEM, p_first, p_last, true, r_result);
}

void MCStringsEvalMutableItemsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_item, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByExpression(ctxt, p_source, CT_ITEM, p_item, true, r_result);
}

void MCStringsEvalMutableItemsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByOrdinal(ctxt, p_source, CT_ITEM, p_ordinal_type, true, r_result);
}

void MCStringsEvalMutableWordsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByRange(ctxt, p_source, CT_WORD, p_first, p_last, true, r_result);
}

void MCStringsEvalMutableWordsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_word, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByExpression(ctxt, p_source, CT_WORD, p_word, true, r_result);
}

void MCStringsEvalMutableWordsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByOrdinal(ctxt, p_source, CT_WORD, p_ordinal_type, true, r_result);
}

void MCStringsEvalMutableTokensOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByRange(ctxt, p_source, CT_TOKEN, p_first, p_last, true, r_result);
}

void MCStringsEvalMutableTokensOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_token, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByExpression(ctxt, p_source, CT_TOKEN, p_token, true, r_result);
}

void MCStringsEvalMutableTokensOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByOrdinal(ctxt, p_source, CT_TOKEN, p_ordinal_type, true, r_result);
}

void MCStringsExecSetLinesOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result)
{
    MCStringsSetTextChunkByRange(ctxt, p_source, p_type, CT_LINE, p_first, p_last, r_result);
}

void MCStringsExecSetLinesOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_line, MCStringRef& r_result)
{
    MCStringsSetTextChunkByExpression(ctxt, p_source, p_type, CT_LINE, p_line, r_result);
}

void MCStringsExecSetLinesOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result)
{
    MCStringsSetTextChunkByOrdinal(ctxt, p_source, p_type, CT_LINE, p_ordinal_type, r_result);
}

void MCStringsExecSetItemsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result)
{
    MCStringsSetTextChunkByRange(ctxt, p_source, p_type, CT_ITEM, p_first, p_last, r_result);
}

void MCStringsExecSetItemsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_item, MCStringRef& r_result)
{
    MCStringsSetTextChunkByExpression(ctxt, p_source, p_type, CT_ITEM, p_item, r_result);
}

void MCStringsExecSetItemsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result)
{
    MCStringsSetTextChunkByOrdinal(ctxt, p_source, p_type, CT_ITEM, p_ordinal_type, r_result);
}

void MCStringsExecSetWordsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result)
{
    MCStringsSetTextChunkByRange(ctxt, p_source, p_type, CT_WORD, p_first, p_last, r_result);
}

void MCStringsExecSetWordsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_word, MCStringRef& r_result)
{
    MCStringsSetTextChunkByExpression(ctxt, p_source, p_type, CT_WORD, p_word, r_result);
}

void MCStringsExecSetWordsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result)
{
    MCStringsSetTextChunkByOrdinal(ctxt, p_source, p_type, CT_WORD, p_ordinal_type, r_result);
}

void MCStringsExecSetTokensOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result)
{
    MCStringsSetTextChunkByRange(ctxt, p_source, p_type, CT_TOKEN, p_first, p_last, r_result);
}

void MCStringsExecSetTokensOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_token, MCStringRef& r_result)
{
    MCStringsSetTextChunkByExpression(ctxt, p_source, p_type, CT_TOKEN, p_token, r_result);
}

void MCStringsExecSetTokensOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result)
{
    MCStringsSetTextChunkByOrdinal(ctxt, p_source, p_type, CT_TOKEN, p_ordinal_type, r_result);
}

void MCStringsExecSetCharsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result)
{
    MCStringsSetTextChunkByRange(ctxt, p_source, p_type, CT_CHARACTER, p_first, p_last, r_result);
}

void MCStringsExecSetCharsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_char, MCStringRef& r_result)
{
    MCStringsSetTextChunkByExpression(ctxt, p_source, p_type, CT_CHARACTER, p_char, r_result);
}

void MCStringsExecSetCharsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result)
{
    MCStringsSetTextChunkByOrdinal(ctxt, p_source, p_type, CT_CHARACTER, p_ordinal_type, r_result);
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsGetTextChunk(MCExecContext& ctxt, MCStringRef p_source, integer_t p_start, integer_t p_end, MCStringRef& r_result)
{
    MCStringCopySubstring(p_source, MCRangeMakeMinMax(p_start, p_end), r_result);
}

void MCStringsEvalTextChunkByRange(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByRangeInRange(ctxt, p_chunk_type, p_first, p_last, p_source, nil, t_first, t_chunk_count);
    
    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, t_start, t_end, false, false, true, t_add);
    MCStringsGetTextChunk(ctxt, p_source, t_start, t_end, r_result);

    x_end = x_start + t_end;
    x_start += t_start;
}

void MCStringsEvalTextChunkByExpression(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByExpressionInRange(ctxt, p_chunk_type, p_first, p_source, nil, t_first, t_chunk_count);
    
    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, t_start, t_end, false, false, true, t_add);
    MCStringsGetTextChunk(ctxt, p_source, t_start, t_end, r_result);
    
    x_end = x_start + t_end;
    x_start += t_start;
}

void MCStringsEvalTextChunkByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByOrdinalInRange(ctxt, p_chunk_type, p_ordinal_type, p_source, nil, t_first, t_chunk_count);
    
    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, t_start, t_end, false, false, true, t_add);
    MCStringsGetTextChunk(ctxt, p_source, t_start, t_end, r_result);
    
    x_end = x_start + t_end;
    x_start += t_start;
}

void MCStringsEvalLinesOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByRange(ctxt, p_source, CT_LINE, p_first, p_last, x_start, x_end, r_result);
}

void MCStringsEvalLinesOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_line, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByExpression(ctxt, p_source, CT_LINE, p_line, x_start, x_end, r_result);
}

void MCStringsEvalLinesOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByOrdinal(ctxt, p_source, CT_LINE, p_ordinal_type, x_start, x_end, r_result);
}

void MCStringsEvalItemsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByRange(ctxt, p_source, CT_ITEM, p_first, p_last, x_start, x_end, r_result);
}

void MCStringsEvalItemsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_item, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByExpression(ctxt, p_source, CT_ITEM, p_item, x_start, x_end, r_result);
}

void MCStringsEvalItemsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByOrdinal(ctxt, p_source, CT_ITEM, p_ordinal_type, x_start, x_end, r_result);
}

void MCStringsEvalWordsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByRange(ctxt, p_source, CT_WORD, p_first, p_last, x_start, x_end, r_result);
}

void MCStringsEvalWordsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_word, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByExpression(ctxt, p_source, CT_WORD, p_word, x_start, x_end, r_result);
}

void MCStringsEvalWordsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByOrdinal(ctxt, p_source, CT_WORD, p_ordinal_type, x_start, x_end, r_result);
}

void MCStringsEvalTokensOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByRange(ctxt, p_source, CT_TOKEN, p_first, p_last, x_start, x_end, r_result);
}

void MCStringsEvalTokensOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_token, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByExpression(ctxt, p_source, CT_TOKEN, p_token, x_start, x_end, r_result);
}

void MCStringsEvalTokensOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByOrdinal(ctxt, p_source, CT_TOKEN, p_ordinal_type, x_start, x_end, r_result);
}

void MCStringsEvalCharsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByRange(ctxt, p_source, CT_CHARACTER, p_first, p_last, x_start, x_end, r_result);
}

void MCStringsEvalCharsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_char, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByExpression(ctxt, p_source, CT_CHARACTER, p_char, x_start, x_end, r_result);
}

void MCStringsEvalCharsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    MCStringsEvalTextChunkByOrdinal(ctxt, p_source, CT_CHARACTER, p_ordinal_type, x_start, x_end, r_result);
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsAddChunks(MCExecContext& ctxt, Chunk_term p_chunk_type, uindex_t p_to_add, MCMarkedText& x_text)
{
    if ((p_chunk_type != CT_ITEM && p_chunk_type != CT_LINE) || !p_to_add)
        return;

    MCStringRef t_delimiter;
    MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringMutableCopyAndRelease((MCStringRef)x_text . text, &t_string);
    t_delimiter = p_chunk_type == CT_LINE ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter();
    uindex_t t_count = p_to_add;
    
    while (t_count--)
        /* UNCHECKED */ MCStringInsert(*t_string, x_text . finish, t_delimiter);
    
    /* UNCHECKED */ MCStringCopy(*t_string, (MCStringRef&)x_text . text);
    
    // SN-2014-09-03: [[ Bug 13314 ]] The line delimiter might be more than 1 character-long
    x_text . start += MCStringGetLength(t_delimiter) * p_to_add;
    x_text . finish += MCStringGetLength(t_delimiter) * p_to_add;
    
    // the text has changed
    // SN-2014-09-03: [[ Bug 13314 ]] MCMarkedText::changed updated to store the number of chars appended
    // SN-2015-05-05: [[ Bug 15315 ]] put "hello" into item 2 of line 4 of ...
    //  will add twice chunk delimiters, and we want to keep the count
    //  (see note for bug 15315 in MCInterfaceExecPutIntoField).
    x_text . changed += p_to_add * MCStringGetLength(t_delimiter);
}

void MCStringsEvalTextChunk(MCExecContext& ctxt, MCMarkedText p_source, MCStringRef& r_string)
{
    if (p_source . text == nil)
        return;

    // The incoming indices are for codeunits
    MCRange t_cu_range;
    t_cu_range = MCRangeMakeMinMax(p_source . start, p_source . finish);
    
    if (MCStringCopySubstring((MCStringRef)p_source . text, t_cu_range, r_string))
        return;
    
    ctxt . Throw();
}

void MCStringsEvalByteChunk(MCExecContext& ctxt, MCMarkedText p_source, MCDataRef& r_bytes)
{
    // If the text is not a data ref at this point, then something has gone wrong.
    MCAssert(MCValueGetTypeCode(p_source . text) == kMCValueTypeCodeData);

    // MW-2014-04-11: [[ Bug 12179 ]] Use a subrange copy - previously clamping wasn't being
    //   performed.
    if (MCDataCopyRange((MCDataRef)p_source . text, MCRangeMakeMinMax(p_source . start, p_source . finish), r_bytes))
        return;
    
    ctxt . Throw();
}

void MCStringsMarkTextChunkByRange(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    // The incoming indices are for codeunits
    MCRange t_cu_range;
    t_cu_range = MCRangeMakeMinMax(x_mark . start, x_mark . finish);

    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByRangeInRange(ctxt, p_chunk_type, p_first, p_last, x_mark . text, &t_cu_range, t_first, t_chunk_count);

    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunkInRange(ctxt, (MCStringRef)x_mark . text, t_cu_range, p_chunk_type, t_first, t_chunk_count, t_start, t_end, p_whole_chunk, p_further_chunks, true, t_add);
    
    x_mark . start = t_start;
    x_mark . finish = t_end;

    if (p_force)
        MCStringsAddChunks(ctxt, p_chunk_type, t_add, x_mark);
}

void MCStringsMarkTextChunkByOrdinal(MCExecContext& ctxt, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    // The incoming indices are for codeunits
    MCRange t_cu_range;
    t_cu_range = MCRangeMakeMinMax(x_mark . start, x_mark . finish);
    
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByOrdinalInRange(ctxt, p_chunk_type, p_ordinal_type, x_mark . text, &t_cu_range, t_first, t_chunk_count);
    
    // SN-2014-12-15: [[ Bug 14211 ]] MCStringsGetExtensByOrdinal may throw an error.
    // The release of x_mark.text will be done in MCChunk::evalobjectchunk
    if (ctxt . HasError())
        return;
    
    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunkInRange(ctxt, (MCStringRef)x_mark . text, t_cu_range, p_chunk_type, t_first, t_chunk_count, t_start, t_end, p_whole_chunk, p_further_chunks, true, t_add);
    

    // AL-2015-02-25: [[ Bug 14532 ]] Set the mark to the correct indices -
    //  those returned from MarkTextChunkInRange are absolute.
    x_mark . start = t_start;
    x_mark . finish = t_end;

    if (p_force)
        MCStringsAddChunks(ctxt, p_chunk_type, t_add, x_mark);
}

void MCStringsMarkLinesOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_LINE, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkLinesOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_LINE, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkParagraphsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_PARAGRAPH, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkParagraphsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_PARAGRAPH, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkSentencesOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_SENTENCE, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkSentencesOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_SENTENCE, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkItemsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_ITEM, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkItemsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_ITEM, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkTrueWordsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_TRUEWORD, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkTrueWordsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_TRUEWORD, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkWordsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_WORD, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkWordsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_WORD, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkTokensOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_TOKEN, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkTokensOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_TOKEN, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkCharsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_CHARACTER, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkCharsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_CHARACTER, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkCodepointsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_CODEPOINT, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkCodepointsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_CODEPOINT, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkCodeunitsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_CODEUNIT, p_first, p_last, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkCodeunitsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_CODEUNIT, p_ordinal_type, p_force, p_whole_chunk, p_further_chunks, x_mark);
}

void MCStringsMarkBytesOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, MCMarkedText& x_mark)
{
    // The incoming indices are for codeunits
    MCRange t_cu_range;
    t_cu_range = MCRangeMakeMinMax(x_mark . start, x_mark . finish);
    
    // AL-2014-09-10: [[ Bug 13400 ]] Keep marked strings the correct type where possible
    if (MCValueGetTypeCode(x_mark . text) != kMCValueTypeCodeData)
    {
        // Convert to data if it is not already data
        MCAutoDataRef t_data;
        ctxt . ConvertToData((MCStringRef)x_mark . text, &t_data);
        
        MCValueRelease(x_mark . text);
        x_mark . text = MCValueRetain(*t_data);
    }
    
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByRangeInRange(ctxt, CT_BYTE, p_first, p_last, x_mark . text, &t_cu_range, t_first, t_chunk_count);
    
    // adjust the byte indices
    x_mark . start = t_cu_range . offset + t_first;
    x_mark . finish = x_mark . start + t_chunk_count;
}

void MCStringsMarkBytesOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, MCMarkedText& x_mark)
{
    // The incoming indices are for codeunits
    MCRange t_cu_range;
    t_cu_range = MCRangeMakeMinMax(x_mark . start, x_mark . finish);
    
    // AL-2014-09-10: [[ Bug 13400 ]] Keep marked strings the correct type where possible
    if (MCValueGetTypeCode(x_mark . text) != kMCValueTypeCodeData)
    {
        MCAutoDataRef t_data;
        ctxt . ConvertToData((MCStringRef)x_mark . text, &t_data);
        
        MCValueRelease(x_mark . text);
        x_mark . text = MCValueRetain(*t_data);
    }
    
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByOrdinalInRange(ctxt, CT_BYTE, p_ordinal_type, x_mark . text, &t_cu_range, t_first, t_chunk_count);
    
    // adjust the byte indices
    x_mark . start = t_cu_range . offset + t_first;
    x_mark . finish = x_mark . start + t_chunk_count;
}

////////////////////////////////////////////////////////////////////////////////

MCTextChunkIterator_Tokenized::MCTextChunkIterator_Tokenized(MCStringRef p_text, MCChunkType p_chunk_type) : MCTextChunkIterator(p_text, p_chunk_type)
{
    m_sp = new (nothrow) MCScriptPoint(p_text);
}

MCTextChunkIterator_Tokenized::MCTextChunkIterator_Tokenized(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction) : MCTextChunkIterator(p_text, p_chunk_type, p_restriction)
{
    MCAutoStringRef t_substring;
    MCStringCopySubstring(m_text, p_restriction, &t_substring);
    MCValueAssign(m_text, *t_substring);
    m_sp = new (nothrow) MCScriptPoint(m_text);
}

MCTextChunkIterator_Tokenized::~MCTextChunkIterator_Tokenized()
{
    delete m_sp;
}

bool MCTextChunkIterator_Tokenized::Next()
{
    MCerrorlock++;
    
    bool t_found = true;
    Parse_stat ps = m_sp -> nexttoken();
    if (ps == PS_ERROR || ps == PS_EOF)
        t_found = false;
    
    MCerrorlock--;
    
    if (t_found)
    {
        m_range . offset = m_sp -> getindex();
        m_range . length = MCStringGetLength(m_sp -> gettoken_stringref());
    }
    
    return t_found;
}


MCTextChunkIterator *MCStringsTextChunkIteratorCreate(MCExecContext& ctxt, MCStringRef p_text, Chunk_term p_chunk_type)
{
    if (p_chunk_type == CT_TOKEN)
    {
        return new (nothrow) MCTextChunkIterator_Tokenized(p_text, MCChunkTypeFromChunkTerm(p_chunk_type));
    }
    
    return MCChunkCreateTextChunkIterator(p_text, nil, MCChunkTypeFromChunkTerm(p_chunk_type), ctxt . GetLineDelimiter(), ctxt . GetItemDelimiter(), ctxt . GetStringComparisonType());
}

MCTextChunkIterator *MCStringsTextChunkIteratorCreateWithRange(MCExecContext& ctxt, MCStringRef p_text, MCRange p_range, Chunk_term p_chunk_type)
{
    if (p_chunk_type == CT_TOKEN)
    {
        return new (nothrow) MCTextChunkIterator_Tokenized(p_text, MCChunkTypeFromChunkTerm(p_chunk_type), p_range);
    }
    
    return MCChunkCreateTextChunkIterator(p_text, &p_range, MCChunkTypeFromChunkTerm(p_chunk_type), ctxt . GetLineDelimiter(), ctxt . GetItemDelimiter(), ctxt . GetStringComparisonType());
}

bool MCStringsTextChunkIteratorNext(MCExecContext& ctxt, MCTextChunkIterator *tci)
{
    tci -> SetOptions(ctxt . GetStringComparisonType());
    
    MCChunkType t_type;
    t_type = tci -> GetType();
    
    if (t_type == kMCChunkTypeLine || t_type == kMCChunkTypeItem)
    {
        MCStringRef t_delimiter;
        t_delimiter = t_type == kMCChunkTypeLine ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter();
        reinterpret_cast<MCTextChunkIterator_Delimited *>(tci) -> SetDelimiter(t_delimiter);
    }
    
    return tci -> Next();
}

////////////////////////////////////////////////////////////////////////////////
