/*
 *  SetPos.c
 *
 *  $Id: SetPos.c,v 1.6 2006/01/20 15:58:35 source Exp $
 *
 *  SQLSetPos trace functions
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
_trace_setpos_irow (SQLSETPOSIROW i)
{
#ifdef _WIN64
  trace_emit ("\t\t%-15.15s   %I64d\n", "SQLSETPOSIROW", (INT64) i);
#else
  trace_emit ("\t\t%-15.15s   %ld\n", "SQLSETPOSIROW", (long) i);
#endif
}


void
_trace_setpos_oper (SQLUSMALLINT type)
{
  char *ptr = "unknown operation";

  switch (type)
    {
      _S (SQL_POSITION);
      _S (SQL_REFRESH);
      _S (SQL_UPDATE);
      _S (SQL_DELETE);
      _S (SQL_ADD);
#if ODBCVER >= 0x0300
      _S (SQL_UPDATE_BY_BOOKMARK);
      _S (SQL_DELETE_BY_BOOKMARK);
      _S (SQL_FETCH_BY_BOOKMARK);
#endif
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLUSMALLINT", (int) type, ptr);
}


void
_trace_setpos_lock (SQLUSMALLINT type)
{
  char *ptr = "unknown lock type";

  switch (type)
    {
      _S (SQL_LOCK_NO_CHANGE);
      _S (SQL_LOCK_EXCLUSIVE);
      _S (SQL_LOCK_UNLOCK);
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLUSMALLINT", (int) type, ptr);
}


void
trace_SQLSetPos (int trace_leave, int retcode,
  SQLHSTMT		  hstmt,
  SQLSETPOSIROW		  irow,
  SQLUSMALLINT		  fOption,
  SQLUSMALLINT		  fLock)
{
  /* Trace function */
  _trace_print_function (en_SetPos, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_STMT, hstmt);
  _trace_setpos_irow (irow);
  _trace_setpos_oper (fOption);
  _trace_setpos_lock (fLock);
}
