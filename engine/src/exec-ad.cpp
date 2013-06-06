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
#include "mcio.h"

#include "mcerror.h"
#include "globals.h"
#include "exec.h"
#include "eventqueue.h"
#include "stack.h"
#include "card.h"

#include "mblad.h"


////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EXEC_METHOD(Ad, Register, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Ad, Create, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Ad, Delete, 1)
MC_EXEC_DEFINE_SET_METHOD(Ad, VisibleOfAd, 2)
MC_EXEC_DEFINE_GET_METHOD(Ad, VisibleOfAd, 2)
MC_EXEC_DEFINE_SET_METHOD(Ad, TopLeftOfAd, 3)
MC_EXEC_DEFINE_GET_METHOD(Ad, TopLeftOfAd, 3)
MC_EXEC_DEFINE_GET_METHOD(Ad, Ads, 1)


////////////////////////////////////////////////////////////////////////////////


void MCSystemInneractiveAdInit();
bool MCSystemInneractiveAdCreate(MCExecContext &ctxt, MCAd*& r_ad, MCAdType p_type, MCAdTopLeft p_top_left, uint32_t p_timeout, MCArrayRef p_meta_data);

////////////////////////////////////////////////////////////////////////////////

static MCAd *s_ads = nil;
static uint32_t s_last_ad_id = 0;

static char *s_inneractive_ad_key = nil;

void MCAdInitialize(void)
{
    s_ads = nil;
    s_last_ad_id = 0;
    s_inneractive_ad_key = nil;
	
	MCSystemInneractiveAdInit();
}

void MCAdFinalize(void)
{
    MCCStringFree(s_inneractive_ad_key);
    MCAd::Finalize();
    s_ads = nil;
    s_last_ad_id = 0;
}

////////////////////////////////////////////////////////////////////////////////

MCAdType MCAdTypeFromCString(const char *p_string)
{
    if (MCCStringEqualCaseless(p_string, "banner"))
        return kMCAdTypeBanner;
    else if (MCCStringEqualCaseless(p_string, "text"))
        return kMCAdTypeText;
    else if (MCCStringEqualCaseless(p_string, "full screen"))
        return kMCAdTypeFullscreen;
    return kMCAdTypeUnknown;
}

////////////////////////////////////////////////////////////////////////////////

const char *MCAdGetInneractiveKey(void)
{
    return s_inneractive_ad_key;
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
                    MCdefaultstackptr->getcurcard()->message_with_args(MCM_ad_loaded, MCfalsemcstring);
                    break;
                case kMCAdEventTypeReceiveDefault:
                    MCdefaultstackptr->getcurcard()->message_with_args(MCM_ad_loaded, MCtruemcstring);
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
            MCObjectHandle *t_object;
            t_object = m_target->GetOwner();
            if (t_object != nil && t_object->Exists())
            {
                switch(m_event)
                {
                    case kMCAdEventTypeReceive:
                        t_object->Get()->message_with_args(MCM_ad_loaded, m_target->GetName(), MCfalsemcstring);
                        break;
                    case kMCAdEventTypeReceiveDefault:
                        t_object->Get()->message_with_args(MCM_ad_loaded, m_target->GetName(), MCtruemcstring);
                        break;
                    case kMCAdEventTypeReceiveFailed:
                        t_object->Get()->message_with_args(MCM_ad_load_failed, m_target->GetName());
                        break;
                    case kMCAdEventTypeClick:
                        t_object->Get()->message_with_args(MCM_ad_clicked, m_target->GetName());
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
    t_event = new MCAdEvent(p_ad, p_type);
    if (t_event != nil)
        MCEventQueuePostCustom(t_event);
}

////////////////////////////////////////////////////////////////////////////////

MCAd::MCAd(void)
{
	m_references = 1;
	m_id = ++s_last_ad_id;
	m_name = nil;
	m_object = nil;
	m_next = nil;
}

MCAd::~MCAd(void)
{
	if (m_object != nil)
	{
		m_object -> Release();
		m_object = nil;
	}
	
	if (m_name != nil)
	{
		MCCStringFree(m_name);
		m_name = nil;
	}
    
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

const char *MCAd::GetName(void)
{
	return m_name;
}

MCObjectHandle *MCAd::GetOwner(void)
{
	return m_object;
}

void MCAd::SetOwner(MCObjectHandle *p_owner)
{
	if (m_object != nil)
		m_object -> Release();
	m_object = p_owner;
}

bool MCAd::SetName(MCStringRef p_name)
{
	if (m_name != nil)
	{
		MCCStringFree(m_name);
		m_name = nil;
	}
	
	if (p_name != nil)
		return MCCStringClone(MCStringGetCString(p_name), m_name);
	
	return true;
}

MCAd *MCAd::GetFirst()
{
    return s_ads;
}

bool MCAd::FindByNameOrId(MCStringRef p_name, MCAd *&r_ad)
{
	char *t_id_end;
	uint32_t t_id;
	t_id = strtoul(MCStringGetCString(p_name), &t_id_end, 10);
	if (t_id_end != MCStringGetCString(p_name))
		return FindById(t_id, r_ad);
	
	for(MCAd *t_ad = s_ads; t_ad != nil; t_ad = t_ad -> m_next)
		if (t_ad -> GetName() != nil && MCCStringEqualCaseless(t_ad -> GetName(), MCStringGetCString(p_name)))
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

void MCAdExecRegisterWithInneractive(MCExecContext& ctxt, MCStringRef p_key)
{
    if (s_inneractive_ad_key != nil)
        MCCStringFree(s_inneractive_ad_key);
    
    if (MCCStringClone(MCStringGetCString(p_key), s_inneractive_ad_key))
        return;
    
    ctxt.Throw();
}

void MCAdExecCreateAd(MCExecContext& ctxt, MCStringRef p_name, MCStringRef p_type, uint32_t p_topleft_x, uint32_t p_topleft_y, MCArrayRef p_metadata)
{
    bool t_success;
    t_success = true;
    
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        t_success = false;;
    }
    
    MCAd *t_ad;
    t_ad = nil;
    
    if (t_success)
        if (MCAd::FindByNameOrId(p_name, t_ad))
        {
            ctxt.SetTheResultToStaticCString("ad already exists");            
            t_success = false;
        }
    
    if (t_success)
    {
        uint32_t t_timeout;
        MCAdType t_type;
        t_type = MCAdTypeFromCString(MCStringGetCString(p_type));
        
        if (t_success)
        {
            t_timeout = 0;
            MCNewAutoNameRef t_key;
            MCNameCreateWithCString("refresh", &t_key);
            
            MCValueRef t_value;
            
            if (p_metadata != nil && MCArrayFetchValue(p_metadata, false, *t_key, t_value))
                t_timeout = MCNumberFetchAsUnsignedInteger((MCNumberRef)t_value);
            if (t_type == kMCAdTypeFullscreen)
                t_timeout = 0;
            else if (t_timeout < 30 || t_timeout > 500)
                t_timeout = 120;
        }
        
        if (t_success)
        {
            MCAdTopLeft t_topleft;
            t_topleft.x = p_topleft_x;
            t_topleft.y = p_topleft_y;
            
            t_success = MCSystemInneractiveAdCreate(ctxt, t_ad, t_type, t_topleft, t_timeout, p_metadata);
        }
        
        if (t_success)
            t_success = t_ad->Create();
        
        if (t_success)
        {
            t_ad->SetNext(s_ads);
            t_ad->SetName(p_name);
            t_ad->SetOwner(ctxt.GetObjectHandle());
            s_ads = t_ad;
            
            return;
        }
        else if (t_ad != nil)
            t_ad->Release();
        
        ctxt.SetTheResultToStaticCString("could not create ad");
    }
    
    ctxt.Throw();
}

void MCAdExecDeleteAd(MCExecContext& ctxt, MCStringRef p_name)
{
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        return;
    }
    
    MCAd *t_ad;
    t_ad = nil;
    if (MCAd::FindByNameOrId(p_name, t_ad))
    {
        t_ad->Release();
        return;
    }
    
    ctxt.SetTheResultToStaticCString("could not find ad");
    ctxt.Throw();
}

void MCAdGetTopLeftOfAd(MCExecContext& ctxt, MCStringRef p_name, uint32_t& r_topleft_x, uint32_t& r_topleft_y)
{
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        ctxt.Throw();
    }
    
    MCAd *t_ad;
    t_ad = nil;
    if (MCAd::FindByNameOrId(p_name, t_ad))
    {
        r_topleft_x = t_ad->GetTopLeft().x;
        r_topleft_y = t_ad->GetTopLeft().y;
        return;
    }
    
    ctxt.SetTheResultToStaticCString("could not find ad");
    ctxt.Throw();
}

void MCAdSetTopLeftOfAd(MCExecContext& ctxt, MCStringRef p_name, uint32_t p_topleft_x, uint32_t p_topleft_y)
{
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        return;
    }
    
    MCAd *t_ad;
    t_ad = nil;
    if (MCAd::FindByNameOrId(p_name, t_ad))
    {
        MCAdTopLeft t_topleft;
        t_topleft.x = p_topleft_x;
        t_topleft.y = p_topleft_y;
        t_ad->SetTopLeft(t_topleft);
        
        return;
    }
    
    ctxt.SetTheResultToStaticCString("could not find ad");
    ctxt.Throw();
}

void MCAdGetVisibleOfAd(MCExecContext& ctxt,  MCStringRef p_name, bool &r_visible)
{
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        ctxt.Throw();
    }
    
    MCAd *t_ad;
    t_ad = nil;
    if (MCAd::FindByNameOrId(p_name, t_ad))
    {
        r_visible = t_ad->GetVisible();
        return;
    }
    
    ctxt.SetTheResultToStaticCString("could not find ad");
    ctxt.Throw();
}

void MCAdSetVisibleOfAd(MCExecContext& ctxt, MCStringRef p_name, bool p_visible)
{
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        ctxt.Throw();
        return;
    }
    
    MCAd *t_ad;
    t_ad = nil;
    if (!MCAd::FindByNameOrId(p_name, t_ad))
    {
        t_ad->SetVisible(p_visible);
        return;
    }
    
    ctxt.SetTheResultToStaticCString("could not find ad");
    ctxt.Throw();
}

void MCAdGetAds(MCExecContext& ctxt, MCStringRef& r_ads)
{
    bool t_success;
    t_success = true;
    MCAutoStringRef t_ads;
	for(MCAd *t_ad = s_ads; t_ad != nil && t_success; t_ad = t_ad->GetNext())
    {
		if (t_ad->GetName() != nil)
        {
            if (*t_ads == nil)
            {
                MCAutoStringRef t_first_ad;
                t_success = MCStringCreateWithCString(t_ad->GetName(), &t_first_ad);
                if (t_success)
                    t_success = MCStringMutableCopy(*t_first_ad, &t_ads);
            }
            else
                t_success = MCStringAppendFormat(*t_ads, "\n%s", t_ad->GetName());
        }
    }
    
    if (t_success)
    {
        r_ads = MCValueRetain(*t_ads);
        return;
    }
    
    ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////