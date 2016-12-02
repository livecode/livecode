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

on _TestCyclicBehavior pStack
   set the behavior of (the behavior of pStack) to pStack
end _TestCyclicBehavior

on TestCyclicBehavior
   create stack "Behavior"
   create stack
   set the behavior of it to the long id of stack "Behavior"
   TestAssertThrow "cycle in behavior script hierarchy throws", \
      "_TestCyclicBehavior", the long id of me, 678, it
end TestCyclicBehavior

on TestInvalidBehaviorChange
   local tScript
   put "on changeBehavior" & return into tScript
   put "set the behavior of me to the long id of stack" && quote & "NewBehavior" & quote & return after tScript
   put "end changeBehavior" after tScript

   create stack "NewBehavior"
   create stack "Behavior"
   set the script of it to tScript
   
   create stack
   set the behavior of it to the long id of stack "Behavior"
   TestAssertThrow "behavior change while parent script is executing throws", \
      "changeBehavior", it, 895
end TestInvalidBehaviorChange