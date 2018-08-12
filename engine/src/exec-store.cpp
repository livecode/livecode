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

#include "mcerror.h"
#include "globals.h"
#include "exec.h"

#include "mblstore.h"

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCStorePurchasePropertyElementInfo[] =
{
    { "product identifier", P_PRODUCT_IDENTIFIER, false},
    { "quantity", P_PURCHASE_QUANTITY, false},
    { "developer payload", P_DEVELOPER_PAYLOAD, false},
    { "localized title", P_LOCALIZED_TITLE, false},
    { "localized description", P_LOCALIZED_DESCRIPTION, false},
    { "localized price", P_LOCALIZED_PRICE, false},
    { "purchase date", P_PURCHASE_DATE, false},
    { "transaction identifier", P_TRANSACTION_IDENTIFIER, false},
    { "receipt", P_RECEIPT, false},
    { "original transaction identifier", P_ORIGINAL_TRANSACTION_IDENTIFIER, false},
    { "original purchase date", P_ORIGINAL_PURCHASE_DATE, false},
    { "original receipt", P_ORIGINAL_RECEIPT, false},
    { "signed data", P_SIGNED_DATA, false},
    { "signature", P_SIGNATURE, false},
    { "unknown", P_UNDEFINED, false}
};

static MCExecEnumTypeInfo _kMCStorePurchasePropertyTypeInfo =
{
    "Store.PurchaseProperty",
    sizeof(_kMCStorePurchasePropertyElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCStorePurchasePropertyElementInfo
};

MCExecEnumTypeInfo* kMCStorePurchasePropertyTypeInfo = &_kMCStorePurchasePropertyTypeInfo;

////////////////////////////////////////////////////////////////////////////////


void MCStoreGetCanMakePurchase(MCExecContext& ctxt, bool& r_result)
{
    r_result = MCStoreCanMakePurchase();
}

void MCStoreExecEnablePurchaseUpdates(MCExecContext& ctxt)
{
    if (!MCStoreEnablePurchaseUpdates())
        ctxt.Throw();
}

void MCStoreExecDisablePurchaseUpdates(MCExecContext& ctxt)
{
    if (!MCStoreDisablePurchaseUpdates())
        ctxt.Throw();
}

void MCStoreExecRestorePurchases(MCExecContext& ctxt)
{
    if (!MCStoreRestorePurchases())
        ctxt.Throw();
}


void MCStoreGetPurchaseList(MCExecContext& ctxt, MCStringRef& r_list)
{
    if (!MCPurchaseList(r_list))
        ctxt.Throw();
}

void MCStoreExecCreatePurchase(MCExecContext& ctxt, MCStringRef p_product_id, uint32_t& r_id)
{
    MCPurchase *t_purchase = nil;
    bool t_success = true;
    
    t_success = MCPurchaseCreate(p_product_id, nil, t_purchase);
    
    if (t_success)
        r_id = t_purchase->id;
    else
        ctxt.Throw();
}

void MCStoreGetPurchaseState(MCExecContext& ctxt, int p_id, MCStringRef& r_state)
{
    const char* t_state = nil;
	MCPurchase *t_purchase = nil;
    bool t_success = true;
    
	t_success = MCPurchaseFindById(p_id, t_purchase);
    
	if (t_success)
		t_success = MCPurchaseStateToString(t_purchase->state, t_state);
	
	if (t_success)
        t_success = MCStringCreateWithCString(t_state, r_state);
    
    if (t_success)
        return;
    
    ctxt.Throw();
}

void MCStoreGetPurchaseError(MCExecContext& ctxt, int p_id, MCStringRef& r_error)
{
    bool t_success = true;
	MCAutoStringRef t_error;
	MCPurchase *t_purchase = nil;
    
    t_success = MCPurchaseFindById(p_id, t_purchase);
	
    // PM-2015-01-19: [[ Bug 14401 ]] Fixed mismerge issue that caused mobileStorePurchaseError to return empty 
    if (t_success)
        t_success = (t_purchase != nil && t_purchase->state == kMCPurchaseStateError);
    
	if (t_success)
		t_success = MCPurchaseGetError(t_purchase, &t_error);
	
	if (t_success)
        if (MCStringCopy(*t_error, r_error))
            return;
    
    ctxt.Throw();
}

MCPropertyInfo *lookup_purchase_property(const MCPurchasePropertyTable *p_table, Properties p_which)
{
	for(uindex_t i = 0; i < p_table -> size; i++)
		if (p_table -> table[i] . property == p_which)
			return &p_table -> table[i];
	
	return nil;
}


void MCStoreExecGet(MCExecContext& ctxt, integer_t p_id, MCStringRef p_prop_name, MCValueRef& r_value)
{
    MCPurchase *t_purchase = nil;
	Properties t_property;
    
    MCPropertyInfo *t_info;
    t_info = nil;
    
    if (MCPurchaseFindById(p_id, t_purchase) && MCPurchaseLookupProperty(p_prop_name, t_property))
        t_info = lookup_purchase_property(getpropertytable(), t_property);
    
	if (t_info != nil)
	{

		MCExecValue t_value;
        MCExecFetchProperty(ctxt, t_info, t_purchase, t_value);
		MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeValueRef, &r_value);
        return;
    }
    
    ctxt .Throw();
}

void MCStoreExecSet(MCExecContext& ctxt, integer_t p_id, MCStringRef p_prop_name, MCValueRef p_value)
{
    MCPurchase *t_purchase = nil;
	Properties t_property;
    
    MCPropertyInfo *t_info;
    t_info = nil;
    
    if (MCPurchaseFindById(p_id, t_purchase) && MCPurchaseLookupProperty(p_prop_name, t_property))
        t_info = lookup_purchase_property(getpropertytable(), t_property);
	
	if (t_info != nil)
	{
		MCExecValue t_value;
		MCExecValueTraits<MCValueRef>::set(t_value, MCValueRetain(p_value));
        MCExecStoreProperty(ctxt, t_info, t_purchase, t_value);
        return;
	}
    
    ctxt . Throw();
}

void MCStoreExecSendPurchaseRequest(MCExecContext& ctxt, uint32_t p_id)
{
	MCPurchase *t_purchase = nil;
    bool t_success = true;
    
    t_success = MCPurchaseFindById(p_id, t_purchase);
	
	if (t_success)
		t_success = MCPurchaseSendRequest(t_purchase);
    
    if (t_success)
        return;
    else
        ctxt.Throw();
}

void MCStoreExecMakePurchase(MCExecContext& ctxt, MCStringRef p_product_id, MCStringRef p_quantity, MCStringRef p_payload)
{
    bool t_success;
	t_success = true;
    
    if (t_success)
		t_success = MCStoreMakePurchase(p_product_id, p_quantity, p_payload);
    
    if (!t_success)
        ctxt . Throw();
}

void MCStoreExecConfirmPurchase(MCExecContext& ctxt, MCStringRef p_product_id)
{
    bool t_success;
	MCPurchase *t_purchase;
    
    t_success = MCPurchaseFindByProdId(p_product_id, t_purchase);
    
    if (t_success)
        t_success = MCPurchaseConfirmDelivery(t_purchase);
    
    if (!t_success)
        ctxt . Throw();
}

void MCStoreExecProductSetType(MCExecContext &ctxt, MCStringRef p_product_id, MCStringRef p_product_type)
{
    if (!MCStoreProductSetType(p_product_id, p_product_type))
        ctxt . Throw();
}


void MCStoreExecConfirmPurchaseDelivery(MCExecContext& ctxt, uint32_t p_id)
{
	MCPurchase *t_purchase = nil;
    bool t_success = true;
    
    t_success = MCPurchaseFindById(p_id, t_purchase);
    
    if (t_success)
        t_success = (t_purchase->state == kMCPurchaseStatePaymentReceived || t_purchase->state == kMCPurchaseStateRefunded || t_purchase->state == kMCPurchaseStateRestored);
	
	if (t_success)
		t_success = MCPurchaseConfirmDelivery(t_purchase);
    
    if (t_success)
        return;
    else
        ctxt.Throw();
}

void MCStoreExecRequestProductDetails(MCExecContext& ctxt, MCStringRef p_product_id)
{
    if (MCStoreRequestProductDetails(p_product_id))
        return;
    
    ctxt.Throw();
}

void MCStoreExecPurchaseVerify(MCExecContext& ctxt, uint32_t p_id, bool p_verified)
{
    MCPurchase *t_purchase = nil;
    bool t_success = true;    
    
    t_success = MCPurchaseFindById(p_id, t_purchase);
    
    if (t_success)
        MCPurchaseVerify(t_purchase, p_verified);

    if (t_success)
        return;
    
    ctxt.Throw();
}

void MCStoreExecReceiveProductDetails(MCExecContext &ctxt, MCStringRef p_product_id, MCStringRef &r_result)
{
    MCAutoStringRef t_result;
    if (MCStoreReceiveProductDetails(p_product_id, &t_result))
    {
        r_result = MCValueRetain(*t_result);
        return;
    }
    
    ctxt.Throw();
}

void MCStoreExecConsumePurchase(MCExecContext &ctxt, MCStringRef p_product_id)
{
    if (MCStoreConsumePurchase(p_product_id))
        return;
    
    ctxt.Throw();
}

