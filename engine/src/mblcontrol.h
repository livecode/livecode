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

#ifndef __MC_MOBILE_CONTROL__
#define __MC_MOBILE_CONTROL__
                                                          
////////////////////////////////////////////////////////////////////////////////

enum MCNativeControlType
{
	kMCNativeControlTypeUnknown,
	kMCNativeControlTypeBrowser,
	kMCNativeControlTypeScroller,
	kMCNativeControlTypePlayer,
	kMCNativeControlTypeInput,
	kMCNativeControlTypeMultiLineInput,
    kMCNativeControlType_Last,
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
	kMCNativeControlPropertyReadyForDisplay,
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
    kMCNativeControlPropertyMaximumTextLength,
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
    
    kMCNativeControlProperty_Last,
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
    
    kMCNativeControlAction_Last,
};

class MCNativeControl;
typedef bool (*MCNativeControlListCallback)(void *context, MCNativeControl *control);

struct MCNativeControlEnumEntry
{
	const char *key;
	int32_t value;
};

struct MCNativeControlColor
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;
};

void MCNativeControlColorParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlColor& r_output);
void MCNativeControlColorFormat(MCExecContext& ctxt, const MCNativeControlColor& p_input, MCStringRef& r_output);
void MCNativeControlColorFree(MCExecContext& ctxt, MCNativeControlColor& p_input);

enum MCNativeControlDecelerationRateType
{
    kMCNativeControlDecelerationRateNormal,
    kMCNativeControlDecelerationRateFast,
    kMCNativeControlDecelerationRateCustom
};

struct MCNativeControlDecelerationRate
{
    MCNativeControlDecelerationRateType type;
    double rate;
};


void MCNativeControlDecelerationRateParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlDecelerationRate& r_output);
void MCNativeControlDecelerationRateFormat(MCExecContext& ctxt, const MCNativeControlDecelerationRate& p_input, MCStringRef& r_output);
void MCNativeControlDecelerationRateFree(MCExecContext& ctxt, MCNativeControlDecelerationRate& p_input);

struct MCNativeControlIndicatorInsets
{
    bool has_insets;
    int16_t top;
    int16_t left;
    int16_t right;
    int16_t bottom;
};

void MCNativeControlIndicatorInsetsParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlIndicatorInsets& r_output);
void MCNativeControlIndicatorInsetsFormat(MCExecContext& ctxt, const MCNativeControlIndicatorInsets& p_input, MCStringRef& r_output);
void MCNativeControlIndicatorInsetsFree(MCExecContext& ctxt, MCNativeControlIndicatorInsets& p_input);

struct MCNativeControlRange
{
    uint32_t start;
    uint32_t length;
};

void MCNativeControlRangeParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlRange& r_output);
void MCNativeControlRangeFormat(MCExecContext& ctxt, const MCNativeControlRange& p_input, MCStringRef& r_output);
void MCNativeControlRangeFree(MCExecContext& ctxt, MCNativeControlRange& p_input);

enum MCNativeControlIndicatorStyle
{
    kMCNativeControlIndicatorStyleEmpty = 0,
    kMCNativeControlIndicatorStyleDefault,
    kMCNativeControlIndicatorStyleWhite,
    kMCNativeControlIndicatorStyleBlack
};

enum MCNativeControlLoadState
{
    kMCNativeControlLoadStateNoneBit,
    kMCNativeControlLoadStatePlayableBit,
    kMCNativeControlLoadStatePlaythroughOKBit,
    kMCNativeControlLoadStateStalledBit,
	
    kMCNativeControlLoadStateNone = 1 << kMCNativeControlLoadStateNoneBit,
    kMCNativeControlLoadStatePlayable = 1 << kMCNativeControlLoadStatePlayableBit,
    kMCNativeControlLoadStatePlaythroughOK = 1 << kMCNativeControlLoadStatePlaythroughOKBit,
    kMCNativeControlLoadStateStalled = 1 << kMCNativeControlLoadStateStalledBit
};

enum MCNativeControlPlaybackState
{
    kMCNativeControlPlaybackStateNone,
    kMCNativeControlPlaybackStateStopped,
    kMCNativeControlPlaybackStatePlaying,
    kMCNativeControlPlaybackStatePaused,
    kMCNativeControlPlaybackStateInterrupted,
    kMCNativeControlPlaybackStateSeekingForward,
    kMCNativeControlPlaybackStateSeekingBackward
};

enum MCNativeControlInputCapitalizationType
{
    kMCNativeControlInputCapitalizeNone,
    kMCNativeControlInputCapitalizeCharacters,
    kMCNativeControlInputCapitalizeWords,
    kMCNativeControlInputCapitalizeSentences
};

enum MCNativeControlInputAutocorrectionType
{
    kMCNativeControlInputAutocorrectionDefault,
    kMCNativeControlInputAutocorrectionNo,
    kMCNativeControlInputAutocorrectionYes
};

enum MCNativeControlInputKeyboardType
{
    kMCNativeControlInputKeyboardTypeDefault,
    kMCNativeControlInputKeyboardTypeAlphabet,
    kMCNativeControlInputKeyboardTypeNumeric,
    kMCNativeControlInputKeyboardTypeURL,
    kMCNativeControlInputKeyboardTypeNumber,
    kMCNativeControlInputKeyboardTypePhone,
    kMCNativeControlInputKeyboardTypeContact,
    kMCNativeControlInputKeyboardTypeEmail,
    kMCNativeControlInputKeyboardTypeDecimal,
};

enum MCNativeControlInputKeyboardStyle
{
    kMCNativeControlInputKeyboardStyleDefault,
    kMCNativeControlInputKeyboardStyleAlert
};

enum MCNativeControlInputReturnKeyType
{
    kMCNativeControlInputReturnKeyTypeDefault,
    kMCNativeControlInputReturnKeyTypeGo,
    kMCNativeControlInputReturnKeyTypeGoogle,
    kMCNativeControlInputReturnKeyTypeJoin,
    kMCNativeControlInputReturnKeyTypeNext,
    kMCNativeControlInputReturnKeyTypeRoute,
    kMCNativeControlInputReturnKeyTypeSearch,
    kMCNativeControlInputReturnKeyTypeSend,
    kMCNativeControlInputReturnKeyTypeYahoo,
    kMCNativeControlInputReturnKeyTypeDone,
    kMCNativeControlInputReturnKeyTypeEmergencyCall
};

enum MCNativeControlInputContentType
{
    kMCNativeControlInputContentTypePlain,
    kMCNativeControlInputContentTypePassword
};

enum MCNativeControlInputDataDetectorType
{
    kMCNativeControlInputDataDetectorTypeNoneBit = 0,
    kMCNativeControlInputDataDetectorTypeWebUrlBit,
    kMCNativeControlInputDataDetectorTypeEmailAddressBit,
    kMCNativeControlInputDataDetectorTypePhoneNumberBit,
    kMCNativeControlInputDataDetectorTypeMapAddressBit,
    kMCNativeControlInputDataDetectorTypeCalendarEventBit,
    kMCNativeControlInputDataDetectorTypeAllBit,
	
	kMCNativeControlInputDataDetectorTypeNone = 1 << kMCNativeControlInputDataDetectorTypeNoneBit,
    kMCNativeControlInputDataDetectorTypeWebUrl = 1 << kMCNativeControlInputDataDetectorTypeWebUrlBit,
    kMCNativeControlInputDataDetectorTypeEmailAddress = 1 << kMCNativeControlInputDataDetectorTypeEmailAddressBit,
    kMCNativeControlInputDataDetectorTypePhoneNumber = 1 << kMCNativeControlInputDataDetectorTypePhoneNumberBit,
    kMCNativeControlInputDataDetectorTypeMapAddress = 1 << kMCNativeControlInputDataDetectorTypeMapAddressBit,
    kMCNativeControlInputDataDetectorTypeCalendarEvent = 1 << kMCNativeControlInputDataDetectorTypeCalendarEventBit,
    kMCNativeControlInputDataDetectorTypeAll = 1 << kMCNativeControlInputDataDetectorTypeAllBit
};

enum MCNativeControlInputTextAlign
{
    kMCNativeControlInputTextAlignCenter,
    kMCNativeControlInputTextAlignLeft,
    kMCNativeControlInputTextAlignRight,
};

enum MCNativeControlClearButtonMode
{
    kMCNativeControlClearButtonModeNever,
    kMCNativeControlClearButtonModeWhileEditing,
    kMCNativeControlClearButtonModeUnlessEditing,
    kMCNativeControlClearButtonModeAlways,
};

enum MCNativeControlBorderStyle
{
    kMCNativeControlBorderStyleNone,
    kMCNativeControlBorderStyleLine,
    kMCNativeControlBorderStyleBezel,
    kMCNativeControlBorderStyleRoundedRect
};

enum MCNativeControlInputVerticalAlign
{
    kMCNativeControlInputVerticalAlignCenter,
    kMCNativeControlInputVerticalAlignTop,
    kMCNativeControlInputVerticalAlignBottom,
};

enum MCNativeControlActionSignature
{
    kMCNativeControlActionSignature_Void,
    kMCNativeControlActionSignature_String,
    kMCNativeControlActionSignature_OptInteger,
    kMCNativeControlActionSignature_String_String,
    kMCNativeControlActionSignature_Integer_Integer,
    kMCNativeControlActionSignature_Integer_OptInteger_OptInteger,
};

struct MCNativeControlActionInfo
{
	bool waitable;
    MCNativeControlAction action;
    MCNativeControlActionSignature signature;
    void *exec_method;
};

struct MCNativeControlActionTable
{
	MCNativeControlActionTable *parent;
	uindex_t size;
	MCNativeControlActionInfo *table;
};

enum MCNativeControlIdentifierType
{
    kMCNativeControlIdentifierName,
    kMCNativeControlIdentifierId,
};

struct MCNativeControlIdentifier
{
    MCNativeControlIdentifierType type;
    union
    {
        MCStringRef name;
        uint32_t id;
    };
};

void MCNativeControlIdentifierParse(MCExecContext& ctxt, MCStringRef p_input, MCNativeControlIdentifier& r_output);
void MCNativeControlIdentifierFormat(MCExecContext& ctxt, const MCNativeControlIdentifier& p_input, MCStringRef& r_output);
void MCNativeControlIdentifierFree(MCExecContext& ctxt, MCNativeControlIdentifier& p_input);

class MCNativeControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;

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
	void GetName(MCStringRef &r_name);
	
	// Set the native control's name
	bool SetName(MCStringRef name);
	
	// Get the owning object of the instance
	MCObject *GetOwner(void);
	
	// Set the owning object of the instance
	void SetOwner(MCObject *owner);
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void GetId(MCExecContext& ctxt, uinteger_t& r_id);
    void GetName(MCExecContext& ctxt, MCStringRef& r_name);
    void SetName(MCExecContext& ctxt, MCStringRef p_name);
    
    
	// The current target of any message that has been dispatched
	static MCNativeControl *ChangeTarget(MCNativeControl *target);
	static MCNativeControl *CurrentTarget(void);
	
	// Tokenization methods
	static bool LookupProperty(MCStringRef p_property, Properties& r_property);
	static bool LookupAction(MCStringRef p_action, MCNativeControlAction& r_action);
	static bool LookupType(MCStringRef p_type, MCNativeControlType& r_type);
	
	// Look for an instance either by name or id
	static bool FindByNameOrId(MCStringRef p_name_or_id, MCNativeControl*& r_control);
	// Look for an instance with a given id
	static bool FindById(uint32_t p_id, MCNativeControl*& r_control);
	
	// Iterate through all controls
	static bool List(MCNativeControlListCallback callback, void *context);
    static bool GetControlList(MCStringRef &r_list);
    
	// Create an instance with the given type
	static bool CreateWithType(MCNativeControlType p_type, MCNativeControl*& r_control);


	// Various helper functions
    // MM-2012-02-22: Clean up all controls
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
	MCStringRef m_name;
	// The instance's owning object (handle)
	MCObjectHandle m_object;
};

void MCNativeControlInitialize(void);
void MCNativeControlFinalize(void);

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
