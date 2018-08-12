script "CoreEngineBehavior"
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

----------------------------------------------------------------
-- Errors
----------------------------------------------------------------

on _TestCyclicBehavior pStack
   set the behavior of (the behavior of pStack) to pStack
end _TestCyclicBehavior

on TestCyclicBehavior
   create stack "Behavior"
   create stack
   set the behavior of it to the long id of stack "Behavior"
   TestAssertThrow "cycle in behavior script hierarchy throws", \
      "_TestCyclicBehavior", the long id of me, \ 
      "EE_PARENTSCRIPT_CYCLICOBJECT", it
end TestCyclicBehavior

on TestInvalidBehaviorChange
   local tScript
   put "on changeBehavior" & return into tScript
   put "set the behavior of me to the long id of stack" \ 
      && quote & "NewBehavior" & quote & return after tScript
   put "end changeBehavior" after tScript

   create stack "NewBehavior"
   create stack "Behavior"
   set the script of it to tScript
   
   create stack
   set the behavior of it to the long id of stack "Behavior"
   TestAssertThrow "behavior change while parent script is executing throws", \
      "changeBehavior", it, "EE_PARENTSCRIPT_EXECUTING"
end TestInvalidBehaviorChange

on _TestInvalidObjectBehavior pStack
   create field
   set the behavior of pStack to it   
end _TestInvalidObjectBehavior

on TestInvalidObjectTypeBehavior
   local tStack, tField
   create stack
   put it into tStack 

   TestAssertThrow "set behavior to non-button control throws", \
      "_TestInvalidObjectBehavior", the long id of me, \
      "EE_PARENTSCRIPT_BADOBJECT", tStack
end TestInvalidObjectTypeBehavior

----------------------------------------------------------------
-- Functionality
----------------------------------------------------------------

on TestSetBehaviorNoChange
   local tScript
   put "on noChangeBehavior" & return into tScript
   put "set the behavior of me to the long id of this me" & return after tScript
   put "end noChangeBehavior" after tScript

   local tBehavior
   create stack "Behavior"
   put it into tBehavior
   set the script of tBehavior to tScript
   
   local tTarget
   create stack
   put it into tTarget
   set the behavior of tTarget to tBehavior
   dispatch "noChangeBehavior" to tTarget
   TestAssert "set behavior to current behavior object", \
      the behavior of tTarget is tBehavior
end TestSetBehaviorNoChange

private command _TestSetBehaviorParent pType
   local tParentScript
   put "on parentHandler; end parentHandler" into tParentScript
   
   local tParentObject
   if pType is "button" then
      create stack
      create button "Behavior"
   else
      create stack "Behavior"
   end if
   put it into tParentObject
   set the script of tParentObject to tParentScript
  
   local tTarget
   create stack
   put it into tTarget
   set the behavior of tTarget to tParentObject
   
   dispatch "parentHandler" to tTarget
   TestAssert pType && "behavior handles dispatched message", \ 
      it is "handled"
   
   set the behavior of tTarget to empty
   TestAssert pType && "behavior set to empty", \ 
      the behavior of tTarget is empty

   dispatch function "returnValue" to tTarget   
   TestAssert pType && "behavior no longer in message path", \ 
      it is "unhandled"
end _TestSetBehaviorParent

on TestSetBehavior
   repeat for each item tType in "button,stack"
      _TestSetBehaviorParent tType
   end repeat      
end TestSetBehavior

on TestBehaviorHandlerResolveOrder
   local tScript
   put "function returnValue; return false; end returnValue" into tScript
   create stack "Behavior"
   set the script of it to tScript
  
   local tTarget
   create stack
   put it into tTarget
   put "function returnValue; return true; end returnValue" into tScript 
   set the script of tTarget to tScript
   set the behavior of tTarget to the long id of stack "Behavior"
   
   dispatch function "returnValue" to tTarget
   TestAssert "dispatched function handled before behavior", \ 
      the result is true
   
   put "function returnValue; pass returnValue; end returnValue" into tScript 
   set the script of tTarget to tScript
   
   dispatch function "returnValue" to tTarget
   TestAssert "behavior handles passed dispatched function", \ 
      the result is false
end TestBehaviorHandlerResolveOrder

on TestChangeBehaviorWhileExecuting
   create stack "test"
   set the defaultStack to "test"
   create button "child"
   set the script of button "child" to "on Foo;Bar;end Foo"
   create button "parent"
   set the script of button "parent" to "on Bar;Baz;end Bar"
   set the script of stack "test" to "on Bar;set the behavior of button " & quote & "child" & quote & " to the long id of button " & quote & "parent"& quote &";end Bar"
   TestAssertDoesNotThrow "can set behavior while executing if it is empty", "Foo", \
      the long id of button "child"
 
   set the script of stack "test" to "on Baz;set the behavior of button " & quote & "child" & quote & " to the long id of button " & quote & "parent"& quote &";end Baz"
   TestAssertDoesNotThrow "can set behavior while executing if it is the same", "Foo", \
      the long id of button "child"
   
   -- wait 0 milliseconds;throw 0 added to test the crash if the error is not thrown
   set the script of stack "test" to "on Baz;set the behavior of button " & quote & "child" & quote & " to empty;wait 0 milliseconds;throw 0;end Baz"  
   TestAssertThrow "set behavior to empty while it is executing throws", "Foo", \
       the long id of button "child", "EE_PARENTSCRIPT_EXECUTING"
   delete stack "test"
 end TestChangeBehaviorWhileExecuting
