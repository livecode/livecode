/*
 *  catalog.c
 *
 *  $Id: catalog.c,v 1.17 2006/01/20 15:58:34 source Exp $
 *
 *  Catalog functions
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

#include <herr.h>
#include <henv.h>
#include <hdbc.h>
#include <hstmt.h>

#include <dlproc.h>
#include <itrace.h>

/*
 *  Check state for executing catalog functions
 */
static SQLRETURN
_iodbcdm_cata_state_ok (
    STMT_t * pstmt,
    int fidx)
{
  sqlstcode_t sqlstat = en_00000;

  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  sqlstat = en_S1010;
	  break;

	case en_stmt_fetched:
	case en_stmt_xfetched:
	  sqlstat = en_24000;
	  break;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != fidx)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  return SQL_SUCCESS;
}


/*
 *  State transition for catalog function
 */
static SQLRETURN
_iodbcdm_cata_state_tr (
    STMT_t * pstmt,
    int fidx,
    SQLRETURN result)
{

  if (pstmt->asyn_on == fidx)
    {
      switch (result)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return result;
	}
    }

  if (pstmt->state <= en_stmt_executed)
    {
      switch (result)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	  pstmt->state = en_stmt_cursoropen;
	  break;

	case SQL_ERROR:
	  pstmt->state = en_stmt_allocated;
	  pstmt->prep_state = 0;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = fidx;
	  break;

	default:
	  break;
	}
    }

  return result;
}


SQLRETURN SQL_API
SQLGetTypeInfo_Internal (
    SQLHSTMT hstmt,
    SQLSMALLINT fSqlType,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;

  waMode = waMode;	/*NOTUSED*/

  for (;;)
    {
#if (ODBCVER < 0x0300)
      if (fSqlType > SQL_TYPE_MAX)
	{
	  sqlstat = en_S1004;
	  break;
	}

      /* Note: SQL_TYPE_DRIVER_START is a negative number So, we use ">" */
      if (fSqlType < SQL_TYPE_MIN && fSqlType > SQL_TYPE_DRIVER_START)
	{
	  sqlstat = en_S1004;
	  break;
	}
#endif	/* ODBCVER < 0x0300 */

      retcode = _iodbcdm_cata_state_ok (pstmt, en_GetTypeInfo);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_GetTypeInfo, (
           pstmt->dhstmt,
           fSqlType));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;
      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_GetTypeInfo, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLGetTypeInfo (SQLHSTMT hstmt,
    SQLSMALLINT fSqlType)
{
  ENTER_STMT (hstmt,
    trace_SQLGetTypeInfo (TRACE_ENTER, hstmt, fSqlType));

  retcode = SQLGetTypeInfo_Internal (hstmt, fSqlType, 'A');

  LEAVE_STMT (hstmt,
    trace_SQLGetTypeInfo (TRACE_LEAVE, hstmt, fSqlType));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLGetTypeInfoA (SQLHSTMT hstmt,
    SQLSMALLINT fSqlType)
{
  ENTER_STMT (hstmt,
    trace_SQLGetTypeInfo (TRACE_ENTER, hstmt, fSqlType));

  retcode = SQLGetTypeInfo_Internal (hstmt, fSqlType, 'A');

  LEAVE_STMT (hstmt,
    trace_SQLGetTypeInfo (TRACE_LEAVE, hstmt, fSqlType));
}


SQLRETURN SQL_API
SQLGetTypeInfoW (SQLHSTMT hstmt,
    SQLSMALLINT fSqlType)
{
  ENTER_STMT (hstmt,
    trace_SQLGetTypeInfoW (TRACE_ENTER, hstmt, fSqlType));

  retcode = SQLGetTypeInfo_Internal (hstmt, fSqlType, 'W');

  LEAVE_STMT (hstmt,
    trace_SQLGetTypeInfoW (TRACE_LEAVE, hstmt, fSqlType));
}
#endif


SQLRETURN SQL_API
SQLSpecialColumns_Internal (
    SQLHSTMT hstmt,
    SQLUSMALLINT fColType,
    SQLPOINTER szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLPOINTER szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLPOINTER szTableName,
    SQLSMALLINT cbTableName,
    SQLUSMALLINT fScope,
    SQLUSMALLINT fNullable,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _TableQualifier = NULL;
  void * _TableOwner = NULL;
  void * _TableName = NULL;

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      if (fColType != SQL_BEST_ROWID && fColType != SQL_ROWVER)
	{
	  sqlstat = en_S1097;
	  break;
	}

      if (fScope != SQL_SCOPE_CURROW
	  && fScope != SQL_SCOPE_TRANSACTION
	  && fScope != SQL_SCOPE_SESSION)
	{
	  sqlstat = en_S1098;
	  break;
	}

      if (fNullable != SQL_NO_NULLS && fNullable != SQL_NULLABLE)
	{
	  sqlstat = en_S1099;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_SpecialColumns);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _TableQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *) szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *) szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *) szTableName, cbTableName);
            }
          else
            {
            /* unicode=>ansi*/
              _TableQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *)szTableName, cbTableName);
            }
          szTableQualifier = _TableQualifier;
          szTableOwner = _TableOwner;
          szTableName = _TableName;
          cbTableQualifier = SQL_NTS;
          cbTableOwner = SQL_NTS;
          cbTableName = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_SpecialColumns, (
           pstmt->dhstmt,
           fColType,
           szTableQualifier,
           cbTableQualifier,
           szTableOwner,
           cbTableOwner,
           szTableName,
           cbTableName,
           fScope,
           fNullable));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;
      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_SpecialColumns, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLSpecialColumns (SQLHSTMT hstmt,
    SQLUSMALLINT fColType,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLUSMALLINT fScope,
    SQLUSMALLINT fNullable)
{
  ENTER_STMT (hstmt,
    trace_SQLSpecialColumns (TRACE_ENTER,
	hstmt,
	fColType,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fScope,
	fNullable));

  retcode =  SQLSpecialColumns_Internal(
  	hstmt,
	fColType,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fScope,
	fNullable,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLSpecialColumns (TRACE_LEAVE,
	hstmt,
	fColType,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fScope,
	fNullable));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLSpecialColumnsA (SQLHSTMT hstmt,
    SQLUSMALLINT fColType,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLUSMALLINT fScope,
    SQLUSMALLINT fNullable)
{
  ENTER_STMT (hstmt,
    trace_SQLSpecialColumns (TRACE_ENTER,
	hstmt,
	fColType,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fScope,
	fNullable));

  retcode =  SQLSpecialColumns_Internal(
  	hstmt,
	fColType,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fScope,
	fNullable,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLSpecialColumns (TRACE_LEAVE,
	hstmt,
	fColType,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fScope,
	fNullable));
}


SQLRETURN SQL_API
SQLSpecialColumnsW (SQLHSTMT hstmt,
    SQLUSMALLINT fColType,
    SQLWCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLUSMALLINT fScope,
    SQLUSMALLINT fNullable)
{
  ENTER_STMT (hstmt,
    trace_SQLSpecialColumnsW (TRACE_ENTER,
	hstmt,
	fColType,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fScope,
	fNullable));

  retcode =  SQLSpecialColumns_Internal(
  	hstmt,
	fColType,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fScope,
	fNullable,
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLSpecialColumnsW (TRACE_LEAVE,
	hstmt,
	fColType,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fScope,
	fNullable));
}
#endif


SQLRETURN SQL_API
SQLStatistics_Internal (
    SQLHSTMT hstmt,
    SQLPOINTER szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLPOINTER szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLPOINTER szTableName,
    SQLSMALLINT cbTableName,
    SQLUSMALLINT fUnique,
    SQLUSMALLINT fAccuracy,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _TableQualifier = NULL;
  void * _TableOwner = NULL;
  void * _TableName = NULL;

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      if (fUnique != SQL_INDEX_UNIQUE && fUnique != SQL_INDEX_ALL)
	{
	  sqlstat = en_S1100;
	  break;
	}

      if (fAccuracy != SQL_ENSURE && fAccuracy != SQL_QUICK)
	{
	  sqlstat = en_S1101;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_Statistics);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}


      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _TableQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *)szTableName, cbTableName);
            }
          else
            {
            /* unicode=>ansi*/
              _TableQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *)szTableName, cbTableName);
            }
          szTableQualifier = _TableQualifier;
          szTableOwner = _TableOwner;
          szTableName = _TableName;
          cbTableQualifier = SQL_NTS;
          cbTableOwner = SQL_NTS;
          cbTableName = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_Statistics, (
           pstmt->dhstmt,
           szTableQualifier,
           cbTableQualifier,
           szTableOwner,
           cbTableOwner,
           szTableName,
           cbTableName,
           fUnique,
           fAccuracy));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_Statistics, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLStatistics (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLUSMALLINT fUnique,
    SQLUSMALLINT fAccuracy)
{
  ENTER_STMT (hstmt,
    trace_SQLStatistics (TRACE_ENTER,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fUnique,
	fAccuracy));

  retcode = SQLStatistics_Internal (
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fUnique,
	fAccuracy,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLStatistics (TRACE_LEAVE,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fUnique,
	fAccuracy));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLStatisticsA (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLUSMALLINT fUnique,
    SQLUSMALLINT fAccuracy)
{
  ENTER_STMT (hstmt,
    trace_SQLStatistics (TRACE_ENTER,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fUnique,
	fAccuracy));

  retcode = SQLStatistics_Internal (
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fUnique,
	fAccuracy,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLStatistics (TRACE_LEAVE,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fUnique,
	fAccuracy));
}


SQLRETURN SQL_API
SQLStatisticsW (SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLUSMALLINT fUnique,
    SQLUSMALLINT fAccuracy)
{
  ENTER_STMT (hstmt,
    trace_SQLStatisticsW (TRACE_ENTER,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fUnique,
	fAccuracy));

  retcode = SQLStatistics_Internal (
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fUnique,
	fAccuracy,
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLStatisticsW (TRACE_LEAVE,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	fUnique,
	fAccuracy));
}
#endif


SQLRETURN SQL_API
SQLTables_Internal (
    SQLHSTMT hstmt,
    SQLPOINTER szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLPOINTER szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLPOINTER szTableName,
    SQLSMALLINT cbTableName,
    SQLPOINTER szTableType,
    SQLSMALLINT cbTableType,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _TableQualifier = NULL;
  void * _TableOwner = NULL;
  void * _TableName = NULL;
  void * _TableType = NULL;

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS)
	  || (cbTableType < 0 && cbTableType != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_Tables);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _TableQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *)szTableName, cbTableName);
              _TableType = _iodbcdm_conv_param_A2W(pstmt, 3, (SQLCHAR *)szTableType, cbTableType);
            }
          else
            {
            /* unicode=>ansi*/
              _TableQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *)szTableName, cbTableName);
              _TableType = _iodbcdm_conv_param_W2A(pstmt, 3, (SQLWCHAR *)szTableType, cbTableType);
            }
          szTableQualifier = _TableQualifier;
          szTableOwner = _TableOwner;
          szTableName = _TableName;
          szTableType = _TableType;
          cbTableQualifier = SQL_NTS;
          cbTableOwner = SQL_NTS;
          cbTableName = SQL_NTS;
          cbTableType = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_Tables, (
           pstmt->dhstmt,
           szTableQualifier,
           cbTableQualifier,
           szTableOwner,
           cbTableOwner,
           szTableName,
           cbTableName,
           szTableType,
           cbTableType));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_Tables, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLTables (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLCHAR * szTableType,
    SQLSMALLINT cbTableType)
{
  ENTER_STMT (hstmt,
    trace_SQLTables (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szTableType, cbTableType));

  retcode =  SQLTables_Internal(
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szTableType, cbTableType,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLTables (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szTableType, cbTableType));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLTablesA (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLCHAR * szTableType,
    SQLSMALLINT cbTableType)
{
  ENTER_STMT (hstmt,
    trace_SQLTables (TRACE_ENTER,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szTableType, cbTableType));

  retcode = SQLTables_Internal (
  	hstmt,
  	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szTableType, cbTableType,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLTables (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szTableType, cbTableType));
}


SQLRETURN SQL_API
SQLTablesW (SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLWCHAR * szTableType,
    SQLSMALLINT cbTableType)
{
  ENTER_STMT (hstmt,
    trace_SQLTablesW (TRACE_ENTER,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szTableType, cbTableType));

  retcode = SQLTables_Internal (
  	hstmt,
  	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szTableType, cbTableType,
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLTablesW (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szTableType, cbTableType));
}
#endif


SQLRETURN SQL_API
SQLColumnPrivileges_Internal (
    SQLHSTMT hstmt,
    SQLPOINTER szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLPOINTER szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLPOINTER szTableName,
    SQLSMALLINT cbTableName,
    SQLPOINTER szColumnName,
    SQLSMALLINT cbColumnName,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _TableQualifier = NULL;
  void * _TableOwner = NULL;
  void * _TableName = NULL;
  void * _ColumnName = NULL;

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS)
	  || (cbColumnName < 0 && cbColumnName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_ColumnPrivileges);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}


      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _TableQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *)szTableName, cbTableName);
              _ColumnName = _iodbcdm_conv_param_A2W(pstmt, 3, (SQLCHAR *)szColumnName, cbColumnName);
            }
          else
            {
            /* unicode=>ansi*/
              _TableQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *)szTableName, cbTableName);
              _ColumnName = _iodbcdm_conv_param_W2A(pstmt, 3, (SQLWCHAR *)szColumnName, cbColumnName);
            }
          szTableQualifier = _TableQualifier;
          szTableOwner = _TableOwner;
          szTableName = _TableName;
          szColumnName = _ColumnName;
          cbTableQualifier = SQL_NTS;
          cbTableOwner = SQL_NTS;
          cbTableName = SQL_NTS;
          cbColumnName = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_ColumnPrivileges, (
           pstmt->dhstmt,
           szTableQualifier,
           cbTableQualifier,
           szTableOwner,
           cbTableOwner,
           szTableName,
           cbTableName,
           szColumnName,
           cbColumnName));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_ColumnPrivileges, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLColumnPrivileges (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLCHAR * szColumnName,
    SQLSMALLINT cbColumnName)
{
  ENTER_STMT (hstmt,
    trace_SQLColumnPrivileges (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));

  retcode = SQLColumnPrivileges_Internal(hstmt,
      szTableQualifier, cbTableQualifier,
      szTableOwner, cbTableOwner,
      szTableName, cbTableName,
      szColumnName, cbColumnName,
      'A');

  LEAVE_STMT (hstmt,
    trace_SQLColumnPrivileges (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLColumnPrivilegesA (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLCHAR * szColumnName,
    SQLSMALLINT cbColumnName)
{
  ENTER_STMT (hstmt,
    trace_SQLColumnPrivileges (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));

  retcode = SQLColumnPrivileges_Internal(hstmt,
      szTableQualifier, cbTableQualifier,
      szTableOwner, cbTableOwner,
      szTableName, cbTableName,
      szColumnName, cbColumnName,
      'A');

  LEAVE_STMT (hstmt,
    trace_SQLColumnPrivileges (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));
}


SQLRETURN SQL_API
SQLColumnPrivilegesW (SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLWCHAR * szColumnName,
    SQLSMALLINT cbColumnName)
{
  ENTER_STMT (hstmt,
    trace_SQLColumnPrivilegesW (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));

  retcode = SQLColumnPrivileges_Internal (hstmt,
      szTableQualifier, cbTableQualifier,
      szTableOwner, cbTableOwner,
      szTableName, cbTableName,
      szColumnName, cbColumnName,
      'W');

  LEAVE_STMT (hstmt,
    trace_SQLColumnPrivilegesW (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));
}
#endif


SQLRETURN SQL_API
SQLColumns_Internal (
    SQLHSTMT hstmt,
    SQLPOINTER szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLPOINTER szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLPOINTER szTableName,
    SQLSMALLINT cbTableName,
    SQLPOINTER szColumnName,
    SQLSMALLINT cbColumnName,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _TableQualifier = NULL;
  void * _TableOwner = NULL;
  void * _TableName = NULL;
  void * _ColumnName = NULL;

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS)
	  || (cbColumnName < 0 && cbColumnName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_Columns);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _TableQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *)szTableName, cbTableName);
              _ColumnName = _iodbcdm_conv_param_A2W(pstmt, 3, (SQLCHAR *)szColumnName, cbColumnName);
            }
          else
            {
            /* unicode=>ansi*/
              _TableQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *) szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *) szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *) szTableName, cbTableName);
              _ColumnName = _iodbcdm_conv_param_W2A(pstmt, 3, (SQLWCHAR *) szColumnName, cbColumnName);
            }
          szTableQualifier = _TableQualifier;
          szTableOwner = _TableOwner;
          szTableName = _TableName;
          szColumnName = _ColumnName;
          cbTableQualifier = SQL_NTS;
          cbTableOwner = SQL_NTS;
          cbTableName = SQL_NTS;
          cbColumnName = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_Columns, (
           pstmt->dhstmt,
           szTableQualifier,
           cbTableQualifier,
           szTableOwner,
           cbTableOwner,
           szTableName,
           cbTableName,
           szColumnName,
           cbColumnName));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_Columns, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLColumns (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLCHAR * szColumnName,
    SQLSMALLINT cbColumnName)
{
  ENTER_STMT (hstmt,
    trace_SQLColumns (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));

  retcode = SQLColumns_Internal (
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLColumns (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLColumnsA (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLCHAR * szColumnName,
    SQLSMALLINT cbColumnName)
{
  ENTER_STMT (hstmt,
    trace_SQLColumns (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));

  retcode = SQLColumns_Internal (
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLColumns (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));
}


SQLRETURN SQL_API
SQLColumnsW (SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName,
    SQLWCHAR * szColumnName,
    SQLSMALLINT cbColumnName)
{
  ENTER_STMT (hstmt,
    trace_SQLColumnsW (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));

  retcode = SQLColumns_Internal (
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName,
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLColumnsW (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	szColumnName, cbColumnName));
}
#endif


SQLRETURN SQL_API
SQLForeignKeys_Internal (
    SQLHSTMT hstmt,
    SQLPOINTER szPkTableQualifier,
    SQLSMALLINT cbPkTableQualifier,
    SQLPOINTER szPkTableOwner,
    SQLSMALLINT cbPkTableOwner,
    SQLPOINTER szPkTableName,
    SQLSMALLINT cbPkTableName,
    SQLPOINTER szFkTableQualifier,
    SQLSMALLINT cbFkTableQualifier,
    SQLPOINTER szFkTableOwner,
    SQLSMALLINT cbFkTableOwner,
    SQLPOINTER szFkTableName,
    SQLSMALLINT cbFkTableName,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _PkTableQualifier = NULL;
  void * _PkTableOwner = NULL;
  void * _PkTableName = NULL;
  void * _FkTableQualifier = NULL;
  void * _FkTableOwner = NULL;
  void * _FkTableName = NULL;

  for (;;)
    {
      if ((cbPkTableQualifier < 0 && cbPkTableQualifier != SQL_NTS)
	  || (cbPkTableOwner < 0 && cbPkTableOwner != SQL_NTS)
	  || (cbPkTableName < 0 && cbPkTableName != SQL_NTS)
	  || (cbFkTableQualifier < 0 && cbFkTableQualifier != SQL_NTS)
	  || (cbFkTableOwner < 0 && cbFkTableOwner != SQL_NTS)
	  || (cbFkTableName < 0 && cbFkTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_ForeignKeys);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _PkTableQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *)szPkTableQualifier, cbPkTableQualifier);
              _PkTableOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *)szPkTableOwner, cbPkTableOwner);
              _PkTableName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *)szPkTableName, cbPkTableName);
              _FkTableQualifier = _iodbcdm_conv_param_A2W(pstmt, 3, (SQLCHAR *)szFkTableQualifier, cbFkTableQualifier);
              _FkTableOwner = _iodbcdm_conv_param_A2W(pstmt, 4, (SQLCHAR *)szFkTableOwner, cbFkTableOwner);
              _FkTableName = _iodbcdm_conv_param_A2W(pstmt, 5, (SQLCHAR *)szFkTableName, cbFkTableName);
            }
          else
            {
            /* unicode=>ansi*/
              _PkTableQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *)szPkTableQualifier, cbPkTableQualifier);
              _PkTableOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *)szPkTableOwner, cbPkTableOwner);
              _PkTableName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *)szPkTableName, cbPkTableName);
              _FkTableQualifier = _iodbcdm_conv_param_W2A(pstmt, 3, (SQLWCHAR *)szFkTableQualifier, cbFkTableQualifier);
              _FkTableOwner = _iodbcdm_conv_param_W2A(pstmt, 4, (SQLWCHAR *)szFkTableOwner, cbFkTableOwner);
              _FkTableName = _iodbcdm_conv_param_W2A(pstmt, 5, (SQLWCHAR *)szFkTableName, cbFkTableName);
            }
          szPkTableQualifier = _PkTableQualifier;
          szPkTableOwner = _PkTableOwner;
          szPkTableName = _PkTableName;
          szFkTableQualifier = _FkTableQualifier;
          szFkTableOwner = _FkTableOwner;
          szFkTableName = _FkTableName;
          cbPkTableQualifier = SQL_NTS;
          cbPkTableOwner = SQL_NTS;
          cbPkTableName = SQL_NTS;
          cbFkTableQualifier = SQL_NTS;
          cbFkTableOwner = SQL_NTS;
          cbFkTableName = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_ForeignKeys, (
           pstmt->dhstmt,
           szPkTableQualifier,
           cbPkTableQualifier,
           szPkTableOwner,
           cbPkTableOwner,
           szPkTableName,
           cbPkTableName,
           szFkTableQualifier,
           cbFkTableQualifier,
           szFkTableOwner,
           cbFkTableOwner,
           szFkTableName,
           cbFkTableName));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_ForeignKeys, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLForeignKeys (SQLHSTMT hstmt,
    SQLCHAR * szPkTableQualifier,
    SQLSMALLINT cbPkTableQualifier,
    SQLCHAR * szPkTableOwner,
    SQLSMALLINT cbPkTableOwner,
    SQLCHAR * szPkTableName,
    SQLSMALLINT cbPkTableName,
    SQLCHAR * szFkTableQualifier,
    SQLSMALLINT cbFkTableQualifier,
    SQLCHAR * szFkTableOwner,
    SQLSMALLINT cbFkTableOwner,
    SQLCHAR * szFkTableName,
    SQLSMALLINT cbFkTableName)
{
  ENTER_STMT (hstmt,
    trace_SQLForeignKeys (TRACE_ENTER,
  	hstmt,
  	szPkTableQualifier, cbPkTableQualifier,
	szPkTableOwner, cbPkTableOwner,
	szPkTableName, cbPkTableName,
	szFkTableQualifier, cbFkTableQualifier,
	szFkTableOwner, cbFkTableOwner,
	szFkTableName, cbFkTableName));

  retcode = SQLForeignKeys_Internal(
  	hstmt,
  	szPkTableQualifier, cbPkTableQualifier,
	szPkTableOwner, cbPkTableOwner,
	szPkTableName, cbPkTableName,
	szFkTableQualifier, cbFkTableQualifier,
	szFkTableOwner, cbFkTableOwner,
	szFkTableName, cbFkTableName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLForeignKeys (TRACE_LEAVE,
  	hstmt,
  	szPkTableQualifier, cbPkTableQualifier,
	szPkTableOwner, cbPkTableOwner,
	szPkTableName, cbPkTableName,
	szFkTableQualifier, cbFkTableQualifier,
	szFkTableOwner, cbFkTableOwner,
	szFkTableName, cbFkTableName));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLForeignKeysA (SQLHSTMT hstmt,
    SQLCHAR * szPkTableQualifier,
    SQLSMALLINT cbPkTableQualifier,
    SQLCHAR * szPkTableOwner,
    SQLSMALLINT cbPkTableOwner,
    SQLCHAR * szPkTableName,
    SQLSMALLINT cbPkTableName,
    SQLCHAR * szFkTableQualifier,
    SQLSMALLINT cbFkTableQualifier,
    SQLCHAR * szFkTableOwner,
    SQLSMALLINT cbFkTableOwner,
    SQLCHAR * szFkTableName,
    SQLSMALLINT cbFkTableName)
{
  ENTER_STMT (hstmt,
    trace_SQLForeignKeys (TRACE_ENTER,
  	hstmt,
  	szPkTableQualifier, cbPkTableQualifier,
	szPkTableOwner, cbPkTableOwner,
	szPkTableName, cbPkTableName,
	szFkTableQualifier, cbFkTableQualifier,
	szFkTableOwner, cbFkTableOwner,
	szFkTableName, cbFkTableName));

  retcode = SQLForeignKeys_Internal(
  	hstmt,
  	szPkTableQualifier, cbPkTableQualifier,
	szPkTableOwner, cbPkTableOwner,
	szPkTableName, cbPkTableName,
	szFkTableQualifier, cbFkTableQualifier,
	szFkTableOwner, cbFkTableOwner,
	szFkTableName, cbFkTableName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLForeignKeys (TRACE_LEAVE,
  	hstmt,
  	szPkTableQualifier, cbPkTableQualifier,
	szPkTableOwner, cbPkTableOwner,
	szPkTableName, cbPkTableName,
	szFkTableQualifier, cbFkTableQualifier,
	szFkTableOwner, cbFkTableOwner,
	szFkTableName, cbFkTableName));
}


SQLRETURN SQL_API
SQLForeignKeysW (SQLHSTMT hstmt,
    SQLWCHAR * szPkTableQualifier,
    SQLSMALLINT cbPkTableQualifier,
    SQLWCHAR * szPkTableOwner,
    SQLSMALLINT cbPkTableOwner,
    SQLWCHAR * szPkTableName,
    SQLSMALLINT cbPkTableName,
    SQLWCHAR * szFkTableQualifier,
    SQLSMALLINT cbFkTableQualifier,
    SQLWCHAR * szFkTableOwner,
    SQLSMALLINT cbFkTableOwner,
    SQLWCHAR * szFkTableName,
    SQLSMALLINT cbFkTableName)
{
  ENTER_STMT (hstmt,
    trace_SQLForeignKeysW (TRACE_ENTER,
  	hstmt,
  	szPkTableQualifier, cbPkTableQualifier,
	szPkTableOwner, cbPkTableOwner,
	szPkTableName, cbPkTableName,
	szFkTableQualifier, cbFkTableQualifier,
	szFkTableOwner, cbFkTableOwner,
	szFkTableName, cbFkTableName));

  retcode = SQLForeignKeys_Internal(
  	hstmt,
  	szPkTableQualifier, cbPkTableQualifier,
	szPkTableOwner, cbPkTableOwner,
	szPkTableName, cbPkTableName,
	szFkTableQualifier, cbFkTableQualifier,
	szFkTableOwner, cbFkTableOwner,
	szFkTableName, cbFkTableName,
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLForeignKeysW (TRACE_LEAVE,
  	hstmt,
  	szPkTableQualifier, cbPkTableQualifier,
	szPkTableOwner, cbPkTableOwner,
	szPkTableName, cbPkTableName,
	szFkTableQualifier, cbFkTableQualifier,
	szFkTableOwner, cbFkTableOwner,
	szFkTableName, cbFkTableName));
}
#endif


SQLRETURN SQL_API
SQLPrimaryKeys_Internal (
    SQLHSTMT hstmt,
    SQLPOINTER szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLPOINTER szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLPOINTER szTableName,
    SQLSMALLINT cbTableName,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _TableQualifier = NULL;
  void * _TableOwner = NULL;
  void * _TableName = NULL;

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_PrimaryKeys);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _TableQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *)szTableName, cbTableName);
            }
          else
            {
            /* unicode=>ansi*/
              _TableQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *)szTableName, cbTableName);
            }
          szTableQualifier = _TableQualifier;
          szTableOwner = _TableOwner;
          szTableName = _TableName;
          cbTableQualifier = SQL_NTS;
          cbTableOwner = SQL_NTS;
          cbTableName = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_PrimaryKeys, (
           pstmt->dhstmt,
           szTableQualifier,
           cbTableQualifier,
           szTableOwner,
           cbTableOwner,
           szTableName,
           cbTableName));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_PrimaryKeys, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLPrimaryKeys (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName)
{
  ENTER_STMT (hstmt,
    trace_SQLPrimaryKeys (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName));

  retcode = SQLPrimaryKeys_Internal (
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLPrimaryKeys (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLPrimaryKeysA (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName)
{
  ENTER_STMT (hstmt,
    trace_SQLPrimaryKeys (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName));

  retcode = SQLPrimaryKeys_Internal (
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLPrimaryKeys (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName));
}


SQLRETURN SQL_API
SQLPrimaryKeysW (SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName)
{
  ENTER_STMT (hstmt,
    trace_SQLPrimaryKeysW (TRACE_ENTER,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName));

  retcode = SQLPrimaryKeys_Internal (
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName,
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLPrimaryKeysW (TRACE_LEAVE,
	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName, cbTableName));
}
#endif


SQLRETURN SQL_API
SQLProcedureColumns_Internal (
    SQLHSTMT hstmt,
    SQLPOINTER szProcQualifier,
    SQLSMALLINT cbProcQualifier,
    SQLPOINTER szProcOwner,
    SQLSMALLINT cbProcOwner,
    SQLPOINTER szProcName,
    SQLSMALLINT cbProcName,
    SQLPOINTER szColumnName,
    SQLSMALLINT cbColumnName,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _ProcQualifier = NULL;
  void * _ProcOwner = NULL;
  void * _ProcName = NULL;
  void * _ColumnName = NULL;

  for (;;)
    {
      if ((cbProcQualifier < 0 && cbProcQualifier != SQL_NTS)
	  || (cbProcOwner < 0 && cbProcOwner != SQL_NTS)
	  || (cbProcName < 0 && cbProcName != SQL_NTS)
	  || (cbColumnName < 0 && cbColumnName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_ProcedureColumns);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _ProcQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *)szProcQualifier, cbProcQualifier);
              _ProcOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *)szProcOwner, cbProcOwner);
              _ProcName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *)szProcName, cbProcName);
              _ColumnName = _iodbcdm_conv_param_A2W(pstmt, 3, (SQLCHAR *)szColumnName, cbColumnName);
            }
          else
            {
            /* unicode=>ansi*/
              _ProcQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *)szProcQualifier, cbProcQualifier);
              _ProcOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *)szProcOwner, cbProcOwner);
              _ProcName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *)szProcName, cbProcName);
              _ColumnName = _iodbcdm_conv_param_W2A(pstmt, 3, (SQLWCHAR *)szColumnName, cbColumnName);
            }
          szProcQualifier = _ProcQualifier;
          szProcOwner = _ProcOwner;
          szProcName = _ProcName;
          szColumnName = _ColumnName;
          cbProcQualifier = SQL_NTS;
          cbProcOwner = SQL_NTS;
          cbProcName = SQL_NTS;
          cbColumnName = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_ProcedureColumns, (
           pstmt->dhstmt,
           szProcQualifier,
           cbProcQualifier,
           szProcOwner,
           cbProcOwner,
           szProcName,
           cbProcName,
           szColumnName,
           cbColumnName));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_ProcedureColumns, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLProcedureColumns (SQLHSTMT hstmt,
    SQLCHAR * szProcQualifier,
    SQLSMALLINT cbProcQualifier,
    SQLCHAR * szProcOwner,
    SQLSMALLINT cbProcOwner,
    SQLCHAR * szProcName,
    SQLSMALLINT cbProcName,
    SQLCHAR * szColumnName,
    SQLSMALLINT cbColumnName)
{
  ENTER_STMT (hstmt,
    trace_SQLProcedureColumns (TRACE_ENTER,
    	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	szColumnName, cbColumnName));

  retcode = SQLProcedureColumns_Internal (
  	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	szColumnName, cbColumnName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLProcedureColumns (TRACE_LEAVE,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	szColumnName, cbColumnName));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLProcedureColumnsA (SQLHSTMT hstmt,
    SQLCHAR * szProcQualifier,
    SQLSMALLINT cbProcQualifier,
    SQLCHAR * szProcOwner,
    SQLSMALLINT cbProcOwner,
    SQLCHAR * szProcName,
    SQLSMALLINT cbProcName,
    SQLCHAR * szColumnName,
    SQLSMALLINT cbColumnName)
{
  ENTER_STMT (hstmt,
    trace_SQLProcedureColumns (TRACE_ENTER,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	szColumnName, cbColumnName));

  retcode = SQLProcedureColumns_Internal (
  	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	szColumnName, cbColumnName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLProcedureColumns (TRACE_LEAVE,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	szColumnName, cbColumnName));
}


SQLRETURN SQL_API
SQLProcedureColumnsW (SQLHSTMT hstmt,
    SQLWCHAR * szProcQualifier,
    SQLSMALLINT cbProcQualifier,
    SQLWCHAR * szProcOwner,
    SQLSMALLINT cbProcOwner,
    SQLWCHAR * szProcName,
    SQLSMALLINT cbProcName,
    SQLWCHAR * szColumnName,
    SQLSMALLINT cbColumnName)
{
  ENTER_STMT (hstmt,
    trace_SQLProcedureColumnsW (TRACE_ENTER,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	szColumnName, cbColumnName));

  retcode = SQLProcedureColumns_Internal (
  	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	szColumnName, cbColumnName,
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLProcedureColumnsW (TRACE_LEAVE,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	szColumnName, cbColumnName));
}
#endif


SQLRETURN SQL_API
SQLProcedures_Internal (
    SQLHSTMT hstmt,
    SQLPOINTER szProcQualifier,
    SQLSMALLINT cbProcQualifier,
    SQLPOINTER szProcOwner,
    SQLSMALLINT cbProcOwner,
    SQLPOINTER szProcName,
    SQLSMALLINT cbProcName,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _ProcQualifier = NULL;
  void * _ProcOwner = NULL;
  void * _ProcName = NULL;

  for (;;)
    {
      if ((cbProcQualifier < 0 && cbProcQualifier != SQL_NTS)
	  || (cbProcOwner < 0 && cbProcOwner != SQL_NTS)
	  || (cbProcName < 0 && cbProcName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_Procedures);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _ProcQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *)szProcQualifier, cbProcQualifier);
              _ProcOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *)szProcOwner, cbProcOwner);
              _ProcName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *)szProcName, cbProcName);
            }
          else
            {
            /* unicode=>ansi*/
              _ProcQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *)szProcQualifier, cbProcQualifier);
              _ProcOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *)szProcOwner, cbProcOwner);
              _ProcName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *)szProcName, cbProcName);
            }
          szProcQualifier = _ProcQualifier;
          szProcOwner = _ProcOwner;
          szProcName = _ProcName;
          cbProcQualifier = SQL_NTS;
          cbProcOwner = SQL_NTS;
          cbProcName = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_Procedures, (
           pstmt->dhstmt,
           szProcQualifier,
           cbProcQualifier,
           szProcOwner,
           cbProcOwner,
           szProcName,
           cbProcName));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_Procedures, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLProcedures (SQLHSTMT hstmt,
    SQLCHAR * szProcQualifier,
    SQLSMALLINT cbProcQualifier,
    SQLCHAR * szProcOwner,
    SQLSMALLINT cbProcOwner,
    SQLCHAR * szProcName,
    SQLSMALLINT cbProcName)
{
  ENTER_STMT (hstmt,
    trace_SQLProcedures (TRACE_ENTER,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName));

  retcode = SQLProcedures_Internal (
  	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLProcedures (TRACE_LEAVE,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLProceduresA (SQLHSTMT hstmt,
    SQLCHAR * szProcQualifier,
    SQLSMALLINT cbProcQualifier,
    SQLCHAR * szProcOwner,
    SQLSMALLINT cbProcOwner,
    SQLCHAR * szProcName,
    SQLSMALLINT cbProcName)
{
  ENTER_STMT (hstmt,
    trace_SQLProcedures (TRACE_ENTER,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName));

  retcode = SQLProcedures_Internal (
  	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLProcedures (TRACE_LEAVE,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName));
}


SQLRETURN SQL_API
SQLProceduresW (SQLHSTMT hstmt,
    SQLWCHAR * szProcQualifier,
    SQLSMALLINT cbProcQualifier,
    SQLWCHAR * szProcOwner,
    SQLSMALLINT cbProcOwner,
    SQLWCHAR * szProcName,
    SQLSMALLINT cbProcName)
{
  ENTER_STMT (hstmt,
    trace_SQLProceduresW (TRACE_ENTER,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName));

  retcode = SQLProcedures_Internal (
  	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName,
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLProceduresW (TRACE_LEAVE,
	hstmt,
	szProcQualifier, cbProcQualifier,
	szProcOwner, cbProcOwner,
	szProcName, cbProcName));
}
#endif


SQLRETURN SQL_API
SQLTablePrivileges_Internal (
    SQLHSTMT hstmt,
    SQLPOINTER szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLPOINTER szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLPOINTER szTableName,
    SQLSMALLINT cbTableName,
    SQLCHAR waMode)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  void * _TableQualifier = NULL;
  void * _TableOwner = NULL;
  void * _TableName = NULL;

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (pstmt, en_TablePrivileges);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      if ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              _TableQualifier = _iodbcdm_conv_param_A2W(pstmt, 0, (SQLCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_A2W(pstmt, 1, (SQLCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_A2W(pstmt, 2, (SQLCHAR *)szTableName, cbTableName);
            }
          else
            {
            /* unicode=>ansi*/
              _TableQualifier = _iodbcdm_conv_param_W2A(pstmt, 0, (SQLWCHAR *)szTableQualifier, cbTableQualifier);
              _TableOwner = _iodbcdm_conv_param_W2A(pstmt, 1, (SQLWCHAR *)szTableOwner, cbTableOwner);
              _TableName = _iodbcdm_conv_param_W2A(pstmt, 2, (SQLWCHAR *)szTableName, cbTableName);
            }
          szTableQualifier = _TableQualifier;
          szTableOwner = _TableOwner;
          szTableName = _TableName;
          cbTableQualifier = SQL_NTS;
          cbTableOwner = SQL_NTS;
          cbTableName = SQL_NTS;
        }

      CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc, penv->unicode_driver,
        en_TablePrivileges, (
           pstmt->dhstmt,
           szTableQualifier,
           cbTableQualifier,
           szTableOwner,
           cbTableOwner,
           szTableName,
           cbTableName));

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (retcode != SQL_STILL_EXECUTING)
    _iodbcdm_FreeStmtParams(pstmt);

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_cata_state_tr (pstmt, en_TablePrivileges, retcode);
  return retcode;
}


SQLRETURN SQL_API
SQLTablePrivileges (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName)
{
  ENTER_STMT (hstmt,
    trace_SQLTablePrivileges (TRACE_ENTER,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName,cbTableName));

  retcode = SQLTablePrivileges_Internal(
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName,cbTableName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLTablePrivileges (TRACE_LEAVE,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName,cbTableName));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLTablePrivilegesA (SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLCHAR * szTableName,
    SQLSMALLINT cbTableName)
{
  ENTER_STMT (hstmt,
    trace_SQLTablePrivileges (TRACE_ENTER,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName,cbTableName));

  retcode = SQLTablePrivileges_Internal(
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName,cbTableName,
	'A');

  LEAVE_STMT (hstmt,
    trace_SQLTablePrivileges (TRACE_LEAVE,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName,cbTableName));
}


SQLRETURN SQL_API
SQLTablePrivilegesW (SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner,
    SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName)
{
  ENTER_STMT (hstmt,
    trace_SQLTablePrivilegesW (TRACE_ENTER,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName,cbTableName));

  retcode = SQLTablePrivileges_Internal(
  	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName,cbTableName,
	'W');

  LEAVE_STMT (hstmt,
    trace_SQLTablePrivilegesW (TRACE_LEAVE,
    	hstmt,
	szTableQualifier, cbTableQualifier,
	szTableOwner, cbTableOwner,
	szTableName,cbTableName));
}
#endif
