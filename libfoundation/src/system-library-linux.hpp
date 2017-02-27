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
 * Linux Handle Class
 * ================================================================ */

/* The __MCSLibraryHandleLinux class is a specialization of the POSIX handle
 * implementation. It overrides CreateWithAddress to handle the case where an
 * address in the executable is given, in this case, dladdr does not return
 * a dlopen'able dli_fname, but the link_map does. Additionally, it implements
 * CopyNativePath by using dlinfo and RTLD_DI_LINKMAP. This returns a usable
 * path in all cases *except* when the handle belongs to the executable image.
 * In this case, /proc/self/exe is fetched, resolved and returned as the
 * path of the library.
 */

#include "system-library-posix.hpp"

#include <link.h>
#include <sys/stat.h>
#include <unistd.h>

class __MCSLibraryHandleLinux: public __MCSLibraryHandlePosix
{
public:
    /* This function will not work 100% correctly (for all addresses) until
     * we compile with PIC. */
    bool CreateWithAddress(void *p_address)
    {
        void *t_dl_handle = nullptr;
        
        /* We use dladdr1 to get both the Dl_info and the link map entry. */
        Dl_info t_addr_info;
        struct link_map *t_link_map = nullptr;
        if (dladdr1(p_address,
                   &t_addr_info,
                   (void **)&t_link_map,
                   RTLD_DL_LINKMAP) != 0)
        {
            /* If the link_map has a non-empty l_name, then we can use that to
             * open another handle to the library.
             * Otherwise, if it has a non-empty Dl_info.fli_name then we take it
             * to be in the main executable. */
            if (*(t_link_map->l_name) != '\0')
            {
                t_dl_handle = dlopen(t_link_map->l_name,
                                     RTLD_LAZY);
        
            }
            else if (*(t_addr_info.dli_fname) != '\0')
            {
                t_dl_handle = dlopen(NULL,
                                     RTLD_LAZY);
            }
        }

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
        struct link_map *t_link_map = nullptr;
        if (dlinfo(m_handle,
                   RTLD_DI_LINKMAP,
                   &t_link_map) != 0)
        {
            return __MCSLibraryThrowResolvePathFailed();
        }

        /* If the link_map is NULL, or the l_name field is NULL or the l_name
         * field is "", then we take it to be the main executable and use a
         * different approach. */
        if (t_link_map == nullptr ||
            t_link_map->l_name == NULL ||
            *(t_link_map->l_name) == '\0')
        {
            char *t_path =
                    realpath("/proc/self/exe",
                             nullptr);
            if (t_path == nullptr)
            {
                return __MCSLibraryThrowResolvePathFailed();
            }
            return MCStringCreateWithSysString(t_path,
                                               r_native_path);
        }
            
        return MCStringCreateWithSysString(t_link_map->l_name, r_native_path);
    }
};

typedef class __MCSLibraryHandleLinux __MCSLibraryHandle;

