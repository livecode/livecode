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

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
#include "util.h"
#include "globals.h"
#include "osspec.h"

#include "prefix.h"

#ifdef TARGET_SUBPLATFORM_IPHONE
#import <CoreText/CoreText.h>
#else
#import <ApplicationServices/ApplicationServices.h>
#endif

static void *coretext_font_create_with_name_and_size(const char *p_name, uint32_t p_size)
{
	CFStringRef t_name;
	t_name = CFStringCreateWithCString(NULL, p_name, kCFStringEncodingMacRoman);
	
	CTFontRef t_font;
    t_font = NULL;
    
    if (t_name != NULL)
    {
        t_font = CTFontCreateWithName(t_name, p_size, NULL);
        CFRelease(t_name);
    }
    
	return (void *)t_font;
}

void *coretext_font_create_with_name_size_and_style(const char *p_name, uint32_t p_size, bool p_bold, bool p_italic)
{
    /*bool t_success;
    t_success = true;
    
    CFStringRef t_name;
    t_name = NULL;
    if (t_success)
    {
        t_name = CFStringCreateWithCString(NULL, p_name, kCFStringEncodingMacRoman);
        t_success = t_name != NULL;
    }

    CFDictionaryRef t_styles;
    t_styles = NULL;
    if (t_success)
    {
        CTFontSymbolicTraits t_symbolic_traits;
        t_symbolic_traits = 0;
        if (p_bold)
            t_symbolic_traits |= kCTFontBoldTrait;
        if (p_italic)
            t_symbolic_traits |= kCTFontItalicTrait;
        
        CFNumberRef t_cfnumber_symbolic_traits;
        t_cfnumber_symbolic_traits = NULL;
        t_cfnumber_symbolic_traits = CFNumberCreate(NULL, kCFNumberIntType, &t_symbolic_traits);
        
        if (t_cfnumber_symbolic_traits != NULL)
        {
            CFStringRef t_keys[] =
            {
                kCTFontSymbolicTrait,
            };
            CFTypeRef t_values[] = {
                t_cfnumber_symbolic_traits,
            };
            t_styles = CFDictionaryCreate(NULL,
                                          (const void **)&t_keys, (const void **)&t_values,
                                          sizeof(t_keys) / sizeof(t_keys[0]),
                                          &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            CFRelease(t_cfnumber_symbolic_traits);
        }
        
        t_success = t_styles != NULL;
    }
    
    CFDictionaryRef t_attributes;
    t_attributes = NULL;
    if (t_success)
    {
        CFStringRef t_keys[] =
        {
            kCTFontNameAttribute,
            kCTFontTraitsAttribute,
        };
        CFTypeRef t_values[] = {
            t_name,
            t_styles,
        };
        t_attributes = CFDictionaryCreate(NULL,
                                          (const void **)&t_keys, (const void **)&t_values,
                                          sizeof(t_keys) / sizeof(t_keys[0]),
                                          &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        t_success = t_attributes != NULL;
    }
    
    CTFontDescriptorRef t_descriptor;
    t_descriptor = NULL;
    if (t_success)
    {
        t_descriptor = CTFontDescriptorCreateWithAttributes(t_attributes);
        t_success = t_descriptor != NULL;
    }
    
    CTFontRef t_font;
    t_font = NULL;
    if (t_success)
        t_font = CTFontCreateWithFontDescriptor(t_descriptor, p_size, NULL);
    
    if (t_descriptor != NULL)
        CFRelease(t_descriptor);
    if (t_attributes != NULL)
        CFRelease(t_attributes);
    if (t_styles != NULL)
        CFRelease(t_styles);
    if (t_name != NULL)
        CFRelease(t_name);
    
    return (void *)t_font;*/
    
    CTFontRef t_base_font;
    t_base_font = (CTFontRef)coretext_font_create_with_name_and_size(p_name, p_size);
	
	CTFontRef t_font;
	t_font = NULL;
    
	if (p_bold && p_italic)
		t_font = CTFontCreateCopyWithSymbolicTraits(t_base_font, p_size, NULL, kCTFontBoldTrait | kCTFontItalicTrait, kCTFontBoldTrait | kCTFontItalicTrait);
	
	if (t_font == NULL && p_bold)
		t_font = CTFontCreateCopyWithSymbolicTraits(t_base_font, p_size, NULL, kCTFontBoldTrait, kCTFontBoldTrait);
	
	if (t_font == NULL && p_italic)
		t_font = CTFontCreateCopyWithSymbolicTraits(t_base_font, p_size, NULL, kCTFontItalicTrait, kCTFontItalicTrait);
    
	if (t_font == NULL)
		t_font = t_base_font;
     
	if (t_font != t_base_font && t_base_font != NULL)
		CFRelease(t_base_font);
    
	return (void *)t_font;
}

void coretext_font_destroy(void *p_font)
{
    if (p_font != NULL)
        CFRelease((CTFontRef) p_font);
}

void coretext_font_get_metrics(void *p_font, float& r_ascent, float& r_descent)
{
	r_ascent = CTFontGetAscent((CTFontRef) p_font);
	r_descent = CTFontGetDescent((CTFontRef) p_font);
}

void coretext_get_font_names(MCExecPoint &ep)
{
    ep . clear();
    
    CTFontCollectionRef t_fonts;
    t_fonts = CTFontCollectionCreateFromAvailableFonts(NULL);
    
    CFArrayRef t_descriptors;
    t_descriptors = CTFontCollectionCreateMatchingFontDescriptors(t_fonts);
    
    char t_cstring_font_name[256];
    
    for(CFIndex i = 0; i < CFArrayGetCount(t_descriptors); i++)
    {
        CTFontDescriptorRef t_font;
		t_font = (CTFontDescriptorRef)CFArrayGetValueAtIndex(t_descriptors, i);
		
        CFStringRef t_font_name;
        t_font_name = (CFStringRef)CTFontDescriptorCopyAttribute(t_font, kCTFontDisplayNameAttribute);
        
		if (CFStringGetCString(t_font_name, t_cstring_font_name, 256, kCFStringEncodingMacRoman) &&
            t_cstring_font_name[0] != '%' && t_cstring_font_name[0] != '.')
			ep . concatcstring(t_cstring_font_name, EC_RETURN, i == 0);
        
        CFRelease(t_font_name);
    }
    
    if (t_descriptors != NULL)
        CFRelease(t_descriptors);
    if (t_fonts != NULL)
        CFRelease(t_fonts);
}

void core_text_get_font_styles(const char *p_name, uint32_t p_size, MCExecPoint &ep)
{
	ep . clear();
    
    CTFontRef t_font_family;
    t_font_family = (CTFontRef)coretext_font_create_with_name_and_size(p_name, p_size);
	
	if (t_font_family != NULL)
	{
		CTFontSymbolicTraits t_traits;
		t_traits = CTFontGetSymbolicTraits(t_font_family);
		
		ep . concatcstring("plain", EC_RETURN, true);
        
		if (t_traits & kCTFontBoldTrait)
			ep . concatcstring("bold", EC_RETURN, false);
		
		if (t_traits & kCTFontItalicTrait)
			ep . concatcstring("italic", EC_RETURN, false);
        
		if (t_traits & kCTFontBoldTrait && t_traits & kCTFontItalicTrait)
			ep . concatcstring("bold-italic", EC_RETURN, false);
	}
	
    if (t_font_family != NULL)
        CFRelease(t_font_family);
}