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

//
// MCPlayer class declarations
//
#ifndef	PLAYER_PLATFORM_H
#define	PLAYER_PLATFORM_H

#include "control.h"
#include "platform.h"

enum
{
    kMCPlayerControllerPartUnknown,
    
    kMCPlayerControllerPartVolume,
    kMCPlayerControllerPartVolumeBar,
    kMCPlayerControllerPartVolumeWell,
    kMCPlayerControllerPartVolumeSelector,
    kMCPlayerControllerPartRateWell,
    kMCPlayerControllerPartRateBar,
    kMCPlayerControllerPartRateSelector,
    kMCPlayerControllerPartPlay,
    kMCPlayerControllerPartScrubBack,
    kMCPlayerControllerPartScrubForward,
    kMCPlayerControllerPartThumb,
    kMCPlayerControllerPartWell,
    kMCPlayerControllerPartSelectionStart,
    kMCPlayerControllerPartSelectionFinish,
    kMCPlayerControllerPartSelectedArea,
    kMCPlayerControllerPartVolumeArea,
    kMCPlayerControllerPartPlayedArea,
    kMCPlayerControllerPartBuffer,
    
};

struct MCPlayerCallback
{
    MCNameRef message;
    MCNameRef parameter;
    uint32_t time;
};

class MCPlayer : public MCControl
{
	MCPlayer *nextplayer;
	char *filename;
    char *resolved_filename;
	uint2 framerate;
	Boolean disposable;
	Boolean istmpfile;
	real8 scale;
	real8 rate;
	uint4 starttime;
	uint4 endtime;
	char *userCallbackStr;  //string contains user movie callbacks
	uint2 formattedwidth;
	uint2 formattedheight;
	uint2 loudness;
	int4 lasttime;
    
	MCPlatformPlayerRef m_platform_player;
    MCPlayerCallback *m_callbacks;
    uindex_t m_callback_count;
    
    int m_grabbed_part;
    bool m_was_paused : 1;
    bool m_inside : 1;
    bool m_show_volume : 1;
    bool m_scrub_back_is_pressed : 1;
    bool m_scrub_forward_is_pressed : 1;
    bool m_modify_selection_while_playing : 1;
    bool m_is_attached : 1;
    bool m_should_attach : 1;
    bool m_should_recreate : 1;
	
public:
	MCPlayer();
	MCPlayer(const MCPlayer &sref);
	// virtual functions from MCObject
	virtual ~MCPlayer();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
	virtual void open();
	virtual void close();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean kup(const char *string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean mdown(uint2 which);
    virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void setrect(const MCRectangle &nrect);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
    
	// MW-2011-09-23: [[ Bug ]] Implement buffer / unbuffer at the point of
	//   selection to stop redraw issues.
	virtual void select(void);
	virtual void deselect(void);
    
	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
    
	// virtual functions from MCControl
	IO_stat load(IO_handle stream, const char *version);
	IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);
    
	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
	// MCPlayer functions
	MCPlayer *getnextplayer()
	{
		return nextplayer;
	}
    
	// MW-2005-08-02: Return the rectangle of the player that bounds the active area
	MCRectangle getactiverect(void);
    
	void getversion(MCExecPoint &ep);
	void freetmp();
	uint4 getduration();        //get movie duration/length
	uint4 gettimescale();       //get movie time scale
    uint4 getmovieloadedtime(); //get movie loaded time
	uint4 getmoviecurtime();    //get movie current time
	void getminwait(real8 &wait);
	void setcurtime(uint4 curtime, bool notify);
	void setselection(bool notify);                  //set movie selection
	void setlooping(Boolean loop);        //to loop or not to loop a movie
	void setplayrate();                   //set the movie playing rate
    real8 getplayrate();
    void updateplayrate(real8 p_rate);
	Boolean setInterestingTimeCB();       //True, if set, False, if not
	void showbadge(Boolean show);         //show & hide the movie's badge
	void showcontroller(Boolean show);
	void editmovie(Boolean edit);
	void playselection(Boolean play);     //play the selected part of QT moive only
	Boolean ispaused();
	void setpause();
    
	MCRegionRef makewindowregion();
    
	MCRectangle getpreferredrect();
	uint2 getloudness();
    void updateloudness(int2 newloudness);
    MCColor getcontrollermaincolor();
    MCColor getcontrollericoncolor();
    MCColor getcontrollerfontcolor();
    MCColor getcontrollerselectedareacolor();
	void setloudness();
	void gettracks(MCExecPoint &ep);
	void getenabledtracks(MCExecPoint &ep);
	Boolean setenabledtracks(const MCString &s);
	void getnodes(MCExecPoint &ep);
	void gethotspots(MCExecPoint &ep);
    
	// MW-2011-09-23: Ensures the buffering state is consistent with current flags
	//   and such.
	void syncbuffering(MCContext *dc);
    
	MCRectangle resize(MCRectangle rect);
	void SynchronizeUserCallbacks(void);
	Boolean isbuffering(void);
	
	Boolean isdisposable()
	{
		return disposable;
	}
	void setscale(real8 &s)
	{
		scale = s;
	}
	Boolean prepare(const char *options);
	Boolean playstart(const char *options);
	Boolean playpause(Boolean on);
	void playstepforward();
    bool resolveplayerfilename(const char *p_filename, char *&r_filename);
    //void playfast(Boolean forward);
    //void playfastforward();
    //void playfastback();
	void playstepback();
	Boolean playstop();
	void setvolume(uint2 tloudness);
	void setfilename(const char *vcname, char *fname, Boolean istmp);
	uint4 getstarttime()
	{
		return starttime;
	}
	uint4 getendtime()
	{
		return endtime;
	}
	int4 getlasttime()
	{
		return lasttime;
	}
	void setendtime(uint4 etime)
	{
        
        if (hasfilename())
            MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFinishTime, kMCPlatformPropertyTypeUInt32, &endtime);
        layer_redrawrect(getcontrollerrect());
	}
	void setlasttime(int4 ltime)
	{
		lasttime = ltime;
	}
    
	MCPlatformPlayerRef getplatformplayer(void)
	{
		return m_platform_player;
	}
    
    bool hasfilename(void) const
    {
        return filename != NULL;
    }
    
    // PM-2014-12-17: [[ Bug 14232 ]] Indicates if a filename is invalid or if the file is corrupted
    bool hasinvalidfilename(void) const
    {
        bool t_has_invalid_filename;
        MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyInvalidFilename, kMCPlatformPropertyTypeBool, &t_has_invalid_filename);
        return t_has_invalid_filename;
    }
    
    void markerchanged(uint32_t p_time);
    void selectionchanged(void);
    void currenttimechanged(void);
	void moviefinished(void);
    
    MCRectangle getcontrollerrect(void);
    MCRectangle getcontrollerpartrect(const MCRectangle& total_rect, int part);

    void drawcontroller(MCDC *dc);
    void drawControllerVolumeButton(MCGContextRef p_gcontext);
    void drawControllerPlayPauseButton(MCGContextRef p_gcontext);
    void drawControllerWellButton(MCGContextRef p_gcontext);
    void drawControllerThumbButton(MCGContextRef p_gcontext);
    void drawControllerScrubForwardButton(MCGContextRef p_gcontext);
    void drawControllerScrubBackButton(MCGContextRef p_gcontext);
    void drawControllerSelectionStartButton(MCGContextRef p_gcontext);
    void drawControllerSelectionFinishButton(MCGContextRef p_gcontext);
    void drawControllerSelectedAreaButton(MCGContextRef p_gcontext);
    void drawControllerPlayedAreaButton(MCGContextRef p_gcontext);
    void drawControllerBufferedAreaButton(MCGContextRef p_gcontext);
    
    void drawcontrollerbutton(MCDC *dc, const MCRectangle& rect);
    void redrawcontroller(void);
    
    int hittestcontroller(int x, int y);
    
    Boolean handle_kdown(const char *string, KeySym key);
    Boolean handle_shift_kdown(const char *string, KeySym key);
    void handle_mdown(int which);
    void handle_mstilldown(int which);
    void handle_shift_mdown(int which);
    void shift_play(void);
    
    void handle_mup(int which);
    void handle_mfocus(int x, int y);
    
    void popup_closed(void);
    
    // PM-2014-10-14: [[ Bug 13569 ]] Make sure changes to player are not visible in preOpenCard
    void attachplayer(void);
    void detachplayer(void);
};

#endif
