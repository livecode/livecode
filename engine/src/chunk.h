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
	Boolean marked : 1;
    
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

#ifdef LEGACY_EXEC
    Exec_stat eval_legacy(MCExecPoint &ep);
#endif
	MCVarref *getrootvarref(void);
	
	void compile(MCSyntaxFactoryRef factory);
	void compile_in(MCSyntaxFactoryRef factory);
	void compile_out(MCSyntaxFactoryRef factory);
	void compile_inout(MCSyntaxFactoryRef factory);	
	void compile_object_ptr(MCSyntaxFactoryRef factory);

	Chunk_term getlastchunktype(void);

    bool evalobjectchunk(MCExecContext& ctxt, bool p_whole_chunk, bool p_force, MCObjectChunkPtr& r_chunk);
    bool evalvarchunk(MCExecContext& ctxt, bool whole_chunk, bool force, MCVariableChunkPtr& r_chunk);
    bool evalurlchunk(MCExecContext& ctxt, bool p_whole_chunk, bool p_force, int p_preposition, MCUrlChunkPtr& r_chunk);
	
	void take_components(MCChunk *tchunk);

    // getobj calls getoptionalobj and throws in case nothing is returned.
#ifdef LEGACY_EXEC
    /* WRAPPER */ Exec_stat getobj(MCExecPoint &, MCObject *&, uint4 &parid, Boolean recurse);
#endif
    bool getobj(MCExecContext &ctxt, MCObject *& objptr, uint4 &parid, Boolean recurse);

#ifdef LEGACY_EXEC
    /* WRAPPER */ Exec_stat getobj(MCExecPoint&, MCObjectPtr&, Boolean recurse);
#endif
    bool getobj(MCExecContext &ctxt,MCObjectPtr&, Boolean recurse);

    // Added for MCChunk::count:
    //  in some cases there is no object to return but no error either
    //  and caller might want to default to something else
    void getoptionalobj(MCExecContext& ctxt, MCObject *&r_object, uint4& r_parid, Boolean p_recurse);
    void getoptionalobj(MCExecContext &ctxt, MCObjectPtr &r_object, Boolean p_recurse);

#ifdef LEGACY_EXEC    
    Exec_stat getobj_legacy(MCExecPoint &ep, MCObject *&objptr, uint4 &parid, Boolean recurse);
    
	Exec_stat extents(MCCRef *ref, int4 &start, int4 &number,
	                  MCExecPoint &ep, const char *sptr, const char *eptr,
	                  int4 (*count)(MCExecPoint &ep, const char *sptr,
	                                const char *eptr));
#endif
    
#ifdef LEGACY_EXEC
    /* WRAPPER */ Exec_stat mark(MCExecPoint &ep, Boolean force, Boolean wholechunk, MCMarkedText& r_mark, bool includechars = true);
#endif
    void mark(MCExecContext &ctxt, bool set, bool wholechunk, MCMarkedText& x_mark, bool includechars = true);
#ifdef LEGACY_EXEC
	Exec_stat mark_legacy(MCExecPoint &, int4 &start, int4 &end, Boolean force, Boolean wholechunk, bool include_characters = true);

	// MW-2012-02-23: [[ CharChunk ]] Compute the start and end field indices corresponding
	//   to the field char chunk in 'field'.
	Exec_stat markcharactersinfield(uint32_t part_id, MCExecPoint& ep, int32_t& start, int32_t& end, MCField *field);

	Exec_stat gets(MCExecPoint &);
	Exec_stat set(MCExecPoint &, Preposition_type ptype);

#endif

#ifdef LEGACY_EXEC
    Exec_stat set(MCExecPoint& ep, Preposition_type p_type, MCValueRef p_text, bool p_unicode = false);
#endif
    bool set(MCExecContext& ctxt, Preposition_type p_type, MCValueRef p_value, bool p_unicode = false);
    bool set(MCExecContext& ctxt, Preposition_type p_type, MCExecValue p_value, bool p_unicode = false);

#ifdef LEGACY_EXEC 
	Exec_stat gets(MCExecPoint &);
	Exec_stat set_legacy(MCExecPoint &, Preposition_type ptype);
       
	// MW-2012-02-23: [[ PutUnicode ]] Set the chunk to the UTF-16 encoded text in ep.
	Exec_stat setunicode(MCExecPoint& ep, Preposition_type ptype);
#endif
    
#ifdef LEGACY_EXEC
    /* WRAPPER */ Exec_stat count(Chunk_term tocount, Chunk_term ptype, MCExecPoint &);
#endif
    void count(MCExecContext &ctxt, Chunk_term tocount, Chunk_term ptype, uinteger_t &r_count);
#ifdef LEGACY_EXEC	
	Exec_stat fmark(MCField *fptr, int4 &start, int4 &end, Boolean wholechunk);

	// MW-2012-01-27: [[ UnicodeChunks ]] Added the 'keeptext' parameter, if True then on exit the
	//   ep will contain the actual content of the field.
	Exec_stat fieldmark(MCExecPoint &, MCField *fptr, uint4 parid, int4 &start, int4 &end, Boolean wholechunk, Boolean force, Boolean keeptext = False);
    
	// MW-2011-11-23: [[ Array Chunk Props ]] If index is not nil, then treat as an array chunk prop
	Exec_stat getprop(Properties w, MCExecPoint &, MCNameRef index, Boolean effective);

	Exec_stat setprop(Properties w, MCExecPoint &, MCNameRef index, Boolean effective);
#endif

    // SN-2015-02-13: [[ Bug 14467 ]] [[ Bug 14053 ]] Refactored object properties
    //  lookup, to ensure it is done the same way in MCChunk::getprop / setprop
    bool getsetprop(MCExecContext& ctxt, Properties which, MCNameRef index, Boolean effective, bool p_is_get_operation, MCExecValue& r_value);
    
    bool getprop(MCExecContext& ctxt, Properties which, MCNameRef index, Boolean effective, MCExecValue& r_value);
    bool setprop(MCExecContext& ctxt, Properties which, MCNameRef index, Boolean effective, MCExecValue p_value);
    
#ifdef LEGACY_EXEC
	Exec_stat getprop_legacy(Properties w, MCExecPoint &, MCNameRef index, Boolean effective);
	Exec_stat setprop_legacy(Properties w, MCExecPoint &, MCNameRef index, Boolean effective);
	Exec_stat getobjforprop(MCExecPoint& ep, MCObject*& r_object, uint4& r_parid);
#endif
    bool getobjforprop(MCExecContext& ctxt, MCObject*& r_object, uint4& r_parid);
	// REMOVE: Exec_stat select(MCExecPoint &, Preposition_type where, Boolean text, Boolean first);
#ifdef LEGACY_EXEC
	Exec_stat cut(MCExecPoint &);
	Exec_stat copy(MCExecPoint &);

	// REMOVE: Exec_stat del(MCExecPoint &);
	Exec_stat changeprop(MCExecPoint &ep, Properties prop, Boolean value);
#endif
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
#ifdef LEGACY_EXEC
	// Returns the field, part and range of the text chunk
	Exec_stat marktextchunk(MCExecPoint& ep, MCField*& r_field, uint4& r_part, uint4& r_start, uint4& r_end);
#endif
	MCObject *getdestobj()
	{
		return destobj;
	}
	void setdestobj(MCObject *d)
	{
		destobj = d;
	}
	Boolean nochunks()
	{
        // SN-2014-03-21: [[ Bug 11954 ]] Typo was ensuring to return false in any case
		return cline == NULL && paragraph == NULL && sentence == NULL && item == NULL
                && trueword == NULL && word == NULL && token == NULL && character == NULL
                && codepoint == NULL && codeunit == NULL && byte == NULL;
	}
};

class MCTextChunkIterator
{
    MCStringRef text;
    MCScriptPoint *sp;
    Chunk_term type;
    MCRange range;
    bool exhausted;
    uindex_t length;
    bool first_chunk;
    MCAutoArray<MCRange> breaks;
    uindex_t break_position;
    
    // store the number of codeunits matched in text when searching for
    //  delimiter, so that we can increment the range appropriately.
    uindex_t delimiter_length;
    
    public:
    MCTextChunkIterator(Chunk_term p_chunk_type, MCStringRef p_text);
    // AL-2015-02-10: [[ Bug 14532 ]] Add text chunk iterator constructor for restricted range chunk operations.
    MCTextChunkIterator(Chunk_term p_chunk_type, MCStringRef p_text, MCRange p_restriction);
    ~MCTextChunkIterator();
    
    MCRange getrange()
    {
        return range;
    }
    
    bool isexhausted()
    {
        return exhausted;
    }
    
    bool next(MCExecContext& ctxt);
    bool copystring(MCStringRef& r_string);
    uindex_t countchunks(MCExecContext& ctxt);
    bool isamong(MCExecContext& ctxt, MCStringRef p_needle);
    uindex_t chunkoffset(MCExecContext& ctxt, MCStringRef p_needle, uindex_t p_start_offset);
};
#endif
