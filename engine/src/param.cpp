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

#ifdef LEGACY_EXEC
bool MCParameter::setoldstring_argument(const MCString& p_string)
{
	MCStringRef t_string_ref;
	if (!MCStringCreateWithNativeChars((const char_t *)p_string . getstring(), p_string . getlength(), t_string_ref))
		return false;
	MCValueRelease(value);
	value = t_string_ref;
	return true;
}
#endif

void MCParameter::setvalueref_argument(MCValueRef p_value)
{
	MCValueRef t_value;
	t_value = MCValueRetain(p_value);

	MCExecTypeRelease(value);
    MCExecTypeSetValueRef(value, t_value);
}

void MCParameter::give_exec_argument(MCExecValue p_value)
{
    if (MCExecTypeIsValueRef(p_value . type))
    {
        setvalueref_argument(p_value . valueref_value);
        // SN-2014-07-09: [[ MemoryLeak ]] give_exec_argument should give the ExecValue, not copy it
        MCExecTypeRelease(p_value);
    }
    else
    {
        MCExecTypeRelease(value);
        value = p_value;
    }
}

// Converts the exec value to a valueref
MCValueRef MCParameter::getvalueref_argument(void)
{
    MCExecContext ctxt(nil, nil, nil);
    MCExecTypeConvertAndReleaseAlways(ctxt, value . type, &value, kMCExecValueTypeValueRef, &value);
    
	return value . valueref_value;
}

void MCParameter::setn_argument(real8 p_number)
{
    MCExecTypeRelease(value);
    value . type  = kMCExecValueTypeDouble;
    value . double_value = p_number;
}

void MCParameter::setrect_argument(MCRectangle p_rect)
{
    MCExecTypeRelease(value);
    value . type  = kMCExecValueTypeRectangle;
    value . rectangle_value = p_rect;
}

void MCParameter::clear_argument(void)
{
    // AL-2014-09-17: [[ Bug 13465 ]] Delete container when clearing a parameter
    delete container;
    container = nil;
    var  = nil;
	MCExecTypeRelease(value);
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
    if (value . type != kMCExecValueTypeNone)
    {
        if (MCExecTypeIsValueRef(value . type))
            return MCValueCopy(value . valueref_value, r_value);
        else
        {
            MCExecValue t_value;
            MCExecTypeCopy(value, t_value);
            MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, t_value . type, &t_value, r_value);
            return true;
        }
    }
    else
        return ctxt . EvalOptionalExprAsValueRef(exp, (MCValueRef)kMCEmptyString, EE_PARAM_BADEXP, r_value);
}

bool MCParameter::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    if (value . type != kMCExecValueTypeNone)
        MCExecTypeCopy(value, r_value);
    else
    {        
        if (exp != nil)
        {
            if (!ctxt . EvaluateExpression(exp, EE_PARAM_BADEXP, r_value))
                return false;
        }
        else
            MCExecTypeSetValueRef(r_value, MCValueRetain(kMCEmptyString));
    }
    
    return true;
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

    // AL-2014-08-28: [[ ArrayElementRefParams ]] MCParameter argument can now be a container
    if (container != nil)
        return container -> eval(ctxt, r_value);
    
    if (value . type == kMCExecValueTypeNone)
        return r_value = MCValueRetain(kMCEmptyString), true;

    MCValueRef t_value;
    if (MCExecTypeIsValueRef(value . type))
    {
        if (!MCValueCopy(value . valueref_value, t_value))
            return false;
    }
    else
    {
        MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, value . type, &value, t_value);
    }
    

    r_value = t_value;
    return true;
}

bool MCParameter::eval_argument_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    if (var != NULL)
        return var -> eval_ctxt(ctxt, r_value);
    
    if (container != nil)
        return container -> eval_ctxt(ctxt, r_value);
    
    MCExecTypeCopy(value, r_value);
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

MCContainer *MCParameter::eval_argument_container(void)
{
	return container;
}

/////////

void MCParameter::set_argument(MCExecContext& ctxt, MCValueRef p_value)
{
    MCValueRef t_new_value;
    t_new_value = MCValueRetain(p_value);
    MCExecTypeRelease(value);
    MCExecTypeSetValueRef(value, t_new_value);
	var = NULL;
}

void MCParameter::set_exec_argument(MCExecContext& ctxt, MCExecValue p_value)
{
	MCExecValue t_old_value;
	t_old_value = value;
    MCExecTypeCopy(p_value, value);
	MCExecTypeRelease(t_old_value);
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

void MCParameter::set_argument_container(MCContainer* p_container)
{
	container = p_container;
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
