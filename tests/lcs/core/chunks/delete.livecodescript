script "CoreChunksDelete"
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

on TestDeleteVar
	create field
	put "a" & return & "b" into field 1
	
	local tToDelete
	put "line 1 of field 1" into tToDelete
	
	delete tToDelete
	
	TestAssert "delete obj text chunk in variable", field 1 is "b"
	
	delete tToDelete
	
	-- ensure left-over components from chunk evaluation are not re-used
	-- See bug 11928
	TestAssert "delete obj text chunk in variable again", field 1 is empty
end TestDeleteVar



