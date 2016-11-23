/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#include <stdio.h>
#include <string.h>

#include "Interface.h"
#include "InterfacePrivate.h"
#include "NativeType.h"
#include "CString.h"

////////////////////////////////////////////////////////////////////////////////

enum InterfaceError
{
	kInterfaceErrorNone,
	kInterfaceErrorUnknownUse,
	kInterfaceErrorTargetAlreadyUsed,
	kInterfaceErrorUnknownHook,
	kInterfaceErrorHookAlreadyDefined,
	kInterfaceErrorInvalidEnumName,
	kInterfaceErrorEnumAlreadyDefined,
	kInterfaceErrorEnumElementAlreadyDefined,
	kInterfaceErrorCannotMixHandlerTypes,
	kInterfaceErrorAmbiguousHandler,
	kInterfaceErrorParamAfterOptionalParam,
	kInterfaceErrorParamAlreadyDefined,
	kInterfaceErrorInvalidParameterType,
	kInterfaceErrorOptionalParamImpliesIn,
	// MERG-2013-06-14: [[ ExternalsApiV5 ]] Error for when a non-pointer type is
	//   optional with no default value.
    kInterfaceErrorNonPointerOptionalParameterMustHaveDefaultValue,
	// MW-2013-06-14: [[ ExternalsApiV5 ]] Error for when a default value is 
	//   specified for a type that doesn't support defaulting.
	kInterfaceErrorDefaultValueNotSupportedForType,
	// MERG-2013-06-14: [[ ExternalsApiV5 ]] Wrong constant type for default value.
	kInterfaceErrorDefaultWrongType,
	kInterfaceErrorUnknownType,
	kInterfaceErrorJavaImpliesInParam,
	kInterfaceErrorMethodsCannotHaveVariants,
	kInterfaceErrorMethodsMustBeJava,
	kInterfaceErrorMethodsAreAlwaysTail,
	kInterfaceErrorUnknownHandlerMapping,
	kInterfaceErrorUnknownPlatform,
	kInterfaceErrorObjCNotSupported,
	kInterfaceErrorJavaNotSupported,
	kInterfaceErrorJavaImpliesNonIndirectReturn,
};

////////////////////////////////////////////////////////////////////////////////

static const char *s_interface_native_types[] =
{
	"boolean",
	"c-string",
	"c-data",
	"lc-array",
	"utf8-c-string",
	"utf8-c-data",
    "utf16-c-string",
    "utf16-c-data",
	"integer",
	"real",
	
	"objc-string",
	"objc-number",
	"objc-data",
	"objc-array",
	"objc-dictionary",
	
	/*"cf-string",
	"cf-number",
	"cf-data",
	"cf-array",
	"cf-dictionary",*/
	
	nil
};

static const char *s_interface_mapped_types[] =
{
    "string",
    "number",
    "data",
    "array",
    "dictionary",
    
    nil
};

////////////////////////////////////////////////////////////////////////////////

extern const char *g_input_filename;

static bool InterfaceReport(InterfaceRef self, Position p_where, InterfaceError p_error, void *p_hint)
{
	fprintf(stderr, "%s:%d:%d: error: ", g_input_filename, PositionGetRow(p_where), PositionGetColumn(p_where));
	
	switch(p_error)
	{
	case kInterfaceErrorUnknownUse:
		fprintf(stderr, "Unknown use '%s'\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorTargetAlreadyUsed:
		fprintf(stderr, "Function '%s' already bound\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorUnknownHook:
		fprintf(stderr, "Unknown hook '%s'\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorHookAlreadyDefined:
		fprintf(stderr, "Hook '%s' already defined\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorInvalidEnumName:
		fprintf(stderr, "Type name '%s' is reserved\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorEnumAlreadyDefined:
		fprintf(stderr, "Enum with name '%s' already defined\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorEnumElementAlreadyDefined:
		fprintf(stderr, "Enum already has an element with name '%s'\n", StringGetCStringPtr((StringRef)p_hint));
		break;
	case kInterfaceErrorCannotMixHandlerTypes:
		fprintf(stderr, "Variants of handler '%s' have different type\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorAmbiguousHandler:
		fprintf(stderr, "Variant of handler '%s' is ambiguous\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorParamAfterOptionalParam:
		fprintf(stderr, "Cannot define non-optional parameter after optional parameters\n");
		break;
	case kInterfaceErrorParamAlreadyDefined:
		fprintf(stderr, "Handler already has a parameter with name '%s'\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorInvalidParameterType:
		fprintf(stderr, "Invalid combination of parameter type and mode\n");
		break;
	case kInterfaceErrorOptionalParamImpliesIn:
		fprintf(stderr, "Optional parameters must be of 'in' type\n");
		break;
    case kInterfaceErrorNonPointerOptionalParameterMustHaveDefaultValue:
        fprintf(stderr, "Default values must be specified for non-pointer type optional parameters\n");
        break;
	case kInterfaceErrorDefaultValueNotSupportedForType:
		fprintf(stderr, "Default value not supported for the given type\n");
		break;
    case kInterfaceErrorDefaultWrongType:
		fprintf(stderr, "Default specified is the wrong type\n");
		break;
	case kInterfaceErrorUnknownType:
		fprintf(stderr, "Unknown type '%s'\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorJavaImpliesInParam:
		fprintf(stderr, "Java mapped methods can only have 'in' parameters\n");
        break;
	case kInterfaceErrorUnknownHandlerMapping:
		fprintf(stderr, "Unknown handler mapping type '%s'\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorUnknownPlatform:
		fprintf(stderr, "Unknown platform '%s'\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;	
	case kInterfaceErrorObjCNotSupported:
		fprintf(stderr, "Objective-C mapping not supported on platform '%s'\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorJavaNotSupported:
		fprintf(stderr, "Java mapping not supported on platform '%s'\n", StringGetCStringPtr(NameGetString((NameRef)p_hint)));
		break;
	case kInterfaceErrorJavaImpliesNonIndirectReturn:
		fprintf(stderr, "Java mapped methods cannot have indirect return value\n");
		break;
	case kInterfaceErrorMethodsCannotHaveVariants:
		fprintf(stderr, "Java mapped methods cannot have variants\n");
		break;
	case kInterfaceErrorMethodsAreAlwaysTail:
		fprintf(stderr, "Java mapped methods are always tail\n");
		break;
	case kInterfaceErrorMethodsMustBeJava:
		fprintf(stderr, "Java mapped methods must be java\n");
		break;
	case kInterfaceErrorNone:
		MCUnreachableReturn(false);
		break;
	}
	
	self -> invalid = true;
	
	return true;
}

static void InterfaceCheckType(InterfaceRef self, Position p_where, NameRef p_type)
{
	for(uint32_t i = 0; s_interface_native_types[i] != nil; i++)
		if (NameEqualToCString(p_type, s_interface_native_types[i]))
			return;
    
    for(uint32_t i = 0; s_interface_mapped_types[i] != nil; i++)
        if (NameEqualToCString(p_type, s_interface_mapped_types[i]))
            return;
    
    for(uint32_t i = 0; i < self -> enum_count; i++)
		if (NameEqual(self -> enums[i] . name, p_type))
			return;
			
	InterfaceReport(self, p_where, kInterfaceErrorUnknownType, p_type);
}

////////////////////////////////////////////////////////////////////////////////

bool InterfaceCreate(InterfaceRef& r_interface)
{
	return MCMemoryNew(r_interface);
}

void InterfaceDestroy(InterfaceRef self)
{
	ValueRelease(self -> name);
	
	for(uint32_t i = 0; i < self -> enum_count; i++)
	{
		ValueRelease(self -> enums[i] . name);
		for(uint32_t j = 0; j < self -> enums[i] . element_count; j++)
			ValueRelease(self -> enums[i] . elements[j] . key);
		MCMemoryDeleteArray(self -> enums[i] . elements);
	}
	MCMemoryDeleteArray(self -> enums);
	
	for(uint32_t i = 0; i < self -> handler_count; i++)
	{
		ValueRelease(self -> handlers[i] . name);
		for(uint32_t j = 0; j < self -> handlers[i] . variant_count; j++)
		{
			for(uint32_t k = 0; k < self -> handlers[i] . variants[j] . parameter_count; k++)
			{
				ValueRelease(self -> handlers[i] . variants[j] . parameters[k] . name);
				ValueRelease(self -> handlers[i] . variants[j] . parameters[k] . type);
				ValueRelease(self -> handlers[i] . variants[j] . parameters[k] . default_value);
			}
			MCMemoryDeleteArray(self -> handlers[i] . variants[j] . parameters);
			
			
			ValueRelease(self -> handlers[i] . variants[j] . return_type);
			ValueRelease(self -> handlers[i] . variants[j] . binding);
		}
		MCMemoryDeleteArray(self -> handlers[i] . variants);
	}
	MCMemoryDeleteArray(self -> handlers);
}

bool InterfaceBegin(InterfaceRef self, Position p_where, NameRef p_name)
{
	MCLog("%s - external %s", PositionDescribe(p_where), StringGetCStringPtr(NameGetString(p_name)));
	
	const char *t_unqualified_name;
	t_unqualified_name = strrchr(NameGetCString(p_name), '.');
	if (t_unqualified_name != nil)
		t_unqualified_name += 1;
	else
		t_unqualified_name = NameGetCString(p_name);
	
	if (!NameCreateWithNativeChars(t_unqualified_name, strlen(t_unqualified_name), self -> name))
		return false;
	
	self -> where = p_where;
	self -> qualified_name = ValueRetain(p_name);
	
	// MW-2013-07-29: [[ ExternalsApiV5 ]] Default mappings are to use C on all platforms.
	for(Platform i = kPlatformMac; i < __kPlatformCount__; i++)
		self -> use_mappings[i] = kHandlerMappingC;
	
	return true;
}

bool InterfaceEnd(InterfaceRef self)
{
	return true;
}

bool InterfaceDefineUse(InterfaceRef self, Position p_where, NameRef p_use)
{
	MCLog("%s - use %s", PositionDescribe(p_where), StringGetCStringPtr(NameGetString(p_use)));
	
	if (NameEqualToCString(p_use, "c++-exceptions"))
		self -> use_cpp_exceptions = true;
	else if (NameEqualToCString(p_use, "c++-naming"))
		self -> use_cpp_naming = true;
	else if (NameEqualToCString(p_use, "objc-exceptions"))
		self -> use_objc_exceptions = true;
	else if (NameEqualToCString(p_use, "objc-objects"))
		self -> use_objc_objects = true;
	else
		return InterfaceReport(self, p_where, kInterfaceErrorUnknownUse, p_use);
	
	return true;
}

bool InterfaceDefineUseOnPlatform(InterfaceRef self, Position p_where, NameRef p_use, NameRef p_platform)
{
	MCLog("%s - use %s on %s", PositionDescribe(p_where), StringGetCStringPtr(NameGetString(p_use)), StringGetCStringPtr(NameGetString(p_platform)));
	
	HandlerMapping t_mapping;
	if (NameEqualToCString(p_use, "none"))
		t_mapping = kHandlerMappingNone;
	else if (NameEqualToCString(p_use, "c"))
		t_mapping = kHandlerMappingC;
	else if (NameEqualToCString(p_use, "objc"))
		t_mapping = kHandlerMappingObjC;
	else if (NameEqualToCString(p_use, "java"))
		t_mapping = kHandlerMappingJava;
	else
		return InterfaceReport(self, p_where, kInterfaceErrorUnknownHandlerMapping, p_use);
	
	Platform t_platform;
	if (NameEqualToCString(p_platform, "mac"))
		t_platform = kPlatformMac;
	else if (NameEqualToCString(p_platform, "windows"))
		t_platform = kPlatformWindows;
	else if (NameEqualToCString(p_platform, "linux"))
		t_platform = kPlatformLinux;
	else if (NameEqualToCString(p_platform, "ios"))
		t_platform = kPlatformIOS;
	else if (NameEqualToCString(p_platform, "android"))
		t_platform = kPlatformAndroid;
	else
		return InterfaceReport(self, p_where, kInterfaceErrorUnknownPlatform, p_platform);
	
	if (t_mapping == kHandlerMappingObjC && !(t_platform == kPlatformMac || t_platform == kPlatformIOS))
		return InterfaceReport(self, p_where, kInterfaceErrorObjCNotSupported, p_platform);
	
	if (t_mapping == kHandlerMappingJava && !(t_platform == kPlatformAndroid))
		return InterfaceReport(self, p_where, kInterfaceErrorJavaNotSupported, p_platform);
	
	self -> use_mappings[t_platform] = t_mapping;
	
	return true;
}

bool InterfaceDefineHook(InterfaceRef self, Position p_where, NameRef p_handler, NameRef p_target)
{
	MCLog("%s - hook %s with %s", PositionDescribe(p_where), StringGetCStringPtr(NameGetString(p_handler)), StringGetCStringPtr(NameGetString(p_target)));
	
	// RULE: Hook must be one of 'startup' or 'shutdown'
	ValueRef *t_hook_var;
	t_hook_var = nil;
	if (NameEqualToCString(p_handler, "startup"))
		t_hook_var = &self -> startup_hook;
	else if (NameEqualToCString(p_handler, "shutdown"))
		t_hook_var = &self -> shutdown_hook;
	else
		return InterfaceReport(self, p_where, kInterfaceErrorUnknownHook, p_handler);
	
	// RULE: Hook must not be previous defined.
	if (*t_hook_var != nil)
		return InterfaceReport(self, p_where, kInterfaceErrorHookAlreadyDefined, p_handler);
	
	// RULE: Hook must target unique method.
	if (NameEqual(self -> startup_hook, p_target) ||
		NameEqual(self -> shutdown_hook, p_target))
		return InterfaceReport(self, p_where, kInterfaceErrorTargetAlreadyUsed, p_handler);
	
	*t_hook_var = ValueRetain(p_target);
	
	return true;
}

bool InterfaceBeginEnum(InterfaceRef self, Position p_where, NameRef p_name)
{
	MCLog("%s - enum %s", PositionDescribe(p_where), StringGetCStringPtr(NameGetString(p_name)));
	
	// RULE: Enum must not used built-in type name
	for(uint32_t i = 0; s_interface_native_types[i] != nil; i++)
		if (NameEqualToCString(p_name, s_interface_native_types[i]))
			InterfaceReport(self, p_where, kInterfaceErrorInvalidEnumName, p_name);
			
	// RULE: Enum must not already exist
	for(uint32_t i = 0; i < self -> enum_count; i++)
		if (NameEqual(self -> enums[i] . name, p_name))
			InterfaceReport(self, p_where, kInterfaceErrorEnumAlreadyDefined, p_name);
			
	if (!MCMemoryResizeArray(self -> enum_count + 1, self -> enums, self -> enum_count))
		return false;
		
	self -> enums[self -> enum_count - 1] . where = p_where;
	self -> enums[self -> enum_count - 1] . name = ValueRetain(p_name);
	
	return true;
}

bool InterfaceEndEnum(InterfaceRef self)
{
	return true;
}

bool InterfaceDefineEnumElement(InterfaceRef self, Position p_where, StringRef p_element, ValueRef p_value)
{
	MCLog("%s - enum element %s as %lld", PositionDescribe(p_where), StringGetCStringPtr(p_element), NumberGetInteger(p_value));
	
	Enum *t_enum;
	t_enum = &self -> enums[self -> enum_count - 1];
	
	// RULE: Element name must not already be present
	for(uint32_t i = 0; i < t_enum -> element_count; i++)
		if (MCCStringEqualCaseless(StringGetCStringPtr(p_element), StringGetCStringPtr(t_enum -> elements[i] . key)))
			return InterfaceReport(self, p_where, kInterfaceErrorEnumElementAlreadyDefined, p_element);
			
	if (!MCMemoryResizeArray(t_enum -> element_count + 1, t_enum -> elements, t_enum -> element_count))
		return false;
		
	t_enum -> elements[t_enum -> element_count - 1] . where = p_where;
	t_enum -> elements[t_enum -> element_count - 1] . key = ValueRetain(p_element);
	t_enum -> elements[t_enum -> element_count - 1] . value = NumberGetInteger(p_value);
	
	return true;
}

bool InterfaceBeginHandler(InterfaceRef self, Position p_where, HandlerType p_type, HandlerAttributes p_attr, NameRef p_name)
{
	MCLog("%s - %s handler %s", PositionDescribe(p_where), p_type == kHandlerTypeCommand ? "command" : "function", StringGetCStringPtr(NameGetString(p_name)));
	
	Handler *t_handler;
	t_handler = nil;
	for(uint32_t i = 0; i < self -> handler_count; i++)
		if (NameEqualCaseless(p_name, self -> handlers[i] . name))
		{
			t_handler = &self -> handlers[i];
			break;
		}
	
	// RULE: Methods cannot have variants.
	if (t_handler != nil &&
			p_type == kHandlerTypeMethod)
		InterfaceReport(self, p_where, kInterfaceErrorMethodsCannotHaveVariants, p_name);
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] 'java' and 'tail' separated out - make sure
	//   all variants have the same kind.
	// RULE: Variants of handlers must all have the same type.
	if (t_handler != nil &&
			t_handler -> type != p_type &&
			/*t_handler -> is_java == ((p_attr & kHandlerAttributeIsJava) != 0) &&*/
			t_handler -> is_tail == ((p_attr & kHandlerAttributeIsTail) != 0))
		InterfaceReport(self, p_where, kInterfaceErrorCannotMixHandlerTypes, p_name);
	
	// RULE: Methods must specify 'java'.
	/*if (p_type == kHandlerTypeMethod && (p_attr & kHandlerAttributeIsJava) == 0)
		InterfaceReport(self, p_where, kInterfaceErrorMethodsMustBeJava, p_name);*/
	
	// RULE: Methods must not specify 'tail' since that is their purpose.
	if (p_type == kHandlerTypeMethod && (p_attr & kHandlerAttributeIsTail) != 0)
		InterfaceReport(self, p_where, kInterfaceErrorMethodsAreAlwaysTail, p_name);
		
	if (t_handler == nil)
	{
		if (!MCMemoryResizeArray(self -> handler_count + 1, self -> handlers, self -> handler_count))
			return false;
			
		t_handler = &self -> handlers[self -> handler_count - 1];
		t_handler -> type = p_type;
		
		// MW-2013-06-14: [[ ExternalsApiV5 ]] Set the attributes appropriate in the
		//   handler.
		/*t_handler -> is_java = (p_attr & kHandlerAttributeIsJava) != 0;*/
		t_handler -> is_tail = (p_attr & kHandlerAttributeIsTail) != 0;

		t_handler -> name = ValueRetain(p_name);
	}
	
	if (!MCMemoryResizeArray(t_handler -> variant_count + 1, t_handler -> variants, t_handler -> variant_count))
		return false;
	
	t_handler -> variants[t_handler -> variant_count - 1] . where = p_where;
	
	memcpy(t_handler -> variants[t_handler -> variant_count - 1] . mappings, self -> use_mappings, sizeof(self -> use_mappings));
	
	self -> current_handler = t_handler;
	
	return true;
}

bool InterfaceEndHandler(InterfaceRef self)
{
	Handler *t_handler;
	t_handler = self -> current_handler;
	
	HandlerVariant *t_variant;
	t_variant = &t_handler -> variants[t_handler -> variant_count - 1];

	// If no binding specified, take the name of the handler
	if (t_variant -> binding == nil)
		t_variant -> binding = ValueRetain(t_handler -> name);
		
	// RULE: Target functions can be referenced only once
	uint32_t t_usage_count;
	t_usage_count = 0;
	if (NameEqual(t_variant -> binding, self -> startup_hook))
		t_usage_count += 1;
	if (NameEqual(t_variant -> binding, self -> shutdown_hook))
		t_usage_count += 1;
	for(uint32_t i = 0; i < self -> handler_count; i++)
		for(uint32_t j = 0; j < self -> handlers[i] . variant_count; j++)
			if (NameEqual(t_variant -> binding, self -> handlers[i] . variants[j] . binding))
				t_usage_count += 1;
	if (t_usage_count > 1)
		InterfaceReport(self, t_variant -> where, kInterfaceErrorTargetAlreadyUsed, t_variant -> binding);
	
	// RULE: Variants must have unique signatures
	for(uint32_t i = 0; i < t_handler -> variant_count - 1; i++)
	{
		if (t_variant == &t_handler -> variants[i])
			continue;
		if (t_variant -> minimum_parameter_count <= t_handler -> variants[i] . parameter_count &&
			t_variant -> parameter_count >= t_handler -> variants[i] . minimum_parameter_count)
		{
			InterfaceReport(self, t_variant -> where, kInterfaceErrorAmbiguousHandler, t_handler -> name);
			break;
		}
	}

	return true;
}

bool InterfaceDefineHandlerParameter(InterfaceRef self, Position p_where, ParameterType p_param_type, NameRef p_name, NameRef p_type, bool p_optional, ValueRef p_default)
{	
	static const char *s_param_types[] = {"in", "out", "inout", "ref"};
	MCLog("%s - %s%s parameter %s as %s", PositionDescribe(p_where),
			p_optional ? "optional " : "",
			s_param_types[p_param_type],
			StringGetCStringPtr(NameGetString(p_name)), 
			StringGetCStringPtr(NameGetString(p_type)));
	
	HandlerVariant *t_variant;
	t_variant = &self -> current_handler -> variants[self -> current_handler -> variant_count - 1];
	
	// RULE: Type must be defined
	InterfaceCheckType(self, p_where, p_type);
	
	// RULE: 'ref' not currently supported
	if (p_param_type == kParameterTypeRef)
		InterfaceReport(self, p_where, kInterfaceErrorInvalidParameterType, nil);
    
    NativeType t_native_type;
    t_native_type = NativeTypeFromName(p_type);
    
    // RULE: only pointer types may not have a default value
    if (p_optional && p_default == nil &&
        (t_native_type == kNativeTypeBoolean ||
         t_native_type == kNativeTypeInteger ||
         t_native_type == kNativeTypeReal ||
         t_native_type == kNativeTypeCData))
        InterfaceReport(self, p_where, kInterfaceErrorNonPointerOptionalParameterMustHaveDefaultValue, nil);
    
	// RULE: default values not supported for c-data, objc-data, objc-dictionary, objc-array types
	if (p_optional && p_default != nil &&
		(t_native_type == kNativeTypeCData ||
         t_native_type == kNativeTypeLCArray ||
         t_native_type == kNativeTypeObjcData ||
         t_native_type == kNativeTypeObjcArray ||
         t_native_type == kNativeTypeObjcDictionary))
        InterfaceReport(self, p_where, kInterfaceErrorDefaultValueNotSupportedForType, nil);
	
    // MERG-2013-06-14: [[ ExternalsApiV5 ]] Check that the type of the constant is
	//   correct for the type of the parameter.
    // RULE: wrong default type
    if (p_default != nil)
    {
        bool t_correct_type = false;
        switch (t_native_type) {
            case kNativeTypeBoolean:
                t_correct_type = ValueIsBoolean(p_default);
                break;
            case kNativeTypeInteger:
                t_correct_type = ValueIsInteger(p_default);
                break;
            case kNativeTypeReal:
                t_correct_type = ValueIsReal(p_default) || ValueIsInteger(p_default);
                break;
            case kNativeTypeObjcString:
            case kNativeTypeCString:
            case kNativeTypeUTF8CString:
            case kNativeTypeUTF16CString:
            case kNativeTypeEnum:
                t_correct_type = ValueIsString(p_default);
                break;
            default:
                t_correct_type = false;
                break;
        }
        
        if (!t_correct_type)
            InterfaceReport(self, p_where, kInterfaceErrorDefaultWrongType, nil);
    }
    
	// RULE: optional parameters can only be 'in'
	if (p_default != nil && p_param_type != kParameterTypeIn)
		InterfaceReport(self, p_where, kInterfaceErrorOptionalParamImpliesIn, nil);
	
	// RULE: Parameter name must be unique
	for(uint32_t i = 0; i < t_variant -> parameter_count; i++)
		if (NameEqualCaseless(p_name, t_variant -> parameters[i] . name))
		{
			InterfaceReport(self, p_where, kInterfaceErrorParamAlreadyDefined, p_name);
			break;
		}
		
	// RULE: No non-optional parameters after an optional one
	if (!p_optional &&
		t_variant -> parameter_count > 0 &&
		t_variant -> parameters[t_variant -> parameter_count - 1] . is_optional)
		InterfaceReport(self, p_where, kInterfaceErrorParamAfterOptionalParam, nil);
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] New rule to check java methods have compatible
	//   signature.
	// RULE: If java handler, then only in parameters are allowed.
	/*if (self -> current_handler -> is_java)
		if (p_param_type != kParameterTypeIn)
			InterfaceReport(self, p_where, kInterfaceErrorJavaImpliesInParam, nil);*/
	if (t_variant -> mappings[kPlatformAndroid] == kHandlerMappingJava &&
		p_param_type != kParameterTypeIn)
			InterfaceReport(self, p_where, kInterfaceErrorJavaImpliesInParam, nil);
	
	if (!MCMemoryResizeArray(t_variant -> parameter_count + 1, t_variant -> parameters, t_variant -> parameter_count))
		return false;
		
	t_variant -> parameters[t_variant -> parameter_count - 1] . where = p_where;
	t_variant -> parameters[t_variant -> parameter_count - 1] . mode = p_param_type;
	t_variant -> parameters[t_variant -> parameter_count - 1] . name = ValueRetain(p_name);
	t_variant -> parameters[t_variant -> parameter_count - 1] . type = ValueRetain(p_type);
	t_variant -> parameters[t_variant -> parameter_count - 1] . default_value = ValueRetain(p_default);
	t_variant -> parameters[t_variant -> parameter_count - 1] . is_optional = p_optional;
	
	if (!p_optional)
		t_variant -> minimum_parameter_count += 1;
	
	return true;
}

bool InterfaceDefineHandlerReturn(InterfaceRef self, Position p_where, NameRef p_type, bool p_indirect)
{
	MCLog("%s - return %s%s", PositionDescribe(p_where),
			p_indirect ? "indirect " : "",
			StringGetCStringPtr(NameGetString(p_type)));
			
	HandlerVariant *t_variant;
	t_variant = &self -> current_handler -> variants[self -> current_handler -> variant_count - 1];

	// MW-2013-07-27: [[ ExternalsApiV5 ]] New rule to check we don't use indirect on
	//   java mapped methods.
	// RULE: If java handler, then only non-indirect return values are allowed.
	if (t_variant -> mappings[kPlatformAndroid] == kHandlerMappingJava &&
		p_indirect)
		InterfaceReport(self, p_where, kInterfaceErrorJavaImpliesNonIndirectReturn, nil);

	InterfaceCheckType(self, p_where, p_type);
	
	t_variant -> return_type = ValueRetain(p_type);
	t_variant -> return_type_indirect = p_indirect;
	
	return true;
}

bool InterfaceDefineHandlerBinding(InterfaceRef self, Position p_where, NameRef p_name)
{
	MCLog("%s - call %s", PositionDescribe(p_where),
			StringGetCStringPtr(NameGetString(p_name)));
			
	self -> current_handler -> variants[self -> current_handler -> variant_count - 1] . binding = ValueRetain(p_name);
			
	return true;
}

////////////////////////////////////////////////////////////////////////////////
