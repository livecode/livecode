script "CoreInterfaceDelete"
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

on TestDeleteStackWithSubstack
	local tMainstack
	create stack
	put the short name of it into tMainstack
	
	local tScript
	put "on closeStack" into tScript
	put return & "local tTarget, tStack" after tScript
	put return & "put the long id of the target into tTarget" after tScript
	put return & "get wordOffset(" & quote & "stack" & quote & ", tTarget)" after tScript
	put return & "put word it to (it + 1) of tTarget into tStack" after tScript
	put return & "get the mainstack of stack tStack" after tScript
	put return & "end closeStack" after tScript
	
	create stack
	set the script of card 1 of it to tScript
	set the mainstack of it to tMainstack

	delete stack tMainstack
	
	TestAssert "delete stack with substack no crash", true
end TestDeleteStackWithSubstack

on TestDeleteSelectedObjects
	create stack
	set the defaultStack to the short name of it
	
	create button
	create field
	select button 1 and field 1
	
	delete
	
	TestAssert "delete selected objects", the number of controls is 0
end TestDeleteSelectedObjects

local sGroup

on TestDeleteSelectedCantDelete
	create stack
	set the defaultStack to the short name of it
	
	create button
	
	create group
	put it into sGroup
	
	create field in sGroup
	set the cantDelete of sGroup to true
	
	select button 1
	delete
	
	TestAssertThrow "can't delete group with cantDelete true", "__TestDeleteGroup", \
      the long id me, "EE_OBJECT_CANTREMOVE"
	
	local tThereIsAGroup
	put there is a sGroup into tThereIsAGroup
		
	undo
	
	TestAssert "undo previous delete after failed delete", there is a button 1
end TestDeleteSelectedCantDelete

on __TestDeleteGroup
	select sGroup
	delete
end __TestDeleteGroup

on TestUndoAfterDeleteNothing
	create stack
	set the defaultStack to the short name of it
	
	create button
	select button 1
	delete

	select empty
	delete
	undo
	
	TestAssert "undo previous delete after failed delete", there is a button 1
end TestUndoAfterDeleteNothing

on TestDeleteComboBox
	create stack
	set the defaultStack to the short name of it
	
	create button
	set the style of it to "menu"
	set the menumode of it to "combobox"
	select it
	delete
	undo
		
	TestAssert "undo delete of combo box", there is a button 1
end TestDeleteComboBox

on TestDeleteUndoChanged
	create stack
	set the defaultStack to the short name of it
	set the script of this card to \ 
		"on undoChanged; set the cCardReceived of this stack to true; end undoChanged"
	
	create button
	set the script of it to \ 
		"on undoChanged; set the cCardReceived of this stack to false; end undoChanged"
	select it
	delete

	TestAssert "undoChanged sent to current card", the cCardReceived of this stack
end TestDeleteUndoChanged