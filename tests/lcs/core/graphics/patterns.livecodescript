script "CoreGraphicsPatterns"
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

on TestSetup
   TestSkipIfNot "ide"
   get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")
end TestSetup

on TestPatternExistence
   local tPattern
   -- patternNumbers 1 to 164 are id 137 to 300
   local tPI_PATTERNS = 136
   repeat with tPattern = 1 to 164
      resolve image id tPattern+tPI_PATTERNS relative to me
      if not (tPattern+tPI_PATTERNS >= 241 and tPattern+tPI_PATTERNS <= 250) and tPattern+tPI_PATTERNS < 282 then
         TestAssert "patternNumber" && tPattern && "exists", it is not empty
         set the brushPattern to tPattern
         TestAssert "patternNumber" && tPattern && "round trips", the brushPattern is tPattern
      else
         TestAssert "patternNumber" && tPattern && "documented as unavailable", it is empty
         TestAssertThrow "patternNumber" && tPattern && "unavailable", \ 
            "SetPatternToNonExistantImage", the long id of me, \ 
            "EE_PROPERTY_BRUSHPATNOIMAGE", tPattern
      end if 
   end repeat
end TestPatternExistence

on SetPatternToNonExistantImage pPattern
   set the brushPattern to pPattern
end SetPatternToNonExistantImage

on TestPatterns
   local tPattern
   
   repeat for each word tPattern in "brushPattern penPattern backdrop"
      PatternTests tPattern, "global"
   end repeat
   
   -- object patterns
   repeat for each word tPattern in "backPattern shadowPattern topPattern hilitePattern forePattern focusPattern bottomPattern borderPattern"
      PatternTests tPattern, "object"
   end repeat
end TestPatterns

on PatternTests pPattern, pType
   local tUnsetValue
   if pPattern is "backdrop" then
      put "none" into tUnsetValue
   end if
   
   TestAssert pType && pPattern && "is initially empty", GetPattern(pPattern) is tUnsetValue
   
   SetPattern pPattern, 100
 	TestAssert pType && pPattern && "round trips <= 164", GetPattern(pPattern) is 100
	
   SetPattern pPattern, 0
   TestAssert pType && pPattern && "0 == empty", GetPattern(pPattern) is tUnsetValue

   SetPattern pPattern, 360
 	TestAssert pType && pPattern && "round trips > 300", GetPattern(pPattern) is 360
	
   SetPattern pPattern, empty
   TestAssert pType && pPattern && "set to empty", GetPattern(pPattern) is tUnsetValue

   SetPattern pPattern, 200
 	TestAssert pType && pPattern && ">= 136 and <= 300 subtracts 136", GetPattern(pPattern) is 64
end PatternTests


on SetPattern pPattern, pValue
   switch pPattern
   case "backPattern"
      set the backPattern of me to pValue
      break
   case "shadowPattern"
      set the shadowPattern of me to pValue
      break
   case "topPattern"
      set the topPattern of me to pValue
      break
   case "hilitePattern"
      set the hilitePattern of me to pValue
      break
   case "forePattern"
      set the forePattern of me to pValue
      break
   case "focusPattern"
      set the focusPattern of me to pValue
      break
   case "bottomPattern"
      set the bottomPattern of me to pValue
      break
   case "borderPattern"
      set the borderPattern of me to pValue
      break
   case "penPattern"
      set the penPattern to pValue
      break
   case "brushPattern"
      set the brushPattern to pValue
      break
   case "backdrop"
      set the backdrop to pValue
      break
   end switch
end SetPattern

function GetPattern pPattern
   switch pPattern
      case "backPattern"
         return the backPattern of me
         break
      case "shadowPattern"
         return the shadowPattern of me
         break
      case "topPattern"
         return the topPattern of me
         break
      case "hilitePattern"
         return the hilitePattern of me
         break
      case "forePattern"
         return the forePattern of me
         break
      case "focusPattern"
         return the focusPattern of me
         break
      case "bottomPattern"
         return the bottomPattern of me
         break
      case "borderPattern"
         return the borderPattern of me
         break
      case "penPattern"
         return the penPattern
         break
      case "brushPattern"
         return the brushPattern
      case "backdrop"
         return the backdrop
         break
      end switch
end GetPattern

on TestTeardown
   if "StandaloneTestRunnerMainstack" is not among the lines of the mainstacks then
      delete stack "revIcons"
   end if
end TestTeardown
