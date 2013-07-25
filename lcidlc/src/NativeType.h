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

#ifndef __NATIVE_TYPE__
#define __NATIVE_TYPE__

#ifndef __VALUE__
#include "Value.h"
#endif

enum NativeType
{
	kNativeTypeNone,
	kNativeTypeBoolean,
	kNativeTypeInteger,
	kNativeTypeReal,
	kNativeTypeCString,
	kNativeTypeCData,
	kNativeTypeCArray,
	kNativeTypeCDictionary,
	kNativeTypeObjcString,
	kNativeTypeObjcNumber,
	kNativeTypeObjcData,
	kNativeTypeObjcArray,
	kNativeTypeObjcDictionary,
	kNativeTypeJavaString,
	kNativeTypeJavaNumber,
	kNativeTypeJavaData,
	kNativeTypeJavaArray,
	kNativeTypeJavaDictionary,
	
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

#endif
