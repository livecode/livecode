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

////////////////////////////////////////////////////////////////////////////////
//
//  Private Linux Source File:
//    unixspec.cpp
//
//  Description:
//    This file contains the implementations of the platform abstraction methods
//    defined in osspec.h and elsewhere for Linux.
//
//  Changes:
//    2009-06-25 MW Added implementation of MCS_fakeopencustom and related
//                  changes to stream handling methods.
//    2009-06-30 MW Refactored fake custom implementation to mcio.cpp and added
//                  new hooks (tell, seek_cur).
//
////////////////////////////////////////////////////////////////////////////////

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "object.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"
//#include "execpt.h"
#include "param.h"
#include "handler.h"
#include "util.h"
#include "globals.h"
#include "ports.cpp"
#include "socket.h"
#include "mcssl.h"
#include "securemode.h"
#include "mode.h"
#include "player.h"
#include "osspec.h"

#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/dir.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <termios.h>
#include <langinfo.h>
#include <locale.h>
#include <pwd.h>

#include <libgnome/gnome-url.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnome-init.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

#ifdef LEGACY_EXEC

// This is in here so we do not need GLIBC2.4
extern "C" void __attribute__ ((noreturn)) __stack_chk_fail (void)
{
}




static void parseSerialControlStr(char *set
                                  ,  struct termios *theTermios);
static void configureSerialPort(int sRefNum);

static Boolean alarmpending;

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
		fprintf(stderr, "%s exiting on signal %d\n", MCcmd, sig);
		MCS_killall();
		// abort() instead of exit(-1) so that we get core dumps.
		abort();
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
					XUnmapWindow (MCdpy, MClastvideowindow);
					XDestroyWindow (MCdpy, MClastvideowindow);
					MClastvideowindow = DNULL ;
				}
				else
				{
					MCS_checkprocesses();
				}
			}
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

static IO_handle MCS_dopen(int4 fd, const char *mode)
{
	IO_handle handle = NULL;
	FILE *fptr = fdopen(fd, mode);
	if (fptr != NULL)
		handle = new IO_header(fptr, NULL, 0, fd, 0);
	return handle;
}

void MCS_init()
{
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
}

void MCS_shutdown()
{}

void MCS_seterrno(int value)
{
	errno = value;
}

int MCS_geterrno()
{
	return errno;
}

void MCS_alarm(real8 secs)
{
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
}

void MCS_startprocess(char *name, char *doc, Open_mode mode, Boolean elevated)
{
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
                // The executable name should be argv[0].
                execvp(argv[0], argv);
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
}

void MCS_checkprocesses()
{	
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
	
}

void MCS_closeprocess(uint2 index)
{
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
}

void MCS_kill(int4 pid, int4 sig)
{
	kill(pid, sig);
}

void MCS_killall()
{
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
}

// MW-2005-02-22: Make global for now so opensslsocket.cpp can access it
real8 curtime;
real8 MCS_time()
{
	struct timezone tz;
	struct timeval tv;

	gettimeofday(&tv, &tz);
	curtime = tv.tv_sec + (real8)tv.tv_usec / 1000000.0;
	return curtime;
}

void MCS_reset_time()
{}

void MCS_sleep(real8 duration)
{
	Boolean wasalarm = alarmpending;
	if (alarmpending)
		MCS_alarm(0.0);

	struct timeval timeoutval;
	timeoutval.tv_sec = (long)duration;
	timeoutval.tv_usec = (long)((duration - floor(duration)) * 1000000.0);
	select(0, NULL, NULL, NULL, &timeoutval);

	if (wasalarm)
		MCS_alarm(CHECK_INTERVAL);
}

char *MCS_getenv(const char *name)
{
	return getenv(name);
}

void MCS_setenv(const char *name, const char *value)
{
#ifdef NOSETENV
	char *dptr = new char[strlen(name) + strlen(value) + 2];
	sprintf(dptr, "%s=%s", name, value);
	putenv(dptr);
#else

	setenv(name, value, True);
#endif
}

void MCS_unsetenv(const char *name)
{
#ifndef NOSETENV
	unsetenv(name);
#endif
}

int4 MCS_rawopen(const char *path, int flags)
{
	char *newpath = MCS_resolvepath(path);
	int4 fd = open(newpath, flags);
	delete newpath;
	return fd;
}

int4 MCS_rawclose(int4 fd)
{
	return close(fd);
}

Boolean MCS_rename(const char *oname, const char *nname)
{
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
}

Boolean MCS_backup(const char *oname, const char *nname)
{
	return MCS_rename(oname, nname);
}

Boolean MCS_unbackup(const char *oname, const char *nname)
{
	return MCS_rename(oname, nname);
}

Boolean MCS_unlink(const char *path)
{
	char *newpath = MCS_resolvepath(path);
	Boolean done = unlink(newpath) == 0;
	delete newpath;
	return done;
}

const char *MCS_tmpnam()
{
	return tmpnam(NULL);
}

char *MCS_resolvepath(const char *path)
{
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
}

char *MCS_get_canonical_path(const char *p_path)
{
	char *t_path = NULL;

	t_path = MCS_resolvepath(p_path);
	MCU_fix_path(t_path);

	return t_path;
}

char *MCS_getcurdir()
{
	char *dptr = new char[PATH_MAX + 2];
	getcwd(dptr, PATH_MAX);
	return dptr;
}

Boolean MCS_setcurdir(const char *path)
{
	char *newpath = MCS_resolvepath(path);
	Boolean done = chdir(newpath) == 0;
	delete newpath;
	return done;
}

#define ENTRIES_CHUNK 4096

#ifdef LEGACY_EXEC
void MCS_getentries(char **dptr, bool files, bool islong)
{
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
}
#endif

#ifdef LEGACY_EXEC
void MCS_getentries(MCExecPoint& p_exec, bool p_files, bool p_islong)
{
	char *t_buffer;
	MCS_getentries(&t_buffer, p_files, p_islong);
	p_exec . copysvalue(t_buffer, strlen(t_buffer));
	delete t_buffer;
}
#endif

#define DNS_SCRIPT "repeat for each line l in url \"binfile:/etc/resolv.conf\";\
if word 1 of l is \"nameserver\" then put word 2 of l & cr after it; end repeat;\
delete last char of it; return it"

#ifdef LEGACY_EXEC
void MCS_getDNSservers(MCExecPoint &ep)
{
	ep . clear();
	MCresult->store(ep, False);
	MCdefaultstackptr->domess(DNS_SCRIPT);
	MCresult->fetch(ep);
}

Boolean MCS_getdevices(MCExecPoint &ep)
{
	ep.clear();
	return True;
}

Boolean MCS_getdrives(MCExecPoint &ep)
{
	ep.clear();
	return True;
}
#endif

Boolean MCS_noperm(const char *path)
{
	struct stat64 buf;
	if (stat64(path, &buf))
		return False;
	if (S_ISDIR(buf.st_mode))
		return True;
	if (!(buf.st_mode & S_IWUSR))
		return True;
	return False;
}

Boolean MCS_exists(const char *path, Boolean file)
{
	// MM-2011-08-24: [[ Bug 9691 ]] Updated to use stat64 so no longer fails on files larger than 2GB
	char *newpath = MCS_resolvepath(path);
	struct stat64 buf;

	Boolean found = stat64(newpath, &buf) == 0;
	if (found)
		if (file)
		{
			if (S_ISDIR(buf.st_mode))
				found = False;
		}
		else
			if (!S_ISDIR(buf.st_mode))
				found = False;
	delete newpath;
	return found;
}

int64_t MCS_fsize(IO_handle stream)
{
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_fsize(stream);

	struct stat64 buf;
	if (stream->fptr == NULL)
		return stream->len;
		
	int fd = fileno(stream->fptr);
	
	if (fstat64(fd, &buf))
		return 0;
	return buf.st_size;
}

Boolean MCS_nodelay(int4 fd)
{
	return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & O_APPEND | O_NONBLOCK)
	       >= 0;
}

IO_handle MCS_open(const char *path, const char *mode,
                   Boolean map, Boolean driver, uint4 offset)
{
	char *newpath = MCS_resolvepath(path);
	IO_handle handle = NULL;
	
#ifndef NOMMAP
	if (map && MCmmap && !driver && strequal(mode, IO_READ_MODE))
	{
		int fd = open(newpath, O_RDONLY);
		struct stat64 buf;
		if (fd != -1 && !fstat64(fd, &buf))
		{
			// The length of a file could be > 32-bit, so we have to check that
			// the file size fits into a 32-bit integer as that is what the
			// IO_header form we use supports.
			off_t len = buf.st_size - offset;
			if (len != 0 && len < UINT32_MAX)
			{
				char *buffer = (char *)mmap(NULL, len, PROT_READ, MAP_SHARED,
				                            fd, offset);
											
				// MW-2013-05-02: [[ x64 ]] Make sure we use the MAP_FAILED constant
				//   rather than '-1'.
				if (buffer != MAP_FAILED)
				{
					delete newpath;
					handle = new IO_header(NULL, buffer, (uint4)len, fd, 0);
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
}

IO_stat MCS_close(IO_handle &stream)
{
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
}


IO_stat MCS_shellread(int fd, char *&buffer, uint4 &buffersize, uint4 &size)
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
			if (MCscreen->wait(READ_INTERVAL, False, True))
			{
				MCshellfd = -1;
				return IO_ERROR;
			}
		}
		else
			size += amount;
	}
	MCshellfd = -1;
	return IO_NORMAL;
}

IO_stat MCS_runcmd(MCExecPoint &ep)
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
				return IO_ERROR;
			}
			MCS_checkprocesses();
			close(tochild[0]);
			write(tochild[1], ep.getsvalue().getstring(),
			      ep.getsvalue().getlength());
			write(tochild[1], "\n", 1);
			close(tochild[1]);
			close(toparent[1]);
			MCS_nodelay(toparent[0]);
		}
		else
		{
			MCeerror->add(EE_SYSTEM_FUNCTION, 0, 0, "pipe");
			MCeerror->add(EE_SYSTEM_CODE, 0, 0, errno);
			MCeerror->add(EE_SYSTEM_MESSAGE, 0, 0, strerror(errno));
			close(tochild[0]);
			close(tochild[1]);
			return IO_ERROR;
		}
	}
	else
	{
		MCeerror->add(EE_SYSTEM_FUNCTION, 0, 0, "pipe");
		MCeerror->add(EE_SYSTEM_CODE, 0, 0, errno);
		MCeerror->add(EE_SYSTEM_MESSAGE, 0, 0, strerror(errno));
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

}

uint2 MCS_umask(uint2 mask)
{
	return umask(mask);
}

IO_stat MCS_chmod(const char *path, uint2 mask)
{
	if (chmod(path, mask) != 0)
		return IO_ERROR;
	return IO_NORMAL;
}

int4 MCS_getumask()
{
	int4 oldmask = umask(0);
	umask(oldmask);
	return oldmask;
}

void MCS_setumask(int4 newmask)
{
	umask(newmask);
}

Boolean MCS_mkdir(const char *path)
{
	char *newpath = MCS_resolvepath(path);
	Boolean done = mkdir(path, 0777) == 0;
	delete newpath;
	return done;
}

Boolean MCS_rmdir(const char *path)
{
	char *newpath = MCS_resolvepath(path);
	Boolean done = rmdir(path) == 0;
	delete newpath;
	return done;
}

IO_stat MCS_trunc(IO_handle stream)
{
	if (ftruncate(fileno(stream->fptr), ftell(stream->fptr)))
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_flush(IO_handle stream)
{
	if (stream->fptr != NULL)
		if (fflush(stream->fptr))
			return IO_ERROR;

	return IO_NORMAL;
}

IO_stat MCS_sync(IO_handle stream)
{
	if (stream->fptr != NULL)
	{
		int64_t pos = ftello64(stream->fptr);
		if (fseeko64(stream->fptr, pos, SEEK_SET) != 0)
			return IO_ERROR;
	}
	return IO_NORMAL;
}

Boolean MCS_eof(IO_handle stream)
{
	if (stream->fptr == NULL)
		return (uint4)(stream->ioptr - stream->buffer) == stream->len;
	return feof(stream->fptr);
}

IO_stat MCS_seek_cur(IO_handle stream, int64_t offset)
{
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
}

IO_stat MCS_seek_set(IO_handle stream, int64_t offset)
{
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_seek_set(stream, offset);

	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->buffer + offset);
	else
		if (fseeko64(stream->fptr, offset, SEEK_SET) != 0)
			return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_end(IO_handle stream, int64_t offset)
{
	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->buffer + stream->len + offset);
	else
		if (fseeko64(stream->fptr, offset, SEEK_END) != 0)
			return IO_ERROR;
	return IO_NORMAL;
}

int64_t MCS_tell(IO_handle stream)
{
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_tell(stream);

	if (stream->fptr != NULL)
		return ftello64(stream->fptr);
	else
		return stream->ioptr - stream->buffer;
}

IO_stat MCS_putback(char c, IO_handle stream)
{
	if (stream -> fptr == NULL)
		return MCS_seek_cur(stream, -1);
	
	if (ungetc(c, stream -> fptr) != c)
		return IO_ERROR;
		
	return IO_NORMAL;
}

IO_stat MCS_read(void *ptr, uint4 size, uint4 &n, IO_handle stream)
{
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
}

IO_stat MCS_write(const void *ptr, uint4 size, uint4 n, IO_handle stream)
{
	if ((stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
		return MCU_dofakewrite(stream -> buffer, stream -> len, ptr, size, n);

	if (fwrite(ptr, size, n, stream->fptr) != n)
		return IO_ERROR;
	return IO_NORMAL;
}

uint4 MCS_getpid()
{
	return getpid();
}

static struct utsname u;

const char *MCS_getaddress()
{
	static char *buffer;
	uname(&u);
	if (buffer == NULL)
		buffer = new char[strlen(u.nodename) + strlen(MCcmd) + 4];
	sprintf(buffer, "%s:%s", u.nodename, MCcmd);
	return buffer;
}

const char *MCS_getmachine()
{
	uname(&u);
	return u.machine;
}

// MW-2013-05-02: [[ x64 ]] If 64-bit then return x86_64, else we must be
//   32-bit Intel so x86.
const char *MCS_getprocessor()
{
#ifdef __LP64__
	return "x86_64";
#else
	return "x86";
#endif
}

const char *MCS_getsystemversion()
{
	static char *buffer;
	uname(&u);
	if (buffer == NULL)
		buffer = new char[strlen(u.sysname) + strlen(u.release) + 2];
	sprintf(buffer, "%s %s", u.sysname, u.release);
	return buffer;
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
	char *newpath = MCS_resolvepath(tpath);
	delete tpath;
	int fd = open(newpath, O_RDONLY);
	ep.clear();
	delete newpath;
	if (fd == -1)
		MCresult->sets("can't open file");
	else
	{
		struct stat64 buf;
		uint4 t_readsize = 0 ;		
		bool t_error ;

		t_error = fstat64(fd, &buf);
		
		//TS-2008-19-06 : [[BUG 6325 - Cannot read /proc/<pid>/status file with the URL form.

		// Special case where buf.st_size == 0 . This can occur if we are trying to read in some special file 
		// such as those in /proc
		if ( !t_error && buf.st_size == 0 )
		{
			char * t_buffer;
			uint2 t_lastread;
			t_buffer = (char*)malloc(1);
			while( (!t_error ) && (t_buffer != NULL) && ((t_lastread = read(fd, ((char*)t_buffer+t_readsize), 1)) > 0))
			{
				t_readsize += t_lastread;
				t_buffer = (char*)realloc(t_buffer, t_readsize+1);
			}
			ep.setbuffer(t_buffer, t_readsize);
			buf.st_size = t_readsize ;
		}
		
		// Check to see if there was an error.
		// tReadSize == -1 can only happen if buf.st_size was zero but we could not read
		// a single byte from the file --> this implies that it was a zero byte size file.
		t_error |= ( t_readsize == -1 ) ;
		
		if ( t_readsize <= 0 && (t_error || ep.getbuffer(buf.st_size) == NULL
				|| read(fd, ep.getbuffer(buf.st_size), buf.st_size) != buf.st_size)) 
		{
			ep.clear();
			MCresult->sets("error reading file");
			t_error = true ;
		}

		
		if ( !t_error)
		{
			ep.setlength(buf.st_size);
			MCresult->clear(False);

			// MW-2007-09-17: [[ Bug 4587 ]] Linux should do end-of-line conversion
			//   when loading a text file too - this ensures it converts Mac and
			//   Windows style line endings correctly.
			if (!binary)
				ep.texttobinary();
		}

		close(fd);
	}
}

void MCS_loadresfile(MCExecPoint &ep)
{
	ep.clear();
	MCresult->sets("error writing file");
}

void MCS_savefile(const MCString &fname, MCExecPoint &data, Boolean b)
{
	if (!MCSecureModeCanAccessDisk())
	{
		MCresult->sets("can't open file");
		return;
	}

	char *tpath = fname.clone();
	char *newpath = MCS_resolvepath(tpath);
	delete tpath;
	int fd = open(newpath, O_CREAT | O_TRUNC | O_WRONLY, 0777);
	delete newpath;
	if (fd == -1)
		MCresult->sets("can't open file");
	else
	{
		MCString s = data.getsvalue();
		if ((uint4)write(fd, s.getstring(), s.getlength())!= s.getlength())
			MCresult->sets("error writing file");
		else
			MCresult->clear(False);
		close(fd);
	}
}

void MCS_saveresfile(const MCString &s, const MCString data)
{
	MCresult->sets("not supported");
}

IO_handle MCS_fakeopen(const MCString &data)
{
	return new IO_header(NULL, (char *)data.getstring(), data.getlength(),
	                     0, IO_FAKE);
}

IO_handle MCS_fakeopenwrite(void)
{
	return new IO_header(NULL, NULL, 0,
	                     0, IO_FAKEWRITE);
}

IO_handle MCS_fakeopencustom(MCFakeOpenCallbacks *p_callbacks, void *p_state)
{
	// MW-2015-05-03: [[ x64 ]] Use 'uintptr_t' as cast for callbacks (param is actually
	//   size_t, but uintptr_t is the 'correct' type for this usage - there should really
	//   be a different constructor for this case!)
	return new IO_header(NULL, (char *)p_state, (uintptr_t)p_callbacks, 0, IO_FAKECUSTOM);
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

uint4 MCS_faketell(IO_handle stream)
{
	return stream -> len;
}

bool MCS_isfake(IO_handle stream)
{
	return (stream -> flags & IO_FAKEWRITE) != 0;
}

void MCS_fakewriteat(IO_handle stream, uint4 p_pos, const void *p_buffer, uint4 p_size)
{
	memcpy(stream -> buffer + p_pos, p_buffer, p_size);
}


void MCS_delete_registry(const char *key, MCExecPoint &dest)
{
	MCresult->sets("not supported");
	dest.setboolean(False);
}

void MCS_query_registry(MCExecPoint &dest, const char** type)
{
	MCresult->sets("not supported");
	dest.clear();
}

void MCS_set_registry(const char *key, MCExecPoint &dest, char *type)
{
	MCresult->sets("not supported");
	dest.setboolean(False);
}

void MCS_list_registry(MCExecPoint &dest)
{
	MCresult->sets("not supported");
	dest.setboolean(False);
}

double MCS_getfreediskspace(void)
{
	return 1.0;
}

void MCS_exec_command ( char * command ) 
{
	MCExecPoint ep;
	sprintf(ep.getbuffer(strlen(command)), command);
	ep.setstrlen();
	if (MCS_runcmd(ep) != IO_NORMAL)
	{
		MCeerror->add(EE_PRINT_ERROR, 0, 0);
	}
	else
		MCresult->sets(ep.getsvalue());
	
}

void MCS_launch_document(char *p_document)
{
	
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
}


#define LAUNCH_URL_SCRIPT		"put \"%s\" into tFilename; " \
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


void MCS_launch_url(char *p_document)
{
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
}

MCSysModuleHandle MCS_loadmodule(const char *p_filename)
{
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
}

void *MCS_resolvemodulesymbol(MCSysModuleHandle p_module, const char *p_symbol)
{
	return ( dlsym ( p_module, p_symbol ) ) ;
}

void MCS_unloadmodule(MCSysModuleHandle p_module)
{
	dlclose ( p_module ) ;
}

Boolean MCS_poll(real8 delay, int fd)
{
	Boolean readinput = False;
	int4 n;
	uint2 i;
	Boolean wasalarm = alarmpending;
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
		return False;
	if (MCshellfd != -1 && FD_ISSET(MCshellfd, &rmaskfd))
		return True;
	if (MCinputfd != -1 && FD_ISSET(MCinputfd, &rmaskfd))
		readinput = True;
    
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
}

void MCS_send(const MCString &message, const char *program,
              const char *eventtype, Boolean reply)
{
	MCresult->sets("not supported");
}

void MCS_reply(const MCString &message, const char *keyword, Boolean error)
{
	MCresult->sets("not supported");
}

char *MCS_request_ae(const MCString &message, uint2 ae)
{
	MCresult->sets("not supported");
	return NULL;
}

char *MCS_request_program(const MCString &message, const char *program)
{
	MCresult->sets("not supported");
	return NULL;
}

void MCS_copyresourcefork(const char *source, const char *dest)
{}

void MCS_copyresource(const char *source, const char *dest,
                      const char *type, const char *name,
                      const char *newid)
{
	MCresult->sets("not supported");
}

void MCS_deleteresource(const char *source, const char *type,
                        const char *name)
{
	MCresult->sets("not supported");
}


void MCS_getresource(const char *source, const char *type,
                     const char *name, MCExecPoint &ep)
{
	ep.clear();
	MCresult->sets("not supported");
}

char *MCS_getresources(const char *source, const char *type)
{
	MCresult->sets("not supported");
	return NULL;
}

void MCS_setresource(const char *source, const char *type,
                     const char *name, const char *id, const char *flags,
                     const MCString &s)
{
	MCresult->sets("not supported");
}

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
		MCLog("Error setting terminous attributes", nil);
	}
	return;
}

void MCS_getspecialfolder(MCExecPoint &ep)
{
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
    // SN-2014-07-30: [[ Bug 13029 ]] specialfolderpath added for Linux
    else if (ep.getsvalue() == "engine")
    {
        extern char *MCcmd;
        char* t_folder;
        t_folder = strndup(MCcmd, strrchr(MCcmd, '/') - MCcmd);
        ep.setcstring(t_folder);
    }
	else
	{
		MCresult->sets("not supported");
		ep.clear();
	}

	delete c_dir ;
}

void MCS_longfilepath(MCExecPoint &ep)
{}

void MCS_shortfilepath(MCExecPoint &ep)
{}

Boolean MCS_createalias(char *srcpath, char *dstpath)
{
	char *source = MCS_resolvepath(srcpath);
	char *dest = MCS_resolvepath(dstpath);
	Boolean done = symlink(source,dest) == 0;
	delete source;
	delete dest;
	return done;
}


void MCS_resolvealias(MCExecPoint &ep)
{
	char *tpath = ep.getsvalue().clone();
	char *dest = MCS_resolvepath(tpath);
	delete tpath;
	ep.copysvalue(dest, strlen(dest));
	delete dest;
}

void MCS_alternatelanguages(MCExecPoint &ep)
{
	ep.clear();
}

void MCS_doalternatelanguage(MCString &s, const char *langname)
{
	MCresult->sets("alternate language not found");
}

void abbrevdatefmt(char *sptr)
{
	while (*sptr)
	{
		if (*sptr == '%')
		{
			if (*++sptr == '#')
				sptr++; //skip
			if (*sptr == 'A' || *sptr == 'B')
				*sptr = MCS_tolower(*sptr);
		}
		sptr++;
	}
}

void MCS_localedateinfo(uint2 which, char *dest)
{
#ifndef NOLANG
	strcpy(dest, nl_langinfo(which == P_SHORT? D_FMT: D_T_FMT));
	if (which == P_ABBREVIATE)
		abbrevdatefmt(dest);
#else

	dest[0] = '\0';
#endif
}

Boolean MCS_isleadbyte(uint1 charset, char *s)
{
	return False;
}


// MW-2005-02-08: Implementation of multibyte conversion routines.
//   These are naive at the moment - only providing conversion to and from
//   ISO8859-1.
void MCS_multibytetounicode(const char *p_mbstring, uint4 p_mblength,
                            char *p_buffer, uint4 p_capacity, uint4& r_used, uint1 p_mbcharset)
{
	if (p_capacity == 0)
		r_used = p_mblength * 2;
	else
	{
		uint4 i;
		for(i = 0; i < MCU_min(p_mblength, p_capacity / 2); ++i)
			((uint2 *)p_buffer)[i] = (unsigned char)p_mbstring[i];
		r_used = i * 2;
	}
}

void MCS_unicodetomultibyte(const char *p_ucstring, uint4 p_uclength,
                            char *p_buffer, uint4 p_capacity, uint4& r_used, uint1 p_mbcharset)
{
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
}

bool MCS_processtypeisforeground(void)
{
	return true;
}

bool MCS_changeprocesstype(bool to_foreground)
{
	if (!to_foreground)
		return false;
	return true;
}

bool MCS_isatty(int fd)
{
	return isatty(fd) != 0;
}

bool MCS_isnan(double v)
{
	return isnan(v);
}

uint32_t MCS_getsyserror(void)
{
	return errno;
}

bool MCS_mcisendstring(const char *command, char buffer[256])
{
	strcpy(buffer, "not supported");
	return true;
}

void MCS_system_alert(const char *p_title, const char *p_message)
	{
	fprintf(stderr, "%s", p_message);
	}

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
#endif
