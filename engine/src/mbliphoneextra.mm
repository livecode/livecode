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


#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "chunk.h"
#include "scriptpt.h"
#include "eventqueue.h"
#include "redraw.h"
#include "mbldc.h"
#include "text.h"
#include "card.h"
#include "osspec.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIGraphics.h>
#import <UIKit/UIImage.h>
#import <UIKit/UIImagePickerController.h>
#import <UIKit/UIAccelerometer.h>
// HC-2011-10-12 [[ Media Picker ]] Included relevant library.
#import <MediaPlayer/MPMediaPickerController.h>
#import <MessageUI/MessageUI.h>
#import <AudioToolbox/AudioServices.h>

#ifdef __IPHONE_5_0
#include <MediaPlayer/MPNowPlayingInfoCenter.h>
#endif

// HC-2011-10-12 [[ Media Picker ]] Included relevant library.
#include "mbliphonecontrol.h"
#include "mbliphone.h"
#include "mbliphoneview.h"

#include <sys/xattr.h>

#include "mblstore.h"
#include "mblsyntax.h"

#define FILEATTR_DONOTBACKUP "com.apple.MobileBackup"
#include <dlfcn.h>
#import <objc/message.h>

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...)
{
    MCExecContext ctxt(nil, nil, nil);
	
	bool t_success;
	t_success = true;
	
	bool t_now_optional;
	t_now_optional = false;
	
	va_list t_args;
	va_start(t_args, p_format);
	
	while(*p_format != '\0' && t_success)
	{
		if (*p_format == '|')
		{
			t_now_optional = true;
			p_format++;
			continue;
		}
		
        MCAutoValueRef t_value;
		if (p_parameters != nil)
        {
            // AL-2014-05-28: [[ Bug 12477 ]] Use eval_argument here otherwise variable references do not get resolved
			t_success = p_parameters -> eval_argument(ctxt, &t_value);
        }
		else if (t_now_optional)
			break;
		else
			t_success = false;
		
		switch(*p_format)
		{
			case 'b':
				if (t_success)
                {
                    MCAutoStringRef t_string;
                    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
					*(va_arg(t_args, bool *)) = MCStringIsEqualTo(*t_string, kMCTrueString, kMCCompareCaseless);
                }
				break;
				
			case 's':
				if (t_success)
                {
                    MCAutoStringRef t_string;
                    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
                    char *temp;
                    /* UNCHECKED */ MCStringConvertToCString(*t_string, temp);
					*(va_arg(t_args, char **)) = temp;
                }
				else
					*(va_arg(t_args, char **)) = nil;
				break;
			
			case 'd':
				if (t_success)
                {
                    MCAutoStringRef t_string;
                    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
                    char *temp;
                    /* UNCHECKED */ MCStringConvertToCString(*t_string, temp);
					(va_arg(t_args, MCString *)) -> set(temp, strlen(temp));
                }
				else
					(va_arg(t_args, MCString *)) -> set(nil, 0);
				break;
			
            case 'x':
            {
				if (t_success)
                {
                    /* UNCHECKED */ ctxt . ConvertToString(*t_value, *(va_arg(t_args, MCStringRef *)));
                }
				else
					t_success = false;
				break;
            }
                
			case 'a':
            {
				if (t_success)
                    /* UNCHECKED */ ctxt . ConvertToArray(*t_value, *(va_arg(t_args, MCArrayRef *)));
				else
					t_success = false;
				break;
            }
				
			case 'r':
			{
				int2 i1, i2, i3, i4;
				if (t_success)
                {
                    MCAutoStringRef t_string;
                    ctxt . ConvertToString(*t_value, &t_string);
					t_success = MCU_stoi2x4(*t_string, i1, i2, i3, i4) == True;
                }
				if (t_success)
					MCU_set_rect(*(va_arg(t_args, MCRectangle *)), i1, i2, i3 - i1, i4 - i2);
			}
				break;
				
			case 'i':
				if (t_success)
				{
                    integer_t t_int;
					if (ctxt . ConvertToInteger(*t_value, t_int))
						*(va_arg(t_args, integer_t *)) = t_int;
					else
						t_success = false;
				}
				break;
				
			case 'u':
				if (t_success)
				{
                    uinteger_t t_uint;
					if (ctxt . ConvertToUnsignedInteger(*t_value, t_uint))
						*(va_arg(t_args, uinteger_t *)) = t_uint;
					else
						t_success = false;
				}
				break;
		};
		
		p_format += 1;
		
		if (p_parameters != nil)
			p_parameters = p_parameters -> getnext();
	}
	
	va_end(t_args);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCIPhoneUseDeviceResolution(bool p_use_dev_res, bool p_use_control_device_res);
float MCIPhoneGetDeviceScale(void);
UIViewController *MCIPhoneGetViewController(void);
UIView *MCIPhoneGetView(void);
UITextView *MCIPhoneGetTextView(void);
UIWindow *MCIPhoneGetWindow(void);

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCExportImageToAlbumDelegate : NSObject
{
	bool m_finished;
	bool m_successful;
}

- (id)init;
- (bool)isFinished;
- (void)image: (UIImage *)image didFinishSavingWithError: (NSError *)error contextInfo: (void *)contextInfo;
@end

@implementation com_runrev_livecode_MCExportImageToAlbumDelegate

- (id)init
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_finished = false;
	m_successful = false;
	
	return self;
}

- (bool)isFinished
{
	return m_finished;
}

- (bool)isSuccessful
{
	return m_successful;
}

- (void)image: (UIImage *)image didFinishSavingWithError: (NSError *)error contextInfo: (void *)contextInfo
{
	m_finished = true;
	m_successful = error == nil;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

@end

struct export_image_t
{
    com_runrev_livecode_MCExportImageToAlbumDelegate *delegate;
    MCDataRef raw_data;
    // PM-2014-12-12: [[ Bug 13860 ]] Added support for exporting referenced images to album
    bool is_raw_data;
};

static void export_image(void *p_context)
{
	export_image_t *ctxt;
	ctxt = (export_image_t *)p_context;
	
    NSData *t_data;
    // PM-2014-12-12: [[ Bug 13860 ]] Allow exporting referenced images to album
    if (ctxt -> is_raw_data)
        t_data = [[NSData alloc] initWithBytes: (void *)MCDataGetBytePtr(ctxt -> raw_data) length: MCDataGetLength(ctxt -> raw_data)];
    else
        t_data = nil;

	UIImage *t_img;
    // For referenced images, init with filename rather than with bytes
    if (t_data != nil)
        t_img = [[UIImage alloc] initWithData: t_data];
    else
        t_img = [[UIImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String: (char*)MCDataGetBytePtr(ctxt -> raw_data)]];

	UIImageWriteToSavedPhotosAlbum(t_img, ctxt -> delegate, @selector(image:didFinishSavingWithError:contextInfo:), nil);
	
	[t_img release];
    
    if (t_data !=nil)
        [t_data release];
}

// SN-2014-12-18: [[ Bug 13860 ]] Parameter added in case it's a filename, not raw data, in the DataRef
bool MCSystemExportImageToAlbum(MCStringRef& r_save_result, MCDataRef p_raw_data, MCStringRef p_file_name, MCStringRef p_file_extension, bool p_is_raw_data)
{
	export_image_t ctxt;
    ctxt . is_raw_data = p_is_raw_data;
    ctxt . raw_data = p_raw_data;
	ctxt . delegate = [[com_runrev_livecode_MCExportImageToAlbumDelegate alloc] init];

	MCIPhoneRunOnMainFiber(export_image, &ctxt);

	while(![ctxt . delegate isFinished])
		MCscreen -> wait(60.0, False, True);

	if ([ctxt . delegate isSuccessful])
        r_save_result = MCValueRetain(kMCEmptyString);
    else
		MCStringCreateWithCString("export failed", r_save_result);

	[ctxt . delegate release];

	return true;
}

////////////////////////////////////////////////////////////////////////////////

extern bool coretext_get_font_names(MCListRef &r_names);
extern bool core_text_get_font_styles(MCStringRef p_name, uint32_t p_size, MCListRef &r_styles);

void MCSystemListFontFamilies(MCListRef &r_names)
{
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text routines.
    coretext_get_font_names(r_names);
}

void MCSystemListFontsForFamily(MCStringRef p_family, uint32_t p_size, MCListRef &r_styles)
{
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text routines.
    core_text_get_font_styles(p_family, p_size, r_styles);
}

////////////////////////////////////////////////////////////////////////////////

static struct { uint1 charset; CFStringEncoding encoding; } s_encoding_map[] =
{
	{ LCH_ENGLISH, kCFStringEncodingMacRoman },
	{ LCH_ROMAN, kCFStringEncodingMacRoman },
	{ LCH_JAPANESE, kCFStringEncodingMacJapanese },
	{ LCH_CHINESE, kCFStringEncodingMacChineseTrad },
	{ LCH_RUSSIAN, kCFStringEncodingMacCyrillic },
	{ LCH_TURKISH, kCFStringEncodingMacCyrillic },
	{ LCH_BULGARIAN, kCFStringEncodingMacCyrillic },
	{ LCH_UKRAINIAN, kCFStringEncodingMacCyrillic },
	{ LCH_ARABIC, kCFStringEncodingMacArabic },
	{ LCH_HEBREW, kCFStringEncodingMacHebrew },
	{ LCH_GREEK, kCFStringEncodingMacGreek },
	{ LCH_KOREAN, kCFStringEncodingMacKorean },
	{ LCH_POLISH, kCFStringEncodingMacCentralEurRoman },
	{ LCH_VIETNAMESE, kCFStringEncodingMacVietnamese },
	{ LCH_LITHUANIAN, kCFStringEncodingMacCentralEurRoman },
	{ LCH_THAI, kCFStringEncodingMacThai },
	{ LCH_SIMPLE_CHINESE, kCFStringEncodingMacChineseSimp },
	{ LCH_UNICODE, kCFStringEncodingUTF16LE }
};

static CFStringEncoding lookup_encoding(uint1 p_charset)
{
	for(uint32_t i = 0; i < sizeof(s_encoding_map) / sizeof(s_encoding_map[0]); i++)
		if (s_encoding_map[i] . charset == p_charset)
			return s_encoding_map[i] . encoding;
	return kCFStringEncodingMacRoman;
}

uint32_t MCIPhoneSystem::TextConvert(const void *p_string, uint32_t p_string_length, void *p_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset)
{
	CFStringEncoding t_from, t_to;
	t_from = lookup_encoding(p_from_charset);
	t_to = lookup_encoding(p_to_charset);
	
	CFStringRef t_string;
	t_string = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)p_string, p_string_length, t_from, FALSE, kCFAllocatorNull);
	
	CFIndex t_used;
	CFStringGetBytes(t_string, CFRangeMake(0, CFStringGetLength(t_string)), t_to, '?', FALSE, (UInt8 *)p_buffer, p_buffer_length, &t_used);
	
	CFRelease(t_string);
	
	return t_used;
}

static struct { uint32_t encoding; CFStringEncoding cfencoding; } s_text_encoding_map[] =
{
	{kMCTextEncodingSymbol,},
	{kMCTextEncodingMacRoman, kCFStringEncodingMacRoman},
	{kMCTextEncodingWindows1252, kCFStringEncodingWindowsLatin1},
	{kMCTextEncodingWindowsNative + 437, kCFStringEncodingDOSLatinUS},
	{kMCTextEncodingWindowsNative + 850, kCFStringEncodingDOSLatin1},
	{kMCTextEncodingWindowsNative + 932, kCFStringEncodingDOSJapanese},
	{kMCTextEncodingWindowsNative + 949, kCFStringEncodingDOSKorean},
	{kMCTextEncodingWindowsNative + 1361, kCFStringEncodingWindowsKoreanJohab},
	{kMCTextEncodingWindowsNative + 936, kCFStringEncodingDOSChineseSimplif},
	{kMCTextEncodingWindowsNative + 950, kCFStringEncodingDOSChineseTrad},
	{kMCTextEncodingWindowsNative + 1253, kCFStringEncodingWindowsGreek},
	{kMCTextEncodingWindowsNative + 1254, kCFStringEncodingWindowsLatin5},
	{kMCTextEncodingWindowsNative + 1258, kCFStringEncodingWindowsVietnamese},
	{kMCTextEncodingWindowsNative + 1255, kCFStringEncodingWindowsHebrew},
	{kMCTextEncodingWindowsNative + 1256, kCFStringEncodingWindowsArabic},
	{kMCTextEncodingWindowsNative + 1257, kCFStringEncodingWindowsBalticRim},
	{kMCTextEncodingWindowsNative + 1251, kCFStringEncodingWindowsCyrillic},
	{kMCTextEncodingWindowsNative + 874, kCFStringEncodingDOSThai},
	{kMCTextEncodingWindowsNative + 1250, kCFStringEncodingWindowsLatin2},
	{65001, kCFStringEncodingUTF8},
};

static CFStringEncoding lookup_text_encoding(uint32_t encoding)
{
	for(uint32_t i = 0; i < sizeof(s_text_encoding_map) / sizeof(s_text_encoding_map[0]); i++)
		if (s_text_encoding_map[i] . encoding == encoding)
			return s_text_encoding_map[i] . cfencoding;
	return kCFStringEncodingMacRoman;
}

// MW-2012-02-01: [[ Bug 9974 ]] This method is needed to ensure RTF import works.
bool MCIPhoneSystem::TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4& p_output_length, uint4& r_used)
{
	if (p_input_length == 0)
	{
		r_used = 0;
		return true;
	}
	
	CFStringEncoding t_encoding;
	t_encoding = lookup_text_encoding(p_input_encoding);
	
	CFStringRef t_string;
	t_string = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)p_input, p_input_length, t_encoding, FALSE, kCFAllocatorNull);
	
	CFIndex t_length;
	t_length = CFStringGetLength(t_string);
	
	if (t_length * 2 > p_output_length)
	{
		r_used = t_length * 2;
		return false;
	}
	
	CFStringGetCharacters(t_string, CFRangeMake(0, t_length), (UniChar *)p_output);
	r_used = t_length * 2;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static UIStatusBarStyle MCMiscUIStatusBarStyleFromMCExecEnum(MCMiscStatusBarStyle p_status_bar_style)
{
    switch(p_status_bar_style)
    {
        // PM-2015-02-17: [[ Bug 14482 ]] "solid" status bar style means
        //  opaque and automatically shift down the app view by 20 pixels
        case kMCMiscStatusBarStyleOpaque:
        case kMCMiscStatusBarStyleSolid:
            return UIStatusBarStyleBlackOpaque;
        case kMCMiscStatusBarStyleTranslucent:
            return UIStatusBarStyleBlackTranslucent;
        default:
            return UIStatusBarStyleDefault;
    }
}

bool MCSystemSetStatusBarStyle(intenum_t p_status_bar_style)
{	
    UIStatusBarStyle t_style;
    t_style = MCMiscUIStatusBarStyleFromMCExecEnum((MCMiscStatusBarStyle)p_status_bar_style);
    [MCIPhoneGetApplication() switchToStatusBarStyle: t_style];

    // PM-2015-02-17: [[ Bug 14482 ]] "solid" status bar style means opaque and automatically shift down the app view by 20 pixels
    if (p_status_bar_style == kMCMiscStatusBarStyleSolid)
        [MCIPhoneGetApplication() setStatusBarSolid:YES];
    else
        [MCIPhoneGetApplication() setStatusBarSolid:NO];
    
    return true;
}

bool MCSystemShowStatusBar()
{
    [MCIPhoneGetApplication() switchToStatusBarVisibility: YES];
    
    return true;
}

bool MCSystemHideStatusBar()
{
    [MCIPhoneGetApplication() switchToStatusBarVisibility: NO];
    
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

/*

// Important: We switch right and left here. Internally we use 'device orientation' but to script
// we use 'interface orientation'. Interface orientation ties landscape to the position of the home
// button as you look at the device.
// MM-2011-09-19: [[ BZ 9737 ]] Changed array end from empty to nil preventing iterations running out of bounds.
static const char *s_orientation_names[] =
{
	"unknown", "portrait", "portrait upside down", "landscape right", "landscape left", "face up", "face down", nil
};
*/

extern bool MCIPhonePickMedia(bool p_allow_multiple_items, MPMediaType p_media_types, NSString*& r_media_returned);

/////////////////////////////////////////////////////////////////////////////////////////////////////

bool MCSystemGetLaunchData(MCArrayRef &r_data)
{
	// Not implemented on iOS
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

// We do not need this in iOS, as beep is already implemented and handled.
bool MCSystemBeep (int32_t p_number_of_beeps)
{
    return true;
}

bool MCSystemVibrate (int32_t p_number_of_vibrates)
{
    for (int32_t i = 0; i < p_number_of_vibrates; i++)
    {
		// MW-2012-08-06: [[ Fibers ]] Invoke the system call on the main fiber.
		MCIPhoneRunBlockOnMainFiber(^(void) {
			AudioServicesPlayAlertSound(kSystemSoundID_Vibrate); // Vibrates and beeps if no vibrate is supported
		});
		MCscreen->wait(BEEP_INTERVAL, False, False);
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

bool MCSystemGetDeviceResolution(MCStringRef& p_resolution)
{
	UIScreenMode *t_mode;
	t_mode = [[UIScreen mainScreen] currentMode];

    // MM-2012-11-22: [[ Bug 10502 ]] Make sure the resolution is reported as width height in relation to portrait.
	char t_size[U4L + 1 + U4L + 1];
    if ([t_mode size] . width < [t_mode size] .height)
        sprintf(t_size, "%u,%u", (uint32_t)[t_mode size] . width, (uint32_t)[t_mode size] . height);
    else
        sprintf(t_size, "%u,%u", (uint32_t)[t_mode size] . height, (uint32_t)[t_mode size] . width);
    
	MCStringCreateWithCString(t_size, p_resolution);

	return ES_NORMAL;
}

bool MCSystemSetDeviceUseResolution(bool p_use_device_res, bool p_use_control_device_res)
{
    MCIPhoneUseDeviceResolution(p_use_control_device_res, p_use_control_device_res);
    
    return true;
}

bool MCSystemGetDeviceScale(real64_t& r_scale)
{
    r_scale = MCIPhoneGetDeviceScale();
    
    return true;
}

bool MCSystemGetPixelDensity(real64_t& r_density)
{
    // AL-2014-11-17: [[ Bug 14031 ]] pixelDensity on iOS should return device scale
    r_density = MCIPhoneGetDeviceScale();
    
    return true;
}

// SN-2014-10-15: [[ Merge-6.7.0-rc-3 ]]
////////////////////////////////////////////////////////////////////////////////

UIKeyboardType MCInterfaceGetUIKeyboardTypeFromExecEnum(MCInterfaceKeyboardType p_type)
{
    switch(p_type)
    {
        case kMCInterfaceKeyboardTypeAlphabet:
            return UIKeyboardTypeAlphabet;
        case kMCInterfaceKeyboardTypeNumeric:
            return UIKeyboardTypeNumbersAndPunctuation;
        case kMCInterfaceKeyboardTypeUrl:
            return UIKeyboardTypeURL;
        case kMCInterfaceKeyboardTypeNumber:
            return UIKeyboardTypeNumberPad;
        case kMCInterfaceKeyboardTypePhone:
            return UIKeyboardTypePhonePad;
        case kMCInterfaceKeyboardTypeContact:
            return UIKeyboardTypeNamePhonePad;
        case kMCInterfaceKeyboardTypeEmail:
            return UIKeyboardTypeEmailAddress;
#ifdef __IPHONE_4_1
        case kMCInterfaceKeyboardTypeDecimal:
            return UIKeyboardTypeDecimalPad;
#endif // __IPHONE_4_1
        default:
            return UIKeyboardTypeDefault;
    }
}

bool MCSystemSetKeyboardType(intenum_t p_type)
{
    MCIPhoneSetKeyboardType(MCInterfaceGetUIKeyboardTypeFromExecEnum((MCInterfaceKeyboardType)p_type));
    
    return  true;
}

UIReturnKeyType MCInterfaceGetUIReturnKeyTypeFromExecEnum(MCInterfaceReturnKeyType p_type)
{
    switch(p_type)
    {
        case kMCInterfaceReturnKeyTypeGo:
            return UIReturnKeyGo;
        case kMCInterfaceReturnKeyTypeGoogle:
            return UIReturnKeyGoogle;
        case kMCInterfaceReturnKeyTypeJoin:
            return UIReturnKeyJoin;
        case kMCInterfaceReturnKeyTypeNext:
            return UIReturnKeyNext;
        case kMCInterfaceReturnKeyTypeSearch:
            return UIReturnKeySearch;
        case kMCInterfaceReturnKeyTypeSend:
            return UIReturnKeySend;
        case kMCInterfaceReturnKeyTypeRoute:
            return UIReturnKeyRoute;
        case kMCInterfaceReturnKeyTypeYahoo:
            return UIReturnKeyYahoo;
        case kMCInterfaceReturnKeyTypeDone:
            return UIReturnKeyDone;
        case kMCInterfaceReturnKeyTypeEmergencyCall:
            return UIReturnKeyEmergencyCall;
        default:
            return UIReturnKeyDefault;
    }
}

bool MCSystemSetKeyboardReturnKey(intenum_t p_type)
{
    MCIPhoneSetReturnKeyType(MCInterfaceGetUIReturnKeyTypeFromExecEnum((MCInterfaceReturnKeyType)p_type));
                             
    return true;
}

bool MCSystemSetKeyboardDisplay(intenum_t p_type)
{
    MCIPhoneSetKeyboardDisplay((MCIPhoneKeyboardDisplayMode)p_type);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]]
bool MCSystemGetIsVoiceOverRunning(bool &r_is_vo_running)
{
    r_is_vo_running = UIAccessibilityIsVoiceOverRunning();
    return true;
}

bool MCSystemGetPreferredLanguages(MCStringRef& r_preferred_languages)
{
	bool t_success;
	t_success = true;

	NSArray *t_preferred_langs = nil;
	t_preferred_langs = [NSLocale preferredLanguages];
	t_success = t_preferred_langs != nil && [t_preferred_langs count] != 0;

    MCAutoListRef t_languages;
    t_success |= MCListCreateMutable('\n', &t_languages);
    
	if (t_success)
	{
        bool t_first = true;
		for (NSString *t_lang in t_preferred_langs)
		{
            MCAutoStringRef t_language;
            if (t_success && MCStringCreateWithCFStringRef((CFStringRef)t_lang, &t_language))
                t_success = MCListAppend(*t_languages, *t_language);
        }
	}
    
    if (t_success)
        return MCListCopyAsString(*t_languages, r_preferred_languages);
    
    return false;
}

bool MCSystemGetCurrentLocale(MCStringRef& r_current_locale)
{
	NSString *t_current_locale_id = nil;
	t_current_locale_id = [[NSLocale currentLocale] objectForKey: NSLocaleIdentifier];

	/* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)t_current_locale_id, r_current_locale);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemGetSystemIdentifier(MCStringRef& r_identifier)
{
    // MM-2013-05-21: [[ Bug 10895 ]] The method uniqueIdentifier of UIDevice is now deprecated (as of May 2013).
    //  Calling the method dynamically prevents apps from being rejected by the app store
    //  but preserves functionality for testing and backwards compatibility.
    NSString *t_identifier;
    t_identifier = objc_msgSend([UIDevice currentDevice], sel_getUid("uniqueIdentifier"));
	
    return MCStringCreateWithCFStringRef((CFStringRef)t_identifier, r_identifier);
}

bool MCSystemGetIdentifierForVendor(MCStringRef& r_identifier)
{
// MM-2013-05-21: [[ Bug 10895 ]] Added iphoneIdentifierForVendor as an initial replacement for iphoneSystemIdentifier.
//  identifierForVendor was only added to UIDevice in iOS 6.1 so make sure we weakly link.
    
    if ([UIDevice instancesRespondToSelector:@selector(identifierForVendor)])
    {
        NSString *t_identifier;
        t_identifier = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
        return MCStringCreateWithCFStringRef((CFStringRef)t_identifier, r_identifier);
    }

    r_identifier = MCValueRetain(kMCEmptyString);
    return true;
}


bool MCSystemGetApplicationIdentifier(MCStringRef& r_identifier)
{
	// Get the plist
	NSDictionary *t_plist;
	t_plist = [[NSBundle mainBundle] infoDictionary];
	
	NSString *t_identifier;
	t_identifier = [t_plist objectForKey: @"CFBundleIdentifier"];
	
	return MCStringCreateWithCFStringRef((CFStringRef)t_identifier, r_identifier);
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemBuildInfo(MCStringRef p_key, MCStringRef& r_value)
{
    // not implemented on Iphone
    return false;
}

////////////////////////////////////////////////////////////////////////////////

extern bool MCFileSetDataProtection(MCStringRef p_filename, NSString *p_protection);
extern bool MCFileGetDataProtection(MCStringRef p_filename, NSString *&r_protection);
extern bool MCDataProtectionFromString(MCStringRef p_string, NSString *&r_protection);
extern bool MCDataProtectionToString(NSString *p_protection, MCStringRef &r_string);

bool MCSystemFileSetDoNotBackup(MCStringRef p_path, bool p_no_backup)
{
   bool t_success = true;
    MCAutoStringRefAsUTF8String t_utf8_path;
    t_utf8_path . Lock(p_path);
    if (p_no_backup)
    {
        uint8_t t_val = 1;
        t_success = 0 == setxattr(*t_utf8_path, FILEATTR_DONOTBACKUP, &t_val, sizeof(t_val), 0, 0);
    }
    else
    {
        t_success = 0 == removexattr(*t_utf8_path, FILEATTR_DONOTBACKUP, 0);
    }
    return t_success;
}

bool MCSystemFileGetDoNotBackup(MCStringRef p_path, bool& r_no_backup)
{
    MCAutoStringRefAsUTF8String t_utf8_path;
    t_utf8_path . Lock(p_path);
    uint8_t t_val = 0;
    if (-1 == getxattr(*t_utf8_path, FILEATTR_DONOTBACKUP, &t_val, sizeof(t_val), 0, 0))
        return false;
    return t_val != 0;
}

bool MCSystemFileSetDataProtection(MCStringRef p_path, MCStringRef p_protection_string, MCStringRef& r_status)
{
	NSString *t_protection = nil;
    bool t_success;

    t_success = MCDataProtectionFromString(p_protection_string, t_protection);
    
    if (t_success)
        t_success = MCFileSetDataProtection(p_path, t_protection);
    else
    {
		MCStringCreateWithCString("unknown protection type", r_status);
        return false;
    }

    if (t_success)
        return true;
    
    MCStringCreateWithCString("cannot set file protection", r_status);
    return false;
}

bool MCSystemFileGetDataProtection(MCStringRef p_path, MCStringRef& r_protection_string)
{
	NSString *t_protection = nil;
	MCAutoStringRef t_protection_string;

	bool t_success;
    t_success = true;

	if (t_success)
		t_success = MCFileGetDataProtection(p_path, t_protection);

	if (t_success)
		t_success = MCDataProtectionToString(t_protection, &t_protection_string);

	if (t_success)
    {
        r_protection_string = MCValueRetain(*t_protection_string);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

// MW-2013-05-30: [[ RemoteControl ]] Support for iOS 'remote controls' and metadata display.

static bool s_remote_control_enabled = false;

class MCRemoteControlEvent: public MCCustomEvent
{
public:
	MCRemoteControlEvent(UIEventSubtype p_type)
		: m_type(p_type)
	{
	}
	
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		const char *t_type_string;
		switch(m_type)
		{
			case UIEventSubtypeRemoteControlPlay:
				t_type_string = "play";
				break;
			case UIEventSubtypeRemoteControlPause:
				t_type_string = "pause";
				break;
			case UIEventSubtypeRemoteControlStop:
				t_type_string = "stop";
				break;
			case UIEventSubtypeRemoteControlTogglePlayPause:
				t_type_string = "toggle play pause";
				break;
			case UIEventSubtypeRemoteControlNextTrack:
				t_type_string = "next track";
				break;
			case UIEventSubtypeRemoteControlPreviousTrack:
				t_type_string = "previous track";
				break;
			case UIEventSubtypeRemoteControlBeginSeekingBackward:
				t_type_string = "begin seeking backward";
				break;
			case UIEventSubtypeRemoteControlEndSeekingBackward:
				t_type_string = "end seeking backward";
				break;
			case UIEventSubtypeRemoteControlBeginSeekingForward:
				t_type_string = "begin seeking forward";
				break;
			case UIEventSubtypeRemoteControlEndSeekingForward:
				t_type_string = "end seeking forward";
				break;
			default:
				return;
		}
		
		MCdefaultstackptr -> getcurcard() -> message_with_valueref_args(MCM_remote_control_received, MCSTR(t_type_string));
	}
	
private:
	UIEventSubtype m_type;
};

bool MCSystemEnableRemoteControl()
{
	if (!s_remote_control_enabled)
	{
		[[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
		s_remote_control_enabled = true;
	}
	
	return true;
}

bool MCSystemDisableRemoteControl()
{
	if (s_remote_control_enabled)
	{
		[[UIApplication sharedApplication] endReceivingRemoteControlEvents];
		s_remote_control_enabled = false;
	}
	
	return true;
}

bool MCSystemGetRemoteControlEnabled(bool& r_enabled)
{
	return s_remote_control_enabled;
}

enum RCDPropType
{
	kRCDPropTypeNumber,
	kRCDPropTypeString,
	kRCDPropTypeImage,
};

bool MCSystemSetRemoteControlDisplayProperties(MCExecContext& ctxt, MCArrayRef p_props)
{
    
    static bool s_resolved = false;
	static Class s_info_center = nil;
	static struct { const char *key; const char *property_symbol; RCDPropType type; NSString *property; } s_props[] =
	{
		{ "title", "MPMediaItemPropertyTitle", kRCDPropTypeString },
		{ "artist", "MPMediaItemPropertyArtist", kRCDPropTypeString },
		{ "artwork", "MPMediaItemPropertyArtwork", kRCDPropTypeImage },
		{ "composer", "MPMediaItemPropertyComposer", kRCDPropTypeString },
		{ "genre", "MPMediaItemPropertyGenre", kRCDPropTypeString },
		{ "album title", "MPMediaItemPropertyAlbumTitle", kRCDPropTypeString},
		{ "album track count", "MPMediaItemPropertyAlbumTrackCount", kRCDPropTypeNumber },
		{ "album track number", "MPMediaItemPropertyAlbumTrackNumber", kRCDPropTypeNumber },
		{ "disc count", "MPMediaItemPropertyDiscCount", kRCDPropTypeNumber },
		{ "disc number", "MPMediaItemPropertyDiscNumber", kRCDPropTypeNumber },
		{ "chapter number", "MPNowPlayingInfoPropertyChapterNumber", kRCDPropTypeNumber },
		{ "chapter count", "MPNowPlayingInfoPropertyChapterCount", kRCDPropTypeNumber },
		{ "playback duration", "MPMediaItemPropertyPlaybackDuration", kRCDPropTypeNumber },
		{ "elapsed playback time", "MPNowPlayingInfoPropertyElapsedPlaybackTime", kRCDPropTypeNumber },
		{ "playback rate", "MPNowPlayingInfoPropertyPlaybackRate", kRCDPropTypeNumber },
		{ "playback queue index", "MPNowPlayingInfoPropertyPlaybackQueueIndex", kRCDPropTypeNumber },
		{ "playback queue count", "MPNowPlayingInfoPropertyPlaybackQueueCount", kRCDPropTypeNumber },
	};
	
	
	// MW-2013-10-01: [[ Bug 11136 ]] Make sure we don't do anything if on anything less
	//   than 5.0.
	if (MCmajorosversion < 500)
		return ES_NORMAL;
	
	// MW-2013-10-01: [[ Bug 11136 ]] Fetch the symbols we cannot link to for 4.3.
	if (!s_resolved)
	{
		s_resolved = true;
        // SN-2014-01-31: [[ Bug 11703 ]] dlsym returns a pointer to NSString and not a NSString
		for(int i = 0; i < sizeof(s_props) / sizeof(s_props[0]); i++)
			s_props[i] . property = *(NSString **)dlsym(RTLD_SELF, s_props[i] . property_symbol);
		s_info_center = NSClassFromString(@"MPNowPlayingInfoCenter");
	}
	
	bool t_success;
	t_success = true;
	
	NSMutableDictionary *t_info_dict;
	t_info_dict = nil;
    
	if (t_success && p_props != nil)
	{
        MCValueRef t_prop_value;
		t_info_dict = [[NSMutableDictionary alloc] initWithCapacity: 8];
		for(uindex_t i = 0; i < sizeof(s_props) / sizeof(s_props[0]); i++)
		{
			if (!MCArrayFetchValue(p_props, false, MCNAME(s_props[i].key), t_prop_value))
				continue;
            
			NSObject *t_value;
			t_value = nil;
			switch(s_props[i] . type)
			{
				case kRCDPropTypeNumber:
                {
                    real8 t_number;
                    if (!ctxt . ConvertToReal(t_prop_value, t_number))
                        continue;
					t_value = [[NSNumber alloc] initWithDouble: t_number];
                }
                    break;
				case kRCDPropTypeString:
                {
                    MCAutoStringRef t_string;
                    if (!ctxt . ConvertToString(t_prop_value, &t_string))
                        continue;
                        
					t_value = [MCStringConvertToAutoreleasedNSString(*t_string) retain];
                }
					break;
				case kRCDPropTypeImage:
                {
                    MCAutoDataRef t_data;
                    ctxt . ConvertToData(t_prop_value, &t_data);
                    UIImage *t_image;
                    // SN-2014-01-31: [[ Bug 11703 ]] t_image wasn't initialised to nil
                    t_image = nil;
                    if (MCImageDataIsJPEG(*t_data) ||
                        MCImageDataIsGIF(*t_data) ||
                        MCImageDataIsPNG(*t_data))
                    {
                        t_image = [[UIImage alloc] initWithData: MCDataConvertToAutoreleasedNSData(*t_data)];
                    }
                    else
                    {
                        MCAutoStringRef t_string;
                        MCAutoStringRef t_resolved;
                        if (!ctxt . ConvertToString(t_prop_value, &t_string))
                            continue;
                        if (MCS_exists(*t_string, true))
                        {
                            /* UNCHECKED */ MCS_resolvepath(*t_string, &t_resolved);
                            t_image = [[UIImage alloc] initWithContentsOfFile: MCStringConvertToAutoreleasedNSString(*t_resolved)];
                        }
                    }
                    
                    if (t_image != nil)
                    {
                        t_value = [[MPMediaItemArtwork alloc] initWithImage: t_image];
                        [t_image release];
                    }
                }
                    break;
			}
            
			if (t_value == nil)
				continue;
			
			[t_info_dict setObject: t_value forKey: s_props[i] . property];
            
            [t_value release];
		}
	}
	
	if (t_success)
		[[s_info_center defaultCenter] setNowPlayingInfo: t_info_dict];
	
    [t_info_dict release];
    
	return ES_NORMAL;
}

void MCIPhoneHandleRemoteControlEvent(UIEventSubtype p_type, NSTimeInterval p_timestamp)
{
	MCCustomEvent *t_event;
	t_event = new MCRemoteControlEvent(p_type);
	MCEventQueuePostCustom(t_event);
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemSetRedrawInterval(int32_t p_interval)
{
    if (p_interval <= 0)
        MCRedrawEnableScreenUpdates();
    else
        MCRedrawDisableScreenUpdates();
    
    [MCIPhoneGetApplication() setRedrawInterval: p_interval];
    
    return true;
}

bool MCSystemSetAnimateAutorotation(bool p_enabled)
{
    [MCIPhoneGetApplication() setAnimateAutorotation: p_enabled];
    
    return true;
}
