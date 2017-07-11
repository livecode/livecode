script "CoreFieldGeneral"
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

on TestSetVisibleOfLine
    create stack
    create field

    set the visible of line 1 of field 1 to true

    TestAssert "getter works", the visible of line 1 of field 1 is true
    TestAssert "correct value", the visible of line 1 of field 1 is not the invisible of line 1 of field 1

    set the visible of line 1 of field 1 to false

    TestAssert "getter works", the visible of line 1 of field 1 is false
    TestAssert "correct value", the visible of line 1 of field 1 is not the invisible of line 1 of field 1
end TestSetVisibleOfLine