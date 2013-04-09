/*
 *  SQLReadFileDSN.c
 *
 *  $Id: SQLReadFileDSN.c,v 1.7 2006/01/20 15:58:35 source Exp $
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
#include <unicode.h>

#include "inifile.h"
#include "iodbc_error.h"
#include "misc.h"

extern int GetPrivateProfileString (LPCSTR lpszSection, LPCSTR lpszEntry,
    LPCSTR lpszDefault, LPSTR lpszRetBuffer, int cbRetBuffer,
    LPCSTR lpszFilename);

BOOL INSTAPI
SQLReadFileDSN (LPCSTR lpszFileName, LPCSTR lpszAppName, LPCSTR lpszKeyName,
    LPSTR lpszString, WORD cbString, WORD * pcbString)
{
  BOOL retcode = FALSE;
  WORD len = 0, i;

  /* Check input parameters */
  CLEAR_ERROR ();

  if (!lpszString || !cbString)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
      goto quit;
    }

  if (!lpszAppName && lpszKeyName)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
      goto quit;
    }

  /* Is a file is specified */
  if (lpszFileName)
    {
      len =
	  GetPrivateProfileString (lpszAppName, lpszKeyName, "", lpszString,
	  cbString, lpszFileName);
      if (numerrors == -1)
	retcode = TRUE;
      goto quit;
    }

  PUSH_ERROR (ODBC_ERROR_INVALID_PATH);
  goto quit;

quit:
  for (i = 0; i < len; i++)
    if (!lpszString[i])
      lpszString[i] = ';';

  if (pcbString)
    *pcbString = len;

  if (len == cbString - 1)
    {
      PUSH_ERROR (ODBC_ERROR_OUTPUT_STRING_TRUNCATED);
      retcode = FALSE;
    }

  return retcode;
}

BOOL INSTAPI
SQLReadFileDSNW (LPCWSTR lpszFileName, LPCWSTR lpszAppName,
    LPCWSTR lpszKeyName, LPWSTR lpszString, WORD cbString, WORD * pcbString)
{
  char *_filename_u8 = NULL;
  char *_appname_u8 = NULL;
  char *_keyname_u8 = NULL;
  char *_string_u8 = NULL;
  BOOL retcode = FALSE;

  _filename_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszFileName, SQL_NTS);
  if (_filename_u8 == NULL && lpszFileName)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      goto done;
    }

  _appname_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszAppName, SQL_NTS);
  if (_appname_u8 == NULL && lpszAppName)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      goto done;
    }

  _keyname_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszKeyName, SQL_NTS);
  if (_keyname_u8 == NULL && lpszKeyName)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      goto done;
    }

  if (cbString > 0)
    {
      if ((_string_u8 = malloc (cbString * UTF8_MAX_CHAR_LEN + 1)) == NULL)
	{
	  PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
	  goto done;
	}
    }

  retcode =
      SQLReadFileDSN (_filename_u8, _appname_u8, _keyname_u8, _string_u8,
      cbString * UTF8_MAX_CHAR_LEN, pcbString);

  if (retcode == TRUE)
    {
      dm_StrCopyOut2_U8toW (_string_u8, lpszString, cbString, pcbString);
    }

done:
  MEM_FREE (_filename_u8);
  MEM_FREE (_appname_u8);
  MEM_FREE (_keyname_u8);
  MEM_FREE (_string_u8);

  return retcode;
}
