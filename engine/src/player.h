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
#ifndef	PLAYER_H
#define	PLAYER_H

#include "control.h"

#ifdef FEATURE_QUICKTIME
enum QTstate {
    QT_NOT_INITTED,
    QT_INITTED,
    QT_FAILED
};

enum QTVRstate {
    QTVR_NOT_INITTED,
    QTVR_INITTED,
    QTVR_FAILED
};

typedef struct
{
	char *token;
	long type;
}
QTEffect;
#endif

struct MCMultimediaTrackList;
struct MCMultimediaQTVRConstraints;
struct MCMultimediaQTVRNode;
struct MCMultimediaQTVRHotSpot;

enum MCMultimediaQTVRHotSpotType
{
	kMCQTVRHotSpotLinkType,
	kMCQTVRHotSpotURLType,
	kMCQTVRHotSpotUndefinedType,
};

enum MCMultimediaQTVRNodeType
{
	kMCQTVRNodePanoramaType,
	kMCQTVRNodeObjectType,
};

typedef uint4 MCPlayerMediaTypeSet;
enum
{
	PLAYER_MEDIA_TYPE_VIDEO_BIT = 0,
	PLAYER_MEDIA_TYPE_VIDEO = 1 << PLAYER_MEDIA_TYPE_VIDEO_BIT,
	PLAYER_MEDIA_TYPE_AUDIO_BIT = 1,
	PLAYER_MEDIA_TYPE_AUDIO = 1 << PLAYER_MEDIA_TYPE_AUDIO_BIT,
	PLAYER_MEDIA_TYPE_TEXT_BIT = 2,
	PLAYER_MEDIA_TYPE_TEXT = 1 << PLAYER_MEDIA_TYPE_TEXT_BIT,
	PLAYER_MEDIA_TYPE_QTVR_BIT = 3,
	PLAYER_MEDIA_TYPE_QTVR = 1 << PLAYER_MEDIA_TYPE_QTVR_BIT,
	PLAYER_MEDIA_TYPE_SPRITE_BIT = 4,
	PLAYER_MEDIA_TYPE_SPRITE = 1 << PLAYER_MEDIA_TYPE_SPRITE_BIT,
	PLAYER_MEDIA_TYPE_FLASH_BIT = 4,
	PLAYER_MEDIA_TYPE_FLASH = 1 << PLAYER_MEDIA_TYPE_FLASH_BIT,
};

struct MCPlayerOffscreenBuffer;

class MCPlayer : public MCControl
{
	MCPlayer *nextplayer;
	MCStringRef filename;
	uint2 framerate;
	Boolean disposable;
	Boolean istmpfile;
	real8 scale;
	real8 rate;
	uint4 starttime;
	uint4 endtime;
	MCStringRef userCallbackStr;  //string contains user movie callbacks
	uint2 formattedwidth;
	uint2 formattedheight;
	uint2 loudness;
	int4 lasttime;

#ifdef FEATURE_MPLAYER
	char *command;
	Atom atom;
	MPlayer *m_player ;
#endif

#ifdef FEATURE_QUICKTIME
	static QTstate qtstate;
	static long qtversion;
	static QTEffect *qteffects;
	static uint2 neffects;
	static void *sgSoundComp;
	static MCStringRef recordtempfile;
	static MCStringRef recordexportfile;
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

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
public:
	MCPlayer();
	MCPlayer(const MCPlayer &sref);
	// virtual functions from MCObject
	virtual ~MCPlayer();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }

	virtual void open();
	virtual void close();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean kup(const char *string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void setrect(const MCRectangle &nrect);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual Exec_stat getprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective);

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

	bool getversion(MCStringRef& r_string);
	void freetmp();
	uint4 getduration();    //get movie duration/length
	uint4 gettimescale();  //get movie time scale
	uint4 getmoviecurtime();//get movie current time
	void getminwait(real8 &wait);
	void setcurtime(uint4 curtime);
	void setselection();                  //set movie selection
	void setlooping(Boolean loop);        //to loop or not to loop a movie
	void setplayrate();                   //set the movie playing rate
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
	void setloudness();
	void gettracks(MCExecPoint &ep);
	void getenabledtracks(MCExecPoint &ep);
	Boolean setenabledtracks(MCStringRef s);
	void getnodes(MCExecPoint &ep);
	void gethotspots(MCExecPoint &ep);
	bool geteffectlist(MCStringRef& r_string);
	void recordsound(MCStringRef fname);
	bool getrecordloudness(integer_t& r_loudness);
	bool getrecordcompressionlist(MCStringRef& r_string);

	//////////////////////////////
	// QT ACCESSORS

	MCPlayerMediaTypeSet getmediatypes();
	uint2 getcurrentnode();
	bool changecurrentnode(uint2 nodeid);
	real8 getpan();
	bool changepan(real8 pan);
	real8 gettilt();
	bool changetilt(real8 tilt);
	real8 getzoom();
	bool changezoom(real8 zoom);
	bool gettrack(uindex_t index, uint4& id, MCStringRef& r_name, uint4& offset, uint4& duration);
	void getqtvrconstraints(uint1 index, real4& minrange, real4& maxrange);
	uint2 getnodecount();
	bool getnode(uindex_t index, uint2 &id, MCMultimediaQTVRNodeType &type);
	uint2 gethotspotcount();
	bool gethotspot(uindex_t index, uint2 &id, MCMultimediaQTVRHotSpotType &type);
	void setmoviecontrollerid(int4 p_id);
	uint2 gettrackcount();

	//////////////////////////////

	// MW-2005-05-15: Augment call with extra title field for consistency
	bool stdrecorddlg(MCStringRef &r_result);
	void stoprecording();

	// MW-2011-09-23: Ensures the buffering state is consistent with current flags
	//   and such.
	void syncbuffering(MCContext *dc);

	bool stdeffectdlg(MCStringRef &r_value, MCStringRef &r_result);

#ifdef _LINUX_DESKTOP
	const char *getcommand()
	{
		return command;
	}
	Boolean syncxanim();
#endif

#if !defined(FEATURE_QUICKTIME)
	Boolean isbuffering()
	{
		return False;
	}
#else
	void initqt();
	void initqtvr();

	MCRectangle resize(MCRectangle rect);
	void checktimes();

	// MW-2005-05-15: Augment call with extra title field for consistency
	void queryeffects(void **effectatomptr);
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
	QTEffect *geteffects()
	{
		queryeffects(NULL);
		return qteffects;
	}
	uint2 getneffects()
	{
		return neffects;
	}
	void deleteUserCallbacks();
	Boolean installUserCallbacks();
	void setMCposition(const MCRectangle &newrect); //re-position the controller
	Boolean prepareQT();
	void bufferDraw(bool p_resize); //direct movie controller to draw to Offscreen buffer
	void unbufferDraw(); //direct movie controller to draw to Screen/Window

	Boolean isbuffering()
	{
		return (m_offscreen != NULL);
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

	Boolean isdisposable()
	{
		return disposable;
	}
	void setscale(real8 &s)
	{
		scale = s;
	}
	Boolean prepare(MCStringRef options);
	Boolean playstart(MCStringRef options);
	Boolean playpause(Boolean on);
	void playstepforward();
	void playstepback();
	Boolean playstop();
	void setvolume(uint2 tloudness);
	void setfilename(MCStringRef vcname, MCStringRef fname, Boolean istmp);
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
	void setstarttime(uint4 stime)
	{
		starttime = stime;
	}
	void setendtime(uint4 etime)
	{
		endtime = etime;
	}
	void setlasttime(int4 ltime)
	{
		lasttime = ltime;
	}

	////////// PROPERTY SUPPORT METHODS

	void Redraw(void);
    void SetVisibility(MCExecContext& ctxt, uinteger_t part, bool setting, bool visible);

	////////// PROPERTY ACCESSORS

	void GetFileName(MCExecContext& ctxt, MCStringRef& r_name);
	void SetFileName(MCExecContext& ctxt, MCStringRef p_name);
	void GetDontRefresh(MCExecContext& ctxt, bool& r_setting);
	void SetDontRefresh(MCExecContext& ctxt, bool setting);
	void GetCurrentTime(MCExecContext& ctxt, uinteger_t& r_time);
	void SetCurrentTime(MCExecContext& ctxt, uinteger_t p_time);
	void GetDuration(MCExecContext& ctxt, uinteger_t& r_duration);
	void GetLooping(MCExecContext& ctxt, bool& r_setting);
	void SetLooping(MCExecContext& ctxt, bool setting);
	void GetPaused(MCExecContext& ctxt, bool& r_setting);
	void SetPaused(MCExecContext& ctxt, bool setting);
	void GetAlwaysBuffer(MCExecContext& ctxt, bool& r_setting);
	void SetAlwaysBuffer(MCExecContext& ctxt, bool setting);
	void GetPlayRate(MCExecContext& ctxt, double& r_rate);
	void SetPlayRate(MCExecContext& ctxt, double p_rate);
	void GetStartTime(MCExecContext& ctxt, uinteger_t*& r_time);
	void SetStartTime(MCExecContext& ctxt, uinteger_t* p_time);
	void GetEndTime(MCExecContext& ctxt, uinteger_t*& r_time);
	void SetEndTime(MCExecContext& ctxt, uinteger_t* p_time);
	void GetShowBadge(MCExecContext& ctxt, bool& r_setting);
	void SetShowBadge(MCExecContext& ctxt, bool setting);
	void GetShowController(MCExecContext& ctxt, bool& r_setting);
	void SetShowController(MCExecContext& ctxt, bool setting);
	void GetPlaySelection(MCExecContext& ctxt, bool& r_setting);
	void SetPlaySelection(MCExecContext& ctxt, bool setting);
	void GetShowSelection(MCExecContext& ctxt, bool& r_setting);
	void SetShowSelection(MCExecContext& ctxt, bool setting);
	void GetCallbacks(MCExecContext& ctxt, MCStringRef& r_callbacks);
	void SetCallbacks(MCExecContext& ctxt, MCStringRef p_callbacks);
	void GetTimeScale(MCExecContext& ctxt, uinteger_t& r_scale);
	void GetFormattedHeight(MCExecContext& ctxt, uinteger_t& r_height);
	void GetFormattedWidth(MCExecContext& ctxt, uinteger_t& r_width);
	void GetMovieControllerId(MCExecContext& ctxt, integer_t& r_id);
	void SetMovieControllerId(MCExecContext& ctxt, integer_t p_id);
	void GetPlayLoudness(MCExecContext& ctxt, uinteger_t& r_loudness);
	void SetPlayLoudness(MCExecContext& ctxt, uinteger_t p_loudness);
	void GetTrackCount(MCExecContext& ctxt, uinteger_t& r_count);
	void GetMediaTypes(MCExecContext& ctxt, intset_t& r_types);
	void GetCurrentNode(MCExecContext& ctxt, uinteger_t& r_node);
	void SetCurrentNode(MCExecContext& ctxt, uinteger_t p_node);
	void GetPan(MCExecContext& ctxt, double& r_pan);
	void SetPan(MCExecContext& ctxt, double p_pan);
	void GetTilt(MCExecContext& ctxt, double& r_tilt);
	void SetTilt(MCExecContext& ctxt, double p_tilt);
	void GetZoom(MCExecContext& ctxt, double& r_zoom);
	void SetZoom(MCExecContext& ctxt, double p_zoom);

	void GetAVITracks(MCExecContext& ctxt, MCStringRef& r_tracks);
	void GetX11Tracks(MCExecContext& ctxt, MCStringRef& r_tracks);
	void GetQTTracks(MCExecContext& ctxt, MCStringRef& r_tracks);
	void GetTracks(MCExecContext& ctxt, MCStringRef& r_tracks);

	void GetConstraints(MCExecContext& ctxt, MCMultimediaQTVRConstraints& r_constraints);
	void GetNodes(MCExecContext& ctxt, MCStringRef& r_nodes);
	void GetHotSpots(MCExecContext& ctxt, MCStringRef& r_spots);
    
    virtual void SetShowBorder(MCExecContext& ctxt, bool setting);
    virtual void SetBorderWidth(MCExecContext& ctxt, uinteger_t width);
    virtual void SetVisible(MCExecContext& ctxt, uinteger_t part, bool setting);
    virtual void SetInvisible(MCExecContext& ctxt, uinteger_t part, bool setting);
    virtual void SetTraversalOn(MCExecContext& ctxt, bool setting);

    void GetEnabledTracks(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tracks);
    
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
    void qt_enablekeys(Boolean enable);
    void qt_setcontrollervisible();
	Boolean qt_ispaused(void);
	void qt_showcontroller(Boolean show);
	MCRectangle qt_getpreferredrect(void);
	uint2 qt_getloudness(void);
	void qt_setloudness(uint2 loudn);
	void qt_gettracks(MCExecPoint& ep);
	void qt_getenabledtracks(MCExecPoint& ep);
    void qt_getenabledtracks(uindex_t& r_count, uinteger_t*& r_tracks);
	Boolean qt_setenabledtracks(const MCString& s);
	void qt_draw(MCDC *dc, const MCRectangle& dirty);
	void qt_move(int2 x, int2 y);
	void qt_click(bool p_state, uint4 p_button);
	void qt_key(bool p_state, uint4 p_key);

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
	void avi_gettracks(MCExecPoint& ep);
	void avi_getenabledtracks(MCExecPoint& ep);
	void avi_getenabledtracks(uindex_t& r_count, uinteger_t*& r_tracks);
	Boolean avi_setenabledtracks(const MCString& s);
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
	MCRectangle x11_getpreferredrect(void) {MCRectangle t_rect; t_rect.x = 0;t_rect.y = 0;t_rect.width=0;t_rect.height=0; return t_rect;}
	void x11_gettracks(MCExecPoint& ep) {}
	void x11_getenabledtracks(MCExecPoint& ep) {}
    void x11_getenabledtracks(uindex_t& r_count, uinteger_t*& r_tracks) {}
	Boolean x11_setenabledtracks(const MCString& s) {return False;}
	void x11_draw(MCDC *dc, const MCRectangle& dirty) {}
	
	pid_t getpid(void);
	void  shutdown(void);
#endif
};

#endif
