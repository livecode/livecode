/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
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
 *	Carl D. Worth <cworth@cworth.org>
 */

#include "cairoint.h"

#include "cairo-surface-fallback-private.h"
#include "cairo-clip-private.h"
#include "cairo-meta-surface-private.h"
#include "cairo-region-private.h"
#include "cairo-tee-surface-private.h"

#define DEFINE_NIL_SURFACE(status, name)			\
const cairo_surface_t name = {					\
    NULL,				/* backend */		\
    CAIRO_SURFACE_TYPE_IMAGE,		/* type */		\
    CAIRO_CONTENT_COLOR,		/* content */		\
    CAIRO_REFERENCE_COUNT_INVALID,	/* ref_count */		\
    status,				/* status */		\
    FALSE,				/* finished */		\
    0,					/* unique id */		\
    { 0, 0, 0, NULL, },			/* user_data */		\
    { 0, 0, 0, NULL, },			/* mime_data */         \
    { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 },   /* device_transform */	\
    { 1.0, 0.0,	0.0, 1.0, 0.0, 0.0 },	/* device_transform_inverse */	\
    0.0,				/* x_resolution */	\
    0.0,				/* y_resolution */	\
    0.0,				/* x_fallback_resolution */	\
    0.0,				/* y_fallback_resolution */	\
    NULL,				/* snapshot_of */	\
    NULL,				/* snapshot_detach */	\
    { 0,	/* size */					\
      0,	/* num_elements */				\
      0,	/* element_size */				\
      NULL,	/* elements */					\
    },					/* snapshots */		\
    FALSE,				/* has_font_options */	\
    { CAIRO_ANTIALIAS_DEFAULT,		/* antialias */		\
      CAIRO_SUBPIXEL_ORDER_DEFAULT,	/* subpixel_order */	\
      CAIRO_HINT_STYLE_DEFAULT,		/* hint_style */	\
      CAIRO_HINT_METRICS_DEFAULT	/* hint_metrics */	\
    }					/* font_options */	\
}

/* XXX error object! */

static DEFINE_NIL_SURFACE(CAIRO_STATUS_NO_MEMORY, _cairo_surface_nil);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_SURFACE_TYPE_MISMATCH, _cairo_surface_nil_surface_type_mismatch);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_STATUS, _cairo_surface_nil_invalid_status);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_CONTENT, _cairo_surface_nil_invalid_content);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_FORMAT, _cairo_surface_nil_invalid_format);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_VISUAL, _cairo_surface_nil_invalid_visual);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_FILE_NOT_FOUND, _cairo_surface_nil_file_not_found);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_TEMP_FILE_ERROR, _cairo_surface_nil_temp_file_error);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_READ_ERROR, _cairo_surface_nil_read_error);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_WRITE_ERROR, _cairo_surface_nil_write_error);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_STRIDE, _cairo_surface_nil_invalid_stride);
static DEFINE_NIL_SURFACE(CAIRO_STATUS_INVALID_SIZE, _cairo_surface_nil_invalid_size);

static void
_cairo_surface_copy_pattern_for_destination (const cairo_pattern_t **pattern,
					     cairo_surface_t *destination,
					     cairo_pattern_t *pattern_copy);

/**
 * _cairo_surface_set_error:
 * @surface: a surface
 * @status: a status value indicating an error
 *
 * Atomically sets surface->status to @status and calls _cairo_error;
 * Does nothing if status is %CAIRO_STATUS_SUCCESS or any of the internal
 * status values.
 *
 * All assignments of an error status to surface->status should happen
 * through _cairo_surface_set_error(). Note that due to the nature of
 * the atomic operation, it is not safe to call this function on the
 * nil objects.
 *
 * The purpose of this function is to allow the user to set a
 * breakpoint in _cairo_error() to generate a stack trace for when the
 * user causes cairo to detect an error.
 *
 * Return value: the error status.
 **/
cairo_status_t
_cairo_surface_set_error (cairo_surface_t *surface,
			  cairo_status_t status)
{
    if (status == CAIRO_STATUS_SUCCESS || status >= CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    /* Don't overwrite an existing error. This preserves the first
     * error, which is the most significant. */
    _cairo_status_set_error (&surface->status, status);

    return _cairo_error (status);
}

/**
 * cairo_surface_get_type:
 * @surface: a #cairo_surface_t
 *
 * This function returns the type of the backend used to create
 * a surface. See #cairo_surface_type_t for available types.
 *
 * Return value: The type of @surface.
 *
 * Since: 1.2
 **/
cairo_surface_type_t
cairo_surface_get_type (cairo_surface_t *surface)
{
    /* We don't use surface->backend->type here so that some of the
     * special "wrapper" surfaces such as cairo_paginated_surface_t
     * can override surface->type with the type of the "child"
     * surface. */
    return surface->type;
}
slim_hidden_def (cairo_surface_get_type);

/**
 * cairo_surface_get_content:
 * @surface: a #cairo_surface_t
 *
 * This function returns the content type of @surface which indicates
 * whether the surface contains color and/or alpha information. See
 * #cairo_content_t.
 *
 * Return value: The content type of @surface.
 *
 * Since: 1.2
 **/
cairo_content_t
cairo_surface_get_content (cairo_surface_t *surface)
{
    return surface->content;
}
slim_hidden_def(cairo_surface_get_content);

/**
 * cairo_surface_status:
 * @surface: a #cairo_surface_t
 *
 * Checks whether an error has previously occurred for this
 * surface.
 *
 * Return value: %CAIRO_STATUS_SUCCESS, %CAIRO_STATUS_NULL_POINTER,
 * %CAIRO_STATUS_NO_MEMORY, %CAIRO_STATUS_READ_ERROR,
 * %CAIRO_STATUS_INVALID_CONTENT, %CAIRO_STATUS_INVALID_FORMAT, or
 * %CAIRO_STATUS_INVALID_VISUAL.
 **/
cairo_status_t
cairo_surface_status (cairo_surface_t *surface)
{
    return surface->status;
}
slim_hidden_def (cairo_surface_status);

static unsigned int
_cairo_surface_allocate_unique_id (void)
{
    static cairo_atomic_int_t unique_id;

#if CAIRO_NO_MUTEX
    if (++unique_id == 0)
	unique_id = 1;
    return unique_id;
#else
    cairo_atomic_int_t old, id;

    do {
	old = _cairo_atomic_uint_get (&unique_id);
	id = old + 1;
	if (id == 0)
	    id = 1;
    } while (! _cairo_atomic_uint_cmpxchg (&unique_id, old, id));

    return id;
#endif
}

static cairo_bool_t
_cairo_surface_has_snapshots (cairo_surface_t *surface)
{
    return surface->snapshots.num_elements != 0;
}

static void
_cairo_surface_detach_snapshots (cairo_surface_t *surface)
{
    cairo_surface_t **snapshots;
    unsigned int i;

    if (! _cairo_surface_has_snapshots (surface))
	return;

    /* XXX do something intelligent! */

    snapshots = _cairo_array_index (&surface->snapshots, 0);
    for (i = 0; i < surface->snapshots.num_elements; i++) {
	snapshots[i]->snapshot_of = NULL;

	if (snapshots[i]->snapshot_detach != NULL)
	    snapshots[i]->snapshot_detach (snapshots[i]);

	cairo_surface_destroy (snapshots[i]);
    }
    surface->snapshots.num_elements = 0;

    assert (! _cairo_surface_has_snapshots (surface));
}

cairo_status_t
_cairo_surface_attach_snapshot (cairo_surface_t *surface,
				cairo_surface_t *snapshot,
				cairo_surface_func_t detach_func)
{
    cairo_status_t status;

    assert (surface != snapshot);

    if (snapshot->snapshot_of != NULL)
	_cairo_surface_detach_snapshot (snapshot);

    snapshot->snapshot_of = surface;
    snapshot->snapshot_detach = detach_func;

    status = _cairo_array_append (&surface->snapshots, &snapshot);
    if (unlikely (status))
	return status;

    cairo_surface_reference (snapshot);
    return CAIRO_STATUS_SUCCESS;
}

cairo_surface_t *
_cairo_surface_has_snapshot (cairo_surface_t *surface,
			     const cairo_surface_backend_t *backend,
			     cairo_content_t content)
{
    cairo_surface_t **snapshots;
    unsigned int i;

    snapshots = _cairo_array_index (&surface->snapshots, 0);
    for (i = 0; i < surface->snapshots.num_elements; i++) {
	if (snapshots[i]->backend == backend &&
	    snapshots[i]->content == content)
	{
	    return snapshots[i];
	}
    }

    return NULL;
}

void
_cairo_surface_detach_snapshot (cairo_surface_t *snapshot)
{
    cairo_surface_t *surface;
    cairo_surface_t **snapshots;
    unsigned int i;

    assert (snapshot->snapshot_of != NULL);
    surface = snapshot->snapshot_of;

    snapshots = _cairo_array_index (&surface->snapshots, 0);
    for (i = 0; i < surface->snapshots.num_elements; i++) {
	if (snapshots[i] == snapshot)
	    break;
    }
    assert (i < surface->snapshots.num_elements);

    surface->snapshots.num_elements--;
    memmove (&snapshots[i],
	     &snapshots[i+1],
	     sizeof (cairo_surface_t *)*(surface->snapshots.num_elements - i));

    snapshot->snapshot_of = NULL;

    if (snapshot->snapshot_detach != NULL)
	snapshot->snapshot_detach (snapshot);

    cairo_surface_destroy (snapshot);
}

static cairo_bool_t
_cairo_surface_is_writable (cairo_surface_t *surface)
{
    return ! surface->finished &&
	   surface->snapshot_of == NULL &&
	   ! _cairo_surface_has_snapshots (surface);
}

static void
_cairo_surface_begin_modification (cairo_surface_t *surface)
{
    assert (surface->status == CAIRO_STATUS_SUCCESS);
    assert (! surface->finished);
    assert (surface->snapshot_of == NULL);

    _cairo_surface_detach_snapshots (surface);
}

void
_cairo_surface_init (cairo_surface_t			*surface,
		     const cairo_surface_backend_t	*backend,
		     cairo_content_t			 content)
{
    CAIRO_MUTEX_INITIALIZE ();

    surface->backend = backend;
    surface->content = content;
    surface->type = backend->type;

    CAIRO_REFERENCE_COUNT_INIT (&surface->ref_count, 1);
    surface->status = CAIRO_STATUS_SUCCESS;
    surface->finished = FALSE;
    surface->unique_id = _cairo_surface_allocate_unique_id ();

    _cairo_user_data_array_init (&surface->user_data);
    _cairo_user_data_array_init (&surface->mime_data);

    cairo_matrix_init_identity (&surface->device_transform);
    cairo_matrix_init_identity (&surface->device_transform_inverse);

    surface->x_resolution = CAIRO_SURFACE_RESOLUTION_DEFAULT;
    surface->y_resolution = CAIRO_SURFACE_RESOLUTION_DEFAULT;

    surface->x_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;
    surface->y_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;

    _cairo_array_init (&surface->snapshots, sizeof (cairo_surface_t *));
    surface->snapshot_of = NULL;

    surface->has_font_options = FALSE;
}

static void
_cairo_surface_copy_similar_properties (cairo_surface_t *surface,
					cairo_surface_t *other)
{
    if (other->has_font_options || other->backend != surface->backend) {
	cairo_font_options_t options;

	cairo_surface_get_font_options (other, &options);
	_cairo_surface_set_font_options (surface, &options);
    }

    cairo_surface_set_fallback_resolution (surface,
					   other->x_fallback_resolution,
					   other->y_fallback_resolution);
}

cairo_surface_t *
_cairo_surface_create_similar_scratch (cairo_surface_t *other,
				       cairo_content_t	content,
				       int		width,
				       int		height)
{
    cairo_surface_t *surface;

    if (unlikely (other->status))
	return _cairo_surface_create_in_error (other->status);

    if (other->backend->create_similar == NULL)
	return NULL;

    surface = other->backend->create_similar (other,
					      content, width, height);
    if (surface == NULL || surface->status)
	return surface;

    _cairo_surface_copy_similar_properties (surface, other);

    return surface;
}

/**
 * cairo_surface_create_similar:
 * @other: an existing surface used to select the backend of the new surface
 * @content: the content for the new surface
 * @width: width of the new surface, (in device-space units)
 * @height: height of the new surface (in device-space units)
 *
 * Create a new surface that is as compatible as possible with an
 * existing surface. For example the new surface will have the same
 * fallback resolution and font options as @other. Generally, the new
 * surface will also use the same backend as @other, unless that is
 * not possible for some reason. The type of the returned surface may
 * be examined with cairo_surface_get_type().
 *
 * Initially the surface contents are all 0 (transparent if contents
 * have transparency, black otherwise.)
 *
 * Return value: a pointer to the newly allocated surface. The caller
 * owns the surface and should call cairo_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if @other is already in an error state
 * or any other error occurs.
 **/
cairo_surface_t *
cairo_surface_create_similar (cairo_surface_t  *other,
			      cairo_content_t	content,
			      int		width,
			      int		height)
{
    if (unlikely (other->status))
	return _cairo_surface_create_in_error (other->status);

    if (unlikely (! CAIRO_CONTENT_VALID (content)))
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_INVALID_CONTENT));

    return _cairo_surface_create_similar_solid (other,
						content, width, height,
						CAIRO_COLOR_TRANSPARENT,
						TRUE);
}

cairo_surface_t *
_cairo_surface_create_similar_solid (cairo_surface_t	 *other,
				     cairo_content_t	  content,
				     int		  width,
				     int		  height,
				     const cairo_color_t *color,
				     cairo_bool_t allow_fallback)
{
    cairo_status_t status;
    cairo_surface_t *surface;
    cairo_solid_pattern_t pattern;

    surface = _cairo_surface_create_similar_scratch (other, content,
						     width, height);
    if (surface == NULL && allow_fallback)
	surface = _cairo_image_surface_create_with_content (content,
							    width, height);
    if (surface == NULL || surface->status)
	return surface;

    _cairo_pattern_init_solid (&pattern, color, content);
    status = _cairo_surface_paint (surface,
				   color == CAIRO_COLOR_TRANSPARENT ?
				   CAIRO_OPERATOR_CLEAR : CAIRO_OPERATOR_SOURCE,
				   &pattern.base, NULL);
    if (unlikely (status)) {
	cairo_surface_destroy (surface);
	surface = _cairo_surface_create_in_error (status);
    }

    return surface;
}

cairo_surface_t *
_cairo_surface_create_solid_pattern_surface (cairo_surface_t	   *other,
					     const cairo_solid_pattern_t *solid_pattern)
{
    if (other->backend->create_solid_pattern_surface != NULL) {
	cairo_surface_t *surface;

	surface = other->backend->create_solid_pattern_surface (other,
								solid_pattern);
	if (surface)
	    return surface;
    }

    return _cairo_surface_create_similar_solid (other,
						solid_pattern->content,
						1, 1,
						&solid_pattern->color,
						FALSE);
}

cairo_int_status_t
_cairo_surface_repaint_solid_pattern_surface (cairo_surface_t	    *other,
					      cairo_surface_t       *solid_surface,
					      const cairo_solid_pattern_t *solid_pattern)
{
    /* Solid pattern surface for these backends are special and not trivial
     * to repaint.  Skip repainting.
     *
     * This does not work optimally with things like analysis surface that
     * are proxies.  But returning UNSUPPORTED is *safe* as it only
     * disables some caching.
     */
    if (other->backend->create_solid_pattern_surface != NULL &&
	! other->backend->can_repaint_solid_pattern_surface (solid_surface,
							     solid_pattern))
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    return _cairo_surface_paint (solid_surface,
				 CAIRO_OPERATOR_SOURCE,
				 &solid_pattern->base,
				 NULL);
}

/**
 * cairo_surface_reference:
 * @surface: a #cairo_surface_t
 *
 * Increases the reference count on @surface by one. This prevents
 * @surface from being destroyed until a matching call to
 * cairo_surface_destroy() is made.
 *
 * The number of references to a #cairo_surface_t can be get using
 * cairo_surface_get_reference_count().
 *
 * Return value: the referenced #cairo_surface_t.
 **/
cairo_surface_t *
cairo_surface_reference (cairo_surface_t *surface)
{
    if (surface == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return surface;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count));

    _cairo_reference_count_inc (&surface->ref_count);

    return surface;
}
slim_hidden_def (cairo_surface_reference);

/**
 * cairo_surface_destroy:
 * @surface: a #cairo_surface_t
 *
 * Decreases the reference count on @surface by one. If the result is
 * zero, then @surface and all associated resources are freed.  See
 * cairo_surface_reference().
 **/
void
cairo_surface_destroy (cairo_surface_t *surface)
{
    if (surface == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count));

    if (! _cairo_reference_count_dec_and_test (&surface->ref_count))
	return;

    assert (surface->snapshot_of == NULL);

    if (! surface->finished)
	cairo_surface_finish (surface);

    _cairo_user_data_array_fini (&surface->user_data);
    _cairo_user_data_array_fini (&surface->mime_data);
    _cairo_array_fini (&surface->snapshots);

    free (surface);
}
slim_hidden_def(cairo_surface_destroy);

/**
 * cairo_surface_get_reference_count:
 * @surface: a #cairo_surface_t
 *
 * Returns the current reference count of @surface.
 *
 * Return value: the current reference count of @surface.  If the
 * object is a nil object, 0 will be returned.
 *
 * Since: 1.4
 **/
unsigned int
cairo_surface_get_reference_count (cairo_surface_t *surface)
{
    if (surface == NULL ||
	    CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return 0;

    return CAIRO_REFERENCE_COUNT_GET_VALUE (&surface->ref_count);
}

/**
 * cairo_surface_finish:
 * @surface: the #cairo_surface_t to finish
 *
 * This function finishes the surface and drops all references to
 * external resources.  For example, for the Xlib backend it means
 * that cairo will no longer access the drawable, which can be freed.
 * After calling cairo_surface_finish() the only valid operations on a
 * surface are getting and setting user, referencing and
 * destroying, and flushing and finishing it.
 * Further drawing to the surface will not affect the
 * surface but will instead trigger a %CAIRO_STATUS_SURFACE_FINISHED
 * error.
 *
 * When the last call to cairo_surface_destroy() decreases the
 * reference count to zero, cairo will call cairo_surface_finish() if
 * it hasn't been called already, before freeing the resources
 * associated with the surface.
 **/
void
cairo_surface_finish (cairo_surface_t *surface)
{
    cairo_status_t status;

    if (surface == NULL)
	return;

    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return;

    if (surface->finished)
	return;

    cairo_surface_flush (surface);

    /* call finish even if in error mode */
    if (surface->backend->finish) {
	status = surface->backend->finish (surface);
	if (unlikely (status))
	    status = _cairo_surface_set_error (surface, status);
    }

    surface->finished = TRUE;

    if (surface->snapshot_of != NULL)
	_cairo_surface_detach_snapshot (surface);
}
slim_hidden_def (cairo_surface_finish);

/**
 * cairo_surface_get_user_data:
 * @surface: a #cairo_surface_t
 * @key: the address of the #cairo_user_data_key_t the user data was
 * attached to
 *
 * Return user data previously attached to @surface using the specified
 * key.  If no user data has been attached with the given key this
 * function returns %NULL.
 *
 * Return value: the user data previously attached or %NULL.
 **/
void *
cairo_surface_get_user_data (cairo_surface_t		 *surface,
			     const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&surface->user_data,
					    key);
}

/**
 * cairo_surface_set_user_data:
 * @surface: a #cairo_surface_t
 * @key: the address of a #cairo_user_data_key_t to attach the user data to
 * @user_data: the user data to attach to the surface
 * @destroy: a #cairo_destroy_func_t which will be called when the
 * surface is destroyed or when new user data is attached using the
 * same key.
 *
 * Attach user data to @surface.  To remove user data from a surface,
 * call this function with the key that was used to set it and %NULL
 * for @data.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or %CAIRO_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 **/
cairo_status_t
cairo_surface_set_user_data (cairo_surface_t		 *surface,
			     const cairo_user_data_key_t *key,
			     void			 *user_data,
			     cairo_destroy_func_t	 destroy)
{
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return surface->status;

    return _cairo_user_data_array_set_data (&surface->user_data,
					    key, user_data, destroy);
}

/**
 * cairo_surface_get_mime_data:
 * @surface: a #cairo_surface_t
 * @mime_type: the mime type of the image data
 * @data: the image data to attached to the surface
 * @length: the length of the image data
 *
 * Return mime data previously attached to @surface using the
 * specified mime type.  If no data has been attached with the given
 * mime type, @data is set %NULL.
 *
 * Since: 1.10
 **/
void
cairo_surface_get_mime_data (cairo_surface_t		*surface,
                             const char			*mime_type,
                             const unsigned char       **data,
                             unsigned int		*length)
{
    cairo_user_data_slot_t *slots;
    int i, num_slots;

    *data = NULL;
    *length = 0;
    if (unlikely (surface->status))
	return;

    /* The number of mime-types attached to a surface is usually small,
     * typically zero. Therefore it is quicker to do a strcmp() against
     * each key than it is to intern the string (i.e. compute a hash,
     * search the hash table, and do a final strcmp).
     */
    num_slots = surface->mime_data.num_elements;
    slots = _cairo_array_index (&surface->mime_data, 0);
    for (i = 0; i < num_slots; i++) {
	if (strcmp ((char *) slots[i].key, mime_type) == 0) {
	    cairo_mime_data_t *mime_data = slots[i].user_data;

	    *data = mime_data->data;
	    *length = mime_data->length;
	    return;
	}
    }
}
slim_hidden_def (cairo_surface_get_mime_data);

static void
_cairo_mime_data_destroy (void *ptr)
{
    cairo_mime_data_t *mime_data = ptr;

    if (! _cairo_reference_count_dec_and_test (&mime_data->ref_count))
	return;

    if (mime_data->destroy && mime_data->closure)
	mime_data->destroy (mime_data->closure);

    free (mime_data);
}

/**
 * cairo_surface_set_mime_data:
 * @surface: a #cairo_surface_t
 * @mime_type: the mime type of the image data
 * @data: the image data to attach to the surface
 * @length: the length of the image data
 * @destroy: a #cairo_destroy_func_t which will be called when the
 * surface is destroyed or when new image data is attached using the
 * same mime type.
 * @closure: the data to be passed to the @destroy notifier
 *
 * Attach an image in the format @mime_type to @surface. To remove
 * the data from a surface, call this function with same mime type
 * and %NULL for @data.
 *
 * Since: 1.10
 *
 * Return value: %CAIRO_STATUS_SUCCESS or %CAIRO_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 **/
cairo_status_t
cairo_surface_set_mime_data (cairo_surface_t		*surface,
                             const char			*mime_type,
                             const unsigned char	*data,
                             unsigned int		 length,
			     cairo_destroy_func_t	 destroy,
			     void			*closure)
{
    cairo_status_t status;
    cairo_mime_data_t *mime_data;

    if (unlikely (surface->status))
	return surface->status;

    status = _cairo_intern_string (&mime_type, -1);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    if (data != NULL) {
	mime_data = malloc (sizeof (cairo_mime_data_t));
	if (unlikely (mime_data == NULL))
	    return _cairo_surface_set_error (surface, _cairo_error (CAIRO_STATUS_NO_MEMORY));

	CAIRO_REFERENCE_COUNT_INIT (&mime_data->ref_count, 1);

	mime_data->data = (unsigned char *) data;
	mime_data->length = length;
	mime_data->destroy = destroy;
	mime_data->closure = closure;
    } else
	mime_data = NULL;

    status = _cairo_user_data_array_set_data (&surface->mime_data,
					      (cairo_user_data_key_t *) mime_type,
					      mime_data,
					      _cairo_mime_data_destroy);
    if (unlikely (status)) {
	if (mime_data != NULL)
	    free (mime_data);

	return _cairo_surface_set_error (surface, status);
    }

    return CAIRO_STATUS_SUCCESS;
}
slim_hidden_def (cairo_surface_set_mime_data);

static void
_cairo_mime_data_reference (const void *key, void *elt, void *closure)
{
    cairo_mime_data_t *mime_data = elt;

    _cairo_reference_count_inc (&mime_data->ref_count);
}

cairo_status_t
_cairo_surface_copy_mime_data (cairo_surface_t *dst,
			       cairo_surface_t *src)
{
    cairo_status_t status;

    if (dst->status)
	return dst->status;

    if (src->status)
	return _cairo_surface_set_error (dst, src->status);

    /* first copy the mime-data, discarding any already set on dst */
    status = _cairo_user_data_array_copy (&dst->mime_data, &src->mime_data);
    if (unlikely (status))
	return _cairo_surface_set_error (dst, status);

    /* now increment the reference counters for the copies */
    _cairo_user_data_array_foreach (&dst->mime_data,
				    _cairo_mime_data_reference,
				    NULL);

    return CAIRO_STATUS_SUCCESS;
}

/**
 * _cairo_surface_set_font_options:
 * @surface: a #cairo_surface_t
 * @options: a #cairo_font_options_t object that contains the
 *   options to use for this surface instead of backend's default
 *   font options.
 *
 * Sets the default font rendering options for the surface.
 * This is useful to correctly propagate default font options when
 * falling back to an image surface in a backend implementation.
 * This affects the options returned in cairo_surface_get_font_options().
 *
 * If @options is %NULL the surface options are reset to those of
 * the backend default.
 **/
void
_cairo_surface_set_font_options (cairo_surface_t       *surface,
				 cairo_font_options_t  *options)
{
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface,
		                           CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    if (options) {
	surface->has_font_options = TRUE;
	_cairo_font_options_init_copy (&surface->font_options, options);
    } else {
	surface->has_font_options = FALSE;
    }
}

/**
 * cairo_surface_get_font_options:
 * @surface: a #cairo_surface_t
 * @options: a #cairo_font_options_t object into which to store
 *   the retrieved options. All existing values are overwritten
 *
 * Retrieves the default font rendering options for the surface.
 * This allows display surfaces to report the correct subpixel order
 * for rendering on them, print surfaces to disable hinting of
 * metrics and so forth. The result can then be used with
 * cairo_scaled_font_create().
 **/
void
cairo_surface_get_font_options (cairo_surface_t       *surface,
				cairo_font_options_t  *options)
{
    if (cairo_font_options_status (options))
	return;

    if (surface->status) {
	_cairo_font_options_init_default (options);
	return;
    }

    if (! surface->has_font_options) {
	surface->has_font_options = TRUE;

	_cairo_font_options_init_default (&surface->font_options);

	if (!surface->finished && surface->backend->get_font_options) {
	    surface->backend->get_font_options (surface, &surface->font_options);
	}
    }

    _cairo_font_options_init_copy (options, &surface->font_options);
}
slim_hidden_def (cairo_surface_get_font_options);

/**
 * cairo_surface_flush:
 * @surface: a #cairo_surface_t
 *
 * Do any pending drawing for the surface and also restore any
 * temporary modification's cairo has made to the surface's
 * state. This function must be called before switching from
 * drawing on the surface with cairo to drawing on it directly
 * with native APIs. If the surface doesn't support direct access,
 * then this function does nothing.
 **/
void
cairo_surface_flush (cairo_surface_t *surface)
{
    cairo_status_t status;

    if (surface->status)
	return;

    if (surface->finished)
	return;

    /* update the current snapshots *before* the user updates the surface */
    _cairo_surface_detach_snapshots (surface);

    if (surface->backend->flush) {
	status = surface->backend->flush (surface);
	if (unlikely (status))
	    status = _cairo_surface_set_error (surface, status);
    }
}
slim_hidden_def (cairo_surface_flush);

/**
 * cairo_surface_mark_dirty:
 * @surface: a #cairo_surface_t
 *
 * Tells cairo that drawing has been done to surface using means other
 * than cairo, and that cairo should reread any cached areas. Note
 * that you must call cairo_surface_flush() before doing such drawing.
 */
void
cairo_surface_mark_dirty (cairo_surface_t *surface)
{
    cairo_surface_mark_dirty_rectangle (surface, 0, 0, -1, -1);
}

/**
 * cairo_surface_mark_dirty_rectangle:
 * @surface: a #cairo_surface_t
 * @x: X coordinate of dirty rectangle
 * @y: Y coordinate of dirty rectangle
 * @width: width of dirty rectangle
 * @height: height of dirty rectangle
 *
 * Like cairo_surface_mark_dirty(), but drawing has been done only to
 * the specified rectangle, so that cairo can retain cached contents
 * for other parts of the surface.
 *
 * Any cached clip set on the surface will be reset by this function,
 * to make sure that future cairo calls have the clip set that they
 * expect.
 */
void
cairo_surface_mark_dirty_rectangle (cairo_surface_t *surface,
				    int              x,
				    int              y,
				    int              width,
				    int              height)
{
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    /* The application *should* have called cairo_surface_flush() before
     * modifying the surface independently of cairo (and thus having to
     * call mark_dirty()). */
    assert (! _cairo_surface_has_snapshots (surface));

    if (surface->backend->mark_dirty_rectangle != NULL) {
	/* XXX: FRAGILE: We're ignoring the scaling component of
	 * device_transform here. I don't know what the right thing to
	 * do would actually be if there were some scaling here, but
	 * we avoid this since device_transfom scaling is not exported
	 * publicly and mark_dirty is not used internally. */
	status = surface->backend->mark_dirty_rectangle (surface,
                                                         x + surface->device_transform.x0,
                                                         y + surface->device_transform.y0,
							 width, height);

	if (unlikely (status))
	    status = _cairo_surface_set_error (surface, status);
    }
}
slim_hidden_def (cairo_surface_mark_dirty_rectangle);

/**
 * _cairo_surface_set_device_scale:
 * @surface: a #cairo_surface_t
 * @sx: a scale factor in the X direction
 * @sy: a scale factor in the Y direction
 *
 * Private function for setting an extra scale factor to affect all
 * drawing to a surface. This is used, for example, when replaying a
 * meta surface to an image fallback intended for an eventual
 * vector-oriented backend. Since the meta surface will record
 * coordinates in one backend space, but the image fallback uses a
 * different backend space, (differing by the fallback resolution
 * scale factors), we need a scale factor correction.
 *
 * Caution: Not all places we use device transform correctly handle
 * both a translate and a scale.  An audit would be nice.
 **/
void
_cairo_surface_set_device_scale (cairo_surface_t *surface,
				 double		  sx,
				 double		  sy)
{
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    _cairo_surface_begin_modification (surface);

    surface->device_transform.xx = sx;
    surface->device_transform.yy = sy;
    surface->device_transform.xy = 0.0;
    surface->device_transform.yx = 0.0;

    surface->device_transform_inverse = surface->device_transform;
    status = cairo_matrix_invert (&surface->device_transform_inverse);
    /* should always be invertible unless given pathological input */
    assert (status == CAIRO_STATUS_SUCCESS);
}

/**
 * cairo_surface_set_device_offset:
 * @surface: a #cairo_surface_t
 * @x_offset: the offset in the X direction, in device units
 * @y_offset: the offset in the Y direction, in device units
 *
 * Sets an offset that is added to the device coordinates determined
 * by the CTM when drawing to @surface. One use case for this function
 * is when we want to create a #cairo_surface_t that redirects drawing
 * for a portion of an onscreen surface to an offscreen surface in a
 * way that is completely invisible to the user of the cairo
 * API. Setting a transformation via cairo_translate() isn't
 * sufficient to do this, since functions like
 * cairo_device_to_user() will expose the hidden offset.
 *
 * Note that the offset affects drawing to the surface as well as
 * using the surface in a source pattern.
 **/
void
cairo_surface_set_device_offset (cairo_surface_t *surface,
				 double           x_offset,
				 double           y_offset)
{
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    _cairo_surface_begin_modification (surface);

    surface->device_transform.x0 = x_offset;
    surface->device_transform.y0 = y_offset;

    surface->device_transform_inverse = surface->device_transform;
    status = cairo_matrix_invert (&surface->device_transform_inverse);
    /* should always be invertible unless given pathological input */
    assert (status == CAIRO_STATUS_SUCCESS);
}
slim_hidden_def (cairo_surface_set_device_offset);

/**
 * cairo_surface_get_device_offset:
 * @surface: a #cairo_surface_t
 * @x_offset: the offset in the X direction, in device units
 * @y_offset: the offset in the Y direction, in device units
 *
 * This function returns the previous device offset set by
 * cairo_surface_set_device_offset().
 *
 * Since: 1.2
 **/
void
cairo_surface_get_device_offset (cairo_surface_t *surface,
				 double          *x_offset,
				 double          *y_offset)
{
    if (x_offset)
	*x_offset = surface->device_transform.x0;
    if (y_offset)
	*y_offset = surface->device_transform.y0;
}
slim_hidden_def (cairo_surface_get_device_offset);

/**
 * cairo_surface_set_fallback_resolution:
 * @surface: a #cairo_surface_t
 * @x_pixels_per_inch: horizontal setting for pixels per inch
 * @y_pixels_per_inch: vertical setting for pixels per inch
 *
 * Set the horizontal and vertical resolution for image fallbacks.
 *
 * When certain operations aren't supported natively by a backend,
 * cairo will fallback by rendering operations to an image and then
 * overlaying that image onto the output. For backends that are
 * natively vector-oriented, this function can be used to set the
 * resolution used for these image fallbacks, (larger values will
 * result in more detailed images, but also larger file sizes).
 *
 * Some examples of natively vector-oriented backends are the ps, pdf,
 * and svg backends.
 *
 * For backends that are natively raster-oriented, image fallbacks are
 * still possible, but they are always performed at the native
 * device resolution. So this function has no effect on those
 * backends.
 *
 * Note: The fallback resolution only takes effect at the time of
 * completing a page (with cairo_show_page() or cairo_copy_page()) so
 * there is currently no way to have more than one fallback resolution
 * in effect on a single page.
 *
 * The default fallback resoultion is 300 pixels per inch in both
 * dimensions.
 *
 * Since: 1.2
 **/
void
cairo_surface_set_fallback_resolution (cairo_surface_t	*surface,
				       double		 x_pixels_per_inch,
				       double		 y_pixels_per_inch)
{
    cairo_status_t status;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    if (x_pixels_per_inch <= 0 || y_pixels_per_inch <= 0) {
	/* XXX Could delay raising the error until we fallback, but throwing
	 * the error here means that we can catch the real culprit.
	 */
	status = _cairo_surface_set_error (surface, CAIRO_STATUS_INVALID_MATRIX);
	return;
    }

    _cairo_surface_begin_modification (surface);

    surface->x_fallback_resolution = x_pixels_per_inch;
    surface->y_fallback_resolution = y_pixels_per_inch;
}
slim_hidden_def (cairo_surface_set_fallback_resolution);

/**
 * cairo_surface_get_fallback_resolution:
 * @surface: a #cairo_surface_t
 * @x_pixels_per_inch: horizontal pixels per inch
 * @y_pixels_per_inch: vertical pixels per inch
 *
 * This function returns the previous fallback resolution set by
 * cairo_surface_set_fallback_resolution(), or default fallback
 * resolution if never set.
 *
 * Since: 1.8
 **/
void
cairo_surface_get_fallback_resolution (cairo_surface_t	*surface,
				       double		*x_pixels_per_inch,
				       double		*y_pixels_per_inch)
{
    if (x_pixels_per_inch)
	*x_pixels_per_inch = surface->x_fallback_resolution;
    if (y_pixels_per_inch)
	*y_pixels_per_inch = surface->y_fallback_resolution;
}

cairo_bool_t
_cairo_surface_has_device_transform (cairo_surface_t *surface)
{
    return ! _cairo_matrix_is_identity (&surface->device_transform);
}

/**
 * _cairo_surface_acquire_source_image:
 * @surface: a #cairo_surface_t
 * @image_out: location to store a pointer to an image surface that
 *    has identical contents to @surface. This surface could be @surface
 *    itself, a surface held internal to @surface, or it could be a new
 *    surface with a copy of the relevant portion of @surface.
 * @image_extra: location to store image specific backend data
 *
 * Gets an image surface to use when drawing as a fallback when drawing with
 * @surface as a source. _cairo_surface_release_source_image() must be called
 * when finished.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if an image was stored in @image_out.
 * %CAIRO_INT_STATUS_UNSUPPORTED if an image cannot be retrieved for the specified
 * surface. Or %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_status_t
_cairo_surface_acquire_source_image (cairo_surface_t         *surface,
				     cairo_image_surface_t  **image_out,
				     void                   **image_extra)
{
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    assert (!surface->finished);

    if (surface->backend->acquire_source_image == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = surface->backend->acquire_source_image (surface,
						     image_out, image_extra);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    _cairo_debug_check_image_surface_is_defined (&(*image_out)->base);

    return CAIRO_STATUS_SUCCESS;
}

/**
 * _cairo_surface_release_source_image:
 * @surface: a #cairo_surface_t
 * @image_extra: same as return from the matching _cairo_surface_acquire_source_image()
 *
 * Releases any resources obtained with _cairo_surface_acquire_source_image()
 **/
void
_cairo_surface_release_source_image (cairo_surface_t        *surface,
				     cairo_image_surface_t  *image,
				     void                   *image_extra)
{
    assert (!surface->finished);

    if (surface->backend->release_source_image)
	surface->backend->release_source_image (surface, image, image_extra);
}

/**
 * _cairo_surface_acquire_dest_image:
 * @surface: a #cairo_surface_t
 * @interest_rect: area of @surface for which fallback drawing is being done.
 *    A value of %NULL indicates that the entire surface is desired.
 *    XXXX I'd like to get rid of being able to pass %NULL here (nothing seems to)
 * @image_out: location to store a pointer to an image surface that includes at least
 *    the intersection of @interest_rect with the visible area of @surface.
 *    This surface could be @surface itself, a surface held internal to @surface,
 *    or it could be a new surface with a copy of the relevant portion of @surface.
 *    If a new surface is created, it should have the same channels and depth
 *    as @surface so that copying to and from it is exact.
 * @image_rect: location to store area of the original surface occupied
 *    by the surface stored in @image.
 * @image_extra: location to store image specific backend data
 *
 * Retrieves a local image for a surface for implementing a fallback drawing
 * operation. After calling this function, the implementation of the fallback
 * drawing operation draws the primitive to the surface stored in @image_out
 * then calls _cairo_surface_release_dest_image(),
 * which, if a temporary surface was created, copies the bits back to the
 * main surface and frees the temporary surface.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or %CAIRO_STATUS_NO_MEMORY.
 *  %CAIRO_INT_STATUS_UNSUPPORTED can be returned but this will mean that
 *  the backend can't draw with fallbacks. It's possible for the routine
 *  to store %NULL in @local_out and return %CAIRO_STATUS_SUCCESS;
 *  that indicates that no part of @interest_rect is visible, so no drawing
 *  is necessary. _cairo_surface_release_dest_image() should not be called in that
 *  case.
 **/
cairo_status_t
_cairo_surface_acquire_dest_image (cairo_surface_t         *surface,
				   cairo_rectangle_int_t   *interest_rect,
				   cairo_image_surface_t  **image_out,
				   cairo_rectangle_int_t   *image_rect,
				   void                   **image_extra)
{
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    assert (_cairo_surface_is_writable (surface));

    if (surface->backend->acquire_dest_image == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = surface->backend->acquire_dest_image (surface,
						   interest_rect,
						   image_out,
						   image_rect,
						   image_extra);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    _cairo_debug_check_image_surface_is_defined (&(*image_out)->base);

    return CAIRO_STATUS_SUCCESS;
}

/**
 * _cairo_surface_release_dest_image:
 * @surface: a #cairo_surface_t
 * @interest_rect: same as passed to the matching _cairo_surface_acquire_dest_image()
 * @image: same as returned from the matching _cairo_surface_acquire_dest_image()
 * @image_rect: same as returned from the matching _cairo_surface_acquire_dest_image()
 * @image_extra: same as return from the matching _cairo_surface_acquire_dest_image()
 *
 * Finishes the operation started with _cairo_surface_acquire_dest_image(), by, if
 * necessary, copying the image from @image back to @surface and freeing any
 * resources that were allocated.
 **/
void
_cairo_surface_release_dest_image (cairo_surface_t         *surface,
				   cairo_rectangle_int_t   *interest_rect,
				   cairo_image_surface_t   *image,
				   cairo_rectangle_int_t   *image_rect,
				   void                    *image_extra)
{
    assert (_cairo_surface_is_writable (surface));

    if (surface->backend->release_dest_image)
	surface->backend->release_dest_image (surface, interest_rect,
					      image, image_rect, image_extra);
}

static cairo_status_t
_cairo_meta_surface_clone_similar (cairo_surface_t  *surface,
			           cairo_surface_t  *src,
				   cairo_content_t   content,
				   int               src_x,
				   int               src_y,
				   int               width,
				   int               height,
				   int              *clone_offset_x,
				   int              *clone_offset_y,
				   cairo_surface_t **clone_out)
{
    cairo_meta_surface_t *meta = (cairo_meta_surface_t *) src;
    cairo_surface_t *similar;
    cairo_status_t status;

    similar = _cairo_surface_has_snapshot (src,
					   surface->backend,
					   src->content & content);
    if (similar != NULL) {
	*clone_out = cairo_surface_reference (similar);
	*clone_offset_x = 0;
	*clone_offset_y = 0;
	return CAIRO_STATUS_SUCCESS;
    }

    if (meta->unbounded ||
	width*height*8 < meta->extents.width*meta->extents.height)
    {
	/* XXX use _solid to perform an initial CLEAR? */
	similar = _cairo_surface_create_similar_scratch (surface,
							 src->content & content,
							 width, height);
	if (similar == NULL)
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	if (unlikely (similar->status))
	    return similar->status;

	cairo_surface_set_device_offset (similar, -src_x, -src_y);

	status = _cairo_meta_surface_replay (src, similar);
	if (unlikely (status)) {
	    cairo_surface_destroy (similar);
	    return status;
	}
    } else {
	similar = _cairo_surface_create_similar_scratch (surface,
							 src->content & content,
							 meta->extents.width,
							 meta->extents.height);
	if (similar == NULL)
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	if (unlikely (similar->status))
	    return similar->status;

	status = _cairo_meta_surface_replay (src, similar);
	if (unlikely (status)) {
	    cairo_surface_destroy (similar);
	    return status;
	}

	status = _cairo_surface_attach_snapshot (src, similar, NULL);
	if (unlikely (status)) {
	    cairo_surface_destroy (similar);
	    return status;
	}

	src_x = src_y = 0;
    }

    *clone_out = similar;
    *clone_offset_x = src_x;
    *clone_offset_y = src_y;
    return CAIRO_STATUS_SUCCESS;
}

/**
 * _cairo_surface_clone_similar:
 * @surface: a #cairo_surface_t
 * @src: the source image
 * @content: target content mask
 * @src_x: extent for the rectangle in src we actually care about
 * @src_y: extent for the rectangle in src we actually care about
 * @width: extent for the rectangle in src we actually care about
 * @height: extent for the rectangle in src we actually care about
 * @clone_out: location to store a surface compatible with @surface
 *   and with contents identical to @src. The caller must call
 *   cairo_surface_destroy() on the result.
 *
 * Creates a surface with contents identical to @src but that
 *   can be used efficiently with @surface. If @surface and @src are
 *   already compatible then it may return a new reference to @src.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if a surface was created and stored
 *   in @clone_out. Otherwise %CAIRO_INT_STATUS_UNSUPPORTED or another
 *   error like %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_status_t
_cairo_surface_clone_similar (cairo_surface_t  *surface,
			      cairo_surface_t  *src,
			      cairo_content_t	content,
			      int               src_x,
			      int               src_y,
			      int               width,
			      int               height,
			      int              *clone_offset_x,
			      int              *clone_offset_y,
			      cairo_surface_t **clone_out)
{
    cairo_status_t status = CAIRO_INT_STATUS_UNSUPPORTED;
    cairo_image_surface_t *image;
    void *image_extra;

    if (unlikely (surface->status))
	return surface->status;

    if (unlikely (surface->finished))
	return _cairo_error (CAIRO_STATUS_SURFACE_FINISHED);

    if (src->type == CAIRO_SURFACE_TYPE_TEE) {
	cairo_surface_t *match;

	match = _cairo_tee_surface_find_match (src,
					       surface->backend,
					       content);
	if (match != NULL)
	    src = match;
    }

    if (surface->backend->clone_similar != NULL) {
	status = surface->backend->clone_similar (surface, src,
						  content,
						  src_x, src_y,
						  width, height,
						  clone_offset_x,
						  clone_offset_y,
						  clone_out);
	if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	    if (_cairo_surface_is_image (src))
		return CAIRO_INT_STATUS_UNSUPPORTED;

	    /* First check to see if we can replay to a similar surface */
	    if (_cairo_surface_is_meta (src)) {
		return _cairo_meta_surface_clone_similar (surface, src,
							  content,
							  src_x, src_y,
							  width, height,
							  clone_offset_x,
							  clone_offset_y,
							  clone_out);
	    }

	    /* If we failed, try again with an image surface */
	    status = _cairo_surface_acquire_source_image (src, &image, &image_extra);
	    if (status == CAIRO_STATUS_SUCCESS) {
		status =
		    surface->backend->clone_similar (surface, &image->base,
						     content,
						     src_x, src_y,
						     width, height,
						     clone_offset_x,
						     clone_offset_y,
						     clone_out);

		_cairo_surface_release_source_image (src, image, image_extra);
	    }
	}
    }

    /* If we're still unsupported, hit our fallback path to get a clone */
    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	status =
	    _cairo_surface_fallback_clone_similar (surface, src,
						   content,
						   src_x, src_y,
						   width, height,
						   clone_offset_x,
						   clone_offset_y,
						   clone_out);
    }

    if (unlikely (status))
	return status;

    /* Update the clone's device_transform (which the underlying surface
     * backend knows nothing about) */
    if (*clone_out != src) {
	(*clone_out)->device_transform = src->device_transform;
	(*clone_out)->device_transform_inverse = src->device_transform_inverse;
    }

    return status;
}

/* XXX: Shouldn't really need to do this here. */
#include "cairo-meta-surface-private.h"

/**
 * _cairo_surface_snapshot
 * @surface: a #cairo_surface_t
 *
 * Make an immutable copy of @surface. It is an error to call a
 * surface-modifying function on the result of this function.
 *
 * The caller owns the return value and should call
 * cairo_surface_destroy() when finished with it. This function will not
 * return %NULL, but will return a nil surface instead.
 *
 * Return value: The snapshot surface. Note that the return surface
 * may not necessarily be of the same type as @surface.
 **/
cairo_surface_t *
_cairo_surface_snapshot (cairo_surface_t *surface)
{
    cairo_surface_t *snapshot;
    cairo_status_t status;

    if (surface->status)
	return _cairo_surface_create_in_error (surface->status);

    if (surface->finished)
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_SURFACE_FINISHED));

    if (surface->snapshot_of != NULL)
	return cairo_surface_reference (surface);

    snapshot = _cairo_surface_has_snapshot (surface,
					    surface->backend,
					    surface->content);
    if (snapshot != NULL)
	return cairo_surface_reference (snapshot);

    if (surface->backend->snapshot != NULL) {
	snapshot = surface->backend->snapshot (surface);
	if (snapshot != NULL) {
	    if (unlikely (snapshot->status))
		return snapshot;

	    /* Is this surface just a proxy - e.g. paginated surfaces? */
	    if (snapshot->backend != surface->backend) {
		cairo_surface_t *previous;

		previous = _cairo_surface_has_snapshot (surface,
							snapshot->backend,
							snapshot->content);
		if (previous != NULL) {
		    cairo_surface_destroy (snapshot);
		    return cairo_surface_reference (previous);
		}
	    }
	}
    }

    if (snapshot == NULL) {
	snapshot = _cairo_surface_has_snapshot (surface,
						&_cairo_image_surface_backend,
						surface->content);
	if (snapshot != NULL)
	    return cairo_surface_reference (snapshot);

	snapshot = _cairo_surface_fallback_snapshot (surface);
	if (unlikely (snapshot->status))
	    return snapshot;
    }

    status = _cairo_surface_copy_mime_data (snapshot, surface);
    if (unlikely (status)) {
	cairo_surface_destroy (snapshot);
	return _cairo_surface_create_in_error (status);
    }

    snapshot->device_transform = surface->device_transform;
    snapshot->device_transform_inverse = surface->device_transform_inverse;

    status = _cairo_surface_attach_snapshot (surface, snapshot, NULL);
    if (unlikely (status)) {
	cairo_surface_destroy (snapshot);
	return _cairo_surface_create_in_error (status);
    }

    return snapshot;
}

/**
 * _cairo_surface_is_similar
 * @surface_a: a #cairo_surface_t
 * @surface_b: a #cairo_surface_t
 * @content: a #cairo_content_t
 *
 * Find out whether the given surfaces share the same backend,
 * and if so, whether they can be considered similar.
 *
 * The definition of "similar" depends on the backend. In
 * general, it means that the surface is equivalent to one
 * that would have been generated by a call to cairo_surface_create_similar().
 *
 * Return value: %TRUE if the surfaces are similar.
 **/
cairo_bool_t
_cairo_surface_is_similar (cairo_surface_t *surface_a,
	                   cairo_surface_t *surface_b,
			   cairo_content_t content)
{
    if (surface_a->backend != surface_b->backend)
	return FALSE;

    if (surface_a->backend->is_similar != NULL)
	return surface_a->backend->is_similar (surface_a, surface_b, content);

    return TRUE;
}

cairo_status_t
_cairo_surface_composite (cairo_operator_t	op,
			  const cairo_pattern_t	*src,
			  const cairo_pattern_t	*mask,
			  cairo_surface_t	*dst,
			  int			src_x,
			  int			src_y,
			  int			mask_x,
			  int			mask_y,
			  int			dst_x,
			  int			dst_y,
			  unsigned int		width,
			  unsigned int		height,
			  cairo_region_t	*clip_region)
{
    cairo_int_status_t status;

    if (unlikely (dst->status))
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    if (mask) {
	/* These operators aren't interpreted the same way by the backends;
	 * they are implemented in terms of other operators in cairo-gstate.c
	 */
	assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);
    }

    if (dst->backend->composite) {
	status = dst->backend->composite (op,
					  src, mask, dst,
                                          src_x, src_y,
                                          mask_x, mask_y,
                                          dst_x, dst_y,
					  width, height,
					  clip_region);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (dst, status);
    }

    return _cairo_surface_set_error (dst,
	    _cairo_surface_fallback_composite (op,
					      src, mask, dst,
					      src_x, src_y,
					      mask_x, mask_y,
					      dst_x, dst_y,
					      width, height,
					      clip_region));
}

/**
 * _cairo_surface_fill_rectangle:
 * @surface: a #cairo_surface_t
 * @op: the operator to apply to the rectangle
 * @color: the source color
 * @x: X coordinate of rectangle, in backend coordinates
 * @y: Y coordinate of rectangle, in backend coordinates
 * @width: width of rectangle, in backend coordinates
 * @height: height of rectangle, in backend coordinates
 *
 * Applies an operator to a rectangle using a solid color as the source.
 * See _cairo_surface_fill_rectangles() for full details.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or the error that occurred
 **/
cairo_status_t
_cairo_surface_fill_rectangle (cairo_surface_t	   *surface,
			       cairo_operator_t	    op,
			       const cairo_color_t *color,
			       int		    x,
			       int		    y,
			       int		    width,
			       int		    height)
{
    cairo_rectangle_int_t rect;

    if (surface->status)
	return surface->status;

    assert (_cairo_surface_is_writable (surface));

    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    return _cairo_surface_fill_rectangles (surface, op, color, &rect, 1);
}

/**
 * _cairo_surface_fill_region:
 * @surface: a #cairo_surface_t
 * @op: the operator to apply to the region
 * @color: the source color
 * @region: the region to modify, in backend coordinates
 *
 * Applies an operator to a set of rectangles specified as a
 * #cairo_region_t using a solid color as the source.
 * See _cairo_surface_fill_rectangles() for full details.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or the error that occurred
 **/
cairo_status_t
_cairo_surface_fill_region (cairo_surface_t	   *surface,
			    cairo_operator_t	    op,
			    const cairo_color_t    *color,
			    cairo_region_t         *region)
{
    int num_rects;
    cairo_rectangle_int_t stack_rects[CAIRO_STACK_ARRAY_LENGTH (cairo_rectangle_int_t)];
    cairo_rectangle_int_t *rects = stack_rects;
    cairo_status_t status;
    int i;

    if (surface->status)
	return surface->status;

    assert (_cairo_surface_is_writable (surface));

    num_rects = cairo_region_num_rectangles (region);
    if (num_rects == 0)
	return CAIRO_STATUS_SUCCESS;

    /* catch a common reduction of _cairo_clip_combine_with_surface() */
    if (op == CAIRO_OPERATOR_IN &&
	_cairo_color_equal (color, CAIRO_COLOR_WHITE))
    {
	return CAIRO_STATUS_SUCCESS;
    }

    if (num_rects > ARRAY_LENGTH (stack_rects)) {
	rects = _cairo_malloc_ab (num_rects,
				  sizeof (cairo_rectangle_int_t));
	if (rects == NULL) {
	    return _cairo_surface_set_error (surface,
					     _cairo_error (CAIRO_STATUS_NO_MEMORY));
	}
    }

    for (i = 0; i < num_rects; i++)
	cairo_region_get_rectangle (region, i, &rects[i]);

    status =  _cairo_surface_fill_rectangles (surface,
					      op, color, rects, num_rects);

    if (rects != stack_rects)
	free (rects);

    return _cairo_surface_set_error (surface, status);
}

/**
 * _cairo_surface_fill_rectangles:
 * @surface: a #cairo_surface_t
 * @op: the operator to apply to the region
 * @color: the source color
 * @rects: the rectangles to modify, in backend coordinates
 * @num_rects: the number of rectangles in @rects
 *
 * Applies an operator to a set of rectangles using a solid color
 * as the source. Note that even if the operator is an unbounded operator
 * such as %CAIRO_OPERATOR_IN, only the given set of rectangles
 * is affected. This differs from _cairo_surface_composite_trapezoids()
 * where the entire destination rectangle is cleared.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or the error that occurred
 **/
cairo_status_t
_cairo_surface_fill_rectangles (cairo_surface_t		*surface,
				cairo_operator_t         op,
				const cairo_color_t	*color,
				cairo_rectangle_int_t	*rects,
				int			 num_rects)
{
    cairo_int_status_t status;

    if (surface->status)
	return surface->status;

    assert (_cairo_surface_is_writable (surface));

    if (num_rects == 0)
	return CAIRO_STATUS_SUCCESS;

    if (surface->backend->fill_rectangles) {
	status = surface->backend->fill_rectangles (surface,
						    op, color,
						    rects, num_rects);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (surface, status);
    }

    return _cairo_surface_set_error (surface,
	    _cairo_surface_fallback_fill_rectangles (surface,
						     op, color,
						     rects, num_rects));
}

cairo_status_t
_cairo_surface_paint (cairo_surface_t	*surface,
		      cairo_operator_t	 op,
		      const cairo_pattern_t *source,
		      cairo_clip_t	    *clip)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    _cairo_surface_begin_modification (surface);

    _cairo_surface_copy_pattern_for_destination (&source,
						 surface,
						 &dev_source.base);

    if (surface->backend->paint != NULL) {
	status = surface->backend->paint (surface, op, source, clip);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_paint (surface, op, source, clip);

 FINISH:
    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_mask (cairo_surface_t		*surface,
		     cairo_operator_t		 op,
		     const cairo_pattern_t	*source,
		     const cairo_pattern_t	*mask,
		     cairo_clip_t		*clip)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;
    cairo_pattern_union_t dev_mask;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    _cairo_surface_begin_modification (surface);

    _cairo_surface_copy_pattern_for_destination (&source, surface,
						 &dev_source.base);
    _cairo_surface_copy_pattern_for_destination (&mask, surface,
						 &dev_mask.base);

    if (surface->backend->mask != NULL) {
	status = surface->backend->mask (surface, op, source, mask, clip);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_mask (surface, op, source, mask, clip);

 FINISH:
    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_fill_stroke (cairo_surface_t	    *surface,
			    cairo_operator_t	     fill_op,
			    const cairo_pattern_t   *fill_source,
			    cairo_fill_rule_t	     fill_rule,
			    double		     fill_tolerance,
			    cairo_antialias_t	     fill_antialias,
			    cairo_path_fixed_t	    *path,
			    cairo_operator_t	     stroke_op,
			    const cairo_pattern_t   *stroke_source,
			    cairo_stroke_style_t    *stroke_style,
			    cairo_matrix_t	    *stroke_ctm,
			    cairo_matrix_t	    *stroke_ctm_inverse,
			    double		     stroke_tolerance,
			    cairo_antialias_t	     stroke_antialias,
			    cairo_clip_t	    *clip)
{
    cairo_status_t status;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    _cairo_surface_begin_modification (surface);

    if (surface->backend->fill_stroke) {
	cairo_pattern_union_t dev_stroke_source;
	cairo_pattern_union_t dev_fill_source;
	cairo_matrix_t dev_ctm = *stroke_ctm;
	cairo_matrix_t dev_ctm_inverse = *stroke_ctm_inverse;

	_cairo_surface_copy_pattern_for_destination (&stroke_source, surface,
						     &dev_stroke_source.base);
	_cairo_surface_copy_pattern_for_destination (&fill_source, surface,
						     &dev_fill_source.base);

	status = surface->backend->fill_stroke (surface,
						fill_op, fill_source, fill_rule,
						fill_tolerance, fill_antialias,
						path,
						stroke_op, stroke_source,
						stroke_style,
						&dev_ctm, &dev_ctm_inverse,
						stroke_tolerance, stroke_antialias,
						clip);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (surface, status);
    }

    status = _cairo_surface_fill (surface, fill_op, fill_source, path,
				  fill_rule, fill_tolerance, fill_antialias,
				  clip);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    status = _cairo_surface_stroke (surface, stroke_op, stroke_source, path,
				    stroke_style, stroke_ctm, stroke_ctm_inverse,
				    stroke_tolerance, stroke_antialias,
				    clip);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_stroke (cairo_surface_t		*surface,
		       cairo_operator_t		 op,
		       const cairo_pattern_t	*source,
		       cairo_path_fixed_t	*path,
		       cairo_stroke_style_t	*stroke_style,
		       cairo_matrix_t		*ctm,
		       cairo_matrix_t		*ctm_inverse,
		       double			 tolerance,
		       cairo_antialias_t	 antialias,
		       cairo_clip_t		*clip)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    _cairo_surface_begin_modification (surface);

    _cairo_surface_copy_pattern_for_destination (&source, surface,
						 &dev_source.base);

    if (surface->backend->stroke != NULL) {
	status = surface->backend->stroke (surface, op, source,
					   path, stroke_style,
					   ctm, ctm_inverse,
					   tolerance, antialias,
					   clip);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_stroke (surface, op, source,
                                             path, stroke_style,
                                             ctm, ctm_inverse,
                                             tolerance, antialias,
					     clip);

 FINISH:
    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_fill (cairo_surface_t	*surface,
		     cairo_operator_t	 op,
		     const cairo_pattern_t *source,
		     cairo_path_fixed_t	*path,
		     cairo_fill_rule_t	 fill_rule,
		     double		 tolerance,
		     cairo_antialias_t	 antialias,
		     cairo_clip_t	*clip)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;

    if (unlikely (surface->status))
	return surface->status;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    _cairo_surface_begin_modification (surface);

    _cairo_surface_copy_pattern_for_destination (&source, surface,
						 &dev_source.base);
    if (surface->backend->fill != NULL) {
	status = surface->backend->fill (surface, op, source,
					 path, fill_rule,
					 tolerance, antialias,
					 clip);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_fill (surface, op, source,
                                           path, fill_rule,
                                           tolerance, antialias,
					   clip);

 FINISH:
    return _cairo_surface_set_error (surface, status);
}

cairo_status_t
_cairo_surface_composite_trapezoids (cairo_operator_t		op,
				     const cairo_pattern_t	*pattern,
				     cairo_surface_t		*dst,
				     cairo_antialias_t		antialias,
				     int			src_x,
				     int			src_y,
				     int			dst_x,
				     int			dst_y,
				     unsigned int		width,
				     unsigned int		height,
				     cairo_trapezoid_t		*traps,
				     int			num_traps,
				     cairo_region_t		*clip_region)
{
    cairo_int_status_t status;

    if (dst->status)
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    /* These operators aren't interpreted the same way by the backends;
     * they are implemented in terms of other operators in cairo-gstate.c
     */
    assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);

    if (dst->backend->composite_trapezoids) {
	status = dst->backend->composite_trapezoids (op,
						     pattern, dst,
						     antialias,
						     src_x, src_y,
                                                     dst_x, dst_y,
						     width, height,
						     traps, num_traps,
						     clip_region);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return _cairo_surface_set_error (dst, status);
    }

    return  _cairo_surface_set_error (dst,
	    _cairo_surface_fallback_composite_trapezoids (op, pattern, dst,
							  antialias,
							  src_x, src_y,
							  dst_x, dst_y,
							  width, height,
							  traps, num_traps,
							  clip_region));
}

cairo_span_renderer_t *
_cairo_surface_create_span_renderer (cairo_operator_t		 op,
				     const cairo_pattern_t	*pattern,
				     cairo_surface_t		*dst,
				     cairo_antialias_t	         antialias,
				     const cairo_composite_rectangles_t *rects,
				     cairo_region_t		*clip_region)
{
    assert (dst->snapshot_of == NULL);

    if (unlikely (dst->status))
	return _cairo_span_renderer_create_in_error (dst->status);

    if (unlikely (dst->finished))
	return _cairo_span_renderer_create_in_error (CAIRO_STATUS_SURFACE_FINISHED);

    if (dst->backend->create_span_renderer) {
	return dst->backend->create_span_renderer (op,
						   pattern, dst,
						   antialias,
						   rects,
						   clip_region);
    }
    ASSERT_NOT_REACHED;
    return _cairo_span_renderer_create_in_error (CAIRO_INT_STATUS_UNSUPPORTED);
}

cairo_bool_t
_cairo_surface_check_span_renderer (cairo_operator_t		 op,
				    const cairo_pattern_t	*pattern,
				    cairo_surface_t		*dst,
				    cairo_antialias_t	         antialias)
{
    assert (dst->snapshot_of == NULL);
    assert (dst->status == CAIRO_STATUS_SUCCESS);
    assert (! dst->finished);

    /* XXX: Currently we have no mono span renderer */
    if (antialias == CAIRO_ANTIALIAS_NONE)
	return FALSE;

    if (dst->backend->check_span_renderer != NULL)
	return dst->backend->check_span_renderer (op, pattern, dst, antialias);

    return FALSE;
}

/**
 * cairo_surface_copy_page:
 * @surface: a #cairo_surface_t
 *
 * Emits the current page for backends that support multiple pages,
 * but doesn't clear it, so that the contents of the current page will
 * be retained for the next page.  Use cairo_surface_show_page() if you
 * want to get an empty page after the emission.
 *
 * There is a convenience function for this that takes a #cairo_t,
 * namely cairo_copy_page().
 *
 * Since: 1.6
 */
void
cairo_surface_copy_page (cairo_surface_t *surface)
{
    cairo_status_t status_ignored;

    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	status_ignored = _cairo_surface_set_error (surface,
		                                 CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    /* It's fine if some backends don't implement copy_page */
    if (surface->backend->copy_page == NULL)
	return;

    status_ignored = _cairo_surface_set_error (surface,
			                 surface->backend->copy_page (surface));
}
slim_hidden_def (cairo_surface_copy_page);

/**
 * cairo_surface_show_page:
 * @surface: a #cairo_Surface_t
 *
 * Emits and clears the current page for backends that support multiple
 * pages.  Use cairo_surface_copy_page() if you don't want to clear the page.
 *
 * There is a convenience function for this that takes a #cairo_t,
 * namely cairo_show_page().
 *
 * Since: 1.6
 **/
void
cairo_surface_show_page (cairo_surface_t *surface)
{
    cairo_status_t status_ignored;

    if (surface->status)
	return;

    _cairo_surface_begin_modification (surface);

    if (surface->finished) {
	status_ignored = _cairo_surface_set_error (surface,
		                                 CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    /* It's fine if some backends don't implement show_page */
    if (surface->backend->show_page == NULL)
	return;

    status_ignored = _cairo_surface_set_error (surface,
			                 surface->backend->show_page (surface));
}
slim_hidden_def (cairo_surface_show_page);

/**
 * _cairo_surface_get_extents:
 * @surface: the #cairo_surface_t to fetch extents for
 *
 * This function returns a bounding box for the surface.  The surface
 * bounds are defined as a region beyond which no rendering will
 * possibly be recorded, in other words, it is the maximum extent of
 * potentially usable coordinates.
 *
 * For vector surfaces, (PDF, PS, SVG and meta-surfaces), the surface
 * might be conceived as unbounded, but we force the user to provide a
 * maximum size at the time of surface_create. So get_extents uses
 * that size.
 *
 * Note: The coordinates returned are in "backend" space rather than
 * "surface" space. That is, they are relative to the true (0,0)
 * origin rather than the device_transform origin. This might seem a
 * bit inconsistent with other #cairo_surface_t interfaces, but all
 * current callers are within the surface layer where backend space is
 * desired.
 *
 * This behavior would have to be changed is we ever exported a public
 * variant of this function.
 */
cairo_bool_t
_cairo_surface_get_extents (cairo_surface_t         *surface,
			    cairo_rectangle_int_t   *extents)
{
    cairo_bool_t bounded = FALSE;

    if (unlikely (surface->status || surface->finished))
	return TRUE;

    if (surface->backend->get_extents != NULL)
	bounded = surface->backend->get_extents (surface, extents);

    if (! bounded)
	_cairo_unbounded_rectangle_init (extents);

    return bounded;
}

/**
 * cairo_surface_has_show_text_glyphs:
 * @surface: a #cairo_surface_t
 *
 * Returns whether the surface supports
 * sophisticated cairo_show_text_glyphs() operations.  That is,
 * whether it actually uses the provided text and cluster data
 * to a cairo_show_text_glyphs() call.
 *
 * Note: Even if this function returns %FALSE, a
 * cairo_show_text_glyphs() operation targeted at @surface will
 * still succeed.  It just will
 * act like a cairo_show_glyphs() operation.  Users can use this
 * function to avoid computing UTF-8 text and cluster mapping if the
 * target surface does not use it.
 *
 * Return value: %TRUE if @surface supports
 *               cairo_show_text_glyphs(), %FALSE otherwise
 *
 * Since: 1.8
 **/
cairo_bool_t
cairo_surface_has_show_text_glyphs (cairo_surface_t	    *surface)
{
    cairo_status_t status_ignored;

    if (surface->status)
	return FALSE;

    if (surface->finished) {
	status_ignored = _cairo_surface_set_error (surface,
						   CAIRO_STATUS_SURFACE_FINISHED);
	return FALSE;
    }

    if (surface->backend->has_show_text_glyphs)
	return surface->backend->has_show_text_glyphs (surface);
    else
	return surface->backend->show_text_glyphs != NULL;
}
slim_hidden_def (cairo_surface_has_show_text_glyphs);

/* Note: the backends may modify the contents of the glyph array as long as
 * they do not return %CAIRO_INT_STATUS_UNSUPPORTED. This makes it possible to
 * avoid copying the array again and again, and edit it in-place.
 * Backends are in fact free to use the array as a generic buffer as they
 * see fit.
 *
 * For show_glyphs backend method, and NOT for show_text_glyphs method,
 * when they do return UNSUPPORTED, they may adjust remaining_glyphs to notify
 * that they have successfully rendered some of the glyphs (from the beginning
 * of the array), but not all.  If they don't touch remaining_glyphs, it
 * defaults to all glyphs.
 *
 * See commits 5a9642c5746fd677aed35ce620ce90b1029b1a0c and
 * 1781e6018c17909311295a9cc74b70500c6b4d0a for the rationale.
 */
cairo_status_t
_cairo_surface_show_text_glyphs (cairo_surface_t	    *surface,
				 cairo_operator_t	     op,
				 const cairo_pattern_t	    *source,
				 const char		    *utf8,
				 int			     utf8_len,
				 cairo_glyph_t		    *glyphs,
				 int			     num_glyphs,
				 const cairo_text_cluster_t *clusters,
				 int			     num_clusters,
				 cairo_text_cluster_flags_t  cluster_flags,
				 cairo_scaled_font_t	    *scaled_font,
				 cairo_clip_t		    *clip)
{
    cairo_status_t status;
    cairo_scaled_font_t *dev_scaled_font = scaled_font;
    cairo_pattern_union_t dev_source;

    if (unlikely (surface->status))
	return surface->status;

    if (num_glyphs == 0 && utf8_len == 0)
	return CAIRO_STATUS_SUCCESS;

    if (clip && clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    _cairo_surface_begin_modification (surface);

    _cairo_surface_copy_pattern_for_destination (&source, surface,
						 &dev_source.base);

    if (_cairo_surface_has_device_transform (surface) &&
	! _cairo_matrix_is_integer_translation (&surface->device_transform, NULL, NULL))
    {
	cairo_font_options_t font_options;
	cairo_matrix_t dev_ctm, font_matrix;

	cairo_scaled_font_get_font_matrix (scaled_font, &font_matrix);
	cairo_scaled_font_get_ctm (scaled_font, &dev_ctm);
	cairo_matrix_multiply (&dev_ctm, &dev_ctm, &surface->device_transform);
	cairo_scaled_font_get_font_options (scaled_font, &font_options);
	dev_scaled_font = cairo_scaled_font_create (cairo_scaled_font_get_font_face (scaled_font),
						    &font_matrix,
						    &dev_ctm,
						    &font_options);
    }
    status = cairo_scaled_font_status (dev_scaled_font);
    if (unlikely (status))
	return _cairo_surface_set_error (surface, status);

    status = CAIRO_INT_STATUS_UNSUPPORTED;

    /* The logic here is duplicated in _cairo_analysis_surface show_glyphs and
     * show_text_glyphs.  Keep in synch. */
    if (clusters) {
	/* A real show_text_glyphs call.  Try show_text_glyphs backend
	 * method first */
	if (surface->backend->show_text_glyphs != NULL) {
	    status = surface->backend->show_text_glyphs (surface, op,
							 source,
							 utf8, utf8_len,
							 glyphs, num_glyphs,
							 clusters, num_clusters, cluster_flags,
							 dev_scaled_font,
							 clip);
	}
	if (status == CAIRO_INT_STATUS_UNSUPPORTED &&
	    surface->backend->show_glyphs)
	{
	    int remaining_glyphs = num_glyphs;
	    status = surface->backend->show_glyphs (surface, op,
						    source,
						    glyphs, num_glyphs,
						    dev_scaled_font,
						    clip,
						    &remaining_glyphs);
	    glyphs += num_glyphs - remaining_glyphs;
	    num_glyphs = remaining_glyphs;
	    if (status == CAIRO_INT_STATUS_UNSUPPORTED && remaining_glyphs == 0)
		status = CAIRO_STATUS_SUCCESS;
	}
    } else {
	/* A mere show_glyphs call.  Try show_glyphs backend method first */
	if (surface->backend->show_glyphs != NULL) {
	    int remaining_glyphs = num_glyphs;
	    status = surface->backend->show_glyphs (surface, op,
						    source,
						    glyphs, num_glyphs,
						    dev_scaled_font,
						    clip,
						    &remaining_glyphs);
	    glyphs += num_glyphs - remaining_glyphs;
	    num_glyphs = remaining_glyphs;
	    if (status == CAIRO_INT_STATUS_UNSUPPORTED && remaining_glyphs == 0)
		status = CAIRO_STATUS_SUCCESS;
	} else if (surface->backend->show_text_glyphs != NULL) {
	    /* Intentionally only try show_text_glyphs method for show_glyphs
	     * calls if backend does not have show_glyphs.  If backend has
	     * both methods implemented, we don't fallback from show_glyphs to
	     * show_text_glyphs, and hence the backend can assume in its
	     * show_text_glyphs call that clusters is not NULL (which also
	     * implies that UTF-8 is not NULL, unless the text is
	     * zero-length).
	     */
	    status = surface->backend->show_text_glyphs (surface, op,
							 source,
							 utf8, utf8_len,
							 glyphs, num_glyphs,
							 clusters, num_clusters, cluster_flags,
							 dev_scaled_font,
							 clip);
	}
    }

    if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	status = _cairo_surface_fallback_show_glyphs (surface, op,
						      source,
						      glyphs, num_glyphs,
						      dev_scaled_font,
						      clip);
    }

    if (dev_scaled_font != scaled_font)
	cairo_scaled_font_destroy (dev_scaled_font);

    return _cairo_surface_set_error (surface, status);
}

/* XXX: Previously, we had a function named _cairo_surface_show_glyphs
 * with not-so-useful semantics. We've now got a
 * _cairo_surface_show_text_glyphs with the proper semantics, and its
 * fallback still uses this old function (which still needs to be
 * cleaned up in terms of both semantics and naming). */
cairo_status_t
_cairo_surface_old_show_glyphs (cairo_scaled_font_t	*scaled_font,
				cairo_operator_t	 op,
				const cairo_pattern_t	*pattern,
				cairo_surface_t		*dst,
				int			 source_x,
				int			 source_y,
				int			 dest_x,
				int			 dest_y,
				unsigned int		 width,
				unsigned int		 height,
				cairo_glyph_t		*glyphs,
				int			 num_glyphs,
				cairo_region_t		*clip_region)
{
    cairo_status_t status;

    if (dst->status)
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    if (dst->backend->old_show_glyphs) {
	status = dst->backend->old_show_glyphs (scaled_font,
						op, pattern, dst,
						source_x, source_y,
                                                dest_x, dest_y,
						width, height,
						glyphs, num_glyphs,
						clip_region);
    } else
	status = CAIRO_INT_STATUS_UNSUPPORTED;

    return _cairo_surface_set_error (dst, status);
}

static cairo_status_t
_cairo_surface_composite_fixup_unbounded_internal (cairo_surface_t         *dst,
						   cairo_rectangle_int_t   *src_rectangle,
						   cairo_rectangle_int_t   *mask_rectangle,
						   int			    dst_x,
						   int			    dst_y,
						   unsigned int		    width,
						   unsigned int		    height,
						   cairo_region_t	    *clip_region)
{
    cairo_rectangle_int_t dst_rectangle;
    cairo_region_t clear_region;
    cairo_status_t status;

    /* The area that was drawn is the area in the destination rectangle but
     * not within the source or the mask.
     */
    dst_rectangle.x = dst_x;
    dst_rectangle.y = dst_y;
    dst_rectangle.width = width;
    dst_rectangle.height = height;

    _cairo_region_init_rectangle (&clear_region, &dst_rectangle);

    if (clip_region != NULL) {
	status = cairo_region_intersect (&clear_region, clip_region);
	if (unlikely (status))
	    goto CLEANUP_REGIONS;
    }

    if (src_rectangle != NULL) {
        if (! _cairo_rectangle_intersect (&dst_rectangle, src_rectangle))
	    goto EMPTY;
    }

    if (mask_rectangle != NULL) {
        if (! _cairo_rectangle_intersect (&dst_rectangle, mask_rectangle))
	    goto EMPTY;
    }

    /* Now compute the area that is in dst but not drawn */
    status = cairo_region_subtract_rectangle (&clear_region, &dst_rectangle);
    if (unlikely (status) || cairo_region_is_empty (&clear_region))
        goto CLEANUP_REGIONS;

  EMPTY:
    status = _cairo_surface_fill_region (dst, CAIRO_OPERATOR_CLEAR,
                                         CAIRO_COLOR_TRANSPARENT,
                                         &clear_region);

  CLEANUP_REGIONS:
    _cairo_region_fini (&clear_region);

    return _cairo_surface_set_error (dst, status);
}

/**
 * _cairo_surface_composite_fixup_unbounded:
 * @dst: the destination surface
 * @src_attr: source surface attributes (from _cairo_pattern_acquire_surface())
 * @src_width: width of source surface
 * @src_height: height of source surface
 * @mask_attr: mask surface attributes or %NULL if no mask
 * @mask_width: width of mask surface
 * @mask_height: height of mask surface
 * @src_x: @src_x from _cairo_surface_composite()
 * @src_y: @src_y from _cairo_surface_composite()
 * @mask_x: @mask_x from _cairo_surface_composite()
 * @mask_y: @mask_y from _cairo_surface_composite()
 * @dst_x: @dst_x from _cairo_surface_composite()
 * @dst_y: @dst_y from _cairo_surface_composite()
 * @width: @width from _cairo_surface_composite()
 * @height: @height_x from _cairo_surface_composite()
 *
 * Eeek! Too many parameters! This is a helper function to take care of fixing
 * up for bugs in libpixman and RENDER where, when asked to composite an
 * untransformed surface with an unbounded operator (like CLEAR or SOURCE)
 * only the region inside both the source and the mask is affected.
 * This function clears the region that should have been drawn but was wasn't.
 **/
cairo_status_t
_cairo_surface_composite_fixup_unbounded (cairo_surface_t            *dst,
					  cairo_surface_attributes_t *src_attr,
					  int                         src_width,
					  int                         src_height,
					  cairo_surface_attributes_t *mask_attr,
					  int                         mask_width,
					  int                         mask_height,
					  int			      src_x,
					  int			      src_y,
					  int			      mask_x,
					  int			      mask_y,
					  int			      dst_x,
					  int			      dst_y,
					  unsigned int		      width,
					  unsigned int		      height,
					  cairo_region_t	     *clip_region)
{
    cairo_rectangle_int_t src_tmp, mask_tmp;
    cairo_rectangle_int_t *src_rectangle = NULL;
    cairo_rectangle_int_t *mask_rectangle = NULL;

    if (unlikely (dst->status))
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    /* The RENDER/libpixman operators are clipped to the bounds of the untransformed,
     * non-repeating sources and masks. Other sources and masks can be ignored.
     */
    if (_cairo_matrix_is_integer_translation (&src_attr->matrix, NULL, NULL) &&
	src_attr->extend == CAIRO_EXTEND_NONE)
    {
	src_tmp.x = (dst_x - (src_x + src_attr->x_offset));
	src_tmp.y = (dst_y - (src_y + src_attr->y_offset));
	src_tmp.width = src_width;
	src_tmp.height = src_height;

	src_rectangle = &src_tmp;
    }

    if (mask_attr &&
	_cairo_matrix_is_integer_translation (&mask_attr->matrix, NULL, NULL) &&
	mask_attr->extend == CAIRO_EXTEND_NONE)
    {
	mask_tmp.x = (dst_x - (mask_x + mask_attr->x_offset));
	mask_tmp.y = (dst_y - (mask_y + mask_attr->y_offset));
	mask_tmp.width = mask_width;
	mask_tmp.height = mask_height;

	mask_rectangle = &mask_tmp;
    }

    return _cairo_surface_composite_fixup_unbounded_internal (dst, src_rectangle, mask_rectangle,
							      dst_x, dst_y, width, height,
							      clip_region);
}

/**
 * _cairo_surface_composite_shape_fixup_unbounded:
 * @dst: the destination surface
 * @src_attr: source surface attributes (from _cairo_pattern_acquire_surface())
 * @src_width: width of source surface
 * @src_height: height of source surface
 * @mask_width: width of mask surface
 * @mask_height: height of mask surface
 * @src_x: @src_x from _cairo_surface_composite()
 * @src_y: @src_y from _cairo_surface_composite()
 * @mask_x: @mask_x from _cairo_surface_composite()
 * @mask_y: @mask_y from _cairo_surface_composite()
 * @dst_x: @dst_x from _cairo_surface_composite()
 * @dst_y: @dst_y from _cairo_surface_composite()
 * @width: @width from _cairo_surface_composite()
 * @height: @height_x from _cairo_surface_composite()
 *
 * Like _cairo_surface_composite_fixup_unbounded(), but instead of
 * handling the case where we have a source pattern and a mask
 * pattern, handle the case where we are compositing a source pattern
 * using a mask we create ourselves, as in
 * _cairo_surface_composite_glyphs() or _cairo_surface_composite_trapezoids()
 **/
cairo_status_t
_cairo_surface_composite_shape_fixup_unbounded (cairo_surface_t            *dst,
						cairo_surface_attributes_t *src_attr,
						int                         src_width,
						int                         src_height,
						int                         mask_width,
						int                         mask_height,
						int			    src_x,
						int			    src_y,
						int			    mask_x,
						int			    mask_y,
						int			    dst_x,
						int			    dst_y,
						unsigned int		    width,
						unsigned int		    height,
						cairo_region_t	    *clip_region)
{
    cairo_rectangle_int_t src_tmp, *src= NULL;
    cairo_rectangle_int_t mask;

    if (dst->status)
	return dst->status;

    assert (_cairo_surface_is_writable (dst));

    /* The RENDER/libpixman operators are clipped to the bounds of the untransformed,
     * non-repeating sources and masks. Other sources and masks can be ignored.
     */
    if (_cairo_matrix_is_integer_translation (&src_attr->matrix, NULL, NULL) &&
	src_attr->extend == CAIRO_EXTEND_NONE)
    {
	src_tmp.x = (dst_x - (src_x + src_attr->x_offset));
	src_tmp.y = (dst_y - (src_y + src_attr->y_offset));
	src_tmp.width  = src_width;
	src_tmp.height = src_height;

	src = &src_tmp;
    }

    mask.x = dst_x - mask_x;
    mask.y = dst_y - mask_y;
    mask.width  = mask_width;
    mask.height = mask_height;

    return _cairo_surface_composite_fixup_unbounded_internal (dst, src, &mask,
							      dst_x, dst_y, width, height,
							      clip_region);
}

/**
 * _cairo_surface_copy_pattern_for_destination
 * @pattern: the pattern to copy
 * @destination: the destination surface for which the pattern is being copied
 * @pattern_copy: the location to hold the copy
 *
 * Copies the given pattern, taking into account device scale and offsets
 * of the destination surface.
 */
static void
_cairo_surface_copy_pattern_for_destination (const cairo_pattern_t **pattern,
                                             cairo_surface_t *destination,
                                             cairo_pattern_t *pattern_copy)
{
    if (! _cairo_surface_has_device_transform (destination))
	return;

    _cairo_pattern_init_static_copy (pattern_copy, *pattern);
    _cairo_pattern_transform (pattern_copy,
			      &destination->device_transform_inverse);

    *pattern = pattern_copy;
}

/**
 * _cairo_surface_set_resolution
 * @surface: the surface
 * @x_res: x resolution, in dpi
 * @y_res: y resolution, in dpi
 *
 * Set the actual surface resolution of @surface to the given x and y DPI.
 * Mainly used for correctly computing the scale factor when fallback
 * rendering needs to take place in the paginated surface.
 */
void
_cairo_surface_set_resolution (cairo_surface_t *surface,
			       double x_res,
			       double y_res)
{
    if (surface->status)
	return;

    surface->x_resolution = x_res;
    surface->y_resolution = y_res;
}

/* Generic methods for determining operation extents. */

static void
_rectangle_intersect_clip (cairo_rectangle_int_t *extents, cairo_clip_t *clip)
{
    const cairo_rectangle_int_t *clip_extents;
    cairo_bool_t is_empty;

    clip_extents = NULL;
    if (clip != NULL)
	clip_extents = _cairo_clip_get_extents (clip);

    if (clip_extents != NULL)
	is_empty = _cairo_rectangle_intersect (extents, clip_extents);
}

static void
_cairo_surface_operation_extents (cairo_surface_t *surface,
				  cairo_operator_t op,
				  const cairo_pattern_t *source,
				  cairo_clip_t *clip,
				  cairo_rectangle_int_t *extents)
{
    cairo_bool_t is_empty;

    is_empty = _cairo_surface_get_extents (surface, extents);

    if (_cairo_operator_bounded_by_source (op)) {
	cairo_rectangle_int_t source_extents;

	_cairo_pattern_get_extents (source, &source_extents);
	is_empty = _cairo_rectangle_intersect (extents, &source_extents);
    }

    _rectangle_intersect_clip (extents, clip);
}

cairo_status_t
_cairo_surface_paint_extents (cairo_surface_t *surface,
			      cairo_operator_t		op,
			      const cairo_pattern_t	*source,
			      cairo_clip_t		*clip,
			      cairo_rectangle_int_t	*extents)
{
    _cairo_surface_operation_extents (surface, op, source, clip, extents);
    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_mask_extents (cairo_surface_t *surface,
			     cairo_operator_t		 op,
			     const cairo_pattern_t	*source,
			     const cairo_pattern_t	*mask,
			     cairo_clip_t		*clip,
			     cairo_rectangle_int_t	*extents)
{
    cairo_bool_t is_empty;

    _cairo_surface_operation_extents (surface, op, source, clip, extents);

    if (_cairo_operator_bounded_by_mask (op)) {
	cairo_rectangle_int_t mask_extents;

	_cairo_pattern_get_extents (mask, &mask_extents);
	is_empty = _cairo_rectangle_intersect (extents, &mask_extents);
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_stroke_extents (cairo_surface_t *surface,
			       cairo_operator_t op,
			       const cairo_pattern_t *source,
			       cairo_path_fixed_t	*path,
			       cairo_stroke_style_t *style,
			       const cairo_matrix_t *ctm,
			       const cairo_matrix_t *ctm_inverse,
			       double tolerance,
			       cairo_antialias_t	 antialias,
			       cairo_clip_t *clip,
			       cairo_rectangle_int_t *extents)
{
    cairo_status_t status;
    cairo_bool_t is_empty;

    _cairo_surface_operation_extents (surface, op, source, clip, extents);

    if (_cairo_operator_bounded_by_mask (op)) {
	cairo_rectangle_int_t mask_extents;

	status = _cairo_path_fixed_stroke_extents (path, style,
						   ctm, ctm_inverse,
						   tolerance,
						   &mask_extents);
	if (unlikely (status))
	    return status;

	is_empty = _cairo_rectangle_intersect (extents, &mask_extents);
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_fill_extents (cairo_surface_t		*surface,
			     cairo_operator_t		 op,
			     const cairo_pattern_t	*source,
			     cairo_path_fixed_t		*path,
			     cairo_fill_rule_t		 fill_rule,
			     double			 tolerance,
			     cairo_antialias_t		 antialias,
			     cairo_clip_t		*clip,
			     cairo_rectangle_int_t	*extents)
{
    cairo_bool_t is_empty;

    _cairo_surface_operation_extents (surface, op, source, clip, extents);

    if (_cairo_operator_bounded_by_mask (op)) {
	cairo_rectangle_int_t mask_extents;

	_cairo_path_fixed_fill_extents (path, fill_rule, tolerance,
					&mask_extents);
	is_empty = _cairo_rectangle_intersect (extents, &mask_extents);
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_glyphs_extents (cairo_surface_t *surface,
			       cairo_operator_t	   op,
			       const cairo_pattern_t *source,
			       cairo_glyph_t	  *glyphs,
			       int		   num_glyphs,
			       cairo_scaled_font_t  *scaled_font,
			       cairo_clip_t         *clip,
			       cairo_rectangle_int_t *extents)
{
    cairo_status_t	     status;
    cairo_bool_t             is_empty;

    _cairo_surface_operation_extents (surface, op, source, clip, extents);

    if (_cairo_operator_bounded_by_mask (op)) {
	cairo_rectangle_int_t glyph_extents;

	status = _cairo_scaled_font_glyph_device_extents (scaled_font,
							  glyphs,
							  num_glyphs,
							  &glyph_extents,
							  NULL);
	if (unlikely (status))
	    return status;

	is_empty = _cairo_rectangle_intersect (extents, &glyph_extents);
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_surface_t *
_cairo_surface_create_in_error (cairo_status_t status)
{
    switch (status) {
    case CAIRO_STATUS_NO_MEMORY:
	return (cairo_surface_t *) &_cairo_surface_nil;
    case CAIRO_STATUS_SURFACE_TYPE_MISMATCH:
	return (cairo_surface_t *) &_cairo_surface_nil_surface_type_mismatch;
    case CAIRO_STATUS_INVALID_STATUS:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_status;
    case CAIRO_STATUS_INVALID_CONTENT:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_content;
    case CAIRO_STATUS_INVALID_FORMAT:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_format;
    case CAIRO_STATUS_INVALID_VISUAL:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_visual;
    case CAIRO_STATUS_READ_ERROR:
	return (cairo_surface_t *) &_cairo_surface_nil_read_error;
    case CAIRO_STATUS_WRITE_ERROR:
	return (cairo_surface_t *) &_cairo_surface_nil_write_error;
    case CAIRO_STATUS_FILE_NOT_FOUND:
	return (cairo_surface_t *) &_cairo_surface_nil_file_not_found;
    case CAIRO_STATUS_TEMP_FILE_ERROR:
	return (cairo_surface_t *) &_cairo_surface_nil_temp_file_error;
    case CAIRO_STATUS_INVALID_STRIDE:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_stride;
    case CAIRO_STATUS_INVALID_SIZE:
	return (cairo_surface_t *) &_cairo_surface_nil_invalid_size;
    case CAIRO_STATUS_SUCCESS:
    case CAIRO_STATUS_LAST_STATUS:
	ASSERT_NOT_REACHED;
	/* fall-through */
    case CAIRO_STATUS_INVALID_RESTORE:
    case CAIRO_STATUS_INVALID_POP_GROUP:
    case CAIRO_STATUS_NO_CURRENT_POINT:
    case CAIRO_STATUS_INVALID_MATRIX:
    case CAIRO_STATUS_NULL_POINTER:
    case CAIRO_STATUS_INVALID_STRING:
    case CAIRO_STATUS_INVALID_PATH_DATA:
    case CAIRO_STATUS_SURFACE_FINISHED:
    case CAIRO_STATUS_PATTERN_TYPE_MISMATCH:
    case CAIRO_STATUS_INVALID_DASH:
    case CAIRO_STATUS_INVALID_DSC_COMMENT:
    case CAIRO_STATUS_INVALID_INDEX:
    case CAIRO_STATUS_CLIP_NOT_REPRESENTABLE:
    case CAIRO_STATUS_FONT_TYPE_MISMATCH:
    case CAIRO_STATUS_USER_FONT_IMMUTABLE:
    case CAIRO_STATUS_USER_FONT_ERROR:
    case CAIRO_STATUS_NEGATIVE_COUNT:
    case CAIRO_STATUS_INVALID_CLUSTERS:
    case CAIRO_STATUS_INVALID_SLANT:
    case CAIRO_STATUS_INVALID_WEIGHT:
    case CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED:
    default:
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t *) &_cairo_surface_nil;
    }
}

/*  LocalWords:  rasterized
 */
