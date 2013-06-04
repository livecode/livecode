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

#include "mblstore.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_GET_METHOD(Store, CanMakePurchase, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Store, EnablePurchaseUpdates, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Store, DisablePurchaseUpdates, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Store, RestorePurchases, 0)
//MC_EXEC_DEFINE_GET_METHOD(Store, PurchaseList, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Store, CreatePurchase, 1)
MC_EXEC_DEFINE_GET_METHOD(Store, PurchaseState, 2)
MC_EXEC_DEFINE_GET_METHOD(Store, PurchaseError, 2)
MC_EXEC_DEFINE_GET_METHOD(Store, PurchaseProperty, 2)
MC_EXEC_DEFINE_SET_METHOD(Store, PurchaseProperty, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Store, SendPurchaseRequest, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Store, ConfirmPurchaseDelivery, 1)
//MC_EXEC_DEFINE_ (Store, RequestProductDetails)


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
    MCStoreRestorePurchases();
}


//void MCStoreGetPurchaseList(MCExecContext& ctxt, MCStringRef& r_list)
//{
//    
//}

void MCStoreExecCreatePurchase(MCExecContext& ctxt, MCStringRef p_product_id, uint32_t& r_id)
{
    MCPurchase *t_purchase = nil;
    bool t_success = true;
    
    t_success = MCPurchaseCreate(MCStringGetCString(p_product_id), nil, t_purchase);
    
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
	MCStringRef t_error = nil;
	MCPurchase *t_purchase = nil;
    
    t_success = MCPurchaseFindById(p_id, t_purchase);
	
	if (t_success)
		t_success = MCPurchaseGetError(t_purchase, t_error);
	
	if (t_success)
    {
        MCStringCopy(t_error, r_error);
        return;
    }
    
    ctxt.Throw();
}

void MCStoreGetPurchaseProperty(MCExecContext& ctxt, int p_id, MCStringRef p_prop_name)
{
	MCPurchase *t_purchase = nil;
	MCPurchaseProperty t_property;
    bool t_success = true;
    
    t_success =
        MCPurchaseFindById(p_id, t_purchase) &&
        MCPurchaseLookupProperty(p_prop_name, t_property);
    
	if (t_success)
		t_success = MCPurchaseGet(t_purchase, t_property, ctxt. GetEP()) == ES_NORMAL;
    
    if (t_success)
        return;
    
    ctxt . Throw();

}

void MCStoreSetPurchaseProperty(MCExecContext& ctxt, int p_id, MCStringRef p_prop_name, uint32_t p_quantity)
{
	MCPurchase *t_purchase = nil;
	MCPurchaseProperty t_property;
    bool t_success = true;
    
    t_success =
        MCPurchaseFindById(p_id, t_purchase) &&
        MCPurchaseLookupProperty(p_prop_name, t_property);
    
	if (t_success)
		t_success = MCPurchaseSet(t_purchase, t_property, p_quantity) == ES_NORMAL;
    
    if (t_success)
        return;
    else
        ctxt.Throw();    
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

void MCStoreExecConfirmPurchaseDelivery(MCExecContext& ctxt, uint32_t p_id)
{
	MCPurchase *t_purchase = nil;
    bool t_success = true;
    
    t_success = MCPurchaseFindById(p_id, t_purchase);
	
	if (t_success)
		t_success = MCPurchaseConfirmDelivery(t_purchase);
    
    if (t_success)
        return;
    else
        ctxt.Throw();
}

//void MCStoreExecRequestProductDetails(MCExecContext& ctxt);
