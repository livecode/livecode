script "CoreFieldLineChunkHidden"
/*
Copyright (C) 2018 LiveCode Ltd.

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

on TestHiddenOfLineChunk
    create field "Test"
    put "" into field "Test"
    put "" into line 30 of field "Test"
    set the hidden of line 11 to 19 of field "Test" to true
    TestAssert "hidden property is correct", the hidden of line 19 of field "Test" is true
end TestHiddenOfLineChunk
