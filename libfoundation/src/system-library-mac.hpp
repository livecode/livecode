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

#include <sys/stat.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>

#include <memory>
#include <type_traits>

/* ---------------------------------------------------------------- */

/* We want a managed pointer for CF pointers which releases them after they're
 * no longer needed.  This can be achieved by specializing std::unique_ptr with
 * a custom deleter.  Main differences between std::unique_ptr and most the
 * MCAuto custom classes:
 *
 * - You can't set a unique_ptr via an raw pointer output parameter (by design);
 *   there's no equivalent to MCAutoPointer::operator&
 * - unique_ptr::operator* returns a _reference_; use the get() method to
 *   return a pointer
 * - You can't use "=" to construct a unique_ptr with a pointer (because the
 *   constructor has two arguments, one of which is defaulted).  Use brace
 *   initialisation.
 */

template <typename T> struct CFReleaseFunctor {
    void operator()(T p) { CFRelease(p); }
};
template <typename T> using MCAutoCF =
    std::unique_ptr<typename std::remove_pointer<T>::type, CFReleaseFunctor<T>>;


/* ================================================================
 * Mac Handle Class
 * ================================================================ */

/* The __MCSLibraryHandleMac class is a specialization of the POSIX handle
 * implementation. Beyond implemented CopyNativePath appropriately, it also
 * overrides CreateWithNativePath to support resolution of frameworks and
 * bundles from just their top-level folder path. This is done in such a way
 * that default search order for frameworks is preserved (assuming the
 * framework's executable file has the same name as the basename of its folder).
 */

#include "system-library-posix.hpp"

class __MCSLibraryHandleMac: public __MCSLibraryHandlePosix
{
public:
    bool
    CreateWithNativePath(MCStringRef p_native_path)
    {
        /* We want to use dlopen's default search strategy for dylibs and
         * frameworks. To do this, we:
         *    1) Check if the path looks like a framework folder, and if so
         *       modify it to a form dlopen will recognise.
         *    2) Check if the path looks like a bundle, and if so, modify it
         *       to the executable path within the bundle.
         * If either of these things do not occur, we just use the path
         * verbatim in the call to dlopen. */
        MCAutoStringRef t_exe_path;
        if (!ResolveFrameworkExecutable(p_native_path,
                                        &t_exe_path) &&
            !ResolveBundleExecutable(p_native_path,
                                     &t_exe_path))
        {
            t_exe_path = p_native_path;
        }
        
        return __MCSLibraryHandlePosix::CreateWithNativePath(*t_exe_path);
    }
                         
    bool
    CopyNativePath(MCStringRef& r_native_path) const
    {
        /* Mac does not have dlinfo or any single function which computes the
         * file path of a loaded shared library. Instead, the list of all loaded
         * images is iterated through, dlopen'd and then compared with the
         * library that is being looked for. If a handle match is found, the
         * native path of the library is taken to be the name of the
         * corresponding image. */
        
        // Iterate through all images currently in memory.
        for(int32_t i = _dyld_image_count(); i >= 0; i--)
        {
            // Fetch the image name
            const char *t_image_native_path =
                    _dyld_get_image_name(i);
            
            // Now dlopen the image with RTLD_NOLOAD so that we only get a handle
            // if it is already loaded and (critically) means it is unaffected by
            // RTLD_GLOBAL (the default mode).
            void *t_image_dl_handle =
                    dlopen(t_image_native_path,
                           RTLD_NOLOAD);
            
            // Determine if they are the same.
            bool t_found = false;
            if (t_image_dl_handle != nullptr)
            {
                t_found = (t_image_dl_handle == m_handle);
                dlclose(t_image_dl_handle);
            }
            
            if (t_found)
            {
                return MCStringCreateWithSysString(t_image_native_path,
                                                   r_native_path);
            }
        }
        
        return __MCSLibraryThrowResolvePathFailed();
    }
    
private:
    /* We consider any path of the form <folder>/<leaf>.framework to be a
     * framework. A path matching this pattern is transformed to:
     *    <folder>/<leaf>.framework/<leaf>
     * So that we can use dlopen's default search behavior. */
    static bool
    ResolveFrameworkExecutable(MCStringRef p_native_path,
                               MCStringRef& r_exe)
    {
        const char *t_framework_ext = ".framework";
        
        if (!MCStringEndsWithCString(p_native_path,
                                     reinterpret_cast<const char_t *>(t_framework_ext),
                                     kMCStringOptionCompareCaseless))
        {
            return false;
        }

        uindex_t t_last_component_start;
        if (MCStringLastIndexOfChar(p_native_path,
                                    '/',
                                    UINDEX_MAX,
                                    kMCStringOptionCompareCaseless,
                                    t_last_component_start))
        {
            t_last_component_start += 1;
        }
        else
        {
            t_last_component_start = 0;
        }
        
        // The last component length is the length of the string
        // minus the last component start minus the length of the
        // .framework extension
        uindex_t t_last_component_length = MCStringGetLength(p_native_path)
            - t_last_component_start - strlen(t_framework_ext);
        
        MCRange t_last_component_range =
                MCRangeMake(t_last_component_start,
                            t_last_component_length);
        
        return MCStringFormat(r_exe,
                              "%@/%*@",
                              p_native_path,
                              &t_last_component_range,
                              p_native_path);
    }
    
    /* We consider any path which successfully resolves as a bundle to be such.
     * Note that bundle's will not search any system paths - only the cwd will
     * be used in the case of a relative path. */
    static bool
    ResolveBundleExecutable(MCStringRef p_native_path,
                            MCStringRef& r_exe)
    {
        MCAutoStringRefAsSysString t_sys_path;
        if (!t_sys_path.Lock(p_native_path))
        {
            return false;
        }
        
        MCAutoCF<CFURLRef> t_url {
        CFURLCreateFromFileSystemRepresentation(nullptr,
                                                reinterpret_cast<const UInt8 *>(*t_sys_path),
                                                t_sys_path.Size(),
                                                true)};
        if (!t_url)
        {
            return false;
        }
        
        MCAutoCF<CFBundleRef> t_bundle {CFBundleCreate(nullptr, t_url.get())};
        if (!t_bundle)
        {
            return false;
        }
        
        MCAutoCF<CFURLRef> t_bundle_exe_url {CFBundleCopyExecutableURL(t_bundle.get())};
        if (!t_bundle_exe_url)
        {
            return false;
        }
        
        char t_sys_exe_path[PATH_MAX];
        if (!CFURLGetFileSystemRepresentation(t_bundle_exe_url.get(),
                                              true,
                                              reinterpret_cast<UInt8 *>(t_sys_exe_path),
                                              PATH_MAX))
        {
            return false;
        }
        
        return MCStringCreateWithSysString(t_sys_exe_path,
                                           r_exe);
    }
};

#if defined(__MAC__)
typedef class __MCSLibraryHandleMac __MCSLibraryHandle;
#endif
