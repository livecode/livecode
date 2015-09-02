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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

struct __MCStream
{
	const MCStreamCallbacks *callbacks;
};

////////////////////////////////////////////////////////////////////////////////

struct __MCMemoryInputStream
{
	const void *buffer;
	size_t length;
	size_t pointer;
	size_t mark;
};

static void __MCMemoryInputStreamDestroy(MCStreamRef p_stream)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
}

static bool __MCMemoryInputStreamIsFinished(MCStreamRef p_stream, bool& r_finished)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
	r_finished = self -> pointer == self -> length;
	return true;
}

static bool __MCMemoryInputStreamGetAvailableForRead(MCStreamRef p_stream, size_t& r_amount)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
	r_amount = self -> length - self -> pointer;
	return true;
}

static bool __MCMemoryInputStreamRead(MCStreamRef p_stream, void *p_buffer, size_t p_amount)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
	if (p_amount > self -> length - self -> pointer)
		return false;
	MCMemoryCopy(p_buffer, (byte_t *)self -> buffer + self -> pointer, p_amount);
	self -> pointer += p_amount;
	return true;
}

static bool __MCMemoryInputStreamSkip(MCStreamRef p_stream, size_t p_amount)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
	if (p_amount > self -> length - self -> pointer)
		return false;
	self -> pointer += p_amount;
	return true;
}

static bool __MCMemoryInputStreamMark(MCStreamRef p_stream, size_t p_limit)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
	self -> mark = self -> pointer;
	return true;
}

static bool __MCMemoryInputStreamReset(MCStreamRef p_stream)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
	self -> pointer = self -> mark;
	return true;
}

static bool __MCMemoryInputStreamTell(MCStreamRef p_stream, filepos_t& r_position)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
	r_position = self -> pointer;
	return true;
}

static bool __MCMemoryInputStreamSeek(MCStreamRef p_stream, filepos_t p_position)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
	if (p_position < 0 || p_position > self -> length)
		return false;
	self -> pointer = (size_t)p_position;
	return true;
}

static MCStreamCallbacks kMCMemoryInputStreamCallbacks =
{
	__MCMemoryInputStreamDestroy,
	__MCMemoryInputStreamIsFinished,
	__MCMemoryInputStreamGetAvailableForRead,
	__MCMemoryInputStreamRead,
	nil,
	nil,
	__MCMemoryInputStreamSkip,
	__MCMemoryInputStreamMark,
	__MCMemoryInputStreamReset,
	__MCMemoryInputStreamTell,
	__MCMemoryInputStreamSeek,
};

bool MCMemoryInputStreamCreate(const void *p_block, size_t p_size, MCStreamRef& r_stream)
{
	MCStreamRef t_stream;
	if (!MCStreamCreate(&kMCMemoryInputStreamCallbacks, sizeof(__MCMemoryInputStream), t_stream))
		return false;

	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(t_stream);
	self -> buffer = p_block;
	self -> length = p_size;
	self -> mark = 0;
	self -> pointer = 0;

	r_stream = t_stream;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCMemoryOutputStreamCreate(MCStreamRef& r_stream)
{
	return false;
}

bool MCMemoryOutputStreamFinish(MCStreamRef stream, void*& r_buffer, size_t& r_size)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

static void __MCStreamDestroy(MCValueRef p_value)
{
	__MCStream *self;
	self = (__MCStream *)MCValueGetExtraBytesPtr(p_value);
	self -> callbacks -> destroy(self);
}

static bool __MCStreamCopy(MCValueRef p_value, bool p_release, MCValueRef& r_value)
{
	if (!p_release)
		MCValueRetain(p_value);
	r_value = p_value;
	return true;
}

static bool __MCStreamEqual(MCValueRef p_value, MCValueRef p_other_value)
{
	return p_value == p_other_value;
}

static hash_t __MCStreamHash(MCValueRef p_value)
{
	return (hash_t)p_value;
}

static bool __MCStreamDescribe(MCValueRef p_value, MCStringRef& r_desc)
{
	return false;
}

static MCValueCustomCallbacks kMCStreamCustomValueCallbacks =
{
	true,
	__MCStreamDestroy,
	__MCStreamCopy,
	__MCStreamEqual,
	__MCStreamHash,
	__MCStreamDescribe,
};

bool MCStreamCreate(const MCStreamCallbacks *p_callbacks, size_t p_extra_bytes, MCStreamRef& r_stream)
{
	__MCStream *self;
	if (!MCValueCreateCustom(&kMCStreamCustomValueCallbacks, sizeof(__MCStream) + p_extra_bytes, self))
		return false;

	self -> callbacks = p_callbacks;

	r_stream = self;
	
	return true;
}

const MCStreamCallbacks *MCStreamGetCallbacks(MCStreamRef self)
{
	return self -> callbacks;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStreamIsReadable(MCStreamRef self)
{
	return self -> callbacks -> read != nil;
}

bool MCStreamIsWritable(MCStreamRef self)
{
	return self -> callbacks -> write != nil;
}

bool MCStreamIsMarkable(MCStreamRef self)
{
	return self -> callbacks -> mark != nil;
}

bool MCStreamIsSeekable(MCStreamRef self)
{
	return self -> callbacks -> seek != nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStreamGetAvailableForRead(MCStreamRef self, size_t& r_available)
{
	if (self -> callbacks -> get_available_for_read == nil)
		return false;
	return self -> callbacks -> get_available_for_read(self, r_available);
}

bool MCStreamRead(MCStreamRef self, void *p_buffer, size_t p_amount)
{
	if (self -> callbacks -> read == nil)
		return false;
	return self -> callbacks -> read(self, p_buffer, p_amount);
}

bool MCStreamGetAvailableForWrite(MCStreamRef self, size_t& r_available)
{
	if (self -> callbacks -> get_available_for_write == nil)
		return false;
	return self -> callbacks -> get_available_for_write(self, r_available);
}

bool MCStreamWrite(MCStreamRef self, const void *p_buffer, size_t p_amount)
{
	if (self -> callbacks -> write == nil)
		return false;
	return self -> callbacks -> write(self, p_buffer, p_amount);
}

bool MCStreamSkip(MCStreamRef self, size_t p_amount)
{
	if (self -> callbacks -> skip != nil)
		return self -> callbacks -> skip(self, p_amount);
	if (self -> callbacks -> seek != nil)
	{
		filepos_t t_pos;
		if (!self -> callbacks -> tell(self, t_pos))
			return false;
		return self -> callbacks -> seek(self, t_pos + p_amount);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStreamMark(MCStreamRef self, size_t p_read_limit)
{
	if (self -> callbacks -> mark == nil)
		return false;
	return self -> callbacks -> mark(self, p_read_limit);
}

bool MCStreamReset(MCStreamRef self)
{
	if (self -> callbacks -> reset == nil)
		return false;
	return self -> callbacks -> reset(self);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStreamTell(MCStreamRef self, filepos_t& r_position)
{
	if (self -> callbacks -> tell == nil)
		return false;
	return self -> callbacks -> tell(self, r_position);
}

bool MCStreamSeek(MCStreamRef self, filepos_t p_position)
{
	if (self -> callbacks -> seek == nil)
		return false;
	return self -> callbacks -> seek(self, p_position);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStreamReadUInt8(MCStreamRef self, uint8_t& r_value)
{
	return MCStreamRead(self, &r_value, sizeof(uint8_t));
}

bool MCStreamReadUInt16(MCStreamRef self, uint16_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(uint16_t)))
	{
		r_value = MCSwapInt16NetworkToHost(r_value);
		return true;
	}
	return false;
}

bool MCStreamReadUInt32(MCStreamRef self, uint32_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(uint32_t)))
	{
		r_value = MCSwapInt32NetworkToHost(r_value);
		return true;
	}
	return false;
}

bool MCStreamReadUInt64(MCStreamRef self, uint64_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(uint64_t)))
	{
		r_value = MCSwapInt64NetworkToHost(r_value);
		return true;
	}
	return false;
}

bool MCStreamReadInt8(MCStreamRef self, int8_t& r_value)
{
	return MCStreamRead(self, &r_value, sizeof(int8_t));
}

bool MCStreamReadInt16(MCStreamRef self, int16_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(int16_t)))
	{
		r_value = (int16_t)MCSwapInt16NetworkToHost((uint16_t)r_value);
		return true;
	}
	return false;
}

bool MCStreamReadInt32(MCStreamRef self, int32_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(int32_t)))
	{
		r_value = (int32_t)MCSwapInt32NetworkToHost((uint32_t)r_value);
		return true;
	}
	return false;
}

bool MCStreamReadInt64(MCStreamRef self, int64_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(int64_t)))
	{
		r_value = (int64_t)MCSwapInt64NetworkToHost((uint64_t)r_value);
		return true;
	}
	return false;
}

bool MCStreamReadCompactUInt32(MCStreamRef stream, uint32_t& r_value);
bool MCStreamReadCompactUInt64(MCStreamRef stream, uint64_t& r_value);
bool MCStreamReadCompactSInt32(MCStreamRef stream, uint32_t& r_value);
bool MCStreamReadCompactSInt64(MCStreamRef stream, uint64_t& r_value);

bool MCStreamReadFloat(MCStreamRef stream, float& r_value);

bool MCStreamReadDouble(MCStreamRef stream, double& r_value)
{
	uint64_t t_bits;
	if (!MCStreamReadUInt64(stream, t_bits))
		return false;

	MCMemoryCopy(&r_value, &t_bits, sizeof(uint64_t));

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStreamReadBoolean(MCStreamRef stream, MCBooleanRef& r_boolean)
{
	uint8_t t_value;
	if (!MCStreamReadUInt8(stream, t_value))
		return false;

	if (t_value != 0)
		r_boolean = MCValueRetain(kMCTrue);
	else
		r_boolean = MCValueRetain(kMCFalse);

	return true;
}

bool MCStreamReadNumber(MCStreamRef stream, MCNumberRef& r_number)
{
	uint8_t t_tag;
	if (!MCStreamReadUInt8(stream, t_tag))
		return false;

	if (t_tag == 0)
	{
		int32_t t_value;
		if (!MCStreamReadInt32(stream, t_value))
			return false;
		return MCNumberCreateWithInteger(t_value, r_number);
	}

	double t_value;
	if (!MCStreamReadDouble(stream, t_value))
		return false;
	return MCNumberCreateWithReal(t_value, r_number);
}

bool MCStreamReadName(MCStreamRef stream, MCNameRef& r_name)
{
	MCStringRef t_string;
	if (!MCStreamReadString(stream, t_string))
		return false;

	return MCNameCreateAndRelease(t_string, r_name);
}

bool MCStreamReadString(MCStreamRef stream, MCStringRef& r_string)
{
	uint32_t t_length;
	if (!MCStreamReadUInt32(stream, t_length))
		return false;

	if (t_length == 0)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}

	MCAutoNativeCharArray t_chars;
	if (!t_chars . New(t_length))
		return false;

	if (!MCStreamRead(stream, t_chars . Chars(), t_chars . CharCount()))
		return false;

	return t_chars . CreateStringAndRelease(r_string);
}

bool MCStreamReadArray(MCStreamRef stream, MCArrayRef& r_array)
{
	uint32_t t_count;
	if (!MCStreamReadUInt32(stream, t_count))
		return false;

	if (t_count == 0)
	{
		r_array = MCValueRetain(kMCEmptyArray);
		return true;
	}

	MCArrayRef t_array;
	if (!MCArrayCreateMutable(t_array))
		return false;

	while(t_count > 0)
	{
		MCNewAutoNameRef t_name;
		if (!MCStreamReadName(stream, &t_name))
			break;

		MCAutoValueRef t_value;
		if (!MCStreamReadValue(stream, &t_value))
			break;

		if (!MCArrayStoreValue(t_array, true, *t_name, *t_value))
			break;

		t_count -= 1;
	}

	if (t_count != 0)
	{
		MCValueRelease(t_array);
		return false;
	}

	return MCArrayCopyAndRelease(t_array, r_array);
}

bool MCStreamReadSet(MCStreamRef stream, MCSetRef& r_set)
{
	uint32_t t_length;
	if (!MCStreamReadUInt32(stream, t_length))
		return false;

	if (t_length == 0)
	{
		r_set = MCValueRetain(kMCEmptySet);
		return true;
	}

	uindex_t *t_limbs;
	if (!MCMemoryNewArray(t_length, t_limbs))
		return false;

	if (!MCStreamRead(stream, t_limbs, sizeof(uint32_t) * t_length) ||
		!MCSetCreateWithLimbsAndRelease(t_limbs, t_length, r_set))
	{
		MCMemoryDeleteArray(t_limbs);
		return false;
	}

	return true;
}

bool MCStreamReadValue(MCStreamRef stream, MCValueRef& r_value)
{
	uint8_t t_tag;
	if (!MCStreamReadUInt8(stream, t_tag))
		return false;

	switch(t_tag)
	{
	case 0:
		r_value = MCValueRetain(kMCNull);
		return true;
	case 1:
		r_value = MCValueRetain(kMCTrue);
		return true;
	case 2:
		r_value = MCValueRetain(kMCFalse);
		return true;
	case 3:
	{
		int32_t t_int;
		if (!MCStreamReadInt32(stream, t_int))
			break;
		MCNumberRef t_value;
		if (MCNumberCreateWithInteger(t_int, t_value))
			return r_value = t_value, true;
		break;
	}
	case 4:
	{
		MCStringRef t_value;
		if (MCStreamReadString(stream, t_value))
			return r_value = t_value, true;
		break;
	}
	case 5:
	{
		MCNameRef t_value;
		if (MCStreamReadName(stream, t_value))
			return r_value = t_value, true;
		break;
	}
	case 6:
	{
		MCSetRef t_value;
		if (MCStreamReadSet(stream, t_value))
			return r_value = t_value, true;
		break;
	}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////
