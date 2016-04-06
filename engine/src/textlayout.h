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

#ifndef __MC_TEXT_LAYOUT__
#define __MC_TEXT_LAYOUT__

////////////////////////////////////////////////////////////////////////////////

struct MCTextLayoutGlyph
{
	uint32_t index;
	double x, y;
};

struct MCTextLayoutSpan
{
	// The char array holds the sequence of characters that this span represents
	const unichar_t *chars;
	// The clusters array is the mapping from character to glyph. It is indexed
	// by character index, and each entry contains the index of the first glyph
	// in the cluster to which the character is part. e.g.
	//   Chars:    | c1u1 | c2u1 | c3u1 c3u2 c3u3 | c4u1 c4u2 |
	//   Glyphs:   | c1g1 | c2g1 c2g2 c2g3 | c3g1 | c4g1 c4g2 c4g3 |
	//   Clusters: | 0 | 1 | 4 4 4 | 5 5 |
	//
	const uint16_t *clusters;
	// The number of characters in the span
	uint32_t char_count;

	// The glyphs array contains the sequence of glyphs this span encodes.
	const MCTextLayoutGlyph *glyphs;
	// The number of glyphs in the glyph array
	uint32_t glyph_count;

	// A system-dependent pointer to the font. This is an HFONT on Windows,
	// a ATSUFontId on Mac, PangoFcFont on Linux and a CTFontRef on iOS.
	void *font;
};

bool MCTextLayoutInitialize(void);
void MCTextLayoutFinalize(void);

typedef bool (*MCTextLayoutCallback)(void *context, const MCTextLayoutSpan *span);
bool MCTextLayout(const unichar_t *chars, uint32_t char_count, MCFontStruct *p_font, MCTextLayoutCallback callback, void *context);

////////////////////////////////////////////////////////////////////////////////

#endif
