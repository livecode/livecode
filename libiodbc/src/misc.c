/*
 *  misc.c
 *
 *  $Id: misc.c,v 1.22 2006/01/20 15:58:35 source Exp $
 *
 *  Miscellaneous functions
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1995 by Ke Jin <kejin@empress.com>
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

#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <unicode.h>

#include <fcntl.h>
#include <sys/stat.h>

#include "herr.h"

#ifdef _MAC
#include <getfpn.h>
#endif /* _MAC */

static int
upper_strneq (
    char *s1,
    char *s2,
    int n)
{
  int i;
  char c1 = 0 , c2 = 0;

  for (i = 1; i < n; i++)
    {
      c1 = s1[i];
      c2 = s2[i];

      if (c1 >= 'a' && c1 <= 'z')
	{
	  c1 += ('A' - 'a');
	}
      else if (c1 == '\n')
	{
	  c1 = '\0';
	}

      if (c2 >= 'a' && c2 <= 'z')
	{
	  c2 += ('A' - 'a');
	}
      else if (c2 == '\n')
	{
	  c2 = '\0';
	}

      if ((c1 - c2) || !c1 || !c2)
	{
	  break;
	}
    }

  return (int) !(c1 - c2);
}


static char *
_iodbcdm_getkeyvalinstr_Internal (char *str,
    int cnlen,
    void *keywd,
    void *value,
    int size)
{
  char *cp, *n;
  char *s, *tmp;
  int count;

  cnlen=cnlen; /*UNUSED*/ 

  if (str == NULL || (s = tmp = strdup (str)) == NULL)
    return NULL;

  for (count = 0; *s; count++)
    {
      /* 
       *  Extract KEY=VALUE upto first ';'
       */
      for (cp = s; *cp && *cp != ';'; cp++)
	{
	  if (*cp == '{')
	    {
	      for (cp++; *cp && *cp != '}'; cp++)
		;
	    }
	}

      /*
       *  Store start of next token if available in n and terminate string
       */
      if (*cp)
	{
	  *cp = 0;
	  n = cp + 1;
	}
      else
	n = cp;

      /*
       *  Find '=' in string
       */
      for (cp = s; *cp && *cp != '='; cp++)
	;

      /*
       *  Check if this is keyword we are searching for
       */
      if (*cp)
	{
	  *cp++ = 0;
	  if (upper_strneq ((char *) s, (char *) keywd, STRLEN (keywd)))
	    {
	      strncpy ((char *) value, (char *) cp, size);
	      free (tmp);
	      return (char *) value;
	    }
	}
      else if (count == 0)
	{
	  /*
	   *  Handle missing DSN=... from the beginning of the string, e.g.:
	   *  'dsn_ora7;UID=scott;PWD=tiger'
	   */
	  if (upper_strneq ("DSN", (char *) keywd, STRLEN (keywd)))
	    {
	      strncpy ((char *) value, (char *) s, size);
	      free (tmp);
	      return (char *) value;
	    }
	}

      /*
       *  Else continue with next token
       */
      s = n;
    }

  free (tmp);
  return NULL;
}


char *
_iodbcdm_getkeyvalinstr (char *cnstr,
    int cnlen, char *keywd, char *value, int size)
{
  return _iodbcdm_getkeyvalinstr_Internal(cnstr, cnlen, keywd, value, size);
}

wchar_t *
_iodbcdm_getkeyvalinstrw (wchar_t *cnstr,
    int cnlen, wchar_t *keywd, wchar_t *value, int size)
{
  char *ret = NULL;
  char *_cnstr;
  char *_keywd;
  char *buf = NULL;

  if (size > 0)
    {
      if ((buf = (char *) malloc (size * UTF8_MAX_CHAR_LEN + 1)) == NULL)
	return NULL;
    }

  _cnstr = (char *) dm_SQL_WtoU8 (cnstr, cnlen);
  _keywd = (char *) dm_SQL_WtoU8 (keywd, SQL_NTS);

  ret = _iodbcdm_getkeyvalinstr_Internal (_cnstr, SQL_NTS, _keywd, buf,
      size * UTF8_MAX_CHAR_LEN);

  MEM_FREE (_cnstr);
  MEM_FREE (_keywd);

  if (ret != NULL)
    {
      dm_StrCopyOut2_U8toW ((SQLCHAR *) ret, value, size, NULL);
      MEM_FREE (buf);
      return value;
    }

  MEM_FREE (buf);
  return NULL;
}
