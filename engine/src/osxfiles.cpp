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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "typedefs.h"
#include "mcio.h"

#include "mcerror.h"
#include "execpt.h"
#include "handler.h"
#include "util.h"

#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "control.h"
#include "param.h"
#include "securemode.h"
#include "osspec.h"

#include "license.h"

#include <sys/stat.h>
#include <sys/utsname.h>

#define ENTRIES_CHUNK 1024

#define SERIAL_PORT_BUFFER_SIZE  16384 //set new buffer size for serial input port
#include <termios.h>
#define B16600 16600

#include <pwd.h>

#define USE_FSCATALOGINFO

//for setting serial port use
typedef struct
{
	short baudrate;
	short parity;
	short stop;
	short data;
}
SerialControl;

//struct
SerialControl portconfig; //serial port configuration structure

extern "C"
{
	extern UInt32 SwapQDTextFlags(UInt32 newFlags);
	typedef UInt32 (*SwapQDTextFlagsPtr)(UInt32 newFlags);
}

static void configureSerialPort(int sRefNum);
static bool getResourceInfo(MCListRef p_list, ResType p_type);
static void parseSerialControlStr(char *set, struct termios *theTermios);

char *path2utf(char *path);

void MCS_setfiletype(const char *newpath);


/********************************************************************/
/*                        File Handling                             */
/********************************************************************/ 

// File opening and closing

IO_handle MCS_open(const char *path, const char *mode,
									 Boolean map, Boolean driver, uint4 offset)
{
	IO_handle handle = NULL;
		//opening regular files
		//set the file type and it's creator. These are 2 global variables
		char *oldpath = strclone(path);
		
		// OK-2008-01-10 : Bug 5764. Check here that MCS_resolvepath does not return NULL
		char *t_resolved_path;
		t_resolved_path = MCS_resolvepath(path);
		if (t_resolved_path == NULL)
			return NULL;
		
		char *newpath = path2utf(t_resolved_path);
		FILE *fptr;

		if (driver)
		{
			fptr = fopen(newpath,  mode );
			if (fptr != NULL)
			{
				int val;
				val = fcntl(fileno(fptr), F_GETFL, val);
				val |= O_NONBLOCK |  O_NOCTTY;
				fcntl(fileno(fptr), F_SETFL, val);
				configureSerialPort((short)fileno(fptr));
			}
		}
		else
		{
			fptr = fopen(newpath, IO_READ_MODE);
			if (fptr == NULL)
				fptr = fopen(oldpath, IO_READ_MODE);
			Boolean created = True;
			if (fptr != NULL)
			{
				created = False;
				if (mode != IO_READ_MODE)
				{
					fclose(fptr);
					fptr = NULL;
				}
			}
			if (fptr == NULL)
				fptr = fopen(newpath, mode);

			if (fptr == NULL && !strequal(mode, IO_READ_MODE))
				fptr = fopen(newpath, IO_CREATE_MODE);
			if (fptr != NULL && created)
				MCS_setfiletype(oldpath);
		}

		delete newpath;
		delete oldpath;
		if (fptr != NULL)
		{
			handle = new IO_header(fptr, 0, 0, 0, NULL, 0, 0);
			if (offset > 0)
				fseek(handle->fptr, offset, SEEK_SET);

			if (strequal(mode, IO_APPEND_MODE))
				handle->flags |= IO_SEEKED;
		}

	return handle;
}

IO_handle MCS_fakeopen(const MCString &data)
{
	return new IO_header(NULL, 0, 0, 0, (char *)data.getstring(),
	                     data.getlength(), IO_FAKE);
}

IO_handle MCS_fakeopenwrite(void)
{
	return new IO_header(NULL, 0, 0, 0, NULL, 0, IO_FAKEWRITE);
}

IO_handle MCS_fakeopencustom(MCFakeOpenCallbacks *p_callbacks, void *p_state)
{
	return new IO_header(NULL, 0, 0, 0, (char *)p_state, (uint32_t)p_callbacks, IO_FAKECUSTOM);
}

IO_stat MCS_fakeclosewrite(IO_handle& stream, char*& r_buffer, uint4& r_length)
{
	if ((stream -> flags & IO_FAKEWRITE) != IO_FAKEWRITE)
	{
		r_buffer = NULL;
		r_length = 0;
		MCS_close(stream);
		return IO_ERROR;
	}

	r_buffer = (char *)realloc(stream -> buffer, stream -> len);
	r_length = stream -> len;

	MCS_close(stream);

	return IO_NORMAL;
}

bool MCS_isfake(IO_handle stream)
{
	return (stream -> flags & IO_FAKEWRITE) != 0;
}

uint4 MCS_faketell(IO_handle stream)
{
	return stream -> len;
}

void MCS_fakewriteat(IO_handle stream, uint4 p_pos, const void *p_buffer, uint4 p_size)
{
	memcpy(stream -> buffer + p_pos, p_buffer, p_size);
}

void MCS_loadfile(MCExecPoint &ep, Boolean binary)
{
	if (!MCSecureModeCanAccessDisk())
	{
		ep.clear();
		MCresult->sets("can't open file");
		return;
	}
	char *tpath = ep.getsvalue().clone();
	
	// MW-2010-10-17: [[ Bug 5246 ]] MCS_resolvepath can return nil if an unknown ~ is used.
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(tpath);
	
	if (t_resolved_path == NULL)
	{
		MCresult -> sets("bad path");
		return;
	}
	
	char *newpath = path2utf(t_resolved_path);
	
	ep.clear();
	delete tpath;
	FILE *fptr = fopen(newpath, IO_READ_MODE);
	delete newpath;
	struct stat buf;
	if (fptr == NULL || fstat(fileno(fptr), (struct stat *)&buf))
		MCresult->sets("can't open file");
	else
	{
		char *buffer;
		/* UNCHECKED */ ep . reserve(buf . st_size, buffer); 
		if (buffer == NULL)
		{
			ep.clear();
			MCresult->sets("can't create data buffer");
		}
		else
		{
			uint4 tsize = fread(buffer, 1, buf.st_size, fptr);
			if (tsize != buf.st_size)
			{
				ep.clear();
				MCresult->sets("error reading file");
			}
			else
			{
				ep . commit(tsize);
				if (!binary)
					ep.texttobinary();
				MCresult->clear(False);
			}
			fclose(fptr);
		}
	}
}

void MCS_savefile(const MCString &fname, MCExecPoint &data, Boolean binary)
{
	if (!MCSecureModeCanAccessDisk())
	{
		MCresult->sets("can't open file");
		return;
	}
	char *tpath = fname.clone();
	
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(tpath);

	if (t_resolved_path == NULL)
	{
		MCresult -> sets("bad path");
		return;
	}
	char *newpath = path2utf(t_resolved_path);

	FILE *fptr = fopen(newpath, IO_WRITE_MODE);
	if (fptr == NULL)
		MCresult->sets("can't open file");
	else
	{
		if (!binary)
			data.binarytotext();
		uint4 toWrite = data.getsvalue().getlength();
		if (fwrite(data.getsvalue().getstring(), 1, toWrite, fptr) != toWrite)
			MCresult->sets("error writing file");
		else
		{
			SetEOF(fileno(fptr), toWrite);
			MCresult->clear(False);
		}
		fclose(fptr);


		tpath = fname.clone();
		MCS_setfiletype(tpath);
	}
	delete tpath;
	delete newpath;
}

IO_stat MCS_close(IO_handle &stream)
{
	IO_stat stat = IO_NORMAL;
	if (stream->serialIn != 0 || stream->serialOut != 0)
	{//close the serial port

	}
	else
		if (stream->fptr == NULL)
		{
			if (!(stream->flags & IO_FAKE))
				delete stream->buffer;
		}
		else
			fclose(stream->fptr);
	delete stream;
	stream = NULL;
	return stat;
}

// File reading and writing

IO_stat MCS_putback(char c, IO_handle stream)
{
	if (stream -> serialIn != 0 || stream -> fptr == NULL)
		return MCS_seek_cur(stream, -1);
	
	if (ungetc(c, stream -> fptr) != c)
		return IO_ERROR;
		
	return IO_NORMAL;
}

IO_stat MCS_read(void *ptr, uint4 size, uint4 &n, IO_handle stream)
{
	if (MCabortscript || stream == NULL)
		return IO_ERROR;

	if ((stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
		return IO_ERROR;

	// MW-2009-06-25: If this is a custom stream, call the appropriate callback.
	// MW-2009-06-30: Refactored to common (platform-independent) implementation
	//   in mcio.cpp
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_read(ptr, size, n, stream);

	IO_stat stat = IO_NORMAL;
	uint4 nread;
	if (stream->serialIn != 0)
	{//read from serial port
		long count = 0;  // n group of size data to be read

		count = MCU_min(count, size * n);
		if (count > 0)
			if ((errno = FSRead(stream->serialIn, &count, ptr)) != noErr)
				stat = IO_ERROR;
		if ((uint4)count < size * n)
			stat = IO_EOF;
		n = count / size;
	}
	else
		if (stream->fptr == NULL)
		{ //read from an IO_handle's buffer
			nread = size * n;
			if (nread > stream->len - (stream->ioptr - stream->buffer))
			{
				n = stream->len - (stream->ioptr - stream->buffer) / size;
				nread = size * n;
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
			// MW-2010-08-26: Taken from the Linux source, this changes the previous code
			//   to take into account pipes and such.
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
}

IO_stat MCS_write(const void *ptr, uint4 size, uint4 n, IO_handle stream)
{
	if (stream == NULL)
		return IO_ERROR;
	if (stream->serialOut != 0)
	{//write to serial port
		uint4 count = size * n;
		errno = FSWrite(stream->serialOut, (long*)&count, ptr);
		if (errno == noErr && count == size * n)
			return IO_NORMAL;
		return IO_ERROR;
	}

	if ((stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
		return MCU_dofakewrite(stream -> buffer, stream -> len, ptr, size, n);

	if (fwrite(ptr, size, n, stream->fptr) != n)
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_flush(IO_handle stream)
{ //flush file buffer
	if (stream->fptr != NULL)
		if (fflush(stream->fptr))
			return IO_ERROR;
	return IO_NORMAL;
}

// File positioning

Boolean MCS_eof(IO_handle stream)
{
	if (stream->fptr == NULL) //no dealing with real file
		return (uint4)(stream->ioptr - stream->buffer) == stream->len;
	return feof(stream->fptr);
}


IO_stat MCS_seek_cur(IO_handle stream, int64_t offset)
{
	// MW-2009-06-25: If this is a custom stream, call the appropriate callback.
	// MW-2009-06-30: Refactored to common implementation in mcio.cpp.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_seek_cur(stream, offset);

	/* seek to offset from the current file mark */
	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->ioptr + offset);
	else
		if (fseeko(stream->fptr, offset, SEEK_CUR) != 0)
			return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_set(IO_handle stream, int64_t offset)
{
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_seek_set(stream, offset);
	
	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->buffer + offset);
	else
		if (fseeko(stream->fptr, offset, SEEK_SET) != 0)
			return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_end(IO_handle stream, int64_t offset)
{ /* seek to offset from the end of the file */
	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->buffer + stream->len + offset);
	else
		if (fseeko(stream->fptr, offset, SEEK_END) != 0)
			return IO_ERROR;
	return IO_NORMAL;
}

int64_t MCS_tell(IO_handle stream)
{
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_tell(stream);

	if (stream->fptr != NULL)
		return ftello(stream->fptr);
	else
		return stream->ioptr - stream->buffer;
}


IO_stat MCS_sync(IO_handle stream)
{
	if (stream->fptr != NULL)
	{
		int4 pos = ftello(stream->fptr);
		if (fseek(stream->fptr, pos, SEEK_SET) != 0)
			return IO_ERROR;
	}
	return IO_NORMAL;
}

// File properties

int64_t MCS_fsize(IO_handle stream)
{
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_fsize(stream);

	if (stream->flags & IO_FAKE)
		return stream->len;

	// get file size of an Opened file
	struct stat buf;
	if (stream->fptr == NULL)
		return stream->len;
	int fd = fileno(stream->fptr);
	if (fstat(fd, (struct stat *)&buf))
		return 0;
	return buf.st_size;
}

// TESTCASE: Callers of MCS_setfiletype
void MCS_setfiletype(const char *p_new_path)
{
	FSRef t_fsref;
	if (MCS_pathtoref(p_new_path, &t_fsref) != noErr)
		return; // ignore errors
		
	FSCatalogInfo t_catalog;
	if (FSGetCatalogInfo(&t_fsref, kFSCatInfoFinderInfo, &t_catalog, NULL, NULL, NULL) == noErr)
	{
		// Set the creator and filetype of the catalog.
		memcpy(&((FileInfo *) t_catalog . finderInfo) -> fileType, &MCfiletype[4], 4);
		memcpy(&((FileInfo *) t_catalog . finderInfo) -> fileCreator, MCfiletype, 4);
		((FileInfo *) t_catalog . finderInfo) -> fileType = MCSwapInt32NetworkToHost(((FileInfo *) t_catalog . finderInfo) -> fileType);
		((FileInfo *) t_catalog . finderInfo) -> fileCreator = MCSwapInt32NetworkToHost(((FileInfo *) t_catalog . finderInfo) -> fileCreator);
	
		FSSetCatalogInfo(&t_fsref, kFSCatInfoFinderInfo, &t_catalog);
	}
}

// File UNIX-related properties

uint2 MCS_umask(uint2 mask)
{
	return 0;
}

int4 MCS_getumask()
{
	return 0;
}

void MCS_setumask(int4 newmask)
{
	//do nothing
}

IO_stat MCS_chmod(const char *path, uint2 mask)
{
	return IO_NORMAL;
}

Boolean MCS_noperm(const char *path)
{
	return False;
}


// File queries

bool MCS_exists(MCStringRef p_path, bool p_is_file)
{
	if (MCStringGetLength(p_path) == 0)
		return false;

	MCAutoStringRef t_resolved, t_utf8_path;
	if (!MCS_resolvepath(p_path, &t_resolved) ||
		!MCU_nativetoutf8(*t_resolved, &t_utf8_path))
		return false;

	bool t_found;
	struct stat buf;
	t_found = stat(MCStringGetCString(*t_utf8_path), (struct stat *)&buf) == 0;
	if (t_found)
		t_found = (p_is_file == ((buf.st_mode & S_IFDIR) == 0));

	return t_found;
}

// File actions

// MW-2007-07-16: [[ Bug 5214 ]] Use rename instead of FSExchangeObjects since
//   the latter isn't supported on all FS's.
// MW-2007-12-12: [[ Bug 5674 ]] Unfortunately, just renaming the current stack
//   causes all Finder meta-data to be lost, so what we will do is first try
//   to FSExchangeObjects and if that fails, do a rename.
Boolean MCS_backup(const char *p_src_path, const char *p_dst_path)
{
	bool t_error;
	t_error = false;
	
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_src_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}

	FSRef t_dst_parent_ref;
	FSRef t_dst_ref;
	UniChar *t_dst_leaf;
	t_dst_leaf = NULL;
	UniCharCount t_dst_leaf_length;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_dst_path, &t_dst_ref);
		if (t_os_error == noErr)
			FSDeleteObject(&t_dst_ref);
			
		// Get the information to create the file
		t_os_error = MCS_pathtoref_and_leaf(p_dst_path, t_dst_parent_ref, t_dst_leaf, t_dst_leaf_length);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSCatalogInfo t_dst_catalog;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSGetCatalogInfo(&t_src_ref, kFSCatInfoFinderInfo, &t_dst_catalog, NULL, NULL, NULL);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (!t_error)
	{
		memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileType, &MCfiletype[4], 4);
		memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator, MCfiletype, 4);
		((FileInfo *) t_dst_catalog . finderInfo) -> fileType = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileType);
		((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator);
	}	
	
	bool t_created_dst;
	t_created_dst = false;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSCreateFileUnicode(&t_dst_parent_ref, t_dst_leaf_length, t_dst_leaf, kFSCatInfoFinderInfo, &t_dst_catalog, &t_dst_ref, NULL);
		if (t_os_error == noErr)
			t_created_dst = true;
		else
			t_error = true;
	}
	
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (t_error && t_created_dst)
		FSDeleteObject(&t_dst_ref);
	
	if (t_dst_leaf != NULL)
		delete t_dst_leaf;
		
	if (t_error)
		t_error = !MCS_rename(p_src_path, p_dst_path);
		
	return !t_error;
}

Boolean MCS_unbackup(const char *p_src_path, const char *p_dst_path)
{
	bool t_error;
	t_error = false;
	
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_src_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSRef t_dst_ref;
	if (!t_error)
	{		
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_dst_path, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	// It appears that the source file here is the ~file, the backup file.
	// So copy it over to p_dst_path, and delete it.
	if (!t_error)
	{	
		OSErr t_os_error;
		t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSDeleteObject(&t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}

	if (t_error)
		t_error = !MCS_rename(p_src_path, p_dst_path);
		
	return !t_error;
}

IO_stat MCS_trunc(IO_handle stream)
{

	if (ftruncate(fileno(stream->fptr), ftell(stream->fptr)))
		return IO_ERROR;
	return IO_NORMAL;
}

// Anything else

bool MCS_tmpnam(MCStringRef& r_path)
{
	char *t_temp_file = nil;
	FSRef t_folder_ref;
	if (FSFindFolder(kOnSystemDisk, kTemporaryFolderType, TRUE, &t_folder_ref) == noErr)
	{
		t_temp_file = MCS_fsref_to_path(t_folder_ref);
		MCCStringAppendFormat(t_temp_file, "/tmp.%d.XXXXXXXX", getpid());
		
		int t_fd;
		t_fd = mkstemp(t_temp_file);
		if (t_fd == -1)
		{
			delete t_temp_file;
			return false;
		}

		close(t_fd);
		unlink(t_temp_file);
	}
	
	if (t_temp_file == nil)
	{
		r_path = MCValueRetain(kMCEmptyString);
		return true;
	}
	
	bool t_success = MCStringCreateWithCString(t_temp_file, r_path);
	delete t_temp_file;
	return t_success;
}
	
	
/********************************************************************/
/*                        Serial Handling                           */
/********************************************************************/ 

// Utilities

static void parseSerialControlStr(char *setting, struct termios *theTermios)
{
	int baud = 0;
	char *type = setting;
	char *value = NULL;
	if ((value = strchr(type, '=')) != NULL)
	{
		*value++ = '\0';
		if (MCU_strncasecmp(type, "baud", strlen(type)) == 0)
		{
			long baudrate = strtol(value, NULL, 10);
			if (baudrate == 57600)
				baud = B57600;
			else if (baudrate == 38400)
				baud = B38400;
			else if (baudrate == 28800)
				baud = B28800;
			else if (baudrate == 19200)
				baud = B19200;
			else if (baudrate == 16600)
				baud = B16600;
			else if (baudrate == 14400)
				baud = B14400;
			else if (baudrate == 9600)
				baud = B9600;
			else if (baudrate == 7200)
				baud = B7200;
			else if (baudrate == 4800)
				baud = B4800;
			else if (baudrate == 3600)
				baud = B4800;
			else if (baudrate == 2400)
				baud = B2400;
			else if (baudrate == 1800)
				baud = B1800;
			else if (baudrate == 1200)
				baud = B1200;
			else if (baudrate == 600)
				baud = B600;
			else if (baudrate == 300)
				baud = B300;
			cfsetispeed(theTermios, baud);
			cfsetospeed(theTermios, baud);
		}
		else if (MCU_strncasecmp(type, "parity", strlen(type)) == 0)
		{
			if (value[0] == 'N' || value[0] == 'n')
				theTermios->c_cflag &= ~(PARENB | PARODD);
			else if (value[0] == 'O' || value[0] == 'o')
				theTermios->c_cflag |= PARENB | PARODD;
			else if (value[0] == 'E' || value[0] == 'e')
				theTermios->c_cflag |= PARENB;
		}
		else if (MCU_strncasecmp(type, "data", strlen(type)) == 0)
		{
			short data = atoi(value);
			switch (data)
			{
			case 5:
				theTermios->c_cflag |= CS5;
				break;
			case 6:
				theTermios->c_cflag |= CS6;
				break;
			case 7:
				theTermios->c_cflag |= CS7;
				break;
			case 8:
				theTermios->c_cflag |= CS8;
				break;
			}
		}
		else if (MCU_strncasecmp(type, "stop", strlen(type)) == 0)
		{
			double stopbit = strtol(value, NULL, 10);
			if (stopbit == 1.0)
				theTermios->c_cflag &= ~CSTOPB;
			else if (stopbit == 1.5)
				theTermios->c_cflag &= ~CSTOPB;
			else if (stopbit == 2.0)
				theTermios->c_cflag |= CSTOPB;
		}
	}
}

static void configureSerialPort(int sRefNum)
{/****************************************************************************
	 *parse MCserialcontrolstring and set the serial output port to the settings*
	 *defined by MCserialcontrolstring accordingly                              *
	 ****************************************************************************/
	//initialize to the default setting
	struct termios	theTermios;
	if (tcgetattr(sRefNum, &theTermios) < 0)
	{
		// TODO: handle error appropriately
	}
	cfsetispeed(&theTermios,  B9600);
	theTermios.c_cflag = CS8;

	char *controlptr = MCStringGetCString(MCserialcontrolsettings);
	char *str = controlptr;
	char *each = NULL;
	while ((each = strchr(str, ' ')) != NULL)
	{
		*each = '\0';
		each++;
		if (str != NULL)
			parseSerialControlStr(str, &theTermios);
		str = each;
	}
	delete controlptr;
	//configure the serial output device
	parseSerialControlStr(str,&theTermios);
	if (tcsetattr(sRefNum, TCSANOW, &theTermios) < 0)
	{
		// TODO: handle error appropriately
	}
	return;
}


/********************************************************************/
/*                        Alias Handling                            */
/********************************************************************/ 

// MH Updating createalias to use FSRefs instead.
// MW-2007-12-18: [[ Bug 5679 ]] 'create alias' not working on OS X
// MW-2007-12-18: [[ Bug 1059 ]] 'create alias' doesn't work when passed a folder
Boolean MCS_createalias(const char *p_source_path, const char *p_dest_path)
{
	bool t_error;
	t_error = false;
	
	// Check if the destination exists already and return an error if it does
	if (!t_error)
	{
		FSRef t_dst_ref;
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_dest_path, &t_dst_ref);
		if (t_os_error == noErr)
			return False; // we expect an error
	}

	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_source_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}

	FSRef t_dst_parent_ref;
	UniChar *t_dst_leaf_name;
	UniCharCount t_dst_leaf_name_length;
	t_dst_leaf_name = NULL;
	t_dst_leaf_name_length = 0;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref_and_leaf(p_dest_path, t_dst_parent_ref, t_dst_leaf_name, t_dst_leaf_name_length);
		if (t_os_error != noErr)
			t_error = true;
	}

	AliasHandle t_alias;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSNewAlias(NULL, &t_src_ref, &t_alias);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	IconRef t_src_icon;
	t_src_icon = NULL;
	if (!t_error)
	{
		OSErr t_os_error;
		SInt16 t_unused_label;
		t_os_error = GetIconRefFromFileInfo(&t_src_ref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, &t_src_icon, &t_unused_label);
		if (t_os_error != noErr)
			t_src_icon = NULL;
	}
	
	IconFamilyHandle t_icon_family;
	t_icon_family = NULL;
	if (!t_error && t_src_icon != NULL)
	{
		OSErr t_os_error;
		IconRefToIconFamily(t_src_icon, kSelectorAllAvailableData, &t_icon_family);
	}
	
	HFSUniStr255 t_fork_name;
	if (!t_error)
		FSGetResourceForkName(&t_fork_name);

	FSRef t_dst_ref;
	FSSpec t_dst_spec;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSCreateResourceFile(&t_dst_parent_ref, t_dst_leaf_name_length, t_dst_leaf_name,
			kFSCatInfoNone, NULL, t_fork_name . length, t_fork_name . unicode, &t_dst_ref, &t_dst_spec);
		if (t_os_error != noErr)
			t_error = true;
	}

	ResFileRefNum t_res_file;
	bool t_res_file_opened;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSOpenResourceFile(&t_dst_ref, t_fork_name . length, t_fork_name . unicode, 3, &t_res_file);
		if (t_os_error != noErr)
			t_error = true;
		else
			t_res_file_opened = true;
	}

	if (!t_error)
	{
		AddResource((Handle)t_alias, rAliasType, 0, (ConstStr255Param)"");
		if (ResError() != noErr)
			t_error = true;
	}
	
	if (!t_error && t_icon_family != NULL)
		AddResource((Handle)t_icon_family, 'icns', -16496, NULL);

	if (t_res_file_opened)
		CloseResFile(t_res_file);
	
	if (!t_error)
	{
		FSCatalogInfo t_info;
		FSGetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info, NULL, NULL, NULL);
		((FileInfo *)&t_info . finderInfo) -> finderFlags |= kIsAlias;
		if (t_icon_family != NULL)
			((FileInfo *)&t_info . finderInfo) -> finderFlags |= kHasCustomIcon;
		FSSetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info);		
	}

	if (t_src_icon != NULL)
		ReleaseIconRef(t_src_icon);

	if (t_dst_leaf_name != NULL)
		delete t_dst_leaf_name;
	
	if (t_error)
	{
		if (t_icon_family != NULL)
			DisposeHandle((Handle)t_icon_family);
		FSDeleteObject(&t_dst_ref);
	}
		
	return !t_error;
}

bool MCS_resolvealias(MCStringRef p_path, MCStringRef& r_resolved, MCStringRef& r_error)
{
	FSRef t_fsref;

	OSErr t_os_error;
	t_os_error = MCS_pathtoref(p_path, t_fsref);
	if (t_os_error != noErr)
		return MCStringCreateWithCString("file not found", r_error);
	
	Boolean t_is_folder;
	Boolean t_is_alias;

	t_os_error = FSResolveAliasFile(&t_fsref, TRUE, &t_is_folder, &t_is_alias);
	if (t_os_error != noErr || !t_is_alias) // this always seems to be false
		return MCStringCreateWithCString("can't get alias", r_error);
	
	if (!MCS_fsref_to_path(t_fsref, r_resolved))
		return MCStringCreateWithCString("can't get alias path", r_error);

	return true;
}



/********************************************************************/
/*                       Resource Handling                          */
/********************************************************************/ 

class MCAutoResourceFileHandle
{
public:
	MCAutoResourceFileHandle()
	{
		m_res_file = 0;
	}
	
	~MCAutoResourceFileHandle()
	{
		if (m_res_file != 0)
			MCS_closeresourcefile(m_res_file);
	}
	
	short operator = (short p_res_file)
	{
		MCAssert(m_res_file == 0);
		m_res_file = p_res_file;
		return m_res_file;
	}
	
	short operator * (void)
	{
		return m_res_file;
	}
	
	short& operator & (void)
	{
		MCAssert(m_res_file == 0);
		return m_res_file;
	}
	
private:
	short m_res_file;
};

/*************************************************************************
 * 'which' param can be an id or a name of a resource. If the dest       *
 * file does not have a  resource fork we will create one for it         *
 *************************************************************************/
bool MCS_copyresource(MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type,
					  MCStringRef p_name, MCStringRef p_newid, MCStringRef& r_error)
{
	short prev_res_file = CurResFile(); //save the current resource fork
	MCAutoResourceFileHandle srcFileRefNum, destFileRefNum;
	
	if (!MCS_openresourcefile_with_path(p_source, fsRdPerm, false, &srcFileRefNum, r_error)) // RESFILE
		return false;
	if (r_error != nil)
		return true;
		
	// exclusive read/write permission for the dest file
	// and do not set result if there is error
	if (!MCS_openresourcefile_with_path(p_dest, fsRdWrPerm, true, &destFileRefNum, r_error)) // RESFILE
		return false;
	if (r_error != nil)
		return true;
	
	UseResFile(*destFileRefNum);
	
	if (MCStringGetLength(p_type) != 4)
	{ 
		//copying the entire resource file
		short resTypeCount = Count1Types();
		short resCount;
		ResType resourceType;
		Handle hres;
		for (uindex_t i = 1; i <= resTypeCount; i++)
		{
			UseResFile(*srcFileRefNum);
			Get1IndType(&resourceType, i);
			resCount = Count1Resources(resourceType);
			Str255 rname;
			short id;
			ResType type;
			for (uindex_t j = 1; j <= resCount; j++)
			{
				UseResFile(*srcFileRefNum);
				hres = Get1IndResource(resourceType, j);
				if (hres != NULL)
				{
					GetResInfo(hres, &id, &type, rname);
					DetachResource(hres);
					UseResFile(*destFileRefNum);
					AddResource(hres, type, id, rname);
				}
			}	//loop through each res within each res type
		} //loop through each res type
		
		UseResFile(prev_res_file); //restore the original state
		return true;
	}
	
	//copy only one resource, specified either by id or name
	UseResFile(*srcFileRefNum); //set the source resource file as the current file
	
	ResType restype;
	// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
	restype = MCSwapInt32HostToNetwork(*(uint32_t*)MCStringGetNativeCharPtr(p_type));

	Str255 t_resname;
	
	const char *whichres = MCStringGetCString(p_name);
	const char *eptr = whichres;    /* find out whichres is a name or an id */
	
	long rid = strtol(whichres, (char **)&eptr, 10); // if can't convert, then the value
	// passed in is a resource name
	Boolean hasResName = False;
	Handle rh = NULL;
	
	if (eptr == whichres)
	{  /*did not do the conversion, use resource name */
		c2pstrcpy(t_resname, MCStringGetCString(p_name));
		rh = Get1NamedResource(restype, t_resname);
		hasResName = True;
	}
	else //we got an resource id
		rh = Get1Resource(restype, rid);
	if (rh == NULL || *rh == 0)
	{//bail out if resource handle is bad
		errno = ResError();
		
		UseResFile(prev_res_file); //restore to the original state
		return MCStringCreateWithCString("can't find the resource specified", r_error);
	}
	
	unsigned char resourceName[255];
	short srcID;        //let's get it's resource name.
	ResType srcType;
	if (!hasResName) //No name specified for the resource to be copied
		GetResInfo(rh, &srcID, &srcType, resourceName);
	
	//detach the src res file, and select the dest res file
	DetachResource(rh);
	UseResFile(*destFileRefNum);
	unsigned long newResID;
	if (p_newid == NULL)
		newResID = srcID; //use the resource id of the src file's resource
	else
		newResID = strtoul(MCStringGetCString(p_newid), (char **)&eptr, 10); //use the id passed in
	
	//delete the resource by id to be copied in the destination file, if it existed
	Handle rhandle = Get1Resource(restype, newResID);
	if (rhandle != NULL && ResError() != resNotFound)
		RemoveResource(rhandle);
	
	//now, let's copy the resource to the dest file
	if (!hasResName)
		AddResource(rh, restype, (short)newResID, (unsigned char*)resourceName);
	else
		AddResource(rh, restype, (short)newResID, t_resname);
	//errno = ResError();//if errno == 0 means O.K.
	OSErr t_os_error = ResError();
	
	UseResFile(prev_res_file); //restore to the original state
	
	return true;
}


/*********************************************************************
 * Functions process resources:					     *
 * MCS_copyresource()						     *
 * MCS_deleteresource()						     *
 * MCS_getresource()						     *
 * MCS_getresources()						     *
 * MCS_setresource()						     *
 *********************************************************************/
void MCS_copyresourcefork(MCStringRef p_source, MCStringRef p_destination)
{

	const char *t_dest = nil;
	if (p_destination != nil)
		t_dest = MCStringGetCString(p_destination);

	const char *t_error;
	t_error = NULL;
	
	SInt16 t_source_ref;
	bool t_source_fork_opened;
	t_source_fork_opened = false;
	t_error = MCS_openresourcefork_with_path(MCStringGetCString(p_source), fsRdPerm, false, &t_source_ref); // RESFORK
	if (t_error == NULL)
		t_source_fork_opened = true;
	
	SInt16 t_dest_ref;
	bool t_dest_fork_opened;
	t_dest_fork_opened = false;
	if (t_error == NULL)
		t_error = MCS_openresourcefork_with_path(t_dest, fsWrPerm, true, &t_dest_ref); // RESFORK
	if (t_error == NULL)
		t_dest_fork_opened = true;

	// In block sizes of 1k, copy over the data from source to destination..
	char *t_buffer = new char[65536];
	if (t_error == NULL)
	{
		OSErr t_os_read_error, t_os_write_error;
		do {
			ByteCount t_actual_read, t_actual_write;
			t_os_read_error = FSReadFork(t_source_ref, fsFromMark, 0, 65536, t_buffer, &t_actual_read);
			if (t_os_read_error == noErr || t_os_read_error == eofErr)
			{
				t_os_write_error = FSWriteFork(t_dest_ref, fsFromMark, 0, t_actual_read, t_buffer, &t_actual_write);
				if (t_os_write_error != noErr || t_actual_write != t_actual_read)
					t_error = "can't copy resource";
			}
		} while(t_error == NULL && t_os_read_error == noErr);
	}
	
	delete t_buffer;
	if (t_source_fork_opened)
		FSCloseFork(t_source_ref);
	if (t_dest_fork_opened)
		FSCloseFork(t_dest_ref);
}

// MH-2007-04-02 rewriting this function to support FSRefs, long filenames in particular.
// This function is quite specific to the older API resource routines, so the prototype needs to be adjusted.
//OSErr MCS_openResFile(const char *p_file, SignedByte p_permission, short *p_file_ref_num, Boolean p_create, Boolean p_set_result)


bool MCS_openresourcefile_with_fsref(FSRef& p_ref, SInt8 p_permissions, bool p_create, SInt16& r_fileref_num, MCStringRef& r_error)
{
	FSSpec fspec;
	
	if (FSGetCatalogInfo(&p_ref, 0, NULL, NULL, &fspec, NULL) != noErr)
		return MCStringCreateWithCString("file not found", r_error);
	
	r_fileref_num = FSOpenResFile(&p_ref, p_permissions);
	if (p_create && r_fileref_num < 0)
	{
		OSType t_creator, t_ftype;
		CInfoPBRec t_cpb;
		MCMemoryClear(&t_cpb, sizeof(t_cpb));
		t_cpb.hFileInfo.ioNamePtr = fspec.name;
		t_cpb.hFileInfo.ioVRefNum = fspec.vRefNum;
		t_cpb.hFileInfo.ioDirID = fspec.parID;
		/* DEPRECATED */ if (PBGetCatInfoSync(&t_cpb) == noErr)
		{
			t_creator = t_cpb.hFileInfo.ioFlFndrInfo.fdCreator;
			t_ftype = t_cpb.hFileInfo.ioFlFndrInfo.fdType;
		}
		else
		{
			t_creator = MCSwapInt32NetworkToHost(*(OSType*)MCfiletype);
			t_ftype = MCSwapInt32NetworkToHost(*(OSType*)(MCfiletype + 4));
		}
		/* DEPRECATED */ FSpCreateResFile(&fspec, t_creator, t_ftype, smRoman);
		
		if ((errno = ResError()) != noErr)
			return MCStringCreateWithCString("can't create resource fork", r_error);
		
		/* DEPRECATED */ r_fileref_num = FSpOpenResFile(&fspec, p_permissions);
	}
	
	if (r_fileref_num < 0)
	{
		errno = fnfErr;
		return MCStringCreateWithCString("Can't open resource fork", r_error);
	}
	
	if ((errno = ResError()) != noErr)
		return MCStringCreateWithCString("Error opening resource fork", r_error);
	
	return true;
}

const char *MCS_openresourcefile_with_fsref(FSRef *p_ref, SInt8 permission, bool create, SInt16 *fileRefNum)
{
	FSSpec fspec;
	
	if (FSGetCatalogInfo(p_ref, 0, NULL, NULL, &fspec, NULL) != noErr)
		return "file not found";
	
	if ((*fileRefNum = FSpOpenResFile(&fspec, permission)) < 0)
	{
		if (create)
		{
			OSType creator, ftype;
			CInfoPBRec cpb;
			memset(&cpb, 0, sizeof(CInfoPBRec));
			cpb.hFileInfo.ioNamePtr = fspec.name;
			cpb.hFileInfo.ioVRefNum = fspec.vRefNum;
			cpb.hFileInfo.ioDirID = fspec.parID;
			if (PBGetCatInfoSync(&cpb) == noErr)
			{
				memcpy(&creator, &cpb.hFileInfo.ioFlFndrInfo.fdCreator, 4);
				memcpy(&ftype, &cpb.hFileInfo.ioFlFndrInfo.fdType, 4);
			}
			else
			{
				memcpy((char*)&creator, MCfiletype, 4);
				memcpy((char*)&ftype, &MCfiletype[4], 4);
				creator = MCSwapInt32NetworkToHost(creator);
				ftype = MCSwapInt32NetworkToHost(ftype);
			}
			FSpCreateResFile(&fspec, creator, ftype, smRoman);
			
			if ((errno = ResError()) != noErr)
				return "can't create resource fork";
				
			*fileRefNum = FSpOpenResFile(&fspec, permission);
		}
		
		if (*fileRefNum < 0)
		{
			errno = fnfErr;
			return "Can't open resource fork";
		}
		
		if ((errno = ResError()) != noErr)
			return "Error opening resource fork";
	}
	
	return NULL;
}

void MCS_closeresourcefile(SInt16 p_ref)
{
	OSErr t_err;
	CloseResFile(p_ref);
	t_err = ResError();
}

const char *MCS_openresourcefork_with_fsref(FSRef *p_ref, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref)
{
	const char *t_error;
	t_error = NULL;
	
	HFSUniStr255 t_resource_fork_name;
	if (t_error == NULL)
	{
		OSErr t_os_error;
		t_os_error = FSGetResourceForkName(&t_resource_fork_name);
		if (t_os_error != noErr)
			t_error = "couldn't get resource fork name";
	}
	
	// Attempt to create a resource fork if required.
	if (t_error == NULL && p_create)
	{
		OSErr t_os_error;
		t_os_error = FSCreateResourceFork(p_ref, (UniCharCount)t_resource_fork_name . length, t_resource_fork_name . unicode, 0);
		if (t_os_error != noErr && t_os_error != errFSForkExists)
			t_error = "can't create resource fork";
	}
	
	// Open it..
	SInt16 t_fork_ref;
	bool t_fork_opened;
	t_fork_opened = false;
	if (t_error == NULL)
	{
		OSErr t_os_error;
		t_os_error = FSOpenFork(p_ref, (UniCharCount)t_resource_fork_name . length, t_resource_fork_name . unicode, p_permission, &t_fork_ref);
		if (t_os_error == noErr)
			t_fork_opened = true;
		else
			t_error = "can't open resource fork";
	}
	
	*r_fork_ref = t_fork_ref;
	return t_error;
}
/*
const char *MCS_openresourcefork_with_path(const char *p_path, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref)
{
	const char *t_error;
	t_error = NULL;
	
	char *t_utf8_path;
	t_utf8_path = path2utf(strdup(p_path));
	
	FSRef t_ref;
	OSErr t_os_error;
	t_os_error = MCS_pathtoref(p_path, &t_ref);
	if (t_os_error != noErr)
		t_error = "can't open file";
		
	if (t_error == NULL)
		t_error = MCS_openresourcefork_with_fsref(&t_ref, p_permission, p_create, r_fork_ref);
		
	delete t_utf8_path;
		
	return t_error;	
}
*/

void MCS_openresourcefork_with_path(MCStringRef p_path, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref, MCStringRef& r_error)
{
	FSRef t_ref;
	OSErr t_os_error;
	t_os_error = MCS_pathtoref(p_path, &t_ref);
	if (t_os_error != noErr)
		return MCStringCreateWithCString("can't open file", r_error);
		
	return MCS_openresourcefork_with_fsref(&t_ref, p_permission, p_create, r_fork_ref, r_error);
		
	

}

bool MCS_openresourcefile_with_path(MCStringRef p_path, SInt8 p_permission, bool p_create, SInt16& r_fork_ref, MCStringRef& r_error)
{
//	MCAutoStringRef t_utf8_path;
//	if (!MCU_nativetoutf8(p_path, &t_utf8_path))
//		return false;
	
	FSRef t_ref;
	OSErr t_os_error;
	
	t_os_error = MCS_pathtoref(p_path, t_ref);
	if (t_os_error != noErr)
		return MCStringCreateWithCString("can't open file", r_error);
	
	return MCS_openresourcefile_with_fsref(t_ref, p_permission, p_create, r_fork_ref, r_error);
}

/*
const char *MCS_openresourcefile_with_path(const char *p_path, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref)
{
	const char *t_error;
	t_error = NULL;
	
	char *t_utf8_path;
	t_utf8_path = path2utf(strdup(p_path));
	
	FSRef t_ref;
	OSErr t_os_error;
	t_os_error = MCS_pathtoref(p_path, &t_ref);
	if (t_os_error != noErr)
		t_error = "can't open file";
		
	if (t_error == NULL)
		t_error = MCS_openresourcefile_with_fsref(&t_ref, p_permission, p_create, r_fork_ref);
		
	delete t_utf8_path;
		
	return t_error;	
}
*/

const char *MCS_openresourcefork_with_path(const MCString& p_path, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref)
{
	const char *t_error;
	t_error = NULL;
	
	FSRef t_ref;
	OSErr t_os_error;
	t_os_error = MCS_pathtoref(p_path, &t_ref);
	if (t_os_error != noErr)
		t_error = "can't open file";
		
	if (t_error == NULL)
		t_error = MCS_openresourcefork_with_fsref(&t_ref, p_permission, p_create, r_fork_ref);
		
	return t_error;	
}

void MCS_loadresfile(MCExecPoint &ep)
{
	if (!MCSecureModeCanAccessDisk())
	{
		ep.clear();
		MCresult->sets("can't open file");
		return;
	}
	/*
	char *t_path = ep.getsvalue().clone();
	ep.clear();
	
	const char *t_open_res_error;
	t_open_res_error = NULL;
	*/
	MCAutoStringRef t_open_res_error_string;
	MCAutoStringRef t_path_string;
	/* UNCHECKED */ ep . copyasstringref(&t_path_string);

	short fRefNum;
	MCS_openresourcefork_with_path(*t_path_string, fsRdPerm, false, &fRefNum, &t_open_res_error_string); // RESFORK

	const char *t_open_res_error = nil;
	if (t_open_res_error_string != nil)
		t_open_res_error =  MCStringGetCString(p_open_res_error_string);
	
	if (t_open_res_error != NULL)
	{
		MCresult -> sets(t_open_res_error);
		
		return;
	}
		
	//file mark should be pointing to 0 which is the begining of the file
	//let's get the end of file mark to determine the file size
	long fsize, toread;
	if ((errno = GetEOF(fRefNum, &fsize)) != noErr)
		MCresult->sets("can't get file size");
	else
	{
		toread = fsize;
		char *buffer;
		/* UNCHECKED */ ep . reserve(fsize, buffer);
		if (buffer == NULL)
			MCresult->sets("can't create data buffer");
		else
		{
			errno = FSRead(fRefNum, &toread, buffer);
			if (toread != fsize) //did not read bytes as specified
				MCresult->sets("error reading file");
			else
			{
				ep . commit(fsize);
				MCresult->clear(False);
			}
		}
	}
	
	FSCloseFork(fRefNum);

}

// MH-2007-04-02: [[ Bug 705 ]] resfile: URLs do not work with long filenames...
void MCS_saveresfile(const MCString& p_path, const MCString p_data)
{
	const char *t_error;
	t_error = NULL;

	if (!MCSecureModeCanAccessDisk())
		t_error = "can't open file";
	
	SInt16 t_fork_ref;
	bool t_fork_opened;
	t_fork_opened = false;
	
	if (t_error == NULL)
	{
		t_error = MCS_openresourcefork_with_path(p_path, fsWrPerm, true, &t_fork_ref); // RESFORK
		if (t_error == NULL)
			t_fork_opened = true;
	}
	
	if (t_error == NULL)
	{
		OSErr t_os_error;
		ByteCount t_actual_count;
		t_os_error = FSWriteFork(t_fork_ref, fsFromStart, 0, p_data . getlength(), (const void *)p_data . getstring(), &t_actual_count);
		if (t_os_error == noErr && t_actual_count == (ByteCount)p_data . getlength())
			FSSetForkSize(t_fork_ref, fsFromStart, t_actual_count);
		else
			t_error = "error writing file";
	}
	
	if (t_fork_opened)
		FSCloseFork(t_fork_ref);
	
	if (t_error != NULL)
		MCresult -> sets(t_error);
	else
		MCresult -> clear(False);
}

void MCS_deleteresource(const char *resourcefile, const char *rtype,
                        const char *which)
{
	ResType restype;
	short rfRefNum;
	memcpy((char *)&restype, rtype, 4); /* let's get the resource type first */
	// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
	restype = (ResType)MCSwapInt32HostToNetwork(restype);

	const char *t_open_res_error;
	t_open_res_error = MCS_openresourcefile_with_path(resourcefile, fsRdWrPerm, true, &rfRefNum); // RESFILE
	if (t_open_res_error != NULL)
	{
		MCresult -> sets(t_open_res_error);
		return;
	}

	Handle rh = NULL;
	const char *eptr = (char *)which;     /* find out if we got a name or an id */
	long rid = strtol(which, (char **)&eptr, 10);
	if (eptr == which)
	{     /* did not do conversion, so use resource name */
		unsigned char *pname = c2pstr((char *)which);
		rh = Get1NamedResource(restype, pname);
	}
	else                  /* 'which' param is an resrouce id */
		rh = Get1Resource(restype, rid);
	if (rh == NULL)
		MCresult->sets("can't find the resource specified");
	else
	{
		SetResAttrs(rh, 0); // override protect flag
		RemoveResource(rh);
		if ((errno = ResError()) != noErr)
			MCresult->sets("can't remove the resource specified");
		DisposeHandle(rh);
	}
	
	MCS_closeresourcefile(rfRefNum);
}

/* WRAPPER */
bool MCS_deleteresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_error)
{
	MCS_deleteresource(MCStringGetCString(p_source), MCStringGetCString(p_type), MCStringGetCString(p_name));
	if (MCresult->isclear())
		return true;

	MCAssert(MCValueGetTypeCode(MCresult->getvalueref()) == kMCValueTypeCodeString);

	return MCStringCopy((MCStringRef)MCresult->getvalueref(), r_error);
}

void MCS_getresource(const char *resourcefile, const char *restype,
                     const char *name, MCExecPoint &ep)
{
	short resFileRefNum;
	const char *t_open_res_error;
	t_open_res_error = MCS_openresourcefile_with_path(resourcefile, fsRdPerm, true, &resFileRefNum); // RESFILE
	if (t_open_res_error != NULL)
	{	
		MCresult -> sets(t_open_res_error);
		return;
	}

	ResType rtype;
	memcpy((char *)&rtype, restype, 4);
	// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
	rtype = (ResType)MCSwapInt32HostToNetwork(rtype);

	/* test to see if "name" is a resource name or an id */
	char *whichres = strclone(name);
	const char *eptr = (char *)name;
	long rid = strtol(whichres, (char **)&eptr, 10);

	unsigned char *rname;
	Handle rh = NULL; //resource handle
	if (eptr == whichres)
	{  /* conversion failed, so 'name' is resource name*/
		rname = c2pstr((char *)whichres); //resource name in Pascal
		rh = Get1NamedResource(rtype, rname);
	}
	else //we got an resrouce id, the 'name' specifies an resource id
		rh = Get1Resource(rtype, rid);
	delete whichres;

	if (rh == NULL)
	{
		errno = ResError();
		MCresult->sets("can't find specified resource");
		MCS_closeresourcefile(resFileRefNum);
		return;
	}
	//getting the the resource's size throuth the resource handle
	int4 resLength = GetHandleSize(rh);
	if (resLength <= 0)
	{
		MCresult->sets("can't get resouce length.");
		MCS_closeresourcefile(resFileRefNum);
		return;
	}
	// store the resource into ep and return
	ep.copysvalue((const char *)*rh, resLength);
	MCresult->clear();
	MCS_closeresourcefile(resFileRefNum);
}

/* WRAPPER */
bool MCS_getresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_value, MCStringRef& r_error)
{
	MCExecPoint ep(nil, nil, nil);
	MCS_getresource(MCStringGetCString(p_source), MCStringGetCString(p_type), MCStringGetCString(p_name), ep);
	if (MCresult->isclear())
		return ep.copyasstringref(r_value);
	
	MCAssert(MCValueGetTypeCode(MCresult->getvalueref()) == kMCValueTypeCodeString);
	
	return MCStringCopy((MCStringRef)MCresult->getvalueref(), r_error);
}

bool MCS_getresources(MCStringRef p_source, MCStringRef p_type, MCListRef& r_resources, MCStringRef& r_error)
{ /* get resources from the resource fork of file 'path',
	   * if resource type is not empty, only resources of the specified type
	   * are listed. otherwise lists all resources from the
	   * resource fork.					    */

	MCAutoResourceFileHandle resFileRefNum;
	if (!MCS_openresourcefile_with_path(p_source, fsRdPerm, true, &resFileRefNum, r_error))
		return false;
	if (r_error != nil)
		return true;
	
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	SetResLoad(False);
	//since most recently opened file is place on the top of the search
	//path, no need to call UseResFile() to set this resource file as
	//the current file
	
	bool t_success = true;
	errno = noErr;
	
	ResType rtype, type;
	if (p_type != nil)
	{ //get the resource info specified by the resource type
		// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
		rtype = MCSwapInt32HostToNetwork(*(uint32_t*)MCStringGetNativeCharPtr(p_type));
		t_success = getResourceInfo(*t_list, rtype);
	}
	else
	{               //rtype is NULL, return All the resources
		short typeCount = Count1Types(); //find out how many resource type there is
		if (ResError() != noErr)
		{
			errno = ResError();
			t_success = false;
		}
		for (uindex_t i = 1; t_success && i <= typeCount; i++)
		{
			Get1IndType(&type, i);
			if (ResError() != noErr || type == 0)
				continue;
			t_success = getResourceInfo(*t_list, type);
		}
	}
	SetResLoad(True);
	
	if (t_success)
	{
		return MCListCopy(*t_list, r_resources);
	}
	else if (errno != noErr)
	{
		r_resources = MCValueRetain(kMCEmptyList);
		r_error = MCValueRetain(kMCEmptyString);
		return true;
	}
	
	return false;
}

void MCS_setresource(const char *resourcefile, const char *type,
                     const char *id, const char *name, const char *attrib,
                     const MCString &s)
/* set a resource in the file specified.  either name or id can be empty,
 * attrib is the attributes of the resource. It's a ";" seperated list */
{
	short newflags = 0; // parse up the attributes
	if (strlen(attrib) != 0)
	{
		const char *sptr = attrib;
		do
		{
			switch (*sptr++)
			{
			case 'S':
			case 's':
				newflags |= resSysHeap;
				break;
			case 'U':
			case 'u':
				newflags |= resPurgeable;
				break;
			case 'L':
			case 'l':
				newflags |= resLocked;
				break;
			case 'P':
			case 'p':
				newflags |= resProtected;
				break;
			case 'R':
			case 'r':
				newflags |= resPreload;
				break;
			case 'C':
			case 'c':
				newflags |= resChanged;
				break;
			}
		}
		while (*sptr);
	}
	else
		newflags |= resChanged;

	ResType rtype;
	memcpy((char *)&rtype, type, 4);
	// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
	rtype = (ResType)MCSwapInt32HostToNetwork(rtype);
	short rid = 0;
	if (strlen(id) != 0)
	{
		const char *eptr;
		rid = (short)strtol(id, (char **)&eptr, 10);
	}
	short resFileRefNum; //open resource fork for read and write permission
	const char *t_open_res_error;
	t_open_res_error = MCS_openresourcefile_with_path(resourcefile, fsRdWrPerm, true, &resFileRefNum); // RESFILE
	if (t_open_res_error != NULL)
	{	
		MCresult -> sets(t_open_res_error);
		return;
	}

	Handle rh = NULL;
	if (rid != 0)
		rh = Get1Resource(rtype, rid);
	else
	{
		char *whichres = strclone(name);
		unsigned char *rname = c2pstr(whichres); //resource name in Pascal
		rh = Get1NamedResource(rtype, rname);
		delete whichres;
	}

	Str255 newname;
	strcpy((char *)newname, name);
	c2pstr((char *)newname);
	if (rh != NULL)
	{
		SInt16 tid;
		ResType ttype;
		Str255 tname;
		GetResInfo(rh, &tid, &ttype, tname);
		if (strlen(name) == 0)
			pStrcpy(newname, tname);
		else
			if (strlen(id) == 0)
				rid = tid;
		if (strlen(attrib) == 0)
			newflags = GetResAttrs(rh) | resChanged;
		SetResAttrs(rh, 0); // override protect flag
		RemoveResource(rh);
		DisposeHandle(rh);
	}
	if (rid == 0)
		rid = UniqueID(rtype);

	uint4 len = s.getlength();
	rh = NewHandle(len);

	if (rh == NULL)
		MCresult->sets("can't create resource handle");
	else
	{
		memcpy(*rh, s.getstring(), len);
		AddResource(rh, rtype, rid, newname);
		if ((errno = ResError()) != noErr)
		{
			DisposeHandle(rh);
			MCresult->sets("can't add resource");
		}
		else
			SetResAttrs(rh, newflags);
	}
	MCS_closeresourcefile(resFileRefNum);
}

/* WRAPPER */
extern bool MCS_setresource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_id, MCStringRef p_name,
							MCStringRef p_flags, MCStringRef p_value, MCStringRef& r_error)
{
	MCS_setresource(MCStringGetCString(p_source), MCStringGetCString(p_type), MCStringGetCString(p_id), MCStringGetCString(p_name), MCStringGetCString(p_flags), MCStringGetOldString(p_value));
	if (MCresult->isclear())
		return true;
	
	MCAssert(MCValueGetTypeCode(MCresult->getvalueref()) == kMCValueTypeCodeString);
	
	return MCStringCopy((MCStringRef)MCresult->getvalueref(), r_error);
}

// Resource utility functions

static bool getResourceInfo(MCListRef p_list, ResType searchType)
{ /* get info of resources of a resource type and build a list of
	   * <resName>, <resID>, <resType>, <resSize> on each line */
	Handle rh;
	short rid;
	ResType rtype;
	Str255 rname;  //Pascal string
	char cstr[256];  //C string
	char typetmp[5]; //buffer for storing type string in c format
	short total = Count1Resources(searchType);
	if (ResError() != noErr)
	{
		errno = ResError();
		return false;
	}
	char buffer[4 + U2L + 255 + U4L + 6];
	for (uindex_t i = 1 ; i <= total ; i++)
	{
		if ((rh = Get1IndResource(searchType, i)) == NULL)
			continue;
		GetResInfo(rh, &rid, &rtype, rname);
		p2cstrcpy(cstr, rname); //convert to C string
		// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
		rtype = (ResType)MCSwapInt32NetworkToHost(rtype);
		memcpy(typetmp, (char*)&rtype, 4);
		typetmp[4] = '\0';
		//format res info into "type, id, name, size, attributes" string --
		short flags = GetResAttrs(rh);
		char fstring[7];
		char *sptr = fstring;
		if (flags & resSysHeap)
			*sptr++ = 'S';
		if (flags & resPurgeable)
			*sptr++ = 'U';
		if (flags & resLocked)
			*sptr++ = 'L';
		if (flags & resProtected)
			*sptr++ = 'P';
		if (flags & resPreload)
			*sptr++ = 'R';
		if (flags & resChanged)
			*sptr++ = 'C';
		*sptr = '\0';
		
		MCAutoStringRef t_string;
		if (!MCStringFormat(&t_string, "%4s,%d,%s,%ld,%s\n", typetmp, rid, cstr,
		        GetMaxResourceSize(rh), fstring))
			return false;
		if (!MCListAppend(p_list, *t_string))
			return false;
	}
	
	return true;
}


/********************************************************************/
/*                        Folder Handling                           */
/********************************************************************/ 

// Directory creation and removal

Boolean MCS_mkdir(const char *path)
{

	char *newpath = path2utf(MCS_resolvepath(path));
	Boolean done = mkdir(newpath, 0777) == 0;
	delete newpath;
	return done;
}

Boolean MCS_rmdir(const char *path)
{

	char *newpath = path2utf(MCS_resolvepath(path));
	Boolean done = rmdir(newpath) == 0;
	delete newpath;
	return done;
}

Boolean MCS_unlink(const char *path)
{
	char *newpath = path2utf(MCS_resolvepath(path));
	Boolean done = remove(newpath) == 0;
	delete newpath;
	return done;
}

// Setting and Getting the current directory

bool MCS_setcurdir(MCStringRef p_path)
{
	return MCS_setcurdir(MCStringGetCString(p_path)) == True;
}

// MW-2006-04-07: Bug 3201 - MCS_resolvepath returns NULL if unable to find a ~<username> folder.
Boolean MCS_setcurdir(const char *path)
{
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(path);
	if (t_resolved_path == NULL)
		return False;

	char *newpath = NULL;
	newpath = path2utf(t_resolved_path);
	
	Boolean done = chdir(newpath) == 0;
	delete newpath;
	if (!done)
		return False;
	
	return True;
}

bool MCS_getcurdir(MCStringRef& r_path)
{
	char namebuf[PATH_MAX + 2];
	if (NULL == getcwd(namebuf, PATH_MAX))
		return false;

	MCAutoNativeCharArray t_buffer;
	if (!t_buffer.New(PATH_MAX + 1))
		return false;

	uint4 outlen;
	outlen = PATH_MAX + 1;
	MCS_utf8tonative(namebuf, strlen(namebuf), (char*)t_buffer.Chars(), outlen);
	t_buffer.Shrink(outlen);
	return t_buffer.CreateStringAndRelease(r_path);
}

// Canonical path resolution

char *MCS_get_canonical_path(const char *p_path)
{
	char *t_path = NULL;
	
	t_path = MCS_resolvepath(p_path);
	MCU_fix_path(t_path);
	
	return t_path;
}

// Special Folders

// MW-2012-10-10: [[ Bug 10453 ]] Added 'mactag' field which is the tag to use in FSFindFolder.
//   This allows macfolder to be 0, which means don't alias the tag to the specified disk.
typedef struct
{
	MCNameRef *token;
	unsigned long macfolder;
	OSType domain;
	unsigned long mactag;
}
sysfolders;

// MW-2008-01-18: [[ Bug 5799 ]] It seems that we are requesting things in the
//   wrong domain - particularly for 'temp'. See:
// http://lists.apple.com/archives/carbon-development/2003/Oct/msg00318.html

static sysfolders sysfolderlist[] = {
                                        {&MCN_apple, 'amnu', kOnAppropriateDisk, 'amnu'},
                                        {&MCN_desktop, 'desk', kOnAppropriateDisk, 'desk'},
                                        {&MCN_control, 'ctrl', kOnAppropriateDisk, 'ctrl'},
                                        {&MCN_extension,'extn', kOnAppropriateDisk, 'extn'},
                                        {&MCN_fonts,'font', kOnAppropriateDisk, 'font'},
                                        {&MCN_preferences,'pref', kUserDomain, 'pref'},
                                        {&MCN_temporary,'temp', kUserDomain, 'temp'},
                                        {&MCN_system, 'macs', kOnAppropriateDisk, 'macs'},
										// TS-2007-08-20: Added to allow a common notion of "home" between all platforms
									    {&MCN_home, 'cusr', kUserDomain, 'cusr'},
										// MW-2007-09-11: Added for uniformity across platforms
										{&MCN_documents, 'docs', kUserDomain, 'docs'},
										// MW-2007-10-08: [[ Bug 10277 ] Add support for the 'application support' at user level.
										{&MCN_support, 0, kUserDomain, 'asup'},
                                    };

bool MCS_specialfolder_to_mac_folder(MCStringRef p_type, uint32_t& r_folder, OSType& r_domain)
{
	for (uindex_t i = 0; i < ELEMENTS(sysfolderlist); i++)
	{
		if (MCStringIsEqualTo(p_type, MCNameGetString(*(sysfolderlist[i].token)), kMCStringOptionCompareCaseless))
		{
			r_folder = sysfolderlist[i].macfolder;
			r_domain = sysfolderlist[i].domain;
            return true;
		}
	}
    return false;
}

// MW-2008-06-18: [[ Bug 6577 ]] specialFolderPath("home") didn't work as it is 4 chars long and
//   the sysfolderlist was being searched second.
// MW-2008-06-18: [[ Bug 6578 ]] specialFolderPath("temp") returns empty sometimes, presumably
//   because the folder wasn't necessarily being created.
bool MCS_getspecialfolder(MCExecContext& ctxt, MCStringRef p_type, MCStringRef& r_path)
{
	uint32_t t_mac_folder = 0;
	OSType t_domain = kOnAppropriateDisk;
	bool t_found_folder = false;

	if (MCS_specialfolder_to_mac_folder(p_type, t_mac_folder, t_domain))
		t_found_folder = true;
	else if (MCStringGetLength(p_type) == 4)
	{
		t_mac_folder = MCSwapInt32NetworkToHost(*((uint32_t*)MCStringGetBytePtr(p_type)));
			
		uindex_t t_i;
		for (t_i = 0 ; t_i < ELEMENTS(sysfolderlist); t_i++)
			if (t_mac_folder == sysfolderlist[t_i] . macfolder)
			{
				t_domain = sysfolderlist[t_i] . domain;
				t_mac_folder = sysfolderlist[t_i] . mactag;
				t_found_folder = true;
				break;
			}
	}

	FSRef t_folder_ref;
	if (t_found_folder)
	{
		OSErr t_os_error;
		Boolean t_create_folder;
		t_create_folder = t_domain == kUserDomain ? kCreateFolder : kDontCreateFolder;
		t_os_error = FSFindFolder(t_domain, t_mac_folder, t_create_folder, &t_folder_ref);
		t_found_folder = t_os_error == noErr;
	}

	if (!t_found_folder)
	{
		r_path = MCValueRetain(kMCEmptyString);
		return true;
	}
		
	return MCS_fsref_to_path(t_folder_ref, r_path);
}


/********************************************************************/
/*                  General Filesystem Handling                     */
/********************************************************************/ 

#define CATALOG_MAX_ENTRIES 16
bool MCS_getentries(bool p_files, bool p_detailed, MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	OSStatus t_os_status;
	
	Boolean t_is_folder;
	FSRef t_current_fsref;
	
	t_os_status = FSPathMakeRef((const UInt8 *)".", &t_current_fsref, &t_is_folder);
	if (t_os_status != noErr || !t_is_folder)
		return false;

	// Create the iterator, pass kFSIterateFlat to iterate over the current subtree only
	FSIterator t_catalog_iterator;
	t_os_status = FSOpenIterator(&t_current_fsref, kFSIterateFlat, &t_catalog_iterator);
	if (t_os_status != noErr)
		return false;
	
	uint4 t_entry_count;
	t_entry_count = 0;
	
	if (!p_files)
	{
		t_entry_count++;
		/* UNCHECKED */ MCListAppendCString(*t_list, "..");
	}
	
	ItemCount t_max_objects, t_actual_objects;
	t_max_objects = CATALOG_MAX_ENTRIES;
	t_actual_objects = 0;
	FSCatalogInfo t_catalog_infos[CATALOG_MAX_ENTRIES];
	HFSUniStr255 t_names[CATALOG_MAX_ENTRIES];
	
	FSCatalogInfoBitmap t_info_bitmap;
	t_info_bitmap = kFSCatInfoAllDates |
					kFSCatInfoPermissions |
					kFSCatInfoUserAccess |
					kFSCatInfoFinderInfo | 
					kFSCatInfoDataSizes |
					kFSCatInfoRsrcSizes |
					kFSCatInfoNodeFlags;

	MCExecPoint t_tmp_context(NULL, NULL, NULL);	
	OSErr t_oserror;
	do
	{
		t_oserror = FSGetCatalogInfoBulk(t_catalog_iterator, t_max_objects, &t_actual_objects, NULL, t_info_bitmap, t_catalog_infos, NULL, NULL, t_names);
		if (t_oserror != noErr && t_oserror != errFSNoMoreItems)
		{	// clean up and exit
			FSCloseIterator(t_catalog_iterator);
			return false;
		}
		
		for(uint4 t_i = 0; t_i < (uint4)t_actual_objects; t_i++)
		{
			// folders
			UInt16 t_is_folder;
			t_is_folder = t_catalog_infos[t_i] . nodeFlags & kFSNodeIsDirectoryMask;
			if ( (!p_files && t_is_folder) || (p_files && !t_is_folder))
			{
				char t_native_name[256];
				uint4 t_native_length;
				t_native_length = 256;
				MCS_utf16tonative((const unsigned short *)t_names[t_i] . unicode, t_names[t_i] . length, t_native_name, t_native_length);
				
				// MW-2008-02-27: [[ Bug 5920 ]] Make sure we convert Finder to POSIX style paths
				for(uint4 i = 0; i < t_native_length; ++i)
					if (t_native_name[i] == '/')
						t_native_name[i] = ':';
				
				char t_buffer[512];
				if (p_detailed)
				{ // the detailed|long files
					FSPermissionInfo *t_permissions;
					t_permissions = (FSPermissionInfo *)&(t_catalog_infos[t_i] . permissions);
				
					t_tmp_context . copysvalue(t_native_name, t_native_length);
					MCU_urlencode(t_tmp_context);
				
					char t_filetype[9];
					if (!t_is_folder)
					{
						FileInfo *t_file_info;
						t_file_info = (FileInfo *) &t_catalog_infos[t_i] . finderInfo;
						uint4 t_creator;
						t_creator = MCSwapInt32NetworkToHost(t_file_info -> fileCreator);
						uint4 t_type;
						t_type = MCSwapInt32NetworkToHost(t_file_info -> fileType);
						
						if (t_file_info != NULL)
						{
							memcpy(t_filetype, (char*)&t_creator, 4);
							memcpy(&t_filetype[4], (char *)&t_type, 4);
							t_filetype[8] = '\0';
						}
						else
							t_filetype[0] = '\0';
					} else
						strcpy(t_filetype, "????????"); // this is what the "old" getentries did	

					CFAbsoluteTime t_creation_time;
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . createDate, &t_creation_time);
					t_creation_time += kCFAbsoluteTimeIntervalSince1970;

					CFAbsoluteTime t_modification_time;
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . contentModDate, &t_modification_time);
					t_modification_time += kCFAbsoluteTimeIntervalSince1970;

					CFAbsoluteTime t_access_time;
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . accessDate, &t_access_time);
					t_access_time += kCFAbsoluteTimeIntervalSince1970;

					CFAbsoluteTime t_backup_time;
					if (t_catalog_infos[t_i] . backupDate . highSeconds == 0 && t_catalog_infos[t_i] . backupDate . lowSeconds == 0 && t_catalog_infos[t_i] . backupDate . fraction == 0)
						t_backup_time = 0;
					else
					{
						UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . backupDate, &t_backup_time);
						t_backup_time += kCFAbsoluteTimeIntervalSince1970;
					}

					sprintf(t_buffer, "%*.*s,%llu,%llu,%.0lf,%.0lf,%.0lf,%.0lf,%d,%d,%03o,%.8s",
						t_tmp_context . getsvalue() . getlength(),  
					    t_tmp_context . getsvalue() . getlength(),  
					   	t_tmp_context . getsvalue() . getstring(),
						t_catalog_infos[t_i] . dataLogicalSize,
						t_catalog_infos[t_i] . rsrcLogicalSize,
						t_creation_time,
						t_modification_time,
						t_access_time,
						t_backup_time,
						t_permissions -> userID,
						t_permissions -> groupID,
						t_permissions -> mode & 0777,
						t_filetype);
						
					/* UNCHECKED */ MCListAppendCString(*t_list, t_buffer);
				}
				else
					/* UNCHECKED */ MCListAppendNativeChars(*t_list, (const char_t *)t_native_name, t_native_length);
					
				t_entry_count += 1;		
			}
		}	
	} while(t_oserror != errFSNoMoreItems);
	
	FSCloseIterator(t_catalog_iterator);
	return MCListCopy(*t_list, r_list);
}

bool MCS_longfilepath(MCStringRef p_path, MCStringRef& r_long_path)
{
	return MCStringCopy(p_path, r_long_path);
}

bool MCS_shortfilepath(MCStringRef p_path, MCStringRef& r_short_path)
{
	return MCStringCopy(p_path, r_short_path);
}

bool MCS_is_link(MCStringRef p_path)
{
	struct stat buf;
	return (lstat(MCStringGetCString(p_path), &buf) == 0 && S_ISLNK(buf.st_mode));
}

bool MCS_readlink(MCStringRef p_path, MCStringRef& r_link)
{
	struct stat t_stat;
	ssize_t t_size;
	MCAutoNativeCharArray t_buffer;
    
	if (lstat(MCStringGetCString(p_path), &t_stat) == -1 ||
		!t_buffer.New(t_stat.st_size))
		return false;
    
	t_size = readlink(MCStringGetCString(p_path), (char*)t_buffer.Chars(), t_stat.st_size);
    
	return (t_size == t_stat.st_size) && t_buffer.CreateStringAndRelease(r_link);
}

bool MCS_resolvepath(MCStringRef p_path, MCStringRef& r_resolved)
{
	if (MCStringGetLength(p_path) == 0)
		return MCS_getcurdir(r_resolved);

	MCAutoStringRef t_tilde_path;
	if (MCStringGetCharAtIndex(p_path, 0) == '~')
	{
		uindex_t t_user_end;
		if (!MCStringFirstIndexOfChar(p_path, '/', 0, kMCStringOptionCompareExact, t_user_end))
			t_user_end = MCStringGetLength(p_path);
		
		// Prepend user name
		struct passwd *t_password;
		if (t_user_end == 1)
			t_password = getpwuid(getuid());
		else
		{
			MCAutoStringRef t_username;
			if (!MCStringCopySubstring(p_path, MCRangeMake(1, t_user_end - 1), &t_username))
				return false;

			t_password = getpwnam(MCStringGetCString(*t_username));
		}
		
		if (t_password != NULL)
		{
			if (!MCStringCreateMutable(0, &t_tilde_path) ||
				!MCStringAppendNativeChars(*t_tilde_path, (char_t*)t_password->pw_dir, MCCStringLength(t_password->pw_dir)) ||
				!MCStringAppendSubstring(*t_tilde_path, p_path, MCRangeMake(t_user_end, MCStringGetLength(p_path) - t_user_end)))
				return false;
		}
		else
			t_tilde_path = p_path;
	}
	else
		t_tilde_path = p_path;

	MCAutoStringRef t_fullpath;
	if (MCStringGetCharAtIndex(*t_tilde_path, 0) != '/')
	{
		MCAutoStringRef t_folder;
		if (!MCS_getcurdir(&t_folder))
			return false;

		MCAutoStringRef t_resolved;
		if (!MCStringMutableCopy(*t_folder, &t_fullpath) ||
			!MCStringAppendChar(*t_fullpath, '/') ||
			!MCStringAppend(*t_fullpath, *t_tilde_path))
			return false;
	}
	else
		t_fullpath = *t_tilde_path;

	if (!MCS_is_link(*t_fullpath))
		return MCStringCopy(*t_fullpath, r_resolved);

	MCAutoStringRef t_newname;
	if (!MCS_readlink(*t_fullpath, &t_newname))
		return false;

	// IM - Should we really be using the original p_path parameter here?
	// seems like we should use the computed t_fullpath value.
	if (MCStringGetCharAtIndex(*t_newname, 0) != '/')
	{
		MCAutoStringRef t_resolved;

		uindex_t t_last_component;
		uindex_t t_path_length;

		if (MCStringLastIndexOfChar(p_path, '/', MCStringGetLength(p_path), kMCStringOptionCompareExact, t_last_component))
			t_last_component++;
		else
			t_last_component = 0;

		if (!MCStringMutableCopySubstring(p_path, MCRangeMake(0, t_last_component), &t_resolved) ||
			!MCStringAppend(*t_resolved, *t_newname))
			return false;

		return MCStringCopy(*t_resolved, r_resolved);
	}
	else
		return MCStringCopy(*t_newname, r_resolved);
}

Boolean MCS_rename(const char *oname, const char *nname)
{ //rename a file or directory

	char *oldpath = path2utf(MCS_resolvepath(oname));
	char *newpath = path2utf(MCS_resolvepath(nname));
	Boolean done = rename(oldpath, newpath) == 0;

	delete oldpath;
	delete newpath;
	return done;
}

bool MCS_getdrives(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	OSErr t_err;
	ItemCount t_index;
	bool t_first;
	
	t_index = 1;
	t_err = noErr;
	t_first = true;
	
	// To list all the mounted volumes on the system we use the FSGetVolumeInfo
	// API with first parameter kFSInvalidVolumeRefNum and an index in the
	// second parameter.
	// This call will return nsvErr when it reaches the end of the list of
	// volumes, other errors being returned if there's a problem getting the
	// information.
	// Due to this, it is perfectly possible that the first index will not be
	// the first volume we put into the list - so we need a boolean flag (t_first)
	while(t_err != nsvErr)
	{
		HFSUniStr255 t_unicode_name;
		t_err = FSGetVolumeInfo(kFSInvalidVolumeRefNum, t_index, NULL, kFSVolInfoNone, NULL, &t_unicode_name, NULL);
		if (t_err == noErr)
		{
			MCAutoStringRef t_volume_name;
			if (!MCStringCreateWithChars(t_unicode_name . unicode, t_unicode_name . length, &t_volume_name))
				return false;
			if (!MCListAppend(*t_list, *t_volume_name))
				return false;
		}
		t_index += 1;
	}
	
	return MCListCopy(*t_list, r_list);
}
