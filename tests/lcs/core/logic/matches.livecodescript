script "CoreLogicMatches"
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

on TestMatchesWildcard
	TestAssert "wildcard pattern matches", "foobar" matches wildcard pattern "*b*"
	TestAssert "wildcard pattern does not match", not ("foobar" matches wildcard pattern "*z*")
end TestMatchesWildcard

constant kEmailRegex = "^[a-zA-Z0-9.!#$%&'*+\/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$"

on TestMatchesRegex
	TestAssert "regex pattern matches", "support@livecode.com" matches regex pattern kEmailRegex
	TestAssert "regex pattern does not match", not ("support dot livecode dot com" matches regex pattern kEmailRegex)
end TestMatchesRegex
