/*
 *  connect.c
 *
 *  $Id: connect.c,v 1.44 2006/01/24 00:09:25 source Exp $
 *
 *  Connect (load) driver
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
#include <odbcinst.h>

#include <dlproc.h>

#include <herr.h>
#include <henv.h>
#include <hdbc.h>
#include <hstmt.h>

#include <itrace.h>

#include <unicode.h>

#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
#include <Carbon/Carbon.h>
#endif


/*
 *  Identification strings
 */
static char sccsid[] = "@(#)iODBC driver manager " VERSION "\n";
char *iodbc_version = VERSION;


/*
 *  Prototypes
 */
extern SQLRETURN _iodbcdm_driverunload (HDBC hdbc);
extern SQLRETURN SQL_API _iodbcdm_SetConnectOption (SQLHDBC hdbc,
    SQLUSMALLINT fOption, SQLULEN vParam, SQLCHAR waMode);

extern char * _iodbcdm_getkeyvalinstr (char *cnstr, int cnlen,
    char *keywd, char *value, int size);
extern wchar_t * _iodbcdm_getkeyvalinstrw (wchar_t *cnstr, int cnlen,
    wchar_t *keywd, wchar_t *value, int size);


#define CHECK_DRVCONN_DIALBOXW(path) \
  { \
    if ((handle = DLL_OPEN(path)) != NULL) \
      { \
        if (DLL_PROC(handle, "_iodbcdm_drvconn_dialboxw") != NULL) \
          { \
            DLL_CLOSE(handle); \
            retVal = TRUE; \
            goto quit; \
          } \
        else \
          { \
            if (DLL_PROC(handle, "_iodbcdm_drvconn_dialbox") != NULL) \
              { \
                DLL_CLOSE(handle); \
                retVal = TRUE; \
                goto quit; \
              } \
          } \
        DLL_CLOSE(handle); \
      } \
  }



static BOOL
_iodbcdm_CheckDriverLoginDlg (
    SQLPOINTER  drv,
    SQLPOINTER  dsn,
    UCHAR       waMode
)
{
  wchar_t *szDSN = NULL;
  wchar_t *szDriver = NULL;
  wchar_t tokenstr[4096];
  char *_szdriver_u8 = NULL;
  char drvbuf[4096] = { L'\0'};
  HDLL handle;
  BOOL retVal = FALSE;

  if (waMode != 'W')
   {
     szDSN = dm_SQL_A2W((SQLCHAR *)dsn, SQL_NTS);
     szDriver = dm_SQL_A2W((SQLCHAR *)drv, SQL_NTS);
   }

  /* Check if the driver is provided */
  if (szDriver == NULL)
    {
      SQLSetConfigMode (ODBC_BOTH_DSN);
      SQLGetPrivateProfileStringW (L"ODBC Data Sources",
        szDSN && szDSN[0] != L'\0' ? szDSN : L"default",
        L"", tokenstr, sizeof (tokenstr)/sizeof(wchar_t), NULL);
      szDriver = tokenstr;
    }

  /* Call the iodbcdm_drvconn_dialbox */
  _szdriver_u8 = dm_SQL_W2A (szDriver, SQL_NTS);

  SQLSetConfigMode (ODBC_USER_DSN);
  if (!access (_szdriver_u8, X_OK))
    { CHECK_DRVCONN_DIALBOXW (_szdriver_u8); }
  if (SQLGetPrivateProfileString (_szdriver_u8, "Driver", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOXW (drvbuf); }
  if (SQLGetPrivateProfileString (_szdriver_u8, "Setup", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOXW (drvbuf); }
  if (SQLGetPrivateProfileString ("Default", "Driver", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOXW (drvbuf); }
  if (SQLGetPrivateProfileString ("Default", "Setup", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOXW (drvbuf); }


  SQLSetConfigMode (ODBC_SYSTEM_DSN);
  if (!access (_szdriver_u8, X_OK))
    { CHECK_DRVCONN_DIALBOXW (_szdriver_u8); }
  if (SQLGetPrivateProfileString (_szdriver_u8, "Driver", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOXW (drvbuf); }
  if (SQLGetPrivateProfileString (_szdriver_u8, "Setup", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOXW (drvbuf); }
  if (SQLGetPrivateProfileString ("Default", "Driver", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOXW (drvbuf); }
  if (SQLGetPrivateProfileString ("Default", "Setup", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOXW (drvbuf); }

quit:

  MEM_FREE (_szdriver_u8);

  return retVal;
}


static SQLRETURN
_iodbcdm_SetConnectOption_init (
    SQLHDBC		  hdbc,
    SQLUSMALLINT	  fOption,
    SQLULEN		  vParam,
    UCHAR		  waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  int retinfo = 0;

  SQLINTEGER strLength = 0;
  void *ptr = (void *) vParam;
  void *_vParam = NULL;

  if (fOption >= 1000)
    {
      retinfo = 1;		/* Change SQL_ERROR -> SQL_SUCCESS_WITH_INFO */
    }

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
            ptr = _vParam;
            strLength = SQL_NTS;
            break;
        }
    }

  if (penv->unicode_driver)
    {
      /* SQL_XXX_W */
      if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectOptionW))
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (hdbc, pdbc, retcode, hproc,
              en_SetConnectOptionW, (pdbc->dhdbc, fOption, ptr));
        }
#if (ODBCVER >= 0x300)
      else
      if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectAttrW))
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (hdbc, pdbc, retcode, hproc,
              en_SetConnectAttrW, (pdbc->dhdbc, fOption, ptr, strLength));
        }
#endif
    }
  else
    {
      /* SQL_XXX */
      /* SQL_XXX_A */
      if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectOption))
           != SQL_NULL_HPROC)
        {
          CALL_DRIVER (hdbc, pdbc, retcode, hproc,
	          en_SetConnectOption, (pdbc->dhdbc, fOption, vParam));
        }
      else
      if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectOptionA))
           != SQL_NULL_HPROC)
        {
          CALL_DRIVER (hdbc, pdbc, retcode, hproc,
              en_SetConnectOptionA, (pdbc->dhdbc, fOption, vParam));
        }
#if (ODBCVER >= 0x300)
      else
      if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectAttr))
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (hdbc, pdbc, retcode, hproc,
              en_SetConnectAttr, (pdbc->dhdbc, fOption, vParam, strLength));
        }
      else
      if ((hproc = _iodbcdm_getproc (pdbc, en_SetConnectAttrA))
          != SQL_NULL_HPROC)
        {
          CALL_DRIVER (hdbc, pdbc, retcode, hproc,
             en_SetConnectAttrA,(pdbc->dhdbc, fOption, vParam, strLength));
        }
#endif
    }
  MEM_FREE(_vParam);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pdbc->herr, en_IM004);
      return SQL_SUCCESS_WITH_INFO;
    }

  if (retcode != SQL_SUCCESS && retinfo)
    return SQL_SUCCESS_WITH_INFO;

  return retcode;
}


static SQLRETURN
_iodbcdm_getInfo_init (SQLHDBC hdbc,
    SQLUSMALLINT fInfoType,
    SQLPOINTER rgbInfoValue,
    SQLSMALLINT cbInfoValueMax,
    SQLSMALLINT * pcbInfoValue,
    SQLCHAR waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;

  waMode = waMode; /*UNUSED*/

  switch(fInfoType)
    {
      case SQL_CURSOR_COMMIT_BEHAVIOR:
      case SQL_CURSOR_ROLLBACK_BEHAVIOR:
        break;
      default:
        return SQL_ERROR;
    }

  CALL_UDRIVER(hdbc, pdbc, retcode, hproc, penv->unicode_driver,
    en_GetInfo, (
       pdbc->dhdbc,
       fInfoType,
       rgbInfoValue,
       cbInfoValueMax,
       pcbInfoValue));

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pdbc->herr, en_IM004);
      return SQL_SUCCESS_WITH_INFO;
    }

  return retcode;
}


/* - Load driver share library( or increase its reference count
 *   if it has already been loaded by another active connection)
 * - Call driver's SQLAllocEnv() (for the first reference only)
 * - Call driver's SQLAllocConnect()
 * - Call driver's SQLSetConnectOption() (set login time out)
 * - Increase the bookkeeping reference count
 */
SQLRETURN
_iodbcdm_driverload (
    char * path,
    HDBC hdbc,
    SWORD thread_safe,
    SWORD unload_safe,
    UCHAR waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, NULL);
  GENV (genv, NULL);
  HDLL hdll = SQL_NULL_HDLL;
  HPROC hproc;
  SQLRETURN retcode = SQL_SUCCESS;
  sqlstcode_t sqlstat = en_00000;
  char driverbuf[1024];

  if (path == NULL || ((char *) path)[0] == '\0')
    {
      PUSHSQLERR (pdbc->herr, en_IM002);
      return SQL_ERROR;
    }

  if (!IS_VALID_HDBC (pdbc) || pdbc->genv == SQL_NULL_HENV)
    {
      return SQL_INVALID_HANDLE;
    }

  /*
   *  If path does not start with / or ., we may have a symbolic driver name
   */
  if (!(path[0] == '/' || path[0] == '.'))
    {
      char *tmp_path = NULL;

      /*
       *  Remove curly braces
       */
      if (path[0] == '{')
	{
	  tmp_path = strdup (path);
	  if (tmp_path[strlen (path) - 1] == '}')
	    tmp_path[strlen (path) - 1] = '\0';
	  path = &tmp_path[1];
	}

      /*
       *  Hopefully the driver was registered under that name in the 
       *  odbcinst.ini file
       */
      if (SQLGetPrivateProfileString ((char *) path, "Driver", "",
	      driverbuf, sizeof (driverbuf), "odbcinst.ini") && driverbuf[0])
	path = driverbuf;

      if (tmp_path)
	free (tmp_path);
    }

  genv = (GENV_t *) pdbc->genv;

  /* This will either load the driver dll or increase its reference count */
  hdll = _iodbcdm_dllopen ((char *) path);

  /* Set flag if it is safe to unload the driver after use */
  if (unload_safe)
    _iodbcdm_safe_unload (hdll);

  if (hdll == SQL_NULL_HDLL)
    {
      PUSHSYSERR (pdbc->herr, _iodbcdm_dllerror ());
      PUSHSQLERR (pdbc->herr, en_IM003);
      return SQL_ERROR;
    }

  penv = (ENV_t *) (pdbc->henv);

  if (penv != NULL)
    {
      if (penv->hdll != hdll)
	{
	  _iodbcdm_driverunload (hdbc);
	  penv->hdll = hdll;
	}
      else
	{
	  /*
	   * this will not unload the driver but only decrease its internal
	   * reference count
	   */
	  _iodbcdm_dllclose (hdll);
	}
    }

  if (penv == NULL)
    {
      /*
       * find out whether this dll has already been loaded on another
       * connection
       */
      for (penv = (ENV_t *) genv->henv;
	  penv != NULL;
	  penv = (ENV_t *) penv->next)
	{
	  if (penv->hdll == hdll)
	    {
	      /*
	       * this will not unload the driver but only decrease its internal
	       * reference count
	       */
	      _iodbcdm_dllclose (hdll);
	      break;
	    }
	}

      if (penv == NULL)
	/* no connection attaching with this dll */
	{
	  int i;

	  /* create a new dll env instance */
	  penv = (ENV_t *) MEM_ALLOC (sizeof (ENV_t));

	  if (penv == NULL)
	    {
	      _iodbcdm_dllclose (hdll);

	      PUSHSQLERR (pdbc->herr, en_S1001);

	      return SQL_ERROR;
	    }

	  /*
	   *  Initialize array of ODBC functions
	   */
	  for (i = 0; i < __LAST_API_FUNCTION__; i++)
	    {
#if 1 
	      (penv->dllproc_tab)[i] = SQL_NULL_HPROC;
#else
	      (penv->dllproc_tab)[i] = _iodbcdm_getproc(pdbc, i);
#endif
	    }

	  pdbc->henv = penv;
	  penv->hdll = hdll;

          /*
           *  If the driver appears not to be thread safe, use a
           *  driver mutex to serialize all calls to this driver
           */
          penv->thread_safe = thread_safe;
          if (!penv->thread_safe)
            MUTEX_INIT (penv->drv_lock);

          penv->unicode_driver = 0;
          /*
           *  If the driver is Unicode
           */
	  if ( _iodbcdm_getproc (pdbc, en_ConnectW))
            penv->unicode_driver = 1;

	  /* call driver's SQLAllocHandle() or SQLAllocEnv() */

#if (ODBCVER >= 0x0300)
	  hproc = _iodbcdm_getproc (pdbc, en_AllocHandle);

	  if (hproc)
	    {
	      CALL_DRIVER (hdbc, genv, retcode, hproc, en_AllocHandle,
		  (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &(penv->dhenv)));
	      if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{		
		  /* 
		   * This appears to be an ODBC3 driver
		   *
		   * Try to set the app's requested version
		   */
		  SQLRETURN save_retcode = retcode;

		  penv->dodbc_ver = SQL_OV_ODBC2;
		  hproc = _iodbcdm_getproc (pdbc, en_SetEnvAttr);
		  if (hproc != SQL_NULL_HPROC)
		    {
		      CALL_DRIVER (hdbc, genv, retcode, hproc, en_SetEnvAttr,
			  (penv->dhenv, SQL_ATTR_ODBC_VERSION, genv->odbc_ver, 
			  0));
		      if (retcode == SQL_SUCCESS)
			penv->dodbc_ver = SQL_OV_ODBC3;
		    }
		  retcode = save_retcode;
		}
	    }
	  else			/* try driver's SQLAllocEnv() */
#endif
	    {
	      hproc = _iodbcdm_getproc (pdbc, en_AllocEnv);

	      if (hproc == SQL_NULL_HPROC)
		{
		  sqlstat = en_IM004;
		}
	      else
		{
#if (ODBCVER >= 0x0300)
		  penv->dodbc_ver = SQL_OV_ODBC2;
#endif
		  CALL_DRIVER (hdbc, genv, retcode, hproc,
		      en_AllocEnv, (&(penv->dhenv)));
		}
	    }

	  if (retcode == SQL_ERROR)
	    {
	      sqlstat = en_IM004;
	    }

	  if (sqlstat != en_00000)
	    {
	      _iodbcdm_dllclose (hdll);
	      MEM_FREE (penv);
	      PUSHSQLERR (pdbc->herr, en_IM004);

	      return SQL_ERROR;
	    }

	  /* insert into dll env list */
	  penv->next = (ENV_t *) genv->henv;
	  genv->henv = penv;

	  /* initiate this new env entry */
	  penv->refcount = 0;	/* we will increase it after
				 * driver's SQLAllocConnect()
				 * success
				 */
	}

      pdbc->henv = penv;

      if (pdbc->dhdbc == SQL_NULL_HDBC)
	{

#if (ODBCVER >= 0x0300)
	  hproc = _iodbcdm_getproc (pdbc, en_AllocHandle);

	  if (hproc)
	    {
	      CALL_DRIVER (hdbc, genv, retcode, hproc, en_AllocHandle,
		  (SQL_HANDLE_DBC, penv->dhenv, &(pdbc->dhdbc)));
	    }
	  else
#endif

	    {
	      hproc = _iodbcdm_getproc (pdbc, en_AllocConnect);

	      if (hproc == SQL_NULL_HPROC)
		{
		  sqlstat = en_IM005;
		}
	      else
		{
		  CALL_DRIVER (hdbc, genv, retcode, hproc,
		      en_AllocConnect, (penv->dhenv, &(pdbc->dhdbc)));
		}
	    }

	  if (retcode == SQL_ERROR)
	    {
	      sqlstat = en_IM005;
	    }

	  if (sqlstat != en_00000)
	    {
	      _iodbcdm_driverunload (hdbc);

	      pdbc->dhdbc = SQL_NULL_HDBC;
	      PUSHSQLERR (pdbc->herr, en_IM005);

	      return SQL_ERROR;
	    }
	}

      pdbc->henv = penv;
      penv->refcount++;		/* bookkeeping reference count on this driver */
    }

  /* driver's login timeout option must been set before
   * its SQLConnect() call */
  if (pdbc->login_timeout != 0UL)
    {
      retcode = _iodbcdm_SetConnectOption_init (hdbc, SQL_LOGIN_TIMEOUT,
	pdbc->login_timeout, waMode);

      if (retcode == SQL_ERROR)
        {
          PUSHSQLERR (pdbc->herr, en_IM006);
          return SQL_SUCCESS_WITH_INFO;
        }
    }

  /*
   *  Now set the driver specific options we saved earlier
   */
  if (pdbc->drvopt != NULL)
    {
      DRVOPT *popt;

      for (popt = pdbc->drvopt; popt != NULL; popt = popt->next)
        {
          retcode = _iodbcdm_SetConnectOption_init (hdbc, popt->Option,
	    popt->Param, popt->waMode);

          if (retcode == SQL_ERROR)
            {
	      PUSHSQLERR (pdbc->herr, en_IM006);
	      return SQL_SUCCESS_WITH_INFO;
	    }
        }
    }

  return SQL_SUCCESS;
}


/* - Call driver's SQLFreeConnect()
 * - Call driver's SQLFreeEnv() ( for the last reference only)
 * - Unload the share library( or decrease its reference
 *   count if it is not the last reference )
 * - decrease bookkeeping reference count
 * - state transition to allocated
 */
SQLRETURN
_iodbcdm_driverunload (HDBC hdbc)
{
  CONN (pdbc, hdbc);
  ENVR (penv, pdbc->henv);
  GENV (genv, pdbc->genv);
  ENV_t *tpenv;
  HPROC hproc;
  SQLRETURN retcode = SQL_SUCCESS;

  if (!IS_VALID_HDBC (pdbc))
    {
      return SQL_INVALID_HANDLE;
    }

  if (penv == NULL || penv->hdll == SQL_NULL_HDLL)
    {
      return SQL_SUCCESS;
    }

#if (ODBCVER >= 0x0300)
  hproc = _iodbcdm_getproc (pdbc, en_FreeHandle);

  if (hproc)
    {
      CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_FreeHandle,
	  (SQL_HANDLE_DBC, pdbc->dhdbc));
    }
  else
#endif

    {
      hproc = _iodbcdm_getproc (pdbc, en_FreeConnect);

      if (hproc != SQL_NULL_HPROC)
	{
	  CALL_DRIVER (hdbc, pdbc, retcode, hproc,
	      en_FreeConnect, (pdbc->dhdbc));

	  pdbc->dhdbc = SQL_NULL_HDBC;
	}
    }

  penv->refcount--;

  if (!penv->refcount)
    /* no other connections still attaching with this driver */
    {

#if (ODBCVER >= 0x0300)
      hproc = _iodbcdm_getproc (pdbc, en_FreeHandle);

      if (hproc)
	{
	  CALL_DRIVER (hdbc, genv, retcode, hproc, en_FreeHandle,
	      (SQL_HANDLE_ENV, penv->dhenv));
	}
      else
#endif

	{
	  hproc = _iodbcdm_getproc (pdbc, en_FreeEnv);

	  if (hproc != SQL_NULL_HPROC)
	    {
	      CALL_DRIVER (hdbc, genv, retcode, hproc, en_FreeEnv,
		  (penv->dhenv));

	      penv->dhenv = SQL_NULL_HENV;
	    }
	}

      _iodbcdm_dllclose (penv->hdll);

      penv->hdll = SQL_NULL_HDLL;

      for (tpenv = (ENV_t *) genv->henv;
	  tpenv != NULL;
	  tpenv = (ENV_t *) penv->next)
	{
	  if (tpenv == penv)
	    {
	      genv->henv = penv->next;
	      break;
	    }

	  if (tpenv->next == penv)
	    {
	      tpenv->next = penv->next;
	      break;
	    }
	}

      MEM_FREE (penv);
    }

  /* pdbc->henv = SQL_NULL_HENV; */
  pdbc->hstmt = SQL_NULL_HSTMT;
  /* pdbc->herr = SQL_NULL_HERR;
     -- delay to DM's SQLFreeConnect() */
  pdbc->dhdbc = SQL_NULL_HDBC;
  pdbc->state = en_dbc_allocated;

  /* set connect options to default values */
	/**********
	pdbc->access_mode	= SQL_MODE_DEFAULT;
	pdbc->autocommit	= SQL_AUTOCOMMIT_DEFAULT;
	pdbc->login_timeout 	= 0UL;
	**********/
  pdbc->odbc_cursors = SQL_CUR_DEFAULT;
  pdbc->packet_size = 0UL;
  pdbc->quiet_mode = (UDWORD) NULL;
  pdbc->txn_isolation = SQL_TXN_READ_UNCOMMITTED;

  if (pdbc->current_qualifier != NULL)
    {
      MEM_FREE (pdbc->current_qualifier);
      pdbc->current_qualifier = NULL;
    }

  return SQL_SUCCESS;
}


static SQLRETURN
_iodbcdm_dbcdelayset (HDBC hdbc, UCHAR waMode)
{
  CONN (pdbc, hdbc);
  SQLRETURN retcode = SQL_SUCCESS;
  SQLRETURN ret;

  if (pdbc->access_mode != SQL_MODE_DEFAULT)
    {
      ret = _iodbcdm_SetConnectOption_init (hdbc, SQL_ACCESS_MODE,
	      pdbc->access_mode, waMode);

      retcode |= ret;
    }

  if (pdbc->autocommit != SQL_AUTOCOMMIT_DEFAULT)
    {
      ret = _iodbcdm_SetConnectOption_init (hdbc, SQL_AUTOCOMMIT,
	      pdbc->autocommit, waMode);

      retcode |= ret;
    }

  if (pdbc->current_qualifier != NULL)
    {
      ret = _iodbcdm_SetConnectOption_init (hdbc, SQL_CURRENT_QUALIFIER,
	      (SQLULEN) pdbc->current_qualifier,
	      pdbc->current_qualifier_WA);

      retcode |= ret;
    }

  if (pdbc->packet_size != 0UL)
    {
      ret = _iodbcdm_SetConnectOption_init (hdbc, SQL_PACKET_SIZE,
	      pdbc->packet_size, waMode);

      retcode |= ret;
    }

  if (pdbc->quiet_mode != (UDWORD) NULL)
    {
      ret = _iodbcdm_SetConnectOption_init (hdbc, SQL_QUIET_MODE,
	      pdbc->quiet_mode, waMode);

      retcode |= ret;
    }

  if (pdbc->txn_isolation != SQL_TXN_READ_UNCOMMITTED)
    {
      ret = _iodbcdm_SetConnectOption_init (hdbc, SQL_TXN_ISOLATION,
	      pdbc->txn_isolation, waMode);

      retcode |= ret;
    }

  /* check error code for driver's SQLSetConnectOption() call */
  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      PUSHSQLERR (pdbc->herr, en_IM006);
      retcode = SQL_ERROR;
    }


  /* get cursor behavior on transaction commit or rollback */
  ret = _iodbcdm_getInfo_init (hdbc, SQL_CURSOR_COMMIT_BEHAVIOR,
	    (PTR) & (pdbc->cb_commit),
	    sizeof (pdbc->cb_commit),
	    NULL, waMode);
  retcode |= ret;

  ret = _iodbcdm_getInfo_init (hdbc, SQL_CURSOR_ROLLBACK_BEHAVIOR,
	    (PTR) & (pdbc->cb_rollback),
	    sizeof (pdbc->cb_rollback),
	    NULL, waMode);
  retcode |= ret;

  if (retcode != SQL_SUCCESS  && retcode != SQL_SUCCESS_WITH_INFO)
    {
      return SQL_ERROR;
    }

  return retcode;
}


static SQLRETURN
_iodbcdm_con_settracing (HDBC hdbc, SQLCHAR *dsn, int dsnlen, UCHAR waMode)
{
  SQLUINTEGER trace = SQL_OPT_TRACE_OFF;
  char buf[1024];

  /* Unused */
  hdbc=hdbc;
  dsnlen=dsnlen;
  waMode = waMode;

  /* Get the TraceFile keyword value from the ODBC section */
  SQLSetConfigMode (ODBC_BOTH_DSN);
  if ((SQLGetPrivateProfileString ((char *) dsn, "TraceFile", "", 
	buf, sizeof (buf), "odbc.ini") == 0 || !buf[0]))
    STRCPY (buf, SQL_OPT_TRACE_FILE_DEFAULT);

  trace_set_filename (buf);	/* UTF-8 */

  /* Get the Trace keyword value from the ODBC section */
  SQLSetConfigMode (ODBC_BOTH_DSN);
  if (SQLGetPrivateProfileString ((char *) dsn, "Trace", "", 
	buf, sizeof (buf), "odbc.ini")
      && (STRCASEEQ (buf, "on") || STRCASEEQ (buf, "yes")
	  || STRCASEEQ (buf, "1")))
    {
      trace = SQL_OPT_TRACE_ON;
    }

  /* Set the trace flag now */
  if (trace == SQL_OPT_TRACE_ON)
    trace_start ();

  return SQL_SUCCESS;
}


static
SQLRETURN SQL_API
SQLConnect_Internal (SQLHDBC hdbc,
    SQLPOINTER szDSN,
    SQLSMALLINT cbDSN,
    SQLPOINTER szUID,
    SQLSMALLINT cbUID,
    SQLPOINTER szAuthStr,
    SQLSMALLINT cbAuthStr,
    SQLCHAR waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, NULL);
  SQLRETURN retcode = SQL_SUCCESS;
  SQLRETURN setopterr = SQL_SUCCESS;
  /* MS SDK Guide specifies driver path can't longer than 255. */
  char driver[1024] = { '\0' };
  char buf[256];
  HPROC hproc = SQL_NULL_HPROC;
  SWORD thread_safe;
  SWORD unload_safe;
  void * _szDSN = NULL;
  void * _szUID = NULL;
  void * _szAuthStr = NULL;
  SQLCHAR *_dsn = (SQLCHAR *) szDSN;
  SQLSMALLINT _dsn_len = cbDSN;

  /* check arguments */
  if ((cbDSN < 0 && cbDSN != SQL_NTS)
      || (cbUID < 0 && cbUID != SQL_NTS)
      || (cbAuthStr < 0 && cbAuthStr != SQL_NTS)
      || (cbDSN > SQL_MAX_DSN_LENGTH))
    {
      PUSHSQLERR (pdbc->herr, en_S1090);
      return SQL_ERROR;
    }

  if (szDSN == NULL || cbDSN == 0)
    {
      PUSHSQLERR (pdbc->herr, en_IM002);
      return SQL_ERROR;
    }

  /* check state */
  if (pdbc->state != en_dbc_allocated)
    {
      PUSHSQLERR (pdbc->herr, en_08002);
      return SQL_ERROR;
    }


  if (waMode == 'W')
    {
      _szDSN = (void *) dm_SQL_WtoU8((SQLWCHAR *)szDSN, cbDSN);
      _dsn = (SQLCHAR *) _szDSN;
      _dsn_len = SQL_NTS;
      if (_dsn == NULL)
        {
          PUSHSQLERR (pdbc->herr, en_S1001);
          return SQL_ERROR;
        }
    }

  /* Get the config mode */
  if (_iodbcdm_con_settracing (pdbc, _dsn, _dsn_len, waMode) == SQL_ERROR)
    {
      MEM_FREE(_szDSN);
      return SQL_ERROR;
    }

  /*
   *  Check whether driver is thread safe
   */
  thread_safe = 1;		/* Assume driver is thread safe */

  SQLSetConfigMode (ODBC_BOTH_DSN);
  if ( SQLGetPrivateProfileString ((char *) _dsn, "ThreadManager", "", 
	buf, sizeof(buf), "odbc.ini") &&
      (STRCASEEQ (buf, "on") || STRCASEEQ (buf, "1")))
    {
      thread_safe = 0;	/* Driver needs a thread manager */
    }

  /*
   *  Check if it is safe to unload the driver
   */
  unload_safe = 0;		/* Assume driver is not unload safe */

  SQLSetConfigMode (ODBC_BOTH_DSN);
  if ( SQLGetPrivateProfileString ((char *) _dsn, "UnloadSafe", "", 
	buf, sizeof(buf), "odbc.ini") &&
      (STRCASEEQ (buf, "on") || STRCASEEQ (buf, "1")))
    {
      unload_safe = 1;
    }


  /*
   *  Get the name of the driver module and load it
   */
  SQLSetConfigMode (ODBC_BOTH_DSN);
  if ( SQLGetPrivateProfileString ((char *) _dsn, "Driver", "", 
	(char *) driver, sizeof(driver), "odbc.ini") == 0)
    /* No specified or default dsn section or
     * no driver specification in this dsn section */
    {
      MEM_FREE(_szDSN);
      _szDSN = NULL;
      PUSHSQLERR (pdbc->herr, en_IM002);
      return SQL_ERROR;
    }

  MEM_FREE(_szDSN);
  _szDSN = NULL;

  retcode = _iodbcdm_driverload ((char *)driver, pdbc, thread_safe, unload_safe, waMode);

  switch (retcode)
    {
    case SQL_SUCCESS:
      break;

    case SQL_SUCCESS_WITH_INFO:
#if 0
      /* 
       *  Unsuccessful in calling driver's SQLSetConnectOption() to set 
       *  login timeout.
       */
      setopterr = SQL_ERROR;
#endif
      break;

    default:
      return retcode;
    }

  penv = (ENV_t *) pdbc->henv;

  if ((penv->unicode_driver && waMode != 'W')
      || (!penv->unicode_driver && waMode == 'W'))
    {
      if (waMode != 'W')
        {
        /* ansi=>unicode*/
          _szDSN = dm_SQL_A2W((SQLCHAR *)szDSN, cbDSN);
          _szUID = dm_SQL_A2W((SQLCHAR *)szUID, cbUID);
          _szAuthStr = dm_SQL_A2W((SQLCHAR *)szAuthStr, cbAuthStr);
        }
      else
        {
        /* unicode=>ansi*/
          _szDSN = dm_SQL_W2A((SQLWCHAR *)szDSN, cbDSN);
          _szUID = dm_SQL_W2A((SQLWCHAR *)szUID, cbUID);
          _szAuthStr = dm_SQL_W2A((SQLWCHAR *)szAuthStr, cbAuthStr);
        }
      cbDSN = SQL_NTS;
      cbUID = SQL_NTS;
      cbAuthStr = SQL_NTS;
      szDSN = _szDSN;
      szUID = _szUID;
      szAuthStr = _szAuthStr;
    }

  CALL_UDRIVER(hdbc, pdbc, retcode, hproc, penv->unicode_driver,
    en_Connect, (
       pdbc->dhdbc,
       szDSN,
       cbDSN,
       szUID,
       cbUID,
       szAuthStr,
       cbAuthStr));

  MEM_FREE(_szDSN);
  MEM_FREE(_szUID);
  MEM_FREE(_szAuthStr);

  if (hproc == SQL_NULL_HPROC)
    {
      _iodbcdm_driverunload (pdbc);
      PUSHSQLERR (pdbc->herr, en_IM001);
      return SQL_ERROR;
    }

  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      /* not unload driver for retrieve error
       * message from driver */
		/*********
		_iodbcdm_driverunload( hdbc );
		**********/

      return retcode;
    }

  /* state transition */
  pdbc->state = en_dbc_connected;

  /* do delayed option setting */
  setopterr |= _iodbcdm_dbcdelayset (pdbc, waMode);

  if (setopterr != SQL_SUCCESS)
    {
      return SQL_SUCCESS_WITH_INFO;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLConnect (
  SQLHDBC		  hdbc,
  SQLCHAR 		* szDSN,
  SQLSMALLINT		  cbDSN,
  SQLCHAR 		* szUID,
  SQLSMALLINT		  cbUID,
  SQLCHAR 		* szAuthStr,
  SQLSMALLINT		  cbAuthStr)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLConnect (TRACE_ENTER,
    	hdbc,
	szDSN, cbDSN,
	szUID, cbUID,
	szAuthStr, cbAuthStr));

  retcode =  SQLConnect_Internal (
  	hdbc,
	szDSN, cbDSN,
	szUID, cbUID,
	szAuthStr, cbAuthStr, 'A');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLConnect (TRACE_LEAVE,
    	hdbc,
	szDSN, cbDSN,
	szUID, cbUID,
	szAuthStr, cbAuthStr));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLConnectA (
  SQLHDBC		  hdbc,
  SQLCHAR 		* szDSN,
  SQLSMALLINT		  cbDSN,
  SQLCHAR 		* szUID,
  SQLSMALLINT		  cbUID,
  SQLCHAR 		* szAuthStr,
  SQLSMALLINT		  cbAuthStr)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLConnect (TRACE_ENTER,
    	hdbc,
	szDSN, cbDSN,
	szUID, cbUID,
	szAuthStr, cbAuthStr));

  retcode =  SQLConnect_Internal (
  	hdbc,
	szDSN, cbDSN,
	szUID, cbUID,
	szAuthStr, cbAuthStr, 'A');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLConnect (TRACE_LEAVE,
    	hdbc,
	szDSN, cbDSN,
	szUID, cbUID,
	szAuthStr, cbAuthStr));
}


SQLRETURN SQL_API
SQLConnectW (SQLHDBC hdbc,
    SQLWCHAR * szDSN,
    SQLSMALLINT cbDSN,
    SQLWCHAR * szUID,
    SQLSMALLINT cbUID,
    SQLWCHAR * szAuthStr,
    SQLSMALLINT cbAuthStr)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLConnectW (TRACE_ENTER,
    	hdbc,
	szDSN, cbDSN,
	szUID, cbUID,
	szAuthStr, cbAuthStr));

  retcode =  SQLConnect_Internal (
  	hdbc,
	szDSN, cbDSN,
	szUID, cbUID,
	szAuthStr, cbAuthStr,
	'W');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLConnectW (TRACE_LEAVE,
    	hdbc,
	szDSN, cbDSN,
	szUID, cbUID,
	szAuthStr, cbAuthStr));
}
#endif


SQLRETURN SQL_API
SQLDriverConnect_Internal (
    SQLHDBC hdbc,
    SQLHWND hwnd,
    SQLPOINTER szConnStrIn,
    SQLSMALLINT cbConnStrIn,
    SQLPOINTER szConnStrOut,
    SQLSMALLINT cbConnStrOutMax,
    SQLPOINTER pcbConnStrOut,
    SQLUSMALLINT fDriverCompletion,
    SQLCHAR waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, NULL);
  HDLL hdll = NULL;
  void *drv;
  SQLWCHAR drvbuf[1024];
  void *dsn;
  SQLWCHAR dsnbuf[SQL_MAX_DSN_LENGTH + 1];
  SQLWCHAR prov[1024];
  SWORD thread_safe;
  SWORD unload_safe;
  char buf[1024];
  HPROC hproc = SQL_NULL_HPROC;
  void *_ConnStrIn = NULL;
  void *_ConnStrOut = NULL;
  void *connStrOut = szConnStrOut;
  void *connStrIn = szConnStrIn;
  char *_dsn_u8 = NULL;
  char *_drv_u8 = NULL;
  UWORD config;
  BOOL bCallDmDlg = FALSE;
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
  CFStringRef libname = NULL;
  CFBundleRef bundle = NULL;
  CFURLRef liburl = NULL;
  char name[1024] = { 0 };
#endif

  HPROC dialproc = SQL_NULL_HPROC;

  sqlstcode_t sqlstat = en_00000;
  SQLRETURN retcode = SQL_SUCCESS;
  SQLRETURN setopterr = SQL_SUCCESS;

  /* check arguments */
  if ((cbConnStrIn < 0 && cbConnStrIn != SQL_NTS) ||
      (cbConnStrOutMax < 0 && cbConnStrOutMax != SQL_NTS))
    {
      PUSHSQLERR (pdbc->herr, en_S1090);
      return SQL_ERROR;
    }

  /* check state */
  if (pdbc->state != en_dbc_allocated)
    {
      PUSHSQLERR (pdbc->herr, en_08002);
      return SQL_ERROR;
    }

  /* Save config mode */
  SQLGetConfigMode (&config);

  if (waMode != 'W')
    {
      drv = _iodbcdm_getkeyvalinstr ((char *) szConnStrIn, cbConnStrIn,
	  "DRIVER", (char *) drvbuf, sizeof (drvbuf));
      dsn = _iodbcdm_getkeyvalinstr ((char *) szConnStrIn, cbConnStrIn,
	  "DSN", (char *) dsnbuf, sizeof (dsnbuf));
    }
  else
    {
      drv = _iodbcdm_getkeyvalinstrw ((wchar_t *) szConnStrIn, cbConnStrIn,
	  L"DRIVER", drvbuf, sizeof (drvbuf) / sizeof (SQLWCHAR));
      dsn = _iodbcdm_getkeyvalinstrw ((wchar_t *) szConnStrIn, cbConnStrIn,
	  L"DSN", dsnbuf, sizeof (dsnbuf) / sizeof (SQLWCHAR));
    }

  switch (fDriverCompletion)
    {
    case SQL_DRIVER_NOPROMPT:
      /* Check if there's a DSN or DRIVER */
      if (!dsn && !drv)
	{
	  PUSHSQLERR (pdbc->herr, en_IM007);
	  return SQL_ERROR;
	}
      break;

    case SQL_DRIVER_COMPLETE:
    case SQL_DRIVER_COMPLETE_REQUIRED:
      if (dsn != NULL || drv != NULL)
	{
	  break;
	}
      /* fall to next case */
    case SQL_DRIVER_PROMPT:
      /* Get data source dialog box function from
       * current executable */
      /* Not really sure here, but should load that from the iodbcadm */
      if (waMode == 'A')
	strncpy ((char *) prov, szConnStrIn, sizeof (prov));
      else
	wcsncpy (prov, szConnStrIn, sizeof (prov) / sizeof (wchar_t));

      if (!dsn && !drv)
        bCallDmDlg = TRUE;
      else if ( _iodbcdm_CheckDriverLoginDlg(drv, dsn, waMode) == FALSE)
        bCallDmDlg = TRUE;

      /* not call iODBC function "iodbcdm_drvconn_dialbox", if there is
       * the function "_iodbcdm_drvconn_dialbox" in the odbc driver,
       * odbc driver must call its function itself
       */
      if (!bCallDmDlg)
        break;

      ODBC_UNLOCK ();
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
      bundle = CFBundleGetBundleWithIdentifier (CFSTR ("org.iodbc.core"));
      if (bundle)
	{
	  /* Search for the drvproxy library */
	  liburl =
	      CFBundleCopyResourceURL (bundle, CFSTR ("iODBCadm.bundle"),
	      NULL, NULL);
	  if (liburl
	      && (libname =
		  CFURLCopyFileSystemPath (liburl, kCFURLPOSIXPathStyle)))
	    {
	      CFStringGetCString (libname, name, sizeof (name),
		  kCFStringEncodingASCII);
	      STRCAT (name, "/Contents/MacOS/iODBCadm");
	      hdll = _iodbcdm_dllopen (name);
	    }
	  if (liburl)
	    CFRelease (liburl);
	  if (libname)
	    CFRelease (libname);
	}
#else
      hdll = _iodbcdm_dllopen ("libiodbcadm.so");
#endif

      if (!hdll)
	break;

      if (waMode != 'W')
	dialproc = _iodbcdm_dllproc (hdll, "iodbcdm_drvconn_dialbox");
      else
	dialproc = _iodbcdm_dllproc (hdll, "iodbcdm_drvconn_dialboxw");

      if (dialproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM008;
	  break;
	}

      retcode = dialproc (hwnd,	/* window or display handle */
	  prov,			/* input/output dsn buf */
	  sizeof (prov) / (waMode == 'A' ? 1 : sizeof (SQLWCHAR)),	/* buf size */
	  &sqlstat,		/* error code */
	  fDriverCompletion,	/* type of completion */
	  &config);		/* config mode */


      ODBC_LOCK ();

      if (retcode != SQL_SUCCESS)
	{
	  if (retcode != SQL_NO_DATA_FOUND)
	    PUSHSQLERR (pdbc->herr, sqlstat);
	  goto end;
	}

      connStrIn = szConnStrIn = prov;

      /*
       * Recalculate length of szConnStrIn if needed, as it may have been
       * changed by iodbcdm_drvconn_dialbox
       */
      if (cbConnStrIn != SQL_NTS)
	{
	  if (waMode != 'W')
	    cbConnStrIn = STRLEN (szConnStrIn);
	  else
	    cbConnStrIn = WCSLEN (szConnStrIn);
	}

      if (waMode != 'W')
	dsn = _iodbcdm_getkeyvalinstr ((char *) prov, STRLEN (prov),
	    "DSN", (char *) dsnbuf, sizeof (dsnbuf));
      else
	dsn = _iodbcdm_getkeyvalinstrw (prov, WCSLEN (prov),
	    L"DSN", dsnbuf, sizeof (dsnbuf) / sizeof (SQLWCHAR));
      break;

    default:
      sqlstat = en_S1110;
      break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);
      return SQL_ERROR;
    }

  if (waMode == 'W')
    {
      if (dsn != NULL)
	{
	  dsn = _dsn_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) dsn, SQL_NTS);
	  if (dsn == NULL)
	    {
	      PUSHSQLERR (pdbc->herr, en_S1001);
	      return SQL_ERROR;
	    }
	}

      if (drv != NULL)
	{
	  drv = _drv_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) drv, SQL_NTS);
	  if (drv == NULL)
	    {
	      PUSHSQLERR (pdbc->herr, en_S1001);
	      return SQL_ERROR;
	    }
	}

    }


  if (dsn == NULL || *(char *) dsn == '\0')
    {
      dsn = (void *) "default";
    }
  else
    /* if you want tracing, you must use a DSN */
    {
      setopterr |=
	  _iodbcdm_con_settracing (pdbc, (SQLCHAR *) dsn, SQL_NTS, waMode);
    }

  /*
   *  Check whether driver is thread safe
   */
  thread_safe = 1;		/* Assume driver is thread safe */

  SQLSetConfigMode (ODBC_BOTH_DSN);
  if (SQLGetPrivateProfileString ((char *) dsn, "ThreadManager", "",
	  buf, sizeof (buf), "odbc.ini")
      && (STRCASEEQ (buf, "on") || STRCASEEQ (buf, "1")))
    {
      thread_safe = 0;		/* Driver needs a thread manager */
    }

  /*
   *  Check whether driver is unload safe
   */
  unload_safe = 0;		/* Assume driver is not unload safe */

  SQLSetConfigMode (ODBC_BOTH_DSN);
  if (SQLGetPrivateProfileString ((char *) dsn, "UnloadSafe", "",
	  buf, sizeof (buf), "odbc.ini")
      && (STRCASEEQ (buf, "on") || STRCASEEQ (buf, "1")))
    {
      unload_safe = 1;
    }

  /*
   *  Get the name of the driver module
   */
  if (drv == NULL || *(char *) drv == '\0')
    {
      SQLSetConfigMode (ODBC_BOTH_DSN);
      if (SQLGetPrivateProfileString ((char *) dsn, "Driver", "",
	      buf, sizeof (buf), "odbc.ini") != 0)
	{
	  drv = buf;
	}
    }

  if (drv == NULL)
    {
      MEM_FREE (_dsn_u8);
      MEM_FREE (_drv_u8);
      PUSHSQLERR (pdbc->herr, en_IM002);
      return SQL_ERROR;
    }

  retcode =
      _iodbcdm_driverload ((char *) drv, pdbc, thread_safe, unload_safe,
      waMode);

  MEM_FREE (_dsn_u8);
  MEM_FREE (_drv_u8);

  switch (retcode)
    {
    case SQL_SUCCESS:
      break;

    case SQL_SUCCESS_WITH_INFO:
#if 0
      /* 
       *  Unsuccessful in calling driver's SQLSetConnectOption() to set 
       *  login timeout.
       */
      setopterr = SQL_ERROR;
#endif
      break;

    default:
      return retcode;
    }

  penv = (ENV_t *) pdbc->henv;

  if ((penv->unicode_driver && waMode != 'W')
      || (!penv->unicode_driver && waMode == 'W'))
    {
      if (waMode != 'W')
	{
	  /* ansi=>unicode */
	  if ((_ConnStrOut =
		  malloc (cbConnStrOutMax * sizeof (SQLWCHAR) + 1)) == NULL)
	    {
	      PUSHSQLERR (pdbc->herr, en_HY001);
	      return SQL_ERROR;
	    }
	  _ConnStrIn = dm_SQL_A2W ((SQLCHAR *) szConnStrIn, cbConnStrIn);
	}
      else
	{
	  /* unicode=>ansi */
	  if ((_ConnStrOut = malloc (cbConnStrOutMax + 1)) == NULL)
	    {
	      PUSHSQLERR (pdbc->herr, en_HY001);
	      return SQL_ERROR;
	    }
	  _ConnStrIn = dm_SQL_W2A ((SQLWCHAR *) szConnStrIn, cbConnStrIn);
	}
      connStrOut = _ConnStrOut;
      connStrIn = _ConnStrIn;
      cbConnStrIn = SQL_NTS;
    }

  /* Restore config mode */
  SQLSetConfigMode (config);

  CALL_UDRIVER (hdbc, pdbc, retcode, hproc, penv->unicode_driver,
      en_DriverConnect, (pdbc->dhdbc,
	  hwnd,
	  connStrIn,
	  cbConnStrIn,
	  connStrOut, cbConnStrOutMax, pcbConnStrOut, fDriverCompletion));

  MEM_FREE (_ConnStrIn);

  if (hproc == SQL_NULL_HPROC)
    {
      MEM_FREE (_ConnStrOut);
      _iodbcdm_driverunload (pdbc);
      PUSHSQLERR (pdbc->herr, en_IM001);
      return SQL_ERROR;
    }

  if (szConnStrOut
      && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
      && ((penv->unicode_driver && waMode != 'W')
	  || (!penv->unicode_driver && waMode == 'W')))
    {
      if (waMode != 'W')
	{
	  /* ansi<=unicode */
	  dm_StrCopyOut2_W2A ((SQLWCHAR *) connStrOut,
	      (SQLCHAR *) szConnStrOut, cbConnStrOutMax, NULL);
	}
      else
	{
	  /* unicode<=ansi */
	  dm_StrCopyOut2_A2W ((SQLCHAR *) connStrOut,
	      (SQLWCHAR *) szConnStrOut, cbConnStrOutMax, NULL);
	}
    }

  MEM_FREE (_ConnStrOut);

  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      /* don't unload driver here for retrieve
       * error message from driver */
		/********
		_iodbcdm_driverunload( hdbc );
		*********/

      return retcode;
    }

  /* state transition */
  pdbc->state = en_dbc_connected;

  /* do delayed option setting */
  setopterr |= _iodbcdm_dbcdelayset (pdbc, waMode);

  if (setopterr != SQL_SUCCESS)
    {
      return SQL_SUCCESS_WITH_INFO;
    }

end:
  return retcode;
}


SQLRETURN SQL_API
SQLDriverConnect (SQLHDBC hdbc,
    SQLHWND hwnd,
    SQLCHAR * szConnStrIn,
    SQLSMALLINT cbConnStrIn,
    SQLCHAR * szConnStrOut,
    SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT * pcbConnStrOut,
    SQLUSMALLINT fDriverCompletion)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLDriverConnect (TRACE_ENTER,
	hdbc,
	hwnd,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
	fDriverCompletion));

  retcode = SQLDriverConnect_Internal(
      hdbc,
      hwnd,
      szConnStrIn, cbConnStrIn,
      szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
      fDriverCompletion,
      'A');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLDriverConnect (TRACE_LEAVE,
	hdbc,
	hwnd,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
	fDriverCompletion));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLDriverConnectA (SQLHDBC hdbc,
    SQLHWND hwnd,
    SQLCHAR * szConnStrIn,
    SQLSMALLINT cbConnStrIn,
    SQLCHAR * szConnStrOut,
    SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT * pcbConnStrOut,
    SQLUSMALLINT fDriverCompletion)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLDriverConnect (TRACE_ENTER,
	hdbc,
	hwnd,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
	fDriverCompletion));

  retcode = SQLDriverConnect_Internal(
      hdbc,
      hwnd,
      szConnStrIn, cbConnStrIn,
      szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
      fDriverCompletion,
      'A');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLDriverConnect (TRACE_LEAVE,
	hdbc,
	hwnd,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
	fDriverCompletion));
}


SQLRETURN SQL_API
SQLDriverConnectW (SQLHDBC hdbc,
    SQLHWND hwnd,
    SQLWCHAR * szConnStrIn,
    SQLSMALLINT cbConnStrIn,
    SQLWCHAR * szConnStrOut,
    SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT * pcbConnStrOut,
    SQLUSMALLINT fDriverCompletion)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLDriverConnectW (TRACE_ENTER,
	hdbc,
	hwnd,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
	fDriverCompletion));

  retcode = SQLDriverConnect_Internal(
      hdbc,
      hwnd,
      szConnStrIn, cbConnStrIn,
      szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
      fDriverCompletion,
      'W');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLDriverConnectW (TRACE_LEAVE,
	hdbc,
	hwnd,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
	fDriverCompletion));
}
#endif


SQLRETURN SQL_API
SQLBrowseConnect_Internal (SQLHDBC hdbc,
    SQLPOINTER szConnStrIn,
    SQLSMALLINT cbConnStrIn,
    SQLPOINTER szConnStrOut,
    SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut,
    SQLCHAR waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, NULL);
  void *drv, *dsn;
  char drvbuf[4096];
  char dsnbuf[SQL_MAX_DSN_LENGTH * UTF8_MAX_CHAR_LEN + 1];
  char buf[1024];
  SWORD thread_safe;
  SWORD unload_safe;
  HPROC hproc = SQL_NULL_HPROC;
  void * _ConnStrIn = NULL;
  void * _ConnStrOut = NULL;
  void * connStrOut = szConnStrOut;
  void * connStrIn = szConnStrIn;

  SQLRETURN retcode = SQL_SUCCESS;
  SQLRETURN setopterr = SQL_SUCCESS;

  /* check arguments */
  if ((cbConnStrIn < 0 && cbConnStrIn != SQL_NTS) || cbConnStrOutMax < 0)
    {
      PUSHSQLERR (pdbc->herr, en_S1090);
      return SQL_ERROR;
    }

  if (pdbc->state == en_dbc_allocated)
    {
        drv = _iodbcdm_getkeyvalinstr ((char *) szConnStrIn, cbConnStrIn,
	  "DRIVER", (char*)drvbuf, sizeof (drvbuf));

        dsn = _iodbcdm_getkeyvalinstr ((char *) szConnStrIn, cbConnStrIn,
	  "DSN", (char*)dsnbuf, sizeof (dsnbuf));


        if (dsn == NULL || ((char*)dsn)[0] == '\0')
          dsn = (void *) "default";
        else
          /* if you want tracing, you must use a DSN */
          {
	    if (_iodbcdm_con_settracing (pdbc, (SQLCHAR *) dsn, SQL_NTS, waMode) == SQL_ERROR)
	      {
	        return SQL_ERROR;
	      }
	  }

        /*
         *  Check whether driver is thread safe
         */
        thread_safe = 1;		/* Assume driver is thread safe */

        SQLSetConfigMode (ODBC_BOTH_DSN);
        if ( SQLGetPrivateProfileString ((char *) dsn, "ThreadManager", "", 
		buf, sizeof(buf), "odbc.ini") &&
            (STRCASEEQ (buf, "on") || STRCASEEQ (buf, "1")))
          {
            thread_safe = 0;	/* Driver needs a thread manager */
          }

        /*
         *  Check whether driver is unload safe
         */
        unload_safe = 0;		/* Assume driver is not unload safe */

        SQLSetConfigMode (ODBC_BOTH_DSN);
        if ( SQLGetPrivateProfileString ((char *) dsn, "ThreadManager", "", 
		buf, sizeof(buf), "odbc.ini") &&
            (STRCASEEQ (buf, "on") || STRCASEEQ (buf, "1")))
          {
            unload_safe = 1;
          }

        /*
         *  Get the name of the driver module and load it
         */
        if (drv == NULL || *(char*)drv == '\0')
          {
            SQLSetConfigMode (ODBC_BOTH_DSN);
            if ( SQLGetPrivateProfileString ((char *) dsn, "Driver", "", 
		buf, sizeof(buf), "odbc.ini") != 0)
              {
                drv = buf;
              }
          }

      if (drv == NULL)
	{
	  PUSHSQLERR (pdbc->herr, en_IM002);
	  return SQL_ERROR;
	}

      retcode = _iodbcdm_driverload ((char *) drv, pdbc, thread_safe, unload_safe, waMode);

      switch (retcode)
	{
	case SQL_SUCCESS:
	  break;

	case SQL_SUCCESS_WITH_INFO:
#if 0
	  /* 
	   *  Unsuccessful in calling driver's SQLSetConnectOption() to set 
	   *  login timeout.
	   */
	  setopterr = SQL_ERROR;
#endif
	  break;

	default:
          return retcode;
	}
    }
  else if (pdbc->state != en_dbc_needdata)
    {
      PUSHSQLERR (pdbc->herr, en_08002);
      return SQL_ERROR;
    }

  penv = (ENV_t *) pdbc->henv;

  if ((penv->unicode_driver && waMode != 'W')
      || (!penv->unicode_driver && waMode == 'W'))
    {
      if (waMode != 'W')
        {
        /* ansi=>unicode*/
          if ((_ConnStrOut = malloc(cbConnStrOutMax * sizeof(SQLWCHAR) + 1)) == NULL)
	    {
              PUSHSQLERR (pdbc->herr, en_HY001);
	      return SQL_ERROR;
            }
          _ConnStrIn = dm_SQL_A2W((SQLCHAR *)szConnStrIn, SQL_NTS);
        }
      else
        {
        /* unicode=>ansi*/
          if ((_ConnStrOut = malloc(cbConnStrOutMax + 1)) == NULL)
	    {
              PUSHSQLERR (pdbc->herr, en_HY001);
	      return SQL_ERROR;
            }
          _ConnStrIn = dm_SQL_W2A((SQLWCHAR *)szConnStrIn, SQL_NTS);
        }
      connStrIn = _ConnStrIn;
      cbConnStrIn = SQL_NTS;
      connStrOut = _ConnStrOut;
    }

  CALL_UDRIVER(hdbc, pdbc, retcode, hproc, penv->unicode_driver,
    en_BrowseConnect, (
       pdbc->dhdbc,
       connStrIn,
       cbConnStrIn,
       connStrOut,
       cbConnStrOutMax,
       pcbConnStrOut));

  MEM_FREE(_ConnStrIn);

  if (hproc == SQL_NULL_HPROC)
    {
      MEM_FREE(_ConnStrOut);
      _iodbcdm_driverunload (pdbc);
      pdbc->state = en_dbc_allocated;
      PUSHSQLERR (pdbc->herr, en_IM001);
      return SQL_ERROR;
    }

  if (szConnStrOut
      && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
      &&  ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W')))
    {
      if (waMode != 'W')
        {
        /* ansi<=unicode*/
          dm_StrCopyOut2_W2A ((SQLWCHAR *) connStrOut, (SQLCHAR *) szConnStrOut, cbConnStrOutMax, NULL);
        }
      else
        {
        /* unicode<=ansi*/
          dm_StrCopyOut2_A2W ((SQLCHAR *) connStrOut, (SQLWCHAR *) szConnStrOut, cbConnStrOutMax, NULL);
        }
    }

  MEM_FREE(_ConnStrOut);

  switch (retcode)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
      pdbc->state = en_dbc_connected;
      setopterr |= _iodbcdm_dbcdelayset (pdbc, waMode);
      if (setopterr != SQL_SUCCESS)
	{
	  retcode = SQL_SUCCESS_WITH_INFO;
	}
      break;

    case SQL_NEED_DATA:
      pdbc->state = en_dbc_needdata;
      break;

    case SQL_ERROR:
      pdbc->state = en_dbc_allocated;
      /* but the driver will not unloaded
       * to allow application retrieve err
       * message from driver
       */
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLBrowseConnect (SQLHDBC hdbc,
    SQLCHAR * szConnStrIn,
    SQLSMALLINT cbConnStrIn,
    SQLCHAR * szConnStrOut,
    SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT * pcbConnStrOut)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLBrowseConnect (TRACE_ENTER,
      	hdbc,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut));

  retcode = SQLBrowseConnect_Internal (
  	hdbc,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
	'A');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLBrowseConnect (TRACE_LEAVE,
      	hdbc,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLBrowseConnectA (SQLHDBC hdbc,
    SQLCHAR * szConnStrIn,
    SQLSMALLINT cbConnStrIn,
    SQLCHAR * szConnStrOut,
    SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT * pcbConnStrOut)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLBrowseConnect (TRACE_ENTER,
      	hdbc,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut));

  retcode = SQLBrowseConnect_Internal (
  	hdbc,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
	'A');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLBrowseConnect (TRACE_LEAVE,
      	hdbc,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut));
}


SQLRETURN SQL_API
SQLBrowseConnectW (SQLHDBC hdbc,
    SQLWCHAR * szConnStrIn,
    SQLSMALLINT cbConnStrIn,
    SQLWCHAR * szConnStrOut,
    SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT * pcbConnStrOut)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLBrowseConnectW (TRACE_ENTER,
      	hdbc,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut));

  retcode = SQLBrowseConnect_Internal (
  	hdbc,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
	'W');

  LEAVE_HDBC (hdbc, 1,
    trace_SQLBrowseConnectW (TRACE_LEAVE,
      	hdbc,
	szConnStrIn, cbConnStrIn,
	szConnStrOut, cbConnStrOutMax, pcbConnStrOut));
}
#endif


static SQLRETURN
SQLDisconnect_Internal (SQLHDBC hdbc)
{
  CONN (pdbc, hdbc);
  STMT (pstmt, NULL);
  SQLRETURN retcode;
  HPROC hproc = SQL_NULL_HPROC;

  sqlstcode_t sqlstat = en_00000;

  /* check hdbc state */
  if (pdbc->state == en_dbc_allocated)
    {
      sqlstat = en_08003;
    }

  /* check stmt(s) state */
  for (pstmt = (STMT_t *) pdbc->hstmt;
      pstmt != NULL && sqlstat == en_00000;
      pstmt = (STMT_t *) pstmt->next)
    {
      if (pstmt->state >= en_stmt_needdata
	  || pstmt->asyn_on != en_NullProc)
	/* In this case one need to call
	 * SQLCancel() first */
	{
	  sqlstat = en_S1010;
	}
    }

  if (sqlstat == en_00000)
    {
      hproc = _iodbcdm_getproc (pdbc, en_Disconnect);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);
      return SQL_ERROR;
    }

  CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_Disconnect, (
	  pdbc->dhdbc));

  if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
    {
      /* diff from MS specs. We disallow
       * driver SQLDisconnect() return
       * SQL_SUCCESS_WITH_INFO and post
       * error message.
       */
      retcode = SQL_SUCCESS;
    }
  else
    {
      return retcode;
    }

  /* free all statement handle(s) on this connection */
  for (; pdbc->hstmt;)
    {
      _iodbcdm_dropstmt (pdbc->hstmt);
    }

  /* state transition */
  if (retcode == SQL_SUCCESS)
    {
      pdbc->state = en_dbc_allocated;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLDisconnect (SQLHDBC hdbc)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLDisconnect (TRACE_ENTER, hdbc));

  retcode = SQLDisconnect_Internal (hdbc);

  LEAVE_HDBC (hdbc, 1,
    trace_SQLDisconnect (TRACE_LEAVE, hdbc));
}


SQLRETURN SQL_API
SQLNativeSql_Internal (SQLHDBC hdbc,
    SQLPOINTER szSqlStrIn,
    SQLINTEGER cbSqlStrIn,
    SQLPOINTER szSqlStr,
    SQLINTEGER cbSqlStrMax,
    SQLINTEGER * pcbSqlStr,
    SQLCHAR waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, pdbc->henv);
  sqlstcode_t sqlstat = en_00000;
  SQLRETURN retcode = SQL_SUCCESS;
  HPROC hproc = SQL_NULL_HPROC;
  void * _SqlStrIn = NULL;
  void * _SqlStr = NULL;
  void * sqlStr = szSqlStr;

  /* check argument */
  if (szSqlStrIn == NULL)
    {
      sqlstat = en_S1009;
    }
  else if (cbSqlStrIn < 0 && cbSqlStrIn != SQL_NTS)
    {
      sqlstat = en_S1090;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);
      return SQL_ERROR;
    }

  /* check state */
  if (pdbc->state <= en_dbc_needdata)
    {
      PUSHSQLERR (pdbc->herr, en_08003);
      return SQL_ERROR;
    }

  if ((penv->unicode_driver && waMode != 'W')
      || (!penv->unicode_driver && waMode == 'W'))
    {
      if (waMode != 'W')
        {
        /* ansi=>unicode*/
          if ((_SqlStr = malloc(cbSqlStrMax * sizeof(SQLWCHAR) + 1)) == NULL)
	    {
              PUSHSQLERR (pdbc->herr, en_HY001);

	      return SQL_ERROR;
            }
          _SqlStrIn = dm_SQL_A2W((SQLCHAR *)szSqlStrIn, SQL_NTS);
        }
      else
        {
        /* unicode=>ansi*/
          if ((_SqlStr = malloc(cbSqlStrMax + 1)) == NULL)
	    {
              PUSHSQLERR (pdbc->herr, en_HY001);

	      return SQL_ERROR;
            }
          _SqlStrIn = dm_SQL_W2A((SQLWCHAR *)szSqlStrIn, SQL_NTS);
        }
      szSqlStrIn = _SqlStrIn;
      cbSqlStrIn = SQL_NTS;
      sqlStr = _SqlStr;
    }

  /* call driver */
  CALL_UDRIVER(hdbc, pdbc, retcode, hproc, penv->unicode_driver,
    en_NativeSql, (
       pdbc->dhdbc,
       szSqlStrIn,
       cbSqlStrIn,
       sqlStr,
       cbSqlStrMax,
       pcbSqlStr));

  MEM_FREE(_SqlStrIn);

  if (hproc == SQL_NULL_HPROC)
    {
      MEM_FREE(_SqlStr);
      PUSHSQLERR (pdbc->herr, en_IM001);

      return SQL_ERROR;
    }

  if (szSqlStr
      && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
      &&  ((penv->unicode_driver && waMode != 'W')
          || (!penv->unicode_driver && waMode == 'W')))
    {
      if (waMode != 'W')
        {
        /* ansi<=unicode*/
          dm_StrCopyOut2_W2A ((SQLWCHAR *) sqlStr, (SQLCHAR *) szSqlStr, cbSqlStrMax, NULL);
        }
      else
        {
        /* unicode<=ansi*/
          dm_StrCopyOut2_A2W ((SQLCHAR *) sqlStr, (SQLWCHAR *) szSqlStr, cbSqlStrMax, NULL);
        }
    }

  MEM_FREE(_SqlStr);

  return retcode;
}


SQLRETURN SQL_API
SQLNativeSql (
    SQLHDBC hdbc,
    SQLCHAR * szSqlStrIn,
    SQLINTEGER cbSqlStrIn,
    SQLCHAR * szSqlStr,
    SQLINTEGER cbSqlStrMax,
    SQLINTEGER * pcbSqlStr)
{
  ENTER_HDBC (hdbc, 0,
    trace_SQLNativeSql (TRACE_ENTER,
    	hdbc,
	szSqlStrIn, cbSqlStrIn,
	szSqlStr, cbSqlStrMax, pcbSqlStr));

  retcode = SQLNativeSql_Internal (
  	hdbc,
	szSqlStrIn, cbSqlStrIn,
	szSqlStr, cbSqlStrMax, pcbSqlStr,
	'A');

  LEAVE_HDBC (hdbc, 0,
    trace_SQLNativeSql (TRACE_LEAVE,
    	hdbc,
	szSqlStrIn, cbSqlStrIn,
	szSqlStr, cbSqlStrMax, pcbSqlStr));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLNativeSqlA (
    SQLHDBC hdbc,
    SQLCHAR * szSqlStrIn,
    SQLINTEGER cbSqlStrIn,
    SQLCHAR * szSqlStr,
    SQLINTEGER cbSqlStrMax,
    SQLINTEGER * pcbSqlStr)
{
  ENTER_HDBC (hdbc, 0,
    trace_SQLNativeSql (TRACE_ENTER,
    	hdbc,
	szSqlStrIn, cbSqlStrIn,
	szSqlStr, cbSqlStrMax, pcbSqlStr));

  retcode = SQLNativeSql_Internal(
  	hdbc,
	szSqlStrIn, cbSqlStrIn,
	szSqlStr, cbSqlStrMax, pcbSqlStr,
	'A');

  LEAVE_HDBC (hdbc, 0,
    trace_SQLNativeSql (TRACE_LEAVE,
    	hdbc,
	szSqlStrIn, cbSqlStrIn,
	szSqlStr, cbSqlStrMax, pcbSqlStr));
}


SQLRETURN SQL_API
SQLNativeSqlW (
    SQLHDBC hdbc,
    SQLWCHAR * szSqlStrIn,
    SQLINTEGER cbSqlStrIn,
    SQLWCHAR * szSqlStr,
    SQLINTEGER cbSqlStrMax,
    SQLINTEGER * pcbSqlStr)
{
  ENTER_HDBC (hdbc, 0,
    trace_SQLNativeSqlW (TRACE_ENTER,
    	hdbc,
	szSqlStrIn, cbSqlStrIn,
	szSqlStr, cbSqlStrMax, pcbSqlStr));

  retcode = SQLNativeSql_Internal(
  	hdbc,
	szSqlStrIn, cbSqlStrIn,
	szSqlStr, cbSqlStrMax, pcbSqlStr,
	'W');

  LEAVE_HDBC (hdbc, 0,
    trace_SQLNativeSqlW (TRACE_LEAVE,
    	hdbc,
	szSqlStrIn, cbSqlStrIn,
	szSqlStr, cbSqlStrMax, pcbSqlStr));
}
#endif
