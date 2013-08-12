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

#ifndef OSSPEC_H
#define OSSPEC_H

extern void MCS_init();
extern void MCS_shutdown();
extern void MCS_seterrno(int value);
extern int MCS_geterrno(void);
extern uint32_t MCS_getsyserror(void);
extern void MCS_alarm(real8 secs);
extern void MCS_launch_document(MCStringRef docname);
extern void MCS_launch_url(MCStringRef url);
extern void MCS_startprocess(MCNameRef appname, MCStringRef docname, Open_mode mode, Boolean elevated);
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
extern void MCS_downloadurl(MCObject *p_target, MCStringRef p_url, MCStringRef p_file);

extern Boolean MCS_rename(MCStringRef oname, MCStringRef nname);
extern Boolean MCS_backup(MCStringRef oname, MCStringRef nname);
extern Boolean MCS_unbackup(MCStringRef oname, MCStringRef nname);
/* LEGACY */ extern bool MCS_unlink(const char *path);
extern Boolean MCS_unlink(MCStringRef path);
extern void MCS_tmpnam(MCStringRef& r_path);
/* LEGACY */ extern const char *MCS_tmpnam();
extern bool MCS_resolvepath(MCStringRef p_path, MCStringRef& r_resolved_path);
/* LEGACY */ extern char *MCS_resolvepath(const char *path);
extern bool MCS_get_canonical_path(MCStringRef path, MCStringRef& r_path);
extern void MCS_getcurdir(MCStringRef& r_path);
/* LEGACY */ extern char *MCS_getcurdir();
extern Boolean MCS_setcurdir(MCStringRef p_path);
///* LEGACY */ extern Boolean MCS_setcurdir(const char *path);
///* LEGACY */ extern void MCS_getentries(MCExecPoint &p_context, bool p_files, bool p_detailed);
extern bool MCS_getentries(bool p_files, bool p_detailed, MCListRef& r_list);

extern bool MCS_getDNSservers(MCListRef& r_list);
extern bool MCS_getdevices(MCListRef& r_list);
extern bool MCS_getdrives(MCListRef& r_list);
extern Boolean MCS_noperm(MCStringRef path);
extern Boolean MCS_exists(MCStringRef p_path, bool p_is_file);
/* LEGACY */ extern Boolean MCS_exists(const char *path, Boolean file);

extern Boolean MCS_nodelay(int4 fd);

extern IO_stat MCS_runcmd(MCExecPoint &);
extern uint2 MCS_umask(uint2 mask);
extern Boolean MCS_chmod(MCStringRef path, uint2 mask);
extern int4 MCS_getumask();
extern void MCS_setumask(int4 newmask);
extern Boolean MCS_mkdir(MCStringRef path);
extern Boolean MCS_rmdir(MCStringRef path);

extern uint4 MCS_getpid();
extern bool MCS_getaddress(MCStringRef& r_string);
extern bool MCS_getmachine(MCStringRef& r_string);
extern MCNameRef MCS_getprocessor();
extern real8 MCS_getfreediskspace(void);
extern bool MCS_getsystemversion(MCStringRef& r_string);
extern void MCS_loadfile(MCExecPoint &ep, Boolean binary);
extern void MCS_loadresfile(MCExecPoint &ep);
extern void MCS_savebinaryfile(MCStringRef f, MCDataRef data);
extern void MCS_saveresfile(const MCString &s, const MCString data);

extern bool MCS_query_registry(MCStringRef p_key, MCStringRef& r_value, MCStringRef& r_type, MCStringRef& r_error);
///* LEGACY */ extern void MCS_query_registry(MCExecPoint &dest);
extern bool MCS_delete_registry(MCStringRef p_key, MCStringRef& r_error);
extern bool MCS_list_registry(MCStringRef p_path, MCListRef& r_list, MCStringRef& r_error);
extern bool MCS_set_registry(MCStringRef p_key, MCStringRef p_value, MCStringRef p_type, MCStringRef& r_error);

extern Boolean MCS_poll(real8 delay, int fd);
// Mac AppleEvent calls
extern void MCS_send(MCStringRef mess, MCStringRef program,
	                     MCStringRef eventtype, Boolean reply);
extern void MCS_reply(MCStringRef mess, MCStringRef keyword, Boolean err);
extern void MCS_request_ae(MCStringRef message, uint2 ae, MCStringRef&r_value);
extern void MCS_request_program(MCStringRef message, MCStringRef program, MCStringRef& r_value);
extern void MCS_copyresourcefork(MCStringRef source, MCStringRef dest);
extern bool MCS_copyresource(MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type,
							 MCStringRef p_name, MCStringRef p_newid, MCStringRef& r_error);
extern bool MCS_deleteresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_error);
extern bool MCS_getresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_value, MCStringRef& r_error);
extern bool MCS_getresources(MCStringRef p_source, MCStringRef p_type, MCListRef& r_list, MCStringRef& r_error);
extern bool MCS_setresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_id, MCStringRef p_name,
							MCStringRef p_flags, MCStringRef p_value, MCStringRef& r_error);
extern bool MCS_getspecialfolder(MCExecContext& ctxt, MCStringRef p_type, MCStringRef& r_path);
///* LEGACY */ extern void MCS_getspecialfolder(MCExecPoint &ep);
extern bool MCS_shortfilepath(MCStringRef p_path, MCStringRef& r_short_path);
extern bool MCS_longfilepath(MCStringRef p_path, MCStringRef& r_long_path);
extern Boolean MCS_createalias(MCStringRef srcpath, MCStringRef dstpath);
extern bool MCS_resolvealias(MCStringRef p_path, MCStringRef& r_resolved, MCStringRef& r_error);
extern void MCS_doalternatelanguage(MCStringRef script, MCStringRef language);
extern bool MCS_alternatelanguages(MCListRef& r_list);
extern uint1 MCS_langidtocharset(uint2 langid);
extern uint2 MCS_charsettolangid(uint1 charset);

extern void MCS_multibytetounicode(const char *s, uint4 len, char *d, uint4 destbufferl, uint4 &destlen, uint1 charset);
extern void MCS_unicodetomultibyte(const char *s, uint4 len, char *d, uint4 destbufferl, uint4 &destlen, uint1 charset);

extern void MCS_nativetoutf16(const char *p_native, uint4 p_native_length, unsigned short *p_utf16, uint4& p_utf16_length);
extern void MCS_utf16tonative(const unsigned short *p_utf16, uint4 p_utf16_length, char *p_native, uint4& p_native_length);

extern void MCS_nativetoutf8(const char *p_native, uint4 p_native_length, char *p_utf8, uint4& p_utf16_length);
extern void MCS_utf8tonative(const char *p_utf8, uint4 p_uitf8_length, char *p_native, uint4& p_native_length);

extern Boolean MCS_isleadbyte(uint1 charset, char *s);

extern MCSysModuleHandle MCS_loadmodule(MCStringRef p_filename);
/* LEGACY */ extern MCSysModuleHandle MCS_loadmodule(const char *p_filename);
extern void *MCS_resolvemodulesymbol(MCSysModuleHandle p_module, const char *p_symbol);
extern void MCS_unloadmodule(MCSysModuleHandle p_module);

extern void MCS_getlocaldatetime(MCDateTime& x_datetime);
extern bool MCS_datetimetouniversal(MCDateTime& x_datetime);
extern bool MCS_datetimetolocal(MCDateTime& x_datetime);
extern bool MCS_datetimetoseconds(const MCDateTime& p_datetime, double& r_seconds);
extern bool MCS_secondstodatetime(double p_seconds, MCDateTime& r_datetime);
extern const MCDateTimeLocale *MCS_getdatetimelocale(void);

extern void MCS_dnsresolve(MCStringRef p_hostname, MCStringRef& r_dns);
extern bool MCS_hostaddress(MCStringRef& r_host_address);

extern bool MCS_processtypeisforeground(void);
extern bool MCS_changeprocesstype(bool to_foreground);

extern bool MCS_isatty(int);
extern bool MCS_isnan(double value);

extern bool MCS_mcisendstring(MCStringRef p_command, MCStringRef& r_result, bool& r_error);

// Called by trial timeout function in MCDispatch to pop-up a system dialog.
void MCS_system_alert(const char *title, const char *message);

bool MCS_generate_uuid(char buffer[128]);

bool MCS_getnetworkinterfaces(MCStringRef& r_interfaces);

///////////////////////////////////////////////////////////////////////////////

void MCS_deleteurl(MCObject *p_target, MCStringRef p_url);
void MCS_loadurl(MCObject *p_target, MCStringRef p_url, MCNameRef p_message);
void MCS_unloadurl(MCObject *p_target, MCStringRef p_url);
void MCS_posttourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url);
void MCS_putintourl(MCObject *p_target, const MCString& p_data, MCStringRef p_url);
void MCS_geturl(MCObject *p_target, MCStringRef p_url);

///////////////////////////////////////////////////////////////////////////////

enum MCSPutKind
{
	kMCSPutNone,
	kMCSPutIntoMessage,
	kMCSPutAfterMessage,
	kMCSPutBeforeMessage,
	
	kMCSPutOutput,
	kMCSPutUnicodeOutput,
	kMCSPutBinaryOutput,
	kMCSPutHeader,
	kMCSPutNewHeader,
	kMCSPutContent,
	kMCSPutUnicodeContent,
	kMCSPutMarkup,
	kMCSPutUnicodeMarkup
};

bool MCS_put(MCExecPoint& ep, MCSPutKind kind, MCStringRef data);

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
	kMCSOutputTextEncodingUndefined,
	
	kMCSOutputTextEncodingWindows1252,
	kMCSOutputTextEncodingMacRoman,
	kMCSOutputTextEncodingISO8859_1,
	kMCSOutputTextEncodingUTF8,
	
#if defined(__MACROMAN__)
	kMCSOutputTextEncodingNative = kMCSOutputTextEncodingMacRoman,
#elif defined(__WINDOWS_1252__)
	kMCSOutputTextEncodingNative = kMCSOutputTextEncodingWindows1252,
#elif defined(__ISO_8859_1__)
	kMCSOutputTextEncodingNative = kMCSOutputTextEncodingISO8859_1,
#else
#error Unknown native text encoding
#endif
};

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
