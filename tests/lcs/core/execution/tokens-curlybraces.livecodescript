script "CoreExecutionTokensCurlyBraces"
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

on TestCurlyBraces
   local tResult
   create stack "Test1"
   
   set the script of stack "Test1" to "on openStack;local tArray;put true into tArray{1};end openStack"
   TestAssert "Curly braces can't be used for array bracket syntax", the result is not empty
   
   set the script of stack "Test1" to "on openStack;set the cProperty{1} of stack " & quote & "Test1" & quote &";end openStack"
   TestAssert "Curly braces can't be used for custom property bracket syntax", the result is not empty
   
   set the script of stack "Test1" to "on openStack;local tVar{};put true into tVar{};end openStack"
   TestAssert "Curly braces can't be used for identifiers", the result is not empty
   
   delete stack "Test1"
end TestCurlyBraces
