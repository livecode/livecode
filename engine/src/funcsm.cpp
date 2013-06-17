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

Exec_stat MCFunction::evalparams(Functions func, MCParameter *params,
                                 MCExecPoint &ep)
{
	uint4 nparams = 0;
	real8 n, tn, oldn;
	n = oldn = 0.0;
	MCSortnode *mditems = NULL;
	if (func == F_POP_STD_DEV || func == F_SMP_STD_DEV)
	{ //use recursion to get average first
		if (evalparams(F_AVERAGE, params, ep) != ES_NORMAL)
			return ES_ERROR;
		oldn = ep.getnvalue();
	}
	if (func == F_MEDIAN)
	{ //use recursion to count items first
		if (evalparams(F_UNDEFINED, params, ep) != ES_NORMAL)
			return ES_ERROR;
		mditems = new MCSortnode[(uint4)ep.getnvalue()];
	}
	if (params != NULL && params->getnext() == NULL)
	{
		if (params->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_FUNCTION_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		if (ep.getformat() == VF_ARRAY)
		{
			if (ep.getarray() -> is_array() && ep.getarray()->get_array()->dofunc(ep, func, nparams, n, oldn, mditems) != ES_NORMAL)
			{
				MCeerror->add(EE_FUNCTION_BADSOURCE, line, pos);
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
					tn = 0.0;
				else
					if (!MCU_stor8(s, tn))
					{
						MCeerror->add
						(EE_FUNCTION_NAN, 0, 0, s);
						return ES_ERROR;
					}
				MCU_dofunc(func, nparams, n, tn, oldn, mditems);
			}
		}
	}
	else
	{
		MCParameter *tparam = params;
		while (tparam != NULL)
		{
			if (tparam->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
			{
				MCeerror->add(EE_FUNCTION_BADSOURCE, line, pos);
				return ES_ERROR;
			}
			MCU_dofunc(func, nparams, n, ep.getnvalue(), oldn, mditems);
			tparam = tparam->getnext();
		}
	}
	if (nparams != 0)
		switch (func)
		{
		case F_AVERAGE:
			n /= nparams;
			break;
		case F_MEDIAN:
			{
				uint4 toffset;
				MCU_sort(mditems, nparams, ST_ASCENDING, ST_NUMERIC);
				toffset = (nparams + 1)/2 - 1;
				if ((nparams % 2) != 0) //odd
					n = mditems[toffset].nvalue;
				else //even average 2 closest values
					n = (mditems[toffset].nvalue + mditems[toffset+1].nvalue)/2;
				break;
			}
		case F_POP_STD_DEV:
			n = sqrt(n/nparams);
			break;
		case F_SMP_STD_DEV:
			n = sqrt(n/(nparams - 1));
			break;
		case F_UNDEFINED:
			n = nparams;
			break;
		default:
			break;
		}
	ep.setnvalue(n);
	delete mditems;
	return ES_NORMAL;
}

MCAbsFunction::~MCAbsFunction()
{
	delete source;
}

Parse_stat MCAbsFunction::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_ABS_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCAbsFunction::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ABS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	ep.setnvalue(fabs(ep.getnvalue()));
	return ES_NORMAL;
}

MCAcos::~MCAcos()
{
	delete source;
}

Parse_stat MCAcos::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_ACOS_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCAcos::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ACOS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(acos(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_ACOS_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
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
		MCperror->add
		(PE_ANNUITY_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCAnnuity::eval(MCExecPoint &ep)
{
	if (rate->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ANNUITY_BADRATE, line, pos);
		return ES_ERROR;
	}
	real8 rn = ep.getnvalue();
	if (periods->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ANNUITY_BADPERIODS, line, pos);
		return ES_ERROR;
	}
	real8 pn = ep.getnvalue();
	ep.setnvalue((1.0 - pow(1.0 + rn, -pn)) / rn);
	return ES_NORMAL;
}

MCAsin::~MCAsin()
{
	delete source;
}

Parse_stat MCAsin::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_ASIN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCAsin::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ASIN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(asin(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_ASIN_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCAtan::~MCAtan()
{
	delete source;
}

Parse_stat MCAtan::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_ATAN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCAtan::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ATAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(atan(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_ATAN_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
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
		MCperror->add
		(PE_ATAN2_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCAtan2::eval(MCExecPoint &ep)
{
	if (s1->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ATAN2_BADS1, line, pos);
		return ES_ERROR;
	}
	real8 n1 = ep.getnvalue();
	if (s2->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ATAN2_BADS2, line, pos);
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
}

MCAverage::~MCAverage()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCAverage::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add
		(PE_AVERAGE_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCAverage::eval(MCExecPoint &ep)
{
	if (evalparams(F_AVERAGE, params, ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_AVERAGE_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
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
		MCperror->add
		(PE_COMPOUND_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCCompound::eval(MCExecPoint &ep)
{
	if (rate->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_COMPOUND_BADRATE, line, pos);
		return ES_ERROR;
	}
	real8 rn = ep.getnvalue();
	if (periods->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_COMPOUND_BADPERIODS, line, pos);
		return ES_ERROR;
	}
	real8 pn = ep.getnvalue();
	ep.setnvalue(pow(1.0 + rn, pn));
	return ES_NORMAL;
}

MCCos::~MCCos()
{
	delete source;
}

Parse_stat MCCos::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_COS_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCCos::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_COS_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(cos(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_COS_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCExp::~MCExp()
{
	delete source;
}

Parse_stat MCExp::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_EXP_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCExp::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_EXP_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(exp(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_EXP_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCExp1::~MCExp1()
{
	delete source;
}

Parse_stat MCExp1::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_EXP1_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCExp1::eval(MCExecPoint &ep)
{
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
}

MCExp2::~MCExp2()
{
	delete source;
}

Parse_stat MCExp2::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_EXP2_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCExp2::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_EXP2_BADSOURCE, line, pos);
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
}

MCExp10::~MCExp10()
{
	delete source;
}

Parse_stat MCExp10::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_EXP10_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCExp10::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_EXP10_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(pow(10.0, ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_EXP10_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCLn::~MCLn()
{
	delete source;
}

Parse_stat MCLn::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_LN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCLn::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_LN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(log(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_LN_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCLn1::~MCLn1()
{
	delete source;
}

Parse_stat MCLn1::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_LN1_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCLn1::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_LN1_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(log(1.0 + ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_LN1_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCLog2::~MCLog2()
{
	delete source;
}

Parse_stat MCLog2::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_LOG2_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCLog2::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_LOG2_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(log(ep.getnvalue()) / log(2.0));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_LOG2_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCLog10::~MCLog10()
{
	delete source;
}

Parse_stat MCLog10::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_LOG10_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCLog10::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_LOG10_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(log(ep.getnvalue()) / log(10.0));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_LOG10_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
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
		MCperror->add
		(PE_MATRIXMULT_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCMatrixMultiply::eval(MCExecPoint &ep)
{
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
		MCperror->add
		(PE_MAX_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCMaxFunction::eval(MCExecPoint &ep)
{
	if (evalparams(F_MAX, params, ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_MAX_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
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
		MCperror->add
		(PE_MEDIAN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCMedian::eval(MCExecPoint &ep)
{
	if (evalparams(F_MEDIAN, params, ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_MEDIAN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCMD5Digest::~MCMD5Digest()
{
	delete source;
}

Parse_stat MCMD5Digest::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add(PE_MD5DIGEST_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCMD5Digest::eval(MCExecPoint &ep)
{
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
}

MCSHA1Digest::~MCSHA1Digest()
{
	delete source;
}

Parse_stat MCSHA1Digest::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add(PE_SHA1DIGEST_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSHA1Digest::eval(MCExecPoint &ep)
{
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
}

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
		MCperror->add
		(PE_MIN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCMinFunction::eval(MCExecPoint &ep)
{
	if (evalparams(F_MIN, params, ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_MIN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCPopStdDev::~MCPopStdDev()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCPopStdDev::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add
		(PE_STDDEV_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCPopStdDev::eval(MCExecPoint &ep)
{
	if (evalparams(F_POP_STD_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCRandom::~MCRandom()
{
	delete limit;
}

Parse_stat MCRandom::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &limit, the) != PS_NORMAL)
	{
		MCperror->add(PE_RANDOM_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

// MW-2007-07-03: [[ Bug 4506 ]] - Large integers result in negative numbers
//   being generated.
Exec_stat MCRandom::eval(MCExecPoint &ep)
{
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
}

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
		MCperror->add
		(PE_ROUND_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCRound::eval(MCExecPoint &ep)
{
	real8 factor = 1.0;
	if (digit != NULL)
	{
		if (digit->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add
			(EE_ROUND_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		// eliminate precision error by using more than exact power of 10,
		// 6.0 is empirically derived on Intel.  Other rounding algorithms
		// don't work with negative numbers and/or are very slow.
		factor = pow(10 + DBL_EPSILON * 6.0, ep.getnvalue());
	}
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ROUND_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	real8 value = ep.getnvalue() * factor;
	if (value < 0.0)
		value = ceil(value - 0.5);
	else
		value = floor(value + 0.5);
	ep.setnvalue(value / factor);
	return ES_NORMAL;
}

MCSin::~MCSin()
{
	delete source;
}

Parse_stat MCSin::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_SIN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSin::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_SIN_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(sin(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_SIN_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCSmpStdDev::~MCSmpStdDev()
{
	while (params != NULL)
	{
		MCParameter *tparams = params;
		params = params->getnext();
		delete tparams;
	}
}

Parse_stat MCSmpStdDev::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	
	if (getparams(sp, &params) != PS_NORMAL)
	{
		MCperror->add
		(PE_STDDEV_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSmpStdDev::eval(MCExecPoint &ep)
{
	if (evalparams(F_SMP_STD_DEV, params, ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_STDDEV_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCSqrt::~MCSqrt()
{
	delete source;
}

Parse_stat MCSqrt::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_SQRT_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSqrt::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_SQRT_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	MCS_seterrno(0);
	ep.setnvalue(sqrt(ep.getnvalue()));
	if (MCS_geterrno() != 0 || MCS_isnan(ep.getnvalue()))
	{
		MCS_seterrno(0);
		MCeerror->add
		(EE_SQRT_DOMAIN, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
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
		MCperror->add
		(PE_ROUND_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCStatRound::eval(MCExecPoint &ep)
{
	real8 factor = 1.0;
	if (digit != NULL)
	{
		if (digit->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add
			(EE_ROUND_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		factor = pow(10 + DBL_EPSILON * 6.0, ep.getnvalue());
	}
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_ROUND_BADSOURCE, line, pos);
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
		MCperror->add
		(PE_SUM_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSum::eval(MCExecPoint &ep)
{
	if (evalparams(F_SUM, params, ep) != ES_NORMAL)
	{
		MCeerror->add
		(EE_SUM_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

MCTan::~MCTan()
{
	delete source;
}

Parse_stat MCTan::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_TAN_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCTan::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_TAN_BADSOURCE, line, pos);
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
}

MCTranspose::~MCTranspose()
{
	delete source;
}

Parse_stat MCTranspose::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add(PE_TRANSPOSE_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCTranspose::eval(MCExecPoint &ep)
{
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
}

MCTrunc::~MCTrunc()
{
	delete source;
}

Parse_stat MCTrunc::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);

	if (get1param(sp, &source, the) != PS_NORMAL)
	{
		MCperror->add
		(PE_TRUNC_BADPARAM, line, pos);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCTrunc::eval(MCExecPoint &ep)
{
	if (source->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add
		(EE_TRUNC_BADSOURCE, line, pos);
		return ES_ERROR;
	}
	if (ep.getnvalue() < 0.0)
		ep.setnvalue(ceil(ep.getnvalue()));
	else
		ep.setnvalue(floor(ep.getnvalue()));
	return ES_NORMAL;
}
