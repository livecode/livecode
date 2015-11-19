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

#ifndef __MC_EMSCRIPTEN_DC_MAINLOOP_H__
#define __MC_EMSCRIPTEN_DC_MAINLOOP_H__

#include <foundation.h>

bool X_init(int argc, MCStringRef argv[], int envc, MCStringRef envp[]);
bool X_main_loop_iteration(void);
int X_close(void);

#endif /* !__MC_EMSCRIPTEN_DC_MAINLOOP_H__ */
