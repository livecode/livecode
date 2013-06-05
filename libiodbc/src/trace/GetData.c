/*
 *  GetData.c
 *
 *  $Id: GetData.c,v 1.7 2006/01/20 15:58:35 source Exp $
 *
 *  SQLGetData trace functions
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

#include "trace.h"


void
_trace_data (
  SQLSMALLINT		  fCType,
  SQLPOINTER		  rgbValue,
  SQLLEN		  cbValueMax,
  SQLLEN	    	* pcbValue,
  int			  output)
{
  char buf[1024];		/* Temp buffer */

  if (!rgbValue)
    {
      trace_emit ("\t\t%-15.15s   0x0\n", "SQLPOINTER");
      return;
    }

  trace_emit ("\t\t%-15.15s   %p\n", "SQLPOINTER", rgbValue);

  if (!output)
    return;			/* Only print buffer content on leave */

  switch (fCType)
    {
    case SQL_C_BINARY:
      {
        int len;
	if (pcbValue && cbValueMax > 0)
	  len = *((SQLINTEGER *) pcbValue);
	else
	  len = cbValueMax;
	trace_emit_binary ((unsigned char *) rgbValue, len);
      }
      break;

    case SQL_C_BIT:
      {
	int i = (int) *(char *) rgbValue;
	sprintf (buf, "%d", i > 0 ? 1 : 0);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_CHAR:
      {
        int len;
        if (pcbValue && cbValueMax > 0)
 	  len =  *((SQLINTEGER *) pcbValue);
        else
	  len = cbValueMax;
	trace_emit_string ((SQLCHAR *) rgbValue, len, 0);
      }
      break;

    case SQL_C_DATE:
#if ODBCVER >= 0x0300
    case SQL_C_TYPE_DATE:
#endif
      {
	DATE_STRUCT *d = (DATE_STRUCT *) rgbValue;
	sprintf (buf, "%04d-%02d-%02d", d->year, d->month, d->day);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_DEFAULT:
      /*
       *  Not enough information to dump the content of the buffer
       */
      return;

    case SQL_C_DOUBLE:
      {
	double d = *(double *) rgbValue;
	sprintf (buf, "%f", d);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_FLOAT:
      {
	float f = *(float *) rgbValue;
	sprintf (buf, "%f", f);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

#if (ODBCVER >= 0x0350)
    case SQL_C_GUID:
      {
	SQLGUID *g = (SQLGUID *) rgbValue;
	sprintf (buf,
	    "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
	    (unsigned long) g->Data1,
	    g->Data2, g->Data3,
	    g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
            g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;
#endif

#if ODBCVER >= 0x0300
    case SQL_C_INTERVAL_DAY:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu days",
	    (unsigned long) i->intval.day_second.day);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_DAY_TO_HOUR:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu days %lu hours",
	    (unsigned long) i->intval.day_second.day,
	    (unsigned long) i->intval.day_second.hour);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_DAY_TO_MINUTE:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu days %lu hours %lu minutes",
	    (unsigned long) i->intval.day_second.day,
	    (unsigned long) i->intval.day_second.hour,
	    (unsigned long) i->intval.day_second.minute);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_DAY_TO_SECOND:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu days %lu hours %lu minutes %lu seconds",
	    (unsigned long) i->intval.day_second.day,
	    (unsigned long) i->intval.day_second.hour,
	    (unsigned long) i->intval.day_second.minute,
	    (unsigned long) i->intval.day_second.second);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_HOUR:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu hours",
	    (unsigned long) i->intval.day_second.hour);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu hours %lu minutes",
	    (unsigned long) i->intval.day_second.hour,
	    (unsigned long) i->intval.day_second.minute);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_HOUR_TO_SECOND:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu hours %lu minutes %lu seconds",
	    (unsigned long) i->intval.day_second.hour,
	    (unsigned long) i->intval.day_second.minute,
	    (unsigned long) i->intval.day_second.second);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_MINUTE:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu minutes",
	    (unsigned long) i->intval.day_second.minute);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_MINUTE_TO_SECOND:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu minutes %lu seconds",
	    (unsigned long) i->intval.day_second.minute,
	    (unsigned long) i->intval.day_second.second);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_MONTH:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu months",
	    (unsigned long) i->intval.year_month.month);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_SECOND:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu seconds",
	    (unsigned long) i->intval.day_second.second);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_YEAR:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu years",
	    (unsigned long) i->intval.year_month.year);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_INTERVAL_YEAR_TO_MONTH:
      {
	SQL_INTERVAL_STRUCT *i = (SQL_INTERVAL_STRUCT *) rgbValue;
	sprintf (buf, "%lu years %lu months",
	    (unsigned long) i->intval.year_month.year,
	    (unsigned long) i->intval.year_month.month);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;
#endif

    case SQL_C_LONG:
    case SQL_C_SLONG:
      {
	long l = *(long *) rgbValue;
	sprintf (buf, "%ld", l);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_ULONG:
      {
	unsigned long l = *(unsigned long *) rgbValue;
	sprintf (buf, "%lu", l);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;


#if ODBCVER >= 0x0300
    case SQL_C_NUMERIC:
      /* NOT YET */
      break;
#endif

#if ODBCVER >= 0x0300
    case SQL_C_SBIGINT:
#if defined (ODBCINT64)
      {
	ODBCINT64 l = *(ODBCINT64 *) rgbValue;
	sprintf (buf, "%lld", l);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
#endif
      break;

    case SQL_C_UBIGINT:
#if defined (ODBCINT64)
      {
	unsigned ODBCINT64 l = *(unsigned ODBCINT64 *) rgbValue;
	sprintf (buf, "%llu", l);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
#endif
      break;
#endif

    case SQL_C_SHORT:
    case SQL_C_SSHORT:
      {
	int i = (int) *(short *) rgbValue;
	sprintf (buf, "%d", i);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_USHORT:
      {
	unsigned int i = (unsigned int) *(unsigned short *) rgbValue;
	sprintf (buf, "%u", i);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_TIME:
#if ODBCVER >= 0x0300
    case SQL_C_TYPE_TIME:
#endif
      {
	TIME_STRUCT *t = (TIME_STRUCT *) rgbValue;
	sprintf (buf, "%02d:%02d:%02d", t->hour, t->minute, t->second);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_TIMESTAMP:
#if ODBCVER >= 0x0300
    case SQL_C_TYPE_TIMESTAMP:
#endif
      {
	TIMESTAMP_STRUCT *t = (TIMESTAMP_STRUCT *) rgbValue;
	sprintf (buf, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
	    t->year, t->month, t->day,
	    t->hour, t->minute, t->second, 
	    (long) t->fraction);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
      {
	int i = (int) *(char *) rgbValue;
	sprintf (buf, "%d", i);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_UTINYINT:
      {
	unsigned int i = (unsigned int) *(unsigned char *) rgbValue;
	sprintf (buf, "%u", i);
	trace_emit_string ((SQLCHAR *) buf, SQL_NTS, 0);
      }
      break;

    case SQL_C_WCHAR:
      {
	SQLCHAR *wstr;
        int len;
	if (pcbValue && cbValueMax > 0)
	  len = *((SQLINTEGER *) pcbValue);
	else
	  len = cbValueMax;
	wstr = dm_SQL_W2A ((wchar_t *) rgbValue, len);
	trace_emit_string (wstr, SQL_NTS, 1);
	free (wstr);
      }
      break;

    default:
      /*
       *  Unhandled/Unknown datatype
       */
      break;
    }

  return;
}


void
trace_SQLGetData (int trace_leave, int retcode,
  SQLHSTMT		  hstmt,
  SQLUSMALLINT		  icol,
  SQLSMALLINT		  fCType,
  SQLPOINTER		  rgbValue,
  SQLLEN		  cbValueMax,
  SQLLEN	    	* pcbValue)
{
  /* Trace function */
  _trace_print_function (en_GetData, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_STMT, hstmt);
  _trace_usmallint (icol);
  _trace_c_type (fCType);
  _trace_data (fCType, rgbValue, cbValueMax, pcbValue, TRACE_OUTPUT_SUCCESS);
  _trace_len (cbValueMax);
  _trace_len_p (pcbValue, TRACE_OUTPUT_SUCCESS);
}
