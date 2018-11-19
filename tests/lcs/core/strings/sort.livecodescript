script "CoreStringSort"
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


on TestInterface95
   local tContainer
   
   put "b,d,f,g,c,h,e,a" into tContainer
   
   sort items of tContainer
   
   TestAssert "test", tContainer is "a,b,c,d,e,f,g,h" 
end TestInterface95

on TestSortWithOptions
	local tContainer
	put "foo1,foo10,foo9a,foo9,bar2" into tContainer

	local tOptions
	put true into tOptions["numeric"]

	sort items of tContainer international with options tOptions
	TestAssert "sort international with numeric option", tContainer is "bar2,foo1,foo9,foo9a,foo10" 

	put "foo1,foo10,foo9a,foo9,bar2" into tContainer
	sort items of tContainer international
	TestAssert "sort international without numeric option", tContainer is "bar2,foo1,foo10,foo9,foo9a" 
end TestSortWithOptions