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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
#include "execpt.h"
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

#import <MediaPlayer/MPMoviePlayerController.h>

#include "mbliphonecontrol.h"
#include "mbliphone.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
UIView *MCIPhoneGetView(void);

////////////////////////////////////////////////////////////////////////////////

class MCiOSInputControl;

// Note that we use the notifications, rather than delegate methods.
@interface MCiOSInputDelegate : NSObject <UITextFieldDelegate, UITextViewDelegate>
{
	MCiOSInputControl *m_instance;
	bool m_didchange_pending;
	bool m_return_pressed;
}

- (id)initWithInstance:(MCiOSInputControl*)instance view:(UIView *)view;
- (void)dealloc;

- (void)didBeginEditing: (NSNotification *)notification;
- (void)didEndEditing: (NSNotification *)notification;
- (void)didChange: (NSNotification *)notification;

- (BOOL)isDidChangePending;
- (void)setDidChangePending: (BOOL)pending;

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string;
- (BOOL)textFieldShouldReturn: (UITextField *)p_field;


- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text;
@end


@interface MCiOSMultiLineDelegate : MCiOSInputDelegate <UIScrollViewDelegate>
{
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
public:
	MCiOSInputControl(void);
#ifdef LEGACY_EXEC
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep);	
    virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
#endif	
    
    virtual void Set(MCExecContext& ctxt, MCNativeControlProperty p_property);
    virtual void Get(MCExecContext& ctxt, MCNativeControlProperty p_property);
    virtual Exec_stat Do(MCExecContext& ctxt, MCNativeControlAction p_action, MCParameter *parameters);
    
    void SetEnabled(MCExecContext& ctxt, bool p_value);
    virtual void SetOpaque(MCExecContext& ctxt, bool p_value);
    void SetText(MCExecContext& ctxt, MCStringRef p_string);
    void SetTextColor(MCExecContext& ctxt, const MCNativeControlColor& p_color);
    void SetFontName(MCExecContext& ctxt, MCStringRef p_font);
    void SetFontSize(MCExecContext& ctxt, integer_t p_size);
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
    void GetFontSize(MCExecContext& ctxt, integer_t& r_size);
    void GetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign& r_align);

    void GetAutoCapitalizationType(MCExecContext& ctxt, MCNativeControlInputCapitalizationType& r_type);
    void GetAutoCorrectionType(MCExecContext& ctxt, MCNativeControlInputAutocorrectionType& r_type);
    void GetKeyboardType(MCExecContext& ctxt, MCNativeControlInputKeyboardType& r_type);
    void GetManageReturnKey(MCExecContext& ctxt, bool& r_value);
    void GetKeyboardStyle(MCExecContext& ctxt, MCNativeControlInputKeyboardStyle& r_style);
    void GetReturnKey(MCExecContext& ctxt, MCNativeControlInputReturnKeyType& r_key);
    void GetContentType(MCExecContext& ctxt, MCNativeControlInputContentType& r_type);
    
	// Input-specific actions
	Exec_stat ExecFocus(MCExecContext& ctxt);
    
	void HandleNotifyEvent(MCNameRef p_notification);

	MCiOSInputDelegate *GetDelegate(void);
	
protected:
	virtual ~MCiOSInputControl(void);
	
	MCiOSInputDelegate *m_delegate;
};

// single line input control
class MCiOSInputFieldControl: public MCiOSInputControl
{
public:
	virtual MCNativeControlType GetType(void);
#ifdef LEGACY_EXEC	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep);
#endif
    virtual void Set(MCExecContext& ctxt, MCNativeControlProperty p_property);
    virtual void Get(MCExecContext& ctxt, MCNativeControlProperty p_property);
    
    void SetAutoFit(MCExecContext& ctxt, bool p_value);
    void SetMinimumFontSize(MCExecContext& ctxt, integer_t p_size);
    void SetAutoClear(MCExecContext& ctxt, bool p_value);
    void SetClearButtonMode(MCExecContext& ctxt, MCNativeControlClearButtonMode p_mode);
    void SetBorderStyle(MCExecContext& ctxt, MCNativeControlBorderStyle p_style);
    
    void GetAutoFit(MCExecContext& ctxt, bool& r_value);
    void GetMinimumFontSize(MCExecContext& ctxt, integer_t& r_size);
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
#ifdef LEGACY_EXEC	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
#endif
	
    virtual void Set(MCExecContext& ctxt, MCNativeControlProperty p_property);
    virtual void Get(MCExecContext& ctxt, MCNativeControlProperty p_property);
	virtual Exec_stat Do(MCExecContext& ctxt, MCNativeControlAction action, MCParameter *parameters);
    
    void SetEditable(MCExecContext& ctxt, bool p_value);
    void SetSelectedRange(MCExecContext& ctxt, const MCNativeControlRange& p_range);
    void SetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType p_type);
    void SetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign p_align);
    virtual void SetContentRect(MCExecContext& ctxt, MCRectangle32 p_rect);
    
    void GetEditable(MCExecContext& ctxt, bool& r_value);
    void GetSelectedRange(MCExecContext& ctxt, MCNativeControlRange& r_range);
    void GetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType& r_type);
    void GetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign& r_align);
    
    void GetContentRect(MCExecContext& ctxt, MCRectangle32& r_rect);
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
	Exec_stat ExecScrollRangeToVisible(MCExecContext& ctxt, int32_t p_integer1, int32_t p_integer2);
    
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
	{"left", UITextAlignmentLeft},
	{"right", UITextAlignmentRight},
	{"center", UITextAlignmentCenter},
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
        NSString *t_string;
        t_string = [NSString stringWithCString: MCStringGetCString(p_string)];
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
        t_string = [NSString stringWithCString: MCStringGetCString(p_font)];
        [t_field setFont: [UIFont fontWithName: t_string size: [[t_field font] pointSize]]];
    }
}

void MCiOSInputControl::SetFontSize(MCExecContext& ctxt, integer_t p_size)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
        [t_field setFont: [[t_field font] fontWithSize: p_size]];
}

void MCiOSInputControl::SetTextAlign(MCExecContext& ctxt, MCNativeControlInputTextAlign p_align)
{
    UITextField *t_field;
	t_field = (UITextField *)GetView();
    
    if (t_field)
    {
        UITextAlignment t_align;
        
        switch (p_align)
        {
            case kMCNativeControlInputTextAlignCenter:
                t_align = UITextAlignmentCenter;
                break;
            case kMCNativeControlInputTextAlignLeft:
                t_align = UITextAlignmentLeft;
                break;
            case kMCNativeControlInputTextAlignRight:
                t_align = UITextAlignmentRight;
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
        [t_field setKeyboardAppearance: t_type];
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
        if (NSStringToMCStringRef([t_field text], r_string))
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
        if (NSStringToMCStringRef([[t_field font] fontName], r_font))
            return;
    }
    else
        r_font = MCValueRetain(kMCEmptyString);
    
    ctxt . Throw();
}

void MCiOSInputControl::GetFontSize(MCExecContext& ctxt, integer_t& r_size)
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
            case UITextAlignmentCenter:
                r_align = kMCNativeControlInputTextAlignCenter;
                return;
            case UITextAlignmentLeft:
                r_align = kMCNativeControlInputTextAlignLeft;
                return;
            case UITextAlignmentRight:
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

#ifdef /* MCNativeInputControl::Set */ LEGACY_EXEC
Exec_stat MCiOSInputControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
	if (t_field == nil)
		return MCiOSControl::Set(p_property, ep);
	
	bool t_bool;
	NSString *t_string;
	int32_t t_integer;
	int32_t t_enum;
	UIColor *t_color;
	
	switch(p_property)
	{	
		case kMCNativeControlPropertyEnabled:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			[t_field setEnabled: t_bool];
			return ES_NORMAL;

		case kMCNativeControlPropertyOpaque:
			if (MCiOSControl::Set(p_property, ep) != ES_NORMAL)
				return ES_ERROR;
			[t_field setNeedsDisplay];
			return ES_NORMAL;
			 
		case kMCNativeControlPropertyText:
			if (!ParseString(ep, t_string))
				return ES_ERROR;
			[t_field setText: t_string];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyUnicodeText:
			if (!ParseUnicodeString(ep, t_string))
				return ES_ERROR;
			[t_field setText: t_string];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyTextColor:
			if (!ParseColor(ep, t_color))
				return ES_ERROR;
			[t_field setTextColor: t_color];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyFontName:
			if (!ParseString(ep, t_string))
				return ES_ERROR;
			[t_field setFont: [UIFont fontWithName: t_string size: [[t_field font] pointSize]]];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyFontSize:
			if (!ParseInteger(ep, t_integer))
				return ES_ERROR;
			[t_field setFont: [[t_field font] fontWithSize: t_integer]];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyTextAlign:
			if (!ParseEnum(ep, s_textalign_enum, t_enum))
				return ES_ERROR;
			[t_field setTextAlignment: (UITextAlignment)t_enum];
			return ES_NORMAL;
			
		/////////
			
		case kMCNativeControlPropertyAutoCapitalizationType:
			if (!ParseEnum(ep, s_autocapitalizationtype_enum, t_enum))
				return ES_ERROR;
			[t_field setAutocapitalizationType: (UITextAutocapitalizationType)t_enum];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyAutoCorrectionType:
			if (!ParseEnum(ep, s_autocorrectiontype_enum, t_enum))
				return ES_ERROR;
			[t_field setAutocorrectionType: (UITextAutocorrectionType)t_enum];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyManageReturnKey:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			[t_field setEnablesReturnKeyAutomatically: t_bool];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyKeyboardStyle:
			if (!ParseEnum(ep, s_keyboardstyle_enum, t_enum))
				return ES_ERROR;
			[t_field setKeyboardAppearance: (UIKeyboardAppearance)t_enum];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyKeyboardType:
			if (!ParseEnum(ep, s_keyboardtype_enum, t_enum))
				return ES_ERROR;
			[t_field setKeyboardType: (UIKeyboardType)t_enum];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyReturnKeyType:
			if (!ParseEnum(ep, s_returnkeytype_enum, t_enum))
				return ES_ERROR;
			[t_field setReturnKeyType: (UIReturnKeyType)t_enum];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyContentType:
			if (!ParseEnum(ep, s_contenttype_enum, t_enum))
				return ES_ERROR;
			if (t_enum == kMCTextContentTypePassword)
				[t_field setSecureTextEntry: YES];
			else
				[t_field setSecureTextEntry: NO];
			return ES_NORMAL;
			
		default:
			break;
	}
	
	return MCiOSControl::Set(p_property, ep);
}
#endif /* MCNativeInputControl::Set */

void MCiOSInputControl::Set(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
	switch(p_property)
	{
		case kMCNativeControlPropertyEnabled:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetEnabled(ctxt, t_value);
            return;
        }
            
		case kMCNativeControlPropertyOpaque:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetOpaque(ctxt, t_value);
            return;
        }
            
		case kMCNativeControlPropertyText:
        case kMCNativeControlPropertyUnicodeText:
        {
            MCAutoStringRef t_text;
            /* UNCHECKED */ ep . copyasstringref(&t_text);
            SetText(ctxt, *t_text);
            return;
        }

		case kMCNativeControlPropertyTextColor:
        {
            MCAutoStringRef t_string;
            MCNativeControlColor t_color;
            /* UNCHECKED */ ep . copyasstringref(&t_string);
            MCNativeControlColorParse(ctxt, *t_string, t_color);
            if (!ctxt . HasError())
                SetTextColor(ctxt, t_color);
            return;
        }
			
		case kMCNativeControlPropertyFontName:
        {
            MCAutoStringRef t_font;
            /* UNCHECKED */ ep . copyasstringref(&t_font);
            SetFontName(ctxt, *t_font);
            return;
        }
			
		case kMCNativeControlPropertyFontSize:
        {
			int32_t t_size;
            if (!ep . copyasint(t_size))
                ctxt . LegacyThrow(EE_OBJECT_NAN);
            else
                SetFontSize(ctxt, t_size);
            return;
        }
			
		case kMCNativeControlPropertyTextAlign:
        {
            intenum_t t_align;
            ctxt_to_enum(ctxt, kMCNativeControlInputTextAlignTypeInfo, t_align);
            SetTextAlign(ctxt, (MCNativeControlInputTextAlign)t_align);
            return;
        }
            /////////
			
		case kMCNativeControlPropertyAutoCapitalizationType:
        {
            intenum_t t_type;
            ctxt_to_enum(ctxt, kMCNativeControlInputCapitalizationTypeTypeInfo, t_type);
            SetAutoCapitalizationType(ctxt, (MCNativeControlInputCapitalizationType)t_type);
            return;
        }
		case kMCNativeControlPropertyAutoCorrectionType:
        {
            intenum_t t_type;
            ctxt_to_enum(ctxt, kMCNativeControlInputAutocorrectiontionTypeTypeInfo, t_type);
            SetAutoCorrectionType(ctxt, (MCNativeControlInputAutocorrectionType)t_type);
            return;
		}
		case kMCNativeControlPropertyManageReturnKey:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetManageReturnKey(ctxt, t_value);
            return;
        }
		case kMCNativeControlPropertyKeyboardStyle:
        {
            intenum_t t_style;
            ctxt_to_enum(ctxt, kMCNativeControlInputKeyboardStyleTypeInfo, t_style);
            SetKeyboardStyle(ctxt, (MCNativeControlInputKeyboardStyle)t_style);
            return;
		}
		case kMCNativeControlPropertyKeyboardType:
        {
            intenum_t t_type;
            ctxt_to_enum(ctxt, kMCNativeControlInputKeyboardTypeTypeInfo, t_type);
            SetKeyboardType(ctxt, (MCNativeControlInputKeyboardType)t_type);
            return;
		}
		case kMCNativeControlPropertyReturnKeyType:
        {
            intenum_t t_type;
            ctxt_to_enum(ctxt, kMCNativeControlInputReturnKeyTypeTypeInfo, t_type);
            SetReturnKey(ctxt, (MCNativeControlInputReturnKeyType)t_type);
            return;
		}
		case kMCNativeControlPropertyContentType:
        {
            intenum_t t_type;
            ctxt_to_enum(ctxt, kMCNativeControlInputContentTypeTypeInfo, t_type);
            SetContentType(ctxt, (MCNativeControlInputContentType)t_type);
            return;
        }
		default:
			break;
	}
	
	MCiOSControl::Set(ctxt, p_property);
}

#ifdef /* MCNativeInputControl::Get */ LEGACY_EXEC
Exec_stat MCiOSInputControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
	if (t_field == nil)
		return MCiOSControl::Get(p_property, ep);
	
	switch(p_property)
	{
		case kMCNativeControlPropertyEnabled:
			FormatBoolean(ep, [t_field isEnabled]);
			return ES_NORMAL;
            
		case kMCNativeControlPropertyText:
			FormatString(ep, [t_field text]);
			return ES_NORMAL;
		case kMCNativeControlPropertyUnicodeText:
			FormatUnicodeString(ep, [t_field text]);
			return ES_NORMAL;
		case kMCNativeControlPropertyTextColor:
			FormatColor(ep, [t_field textColor]);
			return ES_NORMAL;
		case kMCNativeControlPropertyFontName:
			FormatString(ep, [[t_field font] fontName]);
			return ES_NORMAL;
		case kMCNativeControlPropertyFontSize:
			FormatInteger(ep, (int32_t)[[t_field font] pointSize]);
			return ES_NORMAL;
		case kMCNativeControlPropertyTextAlign:
			FormatEnum(ep, s_textalign_enum, [t_field textAlignment]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyAutoCapitalizationType:
			FormatEnum(ep, s_autocapitalizationtype_enum, [t_field autocapitalizationType]);
			return ES_NORMAL;
		case kMCNativeControlPropertyAutoCorrectionType:
			FormatEnum(ep, s_autocorrectiontype_enum, [t_field autocorrectionType]);
			return ES_NORMAL;
		case kMCNativeControlPropertyManageReturnKey:
			FormatBoolean(ep, [t_field enablesReturnKeyAutomatically]);
			return ES_NORMAL;
		case kMCNativeControlPropertyKeyboardStyle:
			FormatEnum(ep, s_keyboardstyle_enum, [t_field keyboardAppearance]);
			return ES_NORMAL;
		case kMCNativeControlPropertyKeyboardType:
			FormatEnum(ep, s_keyboardtype_enum, [t_field keyboardType]);
			return ES_NORMAL;
		case kMCNativeControlPropertyReturnKeyType:
			FormatEnum(ep, s_returnkeytype_enum, [t_field returnKeyType]);
			return ES_NORMAL;
		case kMCNativeControlPropertyContentType:
			if ([t_field isSecureTextEntry])
				ep.setsvalue("password");
			else
				ep.setsvalue("plain");
			return ES_NORMAL;
			
		default:
			break;
	}
	
	return MCiOSControl::Get(p_property, ep);
}
#endif /* MCNativeInputControl::Get */

void MCiOSInputControl::Get(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
	switch(p_property)
	{	
		case kMCNativeControlPropertyEnabled:
        {
            bool t_enabled;
			GetEnabled(ctxt, t_enabled);
            ep . setbool(t_enabled);
            return;
        }
		case kMCNativeControlPropertyText:
        case kMCNativeControlPropertyUnicodeText:
        {
			MCAutoStringRef t_text;
            GetText(ctxt, &t_text);
            if (*t_text != nil)
                ep . setvalueref(*t_text);
			return;
        }
		case kMCNativeControlPropertyTextColor:
        {
            MCNativeControlColor t_color;
            MCAutoStringRef t_string;
            GetTextColor(ctxt, t_color);
            MCNativeControlColorFormat(ctxt, t_color, &t_string);
            if (*t_string != nil)
                ep . setvalueref(*t_string);
            return;
        }
		case kMCNativeControlPropertyFontName:
        {
            MCAutoStringRef t_name;
            GetFontName(ctxt, &t_name);
            if (*t_name != nil)
                ep . setvalueref(*t_name);
            return;
        }
		case kMCNativeControlPropertyFontSize:
        {
            int32_t t_size;
            GetFontSize(ctxt, t_size);
            ep . setnvalue(t_size);
            return;
        }
		case kMCNativeControlPropertyTextAlign:
        {
             MCNativeControlInputTextAlign t_align;
             GetTextAlign(ctxt, t_align);
             enum_to_ctxt(ctxt, kMCNativeControlInputTextAlignTypeInfo, t_align);
             return;
        }
             /////////
             
        case kMCNativeControlPropertyAutoCapitalizationType:
        {
             MCNativeControlInputCapitalizationType t_type;
             GetAutoCapitalizationType(ctxt, t_type);
             enum_to_ctxt(ctxt, kMCNativeControlInputCapitalizationTypeTypeInfo, t_type);
             return;
        }
		case kMCNativeControlPropertyAutoCorrectionType:
        {
            MCNativeControlInputAutocorrectionType t_type;
            GetAutoCorrectionType(ctxt, t_type);
            enum_to_ctxt(ctxt, kMCNativeControlInputAutocorrectiontionTypeTypeInfo, t_type);
            return;
        }
		case kMCNativeControlPropertyManageReturnKey:
        {
            bool t_value;
			GetManageReturnKey(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
		case kMCNativeControlPropertyKeyboardStyle:
        {
            MCNativeControlInputKeyboardStyle t_style;
            GetKeyboardStyle(ctxt, t_style);
            enum_to_ctxt(ctxt, kMCNativeControlInputKeyboardStyleTypeInfo, t_style);
            return;
        }
		case kMCNativeControlPropertyKeyboardType:
        {
            MCNativeControlInputKeyboardType t_type;
            GetKeyboardType(ctxt, t_type);
            enum_to_ctxt(ctxt, kMCNativeControlInputKeyboardTypeTypeInfo, t_type);
            return;
        }
		case kMCNativeControlPropertyReturnKeyType:
        {
            MCNativeControlInputReturnKeyType t_type;
            GetReturnKey(ctxt, t_type);
            enum_to_ctxt(ctxt, kMCNativeControlInputReturnKeyTypeTypeInfo, t_type);
            return;
        }
		case kMCNativeControlPropertyContentType:
        {
            MCNativeControlInputContentType t_type;
            GetContentType(ctxt, t_type);
            enum_to_ctxt(ctxt, kMCNativeControlInputContentTypeTypeInfo, t_type);
            return;
        }
		default:
			break;
	}
	
	MCiOSControl::Get(ctxt, p_property);
}

#ifdef /* MCiOSInputControl::Do */ LEGACY_EXEC
Exec_stat MCiOSInputControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
	if (t_field == nil)
		return MCiOSControl::Do(p_action, p_parameters);
	
	switch(p_action)
	{
		case kMCNativeControlActionFocus:
			[t_field becomeFirstResponder];
			return ES_NORMAL;
			
		default:
			break;
	}
	
	return MCiOSControl::Do(p_action, p_parameters);
}
#endif /* MCiOSInputControl::Do */

Exec_stat MCiOSInputControl::Do(MCExecContext& ctxt, MCNativeControlAction p_action, MCParameter *p_parameters)
{
	switch(p_action)
	{
		case kMCNativeControlActionFocus:
            if (ExecFocus(ctxt) != ES_NORMAL)
                break;
            return ES_NORMAL;
			
		default:
			break;
	}
	
	return MCiOSControl::Do(ctxt, p_action, p_parameters);
}

Exec_stat MCiOSInputControl::ExecFocus(MCExecContext &ctxt)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
	if (t_field == nil)
        return ES_NOT_HANDLED;
    
    [t_field becomeFirstResponder];
    return ES_NORMAL;   
}

////////////////////////////////////////////////////////////////////////////////

MCiOSInputDelegate *MCiOSInputControl::GetDelegate(void)
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

#ifdef /* MCNativeInputFieldControl::Set */ LEGACY_EXEC
Exec_stat MCiOSInputFieldControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
	if (t_field == nil)
		return MCiOSControl::Set(p_property, ep);
	
	bool t_bool;
	NSString *t_string;
	int32_t t_integer;
	int32_t t_enum;
	UIColor *t_color;
	
	switch(p_property)
	{	
		case kMCNativeControlPropertyAutoFit:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			[t_field setAdjustsFontSizeToFitWidth: t_bool];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyMinimumFontSize:
			if (!ParseInteger(ep, t_integer))
				return ES_ERROR;
			[t_field setMinimumFontSize: t_integer];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyAutoClear:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			[t_field setClearsOnBeginEditing: t_bool];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyClearButtonMode:
			if (!ParseEnum(ep, s_clearbuttonmode_enum, t_enum))
				return ES_ERROR;
			[t_field setClearButtonMode: (UITextFieldViewMode)t_enum];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyBorderStyle:
			if (!ParseEnum(ep, s_borderstyle_enum, t_enum))
				return ES_ERROR;
			[t_field setBorderStyle: (UITextBorderStyle)t_enum];
			return ES_NORMAL;
			
			/////////
			
		default:
			break;
	}
	
	return MCiOSInputControl::Set(p_property, ep);
}
#endif /* MCNativeInputFieldControl::Set */

void MCiOSInputFieldControl::Set(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
	switch(p_property)
	{
		case kMCNativeControlPropertyAutoFit:
        {
            bool t_autofit;
            if (!ep . copyasbool(t_autofit))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetAutoFit(ctxt, t_autofit);
            return;
        }
		case kMCNativeControlPropertyMinimumFontSize:
        {
            int32_t t_size;
            if (!ep . copyasint(t_size))
                ctxt . LegacyThrow(EE_OBJECT_NAN);
            else
                SetMinimumFontSize(ctxt, t_size);
            return;
        }
			
		case kMCNativeControlPropertyAutoClear:
        {
            bool t_autoclear;
            if (!ep . copyasbool(t_autoclear))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetAutoClear(ctxt, t_autoclear);
            return;
        }
			
		case kMCNativeControlPropertyClearButtonMode:
        {
            intenum_t t_mode;
            ctxt_to_enum(ctxt, kMCNativeControlClearButtonModeTypeInfo, t_mode);
            SetClearButtonMode(ctxt, (MCNativeControlClearButtonMode)t_mode);
            return;
        }
		case kMCNativeControlPropertyBorderStyle:
        {
            intenum_t t_style;
            ctxt_to_enum(ctxt, kMCNativeControlBorderStyleTypeInfo, t_style);
            SetBorderStyle(ctxt, (MCNativeControlBorderStyle)t_style);
            return;
        }
			
			/////////
			
		default:
			break;
	}
	
	MCiOSInputControl::Set(ctxt, p_property);
}

#ifdef /* MCNativeInputFieldControl::Get */ LEGACY_EXEC
Exec_stat MCiOSInputFieldControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UITextField *t_field;
	t_field = (UITextField *)GetView();
	if (t_field == nil)
		return MCiOSControl::Get(p_property, ep);
	
	switch(p_property)
	{	
		case kMCNativeControlPropertyAutoFit:
			FormatBoolean(ep, [t_field adjustsFontSizeToFitWidth]);
			return ES_NORMAL;
		case kMCNativeControlPropertyMinimumFontSize:
			FormatInteger(ep, [t_field minimumFontSize]);
			return ES_NORMAL;
		case kMCNativeControlPropertyAutoClear:
			FormatBoolean(ep, [t_field clearsOnBeginEditing]);
			return ES_NORMAL;
		case kMCNativeControlPropertyClearButtonMode:
			FormatEnum(ep, s_clearbuttonmode_enum, [t_field clearButtonMode]);
			return ES_NORMAL;
		case kMCNativeControlPropertyBorderStyle:
			FormatEnum(ep, s_borderstyle_enum, [t_field borderStyle]);
			return ES_NORMAL;
		case kMCNativeControlPropertyEditing:
			FormatBoolean(ep, [t_field isEditing]);
			return ES_NORMAL;
			
		default:
			break;
	}
	
	return MCiOSInputControl::Get(p_property, ep);
}
#endif /*MCNativeInputFieldControl::Get */

void MCiOSInputFieldControl::Get(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
	switch(p_property)
	{
		case kMCNativeControlPropertyAutoFit:
        {
            bool t_autofit;
            GetAutoFit(ctxt, t_autofit);
            ep . setbool(t_autofit);
            return;
        }
		case kMCNativeControlPropertyMinimumFontSize:
        {
            int32_t t_size;
            GetMinimumFontSize(ctxt, t_size);
            ep . setnvalue(t_size);
            return;
        }
			
		case kMCNativeControlPropertyAutoClear:
        {
            bool t_autoclear;
            GetAutoClear(ctxt, t_autoclear);
            ep . setbool(t_autoclear);
            return;
        }
			
		case kMCNativeControlPropertyClearButtonMode:
        {
            MCNativeControlClearButtonMode t_mode;
            GetClearButtonMode(ctxt, t_mode);
            enum_to_ctxt(ctxt, kMCNativeControlClearButtonModeTypeInfo, t_mode);
            return;
        }
		case kMCNativeControlPropertyBorderStyle:
        {
            MCNativeControlBorderStyle t_style;
            GetBorderStyle(ctxt, t_style);
            enum_to_ctxt(ctxt, kMCNativeControlBorderStyleTypeInfo, t_style);
            return;
        }
			
			/////////
			
		default:
			break;
	}
    
    MCiOSInputControl::Set(ctxt, p_property);
}

UIView *MCiOSInputFieldControl::CreateView(void)
{
	UITextField *t_view;
	t_view = [[UITextField alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return nil;
	
	[t_view setHidden: YES];
	
	m_delegate = [[MCiOSInputDelegate alloc] initWithInstance: this view: t_view];
	[t_view setDelegate: m_delegate];
	
	return t_view;
}

void MCiOSInputFieldControl::DeleteView(UIView *p_view)
{
	[p_view setDelegate: nil];
	[p_view release];
	
	[m_delegate release];
	m_delegate = nil;
}

////////////////////////////////////////////////////////////////////////////////

MCNativeControlType MCiOSMultiLineControl::GetType(void)
{
	return kMCNativeControlTypeInput;
}

bool datadetectortypes_from_string(const char *p_list, UIDataDetectorTypes &r_types);
bool datadetectortypes_to_string(UIDataDetectorTypes p_types, char *&r_list);

Exec_stat scroller_set_property(UIScrollView *p_view, MCRectangle32 &x_content_rect, MCNativeControlProperty p_property, MCExecPoint&ep);
Exec_stat scroller_get_property(UIScrollView *p_view, const MCRectangle32 &p_content_rect, MCNativeControlProperty p_property, MCExecPoint &ep);

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
        if (p_type & kMCNativeControlInputDataDetectorTypeEmailAddress)
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
       	MCiOSMultiLineDelegate *t_delegate;
        t_delegate = (MCiOSMultiLineDelegate*)m_delegate;
        
        [t_delegate setVerticalTextAlign:(int32_t)p_align];
    }
}

void MCiOSMultiLineControl::SetContentRect(MCExecContext& ctxt, MCRectangle32 p_rect)
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
            t_types |= kMCNativeControlInputDataDetectorTypeEmailAddress;
            t_types |= kMCNativeControlInputDataDetectorTypePhoneNumber;
            t_types |= kMCNativeControlInputDataDetectorTypeWebUrl;
        }
        
        if (t_native_types & UIDataDetectorTypeCalendarEvent)
            t_types |= kMCNativeControlInputDataDetectorTypeCalendarEvent;
        if (t_native_types & UIDataDetectorTypeAddress)
            t_types |= kMCNativeControlInputDataDetectorTypeEmailAddress;
        if (t_native_types & UIDataDetectorTypePhoneNumber)
            t_types |= kMCNativeControlInputDataDetectorTypePhoneNumber;
        if (t_native_types & UIDataDetectorTypeLink)
            t_types |= kMCNativeControlInputDataDetectorTypeWebUrl;
    }
    
    r_type = (MCNativeControlInputDataDetectorType)t_types;
}

void MCiOSMultiLineControl::GetVerticalTextAlign(MCExecContext& ctxt, MCNativeControlInputVerticalAlign& r_align)
{
	MCiOSMultiLineDelegate *t_delegate;
	t_delegate = (MCiOSMultiLineDelegate*)m_delegate;
 
    if (t_delegate)
        r_align = (MCNativeControlInputVerticalAlign)[t_delegate getVerticalTextAlign];
        
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSMultiLineControl::GetContentRect(MCExecContext& ctxt, MCRectangle32& r_rect)
{
    UpdateContentRect();
    if (GetView())
        r_rect = m_content_rect;
}

void MCiOSMultiLineControl::SetHScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    UpdateContentRect();
    
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    int32_t t_x, t_y;
    if (t_view != nil && MCScrollViewGetContentOffset(t_view, t_x, t_y))
        [t_view setContentOffset: CGPointMake((float)(p_scroll - m_content_rect.x) / t_scale, (float)t_y / t_scale)];
}

void MCiOSMultiLineControl::GetHScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    UpdateContentRect();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
    {
        float t_scale;
        t_scale = MCIPhoneGetNativeControlScale();
        
        r_scroll = m_content_rect.x + [t_view contentOffset].x * t_scale;
    }
    else
        r_scroll = 0;
}
void MCiOSMultiLineControl::SetVScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    UpdateContentRect();
    
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    int32_t t_x, t_y;
    if (t_view != nil && MCScrollViewGetContentOffset(t_view, t_x, t_y))
        [t_view setContentOffset: CGPointMake((float)t_x / t_scale, (float)(p_scroll - m_content_rect.y) / t_scale)];
}

void MCiOSMultiLineControl::GetVScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    UpdateContentRect();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
    {
        float t_scale;
        t_scale = MCIPhoneGetNativeControlScale();
        
        r_scroll = m_content_rect.y + [t_view contentOffset].y * t_scale;
    }
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
        [t_view setCanCancelContentTouches: p_value];
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
        [t_view setDelaysContentTouches: p_value];
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
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setScrollIndicatorInsets: UIEdgeInsetsMake((float)p_insets . top / t_scale, (float)p_insets . left / t_scale, (float)p_insets . bottom / t_scale, (float)p_insets . right / t_scale)];
}
void MCiOSMultiLineControl::GetIndicatorInsets(MCExecContext& ctxt, MCNativeControlIndicatorInsets& r_insets)
{
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    UIEdgeInsets t_insets;
    t_insets = [t_view scrollIndicatorInsets];
    
    r_insets . left = (int16_t)(t_insets.left * t_scale);
    r_insets . top = (int16_t)(t_insets.top * t_scale);
    r_insets . right = (int16_t)(t_insets.right * t_scale);
    r_insets . bottom = (int16_t)(t_insets.bottom * t_scale);
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


#ifdef /* MCNativeMultiLineControl::Set */ LEGACY_EXEC
Exec_stat MCiOSMultiLineControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UITextView *t_view;
	t_view = (UITextView *)GetView();
	if (t_view == nil)
		return MCiOSControl::Set(p_property, ep);
	
	MCiOSMultiLineDelegate *t_delegate;
	t_delegate = (MCiOSMultiLineDelegate*)m_delegate;
	
	bool t_bool;
	NSString *t_string;
	int32_t t_integer;
	int32_t t_enum;
	UIColor *t_color;
	NSRange t_range;
	
	switch(p_property)
	{	
		case kMCNativeControlPropertyEditable:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			[t_view setEditable: t_bool];
			return ES_NORMAL;
			
		case kMCNativeControlPropertySelectedRange:
			if (!ParseRange(ep, t_range))
				return ES_ERROR;
			[t_view setSelectedRange: t_range];
			return ES_NORMAL;
			
			/////////
			
		case kMCNativeControlPropertyDataDetectorTypes:
			UIDataDetectorTypes t_data_types;
			t_data_types = UIDataDetectorTypeNone;
			if (ep.isempty() || ep.getsvalue() == "none")
				t_data_types = UIDataDetectorTypeNone;
			else if (ep.getsvalue() == "all")
				t_data_types = UIDataDetectorTypeAll;
			else
			{
				if (!datadetectortypes_from_string(ep.getcstring(), t_data_types))
				{
					MCeerror->add(EE_UNDEFINED, 0, 0, ep.getsvalue());
					return ES_ERROR;
				}
			}
			
			[t_view setDataDetectorTypes: t_data_types];
			
			return ES_NORMAL;

		case kMCNativeControlPropertyVerticalTextAlign:
			if (!ParseEnum(ep, s_verticaltextalign_enum, t_enum))
				return ES_ERROR;
			[t_delegate setVerticalTextAlign:t_enum];
			return ES_NORMAL;
			
			// setting the contentRect is not supported for multiline text fields
		case kMCNativeControlPropertyContentRectangle:
			return ES_NOT_HANDLED;

		default:
			break;
	}
	
	UpdateContentRect();
	
	Exec_stat t_state;
	t_state = scroller_set_property(t_view, m_content_rect, p_property, ep);
	if (t_state == ES_NOT_HANDLED)
		return MCiOSInputControl::Set(p_property, ep);
	else
		return t_state;
}
#endif /* MCNativeMultiLineControl::Set */

void MCiOSMultiLineControl::Set(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
	MCExecPoint& ep = ctxt . GetEP();
	
	switch(p_property)
	{
		case kMCNativeControlPropertyEditable:
        {
            bool t_editable;
            if (!ep . copyasbool(t_editable))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetEditable(ctxt, t_editable);
            return;
        }
			
		case kMCNativeControlPropertySelectedRange:
        {
            MCNativeControlRange t_range;
            MCAutoStringRef t_string;
            /* UNCHECKED */ ep . copyasstringref(&t_string);
            MCNativeControlRangeParse(ctxt, *t_string, t_range);
            if (!ctxt . HasError())
                SetSelectedRange(ctxt, t_range);
        }
			
			/////////
			
		case kMCNativeControlPropertyDataDetectorTypes:
        {
            intset_t t_types;
            ctxt_to_set(ctxt, kMCNativeControlInputDataDetectorTypeTypeInfo, t_types);
            SetDataDetectorTypes(ctxt, (MCNativeControlInputDataDetectorType)t_types);
            return;
        }
            
		case kMCNativeControlPropertyVerticalTextAlign:
		{
            intenum_t t_align;
            ctxt_to_enum(ctxt, kMCNativeControlInputVerticalAlignTypeInfo, t_align);
            SetVerticalTextAlign(ctxt, (MCNativeControlInputVerticalAlign)t_align);
            return;
        }
			
        // setting the contentRect is not supported for multiline text fields
		case kMCNativeControlPropertyContentRectangle:
            return;
///////////////////////////////////////////////////////////////////////////////////////////
/* SCROLLER PROPS */
	
		case kMCNativeControlPropertyHScroll:
		{
			int32_t t_hscroll;
			if (!ep . copyasint(t_hscroll))
                ctxt . LegacyThrow(EE_OBJECT_NAN);
            else
                SetHScroll(ctxt, t_hscroll);
            return;
		}
		case kMCNativeControlPropertyVScroll:
		{
			int32_t t_vscroll;
			if (!ep . copyasint(t_vscroll))
                ctxt . LegacyThrow(EE_OBJECT_NAN);
            else
                SetVScroll(ctxt, t_vscroll);
            return;
		}
            
		case kMCNativeControlPropertyCanBounce:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetCanBounce(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyCanScrollToTop:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetCanScrollToTop(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyCanCancelTouches:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetCanCancelTouches(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyDelayTouches:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetDelayTouches(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyPagingEnabled:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetPagingEnabled(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyScrollingEnabled:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetScrollingEnabled(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyDecelerationRate:
        {
			MCNativeControlDecelerationRate t_rate;
            MCAutoStringRef t_string;
            /* UNCHECKED */ ep . copyasstringref(&t_string);
            MCNativeControlDecelerationRateParse(ctxt, *t_string, t_rate);
            if (!ctxt . HasError())
                SetDecelerationRate(ctxt, t_rate);
            return;
        }
            
		case kMCNativeControlPropertyIndicatorStyle:
        {
            intenum_t t_style;
            ctxt_to_enum(ctxt, kMCNativeControlIndicatorStyleTypeInfo, t_style);
            SetIndicatorStyle(ctxt, (MCNativeControlIndicatorStyle)t_style);
            return;
        }
			
		case kMCNativeControlPropertyIndicatorInsets:
		{
            MCNativeControlIndicatorInsets t_insets;
            MCAutoStringRef t_string;
            /* UNCHECKED */ ep . copyasstringref(&t_string);
            MCNativeControlIndicatorInsetsParse(ctxt, *t_string, t_insets);
            SetIndicatorInsets(ctxt, t_insets);
            return;
		}
			
		case kMCNativeControlPropertyShowHorizontalIndicator:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetShowHorizontalIndicator(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyShowVerticalIndicator:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetShowVerticalIndicator(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyLockDirection:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetLockDirection(ctxt, t_value);
            return;
        }
 ///////////////////////////////////////////////////////////////////////////////////////////           
            
		default:
			break;
	}
	
    MCiOSInputControl::Set(ctxt, p_property);
}

#ifdef /* MCNativeMultiLineControl::Get */ LEGACY_EXEC
Exec_stat MCiOSMultiLineControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UITextView *t_view;
	t_view = (UITextView *)GetView();
	if (t_view == nil)
		return MCiOSControl::Get(p_property, ep);
	
	MCiOSMultiLineDelegate *t_delegate;
	t_delegate = (MCiOSMultiLineDelegate*)m_delegate;
	
	switch(p_property)
	{	
		case kMCNativeControlPropertyEditable:
			FormatBoolean(ep, [t_view isEditable]);
			return ES_NORMAL;
		case kMCNativeControlPropertySelectedRange:
			FormatRange(ep, [t_view selectedRange]);
			return ES_NORMAL;
		case kMCNativeControlPropertyDataDetectorTypes:
		{
			char *t_type_list = nil;
			if (!datadetectortypes_to_string([t_view dataDetectorTypes], t_type_list))
			{
				MCeerror->add(EE_UNDEFINED, 0, 0);
				return ES_ERROR;
			}
			ep.grabbuffer(t_type_list, MCCStringLength(t_type_list));
			return ES_NORMAL;
		}
		
		case kMCNativeControlPropertyVerticalTextAlign:
			FormatEnum(ep, s_verticaltextalign_enum, [t_delegate getVerticalTextAlign]);
			return ES_NORMAL;
			
			// the contentRect of a multiline text field is read-only & auto-set by the UITextView
			// when its content changes
		default:
			break;
	}
	
	UpdateContentRect();

	Exec_stat t_state;
	t_state = scroller_get_property(t_view, m_content_rect, p_property, ep);
	if (t_state == ES_NOT_HANDLED)
		return MCiOSInputControl::Get(p_property, ep);
	else
		return t_state;
}
#endif /* MCNativeMultiLineControl::Get */

void MCiOSMultiLineControl::Get(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
	switch(p_property)
	{
		case kMCNativeControlPropertyEditable:
		{
            bool t_value;
            GetEditable(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
		case kMCNativeControlPropertySelectedRange:
		{
            MCNativeControlRange t_range;
            GetSelectedRange(ctxt, t_range);
            MCAutoStringRef t_string;
            MCNativeControlRangeFormat(ctxt, t_range, &t_string);
            if (*t_string != nil)
                ep . setvalueref(*t_string);
            return;
        }
		case kMCNativeControlPropertyDataDetectorTypes:
		{
            MCNativeControlInputDataDetectorType t_types;
            GetDataDetectorTypes(ctxt, t_types);
            set_to_ctxt(ctxt, kMCNativeControlInputDataDetectorTypeTypeInfo, t_types);
            return;
		}
            
		case kMCNativeControlPropertyVerticalTextAlign:
		{
            MCNativeControlInputVerticalAlign t_align;
            GetVerticalTextAlign(ctxt, t_align);
            enum_to_ctxt(ctxt, kMCNativeControlInputVerticalAlignTypeInfo, t_align);
            return;
        }
///////////////////////////////////////////////////////////////////////////////////////////
/* SCROLLER PROPS */
            
		case kMCNativeControlPropertyHScroll:
		{
			int32_t t_hscroll;
			GetHScroll(ctxt, t_hscroll);
            ep . setnvalue(t_hscroll);
            return;
		}
		case kMCNativeControlPropertyVScroll:
		{
			int32_t t_vscroll;
			GetVScroll(ctxt, t_vscroll);
            ep . setnvalue(t_vscroll);
            return;
		}
            
		case kMCNativeControlPropertyCanBounce:
		{
            bool t_value;
            GetCanBounce(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
			
		case kMCNativeControlPropertyCanScrollToTop:
		{
            bool t_value;
            GetCanScrollToTop(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
			
		case kMCNativeControlPropertyCanCancelTouches:
		{
            bool t_value;
            GetCanCancelTouches(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
			
		case kMCNativeControlPropertyDelayTouches:
		{
            bool t_value;
            GetDelayTouches(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
			
		case kMCNativeControlPropertyPagingEnabled:
        {
            bool t_value;
            GetPagingEnabled(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
			
		case kMCNativeControlPropertyScrollingEnabled:
		{
            bool t_value;
            GetScrollingEnabled(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
			
		case kMCNativeControlPropertyDecelerationRate:
        {
			MCNativeControlDecelerationRate t_rate;
            GetDecelerationRate(ctxt, t_rate);
            MCAutoStringRef t_string;
            MCNativeControlDecelerationRateFormat(ctxt, t_rate, &t_string);
            if (!ctxt . HasError())
                ep . setvalueref(*t_string);
            return;
        }
            
		case kMCNativeControlPropertyIndicatorStyle:
        {
            MCNativeControlIndicatorStyle t_style;
            GetIndicatorStyle(ctxt, t_style);
            enum_to_ctxt(ctxt, kMCNativeControlIndicatorStyleTypeInfo, (intenum_t)t_style);
            return;
        }
			
		case kMCNativeControlPropertyIndicatorInsets:
		{
            MCNativeControlIndicatorInsets t_insets;
            GetIndicatorInsets(ctxt, t_insets);
            MCAutoStringRef t_string;
            MCNativeControlIndicatorInsetsFormat(ctxt, t_insets, &t_string);
            if (!ctxt . HasError())
                ep . setvalueref(*t_string);
            return;
		}
			
		case kMCNativeControlPropertyShowHorizontalIndicator:
		{
            bool t_value;
            GetShowHorizontalIndicator(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
			
		case kMCNativeControlPropertyShowVerticalIndicator:
		{
            bool t_value;
            GetShowVerticalIndicator(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
			
		case kMCNativeControlPropertyLockDirection:
        {
            bool t_value;
            GetLockDirection(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
        case kMCNativeControlPropertyTracking:
        {
            bool t_value;
            GetTracking(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
		case kMCNativeControlPropertyDragging:
        {
            bool t_value;
            GetDragging(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
		case kMCNativeControlPropertyDecelerating:
        {
            bool t_value;
            GetDecelerating(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
///////////////////////////////////////////////////////////////////////////////////////////
            
        default:
            break;
    }
    MCiOSInputControl::Get(ctxt, p_property);
}

#ifdef /* MCiOSMultiLineControl::Do */ LEGACY_EXEC
Exec_stat MCiOSMultiLineControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
	UITextView *t_view;
	t_view = (UITextView *)GetView();
	if (t_view == nil)
		return MCiOSControl::Do(p_action, p_parameters);
	
	int32_t t_integer1, t_integer2;
	NSRange t_range;
	switch(p_action)
	{
		case kMCNativeControlActionScrollRangeToVisible:
			if (!MCParseParameters(p_parameters, "ii", &t_integer1, &t_integer2))
			{
				MCeerror->add(EE_UNDEFINED, 0, 0);
				return ES_ERROR;
			}
			t_range = NSMakeRange(t_integer1, t_integer2);
			[t_view scrollRangeToVisible: t_range];
			return ES_NORMAL;
			
		default:
			break;
	}
	
	return MCiOSInputControl::Do(p_action, p_parameters);
}
#endif /* MCiOSMultiLineControl::Do */

Exec_stat MCiOSMultiLineControl::Do(MCExecContext& ctxt, MCNativeControlAction p_action, MCParameter *p_parameters)
{
	switch(p_action)
	{
		case kMCNativeControlActionScrollRangeToVisible:
        {
            int32_t t_integer1, t_integer2;
            
            if (!MCParseParameters(p_parameters, "ii", &t_integer1, &t_integer2))
            {
                MCeerror->add(EE_UNDEFINED, 0, 0);
                return ES_ERROR;
            }
            
            if (ExecScrollRangeToVisible(ctxt, t_integer1, t_integer2) != ES_NORMAL)
                break;
            
            return ES_NORMAL;
        }            
			
		default:
			break;
	}
	
	return MCiOSInputControl::Do(ctxt, p_action, p_parameters);
}

Exec_stat MCiOSMultiLineControl::ExecScrollRangeToVisible(MCExecContext& ctxt, int32_t p_integer1, int32_t p_integer2)
{
	UITextView *t_view;
	t_view = (UITextView *)GetView();
    
    NSRange t_range;
	if (t_view == nil)
        return ES_NOT_HANDLED;
    
    t_range = NSMakeRange(p_integer1, p_integer2);
    [t_view scrollRangeToVisible: t_range];
    return ES_NORMAL;
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
		t_target->message_with_args(MCM_scroller_end_drag, p_decelerate ? MCtruestring : MCfalsestring);
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
	
	m_delegate = [[MCiOSMultiLineDelegate alloc] initWithInstance: this view: t_view];
	[t_view setDelegate: m_delegate];
	
	return t_view;
}

void MCiOSMultiLineControl::DeleteView(UIView *p_view)
{
	[p_view setDelegate: nil];
	[p_view release];
	
	[m_delegate release];
	m_delegate = nil;
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

@implementation MCiOSInputDelegate

- (id)initWithInstance:(MCiOSInputControl*)instance view: (UIView *)view
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	m_didchange_pending = false;
	m_return_pressed = false;
	
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

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
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

@implementation MCiOSMultiLineDelegate

- (id)initWithInstance:(MCiOSInputControl *)instance view:(UIView *)view
{
	self = [super initWithInstance:instance view:view];
	if (self == nil)
		return nil;
	
	m_verticaltextalign = kMCNativeControlInputVerticalAlignTop;

	[view addObserver:self forKeyPath:@"contentSize" options:NSKeyValueObservingOptionNew context:nil];
	
	return self;
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
