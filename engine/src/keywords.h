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
// Keyword class declarations
//
#ifndef	KEYWORDS_H
#define	KEYWORDS_H

#include "statemnt.h"
#include "express.h"

class MCScriptPoint;
class MCExpression;

class MCGlobal : public MCStatement
{
public:
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt)
    {
	}
	virtual uint4 linecount()
	{
		return 0;
	}
};

class MCLocaltoken : public MCStatement
{
protected:
	Boolean constant;
public:
	virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext &ctxt)
    {
	}
	virtual uint4 linecount()
	{
		return 0;
	}
};

class MCLocalVariable : public MCLocaltoken
{
public:
	MCLocalVariable()
	{
		constant = False;
	}
};

class MCLocalConstant : public MCLocaltoken
{
public:
	MCLocalConstant()
	{
		constant = True;
	}
};

class MCIf : public MCStatement
{
	MCExpression *cond;
	MCStatement *thenstatements;
	MCStatement *elsestatements;
public:
	MCIf()
	{
		cond = NULL;
		thenstatements = NULL;
		elsestatements = NULL;
	}
	~MCIf();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext &ctxt);
	virtual uint4 linecount();
};

class MCRepeat : public MCStatement
{
	Repeat_form form;
	MCExpression *startcond;
	MCExpression *endcond;
	MCVarref *loopvar;
	real8 stepval;
	MCExpression *step;
	MCStatement *statements;
	File_unit each;
public:
	MCRepeat();
	~MCRepeat();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext&);
	virtual uint4 linecount();
};

class MCExit : public MCStatement
{
	Exec_stat exit;
public:
	virtual Parse_stat parse(MCScriptPoint &sp);
	virtual void exec_ctxt(MCExecContext&);
	virtual uint4 linecount();
};

class MCNext : public MCStatement
{
public:
	virtual Parse_stat parse(MCScriptPoint &sp);
	virtual void exec_ctxt(MCExecContext&);
	virtual uint4 linecount();
};

class MCPass : public MCStatement
{
	Boolean all;
public:
	MCPass()
	{
		all = False;
	}
	virtual Parse_stat parse(MCScriptPoint &sp);
	virtual void exec_ctxt(MCExecContext&);
	virtual uint4 linecount();
};

class MCBreak : public MCStatement
{
public:
	virtual void exec_ctxt(MCExecContext&);
	virtual uint4 linecount();
};

class MCSwitch : public MCStatement
{
	MCExpression *cond;
	MCExpression **cases;
	MCStatement *statements;
	uint2 *caseoffsets;
	int2 defaultcase;
	uint2 ncases;
public:
	MCSwitch()
	{
		cond = NULL;
		cases = NULL;
		statements = NULL;
		defaultcase = -1;
		caseoffsets = NULL;
		ncases = 0;
	}
	~MCSwitch();
	virtual Parse_stat parse(MCScriptPoint &sp);
	virtual void exec_ctxt(MCExecContext &);
	virtual uint4 linecount();
};

class MCThrowKeyword : public MCStatement
{
	MCExpression *error;
public:
	MCThrowKeyword()
	{
		error = NULL;
	}
	~MCThrowKeyword();
	virtual Parse_stat parse(MCScriptPoint &sp);
	virtual void exec_ctxt(MCExecContext &);
	virtual uint4 linecount();
};

class MCTry : public MCStatement
{
	MCStatement *trystatements;
	MCStatement *catchstatements;
	MCStatement *finallystatements;
	MCVarref *errorvar;
public:
	MCTry()
	{
		trystatements = catchstatements = finallystatements = NULL;
		errorvar = NULL;
	}
	~MCTry();
	virtual Parse_stat parse(MCScriptPoint &);
	virtual void exec_ctxt(MCExecContext&);
	virtual uint4 linecount();
};

////////////////////////////////////////////////////////////////////////////////

class MCHandref
{
    MCNewAutoNameRef name;
    MCParameter *params;
    MCHandler *handler;
    struct
    {
        unsigned container_count : 16;
        bool resolved : 1;
        bool global_handler : 1;
    };

public:
    MCHandref(MCNameRef name);
    ~MCHandref(void);
    
    MCParameter** getparams(void) { return &params; }
    
    void parse(void);
    void exec(MCExecContext& ctxt, uint2 line, uint2 pos, bool is_function);
};

class MCComref : public MCStatement
{
    MCHandref command;
public:
    MCComref(MCNameRef n);
    virtual ~MCComref();
    virtual Parse_stat parse(MCScriptPoint &);
    virtual void exec_ctxt(MCExecContext&);
};

class MCFuncref : public MCExpression
{
    MCHandref function;
public:
    MCFuncref(MCNameRef);
    virtual ~MCFuncref();
    virtual Parse_stat parse(MCScriptPoint &, Boolean the);
    void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
};

////////////////////////////////////////////////////////////////////////////////

#endif
