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

#ifndef __MBL_SYNTAX__
#define __MBL_SYNTAX__

#ifndef __MC_EXEC__
#include "exec.h"
#endif

#ifndef __DATETIME__
#include "date.h"
#endif

///////////////////////////////////////////////////////////////////////////////
// from Camera module

enum MCPhotoSourceType
{
	kMCPhotoSourceTypeUnknown,
	kMCPhotoSourceTypeAlbum,
	kMCPhotoSourceTypeLibrary,
	kMCPhotoSourceTypeFrontCamera,
	kMCPhotoSourceTypeRearCamera,
};

void MCCameraExecAcquirePhotoAndResize(MCExecContext& ctxt, MCPhotoSourceType p_source, int32_t p_max_width, int32_t p_max_height);
void MCCameraExecAcquirePhoto(MCExecContext& ctxt, MCPhotoSourceType p_photo);

enum MCCameraSourceType
{
	kMCCameraSourceTypeUnknown,
	kMCCameraSourceTypeFront,
	kMCCameraSourceTypeRear,
};

typedef uint32_t MCCameraFeaturesType;
enum
{
	kMCCameraFeaturePhoto = 1 << 0,
	kMCCameraFeatureVideo = 1 << 1,
	kMCCameraFeatureFlash = 1 << 2,
};

void MCCameraGetFeatures(MCExecContext& ctxt, MCCameraSourceType p_camera, char*& r_features);

///////////////////////////////////////////////////////////////////////////////
// from Sensor module

enum MCSensorType
{
    kMCSensorTypeUnknown,
    kMCSensorTypeLocation,
    kMCSensorTypeHeading,
    kMCSensorTypeAcceleration,
    kMCSensorTypeRotationRate,
};

void MCSensorExecStartTrackingSensor(MCExecContext& ctxt, MCSensorType p_sensor, bool p_loosely);
void MCSensorExecStopTrackingSensor(MCExecContext& ctxt, MCSensorType p_sensor);

void MCSensorGetSensorAvailable(MCExecContext& ctxt, MCSensorType p_sensor);

void MCSensorGetDetailedLocation(MCExecContext& ctxt, MCVariableValue *&r_detailed_location);
void MCSensorGetLocation(MCExecContext& ctxt, char *&r_location);
void MCSensorGetDetailedHeading(MCExecContext& ctxt, MCVariableValue *&r_detailed_heading);
void MCSensorGetHeading(MCExecContext& ctxt, char *&r_heading);
void MCSensorGetDetailedAcceleration(MCExecContext& ctxt, MCVariableValue *&r_detailed_acceleration);
void MCSensorGetAcceleration(MCExecContext& ctxt, char *&r_acceleration);
void MCSensorGetDetailedRotationRate(MCExecContext& ctxt, MCVariableValue *&r_detailed_rotation_rate);
void MCSensorGetRotationRate(MCExecContext& ctxt, char *&r_rotation_rate);

void MCSensorSetLocationCalibration(MCExecContext& ctxt, int32_t p_timeout);
void MCSensorGetLocationCalibration(MCExecContext& ctxt, int32_t& r_timeout);

///////////////////////////////////////////////////////////////////////////////
// from Busy Indicator

typedef uint32_t MCBusyIndicatorType;
enum
{
	kMCBusyIndicatorInLine,     // Provides a spinning wheel to the left with text on the right. The background is 50% translucent with bezeled edges      
	kMCBusyIndicatorSquare,     // Provides a spinning wheel in the middle top and text underneth. The background is 50% translucent with bezeled edges
	kMCBusyIndicatorKeyboard,   // Provides a spinning wheel in the middle of the keyboard and text underneth. The background is 50% translucent
};

void MCBusyIndicatorExecStart (MCExecContext& ctxt, MCBusyIndicatorType p_indicator, const char *p_label);
void MCBusyIndicatorExecStop (MCExecContext& ctxt);

struct MCLocation
{
	int32_t x; // The X location
    int32_t y; // The Y location
};

typedef uint32_t MCActivityIndicatorType;
enum
{
	kMCActivityIndicatorWhite,      // This is the default legacy version with no text or background
	kMCActivityIndicatorWhiteLarge, // This is a legacy version with no text or background
	kMCActivityIndicatorGray,       // This is a legacy version with no text or background
};

void MCActivityIndicatorExecStart (MCExecContext& ctxt, MCActivityIndicatorType p_indicator, MCLocation p_location);
void MCActivityIndicatorExecStop (MCExecContext& ctxt);

///////////////////////////////////////////////////////////////////////////////
// from Dialog module

void MCDialogExecPickDate(MCExecContext& ctxt, MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, bool p_cancel_button, bool p_done_button, MCRectangle p_button_rect);
void MCDialogExecPickTime(MCExecContext& ctxt, MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_cancel_button, bool p_done_button, MCRectangle p_button_rect);
void MCDialogExecPickDateAndTime(MCExecContext& ctxt, MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_cancel_button, bool p_done_button, MCRectangle p_button_rect);

struct const_int32_array_t
{
    int32_t *elements;
    uint32_t length;
};

struct const_cstring_array_t
{
    char **elements;
    uint32_t length;
};

typedef uint32_t MCChunkType;
enum
{
    kMCWords,
    kMCLines,
    kMCItems,    
};

void MCDialogExecPickOption(MCExecContext &p_ctxt, MCChunkType chunk_type, const_cstring_array_t *option_lists, const char *initial_choice, bool p_use_hilite_type, bool p_use_picker, bool p_use_cancel, bool p_use_done, char *&r_picked_options);
void MCDialogExecPickOptionByIndex(MCExecContext &p_ctxt, MCChunkType chunk_type, const_cstring_array_t *option_lists, const_int32_array_t initial_indices, bool p_use_hilite_type, bool p_use_picker, bool p_use_cancel, bool p_use_done, const_int32_array_t &r_picked_options);

typedef uint32_t MCMediaType;
enum
{
    kMCunknownMediaType = 0,
    kMCpodcasts =         1 << 0,
    kMCsongs =            1 << 1,
    kMCaudiobooks =       1 << 2,
    kMCmovies =           1 << 3,
    kMCmusicvideos =      1 << 4,
    kMCtv =               1 << 5,
    kMCvideopodcasts =    1 << 6,
};

typedef uint32_t MCMediaScope;
enum
{
    kMCunknownMediaScope,
    kMCaudio,
    kMCmedia,
};

void MCDialogExecPickMedia(MCExecContext &p_ctxt, MCMediaType *p_media_type, bool p_multiple);
void MCDialogExecPickMedia(MCExecContext &p_ctxt, char *p_media_expression, MCMediaScope *p_media_scope, bool p_multiple);

///////////////////////////////////////////////////////////////////////////////
// from Alert

void MCBeepExec (MCExecContext& ctxt);
void MCVibrateExec (MCExecContext& ctxt);

///////////////////////////////////////////////////////////////////////////////
// from Local Notification

void MCLocalNotificationExec (MCExecContext& p_ctxt, const char *p_alert_body, const char *p_alert_action, const char *p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value);
void MCGetRegisteredNotificationsExec (MCExecContext& p_ctxt);
void MCCancelLocalNotificationExec (MCExecContext& p_ctxt, const char *t_alert_descriptor);
void MCCancelAllLocalNotificationsExec (MCExecContext& p_ctxt);
void MCGetNotificationBadgeValueExec (MCExecContext& p_ctxt);
void MCSetNotificationBadgeValueExec (MCExecContext p_ctxt);

///////////////////////////////////////////////////////////////////////////////
// from Push Notification

void MCGetDeviceTokenExec (MCExecContext& p_ctxt);

///////////////////////////////////////////////////////////////////////////////
// from Custom URL Schemes

void MCGetLaunchUrlExec (MCExecContext& p_ctxt);

///////////////////////////////////////////////////////////////////////////////
// from Text Messaging

void MCCanSendTextMessageExec(MCExecContext& ctxt, bool& r_can_send);
void MCComposeTextMessageExec(MCExecContext& ctxt, const char *p_recipients, const char *p_body);

///////////////////////////////////////////////////////////////////////////////
// from Sound module

enum MCSoundChannelPlayType
{
	kMCSoundChannelPlayNow,
	kMCSoundChannelPlayNext,
	kMCSoundChannelPlayLooping
};

enum MCSoundChannelStatus
{
	kMCSoundChannelStatusStopped,
	kMCSoundChannelStatusPaused,
	kMCSoundChannelStatusPlaying
};

// MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
enum MCSoundAudioCategory
{
    kMCMCSoundAudioCategoryUnknown,
    kMCMCSoundAudioCategoryAmbient,
    kMCMCSoundAudioCategorySoloAmbient,
    kMCMCSoundAudioCategoryPlayback,
    kMCMCSoundAudioCategoryRecord,
    kMCMCSoundAudioCategoryPlayAndRecord,
    kMCMCSoundAudioCategoryAudioProcessing
};

void MCSoundExecPlaySoundOnChannel(MCExecContext& ctxt, const char *p_channel, const char *p_file, MCSoundChannelPlayType p_type);
void MCSoundExecStopSoundOnChannel(MCExecContext& ctxt, const char *p_channel);
void MCSoundExecPauseSoundOnChannel(MCExecContext& ctxt, const char *p_channel);
void MCSoundExecResumeSoundOnChannel(MCExecContext& ctxt, const char *p_channel);
void MCSoundExecDeleteChannel(MCExecContext& ctxt, const char *p_channel);

void MCSoundSetVolumeOfChannel(MCExecContext& ctxt, const char *p_channel, int32_t p_volume);

bool MCSoundGetVolumeOfChannel(MCExecContext& ctxt, const char *p_channel, int32_t& r_volume);
bool MCSoundGetStatusOfChannel(MCExecContext& ctxt, const char *p_channel, MCSoundChannelStatus& r_status);
bool MCSoundGetSoundOfChannel(MCExecContext& ctxt, const char *p_channel, char*& r_sound);
bool MCSoundGetNextSoundOfChannel(MCExecContext& ctxt, const char *p_channel, char*& r_sound);
bool MCSoundGetSoundChannels(MCExecContext& ctxt, char*& r_channels);

bool MCSoundSetAudioCategory(MCExecContext &ctxt, MCSoundAudioCategory p_category);

///////////////////////////////////////////////////////////////////////////////
// from Ad module

enum MCAdType {
    kMCAdTypeUnknown,
	kMCAdTypeBanner,
	kMCAdTypeText,
	kMCAdTypeFullscreen
};

struct MCAdTopLeft {
    uint32_t x;
    uint32_t y;
};

void MCAdExecRegisterWithInneractive(MCExecContext& ctxt, const char *p_key);
void MCAdExecCreateAd(MCExecContext& ctxt, const char *p_name, MCAdType p_type, MCAdTopLeft p_top_left, MCVariableValue *p_meta_data);
void MCAdExecDeleteAd(MCExecContext& ctxt, const char *p_name);

bool MCAdGetTopLeftOfAd(MCExecContext& ctxt, const char *p_name, MCAdTopLeft &r_top_left);
bool MCAdGetVisibleOfAd(MCExecContext& ctxt, const char *p_name, bool &r_visible);
bool MCAdGetAds(MCExecContext& ctxt, char*& r_ads);

void MCAdSetTopLeftOfAd(MCExecContext& ctxt, const char *p_name, MCAdTopLeft p_top_left);
void MCAdSetVisibleOfAd(MCExecContext& ctxt, const char *p_name, bool p_visible);

///////////////////////////////////////////////////////////////////////////////

#endif
