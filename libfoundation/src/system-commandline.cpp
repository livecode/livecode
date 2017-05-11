/*                                                                     -*-c++-*-
Copyright (C) 2015 LiveCode Ltd.

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

#include "system-private.h"

#include <foundation-auto.h>

#if defined(__WINDOWS__)
#	include <windows.h>
#endif

/* ---------------------------------------------------------------- */

/* The current command-line arguments. */
MCProperListRef s_arguments = NULL;

/* The current command name. */
MCStringRef s_name = NULL;

/* The current command filename. */
MCStringRef s_filename = NULL;

/* ================================================================
 * File-local declarations
 * ================================================================ */

#if defined(__WINDOWS__)

/* Windows-specific functions for getting the command line arguments
 * encoded as UTF-16 rather than using the system codepage. */
static bool __MCSWindowsCommandLineGet (uindex_t &, unichar_t **&);
static void __MCSWindowsCommandLineFree (uindex_t &, unichar_t **& );

#endif /* __WINDOWS__ */

/* ================================================================
 * Setters and getters
 * ================================================================ */

MC_DLLEXPORT_DEF bool
MCSCommandLineGetArguments (MCProperListRef & r_arg_list)
{
	if (NULL != s_arguments)
		return MCProperListCopy (s_arguments, r_arg_list);

	r_arg_list = MCValueRetain (kMCEmptyProperList);
	return true;
}

MC_DLLEXPORT_DEF bool
MCSCommandLineSetArguments (MCProperListRef p_arg_list)
{
	MCAssert (NULL != p_arg_list);
	MCValueRelease (s_arguments);
	s_arguments = MCValueRetain (p_arg_list);
	return true;
}

MC_DLLEXPORT_DEF bool
MCSCommandLineGetName (MCStringRef & r_name)
{
	if (NULL != s_name)
		return MCStringCopy (s_name, r_name);

	r_name = MCValueRetain (kMCEmptyString);
	return true;
}

MC_DLLEXPORT_DEF bool
MCSCommandLineSetName (MCStringRef p_name)
{
	MCAssert (NULL != p_name);
	MCValueRelease (s_name);
	s_name = MCValueRetain (p_name);
	return true;
}

MC_DLLEXPORT_DEF bool
MCSCommandLineGetFilename (MCStringRef & r_filename)
{
	if (NULL != s_filename)
		return MCStringCopy (s_filename, r_filename);

	r_filename = MCValueRetain (kMCEmptyString);
	return true;
}

MC_DLLEXPORT_DEF bool
MCSCommandLineSetFilename (MCStringRef p_filename)
{
	MCAssert (NULL != p_filename);
	MCValueRelease (s_filename);
	s_filename = MCValueRetain (p_filename);
	return true;
}

/* ================================================================
 * Set up initial values by capturing command line
 * ================================================================ */

/* These __MCSCommandLineStringCreate() functions are used to decode
 * the command line arguments into LiveCode native strings correctly
 * for the platform.  The assumptions are:
 *
 * - Windows: UTF-16
 * - Linux: locale-dependent
 * - All other platforms: UTF-8
 */
static inline bool
__MCSCommandLineStringCreate(const char *in, MCStringRef &out)
{
#if defined(__LINUX__)
	return MCStringCreateWithSysString (in, out);
#else
	return MCStringCreateWithBytes ((const byte_t *)in, strlen(in), kMCStringEncodingUTF8,
	                                false, out);
#endif
}

static inline bool
__MCSCommandLineStringCreate(const unichar_t *in, MCStringRef &out)
{
	return MCStringCreateWithWString (in, out);
}

/* The bulk of the command-line capture code is identical for all
 * platforms: iterate over an array of nul-terminated strings and
 * convert them into a proper list, differing only in the array type.
 * This template is used to allow maximum commonality between
 * platforms. */
template<typename T>
static bool
__MCSCommandLineCaptureNameAndArguments (uindex_t p_arg_count,
                                         const T p_args[])
{
	/* ---------- Command name */
	MCAutoStringRef t_name;
	if (0 < p_arg_count)
	{
		if (!__MCSCommandLineStringCreate (p_args[0], &t_name))
			return false;
	}
	else
	{
		t_name = kMCEmptyString;
	}

	if (!MCSCommandLineSetName (*t_name))
		return false;

	/* ---------- Command arguments */
	MCAutoProperListRef t_arg_list;
	if (1 < p_arg_count)
	{
		const T *t_args_nonname = p_args + 1;
		uindex_t t_arg_count_nonname = p_arg_count - 1;

		MCAutoStringRefArray t_arg_strings;
		if (!t_arg_strings.New (t_arg_count_nonname))
			return false;

		for (uindex_t i = 0; i < t_arg_count_nonname; ++i)
		{
			if (!__MCSCommandLineStringCreate (t_args_nonname[i],
			                                   t_arg_strings[i]))
				return false;
		}

		if (!t_arg_strings.TakeAsProperList (&t_arg_list))
			return false;
	}
	else
	{
		t_arg_list = kMCEmptyProperList;
	}

	if (!MCSCommandLineSetArguments (*t_arg_list))
		return false;

	return true;
}

/* ----------------------------------------------------------------
 * Capture command information using C argument array
 * ---------------------------------------------------------------- */

MC_DLLEXPORT_DEF bool
MCSCommandLineCapture (uindex_t p_arg_count, const char *p_arg_array[])
{
	return
		__MCSCommandLineCaptureNameAndArguments<>(p_arg_count, p_arg_array);
}

/* ================================================================
 * Initialization
 * ================================================================ */

bool
__MCSCommandLineInitialize (void)
{
	s_arguments = NULL;
	s_name = NULL;
	return true;
}

void
__MCSCommandLineFinalize (void)
{
	if (NULL != s_arguments)
	{
		MCValueRelease (s_arguments);
		s_arguments = NULL;
	}
	if (NULL != s_name)
	{
		MCValueRelease (s_name);
		s_name = NULL;
	}
}

#if defined(__WINDOWS__)
/* ================================================================
 * Windows-specific functions
 * ================================================================ */

/* This is needed for CommandLineToArgvW */
#pragma comment(lib, "shell32.lib")

MC_DLLEXPORT_DEF bool
MCSCommandLineCaptureWindows (void)
{
	bool t_success = true;
	unichar_t **t_arg_array_w32 = NULL;
	uindex_t t_arg_count_w32;

	/* ---------- Get command name and arguments */
	if (t_success)
		t_success = __MCSWindowsCommandLineGet (t_arg_count_w32,
		                                        t_arg_array_w32);

	if (t_success)
		t_success = __MCSCommandLineCaptureNameAndArguments<>(t_arg_count_w32,
		                                                      t_arg_array_w32);

	__MCSWindowsCommandLineFree (t_arg_count_w32, t_arg_array_w32);

	return t_success;
}

/* ----------------------------------------------------------------
 * Get Unicode argument array on Windows
 * ---------------------------------------------------------------- */

static bool
__MCSWindowsCommandLineGet (uindex_t & r_argc,
                            unichar_t **& r_argv)
{
	LPCWSTR t_args_w32;
	int t_arg_count_w32;
	LPWSTR *t_arg_array_w32;

	t_args_w32 = GetCommandLineW ();
	t_arg_array_w32 = CommandLineToArgvW (t_args_w32, &t_arg_count_w32);
	if (NULL == t_arg_array_w32)
		return false; /* FIXME proper error */

	r_argc = t_arg_count_w32;
	r_argv = t_arg_array_w32;
	return true;
}

static void
__MCSWindowsCommandLineFree (uindex_t& p_argc,
                             unichar_t **&p_argv)
{
	if (p_argv != NULL)
		LocalFree (p_argv);
}

#endif /* __WINDOWS__ */
