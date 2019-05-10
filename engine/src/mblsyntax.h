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
    kMCPhotoSourceTypeCamera,
	kMCPhotoSourceTypeFrontCamera,
	kMCPhotoSourceTypeRearCamera,
};
/*
void MCCameraExecAcquirePhotoAndResize(MCExecContext& ctxt, MCPhotoSourceType p_source, int32_t p_max_width, int32_t p_max_height);
void MCCameraExecAcquirePhoto(MCExecContext& ctxt, MCPhotoSourceType p_photo);
*/
enum MCCameraSourceType
{
	kMCCameraSourceTypeUnknown,
	kMCCameraSourceTypeFront,
	kMCCameraSourceTypeRear,
};

enum
{
    kMCCameraFeatureTypePhoto,
    kMCCameraFeatureTypeVideo,
    kMCCameraFeatureTypeFlash,
    kMCCameraFeatureRearShift = 3,
};

typedef uint32_t MCCameraFeaturesType;
enum
{
	kMCCameraFeaturePhoto = 1 << kMCCameraFeatureTypePhoto,
	kMCCameraFeatureVideo = 1 << kMCCameraFeatureTypeVideo,
	kMCCameraFeatureFlash = 1 << kMCCameraFeatureTypeFlash,
};

typedef uint32_t MCCamerasFeaturesType;
enum
{
    kMCCamerasFeatureFrontPhoto = 1 << kMCCameraFeatureTypePhoto,
    kMCCamerasFeatureFrontVideo = 1 << kMCCameraFeatureTypeVideo,
    kMCCamerasFeatureFrontFlash = 1 << kMCCameraFeatureTypeFlash,
    kMCCamerasFeatureRearPhoto = 1 << (kMCCameraFeatureTypePhoto + kMCCameraFeatureRearShift),
    kMCCamerasFeatureRearVideo = 1 << (kMCCameraFeatureTypeVideo + kMCCameraFeatureRearShift),
    kMCCamerasFeatureRearFlash = 1 << (kMCCameraFeatureTypeFlash + kMCCameraFeatureRearShift),
};

//void MCCameraGetFeatures(MCExecContext& ctxt, MCCameraSourceType p_camera, char*& r_features);
MCCameraFeaturesType MCSystemGetSpecificCameraFeatures(MCCameraSourceType p_source);
MCCamerasFeaturesType MCSystemGetAllCameraFeatures();

bool MCSystemCanAcquirePhoto(MCPhotoSourceType p_source);
bool MCSystemAcquirePhoto(MCPhotoSourceType p_source, int32_t p_max_width, int32_t p_max_height, void*& r_image_data, size_t& r_image_data_size, MCStringRef& r_result);

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

bool MCSystemBusyIndicatorStart (intenum_t p_indicator, MCStringRef p_label, int32_t p_opacity);
bool MCSystemBusyIndicatorStop ();
bool MCSystemActivityIndicatorStart (intenum_t p_indicator, integer_t p_location_x, integer_t p_location_y);
bool MCSystemActivityIndicatorStop ();

///////////////////////////////////////////////////////////////////////////////
// from Dialog module

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

typedef uint32_t MCMediaType;
enum
{
    kMCUnknownMediaType = 0,
    kMCMediaTypePodcasts =         1 << 0,
    kMCMediaTypeSongs =            1 << 1,
    kMCMediaTypeAudiobooks =       1 << 2,
    kMCMediaTypeMovies =           1 << 3,
    kMCMediaTypeMusicVideos =      1 << 4,
    kMCMediaTypeTv =               1 << 5,
    kMCMediaTypeVideoPodcasts =    1 << 6,
    kMCMediaTypeAnyAudio =         kMCMediaTypePodcasts + kMCMediaTypeSongs + kMCMediaTypeAudiobooks,
    kMCMediaTypeAnyVideo =         kMCMediaTypeMovies + kMCMediaTypeMusicVideos + kMCMediaTypeTv + kMCMediaTypeVideoPodcasts,
};

typedef uint32_t MCMediaScope;
enum
{
    kMCunknownMediaScope,
    kMCaudio,
    kMCmedia,
};

///////////////////////////////////////////////////////////////////////////////
// from Alert

//void MCBeepExec (MCExecContext& ctxt);
//void MCVibrateExec (MCExecContext& ctxt);

///////////////////////////////////////////////////////////////////////////////
// from Local Notification

//void MCLocalNotificationExec (MCExecContext& p_ctxt, const char *p_alert_body, const char *p_alert_action, const char *p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value);
//void MCGetRegisteredNotificationsExec (MCExecContext& p_ctxt);
//void MCCancelLocalNotificationExec (MCExecContext& p_ctxt, const char *t_alert_descriptor);
//void MCCancelAllLocalNotificationsExec (MCExecContext& p_ctxt);
//void MCGetNotificationBadgeValueExec (MCExecContext& p_ctxt);
//void MCSetNotificationBadgeValueExec (MCExecContext p_ctxt);

///////////////////////////////////////////////////////////////////////////////
// from Push Notification

//void MCGetDeviceTokenExec (MCExecContext& p_ctxt);

///////////////////////////////////////////////////////////////////////////////
// from Custom URL Schemes

//void MCGetLaunchUrlExec (MCExecContext& p_ctxt);

///////////////////////////////////////////////////////////////////////////////
// from Text Messaging

bool MCSystemCanSendTextMessage();
bool MCSystemComposeTextMessage(MCStringRef p_recipients, MCStringRef p_body);

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
    kMCSoundAudioCategoryUnknown,
    kMCSoundAudioCategoryAmbient,
    kMCSoundAudioCategorySoloAmbient,
    kMCSoundAudioCategoryPlayback,
    kMCSoundAudioCategoryRecord,
    kMCSoundAudioCategoryPlayAndRecord,
    kMCSoundAudioCategoryAudioProcessing
};

bool MCSystemSoundInitialize();
bool MCSystemSoundFinalize();
bool MCSystemPlaySoundOnChannel(MCStringRef p_channel, MCStringRef p_file, MCSoundChannelPlayType p_type, MCObjectHandle p_object);
bool MCSystemStopSoundChannel(MCStringRef p_channel);
bool MCSystemPauseSoundChannel(MCStringRef p_channel);
bool MCSystemResumeSoundChannel(MCStringRef p_channel);
bool MCSystemDeleteSoundChannel(MCStringRef p_channel);
bool MCSystemSetSoundChannelVolume(MCStringRef p_channel, int32_t p_volume);
bool MCSystemSoundChannelVolume(MCStringRef p_channel, int32_t& r_volume);
bool MCSystemSoundChannelStatus(MCStringRef p_channel, intenum_t& r_status);
bool MCSystemSoundOnChannel(MCStringRef p_channel, MCStringRef &r_sound);
bool MCSystemNextSoundOnChannel(MCStringRef p_channel, MCStringRef &r_sound);
bool MCSystemListSoundChannels(MCStringRef &r_channels);
bool MCSystemSetAudioCategory(intenum_t p_category);


///////////////////////////////////////////////////////////////////////////////
// from Calendar

typedef struct
{
    MCStringRef   mceventid;           //
    MCStringRef   mctitle;             //                                     RW
    MCStringRef   mcnote;              //                                     RW
    MCStringRef   mclocation;          //                                     RW
    bool          mcalldayset;         //
    bool          mcallday;            //                                     RW
    bool          mcstartdateset;      //
    MCDateTime    mcstartdate;         //                                     RW
    bool          mcenddateset;        //
    MCDateTime    mcenddate;           //                                     RW
    int           mcalert1;            //                                     RW
    int           mcalert2;            //                                     RW
    MCStringRef   mcfrequency;         // EKReocurrenceFrequency              RW
    int           mcfrequencycount;    //                                     RW
    int           mcfrequencyinterval; //                                     RW
    MCStringRef   mccalendar;          //
} MCCalendar;

//bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
bool MCCalendarToArrayData (MCExecContext &r_ctxt, MCCalendar p_contact, MCArrayRef &r_result);
bool MCArrayDataToCalendar (MCArrayRef p_array, MCCalendar& r_calendar);
bool MCSystemShowEvent(MCStringRef p_event_id, MCStringRef& r_result);
bool MCSystemCreateEvent(MCStringRef& r_result);
bool MCSystemUpdateEvent(MCStringRef p_event_id, MCStringRef& r_result);
bool MCSystemGetEventData(MCExecContext &r_ctxt, MCStringRef p_event_id, MCArrayRef& r_event_data);
bool MCSystemRemoveEvent(MCStringRef p_event_id, bool p_reocurring, MCStringRef& r_event_id_deleted);
bool MCSystemAddEvent(MCCalendar p_new_calendar_data, MCStringRef& r_result);
bool MCSystemGetCalendarsEvent(MCStringRef& r_result);
bool MCSystemFindEvent(MCDateTime p_start_date, MCDateTime p_end_date, MCStringRef& r_result);

///////////////////////////////////////////////////////////////////////////////
// from Ad module

class MCAd;

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

bool MCSystemInneractiveAdCreate(MCExecContext &ctxt, MCAd*& r_ad, MCAdType p_type, uint32_t p_top_left_x, uint32_t p_top_left_y, uint32_t p_timeout, MCArrayRef p_meta_data);

//void MCAdExecRegisterWithInneractive(MCExecContext& ctxt, const char *p_key);
//void MCAdExecCreateAd(MCExecContext& ctxt, const char *p_name, MCAdType p_type, MCAdTopLeft p_top_left, MCVariableValue *p_meta_data);
//void MCAdExecDeleteAd(MCExecContext& ctxt, const char *p_name);
//
//bool MCAdGetTopLeftOfAd(MCExecContext& ctxt, const char *p_name, MCAdTopLeft &r_top_left);
//bool MCAdGetVisibleOfAd(MCExecContext& ctxt, const char *p_name, bool &r_visible);
//bool MCAdGetAds(MCExecContext& ctxt, char*& r_ads);
//
//void MCAdSetTopLeftOfAd(MCExecContext& ctxt, const char *p_name, MCAdTopLeft p_top_left);
//void MCAdSetVisibleOfAd(MCExecContext& ctxt, const char *p_name, bool p_visible);

///////////////////////////////////////////////////////////////////////////////
// from Orientation module

typedef enum
{
	ORIENTATION_UNKNOWN_BIT = 0,
    ORIENTATION_PORTRAIT_BIT = 1,
    ORIENTATION_PORTRAIT_UPSIDE_DOWN_BIT = 2,
    ORIENTATION_LANDSCAPE_RIGHT_BIT = 3,
    ORIENTATION_LANDSCAPE_LEFT_BIT = 4,
    ORIENTATION_FACE_UP_BIT = 5,
    ORIENTATION_FACE_DOWN_BIT = 6,
} MCOrientation;

typedef enum
{
	ORIENTATION_UNKNOWN = 1 << ORIENTATION_UNKNOWN_BIT,
	ORIENTATION_PORTRAIT = 1 << ORIENTATION_PORTRAIT_BIT,
	ORIENTATION_PORTRAIT_UPSIDE_DOWN = 1 << ORIENTATION_PORTRAIT_UPSIDE_DOWN_BIT,
	ORIENTATION_LANDSCAPE_RIGHT = 1 << ORIENTATION_LANDSCAPE_RIGHT_BIT,
	ORIENTATION_LANDSCAPE_LEFT = 1 << ORIENTATION_LANDSCAPE_LEFT_BIT,
	ORIENTATION_FACE_UP = 1 << ORIENTATION_FACE_UP_BIT,
	ORIENTATION_FACE_DOWN = 1 << ORIENTATION_FACE_DOWN_BIT
} MCOrientationSet;

void MCSystemGetAllowedOrientations(uint32_t& r_orientations);
void MCSystemSetAllowedOrientations(uint32_t p_orientations);
void MCSystemGetOrientation(MCOrientation& r_orientation);
void MCSystemGetDeviceOrientation(MCOrientation& r_orientation);
void MCSystemLockOrientation();
void MCSystemUnlockOrientation();
void MCSystemGetOrientationLocked(bool &r_locked);

///////////////////////////////////////////////////////////////////////////////
// from Mail module

enum MCMailType
{
	kMCMailTypePlain,
	kMCMailTypeUnicode,
	kMCMailTypeHtml
};

struct MCAttachmentData
{
	MCDataRef data;
	MCStringRef file;
	MCStringRef type;
	MCStringRef name;
};

void MCSystemMailResult(MCStringRef& r_result);
void MCSystemSendMail(MCStringRef p_address, MCStringRef p_cc_address, MCStringRef p_subject, MCStringRef p_message_body, MCStringRef& r_result);
void MCSystemSendMailWithAttachments(MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCMailType p_type, MCAttachmentData *p_attachments, uindex_t p_attachment_count, MCStringRef& r_result);
void MCSystemGetCanSendMail(bool& r_result);


////////////////////////////////////////////////////////////////////////////////
// from Idle Timer

void MCSystemLockIdleTimer();
void MCSystemUnlockIdleTimer();
bool MCSystemIdleTimerLocked();

///////////////////////////////////////////////////////////////////////////////
// from Dialog module

typedef enum
{
    kMCPickButtonNone,
    kMCPickButtonCancel,
    kMCPickButtonDone,
    kMCPickButtonCancelAndDone
} MCPickButtonType;

struct MCPickList
{
    MCStringRef *options;
    uindex_t option_count;
    uindex_t initial;
};

bool MCSystemPickDate(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect);
bool MCSystemPickTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect);
bool MCSystemPickDateAndTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect);

bool MCSystemPickOption(MCPickList *p_pick_lists, uindex_t p_pick_list_count, uindex_t *&r_result, uindex_t &r_result_count, bool p_use_hilited, bool p_use_picker, bool p_use_cancel, bool p_use_done, bool &r_canceled, MCRectangle p_button_rect);

bool MCSystemPickMedia(MCMediaType p_media_type, bool p_multiple, MCStringRef& r_result);


////////////////////////////////////////////////////////////////////////////////
// From Notification module

typedef struct
{
    MCStringRef body;
    MCStringRef action;
    MCStringRef user_info;
    uint32_t time; // in seconds
    uint32_t badge_value;
    bool play_sound;
} MCNotification;

bool MCSystemCreateLocalNotification (MCStringRef p_alert_body, MCStringRef p_alert_action, MCStringRef p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value, int32_t &r_id);
bool MCSystemGetRegisteredNotifications (MCStringRef& r_registered_alerts);
bool MCSystemGetNotificationDetails(int32_t p_id, MCNotification &r_notification);
bool MCSystemCancelLocalNotification(uint32_t p_alert_descriptor);
bool MCSystemCancelAllLocalNotifications ();
bool MCSystemGetNotificationBadgeValue (uint32_t &r_badge_value);
bool MCSystemSetNotificationBadgeValue (uint32_t r_badge_value);

////////////////////////////////////////////////////////////////////////////////
// From Misc module

bool MCSystemGetDeviceToken (MCStringRef& r_device_token);
bool MCSystemGetLaunchUrl (MCStringRef& r_launch_url);

bool MCSystemGetLaunchData(MCArrayRef &r_lauch_data);

bool MCSystemBeep(int32_t p_number_of_times);
bool MCSystemVibrate(int32_t p_number_of_times);

bool MCSystemGetDeviceResolution(MCStringRef& p_resolution);
bool MCSystemGetPixelDensity(real64_t& r_density);
bool MCSystemSetDeviceUseResolution(bool p_use_device_res, bool p_use_control_device_res);
bool MCSystemGetDeviceScale(real64_t& r_scale);

bool MCSystemSetStatusBarStyle(intenum_t p_style);
bool MCSystemShowStatusBar();
bool MCSystemHideStatusBar();

bool MCSystemSetKeyboardType(intenum_t p_type);
bool MCSystemSetKeyboardReturnKey(intenum_t p_type);
bool MCSystemSetKeyboardDisplay(intenum_t p_type);
bool MCSystemGetKeyboardDisplay(intenum_t& r_type);

bool MCSystemGetPreferredLanguages(MCStringRef& r_preferred_languages);
bool MCSystemGetCurrentLocale(MCStringRef& r_current_locale);

bool MCSystemClearTouches();

bool MCSystemGetSystemIdentifier(MCStringRef& r_identifier);
bool MCSystemGetApplicationIdentifier(MCStringRef& r_identifier);
bool MCSystemGetIdentifierForVendor(MCStringRef& r_identifier);

bool MCSystemSetReachabilityTarget(MCStringRef p_hostname);
bool MCSystemGetReachabilityTarget(MCStringRef& r_hostname);

// SN-2014-12-18: [[ Bug 13860 ]] Parameter added in case it's a filename, not raw data, in the DataRef
bool MCSystemExportImageToAlbum(MCStringRef& r_save_result, MCDataRef p_raw_data, MCStringRef p_file_name, MCStringRef p_file_extension, bool p_is_raw_data = true);

bool MCSystemSetRedrawInterval(int32_t p_interval);
bool MCSystemSetAnimateAutorotation(bool p_enabled);

bool MCSystemFileSetDoNotBackup(MCStringRef p_path, bool p_no_backup);
bool MCSystemFileGetDoNotBackup(MCStringRef p_path, bool& r_no_backup);
bool MCSystemFileSetDataProtection(MCStringRef p_path, MCStringRef p_protection_string, MCStringRef& r_status);
bool MCSystemFileGetDataProtection(MCStringRef p_path, MCStringRef& r_protection_string);

bool MCSystemBuildInfo(MCStringRef p_key, MCStringRef& r_value);
bool MCSystemRequestPermission(MCStringRef p_permission, bool& r_granted);
bool MCSystemPermissionExists(MCStringRef p_permission, bool& r_exists);
bool MCSystemHasPermission(MCStringRef p_permission, bool& r_permission_granted);

bool MCSystemEnableRemoteControl();
bool MCSystemDisableRemoteControl();
bool MCSystemGetRemoteControlEnabled(bool& r_enabled);
bool MCSystemSetRemoteControlDisplayProperties(MCExecContext& ctxt, MCArrayRef p_array);

// SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]]
bool MCSystemGetIsVoiceOverRunning(bool& r_is_vo_running);

enum MCMiscStatusBarStyle
{
    kMCMiscStatusBarStyleDefault,
    kMCMiscStatusBarStyleTranslucent,
    kMCMiscStatusBarStyleOpaque,
    kMCMiscStatusBarStyleSolid
};

////////////////////////////////////////////////////////////////////////////////
// From Url module

bool MCSystemLaunchUrl(MCStringRef p_url);

////////////////////////////////////////////////////////////////////////////////
// From NFC module

bool MCSystemNFCIsAvailable(void);
bool MCSystemNFCIsEnabled(void);

void MCSystemEnableNFCDispatch(void);
void MCSystemDisableNFCDispatch(void);

////////////////////////////////////////////////////////////////////////////////

#endif
