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

#ifndef __MC_OBJECT_STREAM__
#define __MC_OBJECT_STREAM__

///////////////////////////////////////////////////////////////////////////////

class MCObjectInputStream
{
public:
	MCObjectInputStream(IO_handle p_stream, uint32_t p_remaining);
	virtual ~MCObjectInputStream(void);

	IO_stat ReadTag(uint32_t& r_flags, uint32_t& r_length, uint32_t& r_header_length);

	IO_stat ReadU8(uint8_t& r_value);
	IO_stat ReadU16(uint16_t& r_value);
	IO_stat ReadU32(uint32_t& r_value);
	IO_stat ReadU64(uint64_t& r_value);
	IO_stat ReadS8(int8_t& r_value);
	IO_stat ReadS16(int16_t& r_value);
	IO_stat ReadS32(int32_t& r_value);
	IO_stat ReadFloat32(float& r_value);
	IO_stat ReadFloat64(double& r_value);
	IO_stat ReadCString(char*& r_value);
	IO_stat ReadNameRef(MCNameRef& r_value);
	IO_stat ReadColor(MCColor &r_color);

	IO_stat Read(void *p_buffer, uint32_t p_amount);

	IO_stat Mark(void);
	IO_stat Skip(uint32_t p_amount);
	IO_stat Flush(void);

protected:
	virtual IO_stat Fill(void);
	
	IO_handle m_stream;

	int32_t m_mark;

	// Pointer to buffer holding input data
	void *m_buffer;

	// The current read head
	uint32_t m_frontier;

	// The limit of the decrypted data
	uint32_t m_limit;

	// The limit of the data
	uint32_t m_bound;

	// The amount of data remaining in the input file for this stream
	uint32_t m_remaining;
};

///////////////////////////////////////////////////////////////////////////////

class MCObjectOutputStream
{
public:
	MCObjectOutputStream(IO_handle p_stream);
	virtual ~MCObjectOutputStream(void);

	IO_stat WriteTag(uint32_t p_flags, uint32_t p_length);

	IO_stat WriteFloat32(float p_value);
	IO_stat WriteFloat64(double p_value);
	IO_stat WriteU8(uint8_t p_value);
	IO_stat WriteU16(uint16_t p_value);
	IO_stat WriteU32(uint32_t p_value);
	IO_stat WriteU64(uint64_t p_value);
	IO_stat WriteCString(const char *p_value);
	IO_stat WriteNameRef(MCNameRef p_value);
	IO_stat WriteColor(const MCColor &p_value);

	IO_stat WriteS8(int8_t p_value)
	{
		return WriteU8((unsigned)p_value);
	}

	IO_stat WriteS16(int16_t p_value)
	{
		return WriteU16((unsigned)p_value);
	}

	IO_stat WriteS32(int32_t p_value)
	{
		return WriteU32((unsigned)p_value);
	}

	IO_stat Write(const void *p_buffer, uint32_t p_amount);

	virtual IO_stat Flush(bool p_end);

protected:
	IO_handle m_stream;

	void *m_buffer;
	uint32_t m_mark;
	uint32_t m_frontier;
};

///////////////////////////////////////////////////////////////////////////////

#endif
