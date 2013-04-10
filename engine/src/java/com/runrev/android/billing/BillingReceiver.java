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

/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.runrev.android.billing;

import com.runrev.android.Engine;

import com.runrev.android.billing.C.ResponseCode;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

/**
 * This class implements the broadcast receiver for in-app billing. All asynchronous messages from
 * Android Market come to this app through this receiver. This class forwards all
 * messages to the {@link BillingService}, which can start background threads,
 * if necessary, to process the messages. This class runs on the UI thread and must not do any
 * network I/O, database updates, or any tasks that might take a long time to complete.
 * It also must not start a background thread because that may be killed as soon as
 * {@link #onReceive(Context, Intent)} returns.
 *
 * You should modify and obfuscate this code before using it.
 */
public class BillingReceiver
{
    private static final String TAG = "revandroid.BillingReceiver";

    /**
     * This is the entry point for all asynchronous messages sent from Android Market to
     * the application. This method forwards the messages on to the
     * {@link BillingService}, which handles the communication back to Android Market.
     * The {@link BillingService} also reports state changes back to the application through
     * the {@link ResponseHandler}.
     */
    public static boolean onReceive(Context context, Intent intent) {
    	if (Engine.getBillingServiceClass() == null)
    		return false;
    	
        String action = intent.getAction();
        if (C.ACTION_PURCHASE_STATE_CHANGED.equals(action)) {
            String signedData = intent.getStringExtra("inapp_signed_data");
            String signature = intent.getStringExtra("inapp_signature");
            purchaseStateChanged(context, signedData, signature);
            return true;
        } else if (C.ACTION_NOTIFY.equals(action)) {
            String notifyId = intent.getStringExtra("notification_id");
            notify(context, notifyId);
            return true;
        } else if (C.ACTION_RESPONSE_CODE.equals(action)) {
            long requestId = intent.getLongExtra("request_id", -1);
            int responseCodeIndex = intent.getIntExtra("response_code",
                    ResponseCode.RESULT_ERROR.ordinal());
            checkResponseCode(context, requestId, responseCodeIndex);
            return true;
        } else {
            return false;
//            Log.w(TAG, "unexpected action: " + action);
        }
    }

    /**
     * This is called when Android Market sends information about a purchase state
     * change. The signedData parameter is a plaintext JSON string that is
     * signed by the server with the developer's private key. The signature
     * for the signed data is passed in the signature parameter.
     * @param context the context
     * @param signedData the (unencrypted) JSON string
     * @param signature the signature for the signedData
     */
    private static void purchaseStateChanged(Context context, String signedData, String signature) {
        Intent intent = new Intent(C.ACTION_PURCHASE_STATE_CHANGED);
        intent.setClass(context, Engine.getBillingServiceClass());
        intent.putExtra("inapp_signed_data", signedData);
        intent.putExtra("inapp_signature", signature);
        context.startService(intent);
    }

    /**
     * This is called when Android Market sends a "notify" message  indicating that transaction
     * information is available. The request includes a nonce (random number used once) that
     * we generate and Android Market signs and sends back to us with the purchase state and
     * other transaction details. This BroadcastReceiver cannot bind to the
     * MarketBillingService directly so it starts the {@link BillingService}, which does the
     * actual work of sending the message.
     *
     * @param context the context
     * @param notifyId the notification ID
     */
    private static void notify(Context context, String notifyId) {
        Intent intent = new Intent(C.ACTION_GET_PURCHASE_INFORMATION);
        intent.setClass(context, Engine.getBillingServiceClass());
        intent.putExtra("notification_id", notifyId);
        context.startService(intent);
    }

    /**
     * This is called when Android Market sends a server response code. The BillingService can
     * then report the status of the response if desired.
     *
     * @param context the context
     * @param requestId the request ID that corresponds to a previous request
     * @param responseCodeIndex the ResponseCode ordinal value for the request
     */
    private static void checkResponseCode(Context context, long requestId, int responseCodeIndex) {
        Intent intent = new Intent(C.ACTION_RESPONSE_CODE);
        intent.setClass(context, Engine.getBillingServiceClass());
        intent.putExtra("request_id", requestId);
        intent.putExtra("response_code", responseCodeIndex);
        context.startService(intent);
    }
}
