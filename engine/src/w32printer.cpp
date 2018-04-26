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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "globals.h"
#include "metacontext.h"
#include "printer.h"
#include "stacklst.h"
#include "mode.h"
#include "region.h"

#include "w32dc.h"
#include "w32printer.h"

#include "graphics.h"
#include "graphicscontext.h"

#include "graphics_util.h"

#include "font.h"

///////////////////////////////////////////////////////////////////////////////

extern void Windows_RenderMetaFile(HDC p_color_dc, HDC p_mask_dc, uint1 *p_data, uint4 p_length, const MCRectangle& p_dst_rect);

extern void MCRemotePrintSetupDialog(MCDataRef p_config_data, MCDataRef &r_reply_data, uint32_t &r_result);
extern void MCRemotePageSetupDialog(MCDataRef p_config_data, MCDataRef &r_reply_data, uint32_t &r_result);

extern bool serialize_bytes(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, const void *p_data, uint32_t p_data_size);
extern bool deserialize_bytes(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, void *p_dest, uint32_t p_size);
extern bool serialize_uint32(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, uint32_t p_val);
extern bool deserialize_uint32(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, uint32_t &r_val);
extern bool serialize_data(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, const void *p_data, uint32_t p_data_size);
extern bool deserialize_data(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, void *&r_data, uint32_t &r_size);

extern bool deserialize_data_to_hglobal(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, HGLOBAL &r_hglobal);
// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
extern bool serialize_printdlg_data(char *&r_buffer, uint32_t &r_size, PRINTDLGEXW &p_data);
extern bool deserialize_printdlg_data(const char *p_buffer, uint32_t p_size, PRINTDLGEXW *&x_data);
extern bool serialize_pagedlg_data(char *&r_buffer, uint32_t &r_size, PAGESETUPDLGW &p_data);
extern bool deserialize_pagedlg_data(const char *p_buffer, uint32_t p_size, PAGESETUPDLGW *&x_data);

extern bool MCGImageSplitHBITMAPWithMask(HDC p_dc, MCGImageRef p_image, HBITMAP &r_bitmap, HBITMAP &r_mask);
extern bool create_temporary_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);

///////////////////////////////////////////////////////////////////////////////

inline XFORM MCGAffineTransformToXFORM(const MCGAffineTransform &p_transform)
{
	XFORM t_xform;
	t_xform.eM11 = p_transform.a;
	t_xform.eM12 = p_transform.b;
	t_xform.eM21 = p_transform.c;
	t_xform.eM22 = p_transform.d;
	t_xform.eDx = p_transform.tx;
	t_xform.eDy = p_transform.ty;

	return t_xform;
}

///////////////////////////////////////////////////////////////////////////////

inline int32_t TenthMillimetresToPoints(int32_t p_value)
{
	return (p_value * 72 + 127) / 254;
}

inline int32_t PointsToTenthMillimetres(int32_t p_value)
{
	return (p_value * 254 + 36) / 72;
}

inline int32_t ThousandthsInchToPoints(int32_t p_value)
{
	return (p_value * 72 + 500) / 1000;
}

inline int32_t PointsToThousandthsInch(int32_t p_value)
{
	return (p_value * 1000 + 36) / 72;
}

///////////////////////////////////////////////////////////////////////////////

static void Win32GetDefaultPrinter(MCStringRef &r_printer)
{
	DWORD t_size;
	t_size = 0;
	GetDefaultPrinterW(NULL, &t_size);

	MCAutoArray<unichar_t> t_buffer;
	/* UNCHECKED */ t_buffer.New(t_size);

	if (GetDefaultPrinterW(t_buffer.Ptr(), &t_size) == 0)
		r_printer = MCValueRetain(kMCEmptyString);
	else
		/* UNCHECKED */ MCStringCreateWithChars(t_buffer.Ptr(), t_size, r_printer);
}

static void Win32GetProfileString(const wchar_t *p_app, const wchar_t *p_key, const wchar_t *p_default, MCStringRef &r_string)
{
	MCAutoArray<unichar_t> t_buffer;
	/* UNCHECKED */ t_buffer.New(256);

	DWORD t_size;
	t_size = GetProfileStringW(p_app, p_key, p_default, t_buffer.Ptr(), t_buffer.Size());
	
	/* UNCHECKED */ MCStringCreateWithChars(t_buffer.Ptr(), t_size, r_string);
}

static HANDLE Win32OpenPrinter(MCStringRef p_name)
{
	HANDLE t_printer;
	PRINTER_DEFAULTSW t_defaults;
	t_defaults . pDatatype = NULL;
	t_defaults . pDevMode = NULL;
	t_defaults . DesiredAccess = PRINTER_ACCESS_USE;

	MCAutoStringRefAsLPWSTR t_name_wstr;
	/* UNCHECKED */ t_name_wstr.Lock(p_name);

	if (OpenPrinterW(*t_name_wstr, &t_printer, &t_defaults) == 0)
		return NULL;

	return t_printer;
}

static void WindowsGetDefaultPrinter(MCStringRef &r_printer)
{
	if (MCmajorosversion >= 0x0500)
		Win32GetDefaultPrinter(r_printer);
	else
	{
		MCAutoStringRef t_string;
		Win32GetProfileString(L"windows", L"device", L",,,", &t_string);

		if (!MCStringIsEmpty(*t_string))
		{
			uindex_t t_comma;
			if (MCStringFirstIndexOfChar(*t_string, ',', 0, kMCStringOptionCompareExact, t_comma))
				/* UNCHECKED */ MCStringCopySubstring(*t_string, MCRangeMake(0, t_comma), r_printer);
			else
				r_printer = MCValueRetain(kMCEmptyString);
		}
	}
}

static DEVMODEW *WindowsGetPrinterInfo(MCStringRef p_printer_name, DEVMODEW *p_indevmode = NULL)
{
	bool t_success;
	t_success = true;

	HANDLE t_printer;
	t_printer = NULL;
	if (t_success)
	{
		t_printer = Win32OpenPrinter(p_printer_name);
		if (t_printer == NULL)
			t_success = false;
	}

	LONG t_devmode_size;
	if (t_success)
	{
		t_devmode_size = DocumentPropertiesW(NULL, t_printer, NULL, NULL, NULL, 0);
		if (t_devmode_size < 0)
			t_success = false;
	}

	// SN-2015-03-05: [[ Bug 14160 ]] Ensure that the value returned
	//  has been initialised (otherwise, we are at the risk a non-nil
	//  value is returned, and MCWindowsPrinter::DoInitialise will not
	//  return false.
	DEVMODEW *t_devmode;
	t_devmode = NULL;
	if (t_success)
	{
		// NOTE: the memory required for the DEVMODE buffer can be bigger than
		// the DEVMODE structure as the printer driver may append extra data
		t_devmode = (DEVMODEW*)malloc(t_devmode_size);
		if (DocumentPropertiesW(NULL, t_printer, NULL, t_devmode, p_indevmode, (p_indevmode != NULL ? DM_IN_BUFFER : 0) | DM_OUT_BUFFER) < 0)
			t_success = false;
		else
		{
			if ((t_devmode -> dmFields & DM_PAPERSIZE) == 0)
			{
				t_devmode -> dmFields |= DM_PAPERSIZE;
				t_devmode -> dmPaperSize = 0;
			}
		}
	}

	if (t_printer != NULL)
		ClosePrinter(t_printer);

	return t_devmode;
}

bool WindowsGetPrinterPaperSize(MCStringRef p_name, DEVMODEW *p_devmode, POINT& r_point)
{
	MCAutoStringRefAsWString t_name_wstr;
	/* UNCHECKED */ t_name_wstr.Lock(p_name);
	
	DWORD t_papers_size;
	t_papers_size = DeviceCapabilitiesW(*t_name_wstr, L"", DC_PAPERS, NULL, p_devmode);

	DWORD t_papersizes_size;
	t_papersizes_size = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_PAPERNAMES, NULL, p_devmode);

	if (t_papers_size < 0 || t_papersizes_size < 0)
		return false;

	WORD *t_papers;
	t_papers = new (nothrow) WORD[t_papers_size];

	POINT *t_papersizes;
	t_papersizes = new (nothrow) POINT[t_papersizes_size];

	DWORD t_papers_count;
	t_papers_count = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_PAPERS, (LPWSTR)t_papers, p_devmode);

	DWORD t_papersizes_count;
	t_papersizes_count = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_PAPERSIZE, (LPWSTR)t_papersizes, p_devmode);

	bool t_success;
	t_success = false;
	if (t_papers_count >= 0 && t_papersizes_count >= 0)
	{
		for(unsigned int i = 0; i < t_papers_count; ++i)
			if (p_devmode -> dmPaperSize == t_papers[i])
			{
				r_point = t_papersizes[i];
				t_success = true;
				break;
			}
	}

	delete t_papers;
	delete t_papersizes;

	return t_success;
}

bool WindowsGetPrinterPaperIndex(MCStringRef p_name, DEVMODEW *p_devmode, uint32_t p_width_pts, uint32_t p_height_pts, uint32_t& r_index)
{
	MCAutoStringRefAsWString t_name_wstr;
	/* UNCHECKED */ t_name_wstr.Lock(p_name);
	
	DWORD t_papers_size;
	t_papers_size = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_PAPERS, NULL, p_devmode);

	DWORD t_papersizes_size;
	t_papersizes_size = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_PAPERNAMES, NULL, p_devmode);

	if (t_papers_size < 0 || t_papersizes_size < 0)
		return false;

	WORD *t_papers;
	t_papers = new (nothrow) WORD[t_papers_size];

	POINT *t_papersizes;
	t_papersizes = new (nothrow) POINT[t_papersizes_size];

	DWORD t_papers_count;
	t_papers_count = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_PAPERS, (LPWSTR)t_papers, p_devmode);

	DWORD t_papersizes_count;
	t_papersizes_count = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_PAPERSIZE, (LPWSTR)t_papersizes, p_devmode);

	bool t_success;
	t_success = false;
	if (t_papers_count >= 0 && t_papersizes_count >= 0)
	{
		for(unsigned int i = 0; i < t_papers_count; ++i)
			if (TenthMillimetresToPoints(t_papersizes[i] . x) == p_width_pts && TenthMillimetresToPoints(t_papersizes[i] . y) == p_height_pts)
			{
				r_index = t_papers[i];
				t_success = true;
				break;
			}
	}

	delete t_papers;
	delete t_papersizes;

	return t_success;
}

MCPrinterFeatureSet WindowsGetPrinterFeatures(MCStringRef p_name, DEVMODEW *p_devmode)
{
	MCAutoStringRefAsWString t_name_wstr;
	/* UNCHECKED */ t_name_wstr.Lock(p_name);
	
	MCPrinterFeatureSet t_features;
	t_features = 0;

	DWORD t_copies;
	t_copies = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_COPIES, NULL, p_devmode);
	if (t_copies > 1)
		t_features |= PRINTER_FEATURE_COPIES;

	DWORD t_collate;
	t_collate = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_COLLATE, NULL, p_devmode);
	if (t_collate != 0)
		t_features |= PRINTER_FEATURE_COLLATE;

	DWORD t_duplex;
	t_duplex = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_DUPLEX, NULL, p_devmode);
	if (t_duplex != 0)
		t_features |= PRINTER_FEATURE_DUPLEX;

	DWORD t_color;
	t_color = DeviceCapabilitiesW(*t_name_wstr, NULL, DC_COLORDEVICE, NULL, p_devmode);
	if (t_color != 0)
		t_features |= PRINTER_FEATURE_COLOR;

	return t_features;
}

MCRectangle WindowsGetPrinterRectangle(MCStringRef p_name, DEVMODEW *p_devmode)
{
	MCAutoStringRefAsWString t_name_wstr;
	/* UNCHECKED */ t_name_wstr.Lock(p_name);
	
	MCRectangle t_rect;

	// MW-2008-03-26: [[ Bug 5936 ]] Make sure the IC is actually created and
	//   that we don't attempt to divide by 0 if the resolution is returned as
	//   0.
	HDC t_dc;
	t_dc = CreateICW(p_devmode -> dmDeviceName, *t_name_wstr, NULL, p_devmode);
	if (t_dc != NULL)
	{
		int t_physical_width, t_physical_height;
		t_physical_width = GetDeviceCaps(t_dc, PHYSICALWIDTH);
		t_physical_height = GetDeviceCaps(t_dc, PHYSICALHEIGHT);

		int t_offset_x, t_offset_y;
		t_offset_x = GetDeviceCaps(t_dc, PHYSICALOFFSETX);
		t_offset_y = GetDeviceCaps(t_dc, PHYSICALOFFSETY);

		int t_width, t_height;
		t_width = GetDeviceCaps(t_dc, HORZRES);
		t_height = GetDeviceCaps(t_dc, VERTRES);

		int t_scale;
		if (p_devmode != NULL)
			t_scale = p_devmode -> dmScale;
		else
			t_scale = 100;

		if (t_scale == 0)
			t_scale = 100;

		int32_t t_dpi_x, t_dpi_y;
		t_dpi_x = GetDeviceCaps(t_dc, LOGPIXELSX) * t_scale;
		t_dpi_y = GetDeviceCaps(t_dc, LOGPIXELSY) * t_scale;

		DeleteDC(t_dc);

		if (t_dpi_x != 0 && t_dpi_y != 0)
			MCU_set_rect(t_rect, t_offset_x * 72 * 100 / t_dpi_x, t_offset_y * 72 * 100 / t_dpi_y, t_width * 72 * 100 / t_dpi_x, t_height * 72 * 100 / t_dpi_y);
		else
			MCU_set_rect(t_rect, 0, 0, 0, 0);
	}
	else
		MCU_set_rect(t_rect, 0, 0, 0, 0);

	return t_rect;
}

// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
HRESULT WindowsPrintDlgEx(PRINTDLGEXW *p_dlg)
{
	return PrintDlgExW(p_dlg);
}

///////////////////////////////////////////////////////////////////////////////

class MCGDIMetaContext: public MCMetaContext
{
public:
	MCGDIMetaContext(const MCRectangle& p_page);
	~MCGDIMetaContext(void);

	void render(HDC p_dc, const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect);

protected:
	bool candomark(MCMark *mark);
	void domark(MCMark *mark);

	bool begincomposite(const MCRectangle& p_region, MCGContextRef &r_context);
	void endcomposite(MCRegionRef p_clip_region);

private:
	HDC m_dc;
	MCRectangle m_composite_rect;
	HBITMAP m_composite_bitmap;
	MCGContextRef m_composite_context;
};

MCGDIMetaContext::MCGDIMetaContext(const MCRectangle& p_page)
	: MCMetaContext(p_page)
{
}

MCGDIMetaContext::~MCGDIMetaContext(void)
{
}

void MCGDIMetaContext::render(HDC p_dc, const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect)
{
	m_dc = p_dc;

	int32_t t_dpi_x, t_dpi_y;
	t_dpi_x = GetDeviceCaps(m_dc, LOGPIXELSX);
	t_dpi_y = GetDeviceCaps(m_dc, LOGPIXELSY);

	int32_t t_origin_x, t_origin_y;
	t_origin_x = GetDeviceCaps(m_dc, PHYSICALOFFSETX);
	t_origin_y = GetDeviceCaps(m_dc, PHYSICALOFFSETY);

	int32_t t_width, t_height;
	t_width = GetDeviceCaps(m_dc, PHYSICALWIDTH);
	t_height = GetDeviceCaps(m_dc, PHYSICALHEIGHT);

	int32_t t_device_left, t_device_top, t_device_right, t_device_bottom;
	t_device_left = (int32_t)floor(p_dst_rect . left * t_dpi_x / 72.0) - t_origin_x;
	t_device_top = (int32_t)floor(p_dst_rect . top * t_dpi_y / 72.0) - t_origin_y;
	t_device_right = (int32_t)floor(p_dst_rect . right * t_dpi_x / 72.0) - t_origin_x;
	t_device_bottom = (int32_t)floor(p_dst_rect . bottom * t_dpi_y / 72.0) - t_origin_y;

	int32_t t_logical_left, t_logical_top, t_logical_right, t_logical_bottom;
	t_logical_left = (int32_t)floor(p_src_rect . left);
	t_logical_top = (int32_t)floor(p_src_rect . top);
	t_logical_right = (int32_t)floor(p_src_rect . right);
	t_logical_bottom = (int32_t)floor(p_src_rect . bottom);

	SetMapMode(m_dc, MM_ANISOTROPIC);
	SetTextAlign(m_dc, TA_LEFT | TA_BASELINE);
	SetWindowOrgEx(m_dc, t_logical_left, t_logical_top, NULL);
	SetWindowExtEx(m_dc, t_logical_right - t_logical_left, t_logical_bottom - t_logical_top, NULL);
	SetViewportOrgEx(m_dc, t_device_left, t_device_top, NULL);
	SetViewportExtEx(m_dc, t_device_right - t_device_left, t_device_bottom - t_device_top, NULL);

	execute();
}

static inline DWORD colour_to_pixel(const MCColor& p_colour)
{
	return RGB(p_colour . red >> 8, p_colour . green >> 8, p_colour . blue >> 8);
}

extern void gdi_do_arc(HDC p_dc, HDC p_mask_dc, bool p_fill, int4 p_left, int4 p_top, int4 p_right, int4 p_bottom, int4 p_start, int4 p_end);

static void resolve_dashes(int2 p_offset, uint1* p_data, uint4 p_length, DWORD*& r_out_data, uint4& r_out_length)
{
	bool t_on;
	t_on = true;

	uint2 t_start;
	t_start = 0;
	
	r_out_data = new (nothrow) DWORD[p_length + 2];

	while(p_offset >= p_data[t_start])
	{
		p_offset -= p_data[t_start++];
		t_start %= p_length;
		t_on = !t_on;
	}

	uint2 t_current;
	t_current = 0;

	r_out_length = p_length;

	if (!t_on)
	{
		r_out_data[t_current++] = 0;
		r_out_length += 1;
	}

	r_out_data[t_current++] = p_data[t_start++] - p_offset;

	for(uint4 t_index = 1; t_index < p_length; ++t_index)
	{
		r_out_data[t_current++] = p_data[t_start++];
		t_start %= p_length;
	}

	if (p_offset != 0)
	{
		r_out_data[t_current++] = p_offset;
		r_out_length++;
	}
}

bool MCGDIMetaContext::candomark(MCMark *mark)
{
	// printing transformed images does not seem to work reliably
	if (mark -> type == MARK_TYPE_IMAGE && mark -> image . descriptor . has_transform)
	{
		// we can handle scaled images through StretchBlt, only return false here
		// if the transform includes rotation
		MCGAffineTransform t_transform = mark -> image . descriptor . transform;
		if (t_transform.b != 0 || t_transform.c != 0)
			return false;
	}
	// Group marks are only generated precisely when the (most feable)
	// system context cannot render it.
	return mark -> type != MARK_TYPE_GROUP;
}

void MCGDIMetaContext::domark(MCMark *p_mark)
{
	HDC t_dc;
	HGDIOBJ t_old_pen = NULL;
	HGDIOBJ t_old_brush = NULL;
	
	t_dc = m_dc;

	RECT t_clip;
	t_clip . left = p_mark -> clip . x;
	t_clip . top = p_mark -> clip . y;
	t_clip . right = p_mark -> clip . x + p_mark -> clip . width;
	t_clip . bottom = p_mark -> clip . y + p_mark -> clip . height;
	LPtoDP(m_dc, (POINT *)&t_clip, 2);
	HRGN t_clip_rgn;
	t_clip_rgn = CreateRectRgnIndirect(&t_clip);
	SelectClipRgn(m_dc, t_clip_rgn);
	DeleteObject(t_clip_rgn);

	bool t_is_geometric_mark;
	t_is_geometric_mark = (p_mark -> type != MARK_TYPE_TEXT && p_mark -> type != MARK_TYPE_IMAGE && p_mark -> type != MARK_TYPE_IMAGE);

	bool t_is_pattern_fill, t_is_stroke;
	t_is_pattern_fill = false;
	t_is_stroke = false;
	
    // MM-2014-04-23: [[ Bug 11884 ]] If possible, we should let GDI handle insetting, using the PS_INSIDEFRAME pen type.
    //  This only works for stroking solid lines, so for fills and dashed strokes, inset ourselves.
	bool t_should_inset;
	t_should_inset = false;
	
	if (p_mark -> stroke != NULL)
	{
		uint4 t_style;
		uint4 t_width;
		LOGBRUSH t_brush;

		if (p_mark -> stroke -> width == 0)
		{
			t_style = PS_JOIN_MITER | PS_ENDCAP_FLAT | PS_GEOMETRIC | PS_INSIDEFRAME;
			t_width = 1;
		}
		else
		{
			t_style = PS_GEOMETRIC;
			
			// MW-2012-02-28: [[ Bug 9997 ]] Map the internal join style to GDI flags. Previously
			//   this wasn't being done causing incorrect line styles when printing.
			switch(p_mark -> stroke -> join)
			{
			case JoinRound:
				t_style |= PS_JOIN_ROUND;
				break;
			case JoinMiter:
				t_style |= PS_JOIN_MITER;
				break;
			case JoinBevel:
				t_style |= PS_JOIN_BEVEL;
				break;
			}

			switch(p_mark -> stroke -> cap)
			{
			case CapButt:
				t_style |= PS_ENDCAP_FLAT;
				break;
			case CapRound:
				t_style |= PS_ENDCAP_ROUND;
				break;
			case CapProjecting:
				t_style |= PS_ENDCAP_SQUARE;
				break;
			}

			t_width = p_mark -> stroke -> width;

            // MM-2014-04-23: [[ Bug 11884 ]] If we want to inset this mark, use the PS_INSIDEFRAME pen type.
			uint2 t_inset;
			switch(p_mark -> type)
			{
				case MARK_TYPE_RECTANGLE:
					t_inset = p_mark -> rectangle . inset;
					break;
				case MARK_TYPE_ROUND_RECTANGLE:
					t_inset = p_mark -> round_rectangle . inset;
					break;
				case MARK_TYPE_ARC:
					t_inset = p_mark -> arc . inset;
					break;
				default:
					t_inset = 0;
					break;
			}
			if (t_width < 2 || t_inset > 0)
				t_style |= PS_INSIDEFRAME;
		}

		if (p_mark -> fill -> style == FillSolid || !t_is_geometric_mark)
		{
			t_brush . lbStyle = BS_SOLID;
			t_brush . lbHatch = 0;
		}
		else if (p_mark -> fill -> style == FillTiled)
		{
			t_is_pattern_fill = true;
			t_is_stroke = true;

			t_brush . lbStyle = BS_SOLID;
			t_brush . lbHatch = 0;
		}
		else
		{
			static HBITMAP s_stipple = NULL;
			static uint4 s_stipple_data[8] =
			{
			   0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
			   0x55555555, 0xAAAAAAAA, 0x55555555,
			   0xAAAAAAAA, 0x55555555
			};
			if (s_stipple == NULL)
				s_stipple = CreateBitmap(32, 8, 1, 1, s_stipple_data);

			t_brush . lbStyle = BS_PATTERN;
			t_brush . lbHatch = (LONG)s_stipple;
		}

		DWORD *t_dashes;
		t_dashes = NULL;

		uint4 t_dash_length;
		t_dash_length = 0;

		if (p_mark -> stroke -> dash . length != 0)
		{
			t_style |= PS_USERSTYLE;
			
			resolve_dashes(p_mark -> stroke -> dash . offset, p_mark -> stroke -> dash . data, p_mark -> stroke -> dash . length, t_dashes, t_dash_length);

            // MM-2014-04-23: [[ Bug 11884 ]] For dashed lines, we can't use the PS_INSIDEFRAME, so we must inset ourselves.
			t_should_inset = true;
		}

		t_brush . lbColor = colour_to_pixel(p_mark -> fill -> colour);
		t_old_pen = SelectObject(t_dc, ExtCreatePen(t_style, t_width, &t_brush, t_dash_length, t_dashes));
		SelectObject(t_dc, GetStockObject(NULL_BRUSH));

		if (t_dashes != NULL)
			delete t_dashes;
	}
	else if (p_mark -> fill != NULL)
	{
		LOGBRUSH t_brush;
		if (p_mark -> fill -> style == FillSolid || !t_is_geometric_mark)
		{
			t_brush . lbStyle = BS_SOLID;
			t_brush . lbHatch = 0;
		}
		else if (p_mark -> fill -> style == FillTiled)
		{
			t_is_pattern_fill = true;

			t_brush . lbStyle = BS_SOLID;
			t_brush . lbHatch = 0;
		}
		else
		{
			static HBITMAP s_stipple = NULL;
			static uint4 s_stipple_data[8] =
			{
			   0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
			   0x55555555, 0xAAAAAAAA, 0x55555555,
			   0xAAAAAAAA, 0x55555555
			};
			if (s_stipple == NULL)
				s_stipple = CreateBitmap(32, 8, 1, 1, s_stipple_data);

			t_brush . lbStyle = BS_PATTERN;
			t_brush . lbHatch = (LONG)s_stipple;

			if (p_mark -> fill -> style == FillStippled)
			{
				SetTextColor(t_dc, colour_to_pixel(p_mark -> fill -> colour));
				SetBkColor(t_dc, 0x808080);
			}
			else if (p_mark -> fill -> style == FillOpaqueStippled)
			{
				SetTextColor(t_dc, colour_to_pixel(p_mark -> fill -> colour));
				SetBkColor(t_dc, 0x000000);
			}
		}

		t_brush . lbColor = colour_to_pixel(p_mark -> fill -> colour);
		t_old_brush = SelectObject(t_dc, CreateBrushIndirect(&t_brush));

		POINT t_origin;
		t_origin.x = p_mark->fill->origin.x;
		t_origin.y = p_mark->fill->origin.y;
		LPtoDP(t_dc, &t_origin, 1);

		SetBrushOrgEx(t_dc, t_origin.x, t_origin.y, NULL);
		SelectObject(t_dc, GetStockObject(NULL_PEN));
		
        // MM-2014-04-23: [[ Bug 11884 ]] For fills, we can't use the PS_INSIDEFRAME, so we must inset ourselves.
		t_should_inset = true;
	}
	else
	{
		SelectObject(t_dc, GetStockObject(NULL_PEN));
		SelectObject(t_dc, GetStockObject(NULL_BRUSH));
		
        // MM-2014-04-23: [[ Bug 11884 ]] For fills, we can't use the PS_INSIDEFRAME, so we must inset ourselves.
		t_should_inset = true;
	}

	if (t_is_pattern_fill)
		BeginPath(t_dc);

	switch(p_mark -> type)
	{
		case MARK_TYPE_LINE:
			MoveToEx(t_dc, p_mark -> line . start . x, p_mark -> line . start . y, NULL);
			LineTo(t_dc, p_mark -> line . end . x, p_mark -> line . end . y);
		break;

		case MARK_TYPE_POLYGON:
		{
			POINT *t_points;
			t_points = new (nothrow) POINT[p_mark -> polygon . count];
			if (t_points == NULL)
				break;

			for(uint4 t_vertex = 0; t_vertex < p_mark -> polygon . count; ++t_vertex)
			{
				t_points[t_vertex] . x = p_mark -> polygon . vertices[t_vertex] . x;
				t_points[t_vertex] . y = p_mark -> polygon . vertices[t_vertex] . y;
			}
					
			if (p_mark -> stroke != NULL)
				Polyline(t_dc, t_points, p_mark -> polygon . count);
			else
				Polygon(t_dc, t_points, p_mark -> polygon . count);

			delete t_points;
		}
		break;

		case MARK_TYPE_TEXT:
		{
			MCFontStruct *f;
			f = MCFontGetFontStruct(p_mark->text.font);

			// MW-2013-11-07: [[ Bug 11393 ]] If using 'ideal' metrics, then make the font
			//   at -size (using the screen font causes size / placement issues).
			MCSysFontHandle t_new_font;
			if (f -> printer)
			{
				LOGFONTA t_font;
				GetObjectA(f -> fid, sizeof(LOGFONTA), &t_font);
				t_font . lfHeight = -f -> size;

				t_new_font = (MCSysFontHandle)CreateFontIndirectA(&t_font);
			}
			else
				t_new_font = f -> fid;

			HGDIOBJ t_old_font;
			t_old_font = SelectObject(t_dc, t_new_font);
			SetTextColor(t_dc, colour_to_pixel(p_mark -> fill -> colour));
			if (p_mark -> text . background != NULL)
			{
				SetBkColor(t_dc, colour_to_pixel(p_mark -> text . background -> colour));
				SetBkMode(t_dc, OPAQUE);
			}
			else
				SetBkMode(t_dc, TRANSPARENT);
			if (p_mark -> text . unicode_override)
				// SN-2014-10-31: [[ Bug 13866 ]] The length is in characters, so fits the number of wchars
				TextOutW(t_dc, p_mark -> text . position . x, p_mark -> text . position . y, (LPCWSTR)p_mark -> text . data, p_mark -> text . length);
			else
				TextOutA(t_dc, p_mark -> text . position . x, p_mark -> text . position . y, (LPCSTR)p_mark -> text . data, p_mark -> text . length);

			SelectObject(t_dc, t_old_font);

			// MW-2013-11-07: [[ Bug 11393 ]] Delete the font if we had to create a new one.
			if (t_new_font != f -> fid)
				DeleteObject(t_new_font);
		}
		break;

		case MARK_TYPE_RECTANGLE:
		{
			if (t_should_inset)
			{
                // MM-2014-04-23: [[ Bug 11884 ]] Inset the bounds. Since GDI only accepts ints, if the inset value is uneven,
                // round up to the nearest even value, keeping behaviour as close to that of the graphics context as possible.
				if (!(p_mark -> rectangle . inset % 2))
					p_mark -> rectangle . inset ++;
				p_mark -> rectangle . bounds = MCRectangleMake(p_mark -> rectangle . bounds . x + p_mark -> rectangle . inset / 2,
															   p_mark -> rectangle . bounds . y + p_mark -> rectangle . inset / 2, 
															   p_mark -> rectangle . bounds . width - p_mark -> rectangle . inset, 
															   p_mark -> rectangle . bounds . height - p_mark -> rectangle . inset);
			}
			
			Rectangle(t_dc, p_mark -> rectangle . bounds . x, p_mark -> rectangle . bounds . y,
											p_mark -> rectangle . bounds . x + p_mark -> rectangle . bounds . width,
											p_mark -> rectangle . bounds . y + p_mark -> rectangle . bounds . height);
		}
		break;

		case MARK_TYPE_ROUND_RECTANGLE:
		{
			if (t_should_inset)
			{
                // MM-2014-04-23: [[ Bug 11884 ]] Inset the bounds. Since GDI only accepts ints, if the inset value is uneven,
                // round up to the nearest even value, keeping behaviour as close to that of the graphics context as possible.
				if (!(p_mark -> round_rectangle . inset % 2))
					p_mark -> round_rectangle . inset ++;
				p_mark -> round_rectangle . bounds = MCRectangleMake(p_mark -> round_rectangle . bounds . x + p_mark -> round_rectangle . inset / 2,
																	 p_mark -> round_rectangle . bounds . y + p_mark -> round_rectangle . inset / 2, 
																	 p_mark -> round_rectangle . bounds . width - p_mark -> round_rectangle . inset, 
																	 p_mark -> round_rectangle . bounds . height - p_mark -> round_rectangle . inset);
			}
			
			int4 t_adjust;
			t_adjust = p_mark -> stroke != NULL ? 0 : 1;
			RoundRect(t_dc, p_mark -> round_rectangle . bounds . x, p_mark -> round_rectangle . bounds . y,
											p_mark -> round_rectangle . bounds . x + p_mark -> round_rectangle . bounds . width,
											p_mark -> round_rectangle . bounds . y + p_mark -> round_rectangle . bounds . height, p_mark -> round_rectangle . radius,
											p_mark -> round_rectangle . radius);
		}
		break;

		case MARK_TYPE_ARC:
			if (t_should_inset)
			{
                // MM-2014-04-23: [[ Bug 11884 ]] Inset the bounds. Since GDI only accepts ints, if the inset value is uneven,
                // round up to the nearest even value, keeping behaviour as close to that of the graphics context as possible.
				if (!(p_mark -> arc . inset % 2))
					p_mark -> arc . inset ++;
				p_mark -> arc . bounds = MCRectangleMake(p_mark -> arc . bounds . x + p_mark -> arc . inset / 2,
														 p_mark -> arc . bounds . y + p_mark -> arc . inset / 2, 
														 p_mark -> arc . bounds . width - p_mark -> arc . inset, 
														 p_mark -> arc . bounds . height - p_mark -> arc . inset);				
			}
			
			gdi_do_arc(t_dc, NULL, p_mark -> stroke == NULL, p_mark -> arc . bounds . x, p_mark -> arc . bounds . y, p_mark -> arc . bounds . x + p_mark -> arc . bounds . width, p_mark -> arc . bounds . y + p_mark -> arc . bounds . height, p_mark -> arc . start, p_mark -> arc . start + p_mark -> arc . angle);
			if (p_mark -> stroke != NULL)
			{
				if (p_mark -> arc . complete)
				{
					int2 cx = p_mark -> arc . bounds . x + (p_mark -> arc . bounds . width >> 1);
					int2 cy = p_mark -> arc . bounds . y + (p_mark -> arc . bounds . height >> 1);
					real8 torad = M_PI * 2.0 / 360.0;
					real8 tw = (real8)p_mark -> arc . bounds . width;
					real8 th = (real8)p_mark -> arc . bounds . height;
					real8 sa = (real8)p_mark -> arc . start * torad;
					
					int2 dx = cx + (int2)(cos(sa) * tw / 2.0);
					int2 dy = cy - (int2)(sin(sa) * th / 2.0);
					MoveToEx(t_dc, dx, dy, NULL);
					LineTo(t_dc, cx, cy);

					sa = (real8)(p_mark -> arc . start + p_mark -> arc . angle) * torad;
					dx = cx + (int2)(cos(sa) * tw / 2.0);
					dy = cy - (int2)(sin(sa) * th / 2.0);
					LineTo(t_dc, dx, dy);
				}
			}
		break;

		case MARK_TYPE_IMAGE:
		{
			HDC t_src_dc;
			t_src_dc = ((MCScreenDC *)MCscreen) -> getsrchdc();
			
			// Work out source/dst sizes and decompressed bitmap
			HBITMAP t_src_bitmap = nil;
			HBITMAP t_src_mask = nil;
			uint32_t t_src_width, t_src_height, t_dst_width, t_dst_height;

			int32_t t_src_x = p_mark->image.dx - p_mark->image.sx;
			int32_t t_src_y = p_mark->image.dy - p_mark->image.sy;

			DWORD t_err;
			if (p_mark->image.descriptor.image != nil)
			{
				/* UNCHECKED */ MCGImageSplitHBITMAPWithMask(t_src_dc, p_mark->image.descriptor.image, t_src_bitmap, t_src_mask);
				t_src_width = MCGImageGetWidth(p_mark->image.descriptor.image);
				t_src_height = MCGImageGetHeight(p_mark->image.descriptor.image);
			}

			if (!p_mark->image.descriptor.has_transform)
			{
				t_dst_width = t_src_width;
				t_dst_height = t_src_height;
			}
			else
			{
				// here, we only handle scaling transforms
				MCGAffineTransform t_transform = p_mark->image.descriptor.transform;

				MCGPoint t_topleft = MCGPointApplyAffineTransform(MCGPointMake(t_src_x, t_src_y), t_transform);
				MCGPoint t_bottomright = MCGPointApplyAffineTransform(MCGPointMake(t_src_x + t_src_width, t_src_y + t_src_height), t_transform);

				t_dst_width = t_bottomright.x - t_topleft.x;
				t_dst_height = t_bottomright.y - t_topleft.y;
			}

			// Pass JPEG data straight through to the printer, if it supports such data.
			bool t_handled;
			t_handled = false;
			if (p_mark -> image . descriptor . data_type == kMCImageDataJPEG)
			{
				DWORD t_test;
				t_test = CHECKJPEGFORMAT;
				if (ExtEscape(t_dc, QUERYESCSUPPORT, sizeof(t_test), (LPCSTR)&t_test, 0, 0) > 0 &&
					ExtEscape(t_dc, CHECKJPEGFORMAT, p_mark -> image . descriptor . data_size, (LPCSTR)p_mark -> image . descriptor . data_bits, sizeof(t_test), (LPSTR)&t_test) > 0 &&
					t_test == 1)
				{
					BITMAPINFO t_bitmap;
					memset(&t_bitmap, 0, sizeof(t_bitmap));
					t_bitmap . bmiHeader . biSize = sizeof(BITMAPINFOHEADER);
					t_bitmap . bmiHeader . biWidth = t_src_width;
					t_bitmap . bmiHeader . biHeight = -(signed)t_src_height; // top-down image
					t_bitmap . bmiHeader . biPlanes = 1;
					t_bitmap . bmiHeader . biBitCount = 0;
					t_bitmap . bmiHeader . biCompression = BI_JPEG;
					t_bitmap . bmiHeader . biSizeImage = p_mark -> image . descriptor . data_size;

					if (StretchDIBits(t_dc,
							t_src_x, t_src_y, t_dst_width, t_dst_height,
							0, 0, t_src_width, t_src_height,
							p_mark -> image . descriptor . data_bits,
							&t_bitmap,
							DIB_RGB_COLORS,
							SRCCOPY) != GDI_ERROR)
						t_handled = true;
				}
			}
			
			if (!t_handled)
			{
				HGDIOBJ t_old_bitmap;
				t_old_bitmap = SelectObject(t_src_dc, t_src_bitmap);
				StretchBlt(t_dc, t_src_x, t_src_y, t_dst_width, t_dst_height,
							t_src_dc, 0, 0, t_src_width, t_src_height,
							SRCCOPY);
				SelectObject(t_src_dc, t_old_bitmap);
			}

			if (t_src_bitmap != nil)
				DeleteObject(t_src_bitmap);
			if (t_src_mask != nil)
				DeleteObject(t_src_mask);
			RestoreDC(t_dc, -1);
			SetGraphicsMode(t_dc, GM_COMPATIBLE);
		}
		break;

		case MARK_TYPE_METAFILE:
		{
			Windows_RenderMetaFile(t_dc, NULL, (uint1 *)p_mark -> metafile . data, p_mark -> metafile . data_length, p_mark -> metafile . dst_area);
		}
		break;
	}

	if (t_is_pattern_fill)
	{
		// Set the clip path to the area we want to fill
		EndPath(t_dc);
		if (t_is_stroke)
			WidenPath(t_dc);
		SelectClipPath(t_dc, RGN_AND);

		// Compute the region we need to tile
		RECT t_clip;
		GetClipBox(t_dc, &t_clip);

		MCGImageRef t_image;
		MCGAffineTransform t_transform;

		// IM-2014-05-13: [[ HiResPatterns ]] Update pattern access to use lock function
		if (MCPatternLockForContextTransform(p_mark->fill->pattern, MCGAffineTransformMakeIdentity(), t_image, t_transform))
		{
			MCGRectangle t_src_rect;
			t_src_rect = MCGRectangleMake(0, 0,
				MCGImageGetWidth(t_image), MCGImageGetHeight(t_image));

			MCGRectangle t_dst_rect;
			t_dst_rect = MCGRectangleApplyAffineTransform(t_src_rect, t_transform);

			// Now create a suitable pattern by tiling out the selected pattern to be
			// at least 32x32

			int32_t t_x_tiles, t_y_tiles;
			t_x_tiles = ceilf(32.0 / t_dst_rect.size.width);
			t_y_tiles = ceilf(32.0 / t_dst_rect.size.height);

			int32_t t_width, t_height;
			t_width = ceilf(t_x_tiles * t_dst_rect.size.width);
			t_height = ceilf(t_y_tiles * t_dst_rect.size.height);

			HDC t_src_dc;
			t_src_dc = CreateCompatibleDC(m_dc);

			// draw the pattern image into a bitmap
			HBITMAP t_pattern = nil;
			void *t_bits = nil;
			/* UNCHECKED */ create_temporary_dib(t_src_dc, t_width, t_height, t_pattern, t_bits);
			MCGContextRef t_context = nil;
			/* UNCHECKED */ MCGContextCreateWithPixels(t_width, t_height, t_width * sizeof(uint32_t), t_bits, true, t_context);

			// IM-2013-08-14: [[ ResIndependence ]] Apply pattern scale factor
			// MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was nearest).
			MCGContextSetFillPattern(t_context, t_image, t_transform, kMCGImageFilterNone);
			MCGContextAddRectangle(t_context, MCGRectangleMake(0, 0, t_width, t_height));
			MCGContextFill(t_context);

			MCGContextRelease(t_context);

			MCPatternUnlock(p_mark->fill->pattern, t_image);


			// Finally adjust the starting position based on origin, and tile the clip.
			int32_t x, y;
			x = p_mark -> fill -> origin . x;
			y = p_mark -> fill -> origin . y;
			while(x > t_clip . left)
				x -= t_width;
			while(x + t_width < t_clip . left)
				x += t_width;
			while(y > t_clip . top)
				y -= t_height;
			while(y + t_height < t_clip . top)
				y += t_height;

			SelectObject(t_src_dc, t_pattern);
			for(; y < t_clip . bottom; y += t_height)
				for(int32_t tx = x; tx < t_clip . right; tx += t_width)
					BitBlt(m_dc, tx, y, t_width, t_height, t_src_dc, 0, 0, SRCCOPY);

			if (t_pattern != nil)
				DeleteObject(t_pattern);
			if (t_src_dc != nil)
				DeleteDC(t_src_dc);
		}
	}

	if (t_old_pen != NULL)
		DeleteObject(SelectObject(t_dc, t_old_pen));

	if (t_old_brush != NULL)
		DeleteObject(SelectObject(t_dc, t_old_brush));

	SelectClipRgn(t_dc, NULL);
}

#define SCALE 4

bool MCGDIMetaContext::begincomposite(const MCRectangle& p_mark_clip, MCGContextRef &r_context)
{
	bool t_success = true;

	MCGContextRef t_context = nil;
	void *t_bits = nil;

	HBITMAP t_bitmap;
	t_bitmap = nil;

	uint4 t_scale = SCALE;

	uint32_t t_width, t_height;
	t_width = p_mark_clip . width * t_scale;
	t_height = p_mark_clip . height * t_scale;

	t_success = create_temporary_dib(((MCScreenDC*)MCscreen)->getsrchdc(), t_width, t_height, t_bitmap, t_bits);

	if (t_success)
		t_success = MCGContextCreateWithPixels(t_width, t_height, t_width * sizeof(uint32_t), t_bits, true, t_context);
	
	if (t_success)
	{
		MCGContextScaleCTM(t_context, t_scale, t_scale);
		MCGContextTranslateCTM(t_context, -(MCGFloat)p_mark_clip . x, -(MCGFloat)p_mark_clip . y);

		m_composite_context = t_context;
		m_composite_bitmap = t_bitmap;
		m_composite_rect = p_mark_clip;

		r_context = m_composite_context;
	}
	else
	{
		MCGContextRelease(t_context);
		
		if (t_bitmap != nil)
			DeleteObject(t_bitmap);
	}

	return t_success;
}

void MCGDIMetaContext::endcomposite(MCRegionRef p_clip_region)
{
	POINT t_old_org;
	SIZE t_old_ext;
	GetWindowOrgEx(m_dc, &t_old_org);
	GetWindowExtEx(m_dc, &t_old_ext);

	SetWindowOrgEx(m_dc, t_old_org . x * SCALE, t_old_org . y * SCALE, NULL);
	SetWindowExtEx(m_dc, t_old_ext . cx * SCALE, t_old_ext . cy * SCALE, NULL);

	uint4 t_scale = SCALE;
	if (p_clip_region == NULL)
		SelectClipRgn(m_dc, NULL);
	else
		MCRegionConvertToDeviceAndClip(p_clip_region, (MCSysContextHandle)m_dc);

	HDC t_src_dc = ((MCScreenDC*)MCscreen)->getsrchdc();
	SelectObject(t_src_dc, m_composite_bitmap);

	BitBlt(m_dc, m_composite_rect . x * SCALE, m_composite_rect . y * SCALE, m_composite_rect . width * SCALE, m_composite_rect . height * SCALE, t_src_dc, 0, 0, SRCCOPY);

	if (p_clip_region != NULL)
	{
		MCRegionDestroy(p_clip_region);
		SelectClipRgn(m_dc, NULL);
	}

	SetWindowOrgEx(m_dc, t_old_org . x, t_old_org . y, NULL);
	SetWindowExtEx(m_dc, t_old_ext . cx, t_old_ext . cy, NULL);

	MCGContextRelease(m_composite_context);
	m_composite_context = nil;

	DeleteObject(m_composite_bitmap);
	m_composite_bitmap = nil;
}

////////////////////////////////////////////////////////////////////////////////

MCWindowsPrinterDevice::MCWindowsPrinterDevice(void)
{
	m_dc = NULL;
	m_error = NULL;
	m_page_started = false;
}

MCWindowsPrinterDevice::~MCWindowsPrinterDevice(void)
{
	delete m_error;
}

void MCWindowsPrinterDevice::SetError(const char *p_message)
{
	delete m_error;
	m_error = strdup(p_message);
}

const char *MCWindowsPrinterDevice::Error(void) const
{
	return m_error;
}

// SN-2014-12-22: [[ Bug 14278 ]] Function updated to take wchar params
MCPrinterResult MCWindowsPrinterDevice::Start(HDC p_dc, const wchar_t *p_document_name, const wchar_t *p_output_file)
{
    // SN-2014-12-22: [[ Bug 14278 ]] Document name and output file now encoded as UTF16 chars
	DOCINFOW t_info;
	t_info . cbSize = sizeof(DOCINFOW);
	t_info . lpszDocName = p_document_name;
	t_info . lpszOutput = p_output_file;
	t_info . lpszDatatype = NULL;
	t_info . fwType = 0;
	
	if (StartDocW(p_dc, &t_info) <= 0)
	{
		SetError("unable to start document");
		return PRINTER_RESULT_ERROR;
	}

	m_dc = p_dc;
	m_page_started = false;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCWindowsPrinterDevice::Finish(void)
{
	if (m_dc != NULL && m_page_started && EndPage(m_dc) <= 0)
	{
		Cancel();
		SetError("unable to end page");
		return PRINTER_RESULT_ERROR;
	}

	if (m_dc != NULL && EndDoc(m_dc) <= 0)
	{
		SetError("unable to finish document"); 
		return PRINTER_RESULT_ERROR;
	}

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCWindowsPrinterDevice::Cancel(void)
{
	if (m_dc != NULL)
	{
		AbortDoc(m_dc);
		m_dc = NULL;
	}

	return PRINTER_RESULT_CANCEL;
}

MCPrinterResult MCWindowsPrinterDevice::Show(void)
{
	if (m_dc == NULL)
		return PRINTER_RESULT_SUCCESS;

	if (!m_page_started && StartPage(m_dc) <= 0)
	{
		Cancel();
		SetError("unable to begin page");
		return PRINTER_RESULT_ERROR;
	}

	if (EndPage(m_dc) <= 0)
	{
		Cancel();
		SetError("Unable to end page");
		return PRINTER_RESULT_ERROR;
	}

	m_page_started = false;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCWindowsPrinterDevice::Begin(const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, MCContext*& r_context)
{
	if (m_dc == NULL)
		return PRINTER_RESULT_SUCCESS;

	if (!m_page_started && StartPage(m_dc) <= 0)
	{
		Cancel();
		SetError("unable to begin page");
		return PRINTER_RESULT_ERROR;
	}

	// Calculate the convex integer hull of the source rectangle.
	//
	MCRectangle t_src_rect_hull;
	t_src_rect_hull . x = (int4)floor(p_src_rect . left);
	t_src_rect_hull . y = (int4)floor(p_src_rect . top);
	t_src_rect_hull . width = (int4)(ceil(p_src_rect . right) - floor(p_src_rect . left));
	t_src_rect_hull . height = (int4)(ceil(p_src_rect . bottom) - floor(p_src_rect . top));

	// Create a GDI-based meta context targetting the previously computed
	// rectangle
	//
	MCGDIMetaContext *t_context;
	t_context = new (nothrow) MCGDIMetaContext(t_src_rect_hull);
	r_context = t_context;

	m_src_rect = p_src_rect;
	m_dst_rect = p_dst_rect;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCWindowsPrinterDevice::End(MCContext* p_raw_context)
{
	if (m_dc == NULL)
		return PRINTER_RESULT_SUCCESS;

	MCGDIMetaContext *t_context;
	t_context = static_cast<MCGDIMetaContext *>(p_raw_context);
	t_context -> render(m_dc, m_src_rect, m_dst_rect);

	delete t_context;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCWindowsPrinterDevice::Anchor(const char *name, double x, double y)
{
	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCWindowsPrinterDevice::Link(const char *dest, const MCPrinterRectangle& area, MCPrinterLinkType type)
{
	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCWindowsPrinterDevice::Bookmark(const char *title, double x, double y, int depth, bool closed)
{
	return PRINTER_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

void MCWindowsPrinter::DoInitialize(void)
{
	m_valid = false;
	// SN-2015-03-18: [[ Win shutdown crash ]] Avoid memory leak at printer
	//  initialisation
	MCValueAssign(m_name, kMCEmptyString);
	m_devmode = NULL;
	
	m_dc = NULL;
	m_dc_locked = false;
	m_dc_changed = false;

	DoReset(kMCEmptyString);
}

void MCWindowsPrinter::DoFinalize(void)
{
	if (m_dc != NULL)
		DeleteDC(m_dc);
	m_dc_locked = false;
	m_dc_changed = false;

	MCValueRelease(m_name);
	// SN-2015-03-18: [[ Win shutdown crash ]] Do not over-release m_name
	m_name = NULL;

	delete m_devmode;
	m_valid = false;
}

bool MCWindowsPrinter::DoReset(MCStringRef p_name)
{
	// Get a copy of the printer name - fetching the default
	// name if the incoming name is NULL.
	//
	MCAutoStringRef t_name;
	if (MCStringIsEmpty(p_name))
	{
		WindowsGetDefaultPrinter(&t_name);
		if (MCStringIsEmpty(*t_name))
			return false;
	}
	else
		t_name = p_name;

	// Fetch the DEVMODE for the printer with the given name
	//
	DEVMODEW *t_devmode;
	t_devmode = WindowsGetPrinterInfo(*t_name);
	if (t_devmode == NULL)
	{
		return false;
	}

	// Reset the printer properties based on the name and devmode
	// Ownership of these pointers passes to 'Reset'.
	//
	Reset(*t_name, t_devmode);

	return true;
}

bool MCWindowsPrinter::DoResetSettings(MCDataRef p_settings)
{
	if (MCDataIsEmpty(p_settings))
		return DoReset(kMCEmptyString);

	// Attempt to decode the settings string into name and devmode
	// This call also automatically validates the settings
	//
	MCAutoStringRef t_name;
	DEVMODEW *t_devmode;
	if (!DecodeSettings(p_settings, &t_name, t_devmode))
		return false;

	// Reset the printer properties based on the name and devmode
	// Ownership of these pointers passes to 'Reset'.
	//
	Reset(*t_name, t_devmode);

	return true;
}

const char *MCWindowsPrinter::DoFetchName(void)
{
    char *t_name;
    /* UNCHECKED */ MCStringConvertToCString(m_name, t_name);
	return t_name;
}

void MCWindowsPrinter::DoFetchSettings(void*& r_buffer, uint4& r_length)
{
	Synchronize();

	MCAutoDataRef t_data;
	EncodeSettings(m_name, m_devmode, &t_data);

	// This is ugly. Hopefully printing will get sorted out soon...
	void *t_buffer = new (nothrow) byte_t[MCDataGetLength(*t_data)];
	memcpy(t_buffer, MCDataGetBytePtr(*t_data), MCDataGetLength(*t_data));
	r_buffer = t_buffer;
	r_length = MCDataGetLength(*t_data);
}

void MCWindowsPrinter::DoResync(void)
{
	Synchronize();
	m_resync = false;
}

///////////////////////////////////////////////////////////////////////////////

MCPrinterDialogResult MCWindowsPrinter::DoPrinterSetup(bool p_window_modal, Window p_owner)
{
	HGLOBAL t_devmode_handle, t_devnames_handle;
	if (!FetchDialogData(t_devmode_handle, t_devnames_handle))
		return PRINTER_DIALOG_RESULT_ERROR;

	int t_range_count;
	t_range_count = MCU_max(0, GetJobRangeCount());

	PRINTPAGERANGE *t_ranges;
	t_ranges = new (nothrow) PRINTPAGERANGE[64];
	for(int i = 0; i < t_range_count; ++i)
	{
		t_ranges[i] . nFromPage = GetJobRanges()[i] . from;
		t_ranges[i] . nToPage = GetJobRanges()[i] . to;
	}

	MCPrinterDialogResult t_result;
	bool t_apply;
	t_apply = false;
	if (MCmajorosversion >= 0x0500)
	{
		// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
		PRINTDLGEXW t_dlg;
		memset(&t_dlg, 0, sizeof(PRINTDLGEXW));
		t_dlg . lStructSize = sizeof(PRINTDLGEXW);
		t_dlg . Flags = PD_USEDEVMODECOPIESANDCOLLATE;
		t_dlg . nStartPage = START_PAGE_GENERAL;
		t_dlg . hDevMode = t_devmode_handle;
		t_dlg . hDevNames = t_devnames_handle;
		t_dlg . hwndOwner = p_owner != NULL ? (HWND)p_owner -> handle . window : NULL;
		
		MCPrinterPageRangeCount t_count;

		t_count = MCprinter -> GetJobRangeCount();

		switch(GetJobRangeCount())
		{
		case PRINTER_PAGE_RANGE_ALL:
			t_dlg . Flags |= PD_ALLPAGES;
		break;

		case PRINTER_PAGE_RANGE_CURRENT:
			t_dlg . Flags |= PD_CURRENTPAGE;
		break;

		case PRINTER_PAGE_RANGE_SELECTION:
			t_dlg . Flags |= PD_SELECTION;
		break;

		default:
			t_dlg . Flags |= PD_PAGENUMS;
		break;
		}

		t_dlg . nPageRanges = t_range_count;
		t_dlg . nMaxPageRanges = 64;
		t_dlg . lpPageRanges = t_ranges;
		
		t_dlg . nMinPage = 0;
		t_dlg . nMaxPage = 65535;
	
		HRESULT t_dialog_result;
		if (!MCModeMakeLocalWindows())
		{
			char *t_databuffer = NULL;
			uint32_t t_databuffer_size = 0;
			if (serialize_printdlg_data(t_databuffer, t_databuffer_size, t_dlg))
			{
				MCAutoDataRef t_replydata;
				uint32_t t_replyresult;

                MCAutoDataRef t_databuffer_str;
                 /* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t *)t_databuffer, t_databuffer_size, &t_databuffer_str);
                MCRemotePrintSetupDialog(*t_databuffer_str, &t_replydata, t_replyresult);

				// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
				PRINTDLGEXW *t_dlg_ptr = &t_dlg;
				deserialize_printdlg_data((const char *)MCDataGetBytePtr(*t_replydata), MCDataGetLength(*t_replydata), t_dlg_ptr);
				t_dialog_result = (HRESULT)t_replyresult;
			}
		}
		else
		{
			t_dialog_result = WindowsPrintDlgEx(&t_dlg);
		}

		if (t_dialog_result == S_OK)
		{
			switch(t_dlg . dwResultAction)
			{
			case PD_RESULT_APPLY:
				t_result = PRINTER_DIALOG_RESULT_CANCEL;
				t_apply = true;
			break;
			case PD_RESULT_CANCEL:
				t_result = PRINTER_DIALOG_RESULT_CANCEL;
			break;
			case PD_RESULT_PRINT:
				t_result = PRINTER_DIALOG_RESULT_OKAY;
				t_apply = true;
			break;
			}
		}
		else
			t_result = PRINTER_DIALOG_RESULT_ERROR;

		if ((t_dlg . Flags & PD_ALLPAGES) != 0)
			t_range_count = PRINTER_PAGE_RANGE_ALL;
		else if ((t_dlg . Flags & PD_SELECTION) != 0)
			t_range_count = PRINTER_PAGE_RANGE_SELECTION;
		else if ((t_dlg . Flags & PD_CURRENTPAGE) != 0)
			t_range_count = PRINTER_PAGE_RANGE_CURRENT;
		else if ((t_dlg . Flags & PD_PAGENUMS) != 0)
			t_range_count = t_dlg . nPageRanges;

		t_devmode_handle = t_dlg . hDevMode;
		t_devnames_handle = t_dlg . hDevNames;
	}
	else
	{
		// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
		PRINTDLGW t_dlg;
		memset(&t_dlg, 0, sizeof(PRINTDLGW));
		t_dlg . lStructSize = sizeof(PRINTDLGW);
		t_dlg . Flags = PD_USEDEVMODECOPIESANDCOLLATE;
		t_dlg . hDevMode = t_devmode_handle;
		t_dlg . hDevNames = t_devnames_handle;
		t_dlg . hwndOwner = p_owner != NULL ? (HWND)p_owner -> handle . window : NULL;
		
		switch(GetJobRangeCount())
		{
		case PRINTER_PAGE_RANGE_ALL:
			t_dlg . Flags |= PD_ALLPAGES;
		break;

		case PRINTER_PAGE_RANGE_CURRENT:
		case PRINTER_PAGE_RANGE_SELECTION:
			t_dlg . Flags |= PD_SELECTION;
		break;

		default:
			t_dlg . Flags |= PD_PAGENUMS;
		break;
		}

		if (t_range_count > 0)
		{
			t_dlg . nFromPage = (WORD)t_ranges[0] . nFromPage;
			t_dlg . nToPage = (WORD)t_ranges[t_range_count - 1] . nToPage;
		}

		t_dlg . nMinPage = 0;
		t_dlg . nMaxPage = 65535;

		// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
		if (PrintDlgW(&t_dlg) != 0)
			t_result = PRINTER_DIALOG_RESULT_OKAY, t_apply = true;
		else if (CommDlgExtendedError() == 0)
			t_result = PRINTER_DIALOG_RESULT_CANCEL;
		else
			t_result = PRINTER_DIALOG_RESULT_ERROR;

		if ((t_dlg . Flags & PD_ALLPAGES) != 0)
			t_range_count = PRINTER_PAGE_RANGE_ALL;
		else if ((t_dlg . Flags & PD_SELECTION) != 0)
			t_range_count = PRINTER_PAGE_RANGE_SELECTION;
		else if ((t_dlg . Flags & PD_PAGENUMS) != 0)
		{
			t_range_count = 1;
			t_ranges[0] . nFromPage = t_dlg . nFromPage;
			t_ranges[0] . nToPage = t_dlg . nToPage;
		}

		t_devmode_handle = t_dlg . hDevMode;
		t_devnames_handle = t_dlg . hDevNames;
	}

	if (t_apply)
	{
		MCInterval *t_page_ranges;
		t_page_ranges = NULL;
		int t_page_range_count;
		t_page_range_count = 0;
		for(int i = 0; i < t_range_count; ++i)
		{
			if (t_ranges[i] . nFromPage > t_ranges[i] . nToPage)
				continue;
			if (t_ranges[i] . nToPage < 1)
				continue;
			MCU_disjointrangeinclude(t_page_ranges, t_page_range_count, t_ranges[i] . nFromPage, t_ranges[i] . nToPage);
		}
		// MW-2010-10-18: [[ Bug 9102 ]] Wrong count passed to the method.
		SetJobRanges(t_page_range_count, t_page_ranges);
		StoreDialogData(t_devmode_handle, t_devnames_handle);

		delete t_page_ranges;
	}

	delete t_ranges;

	return t_result;
}

MCPrinterDialogResult MCWindowsPrinter::DoPageSetup(bool p_window_modal, Window p_owner)
{
	HGLOBAL t_devmode_handle, t_devnames_handle;
	if (!FetchDialogData(t_devmode_handle, t_devnames_handle))
		return PRINTER_DIALOG_RESULT_ERROR;

	// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
	MCPrinterDialogResult t_result;
	PAGESETUPDLGW t_dlg;
	memset(&t_dlg, 0, sizeof(PAGESETUPDLGW));
	t_dlg . lStructSize = sizeof(PAGESETUPDLGW);
	t_dlg . Flags = PSD_INTHOUSANDTHSOFINCHES | PSD_MARGINS | PSD_MINMARGINS;
	t_dlg . hDevMode = t_devmode_handle;
	t_dlg . hDevNames = t_devnames_handle;
	t_dlg . ptPaperSize . x = PointsToThousandthsInch(GetPageWidth());
	t_dlg . ptPaperSize . y = PointsToThousandthsInch(GetPageHeight());
	t_dlg . rtMargin . left = PointsToThousandthsInch(GetPageLeftMargin());
	t_dlg . rtMargin . top = PointsToThousandthsInch(GetPageTopMargin());
	t_dlg . rtMargin . right = PointsToThousandthsInch(GetPageRightMargin());
	t_dlg . rtMargin . bottom = PointsToThousandthsInch(GetPageBottomMargin());
	t_dlg . hwndOwner = p_owner != NULL ? (HWND)p_owner -> handle . window : NULL;
	
	bool t_apply;
	t_apply = false;
    
	if (!MCModeMakeLocalWindows())
	{
		char *t_databuffer = NULL;
		uint32_t t_databuffer_size = 0;
		if (serialize_pagedlg_data(t_databuffer, t_databuffer_size, t_dlg))
		{
			MCAutoDataRef t_replydata;
            uint32_t t_replyresult;

            MCAutoDataRef t_databuffer_str;
            /* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t *)t_databuffer, t_databuffer_size, &t_databuffer_str);
			MCRemotePageSetupDialog(*t_databuffer_str, &t_replydata, t_replyresult);

			// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
			PAGESETUPDLGW *t_dlg_ptr = &t_dlg;
			deserialize_pagedlg_data((const char *)MCDataGetBytePtr(*t_replydata), MCDataGetLength(*t_replydata), t_dlg_ptr);
			t_result = (MCPrinterDialogResult) t_replyresult;
			if (t_result == PRINTER_DIALOG_RESULT_OKAY)
				t_apply = true;

		}
	}
	else
	{
		// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
		if (PageSetupDlgW(&t_dlg) != 0)
			t_result = PRINTER_DIALOG_RESULT_OKAY, t_apply = true;
		else if (CommDlgExtendedError() == 0)
			t_result = PRINTER_DIALOG_RESULT_CANCEL;
		else
			t_result = PRINTER_DIALOG_RESULT_ERROR;
	}

	t_devmode_handle = t_dlg . hDevMode;
	t_devnames_handle = t_dlg . hDevNames;

	if (t_apply)
	{
		SetPageSize(ThousandthsInchToPoints(t_dlg . ptPaperSize . x), ThousandthsInchToPoints(t_dlg . ptPaperSize . y));
		SetPageMargins(ThousandthsInchToPoints(t_dlg . rtMargin . left), ThousandthsInchToPoints(t_dlg . rtMargin . top), ThousandthsInchToPoints(t_dlg . rtMargin . right), ThousandthsInchToPoints(t_dlg . rtMargin . bottom));
		StoreDialogData(t_devmode_handle, t_devnames_handle);
	}

	return t_result;
}

///////////////////////////////////////////////////////////////////////////////

MCPrinterResult MCWindowsPrinter::DoBeginPrint(MCStringRef p_document_name, MCPrinterDevice*& r_device)
{
	if (!m_valid)
		return PRINTER_RESULT_ERROR;

	HDC t_dc;
	t_dc = LockDC();
	if (t_dc == NULL)
		return PRINTER_RESULT_ERROR;

	MCWindowsPrinterDevice *t_device;
	t_device = new (nothrow) MCWindowsPrinterDevice;

	MCPrinterResult t_result;
    // SN-2014-12-22: [[ Bug 14278 ]] Updated to Unicode name and location
    MCAutoStringRefAsWString t_doc_name, t_output_file;
    /* UNCHECKED */ t_doc_name . Lock(p_document_name);
    
    MCAutoStringRef t_output_file_stringref;
	if (GetDeviceOutputLocation() != NULL)
	{
		/* UNCHECKED */ MCStringCreateWithBytes((byte_t*)GetDeviceOutputLocation(), strlen(GetDeviceOutputLocation()), kMCStringEncodingUTF8, false, &t_output_file_stringref);
		/* UNCHECKED */ t_output_file . Lock(*t_output_file_stringref);
	}
	else
		t_output_file . Lock(kMCEmptyString);
    
	t_result = t_device -> Start(t_dc, *t_doc_name, *t_output_file);

	r_device = t_device;

	return t_result;
}

MCPrinterResult MCWindowsPrinter::DoEndPrint(MCPrinterDevice *p_device)
{
	MCPrinterResult t_result;
	t_result = static_cast<MCWindowsPrinterDevice *>(p_device) -> Finish();

	UnlockDC();

	delete p_device;

	return t_result;
}

///////////////////////////////////////////////////////////////////////////////

// This call synchronizes the printer properties with those contained
// within p_name and p_devmode.
//
// Ownership of p_name and p_devmode pass to this routine.
//
void MCWindowsPrinter::Reset(MCStringRef p_name, DEVMODEW *p_devmode)
{
	// MW-2008-10-15: [[ Bug ]] Changed routine to *not* modify the DEVMODEA structure.
	//   Doing this caused Windows to completely ignore settings on certain printers
	//   thus resulting in a mismatch between printPaperRect and printRect.

	SetDeviceOutput(PRINTER_OUTPUT_DEVICE, NULL);
	
	int32_t t_paper_width;
	t_paper_width = TenthMillimetresToPoints(p_devmode -> dmPaperWidth);

	int32_t t_paper_height;
	t_paper_height = TenthMillimetresToPoints(p_devmode -> dmPaperLength);

	// Work out the paper size if not specified explicitly
	if ((p_devmode -> dmFields & DM_PAPERSIZE) != 0 && p_devmode -> dmPaperSize != 0)
	{
		POINT t_paper_size;
		if (WindowsGetPrinterPaperSize(p_name, p_devmode, t_paper_size))
		{
			t_paper_width = TenthMillimetresToPoints(t_paper_size . x);
			t_paper_height = TenthMillimetresToPoints(t_paper_size . y);
		}
	}
	
	// Ensure that all relevant fields in the DEVMODE structure are filled in
	// with defaults if not already set.
	//
	if (t_paper_width == 0 && t_paper_height == 0)
	{
		t_paper_width = PointsToTenthMillimetres(PRINTER_DEFAULT_PAGE_WIDTH);
		t_paper_height = PointsToTenthMillimetres(PRINTER_DEFAULT_PAGE_HEIGHT);
	}

	MCPrinterOrientation t_orientation;
	if ((p_devmode -> dmFields & DM_ORIENTATION) == 0)
		t_orientation = PRINTER_ORIENTATION_PORTRAIT;
	else
		t_orientation = p_devmode -> dmOrientation == DMORIENT_PORTRAIT ? PRINTER_ORIENTATION_PORTRAIT : PRINTER_ORIENTATION_LANDSCAPE;

	float64_t t_scale;
	if ((p_devmode -> dmFields & DM_SCALE) == 0)
		t_scale = 1.0;
	else
		t_scale = p_devmode -> dmScale / 100.0;

	uint32_t t_copies;
	if ((p_devmode -> dmFields & DM_COPIES) == 0)
		t_copies = 1;
	else
		t_copies = p_devmode -> dmCopies;

	bool t_color;
	if ((p_devmode -> dmFields & DM_COLOR) == 0)
		t_color = true;
	else
		t_color = p_devmode -> dmColor == DMCOLOR_COLOR;

	MCPrinterDuplexMode t_duplex;
	if ((p_devmode -> dmFields & DM_DUPLEX) == 0)
		t_duplex = PRINTER_DUPLEX_MODE_SIMPLEX;
	else
		t_duplex = p_devmode -> dmDuplex == DMDUP_SIMPLEX ? PRINTER_DUPLEX_MODE_SIMPLEX : (p_devmode -> dmDuplex == DMDUP_HORIZONTAL ? PRINTER_DUPLEX_MODE_SHORT_EDGE : PRINTER_DUPLEX_MODE_LONG_EDGE);

	bool t_collate;
	if ((p_devmode -> dmFields & DM_COLLATE) != 0)
		t_collate = true;
	else
		t_collate = p_devmode -> dmCollate == DMCOLLATE_TRUE;

	SetDeviceFeatures(WindowsGetPrinterFeatures(p_name, p_devmode));
	SetDeviceRectangle(WindowsGetPrinterRectangle(p_name, p_devmode));
	SetPageSize(t_paper_width, t_paper_height);
	SetPageOrientation(t_orientation);
	SetPageScale(t_scale);
	SetJobCopies(t_copies);
	SetJobColor(t_color);
	SetJobDuplex(t_duplex);
	SetJobCollate(t_collate);

	MCValueAssign(m_name, p_name);

	if (m_devmode != NULL)
		delete m_devmode;

	m_devmode = p_devmode;

	m_valid = true;

	ChangeDC();
}

void MCWindowsPrinter::Synchronize(void)
{
	if (!m_valid)
		return;

	uint32_t t_index;
	if (WindowsGetPrinterPaperIndex(m_name, m_devmode, GetPageWidth(), GetPageHeight(), t_index))
	{
		m_devmode -> dmPaperSize = t_index;
		m_devmode -> dmFields |= DM_PAPERSIZE;
		m_devmode -> dmFields &= ~(DM_PAPERWIDTH | DM_PAPERLENGTH);
	}
	else
	{
		m_devmode -> dmPaperSize = 0;
		m_devmode -> dmFields |= DM_PAPERSIZE;

		m_devmode -> dmPaperWidth = PointsToTenthMillimetres(GetPageWidth());
		m_devmode -> dmFields |= DM_PAPERWIDTH;

		m_devmode -> dmPaperLength = PointsToTenthMillimetres(GetPageHeight());
		m_devmode -> dmFields |= DM_PAPERLENGTH;
	}

	m_devmode -> dmFields |= DM_ORIENTATION;
	if (GetPageOrientation() == PRINTER_ORIENTATION_PORTRAIT || GetPageOrientation() == PRINTER_ORIENTATION_REVERSE_PORTRAIT)
		m_devmode -> dmOrientation = DMORIENT_PORTRAIT;
	else
		m_devmode -> dmOrientation = DMORIENT_LANDSCAPE;

	m_devmode -> dmFields |= DM_SCALE;
	m_devmode -> dmScale = (int32_t)floor(GetPageScale() * 100.0);

	m_devmode -> dmFields |= DM_COPIES;
	m_devmode -> dmCopies = GetJobCopies();

	m_devmode -> dmFields |= DM_DUPLEX;
	m_devmode -> dmDuplex = GetJobDuplex() == PRINTER_DUPLEX_MODE_SIMPLEX ? DMDUP_SIMPLEX : (GetJobDuplex() == PRINTER_DUPLEX_MODE_SHORT_EDGE ? DMDUP_HORIZONTAL : DMDUP_VERTICAL);

	m_devmode -> dmFields |= DM_COLLATE;
	m_devmode -> dmCollate = GetJobCollate() ? DMCOLLATE_TRUE : DMCOLLATE_FALSE;

	DEVMODEW *t_devmode;
	t_devmode = WindowsGetPrinterInfo(m_name, m_devmode);
	if (t_devmode != NULL)
	{
		delete m_devmode;
		m_devmode = t_devmode;

		SetDeviceFeatures(WindowsGetPrinterFeatures(m_name, t_devmode));
		SetDeviceRectangle(WindowsGetPrinterRectangle(m_name, t_devmode));
	}

	m_resync = false;
}

// Fetch global handles filled with the current printer settings suitable for
// use with a printer dialog.
//
bool MCWindowsPrinter::FetchDialogData(HGLOBAL& r_devmode_handle, HGLOBAL& r_devnames_handle)
{
	bool t_success;
	t_success = true;

	if (!m_valid)
		t_success = false;

	if (m_valid)
		Synchronize();

	// Allocate and store the DEVMODE structure into a Global memory block
	//
	HANDLE t_devmode_handle;
	t_devmode_handle = NULL;
	if (t_success)
	{
		int t_devmode_size;
		t_devmode_size = m_devmode -> dmSize + m_devmode -> dmDriverExtra;
		t_devmode_handle = GlobalAlloc(GMEM_MOVEABLE, t_devmode_size);
		if (t_devmode_handle != NULL)
		{
			memcpy(GlobalLock(t_devmode_handle), m_devmode, t_devmode_size);
			GlobalUnlock(t_devmode_handle);
		}
		else
			t_success = false;
	}

	// Allocate and store the DEVNAMES structure in a Global memory block
	// 
	// Here, we allocated the size of the header (DEVNAMES) + the printer name
	// + \0 + "FILE:" + \0;
	//
	HANDLE t_devnames_handle;
	t_devnames_handle = NULL;
	if (t_success)
	{
		MCAutoStringRef t_string;
		/* UNCHECKED */ MCStringFormat(&t_string, "%@\0FILE:\0", m_name);

		int t_devnames_size;
		// SN-2014-07-24: [[ Bug 12916 ]] Closing the Page Setup dialog causes a crash
		//  The size is indeed in unichars, not chars
		//t_devnames_size = sizeof(DEVNAMES) + strlen(t_string) + 7;
		t_devnames_size = sizeof(DEVNAMES) + sizeof(unichar_t) * MCStringGetLength(*t_string);
		t_devnames_handle = GlobalAlloc(GMEM_MOVEABLE, t_devnames_size);
		if (t_devnames_handle != NULL)
		{
			DEVNAMES *t_devnames;
			t_devnames = (DEVNAMES *)GlobalLock(t_devnames_handle);
			t_devnames -> wDriverOffset = t_devnames_size -	1;
			t_devnames -> wOutputOffset = GetDeviceOutputType() == PRINTER_OUTPUT_FILE ? t_devnames_size - 6 : t_devnames_size - 1;
			t_devnames -> wDeviceOffset = sizeof(DEVNAMES);
			t_devnames -> wDefault = 0;

			// SN-2014-08-07: [[ Bug 13084 ]] The pointer arithmetic wasn't right
			MCStringGetChars(*t_string, MCRangeMake(0, MCStringGetLength(*t_string)), (unichar_t*)(t_devnames + 1));

			GlobalUnlock(t_devnames_handle);
		}
		else
			t_success = false;
	}

	if (t_success)
	{
		r_devmode_handle = t_devmode_handle;
		r_devnames_handle = t_devnames_handle;
	}
	else
	{
		if (t_devmode_handle != NULL)
			GlobalFree(t_devmode_handle);

		if (t_devnames_handle != NULL)
			GlobalFree(t_devnames_handle);
	}

	return t_success;
}

void MCWindowsPrinter::StoreDialogData(HGLOBAL p_devmode_handle, HGLOBAL p_devnames_handle)
{
	bool t_success;
	t_success = true;

	MCAutoStringRef t_output_file;
	MCAutoStringRef t_printer_name;
	
	if (t_success)
	{
		if (p_devnames_handle != NULL)
		{
			// SN-2014-07-08: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
			//  and make sure the pointer arithmetic is right
			DEVNAMES *t_devnames;
			t_devnames = (DEVNAMES *)GlobalLock(p_devnames_handle);

			wchar_t* t_chars;
			t_chars = (wchar_t *)t_devnames + t_devnames -> wDeviceOffset;
			/* UNCHECKED */ MCStringCreateWithChars(t_chars, lstrlenW(t_chars), &t_printer_name);
 
			if (lstrcmpW((wchar_t *)t_devnames + t_devnames -> wOutputOffset, L"FILE:") == 0)
				/* UNCHECKED */ t_output_file = kMCEmptyString;

			GlobalUnlock(p_devnames_handle);
			GlobalFree(p_devnames_handle);
		}
		else
			WindowsGetDefaultPrinter(&t_printer_name);

		if (MCStringIsEmpty(*t_printer_name))
			t_success = false;
	}

	DEVMODEW *t_devmode;
	t_devmode = NULL;
	if (t_success)
	{
		if (p_devmode_handle != NULL)
		{
			DEVMODEW *t_devmode_ptr;
			t_devmode_ptr = (DEVMODEW *)GlobalLock(p_devmode_handle);
			t_devmode = (DEVMODEW *)memdup(t_devmode_ptr, t_devmode_ptr -> dmSize + t_devmode_ptr -> dmDriverExtra);
			GlobalUnlock(p_devmode_handle);
			GlobalFree(p_devmode_handle);
		}
		else
			t_devmode = WindowsGetPrinterInfo(*t_printer_name);

		if (t_devmode == NULL)
			t_success = false;
	}

	if (t_success)
	{
		if ((t_devmode -> dmFields & DM_SCALE) == 0)
		{
			t_devmode -> dmScale = (int32_t)floor(GetPageScale() * 100.0);
			t_devmode -> dmFields |= DM_SCALE;
		}

		Reset(*t_printer_name, t_devmode);
		if (*t_output_file != NULL)
			SetDeviceOutput(PRINTER_OUTPUT_FILE, *t_output_file);
	}
	else
	{
		delete t_devmode;
	}
}

bool MCWindowsPrinter::DecodeSettings(MCDataRef p_settings, MCStringRef &r_name, DEVMODEW* &r_devmode)
{
	MCDictionary t_dictionary;
	if (!t_dictionary . Unpickle(MCDataGetBytePtr(p_settings), MCDataGetLength(p_settings)))
		return false;

	MCString t_name_old;
	MCAutoStringRef t_name;
	if (!t_dictionary . Get('NMEA', t_name_old))
		return false;
	if (!MCStringCreateWithOldString(t_name_old, &t_name))
		return false;

	MCString t_devmode;
	if (!t_dictionary . Get('W32A', t_devmode))
		return false;

	DEVMODEW *t_validated_devmode;
	t_validated_devmode = WindowsGetPrinterInfo(*t_name, (DEVMODEW *)t_devmode . getstring());
	if (t_validated_devmode == NULL)
		return false;

	r_name = MCValueRetain(*t_name);
	r_devmode = t_validated_devmode;

	return true;
}

void MCWindowsPrinter::EncodeSettings(MCStringRef p_name, DEVMODEW* p_devmode, MCDataRef &r_buffer)
{
	if (MCStringIsEmpty(p_name) || p_devmode == NULL)
	{
		r_buffer = MCValueRetain(kMCEmptyData);
		return;
	}

	MCDictionary t_dictionary;

	void *t_temp;
	uint4 t_len;
    MCAutoStringRefAsCString t_name;
    /* UNCHECKED */ t_name.Lock(p_name);
    t_dictionary . Set('NMEA', *t_name);
	t_dictionary . Set('W32A', MCString((char *)p_devmode, p_devmode -> dmSize + p_devmode -> dmDriverExtra));
	t_dictionary . Pickle(t_temp, t_len);

	/* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t*)t_temp, t_len, r_buffer);
}

HDC MCWindowsPrinter::GetDC(bool p_synchronize)
{
	if ((p_synchronize || m_resync) && m_dc != NULL)
	{
		DeleteDC(m_dc);
		m_dc = NULL;
	}

	if (m_dc == NULL)
	{
		if (p_synchronize || m_resync)
			Synchronize();

		MCAutoStringRefAsWString t_name_wstr;
		/* UNCHECKED */ t_name_wstr.Lock(m_name);

		m_dc = CreateDCW(NULL, *t_name_wstr, NULL, m_devmode);

		// MW-2009-04-23: [[ Bug ]] Make sure we set up the standard printer co-ordinate mapping to ensure
		//   that fonts are measured correctly.
		SetMapMode(m_dc, MM_ANISOTROPIC);
		SetWindowExtEx(m_dc, 72, 72, NULL);
		SetViewportExtEx(m_dc, GetDeviceCaps(m_dc, LOGPIXELSX), GetDeviceCaps(m_dc, LOGPIXELSY), NULL);
	}

	return m_dc;
}

HDC MCWindowsPrinter::LockDC(void)
{
	m_dc_locked = true;
	return GetDC(true);
}

void MCWindowsPrinter::UnlockDC(void)
{
	m_dc_locked = false;
	if (m_dc_changed)
	{
		m_dc_changed = false;
		ChangeDC();
	}
}

void MCWindowsPrinter::ChangeDC(void)
{
	if (m_dc == NULL)
		return;

	if (m_dc_locked)
	{
		m_dc_changed = true;
		return;
	}

	DeleteDC(m_dc);
	m_dc = NULL;

	MCstacks -> purgefonts();
}

///////////////////////////////////////////////////////////////////////////////

// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
bool deserialize_data_to_hglobal(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, HGLOBAL &r_hglobal)
{
	bool t_success = true;
	uint32_t t_size;
	uint32_t t_offset = r_offset;
	t_success = deserialize_uint32(p_stream, p_stream_size, t_offset, t_size);
	if (t_success)
	{
		if (r_hglobal == NULL)
			r_hglobal = GlobalAlloc(GMEM_MOVEABLE, t_size);
		else if (GlobalSize(r_hglobal) < t_size)
			r_hglobal = GlobalReAlloc(r_hglobal, t_size, 0);
		t_success = (r_hglobal != NULL);
	}
	if (t_success)
	{
		void *t_ptr = GlobalLock(r_hglobal);
		t_success = deserialize_data(p_stream, p_stream_size, r_offset, t_ptr, t_size);
		GlobalUnlock(r_hglobal);
	}
	return t_success;
}

// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
bool serialize_printdlg_data(char *&r_buffer, uint32_t &r_size, PRINTDLGEXW &p_data)
{
	bool t_success = true;

	uint32_t t_size, t_devmode_size, t_devnames_size, t_pagerange_size;
	t_devmode_size = GlobalSize(p_data.hDevMode);
	t_devnames_size = GlobalSize(p_data.hDevNames);
	t_pagerange_size = p_data.nMaxPageRanges * sizeof(PRINTPAGERANGE);
	t_size = p_data.lStructSize + t_devmode_size + t_devnames_size + t_pagerange_size;
	// reserve space for byte counts
	t_size += sizeof(p_data.lStructSize) + sizeof(t_devmode_size) + sizeof(t_devnames_size) + sizeof(t_pagerange_size);

	char *t_buffer = NULL;
	uint32_t t_offset = 0;
	t_buffer = (char*)malloc(t_size);

	t_success = (t_buffer != NULL);

	if (t_success)
		t_success = serialize_data(t_buffer, t_size, t_offset, &p_data, p_data.lStructSize);

	if (t_success)
	{
		t_success = serialize_data(t_buffer, t_size, t_offset, GlobalLock(p_data.hDevMode), t_devmode_size);
		GlobalUnlock(p_data.hDevMode);
	}

	if (t_success)
	{
		t_success = serialize_data(t_buffer, t_size, t_offset, GlobalLock(p_data.hDevNames), t_devnames_size);
		GlobalUnlock(p_data.hDevNames);
	}

	if (t_success)
		t_success = serialize_data(t_buffer, t_size, t_offset, &p_data.lpPageRanges, t_pagerange_size);

	if (t_success)
	{
		r_buffer = t_buffer;
		r_size = t_size;
	}

	return t_success;
}

bool deserialize_printdlg_data(const char *p_buffer, uint32_t p_size, PRINTDLGEXW *&x_data)
{
	bool t_success = true;

	HGLOBAL t_devmode, t_devnames;
	PRINTPAGERANGE *t_ranges;
	uint32_t t_ranges_size = 0;
	uint32_t t_dlg_size = 0;

	uint32_t t_offset = 0;
	void *t_ptr;

	if (x_data == NULL)
	{
		t_devmode = t_devnames = t_ranges = NULL;
	}
	else
	{
		t_devmode = x_data->hDevMode;
		t_devnames = x_data->hDevNames;
		t_ranges = x_data->lpPageRanges;
		t_ranges_size = x_data->nMaxPageRanges * sizeof(PRINTPAGERANGE);
		// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
		t_dlg_size = sizeof(PRINTDLGEXW);
	}

	// copy dialog data
	t_ptr = x_data;
	t_success = deserialize_data(p_buffer, p_size, t_offset, t_ptr, t_dlg_size);
	// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
	x_data = (PRINTDLGEXW*)t_ptr;

	if (t_success)
		t_success = deserialize_data_to_hglobal(p_buffer, p_size, t_offset, t_devmode);

	if (t_success)
		t_success = deserialize_data_to_hglobal(p_buffer, p_size, t_offset, t_devnames);

	if (t_success)
	{
		t_ptr = t_ranges;
		t_success = deserialize_data(p_buffer, p_size, t_offset, t_ptr, t_ranges_size);
		t_ranges = (PRINTPAGERANGE*)t_ptr;
	}

	if (t_success)
	{
		x_data->hDevMode = t_devmode;
		x_data->hDevNames = t_devnames;
		x_data->lpPageRanges = t_ranges;
	}

	return t_success;
}

// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
bool serialize_pagedlg_data(char *&r_buffer, uint32_t &r_size, PAGESETUPDLGW &p_data)
{
	bool t_success = true;

	uint32_t t_size, t_devmode_size, t_devnames_size;
	t_devmode_size = GlobalSize(p_data.hDevMode);
	t_devnames_size = GlobalSize(p_data.hDevNames);
	t_size = p_data.lStructSize + t_devmode_size + t_devnames_size;
	// reserve space for byte counts
	t_size += sizeof(p_data.lStructSize) + sizeof(t_devmode_size) + sizeof(t_devnames_size);

	char *t_buffer = NULL;
	uint32_t t_offset = 0;
	t_buffer = (char*)malloc(t_size);

	t_success = (t_buffer != NULL);

	if (t_success)
		t_success = serialize_data(t_buffer, t_size, t_offset, &p_data, p_data.lStructSize);

	if (t_success)
	{
		t_success = serialize_data(t_buffer, t_size, t_offset, GlobalLock(p_data.hDevMode), t_devmode_size);
		GlobalUnlock(p_data.hDevMode);
	}

	if (t_success)
	{
		t_success = serialize_data(t_buffer, t_size, t_offset, GlobalLock(p_data.hDevNames), t_devnames_size);
		GlobalUnlock(p_data.hDevNames);
	}

	if (t_success)
	{
		r_buffer = t_buffer;
		r_size = t_size;
	}

	return t_success;
}

// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
bool deserialize_pagedlg_data(const char *p_buffer, uint32_t p_size, PAGESETUPDLGW *&x_data)
{
	bool t_success = true;

	HGLOBAL t_devmode, t_devnames;
	uint32_t t_dlg_size = 0;

	uint32_t t_offset = 0;
	void *t_ptr;

	if (x_data == NULL)
	{
		t_devmode = t_devnames = NULL;
	}
	else
	{
		t_devmode = x_data->hDevMode;
		t_devnames = x_data->hDevNames;
		// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
		t_dlg_size = sizeof(PAGESETUPDLGW);
	}

	// copy dialog data
	t_ptr = x_data;
	t_success = deserialize_data(p_buffer, p_size, t_offset, t_ptr, t_dlg_size);
	// SN-2014-08-07: [[ Bug 13084 ]] Update the Windows structures to the Unicode ones
	x_data = (PAGESETUPDLGW*)t_ptr;

	if (t_success)
		t_success = deserialize_data_to_hglobal(p_buffer, p_size, t_offset, t_devmode);

	if (t_success)
		t_success = deserialize_data_to_hglobal(p_buffer, p_size, t_offset, t_devnames);

	if (t_success)
	{
		x_data->hDevMode = t_devmode;
		x_data->hDevNames = t_devnames;
	}

	return t_success;
}

///////////////////////////////////////////////////////////////////////////////
