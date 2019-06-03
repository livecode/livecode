script "CoreEngineResources"
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

on TestSetup
   TestSkipIfNot "lcb"
end TestSetup

on TestModuleWithResourcesFolder
    set the itemdelimiter to slash
    local tFolder
    put item 1 to -2 of the effective filename of me into tFolder

    TestLoadAuxiliaryExtension "_resources", tFolder

    TestAssert "module with resources folder returns non-nothing", \
		        ResourcesGetMyResourcesFolder() is not empty
end TestModuleWithResourcesFolder

on TestModuleWithoutResourcesFolder
    TestLoadAuxiliaryExtension "_resources"
    
    TestAssert "module without resources folder returns nothing", \
		        ResourcesGetMyResourcesFolder() is empty
end TestModuleWithoutResourcesFolder
