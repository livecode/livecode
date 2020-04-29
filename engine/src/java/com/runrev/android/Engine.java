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

import com.runrev.android.billing.*;
/*
import com.runrev.android.billing.C.ResponseCode;
import com.runrev.android.billing.PurchaseUpdate.Purchase;
import com.runrev.android.billing.BillingService.RestoreTransactions;
import com.runrev.android.billing.BillingService.GetPurchaseInformation;
import com.runrev.android.billing.BillingService.ConfirmNotification;
import com.runrev.android.billing.BillingService.RequestPurchase;
 */

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
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.charset.*;
import java.lang.reflect.*;
import java.util.*;
import java.text.Collator;
import java.lang.Math;

import java.security.KeyStore;
import java.security.cert.CertificateFactory;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import java.security.cert.CertificateException;

// This is the main class that interacts with the engine. Although only one
// instance of the engine is allowed, we still need an object on which we can
// invoke methods from the native code so we wrap all this up into a single
// view object.

public class Engine extends View implements EngineApi
{
	public interface LifecycleListener
	{
		public abstract void OnResume();
		public abstract void OnPause();
	}

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
	private boolean m_disabling_opengl;
    private boolean m_enabling_opengl;
    
	private BitmapView m_bitmap_view;

	private File m_temp_image_file;

	private Email m_email;

	private ShakeEventListener m_shake_listener;
	private ScreenOrientationEventListener m_orientation_listener;

	private boolean m_text_editor_visible;
	private int m_text_editor_mode;
    private int m_default_text_editor_mode;

    private int m_default_ime_action;
    private int m_ime_action;
    
    private SensorModule m_sensor_module;
    private DialogModule m_dialog_module;
    private NetworkModule m_network_module;
    private NativeControlModule m_native_control_module;
    private SoundModule m_sound_module;
    private NotificationModule m_notification_module;
	private NFCModule m_nfc_module;
    private RelativeLayout m_view_layout;

    private PowerManager.WakeLock m_wake_lock;
    
    // AL-2013-14-07 [[ Bug 10445 ]] Sort international on Android
    private Collator m_collator;
    
    // MM-2015-06-11: [[ MobileSockets ]] Trust manager and last verification error, used for verifying ssl certificates.
    private X509TrustManager m_trust_manager;
    private String m_last_certificate_verification_error;
	
	private boolean m_new_intent;

	private int m_photo_width, m_photo_height;
	private int m_jpeg_quality;
	
	private int m_night_mode;

	private List<LifecycleListener> m_lifecycle_listeners;

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
		m_default_text_editor_mode = 1;
        m_text_editor_mode = 0;
        m_text_editor_visible = false;

        m_default_ime_action = EditorInfo.IME_FLAG_NO_ENTER_ACTION | EditorInfo.IME_ACTION_DONE;
        m_ime_action = 0;
        
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
		m_nfc_module = new NFCModule(this);
        m_view_layout = null;
        
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
        m_disabling_opengl = false;
        m_enabling_opengl = false;
        
		// But we do have a bitmap view.
		m_bitmap_view = new BitmapView(getContext());
        
        // AL-2013-14-07 [[ Bug 10445 ]] Sort international on Android
        m_collator = Collator.getInstance(Locale.getDefault());
		
        // MM-2015-06-11: [[ MobileSockets ]] Trust manager and last verification error, used for verifying ssl certificates.
        m_trust_manager = null;
        m_last_certificate_verification_error = null;
        
		// MW-2013-10-09: [[ Bug 11266 ]] Turn off keep-alive connections to
		//   work-around a general bug in android:
		// https://code.google.com/p/google-http-java-client/issues/detail?id=116
		System.setProperty("http.keepAlive", "false");
		
		m_new_intent = false;

		m_photo_width = 0;
		m_photo_height = 0;
		m_jpeg_quality = 100;
		
		m_night_mode =
			p_context.getResources().getConfiguration().uiMode &
			Configuration.UI_MODE_NIGHT_MASK;

		m_lifecycle_listeners = new ArrayList<LifecycleListener>();
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
	
	public void nativeNotify(long p_callback, long p_context)
    {
        final long t_callback = p_callback;
		final long t_context = p_context;
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

    // MM-2015-06-08: [[ MobileSockets ]] This can now potentially be called from several threads so make method synchronized.
	public synchronized void scheduleWakeUp(int p_in_time, boolean p_any_event)
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

	// IM-2016-03-04: [[ Bug 16917 ]] Return location of native libraries installed with this app
	public String getLibraryPath()
	{
		return getContext() . getApplicationInfo() . nativeLibraryDir;
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
            Log.i("revandroid", e.toString());
			return null;
		}
		catch ( SecurityException e )
		{
            Log.i("revandroid", e.toString());
			return null;
		}
	}

////////////////////////////////////////////////////////////////////////////////

    public void onConfigurationChanged(Configuration p_new_config)
	{
		int t_night_mode =
			getContext().getResources().getConfiguration().uiMode &
			Configuration.UI_MODE_NIGHT_MASK;
		
		if (t_night_mode != m_night_mode)
		{
			m_night_mode = t_night_mode;
			doSystemAppearanceChanged();
		}
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
        
        // MM-2014-03-25: [[ Bug 11708 ]] Moved call to update orientation (from onScreenOrientationChanged).
        //  This way we only flag orientation changed if the we've set it in the activity (i.e. or app has actually rotated).
        //  Prevents changes in device orientation during lock screen confusing things.
        // IM-2013-11-15: [[ Bug 10485 ]] Record the change in orientation
        updateOrientation(p_orientation);
        
		((LiveCodeActivity)getContext()).setRequestedOrientation(s_orientation_map[p_orientation]);
	}

////////////////////////////////////////////////////////////////////////////////

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs)
    {
		Log.i(TAG, "onCreateInputConnection()");
		if (!m_text_editor_visible)
			return null;
		
        InputConnection t_connection = new BaseInputConnection(this, true) {
        	String m_current_text = "";
        	
			void handleKey(int keyCode, int charCode)
			{
				if (charCode == 0)
				{
					switch (keyCode)
					{
						case KeyEvent.KEYCODE_DEL:
							keyCode = 0xff08;
							break;
						// Hao-2017-02-08: [[ Bug 11727 ]] Detect arrow key for field input
						case KeyEvent.KEYCODE_DPAD_LEFT:
							keyCode = 0xff51;
							break;
						case KeyEvent.KEYCODE_DPAD_UP:
							keyCode = 0xff52;
							break;
						case KeyEvent.KEYCODE_DPAD_RIGHT:
							keyCode = 0xff53;
							break;
						case KeyEvent.KEYCODE_DPAD_DOWN:
							keyCode = 0xff54;
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
					if (t_key_code == KeyEvent.KEYCODE_UNKNOWN)
					{
						// handle string of chars
						CharSequence t_chars = key.getCharacters();
						for (int i = 0; i < t_chars.length(); i++)
							handleKey(t_key_code, t_chars.charAt(i));
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
			
			// Show text changes in the field as the composing text is modified.
			// We do this by removing edited text with fake backspace key events
			// and sending key events for each new character.
			void updateComposingText()
			{
				String t_new = getEditable().toString();
				
				// send changes to the engine as a sequence of key events.
				int t_match_length = 0;
				int t_current_length = 0;
				int t_new_length = 0;
				int t_max_length = 0;
				
				t_current_length = m_current_text.length();
				t_new_length = t_new.length();
				
				t_max_length = Math.min(t_current_length, t_new_length);
				for (int i = 0; i < t_max_length; i++)
				{
					if (t_new.charAt(i) != m_current_text.charAt(i))
						break;
					t_match_length += 1;
				}
				
				// send backspaces
				for (int i = 0; i < t_current_length - t_match_length; i++)
					handleKey(KeyEvent.KEYCODE_DEL, 0);
				// send new text
				for (int i = t_match_length; i < t_new_length; i++)
					handleKey(KeyEvent.KEYCODE_UNKNOWN, t_new.charAt(i));
				
				m_current_text = t_new;
				
				if (m_wake_on_event)
					doProcess(false);
			}
			
			// override input connection methods to catch changes to the composing text
			@Override
			public boolean commitText(CharSequence text, int newCursorPosition)
			{
				boolean t_return_value = super.commitText(text, newCursorPosition);
				updateComposingText();
				return t_return_value;
			}
			@Override
			public boolean finishComposingText()
			{
				boolean t_return_value = super.finishComposingText();
				updateComposingText();
				return t_return_value;
			}
			@Override
			public boolean setComposingText(CharSequence text, int newCursorPosition)
			{
				boolean t_return_value =  super.setComposingText(text, newCursorPosition);
				updateComposingText();
				return t_return_value;
			}
            @Override
            public boolean performEditorAction (int editorAction)
            {
                handleKey(0, 10);
                return true;
            }
            
            @Override
            public boolean deleteSurroundingText(int beforeLength, int afterLength)
            {
            	boolean t_return_value = super.deleteSurroundingText(beforeLength, afterLength);
            	updateComposingText();
            	return t_return_value;
            }
        };
        
		int t_type = getInputType(false);
		
        outAttrs.actionLabel = null;
		outAttrs.inputType = t_type;
        
        if (m_ime_action != 0)
        {
            outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI | m_ime_action;
        }
        else
        {
            outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI | m_default_ime_action;
        }
        
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
		
		// HH-2017-01-18: [[ Bug 18058 ]] Fix keyboard not show in landscape orientation
        imm.showSoftInput(this, InputMethodManager.SHOW_FORCED);
    }
    
    @Override
    public void getFocusedRect(Rect r_rect)
    {
        Rect t_rect = doGetFocusedRect();
        
        if (t_rect == null)
        {
            super.getFocusedRect(r_rect);
        }
        else
        {
            r_rect.set(t_rect);
        }
    }
    
    private static final int KEYBOARD_DISPLAY_OVER = 0;
    private static final int KEYBOARD_DISPLAY_PAN = 1;
    
    public void setKeyboardDisplay(int p_mode)
    {
        if (p_mode == KEYBOARD_DISPLAY_PAN)
        {
            getActivity()
                .getWindow()
                .setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        }
        else if (p_mode == KEYBOARD_DISPLAY_OVER)
        {
            getActivity()
                .getWindow()
                .setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        }
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

	public void setTextInputVisible(boolean p_visible, int p_input_mode, int p_ime_action)
	{
		m_text_editor_visible = p_visible;
		
		if (!s_running)
			return;
		
		if (p_visible)
        {
            m_text_editor_mode = p_input_mode;
            m_ime_action = p_ime_action;
			showKeyboard();
        }
		else
        {
			hideKeyboard();
            m_text_editor_mode = 0;
            m_ime_action = 0;
        }
	}
    
    public void setKeyboardReturnKey(int p_ime_action)
    {
        m_default_ime_action = p_ime_action;
    }
	
	public void setTextInputMode(int p_mode)
	{
		// 0 is none
		// 1 is text (normal)
		// 2 is number
		// 3 is decimal
		// 4 is phone
		// 5 is email
		
		boolean t_reset = s_running && m_text_editor_visible && p_mode != m_default_text_editor_mode;
		
		m_default_text_editor_mode = p_mode;
		
		if (t_reset)
			resetKeyboard();
	}
	
	public static final int TYPE_NUMBER_VARIATION_PASSWORD = 16;
	public int getInputType(boolean p_password)
	{
		int t_type;
		
		int t_mode = m_text_editor_mode;
        if (t_mode == 0)
        {
            t_mode = m_default_text_editor_mode;
        }
        
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

	// Native layer view functionality
	
	Object getNativeLayerContainer()
	{
		if (m_view_layout == null)
		{
			FrameLayout t_main_view;
			t_main_view = ((LiveCodeActivity)getContext()).s_main_layout;
			
			m_view_layout = new RelativeLayout(getContext());
			t_main_view.addView(m_view_layout, new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
			t_main_view.bringChildToFront(m_view_layout);
		}
		
		return m_view_layout;
	}
	
	Object createNativeLayerContainer()
	{
		return new RelativeLayout(getContext());
	}
	
	// insert the view into the container, layered below p_view_above if not null.
	void addNativeViewToContainer(Object p_view, Object p_view_above, Object p_container)
	{
		ViewGroup t_container;
		t_container = (ViewGroup)p_container;
		
		int t_index;
		if (p_view_above != null)
			t_index = t_container.indexOfChild((View)p_view_above);
		else
			t_index = t_container.getChildCount();
		
		t_container.addView((View)p_view, t_index, new RelativeLayout.LayoutParams(0, 0));
	}
	
	void removeNativeViewFromContainer(Object p_view)
	{
		// Remove view from its parent
		View t_view;
		t_view = (View)p_view;
		
		ViewGroup t_parent;
		t_parent = (ViewGroup)t_view.getParent();
		if (t_parent != null)
			t_parent.removeView(t_view);
	}
	
	void setNativeViewRect(Object p_view, int left, int top, int width, int height)
	{
		RelativeLayout.LayoutParams t_layout = new RelativeLayout.LayoutParams(width, height);
		t_layout.leftMargin = left;
		t_layout.topMargin = top;
		t_layout.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
		t_layout.addRule(RelativeLayout.ALIGN_PARENT_TOP);
		
		View t_view = (View)p_view;
		
		t_view.setLayoutParams(t_layout);
	}
	
    // native control functionality
	
    void addNativeControl(Object p_control)
    {
        m_native_control_module.addControl(p_control);
    }

    void removeNativeControl(Object p_control)
    {
        m_native_control_module.removeControl(p_control);
    }

	Object createNativeControl(String p_class_name)
	{
		return m_native_control_module.createControl(p_class_name);
	}
    
    Object createBrowserControl()
    {
        return m_native_control_module.createControl("com.runrev.android.nativecontrol.BrowserControl");
    }

    Object createScrollerControl()
    {
        return m_native_control_module.createControl("com.runrev.android.nativecontrol.ScrollerControl");
    }
    
    Object createPlayerControl()
    {
        return m_native_control_module.createControl("com.runrev.android.nativecontrol.VideoControl");
    }
    
    Object createInputControl()
    {
        return m_native_control_module.createControl("com.runrev.android.nativecontrol.InputControl");
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

    public void showListPicker(List p_items, String p_title, boolean p_item_selected, int p_selection_index, boolean p_use_hilite, boolean p_use_cancel, boolean p_use_done)
    {
        String[] t_items;
        t_items = (String[])p_items.toArray(new String[p_items.size()]);
        m_dialog_module.showListPicker(t_items, p_title, p_item_selected, p_selection_index, p_use_hilite, p_use_cancel, p_use_done);
    }
    public void onListPickerDone(int p_index, boolean p_done)
    {
        doListPickerDone(p_index, p_done);
        if (m_wake_on_event)
            doProcess(false);
    }
    
    public void onAskPermissionDone(boolean p_granted)
    {
        doAskPermissionDone(p_granted);
    }

////////////////////////////////////////////////////////////////////////////////
	
	// Return a bitmap snapshot of the specified region of the root view
	public Object getSnapshotBitmapAtSize(int x, int y, int width, int height, int sizeWidth, int sizeHeight)
	{
		Bitmap t_bitmap = Bitmap.createBitmap(sizeWidth, sizeHeight, Bitmap.Config.ARGB_8888);
		Canvas t_canvas = new Canvas(t_bitmap);
		t_canvas.scale((float)sizeWidth / (float)width, (float)sizeHeight / (float)height);
		t_canvas.translate((float)-x, (float)-y);
		
		getActivity().getWindow().getDecorView().getRootView().draw(t_canvas);
		
		return t_bitmap;
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

	// IM-2013-11-15: [[ Bug 10485 ]] Refactor to return the viewport as Rect
	public Rect getViewport()
	{
		DisplayMetrics t_metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(t_metrics);
		
		return new Rect(0, 0, t_metrics.widthPixels, t_metrics.heightPixels);
	}
	
	public String getViewportAsString()
	{
		return rectToString(getViewport());
	}

	// IM-2013-11-15: [[ Bug 10485 ]] Rename to "effective" as the returned value accounts
	// for the keyboard if visible
	private Rect getEffectiveWorkarea()
	{
		Rect t_workrect = new Rect();
        getWindowVisibleDisplayFrame(t_workrect);
        return t_workrect;
	}

	public String getEffectiveWorkareaAsString()
	{
		return rectToString(getEffectiveWorkarea());
	}
	
	private boolean m_know_portrait_size = false;
	private boolean m_know_landscape_size = false;
	private boolean m_know_statusbar_size = false;
	
	private Rect m_portrait_rect;
	private Rect m_landscape_rect;
	private int m_statusbar_size;
	
	// IM-2013-11-15: [[ Bug 10485 ]] Return the known work area, updating with new values
	// if requested.
	private Rect getWorkarea(boolean p_update, int p_new_width, int p_new_height)
	{
		// If the keyboard is visible when the orientation changes, the reported size may be wrong
		// so we either use the known size for the current orientation or make our best guess
		// based on the total screen height being reduced by the same amount in either orientation.
		Rect t_working_rect;
		t_working_rect = null;
		
		Rect t_viewport;
		t_viewport = getViewport();
		
		boolean t_portrait = t_viewport.height() > t_viewport.width();

		int[] t_origin = new int[2];
		getLocationOnScreen(t_origin);
		
		// We have new values and the keyboard isn't showing so update any sizes we don't already know
		if (p_update && !keyboardIsVisible())
		{
			t_working_rect = new Rect(t_origin[0], t_origin[1], t_origin[0] + p_new_width, t_origin[1] + p_new_height);
			
			if (t_portrait && !m_know_portrait_size)
			{
				m_portrait_rect = t_working_rect;
				m_know_portrait_size = true;
			}
			else if (!t_portrait && !m_know_landscape_size)
			{
				m_landscape_rect = t_working_rect;
				m_know_landscape_size = true;
			}
			
			if (!m_know_statusbar_size)
			{
				m_statusbar_size = t_viewport.height() - p_new_height;
				m_know_statusbar_size = true;
			}
		}
		else
		{
			if (t_portrait && m_know_portrait_size)
				t_working_rect = m_portrait_rect;
			else if (!t_portrait && m_know_landscape_size)
				t_working_rect = m_landscape_rect;
		}
		
		// If we don't have a known size for the current orientation, compute by subtracting
		// the height of the screen furniture from the viewport rect.
		if (t_working_rect == null)
		{
			t_working_rect = new Rect(t_viewport);
			if (m_know_statusbar_size)
				t_working_rect.bottom -= m_statusbar_size;
			t_working_rect.offsetTo(t_origin[0], t_origin[1]);
		}
		
		return t_working_rect;
	}
	
	public Rect getWorkarea()
	{
		return getWorkarea(false, 0, 0);
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

	private boolean m_keyboard_sizechange = false;
	private boolean m_orientation_sizechange = false;
	
	private boolean m_keyboard_visible = false;
    
    boolean keyboardIsVisible()
    {
        // status bar height
        int t_status_bar_height = 0;
        int t_resource_id = getResources().getIdentifier("status_bar_height", "dimen", "android");
        if (t_resource_id > 0)
        {
            t_status_bar_height = getResources().getDimensionPixelSize(t_resource_id);
        }
        
        // display window size for the app layout
        Rect t_app_rect = new Rect();
        getActivity().getWindow().getDecorView().getWindowVisibleDisplayFrame(t_app_rect);
        
        int t_screen_height = ((LiveCodeActivity)getContext()).s_main_layout.getRootView().getHeight();
        
        // keyboard height equals (screen height - (user app height + status))
        int t_keyboard_height = t_screen_height - (t_app_rect.height() + t_status_bar_height) - getSoftbuttonsbarHeight();
        
        return t_keyboard_height > 0;
    }
    
    private int getSoftbuttonsbarHeight() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1) {
            return 0;
        }
        
        DisplayMetrics t_metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(t_metrics);
        int t_usable_height = t_metrics.heightPixels;
        getWindowManager().getDefaultDisplay().getRealMetrics(t_metrics);
        int t_real_height = t_metrics.heightPixels;
        
        return t_real_height > t_usable_height ? t_real_height - t_usable_height : 0;
    }
	
	void updateKeyboardVisible()
	{
        boolean t_visible = keyboardIsVisible();
        
		if (t_visible == m_keyboard_visible)
			return;
		
		// Log.i(TAG, "updateKeyboardVisible(" + p_visible + ")");
		
		m_keyboard_visible = t_visible;
		
		// IM-2013-11-15: [[ Bug 10485 ]] Notify engine when keyboard visiblity changes
		if (t_visible)
			doKeyboardShown(0);
		else
			doKeyboardHidden();
		
		m_keyboard_sizechange = true;
	}
	
	void updateOrientation(int p_orientation)
	{
		// Log.i(TAG, "updateOrientation(" + p_orientation + ")");
		
		m_orientation_sizechange = true;
	}
	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		// Log.i(TAG, "onSizeChanged({" + w + "x" + h + "}, {" + oldw + ", " + oldh + "})");
		
		Rect t_rect;
		t_rect = null;
		
		if ((oldw == 0 && oldh == 0) || m_orientation_sizechange)
			t_rect = getWorkarea(true, w, h);
		
		m_keyboard_sizechange = m_orientation_sizechange = false;
		
		if (t_rect == null)
			return;
		
		m_bitmap_view . resizeBitmap(t_rect.width(), t_rect.height());
		
		// pass view location
		int[] t_origin = new int[2];
		getLocationOnScreen(t_origin);
		
		doReconfigure(t_origin[0], t_origin[1], t_rect.width(), t_rect.height(), m_bitmap_view . getBitmap());
		
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

    public void onAccelerationChanged(float p_x, float p_y, float p_z, double p_timestamp)
    {
        doAccelerationChanged(p_x, p_y, p_z, p_timestamp);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onLocationChanged(double p_latitude, double p_longitude, double p_altitude, double p_timestamp, float p_accuracy, double p_speed, double p_course)
    {
        // MM-2013-02-21: Added spead and course to location readings.
        doLocationChanged(p_latitude, p_longitude, p_altitude, p_timestamp, p_accuracy, p_speed, p_course);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onHeadingChanged(double p_heading, double p_magnetic_heading, double p_true_heading, double p_timestamp,
                                 float p_x, float p_y, float p_z, float p_accuracy)
    {
        doHeadingChanged(p_heading, p_magnetic_heading, p_true_heading, p_timestamp, p_x, p_y, p_z, p_accuracy);
        if (m_wake_on_event)
            doProcess(false);
    }

    public void onRotationRateChanged(float p_x, float p_y, float p_z, double p_timestamp)
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
        Uri t_uri;
		if (p_url.startsWith("file:") || p_url.startsWith("binfile:"))
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
            
            // Store the actual path
            String t_path;
            t_path = p_url.substring(p_url.indexOf(":") + 1);
            
            // In the new Android permissions model, "file://" and "binfile://" URIs are not allowed
            // for sharing files with other apps. They need to be replaced with "content://" URI and
            // use a FileProvider instead
            t_uri = FileProvider.getProvider(getContext()).addPath(t_path, t_path, t_type, false, ParcelFileDescriptor.MODE_READ_ONLY);
		}
        else
        {
            t_uri = Uri.parse(p_url);
        }
        
		Intent t_view_intent = new Intent(Intent.ACTION_VIEW);
        
		if (t_type != null)
			t_view_intent.setDataAndType(t_uri, t_type);
		else
			t_view_intent.setData(t_uri);
        
        t_view_intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
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

        m_video_control = (VideoControl)m_native_control_module.createControl("com.runrev.android.nativecontrol.VideoControl");
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

    public static final int PERMISSION_REQUEST_CODE = 1;
    public boolean askPermission(String p_permission)
    {
        if (Build.VERSION.SDK_INT >= 23 && getContext().checkSelfPermission(p_permission)
            != PackageManager.PERMISSION_GRANTED)
        {
            Activity t_activity = (LiveCodeActivity)getContext();
            t_activity.requestPermissions(new String[]{p_permission}, PERMISSION_REQUEST_CODE);
        }
        else
            onAskPermissionDone(true);
        return true;
    }
    
    public boolean checkHasPermissionGranted(String p_permission)
    {
        if (Build.VERSION.SDK_INT >= 23)
        {
            return getContext().checkSelfPermission(p_permission) == PackageManager.PERMISSION_GRANTED;
        }
        return true;
    }
    
    
    public boolean checkPermissionExists(String p_permission)
    {
        if (Build.VERSION.SDK_INT >= 23)
        {
            List<PermissionGroupInfo> t_group_info_list = getAllPermissionGroups();
            if (t_group_info_list == null)
                return false;
            
            ArrayList<String> t_group_name_list = new ArrayList<String>();
            for (PermissionGroupInfo t_group_info : t_group_info_list)
            {
                String t_group_name = t_group_info.name;
                if (t_group_name != null)
                    t_group_name_list.add(t_group_name);
            }
            
            for (String t_group_name : t_group_name_list)
            {
                ArrayList<String> t_permission_name_list = getPermissionsForGroup(t_group_name);
                
                if (t_permission_name_list.contains(p_permission))
                    return true;
            }
            return false;
        }
        return true;
    }
    
    private List<PermissionGroupInfo> getAllPermissionGroups()
    {
        final PackageManager t_package_manager = getContext().getPackageManager();
        if (t_package_manager == null)
            return null;
        
        return t_package_manager.getAllPermissionGroups(0);
    }
    
    private ArrayList<String> getPermissionsForGroup(String p_group_name)
    {
        final PackageManager t_package_manager = getContext().getPackageManager();
        final ArrayList<String> t_permission_name_list = new ArrayList<String>();
        
        try
        {
            List<PermissionInfo> t_permission_info_list =
            t_package_manager.queryPermissionsByGroup(p_group_name, PackageManager.GET_META_DATA);
            if (t_permission_info_list != null)
            {
                for (PermissionInfo t_permission_info : t_permission_info_list)
                {
                    String t_permission_name = t_permission_info.name;
                    t_permission_name_list.add(t_permission_name);
                }
            }
        }
        catch (PackageManager.NameNotFoundException e)
        {
            // e.printStackTrace();
            Log.d(TAG, "permissions not found for group = " + p_group_name);
        }
        
        Collections.sort(t_permission_name_list);
        
        return t_permission_name_list;
    }
    
    
    public void showPhotoPicker(String p_source, int p_width, int p_height, int p_jpeg_quality)
    {
        m_photo_width = p_width;
        m_photo_height = p_height;
		m_jpeg_quality = p_jpeg_quality;
        
        if (p_source.contains("camera"))
            showCamera(p_source);
        else if (p_source.equals("album"))
            showLibrary();
        else if (p_source.equals("library"))
            showLibrary();
        else
        {
            doPhotoPickerError("source not available");
        }
        
    }
    
    // sent by the callback
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        onAskPermissionDone(grantResults[0] == PackageManager.PERMISSION_GRANTED);
    }
    
	public void showCamera(String p_source)
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

		String t_path = m_temp_image_file.getPath();

		Uri t_uri;
		t_uri = FileProvider.getProvider(getContext()).addPath(t_path, t_path, "image/jpeg", true, ParcelFileDescriptor.MODE_READ_WRITE);

		Intent t_image_capture = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		
		if (p_source.equals("front camera"))
		{
			t_image_capture.putExtra("android.intent.extras.CAMERA_FACING", 1);
			t_image_capture.putExtra("android.intent.extras.LENS_FACING_FRONT", 1);
			t_image_capture.putExtra("android.intent.extra.USE_FRONT_CAMERA", true);
			
		}
		else if (p_source.equals("rear camera"))
		{
			t_image_capture.putExtra("android.intent.extras.CAMERA_FACING", 0);
			t_image_capture.putExtra("android.intent.extras.LENS_FACING_FRONT", 0);
			t_image_capture.putExtra("android.intent.extra.USE_FRONT_CAMERA", false);
		}
		t_image_capture.putExtra(MediaStore.EXTRA_OUTPUT, t_uri);
		t_image_capture.setFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
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
				
				// HH-2017-01-19: [[ Bug 11313 ]]Support maximum width and height of the image
				if(m_photo_height > 0 && m_photo_width > 0)
				{
					Bitmap t_bitmap = BitmapFactory.decodeStream(t_in);
					
					// scale to required max width/height
					float t_width = t_bitmap.getWidth();
					float t_height = t_bitmap.getHeight();
					
					InputStream t_exif_in = ((LiveCodeActivity)getContext()).getContentResolver().openInputStream(t_photo_uri);
					
					ExifInterface t_exif = new ExifInterface(t_exif_in);
					
					int t_orientation = t_exif.getAttributeInt(ExifInterface.TAG_ORIENTATION,
															   ExifInterface.ORIENTATION_NORMAL);
					
					/* Max width and height need to be flipped if the image orientation is
					 * requires a 90 or 270 degree rotation */
					int t_max_width;
					int t_max_height;
					switch (t_orientation)
					{
						case ExifInterface.ORIENTATION_TRANSPOSE:
						case ExifInterface.ORIENTATION_ROTATE_90:
						case ExifInterface.ORIENTATION_TRANSVERSE:
						case ExifInterface.ORIENTATION_ROTATE_270:
							t_max_height = m_photo_width;
							t_max_width = m_photo_height;
							break;
						default:
							t_max_width = m_photo_width;
							t_max_height = m_photo_height;
							break;
					}
					
					float t_scale = Math.min(t_max_width / t_width, t_max_height / t_height);
					
					Matrix t_matrix = new Matrix();
					t_matrix.setScale(t_scale, t_scale, 0, 0);
					
					switch (t_orientation)
					{
						case ExifInterface.ORIENTATION_FLIP_HORIZONTAL:
							t_matrix.postScale(-1, 1);
							break;
						case ExifInterface.ORIENTATION_ROTATE_180:
							t_matrix.postRotate(180);
							break;
						case ExifInterface.ORIENTATION_FLIP_VERTICAL:
							t_matrix.postRotate(180);
							t_matrix.postScale(-1, 1);
							break;
						case ExifInterface.ORIENTATION_TRANSPOSE:
							t_matrix.postRotate(90);
							t_matrix.postScale(-1, 1);
							break;
						case ExifInterface.ORIENTATION_ROTATE_90:
							t_matrix.postRotate(90);
							break;
						case ExifInterface.ORIENTATION_TRANSVERSE:
							t_matrix.postRotate(-90);
							t_matrix.postScale(-1, 1);
							break;
						case ExifInterface.ORIENTATION_ROTATE_270:
							t_matrix.postRotate(-90);
							break;
					}
		
					Bitmap t_scaled_bitmap = Bitmap.createBitmap(t_bitmap, 0, 0, (int)t_width, (int)t_height, t_matrix, true);
					t_scaled_bitmap.compress(Bitmap.CompressFormat.JPEG, m_jpeg_quality, t_out);
				}
				else
				{
					byte[] t_buffer = new byte[4096];
					int t_readcount;
					while (-1 != (t_readcount = t_in.read(t_buffer)))
					{
						t_out.write(t_buffer, 0, t_readcount);
					}
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
				FileProvider.getProvider(getContext()).removePath(m_temp_image_file.getPath());
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
        // SN-2014-02-03: [[ bug 11069 ]] Pass the Activity to the Email addAttachment
        m_email.addAttachment(getActivity(), path, mime_type, name);
	}

	public void addAttachment(byte[] data, String mime_type, String name)
	{
        // SN-2014-02-03: [[ bug 11069 ]] Pass the Activity to the Email addAttachment
        m_email.addAttachment(getActivity(), data, mime_type, name);
	}

	public void sendEmail()
	{
		((LiveCodeActivity)getContext()).startActivityForResult(m_email.createIntent(), EMAIL_RESULT);
	}

	private void onEmailResult(int resultCode, Intent data)
	{
		m_email.cleanupAttachments(getContext());

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
    private static final int GOOGLE_BILLING_RESULT = 10001;
    private static final int SAMSUNG_BILLING_RESULT = 100;
    private static final int SAMSUNG_ACCOUNT_CERTIFICATION_RESULT = 101;
	
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
            case GOOGLE_BILLING_RESULT:
            case SAMSUNG_BILLING_RESULT:
            case SAMSUNG_ACCOUNT_CERTIFICATION_RESULT:
                mBillingProvider.onActivityResult(requestCode, resultCode, data);
                break;
			default:
				break;
		}
	}

////////////////////////////////////////////////////////////////////////////////
    
    private boolean openGLViewEnabled()
    {
        return m_opengl_view != null && m_opengl_view.getParent() != null;
    }
    
    private void ensureBitmapViewVisibility()
    {
        if (openGLViewEnabled())
        {
            m_bitmap_view.setVisibility(View.INVISIBLE);
        }
        else
        {
            m_bitmap_view.setVisibility(View.VISIBLE);
        }
    }

	public void enableOpenGLView()
	{
        Log.i("revandroid", "enableOpenGLView");
        
        if (m_disabling_opengl)
        {
            m_disabling_opengl = false;
        }
        
        if (!m_enabling_opengl)
        {
            m_enabling_opengl = true;
            
            post(new Runnable() {
                public void run() {
                    Log.i("revandroid", "enableOpenGLView callback");
                    
                    if (!m_disabling_opengl && m_enabling_opengl)
                    {
                        if (!openGLViewEnabled())
                        {
                            Log.i("revandroid", "enableOpenGLView adding");
                            if (m_opengl_view == null)
                            {
                                m_opengl_view = new OpenGLView(getContext());
                            }
                            
                            // Add the view to the hierarchy - we add at the bottom and bring to
                            // the front as soon as we've shown the first frame.
                            ((ViewGroup)getParent()).addView(m_opengl_view, 0,
                                                             new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
                                                                                          FrameLayout.LayoutParams.MATCH_PARENT));
                        }
                        else
                        {
                            // We need to call this explicitly here as we must re-enable drawing
                            m_opengl_view.doSurfaceChanged(m_opengl_view);
                        }
                    }
                    
                    m_enabling_opengl = false;
                    ensureBitmapViewVisibility();
                }
            });
        }
	}

	public void disableOpenGLView()
	{
        Log.i("revandroid", "disableOpenGLView");
        
        if (m_enabling_opengl)
        {
            m_enabling_opengl = false;
        }
        
        if (!m_disabling_opengl)
        {
            m_disabling_opengl = true;
            
            // Before removing the OpenGL mode, make sure we show the bitmap view.
            m_bitmap_view.setVisibility(View.VISIBLE);

            // Move the current opengl view to old.
            // Post an runnable that removes the OpenGL view. Doing that here will
            // cause a black screen.
            post(new Runnable() {
                public void run() {
                    Log.i("revandroid", "disableOpenGLView callback");
                    
                    if (!m_enabling_opengl && m_disabling_opengl)
                    {
                       if (openGLViewEnabled())
                        {
                            Log.i("revandroid", "disableOpenGLView removing");
                            ((ViewGroup)m_opengl_view.getParent()).removeView(m_opengl_view);
                        }
                    }
                    
                    m_disabling_opengl = false;
                    ensureBitmapViewVisibility();
                }
            });
        }
	}
    
    // MW-2015-05-06: [[ Bug 15232 ]] Post a runnable to prevent black flash when enabling openGLView
    public void hideBitmapViewInTime()
    {
        post(new Runnable() {
            public void run() {
                ensureBitmapViewVisibility();
            }
        });
    }
    
	public void showBitmapView()
	{
        // force visible for visual effects
		m_bitmap_view.setVisibility(View.VISIBLE);
	}

////////////////////////////////////////////////////////////////////////////////

	// in-app purchasing

	public static BillingModule mBillingModule;
    
    public static BillingProvider mBillingProvider;
    
    public EnginePurchaseObserver mPurchaseObserver;

	private void initBilling()
	{
        mBillingModule = new BillingModule();
        if (mBillingModule == null)
            return;
        
        mBillingProvider = mBillingModule.getBillingProvider();
        // PM-2014-04-03: [[Bug 12116]] Avoid a NullPointerException is in-app purchasing is not used
        if (mBillingProvider == null)
            return;
        mBillingProvider.setActivity(getActivity());
        mPurchaseObserver = new EnginePurchaseObserver((Activity)getContext());
        mBillingProvider.setPurchaseObserver(mPurchaseObserver);
        mBillingProvider.initBilling();
	}

	public boolean storeCanMakePurchase()
	{
		return mBillingProvider.canMakePurchase();
	}

	public void storeSetUpdates(boolean enabled)
	{
		if (mPurchaseObserver == null)
			return;

		if (enabled)
			mBillingProvider.enableUpdates();
		else
			mBillingProvider.disableUpdates();
	}

	public boolean storeRestorePurchases()
	{
		if (mPurchaseObserver == null)
			return false;

		return mBillingProvider.restorePurchases();
	}

	public boolean purchaseSendRequest(int purchaseId, String productId, String developerPayload)
	{
		if (mPurchaseObserver == null)
			return false;

		Log.i(TAG, "purchaseSendRequest(" + purchaseId + ", " + productId + ")");
        return mBillingProvider.sendRequest(purchaseId, productId, developerPayload);
	}
    
    public boolean storeConsumePurchase(String productID)
    {
        if (mPurchaseObserver == null)
			return false;
        
        return mBillingProvider.consumePurchase(productID);
    }
    
    public boolean storeRequestProductDetails(String productId)
    {
        return mBillingProvider.requestProductDetails(productId);
    }
    
    public String storeReceiveProductDetails(String productId)
    {
        return mBillingProvider.receiveProductDetails(productId);
    }
    
    public boolean storeMakePurchase(String productId, String quantity, String payload)
    {
        return mBillingProvider.makePurchase(productId, quantity, payload);
    }
    
    public boolean storeProductSetType(String productId, String productType)
    {
        Log.d(TAG, "Setting type for productId" + productId + ", type is : " + productType);
        return mBillingProvider.productSetType(productId, productType);
    }
    
    public boolean storeSetPurchaseProperty(String productId, String propertyName, String propertyValue)
    {
        return mBillingProvider.setPurchaseProperty(productId, propertyName, propertyValue);
    }
    
    public String storeGetPurchaseProperty(String productId, String propName)
    {
        return mBillingProvider.getPurchaseProperty(productId, propName);
    }

	public boolean purchaseConfirmDelivery(int purchaseId, String notificationId)
	{
		if (mPurchaseObserver == null)
			return false;

		return mBillingProvider.confirmDelivery(purchaseId);
	}

    public String storeGetPurchaseList()
    {
        return mBillingProvider.getPurchaseList();
    }
    
////////

	private class EnginePurchaseObserver extends PurchaseObserver
	{
		public EnginePurchaseObserver(Activity pActivity)
		{
			super(pActivity);
		}

        // Sent to the observer to indicate a change in the purchase state
        public void onPurchaseStateChanged(String productId, int state)
        {
            final boolean tVerified = true;
            final int tPurchaseState = state;
            final String tNotificationId = "";
            final String tProductId = productId;
            final String tOrderId = storeGetPurchaseProperty(productId, "orderId");
            
            String tTimeString = storeGetPurchaseProperty(productId, "purchaseTime");
            long tTime = 0l;
            try
            {
                tTime = Long.parseLong(tTimeString, 10);
            }
            catch (NumberFormatException nfe)
            {
                
            }
            
            final long tPurchaseTime = tTime;
            final String tDeveloperPayload = storeGetPurchaseProperty(productId, "developerPayload");
            final String tSignedData = "";
            final String tSignature = storeGetPurchaseProperty(productId, "signature");
            
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
        
        public void onProductDetailsReceived(String productId)
        {
            final String tProductId = productId;
            post(new Runnable() {
                public void run() {
                    doProductDetailsResponse(tProductId);
                    if (m_wake_on_event)
                        doProcess(false);
                }
            });
        }
        
        public void onProductDetailsError(String productId, String error)
        {
            final String tProductId = productId;
            final String tError = error;
            post(new Runnable() {
                public void run() {
                    doProductDetailsError(tProductId, tError);
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

	// NFC
	public boolean isNFCAvailable()
	{
		return m_nfc_module.isAvailable();
	}
	
	public boolean isNFCEnabled()
	{
		return m_nfc_module.isEnabled();
	}
	
	public void enableNFCDispatch()
	{
		m_nfc_module.setDispatchEnabled(true);
	}
	
	public void disableNFCDispatch()
	{
		m_nfc_module.setDispatchEnabled(false);
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

//////////

	private static boolean isValueRefCompatible(Class p_class)
	{
		if (String.class == p_class)
			return true;
		if (Integer.class == p_class)
			return true;
		if (Double.class == p_class)
			return true;
		if (Boolean.class == p_class)
			return true;
		if (byte[].class == p_class)
			return true;

		if (p_class.isArray() && isValueRefCompatible(p_class.getComponentType()))
			return true;
			
		return false;
	}
	
	private static boolean isValueRefConvertable(Class p_class)
	{
		if (Bundle.class == p_class)
			return true;
		
		if (p_class.isArray() && isValueRefConvertable(p_class.getComponentType()))
			return true;
		
		return false;
	}
	
	private static Object makeValueRefCompatible(Object p_object)
	{
		// Object types we can pass through safely
		if (isValueRefCompatible(p_object.getClass()))
			return p_object;
			
		// try converting bundle
		else if (p_object instanceof Bundle)
			return bundleToMap((Bundle)p_object);
		
		else if (p_object.getClass().isArray() && isValueRefConvertable(p_object.getClass()))
		{
			Object[] t_input_array = (Object[])p_object;
			Object[] t_converted_array = new Object[t_input_array.length];
			
			for (int i = 0; i < t_input_array.length; i++)
				t_converted_array[i] = makeValueRefCompatible(t_input_array[i]);
			
			return t_converted_array;
		}
		
		// fallback to using toString() method
		return p_object.toString();
	}
	
	private static Map<String, Object> bundleToMap(Bundle p_bundle)
	{
		Map<String, Object> t_map;
		t_map = new HashMap<String, Object>();
		
		for (String t_key : p_bundle.keySet())
		{
			Object t_value;
			t_value = p_bundle.get(t_key);
			
			if (t_value != null)
			{
				Object t_converted;
				t_converted = makeValueRefCompatible(t_value);
			
				if (t_converted != null)
					t_map.put(t_key, t_converted);
				else
					Log.i(TAG, "conversion failed for bundle key " + t_key);
			}
		}
		
		return t_map;
	}
	
	// IM-2015-07-08: [[ LaunchData ]] Retreive info from launch Intent and return as a Map object.
	public Map<String, Object> getLaunchData()
	{
		Intent t_intent = ((Activity)getContext()).getIntent();
		
		// For now we're just storing strings, though this could change if we include the 'extra' data
		Map<String, Object> t_data;
		t_data = new HashMap<String, Object>();
		
		String t_value;
		
		if (t_intent != null)
		{
			t_value = t_intent.getAction();
			if (t_value != null)
				t_data.put("action", t_value);
			
			t_value = t_intent.getDataString();
			if (t_value != null)
				t_data.put("data", t_value);
			
			t_value = t_intent.getType();
			if (t_value != null)
				t_data.put("type", t_value);
			
			Set<String> t_categories;
			t_categories = t_intent.getCategories();
			
			if (t_categories != null && !t_categories.isEmpty())
			{
				// Store categories as a string of comma-separated values
				StringBuilder t_category_list;
				t_category_list = new StringBuilder();
				boolean t_first = true;
				for (String t_category : t_categories)
				{
					if (!t_first)
						t_category_list.append(',');
					t_category_list.append(t_category);
					t_first = false;
				}
				
				t_data.put("categories", t_category_list.toString());
			}
			
			// IM-2015-08-04: [[ Bug 15684 ]] Retrieve Intent extra data.
			Bundle t_extras;
			t_extras = t_intent.getExtras();
			
			if (t_extras != null && !t_extras.isEmpty())
				t_data.put("extras", bundleToMap(t_extras));
		}
		
		return t_data;
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
		/* Pause registered listeners in reverse order of registration as
		 * then components will be paused before any components they
		 * depend on. */
		for (int i = m_lifecycle_listeners.size() - 1; i >= 0; i--)
		{
			m_lifecycle_listeners.get(i).OnPause();
		}

		if (m_text_editor_visible)
			hideKeyboard();
		
		s_running = false;

		m_shake_listener.onPause();
		m_orientation_listener.disable();

        if (m_sensor_module != null)
            m_sensor_module.onPause();

        if (m_sound_module != null)
            m_sound_module.onPause();

		if (m_native_control_module != null)
			m_native_control_module.onPause();
		
		if (m_nfc_module != null)
			m_nfc_module.onPause();
		
		if (m_video_is_playing)
			m_video_control . suspend();

		doPause();
	}

	public void onResume()
	{
		m_shake_listener.onResume();
		m_orientation_listener.enable();
        // PM-2014-04-07: [[Bug 12099]] On awakening Android Device from sleep, make sure we update the orientation
        // so as no part of the screen is blacked out
        updateOrientation(getDeviceRotation());

		if (m_sensor_module != null)
            m_sensor_module.onResume();

        if (m_sound_module != null)
            m_sound_module.onResume();
		
		if (m_native_control_module != null)
			m_native_control_module.onResume();
		
		if (m_nfc_module != null)
			m_nfc_module.onResume();

		if (m_video_is_playing)
			m_video_control . resume();

		doResume();

		if (m_new_intent)
		{
			if (m_nfc_module != null)
				m_nfc_module.onNewIntent(((Activity)getContext()).getIntent());
				
			doLaunchDataChanged();
			
			String t_launch_url;
			t_launch_url = getLaunchUri();
			if (t_launch_url != null)
				doLaunchFromUrl(t_launch_url);

			m_new_intent = false;
		}

		s_running = true;
		if (m_text_editor_visible)
			showKeyboard();
		
		// IM-2013-08-16: [[ Bugfix 11103 ]] dispatch any remote notifications received while paused
		dispatchNotifications();

		/* Resume registered listeners in order of registration as then
		 * components will be resumed before any components which depend
		 * on them. */
		for (int i = 0; i < m_lifecycle_listeners.size(); i++)
		{
			m_lifecycle_listeners.get(i).OnResume();
		}

		if (m_wake_on_event)
			doProcess(false);
	}

	public void onDestroy()
	{
		doDestroy();
        // PM-2014-04-03: [[Bug 12116]] Avoid a NullPointerException is in-app purchasing is not used
        if (mBillingProvider != null)
            mBillingProvider.onDestroy();
        s_engine_instance = null;
	}

    ////////

    public void onNewIntent(Intent intent)
    {
		// IM-2015-10-08: [[ Bug 15417 ]] Update the Intent of the Activity to the new one.
		((Activity)getContext()).setIntent(intent);
		m_new_intent = true;
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
        Log.i("revandroid", String.format("exportToAlbum called: %s %s", t_file_name, t_file_type));
        File t_file = null;
        UUID t_uuid;
        if (t_image_data == null)
            return "export failed";
        else
        {
            File t_folder = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
            String t_filename;
            
            // SN-2015-01-05: [[ Bug 11417 ]] Ensure that the folder exists.
            t_folder.mkdirs();
            
            // The user did not supply a file name, so create one now
            // SN-2015-04-29: [[ Bug 15296 ]] From 7.0 onwards, t_file_name will
            //   not be nil, but empty
            if (t_file_name . isEmpty())
            {
                t_uuid = UUID.randomUUID();
                Log.i("revandroid", "Generated File Name: " + Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES) + "/I" + t_uuid.toString().substring(0,7) + t_file_type);
                t_filename = "I" + t_uuid.toString().substring(0,7) + t_file_type;
            }
            else
            {
                t_filename = t_file_name + t_file_type;
            }
            
            // SN-2015-01-05: [[ Bug 11417 ]] Let File create the file with the File returned
            //  by getExternalStoragePublicDirectory and t_file_type.
            t_file = new File(t_folder, t_filename);
            
            FileOutputStream fs = null;
            try
            {
                fs = new FileOutputStream(t_file);
                fs.write(t_image_data,0,t_image_data.length);
                
                fs.close();
                
                // SN-2015-01-05 [[ Bug 11417 ]] Ask the Media Scanner to scan the newly created file.
                Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
                intent.setData(Uri.fromFile(t_file));
                getContext().sendBroadcast(intent);
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
            String t_path = "";
            if (t_data != null)
            {
                Cursor t_cursor = null;
                try
                {
                    t_cursor = ((LiveCodeActivity) getContext())
                        .getContentResolver()
                        .query(t_data, null, null, null, null);
                }
                catch (SecurityException e) {}

                if (t_cursor != null)
                {
                    t_cursor.moveToFirst();
                    int t_index = t_cursor.getColumnIndex(MediaStore.Images.ImageColumns.DATA);
                    if (t_index > 0)
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
    
    public Object createTypefaceFromAsset(String path)
    {
        Log.i("revandroid", "createTypefaceFromAsset");
        return Typeface.createFromAsset(getContext().getAssets(),path);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    
    private X509TrustManager getTrustManager()
    {
        if (m_trust_manager != null)
            return m_trust_manager;
        
        try
        {
            TrustManagerFactory t_trust_manager_factory;
            t_trust_manager_factory = TrustManagerFactory . getInstance("X509");
            t_trust_manager_factory . init((KeyStore) null);
            
            TrustManager[] t_trust_managers;
            t_trust_managers = t_trust_manager_factory . getTrustManagers();
            
            if (t_trust_managers != null)
            {
                for (TrustManager t_trust_manager : t_trust_managers)
                {
                    if (t_trust_manager instanceof X509TrustManager)
                    {
                        m_trust_manager = (X509TrustManager) t_trust_manager;
                        break;
                    }
                }
            }
        }
        catch (Exception e)
        {
            m_trust_manager = null;
        }
        
        return m_trust_manager;
    }
    
    private X509Certificate[] certDataToX509CertChain(Object[] p_cert_chain)
    {
        X509Certificate[] t_cert_chain;
        try
        {
            t_cert_chain = new X509Certificate[p_cert_chain . length];
            
            CertificateFactory t_cert_factory;
            t_cert_factory = CertificateFactory . getInstance("X.509");

            for (int i = 0; i < p_cert_chain . length; i++)
            {
                byte[] t_cert_data;
                t_cert_data = (byte[]) p_cert_chain[i];
                
                InputStream t_input_stream;
                t_input_stream = new ByteArrayInputStream(t_cert_data);
                
                Certificate t_cert;
                t_cert = t_cert_factory . generateCertificate(t_input_stream);
                
                t_cert_chain[i] = (X509Certificate) t_cert;
            }
        }
        catch (Exception e)
        {
            t_cert_chain = null;
        }

        return t_cert_chain;
    }
    
    private boolean hostNameMatchesCertificateDNSName(String p_host_name, String p_cert_host_name)
    {
        if (p_host_name == null || p_host_name . isEmpty() || p_cert_host_name == null || p_cert_host_name . isEmpty())
            return false;

        p_host_name = p_host_name . toLowerCase();
        p_cert_host_name = p_cert_host_name . toLowerCase();
        
        if (!p_cert_host_name . contains("*"))
            return p_host_name . equals(p_cert_host_name);

        if (p_cert_host_name . startsWith("*.") && p_host_name . regionMatches(0, p_cert_host_name, 2, p_cert_host_name . length() - 2))
            return true;
        
        String t_cert_host_name_prefix;
        t_cert_host_name_prefix = p_cert_host_name . substring(0, p_cert_host_name . indexOf('*'));
        if (t_cert_host_name_prefix != null && !t_cert_host_name_prefix . isEmpty() && !p_host_name . startsWith(t_cert_host_name_prefix))
            return false;
            
        String t_cert_host_name_suffix;
        t_cert_host_name_suffix = p_cert_host_name . substring(p_cert_host_name . indexOf('*') + 1);
        if (t_cert_host_name_suffix != null && !t_cert_host_name_suffix . isEmpty() && !p_host_name . endsWith(t_cert_host_name_suffix))
            return false;
        
        return true;
    }
    
    private boolean hostNameIsValidForCertificate(String p_host_name, X509Certificate p_certificate)
    {
        Collection t_subject_alt_names;
        try
        {
            t_subject_alt_names = p_certificate . getSubjectAlternativeNames();
        }
        catch (Exception e)
        {
            return false;
        }
        
        if (t_subject_alt_names != null)
        {
            for (Object t_subject_alt_name : t_subject_alt_names)
            {
                List t_entry;
                t_entry = (List) t_subject_alt_name;
                if (t_entry == null || t_entry . size() < 2)
                    continue;
                
                Integer t_alt_name_type;
                t_alt_name_type = (Integer) t_entry . get(0);
                if (t_alt_name_type == null || t_alt_name_type != 2 /* DNS NAME */)
                    continue;
                
                String t_alt_name;
                t_alt_name = (String) t_entry . get(1);
                if (t_alt_name == null)
                    continue;
                
                if (hostNameMatchesCertificateDNSName(p_host_name, t_alt_name))
                    return true;
            }
        }
        
        return false;
    }
    
    // MM-2015-06-11: [[ MobileSockets ]] Return true if the given certifcate chain should be trusted and matches the passed domain name.
    public boolean verifyCertificateChainIsTrusted(Object[] p_cert_data, String p_host_name)
    {
        boolean t_success;
        t_success = true;
        
        X509Certificate[] t_cert_chain;
        t_cert_chain = null;
        if (t_success)
        {
            t_cert_chain = certDataToX509CertChain(p_cert_data);
            t_success = t_cert_chain != null;
        }
        
        X509TrustManager t_trust_manager;
        t_trust_manager = null;
        if (t_success)
        {
            t_trust_manager = getTrustManager();
            t_success = t_trust_manager != null;
        }
        
        if (t_success)
        {
            try
            {
                t_trust_manager . checkServerTrusted(t_cert_chain, "RSA");
            }
            catch (CertificateException t_exception)
            {
                m_last_certificate_verification_error = t_exception . toString();
                t_success = false;
            }
        }
        
        if (t_success)
            t_success = hostNameIsValidForCertificate(p_host_name, t_cert_chain[0]);
        
        return t_success;
    }
    
    // MM-2015-06-11: [[ MobileSockets ]] Return the last certificate verifcation error (if any) and reset the error to null.
    public String getLastCertificateVerificationError()
    {
        String t_error;
        t_error = m_last_certificate_verification_error;
        m_last_certificate_verification_error = null;
        return t_error;
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

    public interface ServiceListener
    {
        public void onStart(Context context);
        public void onFinish(Context context);
    }
    
    Context m_service_context = null;
    ArrayList<ServiceListener> m_running_services = null;
    ArrayList<ServiceListener> m_pending_services = null;
    
    private int serviceListFind(ArrayList<ServiceListener> p_list, ServiceListener p_obj)
    {
        for(int i = 0; i < p_list.size(); i++)
        {
            if (p_list.get(i) == p_obj)
            {
                return i;
            }
        }
        return -1;
    }
    
    public void startService(ServiceListener p_listener)
    {
        if (m_running_services == null)
        {
            m_pending_services = new ArrayList<ServiceListener>();
        }
        
        m_pending_services.add(p_listener);
        
        Intent t_service = new Intent(getContext(), getServiceClass());
        getContext().startService(t_service);
    }
    
    public void stopService(ServiceListener p_listener)
    {
        if (serviceListFind(m_pending_services, p_listener) != -1)
        {
            m_pending_services.remove(serviceListFind(m_pending_services, p_listener));
            return;
        }
        
        if (serviceListFind(m_running_services, p_listener) != -1)
        {
            p_listener.onFinish(m_service_context);
            m_running_services.remove(serviceListFind(m_running_services, p_listener));
        }
        
        if (m_pending_services.isEmpty() &&
            m_running_services.isEmpty())
        {
            Intent t_service = new Intent(getContext(), getServiceClass());
            getContext().stopService(t_service);
        }
    }
    
    public int handleStartService(Context p_service_context, Intent p_intent, int p_flags, int p_start_id)
    {
        if (m_pending_services == null ||
            m_pending_services.isEmpty())
        {
            if (m_running_services == null ||
                m_running_services.isEmpty())
            {
                Intent t_service = new Intent(getContext(), getServiceClass());
                getContext().stopService(t_service);
                return Service.START_NOT_STICKY;
            }
            return Service.START_STICKY;
        }
        
        m_service_context = p_service_context;
        
        ServiceListener t_listener = m_pending_services.get(0);
        m_pending_services.remove(0);
        
        if (m_running_services == null)
        {
            m_running_services = new ArrayList<ServiceListener>();
        }
        
        m_running_services.add(t_listener);
        t_listener.onStart(p_service_context);
        
        return Service.START_STICKY;
    }
    
    public void handleFinishService(Context p_service_context)
    {
        m_pending_services = null;
        
        while(!m_running_services.isEmpty())
        {
            m_running_services.get(0).onFinish(p_service_context);
            m_running_services.remove(0);
        }
        m_running_services = null;
        
        m_service_context = null;
    }
    
    public Class getServiceClass()
    {
        return ((LiveCodeActivity)getContext()).getServiceClass();
    }
	
	////////////////////////////////////////////////////////////////////////////////
	
	public int getSystemAppearance()
	{
		switch (m_night_mode)
		{
			case Configuration.UI_MODE_NIGHT_YES:
				return 1;
			case Configuration.UI_MODE_NIGHT_NO:
			case Configuration.UI_MODE_NIGHT_UNDEFINED:
			default:
				return 0;
		}
	}

	////////////////////////////////////////////////////////////////////////////////

	public boolean registerLifecycleListener(LifecycleListener p_listener)
	{
		return m_lifecycle_listeners.add(p_listener);
	}

	public boolean unregisterLifecycleListener(LifecycleListener p_listener)
	{
		/* We can't remove the listener directly since LifecycleListener does
		 * not implement equals. Instead, search backwards through the array
		 * until we find the passed listener. */
		for (int i = m_lifecycle_listeners.size() - 1; i >= 0; i--)
		{
			if (m_lifecycle_listeners.get(i) == p_listener)
			{
				m_lifecycle_listeners.remove(i);
				return true;
			}
		}

		return false;
	}

    ////////////////////////////////////////////////////////////////////////////////
    
    // url launch callback
    public static native void doLaunchFromUrl(String url);
	// intent launch callback
	public static native void doLaunchDataChanged();

	// callbacks from the billing service

	public static native void doPurchaseStateChanged(boolean verified, int purchaseState,
		String notificationId, String productId, String orderId, long purchaseTime,
		String developerPayload, String signedData, String signature);

	public static native void doConfirmNotificationResponse(int purchaseId, int responseCode);
	public static native void doRestoreTransactionsResponse(int responseCode);
	public static native void doRequestPurchaseResponse(int purchaseId, int responseCode);
    public static native void doProductDetailsResponse(String productId);
    public static native void doProductDetailsError(String productId, String error);


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

	public static native void doNativeNotify(long callback, long context);
			 
////////////////////////////////////////////////////////////////////////////////

	// Our engine interface

	public static native void doProcess(boolean timedout);

	public static native void doReconfigure(int x, int y, int w, int h, Bitmap bitmap);

    public static native String doGetCustomPropertyValue(String set, String property);
    
    public static native Rect doGetFocusedRect();

	// MW-2013-08-07: [[ ExternalsApiV5 ]] Native wrapper around MCScreenDC::wait
	//   used by runActivity() API.
	public static native void doWait(double time, boolean dispatch, boolean anyevent);
	
    // sensor handlers
    public static native void doLocationChanged(double p_latitude, double p_longitude, double p_altitude, double p_timestamp, float p_accuracy, double p_speed, double p_course);
    public static native void doHeadingChanged(double p_heading, double p_magnetic_heading, double p_true_heading, double p_timestamp,
                                               float p_x, float p_y, float p_z, float p_accuracy);
	public static native void doAccelerationChanged(float x, float y, float z, double timestamp);
	public static native void doRotationRateChanged(float x, float y, float z, double timestamp);

    // input event handlers
	public static native void doBackPressed();
    public static native void doMenuKey();
    public static native void doSearchKey();
	public static native void doTouch(int action, int id, int timestamp, int x, int y);
	public static native void doKeyPress(int modifiers, int char_code, int key_code);
	public static native void doShake(int action, long timestamp);

	public static native void doOrientationChanged(int orientation);
	public static native void doSystemAppearanceChanged();

	public static native void doKeyboardShown(int height);
	public static native void doKeyboardHidden();

    // dialog handlers
	public static native void doAnswerDialogDone(int action);
	public static native void doAskDialogDone(String result);
    public static native void doDatePickerDone(int year, int month, int day, boolean done);
    public static native void doTimePickerDone(int hour, int minute, boolean done);
    public static native void doListPickerDone(int index, boolean done);
    public static native void doAskPermissionDone(boolean granted);

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
