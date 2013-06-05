/*
 *  GetConnectOption.c
 *
 *  $Id: GetConnectOption.c,v 1.4 2006/01/20 15:58:35 source Exp $
 *
 *  SQLGetConnectOption trace functions
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
_trace_connopt_type (SQLUSMALLINT type)
{
  char *ptr = "unknown connection attribute";

  switch (type)
    {
      _S (SQL_ACCESS_MODE);
      _S (SQL_AUTOCOMMIT);
      _S (SQL_CURRENT_QUALIFIER);
      _S (SQL_LOGIN_TIMEOUT);
      _S (SQL_ODBC_CURSORS);
      _S (SQL_OPT_TRACE);
      _S (SQL_OPT_TRACEFILE);
      _S (SQL_PACKET_SIZE);
      _S (SQL_QUIET_MODE);
      _S (SQL_TRANSLATE_DLL);
      _S (SQL_TRANSLATE_OPTION);
      _S (SQL_TXN_ISOLATION);

      /* 2.0 Driver Manager also allows statement options at this time */
      _S (SQL_ASYNC_ENABLE);
      _S (SQL_BIND_TYPE);
      _S (SQL_CONCURRENCY);
      _S (SQL_CURSOR_TYPE);
      _S (SQL_KEYSET_SIZE);
      _S (SQL_MAX_LENGTH);
      _S (SQL_MAX_ROWS);
      _S (SQL_NOSCAN);
      _S (SQL_QUERY_TIMEOUT);
      _S (SQL_RETRIEVE_DATA);
      _S (SQL_ROWSET_SIZE);
      _S (SQL_SIMULATE_CURSOR);
      _S (SQL_USE_BOOKMARKS);
    }

  trace_emit ("\t\t%-15.15s   %ld (%s)\n", "SQLUSMALLINT ", (int) type, ptr);
}


void
trace_SQLGetConnectOption (int trace_leave, int retcode,
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fOption,
  SQLPOINTER		  pvParam)
{
  /* Trace function */
  _trace_print_function (en_GetConnectOption, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_DBC, hdbc);
  _trace_connopt_type (fOption);
  _trace_pointer (pvParam);
}


#if ODBCVER >= 0x0300
void
trace_SQLGetConnectOptionW (int trace_leave, int retcode,
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fOption,
  SQLPOINTER		  pvParam)
{
  /* Trace function */
  _trace_print_function (en_GetConnectOptionW, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_DBC, hdbc);
  _trace_connopt_type (fOption);
  _trace_pointer (pvParam);
}
#endif
