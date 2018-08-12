script "CoreInterfaceRelayer"
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

on TestSetLayerToTopAndBottom
    local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack
	
	local tButton
	create button
	put it into tButton
	
	local tField
	create field
	put it into tField
	
	set the layer of tField to bottom
	
	TestAssert "set layer to bottom", the layer of tField is 1
		
	set the layer of tField to top
	
	TestAssert "set layer to top", the layer of tField is (the number of controls)
	
end TestSetLayerToTopAndBottom

on TestSetLayerToLayerOfControl
    local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack
	
	create graphic
	
	local tButton
	create button
	put it into tButton
	
	local tField
	create field
	put it into tField
	
	local tButtonLayer
	put the layer of tButton into tButtonLayer
	set the layer of tField to tButtonLayer
	
	TestAssert "set layer to layer of control", the layer of tField is tButtonLayer
end TestSetLayerToLayerOfControl

on TestSetLayerToLayerOfGroup
    local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack
	
	local tGroup
	create group
	put it into tGroup
	
	local tField
	create field
	put it into tField

	set the relayerGroupedControls to false	
	set the layer of tField to the layer of tGroup
	
	// Replaces group in layers, so should end up on bottom layer where
	// group was
	TestAssert "set layer to layer of group with relayer grouped true - " \
		& "layer value", the layer of tField is (the layer of tGroup - 1)
	TestAssert "set layer to layer of group with relayer grouped true - " \
		& "field owner", the long id of the owner of tField is the long \
		id of this card of tStack
	
	set the relayerGroupedControls to true
	set the layer of tField to the layer of tGroup
	
	// Setting to layer of target group relayers into group as bottom
	// control in group 
	TestAssert "set layer to layer of group with relayer grouped true - " \
		& "layer value", the layer of tField is (the layer of tGroup) + 1
	TestAssert "set layer to layer of group with relayer grouped true - " \
		& "field owner", the long id of the owner of tField is the long \
		id of tGroup
end TestSetLayerToLayerOfGroup

on TestSetLayerToLayerOfGroupedControl
    local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack
	
	local tGroup
	create group
	put it into tGroup
	
	local tButton
	create button in tGroup
	put it into tButton
	
	local tField
	create field
	put it into tField
	
	set the relayerGroupedControls to false
	set the layer of tField to the layer of tButton
	
	TestAssert "set layer to layer of grouped control with relayer " \
		& "grouped false - layer value", the layer of tField is (the \ 
		layer of tButton) + 1
	TestAssert "set layer to layer of grouped control with relayer " \
		& "grouped false - field owner", the long id of the owner of tField is the long \
		id of this card of tStack
	
	set the relayerGroupedControls to true
	set the layer of tField to the layer of tButton
	
	TestAssert "set layer to layer of grouped control with relayer " \
		& "grouped true - layer value", the layer of tField is (the \ 
		layer of tButton) - 1
	TestAssert "set layer to layer of grouped control with relayer " \
		& "grouped true - field owner", the long id of the owner of tField is the long \
		id of tGroup
end TestSetLayerToLayerOfGroupedControl

on TestSetLayerToLayerOfGroupedControlNotFirst
    local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack

	create graphic
	
	local tGroup
	create group
	put it into tGroup

	create button in tGroup

	// Create second control in group
	local tButton
	create button in tGroup
	put it into tButton
	
	local tField
	create field
	put it into tField
	
	set the relayerGroupedControls to true
	set the layer of tField to the layer of tButton
	
	TestAssert "set layer to layer of second grouped control" \
		& " with relayer grouped true - layer value", \ 
		the layer of tField is the layer of tButton - 1
	TestAssert "sset layer to layer of second grouped control" \
		& " with relayer grouped true - field owner", \ 
		the long id of the owner of tField is tGroup
end TestSetLayerToLayerOfGroupedControlNotFirst

// The following tests are needed as a result of the code that implements
// this behavior: http://quality.livecode.com/show_bug.cgi?id=19455
// Should be removed when bug 19455 is resolved.
on TestSetLayerToLayerOfGroupedControlAnomaly
	repeat for each item tDesc in "on bottom layer,not on bottom layer"
		_TestSetLayerToLayerOfGroupedControlAnomaly tDesc
	end repeat
end TestSetLayerToLayerOfGroupedControlAnomaly

private command _TestSetLayerToLayerOfGroupedControlAnomaly pDesc
    local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack

	// A distinct code path is taken in the case where the layer of a
	// control is set to the layer of any control in a group except the 
	// first, where the outer group is on the bottom layer.
	if pDesc is not "on bottom layer" then
		create graphic
	end if
	
	local tGroup
	create group
	put it into tGroup

	create button in tGroup

	// Create second control in group
	local tButton
	create button in tGroup
	put it into tButton
	
	local tField
	create field
	put it into tField
	
	set the relayerGroupedControls to false
	set the layer of tField to the layer of tButton
	
	TestAssert "set layer to layer of second grouped control of outer group " \
		& pDesc & " with relayer grouped false - layer value", \ 
		the layer of tField is the layer of tGroup - 1
	TestAssert "set layer to layer of second grouped control of outer group " \
		& pDesc & " with relayer grouped false - field owner", \ 
		the long id of the owner of tField is the long id of this card of tStack
end _TestSetLayerToLayerOfGroupedControlAnomaly

command _TestSetLayerOfGroupedControlError pControl
	set the relayerGroupedControls to false
	set the layer of pControl to "bottom"
end _TestSetLayerOfGroupedControlError

on TestSetLayerOfGroupedControl 
    local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack
	
	local tGroup
	create group
	put it into tGroup
	
	local tButton
	create button in tGroup
	put it into tButton
	
	TestAssertThrow "set layer of grouped control with relayerGroupedControls false", \
		"_TestSetLayerOfGroupedControlError", the long id of me, \ 
		"EE_OBJECT_BADRELAYER", tButton
		
	set the relayerGroupedControls to true
	set the layer of tButton to "bottom"
	-- Long id of button will change
	
	TestAssert "set layer of grouped control with relayerGroupedControls true " \ 
		& "- layer value", the layer of button 1 is 1
		
	TestAssert "set layer of grouped control with relayerGroupedControls true " \ 
		& "- owner", the long id of the owner of button 1 is the long id of \ 
		this card of tStack
end TestSetLayerOfGroupedControl

on TestSetLayerOfControlOutOfRange
    local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack
	
	local tButton
	create button
	put it into tButton
	
	local tField
	create field
	put it into tField
	
	set the layer of tButton to 10000
	
	TestAssert "layer property clamped to feasible values", \
		the layer of tButton is (the layer of tField) + 1
end TestSetLayerOfControlOutOfRange
 