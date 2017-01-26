#ifndef libfoundation_foundation_java_private_h
#define libfoundation_foundation_java_private_h

bool initialise_jvm();
void finalise_jvm();

enum MCJavaCallType {
    MCJavaCallTypeInstance,
    MCJavaCallTypeStatic,
    MCJavaCallTypeNonVirtual
};

enum MCJavaType {
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

static java_type_map type_map[] =
{
    {"Z", kMCJavaTypeBoolean},
    {"B", kMCJavaTypeByte},
    {"C", kMCJavaTypeChar},
    {"S", kMCJavaTypeShort},
    {"I", kMCJavaTypeInt},
    {"J", kMCJavaTypeLong},
    {"F", kMCJavaTypeFloat},
    {"D", kMCJavaTypeDouble},
};

typedef struct __MCJavaObject *MCJavaObjectRef;


bool MCJavaPrivateCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count);
bool MCJavaPrivateObjectDescribe(MCValueRef p_value, MCStringRef &r_desc);
bool MCJavaPrivateConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string);
bool MCJavaPrivateConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object);
bool MCJavaPrivateCallConstructor(MCNameRef p_class_name, MCListRef p_args, MCJavaObjectRef& r_object);
void* MCJavaPrivateGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_signature);

bool MCJavaPrivateCheckSignature(MCTypeInfoRef p_signature, MCStringRef p_args, MCStringRef p_return, int p_call_type);
#endif
