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

#include "core.h"
#include "system.h"
#include "mblandroid.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

////////////////////////////////////////////////////////////////////////////////

class MCStdioFileHandle: public MCSystemFileHandle
{
public:
	static MCStdioFileHandle *Open(const char *p_path, const char *p_mode)
	{
		FILE *t_stream;
		t_stream = fopen(p_path, p_mode);
		if (t_stream == NULL)
			return NULL;
		
		MCStdioFileHandle *t_handle;
		t_handle = new MCStdioFileHandle;
		t_handle -> m_stream = t_stream;
		
		return t_handle;
	}
	
	static MCStdioFileHandle *OpenFd(int fd, const char *p_mode)
	{
		FILE *t_stream;
		t_stream = fdopen(fd, p_mode);
		if (t_stream == NULL)
			return NULL;
		
		MCStdioFileHandle *t_handle;
		t_handle = new MCStdioFileHandle;
		t_handle -> m_stream = t_stream;
		
		return t_handle;
		
	}
	
	virtual void Close(void)
	{
		fclose(m_stream);
		delete this;
	}
	
	virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
	{
		size_t t_amount;
		t_amount = fread(p_buffer, 1, p_length, m_stream);
		r_read = t_amount;
		
		if (t_amount < p_length)
			return ferror(m_stream) == 0;
		
		return true;
	}
	
	virtual bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written)
	{
		size_t t_amount;
		t_amount = fwrite(p_buffer, 1, p_length, m_stream);
		r_written = t_amount;
		
		if (t_amount < p_length)
			return false;
		
		return true;
	}
	
	virtual bool Seek(int64_t offset, int p_dir)
	{
		return fseeko(m_stream, offset, p_dir < 0 ? SEEK_END : (p_dir > 0 ? SEEK_SET : SEEK_CUR)) == 0;
	}
	
	virtual bool Truncate(void)
	{
		return ftruncate(fileno(m_stream), ftell(m_stream));
	}
	
	virtual bool Sync(void) 
	{
		int64_t t_pos;
		t_pos = ftell(m_stream);
		return fseek(m_stream, t_pos, SEEK_SET) == 0;
	}
	
	virtual bool Flush(void)
	{
		return fflush(m_stream) == 0;
	}
	
	virtual bool PutBack(char p_char)
	{
		return ungetc(p_char, m_stream) != EOF;
	}
	
	virtual int64_t Tell(void)
	{
		return ftell(m_stream);
	}
	
	virtual int64_t GetFileSize(void)
	{
		struct stat t_info;
		if (fstat(fileno(m_stream), &t_info) != 0)
			return 0;
		return t_info . st_size;
	}
	
	virtual void *GetFilePointer(void)
	{
		return NULL;
	}
	
private:
	FILE *m_stream;
};

////////////////////////////////////////////////////////////////////////////////

extern char *MCcmd;
bool path_to_apk_path(const char * p_path, const char *&r_apk_path);
bool apk_file_exists(const char *p_apk_path);
bool apk_get_file_length(const char *p_apk_path, int32_t &r_length);
bool apk_get_file_offset(const char *p_apk_path, int32_t &r_offset);

class MCAssetFileHandle: public MCSystemFileHandle
{
public:
	static MCAssetFileHandle *Open(const char *p_path, const char *p_mode)
	{
		int32_t t_size = 0;
		int32_t t_offset = 0;

		if (!MCCStringEqual(p_mode, "r"))
			return NULL;

		if (!apk_get_file_length(p_path, t_size) || !apk_get_file_offset(p_path, t_offset))
			return NULL;

		FILE *t_stream;
		t_stream = fopen(MCcmd, p_mode);
		if (t_stream == NULL)
			return NULL;
		
		if (fseeko(t_stream, t_offset, SEEK_SET) != 0)
		{
			fclose(t_stream);
			return NULL;
		}

		MCAssetFileHandle *t_handle;
		t_handle = new MCAssetFileHandle;
		t_handle -> m_stream = t_stream;
		t_handle -> m_size = t_size;
		t_handle -> m_offset = t_offset;
		t_handle -> m_position = 0;
		
		return t_handle;
	}
		
	virtual void Close(void)
	{
		fclose(m_stream);
		delete this;
	}
	
	virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
	{
		size_t t_amount;
		size_t t_toread;
		t_toread = MCU_min((signed)p_length, m_size - m_position);
		t_amount = fread(p_buffer, 1, t_toread, m_stream);
		r_read = t_amount;
		
		m_position += t_amount;

		if (t_amount < t_toread)
			return ferror(m_stream) == 0;
		
		return true;
	}
	
	virtual bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written)
	{
		return false;
	}
	
	virtual bool Seek(int64_t offset, int p_dir)
	{
		int64_t t_position;
		if (p_dir < 0)
			t_position = m_offset + m_size - offset;
		else if (p_dir > 0)
			t_position = m_offset + offset;
		else
			t_position = m_offset + m_position + offset;

		if (t_position < m_offset || t_position > m_offset + m_size)
			return false;

		if (fseeko(m_stream, t_position, SEEK_SET) != 0)
			return false;

		m_position = t_position - m_offset;
		return true;
	}
	
	virtual bool Truncate(void)
	{
		return false;
	}
	
	virtual bool Sync(void) 
	{
		int64_t t_pos;
		t_pos = ftell(m_stream);
		return fseek(m_stream, t_pos, SEEK_SET) == 0;
	}
	
	virtual bool Flush(void)
	{
		return fflush(m_stream) == 0;
	}
	
	virtual bool PutBack(char p_char)
	{
		if (ungetc(p_char, m_stream) == EOF)
			return false;
		m_position--;
		return true;
	}
	
	virtual int64_t Tell(void)
	{
		return m_position;
	}
	
	virtual int64_t GetFileSize(void)
	{
		return m_size;
	}
	
	virtual void *GetFilePointer(void)
	{
		return NULL;
	}
	
private:
	FILE *m_stream;
	int32_t m_offset;
	int32_t m_size;
	int32_t m_position;
};

////////////////////////////////////////////////////////////////////////////////

MCSystemFileHandle *MCAndroidSystem::OpenFile(const char *p_path, uint32_t p_mode, bool p_map)
{
	static const char *s_modes[] = { "r", "w", "r+", "a" };

	const char *t_apk_path = nil;
	if (path_to_apk_path(p_path, t_apk_path))
		return MCAssetFileHandle::Open(t_apk_path, s_modes[p_mode & 0xff]);
	else
	{
		MCSystemFileHandle *t_handle;
		t_handle = MCStdioFileHandle::Open(p_path, s_modes[p_mode & 0xff]);
		if (t_handle == NULL && p_mode == kMCSystemFileModeUpdate)
			t_handle = MCStdioFileHandle::Open(p_path, "w+");
		
		return t_handle;
	}
}

MCSystemFileHandle *MCAndroidSystem::OpenStdFile(uint32_t i)
{
	static const char *s_modes[] = { "r", "w", "w" };
	return MCStdioFileHandle::OpenFd(i, s_modes[i]);
}

MCSystemFileHandle *MCAndroidSystem::OpenDevice(const char *p_path, uint32_t p_mode, const char *p_control_string)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
