

package com.runrev.android.billing.google;

import com.runrev.android.billing.google.Purchase;
import com.runrev.android.billing.*;
import com.runrev.android.Engine;

import android.app.*;
import android.util.*;
import android.content.*;

import java.util.*;



public class GoogleBillingProvider implements BillingProvider
{
    public static final String TAG = "GoogleBillingProvider";
    private Activity mActivity;
    private Boolean started = false;
    private PurchaseObserver mPurchaseObserver;
    private Map<String,String> types = new HashMap<String,String>();
    private Map<String,Map<String,String>> itemProps = new HashMap<String, Map<String,String>>();
	
	private List<SkuDetails> knownItems = new ArrayList<SkuDetails>();
    private Set<String> ownedItems = new HashSet<String>();
    
    /* 
     Temp var for holding the productId, to pass it in onIabPurchaseFinished(IabResult result, Purchase purchase), in case purchase is null.
     Thus, purchase.getSku() will not work
    */
    private String pendingPurchaseSku = "";
    
    // (arbitrary) request code for the purchase flow
    static final int RC_REQUEST = 10001;
    
    // The helper object
    IabHelper mHelper = null;
    
    public void initBilling()
    {
        String t_public_key = Engine.doGetCustomPropertyValue("cREVStandaloneSettings", "android,storeKey");
        
        if (t_public_key != null && t_public_key.length() > 0)
            Security.setPublicKey(t_public_key);
        
        // Create the helper, passing it our context and the public key to verify signatures with
        Log.d(TAG, "Creating IAB helper.");
        mHelper = new IabHelper(getActivity(), t_public_key);
        
        // TODO enable debug logging (for a production application, you should set this to false).
        mHelper.enableDebugLogging(false);
        
        // Start setup. This is asynchronous and the specified listener
        // will be called once setup completes.
        Log.d(TAG, "Starting setup.");
        mHelper.startSetup(new IabHelper.OnIabSetupFinishedListener()
                           {
            public void onIabSetupFinished(IabResult result)
            {
                Log.d(TAG, "Setup finished.");
                
                if (!result.isSuccess())
                {
                    // Oh no, there was a problem.
                    complain("Problem setting up in-app billing: " + result);
                    return;
                }
                
                // Have we been disposed of in the meantime? If so, quit.
                if (mHelper == null) return;
                
                // IAB is fully set up.
                Log.d(TAG, "Setup successful.");
                
                //mHelper.queryInventoryAsync(mGotInventoryListener);
            }
        });
    }
	
	public void onDestroy()
	{
		if (mHelper != null)
            mHelper.dispose();
        mHelper = null;
	}
    
    public boolean canMakePurchase()
    {
        if (mHelper == null)
            return false;
	
        else
            return mHelper.is_billing_supported;
    }
    
    public boolean enableUpdates()
    {
        if (mHelper == null)
            return false;
        
        return true;
    }
    
    public boolean disableUpdates()
    {
        if (mHelper == null)
            return false;
        
		return true;
    }
    
    public boolean restorePurchases()
    {
        if (mHelper == null)
		{
			return false;
		}
         
		Log.d(TAG, "Querying inventory.");
		mHelper.queryInventoryAsync(mGotInventoryListener);
         
        return true;
    }
    
    public boolean sendRequest(int purchaseId, String productId, String developerPayload)
    {
        if (mHelper == null)
            return false;
        
        String type = productGetType(productId);
        if (type == null)
        {
            Log.i(TAG, "Item type is null (not specified). Exiting..");
            return false;
        }
        
        pendingPurchaseSku = productId;
        
        Log.i(TAG, "purchaseSendRequest(" + purchaseId + ", " + productId + ", " + type + ")");
        
        if (type.equals("subs"))
        {
            mHelper.launchSubscriptionPurchaseFlow(getActivity(), productId, RC_REQUEST, mPurchaseFinishedListener, developerPayload);
			return true;
        }
        else if (type.equals("inapp"))
        {
            mHelper.launchPurchaseFlow(getActivity(), productId, RC_REQUEST, mPurchaseFinishedListener, developerPayload);
			return true;
        }
		else
		{
			Log.i(TAG, "Item type is not recognized. Exiting..");
            return false;
		}
        
    }
    
    public boolean makePurchase(String productId, String quantity, String payload)
    {
        if (mHelper == null)
            return false;
        
        String type = productGetType(productId);
        if (type == null)
        {
            Log.i(TAG, "Item type is null (not specified). Exiting..");
            return false;
        }
        
        pendingPurchaseSku = productId;
		setPurchaseProperty(productId, "developerPayload", payload);
        
        Log.i(TAG, "purchaseSendRequest("  + productId + ", " + type + ")");
        
        if (type.equals("subs"))
        {
            mHelper.launchSubscriptionPurchaseFlow(getActivity(), productId, RC_REQUEST, mPurchaseFinishedListener, payload);
			return true;
        }
        else if (type.equals("inapp"))
		{
            mHelper.launchPurchaseFlow(getActivity(), productId, RC_REQUEST, mPurchaseFinishedListener, payload);
			return true;
        }
		else
		{
			Log.i(TAG, "Item type is not recognized. Exiting..");
            return false;
		}
        
    }
    
    public boolean productSetType(String productId, String productType)
    {
        Log.d(TAG, "Setting type for productId" + productId + ", type is : " + productType);
        types.put(productId, productType);
        Log.d(TAG, "Querying HashMap, type is " + types.get(productId));
        return true;
    }
    
    private String productGetType(String productId)
    {
        return types.get(productId);
    }
    
    public boolean consumePurchase(final String productId)
    {
        mHelper.queryInventoryAsync(new IabHelper.QueryInventoryFinishedListener()
                                    {
            @Override
            public void onQueryInventoryFinished(IabResult result, Inventory inventory)
            {
                
                if (result.isFailure())
                {
                    return;
                }
                else
                {
                    Purchase purchase = inventory.getPurchase(productId);
                    // Do this check to avoid a NullPointerException
                    if (purchase == null)
                    {
                        Log.d(TAG, "You cannot consume item : " + productId + ", since you don't own it!");
                        return;
                    }
                    mHelper.consumeAsync(purchase, mConsumeFinishedListener);
                }
            }
        });
        return true;
    }
    
    public boolean requestProductDetails(final String productId)
    {
        //arbitrary initial capacity
        int capacity = 25; 
        List<String> productList = new ArrayList<String>(capacity);
        productList.add(productId);
        mHelper.queryInventoryAsync(true, productList, new IabHelper.QueryInventoryFinishedListener()
                                    {
            @Override
            public void onQueryInventoryFinished(IabResult result, Inventory inventory)
            {
                
                if (result.isFailure())
                {
                    return;
                }
                else
                {
                    SkuDetails skuDetails = inventory.getSkuDetails(productId);
                    // Do this check to avoid a NullPointerException
                    if (skuDetails == null)
                    {
                        Log.d(TAG, "No product found with the specified ID : " + productId + " !");
						mPurchaseObserver.onProductDetailsError(productId, "No product found with the specified ID");
                        return;
                    }
                   
					knownItems.add(skuDetails);
					loadKnownItemToLocalInventory(skuDetails);
                    Log.d(TAG, "Details for requested product : " + skuDetails.toString());
                    
					mPurchaseObserver.onProductDetailsReceived(productId);
					
                }
            }
        });
		
		return true;
    }
	
	public String receiveProductDetails(String productId)
    {
		for (SkuDetails skuDetails : knownItems)
		{
			if (productId.equals(skuDetails.getSku()))
			{
				return skuDetails.toString();
			}
		}
		
        return "Product ID not found";
	}
    
    public boolean confirmDelivery(int purchaseId)
    {
        if (mHelper == null)
            return false;
        
        else
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
    
    //some helper methods
    
    boolean addPurchaseToLocalInventory(Purchase purchase)
    {
        boolean success = true;
		if (success)
            success = setPurchaseProperty(purchase.getSku(), "productId", purchase.getSku());
        if (success)
            success = setPurchaseProperty(purchase.getSku(), "itemType", purchase.getItemType());
        
        if (success)
            success = setPurchaseProperty(purchase.getSku(), "orderId", purchase.getOrderId());
        
        if (success)
            success = setPurchaseProperty(purchase.getSku(), "packageName", purchase.getPackageName());

        if (success)
            success = setPurchaseProperty(purchase.getSku(), "purchaseToken", purchase.getToken());

        if (success)
            success = setPurchaseProperty(purchase.getSku(), "signature", purchase.getSignature());

		if (success)
            success = setPurchaseProperty(purchase.getSku(), "developerPayload", purchase.getDeveloperPayload());

        if (success)
            success = setPurchaseProperty(purchase.getSku(), "purchaseTime", new Long(purchase.getPurchaseTime()).toString());
			
        return success;

    }
	
	boolean loadKnownItemToLocalInventory(SkuDetails skuDetails)
	{
		boolean success = true;
		if (success)
            success = setPurchaseProperty(skuDetails.getSku(), "productId", skuDetails.getSku());
        if (success)
            success = setPurchaseProperty(skuDetails.getSku(), "itemType", skuDetails.getType());
        
        if (success)
            success = setPurchaseProperty(skuDetails.getSku(), "price", skuDetails.getPrice());
        
        if (success)
            success = setPurchaseProperty(skuDetails.getSku(), "title", skuDetails.getTitle());
		
        if (success)
            success = setPurchaseProperty(skuDetails.getSku(), "description", skuDetails.getDescription());
		
        return success;
	}
    
    void removePurchaseFromLocalInventory(Purchase purchase)
    {
        ownedItems.remove(purchase.getSku());
        
    }
    
    void complain(String message)
    {
        Log.d(TAG, "**** Error: " + message);
        alert("Error: " + message);
    }
    
    void alert(String message)
    {
        AlertDialog.Builder bld = new AlertDialog.Builder(getActivity());
        bld.setMessage(message);
        bld.setNeutralButton("OK", null);
        Log.d(TAG, "Showing alert dialog: " + message);
        bld.create().show();
    }
    
    // Listeners
    
    IabHelper.OnIabPurchaseFinishedListener mPurchaseFinishedListener = new IabHelper.OnIabPurchaseFinishedListener()
    {
        // parameter "purchase" is null if purchase failed
        public void onIabPurchaseFinished(IabResult result, Purchase purchase)
        {
            Log.d(TAG, "Purchase finished: " + result + ", purchase: " + purchase);
            
            // if we were disposed of in the meantime, quit.
            if (mHelper == null)
            {
                return;
            }
    
            if (result.isFailure())
            {
				// PM-2015-01-27: [[ Bug 14450 ]] [Removed code] No need to display an alert with the error message, since this information is also contained in the purchaseStateUpdate message
                mPurchaseObserver.onPurchaseStateChanged(pendingPurchaseSku, mapResponseCode(result.getResponse()));
                pendingPurchaseSku = "";
                return;
            }
            
            if (!verifyDeveloperPayload(purchase))
            {
                complain("Error purchasing. Authenticity verification failed.");
                return;
            }
            
            Log.d(TAG, "Purchase successful.");
            pendingPurchaseSku = "";
			ownedItems.add(purchase.getSku());
            addPurchaseToLocalInventory(purchase);
            offerPurchasedItems(purchase);
                
        }
    };

    void offerPurchasedItems(Purchase purchase)
    {
        if (purchase != null)
            mPurchaseObserver.onPurchaseStateChanged(purchase.getSku(), mapResponseCode(purchase.getPurchaseState()));

    }

    // Called when consumption is complete
    IabHelper.OnConsumeFinishedListener mConsumeFinishedListener = new IabHelper.OnConsumeFinishedListener()
    {
        public void onConsumeFinished(Purchase purchase, IabResult result)
        {
            Log.d(TAG, "Consumption finished. Purchase: " + purchase + ", result: " + result);

            if (result.isSuccess())
            {
                Log.d(TAG, "Consumption successful. Provisioning.");
                removePurchaseFromLocalInventory(purchase);
            }
            else
            {
                complain("Error while consuming: " + result);
            }

            Log.d(TAG, "End consumption flow.");
        }
    };

    // Called when we finish querying the items we own
    IabHelper.QueryInventoryFinishedListener mGotInventoryListener = new IabHelper.QueryInventoryFinishedListener()
    {
        public void onQueryInventoryFinished(IabResult result, Inventory inventory)
        {
            Log.d(TAG, "Query inventory finished.");

            // Have we been disposed of in the meantime? If so, quit.
            if (mHelper == null)
                return;

            if (result.isFailure())
            {
                complain("Failed to query inventory: " + result);
                return;
            }

            Log.d(TAG, "Query inventory was successful.");

            Log.d(TAG, "Initial inventory query finished; enabling main UI.");

			// PM-2015-02-05: [[ Bug 14402 ]] Handle case when calling mobileStoreRestorePurchases but there are no previous purchases to restore
			boolean t_did_restore;
			t_did_restore = false;

            List<Purchase> purchaseList = inventory.getallpurchases();

            for (Purchase p : purchaseList)
            {
                addPurchaseToLocalInventory(p);
				ownedItems.add(p.getSku());
				// onPurchaseStateChanged to be called with state = 5 (restored)
				mPurchaseObserver.onPurchaseStateChanged(p.getSku(), 5);
				t_did_restore = true;
            }

			if(!t_did_restore)
			{
				// PM-2015-02-12: [[ Bug 14402 ]] When there are no previous purchases to restore, send a purchaseStateUpdate msg with state=restored and productID=""
				mPurchaseObserver.onPurchaseStateChanged("",5);
			}
        }
    };

    /** Verifies the developer payload of a purchase. */
    boolean verifyDeveloperPayload(Purchase p)
    {
        String payload = p.getDeveloperPayload();

    /*
     * TODO: verify that the developer payload of the purchase is correct. It will be
     * the same one that you sent when initiating the purchase.
     *
     * WARNING: Locally generating a random string when starting a purchase and
     * verifying it here might seem like a good approach, but this will fail in the
     * case where the user purchases an item on one device and then uses your app on
     * a different device, because on the other device you will not have access to the
     * random string you originally generated.
     *
     * So a good developer payload has these characteristics:
     *
     * 1. If two different users purchase an item, the payload is different between them,
     *    so that one user's purchase can't be replayed to another user.
     *
     * 2. The payload must be such that you can verify it even when the app wasn't the
     *    one who initiated the purchase flow (so that items purchased by the user on
     *    one device work on other devices owned by the user).
     *
     * Using your own server to store and verify developer payloads across app
     * installations is recommended.
     */

        return true;
    }

    public void onActivityResult (int requestCode, int resultCode, Intent data)
    {
        Log.d(TAG, "onActivityResult(" + requestCode + "," + resultCode + "," + data);

        // Pass on the activity result to the helper for handling
        if (!mHelper.handleActivityResult(requestCode, resultCode, data))
        {
            // not handled
            Log.d(TAG, "onActivityResult NOT handled by IABUtil.");
        }
        else
        {
            Log.d(TAG, "onActivityResult handled by IABUtil.");
        }
    }

    // Should match the order of enum MCAndroidPurchaseState (mblandroidstore.cpp)
    int mapResponseCode(int responseCode)
    {
        int result;
        switch(responseCode)
        {
            case IabHelper.BILLING_RESPONSE_RESULT_OK:
                result = 0;
                break;

            case IabHelper.BILLING_RESPONSE_RESULT_USER_CANCELED:
                result = 1;
                break;

			case IabHelper.BILLING_RESPONSE_RESULT_ITEM_UNAVAILABLE:
				result = 2;
				break;

            case IabHelper.BILLING_RESPONSE_RESULT_ITEM_ALREADY_OWNED:
                result = 3;
                break;

            default:
                result = 1;
                break;
        }
        return result;
    }

}
