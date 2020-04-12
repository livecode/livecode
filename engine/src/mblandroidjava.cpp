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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "exec.h"

#include "mblandroidjava.h"

#include <jni.h>


////////////////////////////////////////////////////////////////////////////////

static jclass s_boolean_class = nil;
static jmethodID s_boolean_constructor = nil;
static jmethodID s_boolean_boolean_value= nil;

static bool init_boolean_class(JNIEnv *env)
{
    jclass t_boolean_class = env->FindClass("java/lang/Boolean");
    s_boolean_class = (jclass)env->NewGlobalRef(t_boolean_class);
    
    if (s_boolean_class == nil)
        return false;
    
    if (s_boolean_constructor == nil)
        s_boolean_constructor = env->GetMethodID(s_boolean_class, "<init>", "(Z)V");
    if (s_boolean_boolean_value == nil)
        s_boolean_boolean_value = env->GetMethodID(s_boolean_class, "booleanValue", "()Z");
    if (s_boolean_constructor == nil || s_boolean_boolean_value == nil)
        return false;
    
    return true;
}

static jclass s_integer_class = nil;
static jmethodID s_integer_constructor = nil;
static jmethodID s_integer_integer_value = nil;

static bool init_integer_class(JNIEnv *env)
{
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_integer_class = env->FindClass("java/lang/Integer");
    s_integer_class = (jclass)env->NewGlobalRef(t_integer_class);
    
    if (s_integer_class == nil)
		return false;

	if (s_integer_constructor == nil)
		s_integer_constructor = env->GetMethodID(s_integer_class, "<init>", "(I)V");
    if (s_integer_integer_value == nil)
        s_integer_integer_value = env->GetMethodID(s_integer_class, "intValue", "()I");
	if (s_integer_constructor == nil || s_integer_integer_value == nil)
		return false;
	
	return true;
}

static jclass s_double_class = nil;
static jmethodID s_double_constructor = nil;
static jmethodID s_double_double_value = nil;

static bool init_double_class(JNIEnv *env)
{
    jclass t_double_class = env->FindClass("java/lang/Double");
    s_double_class = (jclass)env->NewGlobalRef(t_double_class);
    
    if (s_double_class == nil)
        return false;
    
    if (s_double_constructor == nil)
        s_double_constructor = env->GetMethodID(s_double_class, "<init>", "(D)V");
    if (s_double_double_value == nil)
        s_double_double_value = env->GetMethodID(s_double_class, "doubleValue", "()D");
    if (s_double_constructor == nil || s_double_double_value == nil)
        return false;
    
    return true;
}

static jclass s_string_class = nil;

static bool init_string_class(JNIEnv *env)
{
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_string_class = env->FindClass("java/lang/String");
    s_string_class = (jclass)env->NewGlobalRef(t_string_class);
    
	if (s_string_class == nil)
		return false;

	return true;
}

static jclass s_array_list_class = nil;
static jmethodID s_array_list_constructor = nil;
static jmethodID s_array_list_append = nil;

bool init_arraylist_class(JNIEnv *env)
{
    // PM-2014-02-16: Bug [[ 14489 ]] Use global ref for s_array_list_class to ensure it will be valid next time we use it
    jclass t_array_list_class = env->FindClass("java/util/ArrayList");
    s_array_list_class = (jclass)env->NewGlobalRef(t_array_list_class);
    
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
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_hash_map_class = env->FindClass("java/util/HashMap");
    s_hash_map_class = (jclass)env->NewGlobalRef(t_hash_map_class);
    
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
    {
        // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
        jclass t_map_entry_class = env->FindClass("java/util/Map$Entry");
        s_map_entry_class = (jclass)env->NewGlobalRef(t_map_entry_class);
    }
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
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_iterator_class = env->FindClass("java/util/Iterator");
    s_iterator_class = (jclass)env->NewGlobalRef(t_iterator_class);
    
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
    // PM-2014-02-16: Bug [[ 14489 ]] Use global refs for statics
    jclass t_set_class = env->FindClass("java/util/Set");
    s_set_class = (jclass)env->NewGlobalRef(t_set_class);

	if (s_set_class == nil)
		return false;
	
	if (s_set_iterator == nil)
		s_set_iterator = env->GetMethodID(s_set_class, "iterator", "()Ljava/util/Iterator;");
	if (s_set_iterator == nil)
		return false;
	
	return true;
}

static jclass s_object_array_class = nil;
static jclass s_byte_array_class = nil;

static bool init_array_classes(JNIEnv *env)
{
    jclass t_object_array_class = env->FindClass("[Ljava/lang/Object;");
    s_object_array_class = (jclass)env->NewGlobalRef(t_object_array_class);
    if (s_object_array_class == nil)
        return false;

    jclass t_byte_array_class = env->FindClass("[B");
    s_byte_array_class = (jclass)env->NewGlobalRef(t_byte_array_class);
    if (s_byte_array_class == nil)
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////

// PM-2015-02-16: [[ Bug 14489 ]] Delete global ref
void free_arraylist_class(JNIEnv *env)
{
    if (s_array_list_class != nil)
    {
        env->DeleteGlobalRef(s_array_list_class);
        s_array_list_class = nil;
    }
}

void free_boolean_class(JNIEnv *env)
{
    if (s_boolean_class != nil)
    {
        env->DeleteGlobalRef(s_boolean_class);
        s_boolean_class = nil;
    }
}

void free_integer_class(JNIEnv *env)
{
    if (s_integer_class != nil)
    {
        env->DeleteGlobalRef(s_integer_class);
        s_integer_class = nil;
    }
}

void free_double_class(JNIEnv *env)
{
    if (s_double_class != nil)
    {
        env->DeleteGlobalRef(s_double_class);
        s_double_class = nil;
    }
}

void free_string_class(JNIEnv *env)
{
    if (s_string_class != nil)
    {
        env->DeleteGlobalRef(s_string_class);
        s_string_class = nil;
    }
}

void free_hashmap_class(JNIEnv *env)
{
    if (s_hash_map_class != nil)
    {
        env->DeleteGlobalRef(s_hash_map_class);
        s_hash_map_class = nil;
    }
}

void free_map_entry_class(JNIEnv *env)
{
    if (s_map_entry_class != nil)
    {
        env->DeleteGlobalRef(s_map_entry_class);
        s_map_entry_class = nil;
    }
}

void free_iterator_class(JNIEnv *env)
{
    if (s_iterator_class != nil)
    {
        env->DeleteGlobalRef(s_iterator_class);
        s_iterator_class = nil;
    }
}

void free_set_class(JNIEnv *env)
{
    if (s_set_class != nil)
    {
        env->DeleteGlobalRef(s_set_class);
        s_set_class = nil;
    }
}

void free_array_classes(JNIEnv *env)
{
    if (s_object_array_class != nil)
    {
        env->DeleteGlobalRef(s_object_array_class);
        s_object_array_class = nil;
    }
    if (s_byte_array_class != nil)
    {
        env->DeleteGlobalRef(s_byte_array_class);
        s_byte_array_class = nil;
    }
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaInitialize(JNIEnv *env)
{
    if (!init_boolean_class(env))
        return false;
    
    if (!init_integer_class(env))
        return false;
    
    if (!init_double_class(env))
        return false;
    
    if (!init_string_class(env))
        return false;
    
    if (!init_arraylist_class(env))
        return false;
    
    if (!init_hashmap_class(env))
        return false;
    
    if (!init_iterator_class(env))
        return false;
    
    if (!init_set_class(env))
        return false;
    
    if (!init_array_classes(env))
        return false;
    
    return true;
}

void MCJavaFinalize(JNIEnv *env)
{
    free_arraylist_class(env);
    free_boolean_class(env);
    free_integer_class(env);
    free_double_class(env);
    free_string_class(env);
    free_hashmap_class(env);
    free_map_entry_class(env);
    free_iterator_class(env);
    free_set_class(env);
    free_array_classes(env);
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

bool MCJavaStringFromUTF8(JNIEnv *env, const char *p_string, jstring &r_java_string)
{
    if (p_string == nil)
    {
        r_java_string = nil;
        return true;
    }
    
    /* The JNI NewStringUTF function expected Modified UTF-8 which encodes NUL
     * as two bytes, and SMP chars as two surrogates. This differs from the
     * encoding of p_string, which is standard UTF-8. Therefore, we convert
     * p_string to UTF-16, and then use NewString instead. */
    
    /* Create a StringRef from the UTF-8, which does the necessary conversion
     * to UTF-16. */
    MCAutoStringRef t_string;
    if (!MCStringCreateWithBytes((const char_t *)p_string, strlen(p_string), kMCStringEncodingUTF8, false, &t_string))
    {
        return false;
    }
    
    /* Now lock the content of the stringref as UTF-16 so we can pass the buffer
     * and length to the JNI function. */
    MCAutoStringRefAsUTF16String t_utf16_string;
    if (!t_utf16_string.Lock(*t_string))
    {
        return false;
    }
    
    return nil != (r_java_string = env->NewString(t_utf16_string.Ptr(), t_utf16_string.Size()));
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

bool MCJavaStringToUTF8(JNIEnv *env, jstring p_java_string, char *&r_utf_8)
{
    bool t_success = true;
    
    const jchar *t_unicode_string = nil;
    int t_unicode_length = 0;
    char *t_utf8 = nil;
    int t_utf8_length;
    
    if (p_java_string != nil)
        t_unicode_string = env -> GetStringChars(p_java_string, NULL);
    
    if (t_unicode_string != nil)
    {
        t_unicode_length = env -> GetStringLength(p_java_string);

        t_utf8_length = UnicodeToUTF8(t_unicode_string, t_unicode_length * 2, nil, 0);

        t_success = MCMemoryAllocate(t_utf8_length + 1, t_utf8);

        if (t_success)
        {
            UnicodeToUTF8(t_unicode_string, t_unicode_length * 2, t_utf8, t_utf8_length);
            t_utf8[t_utf8_length] = 0;
        }

        env -> ReleaseStringChars(p_java_string, t_unicode_string);
    }
    
    if (t_success)
        r_utf_8 = t_utf8;
    
    return t_success;
}

//////////

bool MCJavaStringFromStringRef(JNIEnv *env, MCStringRef p_string, jstring &r_java_string)
{
    if (p_string == nil)
    {
        r_java_string = nil;
        return true;
    }
    
    bool t_success = true;
    jstring t_java_string = nil;

    // SN-2015-04-28: [[ Bug 15151 ]] If the string is native, we don't want to
    //  unnativise it - that's how we end up with a CantBeNative empty string!
    if (MCStringIsNative(p_string))
    {
        unichar_t *t_string;
        uindex_t t_string_length;
        t_string = nil;
        t_success = MCStringConvertToUnicode(p_string, t_string, t_string_length);
        
        if (t_success)
            t_success = nil != (t_java_string = env -> NewString((const jchar*)t_string, t_string_length));
        
        if (t_string != nil)
            MCMemoryDeleteArray(t_string);
    }
    else
        t_success = nil != (t_java_string = env -> NewString((const jchar*)MCStringGetCharPtr(p_string), MCStringGetLength(p_string)));
    
    if (t_success)
        r_java_string = t_java_string;

    return t_success;

}

//////////

bool MCJavaStringToStringRef(JNIEnv *env, jstring p_java_string, MCStringRef &r_string)
{
    unichar_t *t_unicode_string;
    uindex_t t_length;
    
    if (MCJavaStringToUnicode(env, p_java_string, t_unicode_string, t_length))
        return MCStringCreateWithChars(t_unicode_string, t_length, r_string);
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaByteArrayFromDataRef(JNIEnv *env, MCDataRef p_data, jbyteArray &r_byte_array)
{
    if (p_data == nil || MCDataGetLength(p_data) == 0)
    {
        r_byte_array = nil;
        return true;
    }
    
    bool t_success = true;
    jbyteArray t_bytes = nil;
    
    t_success = nil != (t_bytes = env -> NewByteArray(MCDataGetLength(p_data)));
    
    if (t_success)
    {
        env -> SetByteArrayRegion(t_bytes, 0, MCDataGetLength(p_data), (const jbyte*)MCDataGetBytePtr(p_data));
    }
    
    if (t_success)
        r_byte_array = t_bytes;
    
    return t_success;
}

//////////

bool MCJavaByteArrayToDataRef(JNIEnv *env, jbyteArray p_byte_array, MCDataRef& r_data)
{
    bool t_success = true;
    
    jbyte *t_bytes = nil;
    uint32_t t_length = 0;
    
    if (p_byte_array != nil)
        t_bytes = env -> GetByteArrayElements(p_byte_array, nil);
    
    if (t_bytes != nil)
    {
        t_length = env -> GetArrayLength(p_byte_array);
        t_success = MCDataCreateWithBytes((const byte_t *)t_bytes, t_length, r_data);
        env -> ReleaseByteArrayElements(p_byte_array, t_bytes, 0);
    }
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaIntegerFromInt(JNIEnv *env, jint p_int, jobject &r_integer)
{
    bool t_success = true;
    
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

bool MCJavaListAppendStringRef(JNIEnv *env, jobject p_list, MCStringRef p_string)
{
    bool t_success = true;
    jstring t_jstring = nil;
    
    t_success = MCJavaStringFromStringRef(env, p_string, t_jstring);
    
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

bool MCJavaMapPutStringToObject(JNIEnv *env, jobject p_map, MCStringRef p_key, jobject p_value)
{
	bool t_success;
	t_success = true;
	
	jstring t_key;
	t_key = nil;
	if (t_success)
		t_success = MCJavaStringFromStringRef(env, p_key, t_key);
	
	if (t_success)
		t_success = MCJavaMapPutObjectToObject(env, p_map, t_key, p_value);
	
	if (t_key != nil)
		env->DeleteLocalRef(t_key);
	
	return t_success;
}

bool MCJavaMapPutStringToString(JNIEnv *env, jobject p_map, MCStringRef p_key, MCStringRef p_value)
{
    bool t_success;
    t_success = true;
    
    jstring t_value;
    t_value = nil;
    if (t_success)
        t_success = MCJavaStringFromStringRef(env, p_value, t_value);
    
    if (t_success)
        t_success = MCJavaMapPutStringToObject(env, p_map, p_key, t_value);
    
    if (t_value != nil)
        env->DeleteLocalRef(t_value);
    
    return t_success;
}

//////////

bool MCJavaMapFromArrayRef(JNIEnv *p_env, MCArrayRef p_value, jobject &r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    jobject t_map;
    t_map = NULL;
    if (t_success)
        t_success = MCJavaInitMap(p_env, t_map);
    
    MCValueRef t_element;
    t_element = NULL;
    
    MCNameRef t_name;
    t_name = NULL;
    
    uintptr_t t_position;
    t_position = 0;
    
    while (t_success && MCArrayIterate(p_value, t_position, t_name, t_element))
    {
        jobject t_value;
        t_value = NULL;
        if (t_success)
            t_success = MCJavaObjectFromValueRef(p_env, t_element, t_value);
        
        if (t_success)
            t_success = MCJavaMapPutStringToObject(p_env, t_map, MCNameGetString(t_name), t_value);
        
        if (t_value != NULL)
            p_env -> DeleteLocalRef(t_value);
    }
    
    if (t_success)
        r_object = t_map;
    else if (t_map != nil)
        MCJavaFreeMap(p_env, t_map);
    
    return t_success;
}

//////////

typedef struct
{
    MCArrayRef array;
} map_to_array_context_t;

static bool s_map_to_array_callback(JNIEnv *p_env, MCNameRef p_key, jobject p_value, void *p_context)
{
    bool t_success = true;
    
    map_to_array_context_t *t_context = (map_to_array_context_t*)p_context;
    
    MCAutoValueRef t_value;
    if (t_success)
        t_success = MCJavaObjectToValueRef(p_env, p_value, &t_value);

    if (t_success)
        t_success = MCArrayStoreValue(t_context -> array, false, p_key, *t_value);
        
    return t_success;
}

bool MCJavaIterateMap(JNIEnv *env, jobject p_map, MCJavaMapCallback p_callback, void *p_context)
{
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
		
		t_success = nil != (t_entry = env->CallObjectMethod(t_iterator, s_iterator_next));
		
		// fetch entry key & value
		if (t_success)
			t_success = nil != (t_key = env->CallObjectMethod(t_entry, s_map_entry_get_key));
		if (t_success)
			t_success = nil != (t_value = env->CallObjectMethod(t_entry, s_map_entry_get_value));
		
		// convert key string to stringref
        MCAutoStringRef t_key_string;
		if (t_success)
			t_success = MCJavaStringToStringRef(env, (jstring)t_key, &t_key_string);
		
        // and then to nameref
        MCNewAutoNameRef t_key_name;
        if (t_success)
            t_success = MCNameCreate(*t_key_string, &t_key_name);
        
		// call callback
		if (t_success)
			t_success = p_callback(env, *t_key_name, t_value, p_context);
		

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

bool MCJavaMapToArrayRef(JNIEnv *p_env, jobject p_map, MCArrayRef &r_array)
{
    if (!init_string_class(p_env) || !init_hashmap_class(p_env))
        return false;
    
    bool t_success = true;
    
    MCAutoArrayRef t_array;
    t_success = MCArrayCreateMutable(&t_array);
    
    map_to_array_context_t t_context;
    t_context.array = *t_array;
    
    if (t_success)
        t_success = MCJavaIterateMap(p_env, p_map, s_map_to_array_callback, &t_context);
    
    if (t_success)
        r_array = MCValueRetain(*t_array);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaBooleanFromBooleanRef(JNIEnv *p_env, MCBooleanRef p_value, jobject& r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    jobject t_boolean;
    t_boolean = NULL;
    if (t_success)
    {
        t_boolean = p_env -> NewObject(s_boolean_class, s_boolean_constructor, p_value == kMCTrue);
        t_success = t_boolean != NULL;
    }
    
    if (t_success)
        r_object = t_boolean;

    return t_success;
}

//////////

bool MCJavaBooleanToBooleanRef(JNIEnv *p_env, jobject p_object, MCBooleanRef& r_value)
{
    if (p_object == NULL)
    {
        r_value = NULL;
        return true;
    }
    
    if (p_env -> CallBooleanMethod(p_object, s_boolean_boolean_value))
        r_value = MCValueRetain(kMCTrue);
    else
        r_value = MCValueRetain(kMCFalse);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaNumberFromNumberRef(JNIEnv *p_env, MCNumberRef p_value, jobject& r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    jobject t_number;
    t_number = NULL;
    if (t_success)
    {
        if (MCNumberIsInteger(p_value))
            t_number = p_env -> NewObject(s_integer_class, s_integer_constructor, MCNumberFetchAsInteger(p_value));
        else if (MCNumberIsInteger(p_value))
            t_number = p_env -> NewObject(s_double_class, s_double_constructor, MCNumberFetchAsReal(p_value));
        t_success = t_number != NULL;
    }
    
    if (t_success)
        r_object = t_number;
    
    return t_success;
}

//////////

bool MCJavaNumberToNumberRef(JNIEnv *p_env, jobject p_object, MCNumberRef& r_value)
{
    if (p_object == NULL)
    {
        r_value = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    MCAutoNumberRef t_value;
    if (t_success)
    {
        if (p_env -> IsInstanceOf(p_object, s_integer_class))
        {
            integer_t t_int;
            t_int = p_env -> CallIntMethod(p_object, s_integer_integer_value);
            t_success = MCNumberCreateWithInteger(t_int, &t_value);
        }
        else if (p_env -> IsInstanceOf(p_object, s_double_class))
        {
            real64_t t_double;
            t_double = p_env -> CallDoubleMethod(p_object, s_double_double_value);
            t_success = MCNumberCreateWithReal(t_double, &t_value);
        }
        else
            t_success = false;
    }
    
    if (t_success)
        r_value = MCValueRetain(*t_value);

    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCJavaArrayFromArrayRef(JNIEnv *p_env, MCArrayRef p_value, jobjectArray& r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    jclass t_object_class;
    t_object_class = NULL;
    if (t_success)
    {
        t_object_class = p_env -> FindClass("java/lang/Object");
        t_success = t_object_class != NULL;
    }
    
    jobjectArray t_array;
    t_array = NULL;
    if (t_success)
    {
        t_array = p_env -> NewObjectArray(MCArrayGetCount(p_value), t_object_class, NULL);
        t_success = t_array != NULL;
    }
    
    if (t_success)
    {
        for (uint32_t i = 0; i < MCArrayGetCount(p_value) && t_success; i++)
        {
            MCValueRef t_value;
            t_value = NULL;
            if (t_success)
                t_success = MCArrayFetchValueAtIndex(p_value, i + 1, t_value);
            
            jobject t_j_value;
            t_j_value = NULL;
            if (t_success)
                t_success = MCJavaObjectFromValueRef(p_env, t_value, t_j_value);
            
            if (t_success)
                p_env -> SetObjectArrayElement(t_array, i, t_j_value);
            
            if (t_j_value != NULL)
                p_env -> DeleteLocalRef(t_j_value);
        }
    }
    
    if (t_success)
        r_object = t_array;
    
    return t_success;
}

//////////

bool MCJavaArrayToArrayRef(JNIEnv *p_env, jobjectArray p_object, MCArrayRef& r_value)
{
    if (p_object == NULL)
    {
        r_value = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    MCAutoArrayRef t_array;
    if (t_success)
        t_success = MCArrayCreateMutable(&t_array);
    
    if (t_success)
    {
        uint32_t t_size;
        t_size = p_env -> GetArrayLength(p_object);

        for (uint32_t i = 0; i < t_size && t_success; i++)
        {
            MCAutoValueRef t_value;
            if (t_success)
            {
                jobject t_object;
                t_object = NULL;
                
                t_object = p_env -> GetObjectArrayElement(p_object, i);
                t_success = MCJavaObjectToValueRef(p_env, t_object, &t_value);
                
                if (t_object != NULL)
                    p_env -> DeleteLocalRef(t_object);
            }
            
            if (t_success)
                t_success = MCArrayStoreValueAtIndex(*t_array, i + 1, *t_value);
        }
    }
    
    if (t_success)
        r_value = MCValueRetain(*t_array);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////
// MM-2015-06-10: We can now communicate with Java using MCValueRefs.
//
// Using @ as the sig type, ValueRefs will be automatically converted to and from jobjects.
//
// The type of the params passed on the Java side will be inferred from the type of the ValueRef.
// For example, if you pass a sequential ArrayRef then we will assume that the corresponding
// param type on the Java side will be Object[].
//
// The return type on the Java side should always be Object.
//
// The following conversions will occur:
//    * MCBooleanRef   -> Boolean
//    * MCNumberRef    -> Integer (if int)
//                     -> Double (if real)
//    * MCNameRef      -> String
//    * MCStringRef    -> String
//    * MCDataRef      -> byte[]
//    * MCArrayRef     -> Object[] (if sequential)
//                     -> Map (otherwise)
//

bool MCJavaObjectFromValueRef(JNIEnv *p_env, MCValueRef p_value, jobject& r_object)
{
    if (p_value == NULL)
    {
        r_object = NULL;
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    switch (MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeNull:
            r_object = NULL;
            break;
            
        case kMCValueTypeCodeBoolean:
            t_success = MCJavaBooleanFromBooleanRef(p_env, (MCBooleanRef) p_value, r_object);
            break;
            
        case kMCValueTypeCodeNumber:
            t_success = MCJavaNumberFromNumberRef(p_env, (MCNumberRef) p_value, r_object);
            break;
            
        case kMCValueTypeCodeName:
        {
            jstring t_string;
            t_success = MCJavaStringFromStringRef(p_env, MCNameGetString((MCNameRef) p_value), t_string);
            if (t_success)
                r_object = t_string;
            break;
        }
            
        case kMCValueTypeCodeString:
        {
            jstring t_string;
            t_success = MCJavaStringFromStringRef(p_env, (MCStringRef) p_value, t_string);
            if (t_success)
                r_object = t_string;
            break;
        }
            
        case kMCValueTypeCodeData:
        {
            jbyteArray t_array;
            t_success = MCJavaByteArrayFromDataRef(p_env, (MCDataRef) p_value, t_array);
            if (t_success)
                r_object = t_array;
            break;
        }
            
        case kMCValueTypeCodeArray:
            if (MCArrayIsSequence((MCArrayRef) p_value))
            {
                jobjectArray t_array;
                t_success = MCJavaArrayFromArrayRef(p_env, (MCArrayRef) p_value, t_array);
                if (t_success)
                    r_object = t_array;
            }
            else
                t_success = MCJavaMapFromArrayRef(p_env, (MCArrayRef) p_value, r_object);
            break;
            
        case kMCValueTypeCodeList:
            // TODO
            t_success = false;
            break;
            
        case kMCValueTypeCodeSet:
            // TODO
            t_success = false;
            break;
            
        case kMCValueTypeCodeCustom:
            // TODO
            t_success = false;
            break;
            
        default:
          MCUnreachable();
          break;
    }
    
    return t_success;
}

//////////

bool MCJavaObjectToValueRef(JNIEnv *p_env, jobject p_object, MCValueRef &r_value)
{
    if (p_object == NULL)
    {
        r_value = NULL;
        return true;
    }
    
    if (p_env -> IsInstanceOf(p_object, s_boolean_class))
    {
        MCAutoBooleanRef t_value;
        if (MCJavaBooleanToBooleanRef(p_env, p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_integer_class) || p_env -> IsInstanceOf(p_object, s_double_class))
    {
        MCAutoNumberRef t_value;
        if (MCJavaNumberToNumberRef(p_env, p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_string_class))
    {
        MCAutoStringRef t_value;
        if (MCJavaStringToStringRef(p_env, (jstring) p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_byte_array_class))
    {
        MCAutoDataRef t_value;
        if (MCJavaByteArrayToDataRef(p_env, (jbyteArray) p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_object_array_class))
    {
        MCAutoArrayRef t_value;
        if (MCJavaArrayToArrayRef(p_env, (jobjectArray) p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    else if (p_env -> IsInstanceOf(p_object, s_hash_map_class))
    {
        MCAutoArrayRef t_value;
        if (MCJavaMapToArrayRef(p_env, p_object, &t_value))
        {
            r_value = MCValueRetain(*t_value);
            return true;
        }
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

static MCJavaType valueref_to_returntype(MCValueRef p_value)
{
    switch (MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeBoolean:
            return kMCJavaTypeBooleanRef;
        case kMCValueTypeCodeNumber:
            return kMCJavaTypeNumberRef;
        case kMCValueTypeCodeString:
            return kMCJavaTypeMCStringRef;
        case kMCValueTypeCodeData:
            return kMCJavaTypeByteArray;
        case kMCValueTypeCodeArray:
            if (MCArrayIsSequence((MCArrayRef) p_value))
                return kMCJavaTypeObjectArray;
            else
                return kMCJavaTypeMap;
            
        case kMCValueTypeCodeNull:
        case kMCValueTypeCodeName:
        case kMCValueTypeCodeList:
        case kMCValueTypeCodeSet:
        case kMCValueTypeCodeCustom:
        default:
            return kMCJavaTypeMCValueRef;
    }
}

static MCJavaType native_sigchar_to_returntype(char p_sigchar)
{
    switch (p_sigchar)
    {
        case 'v':
            return kMCJavaTypeVoid;
        case 's':
            return kMCJavaTypeCString;
        case 't':
            return kMCJavaTypeUtf8CString;
        case 'S':
            return kMCJavaTypeMCString;
        case 'U':
            return kMCJavaTypeMCStringUnicode;
		case 'x':
			return kMCJavaTypeMCStringRef;
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
        case '@':
            return kMCJavaTypeMCValueRef;
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
        case kMCJavaTypeUtf8CString: // string from utf8 char *
        case kMCJavaTypeMCString:  // string from MCString *
        case kMCJavaTypeMCStringUnicode:  // string from utf16 MCString *
		case kMCJavaTypeMCStringRef: // string from MCStringRef
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
        case kMCJavaTypeMCValueRef:
            return "Ljava/lang/Object;";
        case kMCJavaTypeObjectArray:
            return "[Ljava/lang/Object;";
        case kMCJavaTypeBooleanRef:
            return "java/lang/Boolean";
        case kMCJavaTypeNumberRef:
            return "java/lang/Number";
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
            
			case kMCJavaTypeMCStringRef:
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
    jobject t_java_object;
    
    const char *t_param_sig;
    uint32_t t_index = 0;
    
    while (t_success && *p_signature != '\0')
    {
        MCJavaType t_arg_type;
        t_arg_type = native_sigchar_to_returntype(*p_signature);
        
        // If the sig suggests a value ref has been passed, then determine the type based on the type of the value ref passed.
        // Otherwise just use the type passed in the sig.
        MCJavaType t_param_type;
        MCValueRef t_value_ref;
        t_value_ref = NULL;
        if (t_arg_type == kMCJavaTypeMCValueRef)
        {
            t_value_ref = va_arg(p_args, MCValueRef);
            t_param_type = valueref_to_returntype(t_value_ref);
        }
        else
            t_param_type = t_arg_type;
        
        t_success = MCMemoryResizeArray(t_index + 1, t_params->params, t_params->param_count);
        if (t_success)
            t_success = MCMemoryResizeArray(t_index + 1, t_params->delete_param, t_params->param_count);
        
        if (t_success)
            t_success = nil != (t_param_sig = return_type_to_java_sig(t_param_type));
        
        if (t_success)
            t_success = MCCStringAppend(t_params->signature, t_param_sig);
        
        if (t_success)
        {
            jvalue t_value;
            bool t_delete = false;
            bool t_object = false;
            switch (t_arg_type)
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
                case kMCJavaTypeUtf8CString:
                {
                    t_cstring = va_arg(p_args, const char *);

                    t_success = MCJavaStringFromUTF8(env, t_cstring, t_java_string);
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
				case kMCJavaTypeMCStringRef:
				{
					if (t_success)
						t_success = MCJavaStringFromStringRef(env, va_arg(p_args, MCStringRef), t_java_string);
                    if (t_success)
                        t_value . l = t_java_string;
                    
                    t_delete = true;
                    t_object = true;
				}
					break;
                case kMCJavaTypeByteArray:
                {
					if (t_success)
						t_success = MCJavaByteArrayFromDataRef(env, va_arg(p_args, MCDataRef), t_byte_array);
                    
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
                case kMCJavaTypeMCValueRef:
                {
                    if (t_success)
                        t_success = MCJavaObjectFromValueRef(env, t_value_ref, t_java_object);
                    if (t_success)
                        t_value . l = t_java_object;
                    
                    t_delete = true;
                    t_object = true;

                    break;
                }
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
        MCJavaMethodParamsFree(env, t_params, p_global_refs);
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
