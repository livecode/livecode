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
#elif !defined(_ANDROID_MOBILE)
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
#elif defined(_LINUX_DESKTOP) || defined(_SERVER) || defined(_MOBILE)
#define IO_APPEND_MODE "a"
#define IO_READ_MODE "r"
#define IO_WRITE_MODE "w"
#define IO_UPDATE_MODE "r+"
#define IO_CREATE_MODE "w+"
#define ENV_SEPARATOR ':'
#endif

////////////////////////////////////////////////////////////////////////////////

extern void IO_set_stream(IO_handle stream, char *newptr);
extern Boolean IO_findstream(Streamnode *nodes, uint2 nitems, const char *name, uint2 &i);
extern Boolean IO_findfile(const char *name, uint2 &i);
extern Boolean IO_closefile(const char *name);
extern Boolean IO_findprocess(const char *name, uint2 &i);
extern void IO_cleanprocesses();
extern Boolean IO_findsocket(const char *name, uint2 &i);
extern real8 IO_cleansockets(real8 ctime);
extern void IO_freeobject(MCObject *o);
extern IO_stat IO_read(void *ptr, uint4 size, uint4 &n, IO_handle stream);
extern IO_stat IO_write(const void *ptr, uint4 s, uint4 n, IO_handle stream);
extern IO_stat IO_read_to_eof(IO_handle stream, MCExecPoint &ep);
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

extern IO_stat IO_read_string(char *&r_string, uint32_t &r_length, IO_handle p_stream, uint8_t p_size, bool p_includes_null, bool p_translate);
extern IO_stat IO_read_string(char *&string, IO_handle stream, uint1 size = 2);
extern IO_stat IO_read_string(char *&string, uint4 &outlen, IO_handle stream, bool isunicode, uint1 size = 2);
extern IO_stat IO_read_string_no_translate(char *&string, IO_handle stream, uint1 size = 2);
extern IO_stat IO_write_string(const MCString &string, IO_handle stream, uint1 size = 2, bool p_write_null = true);
extern IO_stat IO_write_string(const char *string, IO_handle stream, uint1 size = 2);
extern IO_stat IO_write_string(const char *string, uint4 outlen, IO_handle stream, Boolean isunicode, uint1 size = 2);

extern IO_stat IO_read_mccolor(MCColor& r_color, IO_handle stream);
extern IO_stat IO_write_mccolor(const MCColor& color, IO_handle stream);

extern IO_stat IO_read_nameref(MCNameRef& r_name, IO_handle stream, uint1 size = 2);
extern IO_stat IO_write_nameref(MCNameRef name, IO_handle stream, uint1 size = 2);

// MW-2009-06-30: This method reads the given number of bytes and fails
//   if that is not possible.
extern IO_stat IO_read_bytes(void *ptr, uint4 size, IO_handle stream);

////////////////////////////////////////////////////////////////////////////////

struct MCFakeOpenCallbacks
{
	IO_stat (*read)(void *state, void *buffer, uint32_t size, uint32_t& r_filled);
	int64_t (*tell)(void *state);
	IO_stat (*seek_set)(void *state, int64_t offset);
	IO_stat (*seek_cur)(void *state, int64_t offset);
};
extern IO_handle MCS_fakeopencustom(struct MCFakeOpenCallbacks *callbacks, void *state);

extern IO_handle MCS_fakeopen(const MCString &data);
extern IO_handle MCS_fakeopenwrite(void);
extern IO_stat MCS_fakeclosewrite(IO_handle &stream, char*& r_buffer, uint4& r_length);

extern bool MCS_isfake(IO_handle stream);
extern uint4 MCS_faketell(IO_handle stream);
extern void MCS_fakewriteat(IO_handle stream, uint4 p_pos, const void *p_buffer, uint4 p_size);

extern IO_handle MCS_open(const char *path, const char *mode, Boolean map, Boolean driver, uint4 offset);
extern IO_stat MCS_close(IO_handle &stream);

extern IO_stat MCS_read(void *ptr, uint4 size, uint4 &n, IO_handle stream);
extern IO_stat MCS_write(const void *ptr, uint4 size, uint4 n, IO_handle stream);

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
