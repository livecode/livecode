script "TestLibUrlCONNECT"

on TestSetup
   TestSkipIfNot "platform", "MacOS,Windows,Linux"
   TestSkipIf "stack", "StandaloneTestRunnerMainstack"
   TestSkipIf "environment", "server"
         
   local tLibURl
   put TestGetEngineRepositoryPath() & "/ide-support/revliburl.livecodescript" into tLibUrl
   send "extensionInitialize" to stack tLibUrl
end TestSetup


on TestCONNECTHeader
   constant kTestHost = "google.com"
   set the httpproxy to "http://127.0.0.1:65535"
   
   put url ("https://" & kTestHost) into tValue
   
   put liburlLastHTTPHeaders() into tHeaders
   
   put "CONNECT" && kTestHost & ":443 HTTP/1.1" & cr & \
         "Host: " & kTestHost & ":443" into tExpectedHeader
   
   TestAssert "correct CONNECT header", tHeaders is tExpectedHeader
end TestCONNECTHeader
