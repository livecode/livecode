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

//
// MCPlayer class declarations
// This implementation is only made if FEATURE_PLATFORM_PLAYER is not set
//
#ifndef	PLAYER_LEGACY_H
#define	PLAYER_LEGACY_H

#ifndef FEATURE_PLATFORM_PLAYER

#include "mccontrol.h"
#include "player-interface.h"
#include "exec-interface.h"

#ifdef FEATURE_QUICKTIME
// Forward declaration
struct MCPlayerOffscreenBuffer;
#endif

typedef MCObjectProxy<MCPlayer>::Handle MCPlayerHandle;

// SN-2014-07-23: [[ Bug 12893 ]] MCControl must be the first class inherited
//  since we use &MCControl::kPropertyTable
class MCPlayer : public MCControl, public MCMixinObjectHandle<MCPlayer>, public MCPlayerInterface
{
public:
    
    enum { kObjectType = CT_PLAYER };
    using MCMixinObjectHandle<MCPlayer>::GetHandle;
    
private:
    
#ifdef FEATURE_MPLAYER
	char *command;
	Atom atom;
	MPlayer *m_player ;
#endif
	
#ifdef FEATURE_QUICKTIME
	static QTstate qtstate;
	static long qtversion;
	static void *sgSoundComp;
	static const char  *recordtempfile;
	static char  *recordexportfile;
	static long sgSndDriver;
	static MCPlayer *s_ephemeral_player;

	void *theMovie;
	void *theMC;
	QTVRstate qtvrstate;
	void *qtvrinstance;
	Boolean isinteractive;

	/* QT only, the last time the user receives the movie's current
	* time. this variable * is changed in prepareQT(),
	* setmoviecurtime(), getcurtime() and mfocus() routines */

	/* QT movie call back proc reference */
	void *stopMovieCB; //callback ptr for movie when it goes to the end(a stop)
	void *interestingTimeCB;
	void *bufferGW; //GWorld for the buffering draw for QT movie, in WIN & MAC

	MCPlayerOffscreenBuffer *m_offscreen;

#ifdef _MAC_DESKTOP
	void *movie_view;
#elif defined _WINDOWS_DESKTOP
	uint32_t deviceID;  //device id for opened AVI device.
	uint32_t mciPlayFrom, mciPlayTo;
	uint32_t mciPlayFlag;
	MCSysWindowHandle hwndMovie; //a movie window is created for avi movie
	bool m_has_port_association;
#endif

#endif
	MCPlayerHandle nextplayer;
    
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;

public:
	MCPlayer();
	MCPlayer(const MCPlayer &sref);
	virtual ~MCPlayer();
    
    MCPlayerHandle getnextplayer()
    {
        return nextplayer;
    }
    
	// virtual functions from MCObject
	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    
    virtual bool visit_self(MCObjectVisitor *p_visitor);
    
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
	virtual void applyrect(const MCRectangle &nrect);
	virtual void timer(MCNameRef mptr, MCParameter *params);
    
	// MW-2011-09-23: [[ Bug ]] Implement buffer / unbuffer at the point of
	//   selection to stop redraw issues.
	virtual void select(void);
	virtual void deselect(void);
    
	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
    
    virtual void removereferences(void);
    void removefromplayers(void);
    
	// virtual functions from MCControl
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);
    
	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
    
	// MW-2005-08-02: Return the rectangle of the player that bounds the active area
	MCRectangle getactiverect(void);
    // End MCObjet functions
	
#ifdef _LINUX_DESKTOP
	const char *getcommand()
	{
		return command;
	}
	Boolean syncxanim();
#endif
    
    // virtual function from MCPlayerInterface
	virtual bool getversion(MCStringRef& r_string);
	virtual void freetmp();
	virtual MCPlayerDuration getduration();    //get movie duration/length
	virtual MCPlayerDuration gettimescale();  //get movie time scale
	virtual MCPlayerDuration getmoviecurtime();//get movie current time
	virtual void setcurtime(MCPlayerDuration curtime, bool notify);
	virtual void setselection(bool notify);                  //set movie selection
	virtual void setlooping(Boolean loop);        //to loop or not to loop a movie
	virtual void setplayrate();                   //set the movie playing rate
	virtual void showbadge(Boolean show);         //show & hide the movie's badge
	virtual void showcontroller(Boolean show);
	virtual void editmovie(Boolean edit);
	virtual void playselection(Boolean play);     //play the selected part of QT moive only
	virtual Boolean ispaused();
	virtual MCRectangle getpreferredrect();
    virtual uint2 getloudness();
	virtual void setloudness();
	virtual double getleftbalance();
	virtual void setleftbalance(double p_left_balance);
	virtual double getrightbalance();
	virtual void setrightbalance(double p_right_balance);
	virtual double getaudiopan();
	virtual void setaudiopan(double p_pan);

	virtual Boolean prepare(MCStringRef options);
	virtual Boolean playstart(MCStringRef options);
	virtual Boolean playpause(Boolean on);
    virtual void playstepforward();
	virtual void playstepback();
	virtual Boolean playstop();
	virtual void setvolume(uint2 tloudness);
	virtual void setfilename(MCStringRef vcname, MCStringRef fname, Boolean istmp);
    
	virtual void setstarttime(MCPlayerDuration stime)
	{
		starttime = stime;
	}
	virtual void setendtime(MCPlayerDuration etime)
	{
		endtime = etime;
	}
	virtual void setlasttime(MCPlayerDuration ltime)
	{
		lasttime = ltime;
	}
    
    virtual void syncbuffering(MCContext *dc);
    
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
    virtual void gettracks(MCStringRef &r_tracks);
    virtual uinteger_t gettrackcount();
    virtual void getnodes(MCStringRef &r_nodes);
    virtual void gethotspots(MCStringRef &r_nodes);
    virtual void getconstraints(MCMultimediaQTVRConstraints &r_constraints);
    virtual void getenabledtracks(uindex_t &r_count, uint32_t *&r_tracks_id);
    virtual void setenabledtracks(uindex_t p_count, uint32_t *p_tracks_id);
    
    
    virtual void updatevisibility();
    virtual void updatetraversal();

    // SN-2015-01-06: [[ Merge-6.7.2-rc-1 ]]
    virtual bool resolveplayerfilename(MCStringRef p_filename, MCStringRef &r_filename);
    
    // End of virtual functions from MCPlayerInterface
    
	//////////////////////////////
	// QT ACCESSORS

	void getqtvrconstraints(uint1 index, real4& minrange, real4& maxrange);
	uint2 getnodecount();
	virtual bool getnode(uindex_t index, uint2 &id, MCMultimediaQTVRNodeType &type);
	virtual uint2 gethotspotcount();
	virtual bool gethotspot(uindex_t index, uint2 &id, MCMultimediaQTVRHotSpotType &type);
    
    
#if !defined(FEATURE_QUICKTIME)
	virtual Boolean isbuffering()
	{
		return False;
	}
#else
	void initqt();
	void initqtvr();
    
	MCRectangle resize(MCRectangle rect);
	void checktimes();
    
	// MW-2005-05-15: Augment call with extra title field for consistency
	void handlerecord();
	void reloadcallbacks(Boolean reloadstopmovie, long p_from_time);
    
	void *getmovie()
	{
		return theMovie;
	}
	void *getinterestCB()
	{
		return interestingTimeCB;
	}
	void *getMovieController()
	{
		return theMC;
	}
	static Boolean isQTinitted()
	{
		return (qtstate == QT_INITTED);
	}
	Boolean isInteractive()
	{
		return isinteractive;
	}
	void deleteUserCallbacks();
	Boolean installUserCallbacks();
	void setMCposition(const MCRectangle &newrect); //re-position the controller
	Boolean prepareQT();
	void bufferDraw(bool p_resize); //direct movie controller to draw to Offscreen buffer
	void unbufferDraw(); //direct movie controller to draw to Screen/Window
    
	virtual Boolean isbuffering()
	{
		return (m_offscreen != NULL);
	}
	
	Boolean usingQT()
	{
		return usingqt;
	}

#ifdef _WINDOWS_DESKTOP
	void changewindow(MCSysWindowHandle p_old_window);
    
	Boolean prepareAVI();
	uint32_t getDeviceID()
	{
		return deviceID;
	} //AVI stuff
	void setAVIplayselection(Boolean selection);
	uint4 getAVIlength(); //get the length(duration) of the movie
	Boolean AVIseek(uint4 to); //seek to position in a AVI movie
	MCSysWindowHandle getplayerwindow()
	{
		return hwndMovie;
	}
#endif    
#endif
	
#ifdef FEATURE_QUICKTIME
	Boolean qt_prepare(void);
	Boolean qt_playpause(Boolean on);
	void qt_playstepforward(void);
	void qt_playstepback(void);
	Boolean qt_playstop(void);
	void qt_setrect(const MCRectangle& nrect);
	uint4 qt_getduration(void);
	uint4 qt_gettimescale(void);
	uint4 qt_getmoviecurtime(void);
	void qt_setcurtime(uint4 newtime);
	void qt_setselection(void);
	void qt_setlooping(Boolean loop);
	void qt_setplayrate(void);
	void qt_showbadge(Boolean show);
	void qt_editmovie(Boolean edit);
	void qt_playselection(Boolean play);
	Boolean qt_ispaused(void);
	void qt_showcontroller(Boolean show);
	MCRectangle qt_getpreferredrect(void);
	uint2 qt_getloudness(void);
	void qt_setloudness(uint2 loudn);
    void qt_gettracks(MCStringRef &r_tracks);
    void qt_getenabledtracks(uindex_t &r_count, uint32_t *&r_tracks_id);
    void qt_setenabledtracks(uindex_t p_count, uint32_t* p_tracks);
	void qt_draw(MCDC *dc, const MCRectangle& dirty);
	void qt_move(int2 x, int2 y);
	void qt_click(bool p_state, uint4 p_button);
	void qt_key(bool p_state, uint4 p_key);
	void qt_enablekeys(Boolean enable);
	void qt_setcontrollervisible();

#ifdef _WINDOWS_DESKTOP
	Boolean avi_prepare(void);
	Boolean avi_playpause(Boolean on);
	void avi_playstepforward(void);
	void avi_playstepback(void);
	Boolean avi_playstop(void);
	void avi_setrect(const MCRectangle& nrect);
	uint4 avi_getduration(void);
	uint4 avi_gettimescale(void);
	uint4 avi_getmoviecurtime(void);
	void avi_setcurtime(uint4 newtime);
	void avi_setselection(void);
	void avi_setlooping(Boolean loop);
	void avi_setplayrate(void);
	void avi_showbadge(Boolean show);
	void avi_editmovie(Boolean edit);
	void avi_playselection(Boolean play);
	Boolean avi_ispaused(void);
	void avi_showcontroller(Boolean show);
	MCRectangle avi_getpreferredrect(void);
	uint2 avi_getloudness(void);
	void avi_setloudness(uint2 loudn);
    void avi_gettracks(MCStringRef &r_tracks);
    void avi_getenabledtracks(uindex_t &r_count, uint32_t *&r_tracks_id);
    void avi_setenabledtracks(uindex_t p_count, uint32_t* p_tracks);
	void avi_draw(MCDC *dc, const MCRectangle& dirty);

	bool mode_avi_closewindowonplaystop();
#endif
#endif

#if defined(_LINUX_DESKTOP)
	Boolean x11_prepare(void) ;
	Boolean x11_playpause(Boolean on) ;
	void x11_playstepforward(void) ;
	void x11_playstepback(void) ;
	Boolean x11_playstop(void) ;
	void x11_setrect(const MCRectangle& nrect);
	uint4 x11_getduration(void);
	uint4 x11_gettimescale(void);
	uint4 x11_getmoviecurtime(void);
	void x11_setlooping(Boolean loop);
	void x11_setplayrate(void);
	Boolean x11_ispaused(void);
	uint2 x11_getloudness(void);
	void x11_setloudness(uint2 loudn);
	
	// Not supported
	void x11_setcurtime(uint4 newtime) {}
	void x11_setselection(void) {}
	void x11_showbadge(Boolean show) {}
	void x11_editmovie(Boolean edit) {}
	void x11_playselection(Boolean play) {}
	void x11_showcontroller(Boolean show) {}
    MCRectangle x11_getpreferredrect(void) { MCRectangle t_rect; t_rect.x = 0;t_rect.y = 0;t_rect.width=0;t_rect.height=0; return t_rect;}
    void x11_gettracks(MCStringRef &r_tracks) { r_tracks = MCValueRetain(kMCEmptyString); }
    void x11_getenabledtracks(uindex_t &r_count, uint32_t *&r_tracks_id) { r_count = 0; }
    void x11_setenabledtracks(uindex_t p_count, uint32_t *p_tracks_id) {}
	void x11_draw(MCDC *dc, const MCRectangle& dirty) {}
	
	pid_t getpid(void);
	void  shutdown(void);
#endif
    
	////////// PROPERTY SUPPORT METHODS
    
	virtual void Redraw(void);
    
	////////// PROPERTY ACCESSORS
    
	virtual void GetFileName(MCExecContext& ctxt, MCStringRef& r_name);
	virtual void SetFileName(MCExecContext& ctxt, MCStringRef p_name);
	virtual void GetDontRefresh(MCExecContext& ctxt, bool& r_setting);
	virtual void SetDontRefresh(MCExecContext& ctxt, bool setting);
	virtual void GetCurrentTime(MCExecContext& ctxt, double& r_time);
	virtual void SetCurrentTime(MCExecContext& ctxt, double p_time);
	virtual void GetDuration(MCExecContext& ctxt, double& r_duration);
    virtual void GetLoadedTime(MCExecContext& ctxt, double& r_time);
	virtual void GetLooping(MCExecContext& ctxt, bool& r_setting);
	virtual void SetLooping(MCExecContext& ctxt, bool setting);
	virtual void GetPaused(MCExecContext& ctxt, bool& r_setting);
	virtual void SetPaused(MCExecContext& ctxt, bool setting);
	virtual void GetAlwaysBuffer(MCExecContext& ctxt, bool& r_setting);
	virtual void SetAlwaysBuffer(MCExecContext& ctxt, bool setting);
	virtual void GetPlayRate(MCExecContext& ctxt, double& r_rate);
	virtual void SetPlayRate(MCExecContext& ctxt, double p_rate);
	virtual void GetStartTime(MCExecContext& ctxt, double*& r_time);
	virtual void SetStartTime(MCExecContext& ctxt, double* p_time);
	virtual void GetEndTime(MCExecContext& ctxt, double*& r_time);
	virtual void SetEndTime(MCExecContext& ctxt, double* p_time);
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
	virtual void GetTimeScale(MCExecContext& ctxt, double& r_scale);
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
	virtual void GetLeftBalance(MCExecContext& ctxt, double &r_left_balance);
	virtual void SetLeftBalance(MCExecContext& ctxt, double p_left_balance);
	virtual void GetRightBalance(MCExecContext& ctxt, double &r_right_balance);
	virtual void SetRightBalance(MCExecContext& ctxt, double p_right_balance);
	virtual void GetAudioPan(MCExecContext& ctxt, double &r_pan);
	virtual void SetAudioPan(MCExecContext& ctxt, double p_pan);

	virtual void GetTracks(MCExecContext& ctxt, MCStringRef& r_tracks);
    
	virtual void GetConstraints(MCExecContext& ctxt, MCMultimediaQTVRConstraints& r_constraints);
	virtual void GetNodes(MCExecContext& ctxt, MCStringRef& r_nodes);
	virtual void GetHotSpots(MCExecContext& ctxt, MCStringRef& r_spots);
    
    virtual void SetShowBorder(MCExecContext& ctxt, bool setting);
    virtual void SetBorderWidth(MCExecContext& ctxt, uinteger_t width);
    virtual void SetVisible(MCExecContext& ctxt, uinteger_t part, bool setting);
    virtual void SetTraversalOn(MCExecContext& ctxt, bool setting);
    
    virtual void GetEnabledTracks(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tracks);
    virtual void SetEnabledTracks(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_tracks);
    
    virtual void GetDontUseQT(MCExecContext& ctxt, bool &p_dont_use_qt);
    virtual void SetDontUseQT(MCExecContext& ctxt, bool r_dont_use_qt);
    
    // MCplayers handling
    static void StopPlayers(MCStack* p_stack);
    static void SyncPlayers(MCStack* p_stack, MCContext* p_context);
    static void ClosePlayers(MCStack* p_stack);
    static void SetPlayersVolume(uinteger_t p_volume);
    
    static MCPlayer* FindPlayerByName(MCNameRef p_name);
    static MCPlayer* FindPlayerById(uint32_t p_id);
    
#endif // FEATURE_PLATFORM_PLAYER
};


#endif // PLAYER_LEGACY_H
