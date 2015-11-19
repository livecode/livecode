script "ControlRepeat"
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

on BenchmarkRepeatForever
   local tCount
   put 10000000 into tCount
   BenchmarkStartTiming
   repeat forever
      subtract 1 from tCount
      if tCount is 0 then
         exit repeat
      end if
   end repeat
   BenchmarkStopTiming
end BenchmarkRepeatForever

on BenchmarkRepeatCount
   BenchmarkStartTiming
   repeat 100000000  times
   end repeat
   BenchmarkStopTiming
end BenchmarkRepeatCount

on BenchmarkRepeatWith
   BenchmarkStartTiming
   repeat with i = 1 to 10000000 
   end repeat
   BenchmarkStopTiming
end BenchmarkRepeatWith

