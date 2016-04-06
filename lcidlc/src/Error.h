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

#ifndef __ERROR__
#define __ERROR__

#include "foundation.h"

#ifndef __VALUE__
#include "Value.h"
#endif

////////////////////////////////////////////////////////////////////////////////

enum
{
	kErrorNone,
	kErrorCouldNotOpenFile,
	kErrorCouldNotReadFile,
	kErrorCantAdvancePastEnd,
	kErrorCantRetreatPastMark,
	kErrorNoCurrentToken
};

bool Throw(uint32_t error);
bool ThrowWithHint(uint32_t error, ValueRef hint);

////////////////////////////////////////////////////////////////////////////////

#endif
