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

package com.runrev.android;

import com.runrev.android.billing.*;
import com.runrev.android.billing.C.ResponseCode;
import com.runrev.android.billing.PurchaseUpdate.Purchase;
import com.runrev.android.billing.BillingService.RestoreTransactions;
import com.runrev.android.billing.BillingService.GetPurchaseInformation;
import com.runrev.android.billing.BillingService.ConfirmNotification;
import com.runrev.android.billing.BillingService.RequestPurchase;

import com.runrev.android.nativecontrol.NativeControlModule;
import com.runrev.android.nativecontrol.VideoControl;

import android.content.*;
import android.content.res.*;
import android.content.pm.*;
import android.database.*;
import android.util.*;
import android.graphics.*;
import android.graphics.drawable.*;
import android.view.*;
import android.view.inputmethod.*;
import android.os.*;
import android.app.*;
import android.text.*;
import android.widget.*;
import android.provider.*;
import android.hardware.*;
import android.media.*;
import android.net.*;
import android.telephony.SmsManager;
import android.os.Vibrator;
import android.os.Environment;
import android.provider.MediaStore.*;
import android.provider.MediaStore.Images.Media;

import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.charset.*;
import java.lang.reflect.*;
import java.util.*;
import java.text.Collator;

// This is the main class that interacts with the engine. Although only one
// instance of the engine is allowed, we still need an object on which we can
// invoke methods from the native code so we wrap all this up into a single
// view object.

public class Engine extends View implements EngineApi
{
	public static final String TAG = "revandroid.Engine";

	// This is true if the engine is not suspended.
	private static boolean s_running;
    private static Engine s_engine_instance;

	private Handler m_handler;
	private boolean m_wake_on_event;
	private boolean m_wake_scheduled;

	private boolean m_video_is_playing;
    private VideoControl m_video_control;

    private BusyIndicator m_busy_indicator_module;
	private TextMessaging m_text_messaging_module;
    private Alert m_beep_vibrate_module;
    private Contact m_contact_module;
    private CalendarEvents m_calendar_module;
    
    private OpenGLView m_opengl_view;
	private OpenGLView m_old_opengl_view;
	private BitmapView m_bitmap_view;

	private File m_temp_image_file;

	private Email m_email;

	private ShakeEventListener m_shake_listener;
	private ScreenOrientationEventListener m_orientation_listener;

	private boolean m_text_editor_visible;
	private int m_text_editor_mode;

    private SensorModule m_sensor_module;
    private DialogModule m_dialog_module;
    private NetworkModule m_network_module;
    private NativeControlModule m_native_control_module;
    private SoundModule m_sound_module;
    private NotificationModule m_notification_module;

    private PowerManager.WakeLock m_wake_lock;
    
    // AL-2013-14-07 [[ Bug 10445 ]] Sort international on Android
    private Collator m_collator;

////////////////////////////////////////////////////////////////////////////////

	public Engine(Context p_context)
	{
		super(p_context);

        s_engine_instance = this;

		// Temporary for testing purposes
		OpenGLView.listConfigs();
		//

		setFocusable(true);
		setFocusableInTouchMode(true);

		// Create the main handler, this simply calls the 'doProcess' method
		// of the engine since we only use it for wake-up notifications.
		m_handler = new Handler() {
			public void handleMessage(Message p_message) {
				m_wake_scheduled = false;
				doProcess(true);
			}
		};

		// create our text editor
        // IM-2012-03-23: switch to monitoring the InputConnection to the EditText field to fix
        // bugs introduced by the previous method of checking for changes to the field
		m_text_editor_mode = 1;
		m_text_editor_visible = false;

        // initialise modules
        m_sensor_module = new SensorModule(this);
        m_dialog_module = new DialogModule(this);
        m_network_module = new NetworkModule(this);
        m_busy_indicator_module = new BusyIndicator (this);
        m_text_messaging_module = new TextMessaging (this);
		m_beep_vibrate_module = new Alert (this);
        m_contact_module = new Contact (this, ((LiveCodeActivity)getContext()));
        m_calendar_module = new CalendarEvents (this, ((LiveCodeActivity)getContext()));
        m_native_control_module = new NativeControlModule(this, ((LiveCodeActivity)getContext()).s_main_layout);
        m_sound_module = new SoundModule(this);
        m_notification_module = new NotificationModule(this);
        
        // MM-2012-08-03: [[ Bug 10316 ]] Initialise the wake lock object.
        PowerManager t_power_manager = (PowerManager) p_context.getSystemService(p_context.POWER_SERVICE);
        m_wake_lock = t_power_manager.newWakeLock(PowerManager.FULL_WAKE_LOCK, TAG);

		// Create listeners for shake events
		m_shake_listener = new ShakeEventListener(p_context)
		{
			public void onShake(int type, long timestamp)
			{
				doShake(type, timestamp);
				// Make sure we trigger handling
				if (m_wake_on_event)
					doProcess(false);
			}
		};
		m_orientation_listener = new ScreenOrientationEventListener(p_context, 0)
		{
			public void onScreenOrientationChanged(int orientation)
			{
				doOrientationChanged(orientation);
				if (m_wake_on_event)
					doProcess(false);
			}
		};

		m_shake_listener.setListening(true);

		// We have no opengl view to begin with.
		m_opengl_view = null;
		m_old_opengl_view = null;

		// But we do have a bitmap view.
		m_bitmap_view = new BitmapView(getContext());
        
        // AL-2013-14-07 [[ Bug 10445 ]] Sort international on Android
        m_collator = Collator.getInstance(Locale.getDefault());
	}

////////////////////////////////////////////////////////////////////////////////

    public void wakeEngineThread()
    {
        post(new Runnable() {
            public void run()
            {
                if (m_wake_on_event)
                    doProcess(false);
            }
        });
    }
	
	public void nativeNotify(int p_callback, int p_context)
	{
		final int t_callback = p_callback;
		final int t_context = p_context;
		post(new Runnable() {
			public void run()
			{
				s_engine_instance . doNativeNotify(t_callback, t_context);
			}});
	}

    public static boolean isRunning()
    {
        return s_running;
    }

    public static Engine getEngine()
    {
        return s_engine_instance;
    }

////////////////////////////////////////////////////////////////////////////////

	public void showSplashScreen()
	{
		m_bitmap_view . showSplashScreen();
	}

	public void hideSplashScreen()
	{
		m_bitmap_view . hideSplashScreen();
	}

////////////////////////////////////////////////////////////////////////////////

	public void clearWakeUp()
	{
		if (m_wake_scheduled)
		{
			m_handler . removeMessages(0);
			m_wake_scheduled = false;
		}
	}

	public void scheduleWakeUp(int p_in_time, boolean p_any_event)
	{
		if (m_wake_scheduled)
		{
			m_handler . removeMessages(0);
			m_wake_scheduled = false;
		}

		m_wake_scheduled = true;
		m_wake_on_event = p_any_event;
		m_handler . sendEmptyMessageDelayed(0, p_in_time);
	}

	public String getPackagePath()
	{
		return getContext() . getApplicationInfo() . sourceDir;
	}

	public void finishActivity()
	{
        // MM-2012-03-19: [[ Bug 10104 ]] Stop tracking any sensors on shutdown - not doing so prevents a restart for some reason.
        if (m_sensor_module != null)
            m_sensor_module.finish();
		((LiveCodeActivity)getContext()).finish();
	}

////////////////////////////////////////////////////////////////////////////////

	public String loadExternalLibrary(String name)
	{
		try
		{
			System . loadLibrary(name);
			return System . mapLibraryName(name);
		}
		catch ( UnsatisfiedLinkError e )
		{
			return null;
		}
		catch ( SecurityException e )
		{
			return null;
		}
	}

////////////////////////////////////////////////////////////////////////////////

    public void onConfigurationChanged(Configuration p_new_config)
	{
	}

////////////////////////////////////////////////////////////////////////////////

	public float getPixelDensity()
	{
		DisplayMetrics t_metrics;
		t_metrics = new DisplayMetrics();
		getWindowManager() . getDefaultDisplay() . getMetrics(t_metrics);
		return t_metrics . density;
	}

////////////////////////////////////////////////////////////////////////////////

	public String getBuildInfo(String p_key)
	{
		try
		{
			if (p_key.startsWith("VERSION."))
			{
				Class t_version_class = Class.forName("android.os.Build$VERSION");
				Field t_field = t_version_class.getField(p_key.substring(p_key.indexOf('.') + 1));
				return t_field.get(null).toString();
			}
			else
			{
				Class t_build_class = Class.forName("android.os.Build");
				Field t_field = t_build_class.getField(p_key);
				return t_field.get(null).toString();
			}
		}
		catch (Exception e)
		{
			Log.i("revandroid", e.toString());
			return null;
		}
	}

////////////////////////////////////////////////////////////////////////////////

	public int getDeviceRotation()
	{
		return m_orientation_listener.getOrientation();
	}

	public int getDisplayOrientation()
	{
		return getContext().getResources().getConfiguration().orientation;
	}

	public int getDisplayRotation()
	{
		WindowManager t_wm = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
		Display t_display = t_wm.getDefaultDisplay();
		// screen rotation direction is opposite to device rotation
		switch (t_display.getRotation())
		{
			case Surface.ROTATION_0:
				return 0;
			case Surface.ROTATION_90:
				return 270;
			case Surface.ROTATION_180:
				return 180;
			case Surface.ROTATION_270:
				return 90;
		}

		return 0;
	}

	private static final int[] s_orientation_map = new int[] {
		ActivityInfo.SCREEN_ORIENTATION_PORTRAIT,
		8, // ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE
		9, // ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT
		ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE,
		};

	public void setDisplayOrientation(int p_orientation)
	{
		if ((p_orientation == 1 || p_orientation == 2) && Build.VERSION.SDK_INT < 9) // Build.VERSION_CODES.GINGERBREAD
			return;
		((LiveCodeActivity)getContext()).setRequestedOrientation(s_orientation_map[p_orientation]);
	}

////////////////////////////////////////////////////////////////////////////////

	protected CharSequence m_composing_text;
	
    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs)
    {
		Log.i(TAG, "onCreateInputConnection()");
		if (!m_text_editor_visible)
			return null;
		
		m_composing_text = null;
        InputConnection t_connection = new BaseInputConnection(this, false) {
			void handleKey(int keyCode, int charCode)
			{
				if (charCode == 0)
				{
					switch (keyCode)
					{
						case KeyEvent.KEYCODE_DEL:
							keyCode = 0xff08;
							break;
							
						default:
					}
				}
				else if (charCode == 10)
				{
					// check for return key
					charCode = 0;
					keyCode = 0xff0d;
				}

				Log.i(TAG, "doing keypress for char " + charCode);
				doKeyPress(0, charCode, keyCode);
			}
			
            @Override
            public boolean sendKeyEvent(KeyEvent key)
            {
                
				int t_key_code = key.getKeyCode();
				int t_char_code = key.getUnicodeChar();
                    
                if (key.getAction() == KeyEvent.ACTION_DOWN)
					handleKey(t_key_code, t_char_code);
				else if (key.getAction() == KeyEvent.ACTION_MULTIPLE)
				{
					// IM-2013-02-21: [[ BZ 10684 ]]
					// allow BaseInputConnection to do the handling of commitText(), etc
					// and instead catch the raw key events that are generated.
					if (key.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN)
					{
						// handle string of chars
						CharSequence t_chars = key.getCharacters();
						for (int i = 0; i < t_chars.length(); i++)
							doKeyPress(0, t_chars.charAt(i), 0);
					}
					else
					{
						// handle repeated char
						for (int i = 0; i < key.getRepeatCount(); i++)
							handleKey(t_key_code, t_char_code);
					}
				}
                
				if (m_wake_on_event)
					doProcess(false);
				
				return true;
            }
			
			// IM-2013-02-25: [[ BZ 10684 ]] - updated to show text changes in the field
			// as software keyboards modify the composing text.
			void updateComposingText(CharSequence p_new)
			{
				// send changes to the engine as a sequence of key events.
				int t_match_length = 0;
				int t_current_length = 0;
				int t_new_length = 0;
				int t_max_length = 0;
				
				if (m_composing_text != null)
					t_current_length = m_composing_text.length();
				if (p_new != null)
					t_new_length = p_new.length();
				
				t_max_length = Math.min(t_current_length, t_new_length);
				for (int i = 0; i < t_max_length; i++)
				{
					if (p_new.charAt(i) != m_composing_text.charAt(i))
						break;
					t_match_length += 1;
				}
				
				// send backspaces
				for (int i = 0; i < t_current_length - t_match_length; i++)
					doKeyPress(0, 0, 0xff08);
				// send new text
				for (int i = t_match_length; i < t_new_length; i++)
					doKeyPress(0, p_new.charAt(i), 0);
				
				m_composing_text = p_new;
				
				if (m_wake_on_event)
					doProcess(false);
			}
			
			// override input connection methods to catch changes to the composing text
			@Override
			public boolean commitText(CharSequence text, int newCursorPosition)
			{
				updateComposingText(text);
				m_composing_text = null;
				getEditable().clear();
				return true;
			}
			@Override
			public boolean finishComposingText()
			{
				m_composing_text = null;
				getEditable().clear();
				return true;
			}
			@Override
			public boolean setComposingText(CharSequence text, int newCursorPosition)
			{
				updateComposingText(text);
				return super.setComposingText(text, newCursorPosition);
			}
        };
        
		int t_type = getInputType(false);
		
        outAttrs.actionLabel = null;
		outAttrs.inputType = t_type;
        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_ENTER_ACTION | EditorInfo.IME_ACTION_DONE;
        
        return t_connection;
    }
    
    public void showKeyboard()
    {
        if (!m_text_editor_visible)
            return;

        requestFocus();
        
        InputMethodManager imm;
        imm = (InputMethodManager)getContext().getSystemService(Context.INPUT_METHOD_SERVICE);

		if (imm != null)
			imm.restartInput(this);
		
        imm.showSoftInput(this, InputMethodManager.SHOW_IMPLICIT);
    }

    public void hideKeyboard()
    {
        // Hide the IME
        InputMethodManager imm;
        imm = (InputMethodManager)getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
		
		if (imm != null)
			imm.restartInput(this);
		
        imm.hideSoftInputFromWindow(getWindowToken(), 0);
    }

	public void resetKeyboard()
	{
		InputMethodManager imm;
		imm = (InputMethodManager)getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
		if (imm != null)
			imm.restartInput(this);
	}

	public void setTextInputVisible(boolean p_visible)
	{
		m_text_editor_visible = p_visible;
		
		if (!s_running)
			return;
		
		if (p_visible)
			showKeyboard();
		else
			hideKeyboard();
	}
	
	public void setTextInputMode(int p_mode)
	{
		// 0 is none
		// 1 is text (normal)
		// 2 is number
		// 3 is decimal
		// 4 is phone
		// 5 is email
		
		boolean t_reset = s_running && m_text_editor_visible && p_mode != m_text_editor_mode;
		
		m_text_editor_mode = p_mode;
		
		if (t_reset)
			resetKeyboard();
	}
	
	public static final int TYPE_NUMBER_VARIATION_PASSWORD = 16;
	public int getInputType(boolean p_password)
	{
		int t_type;
		
		int t_mode = m_text_editor_mode;
		// the phone class does not support a password variant, so we switch this for one of the number types
		if (p_password && t_mode == 4)
			t_mode = 2;
		// the number password variant is not supported pre-honeycomb, so rather than show passwords in plain-text,
		// we switch to the default text input type
		if (p_password && Build.VERSION.SDK_INT < 11 && (t_mode == 2 || t_mode == 3))
			t_mode = 1;
		
		switch(t_mode)
		{
			default:
			case 1:
				t_type = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_MULTI_LINE | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
				if (p_password)
					t_type |= InputType.TYPE_TEXT_VARIATION_PASSWORD;
				break;
			case 2:
				t_type = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED;
				if (p_password)
					t_type |= TYPE_NUMBER_VARIATION_PASSWORD;
				break;
			case 3:
				t_type = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED | InputType.TYPE_NUMBER_FLAG_DECIMAL;
				if (p_password)
					t_type |= TYPE_NUMBER_VARIATION_PASSWORD;
				break;
			case 4:
				t_type = InputType.TYPE_CLASS_PHONE;
				break;
			case 5:
				t_type = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
				if (p_password)
					t_type |= InputType.TYPE_TEXT_VARIATION_PASSWORD;
				break;
		}
		
		return t_type;
	}
    
	public void configureTextInput(int p_mode)
	{
     
		m_text_editor_mode = p_mode;

		if (!s_running)
			return;

		if (p_mode == 0)
		{
            hideKeyboard();
		}
		else
		{
			// Show the IME
            showKeyboard();
		}
	}

    protected void onFocusChanged (boolean gainFocus, int direction, Rect previouslyFocusedRect)
    {
        if (!gainFocus)
        {
            hideKeyboard();
        }
        else if (gainFocus)
        {
			if (m_text_editor_visible)
				showKeyboard();
			else
				hideKeyboard();
        }

        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
    }

////////////////////////////////////////////////////////////////////////////////

	// string utility functions

	public int conversionByteCount(byte[] p_input, String p_in_charset, String p_out_charset)
	{
		Charset t_in_charset = Charset.forName(p_in_charset);
		Charset t_out_charset = Charset.forName(p_out_charset);

		if (t_in_charset == null || t_out_charset == null)
			return 0;

		CharsetDecoder t_decode = t_in_charset.newDecoder();
		CharsetEncoder t_encode = t_out_charset.newEncoder();
		t_decode.replaceWith("?");

		try
		{
			CharBuffer t_utf16;
			t_utf16 = t_decode.decode(ByteBuffer.wrap(p_input));

			ByteBuffer t_encoded;
			t_encoded = t_encode.encode(t_utf16);

			return t_encoded.limit();
		}
		catch (CharacterCodingException e)
		{
			return 0;
		}
	}

	public byte[] convertCharset(byte[] p_input, String p_in_charset, String p_out_charset)
	{
		Charset t_in_charset = Charset.forName(p_in_charset);
		Charset t_out_charset = Charset.forName(p_out_charset);

		if (t_in_charset == null || t_out_charset == null)
		{
			return null;
		}

		CharsetDecoder t_decode = t_in_charset.newDecoder();
		CharsetEncoder t_encode = t_out_charset.newEncoder();
		t_decode.onUnmappableCharacter(CodingErrorAction.REPLACE);
		t_decode.replaceWith("?");
		t_encode.onUnmappableCharacter(CodingErrorAction.REPLACE);

		byte[] t_bytes = null;
		try
		{
			CharBuffer t_utf16;
			t_utf16 = t_decode.decode(ByteBuffer.wrap(p_input));
			ByteBuffer t_encoded;
			t_encoded = t_encode.encode(t_utf16);

			t_bytes = new byte[t_encoded.limit()];
			t_encoded.get(t_bytes);
		}
		catch (Exception e)
		{
			return null;
		}

		return t_bytes;
	}

////////////////////////////////////////////////////////////////////////////////

	static final String s_external_prefix = "external ";
	public String getSpecialFolderPath(String p_name)
	{
		boolean t_external = false;
		if (p_name.startsWith(s_external_prefix))
		{
			t_external = true;
			p_name = p_name.substring(s_external_prefix.length());
		}

		try
		{
			if (p_name.equalsIgnoreCase("documents"))
			{
				if (t_external)
					return getContext().getExternalFilesDir(null).getAbsolutePath();
				else
					return getContext().getFilesDir().getAbsolutePath();
			}
			else if (p_name.equalsIgnoreCase("temporary") || p_name.equalsIgnoreCase("cache"))
			{
				if (t_external)
					return getContext().getExternalCacheDir().getAbsolutePath();
				else
					return getContext().getCacheDir().getAbsolutePath();
			}
		}
		catch (Exception e)
		{
			return "";
		}

		return "";
	}

////////////////////////////////////////////////////////////////////////////////

	public int getAssetFileLength(String p_path)
	{
		int t_result;
		try
		{
			AssetFileDescriptor t_descriptor;
			t_descriptor = getContext() . getAssets() . openFd(p_path);
			t_result = (int)t_descriptor . getLength();
			t_descriptor . close();
		}
		catch(Exception e)
		{
			t_result = -1;
		}
		return t_result;
	}

	public int getAssetFileStartOffset(String p_path)
	{
		int t_result;
		try
		{
			AssetFileDescriptor t_descriptor;
			t_descriptor = getContext() . getAssets() . openFd(p_path);
			t_result = (int)t_descriptor . getStartOffset();
			t_descriptor . close();
		}
		catch(Exception e)
		{
			t_result = -1;
		}
		return t_result;
	}

	public int getAssetInfo(String p_filename, int p_field)
	{
		if (p_field == 0)
			return getAssetFileStartOffset(p_filename);
		else
			return getAssetFileLength(p_filename);
	}

////////////////////////////////////////////////////////////////////////////////

	// asset file access functions
	public boolean isAssetFile(String p_path)
	{
		try
		{
			InputStream t_fstream = getContext().getAssets().open(p_path);
			t_fstream.close();
			return true;
		}
		catch (IOException e)
		{
			return false;
		}
	}

	public boolean isAssetFolder(String p_path)
	{
		if (isAssetFile(p_path))
			return false;
		try
		{
			// MW-2011-03-08: If the path has a trailing '/' then remove it
			//   as the asset manager doesn't like it like that.
			if (p_path.endsWith("/"))
				p_path = p_path.substring(0, p_path.length() - 1);
			String[] t_contents = getContext().getAssets().list(p_path);
			return t_contents.length > 0;
		}
		catch (IOException e)
		{
			return false;
		}
	}

	public String getAssetFolderEntryList(String p_path)
	{
		if (!isAssetFolder(p_path))
			return null;

		// MW-2011-03-08: If the path has a trailing '/' then remove it
		//   as the asset manager doesn't like it like that.
		if (p_path.endsWith("/"))
			p_path = p_path.substring(0, p_path.length() - 1);

		StringBuilder t_folderlist = new StringBuilder();
		String[] t_contents;
		try
		{
			t_contents = getContext().getAssets().list(p_path);
		}
		catch (IOException e)
		{
			return null;
		}
		for (String t_item : t_contents)
		{
			int t_item_size;
			boolean t_is_folder;

			String t_itempath;
			if (p_path.length() == 0)
				t_itempath=t_item;
			else
				t_itempath=p_path + '/' + t_item;

			t_is_folder = !isAssetFile(t_itempath);
			if (t_is_folder)
				t_item_size = 0;
			else
				t_item_size = getAssetFileLength(t_itempath);

			if (t_folderlist.length() > 0)
				t_folderlist.append('\n');
			t_folderlist.append(t_item);
			t_folderlist.append('\n');
			t_folderlist.append(Integer.toString(t_item_size) + "," + t_is_folder);
		}

		String t_result = t_folderlist.toString();
		return t_result;
	}

////////////////////////////////////////////////////////////////////////////////

    // native control functionality
    void addNativeControl(Object p_control)
    {
        m_native_control_module.addControl(p_control);
    }

    void removeNativeControl(Object p_control)
    {
        m_native_control_module.removeControl(p_control);
    }

    Object createBrowserControl()
    {
        return m_native_control_module.createBrowser();
    }

    Object createScrollerControl()
    {
        return m_native_control_module.createScroller();
    }
    
    Object createPlayerControl()
    {
        return m_native_control_module.createPlayer();
    }
    
    Object createInputControl()
    {
        return m_native_control_module.createInput();
    }

////////////////////////////////////////////////////////////////////////////////

	// IM-2013-04-17: [[ AdModule ]] attempt to load AdModule class, returning null if the class cannot be found
	Object loadAdModule()
	{
		try
		{
			Class t_ad_module_class;
			Constructor t_ad_module_constructor;

			t_ad_module_class = Class.forName("com.runrev.android.AdModule");
			t_ad_module_constructor = t_ad_module_class.getConstructor(new Class[] {Engine.class, ViewGroup.class});

			return t_ad_module_constructor.newInstance(new Object[] {this, ((LiveCodeActivity)getContext()).s_main_layout});
		}
		catch (Exception e)
		{
			return null;
		}
	}

////////////////////////////////////////////////////////////////////////////////

	// Called by MCScreenDC::popupanswerdialog()
	public void popupAnswerDialog(String p_title, String p_message, String p_ok_button, String p_cancel_button, String p_other_button)
	{
        m_dialog_module.showAnswerDialog(p_title, p_message, p_ok_button, p_cancel_button, p_other_button);
    }
    public void onAnswerDialogDone(int p_which)
    {
        doAnswerDialogDone(p_which);
    }

	public void popupAskDialog(boolean p_is_password, String p_title, String p_message, String p_initial, boolean p_hint)
    {
        m_dialog_module.showAskDialog(p_is_password, p_title, p_message, p_initial, p_hint);
    }
    public void onAskDialogDone(String p_result)
    {
        doAskDialogDone(p_result);
    }

    public void showDatePicker(boolean p_with_min, boolean p_with_max, int p_min, int p_max, int p_current)
    {
        m_dialog_module.showDatePicker(p_with_min, p_with_max, p_min, p_max, p_current);
    }
    public void onDatePickerDone(int p_year, int p_month, int p_day, boolean p_done)
    {
        doDatePickerDone(p_year, p_month, p_day, p_done);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void showTimePicker(int p_hour, int p_minute)
    {
        m_dialog_module.showTimePicker(p_hour, p_minute);
    }
    public void onTimePickerDone(int p_hour, int p_minute, boolean p_done)
    {
        doTimePickerDone(p_hour, p_minute, p_done);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void showListPicker(List p_items, String p_title, boolean p_item_selected, int p_selection_index, boolean p_use_checkmark, boolean p_use_cancel, boolean p_use_done)
    {
        String[] t_items;
        t_items = (String[])p_items.toArray(new String[p_items.size()]);
        m_dialog_module.showListPicker(t_items, p_title, p_item_selected, p_selection_index, p_use_checkmark, p_use_cancel, p_use_done);
    }
    public void onListPickerDone(int p_index, boolean p_done)
    {
        doListPickerDone(p_index, p_done);
        if (m_wake_on_event)
            doProcess(false);
    }

////////////////////////////////////////////////////////////////////////////////

	public String getSystemVersion()
	{
		return Build.VERSION.RELEASE;
	}

	public String getMachine()
	{
		return Build.MODEL;
	}

	private String rectToString(int left, int top, int right, int bottom)
	{
		return String.format("%d,%d,%d,%d", left, top, right, bottom);
	}

	private String rectToString(Rect t_rect)
	{
		return rectToString(t_rect.left, t_rect.top, t_rect.right, t_rect.bottom);
	}

	private WindowManager getWindowManager()
	{
		return (WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE);
	}

	public String getViewportAsString()
	{
		DisplayMetrics t_metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(t_metrics);
		return rectToString(0, 0, t_metrics.widthPixels, t_metrics.heightPixels);
	}

	private Rect getWorkarea()
	{
		Rect t_workrect = new Rect();
		getGlobalVisibleRect(t_workrect);
		return t_workrect;
	}

	public String getWorkareaAsString()
	{
		return rectToString(getWorkarea());
	}

////////////////////////////////////////////////////////////////////////////////

	private static final int STATUS_BAR_VISIBLE = 0;
	private static final int STATUS_BAR_HIDDEN = 1;
	public void setStatusbarVisibility(boolean p_visible)
	{
		try
		{
			Method t_setSystemUiVisibility;
			t_setSystemUiVisibility = this.getClass().getMethod("setSystemUiVisibility", new Class[] { Integer.TYPE });
			t_setSystemUiVisibility.invoke(this, new Object[] { new Integer(p_visible ? STATUS_BAR_VISIBLE : STATUS_BAR_HIDDEN) });
		}
		catch (Exception e)
		{
			int t_flags, t_mask;
			t_flags = p_visible ? 0 : WindowManager.LayoutParams.FLAG_FULLSCREEN;
			t_mask = WindowManager.LayoutParams.FLAG_FULLSCREEN;
			((Activity)getContext()).getWindow().setFlags(t_flags, t_mask);
		}
	}

	public boolean getStatusbarVisibility()
	{
		return 0 == (((Activity)getContext()).getWindow().getAttributes().flags & WindowManager.LayoutParams.FLAG_FULLSCREEN);
	}

////////////////////////////////////////////////////////////////////////////////

	public void onBackPressed()
	{
		if (!m_native_control_module.handleBackPressed())
			doBackPressed();

		// Make sure we trigger handling
		if (m_wake_on_event)
			doProcess(false);
	}

    public void onMenuKey()
    {
        doMenuKey();
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onSearchKey()
    {
        doSearchKey();
        if (m_wake_on_event)
            doProcess(false);
    }

	@Override
	public boolean onTouchEvent(MotionEvent p_event)
	{
		if (!hasFocus())
        {
            requestFocus();
        }

		int t_pointer_count;
		t_pointer_count = p_event . getPointerCount();

		int t_action = p_event.getActionMasked();
		if (t_action == MotionEvent.ACTION_POINTER_DOWN || t_action == MotionEvent.ACTION_POINTER_UP)
		{
			int i = p_event.getActionIndex();
			doTouch(t_action, p_event.getPointerId(i), (int)p_event.getEventTime(), (int)p_event.getX(i), (int)p_event.getY(i));
		}
		else
		{
			for(int i = 0; i < t_pointer_count; i++)
				doTouch(t_action, p_event . getPointerId(i), (int)p_event . getEventTime(), (int)p_event . getX(i), (int)p_event . getY(i));
		}

		// Make sure we trigger handling
		if (m_wake_on_event)
			doProcess(false);

		return true;
	}

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		// If the height changes, we assume its a keyboard show event so don't
		// resize but do send notifications.
		if (w == oldw && h != oldh)
		{
			if (h > oldh)
				doKeyboardHidden();
			else
				doKeyboardShown(oldh - h);
			return;
		}
		
		m_bitmap_view . resizeBitmap(w, h);

		doReconfigure(w, h, m_bitmap_view . getBitmap());

		// Make sure we trigger handling
		if (m_wake_on_event)
			doProcess(false);
	}

	@Override
	protected void onAttachedToWindow()
	{
		((ViewGroup)getParent()).addView(m_bitmap_view, 0,
										 new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
																	  FrameLayout.LayoutParams.MATCH_PARENT));
	}
	
	public void invalidateBitmap(int l, int t, int r, int b)
	{
		if (m_bitmap_view != null && m_bitmap_view.getVisibility() == View.VISIBLE)
			m_bitmap_view.invalidate(l, t, r, b);
		else if (m_opengl_view != null && m_opengl_view.getVisibility() == View.VISIBLE)
			m_opengl_view.invalidate(l, t, r, b);
	}

////////////////////////////////////////////////////////////////////////////////

	// enable/disable sensor events
    public boolean isSensorAvailable(int p_sensor)
    {
        return m_sensor_module.isSensorAvailable(p_sensor);
    }

    public boolean startTrackingLocation(boolean p_loosely)
    {
        return m_sensor_module.startTrackingLocation(p_loosely);
    }
    public boolean stopTrackingLocation()
    {
        return m_sensor_module.stopTrackingLocation();
    }

    public boolean startTrackingHeading(boolean p_loosely)
    {
        return m_sensor_module.startTrackingHeading(p_loosely);
    }
    public boolean stopTrackingHeading()
    {
        return m_sensor_module.stopTrackingHeading();
    }

    public boolean startTrackingAcceleration(boolean p_loosely)
    {
        return m_sensor_module.startTrackingAcceleration(p_loosely);
    }
    public boolean stopTrackingAcceleration()
    {
        return m_sensor_module.stopTrackingAcceleration();
    }

    public boolean startTrackingRotationRate(boolean p_loosely)
    {
        return m_sensor_module.startTrackingRotationRate(p_loosely);
    }
    public boolean stopTrackingRotationRate()
    {
        return m_sensor_module.stopTrackingRotationRate();
    }

    public void onAccelerationChanged(float p_x, float p_y, float p_z, float p_timestamp)
    {
        doAccelerationChanged(p_x, p_y, p_z, p_timestamp);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onLocationChanged(double p_latitude, double p_longitude, double p_altitude, float p_timestamp, float p_accuracy, double p_speed, double p_course)
    {
        // MM-2013-02-21: Added spead and course to location readings.
        doLocationChanged(p_latitude, p_longitude, p_altitude, p_timestamp, p_accuracy, p_speed, p_course);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onHeadingChanged(double p_heading, double p_magnetic_heading, double p_true_heading, float p_timestamp,
                                 float p_x, float p_y, float p_z, float p_accuracy)
    {
        doHeadingChanged(p_heading, p_magnetic_heading, p_true_heading, p_timestamp, p_x, p_y, p_z, p_accuracy);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onRotationRateChanged(float p_x, float p_y, float p_z, float p_timestamp)
    {
        doRotationRateChanged(p_x, p_y, p_z, p_timestamp);
        if (m_wake_on_event)
            doProcess(false);
    }

////////////////////////////////////////////////////////////////////////////////

	// network functions

    public String getNetworkInterfaces()
    {
        return m_network_module.getNetworkInterfaces();
    }

	// url function interface

	public void setURLTimeout(int p_timeout)
	{
        m_network_module.setURLTimeout(p_timeout);
	}
	
	public void setURLSSLVerification(boolean p_enabled)
	{
		m_network_module.setURLSSLVerification(p_enabled);
	}

	public boolean loadURL(int p_id, String p_url, String p_headers)
	{
        return m_network_module.loadURL(p_id, p_url, p_headers);
	}

	public boolean postURL(int p_id, String p_url, String p_headers, byte[] p_post_data)
	{
        return m_network_module.postURL(p_id, p_url, p_headers, p_post_data);
	}

	public boolean putURL(int p_id, String p_url, String p_headers, byte[] p_send_data)
	{
        return m_network_module.putURL(p_id, p_url, p_headers, p_send_data);
	}

    public void onUrlDidStart(int p_id)
    {
        doUrlDidStart(p_id);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onUrlDidConnect(int p_id, int p_content_length)
    {
        doUrlDidConnect(p_id, p_content_length);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onUrlDidSendData(int p_id, int p_sent)
    {
        doUrlDidSendData(p_id, p_sent);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onUrlDidReceiveData(int p_id, byte p_bytes[], int p_length)
    {
        doUrlDidReceiveData(p_id, p_bytes, p_length);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onUrlDidFinish(int p_id)
    {
        doUrlDidFinish(p_id);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onUrlError(int p_id, String p_err_msg)
    {
        doUrlError(p_id, p_err_msg);
        if (m_wake_on_event)
            doProcess(false);
    }

	////////

	public void launchUrl(String p_url)
	{
		String t_type = null;
		if (p_url.startsWith("file:"))
		{
			try
			{
				t_type = URLConnection.guessContentTypeFromStream(new URL(p_url).openStream());
			}
			catch (IOException e)
			{
				t_type = null;
			}

			if (t_type == null)
				t_type = URLConnection.guessContentTypeFromName(p_url);
		}

		Intent t_view_intent = new Intent(Intent.ACTION_VIEW);
		if (t_type != null)
			t_view_intent.setDataAndType(Uri.parse(p_url), t_type);
		else
			t_view_intent.setData(Uri.parse(p_url));
		((LiveCodeActivity)getContext()).startActivityForResult(t_view_intent, LAUNCHURL_RESULT);
	}

	public void onLaunchUrlResult(int resultCode, Intent data)
	{
		// void
	}

////////////////////////////////////////////////////////////////////////////////

	// audio playback
    // MM-2012-02-10: Refactored audio playbak into sound module.

	public boolean playSound(String p_path, boolean p_is_asset, boolean p_loop)
	{
        return m_sound_module.playSound(p_path, p_is_asset, p_loop);
	}

	public int getPlayLoudness()
	{
		return m_sound_module.getPlayLoudness();
	}

	public boolean setPlayLoudness(int p_loudness)
	{
		return m_sound_module.setPlayLoudness(p_loudness);
	}

    public boolean playSoundOnChannel(String p_channel, String p_sound, String p_sound_path, int p_type, boolean p_is_asset, long p_callback_handle)
    {
        return m_sound_module.playSoundOnChannel(p_sound, p_channel, p_type, p_sound_path, p_is_asset, p_callback_handle);
    }

    public boolean stopSoundOnChannel(String p_channel)
    {
        return m_sound_module.stopPlayingOnChannel(p_channel);
    }

    public boolean pauseSoundOnChannel(String p_channel)
    {
        return m_sound_module.pausePlayingOnChannel(p_channel);
    }
    public boolean resumeSoundOnChannel(String p_channel)
    {
        return m_sound_module.resumePlayingOnChannel(p_channel);
    }

    public boolean deleteSoundChannel(String p_channel)
    {
        return m_sound_module.deleteSoundChannel(p_channel);
    }

    public boolean setSoundChannelVoulme(String p_channel, int p_volume)
    {
        return m_sound_module.setChannelVolume(p_channel, p_volume);
    }

    public int getSoundChannelVolume(String p_channel)
    {
        return m_sound_module.getChannelVolume(p_channel);
    }

    public int getSoundChannelStatus(String p_channel)
    {
        return m_sound_module.getChannelStatus(p_channel);
    }

    public String getSoundOnChannel(String p_channel)
    {
        return m_sound_module.getSoundOnChannel(p_channel);
    }

    public String getNextSoundOnChannel(String p_channel)
    {
        return m_sound_module.getNextSoundOnChannel(p_channel);
    }

    public String getSoundChannels()
    {
        return m_sound_module.getSoundChannels();
    }

////////////////////////////////////////////////////////////////////////////////

	// video playback

	public boolean playVideo(String p_path, boolean p_is_asset, boolean p_is_url, boolean p_looping, boolean p_show_controller)
	{
		if (p_path == null || p_path.length() == 0)
		{
			stopVideo();
			return true;
		}
        
        boolean t_success = true;

        m_video_control = (VideoControl)m_native_control_module.createPlayer();
        m_native_control_module.addControl(m_video_control);

		Rect t_workarea = getWorkarea();
        m_video_control.setRect(0, 0, t_workarea.right - t_workarea.left, t_workarea.bottom - t_workarea.top);
        
        if (p_is_url)
            t_success = m_video_control.setUrl(p_path);
        else
            t_success = m_video_control.setFile(p_path, p_is_asset);

        if (t_success)
        {
            m_video_control.setOnCompletionListener(new MediaPlayer.OnCompletionListener()
            {
                @Override
                public void onCompletion(MediaPlayer mp)
                {
                    Log.i(TAG, "videocompletion");
                    stopVideo();
                }
            });
            m_video_control.setOnErrorListener(new MediaPlayer.OnErrorListener() {
                @Override
                public boolean onError(MediaPlayer mp, int what, int extra)
                {
                    m_video_is_playing = false;
                    return false;
                }
            });
            m_video_control.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
                @Override
                public void onPrepared(MediaPlayer mp)
                {
                }
            });
            m_video_control.setOnVideoSizeChangedListener(new MediaPlayer.OnVideoSizeChangedListener() {
                @Override
                public void onVideoSizeChanged(MediaPlayer mp, int width, int height)
                {
                }
            });
			
			m_video_control.setOnMovieTouchedListener(new VideoControl.OnMovieTouchedListener() {
				@Override
				public void onMovieTouched()
				{
					doMovieTouched();
					if (m_wake_on_event)
						doProcess(false);
				}
			});
            m_video_control.setShowController(p_show_controller);
            m_video_control.setVisible(true);
            m_video_control.start();
        }
        else
		{
			stopVideo();
			return false;
		}

		return true;
	}

	public void stopVideo()
	{
		m_video_is_playing = false;
		if (m_video_control != null)
		{
			m_video_control.stop();

            m_native_control_module.removeControl(m_video_control);
            
            m_video_control = null;
		}
		doMovieStopped();
		if (m_wake_on_event)
			doProcess(false);
	}

////////////////////////////////////////////////////////////////////////////////

	// camera information


	public int getCameraCount()
	{
		return CameraCompat.getNumberOfCameras();
	}


	public String getCameraDirections()
	{
		int t_count = CameraCompat.getNumberOfCameras();
		char[] t_directions = new char[t_count];

		CameraCompat.CameraInfo t_caminfo = new CameraCompat.CameraInfo();
		for (int i = 0; i < t_count; i++)
		{
			CameraCompat.getCameraInfo(i, t_caminfo);
			if (t_caminfo.facing == CameraCompat.CameraInfo.CAMERA_FACING_BACK)
				t_directions[i] = 'b';
			else if (t_caminfo.facing == CameraCompat.CameraInfo.CAMERA_FACING_FRONT)
				t_directions[i] = 'f';
			else
				t_directions[i] = '?';
		}
		return new String(t_directions);
	}

	public void showPhotoPicker(String p_source)
	{
		if (p_source.equals("camera"))
			showCamera();
		else if (p_source.equals("album"))
			showLibrary();
		else if (p_source.equals("library"))
			showLibrary();
		else
		{
			doPhotoPickerError("source not available");
		}
	}

	public void showCamera()
	{
		// 2012-01-18-IM temp file may be created in app cache folder, in which case
		// the file needs to be made world-writable

		boolean t_have_temp_file = false;
		
		Activity t_activity = (LiveCodeActivity)getContext();
		
		try
		{
			// try external cache first
			m_temp_image_file = File.createTempFile("img", ".jpg", t_activity.getExternalCacheDir());
			m_temp_image_file.setWritable(true, false);
			t_have_temp_file = true;
		}
		catch (IOException e)
		{
			m_temp_image_file = null;
			t_have_temp_file = false;
		}

		if (!t_have_temp_file)
		{
			try
			{
				// now try internal cache - should succeed but may not have enough space on the device for large image files
				m_temp_image_file = File.createTempFile("img", ".jpg", t_activity.getCacheDir());
				m_temp_image_file.setWritable(true, false);
				t_have_temp_file = true;
			}
			catch (IOException e)
			{
				m_temp_image_file = null;
				t_have_temp_file = false;
			}
		}
		
		if (!t_have_temp_file)
		{
			doPhotoPickerError("error: could not create temporary image file");
			return;
		}
		

		Uri t_tmp_uri = Uri.fromFile(m_temp_image_file);

		Intent t_image_capture = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		t_image_capture.putExtra(MediaStore.EXTRA_OUTPUT, t_tmp_uri);
		t_activity.startActivityForResult(t_image_capture, IMAGE_RESULT);
	}

	public void showLibrary()
	{
		Intent t_get_content = new Intent(Intent.ACTION_GET_CONTENT);
		t_get_content.setType("image/*");
		((LiveCodeActivity)getContext()).startActivityForResult(t_get_content, IMAGE_RESULT);
	}

	private void onImageResult(int resultCode, Intent data)
	{
		if (resultCode == Activity.RESULT_CANCELED)
		{
			doPhotoPickerCanceled();
		}
		else if (resultCode == Activity.RESULT_OK)
		{
			try
			{
				Uri t_photo_uri = null;
				if (m_temp_image_file != null)
					t_photo_uri = Uri.fromFile(m_temp_image_file);
				else
					t_photo_uri = data.getData();
				InputStream t_in = ((LiveCodeActivity)getContext()).getContentResolver().openInputStream(t_photo_uri);
				ByteArrayOutputStream t_out = new ByteArrayOutputStream();
				byte[] t_buffer = new byte[4096];
				int t_readcount;
				while (-1 != (t_readcount = t_in.read(t_buffer)))
				{
					t_out.write(t_buffer, 0, t_readcount);
				}
				doPhotoPickerDone(t_out.toByteArray(), t_out.size());
				t_in.close();
				t_out.close();
			}
			catch (Exception e)
			{
				doPhotoPickerError("error: could not get image data");
			}
			if (m_temp_image_file != null)
			{
				m_temp_image_file.delete();
				m_temp_image_file = null;
			}
		}
		if (m_wake_on_event)
			doProcess(false);
	}

////////////////////////////////////////////////////////////////////////////////

	// email sending

	public boolean canSendMail()
	{
		Intent t_mail_intent = new Intent(Intent.ACTION_SEND_MULTIPLE);
		t_mail_intent.putExtra(Intent.EXTRA_EMAIL, "user@email.com");
		t_mail_intent.putExtra(Intent.EXTRA_SUBJECT, "subject");
		t_mail_intent.putExtra(Intent.EXTRA_TEXT, "message");
		t_mail_intent.setType("text/plain");

		return t_mail_intent.resolveActivity(getContext().getPackageManager()) != null;
	}

	public void sendEmail(String address, String cc, String subject, String message_body)
	{
		prepareEmail(address, cc, null, subject, message_body, false);
		sendEmail();
	}

	public void prepareEmail(String address, String cc, String bcc, String subject, String message_body, boolean is_html)
	{
		m_email = new Email(address, cc, bcc, subject, message_body, is_html);
	}

	public void addAttachment(String path, String mime_type, String name)
	{
		m_email.addAttachment(path, mime_type, name);
	}

	public void addAttachment(byte[] data, String mime_type, String name)
	{
		m_email.addAttachment(data, mime_type, name);
	}

	public void sendEmail()
	{
		((LiveCodeActivity)getContext()).startActivityForResult(m_email.createIntent(), EMAIL_RESULT);
	}

	private void onEmailResult(int resultCode, Intent data)
	{
		m_email.cleanupTempFiles();

		if (resultCode == Activity.RESULT_CANCELED)
		{
			doMailCanceled();
		}
		else if (resultCode == Activity.RESULT_OK)
		{
			doMailDone();
		}
		else
		{
			doMailCanceled();
		}

		if (m_wake_on_event)
			doProcess(false);
	}

////////////////////////////////////////////////////////////////////////////////

	private static final int IMAGE_RESULT = 1;
	private static final int EMAIL_RESULT = 2;
	private static final int LAUNCHURL_RESULT = 3;
	private static final int TEXT_RESULT = 4;
    private static final int MEDIA_RESULT = 5;
    private static final int PICK_CONTACT_RESULT = 6;
    private static final int CREATE_CONTACT_RESULT = 7;
    private static final int UPDATE_CONTACT_RESULT = 8;
    private static final int SHOW_CONTACT_RESULT = 9;
    private static final int PICK_CALENDAR_RESULT = 10;
    private static final int CREATE_CALENDAR_RESULT = 11;
    private static final int UPDATE_CALENDAR_RESULT = 12;
    private static final int SHOW_CALENDAR_RESULT = 13;
	
	// MW-2013-08-07: [[ ExternalsApiV5 ]] Activity result code for activities
	//   launched through 'runActivity' API.
	private static final int RUN_ACTIVITY_RESULT = 14;

	public void onActivityResult (int requestCode, int resultCode, Intent data)
	{
		switch (requestCode)
		{
			case IMAGE_RESULT:
				onImageResult(resultCode, data);
				break;
			case EMAIL_RESULT:
				onEmailResult(resultCode, data);
				break;
			case LAUNCHURL_RESULT:
				onLaunchUrlResult(resultCode, data);
				break;
			case TEXT_RESULT:
				onTextResult(resultCode, data);
				break;
			case MEDIA_RESULT:
				onMediaResult(resultCode, data);
				break;
			case PICK_CONTACT_RESULT:
				onPickContactResult(resultCode, data);
				break;
			case CREATE_CONTACT_RESULT:
				onCreateContactResult(resultCode, data);
				break;
			case UPDATE_CONTACT_RESULT:
				onUpdateContactResult(resultCode, data);
				break;
			case SHOW_CONTACT_RESULT:
				onShowContactResult(resultCode, data);
				break;
			case CREATE_CALENDAR_RESULT:
                onCreateCalendarEventResult(resultCode, data);
				break;
			case UPDATE_CALENDAR_RESULT:
				onUpdateCalendarEventResult(resultCode, data);
				break;
			case SHOW_CALENDAR_RESULT:
				onShowCalendarEventResult(resultCode, data);
				break;
			// MW-2013-08-07: [[ ExternalsApiV5 ]] Dispatch the activity result
			//   to the 'runActivity' handler.
			case RUN_ACTIVITY_RESULT:
				onRunActivityResult(resultCode, data);
				break;
			default:
				break;
		}
	}

////////////////////////////////////////////////////////////////////////////////

	public void enableOpenGLView()
	{
		// If OpenGL is already enabled, do nothing.
		if (m_opengl_view != null)
			return;

		Log.i("revandroid", "enableOpenGLView");


		// If we have an old OpenGL view, use it.
		if (m_old_opengl_view != null)
		{
			m_opengl_view = m_old_opengl_view;
			m_old_opengl_view = null;
		}

		// Create the OpenGL view, if needed.
		if (m_opengl_view == null)
		{
			m_opengl_view = new OpenGLView(getContext());

			// Add the view to the hierarchy - we add at the bottom and bring to
			// the front as soon as we've shown the first frame.
			((ViewGroup)getParent()).addView(m_opengl_view, 0,
											 new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
																		  FrameLayout.LayoutParams.MATCH_PARENT));
		}
	}

	public void disableOpenGLView()
	{
		// If OpenGL is not enabled, do nothing.
		if (m_opengl_view == null)
			return;

		Log.i("revandroid", "disableOpenGLView");

		// Before removing the OpenGL mode, make sure we show the bitmap view.
		m_bitmap_view.setVisibility(View.VISIBLE);

		// Move the current opengl view to old.
		m_old_opengl_view = m_opengl_view;
		m_opengl_view = null;

		// Post an runnable that removes the OpenGL view. Doing that here will
		// cause a black screen.
		post(new Runnable() {
			public void run() {
				if (m_old_opengl_view == null)
					return;

				Log.i("revandroid", "disableOpenGLView callback");
				((ViewGroup)m_old_opengl_view.getParent()).removeView(m_old_opengl_view);
				m_old_opengl_view = null;
		}
		});
	}

	public void hideBitmapView()
	{
		m_bitmap_view.setVisibility(View.INVISIBLE);
	}

	public void showBitmapView()
	{
		m_bitmap_view.setVisibility(View.VISIBLE);
	}

////////////////////////////////////////////////////////////////////////////////

	// in-app purchasing

	private static Class mBillingServiceClass = null;

	public static Class getBillingServiceClass()
	{
		return mBillingServiceClass;
	}

	private static void setBillingServiceClass(Class pClass)
	{
		mBillingServiceClass = pClass;
	}

	private BillingService mBilling = null;
	private EnginePurchaseObserver mPurchaseObserver = null;

	private void initBilling()
	{
        String t_public_key = doGetCustomPropertyValue("cREVStandaloneSettings", "android,storeKey");
        if (t_public_key != null && t_public_key.length() > 0)
            Security.setPublicKey(t_public_key);

		String classFqn = getContext().getPackageName() + ".AppService";
		try
		{
			Class tClass = Class.forName(classFqn);
			setBillingServiceClass(tClass);

			mBilling = (BillingService)tClass.newInstance();
		}
		catch (Exception e)
		{
			return;
		}

		mBilling.setContext(this.getContext());

		mPurchaseObserver = new EnginePurchaseObserver((Activity)getContext());
		ResponseHandler.register(mPurchaseObserver);
	}

	public boolean storeCanMakePurchase()
	{
		if (mBilling == null)
			return false;

		return mBilling.checkBillingSupported();
	}

	public void storeSetUpdates(boolean enabled)
	{
		if (mBilling == null)
			return;

		if (enabled)
			ResponseHandler.register(mPurchaseObserver);
		else
			ResponseHandler.unregister(mPurchaseObserver);
	}

	public boolean storeRestorePurchases()
	{
		if (mBilling == null)
			return false;

		return mBilling.restoreTransactions();
	}

	public boolean purchaseSendRequest(int purchaseId, String productId, String developerPayload)
	{
		if (mBilling == null)
			return false;

		Log.i(TAG, "purchaseSendRequest(" + purchaseId + ", " + productId + ")");
		return mBilling.requestPurchase(purchaseId, productId, developerPayload);
	}

	public boolean purchaseConfirmDelivery(int purchaseId, String notificationId)
	{
		if (mBilling == null)
			return false;

		return mBilling.confirmNotification(purchaseId, notificationId);
	}

////////

	private class EnginePurchaseObserver extends PurchaseObserver
	{
		public EnginePurchaseObserver(Activity pActivity)
		{
			super(pActivity);
		}

		public void onBillingSupported(boolean supported)
		{
			final boolean tSupported = supported;
			post(new Runnable() {
				public void run() {
					doBillingSupported(tSupported);
					if (m_wake_on_event)
						doProcess(false);
				}
			});
		}

		public void onPurchaseStateChanged(Purchase purchase, boolean verified, String signedData, String signature)
		{
			final boolean tVerified = verified;
			final int tPurchaseState = purchase.purchaseState.ordinal();
			final String tNotificationId = purchase.notificationId;
			final String tProductId = purchase.productId;
			final String tOrderId = purchase.orderId;
			final long tPurchaseTime = purchase.purchaseTime;
			final String tDeveloperPayload = purchase.developerPayload;
			final String tSignedData = signedData;
			final String tSignature = signature;

			post(new Runnable() {
				public void run() {
					doPurchaseStateChanged(tVerified, tPurchaseState,
						tNotificationId, tProductId, tOrderId,
						tPurchaseTime, tDeveloperPayload, tSignedData, tSignature);
					if (m_wake_on_event)
						doProcess(false);
				}
			});
		}

		public void onRestoreTransactionsResponse(RestoreTransactions request, ResponseCode responseCode)
		{
			final int tResponseCode = responseCode.ordinal();
			post(new Runnable() {
				public void run() {
					doRestoreTransactionsResponse(tResponseCode);
					if (m_wake_on_event)
						doProcess(false);
				}
			});
		}

		public void onConfirmNotificationResponse(ConfirmNotification request, ResponseCode responseCode)
		{
			final int tPurchaseId = request.mPurchaseId;
			final int tResponseCode = responseCode.ordinal();
			post(new Runnable() {
				public void run() {
					doConfirmNotificationResponse(tPurchaseId, tResponseCode);
					if (m_wake_on_event)
						doProcess(false);
				}
			});
		}

		public void onRequestPurchaseResponse(RequestPurchase request, ResponseCode responseCode)
		{
			final int tPurchaseId = request.mPurchaseId;
			final int tResponseCode = responseCode.ordinal();
			post(new Runnable() {
				public void run() {
					doRequestPurchaseResponse(tPurchaseId, tResponseCode);
					if (m_wake_on_event)
						doProcess(false);
				}
			});
		}
	}

////////////////////////////////////////////////////////////////////////////////

	public void showBusyIndicator(String p_label)
	{
        m_busy_indicator_module.showBusyIndicator(p_label);
    }

    public void hideBusyIndicator()
	{
        m_busy_indicator_module.hideBusyIndicator();
    }

////////////////////////////////////////////////////////////////////////////////

	public boolean canSendTextMessage()
	{
        Intent t_sms_intent = m_text_messaging_module.canSendTextMessage();
		return t_sms_intent.resolveActivity(getContext().getPackageManager()) != null;
    }

    public void composeTextMessage(String p_recipients, String p_body)
	{
        Intent t_intent = m_text_messaging_module.composeTextMessage(p_recipients, p_body);
        ((LiveCodeActivity)getContext()).startActivityForResult(t_intent, TEXT_RESULT);
    }

    private void onTextResult(int resultCode, Intent data)
	{

		if (resultCode == Activity.RESULT_CANCELED)
		{
			doTextCanceled();
		}
		else if (resultCode == Activity.RESULT_OK)
		{
			doTextDone();
		}
		else
		{
			doTextCanceled();
		}

		if (m_wake_on_event)
        {
            doProcess(false);
        }
	}

////////////////////////////////////////////////////////////////////////////////

	public void doBeep(int p_number_of_beeps)
	{
        try
        {
            m_beep_vibrate_module.doBeep(p_number_of_beeps);
        }
        catch (IOException e)
        {

        }
    }

    public void doVibrate (int p_number_of_vibrations)
	{
        m_beep_vibrate_module.doVibrate(p_number_of_vibrations);
    }

    ////////////////////////////////////////////////////////////////////////////////

    // processing contacts
    
    public int pickContact ()
    {
        int t_result = 0;
        m_contact_module.pickContact();
        return t_result;
    }
    
    public int showContact (int p_contact_id)
    {
        int t_result = 0;
        m_contact_module.showContact(p_contact_id);
        return t_result;
    }
    
    public int createContact ()
    {
        int t_result = 0;
        m_contact_module.createContact();
        return t_result;
    }

    public void updateContact(Map p_contact, String p_title, String p_message, String p_alternate_name)
	{
        Log.i(TAG, " ENG updateContact");
		m_contact_module.updateContact(p_contact, p_title, p_message, p_alternate_name);
	}

    public Map getContactData (int p_contact_id)
    {
        return m_contact_module.getContactData(p_contact_id);
    }

    public void removeContact (int p_contact_id)
    {
        m_contact_module.removeContact(p_contact_id);
    }

	public int addContact(Map p_contact)
	{
		Log.i(TAG, "ENG addContact");
		return m_contact_module.addContact(p_contact);
	}
    
    public void findContact(String p_contact_name)
    {
        Log.i("revandroid", "ENG findContact - name: " + p_contact_name);
        m_contact_module.findContact (p_contact_name);
    }   
    
    private void onPickContactResult(int resultCode, Intent data)
	{
        Log.i("revandroid", "onPickContact Called");
        if (resultCode == Activity.RESULT_OK)
		{
            Log.i("revandroid", "pickContact Okay");
            Uri t_data = data.getData();
            int t_selected_contact = 0;
            if (t_data != null)
            {
                Cursor t_database_cursor = ((LiveCodeActivity)getContext()).getContentResolver().query(t_data, null, null, null, null);
                if (t_database_cursor != null)
                {
                    t_database_cursor.moveToFirst();
                    t_selected_contact = t_database_cursor.getInt(t_database_cursor.getColumnIndex(ContactsContract.Contacts._ID)); 
                }
                Log.i("revandroid", "pickContact Okay1 : " + t_selected_contact);                
            }
			doPickContactDone(t_selected_contact);
		}
		else
        {
            Log.i("revandroid", "pickContact Canceled");
			doPickContactCanceled(0);
		}
		if (m_wake_on_event)
        {
            doProcess(false);
        }
	}
       
    private void onUpdateContactResult(int resultCode, Intent data)
	{
        Log.i("revandroid", "onUpdateContact Called");
        if (resultCode == Activity.RESULT_OK)
		{
            Log.i("revandroid", "updateContact Okay");
            Uri t_data = data.getData();
            int t_selected_contact = 0;
            if (t_data != null)
            {
                Cursor t_database_cursor = ((LiveCodeActivity)getContext()).getContentResolver().query(t_data, null, null, null, null);
                if (t_database_cursor != null)
                {
                    t_database_cursor.moveToFirst();
                    t_selected_contact = t_database_cursor.getInt(t_database_cursor.getColumnIndex(ContactsContract.Contacts._ID)); 
                }
                Log.i("revandroid", "updateContact Okay1 : " + t_selected_contact);                
            }
			doUpdateContactDone(t_selected_contact);
		}
		else
        {
            Log.i("revandroid", "updateContact Canceled");
			doUpdateContactCanceled(0);
		}
		if (m_wake_on_event)
        {
            doProcess(false);
        }
	}

    private void onCreateContactResult(int resultCode, Intent data)
	{
        Log.i("revandroid", "onCreateContact Called");
        if (resultCode == Activity.RESULT_OK)
		{
            Log.i("revandroid", "createContact Okay");
            Uri t_data = data.getData();
            int t_selected_contact = 0;
            if (t_data != null)
            {
                Cursor t_database_cursor = ((LiveCodeActivity)getContext()).getContentResolver().query(t_data, null, null, null, null);
                if (t_database_cursor != null)
                {
                    t_database_cursor.moveToFirst();
                    t_selected_contact = t_database_cursor.getInt(t_database_cursor.getColumnIndex(ContactsContract.Contacts._ID)); 
                }
                Log.i("revandroid", "createContact Okay1 : " + t_selected_contact);                
            }
			doCreateContactDone(t_selected_contact);
        }
		else
        {
            Log.i("revandroid", "createContact Canceled");
			doCreateContactCanceled(0);
		}
		if (m_wake_on_event)
        {
            doProcess(false);
        }
	}

    private void onShowContactResult(int resultCode, Intent data)
	{
        Log.i("revandroid", "onShowContact Called");
        if (resultCode == Activity.RESULT_OK)
		{
            Log.i("revandroid", "showContact Okay");
            Uri t_data = data.getData();
            int t_selected_contact = 0;
            if (t_data != null)
            {
                Cursor t_database_cursor = ((LiveCodeActivity)getContext()).getContentResolver().query(t_data, null, null, null, null);
                if (t_database_cursor != null)
                {
                    t_database_cursor.moveToFirst();
                    t_selected_contact = t_database_cursor.getInt(t_database_cursor.getColumnIndex(ContactsContract.Contacts._ID)); 
                }
                Log.i("revandroid", "showContact Okay1 : " + t_selected_contact);                
            }
			doShowContactDone(t_selected_contact);
        }
		else
        {
            Log.i("revandroid", "showContact Canceled");
			doShowContactCanceled(0);
		}
		if (m_wake_on_event)
        {
            doProcess(false);
        }
	}

    ////////////////////////////////////////////////////////////////////////////////
    
    // TO BE EXTENDED FOR IPA LEVEL 14 ->
    public void createCalendarEvent ()
    {
        Log.i("revandroid", "createCalendarEvent Called");
        m_calendar_module.createCalendarEvent();
    }

    public void updateCalendarEvent (String p_calendar_event_id)
    {
        Log.i("revandroid", "updateCalendarEvent Called");
        m_calendar_module.updateCalendarEvent(p_calendar_event_id);
    }
    
    private void onCreateCalendarEventResult(int resultCode, Intent data)
	{
        Log.i("revandroid", "onCreateCalendarEvent Called with resultCode: " + resultCode);
        if (resultCode == Activity.RESULT_OK)
		{
            Log.i("revandroid", "createCalendarEvent Okay");
            Uri t_data = data.getData();
            String t_selected_calendar_event = "";
            if (t_data != null)
            {
                Cursor t_database_cursor = ((LiveCodeActivity)getContext()).getContentResolver().query(t_data, null, null, null, null);
                if (t_database_cursor != null)
                {
                    t_database_cursor.moveToFirst();
                    t_selected_calendar_event = t_database_cursor.getString(t_database_cursor.getColumnIndex(ContactsContract.Contacts._ID)); 
                }
                Log.i("revandroid", "createCalendarEvent Okay1 : " + t_selected_calendar_event);                
            }
			doCreateCalendarEventDone(t_selected_calendar_event);
        }
		else
        {
            Log.i("revandroid", "createCalendarEvent Canceled");
			doCreateCalendarEventCanceled("");
		}
		if (m_wake_on_event)
        {
            doProcess(false);
        }
	}

    private void onUpdateCalendarEventResult(int resultCode, Intent data)
	{
        Log.i("revandroid", "onUpdateCalendaaEvent Called");
        if (resultCode == Activity.RESULT_OK)
		{
            Log.i("revandroid", "updateCalendarEvent Okay");
            Uri t_data = data.getData();
            String t_selected_calendar_event = "";
            if (t_data != null)
            {
                Cursor t_database_cursor = ((LiveCodeActivity)getContext()).getContentResolver().query(t_data, null, null, null, null);
                if (t_database_cursor != null)
                {
                    t_database_cursor.moveToFirst();
                    t_selected_calendar_event = t_database_cursor.getString(t_database_cursor.getColumnIndex(ContactsContract.Contacts._ID)); 
                }
                Log.i("revandroid", "updateCalendarEvent Okay1 : " + t_selected_calendar_event);                
            }
			doUpdateCalendarEventDone(t_selected_calendar_event);
		}
		else
        {
            Log.i("revandroid", "updateCalendarEvent Canceled");
			doUpdateCalendarEventCanceled("");
		}
		if (m_wake_on_event)
        {
            doProcess(false);
        }
	}
    
    private void onShowCalendarEventResult(int resultCode, Intent data)
	{
        Log.i("revandroid", "onShowCalendarEvent Called");
        if (resultCode == Activity.RESULT_OK)
		{
            Log.i("revandroid", "showCalendarEvent Okay");
            Uri t_data = data.getData();
            String t_selected_calendar_event = "";
            if (t_data != null)
            {
                Cursor t_database_cursor = ((LiveCodeActivity)getContext()).getContentResolver().query(t_data, null, null, null, null);
                if (t_database_cursor != null)
                {
                    t_database_cursor.moveToFirst();
                    t_selected_calendar_event = t_database_cursor.getString(t_database_cursor.getColumnIndex(ContactsContract.Contacts._ID)); 
                }
                Log.i("revandroid", "showCalendarEvent Okay1 : " + t_selected_calendar_event);                
            }
			doShowCalendarEventDone(t_selected_calendar_event);
        }
		else
        {
            Log.i("revandroid", "showCalendarEvent Canceled");
			doShowCalendarEventCanceled("");
		}
		if (m_wake_on_event)
        {
            doProcess(false);
        }
	}
    // <- TO BE EXTENDED FOR IPA LEVEL 14
    
    ////////////////////////////////////////////////////////////////////////////////

    public long createLocalNotification(String p_body, String p_action, String p_user_info, int p_seconds, boolean p_play_sound, int p_badge_value)
    {
        return m_notification_module.createLocalNotification(p_body, p_action, p_user_info, (long)p_seconds * 1000, p_play_sound, p_badge_value);
    }

    public String getRegisteredNotifications()
    {
        return m_notification_module.getRegisteredNotifications();
    }

    public boolean getNotificationDetails(long id)
    {
        return m_notification_module.getNotificationDetails(id);
    }

    public boolean cancelLocalNotification(long id)
    {
        return m_notification_module.cancelLocalNotification(id);
    }

    public boolean cancelAllLocalNotifications()
    {
        return m_notification_module.cancelAllLocalNotifications();
    }

    public void dispatchNotifications()
    {
        m_notification_module.dispatchNotifications();
    }

////////

    // remote notification
    public boolean registerForRemoteNotifications()
    {
        String t_sender = doGetCustomPropertyValue("cREVStandaloneSettings", "android,pushSenderID");
        if (t_sender == null || t_sender.length() == 0)
            return false;
        else
            return m_notification_module.registerForRemoteNotifications(t_sender);
    }

    public String getRemoteNotificationId()
    {
        return NotificationModule.getRemoteNotificationId();
    }

////////

    // if the app was launched to handle a Uri view intent, return the Uri as a string, else return null
    public String getLaunchUri(Intent intent)
    {
        if (intent != null && Intent.ACTION_VIEW.equals(intent.getAction()))
        {
            Log.i(TAG, intent.toString());
            // we were launched to handle a VIEW request, so the intent data should contain the URI
            Uri t_uri = intent.getData();
            if (t_uri == null)
                return null;
            else
                return t_uri.toString();
        }
        else
            return null;
    }

    public String getLaunchUri()
    {
        Intent t_intent = ((Activity)getContext()).getIntent();
        return getLaunchUri(t_intent);
    }

////////////////////////////////////////////////////////////////////////////////

    // called from the engine to signal that the engine has launched
    // we perform some initialisation here
    public void onAppLaunched()
    {
        // check launch url
        String t_launch_url = getLaunchUri();
        if (t_launch_url != null)
            doLaunchFromUrl(t_launch_url);

        // set up billing
        initBilling();

        // send any pending local / remote notifications
        dispatchNotifications();

        // register for remote notifications
        registerForRemoteNotifications();
    }

	public void onPause()
	{
		if (m_text_editor_visible)
			hideKeyboard();
		
		s_running = false;

		m_shake_listener.onPause();
		m_orientation_listener.disable();

        if (m_sensor_module != null)
            m_sensor_module.onPause();

        if (m_sound_module != null)
            m_sound_module.onPause();

		if (m_video_is_playing)
			m_video_control . suspend();

		doPause();
	}

	public void onResume()
	{
		m_shake_listener.onResume();
		m_orientation_listener.enable();

		if (m_sensor_module != null)
            m_sensor_module.onResume();

        if (m_sound_module != null)
            m_sound_module.onResume();

		if (m_video_is_playing)
			m_video_control . resume();

		doResume();

		s_running = true;
		if (m_text_editor_visible)
			showKeyboard();
		
		// IM-2013-08-16: [[ Bugfix 11103 ]] dispatch any remote notifications received while paused
		dispatchNotifications();
	}

	public void onDestroy()
	{
		doDestroy();
        s_engine_instance = null;
	}

    ////////

    public void onNewIntent(Intent intent)
    {
        String t_launch_url = getLaunchUri(intent);
        if (t_launch_url != null)
        {
            doLaunchFromUrl(t_launch_url);
            if (m_wake_on_event)
                doProcess(false);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    public String getPreferredLanguages ()
    {
		String t_language = Locale.getDefault().getLanguage(); //
		return t_language;
    }

    public String getPreferredLocale ()
    {
		String t_locale = Locale.getDefault().toString(); //
		return t_locale;
    }

    ////////////////////////////////////////////////////////////////////////////////

    public void doLockIdleTimer ()
    {
        if (m_wake_lock.isHeld () == false)
            m_wake_lock.acquire ();
    }

    public void doUnlockIdleTimer ()
    {
        if (m_wake_lock.isHeld () == true)
            m_wake_lock.release ();
    }

    public boolean getLockIdleTimerLocked ()
    {
        return m_wake_lock.isHeld ();
    }

    ////////////////////////////////////////////////////////////////////////////////

    public String exportImageToAlbum (byte[] t_image_data, String t_file_name, String t_file_type)
    {
        Log.i("revandroid", "exportToAlbum called 1" + t_file_name + t_file_type);
        File t_file = null;
        UUID t_uuid;
        if (t_image_data == null)
            return "export failed";
        else
        {
            // The user did not supply a file name, so create one now
            if (t_file_name == null)
            {
                t_uuid = UUID.randomUUID();
                Log.i("revandroid", "Generated File Name: " + Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES) + "/I" + t_uuid.toString().substring(0,7) + t_file_type);
                t_file = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES) + "/I" + t_uuid.toString().substring(0,7) + t_file_type);
            }
            else
            {
                t_file = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES) + "/" + t_file_name + t_file_type);
            }
            FileOutputStream fs = null;
            try
            {
                fs = new FileOutputStream(t_file);
                fs.write(t_image_data,0,t_image_data.length);
                fs.close();
            }
            catch (IOException e)
            {
                return e.toString();
            }
            return null;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    public void pickMedia (String p_media_types)
	{
        Log.i("revandroid", "pickMedia");
        Intent t_media_pick_intent = new Intent(Intent.ACTION_GET_CONTENT);
        Log.i("revandroid", "MIME type: " + p_media_types);
        t_media_pick_intent.setType (p_media_types);
        Intent t_chooser = Intent.createChooser (t_media_pick_intent, "");
        ((LiveCodeActivity)getContext()).startActivityForResult (t_chooser,MEDIA_RESULT);
    }

    private void onMediaResult(int resultCode, Intent data)
	{
        if (resultCode == Activity.RESULT_OK)
		{
            Uri t_data = data.getData();
            String t_path = null;
            if (t_data != null)
            {
                Cursor t_cursor = ((LiveCodeActivity)getContext()).getContentResolver().query(t_data, null, null, null, null);
                if (t_cursor != null)
                {
                    t_cursor.moveToFirst();
                    int t_index = t_cursor.getColumnIndex (MediaStore.Images.ImageColumns.DATA);
                    t_path = t_cursor.getString(t_index);
                }
                Log.i("revandroid", "onMediaResult picked path: " + t_path);
            }
			doMediaDone(t_path);
		}
		else
        {
            Log.i("revandroid", "pickMedia Canceled");
			doMediaCanceled();
		}
		if (m_wake_on_event)
        {
            doProcess(false);
        }
	}

    // AL-2013-14-07 [[ Bug 10445 ]] Sort international on Android
    public int compareInternational(String left, String right)
    {
        Log.i("revandroid", "compareInternational"); 
        return m_collator.compare(left, right);
    }
    
    ////////////////////////////////////////////////////////////////////////////////

	// EngineApi implementation
	
	// MW-2013-08-07: [[ ExternalsApiV5 ]] Implement the 'getActivity()' API method.
	public Activity getActivity()
	{
		return (LiveCodeActivity)getContext();
	}
	
	// MW-2013-08-07: [[ ExternalsApiV5 ]] Implement the 'getContainer()' API method.
	public ViewGroup getContainer()
	{
		return (ViewGroup)getParent();
	}
	
	// MW-2013-08-07: [[ ExternalsApiV5 ]] Implement the 'runActivity()' API method -
	//   hooks into onActivityResult handler too.
	private boolean m_pending_activity_running = false;
	private int m_pending_activity_result_code = 0;
	private Intent m_pending_activity_data = null;
	public void runActivity(Intent p_intent, ActivityResultCallback p_callback)
	{
		// We aren't re-entrant, so just invoke the callback as 'cancelled' if one is
		// already running.
		if (m_pending_activity_running)
		{
			p_callback . handleActivityResult(Activity.RESULT_CANCELED, null);
			return;
		}
		
		// Mark an activity as running.
		m_pending_activity_running = true;
		
		// Run the activity.
		((LiveCodeActivity)getContext()) . startActivityForResult(p_intent, RUN_ACTIVITY_RESULT);
		
		// Wait until the activity returns.
		while(m_pending_activity_running)
			doWait(60.0, false, true);
		
		// Take local copies of the instance vars (to stop hanging data).
		Intent t_data;
		int t_result_code;
		t_data = m_pending_activity_data;
		t_result_code = m_pending_activity_result_code;
		
		// Reset the instance vars (to stop hanging data and so that the callback
		// can start another activity if it wants).
		m_pending_activity_data = null;
		m_pending_activity_result_code = 0;
		
		p_callback . handleActivityResult(t_result_code, t_data);
	}
	
	// MW-2013-08-07: [[ ExternalsApiV5 ]] Called when an activity invoked using
	//   'runActivity()' API method returns data.
	private void onRunActivityResult(int p_result_code, Intent p_data)
	{
		// Store the result details.
		m_pending_activity_data = p_data;
		m_pending_activity_result_code = p_result_code;
		m_pending_activity_running = false;
		
		// Make sure we signal a switch back to the script thread.
		if (m_wake_on_event)
			doProcess(false);
	}
	
    ////////////////////////////////////////////////////////////////////////////////

    // url launch callback
    public static native void doLaunchFromUrl(String url);

	// callbacks from the billing service

	public static native void doBillingSupported(boolean supported);
	public static native void doPurchaseStateChanged(boolean verified, int purchaseState,
		String notificationId, String productId, String orderId, long purchaseTime,
		String developerPayload, String signedData, String signature);

	public static native void doConfirmNotificationResponse(int purchaseId, int responseCode);
	public static native void doRestoreTransactionsResponse(int responseCode);
	public static native void doRequestPurchaseResponse(int purchaseId, int responseCode);

////////////////////////////////////////////////////////////////////////////////

	// These are the methods implemented by the engine. Notice that they all
	// begin with 'do', this is to make sure we don't get any conflicts with
	// java methods, which tend to start with 'on' when they are event handlers.

	public static native void doCreate(Activity activity, FrameLayout container, Engine self);
	public static native void doDestroy();
	public static native void doRestart(Engine self);
	public static native void doStart();
	public static native void doStop();
	public static native void doPause();
	public static native void doResume();
	public static native void doLowMemory();

	public static native void doNativeNotify(int callback, int context);
			 
////////////////////////////////////////////////////////////////////////////////

	// Our engine interface

	public static native void doProcess(boolean timedout);

	public static native void doReconfigure(int w, int h, Bitmap bitmap);

    public static native String doGetCustomPropertyValue(String set, String property);

	// MW-2013-08-07: [[ ExternalsApiV5 ]] Native wrapper around MCScreenDC::wait
	//   used by runActivity() API.
	public static native void doWait(double time, boolean dispatch, boolean anyevent);
	
    // sensor handlers
    public static native void doLocationChanged(double p_latitude, double p_longitude, double p_altitude, float p_timestamp, float p_accuracy, double p_speed, double p_course);
    public static native void doHeadingChanged(double p_heading, double p_magnetic_heading, double p_true_heading, float p_timestamp,
                                               float p_x, float p_y, float p_z, float p_accuracy);
	public static native void doAccelerationChanged(float x, float y, float z, float timestamp);
	public static native void doRotationRateChanged(float x, float y, float z, float timestamp);

    // input event handlers
	public static native void doBackPressed();
    public static native void doMenuKey();
    public static native void doSearchKey();
	public static native void doTouch(int action, int id, int timestamp, int x, int y);
	public static native void doKeyPress(int modifiers, int char_code, int key_code);
	public static native void doShake(int action, long timestamp);

	public static native void doOrientationChanged(int orientation);

	public static native void doKeyboardShown(int height);
	public static native void doKeyboardHidden();

    // dialog handlers
	public static native void doAnswerDialogDone(int action);
	public static native void doAskDialogDone(String result);
    public static native void doDatePickerDone(int year, int month, int day, boolean done);
    public static native void doTimePickerDone(int hour, int minute, boolean done);
    public static native void doListPickerDone(int index, boolean done);

	public static native void doMovieStopped();
	public static native void doMovieTouched();

	public static native void doUrlDidStart(int id);
	public static native void doUrlDidConnect(int id, int content_length);
	public static native void doUrlDidRequest(int id);
	public static native void doUrlDidSendData(int id, int bytes_sent);
	public static native void doUrlDidReceiveData(int id, byte data[], int length);
	public static native void doUrlDidFinish(int id);
	public static native void doUrlError(int id, String error_str);

	public static native void doPhotoPickerCanceled();
	public static native void doPhotoPickerDone(byte[] p_data, int p_size);
	public static native void doPhotoPickerError(String p_error);

	public static native void doMailDone();
	public static native void doMailCanceled();

	public static native void doTextDone();
	public static native void doTextCanceled();

	public static native void doMediaDone(String p_media_content);
	public static native void doMediaCanceled();
    
	public static native void doPickContactDone(int p_contact_id);
	public static native void doPickContactCanceled(int p_contact_id);
	public static native void doShowContact(int p_contact_id);
	public static native void doUpdateContactDone(int p_contact_id);
	public static native void doUpdateContactCanceled(int p_contact_id);
	public static native void doCreateContactDone(int t_contact_id);
	public static native void doCreateContactCanceled(int p_contact_id);
	public static native void doShowContactDone(int p_contact_id);
	public static native void doShowContactCanceled(int p_contact_id);
	public static native void doFindContact(String p_contacts_found);

// TO BE EXTENDED FOR API LEVEL 14 ->
	public static native void doShowCalendarEvent(String p_calendar_event_id);
	public static native void doGetCalendarEventData(String p_eventid, String p_title, String p_note, String p_location, boolean p_alldayset, boolean p_allday, boolean p_startdateset, int p_startdate, boolean p_enddateset, int p_enddate, int p_alert1, int p_alert2, String p_frequency, int p_frequencycount, int p_frequencyinterval, String p_calendar);
	public static native void doUpdateCalendarEventDone(String p_calendar_event_id);
	public static native void doUpdateCalendarEventCanceled(String p_alendar_event_id);
	public static native void doCreateCalendarEventDone(String t_calendar_event_id);
	public static native void doCreateCalendarEventCanceled(String p_calendar_event_id);
	public static native void doShowCalendarEventDone(String p_calendar_event_id);
	public static native void doShowCalendarEventCanceled(String p_calendar_event_id);
	public static native void doAddCalendarEvent(String p_calendar_event_id);
	public static native void doFindCalendarEvent(String p_calendar_events_found);
	public static native void doRemoveCalendarEvent(String p_calendar_event_id);
// <- TO BE EXTENDED FOR API LEVEL 14 ->

}
