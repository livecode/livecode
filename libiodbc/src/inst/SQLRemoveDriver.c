/*
 *  SQLRemoveDriver.c
 *
 *  $Id: SQLRemoveDriver.c,v 1.8 2006/01/20 15:58:35 source Exp $
 *
 *  Remove a driver
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


BOOL INSTAPI
SQLRemoveDriver (LPCSTR lpszDriver, BOOL fRemoveDSN, LPDWORD lpdwUsageCount)
{
  /*char *szDriverFile = NULL; */
  BOOL retcode = FALSE;
  PCONFIG pCfg = NULL, pInstCfg = NULL;
  LPSTR entries = (LPSTR) malloc (sizeof (char) * 65535), curr;
  int len = 0, i = 0;

  /* Check input parameters */
  CLEAR_ERROR ();
  if (!lpszDriver || !STRLEN (lpszDriver))
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
      goto quit;
    }

  /* Else go through user/system odbcinst.ini */
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

  if (_iodbcdm_cfg_search_init (&pCfg, "odbc.ini", FALSE))
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto done;
    }

  if (_iodbcdm_cfg_search_init (&pInstCfg, "odbcinst.ini", FALSE))
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto done;
    }

  if (fRemoveDSN)
    {
#ifdef WIN32
      if (entries &&
	  (len = _iodbcdm_list_entries (pCfg, "ODBC 32 bit Data Sources",
		  entries, 65535)))
#else
      if (entries
	  && (len =
	      _iodbcdm_list_entries (pCfg, "ODBC Data Sources", entries,
		  65535)))
#endif
	{
	  for (curr = entries; i < len;
	      i += (STRLEN (curr) + 1), curr += (STRLEN (curr) + 1))
	    {
	      int nCursor = pCfg->cursor;

	      if (_iodbcdm_cfg_rewind (pCfg))
		{
		  PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
		  goto done;
		}

#ifdef WIN32
	      if (_iodbcdm_cfg_find (pCfg, "ODBC 32 bit Data Sources", curr))
#else
	      if (_iodbcdm_cfg_find (pCfg, "ODBC Data Sources", curr))
#endif
		{
		  if (_iodbcdm_cfg_rewind (pCfg))
		    {
		      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
		      goto done;
		    }
		  pCfg->cursor = nCursor;
		  continue;
		}

	      if (!strcmp (pCfg->value, lpszDriver))
		{
		  if (_iodbcdm_cfg_write (pCfg, curr, NULL, NULL))
		    {
		      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
		      goto done;
		    }

#ifdef WIN32
		  if (_iodbcdm_cfg_write (pCfg, "ODBC 32 bit Data Sources",
			  curr, NULL))
#else
		  if (_iodbcdm_cfg_write (pCfg, "ODBC Data Sources", curr,
			  NULL))
#endif
		    {
		      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
		      goto done;
		    }
		}

	      pCfg->cursor = nCursor;
	    }
	}
    }

  if (_iodbcdm_cfg_write (pInstCfg, (char *) lpszDriver, NULL, NULL))
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto done;
    }

#ifdef WIN32
  if (_iodbcdm_cfg_write (pInstCfg, "ODBC 32 bit Drivers", lpszDriver, NULL))
#else
  if (_iodbcdm_cfg_write (pInstCfg, "ODBC Drivers", (LPSTR) lpszDriver, NULL))
#endif
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto done;
    }

  if (_iodbcdm_cfg_commit (pCfg) || _iodbcdm_cfg_commit (pInstCfg))
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto done;
    }

  retcode = TRUE;

done:
  if (pCfg)
    _iodbcdm_cfg_done (pCfg);
  if (pInstCfg)
    _iodbcdm_cfg_done (pInstCfg);
  if (entries)
    free (entries);

quit:
  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;

  return retcode;
}

BOOL INSTAPI
SQLRemoveDriverW (LPCWSTR lpszDriver, BOOL fRemoveDSN, LPDWORD lpdwUsageCount)
{
  char *_driver_u8 = NULL;
  BOOL retcode = FALSE;

  _driver_u8 = (char *) dm_SQL_WtoU8 ((SQLWCHAR *) lpszDriver, SQL_NTS);
  if (_driver_u8 == NULL && lpszDriver)
    {
      PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
      goto done;
    }

  retcode = SQLRemoveDriver (_driver_u8, fRemoveDSN, lpdwUsageCount);

done:
  MEM_FREE (_driver_u8);

  return retcode;
}
