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

inline void MCColorMatrix3x3GetElements(const MCColorMatrix3x3 &p_matrix, MCGFloat r_values[9])
{
	r_values[0] = p_matrix.m[0][0];
	r_values[1] = p_matrix.m[1][0];
	r_values[2] = p_matrix.m[2][0];
	r_values[3] = p_matrix.m[0][1];
	r_values[4] = p_matrix.m[1][1];
	r_values[5] = p_matrix.m[2][1];
	r_values[6] = p_matrix.m[0][2];
	r_values[7] = p_matrix.m[1][2];
	r_values[8] = p_matrix.m[2][2];
}

//////////

bool MCColorTransformLinearRGBToXYZ(const MCColorVector2 &p_white, const MCColorVector2 &p_red, const MCColorVector2 &p_green, const MCColorVector2 &p_blue,
									MCColorVector3 &r_white, MCColorMatrix3x3 &r_matrix);

////////////////////////////////////////////////////////////////////////////////
