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

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"


#include "font.h"
#include "util.h"

#include "globals.h"

#include "lnxdc.h"
#include "lnxflst.h"
#include "packed.h"
#include "platform.h"

#include <pango/pangoft2.h>
#include <glib.h>

////////////////////////////////////////////////////////////////////////////////

// IMPORTANT: Pango (like GTK) will call exit() on memory allocation failure.
//   For this reason we do not check return values, as any errors will terminate
//   the engine.
// TODO: We need to revise the situation periodically and implement
//   a fix should this become feasable

////////////////////////////////////////////////////////////////////////////////

extern "C" int initialise_weak_link_xft();
extern "C" int initialise_weak_link_pango();
extern "C" int initialise_weak_link_pangoxft();
extern "C" int initialise_weak_link_pangoft2();
extern "C" int initialise_weak_link_pangocairo();
extern "C" int initialise_weak_link_glib();
extern "C" int initialise_weak_link_gobject();

////////////////////////////////////////////////////////////////////////////////

class MCNewFontlist: public MCFontlist
{
public:
	MCNewFontlist();
	virtual ~MCNewFontlist();

	virtual bool create(void);
	virtual void destroy(void);

	virtual MCFontStruct *getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer);
	virtual bool getfontnames(MCStringRef p_type, MCListRef& r_names);
	virtual bool getfontsizes(MCStringRef p_fname, MCListRef& r_sizes);
	virtual bool getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles);
	virtual bool getfontstructinfo(MCNameRef& r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font);
	virtual void getfontreqs(MCFontStruct *f, MCNameRef& r_name, uint2& r_size, uint2& r_style);

	virtual int4 ctxt_textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override);
	virtual bool ctxt_layouttext(const unichar_t *chars, uint32_t char_count, MCFontStruct *font, MCTextLayoutCallback callback, void *context);
	
private:
	MCNewFontStruct *m_fonts;

	PangoContext *m_pango;
	PangoLayout *m_layout;
	PangoFontMap *m_font_map;
};

////////////////////////////////////////////////////////////////////////////////

MCNewFontlist::MCNewFontlist()
{
	m_font_map = nil;
	m_pango = nil;
	m_layout = nil;
	m_fonts = nil;
}

MCNewFontlist::~MCNewFontlist()
{
    // The font map object is owned by Pango so we don't release it
    
    if (m_layout != nil)
		g_object_unref(m_layout);
	if (m_pango != nil)
		g_object_unref(m_pango);

	while (m_fonts)
    {
		MCNewFontStruct* t_font = m_fonts;
        m_fonts = t_font -> next;
        MCValueRelease(t_font -> family);
		pango_font_description_free(t_font -> description);
		delete t_font;
	}
}

bool MCNewFontlist::create(void)
{
#ifdef _SERVER
	// Unlike on desktop, the server build does not require GLib to run so it
	// needs to be loaded here
	if (!initialise_weak_link_glib() ||
		!initialise_weak_link_gobject() ||
		!initialise_weak_link_pango() ||
		!initialise_weak_link_pangoft2() ||
		!initialise_weak_link_pangocairo())
		return false;
#else
	if (initialise_weak_link_pango() == 0 ||
        initialise_weak_link_pangocairo() == 0 ||
		initialise_weak_link_pangoft2() == 0)
		return false;
	
#endif
	
    // Note that we do not own this font map and should not release it
    m_font_map = pango_cairo_font_map_get_default();
	if (m_font_map == nil)
		return false;
	
    m_pango = pango_context_new();
	if (m_pango == nil)
		return false;
    
    pango_context_set_font_map(m_pango, m_font_map);

	m_layout = pango_layout_new(m_pango);
	if (m_layout == nil)
		return false;

	return true;
}

void MCNewFontlist::destroy(void)
{
	delete this;
}

////////////////////////////////////////////////////////////////////////////////

MCFontStruct *MCNewFontlist::getfont(MCNameRef p_family, uint2& p_size, uint2 p_style, Boolean p_printer)
{
	MCNewFontStruct *t_font;
	for(t_font = m_fonts; t_font != nil; t_font = t_font -> next)
		if (MCNameIsEqualToCaseless(p_family, t_font -> family) && p_size == t_font -> size && p_style == t_font -> style)
			return t_font;

	t_font = new (nothrow) MCNewFontStruct;
	t_font -> family = MCValueRetain(p_family);
	t_font -> size = p_size;
	t_font -> style = p_style;
	t_font -> next = m_fonts;
	m_fonts = t_font;

    // If the font name identifies one of the special fonts, resolve it
    MCAutoStringRef t_family_name;
    if (MCNameIsEqualToCaseless(p_family, MCN_font_usertext))
        MCPlatformGetControlThemePropString(kMCPlatformControlTypeInputField, kMCPlatformControlPartNone, kMCPlatformControlStateNormal, kMCPlatformThemePropertyTextFont, &t_family_name);
    else if (MCNameIsEqualToCaseless(p_family, MCN_font_menutext))
        MCPlatformGetControlThemePropString(kMCPlatformControlTypeMenu, kMCPlatformControlPartNone, kMCPlatformControlStateNormal, kMCPlatformThemePropertyTextFont, &t_family_name);
    else if (MCNameIsEqualToCaseless(p_family, MCN_font_content))
        MCPlatformGetControlThemePropString(kMCPlatformControlTypeInputField, kMCPlatformControlPartNone, kMCPlatformControlStateNormal, kMCPlatformThemePropertyTextFont, &t_family_name);
    else if (MCNameIsEqualToCaseless(p_family, MCN_font_message))
        MCPlatformGetControlThemePropString(kMCPlatformControlTypeButton, kMCPlatformControlPartNone, kMCPlatformControlStateNormal, kMCPlatformThemePropertyTextFont, &t_family_name);
    else if (MCNameIsEqualToCaseless(p_family, MCN_font_tooltip))
        MCPlatformGetControlThemePropString(kMCPlatformControlTypeLabel, kMCPlatformControlPartNone, kMCPlatformControlStateNormal, kMCPlatformThemePropertyTextFont, &t_family_name);
    else if (MCNameIsEqualToCaseless(p_family, MCN_font_system))
        MCPlatformGetControlThemePropString(kMCPlatformControlTypeGeneric, kMCPlatformControlPartNone, kMCPlatformControlStateNormal, kMCPlatformThemePropertyTextFont, &t_family_name);
    else
    {
        uindex_t t_offset;
        MCStringRef t_family_string = MCNameGetString(p_family);
        if (MCStringFirstIndexOfChar(t_family_string, ',', 0, kMCStringOptionCompareExact, t_offset))
        {
            MCStringCopySubstring(t_family_string, MCRangeMake(0, t_offset), &t_family_name);
        }
        else
        {
            t_family_name = t_family_string;
        }
    }

	t_font -> description = pango_font_description_new();
    MCAutoStringRefAsSysString t_font_family;
    /* UNCHECKED */ t_font_family.Lock(*t_family_name);
    pango_font_description_set_family(t_font -> description, *t_font_family);
	pango_font_description_set_absolute_size(t_font -> description, p_size * PANGO_SCALE);
	if ((p_style & (FA_ITALIC | FA_OBLIQUE)) != 0)
		pango_font_description_set_style(t_font -> description, PANGO_STYLE_ITALIC);
	if ((p_style & FA_WEIGHT) > 0x05)
		pango_font_description_set_weight(t_font -> description, PANGO_WEIGHT_BOLD);
	if ((p_style & FA_EXPAND) > 0x50)
		pango_font_description_set_stretch(t_font -> description, PANGO_STRETCH_EXPANDED);
	else if ((p_style & FA_EXPAND) < 0x50)
		pango_font_description_set_stretch(t_font -> description, PANGO_STRETCH_CONDENSED);

	PangoFont *t_p_font;
	t_p_font = pango_context_load_font(m_pango, t_font -> description);

	FT_Face t_face;
	t_face = pango_fc_font_lock_face((PangoFcFont*)t_p_font);

	int32_t t_ascent, t_descent, t_height;
	t_ascent = t_face -> size -> metrics . ascender / 64;
	t_descent = t_face -> size -> metrics . descender / 64;
	t_height = t_face -> size -> metrics . height / 64.0;

    t_font -> m_ascent = t_face -> size -> metrics.ascender / 64.0f;
    t_font -> m_descent = t_face -> size -> metrics.descender / -64.0f; // Note: descender is negative in FT!
    t_font -> m_leading = (t_face -> size -> metrics.height / 64.0f) - t_font -> m_ascent - t_font -> m_descent;

    // Guess the x-height based on the strikethrough position
    PangoFontDescription* t_desc;
    PangoFontMetrics* t_metrics;
    int t_strikepos, t_strikewidth;
    t_desc = pango_font_describe(t_p_font);
    t_metrics = pango_context_get_metrics(m_pango, t_desc, NULL);
    t_strikepos = pango_font_metrics_get_strikethrough_position(t_metrics);
    t_strikewidth = pango_font_metrics_get_underline_thickness(t_metrics);
    pango_font_metrics_unref(t_metrics);
    pango_font_description_free(t_desc);
    t_font -> m_xheight = 2 * (float(t_strikepos - t_strikewidth/2) / PANGO_SCALE);
    
	pango_fc_font_unlock_face((PangoFcFont*)t_p_font);

	g_object_unref(t_p_font);

	return t_font;
}

bool MCNewFontlist::getfontnames(MCStringRef p_type, MCListRef& r_names)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return true;

	bool t_success = true;

	PangoFontFamily **t_families;
	int t_family_count;
	pango_context_list_families(m_pango, &t_families, &t_family_count);
	for(int i = 0; t_success && i < t_family_count; i++)
		t_success = MCListAppendCString(*t_list, pango_font_family_get_name(t_families[i]));

	g_free(t_families);

	if (t_success)
		t_success = MCListCopy(*t_list, r_names);
	return t_success;
}

bool MCNewFontlist::getfontsizes(MCStringRef p_fname, MCListRef& r_sizes)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	bool t_success = true;

	PangoFontFamily **t_families;
	int t_family_count;
	pango_context_list_families(m_pango, &t_families, &t_family_count);
	for(int i = 0; t_success && i < t_family_count; i++)
		if (MCStringIsEqualToCString(p_fname, pango_font_family_get_name(t_families[i]), kMCCompareCaseless))
		{
			PangoFontFace **t_faces;
			int t_face_count;
			pango_font_family_list_faces(t_families[i], &t_faces, &t_face_count);
			if (t_face_count > 0)
			{
				int *t_sizes;
				int t_size_count;
				pango_font_face_list_sizes(t_faces[0], &t_sizes, &t_size_count);
				if (t_sizes == nil)
					t_success = MCListAppendInteger(*t_list, 0);
				else
				{
					for(int i = 0; t_success && i < t_size_count; i++)
						t_success = MCListAppendInteger(*t_list, t_sizes[i] / PANGO_SCALE);
					g_free(t_sizes);
				}
				g_free(t_faces);
			}
			break;
		}
	g_free(t_families);

	if (t_success)
		t_success = MCListCopy(*t_list, r_sizes);
	return t_success;
}

bool MCNewFontlist::getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
	bool t_success = true;
	PangoFontFamily **t_families;
	int t_family_count;
	pango_context_list_families(m_pango, &t_families, &t_family_count);
	for(int i = 0; t_success && i < t_family_count; i++)
		if (MCStringIsEqualToCString(p_fname, pango_font_family_get_name(t_families[i]), kMCCompareCaseless))
		{
			PangoFontFace **t_faces;
			int t_face_count;
			pango_font_family_list_faces(t_families[i], &t_faces, &t_face_count);
			for(int i = 0; t_success && i < t_face_count; i++)
				t_success = MCListAppendCString(*t_list, pango_font_face_get_face_name(t_faces[i]));
			g_free(t_faces);
		}

	g_free(t_families);
	if (t_success)
		t_success = MCListCopy(*t_list, r_styles);
	return t_success;
}

bool MCNewFontlist::getfontstructinfo(MCNameRef &r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
{
	MCNewFontStruct *t_font;
	t_font = static_cast<MCNewFontStruct *>(p_font);
	r_name = t_font -> family;
	r_size = t_font -> size;
	r_style = t_font -> style;
	r_printer = False;
	return true;
}

void MCNewFontlist::getfontreqs(MCFontStruct *p_font, MCNameRef& r_name, uint2& r_size, uint2& r_style)
{
	MCNewFontStruct *t_font;
	t_font = static_cast<MCNewFontStruct *>(p_font);
	r_name = t_font -> family;
	r_size = t_font -> size;
	r_style = t_font -> style;
}

int4 MCNewFontlist::ctxt_textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override)
{
	bool t_is_unicode;
	t_is_unicode = p_unicode_override;

	char *t_utf8;
	if (t_is_unicode)
	{
		if (!MCCStringFromUnicodeSubstring((const unichar_t *)s, l / 2, t_utf8))
			return 0;
	}
	else
	{
		if (!MCCStringFromNativeSubstring(s, l, t_utf8))
			return 0;
	}
	pango_layout_set_text(m_layout, t_utf8, -1);
	MCCStringFree(t_utf8);

	pango_layout_set_font_description(m_layout, static_cast<MCNewFontStruct *>(f) -> description);

	// We want the logical size for textwidth.
	PangoRectangle t_logical;
	pango_layout_get_pixel_extents(m_layout, nil, &t_logical);

	return t_logical . width;
}

bool MCNewFontlist::ctxt_layouttext(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context)
{
	char *t_utf8;
	if (!MCCStringFromUnicodeSubstring(p_chars, p_char_count, t_utf8))
		return false;
	
	pango_layout_set_text(m_layout, t_utf8, -1);
	MCCStringFree(t_utf8);

	pango_layout_set_font_description(m_layout, static_cast<MCNewFontStruct *>(p_font) -> description);

	PangoLayoutIter *t_iter;
	t_iter = pango_layout_get_iter(m_layout);

	bool t_success;
	double t_running_x;
	t_running_x = 0.0;
	t_success = true;
	while(t_success)
	{
		PangoLayoutRun *t_run;
		t_run = pango_layout_iter_get_run(t_iter);
		if (t_run == nil)
			break;

		MCTextLayoutGlyph *t_glyphs = nullptr;
		if (!MCMemoryNewArray(t_run -> glyphs -> num_glyphs, t_glyphs))
			t_success = false;
        
		PangoFcFont *t_font;
		t_font = PANGO_FC_FONT(t_run -> item -> analysis . font);
		if (t_success)
			for(int i = 0; i < t_run -> glyphs -> num_glyphs; i++)
			{
				t_glyphs[i] . index = t_run -> glyphs -> glyphs[i] . glyph;
				t_glyphs[i] . x = t_running_x + (double)t_run -> glyphs -> glyphs[i] . geometry . x_offset / PANGO_SCALE;
				t_glyphs[i] . y = (double)t_run -> glyphs -> glyphs[i] . geometry . y_offset / PANGO_SCALE;
				t_running_x += (double)t_run -> glyphs -> glyphs[i] . geometry . width / PANGO_SCALE;
			}
        
        MCAutoStringRef t_utf_string;
		if (t_success)
            t_success = MCStringCreateWithBytes((char_t*)pango_layout_get_text(m_layout) + t_run -> item -> offset, t_run -> item -> length, kMCStringEncodingUTF8, false, &t_utf_string);
        
        MCAutoDataRef t_utf16_data;
        if (t_success)
            t_success = MCStringEncode(*t_utf_string, kMCStringEncodingUTF16, false, &t_utf16_data);
		
		// We must now compute the cluster information. Pango gives this to us
		// as an array of byte offsets into the UTF-8 string for each glyph.
		// So first, we create a map from UTF-8 byte to UTF-16 char, then use this
		// to build the cluster array.
		uint32_t *t_char_map;
		uint16_t *t_clusters;
		t_char_map = nil;
		t_clusters = nil;
		if (t_success)
			t_success =
				MCMemoryNewArray(t_run -> item -> length, t_char_map) &&
                MCMemoryNewArray(MCDataGetLength(*t_utf16_data) / 2, t_clusters);
		
		if (t_success)
		{
			const unichar_t *t_chars;
			uint32_t t_char_count;
            t_chars = (const unichar_t *)MCDataGetBytePtr(*t_utf16_data);
            t_char_count = MCDataGetLength(*t_utf16_data) / 2;
			
			const uint8_t *t_bytes;
			uint32_t t_byte_count;
			t_bytes = (const uint8_t *)pango_layout_get_text(m_layout) + t_run -> item -> offset;
			t_byte_count = t_run -> item -> length;
			
			
			// Compute the byte -> char mapping. Note that we can be sure that the
			// UTF-8 is well-formed since it was generated from UTF-16.
			uint32_t t_char_index, t_next_char_index;
			t_char_index = t_next_char_index = 0;
			for(uint32_t i = 0; i < t_byte_count; i++)
			{
				if ((t_bytes[i] & 0xC0) == 0x80)
				{
					// A non-lead byte.
					t_char_map[i] = t_char_index;
				}
				else
				{
					// A lead byte.
					t_char_index = t_next_char_index;
					t_char_map[i] = t_char_index;
					
					// Either one codepoint or two, depending on the type of lead
					// byte.
					if ((t_bytes[i] & 0xf8) != 0xf0)
						t_next_char_index += 1;
					else
						t_next_char_index += 2;
				}
			}
			
			// Now we have the byte -> char mapping we can compute the clusters.
			for(uint32_t i = 0; i < t_char_count; i++)
				t_clusters[i] = 65535;
			for(int i = 0; i < t_run -> glyphs -> num_glyphs; i++)
			{
				uint32_t t_byte_offset;
				t_byte_offset = t_run -> glyphs -> log_clusters[i];
				t_clusters[t_char_map[t_byte_offset]] = MCMin(t_clusters[t_char_map[t_byte_offset]], i);
			}
			for(uint32_t i = 1; i < t_char_count; i++)
				if (t_clusters[i] == 65535)
					t_clusters[i] = t_clusters[i - 1];

			// Now emit the span
			MCTextLayoutSpan t_span;
			t_span . chars = (const unichar_t *)t_chars;
			t_span . clusters = t_clusters;
			t_span . char_count = t_char_count;
			t_span . glyphs = t_glyphs;
			t_span . glyph_count = t_run -> glyphs -> num_glyphs;
			t_span . font = t_font;
			p_callback(p_context, &t_span);
		}

		MCMemoryDeleteArray(t_char_map);
		MCMemoryDeleteArray(t_clusters);
		MCMemoryDeleteArray(t_glyphs);

        if (t_success)
        {
            if (!pango_layout_iter_next_run(t_iter))
                break;
        }
	}
	
	pango_layout_iter_free(t_iter);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// MW-2013-12-19: [[ Bug 11559 ]] This is used if the necessary font libs
//   cannot be loaded.
class MCDummyFontlist: public MCFontlist
{
public:
	MCDummyFontlist() {}
	~MCDummyFontlist() {}
	
	virtual bool create(void) { return true; }
	virtual void destroy(void) {}
	
    virtual MCFontStruct *getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer) { return nil; }
    virtual bool getfontnames(MCStringRef p_type, MCListRef& r_names) { return false; }
    virtual bool getfontsizes(MCStringRef p_fname, MCListRef& r_sizes) { return false; }
    virtual bool getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles) { return false; }
    virtual bool getfontstructinfo(MCNameRef &r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font) { return false; }
    virtual void getfontreqs(MCFontStruct *f, MCNameRef & r_name, uint2& r_size, uint2& r_style) { r_name = MCValueRetain(kMCEmptyName); r_size = 0; r_style = 0; }
	
	virtual int4 ctxt_textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override) { return 0; }
	virtual bool ctxt_layouttext(const unichar_t *chars, uint32_t char_count, MCFontStruct *font, MCTextLayoutCallback callback, void *context) { return false; }
};

////////////////////////////////////////////////////////////////////////////////

MCFontlist *MCFontlistCreateNew(void)
{
	MCNewFontlist *t_fontlist;
	t_fontlist = new (nothrow) MCNewFontlist;
	if (!t_fontlist -> create())
	{
		delete t_fontlist;
		
		// MW-2013-12-19: [[ Bug 11559 ]] If we couldn't setup the proper fontlist
		//   then return a dummy one.
		return new MCDummyFontlist;
	}

	return t_fontlist;
}

////////////////////////////////////////////////////////////////////////////////
