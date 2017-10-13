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
/////////

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
	MCExecTypeRelease(value);
}

////////

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

bool MCParameter::evalcontainer(MCExecContext &ctxt, MCContainer& r_container)
{
    if (exp == NULL)
        return false;

    return exp -> evalcontainer(ctxt, r_container);
}

bool MCParameter::eval_argument(MCExecContext &ctxt, MCValueRef &r_value)
{
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
    if (container != nil)
        return container -> eval_ctxt(ctxt, r_value);
    
    MCExecTypeCopy(value, r_value);
    return true;
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
}

void MCParameter::set_exec_argument(MCExecContext& ctxt, MCExecValue p_value)
{
	MCExecValue t_old_value;
	t_old_value = value;
    MCExecTypeCopy(p_value, value);
	MCExecTypeRelease(t_old_value);
}

void MCParameter::set_argument_container(MCContainer* p_container)
{
	container = p_container;
}

////////////////////////////////////////////////////////////////////////////////
