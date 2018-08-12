script "TestRevZip"
on _testInclusion
  get revZipOpenArchives()
  if it is empty then
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
