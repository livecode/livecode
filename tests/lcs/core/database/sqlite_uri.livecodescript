script "TestSQLiteUri"
local lDatabaseID, lDatabaseDir

private command __createDatabase pDatabaseFile, pURI
   if pURI is false then
      put revOpenDatabase("sqlite",pDatabaseFile,,,,) into lDatabaseID
   else
      put revOpenDatabase("sqlite","file:" & pDatabaseFile,"uri",,,) into lDatabaseID
   end if
end __createDatabase

private command __deleteDatabase pDatabaseFile
   revCloseDatabase lDatabaseID
   delete file pDatabaseFile
end __deleteDatabase


on TestSetup
   TestSkipIfNot "database", "sqlite"
   TestSkipIfNot "external", "revsecurity"
   
   TestLoadExternal "revdb"
   
   put TestGetEngineRepositoryPath() & "/_tests/_build/sqlite_uri_test" into lDatabaseDir
   create directory lDatabaseDir
end TestSetup

on TestTeardown
   delete directory lDatabaseDir
end TestTeardown

on TestBinaryWithOptionURI
   local tDatabaseFile   
   put lDatabaseDir & "/sqlite-uri.db" into tDatabaseFile
   __createDatabase tDatabaseFile, true
   TestAssert "check creation of SQLite database with uri option", lDatabaseID is a number
   __deleteDatabase tDatabaseFile
   
   local tDatabaseFileReadOnly  
   put lDatabaseDir & "/sqlite-uri.db?mode=ro" into tDatabaseFileReadOnly
   __createDatabase tDatabaseFileReadOnly, true
   TestAssert "check creation of a read-only SQLite database with uri option", lDatabaseID is not a number
   
   local tDatabaseFileReadWrite  
   put lDatabaseDir & "/sqlite-uri.db?mode=rw" into tDatabaseFileReadWrite
   __createDatabase tDatabaseFileReadWrite, true
   TestAssert "check creation of SQLite read-write database with uri option", lDatabaseID is not a number
   
   local tDatabaseFileReadWriteCreate 
   put lDatabaseDir & "/sqlite-uri.db?mode=rwc" into tDatabaseFileReadWriteCreate
   __createDatabase tDatabaseFileReadWriteCreate, true
   TestAssert "check creation of read-write-create SQLite database with uri option", lDatabaseID is a number
   __deleteDatabase tDatabaseFileReadWriteCreate
   
end TestBinaryWithOptionURI

on TestBinaryWithoutOptionURI
   local tDatabaseFile
   
   put lDatabaseDir & "/sqlite-no-uri.db" into tDatabaseFile
   __createDatabase tDatabaseFile, false
   TestAssert "check creation of SQLite database without uri option", lDatabaseID is a number
   __deleteDatabase tDatabaseFile
end TestBinaryWithoutOptionURI
