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

private command __SemverSplitRange @xRange
   replace "||" with " || " in xRange
   repeat while xRange contains "  "
      replace "  " with " " in xRange
   end repeat
   split xRange by space
end __SemverSplitRange

command semverParseRange @xRange
   __SemverSplitRange xRange
   
   local tHyphen = false
   repeat with tIndex = 1 to the number of elements of xRange
      if xRange[tIndex] is "||" then
         next repeat
      end if
      if xRange[tIndex] is "-" then
         put ">=" into xRange[tIndex-1]["operator"]
         put true into tHyphen
         next repeat
      end if
      
      __SemverParse xRange[tIndex], true
      if the result is not empty then
         return the result for error
      end if
      
      if tHyphen then
         put "<=" into xRange[tIndex]["operator"]
         put false into tHyphen
      end if
   end repeat
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
