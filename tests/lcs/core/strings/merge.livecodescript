script "CoreStringMerge"
/*
Copyright (C) 2015 LiveCode Ltd.

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

function foo
   return 1 + "string"
end foo

function bar
    return "string"
end bar

on _TestMergeShouldThrowHandler
    local s
    put "This is [[ foo() ]] a test" into s
    put merge(s)
end _TestMergeShouldThrowHandler

on TestMergeShouldThrow
    TestAssertThrow "calling the handler should throw", "_TestMergeShouldThrowHandler",\
        the long id of me, "EE_PLUS_BADRIGHT"
end TestMergeShouldThrow

on TestMergeShouldNotThrow
    local s
    put "This is [[ bar() ]] a test" into s
    put merge(s)
end TestMergeShouldNotThrow