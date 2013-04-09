/*
 *  ColAttribute.c
 *
 *  $Id: ColAttribute.c,v 1.7 2006/01/20 15:58:35 source Exp $
 *
 *  SQLColAttribute trace functions
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
_trace_colattr2_type (SQLUSMALLINT type)
{
  char *ptr = "unknown option";

  switch (type)
    {
      _S (SQL_COLUMN_COUNT);
      _S (SQL_COLUMN_NAME);
      _S (SQL_COLUMN_TYPE);
      _S (SQL_COLUMN_LENGTH);
      _S (SQL_COLUMN_PRECISION);
      _S (SQL_COLUMN_SCALE);
      _S (SQL_COLUMN_DISPLAY_SIZE);
      _S (SQL_COLUMN_NULLABLE);
      _S (SQL_COLUMN_UNSIGNED);
      _S (SQL_COLUMN_MONEY);
      _S (SQL_COLUMN_UPDATABLE);
      _S (SQL_COLUMN_AUTO_INCREMENT);
      _S (SQL_COLUMN_CASE_SENSITIVE);
      _S (SQL_COLUMN_SEARCHABLE);
      _S (SQL_COLUMN_TYPE_NAME);
      _S (SQL_COLUMN_TABLE_NAME);
      _S (SQL_COLUMN_OWNER_NAME);
      _S (SQL_COLUMN_QUALIFIER_NAME);
      _S (SQL_COLUMN_LABEL);
    }

  trace_emit ("\t\t%-15.15s   %ld (%s)\n", "SQLUSMALLINT ", (int) type, ptr);
}


#if ODBCVER >= 0x0300
void
_trace_colattr3_type (SQLUSMALLINT type)
{
  char *ptr = "unknown option";

  switch (type)
    {
      _S (SQL_DESC_AUTO_UNIQUE_VALUE);
      _S (SQL_DESC_BASE_COLUMN_NAME);
      _S (SQL_DESC_BASE_TABLE_NAME);
      _S (SQL_DESC_CASE_SENSITIVE);
      _S (SQL_DESC_CATALOG_NAME);
      _S (SQL_DESC_CONCISE_TYPE);
      _S (SQL_DESC_COUNT);
      _S (SQL_DESC_DISPLAY_SIZE);
      _S (SQL_DESC_FIXED_PREC_SCALE);
      _S (SQL_DESC_LABEL);
      _S (SQL_DESC_LENGTH);
      _S (SQL_DESC_LITERAL_PREFIX);
      _S (SQL_DESC_LITERAL_SUFFIX);
      _S (SQL_DESC_LOCAL_TYPE_NAME);
      _S (SQL_DESC_NAME);
      _S (SQL_DESC_NULLABLE);
      _S (SQL_DESC_NUM_PREC_RADIX);
      _S (SQL_DESC_OCTET_LENGTH);
      _S (SQL_DESC_PRECISION);
      _S (SQL_DESC_SCALE);
      _S (SQL_DESC_SCHEMA_NAME);
      _S (SQL_DESC_SEARCHABLE);
      _S (SQL_DESC_TABLE_NAME);
      _S (SQL_DESC_TYPE);
      _S (SQL_DESC_TYPE_NAME);
      _S (SQL_DESC_UNNAMED);
      _S (SQL_DESC_UNSIGNED);
      _S (SQL_DESC_UPDATABLE);
    }

  trace_emit ("\t\t%-15.15s   %d (%s)\n", "SQLUSMALLINT ", (int) type, ptr);
}
#endif


#if ODBCVER >= 0x0300
static void
_trace_colattr3_data (
  SQLUSMALLINT		  FieldIdentifier,
  SQLPOINTER		  CharacterAttributePtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLLEN		* NumericAttributePtr,
  int			  w_mode,
  int			  output)
{
  int type = 0;

  switch (FieldIdentifier)
    {
    case SQL_DESC_BASE_COLUMN_NAME:
    case SQL_DESC_BASE_TABLE_NAME:
    case SQL_DESC_CATALOG_NAME:
    case SQL_DESC_LABEL:
    case SQL_DESC_LITERAL_PREFIX:
    case SQL_DESC_LITERAL_SUFFIX:
    case SQL_DESC_LOCAL_TYPE_NAME:
    case SQL_DESC_NAME:
    case SQL_DESC_SCHEMA_NAME:
    case SQL_DESC_TYPE_NAME:
    case SQL_DESC_TABLE_NAME:
      type = 1;			/* string */
      break;

    case SQL_DESC_CONCISE_TYPE:
    case SQL_DESC_COUNT:
    case SQL_DESC_DISPLAY_SIZE:
    case SQL_DESC_LENGTH:
    case SQL_DESC_NUM_PREC_RADIX:
    case SQL_DESC_OCTET_LENGTH:
    case SQL_DESC_PRECISION:
    case SQL_DESC_SCALE:
    case SQL_DESC_TYPE:
      type = 2;			/* integer */
      break;

    case SQL_DESC_AUTO_UNIQUE_VALUE:
    case SQL_DESC_CASE_SENSITIVE:
    case SQL_DESC_FIXED_PREC_SCALE:
    case SQL_DESC_UNSIGNED:
      type = 3;			/* boolean */
      break;

    case SQL_DESC_NULLABLE:
      type = 4;
      break;

    case SQL_DESC_SEARCHABLE:
      type = 5;
      break;

    case SQL_DESC_UNNAMED:
      type = 6;
      break;

    case SQL_DESC_UPDATABLE:
      type = 7;
      break;

    default:
      type = 0;			/* unknown */
      break;
    }

  /*
   *  If we cannot determine the type or we are entering, just
   *  emit the pointers
   */
  if (type == 0 || !output)
    {
      _trace_pointer (CharacterAttributePtr);
      _trace_bufferlen ((SQLINTEGER) BufferLength);
      _trace_smallint_p (StringLengthPtr, output);
      _trace_len_p (NumericAttributePtr, output);
      return;
    }

  if (type == 1)		/* string */
    {
      if (w_mode)
	_trace_string_w (CharacterAttributePtr, BufferLength, StringLengthPtr,
	    output);
      else
	_trace_string (CharacterAttributePtr, BufferLength, StringLengthPtr,
	    output);
      _trace_bufferlen ((SQLINTEGER) BufferLength);
      _trace_smallint_p (StringLengthPtr, output);
      _trace_len_p (NumericAttributePtr, 0);	/* just display pointer */
    }
  else				/* integer */
    {
      _trace_pointer (CharacterAttributePtr);
      _trace_bufferlen ((SQLINTEGER) BufferLength);
      _trace_smallint_p (StringLengthPtr, 0);	/* just display pointer */

      if (!NumericAttributePtr)	/* null pointer */
	trace_emit ("\t\t%-15.15s * 0x0\n", "SQLLEN");
      else if (type == 3)	/* boolean */
	{
	  trace_emit ("\t\t%-15.15s * %p (%s)\n", "SQLLEN",
	      NumericAttributePtr,
	      *(SQLLEN *) NumericAttributePtr ? "SQL_TRUE" : "SQL_FALSE");
	}
      else if (type == 4)	/* nullable */
	{
	  char *ptr = "unknown nullable type";

	  switch (*NumericAttributePtr)
	    {
	      _S (SQL_NULLABLE);
	      _S (SQL_NULLABLE_UNKNOWN);
	      _S (SQL_NO_NULLS);
	    }
	  trace_emit ("\t\t%-15.15s * %p (%s)\n", "SQLLEN",
	      NumericAttributePtr, ptr);
	}
      else if (type == 5)	/* searchable */
	{
	  char *ptr = "unknown searchable type";

	  switch (*NumericAttributePtr)
	    {
	      _S (SQL_PRED_NONE);
	      _S (SQL_PRED_CHAR);
	      _S (SQL_PRED_BASIC);
	      _S (SQL_PRED_SEARCHABLE);
	    }
	  trace_emit ("\t\t%-15.15s * %p (%s)\n", "SQLLEN",
	      NumericAttributePtr, ptr);
	}
      else if (type == 6)	/* named */
	{
	  char *ptr = "unknown named type";

	  switch (*NumericAttributePtr)
	    {
	      _S (SQL_NAMED);
	      _S (SQL_UNNAMED);
	    }
	  trace_emit ("\t\t%-15.15s * %p (%s)\n", "SQLLEN",
	      NumericAttributePtr, ptr);
	}
      else if (type == 7)	/* updatable */
	{
	  char *ptr = "unknown ubdatable type";

	  switch (*NumericAttributePtr)
	    {
	      _S (SQL_ATTR_READONLY);
	      _S (SQL_ATTR_WRITE);
	      _S (SQL_ATTR_READWRITE_UNKNOWN);
	   
	    }
	  trace_emit ("\t\t%-15.15s * %p (%s)\n", "SQLLEN",
	      NumericAttributePtr, ptr);
	}
      else			/* just a value */
	{
	  _trace_len_p (NumericAttributePtr, output);
	}
    }

  return;
}
#endif


#if ODBCVER >= 0x0300
void
trace_SQLColAttribute (int trace_leave, int retcode,
  SQLHSTMT		  StatementHandle,
  SQLUSMALLINT		  ColumnNumber,
  SQLUSMALLINT		  FieldIdentifier,
  SQLPOINTER		  CharacterAttributePtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLLEN		* NumericAttributePtr
  )
{
  /* Trace function */
  _trace_print_function (en_ColAttribute, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_STMT, StatementHandle);
  _trace_usmallint (ColumnNumber);
  _trace_colattr3_type (FieldIdentifier);

  _trace_colattr3_data (FieldIdentifier, CharacterAttributePtr, BufferLength,
      StringLengthPtr, NumericAttributePtr, 0, TRACE_OUTPUT_SUCCESS);
}
#endif


#if ODBCVER >= 0x0300
void
trace_SQLColAttributeW (int trace_leave, int retcode,
  SQLHSTMT		  StatementHandle,
  SQLUSMALLINT		  ColumnNumber,
  SQLUSMALLINT		  FieldIdentifier,
  SQLPOINTER		  CharacterAttributePtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLLEN		* NumericAttributePtr
  )
{
  /* Trace function */
  _trace_print_function (en_ColAttributeW, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_STMT, StatementHandle);
  _trace_usmallint (ColumnNumber);
  _trace_colattr3_type (FieldIdentifier);

  _trace_colattr3_data (FieldIdentifier, CharacterAttributePtr, BufferLength,
      StringLengthPtr, NumericAttributePtr, 1, TRACE_OUTPUT_SUCCESS);
}
#endif


void
trace_SQLColAttributes (int trace_leave, int retcode,
  SQLHSTMT		  StatementHandle,
  SQLUSMALLINT		  icol,
  SQLUSMALLINT		  fDescType,
  SQLPOINTER		  rgbDesc,
  SQLSMALLINT		  cbDescMax,
  SQLSMALLINT		* pcbDesc,
  SQLLEN		* pfDesc)
{
  /* Trace function */
  _trace_print_function (en_ColAttributes, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_STMT, StatementHandle);
  _trace_usmallint (icol);
  _trace_colattr2_type (fDescType);
  _trace_pointer (rgbDesc);		/* TODO */
  _trace_smallint (cbDescMax);
  _trace_smallint_p (pcbDesc, TRACE_OUTPUT_SUCCESS);
  _trace_len_p (pfDesc, TRACE_OUTPUT_SUCCESS);
}


#if ODBCVER >= 0x0300
void
trace_SQLColAttributesW (int trace_leave, int retcode,
  SQLHSTMT		  StatementHandle,
  SQLUSMALLINT		  icol,
  SQLUSMALLINT		  fDescType,
  SQLPOINTER		  rgbDesc,
  SQLSMALLINT		  cbDescMax,
  SQLSMALLINT		* pcbDesc,
  SQLLEN		* pfDesc)
{
  /* Trace function */
  _trace_print_function (en_ColAttributeW, trace_leave, retcode);

  /* Trace Arguments */
  _trace_handle (SQL_HANDLE_STMT, StatementHandle);
  _trace_usmallint (icol);
  _trace_colattr2_type (fDescType);
  _trace_pointer (rgbDesc);		/* TODO */
  _trace_smallint (cbDescMax);
  _trace_smallint_p (pcbDesc, TRACE_OUTPUT_SUCCESS);
  _trace_len_p (pfDesc, TRACE_OUTPUT_SUCCESS);
}
#endif
