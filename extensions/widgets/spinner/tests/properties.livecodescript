script "SpinnerWidgetProperties"
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

local sID

on TestSetup
	TestLoadExtension "com.livecode.widget.spinner"	
	
	create stack
	create widget as "com.livecode.widget.spinner"
	
	put it into sID
end TestSetup

on _TestOutOfRangeMarkerCount
	set the markerCount of sID to 4
end _TestOutOfRangeMarkerCount

on TestOutOfRangeMarkerCount
	TestAssertThrow "set markerCount to 4", "_TestOutOfRangeMarkerCount", \
		the long id of me, "EE_EXTENSION_ERROR_DOMAIN"
end TestOutOfRangeMarkerCount
