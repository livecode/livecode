/*
 *  SQLGetInstalledDrivers.c
 *
 *  $Id: SQLGetInstalledDrivers.c,v 1.10 2006/01/20 15:58:35 source Exp $
 *
 *  Get a list of installed drivers
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

#include "misc.h"
#include "iodbc_error.h"

#ifdef WIN32
#define SECT1			"ODBC 32 bit Data Sources"
#define SECT2			"ODBC 32 bit Drivers"
#else
#define SECT1			"ODBC Data Sources"
#define SECT2			"ODBC Drivers"
#endif

#define MAX_ENTRIES		1024

extern BOOL GetAvailableDrivers (LPCSTR lpszInfFile, LPSTR lpszBuf,
    WORD cbBufMax, WORD * pcbBufOut, BOOL infFile);

static int
SectSorter (const void *p1, const void *p2)
{
  const char **s1 = (const char **) p1;
  const char **s2 = (const char **) p2;

  return strcasecmp (*s1, *s2);
}

BOOL INSTAPI
SQLGetInstalledDrivers_Internal (LPSTR lpszBuf, WORD cbBufMax,
    WORD * pcbBufOut, SQLCHAR waMode)
{
  char buffer[4096], desc[1024], *ptr, *oldBuf = lpszBuf;
  int i, j, usernum = 0, num_entries = 0;
  void **sect = NULL;
  SQLUSMALLINT fDir = SQL_FETCH_FIRST_USER;

  if (pcbBufOut)
    *pcbBufOut = 0;

  /*
   *  Allocate the buffer for the list
   */
  if ((sect = (void **) calloc (MAX_ENTRIES, sizeof (void *))) == NULL)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      return SQL_FALSE;
    }

  do
    {
      SQLSetConfigMode (fDir ==
	  SQL_FETCH_FIRST_SYSTEM ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
      SQLGetPrivateProfileString (SECT2, NULL, "", buffer,
	  sizeof (buffer) / sizeof (SQLTCHAR), "odbcinst.ini");

      /* For each drivers */
      for (ptr = buffer, i = 1; *ptr && i; ptr += STRLEN (ptr) + 1)
	{
	  /* Add this section to the datasources list */
	  if (fDir == SQL_FETCH_FIRST_SYSTEM)
	    {
	      for (j = 0; j < usernum; j++)
		{
		  if (STREQ (sect[j], ptr))
		    j = usernum;
		}
	      if (j == usernum + 1)
		continue;
	    }

	  if (num_entries >= MAX_ENTRIES)
	    {
	      i = 0;
	      break;
	    }			/* Skip the rest */

	  /* ... and its description */
	  SQLSetConfigMode (fDir ==
	      SQL_FETCH_FIRST_SYSTEM ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
	  SQLGetPrivateProfileString (SECT2, ptr, "", desc,
	      sizeof (desc) / sizeof (SQLTCHAR), "odbcinst.ini");

	  /* Check if the driver is installed */
	  if (!STRCASEEQ (desc, "Installed"))
	    continue;

	  /* Copy the driver name */
	  sect[num_entries++] = STRDUP (ptr);
	}

      switch (fDir)
	{
	case SQL_FETCH_FIRST_USER:
	  fDir = SQL_FETCH_FIRST_SYSTEM;
	  usernum = num_entries;
	  break;
	case SQL_FETCH_FIRST_SYSTEM:
	  fDir = SQL_FETCH_FIRST;
	  break;
	}
    }
  while (fDir != SQL_FETCH_FIRST);

  /*
   *  Sort all entries so we can present a nice list
   */
  if (num_entries > 1)
    {
      qsort (sect, num_entries, sizeof (char **), SectSorter);

      /* Copy back the result */
      for (i = 0; cbBufMax > 0 && i < num_entries; i++)
	{
	  if (waMode == 'A')
	    {
	      STRNCPY (lpszBuf, sect[i], cbBufMax);
	      cbBufMax -= (STRLEN (sect[i]) + 1);
	      lpszBuf += (STRLEN (sect[i]) + 1);
	    }
	  else
	    {
	      dm_StrCopyOut2_A2W (sect[i], (LPWSTR) lpszBuf, cbBufMax, NULL);
	      cbBufMax -= (STRLEN (sect[i]) + 1);
	      lpszBuf += (STRLEN (sect[i]) + 1) * sizeof (wchar_t);
	    }
	}

      if (waMode == 'A')
	*lpszBuf = '\0';
      else
	*((wchar_t *) lpszBuf) = L'\0';
    }

  /*
   *  Free old section list
   */
  if (sect)
    {
      for (i = 0; i < MAX_ENTRIES; i++)
	if (sect[i])
	  free (sect[i]);
      free (sect);
    }

  if (pcbBufOut)
    *pcbBufOut =
	lpszBuf - oldBuf + (waMode == 'A' ? sizeof (char) : sizeof (wchar_t));

  return waMode == 'A' ? (oldBuf[0] ? SQL_TRUE : SQL_FALSE) :
      (((wchar_t *) oldBuf)[0] ? SQL_TRUE : SQL_FALSE);
}

BOOL INSTAPI
SQLGetInstalledDrivers (LPSTR lpszBuf, WORD cbBufMax, WORD * pcbBufOut)
{
  return SQLGetInstalledDrivers_Internal (lpszBuf, cbBufMax, pcbBufOut, 'A');
}

BOOL INSTAPI
SQLGetInstalledDriversW (LPWSTR lpszBuf, WORD cbBufMax, WORD FAR * pcbBufOut)
{
  return SQLGetInstalledDrivers_Internal ((LPSTR) lpszBuf, cbBufMax,
      pcbBufOut, 'W');
}
