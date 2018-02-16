script "TestLCSParser"
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

/* TODO 15/05/2017: Turn these tests into an LCS parser test when merging
up into develop branch */

private command __CompileCheck pMessage
   local tCompileTest, tCompileResult
   --checks to see if the message compiles
   lock messages
   create script only stack "compilationTest"
   put "on compileTest" & return & pMessage & return \
      & "end compileTest" into tCompileTest
   set the script of stack "compilationTest" to tCompileTest
   put the result into tCompileResult
   delete stack "compilationTest"
   unlock messages
   repeat for each line tError in tCompileResult
      if item 2 of tError is not 0 then
         return tError
      end if
   end repeat

   return empty
end __CompileCheck

on TestBug19688
	__CompileCheck "get the width"
   TestAssert "'get the <objprop>' parse error", the result is not empty
end TestBug19688

on TestBug20293
  __CompileCheck format("get the last char to -4 of \"string\"")
  TestAssert "'badrange parse error'", the result is not empty
end TestBug20293

on TestBug20269
	__CompileCheck "dispatch" && quote & "something" & quote && "with"
   TestAssert "dispatch command with 'with' and no params throws", \
      the result is not empty
end TestBug20269
