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

#ifndef __MBLANDROIDJAVA_H__
#define __MBLANDROIDJAVA_H__


#include <jni.h>


typedef enum
{
    kMCJavaTypeUnknown,
    kMCJavaTypeVoid,
    
    // basic types
    kMCJavaTypeBoolean,
    kMCJavaTypeByte,
    kMCJavaTypeChar,
    kMCJavaTypeShort,
    kMCJavaTypeInt,
    kMCJavaTypeLong,
    kMCJavaTypeFloat,
    kMCJavaTypeDouble,
    
    kMCJavaTypeObject,
    
    // specialized return types
    kMCJavaTypeCString,
    kMCJavaTypeUtf8CString,
    kMCJavaTypeMCString,
    kMCJavaTypeMCStringUnicode,
	kMCJavaTypeMCStringRef,
    kMCJavaTypeByteArray,
    kMCJavaTypeList,
    kMCJavaTypeMap, // MM-2012-02-22: Added ability to create Java maps
    
    // special retuyrn types used by MCValueRef
    kMCJavaTypeObjectArray,
    kMCJavaTypeBooleanRef,
    kMCJavaTypeNumberRef,
    kMCJavaTypeMCValueRef,
} MCJavaType;

typedef struct
{
    char *signature;
    
    MCJavaType return_type;
    
    jvalue *params;
    bool *delete_param;
    uint32_t param_count;
    
} MCJavaMethodParams;

JNIEnv *MCJavaGetThreadEnv();
JNIEnv *MCJavaAttachCurrentThread();
void MCJavaDetachCurrentThread();

bool MCJavaStringFromNative(JNIEnv *env, const MCString *p_string, jstring &r_java_string);
bool MCJavaStringFromUnicode(JNIEnv *env, const MCString *p_string, jstring &r_java_string);
bool MCJavaStringToUnicode(JNIEnv *env, jstring p_java_string, unichar_t *&r_unicode, uint32_t &r_length);
bool MCJavaStringToNative(JNIEnv *env, jstring p_java_string, char *&r_native);
bool MCJavaStringToUTF8(JNIEnv *env, jstring p_java_string, char *&r_utf_8);
bool MCJavaStringToStringRef(JNIEnv *env, jstring p_java_string, MCStringRef& r_stringref);
bool MCJavaStringFromStringRef(JNIEnv *env, MCStringRef p_string, jstring &r_java_string);
bool MCJavaByteArrayFromDataRef(JNIEnv *env, MCDataRef p_data, jbyteArray &r_byte_array);
bool MCJavaByteArrayToDataRef(JNIEnv *env, jbyteArray p_byte_array, MCDataRef& r_data);

// PM-2015-02-18: [[ Bug 14489 ]] Create/Delete global refs for statics
bool MCJavaInitialize(JNIEnv *env);
void MCJavaFinalize(JNIEnv *env);

bool MCJavaInitList(JNIEnv *env, jobject&);
bool MCJavaFreeList(JNIEnv *env, jobject);
bool MCJavaListAppendObject(JNIEnv *env, jobject, jobject);
bool MCJavaListAppendStringRef(JNIEnv *env, jobject p_list, MCStringRef p_string);
bool MCJavaListAppendInt(JNIEnv *env, jobject p_list, jint p_int);

bool MCJavaInitMap(JNIEnv *env, jobject &r_map);
bool MCJavaFreeMap(JNIEnv *env, jobject p_map);
bool MCJavaMapPutObjectToObject(JNIEnv *env, jobject p_map, jobject p_key, jobject p_value);
bool MCJavaMapPutStringToObject(JNIEnv *env, jobject p_map, MCStringRef p_key, jobject p_value);
bool MCJavaMapPutStringToString(JNIEnv *env, jobject p_map, MCStringRef p_key, MCStringRef p_value);
bool MCJavaMapFromArrayRef(JNIEnv *p_env, MCArrayRef p_array, jobject &r_object);
bool MCJavaMapToArrayRef(JNIEnv *p_env, jobject p_map, MCArrayRef &r_array);

typedef bool (*MCJavaMapCallback)(JNIEnv *env, MCNameRef p_key, jobject p_value, void *p_context);
bool MCJavaIterateMap(JNIEnv *env, jobject p_map, MCJavaMapCallback p_callback, void *p_context);

bool MCJavaObjectGetField(JNIEnv *env, jobject p_object, const char *p_fieldname, MCJavaType p_fieldtype, void *r_value);

bool MCJavaConvertParameters(JNIEnv *env, const char *p_signature, va_list p_args, MCJavaMethodParams *&r_params, bool p_global_refs = false);
void MCJavaMethodParamsFree(JNIEnv *env, MCJavaMethodParams *p_params, bool p_global_refs);

void MCJavaColorToComponents(uint32_t p_color, uint16_t &r_red, uint16_t &r_green, uint16_t &r_blue, uint16_t &r_alpha);

bool MCJavaBooleanFromBooleanRef(JNIEnv *p_env, MCBooleanRef p_value, jobject& r_object);
bool MCJavaBooleanToBooleanRef(JNIEnv *p_env, jobject p_object, MCBooleanRef& r_value);
bool MCJavaNumberFromNumberRef(JNIEnv *p_env, MCNumberRef p_value, jobject& r_object);
bool MCJavaNumberToNumberRef(JNIEnv *p_env, jobject p_object, MCNumberRef& r_value);
bool MCJavaArrayFromArrayRef(JNIEnv *p_env, MCArrayRef p_value, jobjectArray& r_object);
bool MCJavaArrayToArrayRef(JNIEnv *p_env, jobjectArray p_object, MCArrayRef& r_value);
bool MCJavaObjectFromValueRef(JNIEnv *p_env, MCValueRef p_value, jobject& r_object);
bool MCJavaObjectToValueRef(JNIEnv *p_env, jobject p_object, MCValueRef &r_value);

#endif //__MBLANDROIDJAVA_H__
