/*
 *  SQLGetAvailableDrivers.c
 *
 *  $Id: SQLGetAvailableDrivers.c,v 1.10 2006/01/20 15:58:35 source Exp $
 *
 *  Get a list of all available drivers
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
#include "inifile.h"
#include "iodbc_error.h"

BOOL
GetAvailableDrivers (LPCSTR lpszInfFile, LPSTR lpszBuf, WORD cbBufMax,
    WORD * pcbBufOut, BOOL infFile)
{
  int sect_len = 0;
  WORD curr = 0;
  BOOL retcode = FALSE;
  PCONFIG pCfg;
  char *szId;

  if (!lpszBuf || !cbBufMax)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
      goto quit;
    }

  /* Open the file to read from */
  if (_iodbcdm_cfg_init (&pCfg, lpszInfFile, FALSE))
    {
      PUSH_ERROR (ODBC_ERROR_COMPONENT_NOT_FOUND);
      goto quit;
    }

  /* Get the ODBC Drivers section */
#ifdef WIN32
  if (_iodbcdm_cfg_find (pCfg, "ODBC 32 bit Drivers", NULL))
#else
  if (_iodbcdm_cfg_find (pCfg, "ODBC Drivers", NULL))
#endif
    {
      PUSH_ERROR (ODBC_ERROR_COMPONENT_NOT_FOUND);
      goto done;
    }

  while (curr < cbBufMax && 0 == _iodbcdm_cfg_nextentry (pCfg))
    {
      if (_iodbcdm_cfg_section (pCfg))
	break;

      if (_iodbcdm_cfg_define (pCfg) && pCfg->id)
	{
	  szId = pCfg->id;
	  while (infFile && *szId == '"')
	    szId++;

	  sect_len = STRLEN (szId);
	  if (!sect_len)
	    {
	      PUSH_ERROR (ODBC_ERROR_INVALID_INF);
	      goto done;
	    }

	  while (infFile && *(szId + sect_len - 1) == '"')
	    sect_len -= 1;

	  sect_len = sect_len > cbBufMax - curr ? cbBufMax - curr : sect_len;

	  if (sect_len)
	    memmove (lpszBuf + curr, szId, sect_len);
	  else
	    {
	      PUSH_ERROR (ODBC_ERROR_INVALID_INF);
	      goto done;
	    }

	  curr += sect_len;
	  lpszBuf[curr++] = 0;
	}
    }

  if (curr < cbBufMax)
    lpszBuf[curr + 1] = 0;
  if (pcbBufOut)
    *pcbBufOut = curr;
  retcode = TRUE;

done:
  _iodbcdm_cfg_done (pCfg);

quit:
  return retcode;
}


BOOL INSTAPI
SQLGetAvailableDrivers (LPCSTR lpszInfFile, LPSTR lpszBuf, WORD cbBufMax,
    WORD * pcbBufOut)
{
  BOOL retcode = FALSE;
  WORD lenBufOut;

  /* Get from the user files */
  CLEAR_ERROR ();

  switch (configMode)
    {
    case ODBC_BOTH_DSN:
    case ODBC_USER_DSN:
      wSystemDSN = USERDSN_ONLY;
      break;

    case ODBC_SYSTEM_DSN:
      wSystemDSN = SYSTEMDSN_ONLY;
      break;
    }

  retcode =
      GetAvailableDrivers (lpszInfFile, lpszBuf, cbBufMax, &lenBufOut, FALSE);

  if (pcbBufOut)
    *pcbBufOut = lenBufOut;

  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;
  return retcode;
}

BOOL INSTAPI
SQLGetAvailableDriversW (LPCWSTR lpszInfFile, LPWSTR lpszBuf, WORD cbBufMax,
    WORD FAR * pcbBufOut)
{
  BOOL retcode = FALSE;
  char *_inf_u8 = NULL;
  char *_buffer_u8 = NULL;
  SQLCHAR *ptr;
  SQLWCHAR *ptrW;
  WORD len = 0, length;

  _inf_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszInfFile, SQL_NTS);
  if (_inf_u8 == NULL && lpszInfFile)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      goto done;
    }

  if (cbBufMax > 0)
    {
      if ((_buffer_u8 = malloc (cbBufMax * UTF8_MAX_CHAR_LEN + 1)) == NULL)
	{
	  PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
	  goto done;
	}
    }

  retcode =
      SQLGetAvailableDrivers (_inf_u8, _buffer_u8,
      cbBufMax * UTF8_MAX_CHAR_LEN, pcbBufOut);

  if (retcode == TRUE)
    {
      length = 0;

      for (ptr = _buffer_u8, ptrW = lpszBuf; *ptr;
	  ptr += STRLEN (ptr) + 1, ptrW += WCSLEN (ptrW) + 1)
	{
	  dm_StrCopyOut2_U8toW (ptr, ptrW, cbBufMax - 1, &len);
	  length += len;
	}

      *ptrW = L'\0';
      if (pcbBufOut)
	*pcbBufOut = length + 1;
    }

done:
  MEM_FREE (_inf_u8);
  MEM_FREE (_buffer_u8);

  return retcode;
}
