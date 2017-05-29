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
   TestLoadAuxillaryExtension "_widget"
   TestLoadAuxillaryExtension "_widget_support"
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
