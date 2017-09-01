script "CoreDatabaseTest"

/*
Copyright (C) 2017 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

function generateStrings 
   local tArray, tBaseString, tCounter
   put 1 into tCounter
   put "Table%d" into tBaseString
   repeat 1800 times
      put format(tBaseString,tCounter) into tArray[tCounter]
      put tCounter+1 into tCounter
   end repeat
   return tArray
end generateStrings

command createTables pConnection, pStrings
   local tBaseStmt, tCounter
   put "CREATE TABLE %s (id integer)" into tBaseStmt
   put 1 into tCounter
   repeat 1800 times
      revExecuteSQL pConnection, format(tBaseStmt, pStrings[tCounter])
      put tCounter+1 into tCounter
   end repeat
end createTables

on TestDBTableGettersMySQL
   TestSkip "skipping DB test", "cannot use mysql in database tests"
   exit TestDBTableGettersMySQL
   
   TestSkipIfNot "database", "mysql"

   TestLoadExternal "revdb"
   
   local tArray
   put generateStrings() into tArray
   local tDatabaseID, tNames
   put revOpenDatabase("mysql","localhost","t2","root","test_password",false,"/tmp/mysql.sock",10,true) into tDatabaseID
   createTables(tDatabaseID, tArray)
   put revDatabaseTableNames(tDatabaseID) into tNames
   TestAssert "check the correct things are returned", the number of lines in tNames is 1800
end TestDBTableGettersMySQL
