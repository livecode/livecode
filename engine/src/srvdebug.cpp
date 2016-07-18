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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"


#include "globals.h"
#include "debug.h"
#include "mcerror.h"
#include "osspec.h"

#include "srvscript.h"
#include "srvdebug.h"
#include "srvmain.h"

////////

extern bool IreviamDebuggerConnect(const char *p_site, const char *p_url);
extern void IreviamDebuggerDisconnect(void);
extern bool IreviamDebuggerUpdate(uint32_t p_until);
extern bool IreviamDebuggerRun(const char *p_reason, const char *p_file, uint32_t line, uint32_t pos);

extern void MCVariableViewEnter(MCExecContext &ctxt);
extern void MCVariableViewLeave(void);

////////

enum MCDebugActionTag
{
	kMCDebugActionNone,
	kMCDebugActionPause,
	kMCDebugActionContinue,
	kMCDebugActionStop,
	kMCDebugActionStepInto,
	kMCDebugActionStepOver,
	kMCDebugActionStepOut,
	kMCDebugActionBreak
};

static bool s_debugging = false;

#ifdef _IREVIAM
static uint32_t s_interrupts = 0;
#endif

static MCDebugActionTag s_action = kMCDebugActionNone;
static uint32_t s_current_context = 0;

extern MCSErrorMode g_server_error_mode;
extern MCServerScript *MCserverscript;
extern MCStringRef MCservercgidocumentroot;

////////////////////////////////////////////////////////////////////////
//
//  BREAKPOINT TABLE
//

struct MCServerBreakpoint
{
	uint4 file;
	uint4 row;
};

static MCServerBreakpoint *s_breakpoints = NULL;
static uint4 s_breakpoint_count = 0;

// Search for the given breakpoint, returning either its entry, or
// the one that would follow it were it to exist.
static uint4 BreakpointsSearch(uint4 p_file, uint4 p_row)
{
	uint4 t_low, t_high;
	t_low = 0;
	t_high = s_breakpoint_count;
	while(t_low < t_high)
	{
		// Find the mid point (always rounding down)
		uint4 t_mid;
		t_mid = (t_low + t_high) / 2;
		
		// Compare t_mid with breakpoint
		int d;
		d = s_breakpoints[t_mid] . file - p_file;
		if (d == 0)
			d = s_breakpoints[t_mid] . row - p_row;
		
		if (d > 0)
			t_high = t_mid;
		else if (d < 0)
			t_low = t_mid + 1;
		else
			return t_mid;
	}
	
	// t_low == t_high == entry after an exact match.
	return t_high;
}

////////////////////////////////////////////////////////////////////////
//
//  UTILITIES
//

static bool GetFileForContext(MCExecContext &ctxt, MCStringRef &r_file)
{
    MCAutoStringRef t_file;
    if (MCserverscript -> GetFileForContext(ctxt, &t_file))
    {
        if (!MCStringBeginsWith(*t_file, MCservercgidocumentroot, kMCStringOptionCompareExact))
            return MCStringCopy(*t_file, r_file);
	
        return MCStringCopySubstring(*t_file, MCRangeMake(MCStringGetLength(MCservercgidocumentroot), UINDEX_MAX), r_file);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////
//
//  INTERFACE TO ENGINE
//

bool MCServerDebugConnect(MCStringRef p_site, MCStringRef p_url)
{
#ifdef _IREVIAM
	s_debugging = IreviamDebuggerConnect(MCStringGetCString(p_site), MCStringGetCString(p_url));
	if (s_debugging)
	{
		MCservererrormode = kMCSErrorModeDebugger;
		MCtrace = True;
	}
#endif
	
	return s_debugging;
}

void MCServerDebugDisconnect(void)
{
#ifdef _IREVIAM
	if (s_debugging)
		IreviamDebuggerDisconnect();
#endif
}

void MCServerDebugInterrupt(void)
{
#ifdef _IREVIAM
	if (!s_debugging)
		return;
	
	// If we get an 'interrupt' it means that the debugger needs to be serviced before
	// doing anything further. Therefore, we make sure MCtrace is true, and set a flag
	// to ensure we do so.
	s_interrupts += 1;
#endif
}

void MCServerDebugTrace(MCExecContext &ctxt, uint2 p_row, uint2 p_col)
{
#ifdef _IREVIAM
	if (!s_debugging)
		return;
	
	// If we were interrupted, make sure we service the debugger.
	s_debugging = IreviamDebuggerUpdate(s_interrupts);
	
	// Check that we are still debugging, if not terminate
	if (!s_debugging)
	{
		s_action = kMCDebugActionNone;
		MCexitall = True;
		MCtrace = false;
		return;
	}
		
	// Check that we are in something we can debug
	uint4 t_file_index;
	t_file_index = MCserverscript -> GetFileIndexForContext(ep);
	if (t_file_index == 0)
		return;
	
	// If there is no action, or we are stepping over or out, check breakpoint
	// list. (The other cases all run the debugger regardless).
	if (s_action == kMCDebugActionNone || s_action == kMCDebugActionStepOver || s_action == kMCDebugActionStepOut)
	{
		// Look for a matching entry
		uint4 t_index;
		t_index = BreakpointsSearch(t_file_index, p_row);
		
		// If the returned index *is* what we are looking for, change the action to 'break'
		if (t_index < s_breakpoint_count &&
			s_breakpoints[t_index] . file == t_file_index && s_breakpoints[t_index] . row == p_row)
			s_action = kMCDebugActionBreak;
	}
	
	// Do the appropriate thing based on the current action.
	switch(s_action)
	{
		case kMCDebugActionNone:
			// Nothing to check for, so just carry on with execution
			break;
			
		case kMCDebugActionPause:
			// Make sure the debug action is none.
			s_action = kMCDebugActionNone;
			
			// We are pausing, so run the debugger
			MCS_flush(IO_stdout);
			
			MCVariableViewEnter(ctxt);
			s_debugging = IreviamDebuggerRun("pause", GetFileForContext(ctxt), p_row, p_col);
			MCVariableViewLeave();
			break;
			
		case kMCDebugActionStepInto:
			// Stepping into is easy - we just pause!
			s_action = kMCDebugActionNone;
			MCS_flush(IO_stdout);
			
			MCVariableViewEnter(ctxt);
			s_debugging = IreviamDebuggerRun("step", GetFileForContext(ctxt), p_row, p_col);
			MCVariableViewLeave();
			break;
			
		case kMCDebugActionStepOver:
			// For stepping over, we run until we reach the same context again.
			if (MCnexecutioncontexts == s_current_context)
			{
				s_action = kMCDebugActionNone;
				MCS_flush(IO_stdout);

				MCVariableViewEnter(ctxt);
				s_debugging = IreviamDebuggerRun("step", GetFileForContext(ctxt), p_row, p_col);
				MCVariableViewLeave();
			}
			break;
			
		case kMCDebugActionStepOut:
			// For stepping out, we run until we are an earlier context
			if (MCnexecutioncontexts < s_current_context)
			{
				s_action = kMCDebugActionNone;
				MCS_flush(IO_stdout);
				
				MCVariableViewEnter(ctxt);
				s_debugging = IreviamDebuggerRun("step", GetFileForContext(ctxt), p_row, p_col);
				MCVariableViewLeave();
			}
			break;
			
		case kMCDebugActionBreak:
			// We hit an implicit breakpoint
			s_action = kMCDebugActionNone;
			MCS_flush(IO_stdout);
			
			MCVariableViewEnter(&ep);
			s_debugging = IreviamDebuggerRun("break", GetFileForContext(ctxt), p_row, p_col);
			MCVariableViewLeave();
			break;
	}
	
	// Now handle 'stop' - we do this here, because any of the non-trivial
	// other actions could result in a stop request.
	// Note we also terminate if debugging stops.
	if (s_action == kMCDebugActionStop && s_debugging || !s_debugging)
	{
		s_action = kMCDebugActionNone;
		MCtrace = False;
		MCexitall = True;
	}
#endif
}

void MCServerDebugBreak(MCExecContext &ctxt, uint2 p_row, uint2 p_col)
{
#ifdef _IREVIAM
	if (!s_debugging)
		return;
	
	// Check that we are in something we can debug
	const char *t_file;
	t_file = GetFileForContext(ctxt);
	if (t_file == NULL)
		return;

	// Reset the debug action to none.
	s_action = kMCDebugActionNone;
	
	// Run the debugger with due to a 'break'
	MCS_flush(IO_stdout);
	
	MCVariableViewEnter(ctxt);
	s_debugging = IreviamDebuggerRun("break", t_file, p_row, p_col);
	MCVariableViewLeave();

	// Check for stop
	// Note we also terminate if debugging stops.
	if (s_action == kMCDebugActionStop && s_debugging || !s_debugging)
	{
		s_action = kMCDebugActionNone;
		MCtrace = False;
		MCexitall = True;
	}
#endif
}

void MCServerDebugError(MCExecContext& ctxt, uint2 p_row, uint2 p_col, uint2 p_id)
{
#ifdef _IREVIAM
	if (!s_debugging)
		return;
	
	// Check that we are in something we can debug
	const char *t_file;
	t_file = GetFileForContext(ctxt);
	if (t_file == NULL)
		return;
	
	// Run the debugger due to an error
	MCS_flush(IO_stdout);
	
	MCVariableViewEnter(ctxt);
	s_debugging = IreviamDebuggerRun("error", t_file, p_row, p_col);
	MCVariableViewLeave();
	
	// Exit all
	s_action = kMCDebugActionNone;
	MCtrace = False;
	MCexitall = True;
#endif
}

void MCServerDebugVariableChanged(MCExecContext& ctxt, MCNameRef p_name)
{
}

////////////////////////////////////////////////////////////////////////////////
//
//  INTERFACE TO DEBUGGER
//

typedef bool (*MCServerDebugListVariablesCallback)(void *p_context, uint32_t p_index, uint32_t p_depth, bool p_array, bool p_expanded, const char *p_name, const char *p_value);

void MCServerDebugAction(MCStringRef p_action)
{
	if (MCStringIsEqualToCString(p_action, "stop", kMCCompareExact))
		s_action = kMCDebugActionStop;
	else if (MCStringIsEqualToCString(p_action, "pause", kMCCompareExact))
		s_action = kMCDebugActionPause;
	else if (MCStringIsEqualToCString(p_action, "step over", kMCCompareExact))
	{
		s_action = kMCDebugActionStepOver;
		s_current_context = MCnexecutioncontexts;
	}
	else if (MCStringIsEqualToCString(p_action, "step into", kMCCompareExact))
	{
		s_action = kMCDebugActionStepInto;
		s_current_context = MCnexecutioncontexts;
	}
	else if (MCStringIsEqualToCString(p_action, "step out", kMCCompareExact))
	{
		s_action = kMCDebugActionStepOut;
		s_current_context = MCnexecutioncontexts;
	}
}

void MCServerDebugBreakpoint(MCStringRef p_action, MCStringRef p_file, uint32_t p_row, uint32_t p_column)
{
	bool t_add;
	t_add = MCStringIsEqualToCString(p_action, "add", kMCCompareExact);
	
	// Construct the absolute file path.
	MCAutoStringRef t_filename;
    
	if (MCStringGetNativeCharAtIndex(p_file, 0) != '/')
        /* UNCHECKED */ MCStringFormat(&t_filename, "%@/%@", MCservercgidocumentroot, p_file);
    else
        /* UNCHECKED */ t_filename = p_file;
		
	// Lookup the file's index - creating an entry if we are 'adding' a breakpoint.
	uint4 t_file;
	t_file = MCserverscript -> FindFileIndex(*t_filename, t_add);
	
	// If we didn't find an entry there can't be a breakpoint on it so exit.
	if (t_file == 0)
		return;
	
	// Find the entry matching the breakpoint, or the one that would follow it were it to exist.
	uint4 t_entry;
	t_entry = BreakpointsSearch(t_file, p_row);
	
	bool t_matches;
	t_matches = t_entry < s_breakpoint_count &&
				s_breakpoints[t_entry] . file == t_file &&
				s_breakpoints[t_entry] . row == p_row;
	
	if (!t_add)
	{
		// If we are removing and it doesn't exist, exit
		if (!t_matches)
			return;
		
		// Otherwise delete it
		memmove(s_breakpoints + t_entry, s_breakpoints + t_entry + 1, s_breakpoint_count - t_entry - 1);
		s_breakpoint_count -= 1;
	}
	else
	{
		// If we are adding and it exists, exit
		if (t_matches)
			return;
		
		// Otherwise insert it
		s_breakpoints = static_cast<MCServerBreakpoint *>(realloc(s_breakpoints, sizeof(MCServerBreakpoint) * (s_breakpoint_count + 1)));
		memmove(s_breakpoints + t_entry + 1, s_breakpoints + t_entry, s_breakpoint_count - t_entry);
		s_breakpoints[t_entry] . file = t_file;
		s_breakpoints[t_entry] . row = p_row;
		s_breakpoint_count += 1;
	}
	
}

void MCServerDebugGet(MCStringRef p_property, MCStringRef& r_result)
{
    MCStringRef t_error;
	
	if (MCStringIsEqualToCString(p_property, "execution error", kMCCompareExact))
        /* UNCHECKED */ MCeerror -> copyasstringref(t_error);
	else if (MCStringIsEqualToCString(p_property, "parse error", kMCCompareExact))
        MCperror -> copyasstringref(t_error);
    else if (MCStringIsEqualToCString(p_property, "files", kMCCompareExact))
        t_error = MCValueRetain(kMCEmptyString);
	else
		t_error = MCValueRetain(kMCEmptyString);

    r_result = t_error;
}

void MCServerDebugPut(MCStringRef p_property, MCStringRef p_value)
{
}
