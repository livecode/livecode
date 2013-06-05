/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2009 Chris Wilson
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
 *	Kristian Høgsberg <krh@redhat.com>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "cairoint.h"
#include "cairo-clip-private.h"
#include "cairo-path-fixed-private.h"
#include "cairo-region-private.h"

/* Keep a stash of recently freed clip_paths, since we need to
 * reallocate them frequently.
 */
#define MAX_FREED_POOL_SIZE 4
typedef struct {
    void *pool[MAX_FREED_POOL_SIZE];
    int top;
} freed_pool_t;

static freed_pool_t clip_path_pool;

static void *
_atomic_fetch (void **slot)
{
    return _cairo_atomic_ptr_cmpxchg (slot, *slot, NULL);
}

static cairo_bool_t
_atomic_store (void **slot, void *ptr)
{
    return _cairo_atomic_ptr_cmpxchg (slot, NULL, ptr) == NULL;
}

static void *
_freed_pool_get (freed_pool_t *pool)
{
    void *ptr;
    int i;

    i = pool->top - 1;
    if (i < 0)
	i = 0;

    ptr = _atomic_fetch (&pool->pool[i]);
    if (ptr != NULL) {
	pool->top = i;
	return ptr;
    }

    /* either empty or contended */
    for (i = ARRAY_LENGTH (pool->pool); i--;) {
	ptr = _atomic_fetch (&pool->pool[i]);
	if (ptr != NULL) {
	    pool->top = i;
	    return ptr;
	}
    }

    /* empty */
    pool->top = 0;
    return NULL;
}

static void
_freed_pool_put (freed_pool_t *pool, void *ptr)
{
    int i = pool->top;

    if (_atomic_store (&pool->pool[i], ptr)) {
	pool->top = i + 1;
	return;
    }

    /* either full or contended */
    for (i = 0; i < ARRAY_LENGTH (pool->pool); i++) {
	if (_atomic_store (&pool->pool[i], ptr)) {
	    pool->top = i + 1;
	    return;
	}
    }

    /* full */
    pool->top = ARRAY_LENGTH (pool->pool);
    free (ptr);
}

static void
_freed_pool_reset (freed_pool_t *pool)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (pool->pool); i++) {
	free (pool->pool[i]);
	pool->pool[i] = NULL;
    }
}

static cairo_clip_path_t *
_cairo_clip_path_create (cairo_clip_t *clip)
{
    cairo_clip_path_t *clip_path;

    clip_path = _freed_pool_get (&clip_path_pool);
    if (unlikely (clip_path == NULL)) {
	clip_path = malloc (sizeof (cairo_clip_path_t));
	if (unlikely (clip_path == NULL))
	    return NULL;
    }

    CAIRO_REFERENCE_COUNT_INIT (&clip_path->ref_count, 1);

    clip_path->flags = 0;
    clip_path->region = NULL;
    clip_path->surface = NULL;

    clip_path->prev = clip->path;
    clip->path = clip_path;

    return clip_path;
}

static cairo_clip_path_t *
_cairo_clip_path_reference (cairo_clip_path_t *clip_path)
{
    if (clip_path == NULL)
	return NULL;

    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&clip_path->ref_count));

    _cairo_reference_count_inc (&clip_path->ref_count);

    return clip_path;
}

static void
_cairo_clip_path_destroy (cairo_clip_path_t *clip_path)
{
    assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&clip_path->ref_count));

    if (! _cairo_reference_count_dec_and_test (&clip_path->ref_count))
	return;

    _cairo_path_fixed_fini (&clip_path->path);
    if (clip_path->region != NULL)
	cairo_region_destroy (clip_path->region);
    if (clip_path->surface != NULL)
	cairo_surface_destroy (clip_path->surface);

    if (clip_path->prev != NULL)
	_cairo_clip_path_destroy (clip_path->prev);

    _freed_pool_put (&clip_path_pool, clip_path);
}

void
_cairo_clip_init (cairo_clip_t *clip)
{
    clip->all_clipped = FALSE;
    clip->path = NULL;
}

static void
_cairo_clip_set_all_clipped (cairo_clip_t *clip)
{
    clip->all_clipped = TRUE;
    if (clip->path != NULL) {
	_cairo_clip_path_destroy (clip->path);
	clip->path = NULL;
    }
}

static cairo_status_t
_cairo_clip_intersect_rectangle (cairo_clip_t *clip,
				 const cairo_rectangle_int_t *rect)
{
    cairo_clip_path_t *clip_path;
    cairo_status_t status;

    if (clip->path != NULL) {
	if (rect->x <= clip->path->extents.x &&
	    rect->y <= clip->path->extents.y &&
	    rect->x + rect->width >= clip->path->extents.x + clip->path->extents.width &&
	    rect->y + rect->height >= clip->path->extents.y + clip->path->extents.height)
	{
	    return CAIRO_STATUS_SUCCESS;
	}
    }

    clip_path = _cairo_clip_path_create (clip);
    if (unlikely (clip_path == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_path_fixed_init (&clip_path->path);

    status = _cairo_path_fixed_move_to (&clip_path->path,
					_cairo_fixed_from_int (rect->x),
					_cairo_fixed_from_int (rect->y));
    assert (status == CAIRO_STATUS_SUCCESS);
    status = _cairo_path_fixed_rel_line_to (&clip_path->path,
					    _cairo_fixed_from_int (rect->width),
					    _cairo_fixed_from_int (0));
    assert (status == CAIRO_STATUS_SUCCESS);
    status = _cairo_path_fixed_rel_line_to (&clip_path->path,
					    _cairo_fixed_from_int (0),
					    _cairo_fixed_from_int (rect->height));
    assert (status == CAIRO_STATUS_SUCCESS);
    status = _cairo_path_fixed_rel_line_to (&clip_path->path,
					    _cairo_fixed_from_int (-rect->width),
					    _cairo_fixed_from_int (0));
    assert (status == CAIRO_STATUS_SUCCESS);
    status = _cairo_path_fixed_close_path (&clip_path->path);
    assert (status == CAIRO_STATUS_SUCCESS);

    clip_path->extents = *rect;
    clip_path->fill_rule = CAIRO_FILL_RULE_WINDING;
    clip_path->tolerance = 1;
    clip_path->antialias = CAIRO_ANTIALIAS_NONE;
    clip_path->flags |= CAIRO_CLIP_PATH_IS_BOX;

    /* could preallocate the region if it proves worthwhile */

    return CAIRO_STATUS_SUCCESS;
}

/* XXX consider accepting a matrix, no users yet. */
cairo_status_t
_cairo_clip_init_rectangle (cairo_clip_t *clip,
			    const cairo_rectangle_int_t *rect)
{
    _cairo_clip_init (clip);

    if (rect == NULL)
	return CAIRO_STATUS_SUCCESS;

    if (rect->width == 0 || rect->height == 0) {
	_cairo_clip_set_all_clipped (clip);
	return CAIRO_STATUS_SUCCESS;
    }

    return _cairo_clip_intersect_rectangle (clip, rect);
}

cairo_clip_t *
_cairo_clip_init_copy (cairo_clip_t *clip, cairo_clip_t *other)
{
    if (other != NULL) {
	if (other->path == NULL) {
	    _cairo_clip_init (clip);
	    clip = NULL;
	} else {
	    clip->all_clipped = other->all_clipped;
	    clip->path = _cairo_clip_path_reference (other->path);
	}
    } else {
	_cairo_clip_init (clip);
    }

    return clip;
}

void
_cairo_clip_reset (cairo_clip_t *clip)
{
    clip->all_clipped = FALSE;
    if (clip->path != NULL) {
	_cairo_clip_path_destroy (clip->path);
	clip->path = NULL;
    }
}

static cairo_status_t
_cairo_clip_intersect_path (cairo_clip_t       *clip,
			    const cairo_path_fixed_t *path,
			    cairo_fill_rule_t   fill_rule,
			    double              tolerance,
			    cairo_antialias_t   antialias)
{
    cairo_clip_path_t *clip_path;
    cairo_status_t status;
    cairo_rectangle_int_t extents;
    cairo_box_t box;
    cairo_bool_t is_box = FALSE;

    if (clip->path != NULL) {
	if (clip->path->fill_rule == fill_rule &&
	    (path->is_rectilinear ||
	     (tolerance == clip->path->tolerance &&
	      antialias == clip->path->antialias)) &&
	    _cairo_path_fixed_equal (&clip->path->path, path))
	{
	    return CAIRO_STATUS_SUCCESS;
	}
    }

    _cairo_path_fixed_approximate_clip_extents (path, &extents);
    if (extents.width == 0 || extents.height == 0) {
	_cairo_clip_set_all_clipped (clip);
	return CAIRO_STATUS_SUCCESS;
    }

    is_box = _cairo_path_fixed_is_box (path, &box);
    if (clip->path != NULL) {
	if (! _cairo_rectangle_intersect (&extents, &clip->path->extents)) {
	    _cairo_clip_set_all_clipped (clip);
	    return CAIRO_STATUS_SUCCESS;
	}

	/* does this clip wholly subsume the others? */
	if (is_box &&
	    box.p1.x <= _cairo_fixed_from_int (clip->path->extents.x) &&
	    box.p2.x >= _cairo_fixed_from_int (clip->path->extents.x + clip->path->extents.width) &&
	    box.p1.y <= _cairo_fixed_from_int (clip->path->extents.y) &&
	    box.p2.y >= _cairo_fixed_from_int (clip->path->extents.y + clip->path->extents.height))
	{
	    return CAIRO_STATUS_SUCCESS;
	}
    }

    clip_path = _cairo_clip_path_create (clip);
    if (unlikely (clip_path == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    status = _cairo_path_fixed_init_copy (&clip_path->path, path);
    if (unlikely (status)) {
	clip->path = clip->path->prev;
	_cairo_clip_path_destroy (clip_path);
	return status;
    }

    clip_path->extents = extents;
    clip_path->fill_rule = fill_rule;
    clip_path->tolerance = tolerance;
    clip_path->antialias = antialias;
    if (is_box)
	clip_path->flags |= CAIRO_CLIP_PATH_IS_BOX;

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_clip_clip (cairo_clip_t       *clip,
		  const cairo_path_fixed_t *path,
		  cairo_fill_rule_t   fill_rule,
		  double              tolerance,
		  cairo_antialias_t   antialias)
{
    if (clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    /* catch the empty clip path */
    if (_cairo_path_fixed_fill_is_empty (path)) {
	_cairo_clip_set_all_clipped (clip);
	return CAIRO_STATUS_SUCCESS;
    }

    return _cairo_clip_intersect_path (clip,
				       path, fill_rule, tolerance,
				       antialias);
}

cairo_status_t
_cairo_clip_rectangle (cairo_clip_t       *clip,
		       const cairo_rectangle_int_t *rectangle)
{
    if (clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    if (rectangle->width == 0 || rectangle->height == 0) {
	_cairo_clip_set_all_clipped (clip);
	return CAIRO_STATUS_SUCCESS;
    }

    /* if a smaller clip has already been set, ignore the new path */
    if (clip->path != NULL) {
	if (rectangle->x <= clip->path->extents.x &&
	    rectangle->y <= clip->path->extents.x &&
	    rectangle->x + rectangle->width >= clip->path->extents.x + clip->path->extents.width &&
	    rectangle->y + rectangle->height >= clip->path->extents.y + clip->path->extents.height)
	{
	    return CAIRO_STATUS_SUCCESS;
	}
    }

    return _cairo_clip_intersect_rectangle (clip, rectangle);
}

static cairo_status_t
_cairo_clip_path_reapply_clip_path_transform (cairo_clip_t      *clip,
					      cairo_clip_path_t *other_path,
					      const cairo_matrix_t *matrix)
{
    cairo_status_t status;
    cairo_clip_path_t *clip_path;
    cairo_bool_t is_empty;

    if (other_path->prev != NULL) {
        status = _cairo_clip_path_reapply_clip_path_transform (clip,
							       other_path->prev,
							       matrix);
	if (unlikely (status))
	    return status;
    }

    clip_path = _cairo_clip_path_create (clip);
    if (unlikely (clip_path == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    status = _cairo_path_fixed_init_copy (&clip_path->path,
					  &other_path->path);
    if (unlikely (status)) {
	_cairo_clip_path_destroy (clip_path);
	return status;
    }

    _cairo_path_fixed_transform (&clip_path->path, matrix);
    _cairo_path_fixed_approximate_clip_extents (&clip_path->path,
						&clip_path->extents);
    if (clip_path->prev != NULL) {
	is_empty = _cairo_rectangle_intersect (&clip_path->extents,
					       &clip_path->prev->extents);
    }

    clip_path->fill_rule = other_path->fill_rule;
    clip_path->tolerance = other_path->tolerance;
    clip_path->antialias = other_path->antialias;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_clip_path_reapply_clip_path_translate (cairo_clip_t      *clip,
					      cairo_clip_path_t *other_path,
					      int tx, int ty)
{
    cairo_status_t status;
    cairo_clip_path_t *clip_path;

    if (other_path->prev != NULL) {
        status = _cairo_clip_path_reapply_clip_path_translate (clip,
							       other_path->prev,
							       tx, ty);
	if (unlikely (status))
	    return status;
    }

    clip_path = _cairo_clip_path_create (clip);
    if (unlikely (clip_path == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    status = _cairo_path_fixed_init_copy (&clip_path->path,
					  &other_path->path);
    if (unlikely (status)) {
	_cairo_clip_path_destroy (clip_path);
	return status;
    }

    _cairo_path_fixed_translate (&clip_path->path,
				 _cairo_fixed_from_int (tx),
				 _cairo_fixed_from_int (ty));

    clip_path->fill_rule = other_path->fill_rule;
    clip_path->tolerance = other_path->tolerance;
    clip_path->antialias = other_path->antialias;

    clip_path->flags = other_path->flags;
    if (other_path->region != NULL) {
	clip_path->region = cairo_region_copy (other_path->region);
	cairo_region_translate (clip_path->region, tx, ty);
    }
    clip_path->surface = cairo_surface_reference (other_path->surface);

    clip_path->extents = other_path->extents;
    clip_path->extents.x += tx;
    clip_path->extents.y += ty;

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_clip_init_copy_transformed (cairo_clip_t    *clip,
				   cairo_clip_t    *other,
				   const cairo_matrix_t *matrix)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    int tx, ty;

    if (other == NULL) {
	_cairo_clip_init (clip);
	return CAIRO_STATUS_SUCCESS;
    }

    if (other->all_clipped) {
	_cairo_clip_init (clip);
	clip->all_clipped = TRUE;
	return CAIRO_STATUS_SUCCESS;
    }

    if (_cairo_matrix_is_identity (matrix)) {
	_cairo_clip_init_copy (clip, other);
	return CAIRO_STATUS_SUCCESS;
    }

    if (other->path != NULL) {
	_cairo_clip_init (clip);

	/* if we only need to translate, so we can reuse the caches... */
	/* XXX we still loose the benefit of constructs when the copy is
	 * deleted though. Indirect clip_paths?
	 */
	if (_cairo_matrix_is_integer_translation (matrix, &tx, &ty)) {
	    status = _cairo_clip_path_reapply_clip_path_translate (clip,
								   other->path,
								   tx, ty);
	} else {
	    status = _cairo_clip_path_reapply_clip_path_transform (clip,
								   other->path,
								   matrix);
	    if (clip->path->extents.width == 0 &&
		clip->path->extents.height == 0)
	    {
		_cairo_clip_set_all_clipped (clip);
	    }
	}
    }

    return status;
}

static cairo_status_t
_cairo_clip_apply_clip_path (cairo_clip_t *clip,
			     const cairo_clip_path_t *path)
{
    cairo_status_t status;

    if (path->prev != NULL)
	status = _cairo_clip_apply_clip_path (clip, path->prev);

    return _cairo_clip_intersect_path (clip,
				       &path->path,
				       path->fill_rule,
				       path->tolerance,
				       path->antialias);
}

cairo_status_t
_cairo_clip_apply_clip (cairo_clip_t *clip,
			const cairo_clip_t *other)
{
    cairo_status_t status;

    if (clip->all_clipped)
	return CAIRO_STATUS_SUCCESS;

    if (other->all_clipped) {
	_cairo_clip_set_all_clipped (clip);
	return CAIRO_STATUS_SUCCESS;
    }

    status = CAIRO_STATUS_SUCCESS;
    if (other->path != NULL)
	status = _cairo_clip_apply_clip_path (clip, other->path);

    return status;
}

static inline cairo_bool_t
_clip_paths_are_rectilinear (cairo_clip_path_t *clip_path)
{
    while (clip_path != NULL) {
	if (! clip_path->path.is_rectilinear)
	    return FALSE;

	clip_path = clip_path->prev;
    }

    return TRUE;
}

static cairo_int_status_t
_cairo_clip_path_to_region_geometric (cairo_clip_path_t *clip_path)
{
    cairo_traps_t traps;
    cairo_box_t stack_boxes[CAIRO_STACK_ARRAY_LENGTH (cairo_box_t)];
    cairo_box_t *boxes = stack_boxes;
    cairo_status_t status;
    int n;

    /* If we have nothing to intersect with this path, then it cannot
     * magically be reduced into a region.
     */
    if (clip_path->prev == NULL)
	goto UNSUPPORTED;

    /* Start simple... Intersect some boxes with an arbitrary path. */
    if (! clip_path->path.is_rectilinear)
	goto UNSUPPORTED;
    if (clip_path->prev->prev != NULL)
	goto UNSUPPORTED;

    _cairo_traps_init (&traps);
    _cairo_box_from_rectangle (&boxes[0], &clip_path->extents);
    _cairo_traps_limit (&traps, boxes, 1);

    status = _cairo_path_fixed_fill_rectilinear_to_traps (&clip_path->path,
							  clip_path->fill_rule,
							  &traps);
    if (unlikely (_cairo_status_is_error (status)))
	return status;
    if (status == CAIRO_INT_STATUS_UNSUPPORTED)
	goto UNSUPPORTED;

    if (traps.num_traps > ARRAY_LENGTH (stack_boxes)) {
	boxes = _cairo_malloc_ab (traps.num_traps, sizeof (cairo_box_t));
	if (unlikely (boxes == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    for (n = 0; n < traps.num_traps; n++) {
	boxes[n].p1.x = traps.traps[n].left.p1.x;
	boxes[n].p1.y = traps.traps[n].top;
	boxes[n].p2.x = traps.traps[n].right.p1.x;
	boxes[n].p2.y = traps.traps[n].bottom;
    }

    _cairo_traps_clear (&traps);
    _cairo_traps_limit (&traps, boxes, n);
    status = _cairo_path_fixed_fill_to_traps (&clip_path->prev->path,
					      clip_path->prev->fill_rule,
					      clip_path->prev->tolerance,
					      &traps);
    if (boxes != stack_boxes)
	free (boxes);

    if (unlikely (status))
	return status;

    status = _cairo_traps_extract_region (&traps, &clip_path->region);
    _cairo_traps_fini (&traps);

    if (status == CAIRO_INT_STATUS_UNSUPPORTED)
	goto UNSUPPORTED;
    if (unlikely (status))
	return status;

    clip_path->flags |= CAIRO_CLIP_PATH_HAS_REGION;
    return CAIRO_STATUS_SUCCESS;

UNSUPPORTED:
    clip_path->flags |= CAIRO_CLIP_PATH_REGION_IS_UNSUPPORTED;
    return CAIRO_INT_STATUS_UNSUPPORTED;
}

static cairo_int_status_t
_cairo_clip_path_to_region (cairo_clip_path_t *clip_path)
{
    cairo_int_status_t status;
    cairo_region_t *prev = NULL;

    if (clip_path->flags &
	(CAIRO_CLIP_PATH_HAS_REGION |
	 CAIRO_CLIP_PATH_REGION_IS_UNSUPPORTED))
    {
	return clip_path->flags & CAIRO_CLIP_PATH_REGION_IS_UNSUPPORTED ?
	    CAIRO_INT_STATUS_UNSUPPORTED :
	    CAIRO_STATUS_SUCCESS;
    }

    if (! clip_path->path.maybe_fill_region)
	return _cairo_clip_path_to_region_geometric (clip_path);

    /* first retrieve the region for our antecedents */
    if (clip_path->prev != NULL) {
	status = _cairo_clip_path_to_region (clip_path->prev);
	if (status) {
	    if (status == CAIRO_INT_STATUS_UNSUPPORTED)
		return _cairo_clip_path_to_region_geometric (clip_path);

	    return status;
	}

	prev = clip_path->prev->region;
    }

    /* now extract the region for ourselves */
    clip_path->region =
	_cairo_path_fixed_fill_rectilinear_to_region (&clip_path->path,
						      clip_path->fill_rule,
						      &clip_path->extents);
    assert (clip_path->region != NULL);

    status = clip_path->region->status;
    if (unlikely (status))
	return status;

    if (prev != NULL) {
	status = cairo_region_intersect (clip_path->region, prev);
	if (unlikely (status))
	    return status;
    }

    clip_path->flags |= CAIRO_CLIP_PATH_HAS_REGION;
    return CAIRO_STATUS_SUCCESS;
}

static inline int
pot (int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

/* XXX there is likely a faster method! ;-) */
static cairo_status_t
_region_clip_to_boxes (const cairo_region_t *region,
		       cairo_box_t **boxes,
		       int *num_boxes,
		       int *size_boxes)
{
    cairo_traps_t traps;
    cairo_status_t status;
    int n, num_rects;

    _cairo_traps_init (&traps);
    _cairo_traps_limit (&traps, *boxes, *num_boxes);
    traps.is_rectilinear = TRUE;
    traps.is_rectangular = TRUE;

    num_rects = cairo_region_num_rectangles (region);
    for (n = 0; n < num_rects; n++) {
	cairo_rectangle_int_t rect;
	cairo_point_t p1, p2;

	cairo_region_get_rectangle (region, n, &rect);

	p1.x = _cairo_fixed_from_int (rect.x);
	p1.y = _cairo_fixed_from_int (rect.y);
	p2.x = _cairo_fixed_from_int (rect.x + rect.width);
	p2.y = _cairo_fixed_from_int (rect.y + rect.height);

	status = _cairo_traps_tessellate_rectangle (&traps, &p1, &p2);
	if (unlikely (status))
	    goto CLEANUP;
    }

    status = _cairo_bentley_ottmann_tessellate_rectangular_traps (&traps, CAIRO_FILL_RULE_WINDING);
    if (unlikely (status))
	goto CLEANUP;

    n = *size_boxes;
    if (n < 0)
	n = -n;

    if (traps.num_traps > n) {
	cairo_box_t *new_boxes;

	new_boxes = _cairo_malloc_ab (traps.num_traps, sizeof (cairo_box_t));
	if (unlikely (new_boxes == NULL)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto CLEANUP;
	}

	if (*size_boxes > 0)
	    free (*boxes);

	*boxes = new_boxes;
	*size_boxes = traps.num_traps;
    }

    for (n = 0; n < traps.num_traps; n++) {
	(*boxes)[n].p1.x = traps.traps[n].left.p1.x;
	(*boxes)[n].p1.y = traps.traps[n].top;
	(*boxes)[n].p2.x = traps.traps[n].right.p1.x;
	(*boxes)[n].p2.y = traps.traps[n].bottom;
    }
    *num_boxes = n;

  CLEANUP:
    _cairo_traps_fini (&traps);

    return status;
}

static cairo_status_t
_rectilinear_clip_to_boxes (const cairo_path_fixed_t *path,
			    cairo_fill_rule_t fill_rule,
			    cairo_box_t **boxes,
			    int *num_boxes,
			    int *size_boxes)
{
    cairo_polygon_t polygon;
    cairo_traps_t traps;
    cairo_status_t status;

    _cairo_traps_init (&traps);
    _cairo_traps_limit (&traps, *boxes, *num_boxes);

    _cairo_polygon_init (&polygon);
    _cairo_polygon_limit (&polygon, *boxes, *num_boxes);

    status = _cairo_path_fixed_fill_rectilinear_to_traps (path,
							  fill_rule,
							  &traps);
    if (unlikely (_cairo_status_is_error (status)))
	goto CLEANUP;
    if (status == CAIRO_STATUS_SUCCESS)
	goto BOXES;

    /* tolerance will be ignored as the path is rectilinear */
    status = _cairo_path_fixed_fill_to_polygon (path, 0., &polygon);
    if (unlikely (status))
	goto CLEANUP;

    if (polygon.num_edges == 0) {
	*num_boxes = 0;
    } else {
	status = _cairo_bentley_ottmann_tessellate_rectilinear_polygon (&traps,
									&polygon,
									fill_rule);
	if (likely (status == CAIRO_STATUS_SUCCESS)) {
	    int i;

          BOXES:
	    i = *size_boxes;
	    if (i < 0)
		i = -i;

	    if (traps.num_traps > i) {
		cairo_box_t *new_boxes;
		int new_size;

		new_size = pot (traps.num_traps);
		new_boxes = _cairo_malloc_ab (new_size, sizeof (cairo_box_t));
		if (unlikely (new_boxes == NULL)) {
		    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
		    goto CLEANUP;
		}

		if (*size_boxes > 0)
		    free (*boxes);

		*boxes = new_boxes;
		*size_boxes = new_size;
	    }

	    for (i = 0; i < traps.num_traps; i++) {
		(*boxes)[i].p1.x = traps.traps[i].left.p1.x;
		(*boxes)[i].p1.y = traps.traps[i].top;
		(*boxes)[i].p2.x = traps.traps[i].right.p1.x;
		(*boxes)[i].p2.y = traps.traps[i].bottom;
	    }
	    *num_boxes = i;
	}
    }

  CLEANUP:
    _cairo_polygon_fini (&polygon);
    _cairo_traps_fini (&traps);

    return status;
}

static cairo_int_status_t
_cairo_clip_path_to_boxes (cairo_clip_path_t *clip_path,
			   cairo_box_t **boxes,
			   int *count)
{
    int size = -*count;
    int num_boxes = 0;
    cairo_status_t status;

    if (clip_path->region != NULL) {
	int num_rects, n;

	num_rects = cairo_region_num_rectangles (clip_path->region);
	if (num_rects > -size) {
	    cairo_box_t *new_boxes;

	    new_boxes = _cairo_malloc_ab (num_rects, sizeof (cairo_box_t));
	    if (unlikely (new_boxes == NULL))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	    *boxes = new_boxes;
	}

	for (n = 0; n < num_rects; n++) {
	    cairo_rectangle_int_t rect;

	    cairo_region_get_rectangle (clip_path->region, n, &rect);
	    (*boxes)[n].p1.x = _cairo_fixed_from_int (rect.x);
	    (*boxes)[n].p1.y = _cairo_fixed_from_int (rect.y);
	    (*boxes)[n].p2.x = _cairo_fixed_from_int (rect.x + rect.width);
	    (*boxes)[n].p2.y = _cairo_fixed_from_int (rect.y + rect.height);
	}

	*count = num_rects;
	return CAIRO_STATUS_SUCCESS;
    }

    /* keep it simple at first */
    if (! _clip_paths_are_rectilinear (clip_path))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    assert (-size >= 1);
    if (_cairo_path_fixed_is_box (&clip_path->path, *boxes)) {
	num_boxes = 1;
    } else {
	status = _rectilinear_clip_to_boxes (&clip_path->path,
					     clip_path->fill_rule,
					     boxes, &num_boxes, &size);
	if (unlikely (status))
	    return status;
    }

    while ((clip_path = clip_path->prev) != NULL) {
	cairo_box_t box;

	if (clip_path->region != NULL) {
	    status = _region_clip_to_boxes (clip_path->region,
					    boxes, &num_boxes, &size);
	    if (unlikely (status))
		return status;

	    break;
	} else if (_cairo_path_fixed_is_box (&clip_path->path, &box)) {
	    int i, j;

	    for (i = j = 0; i < num_boxes; i++) {
		if (j != i)
		    (*boxes)[j] = (*boxes)[i];

		if (box.p1.x > (*boxes)[j].p1.x)
		    (*boxes)[j].p1.x = box.p1.x;
		if (box.p2.x < (*boxes)[j].p2.x)
		    (*boxes)[j].p2.x = box.p2.x;

		if (box.p1.y > (*boxes)[j].p1.y)
		    (*boxes)[j].p1.y = box.p1.y;
		if (box.p2.y < (*boxes)[j].p2.y)
		    (*boxes)[j].p2.y = box.p2.y;

		j += (*boxes)[j].p2.x > (*boxes)[j].p1.x &&
		     (*boxes)[j].p2.y > (*boxes)[j].p1.y;
	    }

	    num_boxes = j;
	} else {
	    status = _rectilinear_clip_to_boxes (&clip_path->path,
						 clip_path->fill_rule,
						 boxes, &num_boxes, &size);
	    if (unlikely (status))
		return status;
	}

	if (num_boxes == 0)
	    break;
    }

    *count = num_boxes;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_combine_region (cairo_surface_t *surface,
		 const cairo_region_t *region,
		 const cairo_rectangle_int_t *extents)
{
    cairo_region_t clear_region;
    cairo_status_t status;

    _cairo_region_init_rectangle (&clear_region, extents);
    status = cairo_region_subtract (&clear_region, region);
    if (unlikely (status))
	return status;

    if (! cairo_region_is_empty (&clear_region)) {
	cairo_region_translate (&clear_region, -extents->x, -extents->y);
	status = _cairo_surface_fill_region (surface,
					     CAIRO_OPERATOR_CLEAR,
					     CAIRO_COLOR_TRANSPARENT,
					     &clear_region);
    }
    _cairo_region_fini (&clear_region);

    return status;
}

static cairo_surface_t *
_cairo_clip_path_get_surface (cairo_clip_path_t *clip_path,
			      cairo_surface_t *target)
{
    cairo_surface_t *surface;
    cairo_pattern_union_t pattern;
    cairo_status_t status;
    const cairo_rectangle_int_t *clip_extents = &clip_path->extents;
    cairo_clip_path_t *prev;
    cairo_bool_t need_translate;

    if (clip_path->surface != NULL &&
	clip_path->surface->backend == target->backend)
    {
	return cairo_surface_reference (clip_path->surface);
    }

    surface = _cairo_surface_create_similar_solid (target,
						   CAIRO_CONTENT_ALPHA,
						   clip_extents->width,
						   clip_extents->height,
						   CAIRO_COLOR_TRANSPARENT,
						   FALSE);
    if (surface == NULL) {
	if (clip_path->surface != NULL &&
	    clip_path->surface->backend == &_cairo_image_surface_backend)
	{
	    return cairo_surface_reference (clip_path->surface);
	}

	surface =
	    _cairo_image_surface_create_with_content (CAIRO_CONTENT_ALPHA,
						      clip_extents->width,
						      clip_extents->height);
    }
    if (unlikely (surface->status))
	return surface;

    _cairo_pattern_init_solid (&pattern.solid,
			       CAIRO_COLOR_WHITE,
			       CAIRO_CONTENT_COLOR);

    status = _cairo_clip_path_to_region (clip_path);
    if (unlikely (_cairo_status_is_error (status)))
	goto BAIL;

    need_translate = clip_extents->x | clip_extents->y;
    if (status == CAIRO_STATUS_SUCCESS) {
	if (need_translate) {
	    cairo_region_translate (clip_path->region,
				    -clip_extents->x, -clip_extents->y);
	}
	status = _cairo_surface_fill_region (surface,
					     CAIRO_OPERATOR_SOURCE,
					     CAIRO_COLOR_WHITE,
					     clip_path->region);
	if (need_translate) {
	    cairo_region_translate (clip_path->region,
				    clip_extents->x, clip_extents->y);
	}
	if (unlikely (status))
	    goto BAIL;

	goto DONE;
    } else {
	if (need_translate) {
	    _cairo_path_fixed_translate (&clip_path->path,
					 _cairo_fixed_from_int (-clip_extents->x),
					 _cairo_fixed_from_int (-clip_extents->y));
	}
	status = _cairo_surface_fill (surface,
				      CAIRO_OPERATOR_OVER,
				      &pattern.base,
				      &clip_path->path,
				      clip_path->fill_rule,
				      clip_path->tolerance,
				      clip_path->antialias,
				      NULL);
	if (need_translate) {
	    _cairo_path_fixed_translate (&clip_path->path,
					 _cairo_fixed_from_int (clip_extents->x),
					 _cairo_fixed_from_int (clip_extents->y));
	}

	if (unlikely (status))
	    goto BAIL;
    }

    prev = clip_path->prev;
  NEXT_PATH:
    if (prev != NULL) {
	status = _cairo_clip_path_to_region (prev);
	if (unlikely (_cairo_status_is_error (status)))
	    goto BAIL;

	if (status == CAIRO_STATUS_SUCCESS) {
	    status = _combine_region (surface, prev->region, clip_extents);
	    if (unlikely (status))
		goto BAIL;
	} else if (prev->flags & CAIRO_CLIP_PATH_IS_BOX) {
	    /* a simple box only affects the extents */
	} else if (prev->path.is_rectilinear) {
	    if (need_translate) {
		_cairo_path_fixed_translate (&prev->path,
					     _cairo_fixed_from_int (-clip_extents->x),
					     _cairo_fixed_from_int (-clip_extents->y));
	    }
	    status = _cairo_surface_fill (surface,
					  CAIRO_OPERATOR_IN,
					  &pattern.base,
					  &prev->path,
					  prev->fill_rule,
					  prev->tolerance,
					  prev->antialias,
					  NULL);
	    if (need_translate) {
		_cairo_path_fixed_translate (&prev->path,
					     _cairo_fixed_from_int (clip_extents->x),
					     _cairo_fixed_from_int (clip_extents->y));
	    }

	    if (unlikely (status))
		goto BAIL;

	    prev = prev->prev;
	    goto NEXT_PATH;
	} else {
	    cairo_surface_t *prev_surface;

	    prev_surface = _cairo_clip_path_get_surface (prev, target);
	    _cairo_pattern_init_for_surface (&pattern.surface, prev_surface);
	    cairo_surface_destroy (prev_surface);

	    cairo_matrix_init_translate (&pattern.base.matrix,
					 -prev->extents.x + clip_extents->x,
					 -prev->extents.y + clip_extents->y);
	    status = _cairo_surface_paint (surface,
					   CAIRO_OPERATOR_IN,
					   &pattern.base,
					   NULL);
	    _cairo_pattern_fini (&pattern.base);

	    if (unlikely (status))
		goto BAIL;
	}
    }

  DONE:
    cairo_surface_destroy (clip_path->surface);
    return clip_path->surface = cairo_surface_reference (surface);

  BAIL:
    cairo_surface_destroy (surface);
    return _cairo_surface_create_in_error (status);
}

void
_cairo_debug_print_clip (FILE *stream, cairo_clip_t *clip)
{
    cairo_clip_path_t *clip_path;

    if (clip == NULL) {
	fprintf (stream, "no clip\n");
	return;
    }

    if (clip->all_clipped) {
	fprintf (stream, "clip: all-clipped\n");
	return;
    }

    if (clip->path == NULL) {
	fprintf (stream, "clip: empty\n");
	return;
    }

    fprintf (stream, "clip:\n");

    clip_path = clip->path;
    do {
	fprintf (stream, "path: has region? %s, has surface? %s: ",
		 clip_path->region == NULL ? "no" : "yes",
		 clip_path->surface == NULL ? "no" : "yes");
	_cairo_debug_print_path (stream, &clip_path->path);
	fprintf (stream, "\n");
    } while ((clip_path = clip_path->prev) != NULL);
}

cairo_surface_t *
_cairo_clip_get_surface (cairo_clip_t *clip, cairo_surface_t *target)
{
    assert (clip->path != NULL);
    return _cairo_clip_path_get_surface (clip->path, target);
}

cairo_status_t
_cairo_clip_combine_with_surface (cairo_clip_t *clip,
				  cairo_surface_t *dst,
				  const cairo_rectangle_int_t *extents)
{
    cairo_pattern_union_t pattern;
    cairo_clip_path_t *clip_path = clip->path;
    cairo_bool_t need_translate;
    cairo_status_t status;

    assert (clip_path != NULL);

    if (clip_path->surface != NULL &&
	clip_path->surface->backend == dst->backend)
    {
	_cairo_pattern_init_for_surface (&pattern.surface,
					 clip_path->surface);
	cairo_matrix_init_translate (&pattern.base.matrix,
				     extents->x - clip_path->extents.x,
				     extents->y - clip_path->extents.y);
	status = _cairo_surface_paint (dst,
				       CAIRO_OPERATOR_IN,
				       &pattern.base,
				       NULL);

	_cairo_pattern_fini (&pattern.base);

	return status;
    }

    _cairo_pattern_init_solid (&pattern.solid,
			       CAIRO_COLOR_WHITE,
			       CAIRO_CONTENT_COLOR);

    need_translate = extents->x | extents->y;
    do {
	status = _cairo_clip_path_to_region (clip_path);
	if (unlikely (_cairo_status_is_error (status)))
	    return status;

	if (status == CAIRO_STATUS_SUCCESS)
	    return _combine_region (dst, clip_path->region, extents);

	if (clip_path->surface != NULL &&
	    clip_path->surface->backend == dst->backend)
	{
	    _cairo_pattern_init_for_surface (&pattern.surface,
					     clip_path->surface);
	    cairo_matrix_init_translate (&pattern.base.matrix,
					 extents->x - clip_path->extents.x,
					 extents->y - clip_path->extents.y);
	    status = _cairo_surface_paint (dst,
					   CAIRO_OPERATOR_IN,
					   &pattern.base,
					   NULL);

	    _cairo_pattern_fini (&pattern.base);

	    return status;
	}

	if (clip_path->flags & CAIRO_CLIP_PATH_IS_BOX) {
	    cairo_region_t clip_region;

	    _cairo_region_init_rectangle (&clip_region, &clip_path->extents);
	    status = _combine_region (dst, &clip_region, extents);
	} else {
	    if (need_translate) {
		_cairo_path_fixed_translate (&clip_path->path,
					     _cairo_fixed_from_int (-extents->x),
					     _cairo_fixed_from_int (-extents->y));
	    }
	    status = _cairo_surface_fill (dst,
					  CAIRO_OPERATOR_IN,
					  &pattern.base,
					  &clip_path->path,
					  clip_path->fill_rule,
					  clip_path->tolerance,
					  clip_path->antialias,
					  NULL);
	    if (need_translate) {
		_cairo_path_fixed_translate (&clip_path->path,
					     _cairo_fixed_from_int (extents->x),
					     _cairo_fixed_from_int (extents->y));
	    }
	}

	if (unlikely (status))
	    return status;
    } while ((clip_path = clip_path->prev) != NULL);

    return CAIRO_STATUS_SUCCESS;
}

const cairo_rectangle_int_t *
_cairo_clip_get_extents (const cairo_clip_t *clip)
{
    if (clip->path == NULL)
	return NULL;

    return &clip->path->extents;
}

void
_cairo_clip_drop_cache (cairo_clip_t  *clip)
{
    cairo_clip_path_t *clip_path;

    if (clip->path == NULL)
	return;

    clip_path = clip->path;
    do {
	if (clip_path->region != NULL) {
	    cairo_region_destroy (clip_path->region);
	    clip_path->region = NULL;
	}

	if (clip_path->surface != NULL) {
	    cairo_surface_destroy (clip_path->surface);
	    clip_path->surface = NULL;
	}

	clip_path->flags &= ~CAIRO_CLIP_PATH_HAS_REGION;
    } while ((clip_path = clip_path->prev) != NULL);
}

const cairo_rectangle_list_t _cairo_rectangles_nil =
  { CAIRO_STATUS_NO_MEMORY, NULL, 0 };
static const cairo_rectangle_list_t _cairo_rectangles_not_representable =
  { CAIRO_STATUS_CLIP_NOT_REPRESENTABLE, NULL, 0 };

static cairo_bool_t
_cairo_clip_int_rect_to_user (cairo_gstate_t *gstate,
			      cairo_rectangle_int_t *clip_rect,
			      cairo_rectangle_t *user_rect)
{
    cairo_bool_t is_tight;

    double x1 = clip_rect->x;
    double y1 = clip_rect->y;
    double x2 = clip_rect->x + (int) clip_rect->width;
    double y2 = clip_rect->y + (int) clip_rect->height;

    _cairo_gstate_backend_to_user_rectangle (gstate,
					     &x1, &y1, &x2, &y2,
					     &is_tight);

    user_rect->x = x1;
    user_rect->y = y1;
    user_rect->width  = x2 - x1;
    user_rect->height = y2 - y1;

    return is_tight;
}

cairo_int_status_t
_cairo_clip_get_region (cairo_clip_t *clip,
			cairo_region_t **region)
{
    cairo_int_status_t status;

    if (clip->all_clipped)
	goto CLIPPED;

    assert (clip->path != NULL);

    status = _cairo_clip_path_to_region (clip->path);
    if (status)
	return status;

    if (cairo_region_is_empty (clip->path->region)) {
	_cairo_clip_set_all_clipped (clip);
	goto CLIPPED;
    }

    if (region)
	*region = clip->path->region;
    return CAIRO_STATUS_SUCCESS;

  CLIPPED:
    if (region)
	*region = NULL;
    return CAIRO_INT_STATUS_NOTHING_TO_DO;
}

cairo_int_status_t
_cairo_clip_get_boxes (cairo_clip_t *clip,
		       cairo_box_t **boxes,
		       int *count)
{
    cairo_int_status_t status;

    if (clip->all_clipped)
	return CAIRO_INT_STATUS_NOTHING_TO_DO;

    assert (clip->path != NULL);

    status = _cairo_clip_path_to_boxes (clip->path, boxes, count);
    if (status)
	return status;

    if (*count == 0) {
	_cairo_clip_set_all_clipped (clip);
	return CAIRO_INT_STATUS_NOTHING_TO_DO;
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_rectangle_list_t *
_cairo_rectangle_list_create_in_error (cairo_status_t status)
{
    cairo_rectangle_list_t *list;

    if (status == CAIRO_STATUS_NO_MEMORY)
	return (cairo_rectangle_list_t*) &_cairo_rectangles_nil;
    if (status == CAIRO_STATUS_CLIP_NOT_REPRESENTABLE)
	return (cairo_rectangle_list_t*) &_cairo_rectangles_not_representable;

    list = malloc (sizeof (*list));
    if (unlikely (list == NULL)) {
	_cairo_error_throw (status);
	return (cairo_rectangle_list_t*) &_cairo_rectangles_nil;
    }

    list->status = status;
    list->rectangles = NULL;
    list->num_rectangles = 0;

    return list;
}

cairo_rectangle_list_t *
_cairo_clip_copy_rectangle_list (cairo_clip_t *clip, cairo_gstate_t *gstate)
{
#define ERROR_LIST(S) _cairo_rectangle_list_create_in_error (_cairo_error (S));

    cairo_rectangle_list_t *list;
    cairo_rectangle_t *rectangles = NULL;
    cairo_region_t *region = NULL;
    cairo_int_status_t status;
    int n_rects = 0;
    int i;

    if (clip != NULL && clip->path != NULL) {
	status = _cairo_clip_get_region (clip, &region);
	if (status == CAIRO_INT_STATUS_NOTHING_TO_DO) {
	    goto DONE;
	} else if (status == CAIRO_INT_STATUS_UNSUPPORTED) {
	    return ERROR_LIST (CAIRO_STATUS_CLIP_NOT_REPRESENTABLE)
	} else if (unlikely (status)) {
	    return ERROR_LIST (status);
	}
    }

    if (region != NULL) {
	n_rects = cairo_region_num_rectangles (region);
	if (n_rects) {
	    rectangles = _cairo_malloc_ab (n_rects, sizeof (cairo_rectangle_t));
	    if (unlikely (rectangles == NULL)) {
		return ERROR_LIST (CAIRO_STATUS_NO_MEMORY);
	    }

	    for (i = 0; i < n_rects; ++i) {
		cairo_rectangle_int_t clip_rect;

		cairo_region_get_rectangle (region, i, &clip_rect);

		if (! _cairo_clip_int_rect_to_user (gstate,
						    &clip_rect,
						    &rectangles[i]))
		{
		    free (rectangles);
		    return ERROR_LIST (CAIRO_STATUS_CLIP_NOT_REPRESENTABLE);
		}
	    }
	}
    } else {
        cairo_rectangle_int_t extents;

	if (! _cairo_surface_get_extents (_cairo_gstate_get_target (gstate),
					  &extents))
	{
	    /* unbounded surface -> unclipped */
	    goto DONE;
	}

	n_rects = 1;
	rectangles = malloc(sizeof (cairo_rectangle_t));
	if (unlikely (rectangles == NULL))
	    return ERROR_LIST (CAIRO_STATUS_NO_MEMORY);

	if (! _cairo_clip_int_rect_to_user (gstate, &extents, rectangles)) {
	    free (rectangles);
	    return ERROR_LIST (CAIRO_STATUS_CLIP_NOT_REPRESENTABLE);
	}
    }

 DONE:
    list = malloc (sizeof (cairo_rectangle_list_t));
    if (unlikely (list == NULL)) {
        free (rectangles);
	return ERROR_LIST (CAIRO_STATUS_NO_MEMORY);
    }

    list->status = CAIRO_STATUS_SUCCESS;
    list->rectangles = rectangles;
    list->num_rectangles = n_rects;
    return list;

#undef ERROR_LIST
}

/**
 * cairo_rectangle_list_destroy:
 * @rectangle_list: a rectangle list, as obtained from cairo_copy_clip_rectangles()
 *
 * Unconditionally frees @rectangle_list and all associated
 * references. After this call, the @rectangle_list pointer must not
 * be dereferenced.
 *
 * Since: 1.4
 **/
void
cairo_rectangle_list_destroy (cairo_rectangle_list_t *rectangle_list)
{
    if (rectangle_list == NULL || rectangle_list == &_cairo_rectangles_nil ||
        rectangle_list == &_cairo_rectangles_not_representable)
        return;

    free (rectangle_list->rectangles);
    free (rectangle_list);
}

void
_cairo_clip_reset_static_data (void)
{
    _freed_pool_reset (&clip_path_pool);
}
