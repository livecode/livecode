NAME=libiodbc
TYPE=archive

include ../../rules/environment.linux.makefile

SOURCES= \
	catalog.c connect.c dlproc.c execute.c fetch.c hdbc.c henv.c herr.c \
	hstmt.c info.c main.c misc.c odbc3.c prepare.c result.c trace/AllocConnect.c \
	trace/AllocEnv.c trace/AllocHandle.c trace/AllocStmt.c trace/BindCol.c \
	trace/BindParameter.c trace/BrowseConnect.c trace/BulkOperations.c \
	trace/Cancel.c trace/CloseCursor.c trace/ColAttribute.c trace/ColumnPrivileges.c \
	trace/Columns.c trace/Connect.c trace/CopyDesc.c trace/DataSources.c \
	trace/DescribeCol.c trace/DescribeParam.c trace/Disconnect.c trace/DriverConnect.c \
	trace/Drivers.c trace/EndTran.c trace/Error.c trace/ExecDirect.c trace/Execute.c \
	trace/ExtendedFetch.c trace/Fetch.c trace/FetchScroll.c trace/ForeignKeys.c \
	trace/FreeConnect.c trace/FreeEnv.c trace/FreeHandle.c trace/FreeStmt.c \
	trace/GetConnectAttr.c trace/GetConnectOption.c trace/GetCursorName.c \
	trace/GetData.c trace/GetDescField.c trace/GetDescRec.c trace/GetDiagField.c \
	trace/GetDiagRec.c trace/GetEnvAttr.c trace/GetFunctions.c trace/GetStmtAttr.c \
	trace/GetStmtOption.c trace/GetTypeInfo.c trace/Info.c trace/MoreResults.c \
	trace/NativeSql.c trace/NumParams.c trace/NumResultCols.c trace/ParamData.c \
	trace/ParamOptions.c trace/Prepare.c trace/PrimaryKeys.c trace/ProcedureColumns.c \
	trace/Procedures.c trace/PutData.c trace/RowCount.c trace/SetConnectAttr.c \
	trace/SetConnectOption.c trace/SetCursorName.c trace/SetDescField.c \
	trace/SetDescRec.c trace/SetEnvAttr.c trace/SetPos.c trace/SetScrollOptions.c \
	trace/SetStmtAttr.c trace/SetStmtOption.c trace/SpecialColumns.c trace/Statistics.c \
	trace/TablePrivileges.c trace/Tables.c trace/Transact.c trace/trace.c \
	inst/Info.c inst/SQLPostInstallerError.c inst/SQLConfigDataSource.c inst/SQLReadFileDSN.c \
	inst/SQLConfigDriver.c inst/SQLRemoveDSNFromIni.c inst/SQLCreateDataSource.c \
	inst/SQLRemoveDefaultDataSource.c inst/SQLGetAvailableDrivers.c inst/SQLRemoveDriver.c \
	inst/SQLGetConfigMode.c inst/SQLRemoveDriverManager.c inst/SQLGetInstalledDrivers.c \
	inst/SQLRemoveTranslator.c inst/SQLGetPrivateProfileString.c  inst/SQLSetConfigMode.c \
	inst/SQLGetTranslator.c inst/SQLValidDSN.c inst/SQLInstallDriver.c inst/SQLWriteDSNToIni.c \
	inst/SQLInstallDriverEx.c inst/SQLWriteFileDSN.c inst/SQLInstallDriverManager.c \
	inst/SQLWritePrivateProfileString.c inst/SQLInstallODBC.c inst/dlf.c inst/SQLInstallTranslator.c \
        inst/inifile.c inst/SQLInstallTranslatorEx.c inst/iodbc_error.c inst/SQLInstallerError.c \
	inst/misc.c inst/SQLManageDataSource.c inst/unicode.c


CUSTOM_DEFINES=HAVE_CONFIG_H

CUSTOM_INCLUDES=./include ./src ./src/inst ./src/trace

CUSTOM_CCFLAGS=

include ../../rules/archive.linux.makefile


