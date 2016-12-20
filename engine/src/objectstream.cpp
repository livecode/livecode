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
#include "mcio.h"
#include "globals.h"

#include "object.h"
#include "objectstream.h"

MCObjectInputStream::MCObjectInputStream(IO_handle p_stream, uint32_t p_remaining, bool p_new_format)
{
	m_stream = p_stream;
	m_buffer = NULL;
	m_frontier = 0;
	m_limit = 0;
	m_bound = 0;
	m_remaining = p_remaining;
	m_mark = 0;
    m_new_format = p_new_format;
}

MCObjectInputStream::~MCObjectInputStream(void)
{
	delete[] (char *)m_buffer; /* Allocated with new[] */
}

// Flushing reads and discards the rest of the stream
IO_stat MCObjectInputStream::Flush(void)
{
	return Read(NULL, m_remaining);
}

// The mark point is 0 where the current frontier is. It is updated whenever we will thus meaning that:
//   <stream pos> + m_mark - m_frontier
// Is the location in the stream we marked.
IO_stat MCObjectInputStream::Mark(void)
{
	m_mark = m_frontier;
	return IO_NORMAL;
}

IO_stat MCObjectInputStream::Skip(uint32_t p_length)
{
	// Take into account the current frontier
	m_mark -= m_frontier;

	// Calculate where we want the frontier to be
	m_mark += p_length;

	// We should never be skipping backwards. If we are its an error in the stream
	if (m_mark < 0)
		return IO_ERROR;

	IO_stat t_stat;

	// If mark is positive we have to advance the frontier by that amount
	if (m_mark > 0)
		t_stat = Read(NULL, m_mark);
	else
		t_stat = IO_NORMAL;

	// At this point we should have that the frontier is pointing to the original mark
	// offset by length.

	return t_stat;
}    

IO_stat MCObjectInputStream::ReadTag(uint32_t& r_flags, uint32_t& r_length, uint32_t& r_header_length)
{
	IO_stat t_stat;

	uint32_t t_tag;
	t_stat = ReadU32(t_tag);
	if (t_stat == IO_NORMAL && (t_tag & (1U << 31)) == 0)
	{
		// Top bit not set means its 23:8 tag.
		// Top 23 bits are length, bottom 8 bits are flags.
		r_flags = t_tag & 0xFF;
		r_length = t_tag >> 8;
		r_header_length = 4;
	}
	else if (t_stat == IO_NORMAL)
	{
		uint32_t t_extension;
		if (t_stat == IO_NORMAL)
			t_stat = ReadU32(t_extension);
		if (t_stat == IO_NORMAL)
		{
			r_flags = (t_tag & 0xFF) | ((t_extension & 0xFFFFFF) << 8);
			
			// MW-2010-05-06: Mask for the upper 24 bits of t_tag was incorrect, it should
			//   contain all but the 24th bit. (Reported in [[Bug 8716]]).
			r_length = ((t_tag >> 8) & 0x7fffff) | ((t_extension & 0xFF000000) >> 1);
			r_header_length = 8;
		}
	}

	return t_stat;
}

IO_stat MCObjectInputStream::ReadFloat32(float& r_value)
{
	uint32_t t_bits;

	IO_stat t_stat;
	t_stat = ReadU32(t_bits);
	if (t_stat == IO_NORMAL)
		memcpy(&r_value, &t_bits, sizeof(float));

	return t_stat;
}

// MW-2011-09-12: Hit an ICE in GCC for ARM when using &r_value directly.
IO_stat MCObjectInputStream::ReadFloat64(double& r_value)
{
	uint64_t t_bits;
	double t_value;

	IO_stat t_stat;
	t_stat = ReadU64(t_bits);
	if (t_stat == IO_NORMAL)
	{
		memcpy(&t_value, &t_bits, sizeof(double));
		r_value = t_value;
	}
		
	return t_stat;
}

IO_stat MCObjectInputStream::ReadU8(uint8_t& r_value)
{
	return Read(&r_value, 1);
}

IO_stat MCObjectInputStream::ReadU16(uint16_t& r_value)
{
	IO_stat t_stat;
	t_stat = Read(&r_value, 2);
	if (t_stat == IO_NORMAL)
		r_value = MCSwapInt16NetworkToHost(r_value);
	return t_stat;
}

IO_stat MCObjectInputStream::ReadU32(uint32_t& r_value)
{
	IO_stat t_stat;
	t_stat = Read(&r_value, 4);
	if (t_stat == IO_NORMAL)
		r_value = MCSwapInt32NetworkToHost(r_value);

	return t_stat;
}

IO_stat MCObjectInputStream::ReadU64(uint64_t& r_value)
{
	IO_stat t_stat;
	t_stat = Read(&r_value, 8);
	if (t_stat == IO_NORMAL)
#ifndef __LITTLE_ENDIAN__
		r_value = r_value;
#else
		r_value = ((r_value >> 56) | ((r_value >> 40) & 0xFF00) | ((r_value >> 24) & 0xFF0000) | ((r_value >> 8) & 0xFF000000) |
				  ((r_value & 0xFF000000) << 8) | ((r_value & 0xFF0000) << 24) | ((r_value & 0xFF00) << 40) | (r_value << 56));
#endif
	return t_stat;
}

// SN-2015-04-30: [[ Bug 15175 ]] Added ReadS32, needed to store the int8-cast
//  intenum_t MCField::alignments
IO_stat MCObjectInputStream::ReadS8(int8_t &r_value)
{
    IO_stat t_stat;
    t_stat = Read(&r_value, 1);
    return t_stat;
}

IO_stat MCObjectInputStream::ReadS16(int16_t& r_value)
{
	IO_stat t_stat;
	t_stat = Read(&r_value, 2);
	if (t_stat == IO_NORMAL)
		r_value = (signed short)MCSwapInt16NetworkToHost((unsigned short)r_value);
	return t_stat;
}

// SN-2015-04-30: [[ Bug 15175 ]] Added ReadS32, needed to store the intenum_t
//  MCField::alignments
IO_stat MCObjectInputStream::ReadS32(int32_t& r_value)
{
    IO_stat t_stat;
    t_stat = Read(&r_value, 4);
    if (t_stat == IO_NORMAL)
        r_value = (int32_t)MCSwapInt32NetworkToHost((uint32_t)r_value);
    return t_stat;
}

//

IO_stat MCObjectInputStream::ReadStringRefNew(MCStringRef &r_value, bool p_supports_unicode)
{
	if (!p_supports_unicode)
	{
        MCAutoStringRef t_string;
        if (!MCStringCreateMutable(0, &t_string))
			return IO_ERROR;
		
		bool t_finished;
		t_finished = false;

		while(!t_finished)
		{
			if (m_limit == m_frontier)
			{
				IO_stat t_stat;
				t_stat = Fill();
				if (t_stat != IO_NORMAL) 
					return t_stat;
			}

			uint32_t t_offset;
			for(t_offset = 0; t_offset < m_limit - m_frontier; ++t_offset)
				if (((char *)m_buffer)[m_frontier + t_offset] == '\0')
				{
					t_finished = true;
					break;
				}

            if(!MCStringAppendNativeChars(*t_string, (const byte_t*)m_buffer + m_frontier, t_offset))
				return IO_ERROR;

			m_frontier += t_offset;
		}
		
		m_frontier += 1;

        if (!t_string.MakeImmutable())
        {
            return IO_ERROR;
        }

        r_value = t_string.Take();
		return IO_NORMAL;
	}
	
	uint32_t t_length;
	if (ReadU32(t_length) != IO_NORMAL)
		return IO_ERROR;
	
	MCAutoArray<char> t_bytes;
	if (!t_bytes.New(t_length))
		return IO_ERROR;
	
	if (Read(t_bytes.Ptr(), t_length) != IO_NORMAL)
		return IO_ERROR;
	
	if (!MCStringCreateWithBytes((const uint8_t *)t_bytes.Ptr(), t_length, kMCStringEncodingUTF8, false, r_value))
		return IO_ERROR;
	
	return IO_NORMAL;
}

IO_stat MCObjectInputStream::ReadNameRefNew(MCNameRef& r_value, bool p_supports_unicode)
{
	MCAutoStringRef t_name_string;

	IO_stat t_stat;
	t_stat = ReadStringRefNew(&t_name_string, p_supports_unicode);
	if (t_stat == IO_NORMAL)
    {
		if (MCNameCreate(*t_name_string, r_value))
			return IO_NORMAL;
		else
			t_stat = IO_ERROR;
    }

	return t_stat;
}

IO_stat MCObjectInputStream::ReadTranslatedStringRef(MCStringRef &r_value)
{
    // Read the text as a StringRef initially (because there is no support
    // for loading as a CString anymore)
    MCStringRef t_read;
    IO_stat t_stat;
    t_stat = ReadStringRefNew(t_read, false);
    
    // Abort if the string could not be read
    if (t_stat != IO_NORMAL)
        return t_stat;
    
    // If the string needs to be converted, do so
    if (MCtranslatechars)
    {
        char_t *t_chars;
        uindex_t t_char_count;
        
        MCStringConvertToNative(t_read, t_chars, t_char_count);

#ifdef __MACROMAN__
        IO_iso_to_mac((char *)t_chars, t_char_count);
#else
        IO_mac_to_iso((char *)t_chars, t_char_count);
#endif
        
        // Conversion complete
        MCValueRelease(t_read);
        if (!MCStringCreateWithNativeCharsAndRelease(t_chars, t_char_count, t_read))
        {
            MCMemoryDeleteArray(t_chars);
            return IO_ERROR;
        }
    }
    
    // All done
    r_value = t_read;
    return IO_NORMAL;
}

IO_stat MCObjectInputStream::ReadColor(MCColor &r_color)
{
	IO_stat t_stat = IO_NORMAL;

	if (t_stat == IO_NORMAL)
		t_stat = ReadU16(r_color . red);
	if (t_stat == IO_NORMAL)
		t_stat = ReadU16(r_color . green);
	if (t_stat == IO_NORMAL)
		t_stat = ReadU16(r_color . blue);

	return t_stat;
}

//

IO_stat MCObjectInputStream::Read(void *p_buffer, uint32_t p_amount)
{
	while(p_amount > 0)
	{
		if (m_limit == m_frontier)
		{
			IO_stat t_stat;
			t_stat = Fill();
			if (t_stat != IO_NORMAL)
				return t_stat;
		}

		uint32_t t_available;
		t_available = MCU_min(m_limit - m_frontier, p_amount);

		if (p_buffer != NULL)
		{
			memcpy(p_buffer, (char *)m_buffer + m_frontier, t_available);
			p_buffer = (char *)p_buffer + t_available;
		}

		p_amount -= t_available;
		m_frontier += t_available;
	}

	return IO_NORMAL;
}

IO_stat MCObjectInputStream::Fill(void)
{
	if (m_remaining == 0)
		return IO_EOF;

	IO_stat t_stat;

	if (m_buffer == nil)
		m_buffer = new (nothrow) char[16384];
	
	if (m_buffer == nil)
		return IO_ERROR;
	
	// Move remaining data to start of buffer
	memmove(m_buffer, (char *)m_buffer + m_frontier, m_bound - m_frontier);
	m_limit -= m_frontier;
	m_bound -= m_frontier;
	m_mark -= m_frontier;
	m_frontier = 0;

	// Compute the amount of data to read - this is the minimum of the remaining
	// number of bytes and the remaining space in the buffer. The buffer is fixed
	// at 16K.
	uint32_t t_available;
	t_available = MCU_min(m_remaining, 16384 - m_bound);

	t_stat = MCS_readfixed((char *)m_buffer + m_bound, t_available, m_stream);
	if (t_stat != IO_NORMAL)
		return t_stat;

	m_bound += t_available;
	m_remaining -= t_available;
	m_limit += t_available;

	return t_stat;
}

///////////////////////////////////////////////////////////////////////////////

MCObjectOutputStream::MCObjectOutputStream(IO_handle p_stream)
{
	m_stream = p_stream;

	m_buffer = new (nothrow) char[16384];
	m_frontier = 0;
	m_mark = 0;
}

MCObjectOutputStream::~MCObjectOutputStream(void)
{
	delete[] (char *)m_buffer; /* Allocated with new[] */
}

IO_stat MCObjectOutputStream::WriteTag(uint32_t p_flags, uint32_t p_length)
{
	if (p_flags <= 255 && p_length < 1 << 23)
		return WriteU32(p_flags | (p_length << 8));

	IO_stat t_stat;
	t_stat = WriteU32((p_flags & 0xFF) | ((p_length & 0x7FFFFF) << 8) | (1U << 31));
	if (t_stat == IO_NORMAL)
		t_stat = WriteU32((p_flags >> 8) | ((p_length >> 23) << 24));

	return t_stat;
}

IO_stat MCObjectOutputStream::WriteFloat32(float p_value)
{
	uint32_t t_value;
	memcpy(&t_value, &p_value, sizeof(float));
	return WriteU32(t_value);
}

IO_stat MCObjectOutputStream::WriteFloat64(double p_value)
{
	uint64_t t_value;
	memcpy(&t_value, &p_value, sizeof(double));
	return WriteU64(t_value);
}

IO_stat MCObjectOutputStream::WriteU8(uint8_t p_value)
{
	return Write(&p_value, 1);
}

IO_stat MCObjectOutputStream::WriteU16(uint16_t p_value)
{
	p_value = MCSwapInt16HostToNetwork(p_value);
	return Write(&p_value, 2);
}

IO_stat MCObjectOutputStream::WriteU32(uint32_t p_value)
{
	p_value = MCSwapInt32HostToNetwork(p_value);
	return Write(&p_value, 4);
}

IO_stat MCObjectOutputStream::WriteU64(uint64_t p_value)
{
#ifndef __LITTLE_ENDIAN__
	p_value = p_value;
#else
	p_value = ((p_value >> 56) | ((p_value >> 40) & 0xFF00) | ((p_value >> 24) & 0xFF0000) | ((p_value >> 8) & 0xFF000000) |
			  ((p_value & 0xFF000000) << 8) | ((p_value & 0xFF0000) << 24) | ((p_value & 0xFF00) << 40) | (p_value << 56));
#endif
	return Write(&p_value, 8);
}

IO_stat MCObjectOutputStream::WriteStringRefNew(MCStringRef p_value, bool p_supports_unicode)
{
	if (!p_supports_unicode)
	{
		if (p_value == NULL || MCStringIsEmpty(p_value))
			return WriteU8(0);

		// StringRefs may contain '\0' internally but we can't write internal
		// nulls without creating a corrupt file.
		IO_stat t_stat;
		uint32_t t_length;
		t_length = MCStringGetLength(p_value);
		MCAutoPointer<char> t_value;
		/* UNCHECKED */ MCStringConvertToCString(p_value, &t_value);
		t_stat = Write(*t_value, t_length + 1);
		return t_stat;
	}
	
	MCAutoPointer<char> t_utf8_string;
	uindex_t t_utf8_string_length;
	if (!MCStringConvertToUTF8(p_value, &t_utf8_string, t_utf8_string_length))
		return IO_ERROR;
	
	if (WriteU32(t_utf8_string_length) != IO_NORMAL)
		return IO_ERROR;
	
	if (Write(*t_utf8_string, t_utf8_string_length) != IO_NORMAL)
		return IO_ERROR;
	
	return IO_NORMAL;
}

IO_stat MCObjectOutputStream::WriteNameRefNew(MCNameRef p_value, bool p_supports_unicode)
{
	return WriteStringRefNew(MCNameGetString(p_value), p_supports_unicode);
}

IO_stat MCObjectOutputStream::WriteColor(const MCColor &p_value)
{
	IO_stat t_stat = IO_NORMAL;
	if (t_stat == IO_NORMAL)
		t_stat = WriteU16(p_value.red);
	if (t_stat == IO_NORMAL)
		t_stat = WriteU16(p_value.green);
	if (t_stat == IO_NORMAL)
		t_stat = WriteU16(p_value.blue);

	return t_stat;
}

IO_stat MCObjectOutputStream::Write(const void *p_buffer, uint32_t p_amount)
{
	while(p_amount > 0)
	{
		if (m_frontier == 16384)
		{
			IO_stat t_stat;
			t_stat = Flush(false);
			if (t_stat != IO_NORMAL)
				return t_stat;
		}

		uint32_t t_available;
		t_available = MCU_min(16384 - m_frontier, p_amount);

		memcpy((char *)m_buffer + m_frontier, p_buffer, t_available);

		p_buffer = (char *)p_buffer + t_available;
		p_amount -= t_available;
		m_frontier += t_available;
	}

	return IO_NORMAL;
}

IO_stat MCObjectOutputStream::Flush(bool p_end)
{
	m_mark = m_frontier;

	uint32_t t_count;
	t_count = 1;

	IO_stat t_stat;
	t_stat = MCS_write(m_buffer, m_mark, t_count, m_stream);
	if (t_stat != IO_NORMAL)
		return t_stat;

	memmove(m_buffer, (char *)m_buffer + m_mark, m_frontier - m_mark);
	m_frontier -= m_mark;
	m_mark = 0;

	return IO_NORMAL;
}

// SN-2014-10-27: [[ Bug 13554 ]] The string length is different according to the support of Unicode
uint32_t MCObjectOutputStream::MeasureStringRefNew(MCStringRef p_string, bool p_supports_unicode)
{
    if (p_supports_unicode)
    {
        MCAutoStringRefAsUTF8String t_utf8_string;
        /* UNCHECKED */ t_utf8_string . Lock(p_string);
        
        // We write first the size (uint32_t) and then the UTF-8 string.
        return 4 + t_utf8_string . Size();
    }
    else
    {
        // C-strings are written as null-terminated strings
        return 1 + MCStringGetLength(p_string);
    }
}
