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
         void (*EvalMethod)(MCExecContext&, typename MCExecValueTraits<ParamType>::in_type, typename MCExecValueTraits<ParamType>::in_type, typename MCExecValueTraits<ParamType>::out_type),
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
        ParamType t_result;

        if (!MCExecValueTraits<ParamType>::eval(ctxt, left, EvalLeftError, t_left)
                || !MCExecValueTraits<ParamType>::eval(ctxt, right, EvalRightError, t_right))
            return;

        EvalMethod(ctxt, t_left, t_right, t_result);

        MCExecValueTraits<ParamType>::free(t_left);
        MCExecValueTraits<ParamType>::free(t_right);

        if (!ctxt . HasError())
            MCExecValueTraits<ParamType>::set(r_value, t_result);
    }

    virtual MCExecMethodInfo *getmethodinfo() const { return MethodInfo; }
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

class MCAndBits : public MCBinaryOperatorCtxt<uinteger_t, MCMathEvalBitwiseAnd, EE_ANDBITS_BADLEFT, EE_ANDBITS_BADRIGHT, FR_AND_BITS, kMCMathEvalBitwiseAndMethodInfo>
{
public:
    MCAndBits(){}
};

class MCConcat : public MCBinaryOperatorCtxt<MCStringRef, MCStringsEvalConcatenate, EE_CONCAT_BADLEFT, EE_CONCAT_BADRIGHT, FR_CONCAT, kMCStringsEvalConcatenateMethodInfo>
{
public:
    MCConcat(){}
};

class MCConcatSpace : public MCBinaryOperatorCtxt<MCStringRef, MCStringsEvalConcatenateWithSpace, EE_CONCATSPACE_BADLEFT, EE_CONCATSPACE_BADRIGHT, FR_CONCAT, kMCStringsEvalConcatenateWithSpaceMethodInfo>
{
public:
    MCConcatSpace(){}
};

class MCContains : public MCBinaryOperator
{
public:
	MCContains()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCStringsEvalContainsMethodInfo;}
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

class MCEqual : public MCBinaryOperator
{
public:
	MCEqual()
	{
		rank = FR_EQUAL;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCLogicEvalIsEqualToMethodInfo;}
};

class MCGreaterThan : public MCBinaryOperator
{
public:
	MCGreaterThan()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCLogicEvalIsGreaterThanMethodInfo;}
};

class MCGreaterThanEqual : public MCBinaryOperator
{
public:
	MCGreaterThanEqual()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCLogicEvalIsGreaterThanOrEqualToMethodInfo;}
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

class MCItem : public MCBinaryOperator
{
public:
	MCItem()
	{
		rank = FR_CONCAT;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCStringsEvalConcatenateWithCommaMethodInfo;}
};

class MCLessThan : public MCBinaryOperator
{
public:
	MCLessThan()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCLogicEvalIsLessThanMethodInfo;}
};

class MCLessThanEqual : public MCBinaryOperator
{
public:
	MCLessThanEqual()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCLogicEvalIsLessThanOrEqualToMethodInfo;}
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
public:
    MCNot(){}
};

class MCNotBits : public MCUnaryOperatorCtxt<uinteger_t, MCMathEvalBitwiseNot, EE_NOTBITS_BADRIGHT, FR_UNARY, kMCMathEvalBitwiseNotMethodInfo>
{
public:
    MCNotBits(){}
};

class MCNotEqual : public MCBinaryOperator
{
public:
	MCNotEqual()
	{
		rank = FR_EQUAL;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCLogicEvalIsNotEqualToMethodInfo;}
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

class MCOrBits : public MCBinaryOperator
{
public:
	MCOrBits()
	{
		rank = FR_OR_BITS;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCMathEvalBitwiseOrMethodInfo;}
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

class MCPow : public MCBinaryOperator
{
public:
	MCPow()
	{
		rank = FR_POW;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCMathEvalPowerMethodInfo;}
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

class MCTimes : public MCMultiBinaryOperator
{
public:
	MCTimes()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual void getmethodinfo(MCExecMethodInfo**& r_methods, uindex_t& r_count) const;
};

class MCXorBits : public MCBinaryOperator
{
public:
	MCXorBits()
	{
		rank = FR_XOR_BITS;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCMathEvalBitwiseXorMethodInfo;}
};

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
	virtual Exec_stat eval(MCExecPoint&);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCStringsEvalBeginsWithMethodInfo;}
};

class MCEndsWith : public MCBeginsEndsWith
{
public:
	virtual Exec_stat eval(MCExecPoint&);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCStringsEvalEndsWithMethodInfo;}
};

#endif
