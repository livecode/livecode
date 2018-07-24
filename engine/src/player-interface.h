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

#ifndef PLAYER_INTERFACE_H
#define PLAYER_INTERFACE_H

typedef uint4 MCPlayerMediaTypeSet;
typedef uint64_t MCPlayerDuration;

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
#endif

enum MCInterfacePlayerStatus
{
    kMCInterfacePlayerStatusNone,
    kMCInterfacePlayerStatusLoading,
    kMCInterfacePlayerStatusPlaying,
    kMCInterfacePlayerStatusPaused,
};

// SN-2014-07-03: [[ PlatformPlayer ]] Player constraints must be the same for
// new platform or old player
struct MCMultimediaQTVRConstraints;

// SN-2014-07-02: [[ PlatformPlayer ]]
// Needed to refactor the new players, since we need to have the property table
// MCPlayerInterface comprises all the former MCPlayer functions appearing as well
// in the new PlatformPlayer.
class MCPlayerInterface
{
protected:
	MCStringRef filename;
    MCStringRef resolved_filename;
	uint2 framerate;
	Boolean disposable;
	Boolean istmpfile;
	real8 scale;
	real8 rate;
	MCPlayerDuration starttime;
	MCPlayerDuration endtime;
	MCStringRef userCallbackStr;  //string contains user movie callbacks
	uint2 formattedwidth;
	uint2 formattedheight;
	uint2 loudness;
    MCPlayerDuration lasttime;
    Boolean dontuseqt;
    Boolean usingqt;
	
    double m_left_balance;
    double m_right_balance;
    double m_audio_pan;
    
public:
    MCPlayerInterface(){};
    ~MCPlayerInterface(){};
    
	virtual bool getversion(MCStringRef& r_string) = 0;
	virtual void freetmp() = 0;
	virtual MCPlayerDuration getduration() = 0;    //get movie duration/length
	virtual MCPlayerDuration gettimescale() = 0;  //get movie time scale
	virtual MCPlayerDuration getmoviecurtime() = 0;//get movie current time
	virtual void setcurtime(MCPlayerDuration curtime, bool notify) = 0;
	virtual void setselection(bool notify) = 0;                  //set movie selection
	virtual void setlooping(Boolean loop) = 0;        //to loop or not to loop a movie
	virtual void setplayrate() = 0;                   //set the movie playing rate
	virtual void showbadge(Boolean show) = 0;         //show & hide the movie's badge
	virtual void showcontroller(Boolean show) = 0;
	virtual void editmovie(Boolean edit) = 0;
	virtual void playselection(Boolean play) = 0;     //play the selected part of QT moive only
	virtual Boolean ispaused() = 0;

    virtual void gettracks(MCStringRef& r_tracks) = 0;
    
	virtual MCRectangle getpreferredrect() = 0;
	virtual uint2 getloudness() = 0;
	virtual void setloudness() = 0;
	virtual double getleftbalance() = 0;
	virtual void setleftbalance(double p_left_balance) = 0;
	virtual double getrightbalance() = 0;
	virtual void setrightbalance(double p_right_balance) = 0;
	virtual double getaudiopan() = 0;
	virtual void setaudiopan(double p_pan) = 0;

    Boolean isdisposable()
    {
        return disposable;
    }
	
	void setscale(real8 &s)
	{
		scale = s;
	}
    
    virtual void setdontuseqt(bool p_dont_use_qt) { dontuseqt = p_dont_use_qt; }
    virtual void getdontuseqt(bool &r_dont_use_qt) { r_dont_use_qt = dontuseqt; }
    
	virtual Boolean prepare(MCStringRef options) = 0;
	virtual Boolean playstart(MCStringRef options) = 0;
	virtual Boolean playpause(Boolean on) = 0;
    virtual void playstepforward() = 0;
    virtual void playstepback() = 0;
	virtual Boolean playstop() = 0;
	virtual void setvolume(uint2 tloudness) = 0;
	virtual void setfilename(MCStringRef vcname, MCStringRef fname, Boolean istmp) = 0;
    
	MCPlayerDuration getstarttime()
	{
		return starttime;
	}
	MCPlayerDuration getendtime()
	{
		return endtime;
	}
	MCPlayerDuration getlasttime()
	{
		return lasttime;
	}
    
	virtual void setstarttime(MCPlayerDuration stime) = 0;
	virtual void setendtime(MCPlayerDuration etime) = 0;
	virtual void setlasttime(MCPlayerDuration ltime) = 0;
    
    
	virtual void syncbuffering(MCContext *dc) = 0;
    
    virtual Boolean isbuffering() = 0;
    
    // SN-2014-07-03: [[ PlatformPlayer ]]
    // The functions from exec-interface-player.cpp should
    // not bother anymore with the kind of player in use.
    // Missing functions added here.
    virtual void setcallbacks(MCStringRef p_callbacks) = 0;
    
    virtual void setmoviecontrollerid(integer_t p_id) = 0;
    virtual integer_t getmoviecontrollerid() = 0;
    virtual integer_t getmediatypes() = 0;
    virtual uinteger_t getcurrentnode() = 0;
    virtual bool changecurrentnode(uinteger_t p_node_id) = 0;
	virtual real8 getpan() = 0;
	virtual bool changepan(real8 pan) = 0;
	virtual real8 gettilt() = 0;
	virtual bool changetilt(real8 tilt) = 0;
	virtual real8 getzoom() = 0;
	virtual bool changezoom(real8 zoom) = 0;

    virtual uinteger_t gettrackcount() = 0;
    virtual void getnodes(MCStringRef &r_nodes) = 0;
    virtual void gethotspots(MCStringRef &r_nodes) = 0;
    virtual void getconstraints(MCMultimediaQTVRConstraints &r_constraints) = 0;
    virtual void getenabledtracks(uindex_t &r_count, uint32_t *&r_tracks_id) = 0;
    virtual void setenabledtracks(uindex_t p_count, uint32_t *p_tracks_id) = 0;
    
    virtual void updatevisibility() = 0;
    virtual void updatetraversal() = 0;

    // SN-2015-01-06: [[ Merge-6.7.2-rc-1 ]] Update to MCStringRef
    virtual bool resolveplayerfilename(MCStringRef p_filename, MCStringRef &r_filename) = 0;
    
	////////// PROPERTY SUPPORT METHODS
    
	virtual void Redraw(void) = 0;
    
	////////// PROPERTY ACCESSORS
    
	virtual void GetFileName(MCExecContext& ctxt, MCStringRef& r_name) = 0;
	virtual void SetFileName(MCExecContext& ctxt, MCStringRef p_name) = 0;
	virtual void GetDontRefresh(MCExecContext& ctxt, bool& r_setting) = 0;
	virtual void SetDontRefresh(MCExecContext& ctxt, bool setting) = 0;
	virtual void GetCurrentTime(MCExecContext& ctxt, double& r_time) = 0;
	virtual void SetCurrentTime(MCExecContext& ctxt, double p_time) = 0;
	virtual void GetDuration(MCExecContext& ctxt, double& r_duration) = 0;
    // PM-2014-11-03: [[ Bug 13920 ]] Make sure we support loadedTime property
    virtual void GetLoadedTime(MCExecContext& ctxt, double& r_loaded_time) = 0;
	virtual void GetLooping(MCExecContext& ctxt, bool& r_setting) = 0;
	virtual void SetLooping(MCExecContext& ctxt, bool setting) = 0;
	virtual void GetPaused(MCExecContext& ctxt, bool& r_setting) = 0;
	virtual void SetPaused(MCExecContext& ctxt, bool setting) = 0;
	virtual void GetAlwaysBuffer(MCExecContext& ctxt, bool& r_setting) = 0;
	virtual void SetAlwaysBuffer(MCExecContext& ctxt, bool setting) = 0;
	virtual void GetPlayRate(MCExecContext& ctxt, double& r_rate) = 0;
	virtual void SetPlayRate(MCExecContext& ctxt, double p_rate) = 0;
	virtual void GetStartTime(MCExecContext& ctxt, double*& r_time) = 0;
	virtual void SetStartTime(MCExecContext& ctxt, double* p_time) = 0;
	virtual void GetEndTime(MCExecContext& ctxt, double*& r_time) = 0;
	virtual void SetEndTime(MCExecContext& ctxt, double* p_time) = 0;
	virtual void GetShowBadge(MCExecContext& ctxt, bool& r_setting) = 0;
	virtual void SetShowBadge(MCExecContext& ctxt, bool setting) = 0;
	virtual void GetShowController(MCExecContext& ctxt, bool& r_setting) = 0;
	virtual void SetShowController(MCExecContext& ctxt, bool setting) = 0;
	virtual void GetPlaySelection(MCExecContext& ctxt, bool& r_setting) = 0;
	virtual void SetPlaySelection(MCExecContext& ctxt, bool setting) = 0;
	virtual void GetShowSelection(MCExecContext& ctxt, bool& r_setting) = 0;
	virtual void SetShowSelection(MCExecContext& ctxt, bool setting) = 0;
	virtual void GetCallbacks(MCExecContext& ctxt, MCStringRef& r_callbacks) = 0;
	virtual void SetCallbacks(MCExecContext& ctxt, MCStringRef p_callbacks) = 0;
	virtual void GetTimeScale(MCExecContext& ctxt, double& r_scale) = 0;
	virtual void GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height) = 0;
	virtual void GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width) = 0;
	virtual void GetMovieControllerId(MCExecContext& ctxt, integer_t& r_id) = 0;
	virtual void SetMovieControllerId(MCExecContext& ctxt, integer_t p_id) = 0;
	virtual void GetPlayLoudness(MCExecContext& ctxt, uinteger_t& r_loudness) = 0;
	virtual void SetPlayLoudness(MCExecContext& ctxt, uinteger_t p_loudness) = 0;
	virtual void GetTrackCount(MCExecContext& ctxt, uinteger_t& r_count) = 0;
	virtual void GetMediaTypes(MCExecContext& ctxt, intset_t& r_types) = 0;
	virtual void GetCurrentNode(MCExecContext& ctxt, uinteger_t& r_node) = 0;
	virtual void SetCurrentNode(MCExecContext& ctxt, uinteger_t p_node) = 0;
	virtual void GetPan(MCExecContext& ctxt, double& r_pan) = 0;
	virtual void SetPan(MCExecContext& ctxt, double p_pan) = 0;
	virtual void GetTilt(MCExecContext& ctxt, double& r_tilt) = 0;
	virtual void SetTilt(MCExecContext& ctxt, double p_tilt) = 0;
	virtual void GetZoom(MCExecContext& ctxt, double& r_zoom) = 0;
	virtual void SetZoom(MCExecContext& ctxt, double p_zoom) = 0;
	virtual void GetLeftBalance(MCExecContext& ctxt, double &r_left_balance) = 0;
	virtual void SetLeftBalance(MCExecContext& ctxt, double p_left_balance) = 0;
	virtual void GetRightBalance(MCExecContext& ctxt, double &r_right_balance) = 0;
	virtual void SetRightBalance(MCExecContext& ctxt, double p_right_balance) = 0;
	virtual void GetAudioPan(MCExecContext& ctxt, double &r_pan) = 0;
	virtual void SetAudioPan(MCExecContext& ctxt, double p_pan) = 0;

	virtual void GetTracks(MCExecContext& ctxt, MCStringRef& r_tracks) = 0;
    
	virtual void GetConstraints(MCExecContext& ctxt, MCMultimediaQTVRConstraints& r_constraints) = 0;
	virtual void GetNodes(MCExecContext& ctxt, MCStringRef& r_nodes) = 0;
	virtual void GetHotSpots(MCExecContext& ctxt, MCStringRef& r_spots) = 0;
    
    virtual void SetShowBorder(MCExecContext& ctxt, bool setting) = 0;
    virtual void SetBorderWidth(MCExecContext& ctxt, uinteger_t width) = 0;
    virtual void SetVisible(MCExecContext& ctxt, uinteger_t part, bool setting) = 0;
    virtual void SetTraversalOn(MCExecContext& ctxt, bool setting) = 0;
    
    virtual void GetEnabledTracks(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tracks) = 0;
    virtual void SetEnabledTracks(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_tracks) = 0;
    
    virtual void GetDontUseQT(MCExecContext& ctxt, bool &p_dont_use_qt) = 0;
    virtual void SetDontUseQT(MCExecContext& ctxt, bool r_dont_use_qt) = 0;
};


#endif
