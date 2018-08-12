script "TestLibUrlStatusCodes"

local sLogField

on TestSetup
   TestSkipIfNot "platform", "MacOS,Windows,Linux"
   TestSkipIf "environment", "server"
   
   if not TestIsInStandalone() then
      local tLibURl
      put TestGetEngineRepositoryPath() & "/ide-support/revliburl.livecodescript" into tLibUrl
      send "extensionInitialize" to stack tLibUrl
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
