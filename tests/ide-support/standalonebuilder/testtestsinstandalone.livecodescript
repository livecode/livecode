script "RunTestsInStandalone"
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

local sSupportStack
local sBuilderStack

on TestSetup   
   set the itemdelimiter to slash
   local tStack
   put the filename of me into tStack
   put "_support.livecodescript" into item -1 of tStack
   put tStack into sSupportStack
   start using stack sSupportStack
   
   put "_standalonetestrunner/StandaloneTestRunnerBuilder.livecodescript" into item -3 to -1 of tStack
   put tStack into sBuilderStack
end TestSetup

on TestTestsInStandalone
   dispatch "BuildAndRun" to stack sBuilderStack with GetBuildPlatform()
   local tResult
   put the result into tResult
   TestAssert "Tests pass when run in standalone", tResult is empty
   if tResult is not empty then
      TestDiagnostic "Result:" & return & tResult
   end if
end TestTestsInStandalone

on TestTeardown
   --StandaloneBuilderCleanUpStandalones
   stop using stack sSupportStack
end TestTeardown
