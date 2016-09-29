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

#include "system.h"
#include "mblandroid.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

////////////////////////////////////////////////////////////////////////////////

class MCStdioFileHandle: public MCSystemFileHandle
{
public:
    
    MCStdioFileHandle(FILE* p_fptr)
    {
        m_stream = p_fptr;
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
	
	virtual bool Write(const void *p_buffer, uint32_t p_length)
	{
		size_t t_amount;
		t_amount = fwrite(p_buffer, 1, p_length, m_stream);
		
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
	
	virtual uint64_t GetFileSize(void)
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
    
    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        return false;
    }
    
    virtual bool IsExhausted(void)
    {
        return feof(m_stream);
    }
	
private:
	FILE *m_stream;
};

////////////////////////////////////////////////////////////////////////////////

extern MCStringRef MCcmd;
bool path_to_apk_path(MCStringRef p_path, MCStringRef &r_apk_path);
bool apk_file_exists(MCStringRef p_apk_path);
bool apk_get_file_length(MCStringRef p_apk_path, int32_t &r_length);
bool apk_get_file_offset(MCStringRef p_apk_path, int32_t &r_offset);

class MCAssetFileHandle: public MCSystemFileHandle
{
public:
    MCAssetFileHandle(FILE* p_fptr, uint32_t p_size, uint32_t p_offset)
    {
        m_stream = p_fptr;
        m_size = p_size;
        m_offset = p_offset;
        m_position = 0;
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
	
	virtual bool Write(const void *p_buffer, uint32_t p_length)
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
	
	virtual uint64_t GetFileSize(void)
	{
		return m_size;
	}
	
	virtual void *GetFilePointer(void)
	{
		return NULL;
	}
    
    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        return false;
    }
    
    virtual bool IsExhausted(void)
    {
        return m_position >= m_size;
    }
	
private:
	FILE *m_stream;
	int32_t m_offset;
	int32_t m_size;
	int32_t m_position;
};

////////////////////////////////////////////////////////////////////////////////

IO_handle MCAndroidSystem::OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map)
{
	static const char *s_modes[] = { "r", "w", "r+", "a" };
    
    uint32_t t_mode;
    switch(p_mode)
    {
    case kMCOpenFileModeRead:
        t_mode = 0;
        break;
    case kMCOpenFileModeWrite:
        t_mode = 1;
        break;
    case kMCOpenFileModeUpdate:
        t_mode = 2;
        break;
    case kMCOpenFileModeAppend:
        t_mode = 3;
        break;
    }

	MCAutoStringRef t_apk_path;
    IO_handle t_handle;
    t_handle = nil;
    
	if (path_to_apk_path(p_path, &t_apk_path))
    {
        FILE *t_stream;
		int32_t t_size = 0;
		int32_t t_offset = 0;
        
        if (p_mode != kMCOpenFileModeRead)
			return NULL;
        
		if (!apk_get_file_length(*t_apk_path, t_size) || !apk_get_file_offset(*t_apk_path, t_offset))
			return NULL;
        
        MCAutoStringRefAsUTF8String t_utf8_mccmd;
        /* UNCHECKED */ t_utf8_mccmd . Lock(MCcmd);
		t_stream = fopen(*t_utf8_mccmd, s_modes[t_mode]);
		if (t_stream == NULL)
			return NULL;
		
		if (fseeko(t_stream, t_offset, SEEK_SET) != 0)
		{
			fclose(t_stream);
			return NULL;
		}
        
		t_handle = new (nothrow) MCAssetFileHandle(t_stream, t_size, t_offset);
    }
	else
	{
        FILE *t_stream;
        MCAutoStringRefAsUTF8String t_utf8_path;
        /* UNCHECKED */ t_utf8_path . Lock(p_path);
        t_stream = fopen(*t_utf8_path, s_modes[t_mode]);
        
        // SN-2015-01-28: [[ Bug 14383 ]] Removed code: we want to check that we
        //  are not updating before returning because of a NULL FILE*
        
		if (t_stream == NULL && p_mode == kMCOpenFileModeUpdate)
			t_stream = fopen(*t_utf8_path, "w+");
        
        if (t_stream == NULL)
            return NULL;
        
        t_handle = new (nothrow) MCStdioFileHandle(t_stream);
	}
    
    return t_handle;
}

IO_handle MCAndroidSystem::OpenFd(uint32_t fd, intenum_t p_mode)
{
	static const char *s_modes[] = { "r", "w", "w" };
    
    FILE *t_stream;
    t_stream = fdopen(fd, s_modes[fd]);
    if (t_stream == NULL)
        return NULL;
    
	return new MCStdioFileHandle(t_stream);
}

IO_handle MCAndroidSystem::OpenDevice(MCStringRef p_path, intenum_t p_mode)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
