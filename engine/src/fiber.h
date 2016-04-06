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

#ifndef __MC_FIBER__
#define __MC_FIBER__

typedef struct MCFiber *MCFiberRef;
typedef void (*MCFiberCallback)(void *);

// Convert the current thread to a fiber.
bool MCFiberConvert(MCFiberRef& r_fiber);

// Create a new fiber with the given stack size.
bool MCFiberCreate(size_t stack_size, MCFiberRef& r_fiber);

// Destroy the given fiber. This implicitly jumps to the fiber and
// waits for termination.
void MCFiberDestroy(MCFiberRef fiber);

// Get the fiber that is currently active.
MCFiberRef MCFiberGetCurrent(void);

// Switch to the given fiber, pausing the current one.
void MCFiberMakeCurrent(MCFiberRef fiber);

// Invoke the given routine on the given fiber and wait for return.
void MCFiberCall(MCFiberRef fiber, void (*callback)(void *), void *context);

// Returns true if the current fiber is currently running.
bool MCFiberIsCurrentThread(MCFiberRef fiber);

#endif
