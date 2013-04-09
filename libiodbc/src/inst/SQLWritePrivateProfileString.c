/*
 *  SQLWritePrivateProfileString.c
 *
 *  $Id: SQLWritePrivateProfileString.c,v 1.7 2006/01/20 15:58:35 source Exp $
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
#include "iodbc_error.h"
#include "misc.h"

#ifndef WIN32
BOOL
WritePrivateProfileString (LPCSTR lpszSection, LPCSTR lpszEntry,
    LPCSTR lpszString, LPCSTR lpszFilename)
{
  BOOL retcode = FALSE;
  PCONFIG pCfg = NULL;

  /* Check Input parameters */
  if (lpszSection == NULL || *lpszSection == '\0')
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
      goto fail;
    }

  /* If error during reading the file */
  if (_iodbcdm_cfg_search_init (&pCfg, lpszFilename, TRUE))
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto fail;
    }

  /* Check if the section must be deleted */
  if (!lpszEntry)
    {
      _iodbcdm_cfg_write (pCfg, (LPSTR) lpszSection, NULL, NULL);
      goto done;
    }

  /* Check if the entry must be deleted */
  if (!lpszString)
    {
      _iodbcdm_cfg_write (pCfg, (LPSTR) lpszSection, (LPSTR) lpszEntry, NULL);
      goto done;
    }

  /* Else add the entry */
  _iodbcdm_cfg_write (pCfg, (LPSTR) lpszSection, (LPSTR) lpszEntry,
      (LPSTR) lpszString);

done:
  if (!_iodbcdm_cfg_commit (pCfg))
    retcode = TRUE;
  else
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto fail;
    }

fail:
  if (pCfg)
    _iodbcdm_cfg_done (pCfg);
  return retcode;
}
#endif


BOOL INSTAPI
SQLWritePrivateProfileString (LPCSTR lpszSection, LPCSTR lpszEntry,
    LPCSTR lpszString, LPCSTR lpszFilename)
{
  char pathbuf[1024];
  BOOL retcode = FALSE;

  /* Check input parameters */
  CLEAR_ERROR ();

  /* Else go through user/system odbc.ini */
  switch (configMode)
    {
    case ODBC_USER_DSN:
      wSystemDSN = USERDSN_ONLY;
      if (lpszFilename)
	{
	  retcode =
	      WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	      lpszFilename);
	  goto quit;
	}
      if (_iodbcadm_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
	retcode =
	    WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	    pathbuf);
      goto quit;

    case ODBC_SYSTEM_DSN:
      wSystemDSN = SYSTEMDSN_ONLY;
      if (lpszFilename)
	{
	  retcode =
	      WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	      lpszFilename);
	  goto quit;
	}
      if (_iodbcadm_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
	retcode =
	    WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	    pathbuf);
      goto quit;

    case ODBC_BOTH_DSN:
      wSystemDSN = USERDSN_ONLY;
      if (lpszFilename)
	{
	  retcode =
	      WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	      lpszFilename);
	  if (!retcode)
	    {
	      CLEAR_ERROR ();
	      wSystemDSN = SYSTEMDSN_ONLY;
	      retcode =
		  WritePrivateProfileString (lpszSection, lpszEntry,
		  lpszString, lpszFilename);
	    }
	  goto quit;
	}
      if (_iodbcadm_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
	retcode =
	    WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	    pathbuf);
      else
	{
	  CLEAR_ERROR ();
	  wSystemDSN = SYSTEMDSN_ONLY;
	  if (_iodbcadm_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
	    retcode =
		WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
		pathbuf);
	}
      goto quit;
    }

  PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
  goto quit;

quit:
  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;
  return retcode;
}

BOOL INSTAPI
SQLWritePrivateProfileStringW (LPCWSTR lpszSection, LPCWSTR lpszEntry,
    LPCWSTR lpszString, LPCWSTR lpszFilename)
{
  char *_section_u8 = NULL;
  char *_entry_u8 = NULL;
  char *_string_u8 = NULL;
  char *_filename_u8 = NULL;
  BOOL retcode = FALSE;

  _section_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszSection, SQL_NTS);
  if (_section_u8 == NULL && lpszSection)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      goto done;
    }

  _entry_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszEntry, SQL_NTS);
  if (_entry_u8 == NULL && lpszEntry)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      goto done;
    }

  _string_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszString, SQL_NTS);
  if (_string_u8 == NULL && lpszString)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      goto done;
    }

  _filename_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszFilename, SQL_NTS);
  if (_filename_u8 == NULL && lpszFilename)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      goto done;
    }

  retcode =
      SQLWritePrivateProfileString (_section_u8, _entry_u8, _string_u8,
      _filename_u8);

done:
  MEM_FREE (_section_u8);
  MEM_FREE (_entry_u8);
  MEM_FREE (_string_u8);
  MEM_FREE (_filename_u8);

  return retcode;
}

BOOL INSTAPI
SQLSetKeywordValue (LPCSTR lpszSection,
    LPCSTR lpszEntry, LPSTR lpszString, int cbString)
{
  return SQLWritePrivateProfileString (lpszSection,
      lpszEntry, lpszString, "odbc.ini");
}

BOOL INSTAPI
SQLSetKeywordValueW (LPCWSTR lpszSection,
    LPCWSTR lpszEntry, LPWSTR lpszString, int cbString)
{
  return SQLWritePrivateProfileStringW (lpszSection,
      lpszEntry, lpszString, L"odbc.ini");
}
