/* Copyright (C) 2003-2017 LiveCode Ltd.

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

#include "prefix.h"
#include "util.h"

struct TestData
{
    const char *path;
    const char *dir;
    const char *base;
};

/* Test paths which should process the same regardless of platform. */
static const TestData kCommonTestData[] =
{
    /* Absolute paths */
    { "/", "/", "" },
    { "/foo", "/", "foo" },
    { "/foo/", "/foo", "" },
    { "/foo/bar", "/foo", "bar" },
    { "/foo/bar/", "/foo/bar", "" },
    
    /* Relative paths */
    { "", "", "" },
    { "foo", "", "foo" },
    { "foo/", "foo", "" },
    { "foo/bar", "foo", "bar" },
    { "foo/bar/", "foo/bar", "" },
    
    /* Absolute paths with double separators */
    //{ "/foo", "/", "foo" },
    { "/foo//", "/foo", "" },
    { "/foo//bar", "/foo", "bar" },
    { "/foo//bar/", "/foo//bar", "" },
    { "/foo//bar//", "/foo//bar", "" },
    { "/foo/bar//", "/foo/bar", "" },
    
    /* Relative paths with double separators */
    { "foo//", "foo", "" },
    { "foo//bar", "foo", "bar" },
    { "foo//bar/", "foo//bar", "" },
    { "foo/bar//", "foo/bar", "" },
};

/* Test paths which process differently on UNIX. */
static const TestData kUnixTestData[] =
{
    /* Absolute paths with double separators at front (these conflict with
     * UNC style paths on Win32). */
    { "//", "/", "" },
    { "//foo", "/", "foo" },
    { "//foo/", "//foo", "" },
    { "//foo//", "//foo", "" },
    { "//foo/bar", "//foo", "bar" },
    { "//foo//bar", "//foo", "bar" },
    { "//foo//bar//", "//foo//bar", "" },
    { "//foo/bar//", "//foo/bar", "" },
    { "//foo//bar/", "//foo//bar", "" },
};

/* Test paths which process differently on Win32. */
static const TestData kWin32TestData[] =
{
    /* Absolute drive */
    { "C:/", "C:/", "" },
    { "C:/foo", "C:/", "foo" },
    { "C:/foo/", "C:/foo", "" },
    { "C:/foo/bar", "C:/foo", "bar" },
    { "C:/foo/bar/", "C:/foo/bar", "" },
    
    /* Relative drive */
    { "C:", "C:", "" },
    { "C:foo", "C:", "foo" },
    { "C:foo/", "C:foo", "" },
    { "C:foo/bar", "C:foo", "bar" },
    { "C:foo/bar/", "C:foo/bar", "" },
    
    /* UNC */
    { "//", "//", "" },
    { "///", "///", "" },
    { "////", "///", "" },
    { "//Foo", "//Foo", "" },
    { "//Foo/", "//Foo/", "" },
    { "//Foo/Bar", "//Foo/Bar", "" },
    { "//Foo/Bar/", "//Foo/Bar", "" },
    { "//Foo/Bar/Baz", "//Foo/Bar", "Baz" },
    { "//Foo/Bar/Baz/", "//Foo/Bar/Baz", "" },
    
    /* Absolute drive double separators */
    { "C://", "C:/", "" },
    { "C://foo", "C:/", "foo" },
    { "C://foo/", "C://foo", "" },
    { "C://foo/bar", "C://foo", "bar" },
    { "C://foo/bar/", "C://foo/bar", "" },
    { "C:/foo//", "C:/foo", "" },
    { "C:/foo//bar", "C:/foo", "bar" },
    { "C:/foo//bar/", "C:/foo//bar", "" },
    { "C:/foo/bar//", "C:/foo/bar", "" },
    { "C://foo///", "C://foo", "" },
    { "C://foo//bar", "C://foo", "bar" },
    { "C://foo//bar/", "C://foo//bar", "" },
    { "C://foo/bar//", "C://foo/bar", "" },
    { "C:/foo//bar//", "C:/foo//bar", "" },
    { "C://foo//bar//", "C://foo//bar", "" },
    
    /* Relative drive double separators */
    { "C:foo//", "C:foo", "" },
    { "C:foo//bar", "C:foo", "bar" },
    { "C:foo//bar/", "C:foo//bar", "" },
    { "C:foo/bar//", "C:foo/bar", "" },
    
    /* UNC double separators*/
    { "////", "///", "" },
    { "//Foo//", "//Foo/", "" },
    { "//Foo//Bar", "//Foo/", "Bar" },
    { "//Foo//Bar/", "//Foo//Bar", "" },
    { "//Foo//Bar/Baz", "//Foo//Bar", "Baz" },
    { "//Foo//Bar/Baz/", "//Foo//Bar/Baz", "" },
    { "//Foo/Bar//", "//Foo/Bar", "" },
    { "//Foo/Bar//Baz", "//Foo/Bar", "Baz" },
    { "//Foo/Bar//Baz/", "//Foo/Bar//Baz", "" },
    { "//Foo/Bar/Baz//", "//Foo/Bar/Baz", "" },
    { "//Foo//Bar//", "//Foo//Bar", "" },
    { "//Foo//Bar//Baz", "//Foo//Bar", "Baz" },
    { "//Foo//Bar//Baz/", "//Foo//Bar//Baz", "" },
    { "//Foo//Bar/Baz//", "//Foo//Bar/Baz", "" },
    { "//Foo/Bar//Baz//", "//Foo/Bar//Baz", "" },
};

TEST(path, split)
{
    MCInitialize();

    for(size_t i = 0; i < sizeof(kUnixTestData) / sizeof(kUnixTestData[0]); i++)
    {
        MCAutoStringRef t_dir, t_base;
        ASSERT_TRUE(MCU_path_split(MCSTR(kCommonTestData[i].path), &(&t_dir), &(&t_base)));
        MCAutoStringRefAsUTF8String t_dir_c, t_base_c;
        t_dir_c.Lock(*t_dir);
        t_base_c.Lock(*t_base);
        
        MCAutoStringRef t_win32_dir, t_win32_base;
        ASSERT_TRUE(MCU_path_split_win32(MCSTR(kCommonTestData[i].path), &(&t_win32_dir), &(&t_win32_base)));
        MCAutoStringRefAsUTF8String t_win32_dir_c, t_win32_base_c;
        t_win32_dir_c.Lock(*t_dir);
        t_win32_base_c.Lock(*t_base);
        
        MCAutoStringRef t_unix_dir, t_unix_base;
        ASSERT_TRUE(MCU_path_split_unix(MCSTR(kCommonTestData[i].path), &(&t_unix_dir), &(&t_unix_base)));
        MCAutoStringRefAsUTF8String t_unix_dir_c, t_unix_base_c;
        t_unix_dir_c.Lock(*t_dir);
        t_unix_base_c.Lock(*t_base);
        
        EXPECT_STREQ(*t_dir_c, kCommonTestData[i].dir) << "Failed dir extraction for " << kCommonTestData[i].path;
        EXPECT_STREQ(*t_base_c, kCommonTestData[i].base) << "Failed base extraction for " << kCommonTestData[i].path;
        
        EXPECT_STREQ(*t_dir_c, *t_win32_dir_c) << "Different dir on win32 for " << kCommonTestData[i].path;
        EXPECT_STREQ(*t_base_c, *t_win32_base_c) << "Different base on win32 for " << kCommonTestData[i].path;
        EXPECT_STREQ(*t_dir_c, *t_unix_dir_c) << "Different dir on win32 for " << kCommonTestData[i].path;
        EXPECT_STREQ(*t_base_c, *t_unix_base_c) << "Different base on win32 for " << kCommonTestData[i].path;
    }
    
    for(size_t i = 0; i < sizeof(kUnixTestData) / sizeof(kUnixTestData[0]); i++)
    {
        MCAutoStringRef t_dir, t_base;
        ASSERT_TRUE(MCU_path_split_unix(MCSTR(kUnixTestData[i].path), &(&t_dir), &(&t_base)));
        
        MCAutoStringRefAsUTF8String t_dir_c, t_base_c;
        t_dir_c.Lock(*t_dir);
        t_base_c.Lock(*t_base);
        EXPECT_STREQ(*t_dir_c, kUnixTestData[i].dir) << "Failed unix dir extraction for " << kUnixTestData[i].path;
        EXPECT_STREQ(*t_base_c, kUnixTestData[i].base) << "Failed unix base extraction for " << kUnixTestData[i].path;
    }
    
    for(size_t i = 0; i < sizeof(kWin32TestData) / sizeof(kWin32TestData[0]); i++)
    {
        MCAutoStringRef t_dir, t_base;
#ifdef __WINDOWS__
        ASSERT_TRUE(MCU_path_split(MCSTR(kWin32TestData[i].path), &(&t_dir), &(&t_base)));
#else
        ASSERT_TRUE(MCU_path_split_win32(MCSTR(kWin32TestData[i].path), &(&t_dir), &(&t_base)));
#endif
        
        MCAutoStringRefAsUTF8String t_dir_c, t_base_c;
        t_dir_c.Lock(*t_dir);
        t_base_c.Lock(*t_base);
        EXPECT_STREQ(*t_dir_c, kWin32TestData[i].dir) << "Failed win32 dir extraction for " << kWin32TestData[i].path;
        EXPECT_STREQ(*t_base_c, kWin32TestData[i].base) << "Failed win32 base extraction for " << kWin32TestData[i].path;
    }
    
    MCFinalize();
}
