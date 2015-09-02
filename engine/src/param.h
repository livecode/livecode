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
// MCParameter class declarations
//
#ifndef	PARAMETER_H
#define	PARAMETER_H

#include "express.h"
#include "variable.h"

class MCParameter
{
public:
	MCParameter(void)
	{
		exp = nil;
		next = nil;
		var = nil;
        container = nil;
		value . type = kMCExecValueTypeNone;
        value . valueref_value = nil;
	}

	~MCParameter(void)
	{
		delete exp;
        // AL-2014-09-17: [[ Bug 13465 ]] Delete container when parameter is deleted
        delete container;
		MCExecTypeRelease(value);
	}

	void setnext(MCParameter *n)
	{
		next = n;
	}

	MCParameter *getnext(void)
	{
		return next;
	}

#ifdef LEGACY_EXEC
	void sets_argument(const MCString& string)
	{
		/* UNCHECKED */ setoldstring_argument(string);
	}

	void copysvalue_argument(const MCString& string)
	{
		/* UNCHECKED */ setoldstring_argument(string);
	}
	
	bool setoldstring_argument(const MCString& string);
#endif

	void setvalueref_argument(MCValueRef name);
	void give_exec_argument(MCExecValue name);
	void setn_argument(real8 n);
	void clear_argument(void);
	
	// MW-2014-01-22: [[ CompatV1 ]] Used by the V1 interface to get the arg value.
	MCValueRef getvalueref_argument(void);

	// Evaluate the value *stored* in the parameter - i.e. Used by
    // the callee in a function/command invocation.
#ifdef LEGACY_EXEC
    Exec_stat eval_argument(MCExecPoint& ep);
#endif
    MCVariable *eval_argument_var(void);
    MCContainer *eval_argument_container(void);
    
    bool eval_argument(MCExecContext& ctxt, MCValueRef &r_value);
    bool eval_argument_ctxt(MCExecContext& ctxt, MCExecValue &r_value);

	// Set the value of the parameter to be used by the callee.
    void set_argument(MCExecContext &ctxt, MCValueRef p_value);
    void set_exec_argument(MCExecContext& ctxt, MCExecValue p_value);
#ifdef LEGACY_EXEC
	void set_argument(MCExecPoint& ep);
#endif
	void set_argument_var(MCVariable* var);
    void set_argument_container(MCContainer* container);

	// Evaluate the value of the given parameter in the context of
	// <ep>.
#ifdef LEGACY_EXEC
	Exec_stat eval(MCExecPoint& ep);

	Exec_stat evalcontainer(MCExecPoint& ep, MCContainer*& r_container);
	MCVariable *evalvar(MCExecPoint& ep);
#endif

    bool eval(MCExecContext& ctxt, MCValueRef &r_value);
    bool eval_ctxt(MCExecContext& ctxt, MCExecValue &r_value);
    bool evalcontainer(MCExecContext& ctxt, MCContainer*& r_container);
    MCVariable *evalvar(MCExecContext& ctxt);

	Parse_stat parse(MCScriptPoint &);

	void compile(MCSyntaxFactoryRef);
	void compile_in(MCSyntaxFactoryRef);
	void compile_out(MCSyntaxFactoryRef);
	void compile_inout(MCSyntaxFactoryRef);

private:
	// Parameter as syntax (i.e. location of the expression
	// being passed to a function/command).
	uint2 line, pos;
	MCExpression *exp;
	
	// Linkage for the parameter list.
	MCParameter *next;

	// Parameter as value (i.e. value of the argument when
	// passed to a function/command).
	MCVariable *var;
    MCContainer *container;
	MCExecValue value;
};

#endif
