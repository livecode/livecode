/*
 *  GetFunctions.c
 *
 *  $Id: GetFunctions.c,v 1.5 2006/01/20 15:58:35 source Exp $
 *
 *  SQLGetFunctions trace functions
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

static void
_trace_func_name (SQLUSMALLINT fFunc, int format)
{
  char *ptr = "unknown function";

  switch (fFunc)
    {
/* All ODBC 2.x functions */
      _S (SQL_API_ALL_FUNCTIONS);

/* ODBC 2.x */
      _S (SQL_API_SQLALLOCCONNECT);
      _S (SQL_API_SQLALLOCENV);
      _S (SQL_API_SQLALLOCSTMT);
      _S (SQL_API_SQLBINDCOL);
      _S (SQL_API_SQLBINDPARAMETER);
      _S (SQL_API_SQLBROWSECONNECT);
      _S (SQL_API_SQLCANCEL);
#if (ODBCVER < 0x0300)
      _S (SQL_API_SQLCOLATTRIBUTES);
#endif
      _S (SQL_API_SQLCOLUMNPRIVILEGES);
      _S (SQL_API_SQLCOLUMNS);
      _S (SQL_API_SQLCONNECT);
      _S (SQL_API_SQLDATASOURCES);
      _S (SQL_API_SQLDESCRIBECOL);
      _S (SQL_API_SQLDESCRIBEPARAM);
      _S (SQL_API_SQLDISCONNECT);
      _S (SQL_API_SQLDRIVERCONNECT);
      _S (SQL_API_SQLDRIVERS);
      _S (SQL_API_SQLERROR);
      _S (SQL_API_SQLEXECDIRECT);
      _S (SQL_API_SQLEXECUTE);
      _S (SQL_API_SQLEXTENDEDFETCH);
      _S (SQL_API_SQLFETCH);
      _S (SQL_API_SQLFOREIGNKEYS);
      _S (SQL_API_SQLFREECONNECT);
      _S (SQL_API_SQLFREEENV);
      _S (SQL_API_SQLFREESTMT);
      _S (SQL_API_SQLGETCONNECTOPTION);
      _S (SQL_API_SQLGETCURSORNAME);
      _S (SQL_API_SQLGETDATA);
      _S (SQL_API_SQLGETFUNCTIONS);
      _S (SQL_API_SQLGETINFO);
      _S (SQL_API_SQLGETSTMTOPTION);
      _S (SQL_API_SQLGETTYPEINFO);
      _S (SQL_API_SQLMORERESULTS);
      _S (SQL_API_SQLNATIVESQL);
      _S (SQL_API_SQLNUMPARAMS);
      _S (SQL_API_SQLNUMRESULTCOLS);
      _S (SQL_API_SQLPARAMDATA);
      _S (SQL_API_SQLPARAMOPTIONS);
      _S (SQL_API_SQLPREPARE);
      _S (SQL_API_SQLPRIMARYKEYS);
      _S (SQL_API_SQLPROCEDURECOLUMNS);
      _S (SQL_API_SQLPROCEDURES);
      _S (SQL_API_SQLPUTDATA);
      _S (SQL_API_SQLROWCOUNT);
      _S (SQL_API_SQLSETCONNECTOPTION);
      _S (SQL_API_SQLSETCURSORNAME);
      _S (SQL_API_SQLSETPARAM);
      _S (SQL_API_SQLSETPOS);
      _S (SQL_API_SQLSETSCROLLOPTIONS);
      _S (SQL_API_SQLSETSTMTOPTION);
      _S (SQL_API_SQLSPECIALCOLUMNS);
      _S (SQL_API_SQLSTATISTICS);
      _S (SQL_API_SQLTABLEPRIVILEGES);
      _S (SQL_API_SQLTABLES);
      _S (SQL_API_SQLTRANSACT);
#if (ODBCVER >= 0x0300)
/* All ODBC 2.x functions */
      _S (SQL_API_ODBC3_ALL_FUNCTIONS);

/* ODBC 3.x */
      _S (SQL_API_SQLALLOCHANDLE);
      _S (SQL_API_SQLALLOCHANDLESTD);
      _S (SQL_API_SQLBINDPARAM);
      _S (SQL_API_SQLBULKOPERATIONS);
      _S (SQL_API_SQLCLOSECURSOR);
      _S (SQL_API_SQLCOLATTRIBUTE);
      _S (SQL_API_SQLCOPYDESC);
      _S (SQL_API_SQLENDTRAN);
      _S (SQL_API_SQLFETCHSCROLL);
      _S (SQL_API_SQLFREEHANDLE);
      _S (SQL_API_SQLGETCONNECTATTR);
      _S (SQL_API_SQLGETDESCFIELD);
      _S (SQL_API_SQLGETDESCREC);
      _S (SQL_API_SQLGETDIAGFIELD);
      _S (SQL_API_SQLGETDIAGREC);
      _S (SQL_API_SQLGETENVATTR);
      _S (SQL_API_SQLGETSTMTATTR);
      _S (SQL_API_SQLSETCONNECTATTR);
      _S (SQL_API_SQLSETDESCFIELD);
      _S (SQL_API_SQLSETDESCREC);
      _S (SQL_API_SQLSETENVATTR);
      _S (SQL_API_SQLSETSTMTATTR);

#endif
    }

  if (format)
    trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLUSMALLINT", (int) fFunc, ptr);
  else
    trace_emit_string (ptr, SQL_NTS, 0);
}


void
_trace_func_result (
    SQLUSMALLINT	  fFunc, 
    SQLUSMALLINT	* pfExists, 
    int			  output)
{
  int i;

  if (fFunc == SQL_API_ALL_FUNCTIONS)
    {
      _trace_usmallint_p (pfExists, 0);

      if (!output)
        return;

      for (i = 1; i < 100; i++)
	if (pfExists[i])
	  _trace_func_name (i, 0);
    }
#if (ODBCVER >= 0x0300)
  else if (fFunc == SQL_API_ODBC3_ALL_FUNCTIONS)
    {
      if (!output)
        return;

      _trace_usmallint_p (pfExists, 0);
      for (i = 1; i < SQL_API_ODBC3_ALL_FUNCTIONS; i++)
	if (SQL_FUNC_EXISTS (pfExists, i))
	  _trace_func_name (i, 0);
    }
#endif
  else
    {
      _trace_usmallint_p (pfExists, output);
    }
}


void
trace_SQLGetFunctions (int trace_leave, int retcode,
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fFunc,
  SQLUSMALLINT     	* pfExists)
{
  /* Trace function */
  _trace_print_function (en_GetFunctions, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_DBC, hdbc);
  _trace_func_name (fFunc, 1);
  _trace_func_result (fFunc, pfExists, trace_leave);
}
