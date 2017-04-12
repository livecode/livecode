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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


#include "globals.h"
#include "stack.h"
#include "card.h"
#include "eventqueue.h"
#include "exec.h"

#include "param.h"

#include "mblstore.h"

static struct {const char *name; Properties property;} s_purchase_properties[] =
{
	{"productId", P_PRODUCT_IDENTIFIER},
	
	{"quantity", P_PURCHASE_QUANTITY},
	
	{"developerPayload", P_DEVELOPER_PAYLOAD},
	
	{"title", P_LOCALIZED_TITLE},
	{"description", P_LOCALIZED_DESCRIPTION},
	{"price", P_LOCALIZED_PRICE},
	
	{"purchaseDate", P_PURCHASE_DATE},
	
	{"transactionIdentifier", P_TRANSACTION_IDENTIFIER},
	{"receipt", P_RECEIPT},
	{"originalTransactionIdentifier", P_ORIGINAL_TRANSACTION_IDENTIFIER},
	{"originalPurchaseDate", P_ORIGINAL_PURCHASE_DATE},
	{"originalReceipt", P_ORIGINAL_RECEIPT},

	{"signedData", P_SIGNED_DATA},
	{"signature", P_SIGNATURE},
    {"orderId", P_TRANSACTION_IDENTIFIER}, // alias for transactionIdentifier
    {"purchaseTime", P_PURCHASE_DATE}, // alias for purchaseDate
	
	{nil, P_UNDEFINED},
};

static struct {const char *name; MCPurchaseState state;} s_purchase_states[] = 
{
	{"initialized", kMCPurchaseStateInitialized},
	{"sendingRequest", kMCPurchaseStateSendingRequest},
	{"paymentReceived", kMCPurchaseStatePaymentReceived},
	{"complete", kMCPurchaseStateComplete},
	{"restored", kMCPurchaseStateRestored},
	{"cancelled", kMCPurchaseStateCancelled},
    {"alreadyEntitled", kMCPurchaseStateAlreadyEntitled},
    {"invalidSKU", kMCPurchaseStateInvalidSKU},
    {"refunded", kMCPurchaseStateRefunded},
	{"error", kMCPurchaseStateError},
    {"unverified", kMCPurchaseStateUnverified},
};

// we maintain here a list of known pending purchases, and a list of completed purchases
static MCPurchase *s_purchases = nil;
static MCListRef s_completed_purchases = nil;
static uint32_t s_last_purchase_id = 1;
static uint32_t s_id = 0;

////////////////////////////////////////////////////////////////////////////////

// SN-2015-02-24: [[ Merg 6.7.4-rc-1 ]] Add a function to clean the completed
//  purchase list
void MCPurchaseClearPurchaseList()
{
    MCValueRelease(s_completed_purchases);
    s_completed_purchases = nil;
}

////////////////////////////////////////////////////////////////////////////////

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

bool MCPurchaseFindByProdId(MCStringRef p_prod_id, MCPurchase *&r_purchase)
{
    // First look up the purchase by purchase ID (unique). This prevents a crash when restoring auto-renewing subscriptions (iOS), where the same product ID appears multiple times.
    MCPurchase *t_purchase;
    bool t_found_by_id = MCPurchaseFindById(s_id, t_purchase);
    if (t_found_by_id && MCStringIsEqualTo(t_purchase -> prod_id, p_prod_id, kMCStringOptionCompareCaseless))
    {
        r_purchase = t_purchase;
        return true;
    }
    
	for (MCPurchase *t_purchase = MCStoreGetPurchases(); t_purchase != NULL; t_purchase = t_purchase->next)
	{
        if (MCStringIsEqualTo(t_purchase->prod_id, p_prod_id, kMCStringOptionCompareCaseless))
		{
            r_purchase = t_purchase;
			return true;
		}
	}
	return false;
}

bool MCPurchaseLookupProperty(MCStringRef p_property, Properties &r_property)
{
	for (uint32_t i = 0; s_purchase_properties[i].name != nil; i++)
	{
		if (MCStringIsEqualToCString(p_property, s_purchase_properties[i].name, kMCCompareCaseless))
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

bool MCPurchaseList(MCStringRef& r_string)
{
    if (s_completed_purchases != NULL)
        return MCListCopyAsString(s_completed_purchases, r_string);
    else
    {
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    }
}

bool MCPurchaseInit(MCPurchase *p_purchase, MCStringRef p_product_id, void *p_context);
bool MCPurchaseCreate(MCStringRef p_product_id, void *p_context, MCPurchase *&r_purchase)
{
	bool t_success = true;
	
	MCPurchase *t_purchase;
	
	t_success = MCMemoryNew(t_purchase);
	
	if (t_success)
	{
        // PM-2015-01-07: [[ Bug 14343 ]] Nil-check to prevent crash
        t_purchase->prod_id = (p_product_id != nil ? MCValueRetain(p_product_id) : MCValueRetain(kMCEmptyString));
        MCLog("MCPurchaseCreate :purchase->prod_id is : %@", t_purchase->prod_id);
		t_purchase->id = s_last_purchase_id++;
		t_purchase->ref_count = 1;
        MCLog("MCPurchaseCreate : reference count : %d", t_purchase->ref_count);
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
    {
        MCPurchaseDelete(t_purchase);
    }
	
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
    MCValueRelease(p_purchase -> prod_id);
	//MCLog("freeing memory...", nil);
	MCMemoryDelete(p_purchase);
	//MCLog("...done", nil);
}

void MCPurchaseRetain(MCPurchase *p_purchase)
{
	if (p_purchase != NULL)
		p_purchase -> ref_count ++;
    MCLog("MCPurchaseRetain : reference count: %d", p_purchase->ref_count);
}

void MCPurchaseRelease(MCPurchase *p_purchase)
{
	MCLog("MCPurchaseRelease(%p)...", p_purchase);
	if (p_purchase != NULL)
	{
        if (p_purchase -> ref_count > 1)
            p_purchase -> ref_count -= 1;
        else
            MCPurchaseDelete(p_purchase);
	}
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
	MCLog("retaining purchase (%p) until event dispatch", p_purchase);
	MCPurchaseRetain(m_purchase);
	
	m_next = s_pending_events;
	s_pending_events = this;
}

void MCPurchaseUpdateEvent::Destroy()
{
	MCLog("releasing purchase (%p) after event deletion", m_purchase);
	MCPurchaseRelease(m_purchase);
	delete this;
}

/*
     PM : CONSIDER - backward compatibility. purchaseStateUpdate callback was returned with purchaseId and purchaseState.
     Now is returned with purchaseId, productId and purchaseState     
*/
void MCPurchaseUpdateEvent::Dispatch()
{
	bool t_success = true;
	
	MCLog("removing purchase (%p) event from pending list", m_purchase);
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

	MCLog("dispatching purchase (%p) event", m_purchase);

	MCAutoStringRef t_id, t_state, t_prod_id;
	const char *t_state_str = NULL;
	
	t_success = MCPurchaseStateToString(m_purchase->state, t_state_str);
    
    if (t_success)
        t_success = MCStringCreateWithCString(t_state_str, &t_state);
	
	if (t_success)
        t_success = MCStringFormat(&t_id, "%d", m_purchase->id);
    
    // PM-2015-01-07: [[ Bug 14343 ]] m_purchase->prod_id is an MCStringRef
    if (t_success)
        t_success = MCStringFormat(&t_prod_id, "%@", m_purchase->prod_id);
    
    s_id = m_purchase->id;
    
	if (t_success)
		MCdefaultstackptr->getcurcard()->message_with_valueref_args(MCM_purchase_updated, *t_id, *t_prod_id, *t_state);
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
	t_event = new (nothrow) MCPurchaseUpdateEvent(p_purchase);
	MCEventQueuePostCustom(t_event);
}

void MCPurchaseCompleteListUpdate(MCPurchase *p_purchase)
{
    if (s_completed_purchases == NULL)
        /* UNCHECKED */ MCListCreateMutable('\n', s_completed_purchases);

    MCListAppend(s_completed_purchases, p_purchase -> prod_id);
}

