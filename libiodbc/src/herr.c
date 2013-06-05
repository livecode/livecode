/*
 *  herr.c
 *
 *  $Id: herr.c,v 1.23 2006/01/20 15:58:34 source Exp $
 *
 *  Error stack management functions
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
#if (ODBCVER >= 0x0300)
#include <hdesc.h>
#endif
#include <henv.h>
#include <hdbc.h>
#include <hstmt.h>

#include <itrace.h>

#include "herr.ci"

static HERR 
_iodbcdm_popsqlerr (HERR herr)
{
  sqlerr_t *list = (sqlerr_t *) herr;
  sqlerr_t *next;

  if (herr == SQL_NULL_HERR)
    {
      return herr;
    }

  next = list->next;
  list->next = NULL;

  MEM_FREE (list);

  return next;
}


void 
_iodbcdm_freesqlerrlist (HERR herrlist)
{
  HERR list = herrlist;

  while (list != SQL_NULL_HERR)
    {
      list = _iodbcdm_popsqlerr (list);
    }
}


HERR 
_iodbcdm_pushsqlerr (
    HERR herr,
    sqlstcode_t code,
    void *msg)
{
  sqlerr_t *ebuf;
  sqlerr_t *perr = (sqlerr_t *) herr;
  int idx = 0;

  if (herr != SQL_NULL_HERR)
    {
      idx = perr->idx + 1;
    }

  if (idx == 64)
    /* overwrite the top entry to prevent error stack blow out */
    {
      perr->code = code;
      perr->msg = (char *) msg;

      return herr;
    }

  ebuf = (sqlerr_t *) MEM_ALLOC (sizeof (sqlerr_t));

  if (ebuf == NULL)
    {
      return NULL;
    }

  ebuf->msg = (char *) msg;
  ebuf->code = code;
  ebuf->idx = idx;
  ebuf->next = (sqlerr_t *) herr;
  return (HERR) ebuf;
}


static char *
_iodbcdm_getsqlstate (
    HERR herr,
    void * tab)
{
  sqlerr_t *perr = (sqlerr_t *) herr;
  sqlerrmsg_t *ptr;
  sqlstcode_t perr_code;

  if (herr == SQL_NULL_HERR || tab == NULL)
    {
      return (char *) NULL;
    }

  perr_code = perr->code;
#if (ODBCVER >= 0x0300)
  switch (perr_code)
    {
    case en_S1009:
      perr_code = en_HY009;
      break;

    default:
      break;
    }
#endif
  for (ptr = (sqlerrmsg_t *) tab; ptr->code != en_sqlstat_total; ptr++)
    {
      if (ptr->code == perr_code)
	{
	  return (char *) (ptr->stat);
	}
    }

  return (char *) NULL;
}


static char *
_iodbcdm_getsqlerrmsg (
    HERR herr,
    void * errtab)
{
  sqlerr_t *perr = (sqlerr_t *) herr;
  sqlerrmsg_t *ptr;

  if (herr == SQL_NULL_HERR)
    {
      return NULL;
    }

  if (perr->msg == NULL && errtab == NULL)
    {
      return NULL;
    }

  if (perr->msg != NULL)
    {
      return perr->msg;
    }

  for (ptr = (sqlerrmsg_t *) errtab;
      ptr->code != en_sqlstat_total;
      ptr++)
    {
      if (ptr->code == perr->code)
	{
	  return (char *) ptr->msg;
	}
    }

  return (char *) NULL;
}


SQLRETURN SQL_API 
_iodbcdm_sqlerror (
  SQLHENV		  henv,
  SQLHDBC		  hdbc,
  SQLHSTMT		  hstmt,
  SQLPOINTER		  szSqlstate,
  SQLINTEGER		* pfNativeError,
  SQLPOINTER		  szErrorMsg,
  SQLSMALLINT		  cbErrorMsgMax,
  SQLSMALLINT 		* pcbErrorMsg,
  int		  	  bDelete,
  SQLCHAR		  waMode)
{
  GENV (genv, henv);
  CONN (pdbc, hdbc);
  STMT (pstmt, hstmt);
  HDBC thdbc = SQL_NULL_HDBC;

  HENV dhenv = SQL_NULL_HENV;
  HDBC dhdbc = SQL_NULL_HDBC;
  HSTMT dhstmt = SQL_NULL_HSTMT;

  HERR herr = SQL_NULL_HERR;
  HPROC hproc = SQL_NULL_HPROC;
#if (ODBCVER >= 0x0300)  
  SQLINTEGER handleType = 0;
  SQLHANDLE handle3;
  SQLHANDLE dhandle3 = 0;
  SQLSMALLINT *perr_rec = NULL;
#endif
  SWORD unicode_driver = 0;
  SQLUINTEGER dodbc_ver = SQL_OV_ODBC2;	
  wchar_t _sqlState[6] = {L"\0"};
  void *SqlstateOut = szSqlstate;
  void *_ErrorMsg = NULL;
  void *errorMsgOut = szErrorMsg;

  void *errmsg = NULL;
  void *ststr = NULL;

  int handle = 0;
  SQLRETURN retcode = SQL_SUCCESS;

  if (IS_VALID_HSTMT (hstmt))	/* retrieve stmt err */
    {
      herr = pstmt->herr;
      thdbc = pstmt->hdbc;

      if (thdbc == SQL_NULL_HDBC)
	{
	  return SQL_INVALID_HANDLE;
	}

#if (ODBCVER >= 0x0300)
      handleType = SQL_HANDLE_STMT;
      handle3 = hstmt;
      dhandle3 = pstmt->dhstmt;
      perr_rec = &((STMT_t *)pstmt)->err_rec;
#endif      
      dhstmt = pstmt->dhstmt;
      handle = 3;
    }
  else if (IS_VALID_HDBC (hdbc))	/* retrieve dbc err */
    {
      herr = pdbc->herr;
      thdbc = pdbc;
      if (thdbc == SQL_NULL_HDBC)
	{
	  return SQL_INVALID_HANDLE;
	}

#if (ODBCVER >= 0x0300)
      handleType = SQL_HANDLE_DBC;
      handle3 = hdbc;
      dhandle3 = pdbc->dhdbc;
      perr_rec = &((DBC_t *)pdbc)->err_rec;
#endif      
      dhdbc = pdbc->dhdbc;
      handle = 2;

      if (herr == SQL_NULL_HERR
	  && pdbc->henv == SQL_NULL_HENV)
	{
	  return SQL_NO_DATA_FOUND;
	}
    }
  else if (IS_VALID_HENV (henv))	/* retrieve env err */
    {
      herr = genv->herr;

      /* Drivers shouldn't push error message 
       * on environment handle */

      if (herr == SQL_NULL_HERR)
	{
	  return SQL_NO_DATA_FOUND;
	}

      handle = 1;
    }
  else
    {
      return SQL_INVALID_HANDLE;
    }

  if (szErrorMsg != NULL)
    {
      if (cbErrorMsgMax < 0)
	{
	  return SQL_ERROR;
	  /* SQLError() doesn't post error for itself */
	}
    }

  if (herr == SQL_NULL_HERR)	/* no err on drv mng */
    {
      /* call driver */
      unicode_driver = ((ENV_t *) ((DBC_t *)thdbc)->henv)->unicode_driver;
      dodbc_ver = ((ENV_t *) ((DBC_t *)thdbc)->henv)->dodbc_ver;

      if ((unicode_driver && waMode != 'W') 
          || (!unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              if ((_ErrorMsg = malloc(cbErrorMsgMax * sizeof(wchar_t) + 1)) == NULL)
                return SQL_ERROR;
            }
          else
            {
            /* unicode=>ansi*/
              if ((_ErrorMsg = malloc(cbErrorMsgMax + 1)) == NULL)
                return SQL_ERROR;
            }
          errorMsgOut = _ErrorMsg;
          SqlstateOut = _sqlState;
        }
    
      /* call driver */
      if (unicode_driver)
        {
          /* SQL_XXX_W */
#if (ODBCVER >= 0x0300)
          if (dodbc_ver >= SQL_OV_ODBC3)
            {
              if ((hproc = _iodbcdm_getproc (thdbc, en_GetDiagRecW)) 
                  != SQL_NULL_HPROC)
                {
                 (*perr_rec) = (*perr_rec) + 1;
                  CALL_DRIVER (thdbc, NULL, retcode, hproc,
                       en_GetDiagRecW, (
                          handleType, 
                          dhandle3, 
                          (*perr_rec),
                          SqlstateOut, 
                          pfNativeError, 
                          errorMsgOut,
                          cbErrorMsgMax, 
                          pcbErrorMsg));
                }
            }
          else
#endif
            {
              if ((hproc = _iodbcdm_getproc (thdbc, en_ErrorW)) 
                  != SQL_NULL_HPROC)
                {
                  CALL_DRIVER (thdbc, NULL, retcode, hproc,
                       en_ErrorW, (
                          dhenv, 
                          dhdbc, 
                          dhstmt, 
                          SqlstateOut, 
                          pfNativeError, 
                          errorMsgOut,
                          cbErrorMsgMax, 
                          pcbErrorMsg));
                }
            }
        }
      else
        {
          /* SQL_XXX */
          /* SQL_XXX_A */
#if (ODBCVER >= 0x0300)
          if (dodbc_ver >= SQL_OV_ODBC3)
            {
              if ((hproc = _iodbcdm_getproc (thdbc, en_GetDiagRec)) 
                  != SQL_NULL_HPROC)
                {
                 (*perr_rec) = (*perr_rec) + 1;
                  CALL_DRIVER (thdbc, NULL, retcode, hproc,
                       en_GetDiagRec, (
                          handleType, 
                          dhandle3, 
                          (*perr_rec),
                          SqlstateOut, 
                          pfNativeError, 
                          errorMsgOut,
                          cbErrorMsgMax, 
                          pcbErrorMsg));
                }
              else
              if ((hproc = _iodbcdm_getproc (thdbc, en_GetDiagRecA)) 
                  != SQL_NULL_HPROC)
                {
                 (*perr_rec) = (*perr_rec) + 1;
                  CALL_DRIVER (thdbc, NULL, retcode, hproc,
                       en_GetDiagRecA, (
                          handleType, 
                          dhandle3, 
                          (*perr_rec),
                          SqlstateOut, 
                          pfNativeError, 
                          errorMsgOut,
                          cbErrorMsgMax, 
                          pcbErrorMsg));
                }
            }
          else
#endif
            {
              if ((hproc = _iodbcdm_getproc (thdbc, en_Error)) 
                  != SQL_NULL_HPROC)
                {
                  CALL_DRIVER (thdbc, NULL, retcode, hproc,
                       en_Error, (
                          dhenv, 
                          dhdbc, 
                          dhstmt, 
                          SqlstateOut, 
                          pfNativeError, 
                          errorMsgOut,
                          cbErrorMsgMax, 
                          pcbErrorMsg));
                }
              else
              if ((hproc = _iodbcdm_getproc (thdbc, en_ErrorA)) 
                  != SQL_NULL_HPROC)
                {
                  CALL_DRIVER (thdbc, NULL, retcode, hproc,
                       en_ErrorA, (
                          dhenv, 
                          dhdbc, 
                          dhstmt, 
                          SqlstateOut, 
                          pfNativeError, 
                          errorMsgOut,
                          cbErrorMsgMax, 
                          pcbErrorMsg));
                }
            }
        }
    
      if (hproc == SQL_NULL_HPROC)
        {
          MEM_FREE(_ErrorMsg);
          return SQL_NO_DATA_FOUND;
        }
    
      if (szErrorMsg 
          && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
          &&  ((unicode_driver && waMode != 'W') 
              || (!unicode_driver && waMode == 'W')))
        {
          if (waMode != 'W')
            {
            /* ansi<=unicode*/
              dm_StrCopyOut2_W2A ((SQLWCHAR *)errorMsgOut, (SQLCHAR *)szErrorMsg, cbErrorMsgMax, NULL);
              dm_StrCopyOut2_W2A ((SQLWCHAR *)SqlstateOut, (SQLCHAR *)szSqlstate, 6, NULL);
            }
          else
            {
            /* unicode<=ansi*/
              dm_StrCopyOut2_A2W ((SQLCHAR *)errorMsgOut, (SQLWCHAR *)szErrorMsg, cbErrorMsgMax, NULL);
              dm_StrCopyOut2_A2W ((SQLCHAR *)SqlstateOut, (SQLWCHAR *)szSqlstate, 6, NULL);
            }
        }
    
      MEM_FREE(_ErrorMsg);

      return retcode;
    }

  if (szSqlstate != NULL)
    {
      int len;

      /* get sql state  string */
      ststr = (char *) _iodbcdm_getsqlstate (herr,
	  (void *) sqlerrmsg_tab);

      if (ststr == NULL)
	{
	  len = 0;
	}
      else
	{
	  len = (int) STRLEN (ststr);
	}

      /* buffer size of szSqlstate is not checked. Applications
       * suppose provide enough ( not less than 6 bytes ) buffer
       * or NULL for it.
       */
      if (waMode != 'W')
        {	
          STRNCPY (szSqlstate, ststr, len);
          ((char*)szSqlstate)[len] = 0;
        }
      else
        {
          dm_StrCopyOut2_A2W ((SQLCHAR *)ststr, (SQLWCHAR *)szSqlstate, 6, NULL);
          ((wchar_t*)szSqlstate)[len] = 0;
        }
    }

  if (pfNativeError != NULL)
    {
      /* native error code is specific to data source */
      *pfNativeError = (SDWORD) 0L;
    }

  if (szErrorMsg == NULL || cbErrorMsgMax == 0)
    {
      if (pcbErrorMsg != NULL)
	{
	  *pcbErrorMsg = (SWORD) 0;
	}
    }
  else
    {
      int len;
      char msgbuf[256] = {'\0'};

      /* get sql state message */
      errmsg = _iodbcdm_getsqlerrmsg (herr, (void *) sqlerrmsg_tab);

      if (errmsg == NULL)
	{
	  errmsg = (char *) "";
	}

#if defined(HAVE_SNPRINTF)
      snprintf (msgbuf, sizeof(msgbuf), "%s%s", sqlerrhd, (char*)errmsg);
#else
      sprintf (msgbuf, "%s%s", sqlerrhd, (char*)errmsg);
#endif

      len = STRLEN (msgbuf);

      if (len < cbErrorMsgMax - 1)
	{
	  retcode = SQL_SUCCESS;
	}
      else
	{
	  len = cbErrorMsgMax - 1;
	  retcode = SQL_SUCCESS_WITH_INFO;
	  /* and not posts error for itself */
	}

      if (waMode != 'W')
        {
          STRNCPY ((char *) szErrorMsg, msgbuf, len);
          ((char*)szErrorMsg)[len] = 0;
          if (pcbErrorMsg != NULL)
	    *pcbErrorMsg = (SWORD) len;
        }
      else
        {
          dm_StrCopyOut2_A2W ((SQLCHAR *) msgbuf, 
	  	(SQLWCHAR *) szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
        }
    }

  if (bDelete)
    switch (handle)		/* free this err */
      {
	case 1:
	    genv->herr = _iodbcdm_popsqlerr (genv->herr);
	    break;

	case 2:
	    pdbc->herr = _iodbcdm_popsqlerr (pdbc->herr);
	    break;

	case 3:
	    pstmt->herr = _iodbcdm_popsqlerr (pstmt->herr);
	    break;

	default:
	    break;
      }

  return retcode;
}


SQLRETURN SQL_API 
SQLError (
  SQLHENV		  henv,
  SQLHDBC		  hdbc,
  SQLHSTMT		  hstmt,
  SQLCHAR 		* szSqlstate,
  SQLINTEGER 		* pfNativeError,
  SQLCHAR 		* szErrorMsg,
  SQLSMALLINT		  cbErrorMsgMax,
  SQLSMALLINT 		* pcbErrorMsg)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();
  TRACE (trace_SQLError (TRACE_ENTER,
  	henv, 
	hdbc, 
	hstmt, 
	szSqlstate, 
	pfNativeError,
        szErrorMsg, cbErrorMsgMax, pcbErrorMsg));

  retcode = _iodbcdm_sqlerror (
  	henv, 
	hdbc, 
	hstmt, 
	szSqlstate, 
	pfNativeError,
        szErrorMsg, cbErrorMsgMax, pcbErrorMsg, 
	1, 
	'A');

  TRACE (trace_SQLError (TRACE_LEAVE,
  	henv, 
	hdbc, 
	hstmt, 
	szSqlstate, 
	pfNativeError,
        szErrorMsg, cbErrorMsgMax, pcbErrorMsg));

  ODBC_UNLOCK ();
  return retcode;
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLErrorA (
  SQLHENV		  henv,
  SQLHDBC 		  hdbc,
  SQLHSTMT 		  hstmt,
  SQLCHAR 		* szSqlstate,
  SQLINTEGER 		* pfNativeError,
  SQLCHAR 		* szErrorMsg,
  SQLSMALLINT		  cbErrorMsgMax,
  SQLSMALLINT 		* pcbErrorMsg)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();
  TRACE (trace_SQLError (TRACE_ENTER,
  	henv, 
	hdbc, 
	hstmt, 
	szSqlstate, 
	pfNativeError,
        szErrorMsg, cbErrorMsgMax, pcbErrorMsg));

  retcode = _iodbcdm_sqlerror (
  	henv, 
	hdbc, 
	hstmt, 
	szSqlstate, 
	pfNativeError,
        szErrorMsg, cbErrorMsgMax, pcbErrorMsg, 
	1, 
	'A');

  TRACE (trace_SQLError (TRACE_LEAVE,
  	henv, 
	hdbc, 
	hstmt, 
	szSqlstate, 
	pfNativeError,
        szErrorMsg, cbErrorMsgMax, pcbErrorMsg));

  ODBC_UNLOCK ();
  return retcode;
}


SQLRETURN SQL_API
SQLErrorW (
  SQLHENV		  henv,
  SQLHDBC		  hdbc,
  SQLHSTMT		  hstmt,
  SQLWCHAR 		* szSqlstate,
  SQLINTEGER 		* pfNativeError,
  SQLWCHAR 		* szErrorMsg,
  SQLSMALLINT		  cbErrorMsgMax,
  SQLSMALLINT 		* pcbErrorMsg)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();
  TRACE (trace_SQLErrorW (TRACE_ENTER,
  	henv, 
	hdbc, 
	hstmt, 
	szSqlstate, 
	pfNativeError,
        szErrorMsg, cbErrorMsgMax, pcbErrorMsg));

  retcode = _iodbcdm_sqlerror (
  	henv, 
	hdbc, 
	hstmt, 
	szSqlstate, 
	pfNativeError,
	szErrorMsg, cbErrorMsgMax, pcbErrorMsg, 
	1, 
	'W');

  TRACE (trace_SQLErrorW (TRACE_LEAVE,
  	henv, 
	hdbc, 
	hstmt, 
	szSqlstate, 
	pfNativeError,
        szErrorMsg, cbErrorMsgMax, pcbErrorMsg));

  ODBC_UNLOCK ();
  return retcode;
}
#endif


#if (ODBCVER >= 0x0300)
static int
error_rec_count (HERR herr)
{
  sqlerr_t *err = (sqlerr_t *)herr;
  if (err)
    return err->idx + 1;
  else
    return 0;
}


static sqlerr_t *
get_nth_error(HERR herr, int nIndex)
{
  sqlerr_t *err = (sqlerr_t *) herr;
  while (err && err->idx != nIndex)
      err = err->next;
  return err;
}


RETCODE SQL_API
SQLGetDiagRec_Internal (
  SQLSMALLINT		  HandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  RecNumber,
  SQLPOINTER		  Sqlstate,
  SQLINTEGER		* NativeErrorPtr,
  SQLPOINTER		  MessageText,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* TextLengthPtr,
  SQLCHAR		  waMode)
{
  sqlerr_t *curr_err = NULL;
  HERR err = NULL;
  int nRecs;
  HPROC hproc = SQL_NULL_HPROC;
  HDBC hdbc = SQL_NULL_HDBC;
  RETCODE retcode = SQL_SUCCESS;
  SQLHANDLE dhandle = SQL_NULL_HANDLE;
  SWORD unicode_driver = 0;
  wchar_t _sqlState[6] = {L"\0"};
  void *_MessageText = NULL;
  void *messageTextOut = MessageText;
  void *SqlstateOut = Sqlstate;


  if (RecNumber < 1)
    return SQL_ERROR;

  if (BufferLength < 0)
    return SQL_ERROR;

  switch (HandleType)
    {
    case SQL_HANDLE_ENV:
      if (!IS_VALID_HENV (Handle))
	{
	  return SQL_INVALID_HANDLE;
	}
      err = ((GENV_t *) Handle)->herr;
      break;

    case SQL_HANDLE_DBC:
      if (!IS_VALID_HDBC (Handle))
	{
	  return SQL_INVALID_HANDLE;
	}
      err = ((DBC_t *) Handle)->herr;
      dhandle = ((DBC_t *) Handle)->dhdbc;
      hdbc = Handle;
      break;

    case SQL_HANDLE_STMT:
      if (!IS_VALID_HSTMT (Handle))
	{
	  return SQL_INVALID_HANDLE;
	}
      err = ((STMT_t *) Handle)->herr;
      dhandle = ((STMT_t *) Handle)->dhstmt;
      hdbc = ((STMT_t *) Handle)->hdbc;
      break;

    case SQL_HANDLE_DESC:
      if (!IS_VALID_HDESC (Handle))
	{
	  return SQL_INVALID_HANDLE;
	}
      err = ((DESC_t *) Handle)->herr;
      dhandle = ((DESC_t *) Handle)->dhdesc;
      hdbc = ((DESC_t *) Handle)->hdbc;
      break;

    default:
      return SQL_INVALID_HANDLE;
    }

  nRecs = error_rec_count (err);

  if (nRecs >= RecNumber)
    {				/* DM error range */
      curr_err = get_nth_error (err, RecNumber - 1);

      if (!curr_err)
	{
	  return (SQL_NO_DATA_FOUND);
	}

      retcode = SQL_SUCCESS;

      if (Sqlstate != NULL)
	{
	  int len;
	  char *ststr = (char *) _iodbcdm_getsqlstate (curr_err,
	      (void *) sqlerrmsg_tab);

	  if (ststr == NULL)
	    {
	      len = 0;
	    }
	  else
	    {
	      len = (int) STRLEN (ststr);
	    }

          /* buffer size of szSqlstate is not checked. Applications
           * suppose provide enough ( not less than 6 bytes ) buffer
           * or NULL for it.
           */
          if (waMode != 'W')
            {	
              STRNCPY (Sqlstate, ststr, len);
              ((char*)Sqlstate)[len] = 0;
            }
          else
            {
              dm_StrCopyOut2_A2W ((SQLCHAR *) ststr, 
			(SQLWCHAR *) Sqlstate, 6, NULL);
              ((wchar_t*)Sqlstate)[len] = 0;
            }
	}

      if (MessageText == NULL || BufferLength == 0)
	{
	  if (TextLengthPtr != NULL)
	    {
	      *TextLengthPtr = (SWORD) 0;
	    }
	}
      else
	{
	  int len;
	  char msgbuf[256] = { '\0' };
	  char *errmsg;

	  /* get sql state message */
	  errmsg =
	      _iodbcdm_getsqlerrmsg (curr_err, (void *) sqlerrmsg_tab);

	  if (errmsg == NULL)
	    {
	      errmsg = (char *) "";
	    }

#if defined(HAVE_SNPRINTF)
	  snprintf (msgbuf, sizeof (msgbuf), "%s%s", sqlerrhd, errmsg);
#else
	  sprintf (msgbuf, "%s%s", sqlerrhd, errmsg);
#endif

	  len = STRLEN (msgbuf);

	  if (len < BufferLength - 1)
	    {
	      retcode = SQL_SUCCESS;
	    }
	  else
	    {
	      len = BufferLength - 1;
	      retcode = SQL_SUCCESS_WITH_INFO;
	      /* and not posts error for itself */
	    }

          if (waMode != 'W')
            {
              STRNCPY ((char *) MessageText, msgbuf, len);
              ((char*)MessageText)[len] = 0;
              if (TextLengthPtr != NULL)
  	        *TextLengthPtr = (SWORD) len;
            }
          else
            {
              dm_StrCopyOut2_A2W ((SQLCHAR *) msgbuf, 
		    (SQLWCHAR *) MessageText, BufferLength, TextLengthPtr);
            }
	}
      return retcode;
    }
  else
    {				/* Driver errors */
      if (hdbc == SQL_NULL_HDBC)
	{
	  return SQL_NO_DATA_FOUND;
	}
      RecNumber -= nRecs;

      if (((DBC_t *)hdbc)->henv)
        unicode_driver = ((ENV_t *) ((DBC_t *)hdbc)->henv)->unicode_driver;

      if ((unicode_driver && waMode != 'W') 
          || (!unicode_driver && waMode == 'W'))
        {
          if (waMode != 'W')
            {
            /* ansi=>unicode*/
              if ((_MessageText = malloc((BufferLength + 1) * sizeof(wchar_t))) == NULL)
                {
                  return SQL_ERROR;
                }
            }
          else
            {
            /* unicode=>ansi*/
              if ((_MessageText = malloc(BufferLength + 1)) == NULL)
                {
                  return SQL_ERROR;
                }
            }
          messageTextOut = _MessageText;
          SqlstateOut = _sqlState;
        }

      /* call driver */
      if (unicode_driver)
        {
          /* SQL_XXX_W */
          if ((hproc = _iodbcdm_getproc (hdbc, en_GetDiagRecW)) 
              != SQL_NULL_HPROC)
            {
              CALL_DRIVER (hdbc, Handle, retcode, hproc,
                   en_GetDiagRecW, (
                      HandleType, 
                      dhandle, 
                      RecNumber,
                      SqlstateOut, 
                      NativeErrorPtr, 
                      messageTextOut,
                      BufferLength, 
                      TextLengthPtr));
            }
          else
          if ((hproc = _iodbcdm_getproc (hdbc, en_ErrorW)) 
              != SQL_NULL_HPROC)
            {
   	      if (RecNumber > 1 || HandleType == SQL_HANDLE_DESC)
	        {
	           MEM_FREE(_MessageText);
	           return SQL_NO_DATA_FOUND;
	        }
              CALL_DRIVER (hdbc, Handle, retcode, hproc,
                   en_ErrorW, (
                      SQL_NULL_HENV,
                      HandleType == SQL_HANDLE_DBC ? dhandle : SQL_NULL_HDBC,
                      HandleType == SQL_HANDLE_STMT ? dhandle : SQL_NULL_HSTMT,
                      SqlstateOut, 
                      NativeErrorPtr, 
                      messageTextOut,
                      BufferLength, 
                      TextLengthPtr));
                }
        }
      else
        {
          /* SQL_XXX */
          /* SQL_XXX_A */
          if ((hproc = _iodbcdm_getproc (hdbc, en_GetDiagRec)) 
              != SQL_NULL_HPROC)
            {
              CALL_DRIVER (hdbc, Handle, retcode, hproc,
                   en_GetDiagRec, (
                      HandleType, 
                      dhandle, 
                      RecNumber,
                      SqlstateOut, 
                      NativeErrorPtr, 
                      messageTextOut,
                      BufferLength, 
                      TextLengthPtr));
            }
          else
          if ((hproc = _iodbcdm_getproc (hdbc, en_GetDiagRecA)) 
              != SQL_NULL_HPROC)
            {
              CALL_DRIVER (hdbc, Handle, retcode, hproc,
                   en_GetDiagRecA, (
                      HandleType, 
                      dhandle, 
                      RecNumber,
                      SqlstateOut, 
                      NativeErrorPtr, 
                      messageTextOut,
                      BufferLength, 
                      TextLengthPtr));
            }
          else                                   /* no SQLGetDiagRec */
          if ((hproc = _iodbcdm_getproc (hdbc, en_Error)) 
              != SQL_NULL_HPROC)
            {
   	      if (RecNumber > 1 || HandleType == SQL_HANDLE_DESC)
	        {
	           MEM_FREE(_MessageText);
	           return SQL_NO_DATA_FOUND;
	        }
              CALL_DRIVER (hdbc, Handle, retcode, hproc,
                   en_Error, (
                      SQL_NULL_HENV,
                      HandleType == SQL_HANDLE_DBC ? dhandle : SQL_NULL_HDBC,
                      HandleType == SQL_HANDLE_STMT ? dhandle : SQL_NULL_HSTMT,
                      SqlstateOut, 
                      NativeErrorPtr, 
                      messageTextOut,
                      BufferLength, 
                      TextLengthPtr));
            }
          else
          if ((hproc = _iodbcdm_getproc (hdbc, en_ErrorA)) 
              != SQL_NULL_HPROC)
            {
   	      if (RecNumber > 1 || HandleType == SQL_HANDLE_DESC)
	        {
	           MEM_FREE(_MessageText);
	           return SQL_NO_DATA_FOUND;
	        }
              CALL_DRIVER (hdbc, Handle, retcode, hproc,
                   en_ErrorA, (
                      SQL_NULL_HENV,
                      HandleType == SQL_HANDLE_DBC ? dhandle : SQL_NULL_HDBC,
                      HandleType == SQL_HANDLE_STMT ? dhandle : SQL_NULL_HSTMT,
                      SqlstateOut, 
                      NativeErrorPtr, 
                      messageTextOut,
                      BufferLength, 
                      TextLengthPtr));
            }
        }
    
      if (hproc == SQL_NULL_HPROC)
        {
          MEM_FREE(_MessageText);
          return SQL_ERROR;
        }
    
      if (MessageText 
          && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
          &&  ((unicode_driver && waMode != 'W') 
              || (!unicode_driver && waMode == 'W')))
        {
          if (waMode != 'W')
            {
            /* ansi<=unicode*/
              dm_StrCopyOut2_W2A ((SQLWCHAR *)messageTextOut, (SQLCHAR *)MessageText, BufferLength, NULL);
              dm_StrCopyOut2_W2A ((SQLWCHAR *)SqlstateOut, (SQLCHAR *)Sqlstate, 6, NULL);
            }
          else
            {
            /* unicode<=ansi*/
              dm_StrCopyOut2_A2W ((SQLCHAR *) messageTextOut, (SQLWCHAR *) MessageText, BufferLength, NULL);
              dm_StrCopyOut2_A2W ((SQLCHAR *) SqlstateOut, (SQLWCHAR *) Sqlstate, 6, NULL);
            }
        }
    
      MEM_FREE(_MessageText);

      return retcode;
    }
}


RETCODE SQL_API
SQLGetDiagRec (
  SQLSMALLINT		  HandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  RecNumber,
  SQLCHAR		* Sqlstate,
  SQLINTEGER		* NativeErrorPtr,
  SQLCHAR		* MessageText,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* TextLengthPtr)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();

  TRACE (trace_SQLGetDiagRec (TRACE_ENTER,
  	HandleType,
	Handle,
	RecNumber,
	Sqlstate,
	NativeErrorPtr,
	MessageText, BufferLength, TextLengthPtr));

  retcode = SQLGetDiagRec_Internal (
  	HandleType, 
	Handle, 
	RecNumber,
	Sqlstate, 
	NativeErrorPtr, 
	MessageText, BufferLength, TextLengthPtr, 
	'A');

  TRACE (trace_SQLGetDiagRec (TRACE_LEAVE,
  	HandleType,
	Handle,
	RecNumber,
	Sqlstate,
	NativeErrorPtr,
	MessageText, BufferLength, TextLengthPtr));

  ODBC_UNLOCK ();

  return retcode;
}


RETCODE SQL_API
SQLGetDiagRecA (
  SQLSMALLINT		  HandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  RecNumber,
  SQLCHAR		* Sqlstate,
  SQLINTEGER		* NativeErrorPtr,
  SQLCHAR		* MessageText,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* TextLengthPtr)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();

  TRACE (trace_SQLGetDiagRec (TRACE_ENTER,
  	HandleType,
	Handle,
	RecNumber,
	Sqlstate,
	NativeErrorPtr,
	MessageText, BufferLength, TextLengthPtr));

  retcode = SQLGetDiagRec_Internal (
  	HandleType, 
	Handle, 
	RecNumber,
	Sqlstate, 
	NativeErrorPtr, 
	MessageText, BufferLength, TextLengthPtr, 
	'A');

  TRACE (trace_SQLGetDiagRec (TRACE_LEAVE,
  	HandleType,
	Handle,
	RecNumber,
	Sqlstate,
	NativeErrorPtr,
	MessageText, BufferLength, TextLengthPtr));

  ODBC_UNLOCK ();

  return retcode;
}


RETCODE SQL_API
SQLGetDiagRecW (
  SQLSMALLINT		  HandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  RecNumber,
  SQLWCHAR		* Sqlstate,
  SQLINTEGER		* NativeErrorPtr,
  SQLWCHAR		* MessageText,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* TextLengthPtr)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();

  TRACE (trace_SQLGetDiagRecW (TRACE_ENTER,
  	HandleType,
	Handle,
	RecNumber,
	Sqlstate,
	NativeErrorPtr,
	MessageText, BufferLength, TextLengthPtr));

  retcode = SQLGetDiagRec_Internal (
  	HandleType, 
	Handle, 
	RecNumber,
	Sqlstate, 
	NativeErrorPtr, 
	MessageText, BufferLength, TextLengthPtr, 
	'W');

  TRACE (trace_SQLGetDiagRecW (TRACE_LEAVE,
  	HandleType,
	Handle,
	RecNumber,
	Sqlstate,
	NativeErrorPtr,
	MessageText, BufferLength, TextLengthPtr));

  ODBC_UNLOCK ();

  return retcode;
}


RETCODE SQL_API
SQLGetDiagField_Internal (
  SQLSMALLINT		  nHandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  nRecNumber,
  SQLSMALLINT		  nDiagIdentifier,
  SQLPOINTER		  pDiagInfoPtr,
  SQLSMALLINT		  nBufferLength,
  SQLSMALLINT		* pnStringLengthPtr,
  SQLCHAR		  waMode)
{
  GENV (genv, Handle);
  CONN (con, Handle);
  STMT (stmt, Handle);
  DESC (desc, Handle);
  HERR err;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode = SQL_SUCCESS;
  SQLHANDLE dhandle = SQL_NULL_HANDLE;
  SWORD unicode_driver = 0;
  void *_DiagInfoPtr = NULL;
  void *diagInfoPtr = pDiagInfoPtr;


  switch (nHandleType)
    {
    case SQL_HANDLE_ENV:
      if (!IS_VALID_HENV (Handle))
	{
	  return SQL_INVALID_HANDLE;
	}
      err = genv->herr;
      con = NULL;
      stmt = NULL;
      desc = NULL;
      break;

    case SQL_HANDLE_DBC:
      if (!IS_VALID_HDBC (Handle))
	{
	  return SQL_INVALID_HANDLE;
	}
      err = con->herr;
      genv = (GENV_t *) con->genv;
      stmt = NULL;
      desc = NULL;
      dhandle = con->dhdbc;
      break;

    case SQL_HANDLE_STMT:
      if (!IS_VALID_HSTMT (Handle))
	{
	  return SQL_INVALID_HANDLE;
	}
      err = stmt->herr;
      con = (DBC_t *) stmt->hdbc;
      genv = (GENV_t *) con->genv;
      desc = NULL;
      dhandle = stmt->dhstmt;
      break;

    case SQL_HANDLE_DESC:
      if (!IS_VALID_HDESC (Handle))
	{
	  return SQL_INVALID_HANDLE;
	}
      err = desc->herr;
      stmt = (STMT_t *) desc->hstmt;
      con = (DBC_t *) desc->hdbc;
      genv = (GENV_t *) con->genv;
      dhandle = desc->dhdesc;
      break;

    default:
      return SQL_INVALID_HANDLE;
    }

  if (con != NULL && con->henv != SQL_NULL_HENV)
    unicode_driver = ((ENV_t *) con->henv)->unicode_driver;

  switch (nRecNumber)
    {

    case 0:			/* Header record */
      switch (nDiagIdentifier)
	{
	case SQL_DIAG_ROW_COUNT:
	  {
	    if (nHandleType != SQL_HANDLE_STMT || !stmt)
	      {
		return SQL_ERROR;
	      }

	    if (stmt->state != en_stmt_executed_with_info &&
	    	stmt->state != en_stmt_executed &&
		stmt->state != en_stmt_cursoropen)
	      {
		return SQL_ERROR;
	      }
	    if (!con)
	      {
		return SQL_INVALID_HANDLE;
	      }

            CALL_UDRIVER(con, stmt, retcode, hproc, unicode_driver, en_GetDiagField,
              (SQL_HANDLE_DBC, stmt->dhstmt, nRecNumber, nDiagIdentifier, 
               pDiagInfoPtr, nBufferLength, pnStringLengthPtr ));
            if (hproc == SQL_NULL_HPROC)
              {
		if (!con)
		  {
		    return SQL_INVALID_HANDLE;
		  }
		hproc = _iodbcdm_getproc (con, en_RowCount);
		if (!hproc)
		  {
		    return SQL_ERROR;
		  }
		CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc, en_RowCount,
		    (stmt->dhstmt, pDiagInfoPtr));
              }
	    return retcode;
	  }

	case SQL_DIAG_CURSOR_ROW_COUNT:
	case SQL_DIAG_DYNAMIC_FUNCTION:
	case SQL_DIAG_DYNAMIC_FUNCTION_CODE:

	  {
	    if (nHandleType != SQL_HANDLE_STMT || !stmt)
	      {
		return SQL_ERROR;
	      }

	    if (stmt->state != en_stmt_executed_with_info &&
	    	stmt->state != en_stmt_executed &&
		stmt->state != en_stmt_cursoropen)
	      {
		return SQL_ERROR;
	      }
	    if (!con)
	      {
		return SQL_INVALID_HANDLE;
	      }

            CALL_UDRIVER(con, stmt, retcode, hproc, unicode_driver, en_GetDiagField,
              (SQL_HANDLE_DBC, stmt->dhstmt, nRecNumber, nDiagIdentifier, 
               pDiagInfoPtr, nBufferLength, pnStringLengthPtr ));
            if (hproc == SQL_NULL_HPROC)
              return SQL_ERROR;
            else
	      return retcode;
	  }

	case SQL_DIAG_RETURNCODE:

	  if (pDiagInfoPtr)
	    *((SQLRETURN *) pDiagInfoPtr) = ((GENV_t *) Handle)->rc;
	  {
	    return SQL_SUCCESS;
	  }

	case SQL_DIAG_NUMBER:

	  if (pDiagInfoPtr)
	    {
	      (*(SQLINTEGER *) pDiagInfoPtr) = 0;
	      /* get the number from the driver */
	      if (con)
		{
                  CALL_UDRIVER(con, Handle, retcode, hproc, unicode_driver, en_GetDiagField,
                    (nHandleType, dhandle, 0, nDiagIdentifier, 
                     pDiagInfoPtr, nBufferLength, pnStringLengthPtr ));
                  if (hproc != SQL_NULL_HPROC)
                    {
		      if (retcode != SQL_SUCCESS)
			{
			  return retcode;
			}

		      /* and add the DM's value */
		      (*(SQLINTEGER *) pDiagInfoPtr) += error_rec_count (err);
                    }
		  else if (((ENV_t *) con->henv)->dodbc_ver == SQL_OV_ODBC2 &&
		      ((GENV_t *) Handle)->rc)
		    {		/* ODBC2 drivers can only have one error */
		      (*(SQLINTEGER *) pDiagInfoPtr) = 1;
		    }
		}
	      else if (genv)
		{
		  (*(SQLINTEGER *) pDiagInfoPtr) = error_rec_count (err);
		}

	    }
	  break;

	default:
	  return SQL_ERROR;
	}
      break;

    default:			/* status records */
      {
	int nRecs = 0;

	if (nRecNumber < 1)
	  {
	    return SQL_ERROR;
	  }
	nRecs = error_rec_count (err);
	if (nRecNumber <= nRecs)
	  {			/* DM Errors */
	    char *szval = "";
	    int ival = 0;
	    int isInt = 0;
	    sqlerr_t *rec = NULL;

	    rec = get_nth_error (err, nRecNumber - 1);

	    if (!rec)
	      {
		return (SQL_NO_DATA_FOUND);
	      }

	    switch (nDiagIdentifier)
	      {

	      case SQL_DIAG_SUBCLASS_ORIGIN:
	      case SQL_DIAG_CLASS_ORIGIN:
		isInt = 0;

		szval = (rec->code >= en_HY001
		    && rec->code <= en_IM014) ? (char *) "ODBC 3.0" : (char *) "ISO 9075";
		break;

	      case SQL_DIAG_COLUMN_NUMBER:

		if (nHandleType != SQL_HANDLE_STMT || !stmt)
		  {
		    return SQL_ERROR;
		  }
		if (!con)
		  {
		    return SQL_INVALID_HANDLE;
		  }

		if (pDiagInfoPtr)
		  *((SQLINTEGER *) pDiagInfoPtr) = SQL_COLUMN_NUMBER_UNKNOWN;

		return SQL_SUCCESS;

	      case SQL_DIAG_CONNECTION_NAME:
	      case SQL_DIAG_SERVER_NAME:

		isInt = 0;
		if (con)
		  {
		    if (waMode != 'W')
		       retcode = SQLGetInfo (con, SQL_DATA_SOURCE_NAME, 
		          pDiagInfoPtr,	nBufferLength, pnStringLengthPtr);
		    else
		       retcode = SQLGetInfoW (con, SQL_DATA_SOURCE_NAME, 
		          pDiagInfoPtr,	nBufferLength, pnStringLengthPtr);

		    return retcode;
		  }
		else
		  break;

	      case SQL_DIAG_MESSAGE_TEXT:

		isInt = 0;
		szval =
		    _iodbcdm_getsqlerrmsg (rec, (void *) sqlerrmsg_tab);
		break;

	      case SQL_DIAG_NATIVE:

		isInt = 1;
		ival = 0;
		break;

	      case SQL_DIAG_ROW_NUMBER:

		isInt = 1;
		if (nHandleType != SQL_HANDLE_STMT || !stmt)
		  {
		    return SQL_ERROR;
		  }
		if (!con)
		  {
		    return SQL_INVALID_HANDLE;
		  }
                CALL_UDRIVER(con, Handle, retcode, hproc, unicode_driver, en_GetDiagField,
                  (nHandleType, dhandle, nRecNumber, nDiagIdentifier, 
                   pDiagInfoPtr, nBufferLength, pnStringLengthPtr ));
               if (hproc != SQL_NULL_HPROC)
                 {
		    return retcode;
                 }
               else
                 {
		    ival = SQL_ROW_NUMBER_UNKNOWN;
		    break;
                 }

	      case SQL_DIAG_SQLSTATE:

		isInt = 0;
		szval = _iodbcdm_getsqlstate (rec, (void *) sqlerrmsg_tab);
		break;

	      default:
		return SQL_ERROR;
	      }
	    if (isInt)
	      {
		if (pDiagInfoPtr)
		  *((SQLINTEGER *) pDiagInfoPtr) = ival;
	      }
	    else
	      {
	        if (waMode != 'W')
	          {
		    int len = strlen (szval), len1;
		    len1 = len > nBufferLength ? nBufferLength : len;
		    if (pnStringLengthPtr)
		      *pnStringLengthPtr = len;
		    if (pDiagInfoPtr)
		      {
		        STRNCPY (pDiagInfoPtr, szval, len1);
		        *(((SQLCHAR *) pDiagInfoPtr) + len1) = 0;
		      }
		  }
		else
		  {
		    dm_StrCopyOut2_A2W((SQLCHAR *) szval, 
		    	(SQLWCHAR *) pDiagInfoPtr, nBufferLength, pnStringLengthPtr);
		  }
	      
	      }
	    break;
	  }
	else
	  {			/* Driver's errors */
	    nRecNumber -= nRecs;

	    if (!con)
	      {
		return SQL_NO_DATA_FOUND;
	      }

            if ((unicode_driver && waMode != 'W') 
                || (!unicode_driver && waMode == 'W'))
              {
                switch(nDiagIdentifier)
                  {
                  case SQL_DIAG_DYNAMIC_FUNCTION:
                  case SQL_DIAG_CLASS_ORIGIN:
                  case SQL_DIAG_CONNECTION_NAME:
                  case SQL_DIAG_MESSAGE_TEXT:
                  case SQL_DIAG_SERVER_NAME:
                  case SQL_DIAG_SQLSTATE:
                  case SQL_DIAG_SUBCLASS_ORIGIN:
                    if (waMode != 'W')
                      {
                      /* ansi=>unicode*/
                        if ((_DiagInfoPtr = malloc((nBufferLength + 1) * 
                               sizeof(wchar_t))) == NULL)
                          {
                            return SQL_ERROR;
                          }
                      }
                    else
                      {
                      /* unicode=>ansi*/
                        if ((_DiagInfoPtr = malloc(nBufferLength + 1)) == NULL)
                          {
                            return SQL_ERROR;
                          }
                      }
                    diagInfoPtr = _DiagInfoPtr;
                    break;
                  }
              }

            CALL_UDRIVER(con, Handle, retcode, hproc, unicode_driver, en_GetDiagField,
              (nHandleType, dhandle, nRecNumber, nDiagIdentifier, 
               diagInfoPtr, nBufferLength, pnStringLengthPtr ));
            if (hproc != SQL_NULL_HPROC)
              {
                if (pDiagInfoPtr
                    && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                    &&  ((unicode_driver && waMode != 'W')
                        || (!unicode_driver && waMode == 'W')))
                  {
                    switch(nDiagIdentifier)
                      {
                      case SQL_DIAG_DYNAMIC_FUNCTION:
                      case SQL_DIAG_CLASS_ORIGIN:
                      case SQL_DIAG_CONNECTION_NAME:
                      case SQL_DIAG_MESSAGE_TEXT:
                      case SQL_DIAG_SERVER_NAME:
                      case SQL_DIAG_SQLSTATE:
                      case SQL_DIAG_SUBCLASS_ORIGIN:
                        if (waMode != 'W')
                          {
                          /* ansi<=unicode*/
                            dm_StrCopyOut2_W2A ((SQLWCHAR *) diagInfoPtr, 
				(SQLCHAR *) pDiagInfoPtr, 
				nBufferLength, pnStringLengthPtr);
                          }
                        else
                          {
                          /* unicode<=ansi*/
                            dm_StrCopyOut2_A2W ((SQLCHAR *)diagInfoPtr, 
			    	(SQLWCHAR *) pDiagInfoPtr, 
				nBufferLength, pnStringLengthPtr);
                          }
                      }
                  }

                MEM_FREE(_DiagInfoPtr);
		return retcode;
              }
            else
	      {			/* an ODBC2->ODBC3 translation */
		char *szval = "";
		wchar_t szState[6];
		SQLINTEGER nNative;

		if (nRecNumber > 1)
		  {
                    MEM_FREE(_DiagInfoPtr);
		    return SQL_NO_DATA_FOUND;
		  }

		if (nHandleType == SQL_HANDLE_DESC)
		  {
                    MEM_FREE(_DiagInfoPtr);
		    return SQL_INVALID_HANDLE;
		  }

		if (nDiagIdentifier != SQL_DIAG_MESSAGE_TEXT)
                   MEM_FREE(_DiagInfoPtr);

		switch (nDiagIdentifier)
		  {
		  case SQL_DIAG_SUBCLASS_ORIGIN:
		  case SQL_DIAG_CLASS_ORIGIN:
		    
		    CALL_UDRIVER (con, Handle, retcode, hproc, unicode_driver,
		      en_Error, (SQL_NULL_HENV,
		       nHandleType == SQL_HANDLE_DBC ? dhandle : SQL_NULL_HDBC,
		       nHandleType == SQL_HANDLE_STMT ? dhandle : SQL_NULL_HSTMT,
 		       szState, &nNative, NULL, 0, NULL));
                    if (hproc == SQL_NULL_HPROC)
                      {
		        return SQL_INVALID_HANDLE;
                      }
		    if (retcode != SQL_SUCCESS)
		      {
			return SQL_NO_DATA_FOUND;
		      }
		    if (waMode != 'W')
                      {
		        szval = !STRNEQ (szState, "IM", 2) ? (char *) "ODBC 3.0" : (char *) "ISO 9075";
                      }
		    else
                      {
                        if (szState[0] != L'I' && szState[1] != L'M')
		          szval = (char *) "ODBC 3.0";
                        else
		          szval = (char *) "ISO 9075";
                      }
		    break;

		  case SQL_DIAG_ROW_NUMBER:
		  case SQL_DIAG_COLUMN_NUMBER:
		    if (nHandleType != SQL_HANDLE_STMT || !stmt)
		      {
			return SQL_ERROR;
		      }
		    if (!con)
		      {
			return SQL_INVALID_HANDLE;
		      }
		    if (pDiagInfoPtr)
		      *((SQLINTEGER *) pDiagInfoPtr) =
			  SQL_COLUMN_NUMBER_UNKNOWN;
		    {
		      return SQL_SUCCESS;
		    }

		  case SQL_DIAG_SERVER_NAME:
		  case SQL_DIAG_CONNECTION_NAME:
		    break;

		  case SQL_DIAG_MESSAGE_TEXT:
		    CALL_UDRIVER (con, Handle, retcode, hproc, unicode_driver,
		      en_Error, (SQL_NULL_HENV,
		      nHandleType == SQL_HANDLE_DBC ? dhandle : SQL_NULL_HDBC,
		      nHandleType == SQL_HANDLE_STMT ? dhandle : SQL_NULL_HSTMT,
 		      szState, &nNative, diagInfoPtr, nBufferLength, 
 		      pnStringLengthPtr));
                    if (hproc == SQL_NULL_HPROC)
                      {
                        MEM_FREE(_DiagInfoPtr);
		        return SQL_INVALID_HANDLE;
                      }
                    if (pDiagInfoPtr
                        && (retcode == SQL_SUCCESS 
                            || retcode == SQL_SUCCESS_WITH_INFO)
                        &&  ((unicode_driver && waMode != 'W')
                            || (!unicode_driver && waMode == 'W')))
                      {
                        if (waMode != 'W')
                          {
                          /* ansi<=unicode*/
                            dm_StrCopyOut2_W2A ((SQLWCHAR *) diagInfoPtr, 
				(SQLCHAR *) pDiagInfoPtr, 
		      		nBufferLength, pnStringLengthPtr);
                          }
                        else
                          {
                          /* unicode<=ansi*/
                            dm_StrCopyOut2_A2W ((SQLCHAR *)diagInfoPtr, 
			    	(SQLWCHAR *) pDiagInfoPtr, 
				nBufferLength, pnStringLengthPtr);
                          }
                      }

                    MEM_FREE(_DiagInfoPtr);
		    return retcode;

		  case SQL_DIAG_NATIVE:
		    CALL_UDRIVER (con, Handle, retcode, hproc, unicode_driver,
		      en_Error, (SQL_NULL_HENV,
		      nHandleType == SQL_HANDLE_DBC ? dhandle : SQL_NULL_HDBC,
		      nHandleType == SQL_HANDLE_STMT ? dhandle : SQL_NULL_HSTMT,
 		      szState, &nNative, NULL, 0, NULL));
                    if (hproc == SQL_NULL_HPROC)
                      {
		        return SQL_INVALID_HANDLE;
                      }
		    if (pDiagInfoPtr)
		      *((SQLINTEGER *) pDiagInfoPtr) = nNative;
		    return retcode;

		  case SQL_DIAG_SQLSTATE:
		    CALL_UDRIVER (con, Handle, retcode, hproc, unicode_driver,
		      en_Error, (SQL_NULL_HENV,
		      nHandleType == SQL_HANDLE_DBC ? dhandle : SQL_NULL_HDBC,
		      nHandleType == SQL_HANDLE_STMT ? dhandle : SQL_NULL_HSTMT,
 		      szState, &nNative, NULL, 0, NULL));
                    if (hproc == SQL_NULL_HPROC)
                      {
		        return SQL_INVALID_HANDLE;
                      }
                    if (pDiagInfoPtr
                        && (retcode == SQL_SUCCESS 
                            || retcode == SQL_SUCCESS_WITH_INFO)
                        &&  ((unicode_driver && waMode != 'W')
                            || (!unicode_driver && waMode == 'W')))
                      {
                        if (waMode != 'W')
                          {
                          /* ansi<=unicode*/
                            dm_StrCopyOut2_W2A ((SQLWCHAR *) szState, 
				(SQLCHAR *) pDiagInfoPtr, 
		      		nBufferLength, pnStringLengthPtr);
                          }
                        else
                          {
                          /* unicode<=ansi*/
                            dm_StrCopyOut2_A2W ((SQLCHAR *)szState, 
			    	(SQLWCHAR *) pDiagInfoPtr, 
				nBufferLength, pnStringLengthPtr);
                          }
                      }

		    return retcode;

		  default:
		    return SQL_ERROR;
		  }

	        if (waMode != 'W')
	          {
		    if (pDiagInfoPtr)
		      {
		        int len = strlen (szval);
		        if (len > nBufferLength)
		          len = nBufferLength;
		        if (len)
		          strncpy ((char *) pDiagInfoPtr, szval, len);
		      }
		    if (pnStringLengthPtr)
		      *pnStringLengthPtr = strlen (szval);
		  }
		else
		  {
		    dm_StrCopyOut2_A2W((SQLCHAR *) szval, 
		    	(SQLWCHAR *) pDiagInfoPtr, 
			nBufferLength, pnStringLengthPtr);
		  }
	      }			/* ODBC3->ODBC2 */
	  }			/* driver's errors */
      }				/* status records */
    }				/* switch (nRecNumber */
  return (SQL_SUCCESS);
}


RETCODE SQL_API
SQLGetDiagField (
  SQLSMALLINT		  HandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  DiagIdentifier,
  SQLPOINTER		  DiagInfoPtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();

  TRACE (trace_SQLGetDiagField (TRACE_ENTER,
  	HandleType, 
	Handle, 
	RecNumber, 
	DiagIdentifier,
	DiagInfoPtr, BufferLength, StringLengthPtr));

  retcode = SQLGetDiagField_Internal (
  	HandleType, 
	Handle, 
	RecNumber, 
	DiagIdentifier,
	DiagInfoPtr, BufferLength, StringLengthPtr, 
	'A');

  TRACE (trace_SQLGetDiagField (TRACE_LEAVE,
  	HandleType, 
	Handle, 
	RecNumber, 
	DiagIdentifier,
	DiagInfoPtr, BufferLength, StringLengthPtr));

  ODBC_UNLOCK ();

  return retcode;
}


RETCODE SQL_API
SQLGetDiagFieldA (
  SQLSMALLINT		  HandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  DiagIdentifier,
  SQLPOINTER		  DiagInfoPtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();

  TRACE (trace_SQLGetDiagField (TRACE_ENTER,
  	HandleType, 
	Handle, 
	RecNumber, 
	DiagIdentifier,
	DiagInfoPtr, BufferLength, StringLengthPtr));

  retcode = SQLGetDiagField_Internal (
  	HandleType, 
	Handle, 
	RecNumber, 
	DiagIdentifier,
	DiagInfoPtr, BufferLength, StringLengthPtr, 
	'A');

  TRACE (trace_SQLGetDiagField (TRACE_LEAVE,
  	HandleType, 
	Handle, 
	RecNumber, 
	DiagIdentifier,
	DiagInfoPtr, BufferLength, StringLengthPtr));

  ODBC_UNLOCK ();

  return retcode;
}


RETCODE SQL_API
SQLGetDiagFieldW (
  SQLSMALLINT		  HandleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  DiagIdentifier,
  SQLPOINTER		  DiagInfoPtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();

  TRACE (trace_SQLGetDiagFieldW (TRACE_ENTER,
  	HandleType, 
	Handle, 
	RecNumber, 
	DiagIdentifier,
	DiagInfoPtr, BufferLength, StringLengthPtr));

  retcode = SQLGetDiagField_Internal (
  	HandleType, 
	Handle, 
	RecNumber, 
	DiagIdentifier,
	DiagInfoPtr, BufferLength, StringLengthPtr, 
	'W');

  TRACE (trace_SQLGetDiagFieldW (TRACE_LEAVE,
  	HandleType, 
	Handle, 
	RecNumber, 
	DiagIdentifier,
	DiagInfoPtr, BufferLength, StringLengthPtr));

  ODBC_UNLOCK ();

  return retcode;
}
#endif
