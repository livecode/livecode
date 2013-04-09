/*
 *  hdesc.h
 *
 *  $Id: hdesc.h,v 1.9 2006/01/20 15:58:34 source Exp $
 *
 *  Descriptor object
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


#ifndef __DESC_H
#define __DESC_H

#define APP_ROW_DESC	0
#define APP_PARAM_DESC	1
#define IMP_ROW_DESC	2
#define IMP_PARAM_DESC	3

typedef struct DESC_s {

  int type;
  HERR herr;   		/* list of descriptor errors */
  SQLRETURN rc;
  
  struct DESC_s *next;

  SQLHDBC hdbc;	 	/* connection associated with the descriptor */
  SQLHDESC dhdesc; 	/* the driver's desc handle */
  HSTMT hstmt;   	/* if not null - the descriptor is implicit to that statement */

  SWORD desc_cip;        /* Call in Progess flag */

  SQLSMALLINT err_rec;
} DESC_t;

#ifndef HDESC
#define HDESC SQLHDESC
#endif


#define IS_VALID_HDESC(x) \
	((x) != SQL_NULL_HDESC && \
	 ((DESC_t *)(x))->type == SQL_HANDLE_DESC && \
	 ((DESC_t *)(x))->hdbc != SQL_NULL_HDBC)


#define ENTER_DESC(hdesc, trace) \
	DESC (pdesc, hdesc); \
	SQLRETURN retcode = SQL_SUCCESS; \
        ODBC_LOCK();\
	TRACE(trace); \
    	if (!IS_VALID_HDESC (pdesc)) \
	  { \
	    retcode = SQL_INVALID_HANDLE; \
	    goto done; \
	  } \
	else if (pdesc->desc_cip) \
          { \
	    PUSHSQLERR (pdesc->herr, en_S1010); \
	    retcode = SQL_ERROR; \
	    goto done; \
	  } \
	pdesc->desc_cip = 1; \
	CLEAR_ERRORS (pdesc); \
	ODBC_UNLOCK()


#define LEAVE_DESC(hdesc, trace) \
	ODBC_LOCK (); \
    done: \
    	TRACE(trace); \
	pdesc->desc_cip = 0; \
	ODBC_UNLOCK (); \
	return (retcode)

#endif /* __DESC_H */
