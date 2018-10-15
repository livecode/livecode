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

struct __MCStreamImpl
{
	const MCStreamCallbacks *callbacks;
};

MC_DLLEXPORT_DEF MCTypeInfoRef kMCStreamTypeInfo;

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCStreamTypeInfo() { return kMCStreamTypeInfo; }

static inline __MCStreamImpl &__MCStreamGet(MCStreamRef p_stream)
{
	return *(__MCStreamImpl*)MCValueGetExtraBytesPtr(p_stream);
}

static inline const MCStreamCallbacks *__MCStreamCallbacks(MCStreamRef self)
{
	return __MCStreamGet(self).callbacks;
}

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
	/* Do nothing because the memory input stream doesn't own its buffer */
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
    return MCNarrow(self->pointer, r_position);
}

static bool __MCMemoryInputStreamSeek(MCStreamRef p_stream, filepos_t p_position)
{
	__MCMemoryInputStream *self;
	self = (__MCMemoryInputStream *)MCStreamGetExtraBytesPtr(p_stream);
    return MCNarrow(p_position, self->pointer);
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

MC_DLLEXPORT_DEF
bool MCMemoryInputStreamCreate(const void *p_block, size_t p_size, MCStreamRef& r_stream)
{
	MCAssert(nil != p_block || 0 == p_size);

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

struct __MCMemoryOutputStream
{
    void *buffer;
	size_t length;
	size_t capacity;
};

static void __MCMemoryOutputStreamDestroy(MCStreamRef p_stream)
{
	__MCMemoryOutputStream *self;
	self = (__MCMemoryOutputStream *)MCStreamGetExtraBytesPtr(p_stream);
    
    free(self -> buffer);
}

static bool __MCMemoryOutputStreamIsFinished(MCStreamRef p_stream, bool& r_finished)
{
    r_finished = false;
	return true;
}

static bool __MCMemoryOutputStreamGetAvailableForWrite(MCStreamRef p_stream, size_t& r_amount)
{
	r_amount = SIZE_MAX;
	return true;
}

static bool __MCMemoryOutputStreamWrite(MCStreamRef p_stream, const void *p_buffer, size_t p_amount)
{
	__MCMemoryOutputStream *self;
	self = (__MCMemoryOutputStream *)MCStreamGetExtraBytesPtr(p_stream);
	if (p_amount > self -> capacity - self -> length)
    {
        size_t t_new_capacity;
        t_new_capacity = (self -> length + p_amount + 65536) & ~65535;
        
        void *t_new_buffer;
        t_new_buffer = realloc(self -> buffer, t_new_capacity);
        if (t_new_buffer == nil)
            return false;
        
        self -> buffer = t_new_buffer;
        self -> capacity = t_new_capacity;
    }
	MCMemoryCopy((byte_t *)self -> buffer + self -> length, p_buffer, p_amount);
	self -> length += p_amount;
	return true;
}

static MCStreamCallbacks kMCMemoryOutputStreamCallbacks =
{
	__MCMemoryOutputStreamDestroy,
	__MCMemoryOutputStreamIsFinished,
	nil,
	nil,
	__MCMemoryOutputStreamGetAvailableForWrite,
	__MCMemoryOutputStreamWrite,
	nil,
	nil,
	nil,
	nil,
	nil,
};

MC_DLLEXPORT_DEF
bool MCMemoryOutputStreamCreate(MCStreamRef& r_stream)
{
	MCStreamRef t_stream;
	if (!MCStreamCreate(&kMCMemoryOutputStreamCallbacks, sizeof(__MCMemoryOutputStream), t_stream))
		return false;
    
	__MCMemoryOutputStream *self;
	self = (__MCMemoryOutputStream *)MCStreamGetExtraBytesPtr(t_stream);
	self -> buffer = nil;
	self -> length = 0;
	self -> capacity = 0;
    
	r_stream = t_stream;
    
	return true;
}

MC_DLLEXPORT_DEF
bool MCMemoryOutputStreamFinish(MCStreamRef p_stream, void*& r_buffer, size_t& r_size)
{
	__MCMemoryOutputStream *self;
	self = (__MCMemoryOutputStream *)MCStreamGetExtraBytesPtr(p_stream);
    
    r_buffer = realloc(self -> buffer, self -> length);
    r_size = self -> length;
    
    self -> buffer = nil;
    self -> length = 0;
    self -> capacity = 0;
    
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static void __MCStreamDestroy(MCValueRef p_value)
{
	__MCStreamCallbacks((MCStreamRef)p_value)->destroy((MCStreamRef)p_value);
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
	return (hash_t) MCHashPointer (p_value);
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
	nil,
	nil,
};

static inline void __MCAssertIsStream(MCStreamRef ref)
{
	__MCValue *val = reinterpret_cast<__MCValue *>(ref);
	MCAssert(MCValueGetTypeInfo(val) == kMCStreamTypeInfo);
}

MC_DLLEXPORT_DEF
bool MCStreamCreate(const MCStreamCallbacks *p_callbacks, size_t p_extra_bytes, MCStreamRef& r_stream)
{
	MCStreamRef self;
	if (!MCValueCreateCustom(kMCStreamTypeInfo, sizeof(__MCStreamImpl) + p_extra_bytes, self))
		return false;

	__MCStreamGet(self).callbacks = p_callbacks;

	r_stream = self;
	
	return true;
}

MC_DLLEXPORT_DEF
const MCStreamCallbacks *MCStreamGetCallbacks(MCStreamRef self)
{
	__MCAssertIsStream(self);

	return __MCStreamCallbacks(self);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCStreamIsReadable(MCStreamRef self)
{
	__MCAssertIsStream(self);

	return __MCStreamCallbacks(self) -> read != nil;
}

MC_DLLEXPORT_DEF
bool MCStreamIsWritable(MCStreamRef self)
{
	__MCAssertIsStream(self);

	return __MCStreamCallbacks(self) -> write != nil;
}

MC_DLLEXPORT_DEF
bool MCStreamIsMarkable(MCStreamRef self)
{
	__MCAssertIsStream(self);

	return __MCStreamCallbacks(self) -> mark != nil;
}

MC_DLLEXPORT_DEF
bool MCStreamIsSeekable(MCStreamRef self)
{
	__MCAssertIsStream(self);

	return __MCStreamCallbacks(self) -> seek != nil;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCStreamGetAvailableForRead(MCStreamRef self, size_t& r_available)
{
	__MCAssertIsStream(self);

	if (__MCStreamCallbacks(self) -> get_available_for_read == nil)
		return false;
	return __MCStreamCallbacks(self) -> get_available_for_read(self, r_available);
}

MC_DLLEXPORT_DEF
bool MCStreamRead(MCStreamRef self, void *p_buffer, size_t p_amount)
{
	__MCAssertIsStream(self);
	MCAssert(nil != p_buffer || 0 == p_amount);

	if (__MCStreamCallbacks(self) -> read == nil)
		return false;
	return __MCStreamCallbacks(self) -> read(self, p_buffer, p_amount);
}

MC_DLLEXPORT_DEF
bool MCStreamGetAvailableForWrite(MCStreamRef self, size_t& r_available)
{
	__MCAssertIsStream(self);

	if (__MCStreamCallbacks(self) -> get_available_for_write == nil)
		return false;
	return __MCStreamCallbacks(self) -> get_available_for_write(self, r_available);
}

MC_DLLEXPORT_DEF
bool MCStreamWrite(MCStreamRef self, const void *p_buffer, size_t p_amount)
{
	__MCAssertIsStream(self);
	MCAssert(nil != p_buffer || 0 == p_amount);

	if (__MCStreamCallbacks(self) -> write == nil)
		return false;
	return __MCStreamCallbacks(self) -> write(self, p_buffer, p_amount);
}

MC_DLLEXPORT_DEF
bool MCStreamSkip(MCStreamRef self, size_t p_amount)
{
	__MCAssertIsStream(self);

	if (__MCStreamCallbacks(self) -> skip != nil)
		return __MCStreamCallbacks(self) -> skip(self, p_amount);
	if (__MCStreamCallbacks(self) -> seek != nil)
	{
		filepos_t t_pos;
		if (!__MCStreamCallbacks(self) -> tell(self, t_pos))
			return false;
		return __MCStreamCallbacks(self) -> seek(self, t_pos + p_amount);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCStreamMark(MCStreamRef self, size_t p_read_limit)
{
	__MCAssertIsStream(self);

	if (__MCStreamCallbacks(self) -> mark == nil)
		return false;
	return __MCStreamCallbacks(self) -> mark(self, p_read_limit);
}

MC_DLLEXPORT_DEF
bool MCStreamReset(MCStreamRef self)
{
	__MCAssertIsStream(self);

	if (__MCStreamCallbacks(self) -> reset == nil)
		return false;
	return __MCStreamCallbacks(self) -> reset(self);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCStreamTell(MCStreamRef self, filepos_t& r_position)
{
	__MCAssertIsStream(self);

	if (__MCStreamCallbacks(self) -> tell == nil)
		return false;
	return __MCStreamCallbacks(self) -> tell(self, r_position);
}

MC_DLLEXPORT_DEF
bool MCStreamSeek(MCStreamRef self, filepos_t p_position)
{
	__MCAssertIsStream(self);

	if (__MCStreamCallbacks(self) -> seek == nil)
		return false;
	return __MCStreamCallbacks(self) -> seek(self, p_position);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCStreamReadUInt8(MCStreamRef self, uint8_t& r_value)
{
	return MCStreamRead(self, &r_value, sizeof(uint8_t));
}

MC_DLLEXPORT_DEF
bool MCStreamReadUInt16(MCStreamRef self, uint16_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(uint16_t)))
	{
		r_value = MCSwapInt16NetworkToHost(r_value);
		return true;
	}
	return false;
}

MC_DLLEXPORT_DEF
bool MCStreamReadUInt32(MCStreamRef self, uint32_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(uint32_t)))
	{
		r_value = MCSwapInt32NetworkToHost(r_value);
		return true;
	}
	return false;
}

MC_DLLEXPORT_DEF
bool MCStreamReadUInt64(MCStreamRef self, uint64_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(uint64_t)))
	{
		r_value = MCSwapInt64NetworkToHost(r_value);
		return true;
	}
	return false;
}

MC_DLLEXPORT_DEF
bool MCStreamReadInt8(MCStreamRef self, int8_t& r_value)
{
	return MCStreamRead(self, &r_value, sizeof(int8_t));
}

MC_DLLEXPORT_DEF
bool MCStreamReadInt16(MCStreamRef self, int16_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(int16_t)))
	{
		r_value = (int16_t)MCSwapInt16NetworkToHost((uint16_t)r_value);
		return true;
	}
	return false;
}

MC_DLLEXPORT_DEF
bool MCStreamReadInt32(MCStreamRef self, int32_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(int32_t)))
	{
		r_value = (int32_t)MCSwapInt32NetworkToHost((uint32_t)r_value);
		return true;
	}
	return false;
}

MC_DLLEXPORT_DEF
bool MCStreamReadInt64(MCStreamRef self, int64_t& r_value)
{
	if (MCStreamRead(self, &r_value, sizeof(int64_t)))
	{
		r_value = (int64_t)MCSwapInt64NetworkToHost((uint64_t)r_value);
		return true;
	}
	return false;
}

MC_DLLEXPORT_DEF
bool MCStreamReadDouble(MCStreamRef stream, double& r_value)
{
	uint64_t t_bits;
	if (!MCStreamReadUInt64(stream, t_bits))
		return false;

	MCMemoryCopy(&r_value, &t_bits, sizeof(uint64_t));

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCStreamWriteUInt8(MCStreamRef self, uint8_t p_value)
{
	return MCStreamWrite(self, &p_value, sizeof(uint8_t));
}

MC_DLLEXPORT_DEF
bool MCStreamWriteUInt16(MCStreamRef self, uint16_t p_value)
{
    uint16_t t_swapped_value;
    t_swapped_value = MCSwapInt16NetworkToHost(p_value);
	return MCStreamWrite(self, &t_swapped_value, sizeof(uint16_t));
}

MC_DLLEXPORT_DEF
bool MCStreamWriteUInt32(MCStreamRef self, uint32_t p_value)
{
    uint32_t t_swapped_value;
    t_swapped_value = MCSwapInt32NetworkToHost(p_value);
	return MCStreamWrite(self, &t_swapped_value, sizeof(uint32_t));
}

MC_DLLEXPORT_DEF
bool MCStreamWriteUInt64(MCStreamRef self, uint64_t p_value)
{
    uint64_t t_swapped_value;
    t_swapped_value = MCSwapInt64NetworkToHost(p_value);
	return MCStreamWrite(self, &t_swapped_value, sizeof(uint64_t));
}

MC_DLLEXPORT_DEF
bool MCStreamWriteInt8(MCStreamRef self, int8_t p_value)
{
	return MCStreamWrite(self, &p_value, sizeof(int8_t));
}

MC_DLLEXPORT_DEF
bool MCStreamWriteInt16(MCStreamRef self, int16_t p_value)
{
    uint16_t t_swapped_value;
    t_swapped_value = MCSwapInt16NetworkToHost((uint16_t)p_value);
	return MCStreamWrite(self, &t_swapped_value, sizeof(uint16_t));
}

MC_DLLEXPORT_DEF
bool MCStreamWriteInt32(MCStreamRef self, int32_t p_value)
{
    uint32_t t_swapped_value;
    t_swapped_value = MCSwapInt16NetworkToHost((uint32_t)p_value);
	return MCStreamWrite(self, &t_swapped_value, sizeof(uint32_t));
}

MC_DLLEXPORT_DEF
bool MCStreamWriteInt64(MCStreamRef self, int64_t p_value)
{
    uint64_t t_swapped_value;
    t_swapped_value = MCSwapInt16NetworkToHost((uint64_t)p_value);
	return MCStreamWrite(self, &t_swapped_value, sizeof(uint64_t));
}

MC_DLLEXPORT_DEF
bool MCStreamWriteDouble(MCStreamRef stream, double p_value)
{
	uint64_t t_bits;
    MCMemoryCopy(&t_bits, &p_value, sizeof(uint64_t));
	return MCStreamWriteUInt64(stream, t_bits);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
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

MC_DLLEXPORT_DEF
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

MC_DLLEXPORT_DEF
bool MCStreamReadName(MCStreamRef stream, MCNameRef& r_name)
{
	MCStringRef t_string;
	if (!MCStreamReadString(stream, t_string))
		return false;

	return MCNameCreateAndRelease(t_string, r_name);
}

MC_DLLEXPORT_DEF
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

MC_DLLEXPORT_DEF
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

MC_DLLEXPORT_DEF
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

MC_DLLEXPORT_DEF
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

bool __MCStreamInitialize(void)
{
	if (!MCNamedCustomTypeInfoCreate(MCNAME("livecode.lang.Stream"), kMCNullTypeInfo, &kMCStreamCustomValueCallbacks, kMCStreamTypeInfo))
		return false;
	
    return true;
}

void __MCStreamFinalize(void)
{
    MCValueRelease(kMCStreamTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
