
package com.runrev.android.billing.amazon;

import com.runrev.android.billing.*;
import com.amazon.inapp.purchasing.*;
import com.amazon.inapp.purchasing.PurchaseResponse.PurchaseRequestStatus;

import android.app.*;
import android.util.*;
import android.content.*;
import java.util.*;



public class AmazonBillingProvider implements BillingProvider
{

    public static final String TAG = "AmazonBillingProvider";
    private Activity mActivity;
    private MyPurchasingObserver mPurchasingObserver;
    private Boolean started = false;
    private PurchaseObserver mPurchaseObserver;
    private Map<String,Map<String,String>> itemProps = new HashMap<String, Map<String,String>>();
    
    private List<Item> knownItems = new ArrayList<Item>();
    private Set<String> ownedItems = new HashSet<String>();
    
    public void initBilling()
    {
        mPurchasingObserver = new MyPurchasingObserver(getActivity());
        PurchasingManager.registerObserver(mPurchasingObserver);
        Log.v(TAG, "IAP initialised");
        started = true;
        PurchasingManager.initiateGetUserIdRequest();
    }
    
    public void onDestroy()
	{
    
	}
    
    /*Always equals true. The Amazon Appstore allows a customer to disable In-App Purchasing, the IAP workflow
     will reflect this when the user is prompted to buy an item. There is no way for your app to know if a
     user has disabled Amazon In-App Purchasing */
    public boolean canMakePurchase()
    {
        if (!started)
            return false;
        return true;
    }
    
    public boolean enableUpdates()
    {
        if (!started)
            return false;
        
        //PurchasingManager.registerObserver(mPurchasingObserver);
        return true;
    }

    public boolean disableUpdates()
    {
        if (!started)
            return false;
        
        return true;
    }
    
    public boolean restorePurchases()
    {
        if (!started)
            return false;
        
        PurchasingManager.initiatePurchaseUpdatesRequest(mPurchasingObserver.getPersistedOffset());
        return true;
    }

    public boolean sendRequest(int purchaseId, String productId, String developerPayload)
    {
        if (!started)
            return false;
        
        Log.v(TAG, "Purchase request started");
        String requestId = PurchasingManager.initiatePurchaseRequest(productId);
        Log.v(TAG, "Purchase request finished");
        mPurchasingObserver.requestIds.put(requestId, productId);
        
        return true;
    }
    
    public boolean makePurchase(String productId, String quantity, String payload)
    {
        if (!started)
            return false;
        
        Log.v(TAG, "Purchase request started");
        String requestId = PurchasingManager.initiatePurchaseRequest(productId);
        Log.v(TAG, "Purchase request finished");
        mPurchasingObserver.requestIds.put(requestId, productId);
        
        return true;
    }
    
    public boolean productSetType(String productId, String productType)
    {
        return true;
    }
    
    public boolean setPurchaseProperty(String productId, String propertyName, String propertyValue)
    {
        if (!itemProps.containsKey(productId))
            itemProps.put(productId, new HashMap<String,String>());
        (itemProps.get(productId)).put(propertyName, propertyValue);
        
        return true;
    }
    
    public String getPurchaseProperty(String productId, String propName)
    {
        Log.d(TAG, "Stored properties for productId :" + productId);
        Map<String,String> map = itemProps.get(productId);
        if (map != null)
            return map.get(propName);
        else
            return "";
    }
    
    public String getPurchaseList()
    {
        return ownedItems.toString();
    }
    
    public boolean consumePurchase(String productID)
    {
        if (ownedItems.contains(productID))
            ownedItems.remove(productID);
        return true;
    }
    
    
    public boolean requestProductDetails(String productId)
    {
        Set<String> skuSet = new HashSet<String>();
        skuSet.add(productId);
        PurchasingManager.initiateItemDataRequest(skuSet);
        return true;
    }
    
    public String receiveProductDetails(String productId)
    {
        for (Item item : knownItems)
        {
            if (productId.equals(item.getSku()))
            {
                return item.toString();
            }
        }
        
        return "Product ID not found";
    }

    
    public boolean confirmDelivery(int purchaseId)
    {
        if (!started)
            return false;
        
        // Amazon client is responsible for validating purchase receipts. 
        return true;
    }

    public void setPurchaseObserver(PurchaseObserver observer)
    {
        mPurchaseObserver = observer;
    }
    
    Activity getActivity()
    {
        return mActivity;
    }
    
    public void setActivity(Activity activity)
    {
        mActivity = activity;
    }
    
    public void onActivityResult (int requestCode, int resultCode, Intent data)
    {
        
    }
    
    boolean addPurchaseReceiptToLocalInventory(Receipt receipt)
    {
        boolean success = true;
        if (success)
            success = setPurchaseProperty(receipt.getSku(), "productId", receipt.getSku());
        if (success)
            success = setPurchaseProperty(receipt.getSku(), "itemType", receipt.getItemType().toString());
        
        if (success && receipt.getSubscriptionPeriod() != null)
            success = setPurchaseProperty(receipt.getSku(), "subscriptionPeriod", receipt.getSubscriptionPeriod().toString());
        
        if (success)
            success = setPurchaseProperty(receipt.getSku(), "purchaseToken", receipt.getPurchaseToken());
        
        return success;
        
    }
    
    boolean loadKnownItemToLocalInventory(Item item)
    {
        boolean success = true;
        if (success)
            success = setPurchaseProperty(item.getSku(), "itemType", item.getItemType().toString());
        
        if (success)
            success = setPurchaseProperty(item.getSku(), "price", item.getPrice());
        
        if (success)
            success = setPurchaseProperty(item.getSku(), "description", item.getDescription());
        
        if (success)
            success = setPurchaseProperty(item.getSku(), "title", item.getTitle());
        
        if (success)
            success = setPurchaseProperty(item.getSku(), "itemImageUrl", item.getSmallIconUrl());
        
        return success;
    }
    
    private class MyPurchasingObserver extends BasePurchasingObserver
    {
        
        private boolean rvsProductionMode = false;
        private String currentUserID = null;
        public Map<String, String> requestIds;
        private Map<String, Item> availableSkus;
        private Set<String> unavailableSkus;
        
        private static final String TAG = "IAPPurchasingObserver";
        
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
        
        public void onItemDataResponse(ItemDataResponse response)
        {
            Log.v(TAG, "onItemDataResponse recieved: Response -" + response);
            Log.v(TAG, "RequestId:" + response.getRequestId());
            Log.v(TAG, "ItemDataRequestStatus:" + response.getItemDataRequestStatus());
            
            
            if (response.getItemDataRequestStatus() ==
                ItemDataResponse.ItemDataRequestStatus.SUCCESSFUL)
            {
                availableSkus = response.getItemData();
                for (String key : availableSkus.keySet())
                {
                    Item item = availableSkus.get(key);
                    knownItems.add(item);
                    loadKnownItemToLocalInventory(item);
                    Log.v(TAG, "Item details : " + item.toString());
                    mPurchaseObserver.onProductDetailsReceived(item.getSku());
                }
                
               // 
            }
            else
            {
                unavailableSkus = response.getUnavailableSkus();
                for (String unavailableSku : unavailableSkus)
                    mPurchaseObserver.onProductDetailsError(unavailableSku, "No product found with the specified ID");
                Log.v(TAG, "onItemDataResponse: Unable to get Item data.");
            }
        }
        
        /**
         * Retrieve the offset you have previously persisted.
         * If no offset exists or the app is dealing exclusively with consumables use Offset.BEGINNING
         */
        public Offset getPersistedOffset()
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
            
            // PM-2015-02-05: [[ Bug 14402 ]] Handle case when calling mobileStoreRestorePurchases but there are no previous purchases to restore
            boolean t_did_restore;
            t_did_restore = false;
            
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
                        Log.v(TAG, "Processing receipt : " + receipt.toString());
                        switch (receipt.getItemType())
                        {
                            case ENTITLED:
                            {
                                // If the receipt is for an entitlement,the customer is re-entitled.
                                // Add re-entitlement code here
                                
                                Log.v(TAG, "Time to add receipt to local inventory...");
                                addPurchaseReceiptToLocalInventory(receipt);
                                ownedItems.add(receipt.getSku());
                                // onPurchaseStateChanged to be called with state = 5 (restored)
                                mPurchaseObserver.onPurchaseStateChanged(receipt.getSku(),5);
                                t_did_restore = true;
                                break;
                            }
                            case SUBSCRIPTION:
                                // Purchase Updates for subscriptions can be done here in one of two ways:
                                // 1. Use the receipts to determineif the user currently has an active subscription
                                // 2. Use the receipts to create a subscription history for your customer.
                                
                                SubscriptionPeriod subscriptionPeriod = receipt.getSubscriptionPeriod();
                                
                                Date subscriptionStart = subscriptionPeriod.getStartDate();
                                Date subscriptionEnd = subscriptionPeriod.getEndDate();
                                
                                Date today = new Date();
                                
                                boolean subscriptionHasStarted = subscriptionStart.before(today) || subscriptionStart.equals(today);
                                boolean subscriptionHasEnded = subscriptionEnd != null && today.after(subscriptionEnd);
                                
                                if (subscriptionHasStarted && !subscriptionHasEnded)
                                {
                                    addPurchaseReceiptToLocalInventory(receipt);
                                    ownedItems.add(receipt.getSku());
                                    // onPurchaseStateChanged to be called with state = 5 (restored)
                                    mPurchaseObserver.onPurchaseStateChanged(receipt.getSku(),5);
                                    t_did_restore = true;
                                }
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
            
            if(!t_did_restore)
            {
                // PM-2015-02-12: [[ Bug 14402 ]] When there are no previous purchases to restore, send a purchaseStateUpdate msg with state=restored and productID=""
                mPurchaseObserver.onPurchaseStateChanged("",5);
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
                    ownedItems.add(receipt.getSku());
                    addPurchaseReceiptToLocalInventory(receipt);
                    break;
                default:
                    tProductId = requestIds.get(response.getRequestId());
                    break;
            }
            
            mPurchaseObserver.onPurchaseStateChanged(tProductId, mapResponseCode(response.getPurchaseRequestStatus()));
            
        }
        
        // Should match the order of enum MCAndroidPurchaseState (mblandroidstore.cpp)
        int mapResponseCode(PurchaseResponse.PurchaseRequestStatus responseCode)
        {
            int result;
            switch(responseCode)
            {
                case SUCCESSFUL:
                    result = 0;
                    break;
                    
                case FAILED:
                    result = 1;
                    break;
                    
                case INVALID_SKU:
                    result = 2;
                    break;
                    
                case ALREADY_ENTITLED:
                    result = 3;
                    break;
                default:
                    result = 1;
                    break;
            }
            return result;
        }

    }
}