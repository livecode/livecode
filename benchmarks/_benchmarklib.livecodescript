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

on BenchmarkStartTiming
	put the millisecs into sBenchmarkStartTime
end BenchmarkStartTiming

on BenchmarkEndTiming
	write (the millisecs - sBenchmarkStartTime) to stdout
end BenchmarkEndTiming

on errorDialog executionError, parseError
   write executionError & return to stderr
   quit 1
end errorDialog
