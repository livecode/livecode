script "CoreExecutionUQL"
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

on TestUQLPut
	put "foo" into tTest1
	TestAssert "unquoted literal put into", tTest1 is "foo"
	put "foo" after tTest2
	TestAssert "unquoted literal put after", tTest2 is "foo"
	put "foo" before tTest3
	TestAssert "unquoted literal put before", tTest3 is "foo"
end TestUQLPut

on TestUQLValue
	TestAssert "unquoted literal value", tTest1 is "tTest1"	
end TestUQLValue

on TestUQLMatchText
	put "rangeInt={0,3}" into tVar
	get matchText(tVar,"(.*?)={(.*?)}", tOption, tValue)
	TestAssert "matchText assigns correct values to UQL", \
			tOption is "rangeInt" and tValue is "0,3"
end TestUQLMatchText

on TestUQLBinaryDecode
	get binaryDecode("h*",sha1Digest("foobar"), tFoobar)
	TestAssert "binaryDecode assigns correct values to UQL", \
			tFoobar is "88347d9f426112d19ebe9b36ffc42e1852398287"
end TestUQLBinaryDecode

on TestUQLQueryRegistry
	TestSkipIfNot "platform", "win32"
	get queryRegistry("HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\ProgramFilesDir", tType)
	TestAssert "queryRegistry assigns correct values to UQL", \
			tType is "string"
end TestUQLQueryRegistry
