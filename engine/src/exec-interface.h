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

#ifndef EXEC_INTERFACE_H
#define EXEC_INTERFACE_H

#include "mctristate.h"

////////////////////////////////////////////////////////////////////////////////

struct MCInterfaceImagePaletteSettings
{
	MCImagePaletteType type;
	union
	{
		struct
		{
			MCColor *colors;
			uindex_t count;
		} custom;
		struct
		{
			uinteger_t palette_size;
		} optimal;
	};
};

void MCInterfaceImagePaletteSettingsFree(MCExecContext& ctxt, MCInterfaceImagePaletteSettings& p_settings);

//////////

struct MCInterfaceVisualEffectArgument
{
	MCStringRef key;
	MCStringRef value;
};

void MCInterfaceVisualEffectArgumentCopy(MCExecContext& ctxt, MCInterfaceVisualEffectArgument p_source, MCInterfaceVisualEffectArgument& r_target);
void MCInterfaceVisualEffectArgumentFree(MCExecContext& ctxt, MCInterfaceVisualEffectArgument& p_arg);

//////////

struct MCInterfaceVisualEffect
{
	MCStringRef name;
	MCStringRef sound;
	MCInterfaceVisualEffectArgument *arguments;
	uindex_t nargs;

	Visual_effects type;
	Visual_effects direction;
	Visual_effects speed;
	Visual_effects image;
};

void MCInterfaceVisualEffectFree(MCExecContext& ctxt, MCInterfaceVisualEffect& p_effect);

//////////

void MCInterfaceMakeCustomImagePaletteSettings(MCExecContext& ctxt, MCColor *colors, uindex_t color_count, MCInterfaceImagePaletteSettings& r_settings);
void MCInterfaceMakeOptimalImagePaletteSettings(MCExecContext& ctxt, integer_t *count, MCInterfaceImagePaletteSettings& r_settings);
void MCInterfaceMakeWebSafeImagePaletteSettings(MCExecContext& ctxt, MCInterfaceImagePaletteSettings& r_settings);

//////////

void MCInterfaceMakeVisualEffect(MCExecContext& ctxt, MCStringRef name, MCStringRef sound, MCInterfaceVisualEffectArgument *effect_args, uindex_t count, Visual_effects type, Visual_effects direction, Visual_effects speed, Visual_effects image, MCInterfaceVisualEffect& r_effect);
void MCInterfaceMakeVisualEffectArgument(MCExecContext& ctxt, MCStringRef p_value, MCStringRef p_key, bool p_has_id, MCInterfaceVisualEffectArgument& r_arg);

////////////////////////////////////////////////////////////////////////////////

struct MCInterfaceNamedColor
{
	MCStringRef name;
	MCColor color;
};

void MCInterfaceNamedColorParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceNamedColor& r_output);
void MCInterfaceNamedColorFormat(MCExecContext& ctxt, const MCInterfaceNamedColor& p_input, MCStringRef& r_output);
void MCInterfaceNamedColorInit(MCExecContext& ctxt, MCInterfaceNamedColor& r_output);
void MCInterfaceNamedColorFree(MCExecContext& ctxt, MCInterfaceNamedColor& p_input);
void MCInterfaceNamedColorCopy(MCExecContext& ctxt, const MCInterfaceNamedColor& p_source, MCInterfaceNamedColor& r_target);
bool MCInterfaceNamedColorIsEqualTo(const MCInterfaceNamedColor& p_left, const MCInterfaceNamedColor& p_right);

////////////////////////////////////////////////////////////////////////////////

void set_interface_color(MCColor& x_color, MCStringRef& x_color_name, const MCInterfaceNamedColor& p_color);
void get_interface_color(const MCColor& p_color, MCStringRef p_color_name, MCInterfaceNamedColor& r_color);

////////////////////////////////////////////////////////////////////////////////

struct MCInterfaceTextStyle
{
	uint2 style;
};

////////////////////////////////////////////////////////////////////////////////

struct MCInterfaceLayer
{
	uint4 layer;
};

void MCInterfaceLayerParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceLayer& r_output);
void MCInterfaceLayerFormat(MCExecContext& ctxt, const MCInterfaceLayer& p_input, MCStringRef& r_output);
void MCInterfaceLayerFree(MCExecContext& ctxt, MCInterfaceLayer& p_input);

//////////

enum MCInterfaceMarginsType
{
    kMCInterfaceMarginsTypeSingle,
    kMCInterfaceMarginsTypeQuadruple
    
};

struct MCInterfaceMargins
{
    MCInterfaceMarginsType type;
    
    union
    {
        int2 margin;
        int2 margins[4];
    };
};

//////////

struct MCMultimediaTrack
{
	uint4 id;
	MCStringRef name;
	uint4 offset;
	uint4 duration;
};

struct MCMultimediaTrackList
{
	MCMultimediaTrack track;
	MCMultimediaTrackList *next;
};

enum MCMultimediaQTVRHotSpotType
{
	kMCQTVRHotSpotLinkType,
	kMCQTVRHotSpotURLType,
	kMCQTVRHotSpotUndefinedType,
};

enum MCMultimediaQTVRNodeType
{
	kMCQTVRNodePanoramaType,
	kMCQTVRNodeObjectType,
};

struct MCMultimediaQTVRNode
{
	uint2 id;
	MCMultimediaQTVRNodeType type;
};

struct MCMultimediaQTVRHotSpot
{
	uint2 id;
	MCMultimediaQTVRHotSpotType type;
};

//////////

// SN-2014-06-25: [[ PlatformPlayer ]]
// MCMultimediaQTVRConstraints must follow the definitions of MCPlatformPlayerQTVRConstraints
// SN-2014-07-03: [[ PlatformPlayer ]]
// Now having a common interface, both former and new players must have the same constraints struct
struct MCMultimediaQTVRConstraints
{
	real4 minpan, maxpan;
	real4 mintilt, maxtilt;
	real4 minzoom, maxzoom;
};

//////////

struct MCInterfaceTriState
{
    MCTristate value;
};

void MCInterfaceTriStateParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceTriState& r_output);
void MCInterfaceTriStateFormat(MCExecContext& ctxt, const MCInterfaceTriState& p_input, MCStringRef& r_output);
void MCInterfaceTriStateFree(MCExecContext& ctxt, MCInterfaceTriState& p_input);

//////////

struct MCInterfaceFieldRange
{
    uint32_t start;
    uint32_t end;
};

struct MCInterfaceFieldRanges
{
    MCInterfaceFieldRange *ranges;
    uindex_t count;
};

//////////

struct MCInterfaceStackFileVersion
{
    uint4 version;
};

void MCInterfaceStackFileVersionParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceStackFileVersion& r_output);
void MCInterfaceStackFileVersionFormat(MCExecContext& ctxt, const MCInterfaceStackFileVersion& p_input, MCStringRef& r_output);
void MCInterfaceStackFileVersionFree(MCExecContext& ctxt, MCInterfaceStackFileVersion& p_input);

//////////

void MCInterfaceTabStopsParse(MCExecContext& ctxt, bool p_is_relative, uinteger_t* p_tabs, uindex_t p_count, uint2*& r_new_stops, uindex_t& r_new_stop_count);

//////////

enum MCInterfaceKeyboardType : unsigned
{
    kMCInterfaceKeyboardTypeNone,
    kMCInterfaceKeyboardTypeDefault,
    kMCInterfaceKeyboardTypeAlphabet,
    kMCInterfaceKeyboardTypeNumeric,
    kMCInterfaceKeyboardTypeDecimal,
    kMCInterfaceKeyboardTypeNumber,
    kMCInterfaceKeyboardTypePhone,
    kMCInterfaceKeyboardTypeEmail,
    kMCInterfaceKeyboardTypeUrl,
    kMCInterfaceKeyboardTypeContact
};


enum MCInterfaceReturnKeyType : unsigned
{
    kMCInterfaceReturnKeyTypeNone,
    kMCInterfaceReturnKeyTypeDefault,
    kMCInterfaceReturnKeyTypeGo,
    kMCInterfaceReturnKeyTypeGoogle,
    kMCInterfaceReturnKeyTypeJoin,
    kMCInterfaceReturnKeyTypeNext,
    kMCInterfaceReturnKeyTypeRoute,
    kMCInterfaceReturnKeyTypeSearch,
    kMCInterfaceReturnKeyTypeSend,
    kMCInterfaceReturnKeyTypeYahoo,
    kMCInterfaceReturnKeyTypeDone,
    kMCInterfaceReturnKeyTypeEmergencyCall
};

#endif // EXEC_INTERFACE_H

//////////
