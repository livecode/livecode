//
//  NativeType.cpp
//  lcidlc
//
//  Created by Monte Goulding on 13/06/13.
//
//

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
			return "int";
		case kNativeTypeReal:
			return "double";
		default:
			break;
	}
	return "not_supported_type";
}
