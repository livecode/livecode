/*
 *  hdbc.c
 *
 *  $Id: hdbc.c,v 1.30 2006/01/20 15:58:34 source Exp $
 *
 *  Data source connect object management functions
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
#include <iodbcext.h>

#include <unicode.h>

#include <dlproc.h>

#include <herr.h>
#include <henv.h>
#include <hdbc.h>
#include <hstmt.h>

#include <itrace.h>
#include <stdio.h>


extern SQLRETURN _iodbcdm_driverunload (HDBC hdbc);

static SQLRETURN
_iodbcdm_drvopt_store (SQLHDBC hdbc, SQLUSMALLINT fOption, SQLULEN vParam,
	SQLCHAR waMode)
{
  CONN (pdbc, hdbc);
  DRVOPT *popt;

  /*
   *  Check if this option is already registered
   */
  for (popt = pdbc->drvopt; popt != NULL; popt = popt->next)
    {
      if (popt->Option == fOption)
	break;
    }

  /*
   *  New option
   */
  if (popt == NULL)
    {
      if ((popt = (DRVOPT *) MEM_ALLOC (sizeof (DRVOPT))) == NULL)
	return SQL_ERROR;

      popt->Option = fOption;
      popt->next = pdbc->drvopt;
      pdbc->drvopt = popt;
    }

  /*
   *  Store the value
   */
  popt->Param = vParam;
  popt->waMode = waMode;

  return SQL_SUCCESS;
}


static SQLRETURN
_iodbcdm_drvopt_free (SQLHDBC hdbc)
{
  CONN (pdbc, hdbc);
  DRVOPT *popt;

  popt = pdbc->drvopt;
  while (popt != NULL)
    {
      DRVOPT *tmp = popt->next;
      free (popt);
      popt = tmp;
    }
  pdbc->drvopt = NULL;

  return SQL_SUCCESS;
}


SQLRETURN 
SQLAllocConnect_Internal (
    SQLHENV henv,
    SQLHDBC * phdbc)
{
  GENV (genv, henv);
  CONN (pdbc, NULL);

  if (phdbc == NULL)
    {
      PUSHSQLERR (genv->herr, en_S1009);
      return SQL_ERROR;
    }

  pdbc = (DBC_t *) MEM_ALLOC (sizeof (DBC_t));

  if (pdbc == NULL)
    {
      *phdbc = SQL_NULL_HDBC;

      PUSHSQLERR (genv->herr, en_S1001);
      return SQL_ERROR;
    }
  pdbc->rc = 0;

  /*
   *  Initialize this handle
   */
  pdbc->type = SQL_HANDLE_DBC;

  /* insert this dbc entry into the link list */
  pdbc->next = (DBC_t *) genv->hdbc;
  genv->hdbc = pdbc;
#if (ODBCVER >= 0x0300)
  if (genv->odbc_ver == 0)
    genv->odbc_ver = SQL_OV_ODBC2;
  pdbc->hdesc = NULL;
#endif
  pdbc->genv = genv;

  pdbc->henv = SQL_NULL_HENV;
  pdbc->hstmt = SQL_NULL_HSTMT;
  pdbc->herr = SQL_NULL_HERR;
  pdbc->dhdbc = SQL_NULL_HDBC;
  pdbc->state = en_dbc_allocated;
  pdbc->drvopt = NULL;
  pdbc->dbc_cip = 0;
  pdbc->err_rec = 0;

  /* set connect options to default values */
  pdbc->access_mode = SQL_MODE_DEFAULT;
  pdbc->autocommit = SQL_AUTOCOMMIT_DEFAULT;
  pdbc->current_qualifier = NULL;
  pdbc->login_timeout = 0UL;
  pdbc->odbc_cursors = SQL_CUR_DEFAULT;
  pdbc->packet_size = 0UL;
  pdbc->quiet_mode = (UDWORD) NULL;
  pdbc->txn_isolation = SQL_TXN_READ_UNCOMMITTED;
  pdbc->cb_commit = (SWORD) SQL_CB_DELETE;
  pdbc->cb_rollback = (SWORD) SQL_CB_DELETE;

  *phdbc = (SQLHDBC) pdbc;

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLAllocConnect (SQLHENV henv, SQLHDBC * phdbc)
{
  GENV (genv, henv);
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();
  if (!IS_VALID_HENV (genv))
    {
      ODBC_UNLOCK ();
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (genv);

  TRACE (trace_SQLAllocConnect (TRACE_ENTER, henv, phdbc));

  retcode = SQLAllocConnect_Internal (henv, phdbc);

  TRACE (trace_SQLAllocConnect (TRACE_LEAVE, henv, phdbc));

  ODBC_UNLOCK ();

  return SQL_SUCCESS;
}


SQLRETURN
SQLFreeConnect_Internal (SQLHDBC hdbc)
{
  CONN (pdbc, hdbc);
  GENV (genv, pdbc->genv);
  CONN (tpdbc, NULL);

  /* check state */
  if (pdbc->state != en_dbc_allocated)
    {
      PUSHSQLERR (pdbc->herr, en_S1010);
      return SQL_ERROR;
    }

  for (tpdbc = (DBC_t *) genv->hdbc; tpdbc != NULL; tpdbc = tpdbc->next)
    {
      if (pdbc == tpdbc)
	{
	  genv->hdbc = pdbc->next;
	  break;
	}

      if (pdbc == tpdbc->next)
	{
	  tpdbc->next = pdbc->next;
	  break;
	}
    }

  /* free this dbc */
  _iodbcdm_driverunload (pdbc);

  /* free driver connect options */
  _iodbcdm_drvopt_free (pdbc);
   
  /*
   *  Invalidate this handle
   */
  pdbc->type = 0;

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLFreeConnect (SQLHDBC hdbc)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLFreeConnect (TRACE_ENTER, hdbc));

  retcode = SQLFreeConnect_Internal (hdbc);

  LEAVE_HDBC (hdbc, 1,
    trace_SQLFreeConnect (TRACE_LEAVE, hdbc);
    MEM_FREE(hdbc);
  );
}


SQLRETURN
_iodbcdm_SetConnectOption (
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fOption,
  SQLULEN		  vParam,
  SQLCHAR		  waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, pdbc->henv);
  STMT (pstmt, NULL);
  HPROC hproc = SQL_NULL_HPROC;
  sqlstcode_t sqlstat = en_00000;
  SQLRETURN retcode = SQL_SUCCESS;

#if (ODBCVER < 0x0300)
  /* check option */
  if (fOption < SQL_CONN_OPT_MIN ||
      (fOption > SQL_CONN_OPT_MAX && fOption < SQL_CONNECT_OPT_DRVR_START))
    {
      PUSHSQLERR (pdbc->herr, en_S1092);

      return SQL_ERROR;
    }
#endif

  /* check state of connection handle */
  switch (pdbc->state)
    {
    case en_dbc_allocated:
      if (fOption == SQL_TRANSLATE_DLL || fOption == SQL_TRANSLATE_OPTION)
	{
	  /* This two options are only meaningful
	   * for specified driver. So, has to be
	   * set after a driver has been loaded.
	   */
	  sqlstat = en_08003;
	  break;
	}

      /*
       *  An option only meaningful for the driver is passed before the 
       *  driver was actually loaded. We save it here and pass it onto 
       *  the driver at a later stage.
       */
      if (fOption >= SQL_CONNECT_OPT_DRVR_START && pdbc->henv == SQL_NULL_HENV)
        _iodbcdm_drvopt_store (hdbc, fOption, vParam, waMode);

      break;

    case en_dbc_needdata:
      sqlstat = en_S1010;
      break;

    case en_dbc_connected:
    case en_dbc_hstmt:
      if (fOption == SQL_ODBC_CURSORS)
	{
	  sqlstat = en_08002;
	}
      break;

    default:
      break;
    }

  /* check state of statement handle(s) */
  for (pstmt = (STMT_t *) pdbc->hstmt;
      pstmt != NULL && sqlstat == en_00000;
      pstmt = (STMT_t *) pstmt->next)
    {
      if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
	{
	  sqlstat = en_S1010;
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);

      return SQL_ERROR;
    }

#if (ODBCVER >= 0x0300)
  if (fOption == SQL_OPT_TRACE || fOption == SQL_ATTR_TRACE)
#else
  if (fOption == SQL_OPT_TRACE)
#endif
    /* tracing flag can be set before and after connect 
     * and only meaningful for driver manager(actually
     * there is only one tracing file under one global
     * environment).
     */
    {
      switch ((SQLUINTEGER)vParam)
	{
	case SQL_OPT_TRACE_ON:
	  trace_start ();
	  break;

	case SQL_OPT_TRACE_OFF:
	  trace_stop ();
	  break;

	default:
	  PUSHSQLERR (pdbc->herr, en_HY024);
	  return SQL_ERROR;
	}                              	

      if (sqlstat != en_00000)
	{
	  PUSHSQLERR (pdbc->herr, sqlstat);
	}

      return retcode;
    }

#if (ODBCVER >= 0x0300)
  if (fOption == SQL_OPT_TRACEFILE || fOption == SQL_ATTR_TRACEFILE)
#else
  if (fOption == SQL_OPT_TRACEFILE)
#endif
    /* Tracing file can be set before and after connect 
     * and only meaningful for driver manager. 
     */
    {
      SQLCHAR *_vParam;
      SQLCHAR *tmp = NULL;

      if (((char *)vParam) == NULL 
          || (waMode != 'W' && ((char *) vParam)[0] == '\0')
          || (waMode == 'W' && ((wchar_t *) vParam)[0] == L'\0'))
	{
	  PUSHSQLERR (pdbc->herr, en_S1009);
	  return SQL_ERROR;
	}

      _vParam = (SQLCHAR *)vParam;
      if (waMode == 'W')
        {
          if ((_vParam = tmp = dm_SQL_WtoU8((wchar_t *)vParam, SQL_NTS)) == NULL)
	    {
              PUSHSQLERR (pdbc->herr, en_S1001);
              return SQL_ERROR;
            }
        }

      if (ODBCSharedTraceFlag)
	{
	  PUSHSQLERR (pdbc->herr, en_IM013);
	  return SQL_ERROR;
	}

      trace_set_filename ((char *) _vParam);
      MEM_FREE (tmp);
      return SQL_SUCCESS;
    }

  if (pdbc->state != en_dbc_allocated)
    {
      /* If already connected, then, driver's odbc call
       * will be invoked. Otherwise, we only save the options
       * and delay the setting process until the connection 
       * been established.  
       */
     void * _vParam = NULL;
     int procid = 0;

     if ((penv->unicode_driver && waMode != 'W') 
        || (!penv->unicode_driver && waMode == 'W'))
      {
        switch (fOption)
          {
          case SQL_ATTR_TRACEFILE:
          case SQL_CURRENT_QUALIFIER:
          case SQL_TRANSLATE_DLL:
          case SQL_APPLICATION_NAME:
          case SQL_COPT_SS_ENLIST_IN_DTC:
          case SQL_COPT_SS_PERF_QUERY_LOG:
          case SQL_COPT_SS_PERF_DATA_LOG:
          case SQL_CURRENT_SCHEMA:
            if (waMode != 'W')
              {
              /* ansi=>unicode*/
                _vParam = dm_SQL_A2W((SQLCHAR *)vParam, SQL_NTS);
              }
            else
              {
              /* unicode=>ansi*/
                _vParam = dm_SQL_W2A((SQLWCHAR *)vParam, SQL_NTS);
              }
            vParam = (SQLULEN)_vParam;
            break;
          }
      }

#if (ODBCVER >= 0x0300)
     if (penv->unicode_driver)
       {
         /* SQL_XXX_W */
         if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectAttrW))
             != SQL_NULL_HPROC)
           procid = en_SetConnectAttrW;
       }
     else
       {
         /* SQL_XXX */
         /* SQL_XXX_A */
         if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectAttr))
             != SQL_NULL_HPROC)
           procid = en_SetConnectAttr;
         else
         if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectAttrA))
             != SQL_NULL_HPROC)
           procid = en_SetConnectAttrA;
       }

      if (hproc != SQL_NULL_HPROC)
	{
	  switch (fOption)
	    {
	      /* integer attributes */
	    case SQL_ATTR_ACCESS_MODE:
	    case SQL_ATTR_AUTOCOMMIT:
	    case SQL_ATTR_LOGIN_TIMEOUT:
	    case SQL_ATTR_ODBC_CURSORS:
	    case SQL_ATTR_PACKET_SIZE:
	    case SQL_ATTR_QUIET_MODE:
	    case SQL_ATTR_TRANSLATE_OPTION:
	    case SQL_ATTR_TXN_ISOLATION:
	      CALL_DRIVER (hdbc, pdbc, retcode, hproc, procid,
		  (pdbc->dhdbc, fOption, vParam, 0));
	      break;

	      /* ODBC3 defined options */
	    case SQL_ATTR_ASYNC_ENABLE:
	    case SQL_ATTR_AUTO_IPD:
	    case SQL_ATTR_CONNECTION_DEAD:
	    case SQL_ATTR_CONNECTION_TIMEOUT:
	    case SQL_ATTR_METADATA_ID:
	      PUSHSQLERR (pdbc->herr, en_IM001);
	      MEM_FREE(_vParam);
	      return SQL_ERROR;

	    default:		/* string & driver defined */

	      CALL_DRIVER (hdbc, pdbc, retcode, hproc, procid,
		  (pdbc->dhdbc, fOption, vParam, SQL_NTS));
	    }
	}
      else
#endif
	{
          if (penv->unicode_driver)
            {
             /* SQL_XXX_W */
             if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectOptionW))
                 != SQL_NULL_HPROC)
               procid = en_SetConnectOptionW;
            }
          else
            {
             /* SQL_XXX */
             /* SQL_XXX_A */
             if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectOption))
                != SQL_NULL_HPROC)
               procid = en_SetConnectOption;
             else
             if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectOptionA))
                != SQL_NULL_HPROC)
               procid = en_SetConnectOptionA;
            }

	  if (hproc == SQL_NULL_HPROC)
	    {
	      PUSHSQLERR (pdbc->herr, en_IM001);
	      MEM_FREE(_vParam);
	      return SQL_ERROR;
	    }

	  CALL_DRIVER (hdbc, pdbc, retcode, hproc, procid,
	      (pdbc->dhdbc, fOption, vParam));
	}

      MEM_FREE(_vParam);
      if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
	  return retcode;
	}
    }

  /* 
   * Now, either driver's odbc call was successful or
   * driver has not been loaded yet. In the first case, we
   * need flip flag for(such as access_mode, autocommit, ...)
   * for our finit state machine. While in the second case, 
   * we need save option values(such as current_qualifier, ...)
   * for delayed setting. So, ...
   */

  /* No matter what state we are(i.e. allocated or connected, ..)
   * we need to flip the flag.
   */
  switch (fOption)
    {
    case SQL_ACCESS_MODE:
      pdbc->access_mode = vParam;
      break;

    case SQL_AUTOCOMMIT:
      pdbc->autocommit = vParam;
      break;
    }

  /* state transition */
  if (pdbc->state != en_dbc_allocated)
    {
      return retcode;
    }

  /* Only 'allocated' state is possible here, and we need to
   * save the options for delayed setting.
   */
  switch (fOption)
    {
    case SQL_CURRENT_QUALIFIER:
      if (pdbc->current_qualifier != NULL)
	{
	  MEM_FREE (pdbc->current_qualifier);
	}

      if (vParam == 0UL)
	{
	  pdbc->current_qualifier = NULL;
	  break;
	}

      if (waMode != 'W')
        pdbc->current_qualifier = (wchar_t *) MEM_ALLOC (STRLEN (vParam) + 1);
      else
        pdbc->current_qualifier = (wchar_t *) MEM_ALLOC((WCSLEN (vParam) + 1) 
        	* sizeof(wchar_t));

      if (pdbc->current_qualifier == NULL)
	{
	  PUSHSQLERR (pdbc->herr, en_S1001);
	  return SQL_ERROR;
	}

      if (waMode != 'W')
        STRCPY ((char *)pdbc->current_qualifier, vParam);
      else
        WCSCPY ((wchar_t *)pdbc->current_qualifier, vParam);

      pdbc->current_qualifier_WA = waMode;
      break;

    case SQL_LOGIN_TIMEOUT:
      pdbc->login_timeout = vParam;
      break;

    case SQL_ODBC_CURSORS:
      pdbc->odbc_cursors = vParam;
      break;

    case SQL_PACKET_SIZE:
      pdbc->packet_size = vParam;
      break;

    case SQL_QUIET_MODE:
      pdbc->quiet_mode = vParam;
      break;

    case SQL_TXN_ISOLATION:
      pdbc->txn_isolation = vParam;
      break;

    default:
      /* Since we didn't save the option value for delayed
       * setting, we should raise an error here.
       */
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLSetConnectOption (
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fOption,
  SQLULEN		  vParam)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLSetConnectOption (TRACE_ENTER, hdbc, fOption, vParam));

  retcode = _iodbcdm_SetConnectOption (hdbc, fOption, vParam, 'A');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLSetConnectOption (TRACE_LEAVE, hdbc, fOption, vParam));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLSetConnectOptionA (
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fOption,
  SQLULEN		  vParam)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLSetConnectOption (TRACE_ENTER, hdbc, fOption, vParam));

  retcode = _iodbcdm_SetConnectOption (hdbc, fOption, vParam, 'A');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLSetConnectOption (TRACE_LEAVE, hdbc, fOption, vParam));
}


SQLRETURN SQL_API
SQLSetConnectOptionW (
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fOption,
  SQLULEN		  vParam)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLSetConnectOptionW (TRACE_ENTER, hdbc, fOption, vParam));

  retcode = _iodbcdm_SetConnectOption (hdbc, fOption, vParam, 'W');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLSetConnectOptionW (TRACE_LEAVE, hdbc, fOption, vParam));
}
#endif


SQLRETURN SQL_API
_iodbcdm_GetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam,
    SQLCHAR waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  sqlstcode_t sqlstat = en_00000;
  SQLRETURN retcode = SQL_SUCCESS;

#if (ODBCVER < 0x0300)
  /* check option */
  if (fOption < SQL_CONN_OPT_MIN ||
      (fOption > SQL_CONN_OPT_MAX && fOption < SQL_CONNECT_OPT_DRVR_START))
    {
      PUSHSQLERR (pdbc->herr, en_S1092);

      return SQL_ERROR;
    }
#endif

  /* check state */
  switch (pdbc->state)
    {
    case en_dbc_allocated:
      if (fOption != SQL_ACCESS_MODE
	  && fOption != SQL_AUTOCOMMIT
	  && fOption != SQL_LOGIN_TIMEOUT
	  && fOption != SQL_OPT_TRACE
	  && fOption != SQL_OPT_TRACEFILE)
	{
	  sqlstat = en_08003;
	}
      /* MS ODBC SDK document only
       * allows SQL_ACCESS_MODE
       * and SQL_AUTOCOMMIT in this
       * dbc state. We allow another 
       * two options, because they 
       * are only meaningful for driver 
       * manager.  
       */
      break;

    case en_dbc_needdata:
      sqlstat = en_S1010;
      break;

    default:
      break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);
      return SQL_ERROR;
    }

  /* Tracing and tracing file options are only 
   * meaningful for driver manager
   */
#if (ODBCVER >= 0x0300)
  if (fOption == SQL_OPT_TRACE || fOption == SQL_ATTR_TRACE)
#else
  if (fOption == SQL_OPT_TRACE)
#endif
    {
      if (ODBCSharedTraceFlag)
	*((UDWORD *) pvParam) = (UDWORD) SQL_OPT_TRACE_ON;
      else
	*((UDWORD *) pvParam) = (UDWORD) SQL_OPT_TRACE_OFF;

      return SQL_SUCCESS;
    }

#if (ODBCVER >= 0x0300)
  if (fOption == SQL_OPT_TRACEFILE || fOption == SQL_ATTR_TRACEFILE)
#else
  if (fOption == SQL_OPT_TRACEFILE)
#endif
    {
      char *fname;
      
      fname = trace_get_filename ();	/* UTF8 format */

      if (waMode != 'W')
        {
           STRCPY (pvParam, fname);
        }
      else
        {
           dm_strcpy_A2W ((SQLWCHAR *)pvParam, (SQLCHAR *)fname);
        }

      free (fname);

      return SQL_SUCCESS;
    }

  if (pdbc->state != en_dbc_allocated)
    /* if already connected, we will invoke driver's function */
    {

      void * _Param = NULL;
      void * paramOut = pvParam;
      int procid = 0;

      switch (fOption)
        {
        case SQL_CURRENT_QUALIFIER:
        case SQL_TRANSLATE_DLL:

          if ((penv->unicode_driver && waMode != 'W') 
              || (!penv->unicode_driver && waMode == 'W'))
            {
              if (waMode != 'W')
                {
                /* ansi=>unicode*/
                  if ((_Param = malloc(SQL_MAX_OPTION_STRING_LENGTH * sizeof(wchar_t))) == NULL)
	            {
                      PUSHSQLERR (pdbc->herr, en_HY001);
                      return SQL_ERROR;
                    }
                }
              else
                {
                /* unicode=>ansi*/
                  if ((_Param = malloc(SQL_MAX_OPTION_STRING_LENGTH)) == NULL)
    	            {
                      PUSHSQLERR (pdbc->herr, en_HY001);
                      return SQL_ERROR;
                    }
                }
              paramOut = _Param;
            }
          break;
        }

#if (ODBCVER >= 0x0300)

     if (penv->unicode_driver)
       {
         /* SQL_XXX_W */
         if ((hproc = _iodbcdm_getproc (pdbc, en_GetConnectAttrW))
             != SQL_NULL_HPROC)
           procid = en_GetConnectAttrW;
       }
     else
       {
         /* SQL_XXX */
         /* SQL_XXX_A */
         if ((hproc = _iodbcdm_getproc (pdbc, en_GetConnectAttr))
             != SQL_NULL_HPROC)
           procid = en_GetConnectAttr;
         else
         if ((hproc = _iodbcdm_getproc (pdbc, en_GetConnectAttrA))
             != SQL_NULL_HPROC)
           procid = en_GetConnectAttrA;
       }

      if (hproc != SQL_NULL_HPROC)
	{
	  switch (fOption)
	    {
	      /* integer attributes */
	    case SQL_ATTR_ACCESS_MODE:
	    case SQL_ATTR_AUTOCOMMIT:
	    case SQL_ATTR_LOGIN_TIMEOUT:
	    case SQL_ATTR_ODBC_CURSORS:
	    case SQL_ATTR_PACKET_SIZE:
	    case SQL_ATTR_QUIET_MODE:
	    case SQL_ATTR_TRANSLATE_OPTION:
	    case SQL_ATTR_TXN_ISOLATION:
	      CALL_DRIVER (hdbc, pdbc, retcode, hproc, procid,
		  (pdbc->dhdbc, fOption, paramOut, 0, NULL));
	      break;

	      /* ODBC3 defined options */
	    case SQL_ATTR_ASYNC_ENABLE:
	    case SQL_ATTR_AUTO_IPD:
	    case SQL_ATTR_CONNECTION_DEAD:
	    case SQL_ATTR_CONNECTION_TIMEOUT:
	    case SQL_ATTR_METADATA_ID:
	      PUSHSQLERR (pdbc->herr, en_IM001);
	      MEM_FREE(_Param);
	      return SQL_ERROR;

	    default:		/* string & driver defined */

	      CALL_DRIVER (hdbc, pdbc, retcode, hproc, procid,
		  (pdbc->dhdbc, fOption, paramOut, SQL_MAX_OPTION_STRING_LENGTH, NULL));

	    }
	}
      else
#endif
	{
          if (penv->unicode_driver)
            {
             /* SQL_XXX_W */
             if ((hproc = _iodbcdm_getproc (pdbc, en_GetConnectOptionW))
                 != SQL_NULL_HPROC)
               procid = en_GetConnectOptionW;
            }
          else
            {
             /* SQL_XXX */
             /* SQL_XXX_A */
             if ((hproc = _iodbcdm_getproc (pdbc, en_GetConnectOption))
                != SQL_NULL_HPROC)
               procid = en_GetConnectOption;
             else
             if ((hproc = _iodbcdm_getproc (pdbc, en_GetConnectOptionA))
                != SQL_NULL_HPROC)
               procid = en_GetConnectOptionA;
            }

	  if (hproc == SQL_NULL_HPROC)
	    {
	      PUSHSQLERR (pdbc->herr, en_IM001);
              MEM_FREE(_Param);
	      return SQL_ERROR;
	    }

	  CALL_DRIVER (hdbc, pdbc, retcode, hproc, procid,
	      (pdbc->dhdbc, fOption, paramOut));
	}

      if (pvParam
          && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
          &&  ((penv->unicode_driver && waMode != 'W') 
              || (!penv->unicode_driver && waMode == 'W')))
        {
          switch (fOption)
            {
            case SQL_ATTR_TRACEFILE:
            case SQL_CURRENT_QUALIFIER:
            case SQL_TRANSLATE_DLL:
              if (waMode != 'W')
                {
                 /* ansi<=unicode*/
                  dm_StrCopyOut2_W2A ((SQLWCHAR *)paramOut, (SQLCHAR *)pvParam, SQL_MAX_OPTION_STRING_LENGTH, NULL);
                }
              else
                {
                 /* unicode<=ansi*/
                  dm_StrCopyOut2_A2W ((SQLCHAR *)paramOut, (SQLWCHAR *)pvParam, SQL_MAX_OPTION_STRING_LENGTH, NULL);
                }
              break;
            }
        }
      
      MEM_FREE(_Param);
      return retcode;
    }

  /* We needn't to handle options which are not allowed 
   * to be *get* at a allocated dbc state(and two tracing
   * options which has been handled and returned). Thus, 
   * there are only two possible cases. 
   */
  switch (fOption)
    {
    case SQL_ACCESS_MODE:
      *((UDWORD *) pvParam) = pdbc->access_mode;
      break;

    case SQL_AUTOCOMMIT:
      *((UDWORD *) pvParam) = pdbc->autocommit;
      break;

    case SQL_LOGIN_TIMEOUT:
      *((UDWORD *) pvParam) = pdbc->login_timeout;
      break;

    default:
      break;
    }

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLGetConnectOption (SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
  ENTER_HDBC (hdbc, 0,
    trace_SQLGetConnectOption (TRACE_ENTER, hdbc, fOption, pvParam));

  retcode = _iodbcdm_GetConnectOption (hdbc, fOption, pvParam, 'A');

  LEAVE_HDBC (hdbc, 0,
    trace_SQLGetConnectOption (TRACE_LEAVE, hdbc, fOption, pvParam));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLGetConnectOptionA (
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fOption,
  SQLPOINTER		  pvParam)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLGetConnectOption (TRACE_ENTER, hdbc, fOption, pvParam));

  retcode = _iodbcdm_GetConnectOption (hdbc, fOption, pvParam, 'A');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLGetConnectOption (TRACE_LEAVE, hdbc, fOption, pvParam));
}


SQLRETURN SQL_API
SQLGetConnectOptionW (
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fOption,
  SQLPOINTER		  pvParam)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLGetConnectOptionW (TRACE_ENTER, hdbc, fOption, pvParam));

  retcode = _iodbcdm_GetConnectOption (hdbc, fOption, pvParam, 'W');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLGetConnectOptionW (TRACE_LEAVE, hdbc, fOption, pvParam));
}
#endif


static SQLRETURN
_iodbcdm_transact (
    HDBC hdbc,
    UWORD fType)
{
  CONN (pdbc, hdbc);
  STMT (pstmt, NULL);
  HPROC hproc;
  SQLRETURN retcode;

  /* check state */
  switch (pdbc->state)
    {
    case en_dbc_allocated:
    case en_dbc_needdata:
      PUSHSQLERR (pdbc->herr, en_08003);
      return SQL_ERROR;

    case en_dbc_connected:
      return SQL_SUCCESS;

    case en_dbc_hstmt:
    default:
      break;
    }

  for (pstmt = (STMT_t *) (pdbc->hstmt);
      pstmt != NULL;
      pstmt = pstmt->next)
    {
      if (pstmt->state >= en_stmt_needdata
	  || pstmt->asyn_on != en_NullProc)
	{
	  PUSHSQLERR (pdbc->herr, en_S1010);

	  return SQL_ERROR;
	}
    }
  hproc = _iodbcdm_getproc (pdbc, en_Transact);

  if (hproc != SQL_NULL_HPROC)
    {
      CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_Transact,
	  (SQL_NULL_HENV, pdbc->dhdbc, fType));
    }
  else
    {
#if (ODBCVER >= 0x300)
      hproc = _iodbcdm_getproc (pdbc, en_EndTran);
      if (hproc != SQL_NULL_HPROC)
	{
	  CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_EndTran,
	      (SQL_HANDLE_DBC, pdbc->dhdbc, fType));
	}
      else
	{
	  PUSHSQLERR (pdbc->herr, en_IM001);
	  return SQL_ERROR;
	}
#else
      PUSHSQLERR (pdbc->herr, en_IM001);
      return SQL_ERROR;
#endif
    }

  /* state transition */
  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      return retcode;
    }

  pdbc->state = en_dbc_hstmt;

  for (pstmt = (STMT_t *) (pdbc->hstmt);
      pstmt != NULL;
      pstmt = pstmt->next)
    {
      switch (pstmt->state)
	{
	case en_stmt_prepared:
	  if (pdbc->cb_commit == SQL_CB_DELETE
	      || pdbc->cb_rollback == SQL_CB_DELETE)
	    {
	      pstmt->state = en_stmt_allocated;
	      pstmt->prep_state = 0;
	      break;
	    }
	  break;

	case en_stmt_executed_with_info:
	case en_stmt_executed:
	case en_stmt_cursoropen:
	case en_stmt_fetched:
	case en_stmt_xfetched:
	  if (!pstmt->prep_state
	      && pdbc->cb_commit != SQL_CB_PRESERVE
	      && pdbc->cb_rollback != SQL_CB_PRESERVE)
	    {
	      pstmt->state = en_stmt_allocated;
	      pstmt->prep_state = 0;
	      pstmt->cursor_state = en_stmt_cursor_no;
	      break;
	    }

	  if (pstmt->prep_state)
	    {
	      if (pdbc->cb_commit == SQL_CB_DELETE
		  || pdbc->cb_rollback == SQL_CB_DELETE)
		{
		  pstmt->state = en_stmt_allocated;
		  pstmt->prep_state = 0;
		  pstmt->cursor_state = en_stmt_cursor_no;
		  break;
		}

	      if (pdbc->cb_commit == SQL_CB_CLOSE
		  || pdbc->cb_rollback == SQL_CB_CLOSE)
		{
		  pstmt->state
		      = en_stmt_prepared;
		  pstmt->cursor_state
		      = en_stmt_cursor_no;
		  break;
		}
	      break;
	    }
	  break;

	default:
	  break;
	}
    }

  return retcode;
}


SQLRETURN 
SQLTransact_Internal (
    SQLHENV henv,
    SQLHDBC hdbc,
    SQLUSMALLINT fType)
{
  GENV (genv, henv);
  CONN (pdbc, hdbc);
  HERR herr;
  SQLRETURN retcode = SQL_SUCCESS;

  if (IS_VALID_HDBC (pdbc))
    {
      CLEAR_ERRORS (pdbc);
      herr = pdbc->herr;
    }
  else if (IS_VALID_HENV (genv))
    {
      CLEAR_ERRORS (genv);
      herr = genv->herr;
    }
  else
    {
      return SQL_INVALID_HANDLE;
    }

  /* check argument */
  if (fType != SQL_COMMIT && fType != SQL_ROLLBACK)
    {
/*      SQLHENV handle = IS_VALID_HDBC (pdbc) ? ((SQLHENV) pdbc) : genv;*/
      PUSHSQLERR (herr, en_S1012);
      return SQL_ERROR;
    }

  if (hdbc != SQL_NULL_HDBC)
    {
      retcode = _iodbcdm_transact (pdbc, fType);
    }
  else
    {
      for (pdbc = (DBC_t *) (genv->hdbc); pdbc != NULL; pdbc = pdbc->next)
	{
	  retcode |= _iodbcdm_transact (pdbc, fType);
	}
    }

  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      /* fail on one of the connection */
      return SQL_ERROR;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLTransact (
  SQLHENV		  henv,
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fType)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();
  TRACE (trace_SQLTransact (TRACE_ENTER, henv, hdbc, fType));

  retcode = SQLTransact_Internal (henv, hdbc, fType);

  TRACE (trace_SQLTransact (TRACE_LEAVE, henv, hdbc, fType));
  ODBC_UNLOCK ();

  return retcode;
}
