script "TestLibrary"
/*
Copyright (C) 2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

----------------------------------------------------------------
-- Helper functions
----------------------------------------------------------------

private function _TestValidateDescription pDescription
   if the number of lines in pDescription > 1 then
      throw "Bad test description '" & line 1 of pDescription & "...': multiple lines"
   end if
   
   if "0123456789" contains codepoint 1 of pDescription then
      throw "Bad test description '" & pDescription & "': starts with digit"
   end if
   
   if pDescription contains "#" then
      throw "Bad test description '" & pDescription & "': contains '#'"
   end if
   
   return line 1 of pDescription
end _TestValidateDescription

private function _TestValidateReason pReason
   if the number of lines in pReason > 1 then
      throw "Bad test directive reason '" & line 1 of pReason & "...': multiple lines"
   end if
   
   if pReason contains "#" then
      throw "Bad test directive reason '" & pReason & "': contains '#'"
   end if
   
   return line 1 of pReason
end _TestValidateReason

private function _TestValidateDirective pDirective
   switch pDirective
      case empty
         return empty
      case "TODO"
         return "TODO"
      case "SKIP"
         return "SKIP"
      default
         throw "Bad test directive '" & line 1 of pDirective & "'"
   end switch
end _TestValidateDirective

-- Used by top level assertion functions to generate output
private command _TestOutput pIsOkay, pDescription, pDirective, pReason
   local tDescription, tDirective, tReason
   put _TestValidateDescription(pDescription) into tDescription
   put _TestValidateDirective(pDirective) into tDirective
   put _TestValidateReason(pReason) into tReason
   
   local tMessage
   if pIsOkay then
      put "ok" into tMessage
   else
      put "not ok" into tMessage
   end if
   
   if tDescription is not empty then
      put " - " & tDEscription after tMessage
   end if
   
   if tDirective is not empty then
      put " # " & tDirective after tMessage
      
      if tReason is not empty then
         put " " & tReason after tMessage
      end if
   end if
   
   write tMessage & return to stdout
end _TestOutput

----------------------------------------------------------------
-- Unit test library functions
----------------------------------------------------------------

on TestDiagnostic pMessage
   local tLine
   repeat for each line tLine in pMessage
      write "#" && tLine & return to stdout
   end repeat
end TestDiagnostic

on TestSkip pDescription, pReasonSkipped
   _TestOutput true, pDescription, "SKIP", pReasonSkipped
end TestSkip

on TestAssert pDescription, pExpectTrue
   _TestOutput pExpectTrue, pDescription, empty, empty
end TestAssert

on TestAssertBroken pDescription, pExpectTrue, pReasonBroken
   _TestOutput pExpectTrue, pDescription, "TODO", pReasonBroken
end TestAssertBroken

on TestAssertThrow pDescription, pHandler, pTarget, pErrorCode
   local tError
   try
      dispatch pHandler to pTarget
    catch tError
    end try
    
    TestAssert pDescription, pErrorCode is item 1 of line 1 of tError
end TestAssertThrow

on ErrorDialog executionError, parseError
   write executionError & return to stderr
   quit 1
end ErrorDialog
