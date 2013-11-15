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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
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

Exec_stat MCFunction::params_to_doubles(MCExecPoint& ep, MCParameter *p_params, real64_t*& r_doubles, uindex_t& r_count)
{
	MCAutoArray<real64_t> t_list;
	real64_t t_number;

	if (p_params != NULL && p_params->getnext() == NULL)
	{
		if (p_params->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_FUNCTION_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		if (ep.isarray())
		{
			MCNameRef t_key;
			MCValueRef t_value;
			uintptr_t t_index = 0;
			while (MCArrayIterate(ep.getarrayref(), t_index, t_key, t_value))
			{
				if (!ep.convertvaluereftoreal(t_value, t_number))
				{
					MCeerror->add(EE_FUNCTION_BADSOURCE, line, pos);
					return ES_ERROR;
				}
				if (!t_list.Push(t_number))
					return ES_ERROR;
			}
		}
		else
		{
			MCString s(ep.getsvalue());
			uint4 length = s.getlength();
			const char *sptr = s.getstring();
			MCU_skip_spaces(sptr, length);
			while (length != 0)
			{
				s.setstring(sptr);
				if (!MCU_strchr(sptr, length, ','))
				{
					s.setlength(length);
					length = 0;
				}
				else
				{
					s.setlength(sptr - s.getstring());
					MCU_skip_char(sptr, length);
					MCU_skip_spaces(sptr, length);
				}
				if (s.getlength() == 0)
					t_number = 0.0;
  				else
				{
					if (!MCU_stor8(s, t_number))
					{
						MCeerror->add(EE_FUNCTION_NAN, 0, 0, s);
						return ES_ERROR;
					}
				}
				
				if (!t_list.Push(t_number))
					return ES_ERROR;
			}
		}
	}
	else
	{
		MCParameter *t_param = p_params;
		while (t_param != NULL)
		{
			if (t_param->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
			{
				MCeerror->add(EE_FUNCTION_BADSOURCE, line, pos);
				return ES_ERROR;
			}
            
			if (!t_list.Push(ep.getnvalue()))
				return ES_ERROR;
            
			t_param = t_param->getnext();
		}
	}
    
	t_list.Take(r_doubles, r_count);
 	return ES_NORMAL;   
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

// JS-2013-06-19: [[ StatsFunctions ]] Implementation of arithmeticMean (was average)
MCArithmeticMean::~MCArithmeticMean()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCArithmeticMean::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_AVERAGE_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCArithmeticMean::eval(MCExecPoint &ep)
{
#ifdef LEGACY_EXEC
	if (evalparams(F_ARI_MEAN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_AVERAGE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif
    
	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;
    
	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_AVERAGE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	MCMathEvalArithmeticMean(ctxt, t_values.Ptr(), t_values.Size(), t_result);
    
	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}
    
	return ctxt.Catch(line, pos);
}


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

// JS-2013-06-19: [[ StatsFunctions ]] Implementation of averageDeviation
MCAvgDev::~MCAvgDev()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCAvgDev::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_AVERAGE_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCAvgDev::eval(MCExecPoint &ep)
{
#ifdef /* MCAverage */ LEGACY_EXEC

	if (evalparams(F_AVG_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_AVERAGE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCAverage */

	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;

	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_AVERAGE_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	MCMathEvalAverageDeviation(ctxt, t_values.Ptr(), t_values.Size(), t_result);

	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}

	return ctxt.Catch(line, pos);
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


// JS-2013-06-19: [[ StatsFunctions ]] Implementation of geometricMean
MCGeometricMean::~MCGeometricMean()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCGeometricMean::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_GEO_MEAN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCGeometricMean::eval(MCExecPoint &ep)
{
#ifdef LEGACY_EXEC
	if (evalparams(F_GEO_MEAN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_GEO_MEAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif
    
	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;
    
	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_GEO_MEAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	MCMathEvalGeometricMean(ctxt, t_values.Ptr(), t_values.Size(), t_result);
    
	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}
    
	return ctxt.Catch(line, pos);
}

// JS-2013-06-19: [[ StatsFunctions ]] Implementation of harmonicMean
MCHarmonicMean::~MCHarmonicMean()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCHarmonicMean::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_HAR_MEAN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCHarmonicMean::eval(MCExecPoint &ep)
{
#ifdef LEGACY_EXEC
	if (evalparams(F_HAR_MEAN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_HAR_MEAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif
    
	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;
    
	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	MCMathEvalHarmonicMean(ctxt, t_values.Ptr(), t_values.Size(), t_result);
    
	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}
    
	return ctxt.Catch(line, pos);
}


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
    
	MCVariableValue *t_dst_array;
	Boolean t_delete_dst_array;
	ep . takearray(t_dst_array, t_delete_dst_array);
    
	if (source -> eval(ep) != ES_NORMAL)
	{
		if (t_delete_dst_array)
			delete t_dst_array;
        
		MCeerror->add(EE_MATRIXMULT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	MCVariableValue *t_src_array;
	Boolean t_delete_src_array;
	ep . takearray(t_src_array, t_delete_src_array);
    
	MCVariableValue *v = new MCVariableValue();
	if (v->matrixmultiply(ep, *t_dst_array, *t_src_array) != ES_NORMAL)
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

MCMaxFunction::~MCMaxFunction()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCMaxFunction::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_MAX_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCMaxFunction::eval(MCExecPoint &ep)
{
#ifdef /* MCMaxFunction */ LEGACY_EXEC
	if (evalparams(F_MAX, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MAX_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCMaxFunction */

	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;

	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_MAX_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	MCMathEvalMax(ctxt, t_values.Ptr(), t_values.Size(), t_result);

	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}

	return ctxt.Catch(line, pos);
}

MCMedian::~MCMedian()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCMedian::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_MEDIAN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCMedian::eval(MCExecPoint &ep)
{
#ifdef /* MCMedian */ LEGACY_EXEC
	if (evalparams(F_MEDIAN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MEDIAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCMedian */
    
	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;

	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_MEDIAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	MCMathEvalMedian(ctxt, t_values.Ptr(), t_values.Size(), t_result);

	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}

	return ctxt.Catch(line, pos);
}


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


MCMinFunction::~MCMinFunction()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCMinFunction::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_MIN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCMinFunction::eval(MCExecPoint &ep)
{
#ifdef /* MCMinFunction */ LEGACY_EXEC
	if (evalparams(F_MIN, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_MIN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCMinFunction */

	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;

	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_MIN_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	MCMathEvalMin(ctxt, t_values.Ptr(), t_values.Size(), t_result);

	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}

	return ctxt.Catch(line, pos);
}

// JS-2013-06-19: [[ StatsFunctions ]] Implementation of populationStdDev
MCPopulationStdDev::~MCPopulationStdDev()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCPopulationStdDev::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_POP_STDDEV_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCPopulationStdDev::eval(MCExecPoint &ep)
{
#ifdef LEGACY_EXEC
	if (evalparams(F_POP_STD_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_POP_VARIANCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif
    
	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;
    
	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_POP_VARIANCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	MCMathEvalPopulationStdDev(ctxt, t_values.Ptr(), t_values.Size(), t_result);
    
	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}
    
	return ctxt.Catch(line, pos);
}

// JS-2013-06-19: [[ StatsFunctions ]] Implementation of populationVariance
MCPopulationVariance::~MCPopulationVariance()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCPopulationVariance::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_POP_VARIANCE_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCPopulationVariance::eval(MCExecPoint &ep)
{
#ifdef LEGACY_EXEC
	if (evalparams(F_POP_VARIANCE, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_POP_VARIANCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif
    
	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;
    
	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_POP_VARIANCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	MCMathEvalPopulationVariance(ctxt, t_values.Ptr(), t_values.Size(), t_result);
    
	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}
    
	return ctxt.Catch(line, pos);
}

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
    if (!ctxt . EvalOptionalExprAsDouble(digit, nil, EE_RANDOM_BADSOURCE, t_digit))
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


// JS-2013-06-19: [[ StatsFunctions ]] Implementation of sampleStdDev (was stdDev)
MCSampleStdDev::~MCSampleStdDev()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCSampleStdDev::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_STDDEV_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSampleStdDev::eval(MCExecPoint &ep)
{
#ifdef /* MCSampleStdDev */ LEGACY_EXEC
	if (evalparams(F_SMP_STD_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSampleStdDev */
    
	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;
    
	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	MCMathEvalSampleStdDev(ctxt, t_values.Ptr(), t_values.Size(), t_result);
    
	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}
    
	return ctxt.Catch(line, pos);
}

// JS-2013-06-19: [[ StatsFunctions ]] Implementation of sampleVariance
MCSampleVariance::~MCSampleVariance()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCSampleVariance::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_VARIANCE_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSampleVariance::eval(MCExecPoint &ep)
{
#ifdef LEGACY_EXEC
	if (evalparams(F_SMP_VARIANCE, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_VARIANCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif
    
	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;
    
	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_VARIANCE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
    
	MCMathEvalPopulationVariance(ctxt, t_values.Ptr(), t_values.Size(), t_result);
    
	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}
    
	return ctxt.Catch(line, pos);
}


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
    if (!ctxt . EvalOptionalExprAsDouble(digit, nil, EE_RANDOM_BADSOURCE, t_digit))
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

MCStdDev::~MCStdDev()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCStdDev::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_STDDEV_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCStdDev::eval(MCExecPoint &ep)
{
#ifdef /* MCStdDev */ LEGACY_EXEC
	if (evalparams(F_STD_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCStdDev */


	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;

	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	MCMathEvalSampleStdDev(ctxt, t_values.Ptr(), t_values.Size(), t_result);

	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}

	return ctxt.Catch(line, pos);
}

MCSum::~MCSum()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCSum::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add(PE_SUM_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSum::eval(MCExecPoint &ep)
{
#ifdef /* MCSum */ LEGACY_EXEC
	if (evalparams(F_SUM, params, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_SUM_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCSum */

	MCExecContext ctxt(ep);
	MCAutoArray<real64_t> t_values;
	real64_t t_result;

	if (params_to_doubles(ep, params, t_values.PtrRef(), t_values.SizeRef()) != ES_NORMAL)
	{
		MCeerror->add(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}

	MCMathEvalSum(ctxt, t_values.Ptr(), t_values.Size(), t_result);

	if (!ctxt.HasError())
	{
		/* UNCHECKED */ ep.setnvalue(t_result);
		return ES_NORMAL;
	}

	return ctxt.Catch(line, pos);
}


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


#ifdef /* MCTrunc */ LEGACY_EXEC
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_TRUNC_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	if (ep.getnvalue() < 0.0)
		ep.setnvalue(ceil(ep.getnvalue()));
	else
		ep.setnvalue(floor(ep.getnvalue()));
	return ES_NORMAL;
#endif /* MCTrunc */
