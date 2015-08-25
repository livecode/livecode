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

//
// Script class declarations
//
#ifndef	SCRIPTPOINT_H
#define	SCRIPTPOINT_H

typedef struct
{
	const char *token;
	Token_type type;
	uint2 which;
}
LT;

struct Cvalue
{
	const char *token;
	const char *svalue;
	real8 nvalue;
};

class MCScriptPoint
{
	char *script;
	MCObject *curobj;
	MCHandlerlist *curhlist;
	MCHandler *curhandler;
	const uint1 *curptr;
	const uint1 *tokenptr;
	const uint1 *backupptr;
	char *lowered;
	MCString token;
	MCNameRef token_nameref;
	uint2 loweredsize;
	uint2 line;
	uint2 pos;
	Boolean escapes;
	
	// MW-2011-06-23: If this is true, then we parse the script in 'tag' mode.
	Boolean tagged;
	// MW-2011-06-23: This is true if we are currently consuming tokens inside
	//   a <?rev tag.
	Boolean in_tag;
	// MW-2011-06-23: This is true if we backed up over a tag change point.
	Boolean was_in_tag;

public:
	MCScriptPoint(MCScriptPoint &sp);
	MCScriptPoint(MCObject *, MCHandlerlist *, const char *);
	MCScriptPoint(MCExecPoint &ep);
	MCScriptPoint(const MCString &s);
	~MCScriptPoint();
	MCScriptPoint& operator=(const MCScriptPoint& sp)
	{
		curobj = sp.curobj;
		curhlist = sp.curhlist;
		curhandler = sp.curhandler;
		curptr = sp.curptr;
		tokenptr = sp.tokenptr;
		backupptr = sp.backupptr;
		token = sp.token;
		line = sp.line;
		pos = sp.pos;
		return *this;
	}
	void allowescapes(Boolean which)
	{
		escapes = which;
	}
	
	// MW-2009-03-03: If allowtags is true then we use PHP style tagged parsing,
	//   producing a ST_DATA token in between valid tags.
	void allowtags(Boolean which)
	{
		tagged = which;
	}

	uint2 getline()
	{
		return line;
	}
	void setline(uint2 l)
	{
		line = l;
	}
	uint2 getpos()
	{
		return pos > (curptr - backupptr)
		       ?  pos - (curptr - backupptr) : 1;
	}
	MCString &gettoken()
	{
		return token;
	}

	MCNameRef gettoken_nameref(void);

	const char *getscript()
	{
		return script;
	}
	MCObject *getobj()
	{
		return curobj;
	}
	void sethandler(MCHandler *h)
	{
		curhandler = h;
	}
	MCHandler *gethandler()
	{
		return curhandler;
	}
	MCHandlerlist *gethlist()
	{
		return curhlist;
	}

	const uint1 *getcurptr(void)
	{
		return curptr;
	}

	Parse_stat skip_space();
	Parse_stat skip_eol();
	Parse_stat backup();
	Parse_stat next(Symbol_type &type);
	Parse_stat nexttoken();
	void cleartoken(void);
	Parse_stat lookup(Script_point, const LT *&);
	Parse_stat lookupconstant(MCExpression **);
	Parse_stat skip_token(Script_point, Token_type, uint2 n = 0);
	MCExpression *insertfactor(MCExpression *nfact, MCExpression *&cfact,
	                           MCExpression **top);
	MCExpression *insertbinop(MCExpression *nfact, MCExpression *&cfact,
	                          MCExpression **top);
	Parse_stat parseexp(Boolean single, Boolean items, MCExpression **);
	Exec_stat getcommands(MCExecPoint &ep);
	Exec_stat getfactors(MCExecPoint &ep, Token_type which);
	Exec_stat getconstants(MCExecPoint &ep);
	
	// Search for an existing variable in scope, returning an error if it
	// doesn't exist.
	Parse_stat findvar(MCNameRef name, MCVarref** r_var);

	// Search for an existing variable in scope, creating one if it doesn't
	// exist.
	Parse_stat findnewvar(MCNameRef name, MCNameRef init, MCVarref** r_var);

	// Search for an existing variable in scope, creating a uql-var if it
	// doesn't exist. A uql-var starts off with the same content as its
	// name, but as soon as its used as a container becomes empty.
	Parse_stat finduqlvar(MCNameRef name, MCVarref** r_var);
};
#endif

