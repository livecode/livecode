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

#ifndef __MC_FOUNDATION_CHUNK__
#define __MC_FOUNDATION_CHUNK__

#ifndef __MC_FOUNDATION_AUTO__
#include "foundation-auto.h"
#endif

enum MCChunkType
{
    kMCChunkTypeLine,
    kMCChunkTypeParagraph,
    kMCChunkTypeSentence,
    kMCChunkTypeItem,
    kMCChunkTypeTrueWord,
    kMCChunkTypeWord,
    kMCChunkTypeToken,
    kMCChunkTypeCharacter,
    kMCChunkTypeCodepoint,
    kMCChunkTypeCodeunit,
    kMCChunkTypeByte,
};

typedef uinteger_t (MCChunkCountCallback(void *context, MCRange *range));

uinteger_t MCChunkCountByteChunkCallback(void *context, MCRange *range);
uinteger_t MCChunkCountCodepointChunkCallback(void *context, MCRange *range);

uindex_t MCChunkCountChunkChunksInRange(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, MCRange *p_range);

bool MCChunkGetExtentsByRangeInRange(bool p_strict, bool p_boundary_start, bool p_boundary_end, integer_t p_first, integer_t p_last, MCChunkCountCallback p_callback, void *p_context, MCRange *p_range, uindex_t& r_first, uindex_t& r_chunk_count);
bool MCChunkGetExtentsByExpressionInRange(bool p_strict, bool p_boundary_start, bool p_boundary_end, integer_t p_first, MCChunkCountCallback p_callback, void *p_context, MCRange *p_range, uindex_t& r_first, uindex_t& r_chunk_count);

bool MCChunkGetExtentsOfByteChunkByRangeInRange(MCDataRef p_data, MCRange *p_range, integer_t p_first, integer_t p_last, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);

bool MCChunkGetExtentsOfByteChunkByExpressionInRange(MCDataRef p_data, MCRange *p_range, integer_t p_first, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);

bool MCChunkGetExtentsOfCodeunitChunkByRangeInRange(MCStringRef p_data, MCRange *p_range, integer_t p_first, integer_t p_last, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);
bool MCChunkGetExtentsOfCodeunitChunkByExpressionInRange(MCStringRef p_data, MCRange *p_range, integer_t p_first, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);

bool MCChunkGetExtentsOfGraphemeChunkByRangeInRange(MCStringRef p_string, MCRange *p_range, integer_t p_first, integer_t p_last, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);
bool MCChunkGetExtentsOfGraphemeChunkByExpressionInRange(MCStringRef p_string, MCRange *p_range, integer_t p_first, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);

bool MCChunkGetExtentsOfElementChunkByRangeInRange(MCProperListRef p_string, MCRange *p_range, integer_t p_first, integer_t p_last, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);
bool MCChunkGetExtentsOfElementChunkByExpressionInRange(MCProperListRef p_string, MCRange *p_range, integer_t p_first, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);

bool MCChunkGetExtentsOfChunkChunkByRangeInRange(MCStringRef p_string, MCRange *p_range, MCStringRef p_delimiter, MCStringOptions p_options, integer_t p_first, integer_t p_last, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);
bool MCChunkGetExtentsOfChunkChunkByExpressionInRange(MCStringRef p_string, MCRange *p_range, MCStringRef p_delimiter, MCStringOptions p_options, integer_t p_first, bool p_strict, bool p_boundary_start, bool p_boundary_end, uindex_t& r_first, uindex_t& r_chunk_count);

bool MCChunkIsAmongTheChunksOfRange(MCStringRef p_chunk, MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, MCRange p_range);

bool MCChunkOffsetOfChunkInRange(MCStringRef p_string, MCStringRef p_needle, MCStringRef p_delimiter, bool p_whole_matches, MCStringOptions p_options, MCRange p_range, uindex_t& r_offset);

void MCChunkSkipWord(MCStringRef p_string, MCStringRef p_line_delimiter, MCStringOptions p_options, bool p_skip_spaces, uindex_t& x_offset);

class MCTextChunkIterator
{
protected:
    MCStringRef m_text;
    MCRange m_range;
    bool m_exhausted;
    uindex_t m_length;
    MCStringOptions m_options;
    MCChunkType m_chunk_type;
    
public:
    MCTextChunkIterator(MCStringRef p_text, MCChunkType p_chunk_type);
    // AL-2015-02-10: [[ Bug 14532 ]] Add text chunk iterator constructor for restricted range chunk operations.
    MCTextChunkIterator(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction);
    
    virtual ~MCTextChunkIterator();
    
    MCRange GetRange() const
    {
        return m_range;
    }
    
    bool IsExhausted() const
    {
        return m_exhausted;
    }
    
    bool CopyString(MCStringRef& r_string) const
    {
        return MCStringCopySubstring(m_text, m_range, r_string);
    }
    
    void SetOptions(MCStringOptions p_options)
    {
        m_options = p_options;
    }
    
    MCChunkType GetType() const
    {
        return m_chunk_type;
    }
    
    virtual bool Next() = 0;
    
    virtual uindex_t ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset, uindex_t *p_end_offset, bool p_whole_matches)
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
        while (Next() && (p_end_offset == nil || *p_end_offset < t_chunk_offset));
        
        return 0;
    }
    
    virtual uindex_t CountChunks()
    {
        uindex_t t_count = 0;
        while (Next())
            t_count++;
        
        return t_count;
    }

    virtual bool IsAmong(MCStringRef p_needle)
    {
        if (MCStringIsEmpty(p_needle))
            return false;
    
        while (Next())
            if (MCStringSubstringIsEqualTo(m_text, m_range, p_needle, m_options))
                return true;
        
        // AL-2014-09-10: [[ Bug 13356 ]] If we were not 'exhausted', then there was a trailing delimiter
        //  which means empty is considered to be among the chunks.
        if (MCStringIsEmpty(p_needle) && !m_exhausted)
            return true;
        
        return false;
    }
};

class MCTextChunkIterator_Grapheme : public MCTextChunkIterator
{
public:
    MCTextChunkIterator_Grapheme(MCStringRef p_text, MCChunkType p_chunk_type);
    MCTextChunkIterator_Grapheme(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction);
    ~MCTextChunkIterator_Grapheme();
    
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
};

class MCTextChunkIterator_Codepoint : public MCTextChunkIterator
{
public:
    MCTextChunkIterator_Codepoint(MCStringRef p_text, MCChunkType p_chunk_type);
    // AL-2015-02-10: [[ Bug 14532 ]] Add text chunk iterator constructor for restricted range chunk operations.
    MCTextChunkIterator_Codepoint(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction);
    ~MCTextChunkIterator_Codepoint();
    
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
};

class MCTextChunkIterator_Codeunit : public MCTextChunkIterator
{
public:
    MCTextChunkIterator_Codeunit(MCStringRef p_text, MCChunkType p_chunk_type);
    // AL-2015-02-10: [[ Bug 14532 ]] Add text chunk iterator constructor for restricted range chunk operations.
    MCTextChunkIterator_Codeunit(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction);
    ~MCTextChunkIterator_Codeunit();
    
    virtual uindex_t CountChunks();
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
    virtual uindex_t ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset, uindex_t *p_end_offset, bool p_whole_matches);
};

class MCTextChunkIterator_Delimited : public MCTextChunkIterator
{
    // store the number of codeunits matched in text when searching for
    //  delimiter, so that we can increment the range appropriately.
    uindex_t m_delimiter_length;
    MCStringRef m_delimiter;
    bool m_first_chunk;
public:
    MCTextChunkIterator_Delimited(MCStringRef p_text, MCChunkType p_chunk_type, MCStringRef p_delimiter);
    // AL-2015-02-10: [[ Bug 14532 ]] Add text chunk iterator constructor for restricted range chunk operations.
    MCTextChunkIterator_Delimited(MCStringRef p_text, MCChunkType p_chunk_type, MCStringRef p_delimiter, MCRange p_restriction);
    ~MCTextChunkIterator_Delimited();
    
    void SetDelimiter(MCStringRef p_delimiter)
    {
        MCValueAssign(m_delimiter, p_delimiter);
    }
    
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
    virtual uindex_t ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset, uindex_t *p_end_offset, bool p_whole_matches);
};

class MCTextChunkIterator_Word : public MCTextChunkIterator
{
    // store the number of codeunits matched in text when searching for
    //  delimiter, so that we can increment the range appropriately.
    MCStringRef m_line_delimiter;
public:
    MCTextChunkIterator_Word(MCStringRef p_text, MCChunkType p_chunk_type, MCStringRef p_line_delimiter);
    // AL-2015-02-10: [[ Bug 14532 ]] Add text chunk iterator constructor for restricted range chunk operations.
    MCTextChunkIterator_Word(MCStringRef p_text, MCChunkType p_chunk_type, MCStringRef p_line_delimiter, MCRange p_restriction);
    ~MCTextChunkIterator_Word();
    
    void SetLineDelimiter(MCStringRef p_delimiter)
    {
        MCValueAssign(m_line_delimiter, p_delimiter);
    }
    
    virtual bool Next();
};

class MCTextChunkIterator_ICU : public MCTextChunkIterator
{
    MCAutoArray<MCRange> m_breaks;
    uindex_t m_break_position;
public:
    MCTextChunkIterator_ICU(MCStringRef p_text, MCChunkType p_chunk_type);
    // AL-2015-02-10: [[ Bug 14532 ]] Add text chunk iterator constructor for restricted range chunk operations.
    MCTextChunkIterator_ICU(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction);
    ~MCTextChunkIterator_ICU();
    
    virtual bool Next();
};

class MCScriptPoint;

class MCTextChunkIterator_Tokenized : public MCTextChunkIterator
{
    MCScriptPoint *m_sp;
public:
    MCTextChunkIterator_Tokenized(MCStringRef p_text, MCChunkType p_chunk_type);
    // AL-2015-02-10: [[ Bug 14532 ]] Add text chunk iterator constructor for restricted range chunk operations.
    MCTextChunkIterator_Tokenized(MCStringRef p_text, MCChunkType p_chunk_type, MCRange p_restriction);
    ~MCTextChunkIterator_Tokenized();
    
    virtual bool Next();
};

MCTextChunkIterator *MCChunkCreateTextChunkIterator(MCStringRef p_text, MCRange *p_range, MCChunkType p_chunk_type, MCStringRef p_line_delimiter, MCStringRef p_item_delimiter, MCStringOptions p_options);

MCChunkType MCChunkTypeSimplify(MCStringRef p_string, MCChunkType p_type);
MCChunkType MCChunkTypeFromCharChunkType(MCCharChunkType p_type);

#endif // __MC_FOUNDATION_CHUNK__
