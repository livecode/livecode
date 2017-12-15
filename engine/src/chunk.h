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

#ifndef	CHUNK_H
#define	CHUNK_H

#ifndef __MC_FOUNDATION_CHUNK__
#include "foundation-chunk.h"
#endif

#include "express.h"

class MCCRef
{
public:
	Chunk_term etype;
	Chunk_term otype;
	Chunk_term ptype;
	MCExpression *startpos;
	MCExpression *endpos;
	MCCRef *next;
	MCCRef();
	~MCCRef();
};

class MCChunk : public MCExpression
{
	MCCRef *url;
	MCCRef *stack;
	MCCRef *background;
	MCCRef *card;
	MCCRef *group;
	MCCRef *object;
    
    // [[ Element Chunk ]] Add element chunk
    MCCRef *element;
    
	MCCRef *cline;
	MCCRef *token;
	MCCRef *item;
	MCCRef *word;
	MCCRef *character;
    // AL-2014-01-08 [[ CharChunks ]] Add 'codepoint, codeunit and byte' to chunk types
    MCCRef *codepoint;
    MCCRef *codeunit;
    MCCRef *byte;
	
    // AL-2014-03-11 [[ LanguageChunks ]] Add 'paragraph, sentence and trueword' to chunk types
    MCCRef *paragraph;
    MCCRef *sentence;
    MCCRef *trueword;
    
	// MW-2008-03-05: [[ Owner Reference ]] If desttype == DT_OWNER, then this pointer will
	//   be an MCChunk, otherwise it will be an MCExpression.
	MCExpression *source;

	MCVarref *destvar;
	MCObject *destobj;
	Dest_type desttype;
	Functions function;
	bool marked : 1;
    
    // MW-2014-05-28: [[ Bug 11928 ]] This is set to true after 'destvar' has been evaluated
    //   as a chunk. This stops stale chunk information being used in MCChunk::del.
    bool m_transient_text_chunk : 1;
public:
	MCChunk *next;
	MCChunk(Boolean isdest);
	~MCChunk();

	Parse_stat parse(MCScriptPoint &spt, Boolean the);
    //Exec_stat eval(MCExecPoint &);
    void eval_ctxt(MCExecContext &ctxt, MCExecValue& r_value);

	MCVarref *getrootvarref(void);

	Chunk_term getlastchunktype(void);

    bool evalobjectchunk(MCExecContext& ctxt, bool p_whole_chunk, bool p_force, MCObjectChunkPtr& r_chunk);
    bool evalvarchunk(MCExecContext& ctxt, bool whole_chunk, bool force, MCVariableChunkPtr& r_chunk);
    bool evalurlchunk(MCExecContext& ctxt, bool p_whole_chunk, bool p_force, int p_preposition, MCUrlChunkPtr& r_chunk);
	bool evalelementchunk(MCExecContext& ctxt, MCProperListRef& r_elements);
    
	void take_components(MCChunk *tchunk);

    // getobj calls getoptionalobj and throws in case nothing is returned.
    bool getobj(MCExecContext &ctxt, MCObject *& objptr, uint4 &parid, Boolean recurse);

    bool getobj(MCExecContext &ctxt,MCObjectPtr&, Boolean recurse);

    // Added for MCChunk::count:
    //  in some cases there is no object to return but no error either
    //  and caller might want to default to something else
    void getoptionalobj(MCExecContext& ctxt, MCObject *&r_object, uint4& r_parid, Boolean p_recurse);
    void getoptionalobj(MCExecContext &ctxt, MCObjectPtr &r_object, Boolean p_recurse);

    
    void mark(MCExecContext &ctxt, bool set, bool wholechunk, MCMarkedText& x_mark, bool includechars = true);
    void mark_for_eval(MCExecContext& ctxt, MCMarkedText& x_mark);
    bool set(MCExecContext& ctxt, Preposition_type p_type, MCValueRef p_value, bool p_unicode = false);
    bool set(MCExecContext& ctxt, Preposition_type p_type, MCExecValue p_value, bool p_unicode = false);

    
    void count(MCExecContext &ctxt, Chunk_term tocount, Chunk_term ptype, uinteger_t &r_count);
    // SN-2015-02-13: [[ Bug 14467 ]] [[ Bug 14053 ]] Refactored object properties
    //  lookup, to ensure it is done the same way in MCChunk::getprop / setprop
    bool getsetprop(MCExecContext& ctxt, Properties which, MCNameRef index, Boolean effective, bool p_is_get_operation, MCExecValue& r_value);
    
    bool getprop(MCExecContext& ctxt, Properties which, MCNameRef index, Boolean effective, MCExecValue& r_value);
    bool setprop(MCExecContext& ctxt, Properties which, MCNameRef index, Boolean effective, MCExecValue p_value);
    
    bool getsetcustomprop(MCExecContext& ctxt, MCNameRef p_prop_name, MCNameRef p_index_name, bool p_is_get_operation, MCExecValue& r_value);
    
    bool getcustomprop(MCExecContext& ctxt, MCNameRef p_prop_name, MCNameRef p_index_name, MCExecValue& r_value);
    bool setcustomprop(MCExecContext& ctxt, MCNameRef p_prop_name, MCNameRef p_index_name, MCExecValue p_value);
    
    bool getobjforprop(MCExecContext& ctxt, MCObject*& r_object, uint4& r_parid);
	// REMOVE: Exec_stat select(MCExecPoint &, Preposition_type where, Boolean text, Boolean first);
	// Returns true if this chunk is of text type
	bool istextchunk(void) const;

	// Returns true if this chunk is of text type and stops at line
	bool islinechunk(void) const;
	
	// Returns true if this chunk is a var, or indexed var
	bool isvarchunk(void) const;

	// Returns true if this chunk is a substring of a variable.
	bool issubstringchunk(void) const;
	
	// Returns true if this chunk is of a url
	bool isurlchunk(void) const;

    // Returns true if the underlying value should should be considered a string
    // (not excluding the case where it is later converted to data).
    bool isstringchunk(void) const;
    
    // Returns true if the byte chunk is non-nil and thus the underlying
    // value should ultimately be converted to data.
    bool isdatachunk(void) const
    {
        return (byte != nil);
    }
    
    // Returns true if the element chunk is non-nil and thus a property of
    // the chunk expression should be evaluated accordingly
    bool iselementchunk(void) const
    {
        return (element != nil);
    }
	MCObject *getdestobj()
	{
		return destobj;
	}
	void setdestobj(MCObject *d)
	{
		destobj = d;
	}
	bool notextchunks()
	{
        // SN-2014-03-21: [[ Bug 11954 ]] Typo was ensuring to return false in any case
		return cline == NULL && paragraph == NULL && sentence == NULL && item == NULL
                && trueword == NULL && word == NULL && token == NULL && character == NULL
                && codepoint == NULL && codeunit == NULL && byte == NULL;
	}
    bool noobjectchunks()
    {
        return stack == nil && background == nil && card == nil
                && group == nil && object == nil;
    }
};

MCChunkType MCChunkTypeFromChunkTerm(Chunk_term p_chunk_term);
inline bool MCChunkTermIsControl(Chunk_term p_chunk_term)
{
	return p_chunk_term >= CT_FIRST_CONTROL && p_chunk_term <= CT_LAST_CONTROL;
}
#endif
