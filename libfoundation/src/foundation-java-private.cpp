/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"
#include "foundation-java-private.h"

static MCJavaType MCJavaMapTypeCodeSubstring(MCStringRef p_type_code, MCRange p_range)
{
    if (MCStringSubstringIsEqualToCString(p_type_code, p_range, "[",
                                          kMCStringOptionCompareExact))
        return kMCJavaTypeArray;
    
    for (uindex_t i = 0; i < sizeof(type_map) / sizeof(type_map[0]); i++)
    {
        if (MCStringSubstringIsEqualToCString(p_type_code, p_range, type_map[i] . name, kMCStringOptionCompareExact))
        {
            return type_map[i] . type;
        }
    }
    
    return kMCJavaTypeObject;
}

int MCJavaMapTypeCode(MCStringRef p_type_code)
{
    return static_cast<int>(MCJavaMapTypeCodeSubstring(p_type_code,MCRangeMake(0,MCStringGetLength(p_type_code))));
}

static bool __GetExpectedTypeCode(MCTypeInfoRef p_type, MCJavaType& r_code)
{
    if (p_type == kMCIntTypeInfo)
        r_code = kMCJavaTypeInt;
    else if (p_type == kMCBoolTypeInfo)
        r_code = kMCJavaTypeBoolean;
    else if (p_type == kMCFloatTypeInfo)
        r_code = kMCJavaTypeFloat;
    else if (p_type == kMCDoubleTypeInfo)
        r_code = kMCJavaTypeDouble;
    else if (p_type == kMCNullTypeInfo)
        r_code = kMCJavaTypeVoid;
    else if (p_type == MCJavaGetObjectTypeInfo())
        r_code = kMCJavaTypeObject;
    else
    {
        MCResolvedTypeInfo t_src, t_target;
        if (!MCTypeInfoResolve(p_type, t_src))
            return false;
        
        if (!MCTypeInfoResolve(MCJavaGetObjectTypeInfo(), t_target))
            return false;
        
        if (!MCResolvedTypeInfoConforms(t_src, t_target))
            return false;
        
        r_code = kMCJavaTypeObject;
    }
    
    return true;
}

static bool __MCTypeInfoConformsToJavaType(MCTypeInfoRef p_type, MCJavaType p_code)
{
    MCJavaType t_code;
    if (!__GetExpectedTypeCode(p_type, t_code))
        return false;
    
    // Just allow long and int interchangeably for now
    if (t_code == kMCJavaTypeInt &&
        p_code == kMCJavaTypeLong)
        return true;
    
    return t_code == p_code;
}

static bool __NextArgument(MCStringRef p_arguments, MCRange& x_range)
{
    if (x_range . offset + x_range . length >= MCStringGetLength(p_arguments))
        return false;
    
    x_range . offset = x_range . offset + x_range . length;
    
    MCRange t_new_range;
    t_new_range . offset = x_range . offset;
    t_new_range . length = 1;
    
    uindex_t t_length = 1;
    
    MCJavaType t_next_type;
    while ((t_next_type = MCJavaMapTypeCodeSubstring(p_arguments, t_new_range)) == kMCJavaTypeArray)
    {
        t_new_range . offset++;
        t_length++;
    }
    
    if (t_next_type == kMCJavaTypeObject)
    {
        if (!MCStringFirstIndexOfChar(p_arguments, ';', x_range . offset, kMCStringOptionCompareExact, t_length))
            return false;
    }
    
    x_range . length = t_length;
    return true;
}

static bool __MCJavaCallNeedsClassInstance(MCJavaCallType p_type)
{
    switch (p_type)
    {
        case MCJavaCallTypeStatic:
        case MCJavaCallTypeConstructor:
        case MCJavaCallTypeStaticGetter:
        case MCJavaCallTypeStaticSetter:
            return false;
        case MCJavaCallTypeInstance:
        case MCJavaCallTypeNonVirtual:
        case MCJavaCallTypeGetter:
        case MCJavaCallTypeSetter:
            return true;
    }
    
    MCUnreachableReturn(false);
}

static bool __RemoveSurroundingParentheses(MCStringRef p_in, MCStringRef& r_out)
{
    return MCStringCopySubstring(p_in,
                                 MCRangeMake(1, MCStringGetLength(p_in) - 2),
                                 r_out);
}

bool MCJavaPrivateCheckSignature(MCTypeInfoRef p_signature, MCStringRef p_args, MCStringRef p_return, int p_call_type)
{
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    uindex_t t_first_param = 0;
    if (__MCJavaCallNeedsClassInstance(static_cast<MCJavaCallType>(p_call_type)))
    {
        t_first_param = 1;
    }
    
    // Remove brackets from arg string
    MCAutoStringRef t_args;
    if (!__RemoveSurroundingParentheses(p_args, &t_args))
        return false;
    
    MCRange t_range = MCRangeMake(0, 0);
    
    // Check the types of the arguments.
    for(uindex_t i = t_first_param; i < t_param_count; i++)
    {
        MCTypeInfoRef t_type;
        t_type = MCHandlerTypeInfoGetParameterType(p_signature, i);
        if (!__NextArgument(*t_args, t_range))
            return false;
        MCJavaType t_jtype;
        t_jtype = MCJavaMapTypeCodeSubstring(*t_args, t_range);
        
        if (!__MCTypeInfoConformsToJavaType(t_type, t_jtype))
            return false;
    }
    
    MCTypeInfoRef t_return_type = MCHandlerTypeInfoGetReturnType(p_signature);
    switch (p_call_type)
    {
        case MCJavaCallTypeConstructor:
            return __MCTypeInfoConformsToJavaType(t_return_type, kMCJavaTypeObject);
        case MCJavaCallTypeSetter:
        case MCJavaCallTypeStaticSetter:
            return __MCTypeInfoConformsToJavaType(t_return_type, kMCJavaTypeVoid);
        default:
        {
            auto t_return_code = static_cast<MCJavaType>(MCJavaMapTypeCode(p_return));
            return __MCTypeInfoConformsToJavaType(t_return_type, t_return_code);
        }
    }
}

MCTypeInfoRef kMCJavaNativeMethodIdErrorTypeInfo;
MCTypeInfoRef kMCJavaNativeMethodCallErrorTypeInfo;
MCTypeInfoRef kMCJavaBindingStringSignatureErrorTypeInfo;
MCTypeInfoRef kMCJavaCouldNotInitialiseJREErrorTypeInfo;
MCTypeInfoRef kMCJavaJRENotSupportedErrorTypeInfo;

bool MCJavaPrivateErrorsInitialize()
{
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.NativeMethodIdError"), MCNAME("java"), MCSTR("JNI exeception thrown when getting native method id"), kMCJavaNativeMethodIdErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.NativeMethodCallError"), MCNAME("java"), MCSTR("JNI exeception thrown when calling native method"), kMCJavaNativeMethodCallErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.BindingStringSignatureError"), MCNAME("java"), MCSTR("Java binding string does not match foreign handler signature"), kMCJavaBindingStringSignatureErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.CouldNotInitialiseJREError"), MCNAME("java"), MCSTR("Could not initialise Java Runtime Environment"), kMCJavaCouldNotInitialiseJREErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.JRENotSupported"), MCNAME("java"), MCSTR("Java Runtime Environment no supported with current configuration"), kMCJavaJRENotSupportedErrorTypeInfo))
        return false;
    
    return true;
}

void MCJavaPrivateErrorsFinalize()
{
    MCValueRelease(kMCJavaNativeMethodIdErrorTypeInfo);
    MCValueRelease(kMCJavaNativeMethodCallErrorTypeInfo);
    MCValueRelease(kMCJavaBindingStringSignatureErrorTypeInfo);
    MCValueRelease(kMCJavaCouldNotInitialiseJREErrorTypeInfo);
    MCValueRelease(kMCJavaJRENotSupportedErrorTypeInfo);
}

bool MCJavaPrivateErrorThrow(MCTypeInfoRef p_error_type)
{
    MCAutoErrorRef t_error;
    if (!MCErrorCreate(p_error_type, nil, &t_error))
        return false;
    
    return MCErrorThrow(*t_error);
}

#ifdef TARGET_SUPPORTS_JAVA
#include <jni.h>

#ifdef TARGET_SUBPLATFORM_ANDROID
extern JNIEnv *MCJavaGetThreadEnv();
extern JNIEnv *MCJavaAttachCurrentThread();
extern void MCJavaDetachCurrentThread();
#endif

static JNIEnv *s_env;
static JavaVM *s_jvm;

#if defined(TARGET_PLATFORM_LINUX) || defined(TARGET_PLATFORM_MACOS_X)

static bool s_weak_link_jvm = false;

extern "C" int initialise_weak_link_jvm_with_path(const char*);
static bool initialise_weak_link_jvm()
{
    if (s_weak_link_jvm)
        return true;
    
    MCAutoStringRef t_path;
#if defined(TARGET_PLATFORM_LINUX)
    // On linux we require the path to libjvm.so to be in LD_LIBRARY_PATH
    // so we can just weak link directly.
    if (!MCStringFormat(&t_path, "libjvm.so"))
        return false;
#else
    const char *t_javahome = getenv("JAVA_HOME");
    
    if (t_javahome == nullptr)
        return false;
    
    if (!MCStringFormat(&t_path, "%s/jre/lib/jli/libjli.dylib", t_javahome))
        return false;
#endif
    
    MCAutoStringRefAsSysString t_jvm_lib;
    if (!t_jvm_lib . Lock(*t_path))
        return false;
    
    if (!initialise_weak_link_jvm_with_path(*t_jvm_lib))
        return false;
    
    s_weak_link_jvm = true;
    return true;
}

#endif

static void init_jvm_args(JavaVMInitArgs *x_args)
{
#if defined(TARGET_PLATFORM_LINUX) || defined(TARGET_PLATFORM_MACOS_X)
    JNI_GetDefaultJavaVMInitArgs(x_args);
#endif
}

static bool create_jvm(JavaVMInitArgs *p_args)
{
    int ret = 0;
#if defined(TARGET_PLATFORM_LINUX) || defined(TARGET_PLATFORM_MACOS_X)
    ret = JNI_CreateJavaVM(&s_jvm, (void **)&s_env, p_args);
#endif
    
    return ret == 0;
}

bool initialise_jvm()
{
#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
    if (!initialise_weak_link_jvm())
        return false;
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_6;
    init_jvm_args(&vm_args);
    
    JavaVMOption* options = new (nothrow) JavaVMOption[1];
    options[0].optionString = "-Djava.class.path=/usr/lib/java";
    
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;
    
    return create_jvm(&vm_args);
#endif
    return true;
}

void finalise_jvm()
{
#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
    if (s_jvm != nullptr)
    {
        s_jvm -> DestroyJavaVM();
    }
#endif
}

void MCJavaDoAttachCurrentThread()
{
#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
    s_jvm -> AttachCurrentThread((void **)&s_env, nullptr);
#else
    s_env = MCJavaAttachCurrentThread();
#endif
}

static bool __MCJavaStringFromJString(jstring p_string, MCStringRef& r_string)
{
    MCJavaDoAttachCurrentThread();
    const char *nativeString = s_env -> GetStringUTFChars(p_string, 0);
    bool t_success = MCStringCreateWithCString(nativeString, r_string);
    s_env->ReleaseStringUTFChars(p_string, nativeString);
    return t_success;
}

static bool __MCJavaStringToJString(MCStringRef p_string, jstring& r_string)
{
    MCJavaDoAttachCurrentThread();
    
    MCAutoStringRefAsCString t_string_cstring;
    t_string_cstring . Lock(p_string);
    
    jstring t_result;
    t_result = s_env->NewStringUTF(*t_string_cstring);
    
    if (t_result != nullptr)
    {
        r_string = t_result;
        return true;
    }
    
    return false;
}

static bool __MCJavaDataFromJByteArray(jbyteArray p_byte_array, MCDataRef& r_data)
{
    MCJavaDoAttachCurrentThread();
    
    jbyte *t_jbytes = nullptr;
    if (p_byte_array != nullptr)
        t_jbytes = s_env -> GetByteArrayElements(p_byte_array, nullptr);
    
    if (t_jbytes == nullptr)
        return false;

    jsize t_length = s_env -> GetArrayLength(p_byte_array);
    
    MCAssert(t_length >= 0);
    
    if (size_t(t_length) > size_t(UINDEX_MAX))
        return false;
    
    auto t_bytes = reinterpret_cast<const byte_t *>(t_jbytes);
    bool t_success = MCDataCreateWithBytes(t_bytes, t_length, r_data);
    s_env -> ReleaseByteArrayElements(p_byte_array, t_jbytes, 0);

    return t_success;
}

static bool __MCJavaDataToJByteArray(MCDataRef p_data, jbyteArray& r_byte_array)
{
    MCJavaDoAttachCurrentThread();
    
    if (p_data == nullptr || MCDataIsEmpty(p_data))
    {
        r_byte_array = nullptr;
        return true;
    }
    
    uindex_t t_length = MCDataGetLength(p_data);
    jbyteArray t_bytes = s_env -> NewByteArray(t_length);
    
    if (t_bytes == nullptr)
        return false;
    
    auto t_data = reinterpret_cast<const jbyte*>(MCDataGetBytePtr(p_data));
    s_env->SetByteArrayRegion(t_bytes, 0, t_length, t_data);
    
    r_byte_array = t_bytes;
    return true;
}

bool MCJavaObjectCreateGlobalRef(jobject p_object, MCJavaObjectRef &r_object)
{
    MCAssert(p_object != nullptr);
    void *t_obj_ptr = s_env -> NewGlobalRef(p_object);
    return MCJavaObjectCreate(t_obj_ptr, r_object);
}

bool MCJavaObjectCreateNullableGlobalRef(jobject p_object, MCJavaObjectRef &r_object)
{
    if (p_object == nullptr)
    {
        r_object = nullptr;
        return true;
    }
    return MCJavaObjectCreateGlobalRef(p_object, r_object);
}

static jstring MCJavaGetJObjectClassName(jobject p_obj)
{
    jclass t_class = s_env->GetObjectClass(p_obj);
    
    jclass javaClassClass = s_env->FindClass("java/lang/Class");
    
    jmethodID javaClassNameMethod = s_env->GetMethodID(javaClassClass, "getName", "()Ljava/lang/String;");
    
    jstring className = (jstring)s_env->CallObjectMethod(t_class, javaClassNameMethod);
    
    return className;
}

static bool __JavaJNIInstanceMethodResult(jobject p_instance, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_return_type = static_cast<MCJavaType>(p_return_type);
    
    switch (t_return_type)
    {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> CallBooleanMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> CallByteMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> CallCharMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> CallShortMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> CallIntMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> CallLongMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                s_env -> CallFloatMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> CallDoubleMethodA(p_instance, p_method_id, p_params);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeArray:
        {
            jobject t_result =
                s_env -> CallObjectMethodA(p_instance, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            s_env -> CallVoidMethodA(p_instance, p_method_id, p_params);
            return true;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNIStaticMethodResult(jclass p_class, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_return_type = static_cast<MCJavaType>(p_return_type);
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> CallStaticBooleanMethodA(p_class, p_method_id, p_params);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> CallStaticByteMethodA(p_class, p_method_id, p_params);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> CallStaticCharMethodA(p_class, p_method_id, p_params);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> CallStaticShortMethodA(p_class, p_method_id, p_params);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> CallStaticIntMethodA(p_class, p_method_id, p_params);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> CallStaticLongMethodA(p_class, p_method_id, p_params);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                s_env -> CallStaticFloatMethodA(p_class, p_method_id, p_params);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> CallStaticDoubleMethodA(p_class, p_method_id, p_params);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeArray:
        {
            jobject t_result =
                s_env -> CallStaticObjectMethodA(p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            s_env -> CallStaticVoidMethodA(p_class, p_method_id, p_params);
            return true;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNINonVirtualMethodResult(jobject p_instance, jclass p_class, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_return_type = static_cast<MCJavaType>(p_return_type);
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> CallNonvirtualBooleanMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> CallNonvirtualByteMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> CallNonvirtualCharMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> CallNonvirtualShortMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> CallNonvirtualIntMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> CallNonvirtualLongMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                s_env -> CallNonvirtualFloatMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> CallNonvirtualDoubleMethodA(p_instance, p_class, p_method_id, p_params);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeArray:
        {
            jobject t_result =
                s_env -> CallNonvirtualObjectMethodA(p_instance, p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            s_env -> CallNonvirtualVoidMethodA(p_instance, p_class, p_method_id, p_params);
            return true;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNIGetFieldResult(jobject p_instance, jfieldID p_field_id, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_return_type = static_cast<MCJavaType>(p_return_type);
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> GetBooleanField(p_instance, p_field_id);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> GetByteField(p_instance, p_field_id);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> GetCharField(p_instance, p_field_id);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> GetShortField(p_instance, p_field_id);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> GetIntField(p_instance, p_field_id);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> GetLongField(p_instance, p_field_id);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                s_env -> GetFloatField(p_instance, p_field_id);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> GetDoubleField(p_instance, p_field_id);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeArray:
        {
            jobject t_result =
                s_env -> GetObjectField(p_instance, p_field_id);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            break;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNIGetStaticFieldResult(jclass p_class, jfieldID p_field_id, int p_field_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    auto t_field_type = static_cast<MCJavaType>(p_field_type);
    
    switch (t_field_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result =
                s_env -> GetStaticBooleanField(p_class, p_field_id);
            *(static_cast<jboolean *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result =
                s_env -> GetStaticByteField(p_class, p_field_id);
            *(static_cast<jbyte *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result =
                s_env -> GetStaticCharField(p_class, p_field_id);
            *(static_cast<jchar *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result =
                s_env -> GetStaticShortField(p_class, p_field_id);
            *(static_cast<jshort *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeInt:
        {
            jint t_result =
                s_env -> GetStaticIntField(p_class, p_field_id);
            *(static_cast<jint *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result =
                s_env -> GetStaticLongField(p_class, p_field_id);
            *(static_cast<jlong *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result =
                   s_env -> GetStaticFloatField(p_class, p_field_id);
            *(static_cast<jfloat *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeDouble:
        {
            jdouble t_result =
                s_env -> GetStaticDoubleField(p_class, p_field_id);
            *(static_cast<jdouble *>(r_result)) = t_result;
            return true;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeArray:
        {
            jobject t_result =
                s_env -> GetStaticObjectField(p_class, p_field_id);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullableGlobalRef(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            break;
    }
    
    MCUnreachableReturn(false);
}

static void __JavaJNISetFieldResult(jobject p_instance, jfieldID p_field_id, const void *p_param, int p_field_type)
{
    MCJavaDoAttachCurrentThread();
    auto t_field_type = static_cast<MCJavaType>(p_field_type);
    
    switch (t_field_type)
    {
        case kMCJavaTypeBoolean:
        {
            s_env -> SetBooleanField(p_instance,
                                     p_field_id,
                                     *(static_cast<const jboolean *>(p_param)));
            return;
        }
        case kMCJavaTypeByte:
        {
            s_env -> SetByteField(p_instance,
                                  p_field_id,
                                  *(static_cast<const jbyte *>(p_param)));
            return;
        }
        case kMCJavaTypeChar:
        {
            s_env -> SetCharField(p_instance,
                                  p_field_id,
                                  *(static_cast<const jchar *>(p_param)));
            return;
        }
        case kMCJavaTypeShort:
        {
            s_env -> SetShortField(p_instance,
                                   p_field_id,
                                   *(static_cast<const jshort *>(p_param)));
            return;
        }
        case kMCJavaTypeInt:
        {
            s_env -> SetIntField(p_instance,
                                 p_field_id,
                                 *(static_cast<const jint *>(p_param)));
            return;
        }
        case kMCJavaTypeLong:
        {
            s_env -> SetLongField(p_instance,
                                  p_field_id,
                                  *(static_cast<const jlong *>(p_param)));
            return;
        }
        case kMCJavaTypeFloat:
        {
            s_env -> SetFloatField(p_instance,
                                   p_field_id,
                                   *(static_cast<const jfloat *>(p_param)));
            return;
        }
        case kMCJavaTypeDouble:
        {
            s_env -> SetDoubleField(p_instance,
                                    p_field_id,
                                    *(static_cast<const jdouble *>(p_param)));
            return;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeArray:
        {
            jobject t_obj = nullptr;
            if (p_param != nullptr)
            {
                MCJavaObjectRef t_param =
                *(static_cast<const MCJavaObjectRef *>(p_param));
                
                t_obj = static_cast<jobject>(MCJavaObjectGetObject(t_param));
            }
            s_env -> SetObjectField(p_instance,
                                    p_field_id,
                                    t_obj);
        }
        case kMCJavaTypeVoid:
            break;
    }
    
    MCUnreachable();
}

static void __JavaJNISetStaticFieldResult(jclass p_class, jfieldID p_field_id, const void *p_param, int p_field_type)
{
    MCJavaDoAttachCurrentThread();
    auto t_field_type = static_cast<MCJavaType>(p_field_type);
    
    switch (t_field_type)
    {
        case kMCJavaTypeBoolean:
        {
            s_env -> SetStaticBooleanField(p_class,
                                     p_field_id,
                                     *(static_cast<const jboolean *>(p_param)));
            return;
        }
        case kMCJavaTypeByte:
        {
            s_env -> SetStaticByteField(p_class,
                                  p_field_id,
                                  *(static_cast<const jbyte *>(p_param)));
            return;
        }
        case kMCJavaTypeChar:
        {
            s_env -> SetStaticCharField(p_class,
                                  p_field_id,
                                  *(static_cast<const jchar *>(p_param)));
            return;
        }
        case kMCJavaTypeShort:
        {
            s_env -> SetStaticShortField(p_class,
                                   p_field_id,
                                   *(static_cast<const jshort *>(p_param)));
            return;
        }
        case kMCJavaTypeInt:
        {
            s_env -> SetStaticIntField(p_class,
                                 p_field_id,
                                 *(static_cast<const jint *>(p_param)));
            return;
        }
        case kMCJavaTypeLong:
        {
            s_env -> SetStaticLongField(p_class,
                                  p_field_id,
                                  *(static_cast<const jlong *>(p_param)));
            return;
        }
        case kMCJavaTypeFloat:
        {
            s_env -> SetStaticFloatField(p_class,
                                   p_field_id,
                                   *(static_cast<const jfloat *>(p_param)));
            return;
        }
        case kMCJavaTypeDouble:
        {
            s_env -> SetStaticDoubleField(p_class,
                                    p_field_id,
                                    *(static_cast<const jdouble *>(p_param)));
            return;
        }
        case kMCJavaTypeObject:
        case kMCJavaTypeArray:
        {
            jobject t_obj = nullptr;
            if (p_param != nullptr)
            {
                MCJavaObjectRef t_param =
                    *(static_cast<const MCJavaObjectRef *>(p_param));
            
                t_obj = static_cast<jobject>(MCJavaObjectGetObject(t_param));
            }
            s_env -> SetStaticObjectField(p_class,
                                          p_field_id,
                                          t_obj);
            return;
        }
        case kMCJavaTypeVoid:
            break;
    }
    
    MCUnreachable();
}

static bool __JavaJNIConstructorResult(jclass p_class, jmethodID p_method_id, jvalue *p_params, void *r_result)
{
    MCJavaDoAttachCurrentThread();

    jobject t_result = s_env -> NewObjectA(p_class, p_method_id, p_params);
    
    MCJavaObjectRef t_result_value;
    if (!MCJavaObjectCreateGlobalRef(t_result, t_result_value))
        return false;
    *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;

    return true;
}

static bool __JavaJNIGetParams(void **args, MCTypeInfoRef p_signature, jvalue *&r_params)
{
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    MCAutoArray<jvalue> t_args;
    if (!t_args . New(t_param_count))
        return false;
    
    for (uindex_t i = 0; i < t_param_count; i++)
    {
        MCTypeInfoRef t_type = MCHandlerTypeInfoGetParameterType(p_signature, i);
        
        MCJavaType t_expected;
        if (!__GetExpectedTypeCode(t_type, t_expected))
            return false;
        
        switch (t_expected)
        {
            case kMCJavaTypeArray:
            case kMCJavaTypeObject:
            {
                MCJavaObjectRef t_obj = *(static_cast<MCJavaObjectRef *>(args[i]));
                if (t_obj == nullptr)
                {
                    t_args[i] . l = nullptr;
                }
                else
                {
                    t_args[i] . l =
                        static_cast<jobject>(MCJavaObjectGetObject(t_obj));
                }
                break;
            }
            case kMCJavaTypeBoolean:
                t_args[i].z = *(static_cast<jboolean *>(args[i]));
                break;
            case kMCJavaTypeByte:
                t_args[i].b = *(static_cast<jbyte *>(args[i]));
                break;
            case kMCJavaTypeChar:
                t_args[i].c = *(static_cast<jchar *>(args[i]));
                break;
            case kMCJavaTypeShort:
                t_args[i].s = *(static_cast<jshort *>(args[i]));
                break;
            case kMCJavaTypeInt:
                t_args[i].i = *(static_cast<jint *>(args[i]));
                break;
            case kMCJavaTypeLong:
                t_args[i].j = *(static_cast<jlong *>(args[i]));
                break;
            case kMCJavaTypeFloat:
                t_args[i].f = *(static_cast<jfloat *>(args[i]));
                break;
            case kMCJavaTypeDouble:
                t_args[i].d = *(static_cast<jdouble *>(args[i]));
                break;
            default:
                MCUnreachableReturn(false);
        }
    }
    
    t_args . Take(r_params, t_param_count);
    return true;
}

static bool MCJavaClassNameToPathString(MCNameRef p_class_name, MCStringRef& r_string)
{
    MCAutoStringRef t_escaped;
    if (!MCStringMutableCopy(MCNameGetString(p_class_name), &t_escaped))
        return false;
    
    if (!MCStringFindAndReplaceChar(*t_escaped, '.', '/', kMCStringOptionCompareExact))
        return false;
    
    return MCStringCopy(*t_escaped, r_string);
}

bool MCJavaPrivateCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count)
{
    if (p_method_id == nullptr)
        return false;
    
    MCAutoStringRef t_class;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class))
        return false;

    MCAutoStringRefAsCString t_class_cstring;
    if (!t_class_cstring . Lock(*t_class))
        return false;
    
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    MCJavaType t_return_type;
    if (!__GetExpectedTypeCode(MCHandlerTypeInfoGetReturnType(p_signature), t_return_type))
        return false;
    
    jvalue *t_params = nullptr;
    if (!__JavaJNIGetParams(p_args, p_signature, t_params))
        return false;
    
    switch (p_call_type)
    {
        // JavaJNI...Result functions only return false due to memory
        // allocation failures. If they succeed, fall through the switch
        // statement and check the JNIEnv for exceptions.
        case MCJavaCallTypeConstructor:
        {
            MCAssert(t_return_type == kMCJavaTypeObject);
            jclass t_target_class = s_env -> FindClass(*t_class_cstring);
            if (!__JavaJNIConstructorResult(t_target_class,
                                            static_cast<jmethodID>(p_method_id),
                                            &t_params[0],
                                            r_return))
                return false;
            
            break;
        }
        case MCJavaCallTypeInstance:
        {
            // Java object on which to call instance method should always be first argument.
            MCAssert(t_param_count > 0);
            jobject t_instance = t_params[0].l;
            if (t_param_count > 1)
            {
                if (!__JavaJNIInstanceMethodResult(t_instance,
                                                   static_cast<jmethodID>(p_method_id),
                                                   &t_params[1],
                                                   t_return_type,
                                                   r_return))
                    return false;
            }
            else
            {
                if (!__JavaJNIInstanceMethodResult(t_instance,
                                                   static_cast<jmethodID>(p_method_id),
                                                   nullptr,
                                                   t_return_type,
                                                   r_return))
                    return false;
            }
            break;
        }
        case MCJavaCallTypeStatic:
        {
            jclass t_target_class = s_env -> FindClass(*t_class_cstring);
            if (! __JavaJNIStaticMethodResult(t_target_class,
                                              static_cast<jmethodID>(p_method_id),
                                              t_params, t_return_type,
                                              r_return))
                return false;
            
            break;
        }
        case MCJavaCallTypeNonVirtual:
        {
            MCAssert(t_param_count > 0);
            jobject t_instance = t_params[0].l;
            jclass t_target_class = s_env -> FindClass(*t_class_cstring);
            if (t_param_count > 1)
            {
                if (!__JavaJNINonVirtualMethodResult(t_instance,
                                                    t_target_class,
                                                    static_cast<jmethodID>(p_method_id),
                                                    &t_params[1],
                                                    t_return_type,
                                                    r_return))
                    return false;
            }
            else
            {
                if (!__JavaJNINonVirtualMethodResult(t_instance,
                                                     t_target_class,
                                                     static_cast<jmethodID>(p_method_id),
                                                     nullptr,
                                                     t_return_type,
                                                     r_return))
                    return false;
            }
            break;
        }
        case MCJavaCallTypeGetter:
        case MCJavaCallTypeSetter:
        {
            MCAssert(t_param_count > 0);
            jobject t_instance = t_params[0].l;
            
            if (p_call_type == MCJavaCallTypeGetter)
            {
                if (!__JavaJNIGetFieldResult(t_instance,
                                             static_cast<jfieldID>(p_method_id),
                                             t_return_type,
                                             r_return))
                    return false;

                break;
            }
            
            MCJavaType t_field_type;
            if (!__GetExpectedTypeCode(MCHandlerTypeInfoGetParameterType(p_signature, 1),
                                       t_field_type))
                return false;
            
            __JavaJNISetFieldResult(t_instance,
                                    static_cast<jfieldID>(p_method_id),
                                    &t_params[1],
                                    t_field_type);
            break;
        }
        case MCJavaCallTypeStaticGetter:
        case MCJavaCallTypeStaticSetter:
        {
            jclass t_target_class = s_env -> FindClass(*t_class_cstring);
            if (p_call_type == MCJavaCallTypeStaticGetter)
            {
                if (!__JavaJNIGetStaticFieldResult(t_target_class,
                                                   static_cast<jfieldID>(p_method_id),
                                                   t_return_type,
                                                   r_return))
                    return false;
                
                break;
            }
            
            MCJavaType t_field_type;
            if (!__GetExpectedTypeCode(MCHandlerTypeInfoGetParameterType(p_signature, 1),
                                       t_field_type))
                return false;
            
            __JavaJNISetStaticFieldResult(t_target_class,
                                          static_cast<jfieldID>(p_method_id),
                                          &t_params[0],
                                          t_field_type);
            break;
        }
        default:
            MCUnreachableReturn(false);
    }
    
    // If we got here there were no memory errors. Check the JNI Env for
    // exceptions
    if (s_env -> ExceptionCheck() == JNI_TRUE)
    {
        s_env -> ExceptionDescribe();
        
        // Failure to clear the exception causes a crash when the JNI is
        // next used.
        s_env -> ExceptionClear();
        return MCJavaPrivateErrorThrow(kMCJavaNativeMethodCallErrorTypeInfo);
    }
    
    return true;
}

bool MCJavaPrivateObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
    MCJavaObjectRef t_obj = static_cast<MCJavaObjectRef>(p_value);
    
    MCAutoStringRef t_class_name;
    if (!MCJavaPrivateGetJObjectClassName(t_obj, &t_class_name))
        return false;
    
    void *t_object = MCJavaObjectGetObject(t_obj);
    
    return MCStringFormat(r_desc, "<java: %@ - address: %p>", *t_class_name, t_object);
}

bool MCJavaPrivateConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
    jstring t_string;
    if (!__MCJavaStringToJString(p_string, t_string))
        return false;
    
    return MCJavaObjectCreateGlobalRef(t_string, r_object);
}

bool MCJavaPrivateConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string)
{
    auto t_string = static_cast<jstring>(MCJavaObjectGetObject(p_object));
    return __MCJavaStringFromJString(t_string, r_string);
}

bool MCJavaPrivateConvertDataRefToJByteArray(MCDataRef p_data, MCJavaObjectRef &r_object)
{
    jbyteArray t_array;
    if (!__MCJavaDataToJByteArray(p_data, t_array))
        return false;
    
    return MCJavaObjectCreateGlobalRef(t_array, r_object);
}

bool MCJavaPrivateConvertJByteArrayToDataRef(MCJavaObjectRef p_object, MCDataRef &r_data)
{
    auto t_data = static_cast<jbyteArray>(MCJavaObjectGetObject(p_object));
    return __MCJavaDataFromJByteArray(t_data, r_data);
}

bool MCJavaPrivateGetJObjectClassName(MCJavaObjectRef p_object, MCStringRef &r_name)
{
    jobject t_object = static_cast<jobject>(MCJavaObjectGetObject(p_object));
    jstring t_class_name = MCJavaGetJObjectClassName(t_object);
    return __MCJavaStringFromJString(t_class_name, r_name);
}

void* MCJavaPrivateGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_arguments, MCStringRef p_return, int p_call_type)
{
    MCAutoStringRef t_class_path;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class_path))
        return nullptr;
    
    MCAutoStringRefAsCString t_class_cstring, t_method_cstring, t_return_cstring;
    t_class_cstring . Lock(*t_class_path);
    t_method_cstring . Lock(p_method_name);
    t_return_cstring . Lock(p_return);
    
    MCJavaDoAttachCurrentThread();
    
    jclass t_java_class = s_env->FindClass(*t_class_cstring);
    
    void *t_id = nullptr;
    if (t_java_class != nullptr)
    {
        switch (p_call_type)
        {
            case MCJavaCallTypeInstance:
            case MCJavaCallTypeNonVirtual:
            case MCJavaCallTypeStatic:
            {
                MCAutoStringRef t_sig;
                if (!MCStringCreateWithStrings(&t_sig, p_arguments, p_return))
                    return nullptr;
                
                MCAutoStringRefAsCString t_signature_cstring;
                if (!t_signature_cstring . Lock(*t_sig))
                    return nullptr;
                
                if (p_call_type == MCJavaCallTypeStatic)
                {
                    t_id = s_env->GetStaticMethodID(t_java_class, *t_method_cstring, *t_signature_cstring);
                    break;
                }
                
                t_id = s_env->GetMethodID(t_java_class, *t_method_cstring, *t_signature_cstring);
                break;
            }
            case MCJavaCallTypeConstructor:
            {
                // Constructors are called with a void return type, and using
                // the method name <init>
                MCAutoStringRef t_sig;
                if (!MCStringFormat(&t_sig, "%@%s", p_arguments, "V"))
                    return nullptr;
            
                MCAutoStringRefAsCString t_signature_cstring;
                if (!t_signature_cstring . Lock(*t_sig))
                    return nullptr;
    
                t_id = s_env->GetMethodID(t_java_class, "<init>", *t_signature_cstring);
                break;
            }
            case MCJavaCallTypeGetter:
                t_id = s_env -> GetFieldID(t_java_class, *t_method_cstring, *t_return_cstring);
                break;
            case MCJavaCallTypeStaticGetter:
                t_id = s_env -> GetStaticFieldID(t_java_class, *t_method_cstring, *t_return_cstring);
                break;
                
            case MCJavaCallTypeStaticSetter:
            case MCJavaCallTypeSetter:
            {
                // Remove brackets from arg string to find field type
                MCAutoStringRef t_args;
                if (!__RemoveSurroundingParentheses(p_arguments, &t_args))
                    return nullptr;
                
                MCAutoStringRefAsCString t_signature_cstring;
                if (!t_signature_cstring . Lock(*t_args))
                    return nullptr;
                
                if (p_call_type == MCJavaCallTypeSetter)
                {
                    t_id = s_env -> GetFieldID(t_java_class, *t_method_cstring, *t_signature_cstring);
                    break;
                }
                t_id = s_env -> GetStaticFieldID(t_java_class, *t_method_cstring, *t_signature_cstring);
                break;
            }
        }
    }
 
    // If we got here there were no memory errors. Check the JNI Env for
    // exceptions
    if (s_env -> ExceptionCheck() == JNI_TRUE)
    {
        s_env -> ExceptionDescribe();
        
        // Failure to clear the exception causes a crash when the JNI is
        // next used.
        s_env -> ExceptionClear();
        MCJavaPrivateErrorThrow(kMCJavaNativeMethodCallErrorTypeInfo);
        return nullptr;
    }
    return t_id;
}

void MCJavaPrivateDestroyObject(MCJavaObjectRef p_object)
{
    MCJavaDoAttachCurrentThread();
    
    jobject t_obj = static_cast<jobject>(MCJavaObjectGetObject(p_object));
    
    s_env -> DeleteGlobalRef(t_obj);
}
#else

bool initialise_jvm()
{

    return true;
}

void finalise_jvm()
{
}

bool MCJavaPrivateCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count)
{
    return false;
}

bool MCJavaPrivateObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
    return MCStringFormat (r_desc, "<java: %s>", "not supported");
}

bool MCJavaPrivateConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
    return false;
}

bool MCJavaPrivateConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string)
{
    return false;
}

bool MCJavaPrivateConvertDataRefToJByteArray(MCDataRef p_string, MCJavaObjectRef &r_object)
{
    return false;
}

bool MCJavaPrivateConvertJByteArrayToDataRef(MCJavaObjectRef p_object, MCDataRef &r_string)
{
    return false;
}

void* MCJavaPrivateGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_arguments, MCStringRef p_return, int p_call_type)
{
    return nullptr;
}

void MCJavaPrivateDestroyObject(MCJavaObjectRef p_object)
{
    // no op
}

bool MCJavaPrivateGetJObjectClassName(MCJavaObjectRef p_object, MCStringRef &r_name)
{
    return false;
}
#endif
