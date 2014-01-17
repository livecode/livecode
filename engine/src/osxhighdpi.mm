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

#include "graphics.h"

#include <objc/objc-runtime.h>
#include <AppKit/AppKit.h>

typedef float (*backingScaleFactorIMP)(id);

// IM-2014-01-17: [[ HiDPI ]] returns the maximum backing scale of all attached screens
bool MCOSXGetScreenBackingScale(MCGFloat &r_scale)
{
	static backingScaleFactorIMP s_backingScaleFactor = nil;
	static bool s_initialized = false;
	
	if (!s_initialized)
	{
		// Make sure the NSScreen object has a "backingScale" method - present from 10.7 onward
		s_initialized = true;
		if (![NSScreen instancesRespondToSelector:@selector(backingScaleFactor)])
			return false;
		
		// Objects can handle messages not specified in their header, but there seems to be no way to specify the return type,
		// so here we get the method implementation and cast to a function with the expected return type (float).
		s_backingScaleFactor = (backingScaleFactorIMP)[NSScreen instanceMethodForSelector:@selector(backingScaleFactor)];
	}
	
	if (s_backingScaleFactor == nil)
		return false;
	
	
	MCGFloat t_max_scale;
	bool t_have_max;
	t_have_max = false;
	
	NSArray *t_screens;
	t_screens = [NSScreen screens];
	
	if (t_screens == nil)
		return false;
	
	for (uindex_t i = 0; i < t_screens.count; i++)
	{
		NSScreen *t_screen;
		t_screen = [t_screens objectAtIndex:i];
		
		MCGFloat t_screen_scale;
//		t_screen_scale = [t_screen backingScaleFactor];
		t_screen_scale = s_backingScaleFactor(t_screen);
		
		if (!t_have_max || t_max_scale < t_screen_scale)
		{
			t_max_scale = t_screen_scale;
			t_have_max = true;
		}
	}
	
	if (t_have_max)
		r_scale = t_max_scale;
	
	return t_have_max;
}
