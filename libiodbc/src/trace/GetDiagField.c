/*
 *  GetDiagField.c
 *
 *  $Id: GetDiagField.c,v 1.4 2006/01/20 15:58:35 source Exp $
 *
 *  SQLGetDiagField trace functions
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


#if ODBCVER >= 0x0300
void 
_trace_diag_type (SQLSMALLINT type)
{
  char *ptr = "unknown diag identifier";

  switch (type)
    {
      _S (SQL_DIAG_CLASS_ORIGIN);
      _S (SQL_DIAG_COLUMN_NUMBER);
      _S (SQL_DIAG_CONNECTION_NAME);
      _S (SQL_DIAG_CURSOR_ROW_COUNT);
      _S (SQL_DIAG_DYNAMIC_FUNCTION);
      _S (SQL_DIAG_DYNAMIC_FUNCTION_CODE);
      _S (SQL_DIAG_MESSAGE_TEXT);
      _S (SQL_DIAG_NATIVE);
      _S (SQL_DIAG_NUMBER);
      _S (SQL_DIAG_RETURNCODE);
      _S (SQL_DIAG_ROW_COUNT);
      _S (SQL_DIAG_ROW_NUMBER);
      _S (SQL_DIAG_SERVER_NAME);
      _S (SQL_DIAG_SQLSTATE);
      _S (SQL_DIAG_SUBCLASS_ORIGIN);
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLUSMALLINT ", (int) type, ptr);
}

void
trace_SQLGetDiagField (int trace_leave, int retcode,
  SQLSMALLINT		  HandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  DiagIdentifier,
  SQLPOINTER		  DiagInfoPtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr)
{
  /* Trace function */
  _trace_print_function (en_GetDiagField, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handletype (HandleType);
  _trace_handle (HandleType, Handle);
  _trace_smallint (RecNumber);
  _trace_diag_type (DiagIdentifier);
  _trace_pointer (DiagInfoPtr);
  _trace_bufferlen ((SQLINTEGER) BufferLength);
  _trace_smallint_p (StringLengthPtr, TRACE_OUTPUT_SUCCESS);
}


void
trace_SQLGetDiagFieldW (int trace_leave, int retcode,
  SQLSMALLINT		  HandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  DiagIdentifier,
  SQLPOINTER		  DiagInfoPtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr)
{
  /* Trace function */
  _trace_print_function (en_GetDiagFieldW, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handletype (HandleType);
  _trace_handle (HandleType, Handle);
  _trace_smallint (RecNumber);
  _trace_diag_type (DiagIdentifier);
  _trace_pointer (DiagInfoPtr);
  _trace_bufferlen ((SQLINTEGER) BufferLength);
  _trace_smallint_p (StringLengthPtr, trace_leave);
}
#endif
