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
// MCLiteral class declarations
//
#ifndef	LITERAL_H
#define	LITERAL_H

#include "express.h"

class MCLiteral : public MCExpression
{
	MCNameRef value;
public:
	MCLiteral(MCNameRef v)
	{
		/* UNCHECKED */ MCNameClone(v, value);
	}
	~MCLiteral(void)
	{
		MCNameDelete(value);
	}

	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
	virtual void compile(MCSyntaxFactoryRef ctxt);
};

class MCLiteralNumber : public MCExpression
{
	MCNameRef value;
	double nvalue;
public:
	MCLiteralNumber(MCNameRef v, double n)
	{
		/* UNCHECKED */ MCNameClone(v, value);
		nvalue = n;
	}
	~MCLiteralNumber(void)
	{
		MCNameDelete(value);
	}

	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value);
};

#endif
