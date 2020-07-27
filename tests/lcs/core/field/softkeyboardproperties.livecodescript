script "CoreFieldSoftKeyboardProperties"
/*
Copyright (C) 2019 LiveCode Ltd.

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

on TestKeyboardTypeRoundTrip
	local tValues
	put ",default,numeric,url,number,phone,contact,email" into tValues
	split tValues by comma
	TestPropRoundTrip "field", "keyboardType", tValues
end TestKeyboardTypeRoundTrip

on TestReturnKeyTypeRoundTrip
	local tValues
	put ",default,go,google,join,next,route,search,send,yahoo,done,emergency call" into tValues
	split tValues by comma
	TestPropRoundTrip "field", "returnKeyType", tValues
end TestReturnKeyTypeRoundTrip

