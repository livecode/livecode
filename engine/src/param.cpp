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

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "scriptpt.h"
#include "execpt.h"
#include "param.h"
#include "mcerror.h"
#include "util.h"

#include "globals.h"

Parse_stat MCParameter::parse(MCScriptPoint &sp)
{
	line = sp.getline();
	pos = sp.getpos();
	if (sp.parseexp(False, False, &exp) != PS_NORMAL)
	{
		MCperror->add(PE_PARAM_BADEXP, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

////////

MCVariable *MCParameter::evalvar(MCExecPoint& ep)
{
	if (exp == NULL)
		return NULL;

	return exp -> evalvar(ep);
}

Exec_stat MCParameter::eval(MCExecPoint& ep)
{
	if (!value . is_undefined() || exp == NULL)
	{
		switch(value . get_format())
		{
		case VF_STRING:
			ep . setsvalue(value . get_string());
		break;
		case VF_NUMBER:
			ep . setnvalue(value . get_real());
		break;
		case VF_BOTH:
			ep . setboth(value . get_string(), value . get_real());
		break;
		case VF_ARRAY:
			ep . setarray(&value, False);
		break;
		default:
			ep . clear();
		break;
		}
	}
	else if (exp -> eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_PARAM_BADEXP, line, pos);
		return ES_ERROR;
	}

	return ES_NORMAL;
}

Exec_stat MCParameter::evalcontainer(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_var_value)
{
	if (exp == NULL)
		return ES_ERROR;

	return exp -> evalcontainer(ep, r_var, r_var_value);
}

Exec_stat MCParameter::eval_argument(MCExecPoint& ep)
{
	if (var != NULL)
		return var -> fetch(ep);

	return value . fetch(ep);
}

MCVariable *MCParameter::eval_argument_var(void)
{
	return var;
}

/////////

void MCParameter::set_argument(MCExecPoint& ep)
{
	switch(ep . getformat())
	{
	case VF_UNDEFINED:
		value . clear();
	break;

	case VF_STRING:
		if (ep . usingbuffer())
			value . assign_string(ep . getsvalue());
		else
			value . assign_constant_string(ep . getsvalue());
	break;

	case VF_NUMBER:
		value . assign_real(ep . getnvalue());
	break;

	case VF_BOTH:
		// MW-2008-06-30: [[ Bug ]] Make sure we set 'both', previously the string part
		//   and then numeric part was being set which caused uninitialized parameters to
		//   appear as 0.
		if (ep . usingbuffer())
			value . assign_both(ep . getsvalue(), ep . getnvalue());
		else
			value . assign_constant_both(ep . getsvalue(), ep . getnvalue());
	break;

	case VF_ARRAY:
		value . assign(*(ep . getarray()));
	break;
	}

	var = NULL;
}

void MCParameter::set_argument_var(MCVariable* p_var)
{
	var = p_var;
}
