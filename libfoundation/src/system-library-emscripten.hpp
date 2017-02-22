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

/* The __MCSLibraryHandleEmscripten class is a specialization of the POSIX handle
 * implementation. It only supports manipulating the handle returned by passing
 * NULL to dlopen (which is the main executable) as loadable modules are not
 * currently used.
 */

#include "system-library-posix.hpp"

#include <link.h>
#include <sys/stat.h>
#include <unistd.h>

class __MCSLibraryHandleEmscripten: public __MCSLibraryHandlePosix
{
public:
    bool CreateWithNativePath(MCStringRef p_native_path)
    {
        return CreateWithAddress(nullptr);
    }
    
    bool CreateWithAddress(void *p_address)
    {
        void *t_dl_handle = dlopen(NULL,
                                     RTLD_LAZY);

        if (t_dl_handle == nullptr)
        {
            /* TODO: Use dlerror */
            return __MCSLibraryThrowCreateWithAddressFailed(p_address);
        }
        
        m_handle = t_dl_handle;
        
        return true;
    }
    
    bool
    CopyNativePath(MCStringRef& r_native_path) const
    {
        return MCSCommandLineGetName(r_native_path);
    }
};

typedef class __MCSLibraryHandleLinux __MCSLibraryHandle;

