/* Copyright (C) 2003-2016 LiveCode Ltd.
 
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

#include "libscript/script.h"
#include "script-private.h"

bool
MCScriptThrowPropertyNotFoundError(MCScriptInstanceRef instance,
								   MCNameRef property)
{
	return false;
}

bool
MCScriptThrowHandlerNotFoundError(MCScriptInstanceRef instance,
								  MCNameRef handler)
{
	return false;
}

bool
MCScriptThrowPropertyUsedBeforeAssignedError(MCScriptInstanceRef instance,
											 MCScriptPropertyDefinition *property_def)
{
	return false;
}

bool
MCScriptThrowInvalidValueForPropertyError(MCScriptInstanceRef instance,
										  MCScriptPropertyDefinition *property_def,
										  MCValueRef provided_value)
{
	return false;
}

bool
MCScriptThrowWrongNumberOfArgumentsError(MCScriptInstanceRef instance,
										 MCScriptCommonHandlerDefinition *handler_def,
										 uindex_t provided_argument_count)
{
	return false;
}

bool
MCScriptThrowInvalidValueForArgumentError(MCScriptInstanceRef instance,
										  MCScriptCommonHandlerDefinition *handler_def,
										  uindex_t argument_index,
										  MCValueRef provided_value)
{
	return false;
}

bool
MCScriptThrowInvalidValueForReturnValueError(MCScriptInstanceRef instance,
											 MCScriptCommonHandlerDefinition *handler_def,
											 MCValueRef provided_value)
{
	return false;
}

bool
MCScriptThrowNotAHandlerValueError(MCValueRef actual_value)
{
	return false;
}

bool
MCScriptThrowNotAStringValueError(MCValueRef actual_value)
{
	return false;
}

bool
MCScriptThrowNotABooleanOrBoolValueError(MCValueRef actual_value)
{
	return false;
}

bool
MCScriptThrowGlobalVariableUsedBeforeAssignedError(MCScriptInstanceRef instance,
												   MCScriptVariableDefinition *variable_def)
{
	return false;
}

bool
MCScriptThrowInvalidValueForGlobalVariableError(MCScriptInstanceRef instance,
												MCScriptVariableDefinition *variable_def)
{
	return false;
}

bool
MCScriptThrowLocalVariableUsedBeforeAssignedError(MCScriptInstanceRef instance,
												  uindex_t index)
{
	return false;
}

bool
MCScriptThrowInvalidValueForLocalVariableError(MCScriptInstanceRef instance,
											   uindex_t index,
											   MCValueRef provided_value)
{
	return false;
}

bool
MCScriptThrowUnableToResolveMultiInvokeError(MCScriptInstanceRef instance,
											 MCScriptDefinitionGroupDefinition *group,
											 const uindex_t *arguments,
											 uindex_t argument_count)
{
	return false;
}

bool
MCScriptThrowErrorExpectedError(void)
{
	return false;
}
