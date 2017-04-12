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

/* ================================================================
 * Static Handle Class -- new style (not yet implemented)
 * ================================================================ */

#if defined(MCS_LIBRARY_CXX_IOS_STATIC)

class __MCSLibraryHandleStatic
{
public:
    __MCSLibraryHandleStatic(void)
        : m_handle(nullptr)
    {
    }
    
    ~__MCSLibraryHandleStatic(void)
    {
    }
    
    bool IsDefined(void) const
    {
        return m_handle != nullptr;
    }
    
    bool IsEqualTo(const __MCSLibraryHandleStatic& p_other) const
    {
        return m_handle == p_other.m_handle;
    }
    
    bool Hash(void) const
    {
        return MCHashPointer(m_handle);
    }
    
    bool CreateWithNativePath(MCStringRef p_native_path)
    {
        for(MCSLibraryStaticInfo *t_info = s_chain;
            t_info != nullptr;
            t_info = t_info->__next)
        {
            if (MCStringIsEqualToCString(p_native_path,
                                         t_info->name,
                                         kMCStringOptionCompareCaseless))
            {
                m_handle = t_info;
                return true;
            }
        }
        
        return __MCSLibraryThrowCreateWithNativePathFailed(p_native_path);
    }
    
    bool CreateWithAddress(void *p_address)
    {
        return __MCSLibraryThrowCreateWithAddressFailed(p_address);
    }
    
    bool CopyNativePath(MCStringRef& r_native_path) const
    {
        return MCStringCreateWithCString(m_handle->name,
                                         r_native_path);
    }
    
    void *LookupSymbol(MCStringRef p_symbol) const
    {
        for(size_t i = 0; m_handle->exports[i].symbol != nullptr; i++)
        {
            if (MCStringIsEqualToCString(p_symbol,
                                         m_handle->exports[i].symbol,
                                         kMCStringOptionCompareExact))
            {
                return m_handle->exports[i].address;
            }
        }
        
        return nullptr;
    }
    
    static void Register(MCSLibraryStaticInfo& p_info)
    {
        // If the __next link is already set, then the library has already been
        // registered.
        if (p_info.__next != nullptr)
        {
            return;
        }
        
        p_info.__next = s_chain;
        s_chain = &p_info;
    }
    
protected:
    static MCSLibraryStaticInfo *s_chain;
    const MCSLibraryStaticInfo *m_handle;
};

MCSLibraryStaticInfo *__MCSLibraryHandleStatic::s_chain = nullptr;

#endif

/* ================================================================
 * Static Handle Class -- old style
 * ================================================================ */

#if !defined(MCS_LIBRARY_CXX_IOS_STATIC)

/* The __MCSLibraryHandleStatic class is used on platforms which don't allow
 * user-level dynamic linking (e.g. iOS). In this case, all included loadable
 * libraries are linked into the main executable with a pointer to a (static)
 * MCSLibraryStaticLibInfo structure being placed in the __DATA/__libs section.
 * When looking up static inclusions, the basename of the native path is used.
 */

#include <mach-o/getsect.h>
#include <mach-o/dyld.h>

class __MCSLibraryHandleStatic
{
    public:
    __MCSLibraryHandleStatic(void)
        : m_handle(nullptr)
    {
    }
    
    ~__MCSLibraryHandleStatic(void)
    {
    }
    
    bool IsDefined(void) const
    {
        return m_handle != nullptr;
    }
    
    bool IsEqualTo(const __MCSLibraryHandleStatic& p_other) const
    {
        return m_handle == p_other.m_handle;
    }
    
    bool Hash(void) const
    {
        return MCHashPointer(m_handle);
    }
    
    bool CreateWithNativePath(MCStringRef p_native_path)
    {
        // First process the native path to remove the engine folder prefix
        // and extension - all static libs are compiled in with just the
        // leaf name of the library.
        uindex_t t_last_separator = 0;
        if (MCStringLastIndexOfChar(p_native_path,
                                     '/',
                                     UINDEX_MAX,
                                     kMCStringOptionCompareExact,
                                     t_last_separator))
        {
            t_last_separator += 1;
        }
        
        uindex_t t_first_extension = 0;
        if (!MCStringFirstIndexOfChar(p_native_path,
                                      '.',
                                      t_last_separator,
                                      kMCStringOptionCompareExact,
                                      t_first_extension))
        {
            t_first_extension = UINDEX_MAX;
        }
        
        MCAutoStringRef t_leaf_name;
        if (!MCStringCopySubstring(p_native_path,
                                   MCRangeMakeMinMax(t_last_separator,
                                                     t_first_extension),
                                   &t_leaf_name))
        {
            return false;
        }
        
        size_t t_section_size = 0;
        char *t_section_ptr [[gnu::may_alias]] =
                    getsectdata("__DATA",
                                "__libs",
                                &t_section_size);
        
        if (t_section_ptr != nullptr)
        {
            t_section_ptr += (size_t)_dyld_get_image_vmaddr_slide(0);
            
            MCSLibraryStaticLibInfo **t_libs [[gnu::may_alias]] =
                    reinterpret_cast<MCSLibraryStaticLibInfo **>(t_section_ptr);
            for(size_t t_lib_index = 0; t_lib_index < t_section_size / sizeof(*t_libs); t_lib_index++)
            {
                const char *t_lib_name =
                        *(t_libs[t_lib_index]->name);
                
                if (MCStringIsEqualToCString(*t_leaf_name,
                                             t_lib_name,
                                             kMCStringOptionCompareCaseless))
                {
                    m_handle = t_libs[t_lib_index];
                    return true;
                }
            }
        }
        
        return __MCSLibraryThrowCreateWithNativePathFailed(p_native_path);
    }
    
    bool CreateWithAddress(void *p_address)
    {
        return __MCSLibraryThrowCreateWithAddressFailed(p_address);
    }
    
    bool CopyNativePath(MCStringRef& r_native_path) const
    {
        return MCStringCreateWithCString(*(m_handle->name),
                                         r_native_path);
    }
    
    void *LookupSymbol(MCStringRef p_symbol) const
    {
        for(size_t i = 0; m_handle->exports[i].symbol != nullptr; i++)
        {
            if (MCStringIsEqualToCString(p_symbol,
                                         m_handle->exports[i].symbol,
                                         kMCStringOptionCompareExact))
            {
                return m_handle->exports[i].address;
            }
        }
        
        return nullptr;
    }
    
protected:
    const MCSLibraryStaticLibInfo *m_handle;
};

#endif

