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
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "mcio.h"

#include "mcerror.h"
#include "execpt.h"
#include "player.h"
#include "osspec.h"
#include "globals.h"
#include "util.h"
#include "socket.h"
#include "uidc.h"

#include "stacksecurity.h"

#if !defined(_MOBILE) && !defined(_SERVER)
void IO_set_stream(IO_handle stream, char *newptr)
{
	if (newptr < stream->buffer)
		stream->ioptr = stream->buffer;
	else
		if (newptr > stream->buffer + stream->len)
			stream->ioptr = stream->buffer + stream->len;
		else
			stream->ioptr = newptr;
}
#endif

Boolean IO_findstream(Streamnode *nodes, uint2 nitems,
                      const char *name, uint2 &i)
{
	if (nitems)
		do
		{
			nitems--;
			if (strequal(name, nodes[nitems].name))
			{
				i = nitems;
				return True;
			}
		}
		while (nitems);
	return False;
}

Boolean IO_findfile(const char *name, uint2 &i)
{
	return IO_findstream(MCfiles, MCnfiles, name, i);
}

Boolean IO_closefile(const char *name)
{
	uint2 index;
	if (IO_findfile(name, index))
	{
		if (MCfiles[index].ihandle != NULL)
			MCS_close(MCfiles[index].ihandle);
		else
		{
			if (MCfiles[index].ohandle->flags & IO_WRITTEN
			        && !(MCfiles[index].ohandle->flags & IO_SEEKED))
				MCS_trunc(MCfiles[index].ohandle);
			MCS_close(MCfiles[index].ohandle);
		}
		delete MCfiles[index].name;
		while (++index < MCnfiles)
			MCfiles[index - 1] = MCfiles[index];
		MCnfiles--;
		return True;
	}
	return False;
}

Boolean IO_findprocess(const char *name, uint2 &i)
{
	IO_cleanprocesses();
	return IO_findstream(MCprocesses, MCnprocesses, name, i);
}

void IO_cleanprocesses()
{
	MCS_checkprocesses();
	uint2 i = 0;
	while (i < MCnprocesses)
		if (MCprocesses[i].pid == 0
		        && (MCprocesses[i].ihandle == NULL
		            || MCS_eof(MCprocesses[i].ihandle)))
		{
#ifdef X11
			if (MCprocesses[i].mode == OM_VCLIP)
			{
				MCPlayer *tptr = MCplayers;
				while (tptr != NULL)
				{
					if (strequal(MCprocesses[i].name, tptr->getcommand()))
					{
						tptr->playstop(); // removes from linked list
						break;
					}
					tptr = tptr->getnextplayer();
				}
			}
#endif
			if (MCprocesses[i].ihandle != NULL)
				MCS_close(MCprocesses[i].ihandle);
			if (MCprocesses[i].ohandle != NULL)
				MCS_close(MCprocesses[i].ohandle);
			delete MCprocesses[i].name;
			uint2 j = i;
			while (++j < MCnprocesses)
				MCprocesses[j - 1] = MCprocesses[j];
			MCnprocesses--;
		}
		else
			i++;
}

real8 IO_cleansockets(real8 ctime)
{
	real8 etime = ctime + MCmaxwait;
	uint2 i = 0;
	while (i < MCnsockets)
		if (!MCsockets[i]->waiting && MCsockets[i]->fd == 0
			&& MCsockets[i]->nread == 0 && MCsockets[i]->resolve_state != kMCSocketStateResolving)
		{
			delete MCsockets[i];
			uint2 j = i;
			while (++j < MCnsockets)
				MCsockets[j - 1] = MCsockets[j];
			MCnsockets--;
		}
		else
		{
			MCSocket *s = MCsockets[i++];
			if (!s->waiting && !s->accepting
			        && (!s->connected && ctime > s->timeout
			            || s->wevents != NULL && ctime > s->wevents->timeout
			            || s->revents != NULL && ctime > s->revents->timeout))
			{
				if (!s->connected)
					s->timeout = ctime  + MCsockettimeout;
				if (s->revents != NULL)
					s->revents->timeout = ctime + MCsockettimeout;
				if (s->wevents != NULL)
					s->wevents->timeout = ctime + MCsockettimeout;
				MCscreen->delaymessage(s->object, MCM_socket_timeout, strclone(s->name));
			}
			if (s->wevents != NULL && s->wevents->timeout < etime)
				etime = s->wevents->timeout;
			if (s->revents != NULL && s->revents->timeout < etime)
				etime = s->revents->timeout;
		}
	return etime;
}

Boolean IO_findsocket(const char *name, uint2 &i)
{
	IO_cleansockets(MCS_time());
	for (i = 0 ; i < MCnsockets ; i++)
		if (strequal(MCsockets[i]->name, name))
			return True;
	return False;
}

void IO_freeobject(MCObject *o)
{
	IO_cleansockets(MCS_time());
	uint2 i = 0;
	while (i < MCnsockets)
#if 1
	{
		if (MCsockets[i]->object == o)
			MCsockets[i]->doclose();
		i++;
	}
#else
		if (MCsockets[i]->object == o)
		{
			delete MCsockets[i];
			uint2 j = i;
			while (++j < MCnsockets)
				MCsockets[j - 1] = MCsockets[j];
			MCnsockets--;
		}
		else
			i++;
#endif
}

IO_stat IO_read(void *ptr, uint4 size, uint4 &n, IO_handle stream)
{
	return MCS_read(ptr, size, n, stream);
}

IO_stat IO_write(const void *ptr, uint4 size, uint4 n, IO_handle stream)
{
	return MCS_write(ptr, size, n, stream);
}

IO_stat IO_read_to_eof(IO_handle stream, MCExecPoint &ep)
{
	uint4 nread;
    // SN-2014-05-02 [[ Bug 12351 ]] Ensure we read the right size from a device.
    // With some of them - like mouse pointer - no error is triggered, but writing on it moves the file pointer
    // without increasing the size; that results in nread being 0 - MCS_tell, so a really large unsigned number
	nread = MCMin((uint4)MCS_fsize(stream), (uint4)MCS_fsize(stream) - (uint4)MCS_tell(stream));
	char *dptr = ep.getbuffer(nread);
	MCS_read(dptr, 1, nread, stream);
	ep.setlength(nread);
	return IO_NORMAL;
}

IO_stat IO_fgets(char *ptr, uint4 length, IO_handle stream)
{
	uint4 bytes = length;
	if (MCS_read(ptr, sizeof(uint1), bytes, stream) == IO_ERROR)
		return IO_ERROR;
	ptr[bytes - 1] = '\0';
	strtok(ptr, "\n");
	length = strlen(ptr) + 1;
	if (length != bytes)
		// IM-2014-07-31: [[ ImageLoader ]] Fix unsigned math resulting in huge positive offsets
		if (MCS_seek_cur(stream, (int64_t)length - (int64_t)bytes) != IO_NORMAL)
			return IO_ERROR;
	return IO_NORMAL;
}

IO_stat IO_read_real8(real8 *dest, IO_handle stream)
{
	uint4 n = 1;
	return MCS_read(dest, sizeof(real8), n, stream);
}

IO_stat IO_read_real4(real4 *dest, IO_handle stream)
{
	uint4 n = 1;
	return MCS_read(dest, sizeof(real4), n, stream);
}

IO_stat IO_read_uint4(uint4 *dest, IO_handle stream)
{
	uint4 n = 1;
	IO_stat stat = MCS_read(dest, sizeof(uint4), n, stream);
	if (stat != IO_ERROR)
		swap_uint4(dest);
	return stat;
}

IO_stat IO_read_int4(int4 *dest, IO_handle stream)
{
	uint4 n = 1;
	IO_stat stat = MCS_read(dest, sizeof(uint4), n, stream);
	if (stat != IO_ERROR)
		swap_int4(dest);
	return stat;
}

IO_stat IO_read_uint2(uint2 *dest, IO_handle stream)
{
	uint4 n = 1;
	IO_stat stat = MCS_read(dest, sizeof(uint2), n, stream);
	if (stat != IO_ERROR)
		swap_uint2(dest);
	return stat;
}

IO_stat IO_read_int2(int2 *dest, IO_handle stream)
{
	uint4 n = 1;
	IO_stat stat = MCS_read(dest, sizeof(int2), n, stream);
	if (stat != IO_ERROR)
		swap_int2(dest);
	return stat;
}

IO_stat IO_read_uint1(uint1 *dest, IO_handle stream)
{
	uint4 n = 1;
	return MCS_read(dest, sizeof(uint1), n, stream);
}

IO_stat IO_read_int1(int1 *dest, IO_handle stream)
{
	uint4 n = 1;
	return MCS_read(dest, sizeof(int1), n, stream);
}

IO_stat IO_write_real8(real8 dest, IO_handle stream)
{
	return MCS_write(&dest, sizeof(real8), 1, stream);
}

IO_stat IO_write_real4(real4 dest, IO_handle stream)
{
	return MCS_write(&dest, sizeof(real4), 1, stream);
}

IO_stat IO_write_uint4(uint4 dest, IO_handle stream)
{
	swap_uint4(&dest);
	return MCS_write(&dest, sizeof(uint4), 1, stream);
}

IO_stat IO_write_int4(int4 dest, IO_handle stream)
{
	swap_int4(&dest);
	return MCS_write(&dest, sizeof(int4), 1, stream);
}

IO_stat IO_write_uint2(uint2 dest, IO_handle stream)
{
	swap_uint2(&dest);
	return MCS_write(&dest, sizeof(uint2), 1, stream);
}

IO_stat IO_write_int2(int2 dest, IO_handle stream)
{
	swap_int2(&dest);
	return MCS_write(&dest, sizeof(int2), 1, stream);
}

IO_stat IO_write_uint1(uint1 dest, IO_handle stream)
{
	return MCS_write(&dest, sizeof(uint1), 1, stream);
}

IO_stat IO_write_int1(int1 dest, IO_handle stream)
{
	return MCS_write(&dest, sizeof(int1), 1, stream);
}

void IO_iso_to_mac(char *string, uint4 len)
{
	uint1 *sptr = (uint1 *)string;
	while (len--)
	{
		*sptr = MCisotranslations[*sptr];
		sptr++;
	}
}

void IO_mac_to_iso(char *string, uint4 len)
{
	uint1 *sptr = (uint1 *)string;
	while (len--)
	{
		*sptr = MCmactranslations[*sptr];
		sptr++;
	}
}

IO_stat IO_read_string_no_translate(char*& string, IO_handle stream, uint1 size)
{
	Boolean t_old_translatechars;
	t_old_translatechars = MCtranslatechars;
	MCtranslatechars = False;
	
	IO_stat t_stat;
	t_stat = IO_read_string(string, stream, size);
	
	MCtranslatechars = t_old_translatechars;
	
	return t_stat;
}

IO_stat IO_read_string(char *&r_string, uint32_t &r_length, IO_handle p_stream, uint8_t p_size, bool p_includes_null, bool p_translate)
{
	IO_stat stat;
	
	uint32_t t_length = 0;
	uint32_t t_bytes = 0;
	
	switch (p_size)
	{
		case 1:
		{
			uint8_t t_len;
			if ((stat = IO_read_uint1(&t_len, p_stream)) != IO_NORMAL)
				return stat;
			t_bytes = t_len;
			break;
		}
		case 2:
		{
			uint16_t t_len;
			if ((stat = IO_read_uint2(&t_len, p_stream)) != IO_NORMAL)
				return stat;
			t_bytes = t_len;
			break;
		}
		case 4:
		{
			uint32_t t_len;
			if ((stat = IO_read_uint4(&t_len, p_stream)) != IO_NORMAL)
				return stat;
			t_bytes = t_len;
			break;
		}
	}
	
	char *t_string = nil;
	if (t_bytes != 0)
	{
		t_length = p_includes_null ? t_bytes - 1 : t_bytes;
		/* UNCHECKED */ t_string = new char[t_bytes];
		stat = MCStackSecurityRead(t_string, t_length, p_stream);
		if (stat == IO_NORMAL && p_includes_null)
			stat = IO_read_uint1((uint1*)t_string + t_length, p_stream);
		if (stat != IO_NORMAL)
		{
			delete t_string;
			return stat;
		}

		if (MCtranslatechars && p_translate)
		{
#ifdef __MACROMAN__
			IO_iso_to_mac(t_string, t_length);
#else
			IO_mac_to_iso(t_string, t_length);
#endif
			
		}
	}
	
	r_string = t_string;
	r_length = t_length;
	
	return IO_NORMAL;
}

IO_stat IO_read_string(char *&r_string, IO_handle stream, uint1 size)
{
	uint32_t t_length = 0;
	return IO_read_string(r_string, t_length, stream, size, true, true);
}

IO_stat IO_read_string(char *&string, uint4 &outlen, IO_handle stream,
                       bool isunicode, uint1 size)
{
	IO_stat stat;
	
	char *t_string = nil;
	uint32_t t_length = 0;
	
	if ((stat = IO_read_string(t_string, t_length, stream, size, true, !isunicode)) != IO_NORMAL)
		return stat;

	if (isunicode)
	{
		uint2 *dptr = (uint2 *)t_string;
		uint4 len = t_length >> 1;
		while (len--)
			swap_uint2(dptr++);
	}

	string = t_string;
	outlen = t_length;
	
	return IO_NORMAL;
}

IO_stat IO_write_string(const MCString &p_string, IO_handle p_stream, uint8_t p_size, bool p_write_null)
{
	IO_stat stat = IO_NORMAL;
	uint32_t t_strlen = p_string.getlength();
	uint4 length = 0;
	uint32_t t_inc = p_write_null ? 1 : 0;
	switch (p_size)
	{
		case 1:
		{
			uint1 len = t_strlen == 0 ? 0 : MCU_min(t_strlen + t_inc, MAXUINT1);
			if ((stat = IO_write_uint1(len, p_stream)) != IO_NORMAL)
				return stat;
			length = len;
			break;
		}
		case 2:
		{
			uint2 len = t_strlen == 0 ? 0 : MCU_min(t_strlen + t_inc, MAXUINT2);
			if ((stat = IO_write_uint2(len, p_stream)) != IO_NORMAL)
				return stat;
			length = len;
			break;
		}
		case 4:
			length = t_strlen == 0 ? 0 : t_strlen + t_inc;
			if ((stat = IO_write_uint4(length, p_stream)) != IO_NORMAL)
				return stat;
			break;
	}
	if (length)
	{
		stat = MCStackSecurityWrite(p_string.getstring(), length - t_inc, p_stream);
		if (stat == IO_NORMAL && p_write_null)
			stat = IO_write_uint1(0, p_stream);
	}
	return stat;
}

IO_stat IO_write_string(const char *string, IO_handle stream, uint1 size)
{
	return IO_write_string(MCString(string), stream, size);
}

IO_stat IO_write_string(const char *string, uint4 outlen, IO_handle stream,
                        Boolean isunicode, uint1 size)
{
	IO_stat stat = IO_NORMAL;
	if (isunicode)
	{
		uint16_t *t_uniptr = (uint16_t*)string;
		uint32_t t_len = outlen / 2;
		while (t_len--)
			swap_uint2(t_uniptr++);
	}
	stat = IO_write_string(MCString(string, outlen), stream, size, true);
	if (isunicode)
	{
		uint16_t *t_uniptr = (uint16_t*)string;
		uint32_t t_len = outlen / 2;
		while (t_len--)
			swap_uint2(t_uniptr++);
	}
	
	return stat;
}

// MW-2009-06-30: New IO method reads a block of bytes and fails if there is
//   not enough to satisfy the request.
IO_stat IO_read_bytes(void *ptr, uint4 size, IO_handle stream)
{
	uint4 n;
	n = 1;
	if (MCS_read(ptr, size, n, stream) != IO_NORMAL ||
		n != 1)
		return IO_ERROR;
	return IO_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

IO_stat IO_read_mccolor(MCColor& r_color, IO_handle p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (t_stat == IO_NORMAL)
		t_stat = IO_read_uint2(&r_color . red, p_stream);

	if (t_stat == IO_NORMAL)
		t_stat = IO_read_uint2(&r_color . green, p_stream);

	if (t_stat == IO_NORMAL)
		t_stat = IO_read_uint2(&r_color . blue, p_stream);

	return t_stat;
}

IO_stat IO_write_mccolor(const MCColor& p_color, IO_handle p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint2(p_color . red, p_stream);

	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint2(p_color . green, p_stream);

	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint2(p_color . blue, p_stream);

	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////

// Read/write a variable length integer. The value is either 2 or 4 bytes long.
// If the top-most bit is set in the first 2 byte word, then the second 2 word
// is present - this allows values up to 16383 to be represented with 2 bytes
// and values up to 2^31-1 in 4.

IO_stat IO_read_uint2or4(uint4 *dest, IO_handle stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;
	
	// Read in the first 2-byte word.
	uint16_t t_first;
	t_stat = IO_read_uint2(&t_first, stream);
	if (t_stat != IO_NORMAL)
		return t_stat;
		
	// If the top bit is clear, then we are done so return the value.
	if ((t_first & (1 << 15U)) == 0)
	{
		*dest = t_first;
		return IO_NORMAL;
	}

	// Read in the second 2-byte word.
	uint16_t t_second;
	t_stat = IO_read_uint2(&t_second, stream);
	if (t_stat != IO_NORMAL)
		return t_stat;
	
	// Combine first and second words.
	*dest = (t_first & 0x7fff) | (t_second << 15);
	return IO_NORMAL;
}

IO_stat IO_write_uint2or4(uint4 dest, IO_handle stream)
{
	// If we are writing out a value < 16384, we only need one 2-byte word.
	if (dest < 16384)
		return IO_write_uint2(dest, stream);
		
	// Write out the lowest 15 bits of the value, marking it as having another
	// word by setting the top bit.
	IO_stat t_stat;
	t_stat = IO_write_uint2((dest & 0x7fff) | 0x8000, stream);
	
	// Now write out the remaining 16 top bits.
	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint2(dest >> 15, stream);
	
	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////

IO_stat IO_read_nameref(MCNameRef& r_name, IO_handle stream, uint1 size)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	char *t_string;
	t_string = nil;
	if (t_stat == IO_NORMAL)
		t_stat = IO_read_string(t_string, stream, size);

	if (t_stat == IO_NORMAL &&
		!MCNameCreateWithCString(t_string != nil ? t_string : MCnullstring, r_name))
		t_stat = IO_ERROR;

	delete t_string;

	return t_stat;
}

IO_stat IO_write_nameref(MCNameRef p_name, IO_handle stream, uint1 size)
{
	// MW-2011-10-21: [[ Bug 9826 ]] If the name is empty, write out nil string.
	return IO_write_string(MCNameIsEmpty(p_name) ? nil : MCNameGetCString(p_name), stream, size);
}

////////////////////////////////////////////////////////////////////////////////

int64_t MCS_fake_fsize(IO_handle stream)
{
	return 0;
}

int64_t MCS_fake_tell(IO_handle stream)
{
	MCFakeOpenCallbacks *t_callbacks;
	t_callbacks = (MCFakeOpenCallbacks *)stream -> len;
	if (t_callbacks -> tell == NULL)
		return IO_ERROR;
	return t_callbacks -> tell(stream -> buffer);
}

IO_stat MCS_fake_seek_cur(IO_handle stream, int64_t offset)
{
	MCFakeOpenCallbacks *t_callbacks;
	t_callbacks = (MCFakeOpenCallbacks *)stream -> len;
	if (t_callbacks -> seek_cur == NULL)
		return IO_ERROR;
	return t_callbacks -> seek_cur(stream -> buffer, (int32_t)offset);
}

IO_stat MCS_fake_seek_set(IO_handle stream, int64_t offset)
{
	MCFakeOpenCallbacks *t_callbacks;
	t_callbacks = (MCFakeOpenCallbacks *)stream -> len;
	if (t_callbacks -> seek_set == NULL)
		return IO_ERROR;
	return t_callbacks -> seek_set(stream -> buffer, (int32_t)offset);
}

IO_stat MCS_fake_read(void *ptr, uint4 size, uint4 &n, IO_handle stream)
{
	uint32_t t_amount;
	t_amount = size * n;

	MCFakeOpenCallbacks *t_callbacks;
	t_callbacks = (MCFakeOpenCallbacks *)stream -> len;
	if (t_callbacks -> read == NULL)
		return IO_ERROR;

	uint32_t t_amount_read;
	IO_stat t_stat;
	t_stat = t_callbacks -> read(stream -> buffer, ptr, t_amount, t_amount_read);

	if (t_amount_read < t_amount)
	{
		stream -> flags |= IO_ATEOF;
		t_stat = IO_EOF;
	}
	else
		stream -> flags &= ~IO_ATEOF;

	n = t_amount / size;

	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////
