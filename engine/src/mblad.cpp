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
#include "execpt.h"
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

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
void MCSystemInneractiveAdInit();
bool MCSystemInneractiveAdCreate(MCExecContext &ctxt, MCAd*& r_ad, MCAdType p_type, MCAdTopLeft p_top_left, uint32_t p_timeout, MCVariableValue *p_meta_data);

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

static MCAdType MCAdTypeFromCString(const char *p_string)
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

bool MCAd::SetName(const char *p_name)
{
	if (m_name != nil)
	{
		MCCStringFree(m_name);
		m_name = nil;
	}
	
	if (p_name != nil)
		return MCCStringClone(p_name, m_name);
	
	return true;
}

MCAd *MCAd::GetFirst()
{
    return s_ads;
}

bool MCAd::FindByNameOrId(const char *p_name, MCAd *&r_ad)
{   
	char *t_id_end;
	uint32_t t_id;
	t_id = strtoul(p_name, &t_id_end, 10);
	if (t_id_end != p_name)
		return FindById(t_id, r_ad);
	
	for(MCAd *t_ad = s_ads; t_ad != nil; t_ad = t_ad -> m_next)
		if (t_ad -> GetName() != nil && MCCStringEqualCaseless(t_ad -> GetName(), p_name))
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

void MCAdExecRegisterWithInneractive(MCExecContext& ctxt, const char *p_key)
{
#ifdef /* MCAdExecRegisterWithInneractive */ LEGACY_EXEC
    MCCStringFree(s_inneractive_ad_key);
    /* UNCHECKED */ MCCStringClone(p_key, s_inneractive_ad_key);
#endif /* MCAdExecRegisterWithInneractive */
}

void MCAdExecCreateAd(MCExecContext& ctxt, const char *p_name, MCAdType p_type, MCAdTopLeft p_top_left, MCVariableValue *p_meta_data)
{
#ifdef /* MCAdExecCreateAd */ LEGACY_EXEC
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
        if (t_success)
        {
            t_timeout = 0;
            if (p_meta_data != nil && p_meta_data->fetch_element_if_exists(ctxt.GetEP(), "refresh", false))
                t_timeout = ctxt.GetEP().getint4();
            if (p_type == kMCAdTypeFullscreen)
                t_timeout = 0;
            else if (t_timeout < 30 || t_timeout > 500)
                t_timeout = 120;
        }
        
        if (t_success)
            t_success = MCSystemInneractiveAdCreate(ctxt, t_ad, p_type, p_top_left, t_timeout, p_meta_data);
        
        if (t_success)
            t_success = t_ad->Create();
        
        if (t_success)
        {
            t_ad->SetNext(s_ads);
            t_ad->SetName(p_name);
            t_ad->SetOwner(ctxt.GetObjectHandle());
            s_ads = t_ad;        
        }
        else if (t_ad != nil)
            t_ad->Release();
               
    }

	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not create ad");
#endif /* MCAdExecCreateAd */
}

void MCAdExecDeleteAd(MCExecContext& ctxt, const char *p_name)
{
#ifdef /* MCAdExecDeleteAd */ LEGACY_EXEC
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        return;
    }

    MCAd *t_ad;
    t_ad = nil;    
    if (!MCAd::FindByNameOrId(p_name, t_ad))
        ctxt.SetTheResultToStaticCString("could not find ad");
    else 
        t_ad->Release();
#endif /* MCAdExecDeleteAd */
}

bool MCAdGetTopLeftOfAd(MCExecContext& ctxt, const char *p_name, MCAdTopLeft &r_top_left)
{
#ifdef /* MCAdGetTopLeftOfAd */ LEGACY_EXEC
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        return false;
    }

    MCAd *t_ad;
    t_ad = nil;    
    if (!MCAd::FindByNameOrId(p_name, t_ad))
    {
        ctxt.SetTheResultToStaticCString("could not find ad");
        return false;
    }
    else
    {
        r_top_left = t_ad->GetTopLeft();    
        return true;
    }
#endif /* MCAdGetTopLeftOfAd */
}

void MCAdSetTopLeftOfAd(MCExecContext& ctxt, const char *p_name, MCAdTopLeft p_top_left)
{
#ifdef /* MCAdSetTopLeftOfAd */ LEGACY_EXEC
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        return;
    }

    MCAd *t_ad;
    t_ad = nil;    
    if (!MCAd::FindByNameOrId(p_name, t_ad))
        ctxt.SetTheResultToStaticCString("could not find ad");
    else
        t_ad->SetTopLeft(p_top_left);
#endif /* MCAdSetTopLeftOfAd */
}
bool MCAdGetVisibleOfAd(MCExecContext& ctxt, const char *p_name, bool &r_visible)
{
#ifdef /* MCAdGetVisibleOfAd */ LEGACY_EXEC
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        return false;
    }

    MCAd *t_ad;
    t_ad = nil;    
    if (!MCAd::FindByNameOrId(p_name, t_ad))
    {
        ctxt.SetTheResultToStaticCString("could not find ad");
        return false;
    }
    else
    {
        r_visible = t_ad->GetVisible();
        return true;
    }
#endif /* MCAdGetVisibleOfAd */
}

void MCAdSetVisibleOfAd(MCExecContext& ctxt, const char *p_name, bool p_visible)
{
#ifdef /* MCAdSetVisibleOfAd */ LEGACY_EXEC
    if (s_inneractive_ad_key == nil || MCCStringLength(s_inneractive_ad_key) == 0)
    {
        ctxt.SetTheResultToStaticCString("not registered with ad service");
        return;
    }

    MCAd *t_ad;
    t_ad = nil;    
    if (!MCAd::FindByNameOrId(p_name, t_ad))
        ctxt.SetTheResultToStaticCString("could not find ad");
    else
        t_ad->SetVisible(p_visible);
#endif /* MCAdSetVisibleOfAd */
}

bool MCAdGetAds(MCExecContext& ctxt, char*& r_ads)
{
#ifdef /* MCAdGetAds */ LEGACY_EXEC
    bool t_success;
    t_success = true;
	for(MCAd *t_ad = s_ads; t_ad != nil && t_success; t_ad = t_ad->GetNext())
		if (t_ad->GetName() != nil)
        {
            if (r_ads == nil)
                t_success = MCCStringClone(t_ad->GetName(), r_ads);
            else
                t_success = MCCStringAppendFormat(r_ads, "\n%s", t_ad->GetName());
        }
    return t_success;
#endif /* MCAdGetAds */
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleAdRegister(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAdRegister */ LEGACY_EXEC
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	char *t_key;
	t_key = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_key);
	
	if (t_success)
		MCAdExecRegisterWithInneractive(t_ctxt, t_key);
    
    MCCStringFree(t_key);
    
    return t_ctxt.GetStat();
#endif /* MCHandleAdRegister */
}


Exec_stat MCHandleAdCreate(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAdCreate */ LEGACY_EXEC
	bool t_success;
	t_success = true;
		
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    	
	char *t_ad;
	t_ad = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_ad);	
    
    MCAdType t_type;
    t_type = kMCAdTypeUnknown;
    if (t_success)
    {
        char *t_type_string;
        t_type_string = nil;
        if (MCParseParameters(p_parameters, "s", &t_type_string))
            t_type = MCAdTypeFromCString(t_type_string);
        MCCStringFree(t_type_string);
    }
    
    MCAdTopLeft t_top_left = {0,0};
    if (t_success)
    {
        char *t_top_left_string;
        t_top_left_string = nil;
        if (MCParseParameters(p_parameters, "s", &t_top_left_string))
            /* UNCHECKED */ sscanf(t_top_left_string, "%u,%u", &t_top_left.x, &t_top_left.y);
        MCCStringFree(t_top_left_string);
    }
    
    MCVariableValue *t_meta_data;
    t_meta_data = nil;
    if (t_success)
        MCParseParameters(p_parameters, "a", &t_meta_data);

	if (t_success)
		MCAdExecCreateAd(t_ctxt, t_ad, t_type, t_top_left, t_meta_data);
    
    MCCStringFree(t_ad);
	    
    return t_ctxt.GetStat();
#endif /* MCHandleAdCreate */
}

Exec_stat MCHandleAdDelete(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAdDelete */ LEGACY_EXEC
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	char *t_ad;
	t_ad = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_ad);
	
	if (t_success)
		MCAdExecDeleteAd(t_ctxt, t_ad);
    
    MCCStringFree(t_ad);
    
    return t_ctxt.GetStat();
#endif /* MCHandleAdDelete */
}

Exec_stat MCHandleAdGetVisible(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAdGetVisible */ LEGACY_EXEC
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	char *t_ad;
	t_ad = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_ad);
	
    bool t_visible;
    t_visible = false;
	if (t_success)
		t_success = MCAdGetVisibleOfAd(t_ctxt, t_ad, t_visible);
    
    if (t_success)
        MCresult->sets(MCU_btos(t_visible));
    
    MCCStringFree(t_ad);
    
    return t_ctxt.GetStat();
#endif /* MCHandleAdGetVisible */
}

Exec_stat MCHandleAdSetVisible(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAdSetVisible */ LEGACY_EXEC
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	char *t_ad;
	t_ad = nil;
    bool t_visible;
    t_visible = false;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "sb", &t_ad, &t_visible);
	
	if (t_success)
		MCAdSetVisibleOfAd(t_ctxt, t_ad, t_visible);
    
    MCCStringFree(t_ad);
    
    return t_ctxt.GetStat();
#endif /* MCHandleAdSetVisible */
}

Exec_stat MCHandleAdGetTopLeft(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAdGetTopLeft */ LEGACY_EXEC
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	char *t_ad;
	t_ad = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_ad);
	
    MCAdTopLeft t_top_left = {0,0};
	if (t_success)
		t_success = MCAdGetTopLeftOfAd(t_ctxt, t_ad, t_top_left);
    
    if (t_success)
    {
        MCAutoRawCString t_top_left_string;
        t_success = MCCStringFormat(t_top_left_string, "%u,%u", t_top_left.x, t_top_left.y);
        if (t_success)
            if (t_top_left_string.Borrow() != nil)
                ep.copysvalue(t_top_left_string.Borrow());
    }
    
    if (t_success)
        MCresult->store(ep, False);
    
    MCCStringFree(t_ad);
    
    return t_ctxt.GetStat();
#endif /* MCHandleAdGetTopLeft */
}

Exec_stat MCHandleAdSetTopLeft(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAdSetTopLeft */ LEGACY_EXEC
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	char *t_ad;
	t_ad = nil;
    char *t_top_left_string;
    t_top_left_string = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_ad, &t_top_left_string);
    
    MCAdTopLeft t_top_left = {0,0};
    if (t_success)
        t_success = sscanf(t_top_left_string, "%u,%u", &t_top_left.x, &t_top_left.y);
    
	if (t_success)
		MCAdSetTopLeftOfAd(t_ctxt, t_ad, t_top_left);
    
    MCCStringFree(t_top_left_string);
    MCCStringFree(t_ad);
    
    return t_ctxt.GetStat();
#endif /* MCHandleAdSetTopLeft */
}

Exec_stat MCHandleAds(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAds */ LEGACY_EXEC
    bool t_success;
    t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
        
    if (t_success)
    {
        MCAutoRawCString t_ads;
        t_success = MCAdGetAds(t_ctxt, t_ads);
        if (t_success && t_ads.Borrow() != nil)
            ep.copysvalue(t_ads.Borrow());
    }

    if (t_success)
        MCresult->store(ep, False);
    
    return t_ctxt.GetStat();
#endif /* MCHandleAds */
}

////////////////////////////////////////////////////////////////////////////////
