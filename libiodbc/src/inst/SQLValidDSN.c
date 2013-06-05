/*
 *  SQLValidDSN.c
 *
 *  $Id: SQLValidDSN.c,v 1.7 2006/01/20 15:58:35 source Exp $
 *
 *  Validate a DSN name
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

#include "iodbc_error.h"

#define INVALID_CHARS	"[]{}(),;?*=!@\\"
#define INVALID_CHARSW	L"[]{}(),;?*=!@\\"

BOOL
ValidDSN (LPCSTR lpszDSN)
{
  char *currp = (char *) lpszDSN;

  while (*currp)
    {
      if (strchr (INVALID_CHARS, *currp))
	return FALSE;
      else
	currp++;
    }

  return TRUE;
}


BOOL
ValidDSNW (LPCWSTR lpszDSN)
{
  wchar_t *currp = (wchar_t *) lpszDSN;

  while (*currp)
    {
      if (wcschr (INVALID_CHARSW, *currp))
	return FALSE;
      else
	currp++;
    }

  return TRUE;
}


BOOL INSTAPI
SQLValidDSN (LPCSTR lpszDSN)
{
  BOOL retcode = FALSE;

  /* Check dsn */
  CLEAR_ERROR ();
  if (!lpszDSN || !STRLEN (lpszDSN) || STRLEN (lpszDSN) >= SQL_MAX_DSN_LENGTH)
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto quit;
    }

  retcode = ValidDSN (lpszDSN);

quit:
  return retcode;
}

BOOL INSTAPI
SQLValidDSNW (LPCWSTR lpszDSN)
{
  BOOL retcode = FALSE;

  /* Check dsn */
  CLEAR_ERROR ();
  if (!lpszDSN || !WCSLEN (lpszDSN) || WCSLEN (lpszDSN) >= SQL_MAX_DSN_LENGTH)
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto quit;
    }

  retcode = ValidDSNW (lpszDSN);

quit:
  return retcode;
}
