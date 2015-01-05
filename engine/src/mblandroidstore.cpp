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

#include "mcerror.h"
#include "execpt.h"
#include "globals.h"
#include "stack.h"
#include "card.h"
#include "eventqueue.h"
#include "util.h"

#include "mblandroidutil.h"
#include "core.h"
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
    char *product_id;
    char *developer_payload;
    
    char *signed_data;
    char *signature;
    
    char *notification_id;
    char *order_id;
    int64_t purchase_time;
    int32_t purchase_state;
    
    char *error;
} MCAndroidPurchase;

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

bool MCStoreProductSetType(const char *p_product_id, const char *p_product_type)
{
    
    bool t_result;
    
    MCAndroidEngineRemoteCall("storeProductSetType", "bss", &t_result, p_product_id, p_product_type);
    
    return t_result;
    
}

bool MCStoreSetPurchaseProperty(const char *p_product_id, const char *p_property_name, const char *p_property_value)
{
    
    bool t_result;
    
    MCAndroidEngineRemoteCall("storeSetPurchaseProperty", "bsss", &t_result, p_product_id, p_property_name, p_property_value);
    
    return t_result;
    
}

char* MCStoreGetPurchaseProperty(const char *p_product_id, const char *p_property_name)
{
    char* t_result;
    
    MCAndroidEngineRemoteCall("storeGetPurchaseProperty", "sss", &t_result, p_product_id, p_property_name);
    
    return t_result;
    
}

char* MCStoreGetPurchaseList()
{
    char* t_result;
    
    MCAndroidEngineRemoteCall("storeGetPurchaseList", "s", &t_result);
    
    return t_result;
    
}

bool MCStoreConsumePurchase(const char *p_purchase_id)
{
    
    bool t_result;
    
    MCAndroidEngineRemoteCall("storeConsumePurchase", "bs", &t_result, p_purchase_id);
    
    return t_result;
    
}
 
bool MCStoreMakePurchase(const char *p_prod_id, const char *p_quantity, const char *p_payload)
{
    
    bool t_result;
    
    MCAndroidEngineRemoteCall("storeMakePurchase", "bsss", &t_result, p_prod_id, p_quantity, p_payload);
    
    return t_result;
  
}
  
/*
bool MCStoreMakePurchase(MCPurchase *p_purchase)
{
    if (p_purchase->state != kMCPurchaseStateInitialized)
        return false;
    
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    bool t_success = false;
    
    MCAndroidEngineRemoteCall("storeMakePurchase", "bsss", &t_success, p_purchase->prod_id, "1", t_android_data->developer_payload);
    
    return t_success;
}


char* MCStoreAndroidRequestProductDetails(const char *p_purchase_id)
{
    
    char* t_result;
    
    MCAndroidEngineRemoteCall("storeRequestProductDetails", "ss", &t_result, p_purchase_id);
    
    return t_result;
    
}
*/


// REMOVE THIS METHOD
bool MCStoreRequestForProductDetails(const char *p_purchase_id)
{
    
    bool t_result;
    
    MCAndroidEngineRemoteCall("storeRequestProductDetails", "bs", &t_result, p_purchase_id);
    
    return t_result;
    
}

char* MCStoreReceiveProductDetails(const char *p_purchase_id)
{
    
    char* t_result;
    
    MCAndroidEngineRemoteCall("storeReceiveProductDetails", "ss", &t_result, p_purchase_id);
    
    return t_result;
    
}


////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseFindByProductId(const char *p_product_id, MCPurchase *&r_purchase)
{
    for (MCPurchase *t_purchase = MCStoreGetPurchases(); t_purchase != nil; t_purchase = t_purchase->next)
    {
        MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)t_purchase->platform_data;
        if (MCCStringEqual(p_product_id, t_android_data->product_id))
        {
            r_purchase = t_purchase;
            return true;
        }
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseInit(MCPurchase *p_purchase, const char *p_product_id, void *p_context)
{
    bool t_success = true;
    
    if (p_context != nil)
        p_purchase->platform_data = p_context;
    else
    {
        MCAndroidPurchase *t_android_data = nil;
        
        t_success = p_product_id != nil;
        
        if (t_success)
            t_success = MCMemoryNew(t_android_data);
        if (t_success)
            t_success = MCCStringClone(p_product_id, t_android_data->product_id);
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
    
    MCCStringFree(t_android_data->product_id);
    MCCStringFree(t_android_data->developer_payload);
    MCCStringFree(t_android_data->signed_data);
    MCCStringFree(t_android_data->signature);
    MCCStringFree(t_android_data->notification_id);
    MCCStringFree(t_android_data->order_id);
    MCCStringFree(t_android_data->error);
    
    MCMemoryDelete(t_android_data);
}

Exec_stat MCPurchaseSet(MCPurchase *p_purchase, MCPurchaseProperty p_property, MCExecPoint &ep)
{
    if (p_purchase->state != kMCPurchaseStateInitialized)
        return ES_NOT_HANDLED;
    
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    switch (p_property)
    {
        case kMCPurchasePropertyDeveloperPayload:
        {
            if (ep.getsvalue().getlength() >= 256)
            {
                MCeerror->add(EE_UNDEFINED, 0, 0, ep.getsvalue());
                return ES_ERROR;
            }
            if (t_android_data->developer_payload != nil)
                MCCStringFree(t_android_data->developer_payload);
            MCCStringCloneSubstring(ep.getsvalue().getstring(), ep.getsvalue().getlength(), t_android_data->developer_payload);
            return ES_NORMAL;
        }
        default:
            break;
    }
    
    return ES_NOT_HANDLED;
}

Exec_stat MCPurchaseGet(MCPurchase *p_purchase, MCPurchaseProperty p_property, MCExecPoint &ep)
{
    //MCLog("MCPurchaseGet(%p, %d, ...)", p_purchase, p_property);
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    switch (p_property) {
        case kMCPurchasePropertyProductIdentifier:
            ep.copysvalue(t_android_data->product_id);
            return ES_NORMAL;
            
        case kMCPurchasePropertyDeveloperPayload:
            if (t_android_data->developer_payload == nil)
                ep.clear();
            else
                ep.copysvalue(t_android_data->developer_payload);
            return ES_NORMAL;
            
        case kMCPurchasePropertySignedData:
            if (t_android_data->signed_data == nil)
                ep.clear();
            else
                ep.copysvalue(t_android_data->signed_data);
            return ES_NORMAL;
            
        case kMCPurchasePropertySignature:
            if (t_android_data->signature == nil)
                ep.clear();
            else
                ep.copysvalue(t_android_data->signature);
            return ES_NORMAL;
            
        case kMCPurchasePropertyTransactionIdentifier:
            if (t_android_data->order_id == nil)
                ep.clear();
            else
                ep.copysvalue(t_android_data->order_id);
            return ES_NORMAL;
            
        case kMCPurchasePropertyPurchaseDate:
            ep.setint64(t_android_data->purchase_time);
            return ES_NORMAL;
            
        default:
            break;
    }
    
    return ES_NOT_HANDLED;
}


bool MCPurchaseGetError(MCPurchase *p_purchase, char *&r_error)
{
    if (p_purchase == nil || p_purchase->state != kMCPurchaseStateError)
        return false;
    
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data == nil)
        return false;
    
    return MCCStringClone(t_android_data->error, r_error);
}

////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseSendRequest(MCPurchase *p_purchase)
{
    if (p_purchase->state != kMCPurchaseStateInitialized)
        return false;
    
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    bool t_success = false;
    
    MCAndroidEngineRemoteCall("purchaseSendRequest", "biss", &t_success, p_purchase->id, t_android_data->product_id, t_android_data->developer_payload);
    
    return t_success;
}

static bool purchase_confirm(MCPurchase *p_purchase)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    bool t_result = false;
    
    MCLog("confirming notification: purchaseId=%d, notificationId=%s", p_purchase->id, t_android_data->notification_id);
    MCAndroidEngineRemoteCall("purchaseConfirmDelivery", "bis", &t_result, p_purchase->id, t_android_data->notification_id);
    
    return t_result;
}

bool MCPurchaseConfirmDelivery(MCPurchase *p_purchase)
{
    MCLog("MCPurchaseConfirmDelivery(%p)", p_purchase);
    if (!(p_purchase->state == kMCPurchaseStatePaymentReceived || p_purchase->state == kMCPurchaseStateRefunded || p_purchase->state == kMCPurchaseStateRestored))
        return false;
    
    purchase_confirm(p_purchase);
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
            MCCStringClone("unable to verify message from billing service", t_android_data->error);
            MCPurchaseNotifyUpdate(p_purchase);
            MCPurchaseRelease(p_purchase);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void update_purchase_state(MCPurchase *p_purchase, int32_t p_state, bool p_verified)
{
    MCLog("State is " + p_state, nil);
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

bool MCCStringFromJava(JNIEnv *env, jstring p_jstring, char *&r_cstring)
{
    bool t_success = true;
    
    if (p_jstring == NULL)
    {
        r_cstring = NULL;
        return true;
    }
    
    const char *t_chars = nil;
    
    t_chars = env->GetStringUTFChars(p_jstring, NULL);
    t_success = t_chars != NULL;
    
    if (t_success)
        t_success = MCCStringClone(t_chars, r_cstring);
    
    if (t_chars != NULL)
        env->ReleaseStringUTFChars(p_jstring, t_chars);
    
    return t_success;
}

void MCCStringReplace(char *&src, char *&dest)
{
    if (dest != NULL)
        MCCStringFree(dest);
    dest = src;
    src = NULL;
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
    
    char *t_notification_id = nil;
    char *t_product_id = nil;
    char *t_order_id = nil;
    char *t_developer_payload = nil;
    char *t_signed_data = nil;
    char *t_signature = nil;
    
    t_success = MCCStringFromJava(env, notificationId, t_notification_id) && \
    MCCStringFromJava(env, productId, t_product_id) && \
    MCCStringFromJava(env, orderId, t_order_id) && \
    MCCStringFromJava(env, developerPayload, t_developer_payload) && \
    MCCStringFromJava(env, signedData, t_signed_data) && \
    MCCStringFromJava(env, signature, t_signature);
    
    MCLog("doPurchaseStateChanged(verified=%s, purchaseState=%d, productId=%s, ...)", verified?"TRUE":"FALSE", purchaseState, t_product_id);
    
    if (t_success)
    {
        if (!MCPurchaseFindByProductId(t_product_id, t_purchase))
        {
            MCLog("unrecognized purchase for %s", t_product_id);
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
            
            MCLog("found purchase for %s", t_product_id);
            
            // THIS WAS ADDED 
            t_purchase->prod_id = t_product_id;
            
            MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)t_purchase->platform_data;
            
            MCCStringReplace(t_product_id, t_android_data->product_id);
            MCCStringReplace(t_notification_id, t_android_data->notification_id);
            MCCStringReplace(t_order_id, t_android_data->order_id);
            MCCStringReplace(t_developer_payload, t_android_data->developer_payload);
            MCCStringReplace(t_signed_data, t_android_data->signed_data);
            MCCStringReplace(t_signature, t_android_data->signature);
            
            t_android_data->purchase_time = purchaseTime;
            t_android_data->purchase_state = purchaseState;
            
            update_purchase_state(t_purchase, purchaseState, verified);
            MCLog("ProductID is %s", t_purchase->prod_id);
            MCPurchaseNotifyUpdate(t_purchase);
            
            // now, if the purchase is cancelled, confirm the notification
            if (t_purchase->state == kMCPurchaseStateCancelled)
            {
                MCLog("purchase canceled, confirming notification", nil);
                purchase_confirm(t_purchase);
            }
        }
    }
    
    if (!t_success);
    {
        MCCStringFree(t_notification_id);
        MCCStringFree(t_product_id);
        MCCStringFree(t_order_id);
        MCCStringFree(t_developer_payload);
        MCCStringFree(t_signed_data);
        MCCStringFree(t_signature);
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
            MCCStringClone("requested item unavailable", t_android_data->error);
            MCPurchaseNotifyUpdate(t_purchase);
            MCPurchaseRelease(t_purchase);            
        }
        else if (responseCode == RESULT_DEVELOPER_ERROR)
        {
            t_purchase->state = kMCPurchaseStateError;
            MCCStringClone("developer error", t_android_data->error);
            MCPurchaseNotifyUpdate(t_purchase);
            MCPurchaseRelease(t_purchase);            
        }
        else if (responseCode == RESULT_ERROR)
        {
            t_purchase->state = kMCPurchaseStateError;
            MCCStringClone("sending purchase request failed", t_android_data->error);
            MCPurchaseNotifyUpdate(t_purchase);
            MCPurchaseRelease(t_purchase);
        }
    }
}

bool MCStorePostProductRequestResponse(const char *p_product_id);
bool MCStorePostProductRequestError(const char *p_product, const char *p_error);


extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProductDetailsResponse(JNIEnv *env, jobject object, jstring productId) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProductDetailsResponse(JNIEnv *env, jobject object, jstring productId)
{
    bool t_success = true;

    char *t_product_id = nil;
    t_success = MCCStringFromJava(env, productId, t_product_id);
    if (t_success)
        t_success = MCStorePostProductRequestResponse(t_product_id);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProductDetailsError(JNIEnv *env, jobject object, jstring productId, jstring error) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doProductDetailsError(JNIEnv *env, jobject object, jstring productId, jstring error)
{
    bool t_success = true;
    
    char *t_product_id = nil;
    char *t_error = nil;
    t_success = MCCStringFromJava(env, productId, t_product_id) && MCCStringFromJava(env, error, t_error);
    if (t_success)
        t_success = MCStorePostProductRequestError(t_product_id, t_error);
}

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

Exec_stat MCHandlePurchaseVerify(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandlePurchaseVerify */ LEGACY_EXEC
    bool t_success = true;
    
    bool t_verified = true;
    uint32_t t_id;
    MCPurchase *t_purchase = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "ub", &t_id, &t_verified);
    
    if (t_success)
        t_success = MCPurchaseFindById(t_id, t_purchase);
    
    if (t_success)
        MCPurchaseVerify(t_purchase, t_verified);
    
    return ES_NORMAL;
#endif /* MCHandlePurchaseVerify */
}

///////////////////////////////////////////////////////////////

class MCStoreProductRequestResponseEvent : public MCCustomEvent
{
public:
    MCStoreProductRequestResponseEvent(const char *p_product_id);
    
    void Dispatch();
    void Destroy();
    
private:
    char *m_product_id;
};

MCStoreProductRequestResponseEvent::MCStoreProductRequestResponseEvent(const char *p_product_id)
{
    m_product_id = nil;
    MCCStringClone(p_product_id, m_product_id);
}

void MCStoreProductRequestResponseEvent::Destroy()
{
    MCCStringFree(m_product_id);
    
    delete this;
}



void MCStoreProductRequestResponseEvent::Dispatch()
{
    bool t_success = true;
    
    const char *t_product_id = nil;
    const char *t_description = nil;
    const char *t_title = nil;
    const char *t_itemType = nil;
    const char *t_price = nil;
    const char *t_itemImageUrl = nil;
    const char *t_itemDownloadUrl = nil;
    const char *t_subscriptionDurationUnit = nil;
    const char *t_subscriptionDurationMultiplier= nil;
    
    //t_product_id = m_product_id;
    t_product_id = MCStoreGetPurchaseProperty(m_product_id, "productId");
    t_description = MCStoreGetPurchaseProperty(m_product_id, "description");
    t_title = MCStoreGetPurchaseProperty(m_product_id, "title");
    t_itemType = MCStoreGetPurchaseProperty(m_product_id, "itemType");
    t_price = MCStoreGetPurchaseProperty(m_product_id, "price");
    t_itemImageUrl = MCStoreGetPurchaseProperty(m_product_id, "itemImageUrl");
    t_itemDownloadUrl = MCStoreGetPurchaseProperty(m_product_id, "itemDownloadUrl");
    t_subscriptionDurationUnit = MCStoreGetPurchaseProperty(m_product_id, "subscriptionDurationUnit");
    t_subscriptionDurationMultiplier = MCStoreGetPurchaseProperty(m_product_id, "subscriptionDurationMultiplier");
    
    if (t_success)
    {
        MCExecPoint ep(nil, nil, nil);
        
        MCVariableValue *t_response = nil;
        t_response = new MCVariableValue();
        
        MCVariableValue *t_element = nil;
        
        if (t_product_id != nil)
        {
            t_response->lookup_element(ep, "productId", t_element);
            t_element->assign_string(MCString(t_product_id));
        }
      
        if (t_price != nil)
        {
            t_response->lookup_element(ep, "price", t_element);
            t_element->assign_string(MCString(t_price));
        }
        
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
        
        if (t_itemType != nil)
        {
            t_response->lookup_element(ep, "itemType", t_element);
            t_element->assign_string(MCString(t_itemType));
        }
        
        if (t_itemImageUrl != nil)
        {
            t_response->lookup_element(ep, "itemImageUrl", t_element);
            t_element->assign_string(MCString(t_itemImageUrl));
        }
        
        if (t_itemDownloadUrl != nil)
        {
            t_response->lookup_element(ep, "itemDownloadUrl", t_element);
            t_element->assign_string(MCString(t_itemDownloadUrl));
        }
        
        if (t_subscriptionDurationUnit != nil)
        {
            t_response->lookup_element(ep, "subscriptionDurationUnit", t_element);
            t_element->assign_string(MCString(t_subscriptionDurationUnit));
        }
        
        if (t_subscriptionDurationMultiplier != nil)
        {
            t_response->lookup_element(ep, "subscriptionDurationMultiplier", t_element);
            t_element->assign_string(MCString(t_subscriptionDurationMultiplier));
        }
        
        ep.setarray(t_response, True);
        
        MCParameter p1, p2;
        p1.sets_argument(MCString(t_product_id));
        p1.setnext(&p2);
        p2.set_argument(ep);
        
        MCdefaultstackptr->getcurcard()->message(MCM_product_details_received, &p1);
    }
    
}

bool MCStorePostProductRequestResponse(const char *p_product_id)
{
    bool t_success;
    MCCustomEvent *t_event = nil;
    t_event = new MCStoreProductRequestResponseEvent(p_product_id);
    t_success = t_event != nil;
    
    if (t_success)
        MCEventQueuePostCustom(t_event);
    
    return t_success;
}

////////

class MCStoreProductRequestErrorEvent : public MCCustomEvent
{
public:
    MCStoreProductRequestErrorEvent(const char *p_product, const char *p_error);
    
    void Destroy();
    void Dispatch();
    
private:
    char *m_product;
    char *m_error;
};

MCStoreProductRequestErrorEvent::MCStoreProductRequestErrorEvent(const char *p_product, const char *p_error)
{
    m_product = m_error = nil;
    
    MCCStringClone(p_product, m_product);
    MCCStringClone(p_error, m_error);
}

void MCStoreProductRequestErrorEvent::Destroy()
{
    MCCStringFree(m_product);
    MCCStringFree(m_error);
    
    delete this;
}

void MCStoreProductRequestErrorEvent::Dispatch()
{
    MCdefaultstackptr->getcurcard()->message_with_args(MCM_product_request_error, m_product, m_error);
}

bool MCStorePostProductRequestError(const char *p_product, const char *p_error)
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


Exec_stat MCHandleRequestProductDetails(void *context, MCParameter *p_parameters)
{
    bool t_success = true;
    char *t_product_id = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "s", &t_product_id);
    if (t_success)
        t_success = MCStoreRequestProductDetails(t_product_id);
    
    MCCStringFree(t_product_id);
    
    return ES_NORMAL;
}

bool MCStoreRequestProductDetails(const char *p_product_id)
{
    
    bool t_result;
    
    MCAndroidEngineRemoteCall("storeRequestProductDetails", "bs", &t_result, p_product_id);
    
    return t_result;
    
}

