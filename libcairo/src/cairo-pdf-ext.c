/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2004 Red Hat, Inc
 * Copyright © 2006 Red Hat, Inc
 * Copyright © 2007, 2008 Adrian Johnson
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Kristian Høgsberg <krh@redhat.com>
 *	Carl Worth <cworth@cworth.org>
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#include "cairoint.h"

#include "cairo-pdf-ext-object.h"
#include "cairo-pdf-ext-private.h"
#include "cairo-output-stream-private.h"

#include <string.h>
#include <stdio.h>


struct _escape_char_map
{
	char special_char;
	char escape_char;
};

static struct _escape_char_map s_escape_chars[] = {
	{'\n', 'n'},
	{'\r', 'r'},
	{'\t', 't'},
	{'\b', 'b'},
	{'\f', 'f'},
	{'(', '('},
	{')', ')'},
	{'\\', '\\'},
};

int
_cairo_pdf_escape_in_string(char p_char, char *r_escape_seq)
{
	int i;
	for (i = 0; i < (sizeof(s_escape_chars) / sizeof(struct _escape_char_map)); i++)
	{
		if (p_char == s_escape_chars[i].special_char)
		{
			*r_escape_seq = s_escape_chars[i].escape_char;
			return TRUE;
		}
	}

	return FALSE;
}

int
_cairo_pdf_allowed_in_string(char p_char, char *r_)
{
	return (p_char != '\\' && p_char != '(' && p_char != ')');
}

int
_cairo_pdf_allowed_in_name(char p_char)
{
	return (p_char != '#') &&
		(p_char >= '!' && p_char <= '~');
}


////////////////////////////////////////////////////////////////////////////////
//
// streaming
//

void
_cairo_pdf_output_stream_write_text(cairo_output_stream_t *p_stream, const char *p_text, int p_length)
{
	char t_escape_char, t_begin_char, t_end_char;
	char t_escape_seq;
	int i;

	t_escape_char = '\\';
	t_begin_char = '(';
	t_end_char = ')';

	_cairo_output_stream_write(p_stream, &t_begin_char, 1);
	if (p_text != NULL)
	{
		for (i = 0; i < p_length; i++)
		{
			if (_cairo_pdf_escape_in_string(p_text[i], &t_escape_seq))
			{
				_cairo_output_stream_write(p_stream, &t_escape_char, 1);
				_cairo_output_stream_write(p_stream, &t_escape_seq, 1);
			}
			else
				_cairo_output_stream_write(p_stream, &p_text[i], 1);
		}
	}
	_cairo_output_stream_write(p_stream, &t_end_char, 1);
}

void
_cairo_pdf_output_stream_write_string(cairo_output_stream_t *p_stream, const cairo_pdf_string_t *p_string)
{
	_cairo_pdf_output_stream_write_text(p_stream, p_string->data, p_string->length);
}

void
_cairo_pdf_output_stream_write_name(cairo_output_stream_t *p_stream, const char *p_name)
{
	const char *t_in_ptr;
	char t_escape_char, t_begin_char;

	t_in_ptr = p_name;

	t_escape_char = '#';
	t_begin_char = '/';

	_cairo_output_stream_write(p_stream, &t_begin_char, 1);
	if (t_in_ptr != NULL)
	{
		while (*t_in_ptr)
		{
			if (!_cairo_pdf_allowed_in_name(*t_in_ptr))
				_cairo_output_stream_printf(p_stream, "%c%02x", t_escape_char, *t_in_ptr);
			else
				_cairo_output_stream_write(p_stream, t_in_ptr, 1);
			t_in_ptr++;
		}
	}
}

void
_cairo_pdf_output_stream_write_array(cairo_output_stream_t *p_stream, const cairo_pdf_array_t *p_array)
{
	int i;
	_cairo_output_stream_printf(p_stream, "[");
	for (i = 0; i < p_array->size; i++)
	{
		if (i > 0)
			_cairo_output_stream_printf(p_stream, " ");
		_cairo_pdf_output_stream_write_object(p_stream, &p_array->elements[i]);
	}
	_cairo_output_stream_printf(p_stream, "]");
}

void
_cairo_pdf_output_stream_write_dictionary(cairo_output_stream_t *p_stream, const cairo_pdf_dictionary_t *p_dict)
{
	int32_t i;
	_cairo_output_stream_printf(p_stream, "<< ");
	for (i = 0; i < p_dict->size; i++)
	{
		_cairo_pdf_output_stream_write_name(p_stream, p_dict->keys[i]);
		_cairo_output_stream_printf(p_stream, " ");
		_cairo_pdf_output_stream_write_object(p_stream, &p_dict->elements[i]);
		_cairo_output_stream_printf(p_stream, "\n");
	}
	_cairo_output_stream_printf(p_stream, ">>\n");
}

void
_cairo_pdf_output_stream_write_reference(cairo_output_stream_t *p_stream, const cairo_pdf_reference_t *p_ref)
{
	_cairo_output_stream_printf(p_stream, "%d %d R", p_ref->id, p_ref->generation);
}

void
_cairo_pdf_output_stream_write_action(cairo_output_stream_t *p_stream, const cairo_pdf_action_t *p_action)
{
	switch (p_action->type)
	{
	case CAIRO_PDF_ACTION_TYPE_URI:
		_cairo_output_stream_printf(p_stream,
			"<< /Type /Action\n"
			"   /S /URI\n"
			"   /URI ");
		_cairo_pdf_output_stream_write_text(p_stream, p_action->uri.uri, strlen(p_action->uri.uri));
		if (p_action->uri.is_map)
		{
			_cairo_output_stream_printf(p_stream, "\n"
				"   /IsMap true");
		}
		_cairo_output_stream_printf(p_stream, "\n"
			">>\n");

		break;
	}
}

void
_cairo_pdf_output_stream_write_annotation(cairo_output_stream_t *p_stream, const cairo_pdf_annotation_t *p_annotation)
{
	// MW-2011-01-20: Change to use /Border tag as this works on all readers.
	//   ('/BS' is PDF-1.6+)
	switch (p_annotation->type)
	{
	case CAIRO_PDF_ANNOTATION_TYPE_LINK:
		if (p_annotation->link.dest != NULL)
		{ 
			_cairo_output_stream_printf(p_stream,
				"<< /Type /Annot\n"
				"   /Subtype /Link\n"
				"   /Rect [%f %f %f %f]\n"
				"	/Border [0 0 0]\n"
				//"   /BS <</Type /Border /W 0>>\n"
				"   /Dest ",
				p_annotation->rect.x,
				p_annotation->rect.y,
				p_annotation->rect.x + p_annotation->rect.width,
				p_annotation->rect.y + p_annotation->rect.height);
			_cairo_pdf_output_stream_write_name(p_stream, p_annotation->link.dest);
			_cairo_output_stream_printf(p_stream, "\n"
				">>\n");
		}
		else if (p_annotation->link.uri != NULL)
		{
			_cairo_output_stream_printf(p_stream,
				"<< /Type /Annot\n"
				"   /Subtype /Link\n"
				"   /Rect [%f %f %f %f]\n"
				"	/Border [0 0 0]\n"
				// "   /BS <</Type /Border /W 0>>\n"
				"   /A %d %d R\n"
				">>\n",
				p_annotation->rect.x,
				p_annotation->rect.y,
				p_annotation->rect.x + p_annotation->rect.width,
				p_annotation->rect.y + p_annotation->rect.height,
				p_annotation->link.action.id, p_annotation->link.action.generation);
		}
	}
}

void
_cairo_pdf_output_stream_write_date(cairo_output_stream_t *p_stream, const cairo_pdf_datetime_t *p_date)
{
	// format "(YYYYMMDDHHmmSSOHH'mm)" OHH'mm is offset (with sign +/-, or Z if zero) from UTC

	_cairo_output_stream_printf(p_stream, "(%04d%02d%02d%02d%02d%02d", p_date->year, p_date->month, p_date->day, p_date->hour, p_date->minute, p_date->second);

	if (p_date->utc_minute_offset == 0)
		_cairo_output_stream_printf(p_stream, "Z");
	else
	{
		int32_t t_utc_offset;
		if (p_date->utc_minute_offset > 0)
		{
			t_utc_offset = p_date->utc_minute_offset;
			_cairo_output_stream_printf(p_stream, "+");
		}
		else
		{
			t_utc_offset = -p_date->utc_minute_offset;
			_cairo_output_stream_printf(p_stream, "-");
		}
		_cairo_output_stream_printf(p_stream, "%02d", t_utc_offset / 60);
		if (t_utc_offset % 60 != 0)
			_cairo_output_stream_printf(p_stream, "%02d", t_utc_offset % 60);
	}
	_cairo_output_stream_printf(p_stream, ")");
}

void
_cairo_pdf_output_stream_write_dest(cairo_output_stream_t *p_stream, const cairo_pdf_dest_t *p_dest)
{
	static const char *s_dest_type_string[] = {
		"XYZ",
		"Fit",
		"FitH",
		"FitV",
		"FitR",
		"FitB",
		"FitBH",
		"FitBV",
	};

	_cairo_output_stream_printf(p_stream, "[%d /%s", p_dest->page, s_dest_type_string[p_dest->type]);

	switch (p_dest->type)
	{
	case CAIRO_PDF_DEST_TYPE_XYZ:
		// args: left top zoom
		_cairo_output_stream_printf(p_stream, " %f %f %f", p_dest->left, p_dest->top, p_dest->zoom);
		break;
	case CAIRO_PDF_DEST_TYPE_FIT_H:
	case CAIRO_PDF_DEST_TYPE_FIT_BH:
		// args top
		_cairo_output_stream_printf(p_stream, " %f", p_dest->top);
		break;
	case CAIRO_PDF_DEST_TYPE_FIT_V:
	case CAIRO_PDF_DEST_TYPE_FIT_BV:
		// args left
		_cairo_output_stream_printf(p_stream, " %f", p_dest->left);
		break;
	case CAIRO_PDF_DEST_TYPE_FIT_R:
		// args left bottom right top
		_cairo_output_stream_printf(p_stream, " %f %f %f %f", p_dest->left, p_dest->bottom, p_dest->right, p_dest->top);
		break;
	case CAIRO_PDF_DEST_TYPE_FIT:
	case CAIRO_PDF_DEST_TYPE_FIT_B:
		// no args
		break;
	}

	_cairo_output_stream_printf(p_stream, "]");
}

void
_cairo_pdf_output_stream_write_object(cairo_output_stream_t *p_stream, const cairo_pdf_object_t *p_object)
{
	if (p_object != NULL)
	{
		switch (p_object->type)
		{
		case CAIRO_PDF_OBJECT_TYPE_STRING:
			_cairo_pdf_output_stream_write_string(p_stream, &p_object->string);
			break;
		case CAIRO_PDF_OBJECT_TYPE_NAME:
			_cairo_pdf_output_stream_write_name(p_stream, p_object->name);
			break;
		case CAIRO_PDF_OBJECT_TYPE_ARRAY:
			_cairo_pdf_output_stream_write_array(p_stream, &p_object->array);
			break;
		case CAIRO_PDF_OBJECT_TYPE_DICTIONARY:
			_cairo_pdf_output_stream_write_dictionary(p_stream, &p_object->dictionary);
			break;

		case CAIRO_PDF_OBJECT_TYPE_REFERENCE:
			_cairo_pdf_output_stream_write_reference(p_stream, &p_object->reference);
			break;

		case CAIRO_PDF_OBJECT_TYPE_ACTION:
			_cairo_pdf_output_stream_write_action(p_stream, &p_object->action);
			break;
		case CAIRO_PDF_OBJECT_TYPE_ANNOTATION:
			_cairo_pdf_output_stream_write_annotation(p_stream, &p_object->annotation);
			break;
		case CAIRO_PDF_OBJECT_TYPE_DATE:
			_cairo_pdf_output_stream_write_date(p_stream, &p_object->date);
			break;
		case CAIRO_PDF_OBJECT_TYPE_DEST:
			_cairo_pdf_output_stream_write_dest(p_stream, &p_object->dest);
			break;
		default:
			assert(0);
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// object initialization
//

void
_cairo_pdf_object_string_init(cairo_pdf_string_t *p_string)
{
	p_string->_buffer = NULL;
	p_string->data = NULL;
	p_string->length = 0;
}

void
_cairo_pdf_object_name_init(cairo_pdf_name_t *p_name)
{
	*p_name = NULL;
}

void
_cairo_pdf_object_array_init(cairo_pdf_array_t *p_array)
{
	p_array->elements = NULL;
	p_array->size = 0;
}

void
_cairo_pdf_object_dictionary_init(cairo_pdf_dictionary_t *p_dict)
{
	p_dict->keys = NULL;
	p_dict->elements = NULL;
	p_dict->size = 0;
}

void
_cairo_pdf_object_outline_entry_init(cairo_pdf_outline_entry_t *p_entry)
{
	p_entry->depth = 1;
	p_entry->closed = FALSE;
	_cairo_pdf_object_string_init(&p_entry->title);
	_cairo_pdf_object_dest_set_xyz(&p_entry->destination, 0, 0, 0, 0);
	p_entry->parent = -1;
	p_entry->prev = -1;
	p_entry->next = -1;
	p_entry->first = -1;
	p_entry->last = -1;
}

void
_cairo_pdf_object_init(cairo_pdf_object_t *p_object, cairo_pdf_object_type_t p_type)
{
	p_object->type = p_type;
	switch (p_type)
	{
	case CAIRO_PDF_OBJECT_TYPE_ARRAY:
		_cairo_pdf_object_array_init(&p_object->array);
	case CAIRO_PDF_OBJECT_TYPE_DICTIONARY:
		_cairo_pdf_object_dictionary_init(&p_object->dictionary);
		break;
	default:
		assert(0);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// object finalization
//

void
_cairo_pdf_object_array_finish(cairo_pdf_array_t *p_array)
{
	free(p_array->elements);
}

void
_cairo_pdf_object_dictionary_finish(cairo_pdf_dictionary_t *p_dict)
{
	free(p_dict->keys);
	free(p_dict->elements);
}

void
_cairo_pdf_object_outline_entry_finish(cairo_pdf_outline_entry_t *p_entry)
{
	_cairo_pdf_object_string_finish(&p_entry->title);
}

void
_cairo_pdf_object_string_finish(cairo_pdf_string_t *p_string)
{
	if (p_string->_buffer != NULL)
		free(p_string->_buffer);
}

////////////////////////////////////////////////////////////////////////////////
//
// object set functions
//

cairo_private void
_cairo_pdf_object_dest_set(cairo_pdf_dest_t *p_dest, cairo_pdf_dest_type_t p_type, int p_page, double p_left, double p_top, double p_right, double p_bottom, double p_zoom)
{
	p_dest->type = p_type;
	p_dest->page = p_page;
	p_dest->left = p_left;
	p_dest->top = p_top;
	p_dest->right = p_right;
	p_dest->bottom = p_bottom;
	p_dest->zoom = p_zoom;
}

cairo_private void
_cairo_pdf_object_dest_set_xyz(cairo_pdf_dest_t *p_dest, int p_page, double p_left, double p_top, double p_zoom)
{
	_cairo_pdf_object_dest_set(p_dest, CAIRO_PDF_DEST_TYPE_XYZ, p_page, p_left, p_top, 0, 0, p_zoom);
}

////////////////////////////////////////////////////////////////////////////////
//
// string functions
//

cairo_status_t
_cairo_pdf_object_string_copy_text(cairo_pdf_string_t *p_dest, const char *p_text)
{
	int t_is_unicode = FALSE;
	const unsigned char *t_str_ptr;

	t_str_ptr = (const unsigned char*) p_text;

	while (*t_str_ptr && !t_is_unicode)
	{
		if (*t_str_ptr >= 128)
			t_is_unicode = TRUE;
		t_str_ptr++;
	}

	if (t_is_unicode)
	{
		uint16_t *t_utf16_data = NULL;
		int t_items, t_status;
		int i;

		t_status = _cairo_utf8_to_utf16(p_text, -1, &t_utf16_data, &t_items);
		if (t_status != CAIRO_STATUS_SUCCESS)
			return t_status;
		p_dest->_buffer = _cairo_malloc(2 + t_items * 2);
		if (p_dest->_buffer == NULL)
		{
			free(t_utf16_data);
			return CAIRO_STATUS_NO_MEMORY;
		}
		p_dest->length = 2 + t_items * 2;

		// _cario_utf8_to_utf16 outputs as LE with no bytemark,
		// pdf requires BE with bytemark
		p_dest->_buffer[0] = 0xFE;
		p_dest->_buffer[1] = 0xFF;
		for (i = 0; i < t_items; i++)
		{
			p_dest->_buffer[2 + i*2] = t_utf16_data[i] >> 8;
			p_dest->_buffer[2 + i*2 + 1] = t_utf16_data[i] & 0xFF;
		}
		//memcpy(p_dest->_buffer + 2, t_utf16_data, t_items * 2);
		p_dest->data = p_dest->_buffer;
	}
	else
	{
		p_dest->length = strlen(p_text);
		p_dest->_buffer = (char*)malloc(p_dest->length);
		if (p_dest->_buffer == NULL)
			return CAIRO_STATUS_NO_MEMORY;
		memcpy(p_dest->_buffer, p_text, p_dest->length);
		p_dest->data = p_dest->_buffer;
	}

	return CAIRO_STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//
// array functions
//

int
_cairo_pdf_object_array_reserve(cairo_pdf_array_t *p_dict, int32_t p_count)
{
	p_dict->elements = (cairo_pdf_object_t*) realloc(p_dict->elements, sizeof(cairo_pdf_object_t) * (p_dict->size + p_count));

	return (p_dict->elements != NULL);
}

cairo_status_t
_cairo_pdf_object_array_append(cairo_pdf_array_t *p_array, const cairo_pdf_object_t *p_value)
{
	if (!_cairo_pdf_object_array_reserve(p_array, 1))
		return CAIRO_STATUS_NO_MEMORY;
	p_array->elements[p_array->size] = *p_value;
	p_array->size += 1;
	return CAIRO_STATUS_SUCCESS;
}

void
_cairo_pdf_object_array_clear(cairo_pdf_array_t *p_array)
{
	free(p_array->elements);
	p_array->elements = NULL;
	p_array->size = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// dictionary functions
//

int
_cairo_pdf_object_dictionary_index_of(cairo_pdf_dictionary_t *p_dict, const char *p_key, int32_t *r_index)
{
	uint32_t i;

	for (i = 0; i < p_dict->size; i++)
		if (strcmp(p_key, p_dict->keys[i]) == 0)
		{
			*r_index = i;
			return TRUE;
		}
	return FALSE;
}

int
_cairo_pdf_object_dictionary_reserve(cairo_pdf_dictionary_t *p_dict, int32_t p_count)
{
	p_dict->keys = (const char **) realloc(p_dict->keys, sizeof(char *) * (p_dict->size + p_count));
	p_dict->elements = (cairo_pdf_object_t*) realloc(p_dict->elements, sizeof(cairo_pdf_object_t) * (p_dict->size + p_count));

	return (p_dict->keys != NULL && p_dict->elements != NULL);
}

cairo_status_t
_cairo_pdf_object_dictionary_set(cairo_pdf_dictionary_t *p_dict, const char *p_key, const cairo_pdf_object_t *p_value)
{
	int32_t t_index;

	if (_cairo_pdf_object_dictionary_index_of(p_dict, p_key, &t_index))
	{
		p_dict->elements[t_index] = *p_value;
		return CAIRO_STATUS_SUCCESS;
	}
	else
	{
		if (!_cairo_pdf_object_dictionary_reserve(p_dict, 1))
			return CAIRO_STATUS_NO_MEMORY;
		p_dict->keys[p_dict->size] = p_key;
		p_dict->elements[p_dict->size] = *p_value;
		p_dict->size += 1;
		return CAIRO_STATUS_SUCCESS;
	}
}

