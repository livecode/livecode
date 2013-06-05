/*
 *  henv.c
 *
 *  $Id: henv.c,v 1.21 2006/01/24 00:09:25 source Exp $
 *
 *  Environment object management functions
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

#include <dlproc.h>

#include <herr.h>
#include <henv.h>

#include <itrace.h>

/*
 *  Use static initializer where possible
 */
#if defined (PTHREAD_MUTEX_INITIALIZER)
SPINLOCK_DECLARE (iodbcdm_global_lock) = PTHREAD_MUTEX_INITIALIZER;
#else
SPINLOCK_DECLARE (iodbcdm_global_lock);
#endif

static int _iodbcdm_initialized = 0;


static void
_iodbcdm_env_settracing (GENV_t *genv)
{
  char buf[1024];

  genv = genv; /*UNUSED*/

  /*
   *  Check TraceFile keyword
   */
  SQLSetConfigMode (ODBC_BOTH_DSN);
  if( SQLGetPrivateProfileString ("ODBC", "TraceFile", "", buf, sizeof(buf) / sizeof(SQLTCHAR), "odbc.ini") == 0 || !buf[0])
    STRCPY (buf, SQL_OPT_TRACE_FILE_DEFAULT);
  trace_set_filename (buf);

  /*
   *  Check Trace keyword
   */
  SQLSetConfigMode (ODBC_BOTH_DSN);
  if ( SQLGetPrivateProfileString ("ODBC", "Trace", "", buf, sizeof(buf) / sizeof(SQLTCHAR), "odbc.ini") &&
      (STRCASEEQ (buf, "on") || STRCASEEQ (buf, "yes")
   || STRCASEEQ (buf, "1")))
    trace_start ();

  return;
}

unsigned long _iodbc_env_counter = 0;

SQLRETURN 
SQLAllocEnv_Internal (SQLHENV * phenv, int odbc_ver)
{
  GENV (genv, NULL);
  int retcode = SQL_SUCCESS;

  genv = (GENV_t *) MEM_ALLOC (sizeof (GENV_t));

  if (genv == NULL)
    {
      *phenv = SQL_NULL_HENV;

      return SQL_ERROR;
    }
  genv->rc = 0;

  /*
   *  Initialize this handle
   */
  genv->type = SQL_HANDLE_ENV;
  genv->henv = SQL_NULL_HENV;	/* driver's env list */
  genv->hdbc = SQL_NULL_HDBC;	/* driver's dbc list */
  genv->herr = SQL_NULL_HERR;	/* err list          */
#if (ODBCVER >= 0x300)
  genv->odbc_ver = odbc_ver;
#endif
  genv->err_rec = 0;

  *phenv = (SQLHENV) genv;

  /*
   * Initialize tracing 
   */
  if (++_iodbc_env_counter == 1)
    _iodbcdm_env_settracing (genv);

  return retcode;
}


SQLRETURN SQL_API
SQLAllocEnv (SQLHENV * phenv)
{
  GENV (genv, NULL);
  int retcode = SQL_SUCCESS;

  /* 
   *  One time initialization
   */
  Init_iODBC();

  ODBC_LOCK ();
  retcode = SQLAllocEnv_Internal (phenv, SQL_OV_ODBC2);

  genv = (GENV_t *) *phenv;

  /*
   * Start tracing
   */
  TRACE (trace_SQLAllocEnv (TRACE_ENTER, phenv));
  TRACE (trace_SQLAllocEnv (TRACE_LEAVE, phenv));

  ODBC_UNLOCK ();

  return retcode;
}


SQLRETURN
SQLFreeEnv_Internal (SQLHENV henv)
{
  GENV (genv, henv);

  if (!IS_VALID_HENV (genv))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (genv);

  if (genv->hdbc != SQL_NULL_HDBC)
    {
      PUSHSQLERR (genv->herr, en_S1010);

      return SQL_ERROR;
    }

  /*
   *  Invalidate this handle
   */
  genv->type = 0;

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLFreeEnv (SQLHENV henv)
{
  GENV (genv, henv);
  int retcode = SQL_SUCCESS;

  ODBC_LOCK ();

  TRACE (trace_SQLFreeEnv (TRACE_ENTER, henv));

  retcode = SQLFreeEnv_Internal (henv);

  TRACE (trace_SQLFreeEnv (TRACE_LEAVE, henv));

  MEM_FREE (genv);

  /*
   *  Close trace after last environment handle is freed
   */
  if (--_iodbc_env_counter == 0)
    trace_stop();

  ODBC_UNLOCK ();

  return retcode;
}


/*
 *  Initialize the system and let everyone wait until we have done so
 *  properly
 */
void
Init_iODBC (void)
{
#if !defined (PTHREAD_MUTEX_INITIALIZER) || defined (WINDOWS)
  SPINLOCK_INIT (iodbcdm_global_lock);
#endif

  SPINLOCK_LOCK (iodbcdm_global_lock);
  if (!_iodbcdm_initialized)
    {
      /*
       *  OK, now flag we are not callable anymore
       */
      _iodbcdm_initialized = 1;

      /*
       *  Other one time initializations can be performed here
       */
    }
  SPINLOCK_UNLOCK (iodbcdm_global_lock);

  return;
}


void 
Done_iODBC(void)
{
#if !defined (PTHREAD_MUTEX_INITIALIZER) || defined (WINDOWS)
    SPINLOCK_DONE (iodbcdm_global_lock);
#endif
}


/*
 *  DLL Entry points for Windows
 */
#if defined (WINDOWS)
static int
DLLInit (HINSTANCE hModule)
{
  Init_iODBC ();

  return TRUE;
}


static void
DLLExit (void)
{
  Done_iODBC ();
}


#pragma argused
BOOL WINAPI
DllMain (HINSTANCE hModule, DWORD fdReason, LPVOID lpvReserved)
{
  switch (fdReason)
    {
    case DLL_PROCESS_ATTACH:
      if (!DLLInit (hModule))
	return FALSE;
      break;
    case DLL_PROCESS_DETACH:
      DLLExit ();
    }
  return TRUE;
}
#endif
