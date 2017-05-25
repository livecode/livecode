script "TestLibUrlStatusCodes"

local sLogField

on TestSetup
   -- Only run these tests on desktop platforms
   if the platform is not among the items of "MacOS,Windows,Linux" then
      return "SKIP Tests are not runnable on" && the platform
   end if
   
   if "StandaloneTestRunnerMainstack" is not among the lines of the mainstacks then
      local tLibURl
      put TestGetEngineRepositoryPath() & "/ide-support/revliburl.livecodescript" into tLibUrl
      send "revLoadLibrary" to stack tLibUrl
   end if
end TestSetup


on TestStatusCodes
   # Response Code 204
   set the socketTimeoutInterval to 10000
   put the milliseconds into tStartTime
   put URL "http://httpstat.us/204" into tData
   TestAssert "no error is reported", the result is empty
   TestAssert "response is not delayed until socket timeout", the milliseconds - tStartTime < 9000
   TestAssert "data is empty", tData is empty
end TestStatusCodes
