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

#ifndef __MC_FOUNDATION_CHUNK__
#define __MC_FOUNDATION_CHUNK__

#include "foundation.h"

typedef uinteger_t (MCChunkCountCallback(void *context));

uinteger_t MCChunkCountByteChunkCallback(void *context);
uinteger_t MCChunkCountCodepointChunkCallback(void *context);

uindex_t MCChunkCountChunkChunks(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options);

void MCChunkGetExtentsByRange(integer_t p_first, integer_t p_last, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsByExpression(integer_t p_first, MCChunkCountCallback p_callback, void *p_context, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsOfByteChunkByRange(MCDataRef p_data, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count);
void MCChunkGetExtentsOfByteChunkByExpression(MCDataRef p_data, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsOfCodeunitChunkByRange(MCStringRef p_data, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count);
void MCChunkGetExtentsOfCodeunitChunkByExpression(MCStringRef p_data, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsOfElementChunkByRange(MCProperListRef p_string, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count);
void MCChunkGetExtentsOfElementChunkByExpression(MCProperListRef p_string, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count);

void MCChunkGetExtentsOfChunkChunkByRange(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, integer_t p_first, integer_t p_last, uindex_t& r_first, uindex_t& r_chunk_count);
void MCChunkGetExtentsOfChunkChunkByExpression(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, integer_t p_first, uindex_t& r_first, uindex_t& r_chunk_count);

bool MCChunkIsAmongTheChunksOfRange(MCStringRef p_chunk, MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, MCRange p_range);

bool MCChunkOffsetOfChunkInRange(MCStringRef p_string, MCStringRef p_needle, MCStringRef p_delimiter, bool p_whole_matches, MCStringOptions p_options, MCRange p_range, uindex_t& r_offset);

typedef bool (*MCChunkApplyCallback)(void *context, MCStringRef string, MCRange chunk_range);
bool MCChunkApply(MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, MCChunkApplyCallback p_callback, void *context);

bool MCChunkIterate(MCRange& x_range, MCStringRef p_string, MCStringRef p_delimiter, MCStringOptions p_options, bool p_first);

class MCTextChunkIterator
{
protected:
    MCStringRef m_text;
    MCRange m_range;
    bool m_exhausted;
    uindex_t m_length;
    Chunk_term m_chunk_type;
    
public:
    ~MCTextChunkIterator();
    
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
    
    virtual uindex_t CountChunks()
    {
        uindex_t t_count = 0;
        while (Next())
            t_count++;
        
        return t_count;
    }
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
    virtual uindex_t ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset);
};

class MCTextChunkIterator_Codepoint : public MCTextChunkIterator
{
    MCStringOptions m_options;
public:
    MCTextChunkIterator_Codepoint();
    ~MCTextChunkIterator_Codepoint();
    
    void SetOptions(MCStringOptions p_options)
    {
        m_options = p_options;
    }
    
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
    virtual uindex_t ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset);
};

class MCTextChunkIterator_Codeunit : public MCTextChunkIterator
{
public:
    MCTextChunkIterator_Codeunit();
    ~MCTextChunkIterator_Codeunit();
    
    virtual uindex_t CountChunks();
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
    virtual uindex_t ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset);
};

class MCTextChunkIterator_Delimited : public MCTextChunkIterator
{
    // store the number of codeunits matched in text when searching for
    //  delimiter, so that we can increment the range appropriately.
    uindex_t m_delimiter_length;
    MCStringRef m_delimiter;
    MCStringOptions m_options;
    bool m_whole_matches;
    bool m_first_chunk;
public:
    MCTextChunkIterator_Delimited();
    ~MCTextChunkIterator_Delimited();
    
    void SetDelimiter(MCStringRef p_delimiter)
    {
        MCValueAssign(m_delimiter, p_delimiter);
    }
    
    void SetOptions(MCStringOptions p_options)
    {
        m_options = p_options;
    }
    
    void SetWholeMatches(bool p_whole_matches)
    {
        m_whole_matches = p_whole_matches;
    }
    
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
    virtual uindex_t ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset);
};

class MCTextChunkIterator_ICU : public MCTextChunkIterator
{
    MCAutoArray<MCRange> m_breaks;
    uindex_t m_break_position;
public:
    MCTextChunkIterator_ICU();
    ~MCTextChunkIterator_ICU();
    
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
    virtual uindex_t ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset);
};

class MCTextChunkIterator_Tokenized : public MCTextChunkIterator
{
    MCScriptPoint *m_sp;
public:
    MCTextChunkIterator_Tokenized();
    ~MCTextChunkIterator_Tokenized();
    
    virtual bool Next();
    virtual bool IsAmong(MCStringRef p_needle);
    virtual uindex_t ChunkOffset(MCStringRef p_needle, uindex_t p_start_offset);
};

#endif // __MC_FOUNDATION_CHUNK__