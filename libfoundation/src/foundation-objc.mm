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

#include <foundation.h>
#include <foundation-auto.h>
#include <foundation-objc.h>

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCObjcObjectTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef MCObjcObjectTypeInfo() { return kMCObjcObjectTypeInfo; }

MC_DLLEXPORT_DEF MCTypeInfoRef kMCObjcIdTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef MCObjcIdTypeInfo() { return kMCObjcIdTypeInfo; }

MC_DLLEXPORT_DEF MCTypeInfoRef kMCObjcRetainedIdTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef MCObjcRetainedIdTypeInfo() { return kMCObjcRetainedIdTypeInfo; }

MC_DLLEXPORT_DEF MCTypeInfoRef kMCObjcAutoreleasedIdTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef MCObjcAutoreleasedIdTypeInfo() { return kMCObjcAutoreleasedIdTypeInfo; }

struct __MCObjcObjectImpl
{
    id object;
};

static __MCObjcObjectImpl *__MCObjcObjectGet(MCValueRef p_obj)
{
    return static_cast<__MCObjcObjectImpl *>(MCValueGetExtraBytesPtr(p_obj));
}

static void __MCObjcObjectDestroy(MCValueRef p_value)
{
    __MCObjcObjectImpl *t_impl = __MCObjcObjectGet(p_value);
    [t_impl->object release];
}

static bool __MCObjcObjectCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
    if (p_release)
        r_copy = p_value;
    else
        r_copy = MCValueRetain(p_value);
    
    return true;
}

static bool __MCObjcObjectEqual(MCValueRef p_left, MCValueRef p_right)
{
    if (p_left == p_right)
        return true;
    
    __MCObjcObjectImpl *t_left_impl = __MCObjcObjectGet(p_left);
    __MCObjcObjectImpl *t_right_impl = __MCObjcObjectGet(p_right);
    
    return t_left_impl->object == t_right_impl->object;
}

static hash_t __MCObjcObjectHash(MCValueRef p_value)
{
    __MCObjcObjectImpl *t_impl = __MCObjcObjectGet(p_value);
    return MCHashPointer(t_impl->object);
}

bool __MCObjcObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
    __MCObjcObjectImpl *t_impl = __MCObjcObjectGet(p_value);
    return MCStringFormat(r_desc, "<objc object %p>", t_impl->object);
}

static MCValueCustomCallbacks kMCObjcObjectCustomValueCallbacks =
{
    false,
    __MCObjcObjectDestroy,
    __MCObjcObjectCopy,
    __MCObjcObjectEqual,
    __MCObjcObjectHash,
    __MCObjcObjectDescribe,
    
    nullptr,
    nullptr,
};

////////////////////////////////////////////////////////////////////////////////

/* The Id foreign value is a unmanaged wrapper around 'id' - basically like
 * Pointer but with a bridge to ObjcObject which explicitly retains its value. */

static bool
objc_id_initialize(void *contents)
{
    *(void **)contents = nullptr;
    return true;
}

static bool
objc_id_defined(void *contents)
{
    return *(void **)contents != nullptr;
}

static void
objc_id_finalize(void *contents)
{
}

static bool
objc_id_move(const MCForeignTypeDescriptor* desc, void *from, void *to)
{
    *static_cast<void **>(to) = *static_cast<void **>(from);
    return true;
}

static bool
objc_id_copy(const MCForeignTypeDescriptor* desc, void *from, void *to)
{
    *static_cast<void **>(to) = *static_cast<void **>(from);
    return true;
}

static bool
objc_id_equal(const MCForeignTypeDescriptor* desc, void *from, void *to, bool& r_equal)
{
    r_equal = *static_cast<void **>(to) == *static_cast<void **>(to);
    return true;
}

static bool
objc_id_hash(const MCForeignTypeDescriptor* desc, void *value, hash_t& r_hash)
{
    r_hash = MCHashPointer(*static_cast<void **>(value));
    return true;
}

static bool
objc_id_describe(const MCForeignTypeDescriptor* desc, void *value, MCStringRef& r_string)
{
    return MCStringFormat(r_string, "<objc id %p>", *static_cast<void **>(value));
}

static bool
objc_retained_id_describe(const MCForeignTypeDescriptor* desc, void *value, MCStringRef& r_string)
{
    return MCStringFormat(r_string, "<objc retained id %p>", *static_cast<void **>(value));
}

static bool
objc_autoreleased_id_describe(const MCForeignTypeDescriptor* desc, void *value, MCStringRef& r_string)
{
    return MCStringFormat(r_string, "<objc autoreleased id %p>", *static_cast<void **>(value));
}

static bool
objc_id_import(const MCForeignTypeDescriptor* desc, void *contents, bool p_release, MCValueRef& r_value)
{
    MCAssert(!p_release);
    return MCObjcObjectCreateWithId(*(void **)contents, (MCObjcObjectRef&)r_value);
}

static bool
objc_id_export(const MCForeignTypeDescriptor* desc, MCValueRef p_value, bool p_release, void *contents)
{
    MCAssert(!p_release);
    *(void **)contents = MCObjcObjectGetId((MCObjcObjectRef)p_value);
    return true;
}

static bool
objc_retained_id_import(const MCForeignTypeDescriptor* desc, void *contents, bool p_release, MCValueRef& r_value)
{
    MCAssert(!p_release);
    return MCObjcObjectCreateWithRetainedId(*(void **)contents, (MCObjcObjectRef&)r_value);
}

static bool
objc_retained_id_export(const MCForeignTypeDescriptor* desc, MCValueRef p_value, bool p_release, void *contents)
{
    MCAssert(!p_release);
    *(void **)contents = MCObjcObjectGetRetainedId((MCObjcObjectRef)p_value);
    return true;
}

static bool
objc_autoreleased_id_import(const MCForeignTypeDescriptor* desc, void *contents, bool p_release, MCValueRef& r_value)
{
    MCAssert(!p_release);
    return MCObjcObjectCreateWithAutoreleasedId(*(void **)contents, (MCObjcObjectRef&)r_value);
}

static bool
objc_autoreleased_id_export(const MCForeignTypeDescriptor* desc, MCValueRef p_value, bool p_release, void *contents)
{
    MCAssert(!p_release);
    *(void **)contents = MCObjcObjectGetAutoreleasedId((MCObjcObjectRef)p_value);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF bool MCObjcObjectCreateWithId(void *p_object, MCObjcObjectRef &r_object)
{
    MCObjcObjectRef t_obj;
    if (!MCValueCreateCustom(kMCObjcObjectTypeInfo,
                             sizeof(__MCObjcObjectImpl),
                             t_obj))
    {
        return false;
    }
    
    __MCObjcObjectImpl *t_impl = __MCObjcObjectGet(t_obj);
    t_impl->object = [(id)p_object retain];
    
    r_object = t_obj;

    return true;
}

MC_DLLEXPORT_DEF bool MCObjcObjectCreateWithRetainedId(void *p_object, MCObjcObjectRef &r_object)
{
    MCObjcObjectRef t_obj;
    if (!MCValueCreateCustom(kMCObjcObjectTypeInfo,
                             sizeof(__MCObjcObjectImpl),
                             t_obj))
    {
        return false;
    }
    
    __MCObjcObjectImpl *t_impl = __MCObjcObjectGet(t_obj);
    t_impl->object = (id)p_object;

    r_object = t_obj;
    
    return true;
}

MC_DLLEXPORT_DEF bool MCObjcObjectCreateWithAutoreleasedId(void *p_object, MCObjcObjectRef &r_object)
{
    return MCObjcObjectCreateWithId(p_object, r_object);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF void *MCObjcObjectGetId(const MCObjcObjectRef p_obj)
{
    __MCObjcObjectImpl *t_impl = __MCObjcObjectGet(p_obj);
    return t_impl->object;
}

MC_DLLEXPORT_DEF void *MCObjcObjectGetRetainedId(const MCObjcObjectRef p_obj)
{
    __MCObjcObjectImpl *t_impl = __MCObjcObjectGet(p_obj);
    return [(id)t_impl->object retain];
}

MC_DLLEXPORT_DEF void *MCObjcObjectGetAutoreleasedId(const MCObjcObjectRef p_obj)
{
    __MCObjcObjectImpl *t_impl = __MCObjcObjectGet(p_obj);
    return [[(id)t_impl->object retain] autorelease];
}

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCObjcObjectActionProxy: NSObject
{
    MCHandlerRef m_handler;
    MCValueRef m_context;
}
- (id)initWithHandlerRef:(MCHandlerRef)aHandler context:(MCValueRef)aContext;
- (void)dealloc;

- (void)action:(id)sender;
@end

@implementation com_runrev_livecode_MCObjcObjectActionProxy
- (id)initWithHandlerRef:(MCHandlerRef)aHandler context:(MCValueRef)aContext
{
    self = [super init];
    if (self == nil)
        return nil;
    
    m_handler = MCValueRetain(aHandler);
    m_context = aContext != nullptr ? MCValueRetain(aContext) : nullptr;
    
    return self;
}

- (void)dealloc
{
    MCValueRelease(m_handler);
    if (m_context != nullptr)
    {
        MCValueRelease(m_context);
    }
    [super dealloc];
}

- (void)action:(id)sender
{
    MCAutoValueRef t_sender_arg;
    if (!MCObjcObjectCreateWithId(sender, (MCObjcObjectRef&)&t_sender_arg))
    {
        return;
    }
    
    MCValueRef t_args[2];
    t_args[0] = *t_sender_arg;
    if (m_context != nullptr)
    {
        t_args[1] = m_context;
    }
    
    MCAutoValueRef t_result;
    if (!MCHandlerExternalInvoke(m_handler, t_args, m_context != nullptr ? 2 : 1, &t_result))
    {
        return;
    }
}
@end

extern "C"
MC_DLLEXPORT_DEF MCObjcObjectRef MCObjcObjectCreateActionProxy(MCHandlerRef p_handler, MCValueRef p_context)
{
    MCTypeInfoRef t_signature =
            MCValueGetTypeInfo(p_handler);

    uindex_t t_param_count =
            MCHandlerTypeInfoGetParameterCount(t_signature);
    
    if (p_context == nullptr)
    {
        p_context = kMCNull;
    }
    
    if ((t_param_count == 1 && p_context != kMCNull) ||
        t_param_count > 2)
    {
        MCErrorThrowGeneric(MCSTR("invalid signature for action proxy handler"));
        return nullptr;
    }
    
    if (t_param_count == 1)
    {
        p_context = nullptr;
    }

    if (MCHandlerTypeInfoGetParameterType(t_signature, 0) != kMCObjcObjectTypeInfo)
    {
        MCErrorThrowGeneric(MCSTR("first parameter for action proxy handler must be ObjcObject"));
        return nullptr;
    }

    com_runrev_livecode_MCObjcObjectActionProxy *t_proxy =
            [[com_runrev_livecode_MCObjcObjectActionProxy alloc] initWithHandlerRef:p_handler context:p_context];
    
    MCObjcObjectRef t_proxy_object = nullptr;
    if (!MCObjcObjectCreateWithRetainedId(t_proxy, t_proxy_object))
    {
        [t_proxy release];
        return nullptr;
    }
    
    return t_proxy_object;
}

extern "C"
MC_DLLEXPORT_DEF uintptr_t MCObjcObjectGetActionProxySelector(void)
{
    return (uintptr_t)sel_registerName("action:");
}

////////////////////////////////////////////////////////////////////////////////

#if defined(__MAC__)
#include <AppKit/AppKit.h>
#else
#import <Foundation/NSInvocation.h>
#import <Foundation/NSMethodSignature.h>
#endif
#include <objc/runtime.h>

@interface com_livecode_MCObjcDelegate: NSObject
{
    MCArrayRef m_handlers;
    MCValueRef m_context;
}

- (id)initWithHandlerMapping:(MCArrayRef)aHandlerMapping withContext:(MCValueRef)aContext;
- (void)dealloc;

- (BOOL)respondsToSelector:(SEL)aSelector;

@end

@interface com_livecode_MCObjcFormalDelegate: com_livecode_MCObjcDelegate
{
    Protocol *m_protocol;
}

- (id)initForProtocol:(Protocol *)aProtocol withHandlerMapping:(MCArrayRef)aHandlerMapping withContext:(MCValueRef)aContext;
- (void)dealloc;

@end

@interface com_livecode_MCObjcInformalDelegate: com_livecode_MCObjcDelegate
{
    MCArrayRef m_protocol;
}

- (id)initForProtocol:(MCArrayRef)aProtocol withHandlerMapping:(MCArrayRef)aHandlerMapping withContext:(MCValueRef)aContext;
- (void)dealloc;

@end

static bool _get_type_from_code(char p_char, MCTypeInfoRef& r_type)
{
#define CASE(ctype, lctype) \
if (p_char == @encode(ctype)[0]) \
{ \
r_type = kMC##lctype##TypeInfo; \
return true; \
} \
else
    CASE(char, CSChar);
    CASE(unsigned char, CUChar);
    CASE(short, CSShort);
    CASE(unsigned short, CUShort);
    CASE(int, CSInt);
    CASE(unsigned int, CUInt);
    CASE(long, CSLong);
    CASE(unsigned long, CULong);
    CASE(long long, CSLongLong);
    CASE(unsigned long long, CULongLong);
    CASE(float, Float);
    CASE(double, Double);
    CASE(bool, CBool);
    CASE(char *, Pointer);
#undef CASE
    if (p_char == '@' ||
        p_char == '#')
    {
        r_type = kMCObjcObjectTypeInfo;
        return true;
    }
    else if (p_char == ':')
    {
        r_type = kMCStringTypeInfo;
        return true;
    }
    else if (p_char == 'v')
    {
        r_type = kMCNullTypeInfo;
        return true;
    }
    else
    {
        MCLog("unknown type encoding %c", p_char);
        return false;
    }
}

static bool __MCObjcMapTypeCode(const char *p_typecode, uindex_t& r_length, MCTypeInfoRef& r_type)
{
    MCTypeInfoRef t_type;
    if (!_get_type_from_code(*p_typecode, t_type))
        return false;
    
    // Skip the size information
    uindex_t t_length = 1;
    while (t_length < strlen(p_typecode) && isdigit(*(p_typecode + t_length)))
        t_length++;
    r_type = t_type;
    r_length = t_length;
    return true;
}

static bool _convert_type(char p_type, void *p_buffer, MCValueRef& r_value)
{
#define CASE(ctype, lctype) \
if (p_type == @encode(ctype)[0]) \
{ \
    return MCForeignValueCreate(kMC##lctype##TypeInfo, p_buffer, (MCForeignValueRef&)r_value); \
} \
else
    CASE(char, CSChar);
    CASE(unsigned char, CUChar);
    CASE(short, CSShort);
    CASE(unsigned short, CUShort);
    CASE(int, CSInt);
    CASE(unsigned int, CUInt);
    CASE(long, CSLong);
    CASE(unsigned long, CULong);
    CASE(long long, CSLongLong);
    CASE(unsigned long long, CULongLong);
    CASE(float, Float);
    CASE(double, Double);
    CASE(bool, CBool);
    CASE(char *, Pointer);
#undef CASE
    if (p_type == '@' ||
        p_type == '#')
    {
        if (!MCObjcObjectCreateWithId(*(void **)p_buffer, (MCObjcObjectRef&)r_value))
        {
            return false;
        }
    }
    else if (p_type == ':')
    {
        if (!MCStringCreateWithCString(sel_getName(*(SEL *)p_buffer), (MCStringRef&)r_value))
        {
            return false;
        }
    }
    else if (p_type == 'v')
    {
        r_value = MCValueRetain(kMCNull);
    }
    else
    {
        MCLog("unknown type encoding %c", p_type);
        return false;
    }
    return true;
}

static bool _get_method_description(Protocol *p_protocol, SEL p_selector, BOOL p_instance, objc_method_description& r_desc)
{
    objc_method_description t_desc =
    protocol_getMethodDescription(p_protocol, p_selector, NO, p_instance);
    if (t_desc.types == nullptr)
        t_desc = protocol_getMethodDescription(p_protocol, p_selector, YES, p_instance);
    
    if (t_desc.types == nullptr)
        return false;
    
    r_desc = t_desc;
    return true;
}

static const char *_get_code_from_type(MCTypeInfoRef p_typeinfo)
{
#define CASE(ctype, lctype) \
if (p_typeinfo == kMC##lctype##TypeInfo) \
{ \
    return @encode(ctype); \
} \
else
    CASE(char, CSChar);
    CASE(unsigned char, CUChar);
    CASE(short, CSShort);
    CASE(unsigned short, CUShort);
    CASE(int, CSInt);
    CASE(unsigned int, CUInt);
    CASE(long, CSLong);
    CASE(unsigned long, CULong);
    CASE(long long, CSLongLong);
    CASE(unsigned long long, CULongLong);
    CASE(float, Float);
    CASE(double, Double);
    CASE(bool, CBool);
#undef CASE
    if (p_typeinfo == kMCObjcIdTypeInfo ||
        p_typeinfo == kMCObjcRetainedIdTypeInfo ||
        p_typeinfo == kMCObjcAutoreleasedIdTypeInfo)
    {
        return @encode(id);
    }
    else if (p_typeinfo == kMCPointerTypeInfo)
    {
        return "^?";
    }
    else
    {
        MCLog("unknown encoding for type %@", p_typeinfo);
        return nullptr;
    }
}

static bool _get_code_string(const char *p_code, size_t p_size, MCStringRef& r_code)
{
    return MCStringFormat(r_code, "%s%d", p_code, p_size);
}

static bool _get_code_string_from_type(MCTypeInfoRef p_typeinfo, MCStringRef& r_code)
{
    if (p_typeinfo == kMCNullTypeInfo)
    {
        r_code = MCSTR("v");
        return true;
    }
    
    MCResolvedTypeInfo t_resolved;
    if (!MCTypeInfoResolve(p_typeinfo, t_resolved))
        return false;
    
    if (!MCTypeInfoIsForeign(t_resolved.type))
        return false;
    
    const MCForeignTypeDescriptor *t_descriptor =
        MCForeignTypeInfoGetDescriptor(t_resolved.type);
    
    const char *t_code = _get_code_from_type(p_typeinfo);
    if (t_code == nullptr)
        return false;
    
    return _get_code_string(t_code, t_descriptor->size, r_code);
}

static bool _append_code_string(MCStringRef x_target, const char *p_code, size_t p_size)
{
    return MCStringAppendFormat(x_target,"%s%d", p_code, p_size);
}

static bool _get_informal_protocol_method_types(MCStringRef p_selector, MCArrayRef p_protocol, MCStringRef& r_types)
{
    MCNewAutoNameRef t_key;
    if (!MCNameCreate(p_selector, &t_key))
        return false;
    
    MCValueRef t_handler;
    if (!MCArrayFetchValue(p_protocol, false, *t_key, t_handler))
        return false;
    
    MCTypeInfoRef t_typeinfo = MCValueGetTypeInfo(t_handler);
    if (!MCHandlerTypeInfoIsForeign(t_typeinfo))
        return false;
    
    MCAutoStringRef t_types;
    if (!MCStringCreateMutable(0, &t_types))
        return false;
    
    MCTypeInfoRef t_param_typeinfo = MCHandlerTypeInfoGetReturnType(t_typeinfo);
    
    MCAutoStringRef t_return_type;
    if (!_get_code_string_from_type(t_param_typeinfo, &t_return_type))
        return false;
    
    if (!MCStringAppend(*t_types, *t_return_type))
        return false;
    
    if (!_append_code_string(*t_types, @encode(id), sizeof(id)))
        return false;
    
    if (!_append_code_string(*t_types, @encode(SEL), sizeof(SEL)))
        return false;
    
    // IMPORTANT: We only allow instance methods in the mapping, so the first
    // parameter of the foreign handler typeinfo should be the target instance.
    // So we must skip this parameter when figuring out the obj-c method signature,
    // i.e. start at index 1
    for (uindex_t i = 1; i< MCHandlerTypeInfoGetParameterCount(t_typeinfo); ++i)
    {
        MCAutoStringRef t_param_type;
        if (!_get_code_string_from_type(MCHandlerTypeInfoGetParameterType(t_typeinfo, i),
                                        &t_param_type))
            return false;
        
        if (!MCStringAppend(*t_types, *t_param_type))
            return false;
    }
    
    return MCStringCopy(*t_types, r_types);
}

static MCHandlerRef _fetch_handler_for_selector(SEL p_selector, MCArrayRef p_mapping)
{
    MCAutoStringRef t_selector;
    if (!MCStringCreateWithCString(sel_getName(p_selector), &t_selector))
        return nullptr;
    
    MCNewAutoNameRef t_key;
    if (!MCNameCreate(*t_selector, &t_key))
        return nullptr;
    
    MCValueRef t_handler;
    if (!MCArrayFetchValue(p_mapping, false, *t_key, t_handler))
        return nullptr;
    
    return static_cast<MCHandlerRef>(t_handler);
}

@implementation com_livecode_MCObjcFormalDelegate
- (id)initForProtocol:(Protocol *)aProtocol withHandlerMapping:(MCArrayRef)aHandlerMapping withContext:(MCValueRef)aContext
{
    self = [super initWithHandlerMapping:aHandlerMapping withContext:aContext];
    if (self == nil)
        return nil;
    
    m_protocol = aProtocol;
    
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    // Return the method signature of the protocol method we have an LCB
    // implementation for. This causes forwardInvocation to be called.
    objc_method_description t_desc;
    if (!_get_method_description(m_protocol, aSelector, YES, t_desc) &&
        !_get_method_description(m_protocol, aSelector, NO, t_desc))
        return nullptr;
    
    return [NSMethodSignature signatureWithObjCTypes:t_desc.types];
}

@end

@implementation com_livecode_MCObjcInformalDelegate

- (id)initForProtocol:(MCArrayRef)aProtocol withHandlerMapping:(MCArrayRef)aHandlerMapping withContext:(MCValueRef)aContext
{
    self = [super initWithHandlerMapping:aHandlerMapping withContext:aContext];
    if (self == nil)
        return nil;
    
    m_protocol = MCValueRetain(aProtocol);
    
    return self;
}

- (void)dealloc
{
    MCValueRelease(m_protocol);
    [super dealloc];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    // Construct the type signature
    MCAutoStringRef t_selector;
    if (!MCStringCreateWithCString(sel_getName(aSelector), &t_selector))
        return nullptr;
    
    MCAutoStringRef t_types;
    if (!_get_informal_protocol_method_types(*t_selector, m_protocol, &t_types))
        return nullptr;
    
    MCAutoStringRefAsCString t_types_cstring;
    if (!t_types_cstring.Lock(*t_types))
        return nullptr;
    
    return [NSMethodSignature signatureWithObjCTypes:*t_types_cstring];
}

@end

@implementation com_livecode_MCObjcDelegate

- (id)initWithHandlerMapping:(MCArrayRef)aHandlerMapping withContext:(MCValueRef)aContext
{
    self = [super init];
    if (self == nil)
        return nil;
    
    m_handlers = MCValueRetain(aHandlerMapping);
    m_context = aContext != nullptr ? MCValueRetain(aContext) : aContext;
    
    return self;
}

- (void)dealloc
{
    MCValueRelease(m_handlers);
    MCValueRelease(m_context);
    [super dealloc];
}

-(MCHandlerRef)handlerFromSelector:(SEL)aSelector
{
    return _fetch_handler_for_selector(aSelector, m_handlers);
}

-(BOOL)respondsToSelector:(SEL)aSelector
{
    // Check the handler mapping to see if this delegate wants to handle
    // the incoming selector
    if ([self handlerFromSelector:aSelector] != nullptr)
        return YES;
    
    return [super respondsToSelector:aSelector];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    // Never forward the message to an instance of the base class
    return nil;
}

- (void)forwardInvocation:(NSInvocation *)anInvocation
{
    NSMethodSignature *t_signature = [anInvocation methodSignature];
    
    MCHandlerRef t_handler = [self handlerFromSelector:[anInvocation selector]];
    if (t_handler == nullptr)
        return;
    
    MCAutoProperListRef t_msg_arguments;
    if (!MCProperListCreateMutable(&t_msg_arguments))
    {
        return;
    }
    
    // Add the context as the first param if it was specified
    if (m_context != nullptr &&
        !MCProperListPushElementOntoBack(*t_msg_arguments, m_context))
    {
        return;
    }
    
    // Box the incoming params and put into a proper list. The first two args
    // are the handling obj-c id and selector, so start at index 2.
    for(NSUInteger t_arg_index = 2; t_arg_index < [t_signature numberOfArguments]; t_arg_index++)
    {
        const char *t_arg_type = [t_signature getArgumentTypeAtIndex:t_arg_index];
        
        NSUInteger t_arg_size, t_arg_align;
        NSGetSizeAndAlignment(t_arg_type, &t_arg_size, &t_arg_align);
        
        char t_raw_value[t_arg_size];
        [anInvocation getArgument:t_raw_value atIndex:t_arg_index];
        
        char t_typecode = t_arg_type[0];
        // Special-case signed char / bool conversion to handle BOOL type
        // which is signed char on mac and bool on ios
        if (MCHandlerTypeInfoGetParameterType(MCValueGetTypeInfo(t_handler), t_arg_index - 2)
            == kMCCBoolTypeInfo && t_arg_type[0] == 'c')
        {
            t_typecode = 'B';
        }
        
        MCAutoValueRef t_arg_value;
        if (!_convert_type(t_typecode, t_raw_value, &t_arg_value))
        {
            return;
        }
        
        if (!MCProperListPushElementOntoBack(*t_msg_arguments, *t_arg_value))
        {
            return;
        }
    }
    
    MCProperListRef t_mutable_list;
    if (!MCProperListMutableCopy(*t_msg_arguments, t_mutable_list))
        return;
    
    MCAutoValueRef t_result;
    MCErrorRef t_error =
        MCHandlerTryToExternalInvokeWithList(t_handler, t_mutable_list, &t_result);
    
    MCValueRelease(t_mutable_list);
    
    if (t_error != nil)
    {
        MCErrorThrow(t_error);
        return;
    }
    
    // We have already checked type conformance at bind time
    // Firstly, if the return value is already a foreign value, just set the
    // invocation return value to its contents ptr
    if (MCValueGetTypeCode(*t_result) == kMCValueTypeCodeForeignValue)
    {
        [anInvocation setReturnValue:MCForeignValueGetContentsPtr(*t_result)];
        return;
    }
    
    // Otherwise fetch the required foreign typeinfo, and export the value ref
    // to a foreign value ptr
    const char *t_return_type = [t_signature methodReturnType];
    MCTypeInfoRef t_typeinfo;
    if (!_get_type_from_code(t_return_type[0], t_typeinfo))
    {
        MCLog("can't convert return type to %s", t_return_type[0]);
        return;
    }
    
    // If void return, we are done
    if (t_typeinfo == kMCNullTypeInfo)
        return;
    
    MCResolvedTypeInfo t_resolved_type;
    if (!MCTypeInfoResolve(t_typeinfo, t_resolved_type))
        return;
    
    if (!MCTypeInfoIsForeign(t_resolved_type.type))
    {
        MCLog("can't return non-foreign type");
        return;
    }
    
    const MCForeignTypeDescriptor *t_descriptor =
        MCForeignTypeInfoGetDescriptor(t_typeinfo);
    
    MCAutoBlock<void> t_contents;
    if (!t_contents.Allocate(t_descriptor->size))
        return;
    
    if (!t_descriptor->doexport(t_descriptor, *t_result, false, *t_contents))
        return;
    
    [anInvocation setReturnValue:*t_contents];
}
@end

static bool __MCTypeInfoConformsToObjcTypeCode(MCTypeInfoRef p_typeinfo, const char*& x_desc)
{
    MCTypeInfoRef t_expected;
    uindex_t t_length;
    if (!__MCObjcMapTypeCode(x_desc, t_length, t_expected))
        return false;
    
    bool t_conforms = false;
    // Special-case signed char / bool conformance to handle BOOL type
    // which is signed char on mac and bool on ios
    if (t_expected == kMCCSCharTypeInfo && p_typeinfo == kMCCBoolTypeInfo)
    {
        t_conforms = true;
    }
    else
    {
        MCResolvedTypeInfo t_src, t_target;
        if (!MCTypeInfoResolve(t_expected, t_src))
            return false;
        
        if (!MCTypeInfoResolve(p_typeinfo, t_target))
            return false;
        
        t_conforms = MCResolvedTypeInfoConforms(t_src, t_target);
    }
    
    if (!t_conforms)
        return false;
    
    x_desc += t_length;
    return true;
}

static bool __HandlerConformsToMethodSignature(MCHandlerRef p_handler, const char* p_typecodes, bool p_is_instance, MCValueRef p_context)
{
    /* TODO: Check how things work with protocol class methods */
    
    MCTypeInfoRef t_handler_typeinfo = MCValueGetTypeInfo(p_handler);
    
    // Check return type, the first code in the signature
    bool t_handler_conforms = true;
    
    MCTypeInfoRef t_return_type = MCHandlerTypeInfoGetReturnType(t_handler_typeinfo);
    if (t_handler_conforms)
    {
        t_handler_conforms =
        __MCTypeInfoConformsToObjcTypeCode(t_return_type, p_typecodes);
    }
    
    // The next two types are always id (self) and SEL (the selector)
    if (t_handler_conforms)
    {
        t_handler_conforms =
        __MCTypeInfoConformsToObjcTypeCode(kMCObjcObjectTypeInfo, p_typecodes);
    }
    if (t_handler_conforms)
    {
        t_handler_conforms =
        __MCTypeInfoConformsToObjcTypeCode(kMCStringTypeInfo, p_typecodes);
    }
    
    // Now check all the parameter types.
    uindex_t t_param_count = MCHandlerTypeInfoGetParameterCount(t_handler_typeinfo);
    
    // Skip the first param if we have a context
    for (uindex_t i = p_context != nullptr ? 1 : 0;
         t_handler_conforms && i < t_param_count;
         ++i)
    {
        MCTypeInfoRef t_param_typeinfo =
        MCHandlerTypeInfoGetParameterType(t_handler_typeinfo, i);
        
        if (t_handler_conforms)
        {
            t_handler_conforms =
            __MCTypeInfoConformsToObjcTypeCode(t_param_typeinfo, p_typecodes);
        }
    }
    
    // Ensure all the parameters required by the typecode are present
    if (t_handler_conforms)
        t_handler_conforms = strlen(p_typecodes) == 0;
    
    if (!t_handler_conforms)
    {
        return MCErrorCreateAndThrowWithMessage(kMCObjcDelegateCallbackSignatureErrorTypeInfo,
                                                MCSTR("Callback handler %{handler} parameters must conform to protocol method types"),
                                                "handler", p_handler,
                                                nullptr);
    }
    
    return true;
}

static bool __HandlerConformsToInformalProtocolMethod(MCHandlerRef p_handler, MCNameRef p_selector, MCValueRef p_context, MCArrayRef p_protocol)
{
    MCAutoStringRef t_types;
    if (!_get_informal_protocol_method_types(MCNameGetString(p_selector), p_protocol, &t_types))
    {
        return MCErrorCreateAndThrowWithMessage(kMCObjcDelegateMappingErrorTypeInfo,
                                                MCSTR("Invalid protocol definition for selector %{sel}"),
                                                "sel", p_selector,
                                                nullptr);
    }
    
    MCAutoStringRefAsCString t_typecodes;
    if (!t_typecodes.Lock(*t_types))
        return false;
    
    return __HandlerConformsToMethodSignature(p_handler, *t_typecodes, false, p_context);
}

static bool __HandlerConformsToProtocolMethod(MCHandlerRef p_handler, MCStringRef t_method, MCValueRef p_context, Protocol *p_protocol)
{
    MCAutoStringRefAsCFString t_selector_cfstring;
    if (!t_selector_cfstring.Lock(t_method))
        return false;
    
    SEL t_proxy_selector = NSSelectorFromString((NSString *)*t_selector_cfstring);
    
    bool t_is_instance = false;
    objc_method_description t_desc;
    if (!_get_method_description(p_protocol, t_proxy_selector, YES, t_desc))
        t_is_instance = true;
    
    if (t_is_instance && !_get_method_description(p_protocol, t_proxy_selector, NO, t_desc))
    {
        return MCErrorCreateAndThrowWithMessage(kMCObjcDelegateMappingErrorTypeInfo,
                                                MCSTR("Protocol method %{method} not found"),
                                                "method", t_method,
                                                nullptr);
    }
    
    return __HandlerConformsToMethodSignature(p_handler, t_desc.types, t_is_instance, p_context);
}

static bool __AllRequiredProtocolMethodsImplemented(MCArrayRef p_handler_mapping, BOOL p_instance, Protocol *p_protocol)
{
    uindex_t t_out_count = 0;
    objc_method_description *t_required_methods =
    protocol_copyMethodDescriptionList(p_protocol, YES, p_instance, &t_out_count);
    for (uindex_t i = 0; i < t_out_count; ++i)
    {
        if (_fetch_handler_for_selector(t_required_methods->name,
                                        p_handler_mapping) == nullptr)
        {
            MCAutoStringRef t_selector;
            if (!MCStringCreateWithCString(sel_getName(t_required_methods->name), &t_selector))
                return false;
            
            return MCErrorCreateAndThrowWithMessage(kMCObjcDelegateMappingErrorTypeInfo,
                                                    MCSTR("No mapping found for required protocol method %{method}"),
                                                    "method", *t_selector,
                                                    nullptr);
        }
        t_required_methods++;
    }
    return true;
}

extern "C" MC_DLLEXPORT_DEF
bool MCObjcCreateDelegateWithContext(MCStringRef p_protocol_name, MCArrayRef p_handler_mapping, MCValueRef p_context, MCObjcObjectRef& r_object)
{
    MCAutoStringRefAsCString t_protocol_cstring;
    if (!t_protocol_cstring.Lock(p_protocol_name))
        return false;
    
    // Get protocol object
    Protocol *t_protocol = objc_getProtocol(*t_protocol_cstring);
    if (t_protocol == nullptr)
    {
        return MCErrorCreateAndThrowWithMessage(kMCObjcDelegateCallbackSignatureErrorTypeInfo,
                                                MCSTR("Protocol %{protocol} not found"),
                                                "protocol", p_protocol_name,
                                                nullptr);
    }
    
    MCNameRef t_key;
    MCValueRef t_value;
    uintptr_t t_iterator = 0;
    while (MCArrayIterate(p_handler_mapping, t_iterator, t_key, t_value))
    {
        if (!__HandlerConformsToProtocolMethod(static_cast<MCHandlerRef>(t_value),
                                               MCNameGetString(t_key),
                                               p_context,
                                               t_protocol))
            return false;
    }
    
    // Check all the required protocol methods have callbacks implemented
    if (!__AllRequiredProtocolMethodsImplemented(p_handler_mapping, YES, t_protocol))
        return false;
    if (!__AllRequiredProtocolMethodsImplemented(p_handler_mapping, NO, t_protocol))
        return false;
    
    com_livecode_MCObjcFormalDelegate *t_proxy =
    [[com_livecode_MCObjcFormalDelegate alloc] initForProtocol: t_protocol withHandlerMapping:p_handler_mapping withContext:p_context];
    
    MCObjcObjectRef t_proxy_object = nullptr;
    if (!MCObjcObjectCreateWithRetainedId(t_proxy, t_proxy_object))
    {
        [t_proxy release];
        return false;
    }
    
    r_object = t_proxy_object;
    return true;
}

extern "C" MC_DLLEXPORT_DEF
bool MCObjcCreateDelegate(MCStringRef p_protocol_name, MCArrayRef p_handler_mapping, MCObjcObjectRef& r_object)
{
    return MCObjcCreateDelegateWithContext(p_protocol_name,
                                           p_handler_mapping,
                                           nullptr,
                                           r_object);
}

extern "C" MC_DLLEXPORT_DEF
bool MCObjcCreateInformalDelegateWithContext(MCProperListRef p_foreign_handlers, MCArrayRef p_handler_mapping, MCValueRef p_context, MCObjcObjectRef& r_object)
{
    MCAutoArrayRef t_selector_mapping;
    if (!MCArrayCreateMutable(&t_selector_mapping))
        return false;
    
    for (uindex_t i = 0; i < MCProperListGetLength(p_foreign_handlers); ++i)
    {
        MCValueRef t_element =
            MCProperListFetchElementAtIndex(p_foreign_handlers, i);
        
        if (MCValueGetTypeCode(t_element) != kMCValueTypeCodeHandler)
            return false;
        
        MCHandlerRef t_handler = static_cast<MCHandlerRef>(t_element);
        
        const MCHandlerCallbacks *t_callbacks = MCHandlerGetCallbacks(t_handler);
        if (t_callbacks->query == nullptr)
            return false;
        
        SEL t_selector = nullptr;
        if (!t_callbacks->query(MCHandlerGetContext(t_handler),
                                kMCHandlerQueryTypeObjcSelector,
                                &t_selector))
            return false;
        
        if (t_selector == nullptr)
            return false;
        
        NSString *t_sel_string = NSStringFromSelector(t_selector);
        MCAutoStringRef t_sel_stringref;
        if (!MCStringCreateWithCFStringRef((CFStringRef)t_sel_string, &t_sel_stringref))
            return false;
        
        MCNewAutoNameRef t_key;
        if (!MCNameCreate(*t_sel_stringref, &t_key))
            return false;
        
        if (!MCArrayStoreValue(*t_selector_mapping, false, *t_key, t_element))
            return false;
    }

    MCAutoArrayRef t_selector_mapping_final;
    if (!MCArrayCopy(*t_selector_mapping, &t_selector_mapping_final))
        return false;
    
    MCNameRef t_key;
    MCValueRef t_value;
    uintptr_t t_iterator = 0;
    while (MCArrayIterate(p_handler_mapping, t_iterator, t_key, t_value))
    {
        if (!__HandlerConformsToInformalProtocolMethod(static_cast<MCHandlerRef>(t_value),
                                                       t_key,
                                                       p_context,
                                                       *t_selector_mapping_final))
            return false;
    }
    
    com_livecode_MCObjcInformalDelegate *t_proxy =
    [[com_livecode_MCObjcInformalDelegate alloc]
        initForProtocol: *t_selector_mapping_final
        withHandlerMapping:p_handler_mapping
        withContext:p_context];
    
    MCObjcObjectRef t_proxy_object = nullptr;
    if (!MCObjcObjectCreateWithRetainedId(t_proxy, t_proxy_object))
    {
        [t_proxy release];
        return false;
    }
    
    r_object = t_proxy_object;
    return true;
}

extern "C" MC_DLLEXPORT_DEF
bool MCObjcCreateInformalDelegate(MCProperListRef p_foreign_handlers, MCArrayRef p_handler_mapping, MCObjcObjectRef& r_object)
{
    return MCObjcCreateInformalDelegateWithContext(p_foreign_handlers,
                                                   p_handler_mapping,
                                                   nullptr,
                                                   r_object);
}

////////////////////////////////////////////////////////////////////////////////

MCTypeInfoRef kMCObjcDelegateCallbackSignatureErrorTypeInfo;
MCTypeInfoRef kMCObjcDelegateMappingErrorTypeInfo;

bool MCObjcErrorsInitialize()
{
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.objc.DelegateCallbackSignatureError"), MCNAME("objc"), MCSTR("Could not create delegate"), kMCObjcDelegateCallbackSignatureErrorTypeInfo))
        return false;
    
    if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.objc.DelegateMappingError"), MCNAME("objc"), MCSTR("Could not create delegate"), kMCObjcDelegateMappingErrorTypeInfo))
        return false;
    
    return true;
}

void MCObjcErrorsFinalize()
{
    MCValueRelease(kMCObjcDelegateCallbackSignatureErrorTypeInfo);
}


bool __MCObjcInitialize(void)
{
    if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.objc.ObjcObject"),
                                     kMCNullTypeInfo,
                                     &kMCObjcObjectCustomValueCallbacks,
                                     kMCObjcObjectTypeInfo))
    {
        return false;
    }
    
    MCForeignPrimitiveType p = kMCForeignPrimitiveTypePointer;
    
    MCForeignTypeDescriptor d =
    {
        sizeof(id),
        kMCNullTypeInfo,
        kMCObjcObjectTypeInfo,
        &p,
        1,
        objc_id_initialize, /* initialize */
        objc_id_finalize, /* finalize */
        objc_id_defined, /* defined */
        objc_id_move,
        objc_id_copy,
        objc_id_equal,
        objc_id_hash,
        objc_id_import,
        objc_id_export,
        objc_id_describe,
        
        kMCNullTypeInfo,
        nullptr, /* promote */
    };
    if (!MCNamedForeignTypeInfoCreate(MCNAME("com.livecode.objc.Id"),
                                      &d,
                                      kMCObjcIdTypeInfo))
    {
        return false;
    }
    
    d.doimport = objc_retained_id_import;
    d.doexport = objc_retained_id_export;
    d.describe = objc_retained_id_describe;
    if (!MCNamedForeignTypeInfoCreate(MCNAME("com.livecode.objc.RetainedId"),
                                      &d,
                                      kMCObjcRetainedIdTypeInfo))
    {
        return false;
    }
    
    d.doimport = objc_autoreleased_id_import;
    d.doexport = objc_autoreleased_id_export;
    d.describe = objc_autoreleased_id_describe;
    if (!MCNamedForeignTypeInfoCreate(MCNAME("com.livecode.objc.AutoreleasedId"),
                                      &d,
                                      kMCObjcAutoreleasedIdTypeInfo))
    {
        return false;
    }
    
    if (!MCObjcErrorsInitialize())
        return false;
    
    return true;
}

void __MCObjcFinalize(void)
{
    MCValueRelease(kMCObjcIdTypeInfo);
    MCValueRelease(kMCObjcRetainedIdTypeInfo);
    MCValueRelease(kMCObjcAutoreleasedIdTypeInfo);
    MCValueRelease(kMCObjcObjectTypeInfo);
    
    MCObjcErrorsFinalize();
}

extern "C" bool com_livecode_objc_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_objc_Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

NSString *MCStringConvertToAutoreleasedNSString(MCStringRef p_string_ref)
{
	CFStringRef t_string;
	/* UNCHECKED */ MCStringConvertToCFStringRef(p_string_ref, t_string);
	return [((NSString *)t_string) autorelease];
}

NSString *MCNameConvertToAutoreleasedNSString(MCNameRef p_name_ref)
{
	CFStringRef t_string;
	/* UNCHECKED */ MCStringConvertToCFStringRef(MCNameGetString(p_name_ref), t_string);
	return [((NSString *)t_string) autorelease];
}

NSData *MCDataConvertToAutoreleasedNSData(MCDataRef p_data_ref)
{
	CFDataRef t_data;
	/* UNCHECKED */ MCDataConvertToCFDataRef(p_data_ref, t_data);
	return [((NSData *)t_data) autorelease];
}

////////////////////////////////////////////////////////////////////////////////

void MCCFAutorelease(const void *p_object)
{
    // CFAutorelease isn't exposed until MacOSX 10.9
    id t_object = (id)p_object;
    [t_object autorelease];
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

