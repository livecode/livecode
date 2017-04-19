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
