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

////////////////////////////////////////////////////////////////////////////////

struct MCColorVector2
{
	MCGFloat x;
	MCGFloat y;
};

struct MCColorVector3
{
	MCGFloat x;
	MCGFloat y;
	MCGFloat z;
};

struct MCColorMatrix3x3
{
	MCGFloat m[3][3];
};

inline MCColorVector2 MCColorVector2Make(MCGFloat x, MCGFloat y)
{
	MCColorVector2 t_vector;
	t_vector.x = x;
	t_vector.y = y;
	return t_vector;
}

inline MCColorVector3 MCColorVector3Make(MCGFloat x, MCGFloat y, MCGFloat z)
{
	MCColorVector3 t_vector;
	t_vector.x = x;
	t_vector.y = y;
	t_vector.z = z;
	return t_vector;
}

//////////

bool MCColorTransformLinearRGBToXYZ(const MCColorVector2 &p_white, const MCColorVector2 &p_red, const MCColorVector2 &p_green, const MCColorVector2 &p_blue,
									MCColorVector3 &r_white, MCColorMatrix3x3 &r_matrix);

////////////////////////////////////////////////////////////////////////////////
