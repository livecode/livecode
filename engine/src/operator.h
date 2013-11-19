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

        MCExecValueTraits<ParamType>::free(t_right);

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

        if (!MCExecValueTraits<ParamType>::eval(ctxt, left, EvalLeftError, t_left)
                || !MCExecValueTraits<ParamType>::eval(ctxt, right, EvalRightError, t_right))
            return;

        EvalMethod(ctxt, t_left, t_right, t_result);

        MCExecValueTraits<ParamType>::free(t_left);
        MCExecValueTraits<ParamType>::free(t_right);

        if (!ctxt . HasError())
            MCExecValueTraits<ReturnType>::set(r_value, t_result);
    }

    virtual MCExecMethodInfo *getmethodinfo() const { return MethodInfo; }
};

template<void (*Eval)(MCExecContext&, real64_t, real64_t, real64_t&),
         void (*EvalArrayByArray)(MCExecContext&, MCArrayRef, MCArrayRef, MCArrayRef&),
         void (*EvalArrayByNumber)(MCExecContext&, MCArrayRef, real64_t, MCArrayRef&),
         Exec_errors EvalLeftError,
         Exec_errors EvalRightError,
         bool CanBeUnary,
         Factor_rank Rank,
         MCExecMethodInfo *&EvalNumberMethodInfo,
         MCExecMethodInfo *&EvalArrayByArrayMethodInfo,
         MCExecMethodInfo *&EvalArrayByNumberMethodInfo>
class MCMultiBinaryOperatorCtxt: public MCMultiBinaryOperator
{
public:
    MCMultiBinaryOperatorCtxt() { rank = Rank; }

    virtual void eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
    {
        MCExecValue t_left, t_right;

        left -> eval_ctxt(ctxt, t_left);
        if (ctxt . HasError())
        {
            ctxt . LegacyThrow(EvalLeftError);
            return;
        }

        right -> eval_ctxt(ctxt, t_right);
        if (ctxt . HasError())
        {
            ctxt . LegacyThrow(EvalRightError);
            MCValueRelease(t_left . valueref_value);
            return;
        }

        if (MCValueGetTypeCode(t_left. valueref_value) == kMCExecValueTypeArrayRef)
        {
            MCAutoArrayRef t_result;

            if (MCValueGetTypeCode(t_right . valueref_value) == kMCExecValueTypeArrayRef)
                EvalArrayByArray(ctxt, t_left . arrayref_value, t_right . arrayref_value, &t_result);
            else
            {
                real64_t t_real;
                if (ctxt . ConvertToReal(t_right . valueref_value, t_real))
                    EvalArrayByNumber(ctxt, t_left . arrayref_value, t_real, &t_result);
            }

            if (!ctxt . HasError())
                MCExecValueTraits<MCArrayRef>::set(r_value, *t_result);
        }
        else
        {
            if (MCValueGetTypeCode(t_right . valueref_value) == kMCExecValueTypeArrayRef)
            {
                ctxt . LegacyThrow(EE_DIV_MISMATCH);
            }
            else
            {
                real64_t t_real_result = 0.0;
                real64_t t_left_real, t_right_real;

                if (ctxt . ConvertToReal(t_left . valueref_value, t_left_real)
                        && ctxt . ConvertToReal(t_right . valueref_value, t_right_real))
                    Eval(ctxt, t_left_real, t_right_real, t_real_result);

                if (!ctxt . HasError())
                    MCExecValueTraits<double>::set(r_value, (double)t_real_result);
            }
        }
        MCValueRelease(t_left . valueref_value);
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
	virtual Exec_stat eval(MCExecPoint &);
};

class MCAndBits : public MCBinaryOperatorCtxt<uinteger_t, uinteger_t, MCMathEvalBitwiseAnd, EE_ANDBITS_BADLEFT, EE_ANDBITS_BADRIGHT, FR_AND_BITS, kMCMathEvalBitwiseAndMethodInfo>
{
};

class MCConcat : public MCBinaryOperatorCtxt<MCStringRef, MCStringRef, MCStringsEvalConcatenate, EE_CONCAT_BADLEFT, EE_CONCAT_BADRIGHT, FR_CONCAT, kMCStringsEvalConcatenateMethodInfo>
{
};

class MCConcatSpace : public MCBinaryOperatorCtxt<MCStringRef, MCStringRef, MCStringsEvalConcatenateWithSpace, EE_CONCATSPACE_BADLEFT, EE_CONCATSPACE_BADRIGHT, FR_CONCAT, kMCStringsEvalConcatenateWithSpaceMethodInfo>
{
};

class MCContains : public MCBinaryOperatorCtxt<MCStringRef, bool, MCStringsEvalContains, EE_CONTAINS_BADLEFT, EE_CONTAINS_BADRIGHT, FR_COMPARISON, kMCStringsEvalContainsMethodInfo>
{
};

class MCDiv : public MCMultiBinaryOperator
{
public:
	MCDiv()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const;
};

class MCEqual : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsEqualTo, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_EQUAL, kMCLogicEvalIsEqualToMethodInfo>
{
};

class MCGreaterThan : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsGreaterThan, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_COMPARISON, kMCLogicEvalIsGreaterThanMethodInfo>
{
};

class MCGreaterThanEqual : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsGreaterThanOrEqualTo, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_COMPARISON, kMCLogicEvalIsGreaterThanOrEqualToMethodInfo>
{
};

class MCGrouping : public MCExpression
{
public:
	MCGrouping()
	{
		rank = FR_GROUPING;
	}
	virtual Exec_stat eval(MCExecPoint &);
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
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef ctxt);
};

class MCItem : public MCBinaryOperatorCtxt<MCStringRef, MCStringRef, MCStringsEvalConcatenateWithComma, EE_CONCAT_BADLEFT, EE_CONCAT_BADRIGHT, FR_CONCAT, kMCStringsEvalConcatenateWithCommaMethodInfo>
{
};

class MCLessThan : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsLessThan, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_COMPARISON, kMCLogicEvalIsLessThanMethodInfo>
{
};

class MCLessThanEqual : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsLessThanOrEqualTo, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_COMPARISON, kMCLogicEvalIsLessThanOrEqualToMethodInfo>
{
};

class MCMinus : public MCMultiBinaryOperator
{
public:
	MCMinus()
	{
		rank = FR_ADDSUB;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const;
	virtual bool canbeunary(void) const {return true;}
};

class MCMod : public MCMultiBinaryOperator
{
public:
	MCMod()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const;
};

class MCWrap : public MCMultiBinaryOperator
{
public:
	MCWrap()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const;
};

class MCNot : public MCUnaryOperatorCtxt<bool, MCLogicEvalNot, EE_NOT_BADRIGHT, FR_UNARY, kMCLogicEvalNotMethodInfo>
{
};

class MCNotBits : public MCUnaryOperatorCtxt<uinteger_t, MCMathEvalBitwiseNot, EE_NOTBITS_BADRIGHT, FR_UNARY, kMCMathEvalBitwiseNotMethodInfo>
{
};

class MCNotEqual : public MCBinaryOperatorCtxt<MCValueRef, bool, MCLogicEvalIsNotEqualTo, EE_FACTOR_BADLEFT, EE_FACTOR_BADRIGHT, FR_EQUAL, kMCLogicEvalIsNotEqualToMethodInfo>
{
};

class MCOr : public MCExpression
{
public:
	MCOr()
	{
		rank = FR_OR;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCOrBits : public MCBinaryOperatorCtxt<uinteger_t, uinteger_t, MCMathEvalBitwiseOr, EE_ORBITS_BADLEFT, EE_ORBITS_BADRIGHT, FR_OR_BITS, kMCMathEvalBitwiseOrMethodInfo>
{
};

class MCOver : public MCMultiBinaryOperator
{
public:
	MCOver()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const;
};

class MCPlus : public MCMultiBinaryOperator
{
public:
	MCPlus()
	{
		rank = FR_ADDSUB;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const;
	virtual bool canbeunary(void) const {return true;}
};

class MCPow : public MCBinaryOperatorCtxt<double, double, MCMathEvalPower, EE_POW_BADLEFT, EE_POW_BADRIGHT, FR_POW, kMCMathEvalPowerMethodInfo>
{
};

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
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual void compile(MCSyntaxFactoryRef factory);
};

class MCTimes : public MCMultiBinaryOperatorCtxt<
        MCMathEvalMultiply,
        MCMathEvalMultiplyArrayByArray,
        MCMathEvalMultiplyArrayByNumber,
        EE_TIMES_BADLEFT,
        EE_TIMES_BADRIGHT,
        false,
        FR_MULDIV,
        kMCMathEvalAddMethodInfo,
        kMCMathEvalAddNumberToArrayMethodInfo,
        kMCMathEvalAddArrayToArrayMethodInfo>
{
public:
    MCTimes(){}
};

class MCXorBits : public MCBinaryOperatorCtxt<uinteger_t, uinteger_t, MCMathEvalBitwiseXor, EE_XORBITS_BADLEFT, EE_XORBITS_BADRIGHT, FR_XOR_BITS, kMCMathEvalBitwiseXorMethodInfo>
{
};

class MCBeginsEndsWith : public MCBinaryOperator
{
public:
    MCBeginsEndsWith(void){}
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
