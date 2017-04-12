script "TestlibUrlForms"

on TestSetup
   -- Only run these tests on desktop platforms
   if the platform is not among the items of "MacOS,Windows,Linux" then
      return "SKIP Tests are not runnable on" && the platform
   end if

   local tLibURl
   put TestGetEngineRepositoryPath() & "/ide-support/revliburl.livecodescript" into tLibUrl
   send "revLoadLibrary" to stack tLibUrl
end TestSetup


on TestForm
   put empty into tForm
   put "name" into tName
   put "message" into tMessage
   put libURLMultipartFormData (tForm, "name", tName, "message", tMessage) into tError
   TestAssert "form created", tError is empty
   
   set the httpHeaders to line 1 of tForm
   delete line 1 of tForm
   
   put "<file>" & "lcs/liburl/last, first.txt" into tFile
   put "text/plain" into tType
   put "binary" into tEnc
   put libURLMultipartFormAddPart(tForm,"file", tFile, tType, tEnc) into tError
   TestAssert "file successfully added to form", tError is empty
   
   set the httpHeaders to empty
end TestForm
