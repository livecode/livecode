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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"

#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "exec.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "mbliphonecontrol.h"
#include "mbliphone.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
UIView *MCIPhoneGetView(void);

////////////////////////////////////////////////////////////////////////////////

class MCiOSInputControl;

// Note that we use the notifications, rather than delegate methods.
@interface com_runrev_livecode_MCiOSInputDelegate : NSObject <UITextFieldDelegate, UITextViewDelegate>
{
	MCiOSInputControl *m_instance;
	bool m_didchange_pending;
	bool m_return_pressed;
    NSUInteger m_max_text_length;
}

- (id)initWithInstance:(MCiOSInputControl*)instance view:(UIView *)view;
- (void)dealloc;

- (void)didBeginEditing: (NSNotification *)notification;
- (void)didEndEditing: (NSNotification *)notification;
- (void)didChange: (NSNotification *)notification;

- (BOOL)isDidChangePending;
- (void)setDidChangePending: (BOOL)pending;

- (NSUInteger)getMaxTextLength;
- (void)setMaxTextLength: (NSUInteger)p_length;

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string;
- (BOOL)textFieldShouldReturn: (UITextField *)p_field;


- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text;
@end


@interface com_runrev_livecode_MCiOSMultiLineDelegate : com_runrev_livecode_MCiOSInputDelegate <UIScrollViewDelegate>
{
    UIView* m_view;
	int32_t m_verticaltextalign;
}

- (void)scrollViewWillBeginDragging: (UIScrollView*)scrollView;
- (void)scrollViewDidEndDragging: (UIScrollView*)scrollView willDecelerate:(BOOL)decelerate;
- (void)scrollViewDidScroll: (UIScrollView*)scrollView;
- (void)scrollViewDidScrollToTop: (UIScrollView*)scrollView;
- (void)scrollViewWillBeginDecelerating:(UIScrollView *)scrollView;
- (void)scrollViewDidEndDecelerating: (UIScrollView*)scrollView;

- (void)setVerticalTextAlign: (int32_t)align;
- (int32_t)getVerticalTextAlign;

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context;
@end

// superclass of text input type controls
class MCiOSInputControl: public MCiOSControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;
    
public:
	MCiOSInputControl(void);
    
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void SetEnabled(MCExecContext& ctxt, bool p_value);
    virtual void SetOpaque(MCExecContext& ctxt, bool p_value);
    void SetText(MCExecContext& ctxt, MCStringRef p_string);
    void SetTextColor(MCExecContext& ctxt, const MCNativeControlColor& p_color);
    void SetFontName(MCExecContext& ctxt, MCStringRef p_font);
    void SetFontSize(MCExecContext& ctxt, uinteger_t p_size);
    void SetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign p_align);

    void SetAutoCapitalizationType(MCExecContext& ctxt, MCNativeControlInputCapitalizationType p_type);
    void SetAutoCorrectionType(MCExecContext& ctxt, MCNativeControlInputAutocorrectionType p_type);
    void SetManageReturnKey(MCExecContext& ctxt, bool p_value);
    void SetKeyboardType(MCExecContext& ctxt, MCNativeControlInputKeyboardType p_type);
    void SetKeyboardStyle(MCExecContext& ctxt, MCNativeControlInputKeyboardStyle p_style);
    void SetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType p_key);
    void SetContentType(MCExecContext& ctxt, MCNativeControlInputContentType p_type);

    void GetEnabled(MCExecContext& ctxt, bool& r_value);
    void GetText(MCExecContext& ctxt, MCStringRef& r_string);
    void GetTextColor(MCExecContext& ctxt, MCNativeControlColor& r_color);
    void GetFontName(MCExecContext& ctxt, MCStringRef& r_font);
    void GetFontSize(MCExecContext& ctxt, uinteger_t& r_size);
    void GetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign& r_align);

    void GetAutoCapitalizationType(MCExecContext& ctxt, MCNativeControlInputCapitalizationType& r_type);
    void GetAutoCorrectionType(MCExecContext& ctxt, MCNativeControlInputAutocorrectionType& r_type);
    void GetKeyboardType(MCExecContext& ctxt, MCNativeControlInputKeyboardType& r_type);
    void GetManageReturnKey(MCExecContext& ctxt, bool& r_value);
    void GetKeyboardStyle(MCExecContext& ctxt, MCNativeControlInputKeyboardStyle& r_style);
    void GetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType& r_key);
    void GetContentType(MCExecContext& ctxt, MCNativeControlInputContentType& r_type);
    
	// Input-specific actions
	void ExecFocus(MCExecContext& ctxt);
    
	void HandleNotifyEvent(MCNameRef p_notification);

	com_runrev_livecode_MCiOSInputDelegate *GetDelegate(void);
	
protected:
	virtual ~MCiOSInputControl(void);
	
	com_runrev_livecode_MCiOSInputDelegate *m_delegate;
};

// single line input control
class MCiOSInputFieldControl: public MCiOSInputControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;
    
public:
	virtual MCNativeControlType GetType(void);
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void SetAutoFit(MCExecContext& ctxt, bool p_value);
    void SetMinimumFontSize(MCExecContext& ctxt, integer_t p_size);
    void SetMaximumTextLength(MCExecContext& ctxt, uinteger_t p_length);
    void SetAutoClear(MCExecContext& ctxt, bool p_value);
    void SetClearButtonMode(MCExecContext& ctxt, MCNativeControlClearButtonMode p_mode);
    void SetBorderStyle(MCExecContext& ctxt, MCNativeControlBorderStyle p_style);
    
    void GetAutoFit(MCExecContext& ctxt, bool& r_value);
    void GetMinimumFontSize(MCExecContext& ctxt, integer_t& r_size);
    void GetMaximumTextLength(MCExecContext& ctxt, uinteger_t& r_length);
    void GetAutoClear(MCExecContext& ctxt, bool& r_value);
    void GetClearButtonMode(MCExecContext& ctxt, MCNativeControlClearButtonMode& r_mode);
    void GetBorderStyle(MCExecContext& ctxt, MCNativeControlBorderStyle& r_style);
    
protected:
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
};

// multiline input control
class MCiOSMultiLineControl: public MCiOSInputControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;

public:
	MCiOSMultiLineControl(void)
	{
		m_post_scroll_event = true;
		m_content_rect.x = 0;
		m_content_rect.y = 0;
		m_content_rect.width = 0;
		m_content_rect.height = 0;
	}
	
	virtual MCNativeControlType GetType(void);
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
	virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void SetEditable(MCExecContext& ctxt, bool p_value);
    void SetSelectedRange(MCExecContext& ctxt, const MCNativeControlRange& p_range);
    void SetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType p_type);
    void SetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign p_align);
    virtual void SetContentRect(MCExecContext& ctxt, integer_t p_rect[4]);
    
    void GetEditable(MCExecContext& ctxt, bool& r_value);
    void GetSelectedRange(MCExecContext& ctxt, MCNativeControlRange& r_range);
    void GetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType& r_type);
    void GetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign& r_align);
    
    void GetContentRect(MCExecContext& ctxt, integer_t r_rect[4]);
    void SetHScroll(MCExecContext& ctxt, integer_t p_scroll);
    void GetHScroll(MCExecContext& ctxt, integer_t& r_scroll);
    void SetVScroll(MCExecContext& ctxt, integer_t p_scroll);
    void GetVScroll(MCExecContext& ctxt, integer_t& r_scroll);
    void SetCanBounce(MCExecContext& ctxt, bool p_value);
    void GetCanBounce(MCExecContext& ctxt, bool& r_value);
    void SetCanScrollToTop(MCExecContext& ctxt, bool p_value);
    void GetCanScrollToTop(MCExecContext& ctxt, bool& r_value);
    void SetCanCancelTouches(MCExecContext& ctxt, bool p_value);
    void GetCanCancelTouches(MCExecContext& ctxt, bool& r_value);
    void SetDelayTouches(MCExecContext& ctxt, bool p_value);
    void GetDelayTouches(MCExecContext& ctxt, bool& r_value);
    void SetScrollingEnabled(MCExecContext& ctxt, bool p_value);
    void GetScrollingEnabled(MCExecContext& ctxt, bool& r_value);
    void SetPagingEnabled(MCExecContext& ctxt, bool p_value);
    void GetPagingEnabled(MCExecContext& ctxt, bool& r_value);
    void SetDecelerationRate(MCExecContext& ctxt, const MCNativeControlDecelerationRate& p_rate);
    void GetDecelerationRate(MCExecContext& ctxt, MCNativeControlDecelerationRate& r_rate);
    void SetIndicatorStyle(MCExecContext& ctxt, MCNativeControlIndicatorStyle p_style);
    void GetIndicatorStyle(MCExecContext& ctxt, MCNativeControlIndicatorStyle& r_style);
    void SetIndicatorInsets(MCExecContext& ctxt, const MCNativeControlIndicatorInsets& p_insets);
    void GetIndicatorInsets(MCExecContext& ctxt, MCNativeControlIndicatorInsets& r_insets);
    void SetShowHorizontalIndicator(MCExecContext& ctxt, bool p_value);
    void GetShowHorizontalIndicator(MCExecContext& ctxt, bool& r_value);
    void SetShowVerticalIndicator(MCExecContext& ctxt, bool p_value);
    void GetShowVerticalIndicator(MCExecContext& ctxt, bool& r_value);
    void SetLockDirection(MCExecContext& ctxt, bool p_value);
    void GetLockDirection(MCExecContext& ctxt, bool& r_value);
    
    void GetTracking(MCExecContext& ctxt, bool& r_value);
    void GetDragging(MCExecContext& ctxt, bool& r_value);
    void GetDecelerating(MCExecContext& ctxt, bool& r_value);

	// TextView-specific actions
	void ExecScrollRangeToVisible(MCExecContext& ctxt, integer_t p_integer1, integer_t p_integer2);
    
	void HandleEvent(MCNameRef p_message);
	void HandleEndDragEvent(bool p_decelerate);
	void HandleScrollEvent(void);
	
	MCRectangle32 m_content_rect;
	bool m_post_scroll_event;

protected:
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);

	virtual void UpdateContentRect();
};

////////////////////////////////////////////////////////////////////////////////

// property keyword

// properties of UITextInputTrait interface

static MCNativeControlEnumEntry s_autocapitalizationtype_enum[] =
{
	{"none", UITextAutocapitalizationTypeNone},
	{"words", UITextAutocapitalizationTypeWords},
	{"sentences", UITextAutocapitalizationTypeSentences},
	{"all characters", UITextAutocapitalizationTypeAllCharacters},
	{nil, 0},
};

static MCNativeControlEnumEntry s_autocorrectiontype_enum[] =
{
	{ "default", UITextAutocorrectionTypeDefault },
	{ "no", UITextAutocorrectionTypeNo },
	{ "yes", UITextAutocorrectionTypeYes },
	{nil, 0},
};

static MCNativeControlEnumEntry s_keyboardtype_enum[] =
{
	{"default", UIKeyboardTypeDefault},
	{"alphabet", UIKeyboardTypeAlphabet},
	{"numeric", UIKeyboardTypeNumbersAndPunctuation},
	{"url", UIKeyboardTypeURL},
	{"number", UIKeyboardTypeNumberPad},
	{"phone", UIKeyboardTypePhonePad},
	{"contact", UIKeyboardTypeNamePhonePad},
	{"email", UIKeyboardTypeEmailAddress},
#ifdef __IPHONE_4_1
	{"decimal", UIKeyboardTypeDecimalPad},
#else
	{"decimal", UIKeyboardTypeNumbersAndPunctuation},
#endif
	{nil, 0},
};

static MCNativeControlEnumEntry s_keyboardstyle_enum[] =
{
	{"default", UIKeyboardAppearanceDefault},
	{"alert", UIKeyboardAppearanceAlert},
	{nil, 0}
};

static MCNativeControlEnumEntry s_returnkeytype_enum[] =
{
	{"default", UIReturnKeyDefault},
	{"go", UIReturnKeyGo},
	{"google", UIReturnKeyGoogle},
	{"join", UIReturnKeyJoin},
	{"next", UIReturnKeyNext},
	{"route", UIReturnKeyRoute},
	{"search", UIReturnKeySearch},
	{"send", UIReturnKeySend},
	{"yahoo", UIReturnKeyYahoo},
	{"done", UIReturnKeyDone},
	{"emergency call", UIReturnKeyEmergencyCall},
	{nil, 0}
};

enum
{
	kMCTextContentTypePlain,
	kMCTextContentTypePassword
};

static MCNativeControlEnumEntry s_contenttype_enum[] =
{
	{"plain", kMCTextContentTypePlain},
	{"password", kMCTextContentTypePassword},
	{nil, 0}
};

// common text control properties

static MCNativeControlEnumEntry s_textalign_enum[] =
{ 
	{"left", NSTextAlignmentLeft},
	{"right", NSTextAlignmentRight},
	{"center", NSTextAlignmentCenter},
	{nil, 0}
};

// single-line input properties

static MCNativeControlEnumEntry s_clearbuttonmode_enum[] =
{
	{"never", UITextFieldViewModeNever},
	{"while editing", UITextFieldViewModeWhileEditing},
	{"unless editing", UITextFieldViewModeUnlessEditing},
	{"always", UITextFieldViewModeAlways},
	{nil, 0}
};

static MCNativeControlEnumEntry s_borderstyle_enum[] =
{
	{"none", UITextBorderStyleNone},
	{"line", UITextBorderStyleLine},
	{"bezel", UITextBorderStyleBezel},
	{"rounded", UITextBorderStyleRoundedRect},
	{nil, 0}
};

// multi-line input properties

enum
{
	kMCTextVerticalAlignTop,
	kMCTextVerticalAlignCenter,
	kMCTextVerticalAlignBottom,
};

static MCNativeControlEnumEntry s_verticaltextalign_enum[] =
{
	{"top", kMCTextVerticalAlignTop},
	{"center", kMCTextVerticalAlignCenter},
	{"bottom", kMCTextVerticalAlignBottom},
	{nil, 0}
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCiOSInputControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_ENABLED, Bool, MCiOSInputControl, Enabled)
    DEFINE_RW_CTRL_PROPERTY(P_TEXT, String, MCiOSInputControl, Text)
    DEFINE_RW_CTRL_PROPERTY(P_UNICODE_TEXT, String, MCiOSInputControl, Text)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_TEXT_COLOR, NativeControlColor, MCiOSInputControl, TextColor)
    DEFINE_RW_CTRL_PROPERTY(P_FONT_SIZE, UInt32, MCiOSInputControl, FontSize)
    DEFINE_RW_CTRL_PROPERTY(P_FONT_NAME, String, MCiOSInputControl, FontName)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_TEXT_ALIGN, NativeControlInputTextAlign, MCiOSInputControl, TextAlign)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_AUTO_CAPITALIZATION_TYPE, NativeControlInputCapitalizationType, MCiOSInputControl, AutoCapitalizationType)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_AUTOCORRECTION_TYPE, NativeControlInputAutocorrectionType, MCiOSInputControl, AutoCorrectionType)
    DEFINE_RW_CTRL_PROPERTY(P_MANAGE_RETURN_KEY, Bool, MCiOSInputControl, ManageReturnKey)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_KEYBOARD_TYPE, NativeControlInputKeyboardType, MCiOSInputControl, KeyboardType)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_KEYBOARD_STYLE, NativeControlInputKeyboardStyle, MCiOSInputControl, KeyboardStyle)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_RETURN_KEY_TYPE, NativeControlInputReturnKeyType, MCiOSInputControl, ReturnKey)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_CONTENT_TYPE, NativeControlInputContentType, MCiOSInputControl, ContentType)
};

MCObjectPropertyTable MCiOSInputControl::kPropertyTable =
{
	&MCiOSControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCiOSInputControl::kActions[] =
{
    DEFINE_CTRL_EXEC_METHOD(Focus, Void, MCiOSInputControl, Focus)
};

MCNativeControlActionTable MCiOSInputControl::kActionTable =
{
    &MCiOSControl::kActionTable,
    sizeof(kActions) / sizeof(kActions[0]),
    &kActions[0],
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCiOSInputFieldControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_AUTO_FIT, Bool, MCiOSInputFieldControl, AutoFit)
    DEFINE_RW_CTRL_PROPERTY(P_MINIMUM_FONT_SIZE, Int32, MCiOSInputFieldControl, MinimumFontSize)
    DEFINE_RW_CTRL_PROPERTY(P_MAXIMUM_TEXT_LENGTH, UInt32, MCiOSInputFieldControl, MaximumTextLength)
    DEFINE_RW_CTRL_PROPERTY(P_AUTO_CLEAR, Bool, MCiOSInputFieldControl, AutoClear)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_CLEAR_BUTTON_MODE, NativeControlClearButtonMode, MCiOSInputFieldControl, ClearButtonMode)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_BORDER_STYLE, NativeControlBorderStyle, MCiOSInputFieldControl, BorderStyle)
};

MCObjectPropertyTable MCiOSInputFieldControl::kPropertyTable =
{
	&MCiOSInputControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCiOSInputFieldControl::kActions[] =
{
};

MCNativeControlActionTable MCiOSInputFieldControl::kActionTable =
{
    &MCiOSInputControl::kActionTable,
    0,
    nil,
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCiOSMultiLineControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_EDITABLE, Bool, MCiOSMultiLineControl, Editable)
    DEFINE_RW_CTRL_SET_PROPERTY(P_DATA_DETECTOR_TYPES, NativeControlInputDataDetectorType, MCiOSMultiLineControl, DataDetectorTypes)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_SELECTED_RANGE, NativeControlRange, MCiOSMultiLineControl, SelectedRange)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_VERTICAL_TEXT_ALIGN, NativeControlInputVerticalAlign, MCiOSMultiLineControl, VerticalTextAlign)
    DEFINE_RW_CTRL_PROPERTY(P_CONTENT_RECT, Int32X4, MCiOSMultiLineControl, ContentRect)
    DEFINE_RW_CTRL_PROPERTY(P_HSCROLL, Int32, MCiOSMultiLineControl, HScroll)
    DEFINE_RW_CTRL_PROPERTY(P_VSCROLL, Int32, MCiOSMultiLineControl, VScroll)
    DEFINE_RW_CTRL_PROPERTY(P_CAN_BOUNCE, Bool, MCiOSMultiLineControl, CanBounce)
    DEFINE_RW_CTRL_PROPERTY(P_CAN_SCROLL_TO_TOP, Bool, MCiOSMultiLineControl, CanScrollToTop)
    DEFINE_RW_CTRL_PROPERTY(P_CAN_CANCEL_TOUCHES, Bool, MCiOSMultiLineControl, CanCancelTouches)
    DEFINE_RW_CTRL_PROPERTY(P_DELAY_TOUCHES, Bool, MCiOSMultiLineControl, DelayTouches)
    DEFINE_RW_CTRL_PROPERTY(P_SCROLLING_ENABLED, Bool, MCiOSMultiLineControl, ScrollingEnabled)
    DEFINE_RW_CTRL_PROPERTY(P_PAGING_ENABLED, Bool, MCiOSMultiLineControl, PagingEnabled)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_DECELERATION_RATE, NativeControlDecelerationRate, MCiOSMultiLineControl, DecelerationRate)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_INDICATOR_STYLE, NativeControlIndicatorStyle, MCiOSMultiLineControl, IndicatorStyle)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_INDICATOR_INSETS, NativeControlIndicatorInsets, MCiOSMultiLineControl, IndicatorInsets)
    DEFINE_RW_CTRL_PROPERTY(P_SHOW_HORIZONTAL_INDICATOR, Bool, MCiOSMultiLineControl, ShowHorizontalIndicator)
    DEFINE_RW_CTRL_PROPERTY(P_SHOW_VERTICAL_INDICATOR, Bool, MCiOSMultiLineControl, ShowVerticalIndicator)
    DEFINE_RW_CTRL_PROPERTY(P_LOCK_DIRECTION, Bool, MCiOSMultiLineControl, LockDirection)
    DEFINE_RO_CTRL_PROPERTY(P_TRACKING, Bool, MCiOSMultiLineControl, Tracking)
    DEFINE_RO_CTRL_PROPERTY(P_DRAGGING, Bool, MCiOSMultiLineControl, Dragging)
    DEFINE_RO_CTRL_PROPERTY(P_DECELERATING, Bool, MCiOSMultiLineControl, Decelerating)
};

MCObjectPropertyTable MCiOSMultiLineControl::kPropertyTable =
{
	&MCiOSInputControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCiOSMultiLineControl::kActions[] =
{
    DEFINE_CTRL_EXEC_BINARY_METHOD(ScrollRangeToVisible, Integer_Integer, MCiOSMultiLineControl, Int32, Int32, ScrollRangeToVisible)
};

MCNativeControlActionTable MCiOSMultiLineControl::kActionTable =
{
    &MCiOSInputControl::kActionTable,
    sizeof(kActions) / sizeof(kActions[0]),
    &kActions[0],
};

////////////////////////////////////////////////////////////////////////////////

MCiOSInputControl::MCiOSInputControl(void)
{
	m_delegate = nil;
}

MCiOSInputControl::~MCiOSInputControl(void)
{
}

void MCiOSInputControl::SetEnabled(MCExecContext& ctxt, bool p_value)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
        [t_field setEnabled: p_value];
}

void MCiOSInputControl::SetOpaque(MCExecContext& ctxt, bool p_value)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        MCiOSControl::SetOpaque(ctxt, &p_value);
        [t_field setNeedsDisplay];
    }
}

void MCiOSInputControl::SetText(MCExecContext& ctxt, MCStringRef p_string)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        NSUInteger t_max_length;
        t_max_length =  m_delegate.getMaxTextLength;
        
        NSString *t_string;
        t_string = MCStringConvertToAutoreleasedNSString(p_string);
        
        if (t_string.length > t_max_length)
            [t_field setText: [t_string substringToIndex:t_max_length]];
        else
            [t_field setText: t_string];
    }
}

void MCiOSInputControl::SetTextColor(MCExecContext& ctxt, const MCNativeControlColor& p_color)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        UIColor *t_color;
        ParseColor(p_color, t_color);
        [t_field setTextColor: t_color];
    }
}

void MCiOSInputControl::SetFontName(MCExecContext& ctxt, MCStringRef p_font)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        NSString *t_string;
        t_string = MCStringConvertToAutoreleasedNSString(p_font);
        [t_field setFont: [UIFont fontWithName: t_string size: [[t_field font] pointSize]]];
    }
}

void MCiOSInputControl::SetFontSize(MCExecContext& ctxt, uinteger_t p_size)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        // FG-2013-11-06 [[ Bugfix 11285 ]]
        // On iOS7 devices, UITextView controls were having their font size
        // properties ignored because [t_field font] was returning nil when
        // no text had been added to the control yet.
        UIFont* t_font = [t_field font];
        if (t_font == nil)
            t_font = [UIFont systemFontOfSize: p_size];
        else
            t_font = [t_font fontWithSize: p_size];
        
        [t_field setFont: t_font];
    }
}

void MCiOSInputControl::SetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign p_align)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        NSTextAlignment t_align;
        
        switch (p_align)
        {
            case kMCNativeControlInputTextAlignCenter:
                t_align = NSTextAlignmentCenter;
                break;
            case kMCNativeControlInputTextAlignLeft:
                t_align = NSTextAlignmentLeft;
                break;
            case kMCNativeControlInputTextAlignRight:
                t_align = NSTextAlignmentRight;
                break; 
        }
        [t_field setTextAlignment: t_align];
    }
}

void MCiOSInputControl::SetAutoCapitalizationType(MCExecContext& ctxt, MCNativeControlInputCapitalizationType p_type)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        UITextAutocapitalizationType t_type;
        switch (p_type)
        {
            case kMCNativeControlInputCapitalizeNone:
                t_type = UITextAutocapitalizationTypeNone;
                break;
            case kMCNativeControlInputCapitalizeWords:
                t_type = UITextAutocapitalizationTypeWords;
                break;
            case kMCNativeControlInputCapitalizeSentences:
                t_type = UITextAutocapitalizationTypeSentences;
                break;
            case kMCNativeControlInputCapitalizeCharacters:
                t_type = UITextAutocapitalizationTypeAllCharacters;
                break;
        }
        [t_field setAutocapitalizationType: t_type];
    }
}

void MCiOSInputControl::SetAutoCorrectionType(MCExecContext& ctxt, MCNativeControlInputAutocorrectionType p_type)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        UITextAutocorrectionType t_type;
        switch (p_type)
        {
            case kMCNativeControlInputAutocorrectionDefault:
                t_type = UITextAutocorrectionTypeDefault;
                break;
            case kMCNativeControlInputAutocorrectionNo:
                t_type = UITextAutocorrectionTypeNo;
                break;
            case kMCNativeControlInputAutocorrectionYes:
                t_type = UITextAutocorrectionTypeYes;
                break;
        }
        [t_field setAutocorrectionType: t_type];
    }
}

void MCiOSInputControl::SetManageReturnKey(MCExecContext& ctxt, bool p_value)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
        [t_field setEnablesReturnKeyAutomatically: p_value];
}

void MCiOSInputControl::SetKeyboardType(MCExecContext& ctxt, MCNativeControlInputKeyboardType p_type)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        UIKeyboardType t_type;
        switch (p_type)
        {
            case kMCNativeControlInputKeyboardTypeDefault:
                t_type = UIKeyboardTypeDefault;
                break;
            case kMCNativeControlInputKeyboardTypeAlphabet:
                t_type = UIKeyboardTypeAlphabet;
                break;
            case kMCNativeControlInputKeyboardTypeDecimal:
#ifdef __IPHONE_4_1
                t_type = UIKeyboardTypeDecimalPad;
#else
                t_type = UIKeyboardTypeNumbersAndPunctuation;
#endif
                break;
            case kMCNativeControlInputKeyboardTypeNumeric:
                t_type = UIKeyboardTypeNumbersAndPunctuation;
                break;
            case kMCNativeControlInputKeyboardTypeURL:
                t_type = UIKeyboardTypeURL;
                break;
            case kMCNativeControlInputKeyboardTypeNumber:
                t_type = UIKeyboardTypeNumberPad;
                break;
            case kMCNativeControlInputKeyboardTypePhone:
                t_type = UIKeyboardTypePhonePad;
                break;
            case kMCNativeControlInputKeyboardTypeContact:
                t_type = UIKeyboardTypeNamePhonePad;
                break;
            case kMCNativeControlInputKeyboardTypeEmail:
                t_type = UIKeyboardTypeEmailAddress;
                break;
        }
        [t_field setKeyboardType: t_type];
    }
}

void MCiOSInputControl::SetKeyboardStyle(MCExecContext& ctxt, MCNativeControlInputKeyboardStyle p_style)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        UIKeyboardAppearance t_style;
        t_style = p_style == kMCNativeControlInputKeyboardStyleDefault ? UIKeyboardAppearanceDefault : UIKeyboardAppearanceAlert;
        [t_field setKeyboardAppearance: t_style];
    }
}

void MCiOSInputControl::SetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType p_key)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        UIReturnKeyType t_type;
        switch (p_key)
        {
            case kMCNativeControlInputReturnKeyTypeDefault:
                t_type = UIReturnKeyDefault;
                break;
            case kMCNativeControlInputReturnKeyTypeGo:
                t_type = UIReturnKeyGo;
                break;
            case kMCNativeControlInputReturnKeyTypeNext:
                t_type = UIReturnKeyNext;
                break;
            case kMCNativeControlInputReturnKeyTypeSearch:
                t_type = UIReturnKeySearch;
                break;
            case kMCNativeControlInputReturnKeyTypeSend:
                t_type = UIReturnKeySend;
                break;
            case kMCNativeControlInputReturnKeyTypeDone:
                t_type = UIReturnKeyDone;
                break;
            case kMCNativeControlInputReturnKeyTypeRoute:
                t_type = UIReturnKeyRoute;
                break;
            case kMCNativeControlInputReturnKeyTypeYahoo:
                t_type = UIReturnKeyYahoo;
                break;
            case kMCNativeControlInputReturnKeyTypeGoogle:
                t_type = UIReturnKeyGoogle;
                break;
            case kMCNativeControlInputReturnKeyTypeJoin:
                t_type = UIReturnKeyJoin;
                break;
            case kMCNativeControlInputReturnKeyTypeEmergencyCall:
                t_type = UIReturnKeyEmergencyCall;
                break;
        }
        
        [t_field setReturnKeyType: t_type];
    }
}
    
void MCiOSInputControl::SetContentType(MCExecContext& ctxt, MCNativeControlInputContentType p_type)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        if (p_type == kMCNativeControlInputContentTypePassword)
            [t_field setSecureTextEntry: YES];
        else
            [t_field setSecureTextEntry: NO];
    }
}

void MCiOSInputControl::GetEnabled(MCExecContext& ctxt, bool& r_value)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
        r_value = [t_field isEnabled];
    else
        r_value = false;
}


void MCiOSInputControl::GetText(MCExecContext& ctxt, MCStringRef& r_string)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        if (MCStringCreateWithCFStringRef((CFStringRef)[t_field text], r_string))
            return;
    }
    else
        r_string = MCValueRetain(kMCEmptyString);
    
    ctxt . Throw();
}

void MCiOSInputControl::GetTextColor(MCExecContext& ctxt, MCNativeControlColor& r_color)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
        FormatColor([t_field textColor], r_color);
}

void MCiOSInputControl::GetFontName(MCExecContext& ctxt, MCStringRef& r_font)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        if (MCStringCreateWithCFStringRef((CFStringRef)[[t_field font] fontName], r_font))
            return;
    }
    else
        r_font = MCValueRetain(kMCEmptyString);
    
    ctxt . Throw();
}

void MCiOSInputControl::GetFontSize(MCExecContext& ctxt, uinteger_t& r_size)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
        r_size = [[t_field font] pointSize];
    else
        r_size = 0;
}

void MCiOSInputControl::GetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign& r_align)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    r_align = kMCNativeControlInputTextAlignLeft;
    
    if (t_field)
    {
        switch ([t_field textAlignment])
        {
            case NSTextAlignmentCenter:
                r_align = kMCNativeControlInputTextAlignCenter;
                return;
            case NSTextAlignmentLeft:
                r_align = kMCNativeControlInputTextAlignLeft;
                return;
            case NSTextAlignmentRight:
                r_align = kMCNativeControlInputTextAlignRight;
                return;
        }
    }
}

void MCiOSInputControl::GetAutoCapitalizationType(MCExecContext& ctxt, MCNativeControlInputCapitalizationType& r_type)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    r_type = kMCNativeControlInputCapitalizeNone;
    
    if (t_field)
    {
        switch ([t_field autocapitalizationType])
        {
            case UITextAutocapitalizationTypeAllCharacters:
                r_type = kMCNativeControlInputCapitalizeCharacters;
                return;
            case UITextAutocapitalizationTypeSentences:
                r_type = kMCNativeControlInputCapitalizeSentences;
                return;
            case UITextAutocapitalizationTypeWords:
                r_type = kMCNativeControlInputCapitalizeWords;
                return;
            default:
                return;
        }
    }
}

void MCiOSInputControl::GetAutoCorrectionType(MCExecContext& ctxt, MCNativeControlInputAutocorrectionType& r_type)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    r_type = kMCNativeControlInputAutocorrectionDefault;
    
    if (t_field)
    {
        switch ([t_field autocorrectionType])
        {
            case UITextAutocorrectionTypeNo:
                r_type = kMCNativeControlInputAutocorrectionNo;
                return;
            case UITextAutocorrectionTypeYes:
                r_type = kMCNativeControlInputAutocorrectionYes;
                return;
            default:
                return;
        }
    }
}

void MCiOSInputControl::GetManageReturnKey(MCExecContext& ctxt, bool& r_value)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
        r_value = [t_field enablesReturnKeyAutomatically];
    else
        r_value = false;
}

void MCiOSInputControl::GetKeyboardType(MCExecContext& ctxt, MCNativeControlInputKeyboardType& r_type)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    r_type = kMCNativeControlInputKeyboardTypeDefault;
    
    if (t_field)
    {
        switch ([t_field keyboardType])
        {
            case UIKeyboardTypeAlphabet:
                r_type = kMCNativeControlInputKeyboardTypeAlphabet;
                return;
            case UIKeyboardTypeDecimalPad:
                r_type = kMCNativeControlInputKeyboardTypeDecimal;
                return;
            case UIKeyboardTypeEmailAddress:
                r_type = kMCNativeControlInputKeyboardTypeEmail;
                return;
            case UIKeyboardTypeNumbersAndPunctuation:
#ifdef __IPHONE_4_1
                r_type = kMCNativeControlInputKeyboardTypeNumeric;
#else
                r_type = kMCNativeControlInputKeyboardTypeDecimal;
#endif
                return;
            case UIKeyboardTypeNumberPad:
                r_type = kMCNativeControlInputKeyboardTypeNumber;
                return;
            case UIKeyboardTypePhonePad:
                r_type = kMCNativeControlInputKeyboardTypePhone;
                return;
            case UIKeyboardTypeNamePhonePad:
                r_type = kMCNativeControlInputKeyboardTypeContact;
                return;
            default:
                return;
        }
    }
}

void MCiOSInputControl::GetKeyboardStyle(MCExecContext& ctxt, MCNativeControlInputKeyboardStyle& r_style)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    r_style = kMCNativeControlInputKeyboardStyleDefault;
    
    if (t_field && [t_field keyboardAppearance] == UIKeyboardAppearanceAlert)
        r_style = kMCNativeControlInputKeyboardStyleAlert;
}

void MCiOSInputControl::GetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType& r_key)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    r_key = kMCNativeControlInputReturnKeyTypeDefault;
    
    if (t_field)
    {
        switch ([t_field returnKeyType])
        {
            case UIReturnKeyGo:
                r_key = kMCNativeControlInputReturnKeyTypeGo;
                return;
            case UIReturnKeyGoogle:
                r_key = kMCNativeControlInputReturnKeyTypeGoogle;
                return;
            case UIReturnKeyJoin:
                r_key = kMCNativeControlInputReturnKeyTypeJoin;
                return;
            case UIReturnKeyNext:
                r_key = kMCNativeControlInputReturnKeyTypeNext;
                return;
            case UIReturnKeyRoute:
                r_key = kMCNativeControlInputReturnKeyTypeRoute;
                return;
            case UIReturnKeySearch:
                r_key = kMCNativeControlInputReturnKeyTypeSearch;
                return;
            case UIReturnKeySend:
                r_key = kMCNativeControlInputReturnKeyTypeSend;
                return;
            case UIReturnKeyYahoo:
                r_key = kMCNativeControlInputReturnKeyTypeYahoo;
                return;
            case UIReturnKeyDone:
                r_key = kMCNativeControlInputReturnKeyTypeDone;
                return;
            case UIReturnKeyEmergencyCall:
                r_key = kMCNativeControlInputReturnKeyTypeEmergencyCall;
                return;
            default:
                return;
        }
    }
}

void MCiOSInputControl::GetContentType(MCExecContext& ctxt, MCNativeControlInputContentType& r_type)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    r_type = kMCNativeControlInputContentTypePlain;
    
    if (t_field && [t_field isSecureTextEntry])
        r_type = kMCNativeControlInputContentTypePassword;
}

void MCiOSInputControl::ExecFocus(MCExecContext &ctxt)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
	if (t_field == nil)
        return;
    
    [t_field becomeFirstResponder];
}

////////////////////////////////////////////////////////////////////////////////

com_runrev_livecode_MCiOSInputDelegate *MCiOSInputControl::GetDelegate(void)
{
	return m_delegate;
}

void MCiOSInputControl::HandleNotifyEvent(MCNameRef p_notification)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message(p_notification);
		ChangeTarget(t_old_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

MCNativeControlType MCiOSInputFieldControl::GetType(void)
{
	return kMCNativeControlTypeInput;
}

void MCiOSInputFieldControl::SetAutoFit(MCExecContext& ctxt, bool p_value)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
    
	if (t_field)
        [t_field setAdjustsFontSizeToFitWidth: p_value];
}

void MCiOSInputFieldControl::SetMinimumFontSize(MCExecContext& ctxt, integer_t p_size)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
    
	if (t_field)
        [t_field setMinimumFontSize: p_size];
}

void MCiOSInputFieldControl::SetMaximumTextLength(MCExecContext& ctxt, uinteger_t p_length)
{
    if (m_delegate != nil)
    {
       	com_runrev_livecode_MCiOSInputDelegate *t_delegate;
        t_delegate = (com_runrev_livecode_MCiOSInputDelegate*)m_delegate;
        
        [t_delegate setMaxTextLength: p_length];
        
        UITextField *t_field;
        t_field = (UITextField *)GetView();
        
        // Truncate the text is it exceeds the maximum length
        if (t_field && t_field.text.length > p_length)
        {
            t_field.text = [t_field.text substringToIndex:p_length];
        }
    }
}


void MCiOSInputFieldControl::SetAutoClear(MCExecContext& ctxt, bool p_value)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
    
	if (t_field)
        [t_field setClearsOnBeginEditing: p_value];
}

void MCiOSInputFieldControl::SetClearButtonMode(MCExecContext& ctxt, MCNativeControlClearButtonMode p_mode)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
	if (t_field)
    {
        UITextFieldViewMode t_mode;
        
        switch (p_mode)
        {
            case kMCNativeControlClearButtonModeAlways:
                t_mode = UITextFieldViewModeAlways;
                break;
            case kMCNativeControlClearButtonModeNever:
                t_mode = UITextFieldViewModeNever;
                break;
            case kMCNativeControlClearButtonModeWhileEditing:
                t_mode = UITextFieldViewModeWhileEditing;
                break;
            case kMCNativeControlClearButtonModeUnlessEditing:
                t_mode = UITextFieldViewModeUnlessEditing;
                break;
        }
        [t_field setClearButtonMode: t_mode];
    }
}

void MCiOSInputFieldControl::SetBorderStyle(MCExecContext& ctxt, MCNativeControlBorderStyle p_style)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
	if (t_field)
    {
        UITextBorderStyle t_style;
        
        switch (p_style)
        {
            case kMCNativeControlBorderStyleNone:
                t_style = UITextBorderStyleNone;
                break;
            case kMCNativeControlBorderStyleLine:
                t_style = UITextBorderStyleLine;
                break;
            case kMCNativeControlBorderStyleBezel:
                t_style = UITextBorderStyleBezel;
                break;
            case kMCNativeControlBorderStyleRoundedRect:
                t_style = UITextBorderStyleRoundedRect;
                break;
        }
        [t_field setBorderStyle: t_style];
    }
}

void MCiOSInputFieldControl::GetAutoFit(MCExecContext& ctxt, bool& r_value)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
    
	if (t_field)
        r_value = [t_field adjustsFontSizeToFitWidth];
    else
        r_value = false;
}

void MCiOSInputFieldControl::GetMinimumFontSize(MCExecContext& ctxt, integer_t& r_size)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
    
	if (t_field)
        r_size = [t_field minimumFontSize];
    else
        r_size = 0;
}

void MCiOSInputFieldControl::GetMaximumTextLength(MCExecContext& ctxt, uinteger_t& r_length)
{
    if (m_delegate != nil)
    {
       	com_runrev_livecode_MCiOSInputDelegate *t_delegate;
        t_delegate = (com_runrev_livecode_MCiOSInputDelegate*)m_delegate;
        
        r_length = [t_delegate getMaxTextLength];
    }
    else
        r_length = 0;
}

void MCiOSInputFieldControl::GetAutoClear(MCExecContext& ctxt, bool& r_value)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
    
	if (t_field)
        r_value = [t_field clearsOnBeginEditing];
    else
        r_value = false;
}

void MCiOSInputFieldControl::GetClearButtonMode(MCExecContext& ctxt, MCNativeControlClearButtonMode& r_mode)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    r_mode = kMCNativeControlClearButtonModeNever;
    
	if (t_field)
    {
        switch ([t_field clearButtonMode])
        {
            case UITextFieldViewModeAlways:
                r_mode = kMCNativeControlClearButtonModeAlways;
                return;
            case UITextFieldViewModeWhileEditing:
                r_mode = kMCNativeControlClearButtonModeWhileEditing;
                return;
            case UITextFieldViewModeUnlessEditing:
                r_mode = kMCNativeControlClearButtonModeUnlessEditing;
                return;
            default:
                return;
        }
    }
}

void MCiOSInputFieldControl::GetBorderStyle(MCExecContext& ctxt, MCNativeControlBorderStyle& r_style)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    r_style = kMCNativeControlBorderStyleNone;
    
	if (t_field)
    {
        switch ([t_field borderStyle])
        {
            case UITextBorderStyleLine:
                r_style = kMCNativeControlBorderStyleLine;
                return;
            case UITextBorderStyleBezel:
                r_style = kMCNativeControlBorderStyleBezel;
                return;
            case UITextBorderStyleRoundedRect:
                r_style = kMCNativeControlBorderStyleRoundedRect;
                return;
        }
    }
}

UIView *MCiOSInputFieldControl::CreateView(void)
{
	UITextField *t_view;
	t_view = [[UITextField alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return nil;
	
	[t_view setHidden: YES];
	
	m_delegate = [[com_runrev_livecode_MCiOSInputDelegate alloc] initWithInstance: this view: t_view];
	[t_view setDelegate: m_delegate];
	
	return t_view;
}

void MCiOSInputFieldControl::DeleteView(UIView *p_view)
{
	[p_view setDelegate: nil];
	
	// MW-2013-05-22: [[ Bug 10880 ]] Make sure we release the delegate here as
	//   it might need to refer to the view to deregister itself.
	[m_delegate release];
	m_delegate = nil;
	
	[p_view release];
}

////////////////////////////////////////////////////////////////////////////////

MCNativeControlType MCiOSMultiLineControl::GetType(void)
{
	return kMCNativeControlTypeInput;
}

bool datadetectortypes_from_string(const char *p_list, UIDataDetectorTypes &r_types);
bool datadetectortypes_to_string(UIDataDetectorTypes p_types, char *&r_list);

void MCiOSMultiLineControl::SetEditable(MCExecContext& ctxt, bool p_value)
{
    UITextView *t_view;
	t_view = (UITextView *)GetView();
    
	if (t_view)
        [t_view setEditable: p_value];
}

void MCiOSMultiLineControl::SetSelectedRange(MCExecContext& ctxt, const MCNativeControlRange& p_range)
{
    UITextView *t_view;
	t_view = (UITextView *)GetView();
    
	if (t_view)
    {
        NSRange t_range;
        t_range = NSMakeRange(p_range . start - 1, p_range . length);
        
        [t_view setSelectedRange: t_range];  
    }
}

void MCiOSMultiLineControl::SetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType p_type)
{
    UITextView *t_view;
	t_view = (UITextView *)GetView();
    
	if (t_view)
    {
        UIDataDetectorTypes t_types;
        t_types = UIDataDetectorTypeNone;
        
        if (p_type & kMCNativeControlInputDataDetectorTypeAll)
            t_types |= UIDataDetectorTypeAll;
        if (p_type & kMCNativeControlInputDataDetectorTypeMapAddress)
            t_types |= UIDataDetectorTypeAddress;
        if (p_type & kMCNativeControlInputDataDetectorTypePhoneNumber)
            t_types |= UIDataDetectorTypePhoneNumber;
        if (p_type & kMCNativeControlInputDataDetectorTypeWebUrl)
            t_types |= UIDataDetectorTypeLink;
        if (p_type & kMCNativeControlInputDataDetectorTypeCalendarEvent)
            t_types |= UIDataDetectorTypeCalendarEvent;
    
        [t_view setDataDetectorTypes: t_types];
    }
}

void MCiOSMultiLineControl::SetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign p_align)
{
	if (m_delegate != nil)
    {
       	com_runrev_livecode_MCiOSMultiLineDelegate *t_delegate;
        t_delegate = (com_runrev_livecode_MCiOSMultiLineDelegate*)m_delegate;
        
        [t_delegate setVerticalTextAlign:(int32_t)p_align];
    }
}

void MCiOSMultiLineControl::SetContentRect(MCExecContext& ctxt, integer_t p_rect[4])
{
    // ES_NOT_HANDLED
}

void MCiOSMultiLineControl::GetEditable(MCExecContext& ctxt, bool& r_value)
{
    UITextView *t_view;
	t_view = (UITextView *)GetView();
    
	if (t_view)
        r_value = [t_view isEditable];
}

void MCiOSMultiLineControl::GetSelectedRange(MCExecContext& ctxt, MCNativeControlRange& r_range)
{
    UITextView *t_view;
	t_view = (UITextView *)GetView();
    
	if (t_view)
    {
        NSRange t_range;
        t_range = [t_view selectedRange];
        
        r_range . start = t_range . location + 1;
        r_range . length = t_range . length;
    }
}

void MCiOSMultiLineControl::GetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType& r_type)
{
    UITextView *t_view;
	t_view = (UITextView *)GetView();
    
    uint32_t t_native_types;
    uint32_t t_types;
    
    t_types = 0;
    
	if (t_view)
    {
        t_native_types = [t_view dataDetectorTypes];
        
        if (t_native_types & UIDataDetectorTypeAll)
        {
            t_types |= kMCNativeControlInputDataDetectorTypeCalendarEvent;
            t_types |= kMCNativeControlInputDataDetectorTypeMapAddress;
            t_types |= kMCNativeControlInputDataDetectorTypePhoneNumber;
            t_types |= kMCNativeControlInputDataDetectorTypeWebUrl;
        }
        
        if (t_native_types & UIDataDetectorTypeCalendarEvent)
            t_types |= kMCNativeControlInputDataDetectorTypeCalendarEvent;
        if (t_native_types & UIDataDetectorTypeAddress)
            t_types |= kMCNativeControlInputDataDetectorTypeMapAddress;
        if (t_native_types & UIDataDetectorTypePhoneNumber)
            t_types |= kMCNativeControlInputDataDetectorTypePhoneNumber;
        if (t_native_types & UIDataDetectorTypeLink)
            t_types |= kMCNativeControlInputDataDetectorTypeWebUrl;
    }
    
    r_type = (MCNativeControlInputDataDetectorType)t_types;
}

void MCiOSMultiLineControl::GetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign& r_align)
{
	com_runrev_livecode_MCiOSMultiLineDelegate *t_delegate;
	t_delegate = (com_runrev_livecode_MCiOSMultiLineDelegate*)m_delegate;
 
    if (t_delegate)
        r_align = (MCNativeControlInputVerticalAlign)[t_delegate getVerticalTextAlign];
        
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSMultiLineControl::GetContentRect(MCExecContext& ctxt, integer_t r_rect[4])
{
    UpdateContentRect();
    if (GetView())
    {
        r_rect[0] = m_content_rect . x;
        r_rect[1] = m_content_rect . y;
        r_rect[2] = m_content_rect . x + m_content_rect . width;
        r_rect[3] = m_content_rect . y + m_content_rect . height;
    }
}

void MCiOSMultiLineControl::SetHScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    UpdateContentRect();

    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    int32_t t_x, t_y;
    if (t_view != nil && MCScrollViewGetContentOffset(t_view, t_x, t_y))
    {
        // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
        MCGPoint t_offset;
        t_offset = MCNativeControlUserPointToDevicePoint(MCGPointMake((MCGFloat)p_scroll - m_content_rect . x, (MCGFloat) t_y));
        [t_view setContentOffset: CGPointMake(t_offset . x, t_offset . y)];
    }
}

void MCiOSMultiLineControl::GetHScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    UpdateContentRect();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    // MM-2013-11-26: [[ Bug 11485 ]] The user expects the properties of the scroller in user space, so must converted to device space before returning.
    if (t_view)
        r_scroll = m_content_rect.x + MCNativeControlUserXLocFromDeviceXLoc([t_view contentOffset].x);
        else
            r_scroll = 0;
}
void MCiOSMultiLineControl::SetVScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    UpdateContentRect();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    int32_t t_x, t_y;
    if (t_view != nil && MCScrollViewGetContentOffset(t_view, t_x, t_y))
    {
        // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
        MCGPoint t_offset;
        t_offset = MCNativeControlUserPointToDevicePoint(MCGPointMake((MCGFloat) t_x, (MCGFloat) p_scroll - m_content_rect . y));
        [t_view setContentOffset: CGPointMake(t_offset . x, t_offset . y)];
    }
}

void MCiOSMultiLineControl::GetVScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    UpdateContentRect();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    // MM-2013-11-26: [[ Bug 11485 ]] The user expects the properties of the scroller in user space, so must converted to device space before returning.
    if (t_view)
        r_scroll = m_content_rect.y + MCNativeControlUserYLocFromDeviceYLoc([t_view contentOffset].y);
        else
            r_scroll = 0;
}
void MCiOSMultiLineControl::SetCanBounce(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setBounces: p_value];
}

void MCiOSMultiLineControl::GetCanBounce(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view bounces];
    else
        r_value = false;
}

void MCiOSMultiLineControl::SetCanScrollToTop(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setScrollsToTop: p_value];
}
void MCiOSMultiLineControl::GetCanScrollToTop(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view scrollsToTop];
    else
        r_value = false;
}
void MCiOSMultiLineControl::SetCanCancelTouches(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
    {
        [t_view setCanCancelContentTouches: p_value];
        // SN-2014-01-14: [[ bugfix-11482 ]] DelayContentTouches must be set
        // to the opposite of CanCancelContentTouches to allow scrolling on iOS7
        [t_view setDelaysContentTouches: !p_value];
    }
}
void MCiOSMultiLineControl::GetCanCancelTouches(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view canCancelContentTouches];
    else
        r_value = false;
}
void MCiOSMultiLineControl::SetDelayTouches(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
    {
        [t_view setDelaysContentTouches: p_value];
        // SN-2014-01-14: [[ bugfix-11482 ]] DelayContentTouches must be set
        // to the opposite of CanCancelContentTouches to allow scrolling on iOS7
        [t_view setCanCancelContentTouches: !p_value];
    }
}

void MCiOSMultiLineControl::GetDelayTouches(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view delaysContentTouches];
    else
        r_value = false;
}
void MCiOSMultiLineControl::SetScrollingEnabled(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setScrollEnabled: p_value];
}
void MCiOSMultiLineControl::GetScrollingEnabled(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isScrollEnabled];
    else
        r_value = false;
}

void MCiOSMultiLineControl::SetPagingEnabled(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setPagingEnabled: p_value];
}

void MCiOSMultiLineControl::GetPagingEnabled(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isPagingEnabled];
    else
        r_value = false;
}

void MCiOSMultiLineControl::SetDecelerationRate(MCExecContext& ctxt, const MCNativeControlDecelerationRate& p_rate)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    float t_deceleration;
    switch (p_rate . type)
    {
        case kMCNativeControlDecelerationRateNormal:
            t_deceleration = UIScrollViewDecelerationRateNormal;
            break;
        case kMCNativeControlDecelerationRateFast:
            t_deceleration = UIScrollViewDecelerationRateFast;
            break;
        case kMCNativeControlDecelerationRateCustom:
            t_deceleration = p_rate . rate;
            break;
    }
    
    if (t_view)
        [t_view setDecelerationRate: t_deceleration];
}

void MCiOSMultiLineControl::GetDecelerationRate(MCExecContext& ctxt, MCNativeControlDecelerationRate& r_rate)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    r_rate . type = kMCNativeControlDecelerationRateCustom;
    
    if (t_view)
        r_rate . rate = [t_view decelerationRate];
    else
        r_rate . rate = 0;
}

void MCiOSMultiLineControl::SetIndicatorStyle(MCExecContext& ctxt, MCNativeControlIndicatorStyle p_style)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    UIScrollViewIndicatorStyle t_style;
    
    if (t_view)
    {
        switch (p_style)
        {
            case kMCNativeControlIndicatorStyleDefault:
            case kMCNativeControlIndicatorStyleEmpty:
                t_style = UIScrollViewIndicatorStyleDefault;
                break;
            case kMCNativeControlIndicatorStyleBlack:
                t_style = UIScrollViewIndicatorStyleBlack;
                break;
            case kMCNativeControlIndicatorStyleWhite:
                t_style = UIScrollViewIndicatorStyleWhite;
                break;
        }
        
        [t_view setIndicatorStyle: t_style];
    }
}

void MCiOSMultiLineControl::GetIndicatorStyle(MCExecContext& ctxt, MCNativeControlIndicatorStyle& r_style)
{
    UIScrollView *t_view;
    t_view = (UIScrollView*)GetView();
    
    if (t_view)
    {
        switch ([t_view indicatorStyle])
        {
            case UIScrollViewIndicatorStyleDefault:
                r_style = kMCNativeControlIndicatorStyleDefault;
                break;
            case UIScrollViewIndicatorStyleBlack:
                r_style = kMCNativeControlIndicatorStyleBlack;
                break;
            case UIScrollViewIndicatorStyleWhite:
                r_style = kMCNativeControlIndicatorStyleWhite;
                break;
        }
    }
    else
        r_style = kMCNativeControlIndicatorStyleEmpty;
}

void MCiOSMultiLineControl::SetIndicatorInsets(MCExecContext& ctxt, const MCNativeControlIndicatorInsets& p_insets)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
    MCGRectangle t_rect;
    t_rect = MCNativeControlUserRectToDeviceRect(MCGRectangleMake(p_insets . left, p_insets . top, p_insets . right - p_insets . left, p_insets . bottom - p_insets . top));
    
    if (t_view)
        [t_view setScrollIndicatorInsets: UIEdgeInsetsMake(t_rect . origin . y, t_rect . origin . x, t_rect . origin . y + t_rect . size . height, t_rect . origin . x + t_rect . size . width)];
}
void MCiOSMultiLineControl::GetIndicatorInsets(MCExecContext& ctxt, MCNativeControlIndicatorInsets& r_insets)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view != nil)
    {
        UIEdgeInsets t_insets;
        t_insets = [t_view scrollIndicatorInsets];
        
        // MM-2013-11-26: [[ Bug 11485 ]] The user expects the properties of the scroller in user space, so must converted to device space before returning.
        MCGRectangle t_rect;
        t_rect = MCNativeControlUserRectFromDeviceRect(MCGRectangleMake(t_insets . left, t_insets . top, t_insets . right - t_insets . left, t_insets . bottom - t_insets . top));
        
        r_insets . left = (int16_t)(t_rect . origin . x);
        r_insets . top = (int16_t)(t_rect . origin . y);
        r_insets . right = (int16_t)(t_rect . origin . x + t_rect . size . width);
        r_insets . bottom = (int16_t)(t_rect . origin . y + t_rect . size . height);
        r_insets . has_insets = true;
    }
    else
        r_insets . has_insets = false;
}

void MCiOSMultiLineControl::SetShowHorizontalIndicator(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setShowsHorizontalScrollIndicator: p_value];
}
void MCiOSMultiLineControl::GetShowHorizontalIndicator(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view showsHorizontalScrollIndicator];
    else
        r_value = false;
}

void MCiOSMultiLineControl::SetShowVerticalIndicator(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setShowsVerticalScrollIndicator: p_value];
}

void MCiOSMultiLineControl::GetShowVerticalIndicator(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view showsVerticalScrollIndicator];
    else
        r_value = false;
}

void MCiOSMultiLineControl::SetLockDirection(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setDirectionalLockEnabled: p_value];
}
void MCiOSMultiLineControl::GetLockDirection(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isDirectionalLockEnabled];
    else
        r_value = false;
}

void MCiOSMultiLineControl::GetTracking(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isTracking];
    else
        r_value = false;
}

void MCiOSMultiLineControl::GetDragging(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isDragging];
    else
        r_value = false;
}

void MCiOSMultiLineControl::GetDecelerating(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isDecelerating];
    else
        r_value = false;
}

////////////////////////////////////////////////////////////////////////////////


void MCiOSMultiLineControl::ExecScrollRangeToVisible(MCExecContext& ctxt, integer_t p_integer1, integer_t p_integer2)
{
	UITextView *t_view;
	t_view = (UITextView *)GetView();
    
    NSRange t_range;
	if (t_view == nil)
        return;
    
    t_range = NSMakeRange(p_integer1, p_integer2);
    [t_view scrollRangeToVisible: t_range];
}

void MCiOSMultiLineControl::UpdateContentRect()
{
	UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
	
	float t_scale;
	t_scale = MCIPhoneGetNativeControlScale();
	
	CGSize t_content_size;
	t_content_size = [t_view contentSize];
	
	MCU_set_rect(m_content_rect, 0, 0, t_content_size.width * t_scale, t_content_size.height * t_scale);
}

void MCiOSMultiLineControl::HandleEvent(MCNameRef p_message)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target->message(p_message);
		ChangeTarget(t_old_target);
	}
}

void MCiOSMultiLineControl::HandleEndDragEvent(bool p_decelerate)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target->message_with_valueref_args(MCM_scroller_end_drag, p_decelerate ? kMCTrue : kMCFalse);
		ChangeTarget(t_old_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCScrollViewGetContentOffset(UIScrollView *p_view, int32_t &r_x, int32_t &r_y);

void MCiOSMultiLineControl::HandleScrollEvent(void)
{
	MCObject *t_target;
	t_target = GetOwner();
	
	int32_t t_x, t_y;
	m_post_scroll_event = true;
	if (t_target != nil && MCScrollViewGetContentOffset(GetView(), t_x, t_y))
{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target->message_with_args(MCM_scroller_did_scroll, m_content_rect.x + t_x, m_content_rect.y + t_y);
		ChangeTarget(t_old_target);
	}
}

UIView *MCiOSMultiLineControl::CreateView(void)
{
	UITextView *t_view;
	t_view = [[UITextView alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return nil;
	
	[t_view setHidden: YES];
	
	m_delegate = [[com_runrev_livecode_MCiOSMultiLineDelegate alloc] initWithInstance: this view: t_view];
	[t_view setDelegate: m_delegate];
	
	return t_view;
}

void MCiOSMultiLineControl::DeleteView(UIView *p_view)
{
	[p_view setDelegate: nil];
	
	// MW-2013-05-22: [[ Bug 10880 ]] Make sure we release the delegate here as
	//   it might need to refer to the view to deregister itself.
	[m_delegate release];
	m_delegate = nil;
	
	[p_view release];
}

////////////////////////////////////////////////////////////////////////////////
class MCNativeInputNotifyEvent: public MCCustomEvent
{
public:
	// Note that we require p_notification to be a C-string constant as we don't
	// copy it.
	MCNativeInputNotifyEvent(MCiOSInputControl *p_target, MCNameRef p_notification)
	{
		m_target = p_target;
		m_target -> Retain();
		m_notification = p_notification;
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandleNotifyEvent(m_notification);
	}
	
private:
	MCiOSInputControl *m_target;
	MCNameRef m_notification;
};

class MCNativeInputDidChangeEvent: public MCCustomEvent
{
public:
	// Note that we require p_notification to be a C-string constant as we don't
	// copy it.
	MCNativeInputDidChangeEvent(MCiOSInputControl *p_target)
	{
		m_target = p_target;
		m_target -> Retain();
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandleNotifyEvent(MCM_input_text_changed);
		[ m_target -> GetDelegate() setDidChangePending: NO ];
	}
	
private:
	MCiOSInputControl *m_target;
};

static struct { NSString *name; SEL selector; } s_input_notifications[] =
{
	{ UITextFieldTextDidBeginEditingNotification, @selector(didBeginEditing:) },
	{ UITextFieldTextDidEndEditingNotification, @selector(didEndEditing:) },
	{ UITextFieldTextDidChangeNotification, @selector(didChange:) },
	
	{ UITextViewTextDidBeginEditingNotification, @selector(didBeginEditing:) },
	{ UITextViewTextDidEndEditingNotification, @selector(didEndEditing:) },
	{ UITextViewTextDidChangeNotification, @selector(didChange:) },
	{ nil, nil }
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCiOSInputDelegate

- (id)initWithInstance:(MCiOSInputControl*)instance view: (UIView *)view
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	m_didchange_pending = false;
	m_return_pressed = false;
    m_max_text_length = NSUIntegerMax;
	
	for(uint32_t i = 0; s_input_notifications[i] . name != nil; i++)
		[[NSNotificationCenter defaultCenter] addObserver: self
												 selector: s_input_notifications[i] . selector 
													 name: s_input_notifications[i] . name 
												   object: view];
	
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver: self];
	[super dealloc];
}

- (void)didBeginEditing: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCNativeInputNotifyEvent(m_instance, MCM_input_begin_editing));
}

- (void)didEndEditing: (NSNotification *)notification
{
	if (m_return_pressed)
	{
		m_return_pressed = false;
		MCEventQueuePostCustom(new MCNativeInputNotifyEvent(m_instance, MCM_input_return_key));
	}
	MCEventQueuePostCustom(new MCNativeInputNotifyEvent(m_instance, MCM_input_end_editing));
}

- (void)didChange: (NSNotification *)notification
{
	if (!m_didchange_pending)
	{
		m_didchange_pending = true;
		MCEventQueuePostCustom(new MCNativeInputDidChangeEvent(m_instance));
	}
}

- (BOOL)isDidChangePending;
{
	return m_didchange_pending;
}

- (void)setDidChangePending: (BOOL)p_pending;
{
	m_didchange_pending = p_pending;
}

- (NSUInteger)getMaxTextLength;
{
    return m_max_text_length;
}

- (void)setMaxTextLength: (NSUInteger)p_length;
{
    m_max_text_length = p_length;
}


- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    NSUInteger t_old_length = [textField.text length];
    NSUInteger t_replacement_length = [string length];
    NSUInteger t_range_length = range.length;
    
    NSUInteger t_new_length = t_old_length - t_range_length + t_replacement_length;
    
    BOOL t_return_key = [string rangeOfString: @"\n"].location != NSNotFound;
    
    if (t_new_length > m_max_text_length && !t_return_key)
    {
        // Truncate the text if it exceeds maximum allowed length
        // This also covers the case of pasted text
        textField.text = [[textField.text stringByReplacingCharactersInRange:range withString:string] substringToIndex:m_max_text_length];
        return NO;
    }
    
    return YES;
}

- (BOOL)textFieldShouldReturn: (UITextField *)p_field
{
	m_return_pressed = true;
	[p_field resignFirstResponder];
	return NO;
}


- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text
{
	return YES;
}

@end



class MCiOSMultiLineEvent : public MCCustomEvent
{
public:
	MCiOSMultiLineEvent(MCiOSMultiLineControl *p_target, MCNameRef p_message)
	{
		m_target = p_target;
		m_target->Retain();
		m_message = p_message;
	}
	
	void Destroy(void)
	{
		m_target->Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target->HandleEvent(m_message);
	}
	
private:
	MCiOSMultiLineControl *m_target;
	MCNameRef m_message;
};

class MCiOSMultiLineEndDragEvent : public MCCustomEvent
{
public:
	MCiOSMultiLineEndDragEvent(MCiOSMultiLineControl *p_target, bool p_decelerate)
	{
		m_target = p_target;
		m_target->Retain();
		m_decelerate = p_decelerate;
	}
	
	void Destroy(void)
	{
		m_target->Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target->HandleEndDragEvent(m_decelerate);
	}
	
private:
	MCiOSMultiLineControl *m_target;
	bool m_decelerate;
};


class MCiOSMultiLineScrollEvent : public MCCustomEvent
{
public:
	MCiOSMultiLineScrollEvent(MCiOSMultiLineControl *p_target)
	{
		m_target = p_target;
		m_target->Retain();
	}
	
	void Destroy(void)
	{
		m_target->Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target->HandleScrollEvent();
	}
	
private:
	MCiOSMultiLineControl *m_target;
};

@implementation com_runrev_livecode_MCiOSMultiLineDelegate

- (id)initWithInstance:(MCiOSInputControl *)instance view:(UIView *)view
{
	self = [super initWithInstance:instance view:view];
	if (self == nil)
		return nil;
	
	m_verticaltextalign = kMCNativeControlInputVerticalAlignTop;
    
    // SN-2015-10-19: [[ Bug 16234 ]] We need to keep track of our view, as we
    //  will be deallocated after m_instance has cleared up its view.
    m_view = view;

	[view addObserver:self forKeyPath:@"contentSize" options:NSKeyValueObservingOptionNew context:nil];
	
	return self;
}

// MW-2013-05-22: [[ Bug 10880 ]] Make sure we have a 'dealloc' method so observers
//   can be removed that reference this object.
- (void)dealloc
{
    [m_view removeObserver: self forKeyPath:@"contentSize"];
	[super dealloc];
}
   

- (void)scrollViewWillBeginDragging: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCiOSMultiLineEvent((MCiOSMultiLineControl*)m_instance, MCM_scroller_begin_drag);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidEndDragging: (UIScrollView*)scrollView willDecelerate:(BOOL)decelerate
{
	MCCustomEvent *t_event;
	t_event = new MCiOSMultiLineEndDragEvent((MCiOSMultiLineControl*)m_instance, decelerate);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidScroll: (UIScrollView*)scrollView
{
	MCiOSMultiLineControl *t_instance = (MCiOSMultiLineControl*)m_instance;
	if (t_instance != nil && t_instance->m_post_scroll_event)
	{
		t_instance->m_post_scroll_event = false;
		MCCustomEvent *t_event;
		t_event = new MCiOSMultiLineScrollEvent(t_instance);
		MCEventQueuePostCustom(t_event);
	}
}

- (void)scrollViewDidScrollToTop: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCiOSMultiLineEvent((MCiOSMultiLineControl*)m_instance, MCM_scroller_scroll_to_top);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewWillBeginDecelerating:(UIScrollView *)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCiOSMultiLineEvent((MCiOSMultiLineControl*)m_instance, MCM_scroller_begin_decelerate);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidEndDecelerating: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCiOSMultiLineEvent((MCiOSMultiLineControl*)m_instance, MCM_scroller_end_decelerate);
	MCEventQueuePostCustom(t_event);
}

- (void)adjustVerticalAlignment: (UITextView *)textView
{
	switch (m_verticaltextalign)
	{
		case kMCNativeControlInputVerticalAlignTop:
			textView.contentOffset = CGPointMake(0, 0);
			break;

		case kMCNativeControlInputVerticalAlignCenter:
		{
			CGFloat topCorrect = ([textView bounds].size.height - [textView contentSize].height * [textView zoomScale])/2.0;
			topCorrect = ( topCorrect < 0.0 ? 0.0 : topCorrect );
			textView.contentOffset = CGPointMake(0, -topCorrect);
			break;
		}
			
		case kMCNativeControlInputVerticalAlignBottom:
		{
			CGFloat topCorrect = ([textView bounds].size.height - [textView contentSize].height);
			topCorrect = (topCorrect <0.0 ? 0.0 : topCorrect);
			textView.contentOffset = CGPointMake(0, -topCorrect);
			break;
		}
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([@"contentSize" isEqualToString:keyPath])
	{
		UITextView *textView = (UITextView*)object;
		if ([textView bounds].size.height > [textView contentSize].height)
			[self adjustVerticalAlignment: (UITextView*)object];
	}
}

- (void)setVerticalTextAlign:(int32_t)align
{
	m_verticaltextalign = align;
	[self adjustVerticalAlignment: (UITextView*)m_instance->GetView()];
}

- (int32_t)getVerticalTextAlign
{
	return m_verticaltextalign;
}

@end

////////////////////////////////////////////////////////////////////////////////

bool MCNativeInputControlCreate(MCNativeControl*& r_control)
{
	r_control = new MCiOSInputFieldControl;
	return true;
}

bool MCNativeMultiLineInputControlCreate(MCNativeControl *&r_control)
{
	r_control = new MCiOSMultiLineControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
