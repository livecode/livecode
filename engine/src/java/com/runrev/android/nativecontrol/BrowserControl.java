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

package com.runrev.android.nativecontrol;

import android.app.AlertDialog;
import android.content.*;
import android.content.pm.*;
import android.graphics.*;
import android.media.*;
import android.util.*;
import android.view.*;
import android.webkit.*;
import android.widget.*;

import java.lang.reflect.*;
import java.net.*;
import java.security.*;
import java.util.*;
import java.util.regex.*;

class BrowserControl extends NativeControl
{
    public static final String TAG = "revandroid.NativeBrowserControl";
    
	private static Method mWebView_getOverScrollMode;
	private static Method mWebView_setOverScrollMode;
	
	private boolean m_is_apk_url;
	private boolean m_scrolling_enabled;
	
	private VideoView m_custom_video_view;
	private FrameLayout m_custom_view_container;
	private WebChromeClient.CustomViewCallback m_custom_view_callback;
	private WebChromeClient m_chrome_client;
	
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
	
    public BrowserControl(NativeControlModule p_module)
    {
        super(p_module);
		m_is_apk_url = false;
		m_scrolling_enabled = true;
    }
    
    class JSInterface
    {
    	@JavascriptInterface
        public void storeResult(String p_tag, String p_result)
        {
            doJSExecutionResult(p_tag, p_result);
            m_module.getEngine().wakeEngineThread();
        }
    }
    
    static final Pattern ACCEPTED_URI_SCHEMA = Pattern.compile(
															   "(?i)" + // switch on case insensitive matching
															   "(" +    // begin group for schema
															   "(?:http|https|file):\\/\\/" +
															   "|(?:inline|data|about|javascript):" +
															   ")" +
															   "(.*)" );
	
	private boolean isSpecializedHandlerAvailable(Intent intent)
	{
        PackageManager pm = NativeControlModule.getContext().getPackageManager();
		List<ResolveInfo> handlers = pm.queryIntentActivities(intent,
															  PackageManager.GET_RESOLVED_FILTER);
		if (handlers == null || handlers.size() == 0) {
			return false;
		}
		for (ResolveInfo resolveInfo : handlers) {
			IntentFilter filter = resolveInfo.filter;
			if (filter == null) {
				// No intent filter matches this intent
				// Error on the side of staying in the browser, ignore
				continue;
			}
			if (filter.countDataAuthorities() == 0 || filter.countDataPaths() == 0) {
				// Generic handler, skip
				continue;
			}
			return true;
		}
		return false;
    }
	
	private boolean useExternalHandler(String p_url)
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
		if (NativeControlModule.getContext().getPackageManager().resolveActivity(intent, 0) == null) {
			return false;
		}
		
		// sanitize the Intent, ensuring web pages can not bypass browser
		// security (only access to BROWSABLE activities).
		intent.addCategory(Intent.CATEGORY_BROWSABLE);
		intent.setComponent(null);
		
		if (ACCEPTED_URI_SCHEMA.matcher(p_url).matches())
			return false;
		
		try {
			if (NativeControlModule.getActivity().startActivityIfNeeded(intent, -1)) {
				return true;
			}
		} catch (ActivityNotFoundException ex) {
			// ignore the error. If no application can handle the URL,
			// eg about:blank, assume the browser can handle it.
		}
		
		return false;
	}
	
    public View createView(Context p_context)
    {
        WebView t_view = new WebView(p_context);
        t_view.setWebViewClient(new WebViewClient() {
            public boolean shouldOverrideUrlLoading(WebView p_view, String p_url)
            {
				
				//                //Log.i(TAG, String.format("shouldOverrideUrlLoading(%s)", p_url));
				if (useExternalHandler(p_url))
					return true;
				
                setUrl(p_url);
                return true;
            }
            
            public void onPageStarted(WebView view, String url, Bitmap favicon)
            {
                //Log.i(TAG, "onPageStarted() - " + url);
                doStartedLoading(toAPKPath(url));
                m_module.getEngine().wakeEngineThread();
            }
            
            public void onPageFinished(WebView view, String url)
            {
                //Log.i(TAG, "onPageFinished() - " + url);
                doFinishedLoading(toAPKPath(url));
                m_module.getEngine().wakeEngineThread();
            }
            
            public void onReceivedError(WebView view, int errorCode, String description, String failingUrl)
            {
                doLoadingError(toAPKPath(failingUrl), description);
                m_module.getEngine().wakeEngineThread();
            }
        });
        
        t_view.setOnTouchListener(new View.OnTouchListener()
								  {
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
                return false;
            }
        });
        
		m_chrome_client = new WebChromeClient() {
			@Override
			public void onShowCustomView(View view, CustomViewCallback callback)
			{
				// Log.i(TAG, "onShowCustomView()");
				m_custom_view_container = new FrameLayout(getView().getContext()) {
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
				t_dialog = new AlertDialog.Builder(getView().getContext());
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
		};
		t_view.setWebChromeClient(m_chrome_client);
        t_view.getSettings().setJavaScriptEnabled(true);
        t_view.getSettings().setAllowFileAccessFromFileURLs(true);
        t_view.getSettings().setAllowUniversalAccessFromFileURLs(true);
		t_view.getSettings().setGeolocationEnabled(true);
		t_view.getSettings().setDomStorageEnabled(true);
        t_view.getSettings().setPluginState(WebSettings.PluginState.ON);
        t_view.getSettings().setBuiltInZoomControls(true);
        t_view.addJavascriptInterface(new JSInterface(), "revjs");
        
        return t_view;
    }
	
	public boolean handleBackPressed()
	{
		if (m_custom_view_container != null && m_chrome_client != null)
		{
			m_chrome_client.onHideCustomView();
			return true;
		}
		
		return false;
	}
	
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
    
	public String toAPKPath(String p_url)
	{
		if (!m_is_apk_url)
			return p_url;
		
		if (!p_url.startsWith("file://"))
			return p_url;
		
		String t_url = stripExtraSlashes(p_url.substring(7));
		
		if (!t_url.startsWith(s_asset_path))
			return p_url;
		
		String t_package_path = m_module.getEngine().getPackagePath();
		return "file:" + t_package_path + t_url.substring(s_asset_path.length());
	}
	
	public String fromAPKPath(String p_url)
	{
		m_is_apk_url = false;
		
		if (!p_url.startsWith("file:"))
			return p_url;
		
		String t_package_path = m_module.getEngine().getPackagePath();
		String t_url = stripExtraSlashes(p_url.substring(5));
		
		if (!t_url.startsWith(t_package_path))
			return p_url;
		
		m_is_apk_url = true;
		return "file://" + s_asset_path + t_url.substring(t_package_path.length());
	}
	
    public void setUrl(String p_url)
    {
		p_url = fromAPKPath(p_url);
		
        ((WebView)m_control_view).loadUrl(p_url);
    }
    
    public String getUrl()
    {
		return toAPKPath(((WebView)m_control_view).getUrl());
    }
    
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
	
	public boolean getScrollingEnabled()
	{
		return m_scrolling_enabled;
	}
    
	public void setScrollingEnabled(boolean p_enabled)
	{
		if (m_scrolling_enabled == p_enabled)
			return;
		m_scrolling_enabled = p_enabled;
		((WebView)m_control_view).setHorizontalScrollBarEnabled(p_enabled);
		((WebView)m_control_view).setVerticalScrollBarEnabled(p_enabled);
        ((WebView)m_control_view).getSettings().setBuiltInZoomControls(p_enabled);
		if (!m_scrolling_enabled)
		{
			m_control_view.setOnTouchListener(new View.OnTouchListener() {
				@Override
				public boolean onTouch(View v, MotionEvent event) {
					return (event.getAction() == MotionEvent.ACTION_MOVE);
				}
			});
		}
		else
			m_control_view.setOnTouchListener(null);
	}
	
    public void goBack(int p_steps)
    {
        ((WebView)m_control_view).goBackOrForward(-p_steps);
    }
    
    public void goForward(int p_steps)
    {
        ((WebView)m_control_view).goBackOrForward(p_steps);
    }
    
    public void reload()
    {
        ((WebView)m_control_view).reload();
    }
    
    public void stop()
    {
        ((WebView)m_control_view).stopLoading();
    }
    
    public void loadData(String p_base_url, String p_html)
    {       
        ((WebView)m_control_view).loadDataWithBaseURL(fromAPKPath(p_base_url), p_html, "text/html", "utf-8", null);
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
    
    public String executeJavaScript(String p_javascript)
    {
        SecureRandom t_random = new SecureRandom();
        long t_tag = 0;
        while (t_tag == 0)
            t_tag = t_random.nextLong();
        
		//        Log.i(TAG, "generated tag: " + t_tag);
        String t_js = String.format("javascript:window.revjs.storeResult('%d', eval('%s'))", t_tag, escapeJSString(p_javascript));
		//        Log.i(TAG, t_js);
        ((WebView)m_control_view).loadUrl(t_js);
        
        return Long.toString(t_tag);
    }
    
    public static native void doJSExecutionResult(String tag, String result);
    public native void doStartedLoading(String url);
    public native void doFinishedLoading(String url);
    public native void doLoadingError(String url, String error);
}
