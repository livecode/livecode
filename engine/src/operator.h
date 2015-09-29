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

#include "express.h"

class MCChunk;

class MCAnd : public MCExpression
{
public:
	MCAnd()
	{
		rank = FR_AND;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCAndBits : public MCExpression
{
public:
	MCAndBits()
	{
		rank = FR_AND_BITS;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCConcat : public MCExpression
{
public:
	MCConcat()
	{
		rank = FR_CONCAT;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCConcatSpace : public MCExpression
{
public:
	MCConcatSpace()
	{
		rank = FR_CONCAT;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCContains : public MCExpression
{
public:
	MCContains()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCDiv : public MCExpression
{
public:
	MCDiv()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCEqual : public MCExpression
{
public:
	MCEqual()
	{
		rank = FR_EQUAL;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCGreaterThan : public MCExpression
{
public:
	MCGreaterThan()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCGreaterThanEqual : public MCExpression
{
public:
	MCGreaterThanEqual()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCGrouping : public MCExpression
{
public:
	MCGrouping()
	{
		rank = FR_GROUPING;
	}
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCItem : public MCExpression
{
public:
	MCItem()
	{
		rank = FR_CONCAT;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCLessThan : public MCExpression
{
public:
	MCLessThan()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCLessThanEqual : public MCExpression
{
public:
	MCLessThanEqual()
	{
		rank = FR_COMPARISON;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMinus : public MCExpression
{
public:
	MCMinus()
	{
		rank = FR_ADDSUB;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCMod : public MCExpression
{
public:
	MCMod()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCWrap : public MCExpression
{
public:
	MCWrap()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCNot : public MCExpression
{
public:
	MCNot()
	{
		rank = FR_UNARY;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCNotBits : public MCExpression
{
public:
	MCNotBits()
	{
		rank = FR_UNARY;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCNotEqual : public MCExpression
{
public:
	MCNotEqual()
	{
		rank = FR_EQUAL;
	}
	virtual Exec_stat eval(MCExecPoint &);
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

class MCOrBits : public MCExpression
{
public:
	MCOrBits()
	{
		rank = FR_OR_BITS;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCOver : public MCExpression
{
public:
	MCOver()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCPlus : public MCExpression
{
public:
	MCPlus()
	{
		rank = FR_ADDSUB;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCPow : public MCExpression
{
public:
	MCPow()
	{
		rank = FR_POW;
	}
	virtual Exec_stat eval(MCExecPoint &);
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
};

class MCTimes : public MCExpression
{
public:
	MCTimes()
	{
		rank = FR_MULDIV;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCXorBits : public MCExpression
{
public:
	MCXorBits()
	{
		rank = FR_XOR_BITS;
	}
	virtual Exec_stat eval(MCExecPoint &);
};

class MCBeginsEndsWith : public MCExpression
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
};

class MCEndsWith : public MCBeginsEndsWith
{
public:
	virtual Exec_stat eval(MCExecPoint&);
};

#endif
