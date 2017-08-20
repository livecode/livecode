script "ArrayLiterals"
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

constant kRepetitions = 1000000

command BenchmarkSequenceLiterals
   local tLegacyLiteral
   BenchmarkStartTiming "LegacyCreation"
   repeat kRepetitions times
      get empty
      put true into tLegacyLiteral[1]
      put 1 into tLegacyLiteral[2]
      put pi into tLegacyLiteral[3]
      put "Hello" into tLegacyLiteral[4]
      put false into tLegacyLiteral[5]
      put 2 into tLegacyLiteral[6]
      put pi into tLegacyLiteral[7]
      put "World!" into tLegacyLiteral[8]
      put it into tLegacyLiteral[9]
   end repeat
   BenchmarkStopTiming

   BenchmarkStartTiming "ConstantCreation"
   repeat kRepetitions times
      get [ true, 1, pi, "Hello", false, 2, pi, "World!", [ true, 1, pi, "Hello", false, 2, pi, "World!" ] ]
   end repeat
   BenchmarkStopTiming

   local tConstantLiteral
   put [ true, 1, pi, "Hello", false, 2, pi, "World!", [ true, 1, pi, "Hello", false, 2, pi, "World!" ] ] into tConstantLiteral
   BenchmarkStartTiming "ConstantComparison"
   repeat kRepetitions times
      get tConstantLiteral is [ true, 1, pi, "Hello", false, 2, pi, "World!", [ true, 1, pi, "Hello", false, 2, pi, "World!" ] ]
   end repeat
   BenchmarkStopTiming
end BenchmarkSequenceLiterals
