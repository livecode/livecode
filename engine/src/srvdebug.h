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

#ifndef __MC_SERVER_DEBUG__
#define __MC_SERVER_DEBUG__

////////////////////////////////////////////////////////////////////////
//
// INTERNAL INTERFACE
//
// These methods are how the rest of the engine interacts with the
//  engine-side of the server-side debugger.
//

// Attempt to connect to the server-side debugger for the given site and url
extern bool MCServerDebugConnect(MCStringRef p_site, MCStringRef p_url);

// Disconnect from the server-side debugger (this does nothing if we were
// never connected).
extern void MCServerDebugDisconnect(void);

// Notify the debugger interface that a SIGINT was received (called from
// within the signal handler and used to 'wake up' the debugger to update
// itself).
extern void MCServerDebugInterrupt(void);

// Notify the server-side debugger of various script-events.
extern void MCServerDebugTrace(MCExecContext &ctxt, uint2 line, uint2 pos);
extern void MCServerDebugBreak(MCExecContext &ctxt, uint2 line, uint2 pos);
extern void MCServerDebugError(MCExecContext &ctxt, uint2 line, uint2 pos, uint2 id);

// Notify the server-side debugger that the given variable has changed.
extern void MCServerDebugVariableChanged(MCExecContext &ctxt, MCNameRef name);

////////////////////////////////////////////////////////////////////////
//
// EXTERNAL INTERFACE
//
// These methods are how the remote debugger interacts with the engine-
// side.
//

// Perform one of the following actions:
//   stop, pause, step over, step into, step out
extern void MCServerDebugAction(MCStringRef p_action);

// Manipulate the given breakpoint (either add or remove).
extern void MCServerDebugBreakpoint(MCStringRef p_action, MCStringRef p_file, uint32_t p_row, uint32_t p_column);

// Get one of a handful of current state variables
void MCServerDebugGet(MCStringRef p_property, MCStringRef& r_result);

// Set one of a handful of current state variables
void MCServerDebugPut(MCStringRef p_property, MCStringRef value);

// Configure the variable viewing window
void MCServerDebugConfigureView(uint32_t scroll, uint32_t height);
void MCServerDebugConfigureVariable(uint32_t index, const char *state);

#endif
