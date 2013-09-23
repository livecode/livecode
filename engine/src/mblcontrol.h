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

////////////////////////////////////////////////////////////////////////////////
struct MCNativeControl;
struct MCNativeControlPtr
{
    MCNativeControl *control;
};

template<typename C, typename A, void (C::*Method)(MCExecContext&, A)> inline void MCPropertyNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr ctrl, A arg)
{
	(static_cast<C *>(ctrl . control) ->* Method)(ctxt, arg);
}

#define MCPropertyNativeControlThunkImp(ctrl, mth, typ) (void(*)(MCExecContext&,MCNativeControlPtr,typ))MCPropertyNativeControlThunk<ctrl,typ,&ctrl::mth>

#define MCPropertyNativeControlThunkGetAny(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCValueRef&)
#define MCPropertyNativeControlThunkGetBool(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, bool&)
#define MCPropertyNativeControlThunkGetInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t&)
#define MCPropertyNativeControlThunkGetInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t&)
#define MCPropertyNativeControlThunkGetInt32X2(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCPoint32&)
#define MCPropertyNativeControlThunkGetInt32X4(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle32&)
#define MCPropertyNativeControlThunkGetUInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t&)
#define MCPropertyNativeControlThunkGetUInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t&)
#define MCPropertyNativeControlThunkGetUInt32X4(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle32&)
#define MCPropertyNativeControlThunkGetOptionalInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t*&)
#define MCPropertyNativeControlThunkGetOptionalUInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t*&)
#define MCPropertyNativeControlThunkGetOptionalUInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t*&)
#define MCPropertyNativeControlThunkGetDouble(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, double&)
#define MCPropertyNativeControlThunkGetString(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCStringRef&)
#define MCPropertyNativeControlThunkGetOptionalString(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCStringRef&)
#define MCPropertyNativeControlThunkGetRectangle(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle&)
#define MCPropertyNativeControlThunkGetOptionalRectangle(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle*&)
#define MCPropertyNativeControlThunkGetPoint(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCPoint&)
#define MCPropertyNativeControlThunkGetCustomType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ&)
#define MCPropertyNativeControlThunkGetEnumType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ&)
#define MCPropertyNativeControlThunkGetSetType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ&)
#define MCPropertyNativeControlThunkGetOptionalCustomType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ*&)
#define MCPropertyNativeControlThunkGetOptionalEnumType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ*&)
#define MCPropertyNativeControlThunkGetArray(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCArrayRef&)

#define MCPropertyNativeControlThunkSetAny(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCValueRef)
#define MCPropertyNativeControlThunkSetBool(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, bool)
#define MCPropertyNativeControlThunkSetInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t)
#define MCPropertyNativeControlThunkSetInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t)
#define MCPropertyNativeControlThunkSetInt32X2(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCPoint32)
#define MCPropertyNativeControlThunkSetInt32X4(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle32)
#define MCPropertyNativeControlThunkSetUInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t)
#define MCPropertyNativeControlThunkSetUInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t)
#define MCPropertyNativeControlThunkSetUInt32X4(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle32)
#define MCPropertyNativeControlThunkSetOptionalInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t*)
#define MCPropertyNativeControlThunkSetOptionalUInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t*)
#define MCPropertyNativeControlThunkSetOptionalUInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t*)
#define MCPropertyNativeControlThunkSetDouble(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, double)
#define MCPropertyNativeControlThunkSetString(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCStringRef)
#define MCPropertyNativeControlThunkSetOptionalString(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCStringRef)
#define MCPropertyNativeControlThunkSetRectangle(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle)
#define MCPropertyNativeControlThunkSetOptionalRectangle(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle*)
#define MCPropertyNativeControlThunkSetPoint(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCPoint)
#define MCPropertyNativeControlThunkSetCustomType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, const typ&)
#define MCPropertyNativeControlThunkSetEnumType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ)
#define MCPropertyNativeControlThunkSetSetType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ)
#define MCPropertyNativeControlThunkSetOptionalCustomType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, const typ*&)
#define MCPropertyNativeControlThunkSetOptionalEnumType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ*)
#define MCPropertyNativeControlThunkSetArray(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCArrayRef)

#define DEFINE_RW_CTRL_PROPERTY(prop, type, ctrl, tag) \
{ kMCNativeControlProperty##prop, kMCNativeControlPropertyType##type, nil, (void *)MCPropertyNativeControlThunkGet##type(ctrl, Get##tag), (void *)MCPropertyNativeControlThunkSet##type(ctrl, Set##tag) },

#define DEFINE_RO_CTRL_PROPERTY(prop, type, ctrl, tag) \
{ kMCNativeControlProperty##prop, kMCNativeControlPropertyType##type, nil, (void *)MCPropertyNativeControlThunkGet##type(ctrl, Get##tag), nil },

#define DEFINE_RW_CTRL_CUSTOM_PROPERTY(prop, type, ctrl, tag) \
{ kMCNativeControlProperty##prop, kMCNativeControlPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetCustomType(ctrl, Get##tag, MC##type), (void *)MCPropertyNativeControlThunkSetCustomType(ctrl, Set##tag, MC##type) },

#define DEFINE_RO_CTRL_CUSTOM_PROPERTY(prop, type, ctrl, tag) \
{ kMCNativeControlProperty##prop, kMCNativeControlPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetCustomType(ctrl, Get##tag, MC##type), nil },

#define DEFINE_RO_CTRL_ENUM_PROPERTY(prop, type, ctrl, tag) \
{ kMCNativeControlProperty##prop, kMCNativeControlPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetEnumType(ctrl, Get##tag, MC##type), nil },

#define DEFINE_RW_CTRL_ENUM_PROPERTY(prop, type, ctrl, tag) \
{ kMCNativeControlProperty##prop, kMCNativeControlPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetEnumType(ctrl, Get##tag, MC##type), (void *)MCPropertyNativeControlThunkSetEnumType(ctrl, Set##tag, MC##type) },

#define DEFINE_UNAVAILABLE_CTRL_PROPERTY(prop) \
{ kMCNativeControlProperty##prop, kMCNativeControlPropertyTypeAny, nil, nil, nil },

#define DEFINE_RW_CTRL_SET_PROPERTY(prop, type, ctrl, tag) \
{ kMCNativeControlProperty##prop, kMCNativeControlPropertyTypeSet, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetSetType(ctrl, Get##tag, MC##type), (void *)MCPropertyNativeControlThunkSetSetType(ctrl, Set##tag, MC##type) },

#define DEFINE_RO_CTRL_SET_PROPERTY(prop, type, ctrl, tag) \
{ kMCNativeControlProperty##prop, kMCNativeControlPropertyTypeSet, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetSetType(ctrl, Get##tag, MC##type), nil },

template<typename C, typename X, typename Y, typename Z, void (C::*Method)(MCExecContext&, X, Y, Z)> inline void MCExecNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr ctrl, X param1, Y param2, Z param3)
{
	(static_cast<C *>(ctrl . control) ->* Method)(ctxt, param1, param2, param3);
}

template<typename C, typename X, typename Y, void (C::*Method)(MCExecContext&, X, Y)> inline void MCExecNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr ctrl, X param1, Y param2)
{
	(static_cast<C *>(ctrl . control) ->* Method)(ctxt, param1, param2);
}

template<typename C, typename X, void (C::*Method)(MCExecContext&, X)> inline void MCExecNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr ctrl, X param1)
{
	(static_cast<C *>(ctrl . control) ->* Method)(ctxt, param1);
}

template<typename C, void (C::*Method)(MCExecContext&)> inline void MCExecNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr ctrl)
{
	(static_cast<C *>(ctrl . control) ->* Method)(ctxt);
}

#define MCExecNativeControlThunkExec(ctrl, mth) (void(*)(MCExecContext&,MCNativeControlPtr))MCExecNativeControlThunk<ctrl,&ctrl::mth>

#define MCExecNativeControlUnaryThunkImp(ctrl, mth, typ) (void(*)(MCExecContext&,MCNativeControlPtr,typ))MCExecNativeControlThunk<ctrl,typ,&ctrl::mth>
#define MCExecNativeControlThunkExecString(ctrl, mth) MCExecNativeControlUnaryThunkImp(ctrl, mth, MCStringRef)
#define MCExecNativeControlThunkExecInt32(ctrl, mth) MCExecNativeControlUnaryThunkImp(ctrl, mth, integer_t)

#define MCExecNativeControlBinaryThunkImp(ctrl, mth, typ1, typ2) (void(*)(MCExecContext&,MCNativeControlPtr,typ1,typ2))MCExecNativeControlThunk<ctrl,typ1,typ2,&ctrl::mth>
#define MCExecNativeControlThunkExecStringString(ctrl, mth) MCExecNativeControlBinaryThunkImp(ctrl, mth, MCStringRef, MCStringRef)
#define MCExecNativeControlThunkExecInt32Int32(ctrl, mth) MCExecNativeControlBinaryThunkImp(ctrl, mth, integer_t, integer_t)

#define MCExecNativeControlTernaryThunkImp(ctrl, mth, typ1, typ2, typ3) (void(*)(MCExecContext&,MCNativeControlPtr,typ1,typ2,typ3))MCExecNativeControlThunk<ctrl,typ1,typ2,typ3,&ctrl::mth>
#define MCExecNativeControlThunkExecInt32OptionalInt32OptionalInt32(ctrl, mth) MCExecNativeControlTernaryThunkImp(ctrl, mth, integer_t, integer_t*, integer_t*)

#define DEFINE_CTRL_EXEC_METHOD(act, ctrl, tag) \
{ kMCNativeControlAction##act, (void *)MCExecNativeControlThunkExec(ctrl, Exec##tag) },

#define DEFINE_CTRL_EXEC_UNARY_METHOD(act, ctrl, param1, tag) \
{ kMCNativeControlAction##act, (void *)MCExecNativeControlThunkExec##param1(ctrl, Exec##tag) },

#define DEFINE_CTRL_EXEC_BINARY_METHOD(act, ctrl, param1, param2, tag) \
{ kMCNativeControlAction##act, (void *)MCExecNativeControlThunkExec##param1##param2(ctrl, Exec##tag) },

#define DEFINE_CTRL_EXEC_TERNARY_METHOD(act, ctrl, param1, param2, param3, tag) \
{ kMCNativeControlAction##act, (void *)MCExecNativeControlThunkExec##param1##param2##param3(ctrl, Exec##tag) },
                                                          
////////////////////////////////////////////////////////////////////////////////

enum MCNativeControlPropertyType
{
	kMCNativeControlPropertyTypeAny,
	kMCNativeControlPropertyTypeBool,
	kMCNativeControlPropertyTypeInt16,
	kMCNativeControlPropertyTypeInt32,
	kMCNativeControlPropertyTypeUInt16,
	kMCNativeControlPropertyTypeUInt32,
	kMCNativeControlPropertyTypeDouble,
	kMCNativeControlPropertyTypeChar,
	kMCNativeControlPropertyTypeString,
	kMCNativeControlPropertyTypeBinaryString,
	kMCNativeControlPropertyTypeColor,
	kMCNativeControlPropertyTypeRectangle,
	kMCNativeControlPropertyTypePoint,
	kMCNativeControlPropertyTypeInt16X2,
	kMCNativeControlPropertyTypeInt16X4,
	kMCNativeControlPropertyTypeInt32X2,
	kMCNativeControlPropertyTypeInt32X4,
	kMCNativeControlPropertyTypeUInt32X4,
	kMCNativeControlPropertyTypeArray,
	kMCNativeControlPropertyTypeSet,
	kMCNativeControlPropertyTypeEnum,
	kMCNativeControlPropertyTypeCustom,
	kMCNativeControlPropertyTypeOptionalInt16,
	kMCNativeControlPropertyTypeOptionalUInt16,
	kMCNativeControlPropertyTypeOptionalUInt32,
	kMCNativeControlPropertyTypeOptionalString,
	kMCNativeControlPropertyTypeOptionalRectangle,
	kMCNativeControlPropertyTypeOptionalEnum,
};

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
    kMCNativeControlInputDataDetectorTypeNone = 0,
    kMCNativeControlInputDataDetectorTypeWebUrlBit,
    kMCNativeControlInputDataDetectorTypeEmailAddressBit,
    kMCNativeControlInputDataDetectorTypePhoneNumberBit,
    kMCNativeControlInputDataDetectorTypeMapAddressBit,
    kMCNativeControlInputDataDetectorTypeCalendarEventBit,
    kMCNativeControlInputDataDetectorTypeAllBit,
	
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

struct MCNativeControlPropertyInfo
{
	MCNativeControlProperty property;
	MCNativeControlPropertyType type;
	void *type_info;
	void *getter;
	void *setter;
};

struct MCNativeControlPropertyTable
{
	MCNativeControlPropertyTable *parent;
	uindex_t size;
	MCNativeControlPropertyInfo *table;
};

struct MCNativeControlActionInfo
{
    MCNativeControlAction action;
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
	static MCNativeControlPropertyInfo kProperties[];
	static MCNativeControlPropertyTable kPropertyTable;
    
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
	MCStringRef GetName(void);
	
	// Set the native control's name
	bool SetName(MCStringRef name);
	
	// Get the owning object of the instance
	MCObject *GetOwner(void);
	
	// Set the owning object of the instance
	void SetOwner(MCObject *owner);
#ifdef LEGACY_EXEC	
	// Set property/get property/do verb.
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep) = 0;
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep) = 0;	
    virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters) = 0;
#endif

    virtual const MCNativeControlPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void GetId(MCExecContext& ctxt, uinteger_t& r_id);
    void GetName(MCExecContext& ctxt, MCStringRef& r_name);
    void SetName(MCExecContext& ctxt, MCStringRef p_name);
    
    
	// The current target of any message that has been dispatched
	static MCNativeControl *ChangeTarget(MCNativeControl *target);
	static MCNativeControl *CurrentTarget(void);
	
	// Tokenization methods
	static bool LookupProperty(MCStringRef p_property, MCNativeControlProperty& r_property);
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
	MCObjectHandle *m_object;    
};

void MCNativeControlInitialize(void);
void MCNativeControlFinalize(void);

bool MCExecPointSetRect(MCExecPoint &ep, int2 p_left, int2 p_top, int2 p_right, int2 p_bottom);

#endif // __MC_MOBILE_CONTROL__