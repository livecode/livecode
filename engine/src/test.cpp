/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include "prefix.h"

#include "test.h"

////////////////////////////////////////////////////////////////////////////////

static bool s_lowlevel_tests_fetched = false;
static MCLowLevelTest **s_lowlevel_tests = nil;
static int s_lowlevel_test_count = 0;

static void MCTestLog(const char *format, ...);
static bool MCFetchLowLevelTestSection(MCLowLevelTest**& r_tests, int& r_count);

////////////////////////////////////////////////////////////////////////////////

static void MCHandleSignalDuringTest(int p_signal)
{
    switch(p_signal)
    {
        case SIGILL:
        case SIGSEGV:
        case SIGABRT:
        case SIGFPE:
            MCTestLog("LowLevelTest:Crash\n");
            exit(-1);
            break;
    }
}

static void MCEnsureLowLevelTests(void)
{
    if (s_lowlevel_tests_fetched)
        return;
    
    s_lowlevel_tests_fetched = true;
    
    MCFetchLowLevelTestSection(s_lowlevel_tests, s_lowlevel_test_count);
}

int MCCountLowLevelTests(void)
{
    MCEnsureLowLevelTests();
    
    return s_lowlevel_test_count;
}

void MCExecuteLowLevelTest(int p_index)
{
    MCLowLevelTest *t_test;
    t_test = s_lowlevel_tests[p_index];
    
    void (*t_old_sigill)(int);
    void (*t_old_sigsegv)(int);
    void (*t_old_sigabrt)(int);
    void (*t_old_sigfpe)(int);
    t_old_sigill = signal(SIGILL, MCHandleSignalDuringTest);
    t_old_sigsegv = signal(SIGSEGV, MCHandleSignalDuringTest);
    t_old_sigabrt = signal(SIGABRT, MCHandleSignalDuringTest);
    t_old_sigfpe = signal(SIGFPE, MCHandleSignalDuringTest);
    
    MCTestLog("LowLevelTest:Begin:%s", t_test -> name);
    MCTestLog("LowLevelTest:File:%s", t_test -> file);
    MCTestLog("LowLevelTest:Line:%d", t_test -> line);
    
    t_test -> handler();
    
    MCTestLog("LowLevelTest:End\n", t_test -> name);
    
    signal(SIGILL, t_old_sigill);
    signal(SIGSEGV, t_old_sigsegv);
    signal(SIGABRT, t_old_sigabrt);
    signal(SIGFPE, t_old_sigfpe);
}

////////////////////////////////////////////////////////////////////////////////

static bool s_highlevel_tests_fetched = false;
static MCHighLevelTest **s_highlevel_tests = nil;
static int s_highlevel_test_count = 0;

int MCCountHighLevelTests(void)
{
    return 0;
}

void MCExecuteHighLevelTest(int p_index)
{
}

////////////////////////////////////////////////////////////////////////////////

static void MCTestLog(const char *p_format, ...)
{
    FILE *t_file;
    t_file = fopen("/Users/mark/Desktop/test.log", "a");
    
    time_t t_time;
    t_time = time(NULL);
    
    struct tm *t_tm;
    t_tm = gmtime(&t_time);
    
    char t_timestamp[256];
    strftime(t_timestamp, 256, "%F %T%z", t_tm);
    
    va_list t_args;
    va_start(t_args, p_format);
    fprintf(t_file, "[[ %s ]] ", t_timestamp);
    vfprintf(t_file, p_format, t_args);
    fprintf(t_file, "\n");
    va_end(t_args);
    
    fclose(t_file);
}

void MCTestDoAbort(const char *p_message, const char *p_file, int p_line)
{
    MCTestLog("LowLevelTest:Abort:%s", p_message);
    MCTestLog("LowLevelTest:File:%s", p_file);
    MCTestLog("LowLevelTest:Line:%d", p_line);
}

void MCTestDoAssertTrue(const char *p_message, bool p_value, const char *p_file, int p_line)
{
    if (p_value)
        MCTestLog("LowLevelTest:Success:%s", p_message);
    else
        MCTestLog("LowLevelTest:Failure:%s", p_message);
    MCTestLog("LowLevelTest:File:%s", p_file);
    MCTestLog("LowLevelTest:Line:%d", p_line);
}

void MCTestDoAssertFalse(const char *p_message, bool p_value, const char *p_file, int p_line)
{
    if (!p_value)
        MCTestLog("LowLevelTest:Success:%s", p_message);
    else
        MCTestLog("LowLevelTest:Failure:%s", p_message);
    MCTestLog("LowLevelTest:File:%s", p_file);
    MCTestLog("LowLevelTest:Line:%d", p_line);
}

////////////////////////////////////////////////////////////////////////////////

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)

#include <mach-o/loader.h>
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>

static bool MCFetchLowLevelTestSection(MCLowLevelTest**& r_tests, int& r_count)
{
    
    unsigned long t_section_data_size;
    char *t_section_data;
    t_section_data = getsectdata("__TEST", "__test", &t_section_data_size);
    if (t_section_data != nil)
    {
        t_section_data += (unsigned long)_dyld_get_image_vmaddr_slide(0);
        r_tests = (MCLowLevelTest **)t_section_data;
        r_count = t_section_data_size / sizeof(MCLowLevelTest *);
        return true;
    }
    
    return false;
}
#endif
