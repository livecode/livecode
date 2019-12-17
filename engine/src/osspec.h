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

#ifndef OSSPEC_H
#define OSSPEC_H

#include "foundation-locale.h"

#include "object.h"

///////////////////////////////////////////////////////////////////////////////

// Types for values stored in the Win32 registry
enum MCSRegistryValueType
{
	// Note that these are assigned the same values as their REG_* counterparts
	kMCSRegistryValueTypeNone = 0,					// REG_NONE
	kMCSRegistryValueTypeBinary = 3,				// REG_BINARY
	kMCSRegistryValueTypeDword = 4,					// REG_DWORD
	kMCSRegistryValueTypeDwordLittleEndian = 4,		// REG_DWORD_LITTLE_ENDIAN
	kMCSRegistryValueTypeDwordBigEndian = 5,		// REG_DWORD_BIG_ENDIAN
	kMCSRegistryValueTypeExpandSz = 2,				// REG_EXPAND_SZ
	kMCSRegistryValueTypeLink = 6,					// REG_LINK
	kMCSRegistryValueTypeMultiSz = 7,				// REG_MULTI_SZ
	kMCSRegistryValueTypeQword = 11,				// REG_QWORD
	kMCSRegistryValueTypeQwordLittleEndian = 11,	// REG_QWORD_LITTLE_ENDIAN
	kMCSRegistryValueTypeSz	= 1						// REG_SZ
};

///////////////////////////////////////////////////////////////////////////////


extern void MCS_preinit();
extern void MCS_init();
extern void MCS_shutdown();
extern void MCS_seterrno(int value);
extern int MCS_geterrno(void);
extern uint32_t MCS_getsyserror(void);
extern void MCS_alarm(real8 secs);
extern void MCS_launch_document(MCStringRef docname);
extern void MCS_launch_url(MCStringRef url);
extern void MCS_startprocess(MCNameRef appname, MCStringRef docname, intenum_t mode, Boolean elevated);
extern void MCS_checkprocesses();
extern void MCS_closeprocess(uint2 index);
extern void MCS_kill(int4 pid, int4 sig);
extern void MCS_killall();
extern real8 MCS_time();
extern void MCS_reset_time();
extern void MCS_sleep(real8);
extern bool MCS_getenv(MCStringRef name, MCStringRef& r_result);
extern void MCS_setenv(MCStringRef name, MCStringRef value);
extern void MCS_unsetenv(MCStringRef name);
extern void MCS_downloadurl(MCObjectHandle p_target, MCStringRef p_url, MCStringRef p_file);

extern bool MCS_pathfromnative(MCStringRef p_native_path, MCStringRef& r_livecode_path);
extern bool MCS_pathtonative(MCStringRef p_livecode_path, MCStringRef& r_native_path);

extern Boolean MCS_rename(MCStringRef oname, MCStringRef nname);
extern Boolean MCS_backup(MCStringRef oname, MCStringRef nname);
extern Boolean MCS_unbackup(MCStringRef oname, MCStringRef nname);
/* LEGACY */ extern bool MCS_unlink(const char *path);
extern Boolean MCS_unlink(MCStringRef path);
extern bool MCS_tmpnam(MCStringRef& r_path);
/* LEGACY */ extern const char *MCS_tmpnam();
extern bool MCS_resolvepath(MCStringRef p_path, MCStringRef& r_resolved_path);
/* LEGACY */ extern char *MCS_resolvepath(const char *path);
//extern bool MCS_get_canonical_path(MCStringRef path, MCStringRef& r_path);
extern void MCS_getcurdir(MCStringRef& r_path);
/* LEGACY */ extern char *MCS_getcurdir();
extern Boolean MCS_setcurdir(MCStringRef p_path);
extern bool MCS_getentries(MCStringRef p_folder, bool p_files, bool p_detailed, bool p_utf8, MCListRef& r_list);

extern bool MCS_getDNSservers(MCListRef& r_list);
extern Boolean MCS_getdevices(MCStringRef& r_list);
extern Boolean MCS_getdrives(MCStringRef& r_list);
extern Boolean MCS_noperm(MCStringRef path);
extern Boolean MCS_exists(MCStringRef p_path, bool p_is_file);
/* LEGACY */ extern Boolean MCS_exists(const char *path, Boolean file);

extern IO_stat MCS_runcmd(MCStringRef p_command, MCStringRef& r_output);
extern Boolean MCS_chmod(MCStringRef path, uint2 mask);
extern int4 MCS_getumask();
extern void MCS_setumask(uint2 newmask);
extern Boolean MCS_mkdir(MCStringRef path);
extern Boolean MCS_rmdir(MCStringRef path);

extern uint4 MCS_getpid();
extern bool MCS_getaddress(MCStringRef& r_string);
extern bool MCS_getmachine(MCStringRef& r_string);
extern bool MCS_getprocessor(MCStringRef& r_string);
extern real8 MCS_getfreediskspace(void);
extern bool MCS_getsystemversion(MCStringRef& r_string);
extern bool MCS_loadtextfile(MCStringRef p_filename, MCStringRef& r_text);
extern bool MCS_loadbinaryfile(MCStringRef p_filename, MCDataRef& r_data);

extern void MCS_loadresfile(MCStringRef p_filename, MCStringRef& r_data);
extern bool MCS_savebinaryfile(MCStringRef f, MCDataRef data);
extern bool MCS_savetextfile(MCStringRef f, MCStringRef data);
extern void MCS_saveresfile(MCStringRef p_path, MCDataRef data);

extern bool MCS_query_registry(MCStringRef p_key, MCValueRef& r_value, MCStringRef& r_type, MCStringRef& r_error);
extern bool MCS_delete_registry(MCStringRef p_key, MCStringRef& r_error);
extern bool MCS_list_registry(MCStringRef p_path, MCListRef& r_list, MCStringRef& r_error);
extern bool MCS_set_registry(MCStringRef p_key, MCValueRef p_value, MCSRegistryValueType p_type, MCStringRef& r_error);
extern MCSRegistryValueType MCS_registry_type_from_string(MCStringRef);

extern Boolean MCS_poll(real8 delay, int fd);
// Mac AppleEvent calls
extern void MCS_send(MCStringRef mess, MCStringRef program,
	                     MCStringRef eventtype, Boolean reply);
extern void MCS_reply(MCStringRef mess, MCStringRef keyword, Boolean err);
extern void MCS_request_ae(MCStringRef message, uint2 ae, MCStringRef&r_value);
extern bool MCS_request_program(MCStringRef message, MCStringRef program, MCStringRef& r_result);
extern void MCS_copyresourcefork(MCStringRef source, MCStringRef dest);
extern bool MCS_copyresource(MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type,
							 MCStringRef p_name, MCStringRef p_newid, MCStringRef& r_error);
extern bool MCS_deleteresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_error);
extern bool MCS_getresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_value, MCStringRef& r_error);
extern bool MCS_getresources(MCStringRef p_source, MCStringRef p_type, MCListRef& r_list, MCStringRef& r_error);
extern bool MCS_setresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_id, MCStringRef p_name,
							MCStringRef p_flags, MCStringRef p_value, MCStringRef& r_error);
extern Boolean MCS_getspecialfolder(MCNameRef p_type, MCStringRef& r_path);
// SN-2015-01-16: [[ Bug 14295 ]] Added mode-specific way to get the resources folder
extern void MCS_getresourcesfolder(bool p_standalone, MCStringRef &r_resourcesfolder);
extern bool MCS_shortfilepath(MCStringRef p_path, MCStringRef& r_short_path);
extern bool MCS_longfilepath(MCStringRef p_path, MCStringRef& r_long_path);
extern Boolean MCS_createalias(MCStringRef srcpath, MCStringRef dstpath);
extern Boolean MCS_resolvealias(MCStringRef p_path, MCStringRef& r_resolved);
extern void MCS_doalternatelanguage(MCStringRef script, MCStringRef language);
extern bool MCS_alternatelanguages(MCListRef& r_list);

extern void MCS_nativetoutf16(const char *p_native, uint4 p_native_length, unsigned short *&p_utf16, uint4& p_utf16_length);
extern void MCS_utf16tonative(const unsigned short *p_utf16, uint4 p_utf16_length, char *&p_native, uint4& p_native_length);

extern void MCS_nativetoutf8(const char *p_native, uint4 p_native_length, char *&p_utf8, uint4& p_utf16_length);
extern void MCS_utf8tonative(const char *p_utf8, uint4 p_uitf8_length, char *&p_native, uint4& p_native_length);

extern void MCS_getlocaldatetime(MCDateTime& x_datetime);
extern bool MCS_datetimetouniversal(MCDateTime& x_datetime);
extern bool MCS_datetimetolocal(MCDateTime& x_datetime);
extern bool MCS_datetimetoseconds(const MCDateTime& p_datetime, double& r_seconds);
extern bool MCS_secondstodatetime(double p_seconds, MCDateTime& r_datetime);
extern const MCDateTimeLocale *MCS_getdatetimelocale(void);     // REMOVE ME
extern MCLocaleRef MCS_getsystemlocale();

extern bool MCS_dnsresolve(MCStringRef p_hostname, MCStringRef& r_dns);
extern bool MCS_hostaddress(MCStringRef& r_host_address);

extern bool MCS_processtypeisforeground(void);
extern bool MCS_changeprocesstype(bool to_foreground);

extern bool MCS_isinteractiveconsole(int);
extern bool MCS_isnan(double p_value);
extern bool MCS_isfinite(double p_number);

extern bool MCS_mcisendstring(MCStringRef p_command, MCStringRef& r_result, bool& r_error);

bool MCS_getnetworkinterfaces(MCStringRef& r_interfaces);

///////////////////////////////////////////////////////////////////////////////

void MCS_deleteurl(MCObject *p_target, MCStringRef p_url);
void MCS_loadurl(MCObject *p_target, MCStringRef p_url, MCNameRef p_message);
void MCS_unloadurl(MCObject *p_target, MCStringRef p_url);
void MCS_posttourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url);
void MCS_putintourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url);
void MCS_geturl(MCObject *p_target, MCStringRef p_url);

extern uint2 MCS_getplayloudness();
extern void MCS_setplayloudness(uint2 p_loudness);

///////////////////////////////////////////////////////////////////////////////

extern bool MCS_init_sockets();
extern bool MCS_compare_host_domain(MCStringRef p_host_a, MCStringRef p_host_b);
extern MCSocket *MCS_open_socket(MCNameRef name, MCNameRef from, Boolean datagram, MCObject *o, MCNameRef m, Boolean secure, Boolean sslverify, MCStringRef sslcertfile, MCNameRef p_end_hostname);
extern void MCS_close_socket(MCSocket *s);
extern MCDataRef MCS_read_socket(MCSocket *s, MCExecContext &ctxt, uint4 length, const char *until, MCNameRef m);
extern void MCS_write_socket(const MCStringRef d, MCSocket *s, MCObject *optr, MCNameRef m);
extern MCSocket *MCS_accept(uint2 p, MCObject *o, MCNameRef m, Boolean datagram,Boolean secure,Boolean sslverify, MCStringRef sslcertfile);
extern bool MCS_ha(MCSocket *s, MCStringRef& r_string);
extern bool MCS_hn(MCStringRef& r_string);
extern bool MCS_aton(MCStringRef p_address, MCStringRef& r_name);
extern bool MCS_ntoa(MCStringRef p_hostname, MCObject *p_target, MCNameRef p_message, MCListRef& r_addr);
extern bool MCS_pa(MCSocket *s, MCStringRef& r_string);
extern void MCS_secure_socket(MCSocket *s, Boolean sslverify, MCNameRef end_hostname);

///////////////////////////////////////////////////////////////////////////////

enum MCSPutKind
{
	kMCSPutNone,
	kMCSPutIntoMessage,
	kMCSPutAfterMessage,
	kMCSPutBeforeMessage,
	
	kMCSPutOutput,
	//kMCSPutUnicodeOutput,
	kMCSPutBinaryOutput,
	kMCSPutHeader,
	kMCSPutNewHeader,
	kMCSPutContent,
	//kMCSPutUnicodeContent,
	kMCSPutMarkup,
	//kMCSPutUnicodeMarkup
};

bool MCS_put(MCExecContext &ctxt, MCSPutKind kind, MCStringRef data);
bool MCS_put_binary(MCExecContext &ctxt, MCSPutKind kind, MCDataRef data);

///////////////////////////////////////////////////////////////////////////////

enum MCSErrorMode
{
	kMCSErrorModeNone,
	kMCSErrorModeQuiet,
	kMCSErrorModeInline,
	kMCSErrorModeStderr,
	kMCSErrorModeDebugger,
};

void MCS_set_errormode(MCSErrorMode mode);
MCSErrorMode MCS_get_errormode(void);

enum MCSOutputTextEncoding
{
	kMCSOutputTextEncodingWindows1252,
	kMCSOutputTextEncodingMacRoman,
	kMCSOutputTextEncodingISO8859_1,
	kMCSOutputTextEncodingUTF8,
};

#if defined(__MACROMAN__)
static const MCSOutputTextEncoding kMCSOutputTextEncodingNative = kMCSOutputTextEncodingMacRoman;
#elif defined(__WINDOWS_1252__)
static const MCSOutputTextEncoding kMCSOutputTextEncodingNative = kMCSOutputTextEncodingWindows1252;
#elif defined(__ISO_8859_1__)
static const MCSOutputTextEncoding kMCSOutputTextEncodingNative = kMCSOutputTextEncodingISO8859_1;
#else
#   error Unknown native text encoding
#endif

void MCS_set_outputtextencoding(MCSOutputTextEncoding encoding);
MCSOutputTextEncoding MCS_get_outputtextencoding(void);

enum MCSOutputLineEndings
{
	kMCSOutputLineEndingsLF,
	kMCSOutputLineEndingsCR,
	kMCSOutputLineEndingsCRLF,
	
#if defined(__LF__)
	kMCSOutputLineEndingsNative = kMCSOutputLineEndingsLF,
#elif defined(__CR__)
	kMCSOutputLineEndingsNative = kMCSOutputLineEndingsCR,
#elif defined(__CRLF__)
	kMCSOutputLineEndingsNative = kMCSOutputLineEndingsCRLF,
#else
#error Unknown native line ending encoding
#endif	

};

void MCS_set_outputlineendings(MCSOutputLineEndings line_endings);
MCSOutputLineEndings MCS_get_outputlineendings(void);

bool MCS_set_session_save_path(MCStringRef p_path);
bool MCS_get_session_save_path(MCStringRef& r_path);
bool MCS_set_session_lifetime(uint32_t p_lifetime);
uint32_t MCS_get_session_lifetime(void);
bool MCS_set_session_name(MCStringRef p_name);
bool MCS_get_session_name(MCStringRef &r_name);
bool MCS_set_session_id(MCStringRef p_id);
bool MCS_get_session_id(MCStringRef &r_name);

///////////////////////////////////////////////////////////////////////////////

#endif
