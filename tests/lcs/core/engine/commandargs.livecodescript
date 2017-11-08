script "CoreEngineCommandArgs"
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

on TestCommandName
    -- Bug 20478: run a subprocess in which we get the commandName and
    -- then quit, to test that it no longer crashes
   local tStackToRun, tOptions
   put the effective filename of me into tStackToRun
   set the itemdelimiter to slash
   if the environment is "server" then
      put "_commandname.lc" into item -1 of tStackToRun
   else
      put "_commandname.livecodescript" into item -1 of tStackToRun
      if the environment contains "command line" then
         put "-ui" into tOptions
      end if
   end if
   TestRunStack tOptions, tStackToRun
   TestAssert "command name does not crash on quit", the result is empty
end TestCommandName

command _TestCommandArgumentsZeroIndexError
	return commandArguments(0)
end _TestCommandArgumentsZeroIndexError

command _TestCommandArgumentsNegativeIndexError
	return commandArguments(-1)
end _TestCommandArgumentsNegativeIndexError

on TestCommandArguments
	TestAssertThrow "command arguments invalid index (0) throws", \
		_TestCommandArgumentsZeroIndexError, the long id of me, \
		EE_COMMANDARGUMENTS_BADPARAM
		
	TestAssertThrow "command arguments invalid index (-1) throws", \
		_TestCommandArgumentsNegativeIndexError, the long id of me, \
		EE_COMMANDARGUMENTS_BADPARAM		
end TestCommandArguments
