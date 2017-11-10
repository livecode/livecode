script "TestLibUrlHeaders"

local sLogField

on TestSetup
   TestSkipIfNot "platform", "MacOS,Windows,Linux"
   TestSkipIf "stacks loaded", "tsNetLibURL"
   TestSkipIf "environment", "server"
   
   if not TestIsInStandalone() then
      local tLibURl
      put TestGetEngineRepositoryPath() & "/ide-support/revliburl.livecodescript" into tLibUrl
      send "extensionInitialize" to stack tLibUrl
   end if
   
   create field
   put it into sLogField
   libUrlSetLogField sLogField
end TestSetup

-- Parse the log field to extract the HTTP header block
-- This is a bit fragile
private function GetHeadersFromLogField
   local tHeaders, tFound, tLine
   put false into tFound
   repeat for each line tLine in the text of sLogField
      if not tFound then
         if "/ HTTP/1.1" is in tLine then
            put true into tFound
         else
            next repeat
         end if
      end if

      if tLine is empty then
         exit repeat
      end if

      put tLine & return after tHeaders
   end repeat
   return tHeaders
end GetHeadersFromLogField

on TestHostHeader
   constant kTestHost = "uniquehostname.example.org"
   set the httpHeaders to "Host:" && kTestHost
   post "dummy content" to "http://127.0.0.1:16000"

   local tHeaders
   put GetHeadersFromLogField() into tHeaders
   TestDiagnostic tHeaders

   local tHostCount
   put 0 into tHostCount
   repeat for each line tLine in tHeaders
      put word 1 to -1 of tLine into tLine
      if tLine begins with "Host:" then
         TestAssert "host header has correct value", tLine ends with kTestHost
         add 1 to tHostCount
      end if
   end repeat

   TestAssert "exactly one host header", tHostCount is 1
end TestHostHeader

on TestTeardown
   set the httpHeaders to empty
   libUrlSetLogField empty
end TestTeardown
