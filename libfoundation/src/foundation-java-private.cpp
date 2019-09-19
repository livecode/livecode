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
    for (uindex_t i = 0; i < sizeof(type_map) / sizeof(type_map[0]); i++)
    {
        if (MCStringSubstringIsEqualToCString(p_type_code,
                                              MCRangeMake(p_range.offset, 1),
                                              type_map[i] . name,
                                              kMCStringOptionCompareExact))
        {
            return type_map[i] . type;
        }
    }
    
    return kMCJavaTypeUnknown;
}

MCJavaType MCJavaMapTypeCode(MCStringRef p_type_code)
{
    return MCJavaMapTypeCodeSubstring(p_type_code,
                                      MCRangeMake(0, MCStringGetLength(p_type_code)));
}

static bool __GetExpectedTypeCode(MCTypeInfoRef p_type, MCJavaType& r_code)
{
    if (p_type == kMCSInt8TypeInfo)
        r_code = kMCJavaTypeByte;
    else if (p_type == kMCSInt16TypeInfo)
        r_code = kMCJavaTypeShort;
    else if (p_type == kMCSInt32TypeInfo)
        r_code = kMCJavaTypeInt;
    else if (p_type == kMCSInt64TypeInfo)
        r_code = kMCJavaTypeLong;
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
    
    // At the moment we don't have a separate type for arrays.
    if (p_code == kMCJavaTypeArray &&
        t_code == kMCJavaTypeObject)
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
    
    if (t_next_type == kMCJavaTypeUnknown)
        return false;
    
    if (t_next_type == kMCJavaTypeObject)
    {
        if (!MCStringFirstIndexOfChar(p_arguments, ';', x_range . offset, kMCStringOptionCompareExact, t_length))
            return false;
        
        // Consume the ;
        t_length++;
        
        // Correct the length for the starting point
        t_length -= x_range.offset;
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
        case MCJavaCallTypeInterfaceProxy:
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
                                 MCRangeMakeMinMax(1, MCStringGetLength(p_in) - 1),
                                 r_out);
}

bool MCJavaPrivateCheckSignature(MCTypeInfoRef p_signature, MCStringRef p_args, MCStringRef p_return, int p_call_type)
{
    if (MCHandlerTypeInfoIsVariadic(p_signature))
        return false;

    MCJavaCallType t_call_type = static_cast<MCJavaCallType>(p_call_type);
    if (t_call_type == MCJavaCallTypeInterfaceProxy)
        return true;
    
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    uindex_t t_first_param = 0;
    if (__MCJavaCallNeedsClassInstance(t_call_type))
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
        case MCJavaCallTypeInterfaceProxy:
            return __MCTypeInfoConformsToJavaType(t_return_type, kMCJavaTypeObject);
        case MCJavaCallTypeSetter:
        case MCJavaCallTypeStaticSetter:
            return __MCTypeInfoConformsToJavaType(t_return_type, kMCJavaTypeVoid);
        default:
        {
            auto t_return_code = MCJavaMapTypeCode(p_return);
            return __MCTypeInfoConformsToJavaType(t_return_type, t_return_code);
        }
    }
}

MCTypeInfoRef kMCJavaNativeMethodIdErrorTypeInfo;
MCTypeInfoRef kMCJavaNativeMethodCallErrorTypeInfo;
MCTypeInfoRef kMCJavaBindingStringSignatureErrorTypeInfo;
MCTypeInfoRef kMCJavaCouldNotInitialiseJREErrorTypeInfo;
MCTypeInfoRef kMCJavaJRENotSupportedErrorTypeInfo;
MCTypeInfoRef kMCJavaInterfaceCallbackSignatureErrorTypeInfo;

bool MCJavaPrivateErrorsInitialize()
{
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.NativeMethodIdError"), MCNAME("java"), MCSTR("JNI exception thrown when getting native method id"), kMCJavaNativeMethodIdErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.NativeMethodCallError"), MCNAME("java"), MCSTR("JNI exception thrown when calling native method"), kMCJavaNativeMethodCallErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.BindingStringSignatureError"), MCNAME("java"), MCSTR("Java binding string does not match foreign handler signature or signature not supported"), kMCJavaBindingStringSignatureErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.CouldNotInitialiseJREError"), MCNAME("java"), MCSTR("Could not initialise Java Runtime Environment"), kMCJavaCouldNotInitialiseJREErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.JRENotSupported"), MCNAME("java"), MCSTR("Java Runtime Environment no supported with current configuration"), kMCJavaJRENotSupportedErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.java.InterfaceCallbackSignatureError"), MCNAME("java"), MCSTR("Handler for interface callback does not match callback signature"), kMCJavaInterfaceCallbackSignatureErrorTypeInfo))
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
    MCValueRelease(kMCJavaInterfaceCallbackSignatureErrorTypeInfo);
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

/* The MCJavaAutoLocalRef class should be used to hold a local-ref which is
 * transient. */
template<typename T>
class MCJavaAutoLocalRef
{
public:
    MCJavaAutoLocalRef(T p_object)
        : m_object(p_object)
    {
    }
    
    ~MCJavaAutoLocalRef(void)
    {
        s_env->DeleteLocalRef(m_object);
    }
    
    operator T (void) const
    {
        return m_object;
    }
    
private:
    T m_object;
};

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
    
    char t_option[PATH_MAX];
    const char *t_option_prefix = "-Djava.class.path=";
    strcpy(t_option, t_option_prefix);
    
    const char *t_class_path = getenv("CLASSPATH");
    if (t_class_path == nullptr ||
        strlen(t_option_prefix) + strlen(t_class_path) >= PATH_MAX)
    {
        t_class_path = "/usr/lib/java";
    }
    strcat(t_option, t_class_path);
    t_option[strlen(t_option_prefix)+strlen(t_class_path)] = '\0';
    
    JavaVMOption* options = new (nothrow) JavaVMOption[1];
    options[0].optionString = t_option;
    
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
    
    const jchar *t_chars = s_env->GetStringChars(p_string, 0);
    
    auto t_unichars = reinterpret_cast<const unichar_t *>(t_chars);
    uindex_t t_count = s_env->GetStringLength(p_string);

    bool t_success = MCStringCreateWithChars(t_unichars, t_count,
                                             r_string);
    
    s_env->ReleaseStringChars(p_string, t_chars);
    return t_success;
}

static bool __MCJavaStringToJString(MCStringRef p_string, jstring& r_string)
{
    MCJavaDoAttachCurrentThread();
    
    auto t_chars =
        reinterpret_cast<const jchar *>(MCStringGetCharPtr(p_string));
    
    jstring t_result = s_env->NewString(t_chars,
                                        MCStringGetLength(p_string));
    
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

static bool __MCJavaProperListFromJObjectArray(jobjectArray p_obj_array, MCProperListRef& r_list)
{
    MCJavaDoAttachCurrentThread();
    
    if (p_obj_array == nullptr)
    {
        r_list = MCValueRetain(kMCEmptyProperList);
        return true;
    }
    
    MCAutoProperListRef t_list;
    if (!MCProperListCreateMutable(&t_list))
        return false;
    
    uint32_t t_size = s_env -> GetArrayLength(p_obj_array);
    
    for (uint32_t i = 0; i < t_size; i++)
    {
        MCAutoValueRef t_value;
        
        /* Make sure the fetched jobject (local-ref) is deleted on each iteration */
        MCJavaAutoLocalRef<jobject> t_object =
            s_env -> GetObjectArrayElement(p_obj_array, i);
        
        if (t_object != nullptr)
        {
            MCAutoJavaObjectRef t_obj;
            if (!MCJavaObjectCreate(t_object, &t_obj))
                return false;

            if (!MCProperListPushElementOntoBack(*t_list, *t_obj))
                return false;
        }
        else
        {
            if (!MCProperListPushElementOntoBack(*t_list, kMCNull))
                return false;
        }
    }
    
    return MCProperListCopy(*t_list, r_list);
}

void* MCJavaPrivateGlobalRef(void *p_object)
{
    MCJavaDoAttachCurrentThread();
    jobject t_obj = static_cast<jobject>(p_object);
    return static_cast<void *>(s_env->NewGlobalRef(t_obj));
}

bool MCJavaObjectCreateNullable(jobject p_object, MCJavaObjectRef &r_object)
{
    if (p_object == nullptr)
    {
        r_object = nullptr;
        return true;
    }
    return MCJavaObjectCreate(p_object, r_object);
}

static jstring MCJavaGetJObjectClassName(jobject p_obj)
{
	MCJavaDoAttachCurrentThread();
	
	/* Make sure the fetched class local-refs are deleted on exit */
    MCJavaAutoLocalRef<jclass> t_class =
        s_env->GetObjectClass(p_obj);
    MCJavaAutoLocalRef<jclass> javaClassClass =
        s_env->FindClass("java/lang/Class");

    jmethodID javaClassNameMethod = s_env->GetMethodID(javaClassClass, "getName", "()Ljava/lang/String;");
    
    return (jstring)s_env->CallObjectMethod(t_class, javaClassNameMethod);;
}

static bool __JavaJNIInstanceMethodResult(jobject p_instance, jmethodID p_method_id, jvalue *p_params, MCJavaType p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    
    switch (p_return_type)
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
            /* Make sure the returned jobject (local-ref) is deleted on exit */
            MCJavaAutoLocalRef<jobject> t_result =
                s_env -> CallObjectMethodA(p_instance, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullable(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            s_env -> CallVoidMethodA(p_instance, p_method_id, p_params);
            return true;
            
        // Should be unreachable
        case kMCJavaTypeUnknown:
            break;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNIStaticMethodResult(jclass p_class, jmethodID p_method_id, jvalue *p_params, MCJavaType p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    
    switch (p_return_type) {
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
            /* Make sure the returned jobject (local-ref) is deleted on exit */
            MCJavaAutoLocalRef<jobject> t_result =
                s_env -> CallStaticObjectMethodA(p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullable(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            s_env -> CallStaticVoidMethodA(p_class, p_method_id, p_params);
            return true;
            
        // Should be unreachable
        case kMCJavaTypeUnknown:
            break;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNINonVirtualMethodResult(jobject p_instance, jclass p_class, jmethodID p_method_id, jvalue *p_params, MCJavaType p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    
    switch (p_return_type) {
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
            /* Make sure the returned jobject (local-ref) is deleted on exit */
            MCJavaAutoLocalRef<jobject> t_result =
                s_env -> CallNonvirtualObjectMethodA(p_instance, p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullable(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
            s_env -> CallNonvirtualVoidMethodA(p_instance, p_class, p_method_id, p_params);
            return true;
            
        // Should be unreachable
        case kMCJavaTypeUnknown:
            break;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNIGetFieldResult(jobject p_instance, jfieldID p_field_id, MCJavaType p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    
    switch (p_return_type)
    {
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
            /* Make sure the returned jobject (local-ref) is deleted on exit */
            MCJavaAutoLocalRef<jobject> t_result =
                s_env -> GetObjectField(p_instance, p_field_id);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullable(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
        case kMCJavaTypeUnknown:
            break;
    }
    
    MCUnreachableReturn(false);
}

static bool __JavaJNIGetStaticFieldResult(jclass p_class, jfieldID p_field_id, MCJavaType p_field_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    
    switch (p_field_type)
    {
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
            /* Make sure the returned jobject (local-ref) is deleted on exit */
            MCJavaAutoLocalRef<jobject> t_result =
                s_env -> GetStaticObjectField(p_class, p_field_id);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreateNullable(t_result, t_result_value))
                return false;
            *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
            return true;
        }
        case kMCJavaTypeVoid:
        case kMCJavaTypeUnknown:
            break;
    }
    
    MCUnreachableReturn(false);
}

static void __JavaJNISetFieldResult(jobject p_instance, jfieldID p_field_id, const void *p_param, MCJavaType p_field_type)
{
    MCJavaDoAttachCurrentThread();
    
    switch (p_field_type)
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
        case kMCJavaTypeUnknown:
            break;
    }
    
    MCUnreachable();
}

static void __JavaJNISetStaticFieldResult(jclass p_class, jfieldID p_field_id, const void *p_param, MCJavaType p_field_type)
{
    MCJavaDoAttachCurrentThread();
    
    switch (p_field_type)
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
        case kMCJavaTypeUnknown:
            break;
    }
    
    MCUnreachable();
}

static bool __JavaJNIConstructorResult(jclass p_class, jmethodID p_method_id, jvalue *p_params, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    
    /* Make sure the returned jobject (local-ref) is deleted on exit */
    MCJavaAutoLocalRef<jobject> t_result =
        s_env -> NewObjectA(p_class, p_method_id, p_params);
    
    MCJavaObjectRef t_result_value;
    if (!MCJavaObjectCreate(t_result, t_result_value))
        return false;
    *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;

    return true;
}

static bool __JavaJNIGetParams(void **args, MCTypeInfoRef p_signature, jvalue *&r_params)
{
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    /* There's no need for any local-ref deletion here as the refs held in the
     * jvalues will be global-refs. */
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
            case kMCJavaTypeVoid:
            case kMCJavaTypeUnknown:
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

static jclass MCJavaPrivateFindClass(MCNameRef p_class_name)
{
    // The system class loader does not know about LC's android engine
    // classes. We cache the android engine class loader on startup and
    // call its findClass method to find any classes in the com.runrev.android
    // package, or any custom classes that have been included.
    // For all other classes we just use the JNIEnv FindClass method &
    // system class loader.
#if defined(TARGET_SUBPLATFORM_ANDROID)
    jstring t_class_string;
    if (!__MCJavaStringToJString(MCNameGetString(p_class_name), t_class_string))
        return nullptr;
    
    /* The class loader is a global-ref so no need to manage it locally */
    extern void* MCAndroidGetClassLoader(void);
    jobject t_class_loader =
        static_cast<jobject>(MCAndroidGetClassLoader());
    
    /* Make sure the class loader class local-ref is deleted on exit */
    MCJavaAutoLocalRef<jclass> t_class_loader_class =
        s_env->FindClass("java/lang/ClassLoader");
    jmethodID t_find_class = s_env->GetMethodID(t_class_loader_class,
                                                "findClass",
                                                "(Ljava/lang/String;)Ljava/lang/Class;");
    
    /* The class is the return value so don't manage its local-ref here */
    jobject t_class = s_env->CallObjectMethod(t_class_loader,
                                              t_find_class,
                                              t_class_string);
    if (t_class != nullptr)
        return static_cast<jclass>(t_class);
    
    // Clear the ClassNotFoundException
    s_env -> ExceptionClear();
#endif
    
    MCAutoStringRef t_class_path;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class_path))
        return nullptr;
    
    MCAutoStringRefAsCString t_class_cstring;
    if (!t_class_cstring.Lock(*t_class_path))
        return nullptr;
    
    return s_env->FindClass(*t_class_cstring);
}

static bool __MCJavaIsHandlerSuitableForListener(MCNameRef p_class_name, MCValueRef p_handlers)
{
    /* This handler only manipulates java objects locally, so all jobject
     * (local-refs) are held in auto class instances. */
    
    MCJavaAutoLocalRef<jclass> t_class_class = s_env->FindClass("java/lang/Class");
    jmethodID t_get_methods = s_env->GetMethodID(t_class_class, "getMethods",
                                                 "()[Ljava/lang/reflect/Method;");
    
    MCJavaAutoLocalRef<jclass> t_class = MCJavaPrivateFindClass(p_class_name);
    
    MCJavaAutoLocalRef<jobjectArray> t_methods =
        static_cast<jobjectArray>(s_env->CallObjectMethod(t_class,
                                                          t_get_methods));

    MCJavaAutoLocalRef<jclass> t_method_class =
        s_env->FindClass("java/lang/reflect/Method");
    
    jmethodID t_get_parameters = s_env->GetMethodID(t_method_class,
                                                    "getParameterTypes",
                                                    "()[Ljava/lang/Class;");
    
    jmethodID t_get_return =  s_env->GetMethodID(t_method_class,
                                                 "getReturnType",
                                                 "()Ljava/lang/Class;");
    
    MCJavaAutoLocalRef<jclass> t_void_class = s_env->FindClass("java/lang/Void");
    jfieldID t_void_type_field = s_env->GetStaticFieldID(t_void_class,
                                               "TYPE",
                                               "Ljava/lang/Class;");
    MCJavaAutoLocalRef<jobject> t_void_type =
        s_env-> GetStaticObjectField(t_void_class, t_void_type_field);
    
    
    // Lambda to check if a handler is suitable for the given method
    auto t_check_handler = [&](MCHandlerRef p_handler, jobject p_method)
    {
        MCTypeInfoRef t_type_info = MCValueGetTypeInfo(p_handler);

        // Ensure all callback handler parameters are of JavaObject type
        uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(t_type_info);
        
        for (uindex_t i = 0; i < t_param_count; ++i)
        {
            if (!__MCTypeInfoConformsToJavaType(MCHandlerTypeInfoGetParameterType(t_type_info, i),
                                                kMCJavaTypeObject))
            {
                return MCErrorCreateAndThrowWithMessage(kMCJavaInterfaceCallbackSignatureErrorTypeInfo,
                                                        MCSTR("Callback handler %{handler} parameters must conform to JObject type"),
                                                        "handler", p_handler,
                                                        nullptr);
            }
        }
        
        // Ensure the correct number of parameters
        MCJavaAutoLocalRef<jobjectArray> t_params =
            static_cast<jobjectArray>(s_env->CallObjectMethod(p_method,
                                                              t_get_parameters));
        uindex_t t_expected_param_count =
            static_cast<uindex_t>(s_env->GetArrayLength(t_params));
        if (t_param_count != t_expected_param_count)
        {
            MCAutoNumberRef t_exp;
            if (!MCNumberCreateWithUnsignedInteger(t_expected_param_count,
                                                   &t_exp))
                return false;
            
            return MCErrorCreateAndThrowWithMessage(kMCJavaInterfaceCallbackSignatureErrorTypeInfo,
                                                    MCSTR("Wrong number of parameters for callback handler %{handler}: expected %{number}"),
                                                    "handler", p_handler,
                                                    "number", *t_exp,
                                                    nullptr);
        }

        MCJavaAutoLocalRef<jobject> t_return_class =
            s_env->CallObjectMethod(p_method, t_get_return);
        MCTypeInfoRef t_return_type = MCHandlerTypeInfoGetReturnType(t_type_info);
        
        // Check the return types match
        if (s_env->IsSameObject(t_return_class, t_void_type))
        {
            if (t_return_type == kMCNullTypeInfo)
                return true;
        }
        else
        {
            if (__MCTypeInfoConformsToJavaType(t_return_type,
                                               kMCJavaTypeObject))
                return true;
        }
        
        return MCErrorCreateAndThrowWithMessage(kMCJavaInterfaceCallbackSignatureErrorTypeInfo,
                                                MCSTR("Mismatched return parameter for callback handler %{handler}"),
                                                "handler", p_handler,
                                                nullptr);
    };

    uindex_t t_num_methods = s_env->GetArrayLength(t_methods);
    if (t_num_methods == 0)
    {
        return MCErrorCreateAndThrowWithMessage(kMCJavaInterfaceCallbackSignatureErrorTypeInfo,
                                                MCSTR("Target interface has no callback methods"),
                                                nullptr);
    }
    
    if (MCValueGetTypeCode(p_handlers) == kMCValueTypeCodeArray)
    {
        // Collect all the method names of this interface
        jmethodID t_get_method_name = s_env->GetMethodID(t_method_class,
                                                         "getName",
                                                         "()Ljava/lang/String;");
        MCAutoStringRefArray t_names;
        for (uindex_t i = 0; i < t_num_methods; i++)
        {
            jobject t_object = s_env->GetObjectArrayElement(t_methods, i);
            jstring t_name =
                static_cast<jstring>(s_env->CallObjectMethod(t_object,
                                                             t_get_method_name));
            MCAutoStringRef t_name_stringref;
            if (!__MCJavaStringFromJString(t_name, &t_name_stringref))
                return false;
            
            if (!t_names.Push(*t_name_stringref))
                return false;
        }
        
        // Array of handlers for interface proxy
        uintptr_t t_iterator = 0;
        MCNameRef t_key;
        MCValueRef t_value;
        while (MCArrayIterate(static_cast<MCArrayRef>(p_handlers),
                             t_iterator, t_key, t_value))
        {
            MCStringRef t_match = nullptr;
            uindex_t j = 0;
            for (; j < t_names.Size(); j++)
            {
                if (MCStringIsEqualTo(MCNameGetString(t_key),
                                      t_names[j],
                                      kMCStringOptionCompareCaseless))
                {
                    t_match = t_names[j];
                    break;
                }
            }
            if (t_match == nullptr)
            {
                // No method with matching name found
                return MCErrorCreateAndThrowWithMessage(kMCJavaInterfaceCallbackSignatureErrorTypeInfo,
                                                        MCSTR("No callback method with name %{name}"),
                                                        "name", t_key,
                                                        nullptr);
            }
            
            // If we get here, we have a matching name, so check the handler
            if (!t_check_handler(static_cast<MCHandlerRef>(t_value),
                                 s_env->GetObjectArrayElement(t_methods, j)))
                return false;
        }
        
        // If we get here, then all handlers were assigned to valid callbacks
        // in the interface
        return true;
        
    }
    else if (MCValueGetTypeCode(p_handlers) == kMCValueTypeCodeHandler)
    {
        // Only one handler provided - ensure there is only one callback
        if (t_num_methods != 1)
        {
            return MCErrorCreateAndThrowWithMessage(kMCJavaInterfaceCallbackSignatureErrorTypeInfo,
                                                    MCSTR("Ambiguous callback assignment - target interface has multiple callback methods"),
                                                    nullptr);
        }
        
        return t_check_handler(static_cast<MCHandlerRef>(p_handlers),
                               s_env->GetObjectArrayElement(t_methods, 0));
    }

    // Value was not of correct type
    return false;
}

bool MCJavaCreateInterfaceProxy(MCNameRef p_class_name, MCTypeInfoRef p_signature, void *p_method_id, void *r_result, void **p_args, uindex_t p_arg_count)
{
    if (MCHandlerTypeInfoGetParameterCount(p_signature) != 1)
        return false;
    
    MCValueRef t_handlers = *(static_cast<MCValueRef *>(p_args[0]));
    
    if (!__MCJavaIsHandlerSuitableForListener(p_class_name, t_handlers))
    {
        return false;
    }
    
    /* Make sure the LCB internal class local-ref is deleted on exit */
    MCJavaAutoLocalRef<jclass> t_inv_handler_class =
        MCJavaPrivateFindClass(MCNAME("com.runrev.android.LCBInvocationHandler"));

    jmethodID t_method = static_cast<jmethodID>(p_method_id);
    
    /* Make sure the interface class local-ref is deleted on exit */
    MCJavaAutoLocalRef<jclass> t_interface =
        MCJavaPrivateFindClass(p_class_name);
    
    jlong t_handler = reinterpret_cast<jlong>(MCValueRetain(t_handlers));
    
    /* Make sure the proxy object local-ref is deleted on exit */
    MCJavaAutoLocalRef<jobject> t_proxy =
        s_env->CallStaticObjectMethod(t_inv_handler_class,
                                      t_method,
                                      static_cast<jclass>(t_interface),
                                      t_handler);
    
    
    MCJavaObjectRef t_result_value;
    bool t_success = MCJavaObjectCreateNullable(t_proxy, t_result_value);
    if (t_success)
    {
        *(static_cast<MCJavaObjectRef *>(r_result)) = t_result_value;
    }
    
    return t_success;
}

bool MCJavaPrivateCallJNIMethodOnEnv(void *p_env, MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count)
{
    JNIEnv *t_old_env;
    t_old_env = s_env;
    s_env = (JNIEnv *)p_env;
    bool t_success = MCJavaPrivateCallJNIMethod(p_class_name, p_method_id, p_call_type, p_signature, r_return, p_args, p_arg_count);
    s_env = t_old_env;
    return t_success;
}

bool MCJavaPrivateCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count)
{
    if (p_method_id == nullptr)
        return false;
    
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    MCJavaType t_return_type;
    if (!__GetExpectedTypeCode(MCHandlerTypeInfoGetReturnType(p_signature), t_return_type))
        return false;
    
    /* Any refs in the jvalue array here are global-refs (as they come from
     * MCJavaObject values) so they don't need to be managed. */
    jvalue *t_params = nullptr;
    if (p_call_type != MCJavaCallTypeInterfaceProxy &&
        !__JavaJNIGetParams(p_args, p_signature, t_params))
        return false;
    
    switch (p_call_type)
    {
        case MCJavaCallTypeInterfaceProxy:
            MCAssert(t_return_type == kMCJavaTypeObject);
            if (!MCJavaCreateInterfaceProxy(p_class_name, p_signature,
                                            p_method_id, r_return,
                                            p_args, p_arg_count))
                return false;
            break;
            
        // JavaJNI...Result functions only return false due to memory
        // allocation failures. If they succeed, fall through the switch
        // statement and check the JNIEnv for exceptions.
        case MCJavaCallTypeConstructor:
        {
            MCAssert(t_return_type == kMCJavaTypeObject);
            /* Make sure the target class local-ref is deleted on exit */
            MCJavaAutoLocalRef<jclass> t_target_class =
                MCJavaPrivateFindClass(p_class_name);
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
            /* Make sure the target class local-ref is deleted on exit */
            MCJavaAutoLocalRef<jclass> t_target_class =
                MCJavaPrivateFindClass(p_class_name);
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
            /* Make sure the target class local-ref is deleted on exit */
            MCJavaAutoLocalRef<jclass> t_target_class =
                MCJavaPrivateFindClass(p_class_name);
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
            /* Make sure the target class local-ref is deleted on exit */
            MCJavaAutoLocalRef<jclass> t_target_class =
                MCJavaPrivateFindClass(p_class_name);
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
#ifdef _DEBUG
        s_env -> ExceptionDescribe();
#endif
        
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
    
    /* Make sure the created (localref) jobject is deleted on return */
    MCJavaAutoLocalRef<jstring> t_held_string = t_string;
    
    return MCJavaObjectCreate(t_held_string, r_object);
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
    
    /* Make sure the created (localref) jobject is deleted on return */
    MCJavaAutoLocalRef<jbyteArray> t_held_array = t_array;
    
    return MCJavaObjectCreate(t_held_array, r_object);
}

bool MCJavaPrivateConvertJByteArrayToDataRef(MCJavaObjectRef p_object, MCDataRef &r_data)
{
    auto t_data = static_cast<jbyteArray>(MCJavaObjectGetObject(p_object));
    return __MCJavaDataFromJByteArray(t_data, r_data);
}

bool MCJavaPrivateGetJObjectClassName(MCJavaObjectRef p_object, MCStringRef &r_name)
{
    jobject t_object = static_cast<jobject>(MCJavaObjectGetObject(p_object));
    
    /* Make sure the created (localref) jobject is deleted on return */
    MCJavaAutoLocalRef<jstring> t_class_name =
        MCJavaGetJObjectClassName(t_object);
    
    return __MCJavaStringFromJString(t_class_name, r_name);
}

void* MCJavaPrivateGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_arguments, MCStringRef p_return, int p_call_type)
{
    MCAutoStringRefAsCString t_method_cstring, t_return_cstring;
    t_method_cstring . Lock(p_method_name);
    t_return_cstring . Lock(p_return);
    
    MCJavaDoAttachCurrentThread();
    
    /* Make sure the created (localref) jobject is deleted on return */
    MCJavaAutoLocalRef<jclass> t_java_class =
        MCJavaPrivateFindClass(p_class_name);
    
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
            case MCJavaCallTypeInterfaceProxy:
            {
                /* Make sure the created (localref) jobject is deleted on return */
                MCJavaAutoLocalRef<jclass> t_inv_handler_class =
                    MCJavaPrivateFindClass(MCNAME("com.runrev.android.LCBInvocationHandler"));
                
                t_id = s_env->GetStaticMethodID(t_inv_handler_class,
                                                "getProxy",
                                                "(Ljava/lang/Class;J)Ljava/lang/Object;");
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
#ifdef _DEBUG
        s_env -> ExceptionDescribe();
#endif
        
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

jobject MCJavaPrivateDoNativeListenerCallback(jlong p_handler, jstring p_method_name, jobjectArray p_args)
{
    MCJavaDoAttachCurrentThread();
    
    MCAutoStringRef t_method_name;
    if (!__MCJavaStringFromJString(p_method_name, &t_method_name))
        return nullptr;
 
    MCValueRef t_handler = nullptr;
    MCValueRef t_handlers = reinterpret_cast<MCValueRef>(p_handler);
    if (MCValueGetTypeCode(t_handlers) == kMCValueTypeCodeArray)
    {
        // Array of handlers for interface proxy
        MCNewAutoNameRef t_key;
        if (!MCNameCreate(*t_method_name, &t_key) ||
            !MCArrayFetchValue(static_cast<MCArrayRef>(t_handlers),
                              false, *t_key, t_handler) ||
            MCValueGetTypeCode(t_handler) != kMCValueTypeCodeHandler)
        {
            t_handler = nullptr;
        }
    }
    else
    {
        MCAssert(MCValueGetTypeCode(t_handlers) == kMCValueTypeCodeHandler);
        // Single handler for listener interface
        t_handler = t_handlers;
    }
    
    if (t_handler == nullptr)
    {
        MCErrorThrowGenericWithMessage(MCSTR("callback handler not found for listener method %{method}"),
                                       "method", *t_method_name, nullptr);
        return nullptr;
    }

    // We have an LCB handler, so just invoke with the args.
    MCAutoValueRef t_result;
    MCAutoProperListRef t_list;
    if (!__MCJavaProperListFromJObjectArray(p_args, &t_list))
        return nullptr;
    
    MCProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(*t_list, t_mutable_list))
        return nullptr;
	
	/* The handler is run on the script thread and could call code which executes
	 * MCJavaDoAttachCurrentThread() so we must ensure s_env is reset correctly for the
	 * current thread */
	JNIEnv *t_old_env;
	t_old_env = s_env;
	MCErrorRef t_error =
        MCHandlerTryToExternalInvokeWithList(static_cast<MCHandlerRef>(t_handler),
                                             t_mutable_list, &t_result);
	s_env = t_old_env;
	
    jobject t_return = nullptr;
    if (*t_result != nil)
    {
        if (MCValueGetTypeCode(*t_result) != kMCValueTypeCodeNull)
        {
            MCTypeInfoRef t_return_type = MCValueGetTypeInfo(*t_result);
            MCAssert(__MCTypeInfoConformsToJavaType(t_return_type,
                                                    kMCJavaTypeObject));
            t_return = static_cast<jobject>
                (MCJavaObjectGetObject(static_cast<MCJavaObjectRef>
                                       (*t_result)));
        }
    }
    MCValueRelease(t_mutable_list);
    
    if (t_error != nil)
        MCErrorThrow(t_error);
    
    /* The return value will be a global ref which might well get deleted when
     * the t_result value is released, so create a new local-ref here - which is
     * what is expected as this method is called to return a jobject to a native
     * method call into C. */
    return s_env->NewLocalRef(t_return);
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

void* MCJavaPrivateGlobalRef(void *p_object)
{
    return p_object;
}
#endif
