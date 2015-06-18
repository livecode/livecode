/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 Runtime Revolution Ltd.

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

#include "em-util.h"

#include "sysdefs.h"
#include "osspec.h"

/* ================================================================
 * Locales
 * ================================================================ */

MCLocaleRef
MCS_getsystemlocale()
{
	/* Emscripten locale is basically C.UTF-8.  There's no standard
	 * way to get the user's desired locale from ECMAScript,
	 * either. */
	MCLocaleRef t_locale;
	/* UNCHECKED */ MCLocaleCreateWithName(MCSTR("C"), t_locale);
	return t_locale;
}
