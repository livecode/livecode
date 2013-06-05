/*
 *  proto.h
 *
 *  $Id: proto.h,v 1.10 2006/01/20 15:58:35 source Exp $
 *
 *  Trace functions prototypes
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


/* AllocConnect.c */
void trace_SQLAllocConnect (int trace_leave, int retcode, SQLHENV henv,
    SQLHDBC * phdbc);

/* AllocEnv.c */
void trace_SQLAllocEnv (int trace_leave, int retcode, SQLHENV * phenv);

/* AllocHandle.c */
void trace_SQLAllocHandle (int trace_leave, int retcode,
    SQLSMALLINT HandleType, SQLHANDLE InputHandle,
    SQLHANDLE * OutputHandlePtr);

/* AllocStmt.c */
void trace_SQLAllocStmt (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLHSTMT * phstmt);

/* BindCol.c */
void trace_SQLBindCol (int trace_leave, int retcode, SQLHSTMT StatementHandle,
    SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
    SQLPOINTER TargetValuePtr, SQLLEN BufferLength,
    SQLLEN * Strlen_or_IndPtr);

/* BindParameter.c */
void trace_SQLBindParameter (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber,
    SQLSMALLINT InputOutputType, SQLSMALLINT ValueType,
    SQLSMALLINT ParameterType, SQLUINTEGER ColumnSize,
    SQLSMALLINT DecimalDigits, SQLPOINTER ParameterValuePtr,
    SQLLEN BufferLength, SQLLEN * Strlen_or_IndPtr);

/* BrowseConnect.c */
void trace_SQLBrowseConnect (int trace_leave, int retcode,
    SQLHDBC ConnectionHandle, SQLCHAR * InConnectionString,
    SQLSMALLINT StringLength1, SQLCHAR * OutConnectionString,
    SQLSMALLINT BufferLength, SQLSMALLINT * StringLength2Ptr);
void trace_SQLBrowseConnectW (int trace_leave, int retcode,
    SQLHDBC ConnectionHandle, SQLWCHAR * InConnectionString,
    SQLSMALLINT StringLength1, SQLWCHAR * OutConnectionString,
    SQLSMALLINT BufferLength, SQLSMALLINT * StringLength2Ptr);

/* BulkOperations.c */
void trace_SQLBulkOperations (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLUSMALLINT Operation);

/* Cancel.c */
void trace_SQLCancel (int trace_leave, int retcode, SQLHSTMT StatementHandle);

/* CloseCursor.c */
void trace_SQLCloseCursor (int trace_leave, int retcode,
    SQLHSTMT StatementHandle);

/* ColAttribute.c */
void _trace_colattr2_type (SQLUSMALLINT type);
void _trace_colattr3_type (SQLUSMALLINT type);
void trace_SQLColAttribute (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttributePtr,
    SQLSMALLINT BufferLength, SQLSMALLINT * StringLengthPtr,
    SQLLEN * NumericAttributePtr);
void trace_SQLColAttributeW (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttributePtr,
    SQLSMALLINT BufferLength, SQLSMALLINT * StringLengthPtr,
    SQLLEN * NumericAttributePtr);
void trace_SQLColAttributes (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
    SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT * pcbDesc,
    SQLLEN * pfDesc);
void trace_SQLColAttributesW (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
    SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT * pcbDesc,
    SQLLEN * pfDesc);

/* ColumnPrivileges.c */
void trace_SQLColumnPrivileges (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLCHAR * szTableName,
    SQLSMALLINT cbTableName, SQLCHAR * szColumnName,
    SQLSMALLINT cbColumnName);
void trace_SQLColumnPrivilegesW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName, SQLWCHAR * szColumnName,
    SQLSMALLINT cbColumnName);

/* Columns.c */
void trace_SQLColumns (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLCHAR * szTableName,
    SQLSMALLINT cbTableName, SQLCHAR * szColumnName,
    SQLSMALLINT cbColumnName);
void trace_SQLColumnsW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName, SQLWCHAR * szColumnName,
    SQLSMALLINT cbColumnName);

/* Connect.c */
void trace_SQLConnect (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLCHAR * szDSN, SQLSMALLINT cbDSN, SQLCHAR * szUID, SQLSMALLINT cbUID,
    SQLCHAR * szAuthStr, SQLSMALLINT cbAuthStr);
void trace_SQLConnectW (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLWCHAR * szDSN, SQLSMALLINT cbDSN, SQLWCHAR * szUID, SQLSMALLINT cbUID,
    SQLWCHAR * szAuthStr, SQLSMALLINT cbAuthStr);

/* CopyDesc.c */
#if (ODBCVER >= 0x0300)
void trace_SQLCopyDesc (int trace_leave, int retcode,
    SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle);
#endif

/* DataSources.c */
void _trace_direction (SQLUSMALLINT dir);
void trace_SQLDataSources (int trace_leave, int retcode,
    SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR * ServerName,
    SQLSMALLINT BufferLength1, SQLSMALLINT * NameLength1Ptr,
    SQLCHAR * Description, SQLSMALLINT BufferLength2,
    SQLSMALLINT * NameLength2Ptr);
void trace_SQLDataSourcesW (int trace_leave, int retcode,
    SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLWCHAR * ServerName,
    SQLSMALLINT BufferLength1, SQLSMALLINT * NameLength1Ptr,
    SQLWCHAR * Description, SQLSMALLINT BufferLength2,
    SQLSMALLINT * NameLength2Ptr);

/* DescribeCol.c */
void _trace_desc_null (SQLSMALLINT * p, int output);
void trace_SQLDescribeCol (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLSMALLINT ColumnNumber, SQLCHAR * ColumnName,
    SQLSMALLINT BufferLength, SQLSMALLINT * NameLengthPtr,
    SQLSMALLINT * DataTypePtr, SQLULEN * ColumnSizePtr,
    SQLSMALLINT * DecimalDigitsPtr, SQLSMALLINT * NullablePtr);
void trace_SQLDescribeColW (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLSMALLINT ColumnNumber, SQLWCHAR * ColumnName,
    SQLSMALLINT BufferLength, SQLSMALLINT * NameLengthPtr,
    SQLSMALLINT * DataTypePtr, SQLULEN * ColumnSizePtr,
    SQLSMALLINT * DecimalDigitsPtr, SQLSMALLINT * NullablePtr);

/* DescribeParam.c */
void trace_SQLDescribeParam (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLUSMALLINT ipar, SQLSMALLINT * pfSqlType, SQLULEN * pcbColDef,
    SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable);

/* Disconnect.c */
void trace_SQLDisconnect (int trace_leave, int retcode,
    SQLHDBC ConnectionHandle);

/* DriverConnect.c */
void trace_SQLDriverConnect (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLHWND hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT * pcbConnStrOut, SQLUSMALLINT fDriverCompletion);
void trace_SQLDriverConnectW (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLHWND hwnd, SQLWCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLWCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT * pcbConnStrOut, SQLUSMALLINT fDriverCompletion);

/* Drivers.c */
void trace_SQLDrivers (int trace_leave, int retcode,
    SQLHENV EnvironmentHandle, SQLUSMALLINT Direction,
    SQLCHAR * DriverDescription, SQLSMALLINT BufferLength1,
    SQLSMALLINT * DescriptionLengthPtr, SQLCHAR * DriverAttributes,
    SQLSMALLINT BufferLength2, SQLSMALLINT * AttributesLengthPtr);
void trace_SQLDriversW (int trace_leave, int retcode,
    SQLHENV EnvironmentHandle, SQLUSMALLINT Direction,
    SQLWCHAR * DriverDescription, SQLSMALLINT BufferLength1,
    SQLSMALLINT * DescriptionLengthPtr, SQLWCHAR * DriverAttributes,
    SQLSMALLINT BufferLength2, SQLSMALLINT * AttributesLengthPtr);

/* EndTran.c */
void trace_SQLEndTran (int trace_leave, int retcode, SQLSMALLINT HandleType,
    SQLHANDLE Handle, SQLSMALLINT CompletionType);

/* Error.c */
void trace_SQLError (int trace_leave, int retcode, SQLHENV henv, SQLHDBC hdbc,
    SQLHSTMT hstmt, SQLCHAR * szSqlstate, SQLINTEGER * pfNativeError,
    SQLCHAR * szErrorMsg, SQLSMALLINT cbErrorMsgMax,
    SQLSMALLINT * pcbErrorMsg);
void trace_SQLErrorW (int trace_leave, int retcode, SQLHENV henv,
    SQLHDBC hdbc, SQLHSTMT hstmt, SQLWCHAR * szSqlstate,
    SQLINTEGER * pfNativeError, SQLWCHAR * szErrorMsg,
    SQLSMALLINT cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg);

/* ExecDirect.c */
void trace_SQLExecDirect (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr);
void trace_SQLExecDirectW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStr);

/* Execute.c */
void trace_SQLExecute (int trace_leave, int retcode, SQLHSTMT hstmt);

/* ExtendedFetch.c */
void _trace_fetchtype (SQLUSMALLINT type);
void trace_SQLExtendedFetch (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLUSMALLINT fFetchType, SQLLEN irow, SQLULEN * pcrow,
    SQLUSMALLINT * rgfRowStatus);

/* Fetch.c */
void trace_SQLFetch (int trace_leave, int retcode, SQLHSTMT hstmt);

/* FetchScroll.c */
void trace_SQLFetchScroll (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation,
    SQLLEN FetchOffset);

/* ForeignKeys.c */
void trace_SQLForeignKeys (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szPkTableQualifier, SQLSMALLINT cbPkTableQualifier,
    SQLCHAR * szPkTableOwner, SQLSMALLINT cbPkTableOwner,
    SQLCHAR * szPkTableName, SQLSMALLINT cbPkTableName,
    SQLCHAR * szFkTableQualifier, SQLSMALLINT cbFkTableQualifier,
    SQLCHAR * szFkTableOwner, SQLSMALLINT cbFkTableOwner,
    SQLCHAR * szFkTableName, SQLSMALLINT cbFkTableName);
void trace_SQLForeignKeysW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szPkTableQualifier, SQLSMALLINT cbPkTableQualifier,
    SQLWCHAR * szPkTableOwner, SQLSMALLINT cbPkTableOwner,
    SQLWCHAR * szPkTableName, SQLSMALLINT cbPkTableName,
    SQLWCHAR * szFkTableQualifier, SQLSMALLINT cbFkTableQualifier,
    SQLWCHAR * szFkTableOwner, SQLSMALLINT cbFkTableOwner,
    SQLWCHAR * szFkTableName, SQLSMALLINT cbFkTableName);

/* FreeConnect.c */
void trace_SQLFreeConnect (int trace_leave, int retcode, SQLHDBC hdbc);

/* FreeEnv.c */
void trace_SQLFreeEnv (int trace_leave, int retcode, SQLHENV henv);

/* FreeHandle.c */
void trace_SQLFreeHandle (int trace_leave, int retcode,
    SQLSMALLINT HandleType, SQLHANDLE Handle);

/* FreeStmt.c */
void _trace_freestmt_option (int option);
void trace_SQLFreeStmt (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLUSMALLINT Option);

/* GetConnectAttr.c */
void _trace_connattr_type (SQLINTEGER type);
void trace_SQLGetConnectAttr (int trace_leave, int retcode,
    SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength, SQLINTEGER * StringLengthPtr);
void trace_SQLGetConnectAttrW (int trace_leave, int retcode,
    SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength, SQLINTEGER * StringLengthPtr);

/* GetConnectOption.c */
void _trace_connopt_type (SQLUSMALLINT type);
void trace_SQLGetConnectOption (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLUSMALLINT fOption, SQLPOINTER pvParam);
void trace_SQLGetConnectOptionW (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLUSMALLINT fOption, SQLPOINTER pvParam);

/* GetCursorName.c */
void trace_SQLGetCursorName (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLCHAR * CursorName, SQLSMALLINT BufferLength,
    SQLSMALLINT * NameLengthPtr);
void trace_SQLGetCursorNameW (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLWCHAR * CursorName, SQLSMALLINT BufferLength,
    SQLSMALLINT * NameLengthPtr);

/* GetData.c */
void _trace_data (SQLSMALLINT fCType, SQLPOINTER rgbValue, SQLLEN cbValueMax,
    SQLLEN * pcbValue, int output);
void trace_SQLGetData (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLUSMALLINT icol, SQLSMALLINT fCType, SQLPOINTER rgbValue,
    SQLLEN cbValueMax, SQLLEN * pcbValue);

/* GetDescField.c */
#if (ODBCVER >= 0x0300)
void _trace_descfield_type (SQLSMALLINT type);
void trace_SQLGetDescField (int trace_leave, int retcode,
    SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
    SQLINTEGER * StringLengthPtr);
void trace_SQLGetDescFieldW (int trace_leave, int retcode,
    SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
    SQLINTEGER * StringLengthPtr);

/* GetDescRec.c */
void trace_SQLGetDescRec (int trace_leave, int retcode,
    SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLCHAR * Name,
    SQLSMALLINT BufferLength, SQLSMALLINT * StringLengthPtr,
    SQLSMALLINT * TypePtr, SQLSMALLINT * SubTypePtr, SQLLEN * LengthPtr,
    SQLSMALLINT * PrecisionPtr, SQLSMALLINT * ScalePtr,
    SQLSMALLINT * NullablePtr);
void trace_SQLGetDescRecW (int trace_leave, int retcode,
    SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLWCHAR * Name,
    SQLSMALLINT BufferLength, SQLSMALLINT * StringLengthPtr,
    SQLSMALLINT * TypePtr, SQLSMALLINT * SubTypePtr, SQLLEN * LengthPtr,
    SQLSMALLINT * PrecisionPtr, SQLSMALLINT * ScalePtr,
    SQLSMALLINT * NullablePtr);

/* GetDiagField.c */
void _trace_diag_type (SQLSMALLINT type);
void trace_SQLGetDiagField (int trace_leave, int retcode,
    SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
    SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfoPtr,
    SQLSMALLINT BufferLength, SQLSMALLINT * StringLengthPtr);
void trace_SQLGetDiagFieldW (int trace_leave, int retcode,
    SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
    SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfoPtr,
    SQLSMALLINT BufferLength, SQLSMALLINT * StringLengthPtr);

/* GetDiagRec.c */
void trace_SQLGetDiagRec (int trace_leave, int retcode,
    SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
    SQLCHAR * SqlState, SQLINTEGER * NativeErrorPtr, SQLCHAR * MessageText,
    SQLSMALLINT BufferLength, SQLSMALLINT * TextLengthPtr);
void trace_SQLGetDiagRecW (int trace_leave, int retcode,
    SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
    SQLWCHAR * SqlState, SQLINTEGER * NativeErrorPtr, SQLWCHAR * MessageText,
    SQLSMALLINT BufferLength, SQLSMALLINT * TextLengthPtr);
#endif

/* GetEnvAttr.c */
void _trace_envattr_type (SQLINTEGER type);
void trace_SQLGetEnvAttr (int trace_leave, int retcode,
    SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength, SQLINTEGER * StringLengthPtr);

/* GetFunctions.c */
void trace_SQLGetFunctions (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLUSMALLINT fFunc, SQLUSMALLINT * pfExists);

/* GetStmtAttr.c */
void _trace_stmtattr_type (SQLINTEGER type);
void trace_SQLGetStmtAttr (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength, SQLINTEGER * StringLengthPtr);
void trace_SQLGetStmtAttrW (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength, SQLINTEGER * StringLengthPtr);

/* GetStmtOption.c */
void _trace_stmtopt_type (SQLUSMALLINT type);
void trace_SQLGetStmtOption (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLUSMALLINT Attribute, SQLPOINTER ValuePtr);

/* GetTypeInfo.c */
void _trace_typeinfo (SQLSMALLINT type);
void trace_SQLGetTypeInfo (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLSMALLINT fSqlType);
void trace_SQLGetTypeInfoW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLSMALLINT fSqlType);

/* Info.c */
void trace_SQLGetInfo (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue,
    SQLSMALLINT cbInfoValueMax, SQLSMALLINT * pcbInfoValue);
void trace_SQLGetInfoW (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue,
    SQLSMALLINT cbInfoValueMax, SQLSMALLINT * pcbInfoValue);

/* MoreResults.c */
void trace_SQLMoreResults (int trace_leave, int retcode, SQLHSTMT hstmt);

/* NativeSql.c */
void trace_SQLNativeSql (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLCHAR * InStatementText, SQLINTEGER TextLength1,
    SQLCHAR * OutStatementText, SQLINTEGER BufferLength,
    SQLINTEGER * TextLength2Ptr);
void trace_SQLNativeSqlW (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLWCHAR * InStatementText, SQLINTEGER TextLength1,
    SQLWCHAR * OutStatementText, SQLINTEGER BufferLength,
    SQLINTEGER * TextLength2Ptr);

/* NumParams.c */
void trace_SQLNumParams (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLSMALLINT * pcpar);

/* NumResultCols.c */
void trace_SQLNumResultCols (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLSMALLINT * pccol);

/* ParamData.c */
void trace_SQLParamData (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLPOINTER * prgbValue);

/* ParamOptions.c */
void trace_SQLParamOptions (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLULEN crow, SQLULEN * pirow);

/* Prepare.c */
void trace_SQLPrepare (int trace_leave, int retcode, SQLHSTMT StatementHandle,
    SQLCHAR * StatementText, SQLINTEGER TextLength);
void trace_SQLPrepareW (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLWCHAR * StatementText,
    SQLINTEGER TextLength);

/* PrimaryKeys.c */
void trace_SQLPrimaryKeys (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLCHAR * szTableName,
    SQLSMALLINT cbTableName);
void trace_SQLPrimaryKeysW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName);

/* ProcedureColumns.c */
void trace_SQLProcedureColumns (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szProcQualifier, SQLSMALLINT cbProcQualifier,
    SQLCHAR * szProcOwner, SQLSMALLINT cbProcOwner, SQLCHAR * szProcName,
    SQLSMALLINT cbProcName, SQLCHAR * szColumnName, SQLSMALLINT cbColumnName);
void trace_SQLProcedureColumnsW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szProcQualifier, SQLSMALLINT cbProcQualifier,
    SQLWCHAR * szProcOwner, SQLSMALLINT cbProcOwner, SQLWCHAR * szProcName,
    SQLSMALLINT cbProcName, SQLWCHAR * szColumnName,
    SQLSMALLINT cbColumnName);

/* Procedures.c */
void trace_SQLProcedures (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szProcQualifier, SQLSMALLINT cbProcQualifier,
    SQLCHAR * szProcOwner, SQLSMALLINT cbProcOwner, SQLCHAR * szProcName,
    SQLSMALLINT cbProcName);
void trace_SQLProceduresW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szProcQualifier, SQLSMALLINT cbProcQualifier,
    SQLWCHAR * szProcOwner, SQLSMALLINT cbProcOwner, SQLWCHAR * szProcName,
    SQLSMALLINT cbProcName);

/* PutData.c */
void trace_SQLPutData (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLPOINTER rgbValue, SQLLEN cbValue);

/* RowCount.c */
void trace_SQLRowCount (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLLEN * pcrow);

/* SetConnectAttr.c */
void trace_SQLSetConnectAttr (int trace_leave, int retcode,
    SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER StringLength);
void trace_SQLSetConnectAttrW (int trace_leave, int retcode,
    SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER StringLength);

/* SetConnectOption.c */
void trace_SQLSetConnectOption (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLUSMALLINT fOption, SQLULEN vParam);
void trace_SQLSetConnectOptionW (int trace_leave, int retcode, SQLHDBC hdbc,
    SQLUSMALLINT fOption, SQLULEN vParam);

/* SetCursorName.c */
void trace_SQLSetCursorName (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szCursor, SQLSMALLINT cbCursor);
void trace_SQLSetCursorNameW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szCursor, SQLSMALLINT cbCursor);

/* SetDescField.c */
#if (ODBCVER >= 0x0300)
void trace_SQLSetDescField (int trace_leave, int retcode,
    SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier, SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength);
void trace_SQLSetDescFieldW (int trace_leave, int retcode,
    SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier, SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength);
#endif

/* SetDescRec.c */
#if (ODBCVER >= 0x0300)
void trace_SQLSetDescRec (int trace_leave, int retcode,
    SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT Type,
    SQLSMALLINT SubType, SQLLEN Length, SQLSMALLINT Precision,
    SQLSMALLINT Scale, SQLPOINTER Data, SQLLEN * StringLength,
    SQLLEN * Indicator);
#endif

/* SetEnvAttr.c */
void trace_SQLSetEnvAttr (int trace_leave, int retcode,
    SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER StringLength);

/* SetPos.c */
void _trace_setpos_irow (SQLSETPOSIROW i);
void _trace_setpos_oper (SQLUSMALLINT type);
void _trace_setpos_lock (SQLUSMALLINT type);
void trace_SQLSetPos (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock);

/* SetScrollOptions.c */
void _trace_scrollopt_type (SQLUSMALLINT type);
void trace_SQLSetScrollOptions (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLUSMALLINT fConcurrency, SQLINTEGER crowKeyset,
    SQLUSMALLINT crowRowset);

/* SetStmtAttr.c */
void trace_SQLSetStmtAttr (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER StringLength);
void trace_SQLSetStmtAttrW (int trace_leave, int retcode,
    SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
    SQLINTEGER StringLength);

/* SetStmtOption.c */
void trace_SQLSetStmtOption (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLUSMALLINT fOption, SQLUINTEGER vParam);

/* SpecialColumns.c */
void _trace_spcols_type (SQLUSMALLINT type);
void _trace_spcols_scope (SQLUSMALLINT type);
void _trace_spcols_null (SQLUSMALLINT type);
void trace_SQLSpecialColumns (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLUSMALLINT fColType, SQLCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier, SQLCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT cbTableName,
    SQLUSMALLINT fScope, SQLUSMALLINT fNullable);
void trace_SQLSpecialColumnsW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLUSMALLINT fColType, SQLWCHAR * szTableQualifier,
    SQLSMALLINT cbTableQualifier, SQLWCHAR * szTableOwner,
    SQLSMALLINT cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT cbTableName,
    SQLUSMALLINT fScope, SQLUSMALLINT fNullable);

/* Statistics.c */
void _trace_stats_unique (SQLUSMALLINT type);
void _trace_stats_accuracy (SQLUSMALLINT type);
void trace_SQLStatistics (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLCHAR * szTableName,
    SQLSMALLINT cbTableName, SQLUSMALLINT fUnique, SQLUSMALLINT fAccuracy);
void trace_SQLStatisticsW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName, SQLUSMALLINT fUnique, SQLUSMALLINT fAccuracy);

/* TablePrivileges.c */
void trace_SQLTablePrivileges (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLCHAR * szTableName,
    SQLSMALLINT cbTableName);
void trace_SQLTablePrivilegesW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName);

/* Tables.c */
void trace_SQLTables (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLCHAR * szTableName,
    SQLSMALLINT cbTableName, SQLCHAR * szTableType, SQLSMALLINT cbTableType);
void trace_SQLTablesW (int trace_leave, int retcode, SQLHSTMT hstmt,
    SQLWCHAR * szTableQualifier, SQLSMALLINT cbTableQualifier,
    SQLWCHAR * szTableOwner, SQLSMALLINT cbTableOwner, SQLWCHAR * szTableName,
    SQLSMALLINT cbTableName, SQLWCHAR * szTableType, SQLSMALLINT cbTableType);

/* trace.c */
void trace_set_appname (char *appname);
char *trace_get_filename (void);
void trace_set_filename (char *fname);
void trace_start (void);
void trace_stop (void);
void trace_emitc (char c);
void trace_emit (char *fmt, ...);
void trace_emit_string (SQLCHAR * str, int len, int is_utf8);
void trace_emit_binary (unsigned char *str, int len);
void _trace_print_function (int func, int trace_leave, int retcode);
void _trace_handletype (SQLSMALLINT type);
void _trace_handle_p (SQLSMALLINT type, SQLHANDLE * handle, int output);
void _trace_handle (SQLSMALLINT type, SQLHANDLE handle);
void _trace_smallint (SQLSMALLINT i);
void _trace_usmallint (SQLUSMALLINT i);
void _trace_integer (SQLINTEGER i);
void _trace_uinteger (SQLUINTEGER i);
void _trace_pointer (SQLPOINTER p);
void _trace_smallint_p (SQLSMALLINT * p, int output);
void _trace_usmallint_p (SQLUSMALLINT * p, int output);
void _trace_integer_p (SQLINTEGER * p, int output);
void _trace_uinteger_p (SQLUINTEGER * p, int output);
void _trace_stringlen (char *type, SQLINTEGER len);
void _trace_len (SQLLEN i);
void _trace_ulen (SQLULEN i);
void _trace_len_p (SQLLEN * p, int output);
void _trace_ulen_p (SQLULEN * p, int output);
void _trace_string (SQLCHAR * str, SQLSMALLINT len, SQLSMALLINT * lenptr,
    int output);
void _trace_string_w (SQLWCHAR * str, SQLSMALLINT len, SQLSMALLINT * lenptr,
    int output);
void _trace_c_type (SQLSMALLINT type);
void _trace_inouttype (SQLSMALLINT type);
void _trace_sql_type (SQLSMALLINT type);
void _trace_sql_type_p (SQLSMALLINT * p, int output);
void _trace_sql_subtype (SQLSMALLINT * type, SQLSMALLINT * sub, int output);
void _trace_bufferlen (SQLINTEGER length);

/* Transact.c */
void _trace_tran_completion (SQLSMALLINT option);
void trace_SQLTransact (int trace_leave, int retcode,
    SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle,
    SQLSMALLINT CompletionType);
