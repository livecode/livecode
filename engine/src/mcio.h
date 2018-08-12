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

#ifndef __MC_MCIO__
#define __MC_MCIO__

#define PATH_SEPARATOR '/'

#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
#define PATH_MAX 260
#elif !defined(PATH_MAX)
#define PATH_MAX 1024
#endif

#if defined(_WINDOWS_DESKTOP)
#define IO_APPEND_MODE "ab"
#define IO_READ_MODE "rb"
#define IO_WRITE_MODE "wb"
#define IO_UPDATE_MODE "r+b"
#define ENV_SEPARATOR ';'
#elif defined(_MAC_DESKTOP)
#define IO_APPEND_MODE 	"ab"
#define IO_READ_MODE	"rb"
#define IO_WRITE_MODE	"wb"
#define IO_UPDATE_MODE 	"r+b"
#define IO_CREATE_MODE "wb+"
#define ENV_SEPARATOR 	':'
#elif defined(_LINUX_DESKTOP) || defined(_SERVER) || defined(_MOBILE) || defined(__EMSCRIPTEN__)
#define IO_APPEND_MODE "a"
#define IO_READ_MODE "r"
#define IO_WRITE_MODE "w"
#define IO_UPDATE_MODE "r+"
#define IO_CREATE_MODE "w+"
#define ENV_SEPARATOR ':'
#endif

enum MCOpenFileMode
{
    kMCOpenFileModeRead,
    kMCOpenFileModeWrite,
    kMCOpenFileModeUpdate,
    kMCOpenFileModeAppend,
    kMCOpenFileModeCreate,
};

enum MCFileEncodingType
{
    kMCFileEncodingText,
    kMCFileEncodingNative,
    kMCFileEncodingUTF8,
    kMCFileEncodingUTF16,
    kMCFileEncodingUTF16LE,
    kMCFileEncodingUTF16BE,
    kMCFileEncodingUTF32,
    kMCFileEncodingUTF32LE,
    kMCFileEncodingUTF32BE,
    kMCFileEncodingBinary
};

////////////////////////////////////////////////////////////////////////////////

extern void IO_set_stream(IO_handle stream, char *newptr);
extern bool IO_findfile(MCNameRef p_name, uindex_t& r_index);
extern Boolean IO_closefile(MCNameRef name);
extern bool IO_findprocess(MCNameRef p_name, uindex_t& r_index);
extern void IO_cleanprocesses();
extern bool IO_findsocket(MCNameRef p_name, uindex_t& r_index);
extern real8 IO_cleansockets(real8 ctime);
extern void IO_freeobject(MCObject *o);
extern IO_stat IO_read(void *ptr, uint4 byte_size, IO_handle stream);
extern IO_stat IO_write(const void *ptr, uint4 s, uint4 n, IO_handle stream);
extern IO_stat IO_read_to_eof(IO_handle stream, MCDataRef& r_data);
extern IO_stat IO_fgets(char *ptr, uint4 length, IO_handle stream);

extern IO_stat IO_read_real8(real8 *dest, IO_handle stream);
extern IO_stat IO_read_real4(real4 *dest, IO_handle stream);
extern IO_stat IO_read_uint4(uint4 *dest, IO_handle stream);
extern IO_stat IO_read_int4(int4 *dest, IO_handle stream);
extern IO_stat IO_read_uint2(uint2 *dest, IO_handle stream);
extern IO_stat IO_read_int2(int2 *dest, IO_handle stream);
extern IO_stat IO_read_uint1(uint1 *dest, IO_handle stream);
extern IO_stat IO_read_int1(int1 *dest, IO_handle stream);

extern IO_stat IO_write_real8(real8 dest, IO_handle stream);
extern IO_stat IO_write_real4(real4 dest, IO_handle stream);
extern IO_stat IO_write_uint4(uint4 dest, IO_handle stream);
extern IO_stat IO_write_int4(int4 dest, IO_handle stream);
extern IO_stat IO_write_uint2(uint2 dest, IO_handle stream);
extern IO_stat IO_write_int2(int2 dest, IO_handle stream);
extern IO_stat IO_write_uint1(uint1 dest, IO_handle stream);
extern IO_stat IO_write_int1(int1 dest, IO_handle stream);

// MW-2012-03-04: [[ StackFile5500 ]] Read/Write an unsigned integer using either
//   2 or 4 bytes: 2 if the value is < 16384; otherwise 4.
extern IO_stat IO_read_uint2or4(uint4 *dest, IO_handle stream);
extern IO_stat IO_write_uint2or4(uint4 dest, IO_handle stream);

extern void IO_iso_to_mac(char *sptr, uint4 len);
extern void IO_mac_to_iso(char *sptr, uint4 len);
extern MCStringEncoding MCS_file_to_string_encoding(MCFileEncodingType p_encoding);

extern IO_stat IO_read_mccolor(MCColor& r_color, IO_handle stream);
extern IO_stat IO_write_mccolor(const MCColor& color, IO_handle stream);

// These methods read/write a legacy string (as native).
extern IO_stat IO_read_string_legacy_full(char *&r_string, uint32_t &r_length, IO_handle p_stream, uint8_t p_size, bool p_includes_null, bool p_translate);
extern IO_stat IO_write_string_legacy_full(const MCString &string, IO_handle stream, uint1 size, bool p_write_null);
extern IO_stat IO_read_cstring_legacy(char*& r_string, IO_handle stream, uint1 size);
extern IO_stat IO_write_cstring_legacy(const char* string, IO_handle stream, uint1 size);
// Read and immediately discard a legacy string
extern IO_stat IO_discard_cstring_legacy(IO_handle string, uint1 size);

// These methods are used by 5.5 -> 7.0 props which saved their value out in UTF-8.
extern IO_stat IO_read_stringref_legacy_utf8(MCStringRef& r_string, IO_handle stream, uint1 size = 2);
extern IO_stat IO_write_stringref_legacy_utf8(MCStringRef p_string, IO_handle stream, uint1 size = 2);

// These methods read/write string values in the pre-7.0 format way.
extern IO_stat IO_read_nameref_legacy(MCNameRef& r_name, IO_handle stream, bool as_unicode, uint1 size = 2);
extern IO_stat IO_read_stringref_legacy(MCStringRef& r_string, IO_handle stream, bool as_unicode, uint1 size = 2);
extern IO_stat IO_write_nameref_legacy(MCNameRef name, IO_handle stream, bool as_unicode, uint1 size = 2);
extern IO_stat IO_write_stringref_legacy(MCStringRef string, IO_handle stream, bool as_unicode, uint1 size = 2);

// These methods read/write string values in either the pre-7.0 format way (native)
// or the new way depending on the 'support_unicode' option.
extern IO_stat IO_read_nameref_new(MCNameRef& r_name, IO_handle stream, bool support_unicode, uint1 size = 2);
extern IO_stat IO_read_stringref_new(MCStringRef& r_name, IO_handle stream, bool support_unicode, uint1 size = 2);
extern IO_stat IO_write_nameref_new(MCNameRef name, IO_handle stream, bool support_unicode, uint1 size = 2);
extern IO_stat IO_write_stringref_new(MCStringRef name, IO_handle stream, bool support_unicode, uint1 size = 2);

// These methods read/write a valueref - they are only supported in 7.0+ formats.
extern IO_stat IO_read_valueref_new(MCValueRef& r_value, IO_handle stream);
extern IO_stat IO_write_valueref_new(MCValueRef value, IO_handle stream);

////////////////////////////////////////////////////////////////////////////////

struct MCFakeOpenCallbacks
{
	IO_stat (*read)(void *state, void *buffer, uint32_t size, uint32_t& r_filled);
	int64_t (*tell)(void *state);
	IO_stat (*seek_set)(void *state, int64_t offset);
	IO_stat (*seek_cur)(void *state, int64_t offset);
};
extern IO_handle MCS_fakeopencustom(struct MCFakeOpenCallbacks *callbacks, void *state);

extern IO_handle MCS_fakeopen(const void *p_data, uindex_t p_size);
extern IO_handle MCS_fakeopenwrite(void);
///* LEGACY */ extern IO_stat MCS_fakeclosewrite(IO_handle &stream, char*& r_buffer, uint4& r_length);
extern IO_stat MCS_closetakingbuffer(IO_handle& p_stream, void*& r_buffer, size_t& r_length);
extern IO_stat MCS_closetakingbuffer_uint32(IO_handle& p_stream, void*& r_buffer, uint32_t& r_length);

extern IO_handle MCS_deploy_open(MCStringRef path, intenum_t p_mode);
/* LEGACY */ extern IO_handle MCS_open(const char *path, const char *mode, Boolean map, Boolean driver, uint4 offset);
extern IO_handle MCS_open(MCStringRef path, intenum_t mode, Boolean map, Boolean driver, uint4 offset);
extern void MCS_close(IO_handle &stream);
extern MCFileEncodingType MCS_resolve_BOM_from_bytes(byte_t *p_bytes, uindex_t p_size, uint32_t &r_size);
extern MCFileEncodingType MCS_resolve_BOM(IO_handle &x_stream, uint32_t &r_size);

///* LEGACY */ extern IO_stat MCS_read(void *ptr, uint4 size, uint4 &n, IO_handle stream);
extern IO_stat MCS_readfixed(void *p_ptr, uint32_t p_byte_size, IO_handle p_stream);
extern IO_stat MCS_readall(void *p_ptr, uint32_t p_max_bytes, IO_handle p_stream, uint32_t& r_bytes_read);
extern IO_stat MCS_write(const void *ptr, uint4 size, uint4 n, IO_handle stream);
extern IO_stat MCS_writeat(const void *buffer, uint32_t size, uint32_t pos, IO_handle stream);


// MW-2008-08-15: Put the given character back at the head of the stream.
extern IO_stat MCS_putback(char p_char, IO_handle stream);

extern IO_stat MCS_trunc(IO_handle stream);
extern IO_stat MCS_flush(IO_handle stream);
extern IO_stat MCS_sync(IO_handle stream);
extern Boolean MCS_eof(IO_handle stream);

// MW-2008-03-18: [[ Bug 5078 ]] Update file handling to use 64-bit offsets
extern IO_stat MCS_seek_cur(IO_handle stream, int64_t offset);
extern IO_stat MCS_seek_set(IO_handle stream, int64_t offset);
extern IO_stat MCS_seek_end(IO_handle stream, int64_t offset);
extern int64_t MCS_tell(IO_handle stream);
extern int64_t MCS_fsize(IO_handle stream);

///////////////////////////////////////////////////////////////////////////////

// These are the definitions of the common fake IO methods called by the
// platform dependent methods. Their implementation can be found in mcio.cpp.

int64_t MCS_fake_fsize(IO_handle stream);
int64_t MCS_fake_tell(IO_handle stream);
IO_stat MCS_fake_seek_cur(IO_handle stream, int64_t offset);
IO_stat MCS_fake_seek_set(IO_handle stream, int64_t offset);
IO_stat MCS_fake_read(void *ptr, uint4 size, uint4 &n, IO_handle stream);

///////////////////////////////////////////////////////////////////////////////

#endif
