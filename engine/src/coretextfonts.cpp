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

static void *coretext_font_create_with_name_and_size(MCStringRef p_name, uint32_t p_size)
{
	/*CFStringRef t_name;
	t_name = CFStringCreateWithCString(NULL, p_name, kCFStringEncodingMacRoman);
	
	CTFontRef t_font;
    t_font = NULL;
    
    if (t_name != NULL)
    {
        t_font = CTFontCreateWithName(t_name, p_size, NULL);
        CFRelease(t_name);
    }
    
	return (void *)t_font;*/
    
    bool t_success;
    t_success = true;
    
    MCAutoStringRefAsCFString t_cf_name;
    t_success = t_cf_name . Lock(p_name);
    
    CFDictionaryRef t_attributes;
    t_attributes = NULL;
    if (t_success)
    {
        // MW-2014-07-23: [[ Bug 12426 ]] Only specify the 'name' attribute in the font descriptor
        //   otherwise things don't work correctly on iOS. It seems going via a descriptor stops
        //   the warning on 10.9.
        //
        // Updated the font creation routine to use font descriptors. Though CTFontCreateWithName worked,
        // it logged warnings (on 10.9) whenever it was passed a non-postscript font name. Hopefully using
        // the display name first in the descriptor will remove the warnings but still keep the fall back behaviour.
        CFStringRef t_keys[] =
        {
//            kCTFontDisplayNameAttribute,
            kCTFontNameAttribute,
//            kCTFontFamilyNameAttribute,
        };
        CFTypeRef t_values[] = {
            *t_cf_name,
            *t_cf_name,
            *t_cf_name,
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
    
    CFStringRef t_font_name = CTFontCopyFullName(t_font);
    
    if (t_descriptor != NULL)
        CFRelease(t_descriptor);
    if (t_attributes != NULL)
        CFRelease(t_attributes);
    
    return (void *)t_font;
}

void *coretext_font_create_with_name_size_and_style(MCStringRef p_name, uint32_t p_size, bool p_bold, bool p_italic)
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

bool coretext_font_destroy(void *p_font)
{
    if (p_font != NULL)
        CFRelease((CTFontRef) p_font);
    
    return true;
}


bool coretext_font_get_metrics(void *p_font, float& r_ascent, float& r_descent, float& r_leading, float& r_xheight)
{
    r_ascent = CTFontGetAscent((CTFontRef) p_font);
    r_descent = CTFontGetDescent((CTFontRef) p_font);
    r_leading = CTFontGetLeading((CTFontRef) p_font);
    r_xheight = CTFontGetXHeight((CTFontRef) p_font);

    return true;
}

bool coretext_get_font_names(MCListRef &r_names)
{
    CTFontCollectionRef t_fonts;
    t_fonts = CTFontCollectionCreateFromAvailableFonts(NULL);
    
    CFArrayRef t_descriptors;
    t_descriptors = CTFontCollectionCreateMatchingFontDescriptors(t_fonts);
    
    MCAutoListRef t_names;
    MCListCreateMutable('\n', &t_names);
    
    char t_cstring_font_name[256];
    bool t_success;
    t_success = true;
    
    for(CFIndex i = 0; t_success && i < CFArrayGetCount(t_descriptors); i++)
    {
        CTFontDescriptorRef t_font;
		t_font = (CTFontDescriptorRef)CFArrayGetValueAtIndex(t_descriptors, i);
		
        CFStringRef t_font_name;
        t_font_name = (CFStringRef)CTFontDescriptorCopyAttribute(t_font, kCTFontDisplayNameAttribute);
        
		if (t_font_name != NULL && CFStringGetCString(t_font_name, t_cstring_font_name, 256, kCFStringEncodingMacRoman) &&
                t_cstring_font_name[0] != '%' && t_cstring_font_name[0] != '.')
			t_success = MCListAppendCString(*t_names, t_cstring_font_name);
        
        if (t_font_name != NULL)
            CFRelease(t_font_name);
    }
    
    if (t_descriptors != NULL)
        CFRelease(t_descriptors);
    if (t_fonts != NULL)
        CFRelease(t_fonts);
    
    if (t_success)
        t_success = MCListCopy(*t_names, r_names);
    return t_success;
}

bool core_text_get_font_styles(MCStringRef p_name, uint32_t p_size, MCListRef &r_styles)
{    
    CTFontRef t_font_family;
    t_font_family = (CTFontRef)coretext_font_create_with_name_and_size(p_name, p_size);
    
    MCAutoListRef t_styles;
    /* UNCHECKED */ MCListCreateMutable('\n', &t_styles);
    
    bool t_success;
    t_success = true;
	
	if (t_font_family != NULL)
	{
		CTFontSymbolicTraits t_traits;
		t_traits = CTFontGetSymbolicTraits(t_font_family);
		
		t_success = MCListAppendCString(*t_styles, "plain");
        
		if (t_success && t_traits & kCTFontBoldTrait)
			t_success = MCListAppendCString(*t_styles, "bold");
		
		if (t_success && t_traits & kCTFontItalicTrait)
			t_success = MCListAppendCString(*t_styles, "italic");
        
		if (t_success && t_traits & kCTFontBoldTrait && t_traits & kCTFontItalicTrait)
			t_success = MCListAppendCString(*t_styles, "bold-italic");
	}
	
    if (t_font_family != NULL)
        CFRelease(t_font_family);
    
    if (t_success)
        t_success = MCListCopy(*t_styles, r_styles);
    
    return t_success;
}

bool coretext_font_load_from_path(MCStringRef p_path, bool p_globally)
{
    bool t_success;
    t_success = true;
    
    MCAutoStringRefAsCFString t_cf_path;
    t_success = t_cf_path . Lock(p_path);
    
    CFURLRef t_font_url;
    t_font_url = NULL;
    if (t_success)
    {
        t_font_url = CFURLCreateWithFileSystemPath(NULL, *t_cf_path, kCFURLPOSIXPathStyle, false);
        t_success = t_font_url != NULL;
    }
    
    if (t_success)
    {
        CTFontManagerScope t_scope;
        if (p_globally)
            t_scope = kCTFontManagerScopeUser;
        else
            t_scope = kCTFontManagerScopeProcess;
        t_success = CTFontManagerRegisterFontsForURL(t_font_url, t_scope, NULL);
    }
    
    if (t_font_url != NULL)
        CFRelease(t_font_url);
    
    return t_success;
}

bool coretext_font_unload(MCStringRef p_path, bool p_globally)
{
    bool t_success;
    t_success = true;
    
    MCAutoStringRefAsCFString t_cf_path;
    t_success = t_cf_path . Lock(p_path);
    
    CFURLRef t_font_url;
    t_font_url = NULL;
    if (t_success)
    {
        t_font_url = CFURLCreateWithFileSystemPath(NULL, *t_cf_path, kCFURLPOSIXPathStyle, false);
        t_success = t_font_url != NULL;
    }
    
    if (t_success)
    {
        CTFontManagerScope t_scope;
        if (p_globally)
            t_scope = kCTFontManagerScopeUser;
        else
            t_scope = kCTFontManagerScopeProcess;
        t_success = CTFontManagerUnregisterFontsForURL(t_font_url, t_scope, NULL);
    }
    
    if (t_font_url != NULL)
        CFRelease(t_font_url);
    
    return t_success;
}

void coretext_get_font_name(void *p_font, MCNameRef& r_name)
{
    CFStringRef t_font_name;
    t_font_name = CTFontCopyDisplayName((CTFontRef)p_font);
    
    MCAutoStringRef t_font_name_string;
    MCStringCreateWithCFString(t_font_name, &t_font_name_string);
    MCNameCreate(*t_font_name_string, r_name);
}

uint32_t coretext_get_font_size(void *p_font)
{
    return CTFontGetSize((CTFontRef)p_font);
}

#ifdef _MACOSX
ATSUFontID coretext_font_to_atsufontid(void *p_font)
{
    bool t_success;
    t_success = true;
    
    CTFontRef t_ctfont;
    t_ctfont = NULL;
    if (t_success)
    {
        t_ctfont = (CTFontRef)p_font;
        t_success = t_ctfont != NULL;
    }
    
    char t_name[256];
    if (t_success)
    {
        CFStringRef t_cfname;
        t_cfname = CTFontCopyPostScriptName(t_ctfont);
        t_success = t_cfname != NULL && CFStringGetCString(t_cfname, t_name, 256, kCFStringEncodingMacRoman);
        if (t_cfname != NULL)
            CFRelease(t_cfname);
    }
    
    ATSUFontID t_font_id;
    t_font_id = 0;
    if (t_success)
    {
        uint32_t t_name_length;
        t_name_length = MCCStringLength(t_name);
        t_success = ATSUFindFontFromName(t_name, t_name_length, kFontPostscriptName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) == noErr;
    }
    
    return t_font_id;
}
#endif