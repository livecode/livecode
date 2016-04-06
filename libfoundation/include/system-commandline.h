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

#if !defined(__MCS_SYSTEM_H_INSIDE__)
#	error "Only <foundation-system.h> can be included directly"
#endif

/* ================================================================
 * Command-line handling
 * ================================================================ */

/* The MCSCommandLine library provides an API for handling
 * command-line arguments and executable location information in a
 * standardised fashion.
 *
 * There are three concepts dealt with by the library:
 *
 * 1. The command line arguments.  These are the arguments passed to
 *    the command line program, and correspond roughly to elements 1+
 *    of the standard C argv[] array.  Normally, wrappers (e.g. the
 *    engine or LiveCode builder VM) should consume and remove any
 *    wrapper-specific arguments.
 *
 * 2. The command name.  This corresponds to element 0 of the standard
 *    C argv[] array.  This should normally be set to the name of the
 *    LiveCode program being run, as it appears on the command line.
 *
 * 3. The executable file.  This is the canonical filename of the
 *    program being run, and should normally set to the canonical
 *    filename of the LiveCode program (in LiveCode filename format).
 *
 * Calling MCSCommandLineCapture() will attempt to initialize each of
 * the three values.  There are getters and setters to modify each to
 * suit the exact runtime situation.  For example:
 *
 * 1. Using lc-run to run a builder bytecode program:
 *       $ lc-run foo.lcm bar baz
 *           Arguments: bar baz
 *                Name: foo.lcm
 *          Executable: /path/to/foo.lcm
 *
 * 2. Running a builder program compiled to native code:
 *       $ foo.lcm.native bar baz
 *           Arguments: bar baz
 *                Name: foo.lcm.native
 *          Executable: /path/to/foo.lcm.native
 *
 * 3. Running the LiveCode IDE:
 *       $ LiveCode-Community bar baz
 *           Arguments: <always empty>
 *                Name: LiveCode-Community
 *          Executable: /path/to/LiveCode-Community
 *
 * 4. Running a standalone:
 *       $ LiveCode-Community bar baz
 *           Arguments: <always empty>
 *                Name: LiveCode-Community
 *          Executable: /path/to/LiveCode-Community
 *
 * 5. Running a shebang (!#) script:
 *       $ foo.lc bar baz
 *           Arguments: bar baz
 *                Name: foo.lc
 *          Executable: /path/to/foo.lc
 */

/* FIXME support for getting the command filename isn't implemented
 * yet */

/* ----------------------------------------------------------------
 * Capture command-line to set initial values
 * ---------------------------------------------------------------- */

/* Capture a C-style argument list. */
MC_DLLEXPORT bool MCSCommandLineCapture (uindex_t p_arg_count, const char* p_arg_array[]);

#if defined(__WINDOWS__)
/* Capture an argument list using the Windows API.  You should use
 * this instead of MCSCommandLineCapture() on Windows because the
 * command line is passed to the C main() function using the system
 * codepage, which means that Unicode characters may be converted to
 * '?' by Windows. */
MC_DLLEXPORT bool MCSCommandLineCaptureWindows (void);
#endif /* __WINDOWS__ */

/* ----------------------------------------------------------------
 * Arguments
 * ---------------------------------------------------------------- */

/* Set the argument list directly */
MC_DLLEXPORT bool MCSCommandLineSetArguments (MCProperListRef p_arg_list);

/* Get the argument list */
MC_DLLEXPORT bool MCSCommandLineGetArguments (MCProperListRef & r_arg_list);

/* ----------------------------------------------------------------
 * Name
 * ---------------------------------------------------------------- */

/* Set the command name directly */
MC_DLLEXPORT bool MCSCommandLineSetName (MCStringRef p_name);

/* Get the command name */
MC_DLLEXPORT bool MCSCommandLineGetName (MCStringRef & r_name);

/* ----------------------------------------------------------------
 * Filename
 * ---------------------------------------------------------------- */

/* Set the command filename directly */
MC_DLLEXPORT bool MCSCommandLineSetFilename (MCStringRef p_filename);

/* Get the command filename */
MC_DLLEXPORT bool MCSCommandLineGetFilename (MCStringRef & r_filename);

/* ----------------------------------------------------------------
 * Initialization
 * ---------------------------------------------------------------- */

#ifdef __MCS_INTERNAL_API__
bool __MCSCommandLineInitialize (void);
void __MCSCommandLineFinalize (void);
#endif
