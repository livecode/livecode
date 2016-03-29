script "CoreExecutionLiteralEvaluation"
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

on TestEvaluateNumericLiteral
   set the numberFormat to "00"
   TestAssert "numeric literal remains string", (2 & empty) is "2"
   TestAssert "numeric literal becomes number in numeric context", ((2 + 0) & empty) is "02"

   local tArray
   put "2" into tArray["2"]
   TestAssert "numeric literal remains string in computation context", (tArray[2] + 1) is "03"
end TestEvaluateNumericLiteral
