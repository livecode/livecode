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
// MCHandler class declarations
//
#ifndef	STATEMENT_H
#define	STATEMENT_H

class MCScriptPoint;
class MCParameter;
class MCChunk;
class MCExpression;
class MCVarref;
class MCHandler;

class MCStatement
{
protected:
	uint2 line;
	uint2 pos;
	MCStatement *next;
public:
	MCStatement();
	
	virtual ~MCStatement();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext&);
	
	virtual uint4 linecount();
	
	void setnext(MCStatement *n)
	{
		next = n;
	}
	MCStatement *getnext()
	{
		return next;
	}
	uint4 countlines(MCStatement *stmp);
	void deletestatements(MCStatement *start);
	void deletetargets(MCChunk **targets);
	Parse_stat gettargets(MCScriptPoint &, MCChunk **targets, Boolean forset);
	Parse_stat getparams(MCScriptPoint &, MCParameter **params);
	Parse_stat getmods(MCScriptPoint &, uint2 &mstate);
	Parse_stat gettime(MCScriptPoint &sp, MCExpression **in, Functions &units);
	void initpoint(MCScriptPoint &);
	uint2 getline()
	{
		return line;
	}
	uint2 getpos()
	{
		return pos;
	}
};

#endif
