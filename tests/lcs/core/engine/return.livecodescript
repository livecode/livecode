script "CoreEngineReturn"
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
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

--------

private command SetTheResultTo pValue
   return pValue
end SetTheResultTo

-------- Test behavior of no return statement

-- Engine behavior is that script handlers reset 'the result' to empty before
-- they execute, so this should be seen outside the handler if there is no
-- return.

private command DoNothingThenNoReturnInCommand
end DoNothingThenNoReturnInCommand

private function DoNothingThenNoReturnInFunction
end DoNothingThenNoReturnInFunction

-- Engine behavior is that 'the result' is not touched if there is no return
-- statement, so the last value 'the result' was set to in the handler should
-- be its value after the handler has finished executing.

private command DoTailThenNoReturnInCommand pValue
   SetTheResultTo pValue
end DoTailThenNoReturnInCommand

private function DoTailThenNoReturnInFunction pValue
   SetTheResultTo pValue
end DoTailThenNoReturnInFunction

on TestNoReturn
   SetTheResultTo "TheResult"
   get "TheIt"
   DoNothingThenNoReturnInCommand
   TestAssert "no return in command leaves it and sets the result to empty", \
                  it is "TheIt" and \
                  the result is empty

   SetTheResultTo "TheResult"
   get "TheIt"
   TestAssert "no return in function leaves it, returns empty and sets the result to empty", \
                  DoNothingThenNoReturnInFunction() is empty and \
                  it is "TheIt" and \
                  the result is empty

   SetTheResultTo "TheResult"
   get "TheIt"
   DoTailThenNoReturnInCommand "NewResult"
   TestAssert "no return in tail command leaves it and sets the result to the result of the tail", \
                  it is "TheIt" and \
                  the result is "NewResult"

   SetTheResultTo "TheResult"
   get "TheIt"
   TestAssert "no return in tail function leaves it, returns the result of the tail and sets the result to the tail", \
                  DoTailThenNoReturnInFunction("NewResult") is "NewResult" and \
                  it is "TheIt" and \
                  the result is "NewResult"
end TestNoReturn


-------- Test behavior of the return statement

private command DoReturnInCommand pValue
   return pValue
end DoReturnInCommand

private function DoReturnInFunction pValue
   return pValue
end DoReturnInFunction

on TestReturn
   SetTheResultTo "TheResult"
   get "TheIt"
   DoReturnInCommand "NewResult"
   TestAssert "return in command leaves it and sets the result", \
                  it is "TheIt" and \
                  the result is "NewResult"

   local tFu
   SetTheResultTo "TheResult"
   get "TheIt"
   TestAssert "return in function leaves it, sets the result to return value and returns the return value", \
                  DoReturnInFunction("NewResult") is "NewResult" and \
                  the result is "NewResult" and \
                  it is "TheIt"
end TestReturn

-------- Test behavior of the return for value statement

private command DoReturnValueInCommand pValue
   return pValue for value
end DoReturnValueInCommand

private function DoReturnValueInFunction pValue
   return pValue for value
end DoReturnValueInFunction

on TestReturnValue
   SetTheResultTo "TheResult"
   get "TheIt"
   DoReturnValueInCommand "NewIt"
   TestAssert "return value in command sets it and clears the result", \
                  the result is empty and \
                  it is "NewIt"

   SetTheResultTo "TheResult"
   get "TheIt"
   TestAssert "return value in function leaves it, clears the result and returns the return value", \
                  DoReturnValueInFunction("ReturnValue") is "ReturnValue" and \
                  the result is empty and \
                  it is "TheIt"
end TestReturnValue

-------- Test behavior of the return for error statement

private command DoReturnErrorInCommand pValue
   return pValue for error
end DoReturnErrorInCommand

private function DoReturnErrorInFunction pValue
   return pValue for error
end DoReturnErrorInFunction

on TestReturnError
   SetTheResultTo "TheResult"
   get "TheIt"
   DoReturnErrorInCommand "NewResult"
   TestAssert "return error in command clears it and sets the result", \
                  the result is "NewResult" and \
                  it is empty

   SetTheResultTo "TheResult"
   get "TheIt"
   TestAssert "return error in function leaves it, sets the result to the return value and returns empty", \
                  DoReturnErrorInFunction("ReturnValue") is empty and \
                  the result is "ReturnValue" and \
                  it is "TheIt"
end TestReturnError

-------- Test that nested uses of 'return for' don't affect the return action
-------- of a caller.

private command DoReturnInNestedCommand pValue
   DoReturnInCommand pValue
end DoReturnInNestedCommand

private command DoReturnValueInNestedCommand pValue
   DoReturnValueInCommand pValue
end DoReturnValueInNestedCommand

private command DoReturnErrorInNestedCommand pValue
  DoReturnErrorInCommand pValue
end DoReturnErrorInNestedCommand

private function DoReturnInNestedFunction pValue
   get DoReturnInFunction(pValue)
end DoReturnInNestedFunction

private function DoReturnValueInNestedFunction pValue
   get DoReturnValueInFunction(pValue)
end DoReturnValueInNestedFunction

private function DoReturnErrorInNestedFunction pValue
  get DoReturnErrorInFunction(pValue)
end DoReturnErrorInNestedFunction

-- This checks that the absence of a return clause is not affected by any
-- return clause present in a handler called by the handler with no return
-- clause.
on TestReturnNested
   SetTheResultTo "TheResult"
   get "TheIt"
   DoReturnInNestedCommand "NewResult"
   TestAssert "return handler called in no-return handler leaves it and sets the result", \
                  the result is "NewResult" and \
                  it is "TheIt"

   SetTheResultTo "TheResult"
   get "TheIt"
   DoReturnValueInNestedCommand "NewResult"
   TestAssert "return value handler called in no-return handler leaves it and sets the result to empty", \
                  the result is empty and \
                  it is "TheIt"

   SetTheResultTo "TheResult"
   get "TheIt"
   DoReturnErrorInNestedCommand "NewResult"
   TestAssert "return error handler called in no-return handler leaves it and sets the result", \
                  the result is "NewResult" and \
                  it is "TheIt"

   SetTheResultTo "TheResult"
   get "TheIt"
   TestAssert "return function called in no-return function leaves it, sets the result to the return value and returns the return value", \
                  DoReturnInNestedFunction("ReturnValue") is "ReturnValue" and \
                  the result is "ReturnValue" and \
                  it is "TheIt"

   SetTheResultTo "TheResult"
   get "TheIt"
   TestAssert "return value function called in no-return function leaves it, sets the result to the return value and returns the return value", \
                  DoReturnValueInNestedFunction("ReturnValue") is empty and \
                  the result is empty and \
                  it is "TheIt"

   SetTheResultTo "TheResult"
   get "TheIt"
   TestAssert "return error function called in no-return function leaves it, sets the result to the return value and returns the return value", \
                  DoReturnErrorInNestedFunction("ReturnValue") is "ReturnValue" and \
                  the result is "ReturnValue" and \
                  it is "TheIt"
end TestReturnNested

on TestMessageMessagesResultMode
   local tPath
   put format("%s/../_trace.livecodescript", the filename of me) into tPath
   start using stack tPath
   set the traceReturn to true
   set the traceStack to the long id of me
   DoReturnValueInCommand "foobar"
   local tIt, tResult
   put it into tIt
   put the result into tResult
   TestDiagnostic "it" && tIt
   TestDiagnostic "result" && tResult
   TestAssert "result mode is not clobbered in messageHandled", tIt is "foobar"
   set the traceStack to empty
   stop using stack tPath
end TestMessageMessagesResultMode

