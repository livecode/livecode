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

package com.runrev.android.billing.amazon;

import com.runrev.android.billing.*;
import com.amazon.inapp.purchasing.*;
import java.util.*;
import android.util.*;
import android.os.*;
import android.app.Activity;


public class MyPurchasingObserver extends BasePurchasingObserver
{
    
    private boolean rvsProductionMode = false;
    private String currentUserID = null;
    public Map<String, String> requestIds;
    
    private static final String TAG = "IAPPurchasingObserver";
    private PurchaseObserver mPurchaseObserver;
    
    public MyPurchasingObserver(Activity iapActivity)
    {
        super(iapActivity);
        requestIds = new HashMap<String, String>();
    }
    
    
    /**
     * Invoked once the observer is registered with the Puchasing Manager If the boolean is false, the application is
     * receiving responses from the SDK Tester. If the boolean is true, the application is live in production.
     *
     * @param isSandboxMode
     *            Boolean value that shows if the app is live or not.
     */
    public void onSDKAvailable(boolean isSandboxMode)
    {
        // Switch RVS URL from test to production
        rvsProductionMode = !isSandboxMode;
        Log.v(TAG, "onSdkAvailable recieved: Response -" + isSandboxMode);
        PurchasingManager.initiateGetUserIdRequest();
    }
    
    /**
     * Invoked once the call from initiateGetUserIdRequest is completed.
     * On a successful response, a response object is passed which contains the request id, request status, and the
     * userid generated for your application.
     *
     * @param response
     *            Response object containing the UserID
     */
    public void onGetUserIdResponse(final GetUserIdResponse response)
    {
        
        Log.v(TAG, "onGetUserIdResponse recieved: Response -" + response);
        Log.v(TAG, "RequestId:" + response.getRequestId());
        Log.v(TAG, "IdRequestStatus:" + response.getUserIdRequestStatus());
        
        
        if (response.getUserIdRequestStatus() ==
            GetUserIdResponse.GetUserIdRequestStatus.SUCCESSFUL)
        {
            currentUserID = response.getUserId();
        }
        else
        {
            Log.v(TAG, "onGetUserIdResponse: Unable to get user ID.");
        }
    }
    
    /**
     * Retrieve the offset you have previously persisted.
     * If no offset exists or the app is dealing exclusively with consumables use Offset.BEGINNING
     */
    public static Offset getPersistedOffset()
    {
        return Offset.BEGINNING;
    }
    
    
    /**
     * Is invoked once the call from initiatePurchaseUpdatesRequest is completed.
     * On a successful response, a response object is passed which contains the request id, request status, a set of
     * previously purchased receipts, a set of revoked skus, and the next offset if applicable. If a user downloads your
     * application to another device, this call is used to sync up this device with all the user's purchases.
     *
     * @param response
     *            Response object containing the user's recent purchases.
     */
    public void onPurchaseUpdatesResponse(final PurchaseUpdatesResponse response)
    {
        
        Log.v(TAG, "onPurchaseUpdatesRecived recieved: Response -" + response);
        Log.v(TAG, "PurchaseUpdatesRequestStatus:" + response.getPurchaseUpdatesRequestStatus());
        Log.v(TAG, "RequestID:" + response.getRequestId());
        
        // No implementation required when dealing solely with consumables
        switch (response.getPurchaseUpdatesRequestStatus())
        {
            case SUCCESSFUL:
                // Check for revoked SKUs
                for (final String sku : response.getRevokedSkus())
                {
                    Log.v(TAG, "Revoked Sku:" + sku);
                }
                
                // Process receipts
                for (final Receipt receipt : response.getReceipts())
                {
                    switch (receipt.getItemType())
                    {
                        case ENTITLED:
                        {
                            // If the receipt is for an entitlement,the customer is re-entitled.
                            // Add re-entitlement code here
                            
                            // TODO : How to get the purchase Id
                            //mPurchaseObserver.onPurchaseStateChanged(1,0);
                            mPurchaseObserver.onPurchaseStateChanged(receipt.getSku(),0);
                            
                            //Move this to EnginePurchaseObserver
                            /*
                            final boolean tVerified = true;
                            final int tPurchaseState = 0; //purchase state for success
                            final String tNotificationId = "";
                            final String tProductId = receipt.getSku();
                            final String tOrderId = "";
                            final long tPurchaseTime = 1;
                            final String tDeveloperPayload = "";
                            final String tSignedData = "";
                            final String tSignature = "";
                            
                            post(new Runnable() {
                                public void run() {
                                    doPurchaseStateChanged(tVerified, tPurchaseState,
                                                           tNotificationId, tProductId, tOrderId,
                                                           tPurchaseTime, tDeveloperPayload, tSignedData, tSignature);
                                    if (m_wake_on_event)
                                        doProcess(false);
                                }
                            });
                             */
                            break;
                        }
                        case SUBSCRIPTION:
                            // Purchase Updates for subscriptions can be done here in one of two ways:
                            // 1. Use the receipts to determineif the user currently has an active subscription
                            // 2. Use the receipts to create a subscription history for your customer.
                            
                            // TODO : How to get the purchase Id
                           // mPurchaseObserver.onPurchaseStateChanged(1,0);
                            mPurchaseObserver.onPurchaseStateChanged(receipt.getSku(),0);
                            
                            
                            //Move this to enginePurchaseObserver
                            /*
                            final boolean tVerified = true;
                            final int tPurchaseState = 0; //purchase state for success
                            final String tNotificationId = "";
                            //NOTE : the getSku() method returns the parent SKU of the subscription
                            final String tProductId = receipt.getSku();
                            final String tOrderId = "";
                            final long tPurchaseTime = 1;
                            final String tDeveloperPayload = "";
                            final String tSignedData = "";
                            final String tSignature = "";
                            
                            post(new Runnable() {
                                public void run() {
                                    doPurchaseStateChanged(tVerified, tPurchaseState,
                                                           tNotificationId, tProductId, tOrderId,
                                                           tPurchaseTime, tDeveloperPayload, tSignedData, tSignature);
                                    if (m_wake_on_event)
                                        doProcess(false);
                                }
                            });
                            
                             */
                            break;
                    }
                }
                
                final Offset newOffset = response.getOffset();
                if (response.isMore()) {
                    Log.v(TAG, "Initiating Another Purchase Updates with offset: "
                          + newOffset.toString());
                    PurchasingManager.initiatePurchaseUpdatesRequest(newOffset);
                }
                break;
            case FAILED:
                // Provide the user access to any previously persisted entitlements.
                break;
        }
    }
    
    /**
     * Is invoked once the call from initiatePurchaseRequest is completed.
     * On a successful response, a response object is passed which contains the request id, request status, and the
     * receipt of the purchase.
     *
     * @param response
     *            Response object containing a receipt of a purchase
     */
    public void onPurchaseResponse(PurchaseResponse response)
    {
        Log.v(TAG, "onPurchaseResponse recieved");
        Log.v(TAG, "PurchaseRequestStatus:" + response.getPurchaseRequestStatus());
        
        switch (response.getPurchaseRequestStatus())
        {
            case SUCCESSFUL:
                /*
                 * You can verify the receipt and fulfill the purchase on successful responses.
                 */
                //Receipt receipt = response.getReceipt();
                //String itemType = receipt.getItemType().toString();
                //String sku = receipt.getSku();
                //String purchaseToken = receipt.getPurchaseToken();
                Log.v(TAG, "Successsful purchase for request" + requestIds.get(response.getRequestId()));
                Log.v(TAG, "Ordinal for SUCCESSFUL = " + response.getPurchaseRequestStatus().ordinal());
                break;
                
            case ALREADY_ENTITLED:
                /*
                 * If the customer has already been entitled to the item, a receipt is not returned.
                 * Fulfillment is done unconditionally, we determine which item should be fulfilled by matching the
                 * request id returned from the initial request with the request id stored in the response.
                 */
                Log.v(TAG, "ENTITLED + " + requestIds.get(response.getRequestId()));
                Log.v(TAG, "Ordinal for ENTITLED = " + response.getPurchaseRequestStatus().ordinal());
                break;
                
            case FAILED:
                /*
                 * If the purchase failed for some reason, (The customer canceled the order, or some other
                 * extraneous circumstance happens) the application ignores the request and logs the failure.
                 */
                Log.v(TAG, "Failed purchase for request" + requestIds.get(response.getRequestId()));
                Log.v(TAG, "Ordinal for FAILED = " + response.getPurchaseRequestStatus().ordinal());
                break;
                
            case INVALID_SKU:
                /*
                 * If the sku that was purchased was invalid, the application ignores the request and logs the failure.
                 * This can happen when there is a sku mismatch between what is sent from the application and what
                 * currently exists on the dev portal.
                 */
                Log.v(TAG, "Invalid Sku for request " + requestIds.get(response.getRequestId()));
                Log.v(TAG, "Ordinal for INVALID_SKU = " + response.getPurchaseRequestStatus().ordinal());
                break;
        }
        

        final String tProductId;
        
        /*Q: Why do I get a different SKU back when I initiate a purchase for my subscription SKU?
         A: A subscription is comprised of a non-buyable parent SKU and one or more child term SKUs. This setup prevents users from purchasing multiple subscriptions of the same product. The non-buyable parent represents the product being purchased and is the SKU returned in the purchase response. The child SKUs represent the different subscription terms and are used to initiate the purchase. Subscription terms and charges are handled by Amazon and your app only needs to check whether a subscription is valid.
         */
        
        
        switch (response.getPurchaseRequestStatus())
        {
            case SUCCESSFUL:
                Receipt receipt = response.getReceipt();
                // Don't use tProductId = requestIds.get(response.getRequestId()), it does not work on subscriptions because it returns the child SKU, but we need the parent SKU
                tProductId = receipt.getSku();
                Log.d(TAG, "PRODUCT ID IS : " + tProductId);
                break;
            default:
                tProductId = requestIds.get(response.getRequestId());
                break;
        }
        
        // TODO : How to get the purchase Id
        //mPurchaseObserver.onPurchaseStateChanged(1,response.getPurchaseRequestStatus().ordinal());
        mPurchaseObserver.onPurchaseStateChanged(tProductId,response.getPurchaseRequestStatus().ordinal());
        
        //TODO: MOVE THIS TO EnginePurchaseObserver.
    /*
        final boolean tVerified = true;
        final int tPurchaseState = response.getPurchaseRequestStatus().ordinal();
        final String tNotificationId = "";
        
        final String tOrderId = "";//response.getRequestId();
        final long tPurchaseTime = 1;
        final String tDeveloperPayload = "";
        final String tSignedData = "";
        final String tSignature = "";
        
        post(new Runnable() {
            public void run() {
                doPurchaseStateChanged(tVerified, tPurchaseState,
                                       tNotificationId, tProductId, tOrderId,
                                       tPurchaseTime, tDeveloperPayload, tSignedData, tSignature);
                if (m_wake_on_event)
                    doProcess(false);
            }
        });
     */
        
        
    }
}
