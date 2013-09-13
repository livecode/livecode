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
#include "mblandroidjava.h"
#include "mblstore.h"

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
    PURCHASED,
    CANCELED,
    REFUNDED,
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

static bool s_can_make_purchase_returned = false;
static bool s_can_make_purchase = false;

////////////////////////////////////////////////////////////////////////

bool MCPurchaseGetProductIdentifier(MCPurchase *p_purchase, MCStringRef& r_identifier);
bool MCPurchaseGetDeveloperPayload(MCPurchase *p_purchase, MCStringRef& r_payload);
bool MCPurchaseGetPurchaseDate(MCPurchase *p_purchase, integer_t& r_date);
bool MCPurchaseGetTransactionIdentifier(MCPurchase *p_purchase, MCStringRef& r_identifier);
bool MCPurchaseGetSignedData(MCPurchase *p_purchase, MCStringRef& r_data);
bool MCPurchaseGetSignature(MCPurchase *p_purchase, MCStringRef& r_signature);

////////////////////////////////////////////////////////////////////////

static MCPurchasePropertyInfo kProperties[] =
{
    DEFINE_RO_PROPERTY(kMCPurchasePropertyDeveloperPayload, String, Purchase, DeveloperPayload)
    DEFINE_RO_PROPERTY(kMCPurchasePropertyProductIdentifier, String, Purchase, ProductIdentifier)
    DEFINE_RO_PROPERTY(kMCPurchasePropertyTransactionIdentifier, String, Purchase, TransactionIdentifier)
    DEFINE_RO_PROPERTY(kMCPurchasePropertyPurchaseDate, Int32, Purchase, PurchaseDate)
    DEFINE_RO_PROPERTY(kMCPurchasePropertySignedData, String, Purchase, SignedData)
    DEFINE_RO_PROPERTY(kMCPurchasePropertySignature, String, Purchase, Signature)
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
    
    s_can_make_purchase_returned = false;
    s_can_make_purchase = false;
    
    MCAndroidEngineRemoteCall("storeCanMakePurchase", "b", &t_result);
    
    while (!s_can_make_purchase_returned)
        MCscreen->wait(60, True, True);
    
    return s_can_make_purchase;
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

#ifdef /* MCPurchaseGet */ LEGACY_EXEC 
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
#endif /* MCPurchaseGet */

///////////////////////////////////////////////////////////////////////////////////////////////

bool MCPurchaseGetProductIdentifier(MCPurchase *p_purchase, MCStringRef& r_identifier)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->product_id == nil)
        return false;
    
	r_identifier = MCValueRetain(t_android_data->product_id);
	return true;
}

bool MCPurchaseGetDeveloperPayload(MCPurchase *p_purchase, MCStringRef& r_payload)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->developer_payload == nil)
        return false;
    
    r_payload = MCValueRetain(t_android_data->developer_payload);
	return true;
}

bool MCPurchaseGetPurchaseDate(MCPurchase *p_purchase, integer_t& r_date)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->purchase_time == nil)
        return false;
    
    r_date = (integer_t)(t_android_data->purchase_time);
    return true;
}

bool MCPurchaseGetTransactionIdentifier(MCPurchase *p_purchase, MCStringRef& r_identifier)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->order_id == nil)
        return false;
    
    r_identifier = MCValueRetain(t_android_data->order_id);
	return true;
}

bool MCPurchaseGetSignedData(MCPurchase *p_purchase, MCStringRef& r_data)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->signed_data == nil)
        return false;
    
	r_data = MCValueRetain(t_android_data->signed_data);
	return true;
}

bool MCPurchaseGetSignature(MCPurchase *p_purchase, MCStringRef& r_signature)
{
    MCAndroidPurchase *t_android_data = (MCAndroidPurchase*)p_purchase->platform_data;
    
    if (t_android_data->signature == nil)
        return false;
    
	r_signature = MCValueRetain(t_android_data->signature);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCPurchaseSet */ LEGACY_EXEC
Exec_stat MCPurchaseSet(MCPurchase *p_purchase, MCPurchaseProperty p_property, uint32_t p_quantity)
{
    /*
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
     */
    return ES_NOT_HANDLED;
}
#endif /* MCPurchaseSet */

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
    
    MCLog("confirming notification: purchaseId=%d, notificationId=%s", p_purchase->id, MCStringGetCString(t_android_data->notification_id));
    MCAndroidEngineRemoteCall("purchaseConfirmDelivery", "bix", &t_result, p_purchase->id, t_android_data->notification_id);
    
    return t_result;
}

bool MCPurchaseConfirmDelivery(MCPurchase *p_purchase)
{
    MCLog("MCPurchaseConfirmDelivery(%p)", p_purchase);
    
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
                case REFUNDED:
                    //MCLog("verified refunded purchase", nil);
                    p_purchase->state = kMCPurchaseStateRefunded;
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
    if (!p_verified)
        p_purchase->state = kMCPurchaseStateUnverified;
    else if (p_state == PURCHASED)
        p_purchase->state = kMCPurchaseStatePaymentReceived;
    else if (p_state == REFUNDED)
        p_purchase->state = kMCPurchaseStateRefunded;
    else
        p_purchase->state = kMCPurchaseStateCancelled;
}

#ifdef LEGACY_EXEC
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
#endif

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doBillingSupported(JNIEnv *env, jobject object, jboolean supported) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doBillingSupported(JNIEnv *env, jobject object, jboolean supported)
{
    s_can_make_purchase_returned = true;
	s_can_make_purchase = supported;
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
    
    MCLog("doPurchaseStateChanged(verified=%s, purchaseState=%d, productId=%s, ...)", verified?"TRUE":"FALSE", purchaseState, MCStringGetCString(*t_product_id));
    
    if (t_success)
    {
        if (!MCPurchaseFindByProductId(*t_product_id, t_purchase))
        {
            MCLog("unrecognized purchase for %s", MCStringGetCString(*t_product_id));
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
            MCLog("found purchase for %s", MCStringGetCString(*t_product_id));
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

////////////////////////////////////////////////////////////////////////////////

bool MCStoreRequestProductDetails(MCStringRef p_product_id)
{
    // Not implemented
    return false;
}
