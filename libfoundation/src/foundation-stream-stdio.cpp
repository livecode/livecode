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

/* Force the use of 64-bit file streams API on Linux.  We need to
 * define this before including foundation.h because it pulls in
 * stdio.h, but this means that the platform identification macros
 * aren't available yet. */
#define _FILE_OFFSET_BITS 64

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"

#include <errno.h>

#ifdef __WINDOWS__
#  if defined(_CRT_DISABLE_PERFCRIT_LOCKS) && !defined(_DLL)
#    define fseeko _fseeki64_nolock
#  else
#    define fseeko _fseeki64
#  endif
#  define ftello _ftelli64
#endif

/*
 * This file provides an MCStream implementation that tightly wraps a
 * C standard library io stream (FILE *, defined in stdio.h).  It
 * provides two ways to obtain stdio streams:
 *
 * 1) Pass a FILE * to __MCStdioStreamCreate().  This is intended to
 *    be used within libfoundation (e.g. for creating file IO
 *    streams).
 *
 * 2) Use e.g. MCStreamGetStandardOutput() to obtain one of the three
 *    "standard" streams.
 *
 * FIXME known issues:
 *
 * 1) MCStreamIsReadable() returns true for write-only streams, and
 *    vice-versa.  This is because there's no way to tell whether any
 *    particular FILE* stream is readable or writable (or for that
 *    matter whether its what operations its underlying file handle
 *    supports) without trying it.  For example, stdout is bound to
 *    file descriptor 1 when the program starts up, but there's
 *    nothing that requires it to be writable (or even a valid file
 *    descriptor) other than convention.
 *
 *    It might be necessary to have a number of possible different
 *    sets of callbacks depending on the type of file descriptor being
 *    used (although when one doesn't *know* what type of file
 *    descriptor you've got, that would be a challenge).
 *
 * 2) Partial reads and partial writes are considered to be failures.
 *    This means that if you request 1 kB from a stream, and 900 B are
 *    available, those 900 B will be lost (irrecoverably so if reading
 *    from e.g. a TCP/IP stream).  This is probably a design flaw in
 *    the streams API.
 */

/* ================================================================
 * C stdio errors
 * ================================================================ */

static MCTypeInfoRef kMCStdioStreamIOErrorTypeInfo;
static MCTypeInfoRef kMCStdioStreamEndOfFileErrorTypeInfo;

static bool
__MCStdioStreamCreateNamedErrorType (MCNameRef p_name,
                                     MCStringRef p_message,
                                     MCTypeInfoRef & r_error_type)
{
	MCAutoTypeInfoRef t_raw_typeinfo, t_typeinfo;

	if (!MCErrorTypeInfoCreate (MCNAME("stdio"), p_message, &t_raw_typeinfo))
		return false;

	if (!MCNamedTypeInfoCreate (p_name, &t_typeinfo))
		return false;

	if (!MCNamedTypeInfoBind (*t_typeinfo, *t_raw_typeinfo))
		return false;

	r_error_type = MCValueRetain (*t_typeinfo);
	return true;
}

static bool
__MCStdioStreamInitializeErrors (void)
{
	if (!__MCStdioStreamCreateNamedErrorType(
	        MCNAME("livecode.lang.StdioStreamIOError"),
	        MCSTR("Stream input/output error: %{description}"),
	        kMCStdioStreamIOErrorTypeInfo))
		return false;

	if (!__MCStdioStreamCreateNamedErrorType(
	         MCNAME("livecode.lang.StdioStreamEndOfFileError"),
	         MCSTR("Reached end of file while reading from stream"),
	         kMCStdioStreamEndOfFileErrorTypeInfo))
		return false;

	return true;
}

static void
__MCStdioStreamFinalizeErrors (void)
{
	MCValueRelease (kMCStdioStreamIOErrorTypeInfo);
	MCValueRelease (kMCStdioStreamEndOfFileErrorTypeInfo);
}

/* Create and throw an error that wraps the system error code p_errno.
 * If p_message is non-NULL, uses it instead of the default error
 * message format string. */
static bool
__MCStdioStreamThrowIOError (MCStringRef p_message,
                             int p_errno)
{
	MCAutoStringRef t_description;
	MCAutoNumberRef t_error_code;

	if (p_errno != 0)
	{
		if (!(MCStringCreateWithCString (strerror (p_errno),
		                                &t_description) &&
		      MCNumberCreateWithInteger (p_errno,
		                                 &t_error_code)))
			return false;
	}
	else
	{
		t_description = MCSTR("Unknown error");
		t_error_code = kMCZero;
	}

	if (NULL != p_message)
	{
		return MCErrorCreateAndThrowWithMessage (kMCStdioStreamIOErrorTypeInfo,
		                                         p_message,
		                                         "description", *t_description,
		                                         "error_code", *t_error_code,
		                                         NULL);
	}
	else
	{
		return MCErrorCreateAndThrow (kMCStdioStreamIOErrorTypeInfo,
		                              "description", *t_description,
		                              "error_code", *t_error_code,
		                              NULL);
	}
}

/* Create and throw an end-of-file error. */
static bool
__MCStdioStreamThrowEOFError (void)
{
	return MCErrorCreateAndThrow (kMCStdioStreamEndOfFileErrorTypeInfo, NULL);
}

/* ================================================================
 * C stdio stream structure and accessors
 * ================================================================ */

/* Forward declaration for the vtable */
extern MCStreamCallbacks __kMCStdioStreamCallbacks;

struct __MCStdioStream
{
	FILE *m_cstream;
};

/* Helper function for obtaining the __MCStdioStream pointer from a
 * MCStreamRef.  Also does some validation that the stream pointer is
 * valid. */
static __MCStdioStream *
__MCStdioStreamFromStreamRef (MCStreamRef p_stream)
{
	MCAssert (p_stream);
	MCAssert (&__kMCStdioStreamCallbacks == MCStreamGetCallbacks (p_stream));

	return (__MCStdioStream *) MCStreamGetExtraBytesPtr (p_stream);
}

/* Helper function for obtaining the FILE pointer from an
 * MCStreamRef. */
static FILE *
__MCStdioStreamGetCStream (MCStreamRef p_stream)
{
	return __MCStdioStreamFromStreamRef (p_stream)->m_cstream;
}

/* ================================================================
 * Stream implementation callback functions
 * ================================================================ */

/* The C library specification requires a file positioning function
 * (even if it's a no-op) to interleave when switching from reading a
 * file stream to writing it.  We ignore a possible error from
 * fseek(3) here, because it doesn't make any difference whether it
 * succeeds or not. */
static bool
__MCStdioStreamInterleave (MCStreamRef p_stream)
{
	/* UNCHECKED */ fseek (__MCStdioStreamGetCStream (p_stream), 0, SEEK_END);
	return true;
}

static void
__MCStdioStreamDestroy (MCStreamRef p_stream)
{
	__MCStdioStream *self = __MCStdioStreamFromStreamRef (p_stream);

	/* Note that there's no point in checking for errors in fclose(); if it
	 * fails, there's no sane recovery that can be performed. */
	if (self->m_cstream != NULL)
		/* UNCHECKED */ fclose (self->m_cstream);

	self->m_cstream = NULL;
}

/* Read p_amount bytes from p_stream into x_buffer.  If less than
 * p_amount bytes are available, fail, discarding any bytes that were
 * successfully read.
 *
 * FIXME see the "known issues" at the top of this file.
 */
static bool
__MCStdioStreamRead (MCStreamRef p_stream,
                    void *x_buffer,
                    size_t p_amount)
{
	bool t_success = true;
	FILE *t_stream = __MCStdioStreamGetCStream (p_stream);

	if (!__MCStdioStreamInterleave (p_stream))
		return false;

	byte_t *t_buffer;
	size_t t_total_read = 0;

	if (t_success)
		t_success = MCMemoryNewArray (p_amount, t_buffer);

	errno = 0;
	while (t_success)
	{
		size_t t_read_len;
		int t_save_errno;

		/* Attempt to read the remaining required length */
		t_read_len = fread (t_buffer + t_total_read,
		                    1, /* amount is in bytes */
		                    p_amount - t_total_read,
		                    t_stream);
		t_save_errno = errno;
		t_total_read += t_read_len;

		if (t_total_read >= p_amount)
			break;

		/* Check for EOF */
		if (feof (t_stream))
			t_success = __MCStdioStreamThrowEOFError();

		/* Check for error */
		if (ferror (t_stream) && errno != EINTR)
		{
			clearerr (t_stream);
			t_success = __MCStdioStreamThrowIOError (MCSTR("Failed to read from stream: %{description}"), t_save_errno);
		}
	}

	/* On success, copy the data that was read into place */
	if (t_success)
		MCMemoryCopy (x_buffer, t_buffer, p_amount);

	MCMemoryDeleteArray (t_buffer);
	return t_success;
}

/* Write p_amount bytes to p_stream from p_buffer.  If less than
 * p_amount bytes are successfully written, fail.
 *
 * FIXME see the "known issues" at the top of this file.
 */
static bool
__MCStdioStreamWrite (MCStreamRef p_stream,
                     const void *p_buffer,
                     size_t p_amount)
{
	bool t_success = true;
	FILE *t_stream = __MCStdioStreamGetCStream (p_stream);

	if (!__MCStdioStreamInterleave (p_stream))
		return false;

	size_t t_total_written = 0;

	errno = 0;
	while (t_success)
	{
		size_t t_written_len;
		int t_save_errno;

		/* Attempt to write the remaining required length */
		t_written_len = fwrite ((byte_t *) p_buffer + t_total_written,
		                        1, /* Amount is in bytes */
		                        p_amount - t_total_written,
		                        t_stream);
		t_total_written += t_written_len;
		t_save_errno = errno;

		if (t_total_written >= p_amount)
			break;

		/* Check for error */
		if (ferror (t_stream) && errno != EINTR)
		{
			clearerr (t_stream);

			t_success = __MCStdioStreamThrowIOError (MCSTR("Failed to write to stream: %{description}"), t_save_errno);
		}
	}

	return t_success;
}


static bool
__MCStdioStreamIsFinished (MCStreamRef p_stream,
                          bool & r_finished)
{
	r_finished = feof (__MCStdioStreamGetCStream (p_stream));
	return true;
}

static bool
__MCStdioStreamSeek (MCStreamRef p_stream,
                    filepos_t p_position)
{
	FILE *t_stream = __MCStdioStreamGetCStream (p_stream);

	errno = 0;
	int status = fseeko (t_stream, p_position, SEEK_SET);

	if (status != 0)
		return __MCStdioStreamThrowIOError (MCSTR("Failed to seek in stream: %{description}"), errno);

	return true;
}

static bool
__MCStdioStreamTell (MCStreamRef p_stream,
                    filepos_t & r_position)
{
	FILE *t_stream = __MCStdioStreamGetCStream (p_stream);

	errno = 0;
	filepos_t t_offset = ftello (t_stream);

	if (t_offset == -1)
		return __MCStdioStreamThrowIOError (MCSTR("Failed to get position in stream: %{description}"), errno);

	r_position = t_offset;
	return true;
}

MCStreamCallbacks __kMCStdioStreamCallbacks = {
    __MCStdioStreamDestroy,
    __MCStdioStreamIsFinished,
    nil,
    __MCStdioStreamRead,
    nil,
    __MCStdioStreamWrite,
    nil,
    nil,
    nil,
    __MCStdioStreamTell,
    __MCStdioStreamSeek,
};

/* ================================================================
 * Stdio stream creation
 * ================================================================ */

bool
__MCStdioStreamCreate (FILE *p_cstream,
                       MCStreamRef & r_stream)
{
	MCStreamRef t_stream;
	if (!MCStreamCreate (&__kMCStdioStreamCallbacks,
	                     sizeof (__MCStdioStream),
	                     t_stream))
		return false;

	__MCStdioStream *t_state = __MCStdioStreamFromStreamRef (t_stream);
	t_state->m_cstream = p_cstream;

	r_stream = t_stream;
	return true;
}

/* ================================================================
 * Standard streams
 * ================================================================ */

static MCStreamRef s_standard_output_stream;
static MCStreamRef s_standard_input_stream;
static MCStreamRef s_standard_error_stream;

static bool
__MCStreamGetStandardStream (FILE *p_cstream,
                             MCStreamRef & x_static_stream,
                             MCStreamRef & r_stream)
{
	/* Recreate the MCStreamRef if it hasn't been created yet or if it doesn't
	 * match the current C library value */
	if (NULL == x_static_stream ||
	    p_cstream != __MCStdioStreamGetCStream (x_static_stream))
	{
		MCValueRelease (x_static_stream);
		if (!__MCStdioStreamCreate (p_cstream, x_static_stream))
			return false;
	}

	r_stream = MCValueRetain (x_static_stream);
	return true;
}

bool
MCStreamGetStandardOutput (MCStreamRef & r_stream)
{
	return __MCStreamGetStandardStream (stdout,
	                                    s_standard_output_stream,
	                                    r_stream);
}

bool
MCStreamGetStandardInput (MCStreamRef & r_stream)
{
	return __MCStreamGetStandardStream (stdin,
	                                    s_standard_input_stream,
	                                    r_stream);
}

bool
MCStreamGetStandardError (MCStreamRef & r_stream)
{
	return __MCStreamGetStandardStream (stderr,
	                                    s_standard_error_stream,
	                                    r_stream);
}

/* ================================================================
 * Initialization
 * ================================================================ */

bool
__MCStdioStreamInitialize (void)
{
	s_standard_output_stream = NULL;
	s_standard_input_stream = NULL;
	s_standard_error_stream = NULL;

	/* Create error types */
	if (!__MCStdioStreamInitializeErrors())
		return false;

    return true;
}

void
__MCStdioStreamFinalize (void)
{
	MCValueRelease (s_standard_output_stream);
	MCValueRelease (s_standard_input_stream);
	MCValueRelease (s_standard_error_stream);

	__MCStdioStreamFinalizeErrors();
}
