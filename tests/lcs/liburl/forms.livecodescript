script "TestlibUrlForms"

on TestSetup
   TestSkipIfNot "platform", "MacOS,Windows,Linux"
   TestSkipIf "environment", "server"
   
   if not TestIsInStandalone() then
      local tLibURl
      put TestGetEngineRepositoryPath() & "/ide-support/revliburl.livecodescript" into tLibUrl
      send "extensionInitialize" to stack tLibUrl
   end if
end TestSetup


on TestForm
   put empty into tForm
   put "name" into tName
   put "message" into tMessage
   put libURLMultipartFormData (tForm, "name", tName, "message", tMessage) into tError
   TestAssert "form created", tError is empty
   
   set the httpHeaders to line 1 of tForm
   delete line 1 of tForm
   
   put "<file>" &  TestGetInputFile("last, first.txt") into tFile
   put "text/plain" into tType
   put "binary" into tEnc
   put libURLMultipartFormAddPart(tForm,"file", tFile, tType, tEnc) into tError
   TestAssert "file successfully added to form", tError is empty
   
   set the httpHeaders to empty
end TestForm
