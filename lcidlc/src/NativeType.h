//
//  NativeType.h
//  lcidlc
//
//  Created by Monte Goulding on 13/06/13.
//
//

#include <stdio.h>
#ifndef __VALUE__
#include "Value.h"
#endif

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

NativeType NativeTypeFromName(NameRef p_type);

const char *NativeTypeGetTypedef(NativeType p_type);

const char *NativeTypeGetSecondaryPrefix(NativeType p_type);

const char *NativeTypeGetTag(NativeType p_type);

const char *NativeTypeGetInitializer(NativeType p_type);

const char *native_type_to_java_type_cstring(NativeType p_type);

const char *native_type_to_java_sig(NativeType p_type);

const char *native_type_to_java_method_type_cstring(NativeType p_type);

const char *native_type_to_type_in_cstring(NativeType p_type);

const char *native_type_to_type_out_cstring(NativeType p_type);