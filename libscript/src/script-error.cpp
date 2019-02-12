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

#include <foundation-auto.h>

#include "libscript/script.h"
#include "script-private.h"

bool
MCScriptThrowPropertyNotFoundError(MCScriptInstanceRef p_instance,
								   MCNameRef p_property_name)
{
	return MCErrorCreateAndThrow(kMCScriptPropertyNotFoundErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "property",
								 p_property_name,
								 nil);
}

bool
MCScriptThrowHandlerNotFoundError(MCScriptInstanceRef p_instance,
								  MCNameRef p_handler_name)
{
	return MCErrorCreateAndThrow(kMCScriptHandlerNotFoundErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "handler",
								 p_handler_name,
								 nil);
}

bool
MCScriptThrowPropertyUsedBeforeAssignedError(MCScriptInstanceRef p_instance,
											 MCScriptPropertyDefinition *p_property_def)
{
	return MCErrorCreateAndThrow(kMCScriptPropertyUsedBeforeAssignedErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "property",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																		   p_property_def),
                                 nil);
}

bool
MCScriptThrowInvalidValueForPropertyError(MCScriptInstanceRef p_instance,
										  MCScriptPropertyDefinition *p_property_def,
                                          MCTypeInfoRef p_property_type,
										  MCValueRef p_provided_value)
{
	return MCErrorCreateAndThrow(kMCScriptInvalidPropertyValueErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "property",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																	 p_property_def),
								 "type",
	                             p_property_type,
								 "value",
								 p_provided_value,
								 nil);
}

bool
MCScriptThrowWrongNumberOfArgumentsError(MCScriptInstanceRef p_instance,
										 MCScriptCommonHandlerDefinition *p_handler_def,
										 uindex_t p_provided_argument_count)
{
	// TODO: Add expected / provided argument counts.
	return MCErrorCreateAndThrow(kMCScriptWrongNumberOfArgumentsErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "handler",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																	 p_handler_def),
								 nil);
}

bool
MCScriptThrowInvalidValueForArgumentError(MCScriptInstanceRef p_instance,
										  MCScriptCommonHandlerDefinition *p_handler_def,
										  uindex_t p_argument_index,
										  MCValueRef p_provided_value)
{
	return MCErrorCreateAndThrow(kMCScriptInvalidArgumentValueErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "handler",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																	 p_handler_def),
								 "parameter",
								 MCScriptGetNameOfParameterInModule(p_instance->module,
																	p_handler_def,
																	p_argument_index),
								 "type",
								 MCScriptGetTypeOfParameterInModule(p_instance->module,
																	p_handler_def,
																	p_argument_index),
								 "value",
								 p_provided_value,
								 nil);
}

bool
MCScriptThrowInvalidValueForReturnValueError(MCScriptInstanceRef p_instance,
											 MCScriptCommonHandlerDefinition *p_handler_def,
											 MCValueRef p_provided_value)
{
	return MCErrorCreateAndThrow(kMCScriptInvalidReturnValueErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "handler",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																	 p_handler_def),
								 "type",
								 MCScriptGetTypeOfReturnValueInModule(p_instance->module,
																	  p_handler_def),
								 "value",
								 p_provided_value,
								 nil);
}

bool
MCScriptThrowNotAHandlerValueError(MCValueRef p_provided_value)
{
	return MCErrorCreateAndThrow(kMCScriptNotAHandlerValueErrorTypeInfo,
								 "value",
								 p_provided_value,
								 nil);
}

bool
MCScriptThrowNotAStringValueError(MCValueRef p_provided_value)
{
	return MCErrorCreateAndThrow(kMCScriptNotAStringValueErrorTypeInfo,
								 "value",
								 p_provided_value,
								 nil);
}

bool
MCScriptThrowNotABooleanOrBoolValueError(MCValueRef p_provided_value)
{
	return MCErrorCreateAndThrow(kMCScriptNotABooleanValueErrorTypeInfo,
								 "value",
								 p_provided_value,
								 nil);
}

bool
MCScriptThrowGlobalVariableUsedBeforeAssignedError(MCScriptInstanceRef p_instance,
												   MCScriptVariableDefinition *p_variable_def)
{
	return MCErrorCreateAndThrow(kMCScriptVariableUsedBeforeAssignedErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "variable",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																	 p_variable_def),
								 nil);
}

bool
MCScriptThrowInvalidValueForGlobalVariableError(MCScriptInstanceRef p_instance,
												MCScriptVariableDefinition *p_variable_def,
												MCValueRef p_provided_value)
{
	return MCErrorCreateAndThrow(kMCScriptInvalidVariableValueErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "variable",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																	 p_variable_def),
								 "type",
								 MCScriptGetTypeOfGlobalVariableInModule(p_instance->module,
																		 p_variable_def),
								 "value",
								 p_provided_value,
								 nil);
}

bool
MCScriptThrowLocalVariableUsedBeforeAssignedError(MCScriptInstanceRef p_instance,
												  MCScriptHandlerDefinition *p_handler,
												  uindex_t p_index)
{
	return MCErrorCreateAndThrow(kMCScriptVariableUsedBeforeAssignedErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "handler",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																	 p_handler),
								 "variable",
								 MCScriptGetNameOfLocalVariableInModule(p_instance->module,
																		p_handler,
																		p_index),
								 nil);
}

bool
MCScriptThrowInvalidValueForLocalVariableError(MCScriptInstanceRef p_instance,
											   MCScriptHandlerDefinition *p_handler,
											   uindex_t p_index,
											   MCValueRef p_provided_value)
{
	return MCErrorCreateAndThrow(kMCScriptInvalidVariableValueErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "handler",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																	 p_handler),
								 "variable",
								 MCScriptGetNameOfLocalVariableInModule(p_instance->module,
																		p_handler,
																		p_index),
								 "type",
								 MCScriptGetTypeOfLocalVariableInModule(p_instance->module,
																		p_handler,
																		p_index),
								 "value",
								 p_provided_value,
								 nil);
}

bool
MCScriptThrowUnableToResolveMultiInvokeError(MCScriptInstanceRef p_instance,
											 MCScriptDefinitionGroupDefinition *p_definition,
											 MCProperListRef p_arguments)
{
	MCAutoListRef t_handlers;
	if (!MCListCreateMutable(',',
							 &t_handlers))
	{
		return false;
	}
	
	MCScriptDefinitionGroupDefinition *t_group;
	t_group = static_cast<MCScriptDefinitionGroupDefinition *>(p_definition);
	for(uindex_t i = 0; i < t_group -> handler_count; i++)
	{
		MCNameRef t_name;
		t_name = MCScriptGetNameOfDefinitionInModule(p_instance->module,
													 p_instance->module->definitions[t_group->handlers[i]]);
		if (!MCListAppend(*t_handlers,
						  t_name))
		{
			return false;
		}
	}
	
	MCAutoListRef t_types;
	if (!MCListCreateMutable(',',
							 &t_types))
	{
		return false;
	}
	
	for(uindex_t i = 0; i < MCProperListGetLength(p_arguments); i++)
	{
		MCTypeInfoRef t_type;
		t_type = MCValueGetTypeInfo(MCProperListFetchElementAtIndex(p_arguments,
																	i));
		
		MCAutoStringRef t_type_name;
		if (!MCValueCopyDescription(t_type,
									&t_type_name))
		{
			return false;
		}
		
		if (!MCListAppend(*t_types, *t_type_name))
		{
			return false;
		}
	}
	
	MCAutoStringRef t_handler_list, t_type_list;
	if (!MCListCopyAsString(*t_handlers, &t_handler_list) ||
		!MCListCopyAsString(*t_types, &t_type_list))
	{
		return false;
	}
	
	return MCErrorCreateAndThrow(kMCScriptNoMatchingHandlerErrorTypeInfo,
								 "handlers",
								 *t_handler_list,
								 "types",
								 *t_type_list,
								 nil);
}

bool
MCScriptThrowUnableToResolveForeignHandlerError(MCScriptInstanceRef p_instance,
												MCScriptForeignHandlerDefinition *p_definition)
{
	return MCErrorCreateAndThrow(kMCScriptForeignHandlerBindingErrorTypeInfo,
								 "module",
								 p_instance->module->name,
								 "handler",
								 MCScriptGetNameOfDefinitionInModule(p_instance->module,
																	 p_definition),
								 nil);
}

bool
MCScriptThrowUnknownForeignLanguageError(void)
{
	return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo,
								 "reason",
								 MCSTR("unknown language"),
								 nil);
}

bool
MCScriptThrowUnknownForeignCallingConventionError(void)
{
	return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo,
								 "reason",
								 MCSTR("unknown calling convention"),
								 nil);
}

bool
MCScriptThrowMissingFunctionInForeignBindingError(void)
{
	return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo,
								 "reason",
								 MCSTR("no function specified in binding string"),
								 nil);
}

bool
MCScriptThrowUnableToLoadForiegnLibraryError(void)
{
	return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo,
								 "reason",
								 MCSTR("unable to load foreign library"),
								 nil);
}

bool
MCScriptThrowUnknownThreadAffinityError(void)
{
    return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo,
                                 "reason",
                                 MCSTR("unknown thread affinity specified in binding string"),
                                 nil);
}

bool
MCScriptThrowJavaBindingNotSupported(void)
{
	return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo,
								 "reason",
								 MCSTR("java binding not supported on this platform"),
								 nil);
}

bool
MCScriptThrowForeignExceptionError(MCStringRef p_reason)
{
    return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo,
                                 "reason",
                                 p_reason,
                                 nil);
}

bool
MCScriptThrowObjCBindingNotSupported(void)
{
	return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo,
								 "reason",
								 MCSTR("objc binding not supported on this platform"),
								 nil);
}

bool
MCScriptCreateErrorExpectedError(MCErrorRef& r_error)
{
	return MCErrorCreateS(r_error,
						  kMCGenericErrorTypeInfo,
						  "reason",
						  MCSTR("error propagated without error object"),
						  nil);
}

bool MCScriptThrowCannotSetReadOnlyPropertyError(MCScriptInstanceRef p_instance, MCNameRef p_property)
{
    return MCErrorCreateAndThrow(kMCScriptCannotSetReadOnlyPropertyErrorTypeInfo,
                                 "module",
                                 p_instance -> module -> name,
                                 "property",
                                 p_property,
                                 nil);
}
