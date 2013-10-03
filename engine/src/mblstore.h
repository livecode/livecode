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

#ifndef MBLSTORE_H
#define MBLSTORE_H

#include "exec.h"

enum MCPurchaseProperty
{
	kMCPurchasePropertyProductIdentifier,

	// purchase request properties
	// iOS
	kMCPurchasePropertyQuantity,
	// Android
	kMCPurchasePropertyDeveloperPayload,

	// product properties from app store
	// iOS
	kMCPurchasePropertyLocalizedTitle,
	kMCPurchasePropertyLocalizedDescription,
	kMCPurchasePropertyLocalizedPrice,

	// response properties
	kMCPurchasePropertyPurchaseDate,
	// iOS
	kMCPurchasePropertyTransactionIdentifier,
	kMCPurchasePropertyReceipt,
	kMCPurchasePropertyOriginalTransactionIdentifier,
	kMCPurchasePropertyOriginalPurchaseDate,
	kMCPurchasePropertyOriginalReceipt,
	// Android
	kMCPurchasePropertySignedData,
	kMCPurchasePropertySignature,
    
	kMCPurchasePropertyUnknown,
};

struct MCPurchasePropertyInfo
{
	MCPurchaseProperty property;
	bool effective : 1;
	MCPropertyType type;
	void *type_info;
	void *getter;
	void *setter;
	bool has_effective : 1;
};

enum MCPurchaseState
{
	kMCPurchaseStateInitialized,
	kMCPurchaseStateSendingRequest,
	kMCPurchaseStatePaymentReceived,
	kMCPurchaseStateComplete,
	kMCPurchaseStateRestored,
	kMCPurchaseStateCancelled,
	kMCPurchaseStateRefunded,
	kMCPurchaseStateError,
    kMCPurchaseStateUnverified,
	
	kMCPurchaseStateUnknown,
};

typedef struct _mcpurchase_t
{
	uint32_t			id;
	MCPurchaseState		state;
	uint32_t			ref_count;
	
	void *				platform_data;
	
	struct _mcpurchase_t *	next;
} MCPurchase;

typedef bool (*MCPurchaseListCallback)(void *context, MCPurchase *purchase);

struct MCPurchasePropertyTable
{
	uindex_t size;
	MCPurchasePropertyInfo *table;
};

const MCPurchasePropertyTable *getpropertytable(void);

///////////////////////////////////////////////////////////////////////////////////////////////

//// No longer needed
//Exec_stat MCPurchaseSet(MCPurchase *p_purchase, MCPurchaseProperty p_property, uint32_t p_quantity);
//Exec_stat MCPurchaseGet(MCPurchase *p_purchase, MCPurchaseProperty p_property, MCExecPoint &ep);

void MCStoreExecGet(MCExecContext& ctxt, int p_id, MCStringRef p_prop_name, MCValueRef& r_value);
void MCStoreExecSet(MCExecContext& ctxt, integer_t p_id, MCStringRef p_prop_name, MCValueRef r_value);

///////////////////////////////////////////////////////////////////////////////////////////////

//// No longer needed
//Exec_stat MCPurchaseGetProductIdentifier(MCPurchase *p_purchase, MCExecPoint &ep);
//
//// purchase request properties
//// iOS
//Exec_stat MCPurchaseGetQuantity(MCPurchase *p_purchase, MCExecPoint &ep);
//// Android
//Exec_stat MCPurchaseGetDeveloperPayload(MCPurchase *p_purchase, MCExecPoint &ep);
//
//// product properties from app store
//// iOS
//Exec_stat MCPurchaseGetLocalizedTitle(MCPurchase *p_purchase, MCExecPoint &ep);
//Exec_stat MCPurchaseGetLocalizedDescription(MCPurchase *p_purchase, MCExecPoint &ep);
//Exec_stat MCPurchaseGetLocalizedPrice(MCPurchase *p_purchase, MCExecPoint &ep);
//
//// response properties
//Exec_stat MCPurchaseGetPurchaseDate(MCPurchase *p_purchase, MCExecPoint &ep);
//// iOS
//Exec_stat MCPurchaseGetTransactionIdentifier(MCPurchase *p_purchase, MCExecPoint &ep);
//Exec_stat MCPurchaseGetReceipt(MCPurchase *p_purchase, MCExecPoint &ep);
//Exec_stat MCPurchaseGetOriginalTransactionIdentifier(MCPurchase *p_purchase, MCExecPoint &ep);
//Exec_stat MCPurchaseGetOriginalPurchaseDate(MCPurchase *p_purchase, MCExecPoint &ep);
//Exec_stat MCPurchaseGetOriginalReceipt(MCPurchase *p_purchase, MCExecPoint &ep);
//// Android
//Exec_stat MCPurchaseGetSignedData(MCPurchase *p_purchase, MCExecPoint &ep);
//Exec_stat MCPurchaseGetSignature(MCPurchase *p_purchase, MCExecPoint &ep);
//
//Exec_stat MCPurchaseGetUnknown(MCPurchase *p_purchase, MCExecPoint &ep);

///////////////////////////////////////////////////////////////////////////////////////////////

//// No longer needed
//Exec_stat MCPurchaseSetProductIdentifier(MCPurchase *p_purchase, uint32_t p_quantity);
//
//// purchase request properties
//// iOS
//Exec_stat MCPurchaseSetQuantity(MCPurchase *p_purchase, uint32_t p_quantity);
//// Android
//Exec_stat MCPurchaseSetDeveloperPayload(MCPurchase *p_purchase, uint32_t p_quantity);
//
//// product properties from app store
//// iOS
//Exec_stat MCPurchaseSetLocalizedTitle(MCPurchase *p_purchase, uint32_t p_quantity);
//Exec_stat MCPurchaseSetLocalizedDescription(MCPurchase *p_purchase, uint32_t p_quantity);
//Exec_stat MCPurchaseSetLocalizedPrice(MCPurchase *p_purchase, uint32_t p_quantity);
//
//// response properties
//Exec_stat MCPurchaseSetPurchaseDate(MCPurchase *p_purchase, uint32_t p_quantity);
//// iOS
//Exec_stat MCPurchaseSetTransactionIdentifier(MCPurchase *p_purchase, uint32_t p_quantity);
//Exec_stat MCPurchaseSetReceipt(MCPurchase *p_purchase, uint32_t p_quantity);
//Exec_stat MCPurchaseSetOriginalTransactionIdentifier(MCPurchase *p_purchase, uint32_t p_quantity);
//Exec_stat MCPurchaseSetOriginalPurchaseDate(MCPurchase *p_purchase, uint32_t p_quantity);
//Exec_stat MCPurchaseSetOriginalReceipt(MCPurchase *p_purchase, uint32_t p_quantity);
//// Android
//Exec_stat MCPurchaseSetSignedData(MCPurchase *p_purchase, uint32_t p_quantity);
//Exec_stat MCPurchaseSetSignature(MCPurchase *p_purchase, uint32_t p_quantity);
//
//Exec_stat MCPurchaseSetUnknown(MCPurchase *p_purchase, uint32_t p_quantity);

///////////////////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseLookupProperty(MCStringRef p_property, MCPurchaseProperty &r_property);

bool MCPurchaseFindById(uint32_t p_id, MCPurchase *&r_purchase);
bool MCPurchaseList(MCStringRef& r_string);

bool MCPurchaseCreate(MCStringRef p_product_id, void *p_context, MCPurchase *&r_purchase);

void MCPurchaseRetain(MCPurchase *p_purchase);
void MCPurchaseRelease(MCPurchase *p_purchase);

bool MCPurchaseSendRequest(MCPurchase *p_purchase);
bool MCPurchaseConfirmDelivery(MCPurchase *p_purchase);

void MCPurchaseVerify(MCPurchase *p_purchase, bool p_verified);

bool MCStoreCanMakePurchase();

bool MCStoreEnablePurchaseUpdates();
bool MCStoreDisablePurchaseUpdates();

bool MCStoreRestorePurchases();

MCPurchase *MCStoreGetPurchases();

bool MCPurchaseStateToString(MCPurchaseState p_state, const char*& r_string);
bool MCPurchaseGetError(MCPurchase *p_purchase, MCStringRef &r_error);

bool MCStoreRequestProductDetails(MCStringRef p_product_id);

void MCPurchaseNotifyUpdate(MCPurchase *p_purchase);

#endif //MBLSTORE_H