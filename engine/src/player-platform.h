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
// This implementation is only made if FEATURE_PLATFORM_PLAYER is set
//
#ifndef	PLAYER_PLATFORM_H
#define	PLAYER_PLATFORM_H

#ifdef FEATURE_PLATFORM_PLAYER

#include "control.h"
#include "platform.h"
#include "player-interface.h"

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

// SN-2014-07-23: [[ Bug 12893 ]] MCControl must be the first class inherited
//  since we use &MCControl::kPropertyTable
class MCPlayer : public MCControl, public MCPlayerInterface
{
	MCPlayer *nextplayer;
    
    MCColor controllerbackcolor;
    MCColor controllermaincolor;
    MCColor selectedareacolor;
    
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
    
	static MCPropertyInfo kProperties[];
    static MCObjectPropertyTable kPropertyTable;
	
public:
	MCPlayer();
	MCPlayer(const MCPlayer &sref);
	virtual ~MCPlayer();
    
    MCPlayer *getnextplayer()
    {
        return (MCPlayer*)nextplayer;
    }
    
	// virtual functions from MCObject    
	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
    
	virtual void open();
	virtual void close();
	virtual Boolean kdown(MCStringRef p_string, KeySym key);
	virtual Boolean kup(MCStringRef p_string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void setrect(const MCRectangle &nrect);
	virtual void timer(MCNameRef mptr, MCParameter *params);

#ifdef LEGACY_EXEC
	virtual Exec_stat getprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective, bool recursive = false);
	virtual Exec_stat setprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
#endif
    
	// MW-2011-09-23: [[ Bug ]] Implement buffer / unbuffer at the point of
	//   selection to stop redraw issues.
	virtual void select(void);
	virtual void deselect(void);
    
	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
    
	// virtual functions from MCControl
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);
    
	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
    
	// MW-2005-08-02: Return the rectangle of the player that bounds the active area
	MCRectangle getactiverect(void);
    // End MCObjet functions
    
    ////////////////////////////////////////////////////////////////////////////////
    // virtual MCPlayerInterface functions
    //
	virtual bool getversion(MCStringRef& r_string);
	virtual void freetmp();
	virtual uint4 getduration();    //get movie duration/length
	virtual uint4 gettimescale();  //get movie time scale
	virtual uint4 getmoviecurtime();//get movie current time
	virtual void setcurtime(uint4 curtime, bool notify);
	virtual void setselection(bool notify);                  //set movie selection
	virtual void setlooping(Boolean loop);        //to loop or not to loop a movie
	virtual void setplayrate();                   //set the movie playing rate
	virtual void showbadge(Boolean show);         //show & hide the movie's badge
	virtual void showcontroller(Boolean show);
	virtual void editmovie(Boolean edit);
	virtual void playselection(Boolean play);     //play the selected part of QT moive only
	virtual Boolean ispaused();
    
    virtual void gettracks(MCStringRef& r_tracks);
    
    virtual Boolean setenabledtracks(MCStringRef s);
    virtual MCRectangle getpreferredrect();
	virtual uint2 getloudness();
    virtual void updateloudness(int2 newloudness);
	virtual void setloudness();

    virtual Boolean prepare(MCStringRef options);
    virtual Boolean playstart(MCStringRef options);
    virtual Boolean playpause(Boolean on);
    virtual void playstepforward();
    virtual void playstepback();
	virtual Boolean playstop();
    virtual void setvolume(uint2 tloudness);
    virtual void setfilename(MCStringRef vcname, MCStringRef fname, Boolean istmp);
    
	virtual void setstarttime(uint4 stime)
	{
        if (stime <= 0)
            starttime = 0;
        else if (stime > getduration())
            starttime = getduration();
        else
            starttime = stime;
        
        if (hasfilename())
            MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyStartTime, kMCPlatformPropertyTypeUInt32, &starttime);
        layer_redrawrect(getcontrollerrect());
	}
    
	virtual void setendtime(uint4 etime)
    {
        if (hasfilename())
            MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFinishTime, kMCPlatformPropertyTypeUInt32, &endtime);
        layer_redrawrect(getcontrollerrect());
	}
	
    real8 getplayrate();
    void updateplayrate(real8 p_rate);
    uint4 getmovieloadedtime();
	
    Boolean isdisposable()
	{
		return disposable;
	}
	void setscale(real8 &s)
	{
		scale = s;
	}

    //void playfast(Boolean forward);
    //void playfastforward();
    //void playfastback();
	
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

	virtual void setlasttime(int4 ltime)
	{
		lasttime = ltime;
	}
    
	// MW-2011-09-23: Ensures the buffering state is consistent with current flags
	//   and such.
	virtual void syncbuffering(MCContext *dc);
    virtual Boolean isbuffering();
    
    virtual void setcallbacks(MCStringRef p_callbacks);
    
    virtual void setmoviecontrollerid(integer_t p_id);
    virtual integer_t getmoviecontrollerid();
    virtual integer_t getmediatypes();
    virtual uinteger_t getcurrentnode();
    virtual bool changecurrentnode(uinteger_t p_node_id);
	virtual real8 getpan();
	virtual bool changepan(real8 pan);
	virtual real8 gettilt();
	virtual bool changetilt(real8 tilt);
	virtual real8 getzoom();
	virtual bool changezoom(real8 zoom);
    
    virtual uinteger_t gettrackcount();
    virtual void getnodes(MCStringRef &r_nodes);
    virtual void gethotspots(MCStringRef &r_nodes);
    virtual void getconstraints(MCMultimediaQTVRConstraints &r_constraints);
    virtual void getenabledtracks(uindex_t &r_count, uint32_t *&r_tracks_id);
    
    virtual void updatevisibility();
    virtual void updatetraversal();
    
    virtual void setforegroundcolor(const MCInterfaceNamedColor& p_color);
    virtual void getforegrouncolor(MCInterfaceNamedColor& r_color);
    virtual void sethilitecolor(const MCInterfaceNamedColor& p_color);
    virtual void gethilitecolor(MCInterfaceNamedColor& r_color);
    
    //
    // End of MCPlayerInterface's virtual functions
    ////////////////////////////////////////////////////////////////////////////////
    
    
	////////// PROPERTY SUPPORT METHODS
    
	virtual void Redraw(void);
    virtual void SetVisibility(MCExecContext& ctxt, uinteger_t part, bool setting, bool visible);
    
	////////// PROPERTY ACCESSORS
    
	virtual void GetFileName(MCExecContext& ctxt, MCStringRef& r_name);
	virtual void SetFileName(MCExecContext& ctxt, MCStringRef p_name);
	virtual void GetDontRefresh(MCExecContext& ctxt, bool& r_setting);
	virtual void SetDontRefresh(MCExecContext& ctxt, bool setting);
	virtual void GetCurrentTime(MCExecContext& ctxt, uinteger_t& r_time);
	virtual void SetCurrentTime(MCExecContext& ctxt, uinteger_t p_time);
	virtual void GetDuration(MCExecContext& ctxt, uinteger_t& r_duration);
    // PM-2014-11-03: [[ Bug 13920 ]] Make sure we support loadedTime property
    virtual void GetLoadedTime(MCExecContext& ctxt, uinteger_t& r_loaded_time);
	virtual void GetLooping(MCExecContext& ctxt, bool& r_setting);
	virtual void SetLooping(MCExecContext& ctxt, bool setting);
	virtual void GetPaused(MCExecContext& ctxt, bool& r_setting);
	virtual void SetPaused(MCExecContext& ctxt, bool setting);
	virtual void GetAlwaysBuffer(MCExecContext& ctxt, bool& r_setting);
	virtual void SetAlwaysBuffer(MCExecContext& ctxt, bool setting);
	virtual void GetPlayRate(MCExecContext& ctxt, double& r_rate);
	virtual void SetPlayRate(MCExecContext& ctxt, double p_rate);
	virtual void GetStartTime(MCExecContext& ctxt, uinteger_t*& r_time);
	virtual void SetStartTime(MCExecContext& ctxt, uinteger_t* p_time);
	virtual void GetEndTime(MCExecContext& ctxt, uinteger_t*& r_time);
	virtual void SetEndTime(MCExecContext& ctxt, uinteger_t* p_time);
	virtual void GetShowBadge(MCExecContext& ctxt, bool& r_setting);
	virtual void SetShowBadge(MCExecContext& ctxt, bool setting);
	virtual void GetShowController(MCExecContext& ctxt, bool& r_setting);
	virtual void SetShowController(MCExecContext& ctxt, bool setting);
	virtual void GetPlaySelection(MCExecContext& ctxt, bool& r_setting);
	virtual void SetPlaySelection(MCExecContext& ctxt, bool setting);
	virtual void GetShowSelection(MCExecContext& ctxt, bool& r_setting);
	virtual void SetShowSelection(MCExecContext& ctxt, bool setting);
	virtual void GetCallbacks(MCExecContext& ctxt, MCStringRef& r_callbacks);
	virtual void SetCallbacks(MCExecContext& ctxt, MCStringRef p_callbacks);
	virtual void GetTimeScale(MCExecContext& ctxt, uinteger_t& r_scale);
	virtual void GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height);
	virtual void GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width);
	virtual void GetMovieControllerId(MCExecContext& ctxt, integer_t& r_id);
	virtual void SetMovieControllerId(MCExecContext& ctxt, integer_t p_id);
	virtual void GetPlayLoudness(MCExecContext& ctxt, uinteger_t& r_loudness);
	virtual void SetPlayLoudness(MCExecContext& ctxt, uinteger_t p_loudness);
	virtual void GetTrackCount(MCExecContext& ctxt, uinteger_t& r_count);
	virtual void GetMediaTypes(MCExecContext& ctxt, intset_t& r_types);
	virtual void GetCurrentNode(MCExecContext& ctxt, uinteger_t& r_node);
	virtual void SetCurrentNode(MCExecContext& ctxt, uinteger_t p_node);
	virtual void GetPan(MCExecContext& ctxt, double& r_pan);
	virtual void SetPan(MCExecContext& ctxt, double p_pan);
	virtual void GetTilt(MCExecContext& ctxt, double& r_tilt);
	virtual void SetTilt(MCExecContext& ctxt, double p_tilt);
	virtual void GetZoom(MCExecContext& ctxt, double& r_zoom);
	virtual void SetZoom(MCExecContext& ctxt, double p_zoom);
    
	virtual void GetTracks(MCExecContext& ctxt, MCStringRef& r_tracks);
    
	virtual void GetConstraints(MCExecContext& ctxt, MCMultimediaQTVRConstraints& r_constraints);
	virtual void GetNodes(MCExecContext& ctxt, MCStringRef& r_nodes);
	virtual void GetHotSpots(MCExecContext& ctxt, MCStringRef& r_spots);
    
    virtual void SetShowBorder(MCExecContext& ctxt, bool setting);
    virtual void SetBorderWidth(MCExecContext& ctxt, uinteger_t width);
    virtual void SetVisible(MCExecContext& ctxt, uinteger_t part, bool setting);
    virtual void SetInvisible(MCExecContext& ctxt, uinteger_t part, bool setting);
    virtual void SetTraversalOn(MCExecContext& ctxt, bool setting);
    
    virtual void GetEnabledTracks(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tracks);
    
    virtual void SetForeColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color);
    virtual void GetForeColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
    virtual void SetHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color);
    virtual void GetHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
    
    void GetStatus(MCExecContext& ctxt, intenum_t& r_status);
    
    ////////////////////////////////////////////////////////////////////////////////
    // MCPlayer specific implementation for the platform player
    
	void SynchronizeUserCallbacks(void);
	
    //void playfast(Boolean forward);
    //void playfastforward();
    //void playfastback();
    MCColor getcontrollermaincolor();
    MCColor getcontrollerbackcolor();
    
	MCPlatformPlayerRef getplatformplayer(void)
	{
		return m_platform_player;
	}

    bool hasfilename()
    {
        return !MCStringIsEmpty(filename);
    }
    
    MCRectangle resize(MCRectangle rect);

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
    
    Boolean handle_kdown(MCStringRef p_string, KeySym key);
    Boolean handle_shift_kdown(MCStringRef p_string, KeySym key);
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

#endif
