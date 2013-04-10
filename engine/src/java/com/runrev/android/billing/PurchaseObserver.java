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

import com.runrev.android.billing.C.ResponseCode;
import com.runrev.android.billing.PurchaseUpdate.Purchase;
import com.runrev.android.billing.BillingService.RequestPurchase;
import com.runrev.android.billing.BillingService.ConfirmNotification;
import com.runrev.android.billing.BillingService.GetPurchaseInformation;
import com.runrev.android.billing.BillingService.RestoreTransactions;

import android.app.*;
import android.content.*;
import android.content.IntentSender.SendIntentException;

public abstract class PurchaseObserver
{
	private static final String TAG = "PurchaseObserver";
	private final Activity mActivity;
	
	public PurchaseObserver(Activity activity)
	{
		mActivity = activity;
	}
	
	public abstract void onBillingSupported(boolean supported);
	public abstract void onPurchaseStateChanged(Purchase p, boolean verified, String signedData, String signature);
	
	public abstract void onRequestPurchaseResponse(RequestPurchase request, ResponseCode responseCode);
	public abstract void onConfirmNotificationResponse(ConfirmNotification request, ResponseCode responseCode);
	public abstract void onRestoreTransactionsResponse(RestoreTransactions request, ResponseCode responseCode);
	
//	public abstract void onGetPurchaseInformationResponse(GetPurchaseInformation request, ResponseCode responseCode);

	void startBuyPageActivity(PendingIntent pendingIntent, Intent intent)
	{
		try
		{
			mActivity.startIntentSender(pendingIntent.getIntentSender(), intent, 0, 0, 0);
		}
		catch (SendIntentException e)
		{
		}
	}
}