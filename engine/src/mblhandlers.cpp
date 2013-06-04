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

#include "globals.h"
#include "stack.h"
#include "image.h"
#include "param.h"

#include "exec.h"

#include "mblsyntax.h"


////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleCanComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_result;
    MCTextMessagingGetCanComposeTextMessage(ctxt, t_result);
    
    if (t_result)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
    MCAutoStringRef t_recipients, t_body;
    
    bool t_success;
	MCExecPoint ep(nil, nil, nil);

	t_success = MCParseParameters(p_parameters, "x", &t_recipients);
    if (t_success == false)
    {
        MCresult -> sets(MCfalsestring);
        return ES_NORMAL;
    }
	t_success = MCParseParameters(p_parameters, "x", &t_body);
    
    ep . clear();
    MCExecContext ctxt(ep);
    
    if (t_success)
        MCTextMessagingExecComposeTextMessage(ctxt, *t_recipients, *t_body);
    
	return ES_NORMAL;
}


///////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleLockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCIdleTimerExecLockIdleTimer(ctxt);
    
    return ES_NORMAL;
}


Exec_stat MCHandleUnlockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCIdleTimerExecUnlockIdleTimer(ctxt);
    
    return ES_NORMAL;    
}

Exec_stat MCHandleIdleTimerLocked(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_result;
    
    MCIdleTimerGetIdleTimerLocked(ctxt, t_result);
    
    if (t_result)
        ctxt.SetTheResultToValue(kMCTrue);    
    else
        ctxt.SetTheResultToValue(kMCFalse);
    
    if (ctxt.HasError())
        return ES_ERROR;        
    else
        return ES_NORMAL;
}


///////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleCanMakePurchase(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_result;
    
    MCStoreGetCanMakePurchase(ctxt, t_result);
    
    if (t_result)
        ctxt.SetTheResultToValue(kMCTrue);
    else
        ctxt.SetTheResultToValue(kMCFalse);
    
    if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;    
}

Exec_stat MCHandleEnablePurchaseUpdates(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecEnablePurchaseUpdates(ctxt);
    
    if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}

Exec_stat MCHandleDisablePurchaseUpdates(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecDisablePurchaseUpdates(ctxt);
    
    if(ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}
    
Exec_stat MCHandleRestorePurchases(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecRestorePurchases(ctxt);
    
    if(ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}


Exec_stat MCHandlePurchaseList(void* p_context, MCParameter* p_parameters)
{
#ifdef MOBILE_BROKEN
    MCExecPoint ep(nil, nil, nil);
    MCPurchaseList(list_purchases, &ep);
    MCresult -> store(ep, False);
#endif
    return ES_NORMAL;
}

Exec_stat MCHandlePurchaseCreate(void* p_context, MCParameter* p_parameters)
{
    bool t_success = true;
    MCAutoStringRef t_product_id;
    uint32_t t_id;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &t_product_id);
    
    if (t_success)
        MCStoreExecCreatePurchase(ctxt, &t_product_id, t_id);
    
    if (ctxt.HasError())
    {
        ctxt.SetTheResultToEmpty();
        return ES_ERROR;
    }
    else
    {
        ctxt.SetTheResultToNumber(t_id);
        return ES_NORMAL;
    }
}


Exec_stat MCHandlePurchaseState(void* p_context, MCParameter* p_parameters)
{    
	bool t_success = true;
	
	uint32_t t_id;
    MCAutoStringRef t_state;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
    
	if (t_success)
		MCStoreGetPurchaseState(ctxt, t_id, &t_state);
	
	if (ctxt.HasError())
    {
        ctxt.SetTheResultToEmpty();
        return ES_ERROR;}
    else
    {
        ctxt.SetTheResultToValue(*t_state);
        return ES_NORMAL;
    }
}


Exec_stat MCHandlePurchaseError(void* p_context, MCParameter* p_parameters)
{
	bool t_success = true;
	
    MCStringRef t_error;
	uint32_t t_id;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
	if (t_success)
        MCStoreGetPurchaseError(ctxt, t_id, t_error);
    
    if (ctxt.HasError())
    {
        ctxt.SetTheResultToEmpty();
        return ES_ERROR;
    }
    else
    {
        ctxt.SetTheResultToValue(t_error);
        return ES_NORMAL;
    }
}

Exec_stat MCHandlePurchaseGet(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	MCAutoStringRef t_prop_name;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "us", &t_id, &t_prop_name);
	
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
	if (t_success)
        MCStoreGetPurchaseProperty(ctxt, t_id, &t_prop_name);
	
	if (!ctxt . HasError())
    {
        MCAutoStringRef t_string;
        /* UNCHECKED */ ep . copyasstringref(&t_string);
		ctxt . SetTheResultToValue(*t_string);
        return ES_NORMAL;
    }
    
    ctxt . SetTheResultToEmpty();
	return ES_ERROR;
}


Exec_stat MCHandlePurchaseSet(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	MCAutoStringRef t_prop_name;
    uint32_t t_quantity;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "uxu", &t_id, &t_prop_name, &t_quantity);
		
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	if (t_success)
        MCStoreSetPurchaseProperty(ctxt, t_id, &t_prop_name, t_quantity);
	
    if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}



Exec_stat MCHandlePurchaseSendRequest(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        MCStoreExecSendPurchaseRequest(ctxt, t_id);
    
	if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}

Exec_stat MCHandlePurchaseConfirmDelivery(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	if (t_success)
        MCStoreExecConfirmPurchaseDelivery(ctxt, t_id);
    
    if (ctxt.HasError())
        return ES_ERROR;
	else
        return ES_NORMAL;
}

