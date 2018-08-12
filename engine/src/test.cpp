/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "stack.h"

#include "dispatch.h"
#include "mcio.h"
#include "variable.h"

#include "test.h"

////////////////////////////////////////////////////////////////////////////////

static bool s_lowlevel_tests_fetched = false;
static MCLowLevelTest **s_lowlevel_tests = nil;
static int s_lowlevel_test_count = 0;

static void MCTestLog(const char *format, ...);
static bool MCFetchLowLevelTestSection(MCLowLevelTest**& r_tests, int& r_count);

static const char *low_level = "LowLevel";
static const char *high_level = "HighLevel";

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

struct MCHighLevelTest
{
    MCStringRef filename;
};

static bool s_highlevel_tests_fetched = false;
static MCHighLevelTest *s_highlevel_tests = nil;
static int s_highlevel_test_count = 0;

static const char *s_fetch_highleveltest_script =
"\
if $TEST_DIR is empty then\n\
  return empty\n\
end if\n\
set the folder to $TEST_DIR\n\
local tTests\n\
repeat for each line tFile in the files\n\
  if tFile ends with \".livecodescript\" then\n\
    put the folder & slash & tFile & return after tTests\n\
  end if\n\
end repeat\n\
delete the last char of tTests\n\
split tTests by return\n\
return tTests\n\
";

static void MCEnsureHighLevelTests(void)
{
    if (s_highlevel_tests_fetched)
        return;
    
    s_highlevel_tests_fetched = true;
    
    MCAutoStringRef t_message;
    /* UNCHECKED */ MCStringCreateWithCString(s_fetch_highleveltest_script, &t_message);
    MCdefaultstackptr -> domess(*t_message);
    
    MCValueRef t_value;
    t_value = MCresult -> getvalueref();
    
    if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeArray ||
        !MCArrayIsSequence((MCArrayRef)t_value))
        return;
    
    s_highlevel_test_count = MCArrayGetCount((MCArrayRef)t_value);
    MCMemoryNewArray(s_highlevel_test_count, s_highlevel_tests);
    
    MCNameRef t_key;
    MCValueRef t_test;
    uintptr_t t_iterator = 0;
    uindex_t i = 0;
    while (MCArrayIterate((MCArrayRef)t_value, t_iterator, t_key, t_test))
        s_highlevel_tests[i++] . filename = (MCStringRef)MCValueRetain(t_test);
}

int MCCountHighLevelTests(void)
{
    MCEnsureHighLevelTests();
    return s_highlevel_test_count;
}

void MCExecuteHighLevelTest(int p_index)
{
    MCStack *t_stack;
    
    MCNewAutoNameRef t_test_name;
    MCNameCreate(s_highlevel_tests[p_index] . filename, &t_test_name);
    
    t_stack = MCdispatcher -> findstackname(*t_test_name);
    if (t_stack == nil)
    {
        MCTestLog("HighLevelTest:LoadFailure:%s", s_highlevel_tests[p_index] . filename);
        MCretcode = 1;
        return;
    }
    
    if (!t_stack -> parsescript(False, True))
    {
        MCTestLog("HighLevelTest:ParseFailure:%s", s_highlevel_tests[p_index] . filename);
        MCretcode = 1;
        return;
    }
    
    MCAutoNameRef t_name;
    t_name . CreateWithCString("test");
    t_stack -> message(t_name);
    
    MCdispatcher -> destroystack(t_stack, True);
}

////////////////////////////////////////////////////////////////////////////////

static void MCTestLog(const char *p_format, ...)
{
    char test_log[PATH_MAX];
    snprintf(test_log, sizeof(test_log), "%s/test.log", getenv("TEST_DIR"));
    
    FILE *t_file;
    t_file = fopen(test_log, "a");
    
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

void MCTestDoAbort(const char *p_message, const char *p_file, int p_line, bool p_is_high_level)
{
    const char *level = p_is_high_level ? high_level : low_level;
    
    MCTestLog("%sTest:Abort:%s", level, p_message);
    MCTestLog("%sTest:File:%s", level, p_file);
    MCTestLog("%sTest:Line:%d", level, p_line);
    MCretcode = 1;
}

void MCTestDoAssertTrue(const char *p_message, bool p_value, const char *p_file, int p_line, bool p_is_high_level)
{
    const char *level = p_is_high_level ? high_level : low_level;
    if (p_value)
        MCTestLog("%sTest:Success:%s", level, p_message);
    else
    {
        MCTestLog("%sTest:Failure:%s", level, p_message);
        MCretcode = 1;
    }
    MCTestLog("%sTest:File:%s", level, p_file);
    MCTestLog("%sTest:Line:%d", level, p_line);
}

void MCTestDoAssertFalse(const char *p_message, bool p_value, const char *p_file, int p_line, bool p_is_high_level)
{
    const char *level = p_is_high_level ? high_level : low_level;
    if (!p_value)
        MCTestLog("%sTest:Success:%s", level, p_message);
    else
    {
        MCTestLog("%sTest:Failure:%s", level, p_message);
        MCretcode = 1;
    }
    MCTestLog("%sTest:File:%s", level, p_file);
    MCTestLog("%sTest:Line:%d", level, p_line);
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
