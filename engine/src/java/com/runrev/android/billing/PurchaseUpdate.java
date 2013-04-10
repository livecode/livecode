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

import com.runrev.android.billing.C.PurchaseState;

import org.json.*;

import java.util.*;

public class PurchaseUpdate
{
	public static final String TAG="PurchaseUpdate";
		
	public long nonce;
	public ArrayList<Purchase> orders;
	
	public static class Purchase
	{
		public PurchaseState purchaseState;
		public String notificationId;
		public String productId;
		public String orderId;
		public long purchaseTime;
		public String developerPayload;
		
		public Purchase(PurchaseState purchaseState, String notificationId, String productId,
			String orderId, long purchaseTime, String developerPayload)
		{
			this.purchaseState = purchaseState;
			this.notificationId = notificationId;
			this.productId = productId;
			this.orderId = orderId;
			this.purchaseTime = purchaseTime;
			this.developerPayload = developerPayload;
		}
	}
	
	protected PurchaseUpdate(long nonce, ArrayList<Purchase> orders)
	{
		this.nonce = nonce;
		this.orders = orders;
	}
	
	public static PurchaseUpdate fromJSON(String jsonString)
	{
		JSONObject jObject;
		JSONArray jTransactionsArray = null;
		int numTransactions = 0;
		long nonce = 0L;
		try
		{
			jObject = new JSONObject(jsonString);
			
			nonce = jObject.optLong("nonce");
			jTransactionsArray = jObject.optJSONArray("orders");
			if (jTransactionsArray != null)
				numTransactions = jTransactionsArray.length();
		}
		catch (JSONException e)
		{
			return null;
		}
		
		if (!Security.isNonceKnown(nonce))
		{
			return null;
		}
		
		ArrayList<Purchase> purchases = new ArrayList<Purchase>();
		try
		{
			for (int i = 0; i < numTransactions; i++)
			{
				JSONObject jElement = jTransactionsArray.getJSONObject(i);
				int response = jElement.getInt("purchaseState");
				PurchaseState purchaseState = PurchaseState.valueOf(response);
				String productId = jElement.getString("productId");
				String packageName = jElement.getString("packageName");
				long purchaseTime = jElement.getLong("purchaseTime");
				String orderId = jElement.optString("orderId", "");
				String notifyId = null;
				if (jElement.has("notificationId"))
					notifyId = jElement.getString("notificationId");
				String developerPayload = jElement.optString("developerPayload", null);
				
				purchases.add(new Purchase(purchaseState, notifyId, productId, orderId,
					purchaseTime, developerPayload));
			}
		}
		catch (JSONException e)
		{
			return null;
		}
		
		return new PurchaseUpdate(nonce, purchases);
	}
}
