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

on TestIconGravity
   create stack "Test"
   set the defaultStack to "Test"

   set the iconGravity of the templateButton to "top"
   create button "TestButton"
   TestAssert "create button from template preserves iconGravity", \
                  the iconGravity of button "TestButton" is "top"

   clone button "TestButton" as "TestButtonClone"
   TestAssert "clone button preserves iconGravity", \
                  the iconGravity of button "TestButtonClone" is "top"
end TestIconGravity

on TestBug18293
   TestSkipIf "environment", "command line"
   TestSkipIfNot "wait"
   
   create stack "popup menu"
   create stack "test"
   go stack "test"
   create button "test"
   set the menumode of button "test" to "popup"
   set the menuname of button "test" to "popup menu"
   click at the loc of button "test"
   delete stack "popup menu"
   TestAssert "deleting popped up menu stack shouldn't crash", true
end TestBug18293
