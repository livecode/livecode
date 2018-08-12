/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "em-filehandle.h"
#include "em-util.h"

#include "parsedef.h"
#include "globals.h"

#include <sys/stat.h>
#include <unistd.h>

MCEmscriptenFileHandle::MCEmscriptenFileHandle(int t_fd)
	: m_fd(t_fd), m_eof(false)
{
}

MCEmscriptenFileHandle::~MCEmscriptenFileHandle()
{
	if (m_fd != -1)
	{
		errno = 0;
		if (0 != close(m_fd))
		{
			/* There's nothing that can be done if close(2) fails,
			 * because the file descriptor is no longer valid. */
			MCLog("Failed to close #%i", m_fd);
		}

		m_fd = -1;
	}
}

void *
MCEmscriptenFileHandle::GetFilePointer()
{
	/* MCEmscriptenFileHandle doesn't actually encapsulate a file
	 * pointer, only a file handle. */
	return NULL;
}

/* ----------------------------------------------------------------
 * Basic operations
 * ---------------------------------------------------------------- */

void
MCEmscriptenFileHandle::Close()
{
	/* Self-delete!  This matches the behaviour of the Linux
	 * MCStdioFileHandle class, with the advantage that the stream
	 * will be closed properly even if the class is destroyed without
	 * ever calling Close(). */
	delete this;
}

bool
MCEmscriptenFileHandle::IsExhausted()
{
	return m_eof;
}

bool
MCEmscriptenFileHandle::Read(void *x_buffer,
                             uint32_t p_length,
                             uint32_t & r_read)
{
	MCAssert(NULL != x_buffer);

	/* FIXME Figure out why this is needed */
	if (MCabortscript)
	{
		return false;
	}

	/* Read in a loop until the whole requested length has been read,
	 * or EOF is reached, or an unrecoverable error occurs. */
	char *t_read_ptr = reinterpret_cast<char *>(x_buffer);
	size_t t_total = 0;
	ssize_t t_read = 0;

	while (t_total < p_length)
	{
		errno = 0;
		t_read = read(m_fd,
		              t_read_ptr + t_total,
		              p_length - t_total);

		if (-1 == t_read)
		{
			/* An error occurred before any data was read */

			if (errno == EAGAIN)
			{
				/* FIXME Figure out why this is a success */
				return true;
			}

			if (errno == EINTR)
			{
				/* On a signal interruption, just try again. */
				continue;
			}

			r_read = t_total;
			return false;
		}

		if (0 == t_read)
		{
			/* successfully reading nothing suggests we're at
			 * end-of-file */
			m_eof = true;

			r_read = t_total;
			return false;
		}

		t_total += t_read;
	}

	r_read = t_total;
	return true;
}

bool
MCEmscriptenFileHandle::Write(const void *p_buffer,
                              uint32_t p_length)
{
	MCAssert(NULL != p_buffer);

	/* Write in a loop until the whole requested length has been
	 * written, or EOF is reached, or an unrecoverable error
	 * occurs. */
	const char *t_write_ptr = reinterpret_cast<const char *>(p_buffer);
	size_t t_total = 0;
	ssize_t t_written = 0;

	while (t_total < p_length)
	{
		errno = 0;
		t_written = write(m_fd,
		                  t_write_ptr + t_total,
		                  p_length - t_total);

		if (-1 == t_written)
		{
			/* An error occurred before any data was written */

			if (errno == EAGAIN)
			{
				/* FIXME figure out why this is a success */
				return true;
			}

			if (errno == EINTR)
			{
				/* On a signal interruption, just try again */
				continue;
			}

			return false;
		}

		if (0 == t_written)
		{
			/* successfully writing but making no progress is weird --
			 * treat it as an error */
			return false;
		}

		t_total += t_written;
	}

	return true;
}

/* ----------------------------------------------------------------
 * File position
 * ---------------------------------------------------------------- */

int64_t
MCEmscriptenFileHandle::Tell()
{
	errno = 0;
	return lseek(m_fd, 0, SEEK_CUR);
}

bool
MCEmscriptenFileHandle::Seek(int64_t p_offset,
                             int p_whence)
{
	int t_sys_whence;
	if (p_whence > 0)
	{
		t_sys_whence = SEEK_SET;
	}
	else if (p_whence < 0)
	{
		t_sys_whence = SEEK_END;
	}
	else
	{
		t_sys_whence = SEEK_CUR;
	}

	off_t t_result;

	errno = 0;
	t_result = lseek(m_fd, p_offset, t_sys_whence);

	if (reinterpret_cast<off_t>(-1) == t_result)
	{
		return false;
	}

	return true;
}

/* ----------------------------------------------------------------
 * File size
 * ---------------------------------------------------------------- */

uint64_t
MCEmscriptenFileHandle::GetFileSize()
{
	struct stat t_stat;

	errno = 0;
	if (0 != fstat(m_fd, &t_stat))
	{
		return 0;
	}

	return t_stat.st_size;
}

/* ----------------------------------------------------------------
 * Filesystem-related operations
 * ---------------------------------------------------------------- */

bool
MCEmscriptenFileHandle::Truncate()
{
	errno = 0;

	/* Check the current file position */
	int64_t t_length = Tell();
	if (0 != errno)
	{
		return false;
	}

	/* Truncate to the current file position */
	return (0 == ftruncate(m_fd, t_length));
}

bool
MCEmscriptenFileHandle::Sync()
{
	/* Assume that the filesystem is fully in-memory and therefore fsync()
	 * is meaningless! */
	return true;
}

bool
MCEmscriptenFileHandle::Flush()
{
	/* All MCEmscriptenFileHandle instances are unbuffered. */
	return true;
}

/* ----------------------------------------------------------------
 * Buffer-based operations
 * ---------------------------------------------------------------- */

bool
MCEmscriptenFileHandle::PutBack(char p_char)
{
	return false;
}

bool
MCEmscriptenFileHandle::TakeBuffer(void *& r_buffer,
                                   size_t & r_length)
{
	return false;
}
