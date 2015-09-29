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

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "literal.h"
#include "scriptpt.h"
#include "execpt.h"

Parse_stat MCLiteral::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	return PS_NORMAL;
}

Exec_stat MCLiteral::eval(MCExecPoint &ep)
{
	ep.setnameref_unsafe(value);
	return ES_NORMAL;
}

Parse_stat MCLiteralNumber::parse(MCScriptPoint &sp, Boolean the)
{
	initpoint(sp);
	return PS_NORMAL;
}

Exec_stat MCLiteralNumber::eval(MCExecPoint &ep)
{
	// MW-2013-04-12: [[ Bug 10837 ]] Make sure we set 'both' when evaluating the
	//   literal. Not doing this causes problems for things like 'numberFormat'.
	if (nvalue == BAD_NUMERIC)
		ep.setnameref_unsafe(value);
	else
		ep.setboth(MCNameGetOldString(value), nvalue);
	return ES_NORMAL;
}
