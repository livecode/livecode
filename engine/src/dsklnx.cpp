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

#include "lnxprefix.h"

#include "osspec.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "system.h"
#include "foundation.h"

#include "object.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"
//#include "execpt.h"
//#include "param.h"
#include "handler.h"
#include "util.h"
#include "globals.h"
//#include "ports.cpp"
#include "socket.h"
//#include "mcssl.h"
//#include "securemode.h"
#include "mode.h"
#include "player.h"
#include "text.h"
#include "variable.h"

#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/dir.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <dlfcn.h>
#include <termios.h>
//#include <langinfo.h>
#include <locale.h>
#include <pwd.h>
#include <fcntl.h>

// SN-2014-12-17: [[ Bug 14220 ]] Server should not wait put rather poll
#include <poll.h>

#include <libgnome/gnome-url.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnome-init.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>


////////////////////////////////////////////////////////////////////////////////

// This is in here so we do not need GLIBC2.4
extern "C" void __attribute__ ((noreturn)) __stack_chk_fail (void)
{
}

////////////////////////////////////////////////////////////////////////////////

// MW-2005-02-22: Make global for now so opensslsocket.cpp can access it
real8 curtime;

static Boolean alarmpending;

// MW-2013-10-08: [[ Bug 11259 ]] We use our own tables on linux since
//   we use a fixed locale which isn't available on all systems.
#if !defined(_LINUX_SERVER) && !defined(_LINUX_DESKTOP)
uint1 *MClowercasingtable = NULL;
uint1 *MCuppercasingtable = NULL;
#endif

//

////////////////////////////////////////////////////////////////////////////////

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
            else if (baudrate == 19200)
                baud = B19200;
            else if (baudrate == 9600)
                baud = B9600;

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
        MCLog("Error getting terminous attributes", nil);
    }
    cfsetispeed(&theTermios,  B9600);
    theTermios.c_cflag = CS8;

    MCAutoStringRefAsSysString t_serial_settings;
    /* UNCHECKED */ t_serial_settings.Lock(MCserialcontrolsettings);
    char *controlptr = strclone(*t_serial_settings);
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
        MCLog("Error setting terminous attributes", nil);
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////

static IO_stat MCS_lnx_shellread(int fd, char *&buffer, uint4 &buffersize, uint4 &size)
{
    MCshellfd = fd;
    size = 0;
    while (True)
    {
        int readsize = 0;
        ioctl(fd, FIONREAD, (char *)&readsize);
        readsize += READ_PIPE_SIZE;
        if (size + readsize > buffersize)
        {
            MCU_realloc((char **)&buffer, buffersize,
                        buffersize + readsize + 1, sizeof(char));
            buffersize += readsize;
        }
        errno = 0;
        int4 amount = read(fd, &buffer[size], readsize);
        if (amount <= 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                break;
            MCU_play();

            // SN-2014-12-17: [[ Bug 14220 ]] The server wasn't waiting pre-7.0
#ifdef _SERVER
            pollfd t_poll_fd;
            t_poll_fd . fd = fd;
            t_poll_fd . events = POLLIN;
            t_poll_fd . revents = 0;

            // SN-2015-02-12: [[ Bug 14441 ]] poll might as well get signal interrupted
            //  and we don't want to miss the reading for that only reason.
            int t_result;
            do
            {
                t_result = poll(&t_poll_fd, 1, -1);
            }
            while (t_result != 1 ||
                   (errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK));

            // SN-2015-02-26: [[ CID 37859 ]] Dead code removed (t_result is
            //  always different from -1 here
#else
            if (MCscreen->wait(READ_INTERVAL, False, True))
            {
                MCshellfd = -1;
                return IO_ERROR;
            }
#endif
        }
        else
            size += amount;
    }
    MCshellfd = -1;
    return IO_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

static Boolean MCS_lnx_nodelay(int4 fd)
{
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & O_APPEND | O_NONBLOCK)
           >= 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_lnx_is_link(MCStringRef p_path)
{
#ifdef /* MCS_is_link_dsk_lnx */ LEGACY_SYSTEM_ORPHAN
    struct stat64 buf;
    return (lstat64(MCStringGetCString(p_path), &buf) == 0 && S_ISLNK(buf.st_mode));
#endif /* MCS_is_link_dsk_lnx */
    MCAutoStringRefAsSysString t_path;
    /* UNCHECKED */ t_path.Lock(p_path);
    struct stat64 buf;
    return (lstat64(*t_path, &buf) == 0 && S_ISLNK(buf.st_mode));
}

bool MCS_lnx_readlink(MCStringRef p_path, MCStringRef& r_link)
{
#ifdef /* MCS_readlink_dsk_lnx */ LEGACY_SYSTEM_ORPHAN
    struct stat64 t_stat;
    ssize_t t_size;
    MCAutoNativeCharArray t_buffer;

    if (lstat64(MCStringGetCString(p_path), &t_stat) == -1 ||
        !t_buffer.New(t_stat.st_size))
        return false;

    t_size = readlink(MCStringGetCString(p_path), (char*)t_buffer.Chars(), t_stat.st_size);

    return (t_size == t_stat.st_size) && t_buffer.CreateStringAndRelease(r_link);
#endif /* MCS_readlink_dsk_lnx */
    struct stat64 t_stat;
    ssize_t t_size;
    MCAutoArray<char> t_buffer;

    MCAutoStringRefAsSysString t_path;
    /* UNCHECKED */ t_path.Lock(p_path);
    if (lstat64(*t_path, &t_stat) == -1 ||
        // SN-2014-09-02: [[ Bug 13323 ]] The size needs be 1 bigger to allow
        // a final NIL-byte.
        !t_buffer.New(t_stat.st_size + 1))
        return false;

    t_size = readlink(*t_path, (char*)t_buffer.Ptr(), t_stat.st_size);

    if (t_size != t_stat.st_size)
        return false;

    return MCStringCreateWithSysString(t_buffer.Ptr(), r_link);
}

////////////////////////////////////////////////////////////////////////////////
// REFACTORED FROM lnxmisc.cpp

#include <iconv.h>

struct ConverterRecord
{
    ConverterRecord *next;
    const char *encoding;
    iconv_t converter;
};

static iconv_t fetch_converter(const char *p_encoding)
{
    static ConverterRecord *s_records = NULL;

    ConverterRecord *t_previous, *t_current;
    for(t_previous = NULL, t_current = s_records; t_current != NULL; t_previous = t_current, t_current = t_current -> next)
        if (t_current -> encoding == p_encoding)
            break;

    if (t_current == NULL)
    {
        iconv_t t_converter;
        t_converter = iconv_open("UTF-16", p_encoding);

        ConverterRecord *t_record;
        t_record = new ConverterRecord;
        t_record -> next = s_records;
        t_record -> encoding = p_encoding;
        t_record -> converter = t_converter;
        s_records = t_record;

        return t_record -> converter;
    }

    if (t_previous != NULL)
    {
        t_previous -> next = t_current -> next;
        t_current -> next = s_records;
        s_records = t_current;
    }

    return s_records -> converter;
}

////////////////////////////////////////////////////////////////////////////////

#define DNS_SCRIPT "repeat for each line l in url \"binfile:/etc/resolv.conf\";\
if word 1 of l is \"nameserver\" then put word 2 of l & cr after it; end repeat;\
delete last char of it; return it"

#define LAUNCH_URL_SCRIPT		"put \"%@\" into tFilename; " \
                                "put empty into tCmd; " \
                                "if tCmd is empty and shell(\"which xdg-open\") is not empty then; " \
                                "	put \"xdg-open\" into tCmd; "\
                                "end if; " \
                                "if tCmd is empty and $GNOME_DESKTOP_SESSION_ID is not empty and shell(\"which gnome-open\") is not empty then; " \
                                "	put \"gnome-open\" into tCmd; " \
                                "end if; " \
                                "if tCmd is empty and $KDE_FULL_SESSION is not empty then; " \
                                "	if shell(\"which kde-open\") is not empty then; " \
                                "		put \"kde-open\" into tCmd; " \
                                "	else if shell(\"which kfmclient\") is not empty then; " \
                                "		put \"kfmclient exec\" into tCmd; " \
                                "   end if; " \
                                "end if; " \
                                "if tCmd is not empty then; " \
                                "	launch tFilename with tCmd; " \
                                "else; " \
                                "	return \"not supported\"; " \
                                "end if;"

////////////////////////////////////////////////////////////////////////////////

#ifdef NOATON
int inet_aton(const char *cp, struct in_addr *inp)
{
    unsigned long rv = inet_addr(cp);
    //fprintf(stderr, "%d\n", rv);
    if (rv == -1)
        return False;
    memcpy(inp, &rv, sizeof(unsigned long));
    return True;
}
#endif

// TS-2008-03-03 : Make this global so that MCS_checkprocesses () can look at it
// and figure out if one of its children have already been waited on in the
// SIGCHLD handler.
pid_t waitedpid;

static void handle_signal(int sig)
{
    MCHandler handler(HT_MESSAGE);
    switch (sig)
    {
    case SIGUSR1:
        MCsiguser1++;
        break;
    case SIGUSR2:
        MCsiguser2++;
        break;
    case SIGTERM:
        switch (MCdefaultstackptr->getcard()->message(MCM_shut_down_request))
        {
        case ES_NORMAL:
            return;
        case ES_NOT_HANDLED:
            if (handler.getpass())
            {
                MCdefaultstackptr->getcard()->message(MCM_shut_down);
                MCquit = True; //set MC quit flag, to invoke quitting
                return;
            }
        default:
            break;
        }
    case SIGILL:
    case SIGBUS:
    case SIGSEGV:
    {
        MCAutoStringRefAsSysString t_cmd;
        /* UNCHECKED */ t_cmd.Lock(MCcmd);
        fprintf(stderr, "%s exiting on signal %d\n", *t_cmd, sig);
        MCS_killall();
        exit(-1);
    }
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGIOT:
            fprintf(stderr,"\n\nGot SIGIOT\n");
        if (MCnoui)
            exit(1);
        MCabortscript = True;
        break;
    case SIGFPE:
        errno = EDOM;
        break;
    case SIGCHLD:
        {
#if defined(_LINUX_DESKTOP)
            MCPlayer *tptr = MCplayers;
            // If we have some players waiting then deal with these first
            waitedpid = -1;
            if ( tptr != NULL)
            {
                waitedpid = wait(NULL);
                // Moving these two lines half fixes bug 5966 - however it still isn't quite right
                // as there will still be some interaction between a player and shell command
                while(tptr != NULL)
                {
                    if ( waitedpid == tptr -> getpid())
                    {
                        if (tptr->isdisposable())
                            tptr->playstop();
                        else
                            MCscreen->delaymessage(tptr, MCM_play_stopped, NULL, NULL);

                        tptr->shutdown();
                        break;
                    }
                    tptr = tptr -> getnextplayer() ;
                }
            }
            else
            {
                // Check to see if we have created a video window. If we have got to here it means
                // that we could not start mplayer -- so the child thread has exited, but the player
                // object has not had a chance to be created yet. TODO - investigate if there is a
                // cleaner way of dealing with this situation.
                if ( MClastvideowindow != DNULL )
                {
                    gdk_window_hide(MClastvideowindow);
                    gdk_window_destroy(MClastvideowindow);
                    MClastvideowindow = DNULL ;
                }
                else
                {
                    MCS_checkprocesses();
                }
            }
#endif /* LINUX_DESKTOP */
        }
        break;
    case SIGALRM:
        MCalarm = True;
#ifdef NOITIMERS

        alarm(1);
#endif

        break;
    case SIGPIPE:
    default:
        break;
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_generate_uuid(char p_buffer[128])
{
    typedef void (*uuid_generate_ptr)(unsigned char uuid[16]);
    typedef void (*uuid_unparse_ptr)(unsigned char uuid[16], char *out);
    static void *s_uuid_generate = NULL, *s_uuid_unparse = NULL;

    if (s_uuid_generate == NULL && s_uuid_unparse == NULL)
    {
        void *t_libuuid;
        t_libuuid = dlopen("libuuid.so", RTLD_LAZY);
        if (t_libuuid == NULL)
            t_libuuid = dlopen("libuuid.so.1", RTLD_LAZY);
        if (t_libuuid != NULL)
        {
            s_uuid_generate = dlsym(t_libuuid, "uuid_generate");
            s_uuid_unparse = dlsym(t_libuuid, "uuid_unparse");
}
    }

    if (s_uuid_generate != NULL && s_uuid_unparse != NULL)
    {
        unsigned char t_uuid[16];
        ((uuid_generate_ptr)s_uuid_generate)(t_uuid);
        ((uuid_unparse_ptr)s_uuid_unparse)(t_uuid, p_buffer);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

class MCLinuxDesktop: public MCSystemInterface
{
public:
    virtual MCServiceInterface *QueryService(MCServiceType type)
    {
        // No Linux-specific services
        return nil;
    }

    virtual bool Initialize(void)
    {
#ifdef /* MCS_init_dsk_lnx */ LEGACY_SYSTEM
        IO_stdin = new IO_header(stdin, NULL, 0, 0, 0);
        IO_stdout = new IO_header(stdout, NULL, 0, 0, 0);
        IO_stderr = new IO_header(stderr, NULL, 0, 0, 0);

        setlocale(LC_CTYPE, MCnullstring);
        setlocale(LC_COLLATE, MCnullstring);

        MCinfinity = HUGE_VAL;

        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = handle_signal;
        action.sa_flags = SA_RESTART;
        action.sa_flags |= SA_NOCLDSTOP;

        sigaction(SIGCHLD, &action, NULL);
        sigaction(SIGALRM, &action, NULL);

    #ifndef _DEBUG
        sigaction(SIGHUP, &action, NULL);
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGQUIT, &action, NULL);
        sigaction(SIGILL, &action, NULL);
        sigaction(SIGIOT, &action, NULL);
        sigaction(SIGFPE, &action, NULL);
        sigaction(SIGBUS, &action, NULL);
        sigaction(SIGSEGV, &action, NULL);
        sigaction(SIGPIPE, &action, NULL);
        sigaction(SIGTERM, &action, NULL);
        sigaction(SIGUSR1, &action, NULL);
        sigaction(SIGUSR2, &action, NULL);
    #ifndef LINUX
        sigaction(SIGSYS, &action, NULL);
    #endif
    #endif

        if (!MCS_isatty(0))
            MCS_nodelay(0);

        // MW-2013-10-01: [[ Bug 11160 ]] At the moment NBSP is not considered a space.
        MCctypetable[160] &= ~(1 << 4);

        MCshellcmd = strclone("/bin/sh");
#endif /* MCS_init_dsk_lnx */
        IO_stdin = MCsystem -> OpenFd(0, kMCOpenFileModeRead);
        IO_stdout = MCsystem -> OpenFd(1, kMCOpenFileModeWrite);
        IO_stderr = MCsystem -> OpenFd(2, kMCOpenFileModeWrite);

        // Internally, LiveCode assumes sorting orders etc are those of en_US.
        // Additionally, the "native" string encoding for Linux is ISO-8859-1
        // (even if the Linux system is using something different).
        const char *t_internal_locale = "en_US.ISO-8859-1";
        setlocale(LC_CTYPE, t_internal_locale);
        setlocale(LC_COLLATE, t_internal_locale);

        MCinfinity = HUGE_VAL;

        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = handle_signal;
        action.sa_flags = SA_RESTART;
        action.sa_flags |= SA_NOCLDSTOP;

        sigaction(SIGCHLD, &action, NULL);
        sigaction(SIGALRM, &action, NULL);

    #ifndef _DEBUG
        sigaction(SIGHUP, &action, NULL);
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGQUIT, &action, NULL);
        sigaction(SIGILL, &action, NULL);
        sigaction(SIGIOT, &action, NULL);
        sigaction(SIGFPE, &action, NULL);
        sigaction(SIGBUS, &action, NULL);
        sigaction(SIGSEGV, &action, NULL);
        sigaction(SIGPIPE, &action, NULL);
        sigaction(SIGTERM, &action, NULL);
        sigaction(SIGUSR1, &action, NULL);
        sigaction(SIGUSR2, &action, NULL);
    #ifndef _LINUX
        sigaction(SIGSYS, &action, NULL);
    #endif
    #endif

#ifndef _SERVER
		// ST-2015-04-20: [[ Bug 15257 ]] Stdin shouldn't be set to
		// non-blocking in server.
        if (!IsInteractiveConsole(0))
            MCS_lnx_nodelay(0);
#endif

        // MW-2013-10-01: [[ Bug 11160 ]] At the moment NBSP is not considered a space.
        MCctypetable[160] &= ~(1 << 4);

        MCValueAssign(MCshellcmd, MCSTR("/bin/sh"));
        return true;
    }

    virtual void Finalize(void)
    {
#ifdef /* MCS_shutdown_dsk_lnx */ LEGACY_SYSTEM
#endif /* MCS_shutdown_dsk_lnx */
    }

    virtual void Debug(MCStringRef p_string)
    {
        // TODO implement?
    }

    virtual real64_t GetCurrentTime(void)
    {
#ifdef /* MCS_time_dsk_lnx */ LEGACY_SYSTEM
        struct timezone tz;
        struct timeval tv;

        gettimeofday(&tv, &tz);
        curtime = tv.tv_sec + (real8)tv.tv_usec / 1000000.0;
        return curtime;
#endif /* MCS_time_dsk_lnx */
        struct timezone tz;
        struct timeval tv;

        gettimeofday(&tv, &tz);
        curtime = tv.tv_sec + (real8)tv.tv_usec / 1000000.0;
        return curtime;
    }

    virtual bool GetVersion(MCStringRef& r_string)
    {
#ifdef /* MCS_getsystemversion_dsk_lnx */ LEGACY_SYSTEM
        static char *buffer;
        uname(&u);
        if (buffer == NULL)
            buffer = new char[strlen(u.sysname) + strlen(u.release) + 2];
        sprintf(buffer, "%s %s", u.sysname, u.release);
        return buffer;
#endif /* MCS_getsystemversion_dsk_lnx */
        struct utsname u;
        uname(&u);
        MCAutoStringRef t_sysname, t_release;
        /* UNCHECKED */ MCStringCreateWithSysString(u.sysname, &t_sysname);
        /* UNCHECKED */ MCStringCreateWithSysString(u.release, &t_release);
        return MCStringFormat(r_string, "%@ %@", *t_sysname, *t_release);
    }

    virtual bool GetMachine(MCStringRef& r_string)
    {
#ifdef /* MCS_getmachine_dsk_lnx */ LEGACY_SYSTEM
        uname(&u);
        return u.machine;
#endif /* MCS_getmachine_dsk_lnx */
        struct utsname u;
        uname(&u);
        return MCStringCreateWithNativeChars((const char_t *)u.machine, MCCStringLength(u.machine), r_string);
    }

    virtual MCNameRef GetProcessor(void)
    {
#ifdef /* MCS_getprocessor_dsk_lnx */ LEGACY_SYSTEM
#ifdef __LP64__
        return "x86_64";
#else
        return "x86";
#endif
#endif /* MCS_getprocessor_dsk_lnx */
#if   defined(__X86_64__)
        return MCN_x86_64;
#elif defined(__ARM__)
        return MCN_arm;
#elif defined(__I386__)
        return MCN_x86;
#else
#  error "One of __X86_64__, __ARM__ or __I386__ must be defined"
#endif
    }

    virtual bool GetAddress(MCStringRef& r_address)
    {
#ifdef /* MCS_getaddress_dsk_lnx */ LEGACY_SYSTEM
        static char *buffer;
        uname(&u);
        if (buffer == NULL)
            buffer = new char[strlen(u.nodename) + strlen(MCcmd) + 4];
        sprintf(buffer, "%s:%s", u.nodename, MCcmd);
        return buffer;
#endif /* MCS_getaddress_dsk_lnx */
        struct utsname u;
        uname(&u);
        MCAutoStringRef t_nodename;
        /* UNCHECKED */ MCStringCreateWithSysString(u.nodename, &t_nodename);
        return MCStringFormat(r_address, "%@:%@", *t_nodename, MCcmd);
    }

    virtual uint32_t GetProcessId(void)
    {
#ifdef /* MCS_getpid_dsk_lnx */ LEGACY_SYSTEM
        return getpid();
#endif /* MCS_getpid_dsk_lnx */
        return getpid();
    }

    virtual void Alarm(real64_t p_when)
    {
#ifdef /* MCS_alarm_dsk_lnx */ LEGACY_SYSTEM
    if (!MCnoui)
    {
#ifdef NOITIMERS
        if (secs != 0.)
            alarm(secs + 1.0);
        else
            alarm(0);
#else

        static real8 oldsecs;
        if (secs != oldsecs)
        {
            itimerval val;
            val.it_interval.tv_sec = (long)secs;
            val.it_interval.tv_usec
            = (long)((secs - (double)(long)secs) * 1000000.0);
            val.it_value = val.it_interval;
            setitimer(ITIMER_REAL, &val, NULL);
            oldsecs = secs;
        }
#endif
        if (secs == 0.0)
            alarmpending = False;
        else
            alarmpending = True;
    }
#endif /* MCS_alarm_dsk_lnx */
        if (!MCnoui)
        {
#ifdef NOITIMERS
            if (p_when != 0.)
                alarm(p_when + 1.0);
            else
             alarm(0);
#else

            static real8 oldsecs;
            if (p_when != oldsecs)
            {
                itimerval val;
                val.it_interval.tv_sec = (long)p_when;
                val.it_interval.tv_usec
                    = (long)((p_when - (double)(long)p_when) * 1000000.0);
                val.it_value = val.it_interval;
                setitimer(ITIMER_REAL, &val, NULL);
                oldsecs = p_when;
            }
#endif
            if (p_when == 0.0)
                    alarmpending = False;
            else
                alarmpending = True;
        }
    }

    virtual void Sleep(real64_t p_when)
    {
#ifdef /* MCS_sleep_dsk_lnx */ LEGACY_SYSTEM
        Boolean wasalarm = alarmpending;
        if (alarmpending)
            MCS_alarm(0.0);

        struct timeval timeoutval;
        timeoutval.tv_sec = (long)duration;
        timeoutval.tv_usec = (long)((duration - floor(duration)) * 1000000.0);
        select(0, NULL, NULL, NULL, &timeoutval);

        if (wasalarm)
            MCS_alarm(CHECK_INTERVAL);
#endif /* MCS_sleep_dsk_lnx */
        Boolean wasalarm = alarmpending;
        if (alarmpending)
            MCS_alarm(0.0);

        struct timeval timeoutval;
        timeoutval.tv_sec = (long)p_when;
        timeoutval.tv_usec = (long)((p_when - floor(p_when)) * 1000000.0);
        select(0, NULL, NULL, NULL, &timeoutval);

        if (wasalarm)
            MCS_alarm(CHECK_INTERVAL);
    }

    virtual void SetEnv(MCStringRef p_name, MCStringRef p_value)
    {
#ifdef /* MCS_setenv_dsk_lnx */ LEGACY_SYSTEM
#ifdef NOSETENV
    char *dptr = new char[strlen(name) + strlen(value) + 2];
    sprintf(dptr, "%s=%s", name, value);
    putenv(dptr);
#else

    setenv(name, value, True);
#endif
#endif /* MCS_setenv_dsk_lnx */
#ifdef /* MCS_unsetenv_dsk_lnx */ LEGACY_SYSTEM
#ifndef NOSETENV
    unsetenv(name);
#endif
#endif /* MCS_unsetenv_dsk_lnx */
#ifdef NOSETENV
        char *dptr = new char[strlen(p_name) + strlen(p_value) + 2];
        sprintf(dptr, "%s=%s", p_name, p_value);
        putenv(dptr);
#else
        MCAutoStringRefAsSysString t_name, t_value;
        /* UNCHECKED */ t_name.Lock(p_name);
        if (p_value == nil)
            unsetenv(*t_name);
        else
        {
            /* UNCHECKED */ t_value.Lock(p_value);
            setenv(*t_name, *t_value, True);
        }
#endif
    }

    virtual bool GetEnv(MCStringRef p_name, MCStringRef& r_value)
    {
#ifdef /* MCS_getenv_dsk_lnx */ LEGACY_SYSTEM
        return getenv(name);
#endif /* MCS_getenv_dsk_lnx */
        MCAutoStringRefAsSysString t_name;
        /* UNCHECKED */ t_name.Lock(p_name);

        char *t_env_value = getenv(*t_name);
        // We need to know whethere we actually got an environment variable
        // or if it defaulted to an empty stringref because the env variable
        // was unset
        return t_env_value != nil && MCStringCreateWithSysString(t_env_value, r_value);
    }

    virtual Boolean CreateFolder(MCStringRef p_path)
    {
#ifdef /* MCS_mkdir_dsk_lnx */ LEGACY_SYSTEM
        char *newpath = MCS_resolvepath(path);
        Boolean done = mkdir(path, 0777) == 0;
        delete newpath;
        return done;
#endif /* MCS_mkdir_dsk_lnx */
        MCAutoStringRefAsSysString t_path;
        MCAutoStringRef t_resolved_path_string;
        if (MCS_resolvepath(p_path, &t_resolved_path_string))
        {
            /* UNCHECKED */ t_path.Lock(*t_resolved_path_string);
            return mkdir(*t_path, 0777) == 0;
        }

        return false;
    }

    virtual Boolean DeleteFolder(MCStringRef p_path)
    {
#ifdef /* MCS_rmdir_dsk_lnx */ LEGACY_SYSTEM
        char *newpath = MCS_resolvepath(path);
        Boolean done = rmdir(path) == 0;
        delete newpath;
        return done;
#endif /* MCS_rmdir_dsk_lnx */
        MCAutoStringRefAsSysString t_path;
        MCAutoStringRef t_resolved_path_string;
        if (MCS_resolvepath(p_path, &t_resolved_path_string))
        {
            /* UNCHECKED */ t_path.Lock(*t_resolved_path_string);
            return rmdir(*t_path) == 0;
        }

        return false;
    }

    virtual Boolean DeleteFile(MCStringRef p_path)
    {
#ifdef /* MCS_unlink_dsk_lnx */ LEGACY_SYSTEM
        char *newpath = MCS_resolvepath(path);
        Boolean done = unlink(newpath) == 0;
        delete newpath;
        return done;
#endif /* MCS_unlink_dsk_lnx */
        MCAutoStringRefAsSysString t_path;
        MCAutoStringRef t_resolved_path;
        MCS_resolvepath(p_path, &t_resolved_path);
        /* UNCHECKED */ t_path.Lock(*t_resolved_path);
        Boolean done = unlink(*t_path) == 0;
        return done;
    }

    virtual Boolean RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_rename_dsk_lnx */ LEGACY_SYSTEM
        char *oldpath = MCS_resolvepath(oname);
        char *newpath = MCS_resolvepath(nname);
#ifndef NORENAME

        Boolean done = rename(oldpath, newpath) == 0;
#else
        // doesn't work on directories
        Boolean done = True;
        if (link(oldpath, newpath) != 0)
            done = False;
        else
            if (unlink(oldpath) != 0)
            {
                unlink(newpath);
                done = False;
            }
#endif
        delete oldpath;
        delete newpath;
        return done;
#endif /* MCS_rename_dsk_lnx */
        MCAutoStringRef t_old_resolved_path, t_new_resolved_path;
        MCS_resolvepath(p_old_name, &t_old_resolved_path);
        MCS_resolvepath(p_new_name, &t_new_resolved_path);

        Boolean done;
    #ifndef NORENAME
        MCAutoStringRefAsSysString t_from, t_to;
        /* UNCHECKED */ t_from.Lock(*t_old_resolved_path);
        /* UNCHECKED */ t_to.Lock(*t_new_resolved_path);
        done = rename(*t_from, *t_to) == 0;
    #else
        // doesn't work on directories
        MCAutoStringRefAsSysString t_from, t_to;
        /* UNCHECKED */ t_from.Lock(*t_old_resovled_path);
        /* UNCHECKED */ t_to.Lock(*t_new_resolved_path);
        done = True;
        if (link(*t_from, *t_to) != 0)
            done = False;
        else
            if (unlink(*t_from) != 0)
            {
                unlink(*t_to);
                done = False;
            }
    #endif

        return done;
    }

    virtual Boolean BackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_backup_dsk_lnx */ LEGACY_SYSTEM
        return MCS_rename(oname, nname);
#endif /* MCS_backup_dsk_lnx */
        return MCS_rename(p_old_name, p_new_name);
    }

    virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_unbackup_dsk_lnx */ LEGACY_SYSTEM
    return MCS_rename(oname, nname);
#endif /* MCS_unbackup_dsk_lnx */
        return MCS_rename(p_old_name, p_new_name);
    }

    virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias)
    {
#ifdef /* MCS_createalias_dsk_lnx */ LEGACY_SYSTEM
        char *source = MCS_resolvepath(srcpath);
        char *dest = MCS_resolvepath(dstpath);
        Boolean done = symlink(source,dest) == 0;
        delete source;
        delete dest;
        return done;
#endif /* MCS_createalias_dsk_lnx */
        MCAutoStringRef t_target;
        MCAutoStringRef t_alias;
        /* UNCHECKED */ MCS_resolvepath(p_target, &t_target);
        /* UNCHECKED */ MCS_resolvepath(p_alias, &t_alias);
        MCAutoStringRefAsSysString t_target_sys, t_alias_sys;
        /* UNCHECKED */ t_target_sys.Lock(*t_target);
        /* UNCHECKED */ t_alias_sys.Lock(*t_alias);
        return symlink(*t_target_sys, *t_alias_sys) == 0;
    }

    // NOTE: 'ResolveAlias' returns a standard (not native) path.
    virtual Boolean ResolveAlias(MCStringRef p_target, MCStringRef& r_dest)
    {
#ifdef /* MCS_resolvealias_dsk_lnx */ LEGACY_SYSTEM
        char *tpath = ep.getsvalue().clone();
        char *dest = MCS_resolvepath(tpath);
        delete tpath;
        ep.copysvalue(dest, strlen(dest));
        delete dest;
#endif /* MCS_resolvealias_dsk_lnx */
        return MCS_resolvepath(p_target, r_dest);
    }

    virtual bool GetCurrentFolder(MCStringRef& r_path)
    {
#ifdef /* MCS_getcurdir_dsk_lnx */ LEGACY_SYSTEM
        char *dptr = new char[PATH_MAX + 2];
        getcwd(dptr, PATH_MAX);
        return dptr;
#endif /* MCS_getcurdir_dsk_lnx */
        MCAutoArray<char> t_buffer;
        if (!t_buffer.New(PATH_MAX + 1))
            return false;
        if (NULL == getcwd((char*)t_buffer.Ptr(), PATH_MAX + 1))
            return false;
        return MCStringCreateWithSysString(t_buffer.Ptr(), r_path);
    }

    ///* LEGACY */ char *GetCurrentFolder(void);
    virtual Boolean SetCurrentFolder(MCStringRef p_path)
    {
#ifdef /* MCS_setcurdir_dsk_lnx */ LEGACY_SYSTEM
        char *newpath = MCS_resolvepath(path);
        Boolean done = chdir(newpath) == 0;
        delete newpath;
        return done;
#endif /* MCS_setcurdir_dsk_lnx */
        MCAutoStringRef t_new_path;
        if (MCS_resolvepath(p_path, &t_new_path))
        {
            MCAutoStringRefAsSysString t_path_sys;
            /* UNCHECKED */ t_path_sys.Lock(*t_new_path);
            return chdir(*t_path_sys) == 0;
        }

        return false;
    }

    // NOTE: 'GetStandardFolder' returns a standard (not native) path.
    virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder)
    {
#ifdef /* MCS_getspecialfolder_dsk_lnx */ LEGACY_SYSTEM
        char *c_dir = MCS_resolvepath("~/");

        if ( ep.getsvalue() == "desktop" )
        {
            ep.clear();
            ep.appendcstring(c_dir);
            ep.appendcstring("/Desktop");
        }
        else if ( ep.getsvalue() == "home" )
            ep.setcstring(c_dir);
        else if (ep.getsvalue() == "temporary")
            ep.setcstring("/tmp");
        else
        {
            MCresult->sets("not supported");
            ep.clear();
        }

        delete c_dir ;
#endif /* MCS_getspecialfolder_dsk_lnx */
        MCAutoStringRef t_home, t_tilde;
        if (!MCStringCreateWithCString("~", &t_tilde) ||
            !MCS_resolvepath(*t_tilde, &t_home))
            return false;

        if (MCNameIsEqualTo(p_type, MCN_desktop, kMCCompareCaseless))
            return MCStringFormat(r_folder, "%@/Desktop", *t_home);
        else if (MCNameIsEqualTo(p_type, MCN_home, kMCCompareCaseless))
            return MCStringCopy(*t_home, r_folder);
        else if (MCNameIsEqualTo(p_type, MCN_temporary, kMCCompareCaseless))
            return MCStringCreateWithCString("/tmp", r_folder);
        // SN-2014-08-08: [[ Bug 13026 ]] Fix ported from 6.7
        else if (MCNameIsEqualTo(p_type, MCN_engine, kMCCompareCaseless)
                 // SN-2015-04-20: [[ Bug 14295 ]] If we are here, we are a standalone
                 // so the resources folder is the engine folder.
                 || MCNameIsEqualTo(p_type, MCN_resources, kMCCompareCaseless))
        {
            uindex_t t_last_slash;
            
            if (!MCStringLastIndexOfChar(MCcmd, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_slash))
                t_last_slash = MCStringGetLength(MCcmd);
            
            return MCStringCopySubstring(MCcmd, MCRangeMake(0, t_last_slash), r_folder) ? True : False;
        }

        r_folder = MCValueRetain(kMCEmptyString);
        return true;
    }

    virtual real8 GetFreeDiskSpace()
    {
#ifdef /* MCS_getfreediskspace_dsk_lnx */ LEGACY_SYSTEM
        return 1.0;
#endif /* MCS_getfreediskspace_dsk_lnx */

		/* GetFreeDiskSpace should return the number of bytes free on
		 * the current filesystem that contains the current working
		 * directory. */
		struct statvfs t_fsstat;

		if (-1 == statvfs (".", &t_fsstat))
		{
			return 0;
		}

		/* There are two ways to get a measure of free diskspace: one
		 * which measures how much free space there is, and one that
		 * measures how much free space is available to use.  Here, we
		 * choose the latter.  See also comments on bug 13674. */

		real8 t_space;
		t_space = t_fsstat.f_bavail;
		// t_space = (real8) t_fsstat.f_bfree;
		t_space *= t_fsstat.f_bsize;
		return t_space;
    }

    virtual Boolean GetDevices(MCStringRef& r_devices)
    {
#ifdef /* MCS_getdevices_dsk_lnx */ LEGACY_SYSTEM
        ep.clear();
        return True;
#endif /* MCS_getdevices_dsk_lnx */
        r_devices = MCValueRetain(kMCEmptyString);
        return true;
    }

    virtual Boolean GetDrives(MCStringRef& r_drives)
    {
#ifdef /* MCS_getdrives_dsk_lnx */ LEGACY_SYSTEM
        ep.clear();
        return True;
#endif /* MCS_getdrives_dsk_lnx */
        r_drives = MCValueRetain(kMCEmptyString);
        return true;
    }

    virtual Boolean FileExists(MCStringRef p_path)
    {
#ifdef /* MCS_exists_dsk_lnx */ LEGACY_SYSTEM
        // MM-2011-08-24: [[ Bug 9691 ]] Updated to use stat64 so no longer fails on files larger than 2GB
        char *newpath = MCS_resolvepath(path);
        struct stat64 buf;

        Boolean found = stat64(newpath, &buf) == 0;
        if (found)
            if (file)
            {
                if (buf.st_mode & S_IFDIR)
                    found = False;
            }
            else
                if (!(buf.st_mode & S_IFDIR))
                    found = False;
        delete newpath;
        return found;
#endif /* MCS_exists_dsk_lnx */
        if (MCStringGetLength(p_path) == 0)
            return false;

        MCAutoStringRefAsSysString t_path_sys;
        /* UNCHECKED */ t_path_sys.Lock(p_path);

        bool t_found;
        struct stat64 buf;
        // MM-2011-08-24: [[ Bug 9691 ]] Updated to use stat64 so no longer fails on files larger than 2GB
        t_found = stat64(*t_path_sys, &buf) == 0;

        if (t_found)
            t_found = ((buf.st_mode & S_IFMT) == S_IFREG);

        return t_found ? True : False;
    }

    virtual Boolean FolderExists(MCStringRef p_path)
    {
        if (MCStringGetLength(p_path) == 0)
            return false;

        MCAutoStringRef t_resolved;
        if (!MCS_resolvepath(p_path, &t_resolved))
            return false;

        MCAutoStringRefAsSysString t_path_sys;
        /* UNCHECKED */ t_path_sys.Lock(*t_resolved);

        bool t_found;
        struct stat64 buf;
        // MM-2011-08-24: [[ Bug 9691 ]] Updated to use stat64 so no longer fails on files larger than 2GB
        t_found = stat64(*t_path_sys, &buf) == 0;

        if (t_found)
            t_found = S_ISDIR(buf.st_mode);

        return t_found;
    }

    virtual Boolean FileNotAccessible(MCStringRef p_path)
    {
#ifdef /* MCS_noperm_dsk_lnx */ LEGACY_SYSTEM
        struct stat64 buf;
        if (stat64(path, &buf))
            return False;
        if (buf.st_mode & S_IFDIR)
            return True;
        if (!(buf.st_mode & S_IWUSR))
            return True;
        return False;
#endif /* MCS_noperm_dsk_lnx */
        MCAutoStringRefAsSysString t_path_sys;
        /* UNCHECKED */ t_path_sys.Lock(p_path);
        struct stat64 buf;
        if (stat64(*t_path_sys, &buf))
            return False;
        if (S_ISDIR(buf.st_mode))
            return True;
        if (!(buf.st_mode & S_IWUSR))
            return True;
        return False;
    }

    virtual Boolean ChangePermissions(MCStringRef p_path, uint2 p_mask)
    {
#ifdef /* MCS_chmod_dsk_lnx */ LEGACY_SYSTEM
        if (chmod(path, mask) != 0)
            return IO_ERROR;
        return IO_NORMAL;
#endif /* MCS_chmod_dsk_lnx */
        MCAutoStringRefAsSysString t_path_sys;
        /* UNCHECKED */ t_path_sys.Lock(p_path);
        if (chmod(*t_path_sys, p_mask) != 0)
            return IO_ERROR;
        return IO_NORMAL;
    }

    virtual uint2 UMask(uint2 p_mask)
    {
#ifdef /* MCS_umask_dsk_lnx */ LEGACY_SYSTEM
        return umask(mask);
#endif /* MCS_umask_dsk_lnx */
#ifdef /* MCS_getumask_dsk_lnx */ LEGACY_SYSTEM
        int4 oldmask = umask(0);
        umask(oldmask);
        return oldmask;
#endif /* MCS_getumask_dsk_lnx */
#ifdef /* MCS_setumask_dsk_lnx */ LEGACY_SYSTEM
        umask(newmask);
#endif /* MCS_setumask_dsk_lnx */

        return umask(p_mask);
    }

    virtual IO_handle DeployOpen(MCStringRef p_path, intenum_t p_mode)
    {
        if (p_mode != kMCOpenFileModeCreate)
            return OpenFile(p_path, p_mode, False);

        FILE *fptr;
        IO_handle t_handle;
        t_handle = NULL;

        MCAutoStringRefAsUTF8String t_path_utf;
        if (!t_path_utf.Lock(p_path))
            return NULL;

        fptr = fopen(*t_path_utf, "wb+");

        if (fptr != nil)
            t_handle = new MCStdioFileHandle(fptr);

        return t_handle;
    }

    virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map)
    {
#ifdef /* MCS_open_dsk_lnx */ LEGACY_SYSTEM
        char *newpath = MCS_resolvepath(path);
        IO_handle handle = NULL;

#ifndef NOMMAP
        if (map && MCmmap && !driver && strequal(mode, IO_READ_MODE))
        {
            int fd = open(newpath, O_RDONLY);
            struct stat64 buf;
            if (fd != -1 && !fstat64(fd, &buf))
            {
                uint4 len = buf.st_size - offset;
                if (len != 0)
                {
                    char *buffer = (char *)mmap(NULL, len, PROT_READ, MAP_SHARED,
                                                fd, offset);

                    // MW-2013-05-02: [[ x64 ]] Make sure we use the MAP_FAILED constant
                    //   rather than '-1'.
                    if (buffer != MAP_FAILED)
                    {
                        delete newpath;
                        handle = new IO_header(NULL, buffer, len, fd, 0);
                        return handle;
                    }
                }
                close(fd);
            }
        }
#endif
        FILE *fptr = fopen(newpath, mode);
        if (fptr == NULL && !strequal(mode, IO_READ_MODE))
            fptr = fopen(newpath, IO_CREATE_MODE);
        if (driver)
            configureSerialPort((short)fileno(fptr));
        delete newpath;
        if (fptr != NULL)
        {
            handle = new IO_header(fptr, NULL, 0, 0, 0);
            if (offset > 0)
                fseek(handle->fptr, offset, SEEK_SET);
        }
        return handle;
#endif /* MCS_open_dsk_lnx */
        IO_handle t_handle;
        t_handle = NULL;

        MCAutoStringRefAsSysString t_path_sys;
        /* UNCHECKED */ t_path_sys.Lock(p_path);

        if (p_map && MCmmap && p_mode == kMCOpenFileModeRead)
        {
            int t_fd = open(*t_path_sys, O_RDONLY);
            struct stat64 t_buf;
            if (t_fd != -1 && !fstat64(t_fd, &t_buf))
            {
                off_t t_len = t_buf.st_size;
                if (t_len != 0)
                {
                    char *t_buffer = (char *)mmap(NULL, t_len, PROT_READ, MAP_SHARED,
                                                t_fd, 0);
                    // MW-2013-05-02: [[ x64 ]] Make sure we use the MAP_FAILED constant
                    //   rather than '-1'.
                    if (t_buffer != MAP_FAILED)
                    {
                        t_handle = new MCMemoryMappedFileHandle(t_fd, t_buffer, t_len);
                        return t_handle;
                    }
                }
                close(t_fd);
            }
        }

        FILE *t_fptr;
        const char *t_mode;
        if (p_mode == kMCOpenFileModeRead)
            t_mode = IO_READ_MODE;
        else if (p_mode == kMCOpenFileModeWrite)
            t_mode = IO_WRITE_MODE;
        else if (p_mode == kMCOpenFileModeUpdate)
            t_mode = IO_UPDATE_MODE;
        else if (p_mode == kMCOpenFileModeAppend)
            t_mode = IO_APPEND_MODE;

        t_fptr = fopen(*t_path_sys, t_mode);

        if (t_fptr == NULL && p_mode != kMCOpenFileModeRead)
            t_fptr = fopen(*t_path_sys, IO_CREATE_MODE);

        if (t_fptr != NULL)
        {
            t_handle = new MCStdioFileHandle(t_fptr);
        }

        return t_handle;
    }

    virtual IO_handle OpenFd(uint32_t p_fd, intenum_t p_mode)
    {
       IO_handle t_handle = NULL;
        FILE *t_fptr = NULL;

        switch (p_mode)
        {
        case kMCOpenFileModeRead:
            t_fptr = fdopen(p_fd, IO_READ_MODE);
            break;
        case kMCOpenFileModeWrite:
            t_fptr = fdopen(p_fd, IO_WRITE_MODE);
            break;
        case kMCOpenFileModeUpdate:
            t_fptr = fdopen(p_fd, IO_UPDATE_MODE);
            break;
        case kMCOpenFileModeAppend:
            t_fptr = fdopen(p_fd, IO_APPEND_MODE);
            break;
        }

        // SN-2015-02-11: [[ Bug 14587 ]] Do not buffer if the
        //  targetted fd is a TTY
        //  see srvposix.cpp, former MCStdioFileHandle::OpenFd
        if (t_fptr && isatty(p_fd))
            setbuf(t_fptr, NULL);

        if (t_fptr != NULL)
            t_handle = new MCStdioFileHandle(t_fptr);

        return t_handle;
    }

    virtual IO_handle OpenDevice(MCStringRef p_path, intenum_t p_mode)
    {
        IO_handle t_handle = NULL;
        FILE *t_fptr = NULL;

        MCAutoStringRefAsSysString t_path_sys;
        /* UNCHECKED */ t_path_sys.Lock(p_path);

        if (p_mode == kMCOpenFileModeRead)
            t_fptr = fopen(*t_path_sys, IO_READ_MODE);
        else if (p_mode == kMCOpenFileModeWrite)
            t_fptr = fopen(*t_path_sys, IO_WRITE_MODE);
        else if (p_mode == kMCOpenFileModeUpdate)
            t_fptr = fopen(*t_path_sys, IO_UPDATE_MODE);
        else if (p_mode == kMCOpenFileModeAppend)
            t_fptr = fopen(*t_path_sys, IO_APPEND_MODE);

        if (t_fptr == NULL && p_mode != kMCOpenFileModeRead)
            t_fptr = fopen(*t_path_sys, IO_CREATE_MODE);

        configureSerialPort((short)fileno(t_fptr));

        if (t_fptr != NULL)
            t_handle = new MCStdioFileHandle(t_fptr);

        return t_handle;
    }

    // NOTE: 'GetTemporaryFileName' returns a standard (not native) path.
    virtual bool GetTemporaryFileName(MCStringRef& r_tmp_name)
    {
#ifdef /* MCS_tmpnam_dsk_lnx */ LEGACY_SYSTEM
    return tmpnam(NULL);
#endif /* MCS_tmpnam_dsk_lnx */
        return MCStringCreateWithSysString(tmpnam(NULL), r_tmp_name);
    }

    virtual MCSysModuleHandle LoadModule(MCStringRef p_path)
    {
#ifdef /* MCS_loadmodule_dsk_lnx */ LEGACY_SYSTEM
#ifdef _DEBUG
    // dlopen loads whole 4-byte words when accessing the filename. This causes valgrind to make
    // spurious noise - so in DEBUG mode we make sure we allocate a 4-byte aligned block of memory.
    //
    char *t_aligned_filename;
    t_aligned_filename = new char[(strlen(p_filename) + 4) & ~3];
    strcpy(t_aligned_filename, p_filename);

    MCSysModuleHandle t_result;
    t_result = (MCSysModuleHandle)dlopen(t_aligned_filename, (RTLD_NOW | RTLD_LOCAL));

    delete t_aligned_filename;
    return t_result ;
#else
    return ( (MCSysModuleHandle)dlopen ( p_filename , (RTLD_NOW | RTLD_LOCAL) ));
#endif
#endif /* MCS_loadmodule_dsk_lnx */

        // dlopen loads whole 4-byte words when accessing the filename. This causes valgrind to make
        // spurious noise - so in DEBUG mode we make sure we allocate a 4-byte aligned block of memory.
        //
        // Because converting to a sys string allocates memory, this alignment will always be satisfied.
        // (malloc/new always return alignment >= int/pointer alignment)
        MCAutoStringRefAsSysString t_filename_sys;
        /* UNCHECKED */ t_filename_sys.Lock(p_path);

        MCSysModuleHandle t_result;
        t_result = (MCSysModuleHandle)dlopen(*t_filename_sys, (RTLD_NOW | RTLD_LOCAL));

        return t_result ;
    }

    virtual MCSysModuleHandle ResolveModuleSymbol(MCSysModuleHandle p_module, MCStringRef p_symbol)
    {
#ifdef /* MCS_resolvemodulesymbol_dsk_lnx */ LEGACY_SYSTEM
        return ( dlsym ( p_module, p_symbol ) ) ;
#endif /* MCS_resolvemodulesymbol_dsk_lnx */
        MCAutoStringRefAsSysString t_symbol_sys;
        /* UNCHECKED */ t_symbol_sys.Lock(p_symbol);
        return (MCSysModuleHandle)(dlsym(p_module, *t_symbol_sys));
    }

    virtual void UnloadModule(MCSysModuleHandle p_module)
    {
#ifdef /* MCS_unloadmodule_dsk_lnx */ LEGACY_SYSTEM
        dlclose ( p_module ) ;
#endif /* MCS_unloadmodule_dsk_lnx */
        dlclose(p_module);
    }

    virtual bool ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *x_context)
    {
#ifdef /* MCS_getentries_dsk_lnx */ LEGACY_SYSTEM
        uint4 flag = files ? S_IFREG : S_IFDIR;
        DIR *dirptr;

        if ((dirptr = opendir(".")) == NULL)
        {
            *dptr = MCU_empty();
            return;
        }

        struct dirent64 *direntp;

        char *tptr = new char[ENTRIES_CHUNK];
        tptr[0] = '\0';
        uint4 nchunks = 1;
        uint4 size = 0;
        MCExecPoint ep(NULL, NULL, NULL);
        while ((direntp = readdir64(dirptr)) != NULL)
        {
            if (strequal(direntp->d_name, "."))
                continue;
            struct stat64 buf;
            stat64(direntp->d_name, &buf);
            if (buf.st_mode & flag)
            {
                char tbuf[PATH_MAX * 3 + U4L * 5 + 21];
                uint4 tsize;
                if (islong)
                {
                    ep.copysvalue(direntp->d_name, strlen(direntp->d_name));
                    MCU_urlencode(ep);
                    sprintf(tbuf, "%*.*s,%lld,,,%ld,%ld,,%d,%d,%03o,",
                            (int)ep.getsvalue().getlength(), (int)ep.getsvalue().getlength(),
                            ep.getsvalue().getstring(),buf.st_size, (long)buf.st_mtime,
                            (long)buf.st_atime, (int)buf.st_uid, (int)buf.st_gid,
                            (unsigned int)buf.st_mode & 0777);
                    tsize = strlen(tbuf) + 1;
                }
                else
                    tsize = strlen(direntp->d_name) + 1;
                if (size + tsize > nchunks * ENTRIES_CHUNK)
                {
                    MCU_realloc((char **)&tptr, nchunks * ENTRIES_CHUNK,
                                (nchunks + 1) * ENTRIES_CHUNK, sizeof(char));
                    nchunks++;
                }
                if (size)
                    tptr[size - 1] = '\n';
                if (islong)
                    strcpy(&tptr[size], tbuf);
                else
                    strcpy(&tptr[size], direntp->d_name);
                size += tsize;
            }
        }
        closedir(dirptr);
        *dptr = tptr;
#endif /* MCS_getentries_dsk_lnx */
        DIR *dirptr;

        if ((dirptr = opendir(".")) == NULL)
        {
            return false;
        }

        struct dirent64 *direntp;

        bool t_success = true;

        while (t_success && (direntp = readdir64(dirptr)) != NULL)
        {
            MCSystemFolderEntry p_entry;
            MCStringRef t_unicode_name;

            if (MCCStringEqual(direntp->d_name, "."))
                continue;
            struct stat buf;
            stat(direntp->d_name, &buf);

            if (direntp -> d_name != nil && MCStringCreateWithSysString(direntp -> d_name, t_unicode_name))
                p_entry.name = t_unicode_name;
            else
                p_entry.name = kMCEmptyString;

            p_entry.data_size = buf.st_size;
            p_entry.modification_time = (uint32_t)buf.st_mtime;
            p_entry.access_time = (uint32_t)buf.st_atime;
            p_entry.group_id = buf.st_uid;
            p_entry.user_id = buf.st_uid;
            p_entry.permissions = buf.st_mode & 0777;
            p_entry.is_folder = S_ISDIR(buf.st_mode);

            t_success = p_callback(x_context, &p_entry);

            MCValueRelease(t_unicode_name);
        }

        closedir(dirptr);

        return t_success;
    }
    
    // ST-2014-12-18: [[ Bug 14259 ]] Returns the executable from the system tools, not from argv[0]
    virtual bool GetExecutablePath(MCStringRef &r_executable)
    {
        char t_executable[PATH_MAX];
        uint32_t t_size;
        t_size = readlink("/proc/self/exe", t_executable, PATH_MAX);
        if (t_size == PATH_MAX)
            return false;
        
        t_executable[t_size] = 0;
        return MCStringCreateWithSysString(t_executable, r_executable);
    }

    virtual bool PathToNative(MCStringRef p_path, MCStringRef& r_native)
    {
        return MCStringCopy(p_path, r_native);
    }

    virtual bool PathFromNative(MCStringRef p_native, MCStringRef& r_path)
    {
        return MCStringCopy(p_native, r_path);
    }

    virtual bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path)
    {
#ifdef /* MCS_resolvepath_dsk_lnx */ LEGACY_SYSTEM
        if (path == NULL)
            return MCS_getcurdir();
        char *tildepath;
        if (path[0] == '~')
        {
            char *tpath = strclone(path);
            char *tptr = strchr(tpath, '/');
            if (tptr == NULL)
            {
                tpath[0] = '\0';
                tptr = tpath;
            }
            else
                *tptr++ = '\0';

            struct passwd *pw;
            if (*(tpath + 1) == '\0')
                pw = getpwuid(getuid());
            else
                pw = getpwnam(tpath + 1);
            if (pw == NULL)
                return NULL;
            tildepath = new char[strlen(pw->pw_dir) + strlen(tptr) + 2];
            strcpy(tildepath, pw->pw_dir);
            if (*tptr)
            {
                strcat(tildepath, "/");
                strcat(tildepath, tptr);
            }
            delete tpath;
        }
        else if (path[0] != '/')
        {
            // SN-2015-06-05: [[ Bug 15432 ]] Fix resolvepath on Linux: we want an
            //  absolute path.
            char *t_curfolder;
            t_curfolder = MCS_getcurdir();
            tildepath = new char[strlen(t_curfolder) + strlen(path) + 2];
            /* UNCHECKED */ sprintf(tildepath, "%s/%s", t_curfolder, path);

            delete t_curfolder;
        }
        else
            tildepath = strclone(path);

        struct stat64 buf;
        if (lstat64(tildepath, &buf) != 0 || !S_ISLNK(buf.st_mode))
            return tildepath;

        char *newname = new char[PATH_MAX + 2];

        // SN-2015-06-05: [[ Bug 15432 ]] Use realpath to solve the symlink.
        if (realpath(tildepath, newname) == NULL)
        {
            // Clear the memory in case of failure
            delete newname;
            newname = NULL;
        }

        delete tildepath;
        return newname;
#endif /* MCS_resolvepath_dsk_lnx */
        if (MCStringGetLength(p_path) == 0)
        {
            MCS_getcurdir(r_resolved_path);
            return true;
        }

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
                if (!MCStringCopySubstring(p_path, MCRangeMake(1, t_user_end), &t_username))
                    return false;

                MCAutoStringRefAsSysString t_username_sys;
                /* UNCHECKED */ t_username_sys.Lock(*t_username);

                t_password = getpwnam(*t_username_sys);
            }

            if (t_password != NULL)
            {
                MCAutoStringRef t_pw_dir;
                /* UNCHECKED */ MCStringCreateWithSysString(t_password->pw_dir, &t_pw_dir);

                if (!MCStringCreateMutable(0, &t_tilde_path) ||
                    !MCStringAppend(*t_tilde_path, *t_pw_dir) ||
                    !MCStringAppendSubstring(*t_tilde_path, p_path, MCRangeMake(t_user_end, MCStringGetLength(p_path) - t_user_end)))
                    return false;
            }
            else
                t_tilde_path = p_path;
        }
        else if (MCStringGetNativeCharAtIndex(p_path, 0) != '/')
        {
            // SN-2015-06-05: [[ Bug 15432 ]] Fix resolvepath on Linux: we want an
            //  absolute path.
            MCAutoStringRef t_curdir;
            MCS_getcurdir(&t_curdir);

            if (!MCStringFormat(&t_tilde_path, "%@/%@", *t_curdir, p_path))
            {
                return false;
            }
        }
        else
            t_tilde_path = p_path;

        // SN-2014-12-18: [[ Bug 14001 ]] Update the server file resolution to use realpath
        //  so that we get the absolute path (needed for MCcmd for instance).
        // SN-2015-06-08: Use realpath on desktop as well.
#ifndef _SERVER
        // IM-2012-07-23
        // Keep (somewhat odd) semantics of the original function for now
        if (!MCS_lnx_is_link(*t_tilde_path))
            return MCStringCopy(*t_tilde_path, r_resolved_path);
#endif
        MCAutoStringRefAsSysString t_tilde_path_sys;
        t_tilde_path_sys . Lock(*t_tilde_path);

        char *t_resolved_path;
        bool t_success;

        t_resolved_path = realpath(*t_tilde_path_sys, NULL);

        // If the does not exist, then realpath will fail: we want to
        // return something though, so we keep the input path (as it
        // is done for desktop).
        if (t_resolved_path != NULL)
            t_success = MCStringCreateWithSysString(t_resolved_path, r_resolved_path);
        else
            t_success = MCStringCopy(*t_tilde_path, r_resolved_path);

        MCMemoryDelete(t_resolved_path);

        return t_success;
    }

    virtual bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
    {
#ifdef /* MCS_longfilepath_dsk_lnx */ LEGACY_SYSTEM
#endif /* MCS_longfilepath_dsk_lnx */
        return MCStringCopy(p_path, r_long_path);
    }

    virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
    {
#ifdef /* MCS_shortfilepath_dsk_lnx */ LEGACY_SYSTEM
#endif /* MCS_shortfilepath_dsk_lnx */
        return MCStringCopy(p_path, r_short_path);
    }

    virtual bool Shell(MCStringRef p_filename, MCDataRef& r_data, int& r_retcode)
    {
#ifdef /* MCS_runcmd_dsk_lnx */ LEGACY_SYSTEM
        IO_cleanprocesses();
        int tochild[2];
        int toparent[2];
        int4 index = MCnprocesses;
        if (pipe(tochild) == 0)
        {
            if (pipe(toparent) == 0)
            {
                MCU_realloc((char **)&MCprocesses, MCnprocesses,
                            MCnprocesses + 1, sizeof(Streamnode));
                MCprocesses[MCnprocesses].name = strclone("shell");
                MCprocesses[MCnprocesses].mode = OM_NEITHER;
                MCprocesses[MCnprocesses].ohandle = NULL;
                MCprocesses[MCnprocesses].ihandle = NULL;
                if ((MCprocesses[MCnprocesses++].pid = fork()) == 0)
                {
                    close(tochild[1]);
                    close(0);
                    dup(tochild[0]);
                    close(tochild[0]);
                    close(toparent[0]);
                    close(1);
                    dup(toparent[1]);
                    close(2);
                    dup(toparent[1]);
                    close(toparent[1]);
                    execl(MCshellcmd, MCshellcmd, "-s", NULL);
                    _exit(-1);
                }
                MCS_checkprocesses();
                close(tochild[0]);
                write(tochild[1], ep.getsvalue().getstring(),
                      ep.getsvalue().getlength());
                write(tochild[1], "\n", 1);
                close(tochild[1]);
                close(toparent[1]);
                MCS_nodelay(toparent[0]);
                if (MCprocesses[index].pid == -1)
                {
                    if (MCprocesses[index].pid > 0)
                        MCS_kill(MCprocesses[index].pid, SIGKILL);

                    MCprocesses[index].pid = 0;
                    MCeerror->add
                    (EE_SHELL_BADCOMMAND, 0, 0, ep.getsvalue());
                    return IO_ERROR;
                }
            }
            else
            {
                close(tochild[0]);
                close(tochild[1]);
                MCeerror->add
                (EE_SHELL_BADCOMMAND, 0, 0, ep.getsvalue());
                return IO_ERROR;
            }
        }
        else
        {
            MCeerror->add
            (EE_SHELL_BADCOMMAND, 0, 0, ep.getsvalue());
            return IO_ERROR;
        }
        char *buffer = ep.getbuffer(0);
        uint4 buffersize = ep.getbuffersize();
        uint4 size = 0;
        if (MCS_shellread(toparent[0], buffer, buffersize, size) != IO_NORMAL)
        {
            MCeerror->add
            (EE_SHELL_ABORT, 0, 0);
            close(toparent[0]);
            if (MCprocesses[index].pid != 0)
                MCS_kill(MCprocesses[index].pid, SIGKILL);
            ep.setbuffer(buffer, buffersize);
            return IO_ERROR;
        }
        ep.setbuffer(buffer, buffersize);
        ep.setlength(size);
        close(toparent[0]);
        MCS_checkprocesses();
        if (MCprocesses[index].pid != 0)
        {
            uint2 count = SHELL_COUNT;
            while (count--)
            {
                if (MCscreen->wait(SHELL_INTERVAL, False, False))
                {
                    if (MCprocesses[index].pid != 0)
                        MCS_kill(MCprocesses[index].pid, SIGKILL);
                    return IO_ERROR;
                }
                if (MCprocesses[index].pid == 0)
                    break;
            }
            if (MCprocesses[index].pid != 0)
            {
                MCprocesses[index].retcode = -1;
                MCS_kill(MCprocesses[index].pid, SIGKILL);
            }
        }
        if (MCprocesses[index].retcode)
        {
            MCExecPoint ep2(ep);
            ep2.setint(MCprocesses[index].retcode);
            MCresult->store(ep2, False);
        }
        else
            MCresult->clear(False);
        return IO_NORMAL;
#endif /* MCS_runcmd_dsk_lnx */
        IO_cleanprocesses();
        int tochild[2];
        int toparent[2];
        int4 index = MCnprocesses;
        if (pipe(tochild) == 0)
        {
            if (pipe(toparent) == 0)
            {
                MCU_realloc((char **)&MCprocesses, MCnprocesses,
                            MCnprocesses + 1, sizeof(Streamnode));
                MCprocesses[MCnprocesses].name = (MCNameRef)MCValueRetain(MCM_shell);
                MCprocesses[MCnprocesses].mode = OM_NEITHER;
                MCprocesses[MCnprocesses].ohandle = NULL;
                MCprocesses[MCnprocesses].ihandle = NULL;
                if ((MCprocesses[MCnprocesses++].pid = fork()) == 0)
                {
                    close(tochild[1]);
                    close(0);
                    dup(tochild[0]);
                    close(tochild[0]);
                    close(toparent[0]);
                    close(1);
                    dup(toparent[1]);
                    close(2);
                    dup(toparent[1]);
                    close(toparent[1]);

                    MCAutoStringRefAsSysString t_shellcmd_sys;
                    /* UNCHECKED */ t_shellcmd_sys.Lock(MCshellcmd);

                    execl(*t_shellcmd_sys, *t_shellcmd_sys, "-s", NULL);
                    _exit(-1);
                }
                CheckProcesses();
                close(tochild[0]);

                MCAutoStringRefAsSysString t_filename_sys;
                /* UNCHECKED */ t_filename_sys.Lock(p_filename);

                write(tochild[1], *t_filename_sys, strlen(*t_filename_sys));
                write(tochild[1], "\n", 1);
                close(tochild[1]);
                close(toparent[1]);
                MCS_lnx_nodelay(toparent[0]);
                if (MCprocesses[index].pid == -1)
                {
                    if (MCprocesses[index].pid > 0)
                        Kill(MCprocesses[index].pid, SIGKILL);

                    MCprocesses[index].pid = 0;
                    MCeerror->add
                    (EE_SHELL_BADCOMMAND, 0, 0, p_filename);
                    // SN-2015-01-29: [[ Bug 14462 ]] Should return false, not true
                    return false;
                }
            }
            else
            {
                close(tochild[0]);
                close(tochild[1]);
                MCeerror->add
                (EE_SHELL_BADCOMMAND, 0, 0, p_filename);
                // SN-2015-01-29: [[ Bug 14462 ]] Should return false, not true
                return false;
            }
        }
        else
        {
            MCeerror->add
            (EE_SHELL_BADCOMMAND, 0, 0, p_filename);
            // SN-2015-01-29: [[ Bug 14462 ]] Should return false, not true
            return false;
        }
        char *buffer;
        uint4 buffersize;
        buffer = (char *)malloc(4096);
        buffersize = 4096;
        uint4 size = 0;
        if (MCS_lnx_shellread(toparent[0], buffer, buffersize, size) != IO_NORMAL)
        {
            MCeerror->add
            (EE_SHELL_ABORT, 0, 0);
            close(toparent[0]);
            if (MCprocesses[index].pid != 0)
                Kill(MCprocesses[index].pid, SIGKILL);

            return MCDataCreateWithBytesAndRelease((byte_t*)buffer, size, r_data);
        }

        if (!MCDataCreateWithBytesAndRelease((byte_t*)buffer, size, r_data))
            return false;

        close(toparent[0]);
        CheckProcesses();

        // SN-2015-02-09: [[ Bug 14441 ]] We want to avoid the waiting time
        //  that MCScreen->wait can bring, and which was avoided in
        //  MCPosixSystem::Shell
#ifdef _SERVER
        pid_t t_wait_result;
        int t_wait_stat;
        t_wait_result = waitpid(MCprocesses[index].pid, &t_wait_stat, WNOHANG);
        if (t_wait_result == 0)
        {
            Kill(MCprocesses[index].pid, SIGKILL);
            waitpid(MCprocesses[index].pid, &t_wait_stat, 0);
        }
        else
            t_wait_stat = 0;

        MCprocesses[index].retcode = WEXITSTATUS(t_wait_stat);
#else

        if (MCprocesses[index].pid != 0)
        {
            uint2 count = SHELL_COUNT;
            while (count--)
            {
                if (MCscreen->wait(SHELL_INTERVAL, False, False))
                {
                    if (MCprocesses[index].pid != 0)
                        Kill(MCprocesses[index].pid, SIGKILL);
                    // SN-2015-01-29: [[ Bug 14462 ]] Should return false, not true
                    return false;
                }
                if (MCprocesses[index].pid == 0)
                    break;
            }
            if (MCprocesses[index].pid != 0)
            {
                MCprocesses[index].retcode = -1;
                Kill(MCprocesses[index].pid, SIGKILL);
            }
        }
#endif

        r_retcode = MCprocesses[index].retcode;
        return true;
    }

    virtual uint32_t TextConvert(const void *p_string, uint32_t p_string_length, void *r_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset)
    {
#ifdef /* MCS_multibytetounicode_dsk_lnx */ LEGACY_SYSTEM
        if (p_capacity == 0)
            r_used = p_mblength * 2;
        else
        {
            uint4 i;
            for(i = 0; i < MCU_min(p_mblength, p_capacity / 2); ++i)
                ((uint2 *)p_buffer)[i] = (unsigned char)p_mbstring[i];
            r_used = i * 2;
        }
#endif /* MCS_multibytetounicode_dsk_lnx */
#ifdef /* MCS_unicodetomultibyte_dsk_lnx */ LEGACY_SYSTEM
        if (p_capacity == 0)
            r_used = p_uclength / 2;
        else
        {
            uint4 t_count;
            t_count = MCU_min(p_uclength / 2, p_capacity);
            for(uint4 i = 0; i < t_count; ++i)
                ((unsigned char *)p_buffer)[i] = (unsigned char)((uint2 *)p_ucstring)[i];
            r_used = t_count;
        }
#endif /* MCS_unicodetomultibyte_dsk_lnx */
        uint32_t t_used;
        if (p_to_charset == LCH_UNICODE)
        {
            if (p_buffer_length == 0)
                t_used = p_string_length * 2;
            else
            {
                uint32_t t_count;
                unsigned char *t_input_ptr;
                uint2 *t_output_ptr;
                t_input_ptr = (unsigned char*)p_string;
                t_output_ptr = (uint2*)r_buffer;

                t_count = MCU_min(p_string_length, p_buffer_length / 2);
                for(uint32_t i = 0; i < t_count; ++i)
                    t_output_ptr[i] = t_input_ptr[i];
                t_used = t_count * 2;
            }
        }
        else if (p_from_charset == LCH_UNICODE)
        {
            if (p_buffer_length == 0)
                t_used = p_string_length / 2;
            else
            {
                uint4 t_count;
                unsigned char *t_output_ptr;
                uint2 *t_input_ptr;
                t_input_ptr = (uint2*)p_string;
                t_output_ptr = (unsigned char*)r_buffer;

                t_count = MCU_min(p_string_length / 2, p_buffer_length);
                for(uint4 i = 0; i < t_count; ++i)
                    t_output_ptr[i] = (unsigned char)t_input_ptr[i];
                t_used = t_count;
            }
        }
        return t_used;
    }

    virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4& p_output_length, uint4& r_used)
    {
#ifdef /* MCSTextConvertToUnicode_dsk_lnx */ LEGACY_SYSTEM
        if (p_input_length == 0)
        {
            r_used = 0;
            return true;
        }

        if (p_output_length == 0)
        {
            r_used = p_input_length * 4;
            return false;
        }

        const char *t_encoding;
        t_encoding = NULL;

        if (p_input_encoding >= kMCTextEncodingWindowsNative)
        {
            struct { uint4 codepage; const char *encoding; } s_codepage_map[] =
            {
                {437, "CP437" },
                {850, "CP850" },
                {932, "CP932" },
                {949, "CP949" },
                {1361, "CP1361" },
                {936, "CP936" },
                {950, "CP950" },
                {1253, "WINDOWS-1253" },
                {1254, "WINDOWS-1254" },
                {1258, "WINDOWS-1258" },
                {1255, "WINDOWS-1255" },
                {1256, "WINDOWS-1256" },
                {1257, "WINDOWS-1257" },
                {1251, "WINDOWS-1251" },
                {874, "CP874" },
                {1250, "WINDOWS-1250" },
                {1252, "WINDOWS-1252" },
                {10000, "MACINTOSH" }
            };

            for(uint4 i = 0; i < sizeof(s_codepage_map) / sizeof(s_codepage_map[0]); ++i)
                if (s_codepage_map[i] . codepage == p_input_encoding - kMCTextEncodingWindowsNative)
                {
                    t_encoding = s_codepage_map[i] . encoding;
                    break;
                }

        }
        else if (p_input_encoding >= kMCTextEncodingMacNative)
            t_encoding = "MACINTOSH";

        iconv_t t_converter;
        t_converter = fetch_converter(t_encoding);

        if (t_converter == NULL)
        {
            r_used = 0;
            return true;
        }

        char *t_in_bytes;
        char *t_out_bytes;
        size_t t_in_bytes_left;
        size_t t_out_bytes_left;

        t_in_bytes = (char *)p_input;
        t_in_bytes_left = p_input_length;
        t_out_bytes = (char *)p_output;
        t_out_bytes_left = p_output_length;

        iconv(t_converter, NULL, NULL, &t_out_bytes, &t_out_bytes_left);

        if (iconv(t_converter, &t_in_bytes, &t_in_bytes_left, &t_out_bytes, &t_out_bytes_left) == (size_t)-1)
        {
            r_used = 4 * p_input_length;
            return false;
        }

        r_used = p_output_length - t_out_bytes_left;

        return true;
#endif /* MCSTextConvertToUnicode_dsk_lnx */
        if (p_input_length == 0)
        {
            r_used = 0;
            return true;
        }

        if (p_output_length == 0)
        {
            r_used = p_input_length * 4;
            return false;
        }

        const char *t_encoding;
        t_encoding = NULL;

        if (p_input_encoding >= kMCTextEncodingWindowsNative)
        {
            struct { uint4 codepage; const char *encoding; } s_codepage_map[] =
            {
                {437, "CP437" },
                {850, "CP850" },
                {932, "CP932" },
                {949, "CP949" },
                {1361, "CP1361" },
                {936, "CP936" },
                {950, "CP950" },
                {1253, "WINDOWS-1253" },
                {1254, "WINDOWS-1254" },
                {1258, "WINDOWS-1258" },
                {1255, "WINDOWS-1255" },
                {1256, "WINDOWS-1256" },
                {1257, "WINDOWS-1257" },
                {1251, "WINDOWS-1251" },
                {874, "CP874" },
                {1250, "WINDOWS-1250" },
                {1252, "WINDOWS-1252" },
                {10000, "MACINTOSH" }
            };

            for(uint4 i = 0; i < sizeof(s_codepage_map) / sizeof(s_codepage_map[0]); ++i)
                if (s_codepage_map[i] . codepage == p_input_encoding - kMCTextEncodingWindowsNative)
                {
                    t_encoding = s_codepage_map[i] . encoding;
                    break;
                }

        }
        else if (p_input_encoding >= kMCTextEncodingMacNative)
            t_encoding = "MACINTOSH";

        iconv_t t_converter;
        t_converter = fetch_converter(t_encoding);

        if (t_converter == NULL)
        {
            r_used = 0;
            return true;
        }

        char *t_in_bytes;
        char *t_out_bytes;
        size_t t_in_bytes_left;
        size_t t_out_bytes_left;

        t_in_bytes = (char *)p_input;
        t_in_bytes_left = p_input_length;
        t_out_bytes = (char *)p_output;
        t_out_bytes_left = p_output_length;

        iconv(t_converter, NULL, NULL, &t_out_bytes, &t_out_bytes_left);

        if (iconv(t_converter, &t_in_bytes, &t_in_bytes_left, &t_out_bytes, &t_out_bytes_left) == (size_t)-1)
        {
            r_used = 4 * p_input_length;
            return false;
        }

        r_used = p_output_length - t_out_bytes_left;

        return true;
    }

    virtual void CheckProcesses(void)
    {
#ifdef /* MCS_checkprocesses_dsk_lnx */ LEGACY_SYSTEM
        uint2 i;
        bool cleanPID = false ;

        int wstat;
        for (i = 0 ; i < MCnprocesses ; i++)
        {
            cleanPID = (MCprocesses[i].pid != 0 && MCprocesses[i].pid != -1 ) ;
            if ( waitedpid == -1  || ( waitedpid != -1 && MCprocesses[i].pid != waitedpid ))
                cleanPID = cleanPID && ( waitpid(MCprocesses[i].pid, &wstat, WNOHANG) > 0) ;


            if ( cleanPID )
            {
                if (MCprocesses[i].ihandle != NULL)
                    clearerr(MCprocesses[i].ihandle->fptr);
                MCprocesses[i].pid = 0;
                MCprocesses[i].retcode = WEXITSTATUS(wstat);
            }
        }

#endif /* MCS_checkprocesses_dsk_lnx */
        uint2 i;
        bool cleanPID = false;

        int wstat;
        for (i = 0 ; i < MCnprocesses ; i++)
        {
            cleanPID = (MCprocesses[i].pid != 0 && MCprocesses[i].pid != -1 ) ;
            if (waitedpid == -1  || (waitedpid != -1 && MCprocesses[i].pid != waitedpid))
                cleanPID = cleanPID && (waitpid(MCprocesses[i].pid, &wstat, WNOHANG) > 0) ;

            if (cleanPID)
            {
                if (MCprocesses[i].ihandle != NULL)
                    clearerr((FILE*)MCprocesses[i].ihandle->GetFilePointer());
                MCprocesses[i].pid = 0;
                MCprocesses[i].retcode = WEXITSTATUS(wstat);
            }
        }
    }

    virtual uint32_t GetSystemError(void)
    {
#ifdef /* MCS_getsyserror_dsk_lnx */ LEGACY_SYSTEM
        return errno;
#endif /* MCS_getsyserror_dsk_lnx */
        return errno;
    }

    virtual bool StartProcess(MCNameRef p_name, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated)
    {
#ifdef /* MCS_startprocess_dsk_lnx */ LEGACY_SYSTEM
        Boolean noerror = True;
        Boolean reading = mode == OM_READ || mode == OM_UPDATE;
        Boolean writing = mode == OM_APPEND || mode == OM_WRITE || mode == OM_UPDATE;
        uint2 index = MCnprocesses;
        MCU_realloc((char **)&MCprocesses, MCnprocesses, MCnprocesses + 1,
                    sizeof(Streamnode));
        MCprocesses[MCnprocesses].name = name;
        MCprocesses[MCnprocesses].mode = mode;
        MCprocesses[MCnprocesses].ihandle = NULL;
        MCprocesses[MCnprocesses].ohandle = NULL;

        if (!elevated)
        {
            int tochild[2];
            int toparent[2];
            if (reading)
                if (pipe(toparent) != 0)
                    noerror = False;
            if (noerror && writing)
                if (pipe(tochild) != 0)
                {
                    noerror = False;
                    if (reading)
                    {
                        close(toparent[0]);
                        close(toparent[1]);
                    }
                }
            if (noerror)
            {
                if ((MCprocesses[MCnprocesses++].pid = fork()) == 0)
                {
                    char **argv = NULL;
                    uint2 argc = 0;
                    if (doc == NULL || *doc == '\0')
                    {
                        char *sptr = name;
                        while (*sptr)
                        {
                            while (isspace(*sptr))
                                sptr++;
                            MCU_realloc((char **)&argv, argc, argc + 2, sizeof(char *));
                            if (*sptr == '"')
                            {
                                argv[argc++] = ++sptr;
                                while (*sptr && *sptr != '"')
                                    sptr++;
                            }
                            else
                            {
                                argv[argc++] = sptr;
                                while (*sptr && !isspace(*sptr))
                                    sptr++;
                            }
                            if (*sptr)
                                *sptr++ = '\0';
                        }
                    }
                    else
                    {
                        argv = new char *[3];
                        argv[0] = name;
                        argv[1] = doc;
                        argc = 2;
                    }
                    argv[argc] = NULL;
                    if (reading)
                    {
                        close(toparent[0]);
                        close(1);
                        dup(toparent[1]);
                        close(2);
                        dup(toparent[1]);
                        close(toparent[1]);
                    }
                    else
                    {
                        close(1);
                        close(2);
                    }
                    if (writing)
                    {
                        close(tochild[1]);
                        close(0);
                        dup(tochild[0]);
                        close(tochild[0]);
                    }
                    else
                        close(0);
                    execvp(name, argv);
                    _exit(-1);
                }
                MCS_checkprocesses();
                if (reading)
                {
                    close(toparent[1]);
                    MCS_nodelay(toparent[0]);
                    MCprocesses[index].ihandle = MCS_dopen(toparent[0], IO_READ_MODE);
                }
                if (writing)
                {
                    close(tochild[0]);
                    MCprocesses[index].ohandle = MCS_dopen(tochild[1], IO_WRITE_MODE);
                }
            }
        }
        else
        {
            extern bool MCSystemOpenElevatedProcess(const char *p_command, int32_t& r_pid, int32_t& r_input_fd, int32_t& r_output_fd);
            int32_t t_pid, t_input_fd, t_output_fd;
            if (MCSystemOpenElevatedProcess(name, t_pid, t_input_fd, t_output_fd))
            {
                MCprocesses[MCnprocesses++] . pid = t_pid;
                MCS_checkprocesses();
                if (reading)
                {
                    MCS_nodelay(t_input_fd);
                    MCprocesses[index] . ihandle = MCS_dopen(t_input_fd, IO_READ_MODE);
                }
                else
                    close(t_input_fd);

                if (writing)
                    MCprocesses[index] . ohandle = MCS_dopen(t_output_fd, IO_WRITE_MODE);
                else
                    close(t_output_fd);

                noerror = True;
            }
            else
                noerror = False;
        }
        delete doc;
        if (!noerror || MCprocesses[index].pid == -1)
        {
            if (noerror)
                MCprocesses[index].pid = 0;
            else
                delete name;
            MCresult->sets("not opened");
        }
        else
            MCresult->clear(False);
#endif /* MCS_startprocess_dsk_lnx */
        Boolean noerror = True;
        Boolean reading = p_mode == OM_READ || p_mode == OM_UPDATE;
        Boolean writing = p_mode == OM_APPEND || p_mode == OM_WRITE || p_mode == OM_UPDATE;
        uint2 index = MCnprocesses;
        MCU_realloc((char **)&MCprocesses, MCnprocesses, MCnprocesses + 1,
                    sizeof(Streamnode));
        MCprocesses[MCnprocesses].name = (MCNameRef)MCValueRetain(p_name);
        MCprocesses[MCnprocesses].mode = (Open_mode)p_mode;
        MCprocesses[MCnprocesses].ihandle = NULL;
        MCprocesses[MCnprocesses].ohandle = NULL;

        if (!p_elevated)
        {
            int tochild[2];
            int toparent[2];
            if (reading)
                if (pipe(toparent) != 0)
                    noerror = False;
            if (noerror && writing)
                if (pipe(tochild) != 0)
                {
                    noerror = False;
                    if (reading)
                    {
                        close(toparent[0]);
                        close(toparent[1]);
                    }
                }
            if (noerror)
            {
                if ((MCprocesses[MCnprocesses++].pid = fork()) == 0)
                {
                    MCAutoStringRefAsSysString t_name_sys;
                    // SN-2014-08-22: [[ Bug 12903 ]] t_doc_sys was only defined in the
                    //  bock it locks the document string.
                    MCAutoStringRefAsSysString t_doc_sys;
                    /* UNCHECKED */ t_name_sys.Lock(MCNameGetString(p_name));

                    char **argv = NULL;
                    char *t_name_copy = strclone(*t_name_sys);
                    uint2 argc = 0;
                    if (p_doc == NULL || MCStringGetNativeCharAtIndex(p_doc, 0) == '\0')
                    {
                        char *sptr = t_name_copy;
                        while (*sptr)
                        {
                            while (isspace(*sptr))
                                sptr++;
                            MCU_realloc((char **)&argv, argc, argc + 2, sizeof(char *));
                            if (*sptr == '"')
                            {
                                argv[argc++] = ++sptr;
                                while (*sptr && *sptr != '"')
                                    sptr++;
                            }
                            else
                            {
                                argv[argc++] = sptr;
                                while (*sptr && !isspace(*sptr))
                                    sptr++;
                            }
                            if (*sptr)
                                *sptr++ = '\0';
                        }
                    }
                    else
                    {
                        /* UNCHECKED */ t_doc_sys.Lock(p_doc);

                        argv = new char *[3];
                        argv[0] = t_name_copy;
                        argv[1] = (char *)*t_doc_sys;
                        argc = 2;
                    }
                    argv[argc] = NULL;
                    if (reading)
                    {
                        close(toparent[0]);
                        close(1);
                        dup(toparent[1]);
                        close(2);
                        dup(toparent[1]);
                        close(toparent[1]);
                    }
                    else
                    {
                        close(1);
                        close(2);
                    }
                    if (writing)
                    {
                        close(tochild[1]);
                        close(0);
                        dup(tochild[0]);
                        close(tochild[0]);
                    }
                    else
                        close(0);
                    execvp(argv[0], argv);
                    delete[] t_name_copy;
                    _exit(-1);
                }
                CheckProcesses();
                if (reading)
                {
                    close(toparent[1]);
                    MCS_lnx_nodelay(toparent[0]);
                    MCprocesses[index].ihandle = OpenFd(toparent[0], kMCOpenFileModeRead);
                }
                if (writing)
                {
                    close(tochild[0]);
                    MCprocesses[index].ohandle = OpenFd(tochild[1], kMCOpenFileModeWrite);
                }
            }
        }
        else
        {
            extern bool MCSystemOpenElevatedProcess(MCStringRef p_command, int32_t& r_pid, int32_t& r_input_fd, int32_t& r_output_fd);
            int32_t t_pid, t_input_fd, t_output_fd;
            if (MCSystemOpenElevatedProcess(MCNameGetString(p_name), t_pid, t_input_fd, t_output_fd))
            {
                MCprocesses[MCnprocesses++] . pid = t_pid;
                CheckProcesses();
                if (reading)
                {
                    MCS_lnx_nodelay(t_input_fd);
                    MCprocesses[index] . ihandle = OpenFd(t_input_fd, kMCOpenFileModeRead);
                }
                else
                    close(t_input_fd);

                if (writing)
                    MCprocesses[index] . ohandle = OpenFd(t_output_fd, kMCOpenFileModeWrite);
                else
                    close(t_output_fd);

                noerror = True;
            }
            else
                noerror = False;
        }
        if (!noerror || MCprocesses[index].pid == -1)
        {
            if (noerror)
                MCprocesses[index].pid = 0;

            return false;
        }
        else
            return true;
    }

    virtual void CloseProcess(uint2 p_index)
    {
#ifdef /* MCS_closeprocess_dsk_lnx */ LEGACY_SYSTEM
        if (MCprocesses[index].ihandle != NULL)
        {
            MCS_close(MCprocesses[index].ihandle);
            MCprocesses[index].ihandle = NULL;
        }
        if (MCprocesses[index].ohandle != NULL)
        {
            MCS_close(MCprocesses[index].ohandle);
            MCprocesses[index].ohandle = NULL;
        }
        MCprocesses[index].mode = OM_NEITHER;
#endif /* MCS_closeprocess_dsk_lnx */
        if (MCprocesses[p_index].ihandle != NULL)
        {
            MCS_close(MCprocesses[p_index].ihandle);
            MCprocesses[p_index].ihandle = NULL;
        }
        if (MCprocesses[p_index].ohandle != NULL)
        {
            MCS_close(MCprocesses[p_index].ohandle);
            MCprocesses[p_index].ohandle = NULL;
        }
        MCprocesses[p_index].mode = OM_NEITHER;
    }

    virtual void Kill(int4 p_pid, int4 p_sig)
    {
#ifdef /* MCS_kill_dsk_lnx */ LEGACY_SYSTEM
        kill(pid, sig);
#endif /* MCS_kill_dsk_lnx */
        kill(p_pid, p_sig);
    }

    virtual void KillAll(void)
    {
#ifdef /* MCS_killall_dsk_lnx */ LEGACY_SYSTEM
        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = (void (*)(int))SIG_IGN;

        sigaction(SIGCHLD, &action, NULL);
        while (MCnprocesses--)
        {
            delete MCprocesses[MCnprocesses].name;
            if (MCprocesses[MCnprocesses].pid != 0
                && (MCprocesses[MCnprocesses].ihandle != NULL
                    || MCprocesses[MCnprocesses].ohandle != NULL))
            {
                kill(MCprocesses[MCnprocesses].pid, SIGKILL);
                waitpid(MCprocesses[MCnprocesses].pid, NULL, 0);
            }
        }
#endif /* MCS_killall_dsk_lnx */
        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = (void (*)(int))SIG_IGN;

        sigaction(SIGCHLD, &action, NULL);
        while (MCnprocesses--)
        {
            MCNameDelete(MCprocesses[MCnprocesses] . name);
            MCprocesses[MCnprocesses] . name = nil;
            if (MCprocesses[MCnprocesses].pid != 0
                    && (MCprocesses[MCnprocesses].ihandle != NULL
                        || MCprocesses[MCnprocesses].ohandle != NULL))
            {
                kill(MCprocesses[MCnprocesses].pid, SIGKILL);
                waitpid(MCprocesses[MCnprocesses].pid, NULL, 0);
            }
        }
    }

    virtual Boolean Poll(real8 p_delay, int p_fd)
    {
#ifdef /* MCS_poll_dsk_lnx */ LEGACY_SYSTEM
        Boolean readinput = False;
        int4 n;
        uint2 i;
        Boolean wasalarm = alarmpending;
        Boolean handled = False;
        if (alarmpending)
            MCS_alarm(0.0);

        extern int g_notify_pipe[2];

        fd_set rmaskfd, wmaskfd, emaskfd;
        FD_ZERO(&rmaskfd);
        FD_ZERO(&wmaskfd);
        FD_ZERO(&emaskfd);
        int4 maxfd = 0;
        if (!MCnoui)
        {
            FD_SET(fd, &rmaskfd);
            maxfd = fd;
        }
        if (MCshellfd != -1)
        {
            FD_SET(MCshellfd, &rmaskfd);
            if (MCshellfd > maxfd)
                maxfd = MCshellfd;
        }
        if (MCinputfd != -1)
        {
            FD_SET(MCinputfd, &rmaskfd);
            if (MCinputfd > maxfd)
                maxfd = MCinputfd;
        }
        for (i = 0 ; i < MCnsockets ; i++)
        {
            if (MCsockets[i]->resolve_state != kMCSocketStateResolving &&
               MCsockets[i]->resolve_state != kMCSocketStateError)
            {
                if (MCsockets[i]->connected && !MCsockets[i]->closing
                    && !MCsockets[i]->shared || MCsockets[i]->accepting)
                    FD_SET(MCsockets[i]->fd, &rmaskfd);
                if (!MCsockets[i]->connected || MCsockets[i]->wevents != NULL)
                    FD_SET(MCsockets[i]->fd, &wmaskfd);
                FD_SET(MCsockets[i]->fd, &emaskfd);
                if (MCsockets[i]->fd > maxfd)
                    maxfd = MCsockets[i]->fd;
                if (MCsockets[i]->added)
                {
                    delay = 0.0;
                    MCsockets[i]->added = False;
                    handled = True;
                }
            }
        }

        if (g_notify_pipe[0] != -1)
        {
            FD_SET(g_notify_pipe[0], &rmaskfd);
            if (g_notify_pipe[0] > maxfd)
                maxfd = g_notify_pipe[0];
        }

        MCModePreSelectHook(maxfd, rmaskfd, wmaskfd, emaskfd);

        struct timeval timeoutval;
        timeoutval.tv_sec = (long)delay;
        timeoutval.tv_usec = (long)((delay - floor(delay)) * 1000000.0);

            n = select(maxfd + 1, &rmaskfd, &wmaskfd, &emaskfd, &timeoutval);
        if (n <= 0)
            return handled;
        if (MCshellfd != -1 && FD_ISSET(MCshellfd, &rmaskfd))
            return True;
        if (MCinputfd != -1 && FD_ISSET(MCinputfd, &rmaskfd))
            readinput = True;
        for (i = 0 ; i < MCnsockets ; i++)
        {
            if (FD_ISSET(MCsockets[i]->fd, &emaskfd))
            {
                if (!MCsockets[i]->waiting)
                {
                    MCsockets[i]->error = strclone("select error");
                    MCsockets[i]->doclose();
                }
            }
            else
            {
                /* read first here, otherwise a situation can arise when select indicates
                 * read & write on the socket as part of the sslconnect handshaking
                 * and so consumed during writesome() leaving no data to read
                 */
                if (FD_ISSET(MCsockets[i]->fd, &rmaskfd) && !MCsockets[i]->shared)
                    MCsockets[i]->readsome();
                if (FD_ISSET(MCsockets[i]->fd, &wmaskfd))
                    MCsockets[i]->writesome();
            }
        }

        if (g_notify_pipe[0] != -1 && FD_ISSET(g_notify_pipe[0], &rmaskfd))
        {
            char t_notify_char;
            read(g_notify_pipe[0], &t_notify_char, 1);
        }

        MCModePostSelectHook(rmaskfd, wmaskfd, emaskfd);

        if (readinput)
        {
            int commandsize;
            ioctl(MCinputfd, FIONREAD, (char *)&commandsize);
            char *commands = new char[commandsize + 1];
            read(MCinputfd, commands, commandsize);
            commands[commandsize] = '\0';
            MCdefaultstackptr->getcurcard()->domess(commands);
            delete commands;
        }
        if (wasalarm)
            MCS_alarm(CHECK_INTERVAL);
        return True;
#endif /* MCS_poll_dsk_lnx */

        Boolean readinput = False;
        int4 n;
        uint2 i;
        Boolean wasalarm = alarmpending;
        Boolean handled = False;
        if (alarmpending)
            MCS_alarm(0.0);

        extern int g_notify_pipe[2];

        fd_set rmaskfd, wmaskfd, emaskfd;
        FD_ZERO(&rmaskfd);
        FD_ZERO(&wmaskfd);
        FD_ZERO(&emaskfd);
        int4 maxfd = 0;
        if (!MCnoui)
        {
            FD_SET(p_fd, &rmaskfd);
            maxfd = p_fd;
        }
        if (MCshellfd != -1)
        {
            FD_SET(MCshellfd, &rmaskfd);
            if (MCshellfd > maxfd)
                maxfd = MCshellfd;
        }
        if (MCinputfd != -1)
        {
            FD_SET(MCinputfd, &rmaskfd);
            if (MCinputfd > maxfd)
                maxfd = MCinputfd;
        }
        handled = MCSocketsAddToFileDescriptorSets(maxfd, rmaskfd, wmaskfd, emaskfd);
        if (handled)
            p_delay = 0.0;

        if (g_notify_pipe[0] != -1)
        {
            FD_SET(g_notify_pipe[0], &rmaskfd);
            if (g_notify_pipe[0] > maxfd)
                maxfd = g_notify_pipe[0];
        }

        MCModePreSelectHook(maxfd, rmaskfd, wmaskfd, emaskfd);

        struct timeval timeoutval;
        timeoutval.tv_sec = (long)p_delay;
        timeoutval.tv_usec = (long)((p_delay - floor(p_delay)) * 1000000.0);

            n = select(maxfd + 1, &rmaskfd, &wmaskfd, &emaskfd, &timeoutval);
        if (n <= 0)
            return handled;
        if (MCshellfd != -1 && FD_ISSET(MCshellfd, &rmaskfd))
            return True;
        if (MCinputfd != -1 && FD_ISSET(MCinputfd, &rmaskfd))
            readinput = True;
        MCSocketsHandleFileDescriptorSets(rmaskfd, wmaskfd, emaskfd);

        if (g_notify_pipe[0] != -1 && FD_ISSET(g_notify_pipe[0], &rmaskfd))
        {
            char t_notify_char;
            read(g_notify_pipe[0], &t_notify_char, 1);
        }

        MCModePostSelectHook(rmaskfd, wmaskfd, emaskfd);

        if (readinput)
        {
            int commandsize;
            ioctl(MCinputfd, FIONREAD, (char *)&commandsize);
            MCAutoArray<char> t_commands;
            MCAutoStringRef t_cmd_string;

            t_commands.New(commandsize + 1);
            read(MCinputfd, t_commands.Ptr(), commandsize);
            t_commands.Ptr()[commandsize] = '\0';
            /* UNCHECKED */ MCStringCreateWithSysString(t_commands.Ptr(), &t_cmd_string);
            MCdefaultstackptr->getcurcard()->domess(*t_cmd_string);
        }
        if (wasalarm)
            Alarm(CHECK_INTERVAL);
        return True;
    }

    virtual Boolean IsInteractiveConsole(int p_fd)
    {
#ifdef /* MCS_isatty_dsk_lnx */ LEGACY_SYSTEM
        return isatty(fd) != 0;
#endif /* MCS_isatty_dsk_lnx */
        return isatty(p_fd) != 0;
    }

    virtual int GetErrno(void)
    {
#ifdef /* MCS_geterrno_dsk_lnx */ LEGACY_SYSTEM
        return errno;
#endif /* MCS_geterrno_dsk_lnx */
        return errno;
    }

    virtual void SetErrno(int p_errno)
    {
#ifdef /* MCS_seterrno_dsk_lnx */ LEGACY_SYSTEM
        errno = value;
#endif /* MCS_seterrno_dsk_lnx */
        errno = p_errno;
    }

    virtual void LaunchDocument(MCStringRef p_document)
    {
#ifdef /* MCS_launch_document_dsk_lnx */ LEGACY_SYSTEM
        const char * p_mime_type ;
        const char * p_command ;
        GList * p_args = NULL;
        GnomeVFSMimeApplication * p_gvfs ;

        if ( MCuselibgnome)
        {
            if ( gnome_vfs_initialized() )
            {
                p_mime_type =  gnome_vfs_get_mime_type_for_name (p_document);
                p_gvfs = gnome_vfs_mime_get_default_application_for_uri( p_document, p_mime_type);
                if ( p_gvfs != NULL)
                {
                    p_args = g_list_append ( p_args, p_document );
                    gnome_vfs_mime_application_launch( p_gvfs, p_args);
                    g_list_free ( p_args ) ;

                }
            }
            else
                MCresult -> sets("not supported");
            delete p_document;
        }
        else
        {
            // p_document will be deleted by MCS_launch_url ()
            MCS_launch_url (p_document);
        }
#endif /* MCS_launch_document_dsk_lnx */

#ifdef _LINUX_SERVER
        MCresult -> sets("not supported");
#else
        const char * p_mime_type;
        GList * p_args = NULL;
        GnomeVFSMimeApplication * p_gvfs ;

        if (MCuselibgnome)
        {
            if (gnome_vfs_initialized())
            {
                MCAutoStringRefAsSysString t_document_sys;
                /* UNCHECKED */ t_document_sys.Lock(p_document);

                p_mime_type = gnome_vfs_get_mime_type_for_name(*t_document_sys);
                p_gvfs = gnome_vfs_mime_get_default_application_for_uri(*t_document_sys, p_mime_type);
                if (p_gvfs != NULL)
                {
                    p_args = g_list_append(p_args, (gpointer)*t_document_sys);
                    gnome_vfs_mime_application_launch(p_gvfs, p_args);
                    g_list_free(p_args);
                }
            }
            else
                MCresult -> sets("not supported");
        }
        else
        {
            LaunchUrl(p_document);
        }
#endif
    }

    virtual void LaunchUrl(MCStringRef p_document)
    {
#ifdef /* MCS_launch_url_dsk_lnx */ LEGACY_SYSTEM
        GError *err = NULL;
        if ( MCuselibgnome )
        {
            if (! gnome_url_show (p_document, &err) )
                MCresult -> sets(err->message);
        }
        else
        {
            char *t_handler = nil;
            /* UNCHECKED */ MCCStringFormat(t_handler, LAUNCH_URL_SCRIPT, p_document);
            MCExecPoint ep (NULL, NULL, NULL) ;
            MCdefaultstackptr->domess(t_handler);
            MCresult->fetch(ep);
            MCCStringFree(t_handler);
        }

        // MW-2007-12-13: <p_document> is owned by the callee
        delete p_document;
#endif /* MCS_launch_url_dsk_lnx */

#ifdef _LINUX_SERVER
        MCresult->setvalueref(MCSTR("no association"));
        return;
#else
        GError *err = NULL;
        if (MCuselibgnome)
        {
            MCAutoStringRefAsSysString t_document_sys;
            /* UNCHECKED */ t_document_sys.Lock(p_document);
            if (!gnome_url_show(*t_document_sys, &err))
                MCresult -> sets(err->message);
        }
        else
        {
            MCAutoStringRef t_handler;
            /* UNCHECKED */ MCStringFormat(&t_handler, LAUNCH_URL_SCRIPT, p_document);
            MCdefaultstackptr->domess(*t_handler);
        }
#endif
    }

    virtual void DoAlternateLanguage(MCStringRef p_script, MCStringRef p_language)
    {
#ifdef /* MCS_doalternatelanguage_dsk_lnx */ LEGACY_SYSTEM
        MCresult->sets("alternate language not found");
#endif /* MCS_doalternatelanguage_dsk_lnx */
        MCresult->sets("alternate language not found");
    }

    virtual bool AlternateLanguages(MCListRef& r_list)
    {
#ifdef /* MCS_alternatelanguages_dsk_lnx */ LEGACY_SYSTEM
        ep . clear();
#endif /* MCS_alternatelanguages_dsk_lnx */
        r_list = MCValueRetain(kMCEmptyList);
        return true;
    }

    virtual bool GetDNSservers(MCListRef& r_list)
    {
#ifdef /* MCS_getDNSservers_dsk_lnx */ LEGACY_SYSTEM
        ep . clear();
        MCresult->store(ep, False);
        MCdefaultstackptr->domess(DNS_SCRIPT);
        MCresult->fetch(ep);
#endif /* MCS_getDNSservers_dsk_lnx */
        MCAutoListRef t_list;

        MCresult->clear();
        MCdefaultstackptr->domess(MCSTR(DNS_SCRIPT));

        return MCListCreateMutable('\n', &t_list) &&
            MCListAppend(*t_list, MCresult->getvalueref()) &&
            MCListCopy(*t_list, r_list);
    }
};

MCSystemInterface *MCDesktopCreateLinuxSystem()
{
    return new MCLinuxDesktop;
}
