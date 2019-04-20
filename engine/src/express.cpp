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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "scriptpt.h"

#include "mcerror.h"
#include "param.h"
#include "object.h"
#include "debug.h"
#include "util.h"
#include "handler.h"
#include "parentscript.h"
#include "osspec.h"

#include "globals.h"

#include "statemnt.h"

////////////////////////////////////////////////////////////////////////////////

MCExpression::MCExpression()
{
	rank = FR_VALUE;
	line = pos = 0;
	root = NULL;
	left = NULL;
	right = NULL;
}

MCExpression::~MCExpression()
{
	delete left;
	delete right;
}

MCVarref *MCExpression::getrootvarref(void)
{
	return NULL;
}

bool MCExpression::evalcontainer(MCExecContext& ctxt, MCContainer& r_container)
{
    return false;
}

Parse_stat MCExpression::getexps(MCScriptPoint &sp, MCExpression *earray[],
                                 uint2 &ecount)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_LPAREN) != PS_NORMAL)
	{
		MCperror->add
		(PE_FACTOR_NOLPAREN, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_RPAREN) != PS_NORMAL)
	{
		while (True)
		{
			MCExpression *newexp = NULL;
			if (sp.parseexp(False, False, &newexp) != PS_NORMAL)
			{
				delete newexp;
				MCperror->add
				(PE_FACTOR_BADPARAM, sp);
				return PS_ERROR;
			}
			earray[ecount++] = newexp;
			if (sp.skip_token(SP_FACTOR, TT_RPAREN) == PS_NORMAL)
				break;
			Symbol_type type;
			if (ecount >= MAX_EXP || sp.next(type) != PS_NORMAL)
			{
				MCperror->add
				(PE_FACTOR_NORPAREN, sp);
				return PS_ERROR;
			}
			if (type != ST_SEP)
			{
				MCperror->add
				(PE_FACTOR_NOTSEP, sp);
				return PS_ERROR;
			}
		}
	}
	return PS_NORMAL;
}

void MCExpression::freeexps(MCExpression *earray[], uint2 ecount)
{
	while (ecount--)
		delete earray[ecount];
}

Parse_stat MCExpression::get0params(MCScriptPoint &sp)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 0)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Parse_stat MCExpression::gettheparam(MCScriptPoint& sp, Boolean single, MCExpression** exp)
{
    initpoint(sp);
    if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
	{
		if (!single)
			return PS_NORMAL;
		else
		{
			MCperror->add(PE_FACTOR_NOOF, sp);
			return PS_ERROR;
		}
	}
    if (sp.parseexp(single, False, exp) != PS_NORMAL)
    {
        MCperror->add
        (PE_FACTOR_BADPARAM, sp);
        return PS_ERROR;
    }
	return PS_NORMAL;
}

Parse_stat MCExpression::get0or1param(MCScriptPoint &sp, MCExpression **exp,
                                      Boolean the)
{
	if (the)
	{
        return gettheparam(sp, False, exp);
	}

    MCExpression *earray[MAX_EXP];
    uint2 ecount = 0;
    if (getexps(sp, earray, ecount) != PS_NORMAL || ecount > 1)
    {
        freeexps(earray, ecount);
        return PS_ERROR;
    }
    
    if (ecount == 1)
    {
        *exp = earray[0];
	}
    
	return PS_NORMAL;
}

Parse_stat MCExpression::get1param(MCScriptPoint &sp, MCExpression **exp,
                                   Boolean the)
{
	if (the)
	{
        return gettheparam(sp, True, exp);
	}
    
    MCExpression *earray[MAX_EXP];
    uint2 ecount = 0;
    if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 1)
    {
        freeexps(earray, ecount);
        return PS_ERROR;
    }
    
    *exp = earray[0];
	
    return PS_NORMAL;
}

Parse_stat MCExpression::get0or1or2params(MCScriptPoint &sp, MCExpression **exp1,
                                            MCExpression **exp2, Boolean the)
{
	if (the)
	{
        return gettheparam(sp, False, exp1);
	}
    
    MCExpression *earray[MAX_EXP];
    uint2 ecount = 0;
    if (getexps(sp, earray, ecount) != PS_NORMAL || ecount > 2)
    {
        freeexps(earray, ecount);
        return PS_ERROR;
    }
    
    if (ecount > 0)
    {
        *exp1 = earray[0];
        if (ecount > 1)
        {
            *exp2 = earray[1];
        }
    }
    
	return PS_NORMAL;
}

Parse_stat MCExpression::get1or2params(MCScriptPoint &sp, MCExpression **exp1,
                                       MCExpression **exp2, Boolean the)
{
	if (the)
	{
        return gettheparam(sp, True, exp1);
	}
    
    MCExpression *earray[MAX_EXP];
    uint2 ecount = 0;
    if (getexps(sp, earray, ecount) != PS_NORMAL || ecount < 1 || ecount > 2)
    {
        freeexps(earray, ecount);
        return PS_ERROR;
    }
    
    *exp1 = earray[0];
    
    if (ecount == 2)
    {
        *exp2 = earray[1];
    }
    
	return PS_NORMAL;
}

Parse_stat MCExpression::get2params(MCScriptPoint &sp, MCExpression **exp1,
                                    MCExpression **exp2)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 2)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	return PS_NORMAL;
}

Parse_stat MCExpression::get2or3params(MCScriptPoint &sp, MCExpression **exp1,
                                       MCExpression **exp2, MCExpression **exp3)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount < 2 || ecount > 3)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	if (ecount == 3)
		*exp3 = earray[2];
	return PS_NORMAL;
}

Parse_stat MCExpression::get3params(MCScriptPoint &sp, MCExpression **exp1,
                                    MCExpression **exp2, MCExpression **exp3)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 3)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	*exp3 = earray[2];
	return PS_NORMAL;
}

Parse_stat MCExpression::get4or5params(MCScriptPoint &sp, MCExpression **exp1,
                                       MCExpression **exp2, MCExpression **exp3,
                                       MCExpression **exp4, MCExpression **exp5)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount < 4 || ecount > 5)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	*exp3 = earray[2];
	*exp4 = earray[3];
	if (ecount == 5)
		*exp5 = earray[4];
	return PS_NORMAL;
}

Parse_stat MCExpression::get6params(MCScriptPoint &sp, MCExpression **exp1,
                                    MCExpression **exp2, MCExpression **exp3,
                                    MCExpression **exp4, MCExpression **exp5,
                                    MCExpression **exp6)
{
	MCExpression *earray[MAX_EXP];
	uint2 ecount = 0;
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount != 6)
	{
		freeexps(earray, ecount);
		return PS_ERROR;
	}
	*exp1 = earray[0];
	*exp2 = earray[1];
	*exp3 = earray[2];
	*exp4 = earray[3];
	*exp5 = earray[4];
	*exp6 = earray[5];
	return PS_NORMAL;
}

Parse_stat MCExpression::getvariableparams(MCScriptPoint &sp, uint32_t p_min_params, uint32_t p_param_count, ...)
{
	Parse_stat t_stat = PS_NORMAL;
	
	MCExpression *earray[MAX_EXP];
	uint16_t ecount = 0;
	
	va_list t_params;
	va_start(t_params, p_param_count);
	
	if (getexps(sp, earray, ecount) != PS_NORMAL || ecount < p_min_params || ecount > p_param_count)
	{
		freeexps(earray, ecount);
		t_stat = PS_ERROR;
	}
	else
	{
		for (uint32_t i = 0; i < p_param_count; i++)
			*(va_arg(t_params, MCExpression **)) = (i < ecount) ? earray[i] : NULL;
	}

	va_end(t_params);
	
	return t_stat;
}

Parse_stat MCExpression::getparams(MCScriptPoint &sp, MCParameter **params)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_LPAREN) != PS_NORMAL)
	{
		MCperror->add
		(PE_FACTOR_NOLPAREN, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_RPAREN) == PS_NORMAL)
		return PS_NORMAL;
	MCParameter *pptr = NULL;
	while (True)
	{
		if (pptr == NULL)
			*params = pptr = new (nothrow) MCParameter;
		else
		{
			pptr->setnext(new MCParameter);
			pptr = pptr->getnext();
		}
		if (pptr->parse(sp) != PS_NORMAL)
		{
			MCperror->add
			(PE_FACTOR_BADPARAM, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_FACTOR, TT_RPAREN) == PS_NORMAL)
			break;
		Symbol_type type;
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add
			(PE_FACTOR_NORPAREN, sp);
			return PS_ERROR;
		}
		if (type != ST_SEP)
		{
			MCperror->add
			(PE_FACTOR_NOTSEP, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_FACTOR, TT_RPAREN) == PS_NORMAL)
		{
			pptr->setnext(new MCParameter);
			break;
		}
	}
	return PS_NORMAL;
}

Parse_stat MCExpression::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	return PS_NORMAL;
}

void MCExpression::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    fprintf(stderr, "ERROR: eval method for expression not implemented properly\n");
    abort();
}

void MCExpression::eval_typed(MCExecContext& ctxt, MCExecValueType p_type, void *r_value)
{
	MCExecValue t_value;
	eval_ctxt(ctxt, t_value);
	if (!ctxt . HasError())
		MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , p_type, r_value);
}


void MCExpression::initpoint(MCScriptPoint &sp)
{
	line = sp.getline();
	pos = sp.getpos();
}

////////////////////////////////////////////////////////////////////////////////
