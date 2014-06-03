/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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


#ifndef SYSPOSIX_H
#define SYSPOSIX_H

#include "globdefs.h"
#include "system.h"

#include "globals.h"

#include <sys/ioctl.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

class MCStdioFileHandle: public MCSystemFileHandle
{
public:
    MCStdioFileHandle(FILE* p_fptr)
    {
        m_fptr = p_fptr;
        m_is_eof = false;
    }

    virtual void Close(void)
    {
#ifdef /* MCS_close_dsk_lnx */ LEGACY_SYSTEM
        if (stream->fptr == NULL)
        {
            if (stream->fd == 0)
            {
                if (!(stream->flags & IO_FAKE))
                    delete stream->buffer;
            }
#ifndef NOMMAP
            else
            {
                munmap((char *)stream->buffer, stream->len);
                close(stream->fd);
            }
#endif

        }
        else
            fclose(stream->fptr);
        delete stream;
        stream = NULL;
        return IO_NORMAL;
#endif /* MCS_close_dsk_lnx */
        if (m_fptr != NULL)
            fclose(m_fptr);

        delete this;
    }

    // Returns true if an attempt has been made to read past the end of the
    // stream.
    virtual bool IsExhausted(void)
    {
        return feof(m_fptr);
    }

    virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
    {
#ifdef /* MCS_read_dsk_lnx */ LEGACY_SYSTEM
        if (MCabortscript || ptr == NULL)
            return IO_ERROR;

        if ((stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
            return IO_ERROR;

        // MW-2009-06-25: If this is a custom stream, call the appropriate callback.
        // MW-2009-06-30: Refactored to common (platform-independent) implementation
        //   in mcio.cpp
        if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
            return MCS_fake_read(ptr, size, n, stream);

        IO_stat stat = IO_NORMAL;
        if (stream->fptr == NULL)
        {
            uint4 nread = size * n;
            if (nread > stream->len - (stream->ioptr - stream->buffer))
            {
                n = (stream->len - (stream->ioptr - stream->buffer)) / size;
                nread = (stream->len - (stream->ioptr - stream->buffer)) / size;
                stat = IO_EOF;
            }
            if (nread == 1)
            {
                char *tptr = (char *)ptr;
                *tptr = *stream->ioptr++;
            }
            else
            {
                memcpy(ptr, stream->ioptr, nread);
                stream->ioptr += nread;
            }
        }
        else
        {
            char *sptr = (char *)ptr;
            uint4 nread;
            uint4 toread = n * size;
            uint4 offset = 0;
            errno = 0;
            while ((nread = fread(&sptr[offset], 1, toread, stream->fptr)) != toread)
            {
                offset += nread;
                n = offset / size;
                if (ferror(stream->fptr))
                {
                    clearerr(stream->fptr);

                    if (errno == EAGAIN)
                        return IO_NORMAL;

                    if (errno == EINTR)
                    {
                        toread -= nread;
                        continue;
                    }
                    else
                        return IO_ERROR;
                }
                if (MCS_eof(stream))
                {
                    return IO_EOF;
                }
                return IO_NONE;
            }
        }
        return stat;
#endif /* MCS_read_dsk_lnx */
        if (MCabortscript || p_buffer == NULL)
            return false;

        char *sptr = (char *)p_buffer;
        uint4 nread;
        uint4 toread = p_length;
        uint4 offset = 0;
        errno = 0;
        while ((nread = fread(&sptr[offset], 1, toread, m_fptr)) != toread)
        {
            offset += nread;
            r_read = offset;
            if (ferror(m_fptr))
            {
                clearerr(m_fptr);

                if (errno == EAGAIN)
                    return true;

                if (errno == EINTR)
                {
                    toread -= nread;
                    continue;
                }
            }

            return false;
        }

        r_read = offset + nread;
        return true;
    }

    virtual bool Write(const void *p_buffer, uint32_t p_length)
    {
#ifdef /* MCS_write_dsk_lnx */ LEGACY_SYSTEM
        if ((stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
            return MCU_dofakewrite(stream -> buffer, stream -> len, ptr, size, n);

        if (fwrite(ptr, size, n, stream->fptr) != n)
            return IO_ERROR;
        return IO_NORMAL;
#endif /* MCS_write_dsk_lnx */
        if (fwrite(p_buffer, 1, p_length, m_fptr) != p_length)
            return false;

        return true;
    }

    virtual bool Seek(int64_t p_offset, int p_dir)
    {
#ifdef /* MCS_seek_cur_dsk_lnx */ LEGACY_SYSTEM
        // MW-2009-06-25: If this is a custom stream, call the appropriate callback.
        // MW-2009-06-30: Refactored to common implementation in mcio.cpp.
        if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
            return MCS_fake_seek_cur(stream, offset);

        if (stream->fptr == NULL)
            IO_set_stream(stream, stream->ioptr + offset);
        else
            if (fseeko64(stream->fptr, offset, SEEK_CUR) != 0)
                return IO_ERROR;
        return IO_NORMAL;
#endif /* MCS_seek_cur_dsk_lnx */
#ifdef /* MCS_seek_set_dsk_lnx */ LEGACY_SYSTEM
        // MW-2009-06-30: If this is a custom stream, call the appropriate callback.
        if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
            return MCS_fake_seek_set(stream, offset);

        if (stream->fptr == NULL)
            IO_set_stream(stream, stream->buffer + offset);
        else
            if (fseeko64(stream->fptr, offset, SEEK_SET) != 0)
                return IO_ERROR;
        return IO_NORMAL;
#endif /* MCS_seek_set_dsk_lnx */
#ifdef /* MCS_seek_end_dsk_lnx */ LEGACY_SYSTEM
        if (stream->fptr == NULL)
            IO_set_stream(stream, stream->buffer + stream->len + offset);
        else
            if (fseeko64(stream->fptr, offset, SEEK_END) != 0)
                return IO_ERROR;
        return IO_NORMAL;
#endif /* MCS_seek_end_dsk_lnx */
        return fseeko64(m_fptr, p_offset, p_dir > 0 ? SEEK_SET : (p_dir < 0 ? SEEK_END : SEEK_CUR)) == 0;
    }

    virtual bool Truncate(void)
    {
#ifdef /* MCS_trunc_dsk_lnx */ LEGACY_SYSTEM
        if (ftruncate(fileno(stream->fptr), ftell(stream->fptr)))
            return IO_ERROR;
        return IO_NORMAL;
#endif /* MCS_trunc_dsk_lnx */
        return ftruncate(fileno(m_fptr), ftell(m_fptr)) == 0;
    }

    virtual bool Sync(void)
    {
#ifdef /* MCS_sync_dsk_lnx */ LEGACY_SYSTEM
        if (stream->fptr != NULL)
        {
            int64_t pos = ftello64(stream->fptr);
            if (fseeko64(stream->fptr, pos, SEEK_SET) != 0)
                return IO_ERROR;
        }
        return IO_NORMAL;
#endif /* MCS_sync_dsk_lnx */
        if (m_fptr != NULL)
        {
            int64_t pos = ftello64(m_fptr);
            if (fseeko64(m_fptr, pos, SEEK_SET) != 0)
                return false;
        }
        return true;
    }

    virtual bool Flush(void)
    {
#ifdef /* MCS_flush_dsk_lnx */ LEGACY_SYSTEM
        if (stream->fptr != NULL)
            if (fflush(stream->fptr))
                return IO_ERROR;

        return IO_NORMAL;
#endif /* MCS_flush_dsk_lnx */
        if (fflush(m_fptr))
            return false;

        return true;
    }

    virtual bool PutBack(char p_char)
    {
#ifdef /* MCS_putback_dsk_lnx */ LEGACY_SYSTEM
        if (stream -> fptr == NULL)
            return MCS_seek_cur(stream, -1);

        if (ungetc(c, stream -> fptr) != c)
            return IO_ERROR;

        return IO_NORMAL;
#endif /* MCS_putback_dsk_lnx */
        if (ungetc(p_char, m_fptr) != p_char)
            return false;

        return true;
    }

    virtual int64_t Tell(void)
    {
#ifdef /* MCS_tell_dsk_lnx */ LEGACY_SYSTEM
        // MW-2009-06-30: If this is a custom stream, call the appropriate callback.
        if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
            return MCS_fake_tell(stream);

        if (stream->fptr != NULL)
            return ftello64(stream->fptr);
        else
            return stream->ioptr - stream->buffer;
#endif /* MCS_tell_dsk_lnx */
        return ftello64(m_fptr);
    }

    virtual void *GetFilePointer(void)
    {
        return (void*)m_fptr;
    }

    virtual int64_t GetFileSize(void)
    {
#ifdef /* MCS_fsize_dsk_lnx */ LEGACY_SYSTEM
        if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
            return MCS_fake_fsize(stream);

        struct stat64 buf;
        if (stream->fptr == NULL)
            return stream->len;

        int fd = fileno(stream->fptr);

        if (fstat64(fd, &buf))
            return 0;
        return buf.st_size;
#endif /* MCS_fsize_dsk_lnx */
        struct stat64 buf;

        int fd = fileno(m_fptr);

        if (fstat64(fd, &buf))
            return 0;

        return buf.st_size;
    }

    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        return false;
    }

    virtual FILE* GetStream(void)
    {
        return m_fptr;
    }

private:
    FILE *m_fptr;
    bool m_is_eof;
};

#endif // SYSPOSIX_H
