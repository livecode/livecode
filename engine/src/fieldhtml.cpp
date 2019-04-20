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
#include "osxprefix-legacy.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "field.h"
#include "paragraf.h"
#include "text.h"
#include "osspec.h"

#include "mcstring.h"
#include "uidc.h"
#include "globals.h"
#include "util.h"

////////////////////////////////////////////////////////////////////////////////

typedef uint32_t export_html_tag_type_t;
enum
{
	kExportHtmlTagLink,
	kExportHtmlTag_Lower = kExportHtmlTagLink,
	kExportHtmlTagSpan,
	kExportHtmlTagItalic,
	kExportHtmlTagBold,
	kExportHtmlTagStrike,
	kExportHtmlTagUnderline,
	kExportHtmlTagSuperscript,
	kExportHtmlTagSubscript,
	kExportHtmlTagCondensed,
	kExportHtmlTagExpanded,
	kExportHtmlTagThreeDBox,
	kExportHtmlTagBox,
	kExportHtmlTagFont,
	kExportHtmlTag_Upper,
};

// MW-2012-09-19: [[ Bug 10228 ]] Enum specifying type of escaping to do when
//   emitting chars.
enum export_html_escape_type_t
{
	kExportHtmlEscapeTypeNone,
	kExportHtmlEscapeTypeAttribute,
	kExportHtmlEscapeTypeTag,
};

struct export_html_tag_t
{
	bool present;
	union
	{
		struct
		{
			bool has_color;
			bool has_bg_color;
			MCNameRef name;
			int32_t size;
			uint32_t color;
			uint32_t bg_color;
		} font;
		uint32_t shift;
		MCStringRef metadata;
		struct
		{
			MCStringRef target;
			bool is_href;
		} link;
	};
};

typedef export_html_tag_t export_html_tag_list_t[kExportHtmlTag_Upper];

struct export_html_t
{
	MCStringRef m_text;
	
	bool effective;
	
	uint32_t list_depth;
	uint8_t list_styles[16];
	
	export_html_tag_list_t tags;
	export_html_tag_type_t tag_stack[10];
	uint32_t tag_depth;
};

////////////////////////////////////////////////////////////////////////////////

static const char * const s_export_html_tag_strings[] =
{
	"a",
	"span",
	"i",
	"b",
	"strike",
	"u",
	"sup",
	"sub",
	"condensed",
	"expanded",
	"threedbox",
	"box",
	"font"
};

static const char * const s_export_html_list_types[] =
{
	"",
	"disc",
	"circle",
	"square",
	"1",
	"a",
	"A",
	"i",
	"I",
	nil
};

// This is the list of HTML entities for ISO8859-1 (unicode) codepoints in the range
// 0x00A0 to 0x00FF inclusive.
static const char * const s_export_html_native_entities[] =
{
	"nbsp", "iexcl", "cent", "pound", "curren", "yen", "brvbar", "sect", 
	"uml", "copy", "ordf", "laquo", "not", "shy", "reg", "macr", 
	"deg", "plusmn", "sup2", "sup3", "acute", "micro", "para", "middot", 
	"cedil", "sup1", "ordm", "raquo", "frac14", "frac12", "frac34", "iquest", 
	"Agrave", "Aacute", "Acirc", "Atilde", "Auml", "Aring", "AElig", "Ccedil", 
	"Egrave", "Eacute", "Ecirc", "Euml", "Igrave", "Iacute", "Icirc", "Iuml", 
	"ETH", "Ntilde", "Ograve", "Oacute", "Ocirc", "Otilde", "Ouml", "times", 
	"Oslash", "Ugrave", "Uacute", "Ucirc", "Uuml", "Yacute", "THORN", "szlig", 
	"agrave", "aacute", "acirc", "atilde", "auml", "aring", "aelig", "ccedil", 
	"egrave", "eacute", "ecirc", "euml", "igrave", "iacute", "icirc", "iuml", 
	"eth", "ntilde", "ograve", "oacute", "ocirc", "otilde", "ouml", "divide", 
	"oslash", "ugrave", "uacute", "ucirc", "uuml", "yacute", "thorn", "yuml", 
};

static const struct { const char *entity; uint32_t codepoint; } s_export_html_unicode_entities[] =
{
	{ "OElig", 0x0152 }, { "oelig", 0x0153 }, { "Scaron", 0x0160 }, 
	{ "scaron", 0x0161 }, { "Yuml", 0x0178 }, { "fnof", 0x0192 }, { "circ", 0x02C6 }, 
	{ "tilde", 0x02DC }, { "Alpha", 0x0391 }, { "Beta", 0x0392 }, { "Gamma", 0x0393 }, 
	{ "Delta", 0x0394 }, { "Epsilon", 0x0395 }, { "Zeta", 0x0396 }, { "Eta", 0x0397 }, 
	{ "Theta", 0x0398 }, { "Iota", 0x0399 }, { "Kappa", 0x039A }, { "Lambda", 0x039B }, 
	{ "Mu", 0x039C }, { "Nu", 0x039D }, { "Xi", 0x039E }, { "Omicron", 0x039F }, 
	{ "Pi", 0x03A0 }, { "Rho", 0x03A1 }, { "Sigma", 0x03A3 }, { "Tau", 0x03A4 }, 
	{ "Upsilon", 0x03A5 }, { "Phi", 0x03A6 }, { "Chi", 0x03A7 }, { "Psi", 0x03A8 }, 
	{ "Omega", 0x03A9 }, { "alpha", 0x03B1 }, { "beta", 0x03B2 }, { "gamma", 0x03B3 }, 
	{ "delta", 0x03B4 }, { "epsilon", 0x03B5 }, { "zeta", 0x03B6 }, { "eta", 0x03B7 }, 
	{ "theta", 0x03B8 }, { "iota", 0x03B9 }, { "kappa", 0x03BA }, { "lambda", 0x03BB }, 
	{ "mu", 0x03BC }, { "nu", 0x03BD }, { "xi", 0x03BE }, { "omicron", 0x03BF }, 
	{ "pi", 0x03C0 }, { "rho", 0x03C1 }, { "sigmaf", 0x03C2 }, { "sigma", 0x03C3 }, 
	{ "tau", 0x03C4 }, { "upsilon", 0x03C5 }, { "phi", 0x03C6 }, { "chi", 0x03C7 }, 
	{ "psi", 0x03C8 }, { "omega", 0x03C9 }, { "thetasym", 0x03D1 }, { "upsih", 0x03D2 }, 
	{ "piv", 0x03D6 }, { "ensp", 0x2002 }, { "emsp", 0x2003 }, { "thinsp", 0x2009 }, 
	{ "zwnj", 0x200C }, { "zwj", 0x200D }, { "lrm", 0x200E }, { "rlm", 0x200F }, 
	{ "ndash", 0x2013 }, { "mdash", 0x2014 }, { "lsquo", 0x2018 }, { "rsquo", 0x2019 }, 
	{ "sbquo", 0x201A }, { "ldquo", 0x201C }, { "rdquo", 0x201D }, { "bdquo", 0x201E }, 
	{ "dagger", 0x2020 }, { "Dagger", 0x2021 }, { "bull", 0x2022 }, { "hellip", 0x2026 }, 
	{ "permil", 0x2030 }, { "prime", 0x2032 }, { "Prime", 0x2033 }, { "lsaquo", 0x2039 }, 
	{ "rsaquo", 0x203A }, { "oline", 0x203E }, { "frasl", 0x2044 }, { "euro", 0x20AC }, 
	{ "image", 0x2111 }, { "weierp", 0x2118 }, { "real", 0x211C }, { "trade", 0x2122 }, 
	{ "alefsym", 0x2135 }, { "larr", 0x2190 }, { "uarr", 0x2191 }, { "rarr", 0x2192 }, 
	{ "darr", 0x2193 }, { "harr", 0x2194 }, { "crarr", 0x21B5 }, { "lArr", 0x21D0 }, 
	{ "uArr", 0x21D1 }, { "rArr", 0x21D2 }, { "dArr", 0x21D3 }, { "hArr", 0x21D4 }, 
	{ "forall", 0x2200 }, { "part", 0x2202 }, { "exist", 0x2203 }, { "empty", 0x2205 }, 
	{ "nabla", 0x2207 }, { "isin", 0x2208 }, { "notin", 0x2209 }, { "ni", 0x220B }, 
	{ "prod", 0x220F }, { "sum", 0x2211 }, { "minus", 0x2212 }, { "lowast", 0x2217 }, 
	{ "radic", 0x221A }, { "prop", 0x221D }, { "infin", 0x221E }, { "ang", 0x2220 }, 
	{ "and", 0x2227 }, { "or", 0x2228 }, { "cap", 0x2229 }, { "cup", 0x222A }, 
	{ "int", 0x222B }, { "there4", 0x2234 }, { "sim", 0x223C }, { "cong", 0x2245 }, 
	{ "asymp", 0x2248 }, { "ne", 0x2260 }, { "equiv", 0x2261 }, { "le", 0x2264 }, 
	{ "ge", 0x2265 }, { "sub", 0x2282 }, { "sup", 0x2283 }, { "nsub", 0x2284 }, 
	{ "sube", 0x2286 }, { "supe", 0x2287 }, { "oplus", 0x2295 }, { "otimes", 0x2297 }, 
	{ "perp", 0x22A5 }, { "sdot", 0x22C5 }, { "lceil", 0x2308 }, { "rceil", 0x2309 }, 
	{ "lfloor", 0x230A }, { "rfloor", 0x230B }, { "lang", 0x2329 }, { "rang", 0x232A }, 
	{ "loz", 0x25CA }, { "spades", 0x2660 }, { "clubs", 0x2663 }, { "hearts", 0x2665 }, 
	{ "diams", 0x2666 }, 
};

////////////////////////////////////////////////////////////////////////////////

static int atoi(MCStringRef p_string)
{
	MCAutoNumberRef t_number;
	if (!MCNumberParse(p_string, &t_number))
	{
		return 0;
	}
	
	return MCNumberFetchAsInteger(*t_number);
}

static bool MCStringContainsCString(MCStringRef p_haystack,
									const char *p_needle,
									MCStringOptions p_options)
{
	MCAutoStringRef t_needle_str;
	if (!MCStringCreateWithCString(p_needle,
								   &t_needle_str))
	{
		return false;
	}
	
	return MCStringContains(p_haystack,
							*t_needle_str,
							p_options);
}

////////////////////////////////////////////////////////////////////////////////

static bool export_html_lookup_entity(uint32_t p_codepoint, char *r_buffer)
{
	uint4 t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_export_html_unicode_entities) / sizeof(s_export_html_unicode_entities[0]);
	while(t_low < t_high)
	{
		uint4 t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uint4 t_codepoint;
		t_codepoint = s_export_html_unicode_entities[t_mid] . codepoint;

		if (p_codepoint < t_codepoint)
			t_high = t_mid;
		else if (p_codepoint > t_codepoint)
			t_low = t_mid + 1;
		else
		{
			sprintf(r_buffer, "&%s;", s_export_html_unicode_entities[t_mid] . entity);
			return true;
		}
	}

	return false;
}

static void export_html_emit_char(char *p_output, uint32_t p_char, export_html_escape_type_t p_escape)
{
	if (p_char < 0x00A0)
	{
		// MW-2012-09-19: [[ Bug 10228 ]] Make sure we only escape chars based on context.
		//   Also, no longer escape apostrophe at all, as its not needed.
		if (p_char == 0x0022 && p_escape == kExportHtmlEscapeTypeAttribute)
			strcpy(p_output, "&quot;");
		else if (p_char == 0x0026 && p_escape == kExportHtmlEscapeTypeTag)
			strcpy(p_output, "&amp;");
		else if (p_char == 0x003C && p_escape == kExportHtmlEscapeTypeTag)
			strcpy(p_output, "&lt;");
		else if (p_char == 0x003E && p_escape == kExportHtmlEscapeTypeTag)
			strcpy(p_output, "&gt;");
		else if (p_char >= 0x20 && p_char < 0x7f)
			p_output[0] = p_char, p_output[1] = '\0';
		else
			sprintf(p_output, "&#%d;", p_char);
	}
	else if (p_char < 0x0100)
		sprintf(p_output, "&%s;", s_export_html_native_entities[p_char - 0x00A0]);
	else if (p_char < 0x0152 || p_char > 0x2666 || !export_html_lookup_entity(p_char, p_output))
		sprintf(p_output, "&#%d;", p_char);
}

static void export_html_emit_unicode_text(MCStringRef p_buffer, MCStringRef p_input, MCRange p_range, export_html_escape_type_t p_escape)
{
	uint32_t t_index = p_range.offset;
	while(t_index < p_range.offset + p_range.length)
	{
		unichar_t t_unit =
			MCStringGetCharAtIndex(p_input,
								   t_index);
		
		// If the next unit is a low surrogate *and* there is a next unit
		// *and* it is a high surrogate then map to a codepoint.
		codepoint_t t_codepoint = t_unit;
		if (MCUnicodeCodepointIsLeadingSurrogate(t_unit))
		{
			if (t_index + 1 < p_range.length)
			{
				unichar_t t_next_unit =
					MCStringGetCharAtIndex(p_input,
										   t_index + 1);
				
				if (MCUnicodeCodepointIsTrailingSurrogate(t_next_unit))
				{
					t_codepoint = MCUnicodeCombineSurrogates(t_unit,
															 t_next_unit);
					
					t_index += 1;
				}
			}
		}
		
		char t_output[16];
		export_html_emit_char(t_output, t_codepoint, p_escape);
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, "%s", t_output);
		
		t_index += 1;
	}
}

static void export_html_emit_text(MCStringRef p_buffer, MCStringRef p_input, MCRange p_range, export_html_escape_type_t p_escape)
{
	export_html_emit_unicode_text(p_buffer, p_input, p_range, p_escape);
}

static void export_html_emit_cstring(MCStringRef buffer, MCStringRef p_cstring, export_html_escape_type_t p_escape)
{
	export_html_emit_text(buffer, p_cstring, MCRangeMake(0, MCStringGetLength(p_cstring)), p_escape);
}

static bool export_html_tag_equal(const export_html_tag_t& left, const export_html_tag_t& right)
{
	return memcmp(&left, &right, sizeof(export_html_tag_t)) == 0;
}

static void export_html_compute_tags(const MCFieldCharacterStyle& p_style, bool p_effective, export_html_tag_list_t r_tags)
{
	if (p_effective || p_style . has_text_style)
	{
		if ((p_style . text_style & (FA_ITALIC | FA_OBLIQUE)) != 0)
			r_tags[kExportHtmlTagItalic] . present = true;
		if ((p_style . text_style & FA_WEIGHT) == (FA_BOLD & FA_WEIGHT))
			r_tags[kExportHtmlTagBold] . present = true;
		if ((p_style . text_style & FA_STRIKEOUT) != 0)
			r_tags[kExportHtmlTagStrike] . present = true;
		if ((p_style . text_style & FA_UNDERLINE) != 0)
			r_tags[kExportHtmlTagUnderline] . present = true;
		if ((p_style . text_style & FA_BOX) != 0)
			r_tags[kExportHtmlTagBox] . present = true;
		if ((p_style . text_style & FA_3D_BOX) != 0)
			r_tags[kExportHtmlTagThreeDBox] . present = true;
		// MW-2012-11-13: [[ Bug ]] Condensed/Expanded don't appear in html.
		if ((p_style . text_style & FA_EXPAND) == (FA_EXPANDED & FA_EXPAND))
			r_tags[kExportHtmlTagExpanded] . present = true;
		if ((p_style . text_style & FA_EXPAND) == (FA_CONDENSED & FA_EXPAND))
			r_tags[kExportHtmlTagCondensed] . present = true;
	}
	
	if (p_effective || p_style . has_text_shift)
	{
		export_html_tag_type_t t_tag;
		if (p_style . text_shift < 0)
			t_tag = kExportHtmlTagSuperscript;
		else
			t_tag = kExportHtmlTagSubscript;
		
		r_tags[t_tag] . present = true;
		r_tags[t_tag] . shift = p_style . text_shift;
	}
	
	if (p_effective || p_style . has_text_color)
	{
		r_tags[kExportHtmlTagFont] . present = true;
		r_tags[kExportHtmlTagFont] . font . has_color = true;
		r_tags[kExportHtmlTagFont] . font . color = p_style . text_color;
	}
	if (p_style . has_background_color)
	{
		r_tags[kExportHtmlTagFont] . present = true;
		r_tags[kExportHtmlTagFont] . font . has_bg_color = true;
		r_tags[kExportHtmlTagFont] . font . bg_color = p_style . background_color;
	}
	if (p_effective || p_style . has_text_font)
	{
		r_tags[kExportHtmlTagFont] . present = true;
		r_tags[kExportHtmlTagFont] . font . name = p_style . text_font;
	}
	if (p_effective || p_style . has_text_size)
	{
		r_tags[kExportHtmlTagFont] . present = true;
		r_tags[kExportHtmlTagFont] . font . size = p_style . text_size;
	}
	// MW-2012-03-16: [[ Bug ]] Make sure we emit <a></a> if we have link textStyle but
	//   no linkText.
	if (p_style . has_link_text || (p_style . has_text_style && (p_style . text_style & FA_LINK) != 0))
	{
		r_tags[kExportHtmlTagLink] . present = true;
		r_tags[kExportHtmlTagLink] . link . target = p_style . has_link_text ? p_style . link_text : nil;
		r_tags[kExportHtmlTagLink] . link . is_href = p_style . has_text_style && (p_style . text_style & FA_LINK) != 0;
	}
	if (p_style . has_metadata)
	{
		r_tags[kExportHtmlTagSpan] . present = true;
		r_tags[kExportHtmlTagSpan] . metadata = p_style . metadata;
	}
}

// IM-2013-11-21: [[ Bug 11475 ]] Ensure html hex colors contain only rgb hex values in the right order
static const char *export_html_hexcolor(uint32_t p_pixel)
{
	static char s_color[8];
	uint8_t r, g, b, a;
	MCGPixelUnpackNative(p_pixel, r, g, b, a);
	sprintf(s_color, "#%2.2X%2.2X%2.2X", r, g, b);
	
	return s_color;
}

static void export_html_add_tag(export_html_t& ctxt, export_html_tag_type_t p_tag, export_html_tag_t p_value)
{
	if (!p_value . present)
		return;
	
	ctxt . tags[p_tag] = p_value;
	ctxt . tag_stack[ctxt . tag_depth++] = p_tag;
	
	switch(p_tag)
	{
	case kExportHtmlTagSuperscript:
	case kExportHtmlTagSubscript:
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<%s shift=\"%d\">", s_export_html_tag_strings[p_tag], p_value.shift);
		break;
	case kExportHtmlTagFont:
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<font");
		if (p_value . font . name != nil)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " face=\"%@\"", p_value.font.name);
		if (p_value . font . size != 0)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " size=\"%d\"", p_value.font.size);
		if (p_value . font . has_color)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " color=\"%s\"", export_html_hexcolor(p_value . font . color));
		if (p_value . font . has_bg_color)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " bgcolor=\"%s\"", export_html_hexcolor(p_value . font . bg_color));
		/* UNCHECKED */ MCStringAppendChar(ctxt.m_text, '>');
		break;
	case kExportHtmlTagItalic:
	case kExportHtmlTagBold:
	case kExportHtmlTagStrike:
	case kExportHtmlTagUnderline:
	case kExportHtmlTagCondensed:
	case kExportHtmlTagExpanded:
	case kExportHtmlTagThreeDBox:
	case kExportHtmlTagBox:
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<%s>", s_export_html_tag_strings[p_tag]);
		break;
	case kExportHtmlTagLink:
		{
			// MW-2012-03-16: [[ Bug ]] If the linkText is nil, then just output a <a> tag.
			if (p_value . link . target != nil)
			{
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<a %s=\"", p_value.link.is_href ? "href" : "name");

				// MW-2012-09-19: [[ Bug 10228 ]] Make sure we generate the string in quote context.
				export_html_emit_cstring(ctxt.m_text, p_value.link.target, kExportHtmlEscapeTypeAttribute);
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\">");
			}
			else
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<a>");
		}
		break;
	case kExportHtmlTagSpan:
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<span metadata=\"");
		// MW-2012-09-19: [[ Bug 10228 ]] Make sure we generate the string in quote context.
		export_html_emit_cstring(ctxt.m_text, p_value.metadata, kExportHtmlEscapeTypeAttribute);
		MCStringAppendFormat(ctxt.m_text, "\">");
		break;
	default:
		break;
	}
}

static void export_html_remove_tag(export_html_t& ctxt, export_html_tag_type_t p_tag, bool p_all_above)
{
	if (!ctxt . tags[p_tag] . present)
		return;
	
	while(ctxt . tag_depth > 0)
	{
		ctxt . tag_depth -= 1;
		memset(&ctxt . tags[ctxt . tag_stack[ctxt . tag_depth]], 0, sizeof(export_html_tag_t));
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "</%s>", s_export_html_tag_strings[ctxt.tag_stack[ctxt.tag_depth]]);
		
		if (ctxt . tag_stack[ctxt . tag_depth] == p_tag &&
			(!p_all_above || ctxt . tag_stack[ctxt . tag_depth] > p_tag))
			break;
	}
}

static void export_html_remove_all_tags(export_html_t& ctxt, bool p_keep_link)
{
	while(ctxt . tag_depth > 0)
	{
		ctxt .  tag_depth -= 1;
		memset(&ctxt . tags[ctxt . tag_stack[ctxt . tag_depth]], 0, sizeof(export_html_tag_t));
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "</%s>", s_export_html_tag_strings[ctxt.tag_stack[ctxt.tag_depth]]);
		
		if (ctxt . tag_stack[ctxt . tag_depth] == kExportHtmlTagLink && p_keep_link)
			break;
	}
}

static void export_html_end_lists(export_html_t& ctxt, uint32_t p_new_style, uint32_t p_new_depth)
{
	if (ctxt . list_depth > 0 && !(p_new_depth == ctxt . list_depth && p_new_style == kMCParagraphListStyleSkip))
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "</li>\n");
	
	while(p_new_depth < ctxt . list_depth)
	{
		ctxt . list_depth -= 1;
		MCStringAppendFormat(ctxt.m_text, ctxt.list_styles[ctxt.list_depth] < kMCParagraphListStyleNumeric ? "</ul>" : "</ol>");
	}
}

static void export_html_begin_lists(export_html_t& ctxt, uint32_t p_new_style, uint32_t p_new_depth, uint32_t p_index)
{
	if (p_new_depth == 0)
		return;
		
	if (ctxt . list_depth == p_new_depth && p_new_style != kMCParagraphListStyleSkip && ctxt . list_styles[ctxt . list_depth - 1] != p_new_style)
	{
		ctxt . list_depth -= 1;
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, ctxt.list_styles[ctxt.list_depth] < kMCParagraphListStyleNumeric ? "</ul>" : "</ol>");
	}
	
	while(p_new_depth > ctxt . list_depth)
	{
		ctxt . list_styles[ctxt . list_depth] = p_new_style;
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, ctxt.list_styles[ctxt.list_depth] < kMCParagraphListStyleNumeric ? "<ul type=\"%s\">\n" : "<ol type=\"%s\">\n", s_export_html_list_types[p_new_style]);
		ctxt . list_depth += 1;
	}
	
	if (ctxt . list_depth != p_new_depth || p_new_style != kMCParagraphListStyleSkip)
	{
		if (p_index == 0)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<li>\n");
		else
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<li value=\"%d\">", p_index);
	}
}

static bool export_html_emit_paragraphs(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	export_html_t& ctxt = *(export_html_t *)p_context;
	
	if (p_event_type == kMCFieldExportEventBeginParagraph)
	{
		uint32_t t_new_list_style, t_new_list_depth, t_new_list_index;
		if (p_event_data . has_paragraph_style && p_event_data . paragraph_style . has_list_style)
		{
			t_new_list_style = p_event_data . paragraph_style . list_style;
			t_new_list_depth = p_event_data . paragraph_style . list_depth + 1;
			t_new_list_index = p_event_data . paragraph_style . has_list_index ? p_event_data . paragraph_style . list_index : 0;
		}
		else
			t_new_list_style = kMCParagraphListStyleNone, t_new_list_depth = 0, t_new_list_index = 0;

		export_html_end_lists(ctxt, t_new_list_style, t_new_list_depth);
		export_html_begin_lists(ctxt, t_new_list_style, t_new_list_depth, t_new_list_index);
		
		if (ctxt . effective || p_event_data . has_paragraph_style)
		{
			const MCFieldParagraphStyle& t_style = p_event_data . paragraph_style;
			
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<p");
			if (t_style . has_metadata)
			{
                // SN-2014-11-24: [[ Bug 14064 ]] Remove the additionnal '`'
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " metadata=\"");
                export_html_emit_cstring(ctxt.m_text, t_style.metadata, kExportHtmlEscapeTypeAttribute);
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\"");
			}
			if (ctxt . effective || t_style . has_text_align)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " align=\"%s\"", MCtextalignstrings[t_style.text_align]);
			if (!t_style . has_list_indent && (t_style . has_first_indent || ctxt . effective))
                // AL-2014-09-09: [[ Bug 13353 ]] firstindent parameter is integer not string
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " firstindent=\"%d\"", t_style.first_indent);
			else if (t_style . has_list_indent)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " listindent=\"%d\"", t_style.list_indent);
			if (ctxt . effective || t_style . has_left_indent)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " leftindent=\"%d\"", t_style.left_indent);
			if (ctxt . effective || t_style . has_right_indent)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " rightindent=\"%d\"", t_style.right_indent);
			if (ctxt . effective || t_style . has_space_above)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " spaceabove=\"%d\"", t_style.space_above);
			if (ctxt . effective || t_style . has_space_below)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " spacebelow=\"%d\"", t_style.space_below);
			if (ctxt . effective || t_style . has_tabs)
			{
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " tabstops=\"");
				for(uint32_t i = 0; i < t_style . tab_count; i++)
					/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, i == 0 ? "%d" : ",%d", t_style.tabs[i]);
				/* UNCHECKED */ MCStringAppendChar(ctxt.m_text, '"');
			}
			if (ctxt . effective || t_style . has_tab_alignments)
			{
				MCAutoStringRef t_formatted_tabalign;
				/* UNCHECKED */ MCField::formattabalignments(t_style.tab_alignments, t_style.tab_alignment_count, &t_formatted_tabalign);
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " tabalign=\"%@\"", *t_formatted_tabalign);
			}
			if (t_style . has_background_color)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " bgcolor=\"%s\"", export_html_hexcolor(t_style . background_color));
			if (t_style . has_border_width || ctxt . effective)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " borderwidth=\"%d\"", t_style.border_width);
			if (t_style . has_border_color || ctxt . effective)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " bordercolor=\"%s\"", export_html_hexcolor(t_style . border_color));
			if (t_style . has_padding || ctxt . effective)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " padding=\"%d\"", t_style.padding);
			if (t_style . has_hgrid || ctxt . effective)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, t_style.hgrid ? " hgrid" : " nohgrid");
			if (t_style . has_vgrid || ctxt . effective)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, t_style.vgrid ? " vgrid" : " novgrid");
			if (t_style . has_dont_wrap || ctxt . effective)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, t_style.dont_wrap ? " nowrap" : " wrap");
			if (t_style . hidden)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " hidden");
			/* UNCHECKED */ MCStringAppendChar(ctxt.m_text, '>');
		}
		else
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<p>");
	}
	else if (p_event_type == kMCFieldExportEventEndParagraph)
	{
		export_html_remove_all_tags(ctxt, false);
		
		// MW-2012-09-18: [[ Bug 10216 ]] If we are the last paragraph, but have non-zero listDepth then
		//   still emit a newline.
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, p_event_data.is_last_paragraph && ctxt.list_depth == 0 ? "</p>" : "</p>\n");
		
		if (p_event_data . is_last_paragraph)
			export_html_end_lists(ctxt, kMCParagraphListStyleNone, 0);
	}
	else if (p_event_type == kMCFieldExportEventUnicodeRun || p_event_type == kMCFieldExportEventNativeRun)
	{
		export_html_tag_list_t t_new_tags;
		memset(t_new_tags, 0, sizeof(export_html_tag_list_t));
		if (ctxt . effective || p_event_data . has_character_style)
			export_html_compute_tags(p_event_data . character_style, ctxt . effective, t_new_tags);
		
		for(uint32_t i = kExportHtmlTag_Lower; i < kExportHtmlTag_Upper; i++)
			if (!export_html_tag_equal(t_new_tags[i], ctxt . tags[i]))
				export_html_remove_tag(ctxt, i, i <= kExportHtmlTagSpan);
		
		for(uint32_t i = kExportHtmlTag_Lower; i < kExportHtmlTag_Upper; i++)
			if (!export_html_tag_equal(t_new_tags[i], ctxt . tags[i]))
				export_html_add_tag(ctxt, i, t_new_tags[i]);

		if (p_event_data . character_style . has_image_source)
		{
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "<img src=\"");
			// MW-2012-09-19: [[ Bug 10228 ]] Make sure we generate the string in quote context.
			export_html_emit_cstring(ctxt.m_text, p_event_data.character_style.image_source, kExportHtmlEscapeTypeAttribute);
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\" char=\"");
			// MW-2012-09-19: [[ Bug 10228 ]] Make sure we generate the string in quote context.
			export_html_emit_text(ctxt.m_text, p_event_data.m_text, p_event_data.m_range, kExportHtmlEscapeTypeAttribute);
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\">");
		}
		else
		{
			// MW-2012-09-19: [[ Bug 10228 ]] Make sure we generate the string in tag context.
			export_html_emit_text(ctxt.m_text, p_event_data.m_text, p_event_data.m_range, kExportHtmlEscapeTypeTag);
		}
	}
	
	return true;
}

bool MCField::exportashtmltext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, bool p_effective, MCDataRef& r_text)
{
	return exportashtmltext(resolveparagraphs(p_part_id), p_start_index, p_finish_index, p_effective, r_text);
}

bool MCField::exportashtmltext(MCParagraph *p_paragraphs, int32_t p_start_index, int32_t p_finish_index, bool p_effective, MCDataRef& r_text)
{
	export_html_t ctxt;
	memset(&ctxt, 0, sizeof(export_html_t));
	/* UNCHECKED */ MCStringCreateMutable(0, ctxt.m_text);
	
	ctxt . effective = p_effective;
	ctxt . list_depth = 0;
	
	// Compute the flags to pass to export.
	uint32_t t_flags;
	t_flags = kMCFieldExportParagraphs | kMCFieldExportRuns | kMCFieldExportParagraphStyles | kMCFieldExportCharacterStyles;
	if (p_effective)
		t_flags |= kMCFieldExportFlattenStyles;
	
	// Now execute the second pass which generates all the paragraph content.
	doexport(t_flags, p_paragraphs, p_start_index, p_finish_index, export_html_emit_paragraphs, &ctxt);

	// Return the buffer.
	/* UNCHECKED */ MCStringEncodeAndRelease(ctxt . m_text, kMCStringEncodingNative, false, r_text);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

enum import_html_tag_type_t
{
	kImportHtmlTagNone,
	
	kImportHtmlTagP,
	kImportHtmlTagLi,
	kImportHtmlTagH1,
	kImportHtmlTagH2,
	kImportHtmlTagH3,
	kImportHtmlTagH4,
	kImportHtmlTagH5,
	kImportHtmlTagH6,
	kImportHtmlTagBlockquote,
	kImportHtmlTagDt,
	kImportHtmlTagDd,
	kImportHtmlTagBr,
	kImportHtmlTagHr,
	
	kImportHtmlTagOl,
	kImportHtmlTagUl,

	kImportHtmlTagPre,
	
	kImportHtmlTag_FirstStyleTag,
	
	kImportHtmlTagAnchor,
	kImportHtmlTagImage,
	kImportHtmlTagFont,
	kImportHtmlTagBold,
	kImportHtmlTagItalic,
	kImportHtmlTagUnderline,
	kImportHtmlTagStrike,
	kImportHtmlTagCondensed,
	kImportHtmlTagExpanded,
	kImportHtmlTagBox,
	kImportHtmlTagThreeDBox,
	kImportHtmlTagSub,
	kImportHtmlTagSup,
	// MW-2012-08-31: [[ Bug 10343 ]] Implement support for importing 'span' tags.
	kImportHtmlTagSpan,
	
	kImportHtmlTagTitle,
	kImportHtmlTagHead,
	kImportHtmlTagMeta,
	kImportHtmlTagHtml,
	kImportHtmlTagBody,
	kImportHtmlTagAddress,
	kImportHtmlTagBig,
	kImportHtmlTagCaption,
	kImportHtmlTagCite,
	kImportHtmlTagCode,
	kImportHtmlTagDef,
	kImportHtmlTagEm,
	kImportHtmlTagSmall,
	kImportHtmlTagStrong,
	kImportHtmlTagVar,
	kImportHtmlTagTable,
	kImportHtmlTagTd,
	kImportHtmlTagTh,
	kImportHtmlTagTr,
	kImportHtmlTagTt,
};

static struct { const char *tag; import_html_tag_type_t type; } s_import_html_tag_strings[] =
{
	{ "p", kImportHtmlTagP },
	{ "li", kImportHtmlTagLi },
	{ "h1", kImportHtmlTagH1 },
	{ "h2", kImportHtmlTagH2 },
	{ "h3", kImportHtmlTagH3 },
	{ "h4", kImportHtmlTagH4 },
	{ "h5", kImportHtmlTagH5 },
	{ "h6", kImportHtmlTagH6 },
	{ "blockquote", kImportHtmlTagBlockquote },
	{ "dt", kImportHtmlTagDt },
	{ "dd", kImportHtmlTagDd },
	{ "br", kImportHtmlTagBr },
	{ "hr", kImportHtmlTagHr },
	
	{ "ol", kImportHtmlTagOl },
	{ "ul", kImportHtmlTagUl },
	
	{ "pre", kImportHtmlTagPre },
	
	{ "a", kImportHtmlTagAnchor },
	{ "img", kImportHtmlTagImage },
	{ "font", kImportHtmlTagFont },
	{ "b", kImportHtmlTagBold },
	{ "i", kImportHtmlTagItalic },
	{ "u", kImportHtmlTagUnderline },
	{ "strike", kImportHtmlTagStrike },
	{ "condensed", kImportHtmlTagCondensed },
	{ "expanded", kImportHtmlTagExpanded },
	{ "box", kImportHtmlTagBox },
	{ "threedbox", kImportHtmlTagThreeDBox },
	{ "sub", kImportHtmlTagSub },
	{ "sup", kImportHtmlTagSup },
	// MW-2012-08-31: [[ Bug 10343 ]] Implement support for importing 'span' tags.
	{ "span", kImportHtmlTagSpan },
	
	{ "title", kImportHtmlTagTitle },
	{ "head", kImportHtmlTagHead },
	{ "meta", kImportHtmlTagMeta },
	{ "html", kImportHtmlTagHtml },
	{ "body", kImportHtmlTagBody },
	{ "address", kImportHtmlTagAddress },
	{ "big", kImportHtmlTagBig },
	{ "caption", kImportHtmlTagCaption },
	{ "cite", kImportHtmlTagCite },
	{ "code", kImportHtmlTagCode },
	{ "def", kImportHtmlTagDef },
	{ "em", kImportHtmlTagEm },
	{ "small", kImportHtmlTagSmall },
	{ "strong", kImportHtmlTagStrong },
	{ "var", kImportHtmlTagVar },
	{ "table", kImportHtmlTagTable },
	{ "td", kImportHtmlTagTd },
	{ "th", kImportHtmlTagTh },
	{ "tr", kImportHtmlTagTr },
	{ "tt", kImportHtmlTagTt },
};

enum import_html_attr_type_t
{
	kImportHtmlAttrShift,
	kImportHtmlAttrFace,
	kImportHtmlAttrSize,
	kImportHtmlAttrColor,
	kImportHtmlAttrBgColor,
	kImportHtmlAttrHref,
	kImportHtmlAttrName,
	kImportHtmlAttrMetadata,
	kImportHtmlAttrAlign,
	kImportHtmlAttrFirstIndent,
	kImportHtmlAttrListIndent,
	kImportHtmlAttrLeftIndent,
	kImportHtmlAttrRightIndent,
	kImportHtmlAttrSpaceAbove,
	kImportHtmlAttrSpaceBelow,
	kImportHtmlAttrTabStops,
	kImportHtmlAttrTabAlignments,
	kImportHtmlAttrBorderWidth,
	kImportHtmlAttrBorderColor,
	kImportHtmlAttrPadding,
	kImportHtmlAttrHGrid,
	kImportHtmlAttrNoHGrid,
	kImportHtmlAttrVGrid,
	kImportHtmlAttrNoVGrid,
	kImportHtmlAttrWrap,
	kImportHtmlAttrNoWrap,
	kImportHtmlAttrHidden,
	kImportHtmlAttrSrc,
	kImportHtmlAttrChar,
	kImportHtmlAttrType,
	kImportHtmlAttrValue,
	kImportHtmlAttrStart,
	kImportHtmlAttrCharset,
	kImportHtmlAttrContent,
};

static struct { const char *attr; import_html_attr_type_t type; } s_import_html_attr_strings[] =
{
	{ "shift", kImportHtmlAttrShift },
	{ "face", kImportHtmlAttrFace },
	{ "size", kImportHtmlAttrSize },
	{ "color", kImportHtmlAttrColor },
	{ "bgcolor", kImportHtmlAttrBgColor },
	{ "href", kImportHtmlAttrHref },
	{ "name", kImportHtmlAttrName },
	{ "metadata", kImportHtmlAttrMetadata },
	{ "align", kImportHtmlAttrAlign },
	{ "firstindent", kImportHtmlAttrFirstIndent },
    // MW-2014-06-10: [[ Bug 12578 ]] Make sure listIndent attribute is processed on import.
	{ "listindent", kImportHtmlAttrListIndent },
	{ "leftindent", kImportHtmlAttrLeftIndent },
	{ "rightindent", kImportHtmlAttrRightIndent },
	{ "spaceabove", kImportHtmlAttrSpaceAbove},
	{ "spacebelow", kImportHtmlAttrSpaceBelow },
	{ "tabstops", kImportHtmlAttrTabStops },
	{ "tabalign", kImportHtmlAttrTabAlignments },
	{ "borderwidth", kImportHtmlAttrBorderWidth },
	{ "bordercolor", kImportHtmlAttrBorderColor },
	{ "padding", kImportHtmlAttrPadding },
	{ "hgrid", kImportHtmlAttrHGrid },
	{ "nohgrid", kImportHtmlAttrNoHGrid },
	{ "vgrid", kImportHtmlAttrVGrid },
	{ "novgrid", kImportHtmlAttrNoVGrid },
	{ "wrap", kImportHtmlAttrWrap },
	{ "nowrap", kImportHtmlAttrNoWrap },
	{ "hidden", kImportHtmlAttrHidden },
	{ "src", kImportHtmlAttrSrc },
	{ "char", kImportHtmlAttrChar },
	{ "type", kImportHtmlAttrType },
	{ "charset", kImportHtmlAttrCharset },
	{ "content", kImportHtmlAttrContent },
	{ "value", kImportHtmlAttrValue },
	{ "start", kImportHtmlAttrStart },
};

static const struct { const char *entity; uint32_t codepoint; } s_import_html_entities[] =
{
	{ "AElig", 0x00C6 }, { "Aacute", 0x00C1 }, { "Acirc", 0x00C2 }, { "Agrave", 0x00C0 }, 
	{ "Alpha", 0x0391 }, { "Aring", 0x00C5 }, { "Atilde", 0x00C3 }, { "Auml", 0x00C4 }, 
	{ "Beta", 0x0392 }, { "Ccedil", 0x00C7 }, { "Chi", 0x03A7 }, { "Dagger", 0x2021 }, 
	{ "Delta", 0x0394 }, { "ETH", 0x00D0 }, { "Eacute", 0x00C9 }, { "Ecirc", 0x00CA }, 
	{ "Egrave", 0x00C8 }, { "Epsilon", 0x0395 }, { "Eta", 0x0397 }, { "Euml", 0x00CB }, 
	{ "Gamma", 0x0393 }, { "Iacute", 0x00CD }, { "Icirc", 0x00CE }, { "Igrave", 0x00CC }, 
	{ "Iota", 0x0399 }, { "Iuml", 0x00CF }, { "Kappa", 0x039A }, { "Lambda", 0x039B }, 
	{ "Mu", 0x039C }, { "Ntilde", 0x00D1 }, { "Nu", 0x039D }, { "OElig", 0x0152 }, 
	{ "Oacute", 0x00D3 }, { "Ocirc", 0x00D4 }, { "Ograve", 0x00D2 }, { "Omega", 0x03A9 }, 
	{ "Omicron", 0x039F }, { "Oslash", 0x00D8 }, { "Otilde", 0x00D5 }, { "Ouml", 0x00D6 }, 
	{ "Phi", 0x03A6 }, { "Pi", 0x03A0 }, { "Prime", 0x2033 }, { "Psi", 0x03A8 }, 
	{ "Rho", 0x03A1 }, { "Scaron", 0x0160 }, { "Sigma", 0x03A3 }, { "THORN", 0x00DE }, 
	{ "Tau", 0x03A4 }, { "Theta", 0x0398 }, { "Uacute", 0x00DA }, { "Ucirc", 0x00DB }, 
	{ "Ugrave", 0x00D9 }, { "Upsilon", 0x03A5 }, { "Uuml", 0x00DC }, { "Xi", 0x039E }, 
	{ "Yacute", 0x00DD }, { "Yuml", 0x0178 }, { "Zeta", 0x0396 }, { "aacute", 0x00E1 }, 
	{ "acirc", 0x00E2 }, { "acute", 0x00B4 }, { "aelig", 0x00E6 }, { "agrave", 0x00E0 }, 
	{ "alefsym", 0x2135 }, { "alpha", 0x03B1 }, { "amp", 0x0026 }, { "and", 0x2227 }, 
	{ "ang", 0x2220 }, { "apos", 0x0027 }, { "aring", 0x00E5 }, { "asymp", 0x2248 }, 
	{ "atilde", 0x00E3 }, { "auml", 0x00E4 }, { "bdquo", 0x201E }, { "beta", 0x03B2 }, 
	{ "brvbar", 0x00A6 }, { "bull", 0x2022 }, { "cap", 0x2229 }, { "ccedil", 0x00E7 }, 
	{ "cedil", 0x00B8 }, { "cent", 0x00A2 }, { "chi", 0x03C7 }, { "circ", 0x02C6 }, 
	{ "clubs", 0x2663 }, { "cong", 0x2245 }, { "copy", 0x00A9 }, { "crarr", 0x21B5 }, 
	{ "cup", 0x222A }, { "curren", 0x00A4 }, { "dArr", 0x21D3 }, { "dagger", 0x2020 }, 
	{ "darr", 0x2193 }, { "deg", 0x00B0 }, { "delta", 0x03B4 }, { "diams", 0x2666 }, 
	{ "divide", 0x00F7 }, { "eacute", 0x00E9 }, { "ecirc", 0x00EA }, { "egrave", 0x00E8 }, 
	{ "empty", 0x2205 }, { "emsp", 0x2003 }, { "ensp", 0x2002 }, { "epsilon", 0x03B5 }, 
	{ "equiv", 0x2261 }, { "eta", 0x03B7 }, { "eth", 0x00F0 }, { "euml", 0x00EB }, 
	{ "euro", 0x20AC }, { "exist", 0x2203 }, { "fnof", 0x0192 }, { "forall", 0x2200 }, 
	{ "frac12", 0x00BD }, { "frac14", 0x00BC }, { "frac34", 0x00BE }, { "frasl", 0x2044 }, 
	{ "gamma", 0x03B3 }, { "ge", 0x2265 }, { "gt", 0x003E }, { "hArr", 0x21D4 }, 
	{ "harr", 0x2194 }, { "hearts", 0x2665 }, { "hellip", 0x2026 }, { "iacute", 0x00ED }, 
	{ "icirc", 0x00EE }, { "iexcl", 0x00A1 }, { "igrave", 0x00EC }, { "image", 0x2111 }, 
	{ "infin", 0x221E }, { "int", 0x222B }, { "iota", 0x03B9 }, { "iquest", 0x00BF }, 
	{ "isin", 0x2208 }, { "iuml", 0x00EF }, { "kappa", 0x03BA }, { "lArr", 0x21D0 }, 
	{ "lambda", 0x03BB }, { "lang", 0x2329 }, { "laquo", 0x00AB }, { "larr", 0x2190 }, 
	{ "lceil", 0x2308 }, { "ldquo", 0x201C }, { "le", 0x2264 }, { "lfloor", 0x230A }, 
	{ "lowast", 0x2217 }, { "loz", 0x25CA }, { "lrm", 0x200E }, { "lsaquo", 0x2039 }, 
	{ "lsquo", 0x2018 }, { "lt", 0x003C }, { "macr", 0x00AF }, { "mdash", 0x2014 }, 
	{ "micro", 0x00B5 }, { "middot", 0x00B7 }, { "minus", 0x2212 }, { "mu", 0x03BC }, 
	{ "nabla", 0x2207 }, { "nbsp", 0x00A0 }, { "ndash", 0x2013 }, { "ne", 0x2260 }, 
	{ "ni", 0x220B }, { "not", 0x00AC }, { "notin", 0x2209 }, { "nsub", 0x2284 }, 
	{ "ntilde", 0x00F1 }, { "nu", 0x03BD }, { "oacute", 0x00F3 }, { "ocirc", 0x00F4 }, 
	{ "oelig", 0x0153 }, { "ograve", 0x00F2 }, { "oline", 0x203E }, { "omega", 0x03C9 }, 
	{ "omicron", 0x03BF }, { "oplus", 0x2295 }, { "or", 0x2228 }, { "ordf", 0x00AA }, 
	{ "ordm", 0x00BA }, { "oslash", 0x00F8 }, { "otilde", 0x00F5 }, { "otimes", 0x2297 }, 
	{ "ouml", 0x00F6 }, { "para", 0x00B6 }, { "part", 0x2202 }, { "permil", 0x2030 }, 
	{ "perp", 0x22A5 }, { "phi", 0x03C6 }, { "pi", 0x03C0 }, { "piv", 0x03D6 }, 
	{ "plusmn", 0x00B1 }, { "pound", 0x00A3 }, { "prime", 0x2032 }, { "prod", 0x220F }, 
	{ "prop", 0x221D }, { "psi", 0x03C8 }, { "quot", 0x0022 }, { "rArr", 0x21D2 }, 
	{ "radic", 0x221A }, { "rang", 0x232A }, { "raquo", 0x00BB }, { "rarr", 0x2192 }, 
	{ "rceil", 0x2309 }, { "rdquo", 0x201D }, { "real", 0x211C }, { "reg", 0x00AE }, 
	{ "rfloor", 0x230B }, { "rho", 0x03C1 }, { "rlm", 0x200F }, { "rsaquo", 0x203A }, 
	{ "rsquo", 0x2019 }, { "sbquo", 0x201A }, { "scaron", 0x0161 }, { "sdot", 0x22C5 }, 
	{ "sect", 0x00A7 }, { "shy", 0x00AD }, { "sigma", 0x03C3 }, { "sigmaf", 0x03C2 }, 
	{ "sim", 0x223C }, { "spades", 0x2660 }, { "sub", 0x2282 }, { "sube", 0x2286 }, 
	{ "sum", 0x2211 }, { "sup", 0x2283 }, { "sup1", 0x00B9 }, { "sup2", 0x00B2 }, 
	{ "sup3", 0x00B3 }, { "supe", 0x2287 }, { "szlig", 0x00DF }, { "tau", 0x03C4 }, 
	{ "there4", 0x2234 }, { "theta", 0x03B8 }, { "thetasym", 0x03D1 }, { "thinsp", 0x2009 }, 
	{ "thorn", 0x00FE }, { "tilde", 0x02DC }, { "times", 0x00D7 }, { "trade", 0x2122 }, 
	{ "uArr", 0x21D1 }, { "uacute", 0x00FA }, { "uarr", 0x2191 }, { "ucirc", 0x00FB }, 
	{ "ugrave", 0x00F9 }, { "uml", 0x00A8 }, { "upsih", 0x03D2 }, { "upsilon", 0x03C5 }, 
	{ "uuml", 0x00FC }, { "weierp", 0x2118 }, { "xi", 0x03BE }, { "yacute", 0x00FD }, 
	{ "yen", 0x00A5 }, { "yuml", 0x00FF }, { "zeta", 0x03B6 }, { "zwj", 0x200D }, 
	{ "zwnj", 0x200C },
};

struct import_html_attr_t
{
	import_html_attr_type_t type;
	MCStringRef value;
};

struct import_html_tag_t
{
	import_html_tag_type_t type;
	bool is_terminator;
	import_html_attr_t *attrs;
	uint32_t attr_count;
};

struct import_html_stack_entry_t
{
	import_html_tag_type_t type;
	MCFieldCharacterStyle style;
};

struct import_html_t
{
	// The target string.
	MCField *field;
	
	// The paragraph list we are building.
	MCParagraph *paragraphs;
	
	// The last run style.
	MCFieldCharacterStyle last_used_style;
	
	// The current style stack.
	import_html_stack_entry_t *styles;
	uint32_t style_index;
	uint32_t style_capacity;
	
	// If true, then we need a new paragraph before appending text.
	bool need_paragraph;
	bool is_utf8;
	bool preformatted;
	
	uint32_t list_depth;
	bool list_in_li;
	uint32_t list_index;
	uint32_t list_paragraph_count;
	uint32_t list_starts[16];
	uint8_t list_styles[16];
	import_html_tag_type_t list_tags[16];
	
	// The currently accumulated characters.
	bool is_unicode;
	uint8_t *bytes;
	uint32_t byte_count;
	uint32_t byte_capacity;
};

static void import_html_copy_style(const MCFieldCharacterStyle& p_src, MCFieldCharacterStyle& r_dst)
{
	r_dst = p_src;
	if (p_src . has_link_text)
		r_dst . link_text = MCValueRetain(p_src . link_text);
	if (p_src . has_image_source)
		r_dst . image_source = MCValueRetain(p_src . image_source);
	if (p_src . has_metadata)
		r_dst . metadata = MCValueRetain(p_src . metadata);
	if (p_src . has_text_font)
        r_dst.text_font = MCValueRetain(p_src.text_font);
}

static void import_html_free_style(MCFieldCharacterStyle& p_style)
{
	MCValueRelease(p_style . link_text);
	MCValueRelease(p_style . image_source);
	MCValueRelease(p_style . metadata);
	MCValueRelease(p_style . text_font);
}

static bool import_html_equal_style(const MCFieldCharacterStyle& left, const MCFieldCharacterStyle& right)
{
	// MW-2012-09-19: [[ Bug 10399 ]] If either char style has an imageSource, then they
	//   are different.
	return left . image_source == nil && right . image_source == nil && memcmp(&left, &right, sizeof(MCFieldCharacterStyle)) == 0;
}

static bool import_html_lookup_tag(const char *p_start, const char *p_end, import_html_tag_type_t& r_tag)
{
	for(uint32_t i = 0; i < sizeof(s_import_html_tag_strings) / sizeof(s_import_html_tag_strings[0]); i++)
		if (MCCStringEqualSubstringCaseless(s_import_html_tag_strings[i] . tag, p_start, p_end - p_start) &&
			MCCStringLength(s_import_html_tag_strings[i] . tag) == p_end - p_start)
		{
			r_tag = s_import_html_tag_strings[i] . type;
			return true;
		}
		
	return false;
}

static void import_html_free_tag(import_html_tag_t& p_tag)
{
	for(uint32_t i = 0; i < p_tag . attr_count; i++)
		MCValueRelease(p_tag . attrs[i] . value);
	MCMemoryDeleteArray(p_tag . attrs);
}

static bool import_html_lookup_attr(const char *p_start, const char *p_end, import_html_attr_type_t& r_attr)
{
	for(uint32_t i = 0; i < sizeof(s_import_html_attr_strings) / sizeof(s_import_html_attr_strings[0]); i++)
		if (MCCStringEqualSubstringCaseless(s_import_html_attr_strings[i] . attr, p_start, p_end - p_start) &&
			MCCStringLength(s_import_html_attr_strings[i] . attr) == p_end - p_start)
		{
			r_attr = s_import_html_attr_strings[i] . type;
			return true;
		}
		
	return false;
}

static bool import_html_parse_entity(const char *& x_ptr, const char *p_limit, uint32_t& r_codepoint)
{
	const char *t_start_ptr;
	t_start_ptr = x_ptr + 1;
	while(*x_ptr != ';')
	{
		// MW-2012-03-16: [[ Bug ]] If we don't find a ';' within 16 chars, we fail
		//   and reset the input ptr to the char after the '&'.
		if (x_ptr >= p_limit || (x_ptr - t_start_ptr) > 16)
		{
			x_ptr = t_start_ptr;
			return false;
		}
		x_ptr += 1;
	}

	const char *t_end_ptr;
	t_end_ptr = x_ptr;
	
	x_ptr += 1;

	// MW-2012-03-16: [[ Bug ]] If the entity is of zero length, then reset the ptr
	//   to the char after the '&'.
	if (t_start_ptr == t_end_ptr)
	{
		x_ptr = t_start_ptr;
		return false;
	}
		
	if (t_start_ptr[0] == '#')
	{
		// MW-2012-03-16: [[ Bug ]] If the number doesn't terminate at ';' then its
		//   an invalid entity so return false and readjust the ptr.
		char *t_conv_end_ptr;
		t_conv_end_ptr = nil;
		
		// MW-2012-11-19: Add support for hex-encoded html entities.
		// MDW-2013-04-15: Corrected 't_is_hex == true' to 't_is_hex = true'.
		bool t_is_hex;
		const char *t_digit_start_ptr;
		if (t_start_ptr[1] == 'x')
			t_is_hex = true, t_digit_start_ptr = t_start_ptr + 2;
		else
			t_is_hex = false, t_digit_start_ptr = t_start_ptr + 1;
		
		uint32_t t_codepoint;
		t_codepoint = strtoul(t_digit_start_ptr, &t_conv_end_ptr, t_is_hex ? 16 : 10);
		if (t_conv_end_ptr != t_end_ptr)
		{
			x_ptr = t_start_ptr;
			return false;
		}
		
		// MW-2012-03-23: [[ Bug ]] Interpret entities in the range 128-159 inclusive
		//   as Windows-1252.
		if (t_codepoint > 127 && t_codepoint < 160)
			t_codepoint = MCUnicodeMapFromNative_Windows1252(t_codepoint);
			
		r_codepoint = t_codepoint;
		
		return true;
	}
	else
	{
		for(uint32_t i = 0; i < sizeof(s_import_html_entities) / sizeof(s_import_html_entities[0]); i++)
		{
			// MW-2012-03-10: [[ Bug ]] Compare entities case-sensitively.
			if (MCCStringEqualSubstring(s_import_html_entities[i] . entity, t_start_ptr, t_end_ptr - t_start_ptr) &&
				MCCStringLength(s_import_html_entities[i] . entity) == (t_end_ptr - t_start_ptr))
			{
				r_codepoint = s_import_html_entities[i] . codepoint;
				return true;
			}
		}
	}

	// MW-2012-03-16: [[ Bug ]] If the entity wasn't recognized, reset the ptr to
	//   the char after the '&'.
	x_ptr = t_start_ptr;
	return false;
}

// This method parses a (CDATA encoded) attribute value and returns it as a native
// cstring.
static bool import_html_parse_attr_value(const char *p_start_ptr, const char *p_end_ptr, MCStringRef& r_value)
{
	// First allocate room for the value - this is at most the length of the input
	// string as all entities are strictly longer than what they encode.
	MCAutoStringRef t_value;
	if (!MCStringCreateMutable((uindex_t)(p_end_ptr - p_start_ptr),
							   &t_value))
	{
		return false;
	}
		
	// Now loop through the input characters, emitting appropriate output chars
	// at the tail of t_value.
	while(p_start_ptr < p_end_ptr)
	{
		// If we encounter a '&' then check to see if its an entity - if it is
		// then emit that char (if it can map to native!); otherwise the char is
		// '&' and we emit the entity as written.
		codepoint_t t_codepoint;
		if (*p_start_ptr == '&')
		{
			if (!import_html_parse_entity(p_start_ptr, p_end_ptr, t_codepoint))
			{
				t_codepoint = '&';
			}
		}
		else
			t_codepoint = *p_start_ptr++;
		
		// Append the char to the value string we are accumulating.
		if (!MCStringAppendCodepoint(*t_value,
									 t_codepoint))
		{
			return false;
		}
	}
	
	if (!t_value.MakeImmutable())
	{
		return false;
	}
	
	// And return it.
	r_value = t_value.Take();
	
	return true;
}

static bool import_html_parse_attr(const char*& x_ptr, const char *p_limit, import_html_attr_t& r_attr)
{
	while(*x_ptr == ' ')
	{
		if (x_ptr >= p_limit)
			return false;
		x_ptr++;
	}
			
	const char *t_key_start_ptr, *t_key_end_ptr;
	t_key_start_ptr = x_ptr;
	while(*x_ptr != ' ' && *x_ptr != '=' && *x_ptr != '>')
	{
		if (x_ptr >= p_limit)
			return false;
		x_ptr++;
	}
	t_key_end_ptr = x_ptr;
	
	MCAutoStringRef t_value;
	if (*x_ptr == '=')
	{
		// MW-2014-03-11: [[ Bug 11888 ]] Ensure we consume as much as possible, else
		//   the caller can get into an infinite loop with things like "<a href=".
		x_ptr += 1;
		
		if (x_ptr >= p_limit)
			return false;

		const char *t_value_start_ptr, *t_value_end_ptr;
		if (*x_ptr == '"' || *x_ptr == '\'')
		{
			char t_start_char;
			t_start_char = *x_ptr;
			
			if (x_ptr + 1 >= p_limit)
				return false;
			x_ptr += 1;
			
			t_value_start_ptr = x_ptr;
			while(*x_ptr != t_start_char)
			{
				if (x_ptr >= p_limit)
					return false;
				x_ptr++;
			}
			t_value_end_ptr = x_ptr;
			
			x_ptr += 1;
		}
		else
		{
			t_value_start_ptr = x_ptr;
			while(*x_ptr != ' ' && *x_ptr != '>')
			{
				if (x_ptr >= p_limit)
					return false;
				x_ptr++;
			}
			t_value_end_ptr = x_ptr;
		}
		
		// MW-2012-03-16: [[ Bug ]] Make sure we un-entity encode the attribute value as
		//   its a CDATA section.
		if (!import_html_parse_attr_value(t_value_start_ptr, t_value_end_ptr, &t_value))
			return false;
	}
		
	if (import_html_lookup_attr(t_key_start_ptr, t_key_end_ptr, r_attr . type))
	{
		r_attr . value = t_value.Take();
		return true;
	}
	
	return false;
}

static bool import_html_parse_tag(const char *& x_ptr, const char *p_limit, import_html_tag_t& r_tag)
{
	import_html_tag_t t_tag;
	memset(&t_tag, 0, sizeof(import_html_tag_t));
	
	if (x_ptr + 1 < p_limit && x_ptr[1] == '/')
	{
		t_tag . is_terminator = true;
		x_ptr += 1;
	}
	else
		t_tag . is_terminator = false;

	const char *t_start_ptr;
	t_start_ptr = x_ptr + 1;
	while(*x_ptr != ' ' && *x_ptr != '>')
	{
		if (x_ptr >= p_limit)
			return false;
		x_ptr += 1;
	}
	
	const char *t_end_ptr;
	t_end_ptr = x_ptr;
	
	bool t_keep;
	t_keep = true;
	
	if (!import_html_lookup_tag(t_start_ptr, t_end_ptr, t_tag . type))
		t_keep = false;
	
	while(*x_ptr != '>')
	{
		// MW-2012-04-02: [[ Bug 10124 ]] Make sure we don't choke on '/>' endings.
		if (*x_ptr == '/' && x_ptr + 1 < p_limit && x_ptr[1] == '>')
		{
			x_ptr += 1;
			break;
		}
	
		import_html_attr_t t_attr;
		if (import_html_parse_attr(x_ptr, p_limit, t_attr))
		{
			if (MCMemoryResizeArray(t_tag . attr_count + 1, t_tag . attrs, t_tag . attr_count))
				t_tag . attrs[t_tag . attr_count - 1] = t_attr;
			else
				t_keep = false;
		}
			
		if (x_ptr >= p_limit)
		{
			t_keep = false;
			break;
		}
	}
	
	x_ptr += 1;
	
	if (t_keep)
		r_tag = t_tag;
	else
		import_html_free_tag(t_tag);

	// MW-2012-03-23: [[ Bug 10117 ]] If keep is false, then we failed to parse a tag.
	return t_keep;
}
static void import_html_begin(import_html_t& ctxt, const MCFieldParagraphStyle *p_style)
{
	MCFieldParagraphStyle t_style;
	if (p_style == nil)
		memset(&t_style, 0, sizeof(MCFieldParagraphStyle));
	else
		t_style = *p_style;
	
	if (ctxt . list_depth > 0)
	{
		t_style . has_list_style = true;

		if (!ctxt . list_in_li || ctxt . list_paragraph_count == 0)
			t_style . list_style = ctxt . list_styles[ctxt . list_depth - 1];
		else
			t_style . list_style = kMCParagraphListStyleSkip;

		if (ctxt . list_in_li && ctxt . list_index != 0)
		{
			t_style . has_list_index = true;
			t_style . list_index = ctxt . list_index;
		}
		else if (ctxt . list_starts[ctxt . list_depth - 1] != 0)
		{
			t_style . has_list_index = true;
			t_style . list_index = ctxt . list_starts[ctxt . list_depth - 1];
			ctxt . list_starts[ctxt . list_depth - 1] = 0;
		}

		t_style . list_depth = ctxt . list_depth - 1;
		
		ctxt . list_paragraph_count += 1;
	}
	
	ctxt . field -> importparagraph(ctxt . paragraphs, &t_style);
			
	ctxt . need_paragraph = false;
}

static void import_html_flush_chars(import_html_t& ctxt)
{
	if (ctxt . byte_count != 0)
	{
		// If a paragraph is needed, then make one.
		if (ctxt . need_paragraph)
			import_html_begin(ctxt, nil);

		// Add a block to the last paragraph in the list.
		ctxt . field -> importblock(ctxt . paragraphs -> prev(), ctxt . last_used_style, ctxt . bytes, ctxt . byte_count, ctxt . is_unicode);
	}
	
	import_html_free_style(ctxt . last_used_style);
	import_html_copy_style(ctxt . styles[ctxt . style_index] . style, ctxt . last_used_style);
	
	// We now have nothing to output.
	ctxt . byte_count = 0;
	
	// Reset the unicode flag.
	ctxt . is_unicode = false;
}

static bool import_html_ensure_bytes(import_html_t& ctxt, uint32_t p_amount)
{
	if (ctxt . byte_count + p_amount > ctxt . byte_capacity)
	{
		uint32_t t_new_capacity;
		t_new_capacity = (ctxt . byte_count + p_amount + 4096) & ~4095;
		if (!MCMemoryResizeArray(t_new_capacity, ctxt . bytes, ctxt . byte_capacity))
			return false;
	}
	
	return true;
}

static void import_html_append_native_chars(import_html_t& ctxt, const char *p_chars, uint32_t p_char_count)
{
	// MW-2012-03-16: [[ Bug ]] If the listDepth is non-zero and we aren't inside an
	//   'li' tag, then do nothing.
	if (ctxt . list_depth > 0 && !ctxt . list_in_li)
		return;
		
	// If the current text in the buffer is unicode, or the styling
	// has changed then flush the characters.
	if (ctxt . is_unicode ||
		!import_html_equal_style(ctxt . last_used_style, ctxt . styles[ctxt . style_index] . style))
		import_html_flush_chars(ctxt);
		
	// Make sure there's enough room in the text buffer.
	if (!import_html_ensure_bytes(ctxt, p_char_count))
		return;
	
	// Append the text to the buffer.
	memcpy((char *)ctxt . bytes + ctxt . byte_count, p_chars, p_char_count);
	ctxt . byte_count += p_char_count;
}

static void import_html_append_unicode_char(import_html_t& ctxt, uint32_t p_codepoint)
{
	// MW-2012-03-16: [[ Bug ]] If the listDepth is non-zero and we aren't inside an
	//   'li' tag, then do nothing.
	if (ctxt . list_depth > 0 && !ctxt . list_in_li)
		return;

	unichar_t t_unicode_char;
	t_unicode_char = (unichar_t)p_codepoint;
	
	// See if we can map the char to native.
	if (p_codepoint < 65536)
	{
		char_t t_native_char;
		if (MCUnicodeMapToNative(&t_unicode_char, 1, t_native_char))
		{
			import_html_append_native_chars(ctxt, (const char *)&t_native_char, 1);
			return;
		}
	}
	else
	{
		// MW-2012-11-19: If the codepoint > 65535, then split it into surrogates.
		uint16_t t_lead_char, t_trail_char;
		t_lead_char = 0xD800 - (0x10000 >> 10) + (p_codepoint >> 10);
		t_trail_char = 0xDC00 + (p_codepoint & 0x3FF);
		import_html_append_unicode_char(ctxt, t_lead_char);
		import_html_append_unicode_char(ctxt, t_trail_char);
		return;
	}
	
	// If the text is currently native (and there is some) or if the styling has changed
	// then flush.
	if ((!ctxt . is_unicode && ctxt . byte_count > 0) ||
		!import_html_equal_style(ctxt . last_used_style, ctxt . styles[ctxt . style_index] . style))
		import_html_flush_chars(ctxt);
	
	// Make sure there's enough room in the buffer.
	if (!import_html_ensure_bytes(ctxt, 2))
		return;
		
	// Append the text to the buffer.
	ctxt . is_unicode = true;
	memcpy((char *)ctxt . bytes + ctxt . byte_count, &t_unicode_char, 2);
	ctxt . byte_count += 2;
}

static void import_html_append_stringref(import_html_t& ctxt, MCStringRef p_chars)
{
	// If the text is currently native (and there is some) or if the styling has changed
	// then flush.
	if ((!ctxt . is_unicode && ctxt . byte_count > 0) ||
		!import_html_equal_style(ctxt . last_used_style, ctxt . styles[ctxt . style_index] . style))
		import_html_flush_chars(ctxt);
	
	// Make sure there's enough room in the buffer.
	if (!import_html_ensure_bytes(ctxt, sizeof(unichar_t) * MCStringGetLength(p_chars)))
		return;
	
	// Append the text to the buffer.
	ctxt . is_unicode = true;
	MCStringGetChars(p_chars,
					 MCRangeMake(0, MCStringGetLength(p_chars)),
					 (unichar_t *)(ctxt.bytes + ctxt.byte_count));
	ctxt.byte_count += sizeof(unichar_t) * MCStringGetLength(p_chars);
}

static void import_html_append_utf8_chars(import_html_t& ctxt, const char *p_chars, uint32_t p_char_count)
{
	while(p_char_count > 0)
	{
		const char *t_next_ptr;
		t_next_ptr = p_chars;
		
		// First loop over all ASCII chars.
		while(p_char_count > 0 && (unsigned)*t_next_ptr < 128)
			t_next_ptr++, p_char_count -= 1;
		
		// Append the ASCII chars (if any).
		if (t_next_ptr - p_chars > 0)
			import_html_append_native_chars(ctxt, p_chars, t_next_ptr - p_chars);
		
		// Next loop over all UTF-8 encoded chars.
		p_chars = t_next_ptr;
		while(p_char_count > 0 && (unsigned)*t_next_ptr >= 128)
			t_next_ptr++, p_char_count -= 1;
			
		// Append the UTF8 chars as unicode (if any).
		if (t_next_ptr - p_chars > 0)
		{
            // Convert UTF8 to UTF16.
            MCAutoStringRef t_string;
            /* UNCHECKED */ MCStringCreateWithBytes((const byte_t*)p_chars, t_next_ptr - p_chars, kMCStringEncodingUTF8, false, &t_string);
			
			// Append the chars one by one.
			const unichar_t *t_unicode_chars;
			uindex_t t_unicode_char_count;
			t_unicode_chars = MCStringGetCharPtr(*t_string);
			t_unicode_char_count = MCStringGetLength(*t_string);
			for(uindex_t i = 0; i < t_unicode_char_count; i++)
				import_html_append_unicode_char(ctxt, t_unicode_chars[i]);
		}
		
		p_chars = t_next_ptr;
	}
}

static void import_html_push_tag(import_html_t& ctxt, import_html_tag_type_t p_tag, MCFieldCharacterStyle& p_style)
{
    // MW-2014-08-26: [[ Bug 13256 ]] 'styles' has an implicit bottom element, so we need to ensure
    //   capacity is at least 1 greater than index.
	// Ensure there is room in the stack for a new entry.
	if (ctxt . style_index + 2 > ctxt . style_capacity)
		if (!MCMemoryResizeArray(ctxt . style_capacity * 2, ctxt . styles, ctxt . style_capacity))
			return;
			
	// Push the tag at the top of the stack.
	ctxt . style_index++;
	ctxt . styles[ctxt . style_index] . type = p_tag;
	import_html_copy_style(p_style, ctxt . styles[ctxt . style_index] . style);
}

static void import_html_pop_tag(import_html_t& ctxt, import_html_tag_type_t p_tag, bool p_all)
{
	while(ctxt . style_index > 0)
	{
		// If we aren't meant to ditch all tags and we reach a block tag then
		// we are done.
		if (!p_all && ctxt . styles[ctxt . style_index] . type < kImportHtmlTag_FirstStyleTag)
			break;
		
		// Check to see if the top of the stack matches the tag we are looking for.
		bool t_same_tag;
		t_same_tag = ctxt . styles[ctxt . style_index] . type == p_tag;
		
		// Free the style and move down the stack.
		import_html_free_style(ctxt . styles[ctxt . style_index] . style);
		ctxt . style_index -= 1;
		
		// If we have found the same tag, we are done (after popping it).
		if (t_same_tag)
			break;
	}
}

static void import_html_push_list(import_html_t& ctxt, import_html_tag_type_t p_tag, uint32_t p_style, uint32_t p_start)
{
	if (ctxt . list_depth == 16)
		return;
		
	ctxt . list_depth += 1;
	ctxt . list_in_li = false;
	ctxt . list_paragraph_count = 0;
	ctxt . list_starts[ctxt . list_depth - 1] = p_start;
	ctxt . list_styles[ctxt . list_depth - 1] = p_style;
	ctxt . list_tags[ctxt . list_depth - 1] = p_tag;
}

static void import_html_pop_list(import_html_t& ctxt, import_html_tag_type_t p_tag)
{
	ctxt . list_in_li = false;
	while(ctxt . list_depth > 0)
	{
		ctxt . list_depth -= 1;
		if (ctxt . list_tags[ctxt . list_depth] == p_tag)
			break;
	}
}

static void import_html_break(import_html_t& ctxt, bool p_keep_block_tag)
{
	if (ctxt . need_paragraph && ctxt . byte_count == 0)
		return;

	// Flush any text we've accumulated.
	import_html_flush_chars(ctxt);
		
	// Pop all tags off the stack - either as far as a block tag, or all the
	// way depending on what's been requested.
	import_html_pop_tag(ctxt, kImportHtmlTagNone, !p_keep_block_tag);
	
	// Note the fact we need to add a paragraph.
	ctxt . need_paragraph = true;
}

static void import_html_add_textstyle_to_style(MCFieldCharacterStyle& x_style, uint32_t p_text_style)
{
	if (!x_style . has_text_style)
	{
		x_style . has_text_style = true;
		x_style . text_style = FA_DEFAULT_STYLE;
	}
	
	// MW-2012-05-25: [[ Bug 10227 ]] Make sure we preserve all other flags apart from weight
	//   and expand - otherwise things like link style are lost.
	if (p_text_style == FA_BOLD)
		x_style . text_style = ((x_style . text_style & ~FA_WEIGHT) | FA_BOLD);
	else if (p_text_style == FA_CONDENSED)
		x_style . text_style = ((x_style . text_style & ~FA_EXPAND) | FA_CONDENSED);
	else if (p_text_style == FA_EXPANDED)
		x_style . text_style = ((x_style . text_style & ~FA_EXPAND) | FA_EXPANDED);
	else
		x_style . text_style |= p_text_style;
}

static void import_html_change_style(import_html_t& ctxt, const import_html_tag_t& p_tag)
{
	// Copy the most recent style.
	MCFieldCharacterStyle t_style;
	import_html_copy_style(ctxt . styles[ctxt . style_index] . style, t_style);
	
	// Now apply the effect of the tag.
	switch(p_tag . type)
	{
		case kImportHtmlTagAnchor:
		{
			bool t_is_link;
			MCStringRef t_linktext;
			t_linktext = nil;
			t_is_link = false;
			for(uint32_t i = 0; i < p_tag . attr_count; i++)
			{
				// MW-2012-03-16: [[ Bug ]] LinkText without link style is encoded with a 'name' attr.
				if (p_tag . attrs[i] . value != nil && (p_tag . attrs[i] . type == kImportHtmlAttrName || p_tag . attrs[i] . type == kImportHtmlAttrHref))
				{
					MCValueAssign(t_linktext, p_tag.attrs[i].value);
					t_is_link = p_tag . attrs[i] . type == kImportHtmlAttrHref;
				}
			}
			
			if (t_linktext != nil)
			{
				t_style . has_link_text = true;
				t_style . link_text = t_linktext;
			}
				
			// If no linktext is present, then we default to making it a link style;
			// otherwise we fall back to whether it was a name or href attr.
			if (t_linktext == nil || t_is_link)
				import_html_add_textstyle_to_style(t_style, FA_LINK);
		}
		break;
		case kImportHtmlTagImage:
		{
			MCStringRef t_src;
			t_src = nil;
			for(uint32_t i = 0; i < p_tag . attr_count; i++)
				if (p_tag . attrs[i] . type == kImportHtmlAttrSrc)
				{
					// PM-2015-02-05: [[ Bug 16853 ]] Allow spaces between <atribute_name> and "=" sign
					// Nil-check before passing it to strlen
					if (p_tag . attrs[i] . value != nil)
					{
						MCValueAssign(t_src, p_tag.attrs[i].value);
					}
				}
			
			if (t_src != nil)
			{
				t_style . has_image_source = true;
				t_style . image_source = t_src;
			}
		}
		break;
		case kImportHtmlTagFont:
			{
				for(uint32_t i = 0; i < p_tag . attr_count; i++)
				{
					switch(p_tag . attrs[i] . type)
					{
					case kImportHtmlAttrFace:
						if (p_tag . attrs[i] . value != nil)
						{
							t_style . has_text_font = true;
							if (t_style . text_font != nil)
								MCValueRelease(t_style . text_font);
							MCNameCreate(p_tag . attrs[i] . value, t_style . text_font);
						}
						break;
					case kImportHtmlAttrSize:
						if (p_tag . attrs[i] . value != nil)
						{
							uint32_t t_size;
							t_size = atoi(p_tag . attrs[i] . value);
							if (t_size != 0)
							{
								unichar_t t_first_char =
									MCStringGetCharAtIndex(p_tag.attrs[i].value,
														   0);
								if (t_first_char == '+' || t_first_char == '-')
								{
									t_size += 4;
									if (t_size < 1)
										t_size = 14;
									else if (t_size < 8)
									{
										uint2 s_size_table[] = { 8, 10, 12, 14, 17, 20, 25 };
										t_size = s_size_table[t_size - 1];
									}
								}
								t_style . has_text_size = true;
								t_style . text_size = t_size;
							}
						}
						break;
					case kImportHtmlAttrColor:
						{
							MCColor t_color;
							if (p_tag . attrs[i] . value != nil && MCscreen -> parsecolor(p_tag . attrs[i] . value, t_color, nil))
							{
								t_style . has_text_color = true;
								t_style . text_color = MCColorGetPixel(t_color);
							}
						}
						break;
					case kImportHtmlAttrBgColor:
						{
							MCColor t_color;
							if (p_tag . attrs[i] . value != nil && MCscreen -> parsecolor(p_tag . attrs[i] . value, t_color, nil))
							{
								t_style . has_background_color = true;
								t_style . background_color = MCColorGetPixel(t_color);
							}
						}
						break;
					default:
						break;
					}
				}
			}
			break;
			
		case kImportHtmlTagSub:
		case kImportHtmlTagSup:
			{
				int32_t t_shift;
				if (p_tag . type == kImportHtmlTagSub)
					t_shift = 4;
				else
					t_shift = -4;
					
				for(uint32_t i = 0; i < p_tag . attr_count; i++)
					if (p_tag . attrs[i] . type == kImportHtmlAttrShift &&
						p_tag . attrs[i] . value != nil)
						t_shift = atoi(p_tag . attrs[i] . value);
						
				t_style . has_text_shift = true;
				t_style . text_shift = t_shift;
			}
			break;
			
		// MW-2012-08-31: [[ Bug 10343 ]] Implement support for importing 'span' tags.
		case kImportHtmlTagSpan:
		{
			MCStringRef t_metadata;
			t_metadata = nil;
			for(uint32_t i = 0; i < p_tag . attr_count; i++)
				if (p_tag . attrs[i] . value != nil && p_tag . attrs[i] . type == kImportHtmlAttrMetadata)
				{
					MCValueAssign(t_metadata,
								  p_tag.attrs[i].value);
				}
			
			if (t_metadata != nil)
			{
				t_style . has_metadata = true;
				t_style . metadata = t_metadata;
			}
		}
		break;
			
		case kImportHtmlTagBold:
		case kImportHtmlTagStrong:
			import_html_add_textstyle_to_style(t_style, FA_BOLD);
			break;
		case kImportHtmlTagItalic:
		case kImportHtmlTagEm:
			import_html_add_textstyle_to_style(t_style, FA_ITALIC);
			break;
		case kImportHtmlTagUnderline:
			import_html_add_textstyle_to_style(t_style, FA_UNDERLINE);
			break;
		case kImportHtmlTagStrike:
			import_html_add_textstyle_to_style(t_style, FA_STRIKEOUT);
			break;
		case kImportHtmlTagCondensed:
			import_html_add_textstyle_to_style(t_style, FA_CONDENSED);
			break;
		case kImportHtmlTagExpanded:
			import_html_add_textstyle_to_style(t_style, FA_EXPANDED);
			break;
		case kImportHtmlTagBox:
			import_html_add_textstyle_to_style(t_style, FA_BOX);
			break;
		case kImportHtmlTagThreeDBox:
			import_html_add_textstyle_to_style(t_style, FA_3D_BOX);
			break;
		default:
			break;
	}
	
	// Now push the style and tag on the stack
	import_html_push_tag(ctxt, p_tag . type, t_style);
	
	// Free the style.
	import_html_free_style(t_style);
}

static void import_html_parse_paragraph_attrs(import_html_tag_t& p_tag, MCFieldParagraphStyle& r_style)
{
	for(uint32_t i = 0; i < p_tag . attr_count; i++)
	{
		MCStringRef t_value = p_tag.attrs[i].value;
		if (t_value == nil)
		{
			t_value = kMCEmptyString;
		}
			
		switch(p_tag . attrs[i] . type)
		{
			case kImportHtmlAttrMetadata:
				if (r_style . has_metadata)
				{
                    MCValueRelease(r_style . metadata);
					r_style . metadata = nil;
					r_style . has_metadata = false;
				}

				if (!MCStringIsEmpty(t_value))
				{
                    /* UNCHECKED */ MCValueInter(t_value, r_style . metadata);
					r_style . has_metadata = true;
				}
				break;
			case kImportHtmlAttrAlign:
				// MW-2012-05-01: [[ Bug 10183 ]] Crash and oddness when 'align' attribute is present
				//   due to using i rather than j (oops!).
				for(uint32_t j = 0; j < 4; j++)
					if (MCStringIsEqualToCString(t_value,
												 MCtextalignstrings[j],
												 kMCStringOptionCompareCaseless))
					{
						r_style . has_text_align = true;
						r_style . text_align = j;
						break;
					}
			break;
			case kImportHtmlAttrListIndent:
				r_style . has_list_indent = true;
				r_style . list_indent = atoi(t_value);
			break;
			case kImportHtmlAttrFirstIndent:
				r_style . has_first_indent = true;
				r_style . first_indent = atoi(t_value);
			break;
			case kImportHtmlAttrLeftIndent:
				r_style . has_left_indent = true;
				r_style . left_indent = atoi(t_value);
			break;
			case kImportHtmlAttrRightIndent:
				r_style . has_right_indent = true;
				r_style . right_indent = atoi(t_value);
			break;
			case kImportHtmlAttrSpaceAbove:
				r_style . has_space_above = true;
				r_style . space_above = atoi(t_value);
			break;
			case kImportHtmlAttrSpaceBelow:
				r_style . has_space_below = true;
				r_style . space_below = atoi(t_value);
			break;
			case kImportHtmlAttrTabStops:
            {
				if (r_style . has_tabs)
				{
					delete r_style . tabs;
					r_style . tabs = nil;
					r_style . has_tabs = false;
				}
				if (MCField::parsetabstops(P_TAB_STOPS, t_value, r_style . tabs, r_style . tab_count))
                {
					r_style . has_tabs = true;
                }
            }
			break;
			case kImportHtmlAttrTabAlignments:
			{
				if (r_style . has_tab_alignments)
				{
					MCMemoryDeallocate(r_style . tab_alignments);
					r_style . tab_alignments = nil;
					r_style . tab_alignment_count = 0;
				}
				if (MCField::parsetabalignments(t_value, r_style . tab_alignments, r_style . tab_alignment_count))
				{
					r_style . has_tab_alignments = true;
				}
			}
			break;
			case kImportHtmlAttrBgColor:
            {
				MCColor t_color;
				if (MCscreen -> parsecolor(t_value, t_color, nil))
				{
					r_style . has_background_color = true;
					r_style . background_color = MCColorGetPixel(t_color);
				}
            }
			break;
			case kImportHtmlAttrBorderWidth:
            
				r_style . has_border_width = true;
				r_style . border_width = atoi(t_value);
			break;
			case kImportHtmlAttrBorderColor:
            {
				MCColor t_color;
				if (MCscreen -> parsecolor(t_value, t_color, nil))
				{
					r_style . has_border_color = true;
					r_style . border_color = MCColorGetPixel(t_color);
				}
            }
			break;
			case kImportHtmlAttrPadding:
				r_style . has_padding = true;
				r_style . padding = atoi(t_value);
			break;
			case kImportHtmlAttrWrap:
			case kImportHtmlAttrNoWrap:
				r_style . has_dont_wrap = true;
				r_style . dont_wrap = (p_tag . attrs[i] . type == kImportHtmlAttrNoWrap);
			break;
			case kImportHtmlAttrVGrid:
			case kImportHtmlAttrNoVGrid:
				r_style . has_vgrid = true;
				r_style . vgrid = (p_tag . attrs[i] . type == kImportHtmlAttrVGrid);
			break;
			case kImportHtmlAttrHGrid:
			case kImportHtmlAttrNoHGrid:
				r_style . has_hgrid = true;
				r_style . hgrid = (p_tag . attrs[i] . type == kImportHtmlAttrHGrid);
			break;
			case kImportHtmlAttrHidden:
				r_style . hidden = true;
			break;
			default:
				break;
		}
	}
}

MCParagraph *MCField::importhtmltext(MCValueRef p_text)
{
    MCAutoPointer<char> t_data;
    MCAutoStringRefAsCString t_native_string;
    const char *t_ptr, *t_limit;
    bool t_is_unicode_string;
    t_is_unicode_string = false;
    
    
    MCValueTypeCode t_code = MCValueGetTypeCode(p_text);
    
    if (t_code == kMCValueTypeCodeString || t_code == kMCValueTypeCodeName)
    {
        if (t_code == kMCValueTypeCodeName)
            p_text = MCNameGetString((MCNameRef)p_text);
        
        if (!MCStringIsNative((MCStringRef)p_text))
        {
            t_is_unicode_string = true;
            uindex_t t_length;
            /* UNCHECKED */ MCStringConvertToUTF8((MCStringRef)p_text, &t_data, t_length);
            t_ptr = *t_data;
            t_limit = t_ptr + t_length;
            
        }
        else
        {
            t_native_string . Lock((MCStringRef)p_text);
            t_ptr = *t_native_string;
            t_limit = t_ptr + MCStringGetLength((MCStringRef)p_text);
        }
    }
    else if (t_code == kMCValueTypeCodeData)
    {
        t_ptr = (const char *)MCDataGetBytePtr((MCDataRef)p_text);
        t_limit = t_ptr + MCDataGetLength((MCDataRef)p_text);
    }
    else
    {
        t_ptr = "";
        t_limit = t_ptr;
    }
    
	import_html_t ctxt;
	memset(&ctxt, 0, sizeof(import_html_t));
	ctxt . field = this;
	ctxt . need_paragraph = true;
	MCMemoryResizeArray(16, ctxt . styles, ctxt . style_capacity);
	
	MCFieldCharacterStyle t_char_style;
	memset(&t_char_style, 0, sizeof(MCFieldCharacterStyle));
	import_html_push_tag(ctxt, kImportHtmlTagNone, t_char_style);
	
    if (t_is_unicode_string)
        ctxt . is_utf8 = true;
    
	// We implicitly start with a start tag (notionally).
	bool t_saw_start_tag;
	t_saw_start_tag = true;
	for(;;)
	{
		const char *t_start_ptr;
		t_start_ptr = t_ptr;
		while(t_ptr < t_limit)
		{
			// If there is a space immediately after '<' or '&' we treat them as single
			// chars rather than tag/entity prefixes.
			if (*t_ptr == '<' || *t_ptr == '&')
			{
				if (t_ptr + 1 < t_limit && t_ptr[1] != ' ')
					break;
			}
			
			if (*t_ptr == '\r' || *t_ptr == '\n')
				break;
            
			t_ptr += 1;
		}
		
		// If we found any native chars to output, do so.
		if (t_start_ptr != t_ptr)
		{
			if (!ctxt . is_utf8)
				import_html_append_native_chars(ctxt, t_start_ptr, t_ptr - t_start_ptr);
			else
				import_html_append_utf8_chars(ctxt, t_start_ptr, t_ptr - t_start_ptr);
			t_saw_start_tag = false;
		}
		
		// If we are at the end of the buffer, we are done.
		// MW-2012-04-02: [[ Bug 10124 ]] If ptr exceeds limit then we are done.
		if (t_ptr >= t_limit)
			break;
		
		if (*t_ptr == '<')
		{
			import_html_tag_t t_tag;
			
			// If the next char is the start of a tag, parse it.
			if (import_html_parse_tag(t_ptr, t_limit, t_tag))
			{
				switch(t_tag . type)
				{
                    case kImportHtmlTagUl:
                    case kImportHtmlTagOl:
                        if (!t_tag . is_terminator)
                        {
                            import_html_break(ctxt, true);
                            
                            uint32_t t_style, t_start;
                            t_style = t_tag . type == kImportHtmlTagUl ? kMCParagraphListStyleDisc : kMCParagraphListStyleNumeric;
                            t_start = 0;
                            for(uint32_t i = 0; i < t_tag . attr_count; i++)
                                if (t_tag . attrs[i] . type == kImportHtmlAttrType)
                                {
                                    for(uint32_t j = 0; s_export_html_list_types[j] != nil; j++)
                                        if (MCStringIsEqualToCString(t_tag . attrs[i] . value,
																	 s_export_html_list_types[j],
																	 kMCStringOptionCompareCaseless))
                                        {
                                            t_style = j;
                                            break;
                                        }
                                }
                                else if (t_tag . attrs[i] . type == kImportHtmlAttrStart)
                                    t_start = atoi(t_tag . attrs[i] . value);
                            
                            import_html_push_list(ctxt, t_tag . type, t_style, t_start);
                        }
                        else
                            import_html_pop_list(ctxt, t_tag . type);
                        break;
                        
                        // MW-2012-11-19: Add support for UTF-8 htmlText.
                    case kImportHtmlTagMeta:
                        if (!t_tag . is_terminator && !t_is_unicode_string)
                        {
                            bool t_is_utf8;
                            t_is_utf8 = false;
                            for(uint32_t i = 0; i < t_tag . attr_count; i++)
                                if (t_tag . attrs[i] . type == kImportHtmlAttrContent || t_tag . attrs[i] . type == kImportHtmlAttrCharset)
                                    if (MCStringContainsCString(t_tag . attrs[i] . value,
																"utf-8",
																kMCStringOptionCompareCaseless))
                                    {
                                        t_is_utf8 = true;
                                        break;
                                    }
                            
                            // If the charset of content attr contained 'utf-8' then switch
                            // to that mode.
                            ctxt . is_utf8 = t_is_utf8;
                        }
                        break;
                        
                        // The <li> tag causes a paragraph break.
                    case kImportHtmlTagLi:
                        import_html_break(ctxt, true);
                        if (!t_tag . is_terminator)
                        {
                            uint32_t t_index;
                            t_index = 0;
                            for(uint32_t i = 0; i < t_tag . attr_count; i++)
                                if (t_tag . attrs[i] . type == kImportHtmlAttrValue)
                                    t_index = atoi(t_tag . attrs[i] . value);
                            
                            ctxt . list_in_li = true;
                            ctxt . list_index = t_index;
                            ctxt . list_paragraph_count = 0;
                        }
                        else
                            ctxt . list_in_li = false;
                        break;
                        
                        // The <p> and </p> tags end any existing paragraph and
                        // replace the existing block tag.
                        // The <p> tag begins a new paragraph with styling.
                    case kImportHtmlTagP:
                        import_html_break(ctxt, true);
                        if (!t_tag . is_terminator)
                        {
                            // MW-2012-03-16: [[ Bug ]] Ignore the tag unless we have zero
                            //   list depth, or are inside an li tag.
                            if (ctxt . list_depth == 0 || ctxt . list_in_li)
                            {
                                MCFieldParagraphStyle t_style;
                                memset(&t_style, 0, sizeof(MCFieldParagraphStyle));
                                import_html_parse_paragraph_attrs(t_tag, t_style);
                                import_html_begin(ctxt, &t_style);
                                delete t_style . tabs;
                                delete t_style.tab_alignments;
                                MCValueRelease(t_style . metadata);
                            }
                        }
                        break;
                        
                        // The <br> and <hr> tags both end any existing paragraph but
                        // without popping the current block level tag off the stack
                        // (if any).
                    case kImportHtmlTagBr:
                    case kImportHtmlTagHr:
                        // MW-2012-03-12: [[ Bug ]] Hr/Br should always generate a paragraph
                        //   and continue with the same styling.
                        if (!t_tag . is_terminator)
                        {
                            // Flush any text in the buffer.
                            import_html_flush_chars(ctxt);
                            
                            // Create a new paragraph.
                            import_html_begin(ctxt, nil);
                            
                            // Get the new paragraph.
                            MCParagraph *t_new_paragraph;
                            t_new_paragraph = ctxt . paragraphs -> prev();
                            
                            // Get the previous paragraph.
                            MCParagraph *t_prev_paragraph;
                            t_prev_paragraph = t_new_paragraph -> prev();
                            
                            // If we aren't the first paragraph, then copy the attrs.
                            if (t_new_paragraph != t_prev_paragraph)
                            {
                                t_new_paragraph -> copyattrs(*t_prev_paragraph);
                                
                                // If the prev paragraph had a listStyle, then set this one
                                // to skip.
                                if (t_prev_paragraph -> getliststyle() != kMCParagraphListStyleNone)
                                    t_new_paragraph -> setliststyle(kMCParagraphListStyleSkip);
                                
                                // Make sure the list index of the new paragraph is unset.
                                t_new_paragraph -> setlistindex(0);
                            }
                        }
                        break;
                        
                        // These tags all work identically to <p> tags, except they
                        // have predefined char styles.
                    case kImportHtmlTagH1:
                    case kImportHtmlTagH2:
                    case kImportHtmlTagH3:
                    case kImportHtmlTagH4:
                    case kImportHtmlTagH5:
                    case kImportHtmlTagH6:
                    case kImportHtmlTagBlockquote:
                    case kImportHtmlTagDt:
                    case kImportHtmlTagDd:
                        import_html_break(ctxt, true);
                        if (!t_tag . is_terminator)
                        {
                            import_html_begin(ctxt, nil);
                            
                            MCFieldCharacterStyle t_tag_style;
                            t_tag_style = ctxt . styles[ctxt . style_index] . style;
                            
                            uint32_t t_font_size, t_font_style;
                            t_font_size = 0;
                            t_font_style = 0;
                            switch(t_tag . type)
                            {
                                case kImportHtmlTagH1:
                                    t_font_size = 34, t_font_style = FA_BOLD;
                                    break;
                                case kImportHtmlTagH2:
                                    t_font_size = 24, t_font_style = FA_BOLD;
                                    break;
                                case kImportHtmlTagH3:
                                    t_font_size = 18, t_font_style = FA_BOLD;
                                    break;
                                case kImportHtmlTagH4:
                                    t_font_size = 14, t_font_style = FA_BOLD;
                                    break;
                                case kImportHtmlTagH5:
                                    t_font_size = 12, t_font_style = FA_BOLD | FA_ITALIC;
                                    break;
                                case kImportHtmlTagH6:
                                    t_font_size = 10, t_font_style = FA_BOLD;
                                    break;
								default:
									break;
                            }
                            
                            if (t_font_style != 0)
                                t_tag_style . has_text_style = true, t_tag_style . text_style = t_font_style;
                            if (t_font_size != 0)
                                t_tag_style . has_text_size = true, t_tag_style . text_size = t_font_size;
							
                            import_html_push_tag(ctxt, t_tag . type, t_tag_style);
                        }
                        else
                            import_html_pop_tag(ctxt, t_tag . type, true);
                        break;
                        
                    case kImportHtmlTagPre:
                        if (!t_tag . is_terminator)
                        {
                            ctxt . preformatted = true;
                            import_html_push_tag(ctxt, kImportHtmlTagPre, ctxt . styles[ctxt . style_index] . style);
                        }
                        else
                        {
                            ctxt . preformatted = false;
                            import_html_pop_tag(ctxt, kImportHtmlTagPre, false);
                        }
                        break;
                        
                    case kImportHtmlTagImage:
                        if (!t_tag . is_terminator)
                        {
                            import_html_change_style(ctxt, t_tag);
							
							MCStringRef t_chars = nil;
                            for(uint32_t i = 0; i < t_tag . attr_count; i++)
                                if (t_tag . attrs[i] . type == kImportHtmlAttrChar)
                                    t_chars = t_tag . attrs[i] . value;
							
							if (t_chars != nil)
							{
								import_html_append_stringref(ctxt, t_chars);
							}
							else
							{
								import_html_append_native_chars(ctxt, " ", 1);
							}
							
                            import_html_pop_tag(ctxt, kImportHtmlTagImage, false);
                        }
                        break;
                        
                    case kImportHtmlTagAnchor:
                    case kImportHtmlTagFont:
                    case kImportHtmlTagSub:
                    case kImportHtmlTagSup:
                    case kImportHtmlTagBold:
                    case kImportHtmlTagItalic:
                    case kImportHtmlTagUnderline:
                    case kImportHtmlTagStrike:
                    case kImportHtmlTagCondensed:
                    case kImportHtmlTagExpanded:
                    case kImportHtmlTagBox:
                    case kImportHtmlTagThreeDBox:
                        // MW-2012-08-31: [[ Bug 10343 ]] Implement support for importing 'span' tags.
                    case kImportHtmlTagSpan:
                        // MW-2012-11-19: Add support for strong / em (bold / italic resp.)
                    case kImportHtmlTagStrong:
                    case kImportHtmlTagEm:
                        if (t_tag . is_terminator)
                            import_html_pop_tag(ctxt, t_tag . type, false);
                        else
                            import_html_change_style(ctxt, t_tag);
                        break;
					default:
						break;
				}
                
				t_saw_start_tag = !t_tag . is_terminator;
                
				import_html_free_tag(t_tag);
			}
		}
		else if (*t_ptr == '&')
		{
			// If the next char is the start of an entity, parse it.
			uint32_t t_codepoint;
			if (import_html_parse_entity(t_ptr, t_limit, t_codepoint))
			{
				// Handle entity
				import_html_append_unicode_char(ctxt, t_codepoint);
			}
			else
			{
				// MW-2012-03-16: [[ Bug ]] If the entity was not recognized emit '&' and
				//   subsequent chars.
				import_html_append_native_chars(ctxt, "&", 1);
			}
			
			t_saw_start_tag = false;
		}
		else if (*t_ptr == '\r' || *t_ptr == '\n')
		{
			// The next char is a CR, then check to see if the next char is a
			// newline and skip it if it is.
			if (*t_ptr == '\r' && (t_ptr + 1 < t_limit && t_ptr[1] == '\n'))
				t_ptr += 1;
            
			if (!ctxt . preformatted)
			{
				// MW-2013-06-21: [[ Valgrind ]] Only scan t_ptr if it is within the
				//   string.
				while(t_ptr < t_limit && (*t_ptr == '\r' || *t_ptr == '\n'))
					t_ptr += 1;
			}
			else
				t_ptr += 1;
            
			// An end tag follows if its </ or eod.
			bool t_see_end_tag;
			if (t_ptr + 1 < t_limit)
				t_see_end_tag = t_ptr[0] == '<' && t_ptr[1] == '/';
			else
				t_see_end_tag = t_ptr >= t_limit;
            
			if (!t_saw_start_tag && !t_see_end_tag)
			{
				if (ctxt . preformatted)
				{
					import_html_break(ctxt, false);
					import_html_begin(ctxt, nil);
				}
				else if (ctxt . byte_count != 0)
				{
					// Emit a space character instead.
					import_html_append_native_chars(ctxt, " ", 1);
				}
			}
			
			t_saw_start_tag = false;
		}
	}
    
	import_html_flush_chars(ctxt);
	import_html_pop_tag(ctxt, kImportHtmlTagNone, false);
	
	import_html_free_style(ctxt . last_used_style);
    
	MCMemoryDeleteArray(ctxt . bytes);
	MCMemoryDeleteArray(ctxt . styles);
    
	// MW-2012-03-09: [[ Bug ]] Make sure we always return at least an empty
	//   paragraph - in particular if the input string was empty.
	if (ctxt . paragraphs == nil)
		importparagraph(ctxt . paragraphs, nil);
	
	return ctxt . paragraphs;
}

////////////////////////////////////////////////////////////////////////////////
