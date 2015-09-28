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

#include "w32prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "mode.h"

#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"

#include <msctf.h>

////////////////////////////////////////////////////////////////////////////////

bool X_init(int argc, char *argv[], char *envp[]);
void X_main_loop_iteration();
int X_close();

HINSTANCE MChInst;
char *MCcmdline;

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
	char **argv;
	char **envp;
	bool success;
};

static void DisplayStartupErrorAndExit(void)
{
	const char *mcap;
	const char *mtext;
	MCModeGetStartupErrorMessage(mcap, mtext);
	MessageBoxA(HWND_DESKTOP, mtext, mcap, MB_APPLMODAL | MB_OK);
	exit(-1);
}

// The 'init' fiber is used to execute X_init.
static void CALLBACK InitializeFiberRoutine(void *p_context)
{
	InitializeFiberContext *context;
	context = (InitializeFiberContext *)p_context;

	g_mainthread_errno = _errno();

	context -> success = X_init(context -> argc, context -> argv, context -> envp);

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

	int argc = 1;
	char *argv[MAXARGS];
	char cmdbuffer[MAX_PATH];
	char *sptr = NULL;
	int nEnvVar = 0;
	int sizeEnvVar = 0;
	char **envp = NULL;
	char *envStrings = NULL;

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

	MChInst = hInstance;
	GetModuleFileNameA(NULL, cmdbuffer, MAX_PATH);
	sptr = cmdbuffer;
	if (sptr != NULL && *sptr)
	{
		do
		{
			if (*sptr == '/')
				*sptr = '\\';
			else
				if (*sptr == '\\')
					*sptr = '/';
		}
		while (*++sptr);
	}
	argv[0] = cmdbuffer;
	sptr = lpCmdLine;
	while (*sptr)
	{
		if (*sptr == '\\')
			*sptr = '/';
		sptr++;
	}
	sptr = lpCmdLine;
	MCcmdline = strdup(lpCmdLine);
	while (*sptr && argc < MAXARGS)
	{
		while (isspace(*sptr))
			sptr++;
		if (*sptr == '"')
		{
			argv[argc++] = ++sptr;
			while (*sptr && *sptr != '"')
				sptr++;
		}
		else
		{
			argv[argc++] = sptr;
			while(*sptr && !isspace(*sptr))
				sptr++;
		}
		if (*sptr)
			*sptr++ = '\0';
	}
#undef GetEnvironmentStrings
	envStrings = (char *)GetEnvironmentStrings();// and SetEnvironmentVariable
	sptr = envStrings;
	while ((sizeEnvVar = strlen(sptr)) > 0)
	{
		envp = (char **)realloc(envp, (nEnvVar + 1) *  sizeof(char *));
		envp[nEnvVar++] = sptr;
		sptr = sptr + sizeEnvVar + 1; //move sptr to next set the env var set
	}
	envp = (char **)realloc(envp, (nEnvVar + 1) *  sizeof(char *));
	envp[nEnvVar] = NULL;

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
	t_init . argv = argv;
	t_init . envp = envp;
	t_init . success = False;

	void *t_init_fiber;
	t_init_fiber = CreateFiberEx(64 * 1024, MCstacklimit, 0, InitializeFiberRoutine, &t_init);
	if (t_init_fiber == nil)
		exit(-1);

	SwitchToFiber(t_init_fiber);
	DeleteFiber(t_init_fiber);

	if (!t_init . success)
		DisplayStartupErrorAndExit();

	FreeEnvironmentStringsA(envStrings);

	// Now we loop continually until quit. If 'X_main_loop' returns without quitting
	// it means a stack size change request has occured.
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

	if (t_tsf_mgr != nil)
		t_tsf_mgr -> Release();

	delete envp;
	delete MCcmdline;

	return r;
}

////////////////////////////////////////////////////////////////////////////////
