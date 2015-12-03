script "TreeViewElementChunk"
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

on TestSetup
	TestLoadExtension "com.livecode.widget-utils"
	TestLoadExtension "com.livecode.widget.treeView"
end TestSetup

private function getTestArray
	local tArray
	put "a" into tArray[1]
	put "b" into tArray[2][1]
	put "c" into tArray[3][1][1]
	return tArray
end getTestArray

on TestSetElementEmpty
	create widget "Tree View" as "com.livecode.widget.treeView"
	set the arrayData of element 1 of widget "Tree View" to "a"
	
	get the arrayData of widget "Tree View"
	
	TestAssert "set element of previously empty tree view", it[1] is "a"
end TestSetElementEmpty

on TestGetElementEmpty
	create widget "Tree View" as "com.livecode.widget.treeView"
	
	TestDiagnostic "ArrayData is" &&  the arrayData of element 1 of widget "Tree View"
	
	TestAssert "get element of empty tree view", the arrayData of element 1 of widget "Tree View" is empty
end TestGetElementEmpty

on TestGetElement
	create widget "Tree View" as "com.livecode.widget.treeView"
	
	local tArray
	put getTestArray() into tArray
	set the arrayData of widget "Tree View" to tArray
	
	local tValue
	put the arrayData of element 1 of widget "Tree View" into tValue
	TestAssert "get element of tree view", tValue is tArray[1]
	
	put the arrayData of element 1 of element 2 of widget "Tree View" into tValue
	TestAssert "get nested element of tree view", tValue is tArray[2][1]
	
	local tPath
	put "3" into tPath[1]
	put "1" into tPath[2]
	put "1" into tPath[3]
	
	put the arrayData of element tPath of widget "Tree View" into tValue
	TestAssert "get path element of tree view", tValue is tArray[3][1][1]
	
	local tPath1, tPath2
	put "1" into tPath1[1]
	put "2" into tPath2[1]
	put the arrayData of element tPath1 of element tPath2 of widget "Tree View" into tValue
	TestAssert "get nested path element of tree view", tValue is tArray[2][1]	
	
	put the arrayData of element tPath1 of element "2" of widget "Tree View" into tValue
	TestAssert "get nested mixed element of tree view", tValue is tArray[2][1]
end TestGetElement

on TestSetElement
	create widget "Tree View" as "com.livecode.widget.treeView"
	
	local tArray
	put getTestArray() into tArray
	set the arrayData of widget "Tree View" to tArray
	
	set the arrayData of element 1 of widget "Tree View" to "value1"
	put the arrayData of widget "Tree View" into tArray
	TestAssert "set element of tree view", tArray[1] is "value1"
	
	set the arrayData of element 1 of element 2 of widget "Tree View" to "value2"
	put the arrayData of widget "Tree View" into tArray
	TestAssert "set nested element of tree view", tArray[2][1] is "value2"
	
	local tPath
	put "3" into tPath[1]
	put "1" into tPath[2]
	put "1" into tPath[3]
	
	set the arrayData of element tPath of widget "Tree View" to "value3"
	put the arrayData of widget "Tree View" into tArray
	TestAssert "get path element of tree view", tArray[3][1][1] is "value3"
	
	local tPath1, tPath2
	put "1" into tPath1[1]
	put "2" into tPath2[1]
	set the arrayData of element tPath1 of element tPath2 of widget "Tree View" to "value4"
	put the arrayData of widget "Tree View" into tArray
	TestAssert "get nested path element of tree view", tArray[2][1]	is "value4"
	
	set the arrayData of element tPath1 of element "2" of widget "Tree View" to "value5"
	put the arrayData of widget "Tree View" into tArray
	TestAssert "get nested mixed element of tree view", tArray[2][1] is "value5"

end TestSetElement
