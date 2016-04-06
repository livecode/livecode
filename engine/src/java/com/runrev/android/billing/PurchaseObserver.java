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
	
    
    // Sent to the observer to indicate a change in the purchase state
    public abstract void onPurchaseStateChanged(String productId, int state);
    
	// Sent to the observer once product details have been successfully received
    public abstract void onProductDetailsReceived(String productId);
    
    // Sent to the observer once product details have NOT been successfully received
    public abstract void onProductDetailsError(String productId, String error);

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
