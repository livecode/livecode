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

#include "constant.h"

void MCConstant::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
	if (nvalue == BAD_NUMERIC)
	{
		r_value . type = kMCExecValueTypeValueRef;
		r_value . valueref_value = MCValueRetain(svalue);
	}
	else
	{
		r_value . type = kMCExecValueTypeDouble;
		r_value . double_value = nvalue;
	}
}
