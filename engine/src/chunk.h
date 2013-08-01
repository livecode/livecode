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
	
	// MW-2008-03-05: [[ Owner Reference ]] If desttype == DT_OWNER, then this pointer will
	//   be an MCChunk, otherwise it will be an MCExpression.
	MCExpression *source;

	MCVarref *destvar;
	MCObject *destobj;
	Dest_type desttype;
	Functions function;
	Boolean marked;
public:
	MCChunk *next;
	MCChunk(Boolean isdest);
	~MCChunk();

	Parse_stat parse(MCScriptPoint &spt, Boolean the);
	Exec_stat eval(MCExecPoint &);
	MCVarref *getrootvarref(void);

	void take_components(MCChunk *tchunk);
	Exec_stat getobj(MCExecPoint &, MCObject *&, uint4 &parid, Boolean recurse);
	Exec_stat extents(MCCRef *ref, int4 &start, int4 &number,
	                  MCExecPoint &ep, const char *sptr, const char *eptr,
	                  int4 (*count)(MCExecPoint &ep, const char *sptr,
	                                const char *eptr));
	Exec_stat mark(MCExecPoint &, int4 &start, int4 &end, Boolean force, Boolean wholechunk, bool include_characters = true);
	// MW-2012-02-23: [[ CharChunk ]] Compute the start and end field indices corresponding
	//   to the field char chunk in 'field'.
	Exec_stat markcharactersinfield(uint32_t part_id, MCExecPoint& ep, int32_t& start, int32_t& end, MCField *field);
	Exec_stat gets(MCExecPoint &);
	Exec_stat set(MCExecPoint &, Preposition_type ptype);
	// MW-2012-02-23: [[ PutUnicode ]] Set the chunk to the UTF-16 encoded text in ep.
	Exec_stat setunicode(MCExecPoint& ep, Preposition_type ptype);
	Exec_stat count(Chunk_term tocount, Chunk_term ptype, MCExecPoint &);
	Exec_stat fmark(MCField *fptr, int4 &start, int4 &end, Boolean wholechunk);
	
	// MW-2012-01-27: [[ UnicodeChunks ]] Added the 'keeptext' parameter, if True then on exit the
	//   ep will contain the actual content of the field.
	Exec_stat fieldmark(MCExecPoint &, MCField *fptr, uint4 parid, int4 &start, int4 &end, Boolean wholechunk, Boolean force, Boolean keeptext = False);

	// MW-2011-11-23: [[ Array Chunk Props ]] If index is not nil, then treat as an array chunk prop
	Exec_stat getprop(Properties w, MCExecPoint &, MCNameRef index, Boolean effective);
	Exec_stat setprop(Properties w, MCExecPoint &, MCNameRef index, Boolean effective);
	Exec_stat getobjforprop(MCExecPoint& ep, MCObject*& r_object, uint4& r_parid);

	Exec_stat select(MCExecPoint &, Preposition_type where, Boolean text, Boolean first);
	Exec_stat cut(MCExecPoint &);
	Exec_stat copy(MCExecPoint &);
	Exec_stat del(MCExecPoint &);
	Exec_stat changeprop(MCExecPoint &ep, Properties prop, Boolean value);

	// Returns true if this chunk is of text type
	bool istextchunk(void) const;

	// Returns true if this chunk is of text type and stops at line
	bool islinechunk(void) const;
	
	// Returns true if this chunk is a var, or indexed var
	bool isvarchunk(void) const;

	// Returns the field, part and range of the text chunk
	Exec_stat marktextchunk(MCExecPoint& ep, MCField*& r_field, uint4& r_part, uint4& r_start, uint4& r_end);

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
		return cline == NULL && item == NULL && word == NULL
		       && token == NULL && character == NULL;
	}
};
#endif
