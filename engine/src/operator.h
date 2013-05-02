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

class MCAnd : public MCExpression
{
public:
	MCAnd()
	{
		rank = FR_AND;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCAndBits : public MCBinaryOperator
{
public:
	MCAndBits()
	{
		rank = FR_AND_BITS;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCMathEvalBitwiseAndMethodInfo;}
};

class MCConcat : public MCBinaryOperator
{
public:
	MCConcat()
	{
		rank = FR_CONCAT;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCStringsEvalConcatenateMethodInfo;}
};

class MCConcatSpace : public MCBinaryOperator
{
public:
	MCConcatSpace()
	{
		rank = FR_CONCAT;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCStringsEvalConcatenateWithSpaceMethodInfo;}

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

class MCNot : public MCUnaryOperator
{
public:
	MCNot()
	{
		rank = FR_UNARY;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCLogicEvalNotMethodInfo;}
};

class MCNotBits : public MCUnaryOperator
{
public:
	MCNotBits()
	{
		rank = FR_UNARY;
	}
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual MCExecMethodInfo *getmethodinfo(void) const {return kMCMathEvalBitwiseNotMethodInfo;}
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
