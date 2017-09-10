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
        nullptr, /* initialize */
        objc_id_finalize, /* finalize */
        nullptr, /* defined */
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
    
    return true;
}

void __MCObjcFinalize(void)
{
    MCValueRelease(kMCObjcIdTypeInfo);
    MCValueRelease(kMCObjcRetainedIdTypeInfo);
    MCValueRelease(kMCObjcAutoreleasedIdTypeInfo);
    MCValueRelease(kMCObjcObjectTypeInfo);
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

