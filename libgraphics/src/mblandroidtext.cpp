#include "graphics.h"
#include "graphics-internal.h"

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
	
	char *t_text;
	t_text = nil;
	if (t_success)
	{
		p_length = p_length / 2;
		t_success = MCCStringFromUnicodeSubstring(p_text, p_length, t_text);
	}
	
	if (t_success)
	{
		SkPaint t_paint;
		t_paint . setStyle(SkPaint::kFill_Style);	
		t_paint . setAntiAlias(true);
		t_paint . setColor(MCGColorToSkColor(self -> state -> fill_color));
		t_paint . setTextSize(p_font . size);
		
		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
		t_paint . setXfermode(t_blend_mode);
		if (t_blend_mode != NULL)
			t_blend_mode -> unref();
		
		SkTypeface *t_typeface;
		t_typeface = (SkTypeface *) p_font . fid;
		t_paint . setTypeface(t_typeface);
		
		self -> layer -> canvas -> drawText(t_text, p_length, MCGCoordToSkCoord(p_location . x), MCGCoordToSkCoord(p_location . y), t_paint);
	}
	
	MCCStringFree(t_text);
	self -> is_valid = t_success;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font)
{	
	//if (!MCGContextIsValid(self))
	//	return 0.0;
	
	bool t_success;
	t_success = true;	
	
	char *t_text;
	t_text = nil;
	if (t_success)
	{
		p_length = p_length / 2;
		t_success = MCCStringFromUnicodeSubstring(p_text, p_length, t_text);
	}
	
	MCGFloat t_width;
	t_width = 0.0;
	if (t_success)
	{
		SkPaint t_paint;
		t_paint . setTextSize(p_font . size);
		
		SkTypeface *t_typeface;
		t_typeface = (SkTypeface *) p_font . fid;
		t_paint . setTypeface(t_typeface);
		
		t_width =  (MCGFloat) t_paint . measureText(t_text, p_length);
	}
	
	MCCStringFree(t_text);
	//self -> is_valid = t_success;
	return t_width;
}

////////////////////////////////////////////////////////////////////////////////
