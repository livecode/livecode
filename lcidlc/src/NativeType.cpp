/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include <core.h>

#include "NativeType.h"

////////////////////////////////////////////////////////////////////////////////

NativeType NativeTypeFromName(NameRef p_type)
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

const char *NativeTypeGetTypedef(NativeType p_type)
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

const char *NativeTypeGetSecondaryPrefix(NativeType p_type)
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

const char *NativeTypeGetTag(NativeType p_type)
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

const char *NativeTypeGetInitializer(NativeType p_type)
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

const char *native_type_to_java_type_cstring(NativeType p_type)
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
		case kNativeTypeEnum:
			return "int";
		case kNativeTypeReal:
			return "double";
		default:
			break;
	}
	return "not_supported_type";
}

const char *native_type_to_java_sig(NativeType p_type)
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
		case kNativeTypeEnum:
			return "I";
		case kNativeTypeReal:
			return "D";
		default:
			break;
	}
	return "not_supported_type";
}

const char *native_type_to_java_method_type_cstring(NativeType p_type)
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
		case kNativeTypeEnum:
			return "Int";
		case kNativeTypeReal:
			return "Double";
		default:
			break;
	}
	return "not_supported_type";
}

const char *native_type_to_type_in_cstring(NativeType p_type)
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
		case kNativeTypeEnum:
			return "int";
		case kNativeTypeReal:
			return "double";
		default:
			break;
	}
	return "not_supported_type";
}

const char *native_type_to_type_out_cstring(NativeType p_type)
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
		case kNativeTypeEnum:
			return "int";
		case kNativeTypeReal:
			return "double";
		default:
			break;
	}
	return "not_supported_type";
}
