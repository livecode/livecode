script "CoreEngineObjects"
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


on TestCreateButton
   create button "test"
   TestAssert "button created", there is a button"test"
end TestCreateButton

on TestCreateCard
   create card "test"
   TestAssert "card created", there is a card"test"
end TestCreateCard

on TestCreateEPS
   create EPS "test"
   TestAssert "EPS created", there is a EPS"test"
end TestCreateEPS

on TestCreateField
   create field "test"
   TestAssert "field created", there is a field"test"
end TestCreateField

on TestCreateGraphic
   create graphic "test"
   TestAssert "graphic created", there is a graphic"test"
end TestCreateGraphic

on TestCreateGroup
   create group "test"
   TestAssert "group created", there is a group"test"
end TestCreateGroup

on TestCreateImage
   create image "test"
   TestAssert "image created", there is a image"test"
end TestCreateImage

on TestCreatePlayer
   create player "test"
   TestAssert "player created", there is a player"test"
end TestCreatePlayer

on TestCreateScrollbar
   create scrollbar "test"
   TestAssert "scrollbar created", there is a scrollbar"test"
end TestCreateScrollbar

on TestCreateStack
   create stack "test"
   TestAssert "stack created", there is a stack"test"
end TestCreateStack

constant kIsA_types = "array,boolean,color,date,integer,number,point,rect,ascii string"
local sValueToType

// TestAssert message cannot contain '#'
private function _fixSharp pMess
   replace "#" with "[sharp]" in pMess
   return pMess
end _fixSharp

private command __checkVariableType pVariable, pType
   repeat for each item tType in kIsA_types
      if tType is among the items of pType then
         TestAssert _fixSharp(merge("'Variable containing '[[pVariable]]' is a [[tType]]")), value("pVariable is a " & tType)
      else
         TestAssert _fixSharp(merge("Variable containing '[[pVariable]]' is not a [[tType]]")), not value("pVariable is a " & tType)
      end if
   end repeat
end __checkVariableType

on TestIsA
   // Array
   local tArray
   put "value" into tArray["key"]
   __checkVariableType tArray, "array"
   
   // Boolean
   __checkVariableType false, "boolean,ascii string"
   __checkVariableType true, "boolean,ascii string"
   
   // colour name
   __checkVariableType "red", "color,ascii string"
   
   // RGB colour
   __checkVariableType "123,123,123", "color,ascii string"
   
   // HTML colour
   __checkVariableType "#efefef", "color,ascii string"
   
   // Date
   __checkVariableType the date, "date,ascii string"
   
   // Positive integer
   __checkVariableType 42, "integer,number,date,color,ascii string"
   
   // Negative integer
   __checkVariableType -42, "integer,number,color,ascii string"
   
   // Positive real
   __checkVariableType 3.14, "number,date,ascii string"
   
   // Negative real
   __checkVariableType -3.14, "number,ascii string"
   
   // Positive scientific notation - integer
   __checkVariableType 3.14E2, "number,date,integer,ascii string"
   
   // Positive scientific notation - real
   __checkVariableType 3.141E2, "number,date,ascii string"
   
   // 0
   __checkVariableType 0, "integer,number,date,color,ascii string"
   
   // String as number
   __checkVariableType "42", "integer,number,color,date,ascii string"
   
   // Infinity
   local tInfinity
   put "infinity" into tInfinity
   __checkVariableType tInfinity, "number,integer,ascii string"
   
   // Point
   local tPoint
   put 12,23 into tPoint
   __checkVariableType tPoint, "point,ascii string"
   
   // Rect
   local tRect
   put 0,0,12,12 into tRect
   __checkVariableType tRect, "rect,ascii string"
   
   // ASCII string
   __checkVariableType "some text", "ascii string"
   
   // CharToNum(128)
   __checkVariableType numToChar(128), ""
   
   // Unicode string
   __checkVariableType "ыщьуерштп", ""
end TestIsA
