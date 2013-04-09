/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2004 Red Hat, Inc
 * Copyright © 2006 Red Hat, Inc
 * Copyright © 2007 Adrian Johnson
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

#ifndef CAIRO_PDF_EXT_PRIVATE_H
#define CAIRO_PDF_EXT_PRIVATE_H

#include "cairo-compiler-private.h"
#include "cairo-types-private.h"

#include "cairo-pdf-ext-object.h"

cairo_private void
_cairo_pdf_object_init(cairo_pdf_object_t *p_object, cairo_pdf_object_type_t p_type);

cairo_private void
_cairo_pdf_object_array_init(cairo_pdf_array_t *p_object);

cairo_private void
_cairo_pdf_object_array_finish(cairo_pdf_array_t *p_object);

cairo_private void
_cairo_pdf_object_array_clear(cairo_pdf_array_t *p_object);

cairo_private cairo_status_t
_cairo_pdf_object_array_append(cairo_pdf_array_t *p_array, const cairo_pdf_object_t *p_value);

cairo_private void
_cairo_pdf_object_dictionary_init(cairo_pdf_dictionary_t *p_object);

cairo_private void
_cairo_pdf_object_dictionary_finish(cairo_pdf_dictionary_t *p_object);

cairo_private cairo_status_t
_cairo_pdf_object_dictionary_set(cairo_pdf_dictionary_t *p_object, const char *p_key, const cairo_pdf_object_t *p_value);

cairo_private void
_cairo_pdf_object_dest_set(cairo_pdf_dest_t *p_dest, cairo_pdf_dest_type_t p_type, int p_page, double p_left, double p_top, double p_right, double p_bottom, double p_zoom);

cairo_private void
_cairo_pdf_object_dest_set_xyz(cairo_pdf_dest_t *p_dest, int p_page, double p_left, double p_top, double p_zoom);

cairo_private void
_cairo_pdf_object_string_init(cairo_pdf_string_t *p_string);

cairo_private cairo_status_t
_cairo_pdf_object_string_copy_text(cairo_pdf_string_t *p_dest, const char *p_text);

cairo_private void
_cairo_pdf_object_string_finish(cairo_pdf_string_t *p_string);

cairo_private void
_cairo_pdf_object_outline_entry_init(cairo_pdf_outline_entry_t *p_entry);

cairo_private void
_cairo_pdf_object_outline_entry_finish(cairo_pdf_outline_entry_t *p_entry);


cairo_private void
_cairo_pdf_output_stream_write_object(cairo_output_stream_t *p_stream, const cairo_pdf_object_t *p_object);

cairo_private void
_cairo_pdf_output_stream_write_string(cairo_output_stream_t *p_stream, const cairo_pdf_string_t *p_string);

cairo_private void
_cairo_pdf_output_stream_write_name(cairo_output_stream_t *p_stream, const char *p_name);

cairo_private void
_cairo_pdf_output_stream_write_dictionary(cairo_output_stream_t *p_stream, const cairo_pdf_dictionary_t *p_dict);

cairo_private void
_cairo_pdf_output_stream_write_date(cairo_output_stream_t *p_stream, const cairo_pdf_datetime_t *p_date);

cairo_private void
_cairo_pdf_output_stream_write_dest(cairo_output_stream_t *p_stream, const cairo_pdf_dest_t *p_dest);

#endif /* CAIRO_PDF_EXT_PRIVATE_H */
