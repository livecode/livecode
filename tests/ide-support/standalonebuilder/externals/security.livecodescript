script "TestRevXML"
on _testInclusion
  get cipherNames()
  if it is not empty then
	 quit 0
  else
	 write "error - ciphernames empty" to stderr
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