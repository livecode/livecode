/*
 *  DriverConnect.c
 *
 *  $Id: DriverConnect.c,v 1.6 2006/01/20 15:58:35 source Exp $
 *
 *  SQLDriverConnect trace functions
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

#include "trace.h"


/*
 *  Never print plaintext passwords
 *
 *  NOTE: This function modifies the original string
 *  
 */
static void
_trace_connstr_hidepwd (SQLCHAR *str)
{
  SQLCHAR *ptr;
  int state = 0;

  for (ptr = str; *ptr;)
    {
      switch (state)
	{
	case -1:
	  if (strchr ("\'\"}", *ptr))
	    state = 0;
	  break;

	case 0:
	  if (toupper(*ptr) == 'P')
	    state = 1;
	  else if (strchr ("\'\"{", *ptr))
	    state = -1;		/* in string */
	  break;

	case 1:
	  if (toupper(*ptr) == 'W')
	    state = 2;
	  else
	    state = 0;
	  break;

	case 2:
	  if (toupper(*ptr) == 'D')
	    state = 3;
	  else
	    state = 0;
	  break;

	case 3:
	  if (*ptr == '=')
	    state = 4;		/* goto password mode */
	  else
	    state = 0;
	  break;

	case 4:
	  if (*ptr == ';')
	    {
	      state = 0;	/* go back to normal mode */
	    }
	  else
	    *ptr = '*';
	  break;
	}
      ptr++;
    }
}


static void
_trace_connstr (
  SQLCHAR		* str, 
  SQLSMALLINT		  len, 
  SQLSMALLINT		* lenptr, 
  int 			  output)
{
  SQLCHAR *dup;
  ssize_t length;

  if (!str)
    {
      trace_emit ("\t\t%-15.15s * 0x0\n", "SQLCHAR");
      return;
    }

  trace_emit ("\t\t%-15.15s * %p\n", "SQLCHAR", str);

  if (!output)
    return;

  /*
   *  Calculate string length
   */
  if (lenptr )
    length = *lenptr;
  else
    length = len;

  if (length == SQL_NTS)
    length = STRLEN (str);


  /*
   *  Make a copy of the string
   */
  if ((dup = (SQLCHAR *) malloc (length + 1)) == NULL)
    return;
  memcpy (dup, str, length);
  dup[length] = '\0';

  /*
   *  Emit the string
   */
  _trace_connstr_hidepwd (dup);
  trace_emit_string (dup, length, 0);
  free (dup);
}


static void
_trace_connstr_w (
  SQLWCHAR		* str, 
  SQLSMALLINT		  len, 
  SQLSMALLINT		* lenptr, 
  int 			  output)
{
  SQLCHAR *dup;
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
   *  Calculate string length
   */
  if (lenptr)
    length = *lenptr;
  else
    length = len;

  /* 
   * Emit the string
   */
  dup = dm_SQL_W2A (str, length);
  _trace_connstr_hidepwd (dup);
  trace_emit_string (dup, SQL_NTS, 1);
  free (dup);
}



static void
_trace_drvcn_completion(SQLUSMALLINT fDriverCompletion)
{
  char *ptr = "invalid completion value";

  switch (fDriverCompletion)
    {
      _S (SQL_DRIVER_PROMPT);
      _S (SQL_DRIVER_COMPLETE);
      _S (SQL_DRIVER_COMPLETE_REQUIRED);
      _S (SQL_DRIVER_NOPROMPT);
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", 
  	"SQLUSMALLINT", (int) fDriverCompletion, ptr);
}


void 
trace_SQLDriverConnect (int trace_leave, int retcode,
  SQLHDBC		  hdbc,
  SQLHWND		  hwnd,
  SQLCHAR		* szConnStrIn,
  SQLSMALLINT		  cbConnStrIn,
  SQLCHAR 		* szConnStrOut,
  SQLSMALLINT		  cbConnStrOutMax,
  SQLSMALLINT 	 	* pcbConnStrOut,
  SQLUSMALLINT		  fDriverCompletion)
{
  /* Trace function */
  _trace_print_function (en_DriverConnect, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_DBC, hdbc);
  _trace_pointer (hwnd);
  _trace_connstr (szConnStrIn, cbConnStrIn, NULL, TRACE_INPUT);
  _trace_stringlen ("SQLSMALLINT", cbConnStrIn);
  _trace_connstr (szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
      TRACE_OUTPUT_SUCCESS);
  _trace_stringlen ("SQLSMALLINT", cbConnStrOutMax);
  _trace_smallint_p (pcbConnStrOut, TRACE_OUTPUT_SUCCESS);
  _trace_drvcn_completion (fDriverCompletion);
}


#if ODBCVER >= 0x0300
void 
trace_SQLDriverConnectW (int trace_leave, int retcode,
  SQLHDBC		  hdbc,
  SQLHWND		  hwnd,
  SQLWCHAR 		* szConnStrIn,
  SQLSMALLINT		  cbConnStrIn,
  SQLWCHAR 		* szConnStrOut,
  SQLSMALLINT		  cbConnStrOutMax,
  SQLSMALLINT 	 	* pcbConnStrOut,
  SQLUSMALLINT		  fDriverCompletion)
{
  /* Trace function */
  _trace_print_function (en_DriverConnectW, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_DBC, hdbc);
  _trace_pointer (hwnd);
  _trace_connstr_w (szConnStrIn, cbConnStrIn, NULL, TRACE_INPUT);
  _trace_stringlen ("SQLSMALLINT", cbConnStrIn);
  _trace_connstr_w (szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
      TRACE_OUTPUT_SUCCESS);
  _trace_stringlen ("SQLSMALLINT", cbConnStrOutMax);
  _trace_smallint_p (pcbConnStrOut, TRACE_OUTPUT_SUCCESS);
  _trace_drvcn_completion (fDriverCompletion);
}
#endif
