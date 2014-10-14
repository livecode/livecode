/* Copyright (C) 2014 Runtime Revolution Ltd.
 
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

#include "execpt.h"
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
#include "graphicscontext.h"

#include "globals.h"
#include "context.h"

#include "native-layer-win32.h"


MCNativeLayerWin32::MCNativeLayerWin32(MCWidget *p_widget) :
  m_widget(p_widget),
  m_hwnd(NULL),
  m_cached(NULL)
{

}

MCNativeLayerWin32::~MCNativeLayerWin32()
{
	if (m_hwnd != NULL)
		DestroyWindow(m_hwnd);
	if (m_cached != NULL)
		DeleteObject(m_cached);
}

void MCNativeLayerWin32::OnToolChanged(Tool p_new_tool)
{
    if (p_new_tool == T_BROWSE || p_new_tool == T_HELP)
    {
        // In run mode. Make visible if requested
        if (m_widget->getflags() & F_VISIBLE)
            ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        m_widget->Redraw();
    }
    else
    {
        // In edit mode
        ShowWindow(m_hwnd, SW_HIDE);
        m_widget->Redraw();
    }
}

void MCNativeLayerWin32::OnOpen()
{
	// Unhide the widget, if required
	if (isAttached() && m_widget->getopened() == 1)
        doAttach();
}

void MCNativeLayerWin32::OnClose()
{
	if (isAttached() && m_widget->getopened() == 0)
        doDetach();
}

void MCNativeLayerWin32::OnAttach()
{
    m_attached = true;
    doAttach();
}

void MCNativeLayerWin32::doAttach()
{
    if (m_hwnd == NULL)
    {
        MCRectangle rect = m_widget->getrect();
        m_hwnd = CreateWindow
        (
         L"BUTTON",
         L"Native Button",
         WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|BS_DEFPUSHBUTTON,
         rect.x,
         rect.y,
         rect.width,
         rect.height,
         getStackWindow(),
         NULL,
         (HINSTANCE)GetWindowLong(getStackWindow(), GWL_HINSTANCE),
         NULL
         );
    }

	// Set the parent to the stack
	SetParent(m_hwnd, getStackWindow());

	// Restore the visibility state of the widget (in case it changed due to a
	// tool change while on another card - we don't get a message then)
	if ((m_widget->getflags() & F_VISIBLE) && !m_widget->inEditMode())
		ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
	else
		ShowWindow(m_hwnd, SW_HIDE);
}

void MCNativeLayerWin32::OnDetach()
{
    m_attached = false;
    doDetach();
}

void MCNativeLayerWin32::doDetach()
{
    // Change the window to an invisible child of the desktop
	ShowWindow(m_hwnd, SW_HIDE);
	SetParent(m_hwnd, NULL);
}

void MCNativeLayerWin32::OnPaint(MCDC* p_dc, const MCRectangle& p_dirty)
{
	// If the widget is not in edit mode, we trust it to paint itself
	if (!m_widget->inEditMode())
		return;

	// Create a DC to use for the drawing operation. We create a new DC
	// compatible to the one that would normally be used for a paint operation.
	HDC t_hdc, t_hwindowdc;
	t_hwindowdc = GetDC(m_hwnd);
	t_hdc = CreateCompatibleDC(t_hwindowdc);

	// Create a bitmap for the DC to draw into
	if (m_cached == NULL)
	{
		// Note: this *must* be the original DC because the compatible DC originally
		// has a monochrome (1BPP) bitmap selected into it and we don't want that.
		MCRectangle t_rect;
		t_rect = m_widget->getrect();
		m_cached = CreateCompatibleBitmap(t_hwindowdc, t_rect.width, t_rect.height);
	}

	// Tell the DC to draw into the bitmap we've created
	SelectObject(t_hdc, m_cached);

	// Use the WM_PRINT message to get the control to draw. WM_PAINT should
	// only be called by the windowing system and MSDN recommends PRINT instead
	// if drawing into a specific DC is required.
	SendMessage(m_hwnd, WM_PRINT, (WPARAM)t_hdc, PRF_CHILDREN|PRF_CLIENT);

	// Get the information we need to turn this into a bitmap the engine can use
	BITMAP t_bitmap;
	GetObject(m_cached, sizeof(BITMAP), &t_bitmap);

	// Allocate some memory for capturing the bits from the bitmap
	HANDLE t_dib;
	void *t_bits;
	t_dib = GlobalAlloc(GHND, t_bitmap.bmWidth * t_bitmap.bmHeight * 4);
	t_bits = GlobalLock(t_dib);

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
	MCGImageRef t_gimage;
	MCImageDescriptor t_descriptor;
	t_raster.format = kMCGRasterFormat_ARGB;
	t_raster.width = t_bitmap.bmWidth;
	t_raster.height = t_bitmap.bmHeight;
	t_raster.stride = 4*t_bitmap.bmWidth;
	t_raster.pixels = t_bits;
	/* UNCHECKED */ MCGImageCreateWithRasterNoCopy(t_raster, t_gimage);
	memset(&t_descriptor, 0, sizeof(MCImageDescriptor));
	t_descriptor.image = t_gimage;
	t_descriptor.x_scale = t_descriptor.y_scale = 1.0;

	// At last - we can draw it!
	p_dc->drawimage(t_descriptor, 0, 0, t_raster.width, t_raster.height, 0, 0);
	MCGImageRelease(t_gimage);

	// Clean up the drawing resources that we allocated
	GlobalUnlock(t_dib);
	GlobalFree(t_dib);
	DeleteDC(t_hdc);
	ReleaseDC(m_hwnd, t_hwindowdc);
}

void MCNativeLayerWin32::OnGeometryChanged(const MCRectangle& p_old_rect)
{
	// Move the window. Only trigger a repaint if not in edit mode
	MCRectangle t_rect;
	t_rect = m_widget->getrect();
	MoveWindow(m_hwnd, t_rect.x, t_rect.y, t_rect.width, t_rect.height, !m_widget->inEditMode());

	// We need to delete the bitmap that we've been caching
	DeleteObject(m_cached);
	m_cached = NULL;
}

void MCNativeLayerWin32::OnVisibilityChanged(bool p_visible)
{
	ShowWindow(m_hwnd, p_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
}

void MCNativeLayerWin32::OnLayerChanged()
{
	doRelayer();
}

void MCNativeLayerWin32::doRelayer()
{
	// Find which native layer this should be inserted after
	MCWidget* t_before;
	t_before = findNextLayerBelow(m_widget);

	// Insert the widget in the correct place (but only if the card is current)
	if (isAttached() && m_widget->getstack()->getcard() == m_widget->getstack()->getcurcard())
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
	return (HWND)m_widget->getstack()->getrealwindow();
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCWidget::createNativeLayer()
{
	return new MCNativeLayerWin32(this);
}
