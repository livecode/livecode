script "log"
/*
Copyright (C) 2019 LiveCode Ltd.

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

on TestLog
   local tStackToRun
   put the effective filename of me into tStackToRun
   set the itemdelimiter to slash
   if the environment is "server" then
      put "_log.lc" into item -1 of tStackToRun
   else
      put "_log.livecodescript" into item -1 of tStackToRun
   end if

   local tOptions
   if the environment contains "command line" then
      put "-ui" into tOptions
   end if
   set the itemdelimiter to comma

   local tArgs, tResult, tUUID
   put uuid() into tUUID
   repeat for each item tLogMessage in "logPublic,logPrivate"
      put tLogMessage && tUUID into tArgs
      TestRunStack tOptions, tStackToRun, tArgs
      if the result is not empty then
         TestDiagnostic the result
      else
         TestDiagnostic it
      end if
      TestAssert "log message" && tLogMessage, tUUID is in it
   end repeat

   put quote & quote && tUUID into tArgs
   TestRunStack tOptions, tStackToRun, tArgs
   if the result is not empty then
      TestDiagnostic the result
   else
      TestDiagnostic it
   end if
   TestAssert "log message" && tLogMessage, tUUID is not in it
end TestLog
