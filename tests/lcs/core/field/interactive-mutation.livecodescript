script "CoreFieldInteractiveMutation"
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

on TestReturnBeforeVTab
   TestSkipIfNot "wait"
   
    create stack "TestStack"
    set the defaultStack to "TestStack"
    create field "Test"
    set the text of field "Test" to "First" & numToChar(11) & "Second"
    select after char 5 of field "Test"
    type return
    TestAssert "insert return before vtab", the text of field "Test" is ("First" & return & numToChar(11) & "Second")
end TestReturnBeforeVTab
