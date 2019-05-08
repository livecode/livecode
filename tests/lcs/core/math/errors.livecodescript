script "CoreMathErrors"
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

command DoSin pParam
   get sin(pParam)
end DoSin

command DoCos pParam
   get cos(pParam)
end DoCos

command DoTan pParam
   get tan(pParam)
end DoTan

command DoAcos pParam
   get acos(pParam)
end DoAcos

on TestAcosError
   TestAssertThrow "acos throws on invalid domain", "DoAcos", \
         the long id of me, "EE_MATH_DOMAIN", 2
end TestAcosError

command DoAsin pParam
   get asin(pParam)
end DoAsin

on TestAsinError
   TestAssertThrow "asin throws on invalid domain", "DoAsin", \
         the long id of me, "EE_MATH_DOMAIN", 2
end TestAsinError

command DoAtan pParam
   get atan(pParam)
end DoAtan

command DoExp pParam
   get exp(pParam)
end DoExp

command DoExp1 pParam
   get exp1(pParam)
end DoExp1

command DoExp2 pParam
   get exp2(pParam)
end DoExp2

command DoExp10 pParam
   get exp10(pParam)
end DoExp10

on TestExpErrors
   local tParam
   put GetVeryLargeInteger() into tParam
   repeat for each item tItem in "exp,exp1,exp2,exp10"
      TestAssertThrow tItem && "overflow throws", "Do" & tItem, \
            the long id of me, "EE_MATH_RANGE", tParam
   end repeat
end TestExpErrors

command DoLn pParam
   get ln(pParam)
end DoLn

on TestLnError
   TestAssertThrow "ln throws with negative input", "DoLn", \
         the long id of me, "EE_MATH_DOMAIN", -1

   TestAssertThrow "ln throws with -inf result", "DoLn", \
         the long id of me, "EE_MATH_ZERO", 0
end TestLnError

command DoLn1 pParam
   get ln1(pParam)
end DoLn1

on TestLn1Error
   TestAssertThrow "ln1 throws with input < -1", "DoLn1", \
         the long id of me, "EE_MATH_DOMAIN", -2

   TestAssertThrow "ln1 throws with -inf result", "DoLn1", \
         the long id of me, "EE_MATH_ZERO", -1
end TestLn1Error

command DoLog2 pParam
   get log2(pParam)
end DoLog2

on TestLog2Error
   TestAssertThrow "log2 throws with negative input", "DoLog2", \
         the long id of me, "EE_MATH_DOMAIN", -1

   TestAssertThrow "log2 throws with -inf result", "DoLog2", \
         the long id of me, "EE_MATH_ZERO", 0
end TestLog2Error

command DoLog10 pParam
   get log10(pParam)
end DoLog10

on TestLog10Error
   TestAssertThrow "log10 throws with negative input", "DoLog10", \
         the long id of me, "EE_MATH_DOMAIN", -1

   TestAssertThrow "log10 throws with -inf result", "DoLog10", \
         the long id of me, "EE_MATH_ZERO", 0
end TestLog10Error

command DoSqrt pParam
   get sqrt(pParam)
end DoSqrt

on TestSqrtError
   TestAssertThrow "sqrt throws with negative input", "DoSqrt", \
         the long id of me, "EE_MATH_DOMAIN", -1
end TestSqrtError

command DoAtan2 pParam
   get atan2(pParam["left"], pParam["right"])
end DoAtan2

command DoDiv pParam
   get pParam["left"] div pParam["right"]
end DoDiv

on TestDivError
   local tParam
   put 10 into tParam["left"]
   put 0 into tParam["right"]
   TestAssertThrow "div throws with 0 denominator", "DoDiv", \
         the long id of me, "EE_MATH_ZERO", tParam
end TestDivError

command DoMod pParam
   get pParam["left"] mod pParam["right"]
end DoMod

on TestModError
   local tParam
   put 10 into tParam["left"]
   put 0 into tParam["right"]
   TestAssertThrow "mod 0 throws", "DoMod", \
         the long id of me, "EE_MATH_ZERO", tParam
end TestModError

command DoWrap pParam
   get pParam["left"] wrap pParam["right"]
end DoWrap

on TestWrapError
   local tParam
   put 10 into tParam["left"]
   put 0 into tParam["right"]
   TestAssertThrow "wrap 0 throws", "DoWrap", \
         the long id of me, "EE_MATH_ZERO", tParam
end TestWrapError

// Returns an integer large enough to cause overflow when added to itself
private function GetVeryLargeInteger pNegative
   if pNegative then
      return -(10^308)
   end if
   return 10^308
end GetVeryLargeInteger

command DoPlus pParam
   get pParam["left"] + pParam["right"]
end DoPlus

on TestPlusError
   local tParam
   put GetVeryLargeInteger() into tParam["left"]
   put GetVeryLargeInteger() into tParam["right"]
   TestAssertThrow "plus overflow throws", "DoPlus", \
         the long id of me, "EE_MATH_RANGE", tParam
end TestPlusError

command DoMultiply pParam
   get pParam["left"] * pParam["right"]
end DoMultiply

on TestMultiplyError
   local tParam
   put GetVeryLargeInteger() into tParam["left"]
   put 10 into tParam["right"]
   TestAssertThrow "multiply overflow throws", "DoMultiply", \
         the long id of me, "EE_MATH_RANGE", tParam
end TestMultiplyError

command DoMinus pParam
   get pParam["left"] - pParam["right"]
end DoMinus

on TestMinusError
   local tParam
   put GetVeryLargeInteger(true) into tParam["left"]
   put GetVeryLargeInteger() into tParam["right"]
   TestAssertThrow "minus overflow throws", "DoMinus", \
         the long id of me, "EE_MATH_RANGE", tParam
end TestMinusError

command DoOver pParam
   get pParam["left"] / pParam["right"]
end DoOver

on TestOverError
   local tParam
   put 10 into tParam["left"]
   put 0 into tParam["right"]
   TestAssertThrow "/ throws with 0 denominator", "DoOver", \
         the long id of me, "EE_MATH_ZERO", tParam
end TestOverError

command DoPow pParam
   get pParam["left"] ^ pParam["right"]
end DoPow

on TestPowError
   local tParam
   put -1 into tParam["left"]
   put 0.5 into tParam["right"]
   TestAssertThrow "negative^(abs < 1) throws", "DoPow", \
         the long id of me, "EE_MATH_DOMAIN", tParam

   put -1 into tParam["left"]
   put -0.5 into tParam["right"]
   TestAssertThrow "negative^(-1 < x < 0) throws", "DoPow", \
         the long id of me, "EE_MATH_DOMAIN", tParam

   put 0 into tParam["left"]
   put -2 into tParam["right"]
   TestAssertThrow "zero^(-ve) throws", "DoPow", \
         the long id of me, "EE_MATH_ZERO", tParam
end TestPowError

command DoAnnuity pParam
   get annuity(pParam["left"], pParam["right"])
end DoAnnuity

on TestAnnuityError
   // Divide by zero
   local tParam
   put -1 into tParam["left"]
   put 2 into tParam["right"]
   TestAssertThrow "annuity(-1, +ve) throws", "DoAnnuity", \
         the long id of me, "EE_MATH_ZERO", tParam

   // Sqrt -1
   put -2 into tParam["left"]
   put -0.5 into tParam["right"]
   TestAssertThrow "annuity(-ve, -1 < x < 0) throws", "DoAnnuity", \
         the long id of me, "EE_MATH_DOMAIN", tParam

   put -2 into tParam["left"]
   put 0.5 into tParam["right"]
   TestAssertThrow "annuity(-ve, 0 < x < 1) throws", "DoAnnuity", \
         the long id of me, "EE_MATH_DOMAIN", tParam
end TestAnnuityError

command DoCompound pParam
   get compound(pParam["left"], pParam["right"])
end DoCompound

on TestCompoundError
   // Divide by zero
   local tParam
   put -1 into tParam["left"]
   put -1 into tParam["right"]
   TestAssertThrow "compound(-1, -ve) throws", "DoCompound", \
         the long id of me, "EE_MATH_ZERO", tParam

   // Sqrt -1
   put -2 into tParam["left"]
   put 0.5 into tParam["right"]
   TestAssertThrow "compound(-ve, abs < 1) throws", "DoCompound", \
         the long id of me, "EE_MATH_DOMAIN", tParam
end TestCompoundError

on TestNaryOverflowErrors
   local tParam
   put GetVeryLargeInteger() into tParam["args"][1]
   put GetVeryLargeInteger() into tParam["args"][2]
   repeat for each item tItem in "sum,average,median,avgdev,stddev,pop_stddev,pop_variance,variance"
      put tItem into tParam["which"]
      TestAssertThrow tItem && "overflow throws", "DoNaryFunction", \
            the long id of me, "EE_MATH_RANGE", tParam
   end repeat
end TestNaryOverflowErrors

command DoGeoMean pParam
   get geometricMean(pParam[1], pParam[2])
end DoGeoMean

on TestGeoMeanError
   local tParam
   put -1 into tParam[1]
   put 1 into tParam[2]
   TestAssertThrow "geometric mean with negative number throws domain error", "DoGeoMean", \
         the long id of me, "EE_MATH_DOMAIN", tParam
end TestGeoMeanError

command DoNaryFunction pParams
   switch pParams["which"]
   case "sum"
      get sum(pParams["args"])
      break
   case "average"
      get avg(pParams["args"])
      break
   case "median"
      get median(pParams["args"])
      break
   case "avgdev"
      get averageDeviation(pParams["args"])
      break
   case "pop_stddev"
      get populationStandardDeviation(pParams["args"])
      break
   case "pop_variance"
      get populationVariance(pParams["args"])
      break
   case "stddev"
      get sampleStandardDeviation(pParams["args"])
      break
   case "variance"
      get sampleVariance(pParams["args"])
      break
   end switch
end DoNaryFunction
