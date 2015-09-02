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

#include <foundation.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

static MCErrorCode s_last_error = kMCErrorNone;
static MCErrorHandler s_handler = nil;

////////////////////////////////////////////////////////////////////////////////

bool MCErrorThrow(MCErrorCode p_code)
{
	if (s_handler != nil)
		s_handler(p_code);
	s_last_error = p_code;
	return false;
}

MCErrorCode MCErrorCatch(void)
{
	MCErrorCode t_error;
	t_error = s_last_error;
	s_last_error = kMCErrorNone;
	return t_error;
}

bool MCErrorIsPending(void)
{
	return s_last_error != kMCErrorNone;
}

void MCErrorSetHandler(MCErrorHandler p_handler)
{
	s_handler = p_handler;
}

////////////////////////////////////////////////////////////////////////////////
