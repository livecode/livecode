/*
 *  result.c
 *
 *  $Id: result.c,v 1.23 2006/01/20 15:58:35 source Exp $
 *
 *  Prepare for getting query result
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

static SQLRETURN
SQLBindCol_Internal (
    SQLHSTMT hstmt,
    SQLUSMALLINT icol,
    SQLSMALLINT fCType,
    SQLPOINTER rgbValue,
    SQLLEN cbValueMax,
    SQLLEN *pcbValue)
{
  STMT(pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLSMALLINT nCType;
  SQLRETURN retcode;

  /* check argument */
  switch (fCType)
    {
    case SQL_C_DEFAULT:
    case SQL_C_BIT:
    case SQL_C_BINARY:
    case SQL_C_CHAR:
    case SQL_C_DATE:
    case SQL_C_DOUBLE:
    case SQL_C_FLOAT:
    case SQL_C_LONG:
    case SQL_C_SHORT:
    case SQL_C_SLONG:
    case SQL_C_SSHORT:
    case SQL_C_STINYINT:
    case SQL_C_TIME:
    case SQL_C_TIMESTAMP:
    case SQL_C_TINYINT:
    case SQL_C_ULONG:
    case SQL_C_USHORT:
    case SQL_C_UTINYINT:
#if (ODBCVER >= 0x0300)
    case SQL_C_GUID:
    case SQL_C_INTERVAL_DAY:
    case SQL_C_INTERVAL_DAY_TO_HOUR:
    case SQL_C_INTERVAL_DAY_TO_MINUTE:
    case SQL_C_INTERVAL_DAY_TO_SECOND:
    case SQL_C_INTERVAL_HOUR:
    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
    case SQL_C_INTERVAL_HOUR_TO_SECOND:
    case SQL_C_INTERVAL_MINUTE:
    case SQL_C_INTERVAL_MINUTE_TO_SECOND:
    case SQL_C_INTERVAL_MONTH:
    case SQL_C_INTERVAL_SECOND:
    case SQL_C_INTERVAL_YEAR:
    case SQL_C_INTERVAL_YEAR_TO_MONTH:
    case SQL_C_NUMERIC:
    case SQL_C_SBIGINT:
    case SQL_C_TYPE_DATE:
    case SQL_C_TYPE_TIME:
    case SQL_C_TYPE_TIMESTAMP:
    case SQL_C_UBIGINT:
    case SQL_C_WCHAR:
#endif
      break;

    default:
      PUSHSQLERR (pstmt->herr, en_S1003);
      return SQL_ERROR;
    }

  if (cbValueMax < 0)
    {
      PUSHSQLERR (pstmt->herr, en_S1090);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->state > en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);
      return SQL_ERROR;
    }

  /*
   *  Convert C type to ODBC version of driver
   */
  nCType = _iodbcdm_map_c_type (fCType, penv->dodbc_ver);

  /* call driver's function */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_BindCol);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  if (icol != 0 && !penv->unicode_driver && nCType == SQL_C_WCHAR)
    {
      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_BindCol,
          (pstmt->dhstmt, icol, SQL_C_CHAR, rgbValue, 
           cbValueMax, pcbValue));
    }
  else
    CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_BindCol,
        (pstmt->dhstmt, icol, nCType, rgbValue, cbValueMax, pcbValue));

  if (icol != 0 && !penv->unicode_driver && nCType == SQL_C_WCHAR 
      && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO))
    {
      BIND_t tbind;

      tbind.bn_col = icol;
      tbind.bn_type = nCType;
      tbind.bn_data = rgbValue;
      tbind.bn_size = cbValueMax;
      tbind.bn_pInd = pcbValue;
      
      if (rgbValue)
        _iodbcdm_BindColumn (pstmt, &tbind);
      else
        _iodbcdm_UnBindColumn (pstmt, &tbind);
    }

  return retcode;
}


SQLRETURN SQL_API
SQLBindCol (
    SQLHSTMT hstmt,
    SQLUSMALLINT icol,
    SQLSMALLINT fCType,
    SQLPOINTER rgbValue,
    SQLLEN cbValueMax,
    SQLLEN *pcbValue)
{
  ENTER_STMT (hstmt,
      trace_SQLBindCol (TRACE_ENTER,
	  hstmt, icol, fCType, rgbValue, cbValueMax, pcbValue));

  retcode = SQLBindCol_Internal (
      hstmt, icol, fCType, rgbValue, cbValueMax, pcbValue);

  LEAVE_STMT (hstmt,
      trace_SQLBindCol (TRACE_LEAVE,
	  hstmt, icol, fCType, rgbValue, cbValueMax, pcbValue));
}


SQLRETURN SQL_API
SQLGetCursorName_Internal (
  SQLHSTMT 		  hstmt,
  SQLCHAR 		* szCursor,
  SQLSMALLINT 		  cbCursorMax,
  SQLSMALLINT 	 	* pcbCursor,
  char			  waMode)
{
  STMT(pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  void * cursorOut = szCursor;
  void * _Cursor = NULL;

  /* check argument */
  if (cbCursorMax < (SWORD) 0)
    {
      PUSHSQLERR (pstmt->herr, en_S1090);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  if (pstmt->state < en_stmt_cursoropen
      && pstmt->cursor_state == en_stmt_cursor_no)
    {
      PUSHSQLERR (pstmt->herr, en_S1015);

      return SQL_ERROR;
    }

  /* call driver's function */


  if ((penv->unicode_driver && waMode != 'W') 
      || (!penv->unicode_driver && waMode == 'W'))
    {
      if (waMode != 'W')
        {
        /* ansi=>unicode*/
          if ((_Cursor = malloc((cbCursorMax + 1) * sizeof(wchar_t))) == NULL)
	    {
              PUSHSQLERR (pstmt->herr, en_HY001);
              return SQL_ERROR;
            }
        }
      else
        {
        /* unicode=>ansi*/
          if ((_Cursor = malloc(cbCursorMax + 1)) == NULL)
	    {
              PUSHSQLERR (pstmt->herr, en_HY001);
              return SQL_ERROR;
            }
        }
      cursorOut = _Cursor;
    }

  /* call driver */
  CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver, 
    en_GetCursorName, (
       pstmt->dhstmt,
       cursorOut,
       cbCursorMax, 
       pcbCursor));

  if (hproc == SQL_NULL_HPROC)
    {
      MEM_FREE(_Cursor);
      PUSHSQLERR (pstmt->herr, en_IM001);
      return SQL_ERROR;
    }

  if (szCursor 
      && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
      &&  ((penv->unicode_driver && waMode != 'W') 
          || (!penv->unicode_driver && waMode == 'W')))
    {
      if (waMode != 'W')
        {
        /* ansi<=unicode*/
          dm_StrCopyOut2_W2A ((SQLWCHAR *) cursorOut, (SQLCHAR *) szCursor, cbCursorMax, NULL);
        }
      else
        {
        /* unicode<=ansi*/
          dm_StrCopyOut2_A2W ((SQLCHAR *) cursorOut, (SQLWCHAR *) szCursor, cbCursorMax, NULL);
        }
    }

  MEM_FREE(_Cursor);

  return retcode;
}


SQLRETURN SQL_API
SQLGetCursorName (
  SQLHSTMT 		  hstmt,
  SQLCHAR  		* szCursor,
  SQLSMALLINT 		  cbCursorMax,
  SQLSMALLINT 	 	* pcbCursor)
{
  ENTER_STMT (hstmt,
    trace_SQLGetCursorName (TRACE_ENTER,
    	hstmt,
	szCursor, cbCursorMax, pcbCursor));

  retcode = SQLGetCursorName_Internal(
  	hstmt, 
	szCursor, cbCursorMax, pcbCursor, 
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLGetCursorName (TRACE_LEAVE,
    	hstmt,
	szCursor, cbCursorMax, pcbCursor));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLGetCursorNameA (
  SQLHSTMT 		  hstmt,
  SQLCHAR  		* szCursor,
  SQLSMALLINT 		  cbCursorMax,
  SQLSMALLINT 		* pcbCursor)
{
  ENTER_STMT (hstmt,
    trace_SQLGetCursorName (TRACE_ENTER,
    	hstmt,
	szCursor, cbCursorMax, pcbCursor));

  retcode = SQLGetCursorName_Internal(
  	hstmt, 
	szCursor, cbCursorMax, pcbCursor, 
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLGetCursorName (TRACE_LEAVE,
    	hstmt,
	szCursor, cbCursorMax, pcbCursor));
}


SQLRETURN SQL_API
SQLGetCursorNameW (SQLHSTMT hstmt,
    SQLWCHAR		* szCursor,
    SQLSMALLINT		  cbCursorMax,
    SQLSMALLINT 	* pcbCursor)
{
  ENTER_STMT (hstmt,
    trace_SQLGetCursorNameW (TRACE_ENTER,
    	hstmt,
	szCursor, cbCursorMax, pcbCursor));

  retcode = SQLGetCursorName_Internal(
  	hstmt, 
	(SQLCHAR *) szCursor, cbCursorMax, pcbCursor, 
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLGetCursorNameW (TRACE_LEAVE,
    	hstmt,
	szCursor, cbCursorMax, pcbCursor));
}
#endif


static SQLRETURN 
SQLRowCount_Internal (
  SQLHSTMT		  hstmt,
  SQLLEN		* pcrow)
{
  STMT(pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  /* check state */
  if (pstmt->state >= en_stmt_needdata
      || pstmt->state <= en_stmt_prepared
      || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_RowCount);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_RowCount,
      (pstmt->dhstmt, pcrow));

  return retcode;
}


SQLRETURN SQL_API
SQLRowCount (
  SQLHSTMT		  hstmt,
  SQLLEN 		* pcrow)
{
  ENTER_STMT (hstmt,
    trace_SQLRowCount (TRACE_ENTER, hstmt, pcrow));

  retcode = SQLRowCount_Internal (hstmt, pcrow);
    	
  LEAVE_STMT (hstmt,
    trace_SQLRowCount (TRACE_LEAVE, hstmt, pcrow));
}


SQLRETURN SQL_API
_iodbcdm_NumResultCols (
    SQLHSTMT		  hstmt,
    SQLSMALLINT		* pccol)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;
  SWORD ccol;

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state == en_stmt_allocated
	  || pstmt->state >= en_stmt_needdata)
	{
	  PUSHSQLERR (pstmt->herr, en_S1010);

	  return SQL_ERROR;
	}
    }
  else if (pstmt->asyn_on != en_NumResultCols)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_NumResultCols);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_NumResultCols,
      (pstmt->dhstmt, &ccol));

  /* state transition */
  if (pstmt->asyn_on == en_NumResultCols)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;

	case SQL_STILL_EXECUTING:
	default:
	  break;
	}
    }

  switch (retcode)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
      break;

    case SQL_STILL_EXECUTING:
      ccol = 0;
      pstmt->asyn_on = en_NumResultCols;
      break;

    default:
      ccol = 0;
      break;
    }

  if (pccol)
    {
      *pccol = ccol;
    }
  return retcode;
}


SQLRETURN SQL_API
SQLNumResultCols (
  SQLHSTMT		  hstmt,
  SQLSMALLINT 		* pccol)
{
  ENTER_STMT (hstmt,
    trace_SQLNumResultCols (TRACE_ENTER, hstmt, pccol));

  retcode = _iodbcdm_NumResultCols (hstmt, pccol);

  LEAVE_STMT (hstmt,
    trace_SQLNumResultCols (TRACE_LEAVE, hstmt, pccol));
}


SQLSMALLINT
_iodbcdm_map_sql_type (int type, int odbcver)
{
  switch (type)
    {
      case SQL_DATE:
      case SQL_TYPE_DATE:
      	return (odbcver == SQL_OV_ODBC3) ?  SQL_TYPE_DATE : SQL_DATE;

      case SQL_TIME:
      case SQL_TYPE_TIME:
      	return (odbcver == SQL_OV_ODBC3) ?  SQL_TYPE_TIME : SQL_TIME;

      case SQL_TIMESTAMP:
      case SQL_TYPE_TIMESTAMP:
      	return (odbcver == SQL_OV_ODBC3) ?  SQL_TYPE_TIMESTAMP : SQL_TIMESTAMP;
    }

    return type;
}


SQLSMALLINT
_iodbcdm_map_c_type (int type, int odbcver)
{
  switch (type)
    {
      case SQL_C_DATE:
      case SQL_C_TYPE_DATE:
      	return (odbcver == SQL_OV_ODBC3) ?  SQL_C_TYPE_DATE : SQL_C_DATE;

      case SQL_C_TIME:
      case SQL_C_TYPE_TIME:
      	return (odbcver == SQL_OV_ODBC3) ?  SQL_C_TYPE_TIME : SQL_C_TIME;

      case SQL_C_TIMESTAMP:
      case SQL_C_TYPE_TIMESTAMP:
      	return (odbcver == SQL_OV_ODBC3) ?  SQL_C_TYPE_TIMESTAMP : SQL_C_TIMESTAMP;
    }

    return type;
}


SQLRETURN SQL_API
SQLDescribeCol_Internal (
    SQLHSTMT		  hstmt,
    SQLUSMALLINT	  icol,
    SQLPOINTER		  szColName,
    SQLSMALLINT		  cbColNameMax,
    SQLSMALLINT 	* pcbColName,
    SQLSMALLINT 	* pfSqlType,
    SQLULEN	 	* pcbColDef,
    SQLSMALLINT 	* pibScale,
    SQLSMALLINT 	* pfNullable,
    SQLCHAR		  waMode)
{
  STMT(pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  GENV (genv, pdbc->genv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  void * _ColName = NULL;
  void * colNameOut = szColName;
  sqlstcode_t sqlstat = en_00000;

  /* check arguments */
  if (icol == 0)
    {
      sqlstat = en_S1002;
    }
  else if (cbColNameMax < 0)
    {
      sqlstat = en_S1090;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }
#if (ODBCVER < 0x0300)
  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state == en_stmt_allocated
	  || pstmt->state >= en_stmt_needdata)
	{
	  sqlstat = en_S1010;
	}
    }
  else if (pstmt->asyn_on != en_DescribeCol)
    {
      sqlstat = en_S1010;
    }
#endif

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* call driver */

  if ((penv->unicode_driver && waMode != 'W') 
      || (!penv->unicode_driver && waMode == 'W'))
    {
      if (waMode != 'W')
        {
        /* ansi=>unicode*/
          if ((_ColName = _iodbcdm_alloc_param(pstmt, 0, 
          	             cbColNameMax * sizeof(wchar_t))) == NULL)
	    {
              PUSHSQLERR (pstmt->herr, en_HY001);
              return SQL_ERROR;
            }
        }
      else
        {
        /* unicode=>ansi*/
          if ((_ColName = _iodbcdm_alloc_param(pstmt, 0, cbColNameMax)) == NULL)
	    {
              PUSHSQLERR (pstmt->herr, en_HY001);
              return SQL_ERROR;
            }
        }
      colNameOut = _ColName;
    }

  /* call driver */
  CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver, 
    en_DescribeCol, (
       pstmt->dhstmt, 
       icol, 
       colNameOut, 
       cbColNameMax, 
       pcbColName,
       pfSqlType, 
       pcbColDef, 
       pibScale, 
       pfNullable));

  if (hproc == SQL_NULL_HPROC)
    {
      _iodbcdm_FreeStmtParams(pstmt);
      PUSHSQLERR (pstmt->herr, en_IM001);
      return SQL_ERROR;
    }

  /*
   *  Convert sql type to ODBC version of application
   */
  if (SQL_SUCCEEDED(retcode) && pfSqlType)
    *pfSqlType = _iodbcdm_map_sql_type (*pfSqlType, genv->odbc_ver);

  if (szColName 
      && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
      &&  ((penv->unicode_driver && waMode != 'W') 
          || (!penv->unicode_driver && waMode == 'W')))
    {
      if (waMode != 'W')
        {
        /* ansi<=unicode*/
          dm_StrCopyOut2_W2A ((SQLWCHAR *) colNameOut, (SQLCHAR *) szColName, cbColNameMax, NULL);
        }
      else
        {
        /* unicode<=ansi*/
          dm_StrCopyOut2_A2W ((SQLCHAR *) colNameOut, (SQLWCHAR *) szColName, cbColNameMax, NULL);
        }
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  /* state transition */
  if (pstmt->asyn_on == en_DescribeCol)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	default:
	   return retcode;
	}
    }

  switch (pstmt->state)
    {
    case en_stmt_prepared:
    case en_stmt_cursoropen:
    case en_stmt_fetched:
    case en_stmt_xfetched:
      if (retcode == SQL_STILL_EXECUTING)
	{
	  pstmt->asyn_on = en_DescribeCol;
	}
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLDescribeCol (SQLHSTMT hstmt,
    SQLUSMALLINT	  icol,
    SQLCHAR		* szColName,
    SQLSMALLINT		  cbColNameMax,
    SQLSMALLINT		* pcbColName,
    SQLSMALLINT		* pfSqlType,
    SQLULEN		* pcbColDef,
    SQLSMALLINT		* pibScale,
    SQLSMALLINT		* pfNullable)
{
  ENTER_STMT (hstmt,
    trace_SQLDescribeCol (TRACE_ENTER, 
    	hstmt,
	icol,
	szColName, cbColNameMax, pcbColName,
	pfSqlType,
	pcbColDef,
	pibScale,
	pfNullable));

  retcode = SQLDescribeCol_Internal (
  	hstmt, 
	icol, 
	szColName, cbColNameMax, pcbColName, 
	pfSqlType, 
	pcbColDef, 
	pibScale, 
	pfNullable, 
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLDescribeCol (TRACE_LEAVE, 
    	hstmt,
	icol,
	szColName, cbColNameMax, pcbColName,
	pfSqlType,
	pcbColDef,
	pibScale,
	pfNullable));
} 


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLDescribeColA (SQLHSTMT hstmt,
    SQLUSMALLINT	  icol,
    SQLCHAR		* szColName,
    SQLSMALLINT		  cbColNameMax,
    SQLSMALLINT		* pcbColName,
    SQLSMALLINT		* pfSqlType,
    SQLULEN		* pcbColDef,
    SQLSMALLINT		* pibScale,
    SQLSMALLINT		* pfNullable)
{
  ENTER_STMT (hstmt,
    trace_SQLDescribeCol (TRACE_ENTER, 
    	hstmt,
	icol,
	szColName, cbColNameMax, pcbColName,
	pfSqlType,
	pcbColDef,
	pibScale,
	pfNullable));

  retcode = SQLDescribeCol_Internal (
  	hstmt, 
	icol, 
	szColName, cbColNameMax, pcbColName, 
	pfSqlType, 
	pcbColDef, 
	pibScale, 
	pfNullable, 
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLDescribeCol (TRACE_LEAVE, 
    	hstmt,
	icol,
	szColName, cbColNameMax, pcbColName,
	pfSqlType,
	pcbColDef,
	pibScale,
	pfNullable));
}


SQLRETURN SQL_API
SQLDescribeColW (
  SQLHSTMT		  hstmt,
  SQLUSMALLINT		  icol,
  SQLWCHAR		* szColName,
  SQLSMALLINT		  cbColNameMax,
  SQLSMALLINT		* pcbColName,
  SQLSMALLINT		* pfSqlType,
  SQLULEN		* pcbColDef,
  SQLSMALLINT		* pibScale,
  SQLSMALLINT		* pfNullable)
{
  ENTER_STMT (hstmt,
    trace_SQLDescribeColW (TRACE_ENTER, 
    	hstmt,
	icol,
	szColName, cbColNameMax, pcbColName,
	pfSqlType,
	pcbColDef,
	pibScale,
	pfNullable));

  retcode = SQLDescribeCol_Internal (
  	hstmt, 
	icol, 
	szColName, cbColNameMax, pcbColName, 
	pfSqlType, 
	pcbColDef, 
	pibScale, 
	pfNullable, 
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLDescribeColW (TRACE_LEAVE, 
    	hstmt,
	icol,
	szColName, cbColNameMax, pcbColName,
	pfSqlType,
	pcbColDef,
	pibScale,
	pfNullable));
}
#endif


SQLRETURN SQL_API
SQLColAttributes_Internal (
    SQLHSTMT		  hstmt,
    SQLUSMALLINT	  icol,
    SQLUSMALLINT	  fDescType,
    SQLPOINTER		  rgbDesc,
    SQLSMALLINT		  cbDescMax,
    SQLSMALLINT	  	* pcbDesc,
    SQLLEN 		* pfDesc,
    SQLCHAR		  waMode)
{
  STMT(pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  void * _Desc = NULL;
  void * descOut = rgbDesc;
  sqlstcode_t sqlstat = en_00000;
  SQLUSMALLINT new_attr = fDescType;

  /* check arguments */
  if (icol == 0 && fDescType != SQL_COLUMN_COUNT)
    {
      sqlstat = en_S1002;
    }
  else if (cbDescMax < 0)
    {
      sqlstat = en_S1090;
    }
#if (ODBCVER < 0x0300)
  else if (			/* fDescType < SQL_COLATT_OPT_MIN || *//* turnoff warning */
	(fDescType > SQL_COLATT_OPT_MAX
	  && fDescType < SQL_COLUMN_DRIVER_START))
    {
      sqlstat = en_S1091;
    }
#endif /* ODBCVER < 0x0300 */

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state == en_stmt_allocated
	  || pstmt->state >= en_stmt_needdata)
	{
	  sqlstat = en_S1010;
	}
    }
  else if (pstmt->asyn_on != en_ColAttributes)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* call driver */
  if ((penv->unicode_driver && waMode != 'W') 
      || (!penv->unicode_driver && waMode == 'W'))
    {
      switch(fDescType)
        {
        case SQL_COLUMN_QUALIFIER_NAME:
        case SQL_COLUMN_NAME:
        case SQL_COLUMN_LABEL:
        case SQL_COLUMN_OWNER_NAME:
        case SQL_COLUMN_TABLE_NAME:
        case SQL_COLUMN_TYPE_NAME:

          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              cbDescMax *= sizeof(wchar_t);
              if ((_Desc = _iodbcdm_alloc_param(pstmt, 0, cbDescMax)) == NULL)
	        {
                  PUSHSQLERR (pstmt->herr, en_HY001);
                  return SQL_ERROR;
                }
            }
          else
            {
            /* unicode=>ansi*/
              cbDescMax /= sizeof(wchar_t);
              if ((_Desc = _iodbcdm_alloc_param(pstmt, 0, cbDescMax)) == NULL)
	        {
                  PUSHSQLERR (pstmt->herr, en_HY001);
                  return SQL_ERROR;
                }
            }
          descOut = _Desc;
          break;
        }
    }

#if (ODBCVER >= 0x0300)
  switch (new_attr)
    {
    case SQL_COLUMN_NAME:      new_attr = SQL_DESC_NAME;      break;
    case SQL_COLUMN_NULLABLE:  new_attr = SQL_DESC_NULLABLE;  break;
    case SQL_COLUMN_COUNT:     new_attr = SQL_DESC_COUNT;     break;
    }
#endif

  if (penv->unicode_driver)
    {
      /* SQL_XXX_W */
#if (ODBCVER >= 0x0300)
      if ((hproc = _iodbcdm_getproc (pdbc, en_ColAttributeW)) 
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, 
               en_ColAttributeW, (
       	          pstmt->dhstmt, 
       	          icol, 
       	          new_attr, 
       	          descOut, 
       	          cbDescMax, 
       	          pcbDesc, 
       	          pfDesc));
        }
      else 
#endif
      if ((hproc = _iodbcdm_getproc (pdbc, en_ColAttributesW)) 
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, 
               en_ColAttributesW, (
      	          pstmt->dhstmt, 
      	          icol, 
      	          fDescType, 
      	          descOut, 
      	          cbDescMax, 
      	          pcbDesc, 
      	          pfDesc));
       	}
    }
  else
    {
      /* SQL_XXX */
      /* SQL_XXX_A */
#if (ODBCVER >= 0x0300)
      if ((hproc = _iodbcdm_getproc (pdbc, en_ColAttribute)) 
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, 
               en_ColAttribute, (
       	          pstmt->dhstmt, 
       	          icol, 
       	          new_attr, 
       	          descOut, 
       	          cbDescMax, 
       	          pcbDesc, 
       	          pfDesc));
        }
      else 
#endif
      if ((hproc = _iodbcdm_getproc (pdbc, en_ColAttributes)) 
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, 
               en_ColAttributes, (
      	          pstmt->dhstmt, 
      	          icol, 
      	          fDescType, 
      	          descOut, 
      	          cbDescMax, 
      	          pcbDesc, 
      	          pfDesc));
        }
      else
#if (ODBCVER >= 0x0300)
      if ((hproc = _iodbcdm_getproc (pdbc, en_ColAttributeA)) 
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, 
               en_ColAttributeA, (
       	          pstmt->dhstmt, 
       	          icol, 
       	          new_attr, 
       	          descOut, 
       	          cbDescMax, 
       	          pcbDesc, 
       	          pfDesc));
        }
      else 
#endif
      if ((hproc = _iodbcdm_getproc (pdbc, en_ColAttributesA)) 
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, 
               en_ColAttributesA, (
      	          pstmt->dhstmt, 
      	          icol, 
      	          fDescType, 
      	          descOut, 
      	          cbDescMax, 
      	          pcbDesc, 
      	          pfDesc));
        }
    }

  if (hproc == SQL_NULL_HPROC)
    {
      _iodbcdm_FreeStmtParams(pstmt);
      PUSHSQLERR (pstmt->herr, en_IM001);
      return SQL_ERROR;
    }

  if (rgbDesc 
      && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
      &&  ((penv->unicode_driver && waMode != 'W') 
          || (!penv->unicode_driver && waMode == 'W')))
    {
      switch(fDescType)
        {
        case SQL_COLUMN_QUALIFIER_NAME:
        case SQL_COLUMN_NAME:
        case SQL_COLUMN_LABEL:
        case SQL_COLUMN_OWNER_NAME:
        case SQL_COLUMN_TABLE_NAME:
        case SQL_COLUMN_TYPE_NAME:
          if (waMode != 'W')
            {
            /* ansi<=unicode*/
              dm_StrCopyOut2_W2A ((SQLWCHAR *) descOut, (SQLCHAR *) rgbDesc, cbDescMax / sizeof(wchar_t), pcbDesc);
            }
          else
            {
            /* unicode<=ansi*/
              dm_StrCopyOut2_A2W ((SQLCHAR *) descOut, (SQLWCHAR *) rgbDesc, cbDescMax, pcbDesc);
              if (pcbDesc)
                *pcbDesc = *pcbDesc * sizeof(wchar_t);
            }
        }
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  /* state transition */
  if (pstmt->asyn_on == en_ColAttributes)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	default:
	   return retcode;
	}
    }

  switch (pstmt->state)
    {
    case en_stmt_prepared:
    case en_stmt_cursoropen:
    case en_stmt_fetched:
    case en_stmt_xfetched:
      if (retcode == SQL_STILL_EXECUTING)
	{
	  pstmt->asyn_on = en_ColAttributes;
	}
      break;

    default:
      break;
    }

  return retcode;
}


RETCODE SQL_API
SQLColAttributes (
  SQLHSTMT 		  statementHandle,
  SQLUSMALLINT		  icol,
  SQLUSMALLINT		  fDescType,
  SQLPOINTER		  rgbDesc,
  SQLSMALLINT		  cbDescMax,
  SQLSMALLINT 		* pcbDesc,
  SQLLEN 		* pfDesc)
{
  ENTER_STMT (statementHandle,
    trace_SQLColAttributes (TRACE_ENTER,
    	statementHandle, 
	icol, 
	fDescType,
	rgbDesc, cbDescMax, pcbDesc, 
	pfDesc));

  retcode =  SQLColAttributes_Internal (
  	statementHandle, 
	icol, 
	fDescType,
	rgbDesc, cbDescMax, pcbDesc, 
	pfDesc, 
	'A');

  LEAVE_STMT (statementHandle,
    trace_SQLColAttributes (TRACE_LEAVE,
    	statementHandle, 
	icol, 
	fDescType,
	rgbDesc, cbDescMax, pcbDesc, 
	pfDesc));
}


#if ODBCVER >= 0x0300
RETCODE SQL_API
SQLColAttributesA (
  SQLHSTMT 		  statementHandle,
  SQLUSMALLINT		  icol,
  SQLUSMALLINT		  fDescType,
  SQLPOINTER		  rgbDesc,
  SQLSMALLINT		  cbDescMax,
  SQLSMALLINT 		* pcbDesc,
  SQLLEN		* pfDesc)
{
  ENTER_STMT (statementHandle,
    trace_SQLColAttributes (TRACE_ENTER,
    	statementHandle, 
	icol, 
	fDescType,
	rgbDesc, cbDescMax, pcbDesc, 
	pfDesc));

  retcode =  SQLColAttributes_Internal (
  	statementHandle, 
	icol, 
	fDescType,
	rgbDesc, cbDescMax, pcbDesc, 
	pfDesc, 
	'A');

  LEAVE_STMT (statementHandle,
    trace_SQLColAttributes (TRACE_LEAVE,
    	statementHandle, 
	icol, 
	fDescType,
	rgbDesc, cbDescMax, pcbDesc, 
	pfDesc));
}


RETCODE SQL_API
SQLColAttributesW (
  SQLHSTMT		  statementHandle,
  SQLUSMALLINT		  icol,
  SQLUSMALLINT		  fDescType,
  SQLPOINTER		  rgbDesc,
  SQLSMALLINT		  cbDescMax,
  SQLSMALLINT 		* pcbDesc,
  SQLLEN		* pfDesc)
{
  ENTER_STMT (statementHandle,
    trace_SQLColAttributesW (TRACE_ENTER,
    	statementHandle, 
	icol, 
	fDescType,
	rgbDesc, cbDescMax, pcbDesc, 
	pfDesc));

  retcode =  SQLColAttributes_Internal (
  	statementHandle, 
	icol, 
	fDescType,
	rgbDesc, cbDescMax, pcbDesc, 
	pfDesc, 
	'W');

  LEAVE_STMT (statementHandle,
    trace_SQLColAttributesW (TRACE_LEAVE,
    	statementHandle, 
	icol, 
	fDescType,
	rgbDesc, cbDescMax, pcbDesc, 
	pfDesc));
}
#endif
