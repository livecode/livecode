/*
 *  GetDescField.c
 *
 *  $Id: GetDescField.c,v 1.5 2006/01/20 15:58:35 source Exp $
 *
 *  SQLGetDescField trace functions
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


#if ODBCVER >= 0x0300
void 
_trace_descfield_type (SQLSMALLINT type)
{
  char *ptr = "unknown field identifier";

  switch (type)
    {
      _S (SQL_DESC_ALLOC_TYPE);
      _S (SQL_DESC_ARRAY_SIZE);
      _S (SQL_DESC_ARRAY_STATUS_PTR);
      _S (SQL_DESC_AUTO_UNIQUE_VALUE);
      _S (SQL_DESC_BASE_COLUMN_NAME);
      _S (SQL_DESC_BASE_TABLE_NAME);
      _S (SQL_DESC_BIND_OFFSET_PTR);
      _S (SQL_DESC_BIND_TYPE);
      _S (SQL_DESC_CASE_SENSITIVE);
      _S (SQL_DESC_CATALOG_NAME);
      _S (SQL_DESC_CONCISE_TYPE);
      _S (SQL_DESC_COUNT);
      _S (SQL_DESC_DATA_PTR);
      _S (SQL_DESC_DATETIME_INTERVAL_CODE);
      _S (SQL_DESC_DATETIME_INTERVAL_PRECISION);
      _S (SQL_DESC_DISPLAY_SIZE);
      _S (SQL_DESC_FIXED_PREC_SCALE);
      _S (SQL_DESC_INDICATOR_PTR);
      _S (SQL_DESC_LABEL);
      _S (SQL_DESC_LENGTH);
      _S (SQL_DESC_LITERAL_PREFIX);
      _S (SQL_DESC_LITERAL_SUFFIX);
      _S (SQL_DESC_LOCAL_TYPE_NAME);
      _S (SQL_DESC_MAXIMUM_SCALE);
      _S (SQL_DESC_MINIMUM_SCALE);
      _S (SQL_DESC_NAME);
      _S (SQL_DESC_NULLABLE);
      _S (SQL_DESC_NUM_PREC_RADIX);
      _S (SQL_DESC_OCTET_LENGTH);
      _S (SQL_DESC_OCTET_LENGTH_PTR);
      _S (SQL_DESC_PARAMETER_TYPE);
      _S (SQL_DESC_PRECISION);
      _S (SQL_DESC_ROWS_PROCESSED_PTR);
      _S (SQL_DESC_SCALE);
      _S (SQL_DESC_SCHEMA_NAME);
      _S (SQL_DESC_SEARCHABLE);
      _S (SQL_DESC_TABLE_NAME);
      _S (SQL_DESC_TYPE);
      _S (SQL_DESC_TYPE_NAME);
      _S (SQL_DESC_UNNAMED);
      _S (SQL_DESC_UNSIGNED);
      _S (SQL_DESC_UPDATABLE);

#if (ODBCVER >= 0x0350)
      _S (SQL_DESC_ROWVER);
#endif
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLUSMALLINT ", (int) type, ptr);
}


void
trace_SQLGetDescField (int trace_leave, int retcode,
  SQLHDESC		  DescriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  /* Trace function */
  _trace_print_function (en_GetDescField, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_DESC, DescriptorHandle);
  _trace_smallint (RecNumber);
  _trace_descfield_type (FieldIdentifier);
  _trace_pointer (ValuePtr);
  _trace_bufferlen (BufferLength);
  _trace_integer_p (StringLengthPtr, TRACE_OUTPUT_SUCCESS);
}


void
trace_SQLGetDescFieldW (int trace_leave, int retcode,
  SQLHDESC		  DescriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  /* Trace function */
  _trace_print_function (en_GetDescFieldW, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_DESC, DescriptorHandle);
  _trace_smallint (RecNumber);
  _trace_descfield_type (FieldIdentifier);
  _trace_pointer (ValuePtr);
  _trace_bufferlen (BufferLength);
  _trace_integer_p (StringLengthPtr, TRACE_OUTPUT_SUCCESS);
}
#endif
