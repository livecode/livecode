/*
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

#if !defined(__MCS_SYSTEM_H_INSIDE__)
#	error "Only <foundation-system.h> can be included directly"
#endif

/* ================================================================
 * Loadable library handling
 * ================================================================ */

/* The MCSLibrary library provides an API for handling loadable libraries in a
 * standardized fashion. */

/* ================================================================
 * Types
 * ================================================================ */

/* An opaque custom value ref type representing a loaded library. */
typedef struct __MCSLibrary *MCSLibraryRef;

/* The binding to the internal MCTypeInfoRef for the MCSLibraryRef type */
MC_DLLEXPORT MCTypeInfoRef
MCSLibraryTypeInfo(void) ATTRIBUTE_PURE;

/* ================================================================
 * Construction and querying
 * ================================================================ */

/* Create an MCSLibraryRef object by loading the library from the specified
 * path.
 *
 * If the path is absolute, then only an attempt to load that specific library
 * will be made. If the path is relative, however, an attempt to load the
 * library will use the system's default search order.
 *
 * On Mac, bundles, frameworks and dylibs are supported. In order to load a
 * bundle, an absolute path must be supplied. Otherwise, if the path ends in
 * <LEAF>.framework then the path will be modified to end in
 * <LEAF>.framework/<LEAF>. In the dylib and framework case, dlopen's default
 * search strategy is used.
 *
 * On Windows, dlls are supported. They are loaded using LoadLibraryEx with
 * the LOAD_WITH_ALTERED_SEARCH_PATH flag set.
 *
 * On Linux, shared objects (.so) are supported. They are loaded using dlopen's
 * search strategy.
 *
 * On Android, shared objects (.so) are supported. They are loaded using
 * dlopen's search strategy which requires that non-system libraries be loaded
 * using absolute paths (otherwise they can fail to load).
 *
 * On iOS, only statically linked and registered objects are supported. The
 * name is matched exactly with the name specified in the static registration
 * record. */
MC_DLLEXPORT bool
MCSLibraryCreateWithPath(MCStringRef p_path,
                         MCSLibraryRef& r_library);

/* Create an MCSLibraryRef object by referencing the library currently loaded
 * at the specified address. */
MC_DLLEXPORT bool
MCSLibraryCreateWithAddress(void *p_address,
                            MCSLibraryRef& r_library);

/* Copy the full native path to the specified library. If the library is of
 * static type, this returns the path of the loadable object which the library
 * is linked into. */
MC_DLLEXPORT bool
MCSLibraryCopyNativePath(MCSLibraryRef p_library,
                         MCStringRef& r_path);

/* Copy the full (non-native) path to the specified library. If the library is of
 * static type, this returns the path of the loadable object which the library
 * is linked into. */
MC_DLLEXPORT bool
MCSLibraryCopyPath(MCSLibraryRef p_library,
                   MCStringRef& r_path);

/* Lookup the symbol in the specified library. */
MC_DLLEXPORT void *
MCSLibraryLookupSymbol(MCSLibraryRef p_library,
                       MCStringRef p_symbol);

#if defined(__ANDROID__)
/* Sets the path to use as the base path for the paths of libraries.
 * This must be called after MCSInitialize, with the value of the
 * Android application's context's nativeLibPath member. */
MC_DLLEXPORT void
MCSLibraryAndroidSetNativeLibPath(MCStringRef p_path);
#endif

#ifdef __MCS_INTERNAL_API__

bool
__MCSLibraryThrowCreateWithNativePathFailed(MCStringRef p_native_path);
bool
__MCSLibraryThrowCreateWithAddressFailed(void *p_address);
bool
__MCSLibraryThrowResolvePathFailed(void);

#endif

/* ================================================================
 * Static binding (iOS only)
 * ================================================================ */

#ifdef __IOS__

/* New style (C++ based) - not implemented yet */

#if defined(MCS_LIBRARY_CXX_IOS_STATIC)
/* The record which each statically linked library should register using a
 * constructor. */
struct MCSLibraryStaticInfo
{
    MCSLibraryStaticInfo *__next;
    const char *name;
    struct {
        const char *symbol;
        void *address;
    } *exports;
};

/* Register the given statically linked library and required symbols. The
 * record passed as p_info must exist until the foundation library is
 * shutdown. */
MC_DLLEXPORT void
MCSLibraryRegisterStatic(MCSLibraryStaticInfo& p_info);
#endif

/* Old style (section based) */

#if !defined(MCS_LIBRARY_CXX_IOS_STATIC)

struct MCSLibraryStaticLibExport
{
    const char *symbol;
    void *address;
};

struct MCSLibraryStaticLibInfo
{
    char * const * name;
    const struct MCSLibraryStaticLibExport *exports;
};

#endif

#endif

/* ================================================================
 * Library API initialization
 * ================================================================ */

#ifdef __MCS_INTERNAL_API__
bool
__MCSLibraryInitialize(void);
void
__MCSLibraryFinalize(void);
#endif

/* ================================================================
 * C++ API
 * ================================================================ */

#ifdef __cplusplus

typedef MCAutoValueRefBase<MCSLibraryRef> MCSAutoLibraryRef;

#endif
