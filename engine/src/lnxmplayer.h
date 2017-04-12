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


#ifndef MPLAYER_H
#define MPLAYER_H

enum MCPlayerPropertyType
{
	kMCPlayerPropertyTypeBool,
	kMCPlayerPropertyTypeUInt,
	kMCPlayerPropertyTypeDouble,
};

class MPlayer
{
	public:
		MPlayer(void) ;
		~MPlayer(void) ;
	
		// Initilize the mplayer process and load in the specified media file.
		// This also creates a new child window of the given stack
        bool init (const char *p_filename, MCStack *p_stack, MCRectangle p_rect );
		bool shutdown ( void ) ;
	
		// Basic commands for manipulating the media
		void play ( bool p_play );
		void play ( void ) ;
		void pause ( void ); 
		void seek ( int4 p_amount ) ;
		void seek (void);
		void osd ( uint4 p_level );
		void osd ( void );
		void quit (void) ;
		void setspeed(double p_speed);
		void setlooping(bool p_loop);
		void setloudness(uint4 p_volume );

		// Access methods for getting data back about the media state etc.
	 	bool isplaying(void) { return m_playing; };
		bool isrunning(void) { return (m_window != DNULL); }; 
		bool ispaused (void) { return !m_playing; };
		Window getwindow (void) { return m_window ; } ;
		MCRectangle getrect (void) { return m_player_rect; };
		pid_t getpid(void) { return m_cpid; } ;
		uint4 getduration(void);
		uint4 gettimescale(void);
		uint4 getcurrenttime(void);
		uint4 getloudness(void);

	
		// Commands used to manipulate the media players window.
		void resize( MCRectangle p_rect);
		
	private:
		bool m_playing ;
		char * m_filename ;
		uint4 m_osd_level ;
		MCStack *m_stack ;
		GdkWindow *m_window;
		MCRectangle m_player_rect ;
		int m_pfd_write[2];
		int m_pfd_read[2];
	    pid_t m_cpid;
	
		uint32_t m_duration ;
		uint32_t m_timescale ;
		uint32_t m_loudness ;

	
		bool launch_player(void);
		void write_command(MCStringRef p_cmd);
		bool read_command(MCStringRef p_ans, MCStringRef& r_ret);
		bool get_property(const char *p_prop, MCPlayerPropertyType p_type, void *r_value);
		void set_property(const char *p_prop, MCPlayerPropertyType p_type, void *p_value);
		
} ;


#endif
