script "DevelopmentPropertyChanged"
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

local sTarget

on idePropertyChanged pTarget
	put pTarget into sTarget
end idePropertyChanged

private command _TestPropertyChanged pObjType, pWhere
   create stack
   set the defaultStack to the short name of it
   	
   local tObj
   switch pObjType
      case "field"
         create field
         put it into tObj
         break
      case "button"
         create button
         put it into tObj
         set the menumode of it to "combobox"
         break
   end switch
   
   revIDESubscribe "idePropertyChanged", tObj
   put empty into sTarget
   
   switch pObjType
      case "field"
         if pWhere is "into" then
            put "some text" into field 1
         else if pWhere is "after" then
            put "some text" after field 1
         end if
         break
      case "button"
         put empty into sTarget
         if pWhere is "into" then
            put "some text" into button 1
         else if pWhere is "after" then
            put "some text" after button 1
         end if
         break
   end switch
   
   local tParams
   put tObj into tParams[1]
   TestRepeat "put" && pWhere && pObjType && \ 
         "triggers propertyChanged", IsTargetCorrect, \
         the long id of me, 10, tParams
   
   revIDEUnsubscribe "idePropertyChanged", "", tObj
end _TestPropertyChanged

command IsTargetCorrect pObj
   return pObj is sTarget
end IsTargetCorrect

on TestPutPropertyChanged
	repeat for each item tWhere in "into,after"
		repeat for each item tObjType in "field,button"
			_TestPropertyChanged tObjType, tWhere
		end repeat
	end repeat
end TestPutPropertyChanged

on TestWidgetPropertyChanged
   TestSkipIfNot "lcb"
   TestLoadAuxiliaryExtension "_widget_propertychanged"
   
   create stack
   set the defaultStack to the short name of it
   local tObj
   create widget as "com.livecode.idesupport_tests.development.widget_propertychanged"
   put it into tObj

   revIDESubscribe "idePropertyChanged", tObj
   put empty into sTarget
   
   local tOut
   -- export causes OnSave handler to be run
   -- our widget OnSave handler calls `trigger all`
   export widget 1 to array tOut
   
   local tParams
   put tObj into tParams[1]
   TestRepeat "trigger all triggers propertyChanged", IsTargetCorrect, \
         the long id of me, 10, tParams
   
   revIDEUnsubscribe "idePropertyChanged", "", tObj
end TestWidgetPropertyChanged