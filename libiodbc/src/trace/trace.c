/*
 *  trace.c
 *
 *  $Id: trace.c,v 1.18 2006/01/24 00:09:25 source Exp $
 *
 *  Trace functions
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2006 by OpenLink Software <iodbc@openlinksw.com>
 *  All Rights Reserved.
 *
 *  This software is released under the terms of either of the following
 *  licenses:
 *
 *      - GNU Library General Public License (see LICENSE.LGPL)
 *      - The BSD License (see LICENSE.BSD).
 *
 *  Note that the only valid version of the LGPL license as far as this
 *  project is concerned is the original GNU Library General Public License
 *  Version 2, dated June 1991.
 *
 *  While not mandated by the BSD license, any patches you make to the
 *  iODBC source code may be contributed back into the iODBC project
 *  at your discretion. Contributions will benefit the Open Source and
 *  Data Access community as a whole. Submissions may be made at:
 *
 *      http://www.iodbc.org
 *
 *
 *  GNU Library Generic Public License Version 2
 *  ============================================
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; only
 *  Version 2 of the License dated June 1991.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  The BSD License
 *  ===============
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. Neither the name of OpenLink Software Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OPENLINK OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>

#include <time.h>
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>

#include "herr.h"
#include "henv.h"
#include "ithread.h"
#include "trace.h"
#include "unicode.h"

#define NO_CARBON 1
#if defined(macintosh)
# include       <Errors.h>
# include       <OSUtils.h>
# include       <Processes.h>
#elif defined(__APPLE__) && !defined(NO_CARBON)
# include       <Carbon/Carbon.h>
#endif


/*
 *  Global trace flag
 */
int ODBCSharedTraceFlag = SQL_OPT_TRACE_OFF;

static char *trace_appname = NULL;
static char *trace_fname = NULL;
static FILE *trace_fp = NULL;
static int   trace_fp_close = 0;

void trace_emit (char *fmt, ...);

#ifdef HAVE_GETTIMEOFDAY
static struct timeval starttime = {0};
#endif


/*
 * Internal functions
 */
void
trace_set_appname (char *appname)
{
  MEM_FREE (trace_appname);
  trace_appname = STRDUP (appname);
}


char *
trace_get_filename (void)
{
  return STRDUP (trace_fname);
}


void
trace_set_filename (char *fname)
{
  char *s, *p;
  struct passwd *pwd;
  char *buf;
  size_t buf_len, buf_pos;
  char tmp[255];

  /*  Initialize */
  MEM_FREE (trace_fname);
  trace_fname = NULL;
  buf = (char *) malloc (buf_len = strlen (fname) + sizeof (tmp) + 1);
  if (!buf)
    return;			/* No more memory */
  buf_pos = 0;
  buf[0] = '\0';

  for (s = fname; *s;)
    {
      /*
       *  Make sure we can fit at least 1 more tmp buffer inside
       */
      if (buf_len - buf_pos < sizeof (tmp))
	buf = realloc (buf, buf_len += sizeof (tmp) + 1);
      if (!buf)
	return;			/* No more memory */

      if (*s != '$')
	{
	  buf[buf_pos++] = *s++;
	}
      else
	{
	  /* Handle Escape sequences */
	  switch (*(s + 1))
	    {
	    case '$':
	      {
		buf[buf_pos++] = '$';
		break;
	      }

	    case 'p':
	    case 'P':
	      {
#if defined (HAVE_SNPRINTF)
		snprintf (tmp, sizeof (tmp), "%ld", (long) getpid ());
#else
		sprintf (tmp, "%ld", (long) getpid ());
#endif
		strcpy (&buf[buf_pos], tmp);
		buf_pos += strlen (tmp);
		break;
	      }

	    case 'u':
	    case 'U':
	      {
		if ((pwd = getpwuid (getuid ())) != NULL)
		  {
#if defined (HAVE_SNPRINTF)
		    snprintf (tmp, sizeof (tmp), "%s", pwd->pw_name);
#else
		    sprintf (tmp, "%s", pwd->pw_name);
#endif
		    strcpy (&buf[buf_pos], tmp);
		    buf_pos += strlen (tmp);
		  }
		break;
	      }

	    case 'h':
	    case 'H':
	      {
		p = NULL;
		if ((p = getenv ("HOME")) == NULL)
		  {
		    if ((pwd = getpwuid (getuid ())) != NULL)
		      p = pwd->pw_dir;
		  }

		if (p)
		  {
#if defined (HAVE_SNPRINTF)
		    snprintf (tmp, sizeof (tmp), "%s", p);
#else
		    sprintf (tmp, "%s", p);
#endif
		    strcpy (&buf[buf_pos], tmp);
		    buf_pos += strlen (tmp);
		  }
		break;
	      }

	    case 't':
	    case 'T':
	      {
		time_t now;
		struct tm *timeNow;

		tzset ();
		time (&now);
		timeNow = localtime (&now);

		strftime (tmp, sizeof (tmp), "%Y%m%d-%H%M%S", timeNow);
		strcpy (&buf[buf_pos], tmp);
		buf_pos += strlen (tmp);
		break;
	      }

	    default:
	      /* Skip unknown escapes */
	      break;
	    }
	  s += 2;
	}
    }

  buf[buf_pos] = '\0';
  trace_fname = buf;

  return;
}


void
trace_start(void)
{
  /*
   *  First stop any previous trace
   */
  trace_stop ();

#ifdef HAVE_GETTIMEOFDAY
  gettimeofday (&starttime, NULL);
#endif

  /*
   *  If no trace filename is specified, use the default
   */
  if (!trace_fname)
    trace_fname = STRDUP (SQL_OPT_TRACE_FILE_DEFAULT);

#if defined (stderr)
  else if (STRCASEEQ (trace_fname, "stderr"))
    {
      trace_fp = stderr;
    }
#endif

  else
    {
      int fd;
      int fd_flags = O_WRONLY | O_CREAT | O_TRUNC;
      int fd_mode = 0644;


#if defined (unix)
      /*
       *  As this is a security risk, we do not allow root to overwrite a file
       */
      if (geteuid () == 0)
	{
	  fd_flags |= O_EXCL;
	}
#endif

      fd = open (trace_fname, fd_flags, fd_mode);
      if (fd < 0 || (trace_fp = fdopen (fd, "w")) == NULL)
	{
	  return;		/* no tracing */
	}

      trace_fp_close = 1;

      /*
       *  Set trace stream to line buffered
       */
      setvbuf (trace_fp, NULL, _IOLBF, 0);
    }

  /*
   *  Initialize the debug stream
   */
  if (trace_fp == NULL)
    {
      return;
    }
  else
    {
      char mesgBuf[200];
      time_t now;
      struct tm *timeNow;

      trace_emit ("** iODBC Trace file\n");

      /*
       *  Show start time
       */
      tzset ();
      time (&now);
      timeNow = localtime (&now);
      strftime (mesgBuf,
	  sizeof (mesgBuf), "** Trace started on %a %b %d %H:%M:%S %Y",
	  timeNow);
      trace_emit ("%s\n", mesgBuf);

      /*
       *  Show Driver Manager version similar to SQLGetInfo (SQL_DM_VER)
       */
      sprintf ((char *) mesgBuf, "%02d.%02d.%04d.%04d",
	  SQL_SPEC_MAJOR,
	  SQL_SPEC_MINOR, IODBC_BUILD / 10000, IODBC_BUILD % 10000);
      trace_emit ("** Driver Manager: %s\n\n", mesgBuf);
    }

#if defined (linux)
  {
    extern char *__progname;
    trace_set_appname (__progname);
  }
#elif defined(macintosh) || (defined(__APPLE__) && !defined(NO_CARBON))
  {
    ProcessSerialNumber PSN;
    ProcessInfoRec prec;
    unsigned char processName[40];

    GetCurrentProcess (&PSN);

    prec.processInfoLength = sizeof (ProcessInfoRec);
    prec.processName = processName;
    prec.processAppSpec = NULL;

    if (GetProcessInformation (&PSN, &prec) == noErr)
      {
	processName[processName[0] + 1] = '\0';
	trace_set_appname (processName + 1);
      }
    else
      trace_set_appname ("{No Application Name}");
  }
#elif defined(__APPLE__)
#ifdef MACOSX102
  {
    trace_set_appname ("{No Application Name}");
  }
#else
  {
    trace_set_appname ((char *) getprogname ());
  }
#endif
#endif

  /*
   *  Turn on tracing flag
   */
  ODBCSharedTraceFlag = SQL_OPT_TRACE_ON;

  return;
}


void
trace_stop(void)
{
  char mesgBuf[200];
  time_t now;
  struct tm *timeNow;

  if (trace_fp)
    {
      /*
       * Show end time
       */
      tzset ();
      time (&now);
      timeNow = localtime (&now);
      strftime (mesgBuf,
	  sizeof (mesgBuf), "** Trace finished on %a %b %d %H:%M:%S %Y",
	  timeNow);

      trace_emit ("\n%s\n", mesgBuf);

      if (trace_fp_close)
	fclose (trace_fp);
    }

  ODBCSharedTraceFlag = SQL_OPT_TRACE_OFF;
  trace_fp = NULL;
  trace_fp_close = 0;
}


void
trace_emitc (char c)
{
  /*
   * Make sure tracing is enabled
   */
  if (!trace_fp)
    {
      ODBCSharedTraceFlag = SQL_OPT_TRACE_OFF;
      return;
    }

  fputc (c, trace_fp);
}


void
trace_emit (char *fmt, ...)
{
  va_list ap;

  /*
   * Make sure tracing is enabled
   */
  if (!trace_fp)
    {
      ODBCSharedTraceFlag = SQL_OPT_TRACE_OFF;
      return;
    }

  va_start (ap, fmt);
  vfprintf (trace_fp, fmt, ap);
  va_end (ap);
}


void
trace_emit_string (SQLCHAR *str, int len, int is_utf8)
{
  ssize_t length = len;
  int i, j;
  long col;
  SQLCHAR *ptr;
  int bytes;
  int truncated = 0;

  if (!str)
    return;

  if (len == SQL_NTS)
    length = strlen ((char *) str);
  else if (len <= 0)
    return;

  /*
   *  Guard against very long strings
   */
  if (length > MAX_EMIT_STRING)
    {
      length = MAX_EMIT_STRING;
      truncated = 1;
    }

  /*
   *  Dump the (optional UTF-8) string in chunks of 40 characters
   */
  ptr = str;
  col = 0;
  for (i = 0; i < length; i += bytes)
    {
      register int c = *ptr;

      /*
       *   Assume this is a nul-terminated string
       */
      if (!c)
        break;

      /*
       *  Print prefix
       */
      if (col == 0)
	trace_emit ("\t\t\t\t  | ");

      /*
       *  Take care of UTF-8 encoding
       */
      if (!is_utf8)
	bytes = 1;
      else if (c < 128)
	bytes = 1;
      else if ((c & 0xE0) == 0xC0)
	bytes = 2;
      else if ((c & 0xF0) == 0xE0)
	bytes = 3;
      else if ((c & 0xF8) == 0xF0)
	bytes = 4;
      else if ((c & 0xFC) == 0xF8)
        bytes = 5;
      else if ((c & 0xFE) == 0xFC)
        bytes = 6;
      else
	bytes = -1;	/* Wrong UTF8 character */

      if (bytes > 0)
        {
	  /*
	   *  Emit the number of bytes calculated
	   */
	  for (j = 0; j < bytes; j++)
	    trace_emitc (*ptr++);
	}
      else
        {
	  /*
	   *  Skip this bogus UTF8 character sequence and emit a single #
	   */
	  for (bytes = 1, ptr++; (*ptr & 0xC0) == 0x80; bytes++)
	    ptr++;
	  trace_emitc ('#');
	}

      /*
       *  After 40 characters, start a new line
       */
      if (++col >= 40)
	{
	  trace_emit (" |\n");
	  col = 0;
	}
    }

  /*
   *  Pad the last part of the string with spaces
   */
  if (col > 0)
    {
      for (i = col; i < 40; i++)
	trace_emitc (' ');
      trace_emit (" |\n");
    }

  if (truncated)
    trace_emit ("\t\t\t\t  | %-40.40s |\n", "(truncated)");
}


void
trace_emit_binary (unsigned char *str, int len)
{
  long length = len;
  int i;
  long col;
  unsigned char *ptr;
  int truncated = 0;
  char buf[80];
  char *HEX = "0123456789ABCDEF";

  if (!str || len <= 0)
    return;

  /*
   *  Guard against very long binary buffers
   */
  if (length > MAX_EMIT_BINARY)
    {
      length = MAX_EMIT_BINARY;
      truncated = 1;
    }

  ptr = str;
  col = 0;
  memset (buf, ' ', sizeof (buf));
  buf[40] = '\0';
  for (i = 0; i < length; i++)
    {
      unsigned char c = *ptr++;

      /*
       *  Put data into buffer
       */
      buf[3 * col] = HEX[(c >> 4) & 0xF];
      buf[3 * col + 1] = HEX[c & 0xF];
      if (isprint (c))
	buf[30 + col] = c;
      else
	buf[30 + col] = '.';

      /*
       *  After 10 bytes, start a new line
       */
      if (++col > 9)
	{
	  trace_emit_string ((SQLCHAR *) buf, 40, 0);
	  col = 0;
	  memset (buf, ' ', sizeof (buf));
	}
    }

  /*
   *  Pad the last part of the string with spaces
   */
  if (col > 0)
    trace_emit_string ((SQLCHAR *) buf, 40, 0);

  if (truncated)
    trace_emit ("\t\t\t\t  | %-40.40s |\n", "(truncated)");
}


void
_trace_print_function (int func, int trace_leave, int retcode)
{
  extern char *odbcapi_symtab[];
  char *ptr = "invalid retcode";

  /*
   * Calculate timestamp
   */
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;

  gettimeofday (&tv, NULL);
  tv.tv_sec -= starttime.tv_sec;
  tv.tv_usec -= starttime.tv_usec;
  if (tv.tv_usec < 0)
    {
      tv.tv_sec--;
      tv.tv_usec += 1000000L;
    }
  trace_emit ("\n[%06ld.%06ld]\n", tv.tv_sec, tv.tv_usec);
#else
  trace_emit ("\n");
#endif

  switch (retcode)
    {
      _S (SQL_SUCCESS);
      _S (SQL_SUCCESS_WITH_INFO);
      _S (SQL_NO_DATA_FOUND);
      _S (SQL_NEED_DATA);
      _S (SQL_INVALID_HANDLE);
      _S (SQL_ERROR);
      _S (SQL_STILL_EXECUTING);
    }

#ifndef THREAD_IDENT
#define THREAD_IDENT 0UL
#endif

  if (trace_leave == TRACE_LEAVE)
    trace_emit ("%-15.15s %08lX EXIT  %s with return code %d (%s)\n",
	trace_appname ? trace_appname : "Application",
	THREAD_IDENT, odbcapi_symtab[func], retcode, ptr);
  else
    trace_emit ("%-15.15s %08lX ENTER %s\n",
	trace_appname ? trace_appname : "Application",
	THREAD_IDENT, odbcapi_symtab[func]);
}


static char *_trace_sym_handletype[] = {
  "SQLHANDLE",
  "SQLHENV",
  "SQLHDBC",
  "SQLHSTMT",
  "SQLDESC",
  "SQLSENV"
};


void
_trace_handletype (SQLSMALLINT type)
{
  char *ptr = "invalid handle type";
  switch (type)
    {
      _S (SQL_HANDLE_ENV);
      _S (SQL_HANDLE_DBC);
      _S (SQL_HANDLE_STMT);
#if ODBCVER >= 0x0300
      _S (SQL_HANDLE_DESC);
      _S (SQL_HANDLE_SENV);
#endif
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLSMALLINT", (int) type, ptr);
}


void
_trace_handle_p (SQLSMALLINT type, SQLHANDLE * handle, int output)
{
  if (!handle)
    trace_emit ("\t\t%-15.15s * 0x0 (%s)\n",
	_trace_sym_handletype[type], "SQL_NULL_HANDLE");
  else if (output)
    trace_emit ("\t\t%-15.15s * %p (%p)\n",
	_trace_sym_handletype[type], handle, *handle);
  else
    trace_emit ("\t\t%-15.15s * %p\n", _trace_sym_handletype[type], handle);
}


void
_trace_handle (SQLSMALLINT type, SQLHANDLE handle)
{
  if (!handle)
    trace_emit ("\t\t%-15.15s   0x0 (%s)\n",
	_trace_sym_handletype[type], "SQL_NULL_HANDLE");
  else
    trace_emit ("\t\t%-15.15s   %p\n", _trace_sym_handletype[type], handle);
}


/*
 *  Trace basic data types
 */
void
_trace_smallint (SQLSMALLINT i)
{
  trace_emit ("\t\t%-15.15s   %ld\n", "SQLSMALLINT", (long) i);
}


void
_trace_usmallint (SQLUSMALLINT i)
{
  trace_emit ("\t\t%-15.15s   %lu\n", "SQLUSMALLINT", (unsigned long) i);
}


void
_trace_integer (SQLINTEGER i)
{
  trace_emit ("\t\t%-15.15s   %ld\n", "SQLINTEGER", (long) i);
}


void
_trace_uinteger (SQLUINTEGER i)
{
  trace_emit ("\t\t%-15.15s   %lu\n", "SQLUINTEGER", (unsigned long) i);
}


void
_trace_pointer (SQLPOINTER p)
{
  if (!p)
    trace_emit ("\t\t%-15.15s   0x0\n", "SQLPOINTER");
  else
    trace_emit ("\t\t%-15.15s   %p\n", "SQLPOINTER", p);
}


void
_trace_smallint_p (SQLSMALLINT *p, int output)
{
  if (!p)
    trace_emit ("\t\t%-15.15s * 0x0\n", "SQLSMALLINT");
  else if (output)
    trace_emit ("\t\t%-15.15s * %p (%ld)\n", "SQLSMALLINT", p, (long) *p);
  else
    trace_emit ("\t\t%-15.15s * %p\n", "SQLSMALLINT", p);
}


void
_trace_usmallint_p (SQLUSMALLINT *p, int output)
{
  if (!p)
    trace_emit ("\t\t%-15.15s * 0x0\n", "SQLUSMALLINT");
  else if (output)
    trace_emit ("\t\t%-15.15s * %p (%lu)\n", "SQLUSMALLINT", p, (unsigned long) *p);
  else
    trace_emit ("\t\t%-15.15s * %p\n", "SQLUSMALLINT", p);
}


void
_trace_integer_p (SQLINTEGER *p, int output)
{
  if (!p)
    trace_emit ("\t\t%-15.15s * 0x0\n", "SQLINTEGER");
  else if (output)
    trace_emit ("\t\t%-15.15s * %p (%ld)\n", "SQLINTEGER", p, (long) *p);
  else
    trace_emit ("\t\t%-15.15s * %p\n", "SQLINTEGER", p);
}


void
_trace_uinteger_p (SQLUINTEGER *p, int output)
{
  if (!p)
    trace_emit ("\t\t%-15.15s * 0x0\n", "SQLUINTEGER");
  else if (output)
    trace_emit ("\t\t%-15.15s * %p (%lu)\n", "SQLUINTEGER", p, (unsigned long) *p);
  else
    trace_emit ("\t\t%-15.15s * %p\n", "SQLUINTEGER", p);
}


void
_trace_stringlen (char *type, SQLINTEGER len)
{
  if (len == SQL_NTS)
    trace_emit ("\t\t%-15.15s   %ld (SQL_NTS)\n", type, (long) len);
  else
    trace_emit ("\t\t%-15.15s   %ld\n", type, (long) len);
}

void
_trace_len (SQLLEN i)
{
#ifdef _WIN64
  trace_emit ("\t\t%-15.15s   %I64d\n", "SQLLEN", (INT64) i);
#else
  trace_emit ("\t\t%-15.15s   %ld\n", "SQLLEN", (long) i);
#endif
}


void
_trace_ulen (SQLULEN i)
{
#ifdef _WIN64
  trace_emit ("\t\t%-15.15s   %I64u\n", "SQLLEN", (UINT64) i);
#else
  trace_emit ("\t\t%-15.15s   %ld\n", "SQLLEN", (unsigned long) i);
#endif
}


void
_trace_len_p (SQLLEN *p, int output)
{
  if (!p)
    trace_emit ("\t\t%-15.15s * 0x0\n", "SQLLEN");
  else if (output)
#ifdef _WIN64
    trace_emit ("\t\t%-15.15s * %p (%I64d)\n", "SQLLEN", p, (INT64) *p);
#else
    trace_emit ("\t\t%-15.15s * %p (%ld)\n", "SQLLEN", p, (long) *p);
#endif
  else
    trace_emit ("\t\t%-15.15s * %p\n", "SQLLEN", p);
}


void
_trace_ulen_p (SQLULEN *p, int output)
{
  if (!p)
    trace_emit ("\t\t%-15.15s * 0x0\n", "SQLULEN");
  else if (output)
#ifdef _WIN64
    trace_emit ("\t\t%-15.15s * %p (%I64u)\n", "SQLULEN", p, (UINT64) *p);
#else
    trace_emit ("\t\t%-15.15s * %p (%lu)\n", "SQLULEN", p, (unsigned long) *p);
#endif
  else
    trace_emit ("\t\t%-15.15s * %p\n", "SQLULEN", p);
}


void
_trace_string (SQLCHAR * str, SQLSMALLINT len, SQLSMALLINT * lenptr, int output)
{
  long length;

  if (!str)
    {
      trace_emit ("\t\t%-15.15s * 0x0\n", "SQLCHAR");
      return;
    }

  trace_emit ("\t\t%-15.15s * %p\n", "SQLCHAR", str);

  if (!output)
    return;

  /*
   *  Calculate length of string
   */
  if (lenptr)
    length = *lenptr;
  else
    length = len;

  if (length == SQL_NTS)
    length = STRLEN (str);

  if (*str && length)
    trace_emit_string (str, length, 0);
  else
    trace_emit_string ( (SQLCHAR *) "(empty string)", SQL_NTS, 0);

  return;
}


void
_trace_string_w (SQLWCHAR * str, SQLSMALLINT len, SQLSMALLINT * lenptr,
    int output)
{
  long length;

  if (!str)
    {
      trace_emit ("\t\t%-15.15s * 0x0\n", "SQLWCHAR");
      return;
    }

  trace_emit ("\t\t%-15.15s * %p\n", "SQLWCHAR", str);

  if (!output)
    return;

  /*
   *  Calculate length of string
   */
  if (lenptr)
    length = *lenptr;
  else
    length = len;

  if (length == SQL_NTS)
    length = wcslen (str);

  /*
   *  Translate wide string into UTF-8 format and print it
   */
  if (*str && length)
    {
      SQLCHAR *str_u8 = dm_SQL_W2A (str, length);
      trace_emit_string (str_u8, SQL_NTS, 1);
      free (str_u8);
    }
  else
    trace_emit_string ((SQLCHAR *)"(empty string)", SQL_NTS, 0);

  return;
}


void
_trace_c_type (SQLSMALLINT type)
{
  char *ptr = "unknown C type";

  switch (type)
    {
      _S (SQL_C_BINARY);
      _S (SQL_C_BIT);
      /* _S (SQL_C_BOOKMARK); */
      _S (SQL_C_CHAR);
      _S (SQL_C_DATE);
      _S (SQL_C_DEFAULT);
      _S (SQL_C_DOUBLE);
      _S (SQL_C_FLOAT);
#if (ODBCVER >= 0x0350)
      _S (SQL_C_GUID);
#endif
#if ODBCVER >= 0x0300
      _S (SQL_C_INTERVAL_DAY);
      _S (SQL_C_INTERVAL_DAY_TO_HOUR);
      _S (SQL_C_INTERVAL_DAY_TO_MINUTE);
      _S (SQL_C_INTERVAL_DAY_TO_SECOND);
      _S (SQL_C_INTERVAL_HOUR);
      _S (SQL_C_INTERVAL_HOUR_TO_MINUTE);
      _S (SQL_C_INTERVAL_HOUR_TO_SECOND);
      _S (SQL_C_INTERVAL_MINUTE);
      _S (SQL_C_INTERVAL_MINUTE_TO_SECOND);
      _S (SQL_C_INTERVAL_MONTH);
      _S (SQL_C_INTERVAL_SECOND);
      _S (SQL_C_INTERVAL_YEAR);
      _S (SQL_C_INTERVAL_YEAR_TO_MONTH);
#endif
      _S (SQL_C_LONG);
#if ODBCVER >= 0x0300
      _S (SQL_C_NUMERIC);
      _S (SQL_C_SBIGINT);
#endif
      _S (SQL_C_SHORT);
      _S (SQL_C_SLONG);
      _S (SQL_C_SSHORT);
      _S (SQL_C_STINYINT);
      _S (SQL_C_TIME);
      _S (SQL_C_TIMESTAMP);
      _S (SQL_C_TINYINT);
#if ODBCVER >= 0x0300
      _S (SQL_C_TYPE_DATE);
      _S (SQL_C_TYPE_TIME);
      _S (SQL_C_TYPE_TIMESTAMP);
      _S (SQL_C_UBIGINT);
#endif
      _S (SQL_C_ULONG);
      _S (SQL_C_USHORT);
      _S (SQL_C_UTINYINT);
      /* _S (SQL_C_VARBOOKMARK); */
      _S (SQL_C_WCHAR);

#if ODBCVER >= 0x0300
      _S (SQL_ARD_TYPE);
#endif
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLSMALLINT ", type, ptr);
}


void
_trace_inouttype (SQLSMALLINT type)
{
  char *ptr = "unknown Input/Output type";

  switch (type)
    {
    	_S (SQL_PARAM_INPUT);
	_S (SQL_PARAM_OUTPUT);
	_S (SQL_PARAM_INPUT_OUTPUT);
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLSMALLINT ", type, ptr);
}


void
_trace_sql_type (SQLSMALLINT type)
{
  char *ptr = "unknown SQL type";

  switch (type)
    {
      _S (SQL_UNKNOWN_TYPE);
      _S (SQL_BIGINT);
      _S (SQL_BINARY);
      _S (SQL_BIT);
      _S (SQL_CHAR);
#if (ODBCVER < 0x0300)
      _S (SQL_DATE);
#else
      _S (SQL_DATETIME);
#endif
      _S (SQL_DECIMAL);
      _S (SQL_DOUBLE);
      _S (SQL_FLOAT);
#if (ODBCVER >= 0x0350)
      _S (SQL_GUID);
#endif
      _S (SQL_INTEGER);
      _S (SQL_LONGVARBINARY);
      _S (SQL_LONGVARCHAR);
      _S (SQL_NUMERIC);
      _S (SQL_REAL);
      _S (SQL_SMALLINT);
#if (ODBCVER < 0x0300)
      _S (SQL_TIME);
#else
      _S (SQL_INTERVAL);
#endif
      _S (SQL_TIMESTAMP);
      _S (SQL_TINYINT);
#if ODBCVER >= 0x0300
      _S (SQL_TYPE_DATE);
      _S (SQL_TYPE_TIME);
      _S (SQL_TYPE_TIMESTAMP);
#endif
      _S (SQL_VARBINARY);
      _S (SQL_VARCHAR);
      _S (SQL_WCHAR);
      _S (SQL_WLONGVARCHAR);
      _S (SQL_WVARCHAR);
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLSMALLINT", (int) type, ptr);
}


void
_trace_sql_type_p (SQLSMALLINT *p, int output)
{
  char *ptr = "unknown SQL type";

  if (!p)
    {
      trace_emit ("\t\t%-15.15s * 0x0\n", "SQLSMALLINT");
      return;
    }

  if (!output)
    {
      trace_emit ("\t\t%-15.15s * %p\n", "SQLSMALLINT", p);
      return;
    }

  switch (*p)
    {
      _S (SQL_UNKNOWN_TYPE);
      _S (SQL_BIGINT);
      _S (SQL_BINARY);
      _S (SQL_BIT);
      _S (SQL_CHAR);
#if (ODBCVER < 0x0300)
      _S (SQL_DATE);
#else
      _S (SQL_DATETIME);
#endif
      _S (SQL_DECIMAL);
      _S (SQL_DOUBLE);
      _S (SQL_FLOAT);

#if (ODBCVER >= 0x0350)
      _S (SQL_GUID);
#endif
      _S (SQL_INTEGER);
      _S (SQL_LONGVARBINARY);
      _S (SQL_LONGVARCHAR);
      _S (SQL_NUMERIC);
      _S (SQL_REAL);
      _S (SQL_SMALLINT);
#if (ODBCVER < 0x0300)
      _S (SQL_TIME);
#else
      _S (SQL_INTERVAL);
#endif
      _S (SQL_TIMESTAMP);
      _S (SQL_TINYINT);
#if ODBCVER >= 0x0300
      _S (SQL_TYPE_DATE);
      _S (SQL_TYPE_TIME);
      _S (SQL_TYPE_TIMESTAMP);
#endif
      _S (SQL_VARBINARY);
      _S (SQL_VARCHAR);
      _S (SQL_WCHAR);
      _S (SQL_WLONGVARCHAR);
      _S (SQL_WVARCHAR);
    }

  trace_emit ("\t\t%-15.15s * %p (%s)\n", "SQLSMALLINT", p, ptr);
}


#if ODBCVER >= 0x0300
void
_trace_sql_subtype (SQLSMALLINT *type, SQLSMALLINT *sub, int output)
{
  char *ptr = NULL;

  if (!type || !sub)
    {
      trace_emit ("\t\t%-15.15s * 0x0\n", "SQLSMALLINT");
      return;
    }

  if (!output)
    {
      trace_emit ("\t\t%-15.15s * %p\n", "SQLSMALLINT", sub);
      return;
    }

  if (*type == SQL_DATETIME)
    {
      switch (*sub)
	{
	  _S (SQL_CODE_DATE);
	  _S (SQL_CODE_TIME);
	  _S (SQL_CODE_TIMESTAMP);
	}
    }
  else if (*type == SQL_INTERVAL)
    {
      switch (*sub)
	{
	  _S (SQL_CODE_YEAR);
	  _S (SQL_CODE_MONTH);
	  _S (SQL_CODE_DAY);
	  _S (SQL_CODE_HOUR);
	  _S (SQL_CODE_MINUTE);
	  _S (SQL_CODE_SECOND);
	  _S (SQL_CODE_YEAR_TO_MONTH);
	  _S (SQL_CODE_DAY_TO_HOUR);
	  _S (SQL_CODE_DAY_TO_MINUTE);
	  _S (SQL_CODE_DAY_TO_SECOND);
	  _S (SQL_CODE_HOUR_TO_MINUTE);
	  _S (SQL_CODE_HOUR_TO_SECOND);
	  _S (SQL_CODE_MINUTE_TO_SECOND);
	}
    }

  if (ptr)
    trace_emit ("\t\t%-15.15s * %p (%s)\n", "SQLSMALLINT", sub, ptr);
  else
    trace_emit ("\t\t%-15.15s * %p (%d)\n", "SQLSMALLINT", sub, *sub);
}
#endif


void
_trace_bufferlen (SQLINTEGER length)
{
  char buf[255];
  char *ptr = NULL;

  switch (length)
   {
	_S (SQL_NTS);
#if ODBCVER >= 0x0300
	_S (SQL_IS_POINTER);
	_S (SQL_IS_UINTEGER);
	_S (SQL_IS_INTEGER);
	_S (SQL_IS_USMALLINT);
	_S (SQL_IS_SMALLINT);
#endif
   }

  /*
   *  Translate binary buffer length
   */
  if (length <= SQL_LEN_BINARY_ATTR_OFFSET)
    {
        sprintf (buf, "SQL_LEN_BINARY_ATTR(%ld)", (long) SQL_LEN_BINARY_ATTR(length));
	ptr = buf;
    }

  if (ptr)
    trace_emit ("\t\t%-15.15s * %ld (%s)\n", "SQLINTEGER", length, ptr);
  else
    trace_emit ("\t\t%-15.15s * %ld\n", "SQLINTEGER", (long) length);
}
