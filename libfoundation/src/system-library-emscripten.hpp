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
 * Emscripten Handle Class
 * ================================================================ */

/* The __MCSLibraryHandleEmscripten class is a dummy implementation which
 * notionally represents a handle to the main executable. It currently will
 * not find any symbols.
 */

#include <link.h>
#include <sys/stat.h>
#include <unistd.h>

class __MCSLibraryHandleEmscripten
{
public:
    __MCSLibraryHandleEmscripten(void)
    {
    }
    
    ~__MCSLibraryHandleEmscripten(void)
    {
    }
    
    bool IsDefined(void) const
    {
        return m_defined;
    }
    
    bool IsEqualTo(const __MCSLibraryHandleEmscripten& p_other) const
    {
        return false;
    }
    
    bool Hash(void) const
    {
        return MCHashPointer(nullptr);
    }
    
    bool CreateWithNativePath(MCStringRef p_native_path)
    {
        return false;
    }
    
    bool CreateWithAddress(void *p_address)
    {
        m_defined = true;
        return true;
    }
    
    void *LookupSymbol(MCStringRef p_symbol) const
    {
        return nullptr;
    }
    
    bool CopyNativePath(MCStringRef& r_native_path) const
    {
        return MCStringCreateWithCString("<main>", r_native_path);
    }

private:
    bool m_defined = false;
};

typedef class __MCSLibraryHandleEmscripten __MCSLibraryHandle;

