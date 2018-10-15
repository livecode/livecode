script "CoreEngineGlobalProperties"
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

on _TestSetReadOnlyProperty
	set the screenPixelScale to 2
end _TestSetReadOnlyProperty

on TestSetReadOnlyProperty
	TestAssertThrow "setting read-only global property throws", \
		 "_TestSetReadOnlyProperty", the long id of me, \
		 "EE_PROPERTY_CANTSET"
end TestSetReadOnlyProperty

on TestGetSetSecurityPermissions
   TestAssert "Getting default securityPermissions", the securityPermissions is the securityCategories
		
	TestSkipIfNot "securityPermissions", "set"
	
   local tNewPermissions
   put "disk,network" into tNewPermissions
   set the securityPermissions to tNewPermissions
   TestAssert "Getting new securityPermissions correctly", the securityPermissions is tNewPermissions
end TestGetSetSecurityPermissions

on TestDeletingMenubarGroupDereferencesProperly
    create group
    set the menubar to it
    set the defaultmenubar to it
    delete it
    TestAssert "did not crash", true
end TestDeletingMenubarGroupDereferencesProperly
