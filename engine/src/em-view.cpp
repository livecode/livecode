/*                                                                     -*-c++-*-

Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "em-view.h"

#include "parsedef.h"
#include "util.h"

#include <emscripten.h>
#include <SDL.h>

/* Performs initial setup of the Emscripten view, using SDL. */
bool
MCEmscriptenViewInitialize()
{
	/* Make sure SDL video subsystem has been initialised at some
	 * point. */
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
	{
		if (0 != SDL_InitSubSystem(SDL_INIT_VIDEO))
		{
			return false;
		}
	}

	/* Set the SDL video mode */
	int t_canvas_width, t_canvas_height, t_is_fullscreen;
	emscripten_get_canvas_size(&t_canvas_width,
	                           &t_canvas_height,
	                           &t_is_fullscreen);

	SDL_Surface *t_surface = SDL_SetVideoMode(t_canvas_width,
	                                          t_canvas_height,
	                                          32, /* bits per pixel */
	                                          SDL_SWSURFACE);

	return (nil != t_surface);
}

/* Return the size of the Emscripten view as a rectangle. */
MCRectangle
MCEmscriptenViewGetBounds()
{
	int t_canvas_width, t_canvas_height, t_is_fullscreen;
	emscripten_get_canvas_size(&t_canvas_width,
	                           &t_canvas_height,
	                           &t_is_fullscreen);

	MCRectangle t_result;
	MCU_set_rect(t_result, 0, 0, t_canvas_width, t_canvas_height);

	return t_result;
}

