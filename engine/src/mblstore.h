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

enum MCPurchaseState
{
	kMCPurchaseStateInitialized,
	kMCPurchaseStateSendingRequest,
	kMCPurchaseStatePaymentReceived,
	kMCPurchaseStateComplete,
	kMCPurchaseStateRestored,
	kMCPurchaseStateCancelled,
    //Amazon
    kMCPurchaseStateInvalidSKU,
    kMCPurchaseStateAlreadyEntitled,
	kMCPurchaseStateRefunded,
	kMCPurchaseStateError,
    kMCPurchaseStateUnverified,
	
	kMCPurchaseStateUnknown,
};

typedef struct _mcpurchase_t
{
    const char *              prod_id;
	uint32_t			id;
	MCPurchaseState		state;
	uint32_t			ref_count;
	
	void *				platform_data;
	
	struct _mcpurchase_t *	next;
} MCPurchase;

typedef bool (*MCPurchaseListCallback)(void *context, MCPurchase *purchase);

Exec_stat MCPurchaseSet(MCPurchase *p_purchase, MCPurchaseProperty p_property, MCExecPoint &ep);
Exec_stat MCPurchaseGet(MCPurchase *p_purchase, MCPurchaseProperty p_property, MCExecPoint &ep);

bool MCPurchaseLookupProperty(const char *p_property, MCPurchaseProperty &r_property);

bool MCPurchaseFindById(uint32_t p_id, MCPurchase *&r_purchase);
bool MCPurchaseFindByProdId(const char *p_prod_id, MCPurchase *&r_purchase);
bool MCPurchaseList(MCPurchaseListCallback p_callback, void *p_context);

bool MCPurchaseCreate(const char *p_product_id, void *p_context, MCPurchase *&r_purchase);

void MCPurchaseRetain(MCPurchase *p_purchase);
void MCPurchaseRelease(MCPurchase *p_purchase);
void MCPurchaseCompleteListUpdate(MCPurchase *p_purchase);

bool MCPurchaseSendRequest(MCPurchase *p_purchase);
bool MCPurchaseConfirmDelivery(MCPurchase *p_purchase);

bool MCStoreCanMakePurchase();

bool MCStoreEnablePurchaseUpdates();
bool MCStoreDisablePurchaseUpdates();
bool MCStoreProductSetType(const char *p_purchase_id, const char *p_product_type);
bool MCStoreSetPurchaseProperty(const char *p_purchase_id, const char *p_property_name, const char *p_property_value);
char* MCStoreGetPurchaseProperty(const char *p_purchase_id, const char *p_property_name);
char* MCStoreGetPurchaseList();
bool MCStoreConsumePurchase(const char *p_product_id);
bool MCStoreMakePurchase(const char *p_product_id, const char *p_quantity, const char *p_payload);
bool MCStoreRequestProductDetails(const char *p_product_id);

bool MCStoreRestorePurchases();

MCPurchase *MCStoreGetPurchases();

bool MCPurchaseStateToString(MCPurchaseState p_state, const char *&r_string);
bool MCPurchaseGetError(MCPurchase *p_purchase, char *&r_error);

void MCPurchaseNotifyUpdate(MCPurchase *p_purchase);

#endif //MBLSTORE_H