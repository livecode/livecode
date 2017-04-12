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
#include "mcio.h"

#include "globals.h"
#include "stack.h"
#include "image.h"
#include "param.h"
#include "card.h"

#include "mcerror.h"

#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"

#include "mblad.h"

////////////////////////////////////////////////////////////////////////////////

//bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
void MCSystemInneractiveAdInit();

////////////////////////////////////////////////////////////////////////////////

static MCAd *s_ads = nil;
static uint32_t s_last_ad_id = 0;

static MCStringRef s_inneractive_ad_key;

void MCAdInitialize(void)
{
    s_ads = nil;
    s_last_ad_id = 0;
    s_inneractive_ad_key  = MCValueRetain(kMCEmptyString);
	
	MCSystemInneractiveAdInit();
}

void MCAdFinalize(void)
{
    MCValueRelease(s_inneractive_ad_key);
    MCAd::Finalize();
    s_ads = nil;
    s_last_ad_id = 0;  
}

////////////////////////////////////////////////////////////////////////////////

MCAdType MCAdTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "banner", kMCCompareCaseless))
        return kMCAdTypeBanner;
    else if (MCStringIsEqualToCString(p_string, "text", kMCCompareCaseless))
        return kMCAdTypeText;
    else if (MCStringIsEqualToCString(p_string, "full screen", kMCCompareCaseless))
        return kMCAdTypeFullscreen;    
    return kMCAdTypeUnknown;
}

////////////////////////////////////////////////////////////////////////////////

MCStringRef MCAdGetInneractiveKey(void)
{
    return s_inneractive_ad_key;
}

bool MCAdInneractiveKeyIsNil(void)
{
    return s_inneractive_ad_key == nil || MCStringIsEmpty(s_inneractive_ad_key);
}

bool MCAdSetInneractiveKey(MCStringRef p_new_key)
{
    if (p_new_key != nil)
        MCValueRelease(s_inneractive_ad_key);
    
    return MCStringCopy(p_new_key, s_inneractive_ad_key);
}

////////////////////////////////////////////////////////////////////////////////

MCAd* MCAdGetStaticAdsPtr()
{
    return s_ads;
}

void MCAdSetStaticAdsPtr(MCAd* p_ads_ptr)
{
    s_ads = p_ads_ptr;
}

////////////////////////////////////////////////////////////////////////////////

class MCAdEvent: public MCCustomEvent
{
public:
	MCAdEvent(MCAd *p_target, MCAdEventType p_event)
	{
		m_target = p_target;
        if (m_target != nil)
		m_target -> Retain();
		m_event = p_event;
	}
	
	void Destroy(void)
	{
        if (m_target != nil)
		m_target -> Release();
		delete this;
	}
	
	void Dispatch(void)
	{
        if (m_target == nil)
        {
            switch(m_event)
            {
                case kMCAdEventTypeReceive:
                    MCdefaultstackptr->getcurcard()->message_with_valueref_args(MCM_ad_loaded, kMCFalse);
                    break;
                case kMCAdEventTypeReceiveDefault:
                    MCdefaultstackptr->getcurcard()->message_with_valueref_args(MCM_ad_loaded, kMCTrue);
                    break;
                case kMCAdEventTypeReceiveFailed:
                    MCdefaultstackptr->getcurcard()->message(MCM_ad_load_failed);
                    break;
                case kMCAdEventTypeClick:
                    MCdefaultstackptr->getcurcard()->message(MCM_ad_clicked);
                    break;
                case kMCAdEventTypeResizeStart:
                    MCdefaultstackptr->getcurcard()->message(MCM_ad_resize_start);
                    break;
                case kMCAdEventTypeResizeEnd:
                    MCdefaultstackptr->getcurcard()->message(MCM_ad_resize_end);
                    break;
                case kMCAdEventTypeExpandStart:
                    MCdefaultstackptr->getcurcard()->message(MCM_ad_expand_start);
                    break;
                case kMCAdEventTypeExpandEnd:
                    MCdefaultstackptr->getcurcard()->message(MCM_ad_expand_end);
                    break;
            }
        }
        else
        {
        MCObjectHandle t_object = m_target->GetOwner();
        if (t_object.IsValid())
        {
            switch(m_event)
            {
                case kMCAdEventTypeReceive:
                    t_object->message_with_valueref_args(MCM_ad_loaded, m_target->GetName(), kMCFalse);
                    break;
                case kMCAdEventTypeReceiveDefault:
                    t_object->message_with_valueref_args(MCM_ad_loaded, m_target->GetName(), kMCTrue);
                    break;
                case kMCAdEventTypeReceiveFailed:
                    t_object->message_with_valueref_args(MCM_ad_load_failed, m_target->GetName());
                    break;
                case kMCAdEventTypeClick:
                    t_object->message_with_valueref_args(MCM_ad_clicked, m_target->GetName());
                    break;
            }
        }
	}
	}
	
private:
    MCAd *m_target;
    MCAdEventType m_event;
};

void MCAdPostMessage(MCAd *p_ad, MCAdEventType p_type)
{
    MCCustomEvent *t_event;
    t_event = new (nothrow) MCAdEvent(p_ad, p_type);
    if (t_event != nil)
        MCEventQueuePostCustom(t_event);
}

////////////////////////////////////////////////////////////////////////////////

MCAd::MCAd(void) :
  m_references(1),
  m_id(++s_last_ad_id),
  m_name(MCValueRetain(kMCEmptyString)),
  m_object(nil),
  m_next(nil)
{
}

MCAd::~MCAd(void)
{
	MCValueRelease(m_name);
    
	if (s_ads == this)
		s_ads = m_next;
	else
		for(MCAd *t_previous = s_ads; t_previous != nil; t_previous = t_previous -> m_next)
			if (t_previous -> m_next == this)
			{
				t_previous -> m_next = m_next;
				break;
			}
}

void MCAd::Retain(void)
{
	m_references += 1;
}

void MCAd::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
    {
        Delete();
		delete this;
    }
}

uint32_t MCAd::GetId(void)
{
	return m_id;
}

MCStringRef MCAd::GetName()
{
	return m_name;
}

MCObjectHandle MCAd::GetOwner(void)
{
	return m_object;
}

void MCAd::SetOwner(MCObjectHandle p_owner)
{
	m_object = p_owner;
}

bool MCAd::SetName(MCStringRef p_name)
{
	MCValueAssign(m_name, p_name);
	return true;
}

MCAd *MCAd::GetFirst()
{
    return s_ads;
}

bool MCAd::FindByNameOrId(MCStringRef p_name, MCAd *&r_ad)
{
	integer_t t_id;
    if (MCStringToInteger(p_name, t_id))
		return FindById(t_id, r_ad);
	
	for(MCAd *t_ad = s_ads; t_ad != nil; t_ad = t_ad -> m_next)
		if (MCStringIsEqualTo(p_name, t_ad->GetName(), kMCStringOptionCompareCaseless))
		{
			r_ad = t_ad;
			return true;
		}
	
	return false;
}

bool MCAd::FindById(uint32_t p_id, MCAd *&r_ad)
{
	for(MCAd *t_ad = s_ads; t_ad != nil; t_ad = t_ad -> m_next)
		if (t_ad -> GetId() == p_id)
		{
			r_ad = t_ad;
			return true;
		}
	
	return false;
}

MCAd *MCAd::GetNext()
{
    return m_next;
}

void MCAd::SetNext(MCAd *p_next)
{
    m_next = p_next;
}

void MCAd::Finalize(void)
{
    for(MCAd *t_ad = s_ads; t_ad != nil;)
    {
        MCAd *t_next_ad;
        t_next_ad = t_ad->m_next;
        delete t_ad;
        t_ad = t_next_ad;
    }
}

////////////////////////////////////////////////////////////////////////////////
