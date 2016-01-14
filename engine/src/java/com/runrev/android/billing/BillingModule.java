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

import com.runrev.android.Engine;

import android.app.*;
import android.util.*;
import android.content.*;

import java.lang.reflect.*;
import java.util.*;

public class BillingModule
{
    public static final String TAG = "BillingModule";
    
    public BillingProvider getBillingProvider()
    {
        // PM-2015-02-25: [[ Bug 14665 ]] Make sure the "In-app Purchase" checkbox is ticked in standalone settings
        if (!Engine.doGetCustomPropertyValue("cREVStandaloneSettings", "android,InAppPurchasing").equals("true"))
            return null;
        
        Log.d(TAG, "Fetching the billing provider...");
        String t_billing_provider = Engine.doGetCustomPropertyValue("cREVStandaloneSettings", "android,billingProvider");
        Log.d(TAG, "Provider is " + t_billing_provider);
        if (t_billing_provider.equals("Google"))
            return loadBillingProvider("com.runrev.android.billing.google.GoogleBillingProvider");
        else if (t_billing_provider.equals("Amazon"))
            return loadBillingProvider("com.runrev.android.billing.amazon.AmazonBillingProvider");
        else if (t_billing_provider.equals("Samsung"))
            return loadBillingProvider("com.runrev.android.billing.samsung.SamsungBillingProvider");
        else
            return null;
    }
    
    private BillingProvider loadBillingProvider(String p_provider)
    {
        try
        {
            Class t_billing_provider;
            Constructor t_billing_provider_constructor;
            t_billing_provider = Class.forName(p_provider);
            t_billing_provider_constructor = t_billing_provider.getConstructor(new Class[] {});
            return (BillingProvider) t_billing_provider_constructor.newInstance(new Object[] {});
        }
        catch (Exception e)
        {
            return null;
        }
    }
    
}
