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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "globals.h"
#include "stack.h"
#include "card.h"
#include "param.h"
#include "eventqueue.h"

#include <sys/xattr.h>

#include <Foundation/Foundation.h>

#define FILEATTR_DONOTBACKUP "com.apple.MobileBackup"

bool MCiOSFileSetDoNotBackup(const char *p_path, bool p_no_backup)
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
}

bool MCiOSFileGetDoNotBackup(const char *p_path)
{
#ifdef /* MCiOSFileGetDoNotBackup */ LEGACY_EXEC
    uint8_t t_val = 0;
    if (-1 == getxattr(p_path, FILEATTR_DONOTBACKUP, &t_val, sizeof(t_val), 0, 0))
        return false;
    return t_val != 0;
#endif /* MCiOSFileGetDoNotBackup */
}

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

//static mcprotection_string_t s_protection_strings[] = {
//    {"none", NSFileProtectionNone},
//    {"complete", NSFileProtectionComplete},
//    {"complete unless open", NSFileProtectionCompleteUnlessOpen},
//    {"complete until first user authentication", NSFileProtectionCompleteUntilFirstUserAuthentication},
//};

bool MCDataProtectionFromString(const char *p_string, NSString *&r_protection)
{
//    for (uint32_t i = 0; i < 4; i++)
//    {
//        if (MCCStringEqualCaseless(p_string, s_protection_strings[i].string))
//        {
//            r_protection = s_protection_strings[i].protection;
//            return true;
//        }
//    }
    if (MCCStringEqualCaseless(p_string, "none"))
    {
        r_protection = NSFileProtectionNone;
        return true;
    }
    if (MCCStringEqualCaseless(p_string, "complete"))
    {
        r_protection = NSFileProtectionComplete;
        return true;
    }
#ifdef __IPHONE_5_0
    if (MCmajorosversion >= 500)
    {
        if (MCCStringEqualCaseless(p_string, "complete unless open"))
        {
            //r_protection = NSFileProtectionCompleteUnlessOpen;
            r_protection = @"NSFileProtectionCompleteUnlessOpen";
            return true;
        }
        if (MCCStringEqualCaseless(p_string, "complete until first user authentication"))
        {
            //r_protection = NSFileProtectionCompleteUntilFirstUserAuthentication;
            r_protection = @"NSFileProtectionCompleteUntilFirstUserAuthentication";
            return true;
        }
    }
#endif
    return false;
}

bool MCDataProtectionToString(NSString *p_protection, const char *&r_string)
{
//    for (uint32_t i = 0; i < 4; i++)
//    {
//        if ([s_protection_strings[i].protection isEqualToString: p_protection])
//        {
//            r_string = s_protection_strings[i].string;
//            return true;
//        }
//    }
    if ([p_protection isEqualToString:NSFileProtectionNone])
    {
        r_string = "none";
        return true;
    }
    if ([p_protection isEqualToString:NSFileProtectionComplete])
    {
        r_string = "complete";
        return true;
    }
#ifdef __IPHONE_5_0
    if (MCmajorosversion >= 500)
    {
        //if ([p_protection isEqualToString:NSFileProtectionCompleteUnlessOpen])
        if ([p_protection isEqualToString:@"NSFileProtectionCompleteUnlessOpen"])
        {
            r_string = "complete unless open";
            return true;
        }
        //if ([p_protection isEqualToString:NSFileProtectionCompleteUntilFirstUserAuthentication])
        if ([p_protection isEqualToString:@"NSFileProtectionCompleteUntilFirstUserAuthentication"])
        {
            r_string = "complete until first user authentication";
            return true;
        }
    }
#endif
    return false;
}

bool MCFileSetDataProtection(const char *p_filename, NSString *p_protection)
{
    bool t_success = true;
    
    NSDictionary *t_file_attr = [NSDictionary dictionaryWithObject:p_protection forKey:NSFileProtectionKey];
    NSString *t_path = nil;
    
    t_path = [NSString stringWithCString:p_filename encoding: NSMacOSRomanStringEncoding];
    
    t_success = [[NSFileManager defaultManager] setAttributes:t_file_attr ofItemAtPath:t_path error: nil];
    
    return t_success;
}

bool MCFileGetDataProtection(const char *p_filename, NSString *&r_protection)
{
    bool t_success = true;
   
    NSString *t_path = [NSString stringWithCString:p_filename encoding:NSMacOSRomanStringEncoding];
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
Exec_stat MCHandleFileSetDoNotBackup(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleFileSetDoNotBackup */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);
	
    const char *t_path = nil;
    
	bool t_no_backup;
	t_no_backup = true;
    
    if (p_parameters != nil)
    {
        p_parameters->eval_argument(ep);
        t_path = ep.getsvalue().clone();
        p_parameters = p_parameters->getnext();
    }
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_no_backup = (ep . getsvalue() == MCtruemcstring);
		p_parameters = p_parameters -> getnext();
	}
	
    if (t_path != nil)
        MCiOSFileSetDoNotBackup(t_path, t_no_backup);
	
	return ES_NORMAL;
#endif /* MCHandleFileSetDoNotBackup */
}

Exec_stat MCHandleFileGetDoNotBackup(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleFileGetDoNotBackup */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    const char *t_path = nil;
    if (p_parameters != nil)
    {
        p_parameters->eval_argument(ep);
        t_path = ep.getcstring();
    }
    MCresult->sets(MCU_btos(MCiOSFileGetDoNotBackup(t_path)));
    
    return ES_NORMAL;
#endif /* MCHandleFileGetDoNotBackup */
}

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

Exec_stat MCHandleFileSetDataProtection(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleFileSetDataProtection */ LEGACY_EXEC
    bool t_success = true;
    
    char *t_filename = nil;
    char *t_protection_string = nil;
    
    NSString *t_protection = nil;
    
    t_success = MCParseParameters(p_parameters, "ss", &t_filename, &t_protection_string);
    
    if (t_success)
    {
        if (!MCDataProtectionFromString(t_protection_string, t_protection))
        {
            MCresult->sets("unknown protection type");
            t_success = false;
        }
    }
    
    if (t_success)
    {
        if (!MCFileSetDataProtection(t_filename, t_protection))
        {
            MCresult->sets("cannot set file protection");
            t_success = false;
        }
    }    

    if (t_success)
        MCresult->clear();
    
    return ES_NORMAL;
#endif /* MCHandleFileSetDataProtection */
}

Exec_stat MCHandleFileGetDataProtection(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleFileGetDataProtection */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    bool t_success = true;
    
    const char *t_filename = nil;
    const char *t_protection_string = nil;
    NSString *t_protection = nil;
    
    if (p_parameters != nil)
    {
        p_parameters->eval_argument(ep);
        t_filename = ep.getcstring();
    }
    else
        t_success = false;
    
    if (t_success)
        t_success = MCFileGetDataProtection(t_filename, t_protection);
    
    if (t_success)
        t_success = MCDataProtectionToString(t_protection, t_protection_string);
    
    if (t_success)
        MCresult->sets(t_protection_string);
    else
        MCresult->clear();
    
    return ES_NORMAL;
#endif /* MCHandleFileGetDataProtection */
}
