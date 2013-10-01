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
MC_EXEC_DEFINE_GET_METHOD(Store, PurchaseList, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Store, CreatePurchase, 1)
MC_EXEC_DEFINE_GET_METHOD(Store, PurchaseState, 2)
MC_EXEC_DEFINE_GET_METHOD(Store, PurchaseError, 2)
MC_EXEC_DEFINE_GET_METHOD(Store, PurchaseProperty, 2)
MC_EXEC_DEFINE_SET_METHOD(Store, PurchaseProperty, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Store, SendPurchaseRequest, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Store, ConfirmPurchaseDelivery, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Store, RequestProductDetails, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Store, PurchaseVerify, 2)


////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCStorePurchasePropertyElementInfo[] =
{
    { "product identifier", kMCPurchasePropertyProductIdentifier},
    { "quantity", kMCPurchasePropertyQuantity},
    { "developer payload", kMCPurchasePropertyDeveloperPayload},
    { "localized title", kMCPurchasePropertyLocalizedTitle},
    { "localized description", kMCPurchasePropertyLocalizedDescription},
    { "localized price", kMCPurchasePropertyLocalizedPrice},
    { "purchase date", kMCPurchasePropertyPurchaseDate},
    { "transaction identifier", kMCPurchasePropertyTransactionIdentifier},
    { "receipt", kMCPurchasePropertyReceipt},
    { "original transaction identifier", kMCPurchasePropertyOriginalTransactionIdentifier},
    { "original purchase date", kMCPurchasePropertyOriginalPurchaseDate},
    { "original receipt", kMCPurchasePropertyOriginalReceipt},
    { "signed data", kMCPurchasePropertySignedData},
    { "signature", kMCPurchasePropertySignature},
    { "unknown", kMCPurchasePropertyUnknown}
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
    if (!MCPurchaseList(ctxt.GetEP()))
        ctxt.Throw();
    else
    {
        /* UNCHECKED */ ctxt.CopyAsStringRef(r_list);
    }
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
	
    if (t_success)
        t_success = (t_purchase == nil || t_purchase->state != kMCPurchaseStateError);
    
	if (t_success)
		t_success = MCPurchaseGetError(t_purchase, &t_error);
	
	if (t_success)
        if (MCStringCopy(*t_error, r_error))
            return;
    
    ctxt.Throw();
}

MCPurchasePropertyInfo *lookup_purchase_property(const MCPurchasePropertyTable *p_table, MCPurchaseProperty p_which)
{
	for(uindex_t i = 0; i < p_table -> size; i++)
		if (p_table -> table[i] . property == p_which)
			return &p_table -> table[i];
	
	return nil;
}


void MCStoreExecGet(MCExecContext& ctxt, integer_t p_id, MCStringRef p_prop_name, MCValueRef& r_value)
{
    MCPurchase *t_purchase = nil;
	MCPurchaseProperty t_property;
    
    MCPurchaseFindById(p_id, t_purchase) &&
    MCPurchaseLookupProperty(p_prop_name, t_property);
    
    MCPurchasePropertyInfo *t_info;
    t_info = lookup_purchase_property(getpropertytable(), t_property);
    
	if (t_info != nil)
	{
		switch(t_info -> type)
		{
			case kMCPropertyTypeAny:
			{
				MCAutoValueRef t_any;
				if (((bool(*)(MCPurchase*, MCValueRef&))t_info -> getter)(t_purchase, &t_any))
					r_value = MCValueRetain(*t_any);
			}
                break;
				
			case kMCPropertyTypeBool:
			{
				bool t_value;
				if (((bool(*)(MCPurchase*, bool&))t_info -> getter)(t_purchase, t_value))
					r_value = MCValueRetain(t_value ? kMCTrue : kMCFalse);
			}
                break;
                
			case kMCPropertyTypeInt16:
			case kMCPropertyTypeInt32:
			{
				integer_t t_value;
				if (((bool(*)(MCPurchase*, integer_t&))t_info -> getter)(t_purchase, t_value))
					MCNumberCreateWithInteger(t_value, (MCNumberRef&)r_value);
			}
                break;
                
			case kMCPropertyTypeUInt16:
			case kMCPropertyTypeUInt32:
			{
				uinteger_t t_value;
			if (((bool(*)(MCPurchase*, uinteger_t&))t_info -> getter)(t_purchase, t_value))
					MCNumberCreateWithUnsignedInteger(t_value, (MCNumberRef&)r_value);
			}
                break;
				
			case kMCPropertyTypeDouble:
			{
				double t_value;
				if (((bool(*)(MCPurchase*, double&))t_info -> getter)(t_purchase, t_value))
					MCNumberCreateWithReal(t_value, (MCNumberRef&)r_value);
			}
                break;
                
			case kMCPropertyTypeChar:
			{
				char_t t_value;
				if (((bool(*)(MCPurchase*, char_t&))t_info -> getter)(t_purchase, t_value))
					MCStringCreateWithNativeChars(&t_value, 1, (MCStringRef&)r_value);
			}
                break;
				
			case kMCPropertyTypeString:
			case kMCPropertyTypeBinaryString:
			{
				MCAutoStringRef t_value;
				if (((bool(*)(MCPurchase*, MCStringRef&))t_info -> getter)(t_purchase, &t_value))
					r_value = MCValueRetain(*t_value);
			}
                break;
				
			default:
				MCAssert(false);
				break;
		}
	}
}

void MCStoreExecSet(MCExecContext& ctxt, integer_t p_id, MCStringRef p_prop_name, MCValueRef p_value)
{
    MCPurchase *t_purchase = nil;
	MCPurchaseProperty t_property;
    
    MCPurchaseFindById(p_id, t_purchase) &&
    MCPurchaseLookupProperty(p_prop_name, t_property);
    
    MCPurchasePropertyInfo *t_info;
    t_info = lookup_purchase_property(getpropertytable(), t_property);
	
	if (t_info != nil)
	{
		switch(t_info -> type)
		{
			case kMCPropertyTypeAny:
			{
				((void(*)(MCPurchase*, MCValueRef))t_info -> setter)(t_purchase, p_value);
			}
                break;
				
			case kMCPropertyTypeBool:
			{
				MCAutoBooleanRef t_value;
				if (ctxt.ConvertToBoolean(p_value, &t_value))
					((void(*)(MCPurchase*, bool))t_info -> setter)(t_purchase, MCValueIsEqualTo(*t_value, kMCTrue));
				else
					ctxt.Throw();
			}
                break;
                
			case kMCPropertyTypeInt16:
			{
				integer_t t_value;
				if (!ctxt.ConvertToInteger(p_value, t_value)
					 || t_value < -32768 || t_value > 32767)
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCPurchase*, integer_t))t_info -> setter)(t_purchase, t_value);
			}
                break;
				
			case kMCPropertyTypeInt32:
			{
				integer_t t_value;
				if (!ctxt.ConvertToInteger(p_value, t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCPurchase*, integer_t))t_info -> setter)(t_purchase, t_value);
			}
                break;
                
			case kMCPropertyTypeUInt16:
			{
				uinteger_t t_value;
				if (!ctxt.ConvertToUnsignedInteger(p_value, t_value)
					|| t_value > 65535)
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCPurchase*, uinteger_t))t_info -> setter)(t_purchase, t_value);
			}
                break;
				
			case kMCPropertyTypeUInt32:
			{
				uinteger_t t_value;
				if (!ctxt.ConvertToUnsignedInteger(p_value, t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCPurchase*, uinteger_t))t_info -> setter)(t_purchase, t_value);
			}
                break;
				
			case kMCPropertyTypeDouble:
			{
				double t_value;
				if (!ctxt.ConvertToReal(p_value, t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCPurchase*, double))t_info -> setter)(t_purchase, t_value);
			}
                break;
                
			case kMCPropertyTypeChar:
			{
				MCAutoStringRef t_value;
				if (ctxt.ConvertToString(p_value, &t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAC);
				if (!ctxt . HasError())
					((void(*)(MCPurchase*, char_t))t_info -> setter)(t_purchase, MCStringGetNativeCharAtIndex(*t_value, 0));
			}
                break;
				
			case kMCPropertyTypeString:
			case kMCPropertyTypeBinaryString:
			{
				MCAutoStringRef t_value;
				if (!ctxt.ConvertToString(p_value, &t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAC);
				if (!ctxt . HasError())
					((void(*)(MCPurchase*, MCStringRef))t_info -> setter)(t_purchase, *t_value);
			}
                break;
				
			default:
				ctxt . Unimplemented();
				break;
		}
	}
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
        t_success = (!(t_purchase->state == kMCPurchaseStatePaymentReceived || t_purchase->state == kMCPurchaseStateRefunded || t_purchase->state == kMCPurchaseStateRestored));
	
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
