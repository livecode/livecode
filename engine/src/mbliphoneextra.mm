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

//#include "execpt.h"
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

@interface MCExportImageToAlbumDelegate : NSObject
{
	bool m_finished;
	bool m_successful;
}

- (id)init;
- (bool)isFinished;
- (void)image: (UIImage *)image didFinishSavingWithError: (NSError *)error contextInfo: (void *)contextInfo;
@end

@implementation MCExportImageToAlbumDelegate

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

//// MOVED TO exec-misc.cpp
//static bool is_png_data(const MCString& p_data)
//{
//	return p_data . getlength() > 4 && MCMemoryEqual(p_data . getstring(), "\211PNG", 4);
//}
//
//static bool is_gif_data(const MCString& p_data)
//{
//	return p_data . getlength() > 4 && MCMemoryEqual(p_data . getstring(), "GIF8", 4);
//}
//
//static bool is_jpeg_data(const MCString& p_data)
//{
//	return p_data . getlength() > 2 && MCMemoryEqual(p_data . getstring(), "\xff\xd8", 2);
//}

struct export_image_t
{
	MCExportImageToAlbumDelegate *delegate;
	MCDataRef raw_data;
};

static void export_image(void *p_context)
{
	export_image_t *ctxt;
	ctxt = (export_image_t *)p_context;
	
	NSData *t_data;
	t_data = [[NSData alloc] initWithBytes: (void *)MCDataGetBytePtr(ctxt -> raw_data) length: MCDataGetLength(ctxt -> raw_data)];
	UIImage *t_img;
	t_img = [[UIImage alloc] initWithData: t_data];
	UIImageWriteToSavedPhotosAlbum(t_img, ctxt -> delegate, @selector(image:didFinishSavingWithError:contextInfo:), nil);
	
	[t_img release];
	[t_data release];
}

#ifdef /* MCHandleExportImageToAlbumIphone */ LEGACY_EXEC
Exec_stat MCHandleExportImageToAlbum(void *context, MCParameter *p_parameters)
{
	if (p_parameters == nil)
		return ES_NORMAL;
	
	MCExecPoint ep(nil, nil, nil);	
	p_parameters -> eval_argument(ep);

	MCString t_raw_data;
	if (is_png_data(ep . getsvalue()) ||
		is_gif_data(ep . getsvalue()) ||
		is_jpeg_data(ep . getsvalue()))
		t_raw_data = ep . getsvalue();
	else
	{
		uint4 parid;
		MCObject *objptr;
		MCChunk *tchunk = new MCChunk(False);
		MCerrorlock++;
		MCScriptPoint sp(ep);
		Parse_stat stat = tchunk->parse(sp, False);
		if (stat != PS_NORMAL || tchunk->getobj(ep, objptr, parid, True) != ES_NORMAL)
		{
			MCresult -> sets("could not find image");
			MCerrorlock--;
			delete tchunk;
			return ES_NORMAL;
		}
		
		if (objptr -> gettype() != CT_IMAGE)
		{
			MCresult -> sets("not an image");
			return ES_NORMAL;
		}
		
		MCImage *t_image;
		t_image = static_cast<MCImage *>(objptr);
		if (t_image -> getcompression() != F_PNG &&
		t_image -> getcompression() != F_JPEG &&
			t_image -> getcompression() != F_GIF)
		{
			MCresult -> sets("not a supported format");
			return ES_NORMAL;
		}
		
		t_raw_data = t_image -> getrawdata();
	}
	
	export_image_t ctxt;
	ctxt . raw_data = t_raw_data;
	ctxt . delegate = [[MCExportImageToAlbumDelegate alloc] init];
	
	MCIPhoneRunOnMainFiber(export_image, &ctxt);
	
	while(![ctxt . delegate isFinished])
		MCscreen -> wait(60.0, False, True);
	
	if ([ctxt . delegate isSuccessful])
		MCresult -> clear();
	else
		MCresult -> sets("export failed");
	
	[ctxt . delegate release];
	
	return ES_NORMAL;
}
#endif /* MCHandleExportImageToAlbumIphone */ 

bool MCSystemExportImageToAlbum(MCStringRef& r_save_result, MCDataRef p_raw_data, MCStringRef p_file_name, MCStringRef p_file_extension)
{
	export_image_t ctxt;
    ctxt . raw_data = p_raw_data;
	ctxt . delegate = [[MCExportImageToAlbumDelegate alloc] init];

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
        case kMCMiscStatusBarStyleOpaque:
            return UIStatusBarStyleBlackOpaque;
        case kMCMiscStatusBarStyleTranslucent:
            return UIStatusBarStyleBlackTranslucent;
        default:
            return UIStatusBarStyleDefault;
    }
}

bool MCSystemSetStatusBarStyle(intenum_t p_status_bar_style)
{	
	[MCIPhoneGetApplication() switchToStatusBarStyle: MCMiscUIStatusBarStyleFromMCExecEnum((MCMiscStatusBarStyle)p_status_bar_style)];
    
    return true;
}

bool MCSystemShowStatusBar()
{
#ifdef /* MCHandleShowStatusBar */ LEGACY_EXEC
	[MCIPhoneGetApplication() switchToStatusBarVisibility: YES];
	
	return ES_NORMAL;
#endif /* MCHandleShowStatusBar */
    [MCIPhoneGetApplication() switchToStatusBarVisibility: YES];
    
    return true;
}

bool MCSystemHideStatusBar()
{
#ifdef /* MCHandleHideStatusBar */ LEGACY_EXEC
	[MCIPhoneGetApplication() switchToStatusBarVisibility: NO];
	
	return ES_NORMAL;
#endif /* MCHandleHideStatusBar */
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

#ifdef /* MCHandleCameraFeaturesIphone */ LEGACY_EXEC
static Exec_stat MCHandleCameraFeatures(void *context, MCParameter *p_parameters)
{
	extern Exec_stat MCHandleSpecificCameraFeatures(void *p_context, MCParameter *p_parameters);
	if (p_parameters != nil)
		return MCHandleSpecificCameraFeatures(context, p_parameters);
	
	bool t_rearCamera = false;
	bool t_frontCamera = false;
	bool t_rearFlash = false;
	bool t_frontFlash = false;
    NSArray * t_rearMediaTypes = nil;
    NSArray * t_frontMediaTypes = nil;
	bool t_no_arguments = true;
	NSString *t_string = [[NSString alloc] init];
	NSUInteger t_string_length;
	
	MCExecPoint ep(nil, nil, nil);
	ep . clear();

	t_rearCamera = [UIImagePickerController isCameraDeviceAvailable: UIImagePickerControllerCameraDeviceRear];
	t_frontCamera = [UIImagePickerController isCameraDeviceAvailable: UIImagePickerControllerCameraDeviceFront];
	if (t_rearCamera)
		t_rearFlash = [UIImagePickerController isFlashAvailableForCameraDevice: UIImagePickerControllerCameraDeviceRear];
	if (t_frontCamera)
		t_frontFlash = [UIImagePickerController isFlashAvailableForCameraDevice: UIImagePickerControllerCameraDeviceFront];
	if (t_rearCamera)
		t_rearMediaTypes = [UIImagePickerController availableCaptureModesForCameraDevice: UIImagePickerControllerCameraDeviceRear];
	if (t_frontCamera)
		t_frontMediaTypes = [UIImagePickerController availableCaptureModesForCameraDevice: UIImagePickerControllerCameraDeviceFront];

	// we have not created the result string at this point, so the user has not yet specified what camera he is intersted in
	if (t_frontCamera)
		t_string = [t_string stringByAppendingFormat: @"front photo,"];
	NSEnumerator *t_enumerator = [t_frontMediaTypes objectEnumerator];
	for (NSNumber *t_element in t_enumerator)
	{
		if ([t_element longValue] == UIImagePickerControllerCameraCaptureModeVideo)
			t_string = [t_string stringByAppendingFormat: @"front video,"];
	}
	if (t_frontFlash)
		t_string = [t_string stringByAppendingFormat: @"front flash,"];
	if (t_rearCamera)
		t_string = [t_string stringByAppendingFormat: @"rear photo,"];
	t_enumerator = [t_rearMediaTypes objectEnumerator];
	for (NSNumber *t_element in t_enumerator)
	{
		if ([t_element longValue] == UIImagePickerControllerCameraCaptureModeVideo)
			t_string = [t_string stringByAppendingFormat: @"rear video,"];
	}
	if (t_rearFlash)
		t_string = [t_string stringByAppendingFormat: @"rear flash,"];

	t_string_length = [t_string length];
	if (t_string_length > 0)
		t_string_length--;
	t_string = [t_string substringToIndex: t_string_length];
	MCresult -> copysvalue([t_string cStringUsingEncoding: NSMacOSRomanStringEncoding]);
	return ES_NORMAL;
}
#endif /* MCHandleCameraFeaturesIphone */

/////////////////////////////////////////////////////////////////////////////////////////////////////

// We do not need this in iOS, as beep is already implemented and handled.
bool MCSystemBeep (int32_t p_number_of_beeps)
{
#ifdef /* MCSystemBeepIphone */ LEGACY_EXEC
    return true;
#endif /* MCSystemBeepIphone */
    return true;
}

bool MCSystemVibrate (int32_t p_number_of_vibrates)
{
#ifdef /* MCSystemVibrateIphone */ LEGACY_EXEC
    for (int32_t i = 0; i < p_number_of_vibrates; i++)
    {
		// MW-2012-08-06: [[ Fibers ]] Invoke the system call on the main fiber.
		MCIPhoneRunBlockOnMainFiber(^(void) {
			AudioServicesPlayAlertSound(kSystemSoundID_Vibrate); // Vibrates and beeps if no vibrate is supported
		});
		MCscreen->wait(BEEP_INTERVAL, False, False);
    }
    return true;
#endif /* MCSystemVibrateIphone */
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
#ifdef /* MCHandleDeviceResolution */ LEGACY_EXEC
	UIScreenMode *t_mode;
	t_mode = [[UIScreen mainScreen] currentMode];
    
    // MM-2012-11-22: [[ Bug 10502 ]] Make sure the resolution is reported as width height in relation to portrait.
	char t_size[U4L + 1 + U4L + 1];
    if ([t_mode size] . width < [t_mode size] .height)
        sprintf(t_size, "%u,%u", (uint32_t)[t_mode size] . width, (uint32_t)[t_mode size] . height);
    else
        sprintf(t_size, "%u,%u", (uint32_t)[t_mode size] . height, (uint32_t)[t_mode size] . width);
	MCresult -> copysvalue(t_size);
    
	return ES_NORMAL;
#endif /* MCHandleDeviceResolution */
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
#ifdef /* MCHandleDeviceScale */ LEGACY_EXEC
	MCresult -> setnvalue(MCIPhoneGetDeviceScale());
	
	return ES_NORMAL;
#endif /* MCHandleDeviceScale */

    r_scale = MCIPhoneGetDeviceScale();
    
    return true;
}

bool MCSystemGetPixelDensity(real64_t& r_density)
{
    // Not implemented on IPhone
    return false;
}

/* MOVED TO mbliphoneorientation.mm */
#ifdef /* MCHandleDeviceOrientationIphone */ LEGACY_EXEC
static Exec_stat MCHandleDeviceOrientation(void *context, MCParameter *p_parameters)
{
	MCresult -> sets(s_orientation_names[[[UIDevice currentDevice] orientation]]);
	return ES_NORMAL;
}
#endif /* MCHandleDeviceOrientationIphone */

#ifdef /* MCHandleOrientationIphone */ LEGACY_EXEC
static Exec_stat MCHandleOrientation(void *context, MCParameter *p_parameters)
{
	MCresult -> sets(s_orientation_names[(int)MCIPhoneGetOrientation()]);
	return ES_NORMAL;
}
#endif /* MCHandleOrientationIphone */

#ifdef /* MCHandleAllowedOrientationsIphone */ LEGACY_EXEC
static Exec_stat MCHandleAllowedOrientations(void *context, MCParameter *p_parameters)
{
	uint32_t t_orientations;
	t_orientations = (uint32_t)[MCIPhoneGetApplication() allowedOrientations];
	
	MCExecPoint ep(nil, nil, nil);
	for(uint32_t j = 0; s_orientation_names[j] != nil; j++)
		if ((t_orientations & (1 << j)) != 0)
			ep . concatcstring(s_orientation_names[j], EC_COMMA, ep . isempty());

	MCresult -> store(ep, True);
    
	return ES_NORMAL;
}
#endif /* MCHandleAllowedOrientationsIphone */

#ifdef /* MCHandleOrientationLockedIphone */ LEGACY_EXEC
static Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters)
{
	MCresult -> sets(MCU_btos(((int)[MCIPhoneGetApplication() orientationLocked]) == YES));
	return ES_NORMAL;
}
#endif /* MCHandleOrientationLockedIphone */

#ifdef /* MCHandleLockUnlockOrientation */ LEGACY_EXEC
static Exec_stat MCHandleLockUnlockOrientation(void *context, MCParameter *p_parameters)
{
	if ((bool)context)
		[MCIPhoneGetApplication() lockOrientation];
	else
		[MCIPhoneGetApplication() unlockOrientation];
	return ES_NORMAL;
}
#endif /* MCHandleLockUnlockOrientation */

#ifdef /* MCHandleSetAllowedOrientationsIphone */ LEGACY_EXEC
static Exec_stat MCHandleSetAllowedOrientations(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	char *t_orientations;
	t_orientations = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_orientations);
	
	char **t_orientations_array;
	uint32_t t_orientations_count;
	t_orientations_array = nil;
	t_orientations_count = 0;
	if (t_success)
		t_success = MCCStringSplit(t_orientations, ',', t_orientations_array, t_orientations_count);
	
	uint32_t t_orientations_set;
	t_orientations_set = 0;
	if (t_success)
		for(uint32_t i = 0; i < t_orientations_count; i++)
			for(uint32_t j = 0; s_orientation_names[j] != nil; j++)
				if (MCCStringEqualCaseless(t_orientations_array[i], s_orientation_names[j]))
					t_orientations_set |= (1 << j);
	
	[MCIPhoneGetApplication() setAllowedOrientations: t_orientations_set];
	
	for(uint32_t i = 0; i < t_orientations_count; i++)
		MCCStringFree(t_orientations_array[i]);
	MCMemoryDeleteArray(t_orientations_array);
	
	MCCStringFree(t_orientations);
	
	return ES_NORMAL;
}
#endif /* MCHandleSetAllowedOrientationsIphone */

// MW-2013-10-02: [[ MobileSSLVerify ]] Handle libUrlSetSSLVerification for iOS.
#ifdef /* MCHandleLibUrlSetSSLVerificationIphone */ LEGACY_EXEC
static Exec_stat MCHandleLibUrlSetSSLVerification(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	bool t_enabled;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "b", &t_enabled);
	
	extern void MCS_seturlsslverification(bool enabled);
	if (t_success)
		MCS_seturlsslverification(t_enabled);
	
	return ES_NORMAL;
}
#endif /* MCHandleLibUrlSetSSLVerificationIphone */

////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCHandleSetKeyboardTypeIphone */ LEGACY_EXEC
static Exec_stat MCHandleSetKeyboardType (void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);

	if (p_parameters != nil)
	{
		UIKeyboardType t_type;
		p_parameters -> eval_argument(ep);
		if (ep . getsvalue() == "default")
			t_type = UIKeyboardTypeDefault;
		else if (ep . getsvalue() == "alphabet")
			t_type = UIKeyboardTypeAlphabet;
		else if (ep . getsvalue() == "numeric")
			t_type = UIKeyboardTypeNumbersAndPunctuation;
		else if (ep . getsvalue() == "url")
			t_type = UIKeyboardTypeURL;
		else if (ep . getsvalue() == "number")
			t_type = UIKeyboardTypeNumberPad;
		else if (ep . getsvalue() == "phone")
			t_type = UIKeyboardTypePhonePad;
		else if (ep . getsvalue() == "contact")
			t_type = UIKeyboardTypeNamePhonePad;
		else if (ep . getsvalue() == "email")
			t_type = UIKeyboardTypeEmailAddress;
#ifdef __IPHONE_4_1
		else if (ep . getsvalue() == "decimal")
			t_type = UIKeyboardTypeDecimalPad;				
#endif
		
		MCIPhoneSetKeyboardType(t_type);
	}
	return ES_NORMAL;
}
#endif /* MCHandleSetKeyboardTypeIphone */

static UIKeyboardType MCMiscGetUIKeyboardTypeFromExecEnum(MCMiscKeyboardType p_type)
{
    switch(p_type)
    {
        case kMCMiscKeyboardTypeAlphabet:
            return UIKeyboardTypeAlphabet;
        case kMCMiscKeyboardTypeNumeric:
            return UIKeyboardTypeNumbersAndPunctuation;
        case kMCMiscKeyboardTypeUrl:
            return UIKeyboardTypeURL;
        case kMCMiscKeyboardTypeNumber:
            return UIKeyboardTypeNumberPad;
        case kMCMiscKeyboardTypePhone:
            return UIKeyboardTypePhonePad;
        case kMCMiscKeyboardTypeContact:
            return UIKeyboardTypeNamePhonePad;
        case kMCMiscKeyboardTypeEmail:
            return UIKeyboardTypeEmailAddress;
#ifdef __IPHONE_4_1
        case kMCMiscKeyboardTypeDecimal:
            return UIKeyboardTypeDecimalPad;
#endif // __IPHONE_4_1
        default:
            return UIKeyboardTypeDefault;
    }
}

bool MCSystemSetKeyboardType(intenum_t p_type)
{
    MCIPhoneSetKeyboardType(MCMiscGetUIKeyboardTypeFromExecEnum((MCMiscKeyboardType)p_type));
    
    return  true;
}

static UIReturnKeyType MCMiscGetUIReturnKeyTypeFromMCExecEnum(MCMiscKeyboardReturnKey p_type)
{
    switch(p_type)
    {
        case kMCMiscKeyboardReturnKeyGo:
            return UIReturnKeyGo;
        case kMCMiscKeyboardReturnKeyGoogle:
            return UIReturnKeyGoogle;
        case kMCMiscKeyboardReturnKeyJoin:
            return UIReturnKeyJoin;
        case kMCMiscKeyboardReturnKeyNext:
            return UIReturnKeyNext;
        case kMCMiscKeyboardReturnKeySearch:
            return UIReturnKeySearch;
        case kMCMiscKeyboardReturnKeySend:
            return UIReturnKeySend;
        case kMCMiscKeyboardReturnKeyRoute:
            return UIReturnKeyRoute;
        case kMCMiscKeyboardReturnKeyYahoo:
            return UIReturnKeyYahoo;
        case kMCMiscKeyboardReturnKeyDone:
            return UIReturnKeyDone;
        case kMCMiscKeyboardReturnKeyEmergencyCall:
            return UIReturnKeyEmergencyCall;
        default:
            return UIReturnKeyDefault;
    }
}

bool MCSystemSetKeyboardReturnKey(intenum_t p_type)
{
    MCIPhoneSetReturnKeyType(MCMiscGetUIReturnKeyTypeFromMCExecEnum((MCMiscKeyboardReturnKey)p_type));
                             
    return true;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCHandleCurrentLocaleIphone */ LEGACY_EXEC
static Exec_stat MCHandleCurrentLocale(void *context, MCParameter *p_parameters)
{
	NSString *t_current_locale_id = nil;
	t_current_locale_id = [[NSLocale currentLocale] objectForKey: NSLocaleIdentifier];
	
	const char *t_id_string = nil;
	t_id_string = [t_current_locale_id cStringUsingEncoding: NSMacOSRomanStringEncoding];
	
	MCExecPoint ep;
	ep.setsvalue(t_id_string);
	MCresult->store(ep, True);
	
	return ES_NORMAL;
}
#endif /* MCHandleCurrentLocaleIphone */

#ifdef /* MCHandlePreferredLanguagesIphone */ LEGACY_EXEC
static Exec_stat MCHandlePreferredLanguages(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	NSArray *t_preferred_langs = nil;
	t_preferred_langs = [NSLocale preferredLanguages];
	t_success = t_preferred_langs != nil && [t_preferred_langs count] != 0;
    
	MCExecPoint ep;
	const char *t_lang_string = nil;
	if (t_success)
	{
		bool t_first = true;
		for (NSString *t_lang in t_preferred_langs)
		{
			ep.concatcstring([t_lang cStringUsingEncoding: NSMacOSRomanStringEncoding], EC_RETURN, t_first);
			t_first = false;
		}
	}
	
	if (t_success)
		MCresult->store(ep, True);
	else
		MCresult->clear();
	
	return ES_NORMAL;
}
#endif /* MCHandlePreferredLanguagesIphone */

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
            if (t_success && MCStringCreateWithCFString((CFStringRef)t_lang, &t_language))
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

	/* UNCHECKED */ MCStringCreateWithCFString((CFStringRef)t_current_locale_id, r_current_locale);

	return true;
}

// MOVED TO mblhandlers.cpp
#ifdef /* MCHandleLibUrlDownloadToFileIphone */ LEGACY_EXEC
static Exec_stat MCHandleLibUrlDownloadToFile(void *context, MCParameter *p_parameters)
{
	char *t_url, *t_filename;
	t_url = nil;
	t_filename = nil;
    
	MCExecPoint ep(nil, nil, nil);
    
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_url = ep . getsvalue() . clone();
		p_parameters = p_parameters -> getnext();
	}
    
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_filename = ep . getsvalue() . clone();
		p_parameters = p_parameters -> getnext();
	}
    
	extern void MCS_downloadurl(MCObject *, const char *, const char *);
	MCS_downloadurl(MCtargetptr, t_url, t_filename);
    
    // IM-2012-04-04 - these were allocated but not released
    MCCStringFree(t_url);
    MCCStringFree(t_filename);
    
	return ES_NORMAL;
}
#endif /* MCHandleLibUrlDownloadToFileIphone */

////////////////////////////////////////////////////////////////////////////////

bool MCSystemGetSystemIdentifier(MCStringRef& r_identifier)
{
#ifdef /* MCHandleSystemIdentifier */ LEGACY_EXEC
    // MM-2013-05-21: [[ Bug 10895 ]] The method uniqueIdentifier of UIDevice is now deprecated (as of May 2013).
    //  Calling the method dynamically prevents apps from being rejected by the app store
    //  but preserves functionality for testing and backwards compatibility.
    NSString *t_identifier;
    t_identifier = objc_msgSend([UIDevice currentDevice], sel_getUid("uniqueIdentifier"));
    MCresult -> copysvalue([t_identifier cStringUsingEncoding: NSMacOSRomanStringEncoding]);
    return ES_NORMAL;
#endif /* MCHandleSystemIdentifier */

    // MM-2013-05-21: [[ Bug 10895 ]] The method uniqueIdentifier of UIDevice is now deprecated (as of May 2013).
    //  Calling the method dynamically prevents apps from being rejected by the app store
    //  but preserves functionality for testing and backwards compatibility.
    NSString *t_identifier;
    t_identifier = objc_msgSend([UIDevice currentDevice], sel_getUid("uniqueIdentifier"));
	
    return MCStringCreateWithCFString((CFStringRef)t_identifier, r_identifier);
}

bool MCSystemGetIdentifierForVendor(MCStringRef& r_identifier)
{
// MM-2013-05-21: [[ Bug 10895 ]] Added iphoneIdentifierForVendor as an initial replacement for iphoneSystemIdentifier.
//  identifierForVendor was only added to UIDevice in iOS 6.1 so make sure we weakly link.
#ifdef /* MCHandleIdentifierForVendor */ LEGACY_EXEC
static Exec_stat MCHandleIdentifierForVendor(void *context, MCParameter *p_parameters)
{
    if ([UIDevice instancesRespondToSelector:@selector(identifierForVendor)])
    {
        NSString *t_identifier;
        t_identifier = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
        MCresult -> copysvalue([t_identifier cStringUsingEncoding: NSMacOSRomanStringEncoding]);
    }
    else
        MCresult -> clear();
    return ES_NORMAL;
}
#endif /* MCHandleIdentifierForVendor */
    
    if ([UIDevice instancesRespondToSelector:@selector(identifierForVendor)])
    {
        NSString *t_identifier;
        t_identifier = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
        return MCStringCreateWithCFString((CFStringRef)t_identifier, r_identifier);
    }

    r_identifier = MCValueRetain(kMCEmptyString);
    return true;
}


bool MCSystemGetApplicationIdentifier(MCStringRef& r_identifier)
{
#ifdef /* MCHandleApplicationIdentifier */ LEGACY_EXEC
	// Get the plist
	NSDictionary *t_plist;
	t_plist = [[NSBundle mainBundle] infoDictionary];
	
	NSString *t_identifier;
	t_identifier = [t_plist objectForKey: @"CFBundleIdentifier"];
	
	MCresult -> copysvalue([t_identifier cStringUsingEncoding: NSMacOSRomanStringEncoding]);

	return ES_NORMAL;
#endif /* MCHandleApplicationIdentifier */
	// Get the plist
	NSDictionary *t_plist;
	t_plist = [[NSBundle mainBundle] infoDictionary];
	
	NSString *t_identifier;
	t_identifier = [t_plist objectForKey: @"CFBundleIdentifier"];
	
	return MCStringCreateWithCFString((CFStringRef)t_identifier, r_identifier);
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
#ifdef /* MCiOSFileSetDoNotBackup */ LEGACY_EXEC
    bool t_success = true;
    if (p_no_backup)
    {
        uint8_t t_val = 1;
        t_success = 0 == setxattr(p_path, FILEATTR_DONOTBACKUP, &t_val, sizeof(t_val), 0, 0);
    }
    else
    {
        t_success = 0 == removexattr(p_path, FILEATTR_DONOTBACKUP, 0);
    }
    return t_success;
#endif /* MCiOSFileSetDoNotBackup */
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
#ifdef /* MCiOSFileGetDoNotBackup */ LEGACY_EXEC
    uint8_t t_val = 0;
    if (-1 == getxattr(p_path, FILEATTR_DONOTBACKUP, &t_val, sizeof(t_val), 0, 0))
        return false;
    return t_val != 0;
#endif /* MCiOSFileGetDoNotBackup */
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
#ifdef /* MCHandleSetRemoteControlDisplay */ LEGACY_EXEC
static Exec_stat MCHandleSetRemoteControlDisplay(void *context, MCParameter *p_parameters)
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
	
	MCVariableValue *t_props;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "a", &t_props);
	
	NSMutableDictionary *t_info_dict;
	t_info_dict = nil;
	if (t_success && t_props != nil)
	{
		t_info_dict = [[NSMutableDictionary alloc] initWithCapacity: 8];
		for(uindex_t i = 0; i < sizeof(s_props) / sizeof(s_props[0]); i++)
		{
			MCExecPoint ep;
			
			MCHashentry *t_entry;
			if (!t_props -> has_element(ep, s_props[i] . key))
				continue;
			
			if (t_props -> fetch_element(ep, s_props[i] . key) != ES_NORMAL)
				continue;
			
			NSObject *t_value;
			t_value = nil;
			switch(s_props[i] . type)
			{
				case kRCDPropTypeNumber:
					t_value = [[NSNumber alloc] initWithDouble: ep . getnvalue()];
				break;
				case kRCDPropTypeString:
					t_value = [[NSString alloc] initWithCString: ep . getcstring() encoding: NSMacOSRomanStringEncoding];
					break;
				case kRCDPropTypeImage:
                {
                    UIImage *t_image;
                    // SN-2014-01-31: [[ Bug 11703 ]] t_image wasn't initialised to nil
                    t_image = nil;
                    if (MCImageDataIsJPEG(ep . getsvalue()) ||
                        MCImageDataIsGIF(ep . getsvalue()) ||
                        MCImageDataIsPNG(ep . getsvalue()))
                    {
                        t_image = [[UIImage alloc] initWithData: [NSData dataWithBytes: ep . getsvalue() . getstring() length: ep . getsvalue() . getlength()]];
                    }
                    else if (MCS_exists(ep . getcstring(), true))
                    {
                        MCAutoPointer<char> t_resolved_path;
                        /* UNCHECKED */ t_resolved_path = MCS_resolvepath(ep . getcstring());
                        t_image = [[UIImage alloc] initWithContentsOfFile: [NSString stringWithCString: *t_resolved_path encoding: NSMacOSRomanStringEncoding]];
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
	
	return ES_NORMAL;
}
#endif  /* MCHandleSetRemoteControlDisplay */
    
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
            MCNewAutoNameRef t_key;
            if (!MCNameCreateWithCString(s_props[i] . key, &t_key))
                return false;
            
			if (!MCArrayFetchValue(p_props, false, *t_key, t_prop_value))
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
					t_value = [NSString stringWithMCStringRef: *t_string];
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
                        t_image = [[UIImage alloc] initWithData: [NSData dataWithMCDataRef : *t_data]];
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
                            t_image = [[UIImage alloc] initWithContentsOfFile: [NSString stringWithMCStringRef: *t_resolved]];
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
	
	return ES_NORMAL;
}

void MCIPhoneHandleRemoteControlEvent(UIEventSubtype p_type, NSTimeInterval p_timestamp)
{
	MCCustomEvent *t_event;
	t_event = new MCRemoteControlEvent(p_type);
	MCEventQueuePostCustom(t_event);
}

////////////////////////////////////////////////////////////////////////////////

/* MOVED TO mbliphoneidletimer.mm */
#ifdef /* MCHandleLockIdleTimerIphone */ LEGACY_EXEC
static Exec_stat MCHandleLockIdleTimer(void *context, MCParameter *p_parameters)
{
	g_idle_timer++;
	if (g_idle_timer == 1)
		[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
	return ES_NORMAL;
}
#endif /* MCHandleLockIdleTimerIphone */

#ifdef /* MCHandleUnlockIdleTimerIphone */ LEGACY_EXEC
static Exec_stat MCHandleUnlockIdleTimer(void *context, MCParameter *p_paramters)
{
	if (g_idle_timer == 1)
		[[UIApplication sharedApplication] setIdleTimerDisabled:NO];
	if (g_idle_timer > 0)
		g_idle_timer--;	
	return ES_NORMAL;
}
#endif /* MCHandleUnlockIdleTimerIphone */

#ifdef /* MCHandleIdleTimerLockedIphone */ LEGACY_EXEC
static Exec_stat MCHandleIdleTimerLocked(void *context, MCParameter *p_paramters)
{
	MCExecPoint ep(nil, nil, nil);
	if ([[UIApplication sharedApplication] isIdleTimerDisabled] == YES)
		MCresult -> sets(MCtruemcstring);
	else
		MCresult -> sets(MCfalsestring);
	return ES_NORMAL;
}
#endif /* MCHandleIdleTimerLockedIphone */

////////////////////////////////////////////////////////////////////////////////

// MOVED TO mblhandlers.cpp
#ifdef /* MCHandleClearTouchesIphone */ LEGACY_EXEC
static Exec_stat MCHandleClearTouches(void *context, MCParameter *p_parameters)
{
	MCscreen -> wait(1/25.0, False, False);
	static_cast<MCScreenDC *>(MCscreen) -> clear_touches();
	MCEventQueueClearTouches();
	return ES_NORMAL;
}
#endif /* MCHandleClearTouchesIphone */

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

////////////////////////////////////////////////////////////////////////////////
#ifdef LEGACY_EXEC
static MCPlatformMessageSpec s_platform_messages[] =
{
    // TODO - check for new mobile commands and functions to refactor
    // MM-2012-02-22: Added support for ad management
    {false, "mobileAdRegister", MCHandleAdRegister, nil},
    {false, "mobileAdCreate", MCHandleAdCreate, nil},
    {false, "mobileAdDelete", MCHandleAdDelete, nil},
    {false, "mobileAdGetVisible", MCHandleAdGetVisible, nil},
    {false, "mobileAdSetVisible", MCHandleAdSetVisible, nil},
    {false, "mobileAdGetTopLeft", MCHandleAdGetTopLeft, nil},
    {false, "mobileAdSetTopLeft", MCHandleAdSetTopLeft, nil},
    {false, "mobileAds", MCHandleAds, nil},
    {false, "iphoneAdRegister", MCHandleAdRegister, nil},
    {false, "iphoneAdCreate", MCHandleAdCreate, nil},
    {false, "iphoneAdDelete", MCHandleAdDelete, nil},
    {false, "iphoneAdGetVisible", MCHandleAdGetVisible, nil},
    {false, "iphoneAdSetVisible", MCHandleAdSetVisible, nil},
    {false, "iphoneAdGetTopLeft", MCHandleAdGetTopLeft, nil},
    {false, "iphoneAdSetTopLeft", MCHandleAdSetTopLeft, nil},
    {false, "iphoneAds", MCHandleAds, nil},
    
    {false, "mobileCreateLocalNotification", MCHandleCreateLocalNotification, nil},
    {false, "mobileGetRegisteredNotifications", MCHandleGetRegisteredNotifications, nil},
    {false, "mobileGetNotificationDetails", MCHandleGetNotificationDetails, nil},
    {false, "mobileCancelLocalNotification", MCHandleCancelLocalNotification, nil},
    {false, "mobileCancelAllLocalNotifications", MCHandleCancelAllLocalNotifications, nil},
    {false, "iphoneCreateLocalNotification", MCHandleCreateLocalNotification, nil},
    {false, "iphoneGetRegisteredNotifications", MCHandleGetRegisteredNotifications, nil},
    {false, "iphoneCancelLocalNotification", MCHandleCancelLocalNotification, nil},
    {false, "iphoneCancelAllLocalNotifications", MCHandleCancelAllLocalNotifications, nil},
    
    {false, "iphoneGetNotificationBadgeValue", MCHandleGetNotificationBadgeValue, nil},
    {false, "iphoneSetNotificationBadgeValue", MCHandleSetNotificationBadgeValue, nil},
    
    {false, "mobileGetDeviceToken", MCHandleGetDeviceToken, nil},
    {false, "mobileGetLaunchUrl", MCHandleGetLaunchUrl, nil},
    {false, "iphoneGetDeviceToken", MCHandleGetDeviceToken, nil},
    {false, "iphoneGetLaunchUrl", MCHandleGetLaunchUrl, nil},
    
    {false, "iphoneActivityIndicatorStart", MCHandleStartActivityIndicator, nil},
    {false, "iphoneActivityIndicatorStop", MCHandleStopActivityIndicator, nil},
    
    {false, "mobileBusyIndicatorStart", MCHandleStartBusyIndicator, nil},
    {false, "mobileBusyIndicatorStop", MCHandleStopBusyIndicator, nil},
    {false, "iphoneBusyIndicatorStart", MCHandleStartBusyIndicator, nil},
    {false, "iphoneBusyIndicatorStop", MCHandleStopBusyIndicator, nil},
    
    {false, "mobileBeep", MCHandleBeep, nil},
    {true, "mobileVibrate", MCHandleVibrate, nil},
    {false, "iphoneBeep", MCHandleBeep, nil},
    {true, "iphoneVibrate", MCHandleVibrate, nil},
    
    {true, "iphonePickPhoto", MCHandlePickPhoto, nil},
    {true, "iphonePickMedia", MCHandleIPhonePickMedia, nil},
    {true, "mobilePickMedia", MCHandleIPhonePickMedia, nil},
    {false, "iphoneCameraFeatures", MCHandleCameraFeatures, nil},
    {true, "mobilePickPhoto", MCHandlePickPhoto, nil},
    {false, "mobileCameraFeatures", MCHandleCameraFeatures, nil},
    
    {false, "iphoneDeviceOrientation", MCHandleDeviceOrientation, nil},
    {false, "mobileDeviceOrientation", MCHandleDeviceOrientation, nil},
    
    {false, "iphoneOrientation", MCHandleOrientation, nil},
    {false, "iphoneAllowedOrientations", MCHandleAllowedOrientations, nil},
    {false, "iphoneSetAllowedOrientations", MCHandleSetAllowedOrientations, nil},
    {false, "iphoneOrientationLocked", MCHandleOrientationLocked, nil},
    {false, "iphoneLockOrientation", MCHandleLockUnlockOrientation, (void*)true},
    {false, "iphoneUnlockOrientation", MCHandleLockUnlockOrientation, (void*)false},
    {false, "mobileOrientation", MCHandleOrientation, nil},
    {false, "mobileAllowedOrientations", MCHandleAllowedOrientations, nil},
    {false, "mobileSetAllowedOrientations", MCHandleSetAllowedOrientations, nil},
    {false, "mobileOrientationLocked", MCHandleOrientationLocked, nil},
    {false, "mobileLockOrientation", MCHandleLockUnlockOrientation, (void*)true},
    {false, "mobileUnlockOrientation", MCHandleLockUnlockOrientation, (void*)false},
    
    {false, "iphoneDeviceResolution", MCHandleDeviceResolution, nil},
    {false, "iphoneUseDeviceResolution", MCHandleUseDeviceResolution, nil},
    {false, "iphoneDeviceScale", MCHandleDeviceScale, nil},
    {false, "mobilePixelDensity", MCHandleDeviceScale, nil},
    
    {false, "mobileStartTrackingSensor", MCHandleStartTrackingSensor, nil},
    {false, "mobileStopTrackingSensor", MCHandleStopTrackingSensor, nil},
    {false, "mobileSensorReading", MCHandleSensorReading, nil},
    {false, "mobileSensorAvailable", MCHandleSensorAvailable, nil},
    
    // MM-2012-02-11: Added support old style senseor syntax (iPhoneEnableAcceleromter etc)
    /* DEPRECATED */ {false, "iphoneCanTrackLocation", MCHandleCanTrackLocation, nil},
    /* DEPRECATED */ {false, "iphoneStartTrackingLocation", MCHandleLocationTrackingState, (void *)true},
    /* DEPRECATED */ {false, "iphoneStopTrackingLocation", MCHandleLocationTrackingState, (void *)false},
    /* DEPRECATED */ {false, "iphoneCurrentLocation", MCHandleCurrentLocation, nil},
    /* DEPRECATED */ {false, "mobileCanTrackLocation", MCHandleCanTrackLocation, nil},
    /* DEPRECATED */ {false, "mobileStartTrackingLocation", MCHandleLocationTrackingState, (void *)true},
    /* DEPRECATED */ {false, "mobileStopTrackingLocation", MCHandleLocationTrackingState, (void *)false},
    /* DEPRECATED */ {false, "mobileCurrentLocation", MCHandleCurrentLocation, nil},
    
    /* DEPRECATED */ {false, "iphoneCanTrackHeading", MCHandleCanTrackHeading, nil},
    /* DEPRECATED */ {false, "iphoneStartTrackingHeading", MCHandleHeadingTrackingState, (void *)true},
    /* DEPRECATED */ {false, "iphoneStopTrackingHeading", MCHandleHeadingTrackingState, (void *)false},
    /* DEPRECATED */ {false, "iphoneCurrentHeading", MCHandleCurrentHeading, nil},
    {false, "iphoneSetHeadingCalibrationTimeout", MCHandleSetHeadingCalibrationTimeout, nil},
    {false, "iphoneHeadingCalibrationTimeout", MCHandleHeadingCalibrationTimeout, nil},
    /* DEPRECATED */ {false, "mobileCanTrackHeading", MCHandleCanTrackHeading, nil},
    /* DEPRECATED */ {false, "mobileStartTrackingHeading", MCHandleHeadingTrackingState, (void *)true},
    /* DEPRECATED */ {false, "mobileStopTrackingHeading", MCHandleHeadingTrackingState, (void *)false},
    /* DEPRECATED */ {false, "mobileCurrentHeading", MCHandleCurrentHeading, nil},
    
    /* DEPRECATED */ {false, "iphoneEnableAccelerometer", MCHandleAccelerometerEnablement, (void *)true},
    /* DEPRECATED */ {false, "iphoneDisableAccelerometer", MCHandleAccelerometerEnablement, (void *)false},
    /* DEPRECATED */ {false, "mobileEnableAccelerometer", MCHandleAccelerometerEnablement, (void *)true},
    /* DEPRECATED */ {false, "mobileDisableAccelerometer", MCHandleAccelerometerEnablement, (void *)false},
    
    {true, "iphoneComposeTextMessage", MCHandleComposeTextMessage, nil},
    {false, "iphoneCanComposeTextMessage", MCHandleCanComposeTextMessage, nil},
    {true, "mobileComposeTextMessage", MCHandleComposeTextMessage, nil},
    {false, "mobileCanComposeTextMessage", MCHandleCanComposeTextMessage, nil},
    
    {true, "revMail", MCHandleRevMail, nil},
    
    {false, "iphoneCanSendMail", MCHandleCanSendMail, nil},
    {true, "iphoneComposeMail", MCHandleComposePlainMail, nil},
    {true, "iphoneComposeUnicodeMail", MCHandleComposeUnicodeMail, nil},
    {true, "iphoneComposeHtmlMail", MCHandleComposeHtmlMail, nil},
    {false, "mobileCanSendMail", MCHandleCanSendMail, nil},
    {true, "mobileComposeMail", MCHandleComposePlainMail, nil},
    {true, "mobileComposeUnicodeMail", MCHandleComposeUnicodeMail, nil},
    {true, "mobileComposeHtmlMail", MCHandleComposeHtmlMail, nil},
    
    {true, "libUrlDownloadToFile", MCHandleLibUrlDownloadToFile, nil},
    
    // MW-2013-10-02: [[ MobileSSLVerify ]] Added support for libUrlSetSSLVerification.
    {true, "libUrlSetSSLVerification", MCHandleLibUrlSetSSLVerification, nil},
    
    {false, "iphoneSetStatusBarStyle", MCHandleSetStatusBarStyle, nil},
    {false, "iphoneShowStatusBar", MCHandleShowStatusBar, nil},
    {false, "iphoneHideStatusBar", MCHandleHideStatusBar, nil},
    {false, "mobileSetStatusBarStyle", MCHandleSetStatusBarStyle, nil},
    {false, "mobileShowStatusBar", MCHandleShowStatusBar, nil},
    {false, "mobileHideStatusBar", MCHandleHideStatusBar, nil},
    
    {true, "iphonePick", MCHandlePick, nil},
    {true, "iphonePickDate", MCHandlePickDate, nil},
    
    {true, "mobilePickDate", MCHandlePickDate, nil},
    {true, "mobilePickTime", MCHandlePickTime, nil},
    {true, "mobilePickDateAndTime", MCHandlePickDateAndTime, nil},
    {true, "mobilePick", MCHandlePick, nil},
    
    {false, "iphoneSetKeyboardType", MCHandleSetKeyboardType, nil},
    {false, "iphoneSetKeyboardReturnKey", MCHandleSetKeyboardReturnKey, nil},
    {false, "mobileSetKeyboardType", MCHandleSetKeyboardType, nil},
    
    {false, "iphoneControlCreate", MCHandleControlCreate, nil},
    {false, "iphoneControlDelete", MCHandleControlDelete, nil},
    {false, "iphoneControlSet", MCHandleControlSet, nil},
    {false, "iphoneControlGet", MCHandleControlGet, nil},
    {false, "iphoneControlDo", MCHandleControlDo, nil},
    {false, "iphoneControlTarget", MCHandleControlTarget, nil},
    {false, "iphoneControls", MCHandleControlList, nil},
    {false, "mobileControlCreate", MCHandleControlCreate, nil},
    {false, "mobileControlDelete", MCHandleControlDelete, nil},
    {false, "mobileControlSet", MCHandleControlSet, nil},
    {false, "mobileControlGet", MCHandleControlGet, nil},
    {false, "mobileControlDo", MCHandleControlDo, nil},
    {false, "mobileControlTarget", MCHandleControlTarget, nil},
    {false, "mobileControls", MCHandleControlList, nil},
    
    {false, "iphonePreferredLanguages", MCHandlePreferredLanguages, nil},
    {false, "mobilePreferredLanguages", MCHandlePreferredLanguages, nil},
    {false, "iphoneCurrentLocale", MCHandleCurrentLocale, nil},
    {false, "mobileCurrentLocale", MCHandleCurrentLocale, nil},
    
    {false, "iphonePlaySoundOnChannel", MCHandlePlaySoundOnChannel, nil},
    {false, "iphonePausePlayingOnChannel", MCHandlePausePlayingOnChannel},
    {false, "iphoneResumePlayingOnChannel", MCHandleResumePlayingOnChannel},
    {false, "iphoneStopPlayingOnChannel", MCHandleStopPlayingOnChannel, nil},
    {false, "iphoneDeleteSoundChannel", MCHandleDeleteSoundChannel, nil},
    {false, "iphoneSetSoundChannelVolume", MCHandleSetSoundChannelVolume, nil},
    {false, "iphoneSoundChannelVolume", MCHandleSoundChannelVolume, nil},
    {false, "iphoneSoundChannelStatus", MCHandleSoundChannelStatus, nil},
    {false, "iphoneSoundOnChannel", MCHandleSoundOnChannel, nil},
    {false, "iphoneNextSoundOnChannel", MCHandleNextSoundOnChannel, nil},
    {false, "iphoneSoundChannels", MCHandleSoundChannels, nil},
    
    // MM-2012-09-02: Add support for mobile* multi channel sound syntax
    {false, "mobilePlaySoundOnChannel", MCHandlePlaySoundOnChannel, nil},
    {false, "mobilePausePlayingOnChannel", MCHandlePausePlayingOnChannel},
    {false, "mobileResumePlayingOnChannel", MCHandleResumePlayingOnChannel},
    {false, "mobileStopPlayingOnChannel", MCHandleStopPlayingOnChannel, nil},
    {false, "mobileDeleteSoundChannel", MCHandleDeleteSoundChannel, nil},
    {false, "mobileSetSoundChannelVolume", MCHandleSetSoundChannelVolume, nil},
    {false, "mobileSoundChannelVolume", MCHandleSoundChannelVolume, nil},
    {false, "mobileSoundChannelStatus", MCHandleSoundChannelStatus, nil},
    {false, "mobileSoundOnChannel", MCHandleSoundOnChannel, nil},
    {false, "mobileNextSoundOnChannel", MCHandleNextSoundOnChannel, nil},
    {false, "mobileSoundChannels", MCHandleSoundChannels, nil},
    
    {false, "iphoneLockIdleTimer", MCHandleLockIdleTimer, nil},
    {false, "mobileLockIdleTimer", MCHandleLockIdleTimer, nil},
    {false, "iphoneUnlockIdleTimer", MCHandleUnlockIdleTimer, nil},
    {false, "mobileUnlockIdleTimer", MCHandleUnlockIdleTimer, nil},
    {false, "iphoneIdleTimerLocked", MCHandleIdleTimerLocked, nil},
    {false, "mobileIdleTimerLocked", MCHandleIdleTimerLocked, nil},
    
    {false, "iphoneClearTouches", MCHandleClearTouches, nil},
    {false, "mobileClearTouches", MCHandleClearTouches, nil},
    
    {false, "iphoneSystemIdentifier", MCHandleSystemIdentifier, nil},
    {false, "iphoneApplicationIdentifier", MCHandleApplicationIdentifier, nil},
    
    // MM-2013-05-21: [[ Bug 10895 ]] Added iphoneIdentifierForVendor as an initial replacement for iphoneSystemIdentifier.
    {false, "mobileIdentifierForVendor", MCHandleIdentifierForVendor, nil},
    {false, "iphoneIdentifierForVendor", MCHandleIdentifierForVendor, nil},
    
    {false, "iphoneSetReachabilityTarget", MCHandleSetReachabilityTarget, nil},
    {false, "iphoneReachabilityTarget", MCHandleReachabilityTarget, nil},
    
    {true, "iphoneExportImageToAlbum", MCHandleExportImageToAlbum, nil},
    {true, "mobileExportImageToAlbum", MCHandleExportImageToAlbum, nil},
    
    {false, "iphoneSetRedrawInterval", MCHandleSetRedrawInterval, nil},
    
    // MW-2012-02-15: [[ Bug 9985 ]] Control whether the autorotation animation happens
    //   or not.
    {false, "iphoneSetAnimateAutorotation", MCHandleSetAnimateAutorotation, nil},
    
    {false, "mobileCanMakePurchase", MCHandleCanMakePurchase, nil},
    {false, "mobileEnablePurchaseUpdates", MCHandleEnablePurchaseUpdates, nil},
    {false, "mobileDisablePurchaseUpdates", MCHandleDisablePurchaseUpdates, nil},
    {false, "mobileRestorePurchases", MCHandleRestorePurchases, nil},
    {false, "mobilePurchases", MCHandlePurchaseList, nil},
    {false, "mobilePurchaseCreate", MCHandlePurchaseCreate, nil},
    {false, "mobilePurchaseState", MCHandlePurchaseState, nil},
    {false, "mobilePurchaseError", MCHandlePurchaseError, nil},
    {false, "mobilePurchaseGet", MCHandlePurchaseGet, nil},
    {false, "mobilePurchaseSet", MCHandlePurchaseSet, nil},
    {false, "mobilePurchaseSendRequest", MCHandlePurchaseSendRequest, nil},
    {false, "mobilePurchaseConfirmDelivery", MCHandlePurchaseConfirmDelivery, nil},
    
    {false, "mobileStoreCanMakePurchase", MCHandleCanMakePurchase, nil},
    {false, "mobileStoreEnablePurchaseUpdates", MCHandleEnablePurchaseUpdates, nil},
	{false, "mobileStoreDisablePurchaseUpdates", MCHandleDisablePurchaseUpdates, nil},
    {false, "mobileStoreRestorePurchases", MCHandleRestorePurchases, nil},
    {false, "mobileStoreMakePurchase", MCHandleMakePurchase, nil},
    {false, "mobileStoreConfirmPurchase", MCHandleConfirmPurchase, nil},
    {false, "mobileStoreProductProperty", MCHandleGetPurchaseProperty, nil},
    {false, "mobileStoreSetProductType", MCHandleProductSetType, nil},
    {false, "mobileStoreRequestProductDetails", MCHandleRequestProductDetails, nil},
    {false, "mobileStoreConsumePurchase", MCHandleConsumePurchase, nil},
    {false, "mobileStorePurchasedProducts", MCHandleGetPurchases, nil},
    {false, "mobileStorePurchaseError", MCHandlePurchaseError, nil},
    //{false, "mobileGetPurchases", MCHandlePurchaseList, nil},

    {false, "iphoneRequestProductDetails", MCHandleRequestProductDetails, nil},
    
    {true, "mobilePickContact", MCHandlePickContact, nil},       // ABPeoplePickerNavigationController
    {true, "mobileShowContact", MCHandleShowContact, nil},       // ABPersonViewController
    {true, "mobileGetContactData", MCHandleGetContactData, nil}, // ABNewPersonViewController
    {true, "mobileUpdateContact", MCHandleUpdateContact, nil},   // ABUnknownPersonViewController
    {true, "mobileCreateContact", MCHandleCreateContact, nil},
    {false, "mobileAddContact", MCHandleAddContact, nil},
    {false, "mobileFindContact", MCHandleFindContact, nil},
    {false, "mobileRemoveContact", MCHandleRemoveContact, nil},
    
    {false, "iphoneSetDoNotBackupFile", MCHandleFileSetDoNotBackup, nil},
    {false, "iphoneDoNotBackupFile", MCHandleFileGetDoNotBackup, nil},
    {false, "iphoneSetFileDataProtection", MCHandleFileSetDataProtection, nil},
    {false, "iphoneFileDataProtection", MCHandleFileGetDataProtection, nil},
    
    {true, "mobileShowEvent", MCHandleShowEvent, nil},                     // ???                      // UI
    {false, "mobileGetEventData", MCHandleGetEventData, nil},               // get calendar data for
    {true, "mobileCreateEvent", MCHandleCreateEvent, nil},                 // create event in calendar // UI
    {true, "mobileUpdateEvent", MCHandleUpdateEvent, nil},                 // edit calendar event      // UI
    {false, "mobileAddEvent", MCHandleAddEvent, nil},                       // create calendar entry
    {false, "mobileGetCalendars", MCHandleGetCalendarsEvent, nil}, // create reoccurring calendar entry
    {false, "mobileFindEvent", MCHandleFindEvent, nil},                     // get calendar entry
    {false, "mobileRemoveEvent", MCHandleRemoveEvent, nil},
    
    // MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
    {false, "iphoneSetAudioCategory", MCHandleSetAudioCategory, nil},
    {false, "mobileSetAudioCategory", MCHandleSetAudioCategory, nil},
    
    // MW-2013-05-30: [[ RemoteControl ]] Support for iOS 'remote controls' and metadata display.
    {false, "iphoneEnableRemoteControl", MCHandleEnableRemoteControl, nil},
    {false, "iphoneDisableRemoteControl", MCHandleDisableRemoteControl, nil},
    {false, "iphoneRemoteControlEnabled", MCHandleRemoteControlEnabled, nil},
    {false, "iphoneSetRemoteControlDisplay", MCHandleSetRemoteControlDisplay, nil},
    
    {nil, nil, nil}
}
#endif
//////////////////////////////////////////////////////////////////////////////
//// Moved to mblhandlers.cpp
//typedef Exec_stat (*MCPlatformMessageHandler)(void *context, MCParameter *parameters);
//
//// MW-2012-08-06: [[ Fibers ]] If 'waitable' is true it means the handler must
////   be run on the script fiber. Otherwise it is run on the system fiber (making
////   implementation easier).
//struct MCPlatformMessageSpec
//{
//	bool waitable;
//	const char *message;
//	MCPlatformMessageHandler handler;
//	void *context;
//};
//
//extern Exec_stat MCHandleRequestProductDetails(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleCanMakePurchase(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleEnablePurchaseUpdates(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleDisablePurchaseUpdates(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleRestorePurchases(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePurchaseList(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePurchaseCreate(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePurchaseState(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePurchaseError(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePurchaseSet(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePurchaseGet(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePurchaseSendRequest(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePurchaseConfirmDelivery(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePurchaseVerify(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleComposeTextMessage(void *, MCParameter *);
//extern Exec_stat MCHandleCanComposeTextMessage(void *, MCParameter *);
//
//extern Exec_stat MCHandleRevMail(void *, MCParameter *);
//extern Exec_stat MCHandleCanSendMail(void *, MCParameter *);
//extern Exec_stat MCHandleComposePlainMail(void *, MCParameter *);
//extern Exec_stat MCHandleComposeUnicodeMail(void *, MCParameter *);
//extern Exec_stat MCHandleComposeHtmlMail(void *, MCParameter *);
//
//extern Exec_stat MCHandlePickPhoto(void *context, MCParameter *parameters);
//
//extern Exec_stat MCHandleStartTrackingSensor(void *, MCParameter *);
//extern Exec_stat MCHandleStopTrackingSensor(void *, MCParameter *);
//extern Exec_stat MCHandleSensorReading(void *, MCParameter *);
//extern Exec_stat MCHandleSensorAvailable(void *, MCParameter *);
//
//// MM-2012-02-11: Added support old style senseor syntax (iPhoneEnableAcceleromter etc)
//extern Exec_stat MCHandleCurrentLocation(void *, MCParameter *);
//extern Exec_stat MCHandleCurrentHeading(void *, MCParameter *);
//extern Exec_stat MCHandleAccelerometerEnablement(void *, MCParameter *);
//extern Exec_stat MCHandleCanTrackLocation(void *, MCParameter *);
//extern Exec_stat MCHandleCanTrackHeading(void *, MCParameter *);
//extern Exec_stat MCHandleLocationTrackingState(void *, MCParameter *);
//extern Exec_stat MCHandleHeadingTrackingState(void *, MCParameter *);
//extern Exec_stat MCHandleSetHeadingCalibrationTimeout(void *, MCParameter *);
//extern Exec_stat MCHandleHeadingCalibrationTimeout(void *, MCParameter *);
//
//extern Exec_stat MCHandleStartActivityIndicator(void *, MCParameter *);
//extern Exec_stat MCHandleStopActivityIndicator(void *, MCParameter *);
//extern Exec_stat MCHandleStartBusyIndicator(void *, MCParameter *);
//extern Exec_stat MCHandleStopBusyIndicator(void *, MCParameter *);
//
//extern Exec_stat MCHandlePickDate(void *, MCParameter *);
//extern Exec_stat MCHandlePickTime(void *, MCParameter *);
//extern Exec_stat MCHandlePickDateAndTime(void *, MCParameter *);
//
//extern Exec_stat MCHandlePick(void *, MCParameter *);
//extern Exec_stat MCHandleIPhonePickMedia(void *, MCParameter *);
//
//extern Exec_stat MCHandleBeep(void *, MCParameter *);
//extern Exec_stat MCHandleVibrate(void *, MCParameter *);
//
//extern Exec_stat MCHandleCreateLocalNotification(void *, MCParameter *); 
//extern Exec_stat MCHandleGetRegisteredNotifications(void *, MCParameter *); 
//extern Exec_stat MCHandleGetNotificationDetails(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleCancelLocalNotification(void *, MCParameter *); 
//extern Exec_stat MCHandleCancelAllLocalNotifications(void *, MCParameter *); 
//extern Exec_stat MCHandleGetNotificationBadgeValue(void *, MCParameter *); 
//extern Exec_stat MCHandleSetNotificationBadgeValue(void *, MCParameter *); 
//
//extern Exec_stat MCHandleControlCreate(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleControlDelete(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleControlSet(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleControlGet(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleControlDo(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleControlTarget(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleControlList(void *context, MCParameter *p_parameters);
//
//// MM-2012-09-02: Refactor multi channel sound into new soound channel module. 
//extern Exec_stat MCHandlePlaySoundOnChannel(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePausePlayingOnChannel(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleResumePlayingOnChannel(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleStopPlayingOnChannel(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleDeleteSoundChannel(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleSetSoundChannelVolume(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleSoundChannelVolume(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleSoundChannelStatus(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleSoundOnChannel(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleNextSoundOnChannel(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleSoundChannels(void *context, MCParameter *p_parameters);
//
//// MM-2012-02-22: Added support for ad management
//extern Exec_stat MCHandleAdRegister(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleAdCreate(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleAdDelete(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleAdGetVisible(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleAdSetVisible(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleAdGetTopLeft(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleAdSetTopLeft(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleAds(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandlePickContact(void *context, MCParameter *p_parameters);    // ABPeoplePickerNavigationController
//extern Exec_stat MCHandleShowContact(void *context, MCParameter *p_parameters);    // ABPersonViewController
//extern Exec_stat MCHandleGetContactData(void *context, MCParameter *p_parameters); // ABNewPersonViewController
//extern Exec_stat MCHandleUpdateContact(void *context, MCParameter *p_parameters);  // ABUnknownPersonViewController
//extern Exec_stat MCHandleCreateContact(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleRemoveContact(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleAddContact(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleFindContact(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleFileSetDoNotBackup(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleFileGetDoNotBackup(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleFileSetDataProtection(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleFileGetDataProtection(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleShowEvent(void *context, MCParameter *p_parameters);           // ???                      // UI
//extern Exec_stat MCHandleGetEventData(void *context, MCParameter *p_parameters);        // get calendar data for
//extern Exec_stat MCHandleCreateEvent(void *context, MCParameter *p_parameters);         // create event in calendar // UI
//extern Exec_stat MCHandleUpdateEvent(void *context, MCParameter *p_parameters);         // edit calendar event      // UI
//extern Exec_stat MCHandleAddEvent(void *context, MCParameter *p_parameters);            // create calendar entry
//extern Exec_stat MCHandleGetCalendarsEvent(void *context, MCParameter *p_parameters);   // get the names of the calendars
//extern Exec_stat MCHandleFindEvent(void *context, MCParameter *p_parameters);           // get calendar entry
//extern Exec_stat MCHandleRemoveEvent(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleGetDeviceToken(void *, MCParameter *);
//extern Exec_stat MCHandleGetLaunchUrl(void *, MCParameter *);
//
//extern Exec_stat MCHandleDeviceResolution(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleUseDeviceResolution(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleDeviceScale(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandlePixelDensity(void* p_context, MCParameter* p_parameters);
//
//extern Exec_stat MCHandleLockIdleTimer(void* p_context, MCParameter* p_parameters);
//extern Exec_stat MCHandleUnlockIdleTimer(void* p_context, MCParameter* p_parameters);
//extern Exec_stat MCHandleIdleTimerLocked(void* p_context, MCParameter* p_parameters);
//
//extern Exec_stat MCHandleSetStatusBarStyle(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleShowStatusBar(void* p_context, MCParameter* p_parameter);
//extern Exec_stat MCHandleHideStatusBar(void* p_context, MCParameter* p_parameter);
//extern Exec_stat MCHandleSetKeyboardType (void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleSetKeyboardReturnKey (void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandlePreferredLanguages(void* p_context, MCParameter* p_parameter);
//extern Exec_stat MCHandleCurrentLocale(void* p_context, MCParameter* p_parameter);
//
//extern Exec_stat MCHandleClearTouches(void* p_context, MCParameter* p_parameter);
//
//extern Exec_stat MCHandleSystemIdentifier(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleApplicationIdentifier(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleSetReachabilityTarget(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleReachabilityTarget(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleLibUrlDownloadToFile(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleExportImageToAlbum(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleSetRedrawInterval(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleSetAnimateAutorotation(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleBuildInfo(void *context, MCParameter *p_parameters);
//
//// MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
//extern Exec_stat MCHandleSetAudioCategory(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleDeviceOrientation(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleOrientation(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleAllowedOrientations(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleSetAllowedOrientations(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleLockOrientation(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleUnlockOrientation(void *context, MCParameter *p_parameters);
//extern Exec_stat MCHandleRotateInterface(void *context, MCParameter *p_parameters);
//
//extern Exec_stat MCHandleCameraFeatures(void *context, MCParameter *p_parameters);
//
//static MCPlatformMessageSpec s_platform_messages[] =
//{
//    // MM-2012-02-22: Added support for ad management
//    {false, "mobileAdRegister", MCHandleAdRegister, nil},
//    {false, "mobileAdCreate", MCHandleAdCreate, nil},
//    {false, "mobileAdDelete", MCHandleAdDelete, nil},
//    {false, "mobileAdGetVisible", MCHandleAdGetVisible, nil},
//    {false, "mobileAdSetVisible", MCHandleAdSetVisible, nil},
//    {false, "mobileAdGetTopLeft", MCHandleAdGetTopLeft, nil},
//    {false, "mobileAdSetTopLeft", MCHandleAdSetTopLeft, nil},
//    {false, "mobileAds", MCHandleAds, nil},
//    {false, "iphoneAdRegister", MCHandleAdRegister, nil},
//    {false, "iphoneAdCreate", MCHandleAdCreate, nil},
//    {false, "iphoneAdDelete", MCHandleAdDelete, nil},
//    {false, "iphoneAdGetVisible", MCHandleAdGetVisible, nil},
//    {false, "iphoneAdSetVisible", MCHandleAdSetVisible, nil},
//    {false, "iphoneAdGetTopLeft", MCHandleAdGetTopLeft, nil},
//    {false, "iphoneAdSetTopLeft", MCHandleAdSetTopLeft, nil},
//    {false, "iphoneAds", MCHandleAds, nil},
//    
//    {false, "mobileCreateLocalNotification", MCHandleCreateLocalNotification, nil},
//    {false, "mobileGetRegisteredNotifications", MCHandleGetRegisteredNotifications, nil},
//    {false, "mobileGetNotificationDetails", MCHandleGetNotificationDetails, nil},
//    {false, "mobileCancelLocalNotification", MCHandleCancelLocalNotification, nil},
//    {false, "mobileCancelAllLocalNotifications", MCHandleCancelAllLocalNotifications, nil},
//    {false, "iphoneCreateLocalNotification", MCHandleCreateLocalNotification, nil},
//    {false, "iphoneGetRegisteredNotifications", MCHandleGetRegisteredNotifications, nil},
//    {false, "iphoneCancelLocalNotification", MCHandleCancelLocalNotification, nil},
//    {false, "iphoneCancelAllLocalNotifications", MCHandleCancelAllLocalNotifications, nil},
//    
//    {false, "iphoneGetNotificationBadgeValue", MCHandleGetNotificationBadgeValue, nil},
//    {false, "iphoneSetNotificationBadgeValue", MCHandleSetNotificationBadgeValue, nil},
//    
//    {false, "mobileGetDeviceToken", MCHandleGetDeviceToken, nil},
//    {false, "mobileGetLaunchUrl", MCHandleGetLaunchUrl, nil},
//    {false, "iphoneGetDeviceToken", MCHandleGetDeviceToken, nil},
//    {false, "iphoneGetLaunchUrl", MCHandleGetLaunchUrl, nil},
//    
//	{false, "iphoneActivityIndicatorStart", MCHandleStartActivityIndicator, nil},
//	{false, "iphoneActivityIndicatorStop", MCHandleStopActivityIndicator, nil},
//    
//    {false, "mobileBusyIndicatorStart", MCHandleStartBusyIndicator, nil},
//    {false, "mobileBusyIndicatorStop", MCHandleStopBusyIndicator, nil},
//    {false, "iphoneBusyIndicatorStart", MCHandleStartBusyIndicator, nil},
//    {false, "iphoneBusyIndicatorStop", MCHandleStopBusyIndicator, nil},
//    
//    {false, "mobileBeep", MCHandleBeep, nil},
//    {true, "mobileVibrate", MCHandleVibrate, nil},
//    {false, "iphoneBeep", MCHandleBeep, nil},
//    {true, "iphoneVibrate", MCHandleVibrate, nil},
//    
//	{true, "iphonePickPhoto", MCHandlePickPhoto, nil},
//	{true, "iphonePickMedia", MCHandleIPhonePickMedia, nil},
//	{true, "mobilePickMedia", MCHandleIPhonePickMedia, nil},
////	{false, "iphoneCameraFeatures", MCHandleCameraFeatures, nil},
//	{true, "mobilePickPhoto", MCHandlePickPhoto, nil},
////	{false, "mobileCameraFeatures", MCHandleCameraFeatures, nil},
//    
//	{false, "iphoneDeviceOrientation", MCHandleDeviceOrientation, nil},
//	{false, "mobileDeviceOrientation", MCHandleDeviceOrientation, nil},
//	
//	{false, "iphoneOrientation", MCHandleOrientation, nil},
//	{false, "iphoneAllowedOrientations", MCHandleAllowedOrientations, nil},
//	{false, "iphoneSetAllowedOrientations", MCHandleSetAllowedOrientations, nil},
//	{false, "iphoneOrientationLocked", MCHandleOrientationLocked, nil},
//	{false, "iphoneLockOrientation", MCHandleLockOrientation, nil},
//	{false, "iphoneUnlockOrientation", MCHandleUnlockOrientation, nil},
//	{false, "mobileOrientation", MCHandleOrientation, nil},
//	{false, "mobileAllowedOrientations", MCHandleAllowedOrientations, nil},
//	{false, "mobileSetAllowedOrientations", MCHandleSetAllowedOrientations, nil},
//	{false, "mobileOrientationLocked", MCHandleOrientationLocked, nil},
//	{false, "mobileLockOrientation", MCHandleLockOrientation, nil},
//	{false, "mobileUnlockOrientation", MCHandleUnlockOrientation, nil},
//	
//	{false, "iphoneDeviceResolution", MCHandleDeviceResolution, nil},
//	{false, "iphoneUseDeviceResolution", MCHandleUseDeviceResolution, nil},
//	{false, "iphoneDeviceScale", MCHandleDeviceScale, nil},
//    {false, "mobilePixelDensity", MCHandlePixelDensity, nil},
//    
//	{false, "mobileBuildInfo", MCHandleBuildInfo, nil},
//    
//    {false, "mobileStartTrackingSensor", MCHandleStartTrackingSensor, nil},
//    {false, "mobileStopTrackingSensor", MCHandleStopTrackingSensor, nil},
//    {false, "mobileSensorReading", MCHandleSensorReading, nil},
//    {false, "mobileSensorAvailable", MCHandleSensorAvailable, nil},	
//    
//    // MM-2012-02-11: Added support old style senseor syntax (iPhoneEnableAcceleromter etc)
//	/* DEPRECATED */ {false, "iphoneCanTrackLocation", MCHandleCanTrackLocation, nil},
//	/* DEPRECATED */ {false, "iphoneStartTrackingLocation", MCHandleLocationTrackingState, (void *)true},
//	/* DEPRECATED */ {false, "iphoneStopTrackingLocation", MCHandleLocationTrackingState, (void *)false},
//	/* DEPRECATED */ {false, "iphoneCurrentLocation", MCHandleCurrentLocation, nil},
//    /* DEPRECATED */ {false, "mobileCanTrackLocation", MCHandleCanTrackLocation, nil},
//    /* DEPRECATED */ {false, "mobileStartTrackingLocation", MCHandleLocationTrackingState, (void *)true},
//	/* DEPRECATED */ {false, "mobileStopTrackingLocation", MCHandleLocationTrackingState, (void *)false},
//	/* DEPRECATED */ {false, "mobileCurrentLocation", MCHandleCurrentLocation, nil},
//	
//	/* DEPRECATED */ {false, "iphoneCanTrackHeading", MCHandleCanTrackHeading, nil},
//	/* DEPRECATED */ {false, "iphoneStartTrackingHeading", MCHandleHeadingTrackingState, (void *)true},
//	/* DEPRECATED */ {false, "iphoneStopTrackingHeading", MCHandleHeadingTrackingState, (void *)false},
//	/* DEPRECATED */ {false, "iphoneCurrentHeading", MCHandleCurrentHeading, nil},
//	{false, "iphoneSetHeadingCalibrationTimeout", MCHandleSetHeadingCalibrationTimeout, nil},
//	{false, "iphoneHeadingCalibrationTimeout", MCHandleHeadingCalibrationTimeout, nil},
//    /* DEPRECATED */ {false, "mobileCanTrackHeading", MCHandleCanTrackHeading, nil},
//    /* DEPRECATED */ {false, "mobileStartTrackingHeading", MCHandleHeadingTrackingState, (void *)true},
//	/* DEPRECATED */ {false, "mobileStopTrackingHeading", MCHandleHeadingTrackingState, (void *)false},
//	/* DEPRECATED */ {false, "mobileCurrentHeading", MCHandleCurrentHeading, nil},
//    
//    /* DEPRECATED */ {false, "iphoneEnableAccelerometer", MCHandleAccelerometerEnablement, (void *)true},
//	/* DEPRECATED */ {false, "iphoneDisableAccelerometer", MCHandleAccelerometerEnablement, (void *)false},
//	/* DEPRECATED */ {false, "mobileEnableAccelerometer", MCHandleAccelerometerEnablement, (void *)true},
//	/* DEPRECATED */ {false, "mobileDisableAccelerometer", MCHandleAccelerometerEnablement, (void *)false},
//	
//    {true, "iphoneComposeTextMessage", MCHandleComposeTextMessage, nil},
//    {false, "iphoneCanComposeTextMessage", MCHandleCanComposeTextMessage, nil},
//    {true, "mobileComposeTextMessage", MCHandleComposeTextMessage, nil},
//    {false, "mobileCanComposeTextMessage", MCHandleCanComposeTextMessage, nil},
//    
//	{true, "revMail", MCHandleRevMail, nil},
//	
//	{false, "iphoneCanSendMail", MCHandleCanSendMail, nil},
//	{true, "iphoneComposeMail", MCHandleComposePlainMail, nil},
//	{true, "iphoneComposeUnicodeMail", MCHandleComposeUnicodeMail, nil},
//	{true, "iphoneComposeHtmlMail", MCHandleComposeHtmlMail, nil},
//    {false, "mobileCanSendMail", MCHandleCanSendMail, nil},
//	{true, "mobileComposeMail", MCHandleComposePlainMail, nil},
//	{true, "mobileComposeUnicodeMail", MCHandleComposeUnicodeMail, nil},
//	{true, "mobileComposeHtmlMail", MCHandleComposeHtmlMail, nil},
//	
//	{true, "libUrlDownloadToFile", MCHandleLibUrlDownloadToFile, nil},
//	
//	{false, "iphoneSetStatusBarStyle", MCHandleSetStatusBarStyle, nil},
//	{false, "iphoneShowStatusBar", MCHandleShowStatusBar, nil},
//	{false, "iphoneHideStatusBar", MCHandleHideStatusBar, nil},
//    {false, "mobileSetStatusBarStyle", MCHandleSetStatusBarStyle, nil},
//	{false, "mobileShowStatusBar", MCHandleShowStatusBar, nil},
//	{false, "mobileHideStatusBar", MCHandleHideStatusBar, nil},
//	
//	{true, "iphonePick", MCHandlePick, nil},
//	{true, "iphonePickDate", MCHandlePickDate, nil},
//    
//    {true, "mobilePickDate", MCHandlePickDate, nil},
//    {true, "mobilePickTime", MCHandlePickTime, nil},
//    {true, "mobilePickDateAndTime", MCHandlePickDateAndTime, nil},
//    {true, "mobilePick", MCHandlePick, nil},
//	
//	{false, "iphoneSetKeyboardType", MCHandleSetKeyboardType, nil},
//	{false, "iphoneSetKeyboardReturnKey", MCHandleSetKeyboardReturnKey, nil},
//	{false, "mobileSetKeyboardType", MCHandleSetKeyboardType, nil},
//	
//	{false, "iphoneControlCreate", MCHandleControlCreate, nil},
//	{false, "iphoneControlDelete", MCHandleControlDelete, nil},
//	{false, "iphoneControlSet", MCHandleControlSet, nil},
//	{false, "iphoneControlGet", MCHandleControlGet, nil},
//	{false, "iphoneControlDo", MCHandleControlDo, nil},
//	{false, "iphoneControlTarget", MCHandleControlTarget, nil},
//	{false, "iphoneControls", MCHandleControlList, nil},
//	{false, "mobileControlCreate", MCHandleControlCreate, nil},
//	{false, "mobileControlDelete", MCHandleControlDelete, nil},
//	{false, "mobileControlSet", MCHandleControlSet, nil},
//	{false, "mobileControlGet", MCHandleControlGet, nil},
//	{false, "mobileControlDo", MCHandleControlDo, nil},
//	{false, "mobileControlTarget", MCHandleControlTarget, nil},
//	{false, "mobileControls", MCHandleControlList, nil},
//	
//	{false, "iphonePreferredLanguages", MCHandlePreferredLanguages, nil},
//	{false, "mobilePreferredLanguages", MCHandlePreferredLanguages, nil},
//	{false, "iphoneCurrentLocale", MCHandleCurrentLocale, nil},
//	{false, "mobileCurrentLocale", MCHandleCurrentLocale, nil},
//	
//	{false, "iphonePlaySoundOnChannel", MCHandlePlaySoundOnChannel, nil},
//	{false, "iphonePausePlayingOnChannel", MCHandlePausePlayingOnChannel},
//	{false, "iphoneResumePlayingOnChannel", MCHandleResumePlayingOnChannel},
//	{false, "iphoneStopPlayingOnChannel", MCHandleStopPlayingOnChannel, nil},
//	{false, "iphoneDeleteSoundChannel", MCHandleDeleteSoundChannel, nil},
//	{false, "iphoneSetSoundChannelVolume", MCHandleSetSoundChannelVolume, nil},
//	{false, "iphoneSoundChannelVolume", MCHandleSoundChannelVolume, nil},
//	{false, "iphoneSoundChannelStatus", MCHandleSoundChannelStatus, nil},
//	{false, "iphoneSoundOnChannel", MCHandleSoundOnChannel, nil},
//	{false, "iphoneNextSoundOnChannel", MCHandleNextSoundOnChannel, nil},
//	{false, "iphoneSoundChannels", MCHandleSoundChannels, nil},
//    
//    // MM-2012-09-02: Add support for mobile* multi channel sound syntax
//    {false, "mobilePlaySoundOnChannel", MCHandlePlaySoundOnChannel, nil},
//	{false, "mobilePausePlayingOnChannel", MCHandlePausePlayingOnChannel},
//	{false, "mobileResumePlayingOnChannel", MCHandleResumePlayingOnChannel},
//	{false, "mobileStopPlayingOnChannel", MCHandleStopPlayingOnChannel, nil},
//	{false, "mobileDeleteSoundChannel", MCHandleDeleteSoundChannel, nil},
//	{false, "mobileSetSoundChannelVolume", MCHandleSetSoundChannelVolume, nil},
//	{false, "mobileSoundChannelVolume", MCHandleSoundChannelVolume, nil},
//	{false, "mobileSoundChannelStatus", MCHandleSoundChannelStatus, nil},
//	{false, "mobileSoundOnChannel", MCHandleSoundOnChannel, nil},
//	{false, "mobileNextSoundOnChannel", MCHandleNextSoundOnChannel, nil},
//	{false, "mobileSoundChannels", MCHandleSoundChannels, nil},
//	
//	{false, "iphoneLockIdleTimer", MCHandleLockIdleTimer, nil},
//	{false, "mobileLockIdleTimer", MCHandleLockIdleTimer, nil},
//	{false, "iphoneUnlockIdleTimer", MCHandleUnlockIdleTimer, nil},
//	{false, "mobileUnlockIdleTimer", MCHandleUnlockIdleTimer, nil},
//	{false, "iphoneIdleTimerLocked", MCHandleIdleTimerLocked, nil},
//	{false, "mobileIdleTimerLocked", MCHandleIdleTimerLocked, nil},
//	
//	{false, "iphoneClearTouches", MCHandleClearTouches, nil},
//	{false, "mobileClearTouches", MCHandleClearTouches, nil},
//	
//	{false, "iphoneSystemIdentifier", MCHandleSystemIdentifier, nil},
//	{false, "iphoneApplicationIdentifier", MCHandleApplicationIdentifier, nil},
//	
//	{false, "iphoneSetReachabilityTarget", MCHandleSetReachabilityTarget, nil},
//	{false, "iphoneReachabilityTarget", MCHandleReachabilityTarget, nil},
//	
//	{true, "iphoneExportImageToAlbum", MCHandleExportImageToAlbum, nil},
//	{true, "mobileExportImageToAlbum", MCHandleExportImageToAlbum, nil},
//    
//	{false, "iphoneSetRedrawInterval", MCHandleSetRedrawInterval, nil},
//	
//    // MW-2012-02-15: [[ Bug 9985 ]] Control whether the autorotation animation happens
//    //   or not.
//	{false, "iphoneSetAnimateAutorotation", MCHandleSetAnimateAutorotation, nil},
//	
//	{false, "mobileCanMakePurchase", MCHandleCanMakePurchase, nil},
//	{false, "mobileEnablePurchaseUpdates", MCHandleEnablePurchaseUpdates, nil},
//	{false, "mobileDisablePurchaseUpdates", MCHandleDisablePurchaseUpdates, nil},
//	{false, "mobileRestorePurchases", MCHandleRestorePurchases, nil},
//	{false, "mobilePurchases", MCHandlePurchaseList, nil},
//	{false, "mobilePurchaseCreate", MCHandlePurchaseCreate, nil},
//	{false, "mobilePurchaseState", MCHandlePurchaseState, nil},
//	{false, "mobilePurchaseError", MCHandlePurchaseError, nil},
//	{false, "mobilePurchaseGet", MCHandlePurchaseGet, nil},
//	{false, "mobilePurchaseSet", MCHandlePurchaseSet, nil},
//	{false, "mobilePurchaseSendRequest", MCHandlePurchaseSendRequest, nil},
//	{false, "mobilePurchaseConfirmDelivery", MCHandlePurchaseConfirmDelivery, nil},
//    {false, "mobilePurchaseVerify", MCHandlePurchaseVerify, nil},
//    
//    {false, "iphoneRequestProductDetails", MCHandleRequestProductDetails, nil},
//    
//    {true, "mobilePickContact", MCHandlePickContact, nil},       // ABPeoplePickerNavigationController
//    {true, "mobileShowContact", MCHandleShowContact, nil},       // ABPersonViewController
//    {true, "mobileGetContactData", MCHandleGetContactData, nil}, // ABNewPersonViewController
//    {true, "mobileUpdateContact", MCHandleUpdateContact, nil},   // ABUnknownPersonViewController
//    {true, "mobileCreateContact", MCHandleCreateContact, nil},
//    {false, "mobileAddContact", MCHandleAddContact, nil},
//    {false, "mobileFindContact", MCHandleFindContact, nil},
//    {false, "mobileRemoveContact", MCHandleRemoveContact, nil},
//    
//    {false, "iphoneSetDoNotBackupFile", MCHandleFileSetDoNotBackup, nil},
//    {false, "iphoneDoNotBackupFile", MCHandleFileGetDoNotBackup, nil},
//    {false, "iphoneSetFileDataProtection", MCHandleFileSetDataProtection, nil},
//    {false, "iphoneFileDataProtection", MCHandleFileGetDataProtection, nil},
//    
//    {true, "mobileShowEvent", MCHandleShowEvent, nil},                     // ???                      // UI
//    {false, "mobileGetEventData", MCHandleGetEventData, nil},               // get calendar data for
//    {true, "mobileCreateEvent", MCHandleCreateEvent, nil},                 // create event in calendar // UI
//    {true, "mobileUpdateEvent", MCHandleUpdateEvent, nil},                 // edit calendar event      // UI
//    {false, "mobileAddEvent", MCHandleAddEvent, nil},                       // create calendar entry
//    {false, "mobileGetCalendars", MCHandleGetCalendarsEvent, nil}, // create reoccurring calendar entry
//    {false, "mobileFindEvent", MCHandleFindEvent, nil},                     // get calendar entry
//    {false, "mobileRemoveEvent", MCHandleRemoveEvent, nil},
//
//    // MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
//    {false, "iphoneSetAudioCategory", MCHandleSetAudioCategory, nil},
//    {false, "mobileSetAudioCategory", MCHandleSetAudioCategory, nil},
//    
//	{nil, nil, nil}
//
//};
//
//struct handle_context_t
//{
//	MCPlatformMessageHandler handler;
//	void *context;
//	MCParameter *parameters;
//	Exec_stat result;
//};
//
//static void invoke_platform(void *p_context)
//{
//	handle_context_t *ctxt;
//	ctxt = (handle_context_t *)p_context;
//	ctxt -> result = ctxt -> handler(ctxt -> context, ctxt -> parameters);
//}
//
//extern void MCIPhoneCallOnMainFiber(void (*)(void *), void *);
//
//Exec_stat MCHandlePlatformMessage(Handler_type p_type, const MCString& p_message, MCParameter *p_parameters)
//{
//	for(uint32_t i = 0; s_platform_messages[i] . message != nil; i++)
//		if (p_message == s_platform_messages[i] . message)
//		{
//			// MW-2012-07-31: [[ Fibers ]] If the method doesn't need script / wait, then
//			//   jump to the main fiber for it.
//			if (!s_platform_messages[i] . waitable)
//			{
//				handle_context_t ctxt;
//				ctxt . handler = s_platform_messages[i] . handler;
//				ctxt . context = s_platform_messages[i] . context;
//				ctxt . parameters = p_parameters;
//				MCIPhoneCallOnMainFiber(invoke_platform, &ctxt);
//				return ctxt . result;
//			}
//			
//			// Execute the method as normal, in this case the method will have to jump
//			// to the main fiber to do any system stuff.
//			return s_platform_messages[i] . handler(s_platform_messages[i] . context, p_parameters);
//		}
//	
//	return ES_NOT_HANDLED;
//}
