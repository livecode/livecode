/*
 *  hdbc.h
 *
 *  $Id: hdbc.h,v 1.16 2006/01/20 15:58:34 source Exp $
 *
 *  Data source connect object management functions
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

#ifndef	_HDBC_H
#define	_HDBC_H

#if (ODBCVER >= 0x0300)
#include <hdesc.h>
#endif

typedef struct _drvopt
  {
    SQLUSMALLINT Option;
    SQLULEN Param;
    SQLCHAR waMode;

    struct _drvopt *next;
  } 
DRVOPT;

typedef struct DBC
  {
    int type;			/* must be 1st field */
    HERR herr;
    SQLRETURN rc;

    struct DBC * next;

    HENV genv;			/* back point to global env object */

    HDBC dhdbc;			/* driver's private dbc */
    HENV henv;			/* back point to instant env object */
    HSTMT hstmt;		/* list of statement object handle(s) */
#if (ODBCVER >= 0x300)
    HDESC hdesc;    		/* list of connection descriptors */
#endif    

    int state;

    /* options */
    UDWORD access_mode;
    UDWORD autocommit;

    UDWORD login_timeout;
    UDWORD odbc_cursors;
    UDWORD packet_size;
    UDWORD quiet_mode;
    UDWORD txn_isolation;
    SWORD cb_commit;
    SWORD cb_rollback;

    wchar_t * current_qualifier;
    char current_qualifier_WA;

    SWORD dbc_cip;			/* Call in Progess flag */

    DRVOPT *drvopt;			/* Driver specific connect options */
    SQLSMALLINT err_rec;
  }
DBC_t;


#define IS_VALID_HDBC(x) \
	((x) != SQL_NULL_HDBC && ((DBC_t *)(x))->type == SQL_HANDLE_DBC)


#define ENTER_HDBC(hdbc, holdlock, trace) \
	CONN(pdbc, hdbc); \
        SQLRETURN retcode = SQL_SUCCESS; \
        ODBC_LOCK();\
	TRACE(trace); \
    	if (!IS_VALID_HDBC (pdbc)) \
	  { \
	    retcode = SQL_INVALID_HANDLE; \
	    goto done; \
	  } \
	else if (pdbc->dbc_cip) \
          { \
	    PUSHSQLERR (pdbc->herr, en_S1010); \
	    retcode = SQL_ERROR; \
	    goto done; \
	  } \
	pdbc->dbc_cip = 1; \
	CLEAR_ERRORS (pdbc); \
	if (!holdlock) \
	  ODBC_UNLOCK()


#define LEAVE_HDBC(hdbc, holdlock, trace) \
	if (!holdlock) \
	  ODBC_LOCK (); \
	pdbc->dbc_cip = 0; \
    done: \
    	TRACE(trace); \
	ODBC_UNLOCK (); \
	return (retcode)


/* 
 * Note:
 *  - ODBC applications can see address of driver manager's 
 *    connection object, i.e connection handle -- a void pointer, 
 *    but not detail of it. ODBC applications can neither see 
 *    detail driver's connection object nor its address.
 *
 *  - ODBC driver manager knows its own connection objects and
 *    exposes their address to an ODBC application. Driver manager
 *    also knows address of driver's connection objects and keeps
 *    it via dhdbc field in driver manager's connection object.
 * 
 *  - ODBC driver exposes address of its own connection object to
 *    driver manager without detail.
 *
 *  - Applications can get driver's connection object handle by
 *    SQLGetInfo() with fInfoType equals to SQL_DRIVER_HDBC.
 */

enum
  {
    en_dbc_allocated,
    en_dbc_needdata,
    en_dbc_connected,
    en_dbc_hstmt
  };


/*
 *  Internal prototypes 
 */
SQLRETURN SQL_API _iodbcdm_SetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption, 
    SQLULEN vParam,
    SQLCHAR waMode);
SQLRETURN SQL_API _iodbcdm_GetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption, 
    SQLPOINTER pvParam,
    SQLCHAR waMode);
#endif
