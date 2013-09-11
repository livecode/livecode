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

#include "lnxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
#include "font.h"
#include "util.h"

#include "globals.h"

#include "lnxdc.h"
#include "lnxflst.h"
#include "packed.h"

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

////////////////////////////////////////////////////////////////////////////////

class MCNewFontlist: public MCFontlist
{
public:
	MCNewFontlist();
	~MCNewFontlist();

	virtual bool create(void);
	virtual void destroy(void);

	virtual MCFontStruct *getfont(const MCString &fname, uint2 &size, uint2 style, Boolean printer);
	virtual void getfontnames(MCExecPoint &ep, char *type);
	virtual void getfontsizes(const char *fname, MCExecPoint &ep);
	virtual void getfontstyles(const char *fname, uint2 fsize, MCExecPoint &ep);
	virtual bool getfontstructinfo(const char *&r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font);
	virtual void getfontreqs(MCFontStruct *f, const char*& r_name, uint2& r_size, uint2& r_style);

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
	if (m_font_map != nil)
		g_object_unref(m_font_map);
	if (m_layout != nil)
		g_object_unref(m_layout);
	if (m_pango != nil)
		g_object_unref(m_pango);
}

bool MCNewFontlist::create(void)
{
	if (initialise_weak_link_pango() == 0 ||
		initialise_weak_link_pangoft2() == 0)
		return false;

	m_font_map = pango_ft2_font_map_new();
	if (m_font_map == nil)
		return false;
	
	//m_pango = pango_font_map_create_context(m_font_map);
	m_pango = pango_ft2_font_map_create_context((PangoFT2FontMap *) m_font_map);
	if (m_pango == nil)
		return false;

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

MCFontStruct *MCNewFontlist::getfont(const MCString& p_family, uint2& p_size, uint2 p_style, Boolean p_printer)
{
	MCNewFontStruct *t_font;
	for(t_font = m_fonts; t_font != nil; t_font = t_font -> next)
		if (p_family == t_font -> family && p_size == t_font -> size && p_style == t_font -> style)
			return t_font;

	t_font = new MCNewFontStruct;
	t_font -> family = p_family . clone();
	t_font -> size = p_size;
	t_font -> style = p_style;
	t_font -> next = m_fonts;
	m_fonts = t_font;

	const char *t_charset;
	t_charset = strchr(t_font -> family, ',');
	if (t_charset != nil)
	{
		t_font -> charset = MCU_languagetocharset(t_charset + 1);
		t_font -> unicode = (t_font -> charset != 0);
	}
	else
	{
		t_font -> charset = 0;
		t_font -> unicode = False;
	}

	char *t_family_name;
	if (t_charset != nil)
		MCCStringCloneSubstring(t_font -> family, t_charset - t_font -> family, t_family_name);
	else
		MCCStringClone(t_font -> family, t_family_name);

	t_font -> description = pango_font_description_new();
	pango_font_description_set_family(t_font -> description, t_family_name);
	pango_font_description_set_absolute_size(t_font -> description, p_size * PANGO_SCALE);
	if ((p_style & (FA_ITALIC | FA_OBLIQUE)) != 0)
		pango_font_description_set_style(t_font -> description, PANGO_STYLE_ITALIC);
	if ((p_style & FA_WEIGHT) > 0x05)
		pango_font_description_set_weight(t_font -> description, PANGO_WEIGHT_BOLD);
	if ((p_style & FA_EXPAND) > 0x50)
		pango_font_description_set_stretch(t_font -> description, PANGO_STRETCH_EXPANDED);
	else if ((p_style & FA_EXPAND) < 0x05)
		pango_font_description_set_stretch(t_font -> description, PANGO_STRETCH_CONDENSED);

	PangoFont *t_p_font;
	t_p_font = pango_context_load_font(m_pango, t_font -> description);

	FT_Face t_face;
	t_face = pango_fc_font_lock_face((PangoFcFont*)t_p_font);

	int32_t t_ascent, t_descent, t_height;
	t_ascent = t_face -> size -> metrics . ascender / 64;
	t_descent = t_face -> size -> metrics . descender / 64;
	t_height = t_face -> size -> metrics . height / 64.0;

	if (t_ascent <= p_size)
	{
		t_font -> ascent = t_ascent;
		t_font -> descent = t_height - t_ascent;
	}
	else
	{
		t_font -> ascent = p_size * 7 / 8;
		t_font -> descent = t_height - t_ascent;
	}

	pango_fc_font_unlock_face((PangoFcFont*)t_p_font);

	g_object_unref(t_p_font);

	MCCStringFree(t_family_name);

	return t_font;
}

void MCNewFontlist::getfontnames(MCExecPoint &ep, char *type)
{
	ep . clear();

	PangoFontFamily **t_families;
	int t_family_count;
	pango_context_list_families(m_pango, &t_families, &t_family_count);
	for(uint32_t i = 0; i < t_family_count; i++)
		ep . concatcstring(pango_font_family_get_name(t_families[i]), EC_RETURN, i == 0);

	g_free(t_families);
}

void MCNewFontlist::getfontsizes(const char *fname, MCExecPoint &ep)
{
	ep . clear();

	PangoFontFamily **t_families;
	int t_family_count;
	pango_context_list_families(m_pango, &t_families, &t_family_count);
	for(uint32_t i = 0; i < t_family_count; i++)
		if (MCCStringEqualCaseless(fname, pango_font_family_get_name(t_families[i])))
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
					ep . setcstring("0");
				else
				{
					for(uint32_t i = 0; i < t_size_count; i++)
						ep . concatint(t_sizes[i] / PANGO_SCALE, EC_RETURN, i == 0);
					g_free(t_sizes);
				}
				g_free(t_faces);
			}
			break;
		}

	g_free(t_families);
}

void MCNewFontlist::getfontstyles(const char *fname, uint2 fsize, MCExecPoint &ep)
{
	ep . clear();

	PangoFontFamily **t_families;
	int t_family_count;
	pango_context_list_families(m_pango, &t_families, &t_family_count);
	for(uint32_t i = 0; i < t_family_count; i++)
		if (MCCStringEqualCaseless(fname, pango_font_family_get_name(t_families[i])))
		{
			PangoFontFace **t_faces;
			int t_face_count;
			pango_font_family_list_faces(t_families[i], &t_faces, &t_face_count);
			for(uint32_t i = 0; i < t_face_count; i++)
				ep . concatcstring(pango_font_face_get_face_name(t_faces[i]), EC_RETURN, i == 0);
			g_free(t_faces);
		}

	g_free(t_families);
}

bool MCNewFontlist::getfontstructinfo(const char *&r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
{
	MCNewFontStruct *t_font;
	t_font = static_cast<MCNewFontStruct *>(p_font);
	r_name = t_font -> family;
	r_size = t_font -> size;
	r_style = t_font -> style;
	r_printer = False;
	return true;
}

void MCNewFontlist::getfontreqs(MCFontStruct *p_font, const char*& r_name, uint2& r_size, uint2& r_style)
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
	t_is_unicode = (f -> unicode || p_unicode_override);

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

		MCTextLayoutGlyph *t_glyphs;
		if (!MCMemoryNewArray(t_run -> glyphs -> num_glyphs, t_glyphs))
			t_success = false;
		
		PangoFcFont *t_font;
		t_font = PANGO_FC_FONT(t_run -> item -> analysis . font);
		if (t_success)
			for(uint32_t i = 0; i < t_run -> glyphs -> num_glyphs; i++)
			{
				t_glyphs[i] . index = t_run -> glyphs -> glyphs[i] . glyph;
				t_glyphs[i] . x = t_running_x + (double)t_run -> glyphs -> glyphs[i] . geometry . x_offset / PANGO_SCALE;
				t_glyphs[i] . y = (double)t_run -> glyphs -> glyphs[i] . geometry . y_offset / PANGO_SCALE;
				t_running_x += (double)t_run -> glyphs -> glyphs[i] . geometry . width / PANGO_SCALE;
			}

		MCExecPoint ep;
		if (t_success)
		{
			ep . setsvalue(MCString(pango_layout_get_text(m_layout) + t_run -> item -> offset, t_run -> item -> length));
			ep . utf8toutf16();
			fprintf(stderr, "\nUTF-8 text: %d/%d - %s\n", t_run -> item -> offset, t_run -> item -> length, pango_layout_get_text(m_layout) + t_run -> item -> offset);
			fprintf(stderr, "UTF-16 length in bytes = %d\n", ep . getsvalue() . getlength());
		}
		
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
				MCMemoryNewArray(ep . getsvalue() . getlength() / 2, t_clusters);
		
		if (t_success)
		{
			const unichar_t *t_chars;
			uint32_t t_char_count;
			t_chars = (const unichar_t *)ep . getsvalue() . getstring();
			t_char_count = ep . getsvalue() . getlength() / 2;
			
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
			for(uint32_t i = 0; i < t_run -> glyphs -> num_glyphs; i++)
			{
				uint32_t t_byte_offset;
				t_byte_offset = t_run -> glyphs -> log_clusters[i];
				t_clusters[t_char_map[t_byte_offset]] = MCMin((uint32_t)t_clusters[t_char_map[t_byte_offset]], i);
			}
			for(uint32_t i = 1; i < t_char_count; i++)
				if (t_clusters[i] == 65535)
					t_clusters[i] = t_clusters[i - 1];
			
			fprintf(stderr, "Bytes:     ");
			for(uint32_t i = 0; i < t_byte_count; i++)
				fprintf(stderr, "%02x ", t_bytes[i]);
			fprintf(stderr, "\nMapping:   ");
			for(uint32_t i = 0; i < t_byte_count; i++)
				fprintf(stderr, "%04x ", t_char_map[i]);
			fprintf(stderr, "\nGlyph Map: ");
			for(uint32_t i = 0; i < t_run -> glyphs -> num_glyphs; i++)
				fprintf(stderr, "%04x ", t_run -> glyphs -> log_clusters[i]);
			fprintf(stderr, "\nClusters:  ");
			for(uint32_t i = 0; i < t_char_count; i++)
				fprintf(stderr, "%04x ", t_clusters[i]);
			fprintf(stderr, "\n");
			
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

		pango_layout_iter_next_run(t_iter);
	}
	
	pango_layout_iter_free(t_iter);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

MCFontlist *MCFontlistCreateNew(void)
{
	if (!MCuseXft)
		return nil;

	MCNewFontlist *t_fontlist;
	t_fontlist = new MCNewFontlist;
	if (!t_fontlist -> create())
	{
		delete t_fontlist;
		return nil;
	}

	return t_fontlist;
}

////////////////////////////////////////////////////////////////////////////////
