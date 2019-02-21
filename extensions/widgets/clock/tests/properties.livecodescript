script "ClockWidgetProperties"
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

on TestSetup
	TestLoadExtension "com.livecode.library.widgetutils"
	TestLoadExtension "com.livecode.widget.clock"
	create widget "testClock" as "com.livecode.widget.clock"
end TestSetup

on TestTeardown
	delete widget "testClock"
end TestTeardown

on TestCreateClock
	TestAssert "created clock exists", exists(widget "testClock")
end TestCreateClock

on TestGetDefautTimeZone
   TestAssert "timeZone on creation is empty, i.e. local time", the timeZone of widget "testClock" is ""
end TestGetDefautTimeZone

on TestSettingLocalTimeZone
   set the timeZone of widget "testClock" to 0
 	set the timeZone of widget "testClock" to ""
	TestAssert "timeZone successfully set to local", the timeZone of widget "testClock" is ""
end TestSettingLocalTimeZone

on TestGetDefaultFaceColor
   TestAssert "dayFaceColor on creation is 224,224,224,64", the dayFaceColor of widget "testClock" is "224,224,224,64"
   TestAssert "nightFaceColor on creation is black", the nightFaceColor of widget "testClock" is "0,0,0,255"
end TestGetDefaultFaceColor

on TestSettingFaceColor
   set the dayFaceColor of widget "testClock" to "0,255,0,255"
   TestAssert "dayFaceColor sucessfully set to non-default", the dayFaceColor of widget "testClock" is "0,255,0,255"
   set the nightFaceColor of widget "testClock" to "0,0,255,255"
   TestAssert "nightFaceColor sucessfully set to non-default", the nightFaceColor of widget "testClock" is "0,0,255,255"
end TestSettingFaceColor

on TestDefaultSecondHandColor
   TestAssert "daySecondHandColor on creation is red", the daySecondHandColor of widget "testClock" is "255,0,0,255"
   TestAssert "nightSecondHandColor on creation is red", the nightSecondHandColor of widget "testClock" is "255,0,0,255"
end TestDefaultSecondHandColor

on TestSettingSecondHandColor
   set the daySecondHandColor of widget "testClock" to "0,255,0,255"
   TestAssert "daySecondHandColor sucessfully set to non-default", the daySecondHandColor of widget "testClock" is "0,255,0,255"
   set the nightSecondHandColor of widget "testClock" to "0,0,255,255"
   TestAssert "nightSecondHandColor sucessfully set to non-default", the nightSecondHandColor of widget "testClock" is "0,0,255,255"
end TestSettingSecondHandColor

on TestDefaultMinuteHandColor
   TestAssert "dayMinuteHandColor on creation is black", the dayMinuteHandColor of widget "testClock" is "0,0,0,255"
   TestAssert "nightMinuteHandColor on creation is white", the nightMinuteHandColor of widget "testClock" is "255,255,255,255"
end TestDefaultMinuteHandColor

on TestSettingMinuteHandColor
   set the dayMinuteHandColor of widget "testClock" to "0,255,0,255"
   TestAssert "dayMinuteHandColor sucessfully set to non-default", the dayMinuteHandColor of widget "testClock" is "0,255,0,255"
   set the nightMinuteHandColor of widget "testClock" to "0,0,255,255"
   TestAssert "nightMinuteHandColor sucessfully set to non-default", the nightMinuteHandColor of widget "testClock" is "0,0,255,255"
end TestSettingMinuteHandColor

on TestDefaultHourHandColor
   TestAssert "dayHourHandColor on creation is black", the dayHourHandColor of widget "testClock" is "0,0,0,255"
   TestAssert "nightHourHandColor on creation is white", the nightHourHandColor of widget "testClock" is "255,255,255,255"
end TestDefaultHourHandColor

on TestSettingHourHandColor
   set the dayHourHandColor of widget "testClock" to "0,255,0,255"
   TestAssert "dayHourHandColor sucessfully set to non-default", the dayHourHandColor of widget "testClock" is "0,255,0,255"
   set the nightHourHandColor of widget "testClock" to "0,0,255,255"
   TestAssert "nightHourHandColor sucessfully set to non-default", the nightHourHandColor of widget "testClock" is "0,0,255,255"
end TestSettingHourHandColor

on TestDefaultNumberColor
   TestAssert "dayNumberColor on creation is black", the dayNumberColor of widget "testClock" is "0,0,0,255"
   TestAssert "nightNumberColor on creation is white", the nightNumberColor of widget "testClock" is "255,255,255,255"
end TestDefaultNumberColor

on TestSettingNumberColor
   set the dayNumberColor of widget "testClock" to "0,255,0,255"
   TestAssert "dayNumberColor sucessfully set to non-default", the dayNumberColor of widget "testClock" is "0,255,0,255"
   set the nightNumberColor of widget "testClock" to "0,0,255,255"
   TestAssert "nightNumberColor sucessfully set to non-default", the nightNumberColor of widget "testClock" is "0,0,255,255"
end TestSettingNumberColor

on TestSetReadOnlyPropOfWidget

   local tName
   put the name of widget "testClock" into tName
   TestAssertThrow "set read-only prop", \
         "_TestSetReadOnlyPropOfWidgetError", the long id of me, \ 
         "EE_PROPERTY_CANTSETOBJECT", tName
         
end TestSetReadOnlyPropOfWidget

command _TestSetReadOnlyPropOfWidgetError pWidget
   set the isDay of pWidget to true
end _TestSetReadOnlyPropOfWidgetError
