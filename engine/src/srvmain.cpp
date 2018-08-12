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
#include "srvscript.h"
#include "variable.h"
#include "osspec.h"
#include "system.h"
#include "dispatch.h"
#include "mcerror.h"

#include "exec.h"
#include "object.h"
#include "hndlrlst.h"
#include "handler.h"
#include "param.h"
#include "scriptpt.h"
#include "util.h"
#include "uidc.h"
#include "font.h"
#include "libscript/script.h"
#include "eventqueue.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef _IREVIAM

#include "revbuild.h"
#include <sys/resource.h>
#include "srvdebug.h"

#define HOME_ENV_VAR "REV_HOME"
#define HOME_FOLDER "/opt/livecode/" MC_BUILD_ENGINE_SHORT_VERSION
#else
#define HOME_ENV_VAR "LIVECODE_SERVER_HOME"
#define HOME_FOLDER "/opt/livecode/server"
#endif

////////////////////////////////////////////////////////////////////////////////

// The server engine installation's home folder.
static MCStringRef s_server_home = NULL;

// If true, the server engine is running in CGI mode
static bool s_server_cgi = false;

// The main script the server engine will run.

MCStringRef MCserverinitialscript = nil;

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
MCStringRef MCservercgidocumentroot = NULL;

// The session data save path
MCStringRef MCsessionsavepath = NULL;

// The session cookie name
MCStringRef MCsessionname = NULL;

// The session ID of the current session
MCStringRef MCsessionid = NULL;

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

static void create_var(MCStringRef p_var)
{
	MCAutoStringRef t_vname;
	/* UNCHECKED */ MCStringFormat(&t_vname, "$%d", nvars++);
	
	MCVariable *tvar;
	MCNewAutoNameRef t_name;
	/* UNCHECKED */ MCNameCreate(*t_vname, &t_name);
	/* UNCHECKED */ MCVariable::ensureglobal(*t_name, tvar);
	tvar->setvalueref(p_var);
	
	MCU_realloc((char **)&MCstacknames, MCnstacks, MCnstacks + 1, sizeof(MCStringRef));
	MCstacknames[MCnstacks++] = MCValueRetain(p_var);
}

static void create_var(uint4 p_v)
{
	MCVariable *tvar;
	/* UNCHECKED */ MCVariable::ensureglobal(MCNAME("$#"), tvar);
	tvar->setnvalue(p_v);
}

static Boolean byte_swapped()
{
	uint2 test = 1;
	return *((uint1 *)&test);
}

////////////////////////////////////////////////////////////////////////////////

extern bool cgi_initialize();
extern void cgi_finalize(void);

static void
X_initialize_mccmd(const X_init_options& p_options)
{
    MCSAutoLibraryRef t_self;
    MCSLibraryCreateWithAddress(reinterpret_cast<void *>(X_initialize_mccmd),
                                &t_self);
    MCSLibraryCopyPath(*t_self,
                       MCcmd);
}

static void
X_initialize_mcappcodepath(const X_init_options& p_options)
{
    if (p_options.app_code_path != nullptr)
    {
        MCappcodepath = MCValueRetain(p_options.app_code_path);
        return;
    }
    
    MCU_path_split(MCcmd,
                   &MCappcodepath,
                   nullptr);
}

bool X_init(const X_init_options& p_options)
{
    int argc = p_options.argc;
    MCStringRef *argv = p_options.argv;
    MCStringRef *envp = p_options.envp;
    
	int t_bottom;
	MCstackbottom = (char *)&t_bottom;

    ////

    // SN-2014-08-14: [[ Bug 13177 ]] X_clear_globals sets MCswapbytes to False
    X_clear_globals();

	////
	
	MCswapbytes = byte_swapped();
	MCtruemcstring = MCtruestring;
	MCfalsemcstring = MCfalsestring;
	MCnullmcstring = MCnullmcstring;

	////

#ifdef _WINDOWS_SERVER
	// MW-2011-07-26: Make sure errno pointer is initialized - this won't be
	//   if the engine is running through the plugin.
	extern int *g_mainthread_errno;
	if (g_mainthread_errno == nil)
		g_mainthread_errno = _errno();

	// Call to _wgetenv needed to initialise the WCHAR environment variables
	wchar_t *t_dummy;
	t_dummy = _wgetenv(L"PATH");
#endif

	////
    
	MCS_init();

    /* Set up MCcmd correctly - this is the path to the loadable object
     * containing this folder. */
    X_initialize_mccmd(p_options);
    
    /* Setup MCappcodepath correctly - this is the folder containing the
     * MCcmd. */
    X_initialize_mcappcodepath(p_options);

	////
	
	X_initialize_names();
	
	// MW-2012-02-23: [[ FontRefs ]] Initialize the font module.
	MCFontInitialize();
	// MW-2012-02-23: [[ FontRefs ]] Initialize the logical font table module.
	MCLogicalFontTableInitialize();
	
	// Initialize the event queue
	MCEventQueueInitialize();
	
	////
    
    // Create the basic locale and the system locale
    if (!MCLocaleCreateWithName(MCSTR("en_US"), kMCBasicLocale))
        return false;
    kMCSystemLocale = MCS_getsystemlocale();
    if (kMCSystemLocale == nil)
        return false;
    
    ////
	
	// Fetch the home folder (for resources and such) - this is either that which
	// is specified by REV_HOME environment variable, or the folder containing the
	// engine.
	MCAutoStringRef t_native_home;

	if (MCS_getenv(MCSTR(HOME_ENV_VAR), &t_native_home))
	{
		MCAutoStringRef t_resolved_home;
		MCsystem -> ResolvePath(*t_native_home, &t_resolved_home);
		MCsystem -> PathFromNative(*t_resolved_home, s_server_home);
	}
	else if (MCsystem -> FolderExists(MCSTR(HOME_FOLDER)))
		s_server_home = MCSTR(HOME_FOLDER);
	else
    {
        // SN-2014-12-16: [[ Bug 14001 ]] We can use MCcmd, no need to copy it into s_server_home
		uindex_t t_last_separator;
        MCStringLastIndexOfChar(MCcmd, PATH_SEPARATOR, UINDEX_MAX, kMCStringOptionCompareExact, t_last_separator);

		MCAutoStringRef tmp_s_server_home;
        // SN-2014-12-08: [[ Bug 14001 ]] The path to the externals goes up to the last separator, not one character before.
        /* UNCHECKED */ MCStringCopySubstring(MCcmd, MCRangeMake(0, t_last_separator), &tmp_s_server_home);
		s_server_home = MCValueRetain(*tmp_s_server_home);
	}

	// Check for CGI mode.
    MCAutoStringRef t_env;
	
	if (MCS_getenv(MCSTR("GATEWAY_INTERFACE"), &t_env))
		s_server_cgi = true;
	else
        s_server_cgi = false;
	
	if (!X_open(argc, argv, envp))
		return False;

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
			MCsystem -> ResolvePath(argv[1], MCserverinitialscript);
		else
			MCserverinitialscript = nil;
		
		// Create the $<n> variables.
		for(int i = 2; i < argc; ++i)
			if (argv[i] != nil)
			create_var(argv[i]);
		create_var(nvars);
    }
	
	return True;
}
	
static void IO_printf(IO_handle stream, const char *format, ...)
{
    MCAutoStringRef t_string;
	va_list args;
	va_start(args, format);
    MCStringFormatV(&t_string, format, args);
	va_end(args);
    
    MCAutoStringRefAsSysString t_sys_string;
    t_sys_string . Lock(*t_string);
	MCS_write(*t_sys_string, 1, t_sys_string . Size(), stream);
}

static bool load_extension_callback(void *p_context, const MCSystemFolderEntry *p_entry)
{
	if (p_entry -> is_folder)
		return true;
	
	MCAutoStringRef t_filename;
    if (!MCStringFormat(&t_filename, "%@/externals/%@", s_server_home, p_entry -> name))
		return false;

	MCdispatcher -> loadexternal(*t_filename);

	return true;
}

static void X_load_extensions(MCServerScript *p_script)
{
	MCAutoStringRef t_dir;
	MCS_getcurdir(&t_dir);

	if (MCS_setcurdir(s_server_home) &&
		MCS_setcurdir(MCSTR("externals")))
		MCsystem -> ListFolderEntries(nil, load_extension_callback, p_script);
	
	MCS_setcurdir(*t_dir);
	
}

void X_main_loop(void)
{
	int i;
	MCstackbottom = (char *)&i;
	

	if (MCserverinitialscript == nil)
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
			MCAutoStringRef t_server_name, t_request_uri;
			/* UNCHECKED */ MSC_getenv(MCSTR("SERVER_NAME"), &t_server_name);
			/* UNCHECKED */ MSC_getenv(MCSTR("REQUEST_URI"), &t_request_uri);
			if (MCServerDebugConnect(*t_server_name, *t_request_uri))
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
	
	MCExecContext ctxt;
	if (!MCserverscript -> Include(ctxt, MCserverinitialscript, false) &&
		MCS_get_errormode() != kMCSErrorModeDebugger)
	{
		MCAutoStringRef t_eerror, t_efiles;
		/* UNCHECKED */ MCeerror->copyasstringref(&t_eerror);
		MCserverscript -> ListFiles(&t_efiles);
		MCeerror -> clear();
		
		MCParameter t_exec_stack, t_files;
		t_exec_stack . setvalueref_argument(*t_eerror);
		t_exec_stack . setnext(&t_files);
		t_files . setvalueref_argument(*t_efiles);
		
		Exec_stat t_stat;
		t_stat = MCserverscript -> message(MCM_script_execution_error, &t_exec_stack);
		if (t_stat == ES_NOT_HANDLED && MCS_get_errormode() != kMCSErrorModeQuiet)
		{
			MCHandlerlist *t_handlerlist;
			t_handlerlist = new (nothrow) MCHandlerlist;
			
			MCHandler *t_handler;
			t_handler = new (nothrow) MCHandler(HT_MESSAGE, true);
			
			MCScriptPoint sp(MCserverscript, t_handlerlist, MCSTR(s_default_error_handler));
			
			Parse_stat t_parse_stat;
			t_parse_stat = t_handler -> parse(sp, false);
			if (t_parse_stat != PS_NORMAL)
			{
				t_stat = ES_ERROR;
			}
				else
			{
				t_stat = MCserverscript -> exechandler(t_handler, &t_exec_stack);
			}
			
			delete t_handler;
			delete t_handlerlist;
		}
		
		if ((t_stat != ES_NORMAL && t_stat != ES_PASS) && MCS_get_errormode() != kMCSErrorModeQuiet)
		{
			IO_printf(IO_stderr, "ERROR:\n%@\n", *t_eerror);
			IO_printf(IO_stderr, "FILES:\n%@\n", *t_efiles);
		}
	}
	
	if (s_server_cgi)
		cgi_finalize();
#ifdef _IREVIAM
	if (s_server_cgi)
		MCServerDebugDisconnect();
#endif
}

////////////////////////////////////////////////////////////////////////////////

int platform_main(int argc, char *argv[], char *envp[])
{
	if (!MCInitialize() ||
        !MCSInitialize() ||
	    !MCScriptInitialize())
		exit(-1);
    
// THIS IS MAC SPECIFIC AT THE MOMENT BUT SHOULD WORK ON LINUX

	// On OSX, argv and envp are encoded as UTF8
	MCStringRef *t_new_argv;
	/* UNCHECKED */ MCMemoryNewArray(argc, t_new_argv);
	
	for (int t_arg = 0; t_arg < argc; t_arg++)
	{
		/* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)argv[t_arg], strlen(argv[t_arg]), kMCStringEncodingUTF8, false, t_new_argv[t_arg]);
	}
	
	MCStringRef *t_new_envp;
	/* UNCHECKED */ MCMemoryNewArray(1, t_new_envp);
	
	uindex_t t_env = 0;
	uindex_t t_envp_count = 0;
	
	while (envp[t_env] != NULL)
	{
		t_envp_count++;
		uindex_t t_count = t_env;
		/* UNCHECKED */ MCMemoryResizeArray(t_env + 1, t_new_envp, t_count);
		/* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)envp[t_env], strlen(envp[t_env]), kMCStringEncodingUTF8, false, t_new_envp[t_env]);
		t_env++;
	}
	
	/* UNCHECKED */ MCMemoryResizeArray(t_env + 1, t_new_envp, t_envp_count);
	t_new_envp[t_env] = nil;
// END MAC SPECIFIC	

    struct X_init_options t_options;
    t_options.argc = argc;
    t_options.argv = t_new_argv;
    t_options.envp = t_new_envp;
    t_options.app_code_path = nullptr;
	if (!X_init(t_options))
		exit(-1);
	
	X_main_loop();
	
	int t_exit_code;
	t_exit_code = X_close();

	for (int t_arg = 0; t_arg < argc; t_arg++)
	{
		MCValueRelease(t_new_argv[t_arg]);
	}
	MCMemoryDeleteArray(t_new_argv);

	for (t_env = 0; t_env < t_envp_count; t_env++)
	{
		MCValueRelease(t_new_envp[t_env]);
	}
	MCMemoryDeleteArray(t_new_envp);

    MCScriptFinalize();
	MCFinalize();

	exit(t_exit_code);
}

////////////////////////////////////////////////////////////////////////////////
