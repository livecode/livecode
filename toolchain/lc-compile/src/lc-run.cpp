/*                                                                     -*-c++-*-
Copyright (C) 2015 Runtime Revolution Ltd.

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

#include <foundation.h>
#include <foundation-system.h>
#include <foundation-auto.h>
#include <script.h>

#if defined(__WINDOWS__)
#	include <windows.h>
#endif

/* Possible exit statuses used by lc-run */
enum {
	kMCRunExitStatusSuccess = 0,
	kMCRunExitStatusStartup = 124,
	kMCRunExitStatusBadArgs = 125,
	kMCRunExitStatusUncaughtError = 126,
};

struct MCRunConfiguration
{
	MCStringRef m_filename;
};

static void MCRunUsage (int p_exit_status) ATTRIBUTE_NORETURN;
static void MCRunStartupError (void) ATTRIBUTE_NORETURN;
static void MCRunHandlerError (void) ATTRIBUTE_NORETURN;
static void MCRunPrintMessage (FILE *p_stream, MCStringRef p_message);


/* ----------------------------------------------------------------
 * Error helper functions
 * ---------------------------------------------------------------- */

/* Print a message if no arguments are provided */
static void
MCRunUsage (int p_exit_status)
{
	fprintf (stderr,
"Usage: lc-run LCMFILE [ARGS ...]\n"
"\n"
"Run a compiled Modular Livecode bytecode file.\n"
"\n"
"Any ARGS are available in \"the command arguments\".\n"
"\n"
"Report bugs to <http://quality.runrev.com/>\n"
	         );
	exit (p_exit_status);
}

/* Print an error message if an error occurs while starting the
 * LiveCode runtime */
static void
MCRunStartupError (void)
{
	MCAutoStringRef t_message, t_reason;
	MCErrorRef t_error;

	if (MCErrorCatch (t_error))
		t_reason = MCErrorGetMessage (t_error);
	else
		/* UNCHECKED */ MCStringCopy (MCSTR("Unknown error"), &t_reason);

	/* UNCHECKED */ MCStringFormat (&t_message, "ERROR: %@\n", *t_reason);

	MCRunPrintMessage (stderr, *t_message);
	exit (kMCRunExitStatusStartup);
}

/* Print an error message if an uncaught error occurs in the LiveCode
 * handler */
static void
MCRunHandlerError (void)
{
	MCAutoStringRef t_message, t_reason;
	MCErrorRef t_error;

	if (MCErrorCatch (t_error))
		t_reason = MCErrorGetMessage (t_error);
	else
		/* UNCHECKED */ MCStringCopy (MCSTR("Unknown error"), &t_reason);

	/* UNCHECKED */ MCStringFormat (&t_message,
	                                "ERROR: Uncaught error: %@\n",
	                                *t_reason);

	MCRunPrintMessage (stderr, *t_message);
	exit (kMCRunExitStatusUncaughtError);
}

static void
MCRunPrintMessage (FILE *p_stream,
                   MCStringRef p_message)
{
	MCAssert (p_stream);
	MCAssert (p_message);

#if defined(__WINDOWS__)
	MCAutoStringRefAsCString t_sys;
#else
	MCAutoStringRefAsSysString t_sys;
#endif
	/* UNCHECKED */ t_sys.Lock (p_message);
	fprintf (p_stream, "%s", *t_sys);
}

/* ----------------------------------------------------------------
 * Command-line argument processing
 * ---------------------------------------------------------------- */

static bool
MCRunParseCommandLine (int argc,
                       const char *argv[],
                       MCRunConfiguration & x_config)
{
#if defined(__WINDOWS__)
	if (!MCSCommandLineCaptureWindows())
		return false;
#else
	if (!MCSCommandLineCapture (argc, argv))
		return false;
#endif

	bool t_have_filename = false;
	MCAutoStringRef t_filename;
	MCAutoProperListRef t_raw_args, t_args;

	if (!MCProperListCreateMutable (&t_args))
		return false;

	if (!MCSCommandLineGetArguments (&t_raw_args))
		return false;

	/* FIXME Once we have "real" command line arguments, process them
	 * in this loop. */
	MCValueRef t_arg_val;
	uintptr_t t_raw_args_iter = 0;
	while (MCProperListIterate (*t_raw_args, t_raw_args_iter, t_arg_val))
	{
		MCAssert (MCTypeInfoConforms (MCValueGetTypeInfo (t_arg_val),
		                              kMCStringTypeInfo));
		MCStringRef t_arg = (MCStringRef) t_arg_val;

		if (!t_have_filename)
		{
			t_filename = MCValueRetain (t_arg);
			t_have_filename = true;
		}
		else
		{
			if (!MCProperListPushElementOntoBack (*t_args, t_arg_val))
				return false;
		}
	}

	/* Check that we found a bytecode filename */
	if (!t_have_filename)
	{
		fprintf(stderr, "ERROR: No bytecode filename specified.\n\n");
		MCRunUsage (kMCRunExitStatusBadArgs);
	}

	/* Set the "real" command name and arguments, accessible from
	 * LiveCode */
	if (!MCSCommandLineSetName (*t_filename))
		return false;
	if (!MCSCommandLineSetArguments (*t_args))
		return false;

	MCValueAssign (x_config.m_filename, *t_filename);

	return true;
}

/* ----------------------------------------------------------------
 * VM initialisation and launch
 * ---------------------------------------------------------------- */

static bool
MCRunLoadModule (MCStringRef p_filename,
                 MCScriptModuleRef & r_module)
{
	MCAutoDataRef t_module_data;
	MCAutoValueRefBase<MCStreamRef> t_stream;
	MCAutoValueRefBase<MCScriptModuleRef> t_module;

	if (!MCSFileGetContents (p_filename, &t_module_data))
		return false;

	if (!MCMemoryInputStreamCreate (MCDataGetBytePtr (*t_module_data),
	                                MCDataGetLength (*t_module_data),
	                                &t_stream))
		return false;

	if (!MCScriptCreateModuleFromStream (*t_stream, &t_module))
		return false;

	if (!MCScriptEnsureModuleIsUsable (*t_module))
		return false;

	r_module = MCValueRetain(*t_module);
	return true;
}


/* ----------------------------------------------------------------
 * Main program
 * ---------------------------------------------------------------- */

int
main (int argc,
      const char *argv[])
{
	/* Initialise the libraries. We need these for any further processing. */
	MCInitialize();
	MCSInitialize();
	MCScriptInitialize();

	/* Defaults */
	MCRunConfiguration t_config;
	t_config.m_filename = MCValueRetain (kMCEmptyString);

	/* ---------- Process command-line arguments */
	if (!MCRunParseCommandLine (argc, argv, t_config))
		MCRunStartupError();

	/* ---------- Start VM */
	MCAutoValueRefBase<MCScriptModuleRef> t_module;
	MCAutoValueRefBase<MCScriptInstanceRef> t_instance;
	MCAutoValueRef t_ignored_retval;

	if (!MCRunLoadModule (t_config.m_filename, &t_module))
		MCRunStartupError();

	if (!MCScriptCreateInstanceOfModule (*t_module, &t_instance))
		MCRunStartupError();

	if (!MCScriptCallHandlerOfInstance(*t_instance, MCNAME("main"), NULL, 0,
	                                   &t_ignored_retval))
		MCRunHandlerError();

	MCScriptFinalize();
	MCSFinalize();
	MCFinalize();

	exit (0);
}
