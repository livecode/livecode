/* Copyright (C) 2015 LiveCode Ltd.

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

package com.runrev.android.libraries;

import com.runrev.android.*;
import com.runrev.android.nativecontrol.NativeControlModule;

import android.app.AlertDialog;
import android.app.Activity;
import android.content.*;
import android.content.pm.*;
import android.graphics.*;
import android.media.*;
import android.util.*;
import android.view.*;
import android.webkit.*;
import android.widget.*;
import android.net.*;
import android.os.*;;
import android.media.*;
import android.provider.*;

import java.lang.reflect.*;
import java.net.*;
import java.security.*;
import java.util.*;
import java.util.regex.*;
import java.io.*;

import org.json.JSONArray;
import org.json.JSONException;

public class LibBrowser
{
    public static final String TAG = "revandroid.LibBrowser";

	//public boolean handleBackPressed()
	//{
	//	if (m_custom_view_container != null && m_chrome_client != null)
	//	{
	//		m_chrome_client.onHideCustomView();
	//		return true;
	//	}
	//	
	//	return false;
	//}
	
	public static String stripExtraSlashes(String p_path)
	{
		int i = 0;
		while (p_path.length() > (i + 1) && p_path.charAt(i) == '/' && p_path.charAt(i + 1) == '/')
			i++;
		if (i > 0)
			return p_path.substring(i);
		
		return p_path;
	}
	
	private static final String s_asset_path = "/android_asset";
	private static final String s_file_url_prefix = "file://";
	private static final String s_livecode_file_url_prefix = "file:";
    
	public static String toAPKPath(String p_url)
	{
		if (!p_url.startsWith(s_file_url_prefix))
			return p_url;
		
		String t_url = stripExtraSlashes(p_url.substring(s_file_url_prefix.length()));
		
		if (!t_url.startsWith(s_asset_path))
			return p_url;
		
		String t_package_path = Engine.getEngine().getPackagePath();
		return s_livecode_file_url_prefix + t_package_path + t_url.substring(s_asset_path.length());
	}
	
	public static String fromAPKPath(String p_url)
	{
		if (!p_url.startsWith(s_livecode_file_url_prefix))
			return p_url;
		
		String t_package_path = Engine.getEngine().getPackagePath();
		String t_url = stripExtraSlashes(p_url.substring(s_livecode_file_url_prefix.length()));
		
		if (!t_url.startsWith(t_package_path))
			return p_url;
		
		return s_file_url_prefix + s_asset_path + t_url.substring(t_package_path.length());
	}

	public static String fromAssetPath(String p_url)
	{
		String t_package_path = Engine.getEngine().getPackagePath();
		
		if(!p_url.startsWith(t_package_path))
			return p_url;

		return "file:///android_asset" + p_url.substring(t_package_path.length());
	}
	
	/*
    public boolean canGoBack()
    {
        return ((WebView)m_control_view).canGoBack();
    }
    
    public boolean canGoForward()
    {
        return ((WebView)m_control_view).canGoForward();
    }
	
	public boolean getCanBounce()
	{
		if (mWebView_getOverScrollMode == null)
			return true;
		
		try
		{
			return ((int)(Integer)mWebView_getOverScrollMode.invoke(m_control_view, (Object[])null)) == 0;
		}
		catch(Exception e)
		{
		}
		
		return true;
	}
	
	public void setCanBounce(boolean p_enabled)
	{
		if (mWebView_setOverScrollMode == null)
			return;
		
		try
		{
			mWebView_setOverScrollMode.invoke(m_control_view, new Object[] { new Integer(p_enabled ? 0 : 2) });
		}
		catch(Exception e)
		{
		}
	}
	*/
	
//////////

	public static Object createBrowserView()
	{
		return new LibBrowserWebView(Engine.getEngine().getContext());
	}
}

class LibBrowserWebView extends WebView
{
	public static final String TAG = "revandroid.LibBrowserWebView";
	
	private VideoView m_custom_video_view;
	private FrameLayout m_custom_view_container;
	private WebChromeClient.CustomViewCallback m_custom_view_callback;
	private WebChromeClient m_chrome_client;

	private String m_js_handlers = "";
	private List<String> m_js_handler_list = null;

	private boolean m_allow_user_interaction = true;

	public LibBrowserWebView(Context p_context)
	{
		super(p_context);
		
		setWebViewClient(new WebViewClient() {
			public boolean shouldOverrideUrlLoading(WebView p_view, String p_url)
			{
				//Log.i(TAG, String.format("shouldOverrideUrlLoading(%s)", p_url));
				if (useExternalHandler(getContext(), p_url))
					return true;
	
				setUrl(p_url);
				return true;
			}
		
			public void onPageStarted(WebView view, String url, Bitmap favicon)
			{
				//Log.i(TAG, "onPageStarted() - " + url);
				
				//doStartedLoading(toAPKPath(url));
				doStartedLoading(url);
				wakeEngineThread();
			}
		
			public void onPageFinished(WebView view, String url)
			{
				//Log.i(TAG, "onPageFinished() - " + url);
				
				// Install jshandlers after page has loaded (before doesn't work!)
				if (m_js_handler_list != null)
					addJSHandlers(m_js_handler_list);
				
				//doFinishedLoading(toAPKPath(url));
				doFinishedLoading(url);
				wakeEngineThread();
			}
		
			public void onReceivedError(WebView view, int errorCode, String description, String failingUrl)
			{
				//doLoadingError(toAPKPath(failingUrl), description);
				if (errorCode == ERROR_UNSUPPORTED_SCHEME)
					doUnsupportedScheme(failingUrl);
				else
					doLoadingError(failingUrl, description);
				wakeEngineThread();
			}
		});
	
		setOnTouchListener(new View.OnTouchListener() {
			@Override
			public boolean onTouch(View v, MotionEvent event)
			{
				switch (event.getAction())
				{
					case MotionEvent.ACTION_DOWN:
					case MotionEvent.ACTION_UP:
						if (!v.hasFocus())
						{
							v.requestFocus();
						}
						break;
				}
				return !m_allow_user_interaction;
			}
		});

		class MCWebChromeClient extends WebChromeClient implements LiveCodeActivity.OnActivityResultListener
		{
			private static final int WEBVIEW_INPUT_FILE_RESULT = 9999;
			private ValueCallback<Uri[]> m_browser_file_path_callback = null;
			private File m_temp_image_file = null;

			public void onActivityResult (int p_request_code, int p_result_code, Intent p_data)
			{
				LiveCodeActivity t_activity = (LiveCodeActivity)getContext();
				t_activity.removeOnActivityResultListener(p_request_code);

				switch (p_request_code)
				{
					case WEBVIEW_INPUT_FILE_RESULT:
						Uri[] t_uris = null;
						if (p_result_code == Activity.RESULT_OK)
						{
							if (p_data.getData() != null)
							{
								t_uris = WebChromeClient.FileChooserParams.parseResult(p_result_code, p_data);
							}
							else if (m_temp_image_file != null)
							{
								t_uris = new Uri[] {Uri.fromFile(m_temp_image_file)};
							}
						}

						m_browser_file_path_callback.onReceiveValue(t_uris);
						m_browser_file_path_callback = null;

						if (m_temp_image_file != null)
						{
							FileProvider.getProvider(getContext()).removePath(m_temp_image_file.getPath());
							m_temp_image_file = null;
						}
					default:
						break;
				}
			}

			@Override
			public boolean onShowFileChooser(WebView p_webview, ValueCallback<Uri[]> p_file_path_callback, WebChromeClient.FileChooserParams p_file_chooser_params)
			{
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
				{
					try {
						LiveCodeActivity t_activity = (LiveCodeActivity)getContext();

						Intent t_intent = p_file_chooser_params.createIntent();

						List<Intent> t_extra_intents = new ArrayList<Intent>();

						Intent t_gallery_intent = new Intent(Intent.ACTION_PICK, MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
						t_gallery_intent.setType("image/*");
						t_extra_intents.add(t_gallery_intent);

						boolean t_have_temp_file = false;
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

						if (t_have_temp_file)
						{
							String t_path = m_temp_image_file.getPath();

							Uri t_uri;
							t_uri = FileProvider.getProvider(getContext()).addPath(t_path, t_path, "image/jpeg", false, ParcelFileDescriptor.MODE_READ_WRITE);

							Intent t_image_capture = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);

							t_image_capture.putExtra(MediaStore.EXTRA_OUTPUT, t_uri);
							t_image_capture.setFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
							t_extra_intents.add(t_image_capture);
						}

						Intent t_chooser_intent = Intent.createChooser(t_intent, "Select a file");;
						t_chooser_intent.putExtra(Intent.EXTRA_INITIAL_INTENTS, t_extra_intents.toArray(new Parcelable[] {}));

						if (t_chooser_intent.resolveActivity(t_activity.getPackageManager()) == null)
						{
							return false;
						}

						m_browser_file_path_callback = p_file_path_callback;
						t_activity.setOnActivityResultListener(this, WEBVIEW_INPUT_FILE_RESULT);
						t_activity.startActivityForResult(t_chooser_intent,  WEBVIEW_INPUT_FILE_RESULT);

						return true;
					}
					catch (Exception e)
					{

					}
				}
				return false;
			}

			@Override
			public void onShowCustomView(View view, CustomViewCallback callback)
			{
				// Log.i(TAG, "onShowCustomView()");
				m_custom_view_container = new FrameLayout(getContext()) {
					@Override
					public boolean onTouchEvent(MotionEvent ev)
					{
						return true;
					}
				};
			
				m_custom_view_container.setBackgroundColor(Color.BLACK);
			
				if (view instanceof FrameLayout)
				{
					FrameLayout t_frame = (FrameLayout)view;
				
					if (t_frame.getFocusedChild() instanceof VideoView)
					{
						m_custom_video_view = (VideoView)t_frame.getFocusedChild();
					
						t_frame.removeView(m_custom_video_view);
						view = m_custom_video_view;
					
						m_custom_video_view.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
							@Override
							public void onCompletion(MediaPlayer mp)
							{
								// Log.i(TAG, "onCompletion()");
								mp.stop();
								onHideCustomView();
							}
						});
					
						m_custom_video_view.setOnErrorListener(new MediaPlayer.OnErrorListener() {
							@Override
							public boolean onError(MediaPlayer mp, int what, int extra)
							{
								// Log.i(TAG, "onError()");
								onHideCustomView();
								return true;
							}
						});
					
						m_custom_video_view.start();
					}
				}
			
				// Log.i(TAG, "adding to native control container and showing");
			
				m_custom_view_container.addView(view,
												new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
																			 FrameLayout.LayoutParams.MATCH_PARENT,
																			 Gravity.CENTER));
			
				m_custom_view_callback = callback;
			
				RelativeLayout t_control_container = NativeControlModule.getNativeControlContainer();
				ViewGroup.LayoutParams t_layout = NativeControlModule.createLayoutParams(0, 0, t_control_container.getWidth(), t_control_container.getHeight());
			
				t_control_container.addView(m_custom_view_container, t_layout);
				m_custom_view_container.setVisibility(View.VISIBLE);
			}
		
			@Override
			public void onHideCustomView()
			{
				// Log.i(TAG, "onHideCustomView()");
				if (m_custom_video_view != null)
				{
					m_custom_video_view.setVisibility(View.GONE);
					m_custom_view_container.removeView(m_custom_video_view);
					m_custom_video_view = null;
				}
			
				if (m_custom_view_container != null)
				{
					m_custom_view_container.setVisibility(View.GONE);
					NativeControlModule.getNativeControlContainer().removeView(m_custom_view_container);
					m_custom_view_container = null;
				
					m_custom_view_callback.onCustomViewHidden();
					m_custom_view_callback = null;
				}
			}
			
			
			public void showRequestAccessDialog(final String origin, final GeolocationPermissions.Callback callback, String p_title, String p_message, String p_ok_button, String p_cancel_button)
			{
				DialogInterface.OnClickListener t_listener;
				t_listener = new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface p_dialog, int p_which)
					{
						boolean t_remember = true;
						boolean t_allow = true;
						if (p_which == DialogInterface.BUTTON_POSITIVE)
							t_allow = true;
						else if (p_which == DialogInterface.BUTTON_NEGATIVE)
							t_allow = false;
						callback.invoke(origin, t_allow, t_remember);
					} };
				
				AlertDialog.Builder t_dialog;
				t_dialog = new AlertDialog.Builder(getContext());
				t_dialog . setTitle(p_title);
				t_dialog . setMessage(p_message);
				t_dialog . setPositiveButton(p_ok_button, t_listener);
				if (p_cancel_button != null)
					t_dialog . setNegativeButton(p_cancel_button, t_listener);
				
				t_dialog . show();
			}
			
			public void onGeolocationPermissionsShowPrompt( String origin,  GeolocationPermissions.Callback callback) {
				showRequestAccessDialog(origin, callback, "Location Access", origin + " would like to use your Current Location", "Allow", "Don't Allow");
			}

			@Override
			public void onProgressChanged(WebView p_view, int p_progress)
			{
				doProgressChanged(p_view.getUrl(), p_progress);
				wakeEngineThread();
			}
		}

		m_chrome_client = new MCWebChromeClient();

		setWebChromeClient(m_chrome_client);
		getSettings().setJavaScriptEnabled(true);
        getSettings().setAllowFileAccessFromFileURLs(true);
        getSettings().setAllowUniversalAccessFromFileURLs(true);
		getSettings().setGeolocationEnabled(true);
		getSettings().setDomStorageEnabled(true);
		getSettings().setPluginState(WebSettings.PluginState.ON);
		getSettings().setBuiltInZoomControls(true);
		addJavascriptInterface(new JSInterface(), "liveCode");
	}
	
	private static Method mWebView_getOverScrollMode;
	private static Method mWebView_setOverScrollMode;
	static
	{
		try
		{
			mWebView_getOverScrollMode = View.class.getMethod("getOverScrollMode", (Class[])null);
			mWebView_setOverScrollMode = View.class.getMethod("setOverScrollMode", new Class[] { Integer.TYPE });
		}
		catch (Exception e)
		{
			mWebView_getOverScrollMode = null;
			mWebView_setOverScrollMode = null;
		}
	}

	class JSInterface
	{
		@JavascriptInterface
		public void __invokeHandler(String p_handler, String p_json_args)
		{
			if (m_js_handler_list != null && m_js_handler_list.contains(p_handler))
			{
				JSONArray t_args;
				try
				{
					t_args = new JSONArray(p_json_args);
				}
				catch (JSONException e)
				{
					t_args = new JSONArray();
				}
				
				doCallJSHandler(p_handler, t_args);
				wakeEngineThread();
			}
		}

		@JavascriptInterface
		public void __storeExecuteJavaScriptResult(String p_tag, String p_result)
		{
			doJSExecutionResult(p_tag, p_result);
			wakeEngineThread();
		}
	}
	
	/* PROPERTIES */
	
	private void addJSHandler(String p_handler)
	{
		String t_js = String.format("javascript:liveCode.%s = function() {window.liveCode.__invokeHandler('%s', JSON.stringify(Array.prototype.slice.call(arguments))); }", p_handler, p_handler);
		loadUrl(t_js);
	}

	private void removeJSHandler(String p_handler)
	{
		String t_js = String.format("javascript:delete window.liveCode.%s", p_handler, p_handler);
		loadUrl(t_js);
	}
	
	private void addJSHandlers(List<String> p_handlers)
	{
		for (String t_handler : p_handlers)
			addJSHandler(t_handler);
	}
	
	public void setJavaScriptHandlers(String p_handlers)
	{
		String[] t_handlers = null;
		
		if (!p_handlers.isEmpty())
			t_handlers = p_handlers.split("\n");
		
		if (m_js_handler_list != null)
		{
			for (String t_handler : m_js_handler_list)
				removeJSHandler(t_handler);
		}
		
		m_js_handler_list = null;
		
		if (t_handlers != null)
		{
			m_js_handler_list = Arrays.asList(t_handlers);
			addJSHandlers(m_js_handler_list);
		}
		
		m_js_handlers = p_handlers;
	}
	
	public String getJavaScriptHandlers()
	{
		return m_js_handlers;
	}
	
	public void setUrl(String p_url)
	{
		// HH-2017-01-11: [[ Bug 19036  ]] If p_url is point to asset folder change prefix to "file:///android_asset/".
		p_url = LibBrowser.fromAssetPath(p_url);

		loadUrl(p_url);
	}
	
	//public String getUrl()
	//{
	//	return toAPKPath(super.getUrl());
	//}

	public boolean getVerticalScrollbarEnabled()
	{
		return isVerticalScrollBarEnabled();
	}
	
	public void setVerticalScrollbarEnabled(boolean p_enabled)
	{
		setVerticalScrollBarEnabled(p_enabled);
	}
	
	public boolean getHorizontalScrollbarEnabled()
	{
		return isHorizontalScrollBarEnabled();
	}
	
	public void setHorizontalScrollbarEnabled(boolean p_enabled)
	{
		setHorizontalScrollBarEnabled(p_enabled);
	}
	
	public String getUserAgent()
	{
		return getSettings().getUserAgentString();
	}
	
	public void setUserAgent(String p_useragent)
	{
		getSettings().setUserAgentString(p_useragent);
	}

	public boolean getIsSecure()
	{
		return getCertificate() != null;
	}

	public boolean getAllowUserInteraction()
	{
		return m_allow_user_interaction;
	}

	public void setAllowUserInteraction(boolean p_value)
	{
		m_allow_user_interaction = p_value;
	}
	
	/* ACTIONS */

	public void goBack(int p_steps)
	{
		goBackOrForward(-p_steps);
	}
	
	public void goForward(int p_steps)
	{
		goBackOrForward(p_steps);
	}
	
	/*
	public void reload()
	{
		super.reload();
	}
	*/
	
	public void stop()
	{
		stopLoading();
	}
	
	public void loadHtml(String p_base_url, String p_html)
	{       
		//loadDataWithBaseURL(fromAPKPath(p_base_url), p_html, "text/html", "utf-8", null);
		loadDataWithBaseURL(p_base_url, p_html, "text/html", "utf-8", null);
	}

	public String executeJavaScript(String p_javascript)
	{
		SecureRandom t_random = new SecureRandom();
		long t_tag = 0;
		while (t_tag == 0)
			t_tag = t_random.nextLong();
		
		//        Log.i(TAG, "generated tag: " + t_tag);
		String t_js = String.format("javascript:{var t_result = ''; try {t_result = eval('%s');} catch(e){t_result = String(e);} window.liveCode.__storeExecuteJavaScriptResult('%d', t_result); }", escapeJSString(p_javascript), t_tag);
		//        Log.i(TAG, t_js);
		loadUrl(t_js);
		
		return Long.toString(t_tag);
	}
	
	/* UTILITY METHODS */
	
    private static void wakeEngineThread()
    {
		Engine.getEngine().wakeEngineThread();
	}
	
    private static final Pattern ACCEPTED_URI_SCHEMA = Pattern.compile(
															   "(?i)" + // switch on case insensitive matching
															   "(" +    // begin group for schema
															   "(?:http|https|file):\\/\\/" +
															   "|(?:inline|data|about|javascript):" +
															   ")" +
															   "(.*)" );
	
	private static boolean useExternalHandler(Context p_context, String p_url)
	{
		Intent intent;
		// perform generic parsing of the URI to turn it into an Intent.
		try {
			intent = Intent.parseUri(p_url, Intent.URI_INTENT_SCHEME);
		} catch (URISyntaxException ex) {
			Log.w("Browser", "Bad URI " + p_url + ": " + ex.getMessage());
			return false;
		}
		
		// check whether the intent can be resolved.
		if (p_context.getPackageManager().resolveActivity(intent, 0) == null) {
			return false;
		}
		
		// sanitize the Intent, ensuring web pages can not bypass browser
		// security (only access to BROWSABLE activities).
		intent.addCategory(Intent.CATEGORY_BROWSABLE);
		intent.setComponent(null);
		
		if (ACCEPTED_URI_SCHEMA.matcher(p_url).matches())
			return false;
		
		try {
			if (((Activity)p_context).startActivityIfNeeded(intent, -1)) {
				return true;
			}
		} catch (ActivityNotFoundException ex) {
			// ignore the error. If no application can handle the URL,
			// eg about:blank, assume the browser can handle it.
		}
		
		return false;
	}
	
    public static String escapeJSString(String p_string)
    {
        String t_result = "";
        for (char t_char : p_string.toCharArray())
        {
            switch (t_char)
            {
                case '\'':
                case '\"':
                case '\\':
                    t_result += "\\" + t_char;
                    break;
                    
                case '\n':
                    t_result += "\\n";
                    break;
                    
                case '\r':
                    t_result += "\\r";
                    break;
                    
                case '\t':
                    t_result += "\\r";
                    break;
                    
                case '\b':
                    t_result += "\\r";
                    break;
                    
                case '\f':
                    t_result += "\\r";
                    break;
                    
                default:
                    t_result += t_char;
                    break;
            }
        }
        
        return t_result;
    }
    
	/* NATIVE METHODS */
	
	public native void doCallJSHandler(String p_handler, JSONArray p_args);
	public native void doJSExecutionResult(String tag, String result);
	public native void doStartedLoading(String url);
	public native void doFinishedLoading(String url);
	public native void doLoadingError(String url, String error);
	public native void doUnsupportedScheme(String url);
	public native void doProgressChanged(String url, int progress);
}
