script "TestRevSqlite"
on _testInclusion
  get revOpenDatabase("sqlite","test.db")
  if not (it begins with "revdberr") then
	 quit 0
  else
	 write "error - " & it to stderr
	 quit 1
  end if
end _testInclusion

on startup
  try
	 _testInclusion
  catch tError
	 write "inclusion not loaded" to stderr
	 quit 1
  end try
end startup