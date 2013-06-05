/*
 *  GetTypeInfo.c
 *
 *  $Id: GetTypeInfo.c,v 1.4 2006/01/20 15:58:35 source Exp $
 *
 *  SQLGetTypeInfo trace functions
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


void
_trace_typeinfo (SQLSMALLINT type)
{
  char *ptr = "unknown type";

  switch (type)
    {
      _S (SQL_ALL_TYPES);
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

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLSMALLINT ", (int) type, ptr);
}


void 
trace_SQLGetTypeInfo (int trace_leave, int retcode,
  SQLHSTMT		  hstmt,
  SQLSMALLINT		  fSqlType)
{
  /* Trace function */
  _trace_print_function (en_GetTypeInfo, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_STMT, hstmt);
  _trace_typeinfo (fSqlType);
}


#if ODBCVER >= 0x0300
void 
trace_SQLGetTypeInfoW (int trace_leave, int retcode,
  SQLHSTMT		  hstmt,
  SQLSMALLINT		  fSqlType)
{
  /* Trace function */
  _trace_print_function (en_GetTypeInfoW, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_STMT, hstmt);
  _trace_typeinfo (fSqlType);
}
#endif
