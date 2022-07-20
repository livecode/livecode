script "CoreEngineDebugger"
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

on TestWatchedVariables
   TestAssert "watchedVariables initially empty", the watchedVariables is empty
   
   local tWatches
   put ",,someVar,someVar is true" into tWatches
   set the watchedVariables to tWatches
   TestAssert "watchedVariables global round trips", the watchedVariables is tWatches
   
   set the watchedVariables to empty
   TestAssert "watchedVariables clears", the watchedVariables is empty
   
   put the long id of me,",someVar,someVar is true" into tWatches
   set the watchedVariables to tWatches
   TestAssert "watchedVariables script local round trips", the watchedVariables is tWatches
   
   put the long id of me,"someHandler,someVar,someVar is true" into tWatches
   set the watchedVariables to tWatches
   TestAssert "watchedVariables handler local round trips", the watchedVariables is tWatches
end TestWatchedVariables

command _SwitchChoice
    switch error() 
		case 1
	end switch
end _SwitchChoice

command _SwitchChoiceSplit
	switch \ 
		error()
		case 1
	end switch
end _SwitchChoiceSplit

command _SwitchCase
	switch 1 
		case error()
	end switch
end _SwitchCase

command _SwitchCaseSplit
	switch 1 
		case \
			error()
	end switch
end _SwitchCaseSplit

command _IfCondition
	if error() then 
	end if
end _IfCondition

command _IfConditionSplit
	if \  
		error() then
	end if
end _IfConditionSplit

command _ElseIfCondition
	if 0 then 
	else if error() then
	end if
end _ElseIfCondition

command _ElseIfConditionSplit
	if 0 then 
	else if \
		error() then
	end if
end _ElseIfConditionSplit

command _RepeatCounted
	repeat error() times 
	end repeat
end _RepeatCounted

command _RepeatCountedSplit
	repeat \ 
		error() times
	end repeat
end _RepeatCountedSplit

command _RepeatForFrom
	repeat for x = error() to 10 by 20 
	end repeat
end _RepeatForFrom

command _RepeatForFromSplit
	repeat for x = \ 
		error() to 10 by 20
	end repeat
end _RepeatForFromSplit

command _RepeatForTo
	repeat for x = 1 to error() by 20 
	end repeat
end _RepeatForTo

command _RepeatForToSplit
	repeat for x = 1 to \ 
		error() by 20
	end repeat
end _RepeatForToSplit

command _RepeatForBy
	repeat x = 1 to 10 by error() 
	end repeat
end _RepeatForBy

command _RepeatForBySplit
	repeat x = 1 to 10 by \ 
		error()
	end repeat
end _RepeatForBySplit

command _RepeatForEachTarget
	repeat for each x in error() 
	end repeat
end _RepeatForEachTarget

command _RepeatForEachTargetSplit
	repeat for each x in \ 
		error()
	end repeat
end _RepeatForEachTargetSplit

command _RepeatUntilCondition
	repeat until error() 
	end repeat
end _RepeatUntilCondition

command _RepeatUntilConditionSplit
	repeat until \ 
		error()
	end repeat
end _RepeatUntilConditionSplit

command _RepeatWhileCondition
	repeat while error() 
	end repeat
end _RepeatWhileCondition

command _RepeatWhileConditionSplit
	repeat while \ 
		error()
	end repeat
end _RepeatWhileConditionSplit
