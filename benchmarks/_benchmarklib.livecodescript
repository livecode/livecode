script "BenchmarkLibrary"
/*
Copyright (C) 2015 LiveCode Ltd.

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

----------------------------------------------------------------
-- Helper functions
----------------------------------------------------------------

----------------------------------------------------------------
-- Benchmark library functions
----------------------------------------------------------------

local sBenchmarkStartTime
local sBenchmarkVariant

on BenchmarkStartTiming pVariant
   put pVariant into sBenchmarkVariant
   put the millisecs into sBenchmarkStartTime
end BenchmarkStartTiming

on BenchmarkStopTiming
   write sBenchmarkVariant & tab & (the millisecs - sBenchmarkStartTime) & return to stdout
end BenchmarkStopTiming

on BenchmarkLoadNativeTextFile pFilename
   set the itemDelimiter to slash
   put item 1 to -2 of the filename of the target & slash & pFilename into pFilename
   
   get url ("binfile:" & pFilename)
   if the result is not empty then
      throw "BenchmarkLoadNativeTextFile" && quote & pFilename & quote && "failed" & return & it
   end if
   replace crlf with lf in it
   replace numToChar(13) with lf in it
   
   return it
end BenchmarkLoadNativeTextFile

on errorDialog executionError, parseError
   write executionError & return to stderr
   quit 1
end errorDialog
