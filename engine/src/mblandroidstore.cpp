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

#include "mcerror.h"

#include "globals.h"
#include "stack.h"
#include "card.h"
#include "eventqueue.h"
#include "util.h"

#include "mblandroidutil.h"
#include "mblandroidjava.h"
#include "mblstore.h"

#include "param.h"
#include <jni.h>

typedef enum
{
    RESULT_OK,
    RESULT_USER_CANCELED,
    RESULT_SERVICE_UNAVAILABLE,
    RESULT_BILLING_UNAVAILABLE,
    RESULT_ITEM_UNAVAILABLE,
    RESULT_DEVELOPER_ERROR,
    RESULT_ERROR,
} MCAndroidResponseCode;

typedef enum
{
    PURCHASED, //SUCCESSFUL for Amazon
    CANCELED,  //FAILED for Amazon
    INVALID_SKU,
    ALREADY_ENTITLED,
    REFUNDED,
    RESTORED,
} MCAndroidPurchaseState;

typedef struct
{
    MCStringRef product_id;
    MCStringRef developer_payload;
    
    MCStringRef signed_data;
    MCStringRef signature;
    
    MCStringRef notification_id;
    MCStringRef order_id;
    int64_t purchase_time;
    int32_t purchase_state;
    
    MCStringRef error;
} MCAndroidPurchase;

////////////////////////////////////////////////////////////////////////

void MCPurchaseGetProductIdentifier(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_identifier);
void MCPurchaseGetDeveloperPayload(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_payload);
void MCPurchaseGetPurchaseDate(MCExecContext& ctxt,MCPurchase *p_purchase, integer_t& r_date);
void MCPurchaseGetTransactionIdentifier(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_identifier);
void MCPurchaseGetSignedData(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_data);
void MCPurchaseGetSignature(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_signature);

////////////////////////////////////////////////////////////////////////

static MCPropertyInfo kProperties[] =
{
    DEFINE_RO_STORE_PROPERTY(P_DEVELOPER_PAYLOAD, String, Purchase, DeveloperPayload)
    DEFINE_RO_STORE_PROPERTY(P_PRODUCT_IDENTIFIER, String, Purchase, ProductIdentifier)
    DEFINE_RO_STORE_PROPERTY(P_TRANSACTION_IDENTIFIER, String, Purchase, TransactionIdentifier)
    DEFINE_RO_STORE_PROPERTY(P_PURCHASE_DATE, Int32, Purchase, PurchaseDate)
    DEFINE_RO_STORE_PROPERTY(P_SIGNED_DATA, String, Purchase, SignedData)
    DEFINE_RO_STORE_PROPERTY(P_SIGNATURE, String, Purchase, Signature)
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

bool MCStoreCanMakePurchase()
{
    bool t_result = false;
    
    MCAndroidEngineRemoteCall("storeCanMakePurchase", "b", &t_result);
    
    // PM-2015-01-05: [[ Bug 14285 ]] Removed code. The bool variable that indicates if in-app billing is supported has already been initialised in initBilling() method, so no need to block-wait
    return t_result;
}

bool MCStoreEnablePurchaseUpdates()
{
    bool t_enable = true;
    
    MCAndroidEngineRemoteCall("storeSetUpdates", "vb", NULL, t_enable);
    
    return true;
}

bool MCStoreDisablePurchaseUpdates()
{
    bool t_enable = false;
    
    MCAndroidEngineRemoteCall("storeSetUpdates", "vb", NULL, t_enable);
    
    return true;
}

bool MCStoreRestorePurchases()
{
    bool t_result = false;
    
    MCAndroidEngineRemoteCall("storeRestorePurchases", "b", &t_result);
    
    return t_result;
}

bool MCStoreMakePurchase(MCStringRef p_product_id, MCStringRef p_quantity, MCStringRef p_payload)
{    
    bool t_success = false;
    
    MCAndroidEngineRemoteCall("storeMakePurchase", "bxxx", &t_success, p_product_id, p_quantity, p_payload);
    
    return t_success;
}

bool MCStoreReceiveProductDetails(MCStringRef p_purchase_id, MCStringRef &r_result)
{    
    MCAutoStringRef t_result;
    
    MCAndroidEngineRemoteCall("storeReceiveProductDetails", "xx", &(&t_result), p_purchase_id);
    
    return MCStringCopy(*t_result, r_result);    
}

bool MCStoreConsumePurchase(MCStringRef p_product_id)
{
    bool t_result;
    
    MCAndroidEngineRemoteCall("storeConsumePurchase", "bx", &t_result, p_product_id);
                              
    return t_result;
}

bool MCStoreProductSetType(MCStringRef p_product_id, MCStringRef p_product_type)
{
    bool t_result;
    
    MCAndroidEngineRemoteCall("storeProductSetType", "bxx", &t_result, p_product_id, p_product_type);
    
    return t_result;
}

////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseFindByProductId(MCStringRef p_product_id, MCPurchase *&r_purchase) 
{
    for (MCPurchase *t_purchase = MCStoreGetPurchases(); t_purchase != nil; t_purchase = t_purchase->next)
    {
        MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)t_purchase->platform_data;
        if (MCStringIsEqualTo(p_product_id, t_android_data->product_id, kMCCompareExact))
        {
            r_purchase = t_purchase;
            return true;
        }
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseInit(MCPurchase *p_purchase, MCStringRef p_product_id, void *p_context)
{
    bool t_success = true;
    
    if (p_context != nil)
        p_purchase->platform_data = p_context;
    else
    {
        MCAndroidPurchase *t_android_data = nil;
        
        t_success = MCStringGetLength(p_product_id) != 0;
        
        if (t_success)
            t_success = MCMemoryNew(t_android_data);
        if (t_success)
			t_android_data->product_id = MCValueRetain(p_product_id);
        if (t_success)
            p_purchase->platform_data = t_android_data;
        else
            MCMemoryDelete(t_android_data);
    }
    
    return t_success;
}

void MCPurchaseFinalize(MCPurchase *p_purchase)
{
    if (p_purchase == nil)
        return;
    
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data == nil)
        return;
    
    MCValueRelease(t_android_data->product_id);
    MCValueRelease(t_android_data->developer_payload);
    MCValueRelease(t_android_data->signed_data);
    MCValueRelease(t_android_data->signature);
    MCValueRelease(t_android_data->notification_id);
    MCValueRelease(t_android_data->order_id);
    MCValueRelease(t_android_data->error);
    
    MCMemoryDelete(t_android_data);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void MCPurchaseGetProductIdentifier(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_identifier)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->product_id != nil)
    {
        r_identifier = MCValueRetain(t_android_data->product_id);
        return;
    }
    
    ctxt.Throw();
}

void MCPurchaseGetDeveloperPayload(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_payload)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->developer_payload != nil)
    {
        r_payload = MCValueRetain(t_android_data->developer_payload);
        return;
    }
    
    ctxt.Throw();
}

void MCPurchaseGetPurchaseDate(MCExecContext& ctxt,MCPurchase *p_purchase, integer_t& r_date)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->purchase_time != 0)
    {
        r_date = (integer_t)(t_android_data->purchase_time);
        return;
    }
    
    ctxt.Throw();
}

void MCPurchaseGetTransactionIdentifier(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_identifier)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->order_id != nil)
    {
        r_identifier = MCValueRetain(t_android_data->order_id);
        return;
    }
    
    ctxt.Throw();
}

void MCPurchaseGetSignedData(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_data)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->signed_data != nil)
    {
        r_data = MCValueRetain(t_android_data->signed_data);
        return;
    }
    
    ctxt.Throw();
}

void MCPurchaseGetSignature(MCExecContext& ctxt,MCPurchase *p_purchase, MCStringRef& r_signature)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->signature != nil)
    {
        r_signature = MCValueRetain(t_android_data->signature);
        return;
    }
    
    ctxt.Throw();
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseGetError(MCPurchase *p_purchase, MCStringRef& r_error)
{
    if (p_purchase == nil || p_purchase->state != kMCPurchaseStateError)
        return false;
    
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data == nil)
        return false;
    
    r_error = MCValueRetain(t_android_data->error);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseSendRequest(MCPurchase *p_purchase)
{
    if (p_purchase->state != kMCPurchaseStateInitialized)
        return false;
    
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    bool t_success = false;
    
    MCAndroidEngineRemoteCall("purchaseSendRequest", "bixx", &t_success, p_purchase->id, t_android_data->product_id, t_android_data->developer_payload);
    
    return t_success;
}

static bool purchase_confirm(MCPurchase *p_purchase)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    bool t_result = false;
    
    MCLog("confirming notification: purchaseId=%d, notificationId=%@", p_purchase->id, t_android_data->notification_id);
    MCAndroidEngineRemoteCall("purchaseConfirmDelivery", "bix", &t_result, p_purchase->id, t_android_data->notification_id);
    
    if (t_result)
    {
        // PM-2015-03-04: [[ Bug 14779 ]] Send a purchaseStateUpdate msg with state=complete
        p_purchase->state = kMCPurchaseStateComplete;
        MCPurchaseCompleteListUpdate(p_purchase);
        MCPurchaseNotifyUpdate(p_purchase);
        MCPurchaseRelease(p_purchase);
    }
    
    return t_result;
}

bool MCPurchaseConfirmDelivery(MCPurchase *p_purchase)
{
    MCLog("MCPurchaseConfirmDelivery(%p)", p_purchase);
    
    return purchase_confirm(p_purchase);
}

////////////////////////////////////////////////////////////////////////////////

void MCPurchaseVerify(MCPurchase *p_purchase, bool p_verified)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    if (p_purchase->state == kMCPurchaseStateUnverified)
    {
        if (p_verified)
        {
            switch (t_android_data->purchase_state) {
                case PURCHASED:
                    p_purchase->state = kMCPurchaseStatePaymentReceived;
                    break;
                    
                case CANCELED:
                {
                    //MCLog("verified canceled purchase", nil);
                    p_purchase->state = kMCPurchaseStateCancelled;
                    purchase_confirm(p_purchase);
                    break;
                }
                
                case ALREADY_ENTITLED:
                {
                    MCLog("found ALREADY_ENTITLED purchase", nil);
                    p_purchase->state = kMCPurchaseStateAlreadyEntitled;
                    break;
                }
                case INVALID_SKU:
                    p_purchase->state = kMCPurchaseStateInvalidSKU;
                    break;
                    
                case REFUNDED:
                    //MCLog("verified refunded purchase", nil);
                    p_purchase->state = kMCPurchaseStateRefunded;
                    break;
                    
                case RESTORED:
                    p_purchase->state = kMCPurchaseStateRestored;
                    break;
                    
                default:
                    break;
            }
            MCPurchaseNotifyUpdate(p_purchase);
        }
        else
        {
            p_purchase->state = kMCPurchaseStateError;

            t_android_data->error = MCValueRetain(MCSTR("unable to verify message from billing service"));                                                 
			MCPurchaseNotifyUpdate(p_purchase);

            MCPurchaseRelease(p_purchase);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void update_purchase_state(MCPurchase *p_purchase, int32_t p_state, bool p_verified)
{
    MCLog("State is %d", p_state);
    if (!p_verified)
        p_purchase->state = kMCPurchaseStateUnverified;
    else if (p_state == PURCHASED)
        p_purchase->state = kMCPurchaseStatePaymentReceived;
    else if (p_state == ALREADY_ENTITLED)
        p_purchase->state = kMCPurchaseStateAlreadyEntitled;
    else if (p_state == INVALID_SKU)
        p_purchase->state = kMCPurchaseStateInvalidSKU;
    else if (p_state == REFUNDED)
        p_purchase->state = kMCPurchaseStateRefunded;
    else if (p_state == RESTORED)
        p_purchase->state = kMCPurchaseStateRestored;
    else
        p_purchase->state = kMCPurchaseStateCancelled;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRestoreTransactionsResponse(JNIEnv *env, jobject object, jint responseCode) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRestoreTransactionsResponse(JNIEnv *env, jobject object, jint responseCode)
{
    // TODO - handle errors
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPurchaseStateChanged(JNIEnv *env, jobject object,
                                                                                        jboolean verified, jint purchaseState,
                                                                                        jstring notificationId,
                                                                                        jstring productId, jstring orderid,
                                                                                        jlong purchaseTime, jstring developerPayload,
                                                                                        jstring signedData, jstring signature) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doPurchaseStateChanged(JNIEnv *env, jobject object,
                                                                             jboolean verified, jint purchaseState,
                                                                             jstring notificationId,
                                                                             jstring productId, jstring orderId,
                                                                             jlong purchaseTime, jstring developerPayload,
                                                                             jstring signedData, jstring signature)
{
    bool t_success;
    MCPurchase *t_purchase;
    
    MCAutoStringRef t_notification_id;
    MCAutoStringRef t_product_id;
    MCAutoStringRef t_order_id;
    MCAutoStringRef t_developer_payload;
    MCAutoStringRef t_signed_data;
    MCAutoStringRef t_signature;
    
    t_success = MCJavaStringToStringRef(env, notificationId, &t_notification_id) && \
    MCJavaStringToStringRef(env, productId, &t_product_id) && \
    MCJavaStringToStringRef(env, orderId, &t_order_id) && \
    MCJavaStringToStringRef(env, developerPayload, &t_developer_payload) && \
    MCJavaStringToStringRef(env, signedData, &t_signed_data) && \
    MCJavaStringToStringRef(env, signature, &t_signature);
    
    MCLog("doPurchaseStateChanged(verified=%s, purchaseState=%d, productId=%@, ...)", verified?"TRUE":"FALSE", purchaseState, *t_product_id);
    
    if (t_success)
    {
        if (!MCPurchaseFindByProductId(*t_product_id, t_purchase))
        {
            MCLog("unrecognized purchase for %@", *t_product_id);
            bool t_success = true;
            MCAndroidPurchase *t_android_data = nil;
            
            t_success = MCMemoryNew(t_android_data);
            
            if (t_success)
                t_success = MCPurchaseCreate(nil, t_android_data, t_purchase);
            
            if (!t_success)
                MCMemoryDelete(t_android_data);
        }

        if (t_purchase != NULL)
        {
            MCLog("found purchase for %@", *t_product_id);
            
            // THIS WAS ADDED
            t_purchase->prod_id = MCValueRetain(*t_product_id);
            
            MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)t_purchase->platform_data;
            
			t_android_data->product_id = MCValueRetain(*t_product_id);
			t_android_data->notification_id = MCValueRetain(*t_notification_id);
			t_android_data->order_id = MCValueRetain(*t_order_id);
			t_android_data->developer_payload = MCValueRetain(*t_developer_payload);
			t_android_data->signed_data = MCValueRetain(*t_signed_data);
			t_android_data->signature = MCValueRetain(*t_signature);
            
            t_android_data->purchase_time = purchaseTime;
            t_android_data->purchase_state = purchaseState;
            
            update_purchase_state(t_purchase, purchaseState, verified);
            MCLog("ProductID is %@", t_purchase->prod_id);
            MCPurchaseNotifyUpdate(t_purchase);
            
            // now, if the purchase is cancelled, confirm the notification
            if (t_purchase->state == kMCPurchaseStateCancelled)
            {
                MCLog("purchase canceled, confirming notification", nil);
                purchase_confirm(t_purchase);
            }
        }
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doConfirmNotificationResponse(JNIEnv *env, jobject object,
                                                                                               jint purchaseId, jint responseCode) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doConfirmNotificationResponse(JNIEnv *env, jobject object,
                                                                                    jint purchaseId, jint responseCode)
{
    MCLog("doConfirmNotificationResponse(purchaseId=%d, responseCode=%d", purchaseId, responseCode);
    MCPurchase *t_purchase = NULL;
    
    if (MCPurchaseFindById(purchaseId, t_purchase))
    {
        if (responseCode == RESULT_OK)
        {
            switch (t_purchase->state)
            {
                case kMCPurchaseStatePaymentReceived:
                case kMCPurchaseStateRefunded:
                case kMCPurchaseStateRestored:
                    t_purchase->state = kMCPurchaseStateComplete;
                    MCPurchaseNotifyUpdate(t_purchase);
                    break;
                case kMCPurchaseStateCancelled:
                    break;
            }
            MCPurchaseRelease(t_purchase);
        }
        else
        {
            // TODO - handle errors
        }
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRequestPurchaseResponse(JNIEnv *env, jobject object,
                                                                                           jint purchaseId, jint responseCode) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRequestPurchaseResponse(JNIEnv *env, jobject object,
                                                                                jint purchaseId, jint responseCode)
{
    MCLog("doRequestPurchaseResponse(purchaseId=%d, responseCode=%d", purchaseId, responseCode);
    
    MCPurchase *t_purchase = NULL;
    
    if (MCPurchaseFindById(purchaseId, t_purchase))
    {
        MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)t_purchase->platform_data;
        if (responseCode == RESULT_OK)
        {
            // seems to be sent after purchase updates.
        }
        else if (responseCode == RESULT_USER_CANCELED)
        {
            t_purchase->state = kMCPurchaseStateCancelled;
            MCPurchaseNotifyUpdate(t_purchase);
            MCPurchaseRelease(t_purchase);
        }
        else if (responseCode == RESULT_ITEM_UNAVAILABLE)
        {
            t_purchase->state = kMCPurchaseStateError;
            t_android_data->error = MCSTR("requested item unavailable");
            MCPurchaseNotifyUpdate(t_purchase);
            MCPurchaseRelease(t_purchase);            
        }
        else if (responseCode == RESULT_DEVELOPER_ERROR)
        {
            t_purchase->state = kMCPurchaseStateError;
			t_android_data->error = MCSTR("developer error");
            MCPurchaseNotifyUpdate(t_purchase);
            MCPurchaseRelease(t_purchase);            
        }
        else if (responseCode == RESULT_ERROR)
        {
            t_purchase->state = kMCPurchaseStateError;
			t_android_data->error = MCSTR("sending purchase request failed");
            MCPurchaseNotifyUpdate(t_purchase);
            MCPurchaseRelease(t_purchase);
        }
    }
}

bool MCStorePostProductRequestResponse(MCStringRef p_product_id);
bool MCStorePostProductRequestError(MCStringRef p_product, MCStringRef p_error);


extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProductDetailsResponse(JNIEnv *env, jobject object, jstring productId) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProductDetailsResponse(JNIEnv *env, jobject object, jstring productId)
{
    bool t_success = true;

    MCAutoStringRef t_product_id;
    
    t_success = MCJavaStringToStringRef(env, productId, &t_product_id);
    if (t_success)
        t_success = MCStorePostProductRequestResponse(*t_product_id);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProductDetailsError(JNIEnv *env, jobject object, jstring productId, jstring error) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProductDetailsError(JNIEnv *env, jobject object, jstring productId, jstring error)
{
    bool t_success = true;
    
    MCAutoStringRef t_product_id;
    MCAutoStringRef t_error;
    
    t_success = MCJavaStringToStringRef(env, productId, &t_product_id) && MCJavaStringToStringRef(env, error, &t_error);
    if (t_success)
        t_success = MCStorePostProductRequestError(*t_product_id, *t_error);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStoreRequestProductDetails(MCStringRef p_product_id)
{
    // PM-2015-01-07: [[ Bug 14343 ]] Implement "mobileStoreRequestProductDetails" for LC 7.0.x
    bool t_result;
    
    MCAndroidEngineRemoteCall("storeRequestProductDetails", "bx", &t_result, p_product_id);
    
    return t_result;
}

void MCStoreSetPurchaseProperty(MCExecContext& ctxt, MCStringRef p_product_id, MCStringRef p_property_name, MCStringRef p_property_value)
{
    bool t_success;
    
    MCAndroidEngineRemoteCall("storeSetPurchaseProperty", "bxxx", &t_success, p_product_id, p_property_name, p_property_value);
    
    if(!t_success)
        ctxt.Throw();
    
}

void MCStoreGetPurchaseProperty(MCExecContext& ctxt, MCStringRef p_product_id, MCStringRef p_property_name, MCStringRef& r_property_value)
{
    MCAutoStringRef t_result;
    
    MCAndroidEngineRemoteCall("storeGetPurchaseProperty", "xxx", &(&t_result), p_product_id, p_property_name);
    
    if(!MCStringCopy(*t_result, r_property_value))
        ctxt.Throw();
}

///////////////////////////////////////////////////////////////

class MCStoreProductRequestResponseEvent : public MCCustomEvent
{
public:
    MCStoreProductRequestResponseEvent(MCStringRef p_product_id);
    
    void Dispatch();
    void Destroy();
    
private:
    MCAutoStringRef m_product_id;
};

MCStoreProductRequestResponseEvent::MCStoreProductRequestResponseEvent(MCStringRef p_product_id)
    : m_product_id(p_product_id)
{
}

void MCStoreProductRequestResponseEvent::Destroy()
{
    delete this;
}


void MCStoreProductRequestResponseEvent::Dispatch()
{
    // PM-2015-01-07: We fetch these values from the store listing
    MCExecContext ctxt(nil,nil,nil);
    MCAutoStringRef t_product_id;
    MCAutoStringRef t_description;
    MCAutoStringRef t_title;
    MCAutoStringRef t_itemType;
    MCAutoStringRef t_price;
    MCAutoStringRef t_itemImageUrl;
    MCAutoStringRef t_itemDownloadUrl;
    MCAutoStringRef t_subscriptionDurationUnit;
    MCAutoStringRef t_subscriptionDurationMultiplier;
    
    MCStoreGetPurchaseProperty(ctxt, *m_product_id, MCSTR("productId"), &t_product_id);
    MCStoreGetPurchaseProperty(ctxt, *m_product_id, MCSTR("description"), &t_description);
    MCStoreGetPurchaseProperty(ctxt, *m_product_id, MCSTR("title"), &t_title);
    MCStoreGetPurchaseProperty(ctxt, *m_product_id, MCSTR("itemType"), &t_itemType);
    MCStoreGetPurchaseProperty(ctxt, *m_product_id, MCSTR("price"), &t_price);
    MCStoreGetPurchaseProperty(ctxt, *m_product_id, MCSTR("itemImageUrl"), &t_itemImageUrl);
    MCStoreGetPurchaseProperty(ctxt, *m_product_id, MCSTR("itemDownloadUrl"), &t_itemDownloadUrl);
    MCStoreGetPurchaseProperty(ctxt, *m_product_id, MCSTR("subscriptionDurationUnit"), &t_subscriptionDurationUnit);
    MCStoreGetPurchaseProperty(ctxt, *m_product_id, MCSTR("subscriptionDurationMultiplier"), &t_subscriptionDurationMultiplier);
    
    
    MCAutoArrayRef t_array;
    MCArrayCreateMutable(&t_array);
    
    MCArrayStoreValue(*t_array, false, MCNAME("productId"), *t_product_id);
    MCArrayStoreValue(*t_array, false, MCNAME("description"), *t_description);
    MCArrayStoreValue(*t_array, false, MCNAME("title"), *t_title);
    MCArrayStoreValue(*t_array, false, MCNAME("itemType"), *t_itemType);
    MCArrayStoreValue(*t_array, false, MCNAME("price"), *t_price);
    MCArrayStoreValue(*t_array, false, MCNAME("itemImageUrl"), *t_itemImageUrl);
    MCArrayStoreValue(*t_array, false, MCNAME("itemDownloadUrl"), *t_itemDownloadUrl);
    MCArrayStoreValue(*t_array, false, MCNAME("subscriptionDurationUnit"), *t_subscriptionDurationUnit);
    MCArrayStoreValue(*t_array, false, MCNAME("subscriptionDurationMultiplier"), *t_subscriptionDurationMultiplier);
    
    MCParameter p1, p2;
    p1.setvalueref_argument(*m_product_id);
    p1.setnext(&p2);
    p2.setvalueref_argument(*t_array);
    
    MCdefaultstackptr->getcurcard()->message(MCM_product_details_received, &p1);
}

bool MCStorePostProductRequestResponse(MCStringRef p_product_id)
{
    bool t_success;
    MCCustomEvent *t_event = nil;
    t_event = new (nothrow) MCStoreProductRequestResponseEvent(p_product_id);
    t_success = t_event != nil;
    
    if (t_success)
        MCEventQueuePostCustom(t_event);
    
    return t_success;
}

////////

class MCStoreProductRequestErrorEvent : public MCCustomEvent
{
public:
    MCStoreProductRequestErrorEvent(MCStringRef p_product, MCStringRef p_error);
    
    void Destroy();
    void Dispatch();
    
private:
    MCAutoStringRef m_product;
    MCAutoStringRef m_error;
};

MCStoreProductRequestErrorEvent::MCStoreProductRequestErrorEvent(MCStringRef p_product_id, MCStringRef p_error)
    : m_product(p_product_id), m_error(p_error)
{
}

void MCStoreProductRequestErrorEvent::Destroy()
{
    delete this;
}

void MCStoreProductRequestErrorEvent::Dispatch()
{
    MCdefaultstackptr->getcurcard()->message_with_valueref_args(MCM_product_request_error, *m_product, *m_error);
}

bool MCStorePostProductRequestError(MCStringRef p_product, MCStringRef p_error)
{
    bool t_success;
    MCCustomEvent *t_event = nil;
    t_event = new (nothrow) MCStoreProductRequestErrorEvent(p_product, p_error);
    t_success = t_event != nil;
    
    if (t_success)
        MCEventQueuePostCustom(t_event);
    
    return t_success;
}

