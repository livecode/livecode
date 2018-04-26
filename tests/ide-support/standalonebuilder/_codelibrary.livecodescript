script "TestExtensionCodeLibraryStandalone"
on _testExtension
   local tFileData, tIni
   put url ("file:" & specialFolderPath("resources") & "/test.ini") into tFileData
   put iniToArray(tFileData) into tIni
	
	return tIni["user"]["name"] is "Bob Smith"
end _testExtension

on startup
  set the lockErrorDialogs to true
  try
	 _testExtension
  catch tError
	 write "code library call failed:" && tError to stderr
	 quit 1
  end try
  
  if the result is not true then
     write "code library call failed:" && tError to stderr
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
	write "code library call failed:" && pError to stderr
	quit 1
end errorDialog
