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

#ifndef __MC_TEXT_BUFFER__
#define __MC_TEXT_BUFFER__

////////////////////////////////////////////////////////////////////////////////

// This simple class implements an auto-extended buffer for text accumulation.
class text_buffer_t
{
public:
	text_buffer_t(void)
	{
		m_buffer = nil;
		m_frontier = 0;
		m_capacity = 0;
		m_as_unicode = false;
	}
	
	~text_buffer_t(void)
	{
		if (m_buffer != nil)
			free(m_buffer);
	}
	
	// Returns true if the buffer is accepting unicode.
	bool isunicode(void) const
	{
		return true;
	}

	// Change the output encoding type.
	void setasunicode(bool p_as_unicode)
	{
		m_as_unicode = p_as_unicode;
	}

	// Ensure there is room for at least 'byte_size' bytes in the buffer.
	bool ensure(uint32_t p_byte_size)
	{
		// If there's enough room, we are done.
		if (m_capacity - m_frontier >= p_byte_size)
			return true;
		
		// Compute the new capacity, rounded up to the nearest 64k boundary.
		uint32_t t_new_capacity;
		t_new_capacity = (m_capacity + p_byte_size + 65536) & ~65535;
		
		// Attempt to reallocate the existing buffer.
		void *t_new_buffer;
		t_new_buffer = realloc(m_buffer, t_new_capacity);
		if (t_new_buffer == nil)
			return false;
		
		// Update the ptr and capacity if successful.
		m_capacity = t_new_capacity;
		m_buffer = (char *)t_new_buffer;
		return true;
	}
	
	// Append the given text to the buffer. This text is converted to match
	// the encoding of the buffer.
	bool appendtext(const void *p_bytes, uint32_t p_byte_count, bool p_is_unicode)
	{
		if (p_is_unicode == m_as_unicode)
		{
			if (!ensure(p_byte_count))
				return false;
			memcpy(m_buffer + m_frontier, p_bytes, p_byte_count);
			m_frontier += p_byte_count;
		}
		else if (p_is_unicode)
		{
			if (!ensure(p_byte_count / 2))
				return false;
			while(p_byte_count > 0)
			{
				uint32_t t_used, t_made;
				MCTextRunnify((const uint2 *)p_bytes, p_byte_count / 2, (uint1 *)(m_buffer + m_frontier), t_used, t_made);
				if (t_made == 0)
				{
					for(uint32_t i = 0; i < t_used; i++)
						m_buffer[m_frontier + i] = '?';
					t_made = t_used;
				}
				m_frontier += t_made;
				p_byte_count -= t_used * 2;
				p_bytes = (char *)p_bytes + t_used * 2;
			}
		}
		else
		{
			if (!ensure(p_byte_count * 2))
				return false;
			for(uint32_t i = 0; i < p_byte_count; i++)
				*(uint16_t *)&m_buffer[m_frontier + i * 2] = MCUnicodeMapFromNative(((uint8_t *)p_bytes)[i]);
			m_frontier += p_byte_count * 2;
		}

		return true;
	}

	// Append the given text to the buffer, formatting in standard print-f style firs.
	bool appendtextf(const char *p_format, ...)
	{
		assert(!m_as_unicode);

		va_list t_args;
		int t_count;

#if defined(_HAS_VSCPRINTF)
		va_start(t_args, p_format);
		t_count = _vscprintf(p_format, t_args);
		va_end(t_args);
#elif defined(_HAS_VSNPRINTF)
		va_start(t_args, p_format);
		t_count = vsnprintf(nil, 0, p_format, t_args);
		va_end(t_args);
#else
#error text_buffer_t::appendtextf not implemented
#endif

		// MW-2013-09-30: [[ Bug 11214 ]] Make sure we take into account room for
		//   the NUL terminator.
		if (!ensure(t_count + 1))
			return false;
	
		va_start(t_args, p_format);
		vsprintf((char *)m_buffer + m_frontier, p_format, t_args);
		va_end(t_args);

		m_frontier += t_count;

		return true;
	}

	bool appendcstring(const char *p_string)
	{
		return appendtext(p_string, strlen(p_string), false);
	}
	
	// Transfer the buffer to the EP.
	/*void givetoep(MCExecPoint& ep)
	{
		ep . grabbuffer(m_buffer, m_frontier);
		m_buffer = nil;
		m_frontier = 0;
		m_capacity = 0;
	}*/

	bool takeasstringref(MCStringRef& r_string)
	{
		if (!MCStringCreateWithBytesAndRelease((char_t *)m_buffer, m_frontier, m_as_unicode ? kMCStringEncodingUTF16 : kMCStringEncodingMacRoman, false, r_string))
			return false;

		m_buffer = nil;
		m_frontier = 0;
		m_capacity = 0;

		return true;
	}
	
private:
	char *m_buffer;
	uint32_t m_frontier;
	uint32_t m_capacity;
	bool m_as_unicode;
};

////////////////////////////////////////////////////////////////////////////////

#endif
