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

#include "execpt.h"
#include "globals.h"
#include "stack.h"
#include "card.h"
#include "eventqueue.h"

#import <SystemConfiguration/SystemConfiguration.h>

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

enum MCSystemReachability
{
	kMCSystemReachabilityTransient = 1<<0,
	kMCSystemReachabilityReachable = 1<<1,
	kMCSystemReachabilityConnectionRequired = 1<<2,
	kMCSystemReachabilityConnectionOnTraffic = 1<<3,
	kMCSystemReachabilityInterventionRequired = 1<<4,
	kMCSystemReachabilityIsLocal = 1<<5,
	kMCSystemReachabilityIsDirect = 1<<6,
	kMCSystemReachabilityIsWWAN = 1<<7,
};

class MCReachabilityEvent : public MCCustomEvent
{
public:
	MCReachabilityEvent(MCStringRef p_host, uint32_t p_flags);
	
	void Destroy();
	void Dispatch();
	
private:
	MCStringRef m_target;
	uint32_t m_flags;
};

static bool MCAddReachabilityTarget(MCStringRef p_host);
static bool MCRemoveReachabilityTarget(MCStringRef p_host);
void MCReachabilityEventInitialize();
void MCReachabilityEventFinalize();

static SCNetworkReachabilityRef s_reach_ref = nil;
MCStringRef s_reach_target;

////////////////////////////////////////////////////////////////////////////////

MCReachabilityEvent::MCReachabilityEvent(MCStringRef p_target, uint32_t p_flags)
{
    if (p_target != nil)
        m_target = MCValueRetain(p_target);
    else
        m_target = MCValueRetain(kMCEmptyString);
	m_flags = p_flags;
}

void MCReachabilityEvent::Destroy()
{
	if (m_target != nil)
		MCValueRelease(m_target);
	delete this;
}

void MCReachabilityEvent::Dispatch()
{
	bool t_success = true;
	
	MCAutoStringRef t_reachability;
    MCStringCreateMutable(0, &t_reachability);
    
	if (t_success && m_flags & kMCSystemReachabilityTransient)
		t_success = MCStringAppendFormat(*t_reachability, "transient,");
	if (t_success && m_flags & kMCSystemReachabilityReachable)
		t_success = MCStringAppendFormat(*t_reachability, "reachable,");
	if (t_success && m_flags & kMCSystemReachabilityConnectionRequired)
		t_success = MCStringAppendFormat(*t_reachability, "connection required,");
	if (t_success && m_flags & kMCSystemReachabilityConnectionOnTraffic)
		t_success = MCStringAppendFormat(*t_reachability, "connection on traffic,");
	if (t_success && m_flags & kMCSystemReachabilityInterventionRequired)
		t_success = MCStringAppendFormat(*t_reachability, "intervention required,");
	if (t_success && m_flags & kMCSystemReachabilityIsLocal)
		t_success = MCStringAppendFormat(*t_reachability, "is local,");
	if (t_success && m_flags & kMCSystemReachabilityIsDirect)
		t_success = MCStringAppendFormat(*t_reachability, "is direct,");
	if (t_success && m_flags & kMCSystemReachabilityIsWWAN)
		t_success = MCStringAppendFormat(*t_reachability, "is cell,");
	if (t_success)
	{
		MCdefaultstackptr->getcurcard()->message_with_valueref_args(MCM_reachability_changed, m_target, MCStringIsEmpty(*t_reachability) ? kMCEmptyString : *t_reachability);
	}
}

////////////////////////////////////////////////////////////////////////////////

uint32_t MCIPhoneReachabilityToMCFlags(SCNetworkReachabilityFlags p_flags)
{
	uint32_t t_flags = 0;
	if (p_flags & kSCNetworkReachabilityFlagsTransientConnection)
		t_flags |= kMCSystemReachabilityTransient;
	if (p_flags & kSCNetworkReachabilityFlagsReachable)
		t_flags |= kMCSystemReachabilityReachable;
	if (p_flags & kSCNetworkReachabilityFlagsConnectionRequired)
		t_flags |= kMCSystemReachabilityConnectionRequired;
	if (p_flags & kSCNetworkReachabilityFlagsConnectionOnTraffic)
		t_flags |= kMCSystemReachabilityConnectionOnTraffic;
	if (p_flags & kSCNetworkReachabilityFlagsIsLocalAddress)
		t_flags |= kMCSystemReachabilityIsLocal;
	if (p_flags & kSCNetworkReachabilityFlagsIsDirect)
		t_flags |= kMCSystemReachabilityIsDirect;
	if (p_flags & kSCNetworkReachabilityFlagsIsWWAN)
		t_flags |= kMCSystemReachabilityIsWWAN;
	
	return t_flags;
}

static void reachability_callback(SCNetworkReachabilityRef p_target, SCNetworkReachabilityFlags p_flags, void *p_info)
{
	MCCustomEvent *t_event;
	MCStringRef t_host = (MCStringRef)p_info;
	uint32_t t_flags = MCIPhoneReachabilityToMCFlags(p_flags);
	t_event = new MCReachabilityEvent(t_host, t_flags);
	MCEventQueuePostCustom(t_event);
}

static bool MCAddReachabilityTarget(MCStringRef p_host)
{
	bool t_success = true;
	SCNetworkReachabilityRef t_reach = nil;
	SCNetworkReachabilityContext t_context;
    char* t_host_cstring;
    t_host_cstring = nil;
    
    /* UNCHECKED */ MCStringConvertToCString(p_host, t_host_cstring);
	
	if (t_success)
        t_success = nil != (t_reach = SCNetworkReachabilityCreateWithName(kCFAllocatorDefault, t_host_cstring));
	if (t_success)
	{
		t_context.version = 0;
		t_context.copyDescription = nil;
		t_context.retain = MCValueRetain;
		t_context.release = MCValueRelease;
        t_context.info = p_host;
		SCNetworkReachabilitySetCallback(t_reach, reachability_callback, &t_context);
	}
	if (t_success)
	{
		SCNetworkReachabilityScheduleWithRunLoop(t_reach, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        MCRemoveReachabilityTarget(s_reach_target);
		
		s_reach_ref = t_reach;
        s_reach_target = MCValueRetain(p_host);
	}
	else
	{
		if (t_reach != nil)
            CFRelease(t_reach);
        if (t_host_cstring != nil)
            MCCStringFree(t_host_cstring);
	}
	return t_success;
}

static bool MCRemoveReachabilityTarget(MCStringRef p_host)
{
    bool t_success = true;
    if (s_reach_ref == nil || !MCStringIsEqualTo(p_host, s_reach_target, kMCCompareCaseless))
		return false;
	
	SCNetworkReachabilityUnscheduleFromRunLoop(s_reach_ref, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	CFRelease(s_reach_ref);
	s_reach_ref = nil;
    MCValueAssign(s_reach_target, kMCEmptyString);
	
	return true;
}

void MCReachabilityEventInitialize()
{
    s_reach_target = MCValueRetain(kMCEmptyString);
}

void MCReachabilityEventFinalize()
{
    MCValueRelease(s_reach_target);
}

bool MCSystemSetReachabilityTarget(MCStringRef p_hostname)
{
	if (p_hostname == nil || MCStringGetLength(p_hostname) == 0)
	{
        MCRemoveReachabilityTarget(s_reach_target);
		return true;
	}
	
    return MCAddReachabilityTarget(p_hostname);
}

bool MCSystemGetReachabilityTarget(MCStringRef& r_reach_target)
{
#ifdef /* MCHandleReachabilityTarget */ LEGACY_EXEC
	MCresult -> copysvalue(MCReachabilityGetTarget());
	return ES_NORMAL;
#endif /* MCHandleReachabilityTarget */
    r_reach_target = MCValueRetain(s_reach_target);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
