script "TestExtensionJavaStandalone"
on _testExtension
  return DoTest()
end _testExtension

on startup
  set the lockErrorDialogs to true
  try
	 _testExtension
  catch tError
	 write "java library call failed:" && tError to stderr
	 quit 1
  end try
  
  if the result is not "true" then
  	 write "java foreign handler call failed" to stderr
	 quit 1
  end if
  
  DoQuit
end startup

on DoQuit
  if "errorDialog" is in the pendingmessages then
    send "DoQuit" to me in 0 millisecs
  else
    quit 0
  end if
end DoQuit

on errorDialog pError
	write "java library call failed:" && pError to stderr
	quit 1
end errorDialog