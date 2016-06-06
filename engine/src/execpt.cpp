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
#include "objdefs.h"

#include "object.h"
//#include "execpt.h"
#include "mcerror.h"
#include "util.h"
#include "globals.h"
#include "handler.h"
#include "hndlrlst.h"
#include "osspec.h"
#include "uidc.h"

#include "osxprefix-legacy.h"

#ifdef MODE_SERVER
#include "srvscript.h"
#endif

//////////

MCValueRef MCExecPoint::getvalueref(void)
{
	return value;
}

bool MCExecPoint::setvalueref(MCValueRef p_value)
{
	MCValueRef t_new_value;
	if (!MCValueCopy(p_value, t_new_value))
		return false;
	MCValueRelease(value);
	value = t_new_value;
	return true;
}

bool MCExecPoint::setvalueref_nullable(MCValueRef p_value)
{
	if (p_value == nil)
    {
        setvalueref(kMCEmptyString);
		return true;
	}
	return setvalueref(p_value);
}

bool MCExecPoint::copyasvalueref(MCValueRef& r_value)
{
	return MCValueCopy(value, r_value);
}

