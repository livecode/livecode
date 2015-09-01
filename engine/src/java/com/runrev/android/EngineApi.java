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

import java.lang.*;
import android.app.*;
import android.view.*;
import android.content.*;

public interface EngineApi
{
	public interface ActivityResultCallback
	{
		public abstract void handleActivityResult(int resultCode, Intent data);
	};
	
	// Runs the activity then waits for the result, invoking the callback
	// with the resultCode and data. (Must be run from script thread, callback will
	// be on script thread).
	public abstract void runActivity(Intent intent, ActivityResultCallback callback);
	
	public abstract Activity getActivity();
	public abstract ViewGroup getContainer();
};
