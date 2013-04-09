/*
 *  SQLGetTranslator.c
 *
 *  $Id: SQLGetTranslator.c,v 1.14 2006/01/20 15:58:35 source Exp $
 *
 *  These functions intentionally left blank
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

#include "dlf.h"
#include "inifile.h"
#include "misc.h"
#include "iodbc_error.h"

#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
#include <Carbon/Carbon.h>
#endif

#ifndef WIN32
#include <unistd.h>
#define CALL_CONFIG_TRANSLATOR(path) \
	if ((handle = DLL_OPEN(path)) != NULL) \
	{ \
		if ((pConfigTranslator = (pConfigTranslatorFunc)DLL_PROC(handle, "ConfigTranslator")) != NULL) \
		{ \
	  	if (pConfigTranslator(hwndParent, pvOption)) \
	  	{ \
	    	DLL_CLOSE(handle); \
	    	finish = retcode = TRUE; \
	    	goto done; \
	  	} \
			else \
			{ \
				PUSH_ERROR(ODBC_ERROR_GENERAL_ERR); \
	    	DLL_CLOSE(handle); \
	    	retcode = FALSE; \
	    	goto done; \
			} \
		} \
		DLL_CLOSE(handle); \
	}

#define CALL_TRSCHOOSE_DIALBOX(path) \
	if ((handle = DLL_OPEN(path)) != NULL) \
	{ \
		if ((pTrsChoose = (pTrsChooseFunc)DLL_PROC(handle, "_iodbcdm_trschoose_dialbox")) != NULL) \
		  ret = pTrsChoose(hwndParent, translator, sizeof(translator), NULL); \
		else ret = SQL_NO_DATA; \
		DLL_CLOSE(handle); \
	} \
	else ret = SQL_NO_DATA;
#endif

extern SQLRETURN _iodbcdm_trschoose_dialbox (HWND, LPSTR, DWORD, int *);

BOOL INSTAPI
GetTranslator (HWND hwndParent, LPSTR lpszName, WORD cbNameMax,
    WORD * pcbNameOut, LPSTR lpszPath, WORD cbPathMax,
    WORD * pcbPathOut, DWORD * pvOption)
{
  pConfigTranslatorFunc pConfigTranslator;
  pTrsChooseFunc pTrsChoose;
  BOOL retcode = FALSE, finish = FALSE;
  PCONFIG pCfg;
  UWORD configMode;
  RETCODE ret = SQL_NO_DATA;
  void *handle;
  char translator[1024];
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
  CFStringRef libname = NULL;
  CFBundleRef bundle;
  CFURLRef liburl;
  char name[1024] = { 0 };
#endif

  do
    {
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
	      CALL_TRSCHOOSE_DIALBOX (name);
	    }
	  if (liburl)
	    CFRelease (liburl);
	  if (libname)
	    CFRelease (libname);
	}
#else
      CALL_TRSCHOOSE_DIALBOX ("libiodbcadm.so");
#endif

      if (ret == SQL_NO_DATA)
	{
	  if (pcbNameOut)
	    *pcbNameOut = 0;
	  if (pcbPathOut)
	    *pcbPathOut = 0;
	  finish = TRUE;
	}

      if (ret == SQL_SUCCESS)
	{
	  STRNCPY (lpszName, translator + STRLEN ("TranslationName="),
	      cbNameMax - 1);
	  if (pcbNameOut)
	    *pcbNameOut = STRLEN (lpszName);

	  /* Get it from the user odbcinst file */
	  wSystemDSN = USERDSN_ONLY;
	  if (!_iodbcdm_cfg_search_init (&pCfg, "odbcinst.ini", TRUE))
	    {
	      if (!_iodbcdm_cfg_find (pCfg, (char *) lpszName, "Setup"))
		CALL_CONFIG_TRANSLATOR (pCfg->value);
	      if (!_iodbcdm_cfg_find (pCfg, (char *) lpszName, "Translator"))
		CALL_CONFIG_TRANSLATOR (pCfg->value);
	      if (!access (lpszName, X_OK))
		CALL_CONFIG_TRANSLATOR (lpszName);
	      if (!_iodbcdm_cfg_find (pCfg, "Default", "Setup"))
		CALL_CONFIG_TRANSLATOR (pCfg->value);
	      if (!_iodbcdm_cfg_find (pCfg, "Default", "Translator"))
		CALL_CONFIG_TRANSLATOR (pCfg->value);
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
	      if (!_iodbcdm_cfg_find (pCfg, (char *) lpszName, "Setup"))
		CALL_CONFIG_TRANSLATOR (pCfg->value);
	      if (!_iodbcdm_cfg_find (pCfg, (char *) lpszName, "Translator"))
		CALL_CONFIG_TRANSLATOR (pCfg->value);
	      if (!access (lpszName, X_OK))
		CALL_CONFIG_TRANSLATOR (lpszName);
	      if (!_iodbcdm_cfg_find (pCfg, "Default", "Setup"))
		CALL_CONFIG_TRANSLATOR (pCfg->value);
	      if (!_iodbcdm_cfg_find (pCfg, "Default", "Translator"))
		CALL_CONFIG_TRANSLATOR (pCfg->value);
	    }

	  /* The last ressort, a proxy driver */
	  CALL_CONFIG_TRANSLATOR ("libtranslator.so");

	  /* Error : ConfigDSN could no be found */
	  PUSH_ERROR (ODBC_ERROR_LOAD_LIB_FAILED);

	done:
	  STRNCPY (lpszPath, pCfg->fileName, cbPathMax - 1);
	  if (pcbPathOut)
	    *pcbPathOut = STRLEN (lpszPath);
	  _iodbcdm_cfg_done (pCfg);
	}
    }
  while (!finish);

  retcode = TRUE;

  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;

  return retcode;
}


BOOL INSTAPI
SQLGetTranslator (HWND hwnd,
    LPSTR lpszName,
    WORD cbNameMax,
    WORD * pcbNameOut,
    LPSTR lpszPath, WORD cbPathMax, WORD * pcbPathOut, DWORD * pvOption)
{
  BOOL retcode = FALSE;

  /* Check input parameters */
  CLEAR_ERROR ();
  if (!hwnd)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_HWND);
      goto quit;
    }

  if (!lpszName || !lpszPath || cbNameMax < 1 || cbPathMax < 1)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
      goto quit;
    }

  retcode = GetTranslator (hwnd, lpszName, cbNameMax, pcbNameOut, lpszPath,
      cbPathMax, pcbPathOut, pvOption);

quit:
  return retcode;
}

BOOL INSTAPI
SQLGetTranslatorW (HWND hwnd,
    LPWSTR lpszName,
    WORD cbNameMax,
    WORD FAR * pcbNameOut,
    LPWSTR lpszPath,
    WORD cbPathMax, WORD FAR * pcbPathOut, DWORD FAR * pvOption)
{
  char *_name_u8 = NULL;
  char *_path_u8 = NULL;
  BOOL retcode = FALSE;

  if (cbNameMax > 0)
    {
      if ((_name_u8 = malloc (cbNameMax * UTF8_MAX_CHAR_LEN + 1)) == NULL)
	{
	  PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
	  goto done;
	}
    }

  if (cbPathMax > 0)
    {
      if ((_path_u8 = malloc (cbPathMax * UTF8_MAX_CHAR_LEN + 1)) == NULL)
	{
	  PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
	  goto done;
	}
    }

  retcode =
      SQLGetTranslator (hwnd, _name_u8, cbNameMax * UTF8_MAX_CHAR_LEN,
      pcbNameOut, _path_u8, cbPathMax * UTF8_MAX_CHAR_LEN, pcbPathOut,
      pvOption);

  if (retcode == TRUE)
    {
      dm_StrCopyOut2_U8toW (_name_u8, lpszName, cbNameMax, pcbNameOut);
      dm_StrCopyOut2_U8toW (_path_u8, lpszPath, cbPathMax, pcbPathOut);
    }

done:
  MEM_FREE (_name_u8);
  MEM_FREE (_path_u8);

  return retcode;
}
