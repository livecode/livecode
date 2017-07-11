script "CoreEngineTarget"
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
   create button "script"
   set the script of button "script" to "on doSomething; delete the target; pass doSomething; end doSomething"

   create button "test"
end TestSetup

on TestDeleteTargetInFrontScript
   insert the script of button "script" into front
   dispatch "doSomething" to button "test"
   TestAssert "Delete target in frontscript", there is no button "test"
   TestAssert "Delete target in frontscript blocks message", it is "handled"
   remove the script of button "script" from front
end TestDeleteTargetInFrontScript

on TestDeleteTargetInBackScript
   insert the script of button "script" into back
   dispatch "doSomething" to button "test"
   TestAssert "Delete target in backscript", there is no button "test"
   TestAssert "Delete target in backscript blocks message", it is "handled"
   remove the script of button "script" from back
end TestDeleteTargetInBackScript

on TestDeleteTargetInBehavior
   set the behavior of button "test" to the long id of button "script"
   TestAssertThrow "Delete target in behavior", "__TestDeleteTarget", \
      the long id me, "EE_OBJECT_CANTREMOVE"
   set the behavior of button "test" to empty
end TestDeleteTargetInBehavior

on TestDeleteTargetInOwner
   set the script of the owner of btn "test" to "on doSomething; delete the target; pass doSomething; end doSomething"
   dispatch "doSomething" to button "test"
   TestAssert "Delete target in owner", there is no button "test"
   TestAssert "Delete target in owner blocks message", it is "handled"
end TestDeleteTargetInOwner

/**/

on __TestDeleteTarget
   send "doSomething" to button "test"
end __TestDeleteTarget
