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

#ifndef __MC_BITMAP_EFFECT__
#define __MC_BITMAP_EFFECT__

struct MCBitmapEffects;
typedef MCBitmapEffects *MCBitmapEffectsRef;

void MCBitmapEffectsInitialize(MCBitmapEffectsRef& r_dst);
void MCBitmapEffectsFinalize(MCBitmapEffectsRef dst);

void MCBitmapEffectsClear(MCBitmapEffectsRef& x_dst);
bool MCBitmapEffectsAssign(MCBitmapEffectsRef& x_dst, MCBitmapEffectsRef src);

// This method returns 'true' if the bitmap effects only affect the interior
// of the shape. (i.e. contain no drop-shadow or outerglow effects).
bool MCBitmapEffectsIsInteriorOnly(MCBitmapEffectsRef effects);

bool MCBitmapEffectsScale(MCBitmapEffectsRef& x_dst, int32_t scale);

Exec_stat MCBitmapEffectsSetProperties(MCBitmapEffectsRef& self, Properties which_type, MCExecPoint& ep, MCNameRef prop, Boolean& r_dirty);
Exec_stat MCBitmapEffectsGetProperties(MCBitmapEffectsRef& self, Properties which_type, MCExecPoint& ep, MCNameRef prop);

uint32_t MCBitmapEffectsWeigh(MCBitmapEffectsRef self);
IO_stat MCBitmapEffectsPickle(MCBitmapEffectsRef self, MCObjectOutputStream& p_stream);
IO_stat MCBitmapEffectsUnpickle(MCBitmapEffectsRef& self, MCObjectInputStream& p_stream);

// This method computes the required clipping region required in the source
// layer to correctly render the 'clip' in the destination assuming the shape
// being affects has rect 'shape'.
void MCBitmapEffectsComputeClip(MCBitmapEffectsRef self, const MCRectangle& shape, const MCRectangle& clip, MCRectangle& r_layer_clip);

// This method computes the region covered by the given shape taking into
// account the effects.
void MCBitmapEffectsComputeBounds(MCBitmapEffectsRef self, const MCRectangle& shape, MCRectangle& r_bounds);

// This record contains details about a layer which the render method uses to
// do the drawing.
struct MCBitmapEffectLayer
{
	MCRectangle bounds;
	uint32_t stride;
	void *bits;
	bool has_alpha;
};

// This method renders the source layer to the destination with the specified
// bitmap effects.
void MCBitmapEffectsRender(MCBitmapEffectsRef self, const MCRectangle& shape, MCBitmapEffectLayer& dst, MCBitmapEffectLayer& src);

#endif
