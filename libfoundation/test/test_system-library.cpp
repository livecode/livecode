/* Copyright (C) 2017 LiveCode Ltd.
 
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

#include "gtest/gtest.h"

#include "foundation-system.h"

static const char *kNonExistantLibrary = "libihopethisdoesntexist.badextension";

#if defined(__MAC__)
static const char *kExistingStaticLibrary = nullptr;
static const char *kExistingStaticLibrarySymbol = nullptr;
static const char *kExistingDlHandleLibrary = "libz.dylib";
static const char *kExistingDlHandleLibrarySymbol = "inflate";
static const char *kExistingCFBundleRefLibrary = "CoreGraphics.framework";
static const char *kExistingCFBundleRefLibrarySymbol = "CGBitmapContextCreate";
static const char *kExistingHMODULELibrary = nullptr;
static const char *kExistingHMODULELibrarySymbol = nullptr;
#elif defined(__WINDOWS__)
static const char *kExistingStaticLibrary = nullptr;
static const char *kExistingStaticLibrarySymbol = nullptr;
static const char *kExistingDlHandleLibrary = nullptr;
static const char *kExistingDlHandleLibrarySymbol = nullptr;
static const char *kExistingCFBundleRefLibrary = nullptr;
static const char *kExistingCFBundleRefLibrarySymbol = nullptr;
static const char *kExistingHMODULELibrary = "user32.dll";
static const char *kExistingHMODULELibrarySymbol = "CreateWindowExW";
#elif defined(__LINUX__)
static const char *kExistingStaticLibrary = nullptr;
static const char *kExistingStaticLibrarySymbol = nullptr;
static const char *kExistingDlHandleLibrary = "libz.so";
static const char *kExistingDlHandleLibrarySymbol = "inflate";
static const char *kExistingCFBundleRefLibrary = nullptr;
static const char *kExistingCFBundleRefLibrarySymbol = nullptr;
static const char *kExistingHMODULELibrary = nullptr;
static const char *kExistingHMODULELibrarySymbol = nullptr;
#endif

static bool
test_createwithpath(const char *p_lib)
{
    if (p_lib == nullptr)
        return true;
    MCSAutoLibraryRef t_library;
    return MCSLibraryCreateWithPath(MCSTR(p_lib),
                                    &t_library);
}

static void
test_copypath_roundtrips(const char *p_lib)
{
    if (p_lib == nullptr)
        return;
    
    MCSAutoLibraryRef t_library;
    ASSERT_TRUE(
                MCSLibraryCreateWithPath(MCSTR(p_lib),
                                         &t_library)
                );
    
    MCAutoStringRef t_path;
    ASSERT_TRUE(
                MCSLibraryCopyPath(*t_library,
                                   &t_path));
    
    MCSAutoLibraryRef t_other_library;
    ASSERT_TRUE(
                MCSLibraryCreateWithPath(*t_path,
                                         &t_other_library));
    
    ASSERT_TRUE(MCValueIsEqualTo(*t_library,
                                 *t_other_library));
}

static void
test_lookup_existing_symbol(const char *p_lib, const char *p_symbol)
{
    if (p_lib == nullptr)
        return;
    
    MCSAutoLibraryRef t_library;
    ASSERT_TRUE(
                MCSLibraryCreateWithPath(MCSTR(p_lib),
                                         &t_library)
                );
    
    void *t_symbol =
            MCSLibraryLookupSymbol(*t_library,
                                   MCSTR(p_symbol));
    ASSERT_NE(t_symbol,
              nullptr);
}

static void
test_createwithaddress_roundtrips(const char *p_lib,
                                  const char *p_symbol)
{
    if (p_lib == nullptr)
        return;
    
    MCSAutoLibraryRef t_library;
    ASSERT_TRUE(
                MCSLibraryCreateWithPath(MCSTR(p_lib),
                                         &t_library)
                );
    
    void *t_symbol =
    MCSLibraryLookupSymbol(*t_library,
                           MCSTR(p_symbol));
    ASSERT_NE(t_symbol,
              nullptr);
    
    MCSAutoLibraryRef t_other_library;
    ASSERT_TRUE(
                MCSLibraryCreateWithAddress(t_symbol,
                                            &t_other_library));
    
    ASSERT_TRUE(MCValueIsEqualTo(*t_library,
                                 *t_other_library));
}


//// Non existant path tests
//
// These check that creating a library from a non-existing path throws the
// correct error.

TEST(system_library, createwithpath_non_existing_path)
{
    ASSERT_FALSE(test_createwithpath(kNonExistantLibrary));
    // TODO: Test error
}


//// Existing paths tests
//
// These check that libraries create from paths correctly.

TEST(system_library, createwithpath_existing_static)
{
    ASSERT_TRUE(test_createwithpath(kExistingStaticLibrary));
}

TEST(system_library, createwithpath_existing_dlhandle)
{
    ASSERT_TRUE(test_createwithpath(kExistingDlHandleLibrary));
}

TEST(system_library, createwithpath_existing_cfbundleref)
{
    ASSERT_TRUE(test_createwithpath(kExistingCFBundleRefLibrary));
}

TEST(system_library, createwithpath_existing_hmodule)
{
    ASSERT_TRUE(test_createwithpath(kExistingHMODULELibrary));
}


//// CopyPath roundtripping tests
//
// These check that creating a library from a path, then creating another
// library from the path copied from the original results in the same library.

TEST(system_library, copypath_static_roundtrips)
{
    test_copypath_roundtrips(kExistingStaticLibrary);
}

TEST(system_library, copypath_dlhandle_roundtrips)
{
    test_copypath_roundtrips(kExistingDlHandleLibrary);
}

TEST(system_library, copypath_cfbundleref_roundtrips)
{
    test_copypath_roundtrips(kExistingCFBundleRefLibrary);
}

TEST(system_library, copypath_hmodule_roundtrips)
{
    test_copypath_roundtrips(kExistingHMODULELibrary);
}


//// LookupSymbol existing symbol tests
//
// These check that looking up a known symbol in a library returns non-null.

TEST(system_library, lookupsymbol_existing_static)
{
    test_lookup_existing_symbol(kExistingStaticLibrary,
                                kExistingStaticLibrarySymbol);
}

TEST(system_library, lookupsymbol_existing_dlhandle)
{
    test_lookup_existing_symbol(kExistingDlHandleLibrary,
                                kExistingDlHandleLibrarySymbol);
}

TEST(system_library, lookupsymbol_existing_cfbundleref)
{
    test_lookup_existing_symbol(kExistingCFBundleRefLibrary,
                                kExistingCFBundleRefLibrarySymbol);
}

TEST(system_library, lookupsymbol_existing_hmodule)
{
    test_lookup_existing_symbol(kExistingHMODULELibrary,
                                kExistingHMODULELibrarySymbol);
}

//// CreateWithAddress roundtrips
//
// These check that creating a library from a path, resolving a symbol and then
// creating a library from the symbol address ends up with the same library
// handle.

TEST(system_library, createwithaddress_dlhandle_roundtrips)
{
    test_createwithaddress_roundtrips(kExistingDlHandleLibrary,
                                      kExistingDlHandleLibrarySymbol);
}

TEST(system_library, createwithaddress_hmodule_roundtrips)
{
    test_createwithaddress_roundtrips(kExistingHMODULELibrary,
                                      kExistingHMODULELibrarySymbol);
}
