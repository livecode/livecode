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
	MCObjectInputStream(IO_handle p_stream, uint32_t p_remaining, bool p_new_format);
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
	IO_stat ReadNameRefNew(MCNameRef& r_value, bool p_supports_unicode);
	IO_stat ReadStringRefNew(MCStringRef& r_value, bool p_supports_unicode);
    IO_stat ReadTranslatedStringRef(MCStringRef& r_value);
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
    
    // SN-2014-03-27 [[ Bug 11993 ]] We need to know whether we are reading from a 7.0 file
    // in order to add the missing nil byte in the end before decrypting.  
    bool m_new_format;
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
	IO_stat WriteStringRefNew(MCStringRef p_value, bool p_supports_unicode);
	IO_stat WriteNameRefNew(MCNameRef p_value, bool p_supports_unicode);
	IO_stat WriteColor(const MCColor &p_value);

	IO_stat WriteS8(int8_t p_value)
	{
		return WriteU8(p_value);
	}

	IO_stat WriteS16(int16_t p_value)
	{
		return WriteU16(p_value);
	}

    IO_stat WritePoint(MCPoint p_point)
    {
        IO_stat t_stat = IO_NORMAL;
        if (t_stat == IO_NORMAL)
            t_stat = WriteS16(p_point.x);
        if (t_stat == IO_NORMAL)
            t_stat = WriteS16(p_point.y);
        return t_stat;
    }

	IO_stat WriteS32(int32_t p_value)
	{
		return WriteU32(p_value);
	}

	IO_stat Write(const void *p_buffer, uint32_t p_amount);

	virtual IO_stat Flush(bool p_end);
    
    // SN-2014-10-27: [[ Bug 13554 ]] The string length is different according to the support of Unicode
    uint32_t MeasureStringRefNew(MCStringRef p_string, bool p_supports_unicode);

protected:
	IO_handle m_stream;

	void *m_buffer;
	uint32_t m_mark;
	uint32_t m_frontier;
};

///////////////////////////////////////////////////////////////////////////////

#endif
