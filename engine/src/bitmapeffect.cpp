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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "execpt.h"

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
		if (MCNameIsEqualToCString(p_token, s_bitmap_effect_properties[i] . token, kMCCompareCaseless))
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
		t_new_self = new MCBitmapEffects;
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

static void MCBitmapEffectColorToMCColor(uint32_t p_color, MCColor &r_color)
{
	r_color.pixel = p_color & 0x00FFFFFF;
	MCscreen->querycolor(r_color);
}

static void MCBitmapEffectColorFromMCColor(MCColor &p_color, uint32_t &r_color)
{
	MCColor t_color = p_color;
	MCscreen->alloccolor(t_color);
	r_color = t_color.pixel;
}

// Fetch the given property from the specified type of effect. Note that we assume that 'which'
// is valid for 'type' and that 'ep' is empty.
static Exec_stat MCBitmapEffectGetProperty(MCBitmapEffect *effect, MCBitmapEffectProperty which, MCExecPoint& ep)
{
	// Now dispatch on property
	switch(which)
	{
		// LAYER EFFECTS

		case kMCBitmapEffectPropertyColor:
		{
			MCColor t_color;
			MCBitmapEffectColorToMCColor(effect->layer.color, t_color);
			ep . setcolor(t_color.red >> 8, t_color.green >> 8, t_color.blue >> 8);
		}
		break;

		case kMCBitmapEffectPropertyBlendMode:
			if (effect->layer.blend_mode == kMCBitmapEffectBlendModeNormal)
				ep.setstaticcstring("normal");
			else if (effect->layer.blend_mode == kMCBitmapEffectBlendModeMultiply)
				ep.setstaticcstring("multiply");
			else if (effect->layer.blend_mode == kMCBitmapEffectBlendModeColorDodge)
				ep.setstaticcstring("colordodge");

			break;

		case kMCBitmapEffectPropertyOpacity:
			ep . setuint((effect -> layer . color >> 24) & 0xff);
			break;
		
		// BLUR EFFECTS

		case kMCBitmapEffectPropertyFilter:
			switch (effect->blur.filter)
			{
			case kMCBitmapEffectFilterFastGaussian:
				ep.setstaticcstring("gaussian");
				break;
			case kMCBitmapEffectFilterOnePassBox:
				ep.setstaticcstring("box1pass");
				break;
			case kMCBitmapEffectFilterTwoPassBox:
				ep.setstaticcstring("box2pass");
				break;
			case kMCBitmapEffectFilterThreePassBox:
				ep.setstaticcstring("box3pass");
				break;
			}
			break;

		case kMCBitmapEffectPropertySize:
			ep . setuint(effect -> blur . size);
			break;

		case kMCBitmapEffectPropertySpread:
			ep . setuint(effect -> blur . spread);
			break;
		
		// SHADOW EFFECTS

		case kMCBitmapEffectPropertyDistance:
			ep . setuint(effect -> shadow . distance);
			break;
			
		case kMCBitmapEffectPropertyAngle:
			ep . setuint(effect -> shadow . angle);
			break;

		case kMCBitmapEffectPropertyKnockOut:
			ep . setboolean(effect -> shadow . knockout);
			break;

		// GLOW EFFECTS

		case kMCBitmapEffectPropertyRange:
			ep . setuint(effect -> glow . range);
			break;

		case kMCBitmapEffectPropertySource:
			ep . setstaticcstring(effect -> glow . source == kMCBitmapEffectSourceEdge ? "edge" : "center");
			break;

		default:
			break;
	}

	return ES_NORMAL;
}

Exec_stat MCBitmapEffectsGetProperties(MCBitmapEffectsRef& self, Properties which_type, MCExecPoint& ep, MCNameRef prop)
{
	// Reset ep to empty, the default value.
	ep . clear();

	// First map the property type
	MCBitmapEffectType t_type;
	t_type = (MCBitmapEffectType)(which_type - P_BITMAP_EFFECT_DROP_SHADOW);

	// If 'prop' is the empty string, this is a whole array op.
	bool t_is_array;
	t_is_array = MCNameIsEqualTo(prop, kMCEmptyName, kMCCompareCaseless);

	// Now fetch the bitmap effect we are processing - note that if this is
	// NULL it means it isn't set. In this case we still carry on since we
	// need to report a invalid key error (if applicable).
	MCBitmapEffect *t_effect;
	if (self != NULL && (self -> mask & (1 << t_type)) != 0)
		t_effect = &self -> effects[t_type];
	else
		t_effect = NULL;

	// If prop is not the empty string, then this is a single property fetch.
	if (!t_is_array)
	{
		MCBitmapEffectProperty t_property;
		if (MCBitmapEffectLookupProperty(t_type, prop, t_property) != ES_NORMAL)
			return ES_ERROR;

		// If there is no effect set for this type, then we are done.
		if (t_effect == NULL)
			return ES_NORMAL;
			
		// Otherwise fetch for the appropriate effect.
		return MCBitmapEffectGetProperty(&self -> effects[t_type], t_property, ep);
	}

	// If there is no effect set for this type, then we are done.
	if (t_effect == NULL)
		return ES_NORMAL;

	// Otherwise we have an array get, so first create a new value
	MCVariableValue *v;
	v = new MCVariableValue;
	if (v == NULL)
		return ES_ERROR;

	// Initialize it to an array
	v -> assign_new_array(8);

	// Now loop through all the properties, getting the ones applicable to this type.
	for(uint32_t i = 0; i < ELEMENTS(s_bitmap_effect_properties); i++)
		if ((s_bitmap_effect_properties[i] . mask & (1 << t_type)) != 0)
		{
			// Attempt to fetch the property, then store it into the array.
			if (MCBitmapEffectGetProperty(t_effect, s_bitmap_effect_properties[i] . value, ep) != ES_NORMAL ||
				v -> store_element(ep, s_bitmap_effect_properties[i] . token) != ES_NORMAL)
			{
				delete v;
				return ES_ERROR;
			}
		}

	// Give the array to the ep
	ep . setarray(v, True);

	return ES_NORMAL;
}

// Set the given effect to default values for its type.
static void MCBitmapEffectDefault(MCBitmapEffect *p_effect, MCBitmapEffectType p_type)
{
	// Default color is 75% black
	p_effect -> layer . color = 0xbf000000;

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

static Exec_stat MCBitmapEffectSetCardinalProperty(uint4 p_bound, const MCString& p_data, uint4 p_current_value, uint4& r_new_value, Boolean& r_dirty)
{
	uint4 t_value;
	if (!MCU_stoui4(p_data, t_value))
	{
		MCeerror -> add(EE_BITMAPEFFECT_BADNUMBER, 0, 0, p_data);
		return ES_ERROR;
	}

	t_value = MCU_min(t_value, p_bound);
	if (t_value != p_current_value)
		r_dirty = True;

	r_new_value = t_value;

	return ES_NORMAL;
}

static Exec_stat MCBitmapEffectSetBooleanProperty(const MCString& p_data, bool p_current_value, bool& r_new_value, Boolean& r_dirty)
{
	Boolean t_value;
	if (!MCU_stob(p_data, t_value))
	{
		MCeerror -> add(EE_BITMAPEFFECT_BADBOOLEAN, 0, 0, p_data);
		return ES_ERROR;
	}

	bool t_bvalue;
	t_bvalue = t_value == True;
	if (t_bvalue != p_current_value)
		r_dirty = True;

	r_new_value = t_bvalue;

	return ES_NORMAL;
}

Exec_stat MCBitmapEffectSetProperty(MCBitmapEffect *self, MCBitmapEffectProperty which, MCExecPoint& ep, Boolean& r_dirty)
{
	MCString t_data;
	t_data = ep . getsvalue();

	switch(which)
	{
		// LAYER EFFECTS

		case kMCBitmapEffectPropertyColor:
		{
			MCColor t_mc_color;
			char *t_name;
			t_name = NULL;
			if (!MCscreen -> parsecolor(t_data, &t_mc_color, &t_name))
			{
				MCeerror -> add(EE_BITMAPEFFECT_BADCOLOR, 0, 0, t_data);
				return ES_ERROR;
			}

			uint4 t_new_color;
			MCBitmapEffectColorFromMCColor(t_mc_color, t_new_color);
			if (t_new_color != (self -> layer . color & 0xffffff))
			{
				self -> layer . color = (self -> layer . color & 0xff000000) | t_new_color;
				r_dirty = True;
			}
		}
		break;

		case kMCBitmapEffectPropertyBlendMode:
		{
			MCBitmapEffectBlendMode t_new_mode;
			if (t_data == "normal")
				t_new_mode = kMCBitmapEffectBlendModeNormal;
			else if (t_data == "multiply")
				t_new_mode = kMCBitmapEffectBlendModeMultiply;
			else if (t_data == "colordodge")
				t_new_mode = kMCBitmapEffectBlendModeColorDodge;
			else
			{
				MCeerror -> add(EE_BITMAPEFFECT_BADBLENDMODE, 0, 0, t_data);
				return ES_ERROR;
			}

			if (t_new_mode != self -> layer . blend_mode)
			{
				self -> layer . blend_mode = t_new_mode;
				r_dirty = True;
			}
		}
		break;

		case kMCBitmapEffectPropertyOpacity:
		{
			uint4 t_value;
			if (MCBitmapEffectSetCardinalProperty(255, t_data, self -> layer . color >> 24, t_value, r_dirty) != ES_NORMAL)
				return ES_ERROR;
			
			self -> layer . color = (self -> layer . color & 0xffffff) | (t_value << 24);
		}
		break;

		// BLUR EFFECTS

		case kMCBitmapEffectPropertyFilter:
		{
			MCBitmapEffectFilter t_new_filter;
			if (t_data == "gaussian")
				t_new_filter = kMCBitmapEffectFilterFastGaussian;
			else if (t_data == "box1pass")
				t_new_filter = kMCBitmapEffectFilterOnePassBox;
			else if (t_data == "box2pass")
				t_new_filter = kMCBitmapEffectFilterTwoPassBox;
			else if (t_data == "box3pass")
				t_new_filter = kMCBitmapEffectFilterThreePassBox;
			else
			{
				MCeerror -> add(EE_BITMAPEFFECT_BADFILTER, 0, 0, t_data);
				return ES_ERROR;
			}

			if (t_new_filter != self -> blur . filter)
			{
				self -> blur . filter = t_new_filter;
				r_dirty = True;
			}
		}
		break;
			
		case kMCBitmapEffectPropertySize:
		{
			uint4 t_value;
			if (MCBitmapEffectSetCardinalProperty(255, t_data, self -> blur . size, t_value, r_dirty) != ES_NORMAL)
				return ES_ERROR;
			
			self -> blur . size = t_value;
		}
		break;

		case kMCBitmapEffectPropertySpread:
		{
			uint4 t_value;
			if (MCBitmapEffectSetCardinalProperty(255, t_data, self -> blur . spread, t_value, r_dirty) != ES_NORMAL)
				return ES_ERROR;
			
			self -> blur . spread = t_value;
		}
		break;

		// SHADOW EFFECTS

		case kMCBitmapEffectPropertyDistance:
		{
			uint4 t_value;
			if (MCBitmapEffectSetCardinalProperty(32767, t_data, self -> shadow . distance, t_value, r_dirty) != ES_NORMAL)
				return ES_ERROR;
			
			self -> shadow . distance = t_value;
		}
		break;
			
		case kMCBitmapEffectPropertyAngle:
		{
			uint4 t_value;
			if (!MCU_stoui4(t_data, t_value))
			{
				MCeerror -> add(EE_BITMAPEFFECT_BADNUMBER, 0, 0, t_data);
				return ES_ERROR;
			}

			t_value %= 360;

			if (t_value != self -> shadow . angle)
			{
				self -> shadow . angle = t_value;
				r_dirty = True;
			}
		}
		break;
		
		case kMCBitmapEffectPropertyKnockOut:
		{
			bool t_value;
			if (MCBitmapEffectSetBooleanProperty(t_data, self -> shadow . knockout, t_value, r_dirty) != ES_NORMAL)
				return ES_ERROR;

			self -> shadow . knockout = t_value;
		}
		break;

		// GLOW EFFECTS

		case kMCBitmapEffectPropertyRange:
		{
			uint4 t_value;
			if (MCBitmapEffectSetCardinalProperty(255, t_data, self -> glow . range, t_value, r_dirty) != ES_NORMAL)
				return ES_ERROR;
			
			self -> glow . range = t_value;
		}
		break;

		case kMCBitmapEffectPropertySource:
		{
			MCBitmapEffectSource t_new_source;
			if (t_data == "edge")
				t_new_source = kMCBitmapEffectSourceEdge;
			else if (t_data == "center")
				t_new_source = kMCBitmapEffectSourceCenter;
			else
			{
				MCeerror -> add(EE_BITMAPEFFECT_BADSOURCE, 0, 0, t_data);
				return ES_ERROR;
			}

			if (t_new_source != self -> glow . source)
			{
				self -> glow . source = t_new_source;
				r_dirty = True;
			}
		}
		break;

		default:
			break;
	}

	return ES_NORMAL;
}

Exec_stat MCBitmapEffectsSetProperties(MCBitmapEffectsRef& self, Properties which_type, MCExecPoint& ep, MCNameRef prop, Boolean& r_dirty)
{
	// First map the property type
	MCBitmapEffectType t_type;
	t_type = (MCBitmapEffectType)(which_type - P_BITMAP_EFFECT_DROP_SHADOW);

	// If 'prop' is the empty string, this is a whole array op.
	bool t_is_array;
	t_is_array = MCNameIsEqualTo(prop, kMCEmptyName, kMCCompareCaseless);

	// First handle the 'clear' action (i.e. carray is empty and ep is 'empty')
	if (t_is_array && (ep . getformat() != VF_ARRAY || ep . getarray() == NULL))
	{
		if (self == NULL || (self -> mask & (1 << t_type)) == 0)
			return ES_NORMAL;

		// We are set, so just unset our bit in the mask
		self -> mask &= ~(1 << t_type);

		// If we are now empty, then clear.
		if (self -> mask == 0)
			MCBitmapEffectsClear(self);
		
		// Mark the object as dirty
		r_dirty = True;

		return ES_NORMAL;
	}

	// A temporary boolean to store the dirtiness
	Boolean t_dirty;

	// Fetch (a copy of) the bitmap effect we are processing, otherwise
	// initialize one with defaults for the type.
	MCBitmapEffect t_effect;
	if (self != NULL && (self -> mask & (1 << t_type)) != 0)
	{
		t_effect = self -> effects[t_type];

		// At this point we have made no changes.
		t_dirty = False;
	}
	else
	{
		MCBitmapEffectDefault(&t_effect, t_type);

		// If the effect doesn't yet exist, it means we will dirty the object
		// regardless.
		t_dirty = True;
	}

	// Now update the copy of the effect with any newly set properties
	if (!t_is_array)
	{
		// If carray is not the empty string, then this is a single property store
		
		// Lookup the property and ensure it is appropriate for our type.
		MCBitmapEffectProperty t_property;
		if (MCBitmapEffectLookupProperty(t_type, prop, t_property) != ES_NORMAL)
			return ES_ERROR;

		// Set the property in our (copy of the) bitmap effect.
		if (MCBitmapEffectSetProperty(&t_effect, t_property, ep, t_dirty) != ES_NORMAL)
			return ES_ERROR;
	}
	else if (ep . getarray() != NULL)
	{
		// We are storing an array of properties
		MCVariableValue *v;
		v = ep . getarray();

		// Loop through all the properties in the table and apply the relevant
		// ones.
		for(uint32_t i = 0; i < ELEMENTS(s_bitmap_effect_properties); i++)
			if ((s_bitmap_effect_properties[i] . mask & (1 << t_type)) != 0)
			{
				// If we don't have the given element, then move to the next one
				if (!v -> has_element(ep, s_bitmap_effect_properties[i] . token))
					continue;
				
				// Otherwise, fetch the keys value and attempt to set the property
				if (v -> fetch_element(ep, s_bitmap_effect_properties[i] . token) != ES_NORMAL ||
					MCBitmapEffectSetProperty(&t_effect, s_bitmap_effect_properties[i] . value, ep, t_dirty) != ES_NORMAL)
					return ES_ERROR;
			}
	}

	// If no changes have been made, we have nothing to do.
	if (!t_dirty)
		return ES_NORMAL;

	// Otherwise we must commit the changes

	// If we are currently empty, then allocate a new object
	if (self == NULL)
	{
		self = new MCBitmapEffects;
		if (self == NULL)
			return ES_ERROR;

		// Only need to initialize the mask.
		self -> mask = 0;
	}

	// Now copy in the updated effect.
	self -> mask |= (1 << t_type);
	self -> effects[t_type] = t_effect;

	// Return the dirtiness.
	r_dirty = t_dirty;

	return ES_NORMAL;
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
	t_stat = p_stream . WriteU32(self -> color);
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU8(self -> blend_mode << 4);
	return t_stat;
}

static IO_stat MCShadowEffectPickle(MCShadowEffect *self, MCObjectOutputStream& p_stream)
{
	IO_stat t_stat;
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
	t_new_self = new MCBitmapEffects;
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

PACKED_INLINE uint8_t _scale_bounded(uint8_t a, uint8_t b)
{
	uint32_t u;
	u = a * b + 0x80;
	return (u + (u >> 8)) >> 8;
}


static void MCBitmapEffectCompositeNormal(uint32_t *dst, uint32_t color, uint8_t *mask, int32_t count)
{
	for(int32_t x = 0; x < count; x++)
	{
		if (mask[x] == 0)
			continue;

		uint32_t src;
		src = packed_scale_bounded(color, mask[x]);
		dst[x] = packed_scale_bounded(dst[x], 255 - (src >> 24)) + src;
	}
}

static void MCBitmapEffectCompositeMultiply(uint32_t *dst, uint32_t color, uint8_t *mask, int32_t count)
{
	for(int32_t x = 0; x < count; x++)
	{
		if (mask[x] == 0)
			continue;

		uint32_t src;
		src = packed_scale_bounded(color, mask[x]);
		dst[x] = packed_multiply_bounded(src, dst[x]) + packed_bilinear_bounded(src, 255 - (dst[x] >> 24), dst[x], 255 - (src >> 24));
	}
}

static void MCBitmapEffectCompositeMultiplyOpaque(uint32_t *dst, uint32_t color, uint8_t *mask, int32_t count)
{
	for(int32_t x = 0; x < count; x++)
	{
		if (mask[x] == 0)
			continue;

		uint32_t src;
		src = packed_scale_bounded(color, mask[x]);
		dst[x] = packed_multiply_bounded(src, dst[x]) + packed_scale_bounded(dst[x], 255 - (src >> 24));
	}
}

static void MCBitmapEffectCompositeColorDodge(uint32_t *dst, uint32_t color, uint8_t *mask, int32_t count)
{
	for(int32_t x = 0; x < count; x++)
	{
		if (mask[x] == 0)
			continue;
			
		uint32_t srcv;
		// I.M. 2009-08-26 - color dodge works best blending from white -> black
		// rather than opaque -> transparent so retain the color alpha value
		srcv = (packed_scale_bounded(color, mask[x]) & 0x00FFFFFF) | (color & 0xFF000000);
		//srcv = packed_scale_bounded(color, mask[x]);
		
		uint32_t dstv;
		dstv = dst[x];
		
		uint8_t sr, sg, sb;
		sr = srcv & 0xff;
		sg = (srcv >> 8) & 0xff;
		sb = (srcv >> 16) & 0xff;
		
		uint8_t dr, dg, db;
		dr = dstv & 0xff;
		dg = (dstv >> 8) & 0xff;
		db = (dstv >> 16) & 0xff;
		
		uint8_t sa, da;
		sa = srcv >> 24;
		da = dstv >> 24;
		
		uint32_t r, g, b;
		uint32_t tr,tg,tb;
		r = (sa - sr) * da;
		tr = dr * sa;
		r = (r > tr) ? 255 * tr / r : (dr == 0 ? 0 : 255);
		g = (sa - sg) * da;
		tg = dg * sa;
		g = (g > tg) ? 255 * tg / g : (dg == 0 ? 0 : 255);
		b = (sa - sb) * da;
		tb = (db * sa);
		b = (b > tb) ? 255 * tb / b : (db == 0 ? 0 : 255);

		uint32_t f;
		f = 0xff000000 | r | (g << 8) | (b << 16);
		
		dst[x] =
			packed_scale_bounded(f, _scale_bounded(sa, da)) + 
			packed_scale_bounded(srcv, 255 - da) +
			packed_scale_bounded(dstv, 255 - sa);
	}
}

////////////////////////////////////////////////////////////////////////////////

static void MCBitmapEffectAttenuateInner(uint8_t *mask, uint32_t *src, int32_t count)
{
	for(int32_t x = 0; x < count; x++)
		mask[x] = _scale_bounded(mask[x], src[x] >> 24);
}

static void MCBitmapEffectAttenuateInvertedInner(uint8_t *mask, uint32_t *src, int32_t count)
{
	for(int32_t x = 0; x < count; x++)
		mask[x] = _scale_bounded(255 - mask[x], src[x] >> 24);
}

static void MCBitmapEffectAttenuateOuter(uint8_t *mask, uint32_t *src, int32_t count)
{
	for(int32_t x = 0; x < count; x++)
		mask[x] = _scale_bounded(mask[x], 255 - (src[x] >> 24));
}

////////////////////////////////////////////////////////////////////////////////

typedef void (*MCBitmapEffectAttenuateCallback)(uint8_t *mask, uint32_t *src, int32_t count);
typedef void (*MCBitmapEffectCompositeCallback)(uint32_t *dst, uint32_t src, uint8_t *mask, int32_t count);

struct MCBitmapEffectRenderState
{
	// The region we are rendering
	MCRectangle region;

	// The source color
	uint32_t color;

	// The blur parameters (if needed)
	MCBitmapEffectFilter blur_filter;
	uint32_t blur_size;
	uint8_t blur_spread;
	MCRectangle blur_rect;

	// The mask attenuation callback (if needed)
	MCBitmapEffectAttenuateCallback attenuate;
	MCBitmapEffectCompositeCallback composite;
};

static MCBitmapEffectCompositeCallback MCBitmapEffectChooseCompositer(MCBitmapEffectBlendMode p_mode, bool p_has_dst_alpha)
{
	switch(p_mode)
	{
	case kMCBitmapEffectBlendModeNormal:
		return MCBitmapEffectCompositeNormal;

	case kMCBitmapEffectBlendModeMultiply:
		return p_has_dst_alpha ? MCBitmapEffectCompositeMultiply : MCBitmapEffectCompositeMultiplyOpaque;

	case kMCBitmapEffectBlendModeColorDodge:
		return MCBitmapEffectCompositeColorDodge;

	default:
		break;
	}

	return NULL;
}

static void MCBitmapEffectRender(MCBitmapEffectRenderState& state, MCBitmapEffectLayer& dst, MCBitmapEffectLayer& src)
{
	// Compute the dst ptr/stride in pixels.
	uint32_t t_dst_stride, *t_dst_pixels;
	t_dst_stride = dst . stride / 4;
	t_dst_pixels = (uint4 *)dst . bits + t_dst_stride * (state . region . y - dst . bounds . y) + (state . region . x - dst . bounds . x);

	// Compute the blur src ptr/stride in pixels.
	uint32_t t_blur_src_stride, *t_blur_src_pixels;
	t_blur_src_stride = src . stride / 4;
	t_blur_src_pixels = (uint4 *)src . bits + t_blur_src_stride * (state . blur_rect . y - src . bounds . y) + (state . blur_rect . x - src . bounds . x);

	// Compute the src ptr/stride in pixels.
	uint32_t t_src_stride, *t_src_pixels;
	t_src_stride = src . stride / 4;
	t_src_pixels = (uint4 *)src . bits + t_src_stride * (state . region . y - src . bounds . y) + (state . region . x - src . bounds . x);

	// Compute the pre-multiplied color.
	uint32_t t_color;
	t_color = packed_scale_bounded(state . color | 0xff000000, state . color >> 24);

	// Ensure the color format is correct for the target context (Android is 0xAABBGGRR rather than 0xAARRGGBB)
#ifdef _ANDROID_MOBILE
	t_color = (t_color & 0xff00ff00) | ((t_color & 0x00ff0000) >> 16) | ((t_color & 0x000000ff) << 16);
#endif
	
	// Allocate a run of mask pixels
	uint8_t *t_mask_pixels;
	t_mask_pixels = new uint8_t[dst . bounds . width];

	// Calculate the attenuation bounds
	int32_t t_left, t_top, t_right, t_bottom, t_count;
	t_left = MCU_max(state . region . x, src . bounds . x) - state . region . x;
	t_top = src . bounds . y - state . region . y;
	t_right = MCU_min(state . region . x + state . region . width, src . bounds . x + src . bounds . width) - state . region . x;
	t_bottom = src . bounds . y + src . bounds . height - state . region . y;
	t_count = MCU_max(t_right - t_left, 0);

	// Initialize the blur
	MCBitmapEffectBlurRef t_blur;
	MCBitmapEffectBlurParameters t_blur_params;
	t_blur_params . radius = state . blur_size;
	t_blur_params . spread = state . blur_spread;
	t_blur_params . filter = state . blur_filter;

	if (MCBitmapEffectBlurBegin(t_blur_params, src . bounds, state . blur_rect, t_blur_src_pixels, t_blur_src_stride, t_blur))
	{
		for(int32_t y = 0; y < state . region . height; y++, t_dst_pixels += t_dst_stride, t_src_pixels += t_src_stride)
		{
			// Fetch the next line of blur
			MCBitmapEffectBlurContinue(t_blur, t_mask_pixels);

			// Attenuate the mask pixels appropriately.
			if (state . attenuate && y >= t_top && y < t_bottom)
				state . attenuate(t_mask_pixels + t_left, t_src_pixels + t_left, t_count);

			// Composite the src with dst
			state . composite(t_dst_pixels, t_color, t_mask_pixels, state . region . width);
		}
		MCBitmapEffectBlurEnd(t_blur);
	}

	delete[] t_mask_pixels;
}

static void MCShadowEffectRender(MCShadowEffect* self, bool p_inner, const MCRectangle& p_shape, MCBitmapEffectLayer& dst, MCBitmapEffectLayer& src)
{
	// We fill in this structure and defer to the main rendering routine.
	MCBitmapEffectRenderState t_state;

	// Compute the displacement of the shadow from the shape
	int32_t t_delta_x, t_delta_y;
	t_delta_x = (int32_t)floor(0.5 + self -> distance * cos(self -> angle * M_PI / 180.0));
	t_delta_y = (int32_t)floor(0.5 + self -> distance * sin(self -> angle * M_PI / 180.0));

	if (p_inner)
		t_state . region = MCU_intersect_rect(dst . bounds, p_shape);
	else
		t_state . region = MCU_intersect_rect(dst . bounds, MCU_offset_rect(MCU_reduce_rect(p_shape, -(signed)self -> size), t_delta_x, t_delta_y));

	t_state . blur_filter = (MCBitmapEffectFilter)self -> filter;
	t_state . blur_size = self -> size;
	t_state . blur_spread = self -> spread;
	t_state . blur_rect = MCU_offset_rect(t_state . region, -t_delta_x, -t_delta_y);

	t_state . color = self -> color;

	if (p_inner)
		t_state . attenuate = MCBitmapEffectAttenuateInvertedInner;
	else if (self -> knockout)
		t_state . attenuate = MCBitmapEffectAttenuateOuter;
	else
		t_state . attenuate = NULL;

	t_state . composite = MCBitmapEffectChooseCompositer((MCBitmapEffectBlendMode)self -> blend_mode, dst . has_alpha);

	MCBitmapEffectRender(t_state, dst, src);
}

static void MCGlowEffectRender(MCGlowEffect* self, bool p_inner, const MCRectangle& p_shape, MCBitmapEffectLayer& p_dst, MCBitmapEffectLayer& p_src)
{
	// We fill in this structure and defer to the main rendering routine.
	MCBitmapEffectRenderState t_state;

	// There is nothing to do if the glow radius is 0
	if (self -> size == 0)
		return;

	if (p_inner)
		t_state . region = MCU_intersect_rect(p_shape, p_dst . bounds);
	else
		t_state . region = MCU_intersect_rect(MCU_reduce_rect(p_shape, -(signed)self -> size), p_dst . bounds);

	t_state . blur_filter = (MCBitmapEffectFilter)self -> filter;
	t_state . blur_size = self -> size;
	t_state . blur_spread = self -> spread;
	t_state . blur_rect = t_state . region;

	t_state . color = self -> color;

	if (p_inner)
	{
		if (self -> source == kMCBitmapEffectSourceEdge)
			t_state . attenuate = MCBitmapEffectAttenuateInvertedInner;
		else
			t_state . attenuate = MCBitmapEffectAttenuateInner;
	}
	else
		t_state . attenuate = NULL;//MCBitmapEffectAttenuateOuter;
	
	t_state . composite = MCBitmapEffectChooseCompositer((MCBitmapEffectBlendMode)self -> blend_mode, p_dst . has_alpha);

	MCBitmapEffectRender(t_state, p_dst, p_src);
}

static void MCColorOverlayEffectRender(MCLayerEffect *self, const MCRectangle& p_shape, MCBitmapEffectLayer& p_dst, MCBitmapEffectLayer& p_src)
{
	MCBitmapEffectRenderState t_state;
	t_state . region = MCU_intersect_rect(p_shape, p_dst . bounds);
	t_state . blur_filter = kMCBitmapEffectFilterOnePassBox;
	t_state . blur_size = 0;
	t_state . blur_spread = 0;
	t_state . blur_rect = t_state . region;
	t_state . color = self -> color;
	t_state . attenuate = NULL;
	t_state . composite = MCBitmapEffectChooseCompositer((MCBitmapEffectBlendMode)self -> blend_mode, p_dst . has_alpha);
	MCBitmapEffectRender(t_state, p_dst, p_src);
}

extern void surface_combine_blendSrcOver(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint1 opacity);
void MCFilterEffectRender(MCBitmapEffect* self, const MCRectangle& p_shape, MCBitmapEffectLayer& p_dst, MCBitmapEffectLayer& p_src)
{
	// Compute the output region - the output rect is goverened by the shape
	// and what bits we have in the src and dst.
	MCRectangle t_region;
	t_region = MCU_intersect_rect(p_shape, p_dst . bounds);

	surface_combine_blendSrcOver(
		(char *)p_dst . bits + (t_region . y - p_dst . bounds . y) * p_dst . stride + (t_region . x - p_dst . bounds . x) * 4, p_dst . stride,
		(char *)p_src . bits + (t_region . y - p_src . bounds . y) * p_src . stride + (t_region . x - p_src . bounds . x) * 4, p_src . stride,
		t_region . width, t_region . height, 255);
}

// The order of the layers we render are as follows:
//   1) Drop Shadow
//   2) Image
//   3) Inner Shadow
//   4) Inner/Outer Glows
//   5) Color Overlay
//
// This method assumes that the dst and src are set up correctly. That is that
// the 'dst' bounds represent the dirty area to be redrawn, and the 'src'
// bounds are as least as big as that returned by ComputeClip for the given
// 'dst' bounds.
//
void MCBitmapEffectsRender(MCBitmapEffectsRef self, const MCRectangle& shape, MCBitmapEffectLayer& dst, MCBitmapEffectLayer& src)
{
	// Render the drop shadow
	if ((self -> mask & kMCBitmapEffectTypeDropShadowBit) != 0)
		MCShadowEffectRender(&self -> effects[kMCBitmapEffectTypeDropShadow] . shadow, false, shape, dst, src);

	// Render the outer glow
	if ((self -> mask & kMCBitmapEffectTypeOuterGlowBit) != 0)
		MCGlowEffectRender(&self -> effects[kMCBitmapEffectTypeOuterGlow] . glow, false, shape, dst, src);

	// Render the image
	MCFilterEffectRender(NULL, shape, dst, src);

	// Render the inner shadow
	if ((self -> mask & kMCBitmapEffectTypeInnerShadowBit) != 0)
		MCShadowEffectRender(&self -> effects[kMCBitmapEffectTypeInnerShadow] . shadow, true, shape, dst, src);

	// Render the inner glow
	if ((self -> mask & kMCBitmapEffectTypeInnerGlowBit) != 0)
		MCGlowEffectRender(&self -> effects[kMCBitmapEffectTypeInnerGlow] . glow, true, shape, dst, src);

	// Render the color overlay
	if ((self -> mask & kMCBitmapEffectTypeColorOverlayBit) != 0)
		MCColorOverlayEffectRender(&self -> effects[kMCBitmapEffectTypeColorOverlay] . layer, shape, dst, src);
}
