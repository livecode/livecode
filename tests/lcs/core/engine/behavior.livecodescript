script "CoreEngineBehavior"
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


on TestChangeBehaviorWhileExecuting
   create stack "test"
   set the defaultStack to "test"
   create button "child"
   set the script of button "child" to "on Foo;Bar;end Foo"
   create button "parent"
   set the script of button "parent" to "on Bar;Baz;end Bar"
   set the behavior of button "child" to the long id of button "parent"
   set the script of stack "test" to "on Baz;set the behavior of button " & quote & "child" & quote & " to the long id of button " & quote & "parent"& quote &";end Baz"
   TestAssertDoesNotThrow "can set behavior while it may be executing if it is empty", "Foo", \
      the long id of button "child"
   
   TestAssertDoesNotThrow "can set behavior while it may be executing if it is the same", "Foo", \
      the long id of button "child"
   
   -- wait 0 milliseconds;throw 0 added to test the crash if the error is not thrown
   set the script of stack "test" to "on Baz;set the behavior of button " & quote & "child" & quote & " to empty;wait 0 milliseconds;throw 0;end Baz"  
   TestAssertThrow "can't set behavior while it may be executing", "Foo", \
      the long id of button "child", "EE_OBJECT_SCRIPTEXECUTING"
   delete stack "test"
end TestChangeBehaviorWhileExecuting
