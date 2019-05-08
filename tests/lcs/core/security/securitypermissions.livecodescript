script "CoreSecurityPermissions"
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

on TestSetup
	TestSkipIfNot "securityPermissions", "set"
end TestSetup

on TestSecurityPermissionExtension
	local tCategories
	put the securityCategories into tCategories
	TestAssert "default securityCategories contains extension", "extension" is among the items of tCategories

	filter items of tCategories without "extension"
	set the securityPermissions to tCategories
	TestAssert "extension can be removed from securityPermissions", the securityPermissions is tCategories

	local tError
	try
		TestLoadAuxiliaryExtension "_extension"
	catch tError
		-- TestLoadAuxiliaryExtension throws the result on failure
	end try
	TestAssert "load extension fails with no permission", tError is not empty
	
end TestSecurityPermissionExtension