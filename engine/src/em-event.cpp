/*                                                                     -*-c++-*-

Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "objdefs.h"

/* ----------------------------------------------------------------
 * Functions implemented in em-event.js
 * ---------------------------------------------------------------- */

extern "C" bool MCEmscriptenEventInitializeJS(void);
extern "C" void MCEmscriptenEventFinalizeJS(void);

/* ----------------------------------------------------------------
 * Initialisation / finalisation
 * ---------------------------------------------------------------- */

bool
MCEmscriptenEventInitialize()
{
	return MCEmscriptenEventInitializeJS();
}

void
MCEmscriptenEventFinalize()
{
	MCEmscriptenEventFinalizeJS();
}

/* ---------------------------------------------------------------- */

extern "C"
uint32_t MCEmscriptenEventEncodeModifiers(bool p_shift,
                                          bool p_alt,
                                          bool p_ctrl)
{
	uint32_t t_result;

	if (p_shift) t_result |= MS_SHIFT;
	if (p_alt)   t_result |= MS_ALT;
	if (p_ctrl)  t_result |= MS_CONTROL;

	return t_result;
}
