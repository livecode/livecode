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

#include "parsedef.h"
#include "filedefs.h"
#include "globdefs.h"

#include "execpt.h"
#include "globals.h"
#include "system.h"
#include "osspec.h"
#include "util.h"
#include "mcio.h"
#include "stack.h"

#include <sys/stat.h>

#include "foundation.h"

#define ENTRIES_CHUNK 1024

#define SERIAL_PORT_BUFFER_SIZE  16384 //set new buffer size for serial input port
#include <termios.h>
#define B16600 16600

#include <pwd.h>

#define USE_FSCATALOGINFO

///////////////////////////////////////////////////////////////////////////////

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
    
	char *controlptr = strclone(MCserialcontrolsettings);
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

///////////////////////////////////////////////////////////////////////////////

void MCS_setfiletype(MCStringRef p_new_path)
{
	FSRef t_fsref;
    // TODO Check whether the double path resolution is an issue
	if (MCS_pathtoref(p_new_path, t_fsref) != noErr)
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

///////////////////////////////////////////////////////////////////////////////

extern "C"
{
#include	<CoreFoundation/CoreFoundation.h>
#include	<IOKit/IOKitLib.h>
#include	<IOKit/serial/IOSerialKeys.h>
#include	<IOKit/IOBSD.h>
}

static kern_return_t FindSerialPortDevices(io_iterator_t *serialIterator, mach_port_t *masterPort)
{
    kern_return_t	kernResult;
    CFMutableDictionaryRef classesToMatch;
    if ((kernResult = IOMasterPort(NULL, masterPort)) != KERN_SUCCESS)
        return kernResult;
    if ((classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue)) == NULL)
        return kernResult;
    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey),
                         CFSTR(kIOSerialBSDRS232Type));
    //kIOSerialBSDRS232Type filters KeySpan USB modems use
    //kIOSerialBSDModemType to get 'real' serial modems for OSX
    //computers with real serial ports - if there are any!
    kernResult = IOServiceGetMatchingServices(*masterPort, classesToMatch,
                                              serialIterator);
    return kernResult;
}

static void getIOKitProp(io_object_t sObj, const char *propName,
                         char *dest, uint2 destlen)
{
	CFTypeRef nameCFstring;
	dest[0] = 0;
	nameCFstring = IORegistryEntryCreateCFProperty(sObj,
                                                   CFStringCreateWithCString(kCFAllocatorDefault, propName,
                                                                             kCFStringEncodingASCII),
                                                   kCFAllocatorDefault, 0);
	if (nameCFstring)
	{
		CFStringGetCString((CFStringRef)nameCFstring, (char *)dest, (long)destlen,
                           (unsigned long)kCFStringEncodingASCII);
		CFRelease(nameCFstring);
	}
}

///////////////////////////////////////////////////////////////////////////////

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
static void parseSerialControlStr(char *set, struct termios *theTermios);

void MCS_setfiletype(MCStringRef newpath);

///////////////////////////////////////////////////////////////////////////////

/* LEGACY */
extern char *path2utf(const char *);

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
            case ES_PASS:
            case ES_NOT_HANDLED:
                MCdefaultstackptr->getcard()->message(MCM_shut_down);
                MCquit = True; //set MC quit flag, to invoke quitting
                return;
            default:
                break;
		}
            MCS_killall();
            exit(-1);
            
            // MW-2009-01-29: [[ Bug 6410 ]] If one of these signals occurs, we need
            //   to return, so that the OS can CrashReport away.
        case SIGILL:
        case SIGBUS:
        case SIGSEGV:
            fprintf(stderr, "%s exiting on signal %d\n", MCcmd, sig);
            MCS_killall();
            return;
            
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGIOT:
            if (MCnoui)
                exit(1);
            MCabortscript = True;
            break;
        case SIGFPE:
            errno = EDOM;
            break;
        case SIGCHLD:
            MCS_checkprocesses();
            break;
        case SIGALRM:
            MCalarm = True;
            break;
        case SIGPIPE:
        default:
            break;
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////

void MCS_startprocess_unix(MCNameRef name, MCStringRef doc, Open_mode mode, Boolean elevated);

///////////////////////////////////////////////////////////////////////////////

static Boolean hasPPCToolbox = False;
static Boolean hasAppleEvents = False;

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

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
#ifdef /* MCS_dopen_dsk_mac */ LEGACY_SYSTEM
	IO_handle handle = NULL;
	FILE *fptr = fdopen(fd, mode);
	
	if (fptr != NULL)
	{
		// MH-2007-05-17: [[Bug 3196]] Opening the write pipe to a process should not be buffered.
		if (mode[0] == 'w')
			setvbuf(fptr, NULL, _IONBF, 0);

		handle = new IO_header(fptr, 0, 0, NULL, NULL, 0, 0);
	}	
	return handle;
#endif /* MCS_dopen_dsk_mac */
		FILE *t_stream;
		t_stream = fdopen(fd, p_mode);
		if (t_stream == NULL)
			return NULL;
		
		// MH-2007-05-17: [[Bug 3196]] Opening the write pipe to a process should not be buffered.
		if (mode[0] == 'w')
			setvbuf(t_stream, NULL, _IONBF, 0);
		
		MCStdioFileHandle *t_handle;
		t_handle = new MCStdioFileHandle;
		t_handle -> m_stream = t_stream;
		
		return t_handle;		
	}
	
	virtual void Close(void)
	{
#ifdef /* MCS_close_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_close_dsk_mac */
        if (m_serialIn != 0 || m_serialOut != 0)
        {//close the serial port
            
        }
        else if (m_stream != NULL)
            fclose(m_stream);
        
        delete m_stream;
        m_stream = NULL;
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
#ifdef /* MCS_seek_cur_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_seek_cur_dsk_mac */
#ifdef /* MCS_seek_set_dsk_mac */ LEGACY_SYSTEM
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_seek_set(stream, offset);
	
	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->buffer + offset);
	else
		if (fseeko(stream->fptr, offset, SEEK_SET) != 0)
			return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_seek_set_dsk_mac */
#ifdef /* MCS_seek_end_dsk_mac */ LEGACY_SYSTEM
    /* seek to offset from the end of the file */
	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->buffer + stream->len + offset);
	else
		if (fseeko(stream->fptr, offset, SEEK_END) != 0)
			return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_seek_end_dsk_mac */
		return fseeko(m_stream, offset, p_dir < 0 ? SEEK_END : (p_dir > 0 ? SEEK_SET : SEEK_CUR)) == 0;
	}
	
	virtual bool Truncate(void)
	{
#ifdef /* MCS_trunc_dsk_mac */ LEGACY_SYSTEM
    
	if (ftruncate(fileno(stream->fptr), ftell(stream->fptr)))
		return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_trunc_dsk_mac */
		return ftruncate(fileno(m_stream), ftell(m_stream)) == 0;
	}
	
	virtual bool Sync(void)
	{
#ifdef /* MCS_sync_dsk_mac */ LEGACY_SYSTEM
	if (stream->fptr != NULL)
	{
		int4 pos = ftello(stream->fptr);
		if (fseek(stream->fptr, pos, SEEK_SET) != 0)
			return IO_ERROR;
	}
	return IO_NORMAL;
#endif /* MCS_sync_dsk_mac */
		int64_t t_pos;
		t_pos = ftello(m_stream);
		return fseeko(m_stream, t_pos, SEEK_SET) == 0;
	}
	
	virtual bool Flush(void)
	{
#ifdef /* MCS_flush_dsk_mac */ LEGACY_SYSTEM
    //flush file buffer
	if (stream->fptr != NULL)
		if (fflush(stream->fptr))
			return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_flush_dsk_mac */
		return fflush(m_stream) == 0;
	}
	
	virtual bool PutBack(char p_char)
	{
#ifdef /* MCS_putback_dsk_mac */ LEGACY_SYSTEM
	if (stream -> serialIn != 0 || stream -> fptr == NULL)
		return MCS_seek_cur(stream, -1);
	
	if (ungetc(c, stream -> fptr) != c)
		return IO_ERROR;
		
	return IO_NORMAL;
#endif /* MCS_putback_dsk_mac */
		return ungetc(p_char, m_stream) != EOF;
	}
	
	virtual int64_t Tell(void)
	{
#ifdef /* MCS_tell_dsk_mac */ LEGACY_SYSTEM
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_tell(stream);

	if (stream->fptr != NULL)
		return ftello(stream->fptr);
	else
		return stream->ioptr - stream->buffer;
#endif /* MCS_tell_dsk_mac */
		return ftello(m_stream);
	}
	
	virtual int64_t GetFileSize(void)
	{
#ifdef /* MCS_fsize_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_fsize_dsk_mac */
		struct stat t_info;
		if (fstat(fileno(m_stream), &t_info) != 0)
			return 0;
		return t_info . st_size;
	}
	
	virtual void *GetFilePointer(void)
	{
		return NULL;
	}
	
	FILE *GetStream(void)
	{
		return m_stream;
	}
	
private:
	FILE *m_stream;
};

struct MCMacDesktop: public MCSystemInterface
{
	virtual bool Initialize(void)
    {
#ifdef /* MCS_init_dsk_mac */ LEGACY_SYSTEM
        IO_stdin = new IO_header(stdin, 0, 0, 0, NULL, 0, 0);
        IO_stdout = new IO_header(stdout, 0, 0, 0, NULL, 0, 0);
        IO_stderr = new IO_header(stderr, 0, 0, 0, NULL, 0, 0);
        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = handle_signal;
        action.sa_flags = SA_RESTART;
        sigaction(SIGHUP, &action, NULL);
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGQUIT, &action, NULL);
        sigaction(SIGIOT, &action, NULL);
        sigaction(SIGPIPE, &action, NULL);
        sigaction(SIGALRM, &action, NULL);
        sigaction(SIGTERM, &action, NULL);
        sigaction(SIGUSR1, &action, NULL);
        sigaction(SIGUSR2, &action, NULL);
        sigaction(SIGFPE, &action, NULL);
        action.sa_flags |= SA_NOCLDSTOP;
        sigaction(SIGCHLD, &action, NULL);
        
        // MW-2009-01-29: [[ Bug 6410 ]] Make sure we cause the handlers to be reset to
        //   the OS default so CrashReporter will kick in.
        action.sa_flags = SA_RESETHAND;
        sigaction(SIGSEGV, &action, NULL);
        sigaction(SIGILL, &action, NULL);
        sigaction(SIGBUS, &action, NULL);
        
        // MW-2010-05-11: Make sure if stdin is not a tty, then we set non-blocking.
        //   Without this you can't poll read when a slave process.
        if (!MCS_isatty(0))
            MCS_nodelay(0);
        
        setlocale(LC_ALL, MCnullstring);
        
        _CurrentRuneLocale->__runetype[202] = _CurrentRuneLocale->__runetype[201];
        
        // Initialize our case mapping tables
        
        MCuppercasingtable = new uint1[256];
        for(uint4 i = 0; i < 256; ++i)
            MCuppercasingtable[i] = (uint1)i;
        UppercaseText((char *)MCuppercasingtable, 256, smRoman);
        
        MClowercasingtable = new uint1[256];
        for(uint4 i = 0; i < 256; ++i)
            MClowercasingtable[i] = (uint1)i;
        LowercaseText((char *)MClowercasingtable, 256, smRoman);
        
        //
        
        // MW-2013-03-22: [[ Bug 10772 ]] Make sure we initialize the shellCommand
        //   property here (otherwise it is nil in -ui mode).
        MCshellcmd = strclone("/bin/sh");
        
        //
        
        MoreMasters();
        InitCursor();
        MCinfinity = HUGE_VAL;
        
        long response;
        if (Gestalt(gestaltSystemVersion, &response) == noErr)
            MCmajorosversion = response;
		
        MCaqua = True;
        
        init_utf8_converters();
        
        CFBundleRef theBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.ApplicationServices"));
        if (theBundle != NULL)
        {
            if (CFBundleLoadExecutable(theBundle))
            {
                SwapQDTextFlagsPtr stfptr = (SwapQDTextFlagsPtr)CFBundleGetFunctionPointerForName(theBundle, CFSTR("SwapQDTextFlags"));
                if (stfptr != NULL)
                    stfptr(kQDSupportedFlags);
                CFBundleUnloadExecutable(theBundle);
            }
            CFRelease(theBundle);
        }
        
        MCAutoStringRef dptr;
        MCS_getcurdir(&dptr);
        if (MCStringGetLength(*dptr) <= 1)
        { // if root, then started from Finder
            SInt16 vRefNum;
            SInt32 dirID;
            HGetVol(NULL, &vRefNum, &dirID);
            FSSpec fspec;
            FSMakeFSSpec(vRefNum, dirID, NULL, &fspec);
            char *tpath = MCS_FSSpec2path(&fspec);
            char *newpath = new char[strlen(tpath) + 11];
            strcpy(newpath, tpath);
            strcat(newpath, "/../../../");
            MCAutoStringRef t_new_path_auto;
            /* UNCHECKED */ MCStringCreateWithCString(newpath, &t_new_path_auto);
            MCS_setcurdir(*t_new_path_auto);
            delete tpath;
            delete newpath;
        }
        
        // MW-2007-12-10: [[ Bug 5667 ]] Small font sizes have the wrong metrics
        //   Make sure we always use outlines - then everything looks pretty :o)
        SetOutlinePreferred(TRUE);
        
        MCS_reset_time();
        //do toolbox checking
        long result;
        hasPPCToolbox = (Gestalt(gestaltPPCToolboxAttr, &result)
                         ? False : result != 0);
        hasAppleEvents = (Gestalt(gestaltAppleEventsAttr, &result)
                          ? False : result != 0);
        uint1 i;
        if (hasAppleEvents)
        { //install required AE event handler
            for (i = 0; i < (sizeof(ourkeys) / sizeof(triplets)); ++i)
            {
                if (!ourkeys[i].theUPP)
                {
                    ourkeys[i].theUPP = NewAEEventHandlerUPP(ourkeys[i].theHandler);
                    AEInstallEventHandler(ourkeys[i].theEventClass,
                                          ourkeys[i].theEventID,
                                          ourkeys[i].theUPP, 0L, False);
                }
            }
        }
        
        // ** MODE CHOICE
        if (MCModeShouldPreprocessOpeningStacks())
        {
            EventRecord event;
            i = 2;
            // predispatch any openapp or opendoc events so that stacks[] array
            // can be properly initialized
            while (i--)
                while (WaitNextEvent(highLevelEventMask, &event,
                                     (unsigned long)0, (RgnHandle)NULL))
                    AEProcessAppleEvent(&event);
        }
        //install special handler
        AEEventHandlerUPP specialUPP = NewAEEventHandlerUPP(DoSpecial);
        AEInstallSpecialHandler(keyPreDispatch, specialUPP, False);
        
        if (Gestalt('ICAp', &response) == noErr)
        {
            OSErr err;
            ICInstance icinst;
            ICAttr icattr;
            err = ICStart(&icinst, 'MCRD');
            if (err == noErr)
            {
                Str255 proxystr;
                Boolean useproxy;
                
                long icsize = sizeof(useproxy);
                err = ICGetPref(icinst,  kICUseHTTPProxy, &icattr, &useproxy, &icsize);
                if (err == noErr && useproxy == True)
                {
                    icsize = sizeof(proxystr);
                    err = ICGetPref(icinst, kICHTTPProxyHost ,&icattr, proxystr, &icsize);
                    if (err == noErr)
                    {
                        p2cstr(proxystr);
                        MChttpproxy = strclone((char *)proxystr);
                    }
                }
                ICStop(icinst);
            }
        }
        
        
        MCS_weh = NewEventHandlerUPP(WinEvtHndlr);
        
        // MW-2005-04-04: [[CoreImage]] Load in CoreImage extension
        extern void MCCoreImageRegister(void);
        if (MCmajorosversion >= 0x1040)
            MCCoreImageRegister();
		
        if (!MCnoui)
        {
            setlinebuf(stdout);
            setlinebuf(stderr);
        }
#endif /* MCS_init_dsk_mac */
        IO_stdin = new IO_header(MCStdioFileHandle::OpenFd(stdin, IO_READ_MODE), 0);
        IO_stdout = new IO_header(MCStdioFileHandle::OpenFd(stdout, IO_WRITE_MODE), 0);
        IO_stderr = new IO_header(MCStdioFileHandle::OpenFd(stderr, IO_WRITE_MODE), 0);
        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = handle_signal;
        action.sa_flags = SA_RESTART;
        sigaction(SIGHUP, &action, NULL);
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGQUIT, &action, NULL);
        sigaction(SIGIOT, &action, NULL);
        sigaction(SIGPIPE, &action, NULL);
        sigaction(SIGALRM, &action, NULL);
        sigaction(SIGTERM, &action, NULL);
        sigaction(SIGUSR1, &action, NULL);
        sigaction(SIGUSR2, &action, NULL);
        sigaction(SIGFPE, &action, NULL);
        action.sa_flags |= SA_NOCLDSTOP;
        sigaction(SIGCHLD, &action, NULL);
        
        // MW-2009-01-29: [[ Bug 6410 ]] Make sure we cause the handlers to be reset to
        //   the OS default so CrashReporter will kick in.
        action.sa_flags = SA_RESETHAND;
        sigaction(SIGSEGV, &action, NULL);
        sigaction(SIGILL, &action, NULL);
        sigaction(SIGBUS, &action, NULL);
        
        // MW-2010-05-11: Make sure if stdin is not a tty, then we set non-blocking.
        //   Without this you can't poll read when a slave process.
        if (!MCS_isatty(0))
            MCS_nodelay(0);
        
        setlocale(LC_ALL, MCnullstring);
        
        _CurrentRuneLocale->__runetype[202] = _CurrentRuneLocale->__runetype[201];
        
        // Initialize our case mapping tables
        
        MCuppercasingtable = new uint1[256];
        for(uint4 i = 0; i < 256; ++i)
            MCuppercasingtable[i] = (uint1)i;
        UppercaseText((char *)MCuppercasingtable, 256, smRoman);
        
        MClowercasingtable = new uint1[256];
        for(uint4 i = 0; i < 256; ++i)
            MClowercasingtable[i] = (uint1)i;
        LowercaseText((char *)MClowercasingtable, 256, smRoman);
        
        //
        
        // MW-2013-03-22: [[ Bug 10772 ]] Make sure we initialize the shellCommand
        //   property here (otherwise it is nil in -ui mode).
        MCshellcmd = strclone("/bin/sh");
        
        //
        
        MoreMasters();
        InitCursor();
        MCinfinity = HUGE_VAL;
        
        long response;
        if (Gestalt(gestaltSystemVersion, &response) == noErr)
            MCmajorosversion = response;
		
        MCaqua = True;
        
        init_utf8_converters();
        
        CFBundleRef theBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.ApplicationServices"));
        if (theBundle != NULL)
        {
            if (CFBundleLoadExecutable(theBundle))
            {
                SwapQDTextFlagsPtr stfptr = (SwapQDTextFlagsPtr)CFBundleGetFunctionPointerForName(theBundle, CFSTR("SwapQDTextFlags"));
                if (stfptr != NULL)
                    stfptr(kQDSupportedFlags);
                CFBundleUnloadExecutable(theBundle);
            }
            CFRelease(theBundle);
        }
        
        MCAutoStringRef dptr;
        MCS_getcurdir(&dptr);
        if (MCStringGetLength(*dptr) <= 1)
        { // if root, then started from Finder
            SInt16 vRefNum;
            SInt32 dirID;
            HGetVol(NULL, &vRefNum, &dirID);
            FSSpec fspec;
            FSMakeFSSpec(vRefNum, dirID, NULL, &fspec);
            char *tpath = MCS_FSSpec2path(&fspec);
            char *newpath = new char[strlen(tpath) + 11];
            strcpy(newpath, tpath);
            strcat(newpath, "/../../../");
            MCAutoStringRef t_new_path_auto;
            /* UNCHECKED */ MCStringCreateWithCString(newpath, &t_new_path_auto);
            MCS_setcurdir(*t_new_path_auto);
            delete tpath;
            delete newpath;
        }
        
        // MW-2007-12-10: [[ Bug 5667 ]] Small font sizes have the wrong metrics
        //   Make sure we always use outlines - then everything looks pretty :o)
        SetOutlinePreferred(TRUE);
        
        MCS_reset_time();
        //do toolbox checking
        long result;
        hasPPCToolbox = (Gestalt(gestaltPPCToolboxAttr, &result)
                         ? False : result != 0);
        hasAppleEvents = (Gestalt(gestaltAppleEventsAttr, &result)
                          ? False : result != 0);
        uint1 i;
        if (hasAppleEvents)
        { //install required AE event handler
            for (i = 0; i < (sizeof(ourkeys) / sizeof(triplets)); ++i)
            {
                if (!ourkeys[i].theUPP)
                {
                    ourkeys[i].theUPP = NewAEEventHandlerUPP(ourkeys[i].theHandler);
                    AEInstallEventHandler(ourkeys[i].theEventClass,
                                          ourkeys[i].theEventID,
                                          ourkeys[i].theUPP, 0L, False);
                }
            }
        }
        
        // ** MODE CHOICE
        if (MCModeShouldPreprocessOpeningStacks())
        {
            EventRecord event;
            i = 2;
            // predispatch any openapp or opendoc events so that stacks[] array
            // can be properly initialized
            while (i--)
                while (WaitNextEvent(highLevelEventMask, &event,
                                     (unsigned long)0, (RgnHandle)NULL))
                    AEProcessAppleEvent(&event);
        }
        //install special handler
        AEEventHandlerUPP specialUPP = NewAEEventHandlerUPP(DoSpecial);
        AEInstallSpecialHandler(keyPreDispatch, specialUPP, False);
        
        if (Gestalt('ICAp', &response) == noErr)
        {
            OSErr err;
            ICInstance icinst;
            ICAttr icattr;
            err = ICStart(&icinst, 'MCRD');
            if (err == noErr)
            {
                Str255 proxystr;
                Boolean useproxy;
                
                long icsize = sizeof(useproxy);
                err = ICGetPref(icinst,  kICUseHTTPProxy, &icattr, &useproxy, &icsize);
                if (err == noErr && useproxy == True)
                {
                    icsize = sizeof(proxystr);
                    err = ICGetPref(icinst, kICHTTPProxyHost ,&icattr, proxystr, &icsize);
                    if (err == noErr)
                    {
                        p2cstr(proxystr);
                        MChttpproxy = strclone((char *)proxystr);
                    }
                }
                ICStop(icinst);
            }
        }
        
        
        MCS_weh = NewEventHandlerUPP(WinEvtHndlr);
        
        // MW-2005-04-04: [[CoreImage]] Load in CoreImage extension
        extern void MCCoreImageRegister(void);
        if (MCmajorosversion >= 0x1040)
            MCCoreImageRegister();
		
        if (!MCnoui)
        {
            setlinebuf(stdout);
            setlinebuf(stderr);
        }        
    }
	virtual void Finalize(void)
    {
        
    }
	
	virtual void Debug(MCStringRef p_string)
    {
        
    }
    
	virtual real64_t GetCurrentTime(void)
    {
        
    }
    
	virtual bool GetVersion(MCStringRef& r_string)
    {
        
    }
	virtual bool GetMachine(MCStringRef& r_string)
    {
        
    }
	virtual MCNameRef GetProcessor(void)
    {
        
    }
	virtual void GetAddress(MCStringRef& r_string)
    {
        
    }
    
	virtual uint32_t GetProcessId(void)
    {
        
    }
	
	virtual void Alarm(real64_t p_when)
    {
        
    }
	virtual void Sleep(real64_t p_when)
    {
        
    }
	
	virtual void SetEnv(MCStringRef name, MCStringRef value)
    {
        
    }
	virtual void GetEnv(MCStringRef name, MCStringRef& value)
    {
        
    }
	
	virtual Boolean CreateFolder(MCStringRef p_path)
    {
#ifdef /* MCS_mkdir_dsk_mac */ LEGACY_SYSTEM
    
	char *newpath = path2utf(MCS_resolvepath(path));
	Boolean done = mkdir(newpath, 0777) == 0;
	delete newpath;
	return done;
#endif /* MCS_mkdir_dsk_mac */
        MCAutoStringRefAsUTF8String t_path;
        if (!t_path.Lock(p_path))
            return False;
        
        if (mkdir(*t_path, 0777) != 0)
            return False;
            
        return True;
    }
    
	virtual Boolean DeleteFolder(MCStringRef p_path)
    {
#ifdef /* MCS_rmdir_dsk_mac */ LEGACY_SYSTEM
    
    char *newpath = path2utf(MCS_resolvepath(path));
    Boolean done = rmdir(newpath) == 0;
    delete newpath;
    return done;
#endif /* MCS_rmdir_dsk_mac */
        MCAutoStringRefAsUTF8String t_path;
        if (!t_path.Lock(p_path))
            return False;
        
        if (rmdir(*t_path) != 0)
            return False;
        
        return True;
    }
    
    /* LEGACY */
    virtual bool DeleteFile(const char *path)
    {
        char *newpath = path2utf(MCS_resolvepath(path));
        Boolean done = remove(newpath) == 0;
        delete newpath;
        return done;
    }
	
	virtual Boolean DeleteFile(MCStringRef p_path)
    {
#ifdef /* MCS_unlink_dsk_mac */ LEGACY_SYSTEM
    char *newpath = path2utf(MCS_resolvepath(path));
    Boolean done = remove(newpath) == 0;
    delete newpath;
    return done;
#endif /* MCS_unlink_dsk_mac */
        MCAutoStringRefAsUTF8String t_path;
        if (!t_path.Lock(p_path))
            return False;
        
        if (remove(*t_path) != 0)
            return False;
        
        return True;
    }
	
	virtual Boolean RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_rename_dsk_mac */ LEGACY_SYSTEM
    //rename a file or directory
    
	char *oldpath = path2utf(MCS_resolvepath(oname));
	char *newpath = path2utf(MCS_resolvepath(nname));
	Boolean done = rename(oldpath, newpath) == 0;
    
	delete oldpath;
	delete newpath;
	return done;
#endif /* MCS_rename_dsk_mac */
        MCAutoStringRefAsUTF8String t_old_name, t_new_name;
        
        if (!t_old_name.Lock(p_old_name) || !t_new_name.Lock(p_new_name))
            return False;
        
        if (rename(*t_old_name, *t_new_name) != 0)
            return False;
        
        return True;
    }
	
    // MW-2007-07-16: [[ Bug 5214 ]] Use rename instead of FSExchangeObjects since
    //   the latter isn't supported on all FS's.
    // MW-2007-12-12: [[ Bug 5674 ]] Unfortunately, just renaming the current stack
    //   causes all Finder meta-data to be lost, so what we will do is first try
    //   to FSExchangeObjects and if that fails, do a rename.
	virtual Boolean BackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_backup_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_backup_dsk_mac */
        bool t_error;
        t_error = false;
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_old_name, t_src_ref);
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
            t_os_error = MCS_pathtoref(p_new_name, t_dst_ref);
            if (t_os_error == noErr)
                FSDeleteObject(&t_dst_ref);
			
            // Get the information to create the file
            t_os_error = MCS_pathtoref_and_leaf(p_new_name, t_dst_parent_ref, t_dst_leaf, t_dst_leaf_length);
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
            t_error = !RenameFileOrFolder(p_old_name, p_new_name);
		
        if (t_error)
            return False;
        
        return True;
    }
    
	virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_unbackup_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_unbackup_dsk_mac */
        bool t_error;
        t_error = false;
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_old_name, t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSRef t_dst_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_new_name, t_dst_ref);
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
            t_error = !MCS_rename(p_old_name, p_new_name);
		
        if (t_error)
            return False;
        
        return True;
    }
	
	virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias)
    {
#ifdef /* MCS_createalias_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_createalias_dsk_mac */
        bool t_error;
        t_error = false;
        
        // Check if the destination exists already and return an error if it does
        if (!t_error)
        {
            FSRef t_dst_ref;
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_alias, t_dst_ref);
            if (t_os_error == noErr)
                return False; // we expect an error
        }
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_target, t_src_ref);
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
            t_os_error = MCS_pathtoref_and_leaf(p_alias, t_dst_parent_ref, t_dst_leaf_name, t_dst_leaf_name_length);
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
		
        if (t_error)
            return False;
        
        return True;
    }
	// NOTE: 'ResolveAlias' returns a standard (not native) path.
	virtual Boolean ResolveAlias(MCStringRef p_target, MCStringRef& r_resolved_path)
    {
#ifdef /* MCS_resolvealias_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_resolvealias_dsk_mac */
        FSRef t_fsref;
        
        OSErr t_os_error;
        t_os_error = MCS_pathtoref(p_target, t_fsref);
        if (t_os_error != noErr)
        {
            if (!MCStringCreateWithCString("file not found", r_resolved_path))
                return False;
            else
                return True;
        }
        
        Boolean t_is_folder;
        Boolean t_is_alias;
        
        t_os_error = FSResolveAliasFile(&t_fsref, TRUE, &t_is_folder, &t_is_alias);
        if (t_os_error != noErr || !t_is_alias) // this always seems to be false
        {
            if (!MCStringCreateWithCString("can't get alias", r_resolved_path))
                return False;
            else
                return True;
        }
        
        if (!MCS_fsref_to_path(t_fsref, r_resolved_path))
        {
            if (!MCStringCreateWithCString("can't get alias path", r_resolved_path))
                return False;
            
            return True;
        }
        
        return True;
    }
	
	virtual void GetCurrentFolder(MCStringRef& r_path)
    {
#ifdef /* MCS_getcurdir_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_getcurdir_dsk_mac */
        bool t_success;
        char namebuf[PATH_MAX + 2];
        if (NULL == getcwd(namebuf, PATH_MAX))
            t_success = false;
        
        MCAutoNativeCharArray t_buffer;
        if (t_success)
            t_success = t_buffer.New(PATH_MAX + 1);
        
        uint4 outlen;
        outlen = PATH_MAX + 1;
        MCS_utf8tonative(namebuf, strlen(namebuf), (char*)t_buffer.Chars(), outlen);
        t_buffer.Shrink(outlen);
        if (t_success)
            t_success = t_buffer.CreateStringAndRelease(r_path);
        
        if (!t_success)
            r_path = MCValueRetain(kMCEmptyString);            
    }
    
    // MW-2006-04-07: Bug 3201 - MCS_resolvepath returns NULL if unable to find a ~<username> folder.
	virtual Boolean SetCurrentFolder(MCStringRef p_path)
    {
#ifdef /* MCS_setcurdir_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_setcurdir_dsk_mac */
        bool t_success;
        MCAutoStringRefAsUTF8String t_utf8_string;
        if (!t_utf8_string.Lock(p_path))
            return False;
        
        if (chdir(*t_utf8_string) != 0)
            return False;
        
        return True;
    }
	
	// NOTE: 'GetStandardFolder' returns a standard (not native) path.
	virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder)
    {
#ifdef /* MCS_getspecialfolder_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_getspecialfolder_dsk_mac */
        uint32_t t_mac_folder = 0;
        OSType t_domain = kOnAppropriateDisk;
        bool t_found_folder = false;
        
        if (MCS_specialfolder_to_mac_folder(MCNameGetString(p_type), t_mac_folder, t_domain))
            t_found_folder = true;
        else if (MCStringGetLength(MCNameGetString(p_type)) == 4)
        {
            t_mac_folder = MCSwapInt32NetworkToHost(*((uint32_t*)MCStringGetBytePtr(MCNameGetString(p_type))));
			
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
            r_folder = MCValueRetain(kMCEmptyString);
            return True;
        }
		
        if (!MCS_fsref_to_path(t_folder_ref, r_folder))
            return False;
        
        return True;
    }
	
	virtual Boolean FileExists(MCStringRef p_path)
    {
#ifdef /* MCS_exists_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_exists_dsk_mac */
        if (MCStringGetLength(p_path) == 0)
            return False;
        
        MCAutoStringRefAsUTF8String t_utf8_path;
        if (!t_utf8_path.Lock(p_path))
            return False;
        
        bool t_found;
        struct stat buf;
        t_found = stat(*t_utf8_path, (struct stat *)&buf) == 0;
        if (t_found)
            t_found = (buf.st_mode & S_IFDIR);
        
        if (!t_found)
            return False;
        
        return True;
    }
    
	virtual Boolean FolderExists(MCStringRef p_path)
    {
        if (MCStringGetLength(p_path) == 0)
            return False;
        
        MCAutoStringRefAsUTF8String t_utf8_path;
        if (!t_utf8_path.Lock(p_path))
            return False;
        
        bool t_found;
        struct stat buf;
        t_found = stat(*t_utf8_path, (struct stat *)&buf) == 0;
        if (t_found)
            t_found = (buf.st_mode & S_IFDIR) == 0;
        
        if (!t_found)
            return False;
        
        return True;
    }
    
	virtual Boolean FileNotAccessible(MCStringRef p_path)
    {
#ifdef /* MCS_noperm_dsk_mac */ LEGACY_SYSTEM
    return False;
#endif /* MCS_noperm_dsk_mac */
        return False;
    }
	
	virtual Boolean ChangePermissions(MCStringRef p_path, uint2 p_mask)
    {
#ifdef /* MCS_chmodMacDsk_dsk_mac */ LEGACY_SYSTEM
    return IO_NORMAL;
#endif /* MCS_chmodMacDsk_dsk_mac */
        return True;
    }
    
	virtual uint2 UMask(uint2 p_mask)
    {
#ifdef /* MCS_umask_dsk_mac */ LEGACY_SYSTEM
	return 0;
#endif /* MCS_umask_dsk_mac */
        return 0;
    }
	
	// NOTE: 'GetTemporaryFileName' returns a standard (not native) path.
	virtual void GetTemporaryFileName(MCStringRef& r_tmp_name)
    {
#ifdef /* MCS_tmpnam_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_tmpnam_dsk_mac */
        bool t_error = false;
        MCAutoStringRef t_temp_file_auto;
        FSRef t_folder_ref;
        char* t_temp_file_chars;
        
        t_temp_file_chars = nil;        
        t_error = !MCStringCreateMutable(0, &t_temp_file_auto);
        
        if (!t_error && FSFindFolder(kOnSystemDisk, kTemporaryFolderType, TRUE, &t_folder_ref) == noErr)
        {
            int t_fd;
            t_error = !MCS_fsref_to_path(t_folder_ref, &t_temp_file_auto);
            
            if (!t_error)
                t_error = MCStringAppendFormat(&t_temp_file_auto, "/tmp.%d.XXXXXXXX", getpid());
            
            t_error = MCMemoryAllocateCopy(MCStringGetCString(*t_temp_file_auto), MCStringGetLength(*t_temp_file_auto) + 1, t_temp_file_chars);
            
            if (!t_error)
            {
                t_fd = mkstemp(t_temp_file_chars);
                t_error = t_fd != -1;
            }
            
            if (!t_error)
            {
                close(t_fd);
                t_error = unlink(t_temp_file_chars) != 0;
            }
        }
        
        if (!t_error)
            t_error = !MCStringCreateWithCString(t_temp_file_chars, r_tmp_name);
        
        if (t_error)
            r_tmp_name = MCValueRetain(kMCEmptyString);
        
        MCMemoryDeallocate(t_temp_file_chars);
    }
    
#define CATALOG_MAX_ENTRIES 16
	virtual bool ListFolderEntries(bool p_files, bool p_detailed, MCListRef& r_list)
    {
#ifdef /* MCS_getentries_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_getentries_dsk_mac_dsk_mac */
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
    
    virtual real8 GetFreeDiskSpace()
    {
#ifdef /* MCS_getfreediskspace_dsk_mac */ LEGACY_SYSTEM
	char t_defaultfolder[PATH_MAX + 1];
	getcwd(t_defaultfolder, PATH_MAX);
	
	FSRef t_defaultfolder_fsref;
	OSErr t_os_error;
	if (t_defaultfolder != NULL)
		t_os_error = FSPathMakeRef((const UInt8 *)t_defaultfolder, &t_defaultfolder_fsref, NULL);
		
	FSCatalogInfo t_catalog_info;
	if (t_os_error == noErr)
		t_os_error = FSGetCatalogInfo(&t_defaultfolder_fsref, kFSCatInfoVolume, &t_catalog_info, NULL, NULL, NULL);
	
	FSVolumeInfo t_volume_info;
	if (t_os_error == noErr)
		t_os_error = FSGetVolumeInfo(t_catalog_info . volume, 0, NULL, kFSVolInfoSizes, &t_volume_info, NULL, NULL);
		
	real8 t_free_space;
	t_free_space = 0.;
	
	// MH: freeBytes is a 64bit unsigned int, I follow previous functionality, and simply cast to real8.
	if (t_os_error == noErr)
		t_free_space = (real8) t_volume_info . freeBytes;
		
	return t_free_space;
#endif /* MCS_getfreediskspace_dsk_mac */
        char t_defaultfolder[PATH_MAX + 1];
        getcwd(t_defaultfolder, PATH_MAX);
        
        FSRef t_defaultfolder_fsref;
        OSErr t_os_error;
        if (t_defaultfolder != NULL)
            t_os_error = FSPathMakeRef((const UInt8 *)t_defaultfolder, &t_defaultfolder_fsref, NULL);
		
        FSCatalogInfo t_catalog_info;
        if (t_os_error == noErr)
            t_os_error = FSGetCatalogInfo(&t_defaultfolder_fsref, kFSCatInfoVolume, &t_catalog_info, NULL, NULL, NULL);
        
        FSVolumeInfo t_volume_info;
        if (t_os_error == noErr)
            t_os_error = FSGetVolumeInfo(t_catalog_info . volume, 0, NULL, kFSVolInfoSizes, &t_volume_info, NULL, NULL);
		
        real8 t_free_space;
        t_free_space = 0.;
        
        // MH: freeBytes is a 64bit unsigned int, I follow previous functionality, and simply cast to real8.
        if (t_os_error == noErr)
            t_free_space = (real8) t_volume_info . freeBytes;
		
        return t_free_space;
    }
    
    virtual Boolean GetDevices(MCStringRef& r_devices)
    {
#ifdef /* MCS_getdevices */ LEGACY_SYSTEM
	MCAutoListRef t_list;
	io_iterator_t SerialPortIterator = NULL;
	mach_port_t masterPort = NULL;
	io_object_t thePort;
	if (FindSerialPortDevices(&SerialPortIterator, &masterPort) != KERN_SUCCESS)
	{
		char *buffer = new char[6 + I2L];
		sprintf(buffer, "error %d", errno);
		MCresult->copysvalue(buffer);
		delete buffer;
		return false;
	}
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	uint2 portCount = 0;

	bool t_success = true;
	if (SerialPortIterator != 0)
	{
		while (t_success && (thePort = IOIteratorNext(SerialPortIterator)) != 0)
		{
			char ioresultbuffer[256];

			MCAutoListRef t_result_list;
			MCAutoStringRef t_result_string;

			t_success = MCListCreateMutable(',', &t_result_list);

			if (t_success)
			{
				getIOKitProp(thePort, kIOTTYDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
				t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//name
			}
			if (t_success)
			{
				getIOKitProp(thePort, kIODialinDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
				t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//TTY file
			}
			if (t_success)
			{
				getIOKitProp(thePort, kIOCalloutDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
				t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//TTY file
			}

			if (t_success)
				t_success = MCListCopyAsStringAndRelease(*t_result_list, &t_result_string);

			if (t_success)
				t_success = MCListAppend(*t_list, *t_result_string);

			IOObjectRelease(thePort);
			portCount++;
		}
		IOObjectRelease(SerialPortIterator);
	}
	
	return t_success && MCListCopy(*t_list, r_list);
#endif /* MCS_getdevices */
        MCAutoListRef t_list;
        io_iterator_t SerialPortIterator = NULL;
        mach_port_t masterPort = NULL;
        io_object_t thePort;
        if (FindSerialPortDevices(&SerialPortIterator, &masterPort) != KERN_SUCCESS)
        {
            char *buffer = new char[6 + I2L];
            sprintf(buffer, "error %d", errno);
            MCresult->copysvalue(buffer);
            delete buffer;
            return false;
        }
        if (!MCListCreateMutable('\n', &t_list))
            return false;
        
        uint2 portCount = 0;
        
        bool t_success = true;
        if (SerialPortIterator != 0)
        {
            while (t_success && (thePort = IOIteratorNext(SerialPortIterator)) != 0)
            {
                char ioresultbuffer[256];
                
                MCAutoListRef t_result_list;
                MCAutoStringRef t_result_string;
                
                t_success = MCListCreateMutable(',', &t_result_list);
                
                if (t_success)
                {
                    getIOKitProp(thePort, kIOTTYDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
                    t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//name
                }
                if (t_success)
                {
                    getIOKitProp(thePort, kIODialinDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
                    t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//TTY file
                }
                if (t_success)
                {
                    getIOKitProp(thePort, kIOCalloutDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
                    t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//TTY file
                }
                
                if (t_success)
                    t_success = MCListCopyAsStringAndRelease(*t_result_list, &t_result_string);
                
                if (t_success)
                    t_success = MCListAppend(*t_list, *t_result_string);
                
                IOObjectRelease(thePort);
                portCount++;
            }
            IOObjectRelease(SerialPortIterator);
        }
        
        if (t_success && MCListCopyAsString(*t_list, r_devices))
            return True;
        
        return False;
    }
    
    virtual Boolean GetDrives(MCStringRef& r_drives)
    {
        r_drives = MCValueRetain(kMCEmptyString);
        return True;
    }
    
	virtual bool PathToNative(MCStringRef p_path, MCStringRef& r_native)
    {
        MCAutoStringRef t_resolved_path;
        
        if (!ResolvePath(p_path, &t_resolved_path))
            return false;
        
        return MCStringCopyAndRelease(*t_resolved_path, r_native);
    }
    
	virtual bool PathFromNative(MCStringRef p_native, MCStringRef& r_path)
    {
        MCAutoStringRef t_resolved_path;
        
        if (!ResolvePath(p_native, &t_resolved_path))
            return false;
        
        return MCStringCopyAndRelease(*t_resolved_path, r_path);
    }
    
	virtual bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path)
    {
#ifdef /* MCS_resolvepath */ LEGACY_SYSTEM
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
#endif /* MCS_resolvepath */
        if (MCStringGetLength(p_path) == 0)
        {
            MCS_getcurdir(r_resolved_path);
            return;
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
            MCS_getcurdir(&t_folder);
            
            MCAutoStringRef t_resolved;
            if (!MCStringMutableCopy(*t_folder, &t_fullpath) ||
                !MCStringAppendChar(*t_fullpath, '/') ||
                !MCStringAppend(*t_fullpath, *t_tilde_path))
                return false;
        }
        else
            t_fullpath = *t_tilde_path;
        
        if (!MCS_is_link(*t_fullpath))
            return MCStringCopy(*t_fullpath, r_resolved_path);
        
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
            
            return MCStringCopy(*t_resolved, r_resolved_path);
        }
        else
            return MCStringCopy(*t_newname, r_resolved_path);
    }
    
	virtual bool ResolveNativePath(MCStringRef p_path, MCStringRef& r_resolved_path)
    {
        
    }
	
	virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map, uin32_t offset)
    {
#ifdef /* MCS_open_dsk_mac */ LEGACY_SYSTEM
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
#endif /* MCS_open_dsk_mac */
        IO_handle handle = NULL;
		//opening regular files
		//set the file type and it's creator. These are 2 global variables
        
        MCAutoStringRefAsUTF8String t_path_utf;
        if (!t_path_utf.Lock(p_path))
            return NULL;
        
		FILE *fptr;
        
        fptr = fopen(*t_path_utf, IO_READ_MODE);
        
        Boolean created = True;
        
        if (fptr != NULL)
        {
            created = False;
            if (p_mode == kMCSystemFileModeRead)
            {
                fclose(fptr);
                fptr = NULL;
            }
        }
        if (fptr == NULL)
        {
            switch(p_mode)
            {
                case kMCSystemFileModeRead:
                    fptr = fopen(*t_path_utf, IO_READ_MODE);
                    break;
                case kMCSystemFileModeUpdate:
                    fptr = fopen(*t_path_utf, IO_UPDATE_MODE);
                    break;
                case kMCSystemFileModeAppend:
                    fptr = fopen(*t_path_utf, IO_APPEND_MODE);
                    break;
                case kMCSystemFileModeWrite:
                    fptr = fopen(*t_path_utf, IO_WRITE_MODE);
                    break;
                default:
                    fptr = NULL;
            }
        }
        
        if (fptr == NULL && p_mode == kMCSystemFileModeRead)
            fptr = fopen(*t_path_utf, IO_CREATE_MODE);
        
        if (fptr != NULL && created)
            MCS_setfiletype(p_path);
        
		if (fptr != NULL)
			handle = new IO_header(fptr, 0, 0, 0, NULL, 0, 0);
        
        if (offset > 0)
            fseek(handle->fptr, offset, SEEK_SET);
        
        return handle;
    }
    
	virtual IO_handle OpenStdFile(uint32_t fd, const char *mode)
    {
        return new IO_header(MCStdioFileHandle::OpenFd(fd, mode), 0);
    }
    
	virtual IO_handle OpenDevice(MCStringRef p_path, intenum_t p_mode, MCStringRef p_control_string)
    {
        IO_handle handle = NULL;
        
        MCAutoStringRefAsUTF8String t_path_utf;
        if (!t_path_utf.Lock(p_path))
            return NULL;
        
		FILE *fptr;
        
        fptr = fopen(*t_path_utf, IO_READ_MODE);
        
        if (fptr != NULL)
        {
            int val;
            val = fcntl(fileno(fptr), F_GETFL, val);
            val |= O_NONBLOCK |  O_NOCTTY;
            fcntl(fileno(fptr), F_SETFL, val);
            configureSerialPort((short)fileno(fptr));
            
            handle = new IO_header(fptr, 0, 0, 0, NULL, 0, 0);
        }
        
        return handle;
    }
	
	virtual void *LoadModule(MCStringRef p_path)
    {
        
    }
	virtual void *ResolveModuleSymbol(void *p_module, MCStringRef p_symbol)
    {
        
    }
	virtual void UnloadModule(void *p_module)
    {
        
    }
	
	virtual bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
    {
        
    }
	virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
    {
        
    }
	
	virtual bool Shell(MCStringRef filename, MCDataRef& r_data, int& r_retcode)
    {
        
    }
    
	virtual char *GetHostName(void)
    {
        
    }
	virtual bool HostNameToAddress(MCStringRef p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
    {
        
    }
	virtual bool AddressToHostName(MCStringRef p_address, MCSystemHostResolveCallback p_callback, void *p_context)
    {
        
    }
    
	virtual uint32_t TextConvert(const void *string, uint32_t string_length, void *buffer, uint32_t buffer_length, uint32_t from_charset, uint32_t to_charset)
    {
        
    }
	virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
    {
        
    }
};

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCDesktopCreateMacSystem(void)
{
	return new MCMacDesktop;
}


////////////////////////////////////////////////////////////////////////////////


void MCS_startprocess_unix(MCNameRef name, MCStringRef doc, Open_mode mode, Boolean elevated)
{
	Boolean noerror = True;
	Boolean reading = mode == OM_READ || mode == OM_UPDATE;
	Boolean writing = mode == OM_APPEND || mode == OM_WRITE || mode == OM_UPDATE;
	MCU_realloc((char **)&MCprocesses, MCnprocesses, MCnprocesses + 1, sizeof(Streamnode));
    
	// Store process information.
	uint2 index = MCnprocesses;
	MCprocesses[MCnprocesses].name = (MCNameRef)MCValueRetain(name);
	MCprocesses[MCnprocesses].mode = mode;
	MCprocesses[MCnprocesses].ihandle = NULL;
	MCprocesses[MCnprocesses].ohandle = NULL;
	MCprocesses[MCnprocesses].sn.highLongOfPSN = 0;
	MCprocesses[MCnprocesses].sn.lowLongOfPSN = 0;
	
	if (!elevated)
	{
		int tochild[2]; // pipe to child
		int toparent[2]; // pipe to parent
		
		// If we are reading, create the pipe to parent.
		// Parent reads, child writes.
		if (reading)
			if (pipe(toparent) != 0)
				noerror = False;
		
		// If we are writing, create the pipe to child.
		// Parent writes, child reads.
		if (noerror && writing)
			if (pipe(tochild) != 0)
			{
				noerror = False;
				if (reading)
				{
					// error, get rid of these fds
					close(toparent[0]);
					close(toparent[1]);
				}
			}
        
		if (noerror)
		{
			// Fork
			if ((MCprocesses[MCnprocesses++].pid = fork()) == 0)
			{
				char *t_name_dup;
				t_name_dup = strdup(MCNameGetCString(name));
				
				// The pid is 0, so here we are in the child process.
				// Construct the argument string to pass to the process..
				char **argv = NULL;
				uint32_t argc = 0;
				startprocess_create_argv(t_name_dup, const_cast<char*>(MCStringGetCString(doc)), argc, argv);
				
				// The parent is reading, so we (we are child) are writing.
				if (reading)
				{
					// Don't need to read
					close(toparent[0]);
					
					// Close the current stdout, and duplicate the out descriptor of toparent to stdout.
					close(1);
					dup(toparent[1]);
					
					// Redirect stderr of this child to toparent-out.
					close(2);
					dup(toparent[1]);
					
					// We no longer need this pipe, so close the output descriptor.
					close(toparent[1]);
				}
				else
				{
					// Not reading, so close stdout and stderr.
					close(1);
					close(2);
				}
				if (writing)
				{
					// Parent is writing, so child needs to read. Close tochild[1], we dont need it.
					close(tochild[1]);
					
					// Attach stdin to tochild[0].
					close(0);
					dup(tochild[0]);
					
					// Close, as we no longer need it.
					close(tochild[0]);
				}
				else // not writing, so close stdin
					close(0);
                
				// Execute a new process in a new process image.
				execvp(argv[0], argv);
				
				// If we get here, an error occurred
				_exit(-1);
			}
			
			// If we get here, we are in the parent process, as the child has exited.
			
			MCS_checkprocesses();
			
			if (reading)
			{
				close(toparent[1]);
				MCS_nodelay(toparent[0]);
				// Store the in handle for the "process".
				MCprocesses[index].ihandle = MCS_dopen(toparent[0], IO_READ_MODE);
			}
			if (writing)
			{
				close(tochild[0]);
				// Store the out handle for the "process".
				MCprocesses[index].ohandle = MCS_dopen(tochild[1], IO_WRITE_MODE);
			}
		}
	}
	else
	{
		OSStatus t_status;
		t_status = noErr;
		
		AuthorizationRef t_auth;
		t_auth = nil;
		if (t_status == noErr)
			t_status = AuthorizationCreate(nil, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &t_auth);
		
		if (t_status == noErr)
		{
			AuthorizationItem t_items =
			{
				kAuthorizationRightExecute, 0,
				NULL, 0
			};
			AuthorizationRights t_rights =
			{
				1, &t_items
			};
			AuthorizationFlags t_flags =
            kAuthorizationFlagDefaults |
            kAuthorizationFlagInteractionAllowed |
            kAuthorizationFlagPreAuthorize |
            kAuthorizationFlagExtendRights;
			t_status = AuthorizationCopyRights(t_auth, &t_rights, nil, t_flags, nil);
		}
		
		FILE *t_stream;
		t_stream = nil;
		if (t_status == noErr)
		{
			char *t_arguments[] =
			{
				"-elevated-slave",
				nil
			};
			t_status = AuthorizationExecuteWithPrivileges(t_auth, MCcmd, kAuthorizationFlagDefaults, t_arguments, &t_stream);
		}
		
		uint32_t t_pid;
		t_pid = 0;
		if (t_status == noErr)
		{
			char *t_name_dup;
			t_name_dup = strdup(MCNameGetCString(name));
			
			// Split the arguments
			uint32_t t_argc;
			char **t_argv;
			startprocess_create_argv(t_name_dup, const_cast<char *>(MCStringGetCString(doc)), t_argc, t_argv);
			startprocess_write_uint32_to_fd(fileno(t_stream), t_argc);
			for(uint32_t i = 0; i < t_argc; i++)
				startprocess_write_cstring_to_fd(fileno(t_stream), t_argv[i]);
			if (!startprocess_read_uint32_from_fd(fileno(t_stream), t_pid))
				t_status = errAuthorizationToolExecuteFailure;
			
			delete t_name_dup;
			delete[] t_argv;
		}
		
		if (t_status == noErr)
		{
			MCprocesses[MCnprocesses++].pid = t_pid;
			MCS_checkprocesses();
			
			if (reading)
			{
				int t_fd;
				t_fd = dup(fileno(t_stream));
				MCS_nodelay(t_fd);
				MCprocesses[index].ihandle = MCS_dopen(t_fd, IO_READ_MODE);
			}
			if (writing)
				MCprocesses[index].ohandle = MCS_dopen(dup(fileno(t_stream)), IO_WRITE_MODE);
			
			noerror = True;
		}
		else
			noerror = False;
		
		if (t_stream != nil)
			fclose(t_stream);
		
		if (t_auth != nil)
			AuthorizationFree(t_auth, kAuthorizationFlagDefaults);
	}
	
	if (!noerror || MCprocesses[index].pid == -1 || MCprocesses[index].pid == 0)
	{
		if (noerror)
			MCprocesses[index].pid = 0;
		MCresult->sets("not opened");
	}
	else
		MCresult->clear(False);
}