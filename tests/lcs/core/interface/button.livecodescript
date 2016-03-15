script "CoreInterfaceButton"
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


on TestInterface92
   
   create stack "Test"
   set the defaultstack to "Test"
   
   create button "SelTest"
   
   set the style of button "SelTest" to "menu"
   
   set the menuMode of button "SelTest" to "Option"
   
   set the text of button "SelTest" to "bar" & CR & "foo"
   
   select line 2 of button "SelTest"
   
   TestAssert "test", the label of button "SelTest" is "foo"
end TestInterface92

// Test all button-specific properties.
