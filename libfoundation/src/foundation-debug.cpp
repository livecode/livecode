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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG_LOG)

#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)

#include <windows.h>
#include <crtdbg.h>
#include <dbghelp.h>

extern "C"
NTSYSAPI
WORD
NTAPI
RtlCaptureStackBackTrace(
    DWORD FramesToSkip,
    DWORD FramesToCapture,
    PVOID * BackTrace,
    PDWORD BackTraceHash
);

void __MCAssert(const char *p_file, uint32_t p_line, const char *p_message)
{
	_CrtDbgReport(_CRT_ASSERT, p_file, p_line, NULL, "%s", p_message);
}

static void MCLogV(const char *p_file,
                     uint32_t p_line,
                     const char *p_format,
                     va_list p_format_args)
{
    MCAutoStringRef t_file;
    MCAutoStringRefAsWString t_file_w;
    /* UNCHECKED */ MCStringCreateWithCString(p_file, &t_file);
    /* UNCHECKED */ t_file_w.Lock(*t_file);
    MCAutoStringRef t_message, t_string;
    MCAutoStringRefAsWString t_wstring;
    /* UNCHECKED */ MCStringFormatV(&t_message, p_format, p_format_args);
    /* UNCHECKED */ MCStringFormat(&t_string, "[%u] %@\n",
                                   GetCurrentProcessId(), *t_message);
    /* UNCHECKED */ t_wstring.Lock(*t_string);
    _CrtDbgReportW(_CRT_WARN, *t_file_w, p_line, NULL, *t_wstring);
}

void __MCLog(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
    va_list t_args;
	va_start(t_args, p_format);
    MCLogV(p_file, p_line, p_format, t_args);
	va_end(t_args);
}

void __MCLogWithTrace(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
	typedef BOOL (WINAPI *SymFromAddrPtr)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
	typedef BOOL (WINAPI *SymGetLineFromAddr64Ptr)(HANDLE, DWORD64, PDWORD64, PIMAGEHLP_LINE64);
	typedef BOOL (WINAPI *SymInitializePtr)(HANDLE, PCSTR, BOOL);
	typedef DWORD (WINAPI *SymSetOptionsPtr)(DWORD);
	static SymInitializePtr s_sym_initialize = nil;
	static SymSetOptionsPtr s_sym_setoptions = nil;
	static SymGetLineFromAddr64Ptr s_sym_get_line_from_addr_64 = nil;
	static SymFromAddrPtr s_sym_from_addr = nil;
	if (s_sym_from_addr == nil)
	{
		HMODULE t_dbg_help;
		t_dbg_help = LoadLibraryA("dbghelp.dll");
		if (t_dbg_help != nil)
		{
			s_sym_setoptions = (SymSetOptionsPtr)GetProcAddress(t_dbg_help, "SymSetOptions");
			s_sym_initialize = (SymInitializePtr)GetProcAddress(t_dbg_help, "SymInitialize");
			s_sym_from_addr = (SymFromAddrPtr)GetProcAddress(t_dbg_help, "SymFromAddr");
			s_sym_get_line_from_addr_64 = (SymGetLineFromAddr64Ptr)GetProcAddress(t_dbg_help, "SymGetLineFromAddr64");
			s_sym_initialize(GetCurrentProcess(), NULL, TRUE);
			s_sym_setoptions(SYMOPT_LOAD_LINES);
		}
	}

    va_list t_args;
    va_start(t_args, p_format);
    MCLogV(p_file, p_line, p_format, t_args);
    va_end(t_args);

	if (s_sym_from_addr != nil)
	{
		char t_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(CHAR)];
		PSYMBOL_INFO t_symbol = (PSYMBOL_INFO)t_buffer;

		t_symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		t_symbol->MaxNameLen = MAX_SYM_NAME - 1;

		void *t_backtrace[64];
		USHORT t_count;
		t_count = CaptureStackBackTrace(1, 62, t_backtrace, nil);
		for(uint32_t i = 0; i < t_count; i++)
		{
			if (s_sym_from_addr(GetCurrentProcess(), (DWORD64)t_backtrace[i], nil, t_symbol))
			{
				IMAGEHLP_LINE64 t_line;
				DWORD64 t_displacement;
				t_line . SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				s_sym_get_line_from_addr_64(GetCurrentProcess(), (DWORD64)t_backtrace[i], &t_displacement, &t_line);
				_CrtDbgReport(_CRT_WARN, p_file, p_line, NULL, "[%u]  %s@0x%p, line %d\n", GetCurrentProcessId(), t_symbol -> Name, t_backtrace[i], t_line . LineNumber);
			}
		}
	}
}

void __MCUnreachable(void)
{
	// fprintf(stderr, "**** UNREACHABLE CODE EXECUTED ****\n");
	abort();
}

#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE)

#include <unistd.h>

void __MCUnreachable(void)
{
	fprintf(stderr, "**** UNREACHABLE CODE EXECUTED ****\n");
	abort();
}

void __MCAssert(const char *p_file, uint32_t p_line, const char *p_message)
{
    fprintf(stderr, "MCAssert failed: %s:%u \"%s\"\n", p_file, p_line, p_message);
    abort();
}

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
extern "C"
{
#include    <CoreFoundation/CoreFoundation.h>
    void NSLog(CFStringRef format, ...);
    void NSLogv(CFStringRef format, va_list args);
}
#endif

static void MCLogV(const char *p_file, uint32_t p_line,
                   const char *p_format, va_list p_args)
{
	MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringFormatV(&t_string, p_format, p_args);

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
    CFStringRef t_cf_string;
    if (MCStringConvertToCFStringRef(*t_string, t_cf_string))
    {
        NSLog(CFSTR("%@"), t_cf_string);
        CFRelease(t_cf_string);
    }
#else
    MCAutoStringRefAsSysString t_string_sys;
    /* UNCHECKED */ t_string_sys.Lock(*t_string);
    fprintf(stderr, "[%d] %s\n", getpid(), *t_string_sys);
#endif
}

void __MCLog(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
    va_list t_args;
    va_start(t_args, p_format);
    MCLogV(p_file, p_line, p_format, t_args);
    va_end(t_args);
}

void __MCLogWithTrace(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
    va_list t_args;
    va_start(t_args, p_format);
    MCLogV(p_file, p_line, p_format, t_args);
    va_end(t_args);
}

#elif defined(TARGET_SUBPLATFORM_ANDROID)
#include <android/log.h>

void __MCAssert(const char *p_file, uint32_t p_line, const char *p_message)
{
}

void __MCLog(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
    MCAutoStringRef t_string;

    va_list t_args;
    va_start(t_args, p_format);
    /* UNCHECKED */ MCStringFormatV(&t_string, p_format, t_args);
    va_end(t_args);

    MCAutoStringRefAsSysString t_string_sys;
    /* UNCHECKED */ t_string_sys.Lock(*t_string);
    __android_log_print(ANDROID_LOG_INFO, "revandroid", "%s", *t_string_sys);
}

void __MCUnreachable(void)
{
    __android_log_print(ANDROID_LOG_ERROR, "revandroid",
                        "**** UNREACHABLE CODE EXECUTED ****");
	abort();
}

#elif defined(__EMSCRIPTEN__)

void __MCUnreachable(void)
{
	fprintf(stderr, "**** UNREACHABLE CODE EXECUTED ****\n");
	abort();
}

void __MCAssert(const char *p_file, uint32_t p_line, const char *p_message)
{
    fprintf(stderr, "MCAssert failed: %s:%u \"%s\"\n", p_file, p_line, p_message);
    abort();
}

static void MCLogV(const char *p_file, uint32_t p_line,
                   const char *p_format, va_list p_args)
{
	MCAutoStringRef t_string;
    /* UNCHECKED */ MCStringFormatV(&t_string, p_format, p_args);

    MCAutoStringRefAsSysString t_string_sys;
    /* UNCHECKED */ t_string_sys.Lock(*t_string);
    fprintf(stderr, "%s\n", *t_string_sys);
}

void __MCLog(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
    va_list t_args;
    va_start(t_args, p_format);
    MCLogV(p_file, p_line, p_format, t_args);
    va_end(t_args);
}

void __MCLogWithTrace(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
    va_list t_args;
    va_start(t_args, p_format);
    MCLogV(p_file, p_line, p_format, t_args);
    va_end(t_args);
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////
