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

import com.runrev.android.Engine;
import com.runrev.android.billing.amazon.*;
import com.runrev.android.billing.google.*;
import com.runrev.android.billing.samsung.*;

import android.app.*;
import android.util.*;
import android.content.*;

import java.util.*;

public class BillingModule
{
    public static final String TAG = "BillingModule";
    
    public BillingProvider getBillingProvider()
    {
        Log.d(TAG, "Fetching the billing provider...");
        String t_billing_provider = Engine.doGetCustomPropertyValue("cREVStandaloneSettings", "android,billingProvider");
        Log.d(TAG, "Provider is " + t_billing_provider);
        if (t_billing_provider.equals("Google"))
            return new GoogleBillingProvider();
        else if (t_billing_provider.equals("Amazon"))
            return new AmazonBillingProvider();
        else if (t_billing_provider.equals("Samsung"))
            return new SamsungBillingProvider();
        else
            return null;
        
    }
    
}