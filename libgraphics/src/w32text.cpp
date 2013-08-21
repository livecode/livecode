#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font)
{
	if (!MCGContextIsValid(self))
		return;	
	
	bool t_success;
	t_success = true;
	
	HDC t_gdicontext;
	t_gdicontext = NULL;
	if (t_success)
	{
		t_gdicontext = CreateCompatibleDC(NULL);
		t_success = t_gdicontext != NULL;
	}
	
	if (t_success)
		t_success = SetGraphicsMode(t_gdicontext, GM_ADVANCED) != 0;
	
	if (t_success)
		t_success = SelectObject(t_gdicontext, p_font . fid) != NULL;
	
	SIZE t_size;
	if (t_success)
		t_success = GetTextExtentPoint32W(t_gdicontext, (LPCWSTR)p_text, p_length >> 1, &t_size);
	
	TEXTMETRICA t_metrics;
	if (t_success)
		t_success = GetTextMetricsA(t_gdicontext, &t_metrics);
	
	MCGIntRectangle t_text_bounds, t_clipped_bounds;
	MCGAffineTransform t_transform;
	MCGPoint t_device_location;
	if (t_success)
	{
		MCGRectangle t_float_text_bounds;
		t_float_text_bounds . origin . x = 0;
		t_float_text_bounds . origin . y = -t_metrics . tmAscent;
		t_float_text_bounds . size . width = t_size . cx;
		t_float_text_bounds . size . height = t_metrics . tmAscent + t_metrics . tmDescent;
		
		t_transform = MCGContextGetDeviceTransform(self);
		t_device_location = MCGPointApplyAffineTransform(p_location, t_transform);		
		t_transform . tx = modff(t_device_location . x, &t_device_location . x);
		t_transform . ty = modff(t_device_location . y, &t_device_location . y);
		
		MCGRectangle t_device_clip;
		t_device_clip = MCGContextGetDeviceClipBounds(self);
		t_device_clip . origin . x -= t_device_location . x;
		t_device_clip . origin . y -= t_device_location . y;
		
		MCGRectangle t_float_clipped_bounds;
		t_float_text_bounds = MCGRectangleApplyAffineTransform(t_float_text_bounds, t_transform);		
		t_float_clipped_bounds = MCGRectangleIntersection(t_float_text_bounds, t_device_clip);
		
		t_text_bounds = MCGRecangleIntegerBounds(t_float_text_bounds);
		t_clipped_bounds = MCGRecangleIntegerBounds(t_float_clipped_bounds);
		
		if (t_clipped_bounds . width == 0 || t_clipped_bounds . height == 0)
		{
			DeleteDC(t_gdicontext);
			return;
		}
	}
	
	if (t_success)
	{
		XFORM t_xform;
		t_xform . eM11 = t_transform . a;
		t_xform . eM12 = t_transform . b;
		t_xform . eM21 = t_transform . c;
		t_xform . eM22 = t_transform . d;
		t_xform . eDx = t_transform . tx - (t_clipped_bounds . x - t_text_bounds . x);
		t_xform . eDy = t_transform . ty - (t_clipped_bounds . y - t_text_bounds . y);
		t_success = SetWorldTransform(t_gdicontext, &t_xform);
	}
	
	void *t_dib_data;
	t_dib_data = NULL;
	HBITMAP t_gdibitmap;
	t_gdibitmap = NULL;
	if (t_success)
	{
		BITMAPINFO t_bitmapinfo;
		MCMemoryClear(&t_bitmapinfo, sizeof(BITMAPINFO));
		t_bitmapinfo . bmiHeader . biSize = sizeof(BITMAPINFOHEADER);
		t_bitmapinfo . bmiHeader . biCompression = BI_RGB;
		t_bitmapinfo . bmiHeader . biWidth = t_clipped_bounds . width;
		t_bitmapinfo . bmiHeader . biHeight = -t_clipped_bounds . height;
		t_bitmapinfo . bmiHeader . biPlanes = 1;
		t_bitmapinfo . bmiHeader . biBitCount = 32;
		t_gdibitmap = CreateDIBSection(t_gdicontext, &t_bitmapinfo, DIB_RGB_COLORS, &t_dib_data, NULL, 0);
		t_success = t_gdibitmap != NULL && t_dib_data != NULL;
	}
	
	if (t_success)
		t_success = SelectObject(t_gdicontext, t_gdibitmap) != NULL;
	
	if (t_success)
	{
		MCMemoryClear(t_dib_data, t_clipped_bounds . width * t_clipped_bounds . height * 4);		
		SetTextColor(t_gdicontext, 0x00FFFFFF);
		SetBkColor(t_gdicontext, 0x00000000);
		SetBkMode(t_gdicontext, OPAQUE);		
		t_success = TextOutW(t_gdicontext, 0, 0, (LPCWSTR)p_text, p_length >> 1);
	}
	
	if (t_success)
		t_success = GdiFlush();
	
	void *t_data;
	t_data = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_clipped_bounds . width * t_clipped_bounds . height, t_data);
	
	if (t_success)
	{
		uint32_t *t_src_data;
		t_src_data = (uint32_t *) t_dib_data;
		
		uint8_t *t_dst_data;
		t_dst_data = (uint8_t *) t_data;
		
		for(uint32_t y = 0; y < t_clipped_bounds . height; y++)
		{
			for(uint32_t x = 0; x < t_clipped_bounds . width; x++)
			{
				uint32_t t_lcd32_val;
				t_lcd32_val = *(t_src_data + y * t_clipped_bounds . width + x);		
				
				uint8_t t_a8_val;
				t_a8_val = ((t_lcd32_val & 0xFF) + ((t_lcd32_val & 0xFF00) >> 8) + ((t_lcd32_val & 0xFF0000) >> 16)) / 3;
				
				*(t_dst_data + y * t_clipped_bounds . width + x) = t_a8_val;
			}
		}
		
		SkPaint t_paint;
		t_paint . setStyle(SkPaint::kFill_Style);	
		t_paint . setAntiAlias(self -> state -> should_antialias);
		t_paint . setColor(MCGColorToSkColor(self -> state -> fill_color));
		
		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
		t_paint . setXfermode(t_blend_mode);
		if (t_blend_mode != NULL)
			t_blend_mode -> unref();		
		
		SkBitmap t_bitmap;
		t_bitmap . setConfig(SkBitmap::kA8_Config, t_clipped_bounds . width, t_clipped_bounds . height);
		t_bitmap . setIsOpaque(false);
		t_bitmap . setPixels(t_data);
		self -> layer -> canvas -> drawSprite(t_bitmap, t_clipped_bounds . x + t_device_location . x, 
											  t_clipped_bounds . y + t_device_location . y, &t_paint);
	}
	
	MCMemoryDelete(t_data);
	DeleteObject(t_gdibitmap);	
	DeleteDC(t_gdicontext);
	self -> is_valid = t_success;
}

MCGFloat MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font)
{
	if (!MCGContextIsValid(self))
		return 0.0;	
	
	bool t_success;
	t_success = true;
	
	HDC t_gdicontext;
	t_gdicontext = NULL;
	if (t_success)
	{
		t_gdicontext = CreateCompatibleDC(NULL);
		t_success = t_gdicontext != NULL;
	}
	
	if (t_success)
		t_success = SetGraphicsMode(t_gdicontext, GM_ADVANCED) != 0;
	
	if (t_success)
		t_success = SelectObject(t_gdicontext, p_font . fid) != NULL;
	
	SIZE t_size;
	if (t_success)
		t_success = GetTextExtentPoint32W(t_gdicontext, (LPCWSTR)p_text, p_length >> 1, &t_size);
	
	DeleteDC(t_gdicontext);
	self -> is_valid = t_success;
	
	if (t_success)
		return t_size . cx;
	else
		return 0.0;
}

////////////////////////////////////////////////////////////////////////////////