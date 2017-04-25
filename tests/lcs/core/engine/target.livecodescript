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
   set the script of button "test" to "on doSomething; send __TestDeletedCallback; end doSomething"
   __TestDeleteTarget
   TestAssert "Delete target in frontscript", there is no button "test"
   TestAssert "Delete target in frontscript blocks message", sDeleted is not true
   remove the script of button "script" from front
end TestDeleteTargetInFrontScript

on TestDeleteTargetInBackScript
   insert the script of button "script" into back
   TestAssertThrow "Delete target in backscript", "__TestDeleteTarget", the long id me, 347
   remove the script of button "script" from back
end TestDeleteTargetInBackScript

on TestDeleteTargetInBehavior
   set the behavior of button "test" to the long id of button "script"
   TestAssertThrow "Delete target in behavior", "__TestDeleteTarget", the long id me, 347
   set the behavior of button "test" to empty
end TestDeleteTargetInBehavior

on TestDeleteTargetInOwner
   set the script of the owner of btn "test" to "on doSomething; delete the target; end doSomething"
   TestAssertThrow "Delete target in owner", "__TestDeleteTarget", the long id me, 347
end TestDeleteTargetInOwner

on __TestDeleteTarget
   send "doSomething" to button "test"
end __TestDeleteTarget

local sDeleted
on __TestDeletedCallback
   put true into sDeleted
end __TestDeletedCallback
