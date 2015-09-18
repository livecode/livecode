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

//
// Operator class declarations
//
#ifndef	OPERATORS_H
#define	OPERATORS_H

#include "exec.h"
#include "executionerrors.h"

#include "express.h"

class MCChunk;

//////////

class MCUnaryOperator: public MCExpression
{
public:
	virtual MCExecMethodInfo *getmethodinfo(void) const = 0;
	
	virtual void compile(MCSyntaxFactoryRef factory);
};

class MCBinaryOperator: public MCExpression
{
public:
	virtual MCExecMethodInfo *getmethodinfo(void) const = 0;
	
	virtual void compile(MCSyntaxFactoryRef factory);
};

class MCMultiBinaryOperator: public MCExpression
{
public:
	virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const = 0;
	virtual bool canbeunary(void) const {return false;}
	
	virtual void compile(MCSyntaxFactoryRef factory);
};

//////////

template <typename ParamType,
          void (*EvalMethod)(MCExecContext&, typename MCExecValueTraits<ParamType>::in_type, typename MCExecValueTraits<ParamType>::out_type),
          Exec_errors EvalError,
          Factor_rank Rank,
          MCExecMethodInfo *&MethodInfo>
class MCUnaryOperatorCtxt: public MCUnaryOperator
{
public:
    MCUnaryOperatorCtxt()
    {
        rank = Rank;
    }

    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
    {
        ParamType t_right;
        ParamType t_result;

        if (!MCExecValueTraits<ParamType>::eval(ctxt, right, EvalError, t_right))
            return;

        EvalMethod(ctxt, t_right, t_result);

        MCExecValueTraits<ParamType>::release(t_right);

        if (!ctxt . HasError())
            MCExecValueTraits<ParamType>::set(r_value, t_result);
    }

   virtual MCExecMethodInfo *getmethodinfo(void) const { return MethodInfo; }
};

template<typename ParamType,
         typename ReturnType,
         void (*EvalMethod)(MCExecContext&, typename MCExecValueTraits<ParamType>::in_type, typename MCExecValueTraits<ParamType>::in_type, typename MCExecValueTraits<ReturnType>::out_type),
         Exec_errors EvalLeftError,
         Exec_errors EvalRightError,
         Factor_rank Rank,
         MCExecMethodInfo *&MethodInfo>
class MCBinaryOperatorCtxt: public MCBinaryOperator
{
public:
    MCBinaryOperatorCtxt()
    {
        rank = Rank;
    }

    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
    {
        ParamType t_right;
        ParamType t_left;
        ReturnType t_result;

        if (!MCExecValueTraits<ParamType>::eval(ctxt, left, EvalLeftError, t_left))
                return;

        if (!MCExecValueTraits<ParamType>::eval(ctxt, right, EvalRightError, t_right))
        {
            MCExecValueTraits<ParamType>::release(t_left);
            return;
        }

        EvalMethod(ctxt, t_left, t_right, t_result);

        MCExecValueTraits<ParamType>::release(t_left);
        MCExecValueTraits<ParamType>::release(t_right);

        if (!ctxt . HasError())
            MCExecValueTraits<ReturnType>::set(r_value, t_result);
    }

    virtual MCExecMethodInfo *getmethodinfo() const { return MethodInfo; }
};

template<void (*Eval)(MCExecContext&, real64_t, real64_t, real64_t&),
         void (*EvalArrayByNumber)(MCExecContext&, MCArrayRef, real64_t, MCArrayRef&),
         void (*EvalArrayByArray)(MCExecContext&, MCArrayRef, MCArrayRef, MCArrayRef&),
         Exec_errors EvalLeftError,
         Exec_errors EvalRightError,
         Exec_errors MismatchError,
         bool CanBeUnary,
         Factor_rank Rank,
         MCExecMethodInfo *&EvalNumberMethodInfo,
         MCExecMethodInfo *&EvalArrayByNumberMethodInfo,
         MCExecMethodInfo *&EvalArrayByArrayMethodInfo>
class MCMultiBinaryOperatorCtxt: public MCMultiBinaryOperator
{
public:
    MCMultiBinaryOperatorCtxt()
    {
        rank = Rank;
    }

    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
    {
        MCExecValue t_left, t_right;
        t_left . type = kMCExecValueTypeNone;
        t_right . type = kMCExecValueTypeNone;
        
        Boolean t_old_expectation;
        
        t_old_expectation = ctxt . GetNumberExpected();
        
        ctxt . SetNumberExpected(True);

        left -> eval_ctxt(ctxt, t_left);
        if (ctxt . HasError()
                || ! ctxt . ConvertToNumberOrArray(t_left))
        {
            ctxt . LegacyThrow(EvalLeftError);
            ctxt . SetNumberExpected(t_old_expectation);
            return;
        }

        right -> eval_ctxt(ctxt, t_right);
        if (ctxt . HasError()
                || !ctxt . ConvertToNumberOrArray(t_right))
        {
            ctxt . LegacyThrow(EvalRightError);
            if (t_left . type == kMCExecValueTypeArrayRef)
                MCValueRelease(t_left . arrayref_value);
            ctxt . SetNumberExpected(t_old_expectation);
            return;
        }
        
        // Set the numiber expectation back to its previous state
        ctxt . SetNumberExpected(t_old_expectation);

        r_value . valueref_value = nil;
        if (t_left . type == kMCExecValueTypeArrayRef)
        {
            if (t_right . type == kMCExecValueTypeArrayRef)
                EvalArrayByArray(ctxt, t_left . arrayref_value, t_right . arrayref_value, r_value . arrayref_value);
            else
                EvalArrayByNumber(ctxt, t_left . arrayref_value, t_right . double_value, r_value . arrayref_value);
        }
        else
        {
            if (t_right . type == kMCExecValueTypeArrayRef)
                ctxt . LegacyThrow(MismatchError);
            else
                Eval(ctxt, t_left . double_value, t_right . double_value, r_value . double_value);
        }
        
        if (ctxt . HasError())
            MCExecTypeRelease(r_value);
        else
            r_value . type = t_left . type;

        if (t_left . type == kMCExecValueTypeArrayRef)
            MCValueRelease(t_left . arrayref_value);
        if (t_right . type == kMCExecValueTypeArrayRef)
            MCValueRelease(t_right . arrayref_value);
    }

    virtual bool canbeunary() const { return CanBeUnary; }

    virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const
    {
        static MCExecMethodInfo *s_methods[] = { EvalNumberMethodInfo, EvalArrayByNumberMethodInfo, EvalArrayByArrayMethodInfo };
        r_methods = s_methods;
        r_count = 3;
    }
};

template<void (*Eval)(MCExecContext&, real64_t, real64_t, real64_t&),
         void (*EvalArrayByNumber)(MCExecContext&, MCArrayRef, real64_t, MCArrayRef&),
         void (*EvalArrayByArray)(MCExecContext&, MCArrayRef, MCArrayRef, MCArrayRef&),
         Exec_errors EvalLeftError,
         Exec_errors EvalRightError,
         bool CanBeUnary,
         Factor_rank Rank,
         MCExecMethodInfo *&EvalNumberMethodInfo,
         MCExecMethodInfo *&EvalArrayByNumberMethodInfo,
         MCExecMethodInfo *&EvalArrayByArrayMethodInfo>
class MCMultiBinaryCommutativeOperatorCtxt: public MCMultiBinaryOperator
{
public:
    MCMultiBinaryCommutativeOperatorCtxt()
    {
        rank = Rank;
    }

    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
    {
        MCExecValue t_left, t_right;
        Boolean t_old_expectation;
        
        t_old_expectation = ctxt . GetNumberExpected();
        ctxt . SetNumberExpected(True);

        // PM-2015-01-15: [[ Bug 14384 ]] Prevent a crash when putting +1 into variable
        if ((left != nil && (left -> eval_ctxt(ctxt, t_left), ctxt . HasError()))
                || !ctxt . ConvertToNumberOrArray(t_left))
        {
            ctxt . LegacyThrow(EvalLeftError);
            ctxt . SetNumberExpected(t_old_expectation);
            return;
        }

        if ((right != nil && (right -> eval_ctxt(ctxt, t_right), ctxt . HasError()))
                || !ctxt . ConvertToNumberOrArray(t_right))
        {
            ctxt . LegacyThrow(EvalRightError);
            if (t_left . type == kMCExecValueTypeArrayRef)
                MCValueRelease(t_left . valueref_value);
            ctxt . SetNumberExpected(t_old_expectation);
            return;
        }

        r_value . valueref_value = nil;
        if (t_left . type == kMCExecValueTypeArrayRef)
        {
            if (t_right . type == kMCExecValueTypeArrayRef)
                EvalArrayByArray(ctxt, t_left . arrayref_value, t_right . arrayref_value, r_value . arrayref_value);
            else
                EvalArrayByNumber(ctxt, t_left . arrayref_value, t_right . double_value, r_value . arrayref_value);
        }
        else
        {
            if (t_right . type == kMCExecValueTypeArrayRef)
                EvalArrayByNumber(ctxt, t_right . arrayref_value, t_left . double_value, r_value . arrayref_value);
            else
                Eval(ctxt, t_left . double_value, t_right . double_value, r_value . double_value);
        }
        
        // Set the number expectation back to its previous state
        ctxt . SetNumberExpected(t_old_expectation);
        
        if (ctxt . HasError())
            MCExecTypeRelease(r_value);
        else if (t_left . type == kMCExecValueTypeDouble && t_right . type == kMCExecValueTypeDouble)
            r_value . type = kMCExecValueTypeDouble;
        else
            r_value . type = kMCExecValueTypeArrayRef;

        if (t_left . type == kMCExecValueTypeArrayRef)
            MCValueRelease(t_left . valueref_value);
        if (t_right . type == kMCExecValueTypeArrayRef)
            MCValueRelease(t_right . valueref_value);
        
    }

    virtual bool canbeunary() const { return CanBeUnary; }

    virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const
    {
        static MCExecMethodInfo *s_methods[] = { EvalNumberMethodInfo, EvalArrayByNumberMethodInfo, EvalArrayByArrayMethodInfo };
        r_methods = s_methods;
        r_count = 3;
    }
};



//////////

class MCAnd : public MCExpression
{
public:
    MCAnd()
    {
        rank = FR_AND;
    }
    virtual void eval_ctxt(MCExecContext &, MCExecValue &r_value);
};

class MCAndBits : public MCBinaryOperatorCtxt<uinteger_t, uinteger_t, MCMathEvalBitwiseAnd, EE_ANDBITS_BADLEFT, EE_ANDBITS_BADRIGHT, FR_AND_BITS, kMCMathEvalBitwiseAndMethodInfo>
{};

class MCConcat : public MCExpression
{
public:
	MCConcat()
	{
		rank = FR_CONCAT;
    }
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
	virtual void compile(MCSyntaxFactoryRef ctxt);
};

class MCConcatSpace : public MCBinaryOperatorCtxt<MCStringRef, MCStringRef, MCStringsEvalConcatenateWithSpace, EE_CONCATSPACE_BADLEFT, EE_CONCATSPACE_BADRIGHT, FR_CONCAT, kMCStringsEvalConcatenateWithSpaceMethodInfo>
{};

class MCContains : public MCBinaryOperatorCtxt<MCStringRef, bool, MCStringsEvalContains, EE_CONTAINS_BADLEFT, EE_CONTAINS_BADRIGHT, FR_COMPARISON, kMCStringsEvalContainsMethodInfo>
{};

class MCDiv : public MCMultiBinaryOperatorCtxt<
        MCMathEvalDiv,
        MCMathEvalDivArrayByNumber,
        MCMathEvalDivArrayByArray,
        EE_DIV_BADLEFT,
        EE_DIV_BADRIGHT,
        EE_DIV_MISMATCH,
        false,
        FR_MULDIV,
        kMCMathEvalDivMethodInfo,
        kMCMathEvalDivArrayByNumberMethodInfo,
        kMCMathEvalDivArrayByArrayMethodInfo>
{};

class MCEqual : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsEqualTo, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_EQUAL, kMCLogicEvalIsEqualToMethodInfo>
{};

class MCGreaterThan : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsGreaterThan, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_COMPARISON, kMCLogicEvalIsGreaterThanMethodInfo>
{};

class MCGreaterThanEqual : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsGreaterThanOrEqualTo, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_COMPARISON, kMCLogicEvalIsGreaterThanOrEqualToMethodInfo>
{};

class MCGrouping : public MCExpression
{
public:
	MCGrouping()
	{
		rank = FR_GROUPING;
    }
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
	virtual void compile(MCSyntaxFactoryRef ctxt);
};

class MCIs : public MCExpression
{
	Is_type form;
	Is_validation valid;
	Chunk_term delimiter;
public:
	MCIs()
	{
		rank = FR_EQUAL;
		form = IT_NORMAL;
		valid = IV_UNDEFINED;
		delimiter = CT_UNDEFINED;
	}
    Parse_stat parse(MCScriptPoint &, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
	virtual void compile(MCSyntaxFactoryRef ctxt);
};

class MCItem : public MCBinaryOperatorCtxt<MCStringRef, MCStringRef, MCStringsEvalConcatenateWithComma, EE_CONCAT_BADLEFT, EE_CONCAT_BADRIGHT, FR_CONCAT, kMCStringsEvalConcatenateWithCommaMethodInfo>
{};

class MCLessThan : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsLessThan, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_COMPARISON, kMCLogicEvalIsLessThanMethodInfo>
{};

class MCLessThanEqual : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsLessThanOrEqualTo, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_COMPARISON, kMCLogicEvalIsLessThanOrEqualToMethodInfo>
{};

class MCMinus : public MCMultiBinaryOperator
{
public:
    MCMinus()
    {
        rank = FR_ADDSUB;
    }

    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);

    virtual void getmethodinfo(MCExecMethodInfo **&r_methods, uindex_t &r_count) const;
    virtual bool canbeunary(void) const {return true;}
};

class MCMod : public MCMultiBinaryOperatorCtxt<
        MCMathEvalMod,
        MCMathEvalModArrayByNumber,
        MCMathEvalModArrayByArray,
        EE_MOD_BADLEFT,
        EE_MOD_BADRIGHT,
        EE_MOD_MISMATCH,
        false,
        FR_MULDIV,
        kMCMathEvalModMethodInfo,
        kMCMathEvalModArrayByNumberMethodInfo,
        kMCMathEvalModArrayByArrayMethodInfo>
{};

class MCWrap : public MCMultiBinaryOperatorCtxt<
        MCMathEvalWrap,
        MCMathEvalWrapArrayByNumber,
        MCMathEvalWrapArrayByArray,
        EE_WRAP_BADLEFT,
        EE_WRAP_BADRIGHT,
        EE_WRAP_MISMATCH,
        false,
        FR_MULDIV,
        kMCMathEvalWrapMethodInfo,
        kMCMathEvalWrapArrayByNumberMethodInfo,
        kMCMathEvalWrapArrayByArrayMethodInfo>
{};

class MCNot : public MCUnaryOperator
{
public:
    MCNot()
    {
        rank = FR_UNARY;
    }

    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);

    virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLogicEvalNotMethodInfo; }
};

class MCNotBits : public MCUnaryOperatorCtxt<uinteger_t, MCMathEvalBitwiseNot, EE_NOTBITS_BADRIGHT, FR_UNARY, kMCMathEvalBitwiseNotMethodInfo>
{};

class MCNotEqual : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsNotEqualTo, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_EQUAL, kMCLogicEvalIsNotEqualToMethodInfo>
{};

class MCOr : public MCExpression
{
public:
	MCOr()
	{
		rank = FR_OR;
    }
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
};

class MCOrBits : public MCBinaryOperatorCtxt<uinteger_t, uinteger_t, MCMathEvalBitwiseOr, EE_ORBITS_BADLEFT, EE_ORBITS_BADRIGHT, FR_OR_BITS, kMCMathEvalBitwiseOrMethodInfo>
{};

class MCOver : public MCMultiBinaryOperatorCtxt<
        MCMathEvalOver,
        MCMathEvalOverArrayByNumber,
        MCMathEvalOverArrayByArray,
        EE_OVER_BADLEFT,
        EE_OVER_BADRIGHT,
        EE_OVER_MISMATCH,
        false,
        FR_MULDIV,
        kMCMathEvalOverMethodInfo,
        kMCMathEvalOverArrayByNumberMethodInfo,
        kMCMathEvalOverArrayByArrayMethodInfo>
{};

class MCPlus : public MCMultiBinaryCommutativeOperatorCtxt<
        MCMathEvalAdd,
        MCMathEvalAddNumberToArray,
        MCMathEvalAddArrayToArray,
        EE_PLUS_BADLEFT,
        EE_PLUS_BADRIGHT,
        true,
        FR_ADDSUB,
        kMCMathEvalAddMethodInfo,
        kMCMathEvalAddNumberToArrayMethodInfo,
        kMCMathEvalAddArrayToArrayMethodInfo>
{};

class MCPow : public MCBinaryOperatorCtxt<double, double, MCMathEvalPower, EE_POW_BADLEFT, EE_POW_BADRIGHT, FR_POW, kMCMathEvalPowerMethodInfo>
{};

class MCThere : public MCExpression
{
	Is_type form;
	MCChunk *object;
	There_mode mode;
public:
	MCThere()
	{
		rank = FR_UNARY;
		form = IT_NORMAL;
		object = NULL;
	}
	virtual ~MCThere();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);
	
	virtual void compile(MCSyntaxFactoryRef factory);
};

class MCTimes : public MCMultiBinaryCommutativeOperatorCtxt<
        MCMathEvalMultiply,
        MCMathEvalMultiplyArrayByNumber,
        MCMathEvalMultiplyArrayByArray,
        EE_TIMES_BADLEFT,
        EE_TIMES_BADRIGHT,
        false,
        FR_MULDIV,
        kMCMathEvalAddMethodInfo,
        kMCMathEvalAddNumberToArrayMethodInfo,
        kMCMathEvalAddArrayToArrayMethodInfo>
{
};

class MCXorBits : public MCBinaryOperatorCtxt<uinteger_t, uinteger_t, MCMathEvalBitwiseXor, EE_XORBITS_BADLEFT, EE_XORBITS_BADRIGHT, FR_XOR_BITS, kMCMathEvalBitwiseXorMethodInfo>
{};

class MCBeginsEndsWith : public MCBinaryOperator
{
public:
    MCBeginsEndsWith(void)
    {
        rank = FR_COMPARISON;
    }
    virtual Parse_stat parse(MCScriptPoint&, Boolean the);
};

class MCBeginsWith : public MCBeginsEndsWith
{
public:
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);

    virtual MCExecMethodInfo *getmethodinfo() const {return kMCStringsEvalBeginsWithMethodInfo; }
};

class MCEndsWith : public MCBeginsEndsWith
{
public:
    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value);

    virtual MCExecMethodInfo *getmethodinfo() const {return kMCStringsEvalEndsWithMethodInfo; }
};

#endif
