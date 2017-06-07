script "CoreControlTry"
/*
Copyright (C) 2017 LiveCode Ltd.

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

on TestTry
   try
     try
        NonExistantHandler1
     catch tError
        TestAssert "Catch catches error", true
        TestAssert "Error is not empty in catch", line 1 of tError contains "NonExistantHandler1"
        NonExistantHandler2
     finally
        TestAssert "Finally executes regardless of error in catch", true
     end try
  end try
end TestTry

on TestTryNoCatch
  local tTest
  try
     put false into tTest
  finally
     put true into tTest
  end try
  TestAssert "Finally executes regardless of catch block", tTest
end TestTryNoCatch

on TestTryFinallyNoStatements
  local tTest = false
  try
  finally
     put true into tTest
  end try
  TestAssertBroken "Finally executes regardless of try statements", tTest, "Bug 19811"
end TestTryFinallyNoStatements

on TestTryNoCatchWithError
  try
     NonExistantHandler
  finally
     TestAssert "Finally executes regardless of no catch block with try error", true
  end try
end TestTryNoCatchWithError

on TestTryMultipleEvents
   if the environment is "command line" or the platform is "HTML5" then
      TestSkip "Try execution context", "no type command available"
      return empty
   end if
   
   try
      create stack "test"
      set the defaultStack to "test"
      create field "test"
      set the script of field "test" to "on keyDown;NonExistantHandler1;end keyDown"
      focus on field "test"
      type "g"
   catch tError1
   end try
   TestAssert "Errors from other events aren't caught", tError1 is empty
   
   try
      NonExistantHandler2
   catch tError2
   end try
   TestAssert "Errors from other events don't pollute caught error", line 1 of tError2 contains "NonExistantHandler2"
end TestTryMultipleEvents

on TestTryErrorInCatch
   TestAssertBroken "Error in catch is thrown", false, "Bug 19812"
   /*TestAssertThrow "Error in catch is thrown", "__TestTryErrorInCatch", the long id of me, "EE_STATEMENT_BADCOMMAND"
   try
      NonExistantHandler3
   catch tError
      TestAssert "Unthrown errors in catch are cleared", line 1 of tError contains "NonExistantHandler3"
   end try*/
end TestTryErrorInCatch

on __TestTryErrorInCatch
   try
      NonExistantHandler1
   catch tError
      NonExistantHandler2
   end try
end __TestTryErrorInCatch

on TestTryErrorInFinally
   TestAssertBroken "Error in finally is thrown", false, "Bug 19812"
   /*TestAssertThrow "Error in finally is thrown", "__TestTryErrorInFinally", the long id of me, "EE_STATEMENT_BADCOMMAND"
   try
      NonExistantHandler3
   catch tError
      TestAssert "Unthrown errors in finally are cleared", line 1 of tError contains "NonExistantHandler3"
   end try
   TestAssertThrow "Error in finally is thrown", "__TestTryErrorInFinallyNoTryStatements", the long id of me, "EE_STATEMENT_BADCOMMAND"*/
end TestTryErrorInFinally

on __TestTryErrorInFinally
   try
      NonExistantHandler1
   finally
      NonExistantHandler2
   end try
end __TestTryErrorInFinally

on __TestTryErrorInFinallyNoTryStatements
   try
   finally
      NonExistantHandler2
   end try
end __TestTryErrorInFinallyNoTryStatements

on TestTryErrorNoCatch
   try
      NonExistantHandler
   end try
   TestAssert "Error in try with no catch ignored", true
end TestTryErrorNoCatch

on TestTryPassHandler
   try
      try
         pass TestTryPassHandler
      end try
      NonExistantHandler
   catch tError1
   end try
   TestAssertBroken "Handler passed from try", tError1 is empty, "Bug 19809"
   
   try
      try
         NonExistantHandler
      catch tError2
         pass TestTryPassHandler
      end try
   catch tError3
   end try
   TestAssertBroken "Handler passed from catch", tError3 is empty, "Bug 19809"

   try
      try
         NonExistantHandler
      finally
         pass TestTryPassHandler
      end try
   catch tError4
   end try
   TestAssertBroken "Handler passed from finally", tError4 is empty, "Bug 19809"

end TestTryPassHandler

on TestTryPassHandlerToTop
   try
      pass TestTryPassHandlerToTop to top
   finally
      TestAssert "Finally executed when handler pass to top from try", true
   end try
   TestAssert "Handler passed to top from try", false
end TestTryPassHandlerToTop

on TestTryCatchPassHandlerToTop
   try
      NonExistantHandler
   catch tError
      pass TestTryCatchPassHandlerToTop to top
   finally
      TestAssert "Finally executed when handler pass to top from catch", true
   end try
   TestAssertBroken "Handler passed to top from catch", false
end TestTryCatchPassHandlerToTop

on TestTryFinallyPassHandlerToTop
   try
      NonExistantHandler
   finally
      pass TestTryFinallyPassHandlerToTop to top
   end try
   TestAssertBroken "Handler passed to top from finally", false, "Bug 11773"
end TestTryFinallyPassHandlerToTop

on TestTryExitHandler
   try
      exit TestTryExitHandler
   finally
      TestAssert "Finally executed when handler exits from try", true
   end try
   TestAssert "Handler exit from try", false
end TestTryExitHandler

on TestTryCatchExitHandler
   try
      NonExistantHandler
   catch tError2
      exit TestTryCatchExitHandler
   finally
      TestAssert "Finally executed when handler exits from catch", true
   end try
   TestAssert "Handler exit from catch", false
end TestTryCatchExitHandler

on TestTryFinallyExitHandler
   try
      NonExistantHandler
   finally
      exit TestTryFinallyExitHandler
   end try
   TestAssertBroken "Handler exit from finally", false, "Bug 11773"
end TestTryFinallyExitHandler

on TestTryReturn
   try
      return empty
   finally
      TestAssert "Finally executed when return from try", true
   end try
   TestAssert "Return from try", false
end TestTryReturn

on TestTryCatchReturn
   try
      NonExistantHandler
   catch tError
      return empty
   finally
      TestAssert "Finally executed when return from catch", true
   end try
   TestAssert "Return from catch", false
end TestTryCatchReturn

on TestTryFinallyReturn
   try
      NonExistantHandler
   finally
      return empty
   end try
   TestAssertBroken "Return from finally", false, "Bug 11773"
end TestTryFinallyReturn
