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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "handler.h"

#include "scriptpt.h"
#include "util.h"

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

void MCStringsSkipWord(MCExecContext& ctxt, MCStringRef p_string, bool p_skip_spaces, uindex_t& x_offset)
{
    uindex_t t_space_offset;
    uindex_t t_length = MCStringGetLength(p_string);
    uindex_t t_end_quote_offset = t_length;
    uindex_t t_end_line_offset = t_length;
    
    if (MCStringGetNativeCharAtIndex(p_string, x_offset) == '"')
    {
        // then bump the offset up to the next quotation mark + 1, or the beginning of the next line
        // if neither of these are present then set offset to string length.
        MCStringFirstIndexOfChar(p_string, '"', x_offset + 1, kMCCompareExact, t_end_quote_offset);
        MCStringFirstIndexOfChar(p_string, ctxt . GetLineDelimiter(), x_offset + 1, kMCCompareExact, t_end_line_offset);
        
        if (t_end_quote_offset == t_length && t_end_line_offset == t_length)
            x_offset = t_length;
        else
            x_offset = MCU_min(t_end_quote_offset, t_end_line_offset) + 1;
    }
    else
    {
        while (!isspace(MCStringGetNativeCharAtIndex(p_string, x_offset)) && x_offset < t_length)
            x_offset++;
    }
    
    if (p_skip_spaces)
    {
        while (isspace(MCStringGetNativeCharAtIndex(p_string, x_offset)) && x_offset < t_length)
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
    
    uint4 nchunks = 0;
    char_t t_line_delimiter = ctxt . GetLineDelimiter();
    char_t t_item_delimiter = ctxt . GetItemDelimiter();
    uindex_t t_offset = 0;
    uindex_t t_old_offset = 0;
    uindex_t t_end_index = MCStringGetLength(p_string) - 1;
    
    switch (p_chunk_type)
    {
        case CT_LINE:
        case CT_ITEM:
        {
            char_t t_delimiter;
            t_delimiter = p_chunk_type == CT_LINE ? t_line_delimiter : t_item_delimiter;
            nchunks++;
            while (t_offset <= t_end_index && MCStringFirstIndexOfChar(p_string, t_delimiter, t_offset, kMCCompareExact, t_offset))
            {
                if (t_offset < t_end_index)
                    nchunks++;
                t_offset++;
            }
        }
            break;
            
        case CT_WORD:
        {
            uindex_t t_space_offset, t_word_offset;
            // if there are consecutive spaces at the beginning, skip them
            while (MCStringFirstIndexOfChar(p_string, ' ', t_offset, kMCCompareExact, t_space_offset) &&
                   t_space_offset == t_offset)
                t_offset++;
            
            // then keep skipping to the next word until the end of the string.
            while (t_offset <= t_end_index)
            {
                MCStringsSkipWord(ctxt, p_string, true, t_offset);
                nchunks++;
            }
        }
            break;
            
        case CT_TOKEN:
        {
            MCScriptPoint sp(p_string);
            Parse_stat ps = sp.nexttoken();
            while (ps != PS_ERROR && ps != PS_EOF)
            {
                nchunks++;
                ps = sp.nexttoken();
            }
        }
            break;
            
        case CT_CHARACTER:
        {
            // Convert from code unit indices to codepoint indices
            MCRange t_cu_range, t_cp_range;
            t_cu_range = MCRangeMake(0, MCStringGetLength(p_string));
            /* UNCHECKED */ MCStringUnmapCodepointIndices(p_string, t_cu_range, t_cp_range);
            
            nchunks = t_cp_range.length;
            break;
        }
    }
    
    r_count = nchunks;
}

void MCStringsGetExtentsByOrdinal(MCExecContext& ctxt, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, MCStringRef p_string, integer_t& r_first, integer_t& r_chunk_count)
{
    uinteger_t t_count = 0;
    switch (p_ordinal_type)
    {
        case CT_ANY:
            MCStringsCountChunks(ctxt, p_chunk_type, p_string, t_count);
            r_first = MCU_any(t_count);
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
        case CT_LAST:
            MCStringsCountChunks(ctxt, p_chunk_type, p_string, t_count);
            r_first = t_count - 1;
            break;
        case CT_MIDDLE:
            MCStringsCountChunks(ctxt, p_chunk_type, p_string, t_count);
            r_first = t_count / 2;
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

void MCStringsGetExtentsByRange(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, MCStringRef p_string, integer_t& r_first, integer_t& r_chunk_count)
{
    int4 t_chunk_count;
    
    if (p_first < 0 || p_last < 0)
    {
        uinteger_t t_count;
        MCStringsCountChunks(ctxt, p_chunk_type, p_string, t_count);
        
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

void MCStringsGetExtentsByExpression(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, MCStringRef p_string, integer_t& r_first, integer_t& r_chunk_count)
{
    r_chunk_count = 1;
    
    if (p_first < 0)
    {
        uinteger_t t_count;
        MCStringsCountChunks(ctxt, p_chunk_type, p_string, t_count);
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

void MCStringsMarkTextChunk(MCExecContext& ctxt, MCStringRef p_string, Chunk_term p_chunk_type, integer_t p_first, integer_t p_count, integer_t& r_start, integer_t& r_end, bool p_whole_chunk, bool p_further_chunks, bool p_include_chars, integer_t& r_add)
{
    if (p_count == 0)
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
        return;
    }
    
    uindex_t t_end_index = t_length - 1;
    uindex_t t_offset = 0;
    r_add = 0;
    
    switch (p_chunk_type)
    {
        case CT_LINE:
        case CT_ITEM:
        {
            char_t t_line_delimiter = ctxt . GetLineDelimiter();
            char_t t_item_delimiter = ctxt . GetItemDelimiter();
            
            char_t t_delimiter = (p_chunk_type == CT_LINE) ? t_line_delimiter : t_item_delimiter;
            
            // calculate the start of the (p_first)th line or item
            while (p_first-- && MCStringFirstIndexOfChar(p_string, t_delimiter, t_offset, kMCCompareExact, t_offset))
                t_offset++;
            
            // return the number of additional delimiters at the start, if we want to force the number of delimiters
            if (t_offset > 0 && t_offset < t_length && p_first > 0)
                r_add = p_first;
            
            r_start = t_offset;
            
            // calculate the length of the next p_count lines / items
            while (p_count--)
            {
                if (t_offset > t_end_index || !MCStringFirstIndexOfChar(p_string, t_delimiter, t_offset, kMCCompareExact, t_offset))
                {
                    r_end = t_length;
                    break;
                }
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
                return;
            }
        }
            return;
            
        case CT_WORD:
        {
            uindex_t t_space_offset;
            
            // if there are consecutive spaces at the beginning, skip them
            while (MCStringFirstIndexOfChar(p_string, ' ', t_offset, kMCCompareExact, t_space_offset) &&
                   t_space_offset == t_offset)
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
                while (r_end < t_length && isspace(MCStringGetNativeCharAtIndex(p_string, r_end)))
                    r_end++;
                while (r_start > 0 && isspace(MCStringGetNativeCharAtIndex(p_string, r_start - 1)))
                    r_start--;
                return;
            }
            
            // ignore whitespace at the end
            while (r_end > r_start && isspace(MCStringGetNativeCharAtIndex(p_string, r_end - 1)))
                r_end--;
        }
            return;
            
        case CT_TOKEN:
        {
            MCScriptPoint sp(p_string);
            MCerrorlock++;
            
            uint2 t_pos;
            Parse_stat ps = sp.nexttoken();
            t_pos = sp . getindex();
            
            while (p_first-- && ps != PS_ERROR && ps != PS_EOF)
            {
                ps = sp.nexttoken();
                t_pos += sp . getindex();
            }
            r_start = t_pos;
            while (--p_count && ps != PS_ERROR && ps != PS_EOF)
            {
                ps = sp.nexttoken();
                t_pos += sp . getindex();
            }
            r_end = t_pos + MCStringGetLength(sp.gettoken_stringref());
            MCerrorlock--;
        }
            return;
            
        case CT_CHARACTER:
            if (p_include_chars)
            {
                // Resolve the indices
                MCRange t_cp_range, t_cu_range;
                t_cp_range = MCRangeMake(p_first, p_count);
                MCStringMapCodepointIndices(p_string, t_cp_range, t_cu_range);
        
                r_start = t_cu_range.offset;
                r_end = t_cu_range.offset + t_cu_range.length;
                //r_start = p_first;
                //r_end = p_first + p_count;
            }
            return;
        default:
            MCAssert(false);
    }
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
        char_t t_delimiter;
        t_delimiter = p_chunk_type == CT_LINE ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter();
        while (t_add--)
        {
            MCStringPrependNativeChar(x_target, t_delimiter);
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
    int4 t_first;
    int4 t_chunk_count;
    MCStringsGetExtentsByRange(ctxt, p_chunk_type, p_first, p_last, p_source, t_first, t_chunk_count);
    
    MCStringsGetTextChunk(ctxt, p_source, p_chunk_type, t_first, t_chunk_count, p_eval_mutable, r_result);
}

void MCStringsEvalTextChunkByExpression(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, bool p_eval_mutable, MCStringRef& r_result)
{
    int4 t_first;
    int4 t_chunk_count;
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
    int4 t_first;
    int4 t_chunk_count;
    MCStringsGetExtentsByRange(ctxt, p_chunk_type, p_first, p_last, x_target, t_first, t_chunk_count);
    MCStringsSetTextChunk(ctxt, p_source, p_type, p_chunk_type, t_first, t_chunk_count, x_target);
}

void MCStringsSetTextChunkByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, integer_t p_first, MCStringRef& x_target)
{
    int4 t_first;
    int4 t_chunk_count;
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
    int4 t_first;
    int4 t_chunk_count;
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
    int4 t_first;
    int4 t_chunk_count;
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

void MCStringsEvalTextChunk(MCExecContext& ctxt, MCMarkedText p_source, MCStringRef& r_string)
{
    if (p_source . text == nil)
        return;
    
    // The incoming indices are for codepoints
    MCRange t_cp_range, t_cu_range;
    t_cp_range = MCRangeMake(p_source . start, p_source . finish - p_source . start);
    /* UNCHECKED */ MCStringMapCodepointIndices(p_source . text, t_cp_range, t_cu_range);
    
    if (MCStringCopySubstring(p_source . text, t_cu_range, r_string))
        return;
    
    ctxt . Throw();
}

void MCStringsMarkTextChunkByRange(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, MCMarkedText& x_mark)
{
    // The incoming indices are for codepoints
    MCRange t_cp_range, t_cu_range;
    t_cp_range = MCRangeMake(x_mark . start, x_mark . finish - x_mark . start);
    /* UNCHECKED */ MCStringMapCodepointIndices(x_mark . text, t_cp_range, t_cu_range);
    
    MCAutoStringRef t_string;
    MCStringCopySubstring(x_mark . text, t_cu_range, &t_string);
    
    int4 t_first;
    int4 t_chunk_count;
    MCStringsGetExtentsByRange(ctxt, p_chunk_type, p_first, p_last, *t_string, t_first, t_chunk_count);
    
    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunk(ctxt, *t_string, p_chunk_type, t_first, t_chunk_count, t_start, t_end, false, false, true, t_add);
    
    // The indices returned by MarkTextChunk are code unit indices
    t_cu_range.offset += t_start;
    t_cu_range.length = t_end - t_start;
    /* UNCHECKED */ MCStringUnmapCodepointIndices(x_mark . text, t_cu_range, t_cp_range);
    
    x_mark . start = t_cp_range.offset;
    x_mark . finish = t_cp_range.offset + t_cp_range.length;
    
    //x_mark . finish = x_mark . start + t_end;
    //x_mark . start += t_start;
}

void MCStringsMarkTextChunkByOrdinal(MCExecContext& ctxt, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, MCMarkedText& x_mark)
{
    // The incoming indices are for codepoints
    MCRange t_cp_range, t_cu_range;
    t_cp_range = MCRangeMake(x_mark . start, x_mark . finish - x_mark . start);
    /* UNCHECKED */ MCStringMapCodepointIndices(x_mark . text, t_cp_range, t_cu_range);
    
    MCAutoStringRef t_string;
    MCStringCopySubstring(x_mark . text, t_cu_range, &t_string);
    
    int4 t_first;
    int4 t_chunk_count;
    MCStringsGetExtentsByOrdinal(ctxt, p_chunk_type, p_ordinal_type, *t_string, t_first, t_chunk_count);
    
    int4 t_add;
    int4 t_start, t_end;
    MCStringsMarkTextChunk(ctxt, *t_string, p_chunk_type, t_first, t_chunk_count, t_start, t_end, false, false, true, t_add);
    
    // The indices returned by MarkTextChunk are code unit indices
    t_cu_range.offset += t_start;
    t_cu_range.length = t_end - t_start;
    /* UNCHECKED */ MCStringUnmapCodepointIndices(x_mark . text, t_cu_range, t_cp_range);
    
    x_mark . start = t_cp_range.offset;
    x_mark . finish = t_cp_range.offset + t_cp_range.length;
    
    //x_mark . finish = x_mark . start + t_end;
    //x_mark . start += t_start;
}

void MCStringsMarkLinesOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_LINE, p_first, p_last, x_mark);
}

void MCStringsMarkLinesOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_LINE, p_ordinal_type, x_mark);
}

void MCStringsMarkItemsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_ITEM, p_first, p_last, x_mark);
}

void MCStringsMarkItemsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_ITEM, p_ordinal_type, x_mark);
}

void MCStringsMarkWordsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_WORD, p_first, p_last, x_mark);
}

void MCStringsMarkWordsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_WORD, p_ordinal_type, x_mark);
}

void MCStringsMarkTokensOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_TOKEN, p_first, p_last, x_mark);
}

void MCStringsMarkTokensOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_TOKEN, p_ordinal_type, x_mark);
}

void MCStringsMarkCharsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByRange(ctxt, CT_CHARACTER, p_first, p_last, x_mark);
}

void MCStringsMarkCharsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, MCMarkedText& x_mark)
{
    MCStringsMarkTextChunkByOrdinal(ctxt, CT_CHARACTER, p_ordinal_type, x_mark);
}

bool MCStringsFindNextChunk(MCExecContext& ctxt, MCStringRef p_string, Chunk_term p_chunk_type, uindex_t t_length, MCRange& x_range, bool p_not_first, bool& r_last)
{
    uindex_t t_end_index = t_length - 1;
    uindex_t t_offset = x_range . offset + x_range . length;
    
    if (p_not_first && p_chunk_type != CT_CHARACTER)
        t_offset++;

    if (t_offset >= t_length)
        return false;
    
    x_range . offset = t_offset;
    
    switch (p_chunk_type)
    {
        case CT_LINE:
        case CT_ITEM:
        {
            char_t t_line_delimiter = ctxt . GetLineDelimiter();
            char_t t_item_delimiter = ctxt . GetItemDelimiter();
            
            char_t t_delimiter = (p_chunk_type == CT_LINE) ? t_line_delimiter : t_item_delimiter;
            
            // calculate the length of the line / item
            if (!MCStringFirstIndexOfChar(p_string, t_delimiter, t_offset, kMCCompareExact, t_offset))
                x_range . length = t_length - t_offset;
            else
                x_range . length = t_offset - x_range . offset;
        }
            return true;
            
        case CT_WORD:
        {
            uindex_t t_space_offset;
            // if there are consecutive spaces at the beginning, skip them
            while (MCStringFirstIndexOfChar(p_string, ' ', t_offset, kMCCompareExact, t_space_offset) &&
                   t_space_offset == t_offset)
                t_offset++;
            
            if (t_offset >= t_length)
                return false;
            
            MCStringsSkipWord(ctxt, p_string, false, t_offset);
            
            x_range . length = t_offset - x_range . offset;
        }
            return true;
            
        case CT_TOKEN:
        {
            MCAutoStringRef t_string;
            MCStringCopySubstring(p_string, MCRangeMake(x_range . offset + x_range . length, UINDEX_MAX), &t_string);
            MCScriptPoint sp(*t_string);
            MCerrorlock++;
            
            uint2 t_pos;
            Parse_stat ps = sp.nexttoken();
            if (ps == PS_ERROR || ps == PS_EOF)
                return false;
            t_pos = sp . getindex();
        
            x_range . offset = t_pos;

            ps = sp.nexttoken();
            t_pos += sp . getindex();

            if (ps == PS_ERROR || ps == PS_EOF)
                x_range . length = t_length - t_offset;
            else
                x_range . length = MCStringGetLength(sp.gettoken_stringref());
            
            MCerrorlock--;
        }
            return true;
            
        case CT_CHARACTER:
            x_range . length = 1;
            return true;
            
        default:
            MCAssert(false);
    }
    return false;
}
