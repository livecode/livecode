script "GraphWidgetProperties"
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

local sGraphID

private function getGraphData
	local tData
	put "0,50,100" into tData
	put return & "1,100,200" after tData
	return tData
end getGraphData

on TestSetup
	TestLoadExtension "com.livecode.library.widgetutils"
	TestLoadExtension "com.livecode.library.iconsvg"
	TestLoadExtension "com.livecode.widget.linegraph"	
	
	create stack
	create widget as "com.livecode.widget.linegraph"
	
	put it into sGraphID
end TestSetup

on TestHilitedCoordinates
	set the hilitedCoordinates of sGraphID to "wed,2000"
	
	TestAssert "set hilited coordinates discrete x", true
	
	set the hilitedCoordinates of sGraphID to empty
	
	TestAssert "set hilited coordinates to empty", true
		
	set the graphData of sGraphID to getGraphData()
	set the hilitedCoordinates of sGraphID to "0.5,150"
	
	TestAssert "set hilited coordinates interpolate x value", true
end TestHilitedCoordinates

-- Bug 17692
on _TestEmptyGraphData
	TestDiagnostic "SetEmptyGraphData"
	set the graphData of sGraphID to empty
end _TestEmptyGraphData
on TestEmptyGraphData
	TestAssertThrow "set graph data to empty", "_TestEmptyGraphData", \
		the long id of me, "EE_EXTENSION_ERROR_DOMAIN"
end TestEmptyGraphData
