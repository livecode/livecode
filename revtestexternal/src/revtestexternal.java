// Package name needs to be the same as the external qualified name.
// This is to stop conflicts with things like the 'LC' class.
package com.runrev.revtestexternal;

import android.util.*;
import android.app.*;
import android.view.*;
import android.widget.*;

// We must export a public class with the same name as the external.
public class revtestexternal
{
	// The native button we manage.
	static Button s_button;
	
	// The LC object to send the pressed message to.
	static LC.Object s_target;
	
	public static void revTestExternalAndroidButtonCreate()
	{
		// Get the activity (needed for a context).
		Activity t_activity;
		t_activity = LC.InterfaceQueryActivity();
		
		// Get the target (LC) object.
		s_target = LC.ContextMe();
		
		// Create the Android button and set its text.
		s_button = new Button(t_activity);
		s_button . setText("Hello!");
		
		// Hook up a click listener. In this case we invoke the native
		// method 'nativeButtonPress' we've declared.
		s_button . setOnClickListener(new View.OnClickListener() {
			public void onClick(View v)
			{
				s_target . Post("buttonPressed", "", null);
			}
		});
		
		// Now fetch the top-level container for the app and insert the
		// button fullscreen at the front.
		ViewGroup t_container;
		t_container = LC.InterfaceQueryContainer();
		t_container . addView(s_button, new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
		t_container . bringChildToFront(s_button);
	}
	
	public static void revTestExternalAndroidButtonDestroy()
	{
		// Fetch the top-level container and remove our button from it.
		ViewGroup t_container;
		t_container = LC.InterfaceQueryContainer();
		t_container . removeView(s_button);
		s_button = null;
		s_target = null;
	}
};
