#ifndef libfoundation_foundation_java_private_h
#define libfoundation_foundation_java_private_h

bool initialise_jvm();
void finalise_jvm();

enum MCJavaCallType {
    MCJavaCallTypeInstance,
    MCJavaCallTypeStatic,
    MCJavaCallTypeNonVirtual,
    MCJavaCallTypeConstructor,
    MCJavaCallTypeInterfaceProxy,
    MCJavaCallTypeGetter,
    MCJavaCallTypeSetter,
    MCJavaCallTypeStaticGetter,
    MCJavaCallTypeStaticSetter
};

enum MCJavaType {
    kMCJavaTypeUnknown,
    kMCJavaTypeVoid,
    kMCJavaTypeBoolean,
    kMCJavaTypeByte,
    kMCJavaTypeChar,
    kMCJavaTypeShort,
    kMCJavaTypeInt,
    kMCJavaTypeLong,
    kMCJavaTypeFloat,
    kMCJavaTypeDouble,
    kMCJavaTypeArray,
    kMCJavaTypeObject,
};

typedef struct
{
    const char *name;
    MCJavaType type;
}
java_type_map;

static const java_type_map type_map[] =
{
    {"V", kMCJavaTypeVoid},
    {"Z", kMCJavaTypeBoolean},
    {"B", kMCJavaTypeByte},
    {"C", kMCJavaTypeChar},
    {"S", kMCJavaTypeShort},
    {"I", kMCJavaTypeInt},
    {"J", kMCJavaTypeLong},
    {"F", kMCJavaTypeFloat},
    {"D", kMCJavaTypeDouble},
    {"[", kMCJavaTypeArray},
    {"L", kMCJavaTypeObject}
};

typedef struct __MCJavaObject *MCJavaObjectRef;


bool MCJavaPrivateCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count);
bool MCJavaPrivateCallJNIMethodOnEnv(void *p_env, MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count);
bool MCJavaPrivateObjectDescribe(MCValueRef p_value, MCStringRef &r_desc);
bool MCJavaPrivateConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string);
bool MCJavaPrivateConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object);
bool MCJavaPrivateConvertDataRefToJByteArray(MCDataRef p_string, MCJavaObjectRef &r_object);
bool MCJavaPrivateConvertJByteArrayToDataRef(MCJavaObjectRef p_object, MCDataRef &r_string);

void* MCJavaPrivateGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_arguments, MCStringRef p_return, int p_call_type);
void MCJavaPrivateDestroyObject(MCJavaObjectRef p_object);
bool MCJavaPrivateCheckSignature(MCTypeInfoRef p_signature, MCStringRef p_args, MCStringRef p_return, int p_call_type);
bool MCJavaPrivateGetJObjectClassName(MCJavaObjectRef p_object, MCStringRef &r_name);

void* MCJavaPrivateGlobalRef(void *p_object);

bool MCJavaPrivateErrorThrow(MCTypeInfoRef p_error);
bool MCJavaPrivateErrorsInitialize();
void MCJavaPrivateErrorsFinalize();

extern MCTypeInfoRef kMCJavaNativeMethodIdErrorTypeInfo;
extern MCTypeInfoRef kMCJavaNativeMethodCallErrorTypeInfo;
extern MCTypeInfoRef kMCJavaBindingStringSignatureErrorTypeInfo;
extern MCTypeInfoRef kMCJavaCouldNotInitialiseJREErrorTypeInfo;
extern MCTypeInfoRef kMCJavaJRENotSupportedErrorTypeInfo;
extern MCTypeInfoRef kMCJavaInterfaceCallbackSignatureErrorTypeInfo;

#endif
