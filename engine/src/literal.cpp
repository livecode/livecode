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

#include "literal.h"
#include "scriptpt.h"
#include "execpt.h"

#include "syntax.h"

Parse_stat MCLiteral::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	return PS_NORMAL;
}

void MCLiteral::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCExecValueTraits<MCNameRef>::set(r_value, value);
}

void MCLiteral::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalConstant(ctxt, value);
	MCSyntaxFactoryEvalResult(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

Parse_stat MCLiteralNumber::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	return PS_NORMAL;
}

void MCLiteralNumber::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
	// IM-2013-05-02: *TODO* the bugfix here cannot be applied to the syntax
	// refactor branch as MCExecPoint::setboth() does not exist there
#ifdef OLD_EXEC
	// MW-2013-04-12: [[ Bug 10837 ]] Make sure we set 'both' when evaluating the
	//   literal. Not doing this causes problems for things like 'numberFormat'.
	if (nvalue == BAD_NUMERIC)
		ep.setvalueref_nullable(value);
	else
		ep.setboth(MCNameGetOldString(value), nvalue);
#endif
    if (value != nil)
        MCExecValueTraits<MCNameRef>::set(r_value, value);
    else
        MCExecValueTraits<MCStringRef>::set(r_value, kMCEmptyString);
}
