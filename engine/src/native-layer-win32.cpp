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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


#include "util.h"
#include "mcerror.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "widget.h"
#include "param.h"
#include "osspec.h"
#include "cmds.h"
#include "scriptpt.h"
#include "hndlrlst.h"
#include "debug.h"
#include "redraw.h"
#include "font.h"
#include "chunk.h"
#include "group.h"
#include "graphicscontext.h"

#include "graphics_util.h"

#include "globals.h"
#include "context.h"

#include "native-layer-win32.h"


MCNativeLayerWin32::MCNativeLayerWin32(MCObject *p_object, HWND p_view) :
	m_hwnd(p_view),
	m_cached(NULL),
	m_viewport_hwnd(nil)
{
	m_object = p_object;
}

MCNativeLayerWin32::~MCNativeLayerWin32()
{
	if (m_viewport_hwnd != NULL)
	{
		if (m_hwnd != nil)
			doDetach();
		DestroyWindow(m_viewport_hwnd);
	}
	if (m_cached != NULL)
		DeleteObject(m_cached);
}

// IM-2016-01-19: [[ NativeLayer ]] Windows (prior to Windows 8) doesn't do transparent child windows
//    so instead we place each child window in its own container, sized to expose only the area that
//    should be visible when the native layer is clipped by the bounds of any groups it is in.
void MCNativeLayerWin32::doAttach()
{
	HWND t_parent;
	t_parent = getStackWindow();

	if (m_viewport_hwnd == nil)
		/* UNCHECKED */ CreateNativeContainer(m_object, (void*&)m_viewport_hwnd);

	// Set the parent to the stack
	SetParent(m_viewport_hwnd, t_parent);

	SetParent(m_hwnd, m_viewport_hwnd);

	// Restore the state of the widget (in case it changed due to a
	// tool change while on another card - we don't get a message then)
	doSetViewportGeometry(m_viewport_rect);
	doSetGeometry(m_rect);
	doSetVisible(ShouldShowLayer());
}

void MCNativeLayerWin32::doDetach()
{
    // Change the window to an invisible child of the desktop
	doSetVisible(false);
	SetParent(m_hwnd, NULL);
	SetParent(m_viewport_hwnd, NULL);
}

bool MCNativeLayerWin32::doPaint(MCGContextRef p_context)
{
	MCRectangle t_rect;
	t_rect = m_rect;

	bool t_success;
	t_success = true;
	
	// Create a DC to use for the drawing operation. We create a new DC
	// compatible to the one that would normally be used for a paint operation.
	HDC t_hwindowdc;
	t_hwindowdc = GetDC(m_hwnd);
	
	HDC t_hdc;
	t_hdc = nil;
	if (t_success)
	{
		t_hdc = CreateCompatibleDC(t_hwindowdc);
		t_success = t_hdc != nil;
	}

	// Create a bitmap for the DC to draw into
	if (t_success && m_cached == NULL)
	{
		// Note: this *must* be the original DC because the compatible DC originally
		// has a monochrome (1BPP) bitmap selected into it and we don't want that.
		m_cached = CreateCompatibleBitmap(t_hwindowdc, t_rect.width, t_rect.height);
		t_success = m_cached != nil;
	}

	BITMAP t_bitmap;

	void *t_bits;
	t_bits = nil;
	
	if (t_success)
	{
		// Tell the DC to draw into the bitmap we've created
		SelectObject(t_hdc, m_cached);

		// Use the WM_PRINT message to get the control to draw. WM_PAINT should
		// only be called by the windowing system and MSDN recommends PRINT instead
		// if drawing into a specific DC is required.
		SendMessage(m_hwnd, WM_PRINT, (WPARAM)t_hdc, PRF_CHILDREN|PRF_CLIENT);

		// Get the information we need to turn this into a bitmap the engine can use
		GetObjectW(m_cached, sizeof(BITMAP), &t_bitmap);

		// Allocate some memory for capturing the bits from the bitmap
		t_success = MCMemoryAllocate(t_bitmap.bmWidth * t_bitmap.bmHeight * 4, t_bits);
	}

	MCGImageRef t_gimage;
	t_gimage = nil;

	if (t_success)
	{
		// Describe the format of the device-independent bitmap we want
		BITMAPINFOHEADER t_bi;
		t_bi.biSize = sizeof(BITMAPINFOHEADER);
		t_bi.biWidth = t_bitmap.bmWidth;
		t_bi.biHeight = -t_bitmap.bmHeight;	// Negative: top-down (else bottom-up bitmap)
		t_bi.biPlanes = 1;
		t_bi.biBitCount = 32;
		t_bi.biCompression = BI_RGB;
		t_bi.biSizeImage = 0;
		t_bi.biXPelsPerMeter = 0;
		t_bi.biYPelsPerMeter = 0;
		t_bi.biClrUsed = 0;
		t_bi.biClrImportant = 0;

		// Finally, we can get the bits that were drawn.
		GetDIBits(t_hdc, m_cached, 0, t_bitmap.bmHeight, t_bits, (LPBITMAPINFO)&t_bi, DIB_RGB_COLORS);

		// Process the bits to handle the alpha channel
		uint32_t *t_argb;
		t_argb = reinterpret_cast<uint32_t*>(t_bits);
		for (size_t i = 0; i < t_bitmap.bmWidth*t_bitmap.bmHeight; i++)
		{
			// Set to fully-opaque (kMCGRasterFormat_xRGB doesn't seem to work...)
			t_argb[i] |= 0xFF000000;
		}

		// Turn these bits into something the engine can draw
		MCGRaster t_raster;
		MCImageDescriptor t_descriptor;
		t_raster.format = kMCGRasterFormat_ARGB;
		t_raster.width = t_bitmap.bmWidth;
		t_raster.height = t_bitmap.bmHeight;
		t_raster.stride = 4*t_bitmap.bmWidth;
		t_raster.pixels = t_bits;
		
		t_success = MCGImageCreateWithRasterNoCopy(t_raster, t_gimage);
	}

	if (t_success)
	{
		// At last - we can draw it!
		MCGRectangle rect = {{0, 0}, {MCGFloat(t_rect.width), MCGFloat(t_rect.height)}};
		MCGContextDrawImage(p_context, t_gimage, rect, kMCGImageFilterNone);
	}

	if (t_gimage != nil)
		MCGImageRelease(t_gimage);

	// Clean up the drawing resources that we allocated
	if (t_bits != nil)
		MCMemoryDeallocate(t_bits);

	if (t_hdc != nil)
		DeleteDC(t_hdc);

	ReleaseDC(m_hwnd, t_hwindowdc);
	
	return t_success;
}

void MCNativeLayerWin32::updateViewGeometry()
{
	m_intersect_rect = MCU_intersect_rect(m_viewport_rect, m_rect);

	// IM-2016-02-18: [[ Bug 16603 ]] Transform view rect to device coords
	MCRectangle t_intersect_rect;
	t_intersect_rect = MCRectangleGetTransformedBounds(m_intersect_rect, m_object->getstack()->getdevicetransform());

	// Move the window. Only trigger a repaint if not in edit mode
	MoveWindow(m_viewport_hwnd, t_intersect_rect.x, t_intersect_rect.y, t_intersect_rect.width, t_intersect_rect.height, ShouldShowLayer());

	// IM-2016-02-18: [[ Bug 16603 ]] Transform view rect to device coords
	MCRectangle t_rect;
	t_rect = MCRectangleGetTransformedBounds(m_rect, m_object->getstack()->getdevicetransform());

	t_rect.x -= t_intersect_rect.x;
	t_rect.y -= t_intersect_rect.y;

    // Move the window. Only trigger a repaint if not in edit mode
	MoveWindow(m_hwnd, t_rect.x, t_rect.y, t_rect.width, t_rect.height, ShouldShowLayer());

	// We need to delete the bitmap that we've been caching
	DeleteObject(m_cached);
	m_cached = NULL;
}

void MCNativeLayerWin32::doSetViewportGeometry(const MCRectangle &p_rect)
{
	m_viewport_rect = p_rect;
	updateViewGeometry();
}

void MCNativeLayerWin32::doSetGeometry(const MCRectangle& p_rect)
{
	m_rect = p_rect;
	updateViewGeometry();
}

void MCNativeLayerWin32::doSetVisible(bool p_visible)
{
	ShowWindow(m_hwnd, p_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
	ShowWindow(m_viewport_hwnd, p_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
}

void MCNativeLayerWin32::doRelayer()
{
    // Find which native layer this should be inserted after
	MCObject *t_before;
	t_before = findNextLayerBelow(m_object);

	// Insert the widget in the correct place (but only if the card is current)
	if (isAttached() && m_object->getstack()->getcard() == m_object->getstack()->getcurcard())
	{
		HWND t_insert_after;
		if (t_before != NULL)
		{
			MCNativeLayerWin32 *t_before_layer;
			t_before_layer = reinterpret_cast<MCNativeLayerWin32*>(t_before->getNativeLayer());
			t_insert_after = t_before_layer->m_hwnd;
		}
		else
		{
			t_insert_after = HWND_BOTTOM;
		}

		// Only the window Z order needs to be adjusted
		SetWindowPos(m_hwnd, t_insert_after, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	}	
}

HWND MCNativeLayerWin32::getStackWindow()
{
    return (HWND)m_object->getstack()->getrealwindow();
}

bool MCNativeLayerWin32::GetNativeView(void *&r_view)
{
	r_view = m_hwnd;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCNativeLayer::CreateNativeLayer(MCObject *p_object, void *p_view)
{
	return new MCNativeLayerWin32(p_object, (HWND)p_view);
}

extern HINSTANCE MChInst;
bool getcontainerclass(ATOM &r_class)
{
	static ATOM s_container_class = 0;

	if (s_container_class == 0)
	{
		WNDCLASSEX t_class;
		MCMemoryClear(t_class);

		t_class.cbSize = sizeof(WNDCLASSEX);
		t_class.lpfnWndProc = DefWindowProc;
		t_class.hInstance = MChInst;
		t_class.lpszClassName = "LCCONTAINER";

		s_container_class = RegisterClassEx(&t_class);

		DWORD t_err;
		t_err = GetLastError();

		if (s_container_class == 0)
			return false;
	}

	r_class = s_container_class;

	return true;
}

bool MCNativeLayer::CreateNativeContainer(MCObject *p_object, void *&r_container)
{
	ATOM t_class;
	if (!getcontainerclass(t_class))
		return false;

	HWND t_container;
	t_container = CreateWindow((LPCSTR)t_class, "Container", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1, (HWND)p_object->getstack()->getrealwindow(), nil, MChInst, nil);

	DWORD t_err;
	t_err = GetLastError();

	if (t_container == nil)
		return false;

	r_container = t_container;
	return true;
}

void MCNativeLayer::ReleaseNativeView(void *p_view)
{
	DestroyWindow((HWND)p_view);
}

////////////////////////////////////////////////////////////////////////////////
