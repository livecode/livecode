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
#include "parsedef.h"

#include "mcerror.h"
#include "execpt.h"
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

typedef struct
{
	SKMutablePayment *payment;
	NSString *product_id;
	SKPaymentTransaction *transaction;
	char *error;
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

		t_success = MCStringGetLength(p_product_id) != 0;
		
		if (t_success)
			t_success = MCMemoryNew(t_ios_data);
		
		if (t_success)
			t_success = nil != (t_ios_data->product_id = [NSString stringWithMCStringRef: p_product_id]);
		if (t_success)
			t_success = nil != (t_ios_data->payment = [SKMutablePayment paymentWithProductIdentifier: t_ios_data->product_id]);
		
		if (t_success)
		{
			[t_ios_data->product_id retain];
			[t_ios_data->payment retain];
			p_purchase->platform_data = t_ios_data;
		}
		else
			MCMemoryDelete(t_ios_data);
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
	
	if (t_ios_data->error)
		MCCStringFree(t_ios_data->error);
	if (t_ios_data->payment)
		[t_ios_data->payment release];
	if (t_ios_data->transaction)
		[t_ios_data->transaction release];
	
	MCMemoryDelete(t_ios_data);
}

#ifdef /* MCPurchaseGet */ LEGACY_EXEC
Exec_stat MCPurchaseGet(MCPurchase *p_purchase, MCPurchaseProperty p_property, MCExecPoint &ep)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;

	SKPayment *t_payment = nil;
	SKPaymentTransaction *t_transaction = nil;
	SKPaymentTransaction *t_original_transaction = nil;
	
	if (t_ios_data->transaction != nil)
	{
		t_transaction = t_ios_data->transaction;
		t_payment = [t_transaction payment];
		t_original_transaction = [t_transaction originalTransaction];
	}
	else
		t_payment = t_ios_data->payment;
	
	switch (p_property)
	{
		case kMCPurchasePropertyProductIdentifier:
			if (t_payment == nil)
				break;
			
			ep.copysvalue([[t_payment productIdentifier] cStringUsingEncoding: NSMacOSRomanStringEncoding]);
			return ES_NORMAL;
			
		case kMCPurchasePropertyQuantity:
			if (t_payment == nil)
				break;
			
			ep.setuint([t_payment quantity]);
			return ES_NORMAL;
			
		case kMCPurchasePropertyReceipt:
		{
			if (t_transaction == nil)
				break;
			
			NSData *t_bytes = [t_transaction transactionReceipt];
			ep.copysvalue((const char*)[t_bytes bytes], [t_bytes length]);
			return ES_NORMAL;
		}
			
		case kMCPurchasePropertyPurchaseDate:
			if (t_transaction == nil)
				break;
			
			ep.setnvalue([[t_transaction transactionDate] timeIntervalSince1970]);
			return ES_NORMAL;

		case kMCPurchasePropertyTransactionIdentifier:
			if (t_transaction == nil)
				break;
			
			ep.copysvalue([[t_transaction transactionIdentifier] cStringUsingEncoding:NSMacOSRomanStringEncoding]);
			return ES_NORMAL;
			
		case kMCPurchasePropertyOriginalReceipt:
		{
			if (t_original_transaction == nil)
				break;
			
			NSData *t_bytes = [t_original_transaction transactionReceipt];
			ep.copysvalue((const char*)[t_bytes bytes], [t_bytes length]);
			return ES_NORMAL;
		}
			
		case kMCPurchasePropertyOriginalPurchaseDate:
			if (t_original_transaction == nil)
				break;
			
			ep.setnvalue([[t_original_transaction transactionDate] timeIntervalSince1970]);
			return ES_NORMAL;

		case kMCPurchasePropertyOriginalTransactionIdentifier:
			if (t_original_transaction == nil)
				break;
			
			ep.copysvalue([[t_original_transaction transactionIdentifier] cStringUsingEncoding:NSMacOSRomanStringEncoding]);
			return ES_NORMAL;
			
		default:
			break;
	}
	
	return ES_NOT_HANDLED;
}
#endif /* MCPurchaseGet */

void MCPurchaseGetProductIdentifier(MCExecContext& ctxt, MCPurchase *p_purchase, MCStringRef& r_productIdentifier)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPayment *t_payment = nil;
	SKPaymentTransaction *t_transaction = nil;
	
	if (t_ios_data->transaction != nil)
		t_payment = [t_transaction payment];
	else
		t_payment = t_ios_data->payment;
    
    if (t_payment != nil && MCStringCreateWithCString([[t_payment productIdentifier] cStringUsingEncoding: NSMacOSRomanStringEncoding], r_productIdentifier))
        return;
    
    ctxt . Throw();
}

void MCPurchaseGetQuantity(MCExecContext& ctxt, MCPurchase *p_purchase, uinteger_t& r_quantity)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPayment *t_payment = nil;
	SKPaymentTransaction *t_transaction = nil;
	
	if (t_ios_data->transaction != nil)
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
	
	if (t_ios_data->transaction != nil)
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
	
	if (t_ios_data->transaction != nil)
	{
		t_transaction = t_ios_data->transaction;
		t_payment = [t_transaction payment];
	}
	else
		t_payment = t_ios_data->payment;
    
    if (t_transaction != nil && MCStringCreateWithCString([[t_transaction transactionIdentifier] cStringUsingEncoding:NSMacOSRomanStringEncoding], r_identifier))
        return;
    
    ctxt . Throw();
}

void MCPurchaseGetReceipt(MCExecContext& ctxt, MCPurchase *p_purchase, MCDataRef& r_receipt)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    
	SKPayment *t_payment = nil;
	SKPaymentTransaction *t_transaction = nil;
	
	if (t_ios_data->transaction != nil)
	{
		t_transaction = t_ios_data->transaction;
		t_payment = [t_transaction payment];
	}
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
	
	if (t_ios_data->transaction != nil)
	{
		t_transaction = t_ios_data->transaction;
		t_original_transaction = [t_transaction originalTransaction];
	}
    
    if (t_original_transaction != nil && MCStringCreateWithCString([[t_original_transaction transactionIdentifier] cStringUsingEncoding:NSMacOSRomanStringEncoding], r_identifier))
        return;
    
    ctxt . Throw();
}

void MCPurchaseGetOriginalPurchaseDate(MCExecContext& ctxt, MCPurchase *p_purchase, integer_t& r_date)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
    \
	SKPaymentTransaction *t_transaction = nil;
	SKPaymentTransaction *t_original_transaction = nil;
	
	if (t_ios_data->transaction != nil)
	{
		t_transaction = t_ios_data->transaction;
		t_original_transaction = [t_transaction originalTransaction];
	}
    
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
	
	if (t_ios_data->transaction != nil)
	{
		t_transaction = t_ios_data->transaction;
		t_original_transaction = [t_transaction originalTransaction];
	}
    
    if (t_original_transaction != nil)
    {
        NSData *t_bytes = [t_original_transaction transactionReceipt];
        MCDataCreateWithBytes((const byte_t*)[t_bytes bytes], [t_bytes length], r_receipt);
        return;
    }
    
    ctxt . Throw();
}

///////////////////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCPurchaseSet */ LEGACY_EXEC
Exec_stat MCPurchaseSet(MCPurchase *p_purchase, MCPurchaseProperty p_property, uint32_t p_quantity)
{
	MCiOSPurchase *t_ios_data = (MCiOSPurchase*)p_purchase->platform_data;
	switch (p_property)
	{
		case kMCPurchasePropertyQuantity:
		{
			if (t_ios_data->payment != nil)
				[t_ios_data->payment setQuantity: p_quantity];
			return ES_NORMAL;
		}
			break;
		default:
			break;
	}
	
	return ES_NOT_HANDLED;
}
#endif /* MCPurchaseSet */

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
	
	return MCStringCreateWithCString(t_ios_data->error, r_error);
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
				break;
			case SKPaymentTransactionStateFailed:
			{
				NSError *t_error = [t_ios_data->transaction error];
				if ([[t_error domain] isEqualToString:SKErrorDomain] && [t_error code] == SKErrorPaymentCancelled)
					p_purchase->state = kMCPurchaseStateCancelled;
				else
				{
					p_purchase->state = kMCPurchaseStateError;
					MCCStringClone([[t_error localizedDescription] cStringUsingEncoding: NSMacOSRomanStringEncoding], t_ios_data->error);
				}
				break;
			}
			default:
				p_purchase->state = kMCPurchaseStateUnknown;
				break;
		}
	}
}

@interface MCPurchaseObserver : NSObject <SKPaymentTransactionObserver>
{
}

- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray *)transactions;
- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error;
- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions;
- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue;

@end

@implementation MCPurchaseObserver

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
				t_success = nil != (t_ios_data->product_id = [[t_transaction payment] productIdentifier]);

			if (t_success)
				t_success = MCPurchaseCreate(nil, t_ios_data, t_purchase);

			if (!t_success)
				MCMemoryDelete(t_ios_data);
			else
			{
				[t_ios_data->product_id retain];
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
			
			if ([t_transaction transactionState] == SKPaymentTransactionStateFailed)
			{
				[[SKPaymentQueue defaultQueue] finishTransaction: t_transaction];
				MCPurchaseRelease(t_purchase);
			}
		}
	}
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
}

@end

MCPurchaseObserver *s_purchase_observer = nil;

bool MCStoreEnablePurchaseUpdates()
{
	if (s_purchase_observer != nil)
		return true;
	
	s_purchase_observer = [[MCPurchaseObserver alloc] init];
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

@interface MCProductsRequest : SKProductsRequest
{
    NSString *m_product_id;
}

- (id)initWithProductId:(NSString *)productId;
- (NSString*)getProductId;

@end

@implementation MCProductsRequest

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

@interface MCProductsRequestDelegate : NSObject <SKProductsRequestDelegate>
{
}
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response;

- (void)requestDidFinish:(SKRequest *)request;
- (void)request:(SKRequest *)request didFailWithError:(NSError *)error;

@end

@implementation MCProductsRequestDelegate
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    if (response.invalidProductIdentifiers != nil)
    {
        for (NSString *t_invalid_id in response.invalidProductIdentifiers)
        {
            MCAutoStringRef t_string;
            /* UNCHECKED */ MCStringCreateWithCString([t_invalid_id
                                                       cStringUsingEncoding:
                                                       NSMacOSRomanStringEncoding],
                                                      &t_string);
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
    const char *t_product_id = [[(MCProductsRequest*)request getProductId] cStringUsingEncoding: NSMacOSRomanStringEncoding];
    const char *t_error_str = [[error description] cStringUsingEncoding:
                               NSMacOSRomanStringEncoding];
    MCAutoStringRef t_product, t_error;
    /* UNCHECKED */ MCStringCreateWithCString(t_product_id, &t_product);
    /* UNCHECKED */ MCStringCreateWithCString(t_error_str, &t_error);
    MCStorePostProductRequestError(*t_product, *t_error);
    [request release];
}

@end


bool MCStoreRequestProductDetails(MCStringRef p_product_id)
{
    SKProductsRequest *t_request = nil;
    
    NSString *t_product_id = nil;
    
    t_product_id = [NSString stringWithMCStringRef: p_product_id];
    t_request = [[MCProductsRequest alloc] initWithProductId: t_product_id];
    
    [t_request setDelegate: [[MCProductsRequestDelegate alloc] init]];
    
    [t_request start];
    
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
/*    bool t_success = true;
    
    const char *t_product_id = nil;
    const char *t_description = nil;
    const char *t_title = nil;
    const char *t_currency_code = nil;
    const char *t_currency_symbol = nil;
    
    unichar_t *t_unicode_description = nil;
    uint32_t t_unicode_description_length = 0;

    unichar_t *t_unicode_title = nil;
    uint32_t t_unicode_title_length = 0;
    
    unichar_t *t_unicode_currency_symbol = nil;
    uint32_t t_unicode_currency_symbol_length = 0;
    
    double t_price = 0.0;
    
    NSString *t_locale_currency_code = nil;
    NSString *t_locale_currency_symbol = nil;
    
    t_locale_currency_code = [[m_product priceLocale] objectForKey: NSLocaleCurrencyCode];
    t_locale_currency_symbol = [[m_product priceLocale] objectForKey: NSLocaleCurrencySymbol];
    
    t_product_id = [[m_product productIdentifier] cStringUsingEncoding: NSMacOSRomanStringEncoding];
    t_description = [[m_product localizedDescription] cStringUsingEncoding: NSMacOSRomanStringEncoding];
    t_title = [[m_product localizedTitle] cStringUsingEncoding: NSMacOSRomanStringEncoding];
    t_currency_code = [t_locale_currency_code cStringUsingEncoding: NSMacOSRomanStringEncoding];
    t_currency_symbol = [t_locale_currency_symbol cStringUsingEncoding: NSMacOSRomanStringEncoding];
    
    if (t_success)
        t_success = MCNSStringToUnicode([m_product localizedDescription], t_unicode_description, t_unicode_description_length);
    
    if (t_success)
        t_success = MCNSStringToUnicode([m_product localizedTitle], t_unicode_title, t_unicode_title_length);
    
    if (t_success)
        t_success = MCNSStringToUnicode(t_locale_currency_symbol, t_unicode_currency_symbol, t_unicode_currency_symbol_length);
    
    if (t_success)
    {
        MCExecPoint ep(nil, nil, nil);
        
        MCVariableValue *t_response = nil;
        t_response = new MCVariableValue();
        
        MCVariableValue *t_element = nil;
        
        t_response->lookup_element(ep, "price", t_element);
        t_element->assign_real([[m_product price] doubleValue]);
        
        if (t_description != nil)
        {
            t_response->lookup_element(ep, "description", t_element);
            t_element->assign_string(MCString(t_description));
        }
        
        if (t_title != nil)
        {
            t_response->lookup_element(ep, "title", t_element);
            t_element->assign_string(MCString(t_title));
        }
        
        if (t_currency_code != nil)
        {
            t_response->lookup_element(ep, "currency code", t_element);
            t_element->assign_string(MCString(t_currency_code));
        }
        
        if (t_currency_symbol != nil)
        {
            t_response->lookup_element(ep, "currency symbol", t_element);
            t_element->assign_string(MCString(t_currency_symbol));
        }
        
        if (t_unicode_description != nil)
        {
            t_response->lookup_element(ep, "unicode description", t_element);
            t_element->assign_string(MCString((char*)t_unicode_description, 2 * t_unicode_description_length));
        }
        
        if (t_unicode_title != nil)
        {
            t_response->lookup_element(ep, "unicode title", t_element);
            t_element->assign_string(MCString((char*)t_unicode_title, 2 * t_unicode_title_length));
        }
        
        if (t_unicode_currency_symbol != nil)
        {
            t_response->lookup_element(ep, "unicode currency symbol", t_element);
            t_element->assign_string(MCString((char*)t_unicode_currency_symbol, 2 * t_unicode_currency_symbol_length));
        }
        
        
        ep.setarray(t_response, True);

        MCParameter p1, p2;
        p1.sets_argument(MCString(t_product_id));
        p1.setnext(&p2);
        p2.set_argument(ep);
        
        MCdefaultstackptr->getcurcard()->message(MCM_product_details_received, &p1);
    }
    
    if (t_unicode_description != nil)
        MCMemoryDeleteArray(t_unicode_description);
    if (t_unicode_title != nil)
        MCMemoryDeleteArray(t_unicode_title);
    if (t_unicode_currency_symbol != nil)
        MCMemoryDeleteArray(t_unicode_currency_symbol); */
    
    bool t_success = true;
    
    const char *t_product_id = nil;
    const char *t_description = nil;
    const char *t_title = nil;
    const char *t_currency_code = nil;
    const char *t_currency_symbol = nil;
    
    unichar_t *t_unicode_description = nil;
    uint32_t t_unicode_description_length = 0;
    
    unichar_t *t_unicode_title = nil;
    uint32_t t_unicode_title_length = 0;
    
    unichar_t *t_unicode_currency_symbol = nil;
    uint32_t t_unicode_currency_symbol_length = 0;
    
    double t_price = 0.0;
    
    NSString *t_locale_currency_code = nil;
    NSString *t_locale_currency_symbol = nil;
    
    t_locale_currency_code = [[m_product priceLocale] objectForKey: NSLocaleCurrencyCode];
    t_locale_currency_symbol = [[m_product priceLocale] objectForKey: NSLocaleCurrencySymbol];
    
    t_product_id = [[m_product productIdentifier] cStringUsingEncoding: NSMacOSRomanStringEncoding];
    t_description = [[m_product localizedDescription] cStringUsingEncoding: NSMacOSRomanStringEncoding];
    t_title = [[m_product localizedTitle] cStringUsingEncoding: NSMacOSRomanStringEncoding];
    t_currency_code = [t_locale_currency_code cStringUsingEncoding: NSMacOSRomanStringEncoding];
    t_currency_symbol = [t_locale_currency_symbol cStringUsingEncoding: NSMacOSRomanStringEncoding];
    
    if (t_success)
        t_success = MCNSStringToUnicode([m_product localizedDescription], t_unicode_description, t_unicode_description_length);
    
    if (t_success)
        t_success = MCNSStringToUnicode([m_product localizedTitle], t_unicode_title, t_unicode_title_length);
    
    if (t_success)
        t_success = MCNSStringToUnicode(t_locale_currency_symbol, t_unicode_currency_symbol, t_unicode_currency_symbol_length);
    
    MCAutoStringRef t_description_string, t_title_string, t_currency_code_string, t_currency_symbol_string;
    MCAutoStringRef t_unicode_description_string, t_unicode_title_string, t_unicode_currency_symbol_string;
    MCAutoNumberRef t_price_number;
    
    MCNewAutoNameRef t_price_key, t_description_key, t_title_key, t_currency_code_key, t_currency_symbol_key;
    MCNewAutoNameRef t_unicode_description_key, t_unicode_title_key, t_unicode_currency_symbol_key;
    
    MCAutoArrayRef t_array;
    if (t_success)
        t_success = MCArrayCreateMutable(&t_array);
    
    if (t_success)
        t_success = (MCNumberCreateWithReal([[m_product price] doubleValue], &t_price_number)
                     && MCNameCreateWithCString("price", &t_price_key)
                     && MCArrayStoreValue(*t_array, kMCCompareCaseless, *t_price_key, *t_price_number));

        
    if (t_success && t_description != nil)
    {
        t_success = (MCStringCreateWithCString(t_description, &t_description_string)
                     && MCNameCreateWithCString("description", &t_description_key)
                     && MCArrayStoreValue(*t_array, kMCCompareCaseless, *t_description_key, *t_description_string));
    }
    
    if (t_success && t_title != nil)
    {
        t_success = (MCStringCreateWithCString(t_title, &t_title_string)
                     && MCNameCreateWithCString("title", &t_title_key)
                     && MCArrayStoreValue(*t_array, kMCCompareCaseless, *t_title_key, *t_title_string));
    }
    
    if (t_success && t_currency_code != nil)
    {
        t_success = (MCStringCreateWithCString(t_currency_code, &t_currency_code_string)
                     && MCNameCreateWithCString("currency code", &t_currency_code_key)
                     && MCArrayStoreValue(*t_array, kMCCompareCaseless, *t_currency_code_key, *t_currency_code_string));
    }
    
    if (t_success && t_currency_symbol != nil)
    {
        t_success = (MCStringCreateWithCString(t_currency_symbol, &t_currency_symbol_string)
                     && MCNameCreateWithCString("currency symbol", &t_currency_symbol_key)
                     && MCArrayStoreValue(*t_array, kMCCompareCaseless, *t_currency_symbol_key, *t_currency_symbol_string));
    }
    
    if (t_success && t_unicode_description != nil)
    {
        t_success = (MCStringCreateWithChars(t_unicode_description, 2 * t_unicode_description_length, &t_unicode_description_string)
                     && MCNameCreateWithCString("unicode description", &t_unicode_description_key)
                     && MCArrayStoreValue(*t_array, kMCCompareCaseless, *t_unicode_description_key, *t_unicode_description_string));
    }
    
    if (t_success && t_unicode_title != nil)
    {
        t_success = (MCStringCreateWithChars(t_unicode_title, 2 * t_unicode_title_length, &t_unicode_title_string)
                     && MCNameCreateWithCString("unicode title", &t_unicode_title_key)
                     && MCArrayStoreValue(*t_array, kMCCompareCaseless, *t_unicode_title_key, *t_unicode_title_string));
    }
    
    if (t_success && t_unicode_currency_symbol != nil)
    {
        t_success = (MCStringCreateWithChars(t_unicode_currency_symbol, 2 * t_unicode_currency_symbol_length, &t_unicode_currency_symbol_string)
                     && MCNameCreateWithCString("unicode currency symbol", &t_unicode_currency_symbol_key)
                     && MCArrayStoreValue(*t_array, kMCCompareCaseless, *t_unicode_currency_symbol_key, *t_unicode_currency_symbol_string));
    }
    
    MCAutoStringRef t_product_id_string;
    MCStringCreateWithCString(t_product_id, &t_product_id_string);
    MCParameter p1, p2;
    p1.setvalueref_argument(*t_product_id_string);
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
