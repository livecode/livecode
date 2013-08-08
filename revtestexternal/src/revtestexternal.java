// Package name needs to be the same as the external qualified name.
// This is to stop conflicts with things like the 'LC' class.
package com.runrev.revtestexternal;

import android.util.*;
import android.app.*;
import android.view.*;
import android.widget.*;
import android.content.*;
import android.net.*;

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
				s_target . Post("buttonPressed");
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
	
	public static String revTestExternalRunActivity()
	{
		Intent t_intent;
		t_intent = new Intent(Intent . ACTION_GET_CONTENT);
		t_intent . setType("image/*");
		
		final String[] t_result = new String[1];
		LC.ActivityRun(t_intent, new LC.ActivityResultCallback() {
			public void handleActivityResult(int p_result_code, Intent p_data)
			{
				if (p_result_code == Activity.RESULT_CANCELED)
					t_result[0] = null;
				else
					t_result[0] = p_data . getDataString();
			}
		});
		
		if (t_result[0] == null)
			return "";
		
		return t_result[0];
	}
	
	public static void revTestExternalRunOnSystemThread()
	{
		final LC.Wait t_wait = new LC.Wait(false);
		
		LC.RunOnSystemThread(new Runnable() {
			public void run()
			{
				AlertDialog.Builder t_dialog;
				t_dialog = new AlertDialog.Builder(LC.InterfaceQueryActivity());
				t_dialog . setTitle("Test Dialog");
				t_dialog . setMessage("Hello World!");
				t_dialog . setOnCancelListener(new DialogInterface.OnCancelListener() {
					public void onCancel(DialogInterface p_dialog)
					{
						t_wait . Break();
					}
				});
				t_dialog . show();
			}
		});
		
		t_wait . Run();
		
		t_wait . Release();
	}
	
	public static void revTestExternalObjectPostAndSend()
	{
		LC.Object t_target;
		t_target = LC.ContextMe();
		
		try
		{
			t_target . Post("handlePost", 1, 1.0, false, "foobar", "foobaz" . getBytes("UTF-8"));
			t_target . Send("handleSend", 1, 1.0, false, "foobar", "foobaz" . getBytes("UTF-8"));
		}
		catch(Exception e)
		{
		}
	}
};
