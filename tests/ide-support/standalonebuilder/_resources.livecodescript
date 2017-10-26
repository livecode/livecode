script "TestExtensionResourcesStandalone"
on _testExtension
  local tFolder
  put GetResourcesFolder() into tFolder
  return url("file:" & tFolder & "/resource.txt")
end _testExtension

on startup
  set the lockErrorDialogs to true
  try
	 _testExtension
  catch tError
	 write "resources folder not present in standalone:" && tError to stderr
	 quit 1
  end try
  
  if the result is not "true" then
  	 write "resources file not present in standalone" to stderr
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
	write "resources folder not present in standalone:" && pError to stderr
	quit 1
end errorDialog