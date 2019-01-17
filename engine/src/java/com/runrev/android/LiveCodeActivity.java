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

import android.os.*;
import android.app.*;
import android.view.*;
import android.content.*;
import android.content.res.*;
import android.widget.*;
import android.util.*;
import android.content.pm.PackageManager;
import android.graphics.*;

// This is the main activity exported by the application. This is
// split into two parts, a customizable sub-class that gets dynamically
// generated in the standalone builder and a super-class that is implemented
// in com.runrev.revandroid. The reasoning here is that scripters will want their
// apps to appear in their namespace (and so do we!).
public class LiveCodeActivity extends Activity
{
    public static final String TAG = "revandroid.LiveCodeActivity";
	public static FrameLayout s_main_layout;
	public static Engine s_main_view;

	//////////

	public LiveCodeActivity()
	{
	}

    public Class getServiceClass()
    {
        return LiveCodeService.class;
    }

	//////////

	@Override
	protected void onCreate(Bundle p_saved_state)
	{
		super.onCreate(p_saved_state);
		
        s_main_layout = new FrameLayout(this);

		// Create the view and notify the engine of its attachment (just in case
		// any following methods cause resize and such to be sent!).
		s_main_view = new Engine(this);
		s_main_view.doCreate(this, s_main_layout, s_main_view);
        
        // PM-2015-06-05: [[ Bug 15110 ]] Prevent black flash when setting the acceleratedRendering to true for the very first time
        SurfaceView t_empty_view = new SurfaceView(this);
        s_main_layout . addView(t_empty_view,
								new FrameLayout.LayoutParams(0,0));
		
		// Add the view into the heirarchy
		s_main_layout . addView(s_main_view,
								new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
															 FrameLayout.LayoutParams.MATCH_PARENT));
		setContentView(s_main_layout,
					   new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
												  ViewGroup.LayoutParams.MATCH_PARENT));
        
        // prevent soft keyboard from resizing our view when shown
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        
        s_main_layout.getRootView().getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener()
        {
            @Override
            public void onGlobalLayout()
            {
                s_main_view.updateKeyboardVisible();
            }
        });
	}

	@Override
	protected void onDestroy()
	{
		super.onDestroy();
		s_main_view.clearWakeUp();
		s_main_view.onDestroy();
		s_main_layout = null;
		s_main_view = null;
	}
	
	//////////

	@Override
	protected void onRestart()
	{
		super.onRestart();
		s_main_view.doRestart(s_main_view);
	}
	
	@Override
	protected void onStart()
	{
		super.onStart();
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		s_main_view.onResume();
	}

	@Override
	protected void onPause()
	{
		super.onPause();
		s_main_view.onPause();
	}

	@Override
	protected void onStop()
	{
		super.onStop();
	}
	
	//////////
    
    @Override
    protected void onNewIntent(Intent intent)
    {
        s_main_view.onNewIntent(intent);
    }
    
	//////////

	@Override
	public void onLowMemory()
	{
		super.onLowMemory();

		// During unicodification we will also harden the engine.
		// At this point, we will instruct the engine to minimize
		// its memory usage.

		s_main_view.doLowMemory();
	}

	//////////

	@Override
	protected void onSaveInstanceState(Bundle p_out_state)
	{
	}

	@Override
	public void onConfigurationChanged(Configuration p_new_config)
	{
		super.onConfigurationChanged(p_new_config);
		s_main_view.onConfigurationChanged(p_new_config);
	}

	//////////

	@Override
	public void onBackPressed()
	{
		s_main_view.onBackPressed();
	}
	
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (keyCode == KeyEvent.KEYCODE_MENU && event.getRepeatCount() == 0)
        {
            s_main_view.onMenuKey();
            return true;
        }
        else if (keyCode == KeyEvent.KEYCODE_SEARCH && event.getRepeatCount() == 0)
        {
            s_main_view.onSearchKey();
            return true;
        }
        
        return super.onKeyDown(keyCode, event);
    }

    //////////
    
	static
	{
		System.loadLibrary("revandroid");
	}

////////////////////////////////////////////////////////////////////////////////

	// 
	@Override
	protected void onActivityResult (int requestCode, int resultCode, Intent data)
	{
		s_main_view.onActivityResult(requestCode, resultCode, data);
	}
    
    // Callback sent when the app requests permissions on runtime (Android API 23+)
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        s_main_view.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

}
