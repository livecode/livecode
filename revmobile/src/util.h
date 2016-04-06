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

#ifndef __UTIL__
#define __UTIL__

#ifndef __MC_EXTERNAL__
#include "external.h"
#endif

#ifndef nil
#define nil 0
#endif

bool Throw(const char *p_message);
void Catch(MCVariableRef result);
bool CheckError(MCError p_error);
bool VariableFormat(MCVariableRef var, const char *p_format, ...);
bool VariableAppendFormat(MCVariableRef var, const char *p_format, ...);

#endif
