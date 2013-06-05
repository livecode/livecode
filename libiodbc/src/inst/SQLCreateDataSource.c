/*
 *  SQLCreateDataSource.c
 *
 *  $Id: SQLCreateDataSource.c,v 1.14 2006/01/24 00:08:54 source Exp $
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
#include <iodbcadm.h>
#include <unicode.h>

#include "iodbc_error.h"
#include "dlf.h"

#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
#include <Carbon/Carbon.h>
#endif

extern BOOL ValidDSN (LPCSTR lpszDSN);
extern BOOL ValidDSNW (LPCWSTR lpszDSN);

#define CALL_DRVCONN_DIALBOX(path) \
	if ((handle = DLL_OPEN(path)) != NULL) \
	{ \
		if ((pDrvConn = (pDrvConnFunc)DLL_PROC(handle, "iodbcdm_drvconn_dialbox")) != NULL) \
		  pDrvConn(parent, dsn, sizeof(dsn), NULL, SQL_DRIVER_PROMPT, &config); \
      retcode = TRUE; \
		DLL_CLOSE(handle); \
	}

#define CALL_DRVCONN_DIALBOXW(path) \
	if ((handle = DLL_OPEN(path)) != NULL) \
	{ \
		if ((pDrvConnW = (pDrvConnWFunc)DLL_PROC(handle, "iodbcdm_drvconn_dialboxw")) != NULL) \
		  pDrvConnW(parent, dsn, sizeof(dsn) / sizeof(wchar_t), NULL, SQL_DRIVER_PROMPT, &config); \
      retcode = TRUE; \
		DLL_CLOSE(handle); \
	}

BOOL
CreateDataSource (HWND parent, LPCSTR lpszDSN, SQLCHAR waMode)
{
  char dsn[1024] = { 0 };
  UWORD config = ODBC_USER_DSN;
  BOOL retcode = FALSE;
  void *handle;
  pDrvConnFunc pDrvConn = NULL;
  pDrvConnWFunc pDrvConnW = NULL;
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
  CFStringRef libname = NULL;
  CFBundleRef bundle;
  CFURLRef liburl;
  char name[1024] = { 0 };
#endif

  /* Load the Admin dialbox function */
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
  bundle = CFBundleGetBundleWithIdentifier (CFSTR ("org.iodbc.inst"));
  if (bundle)
    {
      /* Search for the iODBCadm library */
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
	  if (waMode == 'A')
	    {
	      CALL_DRVCONN_DIALBOX (name);
	    }
	  else
	    {
	      CALL_DRVCONN_DIALBOXW (name);
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
      CALL_DRVCONN_DIALBOX ("libiodbcadm.so");
    }
  else
    {
      CALL_DRVCONN_DIALBOXW ("libiodbcadm.so");
    }
#endif

  return retcode;
}


BOOL INSTAPI
SQLCreateDataSource_Internal (HWND hwndParent, SQLPOINTER lpszDSN,
    SQLCHAR waMode)
{
  BOOL retcode = FALSE;

  /* Check input parameters */
  CLEAR_ERROR ();
  if (!hwndParent)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_HWND);
      goto quit;
    }

  if (waMode == 'A')
    {
      if ((!lpszDSN && !ValidDSN (lpszDSN)) || (!lpszDSN
	      && !STRLEN (lpszDSN)))
	{
	  PUSH_ERROR (ODBC_ERROR_INVALID_DSN);
	  goto quit;
	}
    }
  else
    {
      if ((!lpszDSN && !ValidDSNW (lpszDSN)) || (!lpszDSN
	      && !WCSLEN (lpszDSN)))
	{
	  PUSH_ERROR (ODBC_ERROR_INVALID_DSN);
	  goto quit;
	}
    }

  retcode = CreateDataSource (hwndParent, lpszDSN, waMode);

quit:
  return retcode;
}

BOOL INSTAPI
SQLCreateDataSource (HWND hwndParent, LPCSTR lpszDSN)
{
  return SQLCreateDataSource_Internal (hwndParent, (SQLPOINTER) lpszDSN, 'A');
}

BOOL INSTAPI
SQLCreateDataSourceW (HWND hwndParent, LPCWSTR lpszDSN)
{
  return SQLCreateDataSource_Internal (hwndParent, (SQLPOINTER) lpszDSN, 'W');
}
