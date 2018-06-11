/*                                                                     -*-c++-*-
Copyright (C) 2017 LiveCode Ltd.

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

#include "system-private.h"

#include <foundation-auto.h>

/* ---------------------------------------------------------------- */

/* The MCSLibraryRef custom typeinfo. */
MCTypeInfoRef kMCSLibraryTypeInfo = nullptr;

/* The MCSLibraryCouldNotLoadError error. */
MCErrorRef kMCSLibraryCouldNotLoadError = nullptr;

/* The native lib path on Android. */
#if defined(__ANDROID__)
static MCStringRef sMCSLibraryAndroidNativeLibPath = nullptr;
#endif

/* ================================================================
 * Types
 * ================================================================ */

#if defined(__WINDOWS__)
#include "system-library-w32.hpp"
#elif defined(__MAC__)
#include "system-library-mac.hpp"
#elif defined(__IOS__)
#include "system-library-ios.hpp"
#elif defined(__LINUX__)
#include "system-library-linux.hpp"
#elif defined(__ANDROID__)
#include "system-library-android.hpp"
#elif defined(__EMSCRIPTEN__)
#include "system-library-emscripten.hpp"
#endif

class __MCSLibraryImpl
{
public:
    __MCSLibraryImpl(void)
    {
    }
    
    ~__MCSLibraryImpl(void)
    {
    }
    
    bool operator == (const __MCSLibraryImpl& p_other) const
    {
        MCAssert(IsDefined());
        return m_handle.IsEqualTo(p_other.m_handle);
    }
    
    //////////
    
    bool IsDefined(void) const
    {
        return m_handle.IsDefined();
    }
    
    hash_t Hash(void) const
    {
        return m_handle.Hash();
    }
    
    bool CreateWithNativePath(MCStringRef p_native_path)
    {
        MCAssert(!IsDefined());
        return m_handle.CreateWithNativePath(p_native_path);
    }
    
    bool CreateWithAddress(void *p_address)
    {
        MCAssert(!IsDefined());
        return m_handle.CreateWithAddress(p_address);
    }
    
    bool CopyNativePath(MCStringRef& r_native_path) const
    {
        MCAssert(IsDefined());
        return m_handle.CopyNativePath(r_native_path);
    }
    
    void *LookupSymbol(MCStringRef p_symbol) const
    {
        MCAssert(IsDefined());
        return m_handle.LookupSymbol(p_symbol);
    }
    
private:
    __MCSLibraryHandle m_handle;
};

static inline __MCSLibraryImpl&
__MCSLibraryGetImpl(MCSLibraryRef p_library)
{
    return *(__MCSLibraryImpl *)MCValueGetExtraBytesPtr(p_library);
}

static inline __MCSLibraryImpl&
__MCSLibraryGetImpl(MCValueRef p_value)
{
    MCAssert(MCValueGetTypeInfo(p_value) == kMCSLibraryTypeInfo);
    
    return *(__MCSLibraryImpl *)MCValueGetExtraBytesPtr(p_value);
}

static void
__MCSLibraryDestroy(MCValueRef p_value)
{
    __MCSLibraryGetImpl(p_value).~__MCSLibraryImpl();
}

static bool
__MCSLibraryCopy(MCValueRef p_value,
                 bool p_release,
                 MCValueRef& r_copied_value)
{
    if (p_release)
        r_copied_value = p_value;
    else
        r_copied_value = MCValueRetain(p_value);
    return true;
}

static bool
__MCSLibraryEqual(MCValueRef p_left,
                  MCValueRef p_right)
{
    return __MCSLibraryGetImpl(p_left) == __MCSLibraryGetImpl(p_right);
}

static hash_t
__MCSLibraryHash(MCValueRef p_value)
{
    return __MCSLibraryGetImpl(p_value).Hash();
}

static bool
__MCSLibraryDescribe(MCValueRef p_value,
                     MCStringRef& r_description)
{
    return false;
}

static MCValueCustomCallbacks
__kMCSLibraryCallbacks =
{
    false,
    __MCSLibraryDestroy,
    __MCSLibraryCopy,
    __MCSLibraryEqual,
    __MCSLibraryHash,
    __MCSLibraryDescribe,
    nullptr,
    nullptr,
};

bool
__MCSLibraryInitialize(void)
{
    if (!MCNamedCustomTypeInfoCreate(MCNAME("livecode.system.Library"),
                                     kMCNullTypeInfo,
                                     &__kMCSLibraryCallbacks,
                                     kMCSLibraryTypeInfo))
    {
        return false;
    }
    
    #if defined(__ANDROID__)
    sMCSLibraryAndroidNativeLibPath = nullptr;
    #endif
    
    return true;
}

void
__MCSLibraryFinalize(void)
{
#if defined(__ANDROID__)
    MCValueRelease(sMCSLibraryAndroidNativeLibPath);
    sMCSLibraryAndroidNativeLibPath = nil;
#endif
    
    MCValueRelease(kMCSLibraryTypeInfo);
}

static bool
__MCSLibraryCreate(MCSLibraryRef& r_library)
{
    MCSLibraryRef t_library;
    if (!MCValueCreateCustom(kMCSLibraryTypeInfo,
                             sizeof(__MCSLibraryImpl),
                             t_library))
    {
        return false;
    }
    
    new (MCValueGetExtraBytesPtr(t_library)) __MCSLibraryImpl();
    
    r_library = t_library;
    
    return true;
}

/* ================================================================
 * Errors
 * ================================================================ */

/* Thrown if the attempt to load the library on the given path failed. */
bool
__MCSLibraryThrowCreateWithNativePathFailed(MCStringRef p_native_path)
{
    return false;
}

/* Thrown if the attempt to load the library at the given address failed. */
bool
__MCSLibraryThrowCreateWithAddressFailed(void *p_address)
{
    return false;
}

/* Thrown if the attempt to compute a library's path failed */
bool
__MCSLibraryThrowResolvePathFailed(void)
{
    return false;
}

/* ================================================================
 * Public API
 * ================================================================ */

MC_DLLEXPORT_DEF bool
MCSLibraryCreateWithPath(MCStringRef p_path,
                         MCSLibraryRef& r_library)
{
    MCAutoStringRef t_native_path;
    if (!__MCSFilePathToNative(p_path,
                               &t_native_path))
    {
        return false;
    }
    
    MCSAutoLibraryRef t_new_library;
    if (!__MCSLibraryCreate(&t_new_library))
    {
        return false;
    }
    
    if (!__MCSLibraryGetImpl(*t_new_library).CreateWithNativePath(*t_native_path))
    {
        return false;
    }
    
    r_library = t_new_library.Take();
    
    return true;
}

MC_DLLEXPORT_DEF bool
MCSLibraryCreateWithAddress(void *p_address,
                            MCSLibraryRef& r_library)
{
    MCSAutoLibraryRef t_new_library;
    if (!__MCSLibraryCreate(&t_new_library))
    {
        return false;
    }
    
    if (!__MCSLibraryGetImpl(*t_new_library).CreateWithAddress(p_address))
    {
        return false;
    }
    
    r_library = t_new_library.Take();
    
    return true;
}

MC_DLLEXPORT_DEF bool
MCSLibraryCopyNativePath(MCSLibraryRef p_library,
                         MCStringRef& r_path)
{
    return __MCSLibraryGetImpl(p_library).CopyNativePath(r_path);
}

MC_DLLEXPORT_DEF bool
MCSLibraryCopyPath(MCSLibraryRef p_library,
                   MCStringRef& r_path)
{
    MCAutoStringRef t_native_path;
    if (!__MCSLibraryGetImpl(p_library).CopyNativePath(&t_native_path))
    {
        return false;
    }
    
    return __MCSFilePathFromNative(*t_native_path,
                                   r_path);
}

MC_DLLEXPORT_DEF void *
MCSLibraryLookupSymbol(MCSLibraryRef p_library,
                       MCStringRef p_symbol)
{
    return __MCSLibraryGetImpl(p_library).LookupSymbol(p_symbol);
}

#if defined(__ANDROID__)
MC_DLLEXPORT_DEF void
MCSLibraryAndroidSetNativeLibPath(MCStringRef p_path)
{
    MCValueAssign(sMCSLibraryAndroidNativeLibPath,
                  p_path);
}
#endif
