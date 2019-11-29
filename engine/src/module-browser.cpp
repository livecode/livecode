/* Copyright (C) 2015 LiveCode Ltd.
 
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
#include "uidc.h"

#include <libbrowser.h>

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF bool MCEngineRunloopWait();
extern "C" MC_DLLEXPORT_DEF void MCEngineRunloopBreakWait();
extern "C" MC_DLLEXPORT_DEF bool MCEngineAddRunloopAction(MCRunloopActionCallback p_callback, void *p_context, MCRunloopActionRef &r_action);
extern "C" MC_DLLEXPORT_DEF void MCEngineRemoveRunloopAction(MCRunloopActionRef p_action);

static uinteger_t s_runloop_initialized = 0;
static MCRunloopActionRef s_runloop_action = nil;

extern "C" bool MCEngineBrowserLibrarySetupRunloop()
{
	bool t_success = true;
	
	if (s_runloop_initialized == 0)
	{
		if (t_success)
		{
			MCBrowserLibrarySetWaitFunction(MCEngineRunloopWait);
			MCBrowserLibrarySetBreakWaitFunction(MCEngineRunloopBreakWait);
		}
		
		MCBrowserRunloopCallback t_callback;
		void *t_context;
		if (t_success)
			t_success = MCBrowserLibraryGetRunloopCallback(t_callback, t_context);
		
		MCRunloopActionRef t_action = nil;
		if (t_success)
			t_success = MCEngineAddRunloopAction(t_callback, t_context, t_action);
		
		if (t_success)
			s_runloop_action = t_action;
	}
	
	if (t_success)
		s_runloop_initialized++;
	
	return t_success;
}

extern "C" void MCEngineBrowserLibraryTeardownRunloop()
{
	if (s_runloop_initialized > 1)
	{
		s_runloop_initialized--;
		return;
	}

	MCEngineRemoveRunloopAction(s_runloop_action);
	s_runloop_action = nil;
	s_runloop_initialized = 0;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_extensions_libbrowser_Initialize()
{
	return MCBrowserLibraryInitialize();
}

extern "C" void com_livecode_extensions_libbrowser_Finalize(void)
{
	MCBrowserLibraryFinalize();
}

////////////////////////////////////////////////////////////////////////////////
