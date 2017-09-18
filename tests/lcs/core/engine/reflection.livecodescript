script "CoreEngineReflection"
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

on TestSetup
	create button "IsolatedBehavior"
	set the script of it to "command myBehaviorHandler; end myBehaviorHandler; private command myPrivateBehaviorHandler; end myPrivateBehaviorHandler"

	create button "ChainedBehavior"
	set the script of it to "command myChainedBehaviorHandler; end myChainedBehaviorHandler"
	set the behavior of it to the long id of button "IsolatedBehavior"

	create button "TestButton"
	set the script of it to "command myHandler; end myHandler; private command myPrivateHandler; end myPrivateHandler"
end TestSetup

on TestTeardown
	delete button "TestButton"
	delete button "ChainedBehavior"
	delete button "IsolatedBehavior"
end TestTeardown

on TestRevAvailableHandlers
	set the behavior of button "TestButton" to the long id of button "IsolatedBehavior"
	TestAssert "lists handlers only in object script", the revAvailableHandlers of button "TestButton" contains " myHandler "
	TestAssert "lists private handlers only in object script", the revAvailableHandlers of button "TestButton" contains " myPrivateHandler "
end TestRevAvailableHandlers

on TestRevEffectiveAvailableHandlersBehavior
	set the behavior of button "TestButton" to the long id of button "IsolatedBehavior"
	TestAssert "lists object's handlers", the effective revAvailableHandlers of button "TestButton" contains " myHandler "
	TestAssert "lists object's private handlers", the effective revAvailableHandlers of button "TestButton" contains " myPrivateHandler "
	TestAssert "lists object's behavior handlers", the effective revAvailableHandlers of button "TestButton" contains " myBehaviorHandler "
	TestAssert "does not list object's behavior private handlers", not (the effective revAvailableHandlers of button "TestButton" contains " myPrivateBehaviorHandler ")
end TestRevEffectiveAvailableHandlersBehavior

on TestRevEffectiveAvailableHandlersChainedBehavior
	set the behavior of button "TestButton" to the long id of button "ChainedBehavior"
	TestAssert "lists object's chained behavior handlers", the effective revAvailableHandlers of button "TestButton" contains " myChainedBehaviorHandler "
end TestRevEffectiveAvailableHandlersChainedBehavior

on TestRevEffectiveAvailableHandlersBlockedBehavior
	TestSkipIf "platform", "HTML5"
	-- Behaviors can only become 'blocked' when deserialized. Therefore, we
	-- do this by creating a new behavior and user of the behavior, copying the
	-- user to the clipboard (which serializes), then deleting both, and then
	-- pasting to deserialize. At this point the new button's behavior will not
	-- resolve and will be marked as blocked.
	local tBehaviorButton
	create button "BehaviorButton"
	set the script of it to "command myBehaviorHandler; end myBehaviorHandler"
	put the long id of it into tBehaviorButton

	create stack "StackToSave"
	set the defaultStack to "StackToSave"

	create button "TestButton"
	set the behavior of button "TestButton" to tBehaviorButton

	local tTempFile
	put tempName() into tTempFile
	save stack "StackToSave" as tTempFile
	delete stack "StackToSave"
	delete tBehaviorButton

	go stack tTempFile
	delete file tTempFile
	
	TestAssert "ignores blocked behaviors", not (the effective revAvailableHandlers of button "TestButton" of stack "StackToSave" contains " myBehaviorHandler ")

	delete stack "StackToSave"
end TestRevEffectiveAvailableHandlersBlockedBehavior

on TestRevEffectiveAvailableHandlersDeletedBehavior
	clone button "IsolatedBehavior"
	set the behavior of button "TestButton" to it
	delete it
	TestAssert "ignores blocked behaviors", the effective revAvailableHandlers of button "TestButton" contains " myHandler "
end TestRevEffectiveAvailableHandlersDeletedBehavior


on TestRevScriptDescription
	set the behavior of button "TestButton" to the long id of button "IsolatedBehavior"
   local tDescription
   put the revScriptDescription of button "TestButton" into tDescription
   
	TestAssert "lists handlers only in object script", tDescription["handlers"]["myHandler"] is an array
	TestAssert "lists private handlers only in object script", tDescription["handlers"]["myPrivateHandler"] is an array
end TestRevScriptDescription

on TestRevEffectiveScriptDescriptionBehavior
	set the behavior of button "TestButton" to the long id of button "IsolatedBehavior"
   local tDescription, tTestButtonIndex, tIsolatedBehaviorIndex
   put the effective revScriptDescription of button "TestButton" into tDescription
	repeat with tIndex = 1 to the number of elements of tDescription
      if tDescription[tIndex]["object"] is the long id of button "TestButton" then
         put tIndex into tTestButtonIndex
      end if
      
      if tDescription[tIndex]["object"] is the long id of button "IsolatedBehavior" then
         put tIndex into tIsolatedBehaviorIndex
      end if
   end repeat
   
	TestAssert "lists object's handlers", tDescription[tTestButtonIndex]["description"]["handlers"]["myHandler"] is an array
	TestAssert "lists object's private handlers", tDescription[tTestButtonIndex]["description"]["handlers"]["myPrivateHandler"] is an array
	TestAssert "lists object's behavior handlers", tDescription[tIsolatedBehaviorIndex]["description"]["handlers"]["myBehaviorHandler"] is an array
	TestAssert "does not list object's behavior private handlers", tDescription[tIsolatedBehaviorIndex]["description"]["handlers"]["myPrivateBehaviorHandler"] is not an array
end TestRevEffectiveScriptDescriptionBehavior

on TestRevEffectiveScriptDescriptionChainedBehavior
	set the behavior of button "TestButton" to the long id of button "ChainedBehavior"
   
   local tDescription, tChainedBehaviorIndex
   put the effective revScriptDescription of button "TestButton" into tDescription
	repeat with tIndex = 1 to the number of elements of tDescription
      if tDescription[tIndex]["object"] is the long id of button "ChainedBehavior" then
         put tIndex into tChainedBehaviorIndex
      end if
   end repeat
   
	TestAssert "lists object's chained behavior handlers", tDescription[tChainedBehaviorIndex]["description"]["handlers"]["myChainedBehaviorHandler"] is an array
end TestRevEffectiveScriptDescriptionChainedBehavior

on TestRevEffectiveScriptDescriptionBlockedBehavior
   if the platform is "HTML5" then
      TestSkip "ignores blocked behaviors", "SKIP on HTML5 no writable file system"
      exit TestRevEffectiveScriptDescriptionBlockedBehavior
   end if
	-- Behaviors can only become 'blocked' when deserialized. Therefore, we
	-- do this by creating a new behavior and user of the behavior, copying the
	-- user to the clipboard (which serializes), then deleting both, and then
	-- pasting to deserialize. At this point the new button's behavior will not
	-- resolve and will be marked as blocked.
	local tBehaviorButton
	create button "BehaviorButton"
	set the script of it to "command myBehaviorHandler; end myBehaviorHandler"
	put the long id of it into tBehaviorButton

	create stack "StackToSave"
	set the defaultStack to "StackToSave"

	create button "TestButton"
	set the behavior of button "TestButton" to tBehaviorButton

	local tTempFile
	put tempName() into tTempFile
	save stack "StackToSave" as tTempFile
	delete stack "StackToSave"
	delete tBehaviorButton

	go stack tTempFile
	delete file tTempFile
   
   local tDescription, tBlockedBehaviorIndex
   put the effective revScriptDescription of button "TestButton" of stack "StackToSave" into tDescription
	repeat with tIndex = 1 to the number of elements of tDescription
      if tDescription[tIndex]["object"] is tBehaviorButton then
         put tIndex into tBlockedBehaviorIndex
      end if
   end repeat
	
	TestAssert "ignores blocked behaviors", tDescription[tBlockedBehaviorIndex]["description"]["handlers"]["myBehaviorHandler"] is not an array

	delete stack "StackToSave"
end TestRevEffectiveScriptDescriptionBlockedBehavior

on TestRevEffectiveScriptDescriptionDeletedBehavior
	clone button "IsolatedBehavior"
	set the behavior of button "TestButton" to it
	delete it
   
   local tDescription, tButtonIndex
   put the effective revScriptDescription of button "TestButton" into tDescription
	repeat with tIndex = 1 to the number of elements of tDescription
      if tDescription[tIndex]["object"] is the long id of button "TestButton" then
         put tIndex into tButtonIndex
      end if
   end repeat
   
	TestAssert "ignores blocked behaviors", tDescription[tButtonIndex]["description"]["handlers"]["myHandler"] is an array
end TestRevEffectiveScriptDescriptionDeletedBehavior


on TestRevBehaviorUses
   set the behavior of button "TestButton" to the long id of button "IsolatedBehavior"
   local tUses
   put the revBehaviorUses of button "IsolatedBehavior" into tUses
   TestAssert "lists uses of behavior", exists(tUses[1])
   
   put the revBehaviorUses of button "TestButton" into tUses
   TestAssert "empty if no uses of behavior", the number of elements of tUses is 0
end TestRevBehaviorUses

on TestRevBehaviorUsesChainedBehavior
   set the behavior of button "TestButton" to the long id of button "ChainedBehavior"
   
   local tUses
   put the revBehaviorUses of button "ChainedBehavior" into tUses
   TestAssert "lists uses of behavior", exists(tUses[1])
   TestAssert "does not list behavior chain", the number of elements of tUses is 1
   
   put the revBehaviorUses of button "IsolatedBehavior" into tUses
   TestAssert "lists multiple uses of behavior", exists(tUses[2])
end TestRevBehaviorUsesChainedBehavior
