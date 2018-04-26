script "CoreInterfaceSubwindow"
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

on TestSubwindowPropagatesErrors
	create stack "TestStack"
	set the script of stack "TestStack" to "on openStack; throw red; end openStack"
	try
		palette stack "TestStack"
	catch tError
	end try
	TestAssert "Error during subwindow propagates to caller", tError is "red"
end TestSubwindowPropagatesErrors
