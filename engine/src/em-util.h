/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#ifndef __MC_EMSCRIPTEN_UTIL_H__
#define __MC_EMSCRIPTEN_UTIL_H__

#include <foundation.h>

/* ----------------------------------------------------------------
 * Debugging macros
 * ---------------------------------------------------------------- */

inline void
__MCEmscriptenNotImplemented(const char *p_file,
                             uint32_t p_line,
                             const char *p_function)
{
#if defined(_DEBUG)
	__MCLog(p_file, p_line, "not implemented: %s", p_function);
#endif /* _DEBUG */
}

#define MCEmscriptenNotImplemented() __MCEmscriptenNotImplemented(__FILE__, __LINE__, __PRETTY_FUNCTION__)

#endif /* ! __MC_EMSCRIPTEN_UTIL_H__ */
