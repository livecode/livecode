script "CoreInterfaceRevert"
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

on TestRevertTopStack
	local tTempName
	put the tempName into tTempName
	
	local tStack
	create stack
	put it into tStack
	set the filename of tStack to tTempName
	save tStack
	
	create button
	revert
	
	TestAssert "revert the topStack", there is not a button 1 of tStack
end TestRevertTopStack

on TestRevertTargetStack
	local tTempName
	put the tempName into tTempName
	
	local tStack
	create stack
	put it into tStack
	set the filename of tStack to tTempName
	save tStack
	
	create button
	create stack
	revert tStack
	
	TestAssert "revert target stack", there is not a button 1 of tStack
end TestRevertTargetStack
