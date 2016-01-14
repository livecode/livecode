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

#include "sysdefs.h"
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

#include "util.h"

#include "mblsyntax.h"
#include "mblsensor.h"
#include "mblcontrol.h"
#include "mblstore.h"

#include "foundation-chunk.h"


////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

static const char *s_orientation_names[] =
{
	"unknown", "portrait", "portrait upside down", "landscape right", "landscape left", "face up", "face down", nil
};

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleCanComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCanComposeTextMessage */ LEGACY_EXEC
    if (MCSystemCanSendTextMessage())
        MCresult -> sets(MCtruestring);
	else
        MCresult -> sets(MCfalsestring);
	return ES_NORMAL;
#endif /* MCHandleCanComposeTextMessage */
    MCExecContext ctxt(nil, nil, nil);
    
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
#ifdef /* MCHandleComposeTextMessage */ LEGACY_EXEC
    char *t_recipients, *t_body;
    bool t_success;
	MCExecPoint ep(nil, nil, nil);
	ep . clear();
	if (!MCSystemCanSendTextMessage())
    {
        MCresult -> sets(MCfalsestring);
        return ES_NORMAL;
    }
	
	t_success = MCParseParameters(p_parameters, "s", &t_recipients);
    if (t_success == false)
    {
        MCresult -> sets(MCfalsestring);
        return ES_NORMAL;
    }
	t_success = MCParseParameters(p_parameters, "s", &t_body);
    
    MCExecContext t_ctxt(ep);
    
	MCComposeTextMessageExec(t_ctxt, t_recipients, t_body);
    
	return ES_NORMAL;
#endif /* MCHandleComposeTextMessage */
    MCAutoStringRef t_recipients, t_body;
    
    bool t_success;
    MCExecContext ctxt(nil, nil, nil);
    
	t_success = MCParseParameters(p_parameters, "x", &(&t_recipients));
    if (t_success == false)
    {
         ctxt . SetTheResultToValue(kMCFalse);
        return ES_NORMAL;
    }
    t_success = MCParseParameters(p_parameters, "x", &(&t_body));
  
    
    if (t_success)
        MCTextMessagingExecComposeTextMessage(ctxt, *t_recipients, *t_body);
    
	return ES_NORMAL;
}


///////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleLockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCIdleTimerExecLockIdleTimer(ctxt);
    
    return ES_NORMAL;
}


Exec_stat MCHandleUnlockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCIdleTimerExecUnlockIdleTimer(ctxt);
    
    return ES_NORMAL;    
}

Exec_stat MCHandleIdleTimerLocked(void* p_context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
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
#ifdef /* MCHandleCanMakePurchase */ LEGACY_EXEC
	MCresult -> sets(MCU_btos( MCStoreCanMakePurchase() ));
	return ES_NORMAL;
#endif /* MCHandleCanMakePurchase */
    MCExecContext ctxt(nil, nil, nil);
    
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
#ifdef /* MCHandleEnablePurchaseUpdates */ LEGACY_EXEC
	MCStoreEnablePurchaseUpdates();
	return ES_NORMAL;
#endif /* MCHandleEnablePurchaseUpdates */
    MCExecContext ctxt(nil, nil, nil);
    
    MCStoreExecEnablePurchaseUpdates(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDisablePurchaseUpdates(void* p_context, MCParameter* p_parameters)
{
#ifdef /* MCHandleDisablePurchaseUpdates */ LEGACY_EXEC
	MCStoreDisablePurchaseUpdates();
	return ES_NORMAL;
#endif /* MCHandleDisablePurchaseUpdates */
    MCExecContext ctxt(nil, nil, nil);
    
    MCStoreExecDisablePurchaseUpdates(ctxt);
    
    if(!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}
    
Exec_stat MCHandleRestorePurchases(void* p_context, MCParameter* p_parameters)
{
#ifdef /* MCHandleRestorePurchases */ LEGACY_EXEC
	MCStoreRestorePurchases();
	return ES_NORMAL;
#endif /* MCHandleRestorePurchases */
    MCExecContext ctxt(nil, nil, nil);
    
    MCStoreExecRestorePurchases(ctxt);
    
    if(!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandlePurchaseList(void* p_context, MCParameter* p_parameters)
{
#ifdef /* MCHandlePurchaseList */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);
	MCPurchaseList(list_purchases, &ep);
	MCresult -> store(ep, False);
	return ES_NORMAL;
#endif /* MCHandlePurchaseList */
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_list;
    
    MCStoreGetPurchaseList(ctxt, &t_list);
    
    if(!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_list);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseCreate(void* p_context, MCParameter* p_parameters)
{
#ifdef /* MCHandlePurchaseCreate */ LEGACY_EXEC
	bool t_success = true;
	char *t_product_id = nil;
	MCPurchase *t_purchase = nil;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_product_id);
	
	if (t_success)
		MCPurchaseCreate(t_product_id, nil, t_purchase);
	
	if (t_success)
		MCresult->setnvalue(t_purchase->id);
	
	MCCStringFree(t_product_id);
    
	return ES_NORMAL;
#endif /* MCHandlePurchaseCreate */
    bool t_success = true;
    MCAutoStringRef t_product_id;
    uint32_t t_id;
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_product_id));
    
    if (t_success)
        MCStoreExecCreatePurchase(ctxt, *t_product_id, t_id);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToNumber(t_id);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}


Exec_stat MCHandlePurchaseState(void* p_context, MCParameter* p_parameters)
{
#ifdef /* MCHandlePurchaseState */ LEGACY_EXEC
	bool t_success = true;
	
	uint32_t t_id;
	const char *t_state = nil;
	MCPurchase *t_purchase = nil;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	if (t_success)
		t_success = MCPurchaseFindById(t_id, t_purchase);
	
	if (t_success)
		t_success = MCPurchaseStateToString(t_purchase->state, t_state);
	
	if (t_success)
		MCresult -> sets(t_state);
	
	return ES_NORMAL;
#endif /* MCHandlePurchaseState */
	bool t_success = true;
	
	uint32_t t_id;
    MCAutoStringRef t_state;
    
    MCExecContext ctxt(nil, nil, nil);
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
    
	if (t_success)
		MCStoreGetPurchaseState(ctxt, t_id, &t_state);
	
	if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_state);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}


Exec_stat MCHandlePurchaseError(void* p_context, MCParameter* p_parameters)
{
#ifdef /* MCHandlePurchaseError */ LEGACY_EXEC
	bool t_success = true;
	
	uint32_t t_id;
	char *t_error = nil;
	MCPurchase *t_purchase = nil;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
	if (t_success)
		t_success = MCPurchaseFindById(t_id, t_purchase);
	
	if (t_success)
		t_success = MCPurchaseGetError(t_purchase, t_error);
	
	if (t_success)
		MCresult->grab(t_error, MCCStringLength(t_error));
	else
		MCCStringFree(t_error);
	
	return ES_NORMAL;
#endif /* MCHandlePurchaseError */
	bool t_success = true;
	
    MCStringRef t_error;
	uint32_t t_id;
    
    MCExecContext ctxt(nil, nil, nil);
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
	if (t_success)
        MCStoreGetPurchaseError(ctxt, t_id, t_error);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(t_error);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseGet(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePurchaseGet */ LEGACY_EXEC
	bool t_success = true;
	
	uint32_t t_id;
	char *t_prop_name = nil;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "us", &t_id, &t_prop_name);
	
	MCPurchase *t_purchase = nil;
	MCPurchaseProperty t_property;
	
	if (t_success)
		t_success =
		MCPurchaseFindById(t_id, t_purchase) &&
		MCPurchaseLookupProperty(t_prop_name, t_property);
	
	MCExecPoint ep(nil, nil, nil);
	if (t_success)
		t_success = MCPurchaseGet(t_purchase, t_property, ep) == ES_NORMAL;
	
	if (t_success)
		MCresult->store(ep, True);
	else
		MCresult->clear();
    
	MCCStringFree(t_prop_name);
	
	return ES_NORMAL;
#endif /* MCHandlePurchaseGet */
	bool t_success = true;
	
	uint32_t t_id;
	MCAutoStringRef t_prop_name;
    MCAutoValueRef t_value;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ux", &t_id, &(&t_prop_name));
	
    MCExecContext ctxt(nil, nil, nil);
	
	if (t_success)
        MCStoreExecGet(ctxt, t_id, *t_prop_name, &t_value);
	
	if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_value);
        return ES_NORMAL;
    }
    
    ctxt . SetTheResultToEmpty();
	return ES_ERROR;
}


Exec_stat MCHandlePurchaseSet(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePurchaseSet */ LEGACY_EXEC
	bool t_success = true;
	
	uint32_t t_id;
	char *t_prop_name = nil;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "us", &t_id, &t_prop_name);
	
	MCPurchase *t_purchase = nil;
	MCPurchaseProperty t_property;
	
	if (t_success)
		t_success =
		MCPurchaseFindById(t_id, t_purchase) &&
		MCPurchaseLookupProperty(t_prop_name, t_property);
	
	MCExecPoint ep(nil, nil, nil);
	if (t_success && p_parameters != nil)
		t_success = p_parameters -> eval(ep) == ES_NORMAL;
	
	if (t_success)
		t_success = MCPurchaseSet(t_purchase, t_property, ep) == ES_NORMAL;
	
	MCCStringFree(t_prop_name);
	
	return ES_NORMAL;
#endif /* MCHandlePurchaseSet */
	bool t_success = true;
	
	uint32_t t_id;
	MCAutoStringRef t_prop_name;
    uint32_t t_quantity;
    MCAutoNumberRef t_quantity_ref;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "uxu", &t_id, &(&t_prop_name), &t_quantity);
		
	MCExecContext ctxt(nil, nil, nil);
    
	if (t_success)
        t_success = MCNumberCreateWithInteger(t_quantity, &t_quantity_ref);

    if (t_success)
        MCStoreExecSet(ctxt, t_id, &t_prop_name, *t_quantity_ref);
	
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseSendRequest(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePurchaseSendRequest */ LEGACY_EXEC
	bool t_success = true;
	
	uint32_t t_id;
	MCPurchase *t_purchase = nil;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
    
	if (t_success)
		t_success = MCPurchaseFindById(t_id, t_purchase);
	
	if (t_success)
		t_success = MCPurchaseSendRequest(t_purchase);
	
	return ES_NORMAL;
#endif /* MCHandlePurchaseSendRequest */
	bool t_success = true;
	
	uint32_t t_id;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecSendPurchaseRequest(ctxt, t_id);
    
	if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleMakePurchase(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleMakePurchase */ LEGACY_EXEC
    bool t_success = true;
    
    char  *t_prod_id;
    char  *t_quantity;
    char  *t_payload;
    MCPurchase *t_purchase = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "sss", &t_prod_id, &t_quantity, &t_payload);
    
    
    if (t_success)
        t_success = MCPurchaseCreate(t_prod_id, nil, t_purchase);
    
    //if (t_success)
    //t_success = MCStoreMakePurchase(t_purchase);
    if (t_success)
        t_success = MCStoreMakePurchase(t_purchase->prod_id, t_quantity, t_payload);
    
    MCCStringFree(t_prod_id);
    MCCStringFree(t_quantity);
    MCCStringFree(t_payload);
    
    return ES_NORMAL;
#endif /* MCHandleMakePurchase */
    bool t_success = true;
    
    MCAutoStringRef t_prod_id;
    MCAutoStringRef t_quantity;
    MCAutoStringRef t_payload;
    MCPurchase *t_purchase = nil;
    uint32_t t_id;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "xxx", &(&t_prod_id), &(&t_quantity), &(&t_payload));
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecCreatePurchase(ctxt, *t_prod_id, t_id);
   
    if (t_success)
        MCStoreExecMakePurchase(ctxt, *t_prod_id, *t_quantity, *t_payload);
    
    if (!ctxt. HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleConfirmPurchase(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleConfirmPurchase */ LEGACY_EXEC
    bool t_success = true;
    
    char *t_prod_id;
    MCPurchase *t_purchase = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "s", &t_prod_id);
    
    if (t_success)
        t_success = MCPurchaseFindByProdId(t_prod_id, t_purchase);
    
    if (t_success)
        t_success = MCPurchaseConfirmDelivery(t_purchase);
    
    return ES_NORMAL;
#endif /* MCHandleConfirmPurchase */
    bool t_success = true;
    
    MCAutoStringRef t_prod_id;
    MCPurchase *t_purchase = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_prod_id));
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecConfirmPurchase(ctxt, *t_prod_id);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleProductSetType(void *context, MCParameter *p_parameter)
{
#ifdef /* MCHandleProductSetType */ LEGACY_EXEC
    bool t_success = true;
    char *t_product_id = nil;
    char *t_product_type;
    if (t_success)
        t_success = MCParseParameters(p_parameters, "ss", &t_product_id, &t_product_type);
    if (t_success)
        t_success = MCStoreProductSetType(t_product_id, t_product_type);
    
    MCCStringFree(t_product_id);
    MCCStringFree(t_product_type);
    
    return ES_NORMAL;
#endif /* MCHandleProductSetType */
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_product_id, t_product_type;
    bool t_success;
    
    t_success = MCParseParameters(p_parameter, "xx", &(&t_product_id), &(&t_product_type));
    
    if (t_success)
        MCStoreExecProductSetType(ctxt, *t_product_id, *t_product_type);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandlePurchaseConfirmDelivery(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePurchaseConfirmDelivery */ LEGACY_EXEC
	bool t_success = true;
	
	uint32_t t_id;
	MCPurchase *t_purchase = nil;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
	if (t_success)
		t_success = MCPurchaseFindById(t_id, t_purchase);
	
	if (t_success)
		t_success = MCPurchaseConfirmDelivery(t_purchase);
	
	return ES_NORMAL;
#endif /* MCHandlePurchaseConfirmDelivery */
	bool t_success = true;
	
	uint32_t t_id;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
    MCExecContext ctxt(nil, nil, nil);
    
	if (t_success)
        MCStoreExecConfirmPurchaseDelivery(ctxt, t_id);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandleRequestProductDetails(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleRequestProductDetails */ LEGACY_EXEC
    bool t_success = true;
    
    char * t_product;
    MCPurchase *t_purchase = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "s", &t_product);
    
    if (t_success)
        MCStoreRequestProductDetails(t_product);
    
    return ES_NORMAL;
#endif /* MCHandleRequestProductDetails */
    MCAutoStringRef t_product;
    bool t_success = true;    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_product));
        
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecRequestProductDetails(ctxt, *t_product);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleReceiveProductDetails(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleReceiveProductDetails */ LEGACY_EXEC
    bool t_success = true;
    char *t_product_id = nil;
    const char* t_product_details;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "s", &t_product_id);
    if (t_success)
        t_product_details = MCStoreReceiveProductDetails(t_product_id);
    
    MCCStringFree(t_product_id);
    MCresult -> sets(t_product_details);
    
    return ES_NORMAL;
#endif /* MCHandleReceiveProductDetails */
    MCAutoStringRef t_product;
    bool t_success = true;
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_product));
    
    MCExecContext ctxt(nil, nil, nil);
    MCAutoStringRef t_result;
    
    if (t_success)
        MCStoreExecReceiveProductDetails(ctxt, *t_product, &t_result);
    
    ctxt . SetTheResultToValue(*t_result);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleConsumePurchase(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleConsumePurchase */ LEGACY_EXEC
    bool t_success = true;
    char *t_product_id = nil;
    if (t_success)
        t_success = MCParseParameters(p_parameters, "s", &t_product_id);
    if (t_success)
        t_success = MCStoreConsumePurchase(t_product_id);
    
    MCCStringFree(t_product_id);
    return ES_NORMAL;
#endif /* MCHandleConsumePurchase */
    MCAutoStringRef t_product;
    bool t_success = true;
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_product));
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecConsumePurchase(ctxt, *t_product);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetPurchases(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetPurchases */ LEGACY_EXEC
    char *t_purchases;
    t_purchases = MCStoreGetPurchaseList();
    
    MCresult -> sets(t_purchases);
    return ES_NORMAL;
#endif /* MCHandleGetPurchases */
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_purchases;
    MCStoreGetPurchaseList(ctxt, &t_purchases);
    
    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_purchases);
        return ES_NORMAL;
    }
    
    ctxt . SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseVerify(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePurchaseVerify */ LEGACY_EXEC
    bool t_success = true;
    
    bool t_verified = true;
    uint32_t t_id;
    MCPurchase *t_purchase = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "ub", &t_id, &t_verified);
    
    if (t_success)
        t_success = MCPurchaseFindById(t_id, t_purchase);
    
    if (t_success)
        MCPurchaseVerify(t_purchase, t_verified);
    
    return ES_NORMAL;
#endif /* MCHandlePurchaseVerify */
    bool t_success = true;
    bool t_verified = true;
    
    uint32_t t_id;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "ub", &t_id, &t_verified);
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCStoreExecPurchaseVerify(ctxt, t_id, t_verified);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetPurchaseProperty(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetPurchaseProperty */ LEGACY_EXEC
    bool t_success = true;
	
	char *t_product_id = nil;
    char *t_prop_name = nil;
    const char *t_prop_value = nil;
    
	if (t_success)
        t_success = MCParseParameters(p_parameters, "ss", &t_product_id, &t_prop_name);
	if (t_success)
        t_prop_value = MCStoreGetPurchaseProperty(t_product_id, t_prop_name);
    
    MCCStringFree(t_product_id);
    MCCStringFree(t_prop_name);
    
    MCresult -> sets(t_prop_value);
    return ES_NORMAL;
#endif /* MCHandleGetPurchaseProperty */

    bool t_success = true;
    MCAutoStringRef t_product_id, t_prop_name, t_prop_value;
    MCExecContext ctxt(nil,nil,nil);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "xx", &(&t_product_id), &(&t_prop_name));
    
    if (t_success)
        MCStoreGetPurchaseProperty(ctxt, *t_product_id, *t_prop_name, &t_prop_value);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_prop_value);
        return ES_NORMAL;
    }
    
    return ES_ERROR;
}

Exec_stat MCHandleSetPurchaseProperty(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetPurchaseProperty */ LEGACY_EXEC
    bool t_success = true;
    char *t_product_id = nil;
    char *t_prop_name = nil;
    char *t_prop_value = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "sss", &t_product_id, &t_prop_name, &t_prop_value);
    if (t_success)
        t_success = MCStoreSetPurchaseProperty(t_product_id, t_prop_name, t_prop_value);
    
    MCCStringFree(t_product_id);
    MCCStringFree(t_prop_name);
    MCCStringFree(t_prop_value);
    
    return ES_NORMAL;
#endif /* MCHandleSetPurchaseProperty */

    bool t_success = true;
    MCAutoStringRef t_product_id, t_prop_name, t_prop_value;
    MCExecContext ctxt(nil,nil,nil);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "xxx", &(&t_product_id), &(&t_prop_name), &(&t_prop_value));
    if (t_success)
        MCStoreSetPurchaseProperty(ctxt, *t_product_id, *t_prop_name, *t_prop_value);
    
    if (ctxt.HasError())
        return ES_ERROR;
    
    return ES_NORMAL;
    
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleDeviceOrientation(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	intenum_t t_orientation;
	MCOrientationGetDeviceOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)t_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientation(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	intenum_t t_orientation;
	MCOrientationGetOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)t_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	intset_t t_orientations;
	MCOrientationGetAllowedOrientations(ctxt, t_orientations);

	bool t_success;
	t_success = true;
	
	MCAutoListRef t_orientation_list;

	if (t_success)
		t_success = MCListCreateMutable(',', &t_orientation_list);

	for (uint32_t j = 0; s_orientation_names[j] != nil; j++)
	{
		if ((t_orientations & (1 << j)) != 0)
		{		
			MCAutoStringRef t_orientation;
			if (t_success)
				t_success = MCStringFormat(&t_orientation, "%s", s_orientation_names[j]);

			if (t_success)
				t_success = MCListAppend(*t_orientation_list, *t_orientation);
		}
	}

	MCAutoStringRef t_result;
	if (t_success)
		t_success = MCListCopyAsString(*t_orientation_list, &t_result);

	if (t_success)
	{
		ctxt . SetTheResultToValue(*t_result);
		return ES_NORMAL;
	}

	return ES_ERROR;
}

Exec_stat MCHandleSetAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	MCAutoStringRef t_orientations;
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_orientations);
	}

    bool t_success = true;
    MCAutoArrayRef t_orientations_array;
    if (t_success)
        t_success = MCStringSplit(*t_orientations, MCSTR(","), nil, kMCCompareExact, &t_orientations_array);
    
    uint32_t t_orientations_count = MCArrayGetCount(*t_orientations_array);
    intset_t t_orientations_set;
	t_orientations_set = 0;
	if (t_success)
    {
		for(uint32_t i = 0; i < t_orientations_count; i++)
        {
            // Note: 't_orientations_array' is an array of strings
			MCValueRef t_orien_value = nil;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_orientations_array, i + 1, t_orien_value);
			MCStringRef t_orientation = (MCStringRef)(t_orien_value);
            if (MCStringIsEqualToCString(t_orientation, "portrait", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_PORTRAIT;
            else if (MCStringIsEqualToCString(t_orientation, "portrait upside down", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_PORTRAIT_UPSIDE_DOWN;
            else if (MCStringIsEqualToCString(t_orientation, "landscape right", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_LANDSCAPE_RIGHT;
            else if (MCStringIsEqualToCString(t_orientation, "landscape left", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_LANDSCAPE_LEFT;
            else if (MCStringIsEqualToCString(t_orientation, "face up", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_FACE_UP;
            else if (MCStringIsEqualToCString(t_orientation, "face down", kMCCompareCaseless))
                t_orientations_set |= ORIENTATION_FACE_DOWN;
            
        }
	}

	MCOrientationSetAllowedOrientations(ctxt, t_orientations_set);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	bool t_locked;
	MCOrientationGetOrientationLocked(ctxt, t_locked);

    if (t_locked)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleLockOrientation(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	MCOrientationExecLockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUnlockOrientation(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	MCOrientationExecUnlockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleRotateInterface(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleRotateInterface */ LEGACY_EXEC
	return ES_NORMAL;
#endif /* MCHandleRotateInterface */
	return ES_NORMAL;
}


////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleRevMail(void *context, MCParameter *p_parameters)
{
	MCAutoStringRef t_address, t_cc_address, t_subject, t_message_body;

	MCExecContext ctxt(nil, nil, nil);
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_address);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_cc_address);
        p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_subject);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        ctxt . ConvertToString(*t_value, &t_message_body);
		p_parameters = p_parameters -> getnext();
	}

	MCMailExecSendEmail(ctxt, *t_address, *t_cc_address, *t_subject, *t_message_body);
	
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposeMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &(&t_subject), &(&t_to), &(&t_cc), &(&t_bcc), &(&t_body), &(&t_attachments));

	MCExecContext ctxt(nil, nil, nil);

	if (t_success)
		MCMailExecComposeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposePlainMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &(&t_subject), &(&t_to), &(&t_cc), &(&t_bcc), &(&t_body), &(&t_attachments));

	MCExecContext ctxt(nil, nil, nil);

	if (t_success)
		MCMailExecComposeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposeUnicodeMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &(&t_subject), &(&t_to), &(&t_cc), &(&t_bcc), &(&t_body), &(&t_attachments));

	MCExecContext ctxt(nil, nil, nil);

	if (t_success)
		MCMailExecComposeUnicodeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposeHtmlMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &(&t_subject), &(&t_to), &(&t_cc), &(&t_bcc), &(&t_body), &(&t_attachments));

	MCExecContext ctxt(nil, nil, nil);

	if (t_success)
        MCMailExecComposeHtmlMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanSendMail(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);

	bool t_can_send;

	MCMailGetCanSendMail(ctxt, t_can_send);
    
    if (t_can_send)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
    return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleStartTrackingSensor(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleStartTrackingSensor */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_loosely = false;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_loosely = ep . getsvalue() == MCtruemcstring;
    }
    
    MCExecContext t_ctxt(ep);

    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStartTrackingSensor(t_ctxt, t_sensor, t_loosely);
    }
    
    return t_ctxt.GetStat();
#endif /* MCHandleStartTrackingSensor */
    MCExecContext ctxt(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_loosely = false;
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoStringRef t_string;
        p_parameters->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        t_sensor = MCSensorTypeFromString(*t_string);
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoBooleanRef t_bool;
        p_parameters->eval(ctxt, &t_value);
        // PM-2015-03-11: [[ Bug 14855 ]] Evaluate correctly the second param
        if (ctxt . ConvertToBoolean(*t_value, &t_bool))
            t_loosely = MCValueIsEqualTo(*t_bool, kMCTrue);
        // if conversion fails, keep the same behaviour as in LC 6.7
        else
            t_loosely = false;
    }
    
	ctxt . SetTheResultToEmpty();
    
    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStartTrackingSensor(ctxt, (intenum_t)t_sensor, t_loosely);
    }
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleStopTrackingSensor(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleStopTrackingSensor */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStopTrackingSensor(t_ctxt, t_sensor);
    }
    
    return t_ctxt.GetStat();
#endif /* MCHandleStopTrackingSensor */
    MCExecContext ctxt(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoStringRef t_string;
        p_parameters->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        t_sensor = MCSensorTypeFromString(*t_string);
        p_parameters = p_parameters->getnext();
    }

	ctxt . SetTheResultToEmpty();

    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)t_sensor);
    }
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

// MM-2012-02-11: Added support old style sensor syntax (iPhoneEnableAcceleromter etc)
Exec_stat MCHandleAccelerometerEnablement(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAccelerometerEnablement */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(t_ctxt, kMCSensorTypeAcceleration, false);
    else
        MCSensorExecStopTrackingSensor(t_ctxt, kMCSensorTypeAcceleration);
    
    return t_ctxt.GetStat();
#endif /* MCHandleAccelerometerEnablement */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(ctxt, kMCSensorTypeAcceleration, false);
    else
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)kMCSensorTypeAcceleration);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleLocationTrackingState(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleLocationTrackingState */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(t_ctxt, kMCSensorTypeLocation, false);
    else
        MCSensorExecStopTrackingSensor(t_ctxt, kMCSensorTypeLocation);
    
    return t_ctxt.GetStat();
#endif /* MCHandleLocationTrackingState */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(ctxt, kMCSensorTypeLocation, false);
    else
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)kMCSensorTypeLocation);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleHeadingTrackingState(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleHeadingTrackingState */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(t_ctxt, kMCSensorTypeHeading, true);
    else
        MCSensorExecStopTrackingSensor(t_ctxt, kMCSensorTypeHeading);
    
    return t_ctxt.GetStat();
#endif /* MCHandleHeadingTrackingState */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(ctxt, kMCSensorTypeHeading, true);
    else
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)kMCSensorTypeHeading);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSensorReading(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSensorReading */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_detailed = false;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_detailed = ep . getsvalue() == MCtruemcstring;
    }
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCVariableValue *t_detailed_reading = nil;
    MCAutoRawCString t_reading;
    
    switch (t_sensor)
    {
        case kMCSensorTypeLocation:
        {
            if (t_detailed)
                MCSensorGetDetailedLocationOfDevice(t_ctxt, t_detailed_reading);
            else
                MCSensorGetLocationOfDevice(t_ctxt, t_reading);
            break;
        }
        case kMCSensorTypeHeading:
        {
            if (t_detailed)
                MCSensorGetDetailedHeadingOfDevice(t_ctxt, t_detailed_reading);
            else
                MCSensorGetHeadingOfDevice(t_ctxt, t_reading);
            break;
        }
        case kMCSensorTypeAcceleration:
        {
            if (t_detailed)
                MCSensorGetDetailedAccelerationOfDevice(t_ctxt, t_detailed_reading);
            else
                MCSensorGetAccelerationOfDevice(t_ctxt, t_reading);
            break;
        }
        case kMCSensorTypeRotationRate:
        {
            if (t_detailed)
                MCSensorGetDetailedRotationRateOfDevice(t_ctxt, t_detailed_reading);
            else
                MCSensorGetRotationRateOfDevice(t_ctxt, t_reading);
            break;
        }
    }
    
    if (t_detailed)
    {
        if (t_detailed_reading != nil)
            ep.setarray(t_detailed_reading, True);
    }
    else
    {
        if (t_reading.Borrow() != nil)
            ep.copysvalue(t_reading.Borrow());
    }
    
    MCresult->store(ep, False);
    return t_ctxt.GetStat();
#endif /* MCHandleSensorReading */
    MCExecContext ctxt(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_detailed = false;
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoStringRef t_string;
        p_parameters->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        t_sensor = MCSensorTypeFromString(*t_string);
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoBooleanRef t_bool;
        p_parameters->eval(ctxt, &t_value);
        // PM-2015-03-11: [[ Bug 14855 ]] Evaluate correctly the second param
        if(ctxt . ConvertToBoolean(*t_value, &t_bool))
            t_detailed = MCValueIsEqualTo(*t_bool, kMCTrue);
        // if conversion fails, keep the same behaviour as in LC 6.7
        else
            t_detailed = false;
    }
    
    ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCAutoStringRef t_reading;

    switch (t_sensor)
    {
        case kMCSensorTypeLocation:
        {
            if (t_detailed)
                MCSensorGetDetailedLocationOfDevice(ctxt, &t_detailed_reading);
            else
                MCSensorGetLocationOfDevice(ctxt, &t_reading);
            break;
        }
        case kMCSensorTypeHeading:
        {
            if (t_detailed)
                MCSensorGetDetailedHeadingOfDevice(ctxt,& t_detailed_reading);
            else
                MCSensorGetHeadingOfDevice(ctxt, &t_reading);
            break;
        }
        case kMCSensorTypeAcceleration:
        {
            if (t_detailed)
                MCSensorGetDetailedAccelerationOfDevice(ctxt, &t_detailed_reading);
            else
                MCSensorGetAccelerationOfDevice(ctxt, &t_reading);
            break;
        }
        case kMCSensorTypeRotationRate:
        {
            if (t_detailed)
                MCSensorGetDetailedRotationRateOfDevice(ctxt, &t_detailed_reading);
            else
                MCSensorGetRotationRateOfDevice(ctxt, &t_reading);
            break;
        }
        default:
            break;
    }
    
    if (t_detailed)
    {
        if (*t_detailed_reading != nil)
            ctxt . SetTheResultToValue(*t_detailed_reading);
    }
    else
    {
        if (*t_reading != nil)
            ctxt . SetTheResultToValue(*t_reading);
    }

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

// SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]]
// PM-2014-12-08: [[ Bug 13659 ]] New function to detect if Voice Over is turned on (iOS only)
Exec_stat MCHandleIsVoiceOverRunning(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleIsVoiceOverRunning */ MLEGACY_EXEC
    MCresult -> sets(UIAccessibilityIsVoiceOverRunning() ? MCtruestring : MCfalsestring);
    return ES_NORMAL;
#endif /* MCHandleIsVoiceOverRunning */
    MCExecContext ctxt(nil, nil, nil);

    bool t_is_vo_running;
    MCMiscGetIsVoiceOverRunning(ctxt, t_is_vo_running);

    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToBool(t_is_vo_running);
        return ES_NORMAL;
    }

    return ES_ERROR;
}

// MM-2012-02-11: Added support old style sensor syntax (iPhoneGetCurrentLocation etc)
Exec_stat MCHandleCurrentLocation(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCurrentLocation */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCVariableValue *t_detailed_reading = nil;
    MCSensorGetDetailedLocationOfDevice(t_ctxt, t_detailed_reading);
    if (t_detailed_reading != nil)
        ep.setarray(t_detailed_reading, True);
    
    MCresult->store(ep, False);
    return t_ctxt.GetStat();
#endif /* MCHandleCurrentLocation */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCSensorGetDetailedLocationOfDevice(ctxt, &t_detailed_reading);
    
    if (*t_detailed_reading != nil)    
        ctxt . SetTheResultToValue(*t_detailed_reading);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCurrentHeading(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCurrentHeading */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCVariableValue *t_detailed_reading = nil;
    MCSensorGetDetailedHeadingOfDevice(t_ctxt, t_detailed_reading);
    if (t_detailed_reading != nil)
        ep.setarray(t_detailed_reading, True);
    
    MCresult->store(ep, False);
    return t_ctxt.GetStat();
#endif /* MCHandleCurrentHeading */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCSensorGetDetailedHeadingOfDevice(ctxt, &t_detailed_reading);
    if (*t_detailed_reading != nil)
        ctxt . SetTheResultToValue(*t_detailed_reading);
    
	if (!ctxt . HasError())
        return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSetHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetHeadingCalibrationTimeout */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_timeout = atoi(ep.getcstring());
    }
    MCSensorSetLocationCalibration(t_ctxt, t_timeout);
    
    return t_ctxt.GetStat();
#endif /* MCHandleSetHeadingCalibrationTimeout */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    t_timeout = 0;
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoNumberRef t_number;
        p_parameters->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToNumber(*t_value, &t_number);
        t_timeout = MCNumberFetchAsInteger(*t_number);
    }
    else
    {
        // We need a parameter
        ctxt . Throw();
    }
    
    if (!ctxt . HasError())
        MCSensorSetLocationCalibrationTimeout(ctxt, t_timeout);
    
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleHeadingCalibrationTimeout */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    MCSensorGetLocationCalibration(t_ctxt, t_timeout);
    MCresult->setnvalue(t_timeout);
    
    t_ctxt . SetTheResultToEmpty();
    return t_ctxt.GetStat();
#endif /* MCHandleHeadingCalibrationTimeout */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    MCSensorGetLocationCalibrationTimeout(ctxt, t_timeout);
    MCresult->setnvalue(t_timeout);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSensorAvailable(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSensorAvailable */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCSensorType t_sensor;
    t_sensor = kMCSensorTypeUnknown;
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(t_ctxt, t_sensor, t_available);
    
    MCresult->sets(MCU_btos(t_available));
    return t_ctxt.GetStat();
#endif /* MCHandleSensorAvailable */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();

    MCSensorType t_sensor;
    t_sensor = kMCSensorTypeUnknown;    
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoStringRef t_string;
        p_parameters->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
        t_sensor = MCSensorTypeFromString(*t_string);
        p_parameters = p_parameters->getnext();
    }    
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(ctxt, t_sensor, t_available);
    
    if (t_available)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanTrackLocation(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCanTrackLocation */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(t_ctxt, kMCSensorTypeLocation, t_available);
    
    MCresult->sets(MCU_btos(t_available));
    return t_ctxt.GetStat();
#endif /* MCHandleCanTrackLocation */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
        
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(ctxt, kMCSensorTypeLocation, t_available);
    
    if (t_available)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanTrackHeading(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCanTrackHeading */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(t_ctxt, kMCSensorTypeHeading, t_available);
    
    MCresult->sets(MCU_btos(t_available));
    return t_ctxt.GetStat();
#endif /* MCHandleCanTrackHeading */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(ctxt, kMCSensorTypeHeading, t_available);
    
    if (t_available)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

bool MCContactParseParams(MCParameter *p_params, MCArrayRef &r_contact, char *&r_title, char *&r_message, char *&r_alternate_name)
{
	bool t_success = true;
	
	char *t_title = nil;
	char *t_message = nil;
	char *t_alternate_name = nil;
	
	t_success = MCParseParameters(p_params, "a|sss", r_contact, &t_title, &t_message, &t_alternate_name);
	
	if (t_success)
	{
		r_title = t_title;
		r_message = t_message;
		r_alternate_name = t_alternate_name;
	}
	else
	{
		MCCStringFree(t_title);
		MCCStringFree(t_message);
		MCCStringFree(t_alternate_name);
	}
	
	return t_success;
}

Exec_stat MCHandlePickContact(void *context, MCParameter *p_parameters) // ABPeoplePickerNavigationController
{
#ifdef /* MCHandlePickContact */ LEGACY_EXEC
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCPickContactExec(t_ctxt);
    // Set return value
	return t_ctxt.GetStat();
#endif /* MCHandlePickContact */
    int32_t r_result;
    MCExecContext ctxt(nil, nil, nil);

    MCAddressBookExecPickContact(ctxt);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleShowContact(void *context, MCParameter *p_parameters) // ABPersonViewController
{
#ifdef /* MCHandleShowContact */ LEGACY_EXEC
    int32_t t_contact_id = 0;
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCShowContactExec(t_ctxt, t_contact_id);
    // Set return value
	return t_ctxt.GetStat();
#endif /* MCHandleShowContact */
    int32_t t_contact_id = 0;
    int32_t r_result;
    MCExecContext ctxt(nil, nil, nil);

    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoNumberRef t_number;
        p_parameters->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToNumber(*t_value, &t_number);
        t_contact_id = MCNumberFetchAsInteger(*t_number);
    }

    MCAddressBookExecShowContact(ctxt, t_contact_id);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCreateContact(void *context, MCParameter *p_parameters) // ABNewPersonViewController
{
#ifdef /* MCHandleCreateContact */ LEGACY_EXEC
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCCreateContactExec(t_ctxt);
    // Set return value
	return t_ctxt.GetStat();
#endif /* MCHandleCreateContact */
    MCExecContext ctxt(nil, nil, nil);

    MCAddressBookExecCreateContact(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUpdateContact(void *context, MCParameter *p_parameters) // ABUnknownPersonViewController
{
#ifdef /* MCHandleUpdateContact */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    // Handle parameters. We are doing that in a dedicated call
	MCVariableValue *t_contact = nil;
	char *t_title = nil;
	char *t_message = nil;
	char *t_alternate_name = nil;
	/* UNCHECKED */ MCContactParseParams(p_parameters, t_contact, t_title, t_message, t_alternate_name);
    // Call the Exec implementation
    MCUpdateContactExec(ctxt, t_contact, t_title, t_message, t_alternate_name);
    // Set return value
	return ctxt.GetStat();
#endif /* MCHandleUpdateContact */
    MCExecContext ctxt(nil, nil, nil);

	MCAutoArrayRef t_contact;
	MCAutoStringRef t_title;
	MCAutoStringRef t_message;
	MCAutoStringRef t_alternate_name;

    // PM-2015-05-21: [[ Bug 14792 ]] Make sure params are parsed properly
	if (MCParseParameters(p_parameters, "a|xxx", &(&t_contact), &(&t_title), &(&t_message), &(&t_alternate_name)))
	    MCAddressBookExecUpdateContact(ctxt, *t_contact, *t_title, *t_message, *t_alternate_name);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleGetContactData(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetContactData */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    int32_t t_contact_id = 0;
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCGetContactDataExec(t_ctxt, t_contact_id);
    if (MCresult->isempty())
        MCresult->store(ep, True);
	return t_ctxt.GetStat();
#endif /* MCHandleGetContactData */
    MCExecContext ctxt(nil, nil, nil);

    int32_t t_contact_id = 0;
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoNumberRef t_number;
        p_parameters->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToNumber(*t_value, &t_number);
        t_contact_id = MCNumberFetchAsInteger(*t_number);
    }
    
	MCAutoArrayRef t_contact_data;
	MCAddressBookGetContactData(ctxt, t_contact_id, &t_contact_data);

	if (*t_contact_data != nil)
		ctxt . SetTheResultToValue(*t_contact_data);
	else
		ctxt . SetTheResultToEmpty();

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleRemoveContact(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleRemoveContact */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    int32_t t_contact_id = 0;
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCRemoveContactExec(t_ctxt, t_contact_id);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleRemoveContact */
    MCExecContext ctxt(nil, nil, nil);

    int32_t t_contact_id = 0;
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        MCAutoNumberRef t_number;
        p_parameters->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToNumber(*t_value, &t_number);
        t_contact_id = MCNumberFetchAsInteger(*t_number);
    }
    
	MCAddressBookExecRemoveContact(ctxt, t_contact_id);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAddContact(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAddContact */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    // Handle parameters. We are doing that in a dedicated call
	MCVariableValue *t_contact;
	
	/* UNCHECKED */ MCParseParameters(p_parameters, "a", &t_contact);
    
    MCExecContext t_ctxt(ep);
    // Call the Exec implementation
    MCAddContactExec(t_ctxt, t_contact);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleAddContact */
    MCExecContext ctxt(nil, nil, nil);
    // Handle parameters. We are doing that in a dedicated call
	MCAutoArrayRef t_contact;
	
	/* UNCHECKED */ MCParseParameters(p_parameters, "a", &(&t_contact));

    // Call the Exec implementation
    MCAddressBookExecAddContact(ctxt, *t_contact);
    
    // Set return value
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleFindContact(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleFindContact */ LEGACY_EXEC
    const char *t_contact_name = NULL;
    const char *r_result = NULL;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_name = ep.getcstring();
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCFindContactExec(t_ctxt, t_contact_name);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleFindContact */
    MCAutoStringRef t_contact_name;
    MCExecContext ctxt(nil, nil, nil);
    // Handle parameters.
    if (p_parameters)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_contact_name);
    }
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCAddressBookExecFindContact(ctxt, *t_contact_name);
    // Set return value
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}


//////////////////////////////////////////////////////////////////////////////


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
	bool t_success;
	t_success = true;
    
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_key;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_key));
	
	if (t_success)
		MCAdExecRegisterWithInneractive(ctxt, *t_key);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
    return ES_ERROR;
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
        MCAutoStringRef t_type_string;
        if (MCParseParameters(p_parameters, "x", &(&t_type_string)))
            t_type = MCAdTypeFromString(*t_type_string);
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
	bool t_success;
	t_success = true;
    
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    MCAutoStringRef t_type;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &(&t_ad), &(&t_type));
    
    MCAdTopLeft t_topleft = {0,0};
    MCAutoStringRef t_topleft_string;
    
    if (t_success)
    {
        if (MCParseParameters(p_parameters, "x", &(&t_topleft_string)))
            /* UNCHECKED */ sscanf(MCStringGetCString(*t_topleft_string), "%u,%u", &t_topleft.x, &t_topleft.y);
    }
    
    MCAutoArrayRef t_metadata;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "a", &(&t_metadata));
    
	if (t_success)
		MCAdExecCreateAd(ctxt, *t_ad, *t_type, t_topleft, *t_metadata);
    
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
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
	bool t_success;
	t_success = true;
    
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_ad));
	
	if (t_success)
		MCAdExecDeleteAd(ctxt, *t_ad);
        
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
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
	bool t_success;
	t_success = true;
    
    MCExecContext ctxt(nil, nil, nil);
    
	MCAutoStringRef t_ad;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_ad));
	
    bool t_visible;
    t_visible = false;
    
	if (t_success)
		MCAdGetVisibleOfAd(ctxt, *t_ad, t_visible);
    
    
    if (!ctxt . HasError())
    {
        if (t_visible)
            ctxt.SetTheResultToValue(kMCTrueString);
        else
            ctxt.SetTheResultToValue(kMCFalseString);
        
		return ES_NORMAL;
    }

	ctxt . SetTheResultToEmpty();
	return ES_ERROR;
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
	bool t_success;
	t_success = true;
    
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    
    bool t_visible;
    t_visible = false;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "xb", &(&t_ad), &t_visible);
	
	if (t_success)
		MCAdSetVisibleOfAd(ctxt, *t_ad, t_visible);
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAdGetTopLeft(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecContext ctxt(nil, nil, nil);
    
	MCAutoStringRef t_ad;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_ad));
	
    MCAdTopLeft t_topleft;
    
	if (t_success)
		MCAdGetTopLeftOfAd(ctxt, *t_ad, t_topleft);
    
    if (!ctxt . HasError())
    {
        MCAutoStringRef t_topleft_string;
        if(MCStringFormat(&t_topleft_string, "%u,%u", t_topleft.x, t_topleft.y))
        {
            ctxt.SetTheResultToValue(*t_topleft_string);
            return ES_NORMAL;
        }
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
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
	bool t_success;
	t_success = true;
    
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    MCAdTopLeft t_topleft;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xuu", &(&t_ad), t_topleft.x, t_topleft.y);
    
	if (t_success)
		MCAdSetTopLeftOfAd(ctxt, *t_ad, t_topleft);
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
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
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();

    MCAutoStringRef t_ads;
    MCAdGetAds(ctxt, &t_ads);

    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_ads);
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleShowEvent(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleShowEvent */ LEGACY_EXEC
    const char* t_event_id = NULL;
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_event_id = ep.getsvalue().getstring();
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCShowEventExec(t_ctxt, t_event_id);
    // Set return value
	return t_ctxt.GetStat();
#endif /* MCHandleShowEvent */
    MCAutoStringRef t_id;
    bool t_success;
    t_success = true;
    
    // Handle parameters.
    
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "x", &(&t_id));
    }
    
    MCExecContext ctxt(nil, nil, nil);
    
    // Call the Exec implementation
    if (t_success)
        MCCalendarExecShowEvent(ctxt, *t_id);
    
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleUpdateEvent(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleUpdateEvent */ LEGACY_EXEC
    const char* t_event_id = NULL;
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_event_id = ep.getsvalue().getstring();
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCUpdateEventExec(t_ctxt, t_event_id);
    // Set return value
	return t_ctxt.GetStat();
#endif /* MCHandleUpdateEvent */
    MCAutoStringRef t_id;
    bool t_success;
    t_success = true;

    // Handle parameters.    
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "x", &(&t_id));
    }
    
    MCExecContext ctxt(nil, nil, nil);
    
    // Call the Exec implementation
    if (t_success)
        MCCalendarExecUpdateEvent(ctxt, *t_id);
    
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleCreateEvent(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCreateEvent */ LEGACY_EXEC
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCCreateEventExec(t_ctxt);
    // Set return value
	return t_ctxt.GetStat();
#endif /* MCHandleCreateEvent */
    MCAutoStringRef t_id;
    
    MCExecContext ctxt(nil, nil, nil);

    // Call the Exec implementation
    MCCalendarExecCreateEvent(ctxt);
    // Set return value
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleGetEventData(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetEventData */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    const char* t_event_id = NULL;
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_event_id = ep.getsvalue().getstring();
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCGetEventDataExec(t_ctxt, t_event_id);
    if (MCresult->isempty())
        MCresult->store(ep, True);
	return t_ctxt.GetStat();
#endif /* MCHandleGetEventData */
    MCAutoStringRef t_id;
    bool t_success;
    t_success = true;
    
    // Handle parameters.
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_id));
    
    MCExecContext ctxt(nil, nil, nil);
    
    // Call the Exec implementation
    MCAutoArrayRef t_data;
    if (t_success)
    {
        MCCalendarGetEventData(ctxt, *t_id, &t_data);
    }    
    
    if (!ctxt . HasError())
    {
        ctxt.SetTheResultToValue(*t_data);
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleRemoveEvent(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleRemoveEvent */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    const char* t_event_id = NULL;
    bool t_reocurring = false;
    bool t_success = true;
    // Handle parameters.
    t_success = MCParseParameters(p_parameters, "s", &t_event_id);
    
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "b", &t_reocurring);
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCRemoveEventExec(t_ctxt, t_reocurring, t_event_id);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleRemoveEvent */
    MCAutoStringRef t_id;
    bool t_reocurring = false;
    bool t_success = true;
    // Handle parameters.
    t_success = MCParseParameters(p_parameters, "x", &(&t_id));
    
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "b", &t_reocurring);
    }
    
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
//    MCCalendarExecRemoveEvent(ctxt, t_reocurring, &t_id);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAddEvent(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAddEvent */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    // Handle parameters. We are doing that in a dedicated call
    MCCalendar t_new_event_data;
    t_new_event_data = MCParameterDataToCalendar(p_parameters, t_new_event_data);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCAddEventExec(t_ctxt, t_new_event_data);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleAddEvent */
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success;
    
    // Handle parameters. We are doing that in a dedicated call
    MCAutoArrayRef t_array;
    
    t_success = MCParseParameters(p_parameters, "a", &(&t_array));
    
    if (t_success)
        MCCalendarExecAddEvent(ctxt, *t_array);
    
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleGetCalendarsEvent(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetCalendarsEvent */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCGetCalendarsEventExec(t_ctxt);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleGetCalendarsEvent */
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    
    // Call the Exec implementation
    MCCalendarGetCalendars(ctxt);
    
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleFindEvent(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleFindEvent */ LEGACY_EXEC
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    bool t_success = true;
    const char *r_result = NULL;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
    if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
    }
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    MCFindEventExec(t_ctxt, t_start_date, t_end_date);
    // Set return value
    return t_ctxt.GetStat();
#endif /* MCHandleFindEvent */
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    bool t_success = true;

    MCExecContext ctxt(nil, nil, nil);
    // Handle parameters.
    if (p_parameters)
    {
        MCAutoValueRef t_value;

        if (p_parameters->eval(ctxt, &t_value)
                && !MCValueIsEmpty(*t_value))
        {
			t_success = MCD_convert_to_datetime(ctxt, *t_value, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
    if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;

        if (p_parameters->eval(ctxt, &t_value)
                && !MCValueIsEmpty(*t_value))
        {
			t_success = MCD_convert_to_datetime(ctxt, *t_value, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
    }

    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
//    MCCalendarExecFindEvent(ctxt, t_start_date, t_end_date);
    // Set return value
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleCreateLocalNotification (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCreateLocalNotification */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    bool t_success = true;
    char *t_notification_body = nil;
    char *t_notification_action = nil;
    char *t_notification_user_info = nil;
    MCDateTime t_date;
    bool t_play_sound_vibrate = true;
    int32_t t_badge_value = 0;
    
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "sss", &t_notification_body, &t_notification_action, &t_notification_user_info);
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_date);
        }
        p_parameters = p_parameters->getnext();
    }
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_play_sound_vibrate);
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "u", &t_badge_value);
    
	MCLocalNotificationExec (t_ctxt, t_notification_body, t_notification_action, t_notification_user_info, t_date, t_play_sound_vibrate, t_badge_value);
    
    return ES_NORMAL;
#endif /* MCHandleCreateLocalNotification */
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success = true;
    MCAutoStringRef t_notification_body;
    MCAutoStringRef t_notification_action;
    MCAutoStringRef t_notification_user_info;
    MCDateTime t_date;
    bool t_play_sound_vibrate = true;
    int32_t t_badge_value = 0;
    
    ctxt.SetTheResultToEmpty();
    
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "xxx", &(&t_notification_body), &(&t_notification_action), &(&t_notification_user_info));
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        if (p_parameters->eval(ctxt, &t_value)
                && !MCValueIsEmpty(*t_value))
        {
            t_success = MCD_convert_to_datetime(ctxt, *t_value, CF_UNDEFINED, CF_UNDEFINED, t_date);
        }
        p_parameters = p_parameters->getnext();
    }
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_play_sound_vibrate);
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "u", &t_badge_value);
    
	MCNotificationExecCreateLocalNotification (ctxt, *t_notification_body, *t_notification_action, *t_notification_user_info, t_date, t_play_sound_vibrate, t_badge_value);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetRegisteredNotifications(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetRegisteredNotifications */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCGetRegisteredNotificationsExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleGetRegisteredNotifications */
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    
    MCNotificationGetRegisteredNotifications(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetNotificationDetails(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetNotificationDetails */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_success = true;
    
    int32_t t_id = -1;
    MCVariableValue *t_details = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "i", &t_id);
    
    ctxt.SetTheResultToEmpty();
    if (t_success)
    {
        MCNotificationGetDetails(ctxt, t_id, t_details);
        if (t_details != nil)
        {
            ep.setarray(t_details, True);
            MCresult->store(ep, False);
        }
    }
    
    return ES_NORMAL;
#endif /* MCHandleGetNotificationDetails */
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success = true;
    
    int32_t t_id = -1;
    MCAutoArrayRef t_details;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "i", &t_id);
    
    if (t_success)
    {
        MCNotificationGetDetails(ctxt, t_id, &t_details);
        if (!ctxt.HasError() && *t_details != nil)
        {
			ctxt . SetTheResultToValue(*t_details);
            return ES_NORMAL;
        }
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleCancelLocalNotification(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCancelLocalNotification */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    int32_t t_cancel_this;
    bool t_success;
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    if (p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "i", &t_cancel_this);
    MCCancelLocalNotificationExec (t_ctxt, t_cancel_this);
    
    return ES_NORMAL;
#endif /* MCHandleCancelLocalNotification */
    MCExecContext ctxt(nil, nil, nil);
    int32_t t_cancel_this;
    bool t_success;
    
    ctxt.SetTheResultToEmpty();
    if (p_parameters != nil)
    {
		t_success = MCParseParameters (p_parameters, "i", &t_cancel_this);
    
        if (t_success)
            MCNotificationExecCancelLocalNotification (ctxt, t_cancel_this);
    }
    else
        t_success = false;
    
    
    if (t_success && ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleCancelAllLocalNotifications (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCancelAllLocalNotifications */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCCancelAllLocalNotificationsExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleCancelAllLocalNotifications */
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    
    MCNotificationExecCancelAllLocalNotifications(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleGetNotificationBadgeValue (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetNotificationBadgeValue */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCGetNotificationBadgeValueExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleGetNotificationBadgeValue */
    MCExecContext ctxt(nil, nil, nil);
    ctxt.SetTheResultToEmpty();
    MCNotificationGetNotificationBadgeValue (ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleSetNotificationBadgeValue (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetNotificationBadgeValue */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    uint32_t t_badge_value;
    bool t_success = true;
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "i", &t_badge_value);
    if (t_success)
        MCSetNotificationBadgeValueExec (t_ctxt, t_badge_value);
    
    return ES_NORMAL;
#endif /* MCHandleSetNotificationBadgeValue */
    MCExecContext ctxt(nil, nil, nil);
    uint32_t t_badge_value;
    bool t_success = true;
    
    ctxt.SetTheResultToEmpty();
    
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "i", &t_badge_value);
    
    if (t_success)
        MCNotificationSetNotificationBadgeValue (ctxt, t_badge_value);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


////////////////

static MCBusyIndicatorType MCBusyIndicatorTypeFromString(MCStringRef p_string)
{
#ifdef /* MCBusyIndicatorTypeFromCString */ LEGACY_EXEC
    if (MCCStringEqualCaseless(p_string, "in line"))
        return kMCBusyIndicatorInLine;
    else if (MCCStringEqualCaseless(p_string, "square"))
        return kMCBusyIndicatorSquare;
    else if (MCCStringEqualCaseless(p_string, "keyboard"))
        return kMCBusyIndicatorKeyboard;
    
    return kMCBusyIndicatorSquare;
#endif /* MCBusyIndicatorTypeFromCString */
    if (MCStringIsEqualToCString(p_string, "in line", kMCCompareCaseless))
        return kMCBusyIndicatorInLine;
    else if (MCStringIsEqualToCString(p_string, "square", kMCCompareCaseless))
        return kMCBusyIndicatorSquare;
    else if (MCStringIsEqualToCString(p_string, "keyboard", kMCCompareCaseless))
        return kMCBusyIndicatorKeyboard;
    
    return kMCBusyIndicatorSquare;
}

static bool MCBusyIndicatorTypeToString(MCSensorType p_indicator, MCStringRef& r_string)
{
#ifdef /* MCBusyIndicatorTypeToCString */ LEGACY_EXEC
    switch (p_indicator)
    {
        case kMCBusyIndicatorInLine:
            return MCCStringClone("in line", r_string);
        case kMCBusyIndicatorSquare:
            return MCCStringClone("square", r_string);
        case kMCBusyIndicatorKeyboard:
            return MCCStringClone("keyboard", r_string);
        default:
            return MCCStringClone("unknown", r_string);
    }
    return false;
#endif /* MCBusyIndicatorTypeToCString */
    switch (p_indicator)
    {
        case kMCBusyIndicatorInLine:
            return MCStringCreateWithCString("in line", r_string);
        case kMCBusyIndicatorSquare:
            return MCStringCreateWithCString("square", r_string);
        case kMCBusyIndicatorKeyboard:
            return MCStringCreateWithCString("keyboard", r_string);
        default:
            return MCStringCreateWithCString("unknown", r_string);
    }
    return false;
}


// MM-2013-02-04: [[ Bug 10642 ]] Added new optional opacity parameter to busy indicator.
Exec_stat MCHandleStartBusyIndicator(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleStartBusyIndicator */ LEGACY_EXEC
    MCBusyIndicatorType t_indicator_type;
    MCExecPoint ep(nil, nil, nil);
    
    if (p_parameters)
    {
        p_parameters -> eval(ep);
        t_indicator_type = MCBusyIndicatorTypeFromCString(ep . getcstring());
        p_parameters = p_parameters -> getnext();
    }
    
    const char *t_label;
    t_label = nil;
    if (p_parameters)
    {
        p_parameters -> eval(ep);
        t_label = ep . getcstring();
        p_parameters = p_parameters -> getnext();
    }
    
    int32_t t_opacity;
    t_opacity = -1;
    if (p_parameters)
    {
        p_parameters -> eval(ep);
        t_opacity = ep . getint4();
        if (t_opacity < 0 || t_opacity > 100)
            t_opacity = -1;
    }
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    MCBusyIndicatorExecStart(t_ctxt, kMCBusyIndicatorSquare, t_label, t_opacity);
    return t_ctxt.GetStat();
#endif /* MCHandleStartBusyIndicator */
    
    bool t_success = true;
    MCAutoStringRef t_indicator_string;
    MCAutoStringRef t_label;    

    if (t_success && p_parameters)
        t_success = MCParseParameters(p_parameters, "x", &(&t_indicator_string));
                                      
    if (t_success && p_parameters)
        t_success = MCParseParameters(p_parameters, "x", &(&t_label));
    
    intenum_t t_indicator;
    // PM-2014-11-21: [[ Bug 14068 ]] Nil check to prevent a crash
    if (t_success && p_parameters)
        t_success = MCBusyIndicatorTypeFromString(*t_indicator_string);
    
    int32_t t_opacity = -1;
    if (t_success && p_parameters)
    {
        t_success = MCParseParameters(p_parameters, "i", &t_opacity);
        if (t_opacity < 0 || t_opacity > 100)
            t_opacity = -1;
    }
    
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    MCBusyIndicatorExecStartBusyIndicator(ctxt, kMCBusyIndicatorSquare, *t_label, t_opacity);
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;    
}

Exec_stat MCHandleStopBusyIndicator(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleStopBusyIndicator */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCBusyIndicatorExecStop(t_ctxt);
    
    return t_ctxt.GetStat();
#endif /* MCHandleStopBusyIndicator */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	MCBusyIndicatorExecStopBusyIndicator(ctxt);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

///////////////

static MCActivityIndicatorType MCActivityIndicatorTypeFromString(MCStringRef p_string)
{
#ifdef /* MCActivityIndicatorTypeFromCString */ LEGACY_EXEC
    if (MCCStringEqualCaseless(p_string, "white"))
        return kMCActivityIndicatorWhite;
    else if (MCCStringEqualCaseless(p_string, "large white"))
        return kMCActivityIndicatorWhiteLarge;
    else if (MCCStringEqualCaseless(p_string, "gray"))
        return kMCActivityIndicatorGray;
    
    return kMCActivityIndicatorWhite;
#endif /* MCActivityIndicatorTypeFromCString */
    if (MCStringIsEqualToCString(p_string, "white", kMCCompareCaseless))
        return kMCActivityIndicatorWhite;
    else if (MCStringIsEqualToCString(p_string, "large white", kMCCompareCaseless))
        return kMCActivityIndicatorWhiteLarge;
    else if (MCStringIsEqualToCString(p_string, "gray", kMCCompareCaseless))
        return kMCActivityIndicatorGray;
    
    return kMCActivityIndicatorWhite;
}

static bool MCActivityIndicatorTypeToString(MCSensorType p_indicator, MCStringRef& r_string)
{
#ifdef /* MCActivityIndicatorTypeToCString */ LEGACY_EXEC
    switch (p_indicator)
    {
        case kMCActivityIndicatorWhite:
            return MCCStringClone("white", r_string);
        case kMCActivityIndicatorWhiteLarge:
            return MCCStringClone("large white", r_string);
        case kMCActivityIndicatorGray:
            return MCCStringClone("gray", r_string);
        default:
            return MCCStringClone("unknown", r_string);
    }
    return false;
#endif /* MCActivityIndicatorTypeToCString */
    switch (p_indicator)
    {
        case kMCActivityIndicatorWhite:
            return MCStringCreateWithCString("white", r_string);
        case kMCActivityIndicatorWhiteLarge:
            return MCStringCreateWithCString("large white", r_string);
        case kMCActivityIndicatorGray:
            return MCStringCreateWithCString("gray", r_string);
        default:
            return MCStringCreateWithCString("unknown", r_string);
    }
    return false;
}

Exec_stat MCHandleStartActivityIndicator(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleStartActivityIndicator */ LEGACY_EXEC
    MCActivityIndicatorType t_indicator_type;
    t_indicator_type = kMCActivityIndicatorWhite;
    
    char *t_style;
    t_style = nil;
    
    MCLocation t_location;
    t_location.x = -1;
    t_location.y = -1;
    
    MCExecPoint ep(nil, nil, nil);
    if (p_parameters != nil)
    {
        p_parameters->eval(ep);
        // Provide backwards compatibility here for "whiteLarge" rather than "large white".
        if (MCCStringEqualCaseless (ep.getsvalue().getstring(), "whiteLarge"))
            MCCStringClone("large white", t_style);
        else
            t_style = ep.getsvalue().clone();
        if (t_style != nil)
            p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (ep.getformat() != VF_STRING || ep.ston() == ES_NORMAL)
        {
            t_location.x = ep.getint4();
            p_parameters = p_parameters->getnext();
            if (p_parameters != nil)
            {
                p_parameters->eval(ep);
                if (ep.getformat() != VF_STRING || ep.ston() == ES_NORMAL)
                {
                    t_location.y = ep.getint4();
                    p_parameters = p_parameters->getnext();
                }
            }
        }
        if (t_location.y == -1)
            t_location.x = -1;
    }
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	if (t_style != nil)
		t_indicator_type = MCActivityIndicatorTypeFromCString(t_style);
    MCActivityIndicatorExecStart(t_ctxt, t_indicator_type, t_location);
    return t_ctxt.GetStat();
#endif /* MCHandleStartActivityIndicator */
    MCAutoStringRef t_style_string;
    MCActivityIndicatorType t_style;
    t_style = kMCActivityIndicatorWhite;
    
    bool t_success = true;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_style_string));
    
    if (t_success)
    {
        if (MCStringIsEqualToCString(*t_style_string, "whitelarge", kMCCompareCaseless))
            t_style = MCActivityIndicatorTypeFromString(MCSTR("large white"));
        else
            t_style = MCActivityIndicatorTypeFromString(*t_style_string);
    }
    
    bool t_location_param = false;
    integer_t* t_location_x_ptr = nil;
    integer_t* t_location_y_ptr = nil;
    integer_t t_location_x;
    integer_t t_location_y;
    
    if (MCParseParameters(p_parameters, "i", &t_location_x))
    {
        t_location_param = true;
        t_location_x_ptr = &t_location_x;
    }

    if (MCParseParameters(p_parameters, "i", &t_location_y))
    {
        t_location_param = true;
        t_location_y_ptr = &t_location_y;
    }
        
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    if (t_success)
        MCBusyIndicatorExecStartActivityIndicator(ctxt, t_style, t_location_x_ptr, t_location_y_ptr);
    
	if (t_success && !ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleStopActivityIndicator(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleStopActivityIndicator */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCActivityIndicatorExecStop(t_ctxt);
    
    return t_ctxt.GetStat();
#endif /* MCHandleStopActivityIndicator */

    MCExecContext ctxt(nil, nil, nil);
    
	ctxt . SetTheResultToEmpty();
    
    MCBusyIndicatorExecStopActivityIndicator(ctxt);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

////////////////


static MCSoundAudioCategory MCSoundAudioCategoryFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "ambient", kMCCompareCaseless))
        return kMCSoundAudioCategoryAmbient;
    else if (MCStringIsEqualToCString(p_string, "solo ambient", kMCCompareCaseless))
        return kMCSoundAudioCategorySoloAmbient;
    else if (MCStringIsEqualToCString(p_string, "playback", kMCCompareCaseless))
        return kMCSoundAudioCategoryPlayback;
    else if (MCStringIsEqualToCString(p_string, "record", kMCCompareCaseless))
        return kMCSoundAudioCategoryRecord;
    else if (MCStringIsEqualToCString(p_string, "play and record", kMCCompareCaseless))
        return kMCSoundAudioCategoryPlayAndRecord;
    else if (MCStringIsEqualToCString(p_string, "audio processing", kMCCompareCaseless))
        return kMCSoundAudioCategoryAudioProcessing;
    
    return kMCSoundAudioCategoryUnknown;
}

static bool MCSoundAudioCategoryTypeToString(MCSoundAudioCategory p_indicator, MCStringRef& r_string)
{
    switch (p_indicator)
    {
        case kMCSoundAudioCategoryAmbient:
            return MCStringCreateWithCString("ambient", r_string);
        case kMCSoundAudioCategorySoloAmbient:
            return MCStringCreateWithCString("solo ambient", r_string);
        case kMCSoundAudioCategoryPlayback:
            return MCStringCreateWithCString("playback", r_string);
        case kMCSoundAudioCategoryRecord:
            return MCStringCreateWithCString("record", r_string);
        case kMCSoundAudioCategoryPlayAndRecord:
            return MCStringCreateWithCString("play and record", r_string);
        case kMCSoundAudioCategoryAudioProcessing:
            return MCStringCreateWithCString("audio processing", r_string);
        default:
            return false;
    }
}

static MCSoundChannelStatus MCSoundChannelStatusFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "playing", kMCCompareCaseless))
        return kMCSoundChannelStatusPlaying;
    else if (MCStringIsEqualToCString(p_string, "paused", kMCCompareCaseless))
        return kMCSoundChannelStatusPaused;
    else //if (MCStringIsEqualToCString(p_string, "stopped", kMCCompareCaseless))
        return kMCSoundChannelStatusStopped;
}

static bool MCSoundChannelStatusTypeToString(MCSoundChannelStatus p_status, MCStringRef& r_string)
{
    switch(p_status)
    {
        case kMCSoundChannelStatusPaused:
            return MCStringCreateWithCString("paused", r_string);
        case kMCSoundChannelStatusStopped:
            return MCStringCreateWithCString("stopped", r_string);
        case kMCSoundChannelStatusPlaying:
            return MCStringCreateWithCString("playing", r_string);
        default:
            return false;
    }
}

static MCSoundChannelPlayType MCSoundChannelPlayTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "next", kMCCompareCaseless))
        return kMCSoundChannelPlayNext;
    else if (MCStringIsEqualToCString(p_string, "looping", kMCCompareCaseless))
        return kMCSoundChannelPlayLooping;
    else
        return kMCSoundChannelPlayNow;
}

static bool MCSoundChannelPlayTypeToString(MCSoundChannelPlayType p_type, MCStringRef& r_string)
{
    switch(p_type)
    {
        case kMCSoundChannelPlayLooping:
            return MCStringCreateWithCString("play looping", r_string);
        case kMCSoundChannelPlayNext:
            return MCStringCreateWithCString("play next", r_string);
        default:
            return MCStringCreateWithCString("play now", r_string);
    }
}


Exec_stat MCHandlePlaySoundOnChannel(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePlaySoundOnChannel */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_sound, *t_channel, *t_type;
	t_sound = nil;
	t_channel = nil;
	t_type = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "sss", &t_sound, &t_channel, &t_type);
	
	if (t_success)
	{
		MCSoundChannelPlayType t_play_type;
		t_play_type = kMCSoundChannelPlayNow;
		if (MCCStringEqualCaseless(t_type, "next"))
			t_play_type = kMCSoundChannelPlayNext;
		else if (MCCStringEqualCaseless(t_type, "looping"))
			t_play_type = kMCSoundChannelPlayLooping;
		
		MCSoundExecPlaySoundOnChannel(t_ctxt, t_channel, t_sound, t_play_type);
	}
    
	delete t_sound;
	delete t_channel;
	delete t_type;
	
    return t_ctxt.GetStat();
#endif /* MCHandlePlaySoundOnChannel */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_sound;
    MCAutoStringRef t_channel;
    MCAutoStringRef t_type;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xxx", &(&t_sound), &(&t_channel), &(&t_type));
	
    MCSoundChannelPlayType t_play_type;
	if (t_success)
        t_play_type = MCSoundChannelPlayTypeFromString(*t_type);
    
    if(t_success)
		MCSoundExecPlaySoundOnChannel(ctxt, *t_channel, *t_sound, (integer_t)t_play_type);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandlePausePlayingOnChannel(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePausePlayingOnChannel */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	if (t_success)
		MCSoundExecPauseSoundOnChannel(t_ctxt, t_channel);
	
	delete t_channel;
	
    return t_ctxt.GetStat();
#endif /* MCHandlePausePlayingOnChannel */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	if (t_success)
		MCSoundExecPausePlayingOnChannel(ctxt, *t_channel);
	
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleResumePlayingOnChannel(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleResumePlayingOnChannel */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
    
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	if (t_success)
		MCSoundExecResumeSoundOnChannel(t_ctxt, t_channel);
	
	delete t_channel;
	
    return t_ctxt.GetStat();
#endif /* MCHandleResumePlayingOnChannel */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
    
	MCAutoStringRef t_channel;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	if (t_success)
		MCSoundExecResumePlayingOnChannel(ctxt, *t_channel);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleStopPlayingOnChannel(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleStopPlayingOnChannel */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	if (t_success)
		MCSoundExecStopSoundOnChannel(t_ctxt, t_channel);
    
	delete t_channel;
	
    return t_ctxt.GetStat();
#endif /* MCHandleStopPlayingOnChannel */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	if (t_success)
		MCSoundExecStopPlayingOnChannel(ctxt, *t_channel);
	
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleDeleteSoundChannel(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleDeleteSoundChannel */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	if (t_success)
		MCSoundExecDeleteChannel(t_ctxt, t_channel);
    
	delete t_channel;
	
    return t_ctxt.GetStat();
#endif /* MCHandleDeleteSoundChannel */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	if (t_success)
		MCSoundExecDeleteSoundChannel(ctxt, *t_channel);
	
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSetSoundChannelVolume(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetSoundChannelVolume */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	int32_t t_volume;
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "su", &t_channel, &t_volume);
	
	if (t_success)
		MCSoundSetVolumeOfChannel(t_ctxt, t_channel, t_volume);
	
	delete t_channel;
	
    return t_ctxt.GetStat();
#endif /* MCHandleSetSoundChannelVolume */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	int32_t t_volume;
	MCAutoStringRef t_channel;

    if (t_success)
		t_success = MCParseParameters(p_parameters, "xu", &(&t_channel), &t_volume);
	
	if (t_success)
		MCSoundSetVolumeOfChannel(ctxt, *t_channel, t_volume);
	    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSoundChannelVolume(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSoundChannelVolume */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	int32_t t_volume;
	if (t_success)
		t_success = MCSoundGetVolumeOfChannel(t_ctxt, t_channel, t_volume);
	
	if (t_success)
		MCresult -> setnvalue(t_volume);
	
	delete t_channel;
	
    return t_ctxt.GetStat();
#endif /* MCHandleSoundChannelVolume */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	int32_t t_volume;
	if (t_success)
        MCSoundGetVolumeOfChannel(ctxt, *t_channel, t_volume);
		
	if (!ctxt . HasError())
    {
        ctxt.SetTheResultToNumber(t_volume);
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleSoundChannelStatus(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSoundChannelStatus */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	MCSoundChannelStatus t_status;
	if (t_success)
		t_success = MCSoundGetStatusOfChannel(t_ctxt, t_channel, t_status);
	
	if (t_success && t_status >= 0)
	{
		static const char *s_status_strings[] =
		{
			"stopped",
			"paused",
			"playing"
		};
		MCresult -> sets(s_status_strings[t_status]);
	}
	
	delete t_channel;
	
    return t_ctxt.GetStat();
#endif /* MCHandleSoundChannelStatus */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
	intenum_t t_status;
	if (t_success)
		MCSoundGetStatusOfChannel(ctxt, *t_channel, t_status);
	
	if (t_success && t_status >= 0 && !ctxt.HasError())
	{
        MCAutoStringRef t_status_string;
        if (MCSoundChannelStatusTypeToString((MCSoundChannelStatus)t_status, &t_status_string))
        {
            ctxt.SetTheResultToValue(*t_status_string);
            return ES_NORMAL;
        }
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleSoundOnChannel(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSoundOnChannel */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
    MCAutoRawCString t_sound;
	if (t_success)
		t_success = MCSoundGetSoundOfChannel(t_ctxt, t_channel, t_sound);
	
    if (t_success)
        if (t_sound.Borrow() != nil)
            ep.copysvalue(t_sound.Borrow());
    
    if (t_success)
        MCresult->store(ep, False);
    
    return t_ctxt.GetStat();
#endif /* MCHandleSoundOnChannel */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
    MCAutoStringRef t_sound;
	if (t_success)
		MCSoundGetSoundOfChannel(ctxt, *t_channel, &t_sound);
	
    
    if (t_success &&
        !ctxt . HasError() &&
        *t_sound != nil)
    {
        ctxt . SetTheResultToValue(*t_sound);
        return ES_NORMAL;
    }
    
	return ES_ERROR;
}

Exec_stat MCHandleNextSoundOnChannel(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleNextSoundOnChannel */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
    MCAutoRawCString t_sound;
	if (t_success)
		t_success = MCSoundGetNextSoundOfChannel(t_ctxt, t_channel, t_sound);
	
    if (t_success)
        if (t_sound.Borrow() != nil)
            ep.copysvalue(t_sound.Borrow());
    
    if (t_success)
        MCresult->store(ep, False);
    
    return t_ctxt.GetStat();
#endif /* MCHandleNextSoundOnChannel */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_channel;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_channel));
	
    MCAutoStringRef t_sound;
	if (t_success)
		MCSoundGetNextSoundOfChannel(ctxt, *t_channel, &t_sound);
	
    
    if (t_success &&
        !ctxt . HasError() &&
        *t_sound != nil)
    {
        ctxt . SetTheResultToValue(*t_sound);
		return ES_NORMAL;
    }
    
    return ES_ERROR;
}

Exec_stat MCHandleSoundChannels(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSoundChannels */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    bool t_success;
	t_success = true;
    
    MCAutoRawCString t_channels;
	if (t_success)
		t_success = MCSoundGetSoundChannels(t_ctxt, t_channels);
	
    if (t_success)
        if (t_channels.Borrow() != nil)
            ep.copysvalue(t_channels.Borrow());
    
    if (t_success)
        MCresult->store(ep, False);
    
    return t_ctxt.GetStat();
#endif /* MCHandleSoundChannels */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
    bool t_success;
	t_success = true;
    
    MCAutoStringRef t_channels;
	if (t_success)
		MCSoundGetSoundChannels(ctxt, &t_channels);
    
    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_channels);
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

// MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
Exec_stat MCHandleSetAudioCategory(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetAudioCategory */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_category_string;
	t_category_string = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_category_string);
    
    MCSoundAudioCategory t_category;
    t_category = kMCMCSoundAudioCategoryUnknown;
    if (t_success)
    {
        if (MCCStringEqualCaseless(t_category_string, "ambient"))
            t_category = kMCMCSoundAudioCategoryAmbient;
        else if (MCCStringEqualCaseless(t_category_string, "solo ambient"))
            t_category = kMCMCSoundAudioCategorySoloAmbient;
        else if (MCCStringEqualCaseless(t_category_string, "playback"))
            t_category = kMCMCSoundAudioCategoryPlayback;
        else if (MCCStringEqualCaseless(t_category_string, "record"))
            t_category = kMCMCSoundAudioCategoryRecord;
        else if (MCCStringEqualCaseless(t_category_string, "play and record"))
            t_category = kMCMCSoundAudioCategoryPlayAndRecord;
        else if (MCCStringEqualCaseless(t_category_string, "audio processing"))
            t_category = kMCMCSoundAudioCategoryAudioProcessing;
    }
    
    if (t_success)
        t_success = MCSoundSetAudioCategory(t_ctxt, t_category);
    
    if (t_success)
        MCresult->store(ep, False);
    
    MCCStringFree(t_category_string);
    
    return t_ctxt.GetStat();
#endif /* MCHandleSetAudioCategory */
    MCExecContext ctxt(nil, nil, nil);
	ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_category_string;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_category_string));
    
    MCSoundAudioCategory t_category;
    t_category = kMCSoundAudioCategoryUnknown;
    if (t_success)
    {
        MCSoundAudioCategoryFromString(*t_category_string);
    }
    
    if (t_success)
        MCSoundSetAudioCategory(ctxt, t_category);
        
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

/////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleGetDeviceToken (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetDeviceToken */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCGetDeviceTokenExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleGetDeviceToken */
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_token;
    
    MCMiscGetDeviceToken(ctxt, &t_token);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_token);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleGetLaunchUrl (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetLaunchUrl */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCGetLaunchUrlExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleGetLaunchUrl */
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_url;
    
    MCMiscGetLaunchUrl (ctxt, &t_url);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_url);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleGetLaunchData(void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
	
	MCAutoArrayRef t_data;
	
	MCMiscGetLaunchData(ctxt, &t_data);
	
	if (!ctxt.HasError())
	{
		ctxt.SetTheResultToValue(*t_data);
		return ES_NORMAL;
	}
	
	ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleBeep(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleBeep */ LEGACY_EXEC
    int32_t t_number_of_times = 1;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_number_of_times = ep . getint4();
    }
    MCSystemBeep(t_number_of_times);
  	return ES_NORMAL;
#endif /* MCHandleBeep */
    int32_t t_number_of_times;
    int32_t* t_number_of_times_ptr = nil;
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (!MCParseParameters(p_parameters, "i", &t_number_of_times))
        t_number_of_times_ptr = &t_number_of_times;
    
    MCMiscExecBeep(ctxt, t_number_of_times_ptr);
    
    if (!ctxt.HasError())
        return ES_NORMAL;

    return ES_ERROR;
}

Exec_stat MCHandleVibrate(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleVibrate */ LEGACY_EXEC
    int32_t t_number_of_times = 1;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    if (p_parameters)
    {
        p_parameters->eval(ep);
        // PM-2014-05-23: [[ Bug 12055 ]] Make sure that
        ep.ton();
        t_number_of_times = ep . getint4();
    }
    MCSystemVibrate(t_number_of_times);
	return ES_NORMAL;
#endif /* MCHandleVibrate */
    int32_t t_number_of_times;
    int32_t* t_number_of_times_ptr = nil;
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (MCParseParameters(p_parameters, "i", &t_number_of_times))
        t_number_of_times_ptr = &t_number_of_times;
    
    MCMiscExecVibrate(ctxt, t_number_of_times_ptr);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDeviceResolution(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_resolution;
    MCMiscGetDeviceResolution(ctxt, &t_resolution);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_resolution);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleUseDeviceResolution(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleUseDeviceResolution */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);
	
	bool t_use_device_res;
	t_use_device_res = false;
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_use_device_res = (ep . getsvalue() == MCtruemcstring);
		p_parameters = p_parameters -> getnext();
	}
	
	bool t_use_control_device_res;
	t_use_control_device_res = false;
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_use_control_device_res = (ep . getsvalue() == MCtruemcstring);
		p_parameters = p_parameters -> getnext();
	}
	
	
	MCIPhoneUseDeviceResolution(t_use_device_res, t_use_control_device_res);
	
	return ES_NORMAL;
#endif /* MCHandleUseDeviceResolution */
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_use_device_res;
    bool t_use_control_device_res;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "bb", &t_use_device_res, &t_use_control_device_res);
    
    if (t_success)
        MCMiscSetUseDeviceResolution(ctxt, t_use_device_res, t_use_control_device_res);
    
    if (t_success && !ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDeviceScale(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    real64_t t_resolution;
    
    MCMiscGetDeviceScale(ctxt, t_resolution);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToNumber(t_resolution);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandlePixelDensity(void* context, MCParameter* p_parameters)
{
#ifdef /* MCHandlePixelDensity */ LEGACY_EXEC
	float t_density;
	MCAndroidEngineRemoteCall("getPixelDensity", "f", &t_density);
	MCresult -> setnvalue(t_density);
	return ES_NORMAL;
#endif /* MCHandlePixelDensity */
    MCExecContext ctxt(nil, nil, nil);
    
    real64_t t_density;
    
    MCMiscGetPixelDensity(ctxt, t_density);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToNumber(t_density);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

// SN-2014-10-15: [[ Merge-6.7.0-rc-3 ]]
static Exec_stat MCHandleLocationAuthorizationStatus(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_status;
    MCExecContext ctxt(nil, nil,nil);

    MCSensorGetLocationAuthorizationStatus(ctxt, &t_status);

    if (!ctxt . HasError())
    {
        ctxt . SetTheResultToValue(*t_status);
        return ES_NORMAL;
    }

    ctxt . SetTheResultToEmpty();
    return ES_ERROR;
}


static MCMiscStatusBarStyle MCMiscStatusBarStyleFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "translucent", kMCCompareCaseless))
        return kMCMiscStatusBarStyleTranslucent;
    else if (MCStringIsEqualToCString(p_string, "opaque", kMCCompareCaseless))
        return kMCMiscStatusBarStyleOpaque;
    else
        return kMCMiscStatusBarStyleDefault;
}

Exec_stat MCHandleSetStatusBarStyle(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetStatusBarStyle */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);
	
    UIStatusBarStyle t_style;
    t_style = UIStatusBarStyleDefault;
    if (p_parameters != nil)
    {
        p_parameters -> eval_argument(ep);
        if (ep . getsvalue() == "default")
        {
            t_style = UIStatusBarStyleDefault;
            [MCIPhoneGetApplication() setStatusBarSolid:NO];
        }
        else if (ep . getsvalue() == "translucent")
        {
            t_style = UIStatusBarStyleBlackTranslucent;
            [MCIPhoneGetApplication() setStatusBarSolid:NO];
        }
        else if (ep . getsvalue() == "opaque")
        {
            t_style = UIStatusBarStyleBlackOpaque;
            [MCIPhoneGetApplication() setStatusBarSolid:NO];
        }
        // PM-2015-02-17: [[ Bug 14482 ]] "solid" status bar style means opaque and automatically shift down the app view by 20 pixels
        else if (ep . getsvalue() == "solid")
        {
            t_style = UIStatusBarStyleBlackOpaque;
            [MCIPhoneGetApplication() setStatusBarSolid:YES];
        }
	}
	
	[MCIPhoneGetApplication() switchToStatusBarStyle: t_style];
	
	return ES_NORMAL;
#endif /* MCHandleSetStatusBarStyle */
	MCExecContext ctxt(nil, nil, nil);
	
    MCAutoStringRef t_status_bar_style_string;
    MCMiscStatusBarStyle t_status_bar_style;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_status_bar_style_string));
    
    if (t_success)
    {
        t_status_bar_style = MCMiscStatusBarStyleFromString(*t_status_bar_style_string);
        MCMiscSetStatusBarStyle(ctxt, t_status_bar_style);
    }
	
	if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleShowStatusBar(void* context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    bool t_success = true;
        
    if(t_success)
        MCMiscExecShowStatusBar(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleHideStatusBar(void* context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    bool t_success = true;
    
    if(t_success)
        MCMiscExecHideStatusBar(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

static MCMiscKeyboardType MCMiscKeyboardTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "alphabet", kMCCompareCaseless))
        return kMCMiscKeyboardTypeAlphabet;
    else if (MCStringIsEqualToCString(p_string, "numeric", kMCCompareCaseless))
        return kMCMiscKeyboardTypeNumeric;
    else if (MCStringIsEqualToCString(p_string, "decimal", kMCCompareCaseless))
        return kMCMiscKeyboardTypeDecimal;
    else if (MCStringIsEqualToCString(p_string, "number", kMCCompareCaseless))
        return kMCMiscKeyboardTypeNumber;
    else if (MCStringIsEqualToCString(p_string, "phone", kMCCompareCaseless))
        return kMCMiscKeyboardTypePhone;
    else if (MCStringIsEqualToCString(p_string, "email", kMCCompareCaseless))
        return kMCMiscKeyboardTypeEmail;
    else if (MCStringIsEqualToCString(p_string, "url", kMCCompareCaseless))
        return kMCMiscKeyboardTypeUrl;
    else if (MCStringIsEqualToCString(p_string, "contact", kMCCompareCaseless))
        return kMCMiscKeyboardTypeContact;
    else // default
        return kMCMiscKeyboardTypeDefault;
}

Exec_stat MCHandleSetKeyboardType (void *context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
    
    bool t_success = true;
    
    MCAutoStringRef t_keyboard_type_string;
    MCMiscKeyboardType t_keyboard_type;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_keyboard_type_string));
    
    t_keyboard_type = MCMiscKeyboardTypeFromString(*t_keyboard_type_string);
    
    MCMiscSetKeyboardType(ctxt, t_keyboard_type);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    return ES_ERROR;
}

static MCMiscKeyboardReturnKey MCMiscKeyboardReturnKeyTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "go", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyGo;
    else if (MCStringIsEqualToCString(p_string, "google", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyGoogle;
    else if (MCStringIsEqualToCString(p_string, "join", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyJoin;
    else if (MCStringIsEqualToCString(p_string, "next", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyNext;
    else if (MCStringIsEqualToCString(p_string, "route", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyRoute;
    else if (MCStringIsEqualToCString(p_string, "search", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeySearch;
    else if (MCStringIsEqualToCString(p_string, "send", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeySend;
    else if (MCStringIsEqualToCString(p_string, "yahoo", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyYahoo;
    else if (MCStringIsEqualToCString(p_string, "done", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyDone;
    else if (MCStringIsEqualToCString(p_string, "emergency call", kMCCompareCaseless))
        return kMCMiscKeyboardReturnKeyEmergencyCall;
    else // default
        return kMCMiscKeyboardReturnKeyDefault;
}

Exec_stat MCHandleSetKeyboardReturnKey (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetKeyboardReturnKey */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);
    
	if (p_parameters != nil)
	{
		UIReturnKeyType t_type;
		p_parameters -> eval_argument(ep);
		if (ep . getsvalue() == "default")
			t_type = UIReturnKeyDefault;
		else if (ep . getsvalue() == "go")
			t_type = UIReturnKeyGo;
		else if (ep . getsvalue() == "google")
			t_type = UIReturnKeyGoogle;
		else if (ep . getsvalue() == "join")
			t_type = UIReturnKeyJoin;
		else if (ep . getsvalue() == "next")
			t_type = UIReturnKeyNext;
		else if (ep . getsvalue() == "route")
			t_type = UIReturnKeyRoute;
		else if (ep . getsvalue() == "search")
			t_type = UIReturnKeySearch;
		else if (ep . getsvalue() == "send")
			t_type = UIReturnKeySend;
		else if (ep . getsvalue() == "yahoo")
			t_type = UIReturnKeyYahoo;
		else if (ep . getsvalue() == "done")
			t_type = UIReturnKeyDone;
		else if (ep . getsvalue() == "emergency call")
			t_type = UIReturnKeyEmergencyCall;
        
		MCIPhoneSetReturnKeyType(t_type);
	}
	return ES_NORMAL;
#endif /* MCHandleSetKeyboardReturnKey */
	MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_keyboard_return_key_string;
    MCMiscKeyboardReturnKey t_keyboard_return_key;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_keyboard_return_key_string));
    
    if (t_success)
    {
        t_keyboard_return_key = MCMiscKeyboardReturnKeyTypeFromString(*t_keyboard_return_key_string);
        MCMiscSetKeyboardReturnKey(ctxt, t_keyboard_return_key);
    }
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandlePreferredLanguages(void *context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    MCAutoStringRef t_preferred_languages;
    
    MCMiscGetPreferredLanguages(ctxt, &t_preferred_languages);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_preferred_languages);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleCurrentLocale(void *context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_current_locale;
    
    MCMiscGetCurrentLocale(ctxt, &t_current_locale);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_current_locale);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleClearTouches(void* context, MCParameter* p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCMiscExecClearTouches(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleSystemIdentifier(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_identifier;
    
    MCMiscGetSystemIdentifier(ctxt, &t_identifier);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_identifier);
        return ES_NORMAL;
    }

    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleApplicationIdentifier(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_identifier;
    
    MCMiscGetApplicationIdentifier(ctxt, &t_identifier);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_identifier);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleSetReachabilityTarget(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetReachabilityTarget */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	char *t_hostname = nil;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_hostname);
	
	if (t_success)
		t_success = MCReachabilitySetTarget(t_hostname);
	
	MCCStringFree(t_hostname);
	return t_success ? ES_NORMAL : ES_ERROR;
#endif /* MCHandleSetReachabilityTarget */
    MCExecContext ctxt(nil, nil, nil);
    
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_hostname;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_hostname));
	
	if (t_success)
		MCMiscSetReachabilityTarget(ctxt, *t_hostname);
	
	return !ctxt.HasError() ? ES_NORMAL : ES_ERROR;
}

Exec_stat MCHandleReachabilityTarget(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_hostname;
    
    MCMiscGetReachabilityTarget(ctxt, &t_hostname);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_hostname);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

Exec_stat MCHandleExportImageToAlbum(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef r_save_result;
    MCAutoStringRef t_file_name;
    
    bool t_file_extension_ok = false;
    
	MCAutoStringRef t_data_or_id;
    bool t_success = true;
	MCLog("MCHandleExportImageToAlbum() called", nil);
    
    MCExecContext ctxt(nil, nil, nil);
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_data_or_id));    
    
    MCLog("%@", *t_data_or_id);
    
    if (t_success)
    {
        if (!MCParseParameters(p_parameters, "x", &(&t_file_name)))
            t_file_name = kMCEmptyString;
    
        MCMiscExecExportImageToAlbum(ctxt, *t_data_or_id, *t_file_name);
    }
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSetRedrawInterval(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetRedrawInterval */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	int32_t t_interval;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "i", &t_interval);
	
	if (t_success)
	{
		if (t_interval <= 0)
			MCRedrawEnableScreenUpdates();
		else
			MCRedrawDisableScreenUpdates();
        
		[MCIPhoneGetApplication() setRedrawInterval: t_interval];
	}
	
	return ES_NORMAL;
#endif /* MCHandleSetRedrawInterval */
    MCExecContext ctxt(nil, nil, nil);
    
	bool t_success;
	t_success = true;
	
	int32_t t_interval;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "i", &t_interval);
	
	if (t_success)
        MCMiscSetRedrawInterval(ctxt, t_interval);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleSetAnimateAutorotation(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetAnimateAutorotation */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	bool t_enabled;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "b", &t_enabled);
	
	if (t_success)
		[MCIPhoneGetApplication() setAnimateAutorotation: t_enabled];
	
	return ES_NORMAL;
#endif /* MCHandleSetAnimateAutorotation */
    MCExecContext ctxt(nil, nil, nil);
    
	bool t_success;
	t_success = true;

	bool t_enabled;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "b", &t_enabled);
    
    if (t_success)
        MCMiscSetAnimateAutorotation(ctxt, t_enabled);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}


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
	MCExecContext ctxt(nil, nil, nil);
	
    MCAutoStringRef t_path;
    
    bool t_success = true;
	bool t_no_backup;
	t_no_backup = true;
    
	if (t_success)
        t_success = MCParseParameters(p_parameters, "xu", &(&t_path), &t_no_backup);
    
    if (t_success)
        MCMiscSetDoNotBackupFile(ctxt, *t_path, t_no_backup);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
	return ES_ERROR;
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
	MCExecContext ctxt(nil, nil, nil);
    
	MCAutoStringRef t_path;
    bool t_no_backup;
    bool t_success = true;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &(&t_path));
    
    if (t_success)
        MCMiscGetDoNotBackupFile(ctxt, *t_path, t_no_backup);
    
	if (!ctxt . HasError())
    {
        if (t_no_backup)
            ctxt.SetTheResultToValue(kMCTrueString);
        else
            ctxt.SetTheResultToValue(kMCFalseString);
        
		return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

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
	MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_filename;
    MCAutoStringRef t_protection_string;
    
    if (MCParseParameters(p_parameters, "xx", &(&t_filename), &(&t_protection_string)))
	{
        MCMiscSetFileDataProtection(ctxt, *t_filename, *t_protection_string);
	}
    
	if (!ctxt . HasError())
		return ES_NORMAL;
	
	return ES_ERROR;
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
	MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_path;
    MCAutoStringRef t_protection_string;
    
    if (MCParseParameters(p_parameters, "x", &(&t_path)))
        MCMiscGetFileDataProtection(ctxt, *t_path, &t_protection_string);
    
	if (!ctxt . HasError())
    {
        ctxt.SetTheResultToValue(*t_protection_string);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleLibUrlDownloadToFile(void *context, MCParameter *p_parameters)
{
	MCAutoStringRef t_url, t_filename;
	
	MCExecContext ctxt(nil, nil, nil);
	
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "xx", &(&t_url), &(&t_filename));
    
    if (t_success)
        MCMiscExecLibUrlDownloadToFile(ctxt, *t_url, *t_filename);
	
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleBuildInfo(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleBuildInfo */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);
    
	if (p_parameters != nil)
	{
		char *t_value;
		t_value = NULL;
        
		char *t_key;
		t_key = NULL;
        
		p_parameters -> eval_argument(ep);
		t_key = ep . getsvalue() . clone();
        
		if (!MCAndroidGetBuildInfo(t_key, t_value))
			return ES_ERROR;
        
		MCresult->grab(t_value, MCCStringLength(t_value));
        
		MCCStringFree(t_key);
	}
    
	return ES_NORMAL;
#endif /* MCHandleBuildInfo */
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_value, t_key;
    bool t_success;
    
    t_success = MCParseParameters(p_parameters, "x", &(&t_key));
    
    if(t_success)
        MCMiscGetBuildInfo(ctxt, *t_key, &t_value);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_value);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
    return ES_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////

static MCMediaType MCMediaTypeFromString(MCStringRef p_string)
{
#ifdef /* MCMediaTypeFromCString */ LEGACY_EXEC
    const char *t_ptr = p_string;
    MCMediaType t_media_type = kMCunknownMediaType;
    
    while (true)
    {
        while(*t_ptr == ' ' || *t_ptr == ',')
            t_ptr += 1;
        if (*t_ptr == '\0')
            break;
    	// HC-2012-02-01: [[ Bug 9983 ]] - This fix is related as the implementation in the new syntax does not produce a result
        if (MCCStringEqualSubstringCaseless(t_ptr, "podcasts", 7))
            t_media_type = t_media_type | kMCpodcasts;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "songs", 4))
            t_media_type = t_media_type | kMCsongs;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "audiobooks", 9))
            t_media_type = t_media_type | kMCaudiobooks;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "movies", 5))
            t_media_type = t_media_type | kMCmovies;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "musicvideos", 10))
            t_media_type = t_media_type | kMCmusicvideos;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "tv", 2))
            t_media_type = t_media_type | kMCtv;
        else if (MCCStringEqualSubstringCaseless(t_ptr, "videopodcasts", 12))
            t_media_type = t_media_type | kMCvideopodcasts;
		
        while(*t_ptr != ' ' && *t_ptr != ',' && *t_ptr != '\0')
            t_ptr += 1;
		
    }
    return t_media_type;
#endif /* MCMediaTypeFromCString */

    MCMediaType t_media_type = kMCUnknownMediaType;
    
    uindex_t t_pos;
    t_pos = 0;
    while (true)
    {
        while (MCStringGetNativeCharAtIndex(p_string, t_pos) == ' ' || MCStringGetNativeCharAtIndex(p_string, t_pos) == ',')
            t_pos += 1;
        if (MCStringGetLength(p_string) == t_pos)
            break;
    	// HC-2012-02-01: [[ Bug 9983 ]] - This fix is related as the implementation in the new syntax does not produce a result
        
        if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 7), MCSTR("podcasts"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypePodcasts;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 4), MCSTR("songs"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeSongs;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 9), MCSTR("audiobooks"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeAudiobooks;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 5), MCSTR("movies"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeMovies;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 10), MCSTR("musicvideos"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeMusicVideos;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 2), MCSTR("tv"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeTv;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 12), MCSTR("videopodcasts"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeVideoPodcasts;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 12), MCSTR("anyAudio"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeAnyAudio;
        else if (MCStringSubstringIsEqualTo(p_string, MCRangeMake(t_pos, 12), MCSTR("anyVideo"), kMCCompareCaseless))
            t_media_type = t_media_type | kMCMediaTypeAnyVideo;
        while (MCStringGetNativeCharAtIndex(p_string, t_pos) != ' ' && MCStringGetNativeCharAtIndex(p_string, t_pos) != ',' && MCStringGetNativeCharAtIndex(p_string, t_pos) != '\0')
            t_pos += 1;
        
    }
    return t_media_type;
}

//iphonePickMedia [multiple] [, music, podCast, audioBook, anyAudio, movie, tv, videoPodcast, musicVideo, videoITunesU, anyVideo]
Exec_stat MCHandleIPhonePickMedia(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleIPhonePickMedia */ LEGACY_EXEC
	bool t_success, t_allow_multipe_items;
	char *t_option_list;
    const char *r_return_media_types;
	MCMediaType t_media_types;
	
	t_success = true;
	t_allow_multipe_items = false;
	t_media_types = 0;
	
	t_option_list = nil;
	
    MCExecPoint ep(nil, nil, nil);
    
	// Get the options list.
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if (MCCStringEqualCaseless(t_option_list, "true"))
			t_allow_multipe_items = true;
		else if (MCCStringEqualCaseless(t_option_list, "music"))
			t_media_types += kMCsongs;
		else if (MCCStringEqualCaseless(t_option_list, "podCast"))
			t_media_types += kMCpodcasts;
		else if (MCCStringEqualCaseless(t_option_list, "audioBook"))
			t_media_types += kMCaudiobooks;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
		{
			if (MCCStringEqualCaseless(t_option_list, "movie"))
				t_media_types += kMCmovies;
			else if (MCCStringEqualCaseless(t_option_list, "tv"))
				t_media_types += kMCtv;
			else if (MCCStringEqualCaseless(t_option_list, "videoPodcast"))
				t_media_types += kMCvideopodcasts;
			else if (MCCStringEqualCaseless(t_option_list, "musicVideo"))
				t_media_types += kMCmusicvideos;
			else if (MCCStringEqualCaseless(t_option_list, "videoITunesU"))
				t_media_types += kMCmovies;
		}
#endif
        t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
	if (t_media_types == 0)
	{
		t_media_types = MCMediaTypeFromCString("podcast, songs, audiobook");;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
			t_media_types += MCMediaTypeFromCString("movies, tv, videoPodcasts, musicVideos, videoITunesU");;
#endif
	}
    MCExecContext t_ctxt(ep);
    
	// Call MCIPhonePickMedia to process the media pick selection.
    MCDialogExecPickMedia(t_ctxt, &t_media_types, t_allow_multipe_items, r_return_media_types);
	
	return ES_NORMAL;
#endif /* MCHandleIPhonePickMedia */
	bool t_success, t_allow_multipe_items;
	char *t_option_list;
    const char *r_return_media_types;
	MCMediaType t_media_types;
	
	t_success = true;
	t_allow_multipe_items = false;
	t_media_types = 0;
	
	t_option_list = nil;
	
    MCExecContext ctxt(nil, nil, nil);
    
	// Get the options list.
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if (MCCStringEqualCaseless(t_option_list, "true"))
			t_allow_multipe_items = true;
		else if (MCCStringEqualCaseless(t_option_list, "music"))
			t_media_types |= kMCMediaTypeSongs;
		else if (MCCStringEqualCaseless(t_option_list, "podcast"))
			t_media_types += kMCMediaTypePodcasts;
		else if (MCCStringEqualCaseless(t_option_list, "audiobook"))
			t_media_types += kMCMediaTypeAudiobooks;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
		{
			if (MCCStringEqualCaseless(t_option_list, "movie"))
				t_media_types += kMCMediaTypeMovies;
			else if (MCCStringEqualCaseless(t_option_list, "tv"))
				t_media_types += kMCMediaTypeTv;
			else if (MCCStringEqualCaseless(t_option_list, "videoPodcast"))
				t_media_types += kMCMediaTypeVideoPodcasts;
			else if (MCCStringEqualCaseless(t_option_list, "musicVideo"))
				t_media_types += kMCMediaTypeMusicVideos;
			else if (MCCStringEqualCaseless(t_option_list, "videoITunesU"))
				t_media_types += kMCMediaTypeMovies;
		}
#endif
		t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
	if (t_media_types == 0)
	{
		t_media_types = MCMediaTypeFromString(MCSTR("podcast, songs, audiobook"));;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
			t_media_types += MCMediaTypeFromString(MCSTR("movies, tv, videoPodcasts, musicVideos, videoITunesU"));;
#endif
	}
    
	// Call MCIPhonePickMedia to process the media pick selection.
    MCPickExecPickMedia(ctxt, (intset_t)t_media_types, t_allow_multipe_items);
	
	return ES_NORMAL;
}

Exec_stat MCHandlePickMedia(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePickMedia */ LEGACY_EXEC
	bool t_success;
    bool t_audio = false;
    bool t_video = false;
    char *t_option_list;
    
    s_media_status = kMCAndroidMediaWaiting;
    
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if ((MCCStringEqualCaseless(t_option_list, "music")) ||
		    (MCCStringEqualCaseless(t_option_list, "podCast")) ||
		    (MCCStringEqualCaseless(t_option_list, "audioBook")) ||
            (MCCStringEqualCaseless(t_option_list, "anyAudio")))
        {
            t_audio = true;
        }
		if ((MCCStringEqualCaseless(t_option_list, "movie")) ||
			(MCCStringEqualCaseless(t_option_list, "tv")) ||
            (MCCStringEqualCaseless(t_option_list, "videoPodcast")) ||
            (MCCStringEqualCaseless(t_option_list, "musicVideo")) ||
            (MCCStringEqualCaseless(t_option_list, "videoITunesU")) ||
            (MCCStringEqualCaseless(t_option_list, "anyVideo")))
        {
            t_video = true;
		}
		t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
	if (t_audio && !t_video)
	{
        MCAndroidEngineCall("pickMedia", "vs", nil, "audio/*");
	}
	else if (!t_audio && t_video)
	{
        MCAndroidEngineCall("pickMedia", "vs", nil, "video/*");
	}
    else
	{
        MCAndroidEngineCall("pickMedia", "vs", nil, "audio/* video/*");
	}
    
    while (s_media_status == kMCAndroidMediaWaiting)
        MCscreen->wait(60.0, False, True);
    MCresult -> setvalueref(s_media_content);
    //    MCLog("Media Types Returned: %s", s_media_content);
    
	return ES_NORMAL;
#endif /* MCHandlePickMedia */
	bool t_success;
    t_success = true;
    char *t_option_list;
    MCMediaType t_media_type;
    t_media_type = kMCUnknownMediaType;
    
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if ((MCCStringEqualCaseless(t_option_list, "music")) ||
		    (MCCStringEqualCaseless(t_option_list, "podCast")) ||
		    (MCCStringEqualCaseless(t_option_list, "audioBook")) ||
            (MCCStringEqualCaseless(t_option_list, "anyAudio")))
        {
            t_media_type += kMCMediaTypeAnyAudio;
        }
		if ((MCCStringEqualCaseless(t_option_list, "movie")) ||
			(MCCStringEqualCaseless(t_option_list, "tv")) ||
            (MCCStringEqualCaseless(t_option_list, "videoPodcast")) ||
            (MCCStringEqualCaseless(t_option_list, "musicVideo")) ||
            (MCCStringEqualCaseless(t_option_list, "videoITunesU")) ||
            (MCCStringEqualCaseless(t_option_list, "anyVideo")))
        {
            t_media_type += kMCMediaTypeAnyVideo;
		}
		t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
    MCExecContext ctxt(nil, nil, nil);
    
    MCPickExecPickMedia(ctxt, (intset_t)t_media_type, false);
    
	return ES_NORMAL;
}

Exec_stat MCHandlePick(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePick */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
	bool t_use_cancel, t_use_done, t_use_picker, t_use_checkmark, t_more_optional, t_success;
	t_success = true;
	t_more_optional = true;
	t_use_checkmark = false;
	t_use_done = false;
	t_use_cancel = false;
	t_use_picker = false;
	
    char *t_options_list = nil;
    const_cstring_array_t *t_option_list_array = nil;
    const_int32_array_t r_picked_options = {nil, 0};
    
	uint32_t t_initial_index;
    const_int32_array_t *t_initial_index_array = nil;
	
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    
	t_options_list = nil;
	// get the mandatory options list and the initial index
    // HC-30-2011-30 [[ Bug 10036 ]] iPad pick list only returns 0.
	t_success = MCParseParameters(p_parameters, "s", &t_options_list);
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
        if (!t_success)
        {
            // Degrade gracefully, even if the second mandatory parameter is not supplied.
            t_initial_index = 0;
            t_success = true;
        }
    }
    if (t_success)
        
        if (t_success)
            t_success = MCMemoryNew(t_option_list_array);
    if (t_success)
    {
        t_option_list_array->length = 0;
        t_option_list_array->elements = nil;
    }
    if (t_success)
        t_success = MCMemoryNew(t_initial_index_array);
    if (t_success)
    {
        t_initial_index_array->length = 0;
        t_initial_index_array->elements = nil;
    }
    
	// get the optional option lists, initial indexes and the style
	while (t_more_optional && t_success)
	{
        if (t_success)
            t_success = MCMemoryResizeArray(t_option_list_array->length + 1, t_option_list_array->elements, t_option_list_array->length);
        if (t_success)
            t_option_list_array->elements[t_option_list_array->length - 1] = t_options_list;
        
		// convert the initial index for each component into an array entry
        if (t_success)
            t_success = MCMemoryResizeArray(t_initial_index_array->length + 1, t_initial_index_array->elements, t_initial_index_array->length);
        
        if (t_success)
            t_initial_index_array->elements[t_initial_index_array->length - 1] = t_initial_index;
        
		t_success = MCParseParameters(p_parameters, "s", &t_options_list);
        // HC-2011-09-28 [[ Picker Buttons ]] Updated parameter parsing so we do not skip more than one paramter
		if (t_success)
		{
			if (t_options_list != nil && (MCCStringEqualCaseless(t_options_list, "checkmark")) ||
				(MCCStringEqualCaseless(t_options_list, "cancel")) ||
				(MCCStringEqualCaseless(t_options_list, "done")) ||
				(MCCStringEqualCaseless(t_options_list, "cancelDone")) ||
				(MCCStringEqualCaseless(t_options_list, "picker")))
			{
				t_more_optional = false;
				// HC-2011-09-28 [[ Picker Buttons ]] Get the button values that are to be displayed.
				while (t_options_list != nil)
				{
					t_success = true;
					if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "checkmark"))
						t_use_checkmark = true;
					else if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "cancel"))
						t_use_cancel = true;
					else if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "done"))
						t_use_done = true;
					else if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "cancelDone"))
					{
						t_use_cancel = true;
						t_use_done = true;
					}
					else if (t_options_list != nil && MCCStringEqualCaseless(t_options_list, "picker"))
						t_use_picker = true;
					else
						t_success = false;
					if (!MCParseParameters(p_parameters, "s", &t_options_list))
						t_options_list = nil;
				}
			}
			else
				t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
		}
		else
		{
			t_success = true;
			t_more_optional = false;
		}
	}
    
	// call MCSystemPick to process the pick wheel
	MCDialogExecPickOptionByIndex(t_ctxt, kMCChunkTypeLine, t_option_list_array, t_initial_index_array, t_use_checkmark, t_use_picker, t_use_cancel, t_use_done, r_picked_options, MCtargetptr->getrect());
    
	
	if (t_success)
    {
        // at the moment, this is the only way to return a value from the function.  pick (date/time/...) should probably
        // set the value of the 'it' variable
        if (MCresult->isempty())
            MCresult->store(ep, True);
    }
	
	return t_ctxt.GetStat();
#endif /* MCHandlePick */
    MCExecContext ctxt(nil, nil, nil);
    
	bool t_use_cancel, t_use_done, t_use_picker, t_use_checkmark, t_success, t_has_buttons;
	t_success = true;
	t_use_checkmark = false;
	t_use_done = false;
	t_use_cancel = false;
	t_use_picker = false;
    t_has_buttons = false;
	
    
    MCAutoArray<MCStringRef> t_option_lists;
    MCAutoArray<uindex_t> t_indices;
    
    MCStringRef t_string_param;
   	uint32_t t_initial_index;
    // get the mandatory options list and the initial index
    // HC-30-2011-30 [[ Bug 10036 ]] iPad pick list only returns 0.
	t_success = MCParseParameters(p_parameters, "x", &t_string_param);
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
        if (!t_success)
        {
            // Degrade gracefully, even if the second mandatory parameter is not supplied.
            t_initial_index = 0;
            t_success = true;
        }
        t_option_lists . Push(t_string_param);
        t_indices . Push(t_initial_index);
    }
    
    // get further options lists if they exist
    while (t_success && p_parameters != nil)
    {
    	t_success = MCParseParameters(p_parameters, "x", &t_string_param);
        if (t_success)
        {
            if (MCStringIsEqualToCString(t_string_param, "checkmark", kMCCompareCaseless) ||
                MCStringIsEqualToCString(t_string_param, "cancel", kMCCompareCaseless) ||
                MCStringIsEqualToCString(t_string_param, "done", kMCCompareCaseless) ||
                MCStringIsEqualToCString(t_string_param, "cancelDone", kMCCompareCaseless) ||
                MCStringIsEqualToCString(t_string_param, "picker", kMCCompareCaseless))
            {
                t_has_buttons = true;
                break;
            }
            else
            {
                t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
                if (!t_success)
                {
                    // Degrade gracefully, even if the second mandatory parameter is not supplied.
                    t_initial_index = 0;
                    t_success = true;
                }
                t_option_lists . Push(t_string_param);
                t_indices . Push(t_initial_index);
            }
        }
    }
	
    // PM-2015-09-01: [[ Bug 15816 ]] Process any additional parameters correctly
    while (t_success && t_has_buttons && t_string_param != nil)
    {
        if (MCStringIsEqualToCString(t_string_param, "checkmark", kMCCompareCaseless))
            t_use_checkmark = true;
        else if (MCStringIsEqualToCString(t_string_param, "cancel", kMCCompareCaseless))
			t_use_cancel = true;
        else if (MCStringIsEqualToCString(t_string_param, "done", kMCCompareCaseless))
			t_use_done = true;
        else if (MCStringIsEqualToCString(t_string_param, "canceldone", kMCCompareCaseless))
		{
			t_use_cancel = true;
			t_use_done = true;
		}
        else if (MCStringIsEqualToCString(t_string_param, "picker", kMCCompareCaseless))
            t_use_picker = true;
        
        MCValueRelease(t_string_param);
		t_string_param = nil;
		
		if (p_parameters != nil)
			t_success = MCParseParameters(p_parameters, "x", &t_string_param);
		
    }
    
    ctxt.SetTheResultToEmpty();
        
	// call the Exec method to process the pick wheel
    // The function sets the result itself.
	MCPickExecPickOptionByIndex(ctxt, kMCChunkTypeLine, t_option_lists . Ptr(), t_option_lists . Size(), t_indices . Ptr(), t_indices . Size(),t_use_checkmark, t_use_picker, t_use_cancel, t_use_done, MCtargetptr . object -> getrect());
    
    // Free memory
    for (uindex_t i = 0; i < t_option_lists . Size(); i++)
        MCValueRelease(t_option_lists[i]);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

// MM-2012-11-02: Temporarily refactored mobilePickDate to use the old syntax (rather than three separate pick date, pick time, pick date and time).
Exec_stat MCHandlePickDate(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePickDate */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    char *t_type;
    t_type = nil;
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCDateTime t_current_date;
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    
    uint32_t t_step = 1;
    
    if (t_success && p_parameters != nil)
  		t_success = MCParseParameters(p_parameters, "s", &t_type);
    
    if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_current = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_current_date);
        }
        p_parameters = p_parameters->getnext();
    }
    
    if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_start = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_end = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
    if (t_success && p_parameters != nil)
        t_success = MCParseParameters(p_parameters, "u", &t_step);
    
    if (t_success && p_parameters != nil)
    {
        char *t_button;
        t_button = nil;
		t_success = MCParseParameters(p_parameters, "s", &t_button);
        if (t_success)
        {
            if (MCCStringEqualCaseless("cancel", t_button))
                t_use_cancel = true;
            else if (MCCStringEqualCaseless("done", t_button))
                t_use_done = true;
            else if (MCCStringEqualCaseless("canceldone", t_button))
                t_use_cancel = t_use_done = true;
        }
        MCCStringFree(t_button);
    }
    
    
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    
    MCDateTime *t_current_date_ptr = nil;
    MCDateTime *t_start_date_ptr = nil;
    MCDateTime *t_end_date_ptr = nil;
    
    if (t_use_current)
        t_current_date_ptr = &t_current_date;
    if (t_use_start)
        t_start_date_ptr = &t_start_date;
    if (t_use_end)
        t_end_date_ptr = &t_end_date;
    
	if (t_success)
    {
        // MM-2012-03-15: [[ Bug ]] Make sure we handle no type being passed.
        if (t_type == nil)
            MCDialogExecPickDate(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_use_cancel, t_use_done, MCtargetptr->getrect());
        else if (MCCStringEqualCaseless("time", t_type))
            MCDialogExecPickTime(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_step, t_use_cancel, t_use_done, MCtargetptr->getrect());
        else if (MCCStringEqualCaseless("datetime", t_type))
            MCDialogExecPickDateAndTime(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_step, t_use_cancel, t_use_done, MCtargetptr->getrect());
        else
            MCDialogExecPickDate(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_use_cancel, t_use_done, MCtargetptr->getrect());
    }
    
    MCCStringFree(t_type);
    
    // at the moment, this is the only way to return a value from the function.  pick (date/time/...) should probably
    // set the value of the 'it' variable
    if (MCresult->isempty())
        MCresult->store(ep, True);
    
	return t_ctxt.GetStat();
#endif /* MCHandlePickDate */
    MCExecContext ctxt(nil, nil, nil);
    
	bool t_success;
	t_success = true;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    char *t_type;
    t_type = nil;
    
    if (t_success && p_parameters != nil)
  		t_success = MCParseParameters(p_parameters, "s", &t_type);
    
    MCAutoStringRef t_current, t_start, t_end;
    
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_end);
        p_parameters = p_parameters->getnext();
    }
	
    int32_t t_step;
    int32_t *t_step_ptr = nil;
    if (t_success && p_parameters != nil)
        if (MCParseParameters(p_parameters, "i", &t_step))
            t_step_ptr = &t_step;
    
    MCPickButtonType t_button_type = kMCPickButtonNone;
    if (t_success && p_parameters != nil)
    {
        char *t_button;
        t_button = nil;
		t_success = MCParseParameters(p_parameters, "s", &t_button);
        if (t_success)
        {
            if (MCCStringEqualCaseless("cancel", t_button))
                t_button_type = kMCPickButtonCancel;
            else if (MCCStringEqualCaseless("done", t_button))
                t_button_type = kMCPickButtonDone;
            else if (MCCStringEqualCaseless("canceldone", t_button))
                t_button_type = kMCPickButtonCancelAndDone;
        }
        MCCStringFree(t_button);
    }
     
	if (t_success)
    {
        // MM-2012-03-15: [[ Bug ]] Make sure we handle no type being passed.
        if (t_type == nil)
            MCPickExecPickDate(ctxt, *t_current, *t_start, *t_end, (intenum_t)t_button_type, MCtargetptr . object -> getrect());
        else if (MCCStringEqualCaseless("time", t_type))
            MCPickExecPickTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr . object -> getrect());
        else if (MCCStringEqualCaseless("datetime", t_type))
            MCPickExecPickDateAndTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr . object -> getrect());
        else
            MCPickExecPickDate(ctxt, *t_current, *t_start, *t_end, (intenum_t)t_button_type, MCtargetptr . object -> getrect());
    }
    
    MCCStringFree(t_type);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandlePickTime(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePickTime */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCDateTime t_current_date;
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    
    uint32_t t_step = 1;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_current = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_current_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_start = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_end = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
    if (t_success && p_parameters != nil)
        t_success = MCParseParameters(p_parameters, "u", &t_step);
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    
    MCDateTime *t_current_date_ptr = nil;
    MCDateTime *t_start_date_ptr = nil;
    MCDateTime *t_end_date_ptr = nil;
    
    if (t_use_current)
        t_current_date_ptr = &t_current_date;
    if (t_use_start)
        t_start_date_ptr = &t_start_date;
    if (t_use_end)
        t_end_date_ptr = &t_end_date;
    
	if (t_success)
		MCDialogExecPickTime(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_step, t_use_cancel, t_use_done, MCtargetptr->getrect());
    
    if (MCresult->isempty())
        MCresult->store(ep, True);
    
	return t_ctxt.GetStat();
#endif /* MCHandlePickTime */
    MCExecContext ctxt(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCAutoStringRef t_current, t_start, t_end;
    
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_end);
        p_parameters = p_parameters->getnext();
    }
	
    int32_t t_step;
    int32_t *t_step_ptr = nil;
    if (t_success && p_parameters != nil)
        if (MCParseParameters(p_parameters, "i", &t_step))
            t_step_ptr = &t_step;
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCPickButtonType t_button_type = kMCPickButtonNone;
    if (t_success)
    {
        if (t_use_cancel)
        {
            if (t_use_done)
                t_button_type = kMCPickButtonCancelAndDone;
            else
                t_button_type = kMCPickButtonCancel;
        }
        else if (t_use_done)
            t_button_type = kMCPickButtonDone;           
    }

    ctxt.SetTheResultToEmpty();
    
	if (t_success)
		MCPickExecPickTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr . object -> getrect());
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}


Exec_stat MCHandlePickDateAndTime(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePickDateAndTime */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCDateTime t_current_date;
    MCDateTime t_start_date;
    MCDateTime t_end_date;
    
    uint32_t t_step = 1;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_current = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_current_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_start = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_start_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_use_end = true;
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_end_date);
        }
        p_parameters = p_parameters->getnext();
    }
	
    if (t_success && p_parameters != nil)
        t_success = MCParseParameters(p_parameters, "u", &t_step);
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    
    MCDateTime *t_current_date_ptr = nil;
    MCDateTime *t_start_date_ptr = nil;
    MCDateTime *t_end_date_ptr = nil;
    
    if (t_use_current)
        t_current_date_ptr = &t_current_date;
    if (t_use_start)
        t_start_date_ptr = &t_start_date;
    if (t_use_end)
        t_end_date_ptr = &t_end_date;
    
	if (t_success)
		MCDialogExecPickDateAndTime(t_ctxt, t_current_date_ptr, t_start_date_ptr, t_end_date_ptr, t_step, t_use_cancel, t_use_done, MCtargetptr->getrect());
    
    if (MCresult->isempty())
        MCresult->store(ep, True);
    
	return t_ctxt.GetStat();
#endif /* MCHandlePickDateAndTime */
    MCExecContext ctxt(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCAutoStringRef t_current, t_start, t_end;
    
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        MCAutoValueRef t_value;
        p_parameters->eval(ctxt, &t_value);
        t_success = ctxt . ConvertToString(*t_value, &t_end);
        p_parameters = p_parameters->getnext();
    }
	
    int32_t t_step;
    int32_t *t_step_ptr = nil;
    if (t_success && p_parameters != nil)
        if (MCParseParameters(p_parameters, "i", &t_step))
            t_step_ptr = &t_step;
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCPickButtonType t_button_type = kMCPickButtonNone;
    if (t_success)
    {
        if (t_use_cancel)
        {
            if (t_use_done)
                t_button_type = kMCPickButtonCancelAndDone;
            else
                t_button_type = kMCPickButtonCancel;
        }
        else if (t_use_done)
            t_button_type = kMCPickButtonDone;
    }
    
    ctxt.SetTheResultToEmpty();
       
	if (t_success)
		MCPickExecPickDateAndTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr . object -> getrect());
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleSpecificCameraFeatures(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSpecificCameraFeatures */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);
	ep . clear();
	
	MCCameraSourceType t_source;
	p_parameters -> eval_argument(ep);
	if (MCU_strcasecmp(ep . getcstring(), "front"))
		t_source = kMCCameraSourceTypeFront;
	else if (MCU_strcasecmp(ep . getcstring(), "rear"))
		t_source = kMCCameraSourceTypeRear;
	else
		return ES_NORMAL;
	
	////////
	
	MCExecContext t_ctxt(ep);
	
	MCCameraFeaturesType t_features_set;
	MCCameraGetFeatures(t_ctxt, t_source, t_features_set);
	
	////////
	
	if ((t_features_set & kMCCameraFeaturePhoto) != 0)
		ep . concatcstring("photo", EC_COMMA, ep . isempty());
	if ((t_features_set & kMCCameraFeatureVideo) != 0)
		ep . concatcstring("video", EC_COMMA, ep . isempty());
	if ((t_features_set & kMCCameraFeatureFlash) != 0)
		ep . concatcstring("flash", EC_COMMA, ep . isempty());
	MCresult -> store(ep, False);
	
	return ES_NORMAL;
#endif /* MCHandleSpecificCameraFeatures */
	MCExecContext ctxt(nil, nil, nil);
	
	MCCameraSourceType t_source;
    MCAutoValueRef t_value;
    MCAutoStringRef t_string;
	p_parameters -> eval_argument(ctxt, &t_value);
    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
    if (MCStringIsEqualToCString(*t_string, "front", kMCCompareCaseless))
		t_source = kMCCameraSourceTypeFront;
	else if (MCStringIsEqualToCString(*t_string, "rear", kMCCompareCaseless))
		t_source = kMCCameraSourceTypeRear;
	else
		return ES_NORMAL;
	
    intset_t t_result;
    
    MCPickGetSpecificCameraFeatures(ctxt, (intenum_t)t_source, t_result);
	
	MCCameraFeaturesType t_features_set;
    t_features_set = (MCCameraFeaturesType)t_result;
    MCAutoListRef t_list;
    /* UNCHECKED */ MCListCreateMutable(',', &t_list);
    
	if ((t_features_set & kMCCameraFeaturePhoto) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "photo");
	if ((t_features_set & kMCCameraFeatureVideo) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "video");
	if ((t_features_set & kMCCameraFeatureFlash) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "flash");
	
    MCAutoStringRef t_features;
    /* UNCHECKED */ MCListCopyAsString(*t_list, &t_features);
    ctxt . SetTheResultToValue(*t_features);
    
	return ES_NORMAL;
}

Exec_stat MCHandleCameraFeatures(void *context, MCParameter *p_parameters)
{
    if (p_parameters != nil)
		return MCHandleSpecificCameraFeatures(context, p_parameters);
    
    MCExecContext ctxt(nil, nil, nil);
    
    intset_t t_features;
    
    MCPickGetCameraFeatures(ctxt, t_features);
    
    MCCamerasFeaturesType t_features_set;
    t_features_set = (MCCamerasFeaturesType)t_features;
    
    MCAutoListRef t_list;
    /* UNCHECKED */ MCListCreateMutable(',', &t_list);
    
	if ((t_features_set & kMCCamerasFeatureFrontPhoto) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "front photo");
	if ((t_features_set & kMCCamerasFeatureFrontVideo) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "front video");
	if ((t_features_set & kMCCamerasFeatureFrontFlash) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "front flash");
   	if ((t_features_set & kMCCamerasFeatureRearPhoto) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "rear photo");
	if ((t_features_set & kMCCamerasFeatureRearVideo) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "rear video");
	if ((t_features_set & kMCCamerasFeatureRearFlash) != 0)
        /* UNCHECKED */ MCListAppendCString(*t_list, "rear flash");
    
    MCAutoStringRef t_features_string;

    if (!MCListCopyAsString(*t_list, &t_features_string))
        return ES_ERROR;

    ctxt . SetTheResultToValue(*t_features_string);

    return ES_NORMAL;
}

Exec_stat MCHandlePickPhoto(void *p_context, MCParameter *p_parameters)
{
	MCExecContext ctxt(nil, nil, nil);
	
	MCParameter *t_source_param, *t_width_param, *t_height_param;
	t_source_param = p_parameters;
	t_width_param = t_source_param != nil ? t_source_param -> getnext() : nil;
	t_height_param = t_width_param != nil ? t_width_param -> getnext() : nil;
	
	int32_t t_width, t_height;
	t_width = t_height = 0;
	if (t_width_param != nil)
	{
		// MW-2013-07-01: [[ Bug 10989 ]] Make sure we force conversion to a number.
        MCAutoValueRef t_value;
        if (t_width_param -> eval_argument(ctxt, &t_value))
            /* UNCHECKED */ ctxt . ConvertToInteger(*t_value, t_width);
	}
	if (t_height_param != nil)
	{
		// MW-2013-07-01: [[ Bug 10989 ]] Make sure we force conversion to a number.
        MCAutoValueRef t_value;
        if (t_height_param -> eval_argument(ctxt, &t_value))
            /* UNCHECKED */ ctxt . ConvertToInteger(*t_value, t_height);
	}
    
    MCLog("%d, %d", t_width, t_height);
    
	MCAutoStringRef t_source;
	if (p_parameters != nil)
	{
        MCAutoValueRef t_value;
		p_parameters -> eval_argument(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_source);
	}
	
	MCPhotoSourceType t_photo_source;
	bool t_is_take;
	t_is_take = false;
	
	if (MCStringIsEqualToCString(*t_source, "library", kMCCompareCaseless))
		t_photo_source = kMCPhotoSourceTypeLibrary;
	else if (MCStringIsEqualToCString(*t_source, "album", kMCCompareCaseless))
		t_photo_source = kMCPhotoSourceTypeAlbum;
	else if (MCStringIsEqualToCString(*t_source, "camera", kMCCompareCaseless))
        t_photo_source = kMCPhotoSourceTypeCamera;
    else if (MCStringIsEqualToCString(*t_source, "rear camera", kMCCompareCaseless))
		t_photo_source = kMCPhotoSourceTypeRearCamera;
	else if (MCStringIsEqualToCString(*t_source, "front camera", kMCCompareCaseless))
		t_photo_source = kMCPhotoSourceTypeFrontCamera;
	else
	{
		MCresult -> sets("unknown source");
		return ES_NORMAL;
	}
	
	/////
	
	ctxt . SetTheResultToEmpty();
	
	if (t_width != 0 && t_height != 0)
		MCPickExecPickPhotoAndResize(ctxt, t_photo_source, t_width, t_height);
	else
		MCPickExecPickPhoto(ctxt, t_photo_source);
    
	if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleControlCreate(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_type_name;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &(&t_type_name));
	
	MCAutoStringRef t_control_name;
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "x", &(&t_control_name));

    MCExecContext ctxt(nil, nil, nil);
    
    MCNativeControlExecCreateControl(ctxt, *t_type_name, *t_control_name);
    
	if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleControlDelete(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
	MCAutoStringRef t_control_name;
	if (MCParseParameters(p_parameters, "x", &(&t_control_name)))
        MCNativeControlExecDeleteControl(ctxt, *t_control_name);

	return ES_NORMAL;
}

Exec_stat MCHandleControlSet(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlSet */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	char *t_prop_name;
	t_control_name = nil;
	t_prop_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_control_name, &t_prop_name);
	
	MCNativeControl *t_control;
	MCNativeControlProperty t_property;
	if (t_success)
		t_success =
		MCNativeControl::FindByNameOrId(t_control_name, t_control) &&
		MCNativeControl::LookupProperty(t_prop_name, t_property);
	
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	if (t_success && p_parameters != nil)
		t_success = p_parameters -> eval(ep);
	
	if (t_success)
        t_control -> Set(ctxt, t_property);
	
	delete t_prop_name;
	delete t_control_name;
	
	return ES_NORMAL;
#endif /* MCHandleControlSet */
    
    MCAutoStringRef t_control_name;
    MCAutoStringRef t_property;
    
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success;
	t_success = true;
    
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &(&t_control_name), &(&t_property));

    MCAutoValueRef t_value;
    if (t_success && p_parameters != nil)
        t_success = p_parameters -> eval(ctxt, &t_value);
    
    if (t_success)
        MCNativeControlExecSet(ctxt, *t_control_name, *t_property, *t_value);
    
    return ES_NORMAL;
}

Exec_stat MCHandleControlGet(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlGet */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	char *t_prop_name;
	t_control_name = nil;
	t_prop_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_control_name, &t_prop_name);
	
	MCNativeControl *t_control;
	MCNativeControlProperty t_property;
	if (t_success)
		t_success =
		MCNativeControl::FindByNameOrId(t_control_name, t_control) &&
		MCNativeControl::LookupProperty(t_prop_name, t_property);
	
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	if (t_success)
		t_control -> Get(ctxt, t_property);
	
	if (t_success)
	{
		MCAutoStringRef t_value;
        ep . copyasstringref(&t_value);
        ctxt . SetTheResultToValue(*t_value);
	}
	else
		ctxt . SetTheResultToEmpty();
    
	delete t_prop_name;
	delete t_control_name;
	
	return ES_NORMAL;
#endif /* MCHandleControlGet */
    
    MCAutoStringRef t_control_name;
    MCAutoStringRef t_property;
        
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success;
	t_success = true;
    
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &(&t_control_name), &(&t_property));
    
    MCAutoValueRef t_value;
    if (t_success)
        MCNativeControlExecGet(ctxt, *t_control_name, *t_property, &t_value);
    
    if (*t_value != nil)
        ctxt . SetTheResultToValue(*t_value);
    else
        ctxt . SetTheResultToEmpty();
    
    return ES_NORMAL;
}

Exec_stat MCHandleControlDo(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleControlDo */ LEGACY_EXEC
	bool t_success;
	t_success = true;
	
	char *t_control_name;
	char *t_action_name;
	t_control_name = nil;
	t_action_name = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "ss", &t_control_name, &t_action_name);
	
	MCNativeControl *t_control;
	MCNativeControlAction t_action;
	if (t_success)
		t_success =
		MCNativeControl::FindByNameOrId(t_control_name, t_control) &&
		MCNativeControl::LookupAction(t_action_name, t_action);
	
	if (t_success)
		t_success = t_control -> Do(t_action, p_parameters) == ES_NORMAL;
	
	delete t_action_name;
	delete t_control_name;
	
	return ES_NORMAL;
#endif /* MCHandleControlDo */
    
    MCAutoStringRef t_control_name;
    MCAutoStringRef t_property;
    
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success;
	t_success = true;
    
    if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &(&t_control_name), &(&t_property));
	
    MCAutoArray<MCValueRef> t_params;
    
    MCValueRef t_value;
    while (t_success && p_parameters != nil)
    {
        p_parameters -> eval(ctxt, t_value);
        t_success = t_params . Push(t_value);
        p_parameters = p_parameters -> getnext();
    }

	if (t_success)
		MCNativeControlExecDo(ctxt, *t_control_name, *t_property, t_params . Ptr(), t_params . Size());

    
    // SN-2014-11-20: [[ Bug 14062 ]] Cleanup the memory
    for (uint32_t i = 0; i < t_params . Size(); ++i)
        MCValueRelease(t_params[i]);
	
	return ES_NORMAL;
}

Exec_stat MCHandleControlTarget(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    MCNativeControlIdentifier t_identifier;
    MCNativeControlGetTarget(ctxt, t_identifier);
    MCAutoStringRef t_string;
    MCNativeControlIdentifierFormat(ctxt, t_identifier, &t_string);
    MCNativeControlIdentifierFree(ctxt, t_identifier);
    if (*t_string != nil)
        ctxt . SetTheResultToValue(*t_string);
    
    return ES_NORMAL;
}

bool list_native_controls(void *context, MCNativeControl* p_control)
{
	MCListRef t_list;
	t_list = (MCListRef )context;
    MCAutoStringRef t_name;
	p_control -> GetName(&t_name);
	if (!MCStringIsEmpty(*t_name))
		MCListAppend(t_list, *t_name);
	else
		MCListAppendFormat(t_list, "%u", p_control -> GetId());
	
	return true;
}

Exec_stat MCHandleControlList(void *context, MCParameter *p_parameters)
{
    MCAutoListRef t_list;
    MCListCreateMutable('\n', &t_list);
	MCNativeControl::List(list_native_controls, *t_list);
    
	MCExecContext ctxt(nil, nil, nil);
	MCAutoStringRef t_value;
	/* UNCHECKED */ MCListCopyAsString(*t_list, &t_value);
    ctxt . SetTheResultToValue(*t_value);
    
	return ES_NORMAL;
}

// MW-2013-10-02: [[ MobileSSLVerify ]] Handle libUrlSetSSLVerification
Exec_stat MCHandleLibUrlSetSSLVerification(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	bool t_enabled;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "b", &t_enabled);
    
    MCExecContext ctxt(nil, nil, nil);
    
    if (t_success)
        MCMiscExecLibUrlSetSSLVerification(ctxt, t_enabled);
    
    if (t_success && !ctxt . HasError())
        return ES_NORMAL;
	
	return ES_ERROR;
}

// MM-2013-05-21: [[ Bug 10895 ]] Added iphoneIdentifierForVendor as an initial replacement for iphoneSystemIdentifier.
//  identifierForVendor was only added to UIDevice in iOS 6.1 so make sure we weakly link.
Exec_stat MCHandleIdentifierForVendor(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCAutoStringRef t_id;
    MCMiscGetIdentifierForVendor(ctxt, &t_id);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToValue(*t_id);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleEnableRemoteControl(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCMiscExecEnableRemoteControl(ctxt);

    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDisableRemoteControl(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    MCMiscExecDisableRemoteControl(ctxt);

    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleRemoteControlEnabled(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_enabled;
    MCMiscGetRemoteControlEnabled(ctxt, t_enabled);
    
    if (!ctxt.HasError())
    {
        ctxt.SetTheResultToBool(t_enabled);
        return ES_NORMAL;
    }
    
    ctxt.SetTheResultToEmpty();
	return ES_ERROR;
}

Exec_stat MCHandleSetRemoteControlDisplay(void *context, MCParameter *p_parameters)
{
    MCExecContext ctxt(nil, nil, nil);
    
    bool t_success;
	t_success = true;
    
    MCAutoArrayRef t_props;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "a", &(&t_props));
    
    if (t_success)
        MCMiscSetRemoteControlDisplayProperties(ctxt, *t_props);
    
    if (t_success)
        return ES_NORMAL;
    else
        return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////


typedef Exec_stat (*MCPlatformMessageHandler)(void *context, MCParameter *parameters);

// MW-2012-08-06: [[ Fibers ]] If 'waitable' is true it means the handler must
//   be run on the script fiber. Otherwise it is run on the system fiber (making
//   implementation easier).
struct MCPlatformMessageSpec
{
	bool waitable;
	const char *message;
	MCPlatformMessageHandler handler;
	void *context;
};

static MCPlatformMessageSpec s_platform_messages[] =
{
    // MM-2012-02-22: Added support for ad management
    {false, "mobileAdRegister", MCHandleAdRegister, nil},
    {false, "mobileAdCreate", MCHandleAdCreate, nil},
    {false, "mobileAdDelete", MCHandleAdDelete, nil},
    {false, "mobileAdGetVisible", MCHandleAdGetVisible, nil},
    {false, "mobileAdSetVisible", MCHandleAdSetVisible, nil},
    {false, "mobileAdGetTopLeft", MCHandleAdGetTopLeft, nil},
    {false, "mobileAdSetTopLeft", MCHandleAdSetTopLeft, nil},
    {false, "mobileAds", MCHandleAds, nil},
    {false, "iphoneAdRegister", MCHandleAdRegister, nil},
    {false, "iphoneAdCreate", MCHandleAdCreate, nil},
    {false, "iphoneAdDelete", MCHandleAdDelete, nil},
    {false, "iphoneAdGetVisible", MCHandleAdGetVisible, nil},
    {false, "iphoneAdSetVisible", MCHandleAdSetVisible, nil},
    {false, "iphoneAdGetTopLeft", MCHandleAdGetTopLeft, nil},
    {false, "iphoneAdSetTopLeft", MCHandleAdSetTopLeft, nil},
    {false, "iphoneAds", MCHandleAds, nil},
	
	{true, "libUrlDownloadToFile", MCHandleLibUrlDownloadToFile, nil},
    
    // MW-2013-10-02: [[ MobileSSLVerify ]] Added support for libUrlSetSSLVerification.
    {true, "libUrlSetSSLVerification", MCHandleLibUrlSetSSLVerification, nil},

    // PM-2014-10-07: [[ Bug 13590 ]] StartTrackingSensor and StopTrackingSensor must run on the script thread
    {true, "mobileStartTrackingSensor", MCHandleStartTrackingSensor, nil},
    {true, "mobileStopTrackingSensor", MCHandleStopTrackingSensor, nil},
    {false, "mobileSensorReading", MCHandleSensorReading, nil},
    {false, "mobileSensorAvailable", MCHandleSensorAvailable, nil},
    
    // MM-2012-02-11: Added support old style senseor syntax (iPhoneEnableAcceleromter etc)
	/* DEPRECATED */ {false, "iphoneCanTrackLocation", MCHandleCanTrackLocation, nil},

    // PM-2014-10-07: [[ Bug 13590 ]] StartTrackingLocation and StopTrackingLocation must run on the script thread
    /* DEPRECATED */ {true, "iphoneStartTrackingLocation", MCHandleLocationTrackingState, (void *)true},
    /* DEPRECATED */ {true, "iphoneStopTrackingLocation", MCHandleLocationTrackingState, (void *)false},

	/* DEPRECATED */ {false, "iphoneCurrentLocation", MCHandleCurrentLocation, nil},
    /* DEPRECATED */ {false, "mobileCanTrackLocation", MCHandleCanTrackLocation, nil},

    // PM-2014-10-07: [[ Bug 13590 ]] StartTrackingLocation and StopTrackingLocation must run on the script thread
    /* DEPRECATED */ {true, "mobileStartTrackingLocation", MCHandleLocationTrackingState, (void *)true},
    /* DEPRECATED */ {true, "mobileStopTrackingLocation", MCHandleLocationTrackingState, (void *)false},

	/* DEPRECATED */ {false, "mobileCurrentLocation", MCHandleCurrentLocation, nil},
	
	/* DEPRECATED */ {false, "iphoneCanTrackHeading", MCHandleCanTrackHeading, nil},
	/* DEPRECATED */ {false, "iphoneStartTrackingHeading", MCHandleHeadingTrackingState, (void *)true},
	/* DEPRECATED */ {false, "iphoneStopTrackingHeading", MCHandleHeadingTrackingState, (void *)false},
	/* DEPRECATED */ {false, "iphoneCurrentHeading", MCHandleCurrentHeading, nil},
	{false, "iphoneSetHeadingCalibrationTimeout", MCHandleSetHeadingCalibrationTimeout, nil},
	{false, "iphoneHeadingCalibrationTimeout", MCHandleHeadingCalibrationTimeout, nil},
    /* DEPRECATED */ {false, "mobileCanTrackHeading", MCHandleCanTrackHeading, nil},
    /* DEPRECATED */ {false, "mobileStartTrackingHeading", MCHandleHeadingTrackingState, (void *)true},
	/* DEPRECATED */ {false, "mobileStopTrackingHeading", MCHandleHeadingTrackingState, (void *)false},
	/* DEPRECATED */ {false, "mobileCurrentHeading", MCHandleCurrentHeading, nil},
    
    /* DEPRECATED */ {false, "iphoneEnableAccelerometer", MCHandleAccelerometerEnablement, (void *)true},
	/* DEPRECATED */ {false, "iphoneDisableAccelerometer", MCHandleAccelerometerEnablement, (void *)false},
	/* DEPRECATED */ {false, "mobileEnableAccelerometer", MCHandleAccelerometerEnablement, (void *)true},
	/* DEPRECATED */ {false, "mobileDisableAccelerometer", MCHandleAccelerometerEnablement, (void *)false},
    
    {false, "mobileBusyIndicatorStart", MCHandleStartBusyIndicator, nil},
    {false, "mobileBusyIndicatorStop", MCHandleStopBusyIndicator, nil},
    {false, "iphoneBusyIndicatorStart", MCHandleStartBusyIndicator, nil},
    {false, "iphoneBusyIndicatorStop", MCHandleStopBusyIndicator, nil},
    
    {false, "mobileBeep", MCHandleBeep, nil},
    {true, "mobileVibrate", MCHandleVibrate, nil},
    {false, "iphoneBeep", MCHandleBeep, nil},
    {true, "iphoneVibrate", MCHandleVibrate, nil},
	
    {true, "iphoneComposeTextMessage", MCHandleComposeTextMessage, nil},
    {false, "iphoneCanComposeTextMessage", MCHandleCanComposeTextMessage, nil},
    {true, "mobileComposeTextMessage", MCHandleComposeTextMessage, nil},
    {false, "mobileCanComposeTextMessage", MCHandleCanComposeTextMessage, nil},
    
    {false, "iphoneCameraFeatures", MCHandleCameraFeatures, nil},
    {false, "mobileCameraFeatures", MCHandleCameraFeatures, nil},
	{true, "iphonePickPhoto", MCHandlePickPhoto, nil},
	{true, "mobilePickPhoto", MCHandlePickPhoto, nil},
    
	{true, "iphonePickDate", MCHandlePickDate, nil},
    {true, "mobilePickDate", MCHandlePickDate, nil},
    {true, "mobilePickTime", MCHandlePickTime, nil},
    {true, "mobilePickDateAndTime", MCHandlePickDateAndTime, nil},
    
	{true, "iphonePick", MCHandlePick, nil},
    {true, "mobilePick", MCHandlePick, nil},
    
    {true, "mobilePickMedia", MCHandlePickMedia, nil},
    {true, "iphonePickMedia", MCHandleIPhonePickMedia, nil},
    
	{true, "revMail", MCHandleRevMail, nil},
	{false, "iphoneCanSendMail", MCHandleCanSendMail, nil},
	{true, "iphoneComposeMail", MCHandleComposePlainMail, nil},
	{true, "iphoneComposeUnicodeMail", MCHandleComposeUnicodeMail, nil},
	{true, "iphoneComposeHtmlMail", MCHandleComposeHtmlMail, nil},
    {false, "mobileCanSendMail", MCHandleCanSendMail, nil},
	{true, "mobileComposeMail", MCHandleComposePlainMail, nil},
	{true, "mobileComposeUnicodeMail", MCHandleComposeUnicodeMail, nil},
	{true, "mobileComposeHtmlMail", MCHandleComposeHtmlMail, nil},
    
	{false, "iphoneDeviceOrientation", MCHandleDeviceOrientation, nil},
	{false, "iphoneOrientation", MCHandleOrientation, nil},
	{false, "iphoneAllowedOrientations", MCHandleAllowedOrientations, nil},
	{false, "iphoneSetAllowedOrientations", MCHandleSetAllowedOrientations, nil},
	{false, "iphoneOrientationLocked", MCHandleOrientationLocked, nil},
	{false, "iphoneLockOrientation", MCHandleLockOrientation, nil},
	{false, "iphoneUnlockOrientation", MCHandleUnlockOrientation, nil},
	{false, "mobileDeviceOrientation", MCHandleDeviceOrientation, nil},
	{false, "mobileOrientation", MCHandleOrientation, nil},
	{false, "mobileAllowedOrientations", MCHandleAllowedOrientations, nil},
	{false, "mobileSetAllowedOrientations", MCHandleSetAllowedOrientations, nil},
	{false, "mobileLockOrientation", MCHandleLockOrientation, nil},
	{false, "mobileUnlockOrientation", MCHandleUnlockOrientation, nil},
	{false, "mobileOrientationLocked", MCHandleOrientationLocked, nil},
    
    {false, "mobileGetDeviceToken", MCHandleGetDeviceToken, nil},
    {false, "mobileGetLaunchUrl", MCHandleGetLaunchUrl, nil},
    {false, "iphoneGetDeviceToken", MCHandleGetDeviceToken, nil},
    {false, "iphoneGetLaunchUrl", MCHandleGetLaunchUrl, nil},
	
	{false, "mobileGetLaunchData", MCHandleGetLaunchData, nil},
	
	{false, "iphoneSetStatusBarStyle", MCHandleSetStatusBarStyle, nil},
	{false, "iphoneShowStatusBar", MCHandleShowStatusBar, nil},
	{false, "iphoneHideStatusBar", MCHandleHideStatusBar, nil},
    {false, "mobileSetStatusBarStyle", MCHandleSetStatusBarStyle, nil},
	{false, "mobileShowStatusBar", MCHandleShowStatusBar, nil},
	{false, "mobileHideStatusBar", MCHandleHideStatusBar, nil},
    
	{false, "iphoneSetKeyboardType", MCHandleSetKeyboardType, nil},
	{false, "iphoneSetKeyboardReturnKey", MCHandleSetKeyboardReturnKey, nil},
	{false, "mobileSetKeyboardType", MCHandleSetKeyboardType, nil},
    {false, "mobileSetKeyboardReturnKey", MCHandleSetKeyboardReturnKey, nil}, // Added from androidmisc.cpp
	
	{false, "iphoneDeviceResolution", MCHandleDeviceResolution, nil},
	{false, "iphoneUseDeviceResolution", MCHandleUseDeviceResolution, nil},
	{false, "iphoneDeviceScale", MCHandleDeviceScale, nil},
    {false, "mobileDeviceResolution", MCHandleDeviceResolution, nil},
    {false, "mobileUseDeviceResolution", MCHandleUseDeviceResolution, nil},
    {false, "mobileDeviceScale", MCHandleDeviceScale, nil},
    {false, "mobilePixelDensity", MCHandlePixelDensity, nil},

    // SN-2014-10-15: [[ Merge-6.7.0-rc-3 ]]
    {false, "iphoneLocationAuthorizationStatus", MCHandleLocationAuthorizationStatus, nil},
    {false, "mobileLocationAuthorizationStatus", MCHandleLocationAuthorizationStatus, nil},
    
	{false, "mobileBuildInfo", MCHandleBuildInfo, nil},
	
	{false, "mobileCanMakePurchase", MCHandleCanMakePurchase, nil},
	{false, "mobileEnablePurchaseUpdates", MCHandleEnablePurchaseUpdates, nil},
	{false, "mobileDisablePurchaseUpdates", MCHandleDisablePurchaseUpdates, nil},
	{false, "mobileRestorePurchases", MCHandleRestorePurchases, nil},
	{false, "mobilePurchases", MCHandlePurchaseList, nil},
	{false, "mobilePurchaseCreate", MCHandlePurchaseCreate, nil},
	{false, "mobilePurchaseState", MCHandlePurchaseState, nil},
	{false, "mobilePurchaseError", MCHandlePurchaseError, nil},
	{false, "mobilePurchaseGet", MCHandlePurchaseGet, nil},
	{false, "mobilePurchaseSet", MCHandlePurchaseSet, nil},
	{false, "mobilePurchaseSendRequest", MCHandlePurchaseSendRequest, nil},
	{false, "mobilePurchaseConfirmDelivery", MCHandlePurchaseConfirmDelivery, nil},
    {false, "mobilePurchaseVerify", MCHandlePurchaseVerify, nil},
    {false, "iphoneRequestProductDetails", MCHandleRequestProductDetails, nil},
    
    {false, "mobileStoreCanMakePurchase", MCHandleCanMakePurchase, nil},
    {false, "mobileStoreEnablePurchaseUpdates", MCHandleEnablePurchaseUpdates, nil},
    {false, "mobileStoreDisablePurchaseUpdates", MCHandleDisablePurchaseUpdates, nil},
    {false, "mobileStoreRestorePurchases", MCHandleRestorePurchases, nil},
    {false, "mobileStoreMakePurchase", MCHandleMakePurchase, nil},
    {false, "mobileStoreConfirmPurchase", MCHandleConfirmPurchase, nil},
    {false, "mobileStoreProductProperty", MCHandleGetPurchaseProperty, nil},
    {false, "mobileStoreSetProductType", MCHandleProductSetType, nil},
    {false, "mobileStoreRequestProductDetails", MCHandleRequestProductDetails, nil},
    {false, "mobileStoreConsumePurchase", MCHandleConsumePurchase, nil},
    {false, "mobileStorePurchasedProducts", MCHandleGetPurchases, nil},
    {false, "mobileStorePurchaseError", MCHandlePurchaseError, nil},	
    {false, "mobileStoreVerifyPurchase", MCHandlePurchaseVerify, nil},
	
	{false, "iphoneControlCreate", MCHandleControlCreate, nil},
	{false, "iphoneControlDelete", MCHandleControlDelete, nil},
	{false, "iphoneControlSet", MCHandleControlSet, nil},
	{false, "iphoneControlGet", MCHandleControlGet, nil},
	{true, "iphoneControlDo", MCHandleControlDo, nil},
	{false, "iphoneControlTarget", MCHandleControlTarget, nil},
	{false, "iphoneControls", MCHandleControlList, nil},
	{false, "mobileControlCreate", MCHandleControlCreate, nil},
	{false, "mobileControlDelete", MCHandleControlDelete, nil},
	{false, "mobileControlSet", MCHandleControlSet, nil},
	{false, "mobileControlGet", MCHandleControlGet, nil},
	{true, "mobileControlDo", MCHandleControlDo, nil},
	{false, "mobileControlTarget", MCHandleControlTarget, nil},
	{false, "mobileControls", MCHandleControlList, nil},
	
	{false, "iphonePreferredLanguages", MCHandlePreferredLanguages, nil},
	{false, "mobilePreferredLanguages", MCHandlePreferredLanguages, nil},
	{false, "iphoneCurrentLocale", MCHandleCurrentLocale, nil},
	{false, "mobileCurrentLocale", MCHandleCurrentLocale, nil},
	
	{false, "iphoneApplicationIdentifier", MCHandleApplicationIdentifier, nil},
	{false, "iphoneSystemIdentifier", MCHandleSystemIdentifier, nil},

    // MM-2013-05-21: [[ Bug 10895 ]] Added iphoneIdentifierForVendor as an initial replacement for iphoneSystemIdentifier.
    {false, "mobileIdentifierForVendor", MCHandleIdentifierForVendor, nil},
    {false, "iphoneIdentifierForVendor", MCHandleIdentifierForVendor, nil},
    
	{false, "iphoneSetReachabilityTarget", MCHandleSetReachabilityTarget, nil},
	{false, "iphoneReachabilityTarget", MCHandleReachabilityTarget, nil},
    
    // MM-2012-09-02: Add support for mobile* multi channel sound syntax
    {false, "mobilePlaySoundOnChannel", MCHandlePlaySoundOnChannel, nil},
	{false, "mobilePausePlayingOnChannel", MCHandlePausePlayingOnChannel},
	{false, "mobileResumePlayingOnChannel", MCHandleResumePlayingOnChannel},
	{false, "mobileStopPlayingOnChannel", MCHandleStopPlayingOnChannel, nil},
	{false, "mobileDeleteSoundChannel", MCHandleDeleteSoundChannel, nil},
	{false, "mobileSetSoundChannelVolume", MCHandleSetSoundChannelVolume, nil},
	{false, "mobileSoundChannelVolume", MCHandleSoundChannelVolume, nil},
	{false, "mobileSoundChannelStatus", MCHandleSoundChannelStatus, nil},
	{false, "mobileSoundOnChannel", MCHandleSoundOnChannel, nil},
	{false, "mobileNextSoundOnChannel", MCHandleNextSoundOnChannel, nil},
	{false, "mobileSoundChannels", MCHandleSoundChannels, nil},
	{false, "iphonePlaySoundOnChannel", MCHandlePlaySoundOnChannel, nil},
	{false, "iphonePausePlayingOnChannel", MCHandlePausePlayingOnChannel},
	{false, "iphoneResumePlayingOnChannel", MCHandleResumePlayingOnChannel},
	{false, "iphoneStopPlayingOnChannel", MCHandleStopPlayingOnChannel, nil},
	{false, "iphoneDeleteSoundChannel", MCHandleDeleteSoundChannel, nil},
	{false, "iphoneSetSoundChannelVolume", MCHandleSetSoundChannelVolume, nil},
	{false, "iphoneSoundChannelVolume", MCHandleSoundChannelVolume, nil},
	{false, "iphoneSoundChannelStatus", MCHandleSoundChannelStatus, nil},
	{false, "iphoneSoundOnChannel", MCHandleSoundOnChannel, nil},
	{false, "iphoneNextSoundOnChannel", MCHandleNextSoundOnChannel, nil},
	{false, "iphoneSoundChannels", MCHandleSoundChannels, nil},
    // MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
    {false, "iphoneSetAudioCategory", MCHandleSetAudioCategory, nil},
    {false, "mobileSetAudioCategory", MCHandleSetAudioCategory, nil},
    
    {false, "iphoneSetDoNotBackupFile", MCHandleFileSetDoNotBackup, nil},
    {false, "iphoneDoNotBackupFile", MCHandleFileGetDoNotBackup, nil},
    {false, "iphoneSetFileDataProtection", MCHandleFileSetDataProtection, nil},
    {false, "iphoneFileDataProtection", MCHandleFileGetDataProtection, nil},
	
	{false, "iphoneLockIdleTimer", MCHandleLockIdleTimer, nil},
	{false, "mobileLockIdleTimer", MCHandleLockIdleTimer, nil},
	{false, "iphoneUnlockIdleTimer", MCHandleUnlockIdleTimer, nil},
	{false, "mobileUnlockIdleTimer", MCHandleUnlockIdleTimer, nil},
	{false, "iphoneIdleTimerLocked", MCHandleIdleTimerLocked, nil},
	{false, "mobileIdleTimerLocked", MCHandleIdleTimerLocked, nil},
    
    {false, "mobileCreateLocalNotification", MCHandleCreateLocalNotification, nil},
    {false, "mobileGetRegisteredNotifications", MCHandleGetRegisteredNotifications, nil},
    {false, "mobileGetNotificationDetails", MCHandleGetNotificationDetails, nil},
    {false, "mobileCancelLocalNotification", MCHandleCancelLocalNotification, nil},
    {false, "mobileCancelAllLocalNotifications", MCHandleCancelAllLocalNotifications, nil},
    {false, "iphoneCreateLocalNotification", MCHandleCreateLocalNotification, nil},
    {false, "iphoneGetRegisteredNotifications", MCHandleGetRegisteredNotifications, nil},
    {false, "iphoneCancelLocalNotification", MCHandleCancelLocalNotification, nil},
    {false, "iphoneCancelAllLocalNotifications", MCHandleCancelAllLocalNotifications, nil},
    
    {false, "iphoneGetNotificationBadgeValue", MCHandleGetNotificationBadgeValue, nil},
    {false, "iphoneSetNotificationBadgeValue", MCHandleSetNotificationBadgeValue, nil},
    
	{false, "iphoneActivityIndicatorStart", MCHandleStartActivityIndicator, nil},
	{false, "iphoneActivityIndicatorStop", MCHandleStopActivityIndicator, nil},
	
	{true, "iphoneExportImageToAlbum", MCHandleExportImageToAlbum, nil},
	{true, "mobileExportImageToAlbum", MCHandleExportImageToAlbum, nil},	
    
	{false, "iphoneSetRedrawInterval", MCHandleSetRedrawInterval, nil},
	
    // MW-2012-02-15: [[ Bug 9985 ]] Control whether the autorotation animation happens
    //   or not.
	{false, "iphoneSetAnimateAutorotation", MCHandleSetAnimateAutorotation, nil}, 
    
    {true, "mobilePickContact", MCHandlePickContact, nil},       // ABPeoplePickerNavigationController
    {true, "mobileShowContact", MCHandleShowContact, nil},       // ABPersonViewController
    {true, "mobileGetContactData", MCHandleGetContactData, nil}, // ABNewPersonViewController
    {true, "mobileUpdateContact", MCHandleUpdateContact, nil},   // ABUnknownPersonViewController
    {true, "mobileCreateContact", MCHandleCreateContact, nil},

    // PM-2014-10-08: [[ Bug 13621 ]] Add/Find/Remove contact must run on the script thread
    {true, "mobileAddContact", MCHandleAddContact, nil},
    {true, "mobileFindContact", MCHandleFindContact, nil},
    {true, "mobileRemoveContact", MCHandleRemoveContact, nil},
    
    {true, "mobileShowEvent", MCHandleShowEvent, nil},                     // ???                      // UI
    {false, "mobileGetEventData", MCHandleGetEventData, nil},               // get calendar data for
    {true, "mobileCreateEvent", MCHandleCreateEvent, nil},                 // create event in calendar // UI
    {true, "mobileUpdateEvent", MCHandleUpdateEvent, nil},                 // edit calendar event      // UI
    {false, "mobileAddEvent", MCHandleAddEvent, nil},                       // create calendar entry
    {false, "mobileGetCalendarsEvent", MCHandleGetCalendarsEvent, nil}, // create reoccurring calendar entry
    {false, "mobileFindEvent", MCHandleFindEvent, nil},                     // get calendar entry
    {false, "mobileRemoveEvent", MCHandleRemoveEvent, nil},
	
	{false, "iphoneClearTouches", MCHandleClearTouches, nil},
	{false, "mobileClearTouches", MCHandleClearTouches, nil},
    
    // MW-2013-05-30: [[ RemoteControl ]] Support for iOS 'remote controls' and metadata display.
    {false, "iphoneEnableRemoteControl", MCHandleEnableRemoteControl, nil},
    {false, "iphoneDisableRemoteControl", MCHandleDisableRemoteControl, nil},
    {false, "iphoneRemoteControlEnabled", MCHandleRemoteControlEnabled, nil},
    {false, "iphoneSetRemoteControlDisplay", MCHandleSetRemoteControlDisplay, nil},
    
	{nil, nil, nil}    
};

bool MCIsPlatformMessage(MCNameRef handler_name)
{
    bool found = false;
    
    for(uint32_t i = 0; s_platform_messages[i] . message != nil; i++)
    {
		if (MCNameIsEqualToCString(handler_name, s_platform_messages[i].message, kMCCompareCaseless))
			found = true;
    }
    
    return found;
}

#ifdef TARGET_SUBPLATFORM_IPHONE
struct handle_context_t
{
	MCPlatformMessageHandler handler;
	void *context;
	MCParameter *parameters;
	Exec_stat result;
};

static void invoke_platform(void *p_context)
{
	handle_context_t *ctxt;
	ctxt = (handle_context_t *)p_context;
	ctxt -> result = ctxt -> handler(ctxt -> context, ctxt -> parameters);
}

extern void MCIPhoneCallOnMainFiber(void (*)(void *), void *);

bool MCDoHandlePlatformMessage(bool p_waitable, MCPlatformMessageHandler p_handler, void *p_context, MCParameter *p_parameters, Exec_stat& r_result)
{
    // MW-2012-07-31: [[ Fibers ]] If the method doesn't need script / wait, then
    //   jump to the main fiber for it.
    if (!p_waitable)
    {
        handle_context_t ctxt;
        ctxt . handler = p_handler;
        ctxt . context = p_context;
        ctxt . parameters = p_parameters;
        MCIPhoneCallOnMainFiber(invoke_platform, &ctxt);
        r_result = ctxt . result;
        return true;
    }
    
    // Execute the method as normal, in this case the method will have to jump
    // to the main fiber to do any system stuff.
    r_result = p_handler(p_context, p_parameters);
    return true;
    
}
#else // Android
bool MCDoHandlePlatformMessage(bool p_waitable, MCPlatformMessageHandler p_handler, void *p_context, MCParameter *p_parameters, Exec_stat& r_result)
{
    r_result = p_handler(p_context, p_parameters);
    return true;
}
#endif

bool MCHandlePlatformMessage(MCNameRef p_message, MCParameter *p_parameters, Exec_stat& r_result)
{
	for(uint32_t i = 0; s_platform_messages[i] . message != nil; i++)
		if (MCNameIsEqualToCString(p_message, s_platform_messages[i] . message, kMCCompareCaseless))
		{
            return MCDoHandlePlatformMessage(s_platform_messages[i] . waitable, s_platform_messages[i] . handler, s_platform_messages[i] . context, p_parameters, r_result);
		}
	
    r_result = ES_NOT_HANDLED;
	return false;
}
