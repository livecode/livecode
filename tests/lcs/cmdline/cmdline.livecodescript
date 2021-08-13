script "CmdLine"
/*
Copyright (C) 2018 LiveCode Ltd.

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

private command RunStack pWhich
   local tStackToRun, tOptions
   put the effective filename of me into tStackToRun
   set the itemdelimiter to slash
   if the environment is "server" then
      put "_" & pWhich & ".lc" into item -1 of tStackToRun
   else
      put "_" & pWhich & ".livecodescript" into item -1 of tStackToRun
      if the environment contains "command line" then
         put "-ui" into tOptions
      end if
   end if
   if pWhich is empty then
      TestRunStack tOptions, ""
   else
      TestRunStack tOptions, tStackToRun
   end if
   return the result
end RunStack

on TestNonExistentScript
   TestSkipIfNot "environment", "server"
   RunStack "non-existent"
   TestAssert "run server with nonexistent stack argument", \
      the result is not empty
end TestNonExistentScript

on TestNoScript
   TestSkipIfNot "environment", "server"
   RunStack
   TestAssert "run server with no stack argument", \
      the result is not empty
end TestNoScript

on TestBadScript
   TestSkipIfNot "environment", "server"
   RunStack "parseerr"
   TestAssert "run stack with parse error", the result is not empty
end TestBadScript

on TestScriptError
   TestSkipIfNot "environment", "server"
   RunStack "execerr"
   TestAssert "run stack with execution error", the result is not empty
end TestScriptError