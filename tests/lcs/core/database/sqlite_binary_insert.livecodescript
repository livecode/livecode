script "TestSQLiteBinaryINSERT"
local lDatabaseID, lDatabaseDir
constant kTestTable = "binaryTest"

private command __createDatabase pDatabaseFile, pBinary
   if pBinary is false then
      put revOpenDatabase("sqlite",pDatabaseFile,,,,) into lDatabaseID
   else
      put revOpenDatabase("sqlite",pDatabaseFile,"binary",,,) into lDatabaseID
   end if
end __createDatabase

private command __deleteDatabase pDatabaseFile
   revCloseDatabase lDatabaseID
   delete file pDatabaseFile
end __deleteDatabase

private command __createBinaryTable pTable
   local tCreateSQL
   
   put "CREATE TABLE " & pTable & " (binaryTest blob)" into tCreateSQL
   revExecuteSQL lDatabaseID, tCreateSQL
end __createBinaryTable

private command __addBinaryData pTable, pData
   local tFieldData, tInsertSQL
   
   put pData into tFieldData["*b1"]
   put "INSERT INTO " & pTable & " (binaryTest) VALUES (:1)" into tInsertSQL
   revExecuteSQL lDatabaseID, tInsertSQL, "tFieldData"
end __addBinaryData

private function __retrBinaryData pTable, pData
   local tSelectRowsRecordSet, tInsertSQL, tData, tResult
   
   put "SELECT binaryTest FROM " & pTable into tInsertSQL
   put revQueryDatabase(lDatabaseID, tInsertSQL) into tSelectRowsRecordSet
   
   if tSelectRowsRecordSet is a number then
      revMoveToFirstRecord tSelectRowsRecordSet
      put the result into tResult
      
      if tResult is not false and the first item of tResult is not "revdberr" then
         get revDatabaseColumnNamed(tSelectRowsRecordSet,"binaryTest","tData")
         revCloseCursor tSelectRowsRecordSet
      end if
   end if
   
   return tData
end __retrBinaryData

on TestSetup
   TestSkipIfNot "database", "sqlite"
   TestSkipIfNot "external", "revsecurity"
   
   TestLoadExternal "revdb"
   
   put TestGetEngineRepositoryPath() & "/_tests/_build/sqlite_binary_test" into lDatabaseDir
   create directory lDatabaseDir
end TestSetup

on TestTeardown
   delete directory lDatabaseDir
end TestTeardown

on TestBinaryWithOptionINSERT
   local tDatabaseFile, tAddData, tSelectData
   
   put randomBytes(10000000) into tAddData
   
   put lDatabaseDir & "/sqlite-binary.db" into tDatabaseFile
   __createDatabase tDatabaseFile, true
   __createBinaryTable kTestTable
   __addBinaryData kTestTable, tAddData
   put __retrBinaryData(kTestTable, tAddData) into tSelectData
   __deleteDatabase tDatabaseFile
   
   TestAssert "check binary data insert into SQLite database with binary option", tAddData is tSelectData
end TestBinaryWithOptionINSERT

on TestBinaryWithoutOptionINSERT
   local tDatabaseFile, tAddData, tSelectData
   
   put randomBytes(10000000) into tAddData
   
   put lDatabaseDir & "/sqlite-nobinary.db" into tDatabaseFile
   __createDatabase tDatabaseFile, false
   __createBinaryTable kTestTable
   __addBinaryData kTestTable, tAddData
   put __retrBinaryData(kTestTable, tAddData) into tSelectData
   __deleteDatabase tDatabaseFile
   
   TestAssert "check binary data insert into SQLite database without binary option", tAddData is tSelectData
end TestBinaryWithoutOptionINSERT
