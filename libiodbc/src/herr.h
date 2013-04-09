/*
 *  herr.h
 *
 *  $Id: herr.h,v 1.11 2006/01/20 15:58:34 source Exp $
 *
 *  Error stack management functions
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

#ifndef	_HERR_H
#define	_HERR_H

typedef enum
  {
    en_00000 = 0,
    en_01000,
    en_01002,
    en_01004,
    en_01006,
    en_01S00,
    en_01S01,
    en_01S02,
    en_01S03,
    en_01S04,
    en_07001,
    en_07006,
    en_08001,
    en_08002,
    en_08003,
    en_08004,
    en_08007,
    en_08S01,
    en_21S01,
    en_21S02,
    en_22001,
    en_22003,
    en_22005,
    en_22008,
    en_22012,
    en_22026,
    en_23000,
    en_24000,
    en_25000,
    en_28000,
    en_34000,
    en_37000,
    en_3C000,
    en_40001,
    en_42000,
    en_70100,
    en_HY001,
    en_HY009,
    en_HY010,
    en_HY017,
    en_HY024,
    en_HY091,
    en_HY092,
    en_HYC00,
    en_IM001,
    en_IM002,
    en_IM003,
    en_IM004,
    en_IM005,
    en_IM006,
    en_IM007,
    en_IM008,
    en_IM009,
    en_IM010,
    en_IM011,
    en_IM012,
    en_IM013,
    en_IM014,
    en_IM015,
    en_S0001,
    en_S0002,
    en_S0011,
    en_S0012,
    en_S0021,
    en_S0022,
    en_S0023,
    en_S1000,
    en_S1001,
    en_S1002,
    en_S1003,
    en_S1004,
    en_S1008,
    en_S1009,
    en_S1010,
    en_S1011,
    en_S1012,
    en_S1015,
    en_S1090,
    en_S1091,
    en_S1092,
    en_S1093,
    en_S1094,
    en_S1095,
    en_S1096,
    en_S1097,
    en_S1098,
    en_S1099,
    en_S1100,
    en_S1101,
    en_S1103,
    en_S1104,
    en_S1105,
    en_S1106,
    en_S1107,
    en_S1108,
    en_S1109,
    en_S1110,
    en_S1111,
    en_S1C00,
    en_S1T00,
    en_sqlstat_total
  }
sqlstcode_t;

typedef void *HERR;
#define SQL_NULL_HERR	((HERR)NULL)

typedef struct
  {
    sqlstcode_t code;
    char *stat;
    char *msg;
  }
sqlerrmsg_t;

typedef struct sqlerr
  {
    sqlstcode_t code;
    int idx;
    char *msg;
    struct sqlerr *next;
  }
sqlerr_t;

extern void _iodbcdm_freesqlerrlist (HERR herr);
extern HERR _iodbcdm_pushsqlerr (HERR list, sqlstcode_t code, void *sysmsg);

#define	PUSHSYSERR(list, msg)	\
	list = (HERR) _iodbcdm_pushsqlerr ((HERR)(list), en_00000, msg)

#define	PUSHSQLERR(list, code)	\
	list = (HERR) _iodbcdm_pushsqlerr ((HERR)(list), (code), NULL)

#define CLEAR_ERRORS(_handle) \
    { \
	_iodbcdm_freesqlerrlist ((_handle)->herr); \
	 (_handle)->herr = SQL_NULL_HERR; \
	 (_handle)->rc = SQL_SUCCESS; \
         (_handle)->err_rec = 0; \
    }
	
#endif /* _SQLERR_H */
