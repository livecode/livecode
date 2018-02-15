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

#include <dlfcn.h>

/* ================================================================
 * Posix Handle Class
 * ================================================================ */

/* The __MCSLibraryHandlePosix class provides implementations of all methods
 * required by a library handle apart from CopyNativePath. The implementation
 * of the latter depends heavily on the target platform and what features of
 * the dlfcn library are available. */

class __MCSLibraryHandlePosix
{
public:
    __MCSLibraryHandlePosix(void)
        : m_handle(nullptr)
    {
    }
    
    ~__MCSLibraryHandlePosix(void)
    {
        if (m_handle == nullptr)
            return;
        dlclose(m_handle);
    }
    
    bool IsDefined(void) const
    {
        return m_handle != nullptr;
    }
    
    bool IsEqualTo(const __MCSLibraryHandlePosix& p_other) const
    {
        return m_handle == p_other.m_handle;
    }
    
    bool Hash(void) const
    {
        return MCHashPointer(m_handle);
    }
    
    bool CreateWithNativePath(MCStringRef p_native_path)
    {
        MCAutoStringRefAsSysString t_sys_exe_path;
        if (!t_sys_exe_path.Lock(p_native_path))
        {
            return false;
        }
        
        m_handle = dlopen(*t_sys_exe_path,
                          RTLD_LAZY);
        if (m_handle == nullptr)
        {
            MCLog("dlopen failed %s",dlerror());
            /* TODO: dlerror message */
            return __MCSLibraryThrowCreateWithNativePathFailed(p_native_path);
        }
        
        return true;
    }
    
    bool CreateWithAddress(void *p_address)
    {
        Dl_info t_addr_info;
        if (dladdr(p_address,
                   &t_addr_info) != 0)
        {
            m_handle = dlopen(t_addr_info.dli_fname,
                              RTLD_LAZY);
        }
        
        if (m_handle == nullptr)
        {
            /* TODO: Use dlerror */
            return __MCSLibraryThrowCreateWithAddressFailed(p_address);
        }
        
        return true;
    }
    
    void *LookupSymbol(MCStringRef p_symbol) const
    {
        MCAutoStringRefAsCString t_cstring_symbol;
        if (!t_cstring_symbol.Lock(p_symbol))
        {
            /* MASKS OOM */
            return nullptr;
        }
        
        return dlsym(m_handle,
                     *t_cstring_symbol);
    }
    
protected:
    void *m_handle;
};
