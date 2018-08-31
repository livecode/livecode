script "CoreEngineWidgets"
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

//////////

on TestSetup
   TestSkipIfNot "lcb"
   
   TestLoadAuxiliaryExtension "_widget"
   TestLoadAuxiliaryExtension "_widget_support"
   TestLoadAuxiliaryExtension "_widgetthrow"
end TestSetup

//////////

on TestWidgetBindErrorsDontEscape
   create stack "WidgetBindErrorTest"
   set the defaultStack to "WidgetBindErrorTest"

   local tArray
   put "foo" into tArray["$kind"]
   put empty into tArray["$state"]
   import widget from array tArray

   local tErrorIsPending
   try
      put TestWidget_MCErrorIsPending() into tErrorIsPending
   catch tError
      put true into tErrorIsPending
   end try

   TestAssert "no error lingers after widget bind failure", not tErrorIsPending

   delete stack "WidgetBindErrorTest"
end TestWidgetBindErrorsDontEscape

//////////

private command _DoTestWidgetScriptObjectAccess pMode
   set the testMode of widget "Test" to pMode

   local tVar
   export widget "Test" to array tVar

   TestAssertErrorDialog "no script access allowed for" && pMode, \ 
       "EE_EXTENSION_ERROR_DOMAIN"
end _DoTestWidgetScriptObjectAccess

on TestWidgetScriptObjectAccess
   create stack "WidgetScriptAccessTest"
   set the defaultStack to "WidgetScriptAccessTest"

   create widget "Test" as "com.livecode.lcs_tests.core.widget"
   set the behavior of it to the long id of me

   repeat for each item tMode in "resolve,get,set,send,post,execute"
      _DoTestWidgetScriptObjectAccess tMode
   end repeat

   delete stack "WidgetScriptAccessTest"
end TestWidgetScriptObjectAccess

on TestWidgetThrowInOnCreate
   create stack "WidgetThrowOnSaveTest"
   set the defaultStack to "WidgetThrowOnSaveTest"
   
   create widget "Test" as "com.livecode.lcs_tests.core.widgetthrow"

   TestAssertErrorDialog "error dialog when widget throws in OnCreate", \
      "EE_EXTENSION_ERROR_DOMAIN"

   delete stack "WidgetThrowOnSaveTest"
end TestWidgetThrowInOnCreate

on TestWidgetRetainRep
   create stack "WidgetRetainRepTest"
   set the defaultStack to "WidgetRetainRepTest"

   local tWidget
   put "com.livecode.lcs_tests.core.widget_save" into tWidget["$kind"]
   put uuid() into tWidget["$state"]["info"]
   import widget from array tWidget

   local tOut
   export widget 1 to array tOut
   TestAssert "Rep retained in export on failed bind", tOut["$state"]["info"] is tWidget["$state"]["info"]
   
   local tPath
   put the tempName into tPath
   save stack "WidgetRetainRepTest" as tPath
   delete stack "WidgetRetainRepTest"
   
   TestLoadAuxiliaryExtension "_widget_save"
   
   go stack tPath
   
   local tInfo
   put the savedInfo of widget 1 of stack "WidgetRetainRepTest" into tInfo
   TestAssert "Rep retained in save on failed bind", tInfo is tWidget["$state"]["info"]
   
   delete stack "WidgetRetainRepTest"
   delete file tPath
end TestWidgetRetainRep

on TestWidgetEnginePropertyName
   create stack "WidgetEnginePropertyNameTest"
   set the defaultStack to "WidgetEnginePropertyNameTest"
   
   create widget "Test" as "com.livecode.lcs_tests.core.widget"

   local tWidget
   put it into tWidget

   set the autoHilite of tWidget to "test"
   TestAssert "engine property name round trip", the autoHilite of tWidget is "test"

   TestAssert "engine property is not renamed to first alpha", \
         "autoHilight" is not among the lines of the customKeys of tWidget

   TestAssertThrow "engine property not implemented throws", \
         "__TestEnginePropertyNameNotFound", \
         the long id of me, \
         "EE_OBJECT_SETNOPROP"
end TestWidgetEnginePropertyName

on __TestEnginePropertyNameNotFound
   set the radioBehavior of widget "test" of stack "WidgetEnginePropertyNameTest" to true
end __TestEnginePropertyNameNotFound

