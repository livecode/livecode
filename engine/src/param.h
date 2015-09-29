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
		exp = NULL;
		next = NULL;
		var = NULL;
	}

	MCParameter(const MCString& s)
	{
		exp = NULL;
		next = NULL;
		var = NULL;
		value . assign_constant_string(s);
	}

	~MCParameter(void)
	{
		delete exp;
	}

	void setnext(MCParameter *n)
	{
		next = n;
	}

	MCParameter *getnext(void)
	{
		return next;
	}

	void sets_argument(const MCString& s)
	{
		value . assign_constant_string(s);
	}
	
	void copysvalue_argument(const MCString& s)
	{
		value . assign_string(s);
	}

	void setnameref_argument(MCNameRef n)
	{
		value . assign_string(MCNameGetOldString(n));
	}

	void setnameref_unsafe_argument(MCNameRef n)
	{
		value . assign_constant_string(MCNameGetOldString(n));
	}

	void setn_argument(real8 n)
	{
		value . assign_real(n);
	}

	void clear_argument(void)
	{
		value . clear();
	}

	void setbuffer(char *buffer, uint4 length)
	{
		value . assign_buffer(buffer, length);
	}

	// MW-2009-06-26: Returns a mutable reference to the parameters
	//   value object.
	MCVariableValue& getvalue(void)
	{
		return value;
	}

	// Evaluate the value *stored* in the parameter - i.e. Used by
	// the callee in a function/command invocation.
	Exec_stat eval_argument(MCExecPoint& ep);
	MCVariable *eval_argument_var(void);

	// Set the value of the parameter to be used by the callee.
	void set_argument(MCExecPoint& ep);
	void set_argument_var(MCVariable* var);

	// Evaluate the value of the given parameter in the context of
	// <ep>.
	Exec_stat eval(MCExecPoint& ep);
	Exec_stat evalcontainer(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_var_value);
	MCVariable *evalvar(MCExecPoint& ep);

	Parse_stat parse(MCScriptPoint &);

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
	MCVariableValue value;
};

#endif
