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

#include <gtk/gtk.h>

#include <syslog.h>

////////////////////////////////////////////////////////////////////////////////

// This is in here so we do not need GLIBC2.4
extern "C" void __attribute__ ((noreturn)) __stack_chk_fail (void)
{
	abort();
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
            
            switch(baudrate)
            {
            case 4000000: baud = B4000000; break;
            case 3500000: baud = B3500000; break;
            case 3000000: baud = B3000000; break;
            case 2500000: baud = B2500000; break;
            case 2000000: baud = B2000000; break;
            case 1500000: baud = B1500000; break;
            case 1152000: baud = B1152000; break;
            case 1000000: baud = B1000000; break;
            case 921600: baud = B921600; break;
            case 576000: baud = B576000; break;
            case 500000: baud = B500000; break;
            case 460800: baud = B460800; break;
            case 230400: baud = B230400; break;
            case 115200: baud = B115200; break;
            case 57600: baud = B57600; break;
            case 38400: baud = B38400; break;
            case 19200: baud = B19200; break;
            case 9600: baud = B9600; break;
            case 4800: baud = B4800; break;
            case 2400: baud = B2400; break;
            case 1800: baud = B1800; break;
            case 1200: baud = B1200; break;
            case 600: baud = B600; break;
            case 300: baud = B300; break;
            default:
                baud = B9600;
                break;
            }
            
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
        MCLog("Error getting terminous attributes");
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
        parseSerialControlStr(str, &theTermios);
        str = each;
    }

    //configure the serial output device
    parseSerialControlStr(str,&theTermios);
    if (tcsetattr(sRefNum, TCSANOW, &theTermios) < 0)
    {
        MCLog("Error setting terminous attributes");
    }

    delete[] controlptr;
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
	        if (!MCMemoryResizeArray(buffersize + readsize + 1,
	                                 buffer, buffersize))
		        return IO_ERROR;
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
	return fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL, 0) & O_APPEND) | O_NONBLOCK)
           >= 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_lnx_is_link(MCStringRef p_path)
{
    MCAutoStringRefAsSysString t_path;
    /* UNCHECKED */ t_path.Lock(p_path);
    struct stat64 buf;
    return (lstat64(*t_path, &buf) == 0 && S_ISLNK(buf.st_mode));
}

bool MCS_lnx_readlink(MCStringRef p_path, MCStringRef& r_link)
{
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
        // SN-2015-11-19: [[ Bug 16450 ]] Ask for UTF-16LE to avoid iconv to put
        // a BOM char at the start of the stream
        iconv_t t_converter;
        t_converter = iconv_open("UTF-16LE", p_encoding);

        ConverterRecord *t_record;
        t_record = new (nothrow) ConverterRecord;
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
            MCPlayerHandle t_player = MCplayers;
            // If we have some players waiting then deal with these first
            waitedpid = -1;
            if (t_player.IsValid())
            {
                waitedpid = wait(NULL);
                // Moving these two lines half fixes bug 5966 - however it still isn't quite right
                // as there will still be some interaction between a player and shell command
                while(t_player.IsValid())
                {
                    if (t_player.IsValid() && waitedpid == t_player->getpid())
                    {
                        if (t_player->isdisposable())
                            t_player->playstop();
                        else
                            MCscreen->delaymessage(t_player, MCM_play_stopped, NULL, NULL);

                        t_player->shutdown();
                        break;
                    }
                    t_player = t_player -> getnextplayer() ;
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
        if (fwrite(p_buffer, 1, p_length, m_fptr) != p_length)
            return false;
        
        return true;
    }
    
    virtual bool Seek(int64_t p_offset, int p_dir)
    {
        return fseeko64(m_fptr, p_offset, p_dir > 0 ? SEEK_SET : (p_dir < 0 ? SEEK_END : SEEK_CUR)) == 0;
    }
    
    virtual bool Truncate(void)
    {
        return ftruncate(fileno(m_fptr), ftell(m_fptr)) == 0;
    }
    
    virtual bool Sync(void)
    {
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
        if (fflush(m_fptr))
            return false;
        
        return true;
    }
    
    virtual bool PutBack(char p_char)
    {
        if (ungetc(p_char, m_fptr) != p_char)
            return false;
        
        return true;
    }
    
    virtual int64_t Tell(void)
    {
        return ftello64(m_fptr);
    }
    
    virtual void *GetFilePointer(void)
    {
        return (void*)m_fptr;
    }
    
    virtual uint64_t GetFileSize(void)
    {
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
    }

    virtual void Debug(MCStringRef p_string)
    {
		MCAutoStringRefAsSysString t_string;
		if (t_string.Lock(p_string))
			syslog(LOG_DEBUG, "%s", *t_string);
    }

    virtual real64_t GetCurrentTime(void)
    {
        struct timezone tz;
        struct timeval tv;

        gettimeofday(&tv, &tz);
        curtime = tv.tv_sec + (real8)tv.tv_usec / 1000000.0;
        return curtime;
    }

    virtual bool GetVersion(MCStringRef& r_string)
    {
        struct utsname u;
        uname(&u);
        MCAutoStringRef t_sysname, t_release;
        /* UNCHECKED */ MCStringCreateWithSysString(u.sysname, &t_sysname);
        /* UNCHECKED */ MCStringCreateWithSysString(u.release, &t_release);
        return MCStringFormat(r_string, "%@ %@", *t_sysname, *t_release);
    }

    virtual bool GetMachine(MCStringRef& r_string)
    {
        struct utsname u;
        uname(&u);
        return MCStringCreateWithNativeChars((const char_t *)u.machine, MCCStringLength(u.machine), r_string);
    }

    virtual bool GetAddress(MCStringRef& r_address)
    {
        struct utsname u;
        uname(&u);
        MCAutoStringRef t_nodename;
        /* UNCHECKED */ MCStringCreateWithSysString(u.nodename, &t_nodename);
        return MCStringFormat(r_address, "%@:%@", *t_nodename, MCcmd);
    }

    virtual uint32_t GetProcessId(void)
    {
        return getpid();
    }

    virtual void Alarm(real64_t p_when)
    {
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
#ifdef NOSETENV
        char *dptr = new (nothrow) char[strlen(p_name) + strlen(p_value) + 2];
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
        MCAutoStringRefAsSysString t_path;
        MCAutoStringRef t_resolved_path;
        MCS_resolvepath(p_path, &t_resolved_path);
        /* UNCHECKED */ t_path.Lock(*t_resolved_path);
        Boolean done = unlink(*t_path) == 0;
        return done;
    }

    virtual Boolean RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
    {
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
        return MCS_rename(p_old_name, p_new_name);
    }

    virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
        return MCS_rename(p_old_name, p_new_name);
    }

    virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias)
    {
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
        return MCS_resolvepath(p_target, r_dest);
    }

    virtual bool GetCurrentFolder(MCStringRef& r_path)
    {
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
        MCAutoStringRef t_home, t_tilde;
        if (!MCStringCreateWithCString("~", &t_tilde) ||
            !MCS_resolvepath(*t_tilde, &t_home))
            return false;

        if (MCNameIsEqualToCaseless(p_type, MCN_desktop))
            return MCStringFormat(r_folder, "%@/Desktop", *t_home);
        else if (MCNameIsEqualToCaseless(p_type, MCN_home))
            return MCStringCopy(*t_home, r_folder);
        else if (MCNameIsEqualToCaseless(p_type, MCN_temporary))
            return MCStringCreateWithCString("/tmp", r_folder);
        // SN-2014-08-08: [[ Bug 13026 ]] Fix ported from 6.7
        else if (MCNameIsEqualToCaseless(p_type, MCN_engine)
                 // SN-2015-04-20: [[ Bug 14295 ]] If we are here, we are a standalone
                 // so the resources folder is the engine folder.
                 || MCNameIsEqualToCaseless(p_type, MCN_resources))
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
        r_devices = MCValueRetain(kMCEmptyString);
        return true;
    }

    virtual Boolean GetDrives(MCStringRef& r_drives)
    {
        r_drives = MCValueRetain(kMCEmptyString);
        return true;
    }

    virtual Boolean FileExists(MCStringRef p_path)
    {
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
        MCAutoStringRefAsSysString t_path_sys;
        /* UNCHECKED */ t_path_sys.Lock(p_path);
        if (chmod(*t_path_sys, p_mask) != 0)
            return IO_ERROR;
        return IO_NORMAL;
    }

    virtual uint2 UMask(uint2 p_mask)
    {
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
            t_handle = new (nothrow) MCStdioFileHandle(fptr);

        return t_handle;
    }

    virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map)
    {
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
				// The length of a file could be > 32-bit, so we have to check that
				// the file size fits into a 32-bit integer as that is what mmap expects
				off_t t_len = t_buf.st_size;
                if (t_len != 0 && t_len < UINT32_MAX)
                {
                    char *t_buffer = (char *)mmap(NULL, t_len, PROT_READ, MAP_SHARED,
                                                t_fd, 0);
                    // MW-2013-05-02: [[ x64 ]] Make sure we use the MAP_FAILED constant
                    //   rather than '-1'.
                    if (t_buffer != MAP_FAILED)
                    {
                        t_handle = new (nothrow) MCMemoryMappedFileHandle(t_fd, t_buffer, t_len);
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
		else /* No access requested */
			return NULL;

        t_fptr = fopen(*t_path_sys, t_mode);

        if (t_fptr == NULL && p_mode != kMCOpenFileModeRead)
            t_fptr = fopen(*t_path_sys, IO_CREATE_MODE);

        if (t_fptr != NULL)
        {
            t_handle = new (nothrow) MCStdioFileHandle(t_fptr);
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
            t_handle = new (nothrow) MCStdioFileHandle(t_fptr);

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

        if (t_fptr != NULL)
        {
            setbuf(t_fptr, nullptr);
            configureSerialPort((short)fileno(t_fptr));

            t_handle = new (nothrow) MCStdioFileHandle(t_fptr);
        }

        return t_handle;
    }

    // NOTE: 'GetTemporaryFileName' returns a standard (not native) path.
    virtual bool GetTemporaryFileName(MCStringRef& r_tmp_name)
    {
        return MCStringCreateWithSysString(tmpnam(NULL), r_tmp_name);
    }

    virtual bool ListFolderEntries(MCStringRef p_folder, MCSystemListFolderEntriesCallback p_callback, void *x_context)
    {
		MCAutoStringRefAsSysString t_path;
		if (p_folder == nil)
			/* UNCHECKED */ t_path . Lock(MCSTR("."));
		else
			/* UNCHECKED */ t_path . Lock(p_folder);

        DIR *dirptr;

        if ((dirptr = opendir(*t_path)) == NULL)
        {
            return false;
        }

        struct dirent64 *direntp;

        bool t_success = true;

		/* For each directory entry, we need to construct a path that can
		 * be passed to stat(2).  Allocate a buffer large enough for the
		 * path, a path separator character, and any possible filename. */
		size_t t_path_len = strlen(*t_path);
		size_t t_entry_path_len = t_path_len + 1 + NAME_MAX;
		char *t_entry_path = new (nothrow) char[t_entry_path_len + 1];
		strcpy (t_entry_path, *t_path);
		if ((*t_path)[t_path_len - 1] != '/')
		{
			strcat (t_entry_path, "/");
			++t_path_len;
		}

        while (t_success && (direntp = readdir64(dirptr)) != NULL)
        {
            MCSystemFolderEntry p_entry;
            MCAutoStringRef t_unicode_name;

            if (MCCStringEqual(direntp->d_name, "."))
                continue;

			/* Truncate the directory entry path buffer to the path
			 * separator. */
			t_entry_path[t_path_len] = 0;
			strcat (t_entry_path, direntp->d_name);

            struct stat buf;
            stat(t_entry_path, &buf);

            t_success = MCStringCreateWithSysString(direntp -> d_name, &t_unicode_name);
            
            if (t_success)
            {
                p_entry.name = *t_unicode_name;
                p_entry.data_size = buf.st_size;
                p_entry.modification_time = (uint32_t)buf.st_mtime;
                p_entry.access_time = (uint32_t)buf.st_atime;
                p_entry.group_id = buf.st_uid;
                p_entry.user_id = buf.st_uid;
                p_entry.permissions = buf.st_mode & 0777;
                p_entry.is_folder = S_ISDIR(buf.st_mode);

                t_success = p_callback(x_context, &p_entry);
            }
        }

        delete[] t_entry_path; /* Allocated with new[] */
        closedir(dirptr);

        return t_success;
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
                    !MCStringAppendSubstring(*t_tilde_path, p_path, MCRangeMakeMinMax(t_user_end, MCStringGetLength(p_path))))
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
        return MCStringCopy(p_path, r_long_path);
    }

    virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
    {
        return MCStringCopy(p_path, r_short_path);
    }

    virtual bool Shell(MCStringRef p_filename, MCDataRef& r_data, int& r_retcode)
    {
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
                MCprocesses[MCnprocesses].pid = 0;
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
                if (MCprocesses[index].pid == -1)
                {
					MCeerror->add(EE_SYSTEM_FUNCTION, 0, 0, "fork");
					MCeerror->add(EE_SYSTEM_CODE, 0, 0, errno);
					MCeerror->add(EE_SYSTEM_MESSAGE, 0, 0, strerror(errno));
					close(tochild[0]);
					close(tochild[1]);
					close(toparent[0]);
					close(toparent[1]);

                    MCprocesses[index].pid = 0;
                    // SN-2015-01-29: [[ Bug 14462 ]] Should return false, not true
                    return false;
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
            }
            else
            {
                MCeerror->add(EE_SYSTEM_FUNCTION, 0, 0, "pipe");
                MCeerror->add(EE_SYSTEM_CODE, 0, 0, errno);
                MCeerror->add(EE_SYSTEM_MESSAGE, 0, 0, strerror(errno));
                close(tochild[0]);
                close(tochild[1]);
                // SN-2015-01-29: [[ Bug 14462 ]] Should return false, not true
                return false;
            }
        }
        else
        {
            MCeerror->add(EE_SYSTEM_FUNCTION, 0, 0, "pipe");
            MCeerror->add(EE_SYSTEM_CODE, 0, 0, errno);
            MCeerror->add(EE_SYSTEM_MESSAGE, 0, 0, strerror(errno));
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
        uint32_t t_used = 0;
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
        for (int i = 0; i < MCnprocesses; ++i)
        {
            pid_t t_pid = MCprocesses[i].pid;
            if (t_pid <= 0)
                continue; /* No PID in this table slot */

            if (t_pid == waitedpid)
                continue; /* This PID was already dealt with in signal handler */

            int t_wait_status = 0;
            pid_t t_wait_pid = waitpid(t_pid, &t_wait_status, WNOHANG);
            if (t_wait_pid != t_pid)
                continue; /* Process hasn't exited or is not a child */

            if (MCprocesses[i].ihandle != nullptr)
                clearerr(static_cast<FILE*>(MCprocesses[i].ihandle->GetFilePointer()));
            MCprocesses[i].pid = 0;
            MCprocesses[i].retcode = WEXITSTATUS(t_wait_status);
        }
    }

    virtual uint32_t GetSystemError(void)
    {
        return errno;
    }

    virtual bool StartProcess(MCNameRef p_name, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated)
    {
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

                        argv = new (nothrow) char *[3];
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
        kill(p_pid, p_sig);
    }

    virtual void KillAll(void)
    {
        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = (void (*)(int))SIG_IGN;

        sigaction(SIGCHLD, &action, NULL);
        while (MCnprocesses--)
        {
            MCValueRelease(MCprocesses[MCnprocesses] . name);
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

        if (g_notify_pipe[0] != -1)
        {
            FD_SET(g_notify_pipe[0], &rmaskfd);
            if (g_notify_pipe[0] > maxfd)
                maxfd = g_notify_pipe[0];
        }

        // Prepare GLib for the poll we are about to do
        gint t_glib_ready_priority;
        if (g_main_context_prepare(NULL, &t_glib_ready_priority))
            handled = true;
        
        // If things are already ready, ensure the timeout is zero
        if (handled)
            p_delay = 0.0;
            
        // Get the list of file descriptors that the GLib main loop needs to
        // add to the poll operation.
        GMainContext* t_glib_main_context = g_main_context_default();
        MCAutoArray<GPollFD> t_glib_fds;
        gint t_glib_timeout;
        t_glib_fds.Extend(g_main_context_query(t_glib_main_context, G_MAXINT, &t_glib_timeout, NULL, 0));
        g_main_context_query(t_glib_main_context, G_MAXINT, &t_glib_timeout, t_glib_fds.Ptr(), t_glib_fds.Size());
        
        // Add the GLib descriptors to the list
        for (uindex_t i = 0; i < t_glib_fds.Size(); i++)
        {
            // Are we polling this FD for reading?
            if (t_glib_fds[i].events & (G_IO_IN|G_IO_PRI))
                FD_SET(t_glib_fds[i].fd, &rmaskfd);
            if (t_glib_fds[i].events & (G_IO_OUT))
                FD_SET(t_glib_fds[i].fd, &wmaskfd);
            if (t_glib_fds[i].events & (G_IO_ERR|G_IO_HUP))
                FD_SET(t_glib_fds[i].fd, &emaskfd);
            
            if (t_glib_fds[i].events != 0 && t_glib_fds[i].fd > maxfd)
                maxfd = t_glib_fds[i].fd;
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

        // Check whether any of the GLib file descriptors were signalled
        for (uindex_t i = 0; i < t_glib_fds.Size(); i++)
        {
            if (FD_ISSET(t_glib_fds[i].fd, &rmaskfd))
                t_glib_fds[i].revents |= G_IO_IN;
            if (FD_ISSET(t_glib_fds[i].fd, &wmaskfd))
                t_glib_fds[i].revents |= G_IO_OUT;
            if (FD_ISSET(t_glib_fds[i].fd, &emaskfd))
                t_glib_fds[i].revents |= G_IO_ERR;
        }
        
        // Let GLib know which file descriptors were signalled. We don't
        // dispatch these now as that will happen later.
        g_main_context_check(t_glib_main_context, G_MAXINT, t_glib_fds.Ptr(), t_glib_fds.Size());
        
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
        return isatty(p_fd) != 0;
    }

    virtual int GetErrno(void)
    {
        return errno;
    }

    virtual void SetErrno(int p_errno)
    {
        errno = p_errno;
    }

    virtual void LaunchDocument(MCStringRef p_document)
    {
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
        MCresult->sets("alternate language not found");
    }

    virtual bool AlternateLanguages(MCListRef& r_list)
    {
        r_list = MCValueRetain(kMCEmptyList);
        return true;
    }

    virtual bool GetDNSservers(MCListRef& r_list)
    {
        MCAutoListRef t_list;

        MCresult->clear();
        MCdefaultstackptr->domess(MCSTR(DNS_SCRIPT));

        return MCListCreateMutable('\n', &t_list) &&
            MCListAppend(*t_list, MCresult->getvalueref()) &&
            MCListCopy(*t_list, r_list);
    }
    
    virtual void ShowMessageDialog(MCStringRef p_title,
                                   MCStringRef p_message)
    {
        MCAutoStringRefAsUTF8String t_title_utf8;
        if (!t_title_utf8 . Lock(p_title))
            return;
        
        MCAutoStringRefAsUTF8String t_message_utf8;
        if (!t_message_utf8 . Lock(p_message))
            return;
        
        typedef GtkMessageDialog *(*gtk_message_dialog_newPTR)(GtkWindow *parent,
                                                               GtkDialogFlags flags,
                                                               GtkMessageType type,
                                                               GtkButtonsType buttons,
                                                               const gchar *message_format,
                                                               ...);
        extern gtk_message_dialog_newPTR gtk_message_dialog_new_ptr;
        
        GtkMessageDialog *t_dialog;
        t_dialog = gtk_message_dialog_new_ptr(NULL,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_CLOSE,
                                              "%s",
                                              *t_title_utf8);
        
        typedef void (*gtk_message_dialog_format_secondary_textPTR)(GtkMessageDialog *message_dialog,
                                                                    const gchar *message_format,
                                                                    ...);
        extern gtk_message_dialog_format_secondary_textPTR gtk_message_dialog_format_secondary_text_ptr;
        gtk_message_dialog_format_secondary_text_ptr(t_dialog,
                                                     "%s",
                                                     *t_message_utf8);
        gtk_dialog_run(GTK_DIALOG(t_dialog));
        gtk_widget_destroy(GTK_WIDGET(t_dialog));
    }
};

MCSystemInterface *MCDesktopCreateLinuxSystem()
{
    return new MCLinuxDesktop;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_get_browsers(MCStringRef &r_browsers)
{
    r_browsers = nullptr;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
