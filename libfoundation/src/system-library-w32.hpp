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

#include <windows.h>

/* ================================================================
 * HMODULE Handle Class
 * ================================================================ */

/* The __MCSLibraryHandleWin32 class uses the standard Win32 HMODULE
 * manipulation functions to provide the handle functionality.
 */

class __MCSLibraryHandleWin32
{
public:
    __MCSLibraryHandleWin32(void)
        : m_handle(nullptr)
    {
    }
    
    ~__MCSLibraryHandleWin32(void)
    {
        if (m_handle == nullptr)
            return;
        FreeLibrary(m_handle);
    }
    
    bool IsDefined(void) const
    {
        return m_handle != nullptr;
    }
    
    bool IsEqualTo(const __MCSLibraryHandleWin32& p_other) const
    {
        return m_handle == p_other.m_handle;
    }
    
    bool Hash(void) const
    {
        return MCHashPointer(static_cast<void *>(m_handle));
    }
    
    bool CreateWithNativePath(MCStringRef p_native_path)
    {
        MCAutoStringRefAsWString t_wstring_path;
        if (!t_wstring_path.Lock(p_native_path))
        {
            return false;
        }

        m_handle = LoadLibraryExW(*t_wstring_path,
                                  NULL,
                                  LOAD_WITH_ALTERED_SEARCH_PATH);
        
        if (m_handle == NULL)
        {
            /* TODO: Use GetLastError() */
            return __MCSLibraryThrowCreateWithNativePathFailed(p_native_path);
        }

        return true;
    }
    
    bool CreateWithAddress(void *p_address)
    {
        if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                               (LPCTSTR)p_address,
                               &m_handle))
        {
            /* TODO: Use GetLastError() */
            m_handle = nullptr;
            return __MCSLibraryThrowCreateWithAddressFailed(p_address);
        }
        
        return true;
    }
    
    bool CopyNativePath(MCStringRef & r_native_path) const
    {
        MCAutoArray<wchar_t> t_native_path;
        
        for (uindex_t i = 1; i < (UINDEX_MAX-1) / MAX_PATH; ++i)
        {
            if (!t_native_path.Extend(i*MAX_PATH + 1))
            {
                return false;
            }
            
            // Make sure the last char in the buffer is NUL so that we can detect
            // failure on Windows XP
            t_native_path[t_native_path.Size() - 1] = '\0';
            
            // On Windows XP, the returned path will be truncated if the buffer is
            // too small and a NUL byte *will not* be added.
            DWORD t_result_size = GetModuleFileNameW(m_handle,
                                                     t_native_path.Ptr(),
                                                     t_native_path.Size());
            DWORD t_error_code = GetLastError();
            
            // A too small buffer is indicated by a 0 return value and:
            //   on XP: the buffer being filled without terminating NUL
            //   others: GetLastError() returning ERROR_INSUFFICIENT_BUFFER
            if (t_result_size == 0 &&
                (t_error_code == ERROR_SUCCESS &&
                 t_native_path[t_native_path.Size() - 1] != '\0') ||
                t_error_code == ERROR_INSUFFICIENT_BUFFER)
            {
                continue; /* Try again with a bigger buffer */
            }
            
            // If we get an error other than insufficient buffer, then we return
            // it.
            if (t_error_code != ERROR_SUCCESS)
            {
                /* TODO[2017-02-21]: Use last error */
                break;
            }
            
            // Make sure the string is NUL terminated, we already know MAX_PATH-1
            // is NUL, on non-Windows XP char t_native_path_size will be NUL, on
            // WindowsXP t_native_path_size will be the length of the filename
            // without NUL.
            MCAssert(t_result_size < t_native_path.Size());
            t_native_path[t_result_size] = '\0';
            
            return MCStringCreateWithWString(t_native_path.Ptr(), r_native_path);
        }
        
        /* TODO[20170221] Oh dear, the path is too long to fit into a UINDEX_MAX!?! */
        return __MCSLibraryThrowResolvePathFailed();
    }

    void *LookupSymbol(MCStringRef p_symbol) const
    {
        MCAutoStringRefAsCString t_cstring_symbol;
        if (!t_cstring_symbol.Lock(p_symbol))
        {
            /* MASKS_OOM */
            return nullptr;
        }
        
        return GetProcAddress(m_handle,
                              *t_cstring_symbol);
    }
    
protected:
    HMODULE m_handle;
};

typedef class __MCSLibraryHandleWin32 __MCSLibraryHandle;
