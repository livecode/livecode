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

#ifdef _MACOSX
#include <CoreFoundation/CoreFoundation.h>
#endif

////////////////////////////////////////////////////////////////////////////////

struct MCBinaryEncoder
{
	uint32_t capacity;
	uint32_t frontier;
	void *buffer;
};

bool MCBinaryEncoderCreate(MCBinaryEncoder*& r_encoder)
{
	if (!MCMemoryNew(r_encoder))
		return false;
	
	r_encoder -> capacity = 0;
	r_encoder -> frontier = 0;
	r_encoder -> buffer = nil;
	
	return true;
}

void MCBinaryEncoderDestroy(MCBinaryEncoder *self)
{
	if (self == nil)
		return;
	
	MCMemoryDeallocate(self -> buffer);
	MCMemoryDelete(self);
}

void MCBinaryEncoderBorrow(MCBinaryEncoder *self, void*& r_buffer, uint32_t& r_buffer_length)
{
	r_buffer = self -> buffer;
	r_buffer_length = self -> frontier;
}

bool MCBinaryEncoderWriteBytes(MCBinaryEncoder *self, const void *p_data, uint32_t p_length)
{
	if (self -> frontier + p_length > self -> capacity)
	{
		uint32_t t_new_capacity;
		t_new_capacity = (self -> frontier + p_length + 255) & ~255;
		if (!MCMemoryReallocate(self -> buffer, t_new_capacity, self -> buffer))
			return false;
			
		self -> capacity = t_new_capacity;
	}
	
	MCMemoryCopy((char *)self -> buffer + self -> frontier, p_data, p_length);
	self -> frontier += p_length;
	
	return true;
}

bool MCBinaryEncoderWriteInt32(MCBinaryEncoder *self, int32_t p_value)
{
	return MCBinaryEncoderWriteUInt32(self, (uint32_t)p_value);
}

bool MCBinaryEncoderWriteUInt32(MCBinaryEncoder *self, uint32_t p_value)
{
	uint32_t t_value;
	t_value = MCByteSwappedFromHost32(p_value);
	return MCBinaryEncoderWriteBytes(self, &t_value, sizeof(uint32_t));
}

bool MCBinaryEncoderWriteCBlob(MCBinaryEncoder *self, const void *p_data, uint32_t p_length)
{
	if (!MCBinaryEncoderWriteUInt32(self, p_length))
		return false;
	return MCBinaryEncoderWriteBytes(self, p_data, p_length);
}

bool MCBinaryEncoderWriteCString(MCBinaryEncoder *self, const char *p_cstring)
{
	uint32_t t_length;
	t_length = MCCStringLength(p_cstring);
	if (!MCBinaryEncoderWriteUInt32(self, t_length))
		return false;
	return MCBinaryEncoderWriteBytes(self, p_cstring, t_length);
}

#ifdef _MACOSX
bool MCBinaryEncoderWriteCFData(MCBinaryEncoder *self, CFDataRef cfdata)
{
	return MCBinaryEncoderWriteCBlob(self, CFDataGetBytePtr(cfdata), CFDataGetLength(cfdata));
}

bool MCBinaryEncoderWriteCFString(MCBinaryEncoder *self, CFStringRef p_cfstring)
{
	CFDataRef t_data;
	t_data = CFStringCreateExternalRepresentation(kCFAllocatorDefault, p_cfstring, kCFStringEncodingUTF8, 0);
	if (t_data == nil)
		return false;
	
	bool t_success;
	t_success = MCBinaryEncoderWriteCFData(self, t_data);
	CFRelease(t_data);
	
	return t_success;
}
#endif

////////////////////////////////////////////////////////////////////////////////

struct MCBinaryDecoder
{
	uint32_t length;
	uint32_t frontier;
	const void *buffer;
};

bool MCBinaryDecoderCreate(const void *p_buffer, uint32_t p_length, MCBinaryDecoder*& r_decoder)
{
	MCBinaryDecoder *self;
	if (!MCMemoryNew(self))
		return false;
	
	self -> length = p_length;
	self -> buffer = p_buffer;
	self -> frontier = 0;
	
	r_decoder = self;
	
	return true;
}

void MCBinaryDecoderDestroy(MCBinaryDecoder* self)
{
	if (self == nil)
		return;
	
	MCMemoryDelete(self);
}

bool MCBinaryDecoderReadBytes(MCBinaryDecoder* self, void *p_buffer, uint32_t p_length)
{
	if (self -> frontier + p_length > self -> length)
		return false;
	MCMemoryCopy(p_buffer, (char *)self -> buffer + self -> frontier, p_length);
	self -> frontier += p_length;
	return true;
}

bool MCBinaryDecoderReadUInt32(MCBinaryDecoder* self, uint32_t& r_value)
{
	uint32_t t_value;
	if (!MCBinaryDecoderReadBytes(self, &t_value, 4))
		return false;
	r_value = MCByteSwappedToHost32(t_value);
	return true;
}

bool MCBinaryDecoderReadInt32(MCBinaryDecoder* self, int32_t& r_value)
{
	uint32_t t_value;
	if (!MCBinaryDecoderReadBytes(self, &t_value, 4))
		return false;
	r_value = (int32_t)MCByteSwappedToHost32(t_value);
	return true;
}

bool MCBinaryDecoderReadCString(MCBinaryDecoder *self, char *&r_cstring)
{
	uint32_t t_length;
	if (!MCBinaryDecoderReadUInt32(self, t_length))
		return false;
	if (self -> frontier + t_length > self -> length)
		return false;

	char *t_string = nil;
	if (t_length > 0)
	{
		if (!MCCStringCloneSubstring((char*)self->buffer + self->frontier, t_length, t_string))
			return false;
		self->frontier += t_length;
	}
	r_cstring = t_string;
	return true;
}

#ifdef _MACOSX
bool MCBinaryDecoderReadCFString(MCBinaryDecoder* self, CFStringRef& r_cfstring)
{
	uint32_t t_length;
	if (!MCBinaryDecoderReadUInt32(self, t_length))
		return false;
	
	if (self -> frontier + t_length > self -> length)
		return false;
	
	CFDataRef t_data;
	t_data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (UInt8 *)self -> buffer + self -> frontier, t_length, kCFAllocatorNull);
	if (t_data == nil)
		return false;
	
	CFStringRef t_string;
	t_string = CFStringCreateFromExternalRepresentation(kCFAllocatorDefault, t_data, kCFStringEncodingUTF8);
	CFRelease(t_data);
	
	if (t_string != nil)
	{
		self -> frontier += t_length;
		r_cfstring = t_string;
		return true;
	}
	
	return false;
}
#endif

////////////////////////////////////////////////////////////////////////////////
