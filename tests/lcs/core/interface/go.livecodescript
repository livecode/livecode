script "CoreInterfaceGo"
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

on TestGoStackUnquotedName
   -- unquoted literal chunk parsing exception for `go stack "stack foo"`
   local tStackName
   create stack "stackToGo"
   put the short name of it into tStackName
   
   local tToGo
   put "stack stackToGo" into tToGo
   
   try
      go tToGo
   catch tError
   end try
   
   TestAssert "unquoted literal chunk parsing exception for go stack", \
      tError is empty
end TestGoStackUnquotedName

on TestGoCardUnquotedName
   -- unquoted literal chunk parsing exception for `go "card foo of this stack"`
   local tStackName
   create stack
   put the short name of it into tStackName
   set the defaultStack to tStackName
   
   create card "cardToGo"
   local tToGo
   put "card cardToGo of" && the long id of stack tStackName into tToGo
   
   try
      go tToGo
   catch tError
   end try
   
   TestAssert "unquoted literal chunk parsing exception for go card", \
      tError is empty
end TestGoCardUnquotedName

local sStackFileName
command _TestCreateStack pBinary, pVisible
   local tStack
   put "stackToGo" into tStack
   
   if pBinary then
      create stack tStack
      set the visible of stack tStack to pVisible
   else
      create script only stack tStack
      if pVisible then
         put "on preOpenStack" & return & "set the visible of me to" && pVisible & return & "end preOpenStack" into tScript
         set the script of stack tStack to tScript
      end if
   end if
   
   put the tempname into sStackFileName
   save stack tStack as sStackFileName
   delete stack tStack
   return tStack for value
end _TestCreateStack

command _TestCleanupStack
   delete file sStackFileName
end _TestCleanupStack

command _TestGoStackUrl pWhich
   TestSkipIfNot "write"
   local tStack
   _TestCreateStack pWhich is "binary", true
   put it into tStack
   go url ("file:" & sStackFileName)
   TestDiagnostic the result
   TestAssert "go" && pWhich && "stack url", there is a stack tStack
   _TestCleanupStack
end _TestGoStackUrl

on TestGoBinaryStackUrl
   _TestGoStackUrl "binary"
end TestGoBinaryStackUrl

on TestGoScriptOnlyStackUrl
   _TestGoStackUrl "script only"
end TestGoScriptOnlyStackUrl

on TestGoTwice
   create stack
   lock screen
   go it
   go it
   unlock screen
   TestAssert "go stack twice does not cause crash", true
end TestGoTwice


on TestGoToCard
	create stack
	set the defaultstack to the short name of it
	create card
	set the script of this card to \ 
		"on closeCard; set the cClosed of me to true; end closeCard"
	set the script of card 1 to \
		"on openCard; set the cOpened of me to true; end openCard"
	go card 1
	
	TestAssert "openCard sent to opened card", the cOpened of card 1
	TestAssert "closeCard sent to closed card", the cClosed of card 2
end TestGoToCard

on _TestGoInvisibleVisibleStack pMode, pWhich, pVisible
   TestSkipIfNot "write"
   local tStack
   _TestCreateStack pWhich is "binary", pVisible is "visible"
   put it into tStack
   do "go" && pMode && "stack sStackFilename"
   TestDiagnostic the result
   if pMode is empty then
      put pVisible into pExpectedVisibility
   else if pMode is "invisible" and pWhich is "script only" and pVisible is "visible" then
      --a special case:
      --a visible script only stack will be visible even if go invisible
      --is used because the stack becomes visible by a "set the visible..."
      --line in preOpenStack or openStack. So the setting of the visibility
      --by the engine will be overridden when the stack is opened
      put "visible" into pExpectedVisibility
   else
      put pMode into pExpectedVisibility
   end if
   TestAssert "go" && pMode && "on a" && pVisible && pWhich && "stack opened", there is a stack tStack
   TestAssert "go" && pMode && "on a" && pVisible && pWhich && "stack is" && pExpectedVisibility, the visible of stack tStack is (pExpectedVisibility is "visible")
   _TestCleanupStack
end _TestGoInvisibleVisibleStack

on TestGoVisibleOnVisibleBinary
   _TestGoInvisibleVisibleStack "visible", "binary", "visible"
end TestGoVisibleOnVisibleBinary

on TestGoVisibleOnInvisibleBinary
   _TestGoInvisibleVisibleStack "visible", "binary", "invisible"
end TestGoVisibleOnInvisibleBinary

on TestGoVisibleOnVisibleScriptOnly
   _TestGoInvisibleVisibleStack "visible", "script only", "visible"
end TestGoVisibleOnVisibleScriptOnly

on TestGoVisibleOnInvisibleScriptOnly
   _TestGoInvisibleVisibleStack "visible", "script only", "invisible"
end TestGoVisibleOnInvisibleScriptOnly

on TestGoInvisibleOnVisibleBinary
   _TestGoInvisibleVisibleStack "invisible", "binary", "visible"
end TestGoInvisibleOnVisibleBinary

on TestGoInvisibleOnInvisibleBinary
   _TestGoInvisibleVisibleStack "invisible", "binary", "invisible"
end TestGoInvisibleOnInvisibleBinary

on TestGoInvisibleOnVisibleScriptOnly
   _TestGoInvisibleVisibleStack "invisible", "script only", "visible"
end TestGoInvisibleOnVisibleScriptOnly

on TestGoInvisibleOnInvisibleScriptOnly
   _TestGoInvisibleVisibleStack "invisible", "script only", "invisible"
end TestGoInvisibleOnInvisibleScriptOnly

on TestGoRegularOnVisibleBinary
   _TestGoInvisibleVisibleStack "", "binary", "visible"
end TestGoRegularOnVisibleBinary

on TestGoRegularOnInvisibleBinary
   _TestGoInvisibleVisibleStack "", "binary", "invisible"
end TestGoRegularOnInvisibleBinary

on TestGoRegularOnVisibleScriptOnly
   _TestGoInvisibleVisibleStack "", "script only", "visible"
end TestGoRegularOnVisibleScriptOnly

on TestGoRegularOnInvisibleScriptOnly
   _TestGoInvisibleVisibleStack "", "script only", "invisible"
end TestGoRegularOnInvisibleScriptOnly
