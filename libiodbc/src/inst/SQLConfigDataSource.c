/*
 *  SQLConfigDataSource.c
 *
 *  $Id: SQLConfigDataSource.c,v 1.13 2006/01/20 15:58:35 source Exp $
 *
 *  Add, modify or delete datasources
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


#include <iodbc.h>
#include <odbcinst.h>
#include <unicode.h>

#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
#  include <Carbon/Carbon.h>
#endif

#include "dlf.h"
#include "inifile.h"
#include "misc.h"
#include "iodbc_error.h"

#ifndef WIN32
#include <unistd.h>
#define CALL_CONFIG_DSN(path) \
	if ((handle = DLL_OPEN(path)) != NULL) \
	{ \
		if ((pConfigDSN = (pConfigDSNFunc)DLL_PROC(handle, "ConfigDSN")) != NULL) \
		{ \
	  	  if (pConfigDSN(hwndParent, fRequest, lpszDriver, lpszAttributes)) \
	  	  { \
	    	  DLL_CLOSE(handle); \
	    	  retcode = TRUE; \
	    	  goto done; \
	  	  } \
		  else \
		  { \
		    PUSH_ERROR(ODBC_ERROR_REQUEST_FAILED); \
	    	 DLL_CLOSE(handle); \
	    	 retcode = FALSE; \
	    	 goto done; \
		  } \
		} \
	  DLL_CLOSE(handle); \
	}

#define CALL_CONFIG_DSNW(path) \
	if ((handle = DLL_OPEN(path)) != NULL) \
	{ \
		if ((pConfigDSNW = (pConfigDSNWFunc)DLL_PROC(handle, "ConfigDSNW")) != NULL) \
		{ \
	  	  if (pConfigDSNW(hwndParent, fRequest, (SQLWCHAR*)lpszDriver, (SQLWCHAR*)lpszAttributes)) \
	  	  { \
	    	  DLL_CLOSE(handle); \
	    	  retcode = TRUE; \
	    	  goto done; \
	  	  } \
		  else \
		  { \
		    PUSH_ERROR(ODBC_ERROR_REQUEST_FAILED); \
	    	 DLL_CLOSE(handle); \
	    	 retcode = FALSE; \
	    	 goto done; \
		  } \
		} \
                else if ((pConfigDSN = (pConfigDSNFunc)DLL_PROC(handle, "ConfigDSN")) != NULL) \
                { \
                  char *_attrs_u8, *ptr_u8; \
                  wchar_t *ptr; \
                  int length; \
                  for(length = 0, ptr = lpszAttributes ; *ptr ; length += WCSLEN (ptr) + 1, ptr += WCSLEN (ptr) + 1); \
                  if (length > 0) \
                  { \
                    if ((_attrs_u8 = malloc (length * UTF8_MAX_CHAR_LEN + 1)) != NULL) \
                    { \
                      for(ptr = lpszAttributes, ptr_u8 = _attrs_u8 ; *ptr ; ptr += WCSLEN (ptr) + 1, ptr_u8 += STRLEN (ptr_u8) + 1) \
                        dm_StrCopyOut2_W2A (ptr, ptr_u8, WCSLEN (ptr) *  UTF8_MAX_CHAR_LEN, NULL); \
                      *ptr_u8 = '\0'; \
                    } \
                  } \
                  else _attrs_u8 = (char *) dm_SQL_WtoU8((SQLWCHAR*)lpszAttributes, SQL_NTS); \
                  if (_attrs_u8 == NULL && lpszAttributes) \
                  { \
                    PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM); \
                    DLL_CLOSE(handle); \
                    retcode = FALSE; \
                    goto done; \
                  } \
	  	  if (pConfigDSN(hwndParent, fRequest, _drv_u8, _attrs_u8)) \
	  	  { \
                  MEM_FREE (_attrs_u8); \
	    	  DLL_CLOSE(handle); \
	    	  retcode = TRUE; \
	    	  goto done; \
	  	  } \
		  else \
		  { \
                  MEM_FREE (_attrs_u8); \
		    PUSH_ERROR(ODBC_ERROR_REQUEST_FAILED); \
	    	 DLL_CLOSE(handle); \
	    	 retcode = FALSE; \
	    	 goto done; \
		  } \
                } \
	  DLL_CLOSE(handle); \
	}
#endif

extern BOOL RemoveDSNFromIni (LPCSTR lpszDSN, SQLCHAR waMode);

BOOL
RemoveDefaultDataSource (void)
{
  BOOL retcode = FALSE;
  PCONFIG pCfg = NULL;

  /* removes the default dsn */
  if (!RemoveDSNFromIni ("Default", 'A'))
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto quit;
    }

  /* removes the default driver not regarding the current config mode */
  if (_iodbcdm_cfg_search_init (&pCfg, "odbcinst.ini", TRUE))
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto quit;
    }

  _iodbcdm_cfg_write (pCfg, "Default", NULL, NULL);

  if (!_iodbcdm_cfg_commit (pCfg))
    retcode = TRUE;
  else
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto quit;
    }

  /* check now the system only if it was the user first checked */
  if (wSystemDSN != SYSTEMDSN_ONLY)
    {
      if (pCfg)
	{
	  _iodbcdm_cfg_done (pCfg);
	  pCfg = NULL;
	}
      wSystemDSN = SYSTEMDSN_ONLY;

      if (_iodbcdm_cfg_search_init (&pCfg, "odbcinst.ini", TRUE))
	goto quit;
      _iodbcdm_cfg_write (pCfg, "Default", NULL, NULL);
      _iodbcdm_cfg_commit (pCfg);
    }

quit:
  if (pCfg)
    _iodbcdm_cfg_done (pCfg);
  return retcode;
}


BOOL INSTAPI
SQLConfigDataSource_Internal (HWND hwndParent, WORD fRequest,
    SQLPOINTER lpszDriver, SQLPOINTER lpszAttributes, SQLCHAR waMode)
{
  PCONFIG pCfg = NULL;
  BOOL retcode = FALSE;
  void *handle;
  pConfigDSNFunc pConfigDSN;
  pConfigDSNWFunc pConfigDSNW;
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
  CFStringRef libname = NULL;
  CFBundleRef bundle;
  CFURLRef liburl;
  char name[1024] = { 0 };
#endif
  char *_drv_u8 = NULL;

  /* Check input parameters */
  CLEAR_ERROR ();

  /* Map the request User/System */
  switch (fRequest)
    {
    case ODBC_ADD_DSN:
    case ODBC_CONFIG_DSN:
    case ODBC_REMOVE_DSN:
      configMode = ODBC_USER_DSN;
      break;

    case ODBC_ADD_SYS_DSN:
    case ODBC_CONFIG_SYS_DSN:
    case ODBC_REMOVE_SYS_DSN:
      configMode = ODBC_SYSTEM_DSN;
      fRequest = fRequest - ODBC_ADD_SYS_DSN + ODBC_ADD_DSN;
      break;

    case ODBC_REMOVE_DEFAULT_DSN:
      retcode = RemoveDefaultDataSource ();
      goto resetdsnmode;

    default:
      PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
      goto resetdsnmode;
    }

  if (waMode == 'W')
    {
      _drv_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszDriver, SQL_NTS);
      if (_drv_u8 == NULL && lpszDriver)
	{
	  PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
	  goto resetdsnmode;
	}
    }
  else
    _drv_u8 = (char *) lpszDriver;

  if (!_drv_u8 || !STRLEN (_drv_u8))
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
      goto resetdsnmode;
    }

  /* Get it from the user odbcinst file */
  wSystemDSN = USERDSN_ONLY;
  if (!_iodbcdm_cfg_search_init (&pCfg, "odbcinst.ini", TRUE))
    {
      if (!_iodbcdm_cfg_find (pCfg, (char *) _drv_u8, "Setup"))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (pCfg->value);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (pCfg->value);
	    }
	}
      if (!_iodbcdm_cfg_find (pCfg, (char *) _drv_u8, "Driver"))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (pCfg->value);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (pCfg->value);
	    }
	}
      if (!access (_drv_u8, X_OK))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (_drv_u8);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (_drv_u8);
	    }
	}
      if (!_iodbcdm_cfg_find (pCfg, "Default", "Setup"))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (pCfg->value);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (pCfg->value);
	    }
	}
      if (!_iodbcdm_cfg_find (pCfg, "Default", "Driver"))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (pCfg->value);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (pCfg->value);
	    }
	}
    }

  /* Get it from the system odbcinst file */
  if (pCfg)
    {
      _iodbcdm_cfg_done (pCfg);
      pCfg = NULL;
    }
  wSystemDSN = SYSTEMDSN_ONLY;
  if (!_iodbcdm_cfg_search_init (&pCfg, "odbcinst.ini", TRUE))
    {
      if (!_iodbcdm_cfg_find (pCfg, (char *) _drv_u8, "Setup"))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (pCfg->value);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (pCfg->value);
	    }
	}
      if (!_iodbcdm_cfg_find (pCfg, (char *) _drv_u8, "Driver"))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (pCfg->value);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (pCfg->value);
	    }
	}
      if (!access (_drv_u8, X_OK))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (_drv_u8);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (_drv_u8);
	    }
	}
      if (!_iodbcdm_cfg_find (pCfg, "Default", "Setup"))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (pCfg->value);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (pCfg->value);
	    }
	}
      if (!_iodbcdm_cfg_find (pCfg, "Default", "Driver"))
	{
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (pCfg->value);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (pCfg->value);
	    }
	}
    }

  /* The last ressort, a proxy driver */
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
  bundle = CFBundleGetBundleWithIdentifier (CFSTR ("org.iodbc.inst"));
  if (bundle)
    {
      /* Search for the drvproxy library */
      liburl =
	  CFBundleCopyResourceURL (bundle, CFSTR ("iODBCdrvproxy.bundle"),
	  NULL, NULL);
      if (liburl
	  && (libname =
	      CFURLCopyFileSystemPath (liburl, kCFURLPOSIXPathStyle)))
	{
	  CFStringGetCString (libname, name, sizeof (name),
	      kCFStringEncodingASCII);
	  strcat (name, "/Contents/MacOS/iODBCdrvproxy");
	  if (waMode == 'A')
	    {
	      CALL_CONFIG_DSN (name);
	    }
	  else
	    {
	      CALL_CONFIG_DSNW (name);
	    }
	}
      if (liburl)
	CFRelease (liburl);
      if (libname)
	CFRelease (libname);
    }
#else
  if (waMode == 'A')
    {
      CALL_CONFIG_DSN ("libdrvproxy.so");
    }
  else
    {
      CALL_CONFIG_DSNW ("libdrvproxy.so");
    }
#endif

  /* Error : ConfigDSN could no be found */
  PUSH_ERROR (ODBC_ERROR_LOAD_LIB_FAILED);

done:
  if (pCfg)
    _iodbcdm_cfg_done (pCfg);

resetdsnmode:
  if (_drv_u8 != lpszDriver)
    MEM_FREE (_drv_u8);

  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;

  return retcode;
}

BOOL INSTAPI
SQLConfigDataSource (HWND hwndParent, WORD fRequest, LPCSTR lpszDriver,
    LPCSTR lpszAttributes)
{
  return SQLConfigDataSource_Internal (hwndParent, fRequest,
      (SQLPOINTER) lpszDriver, (SQLPOINTER) lpszAttributes, 'A');
}

BOOL INSTAPI
SQLConfigDataSourceW (HWND hwndParent, WORD fRequest, LPCWSTR lpszDriver,
    LPCWSTR lpszAttributes)
{
  return SQLConfigDataSource_Internal (hwndParent, fRequest,
      (SQLPOINTER) lpszDriver, (SQLPOINTER) lpszAttributes, 'W');
}
