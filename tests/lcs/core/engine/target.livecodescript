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

on TestDeleteTargetInFrontScript
   create button "frontscript"
   set the script of button "frontscript" to "on doSomething; delete the target; pass doSomething; end doSomething"
   
   insert the script of button "frontscript" into front
   
   create button "test"
   set the script of button "test" to "on doSomething; end doSomething"
   send "doSomething" to button "test"
   
   TestAssert "Object is deleted", there is no button "test"
end TestDeleteTargetInFrontScript
