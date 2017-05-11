/*                                                                     -*-c++-*-
Copyright (C) 2015-2016 LiveCode Ltd.

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
#include <libscript/script.h>
#include <libscript/script-auto.h>

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
	MCNameRef m_handler;
	MCProperListRef m_load_filenames;
	bool m_list_handlers;
};

static void MCRunUsage (int p_exit_status) ATTRIBUTE_NORETURN;
static void MCRunStartupError (MCStringRef p_stage) ATTRIBUTE_NORETURN;
static void MCRunHandlerError (void) ATTRIBUTE_NORETURN;
static void MCRunBadOptionError (MCStringRef p_arg) ATTRIBUTE_NORETURN;
static void MCRunBadOptionArgError (MCStringRef p_option, MCStringRef p_optarg) ATTRIBUTE_NORETURN;
static void MCRunPrintMessage (FILE *p_stream, MCStringRef p_message);


/* ----------------------------------------------------------------
 * Error helper functions
 * ---------------------------------------------------------------- */

/* Print a message if no arguments are provided */
static void
MCRunUsage (int p_exit_status)
{
	fprintf (stderr,
"Usage: lc-run [OPTIONS] [--] LCMFILE [ARGS ...]\n"
"\n"
"Run a compiled Livecode Builder bytecode file.\n"
"\n"
"Options:\n"
"  -l, --load LCMLIB    Load an additional bytecode file.\n"
"  -H, --handler NAME   Specify name of handler to run.\n"
"      --list-handlers  List possible entry points in LCMFILE and exit.\n"
"  -h, --help           Print this message.\n"
"  --                   Treat next argument as bytecode filename.\n"
"\n"
"Any ARGS are available in \"the command arguments\".\n"
"\n"
"Report bugs to <http://quality.livecode.com/>\n"
	         );
	exit (p_exit_status);
}

/* Print an error message if an error occurs while starting the
 * LiveCode runtime */
static void
MCRunStartupError (MCStringRef p_stage)
{
	MCAutoStringRef t_message, t_reason;
	MCErrorRef t_error;

	if (MCErrorCatch (t_error))
		t_reason = MCErrorGetMessage (t_error);
	else
		/* UNCHECKED */ MCStringCopy (MCSTR("Unknown error"), &t_reason);

	/* UNCHECKED */ MCStringFormat (&t_message, "ERROR[%@]: %@\n",
	                                p_stage, *t_reason);

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
	{
		t_reason = MCErrorGetMessage (t_error);

		/* Print stack trace */
		uindex_t t_num_frames = MCErrorGetDepth (t_error);
		for (uindex_t t_depth = 0; t_depth < t_num_frames; ++t_depth)
		{
			MCAutoStringRef t_frame;
			/* UNCHECKED */ MCStringFormat (&t_frame,
			                                "#%u\tat %@:%u:%u\n",
			                                t_depth,
			                                MCErrorGetTargetAtLevel (t_error, t_depth),
			                                MCErrorGetRowAtLevel (t_error, t_depth),
			                                MCErrorGetColumnAtLevel (t_error, t_depth));
			MCRunPrintMessage (stderr, *t_frame);
		}
	}
	else
	{
		/* UNCHECKED */ MCStringCopy (MCSTR("Unknown error"), &t_reason);
	}

	/* UNCHECKED */ MCStringFormat (&t_message,
	                                "ERROR: Uncaught error: %@\n",
	                                *t_reason);

	MCRunPrintMessage (stderr, *t_message);
	exit (kMCRunExitStatusUncaughtError);
}

/* Print an error message due to an unrecognised command-line
 * option */
static void
MCRunBadOptionError (MCStringRef p_arg)
{
	MCAutoStringRef t_message;
	/* UNCHECKED */ MCStringFormat (&t_message,
	                                "ERROR: Unknown option '%@'\n\n",
	                                p_arg);

	MCRunPrintMessage (stderr, *t_message);
	MCRunUsage (kMCRunExitStatusBadArgs);
}

static void
MCRunBadOptionArgError (MCStringRef p_option,
                        MCStringRef p_optarg)
{
	MCAutoStringRef t_message;
	if (NULL == p_optarg)
		/* UNCHECKED */ MCStringFormat (&t_message, "ERROR: Missing argument for option '%@'\n\n", p_option);
	else
		/* UNCHECKED */ MCStringFormat (&t_message, "ERROR: Bad argument '%@' for option '%@'\n\n", p_optarg, p_option);

	MCRunPrintMessage (stderr, *t_message);
	MCRunUsage (kMCRunExitStatusBadArgs);
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

#define MC_RUN_STRING_EQUAL(s,c) \
	(MCStringIsEqualToCString ((s),(c),kMCStringOptionCompareExact))

/* Some extremely basic command-line argument processing.  This takes
 * a command-line argument and transforms it to a path in the LiveCode
 * internal format (i.e. "/" as separator).
 *
 * FIXME This is currently very basic.  For correctness, it should
 * make the path absolute and then canonicalize it (by removing
 * multiple separators, ".." and "."  elements, and generating an
 * error if invalid path characters are found). */
static bool
MCRunParseCommandLineFilename (MCStringRef p_arg,
                               MCStringRef & r_filename)
{
#if defined (__WINDOWS__)
	/* On Windows, simply turn all backslashes to slashes and hope that the
	 * result is sensible. */
	MCAutoStringRef t_filename;
	return
		MCStringMutableCopy (p_arg, &t_filename) &&
		MCStringFindAndReplaceChar (*t_filename, '\\', '/',
		                            kMCStringOptionCompareExact) &&
		MCStringCopy (*t_filename, r_filename);
#else
	return MCStringCopy (p_arg, r_filename);
#endif
}

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
	bool t_accept_options = true;
	MCAutoStringRef t_filename;
	MCAutoProperListRef t_raw_args, t_args;

	if (!MCProperListCreateMutable (&t_args))
		return false;

	if (!MCSCommandLineGetArguments (&t_raw_args))
		return false;

	/* FIXME Once we have "real" command line arguments, process them
	 * in this loop. */
	uindex_t t_num_args = MCProperListGetLength (*t_raw_args);
	for (uindex_t t_arg_idx = 0; t_arg_idx < t_num_args; ++t_arg_idx)
	{
		MCValueRef t_arg_val, t_argopt_val;
		MCStringRef t_arg, t_argopt;

		t_arg_val = MCProperListFetchElementAtIndex (*t_raw_args, t_arg_idx);
		MCAssert (MCTypeInfoConforms (MCValueGetTypeInfo (t_arg_val),
		                              kMCStringTypeInfo));
		t_arg = (MCStringRef) t_arg_val;

		/* If there's an argument after the current one, fetch it as a possible
		 * option value. */
		uindex_t t_argopt_idx = t_arg_idx + 1;
		if (t_argopt_idx < t_num_args)
		{
			t_argopt_val = MCProperListFetchElementAtIndex (*t_raw_args,
			                                                t_argopt_idx);
			MCAssert (MCTypeInfoConforms (MCValueGetTypeInfo (t_argopt_val),
			                              kMCStringTypeInfo));
			t_argopt = (MCStringRef) t_argopt_val;
		}
		else
		{
			t_argopt = NULL;
		}

		if (t_accept_options)
		{
			if (MC_RUN_STRING_EQUAL (t_arg, "--help") ||
			    MC_RUN_STRING_EQUAL (t_arg, "-h"))
			{
				/* Print help message */
				MCRunUsage (kMCRunExitStatusSuccess);
			}

			if (MC_RUN_STRING_EQUAL (t_arg, "--handler") ||
			    MC_RUN_STRING_EQUAL (t_arg, "-H"))
			{
				if (NULL == t_argopt)
					MCRunBadOptionArgError (t_arg, t_argopt);

				++t_arg_idx; /* Consume option argument */

				/* Set top-level handler */
				MCNewAutoNameRef t_handler_name;
				if (!MCNameCreate (t_argopt, &t_handler_name))
					return false;

				MCValueAssign (x_config.m_handler, *t_handler_name);
				continue;
			}

			if (MC_RUN_STRING_EQUAL (t_arg, "--load") ||
			    MC_RUN_STRING_EQUAL (t_arg, "-l"))
			{
				if (NULL == t_argopt)
					MCRunBadOptionArgError (t_arg, t_argopt);

				++t_arg_idx; /* Consume option argument */

				/* Convert argument to filename */
				MCAutoStringRef t_load_path;
				if (!MCRunParseCommandLineFilename (t_argopt, &t_load_path))
					return false;

				/* Add to load list */
				if (!MCProperListPushElementOntoFront (x_config.m_load_filenames,
				                                       *t_load_path))
					return false;
				continue;
			}

			if (MC_RUN_STRING_EQUAL (t_arg, "--list-handlers"))
			{
				x_config.m_list_handlers = true;
				continue;
			}

			if (MC_RUN_STRING_EQUAL (t_arg, "--"))
			{
				/* No more options */
				t_accept_options = false;
				continue;
			}

			if (MCStringBeginsWithCString (t_arg, (const char_t *) "-",
			                               kMCStringOptionCompareExact))
			{
				/* Don't accept any unrecognised options */
				MCRunBadOptionError (t_arg);
			}
		}

		/* If we haven't got a filename yet, this argument must be it.
		 * Otherwise, queue any remaining arguments to pass as "the
		 * command arguments". */
		if (!t_have_filename)
		{
			if (!MCRunParseCommandLineFilename (t_arg, &t_filename))
				return false;

			t_have_filename = true;
			t_accept_options = false;
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

/* lc-run permits loading of "composite module files", which are made
 * by concatenating multiple .lcm modules together.  It does this by
 * just repeatedly trying to load modules from a file input stream
 * until there's no data left in it.  In a composite module file, the
 * first module in the file is treated as the "main" module. */

/* Loads modules from p_filename, appending them to x_modules and
 * returning true iff all modules loaded successfully and at least one
 * module was loaded. */
static bool
MCRunLoadModulesFromFile (MCStringRef p_filename,
                          MCAutoScriptModuleRefArray & x_modules)
{
	MCAutoDataRef t_module_data;
	MCAutoValueRefBase<MCStreamRef> t_stream;

	if (!MCSFileGetContents (p_filename, &t_module_data))
		return false;

	return MCScriptCreateModulesFromData(*t_module_data, x_modules);
}

static bool
MCRunLoadModules (MCStringRef p_filename,
                  MCProperListRef p_load_filenames,
                  MCScriptModuleRef & r_module)
{
	/* Load main module file, keeping main module */
	MCAutoScriptModuleRefArray t_modules;
	if (!MCRunLoadModulesFromFile (p_filename, t_modules))
		return false;

	MCAutoScriptModuleRef t_module(t_modules[0]);

	/* Load other modules */
	uindex_t t_num_load = MCProperListGetLength (p_load_filenames);

	for (uindex_t i = 0; i < t_num_load; ++i)
	{
		MCValueRef t_element;
		t_element = MCProperListFetchElementAtIndex (p_load_filenames, i);
		if (!MCRunLoadModulesFromFile (MCStringRef(t_element), t_modules))
			return false;
	}

	/* Check main module is usable */
	if (!MCScriptEnsureModuleIsUsable (*t_module))
		return false;

	r_module = MCScriptRetainModule (*t_module);
	return true;
}

/* List all handlers in p_module that are public and take no
 * arguments. */
static bool
MCRunListHandlers (MCScriptModuleRef p_module)
{
	MCAutoProperListRef t_handler_list; /* List of MCNameRef */
	MCAutoStringRef t_message;

	if (!MCScriptListHandlerNamesOfModule (p_module, &t_handler_list))
		return false;

	if (!MCStringMutableCopy (kMCEmptyString, &t_message))
		return false;

	MCValueRef t_handler_val;
	uintptr_t t_handler_iter = 0;
	while (MCProperListIterate (*t_handler_list, t_handler_iter, t_handler_val))
	{
		MCNameRef t_handler_name;
		MCTypeInfoRef t_signature;
		MCAssert (MCTypeInfoConforms (MCValueGetTypeInfo (t_handler_val),
		                              kMCNameTypeInfo));
		t_handler_name = (MCNameRef) t_handler_val;

		if (!MCScriptQueryHandlerSignatureOfModule (p_module, t_handler_name,
		                                            t_signature))
			return false;

		/* Only accept handlers with arity 0 */
		if (0 != MCHandlerTypeInfoGetParameterCount (t_signature))
			continue;

		if (!MCStringAppendFormat (*t_message, "%@\n", t_handler_name))
			return false;
	}

	MCRunPrintMessage (stdout, *t_message);
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
	t_config.m_handler = MCValueRetain (MCNAME("main"));
	t_config.m_list_handlers = false;
	if (!MCProperListCreateMutable (t_config.m_load_filenames))
		MCRunStartupError(MCSTR("Initialization"));

	/* ---------- Process command-line arguments */
	if (!MCRunParseCommandLine (argc, argv, t_config))
		MCRunStartupError(MCSTR("Command Line"));

	/* ---------- Start VM */
	MCAutoScriptModuleRef t_module;
	MCAutoScriptInstanceRef t_instance;
	MCAutoValueRef t_ignored_retval;

	if (!MCRunLoadModules (t_config.m_filename,
	                       t_config.m_load_filenames,
	                       &t_module))
		MCRunStartupError (MCSTR("Load Modules"));

	if (t_config.m_list_handlers)
	{
		if (!MCRunListHandlers (*t_module))
			MCRunHandlerError();
	}
	else
	{
		if (!MCScriptCreateInstanceOfModule (*t_module, &t_instance))
			MCRunStartupError(MCSTR("Create Instance"));

		if (!MCScriptCallHandlerInInstance(*t_instance,
		                                   t_config.m_handler,
		                                   NULL, 0,
		                                   &t_ignored_retval))
			MCRunHandlerError();
	}

	MCScriptFinalize();
	MCSFinalize();
	MCFinalize();

	exit (0);
}

/* ----------------------------------------------------------------
 * Dummy module finit/inits - no canvas, engine, widget module
 * ---------------------------------------------------------------- */

extern "C" bool com_livecode_engine_Initialize(void) { return true; }
extern "C" void com_livecode_engine_Finalize(void) { }
extern "C" bool com_livecode_widget_Initialize(void) { return true; }
extern "C" void com_livecode_widget_Finalize(void) { }
extern "C" bool com_livecode_canvas_Initialize(void) { return true; }
extern "C" void com_livecode_canvas_Finalize(void) { }
