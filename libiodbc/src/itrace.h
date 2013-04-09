/*
 *  itrace.h
 *
 *  $Id: itrace.h,v 1.11 2006/01/20 15:58:35 source Exp $
 *
 *  Trace functions
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

#ifndef	_ITRACE_H
#define _ITRACE_H

/*
 *  Trace function prototypes
 */
#include "trace/proto.h"

extern int ODBCSharedTraceFlag;


/*
 *  Usefull macros
 */
#ifdef NO_TRACING
#define TRACE(X)
#else
#define TRACE(X)	if (ODBCSharedTraceFlag) X
#endif

#define TRACE_ENTER	0, retcode
#define TRACE_LEAVE	1, retcode


#define CALL_DRIVER_FUNC( hdbc, errHandle, ret, proc, plist ) \
    { \
	ret = proc plist; \
	if (errHandle) ((GENV_t *)(errHandle))->rc = ret; \
    }


#define CALL_DRIVER( hdbc, errHandle, ret, proc, procid, plist ) \
    {\
	DBC_t *	t_pdbc = (DBC_t *)(hdbc);\
	ENV_t * t_penv = (ENV_t *)(t_pdbc->henv);\
\
	if (!t_penv->thread_safe)\
	    MUTEX_LOCK (t_penv->drv_lock);\
\
	CALL_DRIVER_FUNC( hdbc, errHandle, ret, proc, plist )\
\
	if (!t_penv->thread_safe)\
	    MUTEX_UNLOCK (t_penv->drv_lock);\
    }

#define CALL_UDRIVER(hdbc, errHandle, retcode, hproc, unicode_driver, procid, plist) \
    { \
	if (unicode_driver) \
	{ \
	    /* SQL_XXX_W */ \
	    if ((hproc = _iodbcdm_getproc (hdbc, procid ## W)) \
		!= SQL_NULL_HPROC) \
	    { \
		CALL_DRIVER (hdbc, errHandle, retcode, hproc, \
		    procid ## W, plist) \
	    } \
	} \
	else \
	{ \
	    /* SQL_XXX */   \
	    /* SQL_XXX_A */ \
	    if ((hproc = _iodbcdm_getproc (hdbc, procid)) \
		!= SQL_NULL_HPROC) \
	    { \
		CALL_DRIVER (hdbc, errHandle, retcode, hproc, \
		    procid, plist) \
	    } \
	    else \
	      if ((hproc = _iodbcdm_getproc (hdbc, procid ## A)) \
		  != SQL_NULL_HPROC) \
	      { \
		  CALL_DRIVER (hdbc, errHandle, retcode, hproc, \
		      procid ## A, plist) \
	      } \
	} \
    }

#endif
