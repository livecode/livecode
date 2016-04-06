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

package com.runrev.android;

import android.content.*;
import android.content.pm.*;

public class Utils
{
    public static int[] splitIntegerList(String p_list)
    {
        try
        {
            String[] t_items = p_list.split(",");
            int[] t_list = new int[t_items.length];
            
            for (int i = 0; i < t_items.length; i++)
            {
                t_list[i] = Integer.parseInt(t_items[i].trim());
            }
            
            return t_list;
        }
        catch (Exception e)
        {
            return null;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    
    public static String getLabel(Context context)
    {
        PackageManager pm = context.getPackageManager();
        ApplicationInfo ai = context.getApplicationInfo();
        ai.loadLabel(pm);
        return pm.getApplicationLabel(ai).toString();
    }
    
}
