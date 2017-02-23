#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"
#include "foundation-java-private.h"

static MCJavaType MCJavaMapTypeCodeSubstring(MCStringRef p_type_code, MCRange p_range)
{
    if (MCStringBeginsWithCString(p_type_code, (const char_t *)"[", kMCStringOptionCompareExact))
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
    return (int)MCJavaMapTypeCodeSubstring(p_type_code, MCRangeMake(0, MCStringGetLength(p_type_code)));
}

static bool __GetExpectedTypeCode(MCTypeInfoRef p_type, MCJavaType& r_code)
{
    if (p_type == kMCIntTypeInfo)
        r_code = kMCJavaTypeLong;
    else if (p_type == kMCBoolTypeInfo)
        r_code = kMCJavaTypeBoolean;
    else if (p_type == kMCFloatTypeInfo)
        r_code = kMCJavaTypeFloat;
    else if (p_type == kMCDoubleTypeInfo)
        r_code = kMCJavaTypeDouble;
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

static bool MCJavaTypeConforms(MCTypeInfoRef p_type, MCJavaType p_code)
{
    MCJavaType t_code;
    if (!__GetExpectedTypeCode(p_type, t_code))
        return false;
    
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

bool MCJavaPrivateCheckSignature(MCTypeInfoRef p_signature, MCStringRef p_args, MCStringRef p_return, int p_call_type)
{
    uindex_t t_param_count;
    t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    uindex_t t_first_param = 0;
    if ((MCJavaCallType)p_call_type != MCJavaCallTypeStatic)
    {
        t_first_param = 1;
    }
    
    // Remove brackets from arg string
    MCAutoStringRef t_args;
    if (!MCStringCopySubstring(p_args, MCRangeMake(1, MCStringGetLength(p_args) - 2), &t_args))
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
        
        if (!MCJavaTypeConforms(t_type, t_jtype))
            return false;
    }
    
    MCJavaType t_return_code;
    t_return_code = (MCJavaType)MCJavaMapTypeCode(p_return);
    
    MCTypeInfoRef t_return_type;
    t_return_type = MCHandlerTypeInfoGetReturnType(p_signature);
    return MCJavaTypeConforms(t_return_type, t_return_code);

}

#ifdef TARGET_SUPPORTS_JAVA
#include <jni.h>

#if defined(TARGET_PLATFORM_LINUX) || defined(TARGET_PLATFORM_MACOS_X)
#include <stdlib.h>
#include <dlfcn.h>
#endif

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
    // so we can just use dlopen directly.
    if (!MCStringFormat(&t_path, "libjvm.so"))
        return false;
#else
    
    const char *t_javahome;
    t_javahome = getenv("JAVA_HOME");
    
    if (t_javahome == nil)
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
    if (s_jvm != nil)
    {
        s_jvm -> DestroyJavaVM();
    }
#endif
}

void MCJavaDoAttachCurrentThread()
{
#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_PLATFORM_LINUX)
    s_jvm -> AttachCurrentThread((void **)&s_env, nil);
#else
    s_env = MCJavaAttachCurrentThread();
#endif
}

static bool __MCJavaStringFromJString(jstring p_string, MCStringRef& r_string)
{
    MCJavaDoAttachCurrentThread();
    
    const char *nativeString = s_env -> GetStringUTFChars(p_string, 0);
    
    bool t_success;
    t_success = MCStringCreateWithCString(nativeString, r_string);
    
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
    
    if (t_result != nil)
    {
        r_string = t_result;
        return true;
    }
    
    return false;
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
    MCJavaType t_return_type = (MCJavaType)p_return_type;
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result;
            t_result = s_env -> CallBooleanMethodA(p_instance, p_method_id, p_params);
            *(jboolean *)r_result = t_result;
            break;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result;
            t_result = s_env -> CallByteMethodA(p_instance, p_method_id, p_params);
            *(jbyte *)r_result = t_result;
            break;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result;
            t_result = s_env -> CallCharMethodA(p_instance, p_method_id, p_params);
            *(jchar *)r_result = t_result;
            break;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result;
            t_result = s_env -> CallShortMethodA(p_instance, p_method_id, p_params);
            *(jshort *)r_result = t_result;
            break;
        }
        case kMCJavaTypeInt:
        {
            jint t_result;
            t_result = s_env -> CallIntMethodA(p_instance, p_method_id, p_params);
            *(jint *)r_result = t_result;
            break;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result;
            t_result = s_env -> CallLongMethodA(p_instance, p_method_id, p_params);
            *(jlong *)r_result = t_result;
            break;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result;
            t_result = s_env -> CallFloatMethodA(p_instance, p_method_id, p_params);
            *(jfloat *)r_result = t_result;
            break;
        }
        case kMCJavaTypeObject:
        default:
        {
            jobject t_result;
            t_result = s_env -> CallObjectMethodA(p_instance, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreate(t_result, t_result_value))
                return false;
            *(MCJavaObjectRef *)r_result = t_result_value;
            break;
        }
    }
    return true;
}

static bool __JavaJNIStaticMethodResult(jclass p_class, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    MCJavaType t_return_type = (MCJavaType)p_return_type;
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result;
            t_result = s_env -> CallStaticBooleanMethodA(p_class, p_method_id, p_params);
            *(jboolean *)r_result = t_result;
            break;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result;
            t_result = s_env -> CallStaticByteMethodA(p_class, p_method_id, p_params);
            *(jbyte *)r_result = t_result;
            break;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result;
            t_result = s_env -> CallStaticCharMethodA(p_class, p_method_id, p_params);
            *(jchar *)r_result = t_result;
            break;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result;
            t_result = s_env -> CallStaticShortMethodA(p_class, p_method_id, p_params);
            *(jshort *)r_result = t_result;
            break;
        }
        case kMCJavaTypeInt:
        {
            jint t_result;
            t_result = s_env -> CallStaticIntMethodA(p_class, p_method_id, p_params);
            *(jint *)r_result = t_result;
            break;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result;
            t_result = s_env -> CallStaticLongMethodA(p_class, p_method_id, p_params);
            *(jlong *)r_result = t_result;
            break;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result;
            t_result = s_env -> CallStaticFloatMethodA(p_class, p_method_id, p_params);
            *(jfloat *)r_result = t_result;
            break;
        }
        case kMCJavaTypeObject:
        default:
        {
            jobject t_result;
            t_result = s_env -> CallStaticObjectMethodA(p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreate(t_result, t_result_value))
                return false;
            *(MCJavaObjectRef *)r_result = t_result_value;
            break;
        }
    }
    return true;
}

static bool __JavaJNINonVirtualMethodResult(jobject p_instance, jclass p_class, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    MCJavaType t_return_type = (MCJavaType)p_return_type;
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result;
            t_result = s_env -> CallNonvirtualBooleanMethodA(p_instance, p_class, p_method_id, p_params);
            *(jboolean *)r_result = t_result;
            break;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result;
            t_result = s_env -> CallNonvirtualByteMethodA(p_instance, p_class, p_method_id, p_params);
            *(jbyte *)r_result = t_result;
            break;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result;
            t_result = s_env -> CallNonvirtualCharMethodA(p_instance, p_class, p_method_id, p_params);
            *(jchar *)r_result = t_result;
            break;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result;
            t_result = s_env -> CallNonvirtualShortMethodA(p_instance, p_class, p_method_id, p_params);
            *(jshort *)r_result = t_result;
            break;
        }
        case kMCJavaTypeInt:
        {
            jint t_result;
            t_result = s_env -> CallNonvirtualIntMethodA(p_instance, p_class, p_method_id, p_params);
            *(jint *)r_result = t_result;
            break;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result;
            t_result = s_env -> CallNonvirtualLongMethodA(p_instance, p_class, p_method_id, p_params);
            *(jlong *)r_result = t_result;
            break;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result;
            t_result = s_env -> CallNonvirtualFloatMethodA(p_instance, p_class, p_method_id, p_params);
            *(jfloat *)r_result = t_result;
            break;
        }
        case kMCJavaTypeObject:
        default:
        {
            jobject t_result;
            t_result = s_env -> CallNonvirtualObjectMethodA(p_instance, p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreate(t_result, t_result_value))
                return false;
            *(MCJavaObjectRef *)r_result = t_result_value;
            break;
        }
    }
    return true;
}

static bool __JavaJNIGetParams(void **args, MCTypeInfoRef p_signature, jvalue *&r_params)
{
    uindex_t t_param_count;
    t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    MCAutoArray<jvalue> t_args;
    if (!t_args . New(t_param_count))
        return false;
    
    for (uindex_t i = 0; i < t_param_count; i++)
    {
        MCTypeInfoRef t_type;
        t_type = MCHandlerTypeInfoGetParameterType(p_signature, i);
        
        MCJavaType t_expected;
        if (!__GetExpectedTypeCode(t_type, t_expected))
            return false;
        
        switch (t_expected)
        {
            case kMCJavaTypeArray:
            case kMCJavaTypeObject:
                t_args[i] . l = (jobject)MCJavaObjectGetObject(*(MCJavaObjectRef *)args[i]);
                break;
            case kMCJavaTypeBoolean:
                t_args[i].z = *(jboolean *)args[i];
                break;
            case kMCJavaTypeByte:
                t_args[i].b = *(jbyte *)args[i];
                break;
            case kMCJavaTypeChar:
                t_args[i].c = *(jchar *)args[i];
                break;
            case kMCJavaTypeShort:
                t_args[i].s = *(jshort *)args[i];
                break;
            case kMCJavaTypeInt:
                t_args[i].i = *(jint *)args[i];
                break;
            case kMCJavaTypeLong:
                t_args[i].j = *(jlong *)args[i];
                break;
            case kMCJavaTypeFloat:
                t_args[i].f = *(jfloat *)args[i];
                break;
            case kMCJavaTypeDouble:
                t_args[i].d = *(jdouble *)args[i];
                break;
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
    jmethodID t_method_id = (jmethodID)p_method_id;
    
    if (t_method_id == nil)
        return false;
    
    MCAutoStringRef t_class;
    MCAutoStringRefAsCString t_class_cstring;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class))
        return false;
    
    if (!t_class_cstring . Lock(*t_class))
        return false;
    
    jvalue *t_params = nil;
    if (!__JavaJNIGetParams(p_args, p_signature, t_params))
        return false;
    
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(p_signature);
    
    MCJavaType t_return_type;
    if (!__GetExpectedTypeCode(MCHandlerTypeInfoGetReturnType(p_signature), t_return_type))
        return false;
    
    switch (p_call_type)
    {
        case MCJavaCallTypeInstance:
        {
            // Java object on which to call instance method should always be first argument.
            MCAssert(t_param_count > 0);
            jobject t_instance = t_params[0].l;
            if (t_param_count > 1)
                return __JavaJNIInstanceMethodResult(t_instance, t_method_id, &t_params[1], t_return_type, r_return);
                else
                    return __JavaJNIInstanceMethodResult(t_instance, t_method_id, nil, t_return_type, r_return);
                    }
        case MCJavaCallTypeStatic:
        {
            jclass t_target_class = s_env -> FindClass(*t_class_cstring);
            return __JavaJNIStaticMethodResult(t_target_class, t_method_id, t_params, t_return_type, r_return);
        }
        case MCJavaCallTypeNonVirtual:
        {
            MCAssert(t_param_count > 0);
            jobject t_instance = t_params[0].l;
            jclass t_target_class = s_env -> FindClass(*t_class_cstring);
            if (t_param_count > 1)
                return __JavaJNINonVirtualMethodResult(t_instance, t_target_class, t_method_id, &t_params[1], t_return_type, r_return);
                else
                    return __JavaJNINonVirtualMethodResult(t_instance, t_target_class, t_method_id, nil, t_return_type, r_return);
                    }
    }
    
    return false;
}

bool MCJavaPrivateObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
    MCJavaObjectRef t_obj = static_cast<MCJavaObjectRef>(p_value);
    
    jobject t_target = (jobject)MCJavaObjectGetObject(t_obj);
    
    MCAutoStringRef t_class_name;
    __MCJavaStringFromJString(MCJavaGetJObjectClassName(t_target), &t_class_name);
    return MCStringFormat (r_desc, "<java: %@>", *t_class_name);
}

bool MCJavaPrivateConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
    jstring t_string;
    if (!__MCJavaStringToJString(p_string, t_string))
        return false;
    
    return MCJavaObjectCreate(t_string, r_object);
}

bool MCJavaPrivateConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string)
{
    jstring t_string;
    t_string = (jstring)MCJavaObjectGetObject(p_object);
    return __MCJavaStringFromJString(t_string, r_string);
}

bool MCJavaPrivateCallConstructor(MCNameRef p_class_name, MCListRef p_args, MCJavaObjectRef& r_object)
{
    void *t_jobject_ptr = nil;
    MCJavaDoAttachCurrentThread();
    
    MCAutoStringRef t_class_path;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class_path))
        return false;
    
    MCAutoStringRefAsCString t_class_cstring;
    t_class_cstring . Lock(*t_class_path);
    
    jclass t_class = s_env->FindClass(*t_class_cstring);
    
    jmethodID t_constructor = nil;
    if (t_class != nil)
        t_constructor = s_env->GetMethodID(t_class, "<init>", "()V");
    
    jobject t_object = nil;
    if (t_constructor != nil)
        t_object = s_env->NewObject(t_class, t_constructor);
    
    if (t_object != nil)
        t_object = s_env -> NewGlobalRef(t_object);
    
    t_jobject_ptr = t_object;
    
    return MCJavaObjectCreate(t_jobject_ptr, r_object);
}

void* MCJavaPrivateGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_signature)
{
    MCAutoStringRef t_class_path;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class_path))
        return nil;
    
    MCAutoStringRefAsCString t_class_cstring, t_method_cstring, t_signature_cstring;
    t_class_cstring . Lock(*t_class_path);
    t_method_cstring . Lock(p_method_name);
    t_signature_cstring . Lock(p_signature);
    
    jclass t_java_class = nil;
    t_java_class = s_env->FindClass(*t_class_cstring);
    
    jmethodID t_method_id = nil;
    if (t_java_class != nil)
        t_method_id = s_env->GetMethodID(t_java_class, *t_method_cstring, *t_signature_cstring);
    
    return t_method_id;
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

bool MCJavaPrivateCallConstructor(MCNameRef p_class_name, MCListRef p_args, MCJavaObjectRef& r_object)
{
    return MCJavaObjectCreate(nil, r_object);
}

void* MCJavaPrivateGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_signature)
{
    return nil;
}
#endif
