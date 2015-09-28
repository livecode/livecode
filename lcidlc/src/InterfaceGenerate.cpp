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
#include <stdlib.h>

#include "Interface.h"
#include "InterfacePrivate.h"

////////////////////////////////////////////////////////////////////////////////

// Includes
// Imports
// Definitions
// Support
// Wrappers
// Exports

////////////////////////////////////////////////////////////////////////////////

enum NativeType
{
	kNativeTypeNone,
	kNativeTypeBoolean,
	kNativeTypeCString,
	kNativeTypeCData,
	kNativeTypeInteger,
	kNativeTypeReal,
	kNativeTypeObjcString,
	kNativeTypeObjcNumber,
	kNativeTypeObjcData,
	kNativeTypeObjcArray,
	kNativeTypeObjcDictionary,
	
	kNativeTypeEnum
};

static NativeType NativeTypeFromName(NameRef p_type)
{
	if (NameEqualToCString(p_type, "boolean"))
		return kNativeTypeBoolean;
	else if (NameEqualToCString(p_type, "c-string"))
		return kNativeTypeCString;
	else if (NameEqualToCString(p_type, "c-data"))
		return kNativeTypeCData;
	else if (NameEqualToCString(p_type, "integer"))
		return kNativeTypeInteger;
	else if (NameEqualToCString(p_type, "real"))
		return kNativeTypeReal;
	else if (NameEqualToCString(p_type, "objc-string"))
		return kNativeTypeObjcString;
	else if (NameEqualToCString(p_type, "objc-number"))
		return kNativeTypeObjcNumber;
	else if (NameEqualToCString(p_type, "objc-data"))
		return kNativeTypeObjcData;
	else if (NameEqualToCString(p_type, "objc-array"))
		return kNativeTypeObjcArray;
	else if (NameEqualToCString(p_type, "objc-dictionary"))
		return kNativeTypeObjcDictionary;
		
	return kNativeTypeEnum;
}

static const char *NativeTypeGetTypedef(NativeType p_type)
{
	switch(p_type)
	{
	case kNativeTypeBoolean:
		return "bool";
	case kNativeTypeCString:
		return "char*";
	case kNativeTypeCData:
		return "LCBytes";
	case kNativeTypeInteger:
		return "int";
	case kNativeTypeReal:
		return "double";
	case kNativeTypeObjcString:
		return "NSString*";
	case kNativeTypeObjcNumber:
		return "NSNumber*";
	case kNativeTypeObjcData:
		return "NSData*";
	case kNativeTypeObjcArray:
		return "NSArray*";
	case kNativeTypeObjcDictionary:
		return "NSDictionary*";
	case kNativeTypeEnum:
		return "int";
	default:
		break;
	}
	return "<<unknown>>";
}

static const char *NativeTypeGetSecondaryPrefix(NativeType p_type)
{
	switch(p_type)
	{
	case kNativeTypeCString:
	case kNativeTypeObjcString:
	case kNativeTypeObjcNumber:
	case kNativeTypeObjcData:
	case kNativeTypeObjcArray:
	case kNativeTypeObjcDictionary:
		return "*";
	}
	
	return "";
}

static const char *NativeTypeGetTag(NativeType p_type)
{
	switch(p_type)
	{
	case kNativeTypeBoolean:
		return "bool";
	case kNativeTypeCString:
		return "cstring";
	case kNativeTypeCData:
		return "cdata";
	case kNativeTypeInteger:
		return "int";
	case kNativeTypeReal:
		return "double";
	case kNativeTypeObjcString:
		return "objc_string";
	case kNativeTypeObjcNumber:
		return "objc_number";
	case kNativeTypeObjcData:
		return "objc_data";
	case kNativeTypeObjcArray:
		return "objc_array";
	case kNativeTypeObjcDictionary:
		return "objc_dictionary";
	case kNativeTypeEnum:
		return "int";
	default:
		break;
	}
	return "<<unknown>>";
}

static const char *NativeTypeGetInitializer(NativeType p_type)
{
	switch(p_type)
	{
	case kNativeTypeBoolean:
		return "false";
	case kNativeTypeCString:
	case kNativeTypeObjcString:
	case kNativeTypeObjcNumber:
	case kNativeTypeObjcData:
	case kNativeTypeObjcArray:
	case kNativeTypeObjcDictionary:
		return "nil";
	case kNativeTypeCData:
		return "{ 0, nil }";
	case kNativeTypeInteger:
		return "0";
	case kNativeTypeReal:
		return "0.0";
	case kNativeTypeEnum:
		return "0";
	default:
		break;
	}
		
	return nil;
}

////////////////////////////////////////////////////////////////////////////////

typedef FILE *CoderRef;

static bool CoderStart(const char *p_filename, CoderRef& r_coder)
{
	CoderRef self;
	self = fopen(p_filename, "w");
	if (self == nil)
		return false;
		
	r_coder = self;
		
	return true;
}

static bool CoderFinish(CoderRef self)
{
	fclose(self);
	return true;
}

static void CoderCancel(CoderRef self)
{
}

static void CoderWriteLine(CoderRef self, const char *p_format, ...)
{
	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(self, p_format, t_args);
	va_end(t_args);
	fprintf(self, "\n");
}

static void CoderWrite(CoderRef self, const char *p_format, ...)
{
	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(self, p_format, t_args);
	va_end(t_args);
}

////////////////////////////////////////////////////////////////////////////////

static const char *name_to_cname(NameRef p_type)
{
	static char *s_buffer = nil;
	MCCStringFree(s_buffer);
	MCCStringClone(NameGetCString(p_type), s_buffer);
	for(uint32_t i = 0; s_buffer[i] != '\0'; i++)
		if (s_buffer[i] == '-')
			s_buffer[i] = '_';
	return s_buffer;
}

static const char *type_to_cstring(NameRef p_type, bool p_is_ref)
{
	if (NameEqualToCString(p_type, "boolean"))
		return "bool";
	else if (NameEqualToCString(p_type, "c-string"))
		return p_is_ref ? "char *" : "const char *";
	else if (NameEqualToCString(p_type, "c-data"))
		return "LCBytes";
	else if (NameEqualToCString(p_type, "integer"))
		return "int";
	else if (NameEqualToCString(p_type, "real"))
		return "double";
	else if (NameEqualToCString(p_type, "objc-string"))
		return "NSString*";
	else if (NameEqualToCString(p_type, "objc-number"))
		return "NSNumber*";
	else if (NameEqualToCString(p_type, "objc-data"))
		return "NSData*";
	else if (NameEqualToCString(p_type, "objc-array"))
		return "NSArray*";
	else if (NameEqualToCString(p_type, "objc-dictionary"))
		return "NSDictionary*";
		
	return "int";
}

////////////////////////////////////////////////////////////////////////////////

static const char *InterfaceGetExternPrefix(InterfaceRef self)
{
	if (!self -> use_cpp_naming)
		return "extern \"C\"";
	return "extern";
}

static const char *InterfaceGetReferenceSuffix(InterfaceRef self)
{
	if (self -> use_cpp_naming)
		return "&";
	return "*";
}

////////////////////////////////////////////////////////////////////////////////

extern const char *g_support_template;

static bool InterfaceGenerateSupport(InterfaceRef self, CoderRef p_coder)
{
	if (self -> use_cpp_exceptions)
		CoderWriteLine(p_coder, "#include <exception>");
	
	CoderWrite(p_coder, "%s", g_support_template);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool InterfaceGenerateImports(InterfaceRef self, CoderRef p_coder)
{
	const char *t_extern;
	t_extern = InterfaceGetExternPrefix(self);
	
	const char *t_reference;
	t_reference = InterfaceGetReferenceSuffix(self);
	
	if (self -> startup_hook != nil)
		CoderWriteLine(p_coder, "%s bool %s(void);", t_extern, NameGetCString(self -> startup_hook));
	if (self -> shutdown_hook != nil)
		CoderWriteLine(p_coder, "%s void %s(void);", t_extern, NameGetCString(self -> shutdown_hook));
		
	for(uint32_t i = 0; i < self -> handler_count; i++)
		for(uint32_t j = 0; j < self -> handlers[i] . variant_count; j++)
		{
			HandlerVariant *t_variant;
			t_variant = &self -> handlers[i] . variants[j];
			
			const char *t_return_type;
			if (t_variant -> return_type == nil || t_variant -> return_type_indirect)
				t_return_type = "void";
			else
				t_return_type = type_to_cstring(t_variant -> return_type, true);
			CoderWrite(p_coder, "%s %s %s(", t_extern, t_return_type, NameGetCString(t_variant -> binding));
			
			if (t_variant -> parameter_count != 0)
			{
				for(uint32_t k = 0; k < t_variant -> parameter_count; k++)
				{
					if (k > 0)
						CoderWrite(p_coder, ", ");
					CoderWrite(p_coder, "%s", type_to_cstring(t_variant -> parameters[k] . type, t_variant -> parameters[k] . mode != kParameterTypeIn));
					if (t_variant -> parameters[k] . mode != kParameterTypeIn)
						CoderWrite(p_coder, "%s", t_reference);
				}
			}
			else if (t_variant -> return_type_indirect)
				CoderWrite(p_coder, "%s%s", type_to_cstring(t_variant -> return_type, true), t_reference);
			else
				CoderWrite(p_coder, "void");
			
			CoderWriteLine(p_coder, ");");
		}
		
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool InterfaceGenerateEnums(InterfaceRef self, CoderRef p_coder)
{
	for(uint32_t i = 0; i < self -> enum_count; i++)
	{
		Enum *t_enum;
		t_enum = &self -> enums[i];
		
		CoderWriteLine(p_coder, "static bool fetchenum__%s(const char *arg, MCVariableRef var, int& r_value)", name_to_cname(t_enum -> name));
		CoderWriteLine(p_coder, "{");
		CoderWriteLine(p_coder, "\tbool success;");
		CoderWriteLine(p_coder, "\tsuccess = true;");
		CoderWriteLine(p_coder, "\tchar *element;");
		CoderWriteLine(p_coder, "\tif (!fetch__cstring(arg, var, element))");
		CoderWriteLine(p_coder, "\t\treturn false;");
		for(uint32_t j = 0; j < t_enum -> element_count; j++)
		{
			CoderWriteLine(p_coder, "\t%sif (strcasecmp(element, \"%s\") == 0)", j == 0 ? "" : "else ", StringGetCStringPtr(t_enum -> elements[j] . key));
			CoderWriteLine(p_coder, "\t\tr_value = %lld;", t_enum -> elements[j] . value);
		}
		if (t_enum -> element_count > 0)
			CoderWriteLine(p_coder, "\telse");
		CoderWriteLine(p_coder, "\t\tsuccess = error__bad_enum_element(arg, element);");
		CoderWriteLine(p_coder, "\tfree(element);");
		CoderWriteLine(p_coder, "\treturn success;");
		CoderWriteLine(p_coder, "}");
		
		CoderWriteLine(p_coder, "static bool storeenum__%s(MCVariableRef var, int value)", name_to_cname(t_enum -> name));
		CoderWriteLine(p_coder, "{");
		for(uint32_t j = 0; j < t_enum -> element_count; j++)
		{
			CoderWriteLine(p_coder, "\t%sif (value == %lld)", j == 0 ? "" : "else ", t_enum -> elements[j] . value);
			CoderWriteLine(p_coder, "\t\treturn store__cstring(var, \"%s\");", StringGetCStringPtr(t_enum -> elements[j] . key));
		}
		if (t_enum -> element_count > 0)
			CoderWriteLine(p_coder, "\telse");
		CoderWriteLine(p_coder, "\t\treturn store__int(var, value);");
		CoderWriteLine(p_coder, "\treturn true;");
		CoderWriteLine(p_coder, "}");
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static int64_t InterfaceResolveEnumElement(InterfaceRef self, NameRef p_name, ValueRef p_element)
{
	for(uint32_t i = 0; i < self -> enum_count; i++)
		if (NameEqual(p_name, self -> enums[i] . name))
		{
			for(uint32_t j = 0; j < self -> enums[i] . element_count; j++)
				if (StringEqualCaseless(p_element, self -> enums[i] . elements[j] . key))
					return self -> enums[i] . elements[j] . value;					
		}
		
	return 0;
}

static const char *native_type_to_java_type_cstring(NativeType p_type)
{
	switch(p_type)
	{
		case kNativeTypeBoolean:
			return "bool";
		case kNativeTypeCString:
			return "jobject";
		case kNativeTypeCData:
			return "jobject";
		case kNativeTypeInteger:
			return "int";
		case kNativeTypeReal:
			return "double";
		default:
			break;
	}
	return "not_supported_type";
}

static const char *native_type_to_java_sig(NativeType p_type)
{
	switch(p_type)
	{
		case kNativeTypeBoolean:
			return "Z";
		case kNativeTypeCString:
			return "Ljava/lang/String;";
		case kNativeTypeCData:
			return "[B";
		case kNativeTypeInteger:
			return "I";
		case kNativeTypeReal:
			return "D";
		default:
			break;
	}
	return "not_supported_type";
}

static const char *native_type_to_java_method_type_cstring(NativeType p_type)
{
	switch(p_type)
	{
		case kNativeTypeBoolean:
			return "Boolean";
		case kNativeTypeCString:
			return "Object";
		case kNativeTypeCData:
			return "Object";
		case kNativeTypeInteger:
			return "Int";
		case kNativeTypeReal:
			return "Double";
		default:
			break;
	}
	return "not_supported_type";
}

static const char *native_type_to_type_in_cstring(NativeType p_type)
{
	switch(p_type)
	{
		case kNativeTypeBoolean:
			return "bool";
		case kNativeTypeCString:
			return "const char *";
		case kNativeTypeCData:
			return "LCBytes";
		case kNativeTypeInteger:
			return "int";
		case kNativeTypeReal:
			return "double";
		default:
			break;
	}
	return "not_supported_type";
}

static const char *native_type_to_type_out_cstring(NativeType p_type)
{
	switch(p_type)
	{
		case kNativeTypeBoolean:
			return "bool";
		case kNativeTypeCString:
			return "char *";
		case kNativeTypeCData:
			return "LCBytes";
		case kNativeTypeInteger:
			return "int";
		case kNativeTypeReal:
			return "double";
		default:
			break;
	}
	return "not_supported_type";
}

static void InterfaceGenerateMethodStubContext(InterfaceRef self, CoderRef p_coder, Handler *p_handler, bool p_in)
{
	NameRef t_name;
	t_name = p_handler -> name;
	
	HandlerVariant *t_variant;
	t_variant = &p_handler -> variants[0];
	
	CoderWriteLine(p_coder, "struct %s_context_t", NameGetCString(t_name));
	CoderWriteLine(p_coder, "{");
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
	{
		const char *t_type;
		if (p_in)
			t_type = native_type_to_type_in_cstring(NativeTypeFromName(t_variant -> parameters[i] . type));
		else
			t_type = native_type_to_type_out_cstring(NativeTypeFromName(t_variant -> parameters[i] . type));
		CoderWriteLine(p_coder, "\t%s arg_%d;", t_type, i);
	}
	if (t_variant -> return_type != nil)
		CoderWriteLine(p_coder, "\t%s result;", native_type_to_type_out_cstring(NativeTypeFromName(t_variant -> return_type)));
	CoderWriteLine(p_coder, "};");
	CoderWriteLine(p_coder, "");
}

static void InterfaceGenerateMethodStubParameters(InterfaceRef self, CoderRef p_coder, Handler *p_handler, bool p_java)
{
	HandlerVariant *t_variant;
	t_variant = &p_handler -> variants[0];
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
	{
		if (i != 0)
			CoderWrite(p_coder, ", ");
		
		const char *t_type;
		if (p_java)
			t_type = native_type_to_java_type_cstring(NativeTypeFromName(t_variant -> parameters[i] . type));
		else
			t_type = native_type_to_type_in_cstring(NativeTypeFromName(t_variant -> parameters[i] . type));
		
		CoderWrite(p_coder, "%s p_param_%d", t_type, i);
	}
}

char *InterfaceGenerateJavaMethodSignature(InterfaceRef self, Handler *p_handler)
{
	HandlerVariant *t_variant;
	t_variant = &p_handler -> variants[0];
	
	char *t_signature;
	t_signature = nil;
	MCCStringAppend(t_signature, "(");
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
		MCCStringAppend(t_signature, native_type_to_java_sig(NativeTypeFromName(t_variant -> parameters[i] . type)));
	MCCStringAppend(t_signature, ")");
	MCCStringAppend(t_signature, t_variant -> return_type != nil ? native_type_to_java_sig(NativeTypeFromName(t_variant -> return_type)) : "V");
	return t_signature;
}

static void InterfaceGenerateJavaMethodStub(InterfaceRef self, CoderRef p_coder, Handler *p_handler)
{
	NameRef t_name;
	t_name = p_handler -> name;
	
	HandlerVariant *t_variant;
	t_variant = &p_handler -> variants[0];
	
	InterfaceGenerateMethodStubContext(self, p_coder, p_handler, true);
	
	CoderWriteLine(p_coder, "static void %s_callback(void *p_context)", NameGetCString(t_name));
	CoderWriteLine(p_coder, "{");
	CoderWriteLine(p_coder, "\t%s_context_t *ctxt = (%s_context_t *)p_context;", NameGetCString(t_name), NameGetCString(t_name));
	CoderWriteLine(p_coder, "");
	
	char *t_java_sig;
	t_java_sig = InterfaceGenerateJavaMethodSignature(self, p_handler);
	CoderWriteLine(p_coder, "\tstatic jmethodID s_method = 0;");
	CoderWriteLine(p_coder, "\tif (s_method == 0)");
	CoderWriteLine(p_coder, "\t\ts_method = s_java_env -> GetStaticMethodID(s_java_class, \"%s\", \"%s\");", NameGetCString(t_name), t_java_sig);
	free(t_java_sig);
	
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
	{
		HandlerParameter *t_parameter;
		t_parameter = &t_variant -> parameters[i];
		
		NativeType t_param_native_type;
		t_param_native_type = NativeTypeFromName(t_parameter -> type);
		
		const char *t_java_param_type;
		t_java_param_type = native_type_to_java_type_cstring(t_param_native_type);
		
		CoderWriteLine(p_coder, "\t%s t_param_%d;", t_java_param_type, i);
		CoderWriteLine(p_coder, "\tt_param_%d = java_from__%s(ctxt -> arg_%d);", i, NativeTypeGetTag(t_param_native_type), i);
	}
	
	const char *t_java_method_type, *t_c_return_type, *t_java_return_type;
	if (t_variant -> return_type != nil)
	{
		t_java_method_type = native_type_to_java_method_type_cstring(NativeTypeFromName(t_variant -> return_type));
		t_c_return_type = native_type_to_type_out_cstring(NativeTypeFromName(t_variant -> return_type));
		t_java_return_type = native_type_to_java_type_cstring(NativeTypeFromName(t_variant -> return_type));
	}
	else
	{
		t_java_method_type = "Void";
		t_c_return_type = "void";
		t_java_return_type = "void";
	}
	
	if (t_variant -> return_type != nil)
		CoderWriteLine(p_coder, "\t%s t_result;", t_java_return_type);
	
	if (t_variant -> return_type != nil)
		CoderWrite(p_coder, "\tt_result = s_java_env -> CallStatic%sMethod(s_java_class, s_method", t_java_method_type);
	else
		CoderWrite(p_coder, "\ts_java_env -> CallStatic%sMethod(s_java_class, s_method", t_java_method_type);
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
		CoderWrite(p_coder, ", t_param_%d", i);
	CoderWrite(p_coder, ");\n");
	
	if (t_variant -> return_type != nil)
	{
		CoderWriteLine(p_coder, "\tctxt -> result = java_to__%s(t_result);", NativeTypeGetTag(NativeTypeFromName(t_variant -> return_type)));
		CoderWriteLine(p_coder, "\tjava_free__%s(t_result);", NativeTypeGetTag(NativeTypeFromName(t_variant -> return_type)));
	}
	
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
		CoderWriteLine(p_coder, "\tjava_free__%s(t_param_%d);", NativeTypeGetTag(NativeTypeFromName(t_variant -> parameters[i] . type)), i);
	
	CoderWriteLine(p_coder, "}");
	CoderWrite(p_coder, "%s %s(", t_c_return_type, NameGetCString(t_name));
	InterfaceGenerateMethodStubParameters(self, p_coder, p_handler, false);
	CoderWriteLine(p_coder, ")");
	CoderWriteLine(p_coder, "{");
	CoderWriteLine(p_coder, "\t%s_context_t ctxt;", NameGetCString(t_name));
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
		CoderWriteLine(p_coder, "\tctxt . arg_%d = p_param_%d;", i, i);
	CoderWriteLine(p_coder, "\ts_interface -> engine_run_on_main_thread((void *)%s_callback, &ctxt, kMCRunOnMainThreadJumpToUI);", NameGetCString(t_name));
	if (t_variant -> return_type != nil)
		CoderWriteLine(p_coder, "\treturn ctxt . result;");
	CoderWriteLine(p_coder, "}");
}

static void InterfaceGenerateNativeMethodStub(InterfaceRef self, CoderRef p_coder, Handler *p_handler)
{
	NameRef t_name;
	t_name = p_handler -> name;
	
	HandlerVariant *t_variant;
	t_variant = &p_handler -> variants[0];
	
	char *t_native_name;
	MCCStringFormat(t_native_name, "Java_%s_%s", NameGetCString(self -> qualified_name), NameGetCString(t_name));
	for(uindex_t i = 0; t_native_name[i] != '\0'; i++)
		if (t_native_name[i] == '.')
			t_native_name[i] = '_';
	
	const char *t_c_return_type, *t_java_return_type;
	if (t_variant -> return_type != nil)
	{
		t_c_return_type = native_type_to_type_out_cstring(NativeTypeFromName(t_variant -> return_type));
		t_java_return_type = native_type_to_java_type_cstring(NativeTypeFromName(t_variant -> return_type));
	}
	else
	{
		t_c_return_type = "void";
		t_java_return_type = "void";
	}

	CoderWrite(p_coder, "extern \"C\" JNIEXPORT %s JNICALL %s(JNIEnv *, jobject", t_java_return_type, t_native_name);
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
		CoderWrite(p_coder, ", %s", native_type_to_java_type_cstring(NativeTypeFromName(t_variant -> parameters[i] . type)));
	CoderWrite(p_coder, ") __attribute__((visibility(\"default\")));\n");
	CoderWrite(p_coder, "");
	
	InterfaceGenerateMethodStubContext(self, p_coder, p_handler, false);
	
	CoderWriteLine(p_coder, "static void %s_callback(void *p_context)", NameGetCString(t_name));
	CoderWriteLine(p_coder, "{");
	CoderWriteLine(p_coder, "\t%s_context_t *ctxt = (%s_context_t *)p_context;", NameGetCString(t_name), NameGetCString(t_name));
	if (t_variant -> return_type != nil)
		CoderWrite(p_coder, "\tctxt -> result = ");
	else
		CoderWrite(p_coder, "\t");
	CoderWrite(p_coder, "%s(", NameGetCString(t_name));
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
	{
		if (i != 0)
			CoderWrite(p_coder, ", ");
		CoderWrite(p_coder, "ctxt -> arg_%d", i);
	}
	CoderWrite(p_coder, ");\n");
	CoderWriteLine(p_coder, "}");
	
	CoderWrite(p_coder, "JNIEXPORT %s JNICALL %s(JNIEnv *env, jobject obj", t_java_return_type, t_native_name);
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
		CoderWrite(p_coder, ", %s p_param_%d", native_type_to_java_type_cstring(NativeTypeFromName(t_variant -> parameters[i] . type)), i);
	CoderWrite(p_coder, ")\n");
	CoderWriteLine(p_coder, "{");
	CoderWriteLine(p_coder, "\t%s_context_t ctxt;", NameGetCString(t_name));
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
	{
		HandlerParameter *t_parameter;
		t_parameter = &t_variant -> parameters[i];
		
		NativeType t_param_native_type;
		t_param_native_type = NativeTypeFromName(t_parameter -> type);
		
		CoderWriteLine(p_coder, "\tctxt . arg_%d = java_to__%s(p_param_%d);", i, NativeTypeGetTag(t_param_native_type), i);
	}
	CoderWriteLine(p_coder, "\ts_interface -> engine_run_on_main_thread((void *)%s_callback, &ctxt, kMCRunOnMainThreadJumpToEngine);", NameGetCString(t_name));
	
	if (t_variant -> return_type != nil)
	{
		CoderWriteLine(p_coder, "\t%s t_result;", t_java_return_type);
		CoderWriteLine(p_coder, "\tt_result = java_from__%s(ctxt . result);", NativeTypeGetTag(NativeTypeFromName(t_variant -> return_type)));
		CoderWriteLine(p_coder, "\tnative_free__%s(ctxt . result);",NativeTypeGetTag(NativeTypeFromName(t_variant -> return_type))); 
	}
	
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
		CoderWriteLine(p_coder, "\tnative_free__%s(ctxt . arg_%d);", NativeTypeGetTag(NativeTypeFromName(t_variant -> parameters[i] . type)), i);
	
	if (t_variant -> return_type != nil)
		CoderWriteLine(p_coder, "\treturn t_result;");
	
	CoderWriteLine(p_coder, "}");
	
	MCCStringFree(t_native_name);
	
}

static bool InterfaceGenerateHandlers(InterfaceRef self, CoderRef p_coder)
{
	for(uint32_t i = 0; i < self -> handler_count; i++)
	{
		Handler *t_handler;
		t_handler = &self -> handlers[i];
		
		if (t_handler -> type == kHandlerTypeJava)
		{
			// Generate stub for calling Java method.
			InterfaceGenerateJavaMethodStub(self, p_coder, t_handler);
			continue;
		}
		
		if (t_handler -> type == kHandlerTypeNative)
		{
			// Generate stub for calling native method.
			InterfaceGenerateNativeMethodStub(self, p_coder, t_handler);
			continue;
		}
		
		for(uint32_t j = 0; j < t_handler -> variant_count; j++)
		{
			HandlerVariant *t_variant;
			t_variant = &t_handler -> variants[j];
					
			CoderWriteLine(p_coder, "static bool variant__%s(MCVariableRef *argv, uint32_t argc, MCVariableRef result)", NameGetCString(t_variant -> binding));
			CoderWriteLine(p_coder, "{");
			if (self -> use_objc_objects)
			{
				CoderWriteLine(p_coder, "\tNSAutoreleasePool *t_pool;");
				CoderWriteLine(p_coder, "\tt_pool = [[NSAutoreleasePool alloc] init];");
				CoderWriteLine(p_coder, "");
			}
			
			CoderWriteLine(p_coder, "\tbool success;");
			CoderWriteLine(p_coder, "\tsuccess = true;");
			CoderWriteLine(p_coder, "");
			
			bool t_need_newline;
			t_need_newline = false;
			for(uint32_t k = 0; k < t_variant -> parameter_count; k++)
			{
				HandlerParameter *t_parameter;
				t_parameter = &t_variant -> parameters[k];
				
				const char *t_name;
				t_name = NameGetCString(t_parameter -> name);
				
				NativeType t_native_type;
				t_native_type = NativeTypeFromName(t_parameter -> type);

				if (t_parameter -> mode == kParameterTypeInOut && (t_native_type == kNativeTypeCString || t_native_type == kNativeTypeCData))
				{
					CoderWriteLine(p_coder, "\t%s original__%s, %sparam__%s;", NativeTypeGetTypedef(t_native_type), t_name, NativeTypeGetSecondaryPrefix(t_native_type), t_name);
					if (t_native_type != kNativeTypeCData)
						CoderWriteLine(p_coder, "\toriginal__%s = param__%s = %s;", t_name, t_name, NativeTypeGetInitializer(t_native_type));
					else
					{
						CoderWriteLine(p_coder, "\toriginal__%s . buffer = param__%s . buffer = nil;", t_name, t_name);
						CoderWriteLine(p_coder, "\toriginal__%s . length = param__%s . length = 0;", t_name, t_name);
					}
				}
				else
				{
					CoderWriteLine(p_coder, "\t%s param__%s;", NativeTypeGetTypedef(t_native_type), t_name);
					if (t_native_type != kNativeTypeCData)
						CoderWriteLine(p_coder, "\tparam__%s = %s;", t_name, NativeTypeGetInitializer(t_native_type));
					else
					{
						CoderWriteLine(p_coder, "\tparam__%s . buffer = NULL;", t_name);
						CoderWriteLine(p_coder, "\tparam__%s . length = 0;", t_name);
					}
				}
				if (t_parameter -> mode == kParameterTypeInOut || t_parameter -> mode == kParameterTypeOut)
				{
					CoderWriteLine(p_coder, "\tif (success)", k);
					CoderWriteLine(p_coder, "\t\tsuccess = verify__out_parameter(\"%s\", argv[%d]);", t_name, k);
				}
				if (t_parameter -> mode == kParameterTypeIn || t_parameter -> mode == kParameterTypeInOut)
				{
					CoderWriteLine(p_coder, "\tif (success)");
					if (t_parameter -> default_value != nil)
					{
						CoderWriteLine(p_coder, "\t{");
						CoderWriteLine(p_coder, "\t\tif (argc > %d)", k);
						if (t_native_type != kNativeTypeEnum)
							CoderWriteLine(p_coder, "\t\t\tsuccess = fetch__%s(\"%s\", argv[%d], param__%s);", NativeTypeGetTag(t_native_type), t_name, k, t_name);
						else
							CoderWriteLine(p_coder, "\t\t\tsuccess = fetchenum__%s(\"%s\", argv[%d], param__%s);", name_to_cname(t_parameter -> type), t_name, k, t_name);
						CoderWriteLine(p_coder, "\t\telse");
						switch(t_native_type)
						{
						case kNativeTypeBoolean:
							// TODO: Implement boolean default values
							CoderWriteLine(p_coder, "\t\t\tsuccess = false;");
							break;
						case kNativeTypeObjcData:
							CoderWriteLine(p_coder, "\t\t\tsuccess = false;");
							break;
						case kNativeTypeCString:
						case kNativeTypeCData:
							CoderWriteLine(p_coder, "\t\t\tsuccess = default__%s(\"%s\", param__%s);", NativeTypeGetTag(t_native_type), StringGetCStringPtr(t_parameter -> default_value), t_name);
							break;
						case kNativeTypeObjcString:
							CoderWriteLine(p_coder, "\t\t\tparam__%s = @\"%s\";", t_name, StringGetCStringPtr(t_parameter -> default_value));
							break;
						case kNativeTypeInteger:
							CoderWriteLine(p_coder, "\t\t\tparam__%s = %lld;", t_name, NumberGetInteger(t_parameter -> default_value));
							break;
						case kNativeTypeReal:
							CoderWriteLine(p_coder, "\t\t\tparam__%s = %.15g;", t_name, NumberGetReal(t_parameter -> default_value));
							break;
						case kNativeTypeEnum:
							CoderWriteLine(p_coder, "\t\t\tparam__%s = %lld;", t_name, InterfaceResolveEnumElement(self, t_parameter -> type, t_parameter -> default_value));
							break;
						default:
							CoderWriteLine(p_coder, "\t\t\tsuccess = false;");
							break;
						}
						CoderWriteLine(p_coder, "\t}");
					}
					else
					{
						if (t_native_type != kNativeTypeEnum)
							CoderWriteLine(p_coder, "\t\tsuccess = fetch__%s(\"%s\", argv[%d], param__%s);", NativeTypeGetTag(t_native_type), t_name, k, t_name);
						else
							CoderWriteLine(p_coder, "\t\tsuccess = fetchenum__%s(\"%s\", argv[%d], param__%s);", name_to_cname(t_parameter -> type), t_name, k, t_name);
					}
				}
				if ((t_native_type == kNativeTypeCString || t_native_type == kNativeTypeCData) && t_parameter -> mode == kParameterTypeInOut)
				{
					CoderWriteLine(p_coder, "\tif (success)");
					CoderWriteLine(p_coder, "\t\toriginal__%s = param__%s", t_name, t_name);
				}
				
				t_need_newline = true;
			}
			if (t_need_newline)
				CoderWriteLine(p_coder, "");
			
			const char *t_ref_char;
			if (self -> use_cpp_naming)
				t_ref_char = "";
			else
				t_ref_char = "&";
			
			NativeType t_native_return_type;
			if (t_variant -> return_type != nil)
				t_native_return_type = NativeTypeFromName(t_variant -> return_type);
			else
				t_native_return_type = kNativeTypeNone;
			
			if (t_native_return_type != kNativeTypeNone)
			{
				CoderWriteLine(p_coder, "\t%s returnvalue;", NativeTypeGetTypedef(t_native_return_type));
				if (t_native_return_type != kNativeTypeCData)
					CoderWriteLine(p_coder, "\treturnvalue = %s;", NativeTypeGetInitializer(t_native_return_type));
				else
				{
					CoderWriteLine(p_coder, "\treturnvalue . buffer = NULL;");
					CoderWriteLine(p_coder, "\treturnvalue . length = 0;");
				}
			}
				
			CoderWriteLine(p_coder, "\tif (success)");
			CoderWriteLine(p_coder, "\t{");
			CoderWriteLine(p_coder, "\t\terror__clear();");
			
			if (self -> use_cpp_exceptions)
			{
				CoderWriteLine(p_coder, "\t\ttry");
				CoderWriteLine(p_coder, "\t\t{");
			}
			
			if (self -> use_objc_exceptions)
				CoderWriteLine(p_coder, "\tNS_DURING");
				
			if (t_native_return_type != kNativeTypeNone)
			{
				if (!t_variant -> return_type_indirect)
					CoderWrite(p_coder, "\t\treturnvalue = %s(", NameGetCString(t_variant -> binding));
				else
					CoderWrite(p_coder, "\t\t%s(%sreturnvalue", NameGetCString(t_variant -> binding), t_ref_char);
			}
			else
				CoderWrite(p_coder, "\t\t%s(", NameGetCString(t_variant -> binding));
			
			for(uint32_t k = 0; k < t_variant -> parameter_count; k++)
			{
				HandlerParameter *t_parameter;
				t_parameter = &t_variant -> parameters[k];
				
				if (k > 0 || t_variant -> return_type_indirect)
					CoderWrite(p_coder, ", ");
				CoderWrite(p_coder, "%sparam__%s", t_parameter -> mode == kParameterTypeInOut || t_parameter -> mode == kParameterTypeOut ? t_ref_char : "", NameGetCString(t_parameter -> name));
			}
			
			CoderWrite(p_coder, ");\n");
			CoderWriteLine(p_coder, "\t\tsuccess = error__catch();");
			
			if (self -> use_objc_exceptions)
			{
				CoderWriteLine(p_coder, "\t\tNS_HANDLER");
				CoderWriteLine(p_coder, "\t\t\tsuccess = error__raise([[localException reason] cStringUsingEncoding: NSMacOSRomanStringEncoding]);");
				CoderWriteLine(p_coder, "\t\tNS_ENDHANDLER");
			}
			
			if (self -> use_cpp_exceptions)
			{
				CoderWriteLine(p_coder, "\t\t}");
				CoderWriteLine(p_coder, "\t\tcatch(std::exception& t_exception)");
				CoderWriteLine(p_coder, "\t\t{");
				CoderWriteLine(p_coder, "\t\t\tsuccess = error__raise(t_exception . what());");
				CoderWriteLine(p_coder, "\t\t}");
				CoderWriteLine(p_coder, "\t\tcatch(...)");
				CoderWriteLine(p_coder, "\t\t{");
				CoderWriteLine(p_coder, "\t\t\tsuccess = error__unknown();");
				CoderWriteLine(p_coder, "\t\t}");
			}
			
			CoderWriteLine(p_coder, "\t}");
			
			if (t_native_return_type != kNativeTypeNone)
			{
				CoderWriteLine(p_coder, "\tif (success)");
				if (t_native_return_type != kNativeTypeEnum)
					CoderWriteLine(p_coder, "\t\tsuccess = store__%s(result, returnvalue);", NativeTypeGetTag(t_native_return_type));
				else
					CoderWriteLine(p_coder, "\t\tsuccess = storeenum__%s(result, returnvalue);", name_to_cname(t_variant -> return_type));
			}
			CoderWriteLine(p_coder, "");
			
			t_need_newline = false;
			for(uint32_t k = t_variant -> parameter_count; k > 0; k--)
			{
				HandlerParameter *t_parameter;
				t_parameter = &t_variant -> parameters[k - 1];
				if (t_parameter -> mode == kParameterTypeIn)
					continue;
				
				const char *t_name;
				t_name = NameGetCString(t_parameter -> name);
				
				NativeType t_native_type;
				t_native_type = NativeTypeFromName(t_parameter -> type);
				
				CoderWriteLine(p_coder, "\tif (success)");
				if (t_native_type != kNativeTypeEnum)
					CoderWriteLine(p_coder, "\t\tsuccess = store__%s(argv[%d], param__%s);", NativeTypeGetTag(t_native_type), k - 1, t_name);
				else
					CoderWriteLine(p_coder, "\t\tsuccess = storeenum__%s(argv[%d], param__%s);", name_to_cname(t_parameter -> type), k - 1, t_name);
			
				t_need_newline = true;
			}
			if (t_need_newline)
				CoderWriteLine(p_coder, "");
			
			CoderWriteLine(p_coder, "\tif (!success)");
			CoderWriteLine(p_coder, "\t\tsuccess = error__report(result);");
			CoderWriteLine(p_coder, "");
			
			t_need_newline = false;
			if (t_native_return_type == kNativeTypeCString)
			{
				CoderWriteLine(p_coder, "\tfree(returnvalue);");
				t_need_newline = true;
			}
			else if (t_native_return_type == kNativeTypeCData)
			{
				CoderWriteLine(p_coder, "\tfree(returnvalue . buffer);");
				t_need_newline = true;
			}
			for(uint32_t k = t_variant -> parameter_count; k > 0; k--)
			{
				HandlerParameter *t_parameter;
				t_parameter = &t_variant -> parameters[k - 1];
				
				const char *t_name;
				t_name = NameGetCString(t_parameter -> name);
				
				NativeType t_native_type;
				t_native_type = NativeTypeFromName(t_parameter -> type);
				
				if (t_native_type == kNativeTypeCString)
				{
					CoderWriteLine(p_coder, "\tfree(param__%s);", t_name);
					if (t_parameter -> mode == kParameterTypeInOut)
					{
						CoderWriteLine(p_coder, "\tif (param__%s != original__%s)", t_name, t_name);
						CoderWriteLine(p_coder, "\t\tfree(original__%s);", t_name);
					}
					
					t_need_newline = true;
				}
				else if (t_native_type == kNativeTypeCData)
				{
					CoderWriteLine(p_coder, "\tfree(param__%s . buffer);", t_name);
					if (t_parameter -> mode == kParameterTypeInOut)
					{
						CoderWriteLine(p_coder, "\tif (param__%s . buffer != original__%s . buffer)", t_name, t_name);
						CoderWriteLine(p_coder, "\t\tfree(original__%s . buffer);", t_name);
					}
					
					t_need_newline = true;
				}
				
			}
			if (t_need_newline)
				CoderWriteLine(p_coder, "");
				
			if (self -> use_objc_objects)
			{
				CoderWriteLine(p_coder, "\t[t_pool release];");
				CoderWriteLine(p_coder, "");
			}
			
			CoderWriteLine(p_coder, "\treturn success;");
			
			CoderWriteLine(p_coder, "}");
		}
		
		if (t_handler -> type == kHandlerTypeTailCommand || t_handler -> type == kHandlerTypeTailFunction)
		{
			CoderWriteLine(p_coder, "struct handler__%s_env_t", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "{");
			CoderWriteLine(p_coder, "\tMCVariableRef *argv;");
			CoderWriteLine(p_coder, "\tuint32_t argc;");
			CoderWriteLine(p_coder, "\tMCVariableRef result;");
			CoderWriteLine(p_coder, "\tbool return_value;");
			CoderWriteLine(p_coder, "};");
			CoderWriteLine(p_coder, "static void do_handler__%s(void *p_env)", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "{");
			CoderWriteLine(p_coder, "\thandler__%s_env_t *env = (handler__%s_env_t *)p_env;", NameGetCString(t_handler -> name), NameGetCString(t_handler -> name));
			for(uint32_t j = 0; j < t_handler -> variant_count; j++)
			{
				HandlerVariant *t_variant;
				t_variant = &t_handler -> variants[j];
				
				if (t_variant -> minimum_parameter_count == t_variant -> parameter_count)
					CoderWriteLine(p_coder, "\tif (env -> argc == %d)", t_variant -> parameter_count);
				else
					CoderWriteLine(p_coder, "\tif (env -> argc >= %d && env -> argc <= %d)", t_variant -> minimum_parameter_count, t_variant -> parameter_count);
					
				CoderWriteLine(p_coder, "\t{");
				CoderWriteLine(p_coder, "\t\tenv -> return_value = variant__%s(env -> argv, env -> argc, env -> result);", NameGetCString(t_variant -> binding));
				CoderWriteLine(p_coder, "\t\treturn;");
				CoderWriteLine(p_coder, "\t}");
			}
			CoderWriteLine(p_coder, "\tenv -> return_value = error__report_bad_parameter_count(env -> result);");
			CoderWriteLine(p_coder, "}");
		
			CoderWriteLine(p_coder, "static bool handler__%s(MCVariableRef *argv, uint32_t argc, MCVariableRef result)", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "{");
			CoderWriteLine(p_coder, "\thandler__%s_env_t env;");
			CoderWriteLine(p_coder, "\tenv . argv = argv;");
			CoderWriteLine(p_coder, "\tenv . argc = argc;");
			CoderWriteLine(p_coder, "\tenv . result = result;");
			CoderWriteLine(p_coder, "\tif (s_interface -> version >= 4)");
			CoderWriteLine(p_coder, "\t\ts_interface -> engine_run_on_main_thread((void *)do_handler__%s, &env, kMCRunOnMainThreadJumpToUI);", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "\telse");
			CoderWriteLine(p_coder, "\t\tdo_handler__%s(&env);", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "\treturn env . return_value;");
			CoderWriteLine(p_coder, "}");
		}
		else
		{
			CoderWriteLine(p_coder, "static bool handler__%s(MCVariableRef *argv, uint32_t argc, MCVariableRef result)", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "{");
			for(uint32_t j = 0; j < t_handler -> variant_count; j++)
			{
				HandlerVariant *t_variant;
				t_variant = &t_handler -> variants[j];
				
				if (t_variant -> minimum_parameter_count == t_variant -> parameter_count)
					CoderWriteLine(p_coder, "\tif (argc == %d)", t_variant -> parameter_count);
				else
					CoderWriteLine(p_coder, "\tif (argc >= %d && argc <= %d)", t_variant -> minimum_parameter_count, t_variant -> parameter_count);
					
				CoderWriteLine(p_coder, "\t\treturn variant__%s(argv, argc, result);", NameGetCString(t_variant -> binding));
			}
			CoderWriteLine(p_coder, "\treturn error__report_bad_parameter_count(result);");
			CoderWriteLine(p_coder, "}");
		}
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static const char *s_exports_template = "\
extern \"C\" MCExternalInfo *MCExternalDescribe(void) __attribute__((visibility(\"default\")));\n\
extern \"C\" bool MCExternalInitialize(MCExternalInterface *) __attribute__((visibility(\"default\")));\n\
extern \"C\" void MCExternalFinalize(void) __attribute__((visibility(\"default\")));\n\
\n\
MCExternalInfo *MCExternalDescribe(void)\n\
{\n\
	static MCExternalInfo s_info;\n\
	s_info . version = 1;\n\
	s_info . flags = 0;\n\
	s_info . name = kMCExternalName;\n\
	s_info . handlers = kMCExternalHandlers;\n\
	return &s_info;\n\
}\n\
\n\
bool MCExternalInitialize(MCExternalInterface *p_interface)\n\
{\n\
	s_interface = p_interface;\n\
\n\
	if (s_interface -> version < 3)\n\
		return false;\n\
\n\
#ifdef kMCExternalStartup\n\
	if (!kMCExternalStartup())\n\
		return false;\n\
#endif\n\
\n\
	return true;\n\
}\n\
\n\
void MCExternalFinalize(void)\n\
{\n\
#ifdef kMCExternalShutdown\n\
	kMCExternalShutdown();\n\
#endif\n\
}\n\n\
";

static bool InterfaceGenerateExports(InterfaceRef self, CoderRef p_coder)
{
	CoderWriteLine(p_coder, "#define kMCExternalName \"%s\"", NameGetCString(self -> name));
	if (self -> startup_hook != nil)
		CoderWriteLine(p_coder, "#define kMCExternalStartup %s", NameGetCString(self -> startup_hook));
	if (self -> shutdown_hook != nil)
		CoderWriteLine(p_coder, "#define kMCExternalShutdown %s", NameGetCString(self -> shutdown_hook));
		
	CoderWriteLine(p_coder, "");
	
	CoderWriteLine(p_coder, "MCExternalHandler kMCExternalHandlers[] =\n{");
	for(uint32_t i = 0; i < self -> handler_count; i++)
	{
		Handler *t_handler;
		t_handler = &self -> handlers[i];

		if (t_handler -> type == kHandlerTypeJava || t_handler -> type == kHandlerTypeNative)
			continue;
		
		CoderWriteLine(p_coder, "\t{ %d, \"%s\", handler__%s },",
			t_handler -> type == kHandlerTypeCommand || t_handler -> type == kHandlerTypeTailCommand ? 1 : 2,
			NameGetCString(t_handler -> name),
			NameGetCString(t_handler -> name));
	}
	CoderWriteLine(p_coder, "\t{ 0 }\n};");
	
	CoderWriteLine(p_coder, "");
	
	CoderWrite(p_coder, s_exports_template);
	
	CoderWriteLine(p_coder, "#ifdef TARGET_OS_IPHONE");
	CoderWriteLine(p_coder, "extern \"C\"\n{");
	CoderWriteLine(p_coder, "");
	CoderWriteLine(p_coder, "static const char *__libname = \"%s\";", NameGetCString(self -> name));
	CoderWriteLine(p_coder, "");
	CoderWriteLine(p_coder, "static struct MCExternalLibraryExport __libexports[] =");
	CoderWriteLine(p_coder, "{");
	CoderWriteLine(p_coder, "\t{ \"MCExternalDescribe\", (void *)MCExternalDescribe },");
	CoderWriteLine(p_coder, "\t{ \"MCExternalInitialize\", (void *)MCExternalInitialize },");
	CoderWriteLine(p_coder, "\t{ \"MCExternalFinalize\", (void *)MCExternalFinalize },");
	CoderWriteLine(p_coder, "\t{ 0, 0 }");
	CoderWriteLine(p_coder, "};");
	CoderWriteLine(p_coder, "");
	CoderWriteLine(p_coder, "static struct MCExternalLibraryInfo __libinfo =");
	CoderWriteLine(p_coder, "{");
	CoderWriteLine(p_coder, "\t&__libname,");
	CoderWriteLine(p_coder, "\t__libexports,");
	CoderWriteLine(p_coder, "};");
	CoderWriteLine(p_coder, "");
	CoderWriteLine(p_coder, "__attribute((section(\"__DATA,__libs\"))) volatile struct MCExternalLibraryInfo *__libinfoptr_%s = &__libinfo;", NameGetCString(self -> name));
	CoderWriteLine(p_coder, "");
	CoderWriteLine(p_coder, "}");
	CoderWriteLine(p_coder, "#endif");
	
	char *t_java_class_name;
	t_java_class_name = strdup(NameGetCString(self -> qualified_name));
	for(uint32_t i = 0; t_java_class_name[i] != '\0'; i++)
		if (t_java_class_name[i] == '.')
			t_java_class_name[i] = '/';
	CoderWriteLine(p_coder, "#ifdef __ANDROID__");
	CoderWriteLine(p_coder, "extern \"C\" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) __attribute__((visibility(\"default\")));");
	CoderWriteLine(p_coder, "JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)");
	CoderWriteLine(p_coder, "{");
	CoderWriteLine(p_coder, "\ts_java_vm = vm;");
	CoderWriteLine(p_coder, "\ts_java_vm -> GetEnv((void **)&s_java_env, JNI_VERSION_1_2);");
	CoderWriteLine(p_coder, "\ts_java_class = (jclass)s_java_env -> NewGlobalRef(s_java_env -> FindClass(\"%s\"));", t_java_class_name);
	CoderWriteLine(p_coder, "");
	CoderWriteLine(p_coder, "\treturn JNI_VERSION_1_2;");
	CoderWriteLine(p_coder, "}");
	for(uint32_t i = 0; t_java_class_name[i] != '\0'; i++)
		if (t_java_class_name[i] == '/')
			t_java_class_name[i] = '_';
	CoderWriteLine(p_coder, "extern \"C\" JNIEXPORT jobject JNICALL Java_%s_getActivity(JNIEnv *env, jobject obj) __attribute((visibility(\"default\")));", t_java_class_name);
	CoderWriteLine(p_coder, "JNIEXPORT jobject JNICALL Java_%s_getActivity(JNIEnv *env, jobject obj)");
	CoderWriteLine(p_coder, "{");
	CoderWriteLine(p_coder, "\treturn java__get_activity();");
	CoderWriteLine(p_coder, "}");
	CoderWriteLine(p_coder, "extern \"C\" JNIEXPORT jobject JNICALL Java_%s_getContainer(JNIEnv *env, jobject obj) __attribute((visibility(\"default\")));", t_java_class_name);
	CoderWriteLine(p_coder, "JNIEXPORT jobject JNICALL Java_%s_getContainer(JNIEnv *env, jobject obj)");
	CoderWriteLine(p_coder, "{");
	CoderWriteLine(p_coder, "\treturn java__get_container();");
	CoderWriteLine(p_coder, "}");
	CoderWriteLine(p_coder, "#endif");
	free(t_java_class_name);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool InterfaceGenerate(InterfaceRef self, const char *p_output)
{
	bool t_success;
	t_success = true;
	
	CoderRef t_coder;
	t_coder = nil;
	
	if (t_success)
		t_success = !self -> invalid;
	
	if (t_success)
		t_success = CoderStart(p_output, t_coder);
	
	if (t_success)
		t_success = InterfaceGenerateSupport(self, t_coder);
	
	if (t_success)
		t_success = InterfaceGenerateImports(self, t_coder);
	
	if (t_success)
		t_success = InterfaceGenerateEnums(self, t_coder);
	
	if (t_success)
		t_success = InterfaceGenerateHandlers(self, t_coder);
		
	if (t_success)
		t_success = InterfaceGenerateExports(self, t_coder);
		
	if (t_success)
		t_success = CoderFinish(t_coder);
	else
		CoderCancel(t_coder);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
