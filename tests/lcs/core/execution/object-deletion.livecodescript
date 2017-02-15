script "CoreExecutionObjectDeletion"
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


on TestSetup
   create stack "test"
end TestSetup


on TestDeleteObjectInNestedGroup
   set the defaultStack to "test"
   create group "test1"
   create group "test2" in group "test1"
   create button "test" in group "test2"
   set the script of button "test" to \
         "on DeleteIt" & return & \
         "DeleteGroup" & return & \
         "end DeleteIt"
   set the script of stack "test" to \
         "on DeleteGroup" & return & \
         "delete group "& quote & "test1" & quote & return & \
         "end DeleteGroup"
   TestAssertThrow "Delete object executing script in nested group", "DeleteIt", the long id of button "test" of stack "test", 347
end TestDeleteObjectInNestedGroup

on TestDeleteThisCard
   set the defaultStack to "test"
   create group "test"
   create button "test" in group "test"
   set the script of button "test" to \
         "on DeleteIt" & return & \
         "delete this card" & return & \
         "end DeleteIt"
   TestAssertThrow "Delete card from button on card", "DeleteIt", the long id of button "test" of stack "test", 347
   
   set the sharedBehavior of group "test" to true
   dispatch "DeleteIt" to button "test"
   TestAssert "Delete card from button in shared group on card", there is not a button "test" of card 1 of stack "test"
end TestDeleteThisCard

on TestDeleteStackWithCantDelete
   set the cantDelete of stack "test" to true
   TestAssertThrow "Delete stack with cantDelete", "__TestDeleteStack", the long id me, 347
   set the cantDelete of stack "test" to false
end TestDeleteStackWithCantDelete

on TestDeleteSubstackWithCantDelete
   create stack "test substack"
   set the mainstack of stack "test substack" to "test"
   set the cantDelete of stack "test substack" to true
   delete stack "test"
   TestAssert "Delete stack with cantDelete substack", true
end TestDeleteSubstackWithCantDelete

on TestDeleteObjectInBehaviorOfNonTarget
   create button "behavior"
   set the script of button "behavior" to \
         "on DeleteIt" & return & \
         "delete stack "& quote & "lib" & quote  & return & \
         "end DeleteIt"
   create stack "lib"
   set the behavior of stack "lib" to the long id of button "behavior"
   start using stack "lib"
   TestAssertThrow "Delete object from its behavior when not target", "DeleteIt", the long id of stack "test", 347
end TestDeleteObjectInBehaviorOfNonTarget


on __TestDeleteStack
      delete stack "test"
end __TestDeleteStack

on TestTeardown
   if there is a stack "test" then
      delete stack "test"
   end if
end TestTeardown
