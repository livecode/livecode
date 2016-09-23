script "TestLibUrlCookies"

local sLogField

on TestSetup
   -- Only run these tests on desktop platforms
   if the platform is not among the items of "MacOS,Windows,Linux" then
      return "SKIP Tests are not runnable on" && the platform
   end if

   local tLibURl
   put TestGetEngineRepositoryPath() & "/ide-support/revliburl.livecodescript" into tLibUrl
   send "revLoadLibrary" to stack tLibUrl
end TestSetup


on TestCookies
   libURLProcessCookies true
   
   put URL "https://www.google.com" into tData
   TestAssert "no error is reported", the result is empty

   put libURLLastRHHeaders() into tHeaders
   TestAssert "google response includes expected cookie", tHeaders contains "Set-Cookie: "

   put "ping" into url "http://www.google.com"
   
   put libURLLastHTTPHeaders() into tHeaders
   TestAssert "request contains cookie", tHeaders contains "Cookie: "
end TestCookies
