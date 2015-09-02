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
//#include "execpt.h"
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
	
	m_duration = -1 ;
	m_timescale = -1 ;
	m_loudness = -1;

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
//		handle = new IO_header(fptr, NULL, 0, fd, 0);
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



void MPlayer::write_command ( char * p_cmd )
{
	if ( m_window == DNULL)
		return ;
	char *t_buffer;
	t_buffer = (char *)malloc(strlen(p_cmd) + 2);
	sprintf(t_buffer, "%s\n", p_cmd);
	write(m_pfd_write[WRITE], t_buffer, strlen(t_buffer));
	free(t_buffer);
}

char * MPlayer::read_command( char * p_ans )
{
	const char * cmd_failed = "Failed to get value of property" ;
	
	if ( m_window == DNULL)
		return NULL;
	char * t_ret, * t_ptr ;
	t_ret = (char*)malloc(2048) ;
	memset(t_ret, 0, 2048);
	t_ptr = t_ret ;
	char t_char ;
	bool t_found = false ;
	*t_ptr = '\0' ;
	int t_size = read(m_pfd_read[READ], &t_char, 1 ) ;
	while (!t_found && t_size != -1)
	{
		while ( t_size != -1 && t_char != '\n' )
		{
			*t_ptr=t_char ;
			t_ptr++;
			int t_size = read(m_pfd_read[READ], &t_char, 1 ) ;
		}
		
		if ( strcasestr(t_ret, cmd_failed) != NULL)
		{
			t_ret = NULL ;
			break ;
		}
		
		
		if ( strstr(t_ret, p_ans) != NULL)
		{
			t_ret += strlen(p_ans) ;
			t_found = true ;
		}
		else 
		{
			t_char = '\0' ;
			memset(t_ret, 0, 2048);
			t_ptr = t_ret ;
			int t_size = read(m_pfd_read[READ], &t_char, 1 ) ;
		}
	}

	return t_ret;
}




void MPlayer::play ( bool p_play )
{		
	if ( m_window != DNULL)
	{
		if ( m_playing != p_play )
		{
			write_command("pause");
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
	char buffer[1024] ;
	sprintf(buffer, "pausing_keep seek %d 0", p_amount);
	write_command ( buffer );
}

void MPlayer::seek(void)
{
	write_command("frame_step");
}

void MPlayer::osd ( uint4 p_level = 0 )
{
	char buffer[1024] ;
	sprintf(buffer, "pausing_keep osd %d", p_level );
	write_command ( buffer );
}

void MPlayer::osd(void)
{
	write_command("pausing_keep osd");
}

void MPlayer::quit(void)
{
	if ( m_cpid > -1 && m_window != DNULL ) 
	{
		int t_status ;
		write_command("quit");
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


void MPlayer::set_property ( char * p_prop, char * p_value ) 
{
	char t_buffer[1024] ;
	sprintf(t_buffer, "pausing_keep set_property %s %s", p_prop, p_value ) ;
	write_command( t_buffer ) ;
}


char * MPlayer::get_property ( char * p_prop ) 
{
	if ( m_window == DNULL ) 
		return "-1";
		
	char t_response[1024] ;
	char t_question[1024];
	sprintf(t_question, "pausing_keep get_property %s", p_prop);
	sprintf(t_response, "ANS_%s=", p_prop);
	write_command(t_question);
	return (read_command(t_response));
}


uint4 MPlayer::getduration(void)
{
	if ( m_duration == -1 )
	{
		char * t_ret ;
		t_ret = get_property("stream_length") ;
		if ( t_ret != NULL )
 			m_duration = atoi(t_ret);
		else 
			m_duration = 0 ;
	}
	return m_duration ;
}


uint4 MPlayer::getcurrenttime(void)
{
	char * t_ret ;
	t_ret = get_property("stream_pos") ;
	if ( t_ret != NULL )
 		return atoi(t_ret);
	else 
		return 0 ;
	
}


uint4 MPlayer::gettimescale(void)
{
	if ( m_timescale == -1 )
	{
		double t_length ;
		char * t_ret ;
		t_ret = get_property("length") ;
		if ( t_ret != NULL )
		{
			t_length = atof(t_ret);
			m_timescale = floor(getduration() / t_length) ;
		}
		else 
			m_timescale = 1 ;
	}
	return m_timescale ;
}


void MPlayer::setspeed(double p_speed)
{
	char t_buffer[100];
	sprintf(t_buffer, "%f", p_speed);
	set_property("speed", t_buffer);
}

void MPlayer::setlooping(bool p_loop)
{
	set_property("looping", (char*)(p_loop ? "0" : "-1") ) ;
}	

void MPlayer::setloudness(uint4 p_volume )
{
	char t_buffer[4] ;
	sprintf(t_buffer,"%d", p_volume);
	set_property("volume", t_buffer);
}

uint4 MPlayer::getloudness(void)
{
	if ( m_loudness == -1 )
	{
		char * t_ret ;
		t_ret = get_property("volume") ;
		if ( t_ret != NULL)
			m_loudness = atoi(t_ret);
		else 
			m_loudness = 0 ;
	}
	return m_loudness;

}
