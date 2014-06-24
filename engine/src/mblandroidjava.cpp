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

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"

#include "execpt.h"

#include "mblandroidjava.h"

#include <jni.h>


////////////////////////////////////////////////////////////////////////////////

static jclass s_integer_class = nil;
static jmethodID s_integer_constructor = nil;

static bool init_integer_class(JNIEnv *env)
{
    if (s_integer_class == nil)
        s_integer_class = env->FindClass("java/lang/Integer");
    if (s_integer_class == nil)
		return false;

	if (s_integer_constructor == nil)
		s_integer_constructor = env->GetMethodID(s_integer_class, "<init>", "(I)V");
	if (s_integer_constructor == nil)
		return false;
	
	return true;
}

static jclass s_string_class = nil;

static bool init_string_class(JNIEnv *env)
{
	if (s_string_class == nil)
		s_string_class = env->FindClass("java/lang/String");
	if (s_string_class == nil)
		return false;

	return true;
}

static jclass s_array_list_class = nil;
static jmethodID s_array_list_constructor = nil;
static jmethodID s_array_list_append = nil;

bool init_arraylist_class(JNIEnv *env)
{
    if (s_array_list_class == nil)
        s_array_list_class = env->FindClass("java/util/ArrayList");
    if (s_array_list_class == nil)
		return false;

	if (s_array_list_constructor == nil)
		s_array_list_constructor = env->GetMethodID(s_array_list_class, "<init>", "()V");
	if (s_array_list_constructor == nil)
		return false;

	if (s_array_list_append == nil)
		s_array_list_append = env->GetMethodID(s_array_list_class, "add", "(Ljava/lang/Object;)Z");
	if (s_array_list_append == nil)
		return false;

	return true;
}

static jclass s_hash_map_class = nil;
static jmethodID s_hash_map_constructor = nil;
static jmethodID s_hash_map_put = nil;
static jmethodID s_hash_map_entry_set = nil;

static jclass s_map_entry_class = nil;
static jmethodID s_map_entry_get_key = nil;
static jmethodID s_map_entry_get_value = nil;

static bool init_hashmap_class(JNIEnv *env)
{
	if (s_hash_map_class == nil)
		s_hash_map_class = env->FindClass("java/util/HashMap");
	if (s_hash_map_class == nil)
		return false;
	
	if (s_hash_map_constructor == nil)
		s_hash_map_constructor = env->GetMethodID(s_hash_map_class, "<init>", "()V");
	if (s_hash_map_constructor == nil)
		return false;
	
	if (s_hash_map_put == nil)
		s_hash_map_put = env->GetMethodID(s_hash_map_class, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	if (s_hash_map_put == nil)
		return false;
	
	if (s_hash_map_entry_set == nil)
		s_hash_map_entry_set = env->GetMethodID(s_hash_map_class, "entrySet", "()Ljava/util/Set;");
	if (s_hash_map_entry_set == nil)
		return false;
	
	
	if (s_map_entry_class == nil)
		s_map_entry_class = env->FindClass("java/util/Map$Entry");
	if (s_map_entry_class == nil)
		return false;
	
	if (s_map_entry_get_key == nil)
		s_map_entry_get_key = env->GetMethodID(s_map_entry_class, "getKey", "()Ljava/lang/Object;");
	if (s_map_entry_get_key == nil)
		return false;
	
	if (s_map_entry_get_value == nil)
		s_map_entry_get_value = env->GetMethodID(s_map_entry_class, "getValue", "()Ljava/lang/Object;");
	if (s_map_entry_get_value == nil)
		return false;
	
	return true;
}

static jclass s_iterator_class = nil;
static jmethodID s_iterator_has_next = nil;
static jmethodID s_iterator_next = nil;

static bool init_iterator_class(JNIEnv *env)
{
	if (s_iterator_class == nil)
		s_iterator_class = env->FindClass("java/util/Iterator");
	if (s_iterator_class == nil)
		return false;
	
	if (s_iterator_has_next == nil)
		s_iterator_has_next = env->GetMethodID(s_iterator_class, "hasNext", "()Z");
	if (s_iterator_has_next == nil)
		return false;
	
	if (s_iterator_next == nil)
		s_iterator_next = env->GetMethodID(s_iterator_class, "next", "()Ljava/lang/Object;");
	if (s_iterator_next == nil)
		return false;

	return true;
}

static jclass s_set_class = nil;
static jmethodID s_set_iterator = nil;

static bool init_set_class(JNIEnv *env)
{
	if (s_set_class == nil)
		s_set_class = env->FindClass("java/util/Set");
	if (s_set_class == nil)
		return false;
	
	if (s_set_iterator == nil)
		s_set_iterator = env->GetMethodID(s_set_class, "iterator", "()Ljava/util/Iterator;");
	if (s_set_iterator == nil)
		return false;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

uint32_t NativeToUnicode(const char *p_string, uint32_t p_string_length, unichar_t *p_buffer, uint32_t p_buffer_length);
uint32_t UnicodeToNative(const unichar_t *p_string, uint32_t p_string_length, char *p_buffer, uint32_t p_buffer_length);

static bool native_to_unicode(const char *p_native, uint32_t p_length, unichar_t *&r_unicode)
{
    if (p_native == nil)
    {
        r_unicode = nil;
        return true;
    }
   
    bool t_success = true;
    
    unichar_t *t_unicode = nil;
    
    t_success = MCMemoryAllocate(2 * p_length, t_unicode);
    
    if (t_success)
        t_success = p_length == NativeToUnicode(p_native, p_length, t_unicode, p_length * 2);
    
    if (t_success)
        r_unicode = t_unicode;
    else
        MCMemoryDeallocate(t_unicode);
    
    return t_success;
}

static bool unicode_to_native(const unichar_t *p_unicode, uint32_t p_length, char *&r_native)
{
    if (p_unicode == nil)
    {
        r_native = nil;
        return true;
    }
    
    bool t_success = true;
    
    char *t_native = nil;
    
    t_success = MCMemoryAllocate(p_length + 1, t_native);
    
    if (t_success)
        t_success = p_length == UnicodeToNative(p_unicode, p_length, t_native, p_length);
    
    if (t_success)
    {
        t_native[p_length] = '\0';
        r_native = t_native;
    }
    else
        MCCStringFree(t_native);
    
    return t_success;
}

bool MCJavaStringFromNative(JNIEnv *env, const MCString *p_string, jstring &r_java_string)
{
    if (p_string == nil)
    {
        r_java_string = nil;
        return true;
    }
    
    bool t_success = true;
    
    jstring t_java_string = nil;
    unichar_t *t_unicode = nil;
    
    t_success = native_to_unicode(p_string->getstring(), p_string->getlength(), t_unicode);
    if (t_success)
        t_success = nil != (t_java_string = env -> NewString((jchar*)t_unicode, p_string->getlength()));
    
    MCMemoryDeallocate(t_unicode);

    if (t_success)
        r_java_string = t_java_string;
    
    return t_success;
}

bool MCJavaStringFromNative(JNIEnv *env, const char *p_string, jstring &r_java_string)
{
    if (p_string == nil)
    {
        r_java_string = nil;
        return true;
    }
    else
    {
        MCString t_mcstring(p_string);
        return MCJavaStringFromNative(env, &t_mcstring, r_java_string);
    }
}

bool MCJavaStringFromUnicode(JNIEnv *env, const MCString *p_string, jstring &r_java_string)
{
    if (p_string == nil)
    {
        r_java_string = nil;
        return true;
    }
    
    bool t_success = true;
    jstring t_java_string = nil;
    
    t_success = nil != (t_java_string = env -> NewString((const jchar*)p_string->getstring(), p_string->getlength() / 2));
    
    if (t_success)
        r_java_string = t_java_string;
    
    return t_success;
}

bool MCJavaStringToUnicode(JNIEnv *env, jstring p_java_string, unichar_t *&r_unicode, uint32_t &r_length)
{
    bool t_success = true;
    
    const jchar *t_unicode_string = nil;
    int t_unicode_length = 0;
    unichar_t *t_copy = nil;
    
    if (p_java_string != nil)
        t_unicode_string = env -> GetStringChars(p_java_string, NULL);
    
    if (t_unicode_string != nil)
    {
        t_unicode_length = env -> GetStringLength(p_java_string);
        t_success = MCMemoryAllocateCopy(t_unicode_string, t_unicode_length * 2, (void*&)t_copy);
        env -> ReleaseStringChars(p_java_string, t_unicode_string);
    }
    
    if (t_success)
    {
        r_unicode = t_copy;
        r_length = t_unicode_length;
    }
    
    return t_success;
}

bool MCJavaStringToNative(JNIEnv *env, jstring p_java_string, char *&r_native)
{
    bool t_success = true;
    
    const jchar *t_unicode_string = nil;
    uint32_t t_unicode_length = 0;
    
    char *t_native = nil;
    uint32_t t_length = 0;
    
    if (p_java_string != nil)
        t_unicode_string = env -> GetStringChars(p_java_string, NULL);
    
    if (t_unicode_string != nil)
    {
        t_unicode_length = env -> GetStringLength(p_java_string);
        
        if (t_success)
            t_success = unicode_to_native(t_unicode_string, t_unicode_length, t_native);
        
        env -> ReleaseStringChars(p_java_string, t_unicode_string);
    }
    
    if (t_success)
        r_native = t_native;
    
    return t_success;
}

bool MCJavaByteArrayFromData(JNIEnv *env, const MCString *p_data, jbyteArray &r_byte_array)
{
    if (p_data == nil || p_data->getlength() == 0)
    {
        r_byte_array = nil;
        return true;
    }
    
    bool t_success = true;
    jbyteArray t_bytes = nil;
    
    t_success = nil != (t_bytes = env -> NewByteArray(p_data->getlength()));
    
    if (t_success)
    {
        env -> SetByteArrayRegion(t_bytes, 0, p_data->getlength(), (const jbyte*)p_data->getstring());
    }
    
    if (t_success)
        r_byte_array = t_bytes;
    
    return t_success;
}

bool MCJavaByteArrayToData(JNIEnv *env, jbyteArray p_byte_array, void *&r_data, uint32_t &r_length)
{
    bool t_success = true;
    
    jbyte *t_bytes = nil;
    void *t_data = nil;
    uint32_t t_length = 0;
    
    if (p_byte_array != nil)
        t_bytes = env -> GetByteArrayElements(p_byte_array, nil);
    
    if (t_bytes != nil)
    {
        t_length = env -> GetArrayLength(p_byte_array);
        t_success = MCMemoryAllocateCopy(t_bytes, t_length, t_data);
        
        env -> ReleaseByteArrayElements(p_byte_array, t_bytes, 0);
    }
    
    if (t_success)
    {
        r_data = t_data;
        r_length = t_length;
    }
    
    return t_success;
}

bool MCJavaIntegerFromInt(JNIEnv *env, jint p_int, jobject &r_integer)
{
    bool t_success = true;
    
	if (!init_integer_class(env))
		return false;
    
    jobject t_integer = nil;
    t_integer = env->NewObject(s_integer_class, s_integer_constructor, p_int);
    t_success = nil != t_integer;
    
    if (t_success)
        r_integer = t_integer;
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaInitList(JNIEnv *env, jobject &r_list)
{
    bool t_success = true;
    
	if (!init_arraylist_class(env))
		return false;
	
    jobject t_list = nil;
    t_list = env->NewObject(s_array_list_class, s_array_list_constructor);
    t_success = nil != t_list;
    
    if (t_success)
        r_list = t_list;
    
    return t_success;
}

bool MCJavaFreeList(JNIEnv *env, jobject p_list)
{
    env->DeleteLocalRef(p_list);
    return true;
}

bool MCJavaListAppendObject(JNIEnv *env, jobject p_list, jobject p_object)
{
    // MM-2012-06-01: [[ 10211 ]] List.Add returns a boolean.
    env->CallBooleanMethod(p_list, s_array_list_append, p_object);
    // TODO: check for exceptions
    return true;
}

////////

bool MCJavaListAppendString(JNIEnv *env, jobject p_list, const MCString *p_string)
{
    bool t_success = true;
    jstring t_jstring = nil;
    
    t_success = MCJavaStringFromNative(env, p_string, t_jstring);
    
    if (t_success)
        t_success = MCJavaListAppendObject(env, p_list, t_jstring);
    
    if (t_jstring != nil)
        env->DeleteLocalRef(t_jstring);
    
    return t_success;
}

bool MCJavaListAppendInt(JNIEnv *env, jobject p_list, jint p_int)
{
    bool t_success = true;
    jobject t_integer = nil;
    
    t_success = MCJavaIntegerFromInt(env, p_int, t_integer);
    
    if (t_success)
        t_success = MCJavaListAppendObject(env, p_list, t_integer);
    
    if (t_integer != nil)
        env->DeleteLocalRef(t_integer);
    
   return t_success;
}

////////////////////////////////////////////////////////////////////////////////
// MM-2012-02-22: Added ability to create Java maps


bool MCJavaInitMap(JNIEnv *env, jobject &r_map)
{
	if (!init_hashmap_class(env))
		return false;
    
    jobject t_map = nil;
    t_map = env->NewObject(s_hash_map_class, s_hash_map_constructor);
    if (nil == t_map)
		return false;
    
	r_map = t_map;
    
    return true;
}

bool MCJavaFreeMap(JNIEnv *env, jobject p_map)
{
    env->DeleteLocalRef(p_map);
    return true;
}

bool MCJavaMapPutObjectToObject(JNIEnv *env, jobject p_map, jobject p_key, jobject p_value)
{
    jstring t_res = (jstring) env->CallObjectMethod(p_map, s_hash_map_put, p_key, p_value);
    // TODO: check for exceptions
    return true;
}

bool MCJavaMapPutStringToObject(JNIEnv *env, jobject p_map, const char *p_key, jobject p_value)
{
	bool t_success;
	t_success = true;
	
	jstring t_key;
	t_key = nil;
	if (t_success)
		t_success = MCJavaStringFromNative(env, p_key, t_key);
	
	if (t_success)
		t_success = MCJavaMapPutObjectToObject(env, p_map, t_key, p_value);
	
	if (t_key != nil)
		env->DeleteLocalRef(t_key);
	
	return t_success;
}

bool MCJavaMapPutStringToString(JNIEnv *env, jobject p_map, const char *p_key, const char *p_value)
{
    bool t_success;
    t_success = true;
    
    jstring t_value;
    t_value = nil;
    if (t_success)
        t_success = MCJavaStringFromNative(env, p_value, t_value);
    
    if (t_success)
        t_success = MCJavaMapPutStringToObject(env, p_map, p_key, t_value);
    
    if (t_value != nil)
        env->DeleteLocalRef(t_value);
    
    return t_success;
}

bool MCJavaIterateMap(JNIEnv *env, jobject p_map, MCJavaMapCallback p_callback, void *p_context)
{
	if (!init_hashmap_class(env) || !init_set_class(env) || !init_iterator_class(env))
		return false;
	
	bool t_success = true;
	jobject t_set = nil, t_iterator = nil;
	// get set of entries from map
	t_success = nil != (t_set = env->CallObjectMethod(p_map, s_hash_map_entry_set));
	
	// get iterator for entry set
	if (t_success)
		t_success = nil != (t_iterator = env->CallObjectMethod(t_set, s_set_iterator));
	
	// iterate over entries
	while (t_success && env->CallBooleanMethod(t_iterator, s_iterator_has_next))
	{
		jobject t_entry = nil;
		jobject t_key = nil, t_value = nil;
		char *t_key_string = nil;
		
		t_success = nil != (t_entry = env->CallObjectMethod(t_iterator, s_iterator_next));
		
		// fetch entry key & value
		if (t_success)
			t_success = nil != (t_key = env->CallObjectMethod(t_entry, s_map_entry_get_key));
		if (t_success)
			t_success = nil != (t_value = env->CallObjectMethod(t_entry, s_map_entry_get_value));
		
		// convert key string to native char*
		if (t_success)
			t_success = MCJavaStringToNative(env, (jstring)t_key, t_key_string);
		
		// call callback
		if (t_success)
			t_success = p_callback(env, t_key_string, t_value, p_context);
		
		if (t_key_string != nil)
			MCCStringFree(t_key_string);
		if (t_key != nil)
			env->DeleteLocalRef(t_key);
		if (t_value != nil)
			env->DeleteLocalRef(t_value);
		if (t_entry != nil)
			env->DeleteLocalRef(t_entry);
	}
	
	if (t_set != nil)
		env->DeleteLocalRef(t_set);
	if (t_iterator != nil)
		env->DeleteLocalRef(t_iterator);
	
	return t_success;
}

bool MCJavaMapFromArray(JNIEnv *p_env, MCExecPoint &p_ep, MCVariableValue *p_array, jobject &r_object)
{
	if (!init_hashmap_class(p_env))
		return false;
	
	bool t_success = true;
	
	MCExecPoint ep(p_ep);
	
	if (!p_array->is_array())
		return false;
	
	MCVariableArray *t_array = p_array->get_array();
	MCHashentry *t_hashentry = nil;
	uindex_t t_index = 0;
	jobject t_map = nil;
	
	t_success = MCJavaInitMap(p_env, t_map);
	while (t_success && nil != (t_hashentry = t_array->getnextelement(t_index, t_hashentry, False, ep)))
	{
		jobject t_jobj = nil;
		if (t_hashentry->value.is_array())
			t_success = MCJavaMapFromArray(p_env, p_ep, &t_hashentry->value, t_jobj);
		else
		{
			t_success = ES_NORMAL == t_hashentry->value.fetch(ep);
			jstring t_jstring = nil;
			if (t_success)
				t_success = MCJavaStringFromNative(p_env, &ep.getsvalue(), t_jstring);
			if (t_success)
				t_jobj = t_jstring;
		}
		if (t_success)
			t_success = MCJavaMapPutStringToObject(p_env, t_map, t_hashentry->string, t_jobj);
		if (t_jobj != nil)
			p_env->DeleteLocalRef(t_jobj);
	}
	if (t_success)
		r_object = t_map;
	else if (t_map != nil)
		MCJavaFreeMap(p_env, t_map);
	
	return t_success;
}

typedef struct
{
	MCVariableValue *array;
	MCExecPoint *ep;
} map_to_array_context_t;

static bool s_map_to_array_callback(JNIEnv *p_env, const char *p_key, jobject p_value, void *p_context)
{
	bool t_success = true;
	
	map_to_array_context_t *t_context = (map_to_array_context_t*)p_context;
	
	MCVariableValue *t_array_entry = nil;
	
	t_success = t_context->array->lookup_element(*(t_context->ep), MCString(p_key), t_array_entry) == ES_NORMAL;
	
	if (t_success)
	{
		if (p_env->IsInstanceOf(p_value, s_string_class))
		{
			char *t_string_value = nil;
			t_success = MCJavaStringToNative(p_env, (jstring)p_value, t_string_value);
			if (t_success)
				t_success = t_array_entry->assign_string(MCString(t_string_value));
			if (t_string_value != nil)
				MCCStringFree(t_string_value);
		}
		else if (p_env->IsInstanceOf(p_value, s_hash_map_class))
		{
			map_to_array_context_t t_new_context;
			t_new_context.array = t_array_entry;
			t_new_context.ep = t_context->ep;
			t_success = MCJavaIterateMap(p_env, p_value, s_map_to_array_callback, &t_new_context);
		}
		else
			t_success = false;
	}
	
	return t_success;
}

bool MCJavaMapToArray(JNIEnv *p_env, MCExecPoint &p_ep, jobject p_map, MCVariableValue *&r_array)
{
	if (!init_string_class(p_env) || !init_hashmap_class(p_env))
		return false;
	
	bool t_success = true;
	
	MCVariableValue *t_array = nil;
	t_success = nil != (t_array = new MCVariableValue());
	
	map_to_array_context_t t_context;
	t_context.array = t_array;
	t_context.ep = &p_ep;
	
	if (t_success)
		t_success = MCJavaIterateMap(p_env, p_map, s_map_to_array_callback, &t_context);
	
	if (t_success)
		r_array = t_array;
	else
		delete t_array;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static MCJavaType native_sigchar_to_returntype(char p_sigchar)
{
    switch (p_sigchar)
    {
        case 'v':
            return kMCJavaTypeVoid;
        case 's':
            return kMCJavaTypeCString;
        case 'S':
            return kMCJavaTypeMCString;
        case 'U':
            return kMCJavaTypeMCStringUnicode;
        case 'd':
            return kMCJavaTypeByteArray;
        case 'i':
            return kMCJavaTypeInt;
        case 'j':
            return kMCJavaTypeLong;
        case 'b':
            return kMCJavaTypeBoolean;
        case 'l':
            return kMCJavaTypeList;
        // MM-2012-02-22: Added ability to create Java maps
        case 'm':
            return kMCJavaTypeMap;
        case 'o':
            return kMCJavaTypeObject;
		case 'f':
			return kMCJavaTypeFloat;
		case 'r':
			return kMCJavaTypeDouble; 
    }
    
    return kMCJavaTypeUnknown;
}

static const char *return_type_to_java_sig(MCJavaType p_type)
{
    switch (p_type)
    {
        case kMCJavaTypeVoid:  // void
            return "V";
        case kMCJavaTypeInt:  // integer (32bit signed)
            return "I";
        case kMCJavaTypeLong:  // long int (64bit signed)
            return "J";
        case kMCJavaTypeBoolean:  // boolean
            return "Z";
        case kMCJavaTypeCString:  // string from char *
        case kMCJavaTypeMCString:  // string from MCString *
        case kMCJavaTypeMCStringUnicode:  // string from utf16 MCString *
            return "Ljava/lang/String;";
        case kMCJavaTypeByteArray:  // binary data from MCString * as byte[]
            return "[B";
        case kMCJavaTypeList:  // List object
            return "Ljava/util/List;";
        // MM-2012-02-22: Added ability to create Java maps
        case kMCJavaTypeMap:  // Map object
            return "Ljava/util/Map;";
        case kMCJavaTypeObject:  // java Object
            return "Ljava/lang/Object;";
		case kMCJavaTypeFloat:
			return "F";
		case kMCJavaTypeDouble:
			return "D";
        default:
            break;
    }
    
    return nil;
}
static const char *native_sigchar_to_javasig(char p_sigchar)
{
    return return_type_to_java_sig(native_sigchar_to_returntype(p_sigchar));
}

static bool build_java_signature(const char *p_signature, char*& r_signature, uint32_t& r_param_count)
{
	uint32_t t_param_count;
	t_param_count = 0;
    
	const char *t_return_type;
	t_return_type = native_sigchar_to_javasig(*p_signature++);
    
	char *t_jsig;
	MCCStringClone("(", t_jsig);
	while(*p_signature != '\0')
	{
		MCCStringAppend(t_jsig, native_sigchar_to_javasig(*p_signature));
		t_param_count += 1;
		p_signature += 1;
	}
    
	MCCStringAppendFormat(t_jsig, ")%s", t_return_type);
    
	r_signature = t_jsig;
	r_param_count = t_param_count;
    
	return true;
}

void MCJavaMethodParamsFree(JNIEnv *env, MCJavaMethodParams *p_params, bool p_global_refs)
{
    if (p_params != nil)
    {
        MCCStringFree(p_params->signature);
        
        for (uint32_t i = 0; i < p_params->param_count; i++)
        {
            if (p_params->delete_param[i] && p_params->params[i].l != nil)
            {
                if (p_global_refs)
                    env->DeleteGlobalRef(p_params->params[i].l);
                else
                    env->DeleteLocalRef(p_params->params[i].l);
            }
        }
        
        MCMemoryDeleteArray(p_params->delete_param);
        MCMemoryDeleteArray(p_params->params);
        
        MCMemoryDelete(p_params);
    }
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaObjectGetField(JNIEnv *env, jobject p_object, const char *p_fieldname, MCJavaType p_fieldtype, void *r_value)
{
    bool t_success = true;
    jfieldID t_field_id = nil;
    
    jclass t_class;
    t_class = env->GetObjectClass(p_object);

    t_field_id = env->GetFieldID(t_class, p_fieldname, return_type_to_java_sig(p_fieldtype));
    t_success = (t_field_id != nil);
    
    if (t_success)
    {
        switch (p_fieldtype)
        {
            case kMCJavaTypeInt:
                *((int*)r_value) = env->GetIntField(p_object, t_field_id);
                break;
                
            case kMCJavaTypeLong:
                *((int64_t*)r_value) = env->GetLongField(p_object, t_field_id);
                break;
                
            case kMCJavaTypeBoolean:
                *((bool*)r_value) = env->GetBooleanField(p_object, t_field_id);
                break;
                
            case kMCJavaTypeFloat:
                *((float*)r_value) = env->GetFloatField(p_object, t_field_id);
                break;
                
            case kMCJavaTypeDouble:
                *((double*)r_value) = env->GetDoubleField(p_object, t_field_id);
                break;
                
            case kMCJavaTypeCString:
            {
                jstring t_jstring = nil;
                char *t_cstring = nil;
                t_jstring = (jstring)env->GetObjectField(p_object, t_field_id);
                t_success = MCJavaStringToNative(env, t_jstring, t_cstring);
                if (t_success)
                    *((char**)r_value) = t_cstring;
                env->DeleteLocalRef(t_jstring);
                break;
            }
                
            case kMCJavaTypeMCString:
            {
                /* UNIMPLEMENTED */
                t_success = false;
                break;
            }
                
            case kMCJavaTypeMCStringUnicode:
            {
                /* UNIMPLEMENTED */
                t_success = false;
                break;
            }
                
            case kMCJavaTypeByteArray:
            {
                /* UNIMPLEMENTED */
                t_success = false;
                break;
            }
                
            case kMCJavaTypeObject:
            case kMCJavaTypeList:
            {
                *((jobject*)r_value) = env->GetObjectField(p_object, t_field_id);
                break;
            }
                
            default:
                t_success = false;
                break;
        }
    }
    
    env->DeleteLocalRef(t_class);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////


bool MCJavaConvertParameters(JNIEnv *env, const char *p_signature, va_list p_args, MCJavaMethodParams *&r_params, bool p_global_refs)
{
    bool t_success = true;
    
    MCJavaMethodParams *t_params = nil;
    
    if (t_success)
        t_success = MCMemoryNew(t_params);
    
    const char *t_return_type;
    
    if (t_success)
        t_success = nil != (t_return_type = native_sigchar_to_javasig(*p_signature));
    
    if (t_success)
    {
        t_params->return_type = native_sigchar_to_returntype(*p_signature++);
    }
    
    if (t_success)
        t_success = MCCStringClone("(", t_params->signature);
    
    
    jstring t_java_string;
    const char *t_cstring;
    const MCString *t_mcstring;
    jbyteArray t_byte_array;
    
    const char *t_param_sig;
    uint32_t t_index = 0;
    
    while (t_success && *p_signature != '\0')
    {
        t_success = MCMemoryResizeArray(t_index + 1, t_params->params, t_params->param_count);
        if (t_success)
            t_success = MCMemoryResizeArray(t_index + 1, t_params->delete_param, t_params->param_count);
        
        if (t_success)
            t_success = nil != (t_param_sig = native_sigchar_to_javasig(*p_signature));
        
        if (t_success)
            t_success = MCCStringAppend(t_params->signature, t_param_sig);
        
        if (t_success)
        {
            jvalue t_value;
            bool t_delete = false;
            bool t_object = false;
            switch(native_sigchar_to_returntype(*p_signature))
            {
                case kMCJavaTypeCString:
                {
                    t_cstring = va_arg(p_args, const char *);
                    
                    t_success = MCJavaStringFromNative(env, t_cstring, t_java_string);
                    if (t_success)
                        t_value . l = t_java_string;
                    
                    t_delete = true;
                    t_object = true;
                }
                    break;
                case kMCJavaTypeMCString:
                {
                    t_mcstring = va_arg(p_args, const MCString *);
                    
                    t_success = MCJavaStringFromNative(env, t_mcstring, t_java_string);
                    if (t_success)
                        t_value . l = t_java_string;
                    
                    t_delete = true;
                    t_object = true;
                }
                    break;
                case kMCJavaTypeMCStringUnicode:
                {
                    t_mcstring = va_arg(p_args, const MCString *);
                    
                    t_success = MCJavaStringFromUnicode(env, t_mcstring, t_java_string);
                    if (t_success)
                        t_value . l = t_java_string;
                    
                    t_delete = true;
                    t_object = true;
                }
                    break;
                case kMCJavaTypeByteArray:
                {
                    t_mcstring = va_arg(p_args, const MCString *);
                    
                    t_success = MCJavaByteArrayFromData(env, t_mcstring, t_byte_array);
                    if (t_success)
                        t_value.l = t_byte_array;
                    
                    t_delete = true;
                    t_object = true;
                }
                    break;
                case kMCJavaTypeInt:
                    t_value . i = va_arg(p_args, int);
                    t_delete = false;
                    break;
                case kMCJavaTypeLong:
                    t_value . j = va_arg(p_args, int64_t);
                    t_delete = false;
                    break;
                case kMCJavaTypeBoolean:
                    t_value . z = va_arg(p_args, int) ? JNI_TRUE : JNI_FALSE;
                    t_delete = false;
                    break;
                case kMCJavaTypeList:
                    t_value . l = va_arg(p_args, jobject);
                    t_delete = false;
                    t_object = true;
                    break;
                // MM-2012-02-22: Added ability to create Java maps
                case kMCJavaTypeMap:
                    t_value . l = va_arg(p_args, jobject);
                    t_delete = false;
                    t_object = true;
                    break;
                case kMCJavaTypeObject:
                    t_value . l = va_arg(p_args, jobject);
                    t_delete = false;
                    t_object = true;
                    break;
				case kMCJavaTypeFloat:
					t_value . f = va_arg(p_args, float);
					t_delete = false;
					break;
				case kMCJavaTypeDouble:
					t_value . d = va_arg(p_args, double);
					t_delete = false;
					break;
            }
            if (p_global_refs && t_object)
            {
                t_params->params[t_index].l = env->NewGlobalRef(t_value.l);
                if (t_delete)
                    env->DeleteLocalRef(t_value.l);
                t_params->delete_param[t_index] = true;
            }
            else
            {
                t_params->params[t_index] = t_value;
                t_params->delete_param[t_index] = t_delete;
            }
        }
        
        t_index += 1;
        p_signature += 1;
    }
    
    if (t_success)
       t_success = MCCStringAppendFormat(t_params->signature, ")%s", t_return_type);
    
    if (t_success)
    {
        r_params = t_params;
    }
    else
    {
        MCJavaMethodParamsFree(env, t_params);
    }
    
    return t_success;
}

void MCJavaColorToComponents(uint32_t p_color, uint16_t &r_red, uint16_t &r_green, uint16_t &r_blue, uint16_t &r_alpha)
{
    uint8_t t_red, t_green, t_blue, t_alpha;
    t_alpha = p_color >> 24;
    t_red = (p_color >> 16) & 0xFF;
    t_green = (p_color >> 8) & 0xFF;
    t_blue = p_color & 0xFF;
    
    r_red = (t_red << 8) | t_red;
    r_green = (t_green << 8) | t_green;
    r_blue = (t_blue << 8) | t_blue;
    r_alpha = (t_alpha << 8) | t_alpha;
}
