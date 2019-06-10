script "com.livecode.library.semver"
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

Documentation for node-semver reproduced under The ISC License
Copyright (c) Isaac Z. Schlueter and Contributors

*/

on extensionInitialize
   if the target is not me then
      pass extensionInitialize
   end if
   
   insert the script of me into back

   if the environment contains "development" then
      set the _ideoverride of me to true
   end if
end extensionInitialize

on extensionFinalize
   if the target is not me then
      pass extensionFinalize
   end if
   
   remove the script of me from back
end extensionFinalize

/**

Title: Semantic Versioning Library

Author: LiveCode 

Type: library

Version: 1.0.0

Description:

## Versions

A "version" is described by the v2.0.0 specification found at http://semver.org/.

A leading `=` or `v` character is stripped off and ignored.

## Ranges

A version range is a set of comparators which specify versions that satisfy the range.

A comparator is composed of an operator and a version. The set of primitive operators is:

- `<` Less than
- `<=` Less than or equal to
- `>` Greater than
- `>=` Greater than or equal to
- `=` Equal. If no operator is specified, then equality is assumed, so this operator is
optional, but MAY be included.

For example, the comparator `>=1.2.7` would match the versions `1.2.7`, `1.2.8`, `2.5.3`,
and `1.3.9`, but not the versions `1.2.6` or `1.1.0`.

Comparators can be joined by whitespace to form a comparator set, which is satisfied by
the intersection of all of the comparators it includes.

A range is composed of one or more comparator sets, joined by ||. A version matches a
range if and only if every comparator in at least one of the ||-separated comparator
sets is satisfied by the version.

For example, the range `>=1.2.7 <1.3.0` would match the versions `1.2.7`, `1.2.8`, and
`1.2.99`, but not the versions `1.2.6`, `1.3.0`, or `1.1.0`.

The range `1.2.7 || >=1.2.9 <2.0.0` would match the versions `1.2.7`, `1.2.9`, and `1.4.6`,
but not the versions `1.2.8` or `2.0.0`.

## Prerelease Tags

If a version has a prerelease tag (for example, `1.2.3-alpha.3`) then it will only be
allowed to satisfy comparator sets if at least one comparator with the same 
[major, minor, patch] tuple also has a prerelease tag.

For example, the range `>1.2.3-alpha.3` would be allowed to match the version `1.2.3-alpha.7`,
but it would not be satisfied by `3.4.5-alpha.9`, even though `3.4.5-alpha.9` is technically 
*greater than* `1.2.3-alpha.3` according to the SemVer sort rules. The version range only
accepts prerelease tags on the `1.2.3` version. The version `3.4.5` would satisfy the range,
because it does not have a prerelease flag, and `3.4.5` is greater than `1.2.3-alpha.7`.

The purpose for this behavior is twofold. First, prerelease versions frequently are updated
very quickly, and contain many breaking changes that are (by the author's design) not yet
fit for public consumption. Therefore, by default, they are excluded from range matching
semantics.

Second, a user who has opted into using a prerelease version has clearly indicated the intent
to use that specific set of alpha/beta/rc versions. By including a prerelease tag in the range,
the user is indicating that they are aware of the risk. However, it is still not appropriate
to assume that they have opted into taking a similar risk on the next set of prerelease
versions.

## Advanced Range Syntax

Advanced range syntax desugars to primitive comparators in deterministic ways.

Advanced ranges may be combined in the same way as primitive comparators using white space
or ||.

### Hyphen Ranges X.Y.Z - A.B.C

Specifies an inclusive set.

`1.2.3 - 2.3.4 := >=1.2.3 <=2.3.4`

If a partial version is provided as the first version in the inclusive range, then the missing
pieces are replaced with zeroes.

`1.2 - 2.3.4 := >=1.2.0 <=2.3.4`

If a partial version is provided as the second version in the inclusive range, then all
versions that start with the supplied parts of the tuple are accepted, but nothing that would
be greater than the provided tuple parts.

`1.2.3 - 2.3 := >=1.2.3 <2.4.0`
`1.2.3 - 2 := >=1.2.3 <3.0.0`

### X-Ranges 1.2.x 1.X 1.2.* *

Any of X, x, or * may be used to *stand in* for one of the numeric values in the
[major, minor, patch] tuple.

- * := >=0.0.0 (Any version satisfies)
- 1.x := >=1.0.0 <2.0.0 (Matching major version)
- 1.2.x := >=1.2.0 <1.3.0 (Matching major and minor versions)

A partial version range is treated as an X-Range, so the special character is in
fact optional.

- "" (empty string) := * := >=0.0.0
- 1 := 1.x.x := >=1.0.0 <2.0.0
- 1.2 := 1.2.x := >=1.2.0 <1.3.0

### Tilde Ranges ~1.2.3 ~1.2 ~1

Allows patch-level changes if a minor version is specified on the comparator. Allows minor-level
changes if not.

- ~1.2.3 := >=1.2.3 <1.(2+1).0 := >=1.2.3 <1.3.0
- ~1.2 := >=1.2.0 <1.(2+1).0 := >=1.2.0 <1.3.0 (Same as 1.2.x)
- ~1 := >=1.0.0 <(1+1).0.0 := >=1.0.0 <2.0.0 (Same as 1.x)
- ~0.2.3 := >=0.2.3 <0.(2+1).0 := >=0.2.3 <0.3.0
- ~0.2 := >=0.2.0 <0.(2+1).0 := >=0.2.0 <0.3.0 (Same as 0.2.x)
- ~0 := >=0.0.0 <(0+1).0.0 := >=0.0.0 <1.0.0 (Same as 0.x)
- ~1.2.3-beta.2 := >=1.2.3-beta.2 <1.3.0 Note that prereleases in the 1.2.3 version will be
allowed, if they are greater than or equal to beta.2. So, 1.2.3-beta.4 would be allowed, but
`1.2.4-beta.2` would not, because it is a prerelease of a different [major, minor, patch] tuple.

### Caret Ranges ^1.2.3 ^0.2.5 ^0.0.4

Allows changes that do not modify the left-most non-zero digit in the [major, minor, patch]
tuple. In other words, this allows patch and minor updates for versions 1.0.0 and above, patch
updates for versions 0.X >=0.1.0, and no updates for versions 0.0.X.

Many authors treat a 0.x version as if the x were the major "breaking-change" indicator.

Caret ranges are ideal when an author may make breaking changes between 0.2.4 and 0.3.0 releases,
which is a common practice. However, it presumes that there will not be breaking changes between
0.2.4 and 0.2.5. It allows for changes that are presumed to be additive (but non-breaking),
according to commonly observed practices.

- ^1.2.3 := >=1.2.3 <2.0.0
- ^0.2.3 := >=0.2.3 <0.3.0
- ^0.0.3 := >=0.0.3 <0.0.4
- ^1.2.3-beta.2 := >=1.2.3-beta.2 <2.0.0 Note that prereleases in the `1.2.3` version will be allowed,
if they are greater than or equal to beta.2. So, `1.2.3-beta.4` would be allowed, but `1.2.4-beta.2`
would not, because it is a prerelease of a different [major, minor, patch] tuple.
- ^0.0.3-beta := >=0.0.3-beta <0.0.4 Note that prereleases in the 0.0.3 version only will be allowed,
if they are greater than or equal to beta. So, 0.0.3-pr.2 would be allowed.

When parsing caret ranges, a missing patch value desugars to the number 0, but will allow flexibility
within that value, even if the major and minor versions are both 0.

- ^1.2.x := >=1.2.0 <2.0.0
- ^0.0.x := >=0.0.0 <0.1.0
- ^0.0 := >=0.0.0 <0.1.0

A missing minor and patch values will desugar to zero, but also allow flexibility within those values,
even if the major version is zero.

- ^1.x := >=1.0.0 <2.0.0
- ^0.x := >=0.0.0 <1.0.0

## Range Grammar

Putting all this together, here is a Backus-Naur grammar for ranges, for the benefit of parser authors:

```
range-set  ::= range ( logical-or range ) *
logical-or ::= ( ' ' ) * '||' ( ' ' ) *
range      ::= hyphen | simple ( ' ' simple ) * | ''
hyphen     ::= partial ' - ' partial
simple     ::= primitive | partial | tilde | caret
primitive  ::= ( '<' | '>' | '>=' | '<=' | '=' | ) partial
partial    ::= xr ( '.' xr ( '.' xr qualifier ? )? )?
xr         ::= 'x' | 'X' | '*' | nr
nr         ::= '0' | ['1'-'9'] ( ['0'-'9'] ) *
tilde      ::= '~' partial
caret      ::= '^' partial
qualifier  ::= ( '-' pre )? ( '+' build )?
pre        ::= parts
build      ::= parts
parts      ::= part ( '.' part ) *
part       ::= nr | [-0-9A-Za-z]+
```

*/

---------------------------------------------------
-- PARSING

/**

Parse the version string

Parameters:

pVersion (string): Parsed to a version array

Description:

Parse the version to an array with keys:

- major
- minor
- patch
- prerelease
- build
- metadata
- operator (ranges)

The result:
An error string if any occurred or empty

*/

command semverParse @xVersion
   __SemverParse xVersion, false
   if the result is not empty then
      return the result for error
   end if
   return empty for value
end semverParse

private command __SemverParse @xVersion, pRange
   local tVersion, tParsedVersion
   put xVersion into tVersion
   
   if tVersion is not an ascii string then
      return "Version is not an ascii string:" && xVersion for error
   end if
   
   -- strip leading v
   if tVersion begins with "v" then
      delete char 1 of tVersion
   end if
   
   -- parse operators
   if pRange then
      repeat while char 1 of tVersion is in "<>=~^"
         put char 1 of tVersion after tParsedVersion["operator"]
         delete char 1 of tVersion
      end repeat
   end if
   
   -- metadata if any -> major.minor.patch-prerelease.build+metadata
   set the itemDelimiter to "+"
   if the number of items of tVersion > 1 then
      put the last item of tVersion into tParsedVersion["metadata"]
      delete the last item of tVersion
   end if
   
   -- prerelease stage if any -> major.minor.patch-prerelease.build
   set the itemDelimiter to "-"
   if the number of items of tVersion > 1 then
      local tPrerelease
      put item 2 to -1 of tVersion into tPrerelease
      delete item 2 to -1 of tVersion
      
      set the itemDelimiter to "."
      put item 1 of tPrerelease into tParsedVersion["prerelease"]
      put item 2 of tPrerelease into tParsedVersion["build"]
      if tParsedVersion["build"] is empty then
         put 0 into tParsedVersion["build"]
      end if
   end if
   
   
   -- major number -> major.minor.patch
   set the itemDelimiter to "."
   put item 1 of tVersion into tParsedVersion["major"]
   if tParsedVersion["major"] is empty then
      if pRange then
         put "x" into tParsedVersion["major"]
      else
         put 0 into tParsedVersion["major"]
      end if
   end if
   if pRange then
      replace "*" with "x" in tParsedVersion["major"]
   end if
   delete item 1 of tVersion
   
   -- minor number -> minor.patch
   put item 1 of tVersion into tParsedVersion["minor"]
   if tParsedVersion["minor"] is empty then
      if pRange then
         put "x" into tParsedVersion["minor"]
      else
         put 0 into tParsedVersion["minor"]
      end if
   end if
   if pRange then
      replace "*" with "x" in tParsedVersion["minor"]
   end if
   delete item 1 of tVersion
   
   -- patch number -> patch
   put tVersion into tParsedVersion["patch"]
   if tParsedVersion["patch"] is empty then
      if pRange then
         put "x" into tParsedVersion["patch"]
      else
         put 0 into tParsedVersion["patch"]
      end if
   end if
   if pRange then
      replace "*" with "x" in tParsedVersion["patch"]
   end if
   
   if tParsedVersion["major"] is not an integer \
         and not (pRange and tParsedVersion["major"] is "x") then
      return "Invalid major number:" && xVersion for error
   end if
   
   if tParsedVersion["minor"] is not an integer \
         and not (pRange and tParsedVersion["minor"] is "x") then
      return "Invalid minor number:" && xVersion for error
   end if
   
   if tParsedVersion["patch"] is not an integer \
         and not (pRange and tParsedVersion["patch"] is "x") then
      return "Invalid patch number:" && xVersion for error
   end if
   
   if tParsedVersion["prerelease"] is not empty and tParsedVersion["build"] is not an integer then
      return "Invalid build number string:" && xVersion for error
   end if
   
   put tParsedVersion into xVersion
end __SemverParse

---------------------------------------------------
-- COMPARISON

private function __GreaterThan pLeft, pRight
   if pLeft["major"] is "x" or pRight["major"] is "x" then
      return __PrereleaseGreaterThan(pLeft, pRight)
   end if
   
   if pLeft["major"] > pRight["major"] then
      return __PrereleaseGreaterThan(pLeft, pRight)
   else if pLeft["major"] < pRight["major"] then
      return false
   end if
   
   if pLeft["minor"] is "x" or pRight["minor"] is "x" then
      return __PrereleaseGreaterThan(pLeft, pRight)
   end if
   
   if pLeft["minor"] > pRight["minor"] then
      return __PrereleaseGreaterThan(pLeft, pRight)
   else if pLeft["minor"] < pRight["minor"] then
      return false
   end if
   
   if pLeft["patch"] is "x" or pRight["patch"] is "x" then
      return __PrereleaseGreaterThan(pLeft, pRight)
   end if
   
   if pLeft["patch"] > pRight["patch"] then
      return __PrereleaseGreaterThan(pLeft, pRight)
   else if pLeft["patch"] < pRight["patch"] then
      return false
   end if
   
   return false
end __GreaterThan

private function __PrereleaseGreaterThan pLeft, pRight
   if pLeft["prerelease"] is "x" and pRight["prerelease"] is "x" or \
         pLeft["prerelease"] is empty and pRight["prerelease"] is empty then
      return true
   end if
   
   if pLeft["prerelease"] is empty and pRight["prerelease"] is not empty then
      return true
   else if pLeft["prerelease"] is not empty and pRight["prerelease"] is empty then
      return false
   end if
   
   -- ascii string comparison
   if pLeft["prerelease"] > pRight["prerelease"] then
      return true
   else if pLeft["prerelease"] < pRight["prerelease"] then
      return false
   end if
   
   if pLeft["build"] is "x" or pRight["build"] is "x" then
      return true
   end if
   
   if pLeft["build"] > pRight["build"] then
      return true
   end if
   
   return false
end __PrereleaseGreaterThan

private function __PrereleaseGreaterThanOrEqual pLeft, pRight
   if pLeft["prerelease"] is pRight["prerelease"] and \
         pLeft["build"] is pRight["build"] then
      return true
   end if
   
   return __PrereleaseGreaterThan(pLeft, pRight)
end __PrereleaseGreaterThanOrEqual

/**

Determine if a version is greater than another

Parameters:
pLeft (string): A valid version string
pRight (string): A valid version string

Returns:
True if pLeft is greater than pRight

*/

function semverGreaterThan pLeft, pRight
   semverParse pLeft
   if the result is not empty then
      return the result for error
   end if
   
   semverParse pRight
   if the result is not empty then
      return the result for error
   end if
   
   return __GreaterThan(pLeft, pRight)
end semverGreaterThan

/**

Determine if a version is greater than or equal to another

Parameters:
pLeft (string): A valid version string
pRight (string): A valid version string

Returns:
True if pLeft is greater than or equal to pRight

*/

function semverGreaterThanOrEqual pLeft, pRight
   semverParse pLeft
   if the result is not empty then
      return the result for error
   end if
   
   semverParse pRight
   if the result is not empty then
      return the result for error
   end if
   
   return __Equal(pLeft, pRight) or __GreaterThan(pLeft, pRight)
end semverGreaterThanOrEqual

private function __Equal pLeft, pRight
   delete variable pLeft["metadata"]
   delete variable pLeft["operator"]
   delete variable pRight["metadata"]
   delete variable pRight["operator"]
   return pLeft is pRight
end __Equal

/**

Determine if a version is equal to another

Parameters:
pLeft (string): A valid version string
pRight (string): A valid version string

Returns:
True if pLeft is less than or equal to pRight

*/

function semverEqual pLeft, pRight
   semverParse pLeft
   if the result is not empty then
      return the result for error
   end if
   
   semverParse pRight
   if the result is not empty then
      return the result for error
   end if
   
   return __Equal(pLeft, pRight)
end semverEqual

---------------------------------------------------
-- RANGES

private function __SatisfiesCaret pVersion, pRange
   local tKey
   local tChecked = false
   repeat for each word tKey in "major minor patch prerelease build"
      if pVersion[tKey] is 0 and pRange[tKey] is 0 then
         next repeat
      end if
      
      if pRange[tKey] is "x" then
         return true
      end if
      
      if not tChecked and pVersion[tKey] > pRange[tKey] then
         return false
      end if
      
      if pVersion[tKey] < pRange[tKey] then
         return false
      end if
      
      put true into tChecked
   end repeat
   
   return true
end __SatisfiesCaret

private function __SatisfiesTilde pVersion, pRange
   if pVersion["major"] is not pRange["major"] then
      return false
   end if
   
   if pRange["minor"] is "x" then
      return true
   else if pVersion["minor"] is not pRange["minor"] then
      return false
   end if
   
   -- allow patches to minor release
   return __GreaterThan(pVersion, pRange)
end __SatisfiesTilde

private function __SatisfiesXRange pVersion, pRange
   local tKey
   repeat for each word tKey in "major minor patch"
      if pRange[tKey] is "x" then
         return __PrereleaseGreaterThanOrEqual(pVersion, pRange)
      else if pVersion[tKey] is not pRange[tKey] then
         return false
      end if
   end repeat
   
   return true
end __SatisfiesXRange

private function __SatisfiesRange pVersion, pRange
   local tConditions
   local tConditionIndex = 1
   local tResult
   
   local tIndex
   repeat with tIndex = 1 to the number of elements of pRange
      if pRange[tIndex] is "||" then
         add 1 to tConditionIndex
      else if pRange[tIndex] is "-" then
         -- hyphen ranges are already assigned appropriate operators
         next repeat
      else
         switch pRange[tIndex]["operator"]
            case "="
               put __Equal(pVersion, pRange[tIndex]) into tResult
               break
            case "<"
               put __GreaterThan(pRange[tIndex], pVersion) into tResult
               break
            case ">="
               put __Equal(pVersion, pRange[tIndex]) or __GreaterThan(pVersion, pRange[tIndex]) into tResult
               break
            case "<="
               put __Equal(pVersion, pRange[tIndex]) or __GreaterThan(pRange[tIndex], pVersion) into tResult
               break
            case "^"
               put __SatisfiesCaret(pVersion, pRange[tIndex]) into tResult
               break
            case ">"
               put __GreaterThan(pVersion, pRange[tIndex]) into tResult
               break
            case "~"
               put __SatisfiesTilde(pVersion, pRange[tIndex]) into tResult
               break
            default
               put __SatisfiesXRange(pVersion, pRange[tIndex]) into tResult
               break
         end switch
         
         if tConditions[tConditionIndex] is empty then
            put tResult into tConditions[tConditionIndex]
         else
            put tResult and tConditions[tConditionIndex] into tConditions[tConditionIndex]
         end if
      end if
   end repeat
   
   put tConditions[1] into tResult
   
   repeat with tIndex = 2 to the number of elements in tConditions
      put tResult or tConditions[tIndex] into tResult
   end repeat
   return tResult
end __SatisfiesRange

private function __IsX pVal
   return pVal is empty or pVal is "x" or pVal is "*"
end __IsX

private function __ExpandCaret pRangeComparator
   local tExpandedRange
   
   if __IsX(pRangeComparator["major"]) then
      -- ^ --> *
      put empty into tExpandedRange
   else if __IsX(pRangeComparator["minor"]) then
      -- ^1 --> >=1.0.0 <2.0.0
      put pRangeComparator["major"] into tExpandedRange[1]["major"]
      put 0 into tExpandedRange[1]["minor"]
      put 0 into tExpandedRange[1]["patch"]
      put ">=" into tExpandedRange[1]["operator"]
      
      put pRangeComparator["major"] + 1 into tExpandedRange[2]["major"]
      put 0 into tExpandedRange[2]["minor"]
      put 0 into tExpandedRange[2]["patch"]
      put "<" into tExpandedRange[2]["operator"]
   else if __IsX(pRangeComparator["patch"]) then
      if pRangeComparator["major"] is 0 then
         -- ^0.1 --> >=0.1.0 <0.2.0
         put pRangeComparator["major"] into tExpandedRange[1]["major"]
         put pRangeComparator["minor"] into tExpandedRange[1]["minor"]
         put 0 into tExpandedRange[1]["patch"]
         put ">=" into tExpandedRange[1]["operator"]
         
         put pRangeComparator["major"] into tExpandedRange[2]["major"]
         put pRangeComparator["minor"] + 1 into tExpandedRange[2]["minor"]
         put 0 into tExpandedRange[2]["patch"]
         put "<" into tExpandedRange[2]["operator"]
      else
         -- ^1.2 --> >=1.2.0 <2.0.0
         put pRangeComparator["major"] into tExpandedRange[1]["major"]
         put pRangeComparator["minor"] into tExpandedRange[1]["minor"]
         put 0 into tExpandedRange[1]["patch"]
         put ">=" into tExpandedRange[1]["operator"]
         
         put pRangeComparator["major"] + 1 into tExpandedRange[2]["major"]
         put 0 into tExpandedRange[2]["minor"]
         put 0 into tExpandedRange[2]["patch"]
         put "<" into tExpandedRange[2]["operator"]
      end if
   else
      if pRangeComparator["major"] is 0 then
         if pRangeComparator["minor"] is 0 then
            -- ^0.0.1 --> >=0.0.1 <0.0.2
            put pRangeComparator["major"] into tExpandedRange[1]["major"]
            put pRangeComparator["minor"] into tExpandedRange[1]["minor"]
            put pRangeComparator["patch"] into tExpandedRange[1]["patch"]
            put ">=" into tExpandedRange[1]["operator"]
            
            put pRangeComparator["major"] into tExpandedRange[2]["major"]
            put pRangeComparator["minor"] into tExpandedRange[2]["minor"]
            put pRangeComparator["patch"] + 1 into tExpandedRange[2]["patch"]
            put "<" into tExpandedRange[2]["operator"]
         else
            -- ^0.1.1 --> >=0.1.1 <0.2.0
            put pRangeComparator["major"] into tExpandedRange[1]["major"]
            put pRangeComparator["minor"] into tExpandedRange[1]["minor"]
            put pRangeComparator["patch"] into tExpandedRange[1]["patch"]
            put ">=" into tExpandedRange[1]["operator"]
            
            put pRangeComparator["major"] into tExpandedRange[2]["major"]
            put pRangeComparator["minor"] + 1 into tExpandedRange[2]["minor"]
            put 0 into tExpandedRange[2]["patch"]
            put "<" into tExpandedRange[2]["operator"]
         end if
      else
         -- ^1.2.3 --> >=1.2.3 <2.0.0
         put pRangeComparator["major"] into tExpandedRange[1]["major"]
         put pRangeComparator["minor"] into tExpandedRange[1]["minor"]
         put pRangeComparator["patch"] into tExpandedRange[1]["patch"]
         put ">=" into tExpandedRange[1]["operator"]
         
         put pRangeComparator["major"] + 1 into tExpandedRange[2]["major"]
         put 0 into tExpandedRange[2]["minor"]
         put 0 into tExpandedRange[2]["patch"]
         put "<" into tExpandedRange[2]["operator"]
      end if
   end if
   
   return tExpandedRange
end __ExpandCaret

private function __ExpandTilde pRangeComparator
   local tExpandedRange
   
   if __IsX(pRangeComparator["major"]) then
      -- ~ --> *
      put empty into tExpandedRange
   else if __IsX(pRangeComparator["minor"]) then
      -- ~1 --> >=1.0.0 <2.0.0
      put pRangeComparator["major"] into tExpandedRange[1]["major"]
      put 0 into tExpandedRange[1]["minor"]
      put 0 into tExpandedRange[1]["patch"]
      put ">=" into tExpandedRange[1]["operator"]
      
      put pRangeComparator["major"] + 1 into tExpandedRange[2]["major"]
      put 0 into tExpandedRange[2]["minor"]
      put 0 into tExpandedRange[2]["patch"]
      put "<" into tExpandedRange[2]["operator"]
   else if __IsX(pRangeComparator["patch"]) then
      -- ~1.2 --> >=1.2.0 <1.3.0
      put pRangeComparator["major"] into tExpandedRange[1]["major"]
      put pRangeComparator["minor"] into tExpandedRange[1]["minor"]
      put 0 into tExpandedRange[1]["patch"]
      put ">=" into tExpandedRange[1]["operator"]
      
      put pRangeComparator["major"] into tExpandedRange[2]["major"]
      put pRangeComparator["minor"] + 1 into tExpandedRange[2]["minor"]
      put 0 into tExpandedRange[2]["patch"]
      put "<" into tExpandedRange[2]["operator"]
   else
      -- ~1.2.3 --> >=1.2.3 <1.3.0
      put pRangeComparator["major"] into tExpandedRange[1]["major"]
      put pRangeComparator["minor"] into tExpandedRange[1]["minor"]
      put pRangeComparator["patch"] into tExpandedRange[1]["patch"]
      put ">=" into tExpandedRange[1]["operator"]
      
      put pRangeComparator["major"] into tExpandedRange[2]["major"]
      put pRangeComparator["minor"] + 1 into tExpandedRange[2]["minor"]
      put 0 into tExpandedRange[2]["patch"]
      put "<" into tExpandedRange[2]["operator"]
   end if
   
   return tExpandedRange
end __ExpandTilde

private function __ExpandXRanges pRangeComparator
   local tExpandedRange
   
   local tMajorIsX, tMinorIsX, tPatchIsX,
   put __IsX(pRangeComparator["major"]) into tMajorIsX
   put __IsX(pRangeComparator["minor"]) into tMinorIsX
   put __IsX(pRangeComparator["patch"]) into tPatchIsX
      
   if not (tMajorIsX or tMinorIsX or tPatchIsX) then
      -- No Xs --> leave as is
      put pRangeComparator into tExpandedRange[1]
   else if tMajorIsX then
      if pRangeComparator["operator"] is among the chars of "><" then
         -- >X -> <0.0.0 (nothing matches)
         -- <X -> <0.0.0
         put 0 into tExpandedRange[1]["major"]
         put 0 into tExpandedRange[1]["minor"]
         put 0 into tExpandedRange[1]["patch"]
         put "<" into tExpandedRange[1]["operator"]
      else
         -- X --> * (everything matches)
         -- >=X --> *
         -- <=X --> *
      end if
   else if pRangeComparator["operator"] is not empty then
      switch pRangeComparator["operator"]
         case ">"
            if tMinorIsX then
               -- >1 --> >=2.0.0
               put pRangeComparator["major"] + 1 into tExpandedRange[1]["major"]
               put 0 into tExpandedRange[1]["minor"]
               put 0 into tExpandedRange[1]["patch"]
               put ">=" into tExpandedRange[1]["operator"]
            else
               -- >1.2 --> >=1.3.0
               put pRangeComparator["major"] into tExpandedRange[1]["major"]
               put pRangeComparator["minor"] + 1 into tExpandedRange[1]["minor"]
               put 0 into tExpandedRange[1]["patch"]
               put ">=" into tExpandedRange[1]["operator"]
            end if
            break
            
         case "<="
            if tMinorIsX then
               -- <=1 --> <2.0.0
               put pRangeComparator["major"] + 1 into tExpandedRange[1]["major"]
               put 0 into tExpandedRange[1]["minor"]
               put 0 into tExpandedRange[1]["patch"]
               put "<" into tExpandedRange[1]["operator"]
            else
               -- <=0.5 --> <0.6.0
               put pRangeComparator["major"] into tExpandedRange[1]["major"]
               put pRangeComparator["minor"] + 1 into tExpandedRange[1]["minor"]
               put 0 into tExpandedRange[1]["patch"]
               put "<" into tExpandedRange[1]["operator"]
            end if
            break
            
         default
            put pRangeComparator into tExpandedRange[1]
            if tMinorIsX then
               put 0 into tExpandedRange[1]["minor"]
            end if
            if tPatchIsX then
               put 0 into tExpandedRange[1]["patch"]
            end if 
            break
      end switch
   else if tMinorIsX then
      -- 1 --> >=1.0.0 <2.0.0
      put pRangeComparator["major"] into tExpandedRange[1]["major"]
      put 0 into tExpandedRange[1]["minor"]
      put 0 into tExpandedRange[1]["patch"]
      put ">=" into tExpandedRange[1]["operator"]
      
      put pRangeComparator["major"] + 1 into tExpandedRange[2]["major"]
      put 0 into tExpandedRange[2]["minor"]
      put 0 into tExpandedRange[2]["patch"]
      put "<" into tExpandedRange[2]["operator"]
   else -- tPatchIsX
      -- 1.2 --> >=1.2.0 <1.3.0
      put pRangeComparator["major"] into tExpandedRange[1]["major"]
      put pRangeComparator["minor"] into tExpandedRange[1]["minor"]
      put 0 into tExpandedRange[1]["patch"]
      put ">=" into tExpandedRange[1]["operator"]
      
      put pRangeComparator["major"] into tExpandedRange[2]["major"]
      put pRangeComparator["minor"] + 1 into tExpandedRange[2]["minor"]
      put 0 into tExpandedRange[2]["patch"]
      put "<" into tExpandedRange[2]["operator"]
   end if
   
   return tExpandedRange
end __ExpandXRanges

command semverParseRange @xRange, pExpand
   local tRange
   put xRange into tRange
   
   replace "||" with " || " in tRange
   repeat while tRange contains "  "
      replace "  " with " " in tRange
   end repeat
   
   local tRangeA, tRangeComparatorCount, tHyphen
   put 1 into tRangeComparatorCount
   put false into tHyphen
   
   local tRangeComparator
   repeat for each word tRangeComparator in tRange
      if tRangeComparator is "||" then
         put tRangeComparator into tRangeA[tRangeComparatorCount]
         add 1 to tRangeComparatorCount
         next repeat
      end if
      
      if tRangeComparator is "-" then
         put ">=" into tRangeA[tRangeComparatorCount - 1]["operator"]
         put true into tHyphen
         next repeat
      end if
      
      __SemverParse tRangeComparator, true
      if the result is not empty then
         return the result for error
      end if
      
      if tHyphen then
         put "<=" into tRangeComparator["operator"]
         put false into tHyphen
      end if
      
      if not pExpand then
         put tRangeComparator into tRangeA[tRangeComparatorCount]
         add 1 to tRangeComparatorCount
      else
         local tExpandedRangeComparatorsA
         switch tRangeComparator["operator"]
            case "^"
               put __ExpandCaret(tRangeComparator) into tExpandedRangeComparatorsA
               break
               
            case "~"
               put __ExpandTilde(tRangeComparator)  into tExpandedRangeComparatorsA
               break
               
            default
               put __ExpandXRanges(tRangeComparator) into tExpandedRangeComparatorsA
               break
         end switch
         
         local tExpandedRangeComparator
         repeat for each element tExpandedRangeComparator in tExpandedRangeComparatorsA
            put tExpandedRangeComparator into tRangeA[tRangeComparatorCount]
            add 1 to tRangeComparatorCount
         end repeat
      end if
   end repeat
   
   put tRangeA into xRange
   return empty for value
end semverParseRange

/**

Determine if a version satisfies a range

Parameters:
pVersion (string): A valid version string
pRange (string): A valid version range string

Returns:
True if a version satisfies the range

*/

function semverSatisfies pVersion, pRange
   if pRange is empty then
      return true
   end if
   
   semverParse pVersion, false
   if the result is not empty then
      return the result for error
   end if
   
   semverParseRange pRange
   if the result is not empty then
      return the result for error
   end if
   
   return __SatisfiesRange(pVersion, pRange)
end semverSatisfies

/**

Determine the maximum version that satisfies a range

Parameters:
pVersions (string): A return delimited list of valid version strings
pRange (string): A valid version range string

Returns:
The highest version satisfying the range

*/

function semverMaxSatisfying pVersions, pRange
   split pVersions by return
   
   if pRange is not empty then
      semverParseRange pRange
      if the result is not empty then
         return the result for error
      end if
   end if
   
   local tVersions
   put pVersions into tVersions
   local tIndex, tFoundIndex
   repeat with tIndex = 1 to the number of elements of pVersions
      semverParse tVersions[tIndex], false
      if the result is not empty then
         return the result for error
      end if
      
      if pRange is not empty and not __SatisfiesRange(tVersions[tIndex], pRange) then
         next repeat
      end if
      
      if tFoundIndex is empty or __GreaterThan(tVersions[tIndex], tVersions[tFoundIndex]) then
         put tIndex into tFoundIndex
      end if
      
   end repeat
   
   -- will return empty if tFoundIndex is empty
   return pVersions[tFoundIndex]
end semverMaxSatisfying

/**

Determine the minimum version that satisfies a range

Parameters:
pVersions (string): A return delimited list of valid version strings
pRange (string): A valid version range string

Returns:
The lowest version satisfying the range

*/

function semverMinSatisfying pVersions, pRange
   split pVersions by return
   
   if pRange is not empty then
      semverParseRange pRange
      if the result is not empty then
         return the result for error
      end if
   end if
   
   local tVersions
   put pVersions into tVersions
   local tIndex, tFoundIndex
   repeat with tIndex = 1 to the number of elements of pVersions
      semverParse tVersions[tIndex], false
      if the result is not empty then
         return the result for error
      end if
      
      if pRange is not empty and not __SatisfiesRange(tVersions[tIndex], pRange) then
         next repeat
      end if
      
      if tFoundIndex is empty or __GreaterThan(tVersions[tFoundIndex], tVersions[tIndex]) then
         put tIndex into tFoundIndex
      end if
      
   end repeat
   
   -- will return empty if tFoundIndex is empty
   return pVersions[tFoundIndex]
end semverMinSatisfying

---------------------------------------------------
-- RANGE INTERSCTION

private function __RangeComparatorsIntersect pLeftRangeComparator pRightRangeComparator
   -- If there is no operator for a range comparator we can treat it like a
   -- version string and simply check if it satifies the other range comparator.
   local tRange
   if pLeftRangeComparator["operator"] is empty then
      put pRightRangeComparator into tRange[1]
      return __SatisfiesRange(pLeftRangeComparator, tRange)
   else if pRightRangeComparator["operator"] is empty then
      put pLeftRangeComparator into tRange[1]
      return __SatisfiesRange(pRightRangeComparator, tRange)
   end if
   
   -- Intersect if range comparator operators are same direction and increasing.
   -- e.g. >=1.2.3 intersects with >4.5.6
   if char 1 of pLeftRangeComparator["operator"] is ">" and \
         char 1 of pRightRangeComparator["operator"] is ">" \
         then
      return true
   end if
   
   -- Intersect if range comparator operators are same direction and decreasing.
   -- e.g. <1.2.3 intersects with <=4.5.6
   if char 1 of pLeftRangeComparator["operator"] is "<" and \
         char 1 of pRightRangeComparator["operator"] is "<" \
         then
      return true
   end if
   
   local tSameSemVer
   put __Equal(pLeftRangeComparator, pRightRangeComparator) into tSameSemVer
   
   local tDifferentDirectionsInclusive
   put (pLeftRangeComparator["operator"] is ">=" or pLeftRangeComparator["operator"] is "<=") and \
         (pRightRangeComparator["operator"] is ">=" or pRightRangeComparator["operator"] is "<=") \
         into tDifferentDirectionsInclusive
   
   -- Intersect if range comparators have the same version and operators are >= or <=.
   -- e.g. >=1.2.3 intersects with <=1.2.3
   if tSameSemVer and tDifferentDirectionsInclusive then
      return true
   end if
   
   -- Intersect if range comparator operators are converging with the left less than right.
   -- e.g. >=1.2.3 intersects with <4.5.6
   if __CompareWithOp(pLeftRangeComparator, pRightRangeComparator, "<") and \
         char 1 of pLeftRangeComparator["operator"] is ">" and \
         char 1 of pRightRangeComparator["operator"] is "<" \
         then
      return true
   end if
   
   -- Intersect if range comparator operators are diverging with the left greater than right.
   -- e.g. <4.5.6 intersects with >1.2.3
   if __CompareWithOp(pLeftRangeComparator, pRightRangeComparator, ">") and \
         char 1 of pLeftRangeComparator["operator"] is "<" and \
         char 1 of pRightRangeComparator["operator"] is ">" \
         then
      return true
   end if
   
   return false
end __RangeComparatorsIntersect

-- Return the set of comparators that intersect with every element from two comparator sets.
--
-- Left =
--   [1] = >=1.2.3
--   [2] = <=4.5.6
-- Right =
--   [1] = >=1.2.3
--   [2] = <=7.8.9
-- Return =
--   [1] = >=1.2.3
--   [2] = <=4.5.6
--   [3] = <=7.8.9
--
-- If there is no intersection between the comparator sets, empty is returned.
--
-- Left =
--   [1] = >=1.2.3
--   [2] = <=4.5.6
-- Right =
--   [1] = >=7.8.9
-- Return = empty
private function __GetComparatorsIntersetingTwoComparatorSets pLeftComparatorsA, pRightComparatorsA
   local tIntersectingComparatorsA, tIntersectingComparatorsIndex
   put 1 into tIntersectingComparatorsIndex
   
   local tLeftComparator
   repeat for each element tLeftComparator in pLeftComparatorsA
      local tRightComparator
      repeat for each element tRightComparator in pRightComparatorsA
         if __RangeComparatorsIntersect(tLeftComparator, tRightComparator) then
            put tLeftComparator into tIntersectingComparatorsA[tIntersectingComparatorsIndex]
            add 1 to tIntersectingComparatorsIndex
            put tRightComparator into tIntersectingComparatorsA[tIntersectingComparatorsIndex]
            add 1 to tIntersectingComparatorsIndex
         else
            return empty
         end if
         
      end repeat
   end repeat
   
   return tIntersectingComparatorsA
end __GetComparatorsIntersetingTwoComparatorSets

-- Return the (formatted) range that intersects two ranges.
--
-- Note, the returned range will be verbose (i.e. it will just
-- contain the intersecting comparators and will need futher parsing
-- to convert into a valid range).
--
-- Left   = 1.2.3 || 4.5.6
-- Right  = 4.5.6 || 1.0.0 - 1.2.3
-- Return = 1.2.3 >=1.0.0 <=1.2.3 || 4.5.6
--
-- If the two ranges don't intersect, empty will be returned.
private function __GetRangeIntersectingTwoRanges pLeftRange, pRightRange
   local tLeftRangeConditionsA, tRightRangeConditionsA
   put __RangeToConditionSet(pLeftRange) into tLeftRangeConditionsA
   put __RangeToConditionSet(pRightRange) into tRightRangeConditionsA
   
   local tIntersectingConditionsA, tIntersectingConditionsIndex
   put 1 into tIntersectingConditionsIndex
   
   -- Compare each individual conditions with each other in the left and
   -- right condition sets, fidning the interrsecting comparators (if any).
   local tLeftComparatorsA
   repeat for each element tLeftComparatorsA in tLeftRangeConditionsA
      local tRightComparatorsA
      repeat for each element tRightComparatorsA in tRightRangeConditionsA
         put __GetComparatorsIntersetingTwoComparatorSets(tLeftComparatorsA, tRightComparatorsA) \
               into tIntersectingConditionsA[tIntersectingConditionsIndex]
         add 1 to tIntersectingConditionsIndex
      end repeat
   end repeat
   
   -- We should now have a set of intersecting comparators for each of the
   -- conditions compared. Format this into a range by adding ||s between
   -- conditions. If the comparator set for a condition is empty, then that
   -- condition did not intersect, so ignore.
   --
   -- For two ranges to intersect, at least one of the individual conditions
   -- compared must intersect (i.e. tResult will be true).
   local tResult, tIntersectingRange
   put true into tResult
   repeat with tIntersectingConditionsIndex = 1 to the number of elements in tIntersectingConditionsA
      put tResult or tIntersectingConditionsA[tIntersectingConditionsIndex] is not empty into tResult
      
      if tIntersectingConditionsA[tIntersectingConditionsIndex] is not empty then
         local tIntersectingRangeIndex
         __AppendToRange tIntersectingRange, tIntersectingConditionsA[tIntersectingConditionsIndex]
         put the result into tIntersectingRangeIndex
         put "||" into tIntersectingRange[tIntersectingRangeIndex]
      end if
   end repeat
   
   -- Clean up any trailing ||.
   if the number of elements in tIntersectingRange > 1 and \
         tIntersectingRange[the number of elements in tIntersectingRange] is "||" \
         then
      delete variable tIntersectingRange[the number of elements in tIntersectingRange]
   end if
   
   if tResult then
      return tIntersectingRange
   end if
   
   return empty
end __GetRangeIntersectingTwoRanges

private function __RangesIntersect pLeftRange, pRightRange
   return __GetRangeIntersectingTwoRanges(pLeftRange, pRightRange) is not empty
end __RangesIntersect

function __GetIntersectionOfRangeSet pRangesA
   local tIntersectingRange, tRangeIndex
   put pRangesA[1] into tIntersectingRange
   repeat with tRangeIndex = 2 to the number of elements in pRangesA
      put __GetRangeIntersectingTwoRanges(tIntersectingRange, pRangesA[tRangeIndex]) \
            into tIntersectingRange
      if tIntersectingRange is empty then
         return empty
      end if
   end repeat
   
   return tIntersectingRange
end __GetIntersectionOfRangeSet

/**

Determine if a set of ranges intersect

Description:
The function semverRangesIntersect returns true if all the given ranges
intersect. Any number of ranges can be checked, passing each range as a
parameter.

Parameters:
pRange (string): A valid version range string

Returns:
True if all the given ranges intersect

The result:
An error string if any range failed to parse.

*/

function semverRangesIntersect pRange...
   -- Any number of ranges can be passed for checking.
   -- Parse each param as a ranges into a single array.
   local tParamIndex, tRangesIndex, tRangesA
   put 1 into tRangesIndex
   repeat with tParamIndex = 1 to the paramCount
      local tRange
      put param(tParamIndex) into tRange
      semverParseRange tRange, true
      if the result is not empty then
         return the result for error
      end if
      
      if tRange is not empty then
         put tRange into tRangesA[tRangesIndex]
         add 1 to tRangesIndex
      end if
   end repeat
   
   return __GetIntersectionOfRangeSet(tRangesA) is not empty for value
end semverRangesIntersect

private function __FormatBoundsIntoShorthandString pLowerBound, pUpperBound
   if pLowerBound is empty and pUpperBound is empty then
      return empty
   end if
   
   if __Equal(pLowerBound, pUpperBound) then
      -- >=1.2.3 <=1.2.3 --> 1.2.3
      if pLowerBound["operator"] is ">=" and pUpperBound["operator"] is "<=" then
         return __ParsedSemverToString(pLowerBound)
      end if
      
      -- 1.2.3 1.2.3 --> 1.2.3
      -- >1.2.3 >1.2.3 --> >1.2.3
      if pLowerBound["operator"] is pUpperBound["operator"] then
         return pLowerBound["operator"] & __ParsedSemverToString(pLowerBound)
      end if
   end if
   
   -- >=1.2.3 <=4.5.6 --> 1.2.3 - 4.5.6
   if pLowerBound["operator"] is ">=" and pUpperBound["operator"] is "<=" then
      return __ParsedSemverToString(pLowerBound) && "-" && __ParsedSemverToString(pUpperBound)
   end if
   
   if pLowerBound["major"] is 0 and pUpperBound["major"] is 0 then
      -- >=0.0.1 <0.0.2 --> ^0.0.1
      if  pLowerBound["minor"] is 0 and pUpperBound["minor"] is 0 and \
            pUpperBound["patch"] is pLowerBound["patch"] + 1 and \
            pLowerBound["operator"] is ">=" and pUpperBound["operator"] is "<" \
            then
         return "^" & __ParsedSemverToString(pLowerBound)
      end if
      
      -- >=0.1.1 <0.2.0 --> ^0.1.1
      if  pUpperBound["minor"] is pLowerBound["minor"] + 1 and \
            pUpperBound["patch"] is 0 and \
            pLowerBound["operator"] is ">=" and pUpperBound["operator"] is "<" \
            then
         return "^" & __ParsedSemverToString(pLowerBound)
      end if
   end if
   
   -- >=1.2.3 <1.3.0 --> ~1.2.3
   if pLowerBound["major"] is pUpperBound["major"] and \
         pUpperBound["minor"] is pLowerBound["minor"] + 1 and \
         pUpperBound["patch"] is 0 and \
         pLowerBound["operator"] is ">=" and pUpperBound["operator"] is "<" \
         then
      return "~" & __ParsedSemverToString(pLowerBound)
   end if
   
   -- >=1.2.3 <2.0.0 --> ^1.2.3
   if pUpperBound["major"] is pLowerBound["major"] + 1 and \
         pUpperBound["minor"] is 0 and pUpperBound["patch"] is 0 and \
         pLowerBound["operator"] is ">=" and pUpperBound["operator"] is "<" \
         then
      return "^" & __ParsedSemverToString(pLowerBound)
   end if
   
   local tFormatted
   if pLowerBound is not empty then
      put pLowerBound["operator"] & __ParsedSemverToString(pLowerBound) into tFormatted
      if pUpperBound is not empty then
         put space after tFormatted
      end if
   end if
   if pUpperBound is not empty then
      put pUpperBound["operator"] & __ParsedSemverToString(pUpperBound) after tFormatted
   end if
   return tFormatted
end __FormatBoundsIntoShorthandString

private function __RangeComparatorIsCompatibleWithBound pRangeComparator, pBound
   if pBound is empty then
      return true
   end if
   
   local tRange
   put pRangeComparator into tRange[1]
   if __SatisfiesRange(tRange, pBound) and __RangeComparatorsIntersect(pRangeComparator, pBound) then
      return true
   end if
   
   return false
end __RangeComparatorIsCompatibleWithBound

private function __GetBoundsOfComparatorSet pComparatorsA
   -- Attempt to find the upper and lower bounds in the array of range comparators.
   local tLowerBound, tUpperBound
   repeat for each element tRangeComparator in pComparatorsA
      -- Since we've fully expanded the ranges, the only operators will be =,>,>=,<,<=.
      switch tRangeComparator["operator"]
         case "="
         case empty
            if not __RangeComparatorIsCompatibleWithBound(tRangeComparator, tLowerBound) or \
                  not __RangeComparatorIsCompatibleWithBound(tRangeComparator, tUpperBound) then
               next repeat
            end if
            
            -- A range comparator with the equals operator limits the upper and lower bounds
            -- to its value.
            put tRangeComparator into tLowerBound
            put ">=" into tLowerBound["operator"]
            put tRangeComparator into tUpperBound
            put "<=" into tUpperBound["operator"]
            break
            
         case ">"
         case ">="
            if not __RangeComparatorIsCompatibleWithBound(tRangeComparator, tUpperBound) then
               next repeat
            end if
            
            -- A range comparator with a greater than or greater than or equals operator could
            -- potentially be a new lower bound. Check to see if it is less than the current
            -- lower bound.
            if tLowerBound is empty or __GreaterThan(tRangeComparator, tLowerBound) or \
                  (tRangeComparator["operator"] is ">=" and __Equal(tRangeComparator, tLowerBound)) then
               put tRangeComparator into tLowerBound
            end if
            break
            
         case "<"
         case "<="
            if not __RangeComparatorIsCompatibleWithBound(tRangeComparator, tLowerBound) then
               next repeat
            end if
            
            -- A range comparator with a less than or less than or equals operator could
            -- potentially be a new upper bound. Check to see if it is greater than the current
            -- upper bound.
            if tUpperBound is empty or __GreaterThan(tUpperBound, tRangeComparator) or \
                  (tRangeComparator["operator"] is "<=" and __Equal(tRangeComparator, tUpperBound)) then
               put tRangeComparator into tUpperBound
            end if
            break
      end switch
   end repeat
   
   local tBoundsA
   put tLowerBound into tBoundsA["lower"]
   put tUpperBound into tBoundsA["upper"]
   return tBoundsA
end __GetBoundsOfComparatorSet

/**

Determine the range that intersects the given ranges

Description:
The function semverRangesIntersection returns a string that defines the
semver range that intersects all the given ranges. Any number of ranges
can be checked, passing each range as a parameter.

Parameters:
pRange (string): A valid version range string

Returns:
The range that intersects all the given ranges. Empty if the ranges do
not intersect.

The result:
An error string if any range failed to parse.

*/

function semverRangesIntersection pRange1, pRange2...
   -- Any number of ranges can be passed for checking.
   -- Parse each param as a ranges into a single array.
   local tParamIndex, tRangesIndex, tRangesA
   put 1 into tRangesIndex
   repeat with tParamIndex = 1 to the paramCount
      local tRange
      put param(tParamIndex) into tRange
      semverParseRange tRange, true
      if the result is not empty then
         return the result for error
      end if
      
      if tRange is not empty then
         put tRange into tRangesA[tRangesIndex]
         add 1 to tRangesIndex
      end if
   end repeat
   
   local tIntersectingRange
   put __GetIntersectionOfRangeSet(tRangesA) into tIntersectingRange
   if tIntersectingRange is empty then
      return empty for value
   end if
   
   local tIntersectingConditionsA
   put __RangeToConditionSet(tIntersectingRange) into tIntersectingConditionsA
   
   -- We now have a set of conditions that intersect the ranges.
   -- Get the lower and upper bounding comparators for each condition in the set.
   local tRangeBoundsA, tConditionsIndex
   repeat with tConditionsIndex = 1 to the number of elements in tIntersectingConditionsA
      put __GetBoundsOfComparatorSet(tIntersectingConditionsA[tConditionsIndex]) \
            into tRangeBoundsA[tConditionsIndex]
   end repeat
   
   -- We've found the bounds, now format them into a shorthand range string.
   local tFormattedBoundsString, tBound
   repeat for each element tBound in tRangeBoundsA
      local tFormattedBound
      put __FormatBoundsIntoShorthandString(tBound["lower"],\
            tBound["upper"]) into tFormattedBound
      
      if tFormattedBound is not among the lines of tFormattedBoundsString then
         if tFormattedBoundsString is not empty then
            put return after tFormattedBoundsString
         end if
         put tFormattedBound after tFormattedBoundsString
      end if
   end repeat
   
   replace return with " || " in tFormattedBoundsString
   return tFormattedBoundsString for value
end semverRangesIntersection

---------------------------------------------------
-- UTILITY

private command __AppendToRange @xRange, pRangeToAppend
   local tRangeIndex
   put the number of elements in xRange + 1 into tRangeIndex
   
   repeat for each element tComparator in pRangeToAppend
      put tComparator into xRange[tRangeIndex]
      add 1 to tRangeIndex
   end repeat
   
   return tRangeIndex
end __AppendToRange

private function __CompareWithOp pLeft, pRight, pOperator
   switch pOperator
      case "="
         return __Equal(pLeft, pRight)
      case ">"
         return __GreaterThan(pLeft, pRight)
      case "<"
         return __GreaterThan(pRight, pLeft)
      case ">="
         return __Equal(pLeft, pRight) or __GreaterThan(pLeft, pRight)
      case "<="
         return __Equal(pLeft, pRight) or __GreaterThan(pRight, pLeft)
      default
         return false
   end switch
end __CompareWithOp

private function __RangeToConditionSet pRange
   -- Split a parsed range into an set of conditions by splitting on "or".
   -- Condition sets are a little cleaner to work with.
   --
   -- A range is an array of comparators split into conditions by "||" elements.
   --
   -- Range =
   --   [1] = comparator 1
   --   [2] = comparator 2
   --   [3] = "||"
   --   [4] = comparator 3
   --
   -- A condition set splits the individual conditions into sub arrays
   -- containing the comprators for that condition.
   --
   -- Condition Set =
   --   [1] =
   --     [1] = comparator 1
   --     [2] = comparator 2
   --   [2] =
   --     [1] comparator 3
   local tConditionsA, tConditionsIndex, tRangeIndex
   put 1 into tConditionsIndex
   repeat with tRangeIndex = 1 to the number of elements in pRange
      if pRange[tRangeIndex] is "||" then
         add 1 to tConditionsIndex
      else
         local tRange
         put pRange[tRangeIndex] into tRange[1]
         __AppendToRange tConditionsA[tConditionsIndex], tRange
      end if
   end repeat
   
   return tConditionsA
end __RangeToConditionSet

private function __RangeFromConditionSet pConditionsA
   local tRange, pConditionsIndex
   repeat with pConditionsIndex = 1 to the number of elements in pConditionsA
      local tRangeIndex
      __AppendToRange tRange, pConditionsA[pConditionsIndex]
      put the result into tRangeIndex
      put "||" into tRange[tRangeIndex]
   end repeat
   
   if tRange[the number of elements in tRange] is "||" then
      delete variable tRange[the number of elements in tRange]
   end if
   return tRange
end __RangeFromConditionSet

private function __ParsedSemverToString pSemVer
   local tString
   if not __IsX(pSemVer["major"]) then
      put pSemVer["major"] into tString
      if not __IsX(pSemVer["minor"]) then
         put "." & pSemVer["minor"] after tString
         if not __IsX(pSemVer["patch"]) then
            put "." & pSemVer["patch"] after tString
         end if
      end if
   end if
   return tString
end __ParsedSemverToString
