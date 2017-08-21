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
   local tOne, tTwo, tThree, tFour
   put 1 + 0 into tOne
   put 2 + 0 into tTwo
   put 3 + 0 into tThree
   put 4 + 0 into tFour

   local tLegacyLiteral
   BenchmarkStartTiming "LegacyConstantCreation"
   repeat kRepetitions times
      put empty into tLegacyLiteral
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

   BenchmarkStartTiming "LegacyDynamicCreation"
   repeat kRepetitions times
      put empty into tLegacyLiteral
      put _Identity(true) into tLegacyLiteral[tOne]
      put _Identity(1) into tLegacyLiteral[tTwo]
      put _Identity(pi) into tLegacyLiteral[tThree]
      put _Identity("Hello") into tLegacyLiteral[tFour]
   end repeat
   BenchmarkStopTiming

   BenchmarkStartTiming "ConstantCreation"
   repeat kRepetitions times
      get [ true, 1, pi, "Hello", false, 2, pi, "World!", [ true, 1, pi, "Hello", false, 2, pi, "World!" ] ]
   end repeat
   BenchmarkStopTiming

   BenchmarkStartTiming "DynamicCreation"
   repeat kRepetitions times
      get [ _Identity(true), _Identity(1), _Identity(pi), _Identity("Hello") ]
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

function _Identity pValue
   return pValue
end _Identity
