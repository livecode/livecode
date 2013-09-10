#include "graphics.h"
#include "graphics-internal.h"

#include <pango/pangoft2.h>
#include <glib.h>

////////////////////////////////////////////////////////////////////////////////

extern "C" int initialise_weak_link_pango();
extern "C" int initialise_weak_link_pangoft2();

static PangoFontMap *s_font_map = NULL;
static PangoContext *s_pango = NULL;
static PangoLayout *s_layout = NULL;

static bool lnx_pango_initialize(void)
{
	bool t_success;
	t_success = true;

	if (t_success)
		t_success == initialise_weak_link_pango() != 0 && initialise_weak_link_pangoft2() != 0;
	
	if (t_success)
		if (s_font_map == NULL)
		{
			s_font_map = pango_ft2_font_map_new();
			t_success = s_font_map != NULL;
		}
	
	if (t_success)
		if (s_pango == NULL)
		{
			s_pango = pango_ft2_font_map_create_context((PangoFT2FontMap *) s_font_map);
			t_success = s_pango != nil;
		}	
	
	if (t_success)		
		if (s_layout == NULL)
		{
			s_layout = pango_layout_new(s_pango);
			t_success = s_layout != NULL;
		}
	
	return t_success;
}

static void lnx_pango_finalize(void)
{
	if (s_layout != NULL)
		g_object_unref(s_layout);
	if (s_pango != NULL)
		g_object_unref(s_pango);
	if (s_font_map != NULL)
		g_object_unref(s_font_map);
}

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void)
{
	/* UNCHECKED */ lnx_pango_initialize();
}

void MCGPlatformFinalize(void)
{
	lnx_pango_finalize();
}
////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font)
{
	if (!MCGContextIsValid(self))
		return;	
	
	bool t_success;
	t_success = true;

	if (t_success)
		t_success = s_layout != NULL;
	
	char *t_text;
	t_text = nil;
	if (t_success)
		t_success = MCCStringFromUnicodeSubstring(p_text, p_length / 2, t_text);
	
	MCGAffineTransform t_transform;
	MCGPoint t_device_location;	
	PangoLayoutLine *t_line;
	t_line = nil;
	if (t_success)
	{
		t_transform = MCGContextGetDeviceTransform(self);
		t_device_location = MCGPointApplyAffineTransform(p_location, t_transform);		
		t_transform . tx = modff(t_device_location . x, &t_device_location . x);
		t_transform . ty = modff(t_device_location . y, &t_device_location . y);		
		
		PangoMatrix t_ptransform;		
		t_ptransform . xx = t_transform . a;
		t_ptransform . xy = t_transform . b;
		t_ptransform . yx = t_transform . c;
		t_ptransform . yy = t_transform . d;
		t_ptransform . x0 = t_transform . tx;
		t_ptransform . y0 = t_transform . ty;
		pango_context_set_matrix(s_pango, &t_ptransform);
		
		pango_layout_set_font_description(s_layout, (PangoFontDescription *) p_font . fid);
		pango_layout_set_text(s_layout, t_text, -1);
		MCCStringFree(t_text);
		
		extern PangoLayoutLine *(*pango_layout_get_line_readonly_ptr)(PangoLayout *, int);
		if (pango_layout_get_line_readonly_ptr != nil)
			t_line = pango_layout_get_line_readonly_ptr(s_layout, 0);
		else
			t_line = pango_layout_get_line(s_layout, 0);
		t_success = t_line != nil;
	}
	
	
	MCGIntRectangle t_text_bounds, t_clipped_bounds;
	if (t_success)
	{
		PangoRectangle t_pbounds;
		pango_layout_line_get_extents(t_line, NULL, &t_pbounds);
		
		MCGRectangle t_float_text_bounds;
        t_float_text_bounds . origin . x = t_pbounds . x / PANGO_SCALE;
        t_float_text_bounds . origin . y = t_pbounds . y / PANGO_SCALE;
        t_float_text_bounds . size . width = t_pbounds . width / PANGO_SCALE;
        t_float_text_bounds . size . height = t_pbounds . height / PANGO_SCALE;		
		
		MCGRectangle t_device_clip;
		t_device_clip = MCGContextGetDeviceClipBounds(self);
		t_device_clip . origin . x -= t_device_location . x;
		t_device_clip . origin . y -= t_device_location . y;
		
		MCGRectangle t_float_clipped_bounds;
		t_float_clipped_bounds = MCGRectangleIntersection(t_float_text_bounds, t_device_clip);
		
		t_text_bounds = MCGRectangleIntegerBounds(t_float_text_bounds);
		t_clipped_bounds = MCGRectangleIntegerBounds(t_float_clipped_bounds);
		
		if (t_clipped_bounds . width == 0 || t_clipped_bounds . height == 0)
			return;
	}
	
	void *t_data;
	t_data = nil;
	if (t_success)
		t_success = MCMemoryNew(t_clipped_bounds . width * t_clipped_bounds . height, t_data);
	
	if (t_success)
	{
		FT_Bitmap t_ftbitmap;
		t_ftbitmap . rows = t_clipped_bounds . height;
		t_ftbitmap . width = t_clipped_bounds . width;
		t_ftbitmap . pitch = t_clipped_bounds . width;
		t_ftbitmap . buffer = (unsigned char*) t_data;
		t_ftbitmap . num_grays = 256;
		t_ftbitmap . pixel_mode = FT_PIXEL_MODE_GRAY;
		t_ftbitmap . palette_mode = 0;
		t_ftbitmap . palette = nil;
		
		pango_ft2_render_layout_line(&t_ftbitmap, t_line, -(t_clipped_bounds . x - t_text_bounds . x), -(t_clipped_bounds . y - t_text_bounds . y) - t_text_bounds . y);
		
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
		t_bitmap . setConfig(SkBitmap::kA8_Config, t_clipped_bounds . width, t_clipped_bounds .  height);
		t_bitmap . setIsOpaque(false);
		t_bitmap . setPixels(t_data);
		self -> layer -> canvas -> drawSprite(t_bitmap, t_clipped_bounds . x + t_device_location . x, 
											  t_clipped_bounds . y + t_device_location . y, &t_paint);
	}
	
	MCMemoryDelete(t_data);
	self -> is_valid = t_success;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font)
{	
	//if (!MCGContextIsValid(self))
	//	return 0.0;
	
	bool t_success;
	t_success = true;		
	
	if (t_success)
		t_success = s_layout != NULL;
	
	char *t_text;
	t_text = nil;
	if (t_success)
		t_success = MCCStringFromUnicodeSubstring(p_text, p_length / 2, t_text);
	
	MCGFloat t_width;
	t_width = 0.0;
	if (t_success)
	{
		pango_layout_set_text(s_layout, t_text, -1);		
		pango_layout_set_font_description(s_layout, (PangoFontDescription *) p_font . fid);
		
		PangoRectangle t_bounds;
		pango_layout_get_pixel_extents(s_layout, NULL, &t_bounds);
		t_width = t_bounds . width;		
	}
	
	MCCStringFree(t_text);
	//self -> is_valid = t_success;

	return t_width;
}

////////////////////////////////////////////////////////////////////////////////
