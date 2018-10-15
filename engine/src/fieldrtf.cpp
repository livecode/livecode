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

#include "field.h"
#include "paragraf.h"
#include "text.h"
#include "osspec.h"

#include "mcstring.h"
#include "uidc.h"

////////////////////////////////////////////////////////////////////////////////

// Platform-specific ifdefs (mac is different from linux/windows in some ways).
#ifdef _MACOSX
#define EXPORT_RTF_PLATFORM_TAG "\\mac"
#define EXPORT_RTF_PLATFORM_CHARSET "77"
#define EXPORT_RTF_PLATFORM_DPI 72
#else
#define EXPORT_RTF_PLATFORM_TAG "\\ansi"
#define EXPORT_RTF_PLATFORM_CHARSET "0"
#define EXPORT_RTF_PLATFORM_DPI 96
#endif

// Simple class managing a set of unique (indexed) values.
template<class T> class export_rtf_table_t
{
public:
	export_rtf_table_t(void)
	{
		m_count = m_capacity = 0;
		m_array = nil;
	}

	~export_rtf_table_t(void)
	{
		MCMemoryDeleteArray(m_array);
	}

	uint32_t count(void) const
	{
		return m_count;
	}

	void add(T p_entry)
	{
		uint32_t t_index;
		if (find(p_entry, t_index))
			return;

		if (m_count + 1 > m_capacity)
			if (!MCMemoryResizeArray(m_capacity == 0 ? 16 : m_capacity * 2, m_array, m_capacity))
				return;

		if (m_count - t_index > 0)
			memmove(m_array + t_index + 1, m_array + t_index, sizeof(T) * (m_count - t_index));

		m_array[t_index] = p_entry;
		m_count += 1;
	}

	uint32_t lookup(T p_entry) const
	{
		uint32_t t_index;
		if (!find(p_entry, t_index))
			t_index = 0;
		return t_index;
	}

	T operator[] (uint32_t p_index) const
	{
		return m_array[p_index];
	}

private:
	bool find(T p_entry, uint32_t& r_index) const
	{
		for(r_index = 0; r_index < m_count; r_index++)
			if (p_entry == m_array[r_index])
				return true;

		return false;
	}

	uint32_t m_count;
	uint32_t m_capacity;
	T *m_array;
};

// Simple class managing an array of values.
template<class T> class export_rtf_array_t
{
public:
	export_rtf_array_t(void)
	{
		m_count = m_capacity = 0;
		m_array = nil;
	}

	~export_rtf_array_t(void)
	{
		MCMemoryDeleteArray(m_array);
	}

	uint32_t count(void) const
	{
		return m_count;
	}

	void append(T p_entry)
	{
		if (m_count + 1 > m_capacity)
			if (!MCMemoryResizeArray(m_capacity == 0 ? 16 : m_capacity * 2, m_array, m_capacity))
				return;

		m_array[m_count++] = p_entry;
	}


	T operator[] (uint32_t p_index) const
	{
		return m_array[p_index];
	}

private:
	uint32_t m_count;
	uint32_t m_capacity;
	T *m_array;
};

// A collection of char styles that are applied to a run.
struct export_rtf_char_style_t
{
	bool italic_on : 1;
	bool bold_on : 1;
	bool strike_on : 1;
	bool underline_on : 1;
	bool superscript_on : 1;
	bool subscript_on : 1;
	bool link_on : 1;
	int32_t font_index;
	int32_t font_size;
	int32_t text_color_index;
	int32_t background_color_index;
	MCStringRef link_text;
	MCStringRef metadata;
};

// The rtf export context structure.
struct export_rtf_t
{
	// The list of fonts that are in the font table.
	export_rtf_table_t<MCNameRef> fonts;
	// The list of colors that are in the color table.
	export_rtf_table_t<uint32_t> colors;
	// The list of list styles that are in the list table.
	export_rtf_table_t<uint32_t> lists;
	// The mapping from paragraph index to list style id (if an entry is zero
	// then the paragraph has no list styling).
	export_rtf_array_t<uint32_t> list_ids;
	// The buffer containing the output text.
	MCStringRef m_text;

	// The styles we inherit from the field (used for list labels).
	export_rtf_char_style_t parent_style;
	
	// The stack of styles currently in play. This will have at most three
	// levels at the moment, as the maximum nesting is (link, metadata, bgcolor).
	export_rtf_char_style_t styles[4];
	// The current 'top' style in the stack.
	uint32_t style_index;

	// The 0-based index of the paragraph (used to get list_ids).
	uint32_t paragraph_index;

	// The encoded list style in effect at this point.
	uint32_t list_style;

	// The current paragraph had metadata attached.
	bool has_metadata;
};

////////////////////////////////////////////////////////////////////////////////

// Convert pixels to twentieth's of a point.
static int32_t pixels_to_twips(int32_t p_pixels)
{
	return p_pixels * 20;
}

// Convert pixels to half points.
static int32_t pixels_to_half_points(int32_t p_pixels)
{
	return (p_pixels * 144) / EXPORT_RTF_PLATFORM_DPI;
}

// Fetch the depth of an encoded list style.
static uint32_t export_rtf_get_list_style_depth(uint32_t p_style)
{
	return p_style & 0xf;
}

// Get the type of list at level p_level in the given encoded list style.
static uint32_t export_rtf_get_list_style_level(uint32_t p_style, uint32_t p_level)
{
	return (((p_style >> 4) >> (p_level * 3)) & 0x7) + 1;
}

// Set the depth of the encoded list style.
static void export_rtf_set_list_style_depth(uint32_t& x_style, uint32_t p_depth)
{
	x_style = (x_style & ~0xf) | (p_depth & 0xf);
}

// Set the level of the encoded list style to the given style.
static void export_rtf_set_list_style_level(uint32_t& x_style, uint32_t p_depth, uint32_t p_list_type)
{
	x_style = (x_style & ~(0x7 << (4 + p_depth * 3))) | ((p_list_type - 1) << (4 + p_depth * 3));
}

// Compute a new encoded list style when presented with a new depth/style
// for an additional paragraph. It returns 'true' if the style has changed,
// 'false' otherwise. (If it returns true, it means 'base_style' should be
// recorded and a new list is beginning as 'new_style').
static bool export_rtf_process_list_style(uint32_t p_base_style, uint32_t p_list_style, uint32_t p_list_depth, uint32_t& r_new_style)
{
	// If this is a skip style, then do nothing.
	if (p_list_style == kMCParagraphListStyleSkip)
	{
		r_new_style = p_base_style;
		return false;
	}

	// If this paragraph has no list style set, then that terminates the list.
	if (p_list_style == kMCParagraphListStyleNone)
	{
		r_new_style = 0;

		// If the base style is 0, then this isn't terminating anything so return
		// false.
		return p_base_style != 0;
	}

	// We can emit a maximum of 9 levels into RTF.
	if (p_list_depth > 8)
		p_list_depth = 8;

	// Get the current number of levels assigned.
	uint32_t t_base_depth;
	t_base_depth = p_base_style & 0xf;

	// If this paragraph is at greater depth, then add the style in to the existing
	// list and return.
	if (p_list_depth + 1 > t_base_depth)
	{
		r_new_style = p_base_style;
		export_rtf_set_list_style_depth(r_new_style, p_list_depth + 1);
		export_rtf_set_list_style_level(r_new_style, p_list_depth, p_list_style);
		return false;
	}

	// Otherwise, we must be adding another paragraph to an existing level so check
	// the style is the same.
	if (export_rtf_get_list_style_level(p_base_style, p_list_depth) != p_list_style)
	{
		r_new_style = 0;
		return true;
	}

	// If we get here, no changes have been made to the style, so continue.
	r_new_style = p_base_style;
	return false;
}

// Accumulate all the tables needed to export the rtf.
static bool export_rtf_build_tables(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	export_rtf_t& ctxt = *(export_rtf_t *)p_context;

	if (p_event_type == kMCFieldExportEventNativeRun || p_event_type == kMCFieldExportEventUnicodeRun)
	{
		ctxt . fonts . add(p_event_data . character_style . text_font);
		ctxt . colors . add(p_event_data . character_style . text_color);
		if (p_event_data . character_style . has_background_color)
			ctxt . colors . add(p_event_data . character_style . background_color);
	}
	else if (p_event_type == kMCFieldExportEventBeginParagraph)
	{
		ctxt . colors . add(p_event_data . paragraph_style . border_color);
		if (p_event_data . paragraph_style . has_background_color)
			ctxt . colors . add(p_event_data . paragraph_style . background_color);

		uint32_t t_new_list_style;
		if (export_rtf_process_list_style(ctxt . list_style, p_event_data . paragraph_style . list_style, p_event_data . paragraph_style . list_depth, t_new_list_style))
			ctxt . lists . add(ctxt . list_style);
		ctxt . list_style = t_new_list_style;

		if (p_event_data . paragraph_style . list_style != kMCParagraphListStyleNone)
			ctxt . list_ids . append(ctxt . lists . count() + 1);
		else
			ctxt . list_ids . append(0);
	}

	return true;
}

// Emit a run of native text, taking care to encode appropriate chars
// (in particular, line break - 0x0b).
static void export_rtf_emit_native_text(MCStringRef& x_buffer, const uint8_t *p_chars, uint32_t p_char_count)
{
	for(;;)
	{
		uint32_t t_end;
		for(t_end = 0; t_end < p_char_count; t_end++)
			if (p_chars[t_end] < 0x20 ||
				p_chars[t_end] >= 0x80 ||
				p_chars[t_end] == 0x5c ||
				p_chars[t_end] == 0x7b ||
				p_chars[t_end] == 0x7d ||
				p_chars[t_end] == 0x0b)
				break;

		if (t_end > 0)
		{
			/* UNCHECKED */ MCStringAppendFormat(x_buffer, "%.*s", t_end, p_chars);
			p_char_count -= t_end;
			p_chars += t_end;
		}

		if (p_char_count == 0)
			break;

		if (*p_chars != 0x0b)
			/* UNCHECKED */ MCStringAppendFormat(x_buffer, "\\'%02X", *p_chars);
		else
			/* UNCHECKED */ MCStringAppendFormat(x_buffer, "\\line ");
		p_chars += 1;
		p_char_count -= 1;
	}
}

// Clear an rtf char style.
static void export_rtf_clear_char_style(export_rtf_char_style_t& x_style)
{
	x_style . italic_on = false;
	x_style . bold_on = false;
	x_style . underline_on = false;
	x_style . strike_on = false;
	x_style . superscript_on = false;
	x_style . subscript_on = false;
	x_style . font_index = -1;
	x_style . font_size = -1;
	x_style . text_color_index = -1;
	x_style . background_color_index = -1;
	x_style . link_text = nil;
	x_style . metadata = nil;
}

// Fetch an rtf char style from a field char style.
static void export_rtf_fetch_char_style(export_rtf_t& ctxt, export_rtf_char_style_t& r_style, const MCFieldCharacterStyle& p_field_style)
{
	r_style . italic_on = (p_field_style . text_style & (FA_ITALIC | FA_OBLIQUE)) != 0;
	r_style . bold_on = (p_field_style . text_style & FA_WEIGHT) == (FA_BOLD & FA_WEIGHT);
	r_style . strike_on = (p_field_style . text_style & FA_STRIKEOUT) != 0;
	r_style . underline_on = (p_field_style . text_style & FA_UNDERLINE) != 0;
	r_style . superscript_on = p_field_style . text_shift > 0;
	r_style . subscript_on = p_field_style . text_shift < 0;
	r_style . link_on = (p_field_style . text_style & FA_LINK) != 0;
	r_style . font_index = ctxt . fonts . lookup(p_field_style . text_font);
	r_style . font_size = pixels_to_half_points(p_field_style . text_size);
	r_style . text_color_index = ctxt . colors . lookup(p_field_style . text_color);
	
	if (p_field_style . has_link_text)
		r_style . link_text = p_field_style . link_text;
	else
		r_style . link_text = nil;
	
	if (p_field_style . has_metadata)
		r_style . metadata = p_field_style . metadata;
	else
		r_style . metadata = nil;
	
	if (p_field_style . has_background_color)
		r_style . background_color_index = ctxt . colors . lookup(p_field_style . background_color);
	else
		r_style . background_color_index = -1;
}

// Emit the changes in char style from old to new.
static void export_rtf_emit_char_style_changes(MCStringRef p_buffer, export_rtf_char_style_t& p_old, export_rtf_char_style_t& p_new)
{
	if (p_new . italic_on != p_old . italic_on)
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, p_new . italic_on ? "\\i " : "\\i0 ");
	if (p_new . bold_on != p_old . bold_on)
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, p_new . bold_on ? "\\b " : "\\b0 ");
	if (p_new . strike_on != p_old . strike_on)
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, p_new . strike_on ? "\\strike " : "\\strike0 ");
	// MW-2012-03-13: [[ Bug ]] Incorrect tag used for underline - should be 'ul' not 'u'.
	if (p_new . underline_on != p_old . underline_on)
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, p_new . underline_on ? "\\ul " : "\\ul0 ");
	if (p_new . superscript_on != p_old . superscript_on || p_new . subscript_on != p_old . subscript_on)
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, p_new . subscript_on ? "\\sub " : (p_new . subscript_on ? "\\super " : "\\nosupersub "));
	if (p_new . font_index != p_old . font_index)
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, "\\f%d ", p_new . font_index);
	if (p_new . font_size != p_old . font_size)
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, "\\fs%d ", p_new . font_size);
	if (p_new . text_color_index != p_old . text_color_index)
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, "\\cf%d ", p_new . text_color_index + 1);
	if (p_new . background_color_index != p_old . background_color_index && p_new . background_color_index != -1)
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, "\\cb%d\\chcbpat%d ", p_new . background_color_index + 1, p_new . background_color_index + 1);
}

// Properly close any open style.
static void close_open_styles(export_rtf_t& ctxt)
{
    // We close all the previous styles when a link is over
    while(ctxt . style_index > 0)
    {
        if (ctxt . styles[ctxt . style_index - 1] . metadata != ctxt . styles[ctxt.style_index] . metadata)
            /* UNCHECKED */ MCStringAppend(ctxt . m_text, MCSTR("}}"));
        
        if (ctxt . styles[ctxt . style_index - 1] . link_text != ctxt . styles[ctxt.style_index] . link_text)
            /* UNCHECKED */ MCStringAppend(ctxt . m_text, MCSTR("}}"));
        
        if (ctxt . styles[ctxt . style_index - 1] . background_color_index != ctxt . styles[ctxt.style_index] . background_color_index)
            /* UNCHECKED */ MCStringAppend(ctxt . m_text, MCSTR("}"));
        
        ctxt . style_index -= 1;
    }
}

// Emit the paragraph content. This is the second pass of rtf generation, the
// first pass constructs the necessary tables.
static bool export_rtf_emit_paragraphs(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	export_rtf_t& ctxt = *(export_rtf_t *)p_context;

	// If this is the first paragraph, then emit the tables first.
	if (p_event_type == kMCFieldExportEventBeginParagraph && p_event_data . is_first_paragraph)
	{
		// Output the inital info.
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\rtf1" EXPORT_RTF_PLATFORM_TAG " ");
		
		// Output the font table.
		if (ctxt . fonts . count() > 0)
		{
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\fonttbl");
			for(uint32_t i = 0; i < ctxt . fonts . count(); i++)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\f%d\\fnil \\fcharset" EXPORT_RTF_PLATFORM_CHARSET " %@;}", i, MCNameGetString(ctxt.fonts[i]));
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "}\n");
		}
		
		// Output the color table.
		if (ctxt . colors . count() > 0)
		{
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\colortbl;");
			for(uint32_t i = 0; i < ctxt . colors . count(); i++)
            {
                // SN-2015-11-19: [[ Bug 16451 ]] MCscreen knows better than
                // us how to unpack a colour pixel.
                MCColor t_color;
				MCColorSetPixel(t_color, ctxt . colors[i]);
                /* UNCHECKED */ MCStringAppendFormat(ctxt.m_text,
                                                     "\\red%d\\green%d\\blue%d;",
                                                     MCClamp(t_color.red >> 8, 0, 255),
                                                     MCClamp(t_color.green >> 8, 0, 255),
                                                     MCClamp(t_color.blue >> 8, 0, 255));
            }
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "}\n");
		}
		
		// Output the list table.
		if (ctxt . lists . count() > 0)
		{
			// Fetch the char rtf styles that are inherited (for listtext labels).
			export_rtf_fetch_char_style(ctxt, ctxt . parent_style, p_event_data . character_style);
			
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\*\\listtable");
			for(uint32_t i = 0; i < ctxt . lists . count(); i++)
			{
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\list\n");
				for(uint32_t j = 0; j < export_rtf_get_list_style_depth(ctxt . lists[i]); j++)
				{
					uint32_t t_list_style;
					t_list_style = export_rtf_get_list_style_level(ctxt . lists[i], j);
					if (t_list_style < kMCParagraphListStyleNumeric)
					{
						int32_t t_char;
						const char *t_marker;
						if (t_list_style == kMCParagraphListStyleDisc)
							t_char = 0x2022, t_marker = "disc";
						else if (t_list_style == kMCParagraphListStyleSquare)
							t_char = 0x25AA, t_marker = "square";
						else if (t_list_style == kMCParagraphListStyleCircle)
							t_char = 0x25E6, t_marker = "circle";
                        else // kMCParagraphListStyleNone
                        {
                            MCAssert(t_list_style == kMCParagraphListStyleNone);
                            return false;
                        }
						/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\listlevel\\levelnfc23\\leveljc0\\levelstartat1\\levelfollow0{\\*\\levelmarker \\{%s\\}}{\\leveltext\\'01\\u%d.;}{\\levelnumbers;}",
													t_marker, t_char);
					}
					else
					{
						int32_t t_type;
						switch(t_list_style)
						{
							case kMCParagraphListStyleNumeric: t_type = 0; break;
							case kMCParagraphListStyleLowerCase: t_type = 4; break;
							case kMCParagraphListStyleUpperCase: t_type = 3; break;
							case kMCParagraphListStyleLowerRoman: t_type = 2; break;
							case kMCParagraphListStyleUpperRoman: t_type = 1; break;
						}
						/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\listlevel\\levelnfc%d\\leveljc0\\levelstartat1\\levelfollow0{\\leveltext\'02\'0%d.;}{\\levelnumbers\'01;}", t_type, j);
					}
					
					export_rtf_emit_char_style_changes(ctxt.m_text, ctxt . styles[0], ctxt . parent_style);
					
					/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "}\n");
				}
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\listid%d}\n", i + 1);
			}
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "}\n");
		
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\*\\listoverridetable");
			for(uint32_t i = 0; i < ctxt . lists . count(); i++)
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\listoverride\\listid%d\\listoverridecount0\\ls%d}\n",
													 i + 1, i + 1);
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "}\n");
		}		
	}
	
	if (p_event_type == kMCFieldExportEventBeginParagraph)
	{
		// If we aren't on the first paragraph, then emit a paragraph separator.
		if (!p_event_data . is_first_paragraph)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\par ");

		// Clear all the paragraph properties.
		/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\pard ");
		
		// If we have paragraph metadata set, then emit that field.
		if (p_event_data . paragraph_style . has_metadata)
		{
			// TODO: paragraph metadata roundtrip - this clause breaks things at the moment :(
			//ctxt . buffer . appendtextf("{{\\field{\\*\\fldinst LCLINEMETADATA \"%s\"}}", MCNameGetCString(p_event_data . paragraph_style . metadata));
			//ctxt . has_metadata = true;
			// MW-2014-03-12: [[ Bug 11769 ]] We haven't emitted any metadata (as it doesn't work right yet) so
			//   best not to emit an end brace!
			ctxt . has_metadata = false;
		}
		else
			ctxt . has_metadata = false;

		// If we have a background color set, then emit a 'cbpat' control. Note that
		// putting this control later on breaks in openoffice :(
		if (p_event_data . paragraph_style . has_background_color)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\cbpat%d ", ctxt.colors.lookup(p_event_data.paragraph_style.background_color) + 1);
		
		// If we have a border set, then emit the border controls.
		if (p_event_data . paragraph_style . border_width != 0)
		{
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\box\\brdrw%d\\brdrcf%d\\brdsp%d ",
											pixels_to_twips(p_event_data . paragraph_style . border_width),
											ctxt . colors . lookup(p_event_data . paragraph_style . border_color) + 1,
											pixels_to_twips(p_event_data . paragraph_style . padding));
		}
		
		// Compute the left / first indent depending on whether the paragraph has a list
		// style or not.
		int32_t t_left_indent, t_first_indent;
		t_left_indent = 0;
		t_first_indent = 0;
		if (p_event_data . paragraph_style . list_style == kMCParagraphListStyleNone)
		{
			// If no list style, then just use the (effective) settings.
			t_left_indent = p_event_data . paragraph_style . left_indent;
			t_first_indent = p_event_data . paragraph_style . first_indent;
		}
		else
		{
			if (p_event_data . paragraph_style . has_list_indent)
			{
				// If there is a list indent, then left indent is a multiple of it (based on
				// depth).
				t_left_indent = p_event_data . paragraph_style . left_indent + (p_event_data . paragraph_style . list_depth + 1) * p_event_data . paragraph_style . list_indent;
				t_first_indent = -(signed)MCAbs(p_event_data . paragraph_style . list_indent);
			}
			else
			{
				// Otherwise, the user has overriden things, so use left / first indent
				// directly.
				t_left_indent = p_event_data . paragraph_style . left_indent;
				t_first_indent = -(signed)MCAbs(p_event_data . paragraph_style . first_indent);
			}
		}
		
		// Emit the controls for first / left / right indent.
		if (t_first_indent != 0)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\fi%d ", pixels_to_twips(t_first_indent));
		if (t_left_indent != 0)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\li%d ", pixels_to_twips(t_left_indent));
		if (p_event_data . paragraph_style . right_indent != 0)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\ri%d ", pixels_to_twips(p_event_data.paragraph_style.right_indent));
		
		// If we have a list style assigned to this paragraph, then process it.
		if (ctxt . list_ids[ctxt . paragraph_index] != 0)
		{
			// Emit a tab-stop at the text indent level.
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\tx%d", pixels_to_twips(-t_first_indent));
			
			// Emit the level and list style controls.
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\ls%d", ctxt.list_ids[ctxt.paragraph_index]);
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\ilvl%d", MCMin(p_event_data.paragraph_style.list_depth, 8U));
			
			// Emit the tag prefix and styling.
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{\\listtext\\tab");
			export_rtf_emit_char_style_changes(ctxt.m_text, ctxt . styles[ctxt . style_index], ctxt . parent_style);
			
			// Now fetch the list style of the current paragraph and output the
			// appropriate 'listtext' tag.
			uint32_t t_list_style;
			t_list_style = p_event_data . paragraph_style . list_style;
			switch(t_list_style)
			{
			case kMCParagraphListStyleDisc:
			case kMCParagraphListStyleCircle:
			case kMCParagraphListStyleSquare:
			{
				// Emit the correct bullet char in the 'listtext' section.
				uint32_t t_char;
				t_char = (t_list_style == kMCParagraphListStyleDisc ? 0x2022 : (t_list_style == kMCParagraphListStyleCircle ? 0x25E6 : 0x25AA));
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\u%d.\\tab}", t_char);			
			}
			break;
			case kMCParagraphListStyleNumeric:
			case kMCParagraphListStyleUpperRoman:
			case kMCParagraphListStyleLowerRoman:
			case kMCParagraphListStyleUpperCase:
			case kMCParagraphListStyleLowerCase:
			{
				// Format the label using the paragraph number supplied by the export process
				// and emit it in the 'listtext' section.
				char t_label_buffer[PG_MAX_INDEX_SIZE];
				const char *t_label_string;
				uint32_t t_label_length;
				MCParagraph::formatliststyleindex(t_list_style, p_event_data . paragraph_number, t_label_buffer, t_label_string, t_label_length);
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, " %.*s\\tab}", t_label_length, t_label_string);
			}
			break;
			case kMCParagraphListStyleSkip:
				// Just emit a tab in the listtext section. This causes the paragraph to be
				// aligned after an (invisible) number.
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\tab}");
				break;
			}
		}

		// Emit the appropriate alignment tag.
		switch(p_event_data . paragraph_style . text_align)
		{
		case kMCParagraphTextAlignLeft:
		case kMCParagraphTextAlignJustify:
			break;
		case kMCParagraphTextAlignRight:
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\qr ");
			break;
		case kMCParagraphTextAlignCenter:
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\qc ");
			break;
		}

		// Emit the spacing above and below controls.
		// NOTE: \sa is "space after" and \sb "space before" (opposite of "above" and "below")
		if (p_event_data . paragraph_style . space_above != 0)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\sb%d ", pixels_to_twips(p_event_data.paragraph_style.space_above));
		if (p_event_data . paragraph_style . space_below != 0)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\sa%d ", pixels_to_twips(p_event_data.paragraph_style.space_below));

		// Increment the paragraph index.
		ctxt . paragraph_index += 1;
	}
	else if (p_event_type == kMCFieldExportEventEndParagraph)
	{
        close_open_styles(ctxt);
		
		if (ctxt . has_metadata)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "}");

		// For neatness, emit a newline after each paragraph.
		if (!p_event_data . is_last_paragraph)
			/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\n");
	}
	else if (p_event_type == kMCFieldExportEventNativeRun || p_event_type == kMCFieldExportEventUnicodeRun)
	{
		// Fetch the new (basic) textstyles - these are ones which can be turned on and off
		// with tags.
		export_rtf_char_style_t t_new_style;
		export_rtf_fetch_char_style(ctxt, t_new_style, p_event_data . character_style);

        // SN-2015-11-17: [[ Bug 16308 ]] Fix RTF export with styles
        // For any change in either the background colour, the metadata or the
        // link associated with the text, we want to reset the style and start
        // with all of them synchronised.
        // That may not be the most efficient way, but that allows styles to
        // overlap without any friction.
        if (t_new_style . link_text != ctxt . styles[ctxt . style_index] . link_text
                || t_new_style . metadata != ctxt . styles[ctxt . style_index] . metadata
                || t_new_style . background_color_index != ctxt . styles[ctxt . style_index] . background_color_index)
            close_open_styles(ctxt);
        
		// Handle a change in link text.
		if (t_new_style . link_text != ctxt . styles[ctxt . style_index] . link_text)
        {
			if (t_new_style . link_text != nil)
			{
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text,
													 "{\\field{\\*\\fldinst %s \"%@\"}{\\fldrslt ",
													 t_new_style . link_on ? "HYPERLINK" : "LCANCHOR",
													 t_new_style . link_text);
				ctxt . styles[ctxt . style_index + 1] = ctxt . styles[ctxt . style_index];
                ctxt . styles[ctxt . style_index + 1] . link_text = t_new_style . link_text;
				ctxt . style_index += 1;
			}
		}

		// Handle a change in metadata.
		if (t_new_style . metadata != ctxt . styles[ctxt . style_index] . metadata)
		{
			if (t_new_style . metadata != nil)
			{
				/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, 
													 "{\\field{\\*\\fldinst LCMETADATA \"%@\"}{\\fldrslt ", 
													 t_new_style . metadata);
				ctxt . styles[ctxt . style_index + 1] = ctxt . styles[ctxt . style_index];
                ctxt . styles[ctxt . style_index + 1] . metadata = t_new_style . metadata;
				ctxt . style_index += 1;
			}
		}

		// Handle a change in background color.
		if (t_new_style . background_color_index != ctxt . styles[ctxt . style_index] . background_color_index)
		{
            if (t_new_style . background_color_index != -1)
			{
                /* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "{");
				ctxt . styles[ctxt . style_index + 1] = ctxt . styles[ctxt . style_index];
				ctxt . style_index += 1;
			}
		}

		// Emit any style changes.
		export_rtf_emit_char_style_changes(ctxt.m_text, ctxt . styles[ctxt . style_index], t_new_style);
		ctxt . styles[ctxt . style_index] = t_new_style;

		// Now emit the text, if native its easy, otherwise we must process.
		if (MCStringIsNative(p_event_data.m_text))
			export_rtf_emit_native_text(ctxt.m_text,
										MCStringGetNativeCharPtr(p_event_data.m_text) + p_event_data.m_range.offset, 
										p_event_data.m_range.length);
		else
		{
			const unichar_t *t_chars;
			uint32_t t_char_count;
			t_chars = MCStringGetCharPtr(p_event_data.m_text) + p_event_data.m_range.offset;
			t_char_count = p_event_data.m_range.length;

			// A temporary buffer to store any native converted text in.
			uint8_t *t_native_text;
			t_native_text = new (nothrow) uint8_t[t_char_count];

			// Loop until we are done.
			while(t_char_count > 0)
			{
				// Scan for the next replacement character
				
				
				// Attempt to convert as much as possible to native text. This returns
				// the number of unicode chars consumed in 'used', and the number of native
				// chars created in made.
				uint32_t t_made, t_used;
				MCTextRunnify(t_chars, t_char_count, t_native_text, t_used, t_made);

				// If made is 0 then we have 'used' unicode chars to emit; otherwise we
				// have 'made' native chars to emit.
				if (t_made == 0)
					for(uint32_t i = 0; i < t_used; i++)
						/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\\u%d?", t_chars[i]);
				else
					export_rtf_emit_native_text(ctxt.m_text, t_native_text, t_made);
			
				// Update the char / char_count.
				t_chars += t_used;
				t_char_count -= t_used;
			}
		
			// Delete the temporary buffer.
			delete t_native_text;
		}
	}

	return true;
}

// MW-2012-02-29: [[ FieldExport ]] New RTF export method.
bool MCField::exportasrtftext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, MCStringRef &r_string)
{
	return exportasrtftext(resolveparagraphs(p_part_id), p_start_index, p_finish_index, r_string);
}

/* UNSAFE */ bool MCField::exportasrtftext(MCParagraph *p_paragraphs, int32_t p_start_index, int32_t p_finish_index, MCStringRef &r_string)
{
	export_rtf_t ctxt;
	
	// Create the text buffer
	/* UNCHECKED */ MCStringCreateMutable(0, ctxt.m_text);

	// Reset the list-style info.
	ctxt . list_style = 0;

	// Execute the first pass which collects all the data that's needed in the header - the font table,
	// color table and list table.
	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns | kMCFieldExportParagraphStyles | kMCFieldExportCharacterStyles | kMCFieldExportFlattenStyles, p_paragraphs, p_start_index, p_finish_index, export_rtf_build_tables, &ctxt);

	// Add the final list style, if any.
	if (ctxt . list_style != 0)
		ctxt . lists . add(ctxt . list_style);
	
	// Setup the initial format settings.
	for(uint32_t i = 0; i < 4; i++)
		export_rtf_clear_char_style(ctxt . styles[i]);
	ctxt . style_index = 0;
	ctxt . list_style = 0;
	ctxt . paragraph_index = 0;

	// Now execute the second pass which generates all the paragraph content.
	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns | kMCFieldExportParagraphStyles | kMCFieldExportCharacterStyles | kMCFieldExportFlattenStyles | kMCFieldExportNumbering, p_paragraphs, p_start_index, p_finish_index, export_rtf_emit_paragraphs, &ctxt);

	// Output the final info.
	/* UNCHECKED */ MCStringAppendFormat(ctxt.m_text, "\n}");

	// Return the buffer.
	if (!MCStringCopyAndRelease(ctxt.m_text, r_string))
    {
        MCValueRelease(ctxt . m_text);
        return false;
    }
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
