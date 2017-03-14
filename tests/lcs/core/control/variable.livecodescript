script "CoreControlVariable"
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

on TestFetchLocal
   local tVar, tIndex

   put empty into tVar
   put 100 into tVar
   TestAssert "local direct access", tVar is 100

   put empty into tVar
   put 100 into tVar["one"]
   TestAssert "local indirect name - depth 1", tVar["one"] is 100

   put empty into tVar
   put 100 into tVar["one"]["one"]
   TestAssert "local indirect name - depth 2", tVar["one"]["one"] is 100

   put empty into tVar
   put 100 into tVar[1+0]
   TestAssert "local indirect number - depth 1", tVar[1+0] is 100

   put empty into tVar
   put 100 into tVar[1+0][1+0]
   TestAssert "local indirect number - depth 2", tVar[1+0][1+0] is 100

   put empty into tIndex
   put 1+0 into tIndex[1+0]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "local indirect number array - depth 1", tVar[tIndex] is 100

   put empty into tIndex
   put 1+0 into tIndex[1+0]
   put 1+0 into tIndex[2+0]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "local indirect number array - depth 2", tVar[tIndex] is 100

   put empty into tIndex
   put "one" into tIndex[1+0]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "local indirect name array - depth 1", tVar[tIndex] is 100

   put empty into tIndex
   put "one" into tIndex[1+0]
   put "one" into tIndex[2+0]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "local indirect name array - depth 2", tVar[tIndex] is 100

   put empty into tIndex
   put 1+0 into tIndex[1]
   put "one" into tIndex[2]
   put empty into tVar
   put 100 into tVar[1][tIndex]["one"]
   TestAssert "local indirect mixed - depth 4", tVar[1+0][tIndex]["one"] is 100
end TestFetchLocal

private command DoTestFetchParam tVar
   local tIndex

   put empty into tVar
   put 100 into tVar
   TestAssert "param direct access", tVar is 100

   put empty into tVar
   put 100 into tVar["one"]
   TestAssert "param indirect name - depth 1", tVar["one"] is 100

   put empty into tVar
   put 100 into tVar["one"]["one"]
   TestAssert "param indirect name - depth 2", tVar["one"]["one"] is 100

   put empty into tVar
   put 100 into tVar[1]
   TestAssert "param indirect number - depth 1", tVar[1] is 100

   put empty into tVar
   put 100 into tVar[1][1]
   TestAssert "param indirect number - depth 2", tVar[1][1] is 100

   put empty into tIndex
   put 1 into tIndex[1]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "param indirect number array - depth 1", tVar[tIndex] is 100

   put empty into tIndex
   put 1 into tIndex[1]
   put 1 into tIndex[2]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "param indirect number array - depth 2", tVar[tIndex] is 100

   put empty into tIndex
   put "one" into tIndex[1]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "param indirect name array - depth 1", tVar[tIndex] is 100

   put empty into tIndex
   put "one" into tIndex[1]
   put "one" into tIndex[2]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "param indirect name array - depth 2", tVar[tIndex] is 100

   put empty into tIndex
   put "1" into tIndex[1]
   put "one" into tIndex[2]
   put empty into tVar
   put 100 into tVar[1][tIndex]["one"]
   TestAssert "param indirect mixed - depth 4", tVar[1][tIndex]["one"] is 100
end DoTestFetchParam

on TestFetchParam
   local tParam
   DoTestFetchParam tParam
end TestFetchParam

private command DoTestFetchRefParam @tVar
   local tIndex

   put empty into tVar
   put 100 into tVar
   TestAssert "param direct access", tVar is 100

   put empty into tVar
   put 100 into tVar["one"]
   TestAssert "param indirect name - depth 1", tVar["one"] is 100

   put empty into tVar
   put 100 into tVar["one"]["one"]
   TestAssert "param indirect name - depth 2", tVar["one"]["one"] is 100

   put empty into tVar
   put 100 into tVar[1]
   TestAssert "param indirect number - depth 1", tVar[1] is 100

   put empty into tVar
   put 100 into tVar[1][1]
   TestAssert "param indirect number - depth 2", tVar[1][1] is 100

   put empty into tIndex
   put 1 into tIndex[1]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "param indirect number array - depth 1", tVar[tIndex] is 100

   put empty into tIndex
   put 1 into tIndex[1]
   put 1 into tIndex[2]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "param indirect number array - depth 2", tVar[tIndex] is 100

   put empty into tIndex
   put "one" into tIndex[1]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "param indirect name array - depth 1", tVar[tIndex] is 100

   put empty into tIndex
   put "one" into tIndex[1]
   put "one" into tIndex[2]
   put empty into tVar
   put 100 into tVar[tIndex]
   TestAssert "param indirect name array - depth 2", tVar[tIndex] is 100

   put empty into tIndex
   put "1" into tIndex[1]
   put "one" into tIndex[2]
   put empty into tVar
   put 100 into tVar[1][tIndex]["one"]
   TestAssert "param indirect mixed - depth 4", tVar[1][tIndex]["one"] is 100
end DoTestFetchRefParam

on TestFetchRefParam
   local tParam
   DoTestFetchRefParam tParam
end TestFetchRefParam

on TestFetchScriptLocal
end TestFetchScriptLocal

on TestFetchGlobal
end TestFetchGlobal
