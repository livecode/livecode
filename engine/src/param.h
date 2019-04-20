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
        container = nil;
		value . type = kMCExecValueTypeNone;
        value . valueref_value = nil;
	}

	~MCParameter(void)
	{
		delete exp;
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

	void setvalueref_argument(MCValueRef name);
	void give_exec_argument(MCExecValue name);
	void setn_argument(real8 n);
    void setrect_argument(MCRectangle rect);
	void clear_argument(void);
	
	// MW-2014-01-22: [[ CompatV1 ]] Used by the V1 interface to get the arg value.
	MCValueRef getvalueref_argument(void);

	// Evaluate the value *stored* in the parameter - i.e. Used by
    // the callee in a function/command invocation.
    MCContainer *eval_argument_container(void);
    
    bool eval_argument(MCExecContext& ctxt, MCValueRef &r_value);
    bool eval_argument_ctxt(MCExecContext& ctxt, MCExecValue &r_value);

	// Set the value of the parameter to be used by the callee.
    void set_argument(MCExecContext &ctxt, MCValueRef p_value);
    void set_exec_argument(MCExecContext& ctxt, MCExecValue p_value);
    void set_argument_container(MCContainer* container);

	// Evaluate the value of the given parameter in the context of
	// <ep>.
    bool eval(MCExecContext& ctxt, MCValueRef &r_value);
    bool eval_ctxt(MCExecContext& ctxt, MCExecValue &r_value);
    bool evalcontainer(MCExecContext& ctxt, MCContainer& r_container);
	Parse_stat parse(MCScriptPoint &);
    
    /* Count the number of containers which is needed to evaluate the parameter
     * list. This is the number of expressions which have a root var-ref. */
    unsigned count_containers(void);

private:
	// Parameter as syntax (i.e. location of the expression
	// being passed to a function/command).
	uint2 line, pos;
	MCExpression *exp;
	
	// Linkage for the parameter list.
	MCParameter *next;

	// Parameter as value (i.e. value of the argument when
	// passed to a function/command).
    // Note: The container member is not owned by the parameter instance.
    MCContainer *container;
	MCExecValue value;
};

#endif
