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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "object.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"

#include "param.h"
#include "handler.h"
#include "util.h"
#include "globals.h"
#include "ports.cpp"

#include "lnxdc.h"
#include "lnxmplayer.h"

#include <sys/stat.h>
#include <sys/wait.h>

#include <unistd.h>

#define C_PLAYER_CMD "/usr/bin/mplayer"
#define C_PLAYER_ARG "/usr/bin/mplayer -vo x11 -slave -wid %d" 

#define READ 0
#define WRITE 1


void handler(int sig)
{
	pid_t pid;
	pid = wait(NULL);
	fprintf(stderr, "Got a child signal from %d\n", pid);

}



bool FileExists(char * p_filename) 
{
	struct stat64 stFileInfo;
	bool blnReturn;
	int intStat;

	intStat = stat64(p_filename,&stFileInfo);
	if(intStat == 0) 
		return true ;
	else
		return false ;
}


MPlayer::MPlayer(void)
{
	m_playing = false ;
	m_window = DNULL ;
	MClastvideowindow = DNULL ;
	m_osd_level = 0 ;
	m_filename = NULL ;
	m_playing = true ;
	m_cpid = -1 ;
	
	m_duration = UINT32_MAX;
	m_timescale = UINT32_MAX;
	m_loudness = UINT32_MAX;

}


MPlayer::~MPlayer(void)
{
	// If we have an mplayer child process running, tell it to quit.
	if ( m_cpid > 0 )
		quit() ;
	
	shutdown();

	if ( m_filename != NULL)
	{
		free(m_filename);
		m_filename = NULL ;
	}
}

bool MPlayer::shutdown(void)
{
	if ( m_window != DNULL)
	{
		gdk_window_destroy(m_window);
		m_window = DNULL;
		MClastvideowindow = DNULL ;
	}
	
		close(m_pfd_write[WRITE]);
		close(m_pfd_read[READ]);
	
	m_cpid = -1 ;
	m_duration = -1 ;
	m_timescale = -1 ;
	m_loudness = -1;
	m_osd_level = 0 ;
	
	return true;
}	
	

//// Never referenced
//static IO_handle open_fd(int4 fd, const char *mode)
//{
//	IO_handle handle = NULL;
//	FILE *fptr = fdopen(fd, mode);
//	if (fptr != NULL)
//		handle = new (nothrow) IO_header(fptr, NULL, 0, fd, 0);
//	return handle;
//}




bool MPlayer::launch_player(void)
{
	// create a PIPE used to WRITE to the child
	if ( pipe(m_pfd_write) != 0 )
		return false ;
	
	// create a PIPE used to READ from the child
	if ( pipe(m_pfd_read) != 0 ) 
	{
		close(m_pfd_write[READ] );
		close(m_pfd_write[WRITE] );
		return false ;
	}
		
	
	// Install a signal handler to let us know if the the child process exits...
	//signal(SIGCHLD, handler);
    
    x11::Window t_xid = x11::gdk_x11_drawable_get_xid(m_window);

	m_cpid = fork() ;
	if ( m_cpid == -1 )
	{
		close(m_pfd_read[READ]);
		close(m_pfd_read[WRITE]);
		close(m_pfd_write[READ]);
		close(m_pfd_write[WRITE]);
		return false ;
	}
	
	// If this is the child process, then remap STDIN and STDOUT.
	if ( m_cpid == 0 ) 
	{
		// Remap the childs STDIN to the READ end of the parents write pipe!
		close(m_pfd_write[WRITE]);
		close(0);
		dup(m_pfd_write[READ]);
		close(m_pfd_write[READ]); 
		
		// Remap the childs STDERR to the WRITE end of the parents read pipe!
		close(2);
		dup(m_pfd_read[WRITE]);

		// Remap the childs STDOUT to the WRITE end of the parents read pipe!
		close(m_pfd_read[READ]);
		close(1);
		dup(m_pfd_read[WRITE]);
		close(m_pfd_read[WRITE]);
		
		char t_widbuf[20];
		sprintf(t_widbuf, "%lu", t_xid);
		
		// TS-2007-12-17 : Added in the ability to drop down to use the native X11 video driver if Xv is not present (i.e. no hardware
		// 					video support.
		if ( !MCXVideo )
			execl(C_PLAYER_CMD, C_PLAYER_CMD, "-vo", "x11","-sws","3","-zoom", "-nokeepaspect", "-osdlevel","0","-noconsolecontrols", "-msglevel", "all=4", "-nomouseinput", "-quiet", "-slave", "-wid", t_widbuf, (char*)m_filename, (char*)NULL);
		else 
			execl(C_PLAYER_CMD, C_PLAYER_CMD, "-osdlevel","0","-noconsolecontrols", "-msglevel", "all=4", "-nomouseinput", "-quiet", "-slave", "-wid", t_widbuf, (char*)m_filename, (char*)NULL);
		_exit(-1);
	}
	else 
	{
		// Close the ends of the two pipe that as the parent we don't want
		close(m_pfd_write[READ]);
		close(m_pfd_read[WRITE]);
	}
	
	return true;
}




bool MPlayer::init(const char * p_filename, MCStack *p_stack, MCRectangle p_rect )
{
	
	// Are we already running a movie? If we are, then stop.
	if ( m_window != DNULL && m_filename != NULL )
		quit();
	
	GdkWindow* w ;
	if ( p_stack == NULL )
		return false ;
	
	// Locate the window for the stack
	GdkWindow* stack_window = p_stack->getwindow();
	if ( stack_window == DNULL)
		return false ;
	
    GdkWindowAttr t_wa;
    t_wa.colormap = ((MCScreenDC*)MCscreen)->getcmapnative();
    t_wa.x = p_rect.x;
    t_wa.y = p_rect.y;
    t_wa.width = p_rect.width;
    t_wa.height = p_rect.height;
    t_wa.event_mask = 0;
    t_wa.wclass = GDK_INPUT_OUTPUT;
    t_wa.visual = ((MCScreenDC*)MCscreen)->getvisual();
    t_wa.window_type = GDK_WINDOW_CHILD;
    
    w = gdk_window_new(stack_window, &t_wa, GDK_WA_X|GDK_WA_Y|GDK_WA_VISUAL);
	
	if ( w == DNULL )
		return False;
	
	// Set-up our hints so that we have NO window decorations.
    gdk_window_set_decorations(w, GdkWMDecoration(0));
	
	// Ensure the newly created window stays above the stack window & map ( show )
    gdk_window_set_transient_for(w, stack_window);
    gdk_window_show_unraised(w);
	
	m_window = w ;
	MClastvideowindow = w ;
	
	if ( m_filename == NULL ) 
		m_filename = strdup(p_filename) ;
	m_player_rect = p_rect ;
	m_stack = p_stack ;

	if ( !launch_player() )
	{
		gdk_window_hide(m_window);
        gdk_window_destroy(m_window);
		m_window = DNULL;
		MClastvideowindow = DNULL ;
		return false;
	}

	// We will be playing at start by default, so mark it as such
	m_playing = true ;
	// Start the media stopped
	pause();

	return true ;
}



void MPlayer::resize( MCRectangle p_rect)
{
	if ( m_window == DNULL)
		return ;
	
    gdk_window_move_resize(m_window, p_rect.x, p_rect.y, p_rect.width, p_rect.height);
	m_player_rect = p_rect ;
}



void MPlayer::write_command (MCStringRef p_cmd )
{
	if ( m_window == DNULL)
		return ;

	MCAutoStringRefAsCString t_cstring;
	if (t_cstring.Lock(p_cmd))
		write(m_pfd_write[WRITE], *t_cstring, MCStringGetLength(p_cmd));
}

bool MPlayer::read_command(MCStringRef p_ans, MCStringRef& r_ret)
{
	const char *cmd_failed = "Failed to get value of property" ;
	
	if (m_window == DNULL)
		return false;
	
	MCAutoStringRef t_read;
	if (!MCStringCreateMutable(0, &t_read))
		return false;	

	char t_char;
	int t_size = 0;
	while (t_size != -1)
	{
		t_size = read(m_pfd_read[READ], &t_char, 1);
		
		// Read a line into the string
		if (t_char != '\n')
		{
			if (!MCStringAppendChar(*t_read, t_char))
				return false;
			continue;
		}


		if (MCStringIsEqualToCString(*t_read, cmd_failed, kMCStringOptionCompareCaseless))
			return false;

		if (MCStringBeginsWith(*t_read, p_ans, kMCStringOptionCompareCaseless))
		{
			MCRange t_range = MCRangeMake(MCStringGetLength(p_ans), UINDEX_MAX);
			return MCStringCopySubstring(*t_read, t_range, r_ret); 
		}

		if (!MCStringRemove(*t_read, MCRangeMake(0, MCStringGetLength(*t_read))))
			return false;
	}

	return false;
}




void MPlayer::play ( bool p_play )
{		
	if ( m_window != DNULL)
	{
		if ( m_playing != p_play )
		{
			write_command(MCSTR("pause"));
			m_playing = !m_playing ;
		}
	}
	else 
		// if we don't have a window, but do have a filename then repeat play this movie.
		// Additionally, we will only re-start the player if we want to play (p_play==true). In the case that p_play==false
		// then we want to stop playing, so don't re-initialize the movie.
	if (( m_filename != NULL ) && ( p_play ))
	{
		init(m_filename, m_stack, m_player_rect);
		play(p_play);
	}
			
}


void MPlayer::play ( void )
{
	play(true);
}

void MPlayer::pause ( void )
{
	play(false);
}


void MPlayer::seek ( int4 p_amount ) 
{
	MCAutoStringRef t_seek_cmd;
	if (MCStringFormat(&t_seek_cmd, "pausing_keep seek %d 0", p_amount))
		write_command (*t_seek_cmd);
}

void MPlayer::seek(void)
{
	write_command(MCSTR("frame_step"));
}

void MPlayer::osd (uint4 p_level = 0)
{
	MCAutoStringRef t_pause_cmd;
	if (MCStringFormat(&t_pause_cmd, "pausing_keep osd %d", p_level))
		write_command (*t_pause_cmd);
}

void MPlayer::osd(void)
{
	write_command(MCSTR("pausing_keep osd"));
}

void MPlayer::quit(void)
{
	if ( m_cpid > -1 && m_window != DNULL ) 
	{
		int t_status ;
		write_command(MCSTR("quit"));
		// Wait for the child to quit.
		waitpid(m_cpid, &t_status, 0) ;
		shutdown();
		m_window = DNULL ; 
		MClastvideowindow = DNULL ;
		if ( m_filename != NULL)
		{
			delete m_filename ;
			m_filename = NULL ;
		}

	}
}  

void MPlayer::set_property(const char * p_prop, MCPlayerPropertyType p_type, void *p_value) 
{
	MCAutoStringRef t_set_cmd;
	bool t_success = false;
	switch (p_type)
	{
		case kMCPlayerPropertyTypeUInt:
		{
			uint4 t_value;
			t_value = *(uint4 *)p_value;
			t_success = MCStringFormat(&t_set_cmd, "pausing_keep set_property %s %d", p_prop, t_value);
		}			
			break;
		case kMCPlayerPropertyTypeDouble:
		{
			double t_value;
			t_value = *(double *)p_value;
			t_success = MCStringFormat(&t_set_cmd, "pausing_keep set_property %s %f", p_prop, t_value);
		}
			break;
		case kMCPlayerPropertyTypeBool:
		{
			const char *t_value;
			if (*(bool *)p_value)
				t_value = "0";
			else
				t_value = "-1";
			t_success = MCStringFormat(&t_set_cmd, "pausing_keep set_property %s %s", p_prop, t_value);
		}
			break;
		default:
			return;
	}
	
	if (t_success)
		write_command (*t_set_cmd);
}


bool MPlayer::get_property(const char* p_prop, MCPlayerPropertyType p_type, void *r_value) 
{
	if ( m_window == DNULL ) 
		return false;

	MCAutoStringRef t_get_cmd;
	if (!MCStringFormat(&t_get_cmd, "pausing_keep get_property %s", p_prop))
		return false;
	
	write_command (*t_get_cmd);
		
	MCAutoStringRef t_response;
	if (!MCStringFormat(&t_response, "ANS_%s=", p_prop))
		return false;

	MCAutoStringRef t_string_value;
	if (!read_command(*t_response, &t_string_value))
		return false;

	switch (p_type)
	{
		case kMCPlayerPropertyTypeUInt:
		{
			uint4 t_value;
			if (!MCU_strtol(*t_string_value, (int4&)t_value))
				return false;
			*(uint4 *)r_value = t_value;
			return true;
		}		
		case kMCPlayerPropertyTypeDouble:
		{
			double t_value;
			if (!MCU_stor8(*t_string_value, t_value, false))
				return false;
			*(double *)r_value = t_value;
			return true;
		}
		case kMCPlayerPropertyTypeBool:
		default:
			break;
	}
	return false;
}

uint4 MPlayer::getduration(void)
{
	if (m_duration == UINT32_MAX)
	{
		if (!get_property("stream_length", kMCPlayerPropertyTypeUInt, &m_duration))
			m_duration = 0;
	}
	return m_duration;
}


uint4 MPlayer::getcurrenttime(void)
{
	uint4 t_time;
	if (!get_property("stream_pos", kMCPlayerPropertyTypeUInt, &t_time))
		t_time = 0;

	return t_time;
}


uint4 MPlayer::gettimescale(void)
{
	if (m_timescale == UINT32_MAX)
	{
		double t_length ;
		if (get_property("length", kMCPlayerPropertyTypeDouble, &t_length))
		{
			m_timescale = floor(getduration() / t_length);
		}
		else
		{
			m_timescale = 1;
		}
	}
	return m_timescale ;
}


void MPlayer::setspeed(double p_speed)
{
	set_property("speed", kMCPlayerPropertyTypeDouble, &p_speed);
}

void MPlayer::setlooping(bool p_loop)
{
	set_property("looping", kMCPlayerPropertyTypeBool, &p_loop);
}	

void MPlayer::setloudness(uint4 p_volume)
{
	set_property("volume", kMCPlayerPropertyTypeUInt, &p_volume);
}

uint4 MPlayer::getloudness(void)
{
	if (m_loudness == UINT32_MAX)
	{
		if (!get_property("volume", kMCPlayerPropertyTypeUInt, &m_loudness))
			m_loudness = 0;
	}
	return m_loudness;

}
