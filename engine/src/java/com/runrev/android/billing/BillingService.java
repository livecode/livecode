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

package com.runrev.android.billing;

import com.android.vending.billing.IMarketBillingService;

import com.runrev.android.billing.C.ResponseCode;
import com.runrev.android.billing.PurchaseUpdate.Purchase;

import android.app.*;
import android.content.*;
import android.os.*;
import android.util.*;

import java.util.*;

public class BillingService extends Service implements ServiceConnection
{
	public static final String TAG = "revandroid.BillingService";
	private static IMarketBillingService mService;
	
	private static LinkedList<BillingRequest> mPendingRequests = new LinkedList<BillingRequest>();
	
	private static HashMap<Long, BillingRequest> mSentRequests = 
		new HashMap<Long, BillingRequest>();
	
	public static boolean isConnected()
	{
		return mService != null;
	}
	
	protected Bundle makeRequestBundle(String method)
	{
		Bundle request = new Bundle();
		request.putString("BILLING_REQUEST", method);
		request.putInt("API_VERSION", 1);
		request.putString("PACKAGE_NAME", getPackageName());
		return request;
	}
	
	abstract class BillingRequest
	{
		//private final int mStartId;
		protected long mRequestId;
		protected ResponseCode mResponseCode;
		
		/*
		public BillingRequest(int startId)
		{
			mStartId = startId;
		}
		
		public int getStartId()
		{
			return mStartId;
		}
		*/
		
		public boolean runRequest()
		{
			if (runIfConnected())
				return true;
			
			if (bindToMarketBillingService())
			{
				mPendingRequests.add(this);
				return true;
			}
			return false;
		}
		
		public boolean runIfConnected()
		{
			if (mService == null)
				return false;
			
			try
			{
				Bundle request = getRequestBundle();
				Bundle response = mService.sendBillingRequest(request);
			
				mRequestId = response.getLong("REQUEST_ID", -1);
				mResponseCode = ResponseCode.valueOf(response.getInt("RESPONSE_CODE"));
				
				if (checkResponse(response) && mResponseCode == ResponseCode.RESULT_OK && mRequestId >=0)
				{
					mSentRequests.put(mRequestId, this);
				}
				return true;
			}
			catch (RemoteException e)
			{
				onRemoteException(e);
			}
			
			return false;
		}
		
		abstract protected String getMethod();
		
		protected Bundle getRequestBundle()
		{
			return makeRequestBundle(getMethod());
		}
		
		protected boolean checkResponse(Bundle response)
		{
			return true;
		}
		
		protected void onRemoteException(RemoteException e)
		{
			mService = null;
		}
		
		protected void responseCodeReceived(ResponseCode responseCode)
		{
		}
	}

	class CheckBillingSupported extends BillingRequest
	{
		protected String getMethod()
		{
			return "CHECK_BILLING_SUPPORTED";
		}

		protected boolean checkResponse(Bundle response)
		{
			boolean billingSupported = (mResponseCode == ResponseCode.RESULT_OK);
			ResponseHandler.checkBillingSupportedResponse(billingSupported);
			
			return true;
		}
	}
	
    public class RequestPurchase extends BillingRequest
    {
		public final String mProductId;
        public final String mDeveloperPayload;
        public final int mPurchaseId;

        public RequestPurchase(int purchaseId, String itemId)
        {
            this(purchaseId, itemId, null);
        }

        public RequestPurchase(int purchaseId, String itemId, String developerPayload)
        {
        	mPurchaseId = purchaseId;
            mProductId = itemId;
            mDeveloperPayload = developerPayload;
        }

		protected String getMethod()
		{
			return "REQUEST_PURCHASE";
		}
		
		protected Bundle getRequestBundle()
		{
			Bundle request = super.getRequestBundle();
			request.putString("ITEM_ID", mProductId);
            // Note that the developer payload is optional.
            if (mDeveloperPayload != null)
                request.putString("DEVELOPER_PAYLOAD", mDeveloperPayload);
            return request;
		}
		
        @Override
        protected boolean checkResponse(Bundle response)
        {
            PendingIntent pendingIntent
                    = response.getParcelable("PURCHASE_INTENT");
            if (pendingIntent == null)
                return false;

            Intent intent = new Intent();
   	        ResponseHandler.buyPageIntentResponse(pendingIntent, intent);

            return true;
        }

        @Override
        protected void responseCodeReceived(ResponseCode responseCode)
        {
            ResponseHandler.responseCodeReceived(BillingService.this, this, responseCode);
        }
    }

    public class ConfirmNotification extends BillingRequest
    {
    	public final int mPurchaseId;
        final String mNotifyId;

        public ConfirmNotification(int purchaseId, String notifyId)
        {
        	mPurchaseId = purchaseId;
            mNotifyId = notifyId;
        }

		protected String getMethod()
		{
			return "CONFIRM_NOTIFICATIONS";
		}
		
		protected Bundle getRequestBundle()
		{
			Bundle request = super.getRequestBundle();
            request.putStringArray("NOTIFY_IDS", new String[] {mNotifyId});
            return request;
		}
		
        @Override
        protected void responseCodeReceived(ResponseCode responseCode)
        {
        	ResponseHandler.responseCodeReceived(BillingService.this, this, responseCode);
        }
    }
    
    public class GetPurchaseInformation extends BillingRequest
    {
        long mNonce;
        final String mNotifyId;

        public GetPurchaseInformation(String notifyId)
        {
            mNotifyId = notifyId;
        }

		protected String getMethod()
		{
			return "GET_PURCHASE_INFORMATION";
		}
		
		protected Bundle getRequestBundle()
		{
			Bundle request = super.getRequestBundle();
            mNonce = Security.generateNonce();
            request.putLong("NONCE", mNonce);
            request.putStringArray("NOTIFY_IDS", new String[] {mNotifyId});
            return request;
		}
		
        @Override
        protected void onRemoteException(RemoteException e)
        {
            super.onRemoteException(e);
            Security.removeNonce(mNonce);
        }

/*
        @Override
        protected void responseCodeReceived(ResponseCode responseCode)
        {
        	ResponseHandler.responseCodeReceived(BillingService.this, this, responseCode);
        }
*/
    }

    public class RestoreTransactions extends BillingRequest
    {
        long mNonce;

		protected String getMethod()
		{
			return "RESTORE_TRANSACTIONS";
		}
		
		protected Bundle getRequestBundle()
		{
			Bundle request = super.getRequestBundle();
            mNonce = Security.generateNonce();
            request.putLong("NONCE", mNonce);
            return request;
		}

        @Override
        protected void onRemoteException(RemoteException e)
        {
            super.onRemoteException(e);
            Security.removeNonce(mNonce);
        }

        @Override
        protected void responseCodeReceived(ResponseCode responseCode)
        {
            ResponseHandler.responseCodeReceived(BillingService.this, this, responseCode);
        }
    }

	public BillingService()
	{
		super();
	}
	
	public void setContext(Context context)
	{
		attachBaseContext(context);
	}
	
	public IBinder onBind(Intent intent)
	{
		return null;
	}
	
	public void onStart(Intent intent, int startId)
	{
		handleCommand(intent);
	}
	
	public void handleCommand(Intent intent)
	{
		String action = intent.getAction();
		Log.i(TAG, "handleCommand(" + action + ")");
		if (C.ACTION_GET_PURCHASE_INFORMATION.equals(action))
		{
			String notifyId = intent.getStringExtra("notification_id");
			getPurchaseInformation(notifyId);
		}
		else if (C.ACTION_PURCHASE_STATE_CHANGED.equals(action))
		{
			String signedData = intent.getStringExtra("inapp_signed_data");
			String signature = intent.getStringExtra("inapp_signature");
			purchaseStateChanged(signedData, signature);
		}
		else if (C.ACTION_RESPONSE_CODE.equals(action))
		{
			long requestId = intent.getLongExtra("request_id", -1);
			int responseCodeIndex = intent.getIntExtra("response_code", ResponseCode.RESULT_ERROR.ordinal());
			ResponseCode responseCode = ResponseCode.valueOf(responseCodeIndex);
			checkResponseCode(requestId, responseCode);
		}
	}
	
	private boolean bindToMarketBillingService()
	{
		try
		{
			boolean bindResult = bindService(
				new Intent("com.android.vending.billing.MarketBillingService.BIND"),
				this,
				Context.BIND_AUTO_CREATE);
			
			return bindResult;
		}
		catch (SecurityException e)
		{
		}
		
		return false;
	}
	
	public boolean checkBillingSupported()
	{
		return new CheckBillingSupported().runRequest();
	}
	
	public boolean requestPurchase(int purchaseId, String productId, String developerPayload)
	{
		return new RequestPurchase(purchaseId, productId, developerPayload).runRequest();
	}

    public boolean restoreTransactions()
    {
        return new RestoreTransactions().runRequest();
    }
    
    public boolean confirmNotification(int purchaseId, String notifyId)
    {
        return new ConfirmNotification(purchaseId, notifyId).runRequest();
    }
    
    private boolean getPurchaseInformation(String notifyId)
    {
        return new GetPurchaseInformation(notifyId).runRequest();
    }
    
    private void purchaseStateChanged(String signedData, String signature)
    {
	    // TODO - figure out what to do with signed data
	    boolean verified = Security.verify(signedData, signature);
    	PurchaseUpdate purchases = PurchaseUpdate.fromJSON(signedData);
    	
    	if (purchases != null)
    	{
    		Security.removeNonce(purchases.nonce);
    		for (Purchase p : purchases.orders)
	    		ResponseHandler.purchaseResponse(p, verified, signedData, signature);
    	}
    }
    
    private void checkResponseCode(long requestId, ResponseCode responseCode)
    {
    	BillingRequest request = mSentRequests.get(requestId);
    	if (request != null)
    	{
    		request.responseCodeReceived(responseCode);
    	}
    	mSentRequests.remove(requestId);
    }
    
    private void runPendingRequests()
    {
    	BillingRequest request;
    	while ((request = mPendingRequests.peek()) != null)
    	{
    		if (request.runIfConnected())
    			mPendingRequests.remove(request);
    		else
    		{
    			bindToMarketBillingService();
    			return;
    		}
    	}
    }
    
    public void onServiceConnected(ComponentName name, IBinder service)
    {
    	Log.i(TAG, "service connected");
    	mService = IMarketBillingService.Stub.asInterface(service);
    	runPendingRequests();
    }
    
    public void onServiceDisconnected(ComponentName name)
    {
    	mService = null;
    }
    
    public void unbind()
    {
    	try
    	{
    		unbindService(this);
    	}
    	catch (IllegalArgumentException e)
    	{
    		//
    	}
    }
}
