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

import android.util.*;
import android.view.*;
import android.content.*;
import android.content.res.*;
import android.content.pm.*;
import android.graphics.*;
import android.graphics.drawable.*;

public class BitmapView extends View
{
	private Rect m_clip_bounds;
	private Paint m_dither_paint;
	private Bitmap m_bitmap;
	private boolean m_showing_splash;
	
	// Constructors

	public BitmapView(Context context)
	{
		super(context);
		initialize();
	}

	public BitmapView(Context context, AttributeSet attrs)
	{
		super(context, attrs);
		initialize();
	}

	// Event handling

	@Override
	public boolean onTouchEvent(MotionEvent p_event)
	{
		if (m_showing_splash)
			return true;
	
		return false;
	}
	
	@Override
	protected void onDraw(Canvas canvas)
	{
		// If no bitmap, we have nothing to draw
		if (m_bitmap == null)
			return;
		
		// If we are all clipped, we have nothing to draw
		if (!canvas . getClipBounds(m_clip_bounds))
			return;
		
		// Otherwise render our bitmap
		if (m_showing_splash)
			drawSplashScreen(canvas);
		else
			canvas . drawBitmap(m_bitmap, 0, 0, m_dither_paint);
	}
	
	// Life-cycle management
	
	private void initialize()
	{
		// Save us reallocating a rect every time
		m_clip_bounds = new Rect();
		
		// We don't have a bitmap for now as we don't know our size
		m_bitmap = null;
		
		// Create a dithering paint
		m_dither_paint = new Paint();
		m_dither_paint . setDither(true);
		
		// Turn off the drawing cache as we don't need it
		setDrawingCacheEnabled(false);
	}
	
	public void resizeBitmap(int p_new_width, int p_new_height)
	{
		if (m_bitmap != null)
		{
			m_bitmap . recycle();
			m_bitmap = null;
		}
		
		m_bitmap = Bitmap . createBitmap(p_new_width, p_new_height, Bitmap . Config . ARGB_8888);
	}
	
	public Bitmap getBitmap()
	{
		return m_bitmap;
	}
	
	// Splash-screen display.
	
	public void showSplashScreen()
	{
		m_showing_splash = true;
		invalidate();
	}
	
	public void hideSplashScreen()
	{
		if (!m_showing_splash)
			return;
		
		m_showing_splash = false;
		invalidate();
	}
	
	public void drawSplashScreen(Canvas p_canvas)
	{
		int t_width, t_height;
		t_width = getWidth();
		t_height = getHeight();
		
		float t_image_size_ratio, t_logo_size_ratio, t_text_size_ratio;
		float t_image_loc_ratio, t_logo_loc_ratio, t_text_loc_ratio;
		if (t_width > t_height)
		{
			t_image_size_ratio = 400.0f / 768.0f;
			t_image_loc_ratio = 260.0f / 768.0f;
			t_logo_size_ratio = 360.0f / 768.0f;
			t_logo_loc_ratio = 580.0f / 768.0f;
			t_text_size_ratio = 440.0f / 768.0f;
			t_text_loc_ratio = 690.0f / 768.0f;
		}
		else
		{
			t_image_size_ratio = 600.0f / 1024.0f;
			t_image_loc_ratio = 384.0f / 1024.0f;
			t_logo_size_ratio = 360.0f / 1024.0f;
			t_logo_loc_ratio = 828.0f / 1024.0f;
			t_text_size_ratio = 440.0f / 1024.0f;
			t_text_loc_ratio = 940.0f / 1024.0f;
		}
		
		String t_package;
		t_package = getContext().getPackageName();
		Resources t_resources;
		t_resources = getContext().getResources();
		
		p_canvas . drawRGB(255, 255, 255);
		
		int t_splash_image_id;
		t_splash_image_id = t_resources.getIdentifier("drawable/splash_image", null, t_package);
		Drawable t_splash_image;
		t_splash_image = t_resources.getDrawable(t_splash_image_id);
		
		int t_splash_logo_id;
		t_splash_logo_id = t_resources.getIdentifier("drawable/splash_logo", null, t_package);
		Drawable t_splash_logo;
		t_splash_logo = t_resources.getDrawable(t_splash_logo_id);
		
		int t_splash_text_id;
		t_splash_text_id = t_resources.getIdentifier("drawable/splash_text", null, t_package);
		Drawable t_splash_text;
		t_splash_text = t_resources.getDrawable(t_splash_text_id);
		
		int t_splash_x, t_splash_y, t_splash_width, t_splash_height;
		t_splash_width = (int)(t_height * t_image_size_ratio);
		t_splash_height = t_splash_width * t_splash_image . getIntrinsicHeight() / t_splash_image . getIntrinsicWidth();
		t_splash_x = t_width / 2 - t_splash_width / 2;
		t_splash_y = (int)(t_height * t_image_loc_ratio) - t_splash_height / 2;
		t_splash_image . setBounds(t_splash_x, t_splash_y, t_splash_x + t_splash_width, t_splash_y + t_splash_height);
		t_splash_image . setDither(true);
		t_splash_image . draw(p_canvas);
		
		int t_logo_x, t_logo_y, t_logo_width, t_logo_height;
		t_logo_width = (int)(t_height * t_logo_size_ratio);
		t_logo_height = t_logo_width * t_splash_logo . getIntrinsicHeight() / t_splash_logo . getIntrinsicWidth();
		t_logo_x = t_width / 2 - t_logo_width / 2;
		t_logo_y = (int)(t_height * t_logo_loc_ratio) - t_logo_height / 2;
		t_splash_logo . setBounds(t_logo_x, t_logo_y, t_logo_x + t_logo_width, t_logo_y + t_logo_height);
		t_splash_logo . setDither(true);
		t_splash_logo . draw(p_canvas);
		
		int t_text_x, t_text_y, t_text_width, t_text_height;
		t_text_width = (int)(t_height * t_text_size_ratio);
		t_text_height = t_text_width * t_splash_text . getIntrinsicHeight() / t_splash_text . getIntrinsicWidth();
		t_text_x = t_width / 2 - t_text_width / 2;
		t_text_y = (int)(t_height * t_text_loc_ratio) - t_text_height / 2;
		t_splash_text . setBounds(t_text_x, t_text_y, t_text_x + t_text_width, t_text_y + t_text_height);
		t_splash_text . setDither(true);
		t_splash_text . draw(p_canvas);
	}
};
