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
#include "parsedef.h"

#include "mcerror.h"

#include "util.h"
#include "globals.h"

#include "objdefs.h"
#include "stack.h"
#include "card.h"
#include "param.h"
#include "exec.h"

#include "eventqueue.h"

#include "mblstore.h"

#include <StoreKit/StoreKit.h>

///////////////////////////////////////////////////////////////////////////////////////////////

void MCPurchaseGetProductIdentifier(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_productIdentifier);
void MCPurchaseGetQuantity(MCExecContext& ctxt,MCPurchase *p_purchase, uinteger_t& r_quantity);
void MCPurchaseGetPurchaseDate(MCExecContext& ctxt,MCPurchase *p_purchase, integer_t& r_date);
void MCPurchaseGetTransactionIdentifier(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_identifier);
void MCPurchaseGetReceipt(MCExecContext& ctxt,MCPurchase *p_purchase, MCDataRef& r_receipt);
void MCPurchaseGetOriginalTransactionIdentifier(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_identifier);
void MCPurchaseGetOriginalPurchaseDate(MCExecContext& ctxt,MCPurchase *p_purchase, integer_t& r_date);
void MCPurchaseGetOriginalReceipt(MCExecContext& ctxt,MCPurchase *p_purchase, MCDataRef& r_receipt);

///////////////////////////////////////////////////////////////////////////////////////////////

void MCPurchaseSetQuantity(MCExecContext& ctxt, MCPurchase *p_purchase, uinteger_t p_quantity);

///////////////////////////////////////////////////////////////////////////////////////////////

static MCPropertyInfo kProperties[] =
{
    DEFINE_RO_STORE_PROPERTY(P_PRODUCT_IDENTIFIER, String, Purchase, ProductIdentifier)
    DEFINE_RW_STORE_PROPERTY(P_PURCHASE_QUANTITY, UInt32, Purchase, Quantity)
    DEFINE_RO_STORE_PROPERTY(P_PURCHASE_DATE, Int32, Purchase, PurchaseDate)
    DEFINE_RO_STORE_PROPERTY(P_TRANSACTION_IDENTIFIER, String, Purchase, TransactionIdentifier)
    DEFINE_RO_STORE_PROPERTY(P_RECEIPT, BinaryString, Purchase, Receipt)
    DEFINE_RO_STORE_PROPERTY(P_ORIGINAL_PURCHASE_DATE, Int32, Purchase, OriginalPurchaseDate)
    DEFINE_RO_STORE_PROPERTY(P_ORIGINAL_TRANSACTION_IDENTIFIER, String, Purchase, OriginalTransactionIdentifier)
    DEFINE_RO_STORE_PROPERTY(P_ORIGINAL_RECEIPT, BinaryString, Purchase, OriginalReceipt)
};

static MCPurchasePropertyTable kPropertyTable =
{
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

const MCPurchasePropertyTable *getpropertytable()
{
    return &kPropertyTable;
}

static MCPurchase *s_purchase_request = nil;
static bool s_did_restore = false;

typedef struct
{
	SKMutablePayment *payment;
	MCStringRef product_id;
	SKPaymentTransaction *transaction;
	MCStringRef error;
} MCiOSPurchase;

bool MCPurchaseFindByTransaction(SKPaymentTransaction *p_transaction, MCPurchase *&r_purchase)
{
	for (MCPurchase *t_purchase = MCStoreGetPurchases(); t_purchase != nil; t_purchase = t_purchase->next)
	{
		MCiOSPurchase *t_ios_data = (MCiOSPurchase*)t_purchase->platform_data;
		
		if (t_ios_data->transaction == p_transaction)
		{
			r_purchase = t_purchase;
			return true;
		}
	}
	
	return false;
}

bool MCPurchaseInit(MCPurchase *p_purchase, MCStringRef p_product_id, void *p_context)
{
	bool t_success = true;

	if (p_context != NULL)
	{
		p_purchase -> platform_data = (MCiOSPurchase*)p_context;
	}
	else
	{
		MCiOSPurchase *t_ios_data = nil;

		t_success = !MCStringIsEmpty(p_product_id);
		
		if (t_success)
			t_success = MCMemoryNew(t_ios_data);
		
		if (t_success)
        {
			t_ios_data->product_id = MCValueRetain(p_product_id);
			t_success = nil != (t_ios_data->payment = [SKMutablePayment paymentWithProductIdentifier: MCStringConvertToAutoreleasedNSString(t_ios_data->product_id)]);
		}
		if (t_success)
		{
			[t_ios_data->payment retain];
			p_purchase->platform_data = t_ios_data;
            t_ios_data->error = MCValueRetain(kMCEmptyString);
		}
		else
        {
            MCValueRelease(t_ios_data -> product_id);
            MCMemoryDelete(t_ios_data);
        }
	}
	
	return t_success;
}

void MCPurchaseFinalize(MCPurchase *p_purchase)
{
	if (p_purchase == nil)
		return;
	
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
	if (t_ios_data == nil)
		return;
	
	MCValueRelease(t_ios_data->error);
    MCValueRelease(t_ios_data->product_id);

	if (t_ios_data->payment)
		[t_ios_data->payment release];
	if (t_ios_data->transaction)
		[t_ios_data->transaction release];
	
	MCMemoryDelete(t_ios_data);
}

// PM-2015-01-12: [[ Bug 14343 ]] Implemented MCStoreGetPurchaseProperty/MCStoreSetPurchaseProperty for iOS
extern MCPropertyInfo *lookup_purchase_property(const MCPurchasePropertyTable *p_table, Properties p_which);

void MCStoreGetPurchaseProperty(MCExecContext& ctxt, MCStringRef p_product_id, MCStringRef p_prop_name, MCStringRef& r_property_value)
{
    MCPurchase *t_purchase = nil;
    Properties t_property;
    
    MCPropertyInfo *t_info;
    t_info = nil;
    
    if (MCPurchaseFindByProdId(p_product_id, t_purchase) && MCPurchaseLookupProperty(p_prop_name, t_property))
        t_info = lookup_purchase_property(getpropertytable(), t_property);
    
    if (t_info != nil)
	{
		MCExecValue t_value;
        MCExecFetchProperty(ctxt, t_info, t_purchase, t_value);
        if (ctxt.HasError())
            r_property_value = MCValueRetain(kMCEmptyString);
        else
            MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeStringRef, &r_property_value);
        return;
    }
    
    ctxt .Throw();
}

void MCStoreSetPurchaseProperty(MCExecContext& ctxt, MCStringRef p_product_id, MCStringRef p_prop_name, MCStringRef p_value)
{
    MCPurchase *t_purchase = nil;
	Properties t_property;
    
    MCPropertyInfo *t_info;
    t_info = nil;
    
    if (MCPurchaseFindByProdId(p_product_id, t_purchase) && MCPurchaseLookupProperty(p_prop_name, t_property))
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

void MCPurchaseGetProductIdentifier(MCExecContext& ctxt, MCPurchase *p_purchase, MCStringRef& r_productIdentifier)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPayment *t_payment = nil;
	SKPaymentTransaction *t_transaction = nil;
    t_transaction = t_ios_data->transaction;
	
    if (t_transaction != nil)
		t_payment = [t_transaction payment];
	else
		t_payment = t_ios_data->payment;
    
    if (t_payment != nil && MCStringCreateWithCFStringRef((CFStringRef)[t_payment productIdentifier], r_productIdentifier))
        return;
    
    ctxt . Throw();
}

void MCPurchaseGetQuantity(MCExecContext& ctxt, MCPurchase *p_purchase, uinteger_t& r_quantity)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPayment *t_payment = nil;
	SKPaymentTransaction *t_transaction = nil;
    t_transaction = t_ios_data->transaction;
	
    if (t_transaction != nil)
		t_payment = [t_transaction payment];
	else
		t_payment = t_ios_data->payment;
    
    if (t_payment != nil)
    {
        r_quantity = [t_payment quantity];
        return;
    }
    
    ctxt . Throw();
}

void MCPurchaseGetPurchaseDate(MCExecContext& ctxt, MCPurchase *p_purchase, integer_t& r_date)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPayment *t_payment = nil;
	SKPaymentTransaction *t_transaction = nil;
    t_transaction = t_ios_data->transaction;
	
    if (t_transaction != nil)
	{
		t_transaction = t_ios_data->transaction;
		t_payment = [t_transaction payment];
	}
	else
		t_payment = t_ios_data->payment;
    
    if (t_transaction != nil)
    {
        r_date = [[t_transaction transactionDate] timeIntervalSince1970];
        return;
    }
    
    ctxt . Throw();
}
// iOS
void MCPurchaseGetTransactionIdentifier(MCExecContext& ctxt, MCPurchase *p_purchase, MCStringRef& r_identifier)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPayment *t_payment = nil;
	SKPaymentTransaction *t_transaction = nil;
    t_transaction = t_ios_data->transaction;
	
    if (t_transaction != nil)
		t_payment = [t_transaction payment];
	else
		t_payment = t_ios_data->payment;
    
    // PM-2015-03-10: [[ Bug 14858 ]] transactionIdentifier can be nil if the purchase is still in progress (i.e when purchaseStateUpdate msg is sent with state=sendingRequest)
    if (t_transaction != nil
            && [t_transaction transactionIdentifier] != nil
            && MCStringCreateWithCFStringRef((CFStringRef)[t_transaction transactionIdentifier], r_identifier))
        return;
    
    ctxt . Throw();
}

void MCPurchaseGetReceipt(MCExecContext& ctxt, MCPurchase *p_purchase, MCDataRef& r_receipt)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPayment *t_payment = nil;
	SKPaymentTransaction *t_transaction = nil;
    t_transaction = t_ios_data->transaction;
	
    if (t_transaction != nil)
		t_payment = [t_transaction payment];
	else
		t_payment = t_ios_data->payment;
    
    if (t_transaction != nil)
    {
        NSData *t_bytes = [t_transaction transactionReceipt];
        MCDataCreateWithBytes((const byte_t*)[t_bytes bytes], [t_bytes length], r_receipt);
        return;
    }
    
    ctxt . Throw();
}

void MCPurchaseGetOriginalTransactionIdentifier(MCExecContext& ctxt, MCPurchase *p_purchase, MCStringRef& r_identifier)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPaymentTransaction *t_transaction = nil;
	SKPaymentTransaction *t_original_transaction = nil;
    t_transaction = t_ios_data->transaction;
	
    if (t_transaction != nil)
		t_original_transaction = [t_transaction originalTransaction];
    
    if (t_original_transaction != nil && MCStringCreateWithCString([[t_original_transaction transactionIdentifier] cStringUsingEncoding:NSMacOSRomanStringEncoding], r_identifier))
        return;
    
    ctxt . Throw();
}

void MCPurchaseGetOriginalPurchaseDate(MCExecContext& ctxt, MCPurchase *p_purchase, integer_t& r_date)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPaymentTransaction *t_transaction = nil;
	SKPaymentTransaction *t_original_transaction = nil;
    t_transaction = t_ios_data->transaction;
	
    if (t_transaction != nil)
		t_original_transaction = [t_transaction originalTransaction];
    
    if (t_original_transaction != nil)
    {
        r_date = [[t_original_transaction transactionDate] timeIntervalSince1970];
        return;
    }
    
    ctxt . Throw();
}

void MCPurchaseGetOriginalReceipt(MCExecContext& ctxt, MCPurchase *p_purchase, MCDataRef& r_receipt)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPaymentTransaction *t_transaction = nil;
	SKPaymentTransaction *t_original_transaction = nil;
    t_transaction = t_ios_data->transaction;
	
    if (t_transaction != nil)
		t_original_transaction = [t_transaction originalTransaction];
    
    if (t_original_transaction != nil)
    {
        NSData *t_bytes = [t_original_transaction transactionReceipt];
        MCDataCreateWithBytes((const byte_t*)[t_bytes bytes], [t_bytes length], r_receipt);
        return;
    }
    
    ctxt . Throw();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void MCPurchaseSetQuantity(MCExecContext& ctxt, MCPurchase *p_purchase, uinteger_t p_quantity)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
    if (t_ios_data->payment == nil)
        return;
    
    [t_ios_data->payment setQuantity: p_quantity];
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseSendRequest(MCPurchase *p_purchase)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
	
	if (t_ios_data->payment == nil)
		return false;
	
	s_purchase_request = p_purchase;
	[[SKPaymentQueue defaultQueue] addPayment: t_ios_data->payment];
	s_purchase_request = nil;
	
	return true;
}

bool MCPurchaseConfirmDelivery(MCPurchase *p_purchase)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
	
	[[SKPaymentQueue defaultQueue] finishTransaction: t_ios_data->transaction];
	p_purchase->state = kMCPurchaseStateComplete;
    
    // PM-2015-01-28: [[ Bug 14461 ]] Once the purchase is completed, add the productID to the completed purchases list
    MCPurchaseCompleteListUpdate(p_purchase);
    
	MCPurchaseNotifyUpdate(p_purchase);
	MCPurchaseRelease(p_purchase);
	
	return true;
}

bool MCStoreCanMakePurchase()
{
	return [SKPaymentQueue canMakePayments];
}

bool MCStoreRestorePurchases()
{
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
	return true;
}

bool MCPurchaseGetError(MCPurchase *p_purchase, MCStringRef &r_error)
{	
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
	
	if (t_ios_data == nil)
		return false;
	
	r_error = MCValueRetain(t_ios_data->error);
    return true;
}

bool MCStoreProductSetType(MCStringRef p_product_id, MCStringRef p_type)
{
    return true;
}

bool MCStoreMakePurchase(MCStringRef p_product_id, MCStringRef p_quantity, MCStringRef p_payload)
{
    MCPurchase *t_purchase = nil;
    bool t_success;    
    t_success = true;
    
    if (t_success)
        t_success = MCPurchaseFindByProdId(p_product_id, t_purchase);
    
    MCiOSPurchase *t_ios_data = (MCiOSPurchase*)t_purchase->platform_data;
    
    if (t_ios_data->payment == nil)
        return false;
    
    uint32_t t_quantity;
    MCU_stoui4(p_quantity, t_quantity);
    [t_ios_data->payment setQuantity: t_quantity];
    
    s_purchase_request = t_purchase;
    [[SKPaymentQueue defaultQueue] addPayment: t_ios_data->payment];
    s_purchase_request = nil;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void update_purchase_state(MCPurchase *p_purchase)
{
	if (p_purchase == nil)
		return;

	MCiOSPurchase *t_ios_data = (MCiOSPurchase *)p_purchase->platform_data;

	if (t_ios_data->transaction != nil)
	{
		switch ([t_ios_data->transaction transactionState])
		{
			case SKPaymentTransactionStatePurchasing:
				p_purchase->state = kMCPurchaseStateSendingRequest;
				break;
			case SKPaymentTransactionStatePurchased:
				p_purchase->state = kMCPurchaseStatePaymentReceived;
				break;
			case SKPaymentTransactionStateRestored:
				p_purchase->state = kMCPurchaseStateRestored;
                s_did_restore = true;
				break;
			case SKPaymentTransactionStateFailed:
			{
				NSError *t_error = [t_ios_data->transaction error];
                
                if(t_error == nil)
                    return;
				if ([[t_error domain] isEqualToString:SKErrorDomain] && [t_error code] == SKErrorPaymentCancelled)
					p_purchase->state = kMCPurchaseStateCancelled;
				else
				{
					p_purchase->state = kMCPurchaseStateError;
                    MCValueRelease(t_ios_data->error);
                    MCStringCreateWithCFStringRef((CFStringRef)[t_error localizedDescription], t_ios_data->error);
				}
				break;
			}
			default:
				p_purchase->state = kMCPurchaseStateUnknown;
				break;
		}
	}
}

@interface com_runrev_livecode_MCPurchaseObserver : NSObject <SKPaymentTransactionObserver>
{
}

- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray *)transactions;
- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error;
- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions;
- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue;

@end

@implementation com_runrev_livecode_MCPurchaseObserver

- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray *)transactions
{
}

- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error
{
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
	for (SKPaymentTransaction *t_transaction in [[SKPaymentQueue defaultQueue] transactions])
	{
		MCPurchase *t_purchase = nil;
		if (s_purchase_request != nil)
			t_purchase = s_purchase_request;
		else if (!MCPurchaseFindByTransaction(t_transaction, t_purchase))
		{
			bool t_success = true;
			MCiOSPurchase *t_ios_data = nil;
			
			t_success = MCMemoryNew(t_ios_data);
			if (t_success)
				t_success = MCStringCreateWithCFStringRef((CFStringRef)[[t_transaction payment] productIdentifier], t_ios_data->product_id);

			if (t_success)
				t_success = MCPurchaseCreate(t_ios_data->product_id, t_ios_data, t_purchase);

			if (!t_success)
            {
                MCValueRelease(t_ios_data -> product_id);
                MCMemoryDelete(t_ios_data);
            }
		}
		
		if (t_purchase != nil)
		{
			MCiOSPurchase *t_ios_data = (MCiOSPurchase*)t_purchase->platform_data;
			if (t_ios_data->transaction != nil)
				[t_ios_data->transaction release];

			t_ios_data->transaction = t_transaction;
			[t_ios_data->transaction retain];
			
			update_purchase_state(t_purchase);
			MCPurchaseNotifyUpdate(t_purchase);
			           
			//if ([t_transaction transactionState] == SKPaymentTransactionStateFailed)
            if ([t_transaction transactionState] != SKPaymentTransactionStatePurchasing)
			{
				[[SKPaymentQueue defaultQueue] finishTransaction: t_transaction];
                if ([t_transaction transactionState] == SKPaymentTransactionStateFailed)
                {
                    MCPurchaseRelease(t_purchase);
                }
			}
		}
	}
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
    // PM-2015-02-12: [[ Bug 14402 ]] When there are no previous purchases to restore, send a purchaseStateUpdate msg with state=restored and productID=""
    if (!s_did_restore)
    {
        MCPurchase *t_empty_purchase = new MCPurchase[1]();
        t_empty_purchase -> prod_id = MCValueRetain(kMCEmptyString);
        t_empty_purchase -> id = 0;
        t_empty_purchase -> ref_count = 0;
        t_empty_purchase -> state = kMCPurchaseStateRestored;
        
        MCPurchaseNotifyUpdate(t_empty_purchase);
    }
}

@end

com_runrev_livecode_MCPurchaseObserver *s_purchase_observer = nil;

bool MCStoreEnablePurchaseUpdates()
{
	if (s_purchase_observer != nil)
		return true;
	
	s_purchase_observer = [[com_runrev_livecode_MCPurchaseObserver alloc] init];
	if (s_purchase_observer == nil)
		return false;
	
	[[SKPaymentQueue defaultQueue] addTransactionObserver:s_purchase_observer];
	
	return true;
}

bool MCStoreDisablePurchaseUpdates()
{
	if (s_purchase_observer == nil)
		return true;
	
	[[SKPaymentQueue defaultQueue] removeTransactionObserver:s_purchase_observer];
	
	s_purchase_observer = nil;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStorePostProductRequestError(MCStringRef p_product, MCStringRef p_error);
bool MCStorePostProductRequestResponse(SKProduct *p_product);

@interface com_runrev_livecode_MCProductsRequest : SKProductsRequest
{
    NSString *m_product_id;
}

- (id)initWithProductId:(NSString *)productId;
- (NSString*)getProductId;

@end

@implementation com_runrev_livecode_MCProductsRequest

- (id)initWithProductId:(NSString *)p_productId
{
    self = [super initWithProductIdentifiers: [NSSet setWithObject:p_productId]];
    if (self == nil)
        return nil;
    
    self->m_product_id = p_productId;
    [self->m_product_id retain];
    
    return self;
}

- (NSString*)getProductId
{
    return m_product_id;
}

- (void)dealloc
{
    [self->m_product_id release];
    
    [super dealloc];
}

@end

@interface com_runrev_livecode_MCProductsRequestDelegate : NSObject <SKProductsRequestDelegate>
{
}
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response;

- (void)requestDidFinish:(SKRequest *)request;
- (void)request:(SKRequest *)request didFailWithError:(NSError *)error;

@end

@implementation com_runrev_livecode_MCProductsRequestDelegate
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    if (response.invalidProductIdentifiers != nil)
    {
        for (NSString *t_invalid_id in response.invalidProductIdentifiers)
        {
            MCAutoStringRef t_string;
            /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)t_invalid_id, &t_string);
            MCStorePostProductRequestError(*t_string, MCSTR("invalid product identifier"));
        }
    }
    
    if (response.products != nil)
    {
        for (SKProduct *t_product in response.products)
        {
            MCStorePostProductRequestResponse(t_product);
        }
    }
}

- (void)requestDidFinish:(SKRequest *)request
{
    [request release];
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
    MCAutoStringRef t_product, t_error;
    
    /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[(com_runrev_livecode_MCProductsRequest*)request getProductId], &t_product);
    /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[error description], &t_error);

    MCStorePostProductRequestError(*t_product, *t_error);
    [request release];
}

@end


bool MCStoreRequestProductDetails(MCStringRef p_product_id)
{
    SKProductsRequest *t_request = nil;
    
    NSString *t_product_id = nil;
    
    t_product_id = MCStringConvertToAutoreleasedNSString(p_product_id);
    t_request = [[com_runrev_livecode_MCProductsRequest alloc] initWithProductId: t_product_id];
    
    [t_request setDelegate: [[com_runrev_livecode_MCProductsRequestDelegate alloc] init]];
    
    [t_request start];
    
    return true;
}

bool MCStoreReceiveProductDetails(MCStringRef p_product_id, MCStringRef &r_details)
{
    r_details = MCValueRetain(kMCEmptyString);
    return true;
}

bool MCStoreConsumePurchase(MCStringRef p_product_id)
{
    return true;
}

////////

class MCStoreProductRequestErrorEvent : public MCCustomEvent
{
public:
    MCStoreProductRequestErrorEvent(MCStringRef p_product, MCStringRef p_error);
    ~MCStoreProductRequestErrorEvent();
    
    void Destroy();
    void Dispatch();
    
private:
    MCStringRef m_product;
    MCStringRef m_error;
};

MCStoreProductRequestErrorEvent::MCStoreProductRequestErrorEvent(MCStringRef p_product, MCStringRef p_error)
{
    m_product = MCValueRetain(p_product);
    m_error = MCValueRetain(p_error);
}

MCStoreProductRequestErrorEvent::~MCStoreProductRequestErrorEvent()
{
    MCValueRelease(m_product);
    MCValueRelease(m_error);
}

void MCStoreProductRequestErrorEvent::Destroy()
{
    delete this;
}

void MCStoreProductRequestErrorEvent::Dispatch()
{
    MCdefaultstackptr->getcurcard()->message_with_valueref_args(MCM_product_request_error, m_product, m_error);
}

bool MCStorePostProductRequestError(MCStringRef p_product, MCStringRef p_error)
{
    bool t_success;
    MCCustomEvent *t_event = nil;
    t_event = new MCStoreProductRequestErrorEvent(p_product, p_error);
    t_success = t_event != nil;
    
    if (t_success)
        MCEventQueuePostCustom(t_event);
    
    return t_success;
}

////////

class MCStoreProductRequestResponseEvent : public MCCustomEvent
{
public:
    MCStoreProductRequestResponseEvent(SKProduct *p_product);
    
    void Dispatch();
    void Destroy();
    
private:
    SKProduct *m_product;
};

MCStoreProductRequestResponseEvent::MCStoreProductRequestResponseEvent(SKProduct *p_product)
{
    m_product = p_product;
    [m_product retain];
}

void MCStoreProductRequestResponseEvent::Destroy()
{
    [m_product release];
}

bool MCNSStringToUnicode(NSString *p_ns_string, unichar_t *&r_uni_string, uint32_t &r_length)
{
    if (p_ns_string == nil || [p_ns_string length] == 0)
    {
        r_uni_string = nil;
        r_length = 0;
        return true;
    }
    
    uint32_t t_length;
    t_length = [p_ns_string length];
    
    if (!MCMemoryAllocate(sizeof(unichar_t) * t_length, r_uni_string))
        return false;
    
    r_length = t_length;
    [p_ns_string getCharacters:r_uni_string range:NSMakeRange(0, r_length)];
    
    return true;
}

void MCStoreProductRequestResponseEvent::Dispatch()
{    
    bool t_success = true;
    
    MCAutoStringRef t_product_id, t_description, t_title, t_currency_code, t_currency_symbol;
    MCAutoDataRef t_utf16_title, t_utf16_description, t_utf16_currency_symbol;
    double t_price = 0.0;
    
    NSString *t_locale_currency_code = nil;
    NSString *t_locale_currency_symbol = nil;
    
    t_locale_currency_code = [[m_product priceLocale] objectForKey: NSLocaleCurrencyCode];
    t_locale_currency_symbol = [[m_product priceLocale] objectForKey: NSLocaleCurrencySymbol];
    
    /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[m_product productIdentifier], &t_product_id);
    /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[m_product localizedDescription], &t_description);
    /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[m_product localizedTitle], &t_title);
    /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)t_locale_currency_code, &t_currency_code);
    /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)t_locale_currency_symbol, &t_currency_symbol);

    unichar_t *t_unicode_description = nil;
    uint32_t t_unicode_description_length = 0;
    
    unichar_t *t_unicode_title = nil;
    uint32_t t_unicode_title_length = 0;
    
    unichar_t *t_unicode_currency_symbol = nil;
    uint32_t t_unicode_currency_symbol_length = 0;
    
    if (t_success)
        t_success = MCNSStringToUnicode([m_product localizedDescription], t_unicode_description, t_unicode_description_length);
    
    if (t_success)
        t_success = MCNSStringToUnicode([m_product localizedTitle], t_unicode_title, t_unicode_title_length);
    
    if (t_success)
        t_success = MCNSStringToUnicode(t_locale_currency_symbol, t_unicode_currency_symbol, t_unicode_currency_symbol_length);
    
    /* UNCHECKED */ MCDataCreateWithBytes((const byte_t *)t_unicode_description, t_unicode_description_length * 2, &t_utf16_description);
    /* UNCHECKED */ MCDataCreateWithBytes((const byte_t *)t_unicode_title, t_unicode_title_length * 2, &t_utf16_title);
    /* UNCHECKED */ MCDataCreateWithBytes((const byte_t *)t_unicode_currency_symbol, t_unicode_currency_symbol_length * 2, &t_utf16_currency_symbol);
    
    MCAutoNumberRef t_price_number;
    
    MCAutoArrayRef t_array;
    if (t_success)
        t_success = MCArrayCreateMutable(&t_array);
    
    if (t_success)
        t_success = (MCNumberCreateWithReal([[m_product price] doubleValue], &t_price_number)
                     && MCArrayStoreValue(*t_array, kMCCompareCaseless, MCNAME("price"), *t_price_number));

        
    if (t_success && *t_description != nil)
    {
        t_success = MCArrayStoreValue(*t_array, kMCCompareCaseless, MCNAME("description"), *t_description);
    }
    
    if (t_success && *t_title != nil)
    {
        t_success = MCArrayStoreValue(*t_array, kMCCompareCaseless, MCNAME("title"), *t_title);
    }
    
    if (t_success && *t_currency_code != nil)
    {
        t_success = MCArrayStoreValue(*t_array, kMCCompareCaseless, MCNAME("currency code"), *t_currency_code);
    }
    
    if (t_success && *t_currency_symbol != nil)
    {
        t_success = MCArrayStoreValue(*t_array, kMCCompareCaseless, MCNAME("currency symbol"), *t_currency_symbol);
    }
    
    if (t_success && *t_unicode_description != 0)
    {
        t_success = MCArrayStoreValue(*t_array, kMCCompareCaseless, MCNAME("unicode description"), *t_utf16_description);
    }
    
    if (t_success && *t_unicode_title != 0)
    {
        t_success = MCArrayStoreValue(*t_array, kMCCompareCaseless, MCNAME("unicode title"), *t_utf16_title);
    }
    
    if (t_success && *t_unicode_currency_symbol != 0)
    {
        t_success = MCArrayStoreValue(*t_array, kMCCompareCaseless, MCNAME("unicode currency symbol"), *t_utf16_currency_symbol);
    }
    
    MCParameter p1, p2;
    p1.setvalueref_argument(*t_product_id);
    p1.setnext(&p2);
    if (*t_array != nil)
        p2.setvalueref_argument(*t_array);
    else
        p2.setvalueref_argument(kMCEmptyArray);
    
    MCdefaultstackptr->getcurcard()->message(MCM_product_details_received, &p1);

    
    if (t_unicode_description != nil)
        MCMemoryDeleteArray(t_unicode_description);
    if (t_unicode_title != nil)
        MCMemoryDeleteArray(t_unicode_title);
    if (t_unicode_currency_symbol != nil)
        MCMemoryDeleteArray(t_unicode_currency_symbol);
}

bool MCStorePostProductRequestResponse(SKProduct *p_product)
{
    bool t_success;
    MCCustomEvent *t_event = nil;
    t_event = new MCStoreProductRequestResponseEvent(p_product);
    t_success = t_event != nil;
    
    if (t_success)
        MCEventQueuePostCustom(t_event);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCPurchaseVerify(MCPurchase *p_purchase, bool p_verified)
{
    // Not Implemented
}

////////////////////////////////////////////////////////////////////////////////

