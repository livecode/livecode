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

//#include "execpt.h"
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
#include "md5.h"
#include "sha1.h"
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

#ifdef /* MCAbsFunction */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ABS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	ep.setnvalue(fabs(ep.getnvalue()));
	return ES_NORMAL;
#endif /* MCAbsFunction */


#ifdef /* MCAcos */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ACOS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(acos(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_ACOS_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCAcos */

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
#ifdef /* MCAnnuity */ LEGACY_EXEC
	if (rate->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ANNUITY_BADRATE, line, pos);
		return ES_ERROR;
	}
	real8 rn = ep.getnvalue();
	if (periods->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ANNUITY_BADPERIODS, line, pos);
		return ES_ERROR;
	}
	real8 pn = ep.getnvalue();
	ep.setnvalue((1.0 - pow(1.0 + rn, -pn)) / rn);
	return ES_NORMAL;
#endif /* MCAnnuity */

    real64_t t_rate;
    if (!ctxt . EvalExprAsDouble(rate, EE_ANNUITY_BADRATE, t_rate))
        return;
        
	real64_t t_periods;
    if (!ctxt . EvalExprAsDouble(periods, EE_ANNUITY_BADPERIODS, t_periods))
        return;
	

	MCMathEvalAnnuity(ctxt, t_rate, t_periods, r_value . double_value);
    r_value .type = kMCExecValueTypeDouble;
}

void MCAnnuity::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt, kMCMathEvalAnnuityMethodInfo, rate, periods);
}

#ifdef /* MCArithmeticMean */ LEGACY_EXEC
	if (evalparams(F_ARI_MEAN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_AVERAGE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCArithmeticMean */

#ifdef /* MCAsin */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ASIN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(asin(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_ASIN_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCAsin */


#ifdef /* MCAtan */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ATAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(atan(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_ATAN_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCAtan */


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
#ifdef /* MCAtan2 */ LEGACY_EXEC
	if (s1->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ATAN2_BADS1, line, pos);
		return ES_ERROR;
	}
	real8 n1 = ep.getnvalue();
	if (s2->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ATAN2_BADS2, line, pos);
		return ES_ERROR;
	}
	real8 n2 = ep.getnvalue();
	MCS_seterrno(0);
	ep.setnvalue(atan2(n1, n2));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_ATAN2_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCAtan2 */

    real64_t t_y;
    if (!ctxt . EvalExprAsDouble(s1, EE_ATAN2_BADS1, t_y))
        return;
	

    real64_t t_x;
    if (!ctxt . EvalExprAsDouble(s2, EE_ATAN2_BADS2, t_x))
        return;
	
	MCMathEvalAtan2(ctxt, t_y, t_x, r_value . double_value);
    r_value . type = kMCExecValueTypeDouble;
}

void MCAtan2::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt, kMCMathEvalAtan2MethodInfo, s1, s2);
}

#ifdef /* MCAverage */ LEGACY_EXEC

	if (evalparams(F_AVG_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_AVERAGE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCAverage */

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
#ifdef /* MCCompound */ LEGACY_EXEC
	if (rate->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_COMPOUND_BADRATE, line, pos);
		return ES_ERROR;
	}
	real8 rn = ep.getnvalue();
	if (periods->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_COMPOUND_BADPERIODS, line, pos);
		return ES_ERROR;
	}
	real8 pn = ep.getnvalue();
	ep.setnvalue(pow(1.0 + rn, pn));
	return ES_NORMAL;
#endif /* MCCompound */

    real64_t t_rate;
    if (!ctxt . EvalExprAsDouble(rate, EE_COMPOUND_BADRATE, t_rate))
        return;
	
    real64_t t_periods;
    if (!ctxt . EvalExprAsDouble(periods, EE_COMPOUND_BADRATE, t_periods))
        return;
	

	MCMathEvalCompound(ctxt, t_rate, t_periods, r_value . double_value);
    r_value . type = kMCExecValueTypeDouble;
}

void MCCompound::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt, kMCMathEvalCompoundMethodInfo, rate, periods);
}


#ifdef /* MCCos */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_COS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(cos(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_COS_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCCos */


#ifdef /* MCExp */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_EXP_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(exp(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_EXP_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCExp */


#ifdef /* MCExp1 */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_EXP1_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(exp(ep.getnvalue()) - 1.0);
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_EXP1_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCExp1 */


#ifdef /* MCExp2 */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_EXP2_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(pow(2.0, ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_EXP2_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCExp2 */


#ifdef /* MCExp10 */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_EXP10_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(pow(10.0, ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_EXP10_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCExp10 */

#ifdef /* MCGeometricMean */ LEGACY_EXEC
	if (evalparams(F_GEO_MEAN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_GEO_MEAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCGeometricMean */

#ifdef /* MCHarmonicMean */ LEGACY_EXEC
	if (evalparams(F_HAR_MEAN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_HAR_MEAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCHarmonicMean */

#ifdef /* MCLn */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_LN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(log(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_LN_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCLn */



#ifdef /* MCLn1 */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_LN1_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(log(1.0 + ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_LN1_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCLn1 */


#ifdef /* MCLog2 */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_LOG2_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(log(ep.getnvalue()) / log(2.0));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_LOG2_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCLog2 */




#ifdef /* MCLog10 */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_LOG10_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(log(ep.getnvalue()) / log(10.0));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_LOG10_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCLog10 */


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
#ifdef /* MCMatrixMultiply */ LEGACY_EXEC
	if (dest -> eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MATRIXMULT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	// MW-2014-03-14: [[ Bug 11924 ]] Make sure the dst is an array.
	MCVariableValue *t_dst_array;
	Boolean t_delete_dst_array;
	if (ep . isempty())
		t_dst_array = nil;
	else if (ep . getformat() == VF_ARRAY)
		ep . takearray(t_dst_array, t_delete_dst_array);
	else
	{
		MCeerror->add(EE_MATRIXMULT_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	if (source -> eval(ep) != ES_NORMAL)
	{
		if (t_delete_dst_array)
			delete t_dst_array;
        
		MCeerror->add(EE_MATRIXMULT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	
	// MW-2014-03-14: [[ Bug 11924 ]] Make sure the src is an array.
	MCVariableValue *t_src_array;
	Boolean t_delete_src_array;
	if (ep . isempty())
		t_src_array = nil;
	else if (ep . getformat() == VF_ARRAY)
		ep . takearray(t_src_array, t_delete_src_array);
	else
	{
		MCeerror->add(EE_MATRIXMULT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	
	// MW-2014-03-14: [[ Bug 11924 ]] If both are empty arrays, then the result
	//   is empty.
	if (t_src_array == nil && t_dst_array == nil)
	{
		ep . clear();
		return ES_NORMAL;
	}
	
	// MW-2014-03-14: [[ Bug 11924 ]] If either array is empty, then its a mismatch.
	MCVariableValue *v = new MCVariableValue();
	if ((t_src_array == nil || t_dst_array == nil) ||
		v->matrixmultiply(ep, *t_dst_array, *t_src_array) != ES_NORMAL)
	{
		MCeerror->add(EE_MATRIXMULT_MISMATCH, line, pos);
		
		if (t_delete_dst_array)
			delete t_dst_array;
        
		if (t_delete_src_array)
			delete t_src_array;
        
		delete v;
		return ES_ERROR;
	}
    
	ep.setarray(v, True);
    
	if (t_delete_dst_array)
		delete t_dst_array;
    
	if (t_delete_src_array)
		delete t_src_array;
    
	return ES_NORMAL;
#endif /* MCMatrixMultiply */
	
    MCAutoArrayRef t_src_array;
    if (!ctxt . EvalExprAsArrayRef(dest, EE_MATRIXMULT_BADSOURCE, &t_src_array))
        return;
	
    MCAutoArrayRef t_dst_array;
    if (!ctxt . EvalExprAsArrayRef(source, EE_MATRIXMULT_BADSOURCE, &t_dst_array))
        return;

    MCArraysEvalMatrixMultiply(ctxt, *t_src_array, *t_dst_array, r_value . arrayref_value);
    r_value . type = kMCExecValueTypeArrayRef;
}

void MCMatrixMultiply::compile(MCSyntaxFactoryRef ctxt)
{
	compile_with_args(ctxt, kMCArraysEvalMatrixMultiplyMethodInfo, dest, source);
}

#ifdef /* MCMaxFunction */ LEGACY_EXEC
	if (evalparams(F_MAX, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MAX_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCMaxFunction */

#ifdef /* MCMedian */ LEGACY_EXEC
	if (evalparams(F_MEDIAN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MEDIAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCMedian */

#ifdef /* MCMD5Digest */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MD5DIGEST_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	md5_state_t state;
	md5_byte_t digest[16];
	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)ep.getsvalue().getstring(), ep.getsvalue().getlength());
	md5_finish(&state, digest);
	ep.copysvalue((char *)digest, 16);
	return ES_NORMAL;
#endif /* MCMD5Digest */


#ifdef /* MCSHA1Digest */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_SHA1DIGEST_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	sha1_state_t state;
	uint8_t digest[20];
	sha1_init(&state);
	sha1_append(&state, ep.getsvalue().getstring(), ep.getsvalue().getlength());
	sha1_finish(&state, digest);
	ep.copysvalue((char *)digest, 20);
	return ES_NORMAL;
#endif /* MCSHA1Digest */

#ifdef /* MCMinFunction */ LEGACY_EXEC
	if (evalparams(F_MIN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MIN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCMinFunction */

#ifdef /* MCPopulationStdDev */ LEGACY_EXEC
	if (evalparams(F_POP_STD_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_POP_VARIANCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCPopulationStdDev */

#ifdef /* MCPopulationVariance */ LEGACY_EXEC
	if (evalparams(F_POP_VARIANCE, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_POP_VARIANCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCPopulationVariance */

// MW-2007-07-03: [[ Bug 4506 ]] - Large integers result in negative numbers
//   being generated.

#ifdef /* MCRandom */ LEGACY_EXEC
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	if (t_stat == ES_NORMAL)
		t_stat = limit -> eval(ep);

	if (t_stat == ES_NORMAL)
		t_stat = ep . ton();

	double t_value;
	if (t_stat == ES_NORMAL)
	{
		t_value = floor(ep . getnvalue() + 0.5);
		if (t_value < 1.0 || t_value >= 4294967296.0)
			t_stat = ES_ERROR;
	}

	if (t_stat == ES_NORMAL)
		ep . setnvalue(floor(t_value * MCU_drand()) + 1);
	else
		MCeerror->add(EE_RANDOM_BADSOURCE, line, pos);

	return t_stat;
#endif /* MCRandom */


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
#ifdef /* MCRound */ LEGACY_EXEC
	real8 factor = 1.0;
	if (digit != NULL)
	{
		if (digit->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_ROUND_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		// eliminate precision error by using more than exact power of 10,
		// 6.0 is empirically derived on Intel.  Other rounding algorithms
		// don't work with negative numbers and/or are very slow.
		factor = pow(10 + DBL_EPSILON * 6.0, ep.getnvalue());
	}
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ROUND_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	real8 value = ep.getnvalue() * factor;
	if (value < 0.0)
		value = ceil(value - 0.5);
	else
		value = floor(value + 0.5);
	ep.setnvalue(value / factor);
	return ES_NORMAL;
#endif /* MCRound */

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

void MCRound::compile(MCSyntaxFactoryRef ctxt)
{
	if (digit != nil)
		compile_with_args(ctxt, kMCMathEvalRoundToPrecisionMethodInfo, source, digit);
	else
		compile_with_args(ctxt, kMCMathEvalRoundMethodInfo, source);
}


#ifdef /* MCSin */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_SIN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(sin(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_SIN_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSin */

#ifdef /* MCSampleStdDev */ LEGACY_EXEC
	if (evalparams(F_SMP_STD_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSampleStdDev */

#ifdef /* MCSampleVariance */ LEGACY_EXEC
	if (evalparams(F_SMP_VARIANCE, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_VARIANCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSampleVariance */

#ifdef /* MCSqrt */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_SQRT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(sqrt(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add(EE_SQRT_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSqrt */


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
#ifdef /* MCStatRound */ LEGACY_EXEC
	real8 factor = 1.0;
	if (digit != NULL)
	{
		if (digit->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_ROUND_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		factor = pow(10 + DBL_EPSILON * 6.0, ep.getnvalue());
	}
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ROUND_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	real8 value = ep.getnvalue() * factor;
	if (value < 0.0)
	{
		value -= 0.5;
		if (fmod(value, 2.0) == -1.0)
			value += 1.0;
		else
			value = ceil(value);
	}
	else
	{
		value += 0.5;
		if (fmod(value, 2.0) == 1.0)
			value -= 1.0;
		else
			value = floor(value);
	}
	ep.setnvalue(value / factor);
	return ES_NORMAL;
#endif /* MCStatRound */

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

void MCStatRound::compile(MCSyntaxFactoryRef ctxt)
{
	if (digit != nil)
		compile_with_args(ctxt, kMCMathEvalStatRoundToPrecisionMethodInfo, source, digit);
	else
		compile_with_args(ctxt, kMCMathEvalStatRoundMethodInfo, source);
}

#ifdef /* MCSampleStdDev */ LEGACY_EXEC
	if (evalparams(F_STD_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSampleStdDev */

#ifdef /* MCSum */ LEGACY_EXEC
	if (evalparams(F_SUM, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_SUM_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSum */

#ifdef /* MCTan */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_TAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(tan(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_TAN_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCTan */

#ifdef /* MCTranspose */ LEGACY_EXEC
	// ARRAYEVAL
	if (source -> eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_TRANSPOSE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	if (ep . getformat() != VF_ARRAY)
	{
		ep . clear();
		return ES_NORMAL;
	}
    
	MCVariableValue *t_array;
	Boolean t_delete_array;
	ep . takearray(t_array, t_delete_array);
    
	MCVariableValue *v = new MCVariableValue();
	if (v->transpose(*t_array) != ES_NORMAL)
	{
		MCeerror->add(EE_TRANSPOSE_MISMATCH, line, pos);
		delete v;
        
		if (t_delete_array)
			delete t_array;
        
		return ES_ERROR;
	}
    
	ep.setarray(v, True);
    
	if (t_delete_array)
		delete t_array;
    
	return ES_NORMAL;
#endif /* MCTranspose */

