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

#include "foundation.h"

#include "NativeType.h"

////////////////////////////////////////////////////////////////////////////////

NativeType NativeTypeFromName(NameRef p_type)
{
	if (NameEqualToCString(p_type, "boolean"))
		return kNativeTypeBoolean;
	else if (NameEqualToCString(p_type, "integer"))
		return kNativeTypeInteger;
	else if (NameEqualToCString(p_type, "real"))
		return kNativeTypeReal;
	else if (NameEqualToCString(p_type, "c-string"))
		return kNativeTypeCString;
	else if (NameEqualToCString(p_type, "c-data"))
		return kNativeTypeCData;
	else if (NameEqualToCString(p_type, "lc-array"))
		return kNativeTypeLCArray;
	else if (NameEqualToCString(p_type, "utf8-c-string"))
		return kNativeTypeUTF8CString;
	else if (NameEqualToCString(p_type, "utf8-c-data"))
		return kNativeTypeUTF8CData;
	else if (NameEqualToCString(p_type, "utf16-c-string"))
		return kNativeTypeUTF16CString;
	else if (NameEqualToCString(p_type, "utf16-c-data"))
		return kNativeTypeUTF16CData;
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
        case kNativeTypeInteger:
            return "int";
        case kNativeTypeReal:
            return "double";
        case kNativeTypeEnum:
            return "int";
        case kNativeTypeCString:
        case kNativeTypeUTF8CString:
        case kNativeTypeUTF16CString:
            return "char*";
        case kNativeTypeCData:
        case kNativeTypeUTF8CData:
        case kNativeTypeUTF16CData:
            return "LCBytes";
        case kNativeTypeLCArray:
            return "LCArrayRef";
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
		case kNativeTypeJavaString:
		case kNativeTypeJavaUTF8String:
		case kNativeTypeJavaUTF16String:
			return "jstring";
		case kNativeTypeJavaNumber:
			return "jobject";
		case kNativeTypeJavaData:
		case kNativeTypeJavaUTF8Data:
		case kNativeTypeJavaUTF16Data:
			return "jobject";
		case kNativeTypeJavaArray:
			return "jobject";
		case kNativeTypeJavaDictionary:
			return "jobject";
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
        case kNativeTypeUTF8CString:
        case kNativeTypeUTF16CString:
        case kNativeTypeObjcString:
        case kNativeTypeObjcNumber:
        case kNativeTypeObjcData:
        case kNativeTypeObjcArray:
        case kNativeTypeObjcDictionary:
            return "*";
		default:
			return "";
	}
}

const char *NativeTypeGetTag(NativeType p_type)
{
	switch(p_type)
	{
        case kNativeTypeBoolean:
            return "bool";
        case kNativeTypeCString:
            return "cstring";
        case kNativeTypeUTF8CString:
            return "utf8cstring";
        case kNativeTypeUTF16CString:
            return "utf16cstring";
        case kNativeTypeCData:
            return "cdata";
        case kNativeTypeUTF8CData:
            return "utf8cdata";
        case kNativeTypeUTF16CData:
            return "utf16cdata";
        case kNativeTypeLCArray:
            return "lc_array";
        case kNativeTypeInteger:
            return "int";
        case kNativeTypeReal:
            return "double";
        case kNativeTypeEnum:
            return "int";
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
		case kNativeTypeJavaString:
			return "java_cstring";
		case kNativeTypeJavaNumber:
			return "java_number";
		case kNativeTypeJavaData:
			return "java_cdata";
		case kNativeTypeJavaArray:
			return "java_array";
		case kNativeTypeJavaDictionary:
			return "java_dictionary";
        // SN-2014-07-17: [[ ExternalsApiV6 ]] New getter for unicode strings
		case kNativeTypeJavaUTF8String:
			return "java_utf8cstring";
		case kNativeTypeJavaUTF16String:
			return "java_utf16cstring";
		case kNativeTypeJavaUTF8Data:
			return "java_utf8cdata";
		case kNativeTypeJavaUTF16Data:
			return "java_utf16cdata";
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
        case kNativeTypeUTF8CString:
        case kNativeTypeUTF16CString:
        case kNativeTypeLCArray:
        case kNativeTypeObjcString:
        case kNativeTypeObjcNumber:
        case kNativeTypeObjcData:
        case kNativeTypeObjcArray:
        case kNativeTypeObjcDictionary:
		case kNativeTypeJavaString:
		case kNativeTypeJavaUTF8String:
		case kNativeTypeJavaUTF16String:
		case kNativeTypeJavaNumber:
		case kNativeTypeJavaData:
		case kNativeTypeJavaUTF8Data:
		case kNativeTypeJavaUTF16Data:
		case kNativeTypeJavaArray:
		case kNativeTypeJavaDictionary:
            return "nil";
        case kNativeTypeCData:
        case kNativeTypeUTF8CData:
        case kNativeTypeUTF16CData:
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
        case kNativeTypeUTF8CString:
        case kNativeTypeUTF16CString:
			return "jobject";
		case kNativeTypeCData:
        case kNativeTypeUTF8CData:
        case kNativeTypeUTF16CData:
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
		case kNativeTypeJavaString:
		case kNativeTypeCString:
        case kNativeTypeUTF8CString:
        case kNativeTypeUTF16CString:
			return "Ljava/lang/String;";
		case kNativeTypeJavaData:
		case kNativeTypeCData:
        case kNativeTypeUTF8CData:
        case kNativeTypeUTF16CData:
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
        case kNativeTypeUTF8CString:
        case kNativeTypeUTF16CString:
			return "Object";
		case kNativeTypeCData:
        case kNativeTypeUTF8CData:
        case kNativeTypeUTF16CData:
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
		case kNativeTypeUTF8CString:
		case kNativeTypeUTF16CString:
			return "const char *";
		case kNativeTypeCData:
		case kNativeTypeUTF8CData:
		case kNativeTypeUTF16CData:
			return "LCBytes";
		case kNativeTypeLCArray:
			return "const LCArrayRef";
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
		case kNativeTypeUTF8CString:
		case kNativeTypeUTF16CString:
			return "char *";
		case kNativeTypeCData:
		case kNativeTypeUTF8CData:
		case kNativeTypeUTF16CData:
			return "LCBytes";
		case kNativeTypeLCArray:
			return "LCArrayRef";
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
