script "CoreEngineMessageBox"
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

local sTarget, sHandler, sLine, sMsg

on TestMessageBox
	put "bar"
	__CheckMessage "Put into", "bar", 21
	put "foo" before msg
	__CheckMessage "Put before", "foobar", 23
	put "baz" after msg
	__CheckMessage "Put after", "foobarbaz", 25
end TestMessageBox

on msgChanged pHandler, pLine
	put the long id of the target into sTarget
	put pHandler into sHandler
	put pLine into sLine
	put msg into sMsg
end msgChanged

on __CheckMessage pTest, pMsg, pLine
	TestDiagnostic "target:" && sTarget
	TestAssert pTest && "target is correct", sTarget is the long id of me
	TestAssert pTest && "handler is correct", sHandler is "TestMessageBox"
	TestAssert pTest && "line is correct", sLine is pLine
	TestAssert pTest && "msg is correct", sMsg is pMsg
	put empty into sTarget
	put empty into sHandler
	put empty into sLine
	put empty into sMsg
end __CheckMessage 
