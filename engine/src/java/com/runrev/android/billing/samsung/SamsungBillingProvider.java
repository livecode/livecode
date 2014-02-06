package com.runrev.android.billing.samsung;

import com.runrev.android.billing.*;

import android.app.*;
import android.os.*;
import android.util.*;
import android.content.*;

import com.sec.android.iap.sample.helper.SamsungIapHelper;
import com.sec.android.iap.sample.helper.SamsungIapHelper.OnGetInboxListListener;
import com.sec.android.iap.sample.helper.SamsungIapHelper.OnGetItemListListener;
import com.sec.android.iap.sample.helper.SamsungIapHelper.OnInitIapListener;
import com.sec.android.iap.sample.vo.BaseVO;
import com.sec.android.iap.sample.vo.InBoxVO;
import com.sec.android.iap.sample.vo.ItemVO;
import com.sec.android.iap.sample.vo.PurchaseVO;

import java.text.SimpleDateFormat;
import java.util.*;

public class SamsungBillingProvider implements BillingProvider
{
    
    public static final String TAG = "SamsungBillingProvider";
    private Activity mActivity;
    private Boolean started = false;
    private PurchaseObserver mPurchaseObserver;
    
    //private static final int iapMode = SamsungIapHelper.IAP_MODE_COMMERCIAL;
    private static final int iapMode = SamsungIapHelper.IAP_MODE_TEST_SUCCESS;
    
    private String itemGroupId = null;
    private String pendingPurchaseItemId = null;
    private boolean prepared = false;
    private boolean isInitialized = false;
    private static final int ITEM_AMOUNT = 25;
    private List<ItemVO> knownItems = new ArrayList<ItemVO>();
    private Set<String> ownedItems = new HashSet<String>();
    //private OnInitIapListener mOnInitIapListener = null;
    //private OnGetItemListListener mOnGetItemListListener = null;
    //private OnGetInboxListListener mOnGetInboxListListener = null;
    
    private Map<String,Map<String,String>> itemProps = new HashMap<String, Map<String,String>>();
    
    
    /* LISTENERS */
    
    SamsungIapHelper.OnInitIapListener mInitIapListener = new SamsungIapHelper.OnInitIapListener()
    {
        public void onSucceedInitIap()
        {
            SamsungIapHelper helper = SamsungIapHelper.getInstance(getActivity(), iapMode);
            helper.safeGetItemList(getActivity(), itemGroupId, 1, 1 + ITEM_AMOUNT, SamsungIapHelper.ITEM_TYPE_ALL);
        }
    };

    SamsungIapHelper.OnGetItemListListener mGetItemListListener = new SamsungIapHelper.OnGetItemListListener()
    {
        public void onSucceedGetItemList(ArrayList<ItemVO> itemList)
        {
            knownItems.addAll(itemList);

            SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd", Locale.getDefault());
            String today = sdf.format(new Date());

            SamsungIapHelper helper = SamsungIapHelper.getInstance(getActivity(), iapMode);
            helper.safeGetItemInboxTask(getActivity(), itemGroupId, 1, 1 + ITEM_AMOUNT, "20131001", today);
        }
    };

    SamsungIapHelper.OnGetInboxListListener mGetInboxListListener = new SamsungIapHelper.OnGetInboxListListener()
    {
        public void OnSucceedGetInboxList(ArrayList<InBoxVO> inboxList)
        {
            for (InBoxVO inboxItem : inboxList)
            {
                final String tItemId = inboxItem.getItemId();
                ownedItems.add(tItemId);
                loadInboxToLocalInventory(inboxItem);


                // Restore previously purchased items (only non-consumables and subscriptions)
                if(!inboxItem.getType().equals("00"))
                {
                    Log.d(TAG, "Item restored :" + tItemId);

                    // How to get the purchaseId?
                    int purchaseId = 1;

                    // 0 is the statuscode for success in samsung -- TODO: write it differently
                   // mPurchaseObserver.onPurchaseStateChanged(1, 0);
                    mPurchaseObserver.onPurchaseStateChanged(tItemId, 0);

                }

            }

            if (pendingPurchaseItemId != null)
            {
                startPurchase(pendingPurchaseItemId);
                pendingPurchaseItemId = null;
            }
        }
    };


    /////////////////////////

    /* HELPER METHODS */

    boolean loadBaseToLocalInventory(BaseVO baseVO)
    {
        boolean success = true;
        //Log.d(TAG, "Base Item id is : " + baseVO.getItemId());
        //Log.d(TAG, "Base Item name is : " + baseVO.getItemName());

        if (success)
        {
            success = setPurchaseProperty(baseVO.getItemId(), "itemName", baseVO.getItemName());
            //Log.d(TAG, "Base itemName loaded successfully");
        }
        if (success)
        {
            success = setPurchaseProperty(baseVO.getItemId(), "itemPriceString", baseVO.getItemPriceString());
            //Log.d(TAG, "Base itemPriceString loaded successfully");
        }
        if (success)
        {
            success = setPurchaseProperty(baseVO.getItemId(), "currencyUnit", baseVO.getCurrencyUnit());
            //Log.d(TAG, "Base currencyUnit loaded successfully");
        }
        if (success)
        {
            success = setPurchaseProperty(baseVO.getItemId(), "itemDescription", baseVO.getItemDesc());
            //Log.d(TAG, "Base itemDescription loaded successfully");
        }
        if (success)
        {
            success = setPurchaseProperty(baseVO.getItemId(), "itemImageUrl", baseVO.getItemImageUrl());
            //Log.d(TAG, "Base itemImageUrl loaded successfully");
        }
        if (success)
        {
            success = setPurchaseProperty(baseVO.getItemId(), "itemDownloadUrl", baseVO.getItemDownloadUrl());
            //Log.d(TAG, "Base itemDownloadUrl loaded successfully");
        }
        return success;
    }

    boolean addPurchaseToLocalInventory(PurchaseVO purchaseVO)
    {
        boolean success = loadBaseToLocalInventory(purchaseVO);

        if (success)
            success = setPurchaseProperty(purchaseVO.getItemId(), "paymentId", purchaseVO.getPaymentId());

        if (success)
            success = setPurchaseProperty(purchaseVO.getItemId(), "purchaseId", purchaseVO.getPurchaseId());

        if (success)
            success = setPurchaseProperty(purchaseVO.getItemId(), "purchaseDate", purchaseVO.getPurchaseDate());

        if (success)
            success = setPurchaseProperty(purchaseVO.getItemId(), "verifyUrl", purchaseVO.getVerifyUrl());
        return success;

    }


    boolean loadInboxToLocalInventory(InBoxVO inboxVO)
    {
        boolean success = loadBaseToLocalInventory(inboxVO);
        //Log.d(TAG, "Inbox Item id is : " + inboxVO.getItemId());
        //Log.d(TAG, "Inbox Item name is : " + inboxVO.getItemName());

        if (success)
            success = setPurchaseProperty(inboxVO.getItemId(), "paymentId", inboxVO.getPaymentId());

        if (success)
            success = setPurchaseProperty(inboxVO.getItemId(), "subscriptionEndDate", inboxVO.getSubscriptionEndDate());

        if (success)
            success = setPurchaseProperty(inboxVO.getItemId(), "purchaseDate", inboxVO.getPurchaseDate());

        if (success)
            success = setPurchaseProperty(inboxVO.getItemId(), "type", inboxVO.getType());

        return success;

    }

    /**
     * Start the IAP initialization process
     */
    private void initHelper()
    {
        if (prepared)
        {
            return;
        }
        prepared = true;

        getActivity().runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                SamsungIapHelper helper = SamsungIapHelper.getInstance(getActivity(), iapMode);
                helper.setOnInitIapListener(mInitIapListener);
                helper.setOnGetItemListListener(mGetItemListListener);
                helper.setOnGetInboxListListener(mGetInboxListListener);

                if (helper.isInstalledIapPackage(getActivity()))
                {
                    if (!helper.isValidIapPackage(getActivity()))
                    {
                        helper.showIapDialog(getActivity(), "title_iap", "msg_invalid_iap_package", true, null);
                    }
                }
                else
                {
                    helper.installIapPackage(getActivity());
                }
            }
        });
    }

    private void startPurchase(String itemId)
    {
        if (!isInitialized)
        {
            isInitialized = true;
            pendingPurchaseItemId = itemId;
            getActivity().runOnUiThread(new Runnable()
            {
                @Override
                public void run()
                {
                    SamsungIapHelper helper = SamsungIapHelper.getInstance(getActivity(), iapMode);
                    helper.showProgressDialog(getActivity());
                    helper.startAccountActivity(getActivity());
                }
            });
            return;
        }

        SamsungIapHelper helper = SamsungIapHelper.getInstance(getActivity(), iapMode);
        helper.showProgressDialog(getActivity());
        helper.startPurchase(getActivity(), SamsungIapHelper.REQUEST_CODE_IS_IAP_PAYMENT, itemGroupId, itemId);
    }


    /* Called in OnActivityResult */

    private void handleRequestCertification(int requestCode, int resultCode, Intent intent)
    {
        if (resultCode == Activity.RESULT_OK)
        {
            bindIapService();
        }
        else if (resultCode == Activity.RESULT_CANCELED)
        {
            SamsungIapHelper helper = SamsungIapHelper.getInstance(getActivity(), iapMode);
            helper.dismissProgressDialog();
            helper.showIapDialog(getActivity(), "title_samsungaccount_authentication", "msg_authentication_has_been_cancelled", false, null);
        }
    }

    private void handleRequestPayment(int requestCode, int resultCode, Intent intent)
    {
        if (intent == null)
        {
            return;
        }

        Bundle extras         = intent.getExtras();
        String itemId         = "";
        String thirdPartyName = "";
        int statusCode        = SamsungIapHelper.IAP_PAYMENT_IS_CANCELED;
        String errorString    = "";
        PurchaseVO purchaseVO = null;
        SamsungIapHelper helper = SamsungIapHelper.getInstance(getActivity(), iapMode);

        if (extras != null)
        {
            thirdPartyName = extras.getString(SamsungIapHelper.KEY_NAME_THIRD_PARTY_NAME);
            statusCode = extras.getInt(SamsungIapHelper.KEY_NAME_STATUS_CODE);
            errorString = extras.getString(SamsungIapHelper.KEY_NAME_ERROR_STRING);
            itemId = extras.getString(SamsungIapHelper.KEY_NAME_ITEM_ID);
        }
        else
        {
            helper.dismissProgressDialog();
            helper.showIapDialog(getActivity(), "title_iap", "msg_payment_was_not_processed_successfully", false, null);
        }

        if (resultCode == Activity.RESULT_OK)
        {
            if (statusCode == SamsungIapHelper.IAP_ERROR_NONE)
            {
                purchaseVO = new PurchaseVO(extras.getString(SamsungIapHelper.KEY_NAME_RESULT_OBJECT));
                helper.verifyPurchaseResult(getActivity(), purchaseVO);
                ownedItems.add(itemId);
                addPurchaseToLocalInventory(purchaseVO);
            }
            else
            {

                helper.dismissProgressDialog();
                helper.showIapDialog(getActivity(), "title_iap", errorString, false, null);
            }
        }
        else if (resultCode == Activity.RESULT_CANCELED)
        {
            helper.dismissProgressDialog();
            helper.showIapDialog(getActivity(), "title_iap", "msg_payment_cancelled", false, null);
        }


        // How to get the purchaseId??
        int purchaseId = 1;
        //mPurchaseObserver.onPurchaseStateChanged(1, statusCode);
        mPurchaseObserver.onPurchaseStateChanged(itemId, statusCode);

    }

    /////////////////

    public void bindIapService()
    {
        SamsungIapHelper helper = SamsungIapHelper.getInstance(getActivity(), iapMode);
        helper.bindIapService(new SamsungIapHelper.OnIapBindListener()
        {
            @Override
            public void onBindIapFinished(int result)
            {
                SamsungIapHelper helper = SamsungIapHelper.getInstance(getActivity(), iapMode);
                if (pendingPurchaseItemId == null)
                {
                    helper.dismissProgressDialog();
                }

                if (result == SamsungIapHelper.IAP_RESPONSE_RESULT_OK)
                {
                    helper.safeInitIap(getActivity());
                }
                else
                {
                    helper.showIapDialog(getActivity(), "title_iap", "msg_iap_service_bind_failed", false, null);
                }
            }
        });
    }



    public void initBilling()
    {
        //TODO
        this.itemGroupId = "100000102710";
        initHelper();
        started = true;
    }

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

        return true;
    }

    public boolean disableUpdates()
    {
        if (!started)
            return false;
        //TODO
        return true;
    }

    public boolean restorePurchases()
    {
        if (!started)
            return false;

        //TODO;
        return true;
    }
    
    //public boolean sendRequest(int purchaseId, String productId, Map<String, String> properties)
    public boolean sendRequest(int purchaseId, String productId, String developerPayload)
    {
        if (!started)
            return false;
        
        startPurchase(productId);
        return true;
    }

    public boolean productSetType(String productId, String productType)
    {
        return true;
    }

    public boolean consumePurchase(String productID)
    {
        return true;
    }

    public boolean requestProductDetails(String productId)
    {
        //TODO
        return false;
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
        return itemProps.keySet().toString();
    }

    public boolean confirmDelivery(int purchaseId)
    {
        if (!started)
            return false;

        // TODO
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
    
    public Activity getActivity()
    {
        return mActivity;
    }
    
    public void setActivity(Activity activity)
    {
        mActivity = activity;
    }
    
    public void onActivityResult (int requestCode, int resultCode, Intent data)
    {
        switch (requestCode)
        {
            case SamsungIapHelper.REQUEST_CODE_IS_ACCOUNT_CERTIFICATION:
            {
                handleRequestCertification(requestCode, resultCode, data);
                break;
            }
            case SamsungIapHelper.REQUEST_CODE_IS_IAP_PAYMENT:
            {
                handleRequestPayment(requestCode, resultCode, data);
                break;
            }
        }
            
    }
}