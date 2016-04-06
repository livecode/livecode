script "CoreStringsBasic"
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

constant kNativeNonFoldingString = ":;_(-)_;:"
constant kNativeFoldingString = "HelloWorld"
constant kNativeDecomposingString = "Café"

constant kNativeUncasedString = ":;_(-)_;:"
constant kNativeUncasedPrefix = ":;_("
constant kNativeUncasedSuffix = ")_;:"
constant kNativeUncasedMiddle = "_(-)_"

constant kNativeLesserUncasedString = "#$%"
constant kNativeGreaterUncasedString = "$%%"

constant kNativeCasedString = "HelloWorlD"
constant kNativeCasedPrefix = "Hello"
constant kNativeCasedSuffix = "WorlD"
constant kNativeCasedMiddle = "loWor"

constant kNativeLesserCasedString = "ABC"
constant kNativeGreaterCasedString = "bcd"

on TestStringOptions
  set the caseSensitive to false
  set the formSensitive to false
  TestAssert "case false form false - case is false", the caseSensitive is false
  TestAssert "case false form false - form is false", the formSensitive is false

  set the caseSensitive to true
  set the formSensitive to true
  TestAssert "case true form true - case is true", the caseSensitive is true
  TestAssert "case true form true - form is true", the formSensitive is true

  set the caseSensitive to false
  set the formSensitive to true
  TestAssert "case false form true - case is false", the caseSensitive is false
  TestAssert "case false form true - form is true", the formSensitive is true

  set the caseSensitive to true
  set the formSensitive to false
  TestAssert "case true form false - case is true", the caseSensitive is true
  TestAssert "case true form false - form is false", the formSensitive is false
end TestStringOptions

on TestToUpper
  set the caseSensitive to true
  TestAssert "upper of lowercase", toUpper("xy") is "XY"
  TestAssert "upper of uppercase", toUpper("XY") is "XY"
  TestAssert "upper of mixedcase", toUpper("xY") is "XY"
end TestToUpper

on TestToLower
  set the caseSensitive to true
  TestAssert "lower of lowercase", toLower("xy") is "xy"
  TestAssert "lower of uppercase", toLower("XY") is "xy"
  TestAssert "lower of mixedcase", toLower("xY") is "xy"
end TestToLower

on TestBeginsWith
  set the caseSensitive to false
  TestAssert "caseless native uncased begins with native uncased", kNativeUncasedString begins with kNativeUncasedPrefix
  TestAssert "caseless native cased begins with native cased", kNativeCasedString begins with kNativeCasedPrefix
  TestAssert "caseless native cased begins with lower native cased", kNativeCasedString begins with toLower(kNativeCasedPrefix)
  TestAssert "caseless native cased begins with upper native cased", kNativeCasedString begins with toUpper(kNativeCasedPrefix)
  TestAssert "caseless lower native cased begins with native cased", toLower(kNativeCasedString) begins with kNativeCasedPrefix
  TestAssert "caseless upper native cased begins with native cased", toUpper(kNativeCasedString) begins with kNativeCasedPrefix

  set the caseSensitive to true
  TestAssert "exact native uncased begins with native uncased", kNativeUncasedString begins with kNativeUncasedPrefix
  TestAssert "exact native cased begins with native cased", kNativeCasedString begins with kNativeCasedPrefix
  TestAssert "exact not native cased begins with lower native cased", not (kNativeCasedString begins with toLower(kNativeCasedPrefix))
  TestAssert "exact not native cased begins with upper native cased", not (kNativeCasedString begins with toUpper(kNativeCasedPrefix))
  TestAssert "exact not lower native cased begins with native cased", not (toLower(kNativeCasedString) begins with kNativeCasedPrefix)
  TestAssert "exact not upper native cased begins with native cased", not (toUpper(kNativeCasedString) begins with kNativeCasedPrefix)
end TestBeginsWith

on TestEndsWith
  set the caseSensitive to false
  TestAssert "caseless native uncased ends with native uncased", kNativeUncasedString ends with kNativeUncasedSuffix
  TestAssert "caseless native cased ends with native cased", kNativeCasedString ends with kNativeCasedSuffix
  TestAssert "caseless native cased ends with lower native cased", kNativeCasedString ends with toLower(kNativeCasedSuffix)
  TestAssert "caseless native cased ends with upper native cased", kNativeCasedString ends with toUpper(kNativeCasedSuffix)
  TestAssert "caseless lower native cased ends with native cased", toLower(kNativeCasedString) ends with kNativeCasedSuffix
  TestAssert "caseless upper native cased ends with native cased", toUpper(kNativeCasedString) ends with kNativeCasedSuffix

  set the caseSensitive to true
  TestAssert "exact native uncased ends with native uncased", kNativeUncasedString ends with kNativeUncasedSuffix
  TestAssert "exact native cased ends with native cased", kNativeCasedString ends with kNativeCasedSuffix
  TestAssert "exact not native cased ends with lower native cased", not (kNativeCasedString ends with toLower(kNativeCasedSuffix))
  TestAssert "exact not native cased ends with upper native cased", not (kNativeCasedString ends with toUpper(kNativeCasedSuffix))
  TestAssert "exact not lower native cased ends with native cased", not (toLower(kNativeCasedString) ends with kNativeCasedSuffix)
  TestAssert "exact not upper native cased ends with native cased", not (toUpper(kNativeCasedString) ends with kNativeCasedSuffix)
end TestEndsWith

on TestContains
  set the caseSensitive to false
  TestAssert "caseless native uncased contains native uncased (prefix)", kNativeUncasedString contains kNativeUncasedPrefix
  TestAssert "caseless native uncased contains native uncased (middle)", kNativeUncasedString contains kNativeUncasedMiddle
  TestAssert "caseless native uncased contains native uncased (suffix)", kNativeUncasedString contains kNativeUncasedSuffix

  TestAssert "caseless native cased contains native cased (prefix)", kNativeCasedString contains kNativeCasedPrefix
  TestAssert "caseless native cased contains native cased (middle)", kNativeCasedString contains kNativeCasedMiddle
  TestAssert "caseless native cased contains native cased (suffix)", kNativeCasedString contains kNativeCasedSuffix

  TestAssert "caseless native cased contains native lower cased (middle)", kNativeCasedString contains toLower(kNativeCasedMiddle)
  TestAssert "caseless native cased contains native upper cased (middle)", kNativeCasedString contains toUpper(kNativeCasedMiddle)
  TestAssert "caseless native lower cased contains native cased (middle)", toLower(kNativeCasedString) contains kNativeCasedMiddle
  TestAssert "caseless native upper cased contains native cased (middle)", toUpper(kNativeCasedString) contains kNativeCasedMiddle

  set the caseSensitive to true
  TestAssert "exact native uncased contains native uncased (prefix)", kNativeUncasedString contains kNativeUncasedPrefix
  TestAssert "exact native uncased contains native uncased (middle)", kNativeUncasedString contains kNativeUncasedMiddle
  TestAssert "exact native uncased contains native uncased (suffix)", kNativeUncasedString contains kNativeUncasedSuffix

  TestAssert "exact native cased contains native cased (prefix)", kNativeCasedString contains kNativeCasedPrefix
  TestAssert "exact native cased contains native cased (middle)", kNativeCasedString contains kNativeCasedMiddle
  TestAssert "exact native cased contains native cased (suffix)", kNativeCasedString contains kNativeCasedSuffix

  TestAssert "exact not native cased contains native lower cased (middle)", not (kNativeCasedString contains toLower(kNativeCasedMiddle))
  TestAssert "exact not native cased contains native upper cased (middle)", not (kNativeCasedString contains toUpper(kNativeCasedMiddle))
  TestAssert "exact not native lower cased contains native cased (middle)", not (toLower(kNativeCasedString) contains kNativeCasedMiddle)
  TestAssert "exact not native upper cased contains native cased (middle)", not (toUpper(kNativeCasedString) contains kNativeCasedMiddle)
end TestContains

on TestIs
  set the caseSensitive to false
  TestAssert "caseless native uncased is native uncased", (kNativeUncasedString & space) is (kNativeUncasedString & space)
  TestAssert "caseless native cased is native cased", (kNativeCasedString & space) is (kNativeCasedString & space)
  TestAssert "caseless native cased is native upper cased", (kNativeCasedString & space) is toUpper(kNativeCasedString & space)
  TestAssert "caseless native cased is native lower cased", (kNativeCasedString & space) is toLower(kNativeCasedString & space)
  TestAssert "caseless native lower cased is native cased", toLower(kNativeCasedString & space) is (kNativeCasedString & space)
  TestAssert "caseless native upper cased is native cased", toUpper(kNativeCasedString & space) is (kNativeCasedString & space)

  set the caseSensitive to true
  TestAssert "exact native uncased is native uncased", (kNativeUncasedString & space) is (kNativeUncasedString & space)
  TestAssert "exact native cased is native cased", (kNativeCasedString & space) is (kNativeCasedString & space)
  TestAssert "exact not native cased is native upper cased", not ((kNativeCasedString & space) is toUpper(kNativeCasedString & space))
  TestAssert "exact not native cased is native lower cased", not ((kNativeCasedString & space) is toLower(kNativeCasedString & space))
  TestAssert "exact not native lower cased is native cased", not (toLower(kNativeCasedString & space) is (kNativeCasedString & space))
  TestAssert "exact not native upper cased is native cased", not (toUpper(kNativeCasedString & space) is (kNativeCasedString & space))
end TestIs

on TestComparison
   set the caseSensitive to false
   TestAssert "caseless native uncased < native uncased", kNativeLesserUncasedString < kNativeGreaterUncasedString
   TestAssert "caseless native uncased <= native uncased", kNativeLesserUncasedString <= kNativeGreaterUncasedString
   TestAssert "caseless native uncased > native uncased", kNativeGreaterUncasedString > kNativeLesserUncasedString
   TestAssert "caseless native uncased >= native uncased", kNativeGreaterUncasedString >= kNativeLesserUncasedString

   TestAssert "caseless native cased < native cased", kNativeLesserCasedString < kNativeGreaterCasedString
   TestAssert "caseless native cased <= native cased", kNativeLesserCasedString <= kNativeGreaterCasedString
   TestAssert "caseless native cased > native cased", kNativeGreaterCasedString > kNativeLesserCasedString
   TestAssert "caseless native cased >= native cased", kNativeGreaterCasedString >= kNativeLesserCasedString

   set the caseSensitive to true
   TestAssert "exact native uncased < native uncased", kNativeLesserUncasedString < kNativeGreaterUncasedString
   TestAssert "exact native uncased <= native uncased", kNativeLesserUncasedString <= kNativeGreaterUncasedString
   TestAssert "exact native uncased > native uncased", kNativeGreaterUncasedString > kNativeLesserUncasedString
   TestAssert "exact native uncased >= native uncased", kNativeGreaterUncasedString >= kNativeLesserUncasedString

   TestAssert "exact native cased < native cased", kNativeLesserCasedString < kNativeGreaterCasedString
   TestAssert "exact native cased <= native cased", kNativeLesserCasedString <= kNativeGreaterCasedString
   TestAssert "exact native cased > native cased", kNativeGreaterCasedString > kNativeLesserCasedString
   TestAssert "exact native cased >= native cased", kNativeGreaterCasedString >= kNativeLesserCasedString
end TestComparison
