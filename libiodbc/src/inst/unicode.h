/*
 *  unicode.h
 *
 *  $Id: unicode.h,v 1.2 2006/01/20 15:58:35 source Exp $
 *
 *  ODBC unicode support
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

#ifndef _UNICODE_H
#define _UNICODE_H

#if HAVE_WCHAR_H
#include <wchar.h>
#endif


/*
 *  Max length of a UTF-8 encoded character sequence
 */
#define UTF8_MAX_CHAR_LEN 4

/*
 *  Function Prototypes
 */
SQLCHAR *dm_SQL_W2A (SQLWCHAR * inStr, ssize_t size);
SQLCHAR *dm_SQL_WtoU8 (SQLWCHAR * inStr, ssize_t size);
SQLCHAR *dm_strcpy_W2A (SQLCHAR * destStr, SQLWCHAR * sourStr);

SQLWCHAR *dm_SQL_A2W (SQLCHAR * inStr, ssize_t size);
SQLWCHAR *dm_SQL_U8toW (SQLCHAR * inStr, SQLSMALLINT size);
SQLWCHAR *dm_strcpy_A2W (SQLWCHAR * destStr, SQLCHAR * sourStr);

int dm_StrCopyOut2_A2W (SQLCHAR * inStr, SQLWCHAR * outStr, SQLSMALLINT size,
    SQLSMALLINT * result);
int dm_StrCopyOut2_U8toW (SQLCHAR * inStr, SQLWCHAR * outStr, size_t size,
    u_short * result);
int dm_StrCopyOut2_W2A (SQLWCHAR * inStr, SQLCHAR * outStr, SQLSMALLINT size,
    SQLSMALLINT * result);


# ifdef WIN32
#define OPL_W2A(w, a, cb)     \
	WideCharToMultiByte(CP_ACP, 0, w, cb, a, cb, NULL, NULL)

#define OPL_A2W(a, w, cb)     \
	MultiByteToWideChar(CP_ACP, 0, a, cb, w, cb)

# else
#define OPL_W2A(XW, XA, SIZE)      wcstombs((char *) XA, (wchar_t *) XW, SIZE)
#define OPL_A2W(XA, XW, SIZE)      mbstowcs((wchar_t *) XW, (char *) XA, SIZE)
# endif

#if MACOSX >= 103
#define HAVE_WCHAR_H 
#define HAVE_WCSLEN 
#define HAVE_WCSCPY 
#define HAVE_WCSNCPY
#define HAVE_WCSCHR
#define HAVE_WCSCAT
#define HAVE_WCSCMP
#define HAVE_TOWLOWER
#endif

/*
 *  Replacement functions
 */
#if !defined(HAVE_WCSLEN)
size_t wcslen (const wchar_t * wcs);
#endif
#if !defined(HAVE_WCSCPY)
wchar_t * wcscpy (wchar_t * wcd, const wchar_t * wcs);
#endif
#if !defined(HAVE_WCSNCPY)
wchar_t * wcsncpy (wchar_t * wcd, const wchar_t * wcs, size_t n);
#endif
#if !defined(HAVE_WCSCHR)
wchar_t* wcschr(const wchar_t *wcs, const wchar_t wc);
#endif
#if !defined(HAVE_WCSCAT)
wchar_t* wcscat(wchar_t *dest, const wchar_t *src);
#endif
#if !defined(HAVE_WCSCMP)
int wcscmp (const wchar_t* s1, const wchar_t* s2);
#endif
#if !defined(HAVE_WCSNCASECMP)
int wcsncasecmp (wchar_t* s1, wchar_t* s2, size_t n);
#endif

#endif /* _UNICODE_H */
