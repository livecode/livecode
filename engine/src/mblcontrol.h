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

#ifndef __MC_MOBILE_CONTROL__
#define __MC_MOBILE_CONTROL__

enum MCNativeControlType
{
	kMCNativeControlTypeUnknown,
	kMCNativeControlTypeBrowser,
	kMCNativeControlTypeScroller,
	kMCNativeControlTypePlayer,
	kMCNativeControlTypeInput,
	kMCNativeControlTypeMultiLineInput,
};

enum MCNativeControlProperty
{
	kMCNativeControlPropertyUnknown,
	
	// Built-in properties
	kMCNativeControlPropertyId,
	kMCNativeControlPropertyName,
	
	// Common UIView properties
	kMCNativeControlPropertyRectangle,
	kMCNativeControlPropertyVisible,
	kMCNativeControlPropertyOpaque,
	kMCNativeControlPropertyAlpha,
	kMCNativeControlPropertyBackgroundColor,
    kMCNativeControlPropertyIgnoreVoiceOverSensitivity,
    
	
	// Browser / Text view properties
	kMCNativeControlPropertyDataDetectorTypes,
	kMCNativeControlPropertyAutoFit,
	
	// Browser-specific properties
	kMCNativeControlPropertyUrl,
	kMCNativeControlPropertyCanRetreat,
	kMCNativeControlPropertyCanAdvance,
	kMCNativeControlPropertyDelayRequests,
	kMCNativeControlPropertyAllowsInlineMediaPlayback,
	kMCNativeControlPropertyMediaPlaybackRequiresUserAction,
	
	// Scroller-specific properties
	kMCNativeControlPropertyContentRectangle,
	kMCNativeControlPropertyCanBounce,
	kMCNativeControlPropertyVScroll,
	kMCNativeControlPropertyHScroll,
	kMCNativeControlPropertyCanScrollToTop,
	kMCNativeControlPropertyCanCancelTouches,
	kMCNativeControlPropertyDelayTouches,
	kMCNativeControlPropertyDecelerationRate,
	kMCNativeControlPropertyIndicatorStyle,
	kMCNativeControlPropertyIndicatorInsets,
	kMCNativeControlPropertyPagingEnabled,
	kMCNativeControlPropertyScrollingEnabled,
	kMCNativeControlPropertyShowHorizontalIndicator,
	kMCNativeControlPropertyShowVerticalIndicator,
	kMCNativeControlPropertyLockDirection,
	kMCNativeControlPropertyTracking,
	kMCNativeControlPropertyDragging,
	kMCNativeControlPropertyDecelerating,
	
	// Player-specific properties
	kMCNativeControlPropertyContent,
	kMCNativeControlPropertyNaturalSize,
	kMCNativeControlPropertyFullscreen,
	kMCNativeControlPropertyPreserveAspect,
	kMCNativeControlPropertyShowController,
	kMCNativeControlPropertyUseApplicationAudioSession,
	kMCNativeControlPropertyDuration,
	kMCNativeControlPropertyPlayableDuration,
	kMCNativeControlPropertyLoadState,
	kMCNativeControlPropertyPlaybackState,
	kMCNativeControlPropertyStartTime,
	kMCNativeControlPropertyEndTime,
	kMCNativeControlPropertyCurrentTime,
	kMCNativeControlPropertyShouldAutoplay,
	kMCNativeControlPropertyLooping,
	kMCNativeControlPropertyAllowsAirPlay,
	kMCNativeControlPropertyPlayRate,
	kMCNativeControlPropertyIsPreparedToPlay,
	
	// Control-specific properties
	kMCNativeControlPropertyEnabled,
	
	// Input-specific properties
	kMCNativeControlPropertyText,
	kMCNativeControlPropertyUnicodeText,
	kMCNativeControlPropertyTextColor,
	kMCNativeControlPropertyTextAlign,
	kMCNativeControlPropertyFontName,
	kMCNativeControlPropertyFontSize,
	kMCNativeControlPropertyEditing,
	
	// TextField-specific properties
	kMCNativeControlPropertyMinimumFontSize,
	kMCNativeControlPropertyAutoClear,
	kMCNativeControlPropertyClearButtonMode,
	kMCNativeControlPropertyBorderStyle,
	kMCNativeControlPropertyVerticalTextAlign,
	
	// TextView-specific properties
	kMCNativeControlPropertyEditable,
	kMCNativeControlPropertySelectedRange,
	
	// TextInputTraits properties
	kMCNativeControlPropertyAutoCapitalizationType,
	kMCNativeControlPropertyAutoCorrectionType,
	kMCNativeControlPropertyManageReturnKey,
	kMCNativeControlPropertyKeyboardStyle,
	kMCNativeControlPropertyKeyboardType,
	kMCNativeControlPropertyReturnKeyType,
	kMCNativeControlPropertyContentType,
    
    // Android specific properties
    kMCNativeControlPropertyMultiLine,
};

enum MCNativeControlAction
{
	kMCNativeControlActionUnknown,
	
	// Common actions
	kMCNativeControlActionStop,
	
	// Browser-specific actions
	kMCNativeControlActionAdvance,
	kMCNativeControlActionRetreat,
	kMCNativeControlActionReload,
	kMCNativeControlActionExecute,
	kMCNativeControlActionLoad,
	
	// Scroller-specific actions
	kMCNativeControlActionFlashScrollIndicators,
	
	// Player-specific actions
	kMCNativeControlActionPlay,
	kMCNativeControlActionPause,
	kMCNativeControlActionPrepareToPlay,
	kMCNativeControlActionBeginSeekingBackward,
	kMCNativeControlActionBeginSeekingForward,
	kMCNativeControlActionEndSeeking,
	kMCNativeControlActionSnapshot,
	kMCNativeControlActionSnapshotExactly,
	
	// Input-specific actions
	kMCNativeControlActionFocus,
	
	// TextView-specific actions
	kMCNativeControlActionScrollRangeToVisible,
};

class MCNativeControl;
typedef bool (*MCNativeControlListCallback)(void *context, MCNativeControl *control);

struct MCNativeControlEnumEntry
{
	const char *key;
	int32_t value;
};

class MCNativeControl
{
public:
	// Increment/decrement reference count. This count is used to control the
	// lifetime of the MCNativeControl instance. This prevents an instance
	// being deleted while it is being referenced on the stack.
	void Retain(void);
	void Release(void);
	
	// Make sure the instance has a view and is active.
	virtual bool Create(void) = 0;
	// Delete the view associated with the control. This is usually called
	// in response to a 'delete control' call.
	virtual void Delete(void) = 0;
	
	// Get the type of the native control
	virtual MCNativeControlType GetType(void) = 0;
	
	// Get the native control id of the instance.
	uint32_t GetId(void);
	
	// Get the native control's name (if any)
	const char *GetName(void);
	
	// Set the native control's name
	bool SetName(const char *name);
	
	// Get the owning object of the instance
	MCObject *GetOwner(void);
	
	// Set the owning object of the instance
	void SetOwner(MCObject *owner);
	
	// Set property/get property/do verb.
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep) = 0;
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep) = 0;	
	virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters) = 0;
    
	// The current target of any message that has been dispatched
	static MCNativeControl *ChangeTarget(MCNativeControl *target);
	static MCNativeControl *CurrentTarget(void);
	
	// Tokenization methods
	static bool LookupProperty(const char *property, MCNativeControlProperty& r_property);
	static bool LookupAction(const char *action, MCNativeControlAction& r_action);
	static bool LookupType(const char *type, MCNativeControlType& r_type);
	
	// Look for an instance either by name or id
	static bool FindByNameOrId(const char *name_or_id, MCNativeControl*& r_control);
	// Look for an instance with a given id
	static bool FindById(uint32_t p_id, MCNativeControl*& r_control);
	
	// Iterate through all controls
	static bool List(MCNativeControlListCallback callback, void *context);
	
	// Create an instance with the given type
	static bool CreateWithType(MCNativeControlType p_type, MCNativeControl*& r_control);


	// Various helper functions
	static bool ParseColor(MCExecPoint& ep, uint16_t &r_red, uint16_t &r_green, uint16_t &r_blue, uint16_t &r_alpha);
	static bool FormatColor(MCExecPoint& ep, uint16_t p_red, uint16_t p_green, uint16_t p_blue, uint16_t p_alpha);
    
	static bool ParseBoolean(MCExecPoint& ep, bool& r_value);
	static bool FormatBoolean(MCExecPoint& ep, bool value);
	
	static bool ParseInteger(MCExecPoint& ep, int32_t& r_value);
	static bool FormatInteger(MCExecPoint& ep, int32_t value);
	
	static bool ParseUnsignedInteger(MCExecPoint& ep, uint32_t& r_value);
	static bool FormatUnsignedInteger(MCExecPoint& ep, uint32_t value);
	
	static bool ParseReal(MCExecPoint& ep, double& r_real);
	static bool FormatReal(MCExecPoint& ep, double real);
	
    static bool ParseEnum(MCExecPoint& ep, MCNativeControlEnumEntry *p_entries, int32_t& r_value);
    static bool FormatEnum(MCExecPoint& ep, MCNativeControlEnumEntry *p_entries, int32_t p_value);

	static bool ParseSet(MCExecPoint& ep, MCNativeControlEnumEntry *entries, int32_t& r_value);
	static bool FormatSet(MCExecPoint& ep, MCNativeControlEnumEntry *entries, int32_t value);
	
	static bool ParseRectangle(MCExecPoint& ep, MCRectangle& r_rect);
	static bool ParseRectangle32(MCExecPoint& ep, MCRectangle32& r_rect);
    
    static bool ParseRange(MCExecPoint &ep, uint32_t &r_start, uint32_t &r_length);
    static bool FormatRange(MCExecPoint &ep, uint32_t p_start, uint32_t p_length);

    // MM-2012-02-22: Clean up all ads
    static void Finalize(void);

protected:
	// Constructor is not available to the outside.
	MCNativeControl(void);
	// Destructor is not available to the outside.
	virtual ~MCNativeControl(void);
    
    // MM-2012-06-12: [[ Bug 10203]] Flag controls as deleted so that they are removed from control lists (but still being retained elsewhere)
    bool m_deleted;

private:
	// The chain of controls
	MCNativeControl *m_next;
	// The reference count for the instance
	uint32_t m_references;
	// The id of the instance
	uint32_t m_id;
	// The name of the instance
	char *m_name;
	// The instance's owning object (handle)
	MCObjectHandle *m_object;    
};

void MCNativeControlInitialize(void);
void MCNativeControlFinalize(void);

bool MCExecPointSetRect(MCExecPoint &ep, int2 p_left, int2 p_top, int2 p_right, int2 p_bottom);

// MM-2013-11-26: [[ Bug 11485 ]] Added functions for converting between user and device space.
MCGAffineTransform MCNativeControlUserToDeviceTransform();
MCGAffineTransform MCNativeControlUserFromDeviceTransform();
MCGRectangle MCNativeControlUserRectToDeviceRect(const MCGRectangle &p_user_rect);
MCGRectangle MCNativeControlUserRectFromDeviceRect(const MCGRectangle &p_device_rect);
MCGPoint MCNativeControlUserPointToDevicePoint(const MCGPoint &p_user_point);
MCGPoint MCNativeControlUserPointFromDevicePoint(const MCGPoint &p_device_point);
int32_t MCNativeControlUserXLocToDeviceXLoc(int32_t p_user_x_loc);
int32_t MCNativeControlUserXLocFromDeviceXLoc(int32_t p_device_x_loc);
int32_t MCNativeControlUserYLocToDeviceYLoc(int32_t p_user_y_loc);
int32_t MCNativeControlUserYLocFromDeviceYLoc(int32_t p_device_y_loc);

#endif // __MC_MOBILE_CONTROL__