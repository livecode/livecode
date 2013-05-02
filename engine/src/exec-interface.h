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

static void MCInterfaceVisualEffectArgumentCopy(MCExecContext& ctxt, MCInterfaceVisualEffectArgument p_source, MCInterfaceVisualEffectArgument& r_target);
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
static void MCInterfaceNamedColorInit(MCExecContext& ctxt, MCInterfaceNamedColor& r_output);
void MCInterfaceNamedColorFree(MCExecContext& ctxt, MCInterfaceNamedColor& p_input);
static void MCInterfaceNamedColorCopy(MCExecContext& ctxt, const MCInterfaceNamedColor& p_source, MCInterfaceNamedColor& r_target);
static bool MCInterfaceNamedColorIsEqualTo(const MCInterfaceNamedColor& p_left, const MCInterfaceNamedColor& p_right);

////////////////////////////////////////////////////////////////////////////////

void set_interface_color(MCColor& x_color, MCStringRef& x_color_name, const MCInterfaceNamedColor& p_color);
void get_interface_color(const MCColor& p_color, MCStringRef p_color_name, MCInterfaceNamedColor& r_color);

////////////////////////////////////////////////////////////////////////////////

struct MCInterfaceTextStyle
{
	uint2 style;
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceEncodingElementInfo[] =
{	
	{ MCnativestring, 0, true },
	{ MCunicodestring, 1, true },
	{ MCmixedstring, 2, true },
};

static MCExecEnumTypeInfo _kMCInterfaceEncodingTypeInfo =
{
	"Interface.Encoding",
	sizeof(_kMCInterfaceEncodingElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceEncodingElementInfo
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

//////////

struct MCMultimediaQTVRConstraints
{
	real4 minpan, maxpan;
	real4 mintilt, maxtilt;
	real4 minzoom, maxzoom;
};