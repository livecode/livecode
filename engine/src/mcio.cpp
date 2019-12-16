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

#include "exec.h"
#include "player.h"
#include "osspec.h"
#include "globals.h"
#include "util.h"
#include "socket.h"
#include "uidc.h"

#include "stacksecurity.h"

bool IO_findstream(Streamnode *p_nodes, uindex_t p_node_count, MCNameRef p_name, uindex_t& r_index)
{
	while (p_node_count-- > 0)
	{
		if (MCNameIsEqualTo(p_name, p_nodes[p_node_count].name, kMCStringOptionCompareExact))
		{
			r_index = p_node_count;
			return true;
		}
	}
	return false;
}

bool IO_findfile(MCNameRef p_name, uindex_t& r_index)
{
	return IO_findstream(MCfiles, MCnfiles, p_name, r_index);
}

Boolean IO_closefile(MCNameRef name)
{
	uindex_t index;
	if (IO_findfile(name, index))
	{
		if (MCfiles[index].ihandle != NULL)
			MCS_close(MCfiles[index].ihandle);
		else
		{
			MCS_close(MCfiles[index].ohandle);
		}
		MCValueRelease(MCfiles[index].name);
		while (++index < MCnfiles)
			MCfiles[index - 1] = MCfiles[index];
		MCnfiles--;
		return True;
	}
	return False;
}

bool IO_findprocess(MCNameRef p_name, uindex_t& r_index)
{
	/* TODO - update processes to use MCNameRef */
	IO_cleanprocesses();
	return IO_findstream(MCprocesses, MCnprocesses, p_name, r_index);
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
				MCPlayerHandle t_player = MCplayers;
				while (t_player.IsValid())
				{
					if (MCStringIsEqualToCString(MCNameGetString(MCprocesses[i].name),
                                                 t_player->getcommand(),
                                                 kMCCompareExact))
					{
						t_player->playstop(); // removes from linked list
						break;
					}
					t_player = t_player->getnextplayer();
				}
			}
#endif
			if (MCprocesses[i].ihandle != NULL)
				MCS_close(MCprocesses[i].ihandle);
			if (MCprocesses[i].ohandle != NULL)
				MCS_close(MCprocesses[i].ohandle);
			MCValueRelease(MCprocesses[i].name);
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
            MCSocketsRemoveFromSocketList(i);
		else
		{
			MCSocket *s = MCsockets[i++];
			if (!s->waiting && !s->accepting
			    && ((!s->connected && ctime > s->timeout)
			        || (s->wevents != NULL && ctime > s->wevents->timeout)
			        || (s->revents != NULL && ctime > s->revents->timeout)))
			{
				if (!s->connected)
					s->timeout = ctime  + MCsockettimeout;
				if (s->revents != NULL)
					s->revents->timeout = ctime + MCsockettimeout;
				if (s->wevents != NULL)
					s->wevents->timeout = ctime + MCsockettimeout;
                
                if (s->object.IsValid())
                    MCscreen->delaymessage(s->object, MCM_socket_timeout, MCNameGetString(s->name));
			}
			if (s->wevents != NULL && s->wevents->timeout < etime)
				etime = s->wevents->timeout;
			if (s->revents != NULL && s->revents->timeout < etime)
				etime = s->revents->timeout;
		}
	return etime;
}

bool IO_findsocket(MCNameRef p_name, uindex_t& r_index)
{
	IO_cleansockets(MCS_time());
	for (r_index = 0 ; r_index < MCnsockets ; r_index++)
		if (MCNameIsEqualToCaseless(p_name, MCsockets[r_index]->name))
			return true;
	return false;
}

void IO_freeobject(MCObject *o)
{
	IO_cleansockets(MCS_time());
	uint2 i = 0;
	while (i < MCnsockets)
#if 1
	{
		if (!MCsockets[i]->object.IsValid() ||
            MCsockets[i]->object == o)
			MCsockets[i]->doclose();
		i++;
	}
#else
		if (MCsockets[i]->object == o)
            MCSocketsRemoveFromSocketList(i);
		else
			i++;
#endif
}

IO_stat IO_read(void *ptr, uint4 byte_size, IO_handle stream)
{
	return MCS_readfixed(ptr, byte_size, stream);
}

IO_stat IO_write(const void *ptr, uint4 size, uint4 n, IO_handle stream)
{
	return MCS_write(ptr, size, n, stream);
}

IO_stat IO_read_to_eof(IO_handle stream, MCDataRef& r_data)
{
	uint4 nread;
    // SN-2014-05-02 [[ Bug 12351 ]] Ensure we read the right size from a device.
    // With some of them - like mouse pointer - no error is triggered, but writing on it moves the file pointer
    // without increasing the size; that results in nread being 0 - MCS_tell, so a really large unsigned number
	nread = MCMin((uint4)MCS_fsize(stream), (uint4)MCS_fsize(stream) - (uint4)MCS_tell(stream));
	void *t_stream;
	/* UNCHECKED */ MCMemoryAllocate(nread, t_stream);
	/* UNCHECKED */ MCS_readall(t_stream, nread, stream, nread);
	/* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t*)t_stream, nread, r_data);
	return IO_NORMAL;
}

IO_stat IO_fgets(char *ptr, uint4 length, IO_handle stream)
{
	uint4 bytes = length;
	if (MCS_readall(ptr, bytes, stream, bytes) == IO_ERROR)
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
	return MCS_readfixed(dest, sizeof(real8), stream);
}

IO_stat IO_read_real4(real4 *dest, IO_handle stream)
{
	return MCS_readfixed(dest, sizeof(real4), stream);
}

IO_stat IO_read_uint4(uint4 *dest, IO_handle stream)
{
	IO_stat stat = MCS_readfixed(dest, sizeof(uint4), stream);
	if (stat != IO_ERROR)
		swap_uint4(dest);
	return stat;
}

IO_stat IO_read_int4(int4 *dest, IO_handle stream)
{
	IO_stat stat = MCS_readfixed(dest, sizeof(uint4), stream);
	if (stat != IO_ERROR)
		swap_int4(dest);
	return stat;
}

IO_stat IO_read_uint2(uint2 *dest, IO_handle stream)
{
	IO_stat stat = MCS_readfixed(dest, sizeof(uint2), stream);
	if (stat != IO_ERROR)
		swap_uint2(dest);
	return stat;
}

IO_stat IO_read_int2(int2 *dest, IO_handle stream)
{
	IO_stat stat = MCS_readfixed(dest, sizeof(int2), stream);
	if (stat != IO_ERROR)
		swap_int2(dest);
	return stat;
}

IO_stat IO_read_uint1(uint1 *dest, IO_handle stream)
{
	return MCS_readfixed(dest, sizeof(uint1), stream);
}

IO_stat IO_read_int1(int1 *dest, IO_handle stream)
{
	return MCS_readfixed(dest, sizeof(int1), stream);
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

MCStringEncoding MCS_file_to_string_encoding(MCFileEncodingType p_encoding)
{
    switch(p_encoding)
    {
    case kMCFileEncodingUTF8:
        return kMCStringEncodingUTF8;

    case kMCFileEncodingUTF16:
        return kMCStringEncodingUTF16;

    case kMCFileEncodingUTF16LE:
        return kMCStringEncodingUTF16LE;

    case kMCFileEncodingUTF16BE:
        return kMCStringEncodingUTF16BE;
            
    case kMCFileEncodingUTF32:
        return kMCStringEncodingUTF32;
            
    case kMCFileEncodingUTF32BE:
        return kMCStringEncodingUTF32BE;
            
    case kMCFileEncodingUTF32LE:
        return kMCStringEncodingUTF32LE;

    default:
        return kMCStringEncodingNative;
    }
}

IO_stat IO_read_string_no_translate(char*& string, IO_handle stream, uint1 size)
{
	uint32_t t_length;
	return IO_read_string_legacy_full(string, t_length, stream, size, true, false);
}

IO_stat IO_read_string_legacy_full(char *&r_string, uint32_t &r_length, IO_handle p_stream, uint8_t p_size, bool p_includes_null, bool p_translate)
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
	
	MCAutoCustomPointer<char,MCMemoryDeallocate> t_string;
	if (t_bytes != 0)
	{
		t_length = p_includes_null ? t_bytes - 1 : t_bytes;
		if (!MCMemoryAllocate(t_bytes, &t_string))
			return IO_ERROR;
		stat = MCStackSecurityRead(*t_string, t_length, p_stream);
		if (stat == IO_NORMAL && p_includes_null)
			stat = IO_read_uint1(reinterpret_cast<uint1*>(*t_string) + t_length,
                                 p_stream);
		if (stat != IO_NORMAL)
			return stat;

		if (MCtranslatechars && p_translate)
		{
#ifdef __MACROMAN__
			IO_iso_to_mac(*t_string, t_length);
#else
			IO_mac_to_iso(*t_string, t_length);
#endif
			
		}
	}
	
	r_string = t_string.Release();
	r_length = t_length;
	
	return IO_NORMAL;
}

IO_stat IO_read_cstring_legacy(char *&r_string, IO_handle stream, uint1 size)
{
	uint32_t t_length = 0;
	return IO_read_string_legacy_full(r_string, t_length, stream, size, true, true);
}

IO_stat IO_discard_cstring_legacy(IO_handle stream, uint1 size)
{
    /* TODO[2017-02-06] Refactor so that this doesn't allocate any
     * memory, rather than inefficiently allocating a buffer and then
     * immediately freeing it. */
	MCAutoCustomPointer<char,MCMemoryDeallocate> t_discarded;
	return IO_read_cstring_legacy(&t_discarded, stream, size);
}

IO_stat IO_write_string_legacy_full(const MCString &p_string, IO_handle p_stream, uint8_t p_size, bool p_write_null)
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

IO_stat IO_write_cstring_legacy(const char *string, IO_handle stream, uint1 size)
{
	return IO_write_string_legacy_full(MCString(string), stream, size, true);
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

// MW-2013-11-20: [[ UnicodeFileFormat ]] If as_unicode is false, this reads a
//   native string; otherwise it reads a byte-swapped UTF-16 string.
IO_stat IO_read_stringref_legacy(MCStringRef& r_string, IO_handle p_stream, bool p_as_unicode, uint1 p_size)
{
	IO_stat stat = IO_NORMAL;
	MCStringEncoding t_encoding = p_as_unicode ? kMCStringEncodingUTF16BE : kMCStringEncodingNative;
	
	uint4 t_length;
	char *t_bytes;
	if ((stat = IO_read_string_legacy_full(t_bytes, t_length, p_stream, p_size, true, !p_as_unicode)) != IO_NORMAL)
		return stat;
	
	if (!MCStringCreateWithBytesAndRelease((byte_t *)t_bytes, t_length, t_encoding, false, r_string))
	{
		MCMemoryDeallocate(t_bytes);
		return IO_ERROR;
	}
	
	return IO_NORMAL;
}

// MW-2013-11-20: [[ UnicodeFileFormat ]] If 'supports_unicode' is false, then this
//   reads the stringref as native; otherwise it expects a self-describing string.
IO_stat IO_read_stringref_new(MCStringRef& r_string, IO_handle p_stream, bool p_supports_unicode, uint1 p_size)
{
	if (!p_supports_unicode)
		return IO_read_stringref_legacy(r_string, p_stream, false, p_size);
	
	uint32_t t_length;
	if (IO_read_uint2or4(&t_length, p_stream) != IO_NORMAL)
		return IO_ERROR;
	
    if (MCStackSecurityReadUTF8StringRef(r_string, t_length, p_stream) != IO_NORMAL)
        return IO_ERROR;
	
	return IO_NORMAL;
}

IO_stat IO_write_stringref_legacy(MCStringRef p_string, IO_handle p_stream, bool p_as_unicode, uint1 p_size)
{
	IO_stat stat = IO_NORMAL;
	MCStringEncoding t_encoding = p_as_unicode ? kMCStringEncodingUTF16BE : kMCStringEncodingNative;
	
	MCDataRef t_data = nil;
	if (!MCStringEncode(p_string, t_encoding, false, t_data))
		return IO_ERROR;
	
	uindex_t t_length = MCDataGetLength(t_data);
	const char *t_bytes = (const char *)MCDataGetBytePtr(t_data);
	stat = IO_write_string_legacy_full(MCString(t_bytes, t_length), p_stream, p_size, true);
	MCValueRelease(t_data);
	return stat;
}

IO_stat IO_write_stringref_new(MCStringRef p_string, IO_handle p_stream, bool p_supports_unicode, uint1 p_size)
{
	if (!p_supports_unicode)
		return IO_write_stringref_legacy(p_string, p_stream, false, p_size);
	
	MCAutoStringRefAsUTF8String t_utf8_string;
	if (!t_utf8_string.Lock(p_string))
		return IO_ERROR;
	
	if (IO_write_uint2or4(t_utf8_string.Size(), p_stream) != IO_NORMAL)
		return IO_ERROR;
	
	if (MCStackSecurityWrite(*t_utf8_string, t_utf8_string.Size(), p_stream) != IO_NORMAL)
		return IO_ERROR;
		
	return IO_NORMAL;
}

//////////

IO_stat IO_read_nameref_legacy(MCNameRef& r_name, IO_handle p_stream, bool p_as_unicode, uint1 p_size)
{
	IO_stat t_stat;
	MCAutoStringRef t_string;
	t_stat = IO_read_stringref_legacy(&t_string, p_stream, p_as_unicode, p_size);
	if (t_stat == IO_NORMAL &&
		!MCNameCreate(*t_string, r_name))
		t_stat = IO_ERROR;
	return t_stat;
}

IO_stat IO_write_nameref_legacy(MCNameRef p_name, IO_handle p_stream, bool p_as_unicode, uint1 p_size)
{
	return IO_write_stringref_legacy(MCNameGetString(p_name), p_stream, p_as_unicode, p_size);
}

IO_stat IO_read_nameref_new(MCNameRef& r_name, IO_handle p_stream, bool p_supports_unicode, uint1 p_size)
{
	IO_stat t_stat;
	MCAutoStringRef t_string;
	t_stat = IO_read_stringref_new(&t_string, p_stream, p_supports_unicode, p_size);
	if (t_stat == IO_NORMAL &&
		!MCNameCreate(*t_string, r_name))
		t_stat = IO_ERROR;
	return t_stat;
}

IO_stat IO_write_nameref_new(MCNameRef p_name, IO_handle p_stream, bool p_supports_unicode, uint1 p_size)
{
	return IO_write_stringref_new(MCNameGetString(p_name), p_stream, p_supports_unicode, p_size);
}

//////////

IO_stat IO_read_stringref_legacy_utf8(MCStringRef& r_string, IO_handle stream, uint1 size)
{
	// Read in the UTF-8 string and create a StringRef
	IO_stat stat = IO_NORMAL;
	char *t_bytes = nil;
	uint4 t_length = 0;
	if ((stat = IO_read_string_legacy_full(t_bytes, t_length, stream, size, true, false)) != IO_NORMAL)
		return stat;
	if (!MCStringCreateWithBytesAndRelease((byte_t *)t_bytes, t_bytes != nil ? strlen(t_bytes) : 0, kMCStringEncodingUTF8, false, r_string))
	{
		MCMemoryDeallocate (t_bytes);
		return IO_ERROR;
	}
	
	return IO_NORMAL;
}

IO_stat IO_write_stringref_legacy_utf8(MCStringRef p_string, IO_handle stream, uint1 size)
{
	// Convert the string to UTF-8 encoding before writing it out
	IO_stat stat;
	char *t_bytes = nil;
	uindex_t t_length = 0;
	if (!MCStringConvertToUTF8(p_string, t_bytes, t_length))
		return IO_ERROR;
	stat = IO_write_string_legacy_full(MCString(t_bytes, t_length), stream, size, true);
	MCMemoryDeleteArray(t_bytes);
	return stat;
}

//////////

enum
{
	IO_VALUEREF_NULL,
	IO_VALUEREF_BOOLEAN_FALSE,
	IO_VALUEREF_BOOLEAN_TRUE,
	IO_VALUEREF_NUMBER_INTEGER,
	IO_VALUEREF_NUMBER_DOUBLE,
	IO_VALUEREF_NAME_EMPTY,
	IO_VALUEREF_NAME_ANY,
	IO_VALUEREF_STRING_EMPTY,
	IO_VALUEREF_STRING_ANY,
	IO_VALUEREF_DATA_EMPTY,
	IO_VALUEREF_DATA_ANY,
	IO_VALUEREF_ARRAY_EMPTY,
	IO_VALUEREF_ARRAY_SEQUENCE,
	IO_VALUEREF_ARRAY_MAP,
    IO_VALUEREF_LIST_EMPTY,
    IO_VALUEREF_LIST_ANY,
};

IO_stat IO_write_valueref_new(MCValueRef p_value, IO_handle p_stream)
{
	IO_stat t_stat;
	switch(MCValueGetTypeCode(p_value))
	{
		case kMCValueTypeCodeNull:
			t_stat = IO_write_uint1(IO_VALUEREF_NULL, p_stream);
			break;
		case kMCValueTypeCodeBoolean:
			t_stat = IO_write_uint1(p_value == kMCFalse ? IO_VALUEREF_BOOLEAN_FALSE : IO_VALUEREF_BOOLEAN_TRUE, p_stream);
			break;
		case kMCValueTypeCodeNumber:
			if (MCNumberIsInteger((MCNumberRef)p_value))
			{
				t_stat = IO_write_uint1(IO_VALUEREF_NUMBER_INTEGER, p_stream);
				if (t_stat == IO_NORMAL)
					t_stat = IO_write_int4(MCNumberFetchAsInteger((MCNumberRef)p_value), p_stream);
			}
			else
			{
				t_stat = IO_write_uint1(IO_VALUEREF_NUMBER_DOUBLE, p_stream);
				if (t_stat == IO_NORMAL)
					t_stat = IO_write_real8(MCNumberFetchAsReal((MCNumberRef)p_value), p_stream);
			}
			break;
		case kMCValueTypeCodeName:
			if (MCNameIsEmpty((MCNameRef)p_value))
				t_stat = IO_write_uint1(IO_VALUEREF_NAME_EMPTY, p_stream);
			else
			{
				t_stat = IO_write_uint1(IO_VALUEREF_NAME_ANY, p_stream);
				if (t_stat == IO_NORMAL)
					t_stat = IO_write_nameref_new((MCNameRef)p_value, p_stream, true);
			}
			break;
		case kMCValueTypeCodeString:
			if (MCStringIsEmpty((MCStringRef)p_value))
				t_stat = IO_write_uint1(IO_VALUEREF_STRING_EMPTY, p_stream);
			else
			{
				t_stat = IO_write_uint1(IO_VALUEREF_STRING_ANY, p_stream);
				if (t_stat == IO_NORMAL)
					t_stat = IO_write_stringref_new((MCStringRef)p_value, p_stream, true);
			}
			break;
		case kMCValueTypeCodeData:
			if (MCDataIsEmpty((MCDataRef)p_value))
				t_stat = IO_write_uint1(IO_VALUEREF_DATA_EMPTY, p_stream);
			else
			{
				t_stat = IO_write_uint1(IO_VALUEREF_DATA_ANY, p_stream);
				if (t_stat == IO_NORMAL)
					t_stat = IO_write_uint4(MCDataGetLength((MCDataRef)p_value), p_stream);
				if (t_stat == IO_NORMAL)
					t_stat = MCStackSecurityWrite((const char *)MCDataGetBytePtr((MCDataRef)p_value), MCDataGetLength((MCDataRef)p_value), p_stream);
			}
			break;
		case kMCValueTypeCodeArray:
		{
			if (MCArrayIsEmpty((MCArrayRef)p_value))
				t_stat = IO_write_uint1(IO_VALUEREF_ARRAY_EMPTY, p_stream);
			else if (MCArrayIsSequence((MCArrayRef)p_value))
			{
				t_stat = IO_write_uint1(IO_VALUEREF_ARRAY_SEQUENCE, p_stream);
				if (t_stat == IO_NORMAL)
					t_stat = IO_write_uint4(MCArrayGetCount((MCArrayRef)p_value), p_stream);
				for(uindex_t i = 1; t_stat == IO_NORMAL && i <= MCArrayGetCount((MCArrayRef)p_value); i++)
				{
					MCValueRef t_element;
					if (!MCArrayFetchValueAtIndex((MCArrayRef)p_value, i, t_element))
						t_stat = IO_ERROR;
					if (t_stat == IO_NORMAL)
						t_stat = IO_write_valueref_new(t_element, p_stream);
				}
			}
			else
			{
				t_stat = IO_write_uint1(IO_VALUEREF_ARRAY_MAP, p_stream);
				if (t_stat == IO_NORMAL)
					t_stat = IO_write_uint4(MCArrayGetCount((MCArrayRef)p_value), p_stream);
					
				uintptr_t t_iterator;
				MCNameRef t_key;
				MCValueRef t_value;
				t_iterator = 0;
				while(t_stat == IO_NORMAL &&
					  MCArrayIterate((MCArrayRef)p_value, t_iterator, t_key, t_value))
				{
					t_stat = IO_write_nameref_new(t_key, p_stream, true);
					if (t_stat == IO_NORMAL)
						t_stat = IO_write_valueref_new(t_value, p_stream);
				}
			}
		}	
		break;
        case kMCValueTypeCodeProperList:
        {
            if (MCProperListIsEmpty((MCProperListRef)p_value))
                t_stat = IO_write_uint1(IO_VALUEREF_LIST_EMPTY, p_stream);
            else
            {
				t_stat = IO_write_uint1(IO_VALUEREF_LIST_ANY, p_stream);
				if (t_stat == IO_NORMAL)
					t_stat = IO_write_uint4(MCProperListGetLength((MCProperListRef)p_value), p_stream);
				for(uindex_t i = 0; t_stat == IO_NORMAL && i < MCProperListGetLength((MCProperListRef)p_value); i++)
				{
					if (t_stat == IO_NORMAL)
						t_stat = IO_write_valueref_new(MCProperListFetchElementAtIndex((MCProperListRef)p_value, i), p_stream);
				}
            }
        }
        break;
		default:
            MCAssert(false);
			return IO_ERROR;
	}
	return t_stat;
}

IO_stat IO_read_valueref_new(MCValueRef& r_value, IO_handle p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;
	
	uint1 t_type;
	if (t_stat == IO_NORMAL)
		t_stat = IO_read_uint1(&t_type, p_stream);
	
	if (t_stat == IO_NORMAL)
		switch(t_type)
		{
			case IO_VALUEREF_NULL:
				r_value = MCValueRetain(kMCNull);
				break;
			case IO_VALUEREF_BOOLEAN_FALSE:
				r_value = MCValueRetain(kMCFalse);
				break;
			case IO_VALUEREF_BOOLEAN_TRUE:
				r_value = MCValueRetain(kMCTrue);
				break;
			case IO_VALUEREF_NUMBER_INTEGER:
			{
				int4 t_integer;
				t_stat = IO_read_int4(&t_integer, p_stream);
				if (t_stat == IO_NORMAL &&
					!MCNumberCreateWithInteger(t_integer, (MCNumberRef&)r_value))
					t_stat = IO_ERROR;
			}
			break;
			case IO_VALUEREF_NUMBER_DOUBLE:
			{
				double t_double;
				t_stat = IO_read_real8(&t_double, p_stream);
				if (t_stat == IO_NORMAL &&
					!MCNumberCreateWithReal(t_double, (MCNumberRef&)r_value))
					t_stat = IO_ERROR;
			}
			break;
			case IO_VALUEREF_NAME_EMPTY:
				r_value = MCValueRetain(kMCEmptyName);
				break;
			case IO_VALUEREF_NAME_ANY:
				t_stat = IO_read_nameref_new((MCNameRef&)r_value, p_stream, true);
				break;
			case IO_VALUEREF_STRING_EMPTY:
				r_value = MCValueRetain(kMCEmptyString);
				break;
			case IO_VALUEREF_STRING_ANY:
				t_stat = IO_read_stringref_new((MCStringRef&)r_value, p_stream, true);
				break;
			case IO_VALUEREF_DATA_EMPTY:
				r_value = MCValueRetain(kMCEmptyData);
				break;
			case IO_VALUEREF_DATA_ANY:
			{
				uint4 t_length;
				t_stat = IO_read_uint4(&t_length, p_stream);
				
				uint8_t *t_bytes;
				t_bytes = nil;
				if (t_stat == IO_NORMAL &&
					!MCMemoryNewArray(t_length, t_bytes))
					t_stat = IO_ERROR;
				
				if (t_stat == IO_NORMAL)
					t_stat = MCStackSecurityRead((char *)t_bytes, t_length, p_stream);
				
				if (t_stat == IO_NORMAL &&
					!MCDataCreateWithBytesAndRelease(t_bytes, t_length, (MCDataRef&)r_value))
					t_stat = IO_ERROR;
			
				if (t_stat == IO_ERROR)
					MCMemoryDeleteArray(t_bytes);
			}
			break;
			case IO_VALUEREF_ARRAY_EMPTY:
				r_value = MCValueRetain(kMCEmptyArray);
				break;
			case IO_VALUEREF_ARRAY_SEQUENCE:
			{
				MCArrayRef t_mutable_array;
				t_mutable_array = nil;
				if (!MCArrayCreateMutable(t_mutable_array))
					t_stat = IO_ERROR;
					
				uint4 t_length;
				if (t_stat == IO_NORMAL)
					t_stat = IO_read_uint4(&t_length, p_stream);
				for(uindex_t i = 0; t_stat == IO_NORMAL && i < t_length; i++)
				{
					MCValueRef t_element;
					t_element = nil;
					
					t_stat = IO_read_valueref_new(t_element, p_stream);
					if (t_stat == IO_NORMAL &&
						!MCArrayStoreValueAtIndex(t_mutable_array, i + 1, t_element))
						t_stat = IO_ERROR;
					
					if (t_element != nil)
						MCValueRelease(t_element);
				}
				
				if (t_stat == IO_NORMAL &&
					!MCArrayCopyAndRelease(t_mutable_array, (MCArrayRef&)r_value))
					t_stat = IO_ERROR;
				
				if (t_stat == IO_ERROR &&
					t_mutable_array != nil)
					MCValueRelease(t_mutable_array);
			}
			break;
			case IO_VALUEREF_ARRAY_MAP:
			{
				MCArrayRef t_mutable_array;
				t_mutable_array = nil;
				if (!MCArrayCreateMutable(t_mutable_array))
					t_stat = IO_ERROR;
				
				uint4 t_length;
				if (t_stat == IO_NORMAL)
					t_stat = IO_read_uint4(&t_length, p_stream);
				for(uindex_t i = 0; t_stat == IO_NORMAL && i < t_length; i++)
				{
					MCNameRef t_key;
					t_key = nil;
					t_stat = IO_read_nameref_new(t_key, p_stream, true);
					
					MCValueRef t_element;
					t_element = nil;
					t_stat = IO_read_valueref_new(t_element, p_stream);
					if (t_stat == IO_NORMAL &&
						!MCArrayStoreValue(t_mutable_array, true, t_key, t_element))
						t_stat = IO_ERROR;
					
					if (t_key != nil)
						MCValueRelease(t_key);
					
					if (t_element != nil)
						MCValueRelease(t_element);
				}
				
				if (t_stat == IO_NORMAL &&
					!MCArrayCopyAndRelease(t_mutable_array, (MCArrayRef&)r_value))
					t_stat = IO_ERROR;
				
				if (t_stat == IO_ERROR &&
					t_mutable_array != nil)
					MCValueRelease(t_mutable_array);
			}
            break;
			case IO_VALUEREF_LIST_EMPTY:
				r_value = MCValueRetain(kMCEmptyProperList);
				break;
			case IO_VALUEREF_LIST_ANY:
			{
				MCProperListRef t_mutable_list;
				t_mutable_list = nil;
				if (!MCProperListCreateMutable(t_mutable_list))
					t_stat = IO_ERROR;
                
				uint4 t_length;
				if (t_stat == IO_NORMAL)
					t_stat = IO_read_uint4(&t_length, p_stream);
				for(uindex_t i = 0; t_stat == IO_NORMAL && i < t_length; i++)
				{
					MCValueRef t_element;
					t_element = nil;
					
					t_stat = IO_read_valueref_new(t_element, p_stream);
					if (t_stat == IO_NORMAL &&
						!MCProperListPushElementOntoBack(t_mutable_list, t_element))
						t_stat = IO_ERROR;
					
					if (t_element != nil)
						MCValueRelease(t_element);
				}
				
				if (t_stat == IO_NORMAL &&
					!MCProperListCopyAndRelease(t_mutable_list, (MCProperListRef&)r_value))
					t_stat = IO_ERROR;
				
				if (t_stat == IO_ERROR &&
					t_mutable_list != nil)
					MCValueRelease(t_mutable_list);
			}
            break;
            // AL-2014-08-04: [[ Bug 13056 ]] Return IO_ERROR if we don't read a valid type
            default:
                return IO_ERROR;
		}
	
	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////
