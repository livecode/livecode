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
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "mode.h"


#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"
#include "libscript/script.h"

#include <msctf.h>

////////////////////////////////////////////////////////////////////////////////

HINSTANCE MChInst;
MCStringRef MCcmdline;

// MW-2008-09-15: [[ Bug 7148 ]] Increase the maximum limit on the number of arguments to
//   256 - should hopefully be enough for most people's applications!
#define MAXARGS 256

////////////////////////////////////////////////////////////////////////////////

void X_main_loop(void)
{
	while (!MCquit)
	{
		X_main_loop_iteration();
		if (MCpendingstacklimit != MCstacklimit)
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

// To allow runtime configuration of the stack, we use fibers to run the main
// part of the engine execution.

// This is the main thread's principal fiber (running WinMain).
static void *s_main_fiber = nil;

// This is the errno pointer (from w32spec.cpp)
extern int *g_mainthread_errno;

struct InitializeFiberContext
{
	int argc;
	MCStringRef *argv;
	MCStringRef *envp;
	bool success;
};

static void DisplayStartupErrorAndExit(void)
{
	MCAutoStringRef mcap;
	MCAutoStringRef mtext;
	MCModeGetStartupErrorMessage(&mcap, &mtext);
	if (!MCnoui)
	{
		MCAutoStringRefAsWString t_cap_wstr, t_text_wstr;
		/* UNCHECKED */ t_cap_wstr.Lock(*mcap);
		/* UNCHECKED */ t_text_wstr.Lock(*mtext);
		MessageBoxW(HWND_DESKTOP, *t_text_wstr, *t_cap_wstr, MB_APPLMODAL | MB_OK);
	}
	else
	{
		MCAutoStringRefAsSysString t_cap_sys, t_text_sys;
		/* UNCHECKED */ t_cap_sys.Lock(*mcap);
		/* UNCHECKED */ t_text_sys.Lock(*mtext);
		fprintf(stderr, "ERROR: %s: %s\n", *t_cap_sys, *t_text_sys);
	}
	exit(-1);
}

// The 'init' fiber is used to execute X_init.
static void CALLBACK InitializeFiberRoutine(void *p_context)
{
	InitializeFiberContext *context;
	context = (InitializeFiberContext *)p_context;

	g_mainthread_errno = _errno();

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_DELAY_FREE_MEM_DF|_CRTDBG_CHECK_CRT_DF);
#endif

    struct X_init_options t_options;
    t_options.argc = context -> argc;
    t_options.argv = context -> argv;
    t_options.envp = context -> envp;
    t_options.app_code_path = nullptr;
	context -> success = X_init(t_options);

	SwitchToFiber(s_main_fiber);
}

// The 'loop' fiber is used to run X_main_loop.
static void CALLBACK LoopFiberRoutine(void *p_parameter)
{
	void *t_bottom;
	MCstackbottom = (char *)&t_bottom;

	g_mainthread_errno = _errno();
	
	X_main_loop();

	SwitchToFiber(s_main_fiber);
}

////////////////////////////////////////////////////////////////////////////////

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// MW-2010-05-09: Elevated process handling - if the cmd line begins with '-elevated-slave'
	//   then we are running in the elevation bootstrap mode.
	extern int MCS_windows_elevation_bootstrap_main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
	if (strncmp(lpCmdLine, "-elevated-slave", 15) == 0 && strlen(lpCmdLine) == 23)
		return MCS_windows_elevation_bootstrap_main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	MCModePreMain();

	int argc = 0;
	int envc = 0;
	MCAutoArray<MCStringRef> argv;
	MCAutoArray<MCStringRef> envp;

	// MW-2004-11-25: Under Win9x it would appear that not all exceptions are masked...
	// MW-2004-11-28: Actually, this doesn't solve the problem, it seems the OS is
	//   meddling with it somewhere so have installed a dummy signal handler
	//  _controlfp( _controlfp(0, 0) | (EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT | EM_ZERODIVIDE | EM_DENORMAL | EM_INVALID), MCW_EM);

#ifdef _DEBUG
	int t_dbg_flags;
	t_dbg_flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	t_dbg_flags |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(t_dbg_flags);
#endif

	// MW-2010-10-12: Turn off critical error reporting dialog - this stops annoying
	//   'cant find disk' messages (we hope!). This is advised at:
	//       http://msdn.microsoft.com/en-us/library/ms680621(VS.85).aspx
	SetErrorMode(SEM_FAILCRITICALERRORS);

	MCModeSetupCrashReporting();

	// SN-2014-08-06: [[ Bug 13027 ]] This line had been removed, thus MChInst was always NULL
	MChInst = hInstance;

	// Get the command line and convert it into the expected argc/argv form
	LPWSTR lpWCmdLine = wcsdup(GetCommandLineW());

    // FG-2014-09-23: [[ Bugfix 12444 ]] Re-arrange command line processing to
    // match behaviour in 6.x and below.
    WCHAR* wcFileNameBuf = new (nothrow) WCHAR[MAX_PATH+1];
    DWORD dwFileNameLen = GetModuleFileNameW(NULL, wcFileNameBuf, MAX_PATH+1);
    
	// Windows uses slashes the opposite way around to the other platforms and requires conversion
    for (DWORD i = 0; i < dwFileNameLen; i++)
    {
        if (wcFileNameBuf[i] == '/')
            wcFileNameBuf[i] = '\\';
        else if (wcFileNameBuf[i] == '\\')
            wcFileNameBuf[i] = '/';
    }
	LPWSTR csptr = lpWCmdLine;
	while (*csptr)
	{
		if (*csptr == '\\')
			*csptr = '/';
		csptr++;
	}

	// SN-2014-11-19: [[ Bug 14058 ]] GetCommandLineW returns the executable name and the arguments, 
	// but we do not want to include the former in MCcmdline
	csptr = lpWCmdLine;
	if (*csptr == '"')
	{
		// The executable is the whole enquoted string
		++csptr;
		while(*csptr && *csptr != '"')
			csptr++;
	}
	else
	{
		// The executable goes up to the first space
		while(*csptr && !iswspace(*csptr))
			csptr++;
	}
	if (*csptr)
	{
		// We discard all the spaces after the quote/space we found
		csptr++;
		while (*csptr && iswspace(*csptr))
			++csptr;
	}

    if (!MCInitialize() ||
        !MCSInitialize() ||
        !MCScriptInitialize())
		exit(-1);
	
    // Ensure the command line variable gets set
	// SN-2014-11-19: [[ Bug 14058 ]] We use the updated command line (a nil pointer is fine)
    /* UNCHECKED */ MCStringCreateWithWString(csptr, MCcmdline);
    
	// Convert the WStrings (UTF-16) into StringRefs
    LPWSTR *lpWargv = CommandLineToArgvW(lpWCmdLine, &argc);
	argv.New(argc);
    /* UNCHECKED */ MCStringCreateWithWString(wcFileNameBuf, argv[0]);
	for (int i = 1; i < argc; i++)
		/* UNCHECKED */ MCStringCreateWithWString(lpWargv[i], argv[i]);
	
	LocalFree(lpWargv);
    delete wcFileNameBuf;
    delete lpWCmdLine;
	
	// Convert the environment strings into StringRefs
	envp.New(1);
	LPWCH lpEnvStrings = GetEnvironmentStringsW();
	LPCWSTR sptr = lpEnvStrings;
	size_t t_len;
	while ((t_len = wcslen(sptr)) > 0)
	{
		uindex_t t_index = envc++;
		/* UNCHECKED */ envp.Extend(envc + 1);
		/* UNCHECKED */ MCStringCreateWithWString(sptr, envp[envc - 1]);
		sptr += t_len + 1;
	}
	
	// Terminate the envp array
	envp[envc] = nil;
	
	FreeEnvironmentStringsW(lpEnvStrings);

	// Initialize OLE on the main thread.
	OleInitialize(nil);

	// Initialize the TSF manager on the main thread
	ITfThreadMgr *t_tsf_mgr;
	t_tsf_mgr = nil;
	CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void**)&t_tsf_mgr);

	// Create the main thread's principal fiber.
	s_main_fiber = ConvertThreadToFiber(nil);

	// Now create the init fiber
	InitializeFiberContext t_init;
	t_init . argc = argc;
	t_init . argv = argv.Ptr();
	t_init . envp = envp.Ptr();
	t_init . success = False;

	void *t_init_fiber;
	t_init_fiber = CreateFiberEx(64 * 1024, MCstacklimit, 0, InitializeFiberRoutine, &t_init);
	if (t_init_fiber == nil)
		exit(-1);

	SwitchToFiber(t_init_fiber);
	DeleteFiber(t_init_fiber);

	if (!t_init . success)
		DisplayStartupErrorAndExit();

	// Now we loop continually until quit. If 'X_main_loop' returns without quitting
	// it means a stack size change request has occurred.
	while(!MCquit)
	{
		// Create ourselves a new fiber with appropriate stack size
		void *t_loop_fiber;

		if (MCpendingstacklimit < 65536)
			MCpendingstacklimit = 65536;

		for(;;)
		{
			t_loop_fiber = CreateFiberEx(64 * 1024, MCpendingstacklimit, 0, LoopFiberRoutine, nil);
			if (t_loop_fiber != nil)
			{
				MCstacklimit = MCpendingstacklimit;
				break;
			}

			if (MCpendingstacklimit < 1024 * 1024)
				break;

			MCpendingstacklimit -= 1024 * 1024;
		}

		if (t_loop_fiber == nil)
			exit(-1);

		MCrecursionlimit = MCU_min(MCrecursionlimit, MCstacklimit - MC_UNCHECKED_STACKSIZE);

		SwitchToFiber(t_loop_fiber);
		DeleteFiber(t_loop_fiber);
	}

	// MW-2011-08-19: [[ Bug ]] Make sure we reset the 'bottom' of the stack and
	//   errno pointer (we're executing script on a different fiber than before).
	void *t_bottom;
	MCstackbottom = (char *)&t_bottom;
	
	g_mainthread_errno = _errno();
	int r = X_close();
    
    MCValueRelease(MCcmdline);
    
    MCScriptFinalize();
	MCFinalize();

	if (t_tsf_mgr != nil)
		t_tsf_mgr -> Release();

	return r;
}

////////////////////////////////////////////////////////////////////////////////
