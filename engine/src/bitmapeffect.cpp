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


#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "mcerror.h"
#include "globals.h"
#include "objectstream.h"
#include "packed.h"

#include "bitmapeffect.h"
#include "bitmapeffectblur.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////
//
// Bitmap effects act on the alpha channel of a layer, producing several layers
// that are composited together to form the final result. Motivation for these
// effects comes from PhotoShop and the syntax/features available at the
// language level reflects this. At present we provide for effects that are
// based on a simple alpha-mask blur - further research is needed to implement
// other effects such as Bevel and Emboss.
//
// Each blur effect involves the following steps:
//   1) Translate the alpha mask of the layer
//   2) Blur the new alpha mask
//   2) Attenuate the new alpha mask by the original alpha mask
//   3) Combine the source color and the new alpha mask
//   4) Composite the resulting src with the dst using a given blend mode.
//

////////////////////////////////////////////////////////////////////////////////

// This record represents a mapping from token to property.
struct MCBitmapEffectPropertyMap
{
	// The string key for the property
	const char *token;
	// The corresponding property value
	MCBitmapEffectProperty value;
	// The bit-mask determining which effect types it is applicable to
	uint32_t mask;
};

// The list of properties applicable to the various bitmap effect arrays.
static MCBitmapEffectPropertyMap s_bitmap_effect_properties[] =
{
	{ "color", kMCBitmapEffectPropertyColor, kMCBitmapEffectTypeAllBits },
	{ "opacity", kMCBitmapEffectPropertyOpacity, kMCBitmapEffectTypeAllBits },
	{ "blendmode", kMCBitmapEffectPropertyBlendMode, kMCBitmapEffectTypeAllBits },
	{ "filter", kMCBitmapEffectPropertyFilter, kMCBitmapEffectTypeAllBlurBits },
	{ "size", kMCBitmapEffectPropertySize, kMCBitmapEffectTypeAllBlurBits },
	{ "spread", kMCBitmapEffectPropertySpread, kMCBitmapEffectTypeAllBlurBits },
	{ "range", kMCBitmapEffectPropertyRange, kMCBitmapEffectTypeAllGlowBits },
	{ "knockout", kMCBitmapEffectPropertyKnockOut, kMCBitmapEffectTypeDropShadowBit },
	{ "distance", kMCBitmapEffectPropertyDistance, kMCBitmapEffectTypeAllShadowBits },
	{ "angle", kMCBitmapEffectPropertyAngle, kMCBitmapEffectTypeAllShadowBits },
	{ "source", kMCBitmapEffectPropertySource, kMCBitmapEffectTypeInnerGlowBit }
};

// Search the properties array for the given property applicable to the given type.
static Exec_stat MCBitmapEffectLookupProperty(MCBitmapEffectType p_type, MCNameRef p_token, MCBitmapEffectProperty& r_prop)
{
	// If a lookup error occurs, we store it in this var so we can add it to the
	// error list.
	Exec_errors t_error;
	t_error = EE_BITMAPEFFECT_BADKEY;

	for(uint4 i = 0; i < ELEMENTS(s_bitmap_effect_properties); i++)
	{
		// Check to see if we've found a match.
		if (MCStringIsEqualToCString(MCNameGetString(p_token), s_bitmap_effect_properties[i] . token, kMCCompareCaseless))
		{
			// Check to see if its applicable to this type.
			if ((s_bitmap_effect_properties[i] . mask & (1 << p_type)) != 0)
			{
				r_prop = s_bitmap_effect_properties[i] . value;
				return ES_NORMAL;
			}

			// The key wasn't applicable to this type.
			t_error = EE_BITMAPEFFECT_BADKEYFORTYPE;
			break;
		}
	}

	// Add the error to the error list.
	MCeerror -> add(t_error, 0, 0, p_token);
	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

void MCBitmapEffectsInitialize(MCBitmapEffectsRef& self)
{
	// Initialize ourselves to empty.
	self = NULL;
}

void MCBitmapEffectsFinalize(MCBitmapEffectsRef self)
{
	// If we are the empty list, there is nothing to do.
	if (self == NULL)
		return;

	// Free the effects object.
	delete self;
}

void MCBitmapEffectsClear(MCBitmapEffectsRef& self)
{
	// Get rid of any effects object that is present.
	MCBitmapEffectsFinalize(self);

	// Set ourselves to empty.
	self = NULL;
}

// This method returns true if the bitmap effects object contains effects
// which only affect the interior of the shape.
bool MCBitmapEffectsIsInteriorOnly(MCBitmapEffectsRef self)
{
	// No effects are interior only (they have no effect at all!).
	if (self == nil)
		return true;

	// Only interior if mask does not contain outerglow / drop-shadow.
	return (self -> mask & (kMCBitmapEffectTypeDropShadowBit | kMCBitmapEffectTypeOuterGlowBit)) == 0;
}

bool MCBitmapEffectsAssign(MCBitmapEffectsRef& self, MCBitmapEffectsRef src)
{
	// We will first build our new value.
	MCBitmapEffects *t_new_self;

	// If the source is not NULL, then we allocate a new object
	if (src != NULL)
	{
		// Allocate, returning false if it fails.
		t_new_self = new (nothrow) MCBitmapEffects;
		if (t_new_self == NULL)
			return false;

		// Copy across the source - since MCBitmapEffects is just a POD type
		// at the moment this is easy.
		memcpy(t_new_self, src, sizeof(MCBitmapEffects));
	}
	else
		t_new_self = NULL;

	// Get rid of any effects object that we are currently attached to.
	MCBitmapEffectsFinalize(self);

	// Now assign our new self.
	self = t_new_self;

	return true;
}

bool MCBitmapEffectsScale(MCBitmapEffectsRef& self, int32_t p_scale)
{
	// Nothing to do if we are empty
	if (self == NULL)
		return true;

	// Now just scale the relevant portions of the effects. Note that this is
	// currently not quite correct due to field size limits in the effects
	// structure - thus we need to change this slightly (although for now it
	// bounds the maximum size to 64, which is computationally feasible).
	for(uint32_t t_type = 0; t_type <= 4; t_type += 1)
	{
		if ((self -> mask & (1 << t_type)) == 0)
			continue;

		self -> effects[t_type] . blur . size = MCU_min((signed)self -> effects[t_type] . blur . size * p_scale, 255);
		if (t_type < 2)
			self -> effects[t_type] . shadow . distance = MCU_min((signed)self -> effects[t_type] . shadow . distance * p_scale, 32767); 
	}

	return true;
}

static void MCBitmapEffectColorToMCColor(uint32_t, MCColor &) ATTRIBUTE_UNUSED;
static void MCBitmapEffectColorFromMCColor(MCColor &, uint32_t &) ATTRIBUTE_UNUSED;

// MM-2013-12-10: [[ Bug  11568 ]] Store colors as BGRA instead of native since never directly rasterized.
static void MCBitmapEffectColorToMCColor(uint32_t p_color, MCColor &r_color)
{
	MCColorSetPixel(r_color, p_color, kMCGPixelFormatBGRA);
}

// MM-2013-12-10: [[ Bug  11568 ]] Store colors as BGRA instead of native since never directly rasterized.
static void MCBitmapEffectColorFromMCColor(MCColor &p_color, uint32_t &r_color)
{
	r_color = MCColorGetPixel(p_color, kMCGPixelFormatBGRA);
}

// Set the given effect to default values for its type.
static void MCBitmapEffectDefault(MCBitmapEffect *p_effect, MCBitmapEffectType p_type)
{
	// Default color is 75% black
	p_effect -> layer . color = MCGPixelPackNative(0, 0, 0, 0xbf);

	// Default blend mode is normal
	p_effect -> layer . blend_mode = kMCBitmapEffectBlendModeNormal;

	if (((1 << p_type) & kMCBitmapEffectTypeAllBlurBits) != 0)
	{
		// Default filter is 3-pass box filter
		p_effect -> blur . filter = kMCBitmapEffectFilterThreePassBox;

		// Default size is 5
		p_effect -> blur . size = 5;

		// Default spread is 0% as we want consistency of look between the
		// filters, and box filter doesn't do spread yet.
		p_effect -> blur . spread = 0x00;

		if (((1 << p_type) & kMCBitmapEffectTypeAllShadowBits) != 0)
		{
			// Default angle is 60
			p_effect -> shadow . angle = 60;

			// Default distance is 5
			p_effect -> shadow . distance = 5;

			// Default knockout is true
			p_effect -> shadow . knockout = true;
		}
		else if (((1 << p_type) & kMCBitmapEffectTypeAllGlowBits) != 0)
		{
			// Default range is 255
			p_effect -> glow . range = 255;

			// Default source is edge
			p_effect -> glow . source = kMCBitmapEffectSourceEdge;
		}
	}
}

uint32_t MCBitmapEffectsWeigh(MCBitmapEffectsRef self)
{
	uint32_t t_size;
	t_size = sizeof(uint32_t);

	// Weigh the drop shadow
	if ((self -> mask & kMCBitmapEffectTypeDropShadowBit) != 0)
		t_size += kMCShadowEffectPickleSize;

	// Weigh the outer glow
	if ((self -> mask & kMCBitmapEffectTypeOuterGlowBit) != 0)
		t_size += kMCGlowEffectPickleSize;

	// Weigh the inner shadow
	if ((self -> mask & kMCBitmapEffectTypeInnerShadowBit) != 0)
		t_size += kMCShadowEffectPickleSize;

	// Weigh the inner glow
	if ((self -> mask & kMCBitmapEffectTypeInnerGlowBit) != 0)
		t_size += kMCGlowEffectPickleSize;

	// Weigh the color overlay
	if ((self -> mask & kMCBitmapEffectTypeColorOverlayBit) != 0)
		t_size += kMCLayerEffectPickleSize;

	// Return the weight
	return t_size;
}

static IO_stat MCLayerEffectPickle(MCLayerEffect *self, MCObjectOutputStream& p_stream)
{
	IO_stat t_stat;
    // MM-2013-12-10: [[ Bug  11568 ]] Colors are now BGRA so no longer need to convert to standard form.
	t_stat = p_stream . WriteU32(self -> color);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU8(self -> blend_mode << 4);
	return t_stat;
}

static IO_stat MCShadowEffectPickle(MCShadowEffect *self, MCObjectOutputStream& p_stream)
{
	IO_stat t_stat;
    // MM-2013-12-10: [[ Bug  11568 ]] Colors are now BGRA so no longer need to convert to standard form.
    t_stat = p_stream . WriteU32(self -> color);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU32((self -> blend_mode << 28) | (self -> filter << 25) | (self -> size << 17) | (self -> spread << 9) | (self -> angle << 0));
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU16((self -> distance << 1) | (self -> knockout << 0));
	return t_stat;
}

static IO_stat MCGlowEffectPickle(MCGlowEffect *self, MCObjectOutputStream& p_stream)
{
	IO_stat t_stat;
    // MM-2013-12-10: [[ Bug  11568 ]] Colors are now BGRA so no longer need to convert to standard form.
    t_stat = p_stream . WriteU32(self -> color);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU32((self -> blend_mode << 28) | (self -> filter << 25) | (self -> size << 17) | (self -> spread << 9) | (self -> range << 1) | (self -> source << 0));
	return t_stat;
}

IO_stat MCBitmapEffectsPickle(MCBitmapEffectsRef self, MCObjectOutputStream& p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	// Write out the size
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU16(MCBitmapEffectsWeigh(self));

	// Write out the mask
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU16(self -> mask);

	// Weigh the drop shadow
	if (t_stat == IO_NORMAL && (self -> mask & kMCBitmapEffectTypeDropShadowBit) != 0)
		t_stat = MCShadowEffectPickle(&self -> effects[kMCBitmapEffectTypeDropShadow] . shadow, p_stream);

	// Weigh the outer glow
	if (t_stat == IO_NORMAL && (self -> mask & kMCBitmapEffectTypeOuterGlowBit) != 0)
		t_stat = MCGlowEffectPickle(&self -> effects[kMCBitmapEffectTypeOuterGlow] . glow, p_stream);

	// Weigh the inner shadow
	if (t_stat == IO_NORMAL && (self -> mask & kMCBitmapEffectTypeInnerShadowBit) != 0)
		t_stat = MCShadowEffectPickle(&self -> effects[kMCBitmapEffectTypeInnerShadow] . shadow, p_stream);

	// Weigh the inner glow
	if (t_stat == IO_NORMAL && (self -> mask & kMCBitmapEffectTypeInnerGlowBit) != 0)
		t_stat = MCGlowEffectPickle(&self -> effects[kMCBitmapEffectTypeInnerGlow] . glow, p_stream);

	// Weigh the color overlay
	if (t_stat == IO_NORMAL && (self -> mask & kMCBitmapEffectTypeColorOverlayBit) != 0)
		t_stat = MCLayerEffectPickle(&self -> effects[kMCBitmapEffectTypeColorOverlay] . layer, p_stream);

	return t_stat;
}

static IO_stat MCLayerEffectUnpickle(MCLayerEffect *self, MCObjectInputStream& p_stream)
{
	IO_stat t_stat;
	uint32_t t_color;
	t_stat = p_stream . ReadU32(t_color);
    // MM-2013-12-10: [[ Bug  11568 ]] Colors are now BGRA so no longer need to convert from standard form.
	self -> color = t_color;
	
	uint8_t t_flags;
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU8(t_flags);

	if (t_stat == IO_NORMAL)
	{
		self -> blend_mode = (MCBitmapEffectBlendMode)(t_flags >> 4);
	}

	return t_stat;
}

static IO_stat MCShadowEffectUnpickle(MCShadowEffect *self, MCObjectInputStream& p_stream)
{
	IO_stat t_stat;
	uint32_t t_color;
	t_stat = p_stream . ReadU32(t_color);
    // MM-2013-12-10: [[ Bug  11568 ]] Colors are now BGRA so no longer need to convert from standard form.
    self -> color = t_color;
	
	uint32_t t_data_a;
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU32(t_data_a);

	uint16_t t_data_b;
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU16(t_data_b);

	if (t_stat == IO_NORMAL)
	{
		self -> blend_mode = (MCBitmapEffectBlendMode)((t_data_a >> 28) & 0x0f);
		self -> filter = (MCBitmapEffectFilter)((t_data_a >> 25) & 0x07);
		self -> size = (t_data_a >> 17) & 0xff;
		self -> spread = (t_data_a >> 9) & 0xff;
		self -> angle = (t_data_a >> 0) & 0x01ff;
		self -> distance = (t_data_b >> 1) & 0x7fff;
		self -> knockout = (t_data_b & 0x01);
	}

	return t_stat;
}

static IO_stat MCGlowEffectUnpickle(MCGlowEffect *self, MCObjectInputStream& p_stream)
{
	IO_stat t_stat;
	uint32_t t_color;
	t_stat = p_stream . ReadU32(t_color);
    // MM-2013-12-10: [[ Bug  11568 ]] Colors are now BGRA so no longer need to convert from standard form.
    self -> color = t_color;
	
	uint32_t t_data;
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU32(t_data);

	if (t_stat == IO_NORMAL)
	{
		self -> blend_mode = (MCBitmapEffectBlendMode)((t_data >> 28) & 0x0f);
		self -> filter = (MCBitmapEffectFilter)((t_data >> 25) & 0x07);
		self -> size = (t_data >> 17) & 0xff;
		self -> spread = (t_data >> 9) & 0xff;
		self -> range = (t_data >> 1) & 0xff;
		self -> source = (MCBitmapEffectSource)((t_data >> 0) & 0x01);
	}

	return t_stat;
}

IO_stat MCBitmapEffectsUnpickle(MCBitmapEffectsRef& self, MCObjectInputStream& p_stream)
{
	// Create a new instance
	MCBitmapEffects *t_new_self;
	t_new_self = new (nothrow) MCBitmapEffects;
	if (t_new_self == NULL)
		return IO_ERROR;

	t_new_self -> mask = 0;

	// Now read in the contents
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	// Read the size
	uint16_t t_size;
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU16(t_size);

	if (t_stat == IO_NORMAL)
	{
		uint16_t t_mask;
		t_stat = p_stream . ReadU16(t_mask);
		if (t_stat == IO_NORMAL)
			t_new_self -> mask = t_mask;
	}

	// Keep track of how much is left.
	if (t_stat == IO_NORMAL)
		t_size -= 4;

	// Extract the drop shadow
	if ((t_stat == IO_NORMAL && t_new_self -> mask & kMCBitmapEffectTypeDropShadowBit) != 0)
	{
		t_size -= kMCShadowEffectPickleSize;
		t_stat = MCShadowEffectUnpickle(&t_new_self -> effects[kMCBitmapEffectTypeDropShadow] . shadow, p_stream);
	}

	// Extract the outer glow
	if (t_stat == IO_NORMAL && (t_new_self -> mask & kMCBitmapEffectTypeOuterGlowBit) != 0)
	{
		t_size -= kMCGlowEffectPickleSize;
		t_stat = MCGlowEffectUnpickle(&t_new_self -> effects[kMCBitmapEffectTypeOuterGlow] . glow, p_stream);
	}

	// Extract the inner shadow
	if (t_stat == IO_NORMAL && (t_new_self -> mask & kMCBitmapEffectTypeInnerShadowBit) != 0)
	{
		t_size -= kMCShadowEffectPickleSize;
		t_stat = MCShadowEffectUnpickle(&t_new_self -> effects[kMCBitmapEffectTypeInnerShadow] . shadow, p_stream);
	}

	// Extract the inner glow
	if (t_stat == IO_NORMAL && (t_new_self -> mask & kMCBitmapEffectTypeInnerGlowBit) != 0)
	{
		t_size -= kMCGlowEffectPickleSize;
		t_stat = MCGlowEffectUnpickle(&t_new_self -> effects[kMCBitmapEffectTypeInnerGlow] . glow, p_stream);
	}

	// Extract the color overlay
	if (t_stat == IO_NORMAL && (t_new_self -> mask & kMCBitmapEffectTypeColorOverlayBit) != 0)
	{
		t_size -= kMCLayerEffectPickleSize;
		t_stat = MCLayerEffectUnpickle(&t_new_self -> effects[kMCBitmapEffectTypeColorOverlay] . layer, p_stream);
	}

	if (t_stat == IO_NORMAL && t_size > 0)
		t_stat = p_stream . Read(NULL, t_size);

	// Assign the effects to the output pointer
	if (t_stat == IO_NORMAL)
		MCBitmapEffectsAssign(self, t_new_self);

	// And finalize the temporary copy.
	MCBitmapEffectsFinalize(t_new_self);

	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////

// Compute the region of the source required to render the shape into clipped
// with the given glow effect.
static MCRectangle MCGlowEffectComputeClip(MCGlowEffect *self, const MCRectangle& p_shape, const MCRectangle& p_clip)
{
	return MCU_reduce_rect(MCU_intersect_rect(p_shape, p_clip), -(signed)self -> size);
}

// Compute the region of the source required to render the shape into clip with
// the given shadow effect,
static MCRectangle MCShadowEffectComputeClip(MCShadowEffect *self, const MCRectangle& p_shape, const MCRectangle& p_clip)
{
	// Compute the displacement of the shadow from the shape
	int32_t t_delta_x, t_delta_y;
	t_delta_x = (int32_t)floor(0.5 + self -> distance * cos(self -> angle * M_PI / 180.0));
	t_delta_y = (int32_t)floor(0.5 + self -> distance * sin(self -> angle * M_PI / 180.0));

	// Now inversely displace the clip and intersect with the shape to get the
	// unblurred region of the shape we need to render the shadow. The expand.
	return MCU_reduce_rect(MCU_intersect_rect(p_shape, MCU_offset_rect(p_clip, -t_delta_x, -t_delta_y)), -(signed)self -> size);
}

void MCBitmapEffectsComputeClip(MCBitmapEffectsRef self, const MCRectangle& p_shape, const MCRectangle& p_clip, MCRectangle& r_layer_clip)
{
	MCRectangle t_layer_clip;
	t_layer_clip = MCU_intersect_rect(p_clip, p_shape);

	// Drop shadows require pixels from a displaced region
	if ((self -> mask & kMCBitmapEffectTypeDropShadowBit) != 0)
		t_layer_clip = MCU_union_rect(t_layer_clip, MCShadowEffectComputeClip(&self -> effects[kMCBitmapEffectTypeDropShadow] . shadow, p_shape, p_clip));

	// Inner shadows require pixels from a displaced region
	if ((self -> mask & kMCBitmapEffectTypeInnerShadowBit) != 0)
		t_layer_clip = MCU_union_rect(t_layer_clip, MCShadowEffectComputeClip(&self -> effects[kMCBitmapEffectTypeInnerShadow] . shadow, p_shape, p_clip));

	// Outer glows require extra pixels for the blur
	if ((self -> mask & kMCBitmapEffectTypeOuterGlowBit) != 0)
		t_layer_clip = MCU_union_rect(t_layer_clip, MCGlowEffectComputeClip(&self -> effects[kMCBitmapEffectTypeOuterGlow] . glow, p_shape, p_clip));

	// Inner glows require extra pixels for the blur
	if ((self -> mask & kMCBitmapEffectTypeInnerGlowBit) != 0)
		t_layer_clip = MCU_union_rect(t_layer_clip, MCGlowEffectComputeClip(&self -> effects[kMCBitmapEffectTypeInnerGlow] . glow, p_shape, p_clip));

	r_layer_clip = MCU_intersect_rect(p_shape, t_layer_clip);
}

static MCRectangle MCOuterGlowEffectComputeBounds(MCGlowEffect *self, const MCRectangle& p_shape)
{
	return MCU_reduce_rect(p_shape, -(signed)self -> size);
}

static MCRectangle MCDropShadowEffectComputeBounds(MCShadowEffect *self, const MCRectangle& p_shape)
{
	// Compute the displacement of the shadow from the shape
	int32_t t_delta_x, t_delta_y;
	t_delta_x = (int32_t)floor(0.5 + self -> distance * cos(self -> angle * M_PI / 180.0));
	t_delta_y = (int32_t)floor(0.5 + self -> distance * sin(self -> angle * M_PI / 180.0));
	return MCU_reduce_rect(MCU_offset_rect(p_shape, t_delta_x, t_delta_y), -(signed)self -> size);
}

void MCBitmapEffectsComputeBounds(MCBitmapEffectsRef self, const MCRectangle& p_shape, MCRectangle& r_bounds)
{
	// The bounds are at least as big as the shape.
	MCRectangle t_bounds;
	t_bounds = p_shape;

	// Only drop shadows and outer glows have an effect on the exterior bounds.

	if ((self -> mask & kMCBitmapEffectTypeDropShadowBit) != 0)
		t_bounds = MCU_union_rect(t_bounds, MCDropShadowEffectComputeBounds(&self -> effects[kMCBitmapEffectTypeDropShadow] . shadow, p_shape));

	if ((self -> mask & kMCBitmapEffectTypeOuterGlowBit) != 0)
		t_bounds = MCU_union_rect(t_bounds, MCOuterGlowEffectComputeBounds(&self -> effects[kMCBitmapEffectTypeOuterGlow] . glow, p_shape));

	r_bounds = t_bounds;
}

////////////////////////////////////////////////////////////////////////////////

static void MCBitmapEffectFetchProperty(MCExecContext& ctxt, MCBitmapEffect *effect, MCBitmapEffectProperty p_prop, MCExecValue& r_value)
{
    if (effect == nil)
    {
        r_value . stringref_value = MCValueRetain(kMCEmptyString);
        r_value . type = kMCExecValueTypeStringRef;
        return;
    }
    switch(p_prop)
    {
        case kMCBitmapEffectPropertyColor:
            MCBitmapEffectColorToMCColor(effect->layer.color, r_value . color_value);
            r_value . type = kMCExecValueTypeColor;
            break;
            
        case kMCBitmapEffectPropertyBlendMode:
            MCExecFormatEnum(ctxt, kMCInterfaceBitmapEffectBlendModeTypeInfo, (intenum_t)effect->layer.blend_mode, r_value);
            break;
            
        case kMCBitmapEffectPropertyFilter:
            MCExecFormatEnum(ctxt, kMCInterfaceBitmapEffectFilterTypeInfo, (intenum_t)effect->blur.filter, r_value);
            break;
            
        case kMCBitmapEffectPropertySource:
            MCExecFormatEnum(ctxt, kMCInterfaceBitmapEffectSourceTypeInfo, (intenum_t)effect -> glow . source, r_value);
            break;
            
        case kMCBitmapEffectPropertyOpacity:
            // MM-2013-12-10: [[ Bug  11568 ]] Store colors as BGRA instead of native since never directly rasterized.
            r_value . uint_value = MCGPixelGetAlpha(kMCGPixelFormatBGRA, effect -> layer . color);
            r_value . type = kMCExecValueTypeUInt;
            break;
            
        case kMCBitmapEffectPropertySize:
            r_value . uint_value = effect -> blur . size;
            r_value . type = kMCExecValueTypeUInt;
            break;
            
        case kMCBitmapEffectPropertySpread:
            r_value . uint_value = effect -> blur . spread;
            r_value . type = kMCExecValueTypeUInt;
            break;
            
        case kMCBitmapEffectPropertyDistance:
            r_value . uint_value = effect -> shadow . distance;
            r_value . type = kMCExecValueTypeUInt;
            break;
            
        case kMCBitmapEffectPropertyAngle:
            r_value . uint_value = effect -> shadow . angle;
            r_value . type = kMCExecValueTypeUInt;
            break;
            
        case kMCBitmapEffectPropertyRange:
            r_value . uint_value = effect -> glow . range;
            r_value . type = kMCExecValueTypeUInt;
            break;
            
        case kMCBitmapEffectPropertyKnockOut:
            r_value . bool_value = effect -> shadow . knockout;
            r_value . type = kMCExecValueTypeBool;
            break;
            
        default:
            break;
    }
}

bool MCBitmapEffectsGetProperty(MCExecContext& ctxt, MCBitmapEffectsRef& self, MCNameRef p_index, Properties which, MCExecValue& r_value)
{
    // First map the property type
	MCBitmapEffectType t_type;
	t_type = (MCBitmapEffectType)(which - P_BITMAP_EFFECT_DROP_SHADOW);
    
    // If 'p_index' is the empty name, this is a whole array op.
    bool t_is_array;
    t_is_array = MCNameIsEmpty(p_index);
    
	// Now fetch the bitmap effect we are processing - note that if this is
	// NULL it means it isn't set. In this case we still carry on since we
	// need to report a invalid key error (if applicable).
	MCBitmapEffect *t_effect;
	if (self != nil && (self  -> mask & (1 << t_type)) != 0)
		t_effect = &self -> effects[t_type];
	else
		t_effect = nil;

    if (t_is_array)
    {
        if (t_effect == nil)
        {
            r_value . stringref_value = MCValueRetain(kMCEmptyString);
            r_value . type = kMCExecValueTypeStringRef;
            return true;
        }
        
        // Otherwise we have an array get, so first create a new value
        MCAutoArrayRef v;
        if (MCArrayCreateMutable(&v))
        {
            // Now loop through all the properties, getting the ones applicable to this type.
            for(uint32_t i = 0; i < ELEMENTS(s_bitmap_effect_properties); i++)
            {
                if ((s_bitmap_effect_properties[i] . mask & (1 << t_type)) != 0)
                {
                    MCExecValue t_value;
                    MCAutoValueRef t_valueref;
                    // Fetch the property, then store it into the array.
                    MCBitmapEffectFetchProperty(ctxt, t_effect, s_bitmap_effect_properties[i] . value, t_value);
                    MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeValueRef, &(&t_valueref));
                    MCArrayStoreValue(*v, false, MCNAME(s_bitmap_effect_properties[i] . token), *t_valueref);
                }
            }
            r_value . arrayref_value = MCValueRetain(*v);
            r_value . type = kMCExecValueTypeArrayRef;
            return true;
        }
    }
    else
    {
        MCBitmapEffectProperty t_prop;
        if (MCBitmapEffectLookupProperty(t_type, p_index,
                                         t_prop) != ES_NORMAL)
            return false;
        MCBitmapEffectFetchProperty(ctxt, t_effect, t_prop, r_value);
        return true;
    }

    return false;
}

static void MCBitmapEffectsSetColorProperty(MCBitmapEffect& x_effect, MCBitmapEffectProperty p_prop, MCColor p_color, bool& x_dirty)
{
    switch (p_prop)
    {
        case kMCBitmapEffectPropertyColor:
        {
            // MM-2013-12-10: [[ Bug  11568 ]] Store colors as BGRA instead of native since never directly rasterized.
			uint4 t_new_color;
			t_new_color = MCGPixelPack(kMCGPixelFormatBGRA, p_color.red >> 8, p_color.green >> 8, p_color.blue >> 8, MCGPixelGetAlpha(kMCGPixelFormatBGRA, x_effect . layer . color));
            
            if (t_new_color != x_effect . layer . color)
            {
                x_effect . layer . color = t_new_color;
                x_dirty = true;
            }
        }
            break;
            
        default:
            break;
    }
}

static void MCBitmapEffectsSetEnumProperty(MCExecContext& ctxt, MCBitmapEffect& x_effect, MCBitmapEffectProperty p_prop, MCExecValue p_value, bool& x_dirty)
{
    intenum_t t_value;
    switch (p_prop)
    {
        case kMCBitmapEffectPropertyBlendMode:
        {
            MCExecParseEnum(ctxt, kMCInterfaceBitmapEffectBlendModeTypeInfo, p_value, t_value);
            if (!ctxt . HasError())
            {
                MCBitmapEffectBlendMode t_new_mode;
                t_new_mode = (MCBitmapEffectBlendMode)t_value;
                if (t_new_mode != x_effect . layer . blend_mode)
                {
                    x_effect . layer . blend_mode = t_new_mode;
                    x_dirty = true;
                }
            }
        }
            break;
            
        case kMCBitmapEffectPropertyFilter:
        {
            MCExecParseEnum(ctxt, kMCInterfaceBitmapEffectFilterTypeInfo, p_value, t_value);
            if (!ctxt . HasError())
            {
                MCBitmapEffectFilter t_new_filter;
                t_new_filter = (MCBitmapEffectFilter)t_value;
                if (t_new_filter != x_effect . blur . filter)
                {
                    x_effect . blur . filter = t_new_filter;
                    x_dirty = true;
                }
            }
        }
            break;
            
        case kMCBitmapEffectPropertySource:
        {
            MCExecParseEnum(ctxt, kMCInterfaceBitmapEffectSourceTypeInfo, p_value, t_value);
            if (!ctxt . HasError())
            {
                MCBitmapEffectSource t_new_source;
                t_new_source = (MCBitmapEffectSource)t_value;
                if (t_new_source != x_effect . glow . source)
                {
                    x_effect . glow . source = t_new_source;
                    x_dirty = true;
                }
            }
        }
            break;
            
        default:
            break;
    }
}

static void MCBitmapEffectsSetUIntProperty(MCBitmapEffect& x_effect, MCBitmapEffectProperty p_prop, uinteger_t p_uint, bool& x_dirty)
{
    switch (p_prop)
    {
        case kMCBitmapEffectPropertyOpacity:
        {
            p_uint = MCU_min(p_uint, (uint4)255);
            // MM-2013-12-10: [[ Bug  11568 ]] Store colors as BGRA instead of native since never directly rasterized.
            if (p_uint != MCGPixelGetAlpha(kMCGPixelFormatBGRA, x_effect . layer . color))
            {
                uint8_t r, g, b, a;
                MCGPixelUnpack(kMCGPixelFormatBGRA, x_effect . layer . color, r, g, b, a);
                
                x_effect . layer . color = MCGPixelPack(kMCGPixelFormatBGRA, r, g, b, p_uint);
                x_dirty = true;
            }
        }
            break;
            
        case kMCBitmapEffectPropertySize:
        {
            p_uint = MCU_min(p_uint, (uint4)255);
            if (p_uint != x_effect . blur . size)
            {
                x_effect . blur . size = p_uint;
                x_dirty = true;
            }
        }
            break;
            
        case kMCBitmapEffectPropertySpread:
        {
            p_uint = MCU_min(p_uint, (uint4)255);
            if (p_uint != x_effect . blur . spread)
            {
                x_effect . blur . spread = p_uint;
                x_dirty = true;
            }
        }
            break;
            
        case kMCBitmapEffectPropertyDistance:
        {
            p_uint = MCU_min(p_uint, (uint4)32767);
            if (p_uint != x_effect . shadow . distance)
            {
                x_effect . shadow . distance = p_uint;
                x_dirty = true;
            }
        }
            break;
            
        case kMCBitmapEffectPropertyAngle:
        {
            p_uint %= 360;
            if (p_uint != x_effect . shadow . angle)
            {
                x_effect . shadow . angle = p_uint;
                x_dirty = true;
            }
        }
            break;
            
        // AL-2014-11-25: [[ Bug 14092 ]] Can't set glow range property
        case kMCBitmapEffectPropertyRange:
        {
            p_uint = MCU_min(p_uint, (uint4)255);
            if (p_uint != x_effect . glow . range)
            {
                x_effect . glow . range = p_uint;
                x_dirty = true;
            }
        }
            break;
		default:
			MCUnreachableReturn();
			break;
    }
}

static void MCBitmapEffectsSetBooleanProperty(MCBitmapEffect& x_effect, MCBitmapEffectProperty p_prop, bool p_setting, bool& x_dirty)
{
    switch (p_prop)
    {
        // AL-2014-11-25: [[ Bug 14092 ]] Can't set shadow knockout property
        case kMCBitmapEffectPropertyKnockOut:
        {
            if (p_setting != x_effect . shadow . knockout)
            {
                x_effect . shadow . knockout = p_setting;
                x_dirty = true;
            }
        }
            break;
            
        default:
            break;
    }
}

static void MCBitmapEffectStoreProperty(MCExecContext& ctxt, MCBitmapEffect& x_effect, MCBitmapEffectProperty p_prop, MCExecValue p_value, bool& r_dirty)
{
    switch (p_prop)
    {
        case kMCBitmapEffectPropertyColor:
        {
            MCColor t_color;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value , kMCExecValueTypeColor, &t_color);
            MCBitmapEffectsSetColorProperty(x_effect, p_prop, t_color, r_dirty);
        }
            break;
        case kMCBitmapEffectPropertyBlendMode:
        case kMCBitmapEffectPropertyFilter:
        case kMCBitmapEffectPropertySource:
            MCBitmapEffectsSetEnumProperty(ctxt, x_effect, p_prop, p_value, r_dirty);
            break;
        case kMCBitmapEffectPropertyOpacity:
        case kMCBitmapEffectPropertySize:
        case kMCBitmapEffectPropertySpread:
        case kMCBitmapEffectPropertyDistance:
        case kMCBitmapEffectPropertyAngle:
        // AL-2014-11-25: [[ Bug 14092 ]] Can't set glow range property
        case kMCBitmapEffectPropertyRange:
        {
            uinteger_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value , kMCExecValueTypeUInt, &t_value);
            MCBitmapEffectsSetUIntProperty(x_effect, p_prop, t_value, r_dirty);
        }
            break;
        // AL-2014-11-25: [[ Bug 14092 ]] Can't set shadow knockout property
        case kMCBitmapEffectPropertyKnockOut:
        {
            bool t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeBool, &t_value);
            MCBitmapEffectsSetBooleanProperty(x_effect, p_prop, t_value, r_dirty);
        }
            break;
        default:
            break;
    }
}

bool MCBitmapEffectsSetProperty(MCExecContext& ctxt, MCBitmapEffectsRef& self, MCNameRef p_index, Properties which, MCExecValue p_value, bool& r_dirty)
{
    // First map the property type
	MCBitmapEffectType t_type;
	t_type = (MCBitmapEffectType)(which - P_BITMAP_EFFECT_DROP_SHADOW);
    
    // If 'p_index' is the empty name, this is a whole array op.
    bool t_is_array;
    t_is_array = MCNameIsEmpty(p_index);

    // AL-2014-05-21: [[ Bug 12459 ]] Ensure setting bitmap effect array to anything
    //  that isn't an array does the same as setting it to 'empty'.
    // AL_2014-05-23: [[ Bug 12483 ]] Only do this if we are setting all the properties at once.
    if (t_is_array &&
        (p_value . type == kMCExecValueTypeValueRef &&
        (MCValueIsEmpty(p_value . valueref_value) ||
         (MCValueGetTypeCode(p_value . valueref_value) != kMCValueTypeCodeArray))))
    {
        if (self == nil || (self -> mask & (1 << t_type)) == 0)
        {
            r_dirty = false;
            return true;
        }

		// We are set, so just unset our bit in the mask
		self -> mask &= ~(1 << t_type);
        
		// If we are now empty, then clear.
		if (self -> mask == 0)
			MCBitmapEffectsClear(self);
		
		// Mark the object as dirty
		r_dirty = true;
        
		return true;
    }
    
    MCBitmapEffect effect;
    bool t_dirty;
    
    // Now fetch the bitmap effect we are processing - note that if this is
    // NULL it means it isn't set. In this case we still carry on since we
    // need to report a invalid key error (if applicable).
    if (self != nil && (self  -> mask & (1 << t_type)) != 0)
    {
        effect = self -> effects[t_type];
        t_dirty = false;
    }
    else
    {
        MCBitmapEffectDefault(&effect, t_type);
        // If the effect doesn't yet exist, it means we will dirty the object
        // regardless.
        t_dirty = true;
    }
    
    if (t_is_array)
    {
        bool t_dirty_array;
        t_dirty_array = false;
        MCAutoArrayRef t_array;
        MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value , kMCExecValueTypeArrayRef, &(&t_array));
        // Loop through all the properties in the table and apply the relevant
        // ones.
        for(uint32_t i = 0; i < ELEMENTS(s_bitmap_effect_properties); i++)
            if ((s_bitmap_effect_properties[i] . mask & (1 << t_type)) != 0)
            {
                MCValueRef t_prop_value;
                
                // If we don't have the given element, then move to the next one
                if (!MCArrayFetchValue(*t_array, false, MCNAME(s_bitmap_effect_properties[i] . token), t_prop_value))
                    continue;
                
                // Otherwise, fetch the keys value and attempt to set the property
                MCExecValue t_value;
                t_value . valueref_value = MCValueRetain(t_prop_value);
                t_value . type = kMCExecValueTypeValueRef;
                MCBitmapEffectStoreProperty(ctxt, effect, s_bitmap_effect_properties[i] . value, t_value, t_dirty_array);
            }
        if (t_dirty_array)
            t_dirty = true;
        
    }
    else
    {
        MCBitmapEffectProperty t_prop;
        if (MCBitmapEffectLookupProperty(t_type, p_index,
                                         t_prop) != ES_NORMAL)
            return false;

        MCBitmapEffectStoreProperty(ctxt, effect, t_prop, p_value, t_dirty);
    }

    if (t_dirty)
    {
        // If we are currently empty, then allocate a new object
        if (self == nil)
        {
            self = new (nothrow) MCBitmapEffects;
            if (self == nil)
                return false;
            
            // Only need to initialize the mask.
            self -> mask = 0;
        }
        
        // Now copy in the updated effect.
        self -> mask |= (1 << t_type);
        self -> effects[t_type] = effect;
    }
    
    r_dirty = t_dirty;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
