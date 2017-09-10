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

#if TARGET_IPHONE_SIMULATOR

/* ================================================================
 * iOS Simulator Handle Class
 * ================================================================ */

/* The iOS simulator is able to load arbitrary loadable libraries and therefore
 * the same library handle strategy as Mac can be used. */

#include "system-library-mac.hpp"

class __MCSLibraryHandleIOSSimulator: public __MCSLibraryHandleMac
{
};

typedef class __MCSLibraryHandleIOSSimulator __MCSLibraryHandle;

#else

/* ================================================================
 * iOS Device Handle Class
 * ================================================================ */

/* The iOS device is not able to load anything other than system loadable
 * libraries at runtime; all other code must be statically linked in. In order
 * to allow both types of library to be manipulated, the
 * __MCSLibraryHandleIOSDevice class can be either an __MCSLibraryHandleStatic
 * *or* an __MCSLibraryHandleMac instance. When creating with a native path,
 * static loading is tried first, and if that fails it falls back to dynamic
 * loading. When creating with an address, only the dynamic behavior is
 * tried - i.e. static libraries can not be resolved using an address within
 * them (indeed, it is unlikely this will ever make sense - if LTO is used on
 * an iOS binary, then parts of the static libraries linked into it could
 * be anywhere in the image, intermingled with other libraries). */

#include "system-library-static.hpp"
#include "system-library-mac.hpp"

class __MCSLibraryHandleIOSDevice
{
public:
    __MCSLibraryHandleIOSDevice(void)
        : m_type(kNone),
          m_static()
    {
    }
    
    ~__MCSLibraryHandleIOSDevice(void)
    {
        switch(m_type)
        {
            case kNone:
                break;
            case kStatic:
                m_static.~__MCSLibraryHandleStatic();
                break;
            case kDynamic:
                m_dynamic.~__MCSLibraryHandleMac();
                break;
            default:
                MCUnreachable();
                break;
        }
    }
    
    bool IsDefined(void) const
    {
        return m_type != kNone;
    }
    
    bool IsEqualTo(const __MCSLibraryHandleIOSDevice& p_other) const
    {
        if (m_type != p_other.m_type)
        {
            return false;
        }
        
        switch(m_type)
        {
            case kNone:
                return false;
            case kStatic:
                return m_static.IsEqualTo(p_other.m_static);
            case kDynamic:
                return m_dynamic.IsEqualTo(p_other.m_dynamic);
            default:
                MCUnreachableReturn(false);
                break;
        }
        
        return false;
    }

    hash_t Hash(void) const
    {
        switch(m_type)
        {
            case kNone:
                return MCHashPointer(nullptr);
            case kStatic:
                return m_static.Hash();
            case kDynamic:
                return m_dynamic.Hash();
            default:
                MCUnreachableReturn(hash_t());
                break;
        }
        
        return hash_t();
    }
    
    bool CreateWithNativePath(MCStringRef p_native_path)
    {
        MCAssert(m_type == kNone);
        if (m_static.CreateWithNativePath(p_native_path))
        {
            m_type = kStatic;
            return true;
        }
        
        if (m_dynamic.CreateWithNativePath(p_native_path))
        {
            m_type = kDynamic;
            return true;
        }
        
        return false;
    }
    
    bool CreateWithAddress(void *p_address)
    {
        MCAssert(m_type == kNone);
        if (m_dynamic.CreateWithAddress(p_address))
        {
            m_type = kDynamic;
            return true;
        }
        
        return false;
    }
    
    bool CopyNativePath(MCStringRef& r_native_path) const
    {
        switch(m_type)
        {
            case kNone:
                return false;
            case kStatic:
                return m_static.CopyNativePath(r_native_path);
            case kDynamic:
                return m_dynamic.CopyNativePath(r_native_path);
            default:
                MCUnreachableReturn(false);
                break;
        }
        return false;
    }
    
    void *LookupSymbol(MCStringRef p_symbol) const
    {
        switch(m_type)
        {
            case kNone:
                return nullptr;
            case kStatic:
                return m_static.LookupSymbol(p_symbol);
            case kDynamic:
                return m_dynamic.LookupSymbol(p_symbol);
            default:
                MCUnreachableReturn(nullptr);
                break;
        }
        return nullptr;
    }
    
private:
    enum HandleType
    {
        kNone,
        kStatic,
        kDynamic,
    };
    
    HandleType m_type;
    union
    {
        __MCSLibraryHandleStatic m_static;
        __MCSLibraryHandleMac m_dynamic;
    };
};

#if MCS_LIBRARY_NEW_IOS_STATIC_IMPL
MC_DLLEXPORT_DEF void
MCSLibraryRegisterStatic(MCSLibraryStaticInfo& p_info)
{
    __MCSLibraryHandleIOSDevice::Register(p_info);
}
#endif

typedef class __MCSLibraryHandleIOSDevice __MCSLibraryHandle;

#endif
