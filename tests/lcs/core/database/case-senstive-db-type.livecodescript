script "case-senstive-db-type"
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

local sDBDir

on TestSetup
   TestSkipIfNot "database", "sqlite"   
   TestLoadExternal "revdb"
   
   put TestGetEngineRepositoryPath() & "/_tests/_build/case-sensitive-db-type" into sDBDir
   create directory sDBDir
end TestSetup

on TestTeardown
   delete directory sDBDir
end TestTeardown

on TestOpenDBWithMixedCaseForType
   local tDBpath, tConnectionID
   
   put sDBDir & "/test1-sqlite.db" into tDBPath   
   put revOpenDatabase("sqlite", tDBPath) into tConnectionID
   TestAssert "open database of type sqlite" && tConnectionID, tConnectionID is an integer
   
   put sDBDir & "/test2-SQLite.db" into tDBPath   
   put revOpenDatabase("SQLite", tDBPath) into tConnectionID
   TestAssert "open database of type SQLite" && tConnectionID, tConnectionID is an integer   
end TestOpenDBWithMixedCaseForType
