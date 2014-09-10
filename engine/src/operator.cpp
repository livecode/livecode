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
#include "mcio.h"

#include "uidc.h"
#include "scriptpt.h"
//#include "execpt.h"
#include "chunk.h"
#include "operator.h"
#include "mcerror.h"
#include "util.h"
#include "date.h"
#include "securemode.h"
#include "osspec.h"

#include "globals.h"
#include "exec.h"

#include "syntax.h"

///////////////////////////////////////////////////////////////////////////////

void MCBinaryOperator::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	
	left -> compile(ctxt);
	if (right != nil)
		right -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstant(ctxt, kMCEmptyString);
	
	MCSyntaxFactoryEvalMethod(ctxt, getmethodinfo());
	
	MCSyntaxFactoryEndExpression(ctxt);

}

void MCUnaryOperator::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	
	right -> compile(ctxt);
	
	MCSyntaxFactoryEvalMethod(ctxt, getmethodinfo());
	
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCMultiBinaryOperator::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	
	if (left != nil)
		left -> compile(ctxt);
	else if (canbeunary())
		MCSyntaxFactoryEvalConstantUInt(ctxt, 0);
	else
		MCAssert(false);
		
	right -> compile(ctxt);
	
	MCExecMethodInfo **t_methods;
	uindex_t t_method_count;
	
	getmethodinfo(t_methods, t_method_count);
	
	for(uindex_t i = 0; i < t_method_count; i++)
		MCSyntaxFactoryEvalMethod(ctxt, t_methods[i]);
	
	MCSyntaxFactoryEndExpression(ctxt);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Logical operators
//

void MCAnd::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCAnd */ LEGACY_EXEC
    Boolean state1;
    Boolean state2 = False;

    if (left->eval(ep) != ES_NORMAL)
    {
        MCeerror->add(EE_AND_BADLEFT, line, pos);
        return ES_ERROR;
    }
    state1 = ep.getsvalue() == MCtruemcstring;
    if (state1)
    {
        if (right->eval(ep) != ES_NORMAL)
        {
            MCeerror->add(EE_AND_BADRIGHT, line, pos);
            return ES_ERROR;
        }
        state2 = ep.getsvalue() == MCtruemcstring;
    }
    ep.setboolean(state1 && state2);
    return ES_NORMAL;
#endif /* MCAnd */

    bool t_result;
    bool t_left, t_right;

    if (!ctxt . EvalExprAsNonStrictBool(left, EE_AND_BADLEFT, t_left))
        return;

    /* CONDITIONAL EVALUATION */
    if(t_left)
    {
        if (!ctxt . EvalExprAsNonStrictBool(right, EE_AND_BADRIGHT, t_right))
            return;

        MCLogicEvalAnd(ctxt, t_left, t_right, t_result);
    }
    else
        t_result = false;

    if (!ctxt . HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}


void MCOr::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCOr */ LEGACY_EXEC
    Boolean state1;
    Boolean state2 = False;

    if (left->eval(ep) != ES_NORMAL)
    {
        MCeerror->add(EE_OR_BADLEFT, line, pos);
        return ES_ERROR;
    }
    state1 = ep.getsvalue() == MCtruemcstring;
    if (!state1)
    {
        if (right->eval(ep) != ES_NORMAL)
        {
            MCeerror->add(EE_OR_BADRIGHT, line, pos);
            return ES_ERROR;
        }
        state2 = ep.getsvalue() == MCtruemcstring;
    }
    ep.setboolean(state1 || state2);
    return ES_NORMAL;
#endif /* MCOr */

    bool t_result;
    bool t_left, t_right;

    if (!ctxt . EvalExprAsNonStrictBool(left, EE_OR_BADLEFT, t_left))
        return;

    if (!t_left)
    {
        if (!ctxt . EvalExprAsNonStrictBool(right, EE_OR_BADRIGHT, t_right))
            return;

        MCLogicEvalOr(ctxt, t_left, t_right, t_result);
    }
    else
        t_result = true;

    if (!ctxt.HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}

void MCNot::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCNot */ LEGACY_EXEC
	if (right->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_NOT_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	Boolean state = ep.getsvalue() == MCtruemcstring;
	ep.setboolean(!state);
	return ES_NORMAL;
#endif /* MCNot */
    bool t_right;
    bool t_result;

    if (!ctxt . EvalExprAsNonStrictBool(right, EE_NOT_BADRIGHT, t_right))
        return;

    MCLogicEvalNot(ctxt, t_right, t_result);

    if (!ctxt . HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bitwise operators
//

#ifdef /* MCAndBits */ LEGACY_EXEC
	if (left->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL )
	{
		MCeerror->add(EE_ANDBITS_BADLEFT, line, pos);
		return ES_ERROR;
	}
	uint4 lo = ep.getuint4();
	if (right->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ANDBITS_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	uint4 ro = ep.getuint4();
	ep.setnvalue(ro & lo);
	return ES_NORMAL;
#endif /* MCAndBits */

#ifdef /* MCNotBits */ LEGACY_EXEC
	if (right->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_NOTBITS_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	ep.setnvalue(~ep.getuint4());
	return ES_NORMAL;
#endif /* MCNotBits */

#ifdef /* MCOrBits */ LEGACY_EXEC
	if (left->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ORBITS_BADLEFT, line, pos);
		return ES_ERROR;
	}
	uint4 lo = ep.getuint4();
	if (right->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_ORBITS_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	uint4 ro = ep.getuint4();
	ep.setnvalue(ro | lo);
	return ES_NORMAL;
#endif /* MCOrBits */

#ifdef /* MCXorBits */ LEGACY_EXEC
	if (left->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_XORBITS_BADLEFT, line, pos);
		return ES_ERROR;
	}
	uint4 lo = ep.getuint4();
	if (right->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_XORBITS_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	uint4 ro = ep.getuint4();
	ep.setnvalue(ro ^ lo);
	return ES_NORMAL;
#endif /* MCXorBits */

///////////////////////////////////////////////////////////////////////////////
//
//  String operators
//

void MCConcat::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCConcat */ LEGACY_EXEC
	MCExecPoint ep2(ep1);
	if (left->eval(ep1) != ES_NORMAL)
	{
		MCeerror->add(EE_CONCAT_BADLEFT, line, pos);
		return ES_ERROR;
	}
	ep1.grabsvalue();
	if (right->eval(ep2) != ES_NORMAL)
	{
		MCeerror->add(EE_CONCAT_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	ep1.appendmcstring(ep2 . getsvalue());
	return ES_NORMAL;
#endif /* MCConcat */
    
    MCAutoValueRef t_left, t_right;
    if (!ctxt . EvalExprAsValueRef(left, EE_CONCAT_BADLEFT, &t_left))
        return;
    
    if (!ctxt . EvalExprAsValueRef(right, EE_CONCAT_BADRIGHT, &t_right))
        return;
    
    // AL-2014-09-11: [[ Bug 12195 ]] Data ought to be concatenated without conversion to text
    if (MCValueGetTypeCode(*t_left) == kMCValueTypeCodeData &&
        MCValueGetTypeCode(*t_right) == kMCValueTypeCodeData)
    {
        MCDataRef t_result;
        MCStringsEvalConcatenate(ctxt, (MCDataRef)*t_left, (MCDataRef)*t_right, t_result);
        
        if (!ctxt . HasError())
            MCExecValueTraits<MCDataRef>::set(r_value, t_result);
        return;
    }
    
    MCAutoStringRef t_left_string, t_right_string;
    if (!ctxt . ConvertToString(*t_left, &t_left_string))
        return;
    
    if (!ctxt . ConvertToString(*t_right, &t_right_string))
        return;
    
    MCStringRef t_result;
    MCStringsEvalConcatenate(ctxt, *t_left_string, *t_right_string, t_result);
    
    if (!ctxt . HasError())
        MCExecValueTraits<MCStringRef>::set(r_value, t_result);
}

void MCConcat::compile(MCSyntaxFactoryRef ctxt)
{
    MCSyntaxFactoryBeginExpression(ctxt, line, pos);
    
    left -> compile(ctxt);
    
    right -> compile(ctxt);
    
    MCSyntaxFactoryEvalMethod(ctxt, kMCStringsEvalConcatenateMethodInfo);
    
    MCSyntaxFactoryEndExpression(ctxt);
}

#ifdef /* MCConcatSpace */ LEGACY_EXEC
	MCExecPoint ep2(ep1);
	if (left->eval(ep1) != ES_NORMAL)
	{
		MCeerror->add(EE_CONCATSPACE_BADLEFT, line, pos);
		return ES_ERROR;
	}
	ep1.grabsvalue();
	if (right->eval(ep2) != ES_NORMAL)
	{
		MCeerror->add(EE_CONCATSPACE_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	ep1.concatmcstring(ep2.getsvalue(), EC_SPACE, false);
	return ES_NORMAL;
#endif /* MCConcatSpace */

#ifdef /* MCItem */ LEGACY_EXEC
	if (left->eval(ep1) != ES_NORMAL)
	{
		MCeerror->add(EE_ITEM_BADLEFT, line, pos);
		return ES_ERROR;
	}
	MCExecPoint ep2(ep1);
	if (right == NULL)
		ep2.clear();
	else
	{
		ep1.grabsvalue();
		if (right->eval(ep2) != ES_NORMAL)
		{
			MCeerror->add(EE_ITEM_BADRIGHT, line, pos);
			return ES_ERROR;
		}
	}
	ep1.concatmcstring(ep2.getsvalue(), EC_COMMA, false);
	return ES_NORMAL;
#endif /* MCItem */

#ifdef /* MCContains */ LEGACY_EXEC
	MCExecPoint ep2(ep1);
	if (right->eval(ep1) != ES_NORMAL)
	{
		MCeerror->add(EE_CONTAINS_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	ep1.grabsvalue();
	if (left->eval(ep2) != ES_NORMAL)
	{
		MCeerror->add(EE_CONTAINS_BADLEFT, line, pos);
		return ES_ERROR;
	}
	uint4 i;
	ep1.setboolean(MCU_offset(ep1.getsvalue(), ep2.getsvalue(), i, ep1.getcasesensitive()));
	return ES_NORMAL;
#endif /* MCContains */

Parse_stat MCBeginsEndsWith::parse(MCScriptPoint& sp, Boolean the)
{
    initpoint(sp);

    if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) != PS_NORMAL)
    {
        MCperror -> add(PE_BEGINSENDS_NOWITH, sp);
        return PS_ERROR;
    }

    return PS_NORMAL;
}

void MCBeginsWith::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCBeginsWith */ LEGACY_EXEC
	MCExecPoint ep2(ep1);
	if (right->eval(ep1) != ES_NORMAL)
	{
		MCeerror->add(EE_BEGINSENDS_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	ep1.grabsvalue();
	if (left->eval(ep2) != ES_NORMAL)
	{
		MCeerror->add(EE_BEGINSENDS_BADLEFT, line, pos);
		return ES_ERROR;
	}

	MCString t_part;
	t_part = ep1 . getsvalue();

	MCString t_whole;
	t_whole = ep2 . getsvalue();

	bool t_result;
	if (t_whole . getlength() < t_part . getlength())
		t_result = false;
	else if (ep1 . getcasesensitive())
		t_result = memcmp(t_whole . getstring(), t_part . getstring(), t_part . getlength()) == 0;
	else
		t_result = MCU_strncasecmp(t_whole . getstring(), t_part . getstring(), t_part . getlength()) == 0;
	
	ep1.setboolean(t_result);

	return ES_NORMAL;
#endif /* MCBeginsWith */

    MCAutoStringRef t_left, t_right;
    bool t_result;

    if (!ctxt . EvalExprAsStringRef(left, EE_BEGINSENDS_BADLEFT, &t_left)
            || !ctxt . EvalExprAsStringRef(right, EE_BEGINSENDS_BADRIGHT, &t_right))
        return;

    MCStringsEvalBeginsWith(ctxt, *t_left, *t_right, t_result);

    MCExecValueTraits<bool>::set(r_value, t_result);
}

void MCEndsWith::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCEndsWith */ LEGACY_EXEC
    MCExecPoint ep2(ep1);
    if (right->eval(ep1) != ES_NORMAL)
	{
		MCeerror->add(EE_BEGINSENDS_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	ep1.grabsvalue();
	if (left->eval(ep2) != ES_NORMAL)
	{
		MCeerror->add(EE_BEGINSENDS_BADLEFT, line, pos);
		return ES_ERROR;
	}

	MCString t_part;
	t_part = ep1 . getsvalue();

	MCString t_whole;
	t_whole = ep2 . getsvalue();

	bool t_result;
	if (t_whole . getlength() < t_part . getlength())
		t_result = false;
	else if (ep1 . getcasesensitive())
		t_result = memcmp(t_whole . getstring() + t_whole . getlength() - t_part . getlength(), t_part . getstring(), t_part . getlength()) == 0;
	else
		t_result = MCU_strncasecmp(t_whole . getstring() + t_whole . getlength() - t_part . getlength(), t_part . getstring(), t_part . getlength()) == 0;
	
	ep1.setboolean(t_result);

	return ES_NORMAL;
#endif /* MCEndsWith */

    MCAutoStringRef t_left, t_right;
    bool t_result;

    if (!ctxt . EvalExprAsStringRef(left, EE_BEGINSENDS_BADLEFT, &t_left)
            || !ctxt . EvalExprAsStringRef(right, EE_BEGINSENDS_BADRIGHT, &t_right))
        return;

    MCStringsEvalEndsWith(ctxt, *t_left, *t_right, t_result);

    MCExecValueTraits<bool>::set(r_value, t_result);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Numeric operators
//

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the left or right can be an array or number so we use 'tona'.
#ifdef /* MCDiv */ LEGACY_EXEC
	if (left->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_DIV_BADLEFT, line, pos);
		return ES_ERROR;
	}
	MCExecPoint ep2(ep);
	if (right->eval(ep2) != ES_NORMAL || ep2.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_DIV_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() != VF_ARRAY && ep2.getnvalue() == 0.0)
	{
		MCeerror->add(EE_DIV_ZERO, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() == VF_ARRAY)
	{
		MCVariableValue *v = new MCVariableValue(*ep . getarray());
		if (v->factorarray(ep2, O_DIV) != ES_NORMAL)
		{
			MCeerror->add(EE_DIV_BADARRAY, line, pos);
			delete v;
			return ES_ERROR;
		}
		ep.setarray(v, True);
	}
	else
	{
		MCS_seterrno(0);
		real8 n = 0.0;
		if (ep2.getformat() == VF_ARRAY)
		{
			MCeerror->add(EE_DIV_MISMATCH, line, pos);
			return ES_ERROR;
		}
		else
			n = ep.getnvalue() / ep2.getnvalue();
		if (n == MCinfinity || MCS_geterrno() != 0)
		{
			MCS_seterrno(0);
			MCeerror->add(EE_DIV_RANGE, line, pos);
			return ES_ERROR;
		}
		if (n < 0.0)
			ep.setnvalue(ceil(n));
		else
			ep.setnvalue(floor(n));
	}
	return ES_NORMAL;
#endif /* MCDiv */

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the left or right can be an array or number so we use 'tona'.
void MCMinus::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCMinus */ LEGACY_EXEC
	if (left == NULL)
		ep.setnvalue(0);
	else if (left->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_MINUS_BADLEFT, line, pos);
		return ES_ERROR;
	}
	MCExecPoint ep2(ep);
	if (right->eval(ep2) != ES_NORMAL || ep2.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_MINUS_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() == VF_ARRAY)
	{
		MCVariableValue *v = new MCVariableValue(*ep.getarray());
		if (v->factorarray(ep2, O_MINUS) != ES_NORMAL)
		{
			MCeerror->add(EE_MINUS_BADARRAY, line, pos);
			delete v;
			return ES_ERROR;
		}
		ep.setarray(v, True);
	}
	else
	{
		MCS_seterrno(0);
		if (ep2.getformat() == VF_ARRAY)
		{
			MCeerror->add(EE_MINUS_MISMATCH, line, pos);
			return ES_ERROR;
		}
		else
			ep.setnvalue(ep.getnvalue() - ep2.getnvalue());
		if (MCS_geterrno() != 0)
		{
			MCS_seterrno(0);
			MCeerror->add(EE_MINUS_RANGE, line, pos);
			return ES_ERROR;
		}
	}
	return ES_NORMAL;
#endif /* MCMinus */

    MCExecValue t_left, t_right;
    
    Boolean t_old_expectation;
    
    t_old_expectation = ctxt . GetNumberExpected();
    ctxt . SetNumberExpected(True);

    if (left == nil)
    {
        MCExecValueTraits<double>::set(t_left, 0.0);
    }
    else if (left -> eval_ctxt(ctxt, t_left), ctxt . HasError()
             || !ctxt . ConvertToNumberOrArray(t_left))
    {
        ctxt . LegacyThrow(EE_MINUS_BADLEFT);
        ctxt . SetNumberExpected(t_old_expectation);
        return;
    }

    if (right -> eval_ctxt(ctxt, t_right), ctxt . HasError()
            || !ctxt . ConvertToNumberOrArray(t_right))
    {
        ctxt . LegacyThrow(EE_MINUS_BADRIGHT);
        ctxt . SetNumberExpected(t_old_expectation);
        MCExecTypeRelease(t_left);
        return;
    }
    
    // Set the number expectation back to its previous state
    ctxt . SetNumberExpected(t_old_expectation);

    r_value . valueref_value = nil;
    if (t_left. type == kMCExecValueTypeArrayRef)
    {
        if (t_right . type == kMCExecValueTypeArrayRef)
            MCMathEvalSubtractArrayFromArray(ctxt, t_left . arrayref_value, t_right . arrayref_value, r_value . arrayref_value);
        else
            MCMathEvalSubtractNumberFromArray(ctxt, t_left . arrayref_value, t_right . double_value, r_value . arrayref_value);
    }
    else
    {
        if (t_right . type == kMCExecValueTypeArrayRef)
            ctxt . LegacyThrow(EE_MINUS_MISMATCH);
        else
            MCMathEvalSubtract(ctxt, t_left . double_value, t_right . double_value, r_value . double_value);
    }

    if (!ctxt . HasError())
        r_value . type = t_left . type;
    
    MCExecTypeRelease(t_left);
    MCExecTypeRelease(t_right);
}

void MCMinus::getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const
{
    static MCExecMethodInfo *s_methods[] = { kMCMathEvalSubtractMethodInfo, kMCMathEvalSubtractNumberFromArrayMethodInfo, kMCMathEvalSubtractArrayFromArrayMethodInfo };
    r_methods = s_methods;
    r_count = 3;
}


// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the left or right can be an array or number so we use 'tona'.
#ifdef /* MCMod */ LEGACY_EXEC
	if (left->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_MOD_BADLEFT, line, pos);
		return ES_ERROR;
	}
	MCExecPoint ep2(ep);
	if (right->eval(ep2) != ES_NORMAL || ep2.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_MOD_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() != VF_ARRAY && ep2.getnvalue() == 0.0)
	{
		MCeerror->add(EE_MOD_ZERO, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() == VF_ARRAY)
	{
		MCVariableValue *v = new MCVariableValue(*ep.getarray());
		if (v->factorarray(ep2, O_MOD) != ES_NORMAL)
		{
			MCeerror->add(EE_MOD_BADARRAY, line, pos);
			delete v;
			return ES_ERROR;
		}
		ep.setarray(v, True);
	}
	else
	{
		MCS_seterrno(0);
		real8 n = 0.0;
		if (ep2.getformat() == VF_ARRAY)
		{
			MCeerror->add(EE_MOD_MISMATCH, line, pos);
			return ES_ERROR;
		}
		else
			n = ep.getnvalue() / ep2.getnvalue();
		if (n == MCinfinity || MCS_geterrno() != 0)
		{
			MCS_seterrno(0);
			MCeerror->add(EE_MOD_RANGE, line, pos);
			return ES_ERROR;
		}
		ep.setnvalue(fmod(ep.getnvalue(), ep2.getnvalue()));
	}
	return ES_NORMAL;
#endif /* MCMod */

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the left or right can be an array or number so we use 'tona'.
#ifdef /* MCWrap */ LEGACY_EXEC
	if (left->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_WRAP_BADLEFT, line, pos);
		return ES_ERROR;
	}
	MCExecPoint ep2(ep);
	if (right->eval(ep2) != ES_NORMAL || ep2.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_WRAP_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() != VF_ARRAY && ep2.getnvalue() == 0.0)
	{
		MCeerror->add
		(EE_WRAP_ZERO, line, pos);
		return ES_ERROR;
}
	if (ep.getformat() == VF_ARRAY)
	{
		MCVariableValue *v = new MCVariableValue(*ep.getarray());
		if (v->factorarray(ep2, O_WRAP) != ES_NORMAL)
		{
			MCeerror->add(EE_WRAP_BADARRAY, line, pos);
			delete v;
			return ES_ERROR;
		}
		ep.setarray(v, True);
	}
	else
	{
		MCS_seterrno(0);
		real8 n = 0.0;
		if (ep2.getformat() == VF_ARRAY)
		{
			MCeerror->add(EE_WRAP_MISMATCH, line, pos);
			return ES_ERROR;
		}
		else
			n = ep.getnvalue() / ep2.getnvalue();
		
		if (n == MCinfinity || MCS_geterrno() != 0)
		{
			MCS_seterrno(0);
			MCeerror->add(EE_WRAP_RANGE, line, pos);
			return ES_ERROR;
		}
		ep . setnvalue(MCU_fwrap(ep . getnvalue(), ep2 . getnvalue()));	
	}
	return ES_NORMAL;
#endif /* MCWrap */

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the left or right can be an array or number so we use 'tona'.
#ifdef /* MCOver */ LEGACY_EXEC
	if (left->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_OVER_BADLEFT, line, pos);
		return ES_ERROR;
	}
	MCExecPoint ep2(ep);
	if (right->eval(ep2) != ES_NORMAL || ep2.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_OVER_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() != VF_ARRAY && ep2.getnvalue() == 0.0)
	{
		MCeerror->add(EE_OVER_ZERO, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() == VF_ARRAY)
	{
		MCVariableValue *v = new MCVariableValue(*ep.getarray());
		if (v->factorarray(ep2, O_OVER) != ES_NORMAL)
		{
			MCeerror->add(EE_OVER_BADARRAY, line, pos);
			delete v;
			return ES_ERROR;
		}
		ep.setarray(v, True);
	}
	else
	{
		MCS_seterrno(0);
		real8 n = 0.0;
		if (ep2.getformat() == VF_ARRAY)
		{
			MCeerror->add(EE_OVER_MISMATCH, line, pos);
			return ES_ERROR;
		}
		else
			n = ep.getnvalue() / ep2.getnvalue();
		if (n == MCinfinity || MCS_geterrno() != 0)
		{
			MCS_seterrno(0);
			MCeerror->add(EE_OVER_RANGE, line, pos);
			return ES_ERROR;
		}
		ep.setnvalue(n);
	}
	return ES_NORMAL;
#endif /* MCOver */

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the left or right can be an array or number so we use 'tona'.
#ifdef /* MCPlus */ LEGACY_EXEC
	if (left == NULL)
		ep.setnvalue(0);
	else
		if (left->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
		{
			MCeerror->add(EE_PLUS_BADLEFT, line, pos);
			return ES_ERROR;
		}
	MCExecPoint ep2(ep);
	if (right->eval(ep2) != ES_NORMAL || ep2.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_PLUS_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() == VF_ARRAY || ep2.getformat() == VF_ARRAY)
	{
		/* give a little slack to developer -- because addition is
		 communicative. The first one to be an array is used as dest and
		 the other one used as source */
		MCVariableValue *v = new MCVariableValue(ep.getformat() == VF_ARRAY
												 ? *ep.getarray() : *ep2.getarray());
		if (v->factorarray(ep.getformat() == VF_ARRAY
		                   ? ep2 : ep, O_PLUS) != ES_NORMAL)
		{
			MCeerror->add(EE_ADD_BADARRAY, line, pos);
			delete v;
			return ES_ERROR;
		}
		ep.setarray(v, True);
	}
	else
	{
		MCS_seterrno(0);
		ep.setnvalue(ep.getnvalue() + ep2.getnvalue());
		if (MCS_geterrno() != 0)
		{
			MCS_seterrno(0);
			MCeerror->add(EE_PLUS_RANGE, line, pos);
			return ES_ERROR;
		}
	}
	return ES_NORMAL;
#endif /* MCPlus */

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
//   Here the left or right can be an array or number so we use 'tona'.
#ifdef /* MCTimes */ LEGACY_EXEC
	if (left->eval(ep) != ES_NORMAL || ep.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_TIMES_BADLEFT, line, pos);
		return ES_ERROR;
	}
	MCExecPoint ep2(ep);
	if (right->eval(ep2) != ES_NORMAL || ep2.tona() != ES_NORMAL)
	{
		MCeerror->add(EE_TIMES_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	if (ep.getformat() == VF_ARRAY || ep2.getformat() == VF_ARRAY)
	{
		/* give a little slack to developer -- because multiplication is
		 communicative.  The first one to be an array is used as dest
		 and the other one used as source */
		MCVariableValue *v = new MCVariableValue(ep.getformat() == VF_ARRAY
												 ? *ep.getarray() : *ep2.getarray());
		if (v->factorarray(ep.getformat() == VF_ARRAY
		                   ? ep2 : ep, O_TIMES) != ES_NORMAL)
		{
			MCeerror->add(EE_TIMES_BADARRAY, line, pos);
			delete v;
			return ES_ERROR;
		}
		ep.setarray(v, True);
	}
	else
	{
		MCS_seterrno(0);
		real8 n = 0.0;
		n = ep.getnvalue() * ep2.getnvalue();
		if (n == MCinfinity || MCS_geterrno() != 0)
		{
			MCS_seterrno(0);
			MCeerror->add(EE_TIMES_RANGE, line, pos);
			return ES_ERROR;
		}
		ep.setnvalue(n);
	}
	return ES_NORMAL;
#endif /* MCTimes */

#ifdef /* MCPow */ LEGACY_EXEC
	if (left->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_POW_BADLEFT, line, pos);
		return ES_ERROR;
	}
	real8 lo = ep.getnvalue();
	if (right->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
	{
		MCeerror->add(EE_POW_BADRIGHT, line, pos);
		return ES_ERROR;
	}
	real8 ro = ep.getnvalue();
	MCS_seterrno(0);
	ep.setnvalue(pow(lo, ro));
	if (MCS_geterrno() != 0)
	{
		MCS_seterrno(0);
		MCeerror->add(EE_POW_RANGE, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
#endif /* MCPow */

///////////////////////////////////////////////////////////////////////////////
//
//  Comparison operators
//

#ifdef LEGACY_EXEC
static bool eval_comparison_factors(MCExecPoint& ep, MCExpression *p_left, MCExpression *p_right, MCValueRef& r_left, MCValueRef& r_right)
{
    MCValueRef t_left, t_right;
    if (p_left->eval(ep) != ES_NORMAL)
    {
        MCeerror->add(EE_FACTOR_BADLEFT, 0, 0);
        return false;
    }

    /* UNCHECKED */ ep . copyasvalueref(t_left);

    if (p_right->eval(ep) != ES_NORMAL)
    {
        MCValueRelease(t_left);
        MCeerror->add(EE_FACTOR_BADRIGHT, 0, 0);
        return false;
    }

    /* UNCHECKED */ ep . copyasvalueref(t_right);

    r_left = t_left;
    r_right = t_right;

    return true;
}
#endif

#ifdef /* MCGreaterThanEqual */ LEGACY_EXEC
Exec_stat MCGreaterThanEqual::eval(MCExecPoint &ep)
{
    MCAutoValueRef t_left, t_right;
    if (!eval_comparison_factors(ep, left, right, &t_left, &t_right))
        return ES_ERROR;

    MCExecContext ctxt(ep);

    bool t_result;
    MCLogicEvalIsGreaterThanOrEqualTo(ctxt, *t_left, *t_right, t_result);
    if (!ctxt . HasError())
    {
        ep . setboolean(t_result);
        return ES_NORMAL;
    }

    return ctxt . Catch(line, pos);
}
#endif /* MCGreaterThanEqual */

#ifdef /* MCLessThan */ LEGACY_EXEC
Exec_stat MCLessThan::eval(MCExecPoint &ep)
{
    MCAutoValueRef t_left, t_right;
    if (!eval_comparison_factors(ep, left, right, &t_left, &t_right))
        return ES_ERROR;

    MCExecContext ctxt(ep);

    bool t_result;
    MCLogicEvalIsLessThan(ctxt, *t_left, *t_right, t_result);
    if (!ctxt . HasError())
    {
        ep . setboolean(t_result);
        return ES_NORMAL;
    }

    return ctxt . Catch(line, pos);
}
#endif /* MCLessThan */

#ifdef /* MCLessThanEqual */ LEGACY_EXEC
Exec_stat MCLessThanEqual::eval(MCExecPoint &ep)
{
    MCAutoValueRef t_left, t_right;
    if (!eval_comparison_factors(ep, left, right, &t_left, &t_right))
        return ES_ERROR;

    MCExecContext ctxt(ep);

    bool t_result;
    MCLogicEvalIsLessThanOrEqualTo(ctxt, *t_left, *t_right, t_result);
    if (!ctxt . HasError())
    {
        ep . setboolean(t_result);
        return ES_NORMAL;
    }

    return ctxt . Catch(line, pos);
}
#endif /* MCLessThanEqual */

#ifdef /* MCNotEqual */ LEGACY_EXEC
Exec_stat MCNotEqual::eval(MCExecPoint &ep)
{
	// MW-2014-01-30: [[ Bug 11732 ]] Enable array comparison mode - this stops
	//   auto-conversion of arrays to empty.
	int2 i;
	if (compare(ep, i, true) != ES_NORMAL)
	{
		MCeerror->add
		(EE_NOTEQUAL_OPS, line, pos);
		return ES_ERROR;
	}
	ep.setboolean(i != 0);
	return ES_NORMAL;
}
#endif /* MCNotEqual */

///////////////////////////////////////////////////////////////////////////////
//
//  Miscellaneous operators
//

void MCGrouping::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    if (right != NULL)
    {
        MCValueRef t_value;
        if (!ctxt . EvaluateExpression(right, EE_GROUPING_BADRIGHT, r_value))
            return;
    }
    else
        ctxt . LegacyThrow(EE_GROUPING_BADRIGHT);
}

void MCGrouping::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	right -> compile(ctxt);
	MCSyntaxFactoryEvalResult(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

Parse_stat MCIs::parse(MCScriptPoint &sp, Boolean the)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_UNOP, O_NOT) == PS_NORMAL)
		form = IT_NOT;
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_IS_NORIGHT, sp);
		return PS_ERROR;
	}
	if (type != ST_ID || sp.lookup(SP_FACTOR, te) != PS_NORMAL)
	{
		if (type != ST_ID || sp.lookup(SP_VALIDATION, te) != PS_NORMAL)
		{
			sp.backup();
			return PS_NORMAL;
		}
		if (te->which == IV_UNDEFINED)
		{
			if (sp.next(type) != PS_NORMAL)
			{
				MCperror->add(PE_IS_NOVALIDTYPE, sp);
				return PS_ERROR;
			}
			if (type != ST_ID || sp.lookup(SP_VALIDATION, te) != PS_NORMAL)
			{
				MCperror->add(PE_IS_BADVALIDTYPE, sp);
				return PS_ERROR;
			}
            
            valid = (Is_validation)te->which;
			
			// MERG-2013-06-24: [[ IsAnAsciiString ]] Parse 'is an ascii string'.
            if (te->which == IV_ASCII)
            {
				if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_STRING) != PS_NORMAL)
                {
                	MCperror->add(PE_IS_BADVALIDTYPE, sp);
                    return PS_ERROR;
                }
            }
			
			return PS_BREAK;
		}
		else
			if (te->which == IV_AMONG)
			{
				sp.skip_token(SP_FACTOR, TT_THE);
				if (sp.next(type) != PS_NORMAL)
				{
					MCperror->add(PE_IS_NOVALIDTYPE, sp);
					return PS_ERROR;
				}
				if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || (te->type != TT_CLASS && (te->type != TT_FUNCTION || te->which != F_KEYS)))
				{
					MCperror->add(PE_IS_BADAMONGTYPE, sp);
					return PS_ERROR;
				}
				
				if (te -> type == TT_FUNCTION && te -> which == F_KEYS)
					delimiter = CT_KEY;
				else if (te -> type == TT_CLASS)
					delimiter = (Chunk_term)te -> which;
				else
					MCUnreachable();

				if (delimiter == CT_CHARACTER)
					if (form == IT_NOT)
						form = IT_NOT_IN;
					else
						form = IT_IN;
				else
					if (form == IT_NOT)
						form = IT_NOT_AMONG;
					else
						form = IT_AMONG;

				sp.skip_token(SP_FACTOR, TT_OF);

				// Support for 'is among the keys of the dragData'
				if (delimiter == CT_KEY)
				{
					if (sp . skip_token(SP_FACTOR, TT_THE) == PS_NORMAL)
					{
						Symbol_type type;
						const LT *te;
						if (sp . next(type) == PS_NORMAL)
						{
							if ((sp.lookup(SP_FACTOR, te) == PS_NORMAL
									&& (te->which == P_DRAG_DATA || te->which == P_CLIPBOARD_DATA)))
							{
								if (te -> which == P_CLIPBOARD_DATA)
								{
									if (form == IT_NOT_AMONG)
										form = IT_NOT_AMONG_THE_CLIPBOARD_DATA;
									else
										form = IT_AMONG_THE_CLIPBOARD_DATA;
								}
								else
								{
									if (form == IT_NOT_AMONG)
										form = IT_NOT_AMONG_THE_DRAG_DATA;
									else
										form = IT_AMONG_THE_DRAG_DATA;
								}
								left = NULL;
								return PS_BREAK;
							}
							sp . backup();
						}
						sp . backup();
					}
				}
			}
			else
			{
				MCperror->add(PE_IS_NOVALIDTYPE, sp);
				return PS_ERROR;
			}
		return PS_NORMAL;
	}
   	if (te->type == TT_IN)
	{
		rank = FR_COMPARISON;
		if (form == IT_NOT)
			form = IT_NOT_IN;
		else
			form = IT_IN;
	}
	else
		if (te->type == TT_FUNCTION && te->which == F_WITHIN)
		{
			if (form == IT_NOT)
				form = IT_NOT_WITHIN;
			else
				form = IT_WITHIN;
		}
		else
			sp.backup();
	return PS_NORMAL;
}

#if 0
Exec_stat MCIs::eval(MCExecPoint &ep)
{
#ifdef /* MCIs */ LEGACY_EXEC
	// Implementation of 'is a <type>'
	if (valid != IV_UNDEFINED)
	{
		int2 i2;
		Boolean cond = False;
		if (right->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_IS_BADLEFT, line, pos);
			return ES_ERROR;
		}
		// more beef up for Jan's "is an integer" bug
		if (valid == IV_ARRAY)
			cond = (ep . getformat() == VF_ARRAY);
		else if (((ep.getformat() == VF_STRING || ep.getformat() == VF_BOTH )
				  && ep.getsvalue().getlength())
				 || ep.getformat() == VF_NUMBER)
			switch (valid)
			{
			case IV_COLOR:
				{
					MCColor c;
					char *cname = NULL;
					cond = MCscreen->parsecolor(ep.getsvalue(), &c, &cname);
					delete cname;
				}
				break;
			case IV_DATE:
				cond = MCD_convert(ep, CF_UNDEFINED, CF_UNDEFINED,
				                   CF_SECONDS, CF_UNDEFINED);
				break;
			case IV_INTEGER:
				// SMR Jan's "is an integer" bug
				if (ep.ton() != ES_NORMAL)
					cond = False;
				else
					cond = floor(ep.getnvalue()) == ep.getnvalue();
				break;
			case IV_LOGICAL:
				cond = ep.getsvalue() == MCtruemcstring
				       || ep.getsvalue() == MCfalsemcstring;
				break;
			case IV_NUMBER:
				cond = ep.ton() == ES_NORMAL;
				break;
			case IV_POINT:
				cond = MCU_stoi2x2(ep.getsvalue(), i2, i2);
				break;
			case IV_RECT:
				cond = MCU_stoi2x4(ep.getsvalue(), i2, i2, i2, i2);
				break;
			// MERG-2013-06-24: [[ IsAnAsciiString ]] Implementation for ascii string
			//   check.
            case IV_ASCII:
                {
                    cond = True;
                    uint1* t_string = (uint1 *) ep.getsvalue().getstring();
                    int t_length = ep.getsvalue().getlength();
                    for (int i=0; i < t_length ;i++)
                        if (t_string[i] > 127)
                        {
                            cond = False;
                            break;
                        }
                }
                break;
			default:
				break;
			}
		if (form == IT_NOT)
			cond = !cond;
		ep.setboolean(cond);
		return ES_NORMAL;
	}

	Boolean match = False;
	uint4 i;
	int2 value;
	
	// Implementation of 'is'
	if (form == IT_NORMAL || form == IT_NOT)
	{
		if (compare(ep, value, true) != ES_NORMAL)
		{
			MCeerror->add
			(EE_IS_BADOPS, line, pos);
			return ES_ERROR;
		}
		if (form == IT_NORMAL)
			match = value == 0;
		else
			match = value != 0;
		ep.setboolean(match);
		return ES_NORMAL;
	}

	// If 'is among the clipboardData' then left is NULL
	MCExpression *t_left, *t_right;
	if (left == NULL)
		t_left = right, t_right = NULL;
	else
		t_left = left, t_right = right;

	if (t_left->eval(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_IS_BADLEFT, line, pos);
		return ES_ERROR;
	}
	
	ep.grabsvalue();
	
	MCExecPoint ep2(ep);
	if (t_right != NULL)
	{
		if (t_right->eval(ep2) != ES_NORMAL)
		{
			MCeerror->add(EE_IS_BADRIGHT, line, pos);
			return ES_ERROR;
		}
	}

	// The rest
	switch (form)
	{
	case IT_AMONG:
	case IT_NOT_AMONG:
		if (delimiter == CT_KEY)
		{
			// MW-2008-06-30: [[ Bug ]] For consistency with arrays elsewhere,
			//   any non-array should be treated as the empty array
			if (ep2 . getformat() != VF_ARRAY)
				match = False;
			else
			{
				MCVariableValue *t_array;
				t_array = ep2 . getarray();
				
				match = t_array -> has_element(ep, ep . getsvalue());
			}
		}
		else if (delimiter == CT_TOKEN)
		{
			match = False;
			MCScriptPoint sp(ep2.getsvalue());
			Parse_stat ps = sp.nexttoken();
			while (ps != PS_ERROR && ps != PS_EOF)
				if (sp.gettoken() == ep.getsvalue())
				{
					match = True;
					break;
				}
				else
					ps = sp.nexttoken();
		}
		else
		{
			MCString w = ep2.getsvalue();
			Boolean whole = True;
			if (ep.getsvalue().getlength() == 0 && w.getlength()
			        && (delimiter == CT_LINE || delimiter == CT_ITEM))
			{
				whole = False;
				MCString p;
				char c;
				if (delimiter == CT_LINE)
				{
					c = '\n';
					p.set("\n\n", 2);
				}
				else
				{
					c = ',';
					p.set(",,", 2);
				}
				uint4 offset;
				if (w.getstring()[0] == c || w.getstring()[w.getlength() - 1] == c
				        || MCU_offset(p, w, offset, ep.getcasesensitive()))
					match = True;
				else
					match = False;
			}
			else
			{
				MCU_chunk_offset(ep, w, whole, delimiter);
				match = ep.getnvalue() != 0.0;
			}
		}
		if (form == IT_NOT_AMONG)
			match = !match;
		break;
	case IT_AMONG_THE_CLIPBOARD_DATA:
	case IT_NOT_AMONG_THE_CLIPBOARD_DATA:
	case IT_AMONG_THE_DRAG_DATA:
	case IT_NOT_AMONG_THE_DRAG_DATA:
		{
			MCTransferData *t_data;
			if (form == IT_AMONG_THE_CLIPBOARD_DATA || form == IT_NOT_AMONG_THE_CLIPBOARD_DATA)
				t_data = MCclipboarddata;
			else
				t_data = MCdragdata;

			match = t_data -> Contains(MCTransferData::StringToType(ep . getsvalue()), true);

			if (form == IT_NOT_AMONG_THE_CLIPBOARD_DATA || form == IT_NOT_AMONG_THE_DRAG_DATA)
				match = !match;
		}
		break;
	case IT_IN:
	case IT_NOT_IN:
		match = MCU_offset(ep.getsvalue(), ep2.getsvalue(),
		                   i, ep.getcasesensitive());
		if (form == IT_NOT_IN)
			match = !match;
		break;
	case IT_WITHIN:
	case IT_NOT_WITHIN:
		int2 i1, i2, i3, i4, i5, i6;
		if (!MCU_stoi2x2(ep.getsvalue(), i1, i2))
		{
			MCeerror->add
			(EE_IS_WITHINNAP, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
		if (!MCU_stoi2x4(ep2.getsvalue(), i3, i4, i5, i6))
		{
			MCeerror->add
			(EE_IS_WITHINNAR, line, pos, ep2.getsvalue());
			return ES_ERROR;
		}
		// MW-2007-01-08: [[ Bug 5745 ]] For consistency across Revolution and also with
		//   HyperCard, 'is within' should *not* include bottom and right edges.
		match = i1 >= i3 && i1 < i5 && i2 >= i4 && i2 < i6;
		if (form == IT_NOT_WITHIN)
			match = !match;
		break;
	default:
		break;
	}
	ep.setboolean(match);
	return ES_NORMAL;
#endif /* MCIs */

	MCExecContext ctxt(ep);
	bool t_result;

	// Implementation of 'is a <type>'
	if (valid != IV_UNDEFINED)
	{
		MCAutoValueRef t_value;

		if (right->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_IS_BADLEFT, line, pos);
			return ES_ERROR;
		}
		/* UNCHECKED */ ep.copyasvalueref(&t_value);

		switch (valid)
		{
		case IV_ARRAY:
			if (form == IT_NORMAL)
				MCArraysEvalIsAnArray(ctxt, *t_value, t_result);
			else
				MCArraysEvalIsNotAnArray(ctxt, *t_value, t_result);
			break;
		case IV_COLOR:
			if (form == IT_NORMAL)
				MCGraphicsEvalIsAColor(ctxt, *t_value, t_result);
			else
				MCGraphicsEvalIsNotAColor(ctxt, *t_value, t_result);
			break;
		case IV_DATE:
			if (form == IT_NORMAL)
				MCDateTimeEvalIsADate(ctxt, *t_value, t_result);
			else
				MCDateTimeEvalIsNotADate(ctxt, *t_value, t_result);
			break;
		case IV_INTEGER:
			if (form == IT_NORMAL)
				MCMathEvalIsAnInteger(ctxt, *t_value, t_result);
			else
				MCMathEvalIsNotAnInteger(ctxt, *t_value, t_result);
			break;
		case IV_NUMBER:
			if (form == IT_NORMAL)
				MCMathEvalIsANumber(ctxt, *t_value, t_result);
			else
				MCMathEvalIsNotANumber(ctxt, *t_value, t_result);
			break;
		case IV_LOGICAL:
			if (form == IT_NORMAL)
				MCLogicEvalIsABoolean(ctxt, *t_value, t_result);
			else
				MCLogicEvalIsNotABoolean(ctxt, *t_value, t_result);
			break;
		case IV_POINT:
			if (form == IT_NORMAL)
				MCGraphicsEvalIsAPoint(ctxt, *t_value, t_result);
			else
				MCGraphicsEvalIsNotAPoint(ctxt, *t_value, t_result);
			break;
		case IV_RECT:
			if (form == IT_NORMAL)
				MCGraphicsEvalIsARectangle(ctxt, *t_value, t_result);
			else
				MCGraphicsEvalIsNotARectangle(ctxt, *t_value, t_result);
			break;
        // MERG-2013-06-24: [[ IsAnAsciiString ]] Implementation for ascii string
        //   check.
        case IV_ASCII:
            if (form == IT_NORMAL)
                MCStringsEvalIsAscii(ctxt, *t_value, t_result);
            else
                MCStringsEvalIsNotAscii(ctxt, *t_value, t_result);
            break;
        }

		if (!ctxt . HasError())
		{
			ep . setboolean(t_result);
			return ES_NORMAL;
		}

		return ctxt . Catch(line, pos);
	}

	// Implementation of 'is'
	if (form == IT_NORMAL || form == IT_NOT)
	{
		MCAutoValueRef t_left, t_right;
		if (!eval_comparison_factors(ep, left, right, &t_left, &t_right))
			return ES_ERROR;

		if (form == IT_NORMAL)
			MCLogicEvalIsEqualTo(ctxt, *t_left, *t_right, t_result);
		else
			MCLogicEvalIsNotEqualTo(ctxt, *t_left, *t_right, t_result);
	}


	// The rest
	switch (form)
	{
	case IT_AMONG:
	case IT_NOT_AMONG:
		if (delimiter == CT_KEY)
		{
			MCAutoArrayRef t_array;
			MCNewAutoNameRef t_name;

			if (left->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IS_BADLEFT, line, pos);
				return ES_ERROR;
			}
			/* UNCHECKED */ ep . copyasnameref(&t_name);

			if (right->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IS_BADRIGHT, line, pos);
				return ES_ERROR;
			}
			/* UNCHECKED */ ep . copyasarrayref(&t_array);

			if (form == IT_AMONG)
				MCArraysEvalIsAmongTheKeysOf(ctxt, *t_name, *t_array, t_result);
			else
				MCArraysEvalIsNotAmongTheKeysOf(ctxt, *t_name, *t_array, t_result);
		}
		else
		{
			MCAutoStringRef t_left, t_right;

			if (left->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IS_BADLEFT, line, pos);
				return ES_ERROR;
			}
			/* UNCHECKED */ ep . copyasstringref(&t_left);

			if (right->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IS_BADRIGHT, line, pos);
				return ES_ERROR;
			}
			/* UNCHECKED */ ep . copyasstringref(&t_right);

			switch (delimiter)
			{
			case CT_TOKEN:
				if (form == IT_AMONG)
					MCStringsEvalIsAmongTheTokensOf(ctxt, *t_left, *t_right, t_result);
				else
					MCStringsEvalIsNotAmongTheTokensOf(ctxt, *t_left, *t_right, t_result);
				break;
			case CT_WORD:
				if (form == IT_AMONG)
					MCStringsEvalIsAmongTheWordsOf(ctxt, *t_left, *t_right, t_result);
				else
					MCStringsEvalIsNotAmongTheWordsOf(ctxt, *t_left, *t_right, t_result);
				break;
			case CT_LINE:
				if (form == IT_AMONG)
					MCStringsEvalIsAmongTheLinesOf(ctxt, *t_left, *t_right, t_result);
				else
					MCStringsEvalIsNotAmongTheLinesOf(ctxt, *t_left, *t_right, t_result);
				break;
			case CT_ITEM:
				if (form == IT_AMONG)
					MCStringsEvalIsAmongTheItemsOf(ctxt, *t_left, *t_right, t_result);
				else
					MCStringsEvalIsNotAmongTheItemsOf(ctxt, *t_left, *t_right, t_result);
				break;
			}
		}
		break;
	case IT_AMONG_THE_CLIPBOARD_DATA:
	case IT_NOT_AMONG_THE_CLIPBOARD_DATA:
	case IT_AMONG_THE_DRAG_DATA:
	case IT_NOT_AMONG_THE_DRAG_DATA:
		{
			MCNewAutoNameRef t_right;

			// If 'is among the clipboardData' then left is NULL
			if (right->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IS_BADLEFT, line, pos);
				return ES_ERROR;
			}
			/* UNCHECKED */ ep . copyasnameref(&t_right);

			if (form == IT_AMONG_THE_CLIPBOARD_DATA)
				MCPasteboardEvalIsAmongTheKeysOfTheClipboardData(ctxt, *t_right, t_result);
			else if (form == IT_NOT_AMONG_THE_CLIPBOARD_DATA)
				MCPasteboardEvalIsNotAmongTheKeysOfTheClipboardData(ctxt, *t_right, t_result);
			else if (form == IT_AMONG_THE_DRAG_DATA)
				MCPasteboardEvalIsAmongTheKeysOfTheDragData(ctxt, *t_right, t_result);
			else if (form == IT_NOT_AMONG_THE_DRAG_DATA)
				MCPasteboardEvalIsNotAmongTheKeysOfTheDragData(ctxt, *t_right, t_result);
		}
		break;
	case IT_IN:
	case IT_NOT_IN:
		{
			MCAutoStringRef t_left, t_right;

			if (left->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IS_BADLEFT, line, pos);
				return ES_ERROR;
			}
			/* UNCHECKED */ ep . copyasstringref(&t_left);

			if (right->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IS_BADRIGHT, line, pos);
				return ES_ERROR;
			}
			/* UNCHECKED */ ep . copyasstringref(&t_right);

			if (form == IT_IN)
				MCStringsEvalContains(ctxt, *t_right, *t_left, t_result);
			else
				MCStringsEvalDoesNotContain(ctxt, *t_right, *t_left,t_result);
		}
		break;
	case IT_WITHIN:
	case IT_NOT_WITHIN:
		{
			MCPoint t_point;
			if (left->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IS_BADLEFT, line, pos);
				return ES_ERROR;
			}
			if (!ep . copyaslegacypoint(t_point))
			{
				MCeerror -> add(EE_IS_WITHINNAP, line, pos, ep . getsvalue());
				return ES_ERROR;
			}

			MCRectangle t_rectangle;
			if (right->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_IS_BADRIGHT, line, pos);
				return ES_ERROR;
			}
			if (!ep . copyaslegacyrectangle(t_rectangle))
			{
				MCeerror -> add(EE_IS_WITHINNAR, line, pos, ep . getsvalue());
				return ES_ERROR;
			}

			if (form == IT_WITHIN)
				MCGraphicsEvalIsWithin(ctxt, t_point, t_rectangle, t_result);
			else
				MCGraphicsEvalIsNotWithin(ctxt, t_point, t_rectangle, t_result);
		}
		break;
	default:
		break;
	}

	if (!ctxt . HasError())
	{
		ep . setboolean(t_result);
		return ES_NORMAL;
	}

	return ctxt . Catch(line, pos);
}
#else

void MCIs::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    bool t_result;

    // Implementation of 'is a <type>'
    if (valid != IV_UNDEFINED)
    {
        MCAutoValueRef t_value;

        if (!ctxt . EvalExprAsValueRef(right, EE_IS_BADLEFT, &t_value))
            return;

        switch (valid)
        {
        case IV_ARRAY:
            if (form == IT_NORMAL)
                MCArraysEvalIsAnArray(ctxt, *t_value, t_result);
            else
                MCArraysEvalIsNotAnArray(ctxt, *t_value, t_result);
            break;
        case IV_COLOR:
            if (form == IT_NORMAL)
                MCGraphicsEvalIsAColor(ctxt, *t_value, t_result);
            else
                MCGraphicsEvalIsNotAColor(ctxt, *t_value, t_result);
            break;
        case IV_DATE:
            if (form == IT_NORMAL)
                MCDateTimeEvalIsADate(ctxt, *t_value, t_result);
            else
                MCDateTimeEvalIsNotADate(ctxt, *t_value, t_result);
            break;
        case IV_INTEGER:
            if (form == IT_NORMAL)
                MCMathEvalIsAnInteger(ctxt, *t_value, t_result);
            else
                MCMathEvalIsNotAnInteger(ctxt, *t_value, t_result);
            break;
        case IV_NUMBER:
            if (form == IT_NORMAL)
                MCMathEvalIsANumber(ctxt, *t_value, t_result);
            else
                MCMathEvalIsNotANumber(ctxt, *t_value, t_result);
            break;
        case IV_LOGICAL:
            if (form == IT_NORMAL)
                MCLogicEvalIsABoolean(ctxt, *t_value, t_result);
            else
                MCLogicEvalIsNotABoolean(ctxt, *t_value, t_result);
            break;
        case IV_POINT:
            if (form == IT_NORMAL)
                MCGraphicsEvalIsAPoint(ctxt, *t_value, t_result);
            else
                MCGraphicsEvalIsNotAPoint(ctxt, *t_value, t_result);
            break;
        case IV_RECT:
            if (form == IT_NORMAL)
                MCGraphicsEvalIsARectangle(ctxt, *t_value, t_result);
            else
                MCGraphicsEvalIsNotARectangle(ctxt, *t_value, t_result);
            break;
        // MERG-2013-06-24: [[ IsAnAsciiString ]] Implementation for ascii string
        //   check.
        case IV_ASCII:
            if (form == IT_NORMAL)
                MCStringsEvalIsAscii(ctxt, *t_value, t_result);
            else
                MCStringsEvalIsNotAscii(ctxt, *t_value, t_result);
            break;
        }

        if (!ctxt . HasError())
            MCExecValueTraits<bool>::set(r_value, t_result);

        return;
    }

    // Implementation of 'is'
    if (form == IT_NORMAL || form == IT_NOT)
    {
        MCAutoValueRef t_left, t_right;

        if (!ctxt . EvalExprAsValueRef(left, EE_FACTOR_BADLEFT, &t_left)
                || !ctxt . EvalExprAsValueRef(right, EE_FACTOR_BADRIGHT, &t_right))
            return;

        if (form == IT_NORMAL)
            MCLogicEvalIsEqualTo(ctxt, *t_left, *t_right, t_result);
        else
            MCLogicEvalIsNotEqualTo(ctxt, *t_left, *t_right, t_result);
    }


    // The rest
    switch (form)
    {
    case IT_AMONG:
    case IT_NOT_AMONG:
        if (delimiter == CT_KEY)
        {
            MCAutoArrayRef t_array;
            MCNewAutoNameRef t_name;

            if (!ctxt . EvalExprAsNameRef(left, EE_IS_BADLEFT, &t_name))
                return;

            ctxt . TryToEvalExprAsArrayRef(right, EE_IS_BADRIGHT, &t_array);

            if (form == IT_AMONG)
                MCArraysEvalIsAmongTheKeysOf(ctxt, *t_name, *t_array, t_result);
            else
                MCArraysEvalIsNotAmongTheKeysOf(ctxt, *t_name, *t_array, t_result);
        }
        else if (delimiter == CT_BYTE)
        {
            MCAutoDataRef t_left, t_right;
            
            if (!ctxt . EvalExprAsDataRef(left, EE_IS_BADLEFT, &t_left))
                return;

            if (!ctxt . EvalExprAsDataRef(right, EE_IS_BADRIGHT, &t_right))
                return;
            
            if (form == IT_AMONG)
                MCStringsEvalIsAmongTheBytesOf(ctxt, *t_left, *t_right, t_result);
            else
                MCStringsEvalIsNotAmongTheBytesOf(ctxt, *t_left, *t_right, t_result);
            break;
        }
        else
        {
            MCAutoStringRef t_left, t_right;

            if (!ctxt . EvalExprAsStringRef(left, EE_IS_BADLEFT, &t_left))
                return;

            if (!ctxt . EvalExprAsStringRef(right, EE_IS_BADRIGHT, &t_right))
                return;

            switch (delimiter)
            {
            case CT_TOKEN:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheTokensOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheTokensOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_WORD:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheWordsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheWordsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_LINE:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheLinesOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheLinesOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_ITEM:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheItemsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheItemsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_PARAGRAPH:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheParagraphsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheParagraphsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_SENTENCE:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheSentencesOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheSentencesOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_TRUEWORD:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheTrueWordsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheTrueWordsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_CODEPOINT:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheCodepointsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheCodepointsOf(ctxt, *t_left, *t_right, t_result);
                break;
            case CT_CODEUNIT:
                if (form == IT_AMONG)
                    MCStringsEvalIsAmongTheCodeunitsOf(ctxt, *t_left, *t_right, t_result);
                else
                    MCStringsEvalIsNotAmongTheCodeunitsOf(ctxt, *t_left, *t_right, t_result);
                break;
            }
        }
        break;
    case IT_AMONG_THE_CLIPBOARD_DATA:
    case IT_NOT_AMONG_THE_CLIPBOARD_DATA:
    case IT_AMONG_THE_DRAG_DATA:
    case IT_NOT_AMONG_THE_DRAG_DATA:
        {
            MCNewAutoNameRef t_right;

            if (!ctxt . EvalExprAsNameRef(right, EE_IS_BADLEFT, &t_right))
                return;

            if (form == IT_AMONG_THE_CLIPBOARD_DATA)
                MCPasteboardEvalIsAmongTheKeysOfTheClipboardData(ctxt, *t_right, t_result);
            else if (form == IT_NOT_AMONG_THE_CLIPBOARD_DATA)
                MCPasteboardEvalIsNotAmongTheKeysOfTheClipboardData(ctxt, *t_right, t_result);
            else if (form == IT_AMONG_THE_DRAG_DATA)
                MCPasteboardEvalIsAmongTheKeysOfTheDragData(ctxt, *t_right, t_result);
            else if (form == IT_NOT_AMONG_THE_DRAG_DATA)
                MCPasteboardEvalIsNotAmongTheKeysOfTheDragData(ctxt, *t_right, t_result);
        }
        break;
    case IT_IN:
    case IT_NOT_IN:
        {
            MCAutoStringRef t_left, t_right;

            if (!ctxt . EvalExprAsStringRef(left, EE_IS_BADLEFT, &t_left))
                return;

            if (!ctxt . EvalExprAsStringRef(right, EE_IS_BADRIGHT, &t_right))
                return;

            if (form == IT_IN)
                MCStringsEvalContains(ctxt, *t_right, *t_left, t_result);
            else
                MCStringsEvalDoesNotContain(ctxt, *t_right, *t_left,t_result);
        }
        break;
    case IT_WITHIN:
    case IT_NOT_WITHIN:
        {
            MCPoint t_point;
            MCRectangle t_rectangle;

            if (!ctxt . EvalExprAsPoint(left, EE_IS_BADLEFT, t_point))
                return;

            if (!ctxt . EvalExprAsRectangle(right, EE_IS_BADRIGHT, t_rectangle))
                return;

            if (form == IT_WITHIN)
                MCGraphicsEvalIsWithin(ctxt, t_point, t_rectangle, t_result);
            else
                MCGraphicsEvalIsNotWithin(ctxt, t_point, t_rectangle, t_result);
        }
        break;
    default:
        break;
    }

    if (!ctxt . HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}

#endif

void MCIs::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	
	if (valid != IV_UNDEFINED)
	{
		MCExecMethodInfo *t_method;
		switch (valid)
		{
			case IV_ARRAY:
				t_method = form == IT_NORMAL ? kMCArraysEvalIsAnArrayMethodInfo : kMCArraysEvalIsNotAnArrayMethodInfo;
				break;
			case IV_COLOR:
				t_method = form == IT_NORMAL ? kMCGraphicsEvalIsAColorMethodInfo : kMCGraphicsEvalIsNotAColorMethodInfo;
				break;
			case IV_DATE:
				t_method = form == IT_NORMAL ? kMCDateTimeEvalIsADateMethodInfo : kMCDateTimeEvalIsNotADateMethodInfo;
				break;
			case IV_INTEGER:
				t_method = form == IT_NORMAL ? kMCMathEvalIsAnIntegerMethodInfo : kMCMathEvalIsNotAnIntegerMethodInfo;
				break;
			case IV_NUMBER:
				t_method = form == IT_NORMAL ? kMCMathEvalIsANumberMethodInfo : kMCMathEvalIsANumberMethodInfo;
				break;
			case IV_LOGICAL:
				t_method = form == IT_NORMAL ? kMCLogicEvalIsABooleanMethodInfo : kMCLogicEvalIsNotABooleanMethodInfo;
				break;
			case IV_POINT:
				t_method = form == IT_NORMAL ? kMCGraphicsEvalIsAPointMethodInfo : kMCGraphicsEvalIsNotAPointMethodInfo;
				break;
			case IV_RECT:
				t_method = form == IT_NORMAL ? kMCGraphicsEvalIsARectangleMethodInfo : kMCGraphicsEvalIsNotARectangleMethodInfo;
				break;
			default:
				MCAssert(false);
				break;
		}
		
		right -> compile(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, t_method);
	}
	else
	{
		MCExecMethodInfo *t_method;
		bool t_is_unary;
		t_is_unary = false;
		switch (form)
		{
			case IT_NORMAL:
				t_method = kMCLogicEvalIsEqualToMethodInfo;
				break;
			case IT_NOT:
				t_method = kMCLogicEvalIsNotEqualToMethodInfo;
				break;
			case IT_AMONG:
			case IT_NOT_AMONG:
				switch(delimiter)
				{
					case CT_KEY:
						t_method = form == IT_AMONG ? kMCArraysEvalIsAmongTheKeysOfMethodInfo : kMCArraysEvalIsNotAmongTheKeysOfMethodInfo;
						break;
					case CT_TOKEN:
						t_method = form == IT_AMONG ? kMCStringsEvalIsAmongTheTokensOfMethodInfo : kMCStringsEvalIsNotAmongTheTokensOfMethodInfo;
						break;
					case CT_WORD:
						t_method = form == IT_AMONG ? kMCStringsEvalIsAmongTheWordsOfMethodInfo : kMCStringsEvalIsNotAmongTheWordsOfMethodInfo;
						break;
					case CT_LINE:
						t_method = form == IT_AMONG ? kMCStringsEvalIsAmongTheLinesOfMethodInfo : kMCStringsEvalIsNotAmongTheLinesOfMethodInfo;
						break;
					case CT_ITEM:
						t_method = form == IT_AMONG ? kMCStringsEvalIsAmongTheItemsOfMethodInfo : kMCStringsEvalIsNotAmongTheItemsOfMethodInfo;
						break;
					default:
						MCAssert(false);
				}
				break;
			case IT_IN:
				t_method = kMCStringsEvalContainsMethodInfo;
				break;
			case IT_NOT_IN:
				t_method = kMCStringsEvalDoesNotContainMethodInfo;
				break;
			case IT_WITHIN:
				t_method = kMCGraphicsEvalIsWithinMethodInfo;
				break;
			case IT_NOT_WITHIN:
				t_method = kMCGraphicsEvalIsNotWithinMethodInfo;
				break;
			case IT_AMONG_THE_CLIPBOARD_DATA:
				t_method = kMCPasteboardEvalIsAmongTheKeysOfTheClipboardDataMethodInfo;
				t_is_unary = true;
				break;
			case IT_NOT_AMONG_THE_CLIPBOARD_DATA:
				t_method = kMCPasteboardEvalIsNotAmongTheKeysOfTheClipboardDataMethodInfo;
				t_is_unary = true;
				break;
			case IT_AMONG_THE_DRAG_DATA:
				t_method = kMCPasteboardEvalIsAmongTheKeysOfTheDragDataMethodInfo;
				t_is_unary = true;
				break;
			case IT_NOT_AMONG_THE_DRAG_DATA:
				t_method = kMCPasteboardEvalIsNotAmongTheKeysOfTheDragDataMethodInfo;
				t_is_unary = true;
				break;
			default:
				MCAssert(false);
				break;
		}				
		if (!t_is_unary)
			left -> compile(ctxt);
		right -> compile(ctxt);
		MCSyntaxFactoryEvalMethod(ctxt, t_method);
	}
	
	MCSyntaxFactoryEndExpression(ctxt);
}

MCThere::~MCThere()
{
	delete object;
}

Parse_stat MCThere::parse(MCScriptPoint &sp, Boolean the)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_BINOP, O_IS) != PS_NORMAL)
	{
		MCperror->add(PE_THERE_NOIS, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_UNOP, O_NOT) == PS_NORMAL)
		form = IT_NOT;
	sp.skip_token(SP_VALIDATION, TT_UNDEFINED, IV_UNDEFINED);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_THERE_NOOBJECT, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_THERE, te) != PS_NORMAL)
	{
		sp.backup();
		object = new MCChunk(False);
		if (object->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_THERE_NOOBJECT, sp);
			return PS_ERROR;
		}
		right = new MCExpression(); // satisfy check in scriptpt.parse
	}
	else
	{
		mode = (There_mode)te->which;
		if (sp.parseexp(True, False, &right) != PS_NORMAL)
		{
			MCperror->add(PE_THERE_BADFILE, sp);
			return PS_ERROR;
		}
	}
	return PS_BREAK;
}

void MCThere::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
#ifdef /* MCThere */ LEGACY_EXEC
	Boolean found;
	if (object == NULL)
	{
		if (right->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_THERE_BADSOURCE, line, pos);
			return ES_ERROR;
		}
		char *sptr = ep.getsvalue().clone();
		if (mode == TM_PROCESS)
		{
			if (MCsecuremode & MC_SECUREMODE_PROCESS)
			{
				MCeerror->add(EE_PROCESS_NOPERM, line, pos);
				return ES_ERROR;
			}
			uint2 index;
			found = IO_findprocess(sptr, index);
		}
		else
		{
			if (MCsecuremode & MC_SECUREMODE_DISK)
			{
				MCeerror->add(EE_DISK_NOPERM, line, pos);
				return ES_ERROR;
			}
			found = MCS_exists(sptr, mode == TM_FILE);
		}
		delete sptr;
	}
	else
	{
		MCObject *optr;
		uint4 parid;
		MCerrorlock++;
		found = object->getobj(ep, optr, parid, True) == ES_NORMAL;
		MCerrorlock--;
	}
	if (form == IT_NOT)
		found = !found;
	ep.setboolean(found);
	return ES_NORMAL;
#endif /* MCThere */

	bool t_result;

	if (object == NULL)
	{
        MCAutoStringRef t_string;

        if (!ctxt . EvalExprAsStringRef(right, EE_THERE_BADSOURCE, &t_string))
            return;

		switch (mode)
		{
		case TM_PROCESS:
			if (form == IT_NORMAL)
				MCFilesEvalThereIsAProcess(ctxt, *t_string, t_result);
			else
				MCFilesEvalThereIsNotAProcess(ctxt, *t_string, t_result);
			break;
		case TM_FILE:
			if (form == IT_NORMAL)
				MCFilesEvalThereIsAFile(ctxt, *t_string, t_result);
			else
				MCFilesEvalThereIsNotAFile(ctxt, *t_string, t_result);
			break;
		case TM_DIRECTORY:
			if (form == IT_NORMAL)
				MCFilesEvalThereIsAFolder(ctxt, *t_string, t_result);
			else
				MCFilesEvalThereIsNotAFolder(ctxt, *t_string, t_result);
			break;
		}
	}
	else
	{
		if (form == IT_NORMAL)
			MCInterfaceEvalThereIsAnObject(ctxt, object, t_result);
		else
			MCInterfaceEvalThereIsNotAnObject(ctxt, object, t_result);
	}

    if (!ctxt.HasError())
        MCExecValueTraits<bool>::set(r_value, t_result);
}

void MCThere::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	
	MCExecMethodInfo *t_method;
	if (object == nil)
	{
		switch(mode)
		{
			case TM_PROCESS:
				t_method = form == IT_NORMAL ? kMCFilesEvalThereIsAProcessMethodInfo : kMCFilesEvalThereIsNotAProcessMethodInfo;
				break;
			case TM_FILE:
				t_method = form == IT_NORMAL ? kMCFilesEvalThereIsAFileMethodInfo : kMCFilesEvalThereIsNotAFileMethodInfo;
				break;
			case TM_DIRECTORY:
				t_method = form == IT_NORMAL ? kMCFilesEvalThereIsAFolderMethodInfo : kMCFilesEvalThereIsNotAFolderMethodInfo;
				break;
			default:
				MCAssert(false);
				break;
		}
	}
	else
		t_method = form == IT_NORMAL ? kMCInterfaceEvalThereIsAnObjectMethodInfo : kMCInterfaceEvalThereIsNotAnObjectMethodInfo;
	
	right -> compile(ctxt);
	MCSyntaxFactoryEvalMethod(ctxt, t_method);
		
	MCSyntaxFactoryEndExpression(ctxt);
}
