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
extern void MCS_launch_document(char *docname);
extern void MCS_launch_url(char *url);
extern void MCS_startprocess(char *appname, char *docname, Open_mode mode, Boolean elevated);
extern void MCS_checkprocesses();
extern void MCS_closeprocess(uint2 index);
extern void MCS_kill(int4 pid, int4 sig);
extern void MCS_killall();
extern real8 MCS_time();
extern void MCS_reset_time();
extern void MCS_sleep(real8);
extern char *MCS_getenv(const char *name);
extern void MCS_setenv(const char *name, const char *value);
extern void MCS_unsetenv(const char *name);
extern int4 MCS_rawopen(const char *path, int4 flags);
extern int4 MCS_rawclose(int4 fd);
extern Boolean MCS_rename(const char *oname, const char *nname);
extern Boolean MCS_backup(const char *oname, const char *nname);
extern Boolean MCS_unbackup(const char *oname, const char *nname);
extern Boolean MCS_unlink(const char *path);
extern const char *MCS_tmpnam();
extern char *MCS_resolvepath(const char *path);
extern char *MCS_get_canonical_path(const char *path);
extern char *MCS_getcurdir();
extern Boolean MCS_setcurdir(const char *path);
extern void MCS_getentries(MCExecPoint &p_context, bool p_files, bool p_detailed);
extern void MCS_getDNSservers(MCExecPoint &);
extern Boolean MCS_getdevices(MCExecPoint &);
extern Boolean MCS_getdrives(MCExecPoint &);
extern Boolean MCS_noperm(const char *path);
extern Boolean MCS_exists(const char *path, Boolean file);
extern Boolean MCS_nodelay(int4 fd);

extern IO_stat MCS_runcmd(MCExecPoint &);
extern uint2 MCS_umask(uint2 mask);
extern IO_stat MCS_chmod(const char *path, uint2 mask);
extern int4 MCS_getumask();
extern void MCS_setumask(int4 newmask);
extern Boolean MCS_mkdir(const char *path);
extern Boolean MCS_rmdir(const char *path);

extern uint4 MCS_getpid();
extern const char *MCS_getaddress();
extern const char *MCS_getmachine();
extern const char *MCS_getprocessor();
extern real8 MCS_getfreediskspace(void);
extern const char *MCS_getsystemversion();
extern void MCS_loadfile(MCExecPoint &ep, Boolean binary);
extern void MCS_loadresfile(MCExecPoint &ep);
extern void MCS_savefile(const MCString &f, MCExecPoint &data, Boolean b);
extern void MCS_saveresfile(const MCString &s, const MCString data);

extern void MCS_query_registry(MCExecPoint &dest, const char** type);
extern void MCS_delete_registry(const char *key, MCExecPoint &dest);
extern void MCS_list_registry(MCExecPoint& dest);
extern void MCS_set_registry(const char *key, MCExecPoint &dest, char *type);

extern Boolean MCS_poll(real8 delay, int fd);
// Mac AppleEvent calls
extern void MCS_send(const MCString &mess, const char *program,
	                     const char *eventtype, Boolean reply);
extern void MCS_reply(const MCString &mess, const char *keyword, Boolean err);
extern char *MCS_request_ae(const MCString &message, uint2 ae);
extern char *MCS_request_program(const MCString &message, const char *program);
extern void MCS_copyresourcefork(const char *source, const char *dest);
extern void MCS_copyresource(const char *source, const char *dest,
	                             const char *type, const char *name,
	                             const char *newid);
extern void MCS_deleteresource(const char *source, const char *type,
	                               const char *name);
extern void MCS_getresource(const char *source, const char *type,
	                            const char *name, MCExecPoint &ep);
extern char *MCS_getresources(const char *source, const char *type);
extern void MCS_setresource(const char *source, const char *type,
	                            const char *name, const char *id, const char *flags,
	                            const MCString &s);
extern void MCS_getspecialfolder(MCExecPoint &ep);
extern void MCS_shortfilepath(MCExecPoint &ep);
extern void MCS_longfilepath(MCExecPoint &ep);
extern Boolean MCS_createalias(char *srcpath, char *dstpath);
extern void MCS_resolvealias(MCExecPoint &ep);
extern void MCS_doalternatelanguage(MCString &s, const char *langname);
extern void MCS_alternatelanguages(MCExecPoint &ep);
extern uint1 MCS_langidtocharset(uint2 langid);
extern uint2 MCS_charsettolangid(uint1 charset);

extern void MCS_multibytetounicode(const char *s, uint4 len, char *d, uint4 destbufferl, uint4 &destlen, uint1 charset);
extern void MCS_unicodetomultibyte(const char *s, uint4 len, char *d, uint4 destbufferl, uint4 &destlen, uint1 charset);

extern void MCS_nativetoutf16(const char *p_native, uint4 p_native_length, unsigned short *p_utf16, uint4& p_utf16_length);
extern void MCS_utf16tonative(const unsigned short *p_utf16, uint4 p_utf16_length, char *p_native, uint4& p_native_length);

extern void MCS_nativetoutf8(const char *p_native, uint4 p_native_length, char *p_utf8, uint4& p_utf16_length);
extern void MCS_utf8tonative(const char *p_utf8, uint4 p_uitf8_length, char *p_native, uint4& p_native_length);

extern Boolean MCS_isleadbyte(uint1 charset, char *s);

extern MCSysModuleHandle MCS_loadmodule(const char *p_filename);
extern void *MCS_resolvemodulesymbol(MCSysModuleHandle p_module, const char *p_symbol);
extern void MCS_unloadmodule(MCSysModuleHandle p_module);

extern void MCS_getlocaldatetime(MCDateTime& x_datetime);
extern bool MCS_datetimetouniversal(MCDateTime& x_datetime);
extern bool MCS_datetimetolocal(MCDateTime& x_datetime);
extern bool MCS_datetimetoseconds(const MCDateTime& p_datetime, double& r_seconds);
extern bool MCS_secondstodatetime(double p_seconds, MCDateTime& r_datetime);
extern const MCDateTimeLocale *MCS_getdatetimelocale(void);

extern char *MCS_dnsresolve(const char *p_hostname);
extern char *MCS_hostaddress(void);

extern bool MCS_processtypeisforeground(void);
extern bool MCS_changeprocesstype(bool to_foreground);

extern bool MCS_isatty(int);
extern bool MCS_isnan(double value);

extern bool MCS_mcisendstring(const char *command, char buffer[256]);

// Called by trial timeout function in MCDispatch to pop-up a system dialog.
void MCS_system_alert(const char *title, const char *message);

bool MCS_generate_uuid(char buffer[128]);

void MCS_getnetworkinterfaces(MCExecPoint& ep);

// MW-2013-05-21: [[ RandomBytes ]] Attempt to generate a sequence of random
//   bytes into the provided buffer. The function returns 'false' if there isnt
//   enough entropy available to generate them.
bool MCS_random_bytes(size_t p_count, void *p_buffer);

extern uint2 MCS_getplayloudness();
extern void MCS_setplayloudness(uint2 p_loudness);

///////////////////////////////////////////////////////////////////////////////

void MCS_deleteurl(MCObject *p_target, const char *p_url);
void MCS_loadurl(MCObject *p_target, const char *p_url, const char *p_message);
void MCS_unloadurl(MCObject *p_target, const char *p_url);
void MCS_posttourl(MCObject *p_target, const MCString& p_data, const char *p_url);
void MCS_putintourl(MCObject *p_target, const MCString& p_data, const char *p_url);
void MCS_geturl(MCObject *p_target, const char *p_url);

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

bool MCS_put(MCExecPoint& ep, MCSPutKind kind, const MCString& data);

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

bool MCS_set_session_save_path(const char *p_path);
const char *MCS_get_session_save_path(void);
bool MCS_set_session_lifetime(uint32_t p_lifetime);
uint32_t MCS_get_session_lifetime(void);
bool MCS_set_session_name(const char *p_name);
const char *MCS_get_session_name(void);
bool MCS_set_session_id(const char *p_id);
const char *MCS_get_session_id(void);

///////////////////////////////////////////////////////////////////////////////

#endif
