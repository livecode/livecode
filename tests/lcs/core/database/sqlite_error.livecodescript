script "TestSQLiteError"
local sDatabaseID, sDatabaseFile

on TestSetup
	TestSkipIfNot "database", "sqlite"
	TestSkipIfNot "external", "revsecurity"

	TestLoadExternal "revdb"

	put the tempname into sDatabaseFile
	put revOpenDatabase("sqlite",sDatabaseFile,,,,) into sDatabaseID
	revExecuteSQL sDatabaseID, \
		"CREATE TABLE FOO (ROWID INTEGER PRIMARY KEY, VALUE TEXT);"
end TestSetup

on TestTeardown
	revCloseDatabase sDatabaseID
	delete file sDatabaseFile
end TestTeardown

on TestErrorWithPercentInQuery
	revExecuteSQL sDatabaseID, "INSERT INTO FOO VALUES (1, 'oh look a %s');"
	-- use query here because the error message includes the query
	get revDataFromQuery(comma, return, sDatabaseID, \
		"INSERT INTO FOO VALUES (1, 'oh look a %s');")
	TestAssert "check error with '%s' in query", \
		the result contains "'oh look a %s'"
end TestErrorWithPercentInQuery
