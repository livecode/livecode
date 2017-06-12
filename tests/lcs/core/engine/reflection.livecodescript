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
	set the script of it to "command myBehaviorHandler; end myBehaviorHandler"

	create button "ChainedBehavior"
	set the script of it to "command myChainedBehaviorHandler; end myChainedBehaviorHandler"
	set the behavior of it to the long id of button "IsolatedBehavior"

	create button "TestButton"
	set the script of it to "command myHandler; end myHandler"
end TestSetup

on TestTeardown
	delete button "TestButton"
	delete button "ChainedBehavior"
	delete button "IsolatedBehavior"
end TestTeardown

on TestRevAvailableHandlers
	set the behavior of button "TestButton" to the long id of button "IsolatedBehavior"
	TestAssert "lists handlers only in object script", the revAvailableHandlers of button "TestButton" contains " myHandler "
end TestRevAvailableHandlers

on TestRevEffectiveAvailableHandlersBehavior
	set the behavior of button "TestButton" to the long id of button "IsolatedBehavior"
	TestAssert "lists object's behavior handlers", the effective revAvailableHandlers of button "TestButton" contains " myBehaviorHandler "
end TestRevEffectiveAvailableHandlersBehavior

on TestRevEffectiveAvailableHandlersChainedBehavior
	set the behavior of button "TestButton" to the long id of button "ChainedBehavior"
	TestAssert "lists object's chained behavior handlers", the effective revAvailableHandlers of button "TestButton" contains " myChainedBehaviorHandler "
end TestRevEffectiveAvailableHandlersChainedBehavior

on TestRevEffectiveAvailableHandlersBlockedBehavior
	clone button "IsolatedBehavior"
	set the behavior of button "TestButton" to it
	delete it
	TestAssert "ignores blocked behaviors", the effective revAvailableHandlers of button "TestButton" contains " myHandler "
end TestRevEffectiveAvailableHandlersBlockedBehavior
