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

#include "widget-native-win32.h"


MCNativeWidgetWin32::MCNativeWidgetWin32() :
  m_hwnd(NULL),
  m_cached(NULL)
{

}

MCNativeWidgetWin32::MCNativeWidgetWin32(const MCNativeWidgetWin32& p_clone) :
  MCWidget(p_clone),
  m_hwnd(NULL),
  m_cached(NULL)
{
	m_hwnd = CreateWindow
	(
		L"BUTTON",
		L"Native Button",
		WS_TABSTOP|WS_CHILD|BS_DEFPUSHBUTTON,
		rect.x,
		rect.y,
		rect.width,
		rect.height,
		getStackWindow(),
		NULL,
		(HINSTANCE)GetWindowLong(getStackWindow(), GWL_HINSTANCE),
		NULL
	);

	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS)-4;
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS)-4, &ncm, 0);
	HFONT font = CreateFontIndirect(&ncm.lfMessageFont);
	SendMessage(m_hwnd, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
}

MCNativeWidgetWin32::~MCNativeWidgetWin32()
{
	if (m_hwnd != NULL)
		DestroyWindow(m_hwnd);
	if (m_cached != NULL)
		DeleteObject(m_cached);
}

MCWidget* MCNativeWidgetWin32::clone(Boolean p_attach, Object_pos p_position, bool invisible)
{
	MCWidget *t_new_widget;
	t_new_widget = new MCNativeWidgetWin32(*this);
	if (p_attach)
		t_new_widget -> attach(p_position, invisible);
	return t_new_widget;
}

void MCNativeWidgetWin32::toolchanged(Tool p_new_tool)
{
	if (p_new_tool == T_BROWSE || p_new_tool == T_HELP)
	{
		// In run mode. Make visible if requested and our card is current.
		if ((flags & F_VISIBLE) && getcard() == getstack()->getcurcard())
			ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
		Redraw();
	}
	else
	{
		// In edit mode
		ShowWindow(m_hwnd, SW_HIDE);
		Redraw();
	}
}

bool MCNativeWidgetWin32::isNative() const
{
	return true;
}

void MCNativeWidgetWin32::nativeOpen()
{
	// Unhide the widget, if required
	if (flags & F_VISIBLE && !inEditMode())
		ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
}

void MCNativeWidgetWin32::nativeClose()
{
	ShowWindow(m_hwnd, SW_HIDE);
}

void MCNativeWidgetWin32::nativePaint(MCDC* p_dc, const MCRectangle& p_dirty)
{
	// If the widget is not in edit mode, we trust it to paint itself
	if (!inEditMode())
		return;

	// Create a DC to use for the drawing operation. We create a new DC
	// compatible to the one that would normally be used for a paint operation.
	HDC t_hdc, t_hwindowdc;
	t_hwindowdc = GetDC(NULL);
	t_hdc = CreateCompatibleDC(t_hwindowdc);

	// Create a bitmap for the DC to draw into
	if (m_cached == NULL)
	{
		// Note: this *must* be the original DC because the compatible DC originally
		// has a monochrome (1BPP) bitmap selected into it and we don't want that.
		m_cached = CreateCompatibleBitmap(t_hwindowdc, rect.width, rect.height);
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

void MCNativeWidgetWin32::nativeGeometryChanged(const MCRectangle& p_old_rect)
{
	// Move the window. Only trigger a repaint if not in edit mode
	MoveWindow(m_hwnd, rect.x, rect.y, rect.width, rect.height, !inEditMode());

	// We need to delete the bitmap that we've been caching
	DeleteObject(m_cached);
	m_cached = NULL;
}

void MCNativeWidgetWin32::nativeVisibilityChanged(bool p_visible)
{
	ShowWindow(m_hwnd, p_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
}

HWND MCNativeWidgetWin32::getStackWindow()
{
	return (HWND)getstack()->getrealwindow();
}
