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

#include "core.h"
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

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import <MediaPlayer/MPMoviePlayerController.h>

#include "mbliphonecontrol.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
UIView *MCIPhoneGetView(void);

////////////////////////////////////////////////////////////////////////////////

class MCNativeInputControl;

// Note that we use the notifications, rather than delegate methods.
@interface MCNativeInputDelegate : NSObject <UITextFieldDelegate, UITextViewDelegate>
{
	MCNativeInputControl *m_instance;
	bool m_didchange_pending;
	bool m_return_pressed;
}

- (id)initWithInstance:(MCNativeInputControl*)instance view:(UIView *)view;
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


@interface MCNativeMultiLineDelegate : MCNativeInputDelegate <UIScrollViewDelegate>
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
class MCNativeInputControl: public MCiOSControl
{
public:
	MCNativeInputControl(void);
	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep);	
	virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
	
	void HandleNotifyEvent(MCNameRef p_notification);
	
	MCNativeInputDelegate *GetDelegate(void);
	
protected:
	virtual ~MCNativeInputControl(void);
	
	MCNativeInputDelegate *m_delegate;
};

// single line input control
class MCNativeInputFieldControl: public MCNativeInputControl
{
public:
	virtual MCNativeControlType GetType(void);
	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep);	
	
protected:
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
};

// multiline input control
class MCNativeMultiLineControl: public MCNativeInputControl
{
public:
	MCNativeMultiLineControl(void)
	{
		m_post_scroll_event = true;
		m_content_rect.x = 0;
		m_content_rect.y = 0;
		m_content_rect.width = 0;
		m_content_rect.height = 0;
	}
	
	virtual MCNativeControlType GetType(void);
	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
	
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

MCNativeInputControl::MCNativeInputControl(void)
{
	m_delegate = nil;
}

MCNativeInputControl::~MCNativeInputControl(void)
{
}

Exec_stat MCNativeInputControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
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

Exec_stat MCNativeInputControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
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

Exec_stat MCNativeInputControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
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

////////////////////////////////////////////////////////////////////////////////

MCNativeInputDelegate *MCNativeInputControl::GetDelegate(void)
{
	return m_delegate;
}

void MCNativeInputControl::HandleNotifyEvent(MCNameRef p_notification)
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

MCNativeControlType MCNativeInputFieldControl::GetType(void)
{
	return kMCNativeControlTypeInput;
}

Exec_stat MCNativeInputFieldControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
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
	
	return MCNativeInputControl::Set(p_property, ep);
}

Exec_stat MCNativeInputFieldControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
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
	
	return MCNativeInputControl::Get(p_property, ep);
}

UIView *MCNativeInputFieldControl::CreateView(void)
{
	UITextField *t_view;
	t_view = [[UITextField alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return nil;
	
	[t_view setHidden: YES];
	
	m_delegate = [[MCNativeInputDelegate alloc] initWithInstance: this view: t_view];
	[t_view setDelegate: m_delegate];
	
	return t_view;
}

void MCNativeInputFieldControl::DeleteView(UIView *p_view)
{
	[p_view setDelegate: nil];
	
	// MW-2013-05-22: [[ Bug 10880 ]] Make sure we release the delegate here as
	//   it might need to refer to the view to deregister itself.
	[m_delegate release];
	m_delegate = nil;
	
	[p_view release];
}

////////////////////////////////////////////////////////////////////////////////

MCNativeControlType MCNativeMultiLineControl::GetType(void)
{
	return kMCNativeControlTypeInput;
}

bool datadetectortypes_from_string(const char *p_list, UIDataDetectorTypes &r_types);
bool datadetectortypes_to_string(UIDataDetectorTypes p_types, char *&r_list);

Exec_stat scroller_set_property(UIScrollView *p_view, MCRectangle32 &x_content_rect, MCNativeControlProperty p_property, MCExecPoint&ep);
Exec_stat scroller_get_property(UIScrollView *p_view, const MCRectangle32 &p_content_rect, MCNativeControlProperty p_property, MCExecPoint &ep);

Exec_stat MCNativeMultiLineControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UITextView *t_view;
	t_view = (UITextView *)GetView();
	if (t_view == nil)
		return MCiOSControl::Set(p_property, ep);
	
	MCNativeMultiLineDelegate *t_delegate;
	t_delegate = (MCNativeMultiLineDelegate*)m_delegate;
	
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
		return MCNativeInputControl::Set(p_property, ep);
	else
		return t_state;
}

Exec_stat MCNativeMultiLineControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UITextView *t_view;
	t_view = (UITextView *)GetView();
	if (t_view == nil)
		return MCiOSControl::Get(p_property, ep);
	
	MCNativeMultiLineDelegate *t_delegate;
	t_delegate = (MCNativeMultiLineDelegate*)m_delegate;
	
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
		return MCNativeInputControl::Get(p_property, ep);
	else
		return t_state;
}

Exec_stat MCNativeMultiLineControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
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
	
	return MCNativeInputControl::Do(p_action, p_parameters);
}

void MCNativeMultiLineControl::UpdateContentRect()
{
	UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
	
	float t_scale;
	t_scale = MCIPhoneGetNativeControlScale();
	
	CGSize t_content_size;
	t_content_size = [t_view contentSize];
	
	MCU_set_rect(m_content_rect, 0, 0, t_content_size.width * t_scale, t_content_size.height * t_scale);
}

void MCNativeMultiLineControl::HandleEvent(MCNameRef p_message)
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

void MCNativeMultiLineControl::HandleEndDragEvent(bool p_decelerate)
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

void MCNativeMultiLineControl::HandleScrollEvent(void)
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

UIView *MCNativeMultiLineControl::CreateView(void)
{
	UITextView *t_view;
	t_view = [[UITextView alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return nil;
	
	[t_view setHidden: YES];
	
	m_delegate = [[MCNativeMultiLineDelegate alloc] initWithInstance: this view: t_view];
	[t_view setDelegate: m_delegate];
	
	return t_view;
}

void MCNativeMultiLineControl::DeleteView(UIView *p_view)
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
	MCNativeInputNotifyEvent(MCNativeInputControl *p_target, MCNameRef p_notification)
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
	MCNativeInputControl *m_target;
	MCNameRef m_notification;
};

class MCNativeInputDidChangeEvent: public MCCustomEvent
{
public:
	// Note that we require p_notification to be a C-string constant as we don't
	// copy it.
	MCNativeInputDidChangeEvent(MCNativeInputControl *p_target)
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
	MCNativeInputControl *m_target;
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

@implementation MCNativeInputDelegate

- (id)initWithInstance:(MCNativeInputControl*)instance view: (UIView *)view
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



class MCNativeMultiLineEvent : public MCCustomEvent
{
public:
	MCNativeMultiLineEvent(MCNativeMultiLineControl *p_target, MCNameRef p_message)
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
	MCNativeMultiLineControl *m_target;
	MCNameRef m_message;
};

class MCNativeMultiLineEndDragEvent : public MCCustomEvent
{
public:
	MCNativeMultiLineEndDragEvent(MCNativeMultiLineControl *p_target, bool p_decelerate)
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
	MCNativeMultiLineControl *m_target;
	bool m_decelerate;
};


class MCNativeMultiLineScrollEvent : public MCCustomEvent
{
public:
	MCNativeMultiLineScrollEvent(MCNativeMultiLineControl *p_target)
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
	MCNativeMultiLineControl *m_target;
};

@implementation MCNativeMultiLineDelegate

- (id)initWithInstance:(MCNativeInputControl *)instance view:(UIView *)view
{
	self = [super initWithInstance:instance view:view];
	if (self == nil)
		return nil;
	
	m_verticaltextalign = kMCTextVerticalAlignTop;

	[view addObserver:self forKeyPath:@"contentSize" options:NSKeyValueObservingOptionNew context:nil];
	
	return self;
}

// MW-2013-05-22: [[ Bug 10880 ]] Make sure we have a 'dealloc' method so observers
//   can be removed that reference this object.
- (void)dealloc
{
	[m_instance -> GetView() removeObserver: self forKeyPath:@"contentSize"];
	[super dealloc];
}
   

- (void)scrollViewWillBeginDragging: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeMultiLineEvent((MCNativeMultiLineControl*)m_instance, MCM_scroller_begin_drag);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidEndDragging: (UIScrollView*)scrollView willDecelerate:(BOOL)decelerate
{
	MCCustomEvent *t_event;
	t_event = new MCNativeMultiLineEndDragEvent((MCNativeMultiLineControl*)m_instance, decelerate);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidScroll: (UIScrollView*)scrollView
{
	MCNativeMultiLineControl *t_instance = (MCNativeMultiLineControl*)m_instance;
	if (t_instance != nil && t_instance->m_post_scroll_event)
	{
		t_instance->m_post_scroll_event = false;
		MCCustomEvent *t_event;
		t_event = new MCNativeMultiLineScrollEvent(t_instance);
		MCEventQueuePostCustom(t_event);
	}
}

- (void)scrollViewDidScrollToTop: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeMultiLineEvent((MCNativeMultiLineControl*)m_instance, MCM_scroller_scroll_to_top);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewWillBeginDecelerating:(UIScrollView *)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeMultiLineEvent((MCNativeMultiLineControl*)m_instance, MCM_scroller_begin_decelerate);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidEndDecelerating: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeMultiLineEvent((MCNativeMultiLineControl*)m_instance, MCM_scroller_end_decelerate);
	MCEventQueuePostCustom(t_event);
}

- (void)adjustVerticalAlignment: (UITextView *)textView
{
	switch (m_verticaltextalign)
	{
		case kMCTextVerticalAlignTop:
			textView.contentOffset = CGPointMake(0, 0);
			break;

		case kMCTextVerticalAlignCenter:
		{
			CGFloat topCorrect = ([textView bounds].size.height - [textView contentSize].height * [textView zoomScale])/2.0;
			topCorrect = ( topCorrect < 0.0 ? 0.0 : topCorrect );
			textView.contentOffset = CGPointMake(0, -topCorrect);
			break;
		}
			
		case kMCTextVerticalAlignBottom:
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
	r_control = new MCNativeInputFieldControl;
	return true;
}

bool MCNativeMultiLineInputControlCreate(MCNativeControl *&r_control)
{
	r_control = new MCNativeMultiLineControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
