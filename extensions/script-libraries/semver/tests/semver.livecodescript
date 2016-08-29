script "TestSemverLib"
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
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.
*/

on TestSetup
   local tLibFilename
   put TestGetEngineRepositoryPath() & \
         "/extensions/script-libraries/semver/semver.livecodescript" \
         into tLibFilename

   start using stack tLibFilename
end TestSetup

on TestSemverParsingVersions
   __TestParsing true, "v1.0.0", false
   __TestParsing true, "v1.0.0-alpha.1", false
   __TestParsing true, "v1.0.0-beta.2+metadata", false
   __TestParsing true, "1.0.0", false
   __TestParsing false, "rc1.0.0", false
end TestSemverParsingVersions

on TestSemverParsingSimpleRanges
   __TestParsing true, "~1.0.0", true
   __TestParsing true, "^1.0.0", true
   __TestParsing true, ">1.0.0", true
   __TestParsing true, "<1.0.0", true
   __TestParsing true, ">=1.0.0", true
   __TestParsing true, "<=1.0.0", true
   __TestParsing true, "=1.0.0", true
   __TestParsing true, "1.0.x", true
   __TestParsing true, "1.0", true
end TestSemverParsingSimpleRanges

on TestSemverParsingComplexRanges
   __TestParsing true, ">=1.2.7 <1.3.0", true
   __TestParsing true, "1.2.7 || >=1.2.9 <2.0.0", true
   __TestParsing true, "1.2.3 - 2.3.4", true
   __TestParsing true, "1.2.3 - 2", true
end TestSemverParsingComplexRanges

-- The comparator `>=1.2.7` would match the versions `1.2.7`, `1.2.8`, `2.5.3`,
-- and `1.3.9`, but not the versions `1.2.6` or `1.1.0`.
on TestSemverSatisfiesGreaterThanOrEqual
   __TestSatisfiesRange "1.2.7", ">=1.2.7", true
   __TestSatisfiesRange "1.2.8", ">=1.2.7", true
   __TestSatisfiesRange "2.5.3", ">=1.2.7", true
   __TestSatisfiesRange "1.3.9", ">=1.2.7", true
   __TestSatisfiesRange "1.2.6", ">=1.2.7", false
   __TestSatisfiesRange "1.1.0", ">=1.2.7", false
end TestSemverSatisfiesGreaterThanOrEqual

-- the range `>=1.2.7 <1.3.0` would match the versions `1.2.7`, `1.2.8`, and
-- `1.2.99`, but not the versions `1.2.6`, `1.3.0`, or `1.1.0`.
on TestSemverSatisfiesGreaterThanOrEqualAndLessThan
   __TestSatisfiesRange "1.2.7", ">=1.2.7 <1.3.0", true
   __TestSatisfiesRange "1.2.8", ">=1.2.7 <1.3.0", true
   __TestSatisfiesRange "1.2.99", ">=1.2.7 <1.3.0", true
   __TestSatisfiesRange "1.2.6", ">=1.2.7 <1.3.0", false
   __TestSatisfiesRange "1.3.0", ">=1.2.7 <1.3.0", false
   __TestSatisfiesRange "1.1.0", ">=1.2.7 <1.3.0", false
end TestSemverSatisfiesGreaterThanOrEqualAndLessThan

-- The range `1.2.7 || >=1.2.9 <2.0.0` would match the versions `1.2.7`, `1.2.9`, and `1.4.6`,
-- but not the versions `1.2.8` or `2.0.0`.
on TestSemverSatisfiesVersionOrGreaterThanOrEqualAndLessThan
   __TestSatisfiesRange "1.2.7", "1.2.7 || >=1.2.9 <2.0.0", true
   __TestSatisfiesRange "1.2.9", "1.2.7 || >=1.2.9 <2.0.0", true
   __TestSatisfiesRange "1.4.6", "1.2.7 || >=1.2.9 <2.0.0", true
   __TestSatisfiesRange "1.2.8", "1.2.7 || >=1.2.9 <2.0.0", false
   __TestSatisfiesRange "2.0.0", "1.2.7 || >=1.2.9 <2.0.0", false
end TestSemverSatisfiesVersionOrGreaterThanOrEqualAndLessThan

on TestSemverGreaterThanPrereleaseRange
   __TestSatisfiesRange "1.2.3","1.2.X-beta.1", true
   __TestSatisfiesRange "1.2.0-alpha-1","1.2.X-beta.1", false
   __TestSatisfiesRange "1.2.1-alpha-1","1.2.X", false
   __TestSatisfiesRange "1.2.1-alpha-1","~1.2.0-alpha-1", false
   __TestSatisfiesRange "1.2.0-alpha-1","~1.2.0-beta-1", false
   __TestSatisfiesRange "1.2.3","~1.2.0-beta-1", true
end TestSemverGreaterThanPrereleaseRange

command __TestSatisfiesRange pVersion, pRange, pSatisfies
   local tOutput
   put "version" && pVersion into tOutput
   if pSatisfies then
      put " in range" && pRange after tOutput
   else
      put " not in range" && pRange after tOutput
   end if
   
   local tResult
   put semverSatisfies(pVersion, pRange) into tResult
   put ": " & tResult after tOutput
   
   TestAssert tOutput, tResult is pSatisfies
end __TestSatisfiesRange

command __TestParsing pValid, pVersion, pRange
   local tOutput
   if pValid then
      put "parsing valid " into tOutput
   else
      put "parsing invalid " into tOutput
   end if
   
   if pRange then
      put "range " after tOutput
   end if
   
   put pVersion after tOutput
   
   if pRange then
      semverParseRange pVersion
   else
      semverParse pVersion
   end if
   
   local tResult
   put the result is empty into tResult
   put ": " & tResult after tOutput
   TestAssert tOutput, tResult is pValid
end __TestParsing
