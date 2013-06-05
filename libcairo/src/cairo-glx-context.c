/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2009 Eric Anholt
 * Copyright © 2009 Chris Wilson
 * Copyright © 2005 Red Hat, Inc
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Carl Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "cairoint.h"

#include "cairo-gl-private.h"

#include <X11/Xutil.h>

/* XXX needs hooking into XCloseDisplay() */

typedef struct _cairo_glx_context {
    cairo_gl_context_t base;

    Display *display;
    Window dummy_window;
    GLXContext context;
} cairo_glx_context_t;

typedef struct _cairo_glx_surface {
    cairo_gl_surface_t base;

    Window win;
} cairo_glx_surface_t;

static void
_glx_make_current (void *abstract_ctx,
	           cairo_gl_surface_t *abstract_surface)
{
    cairo_glx_context_t *ctx = abstract_ctx;
    cairo_glx_surface_t *surface = (cairo_glx_surface_t *) abstract_surface;

    /* Set the window as the target of our context. */
    glXMakeCurrent (ctx->display, surface->win, ctx->context);
}

static void
_glx_swap_buffers (void *abstract_ctx,
		   cairo_gl_surface_t *abstract_surface)
{
    cairo_glx_context_t *ctx = abstract_ctx;
    cairo_glx_surface_t *surface = (cairo_glx_surface_t *) abstract_surface;

    glXSwapBuffers (ctx->display, surface->win);
}

static void
_glx_destroy (void *abstract_ctx)
{
    cairo_glx_context_t *ctx = abstract_ctx;

    if (ctx->dummy_window != None)
	XDestroyWindow (ctx->display, ctx->dummy_window);
}

static cairo_status_t
_glx_dummy_ctx (Display *dpy, GLXContext gl_ctx, Window *dummy)
{
    int attr[3] = { GLX_FBCONFIG_ID, 0, None };
    GLXFBConfig *config;
    XVisualInfo *vi;
    Colormap cmap;
    XSetWindowAttributes swa;
    Window win = None;
    int cnt;

    /* Create a dummy window created for the target GLX context that we can
     * use to query the available GL/GLX extensions.
     */
    glXQueryContext (dpy, gl_ctx, GLX_FBCONFIG_ID, &attr[1]);

    cnt = 0;
    config = glXChooseFBConfig (dpy, DefaultScreen (dpy), attr, &cnt);
    if (unlikely (cnt == 0))
	return _cairo_error (CAIRO_STATUS_INVALID_FORMAT);

    vi = glXGetVisualFromFBConfig (dpy, config[0]);
    XFree (config);

    if (unlikely (vi == NULL))
	return _cairo_error (CAIRO_STATUS_INVALID_FORMAT);

    cmap = XCreateColormap (dpy,
			    RootWindow (dpy, vi->screen),
			    vi->visual,
			    AllocNone);
    swa.colormap = cmap;
    swa.border_pixel = 0;
    win = XCreateWindow (dpy, RootWindow (dpy, vi->screen),
			 -1, -1, 1, 1, 0,
			 vi->depth,
			 InputOutput,
			 vi->visual,
			 CWBorderPixel | CWColormap, &swa);
    XFreeColormap (dpy, cmap);
    XFree (vi);

    XFlush (dpy);
    if (unlikely (! glXMakeCurrent (dpy, win, gl_ctx))) {
	XDestroyWindow (dpy, win);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    *dummy = win;
    return CAIRO_STATUS_SUCCESS;
}

cairo_gl_context_t *
cairo_glx_context_create (Display *dpy, GLXContext gl_ctx)
{
    cairo_glx_context_t *ctx;
    cairo_status_t status;
    Window dummy = None;

    status = _glx_dummy_ctx (dpy, gl_ctx, &dummy);
    if (unlikely (status))
	return _cairo_gl_context_create_in_error (status);

    ctx = calloc (1, sizeof (cairo_glx_context_t));
    if (unlikely (ctx == NULL))
	return _cairo_gl_context_create_in_error (CAIRO_STATUS_NO_MEMORY);

    ctx->display = dpy;
    ctx->dummy_window = dummy;
    ctx->context = gl_ctx;

    ctx->base.make_current = _glx_make_current;
    ctx->base.swap_buffers = _glx_swap_buffers;
    ctx->base.destroy = _glx_destroy;

    status = _cairo_gl_context_init (&ctx->base);
    if (unlikely (status)) {
	free (ctx);
	return _cairo_gl_context_create_in_error (status);
    }

    return &ctx->base;
}

cairo_surface_t *
cairo_gl_surface_create_for_window (cairo_gl_context_t   *ctx,
				    Window                win,
				    int                   width,
				    int                   height)
{
    cairo_glx_surface_t *surface;

    if (unlikely (ctx->status))
	return _cairo_surface_create_in_error (ctx->status);

    surface = calloc (1, sizeof (cairo_glx_surface_t));
    if (unlikely (surface == NULL))
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));

    _cairo_gl_surface_init (ctx, &surface->base,
			    CAIRO_CONTENT_COLOR_ALPHA, width, height);
    surface->win = win;

    return &surface->base.base;
}
