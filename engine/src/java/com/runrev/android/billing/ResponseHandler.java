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

package com.runrev.android.billing;

import com.runrev.android.billing.BillingService.RequestPurchase;
import com.runrev.android.billing.BillingService.ConfirmNotification;
import com.runrev.android.billing.BillingService.GetPurchaseInformation;
import com.runrev.android.billing.BillingService.RestoreTransactions;
import com.runrev.android.billing.C.ResponseCode;
import com.runrev.android.billing.PurchaseUpdate.Purchase;

import android.app.*;
import android.content.*;

public class ResponseHandler
{
	private static final String TAG = "ResponseHandler";
	
	private static PurchaseObserver sPurchaseObserver;
	
	public static synchronized void register(PurchaseObserver observer)
	{
		sPurchaseObserver = observer;
	}
	
	public static synchronized void unregister(PurchaseObserver observer)
	{
		sPurchaseObserver = null;
	}
	
	public static void checkBillingSupportedResponse(boolean supported)
	{
		if (sPurchaseObserver == null)
			return;
		
		sPurchaseObserver.onBillingSupported(supported);
	}
	
	public static void buyPageIntentResponse(PendingIntent pendingIntent, Intent intent)
	{
		if (sPurchaseObserver == null)
			return;
		
		sPurchaseObserver.startBuyPageActivity(pendingIntent, intent);
	}
	
	public static void purchaseResponse(Purchase p, boolean verified, String signedData, String signature)
	{
		if (sPurchaseObserver == null)
			return;
		
		sPurchaseObserver.onPurchaseStateChanged(p, verified, signedData, signature);
	}
	
	public static void responseCodeReceived(Context context, RequestPurchase request, ResponseCode responseCode)
	{
		if (sPurchaseObserver == null)
			return;
		
		sPurchaseObserver.onRequestPurchaseResponse(request, responseCode);
	}
	
	public static void responseCodeReceived(Context context, ConfirmNotification request, ResponseCode responseCode)
	{
		if (sPurchaseObserver == null)
			return;
		
		sPurchaseObserver.onConfirmNotificationResponse(request, responseCode);
	}
	
/*
	public static void responseCodeReceived(Context context, GetPurchaseInformation request, ResponseCode responseCode)
	{
		if (sPurchaseObserver == null)
			return;
		
		sPurchaseObserver.onGetPurchaseInformationResponse(request, responseCode);
	}
*/
	
	public static void responseCodeReceived(Context context, RestoreTransactions request, ResponseCode responseCode)
	{
		if (sPurchaseObserver == null)
			return;
		
		sPurchaseObserver.onRestoreTransactionsResponse(request, responseCode);
	}
}
