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

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "scriptpt.h"
//#include "execpt.h"
#include "param.h"
#include "mcerror.h"
#include "util.h"

#include "globals.h"
#include "syntax.h"

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
/////////

bool MCParameter::setoldstring_argument(const MCString& p_string)
{
	MCStringRef t_string_ref;
	if (!MCStringCreateWithNativeChars((const char_t *)p_string . getstring(), p_string . getlength(), t_string_ref))
		return false;
	MCValueRelease(value);
	value = t_string_ref;
	return true;
}

void MCParameter::setvalueref_argument(MCValueRef p_value)
{
	MCValueRef t_value;
	t_value = MCValueRetain(p_value);

	MCValueRelease(value);
	value = t_value;
}

void MCParameter::setn_argument(real8 p_number)
{
	MCNumberRef t_number_ref;
	/* UNCHECKED */ MCNumberCreateWithReal(p_number, t_number_ref);
	MCValueRelease(value);
	value = t_number_ref;
}

void MCParameter::clear_argument(void)
{
	MCValueRelease(value);
	value = nil;
}

////////

#ifdef LEGACY_EXEC
MCVariable *MCParameter::evalvar(MCExecPoint& ep)
{
	if (exp == NULL)
		return NULL;

    return exp -> evalvar(ep);
}
#endif

MCVariable *MCParameter::evalvar(MCExecContext &ctxt)
{
    if (exp == NULL)
        return NULL;

    return exp -> evalvar(ctxt);
}

#ifdef LEGACY_EXEC
Exec_stat MCParameter::eval(MCExecPoint& ep)
{
	if (value != nil || exp == nil)
		/* UNCHECKED */ ep . setvalueref_nullable(value);
	else if (exp -> eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_PARAM_BADEXP, line, pos);
		return ES_ERROR;
	}

	return ES_NORMAL;
}
#endif

bool MCParameter::eval(MCExecContext &ctxt, MCValueRef &r_value)
{
    if (value == nil)
        return ctxt . EvalOptionalExprAsValueRef(exp, (MCValueRef)kMCEmptyString, EE_PARAM_BADEXP, r_value);
    else
    {
        // ep.copyasvalueref() returns kMCEmptyString if there is no value in the
        // EP, so eval(MCExecPoint) formely never allowed us to get a nil value.
        r_value = MCValueRetain((MCValueRef) kMCEmptyString);
        return true;
    }
}

#ifdef LEGACY_EXEC
Exec_stat MCParameter::evalcontainer(MCExecPoint& ep, MCContainer*& r_container)
{
	if (exp == NULL)
		return ES_ERROR;

	return exp -> evalcontainer(ep, r_container);
}
#endif

bool MCParameter::evalcontainer(MCExecContext &ctxt, MCContainer *&r_container)
{
    if (exp == NULL)
        return false;

    return exp -> evalcontainer(ctxt, r_container);
}

bool MCParameter::eval_argument(MCExecContext &ctxt, MCValueRef &r_value)
{
    if (var != NULL)
        return var -> eval(ctxt, r_value);

    if (value == nil)
        return r_value = MCValueRetain(kMCEmptyString), true;

    MCValueRef t_value;
    if (!MCValueCopy(value, t_value))
        return false;

    r_value = t_value;
    return true;
}

#ifdef LEGACY_EXEC
Exec_stat MCParameter::eval_argument(MCExecPoint& ep)
{
	if (var != NULL)
		return var -> eval(ep);

	if (ep . setvalueref_nullable(value))
		return ES_NORMAL;

	return ES_ERROR;
}
#endif

MCVariable *MCParameter::eval_argument_var(void)
{
	return var;
}

/////////

void MCParameter::set_argument(MCExecContext& ctxt, MCValueRef p_value)
{
	MCValueRef t_old_value;
	t_old_value = value;
    value = MCValueRetain(p_value);
	MCValueRelease(t_old_value);
	var = NULL;
}

#ifdef LEGACY_EXEC
void MCParameter::set_argument(MCExecPoint& ep)
{
	MCValueRef t_old_value;
	t_old_value = value;
	/* UNCHECKED */ ep . copyasvalueref(value);
	MCValueRelease(t_old_value);
	var = NULL;
}
#endif

void MCParameter::set_argument_var(MCVariable* p_var)
{
	var = p_var;
}

////////////////////////////////////////////////////////////////////////////////

void MCParameter::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCParameter::compile_in(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCParameter::compile_out(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCParameter::compile_inout(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}
