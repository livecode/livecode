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

MC_EXEC_DEFINE_EVAL_METHOD(Strings, LinesOfTextByRange, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, LinesOfTextByExpression, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, LinesOfTextByOrdinal, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ItemsOfTextByRange, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ItemsOfTextByExpression, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ItemsOfTextByOrdinal, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, WordsOfTextByRange, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, WordsOfTextByExpression, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, WordsOfTextByOrdinal, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, TokensOfTextByRange, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, TokensOfTextByExpression, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, TokensOfTextByOrdinal, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, CharsOfTextByRange, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, CharsOfTextByExpression, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, CharsOfTextByOrdinal, 3)

////////////////////////////////////////////////////////////////////////////////

struct MCChunkCountState
{
    MCStringRef string;
    Chunk_term chunk;
    MCExecContext *ctxt;
};

uinteger_t MCStringsCountChunkCallback(void *context)
{
    MCChunkCountState *t_state = static_cast<MCChunkCountState *>(context);
    uinteger_t t_count;
    MCStringsCountChunks(*t_state -> ctxt, t_state -> chunk, t_state -> string, t_count);
    return t_count;
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsSkipWord(MCExecContext& ctxt, MCStringRef p_string, bool p_skip_spaces, uindex_t& x_offset)
{
    uindex_t t_space_offset;
    uindex_t t_length = MCStringGetLength(p_string);
    uindex_t t_end_quote_offset = t_length;
    uindex_t t_end_line_offset = t_length;
    
    if (MCStringGetCharAtIndex(p_string, x_offset) == '"')
    {
        // then bump the offset up to the next quotation mark + 1, or the beginning of the next line
        // if neither of these are present then set offset to string length.
        MCStringFirstIndexOfChar(p_string, '"', x_offset + 1, kMCCompareExact, t_end_quote_offset);
        MCStringFirstIndexOf(p_string, ctxt . GetLineDelimiter(), x_offset + 1, kMCCompareExact, t_end_line_offset);
        
        if (t_end_quote_offset < t_end_line_offset)
            x_offset = t_end_quote_offset + 1;
        else if (t_end_line_offset < t_end_quote_offset)
            x_offset = t_end_line_offset + MCStringGetLength(ctxt . GetLineDelimiter());
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

void MCStringsCountChunks(MCExecContext& ctxt, Chunk_term p_chunk_type, MCStringRef p_string, uinteger_t& r_count)
{
    if (MCStringGetLength(p_string) == 0)
    {
        r_count = 0;
        return;
    }
    
    // When the string doesn't contain combining characters or surrogate pairs, we can shortcut.
    if ((p_chunk_type == CT_CHARACTER || p_chunk_type == CT_CODEPOINT))
        if (MCStringIsNative(p_string) || (MCStringIsUncombined(p_string) && MCStringIsSimple(p_string)))
            p_chunk_type = CT_CODEUNIT;
    
    if (p_chunk_type == CT_CODEUNIT)
    {
        r_count = MCStringGetLength(p_string);
        return;
    }
    
    MCTextChunkIterator *tci;
    tci = new MCTextChunkIterator(p_chunk_type, p_string);
    r_count = tci -> countchunks(ctxt);
    delete tci;
    return;
 }

void MCStringsGetExtentsByOrdinal(MCExecContext& ctxt, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, MCValueRef p_string, integer_t& r_first, integer_t& r_chunk_count)
{
    uinteger_t t_count = 0;
    switch (p_ordinal_type)
    {
        case CT_ANY:
        case CT_LAST:
        case CT_MIDDLE:
            if (MCValueGetTypeCode(p_string) == kMCValueTypeCodeData)
                t_count = MCDataGetLength((MCDataRef)p_string);
            else
                MCStringsCountChunks(ctxt, p_chunk_type, (MCStringRef)p_string, t_count);
            
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
            fprintf(stderr, "MCChunk: ERROR bad extents\n");
            abort();
	}
    
    if (r_first < 0)
    {
        r_chunk_count = 0;
        r_first = 0;
        return;
    }
    else
        r_chunk_count = 1;
}

void MCStringsGetExtentsByRange(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, MCValueRef p_string, uinteger_t& r_first, uinteger_t& r_chunk_count)
{
    if (MCValueGetTypeCode(p_string) == kMCValueTypeCodeData)
        MCChunkGetExtentsOfByteChunkByRange((MCDataRef)p_string, p_first, p_last, r_first, r_chunk_count);
    else if (p_chunk_type == CT_CODEUNIT)
        MCChunkGetExtentsOfCodeunitChunkByRange((MCStringRef)p_string, p_first, p_last, r_first, r_chunk_count);
    else
    {
        MCChunkCountState t_state;
        t_state . string = (MCStringRef)p_string;
        t_state . chunk = p_chunk_type;
        t_state . ctxt = &ctxt;
        MCChunkGetExtentsByRange(p_first, p_last, MCStringsCountChunkCallback, &t_state, r_first, r_chunk_count);
    }
}

void MCStringsGetExtentsByExpression(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, MCValueRef p_string, uinteger_t& r_first, uinteger_t& r_chunk_count)
{
    if (MCValueGetTypeCode(p_string) == kMCValueTypeCodeData)
        MCChunkGetExtentsOfByteChunkByExpression((MCDataRef)p_string, p_first, r_first, r_chunk_count);
    else if (p_chunk_type == CT_CODEUNIT)
        MCChunkGetExtentsOfCodeunitChunkByExpression((MCStringRef)p_string, p_first, r_first, r_chunk_count);
    else
    {
        MCChunkCountState t_state;
        t_state . string = (MCStringRef)p_string;
        t_state . chunk = p_chunk_type;
        t_state . ctxt = &ctxt;
        MCChunkGetExtentsByExpression(p_first, MCStringsCountChunkCallback, &t_state, r_first, r_chunk_count);
    }
}

void MCStringsMarkTextChunk(MCExecContext& ctxt, MCStringRef p_string, Chunk_term p_chunk_type, integer_t p_first, integer_t p_count, integer_t& r_start, integer_t& r_end, bool p_whole_chunk, bool p_further_chunks, bool p_include_chars, integer_t& r_add)
{
    r_add = 0;
    if (p_count == 0 && p_chunk_type != CT_CHARACTER && p_chunk_type != CT_WORD)
    {
        r_start = 0;
        r_end = 0;
        return;
    }
    
    uindex_t t_length = MCStringGetLength(p_string);
    
    if (t_length == 0)
    {
        r_start = 0;
        r_end = 0;
        r_add = p_first;
        return;
    }
    
    uindex_t t_end_index = t_length - 1;
    uindex_t t_offset = 0;
    
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
            while (p_first && MCStringFind(p_string, MCRangeMake(t_offset, UINDEX_MAX), t_delimiter, ctxt . GetStringComparisonType(), &t_found_range))
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
                if (t_offset > t_end_index || !MCStringFind(p_string, MCRangeMake(t_offset, UINDEX_MAX), t_delimiter, ctxt . GetStringComparisonType(), &t_found_range))
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
                
                if (r_end < t_length)
                    r_end += t_found_range . length;
                // If we didn't, and this operation does not force additional delimiters, then include the previous delimiter in the mark.
                // e.g. mark item 3 of a,b,c -> a,b(,c) so that delete item 3 of a,b,c -> a,b
                else if (r_start > 0 && !r_add)
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
                t_newline_found = MCStringFirstIndexOfChar(p_string, '\n', t_offset, kMCCompareExact, t_offset);
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
                    t_newline_found = MCStringFirstIndexOfChar(p_string, '\n', t_offset, kMCCompareExact, t_offset);
                    t_pg_found = MCStringFirstIndexOfChar(p_string, 0x2029, t_pg_offset, kMCCompareExact, t_pg_offset);
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
                if (r_end < t_length)
                    r_end++;
                else if (r_start > 0 && !r_add)
                    r_start--;
            }
        }
            break;

        case CT_SENTENCE:
        case CT_TRUEWORD:
        {
            // Resolve the indices
            MCRange t_range, t_cu_range;
            t_range = MCRangeMake(p_first, p_count);
            if (p_chunk_type == CT_SENTENCE)
                /* UNCHECKED */ MCStringMapSentenceIndices(p_string, kMCBasicLocale, t_range, t_cu_range);
            else
                /* UNCHECKED */ MCStringMapTrueWordIndices(p_string, kMCBasicLocale, t_range, t_cu_range);
                
            r_start = t_cu_range.offset;
            r_end = t_cu_range.offset + t_cu_range.length;
            //r_start = p_first;
            //r_end = p_first + p_count;
        }
            break;
            
        case CT_WORD:
        {
            uindex_t t_space_offset;
            
            // if there are consecutive spaces at the beginning, skip them
            while (MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_string, t_offset)))
                t_offset++;
            
            // calculate the start of the (p_first)th word
            while (p_first-- && t_offset < t_length)
            {
                MCStringsSkipWord(ctxt, p_string, true, t_offset);
            }
            
            r_start = t_offset;
            
            while (p_count-- && t_offset < t_length)
            {
                MCStringsSkipWord(ctxt, p_string, p_count != 0, t_offset);
            }
            
            r_end = t_offset;
            
            if (p_whole_chunk && !p_further_chunks)
            {
                while (r_end < t_length && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_string, r_end)))
                    r_end++;
                // AL-2014-09-29: [[ Bug 13550 ]] Only delete preceding whitespace if wholechunks is true
                //  and word chunk range goes to the end of the string. 
                if (r_end == t_length)
                {
                    while (r_start > 0 && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_string, r_start - 1)))
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
            MCScriptPoint sp(p_string);
            MCerrorlock++;
            
            uint2 t_pos;
            Parse_stat ps = sp.nexttoken();
            
            while (p_first-- && ps != PS_ERROR && ps != PS_EOF)
                ps = sp.nexttoken();

            r_start = sp . getindex();
            while (--p_count && ps != PS_ERROR && ps != PS_EOF)
                ps = sp.nexttoken();
            
            r_end = sp . getindex() + MCStringGetLength(sp.gettoken_stringref());
            MCerrorlock--;
        }
            break;
            
        case CT_CHARACTER:
        case CT_CODEPOINT:
            if (p_include_chars)
            {
                // Resolve the indices
                MCRange t_cp_range, t_cu_range;
                t_cp_range = MCRangeMake(p_first, p_count);
                MCStringMapIndices(p_string, p_chunk_type == CT_CHARACTER ? kMCCharChunkTypeGrapheme : kMCCharChunkTypeCodepoint, t_cp_range, t_cu_range);
        
                r_start = t_cu_range.offset;
                r_end = t_cu_range.offset + t_cu_range.length;
                //r_start = p_first;
                //r_end = p_first + p_count;
            }
            break;
        case CT_CODEUNIT:
        case CT_BYTE:
            if (p_include_chars)
            {
                r_start = p_first;
                r_end = p_first + p_count;
            }
            break;
        default:
            MCAssert(false);
    }
    
    // SN-2014-04-07 [[ CombiningChars ]] The indices are already returned in codeunit, not codepoints
    
    // for line, paragraph, item, word and token, start and end are codepoint indices, so map them back to codeunits.
//    MCRange t_cp_range, t_cu_range;
//    t_cp_range = MCRangeMake(r_start, r_end - r_start);
//    MCStringMapIndices(p_string, kMCCharChunkTypeCodepoint, t_cp_range, t_cu_range);
//    r_start = t_cu_range . offset;
//    r_end = t_cu_range . offset + t_cu_range . length;
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
        MCStringCopySubstring(p_source, MCRangeMake(t_start, t_end - t_start), r_result);
    }
    else
    {
        MCStringMutableCopySubstring(p_source, MCRangeMake(t_start, t_end - t_start), r_result);
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
            MCStringReplace(x_target, MCRangeMake(t_start, t_end - t_start), p_source);
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
    MCStringsGetExtentsByRange(ctxt, p_chunk_type, p_first, p_last, p_source, t_first, t_chunk_count);
    
    MCStringsGetTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, p_eval_mutable, r_result);
}

void MCStringsEvalTextChunkByExpression(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, bool p_eval_mutable, MCStringRef& r_result)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByExpression(ctxt, p_chunk_type, p_first, p_source, t_first, t_chunk_count);
    
    MCStringsGetTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, p_eval_mutable, r_result);
}

void MCStringsEvalTextChunkByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, bool p_eval_mutable, MCStringRef& r_result)
{
    int4 t_first;
    int4 t_chunk_count;
    MCStringsGetExtentsByOrdinal(ctxt, p_chunk_type, p_ordinal_type, p_source, t_first, t_chunk_count);
    
    MCStringsGetTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, p_eval_mutable, r_result);
}

void MCStringsSetTextChunkByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, MCStringRef& x_target)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByRange(ctxt, p_chunk_type, p_first, p_last, x_target, t_first, t_chunk_count);
    MCStringsSetTextChunk(ctxt, p_source, p_type, p_chunk_type, t_first, t_chunk_count, x_target);
}

void MCStringsSetTextChunkByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, integer_t p_first, MCStringRef& x_target)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByExpression(ctxt, p_chunk_type, p_first, x_target, t_first, t_chunk_count);
    MCStringsSetTextChunk(ctxt, p_source, p_type, p_chunk_type, t_first, t_chunk_count, x_target);
}

void MCStringsSetTextChunkByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, MCStringRef& x_target)
{
    int4 t_first;
    int4 t_chunk_count;
    MCStringsGetExtentsByOrdinal(ctxt, p_chunk_type, p_ordinal_type, x_target, t_first, t_chunk_count);
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
    MCStringCopySubstring(p_source, MCRangeMake(p_start, p_end - p_start), r_result);
}

void MCStringsEvalTextChunkByRange(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByRange(ctxt, p_chunk_type, p_first, p_last, p_source, t_first, t_chunk_count);
    
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
    MCStringsGetExtentsByExpression(ctxt, p_chunk_type, p_first, p_source, t_first, t_chunk_count);
    
    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, t_start, t_end, false, false, true, t_add);
    MCStringsGetTextChunk(ctxt, p_source, t_start, t_end, r_result);
    
    x_end = x_start + t_end;
    x_start += t_start;
}

void MCStringsEvalTextChunkByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result)
{
    int4 t_first;
    int4 t_chunk_count;
    MCStringsGetExtentsByOrdinal(ctxt, p_chunk_type, p_ordinal_type, p_source, t_first, t_chunk_count);
    
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
    x_text . changed = p_to_add * MCStringGetLength(t_delimiter);
}

void MCStringsEvalTextChunk(MCExecContext& ctxt, MCMarkedText p_source, MCStringRef& r_string)
{
    if (p_source . text == nil)
        return;

    // The incoming indices are for codeunits
    MCRange t_cu_range;
    t_cu_range = MCRangeMake(p_source . start, p_source . finish - p_source . start);
    
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
    if (MCDataCopyRange((MCDataRef)p_source . text, MCRangeMake(p_source . start, p_source . finish - p_source. start), r_bytes))
        return;
    
    ctxt . Throw();
}

void MCStringsMarkTextChunkByRange(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    // The incoming indices are for codeunits
    MCRange t_cu_range;
    t_cu_range = MCRangeMake(x_mark . start, x_mark . finish - x_mark . start);
    
    MCAutoStringRef t_string;
    MCStringCopySubstring((MCStringRef)x_mark . text, t_cu_range, &t_string);
    
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByRange(ctxt, p_chunk_type, p_first, p_last, *t_string, t_first, t_chunk_count);
    
    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunk(ctxt, *t_string, p_chunk_type, t_first, t_chunk_count, t_start, t_end, p_whole_chunk, p_further_chunks, true, t_add);
    
    // The indices returned by MarkTextChunk are code unit indices
    t_cu_range.offset += t_start;
    t_cu_range.length = t_end - t_start;
    
    x_mark . start = t_cu_range.offset;
    x_mark . finish = t_cu_range.offset + t_cu_range.length;
    
    if (p_force)
        MCStringsAddChunks(ctxt, p_chunk_type, t_add, x_mark);
}

void MCStringsMarkTextChunkByOrdinal(MCExecContext& ctxt, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark)
{
    // The incoming indices are for codeunits
    MCRange t_cu_range;
    t_cu_range = MCRangeMake(x_mark . start, x_mark . finish - x_mark . start);
    
    MCAutoStringRef t_string;
    MCStringCopySubstring((MCStringRef)x_mark . text, t_cu_range, &t_string);
    
    int4 t_first;
    int4 t_chunk_count;
    MCStringsGetExtentsByOrdinal(ctxt, p_chunk_type, p_ordinal_type, *t_string, t_first, t_chunk_count);
    
    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunk(ctxt, *t_string, p_chunk_type, t_first, t_chunk_count, t_start, t_end, p_whole_chunk, p_further_chunks, true, t_add);
    
    // The indices returned by MarkTextChunk are code unit indices
    t_cu_range.offset += t_start;
    t_cu_range.length = t_end - t_start;
    
    x_mark . start = t_cu_range.offset;
    x_mark . finish = t_cu_range.offset + t_cu_range.length;

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
    // AL-2014-09-10: [[ Bug 13400 ]] Keep marked strings the correct type where possible
    // Cut the string down, and then convert to data if it is not already data
    if (MCValueGetTypeCode(x_mark . text) != kMCValueTypeCodeData)
    {
        // The incoming indices are for codeunits
        MCRange t_cu_range;
        t_cu_range = MCRangeMake(x_mark . start, x_mark . finish - x_mark . start);
        
        MCAutoStringRef t_string;
        MCStringCopySubstring((MCStringRef)x_mark . text, t_cu_range, &t_string);
        
        MCAutoDataRef t_data;
        ctxt . ConvertToData(*t_string, &t_data);
        
        MCValueRelease(x_mark . text);
        x_mark . text = MCValueRetain(*t_data);
    }
    
    uinteger_t t_first, t_chunk_count;
    MCStringsGetExtentsByRange(ctxt, CT_BYTE, p_first, p_last, x_mark . text, t_first, t_chunk_count);
    
    // convert codeunit indices to byte indices
    x_mark . start = x_mark . start + t_first;
    x_mark . finish = x_mark . start + t_chunk_count;
}

void MCStringsMarkBytesOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, MCMarkedText& x_mark)
{
    // AL-2014-09-10: [[ Bug 13400 ]] Keep marked strings the correct type where possible
    // Cut the string down, and then convert to data if it is not already data
    if (MCValueGetTypeCode(x_mark . text) != kMCValueTypeCodeData)
    {
        // The incoming indices are for codeunits
        MCRange t_cu_range;
        t_cu_range = MCRangeMake(x_mark . start, x_mark . finish - x_mark . start);
        
        MCAutoStringRef t_string;
        MCStringCopySubstring((MCStringRef)x_mark . text, t_cu_range, &t_string);
        
        MCAutoDataRef t_data;
        ctxt . ConvertToData(*t_string, &t_data);
        
        MCValueRelease(x_mark . text);
        x_mark . text = MCValueRetain(*t_data);
    }
    int4 t_first;
    int4 t_chunk_count;
    MCStringsGetExtentsByOrdinal(ctxt, CT_BYTE, p_ordinal_type, x_mark . text, t_first, t_chunk_count);
    
    x_mark . start = x_mark . start + t_first;
    x_mark . finish = x_mark . start + t_chunk_count;
}
