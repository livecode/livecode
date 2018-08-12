script "CoreFieldLayout"
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

on TestLastTabOnLineIsNotIgnored
    create stack "TestStack"
    set the defaultStack to "TestStack"
    create field "Test"

    set the text of field "Test" to "a" & numToChar(9)
    set the tabStops of field "Test" to 1000
    TestAssert "last tab on line is not ignored", the formattedWidth of line 1 of field "Test" is 1000
end TestLastTabOnLineIsNotIgnored
