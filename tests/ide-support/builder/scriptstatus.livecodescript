script "TestBuilderScriptStatus"
/*
Copyright (C) 2018 LiveCode Ltd.

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

local sTestStack

on TestSetup
   TestSkipIf "standalone"
   create stack uuid()
   put it into sTestStack
end TestSetup

private function BuilderFolder
    return TestGetEngineRepositoryPath() & slash & "builder"
end BuilderFolder

private command addToList pValue, @xList
    put pValue into xList[the number of elements in xList + 1]
end addToList

private function ListBuilderStacks
	local tList
    ListBuilderStacksRecursive BuilderFolder(), tList
    return tList
end ListBuilderStacks

private command ListBuilderStacksRecursive pFolder, @xList
    local tStacks
	put files(pFolder) into tStacks
	filter tStacks with regex pattern "^.*\.(rev|livecode(script)?)$"
	repeat for each line tLine in tStacks
	   addToList pFolder & slash & tLine, xList
	end repeat
	
	local tSubFolder
	repeat for each line tSubFolder in folders(pFolder)
	    if tSubFolder begins with "." then next repeat
		ListBuilderStacksRecursive pFolder & slash & tSubFolder, xList
	end repeat
end ListBuilderStacksRecursive

on TestExplicitVariables
   local tStackFiles
   put ListBuilderStacks() into tStackFiles
   repeat for each element tStackName in tStackFiles
      if there is a stack tStackName then
         __RecursiveTest the long id of stack tStackName
      else
      	 TestAssert "compiles" && tStackName, false
      end if
   end repeat
end TestExplicitVariables

private command __RecursiveTest pObject
   if the script of pObject is not empty then
      lock messages
      local tStatus
      put the scriptStatus of pObject into tStatus
      switch tStatus
         case "uncompiled"
            set the script of sTestStack to the script of pObject
            if the scriptStatus of sTestStack is "compiled" then
               put "compiled" into tStatus
               break
            end if
         case "error"
            TestAssert "compiles" && the long name of pObject, false
            break
      end switch
      
      if tStatus is "compiled" then
         set the explicitVariables to true
         set the script of sTestStack to the script of pObject
         TestDiagnostic the result
         set the explicitVariables to false
         TestAssert "explicit variables" && the long name of pObject, \
         	the scriptStatus of sTestStack is "compiled"
      end if
      unlock messages
   end if
   switch word 1 of pObject
      case "stack"
         repeat for each line tStack in the substacks of pObject
            __RecursiveTest the long id of stack tStack
         end repeat
         repeat for each line tID in the cardIDs of pObject
            __RecursiveTest the long id of card id tID of pObject
         end repeat
         repeat for each line tID in the sharedGroupIDs of pObject
            __RecursiveTest the long id of control id tID of pObject
         end repeat
         break
      case "card"
         repeat for each line tID in the childControlIDs of pObject
            if word 1 of the name of control id tID of pObject is "group" and \
                  the sharedBehavior of control id tID of pObject then
               next repeat
            end if
            __RecursiveTest the long id of control id tID of pObject
         end repeat
         break
      case "group"
      case "background"
      case "bkgnd"
         repeat for each line tID in the childControlIDs of pObject
            __RecursiveTest the long id of control id tID of pObject
         end repeat
         break
      default
   end switch
end __RecursiveTest

on TestTeardown
   delete stack sTestStack
end TestTeardown
