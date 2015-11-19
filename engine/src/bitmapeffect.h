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

#ifndef __MC_BITMAP_EFFECT__
#define __MC_BITMAP_EFFECT__

struct MCBitmapEffects;
struct MCExecValue;
typedef MCBitmapEffects *MCBitmapEffectsRef;

void MCBitmapEffectsInitialize(MCBitmapEffectsRef& r_dst);
void MCBitmapEffectsFinalize(MCBitmapEffectsRef dst);

void MCBitmapEffectsClear(MCBitmapEffectsRef& x_dst);
bool MCBitmapEffectsAssign(MCBitmapEffectsRef& x_dst, MCBitmapEffectsRef src);

// This method returns 'true' if the bitmap effects only affect the interior
// of the shape. (i.e. contain no drop-shadow or outerglow effects).
bool MCBitmapEffectsIsInteriorOnly(MCBitmapEffectsRef effects);

bool MCBitmapEffectsScale(MCBitmapEffectsRef& x_dst, int32_t scale);

bool MCBitmapEffectsGetProperty(MCExecContext& ctxt, MCBitmapEffectsRef& self, MCNameRef p_index, Properties which, MCExecValue& r_color);
bool MCBitmapEffectsSetProperty(MCExecContext& ctxt, MCBitmapEffectsRef& self, MCNameRef p_index, Properties which, MCExecValue p_color, bool& r_dirty);

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

////////////////////////////////////////////////////////////////////////////////
// MM-2013-08-16: [[ RefactorGraphics ]] Refactored effect types from bitmapeffect.cpp. Used by graphicscontext.

// The list of all possible bitmap effect types.
enum MCBitmapEffectType
{
	kMCBitmapEffectTypeDropShadow,
	kMCBitmapEffectTypeInnerShadow,
	kMCBitmapEffectTypeOuterGlow,
	kMCBitmapEffectTypeInnerGlow,
	kMCBitmapEffectTypeColorOverlay,
	// kMCBitmapEffectTypeBevelAndEmboss, // NOT YET SUPPORTED
	// kMCBitmapEffectTypeGradientOverlay, // NOT YET SUPPORTED
	// kMCBitmapEffectTypePatternOverlay, // NOT YET SUPPORTED
	// kMCBitmapEffectTypeStroke, // NOT YET SUPPORTED
	kMCBitmapEffectType_Count
};

// The list of all possible bitmap effect properties.
enum MCBitmapEffectProperty
{
	kMCBitmapEffectPropertyColor, // ALL
	kMCBitmapEffectPropertyOpacity, // ALL
	kMCBitmapEffectPropertyBlendMode, // ALL
	kMCBitmapEffectPropertyFilter, // BLUR EFFECTS
	kMCBitmapEffectPropertySize, // BLUR EFFECTS
	kMCBitmapEffectPropertySpread, // BLUR EFFECTS
	kMCBitmapEffectPropertyRange, // GLOW EFFECTS
	kMCBitmapEffectPropertyKnockOut, // DROP SHADOW EFFECT
	kMCBitmapEffectPropertyDistance, // SHADOW EFFECTS
	kMCBitmapEffectPropertyAngle, // SHADOW EFFECTS
	kMCBitmapEffectPropertySource, // INNER GLOW EFFECT
};

// The list of blend modes applicable to the bitmap effects.
enum MCBitmapEffectBlendMode
{
	kMCBitmapEffectBlendModeNormal,
	kMCBitmapEffectBlendModeMultiply,
	kMCBitmapEffectBlendModeScreen,
	kMCBitmapEffectBlendModeOverlay,
	kMCBitmapEffectBlendModeDarken,
	kMCBitmapEffectBlendModeLighten,
	kMCBitmapEffectBlendModeColorDodge,
	kMCBitmapEffectBlendModeColorBurn,
	kMCBitmapEffectBlendModeHardLight,
	kMCBitmapEffectBlendModeSoftLight,
	kMCBitmapEffectBlendModeDifference,
	kMCBitmapEffectBlendModeExclusion,
	kMCBitmapEffectBlendModeHue,
	kMCBitmapEffectBlendModeSaturation,
	kMCBitmapEffectBlendModeColor,
	kMCBitmapEffectBlendModeLuminosity
};

enum MCBitmapEffectSource
{
	kMCBitmapEffectSourceEdge,
	kMCBitmapEffectSourceCenter
};

enum
{
	kMCBitmapEffectTypeDropShadowBit = 1 << kMCBitmapEffectTypeDropShadow,
	kMCBitmapEffectTypeInnerShadowBit = 1 << kMCBitmapEffectTypeInnerShadow,
	kMCBitmapEffectTypeOuterGlowBit = 1 << kMCBitmapEffectTypeOuterGlow,
	kMCBitmapEffectTypeInnerGlowBit = 1 << kMCBitmapEffectTypeInnerGlow,
	kMCBitmapEffectTypeColorOverlayBit = 1 << kMCBitmapEffectTypeColorOverlay,
	
	kMCBitmapEffectTypeAllShadowBits = kMCBitmapEffectTypeDropShadowBit | kMCBitmapEffectTypeInnerShadowBit,
	kMCBitmapEffectTypeAllGlowBits = kMCBitmapEffectTypeOuterGlowBit | kMCBitmapEffectTypeInnerGlowBit,
	kMCBitmapEffectTypeAllBlurBits = kMCBitmapEffectTypeAllShadowBits | kMCBitmapEffectTypeAllGlowBits,
	kMCBitmapEffectTypeAllBits = (1 << (kMCBitmapEffectType_Count + 1)) - 1
};

////////////////////////////////////////////////////////////////////////////////
// MM-2013-08-16: [[ RefactorGraphics ]] Refactored effect structs from bitmapeffect.cpp. Used by graphicscontext.

// Fields common to all effects
struct MCLayerEffect
{
	uint4 color;
	unsigned blend_mode : 4;
	unsigned : 28;
};
#define kMCLayerEffectPickleSize 5

// Fields common to all blur-based effects (shadow and glow)
struct MCBlurEffect
{
	uint4 color;
	unsigned blend_mode : 4;
	unsigned filter : 3;
	unsigned size : 8;
	unsigned spread : 8;
	unsigned : 8;
};
#define kMCBlurEffectPickleSize 8

// Fields common to all shadow effects
struct MCShadowEffect
{
	uint4 color;
	unsigned blend_mode : 4;
	unsigned filter : 3;
	unsigned size : 8; // blurs only
	unsigned spread : 8; // blurs only
	unsigned angle : 9; // shadows only
	unsigned distance : 15; // shadows only
	bool knockout: 1; // drop shadow only
	unsigned : 16;
};
#define kMCShadowEffectPickleSize 10

// Fields common to all glow effects
struct MCGlowEffect
{
	uint4 color;
	unsigned blend_mode : 4;
	unsigned filter : 3;
	unsigned size : 8; // blurs only
	unsigned spread : 8; // blurs only
	unsigned range : 8; // glows only
	unsigned source : 1; // innerGlow only
};
#define kMCGlowEffectPickleSize 8

union MCBitmapEffect
{
	MCLayerEffect layer;
	MCBlurEffect blur;
	MCShadowEffect shadow;
	MCGlowEffect glow;
};

// The opaque MCBitmapEffects object that gets managed by the public API. For
// now we'll just make this a flat array, although it could be improved to be a
// bit-field compressed one (like the color arrays on objects).
struct MCBitmapEffects
{
	// The mask determines which of the effects is present.
	uint32_t mask;
	
	// The effect fields.
	MCBitmapEffect effects[kMCBitmapEffectType_Count];
};

////////////////////////////////////////////////////////////////////////////////

#endif
