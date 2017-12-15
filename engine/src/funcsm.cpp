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


#include "hndlrlst.h"
#include "scriptpt.h"
#include "handler.h"
#include "param.h"
#include "funcs.h"
#include "object.h"
#include "field.h"
#include "stack.h"
#include "mcerror.h"
#include "util.h"
#include "osspec.h"

#include "globals.h"
#include "exec.h"

bool MCParamFunction::params_to_doubles(MCExecContext& ctxt, real64_t *&r_doubles, uindex_t &r_count)
{
	MCAutoArray<real64_t> t_list;
	real64_t t_number;

    if (params != NULL && params->getnext() == NULL)
	{
        MCAutoValueRef t_paramvalue;
        if (!params->eval(ctxt, &t_paramvalue))
		{
            ctxt . LegacyThrow(EE_FUNCTION_BADSOURCE);
            return false;
		}

        if (MCValueIsArray(*t_paramvalue))
		{
            MCArrayRef t_array = (MCArrayRef)*t_paramvalue;
			MCNameRef t_key;
			MCValueRef t_value;
			uintptr_t t_index = 0;

            while (MCArrayIterate(t_array, t_index, t_key, t_value))
			{
                if (!ctxt . ConvertToReal(t_value, t_number))
				{
                    ctxt . LegacyThrow(EE_FUNCTION_BADSOURCE);
                    return false;
				}

				if (!t_list.Push(t_number))
                    return false;
			}
		}
        else
		{
            MCAutoStringRef t_string;
            MCAutoArrayRef t_array;

            if (!ctxt . ConvertToString(*t_paramvalue, &t_string)
                    || !MCStringSplit(*t_string, MCSTR(","), nil, kMCStringOptionCompareExact, &t_array))
            {
                ctxt . LegacyThrow(EE_FUNCTION_BADSOURCE);
                return false;
            }

            for (uinteger_t i = 1 ; i <= MCArrayGetCount(*t_array) ; ++i)
            {
                MCValueRef t_value;
                /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_array, i, t_value);

                if (MCValueIsEmpty(t_value))
                    t_number = 0.0;
                else
                {
                    if (!ctxt . ConvertToReal(t_value, t_number))
                    {
                        ctxt . LegacyThrow(EE_FUNCTION_BADSOURCE);
                        return false;
                    }
                }

                if (!t_list.Push(t_number))
                    return false;
			}
		}
	}
	else
	{
        MCParameter *t_param = params;
        bool t_success;
        t_success = true;
		while (t_param != NULL)
		{
            MCExecValue t_value;
            real64_t t_double;
            
            t_success = t_param->eval_ctxt(ctxt, t_value);
            
            if (t_success)
            {
                MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeDouble, &t_double);
                t_success = !ctxt . HasError();
            }            
            
            if (!t_success)
			{
                ctxt . LegacyThrow(EE_FUNCTION_BADSOURCE);
                return false;
			}
            
            if (!t_list.Push(t_double))
                return false;
            
			t_param = t_param->getnext();
		}
	}
    
	t_list.Take(r_doubles, r_count);
    return true;
}

MCAnnuity::~MCAnnuity()
{
	delete rate;
	delete periods;
}

Parse_stat MCAnnuity::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get2params(sp, &rate, &periods) != PS_NORMAL)
	{
		MCperror->add(PE_ANNUITY_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCAnnuity::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    real64_t t_rate;
    if (!ctxt . EvalExprAsDouble(rate, EE_ANNUITY_BADRATE, t_rate))
        return;
        
	real64_t t_periods;
    if (!ctxt . EvalExprAsDouble(periods, EE_ANNUITY_BADPERIODS, t_periods))
        return;
	

	MCMathEvalAnnuity(ctxt, t_rate, t_periods, r_value . double_value);
    r_value .type = kMCExecValueTypeDouble;
}

MCAtan2::~MCAtan2()
{
	delete s1;
	delete s2;
}

Parse_stat MCAtan2::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get2params(sp, &s1, &s2) != PS_NORMAL)
	{
		MCperror->add(PE_ATAN2_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCAtan2::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    real64_t t_y;
    if (!ctxt . EvalExprAsDouble(s1, EE_ATAN2_BADS1, t_y))
        return;
	

    real64_t t_x;
    if (!ctxt . EvalExprAsDouble(s2, EE_ATAN2_BADS2, t_x))
        return;
	
	MCMathEvalAtan2(ctxt, t_y, t_x, r_value . double_value);
    r_value . type = kMCExecValueTypeDouble;
}

MCCompound::~MCCompound()
{
	delete rate;
	delete periods;
}

Parse_stat MCCompound::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get2params(sp, &rate, &periods) != PS_NORMAL)
	{
		MCperror->add(PE_COMPOUND_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCCompound::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    real64_t t_rate;
    if (!ctxt . EvalExprAsDouble(rate, EE_COMPOUND_BADRATE, t_rate))
        return;
	
    real64_t t_periods;
    if (!ctxt . EvalExprAsDouble(periods, EE_COMPOUND_BADRATE, t_periods))
        return;
	

	MCMathEvalCompound(ctxt, t_rate, t_periods, r_value . double_value);
    r_value . type = kMCExecValueTypeDouble;
}

MCMatrixMultiply::~MCMatrixMultiply()
{
	delete source;
	delete dest;
}

Parse_stat MCMatrixMultiply::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	if (get2params(sp, &dest, &source) != PS_NORMAL)
	{
		MCperror->add(PE_MATRIXMULT_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCMatrixMultiply::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	
    MCAutoArrayRef t_src_array;
    if (!ctxt . EvalExprAsArrayRef(dest, EE_MATRIXMULT_BADSOURCE, &t_src_array))
        return;
	
    MCAutoArrayRef t_dst_array;
    if (!ctxt . EvalExprAsArrayRef(source, EE_MATRIXMULT_BADSOURCE, &t_dst_array))
        return;

    MCArraysEvalMatrixMultiply(ctxt, *t_src_array, *t_dst_array, r_value . arrayref_value);
    r_value . type = kMCExecValueTypeArrayRef;
}

// MW-2007-07-03: [[ Bug 4506 ]] - Large integers result in negative numbers
//   being generated.

MCRound::~MCRound()
{
	delete source;
	delete digit;
}

Parse_stat MCRound::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1or2params(sp, &source, &digit, the) != PS_NORMAL)
	{
		MCperror->add(PE_ROUND_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCRound::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    real64_t t_source;
    if (!ctxt . EvalExprAsDouble(source, EE_RANDOM_BADSOURCE, t_source))
        return;

    real64_t t_digit;
    if (!ctxt . EvalOptionalExprAsDouble(digit, 0, EE_RANDOM_BADSOURCE, t_digit))
        return;

	if (digit != nil)
		MCMathEvalRoundToPrecision(ctxt, t_source, t_digit, r_value . double_value);
	else
		MCMathEvalRound(ctxt, t_source, r_value . double_value);
    r_value . type = kMCExecValueTypeDouble;
}

MCStatRound::~MCStatRound()
{
	delete source;
	delete digit;
}

Parse_stat MCStatRound::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1or2params(sp, &source, &digit, the) != PS_NORMAL)
	{
		MCperror->add(PE_ROUND_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCStatRound::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    real64_t t_source;
    if (!ctxt . EvalExprAsDouble(source, EE_RANDOM_BADSOURCE, t_source))
        return;
	

    real64_t t_digit;
    if (!ctxt . EvalOptionalExprAsDouble(digit, 0, EE_RANDOM_BADSOURCE, t_digit))
        return;
    
    if (digit != nil)
		MCMathEvalStatRoundToPrecision(ctxt, t_source, t_digit, r_value . double_value);
	else
		MCMathEvalStatRound(ctxt, t_source, r_value . double_value);
    r_value . type = kMCExecValueTypeDouble;
}

MCVectorDotProduct::~MCVectorDotProduct()
{
    delete first;
    delete second;
}

Parse_stat MCVectorDotProduct::parse(MCScriptPoint &sp, Boolean the)
{
    initpoint(sp);
    if (get2params(sp, &first, &second) != PS_NORMAL)
    {
        MCperror->add(PE_VECTORDOT_BADPARAM, sp);
        return PS_ERROR;
    }
    return PS_NORMAL;
}

void MCVectorDotProduct::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCAutoArrayRef t_src_array;
    if (!ctxt . EvalExprAsArrayRef(first, EE_VECTORDOT_BADLEFT, &t_src_array))
        return;
    
    MCAutoArrayRef t_dst_array;
    if (!ctxt . EvalExprAsArrayRef(second, EE_VECTORDOT_BADRIGHT, &t_dst_array))
        return;
    
    MCArraysEvalVectorDotProduct(ctxt, *t_src_array, *t_dst_array, r_value . double_value);
    r_value . type = kMCExecValueTypeDouble;
}
