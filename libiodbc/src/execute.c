/*
 *  execute.c
 *
 *  $Id: execute.c,v 1.22 2006/01/20 15:58:34 source Exp $
 *
 *  Invoke a query
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1995 by Ke Jin <kejin@empress.com>
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


#include <iodbc.h>

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>

#include <unicode.h>

#include <dlproc.h>

#include <herr.h>
#include <henv.h>
#include <hdbc.h>
#include <hstmt.h>

#include <itrace.h>

void
_iodbcdm_do_cursoropen (STMT_t * pstmt)
{
  SQLRETURN retcode;
  SWORD ncol;

  pstmt->state = en_stmt_executed;

  retcode = _iodbcdm_NumResultCols ((SQLHSTMT) pstmt, &ncol);

  if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
    {
      if (ncol)
	{
	  pstmt->state = en_stmt_cursoropen;
	  pstmt->cursor_state = en_stmt_cursor_opened;
	}
      else
	{
	  pstmt->state = en_stmt_executed;
	  pstmt->cursor_state = en_stmt_cursor_no;
	}
    }
}


static SQLRETURN
SQLExecute_Internal (SQLHSTMT hstmt)
{
  STMT (pstmt, hstmt);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode;
  sqlstcode_t sqlstat = en_00000;

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	  sqlstat = en_S1010;
	  break;

	case en_stmt_executed_with_info:
	case en_stmt_executed:
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	  break;

	case en_stmt_cursoropen:
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	  break;

	case en_stmt_fetched:
	case en_stmt_xfetched:
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	  else
	    {
	      sqlstat = en_24000;
	    }
	  break;

	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  sqlstat = en_S1010;
	  break;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_Execute)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat == en_00000)
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_Execute);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_Execute,
      (pstmt->dhstmt));

  /* stmt state transition */
  if (pstmt->asyn_on == en_Execute)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NEED_DATA:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  switch (pstmt->state)
    {
    case en_stmt_prepared:
      switch (retcode)
	{
	case SQL_SUCCESS:
	  _iodbcdm_do_cursoropen (pstmt);
	  break;

	case SQL_SUCCESS_WITH_INFO:
	  pstmt->state = en_stmt_executed_with_info;
	  break;

	case SQL_NEED_DATA:
	  pstmt->state = en_stmt_needdata;
	  pstmt->need_on = en_Execute;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = en_Execute;
	  break;

	default:
	  break;
	}
      break;

    case en_stmt_executed:
      switch (retcode)
	{
	case SQL_ERROR:
	  pstmt->state = en_stmt_prepared;
	  pstmt->cursor_state = en_stmt_cursor_no;
	  break;

	case SQL_NEED_DATA:
	  pstmt->state = en_stmt_needdata;
	  pstmt->need_on = en_Execute;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = en_Execute;
	  break;

	default:
	  break;
	}
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLExecute (SQLHSTMT hstmt)
{
  ENTER_STMT (hstmt,
    trace_SQLExecute (TRACE_ENTER, hstmt));

  retcode = SQLExecute_Internal (hstmt);

  LEAVE_STMT (hstmt,
    trace_SQLExecute (TRACE_LEAVE, hstmt));
}


SQLRETURN SQL_API
SQLExecDirect_Internal (SQLHSTMT hstmt,
    SQLPOINTER szSqlStr,
    SQLINTEGER cbSqlStr,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _SqlStr = NULL;

  /* check arguments */
  if (szSqlStr == NULL)
    {
      sqlstat = en_S1009;
    }
  else if (cbSqlStr < 0 && cbSqlStr != SQL_NTS)
    {
      sqlstat = en_S1090;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_fetched:
	case en_stmt_xfetched:
	  sqlstat = en_24000;
	  break;

	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  sqlstat = en_S1010;
	  break;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_ExecDirect)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  if ((penv->unicode_driver && waMode != 'W')
      || (!penv->unicode_driver && waMode == 'W'))
    {
      if (waMode != 'W')
        {
        /* ansi=>unicode*/
          _SqlStr = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *) szSqlStr, cbSqlStr);
        }
      else
        {
        /* unicode=>ansi*/
          _SqlStr = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *) szSqlStr, cbSqlStr);
        }
      szSqlStr = _SqlStr;
      cbSqlStr = SQL_NTS;
    }

  CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
    en_ExecDirect, (
       pstmt->dhstmt,
       szSqlStr,
       cbSqlStr));

  if (hproc == SQL_NULL_HPROC)
    {
      _iodbcdm_FreeStmtParams(pstmt);
      PUSHSQLERR (pstmt->herr, en_IM001);
      return SQL_ERROR;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  /* stmt state transition */
  if (pstmt->asyn_on == en_ExecDirect)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NEED_DATA:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  if (pstmt->state <= en_stmt_executed)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	  _iodbcdm_do_cursoropen (pstmt);
	  pstmt->prep_state = 1;
	  break;

	case SQL_SUCCESS_WITH_INFO:
	  pstmt->state = en_stmt_executed_with_info;
	  pstmt->prep_state = 1;
	  break;

	case SQL_NEED_DATA:
	  pstmt->state = en_stmt_needdata;
	  pstmt->need_on = en_ExecDirect;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = en_ExecDirect;
	  break;

	case SQL_ERROR:
	  pstmt->state = en_stmt_allocated;
	  pstmt->cursor_state = en_stmt_cursor_no;
	  pstmt->prep_state = 0;
	  break;

	default:
	  break;
	}
    }

  return retcode;
}


SQLRETURN SQL_API
SQLExecDirect (SQLHSTMT hstmt, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr)
{
  ENTER_STMT (hstmt,
    trace_SQLExecDirect (TRACE_ENTER, hstmt, szSqlStr, cbSqlStr));

  retcode = SQLExecDirect_Internal(hstmt, szSqlStr, cbSqlStr, 'A');

  LEAVE_STMT (hstmt,
    trace_SQLExecDirect (TRACE_LEAVE, hstmt, szSqlStr, cbSqlStr));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLExecDirectA (SQLHSTMT hstmt, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr)
{
  ENTER_STMT (hstmt,
    trace_SQLExecDirect (TRACE_ENTER, hstmt, szSqlStr, cbSqlStr));

  retcode = SQLExecDirect_Internal(hstmt, szSqlStr, cbSqlStr, 'A');

  LEAVE_STMT (hstmt,
    trace_SQLExecDirect (TRACE_LEAVE, hstmt, szSqlStr, cbSqlStr));
}


SQLRETURN SQL_API
SQLExecDirectW (SQLHSTMT hstmt, SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStr)
{
  ENTER_STMT (hstmt,
    trace_SQLExecDirectW (TRACE_ENTER, hstmt, szSqlStr, cbSqlStr));

  retcode = SQLExecDirect_Internal(hstmt, szSqlStr, cbSqlStr, 'W');

  LEAVE_STMT (hstmt,
    trace_SQLExecDirectW (TRACE_LEAVE, hstmt, szSqlStr, cbSqlStr));
}
#endif


static SQLRETURN
SQLPutData_Internal (
  SQLHSTMT		  hstmt,
  SQLPOINTER		  rgbValue, 
  SQLLEN		  cbValue)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  /* check argument value */
  if (rgbValue == NULL &&
      (cbValue != SQL_DEFAULT_PARAM && cbValue != SQL_NULL_DATA))
    {
      PUSHSQLERR (pstmt->herr, en_S1009);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state <= en_stmt_xfetched)
	{
	  PUSHSQLERR (pstmt->herr, en_S1010);

	  return SQL_ERROR;
	}
    }
  else if (pstmt->asyn_on != en_PutData)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_PutData);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_PutData,
      (pstmt->dhstmt, rgbValue, cbValue));

  /* state transition */
  if (pstmt->asyn_on == en_PutData)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  /* must in mustput or canput states */
  switch (retcode)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
      pstmt->state = en_stmt_canput;
      break;

    case SQL_ERROR:
      switch (pstmt->need_on)
	{
	case en_ExecDirect:
	  pstmt->state = en_stmt_allocated;
	  pstmt->need_on = en_NullProc;
	  break;

	case en_Execute:
	  if (pstmt->prep_state)
	    {
	      pstmt->state = en_stmt_prepared;
	      pstmt->need_on = en_NullProc;
	    }
	  break;

	case en_SetPos:
	  /* Is this possible ???? */
	  pstmt->state = en_stmt_xfetched;
	  break;

	default:
	  break;
	}
      break;

    case SQL_STILL_EXECUTING:
      pstmt->asyn_on = en_PutData;
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLPutData (
  SQLHSTMT		  hstmt, 
  SQLPOINTER		  rgbValue, 
  SQLLEN		  cbValue)
{
  ENTER_STMT (hstmt,
    trace_SQLPutData (TRACE_ENTER, hstmt, rgbValue, cbValue));

  retcode = SQLPutData_Internal (hstmt, rgbValue, cbValue);

  LEAVE_STMT (hstmt,
    trace_SQLPutData (TRACE_LEAVE, hstmt, rgbValue, cbValue));
}


static SQLRETURN
SQLParamData_Internal (SQLHSTMT hstmt, SQLPOINTER * prgbValue)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  /* check argument */

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state <= en_stmt_xfetched)
	{
	  PUSHSQLERR (pstmt->herr, en_S1010);

	  return SQL_ERROR;
	}
    }
  else if (pstmt->asyn_on != en_ParamData)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_ParamData);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_ParamData,
      (pstmt->dhstmt, prgbValue));

  /* state transition */
  if (pstmt->asyn_on == en_ParamData)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  if (pstmt->state < en_stmt_needdata)
    {
      return retcode;
    }

  switch (retcode)
    {
    case SQL_ERROR:
      switch (pstmt->need_on)
	{
	case en_ExecDirect:
	  pstmt->state = en_stmt_allocated;
	  break;

	case en_Execute:
	  pstmt->state = en_stmt_prepared;
	  break;

	case en_SetPos:
	  pstmt->state = en_stmt_xfetched;
	  pstmt->cursor_state = en_stmt_cursor_xfetched;
	  break;

	default:
	  break;
	}
      pstmt->need_on = en_NullProc;
      break;

    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
      switch (pstmt->state)
	{
	case en_stmt_needdata:
	  pstmt->state = en_stmt_mustput;
	  break;

	case en_stmt_canput:
	  switch (pstmt->need_on)
	    {
	    case en_SetPos:
	      pstmt->state = en_stmt_xfetched;
	      pstmt->cursor_state = en_stmt_cursor_xfetched;
	      break;

	    case en_ExecDirect:
	    case en_Execute:
	      _iodbcdm_do_cursoropen (pstmt);
	      break;

	    default:
	      break;
	    }
	  break;

	default:
	  break;
	}
      pstmt->need_on = en_NullProc;
      break;

    case SQL_NEED_DATA:
      pstmt->state = en_stmt_mustput;
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLParamData (SQLHSTMT hstmt, SQLPOINTER * prgbValue)
{
  ENTER_STMT (hstmt,
    trace_SQLParamData (TRACE_ENTER, hstmt, prgbValue));

  retcode = SQLParamData_Internal (hstmt, prgbValue);

  LEAVE_STMT (hstmt,
    trace_SQLParamData (TRACE_LEAVE, hstmt, prgbValue));
}


static SQLRETURN
SQLNumParams_Internal (SQLHSTMT hstmt, SQLSMALLINT * pcpar)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  /* check argument */
  if (!pcpar)
    {
      return SQL_SUCCESS;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  PUSHSQLERR (pstmt->herr, en_S1010);
	  return SQL_ERROR;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_NumParams)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_NumParams);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_NumParams,
      (pstmt->dhstmt, pcpar));

  /* state transition */
  if (pstmt->asyn_on == en_NumParams)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  break;

	default:
	  return retcode;
	}
    }

  if (retcode == SQL_STILL_EXECUTING)
    {
      pstmt->asyn_on = en_NumParams;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLNumParams (SQLHSTMT hstmt, SQLSMALLINT * pcpar)
{
  ENTER_STMT (hstmt,
    trace_SQLNumParams (TRACE_ENTER, hstmt, pcpar));

  retcode = SQLNumParams_Internal (hstmt, pcpar);

  LEAVE_STMT (hstmt,
    trace_SQLNumParams (TRACE_LEAVE, hstmt, pcpar));
}


static SQLRETURN
SQLDescribeParam_Internal (
    SQLHSTMT		  hstmt,
    SQLUSMALLINT	  ipar,
    SQLSMALLINT		* pfSqlType,
    SQLULEN		* pcbColDef,
    SQLSMALLINT		* pibScale, 
    SQLSMALLINT 	* pfNullable)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  GENV (genv, pdbc->genv);

  HPROC hproc;
  SQLRETURN retcode;

  /* check argument */
  if (ipar == 0)
    {
      PUSHSQLERR (pstmt->herr, en_S1093);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  PUSHSQLERR (pstmt->herr, en_S1010);
	  return SQL_ERROR;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_DescribeParam)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_DescribeParam);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_DescribeParam,
      (pstmt->dhstmt, ipar, pfSqlType, pcbColDef, pibScale, pfNullable));

  /*
   *  Convert sql type to ODBC version of application
   */
  if (SQL_SUCCEEDED(retcode) && pfSqlType)
    *pfSqlType = _iodbcdm_map_sql_type (*pfSqlType, genv->odbc_ver);


  /* state transition */
  if (pstmt->asyn_on == en_DescribeParam)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  break;

	default:
	  return retcode;
	}
    }

  if (retcode == SQL_STILL_EXECUTING)
    {
      pstmt->asyn_on = en_DescribeParam;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLDescribeParam (
  SQLHSTMT		  hstmt,
  SQLUSMALLINT		  ipar,
  SQLSMALLINT 		* pfSqlType,
  SQLULEN 		* pcbColDef,
  SQLSMALLINT 		* pibScale,
  SQLSMALLINT 		* pfNullable)
{
  ENTER_STMT (hstmt,
    trace_SQLDescribeParam (TRACE_ENTER,
      hstmt, ipar, pfSqlType,
      pcbColDef, pibScale, pfNullable));

  retcode = SQLDescribeParam_Internal ( hstmt, ipar, pfSqlType,
      pcbColDef, pibScale, pfNullable);

  LEAVE_STMT (hstmt,
    trace_SQLDescribeParam (TRACE_LEAVE,
      hstmt, ipar, pfSqlType,
      pcbColDef, pibScale, pfNullable));
}
