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

#include "execpt.h"
#include "globals.h"
#include "stack.h"
#include "card.h"
#include "eventqueue.h"

#include "core.h"

#include "param.h"

#include "mblstore.h"

static struct {const char *name; MCPurchaseProperty property;} s_purchase_properties[] = 
{
	{"productId", kMCPurchasePropertyProductIdentifier},
	
	{"quantity", kMCPurchasePropertyQuantity},
	
	{"developerPayload", kMCPurchasePropertyDeveloperPayload},
	
	{"title", kMCPurchasePropertyLocalizedTitle},
	{"description", kMCPurchasePropertyLocalizedDescription},
	{"price", kMCPurchasePropertyLocalizedPrice},
	
	{"purchaseDate", kMCPurchasePropertyPurchaseDate},
	
	{"transactionIdentifier", kMCPurchasePropertyTransactionIdentifier},
	{"receipt", kMCPurchasePropertyReceipt},
	{"originalTransactionIdentifier", kMCPurchasePropertyOriginalTransactionIdentifier},
	{"originalPurchaseDate", kMCPurchasePropertyOriginalPurchaseDate},
	{"originalReceipt", kMCPurchasePropertyOriginalReceipt},

	{"signedData", kMCPurchasePropertySignedData},
	{"signature", kMCPurchasePropertySignature},
    {"orderId", kMCPurchasePropertyTransactionIdentifier}, // alias for transactionIdentifier
    {"purchaseTime", kMCPurchasePropertyPurchaseDate}, // alias for purchaseDate
	
	{nil, kMCPurchasePropertyUnknown},
};

static struct {const char *name; MCPurchaseState state;} s_purchase_states[] = 
{
	{"initialized", kMCPurchaseStateInitialized},
	{"sendingRequest", kMCPurchaseStateSendingRequest},
	{"paymentReceived", kMCPurchaseStatePaymentReceived},
	{"complete", kMCPurchaseStateComplete},
	{"restored", kMCPurchaseStateRestored},
	{"cancelled", kMCPurchaseStateCancelled},
    {"refunded", kMCPurchaseStateRefunded},
	{"error", kMCPurchaseStateError},
    {"unverified", kMCPurchaseStateUnverified},
};

// we maintain here a list of known pending purchases
static MCPurchase *s_purchases = nil;
static uint32_t s_last_purchase_id = 1;

bool MCPurchaseFindById(uint32_t p_id, MCPurchase *&r_purchase)
{
	for (MCPurchase *t_purchase = MCStoreGetPurchases(); t_purchase != NULL; t_purchase = t_purchase->next)
	{
		if (t_purchase->id == p_id)
		{
			r_purchase = t_purchase;
			return true;
		}
	}
	return false;
}

bool MCPurchaseLookupProperty(const char *p_property, MCPurchaseProperty &r_property)
{
	for (uint32_t i = 0; s_purchase_properties[i].name != nil; i++)
	{
		if (MCCStringEqualCaseless(p_property, s_purchase_properties[i].name))
		{
			r_property = s_purchase_properties[i].property;
			return true;
		}
	}
	return false;
}

bool MCPurchaseStateToString(MCPurchaseState p_state, const char *&r_string)
{
	for (uint32_t i = 0; s_purchase_states[i].name != nil; i++)
	{
		if (p_state == s_purchase_states[i].state)
		{
			r_string = s_purchase_states[i].name;
			return true;
		}
	}
	return false;
}

bool MCPurchaseList(MCPurchaseListCallback p_callback, void *p_context)
{
	for (MCPurchase *t_purchase = MCStoreGetPurchases(); t_purchase != NULL; t_purchase = t_purchase->next)
		if (!p_callback(p_context, t_purchase))
			return false;
	
	return true;
}

bool MCPurchaseInit(MCPurchase *p_purchase, const char *p_product_id, void *p_context);
bool MCPurchaseCreate(const char *p_product_id, void *p_context, MCPurchase *&r_purchase)
{
	bool t_success = true;
	
	MCPurchase *t_purchase;
	
	t_success = MCMemoryNew(t_purchase);
	
	if (t_success)
	{
		t_purchase->id = s_last_purchase_id++;
		t_purchase->ref_count = 1;
		t_purchase->state = kMCPurchaseStateInitialized;
		
		t_success = MCPurchaseInit(t_purchase, p_product_id, p_context);
	}
	
	if (t_success)
	{
		t_purchase->next = s_purchases;
		s_purchases = t_purchase;
		
		r_purchase = t_purchase;
	}
	else
		MCMemoryDelete(t_purchase);
	
	return t_success;
}

void MCPurchaseFinalize(MCPurchase *);
void MCPurchaseDelete(MCPurchase *p_purchase)
{
	MCLog("MCPurchaseDelete(%p)...", p_purchase);
	if (p_purchase == NULL)
		return;
	
	if (s_purchases == p_purchase)
		s_purchases = p_purchase -> next;
	else
	{
		for (MCPurchase *t_purchase = s_purchases; t_purchase != NULL; t_purchase = t_purchase -> next)
		{
			if (t_purchase -> next == p_purchase)
			{
				t_purchase -> next = p_purchase -> next;
				break;
			}
		}
	}
	
	//MCLog("finalizing...", nil);
	MCPurchaseFinalize(p_purchase);
	//MCLog("freeing memory...", nil);
	MCMemoryDelete(p_purchase);
	//MCLog("...done", nil);
}

void MCPurchaseRetain(MCPurchase *p_purchase)
{
	if (p_purchase != NULL)
		p_purchase -> ref_count ++;
}

void MCPurchaseRelease(MCPurchase *p_purchase)
{
	MCLog("MCPurchaseRelease(%p)...", p_purchase);
	if (p_purchase != NULL)
	{
		MCLog("reference count: %d", p_purchase->ref_count);
		if (p_purchase -> ref_count > 1)
			p_purchase -> ref_count -= 1;
		else
			MCPurchaseDelete(p_purchase);
	}
	//MCLog("...done", nil);
}

MCPurchase *MCStoreGetPurchases()
{
	return s_purchases;
}

////////////////////////////////////////////////////////////////////////////////

class MCPurchaseUpdateEvent : public MCCustomEvent
{
public:
	MCPurchaseUpdateEvent(MCPurchase *p_purchase);
	
	void Destroy();
	void Dispatch();
	
	static bool EventPendingForPurchase(MCPurchase *p_purchase);
private:
	MCPurchase *m_purchase;
	MCPurchaseUpdateEvent *m_next;

	static MCPurchaseUpdateEvent *s_pending_events;
};

MCPurchaseUpdateEvent *MCPurchaseUpdateEvent::s_pending_events = NULL;

MCPurchaseUpdateEvent::MCPurchaseUpdateEvent(MCPurchase *p_purchase)
{
	m_purchase = p_purchase;
	//MCLog("retaining purchase (%p) until event dispatch", p_purchase);
	MCPurchaseRetain(m_purchase);
	
	m_next = s_pending_events;
	s_pending_events = this;
}

void MCPurchaseUpdateEvent::Destroy()
{
	//MCLog("releasing purchase (%p) after event deletion", m_purchase);
	MCPurchaseRelease(m_purchase);
	delete this;
}

void MCPurchaseUpdateEvent::Dispatch()
{
	bool t_success = true;
	
	//MCLog("removing purchase (%p) event from pending list", m_purchase);
	if (s_pending_events == this)
		s_pending_events = m_next;
	else
	{
		for (MCPurchaseUpdateEvent* t_event = s_pending_events; t_event != NULL; t_event = t_event->m_next)
		{
			if (t_event->m_next == this)
			{
				t_event->m_next = m_next;
				break;
			}
		}
	}

	//MCLog("dispatching purchase (%p) event", m_purchase);

	char *t_id = NULL;
	const char *t_state = NULL;
	
	t_success = MCPurchaseStateToString(m_purchase->state, t_state);
	
	if (t_success)
		t_success = MCCStringFormat(t_id, "%d", m_purchase->id);
	
	if (t_success)
		MCdefaultstackptr->getcurcard()->message_with_args(MCM_purchase_updated, t_id, t_state);
	
	MCCStringFree(t_id);
}

bool MCPurchaseUpdateEvent::EventPendingForPurchase(MCPurchase *p_purchase)
{
	for (MCPurchaseUpdateEvent *i = s_pending_events; i != NULL; i = i->m_next)
		if (i->m_purchase == p_purchase)
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCPurchaseNotifyUpdate(MCPurchase *p_purchase)
{
    //MCLog("MCPurchaseNotifyUpdate(%p)", p_purchase);
	if (MCPurchaseUpdateEvent::EventPendingForPurchase(p_purchase))
		return;
	
    //MCLog("posting new update event", p_purchase);
	MCCustomEvent *t_event;
	t_event = new MCPurchaseUpdateEvent(p_purchase);
	MCEventQueuePostCustom(t_event);
}


////////////////////////////////////////////////////////////////////////////////


#include "param.h"

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);


Exec_stat MCHandleCanMakePurchase(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCanMakePurchase */ LEGACY_EXEC
	MCresult -> sets(MCU_btos( MCStoreCanMakePurchase() ));
	return ES_NORMAL;
#endif /* MCHandleCanMakePurchase */
}

Exec_stat MCHandleEnablePurchaseUpdates(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleEnablePurchaseUpdates */ LEGACY_EXEC
	MCStoreEnablePurchaseUpdates();
	return ES_NORMAL;
#endif /* MCHandleEnablePurchaseUpdates */
}

Exec_stat MCHandleDisablePurchaseUpdates(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleDisablePurchaseUpdates */ LEGACY_EXEC
	MCStoreDisablePurchaseUpdates();
	return ES_NORMAL;
#endif /* MCHandleDisablePurchaseUpdates */
}

Exec_stat MCHandleRestorePurchases(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleRestorePurchases */ LEGACY_EXEC
	MCStoreRestorePurchases();
	return ES_NORMAL;
#endif /* MCHandleRestorePurchases */
}

static bool list_purchases(void *context, MCPurchase* p_purchase)
{
	MCExecPoint *ep;
	ep = (MCExecPoint *)context;
	
	ep -> concatuint(p_purchase -> id, EC_RETURN, ep -> isempty());
	
	return true;
}

Exec_stat MCHandlePurchaseList(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePurchaseList */ LEGACY_EXEC
	MCExecPoint ep(nil, nil, nil);
	MCPurchaseList(list_purchases, &ep);
	MCresult -> store(ep, False);
	return ES_NORMAL;
#endif /* MCHandlePurchaseList */
}

Exec_stat MCHandlePurchaseCreate(void *context, MCParameter *p_parameters)
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
}

Exec_stat MCHandlePurchaseState(void *context, MCParameter *p_parameters)
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
}

Exec_stat MCHandlePurchaseError(void *context, MCParameter *p_parameters)
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
}
