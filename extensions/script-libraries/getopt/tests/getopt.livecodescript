script "TestGetOptLib"
/*
Copyright (C) 2016 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.
*/

on TestSetup
   local tLibFilename
   put TestGetEngineRepositoryPath() & \
         "/extensions/script-libraries/getopt/getopt.livecodescript" \
         into tLibFilename

   start using stack tLibFilename
end TestSetup

on TestGetOpt
   local tResult
   
   -- Check single short options work
   _Test_GetOpt "h", "", tResult
   TestAssert "missing short option", \
         _Test_CheckStats(tResult, 0, 0, 0)
   
   _Test_GetOpt "", "-h", tResult
   TestAssert "unknown short option", \
         _Test_CheckStats(tResult, 0, 0, 1)
   
   _Test_GetOpt "h", "-h", tResult
   TestAssert "short option", \
         _Test_CheckOpt(tResult, "h", empty) and \
         _Test_CheckStats(tResult, 1, 0, 0)
   
   _Test_GetOpt "h,help", "-h", tResult
   TestAssert "aliased short option", \
         _Test_CheckOpt(tResult, "help", empty) and \
         _Test_CheckStats(tResult, 1, 0, 0)
   
   -- Check multiple short options work
   _Test_GetOpt "h v", "-h -v", tResult
   TestAssert "separate short options", \
         _Test_CheckOpt(tResult, "v", empty) and \
         _Test_CheckOpt(tResult, "h", empty) and \
         _Test_CheckStats(tResult, 2, 0, 0)
   
   _Test_GetOpt "h v", "-hv", tResult
   TestAssert "joined short options", \
         _Test_CheckOpt(tResult, "v", empty) and \
         _Test_CheckOpt(tResult, "h", empty) and \
         _Test_CheckStats(tResult, 2, 0, 0)
   
   -- Check short options with arguments work
   _Test_GetOpt "o=", "-o", tResult
   TestAssert "no arg short option", \
         _Test_CheckStats(tResult, 0, 0, 1)
   
   _Test_GetOpt "o=", "-o-o", tResult
   TestAssert "joined arg short option", \
         _Test_CheckOpt(tResult, "o", "-o") and \
         _Test_CheckStats(tResult, 1, 0, 0)
   
   _Test_GetOpt "o=", "-o -o", tResult
   TestAssert "separate arg short option", \
         _Test_CheckOpt(tResult, "o", "-o") and \
         _Test_CheckStats(tResult, 1, 0, 0)
   
   -- Check long options work
   _Test_GetOpt "", "--help", tResult
   TestAssert "unknown long option", \
         _Test_CheckStats(tResult, 0, 0, 1)
   
   _Test_GetOpt "help", "--help", tResult
   TestAssert "long option", \
         _Test_CheckOpt(tResult, "help", empty) and \
         _Test_CheckStats(tResult, 1, 0, 0)
   
   _Test_GetOpt "help,alias", "--help", tResult
   TestAssert "aliased long option", \
         _Test_CheckOpt(tResult, "alias", empty) and \
         _Test_CheckStats(tResult, 1, 0, 0)
   
   -- Check long options with arguments work
   _Test_GetOpt "output=", "--output", tResult
   TestAssert "no arg long option", \
         _Test_CheckStats(tResult, 0, 0, 1)
   
   _Test_GetOpt "output=", "--output=--output", tResult
   TestAssert "joined arg long option", \
         _Test_CheckOpt(tResult, "output", "--output") and \
         _Test_CheckStats(tResult, 1, 0, 0)
   
   _Test_GetOpt "output=", "--output==", tResult
   TestAssert "'=' can be passed as option arg", \
         _Test_CheckOpt(tResult, "output", "=") and \
         _Test_CheckStats(tResult, 1, 0, 0)
   
   _Test_GetOpt "output=", "--output --output", tResult
   TestAssert "separate arg long option", \
         _Test_CheckOpt(tResult, "output", "--output") and \
         _Test_CheckSTats(tResult, 1, 0, 0)
   
   -- Check command arguments work
   _Test_GetOpt "h", "A", tResult
   TestAssert "argument", \
         _Test_CheckArg(tResult, "A") and \
         _Test_CheckStats(tResult, 0, 1, 0)
   
   _Test_GetOpt "h", "-", tResult
   TestAssert "'-' is an argument", \
         _Test_CheckArg(tResult, "-") and \
         _Test_CheckStats(tResult, 0, 1, 0)
   
   _Test_GetOpt "h", "A -h", tResult
   TestAssert "argument ends option processing", \
         _Test_CheckArg(tResult, "-h") and \
         _Test_CheckStats(tResult, 0, 2, 0)
   
   _Test_GetOpt "h", "-- -h", tResult
   TestAssert "'--' ends option processing", \
         _Test_CheckArg(tResult, "-h") and \
         _Test_CheckStats(tResult, 0, 1, 0)
   
   _Test_GetOpt "o=", "-o --", tResult
   TestAssert "'--' can be passed as option arg", \
         _Test_CheckOpt(tResult, "o", "--") and \
         _Test_CheckStats(tResult, 1, 0, 0)
   
   put tTap
end TestGetOpt

/*
We don't let the main GetOpt() function accept its arguments
as a string because there's no correct way to convert a string
to an argument array unless you already know the correct way to
split it.  We can use this convenience function for the tests
because we have complete control over the form of the argument
string.
*/
private command _Test_GetOpt pGrammarString, pArgString, @rResult
   split pArgString by space
   put GetOpt(pGrammarString, pArgString) into rResult
end _Test_GetOpt

private function _Test_CheckOpt pResult, pName, pValue
   return \
         (pName is among the keys of pResult["options"]) and \
         (pResult["options"][pName] is pValue)
end _Test_CheckOpt

private function _Test_CheckArg pResult, pValue
   local tArg
   repeat for each element tArg in pResult["arguments"]
      if tArg is pValue then
         return true
      end if
   end repeat
   return false
end _Test_CheckArg

private function _Test_CheckStats pResult, pOptCount, pArgCount, pErrCount
   return \
         (the number of elements in pResult["options"] is pOptCount) and \
         (the number of elements in pResult["arguments"] is pArgCount) and \
         (the number of elements in pResult["errors"] is pErrCount)
end _Test_CheckStats

private command _Test_Log @xLog, pDescription, pSuccess
   if pSuccess then
      put merge("ok - [[pDescription]]") & return after xLog
   else
      put merge("not ok - [[pDescription]]") & return after xLog
   end if
end _Test_Log
