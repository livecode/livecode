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
 * Android Handle Class
 * ================================================================ */

/* One of two different strategies for computing the path to a shared library
 * is used, depending on the API level of the target device.
 *
 * On older devices (API < 21), there is no direct way to get the required
 * path. However, the internal structure of dlhandle is fixed for those versions
 * as follows:
 *    struct soinfo {
 *      char        __padding[128];
 *      const void* phdr;
 *      size_t      phnum;
 *      void*       entry;
 *      void*       base;
 *    }
 * As each dlhandle is just a pointer to a struct soinfo, the base address is
 * easily accessible for passing to dladdr to get the dli_fname of the library
 * which can then be passed to dlopen.
 *
 * On newer devices (API >= 22), the dl_iterate_phdr function is available which
 * allows iterating through all currently open shared libraries. Each is tested
 * in turn (by dladdr'ing the base address and then dlopen'ing as above) to
 * see which gives rise to the same dlhandle which is being searched for. Once
 * the dlhandle which matches is found, the dli_fname of the base address gives
 * us the required information.
 *
 * The reason to split the methods up based on API level is to ensure forward
 * compatibility of code compiled now, should the internal structure of
 * dlhandles change in the future.
 *
 * Android appears to only ever return the 'leaf' of the filepath for a shared
 * library; however, Bug 16917 indicates that passing a non-absolute path to
 * dlopen can cause it to fail. Therefore, we modify the returned path by
 * prepending the current activities 'nativeLibDir' to the leaf - if that file
 * exists. If the file does not exist, it is assumed that the library is a
 * builtin/system library and the name is left as the leaf.
 */

#include "system-library-posix.hpp"

#include <dlfcn.h>
#include <link.h>

#if __ANDROID_API__ < 21
extern "C" __attribute__((weak)) int
dl_iterate_phdr(int (*)(struct dl_phdr_info *, size_t, void *),
                void *);
#endif

class __MCSLibraryHandleAndroid: public __MCSLibraryHandlePosix
{
public:
    bool
    CopyNativePath(MCStringRef& r_native_path) const
    {
        const char *t_sys_filename = nullptr;
        if (dl_iterate_phdr == nullptr)
        {
            t_sys_filename = CopyNativePathLegacy();
        }
        else
        {
            t_sys_filename = CopyNativePathModern(m_handle).Run();
        }

        if (t_sys_filename == nullptr)
        {
            return __MCSLibraryThrowResolvePathFailed();
        }
            
        MCAutoStringRef t_filename;
        if (!MCStringCreateWithSysString(t_sys_filename,
                                         &t_filename))
        {
            return false;
        }
    
        MCAutoStringRef t_full_native_path;
        if (!MCStringFormat(&t_full_native_path,
                            "%@/%@",
                            sMCSLibraryAndroidNativeLibPath,
                            *t_filename))
        {
            return false;
        }
        
        MCSFileType p_type = kMCSFileTypeUnsupported;
        if (!MCSFileGetType(*t_full_native_path,
                       true,
                       p_type))
        {
            MCErrorReset();
        }
        
        if (p_type == kMCSFileTypeRegular)
        {
            r_native_path = t_full_native_path.Take();
        }
        else
        {
            r_native_path = t_filename.Take();
        }
        
        return true;
    }
    
private:
    class CopyNativePathModern
    {
    public:
        CopyNativePathModern(void *p_handle)
            : m_handle(p_handle),
              m_filename(nullptr)
        {
        }
        
        const char *Run(void)
        {
            dl_iterate_phdr([](struct dl_phdr_info* p_info, size_t p_size, void *p_context)
                            {
                                auto t_func = static_cast<CopyNativePathModern *>(p_context);
                                return (*t_func)(p_info, p_size);
                            },
                            this);
            return m_filename;
        }
        
    private:
        int operator () (struct dl_phdr_info* p_info,
                         size_t p_size)
        {
            if (m_filename != nullptr)
            {
                return 0;
            }
            
            Dl_info t_addr_info;
            if (dladdr(reinterpret_cast<void *>(p_info->dlpi_addr),
                       &t_addr_info) == 0)
            {
                return 0;
            }
        
            void *t_other_dl_handle =
                    dlopen(t_addr_info.dli_fname,
                           RTLD_LAZY);
            
            bool t_found = false;
            if (t_other_dl_handle != nullptr)
            {
                t_found = (t_other_dl_handle == m_handle);
                dlclose(t_other_dl_handle);
            }
            
            if (t_found)
            {
                m_filename = t_addr_info.dli_fname;
            }
            
            return 0;
        }
        
        void *m_handle;
        const char *m_filename;
    };
    
    const char *CopyNativePathLegacy(void) const
    {
        struct soinfo {
            char        __padding[128];
            const void* phdr;
            size_t      phnum;
            void*       entry;
            void*       base;
        };
        
        const struct soinfo* t_si =
                static_cast<const struct soinfo*>(m_handle);
        
        Dl_info t_addr_info;
        if (dladdr(t_si->base,
                   &t_addr_info) == 0)
        {
            return nullptr;
        }
        
        return t_addr_info.dli_fname;
    }
};

typedef class __MCSLibraryHandleAndroid __MCSLibraryHandle;
