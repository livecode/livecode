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

/*
	X11Audio : Provides an abstraction between the aclip object and a Linux based audio system.
	This uses EsounD (which comes installed as part of GNOME). However, this interface will be
	simple enough to allow us to plug in any sound engine (OSS, ALSA).
*/

#include "prefix.h"

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

#include "lnxaudio.h"

#include <unistd.h>
#include <esd.h>

extern "C" int initialise_weak_link_esd(void) ;

X11Audio::X11Audio ( void)
{
	m_esd_connection = -1 ;
	m_error = false ;
	m_host = NULL ;	// If this remains as NULL, esd will attempt to open on localhost:ESD_DEFAULT_PORT
}


X11Audio::~X11Audio(void)
{
	if ( m_host != NULL)
		delete m_host ;
}


bool X11Audio::init(char * p_hostname, uint2 p_channels, uint2 p_width)
{
	if ( !isinit() )
	{
		MCuseESD = ( initialise_weak_link_esd() != 0 ) ;
		
		if ( p_hostname != NULL ) 
			m_host = p_hostname ;
		
		m_flags = 0 ;
		if ( p_channels == 1 )
			m_flags |= ESD_MONO ;
		else 
			m_flags |= ESD_STEREO ;
		
		
		if ( p_width == 1 )
			m_flags |= ESD_BITS8 ;
		else 
			m_flags |= ESD_BITS16 ;
		
		// m_esd_connection is no longer used to specify the network sound channel.
		// we still use it as a little flag to say we are all set-up and ready to go.
		m_esd_connection = 1;
		
		return true ;
	}
	return true ;
	
}

int X11Audio::play(int1 * p_sample, uint4 p_samplesize, uint p_rate)
{
	if ( isinit() ) 
	{
		uint t_func = ESD_PLAY | ESD_STREAM | m_flags; 
		
		int chan ;
		chan = esd_play_stream(t_func, p_rate, NULL, NULL);
		
		uint t_bytes_written ;
		t_bytes_written = write(chan, p_sample, p_samplesize);
		
		esd_close(chan);
		
		return t_bytes_written ;
	}

    return 0;
}


void X11Audio::close(void)
{
	if ( isinit() )
	{
		esd_close(m_esd_connection);
	}
}

