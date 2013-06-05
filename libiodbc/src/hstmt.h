/*
 *  hstmt.h
 *
 *  $Id: hstmt.h,v 1.18 2006/01/20 15:58:34 source Exp $
 *
 *  Query statement object management functions
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

#ifndef	_HSTMT_H
#define	_HSTMT_H

typedef struct PARAM
  {
    void *data;
    int   length;
  }
PARAM_t;

#define STMT_PARAMS_MAX      8


/*
 *  Binding parameter from SQLBindCol
 */
typedef struct BIND {
  UWORD		 bn_col;	  /* Column # */
  SWORD		 bn_type;	  /* ODBC C data type */
  void *	 bn_data;	  /* Pointer to data */
  SDWORD	 bn_size;	  /* Size of data area */
  SQLLEN	*bn_pInd;	  /* Holds SQL_NULL_DATA | 0. 
                                   * And length of returned char/bin data 
				   */
} BIND_t;

typedef struct SBLST	TBLST, *PBLST;
/*
 *  Binding cell on the linked list
 */
struct SBLST {
  PBLST		 bl_nextBind;	/* Next binding */
  BIND_t	 bl_bind;	/* Binding information */
};


typedef struct STMT
  {
    int type;			/* must be 1st field */
    HERR herr;
    SQLRETURN rc;		/* Return code of last function */

    struct STMT *next;

    HDBC hdbc;			/* back point to connection object */

    HSTMT dhstmt;		/* driver's stmt handle */

    int state;
    int cursor_state;
    int prep_state;
    int asyn_on;		/* async executing which odbc call */
    int need_on;		/* which call return SQL_NEED_DATA */

    int stmt_cip;		/* Call in progress on this handle */

    SQLUINTEGER rowset_size;
    SQLUINTEGER bind_type;

#if (ODBCVER >= 0x0300)
    DESC_t * imp_desc[4];
    DESC_t * desc[4];
    SQLUINTEGER row_array_size;
    SQLPOINTER fetch_bookmark_ptr, params_processed_ptr;
    SQLUINTEGER paramset_size;
    SQLPOINTER row_status_ptr;
    SQLPOINTER rows_fetched_ptr;
    SQLUSMALLINT row_status_allocated;
#endif

    SQLSMALLINT err_rec;

    PARAM_t params[STMT_PARAMS_MAX]; /* for a conversion parameters ansi<=>unicode*/
    int     params_inserted;

    PBLST   st_pbinding;	/* API user bindings from SQLBindCol */
  }
STMT_t;


#define IS_VALID_HSTMT(x) \
	((x) != SQL_NULL_HSTMT && \
	 ((STMT_t *)(x))->type == SQL_HANDLE_STMT && \
	 ((STMT_t *)(x))->hdbc != SQL_NULL_HDBC)


#define ENTER_STMT(hstmt, trace) \
	STMT (pstmt, hstmt); \
	SQLRETURN retcode = SQL_SUCCESS; \
        ODBC_LOCK(); \
	TRACE (trace); \
    	if (!IS_VALID_HSTMT (pstmt)) \
	  { \
	    retcode = SQL_INVALID_HANDLE; \
	    goto done; \
	  } \
	else if (pstmt->stmt_cip) \
          { \
	    PUSHSQLERR (pstmt->herr, en_S1010); \
	    retcode = SQL_ERROR; \
	    goto done; \
	  } \
	pstmt->stmt_cip = 1; \
	CLEAR_ERRORS (pstmt); \
	if (pstmt->asyn_on == en_NullProc && pstmt->params_inserted > 0) \
	  _iodbcdm_FreeStmtParams(pstmt); \
        ODBC_UNLOCK()
	

#define LEAVE_STMT(hstmt, trace) \
	ODBC_LOCK (); \
	pstmt->stmt_cip = 0; \
    done: \
    	TRACE(trace); \
	ODBC_UNLOCK (); \
	return (retcode)


enum
  {
    en_stmt_allocated = 0,
    en_stmt_prepared,
    en_stmt_executed_with_info,
    en_stmt_executed,
    en_stmt_cursoropen,
    en_stmt_fetched,
    en_stmt_xfetched,
    en_stmt_needdata,		/* not call SQLParamData() yet */
    en_stmt_mustput,		/* not call SQLPutData() yet */
    en_stmt_canput		/* SQLPutData() called */
  };				/* for statement handle state */

enum
  {
    en_stmt_cursor_no = 0,
    en_stmt_cursor_named,
    en_stmt_cursor_opened,
    en_stmt_cursor_fetched,
    en_stmt_cursor_xfetched
  };				/* for statement cursor state */


/*
 *  Internal prototypes
 */
SQLRETURN _iodbcdm_dropstmt (HSTMT stmt);

void _iodbcdm_FreeStmtParams(STMT_t *pstmt);
void *_iodbcdm_alloc_param(STMT_t *pstmt, int i, int size);
wchar_t *_iodbcdm_conv_param_A2W(STMT_t *pstmt, int i, SQLCHAR *pData, int pDataLength);
char *_iodbcdm_conv_param_W2A(STMT_t *pstmt, int i, SQLWCHAR *pData, int pDataLength);
void _iodbcdm_ConvBindData (STMT_t *pstmt);
SQLRETURN _iodbcdm_BindColumn (STMT_t *pstmt, BIND_t *pbind);
int _iodbcdm_UnBindColumn (STMT_t *pstmt, BIND_t *pbind);
void _iodbcdm_RemoveBind (STMT_t *pstmt);
void _iodbcdm_do_cursoropen (STMT_t * pstmt);
SQLSMALLINT _iodbcdm_map_sql_type (int type, int odbcver);
SQLSMALLINT _iodbcdm_map_c_type (int type, int odbcver);


SQLRETURN SQL_API _iodbcdm_ExtendedFetch (
    SQLHSTMT		  hstmt,
    SQLUSMALLINT	  fFetchType,
    SQLLEN		  irow, 
    SQLULEN	 	* pcrow, 
    SQLUSMALLINT 	* rgfRowStatus);

SQLRETURN SQL_API _iodbcdm_SetPos (
    SQLHSTMT		  hstmt, 
    SQLSETPOSIROW	  irow, 
    SQLUSMALLINT	  fOption, 
    SQLUSMALLINT	  fLock);

SQLRETURN SQL_API _iodbcdm_NumResultCols (
    SQLHSTMT hstmt,
    SQLSMALLINT * pccol);

SQLRETURN SQLGetStmtOption_Internal (
  SQLHSTMT	hstmt, 
  SQLUSMALLINT	fOption, 
  SQLPOINTER	pvParam);
#endif
