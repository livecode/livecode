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
#include "Coder.h"
#include "NativeType.h"
#include "CString.h"

////////////////////////////////////////////////////////////////////////////////

// Includes
// Imports
// Definitions
// Support
// Wrappers
// Exports

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
	else if (NameEqualToCString(p_type, "lc-array"))
		return p_is_ref ? "LCArrayRef" : "const LCArrayRef";
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

static NativeType map_native_type(HandlerMapping p_mapping, NameRef p_type)
{
	if (NameEqualToCString(p_type, "string"))
	{
		switch(p_mapping)
		{
			case kHandlerMappingC:
				return kNativeTypeCString;
			case kHandlerMappingObjC:
				return kNativeTypeObjcString;
			case kHandlerMappingJava:
				return kNativeTypeJavaString;
		}
	}
	else if (NameEqualToCString(p_type, "number"))
	{
		switch(p_mapping)
		{
			case kHandlerMappingC:
				return kNativeTypeReal;
			case kHandlerMappingObjC:
				return kNativeTypeObjcNumber;
			case kHandlerMappingJava:
				return kNativeTypeJavaNumber;
		}
	}
	else if (NameEqualToCString(p_type, "data"))
	{
		switch(p_mapping)
		{
			case kHandlerMappingC:
				return kNativeTypeCData;
			case kHandlerMappingObjC:
				return kNativeTypeObjcData;
			case kHandlerMappingJava:
				return kNativeTypeJavaData;
		}
	}
	else if (NameEqualToCString(p_type, "array"))
	{
		switch(p_mapping)
		{
			case kHandlerMappingC:
				return kNativeTypeCArray;
			case kHandlerMappingObjC:
				return kNativeTypeObjcArray;
			case kHandlerMappingJava:
				return kNativeTypeJavaArray;
		}
	}
	else if (NameEqualToCString(p_type, "dictionary"))
	{
		switch(p_mapping)
		{
			case kHandlerMappingC:
				return kNativeTypeCDictionary;
			case kHandlerMappingObjC:
				return kNativeTypeObjcDictionary;
			case kHandlerMappingJava:
				return kNativeTypeJavaDictionary;
		}
	}
	else if (p_mapping == kHandlerMappingJava)
	{
		switch(NativeTypeFromName(p_type))
		{
		case kNativeTypeCString:
            return kNativeTypeJavaString;
        case kNativeTypeUTF8CString:
            return kNativeTypeJavaUTF8String;
        case kNativeTypeUTF16CString:
            return kNativeTypeJavaUTF16String;
		case kNativeTypeCData:
			return kNativeTypeJavaData;
        case kNativeTypeUTF8CData:
            return kNativeTypeJavaUTF8Data;
        case kNativeTypeUTF16CData:
            return kNativeTypeJavaUTF16Data;
		default:
			break;
		}
	}
	
	return NativeTypeFromName(p_type);
}

////////////////////////////////////////////////////////////////////////////////

class TypeMapper
{
public:
	virtual ~TypeMapper() {};

	virtual const char *GetTypedef(ParameterType mode) = 0;
	
	virtual void Initialize(CoderRef coder, ParameterType mode, const char *name) = 0;
	virtual void Fetch(CoderRef coder, ParameterType mode, const char *name, const char *source) = 0;
	virtual void Default(CoderRef coder, ParameterType mode, const char *name, ValueRef value) = 0;
	virtual void Store(CoderRef coder, ParameterType mode, const char *name, const char *target) = 0;
	virtual void Finalize(CoderRef coder, ParameterType mode, const char *name) = 0;
};

//////////

class PrimitiveTypeMapper: public TypeMapper
{
public:
	virtual const char *GetTypedef(ParameterType mode)
	{
		return GetType();
	}

	virtual void Initialize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		CoderWriteStatement(p_coder, "%s %s", GetType(), p_name);
		CoderWriteStatement(p_coder, "%s = %s", p_name, GetInitializer());
	}
	
	virtual void Fetch(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_source)
	{
		CoderWriteStatement(p_coder, "success = fetch__%s(name__%s, %s, %s)", GetTag(), p_name, p_source, p_name);
	}
	
	virtual void Store(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_target)
	{
		CoderWriteStatement(p_coder, "success = store__%s(%s, %s)", GetTag(), p_target, p_name);
	}
	
	virtual void Finalize(CoderRef coder, ParameterType mode, const char *name)
	{
	}
	
protected:
	virtual const char *GetType(void) const = 0;
	virtual const char *GetInitializer(void) const = 0;
	virtual const char *GetTag(void) const = 0;
};

class BooleanTypeMapper: public PrimitiveTypeMapper
{
public:
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "%s = %s", p_name, BooleanGetBool(p_value) ? "true" : "false");
	}

protected:
	virtual const char *GetType(void) const {return "bool";}
	virtual const char *GetInitializer(void) const {return "false";}
	virtual const char *GetTag(void) const {return "bool";}
};

class IntegerTypeMapper: public PrimitiveTypeMapper
{
public:
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "%s = %lld", p_name, NumberGetInteger(p_value));
	}

protected:
	virtual const char *GetType(void) const {return "int";}
	virtual const char *GetInitializer(void) const {return "0";}
	virtual const char *GetTag(void) const {return "int";}
};

class RealTypeMapper: public PrimitiveTypeMapper
{
public:
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "%s = %lf", p_name, NumberGetReal(p_value));
	}

protected:
	virtual const char *GetType(void) const {return "double";}
	virtual const char *GetInitializer(void) const {return "0.0";}
	virtual const char *GetTag(void) const {return "double";}
};

class ObjcStringTypeMapper: public PrimitiveTypeMapper
{
public:
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "%s = @\"%s\"", p_name, StringGetCStringPtr(p_value));
	}

protected:
	virtual const char *GetType(void) const {return "NSString*";}
	virtual const char *GetInitializer(void) const {return "nil";}
	virtual const char *GetTag(void) const {return "objc_string";}
};

class ObjcDataTypeMapper: public PrimitiveTypeMapper
{
public:
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "success = false");
	}

protected:
	virtual const char *GetType(void) const {return "NSData*";}
	virtual const char *GetInitializer(void) const {return "nil";}
	virtual const char *GetTag(void) const {return "objc_data";}
};

class ObjcNumberTypeMapper: public PrimitiveTypeMapper
{
public:
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "success = false");
	}

protected:
	virtual const char *GetType(void) const {return "NSNumber*";}
	virtual const char *GetInitializer(void) const {return "nil";}
	virtual const char *GetTag(void) const {return "objc_number";}
};

class ObjcArrayTypeMapper: public PrimitiveTypeMapper
{
public:
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "success = false");
	}

protected:
	virtual const char *GetType(void) const {return "NSArray*";}
	virtual const char *GetInitializer(void) const {return "nil";}
	virtual const char *GetTag(void) const {return "objc_array";}
};

class ObjcDictionaryTypeMapper: public PrimitiveTypeMapper
{
public:
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "success = false");
	}

protected:
	virtual const char *GetType(void) const {return "NSDictionary*";}
	virtual const char *GetInitializer(void) const {return "nil";}
	virtual const char *GetTag(void) const {return "objc_dictionary";}
};

//////////

class CTypesTypeMapper : public TypeMapper
{
public:
	virtual void Fetch(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_source)
	{
		CoderWriteStatement(p_coder, "success = fetch__%s(name__%s, %s, %s)", GetTag(), p_name, p_source, p_name);
		CodeInOutCopy(p_coder, p_mode, p_name);
	}
	
	virtual void Store(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_target)
	{
		CoderWriteStatement(p_coder, "success = store__%s(%s, %s)", GetTag(), p_target, p_name);
	}

protected:
	virtual const char *GetTag(void) = 0;

	void CodeInOutCopy(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		if (p_mode != kParameterTypeInOut)
			return;
			
		CoderBeginIf(p_coder, "success");
		CoderWriteStatement(p_coder, "original__%s = %s", p_name, p_name);
		CoderEndIf(p_coder);
	}
};

class CStringTypeMapper: public CTypesTypeMapper
{
public:
    CStringTypeMapper(NativeType p_type)
    {
        m_tag = NativeTypeGetTag(p_type);
    }
    
	virtual const char *GetTypedef(ParameterType mode)
	{
		return mode == kParameterTypeIn ? "const char *" : "char *";
	}
	
	virtual void Initialize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		if (p_mode == kParameterTypeInOut)
		{
			CoderWriteStatement(p_coder, "char *%s, *original__%s", p_name, p_name);
			CoderWriteStatement(p_coder, "original__%s = %s = nil;", p_name, p_name);
		}
		else
		{
			CoderWriteStatement(p_coder, "char *%s", p_name);
			CoderWriteStatement(p_coder, "%s = nil", p_name);
		}
	}
	
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "success = default__%s(\"%s\", %s)", GetTag(), StringGetCStringPtr(p_value), p_name);
		CodeInOutCopy(p_coder, p_mode, p_name);
	}
	
	virtual void Finalize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		CoderWriteStatement(p_coder, "free(%s)", p_name);
		if (p_mode == kParameterTypeInOut)
		{
			CoderBeginIf(p_coder, "%s != original__%s", p_name, p_name);
			CoderWriteStatement(p_coder, "free(original__%s)", p_name);
			CoderEndIf(p_coder);
		}
	}
	
protected:
	const char *GetTag(void) {return m_tag;}
    
private:
    const char *m_tag;
};

class CDataTypeMapper: public CTypesTypeMapper
{
public:
    
    CDataTypeMapper(NativeType p_type)
    {
        m_tag = NativeTypeGetTag(p_type);
    }
    
	virtual const char *GetTypedef(ParameterType mode)
	{
		return "LCBytes";
	}

	virtual void Initialize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		if (p_mode == kParameterTypeInOut)
		{
			CoderWriteStatement(p_coder, "LCBytes %s, original__%s", p_name, p_name);
			CoderWriteStatement(p_coder, "original__%s . buffer = %s . buffer = nil;", p_name, p_name);
			CoderWriteStatement(p_coder, "original__%s . length = %s . length = 0;", p_name, p_name);
		}
		else
		{
			CoderWriteStatement(p_coder, "LCBytes %s", p_name);
			CoderWriteStatement(p_coder, "%s . buffer = nil", p_name);
			CoderWriteStatement(p_coder, "%s . length = 0", p_name);
		}
	}
	
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "success = false", StringGetCStringPtr(p_value), p_name);
		CodeInOutCopy(p_coder, p_mode, p_name);
	}
	
	virtual void Finalize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		CoderWriteStatement(p_coder, "free(%s . buffer)", p_name);
		if (p_mode == kParameterTypeInOut)
		{
			CoderBeginIf(p_coder, "%s . buffer != original__%s . buffer", p_name, p_name);
			CoderWriteStatement(p_coder, "free(original__%s . buffer)", p_name);
			CoderEndIf(p_coder);
		}
	}
	
protected:
	const char *GetTag(void) {return m_tag;}
    
private:
    const char* m_tag;
};

class LCArrayTypeMapper: public CTypesTypeMapper
{
public:
    
    LCArrayTypeMapper(NativeType p_type)
    {
        m_tag = NativeTypeGetTag(p_type);
    }
    
	virtual const char *GetTypedef(ParameterType mode)
	{
		return mode == kParameterTypeIn ? "const LCArrayRef" : "LCArrayRef";
	}

	virtual void Initialize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		if (p_mode == kParameterTypeInOut)
		{
			CoderWriteStatement(p_coder, "LCArrayRef %s, original__%s", p_name, p_name);
			CoderWriteStatement(p_coder, "original__%s = %s = nil;", p_name, p_name);
		}
		else
		{
			CoderWriteStatement(p_coder, "LCArrayRef %s", p_name);
			CoderWriteStatement(p_coder, "%s = nil", p_name);
		}
	}
	
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "success = false", StringGetCStringPtr(p_value), p_name);
		CodeInOutCopy(p_coder, p_mode, p_name);
	}
	
	virtual void Finalize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		CoderWriteStatement(p_coder, "LCArrayRelease(%s)", p_name);
		if (p_mode == kParameterTypeInOut)
		{
			CoderBeginIf(p_coder, "%s != original__%s", p_name, p_name);
			CoderWriteStatement(p_coder, "LCArrayRelease(original__%s)", p_name);
			CoderEndIf(p_coder);
		}
	}

protected:
	const char *GetTag(void) {return m_tag;}
    
private:
    const char* m_tag;
};

//////////

class EnumTypeMapper: public TypeMapper
{
public:
	EnumTypeMapper(NameRef p_type_name, int64_t p_default)
	{
		m_type_name = p_type_name;
		m_type_default = p_default;
	}

	virtual const char *GetTypedef(ParameterType mode)
	{
		return "int";
	}

	virtual void Initialize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		CoderWriteStatement(p_coder, "int %s", p_name);
		CoderWriteStatement(p_coder, "%s = 0", p_name);
	}
	
	virtual void Fetch(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_source)
	{
		CoderWriteStatement(p_coder, "success = fetchenum__%s(name__%s, %s, %s)", name_to_cname(m_type_name), p_name, p_source, p_name);
	}
	
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "%s = %lld", p_name, m_type_default);
	}
	
	virtual void Store(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_target)
	{
		CoderWriteStatement(p_coder, "success = storeenum__%s(%s, %s);", name_to_cname(m_type_name), p_target, p_name);
	}
	
	virtual void Finalize(CoderRef coder, ParameterType mode, const char *name)
	{
	}
	
private:
	NameRef m_type_name;
	int64_t m_type_default;
};

//////////

class JavaStringTypeMapper: public TypeMapper
{
public:
    JavaStringTypeMapper(NativeType p_type)
    {
        m_tag = NativeTypeGetTag(p_type);
    }
    
	virtual const char *GetTypedef(ParameterType type)
	{
		return "__INTERNAL_ERROR__";
	}

	virtual void Initialize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		CoderWriteStatement(p_coder, "jobject %s", p_name);
		CoderWriteStatement(p_coder, "%s = nil", p_name);
	}
	
	virtual void Fetch(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_source)
	{
		CoderWriteStatement(p_coder, "success = fetch__%s(__java_env, name__%s, %s, %s)", m_tag, p_name, p_source, p_name);
	}
	
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "success = default__java_string(__java_env, \"%s\", %s)", StringGetCStringPtr(p_value), p_name);
	}
	
	virtual void Store(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_target)
	{
		CoderWriteStatement(p_coder, "success = store__%s(__java_env, %s, %s)", m_tag, p_target, p_name);
	}
	
	virtual void Finalize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		CoderWriteStatement(p_coder, "free__java_string(__java_env, %s)", p_name);
	}
    
private:
    const char* m_tag;
};

class JavaDataTypeMapper: public TypeMapper
{
public:
    
    JavaDataTypeMapper(NativeType p_type)
    {
        m_tag = NativeTypeGetTag(p_type);
    }
    
	virtual const char *GetTypedef(ParameterType type)
	{
		return "__INTERNAL_ERROR__";
	}

	virtual void Initialize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		CoderWriteStatement(p_coder, "jobject %s", p_name);
		CoderWriteStatement(p_coder, "%s = nil", p_name);
	}
	
	virtual void Fetch(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_source)
	{
		CoderWriteStatement(p_coder, "success = fetch__%s(__java_env, name__%s, %s, %s)", m_tag, p_name, p_source, p_name);
	}
	
	virtual void Default(CoderRef p_coder, ParameterType p_mode, const char *p_name, ValueRef p_value)
	{
		CoderWriteStatement(p_coder, "success = false", StringGetCStringPtr(p_value), p_name);
	}
	
	virtual void Store(CoderRef p_coder, ParameterType p_mode, const char *p_name, const char *p_target)
	{
		CoderWriteStatement(p_coder, "success = store__%s(__java_env, %s, %s)", m_tag, p_target, p_name);
	}
	
	virtual void Finalize(CoderRef p_coder, ParameterType p_mode, const char *p_name)
	{
		CoderWriteStatement(p_coder, "free__java_data(__java_env, %s)", p_name);
	}
    
private:
    const char* m_tag;
};

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

extern "C" const char *g_support_template[];

static bool InterfaceGenerateSupport(InterfaceRef self, CoderRef p_coder)
{
	if (self -> use_cpp_exceptions)
		CoderWriteLine(p_coder, "#include <exception>");
	
	for(int i = 0; g_support_template[i] != nil; i++)
		CoderWrite(p_coder, "%s", g_support_template[i]);
	
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
		
#if NOT_USED
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
#endif
		
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

#if NOT_USED
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
	CoderWriteLine(p_coder, "\t\ts_method = s_android_env -> GetStaticMethodID(s_java_class, \"%s\", \"%s\");", NameGetCString(t_name), t_java_sig);
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
		CoderWriteLine(p_coder, "\tt_param_%d = java_from__%s(s_android_env, ctxt -> arg_%d);", i, NativeTypeGetTag(t_param_native_type), i);
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
		CoderWrite(p_coder, "\tt_result = s_android_env -> CallStatic%sMethod(s_java_class, s_method", t_java_method_type);
	else
		CoderWrite(p_coder, "\ts_android_env -> CallStatic%sMethod(s_java_class, s_method", t_java_method_type);
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
		CoderWrite(p_coder, ", t_param_%d", i);
	CoderWrite(p_coder, ");\n");
	
	if (t_variant -> return_type != nil)
	{
		CoderWriteLine(p_coder, "\tctxt -> result = java_to__%s(s_android_env, t_result);", NativeTypeGetTag(NativeTypeFromName(t_variant -> return_type)));
		CoderWriteLine(p_coder, "\tjava_free__%s(s_android_env, t_result);", NativeTypeGetTag(NativeTypeFromName(t_variant -> return_type)));
	}
	
	for(uindex_t i = 0; i < t_variant -> parameter_count; i++)
		CoderWriteLine(p_coder, "\tjava_free__%s(s_android_env, t_param_%d);", NativeTypeGetTag(NativeTypeFromName(t_variant -> parameters[i] . type)), i);
	
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

static void InterfaceGenerateJavaHandlerStubParameters(InterfaceRef self, CoderRef p_coder, Handler *p_handler, HandlerVariant *p_variant)
{
	for(uindex_t i = 0; i < p_variant -> parameter_count; i++)
	{
		if (i != 0)
			CoderWrite(p_coder, ", ");
		
		const char *t_type;
		t_type = native_type_to_type_in_cstring(NativeTypeFromName(p_variant -> parameters[i] . type));
		
		CoderWrite(p_coder, "%s p_param_%d", t_type, i);
	}
}

static void InterfaceGenerateJavaHandlerStub(InterfaceRef self, CoderRef p_coder, Handler *p_handler, HandlerVariant *p_variant)
{	
	// Compute the return types for various contexts.
	const char *t_java_method_type, *t_c_return_type, *t_java_return_type;
	if (p_variant -> return_type != nil)
	{
		t_java_method_type = native_type_to_java_method_type_cstring(NativeTypeFromName(p_variant -> return_type));
		t_c_return_type = native_type_to_type_out_cstring(NativeTypeFromName(p_variant -> return_type));
		t_java_return_type = native_type_to_java_type_cstring(NativeTypeFromName(p_variant -> return_type));
	}
	else
	{
		t_java_method_type = "Void";
		t_c_return_type = "void";
		t_java_return_type = "void";
	}
	
	// Compute the env var.
	const char *t_env_var;
	if (p_handler -> is_tail)
		t_env_var = "s_android_env";
	else
		t_env_var = "s_engine_env";
	
	// Generate the signature.
	CoderWrite(p_coder, "%s %s(", t_c_return_type, NameGetCString(p_variant -> binding));
	InterfaceGenerateJavaHandlerStubParameters(self, p_coder, p_handler, p_variant);
	CoderWriteLine(p_coder, ")");
	CoderWriteLine(p_coder, "{");
	
	// Generate code to fetch the static method id.
	char *t_java_sig;
	t_java_sig = InterfaceGenerateJavaHandlerStubSignature(self, p_handler, p_variant);
	CoderWriteLine(p_coder, "\tstatic jmethodID s_method = 0;");
	CoderWriteLine(p_coder, "\tif (s_method == 0)");
	CoderWriteLine(p_coder, "\t\ts_method = %s -> GetStaticMethodID(s_java_class, \"%s\", \"%s\");", t_env_var, NameGetCString(p_variant -> binding), t_java_sig);
	free(t_java_sig);
	
	for(uindex_t i = 0; i < p_variant -> parameter_count; i++)
	{
		HandlerParameter *t_parameter;
		t_parameter = &p_variant -> parameters[i];
		
		NativeType t_param_native_type;
		t_param_native_type = NativeTypeFromName(t_parameter -> type);
		
		const char *t_java_param_type;
		t_java_param_type = native_type_to_java_type_cstring(t_param_native_type);
		
		CoderWriteLine(p_coder, "\t%s t_param_%d;", t_java_param_type, i);
		CoderWriteLine(p_coder, "\tt_param_%d = java_from__%s(%s, p_param_%d);", i, NativeTypeGetTag(t_param_native_type), t_env_var, i);
	}
	
	if (p_variant -> return_type != nil)
		CoderWriteLine(p_coder, "\t%s t_java_result;", t_java_return_type);
	
	if (p_variant -> return_type != nil)
		CoderWrite(p_coder, "\tt_java_result = %s -> CallStatic%sMethod(s_java_class, s_method", t_env_var, t_java_method_type);
	else
		CoderWrite(p_coder, "\t%s -> CallStatic%sMethod(s_java_class, s_method", t_env_var, t_java_method_type);
	for(uindex_t i = 0; i < p_variant -> parameter_count; i++)
		CoderWrite(p_coder, ", t_param_%d", i);
	CoderWrite(p_coder, ");\n");
	
	if (p_variant -> return_type != nil)
	{
		CoderWriteLine(p_coder, "\t%s t_result;", native_type_to_type_out_cstring(NativeTypeFromName(p_variant -> return_type)));
		CoderWriteLine(p_coder, "\tt_result = java_to__%s(%s, t_java_result);", NativeTypeGetTag(NativeTypeFromName(p_variant -> return_type)), t_env_var);
		CoderWriteLine(p_coder, "\tjava_free__%s(%s, t_java_result);", NativeTypeGetTag(NativeTypeFromName(p_variant -> return_type)), t_env_var);
	}
	
	for(uindex_t i = 0; i < p_variant -> parameter_count; i++)
		CoderWriteLine(p_coder, "\tjava_free__%s(%s, t_param_%d);", NativeTypeGetTag(NativeTypeFromName(p_variant -> parameters[i] . type)), t_env_var, i);
	
	if (p_variant -> return_type != nil)
		CoderWriteLine(p_coder, "\treturn t_result;");
		
	CoderWriteLine(p_coder, "}");

}

#endif

static char *InterfaceGenerateJavaHandlerStubSignature(InterfaceRef self, Handler *p_handler, HandlerVariant *p_variant)
{
	char *t_signature;
	t_signature = nil;
	MCCStringAppend(t_signature, "(");
	for(uindex_t i = 0; i < p_variant -> parameter_count; i++)
		MCCStringAppend(t_signature, native_type_to_java_sig(NativeTypeFromName(p_variant -> parameters[i] . type)));
	MCCStringAppend(t_signature, ")");
	MCCStringAppend(t_signature, p_variant -> return_type != nil ? native_type_to_java_sig(NativeTypeFromName(p_variant -> return_type)) : "V");
	return t_signature;
}

////////////////////////////////////////////////////////////////////////////////

struct MappedParameter
{
	const char *name;
	TypeMapper *mapper;
	char *arg_name;
	char *var_name;
	ParameterType mode;
	ValueRef default_value;
	bool is_optional;
	
	MappedParameter(void)
	{
		memset(this, 0, sizeof(*this));
	}
	
	~MappedParameter(void)
	{
		delete mapper;
		MCCStringFree(arg_name);
		MCCStringFree(var_name);
	}
};

static TypeMapper *map_parameter_type(InterfaceRef self, HandlerMapping p_mapping, NameRef p_type, ValueRef p_default_value)
{
	NativeType t_type;
	t_type = map_native_type(p_mapping, p_type);
	switch(t_type)
	{
	case kNativeTypeBoolean: return new BooleanTypeMapper;
	case kNativeTypeInteger: return new IntegerTypeMapper;
	case kNativeTypeReal: return new RealTypeMapper;
	case kNativeTypeEnum: return new EnumTypeMapper(p_type, p_default_value != nil ? InterfaceResolveEnumElement(self, p_type, p_default_value) : 0);
    case kNativeTypeCString:
    case kNativeTypeUTF8CString:
    case kNativeTypeUTF16CString:
        return new CStringTypeMapper(t_type);
    case kNativeTypeCData:
    case kNativeTypeUTF8CData:
    case kNativeTypeUTF16CData:
        return new CDataTypeMapper(t_type);
	case kNativeTypeLCArray: return new LCArrayTypeMapper(t_type);
	case kNativeTypeObjcString: return new ObjcStringTypeMapper;
	case kNativeTypeObjcNumber: return new ObjcNumberTypeMapper;
	case kNativeTypeObjcData: return new ObjcDataTypeMapper;
	case kNativeTypeObjcArray: return new ObjcArrayTypeMapper;
	case kNativeTypeObjcDictionary: return new ObjcDictionaryTypeMapper;
    case kNativeTypeJavaString:
    case kNativeTypeJavaUTF8String:
    case kNativeTypeJavaUTF16String:
        return new JavaStringTypeMapper(t_type);
	case kNativeTypeJavaData:
    case kNativeTypeJavaUTF8Data:
    case kNativeTypeJavaUTF16Data:
        return new JavaDataTypeMapper(t_type);
	case kNativeTypeCArray:
	case kNativeTypeCDictionary:
	case kNativeTypeJavaArray:
	case kNativeTypeJavaNumber:
	case kNativeTypeJavaDictionary:
	case kNativeTypeNone:
		return nil;
	}
	
	MCUnreachableReturn(nil);
}

static void map_parameter(InterfaceRef self, HandlerMapping p_mapping, HandlerParameter *p_parameter, uint32_t k, MappedParameter& r_param)
{
	r_param . name = NameGetCString(p_parameter -> name);
	MCCStringFormat(r_param . var_name, "param__%s", r_param . name);
	MCCStringFormat(r_param . arg_name, "argv[%d]", k);
	r_param . mode = p_parameter -> mode;
	r_param . mapper = map_parameter_type(self, p_mapping, p_parameter -> type, p_parameter -> default_value);
	r_param . default_value = p_parameter -> default_value;
	r_param . is_optional = p_parameter -> is_optional;
}

static void InterfaceGenerateVariant(InterfaceRef self, CoderRef p_coder, Handler *p_handler, HandlerVariant *p_variant, HandlerMapping p_mapping)
{
	Handler *t_handler;
	t_handler = p_handler;
	
	HandlerVariant *t_variant;
	t_variant = p_variant;
	
	bool t_is_java;
	t_is_java = p_mapping == kHandlerMappingJava;
	
	MappedParameter *t_mapped_params;
	t_mapped_params = new (nothrow) MappedParameter[t_variant -> parameter_count];
	for(uint32_t k = 0; k < t_variant -> parameter_count; k++)
		map_parameter(self, p_mapping, &t_variant -> parameters[k], k, t_mapped_params[k]);
	
	// If c++ then we can use references for indirect return values.
	const char *t_ref_char;
	if (self -> use_cpp_naming)
		t_ref_char = "";
	else
		t_ref_char = "&";
		
	// Get the type mapper for the return type.
	TypeMapper *t_return_type_mapper;
	if (t_variant -> return_type != nil)
		t_return_type_mapper = map_parameter_type(self, p_mapping, t_variant -> return_type, nil);
	else
		t_return_type_mapper = nil;
		
	// Generate the import - but not for Java
	if (!t_is_java)
	{
		CoderBeginStatement(p_coder);
		
		CoderWrite(p_coder, "%s %s %s(",
							InterfaceGetExternPrefix(self), 
							t_return_type_mapper != nil && !t_variant -> return_type_indirect ? t_return_type_mapper -> GetTypedef(kParameterTypeOut) : "void",
							NameGetCString(t_variant -> binding));
							
		bool t_has_param;
		t_has_param = false;
		if (t_return_type_mapper != nil && t_variant -> return_type_indirect)
		{
			CoderWrite(p_coder, "%s%s", t_return_type_mapper -> GetTypedef(kParameterTypeOut), InterfaceGetReferenceSuffix(self));
			t_has_param = true;
		}
		
		if (t_variant -> parameter_count != 0)
			for(uint32_t k = 0; k < t_variant -> parameter_count; k++)
			{
				if (t_has_param)
					CoderWrite(p_coder, ", ");
				CoderWrite(p_coder, "%s%s",
						   t_mapped_params[k] . mapper -> GetTypedef(t_mapped_params[k] . mode),
						   t_mapped_params[k] . mode != kParameterTypeIn ? InterfaceGetReferenceSuffix(self) : "");
				t_has_param = true;
			}
			
		if (!t_has_param)
			CoderWrite(p_coder, "void");
			
		CoderWrite(p_coder, ")");
		
		CoderEndStatement(p_coder);
	}
	
	CoderBegin(p_coder, "static bool variant__%s(MCVariableRef *argv, uint32_t argc, MCVariableRef result)", NameGetCString(t_variant -> binding));
	if (self -> use_objc_objects && !t_is_java)
	{
		CoderBeginPreprocessor(p_coder, "#ifdef __OBJC__");
		CoderWriteStatement(p_coder, "NSAutoreleasePool *t_pool");
		CoderWriteStatement(p_coder, "t_pool = [[NSAutoreleasePool alloc] init]");
		CoderEndPreprocessor(p_coder, "#endif");
		CoderPad(p_coder);
	}
	
	// If java, then emit a standard binding for the correct env.
	if (t_is_java)
	{
		CoderWriteStatement(p_coder, "JNIEnv *__java_env");
		CoderWriteStatement(p_coder, "__java_env = %s", p_handler -> is_tail ? "s_android_env" : "s_engine_env");
		CoderPad(p_coder);
	}
	
	CoderWriteStatement(p_coder, "bool success");
	CoderWriteStatement(p_coder, "success = true");
	CoderPad(p_coder);
	
	for(uint32_t k = 0; k < t_variant -> parameter_count; k++)
	{
		MappedParameter *t_parameter;
		t_parameter = &t_mapped_params[k];
		
		// Generate the name cstring (used by fetch/store for error reporting).
		CoderWriteStatement(p_coder, "const char *name__%s = \"%s\"", t_parameter -> var_name, t_parameter -> name);
		
		// Generate the appropriate variable declarations and initialize.
		t_parameter -> mapper -> Initialize(p_coder, t_parameter -> mode, t_parameter -> var_name);
		
		// Make sure the var is suitable for out (if out/inout).
		if (t_parameter -> mode == kParameterTypeInOut || t_parameter -> mode == kParameterTypeOut)
		{
			CoderBeginIf(p_coder, "success");
			CoderWriteStatement(p_coder, "success = verify__out_parameter(name__%s, %s);", t_parameter -> var_name, t_parameter -> arg_name);
			CoderEndIf(p_coder);
		}
		
		// Generate the fetch (if in / inout).
		if (t_parameter -> mode == kParameterTypeInOut || t_parameter -> mode == kParameterTypeIn)
		{
			CoderBeginIf(p_coder, "success");
			if (t_parameter -> is_optional)
			{
				CoderBeginIf(p_coder, "argc > %d", k);
				t_parameter -> mapper -> Fetch(p_coder, t_parameter -> mode, t_parameter -> var_name, t_parameter -> arg_name);
				if (t_parameter -> default_value != nil)
				{
					CoderElse(p_coder);
					t_parameter -> mapper -> Default(p_coder, t_parameter -> mode, t_parameter -> var_name, t_parameter -> default_value);
				}
				CoderEndIf(p_coder);
			}
			else
				t_parameter -> mapper -> Fetch(p_coder, t_parameter -> mode, t_parameter -> var_name, t_parameter -> arg_name);
			CoderEndIf(p_coder);
		}
		
		CoderPad(p_coder);
	}
	
	// If we have a return value, generate the initializer for it.
	if (t_return_type_mapper != nil)
		t_return_type_mapper -> Initialize(p_coder, kParameterTypeOut, "returnvalue");
	
	// Now generate the method call.
	CoderBeginIf(p_coder, "success");
	CoderWriteStatement(p_coder, "error__clear()");
	
	if (!t_is_java)
	{
		if (self -> use_cpp_exceptions)
			CoderBegin(p_coder, "try");
		if (self -> use_objc_exceptions)
		{
			CoderBeginPreprocessor(p_coder, "#ifdef __OBJC__");
			CoderBegin(p_coder, "NS_DURING");
			CoderEndPreprocessor(p_coder, "#endif");
		}
		
		CoderBeginStatement(p_coder);
		
		if (t_return_type_mapper != nil)
		{
			if (!t_variant -> return_type_indirect)
				CoderWrite(p_coder, "returnvalue = %s(", NameGetCString(t_variant -> binding));
			else
				CoderWrite(p_coder, "%s(%sreturnvalue", NameGetCString(t_variant -> binding), t_ref_char);
		}
		else
			CoderWrite(p_coder, "%s(", NameGetCString(t_variant -> binding));
        
        for(uint32_t k = 0; k < p_variant -> parameter_count; k++)
        {
			if (k > 0 || t_variant -> return_type_indirect)
				CoderWrite(p_coder, ", ");
            CoderWrite(p_coder, "%s", t_mapped_params[k] . var_name);
		}
			
        CoderWrite(p_coder, ")");
		
		CoderEndStatement(p_coder);
		
		if (self -> use_objc_exceptions)
		{
			CoderBeginPreprocessor(p_coder, "#ifdef __OBJC__");
			CoderEndBegin(p_coder, "NS_HANDLER");
			CoderWriteStatement(p_coder, "success = error__raise([[localException reason] cStringUsingEncoding: NSMacOSRomanStringEncoding])");
			CoderEnd(p_coder, "NS_ENDHANDLER");
			CoderEndPreprocessor(p_coder, "#endif");
		}
		
		if (self -> use_cpp_exceptions)
		{
			CoderEndBegin(p_coder, "catch(std::exception& t_exception)");
			CoderWriteStatement(p_coder, "success = error__raise(t_exception . what());");
			CoderEndBegin(p_coder, "catch(...)");
			CoderWriteStatement(p_coder, "success = error__unknown();");
			CoderEnd(p_coder, "");
		}
	}
	else
	{
		char *t_java_signature;
		t_java_signature = InterfaceGenerateJavaHandlerStubSignature(self, p_handler, p_variant);
		
		const char *t_java_method_type;
		if (p_variant -> return_type != nil)
			t_java_method_type = native_type_to_java_method_type_cstring(NativeTypeFromName(p_variant -> return_type));
		else
			t_java_method_type = "Void";
		
		CoderWriteStatement(p_coder, "static jmethodID s_method = 0");
		CoderBeginIf(p_coder, "s_method == 0");
		CoderWriteStatement(p_coder, "s_method = __java_env -> GetStaticMethodID(s_java_class, \"%s\", \"%s\")", NameGetCString(p_variant -> binding), t_java_signature);
		CoderEndIf(p_coder);
		CoderBeginStatement(p_coder);
		if (t_return_type_mapper != nil)
			CoderWrite(p_coder, "returnvalue = __java_env -> CallStatic%sMethod(s_java_class, s_method", t_java_method_type);
		else
			CoderWrite(p_coder, "__java_env -> CallStaticVoidMethod(s_java_class, s_method");
		
        for(uint32_t k = 0; k < p_variant -> parameter_count; k++)
			CoderWrite(p_coder, ", %s", t_mapped_params[k] . var_name);
        
		CoderWrite(p_coder, ")");
		CoderEndStatement(p_coder);
		
		MCCStringFree(t_java_signature);
	}
	
	CoderWriteStatement(p_coder, "success = error__catch()");
	CoderEndIf(p_coder);
	CoderPad(p_coder);
	
	// Now generate the returnvalue store.
	if (t_return_type_mapper != nil)
	{
		CoderBeginIf(p_coder, "success");
		t_return_type_mapper -> Store(p_coder, kParameterTypeOut, "returnvalue", "result");
		CoderEndIf(p_coder);
		CoderPad(p_coder);
	}
	
	for(uint32_t k = t_variant -> parameter_count; k > 0; k--)
	{
		MappedParameter *t_parameter;
		t_parameter = &t_mapped_params[k - 1];
		
		if (t_parameter -> mode == kParameterTypeIn)
			continue;

		CoderBeginIf(p_coder, "success");
		t_parameter -> mapper -> Store(p_coder, t_parameter -> mode, t_parameter -> var_name, t_parameter -> arg_name);
		CoderEndIf(p_coder);
		
		CoderPad(p_coder);
	}
	
	CoderBeginIf(p_coder, "!success");
	CoderWriteStatement(p_coder, "success = error__report(result)");
	CoderEndIf(p_coder);
	CoderPad(p_coder);
	
	if (t_return_type_mapper != nil)
		t_return_type_mapper -> Finalize(p_coder, kParameterTypeOut, "returnvalue");
		
	for(uint32_t k = t_variant -> parameter_count; k > 0; k--)
		t_mapped_params[k - 1] . mapper -> Finalize(p_coder, t_mapped_params[k - 1] . mode, t_mapped_params[k - 1] . var_name);
		
	CoderPad(p_coder);
		
	if (self -> use_objc_objects && !t_is_java)
	{
		CoderBeginPreprocessor(p_coder, "#ifdef __OBJC__");
		CoderWriteStatement(p_coder, "[t_pool release]");
		CoderEndPreprocessor(p_coder, "#endif");
		CoderPad(p_coder);
	}
	
	CoderWriteStatement(p_coder, "return success");
	
	CoderEnd(p_coder, "");
	
	delete t_return_type_mapper;
	delete[] t_mapped_params;
}

static void InterfaceGenerateUnsupportedVariant(InterfaceRef self, CoderRef p_coder, Handler *p_handler, HandlerVariant *p_variant, HandlerMapping p_mapping)
{
	CoderBegin(p_coder, "static bool variant__%s(MCVariableRef *argv, uint32_t argc, MCVariableRef result)", NameGetCString(p_variant -> binding));
	CoderWriteStatement(p_coder, "return error__report_not_supported(result)");
	CoderEnd(p_coder, "");
}

////////////////////////////////////////////////////////////////////////////////

static bool InterfaceGenerateHandlers(InterfaceRef self, CoderRef p_coder)
{
	for(uint32_t i = 0; i < self -> handler_count; i++)
	{
		Handler *t_handler;
		t_handler = &self -> handlers[i];

		for(uint32_t j = 0; j < t_handler -> variant_count; j++)
		{
			HandlerVariant *t_variant;
			t_variant = &t_handler -> variants[j];
			
			for(HandlerMapping t_mapping = kHandlerMappingNone; t_mapping <= kHandlerMappingJava; t_mapping++)
			{
				// Find all platforms requiring the current mapping type.
				uint32_t t_platforms;
				t_platforms = 0;
				for(Platform t_platform = kPlatformMac; t_platform < __kPlatformCount__; t_platform++)
					if (t_variant -> mappings[t_platform] == t_mapping)
						t_platforms |= 1 << t_platform;
						
				// If there are none, there is nothing to do.
				if (t_platforms == 0)
					continue;
					
				char *t_defineds;
				t_defineds = nil;
				for(Platform t_platform = kPlatformMac; t_platform < __kPlatformCount__; t_platform++)
				{
					static const char *s_platform_macros[] = {"__MAC__", "__WINDOWS__", "__LINUX__", "__IOS__", "__ANDROID__"};
					if ((t_platforms & (1 << t_platform)) == 0)
						continue;
					if (t_defineds != nil)
						MCCStringAppend(t_defineds, " || ");
					MCCStringAppendFormat(t_defineds, "defined(%s)", s_platform_macros[t_platform]);
				}
				
				// Generate the variant
				CoderBeginPreprocessor(p_coder, "#if %s", t_defineds);
				if (t_mapping != kHandlerMappingNone)
					InterfaceGenerateVariant(self, p_coder, t_handler, t_variant, t_mapping);
				else
					InterfaceGenerateUnsupportedVariant(self, p_coder, t_handler, t_variant, t_mapping);
				CoderEndPreprocessor(p_coder, "#endif");
				
				MCCStringFree(t_defineds);
			}
		}
		
		if (t_handler -> is_tail)
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
				else if (t_variant -> minimum_parameter_count == 0)
                	CoderWriteLine(p_coder, "\tif (env -> argc <= %d)", t_variant -> parameter_count);
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
			CoderWriteLine(p_coder, "\thandler__%s_env_t env;", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "\tenv . argv = argv;");
			CoderWriteLine(p_coder, "\tenv . argc = argc;");
			CoderWriteLine(p_coder, "\tenv . result = result;");
			CoderWriteLine(p_coder, "#if defined(__IOS__) || defined(__ANDROID__)");
			CoderWriteLine(p_coder, "\tif (s_interface -> version >= 4)");
			CoderWriteLine(p_coder, "\t\ts_interface -> engine_run_on_main_thread((void *)do_handler__%s, &env, kMCRunOnMainThreadJumpToUI);", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "\telse");
			CoderWriteLine(p_coder, "\t\tdo_handler__%s(&env);", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "#else");
			CoderWriteLine(p_coder, "\t\tdo_handler__%s(&env);", NameGetCString(t_handler -> name));
			CoderWriteLine(p_coder, "#endif");
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
#ifndef __WINDOWS__\n\
#define DLLEXPORT\n\
extern \"C\" MCExternalInfo *MCExternalDescribe(void) __attribute__((visibility(\"default\")));\n\
extern \"C\" bool MCExternalInitialize(MCExternalInterface *) __attribute__((visibility(\"default\")));\n\
extern \"C\" void MCExternalFinalize(void) __attribute__((visibility(\"default\")));\n\
#else\n\
#define DLLEXPORT __declspec(dllexport)\n\
extern \"C\" MCExternalInfo DLLEXPORT *MCExternalDescribe(void);\n\
extern \"C\" bool DLLEXPORT MCExternalInitialize(MCExternalInterface *);\n\
extern \"C\" void DLLEXPORT MCExternalFinalize(void);\n\
#endif\n\
\
\n\
MCExternalInfo DLLEXPORT *MCExternalDescribe(void)\n\
{\n\
	static MCExternalInfo s_info;\n\
	s_info . version = 1;\n\
	s_info . flags = 0;\n\
	s_info . name = kMCExternalName;\n\
	s_info . handlers = kMCExternalHandlers;\n\
	return &s_info;\n\
}\n\
\n\
bool DLLEXPORT MCExternalInitialize(MCExternalInterface *p_interface)\n\
{\n\
	s_interface = p_interface;\n\
\n\
#ifndef __ANDROID__\n\
	if (s_interface -> version < 3)\n\
		return false;\n\
#else\n\
	if (s_interface -> version < 5)\n\
		return false;\n\
\n\
	s_interface -> interface_query(kMCExternalInterfaceQueryScriptJavaEnv, &s_engine_env);\n\
	s_interface -> interface_query(kMCExternalInterfaceQuerySystemJavaEnv, &s_android_env);\n\
\n\
	if (!java__initialize(s_engine_env))\n\
		return false;\n\
\n\
#endif\n\
\n\
#ifdef kMCExternalStartup\n\
	if (!kMCExternalStartup())\n\
		return false;\n\
#endif\n\
\n\
	return true;\n\
}\n\
\n\
void DLLEXPORT MCExternalFinalize(void)\n\
{\n\
#ifdef kMCExternalShutdown\n\
	kMCExternalShutdown();\n\
#endif\n\
\n\
#ifdef __ANDROID__\n\
	java__finalize(s_engine_env);\n\
#endif\n\
\n\
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

		// Don't declare methods in the external handlers table.
		if (t_handler -> type == kHandlerTypeMethod)
			continue;
		
		CoderWriteLine(p_coder, "\t{ %d, \"%s\", handler__%s },",
			t_handler -> type == kHandlerTypeCommand ? 1 : 2,
			NameGetCString(t_handler -> name),
			NameGetCString(t_handler -> name));
	}
	CoderWriteLine(p_coder, "\t{ 0, NULL, NULL }\n};");
	
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
	CoderWriteLine(p_coder, "\tJNIEnv *t_java_env; s_java_vm -> GetEnv((void **)&t_java_env, JNI_VERSION_1_2);");
	CoderWriteLine(p_coder, "\ts_java_class = (jclass)t_java_env -> NewGlobalRef(t_java_env -> FindClass(\"%s/%s\"));", t_java_class_name, NameGetCString(self -> name));
	CoderWriteLine(p_coder, "");
	CoderWriteLine(p_coder, "\treturn JNI_VERSION_1_2;");
	CoderWriteLine(p_coder, "}");
	free(t_java_class_name);

	static struct { const char *name; const char *result; const char *signature; const char *args; } s_java_exports[] =
	{
		{ "InterfaceQueryActivity", "jobject", nil, nil },
		{ "InterfaceQueryContainer", "jobject", nil, nil },
		{ "InterfaceQueryEngine", "jobject", nil, nil },
		{ "InterfaceQueryViewScale", "jdouble", nil, nil },
		{ "ObjectResolve", "jlong", "jobject chunk", "chunk" },
		{ "ObjectRetain", "void", "jlong object", "object" },
		{ "ObjectRelease", "void", "jlong object", "object" },
		{ "ObjectExists", "jboolean", "jlong object", "object" },
		{ "ObjectSend", "void", "jlong object, jobject message, jobjectArray arguments", "object, message, arguments" },
		{ "ObjectPost", "void", "jlong object, jobject message, jobjectArray arguments", "object, message, arguments" },
		{ "ContextMe", "jlong", nil, nil },
		{ "ContextTarget", "jlong", nil, nil },
		{ "RunOnSystemThread", "void", "jobject runnable", "runnable" },
		{ "WaitCreate", "jlong", "jint options", "options" },
		{ "WaitRelease", "void", "jlong wait", "wait" },
		{ "WaitIsRunning", "jboolean", "jlong wait", "wait" },
		{ "WaitRun", "void", "jlong wait", "wait" },
		{ "WaitReset", "void", "jlong wait", "wait" },
		{ "WaitBreak", "void", "jlong wait", "wait" },

	};
	t_java_class_name = strdup(NameGetCString(self -> qualified_name));
	for(uint32_t i = 0; t_java_class_name[i] != '\0'; i++)
		if (t_java_class_name[i] == '.')
			t_java_class_name[i] = '_';
	for(unsigned int i = 0; i < sizeof(s_java_exports) / sizeof(s_java_exports[0]); i++)
	{
		if (s_java_exports[i] . signature == nil)
		{
			CoderWriteLine(p_coder, "extern \"C\" JNIEXPORT %s JNICALL Java_%s_LC__1_1%s(JNIEnv *env, jobject obj) __attribute((visibility(\"default\")));", s_java_exports[i] . result, t_java_class_name, s_java_exports[i] . name);
			CoderWriteLine(p_coder, "JNIEXPORT %s JNICALL Java_%s_LC__1_1%s(JNIEnv *env, jobject obj)", s_java_exports[i] . result, t_java_class_name, s_java_exports[i] . name);
			CoderWriteLine(p_coder, "{");
			if (strcmp(s_java_exports[i] . result, "void") == 0)
				CoderWriteLine(p_coder, "  java_lcapi_%s(env);", s_java_exports[i] . name);
			else
				CoderWriteLine(p_coder, "  return java_lcapi_%s(env);", s_java_exports[i] . name);
		}
		else
		{
			CoderWriteLine(p_coder, "extern \"C\" JNIEXPORT %s JNICALL Java_%s_LC__1_1%s(JNIEnv *env, jobject obj, %s) __attribute((visibility(\"default\")));", s_java_exports[i] . result, t_java_class_name, s_java_exports[i] . name, s_java_exports[i] . signature);
			CoderWriteLine(p_coder, "JNIEXPORT %s JNICALL Java_%s_LC__1_1%s(JNIEnv *env, jobject obj, %s)", s_java_exports[i] . result, t_java_class_name, s_java_exports[i] . name, s_java_exports[i] . signature);
			CoderWriteLine(p_coder, "{");
			if (strcmp(s_java_exports[i] . result, "void") == 0)
				CoderWriteLine(p_coder, "  java_lcapi_%s(env, %s);", s_java_exports[i] . name, s_java_exports[i] . args);
			else
				CoderWriteLine(p_coder, "  return java_lcapi_%s(env, %s);", s_java_exports[i] . name, s_java_exports[i] . args);
		}
		CoderWriteLine(p_coder, "}");
	}
	free(t_java_class_name);

	CoderWriteLine(p_coder, "#endif");
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" const char *g_java_support_template[];

/*static struct { const char *tag; void (*generator)(InterfaceRef self, CoderRef coder); } s_sections[] =
{
	{ "package", InterfaceGenerateJavaPackageSection },
};*/

static bool InterfaceGenerateJava(InterfaceRef self, CoderRef p_coder)
{
	CoderWriteLine(p_coder, "package %s;", NameGetCString(self -> qualified_name));
	
	for(unsigned int i = 1; g_java_support_template[i] != nil; i++)
		CoderWrite(p_coder, "%s", g_java_support_template[i]);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool InterfaceGenerate(InterfaceRef self, const char *p_output)
{
	bool t_success;
	t_success = true;
	
	CoderRef t_coder;
	t_coder = nil;
	
	char *t_native_output_filename, *t_java_output_filename;
	if (MCCStringEndsWith(p_output, "/"))
	{
		MCCStringFormat(t_native_output_filename, "%s%s.lcidl.cpp", p_output, NameGetCString(self -> name));
		MCCStringFormat(t_java_output_filename, "%sLC.java", p_output);
	}
	else
	{
		t_native_output_filename = (char *)p_output;
		t_java_output_filename = nil;
	}

	if (t_success)
		t_success = !self -> invalid;
	
	if (t_success)
		t_success = CoderStart(t_native_output_filename, t_coder);
	
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
	else if (t_coder != nil)
		CoderCancel(t_coder);
	
	if (t_success && t_java_output_filename != nil)
	{
        t_coder = nil;
        
		if (t_success)
			t_success = CoderStart(t_java_output_filename, t_coder);
		
		if (t_success)
			t_success = InterfaceGenerateJava(self, t_coder);
		
		if (t_success)
			t_success = CoderFinish(t_coder);
		else if (t_coder != nil)
			CoderCancel(t_coder);
	}
	
	if (t_native_output_filename != p_output)
		free(t_native_output_filename);
	
	free(t_java_output_filename);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
