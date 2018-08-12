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
