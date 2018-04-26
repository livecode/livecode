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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globdefs.h"


#include "globals.h"
#include "stack.h"
#include "card.h"
#include "param.h"
#include "eventqueue.h"
#include "mblsyntax.h"
#include "mbliphone.h"

#include <sys/xattr.h>

#include <Foundation/Foundation.h>

////////////////////////////////////////////////////////////////////////////////

class MCProtectedDataEvent : public MCCustomEvent
{
public:
    MCProtectedDataEvent(MCNameRef p_message)
    {
        m_message = p_message;
    }
    
    void Dispatch()
    {
        MCdefaultstackptr->getcurcard()->message(m_message);
    }
    
    void Destroy()
    {
    }
    
private:
    MCNameRef m_message;
};

void MCiOSFilePostProtectedDataAvailableEvent()
{
    MCCustomEvent *t_event;
    t_event = new MCProtectedDataEvent(MCM_protected_data_available);
    MCEventQueuePostCustom(t_event);
}

void MCiOSFilePostProtectedDataUnavailableEvent()
{
    MCCustomEvent *t_event;
    t_event = new MCProtectedDataEvent(MCM_protected_data_unavailable);
    MCEventQueuePostCustom(t_event);
}


////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    const char *string;
    NSString *protection;
} mcprotection_string_t;

bool MCDataProtectionFromString(MCStringRef p_string, NSString *&r_protection)
{
    if (MCStringIsEqualToCString(p_string, "none", kMCCompareCaseless))
    {
        r_protection = NSFileProtectionNone;
        return true;
    }
    if (MCStringIsEqualToCString(p_string, "complete", kMCCompareCaseless))
    {
        r_protection = NSFileProtectionComplete;
        return true;
    }
#ifdef __IPHONE_5_0
    if (MCmajorosversion >= 500)
    {
        if (MCStringIsEqualToCString(p_string, "complete unless open", kMCCompareCaseless))
        {
            //r_protection = NSFileProtectionCompleteUnlessOpen;
            r_protection = @"NSFileProtectionCompleteUnlessOpen";
            return true;
        }
        if (MCStringIsEqualToCString(p_string, "complete until first user authentication", kMCCompareCaseless))
        {
            //r_protection = NSFileProtectionCompleteUntilFirstUserAuthentication;
            r_protection = @"NSFileProtectionCompleteUntilFirstUserAuthentication";
            return true;
        }
    }
#endif
    return false;
}

bool MCDataProtectionToString(NSString *p_protection, MCStringRef &r_string)
{
    if ([p_protection isEqualToString:NSFileProtectionNone])
    {
        MCStringCreateWithCString("none", r_string);
        return true;
    }
    if ([p_protection isEqualToString:NSFileProtectionComplete])
    {
        MCStringCreateWithCString("complete", r_string);
        return true;
    }
#ifdef __IPHONE_5_0
    if (MCmajorosversion >= 500)
    {
        if ([p_protection isEqualToString:@"NSFileProtectionCompleteUnlessOpen"])
        {
            MCStringCreateWithCString("complete unless open", r_string);
            return true;
        }
        if ([p_protection isEqualToString:@"NSFileProtectionCompleteUntilFirstUserAuthentication"])
        {
            MCStringCreateWithCString("complete until first user authentication", r_string);
            return true;
        }
    }
#endif
    return false;
}

bool MCFileSetDataProtection(MCStringRef p_filename, NSString *p_protection)
{
    bool t_success = true;
    
    NSDictionary *t_file_attr = [NSDictionary dictionaryWithObject:p_protection forKey:NSFileProtectionKey];
    NSString *t_path = nil;
    
    t_path = MCStringConvertToAutoreleasedNSString(p_filename);
    
    t_success = [[NSFileManager defaultManager] setAttributes:t_file_attr ofItemAtPath:t_path error: nil];
    
    return t_success;
}

bool MCFileGetDataProtection(MCStringRef p_filename, NSString *&r_protection)
{
    bool t_success = true;
   
    NSString *t_path = MCStringConvertToAutoreleasedNSString(p_filename);
    NSDictionary *t_file_attr = [[NSFileManager defaultManager] attributesOfItemAtPath:t_path error:nil];
    
    t_success = t_file_attr != nil;
    
    if (t_success)
    {
        r_protection = [t_file_attr objectForKey:NSFileProtectionKey];
        if (r_protection == nil)
            r_protection = NSFileProtectionNone;
    }
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

NSString *MCStringRefToNSString(MCStringRef p_string, bool p_unicode)
{

	if (p_unicode)
		return [NSString stringWithCharacters: MCStringGetCharPtr(p_string) length: MCStringGetLength(p_string)];
    MCAutoPointer<char> t_string;
    /* UNCHECKED */ MCStringConvertToCString(p_string, &t_string);
	return [[[NSString alloc] initWithBytes: *t_string length: MCStringGetLength(p_string) encoding: NSMacOSRomanStringEncoding] autorelease];
}

bool NSStringToMCStringRef(NSString *p_string, MCStringRef& r_string)
{
    return MCStringCreateWithCString([p_string nativeCString], r_string);
}
