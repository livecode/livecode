script "TestLibUrlCONNECT"

on TestSetup
   -- Only run these tests on desktop platforms
   if the platform is not among the items of "MacOS,Windows,Linux" then
      return "SKIP Tests are not runnable on" && the platform
   end if

   local tLibURl
   put TestGetEngineRepositoryPath() & "/ide-support/revliburl.livecodescript" into tLibUrl
   send "revLoadLibrary" to stack tLibUrl
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
