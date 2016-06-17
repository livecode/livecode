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

private command SetTheResultTo pValue
   return pValue
end SetTheResultTo

private command DoReturnInCommand pValue
   return pValue
end DoReturnInCommand

private function DoReturnInFunction pValue
   return pValue
end DoReturnInFunction

on TestReturn
   SetTheResultTo empty
   get "TheIt"
   DoReturnInCommand "TheResult"
   TestAssert "return in command leaves it and sets the result", \
                  it is "TheIt" and \
                  the result is "TheResult"

   local tFu
   SetTheResultTo empty
   get "TheIt"
   TestAssert "return in function leaves it, sets the result to return value and returns the return value", \
                  DoReturnInFunction("TheResult") is "TheResult" and \
                  the result is "TheResult" and \
                  it is "TheIt"
end TestReturn

private command DoReturnValueInCommand pValue
   return pValue for value
end DoReturnValueInCommand

private function DoReturnValueInFunction pValue
   return pValue for value
end DoReturnValueInFunction

on TestReturnValue
   SetTheResultTo "TheResult"
   get empty
   DoReturnValueInCommand "TheIt"
   TestAssert "return value in command sets it and clears the result", \
                  the result is empty and \
                  it is "TheIt"

   SetTheResultTo "TheResult"
   get "TheIt"
   TestAssert "return value in function leaves it, clears the result and returns the return value", \
                  DoReturnValueInFunction("ReturnValue") is "ReturnValue" and \
                  the result is empty and \
                  it is "TheIt"
end TestReturnValue

private command DoReturnErrorInCommand pValue
   return pValue for error
end DoReturnErrorInCommand

private function DoReturnErrorInFunction pValue
   return pValue for error
end DoReturnErrorInFunction

on TestReturnError
   SetTheResultTo empty
   get "TheIt"
   DoReturnErrorInCommand "TheResult"
   TestAssert "return error in command clears it and sets the result", \
                  the result is "TheResult" and \
                  it is empty

   SetTheResultTo empty
   get "TheIt"
   TestAssert "return error in function leaves it, sets the result to the return value and returns empty", \
                  DoReturnErrorInFunction("ReturnValue") is empty and \
                  the result is "ReturnValue" and \
                  it is "TheIt"
end TestReturnError
