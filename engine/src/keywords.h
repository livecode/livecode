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
    /* The switch condition expression */
    MCExpression *cond;
    /* If dynamic_ncases is 0 then static_cases is used, otherwise dynamic_cases
     * is used. */
    union
    {
        MCExpression **dynamic_cases;
        MCArrayRef static_cases;
    };
    /* The complete list of statements in the switch. */
    MCStatement *statements;
    /* The pointers, one for each case, to start executing from if non-default
     * case. */
	MCStatement **casestatements;
    /* The pointer to the statement to start executing from if default case. */
    MCStatement *defaultstatement;
    /* If dynamic_ncases is 0 then it is a static switch */
    uindex_t dynamic_ncases;
public:
	MCSwitch()
	{
		cond = NULL;
		dynamic_cases = NULL;
        statements = NULL;
		casestatements = NULL;
		defaultstatement = NULL;
        dynamic_ncases = 0;
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

#endif
