/*
 *  SQLWriteDSNToIni.c
 *
 *  $Id: SQLWriteDSNToIni.c,v 1.8 2006/01/20 15:58:35 source Exp $
 *
 *  Write a DSN connect string to a file
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

#include "inifile.h"
#include "misc.h"
#include "iodbc_error.h"

extern BOOL ValidDSN (LPCSTR lpszDSN);
extern BOOL ValidDSNW (LPCWSTR lpszDSN);

extern int GetPrivateProfileString (LPCSTR lpszSection, LPCSTR lpszEntry,
    LPCSTR lpszDefault, LPSTR lpszRetBuffer, int cbRetBuffer,
    LPCSTR lpszFilename);

BOOL
WriteDSNToIni (LPCSTR lpszDSN, LPCSTR lpszDriver)
{
  char szBuffer[4096];
  BOOL retcode = FALSE;
  PCONFIG pCfg = NULL;

  if (_iodbcdm_cfg_search_init (&pCfg, "odbc.ini", TRUE))
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto done;
    }

  if (strcmp (lpszDSN, "Default"))
    {
      /* adds a DSN=Driver to the [ODBC data sources] section */
#ifdef WIN32
      if (_iodbcdm_cfg_write (pCfg, "ODBC 32 bit Data Sources",
	      (LPSTR) lpszDSN, (LPSTR) lpszDriver))
#else
      if (_iodbcdm_cfg_write (pCfg, "ODBC Data Sources", (LPSTR) lpszDSN,
	      (LPSTR) lpszDriver))
#endif
	{
	  PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
	  goto done;
	}
    }

  /* deletes the DSN section in odbc.ini */
  if (_iodbcdm_cfg_write (pCfg, (LPSTR) lpszDSN, NULL, NULL))
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto done;
    }

  /* gets the file of the driver if lpszDriver is a valid description */
  wSystemDSN = USERDSN_ONLY;
  if (!GetPrivateProfileString ((LPSTR) lpszDriver, "Driver", "", szBuffer,
	  sizeof (szBuffer) - 1, "odbcinst.ini"))
    {
      wSystemDSN = SYSTEMDSN_ONLY;

      if (!GetPrivateProfileString ((LPSTR) lpszDriver, "Driver", "",
	      szBuffer, sizeof (szBuffer) - 1, "odbcinst.ini"))
	{
	  PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
	  goto done;
	}
    }

  /* adds a [DSN] section with Driver key */
  if (_iodbcdm_cfg_write (pCfg, (LPSTR) lpszDSN, "Driver", szBuffer)
      || _iodbcdm_cfg_commit (pCfg))
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto done;
    }

  retcode = TRUE;

done:
  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;
  if (pCfg)
    _iodbcdm_cfg_done (pCfg);
  return retcode;
}


BOOL INSTAPI
SQLWriteDSNToIni_Internal (SQLPOINTER lpszDSN, SQLPOINTER lpszDriver,
    SQLCHAR waMode)
{
  char *_driver_u8 = NULL;
  char *_dsn_u8 = NULL;
  BOOL retcode = FALSE;

  /* Check input parameters */
  CLEAR_ERROR ();

  if (waMode == 'A')
    {
      if (!lpszDSN || !ValidDSN (lpszDSN) || !STRLEN (lpszDSN))
	{
	  PUSH_ERROR (ODBC_ERROR_INVALID_DSN);
	  goto quit;
	}
    }
  else
    {
      if (!lpszDSN || !ValidDSNW (lpszDSN) || !WCSLEN (lpszDSN))
	{
	  PUSH_ERROR (ODBC_ERROR_INVALID_DSN);
	  goto quit;
	}
    }

  if (waMode == 'W')
    {
      _dsn_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszDSN, SQL_NTS);
      if (_dsn_u8 == NULL && lpszDSN)
	{
	  PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
	  goto quit;
	}
    }
  else
    _dsn_u8 = (SQLCHAR *) lpszDSN;

  if (waMode == 'W')
    {
      _driver_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszDriver, SQL_NTS);
      if (_driver_u8 == NULL && lpszDriver)
	{
	  PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
	  goto quit;
	}
    }
  else
    _driver_u8 = (SQLCHAR *) lpszDriver;

  if (!_driver_u8 || !STRLEN (_driver_u8))
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
      goto quit;
    }

  switch (configMode)
    {
    case ODBC_USER_DSN:
      wSystemDSN = USERDSN_ONLY;
      retcode = WriteDSNToIni (_dsn_u8, _driver_u8);
      goto quit;

    case ODBC_SYSTEM_DSN:
      wSystemDSN = SYSTEMDSN_ONLY;
      retcode = WriteDSNToIni (_dsn_u8, _driver_u8);
      goto quit;

    case ODBC_BOTH_DSN:
      wSystemDSN = USERDSN_ONLY;
      retcode = WriteDSNToIni (_dsn_u8, _driver_u8);
      if (!retcode)
	{
	  CLEAR_ERROR ();
	  wSystemDSN = SYSTEMDSN_ONLY;
	  retcode = WriteDSNToIni (_dsn_u8, _driver_u8);
	}
      goto quit;
    }

  PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
  goto quit;

quit:
  if (_dsn_u8 != lpszDSN)
    MEM_FREE (_dsn_u8);
  if (_driver_u8 != lpszDriver)
    MEM_FREE (_driver_u8);

  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;

  return retcode;
}

BOOL INSTAPI
SQLWriteDSNToIni (LPCSTR lpszDSN, LPCSTR lpszDriver)
{
  return SQLWriteDSNToIni_Internal ((SQLPOINTER) lpszDSN,
      (SQLPOINTER) lpszDriver, 'A');
}

BOOL INSTAPI
SQLWriteDSNToIniW (LPCWSTR lpszDSN, LPCWSTR lpszDriver)
{
  return SQLWriteDSNToIni_Internal ((SQLPOINTER) lpszDSN,
      (SQLPOINTER) lpszDriver, 'W');
}
