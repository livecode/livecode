/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "srvscript.h"
#include "variable.h"
#include "osspec.h"
#include "system.h"
#include "dispatch.h"
#include "mcerror.h"
#include "execpt.h"
#include "object.h"
#include "hndlrlst.h"
#include "handler.h"
#include "param.h"
#include "scriptpt.h"
#include "util.h"
#include "uidc.h"
#include "font.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef _IREVIAM

#include "revbuild.h"
#include <sys/resource.h>
#include "srvdebug.h"

#define HOME_ENV_VAR "REV_HOME"
#define HOME_FOLDER "/opt/livecode/" MC_BUILD_ENGINE_SHORT_VERSION
#else
#define HOME_ENV_VAR "LIVECODE_SERVER_HOME"
#define HOME_FOLDER "/opt/runrev/livecode-server"
#endif

////////////////////////////////////////////////////////////////////////////////

// The server engine installation's home folder.
static char *s_server_home = NULL;

// If true, the server engine is running in CGI mode
static bool s_server_cgi = false;

// The main script the server engine will run.
char *MCserverinitialscript = NULL;

// The root server script object.
MCServerScript *MCserverscript = NULL;

// The current error mode.
MCSErrorMode MCservererrormode = kMCSErrorModeStderr;

// The current output encoding.
MCSOutputTextEncoding MCserveroutputtextencoding = kMCSOutputTextEncodingNative;

// The current output line ending
MCSOutputLineEndings MCserveroutputlineendings = kMCSOutputLineEndingsNative;

// The array of current CGI headers (if any).
char **MCservercgiheaders = NULL;
uint32_t MCservercgiheadercount = 0;

// The array of set cookies
struct mcservercookie_t *MCservercgicookies = NULL;
uint32_t MCservercgicookiecount = 0;

// The current document root of the CGI execution.
char *MCservercgidocumentroot = NULL;

// The session data save path
char *MCsessionsavepath = NULL;

// The session cookie name
char *MCsessionname = NULL;

// The session ID of the current session
char *MCsessionid = NULL;

// The lifetime of session data in seconds.  default = 24mins
uint32_t MCsessionlifetime = 60 * 24;

////////////////////////////////////////////////////////////////////////////////

/*
 get the clipboardData["text"]
 
 replace "\" with "\\" in it
 replace quote with "\" & quote in it
 replace return with "\n\" & return in it
 
 set the clipboardData["text"] to quote & it & quote
*/

/*
command scriptExecutionError pStack, pFiles
  local tLine, tHint

  if the errorMode is "inline" then
    put "<pre>" & return
  end if

  put line 1 of pStack into tLine
  if item 1 of tLine is 730 then
    get "file" && quote & (line (item 4 of tLine) of pFiles) & quote & return
    if the errorMode is "inline" then put content it else write it to stderr
    repeat forever
      delete line 1 of pStack
      put line 1 of pStack into tLine
      if item 1 of tLine is 730 then
        delete line 1 of pStack
        exit repeat
      end if
      if item 4 of tLine is not empty then
        put " (" & item 4 of tLine & ")" into tHint
      else
        put empty into tHint
      end if
      get format("  row %d, col %d: %s%s\n", item 2 of tLine, item 3 of tLine, line (item 1 of tLine) of the scriptParsingErrors, tHint)
      if the errorMode is "inline" then put content it else write it to stderr
    end repeat
    exit scriptExecutionError
  end if
  
  local tCurrentObject
  repeat while pStack is not empty
    local tNextObject
    repeat for each line tLine in pStack
      if item 1 of tLine is 728 then
        put "file" && quote & line (item 4 of tLine) of pFiles & quote into tNextObject
        exit repeat
      else if item 1 of tLine is 353 then
        put item 4 to -1 of tLine into tNextObject
        exit repeat
      end if
    end repeat

    if tNextObject is not tCurrentObject then
      put tNextObject into tCurrentObject
      get tCurrentObject & return
      if the errorMode is "inline" then put content it else write it to stderr
    end if

    put line 1 of pStack into tLine
    delete line 1 of pStack

    if item 1 of tLine is not among the items of "728,353,741" then
      local tRow, tCol, tMessage
      put item 2 of tLine into tRow
      put item 3 of tLine into tCol
      put line (item 1 of tLine) of the scriptExecutionErrors into tMessage
      if item 4 of tLine is not empty then
        put " (" & item 4 of tLine & ")" into tHint
      else
        put empty into tHint
      end if
      if item 2 of tLine is 0 then
        repeat for each line tLine in pStack
          if item 2 of tLine is not 0 then
            put item 2 of tLine into tRow
            put item 3 of tLine into tCol
            exit repeat
          end if
        end repeat
      end if
      get format("  row %d, col %d: %s%s\n", tRow, tCol, tMessage, tHint)
      if the errorMode is "inline" then put content it else write it to stderr
    end if
  end repeat

  if the errorMode is "inline" then
    put "</pre>" & return
  end if
end scriptExecutionError
*/

const char *s_default_error_handler = "\
scriptExecutionError pStack, pFiles\n\
  local tLine, tHint\n\
\n\
  if the errorMode is \"inline\" then\n\
    put \"<pre>\" & return\n\
  end if\n\
\n\
  put line 1 of pStack into tLine\n\
  if item 1 of tLine is 730 then\n\
    get \"file\" && quote & (line (item 4 of tLine) of pFiles) & quote & return\n\
    if the errorMode is \"inline\" then put content it else write it to stderr\n\
    repeat forever\n\
      delete line 1 of pStack\n\
      put line 1 of pStack into tLine\n\
      if item 1 of tLine is 730 then\n\
        delete line 1 of pStack\n\
        exit repeat\n\
      end if\n\
      if item 4 of tLine is not empty then\n\
        put \" (\" & item 4 of tLine & \")\" into tHint\n\
      else\n\
        put empty into tHint\n\
      end if\n\
      get format(\"  row %d, col %d: %s%s\\n\", item 2 of tLine, item 3 of tLine, line (item 1 of tLine) of the scriptParsingErrors, tHint)\n\
      if the errorMode is \"inline\" then put content it else write it to stderr\n\
    end repeat\n\
    exit scriptExecutionError\n\
  end if\n\
  \n\
  local tCurrentObject\n\
  repeat while pStack is not empty\n\
    local tNextObject\n\
    repeat for each line tLine in pStack\n\
      if item 1 of tLine is 728 then\n\
        put \"file\" && quote & line (item 4 of tLine) of pFiles & quote into tNextObject\n\
        exit repeat\n\
      else if item 1 of tLine is 353 then\n\
        put item 4 to -1 of tLine into tNextObject\n\
        exit repeat\n\
      end if\n\
    end repeat\n\
\n\
    if tNextObject is not tCurrentObject then\n\
      put tNextObject into tCurrentObject\n\
      get tCurrentObject & return\n\
      if the errorMode is \"inline\" then put content it else write it to stderr\n\
    end if\n\
\n\
    put line 1 of pStack into tLine\n\
    delete line 1 of pStack\n\
\n\
    if item 1 of tLine is not among the items of \"728,353,741\" then\n\
      local tRow, tCol, tMessage\n\
      put item 2 of tLine into tRow\n\
      put item 3 of tLine into tCol\n\
      put line (item 1 of tLine) of the scriptExecutionErrors into tMessage\n\
      if item 4 of tLine is not empty then\n\
        put \" (\" & item 4 of tLine & \")\" into tHint\n\
      else\n\
        put empty into tHint\n\
      end if\n\
      if item 2 of tLine is 0 then\n\
        repeat for each line tLine in pStack\n\
          if item 2 of tLine is not 0 then\n\
            put item 2 of tLine into tRow\n\
            put item 3 of tLine into tCol\n\
            exit repeat\n\
          end if\n\
        end repeat\n\
      end if\n\
      get format(\"  row %d, col %d: %s%s\\n\", tRow, tCol, tMessage, tHint)\n\
      if the errorMode is \"inline\" then put content it else write it to stderr\n\
    end if\n\
  end repeat\n\
\n\
  if the errorMode is \"inline\" then\n\
    put \"</pre>\" & return\n\
  end if\n\
end scriptExecutionError\
";

////////////////////////////////////////////////////////////////////////////////

MCTheme *MCThemeCreateNative(void)
{
	return nil;
}

MCUIDC *MCCreateScreenDC(void)
{
	return new MCUIDC;
}

////////////////////////////////////////////////////////////////////////////////

static uint2 nvars;

static void create_var(char *v)
{
	char vname[U2L + 1];
	sprintf(vname, "$%d", nvars);
	nvars++;
	
	MCVariable *tvar;
	/* UNCHECKED */ MCVariable::ensureglobal_cstring(vname, tvar);
	tvar->copysvalue(v);

	MCU_realloc((char **)&MCstacknames, MCnstacks, MCnstacks + 1, sizeof(char *));
	MCstacknames[MCnstacks++] = v;
}

static void create_var(uint4 p_v)
{
	MCVariable *tvar;
	/* UNCHECKED */ MCVariable::ensureglobal_cstring("$#", tvar);
	tvar->setnvalue(p_v);
}

static Boolean byte_swapped()
{
	uint2 test = 1;
	return *((uint1 *)&test);
}

////////////////////////////////////////////////////////////////////////////////

bool X_open(int argc, char *argv[], char *envp[]);
int X_close();

extern bool cgi_initialize();
extern void cgi_finalize(void);
extern void MCU_initialize_names();

bool X_init(int argc, char *argv[], char *envp[])
{
	int i;
	MCstackbottom = (char *)&i;

	////
	
	MCswapbytes = byte_swapped();
	MCtruemcstring = MCtruestring;
	MCfalsemcstring = MCfalsestring;
	MCnullmcstring = MCnullmcstring;

	////

	MCS_init();

	////
	
	MCNameInitialize();
	MCU_initialize_names();
	
	// MW-2012-02-23: [[ FontRefs ]] Initialize the font module.
	MCFontInitialize();
	// MW-2012-02-23: [[ FontRefs ]] Initialize the logical font table module.
	MCLogicalFontTableInitialize();
	
	////

	// Store the engine path in MCcmd.
	char *t_native_command;
	t_native_command = MCsystem -> ResolveNativePath(argv[0]);
	MCcmd = MCsystem -> PathFromNative(t_native_command);
	delete t_native_command;
	
	// Fetch the home folder (for resources and such) - this is either that which
	// is specified by REV_HOME environment variable, or the folder containing the
	// engine.
	char *t_native_home;
	t_native_home = MCS_getenv(HOME_ENV_VAR);
	if (t_native_home != NULL)
	{
		t_native_home = MCsystem -> ResolveNativePath(t_native_home);
		s_server_home = MCsystem -> PathFromNative(t_native_home);
		delete t_native_home;
	}
	else if (MCsystem -> FolderExists(HOME_FOLDER))
		s_server_home = strdup(HOME_FOLDER);
	else
	{
		s_server_home = strdup(MCcmd);
		(strrchr(s_server_home, '/'))[0] = '\0';
	}

	// Check for CGI mode.

	s_server_cgi = MCS_getenv("GATEWAY_INTERFACE") != NULL;
	
	if (s_server_cgi)
	{
		MCS_set_errormode(kMCSErrorModeInline);
		
		if (!cgi_initialize())
			return False;

		// MW-2011-08-02: If we initialize as cgi we *don't* want env vars to
		//   be created.
		envp = nil;
	}
	else
	{
		MCS_set_errormode(kMCSErrorModeStderr);
		
		// If there isn't at least one argument, we haven't got anything to run.
		if (argc > 1)
			MCserverinitialscript = MCsystem -> ResolveNativePath(argv[1]);
		else
			MCserverinitialscript = NULL;

        // SN-2015-03-16: [[ Bug 14897 ]] Start to enumerate the params from 1
        //  so that we have the script listed in the params as well (in a
        //  bash-like way)
		// Create the $<n> variables.
        for(int i = 1; i < argc; ++i)
			if (argv[i] != nil)
			create_var(argv[i]);
		create_var(nvars);
	}
	
	return X_open(argc, argv, envp);
}
	
static void IO_printf(IO_handle stream, const char *format, ...)
{
	char t_buffer[4096];
	va_list args;
	va_start(args, format);
	vsprintf(t_buffer, format, args);
	va_end(args);
	MCS_write(t_buffer, 1, strlen(t_buffer), stream);
}

static bool load_extension_callback(void *p_context, const MCSystemFolderEntry *p_entry)
{
	MCServerScript *t_script;
	t_script = static_cast<MCServerScript *>(p_context);
	
	if (p_entry -> is_folder)
		return true;
	
	char *t_filename;
	if (!MCCStringFormat(t_filename, "%s/externals/%s", s_server_home, p_entry -> name))
		return false;

	MCdispatcher -> loadexternal(t_filename);
	
	MCCStringFree(t_filename);

	return true;
}

static void X_load_extensions(MCServerScript *p_script)
{
	char *t_dir;
	t_dir = MCS_getcurdir();
	
	if (MCS_setcurdir(s_server_home) &&
		MCS_setcurdir("externals"))
		MCsystem -> ListFolderEntries(load_extension_callback, p_script);
	
	MCS_setcurdir(t_dir);
	delete t_dir;
}

void X_main_loop(void)
{
	int i;
	MCstackbottom = (char *)&i;

	if (MCserverinitialscript == NULL)
		return;
	
	MCperror -> clear();
	MCeerror -> clear();
	
	MCserverscript = static_cast<MCServerScript *>(MCdispatcher -> gethome());
	
	X_load_extensions(MCserverscript);
	
#ifdef _IREVIAM
	rlim_t t_cpu_time_limit, t_data_limit;
	t_cpu_time_limit = 30;
	t_data_limit = 64 * 1024 * 1024;
	
	if (s_server_cgi)
	{
		const char *t_port;
		t_port = getenv("SERVER_PORT");
		if (t_port != NULL && atoi(t_port) == 7309)
		{
			if (MCServerDebugConnect(getenv("SERVER_NAME"), getenv("REQUEST_URI")))
			{
				t_cpu_time_limit += t_cpu_time_limit / 2;
				t_data_limit += t_data_limit / 2;
			}
		}
	}

	struct rlimit t_limits;
	t_limits . rlim_cur = t_cpu_time_limit;
	t_limits . rlim_max = t_cpu_time_limit;
	if (setrlimit(RLIMIT_CPU, &t_limits) < 0)
		return;
		
	t_limits . rlim_cur = t_data_limit;
	t_limits . rlim_max = t_data_limit;
	if (setrlimit(RLIMIT_AS, &t_limits) < 0)
		return;
	
	t_limits . rlim_cur = t_data_limit / 2;
	t_limits . rlim_max = t_data_limit / 2;
	if (setrlimit(RLIMIT_RSS, &t_limits) < 0)
		return;
#endif
	
	MCExecPoint ep;
	if (!MCserverscript -> Include(ep, MCserverinitialscript, false) &&
		MCS_get_errormode() != kMCSErrorModeDebugger)
	{
		char *t_eerror, *t_efiles;
		t_eerror = MCeerror -> getsvalue() . clone();
		MCserverscript -> ListFiles(ep);
		t_efiles = ep . getsvalue() . clone();
		MCeerror -> clear();
		
		MCParameter t_exec_stack, t_files;
		t_exec_stack . sets_argument(t_eerror);
		t_exec_stack . setnext(&t_files);
		t_files . sets_argument(t_efiles);
		
		Exec_stat t_stat;
		t_stat = MCserverscript -> message(MCM_script_execution_error, &t_exec_stack);
		if (t_stat == ES_NOT_HANDLED && MCS_get_errormode() != kMCSErrorModeQuiet)
		{
			MCHandlerlist *t_handlerlist;
			t_handlerlist = new MCHandlerlist;
			
			MCHandler *t_handler;
			t_handler = new MCHandler(HT_MESSAGE, true);
			
			MCScriptPoint sp(MCserverscript, t_handlerlist, s_default_error_handler);
			
			Parse_stat t_parse_stat;
			t_parse_stat = t_handler -> parse(sp, false);
			t_stat = MCserverscript -> exechandler(t_handler, &t_exec_stack);
			
			delete t_handler;
			delete t_handlerlist;
		}
		
		if ((t_stat != ES_NORMAL && t_stat != ES_PASS) && MCS_get_errormode() != kMCSErrorModeQuiet)
		{
			IO_printf(IO_stderr, "ERROR:\n%s\n", t_eerror);
			IO_printf(IO_stderr, "FILES:\n%s\n", t_efiles);
		}

		delete t_eerror;
		delete t_efiles;
	}
	
	if (s_server_cgi)
		cgi_finalize();
#ifdef _IREVIAM
	if (s_server_cgi)
		MCServerDebugDisconnect();
#endif
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[], char *envp[])
{
	if (!X_init(argc, argv, envp))
		exit(-1);
	
	X_main_loop();
	
	int t_exit_code;
	t_exit_code = X_close();
	
	exit(t_exit_code);
}

////////////////////////////////////////////////////////////////////////////////
