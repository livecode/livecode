/*
 *  dlproc.c
 *
 *  $Id: dlproc.c,v 1.14 2006/01/20 15:58:34 source Exp $
 *
 *  Load driver and resolve driver's function entry point
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

#include <dlproc.h>

#include <herr.h>
#include <henv.h>
#include <hdbc.h>

#include <itrace.h>

char *odbcapi_symtab[] =
{
    "UNKNOWN FUNCTION"
#define FUNCDEF(A, B, C)	,C
#include "henv.ci"
#undef FUNCDEF
};


HPROC
_iodbcdm_getproc (HDBC hdbc, int idx)
{
  CONN (pdbc, hdbc);
  ENV_t *penv;
  HPROC *phproc;

  if (idx <= 0 || idx >= __LAST_API_FUNCTION__)
    return SQL_NULL_HPROC;

  penv = (ENV_t *) (pdbc->henv);

  if (penv == NULL)
    return SQL_NULL_HPROC;

  phproc = penv->dllproc_tab + idx;

  if (*phproc == SQL_NULL_HPROC)
    *phproc = _iodbcdm_dllproc (penv->hdll, odbcapi_symtab[idx]);

  return *phproc;
}


static dlproc_t *pRoot = NULL;


HDLL
_iodbcdm_dllopen (char *path)
{
  dlproc_t *pDrv = NULL, *p;

  /*
   *  Check if we have already loaded the driver
   */
  for (p = pRoot; p; p = p->next)
    {
      if (STREQ (p->path, path))
	{
	  pDrv = p;
	  break;
	}
    }

  /*
   *  If already loaded, increase ref counter
   */
  if (pDrv)
    {
      pDrv->refcount++;

      /*
       *  If the driver was unloaded, load it again
       */
      if (pDrv->dll == NULL)
	pDrv->dll = (HDLL) DLL_OPEN (path);

      return pDrv->dll;
    }

  /*
   *  Initialize new structure
   */
  if ((pDrv = calloc (1, sizeof (dlproc_t))) == NULL)
    return NULL;

  pDrv->refcount = 1;
  pDrv->path = STRDUP (path);
  pDrv->dll = (HDLL) DLL_OPEN (path);

  /*
   *  Add to linked list
   */
  pDrv->next = pRoot;
  pRoot = pDrv;

  return pDrv->dll;
}


HPROC
_iodbcdm_dllproc (HDLL hdll, char *sym)
{
  return (HPROC) DLL_PROC (hdll, sym);
}


int
_iodbcdm_dllclose (HDLL hdll)
{
  dlproc_t *pDrv = NULL, *p;

  /*
   *  Find loaded driver
   */
  for (p = pRoot; p; p = p->next)
    {
      if (p->dll == hdll)
	{
	  pDrv = p;
	  break;
	}
    }

  /*
   *  Not found
   */
  if (!pDrv)
    return -1;

  /*
   *  Decrease reference counter
   */
  pDrv->refcount--;

  /*
   *  Check if it is possible to unload the driver safely
   * 
   *  NOTE: Some drivers set explicit on_exit hooks, which makes it
   *        impossible for the driver manager to unload the driver
   *        as this would crash the executable at exit.
   */
  if (pDrv->refcount == 0 && pDrv->safe_unload)
    {
      DLL_CLOSE (pDrv->dll);
      pDrv->dll = NULL;
    }

  return 0;
}


char *
_iodbcdm_dllerror ()
{
  return DLL_ERROR ();
}


/* 
 *  If driver manager determines this driver is safe, flag the driver can
 *  be unloaded if not used.
 */
void
_iodbcdm_safe_unload (HDLL hdll)
{
  dlproc_t *pDrv = NULL, *p;

  /*
   *  Find loaded driver
   */
  for (p = pRoot; p; p = p->next)
    {
      if (p->dll == hdll)
	{
	  pDrv = p;
	  break;
	}
    }

  /*
   *  Driver not found
   */
  if (!pDrv)
    return;

  pDrv->safe_unload = 1;
}
