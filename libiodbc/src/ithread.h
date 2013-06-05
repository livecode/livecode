/*
 *  ithread.h
 *
 *  $Id: ithread.h,v 1.3 2006/01/20 15:58:35 source Exp $
 *
 *  Macros for locking & multihreading
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

#ifndef _ITHREAD_H
#define _ITHREAD_H


/*
 *  Threading under windows
 */
#if defined (WIN32) && !defined (NO_THREADING)

# define IODBC_THREADING

# define THREAD_IDENT			((unsigned long) GetCurrentThreadId())

# define MUTEX_DECLARE(M)		HANDLE M
# define MUTEX_INIT(M)			M = CreateMutex (NULL, FALSE, NULL)
# define MUTEX_DONE(M)			CloseHandle (M)
# define MUTEX_LOCK(M)			WaitForSingleObject (M, INFINITE)
# define MUTEX_UNLOCK(M)		ReleaseMutex (M)

# define SPINLOCK_DECLARE(M)		CRITICAL_SECTION M
# define SPINLOCK_INIT(M)		InitializeCriticalSection (&M)
# define SPINLOCK_DONE(M)		DeleteCriticalSection (&M)
# define SPINLOCK_LOCK(M)		EnterCriticalSection (&M)
# define SPINLOCK_UNLOCK(M)		LeaveCriticalSection (&M)


/*
 *  Threading with pthreads
 */
#elif defined (WITH_PTHREADS)

#ifndef _REENTRANT
# error Add -D_REENTRANT to your compiler flags
#endif

#include <pthread.h>

# define IODBC_THREADING

# ifndef OLD_PTHREADS
#  define THREAD_IDENT			((unsigned long) (pthread_self ()))
# else
#  define THREAD_IDENT			0UL
# endif

# define MUTEX_DECLARE(M)		pthread_mutex_t M
# define MUTEX_INIT(M)			pthread_mutex_init (&M, NULL)
# define MUTEX_DONE(M)			pthread_mutex_destroy (&M);
# define MUTEX_LOCK(M)			pthread_mutex_lock(&M)
# define MUTEX_UNLOCK(M)		pthread_mutex_unlock(&M)

# define SPINLOCK_DECLARE(M)		MUTEX_DECLARE(M)
# define SPINLOCK_INIT(M)		MUTEX_INIT(M)
# define SPINLOCK_DONE(M)		MUTEX_DONE(M)
# define SPINLOCK_LOCK(M)		MUTEX_LOCK(M)
# define SPINLOCK_UNLOCK(M)		MUTEX_UNLOCK(M)


/*
 *  No threading
 */
#else
	
# undef IODBC_THREADING

# undef THREAD_IDENT

# define MUTEX_DECLARE(M)		int M
# define MUTEX_INIT(M)			M = 1
# define MUTEX_DONE(M)			M = 1
# define MUTEX_LOCK(M)			M = 1
# define MUTEX_UNLOCK(M)		M = 1

# define SPINLOCK_DECLARE(M)		MUTEX_DECLARE (M)
# define SPINLOCK_INIT(M)		MUTEX_INIT (M)
# define SPINLOCK_DONE(M)		MUTEX_DONE (M)
# define SPINLOCK_LOCK(M)		MUTEX_LOCK (M)
# define SPINLOCK_UNLOCK(M)		MUTEX_UNLOCK (M)

#endif

#endif /* _ITHREAD_H */
