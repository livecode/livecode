script "DateTime"
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


constant kTwelveHours = 43200

on TestLongTime24Hours
 local tSeconds
 set the twelveHourTime to false
 put the seconds into tSeconds
 convert tSeconds to long time

 TestAssert "Test 24-hour format 1", char 1 to 2 of tSeconds is a number
 TestAssert "Test 24-hour format 2", char 3 of tSeconds is ":"
 
 put the seconds + kTwelveHours into tSeconds
 convert tSeconds to long time
 
 TestAssert "Test 24-hour format 3", char 1 to 2 of tSeconds is a number
 TestAssert "Test 24-hour format 4", char 3 of tSeconds is ":"
end TestLongTime24Hours

