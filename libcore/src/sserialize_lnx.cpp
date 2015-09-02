/* Copyright (C) 2009-2015 LiveCode Ltd.

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

#include "core.h"

#include "sserialize_lnx.h"

////////////////////////////////////////////////////////////////////////////////

bool MCLinuxPageSetupEncode(const MCLinuxPageSetup& setup, void*& r_data, uint32_t& r_data_size)
{
	bool t_success;
	t_success = true;

	MCBinaryEncoder *t_encoder;
	t_encoder = nil;
	if (t_success)
		t_success = MCBinaryEncoderCreate(t_encoder);

	if (t_success)
		t_success = 
			MCBinaryEncoderWriteUInt32(t_encoder, 0) &&
			MCBinaryEncoderWriteInt32(t_encoder, setup . paper_width) &&
			MCBinaryEncoderWriteInt32(t_encoder, setup . paper_height) &&
			MCBinaryEncoderWriteInt32(t_encoder, setup . left_margin) &&
			MCBinaryEncoderWriteInt32(t_encoder, setup . top_margin) &&
			MCBinaryEncoderWriteInt32(t_encoder, setup . right_margin) &&
			MCBinaryEncoderWriteInt32(t_encoder, setup . bottom_margin) &&
			MCBinaryEncoderWriteUInt32(t_encoder, setup . orientation);

	if (t_success)
	{
		void *t_data;
		uint32_t t_data_size;
		MCBinaryEncoderBorrow(t_encoder, t_data, t_data_size);
		t_success = MCMemoryAllocateCopy(t_data, t_data_size, r_data);
		if (t_success)
			r_data_size = t_data_size;
	}

	MCBinaryEncoderDestroy(t_encoder);

	return t_success;
}

bool MCLinuxPageSetupDecode(const void *p_data, uint32_t p_data_size, MCLinuxPageSetup& setup)
{
	MCBinaryDecoder *t_decoder;

	if (!MCBinaryDecoderCreate(p_data, p_data_size, t_decoder))
		return false;

	bool t_success;
	uint32_t version;
	t_success =
		MCBinaryDecoderReadUInt32(t_decoder, version) &&
		MCBinaryDecoderReadInt32(t_decoder, setup . paper_width) &&
		MCBinaryDecoderReadInt32(t_decoder, setup . paper_height) &&
		MCBinaryDecoderReadInt32(t_decoder, setup . left_margin) &&
		MCBinaryDecoderReadInt32(t_decoder, setup . top_margin) &&
		MCBinaryDecoderReadInt32(t_decoder, setup . right_margin) &&
		MCBinaryDecoderReadInt32(t_decoder, setup . bottom_margin) &&
		MCBinaryDecoderReadUInt32(t_decoder, setup . orientation);

	MCBinaryDecoderDestroy(t_decoder);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCLinuxPrintSetupEncode(const MCLinuxPrintSetup& setup, void*& r_data, uint32_t& r_data_size)
{
	bool t_success;
	t_success = true;

	MCBinaryEncoder *t_encoder;
	t_encoder = nil;
	if (t_success)
		t_success = MCBinaryEncoderCreate(t_encoder);

	if (t_success)
		t_success = true;

	if (t_success)
	{
		void *t_data;
		uint32_t t_data_size;
		MCBinaryEncoderBorrow(t_encoder, t_data, t_data_size);
		t_success = MCMemoryAllocateCopy(t_data, t_data_size, r_data);
		if (t_success)
			r_data_size = t_data_size;
	}

	MCBinaryEncoderDestroy(t_encoder);

	return t_success;
}

bool MCLinuxPrintSetupDecode(const void *p_data, uint32_t p_data_size, MCLinuxPrintSetup& setup)
{
	MCBinaryDecoder *t_decoder;

	if (!MCBinaryDecoderCreate(p_data, p_data_size, t_decoder))
		return false;

	bool t_success;
	t_success = true;

	MCBinaryDecoderDestroy(t_decoder);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
