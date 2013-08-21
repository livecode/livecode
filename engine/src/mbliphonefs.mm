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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globdefs.h"

#include "execpt.h"
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

//static mcprotection_string_t s_protection_strings[] = {
//    {"none", NSFileProtectionNone},
//    {"complete", NSFileProtectionComplete},
//    {"complete unless open", NSFileProtectionCompleteUnlessOpen},
//    {"complete until first user authentication", NSFileProtectionCompleteUntilFirstUserAuthentication},
//};

bool MCDataProtectionFromString(MCStringRef p_string, NSString *&r_protection)
{
//    for (uint32_t i = 0; i < 4; i++)
//    {
//        if (MCCStringEqualCaseless(p_string, s_protection_strings[i].string))
//        {
//            r_protection = s_protection_strings[i].protection;
//            return true;
//        }
//    }
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
        //if ([p_protection isEqualToString:NSFileProtectionCompleteUnlessOpen])
        if ([p_protection isEqualToString:@"NSFileProtectionCompleteUnlessOpen"])
        {
            MCStringCreateWithCString("complete unless open", r_string);
            return true;
        }
        //if ([p_protection isEqualToString:NSFileProtectionCompleteUntilFirstUserAuthentication])
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
    
    t_path = [NSString stringWithCString:MCStringGetCString(p_filename) encoding: NSMacOSRomanStringEncoding];
    
    t_success = [[NSFileManager defaultManager] setAttributes:t_file_attr ofItemAtPath:t_path error: nil];
    
    return t_success;
}

bool MCFileGetDataProtection(MCStringRef p_filename, NSString *&r_protection)
{
    bool t_success = true;
   
    NSString *t_path = [NSString stringWithCString:MCStringGetCString(p_filename) encoding:NSMacOSRomanStringEncoding];
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

//// MOVED TO mblhandlers.cpp
//Exec_stat MCHandleFileSetDoNotBackup(void *context, MCParameter *p_parameters)
//{
///*	MCExecPoint ep(nil, nil, nil);
//	
//    const char *t_path = nil;
//    
//	bool t_no_backup;
//	t_no_backup = true;
//    
//    if (p_parameters != nil)
//    {
//        p_parameters->eval_argument(ep);
//        t_path = ep.getsvalue().clone();
//        p_parameters = p_parameters->getnext();
//    }
//	if (p_parameters != nil)
//	{
//		p_parameters -> eval_argument(ep);
//		t_no_backup = (ep . getsvalue() == MCtruemcstring);
//		p_parameters = p_parameters -> getnext();
//	}
//	
//    if (t_path != nil)
//        MCiOSFileSetDoNotBackup(t_path, t_no_backup);
//	
//	return ES_NORMAL;*/
//
//	MCExecPoint ep(nil, nil, nil);
//	
//    MCAutoStringRef t_path;
//    
//	bool t_no_backup;
//	t_no_backup = true;
//    
//    if (p_parameters != nil)
//    {
//        p_parameters->eval_argument(ep);
//        /* UNCHECKED */ ep . copyasstringref(&t_path);
//        p_parameters = p_parameters->getnext();
//    }
//	if (p_parameters != nil)
//	{
//		p_parameters -> eval_argument(ep);
//		/* UNCHECKED */ ep . copyasbool(t_no_backup);
//		p_parameters = p_parameters -> getnext();
//	}
//	
//	MCExecContext ctxt(ep);
//    if (*t_path != nil)
//        //MCMobileExecFileSetDoNotBackup(ctxt, *t_path, t_no_backup);
//	
//	if (!ctxt . HasError())
//		return ES_NORMAL;
//
//	return ES_ERROR;
//}
//
//Exec_stat MCHandleFileGetDoNotBackup(void *context, MCParameter *p_parameters)
//{
///*    MCExecPoint ep(nil, nil, nil);
//    
//    const char *t_path = nil;
//    if (p_parameters != nil)
//    {
//        p_parameters->eval_argument(ep);
//        t_path = ep.getcstring();
//    }
//    MCresult->sets(MCU_btos(MCiOSFileGetDoNotBackup(t_path)));
//    
//    return ES_NORMAL; */
//
//	MCExecPoint ep(nil, nil, nil);
//
//	MCAutoStringRef t_path;
//    {
//        p_parameters->eval_argument(ep);
//        /* UNCHECKED */ ep . copyasstringref(&t_path);
//    }
//
//	MCExecContext ctxt(ep);
//
//	//MCMobileExecFileGetDoNotBackup(ctxt, *t_path);
//
//	if (!ctxt . HasError())
//		return ES_NORMAL;
//
//	return ES_ERROR;
//}
//
//bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
//
//Exec_stat MCHandleFileSetDataProtection(void *context, MCParameter *p_parameters)
//{
///*  bool t_success = true;
//    
//    char *t_filename = nil;
//    char *t_protection_string = nil;
//    
//    NSString *t_protection = nil;
//    
//    t_success = MCParseParameters(p_parameters, "ss", &t_filename, &t_protection_string);
//    
//    if (t_success)
//    {
//        if (!MCDataProtectionFromString(t_protection_string, t_protection))
//        {
//            MCresult->sets("unknown protection type");
//            t_success = false;
//        }
//    }
//    
//    if (t_success)
//    {
//        if (!MCFileSetDataProtection(t_filename, t_protection))
//        {
//            MCresult->sets("cannot set file protection");
//            t_success = false;
//        }
//    }    
//
//    if (t_success)
//        MCresult->clear();
//    
//    return ES_NORMAL; */
//
//	MCExecPoint ep(nil, nil, nil);
//    MCExecContext ctxt(ep);
//
//    MCAutoStringRef t_filename;
//    MCAutoStringRef t_protection_string;
//    
//    NSString *t_protection = nil;
//    
//    if (MCParseParameters(p_parameters, "xx", &t_filename, &t_protection_string))
//	{
//		//MCMobileExecFileSetDataProtection(ctxt, *t_filename, *t_protection_string);
//	}
//	if (!ctxt . HasError())
//		return ES_NORMAL;
//	
//	return ES_ERROR;
//}
//
//Exec_stat MCHandleFileGetDataProtection(void *context, MCParameter *p_parameters)
//{
///*	MCExecPoint ep(nil, nil, nil);
//    
//    bool t_success = true;
//    
//    const char *t_filename = nil;
//    const char *t_protection_string = nil;
//    NSString *t_protection = nil;
//    
//    if (p_parameters != nil)
//    {
//        p_parameters->eval_argument(ep);
//        t_filename = ep.getcstring();
//    }
//    else
//        t_success = false;
//    
//    if (t_success)
//        t_success = MCFileGetDataProtection(t_filename, t_protection);
//    
//    if (t_success)
//        t_success = MCDataProtectionToString(t_protection, t_protection_string);
//    
//    if (t_success)
//        MCresult->sets(t_protection_string);
//    else
//        MCresult->clear();
//    
//    return ES_NORMAL; */
//		
//	MCExecPoint ep(nil, nil, nil);
//    MCExecContext ctxt(ep);
//
//    MCAutoStringRef t_filename;
//    
//    if (p_parameters != nil)
//    {
//        p_parameters->eval_argument(ep);
//        /* UNCHECKED */ ep . copyasstringref(&t_filename);
//		//MCMobileExecFileGetDataProtection(ctxt, *t_filename);
//    }
//
//	if (!ctxt . HasError())
//		return ES_NORMAL;
//
//	return ES_ERROR;
//}

//////////////////////////////////////////////////////////////////////////////////

//// MOVED TO exec-misc.cpp
//void MCMobileExecFileSetDoNotBackup(MCExecContext& ctxt, MCStringRef p_path, bool p_no_backup)
//{
//	MCiOSFileSetDoNotBackup(MCStringGetCString(p_path), p_no_backup);
//}
//
//void MCMobileExecFileGetDoNotBackup(MCExecContext& ctxt, MCStringRef p_path)
//{
//	MCiOSFileGetDoNotBackup(MCStringGetCString(p_path));
//}
//
//void MCMobileExecFileSetDataProtection(MCExecContext& ctxt, MCStringRef p_filename, MCStringRef p_protection_string)
//{
//	NSString *t_protection = nil;
//
//    if (!MCDataProtectionFromString(p_protection_string, t_protection))
//    {
//		ctxt . SetTheResultToStaticCString("unknown protection type");
//        return;
//    }
//    
//    if (!MCFileSetDataProtection(MCStringGetCString(p_filename), t_protection))
//    {
//		ctxt . SetTheResultToStaticCString("cannot set file protection");
//        return;
//    }    
//    
//	ctxt . SetTheResultToEmpty();
//}
//
//void MCMobileExecFileGetDataProtection(MCExecContext& ctxt, MCStringRef p_filename)
//{
//	NSString *t_protection = nil;
//	MCAutoStringRef t_protection_string;
//	bool t_success;
//
//	if (t_success)
//		t_success = MCFileGetDataProtection(MCStringGetCString(p_filename), t_protection);
//
//	if (t_success)
//		t_success = MCDataProtectionToString(t_protection, &t_protection_string);
//    
//	if (t_success)
//        ctxt . SetTheResultToValue(*t_protection_string);
//    else
//        ctxt . SetTheResultToEmpty();
//}

NSString *MCStringRefToNSString(MCStringRef p_string, bool p_unicode)
{
	if (p_unicode)
		return [NSString stringWithCharacters: MCStringGetCharPtr(p_string) length: MCStringGetLength(p_string)];
	return [[[NSString alloc] initWithBytes: MCStringGetCString(p_string) length: MCStringGetLength(p_string) encoding: NSMacOSRomanStringEncoding] autorelease];
}

bool NSStringToMCStringRef(NSString *p_string, MCStringRef& r_string)
{
    return MCStringCreateWithCString([p_string nativeCString], r_string);
}
