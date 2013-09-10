#include "graphics.h"
#include "graphics-internal.h"

#include "SkDevice.h"

////////////////////////////////////////////////////////////////////////////////

static inline uint32_t packed_bilinear_bounded(uint32_t x, uint8_t a, uint32_t y, uint8_t b)
{
	uint32_t u, v;

	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

////////////////////////////////////////////////////////////////////////////////

static bool w32_draw_text_using_mask_to_context_at_device_location(MCGContextRef p_context, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, MCGIntRectangle p_bounds, HDC p_gdicontext)
{
	bool t_success;
	t_success = true;

	void *t_rgb_data;
	t_rgb_data = NULL;
	HBITMAP t_rgb_bitmap;
	t_rgb_bitmap = NULL;
	if (t_success)
	{
		BITMAPINFO t_bitmapinfo;
		MCMemoryClear(&t_bitmapinfo, sizeof(BITMAPINFO));
		t_bitmapinfo . bmiHeader . biSize = sizeof(BITMAPINFOHEADER);
		t_bitmapinfo . bmiHeader . biCompression = BI_RGB;
		t_bitmapinfo . bmiHeader . biWidth = p_bounds . width;
		t_bitmapinfo . bmiHeader . biHeight = -p_bounds . height;
		t_bitmapinfo . bmiHeader . biPlanes = 1;
		t_bitmapinfo . bmiHeader . biBitCount = 32;
		t_rgb_bitmap = CreateDIBSection(p_gdicontext, &t_bitmapinfo, DIB_RGB_COLORS, &t_rgb_data, NULL, 0);
		t_success = t_rgb_bitmap != NULL && t_rgb_data != NULL;
	}

	HGDIOBJ t_old_object;
	if (t_success)
	{
		t_old_object = SelectObject(p_gdicontext, t_rgb_bitmap);
		t_success = t_old_object != NULL;
	}

	bool t_nearest_white;
	if (t_success)
		t_nearest_white = (0.3 * (p_context -> state -> fill_color & 0xff) + 0.59 * ((p_context -> state -> fill_color >> 8) & 0xff) + 0.11 * ((p_context -> state -> fill_color >> 16) & 0xff)) >= 127.0;

	uint32_t *t_rgb_pxls, *t_context_pxls;
	uint32_t t_context_width;
	int32_t t_x_offset, t_y_offset;
	if (t_success)
	{
		t_rgb_pxls = (uint32_t *) t_rgb_data;

		t_context_pxls = (uint32_t *) p_context -> layer -> canvas -> getTopDevice() -> accessBitmap(false) . getPixels();
		t_context_width = p_context -> layer -> canvas -> getTopDevice() -> accessBitmap(false) . rowBytes() / 4;	

		t_x_offset = p_location . x + p_bounds . x;
		t_y_offset = p_location . y + p_bounds . y;

		for (uint32_t y = 0; y < p_bounds . height; y++)
		{
			for (uint32_t x = 0; x < p_bounds . width; x++)
			{
				uint32_t t_context_pxl;
				t_context_pxl = *(t_context_pxls + (y + t_y_offset) * t_context_width + x + t_x_offset);
				if (((t_context_pxl >> 24) & 0xFF) == 0xFF)
					*(t_rgb_pxls + y * p_bounds . width + x) = t_context_pxl;
				else
					*(t_rgb_pxls + y * p_bounds . width + x) = t_nearest_white ? 0x000000 : 0xffffff;
			}
		}

		SetBkMode(p_gdicontext, TRANSPARENT);
		SetTextColor(p_gdicontext, RGB((p_context -> state -> fill_color >> 16) & 0xFF, (p_context -> state -> fill_color >> 8) & 0xFF, (p_context -> state -> fill_color >> 0) & 0xFF));
		t_success = TextOutW(p_gdicontext, 0, 0, (LPCWSTR)p_text, p_length >> 1);
	}

	if (t_success)
		t_success = GdiFlush();

	if (t_success)
	{
		for (uint32_t y = 0; y < p_bounds . height; y++)
		{
			for (uint32_t x = 0; x < p_bounds . width; x++)
			{
				uint32_t *t_context_pxl;
				t_context_pxl = t_context_pxls + (y + t_y_offset) * t_context_width + x + t_x_offset;
				uint32_t *t_rgb_pxl;
				t_rgb_pxl = t_rgb_pxls + y * p_bounds . width + x;
				if (((*t_context_pxl >> 24) & 0xFF) == 0xFF)
					*t_rgb_pxl = *t_rgb_pxl | 0xFF000000;
				else
				{
					uint32_t t_lcd32_val;
					t_lcd32_val = *(t_rgb_pxls + y * p_bounds . width + x);
					
					uint8_t t_a8_val;
					t_a8_val = ((t_lcd32_val & 0xFF) + ((t_lcd32_val & 0xFF00) >> 8) + ((t_lcd32_val & 0xFF0000) >> 16)) / 3;
					
					if (!t_nearest_white)
						t_a8_val = 255 - t_a8_val;

					*t_rgb_pxl = packed_bilinear_bounded(*t_context_pxl, 255 - t_a8_val, p_context -> state -> fill_color, t_a8_val);
				}
			}
		}

		SkPaint t_paint;
		t_paint . setStyle(SkPaint::kFill_Style);	
		t_paint . setAntiAlias(p_context -> state -> should_antialias);
		t_paint . setColor(MCGColorToSkColor(p_context -> state -> fill_color));
		
		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(p_context -> state -> blend_mode);
		t_paint . setXfermode(t_blend_mode);
		if (t_blend_mode != NULL)
			t_blend_mode -> unref();		
		
		SkBitmap t_bitmap;
		t_bitmap . setConfig(SkBitmap::kARGB_8888_Config, p_bounds . width, p_bounds . height);
		t_bitmap . setIsOpaque(false);
		t_bitmap . setPixels(t_rgb_data);
		p_context -> layer -> canvas -> drawSprite(t_bitmap, p_bounds . x + p_location . x, p_bounds . y + p_location . y, &t_paint);
	}

	if (t_rgb_bitmap != NULL)
	{
		SelectObject(p_gdicontext, t_old_object);
		DeleteObject(t_rgb_bitmap);
	}

	return t_success;
}

static bool w32_draw_text_to_context_at_device_location(MCGContextRef p_context, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, MCGIntRectangle p_bounds, HDC p_gdicontext)
{
	bool t_success;
	t_success = true;

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
		t_bitmapinfo . bmiHeader . biWidth = p_bounds . width;
		t_bitmapinfo . bmiHeader . biHeight = -p_bounds . height;
		t_bitmapinfo . bmiHeader . biPlanes = 1;
		t_bitmapinfo . bmiHeader . biBitCount = 32;
		t_gdibitmap = CreateDIBSection(p_gdicontext, &t_bitmapinfo, DIB_RGB_COLORS, &t_dib_data, NULL, 0);
		t_success = t_gdibitmap != NULL && t_dib_data != NULL;
	}

	if (t_success)
		t_success = SelectObject(p_gdicontext, t_gdibitmap) != NULL;	

	if (t_success)
	{
		SetTextColor(p_gdicontext, 0x00000000);
		SetBkColor(p_gdicontext, 0x00FFFFFF);
		SetBkMode(p_gdicontext, OPAQUE);		
		t_success = TextOutW(p_gdicontext, 0, 0, (LPCWSTR)p_text, p_length >> 1);
	}
	
	if (t_success)
		t_success = GdiFlush();
	
	void *t_data;
	t_data = NULL;
	if (t_success)
		t_success = MCMemoryNew(p_bounds . width * p_bounds . height, t_data);
	
	if (t_success)
	{
		uint32_t *t_src_data;
		t_src_data = (uint32_t *) t_dib_data;
		
		uint8_t *t_dst_data;
		t_dst_data = (uint8_t *) t_data;
		
		for (uint32_t y = 0; y < p_bounds . height; y++)
		{
			for (uint32_t x = 0; x < p_bounds . width; x++)
			{
				uint32_t t_lcd32_val;
				t_lcd32_val = *(t_src_data + y * p_bounds . width + x);		
				
				uint8_t t_a8_val;
				t_a8_val = ((t_lcd32_val & 0xFF) + ((t_lcd32_val & 0xFF00) >> 8) + ((t_lcd32_val & 0xFF0000) >> 16)) / 3;
				
				*(t_dst_data + y * p_bounds . width + x) = 255 - t_a8_val;
			}
		}
		
		SkPaint t_paint;
		t_paint . setStyle(SkPaint::kFill_Style);	
		t_paint . setAntiAlias(p_context -> state -> should_antialias);
		t_paint . setColor(MCGColorToSkColor(p_context -> state -> fill_color));
		
		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(p_context -> state -> blend_mode);
		t_paint . setXfermode(t_blend_mode);
		if (t_blend_mode != NULL)
			t_blend_mode -> unref();		
		
		SkBitmap t_bitmap;
		t_bitmap . setConfig(SkBitmap::kA8_Config, p_bounds . width, p_bounds . height);
		t_bitmap . setIsOpaque(false);
		t_bitmap . setPixels(t_data);
		p_context -> layer -> canvas -> drawSprite(t_bitmap, p_bounds . x + p_location . x, 
											  p_bounds . y + p_location . y, &t_paint);
	}
	
	DeleteObject(t_gdibitmap);
	MCMemoryDelete(t_data);
	return t_success;
}

static bool w32_draw_opaque_text_to_context_at_device_location(MCGContextRef p_context, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, MCGIntRectangle p_bounds, HDC p_gdicontext)
{
	bool t_success;
	t_success = true;

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
		t_bitmapinfo . bmiHeader . biWidth = p_bounds . width;
		t_bitmapinfo . bmiHeader . biHeight = -p_bounds . height;
		t_bitmapinfo . bmiHeader . biPlanes = 1;
		t_bitmapinfo . bmiHeader . biBitCount = 32;
		t_gdibitmap = CreateDIBSection(p_gdicontext, &t_bitmapinfo, DIB_RGB_COLORS, &t_dib_data, NULL, 0);
		t_success = t_gdibitmap != NULL && t_dib_data != NULL;
	}

	if (t_success)
		t_success = SelectObject(p_gdicontext, t_gdibitmap) != NULL;	

	uint32_t *t_context_pxls, *t_dib_pxls;
	uint32_t t_context_width, t_x_offset, t_y_offset;
	if (t_success)
	{
		t_context_pxls = (uint32_t *) p_context -> layer -> canvas -> getTopDevice() -> accessBitmap(true) . getPixels();
		t_dib_pxls = (uint32_t *) t_dib_data;

		t_context_width = p_context -> layer-> canvas -> getTopDevice() -> accessBitmap(false) . rowBytes() / 4;		
		t_x_offset = p_location . x + p_bounds . x;
		t_y_offset = p_location . y + p_bounds . y;

		for (uint32_t y = 0; y < p_bounds . height; y++)
			for (uint32_t x = 0; x < p_bounds . width; x++)
				*(t_dib_pxls + y * p_bounds . width + x) = *(t_context_pxls + (y + t_y_offset) * t_context_width + x + t_x_offset);

		SetBkMode(p_gdicontext, TRANSPARENT);
		SetTextColor(p_gdicontext, RGB((p_context -> state -> fill_color >> 16) & 0xFF, (p_context -> state -> fill_color >> 8) & 0xFF, (p_context -> state -> fill_color >> 0) & 0xFF));
		t_success = TextOutW(p_gdicontext, 0, 0, (LPCWSTR)p_text, p_length >> 1);
	}	

	if (t_success)
		t_success = GdiFlush();

	if (t_success)
	{
		for (uint32_t y = 0; y < p_bounds . height; y++)
			for (uint32_t x = 0; x < p_bounds . width; x++)
				*(t_context_pxls + (y + t_y_offset) * t_context_width + x + t_x_offset) = *(t_dib_pxls + y * p_bounds . width + x) | 0xFF000000;
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void)
{
}

void MCGPlatformFinalize(void)
{
}

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
		
		// IM-2013-09-02: [[ RefactorGraphics ]] modff will round to zero rather than
		// negative infinity (which we need) so calculate manually
		MCGFloat t_int_x, t_int_y;
		t_int_x = floor(t_device_location.x);
		t_int_y = floor(t_device_location.y);
		
		t_transform . tx = t_device_location . x - t_int_x;
		t_transform . ty = t_device_location . y - t_int_y;
		t_device_location . x = t_int_x;
		t_device_location . y = t_int_y;
		
		// IM-2013-09-02: [[ RefactorGraphics ]] constrain device clip rect to device bounds
		SkISize t_device_size;
		t_device_size = self->layer->canvas->getDeviceSize();
		
		MCGRectangle t_device_bounds;
		t_device_bounds = MCGRectangleMake(0, 0, t_device_size.width(), t_device_size.height());
		
		MCGRectangle t_device_clip;
		t_device_clip = MCGRectangleIntersection(MCGContextGetDeviceClipBounds(self), t_device_bounds);
		
		t_device_clip . origin . x -= t_device_location . x;
		t_device_clip . origin . y -= t_device_location . y;
		
		MCGRectangle t_float_clipped_bounds;
		t_float_text_bounds = MCGRectangleApplyAffineTransform(t_float_text_bounds, t_transform);		
		t_float_clipped_bounds = MCGRectangleIntersection(t_float_text_bounds, t_device_clip);
		
		t_text_bounds = MCGRectangleIntegerBounds(t_float_text_bounds);
		t_clipped_bounds = MCGRectangleIntegerBounds(t_float_clipped_bounds);
		
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

	if (t_success)
		t_success = w32_draw_text_using_mask_to_context_at_device_location(self, p_text, p_length, t_device_location, t_clipped_bounds, t_gdicontext);
		//t_success = w32_draw_opaque_text_to_context_at_device_location(self, p_text, p_length, t_device_location, t_clipped_bounds, t_gdicontext);
		//t_success = w32_draw_text_to_context_at_device_location(self, p_text, p_length, t_device_location, t_clipped_bounds, t_gdicontext);

	DeleteDC(t_gdicontext);
	self -> is_valid = t_success;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font)
{
	//if (!MCGContextIsValid(self))
	//	return 0.0;	
	
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
	//self -> is_valid = t_success;
	
	if (t_success)
		return t_size . cx;
	else
		return 0.0;
}

////////////////////////////////////////////////////////////////////////////////