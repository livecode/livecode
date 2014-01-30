
package com.runrev.android.billing.amazon;

import com.runrev.android.billing.*;
import com.runrev.android.billing.amazon.MyPurchasingObserver;
import com.amazon.inapp.purchasing.*;


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
    
    public void initBilling()
    {
        mPurchasingObserver = new MyPurchasingObserver(getActivity());
        PurchasingManager.registerObserver(mPurchasingObserver);
        Log.v(TAG, "IAP initialised");
        started = true;
        PurchasingManager.initiateGetUserIdRequest();
        restorePurchases();
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
        
        PurchasingManager.registerObserver(mPurchasingObserver);
        return true;
    }

    public boolean disableUpdates()
    {
        //TODO
        return true;
    }
    
    public boolean restorePurchases()
    {
        if (!started)
            return false;
        
        PurchasingManager.initiatePurchaseUpdatesRequest(MyPurchasingObserver.getPersistedOffset());
        return true;
    }

    public boolean sendRequest(int purchaseId, String productId, Map<String, String> properties)
    {
        if (!started)
            return false;
        
        Log.v(TAG, "Purchase request started");
        String requestId = PurchasingManager.initiatePurchaseRequest(productId);
        Log.v(TAG, "Purchase request finished");
        mPurchasingObserver.requestIds.put(requestId, productId);
        
        return true;
    }
    
    public boolean consumePurchase(String productID)
    {
        return true;
    }
    
    public boolean confirmDelivery(int purchaseId)
    {
        if (!started)
            return false;
        
        // Amazon client is responsible for validating purchase receipts. Does that mean that this method does nothing?
        return true;
    }

    public void setPurchaseObserver(PurchaseObserver observer)
    {
        mPurchaseObserver = observer;
    }
    
    public Map<String, String> getPurchaseProperties(int purchaseId)
    {
        return new HashMap<String,String>();
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
}