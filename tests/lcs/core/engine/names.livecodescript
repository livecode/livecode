script "CoreEngineNames"
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

private command _TestObjectIsObjectName pType
	local tObj
	switch pType
		case "widget"
			create widget "test" as ""
			break
		default
			do "create" && pType && quote & "test" & quote
			break
	end switch
	put it into tObj
	
	-- force obj resolution to use MCU_matchname
	local tName, tResult
	put the name of tObj into tName
	do "get there is a" && pType && "tName"
	put it into tResult
	
	TestAssert pType && "<name> is" && pType && "<name property of" && pType && ">", tResult
end _TestObjectIsObjectName

on TestObjectIsObjectName
	repeat for each item tObjType in "stack,card,group,button,scrollbar,image,graphic,eps,widget,field"
		_TestObjectIsObjectName tObjType
	end repeat
end TestObjectIsObjectName
