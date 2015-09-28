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

#include "core.h"
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
	MCReachabilityEvent(const char *p_host, uint32_t p_flags);
	
	void Destroy();
	void Dispatch();
	
private:
	char *m_target;
	uint32_t m_flags;
};

static bool MCAddReachabilityTarget(const char *p_host);
static bool MCRemoveReachabilityTarget(const char *p_host);

static SCNetworkReachabilityRef s_reach_ref = nil;
char *s_reach_target = nil;

////////////////////////////////////////////////////////////////////////////////

MCReachabilityEvent::MCReachabilityEvent(const char *p_target, uint32_t p_flags)
{
	m_target = nil;
	MCCStringClone(p_target, m_target);
	m_flags = p_flags;
}

void MCReachabilityEvent::Destroy()
{
	if (m_target != nil)
		MCCStringFree(m_target);
	delete this;
}

void MCReachabilityEvent::Dispatch()
{
	bool t_success = true;
	
	char *t_reachability = nil;
	if (t_success && m_flags & kMCSystemReachabilityTransient)
		t_success = MCCStringAppend(t_reachability, "transient,");
	if (t_success && m_flags & kMCSystemReachabilityReachable)
		t_success = MCCStringAppend(t_reachability, "reachable,");
	if (t_success && m_flags & kMCSystemReachabilityConnectionRequired)
		t_success = MCCStringAppend(t_reachability, "connection required,");
	if (t_success && m_flags & kMCSystemReachabilityConnectionOnTraffic)
		t_success = MCCStringAppend(t_reachability, "connection on traffic,");
	if (t_success && m_flags & kMCSystemReachabilityInterventionRequired)
		t_success = MCCStringAppend(t_reachability, "intervention required,");
	if (t_success && m_flags & kMCSystemReachabilityIsLocal)
		t_success = MCCStringAppend(t_reachability, "is local,");
	if (t_success && m_flags & kMCSystemReachabilityIsDirect)
		t_success = MCCStringAppend(t_reachability, "is direct,");
	if (t_success && m_flags & kMCSystemReachabilityIsWWAN)
		t_success = MCCStringAppend(t_reachability, "is cell,");
	if (t_success)
	{
		if (t_reachability != nil)
			t_reachability[MCCStringLength(t_reachability) - 1] = '\0';
		MCdefaultstackptr->getcurcard()->message_with_args(MCM_reachability_changed, m_target, t_reachability == nil ? "" : t_reachability);
	}
	
	MCCStringFree(t_reachability);
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
	char *t_host = (char*)p_info;
	uint32_t t_flags = MCIPhoneReachabilityToMCFlags(p_flags);
	t_event = new MCReachabilityEvent(t_host, t_flags);
	MCEventQueuePostCustom(t_event);
}

static bool MCAddReachabilityTarget(const char *p_host)
{
	bool t_success = true;
	SCNetworkReachabilityRef t_reach = nil;
	SCNetworkReachabilityContext t_context;
	
	char *t_target = nil;
	
	if (t_success)
		t_success = nil != (t_reach = SCNetworkReachabilityCreateWithName(kCFAllocatorDefault, p_host));
	if (t_success)
		t_success = MCCStringClone(p_host, t_target);
	if (t_success)
	{
		t_context.version = 0;
		t_context.copyDescription = nil;
		t_context.retain = nil;
		t_context.release = nil;
		t_context.info = t_target;
		SCNetworkReachabilitySetCallback(t_reach, reachability_callback, &t_context);
	}
	if (t_success)
	{
		SCNetworkReachabilityScheduleWithRunLoop(t_reach, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		if (s_reach_target != nil)
			MCRemoveReachabilityTarget(s_reach_target);
		
		s_reach_ref = t_reach;
		s_reach_target = t_target;
	}
	else
	{
		if (t_reach != nil)
			CFRelease(t_reach);
		if (t_target != nil)
			MCCStringFree(t_target);
	}
	return t_success;
}

static bool MCRemoveReachabilityTarget(const char *p_host)
{
	bool t_success = true;
	if (s_reach_ref == nil || !MCCStringEqualCaseless(p_host, s_reach_target))
		return false;
	
	SCNetworkReachabilityUnscheduleFromRunLoop(s_reach_ref, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	CFRelease(s_reach_ref);
	s_reach_ref = nil;
	MCCStringFree(s_reach_target);
	s_reach_target = nil;
	
	return true;
}

bool MCReachabilitySetTarget(const char *p_hostname)
{
	if (p_hostname == nil || MCCStringLength(p_hostname) == 0)
	{
		MCRemoveReachabilityTarget(s_reach_target);
		return true;
	}
	
	return MCAddReachabilityTarget(p_hostname);
}

const char *MCReachabilityGetTarget(void)
{
	return s_reach_target == nil ? "" : s_reach_target;
}

////////////////////////////////////////////////////////////////////////////////
