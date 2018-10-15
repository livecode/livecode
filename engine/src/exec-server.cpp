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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "osspec.h"
#include "globals.h"
#include "exec.h"

#ifdef _SERVER
#include "srvscript.h"
#endif

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCServerErrorModeTypeElementInfo[] =
{
	{ "", kMCSErrorModeNone, true },
	{ "quiet", kMCSErrorModeQuiet, false },
	{ "stderr", kMCSErrorModeStderr, false },
	{ "inline", kMCSErrorModeInline, false },
	{ "debugger", kMCSErrorModeDebugger, true },
};

static MCExecEnumTypeInfo _kMCServerErrorModeTypeInfo =
{
	"Server.ErrorMode",
	sizeof(_kMCServerErrorModeTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCServerErrorModeTypeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCServerOutputLineEndingsTypeElementInfo[] =
{
	{ "lf", kMCSOutputLineEndingsLF, false },
	{ "cr", kMCSOutputLineEndingsCR, false },
	{ "crlf", kMCSOutputLineEndingsCRLF, false },
};

static MCExecEnumTypeInfo _kMCServerOutputLineEndingsTypeInfo =
{
	"Server.OutputLineEndings",
	sizeof(_kMCServerOutputLineEndingsTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCServerOutputLineEndingsTypeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCServerOutputTextEncodingTypeElementInfo[] =
{
	{ "windows-1252", kMCSOutputTextEncodingWindows1252, false },
	{ "windows", kMCSOutputTextEncodingWindows1252, false },
	{ "macintosh", kMCSOutputTextEncodingMacRoman, false },
	{ "macroman", kMCSOutputTextEncodingMacRoman, false },
	{ "mac", kMCSOutputTextEncodingMacRoman, false },
	{ "iso-8859-1", kMCSOutputTextEncodingISO8859_1, false },
	{ "linux", kMCSOutputTextEncodingISO8859_1, false },
	{ "utf-8", kMCSOutputTextEncodingUTF8, false },
	{ "utf8", kMCSOutputTextEncodingUTF8, false },
	{ "native", kMCSOutputTextEncodingNative, false },
};

static MCExecEnumTypeInfo _kMCServerOutputTextEncodingTypeInfo =
{
	"Server.OutputTextEncoding",
	sizeof(_kMCServerOutputTextEncodingTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCServerOutputTextEncodingTypeElementInfo
};

//////////

MCExecEnumTypeInfo *kMCServerErrorModeTypeInfo = &_kMCServerErrorModeTypeInfo;
MCExecEnumTypeInfo *kMCServerOutputLineEndingsTypeInfo = &_kMCServerOutputLineEndingsTypeInfo;
MCExecEnumTypeInfo *kMCServerOutputTextEncodingTypeInfo = &_kMCServerOutputTextEncodingTypeInfo;

////////////////////////////////////////////////////////////////////////////////

bool MCServerDeleteSession();

void MCServerExecDeleteSession(MCExecContext& ctxt)
{
#ifdef _SERVER
	if (!MCServerDeleteSession())
		ctxt . LegacyThrow(EE_UNDEFINED);
#endif
}

////////////////////////////////////////////////////////////////////////////////

bool MCServerStartSession(MCExecContext &ctxt);

void MCServerExecStartSession(MCExecContext& ctxt)
{
#ifdef _SERVER
    if (!MCServerStartSession(ctxt))
		ctxt . LegacyThrow(EE_UNDEFINED);
#endif
}

////////////////////////////////////////////////////////////////////////////////

bool MCServerStopSession();

void MCServerExecStopSession(MCExecContext& ctxt)
{
#ifdef _SERVER
	if (!MCServerStopSession())
		ctxt . LegacyThrow(EE_UNDEFINED);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MCServerExecInclude(MCExecContext& ctxt, MCStringRef p_filename, bool p_is_require)
{
#ifdef _SERVER
	MCServerScript *t_script;
	t_script = static_cast<MCServerScript *>(ctxt . GetObject());

	if (t_script -> GetIncludeDepth() > 16)
	{
		ctxt . LegacyThrow(EE_INCLUDE_TOOMANY);
		return;
	}
	
	if (!t_script -> Include(ctxt, p_filename, p_is_require))
	{
		ctxt . LegacyThrow(EE_SCRIPT_ERRORPOS);
		return;
	}
#else
	ctxt . LegacyThrow(p_is_require ? EE_REQUIRE_BADCONTEXT : EE_INCLUDE_BADCONTEXT);
#endif
}

void MCServerExecEcho(MCExecContext& ctxt, MCStringRef p_data)
{
    if (!MCS_put(ctxt, kMCSPutOutput, p_data))
		MCexitall = True;
}

////////////////////////////////////////////////////////////////////////////////

void MCServerExecPutHeader(MCExecContext& ctxt, MCStringRef p_value, bool p_as_new)
{
    if (!MCS_put(ctxt, p_as_new ? kMCSPutNewHeader : kMCSPutHeader, p_value))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}

void MCServerExecPutBinaryOutput(MCExecContext& ctxt, MCDataRef p_value)
{
    if (!MCS_put_binary(ctxt, kMCSPutBinaryOutput, p_value))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}

void MCServerExecPutContent(MCExecContext& ctxt, MCStringRef p_value)
{
	if (!MCS_put(ctxt, kMCSPutContent, p_value))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}

void MCServerExecPutContentUnicode(MCExecContext& ctxt, MCDataRef p_value)
{
	MCAutoStringRef t_string;
	if (!MCStringCreateWithChars((const unichar_t*)MCDataGetBytePtr(p_value), MCDataGetLength(p_value)/sizeof(unichar_t), &t_string)
		|| !MCS_put(ctxt, kMCSPutContent, *t_string))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}

void MCServerExecPutMarkup(MCExecContext& ctxt, MCStringRef p_value)
{
	if (!MCS_put(ctxt, kMCSPutMarkup, p_value))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}

void MCServerExecPutMarkupUnicode(MCExecContext& ctxt, MCDataRef p_value)
{
	MCAutoStringRef t_string;
	if (!MCStringCreateWithChars((const unichar_t*)MCDataGetBytePtr(p_value), MCDataGetLength(p_value)/sizeof(unichar_t), &t_string)
		|| !MCS_put(ctxt, kMCSPutMarkup, *t_string))
		ctxt . LegacyThrow(EE_PUT_CANTSETINTO);
}

bool MCServerSetCookie(MCStringRef p_name, MCStringRef p_value, uint32_t p_expires, MCStringRef p_path, MCStringRef p_domain, bool p_secure, bool p_http_only);
void MCServerExecPutCookie(MCExecContext& ctxt, MCStringRef p_name, MCStringRef p_value, uinteger_t p_expires, MCStringRef p_path, MCStringRef p_domain, bool p_is_secure, bool p_http_only)
{
#ifdef _SERVER
	MCServerSetCookie(p_name, p_value, p_expires, p_path, p_domain, p_is_secure, p_http_only);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MCServerGetErrorMode(MCExecContext& ctxt, intenum_t& r_value)
{
	r_value = (intenum_t)MCS_get_errormode();
}

void MCServerSetErrorMode(MCExecContext& ctxt, intenum_t p_value)
{
	MCS_set_errormode((MCSErrorMode)p_value);
}

void MCServerGetOutputTextEncoding(MCExecContext& ctxt, intenum_t& r_value)
{
	r_value = (intenum_t)MCS_get_outputtextencoding();
}

void MCServerSetOutputTextEncoding(MCExecContext& ctxt, intenum_t p_value)
{
	MCS_set_outputtextencoding((MCSOutputTextEncoding) p_value);
}

void MCServerGetOutputLineEnding(MCExecContext& ctxt, intenum_t& r_value)
{
	r_value = (intenum_t)MCS_get_outputlineendings();
}

void MCServerSetOutputLineEnding(MCExecContext& ctxt, intenum_t p_value)
{
	MCS_set_outputlineendings((MCSOutputLineEndings) p_value);
}

void MCServerGetSessionSavePath(MCExecContext& ctxt, MCStringRef &r_value)
{
	if (MCS_get_session_save_path(r_value))
		return;

	ctxt . Throw();
}

void MCServerSetSessionSavePath(MCExecContext& ctxt, MCStringRef p_value)
{
	if (MCS_set_session_save_path(p_value))
		return;

	ctxt . Throw();
}

void MCServerGetSessionLifetime(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCS_get_session_lifetime();
}

void MCServerSetSessionLifetime(MCExecContext& ctxt, uinteger_t p_value)
{
	MCS_set_session_lifetime(p_value);
}

void MCServerGetSessionCookieName(MCExecContext& ctxt, MCStringRef &r_value)
{
	if (MCS_get_session_name(r_value))
		return;

	ctxt . Throw();
}

void MCServerSetSessionCookieName(MCExecContext& ctxt, MCStringRef p_value)
{
	if (MCS_set_session_name(p_value))
		return;

	ctxt . Throw();
}

void MCServerGetSessionId(MCExecContext& ctxt, MCStringRef &r_value)
{
	if (MCS_get_session_id(r_value))
		return;

	ctxt . Throw();
}

void MCServerSetSessionId(MCExecContext& ctxt, MCStringRef p_value)
{
	if (MCS_set_session_id(p_value))
		return;

	ctxt . Throw();
}
