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

#ifndef __MC_EXEC__
#define __MC_EXEC__

#ifndef OBJDEFS_H
#include "objdefs.h"
#endif

#ifndef OBJECT_H
#include "object.h"
#endif

////////////////////////////////////////////////////////////////////////////////

enum MCExecValueType
{
	kMCExecValueTypeNone,
	
	kMCExecValueTypeValueRef,
	kMCExecValueTypeBooleanRef,
	kMCExecValueTypeStringRef,
	kMCExecValueTypeNameRef,
	kMCExecValueTypeDataRef,
	kMCExecValueTypeArrayRef,
    kMCExecValueTypeNumberRef,
    
	kMCExecValueTypeUInt,
	kMCExecValueTypeInt,
    kMCExecValueTypeBool,
    kMCExecValueTypeDouble,
    kMCExecValueTypeFloat,
    kMCExecValueTypeChar,
    kMCExecValueTypePoint,
    kMCExecValueTypeColor,
    kMCExecValueTypeRectangle,
};

struct MCExecValue
{
public:
	MCExecValue()
	{
		MCMemoryClear(*this);
		type = kMCExecValueTypeNone;
	}
	union
	{
		MCValueRef valueref_value;
		MCBooleanRef booleanref_value;
		MCStringRef stringref_value;
		MCNameRef nameref_value;
		MCDataRef dataref_value;
		MCArrayRef arrayref_value;
		MCNumberRef numberref_value;
		
		uinteger_t uint_value;
		integer_t int_value;
		bool bool_value;
		double double_value;
		float float_value;
		char_t char_value;
		MCPoint point_value;
		MCColor color_value;
		MCRectangle rectangle_value;
        intenum_t enum_value;
	};
	MCExecValueType type;
};

// Convert the slot from_value of type from_type, to the slot to_type at
// to_type. This method releases the from_type even if a type conversion
// error occurs.
void MCExecTypeConvertAndReleaseAlways(MCExecContext& ctxt, MCExecValueType from_type, void *from_value, MCExecValueType to_type, void *to_value);
void MCExecTypeConvertToValueRefAndReleaseAlways(MCExecContext& ctxt, MCExecValueType p_from_type, void *p_from_value, MCValueRef& r_value);
void MCExecTypeConvertFromValueRefAndReleaseAlways(MCExecContext& ctxt, MCValueRef p_from_value, MCExecValueType p_to_type, void *p_to_value);
void MCExecTypeRelease(MCExecValue &self);
void MCExecTypeSetValueRef(MCExecValue &self, MCValueRef p_value);
bool MCExecTypeIsValueRef(MCExecValueType p_type);
bool MCExecTypeIsNumber(MCExecValueType p_type);
void MCExecTypeCopy(const MCExecValue &self, MCExecValue &r_dest);

// Defined for convenience in exec-interface-field-chunk.cpp
// where the template system needs only one parameter
template<typename T>
struct vector_t
{
    T* elements;
    uindex_t count;
};

////////////////////////////////////////////////////////////////////////////////

enum MCExecType
{
	/* A */ kMCExecTypeAny,
	/* B */ kMCExecTypeBoolean,
	/* N */ kMCExecTypeNumber,
	/* S */ kMCExecTypeString,
	/* M */ kMCExecTypeName,
	/* R */ kMCExecTypeArray,
	
	/* b */ kMCExecTypeBool,
	/* i */ kMCExecTypeInt,
	/* j */ kMCExecTypeInt64,
	/* u */ kMCExecTypeUInt,
	/* v */ kMCExecTypeUInt64,
	/* f */ kMCExecTypeFloat,
	/* d */ kMCExecTypeDouble,
	/* c */ kMCExecTypeNativeChar,
	
	/* X */ kMCExecTypeLegacyColor,
	/* Y */ kMCExecTypeLegacyPoint,
	/* Z */ kMCExecTypeLegacyRectangle,
	
	/* E */ kMCExecTypeEnum,
	/* T */ kMCExecTypeSet,
	/* C */ kMCExecTypeCustom,
};

struct MCExecSetTypeElementInfo
{
	const char *tag;
	uindex_t bit;
};

struct MCExecSetTypeInfo
{
	const char *name;
	uindex_t count;
	MCExecSetTypeElementInfo *elements;
};

struct MCExecEnumTypeElementInfo
{
	const char *tag;
	intenum_t value;
	bool read_only;
};

struct MCExecEnumTypeInfo
{
	const char *name;
	uindex_t count;
	MCExecEnumTypeElementInfo *elements;
};

void MCExecParseSet(MCExecContext& ctxt, MCExecSetTypeInfo *p_info, MCExecValue p_value, intset_t& r_value);
void MCExecParseEnum(MCExecContext& ctxt, MCExecEnumTypeInfo *p_info, MCExecValue p_value, intenum_t& r_value);
void MCExecFormatSet(MCExecContext& ctxt, MCExecSetTypeInfo *p_info, intset_t t_value, MCExecValue& r_value);
void MCExecFormatEnum(MCExecContext& ctxt, MCExecEnumTypeInfo *p_info, intenum_t p_value, MCExecValue& r_value);

typedef void (*MCExecCustomTypeParseProc)(MCExecContext& ctxt, MCStringRef input, void *r_output);
typedef void (*MCExecCustomTypeFormatProc)(MCExecContext& ctxt, const void *input, MCStringRef& r_output);
typedef void (*MCExecCustomTypeFreeProc)(MCExecContext& ctxt, void *input);

struct MCExecCustomTypeInfo
{
	const char *name;
	uindex_t size;
	void *parse;
	void *format;
	void *free;
};

////////////////////////////////////////////////////////////////////////////////

enum MCPropertyType
{
	kMCPropertyTypeAny,
	kMCPropertyTypeBool,
	kMCPropertyTypeInt16,
	kMCPropertyTypeInt32,
    kMCPropertyTypeUInt8,
	kMCPropertyTypeUInt16,
	kMCPropertyTypeUInt16_1_16,
	kMCPropertyTypeUInt16_1_65535,
	kMCPropertyTypeUInt32,
	kMCPropertyTypeDouble,
	kMCPropertyTypeChar,
	kMCPropertyTypeString,
	kMCPropertyTypeBinaryString,
	kMCPropertyTypeColor,
	kMCPropertyTypeRectangle,
    kMCPropertyTypeRectangle32,
	kMCPropertyTypePoint,
	kMCPropertyTypeInt16X2,
	kMCPropertyTypeInt16X4,
    kMCPropertyTypeInt32X2,
    kMCPropertyTypeInt32X4,
	kMCPropertyTypeArray,
	kMCPropertyTypeSet,
	kMCPropertyTypeEnum,
	kMCPropertyTypeCustom,
    kMCPropertyTypeOptionalBool,
    kMCPropertyTypeOptionalUInt8,
	kMCPropertyTypeOptionalInt16,
	kMCPropertyTypeOptionalUInt16,
	kMCPropertyTypeOptionalUInt32,
	kMCPropertyTypeOptionalDouble,
	kMCPropertyTypeOptionalString,
	kMCPropertyTypeOptionalRectangle,
    kMCPropertyTypeOptionalPoint,
    kMCPropertyTypeOptionalColor,
	kMCPropertyTypeOptionalEnum,
	kMCPropertyTypeName,
    kMCPropertyTypeProperLinesOfString,
    kMCPropertyTypeLinesOfString,
    kMCPropertyTypeLinesOfLooseUInt,
    kMCPropertyTypeLinesOfUIntX2,
    kMCPropertyTypeLinesOfLooseDouble,
    kMCPropertyTypeLinesOfPoint,
    kMCPropertyTypeProperItemsOfString,
    kMCPropertyTypeItemsOfLooseUInt,
	kMCPropertyTypeOptionalItemsOfLooseUInt,
    kMCPropertyTypeItemsOfString,
    kMCPropertyTypeMixedBool,
	kMCPropertyTypeMixedInt16,
	kMCPropertyTypeMixedUInt8,
	kMCPropertyTypeMixedUInt16,
    kMCPropertyTypeMixedUInt32,
    kMCPropertyTypeMixedOptionalBool,
    kMCPropertyTypeMixedOptionalInt16,
    kMCPropertyTypeMixedOptionalInt32,
	kMCPropertyTypeMixedOptionalUInt8,
    kMCPropertyTypeMixedOptionalUInt16,
    kMCPropertyTypeMixedOptionalUInt32,
    kMCPropertyTypeMixedOptionalString,
	kMCPropertyTypeMixedCustom,
	kMCPropertyTypeMixedEnum,
	kMCPropertyTypeMixedOptionalEnum,
	kMCPropertyTypeMixedItemsOfLooseUInt,
    kMCPropertyTypeMixedItemsOfString,
    kMCPropertyTypeMixedLinesOfLooseUInt,
    kMCPropertyTypeRecord,
    kMCPropertyTypeLegacyPoints,
};

enum MCPropertyInfoChunkType
{
	kMCPropertyInfoChunkTypeNone,
	kMCPropertyInfoChunkTypeChar,
	kMCPropertyInfoChunkTypeLine,
};

struct MCPropertyInfo
{
	Properties property;
	bool effective;
	MCPropertyType type;
	void *type_info;
	void *getter;
	void *setter;
	bool has_effective;
    bool is_array_prop;
    MCPropertyInfoChunkType chunk_type;
};

// SN-2014-09-02: [[ Bug 13314 ]] Added the mark as a parameter, to allow the changes of a mark to taken in account.
void MCExecResolveCharsOfField(MCExecContext& ctxt, MCField *p_field, uint32_t p_part, MCMarkedText p_mark, int32_t& r_start, int32_t& r_finish);

template<typename O, typename A, void (O::*Method)(MCExecContext&, A)> inline void MCPropertyObjectThunk(MCExecContext& ctxt, MCObjectPtr *obj, A arg)
{
	(static_cast<O *>(obj -> object) ->* Method)(ctxt, arg);
}

template<typename O, typename A, void (O::*Method)(MCExecContext&, uint32_t, A)> inline void MCPropertyObjectPartThunk(MCExecContext& ctxt, MCObjectPtr *obj, A arg)
{
	(static_cast<O *>(obj -> object) ->* Method)(ctxt, obj -> part_id, arg);
}

template<typename O, typename A, void (O::*Method)(MCExecContext&, MCNameRef, A)> inline void MCPropertyObjectArrayThunk(MCExecContext& ctxt, MCObjectIndexPtr *obj, A arg)
{
	(static_cast<O *>(obj -> object) ->* Method)(ctxt, obj -> index, arg);
}

template<typename O, typename A, typename B, void (O::*Method)(MCExecContext&, B, A)> inline void MCPropertyObjectListThunk(MCExecContext& ctxt, MCObjectPtr *obj, B count, A arg)
{
    (static_cast<O *>(obj -> object) ->* Method)(ctxt, count, arg);
}

template<typename O, typename A, void (O::*Method)(MCExecContext&, uint32_t, int32_t, int32_t, A)> inline void MCPropertyObjectChunkThunk(MCExecContext& ctxt, MCObjectChunkPtr *obj, A arg)
{
    int32_t t_si, t_ei;
    
    if (obj -> object -> gettype() == CT_FIELD)
    {
        t_si = 0;
        t_ei = INT32_MAX;
        // SN-2014-09-02: [[ Bug 13314 ]] Added the mark as a parameter, to allow the changes of a mark to taken in account.
        MCExecResolveCharsOfField(ctxt, (MCField *)obj -> object, obj -> part_id, obj -> mark, t_si, t_ei);
    }
    else
    {
        t_si = obj -> mark . start;
        t_ei = obj -> mark . finish;
    }
    
	(static_cast<O *>(obj -> object) ->* Method)(ctxt, obj -> part_id, t_si, t_ei, arg);
}

template<typename O, typename A, typename B, void (O::*Method)(MCExecContext&, uint32_t, int32_t, int32_t, B, A)> inline void MCPropertyObjectChunkListThunk(MCExecContext& ctxt, MCObjectChunkPtr *obj, B count, A arg)
{
    int32_t t_si, t_ei;
    
    if (obj -> object -> gettype() == CT_FIELD)
    {
        t_si = 0;
        t_ei = INT32_MAX;
        // SN-2014-09-02: [[ Bug 13314 ]] Added the mark as a parameter, to allow the changes of a mark to taken in account.
        MCExecResolveCharsOfField(ctxt, (MCField *)obj -> object, obj -> part_id, obj -> mark, t_si, t_ei);
    }
    else
    {
        t_si = obj -> mark . start;
        t_ei = obj -> mark . finish;
    }
    
	(static_cast<O *>(obj -> object) ->* Method)(ctxt, obj -> part_id, t_si, t_ei, count, arg);
}


template<typename O, typename A, typename B, void (O::*Method)(MCExecContext&, uint32_t, int32_t, int32_t, bool&, A)> inline void MCPropertyObjectChunkMixedThunk(MCExecContext& ctxt, MCObjectChunkPtr *obj, B mixed, A arg)
{
    int32_t t_si, t_ei;
    
    if (obj -> object -> gettype() == CT_FIELD)
    {
        t_si = 0;
        t_ei = INT32_MAX;
        // SN-2014-09-02: [[ Bug 13314 ]] Added the mark as a parameter, to allow the changes of a mark to taken in account.
        MCExecResolveCharsOfField(ctxt, (MCField *)obj -> object, obj -> part_id, obj -> mark, t_si, t_ei);
    }
    else
    {
        t_si = obj -> mark . start;
        t_ei = obj -> mark . finish;
    }
    
	(static_cast<O *>(obj -> object) ->* Method)(ctxt, obj -> part_id, t_si, t_ei, mixed, arg);
}

template<typename O, typename A, typename B, typename C, void (O::*Method)(MCExecContext&, uint32_t, int32_t, int32_t, bool&, B, A)> inline void MCPropertyObjectChunkMixedListThunk(MCExecContext& ctxt, MCObjectChunkPtr *obj, C mixed, B count, A arg)
{
    int32_t t_si, t_ei;
    
    if (obj -> object -> gettype() == CT_FIELD)
    {
        t_si = 0;
        t_ei = INT32_MAX;
        // SN-2014-09-02: [[ Bug 13314 ]] Added the mark as a parameter, to allow the changes of a mark to taken in account.
        MCExecResolveCharsOfField(ctxt, (MCField *)obj -> object, obj -> part_id, obj -> mark, t_si, t_ei);
    }
    else
    {
        t_si = obj -> mark . start;
        t_ei = obj -> mark . finish;
    }
    
	(static_cast<O *>(obj -> object) ->* Method)(ctxt, obj -> part_id, t_si, t_ei, mixed, count, arg);
}

template<typename O, typename A, typename B, void (O::*Method)(MCExecContext&, MCNameRef, uint32_t, int32_t, int32_t, bool&, A)> inline void MCPropertyObjectChunkMixedArrayThunk(MCExecContext& ctxt, MCObjectChunkIndexPtr *obj, B mixed, A arg)
{
    int32_t t_si, t_ei;
    
    if (obj -> object -> gettype() == CT_FIELD)
    {
        t_si = 0;
        t_ei = INT32_MAX;
        // SN-2014-09-02: [[ Bug 13314 ]] Added the mark as a parameter, to allow the changes of a mark to taken in account.
        MCExecResolveCharsOfField(ctxt, (MCField *)obj -> object, obj -> part_id, obj -> mark, t_si, t_ei);
    }
    else
    {
        t_si = obj -> mark . start;
        t_ei = obj -> mark . finish;
    }
    
	(static_cast<O *>(obj -> object) ->* Method)(ctxt, obj -> index, obj -> part_id, t_si, t_ei, mixed, arg);
}

template<typename O, typename A, void (O::*Method)(MCExecContext&, MCNameRef, uint32_t, int32_t, int32_t, A)> inline void MCPropertyObjectChunkArrayThunk(MCExecContext& ctxt, MCObjectChunkIndexPtr *obj, A arg)
{
    int32_t t_si, t_ei;
    
    if (obj -> object -> gettype() == CT_FIELD)
    {
        t_si = 0;
        t_ei = INT32_MAX;
        // SN-2014-09-02: [[ Bug 13314 ]] Added the mark as a parameter, to allow the changes of a mark to taken in account.
        MCExecResolveCharsOfField(ctxt, (MCField *)obj -> object, obj -> part_id, obj -> mark, t_si, t_ei);
    }
    else
    {
        t_si = obj -> mark . start;
        t_ei = obj -> mark . finish;
    }
    
	(static_cast<O *>(obj -> object) ->* Method)(ctxt, obj -> index, obj -> part_id, t_si, t_ei, arg);
}

template<typename A, void Method(MCExecContext&, MCNameRef, A)> inline void MCPropertyArrayThunk(MCExecContext& ctxt, MCNameRef index, A arg)
{
    Method(ctxt, index, arg);
}

template<typename A, void Method(MCExecContext&, A)> inline void MCPropertyThunk(MCExecContext& ctxt, void *, A arg)
{
    Method(ctxt, arg);
}

template<typename A, typename B, void Method(MCExecContext&, B, A)> inline void MCPropertyListThunk(MCExecContext& ctxt, void *, B count, A arg)
{
    Method(ctxt, count, arg);
}

#define MCPropertyThunkImp(mth,typ) (void(*)(MCExecContext&, void *,typ))MCPropertyThunk<typ,mth>
#define MCPropertyArrayThunkImp(mth,index,typ) (void(*)(MCExecContext&, MCNameRef, typ))MCPropertyArrayThunk<typ,mth>
#define MCPropertyListThunkImp(mth,count,typ) (void(*)(MCExecContext&,void *,count,typ))MCPropertyListThunk<typ,count,mth>

#define MCPropertyThunkArrayGetBinaryString(mth) MCPropertyArrayThunkImp(mth, MCNameRef, MCDataRef&)
#define MCPropertyThunkArraySetBinaryString(mth) MCPropertyArrayThunkImp(mth, MCNameRef, MCDataRef)
#define MCPropertyThunkArrayGetString(mth) MCPropertyArrayThunkImp(mth, MCNameRef, MCStringRef&)
#define MCPropertyThunkArraySetString(mth) MCPropertyArrayThunkImp(mth, MCNameRef, MCStringRef)
#define MCPropertyThunkArrayGetArray(mth) MCPropertyArrayThunkImp(mth, MCNameRef, MCArrayRef&)
#define MCPropertyThunkArrayGetAny(mth) MCPropertyArrayThunkImp(mth, MCNameRef, MCValueRef&)
#define MCPropertyThunkArraySetAny(mth) MCPropertyArrayThunkImp(mth, MCNameRef, MCValueRef)

#define MCPropertyThunkGetAny(mth) MCPropertyThunkImp(mth, MCValueRef&)
#define MCPropertyThunkGetBool(mth) MCPropertyThunkImp(mth, bool&)
#define MCPropertyThunkGetInt16(mth) MCPropertyThunkImp(mth, integer_t&)
#define MCPropertyThunkGetInt16X2(mth) MCPropertyThunkImp(mth, integer_t*)
#define MCPropertyThunkGetInt16X4(mth) MCPropertyThunkImp(mth, integer_t*)
#define MCPropertyThunkGetInt32(mth) MCPropertyThunkImp(mth, integer_t&)
#define MCPropertyThunkGetUInt16(mth) MCPropertyThunkImp(mth, uinteger_t&)
#define MCPropertyThunkGetUInt32(mth) MCPropertyThunkImp(mth, uinteger_t&)
#define MCPropertyThunkGetOptionalInt16(mth) MCPropertyThunkImp(mth, integer_t*&)
#define MCPropertyThunkGetOptionalUInt16(mth) MCPropertyThunkImp(mth, uinteger_t*&)
#define MCPropertyThunkGetOptionalUInt32(mth) MCPropertyThunkImp(mth, uinteger_t*&)
#define MCPropertyThunkGetDouble(mth) MCPropertyThunkImp(mth, double&)
#define MCPropertyThunkGetOptionalDouble(mth) MCPropertyThunkImp(mth, double *&)
#define MCPropertyThunkGetChar(mth) MCPropertyThunkImp(mth, char_t&)
#define MCPropertyThunkGetString(mth) MCPropertyThunkImp(mth, MCStringRef&)
#define MCPropertyThunkGetBinaryString(mth) MCPropertyThunkImp(mth, MCDataRef&)
#define MCPropertyThunkGetOptionalString(mth) MCPropertyThunkImp(mth, MCStringRef&)
#define MCPropertyThunkGetRectangle(mth) MCPropertyThunkImp(mth, MCRectangle&)
#define MCPropertyThunkGetOptionalRectangle(mth) MCPropertyThunkImp(mth, MCRectangle*&)
#define MCPropertyThunkGetPoint(mth) MCPropertyThunkImp(mth, MCPoint&)
#define MCPropertyThunkGetOptionalPoint(mth) MCPropertyThunkImp(mth, MCPoint*&)
#define MCPropertyThunkGetCustomType(mth, typ) MCPropertyThunkImp(mth, typ&)
#define MCPropertyThunkGetEnumType(mth) MCPropertyThunkImp(mth, intenum_t&)
#define MCPropertyThunkGetSetType(mth) MCPropertyThunkImp(mth, intset_t&)
#define MCPropertyThunkGetOptionalCustomType(mth, typ) MCPropertyThunkImp(mth, typ*&)
#define MCPropertyThunkGetOptionalEnumType(mth) MCPropertyThunkImp(mth, intenum_t*&)
#define MCPropertyThunkGetArray(mth) MCPropertyThunkImp(mth, MCArrayRef&)
#define MCPropertyThunkGetName(mth) MCPropertyThunkImp(mth, MCNameRef&)
#define MCPropertyThunkGetProperLinesOfString(mth) MCPropertyThunkImp(mth, MCProperListRef&)
#define MCPropertyThunkGetProperItemsOfString(mth) MCPropertyThunkImp(mth, MCProperListRef&)
#define MCPropertyThunkGetItemsOfLooseUInt(mth) MCPropertyListThunkImp(mth,uindex_t&,uinteger_t*&)
#define MCPropertyThunkGetLinesOfString(mth) MCPropertyListThunkImp(mth,uindex_t&,MCStringRef*&)
#define MCPropertyThunkGetLinesOfLooseDouble(mth) MCPropertyListThunkImp(mth,uindex_t&,double*&)
#define MCPropertyThunkGetItemsOfString(mth) MCPropertyListThunkImp(mth,uindex_t*,MCStringRef*&)

#define MCPropertyThunkSetAny(mth) MCPropertyThunkImp(mth, MCValueRef)
#define MCPropertyThunkSetBool(mth) MCPropertyThunkImp(mth, bool)
#define MCPropertyThunkSetInt16(mth) MCPropertyThunkImp(mth, integer_t)
#define MCPropertyThunkSetInt16X2(mth) MCPropertyThunkImp(mth, integer_t*)
#define MCPropertyThunkSetInt16X4(mth) MCPropertyThunkImp(mth, integer_t*)
#define MCPropertyThunkSetInt32(mth) MCPropertyThunkImp(mth, integer_t)
#define MCPropertyThunkSetUInt16(mth) MCPropertyThunkImp(mth, uinteger_t)
#define MCPropertyThunkSetUInt32(mth) MCPropertyThunkImp(mth, uinteger_t)
#define MCPropertyThunkSetOptionalInt16(mth) MCPropertyThunkImp(mth, integer_t*)
#define MCPropertyThunkSetOptionalUInt16(mth) MCPropertyThunkImp(mth, uinteger_t*)
#define MCPropertyThunkSetOptionalUInt32(mth) MCPropertyThunkImp(mth, uinteger_t*)
#define MCPropertyThunkSetDouble(mth) MCPropertyThunkImp(mth, double)
#define MCPropertyThunkSetOptionalDouble(mth) MCPropertyThunkImp(mth, double*)
#define MCPropertyThunkSetChar(mth) MCPropertyThunkImp(mth, char_t)
#define MCPropertyThunkSetString(mth) MCPropertyThunkImp(mth, MCStringRef)
#define MCPropertyThunkSetBinaryString(mth) MCPropertyThunkImp(mth, MCDataRef)
#define MCPropertyThunkSetOptionalString(mth) MCPropertyThunkImp(mth, MCStringRef)
#define MCPropertyThunkSetRectangle(mth) MCPropertyThunkImp(mth, MCRectangle)
#define MCPropertyThunkSetOptionalRectangle(mth) MCPropertyThunkImp(mth, MCRectangle*)
#define MCPropertyThunkSetPoint(mth) MCPropertyThunkImp(mth, MCPoint)
#define MCPropertyThunkSetOptionalPoint(mth) MCPropertyThunkImp(mth, MCPoint*)
#define MCPropertyThunkSetCustomType(mth, typ) MCPropertyThunkImp(mth, const typ&)
#define MCPropertyThunkSetEnumType(mth) MCPropertyThunkImp(mth, intenum_t)
#define MCPropertyThunkSetSetType(mth) MCPropertyThunkImp(mth, intset_t)
#define MCPropertyThunkSetOptionalCustomType(mth, typ) MCPropertyThunkImp(mth, const typ*&)
#define MCPropertyThunkSetOptionalEnumType(mth) MCPropertyThunkImp(mth, intenum_t*)
#define MCPropertyThunkSetArray(mth) MCPropertyThunkImp(mth, MCArrayRef)
#define MCPropertyThunkSetName(mth) MCPropertyThunkImp(mth, MCNameRef)
#define MCPropertyThunkSetItemsOfLooseUInt(mth) MCPropertyListThunkImp(mth,uindex_t,uinteger_t*)
#define MCPropertyThunkSetLinesOfString(mth) MCPropertyListThunkImp(mth,uindex_t,MCStringRef*)
#define MCPropertyThunkSetLinesOfLooseDouble(mth) MCPropertyListThunkImp(mth,uindex_t,double*)
#define MCPropertyThunkSetItemsOfString(mth) MCPropertyListThunkImp(mth,uindex_t,MCStringRef*)

#define MCPropertyObjectThunkImp(obj, mth, typ) (void(*)(MCExecContext&,MCObjectPtr*,typ))MCPropertyObjectThunk<obj,typ,&obj::mth>
#define MCPropertyObjectPartThunkImp(obj, mth, typ) (void(*)(MCExecContext&,MCObjectPtr*,typ))MCPropertyObjectPartThunk<obj,typ,&obj::mth>
#define MCPropertyObjectArrayThunkImp(obj, mth, typ) (void(*)(MCExecContext&,MCObjectIndexPtr*,typ))MCPropertyObjectArrayThunk<obj,typ,&obj::mth>
#define MCPropertyObjectListThunkImp(obj, mth, count, typ) (void(*)(MCExecContext&,MCObjectPtr*,count,typ))MCPropertyObjectListThunk<obj,typ,count,&obj::mth>
#define MCPropertyObjectChunkThunkImp(obj, mth, typ) (void(*)(MCExecContext&,MCObjectChunkPtr*,typ))MCPropertyObjectChunkThunk<obj,typ,&obj::mth>
#define MCPropertyObjectChunkListThunkImp(obj, mth, count, typ) (void(*)(MCExecContext&,MCObjectChunkPtr*,count,typ))MCPropertyObjectChunkListThunk<obj,typ,count,&obj::mth>
#define MCPropertyObjectChunkMixedThunkImp(obj, mth, mixed, typ) (void(*)(MCExecContext&,MCObjectChunkPtr*,mixed,typ))MCPropertyObjectChunkMixedThunk<obj,typ,mixed,&obj::mth>
#define MCPropertyObjectChunkMixedListThunkImp(obj, mth, mixed, count, typ) (void(*)(MCExecContext&,MCObjectChunkPtr*,mixed,count,typ))MCPropertyObjectChunkMixedListThunk<obj,typ,count,mixed,&obj::mth>

#define MCPropertyObjectChunkMixedArrayThunkImp(obj, mth, mixed, typ) (void(*)(MCExecContext&,MCObjectChunkIndexPtr*,mixed,typ))MCPropertyObjectChunkMixedArrayThunk<obj,typ,mixed,&obj::mth>
#define MCPropertyObjectChunkArrayThunkImp(obj, mth, typ) (void(*)(MCExecContext&,MCObjectChunkIndexPtr*,typ))MCPropertyObjectChunkArrayThunk<obj,typ,&obj::mth>

#define MCPropertyObjectThunkGetAny(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCValueRef&)
#define MCPropertyObjectThunkGetBool(obj, mth) MCPropertyObjectThunkImp(obj, mth, bool&)
#define MCPropertyObjectThunkGetOptionalBool(obj, mth) MCPropertyObjectThunkImp(obj, mth, bool*&)
#define MCPropertyObjectThunkGetInt16(obj, mth) MCPropertyObjectThunkImp(obj, mth, integer_t&)
#define MCPropertyObjectThunkGetInt32(obj, mth) MCPropertyObjectThunkImp(obj, mth, integer_t&)
#define MCPropertyObjectThunkGetInt32X4(obj, mth) MCPropertyObjectThunkImp(obj, mth, integer_t*)
#define MCPropertyObjectThunkGetUInt16(obj, mth) MCPropertyObjectThunkImp(obj, mth, uinteger_t&)
#define MCPropertyObjectThunkGetUInt32(obj, mth) MCPropertyObjectThunkImp(obj, mth, uinteger_t&)
#define MCPropertyObjectThunkGetOptionalInt16(obj, mth) MCPropertyObjectThunkImp(obj, mth, integer_t*&)
#define MCPropertyObjectThunkGetOptionalUInt16(obj, mth) MCPropertyObjectThunkImp(obj, mth, uinteger_t*&)
#define MCPropertyObjectThunkGetOptionalUInt32(obj, mth) MCPropertyObjectThunkImp(obj, mth, uinteger_t*&)
#define MCPropertyObjectThunkGetDouble(obj, mth) MCPropertyObjectThunkImp(obj, mth, double&)
#define MCPropertyObjectThunkGetOptionalDouble(obj, mth) MCPropertyObjectThunkImp(obj, mth, double*&)
#define MCPropertyObjectThunkGetString(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCStringRef&)
#define MCPropertyObjectThunkGetBinaryString(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCDataRef&)
#define MCPropertyObjectThunkGetOptionalString(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCStringRef&)
#define MCPropertyObjectThunkGetRectangle(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCRectangle&)
#define MCPropertyObjectThunkGetOptionalRectangle(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCRectangle*&)
#define MCPropertyObjectThunkGetPoint(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCPoint&)
#define MCPropertyObjectThunkGetCustomType(obj, mth, typ) MCPropertyObjectThunkImp(obj, mth, typ&)
#define MCPropertyObjectThunkGetEnumType(obj, mth) MCPropertyObjectThunkImp(obj, mth, intenum_t&)
#define MCPropertyObjectThunkGetSetType(obj, mth) MCPropertyObjectThunkImp(obj, mth, intset_t&)
#define MCPropertyObjectThunkGetOptionalCustomType(obj, mth, typ) MCPropertyObjectThunkImp(obj, mth, typ*&)
#define MCPropertyObjectThunkGetOptionalEnumType(obj, mth) MCPropertyObjectThunkImp(obj, mth, intenum_t*&)
#define MCPropertyObjectThunkGetArray(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCArrayRef&)
#define MCPropertyObjectThunkGetName(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCNameRef&)
#define MCPropertyObjectThunkGetColor(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCColor&)

#define MCPropertyObjectListThunkGetLinesOfString(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t&, MCStringRef*&)
#define MCPropertyObjectListThunkGetLinesOfLooseUInt(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t&, uinteger_t*&)
#define MCPropertyObjectListThunkGetLinesOfPoint(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t&, MCPoint*&)
#define MCPropertyObjectListThunkGetItemsOfLooseUInt(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t&, uinteger_t*&)
#define MCPropertyObjectListThunkGetItemsOfString(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t&, MCStringRef*&)

#define MCPropertyObjectListThunkGetLegacyPoints(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t&, MCPoint*&)

#define MCPropertyObjectThunkSetAny(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCValueRef)
#define MCPropertyObjectThunkSetBool(obj, mth) MCPropertyObjectThunkImp(obj, mth, bool)
#define MCPropertyObjectThunkSetOptionalBool(obj, mth) MCPropertyObjectThunkImp(obj, mth, bool*)
#define MCPropertyObjectThunkSetInt16(obj, mth) MCPropertyObjectThunkImp(obj, mth, integer_t)
#define MCPropertyObjectThunkSetInt32(obj, mth) MCPropertyObjectThunkImp(obj, mth, integer_t)
#define MCPropertyObjectThunkSetInt32X4(obj, mth) MCPropertyObjectThunkImp(obj, mth, integer_t*)
#define MCPropertyObjectThunkSetUInt16(obj, mth) MCPropertyObjectThunkImp(obj, mth, uinteger_t)
#define MCPropertyObjectThunkSetUInt32(obj, mth) MCPropertyObjectThunkImp(obj, mth, uinteger_t)
#define MCPropertyObjectThunkSetOptionalInt16(obj, mth) MCPropertyObjectThunkImp(obj, mth, integer_t*)
#define MCPropertyObjectThunkSetOptionalUInt16(obj, mth) MCPropertyObjectThunkImp(obj, mth, uinteger_t*)
#define MCPropertyObjectThunkSetOptionalUInt32(obj, mth) MCPropertyObjectThunkImp(obj, mth, uinteger_t*)
#define MCPropertyObjectThunkSetDouble(obj, mth) MCPropertyObjectThunkImp(obj, mth, double)
#define MCPropertyObjectThunkSetOptionalDouble(obj, mth) MCPropertyObjectThunkImp(obj, mth, double*)
#define MCPropertyObjectThunkSetString(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCStringRef)
#define MCPropertyObjectThunkSetBinaryString(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCDataRef)
#define MCPropertyObjectThunkSetOptionalString(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCStringRef)
#define MCPropertyObjectThunkSetRectangle(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCRectangle)
#define MCPropertyObjectThunkSetOptionalRectangle(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCRectangle*)
#define MCPropertyObjectThunkSetPoint(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCPoint)
#define MCPropertyObjectThunkSetCustomType(obj, mth, typ) MCPropertyObjectThunkImp(obj, mth, const typ&)
#define MCPropertyObjectThunkSetEnumType(obj, mth) MCPropertyObjectThunkImp(obj, mth, intenum_t)
#define MCPropertyObjectThunkSetSetType(obj, mth) MCPropertyObjectThunkImp(obj, mth, intset_t)
#define MCPropertyObjectThunkSetOptionalCustomType(obj, mth, typ) MCPropertyObjectThunkImp(obj, mth, const typ*&)
#define MCPropertyObjectThunkSetOptionalEnumType(obj, mth) MCPropertyObjectThunkImp(obj, mth, intenum_t*)
#define MCPropertyObjectThunkSetArray(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCArrayRef)
#define MCPropertyObjectThunkSetName(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCNameRef)
#define MCPropertyObjectThunkSetColor(obj, mth) MCPropertyObjectThunkImp(obj, mth, MCColor)

#define MCPropertyObjectListThunkSetLinesOfString(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t, MCStringRef*)
#define MCPropertyObjectListThunkSetLinesOfLooseUInt(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t, uinteger_t*)
#define MCPropertyObjectListThunkSetLinesOfPoint(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t, MCPoint*)
#define MCPropertyObjectListThunkSetItemsOfLooseUInt(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t, uinteger_t*)
#define MCPropertyObjectListThunkSetItemsOfString(obj, mth), MCPropertyObjectListThunkImp(obj, mth, uindex_t, MCStringRef*)

#define MCPropertyObjectListThunkSetLegacyPoints(obj, mth) MCPropertyObjectListThunkImp(obj, mth, uindex_t, MCPoint*)

#define MCPropertyObjectPartThunkGetAny(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCValueRef&)
#define MCPropertyObjectPartThunkGetBool(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, bool&)
#define MCPropertyObjectPartThunkGetInt16(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, integer_t&)
#define MCPropertyObjectPartThunkGetInt32(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, integer_t&)
#define MCPropertyObjectPartThunkGetUInt16(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, uinteger_t&)
#define MCPropertyObjectPartThunkGetUInt32(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, uinteger_t&)
#define MCPropertyObjectPartThunkGetOptionalInt16(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, integer_t*&)
#define MCPropertyObjectPartThunkGetOptionalUInt16(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, uinteger_t*&)
#define MCPropertyObjectPartThunkGetOptionalUInt32(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, uinteger_t*&)
#define MCPropertyObjectPartThunkGetDouble(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, double&)
#define MCPropertyObjectPartThunkGetOptionalDouble(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, double*&)
#define MCPropertyObjectPartThunkGetString(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCStringRef&)
#define MCPropertyObjectPartThunkGetBinaryString(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCDataRef&)
#define MCPropertyObjectPartThunkGetOptionalString(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCStringRef&)
#define MCPropertyObjectPartThunkGetRectangle(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCRectangle&)
#define MCPropertyObjectPartThunkGetOptionalRectangle(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCRectangle*&)
#define MCPropertyObjectPartThunkGetPoint(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCPoint&)
#define MCPropertyObjectPartThunkGetCustomType(obj, mth, typ) MCPropertyObjectPartThunkImp(obj, mth, typ&)
#define MCPropertyObjectPartThunkGetEnumType(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, intenum_t&)
#define MCPropertyObjectPartThunkGetSetType(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, intset_t&)
#define MCPropertyObjectPartThunkGetOptionalCustomType(obj, mth, typ) MCPropertyObjectPartThunkImp(obj, mth, typ*&)
#define MCPropertyObjectPartThunkGetOptionalEnumType(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, intenum_t*&)
#define MCPropertyObjectPartThunkGetArray(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCArrayRef&)

#define MCPropertyObjectPartThunkSetAny(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCValueRef)
#define MCPropertyObjectPartThunkSetBool(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, bool)
#define MCPropertyObjectPartThunkSetInt16(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, integer_t)
#define MCPropertyObjectPartThunkSetInt32(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, integer_t)
#define MCPropertyObjectPartThunkSetUInt16(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, uinteger_t)
#define MCPropertyObjectPartThunkSetUInt32(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, uinteger_t)
#define MCPropertyObjectPartThunkSetOptionalInt16(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, integer_t*)
#define MCPropertyObjectPartThunkSetOptionalUInt16(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, uinteger_t*)
#define MCPropertyObjectPartThunkSetOptionalUInt32(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, uinteger_t*)
#define MCPropertyObjectPartThunkSetDouble(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, double)
#define MCPropertyObjectPartThunkSetOptionalDouble(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, double*)
#define MCPropertyObjectPartThunkSetString(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCStringRef)
#define MCPropertyObjectPartThunkSetBinaryString(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCDataRef)
#define MCPropertyObjectPartThunkSetOptionalString(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCStringRef)
#define MCPropertyObjectPartThunkSetRectangle(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCRectangle)
#define MCPropertyObjectPartThunkSetOptionalRectangle(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCRectangle*)
#define MCPropertyObjectPartThunkSetPoint(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCPoint)
#define MCPropertyObjectPartThunkSetCustomType(obj, mth, typ) MCPropertyObjectPartThunkImp(obj, mth, const typ&)
#define MCPropertyObjectPartThunkSetEnumType(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, intenum_t)
#define MCPropertyObjectPartThunkSetSetType(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, intset_t)
#define MCPropertyObjectPartThunkSetOptionalCustomType(obj, mth, typ) MCPropertyObjectPartThunkImp(obj, mth, const typ*&)
#define MCPropertyObjectPartThunkSetOptionalEnumType(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, intenum_t*)
#define MCPropertyObjectPartThunkSetArray(obj, mth) MCPropertyObjectPartThunkImp(obj, mth, MCArrayRef)

#define MCPropertyObjectArrayThunkGetAny(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, MCValueRef&)
#define MCPropertyObjectArrayThunkGetBool(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, bool&)
#define MCPropertyObjectArrayThunkGetString(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, MCStringRef&)
#define MCPropertyObjectArrayThunkGetOptionalEnum(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, intenum_t*&)
#define MCPropertyObjectArrayThunkGetOptionalColor(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, MCColor*&)
#define MCPropertyObjectArrayThunkGetOptionalUInt16(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, uinteger_t*&)

#define MCPropertyObjectArrayThunkSetAny(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, MCValueRef)
#define MCPropertyObjectArrayThunkSetBool(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, bool)
#define MCPropertyObjectArrayThunkSetString(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, MCStringRef)
#define MCPropertyObjectArrayThunkSetOptionalEnum(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, intenum_t*)
#define MCPropertyObjectArrayThunkSetOptionalColor(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, MCColor*)
#define MCPropertyObjectArrayThunkSetOptionalUInt16(obj, mth) MCPropertyObjectArrayThunkImp(obj, mth, uinteger_t*)

#define MCPropertyObjectChunkThunkGetAny(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCValueRef&)
#define MCPropertyObjectChunkThunkGetBool(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, bool&)
#define MCPropertyObjectChunkThunkGetString(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCStringRef&)
#define MCPropertyObjectChunkThunkGetBinaryString(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCDataRef&)
#define MCPropertyObjectChunkThunkGetArray(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCArrayRef&)
#define MCPropertyObjectChunkThunkGetUInt32(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, uinteger_t&)
#define MCPropertyObjectChunkThunkGetInt32(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, integer_t&)
#define MCPropertyObjectChunkThunkGetRectangle(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCRectangle&)
#define MCPropertyObjectChunkThunkGetRectangle32(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCRectangle32&)
#define MCPropertyObjectChunkThunkGetEnumType(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, intenum_t&)
#define MCPropertyObjectChunkThunkGetCustomType(obj, mth, typ) MCPropertyObjectChunkThunkImp(obj, mth, typ&)
#define MCPropertyObjectChunkThunkGetOptionalEnumType(obj, mth, typ) MCPropertyObjectChunkThunkImp(obj, mth, intenum_t*&)

#define MCPropertyObjectChunkThunkSetAny(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCValueRef)
#define MCPropertyObjectChunkThunkSetBool(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, bool)
#define MCPropertyObjectChunkThunkSetString(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCStringRef)
#define MCPropertyObjectChunkThunkSetBinaryString(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCDataRef)
#define MCPropertyObjectChunkThunkSetArray(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCArrayRef)
#define MCPropertyObjectChunkThunkSetUInt32(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, uinteger_t)
#define MCPropertyObjectChunkThunkSetInt32(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, integer_t)
#define MCPropertyObjectChunkThunkSetRectangle(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCRectangle)
#define MCPropertyObjectChunkThunkSetRectangle32(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCRectangle32)
#define MCPropertyObjectChunkThunkSetEnumType(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, intenum_t)
#define MCPropertyObjectChunkThunkSetCustomType(obj, mth, typ) MCPropertyObjectChunkThunkImp(obj, mth, const typ&)
#define MCPropertyObjectChunkThunkSetOptionalEnumType(obj, mth, typ) MCPropertyObjectChunkThunkImp(obj, mth, intenum_t*&)

#define MCPropertyObjectChunkMixedThunkGetBool(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, bool&)
#define MCPropertyObjectChunkMixedThunkGetEnumType(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, intenum_t&)
#define MCPropertyObjectChunkMixedThunkGetCustomType(obj, mth, typ) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, typ&)
#define MCPropertyObjectChunkMixedThunkGetInt16(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, integer_t&)
#define MCPropertyObjectChunkMixedThunkGetUInt8(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, uinteger_t&)
#define MCPropertyObjectChunkMixedThunkGetUInt16(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, uinteger_t&)
#define MCPropertyObjectChunkMixedThunkGetOptionalBool(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, bool*&)
#define MCPropertyObjectChunkMixedThunkGetOptionalInt16(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, integer_t*&)
#define MCPropertyObjectChunkMixedThunkGetOptionalUInt8(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, uinteger_t*&)
#define MCPropertyObjectChunkMixedThunkGetOptionalUInt16(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, uinteger_t*&)
#define MCPropertyObjectChunkMixedThunkGetOptionalEnumType(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, intenum_t*&)
#define MCPropertyObjectChunkMixedThunkGetOptionalString(obj, mth) MCPropertyObjectChunkMixedThunkImp(obj, mth, bool&, MCStringRef&)
#define MCPropertyObjectChunkMixedListThunkGetItemsOfLooseUInt(obj, mth) MCPropertyObjectChunkMixedListThunkImp(obj, mth, bool&, uindex_t&, uinteger_t*&)
#define MCPropertyObjectChunkMixedListThunkGetItemsOfString(obj, mth), MCPropertyObjectChunkMixedListThunkImp(obj, mth, bool&, uindex_t&, MCStringRef*&)

#define MCPropertyObjectChunkMixedThunkSetBool(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, bool)
#define MCPropertyObjectChunkMixedThunkSetEnumType(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, intenum_t)
#define MCPropertyObjectChunkMixedThunkSetCustomType(obj, mth, typ) MCPropertyObjectChunkThunkImp(obj, mth, const typ&)
#define MCPropertyObjectChunkMixedThunkSetInt16(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, integer_t)
#define MCPropertyObjectChunkMixedThunkSetUInt8(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, uinteger_t)
#define MCPropertyObjectChunkMixedThunkSetUInt16(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, uinteger_t)
#define MCPropertyObjectChunkMixedThunkSetOptionalBool(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, bool*)
#define MCPropertyObjectChunkMixedThunkSetOptionalInt16(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, integer_t*)
#define MCPropertyObjectChunkMixedThunkSetOptionalUInt8(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, uinteger_t*)
#define MCPropertyObjectChunkMixedThunkSetOptionalUInt16(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, uinteger_t*)
#define MCPropertyObjectChunkMixedThunkSetOptionalEnumType(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, intenum_t*)
#define MCPropertyObjectChunkMixedThunkSetOptionalString(obj, mth) MCPropertyObjectChunkThunkImp(obj, mth, MCStringRef)
#define MCPropertyObjectChunkMixedListThunkSetItemsOfLooseUInt(obj, mth) MCPropertyObjectChunkListThunkImp(obj, mth, uindex_t, uinteger_t*)
#define MCPropertyObjectChunkMixedListThunkSetItemsOfString(obj, mth) MCPropertyObjectChunkListThunkImp(obj, mth, uindex_t, MCStringRef*)

#define MCPropertyObjectChunkMixedArrayThunkGetOptionalBool(obj, mth) MCPropertyObjectChunkMixedArrayThunkImp(obj, mth, bool&, bool*&)
#define MCPropertyObjectChunkMixedArrayThunkGetBool(obj, mth) MCPropertyObjectChunkMixedArrayThunkImp(obj, mth, bool&, bool&)
#define MCPropertyObjectChunkMixedArrayThunkSetOptionalBool(obj, mth) MCPropertyObjectChunkArrayThunkImp(obj, mth, bool*)

//////////

#define DEFINE_RW_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyThunkGet##type(MC##module##Get##tag), (void *)MCPropertyThunkSet##type(MC##module##Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_SET_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyTypeSet, kMC##type##TypeInfo, (void *)MCPropertyThunkGetSetType(MC##module##Get##tag), (void *)MCPropertyThunkSetSetType(MC##module##Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_ENUM_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyThunkGetEnumType(MC##module##Get##tag), (void *)MCPropertyThunkSetEnumType(MC##module##Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_CUSTOM_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyThunkGetCustomType(MC##module##Get##tag, MC##type), (void *)MCPropertyThunkSetCustomType(MC##module##Set##tag, MC##type), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_ARRAY_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyThunkArrayGet##type(MC##module##Get##tag), (void *)MCPropertyThunkArraySet##type(MC##module##Set##tag), false, true, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyThunkGet##type(MC##module##Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_SET_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyTypeSet, kMC##type##TypeInfo, (void *)MCPropertyThunkGetSetType(MC##module##Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_ENUM_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyThunkGetEnumType(MC##module##Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_CUSTOM_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyThunkGetCustomType(MC##module##Get##tag, MC##type), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_EFFECTIVE_PROPERTY(prop, type, module, tag) \
{ prop, true, kMCPropertyType##type, nil, (void *)MCPropertyThunkGet##type(MC##module##GetEffective##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_ARRAY_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyThunkArrayGet##type(MC##module##Get##tag), nil, false, true, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectThunkGet##type(obj, Get##tag), (void *)MCPropertyObjectThunkSet##type(obj, Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectThunkGet##type(obj, Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectThunkGet##type(obj, Get##tag), (void *)MCPropertyObjectThunkSet##type(obj, Set##tag), true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectThunkGet##type(obj, Get##tag), nil, true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyType##type, nil, (void *)MCPropertyObjectThunkGet##type(obj, GetEffective##tag), (void *)MCPropertyObjectThunkSet##type(obj, SetEffective##tag), true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyType##type, nil, (void *)MCPropertyObjectThunkGet##type(obj, GetEffective##tag), nil, true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_PART_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectPartThunkGet##type(obj, Get##tag), (void *)MCPropertyObjectPartThunkSet##type(obj, Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_PART_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectPartThunkGet##type(obj, Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_WO_OBJ_PART_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, nil, (void *)MCPropertyObjectPartThunkSet##type(obj, Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_PART_NON_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectPartThunkGet##type(obj, Get##tag), (void *)MCPropertyObjectPartThunkSet##type(obj, Set##tag), true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_PART_NON_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectPartThunkGet##type(obj, Get##tag), nil, true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_PART_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyType##type, nil, (void *)MCPropertyObjectPartThunkGet##type(obj, GetEffective##tag), (void *)MCPropertyObjectPartThunkSet##type(obj, SetEffective##tag), true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_PART_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyType##type, nil, (void *)MCPropertyObjectPartThunkGet##type(obj, GetEffective##tag), nil, true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetCustomType(obj, Get##tag, MC##type), (void *)MCPropertyObjectThunkSetCustomType(obj, Set##tag, MC##type), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetCustomType(obj, Get##tag, MC##type), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetCustomType(obj, Get##tag, MC##type), (void *)MCPropertyObjectThunkSetCustomType(obj, Set##tag, MC##type), true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetCustomType(obj, Get##tag, MC##type), nil, true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetCustomType(obj, GetEffective##tag, MC##type), nil, true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_PART_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectPartThunkGetCustomType(obj, Get##tag, MC##type), (void *)MCPropertyObjectPartThunkSetCustomType(obj, Set##tag, MC##type), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetEnumType(obj, Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetEnumType(obj, Get##tag), (void *)MCPropertyObjectThunkSetEnumType(obj, Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_PART_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectPartThunkGetEnumType(obj, Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_OPTIONAL_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeOptionalEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetOptionalEnumType(obj, Get##tag), (void *)MCPropertyObjectThunkSetOptionalEnumType(obj, Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_NON_EFFECTIVE_OPTIONAL_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeOptionalEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetOptionalEnumType(obj, Get##tag), (void *)MCPropertyObjectThunkSetOptionalEnumType(obj, Set##tag), true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_NON_EFFECTIVE_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetEnumType(obj, Get##tag), (void *)MCPropertyObjectThunkSetEnumType(obj, Set##tag), true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_EFFECTIVE_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetEnumType(obj, GetEffective##tag), nil, true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_UNAVAILABLE_OBJ_PROPERTY(prop) \
{ prop, false, kMCPropertyTypeAny, nil, nil, nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_SET_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeSet, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetSetType(obj, Get##tag), (void *)MCPropertyObjectThunkSetSetType(obj, Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_SET_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeSet, kMC##type##TypeInfo, (void *)MCPropertyObjectThunkGetSetType(obj, Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_ARRAY_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectArrayThunkGet##type(obj, Get##tag), (void *)MCPropertyObjectArrayThunkSet##type(obj, Set##tag), false, true, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_ARRAY_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectArrayThunkGet##type(obj, Get##tag), nil, false, true, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_LIST_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectListThunkGet##type(obj, Get##tag), (void *)MCPropertyObjectListThunkSet##type(obj, Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_LIST_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectListThunkGet##type(obj, Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },


#define DEFINE_RO_OBJ_NON_EFFECTIVE_LIST_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectListThunkGet##type(obj, Get##tag), nil, true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_EFFECTIVE_LIST_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyType##type, nil, (void *)MCPropertyObjectListThunkGet##type(obj, GetEffective##tag), nil, true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_OBJ_NON_EFFECTIVE_LIST_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectListThunkGet##type(obj, Get##tag), (void *)MCPropertyObjectListThunkSet##type(obj, Set##tag), true, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_WO_OBJ_CHUNK_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, nil, (void *)MCPropertyObjectChunkThunkSet##type(obj, Set##tag##OfCharChunk), false, false, true },

#define DEFINE_RW_OBJ_NON_EFFECTIVE_MIXED_CHUNK_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedThunkGetOptional##type(obj, Get##tag##OfCharChunk), (void *)MCPropertyObjectChunkMixedThunkSetOptional##type(obj, Set##tag##OfCharChunk), true, false, true },

#define DEFINE_RO_OBJ_EFFECTIVE_MIXED_CHUNK_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedThunkGetOptional##type(obj, GetEffective##tag##OfCharChunk), nil, true, false, true },

/////

#define DEFINE_RW_OBJ_CHAR_CHUNK_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfCharChunk), (void *)MCPropertyObjectChunkThunkSet##type(obj, Set##tag##OfCharChunk), false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfCharChunk), nil, false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfCharChunk), (void *)MCPropertyObjectChunkThunkSet##type(obj, Set##tag##OfCharChunk), true, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_NON_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfCharChunk), nil, true, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfCharChunk), nil, true, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_WO_OBJ_CHAR_CHUNK_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, nil, (void *)MCPropertyObjectChunkThunkSet##type(obj, Set##tag##OfCharChunk), false, false, kMCPropertyInfoChunkTypeChar },

//

#define DEFINE_RW_OBJ_CHAR_CHUNK_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkThunkGetCustomType(obj, Get##tag##OfCharChunk, MC##type), (void *)MCPropertyObjectChunkThunkSetCustomType(obj, Set##tag##OfCharChunk, MC##type), false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RW_OBJ_CHAR_CHUNK_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkThunkGetEnumType(obj, Get##tag##OfCharChunk), (void *)MCPropertyObjectChunkThunkSetEnumType(obj, Set##tag##OfCharChunk), false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkThunkGetEnumType(obj, Get##tag##OfCharChunk), nil, false, false, kMCPropertyInfoChunkTypeChar },

//

#define DEFINE_RW_OBJ_CHAR_CHUNK_MIXED_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedThunkGet##type(obj, Get##tag##OfCharChunk), (void *)MCPropertyObjectChunkMixedThunkSet##type(obj, Set##tag##OfCharChunk), false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_MIXED_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedThunkGet##type(obj, Get##tag##OfCharChunk), nil, false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedThunkGet##type(obj, Get##tag##OfCharChunk), (void *)MCPropertyObjectChunkMixedThunkSet##type(obj, Set##tag##OfCharChunk), true, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedThunkGet##type(obj, GetEffective##tag##OfCharChunk), nil, true, false, kMCPropertyInfoChunkTypeChar },

//

#define DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_ARRAY_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedArrayThunkGet##type(obj, Get##tag##OfCharChunk), (void *)MCPropertyObjectChunkMixedArrayThunkSet##type(obj, Set##tag##OfCharChunk), true, true, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_ARRAY_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedArrayThunkGet##type(obj, GetEffective##tag##OfCharChunk), nil, true, true, kMCPropertyInfoChunkTypeChar },

//

#define DEFINE_RW_OBJ_CHAR_CHUNK_MIXED_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetCustomType(obj, Get##tag##OfCharChunk, MC##type), (void *)MCPropertyObjectChunkMixedThunkSetCustomType(obj, Set##tag##OfCharChunk, MC##type), false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_MIXED_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetCustomType(obj, Get##tag##OfCharChunk, MC##type), nil, false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetCustomType(obj, Get##tag##OfCharChunk, MC##type), (void *)MCPropertyObjectChunkMixedThunkSetCustomType(obj, Set##tag##OfCharChunk, MC##type), true, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeMixedCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetCustomType(obj, GetEffective##tag##OfCharChunk, MC##type), nil, true, false, kMCPropertyInfoChunkTypeChar },

//

#define DEFINE_RW_OBJ_CHAR_CHUNK_MIXED_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetEnumType(obj, Get##tag##OfCharChunk), (void *)MCPropertyObjectChunkMixedThunkSetEnumType(obj, Set##tag##OfCharChunk), false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_MIXED_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetEnumType(obj, Get##tag##OfCharChunk), nil, false, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RW_OBJ_CHAR_CHUNK_NON_EFFECTIVE_MIXED_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetEnumType(obj, Get##tag##OfCharChunk), (void *)MCPropertyObjectChunkMixedThunkSetEnumType(obj, Set##tag##OfCharChunk), true, false, kMCPropertyInfoChunkTypeChar },

#define DEFINE_RO_OBJ_CHAR_CHUNK_EFFECTIVE_MIXED_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeMixedEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetEnumType(obj, GetEffective##tag##OfCharChunk), nil, true, false, kMCPropertyInfoChunkTypeChar },


/////

#define DEFINE_RW_OBJ_LINE_CHUNK_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfLineChunk), (void *)MCPropertyObjectChunkThunkSet##type(obj, Set##tag##OfLineChunk), false, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RO_OBJ_LINE_CHUNK_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfLineChunk), nil, false, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfLineChunk), (void *)MCPropertyObjectChunkThunkSet##type(obj, Set##tag##OfLineChunk), true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RO_OBJ_LINE_CHUNK_NON_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfLineChunk), nil, true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyType##type, nil, (void *)MCPropertyObjectChunkThunkGet##type(obj, Get##tag##OfLineChunk), nil, true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_WO_OBJ_LINE_CHUNK_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyType##type, nil, nil, (void *)MCPropertyObjectChunkThunkSet##type(obj, Set##tag##OfLineChunk), false, false, kMCPropertyInfoChunkTypeLine },

////

#define DEFINE_RW_OBJ_LINE_CHUNK_MIXED_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedThunkGet##type(obj, Get##tag##OfLineChunk), (void *)MCPropertyObjectChunkMixedThunkSet##type(obj, Set##tag##OfLineChunk), false, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RW_OBJ_LINE_CHUNK_MIXED_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetCustomType(obj, Get##tag##OfLineChunk, MC##type), (void *)MCPropertyObjectChunkMixedThunkSetCustomType(obj, Set##tag##OfLineChunk, MC##type), false, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetCustomType(obj, Get##tag##OfLineChunk, MC##type), (void *)MCPropertyObjectChunkMixedThunkSetCustomType(obj, Set##tag##OfLineChunk, MC##type), true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_CUSTOM_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeMixedCustom, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetCustomType(obj, GetEffective##tag##OfLineChunk, MC##type), nil, true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RW_OBJ_LINE_CHUNK_MIXED_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetEnumType(obj, Get##tag##OfLineChunk), (void *)MCPropertyObjectChunkMixedThunkSetEnumType(obj, Set##tag##OfLineChunk), false, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedThunkGet##type(obj, Get##tag##OfLineChunk), (void *)MCPropertyObjectChunkMixedThunkSet##type(obj, Set##tag##OfLineChunk), true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_LIST_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedListThunkGet##type(obj, Get##tag##OfLineChunk), (void *)MCPropertyObjectChunkMixedListThunkSet##type(obj, Set##tag##OfLineChunk), true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RW_OBJ_LINE_CHUNK_NON_EFFECTIVE_MIXED_OPTIONAL_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, false, kMCPropertyTypeMixedOptionalEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetOptionalEnumType(obj, Get##tag##OfLineChunk), (void *)MCPropertyObjectChunkMixedThunkSetOptionalEnumType(obj, Set##tag##OfLineChunk), true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedThunkGet##type(obj, GetEffective##tag##OfLineChunk), nil, true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_ENUM_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeMixedEnum, kMC##type##TypeInfo, (void *)MCPropertyObjectChunkMixedThunkGetEnumType(obj, GetEffective##tag##OfLineChunk), nil, true, false, kMCPropertyInfoChunkTypeLine },

#define DEFINE_RO_OBJ_LINE_CHUNK_EFFECTIVE_MIXED_LIST_PROPERTY(prop, type, obj, tag) \
{ prop, true, kMCPropertyTypeMixed##type, nil, (void *)MCPropertyObjectChunkMixedListThunkGet##type(obj, GetEffective##tag##OfLineChunk), nil, true, false, kMCPropertyInfoChunkTypeLine },

////

template<typename O, typename A, void (O::*Method)(MCExecContext&, MCNameRef, A)> inline void MCPropertyObjectRecordPropThunk(MCExecContext& ctxt, MCObjectIndexPtr *obj, A arg)
{
    if (obj -> index == nil)
        (static_cast<O *>(obj -> object) ->*Method)(ctxt, kMCEmptyName, arg);
    else
        (static_cast<O *>(obj -> object) ->*Method)(ctxt, obj -> index, arg);
}

template<typename O, typename A, void (O::*Method)(MCExecContext&, MCNameRef, A)> inline void MCPropertyObjectRecordThunk(MCExecContext& ctxt, MCObjectPtr *obj, A arg)
{
    (static_cast<O *>(obj -> object) ->*Method)(ctxt, kMCEmptyName, arg);
}

#define MCPropertyObjectGetRecordPropThunkImp(obj, mth) (void(*)(MCExecContext&,MCObjectIndexPtr*,MCExecValue&))MCPropertyObjectRecordPropThunk<obj,MCExecValue&, &obj::mth>
#define MCPropertyObjectSetRecordPropThunkImp(obj,mth) (void(*)(MCExecContext&,MCObjectIndexPtr*,MCExecValue))MCPropertyObjectRecordPropThunk<obj,MCExecValue, &obj::mth>
#define MCPropertyObjectGetRecordThunkImp(obj, mth) (void(*)(MCExecContext&,MCObjectPtr*,MCExecValue&))MCPropertyObjectRecordThunk<obj,MCExecValue&, &obj::mth>
#define MCPropertyObjectSetRecordThunkImp(obj,mth) (void(*)(MCExecContext&,MCObjectPtr*,MCExecValue))MCPropertyObjectRecordThunk<obj,MCExecValue, &obj::mth>

#define DEFINE_RW_OBJ_RECORD_PROPERTY(prop, obj, tag) \
{ prop, false, kMCPropertyTypeRecord, nil, (void *)MCPropertyObjectGetRecordPropThunkImp(obj, Get##tag##Property), (void *)MCPropertyObjectSetRecordPropThunkImp(obj, Set##tag##Property), false, true, kMCPropertyInfoChunkTypeNone }, { prop, false, kMCPropertyTypeRecord, nil, (void *)MCPropertyObjectGetRecordThunkImp(obj, Get##tag##Property), (void *)MCPropertyObjectSetRecordThunkImp(obj, Set##tag##Property), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_OBJ_RECORD_PROPERTY(prop, obj, tag) \
{ prop, false, kMCPropertyTypeRecord, nil, (void *)MCPropertyObjectGetRecordPropThunkImp(obj, Get##tag##Property), nil, false, true, kMCPropertyInfoChunkTypeNone }, { prop, false, kMCPropertyTypeRecord, nil, (void *)MCPropertyObjectGetRecordThunkImp(obj, Get##tag##Property), nil, false, false, kMCPropertyInfoChunkTypeNone },

////////////////////////////////////////////////////////////////////////////////

class MCNativeControl;
struct MCNativeControlPtr
{
    MCNativeControl *control;
};

template<typename C, typename A, void (C::*Method)(MCExecContext&, A)> inline void MCPropertyNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr* ctrl, A arg)
{
	(static_cast<C *>(ctrl -> control) ->* Method)(ctxt, arg);
}

#define MCPropertyNativeControlThunkImp(ctrl, mth, typ) (void(*)(MCExecContext&,MCNativeControlPtr*,typ))MCPropertyNativeControlThunk<ctrl,typ,&ctrl::mth>

#define MCPropertyNativeControlThunkGetBool(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, bool&)
#define MCPropertyNativeControlThunkGetInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t&)
#define MCPropertyNativeControlThunkGetInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t&)
#define MCPropertyNativeControlThunkGetInt32X2(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t*)
#define MCPropertyNativeControlThunkGetInt32X4(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t*)
#define MCPropertyNativeControlThunkGetUInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t&)
#define MCPropertyNativeControlThunkGetUInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t&)
#define MCPropertyNativeControlThunkGetDouble(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, double&)
#define MCPropertyNativeControlThunkGetString(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCStringRef&)
#define MCPropertyNativeControlThunkGetOptionalString(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCStringRef&)
#define MCPropertyNativeControlThunkGetRectangle(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle&)
#define MCPropertyNativeControlThunkGetCustomType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ&)
#define MCPropertyNativeControlThunkGetEnumType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ&)
#define MCPropertyNativeControlThunkGetSetType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ&)

#define MCPropertyNativeControlThunkSetBool(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, bool)
#define MCPropertyNativeControlThunkSetInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t)
#define MCPropertyNativeControlThunkSetInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t)
#define MCPropertyNativeControlThunkSetInt32X2(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t*)
#define MCPropertyNativeControlThunkSetInt32X4(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, integer_t*)
#define MCPropertyNativeControlThunkSetUInt16(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t)
#define MCPropertyNativeControlThunkSetUInt32(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, uinteger_t)
#define MCPropertyNativeControlThunkSetDouble(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, double)
#define MCPropertyNativeControlThunkSetString(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCStringRef)
#define MCPropertyNativeControlThunkSetOptionalString(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCStringRef)
#define MCPropertyNativeControlThunkSetRectangle(ctrl, mth) MCPropertyNativeControlThunkImp(ctrl, mth, MCRectangle)
#define MCPropertyNativeControlThunkSetCustomType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, const typ&)
#define MCPropertyNativeControlThunkSetEnumType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ)
#define MCPropertyNativeControlThunkSetSetType(ctrl, mth, typ) MCPropertyNativeControlThunkImp(ctrl, mth, typ)

#define DEFINE_RW_CTRL_PROPERTY(prop, type, ctrl, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyNativeControlThunkGet##type(ctrl, Get##tag), (void *)MCPropertyNativeControlThunkSet##type(ctrl, Set##tag), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_CTRL_PROPERTY(prop, type, ctrl, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPropertyNativeControlThunkGet##type(ctrl, Get##tag), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_CTRL_CUSTOM_PROPERTY(prop, type, ctrl, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetCustomType(ctrl, Get##tag, MC##type), (void *)MCPropertyNativeControlThunkSetCustomType(ctrl, Set##tag, MC##type), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_CTRL_CUSTOM_PROPERTY(prop, type, ctrl, tag) \
{ prop, false, kMCPropertyTypeCustom, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetCustomType(ctrl, Get##tag, MC##type), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_CTRL_ENUM_PROPERTY(prop, type, ctrl, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetEnumType(ctrl, Get##tag, MC##type), nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_CTRL_ENUM_PROPERTY(prop, type, ctrl, tag) \
{ prop, false, kMCPropertyTypeEnum, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetEnumType(ctrl, Get##tag, MC##type), (void *)MCPropertyNativeControlThunkSetEnumType(ctrl, Set##tag, MC##type), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_UNAVAILABLE_CTRL_PROPERTY(prop) \
{ prop, false, kMCPropertyTypeAny, nil, nil, nil, false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RW_CTRL_SET_PROPERTY(prop, type, ctrl, tag) \
{ prop, false, kMCPropertyTypeSet, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetSetType(ctrl, Get##tag, MC##type), (void *)MCPropertyNativeControlThunkSetSetType(ctrl, Set##tag, MC##type), false, false, kMCPropertyInfoChunkTypeNone },

#define DEFINE_RO_CTRL_SET_PROPERTY(prop, type, ctrl, tag) \
{ prop, false, kMCPropertyTypeSet, kMC##type##TypeInfo, (void *)MCPropertyNativeControlThunkGetSetType(ctrl, Get##tag, MC##type), nil, false, false, kMCPropertyInfoChunkTypeNone },

////////////////////////////////////////////////////////////////////////////////

template<typename C, typename X, typename Y, typename Z, void (C::*Method)(MCExecContext&, X, Y, Z)> inline void MCExecNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr* ctrl, X param1, Y param2, Z param3)
{
	(static_cast<C *>(ctrl -> control) ->* Method)(ctxt, param1, param2, param3);
}

template<typename C, typename X, typename Y, void (C::*Method)(MCExecContext&, X, Y)> inline void MCExecNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr* ctrl, X param1, Y param2)
{
	(static_cast<C *>(ctrl -> control) ->* Method)(ctxt, param1, param2);
}

template<typename C, typename X, void (C::*Method)(MCExecContext&, X)> inline void MCExecNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr* ctrl, X param1)
{
	(static_cast<C *>(ctrl -> control) ->* Method)(ctxt, param1);
}

template<typename C, void (C::*Method)(MCExecContext&)> inline void MCExecNativeControlThunk(MCExecContext& ctxt, MCNativeControlPtr* ctrl)
{
	(static_cast<C *>(ctrl -> control) ->* Method)(ctxt);
}

#define MCExecNativeControlThunkExec(ctrl, mth) (void(*)(MCExecContext&,MCNativeControlPtr*))MCExecNativeControlThunk<ctrl,&ctrl::mth>

#define MCExecNativeControlUnaryThunkImp(ctrl, mth, typ) (void(*)(MCExecContext&,MCNativeControlPtr*,typ))MCExecNativeControlThunk<ctrl,typ,&ctrl::mth>
#define MCExecNativeControlThunkExecString(ctrl, mth) MCExecNativeControlUnaryThunkImp(ctrl, mth, MCStringRef)
#define MCExecNativeControlThunkExecInt32(ctrl, mth) MCExecNativeControlUnaryThunkImp(ctrl, mth, integer_t)
#define MCExecNativeControlThunkExecOptionalInt32(ctrl, mth) MCExecNativeControlUnaryThunkImp(ctrl, mth, integer_t*)

#define MCExecNativeControlBinaryThunkImp(ctrl, mth, typ1, typ2) (void(*)(MCExecContext&,MCNativeControlPtr*,typ1,typ2))MCExecNativeControlThunk<ctrl,typ1,typ2,&ctrl::mth>
#define MCExecNativeControlThunkExecStringString(ctrl, mth) MCExecNativeControlBinaryThunkImp(ctrl, mth, MCStringRef, MCStringRef)
#define MCExecNativeControlThunkExecInt32Int32(ctrl, mth) MCExecNativeControlBinaryThunkImp(ctrl, mth, integer_t, integer_t)

#define MCExecNativeControlTernaryThunkImp(ctrl, mth, typ1, typ2, typ3) (void(*)(MCExecContext&,MCNativeControlPtr*,typ1,typ2,typ3))MCExecNativeControlThunk<ctrl,typ1,typ2,typ3,&ctrl::mth>
#define MCExecNativeControlThunkExecInt32OptionalInt32OptionalInt32(ctrl, mth) MCExecNativeControlTernaryThunkImp(ctrl, mth, integer_t, integer_t*, integer_t*)

#define DEFINE_CTRL_EXEC_METHOD(act, actsig, ctrl, tag) \
{ false, (MCNativeControlAction)kMCNativeControlAction##act, (MCNativeControlActionSignature)kMCNativeControlActionSignature_##actsig, (void *)MCExecNativeControlThunkExec(ctrl, Exec##tag) },

#define DEFINE_CTRL_EXEC_UNARY_METHOD(act, actsig, ctrl, param1, tag) \
{ false, (MCNativeControlAction)kMCNativeControlAction##act, (MCNativeControlActionSignature)kMCNativeControlActionSignature_##actsig, (void *)MCExecNativeControlThunkExec##param1(ctrl, Exec##tag) },

#define DEFINE_CTRL_EXEC_BINARY_METHOD(act, actsig, ctrl, param1, param2, tag) \
{ false, (MCNativeControlAction)kMCNativeControlAction##act, (MCNativeControlActionSignature)kMCNativeControlActionSignature_##actsig, (void *)MCExecNativeControlThunkExec##param1##param2(ctrl, Exec##tag) },

#define DEFINE_CTRL_EXEC_TERNARY_METHOD(act, actsig, ctrl, param1, param2, param3, tag) \
{ false, (MCNativeControlAction)kMCNativeControlAction##act, (MCNativeControlActionSignature)kMCNativeControlActionSignature_##actsig, (void *)MCExecNativeControlThunkExec##param1##param2##param3(ctrl, Exec##tag) },

#define DEFINE_CTRL_WAITABLE_EXEC_METHOD(act, actsig, ctrl, tag) \
{ true, (MCNativeControlAction)kMCNativeControlAction##act, (MCNativeControlActionSignature)kMCNativeControlActionSignature_##actsig, (void *)MCExecNativeControlThunkExec(ctrl, Exec##tag) },

#define DEFINE_CTRL_WAITABLE_EXEC_UNARY_METHOD(act, actsig, ctrl, param1, tag) \
{ true, (MCNativeControlAction)kMCNativeControlAction##act, (MCNativeControlActionSignature)kMCNativeControlActionSignature_##actsig, (void *)MCExecNativeControlThunkExec##param1(ctrl, Exec##tag) },

////////////////////////////////////////////////////////////////////////////////


enum MCPurchaseState
{
	kMCPurchaseStateInitialized,
	kMCPurchaseStateSendingRequest,
	kMCPurchaseStatePaymentReceived,
	kMCPurchaseStateComplete,
	kMCPurchaseStateRestored,
	kMCPurchaseStateCancelled,
    //Amazon
    kMCPurchaseStateInvalidSKU,
    kMCPurchaseStateAlreadyEntitled,
	kMCPurchaseStateRefunded,
	kMCPurchaseStateError,
    kMCPurchaseStateUnverified,
	
	kMCPurchaseStateUnknown,
};

typedef struct _mcpurchase_t
{
    MCStringRef         prod_id;
	uint32_t			id;
	MCPurchaseState		state;
	uint32_t			ref_count;
	
	void *				platform_data;
	
	struct _mcpurchase_t *	next;
} MCPurchase;

template<typename A, void Method(MCExecContext&, MCPurchase *, A)> inline void MCPurchasePropertyThunk(MCExecContext& ctxt, MCPurchase *purchase, A arg)
{
    Method(ctxt, purchase, arg);
}

#define MCPurchasePropertyThunkImp(mth,typ) (void(*)(MCExecContext&, MCPurchase *,typ))MCPurchasePropertyThunk<typ,mth>

#define MCPurchasePropertyThunkGetInt32(mth) MCPurchasePropertyThunkImp(mth, integer_t&)
#define MCPurchasePropertyThunkGetUInt32(mth) MCPurchasePropertyThunkImp(mth, uinteger_t&)
#define MCPurchasePropertyThunkGetString(mth) MCPurchasePropertyThunkImp(mth, MCStringRef&)
#define MCPurchasePropertyThunkGetBinaryString(mth) MCPurchasePropertyThunkImp(mth, MCDataRef&)

#define MCPurchasePropertyThunkSetInt32(mth) MCPurchasePropertyThunkImp(mth, integer_t)
#define MCPurchasePropertyThunkSetUInt32(mth) MCPurchasePropertyThunkImp(mth, uinteger_t)
#define MCPurchasePropertyThunkSetString(mth) MCPurchasePropertyThunkImp(mth, MCStringRef)
#define MCPurchasePropertyThunkSetBinaryString(mth) MCPurchasePropertyThunkImp(mth, MCDataRef)

#define DEFINE_RW_STORE_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPurchasePropertyThunkGet##type(MC##module##Get##tag), (void *)MCPurchasePropertyThunkSet##type(MC##module##Set##tag) },

#define DEFINE_RO_STORE_PROPERTY(prop, type, module, tag) \
{ prop, false, kMCPropertyType##type, nil, (void *)MCPurchasePropertyThunkGet##type(MC##module##Get##tag), nil },

////////////////////////////////////////////////////////////////////////////////

void MCExecFetchProperty(MCExecContext& ctxt, const MCPropertyInfo *prop, void *mark, MCExecValue& r_value);
void MCExecStoreProperty(MCExecContext& ctxt, const MCPropertyInfo *prop, void *mark, MCExecValue p_value);

////////////////////////////////////////////////////////////////////////////////

class MCExecContext
{
public:
    MCExecContext()
    {
        memset(this, 0, sizeof(MCExecContext));
        m_itemdel = MCValueRetain(kMCCommaString);
        m_columndel = MCValueRetain(kMCTabString);
        m_rowdel = MCValueRetain(kMCLineEndString);
        m_linedel = MCValueRetain(kMCLineEndString);
        m_nffw = 8;
        m_nftrailing = 6;
        m_cutoff = 35;
        m_stat = ES_NORMAL;
        m_string_options = kMCStringOptionCompareCaseless;
    }

	
    MCExecContext(const MCExecContext& p_ctxt)
        : m_stat(ES_NORMAL)
	{
        *this = p_ctxt;
        MCValueRetain(p_ctxt . m_itemdel);
        MCValueRetain(p_ctxt . m_linedel);
        MCValueRetain(p_ctxt . m_rowdel);
        MCValueRetain(p_ctxt . m_columndel);
	}

    MCExecContext(MCObject *object, MCHandlerlist *hlist, MCHandler *handler)
    {
        memset(this, 0, sizeof(MCExecContext));
        m_object . object = object;
        m_object . part_id = 0;
        m_hlist = hlist;
        m_curhandler = handler;
        m_itemdel = MCValueRetain(kMCCommaString);
        m_columndel = MCValueRetain(kMCTabString);
        m_rowdel = MCValueRetain(kMCLineEndString);
        m_linedel = MCValueRetain(kMCLineEndString);
        m_nffw = 8;
        m_nftrailing = 6;
        m_cutoff = 35;
        m_stat = ES_NORMAL;
        m_string_options = kMCStringOptionCompareCaseless;
    }

    ~MCExecContext()
    {
        MCValueRelease(m_itemdel);
        MCValueRelease(m_linedel);
        MCValueRelease(m_rowdel);
        MCValueRelease(m_columndel);
    }
    
	//////////

	
	Exec_stat GetExecStat(void)
	{
		return m_stat;
	}

    void SetExecStat(Exec_stat p_stat)
    {
        m_stat = p_stat;
    }
    
	///////////

	bool HasError(void)
	{
        return (m_stat == ES_ERROR || m_stat == ES_NOT_FOUND || m_stat == ES_NOT_HANDLED);
	}

	void Throw(void)
	{
		m_stat = ES_ERROR;
	}
	
	void IgnoreLastError()
	{
		m_stat = ES_NORMAL;
	}

    void SetIsReturnHandler()
    {
        m_stat = ES_RETURN_HANDLER;
    }

	Exec_stat Catch(uint2 line, uint2 pos);

	void LegacyThrow(Exec_errors error, MCValueRef hint = nil);
	void LegacyThrow(Exec_errors error, uint32_t hint);
	void UserThrow(MCStringRef string);

	void Unimplemented(void)
	{
		abort();
	}
    
	//////////

	bool GetCaseSensitive(void) const
	{
        return (m_string_options & kMCStringOptionFoldBit) == 0;
	}
    
    bool GetFormSensitive(void) const
    {
        return (m_string_options & kMCStringOptionNormalizeBit) == 0;
    }
    
    MCStringOptions GetStringComparisonType() const
    {
        return m_string_options;
    }

	bool GetConvertOctals(void) const
	{
        return m_convertoctals == True;
	}

	bool GetWholeMatches(void) const
	{
        return m_wholematches == True;
	}

	bool GetUseUnicode(void) const
	{
        return m_useunicode == True;
	}

	bool GetUseSystemDate(void) const
	{
        return m_usesystemdate == True;
	}

	MCStringRef GetLineDelimiter(void) const
	{
        return m_linedel;
	}

	MCStringRef GetItemDelimiter(void) const
	{
        return m_itemdel;
	}

	MCStringRef GetColumnDelimiter(void) const
	{
        return m_columndel;
	}

	MCStringRef GetRowDelimiter(void) const
	{
        return m_rowdel;
	}

	uint2 GetCutOff(void) const
	{
        return m_cutoff;
	}

	uinteger_t GetNumberFormatWidth() const
	{
        return m_nffw;
	}
	
	uinteger_t GetNumberFormatTrailing() const
	{
        return m_nftrailing;
	}
	
	uinteger_t GetNumberFormatForce() const
	{
        return m_nfforce;
	}
	
	//////////

	void SetNumberFormat(uint2 p_fw, uint2 p_trailing, uint2 p_force)
    {
        m_nffw = p_fw;
        m_nftrailing = p_trailing;
        m_nfforce = p_force;
    }

	void SetCaseSensitive(bool p_value)
	{
        if (p_value)
            m_string_options &= ~(unsigned)kMCStringOptionFoldBit;
        else
            m_string_options |= kMCStringOptionFoldBit;
	}
    
    void SetFormSensitive(bool p_value)
    {
        if (p_value)
            m_string_options &= ~(unsigned)kMCStringOptionNormalizeBit;
        else
            m_string_options |= kMCStringOptionNormalizeBit;
    }

	void SetConvertOctals(bool p_value)
	{
        m_convertoctals = p_value;
	}
	
	void SetWholeMatches(bool p_value)
	{
        m_wholematches = p_value;
	}
	
	void SetUseUnicode(bool p_value)
	{
        m_useunicode = p_value;
	}

	void SetUseSystemDate(bool p_value)
	{
        m_usesystemdate = p_value;
	}

	void SetCutOff(uint2 p_value)
	{
        m_cutoff = p_value;
	}

	void SetLineDelimiter(MCStringRef p_value)
	{
        MCValueAssign(m_linedel, p_value);
	}

	void SetItemDelimiter(MCStringRef p_value)
	{
        MCValueAssign(m_itemdel, p_value);
	}

	void SetColumnDelimiter(MCStringRef p_value)
	{
        MCValueAssign(m_columndel, p_value);
	}

	void SetRowDelimiter(MCStringRef p_value)
	{
        MCValueAssign(m_rowdel, p_value);
    }

    //////////
	
	// Convert the given valueref to a string. If the type is not convertable
	// to a string, the empty string is returned.
	// This method should be used in cases where a string is required and
	// we want to silently convert non-stringables to empty.
	bool ForceToString(MCValueRef value, MCStringRef& r_string);
	bool ForceToBoolean(MCValueRef value, MCBooleanRef& r_boolean);

	// These attempt to convert the value as specified, returning 'true' if successeful.
	// These will raise an appropriate error if the conversion fails and
	// strict mode is on. If strict mode is off (the default) then default
	// values will be returned for things that can't be converted.
	//   (boolean - false, string - empty, number/integer/real - 0, array -
	//    empty array).
	bool ConvertToBoolean(MCValueRef value, MCBooleanRef& r_boolean);
	bool ConvertToString(MCValueRef value, MCStringRef& r_string);
    bool ConvertToData(MCValueRef p_value, MCDataRef& r_data);
    bool ConvertToName(MCValueRef p_value, MCNameRef& r_data);
	bool ConvertToNumber(MCValueRef value, MCNumberRef& r_number);
    // SN-2014-12-03: [[ Bug 14147 ]] Some conversions to an array might not accept a string
	bool ConvertToArray(MCValueRef value, MCArrayRef& r_array, bool p_strict = false);
    
    bool ConvertToBool(MCValueRef value, bool& r_bool);
	bool ConvertToInteger(MCValueRef value, integer_t& r_integer);
	bool ConvertToUnsignedInteger(MCValueRef value, uinteger_t& r_integer);
	bool ConvertToReal(MCValueRef value, real64_t& r_real);
	bool ConvertToChar(MCValueRef value, char_t& r_real);
	bool ConvertToLegacyPoint(MCValueRef value, MCPoint& r_point);
	bool ConvertToLegacyRectangle(MCValueRef value, MCRectangle& r_rectangle);
	bool ConvertToLegacyColor(MCValueRef value, MCColor& r_color);
    
    bool ConvertToMutableString(MCValueRef p_value, MCStringRef &r_string);
    bool ConvertToNumberOrArray(MCExecValue &x_value);
    
	// These attempt to convert the given value as specified. If conversion
	// was successful then 'r_converted' is set to true, else 'false'. If
	// an error occurs (such as out-of-memory), false is returned.
	bool TryToConvertToBoolean(MCValueRef value, bool& r_converted, MCBooleanRef& r_boolean);
	bool TryToConvertToString(MCValueRef value, bool& r_converted, MCStringRef& r_string);
	bool TryToConvertToNumber(MCValueRef value, bool& r_converted, MCNumberRef& r_number);
	bool TryToConvertToInteger(MCValueRef value, bool& r_converted, integer_t& r_integer);
	bool TryToConvertToUnsignedInteger(MCValueRef value, bool& r_converted, uinteger_t& r_integer);
	bool TryToConvertToReal(MCValueRef value, bool& r_converted, real64_t& r_real);
	bool TryToConvertToArray(MCValueRef value, bool& r_converted, MCArrayRef& r_array);
	bool TryToConvertToLegacyPoint(MCValueRef value, bool& r_converted, MCPoint& r_point);
	bool TryToConvertToLegacyRectangle(MCValueRef value, bool& r_converted, MCRectangle& r_rectangle);
	bool TryToConvertToLegacyColor(MCValueRef value, bool& r_converted, MCColor& r_color);

	//////////
	
	bool CopyElementAsBoolean(MCArrayRef, MCNameRef key, bool case_sensitive, MCBooleanRef &r_boolean);
	bool CopyElementAsString(MCArrayRef, MCNameRef key, bool case_sensitive, MCStringRef &r_string);
	bool CopyElementAsNumber(MCArrayRef, MCNameRef key, bool case_sensitive, MCNumberRef &r_number);
	bool CopyElementAsInteger(MCArrayRef, MCNameRef key, bool case_sensitive, integer_t &r_integer);
	bool CopyElementAsUnsignedInteger(MCArrayRef, MCNameRef key, bool case_sensitive, uinteger_t &r_integer);
	bool CopyElementAsReal(MCArrayRef, MCNameRef key, bool case_sensitive, real64_t &r_real);
	bool CopyElementAsArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_array);
	
	bool CopyElementAsStringArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_string_array);
	bool CopyElementAsFilepath(MCArrayRef, MCNameRef key, bool case_sensitive, MCStringRef &r_path);
	bool CopyElementAsFilepathArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_path_array);
	
	bool CopyElementAsEnum(MCArrayRef, MCNameRef key, bool case_sensitive, MCExecEnumTypeInfo *enum_type_info, intenum_t &r_intenum);

	//////////
	
	bool CopyOptElementAsBoolean(MCArrayRef, MCNameRef key, bool case_sensitive, MCBooleanRef &r_boolean);
	bool CopyOptElementAsString(MCArrayRef, MCNameRef key, bool case_sensitive, MCStringRef &r_string);
	bool CopyOptElementAsStringArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_string_array);
	bool CopyOptElementAsFilepath(MCArrayRef, MCNameRef key, bool case_sensitive, MCStringRef &r_path);
	bool CopyOptElementAsFilepathArray(MCArrayRef, MCNameRef key, bool case_sensitive, MCArrayRef &r_path_array);
	bool CopyOptElementAsArray(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCArrayRef &r_array);
	
	//////////
	
	bool FormatBool(bool p_bool, MCStringRef& r_output);
	bool FormatReal(real64_t p_real, MCStringRef& r_output);
	bool FormatUnsignedInteger(uinteger_t p_integer, MCStringRef& r_output);
	bool FormatInteger(integer_t p_integer, MCStringRef& r_output);
	bool FormatLegacyPoint(MCPoint value, MCStringRef& r_value);
	bool FormatLegacyRectangle(MCRectangle value, MCStringRef& r_value);
	bool FormatLegacyColor(MCColor value, MCStringRef& r_value);

	
	//////////

	// This method evaluates the given expression returning the result in 'result'.
	// If an error is raised during the course of evaluation, 'false' is returned.
	// Note: This method throws any errors that occur.
	bool EvaluateExpression(MCExpression *expr, Exec_errors p_error, MCExecValue& r_result);
    
    // These methods try to evaluate / set, as many times as the debug context dictates,
    // only throwing an error if they ultimately fail.
    bool TryToEvaluateExpression(MCExpression *p_expr, uint2 line, uint2 pos, Exec_errors p_error, MCValueRef& r_result);
    bool TryToEvaluateExpressionAsDouble(MCExpression *p_expr, uint2 line, uint2 pos, Exec_errors p_error, double& r_result);
    bool TryToEvaluateParameter(MCParameter *p_param, uint2 line, uint2 pos, Exec_errors p_error, MCExecValue& r_result);
    bool TryToEvaluateExpressionAsNonStrictBool(MCExpression * p_expr, uint2 line, uint2 pos, Exec_errors p_error, bool& r_result);
    bool TryToSetVariable(MCVarref *p_var, uint2 line, uint2 pos, Exec_errors p_error, MCExecValue p_value);
    
	//////////
	
	// Note: This method throws any errors that occur.
	bool EnsurePrintingIsAllowed(void);
	bool EnsureDiskAccessIsAllowed(void);
	bool EnsureProcessIsAllowed(void);
	bool EnsureNetworkAccessIsAllowed(void);
	bool EnsurePrivacyIsAllowed(void);

	//////////
	
    MCVarref *GetIt() const;
	void SetItToEmpty(void);
	void SetItToValue(MCValueRef p_value);
    
    // Assign the given ExecValue to it, the 'it' variable takes ownership.
	void GiveValueToIt(/* take */ MCExecValue& p_value);
	
	//////////    

    // MW-2011-06-22: [[ SERVER ]] Provides augmented functionality for finding
    //   variables if there is no handler (i.e. global server scope).
    Parse_stat FindVar(MCNameRef p_name, MCVarref** r_var);

    //////////

    MCHandler *GetHandler(void) const
    {
        return m_curhandler;
    }
	
	void SetHandler(MCHandler *p_handler)
	{
        m_curhandler = p_handler;
	}
	
    MCHandlerlist *GetHandlerList() const
	{
        return m_hlist;
	}
	
	void SetHandlerList(MCHandlerlist *p_list)
	{
        m_hlist = p_list;
	}
    
    MCObject *GetObject(void) const
	{
        return m_object . object;
	}

    MCObjectPtr GetObjectPtr(void) const
    {
        return m_object;
    }
    
    void SetObject(MCObject *p_object)
    {
        m_object . object = p_object;
        m_object . part_id = 0;
    }
    
	void SetObjectPtr(MCObjectPtr p_object)
	{
        m_object = p_object;
    }

    uint2 GetLine() const
    {
        return m_line;
    }

    void SetLine(uint2 p_line)
    {
        m_line = p_line;
    }
    
    uint2 GetPos() const
    {
        return m_pos;
    }
    
    void SetPos(uint2 p_pos)
    {
        m_pos = p_pos;
    }
    
    void SetLineAndPos(uint2 p_line, uint2 p_pos)
    {
        m_line = p_line;
        m_pos = p_pos;
    }
    
	void SetParentScript(MCParentScriptUse *p_parentscript)
	{
        m_parentscript = p_parentscript;
	}

    MCParentScriptUse *GetParentScript(void) const
	{
        return m_parentscript;
	}
    
    // MM-2011-02-16: Added ability to get handle of current object
    MCObjectHandle GetObjectHandle(void) const;
	void SetTheResultToEmpty(void);
	void SetTheResultToValue(MCValueRef p_value);
	void SetTheResultToStaticCString(const char *p_cstring);
    void SetTheResultToNumber(real64_t p_value);
    void GiveCStringToResult(char *p_cstring);
    void SetTheResultToCString(const char *p_string);
    void SetTheResultToBool(bool p_bool);
    
    void SetTheReturnValue(MCValueRef p_value);
    void SetTheReturnError(MCValueRef p_value);

    // SN-2015-06-03: [[ Bug 11277 ]] Refactor MCExecPoint update
    void deletestatements(MCStatement* p_statements);
    void eval_ctxt(MCExecContext &ctxt, MCStringRef p_expression, MCExecValue &r_value);
    void eval(MCExecContext &ctxt, MCStringRef p_expression, MCValueRef &r_value);
    void doscript(MCExecContext &ctxt, MCStringRef p_script, uinteger_t p_line, uinteger_t p_pos);
    
	//////////
	
	bool EvalExprAsValueRef(MCExpression *expr, Exec_errors error, MCValueRef& r_value);
	bool EvalOptionalExprAsValueRef(MCExpression *expr, MCValueRef default_value, Exec_errors error, MCValueRef& r_value);
	
    bool EvalExprAsBooleanRef(MCExpression *expr, Exec_errors error, MCBooleanRef& r_value);
	bool EvalOptionalExprAsBooleanRef(MCExpression *expr, MCBooleanRef default_value, Exec_errors error, MCBooleanRef& r_value);
    
    bool EvalExprAsStringRef(MCExpression *expr, Exec_errors error, MCStringRef& r_value);
    bool EvalOptionalExprAsStringRef(MCExpression *expr, MCStringRef default_value, Exec_errors error, MCStringRef& r_value);
    bool EvalOptionalExprAsNullableStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef& r_value);
    bool EvalExprAsMutableStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef& r_mutable_string);
    
    bool EvalExprAsNameRef(MCExpression *expr, Exec_errors error, MCNameRef& r_value);
	bool EvalOptionalExprAsNameRef(MCExpression *expr, MCNameRef default_value, Exec_errors error, MCNameRef& r_value);
    bool EvalOptionalExprAsNullableNameRef(MCExpression *p_expr, Exec_errors p_error, MCNameRef& r_value);
    
    bool EvalExprAsDataRef(MCExpression *expr, Exec_errors error, MCDataRef& r_value);
	bool EvalOptionalExprAsDataRef(MCExpression *expr, MCDataRef default_value, Exec_errors error, MCDataRef& r_value);
    bool EvalOptionalExprAsNullableDataRef(MCExpression *p_expr, Exec_errors p_error, MCDataRef& r_value);
    
    bool EvalExprAsArrayRef(MCExpression *expr, Exec_errors error, MCArrayRef& r_value);
	bool EvalOptionalExprAsArrayRef(MCExpression *expr, MCArrayRef default_value, Exec_errors error, MCArrayRef& r_value);
    bool EvalOptionalExprAsNullableArrayRef(MCExpression *p_expr, Exec_errors p_error, MCArrayRef& r_value);
    
    bool EvalExprAsNumberRef(MCExpression *expr, Exec_errors error, MCNumberRef& r_value);
	bool EvalOptionalExprAsNumberRef(MCExpression *expr, MCNumberRef default_value, Exec_errors error, MCNumberRef& r_value);
    
	bool EvalExprAsUInt(MCExpression *expr, Exec_errors error, uinteger_t& r_uint);
	bool EvalOptionalExprAsUInt(MCExpression *expr, uinteger_t default_value, Exec_errors error, uinteger_t& r_uint);
    
    bool EvalExprAsInt(MCExpression *expr, Exec_errors error, integer_t& r_int);
	bool EvalOptionalExprAsInt(MCExpression *expr, integer_t default_value, Exec_errors error, integer_t& r_int);
    
    bool EvalExprAsBool(MCExpression *expr, Exec_errors error, bool& r_bool);
	bool EvalOptionalExprAsBool(MCExpression *expr, bool default_value, Exec_errors error, bool& r_bool);
	
    bool EvalExprAsNonStrictBool(MCExpression *expr, Exec_errors error, bool& r_bool);
    
    bool EvalExprAsDouble(MCExpression *expr, Exec_errors error, double& r_double);
	bool EvalOptionalExprAsDouble(MCExpression *expr, double default_value, Exec_errors error, double& r_double);
    
    bool EvalExprAsChar(MCExpression *expr, Exec_errors error, char_t& r_char);
	bool EvalOptionalExprAsChar(MCExpression *expr, char_t default_value, Exec_errors error, char_t& r_char);
    
    bool EvalExprAsPoint(MCExpression *expr, Exec_errors error, MCPoint& r_point);
    bool EvalOptionalExprAsPoint(MCExpression *expr, MCPoint *default_value, Exec_errors error, MCPoint *&r_point);
    
    bool EvalExprAsColor(MCExpression *expr, Exec_errors error, MCColor& r_color);
    bool EvalOptionalExprAsColor(MCExpression *expr, MCColor* default_value, Exec_errors error, MCColor*& r_color);
	
	bool EvalExprAsRectangle(MCExpression *expr, Exec_errors error, MCRectangle& r_rectangle);
    bool EvalOptionalExprAsRectangle(MCExpression *expr, MCRectangle *default_value, Exec_errors error, MCRectangle *&r_rectangle);
    
	void TryToEvalExprAsArrayRef(MCExpression *p_expr, Exec_errors p_error, MCArrayRef& r_value);
    void TryToEvalOptionalExprAsColor(MCExpression *p_expr, MCColor *p_default, Exec_errors p_error, MCColor *&r_value);
    
    bool EvalExprAsStrictUInt(MCExpression *p_expr, Exec_errors p_error, uinteger_t& r_value);
    
    bool EvalExprAsStrictInt(MCExpression *p_expr, Exec_errors p_error, integer_t& r_value);
    
private:
	Exec_stat m_stat;

    MCObjectPtr m_object;

    // MW-2009-01-30: [[ Inherited parentScripts ]]
    // We store a reference to the parentScript use which is the current context
    // so we can retrieve the correct script locals. If this is NULL, then we
    // are not in parentScript context.
    MCParentScriptUse *m_parentscript;

    MCHandlerlist *m_hlist;
    MCHandler *m_curhandler;
    
    MCStringRef m_itemdel;
    MCStringRef m_columndel;
    MCStringRef m_linedel;
    MCStringRef m_rowdel;
    
    uint2 m_nffw;
    uint2 m_nftrailing;
    uint2 m_nfforce;
    uint2 m_cutoff;
    uint2 m_line;
    uint2 m_pos;
    
    MCStringOptions m_string_options;
    
    bool m_convertoctals : 1;
    bool m_wholematches : 1;
    bool m_usesystemdate : 1;
    bool m_useunicode : 1;
};

////////////////////////////////////////////////////////////////////////////////

void MCKeywordsExecSwitch(MCExecContext& ctxt, MCExpression *condition, MCExpression **cases, uindex_t case_count, int2 default_case, uint2 *case_offsets, MCStatement *statements, uint2 line, uint2 pos);
void MCKeywordsExecIf(MCExecContext& ctxt, MCExpression *condition, MCStatement *thenstatements, MCStatement *elsestatements, uint2 line, uint2 pos);
void MCKeywordsExecRepeatCount(MCExecContext& ctxt, MCStatement *statements, MCExpression *endcond, uint2 line, uint2 pos);
void MCKeywordsExecRepeatFor(MCExecContext& ctxt, MCStatement *statements, MCExpression *endcond, MCVarref *loopvar, File_unit each, uint2 line, uint2 pos);
void MCKeywordsExecRepeatWith(MCExecContext& ctxt, MCStatement *statements, MCExpression *step, MCExpression *startcond, MCExpression *endcond, MCVarref *loopvar, real8 stepval, uint2 line, uint2 pos);
void MCKeywordsExecRepeatForever(MCExecContext& ctxt, MCStatement *statements, uint2 line, uint2 pos);
void MCKeywordsExecRepeatUntil(MCExecContext& ctxt, MCStatement *statements, MCExpression *endcond, uint2 line, uint2 pos);
void MCKeywordsExecRepeatWhile(MCExecContext& ctxt, MCStatement *statements, MCExpression *endcond, uint2 line, uint2 pos);
void MCKeywordsExecTry(MCExecContext& ctxt, MCStatement *trystatements, MCStatement *catchstatements, MCStatement *finallystatements, MCVarref *errorvar, uint2 line, uint2 pos);
void MCKeywordsExecExit(MCExecContext& ctxt, Exec_stat stat);
void MCKeywordsExecBreak(MCExecContext& ctxt);
void MCKeywordsExecNext(MCExecContext& ctxt);
void MCKeywordsExecPass(MCExecContext& ctxt);
void MCKeywordsExecPassAll(MCExecContext& ctxt);
void MCKeywordsExecThrow(MCExecContext& ctxt, MCStringRef string);
void MCKeywordsExecResolveCommandOrFunction(MCExecContext& ctxt, MCNameRef p_name, bool is_function, MCHandler*& r_handler);
bool MCKeywordsExecSetupCommandOrFunction(MCExecContext& ctxt, MCParameter *params, MCContainer *containers, uint2 line, uint2 pos, bool is_function);
void MCKeywordsExecTeardownCommandOrFunction(MCParameter *params);
void MCKeywordsExecCommandOrFunction(MCExecContext& ctxt, MCHandler *handler, MCParameter *params, MCNameRef name, uint2 line, uint2 pos, bool platform_message, bool is_function);

////////////////////////////////////////////////////////////////////////////////

void MCLogicEvalIsEqualTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result);
void MCLogicEvalIsNotEqualTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result);
void MCLogicEvalIsGreaterThan(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result);
void MCLogicEvalIsGreaterThanOrEqualTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result);
void MCLogicEvalIsLessThan(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result);
void MCLogicEvalIsLessThanOrEqualTo(MCExecContext& ctxt, MCValueRef p_left, MCValueRef p_right, bool& r_result);

void MCLogicEvalAnd(MCExecContext& ctxt, bool p_a, bool p_b, bool& r_result);
void MCLogicEvalOr(MCExecContext& ctxt, bool p_a, bool p_b, bool& r_result);
void MCLogicEvalNot(MCExecContext& ctxt, bool p_bool, bool& r_result);

void MCLogicEvalIsABoolean(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCLogicEvalIsNotABoolean(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);

///////////

void MCArraysEvalKeys(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef& r_string);
void MCArraysEvalExtents(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef& r_string);
void MCArraysExecCombine(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef p_element_delimiter, MCStringRef p_key_delimiter, MCStringRef& r_string);
// SN-2014-09-01: [[ Bug 13297 ]] Combining by column deserves its own function as it is too
// different from combining by row
void MCArraysExecCombineByRow(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef &r_string);
void MCArraysExecCombineByColumn(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef& r_string);
void MCArraysExecCombineAsSet(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef p_element_delimiter, MCStringRef& r_string);
void MCArraysExecSplit(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_element_delimiter, MCStringRef p_key_delimiter, MCArrayRef& r_array);
void MCArraysExecSplitByColumn(MCExecContext& ctxt, MCStringRef p_string, MCArrayRef& r_array);
void MCArraysExecSplitAsSet(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_element_delimiter, MCArrayRef& r_array);
void MCArraysExecUnion(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result);
void MCArraysExecUnionRecursively(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result);
void MCArraysExecIntersect(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result);
void MCArraysExecIntersectRecursively(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result);
void MCArraysExecDifference(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result);
void MCArraysExecSymmetricDifference(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result);
void MCArraysEvalArrayEncode(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef version, MCDataRef& r_encoding);
void MCArraysEvalArrayDecode(MCExecContext& ctxt, MCDataRef p_encoding, MCArrayRef& r_array);
void MCArraysEvalMatrixMultiply(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result);
void MCArraysEvalTransposeMatrix(MCExecContext& ctxt, MCArrayRef p_matrix, MCArrayRef& r_result);
void MCArraysEvalVectorDotProduct(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, double& r_result);

void MCArraysEvalIsAnArray(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCArraysEvalIsNotAnArray(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCArraysEvalIsAmongTheKeysOf(MCExecContext& ctxt, MCNameRef p_key, MCArrayRef p_array, bool& r_result);
void MCArraysEvalIsNotAmongTheKeysOf(MCExecContext& ctxt, MCNameRef p_key, MCArrayRef p_array, bool& r_result);

void MCArraysExecFilterWildcard(MCExecContext& ctxt, MCArrayRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines, MCArrayRef &r_result);
void MCArraysExecFilterRegex(MCExecContext& ctxt, MCArrayRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines, MCArrayRef &r_result);
void MCArraysExecFilterExpression(MCExecContext& ctxt, MCArrayRef p_source, MCExpression* p_expression, bool p_without, bool p_lines, MCArrayRef &r_result);

///////////

void MCMathEvalBaseConvert(MCExecContext& ctxt, MCStringRef p_source, integer_t p_source_base, integer_t p_dest_base, MCStringRef& r_result);

void MCMathEvalAbs(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalRoundToPrecision(MCExecContext& ctxt, real64_t p_number, real64_t p_precision, real64_t& r_result);
void MCMathEvalRound(MCExecContext& ctxt, real64_t p_number, real64_t& r_result);
void MCMathEvalStatRoundToPrecision(MCExecContext& ctxt, real64_t p_number, real64_t p_precision, real64_t& r_result);
void MCMathEvalStatRound(MCExecContext& ctxt, real64_t p_number, real64_t& r_result);
void MCMathEvalTrunc(MCExecContext& ctxt, real64_t p_number, real64_t& r_result);
void MCMathEvalFloor(MCExecContext& ctxt, real64_t p_number, real64_t& r_result);
void MCMathEvalCeil(MCExecContext& ctxt, real64_t p_number, real64_t& r_result);

void MCMathEvalAcos(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalAsin(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalAtan(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalAtan2(MCExecContext& ctxt, real64_t p_y, real64_t p_x, real64_t& r_result);
void MCMathEvalCos(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalSin(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalTan(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);

void MCMathEvalExp(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalExp1(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalExp2(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalExp10(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalLn(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalLn1(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalLog2(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalLog10(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);
void MCMathEvalSqrt(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);

void MCMathEvalAnnuity(MCExecContext& ctxt, real64_t p_rate, real64_t p_periods, real64_t& r_result);
void MCMathEvalCompound(MCExecContext& ctxt, real64_t p_rate, real64_t p_periods, real64_t& r_result);

void MCMathEvalArithmeticMean(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalMedian(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalMin(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalMax(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalSampleStdDev(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalSum(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);

void MCMathEvalAverageDeviation(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalGeometricMean(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalHarmonicMean(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalPopulationStdDev(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalPopulationVariance(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);
void MCMathEvalSampleVariance(MCExecContext& ctxt, real64_t *p_values, uindex_t p_count, real64_t& r_result);

void MCMathEvalRandom(MCExecContext& ctxt, real64_t p_in, real64_t& r_result);

void MCMathEvalBitwiseAnd(MCExecContext& ctxt, uinteger_t p_left, uinteger_t p_right, uinteger_t& r_result);
void MCMathEvalBitwiseNot(MCExecContext& ctxt, uinteger_t p_right, uinteger_t& r_result);
void MCMathEvalBitwiseOr(MCExecContext& ctxt, uinteger_t p_left, uinteger_t p_right, uinteger_t& r_result);
void MCMathEvalBitwiseXor(MCExecContext& ctxt, uinteger_t p_left, uinteger_t p_right, uinteger_t& r_result);

void MCMathEvalDiv(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result);
void MCMathEvalDivArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result);
void MCMathEvalDivArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result);
void MCMathEvalSubtract(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result);
void MCMathEvalSubtractNumberFromArray(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result);
void MCMathEvalSubtractArrayFromArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result);
void MCMathEvalMod(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result);
void MCMathEvalModArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result);
void MCMathEvalModArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result);
void MCMathEvalWrap(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result);
void MCMathEvalWrapArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result);
void MCMathEvalWrapArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result);
void MCMathEvalOver(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result);
void MCMathEvalOverArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result);
void MCMathEvalOverArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result);
void MCMathEvalAdd(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result);
void MCMathEvalAddNumberToArray(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result);
void MCMathEvalAddArrayToArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result);
void MCMathEvalMultiply(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result);
void MCMathEvalMultiplyArrayByNumber(MCExecContext& ctxt, MCArrayRef p_array, real64_t p_number, MCArrayRef& r_result);
void MCMathEvalMultiplyArrayByArray(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result);

void MCMathEvalPower(MCExecContext& ctxt, real64_t p_left, real64_t p_right, real64_t& r_result);

void MCMathEvalIsAnInteger(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCMathEvalIsNotAnInteger(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCMathEvalIsANumber(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCMathEvalIsNotANumber(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);

void MCMathGetRandomSeed(MCExecContext& ctxt, integer_t& r_value);
void MCMathSetRandomSeed(MCExecContext& ctxt, integer_t p_value);
                            
#define MCMathExecAddNumberToNumber(ctxt, a, b, r) MCMathEvalAdd(ctxt, a, b, r)
#define MCMathExecAddNumberToArray(ctxt, a, b, r) MCMathEvalAddNumberToArray(ctxt, b, a, r)
#define MCMathExecAddArrayToArray(ctxt, a, b, r) MCMathEvalAddArrayToArray(ctxt, a, b, r)
#define MCMathExecSubtractNumberFromNumber(ctxt, a, b, r) MCMathEvalSubtract(ctxt, b, a, r)
#define MCMathExecSubtractNumberFromArray(ctxt, a, b, r) MCMathEvalSubtractNumberFromArray(ctxt, b, a, r)
#define MCMathExecSubtractArrayFromArray(ctxt, a, b, r) MCMathEvalSubtractArrayFromArray(ctxt, b, a, r)
#define MCMathExecDivideNumberByNumber(ctxt, a, b, r) MCMathEvalOver(ctxt, a, b, r)
#define MCMathExecDivideArrayByNumber(ctxt, a, b, r) MCMathEvalOverArrayByNumber(ctxt, a, b, r)
#define MCMathExecDivideArrayByArray(ctxt, a, b, r) MCMathEvalOverArrayByArray(ctxt, a, b, r)
#define MCMathExecMultiplyNumberByNumber(ctxt, a, b, r) MCMathEvalMultiply(ctxt, a, b, r)
#define MCMathExecMultiplyArrayByNumber(ctxt, a, b, r) MCMathEvalMultiplyArrayByNumber(ctxt, a, b, r)
#define MCMathExecMultiplyArrayByArray(ctxt, a, b, r) MCMathEvalMultiplyArrayByArray(ctxt, a, b, r)

///////////

void MCFiltersEvalBase64Encode(MCExecContext& ctxt, MCDataRef p_source, MCStringRef& r_result);
void MCFiltersEvalBase64Decode(MCExecContext& ctxt, MCStringRef p_source, MCDataRef& r_result);
void MCFiltersEvalBinaryEncode(MCExecContext& ctxt, MCStringRef p_format, MCValueRef *p_params, uindex_t p_param_count, MCDataRef& r_string);
void MCFiltersEvalBinaryDecode(MCExecContext& ctxt, MCStringRef p_format, MCDataRef p_data, MCValueRef *r_results, uindex_t p_result_count, integer_t& r_done);
void MCFiltersEvalCompress(MCExecContext& ctxt, MCDataRef p_source, MCDataRef& r_result);
void MCFiltersEvalDecompress(MCExecContext& ctxt, MCDataRef p_source, MCDataRef& r_result);
void MCFiltersEvalIsoToMac(MCExecContext& ctxt, MCDataRef p_source, MCDataRef& r_result);
void MCFiltersEvalMacToIso(MCExecContext& ctxt, MCDataRef p_source, MCDataRef& r_result);
void MCFiltersEvalUrlEncode(MCExecContext& ctxt, MCStringRef p_source, MCStringRef& r_result);
void MCFiltersEvalUrlDecode(MCExecContext& ctxt, MCStringRef p_source, MCStringRef& r_result);
void MCFiltersEvalUniEncodeFromNative(MCExecContext& ctxt, MCStringRef p_input, MCDataRef &r_output);
void MCFiltersEvalUniDecodeToNative(MCExecContext& ctxt, MCDataRef p_input, MCStringRef &r_output);
void MCFiltersEvalUniEncodeFromEncoding(MCExecContext& ctxt, MCDataRef p_src, MCNameRef p_lang, MCDataRef& r_dest);
void MCFiltersEvalUniDecodeToEncoding(MCExecContext& ctxt, MCDataRef p_src, MCNameRef p_lang, MCDataRef& r_dest);
void MCFiltersEvalMessageDigest(MCExecContext& ctxt, MCDataRef p_data, MCNameRef p_digest_name, MCDataRef &r_digest);
void MCFiltersEvalMD5Digest(MCExecContext& ctxt, MCDataRef p_src, MCDataRef& r_digest);
void MCFiltersEvalSHA1Digest(MCExecContext& ctxt, MCDataRef p_src, MCDataRef& r_digest);

///////////

void MCStringsEvalToLower(MCExecContext& ctxt, MCStringRef p_string, MCStringRef& r_lower);
void MCStringsEvalToUpper(MCExecContext& ctxt, MCStringRef p_string, MCStringRef& r_lower);

void MCStringsEvalNumToChar(MCExecContext& ctxt, uinteger_t codepoint, MCValueRef& r_character);
void MCStringsEvalNumToNativeChar(MCExecContext& ctxt, uinteger_t codepoint, MCStringRef& r_character);
void MCStringsEvalNumToUnicodeChar(MCExecContext& ctxt, uinteger_t codepoint, MCStringRef& r_character);
void MCStringsEvalCharToNum(MCExecContext& ctxt, MCValueRef character, MCValueRef& r_codepoint);
void MCStringsEvalNativeCharToNum(MCExecContext& ctxt, MCStringRef character, uinteger_t& r_codepoint);
void MCStringsEvalUnicodeCharToNum(MCExecContext& ctxt, MCStringRef character, uinteger_t& r_codepoint);
void MCStringsEvalNumToByte(MCExecContext& ctxt, integer_t codepoint, MCDataRef& r_byte);
void MCStringsEvalByteToNum(MCExecContext& ctxt, MCStringRef byte, integer_t& r_codepoint);

bool MCStringsEvalTextEncoding(MCStringRef encoding, MCStringEncoding& r_encoding);

void MCStringsEvalTextDecode(MCExecContext& ctxt, MCStringRef p_encoding, MCDataRef p_encoded_text, MCStringRef& r_decoded_text);
void MCStringsEvalTextEncode(MCExecContext& ctxt, MCStringRef p_encoding, MCStringRef p_decoded_text, MCDataRef& r_encoded_text);

void MCStringsEvalNormalizeText(MCExecContext& ctxt, MCStringRef p_text, MCStringRef p_form, MCStringRef& r_string);

void MCStringsEvalCodepointProperty(MCExecContext& ctxt, MCStringRef p_text, MCStringRef p_form, MCValueRef& r_value);

void MCStringsEvalLength(MCExecContext& ctxt, MCStringRef p_string, integer_t& r_length);

void MCStringsEvalMatchText(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_pattern, MCStringRef* r_results, uindex_t p_result_count, bool& r_match);
void MCStringsEvalMatchChunk(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_pattern, MCStringRef* r_results, uindex_t p_result_count, bool& r_match);
void MCStringsEvalReplaceText(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_pattern, MCStringRef p_replacement, MCStringRef& r_result);

void MCStringsEvalFormat(MCExecContext& ctxt, MCStringRef p_format, MCValueRef* p_params, uindex_t p_param_count, MCStringRef& r_result);
void MCStringsEvalMerge(MCExecContext& ctxt, MCStringRef p_format, MCStringRef& r_string);

void MCStringsEvalConcatenate(MCExecContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result);
void MCStringsEvalConcatenate(MCExecContext& ctxt, MCDataRef p_left, MCDataRef p_right, MCDataRef& r_result);
void MCStringsEvalConcatenateWithSpace(MCExecContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result);
void MCStringsEvalConcatenateWithComma(MCExecContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result);

void MCStringsEvalContains(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result);
void MCStringsEvalDoesNotContain(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result);
void MCStringsEvalBeginsWith(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result);
void MCStringsEvalEndsWith(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result);

void MCStringsEvalIsAmongTheLinesOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheLinesOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheParagraphsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheParagraphsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheSentencesOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheSentencesOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheItemsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheItemsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheWordsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheWordsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheTrueWordsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheTrueWordsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheTokensOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheTokensOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheCharsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheCharsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheCodepointsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheCodepointsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheCodeunitsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheCodeunitsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result);
void MCStringsEvalIsAmongTheBytesOf(MCExecContext& ctxt, MCDataRef p_chunk, MCDataRef p_string, bool& r_result);
void MCStringsEvalIsNotAmongTheBytesOf(MCExecContext& ctxt, MCDataRef p_chunk, MCDataRef p_string, bool& r_result);

void MCStringsEvalLineOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalParagraphOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalSentenceOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalItemOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalWordOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalTrueWordOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalTokenOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalCodepointOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalCodeunitOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalByteOffset(MCExecContext& ctxt, MCDataRef p_chunk, MCDataRef p_string, uindex_t p_start_offset, uindex_t& r_result);
void MCStringsEvalOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result);

void MCStringsExecReplace(MCExecContext& ctxt, MCStringRef p_pattern, MCStringRef p_replacement, MCStringRef p_target);

void MCStringsExecFilterWildcard(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines, MCStringRef &r_result);
void MCStringsExecFilterRegex(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines, MCStringRef &r_result);
void MCStringsExecFilterExpression(MCExecContext& ctxt, MCStringRef p_source, MCExpression* p_expression, bool p_without, bool p_lines, MCStringRef &r_result);

void MCStringsEvalIsAscii(MCExecContext& ctxt, MCValueRef p_string, bool& r_result);
void MCStringsEvalIsNotAscii(MCExecContext& ctxt, MCValueRef p_string, bool& r_result);

void MCStringsExecSort(MCExecContext& ctxt, Sort_type p_dir, Sort_type p_form, MCStringRef *p_strings_array, uindex_t p_count, MCExpression *p_by, MCStringRef*& r_sorted_array, uindex_t& r_sorted_count);

void MCStringsEvalBidiDirection(MCExecContext& ctxt, MCStringRef p_string, MCStringRef& r_result);

void MCStringsEvalTextChunkByRange(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, bool p_eval_mutable, MCStringRef& x_string);
void MCStringsEvalTextChunkByExpression(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, integer_t p_first, bool p_eval_mutable, MCStringRef &x_string);
void MCStringsEvalTextChunkByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, bool p_eval_mutable, MCStringRef& x_string);

void MCStringsSetTextChunkByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, MCStringRef& x_target);
void MCStringsSetTextChunkByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, integer_t p_first, MCStringRef& x_target);
void MCStringsSetTextChunkByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, MCStringRef& x_target);

void MCStringsEvalMutableLinesOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, MCStringRef& r_result);
void MCStringsEvalMutableLinesOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_line, MCStringRef& r_result);
void MCStringsEvalMutableLinesOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, MCStringRef& r_result);
void MCStringsEvalMutableItemsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, MCStringRef& r_result);
void MCStringsEvalMutableItemsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_item, MCStringRef& r_result);
void MCStringsEvalMutableItemsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, MCStringRef& r_result);
void MCStringsEvalMutableWordsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, MCStringRef& r_result);
void MCStringsEvalMutableWordsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_word, MCStringRef& r_result);
void MCStringsEvalMutableWordsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, MCStringRef& r_result);
void MCStringsEvalMutableTokensOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, MCStringRef& r_result);
void MCStringsEvalMutableTokensOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_token, MCStringRef& r_result);
void MCStringsEvalMutableTokensOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, MCStringRef& r_result);

void MCStringsExecSetLinesOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result);
void MCStringsExecSetLinesOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_line, MCStringRef& r_result);
void MCStringsExecSetLinesOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result);
void MCStringsExecSetItemsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result);
void MCStringsExecSetItemsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_item, MCStringRef& r_result);
void MCStringsExecSetItemsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result);
void MCStringsExecSetWordsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result);
void MCStringsExecSetWordsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_word, MCStringRef& r_result);
void MCStringsExecSetWordsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result);
void MCStringsExecSetTokensOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result);
void MCStringsExecSetTokensOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_token, MCStringRef& r_result);
void MCStringsExecSetTokensOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result);
void MCStringsExecSetCharsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_first, integer_t p_last, MCStringRef& r_result);
void MCStringsExecSetCharsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, integer_t p_char, MCStringRef& r_result);
void MCStringsExecSetCharsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Preposition_type p_type, Chunk_term p_ordinal_type, MCStringRef& r_result);

void MCStringsCountChunks(MCExecContext& ctxt, Chunk_term p_chunk_type, MCStringRef p_string, uinteger_t& r_count);
///////////

void MCStringsGetTextChunk(MCExecContext& ctxt, MCStringRef p_source, integer_t p_start, integer_t p_end, MCStringRef& r_result);

void MCStringsEvalLinesOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalLinesOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_line, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalLinesOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalItemsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalItemsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_item, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalItemsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalWordsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalWordsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_word, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalWordsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalTokensOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalTokensOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_token, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalTokensOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalCharsOfTextByRange(MCExecContext& ctxt, MCStringRef p_source, integer_t p_first, integer_t p_last, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalCharsOfTextByExpression(MCExecContext& ctxt, MCStringRef p_source, integer_t p_char, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);
void MCStringsEvalCharsOfTextByOrdinal(MCExecContext& ctxt, MCStringRef p_source, Chunk_term p_ordinal_type, integer_t& x_start, integer_t& x_end, MCStringRef& r_result);

void MCStringsEvalTextChunk(MCExecContext& ctxt, MCMarkedText p_source, MCStringRef& r_string);
void MCStringsEvalByteChunk(MCExecContext& ctxt, MCMarkedText p_source, MCDataRef& r_bytes);

void MCStringsMarkLinesOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkLinesOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkParagraphsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkParagraphsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkSentencesOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkSentencesOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkItemsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkItemsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkTrueWordsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkTrueWordsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkWordsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkWordsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkTokensOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkTokensOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkCharsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkCharsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkCodepointsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkCodepointsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkCodeunitsOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkCodeunitsOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);

void MCStringsMarkBytesOfTextByRange(MCExecContext& ctxt, integer_t p_first, integer_t p_last, MCMarkedText& x_mark);
void MCStringsMarkBytesOfTextByOrdinal(MCExecContext& ctxt, Chunk_term p_ordinal_type, MCMarkedText& x_mark);

void MCStringsSkipWord(MCExecContext& ctxt, MCStringRef p_string, bool p_skip_spaces, uindex_t& x_offset);

class MCTextChunkIterator;

MCTextChunkIterator *MCStringsTextChunkIteratorCreate(MCExecContext& ctxt, MCStringRef p_text, Chunk_term p_chunk_type);
MCTextChunkIterator *MCStringsTextChunkIteratorCreateWithRange(MCExecContext& ctxt, MCStringRef p_text, MCRange p_range, Chunk_term p_chunk_type);
bool MCStringsTextChunkIteratorNext(MCExecContext& ctxt, MCTextChunkIterator *tci);

///////////

struct MCInterfaceBackdrop;
struct MCInterfaceNamedColor;
struct MCInterfaceImagePaletteSettings;
struct MCInterfaceVisualEffect;
struct MCInterfaceVisualEffectArgument;
struct MCInterfaceStackFileVersion;

extern MCExecCustomTypeInfo *kMCInterfaceNamedColorTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfacePaintCompressionTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceLookAndFeelTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceBackdropTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceProcessTypeTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceSelectionModeTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceWindowPositionTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceWindowAlignmentTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfacePaletteSettingsTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceVisualEffectTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceVisualEffectArgumentTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceButtonIconTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceTriStateTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceStackFileVersionTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceSystemAppearanceTypeInfo;

void MCInterfaceInitialize(MCExecContext& ctxt);
void MCInterfaceFinalize(MCExecContext& ctxt);

void MCInterfaceImagePaletteSettingsFree(MCExecContext& ctxt, MCInterfaceImagePaletteSettings& p_settings);
void MCInterfaceVisualEffectFree(MCExecContext& ctxt, MCInterfaceVisualEffect& p_effect);
void MCInterfaceVisualEffectArgumentFree(MCExecContext& ctxt, MCInterfaceVisualEffectArgument& p_arg);

bool MCInterfaceTryToResolveObject(MCExecContext& ctxt, MCStringRef long_id, MCObjectPtr& r_object);

void MCInterfaceMakeCustomImagePaletteSettings(MCExecContext& ctxt, MCColor *colors, uindex_t color_count, MCInterfaceImagePaletteSettings& r_settings);
void MCInterfaceMakeOptimalImagePaletteSettings(MCExecContext& ctxt, integer_t *count, MCInterfaceImagePaletteSettings& r_settings);
void MCInterfaceMakeWebSafeImagePaletteSettings(MCExecContext& ctxt, MCInterfaceImagePaletteSettings& r_settings);

void MCInterfaceMakeVisualEffect(MCExecContext& ctxt, MCStringRef name, MCStringRef sound, MCInterfaceVisualEffectArgument *effect_args, uindex_t count, Visual_effects type, Visual_effects direction, Visual_effects speed, Visual_effects image, MCInterfaceVisualEffect& r_effect);
void MCInterfaceMakeVisualEffectArgument(MCExecContext& ctxt, MCStringRef p_value, MCStringRef p_key, bool p_has_id, MCInterfaceVisualEffectArgument& r_arg);

void MCInterfaceEvalScreenColors(MCExecContext& ctxt, real64_t& r_colors);
void MCInterfaceEvalScreenDepth(MCExecContext& ctxt, integer_t& r_depth);
void MCInterfaceEvalScreenName(MCExecContext& ctxt, MCNameRef& r_name);
void MCInterfaceEvalScreenRect(MCExecContext& ctxt, bool p_working, bool p_plural, bool p_effective, MCStringRef& r_string);

void MCInterfaceEvalScreenLoc(MCExecContext& ctxt, MCStringRef& r_string);

void MCInterfaceEvalClickH(MCExecContext& ctxt, integer_t& r_value);
void MCInterfaceEvalClickV(MCExecContext& ctxt, integer_t& r_value);
void MCInterfaceEvalClickLoc(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalClickChar(MCExecContext& ctxt, MCStringRef& r_result);
void MCInterfaceEvalClickText(MCExecContext& ctxt, MCStringRef& r_result);
void MCInterfaceEvalClickCharChunk(MCExecContext& ctxt, MCStringRef& r_result);
void MCInterfaceEvalClickChunk(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalClickLine(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalClickField(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalClickStack(MCExecContext& ctxt, MCStringRef& r_string);

void MCInterfaceEvalMouse(MCExecContext& ctxt, integer_t p_which, MCNameRef& r_result);
void MCInterfaceEvalMouseClick(MCExecContext& ctxt, bool& r_bool);
void MCInterfaceEvalMouseColor(MCExecContext& ctxt, MCColor& r_color);

void MCInterfaceEvalMouseH(MCExecContext& ctxt, integer_t& r_value);
void MCInterfaceEvalMouseV(MCExecContext& ctxt, integer_t& r_value);
void MCInterfaceEvalMouseLoc(MCExecContext& ctxt, MCStringRef& r_loc);
void MCInterfaceEvalMouseChar(MCExecContext& ctxt, MCStringRef& r_result);
void MCInterfaceEvalMouseText(MCExecContext& ctxt, MCStringRef& r_result);
void MCInterfaceEvalMouseCharChunk(MCExecContext& ctxt, MCStringRef& r_result);
void MCInterfaceEvalMouseChunk(MCExecContext& ctxt, MCStringRef& r_result);
void MCInterfaceEvalMouseLine(MCExecContext& ctxt, MCStringRef& r_result);
void MCInterfaceEvalMouseControl(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalMouseStack(MCExecContext& ctxt, MCStringRef& r_string);

void MCInterfaceEvalFoundText(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalFoundField(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalFoundChunk(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalFoundLine(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalFoundLoc(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalFoundField(MCExecContext& ctxt, MCStringRef& r_string);

void MCInterfaceEvalSelectedText(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalSelectedTextOf(MCExecContext& ctxt, MCObjectPtr p_target, MCStringRef& r_string);
void MCInterfaceEvalSelectedChunk(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalSelectedChunkOf(MCExecContext& ctxt, MCObjectPtr p_target, MCStringRef& r_string);
void MCInterfaceEvalSelectedLine(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalSelectedLineOf(MCExecContext& ctxt, MCObjectPtr p_target, MCStringRef& r_string);
void MCInterfaceEvalSelectedLoc(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalSelectedLocOf(MCExecContext& ctxt, MCObjectPtr p_target, MCStringRef& r_string);

void MCInterfaceEvalSelectedField(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalSelectedImage(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalSelectedObject(MCExecContext& ctxt, MCStringRef& r_string);

void MCInterfaceEvalCapsLockKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalCommandKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalControlKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalOptionKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalShiftKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalEventCapsLockKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalEventCommandKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalEventControlKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalEventOptionKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalEventShiftKey(MCExecContext& ctxt, MCNameRef& r_result);
void MCInterfaceEvalKeysDown(MCExecContext& ctxt, MCStringRef& r_string);

void MCInterfaceEvalMainStacks(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalOpenStacks(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalStacks(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalTopStack(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalTopStackOf(MCExecContext& ctxt, integer_t p_stack_number, MCStringRef& r_string);

void MCInterfaceEvalFocusedObject(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalColorNames(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalFlushEvents(MCExecContext& ctxt, MCNameRef p_name, MCStringRef& r_string);
void MCInterfaceEvalGlobalLoc(MCExecContext& ctxt, MCPoint point, MCPoint& r_global_point);
void MCInterfaceEvalLocalLoc(MCExecContext& ctxt, MCPoint point, MCPoint& r_local_point);
void MCInterfaceEvalMovingControls(MCExecContext& ctxt, MCStringRef& r_string);
void MCInterfaceEvalWaitDepth(MCExecContext& ctxt, integer_t& r_depth);

void MCInterfaceEvalIntersect(MCExecContext& ctxt, MCObjectPtr p_object_a, MCObjectPtr p_object_b, bool& r_intersect);
void MCInterfaceEvalIntersectWithThreshold(MCExecContext& ctxt, MCObjectPtr p_object_a, MCObjectPtr p_object_b, MCStringRef p_threshold, bool& r_intersect);
void MCInterfaceEvalWithin(MCExecContext& ctxt, MCObjectPtr p_object, MCPoint p_point, bool& r_within);

void MCInterfaceEvalThereIsAnObject(MCExecContext& ctxt, MCChunk *p_object, bool& r_exists);
void MCInterfaceEvalThereIsNotAnObject(MCExecContext& ctxt, MCChunk *p_object, bool& r_not_exists);

void MCInterfaceEvalControlAtLoc(MCExecContext& ctxt, MCPoint p_location, MCStringRef& r_control);
void MCInterfaceEvalControlAtScreenLoc(MCExecContext& ctxt, MCPoint p_location, MCStringRef& r_control);

void MCInterfaceExecBeep(MCExecContext& ctxt, integer_t p_count);
void MCInterfaceExecClickCmd(MCExecContext& ctxt, uint2 p_button, MCPoint p_location, uint2 p_modifiers);

void MCInterfaceExecCloseStack(MCExecContext& ctxt, MCStack *p_target);
void MCInterfaceExecCloseDefaultStack(MCExecContext& ctxt);

void MCInterfaceExecDrag(MCExecContext& ctxt, uint2 p_which, MCPoint p_start, MCPoint p_end, uint2 p_modifiers);

void MCInterfaceExecFocusOnNothing(MCExecContext &ctxt);
void MCInterfaceExecFocusOn(MCExecContext &ctxt, MCObject *p_object);

void MCInterfaceExecGrab(MCExecContext &ctxt, MCControl *control);

void MCInterfaceExecGroupControls(MCExecContext& ctxt, MCObjectPtr *p_controls, uindex_t p_control_count);
void MCInterfaceExecGroupSelection(MCExecContext& ctxt);

void MCInterfaceExecPopToLast(MCExecContext& ctxt);
void MCInterfaceExecPop(MCExecContext& ctxt, MCStringRef& r_element);
void MCInterfaceExecPushRecentCard(MCExecContext& ctxt);
void MCInterfaceExecPushCurrentCard(MCExecContext& ctxt);
void MCInterfaceExecPushCard(MCExecContext& ctxt, MCCard *p_target);

void MCInterfaceExecPlaceGroupOnCard(MCExecContext& ctxt, MCObject *p_group, MCCard *p_target);
void MCInterfaceExecRemoveGroupFromCard(MCExecContext& ctxt, MCObjectPtr p_group, MCCard *p_target);
void MCInterfaceExecResetCursors(MCExecContext& ctxt);
void MCInterfaceExecResetTemplate(MCExecContext& ctxt, Reset_type type);

void MCInterfaceExecRevert(MCExecContext& ctxt);
void MCInterfaceExecRevertStack(MCExecContext& ctxt, MCObject *p_stack);

void MCInterfaceExecSelectEmpty(MCExecContext& ctxt);
void MCInterfaceExecSelectAllTextOfField(MCExecContext& ctxt, MCObjectPtr target);
void MCInterfaceExecSelectAllTextOfButton(MCExecContext& ctxt, MCObjectPtr target);
void MCInterfaceExecSelectTextOfField(MCExecContext& ctxt, Preposition_type preposition, MCObjectChunkPtr target);
void MCInterfaceExecSelectTextOfButton(MCExecContext& ctxt, Preposition_type preposition, MCObjectChunkPtr target);
void MCInterfaceExecSelectObjects(MCExecContext& ctxt, MCObjectPtr *p_controls, uindex_t p_control_count);

void MCInterfaceExecStartEditingGroup(MCExecContext& ctxt, MCGroup *p_target);

void MCInterfaceExecStopEditingDefaultStack(MCExecContext& ctxt);
void MCInterfaceExecStopEditingGroup(MCExecContext& ctxt, MCGroup *p_target);
void MCInterfaceExecStopMovingObject(MCExecContext& ctxt, MCObject *p_target);

void MCInterfaceExecType(MCExecContext& ctxt, MCStringRef p_typing, uint2 p_modifiers);

void MCInterfaceExecUndo(MCExecContext& ctxt);

void MCInterfaceExecUngroupObject(MCExecContext& ctxt, MCObject *p_group);
void MCInterfaceExecUngroupSelection(MCExecContext& ctxt);

void MCInterfaceExecCopyObjectsToContainer(MCExecContext& ctxt, MCObjectPtr *p_targets, uindex_t p_target_count, MCObjectPtr p_destination);
void MCInterfaceExecCutObjectsToContainer(MCExecContext& ctxt, MCObjectPtr *p_targets, uindex_t p_target_count, MCObjectPtr p_destination);

void MCInterfaceExecDelete(MCExecContext& ctxt);
void MCInterfaceExecDeleteObjects(MCExecContext& ctxt, MCObjectPtr *objects, uindex_t object_count);
void MCInterfaceExecDeleteObjectChunks(MCExecContext& ctxt, MCObjectChunkPtr *chunks, uindex_t chunk_count);

void MCInterfaceExecDisableChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_targets);
void MCInterfaceExecEnableChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_targets);
void MCInterfaceExecUnhiliteChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_targets);
void MCInterfaceExecHiliteChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_targets);

void MCInterfaceExecDisableObject(MCExecContext& ctxt, MCObjectPtr p_targets);
void MCInterfaceExecEnableObject(MCExecContext& ctxt, MCObjectPtr p_targets);
void MCInterfaceExecUnhiliteObject(MCExecContext& ctxt, MCObjectPtr p_targets);
void MCInterfaceExecHiliteObject(MCExecContext& ctxt, MCObjectPtr p_targets);

void MCInterfaceExecSaveStack(MCExecContext& ctxt, MCStack *p_target);
void MCInterfaceExecSaveStackWithVersion(MCExecContext & ctxt, MCStack *p_target, MCStringRef p_version);
void MCInterfaceExecSaveStackWithNewestVersion(MCExecContext & ctxt, MCStack *p_target);
void MCInterfaceExecSaveStackAs(MCExecContext& ctxt, MCStack *p_target, MCStringRef p_new_filename);
void MCInterfaceExecSaveStackAsWithVersion(MCExecContext& ctxt, MCStack *p_target, MCStringRef p_new_filename, MCStringRef p_version);
void MCInterfaceExecSaveStackAsWithNewestVersion(MCExecContext& ctxt, MCStack *p_target, MCStringRef p_new_filename);

void MCInterfaceExecMoveObjectBetween(MCExecContext& ctxt, MCObject *p_target, MCPoint p_from, MCPoint p_to, double p_duration, int p_units, bool p_wait, bool p_dispatch);
void MCInterfaceExecMoveObjectAlong(MCExecContext& ctxt, MCObject *p_target, MCPoint *p_motion, uindex_t p_motion_count, bool p_is_relative, double p_duration, int p_units, bool p_wait, bool p_dispatch);

void MCInterfaceExecHideGroups(MCExecContext& ctxt);
void MCInterfaceExecHideObject(MCExecContext& ctxt, MCObjectPtr p_target);
void MCInterfaceExecHideObjectWithEffect(MCExecContext& ctxt, MCObjectPtr p_target, MCVisualEffect *p_effect);
void MCInterfaceExecHideMenuBar(MCExecContext& ctxt);
void MCInterfaceExecHideTaskBar(MCExecContext& ctxt);

void MCInterfaceExecShowGroups(MCExecContext& ctxt);
void MCInterfaceExecShowAllCards(MCExecContext& ctxt);
void MCInterfaceExecShowMarkedCards(MCExecContext& ctxt);
void MCInterfaceExecShowCards(MCExecContext& ctxt, uint2 p_count);
void MCInterfaceExecShowObject(MCExecContext& ctxt, MCObjectPtr p_target, MCPoint *p_at);
void MCInterfaceExecShowObjectWithEffect(MCExecContext& ctxt, MCObjectPtr p_target, MCPoint *p_at, MCVisualEffect *p_effect);
void MCInterfaceExecShowMenuBar(MCExecContext& ctxt);
void MCInterfaceExecShowTaskBar(MCExecContext& ctxt);

void MCInterfaceExecPopupWidget(MCExecContext &ctxt, MCNameRef p_kind, MCPoint *p_at, MCArrayRef p_properties);
void MCInterfaceExecPopupButton(MCExecContext& ctxt, MCButton *p_target, MCPoint *p_at);
void MCInterfaceExecDrawerStack(MCExecContext& ctxt, MCStack *p_target, MCNameRef parent, bool p_parent_is_thisstack, int p_at, int p_aligned);
void MCInterfaceExecDrawerStackByName(MCExecContext& ctxt, MCNameRef p_target, MCNameRef parent, bool p_parent_is_thisstack, int p_at, int p_aligned);
void MCInterfaceExecDrawerStackLegacy(MCExecContext& ctxt, MCStack *p_target, MCNameRef parent, bool p_parent_is_thisstack, intenum_t p_at, intenum_t p_aligned);
void MCInterfaceExecDrawerStackByNameLegacy(MCExecContext& ctxt, MCNameRef p_target, MCNameRef parent, bool p_parent_is_thisstack, intenum_t p_at, intenum_t p_aligned);
void MCInterfaceExecSheetStack(MCExecContext& ctxt, MCStack *p_target, MCNameRef parent, bool p_parent_is_thisstack);
void MCInterfaceExecSheetStackByName(MCExecContext& ctxt, MCNameRef p_target, MCNameRef parent, bool p_parent_is_thisstack);
void MCInterfaceExecOpenStack(MCExecContext& ctxt, MCStack *p_target, int p_mode);
void MCInterfaceExecOpenStackByName(MCExecContext& ctxt, MCNameRef p_target, int p_mode);
void MCInterfaceExecPopupStack(MCExecContext& ctxt, MCStack *p_target, MCPoint *p_at, int p_mode);
void MCInterfaceExecPopupStackByName(MCExecContext& ctxt, MCNameRef p_target, MCPoint *p_at, int p_mode);

void MCInterfaceExecCreateStack(MCExecContext& ctxt, MCStack *p_owner, MCStringRef p_new_name, bool p_force_invisible);
void MCInterfaceExecCreateScriptOnlyStack(MCExecContext& ctxt, MCStringRef p_new_name);
void MCInterfaceExecCreateStackWithGroup(MCExecContext& ctxt, MCGroup *p_group_to_copy, MCStringRef p_new_name, bool p_force_invisible);
void MCInterfaceExecCreateCard(MCExecContext& ctxt, MCStringRef p_new_name, MCStack *p_parent, bool p_force_invisible);
void MCInterfaceExecCreateControl(MCExecContext& ctxt, MCStringRef p_new_name, int p_type, MCObject *p_container, bool p_force_invisible);
void MCInterfaceExecCreateWidget(MCExecContext& ctxt, MCStringRef p_new_name, MCNameRef p_kind, MCObject *p_container, bool p_force_invisible);

void MCInterfaceExecClone(MCExecContext& ctxt, MCObject *p_target, MCStringRef p_new_name, bool p_force_invisible);

void MCInterfaceExecFind(MCExecContext& ctxt, int p_mode, MCStringRef p_needle, MCChunk *p_target);

void MCInterfaceExecPutIntoObject(MCExecContext& ctxt, MCStringRef value, int where, MCObjectChunkPtr target);
void MCInterfaceExecPutIntoObject(MCExecContext& ctxt, MCExecValue value, int where, MCObjectChunkPtr target);
void MCInterfaceExecPutIntoField(MCExecContext& ctxt, MCStringRef value, int where, MCObjectChunkPtr target);
void MCInterfaceExecPutUnicodeIntoField(MCExecContext& ctxt, MCDataRef p_data, int p_where, MCObjectChunkPtr p_chunk);

void MCInterfaceExecLockCursor(MCExecContext& ctxt);
void MCInterfaceExecLockMenus(MCExecContext& ctxt);
void MCInterfaceExecLockMoves(MCExecContext& ctxt);
void MCInterfaceExecLockRecent(MCExecContext& ctxt);
void MCInterfaceExecLockScreen(MCExecContext& ctxt);
void MCInterfaceExecLockScreenForEffect(MCExecContext& ctxt, MCRectangle *region);

void MCInterfaceExecUnlockCursor(MCExecContext& ctxt);
void MCInterfaceExecUnlockMenus(MCExecContext& ctxt);
void MCInterfaceExecUnlockMoves(MCExecContext& ctxt);
void MCInterfaceExecUnlockRecent(MCExecContext& ctxt);
void MCInterfaceExecUnlockScreen(MCExecContext& ctxt);
void MCInterfaceExecUnlockScreenWithEffect(MCExecContext& ctxt, MCVisualEffect *region);

void MCInterfaceExecImportSnapshotOfScreen(MCExecContext& ctxt, MCRectangle *p_region, MCPoint *p_at_size);
void MCInterfaceExecImportSnapshotOfStack(MCExecContext& ctxt, MCStringRef p_stack, MCStringRef p_display, MCRectangle *p_region, MCPoint *p_at_size);
void MCInterfaceExecImportSnapshotOfObject(MCExecContext& ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size);
void MCInterfaceExecImportAudioClip(MCExecContext& ctxt, MCStringRef p_filename);
void MCInterfaceExecImportVideoClip(MCExecContext& ctxt, MCStringRef p_filename);
void MCInterfaceExecImportImage(MCExecContext& ctxt, MCStringRef p_filename, MCStringRef p_mask_filename, MCObject *p_container);
void MCInterfaceExecImportObjectFromArray(MCExecContext& ctxt, MCArrayRef p_array, MCObject *p_container);

void MCInterfaceExecExportSnapshotOfScreen(MCExecContext& ctxt, MCRectangle *p_region, MCPoint *p_size, int format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCDataRef &r_data);
void MCInterfaceExecExportSnapshotOfScreenToFile(MCExecContext& ctxt, MCRectangle *p_region, MCPoint *p_size, int format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename);
void MCInterfaceExecExportSnapshotOfStack(MCExecContext& ctxt, MCStringRef p_stack, MCStringRef p_display, MCRectangle *p_region, MCPoint *p_size, int format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCDataRef &r_data);
void MCInterfaceExecExportSnapshotOfStackToFile(MCExecContext& ctxt, MCStringRef p_stack, MCStringRef p_display, MCRectangle *p_region, MCPoint *p_size, int format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename);
void MCInterfaceExecExportSnapshotOfObject(MCExecContext& ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size, int format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCDataRef &r_data);
void MCInterfaceExecExportSnapshotOfObjectToFile(MCExecContext& ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size, int format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename);
void MCInterfaceExecExportImage(MCExecContext& ctxt, MCImage *p_target, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCDataRef &r_data);
void MCInterfaceExecExportImageToFile(MCExecContext& ctxt, MCImage *p_target, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename);
void MCInterfaceExecExportObjectToArray(MCExecContext& ctxt, MCObject *p_container, MCArrayRef& r_array);

void MCInterfaceExecSortCardsOfStack(MCExecContext &ctxt, MCStack *p_target, bool p_ascending, int p_format, MCExpression *p_by, bool p_only_marked);
void MCInterfaceExecSortField(MCExecContext &ctxt, MCObjectPtr p_target, int p_chunk_type, bool p_ascending, int p_format, MCExpression *p_by);
void MCInterfaceExecSortContainer(MCExecContext &ctxt, MCStringRef& x_target, int p_chunk_type, bool p_ascending, int p_format, MCExpression *p_by);
void MCInterfaceExecReplaceInField(MCExecContext& ctxt, MCStringRef p_pattern, MCStringRef p_replacement, MCObjectChunkPtr& p_container, bool p_preserve_styles);

void MCInterfaceExecChooseTool(MCExecContext& ctxt, MCStringRef p_input, int p_tool);

enum MCInterfaceExecGoVisibility
{
	kMCInterfaceExecGoVisibilityImplicit,
	kMCInterfaceExecGoVisibilityExplicitVisible,
	kMCInterfaceExecGoVisibilityExplicitInvisible
};
void MCInterfaceExecGoCardAsMode(MCExecContext& ctxt, MCCard *p_card, int p_mode, MCInterfaceExecGoVisibility p_visibility_type, bool p_this_stack);
void MCInterfaceExecGoCardInWindow(MCExecContext& ctxt, MCCard *p_card, MCStringRef p_window, MCInterfaceExecGoVisibility p_visibility_type, bool p_this_stack);
void MCInterfaceExecGoRecentCard(MCExecContext& ctxt);
void MCInterfaceExecGoCardRelative(MCExecContext& ctxt, bool p_forward, real8 p_amount);
void MCInterfaceExecGoCardEnd(MCExecContext& ctxt, bool p_is_start);
void MCInterfaceExecGoHome(MCExecContext& ctxt, MCCard *p_card);

void MCInterfaceExecVisualEffect(MCExecContext& ctxt, MCInterfaceVisualEffect p_effect);

void MCInterfaceGetDialogData(MCExecContext& ctxt, MCValueRef& r_value);
void MCInterfaceSetDialogData(MCExecContext& ctxt, MCValueRef p_value);

void MCInterfaceGetLookAndFeel(MCExecContext& ctxt, intenum_t& r_value);
void MCInterfaceSetLookAndFeel(MCExecContext& ctxt, intenum_t p_value);
void MCInterfaceGetScreenMouseLoc(MCExecContext& ctxt, MCPoint& r_value);
void MCInterfaceSetScreenMouseLoc(MCExecContext& ctxt, MCPoint p_value);

void MCInterfaceGetBackdrop(MCExecContext& ctxt, MCInterfaceBackdrop& r_backdrop);
void MCInterfaceSetBackdrop(MCExecContext& ctxt, const MCInterfaceBackdrop& p_backdrop);
void MCInterfaceGetBufferImages(MCExecContext& ctxt, bool &r_value);
void MCInterfaceSetBufferImages(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetSystemFileSelector(MCExecContext& ctxt, bool &r_value);
void MCInterfaceSetSystemFileSelector(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetSystemColorSelector(MCExecContext& ctxt, bool &r_value);
void MCInterfaceSetSystemColorSelector(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetSystemPrintSelector(MCExecContext& ctxt, bool &r_value);
void MCInterfaceSetSystemPrintSelector(MCExecContext& ctxt, bool p_value);

void MCInterfaceGetPaintCompression(MCExecContext& ctxt, intenum_t& r_value);
void MCInterfaceSetPaintCompression(MCExecContext& ctxt, intenum_t value);

void MCInterfaceGetBrushBackColor(MCExecContext& ctxt, MCValueRef& r_color);
void MCInterfaceSetBrushBackColor(MCExecContext& ctxt, MCValueRef color);
void MCInterfaceGetPenBackColor(MCExecContext& ctxt, MCValueRef& r_color);
void MCInterfaceSetPenBackColor(MCExecContext& ctxt, MCValueRef color);
void MCInterfaceGetBrushColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
void MCInterfaceSetBrushColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
void MCInterfaceGetPenColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
void MCInterfaceSetPenColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
void MCInterfaceGetBrushPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
void MCInterfaceSetBrushPattern(MCExecContext& ctxt, uinteger_t* pattern);
void MCInterfaceGetPenPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
void MCInterfaceSetPenPattern(MCExecContext& ctxt, uinteger_t* pattern);
void MCInterfaceGetFilled(MCExecContext& ctxt, bool& r_filled);
void MCInterfaceSetFilled(MCExecContext& ctxt, bool filled);
void MCInterfaceGetPolySides(MCExecContext& ctxt, uinteger_t& r_sides);
void MCInterfaceSetPolySides(MCExecContext& ctxt, uinteger_t sides);
void MCInterfaceGetLineSize(MCExecContext& ctxt, uinteger_t& r_size);
void MCInterfaceSetLineSize(MCExecContext& ctxt, uinteger_t size);
void MCInterfaceGetRoundRadius(MCExecContext& ctxt, uinteger_t& r_radius);
void MCInterfaceSetRoundRadius(MCExecContext& ctxt, uinteger_t radius);
void MCInterfaceGetStartAngle(MCExecContext& ctxt, uinteger_t& r_radius);
void MCInterfaceSetStartAngle(MCExecContext& ctxt, uinteger_t radius);
void MCInterfaceGetArcAngle(MCExecContext& ctxt, uinteger_t& r_radius);
void MCInterfaceSetArcAngle(MCExecContext& ctxt, uinteger_t radius);
void MCInterfaceGetRoundEnds(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetRoundEnds(MCExecContext& ctxt, bool value);
void MCInterfaceGetDashes(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_points);
void MCInterfaceSetDashes(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_points);

void MCInterfaceGetRecentCards(MCExecContext& ctxt, MCStringRef& r_cards);
void MCInterfaceGetRecentNames(MCExecContext& ctxt, MCStringRef& r_names);

void MCInterfaceGetEditBackground(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetEditBackground(MCExecContext& ctxt, bool value);

void MCInterfaceGetLockScreen(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetLockScreen(MCExecContext& ctxt, bool value);

void MCInterfaceGetAccentColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
void MCInterfaceSetAccentColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
void MCInterfaceGetHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
void MCInterfaceSetHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
void MCInterfaceGetLinkColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
void MCInterfaceSetLinkColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
void MCInterfaceGetLinkHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
void MCInterfaceSetLinkHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
void MCInterfaceGetLinkVisitedColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
void MCInterfaceSetLinkVisitedColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
void MCInterfaceGetUnderlineLinks(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetUnderlineLinks(MCExecContext& ctxt, bool value);

void MCInterfaceGetSelectGroupedControls(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetSelectGroupedControls(MCExecContext& ctxt, bool value);

void MCInterfaceGetIcon(MCExecContext& ctxt, uinteger_t& r_icon);
void MCInterfaceSetIcon(MCExecContext& ctxt, uinteger_t icon);

void MCInterfaceGetAllowInlineInput(MCExecContext& ctxt, bool &r_value);
void MCInterfaceSetAllowInlineInput(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetDragDelta(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetDragDelta(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetStackFileType(MCExecContext& ctxt, MCStringRef& r_value);
void MCInterfaceSetStackFileType(MCExecContext& ctxt, MCStringRef p_value);
void MCInterfaceGetStackFileVersion(MCExecContext& ctxt, MCInterfaceStackFileVersion& r_value);
void MCInterfaceSetStackFileVersion(MCExecContext& ctxt, const MCInterfaceStackFileVersion& p_value);

void MCInterfaceGetIconMenu(MCExecContext& ctxt, MCStringRef& r_menu);
void MCInterfaceSetIconMenu(MCExecContext& ctxt, MCStringRef menu);
void MCInterfaceGetStatusIcon(MCExecContext& ctxt, uinteger_t& r_icon);
void MCInterfaceSetStatusIcon(MCExecContext& ctxt, uinteger_t icon);
void MCInterfaceGetStatusIconToolTip(MCExecContext& ctxt, MCStringRef& r_tooltip);
void MCInterfaceSetStatusIconToolTip(MCExecContext& ctxt, MCStringRef tooltip);
void MCInterfaceGetStatusIconMenu(MCExecContext& ctxt, MCStringRef& r_icon_menu);
void MCInterfaceSetStatusIconMenu(MCExecContext& ctxt, MCStringRef icon_menu);

void MCInterfaceGetProcessType(MCExecContext& ctxt, intenum_t& r_value);
void MCInterfaceSetProcessType(MCExecContext& ctxt, intenum_t value);
void MCInterfaceGetShowInvisibles(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetShowInvisibles(MCExecContext& ctxt, bool p_value);

// SN-2015-07-29: [[ Bug 15649 ]] The cursor can be empty - it is optional
void MCInterfaceGetCursor(MCExecContext& ctxt, uinteger_t *&r_value);
void MCInterfaceSetCursor(MCExecContext& ctxt, uinteger_t* p_value);
void MCInterfaceGetDefaultCursor(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetDefaultCursor(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetDefaultStack(MCExecContext& ctxt, MCStringRef& r_value);
void MCInterfaceSetDefaultStack(MCExecContext& ctxt, MCStringRef p_value);
void MCInterfaceGetDefaultMenubar(MCExecContext& ctxt, MCNameRef& r_value);
void MCInterfaceSetDefaultMenubar(MCExecContext& ctxt, MCNameRef p_value);
void MCInterfaceGetDragSpeed(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetDragSpeed(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetMoveSpeed(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetMoveSpeed(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetLockCursor(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetLockCursor(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetLockErrors(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetLockErrors(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetLockMenus(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetLockMenus(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetLockMessages(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetLockMessages(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetLockMoves(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetLockMoves(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetLockRecent(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetLockRecent(MCExecContext& ctxt, bool p_value);

void MCInterfaceGetIdleRate(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetIdleRate(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetIdleTicks(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetIdleTicks(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetBlinkRate(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetBlinkRate(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetRepeatRate(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetRepeatRate(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetRepeatDelay(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetRepeatDelay(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetTypeRate(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetTypeRate(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetSyncRate(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetSyncRate(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetEffectRate(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetEffectRate(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetDoubleDelta(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetDoubleDelta(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetDoubleTime(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetDoubleTime(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetTooltipDelay(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetTooltipDelay(MCExecContext& ctxt, uinteger_t p_value);

void MCInterfaceGetNavigationArrows(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetNavigationArrows(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetExtendKey(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetExtendKey(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetPointerFocus(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetPointerFocus(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetEmacsKeyBindings(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetEmacsKeyBindings(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetRaiseMenus(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetRaiseMenus(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetActivatePalettes(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetActivatePalettes(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetHidePalettes(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetHidePalettes(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetRaisePalettes(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetRaisePalettes(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetRaiseWindows(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetRaiseWindows(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetHideBackdrop(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetHideBackdrop(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetDontUseNavigationServices(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetDontUseNavigationServices(MCExecContext& ctxt, bool p_value);

void MCInterfaceGetProportionalThumbs(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetProportionalThumbs(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetSharedMemory(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetSharedMemory(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetScreenGamma(MCExecContext& ctxt, double& r_value);
void MCInterfaceSetScreenGamma(MCExecContext& ctxt, double p_value);
void MCInterfaceGetSelectionMode(MCExecContext& ctxt, intenum_t& r_value);
void MCInterfaceSetSelectionMode(MCExecContext& ctxt, intenum_t p_value);
void MCInterfaceGetSystemAppearance(MCExecContext& ctxt, intenum_t& r_value);
void MCInterfaceGetSelectionHandleColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
void MCInterfaceSetSelectionHandleColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color);
void MCInterfaceGetWindowBoundingRect(MCExecContext& ctxt, MCRectangle& r_value);
void MCInterfaceSetWindowBoundingRect(MCExecContext& ctxt, MCRectangle p_value);
void MCInterfaceGetJpegQuality(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetJpegQuality(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetRelayerGroupedControls(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetRelayerGroupedControls(MCExecContext& ctxt, bool p_value);

void MCInterfaceGetBrush(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetBrush(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetEraser(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetEraser(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetSpray(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetSpray(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetCentered(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetCentered(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetGrid(MCExecContext& ctxt, bool& r_value);
void MCInterfaceSetGrid(MCExecContext& ctxt, bool p_value);
void MCInterfaceGetGridSize(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetGridSize(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetSlices(MCExecContext& ctxt, uinteger_t& r_value);
void MCInterfaceSetSlices(MCExecContext& ctxt, uinteger_t p_value);
void MCInterfaceGetBeepLoudness(MCExecContext& ctxt, integer_t& r_value);
void MCInterfaceSetBeepLoudness(MCExecContext& ctxt, integer_t p_value);
void MCInterfaceGetBeepPitch(MCExecContext& ctxt, integer_t& r_value);
void MCInterfaceSetBeepPitch(MCExecContext& ctxt, integer_t p_value);
void MCInterfaceGetBeepDuration(MCExecContext& ctxt, integer_t& r_value);
void MCInterfaceSetBeepDuration(MCExecContext& ctxt, integer_t p_value);
void MCInterfaceGetBeepSound(MCExecContext& ctxt, MCStringRef& r_value);
void MCInterfaceSetBeepSound(MCExecContext& ctxt, MCStringRef p_value);
void MCInterfaceGetTool(MCExecContext& ctxt, MCStringRef& r_value);
void MCInterfaceSetTool(MCExecContext& ctxt, MCStringRef p_value);

void MCInterfaceGetScreenRect(MCExecContext& ctxt, bool p_working, bool p_effective, MCRectangle& r_value);
void MCInterfaceGetScreenRects(MCExecContext& ctxt, bool p_working, bool p_effective, MCStringRef& r_value);

void MCInterfaceEvalHelpStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalHomeStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalSelectedObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalTopStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalClickStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);

MCStack *MCInterfaceTryToEvalStackFromString(MCStringRef p_data);
bool MCInterfaceStringCouldBeStack(MCStringRef p_string);

void MCInterfaceEvalMouseStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalClickFieldAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalSelectedFieldAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalSelectedImageAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalFoundFieldAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalMouseControlAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalFocusedObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalBinaryStackAsObject(MCExecContext& ctxt, MCStringRef p_data, MCObjectPtr& r_object);
void MCInterfaceEvalDefaultStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCInterfaceEvalStackOfStackByName(MCExecContext& ctxt, MCObjectPtr p_parent, MCNameRef p_name, MCObjectPtr& r_stack);
void MCInterfaceEvalStackOfStackById(MCExecContext& ctxt, MCObjectPtr p_parent, uinteger_t p_id, MCObjectPtr& r_stack);
void MCInterfaceEvalStackByValue(MCExecContext& ctxt, MCValueRef p_value, MCObjectPtr& r_stack);
void MCInterfaceEvalSubstackOfStackByName(MCExecContext& ctxt, MCObjectPtr p_parent, MCNameRef p_name, MCObjectPtr& r_stack);
void MCInterfaceEvalSubstackOfStackById(MCExecContext& ctxt, MCObjectPtr p_parent, uinteger_t p_id, MCObjectPtr& r_stack);
void MCInterfaceEvalAudioClipOfStackByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_ordinal_type, MCObjectPtr& r_clip);
void MCInterfaceEvalAudioClipOfStackById(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_id, MCObjectPtr& r_clip);
void MCInterfaceEvalAudioClipOfStackByName(MCExecContext& ctxt, MCObjectPtr p_stack, MCNameRef p_name, MCObjectPtr& r_clip);
void MCInterfaceEvalVideoClipOfStackByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_ordinal_type, MCObjectPtr& r_clip);
void MCInterfaceEvalVideoClipOfStackById(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_id, MCObjectPtr& r_clip);
void MCInterfaceEvalVideoClipOfStackByName(MCExecContext& ctxt, MCObjectPtr p_stack, MCNameRef p_name, MCObjectPtr& r_clip);
void MCInterfaceEvalBackgroundOfStackByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, Chunk_term p_ordinal_type, MCObjectPtr& r_bg);
void MCInterfaceEvalBackgroundOfStackById(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_id, MCObjectPtr& r_bg);
void MCInterfaceEvalBackgroundOfStackByName(MCExecContext& ctxt, MCObjectPtr p_stack, MCNameRef p_name, MCObjectPtr& r_bg);
void MCInterfaceEvalStackWithBackgroundByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, Chunk_term p_ordinal_type, MCObjectPtr& r_stack);
void MCInterfaceEvalStackWithBackgroundById(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_id, MCObjectPtr& r_stack);
void MCInterfaceEvalStackWithBackgroundByName(MCExecContext& ctxt, MCObjectPtr p_stack, MCNameRef p_name, MCObjectPtr& r_stack);
void MCInterfaceEvalCardOfStackByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, bool p_marked, Chunk_term p_ordinal_type, MCObjectPtr& r_card);
void MCInterfaceEvalThisCardOfStack(MCExecContext& ctxt, MCObjectPtr p_stack, MCObjectPtr& r_card);
void MCInterfaceEvalCardOfStackById(MCExecContext& ctxt, MCObjectPtr p_stack, bool p_marked, uinteger_t p_id, MCObjectPtr& r_card);
void MCInterfaceEvalCardOfStackByName(MCExecContext& ctxt, MCObjectPtr p_stack, bool p_marked, MCNameRef p_name, MCObjectPtr& r_card);
void MCInterfaceEvalCardOfBackgroundByOrdinal(MCExecContext& ctxt, MCObjectPtr p_background, bool p_marked, Chunk_term p_ordinal_type, MCObjectPtr& r_card);
void MCInterfaceEvalCardOfBackgroundById(MCExecContext& ctxt, MCObjectPtr p_background, bool p_marked, uinteger_t p_id, MCObjectPtr& r_card);
void MCInterfaceEvalCardOfBackgroundByName(MCExecContext& ctxt, MCObjectPtr p_background, bool p_marked, MCNameRef p_name, MCObjectPtr& r_card);
void MCInterfaceEvalGroupOfCardByOrdinal(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_parent_type, Chunk_term p_ordinal_type, MCObjectPtr& r_group);
void MCInterfaceEvalGroupOfCardById(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_parent_type, uinteger_t p_id, MCObjectPtr& r_group);
void MCInterfaceEvalGroupOfCardByName(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_parent_type, MCNameRef p_name, MCObjectPtr& r_group);
void MCInterfaceEvalGroupOfCardOrStackById(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_parent_type, uinteger_t p_id, MCObjectPtr& r_group);
void MCInterfaceEvalGroupOfGroupByOrdinal(MCExecContext& ctxt, MCObjectPtr p_group, Chunk_term p_ordinal_type, MCObjectPtr& r_group);
void MCInterfaceEvalGroupOfGroupById(MCExecContext& ctxt, MCObjectPtr p_group, uinteger_t p_id, MCObjectPtr& r_group);
void MCInterfaceEvalGroupOfGroupByName(MCExecContext& ctxt, MCObjectPtr p_group, MCNameRef p_name, MCObjectPtr& r_group);
void MCInterfaceEvalMenubarAsObject(MCExecContext& ctxt, MCObjectPtr& r_menubar);
void MCInterfaceEvalObjectOfGroupByOrdinal(MCExecContext& ctxt, MCObjectPtr p_group, Chunk_term p_object_type, Chunk_term p_ordinal_type, MCObjectPtr& r_object);
void MCInterfaceEvalObjectOfGroupById(MCExecContext& ctxt, MCObjectPtr p_group, Chunk_term p_object_type, uinteger_t p_id, MCObjectPtr& r_object);
void MCInterfaceEvalObjectOfGroupByName(MCExecContext& ctxt, MCObjectPtr p_group, Chunk_term p_object_type, MCNameRef p_name, MCObjectPtr& r_object);
void MCInterfaceEvalObjectOfCardByOrdinal(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_object_type, Chunk_term p_parent_type, Chunk_term p_ordinal_type, MCObjectPtr& r_object);
void MCInterfaceEvalObjectOfCardById(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_object_type, Chunk_term p_parent_type, uinteger_t p_id, MCObjectPtr& r_object);
void MCInterfaceEvalObjectOfCardOrStackById(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_object_type, Chunk_term p_parent_type, uinteger_t p_id, MCObjectPtr& r_object);
void MCInterfaceEvalObjectOfCardByName(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_object_type, Chunk_term p_parent_type, MCNameRef p_name, MCObjectPtr& r_object);

void MCInterfaceEvalStackOfObject(MCExecContext& ctxt, MCObjectPtr p_object, MCObjectPtr& r_object);
void MCInterfaceEvalStackWithOptionalBackground(MCExecContext& ctxt, MCObjectPtr p_object, MCObjectPtr& r_object);

void MCInterfaceMarkObject(MCExecContext& ctxt, MCObjectPtr p_object, Boolean wholechunk, MCMarkedText& r_mark);
void MCInterfaceMarkContainer(MCExecContext& ctxt, MCObjectPtr p_container, Boolean wholechunk, MCMarkedText& r_mark);
void MCInterfaceMarkFunction(MCExecContext& ctxt, MCObjectPtr p_object, Functions p_function, bool p_whole_chunk, MCMarkedText& r_mark);
void MCInterfaceMarkCharsOfField(MCExecContext& ctxt, MCObjectPtr t_field, MCCRef *character, MCMarkedText &x_mark);

void MCInterfaceEvalTextOfContainer(MCExecContext& ctxt, MCObjectPtr p_container, MCStringRef &r_text);

void MCInterfaceExecMarkCard(MCExecContext& ctxt, MCObjectPtr t_object);
void MCInterfaceExecUnmarkCard(MCExecContext& ctxt, MCObjectPtr t_object);
void MCInterfaceExecMarkCardsConditional(MCExecContext& ctxt, MCExpression *p_where);
void MCInterfaceExecMarkAllCards(MCExecContext& ctxt);
void MCInterfaceExecUnmarkCardsConditional(MCExecContext& ctxt, MCExpression *p_where);
void MCInterfaceExecUnmarkAllCards(MCExecContext& ctxt);
void MCInterfaceExecMarkFind(MCExecContext& ctxt, Find_mode p_mode, MCStringRef p_needle, MCChunk *p_field);
void MCInterfaceExecUnmarkFind(MCExecContext& ctxt, Find_mode p_mode, MCStringRef p_needle, MCChunk *p_field);

void MCInterfaceExecRelayer(MCExecContext& ctxt, int p_relation, MCObjectPtr p_source, uinteger_t p_layer);
void MCInterfaceExecRelayerRelativeToControl(MCExecContext& ctxt, int p_relation, MCObjectPtr p_source, MCObjectPtr p_target);
void MCInterfaceExecRelayerRelativeToOwner(MCExecContext& ctxt, int p_relation, MCObjectPtr p_source);

void MCInterfaceExecResolveImageById(MCExecContext& ctxt, MCObject *p_object, uinteger_t p_id);
void MCInterfaceExecResolveImageByName(MCExecContext& ctxt, MCObject *p_object, MCStringRef p_name);

void MCInterfaceGetPixelScale(MCExecContext& ctxt, double &r_scale);
void MCInterfaceSetPixelScale(MCExecContext& ctxt, double p_scale);
void MCInterfaceGetSystemPixelScale(MCExecContext& ctxt, double &r_scale);
void MCInterfaceSetUsePixelScaling(MCExecContext& ctxt, bool p_setting);
void MCInterfaceGetUsePixelScaling(MCExecContext& ctxt, bool &r_setting);
void MCInterfaceGetScreenPixelScale(MCExecContext& ctxt, double& r_scale);
void MCInterfaceGetScreenPixelScales(MCExecContext& ctxt, uindex_t& r_count, double*& r_scale);

void MCInterfaceExecGoBackInWidget(MCExecContext& ctxt, MCWidget *p_widget);
void MCInterfaceExecGoForwardInWidget(MCExecContext& ctxt, MCWidget *p_widget);
void MCInterfaceExecLaunchUrlInWidget(MCExecContext& ctxt, MCStringRef p_url, MCWidget *p_widget);
void MCInterfaceExecDoInWidget(MCExecContext& ctxt, MCStringRef p_script, MCWidget *p_widget);

///////////

struct MCInterfaceLayer;

extern MCExecCustomTypeInfo *kMCInterfaceLayerTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceShadowTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceTextAlignTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceTextStyleTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceInkNamesTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceEncodingTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceListStyleTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceThemeTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceThemeControlTypeTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceScriptStatusTypeInfo;

///////////

extern MCExecEnumTypeInfo *kMCInterfacePlayDestinationTypeInfo;

///////////

extern MCExecEnumTypeInfo *kMCInterfaceStackStyleTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceCharsetTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceCompositorTypeTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceStackFullscreenModeTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceDecorationTypeInfo;

///////////

extern MCExecCustomTypeInfo *kMCInterfaceMarginsTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceLayerModeTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceBitmapEffectSourceTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceBitmapEffectBlendModeTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceBitmapEffectFilterTypeInfo;

///////////

extern MCExecEnumTypeInfo *kMCInterfaceButtonStyleTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceButtonMenuModeTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceButtonIconGravityTypeInfo;
extern MCExecSetTypeInfo *kMCInterfaceButtonAcceleratorModifiersTypeInfo;

///////////

struct MCInterfaceFieldTabAlignments
{
    uindex_t m_count;
    intenum_t *m_alignments;
};

extern MCExecEnumTypeInfo *kMCInterfaceFieldStyleTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceFieldRangesTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceFieldCursorMovementTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceTextDirectionTypeInfo;
extern MCExecCustomTypeInfo *kMCInterfaceFieldTabAlignmentsTypeInfo;
extern MCExecEnumTypeInfo* kMCInterfaceKeyboardTypeTypeInfo;
extern MCExecEnumTypeInfo* kMCInterfaceReturnKeyTypeTypeInfo;

///////////

extern MCExecEnumTypeInfo *kMCInterfaceGraphicFillRuleTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceGraphicEditModeTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceGraphicCapStyleTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceGraphicJoinStyleTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceGraphicStyleTypeInfo;

extern MCExecEnumTypeInfo *kMCInterfaceGradientFillKindTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceGradientFillQualityTypeInfo;

///////////

extern MCExecEnumTypeInfo *kMCInterfaceImageResizeQualityTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceImagePaintCompressionTypeInfo;

///////////

extern MCExecEnumTypeInfo *kMCInterfacePlayerStatusTypeInfo;
extern MCExecSetTypeInfo *kMCInterfaceMediaTypesTypeInfo;
extern MCExecCustomTypeInfo *kMCMultimediaTrackTypeInfo;
extern MCExecCustomTypeInfo *kMCMultimediaQTVRConstraintsTypeInfo;
extern MCExecCustomTypeInfo *kMCMultimediaQTVRNodeTypeInfo;
extern MCExecCustomTypeInfo *kMCMultimediaQTVRHotSpotTypeInfo;

///////////

extern MCExecEnumTypeInfo *kMCInterfaceScrollbarStyleTypeInfo;
extern MCExecEnumTypeInfo *kMCInterfaceScrollbarOrientationTypeInfo;

///////////

void MCDialogExecAnswerColor(MCExecContext &ctxt, MCColor *initial_color, MCStringRef p_title, bool p_sheet);
void MCDialogExecAnswerFile(MCExecContext &ctxt, bool p_plural, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef p_title, bool p_sheet);
void MCDialogExecAnswerFileWithFilter(MCExecContext &ctxt, bool p_plural, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef p_filter, MCStringRef p_title, bool p_sheet);
void MCDialogExecAnswerFileWithTypes(MCExecContext &ctxt, bool p_plural, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef *p_types, uindex_t p_type_count, MCStringRef p_title, bool p_sheet);
void MCDialogExecAnswerFolder(MCExecContext &ctxt, bool p_plural, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef p_title, bool p_sheet);
void MCDialogExecAnswerNotify(MCExecContext &ctxt, int p_type, MCStringRef p_prompt, MCStringRef *p_buttons, uindex_t p_button_count, MCStringRef p_title, bool p_sheet);
void MCDialogExecCustomAnswerDialog(MCExecContext &ctxt, MCNameRef p_stack, MCNameRef p_type, bool p_sheet, MCStringRef *p_arg_list, uindex_t p_arg_count, MCStringRef &r_result);

void MCDialogExecAskQuestion(MCExecContext& ctxt, int type, MCStringRef prompt, MCStringRef answer, bool hint_answer, MCStringRef title, bool as_sheet);
void MCDialogExecAskPassword(MCExecContext& ctxt, bool clear, MCStringRef prompt, MCStringRef answer, bool hint_answer, MCStringRef title, bool as_sheet);
void MCDialogExecAskFile(MCExecContext& ctxt, MCStringRef prompt, MCStringRef initial, MCStringRef title, bool as_sheet);
void MCDialogExecAskFileWithFilter(MCExecContext& ctxt, MCStringRef prompt, MCStringRef initial, MCStringRef filter, MCStringRef title, bool as_sheet);
void MCDialogExecAskFileWithTypes(MCExecContext& ctxt, MCStringRef prompt, MCStringRef intial, MCStringRef *types, uindex_t type_count, MCStringRef title, bool as_sheet);
void MCDialogExecCustomAskDialog(MCExecContext& ctxt, MCNameRef p_stack, MCNameRef p_type, bool p_sheet, MCStringRef *p_arglist, uindex_t p_arg_count, bool& r_cancelled, MCStringRef& r_result);

void MCDialogGetColorDialogColors(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_color_list);
void MCDialogSetColorDialogColors(MCExecContext& ctxt, uindex_t p_count, MCStringRef* p_color_list);

///////////

extern MCExecEnumTypeInfo *kMCPasteboardDragActionTypeInfo;
extern MCExecSetTypeInfo *kMCPasteboardAllowableDragActionsTypeInfo;

void MCPasteboardEvalClipboard(MCExecContext& ctxt, MCNameRef& r_string);
void MCPasteboardEvalClipboardKeys(MCExecContext& ctxt, MCStringRef& r_string);
void MCPasteboardEvalDropChunk(MCExecContext& ctxt, MCStringRef& r_string);
void MCPasteboardEvalDragDestination(MCExecContext& ctxt, MCStringRef& r_string);
void MCPasteboardEvalDragSource(MCExecContext& ctxt, MCStringRef& r_string);
void MCPasteboardEvalDragDropKeys(MCExecContext& ctxt, MCStringRef& r_string);

void MCPasteboardEvalRawClipboardKeys(MCExecContext& ctxt, MCStringRef& r_string);
void MCPasteboardEvalRawDragKeys(MCExecContext& ctxt, MCStringRef& r_string);
void MCPasteboardEvalFullClipboardKeys(MCExecContext& ctxt, MCStringRef& r_string);
void MCPasteboardEvalFullDragKeys(MCExecContext& ctxt, MCStringRef& r_string);

void MCPasteboardEvalIsAmongTheKeysOfTheClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsNotAmongTheKeysOfTheClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsAmongTheKeysOfTheDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsNotAmongTheKeysOfTheDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);

void MCPasteboardEvalIsAmongTheKeysOfTheRawClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsNotAmongTheKeysOfTheRawClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsAmongTheKeysOfTheRawDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsNotAmongTheKeysOfTheRawDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsAmongTheKeysOfTheFullClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsNotAmongTheKeysOfTheFullClipboardData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsAmongTheKeysOfTheFullDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);
void MCPasteboardEvalIsNotAmongTheKeysOfTheFullDragData(MCExecContext& ctxt, MCNameRef p_key, bool& r_result);

void MCPasteboardEvalDragSourceAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCPasteboardEvalDragDestinationAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCPasteboardEvalDropChunkAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);

void MCPasteboardExecPaste(MCExecContext& ctxt);

void MCPasteboardExecCopy(MCExecContext& ctxt);
void MCPasteboardExecCopyTextToClipboard(MCExecContext& ctxt, MCObjectChunkPtr p_target);
void MCPasteboardExecCopyObjectsToClipboard(MCExecContext& ctxt, MCObjectPtr *p_targets, uindex_t p_target_count);
void MCPasteboardExecCut(MCExecContext& ctxt);
void MCPasteboardExecCutTextToClipboard(MCExecContext& ctxt, MCObjectChunkPtr p_target);
void MCPasteboardExecCutObjectsToClipboard(MCExecContext& ctxt, MCObjectPtr *p_targets, uindex_t p_target_count);

void MCPasteboardExecLockClipboard(MCExecContext& ctxt);
void MCPasteboardExecUnlockClipboard(MCExecContext& ctxt);

void MCPasteboardGetAcceptDrop(MCExecContext& ctxt, bool& r_value);
void MCPasteboardSetAcceptDrop(MCExecContext& ctxt, bool p_value);
void MCPasteboardGetDragAction(MCExecContext& ctxt, intenum_t& r_value);
void MCPasteboardSetDragAction(MCExecContext& ctxt, intenum_t p_value);
void MCPasteboardGetDragImage(MCExecContext& ctxt, uinteger_t& r_value);
void MCPasteboardSetDragImage(MCExecContext& ctxt, uinteger_t p_value);
void MCPasteboardGetDragImageOffset(MCExecContext& ctxt, MCPoint*& r_value);
void MCPasteboardSetDragImageOffset(MCExecContext& ctxt, MCPoint *p_value);
void MCPasteboardGetAllowableDragActions(MCExecContext& ctxt, intset_t& r_value);
void MCPasteboardSetAllowableDragActions(MCExecContext& ctxt, intset_t p_value);

void MCPasteboardGetClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data);
void MCPasteboardSetClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data);
void MCPasteboardGetRawClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data);
void MCPasteboardSetRawClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data);
void MCPasteboardGetRawDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data);
void MCPasteboardSetRawDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data);
void MCPasteboardGetFullClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data);
void MCPasteboardSetFullClipboardData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data);
void MCPasteboardGetFullDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data);
void MCPasteboardSetFullDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data);
void MCPasteboardGetDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_data);
void MCPasteboardSetDragData(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_data);
void MCPasteboardGetClipboardTextData(MCExecContext& ctxt, MCValueRef& r_data);
void MCPasteboardSetClipboardTextData(MCExecContext& ctxt, MCValueRef p_data);
void MCPasteboardGetRawClipboardTextData(MCExecContext& ctxt, MCValueRef& r_data);
void MCPasteboardSetRawClipboardTextData(MCExecContext& ctxt, MCValueRef p_data);
void MCPasteboardGetRawDragTextData(MCExecContext& ctxt, MCValueRef& r_data);
void MCPasteboardSetRawDragTextData(MCExecContext& ctxt, MCValueRef p_data);
void MCPasteboardGetFullClipboardTextData(MCExecContext& ctxt, MCValueRef& r_data);
void MCPasteboardSetFullClipboardTextData(MCExecContext& ctxt, MCValueRef p_data);
void MCPasteboardGetFullDragTextData(MCExecContext& ctxt, MCValueRef& r_data);
void MCPasteboardSetFullDragTextData(MCExecContext& ctxt, MCValueRef p_data);
void MCPasteboardGetDragTextData(MCExecContext& ctxt, MCValueRef& r_data);
void MCPasteboardSetDragTextData(MCExecContext& ctxt, MCValueRef p_data);

///////////

struct MCEngineNumberFormat;

extern MCExecCustomTypeInfo *kMCEngineNumberFormatTypeInfo;
extern MCExecSetTypeInfo *kMCEngineSecurityCategoriesTypeInfo;

void MCEngineEvalVersion(MCExecContext& ctxt, MCNameRef& r_name);
void MCEngineEvalBuildNumber(MCExecContext& ctxt, integer_t& r_build_number);
void MCEngineEvalPlatform(MCExecContext& ctxt, MCNameRef& r_name);
void MCEngineEvalEnvironment(MCExecContext& ctxt, MCNameRef& r_name);
void MCEngineEvalMachine(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalProcessor(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalSystemVersion(MCExecContext& ctxt, MCStringRef& r_string);

void MCEngineEvalCommandNames(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalConstantNames(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalFunctionNames(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalPropertyNames(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalGlobalNames(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalLocalNames(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalVariableNames(MCExecContext& ctxt, MCStringRef& r_string);

void MCEngineEvalParam(MCExecContext& ctxt, integer_t p_index, MCValueRef& r_value);
void MCEngineEvalParamCount(MCExecContext& ctxt, integer_t& r_count);
void MCEngineEvalParams(MCExecContext& ctxt, MCStringRef& r_string);

void MCEngineEvalResult(MCExecContext& ctxt, MCValueRef& r_value);

void MCEngineEvalBackScripts(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalFrontScripts(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalPendingMessages(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalInterrupt(MCExecContext& ctxt, bool& r_bool);

void MCEngineEvalMe(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalTarget(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalTargetContents(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalOwner(MCExecContext& ctxt, MCObjectPtr p_object, MCStringRef& r_string);

void MCEngineEvalScriptLimits(MCExecContext& ctxt, MCStringRef& r_string);
void MCEngineEvalSysError(MCExecContext& ctxt, uinteger_t& r_error);

void MCEngineEvalValue(MCExecContext& ctxt, MCStringRef p_script, MCValueRef& r_value);
void MCEngineEvalValueWithObject(MCExecContext& ctxt, MCStringRef p_script, MCObjectPtr p_object, MCValueRef& r_value);

bool MCEngineEvalValueAsObject(MCValueRef p_value, bool p_strict, MCObjectPtr& r_object, bool& r_parse_error);
void MCEngineEvalValueAsObject(MCExecContext& ctxt, MCValueRef p_value, MCObjectPtr& r_object);
void MCEngineEvalOwnerAsObject(MCExecContext& ctxt, MCObjectPtr p_object, MCObjectPtr& r_owner);
void MCEngineEvalTemplateAsObject(MCExecContext& ctxt, uinteger_t p_template_type, MCObjectPtr& r_object);
void MCEngineEvalMeAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCEngineEvalMenuObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCEngineEvalTargetAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);
void MCEngineEvalErrorObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object);

void MCEngineExecGet(MCExecContext& ctxt, /* take */ MCExecValue& value);
void MCEngineExecPutIntoVariable(MCExecContext& ctxt, MCValueRef value, int where, MCVariableChunkPtr t_target);
void MCEngineExecPutIntoVariable(MCExecContext& ctxt, MCExecValue value, int where, MCVariableChunkPtr t_target);
void MCEngineExecPutOutput(MCExecContext& ctxt, MCStringRef value);
void MCEngineExecPutOutputUnicode(MCExecContext& ctxt, MCDataRef value);

void MCEngineExecDo(MCExecContext& ctxt, MCStringRef p_script, int p_line, int p_pos);
void MCEngineExecDoInCaller(MCExecContext& ctxt, MCStringRef p_script, int p_line, int p_pos);
void MCEngineExecInsertScriptOfObjectInto(MCExecContext& ctxt, MCObject *p_script, bool p_in_front);
void MCEngineExecQuit(MCExecContext& ctxt, integer_t p_retcode);

void MCEngineExecCancelMessage(MCExecContext& ctxt, integer_t p_id);

void MCEngineExecDeleteVariable(MCExecContext& ctxt, MCVarref *p_target);
void MCEngineExecDeleteVariableChunks(MCExecContext& ctxt, MCVariableChunkPtr *chunk, uindex_t chunk_count);

void MCEngineExecRemoveAllScriptsFrom(MCExecContext& ctxt, bool p_in_front);
void MCEngineExecRemoveScriptOfObjectFrom(MCExecContext& ctxt, MCObject *p_script, bool p_in_front);

void MCEngineExecWaitFor(MCExecContext& ctxt, double p_delay, int p_units, bool p_messages);
void MCEngineExecWaitUntil(MCExecContext& ctxt, MCExpression *p_condition, bool p_messages);
void MCEngineExecWaitWhile(MCExecContext& ctxt, MCExpression *p_condition, bool p_messages);

void MCEngineExecStartUsingStack(MCExecContext& ctxt, MCStack *p_stack);
void MCEngineExecStartUsingStackByName(MCExecContext& ctxt, MCStringRef p_name);

void MCEngineExecStopUsingStack(MCExecContext& ctxt, MCStack *p_stack);
void MCEngineExecStopUsingStackByName(MCExecContext& ctxt, MCStringRef p_name);

void MCEngineExecDispatch(MCExecContext& ctxt, int handler_type, MCNameRef message, MCObjectPtr *target, MCParameter *params);
void MCEngineExecSend(MCExecContext& ctxt, MCStringRef script, MCObjectPtr *target);
void MCEngineExecSendScript(MCExecContext& ctxt, MCStringRef script, MCObjectPtr *target);
void MCEngineExecSendInTime(MCExecContext& ctxt, MCStringRef script, MCObjectPtr target, double p_delay, int p_units);
void MCEngineExecCall(MCExecContext& ctxt, MCStringRef script, MCObjectPtr *target);

void MCEngineExecLockErrors(MCExecContext& ctxt);
void MCEngineExecLockMessages(MCExecContext& ctxt);

void MCEngineExecUnlockErrors(MCExecContext& ctxt);
void MCEngineExecUnlockMessages(MCExecContext& ctxt);

void MCEngineExecSet(MCExecContext& ctxt, MCProperty *target, MCValueRef value);
void MCEngineExecReturn(MCExecContext& ctxt, MCValueRef value);
void MCEngineExecReturnValue(MCExecContext& ctxt, MCValueRef value);
void MCEngineExecReturnError(MCExecContext& ctxt, MCValueRef value);

void MCEngineExecLoadExtension(MCExecContext& ctxt, MCStringRef filename, MCStringRef resource_path);
void MCEngineExecUnloadExtension(MCExecContext& ctxt, MCStringRef filename);

void MCEngineLoadExtensionFromData(MCExecContext& ctxt, MCDataRef p_extension_data, MCStringRef p_resource_path);

void MCEngineSetCaseSensitive(MCExecContext& ctxt, bool p_value);
void MCEngineGetCaseSensitive(MCExecContext& ctxt, bool& r_value);
void MCEngineSetFormSensitive(MCExecContext& ctxt, bool p_value);
void MCEngineGetFormSensitive(MCExecContext& ctxt, bool& r_value);
void MCEngineSetCenturyCutOff(MCExecContext& ctxt, integer_t p_value);
void MCEngineGetCenturyCutOff(MCExecContext& ctxt, integer_t& r_value);
void MCEngineSetConvertOctals(MCExecContext& ctxt, bool p_value);
void MCEngineGetConvertOctals(MCExecContext& ctxt, bool& r_value);
void MCEngineSetItemDelimiter(MCExecContext& ctxt, MCStringRef p_value);
void MCEngineGetItemDelimiter(MCExecContext& ctxt, MCStringRef& r_value);
void MCEngineSetLineDelimiter(MCExecContext& ctxt, MCStringRef p_value);
void MCEngineGetLineDelimiter(MCExecContext& ctxt, MCStringRef& r_value);
void MCEngineSetColumnDelimiter(MCExecContext& ctxt, MCStringRef p_value);
void MCEngineGetColumnDelimiter(MCExecContext& ctxt, MCStringRef& r_value);
void MCEngineSetRowDelimiter(MCExecContext& ctxt, MCStringRef p_value);
void MCEngineGetRowDelimiter(MCExecContext& ctxt, MCStringRef& r_value);
void MCEngineSetWholeMatches(MCExecContext& ctxt, bool p_value);
void MCEngineGetWholeMatches(MCExecContext& ctxt, bool& r_value);
void MCEngineSetUseSystemDate(MCExecContext& ctxt, bool p_value);
void MCEngineGetUseSystemDate(MCExecContext& ctxt, bool& r_value);
void MCEngineSetUseUnicode(MCExecContext& ctxt, bool p_value);
void MCEngineGetUseUnicode(MCExecContext& ctxt, bool& r_value);
void MCEngineSetNumberFormat(MCExecContext& ctxt, const MCEngineNumberFormat& format);
void MCEngineGetNumberFormat(MCExecContext& ctxt, MCEngineNumberFormat& r_format);

void MCEngineGetScriptExecutionErrors(MCExecContext& ctxt, MCStringRef &r_value);
void MCEngineGetScriptParsingErrors(MCExecContext& ctxt, MCStringRef &r_value);
void MCEngineGetAllowInterrupts(MCExecContext& ctxt, bool& r_value);
void MCEngineSetAllowInterrupts(MCExecContext& ctxt, bool p_value);
void MCEngineGetExplicitVariables(MCExecContext& ctxt, bool& r_value);
void MCEngineSetExplicitVariables(MCExecContext& ctxt, bool p_value);
void MCEngineGetPreserveVariables(MCExecContext& ctxt, bool& r_value);
void MCEngineSetPreserveVariables(MCExecContext& ctxt, bool p_value);

void MCEngineGetStackLimit(MCExecContext& ctxt, uinteger_t& r_limit);
void MCEngineGetEffectiveStackLimit(MCExecContext& ctxt, uinteger_t& r_limit);
void MCEngineSetStackLimit(MCExecContext& ctxt, uinteger_t limit);

void MCEngineGetSecureMode(MCExecContext& ctxt, bool& r_value);
void MCEngineSetSecureMode(MCExecContext& ctxt, bool p_value);
void MCEngineGetSecurityCategories(MCExecContext& ctxt, intset_t& r_value);
void MCEngineGetSecurityPermissions(MCExecContext& ctxt, intset_t& r_value);
void MCEngineSetSecurityPermissions(MCExecContext& ctxt, intset_t p_value);

void MCEngineGetRecursionLimit(MCExecContext& ctxt, uinteger_t& r_value);
void MCEngineSetRecursionLimit(MCExecContext& ctxt, uinteger_t p_value);

void MCEngineGetAddress(MCExecContext& ctxt, MCStringRef &r_value);
void MCEngineGetStacksInUse(MCExecContext& ctxt, MCStringRef &r_value);

void MCEngineMarkVariable(MCExecContext& ctxt, MCVarref *p_variable, bool p_data, MCMarkedText& r_mark);

void MCEngineEvalRandomUuid(MCExecContext& ctxt, MCStringRef& r_uuid);
void MCEngineEvalMD5Uuid(MCExecContext& ctxt, MCStringRef p_namespace_id, MCStringRef p_name, MCStringRef& r_uuid);
void MCEngineEvalSHA1Uuid(MCExecContext& ctxt, MCStringRef p_namespace_id, MCStringRef p_name, MCStringRef& r_uuid);

void MCEngineGetEditionType(MCExecContext& ctxt, MCStringRef& r_edition);

void MCEngineGetLoadedExtensions(MCExecContext& ctxt, MCProperListRef& r_extensions);

void MCEngineEvalIsStrictlyNothing(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsNotStrictlyNothing(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsStrictlyABoolean(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsNotStrictlyABoolean(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsStrictlyAnInteger(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsNotStrictlyAnInteger(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsStrictlyAReal(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsNotStrictlyAReal(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsStrictlyAString(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsNotStrictlyAString(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsStrictlyABinaryString(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsNotStrictlyABinaryString(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsStrictlyAnArray(MCExecContext& ctxt, MCValueRef value, bool& r_result);
void MCEngineEvalIsNotStrictlyAnArray(MCExecContext& ctxt, MCValueRef value, bool& r_result);

void MCEngineEvalCommandName(MCExecContext& ctxt, MCStringRef& r_result);
void MCEngineEvalCommandArguments(MCExecContext& ctxt, MCArrayRef& r_result);
void MCEngineEvalCommandArgumentAtIndex(MCExecContext& ctxt, uinteger_t t_index, MCStringRef& r_result);
void MCEngineGetRevLibraryMappingByKey(MCExecContext& ctxt, MCNameRef p_library, MCStringRef& r_mapping);
void MCEngineSetRevLibraryMappingByKey(MCExecContext& ctxt, MCNameRef p_library, MCStringRef p_mapping);

///////////

void MCFilesEvalFileItemsOfDirectory(MCExecContext& ctxt, MCStringRef p_directory, bool p_files, bool p_detailed, bool p_utf8, MCStringRef& r_string);
void MCFilesEvalDiskSpace(MCExecContext& ctxt, real64_t& r_result);
void MCFilesEvalDriverNames(MCExecContext& ctxt, MCStringRef& r_string);
void MCFilesEvalDrives(MCExecContext& ctxt, MCStringRef& r_string);
void MCFilesEvalOpenFiles(MCExecContext& ctxt, MCStringRef& r_string);
void MCFilesEvalTempName(MCExecContext& ctxt, MCStringRef& r_string);
void MCFilesEvalSpecialFolderPath(MCExecContext& ctxt, MCStringRef p_folder, MCStringRef& r_path);
void MCFilesEvalLongFilePath(MCExecContext& ctxt, MCStringRef p_path, MCStringRef& r_long_path);
void MCFilesEvalShortFilePath(MCExecContext& ctxt, MCStringRef p_path, MCStringRef& r_short_path);
void MCFilesEvalOpenProcesses(MCExecContext& ctxt, MCStringRef& r_string);
void MCFilesEvalOpenProcessesIds(MCExecContext& ctxt, MCStringRef& r_string);
void MCFilesEvalProcessId(MCExecContext& ctxt, integer_t& r_pid);
void MCFilesEvalDeleteRegistry(MCExecContext& ctxt, MCStringRef p_key, bool& r_deleted);
void MCFilesEvalListRegistry(MCExecContext& ctxt, MCStringRef p_key, MCStringRef& p_list);
void MCFilesEvalQueryRegistry(MCExecContext& ctxt, MCStringRef p_key, MCValueRef& r_value);
void MCFilesEvalQueryRegistryWithType(MCExecContext& ctxt, MCStringRef p_key, MCStringRef& r_type, MCValueRef& r_value);
void MCFilesEvalSetRegistry(MCExecContext& ctxt, MCStringRef p_key, MCValueRef p_value, bool& r_set);
void MCFilesEvalSetRegistryWithType(MCExecContext& ctxt, MCStringRef p_key, MCValueRef p_value, MCStringRef p_type, bool& r_set);
void MCFilesEvalCopyResourceWithNewId(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type, MCStringRef p_name, MCStringRef p_newid, MCStringRef& r_result);
void MCFilesEvalCopyResource(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_result);
void MCFilesEvalDeleteResource(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_result);
void MCFilesEvalGetResource(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_result);
void MCFilesEvalGetResourcesWithType(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_type, MCStringRef& r_string);
void MCFilesEvalGetResources(MCExecContext& ctxt, MCStringRef p_source, MCStringRef& r_string);
void MCFilesEvalSetResource(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_type, MCStringRef p_id, MCStringRef p_name, MCStringRef p_flags, MCStringRef p_value, MCStringRef& r_result);
void MCFilesEvalAliasReference(MCExecContext& ctxt, MCStringRef p_path, MCStringRef& r_reference);

void MCFilesEvalThereIsAFile(MCExecContext& ctxt, MCStringRef p_path, bool& r_result);
void MCFilesEvalThereIsNotAFile(MCExecContext& ctxt, MCStringRef p_path, bool& r_result);
void MCFilesEvalThereIsAFolder(MCExecContext& ctxt, MCStringRef p_path, bool& r_result);
void MCFilesEvalThereIsNotAFolder(MCExecContext& ctxt, MCStringRef p_path, bool& r_result);
void MCFilesEvalThereIsAProcess(MCExecContext& ctxt, MCStringRef p_path, bool& r_result);
void MCFilesEvalThereIsNotAProcess(MCExecContext& ctxt, MCStringRef p_path, bool& r_result);

void MCFilesEvalShell(MCExecContext& ctxt, MCStringRef command, MCStringRef& r_output);

void MCFilesExecDeleteFile(MCExecContext& ctxt, MCStringRef p_target);

void MCFilesExecCloseFile(MCExecContext& ctxt, MCNameRef p_filename);
void MCFilesExecCloseDriver(MCExecContext& ctxt, MCNameRef p_device);
void MCFilesExecCloseProcess(MCExecContext& ctxt, MCNameRef p_process);

void MCFilesExecLaunchUrl(MCExecContext& ctxt, MCStringRef p_url);
void MCFilesExecLaunchDocument(MCExecContext& ctxt, MCStringRef p_document);
void MCFilesExecLaunchApp(MCExecContext& ctxt, MCNameRef p_app, MCStringRef p_document);

void MCFilesExecOpenFile(MCExecContext& ctxt, MCNameRef p_filename, int p_mode, intenum_t p_encoding);
void MCFilesExecOpenDriver(MCExecContext& ctxt, MCNameRef p_device, int p_mode, intenum_t p_encoding);
void MCFilesExecOpenProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, intenum_t p_encoding);
void MCFilesExecOpenElevatedProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, intenum_t p_encoding);
//void MCFilesExecOpenFile(MCExecContext& ctxt, MCNameRef p_filename, int p_mode, bool p_text_mode);
//void MCFilesExecOpenDriver(MCExecContext& ctxt, MCNameRef p_device, int p_mode, bool p_text_mode);
void MCFilesExecOpenProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, bool p_text_mode);
void MCFilesExecOpenElevatedProcess(MCExecContext& ctxt, MCNameRef p_process, int p_mode, bool p_text_mode);

void MCFilesExecRename(MCExecContext& ctxt, MCStringRef from, MCStringRef to);

void MCFilesExecReadFromStdinFor(MCExecContext& ctxt, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units);
void MCFilesExecReadFromStdinUntil(MCExecContext& ctxt, MCStringRef p_sentinel, double p_max_wait, int p_time_units);
void MCFilesExecReadFromFileOrDriverFor(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units);
void MCFilesExecReadFromFileOrDriverUntil(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, MCStringRef p_sentinel, double p_max_wait, int p_time_units);
void MCFilesExecReadFromFileOrDriverAtFor(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units);
void MCFilesExecReadFromFileOrDriverAtUntil(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, MCStringRef p_sentinel, double p_max_wait, int p_time_units);
void MCFilesExecReadFromFileOrDriverAtEndFor(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units);
void MCFilesExecReadFromFileOrDriverAtEndUntil(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, int64_t p_at, MCStringRef p_sentinel, double p_max_wait, int p_time_units);
void MCFilesExecReadFromProcessFor(MCExecContext& ctxt, MCNameRef p_process, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units);
void MCFilesExecReadFromProcessUntil(MCExecContext& ctxt, MCNameRef p_process, MCStringRef p_sentinel, double p_max_wait, int p_time_units);

void MCFilesExecWriteToStdout(MCExecContext& ctxt, MCStringRef p_data, int p_unit_type);
void MCFilesExecWriteToStderr(MCExecContext& ctxt, MCStringRef p_data, int p_unit_type);
void MCFilesExecWriteToFileOrDriver(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type);
void MCFilesExecWriteToFileOrDriverAt(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type, int64_t p_at);
void MCFilesExecWriteToFileOrDriverAtEnd(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type);
void MCFilesExecWriteToProcess(MCExecContext& ctxt, MCNameRef p_process, MCStringRef p_data, int p_unit_type);

void MCFilesExecSeekToEofInFile(MCExecContext& ctxt, MCNameRef p_file);
void MCFilesExecSeekAbsoluteInFile(MCExecContext& ctxt, int64_t p_to, MCNameRef p_file);
void MCFilesExecSeekRelativeInFile(MCExecContext& ctxt, int64_t p_by, MCNameRef p_file);

void MCFilesExecReadFromFileOrDriverAtEndForLegacy(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, intenum_t p_eof, uint4 p_count, int p_unit_type, double p_max_wait, int p_time_units);
void MCFilesExecReadFromFileOrDriverAtEndUntilLegacy(MCExecContext& ctxt, bool p_driver, MCNameRef p_file, intenum_t p_eof, MCStringRef p_sentinel, double p_max_wait, int p_time_units);
void MCFilesExecSeekToEofInFileLegacy(MCExecContext& ctxt, intenum_t p_eof);
void MCFilesExecWriteToFileOrDriverAtEndLegacy(MCExecContext& ctxt, MCNameRef p_file, MCStringRef p_data, int p_unit_type, intenum_t p_eof);

void MCFilesExecCreateFolder(MCExecContext& ctxt, MCStringRef p_filename);
void MCFilesExecCreateAlias(MCExecContext& ctxt, MCStringRef p_target_filename, MCStringRef p_alias_filename);

void MCFilesExecKillProcess(MCExecContext& ctxt, MCStringRef p_process, MCStringRef p_signal);

void MCFilesGetUMask(MCExecContext& ctxt, uinteger_t& r_value);
void MCFilesSetUMask(MCExecContext& ctxt, uinteger_t p_value);
void MCFilesGetFileType(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesSetFileType(MCExecContext& ctxt, MCStringRef p_value);

void MCFilesGetSerialControlString(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesSetSerialControlString(MCExecContext& ctxt, MCStringRef p_value);
void MCFilesGetHideConsoleWindows(MCExecContext& ctxt, bool& r_value);
void MCFilesSetHideConsoleWindows(MCExecContext& ctxt, bool p_value);

void MCFilesGetShellCommand(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesSetShellCommand(MCExecContext& ctxt, MCStringRef p_value);
void MCFilesGetCurrentFolder(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesSetCurrentFolder(MCExecContext& ctxt, MCStringRef p_value);
void MCFilesGetEngineFolder(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesGetHomeFolder(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesGetDocumentsFolder(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesGetDesktopFolder(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesGetTemporaryFolder(MCExecContext& ctxt, MCStringRef& r_value);

void MCFilesGetFiles(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesGetDetailedFiles(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesGetFolders(MCExecContext& ctxt, MCStringRef& r_value);
void MCFilesGetDetailedFolders(MCExecContext& ctxt, MCStringRef& r_value);

///////////

struct MCMultimediaRecordFormat;
extern MCExecCustomTypeInfo *kMCMultimediaRecordFormatTypeInfo;

void MCMultimediaExecAnswerEffect(MCExecContext &ctxt);
void MCMultimediaExecAnswerRecord(MCExecContext &ctxt);

void MCMultimediaEvalQTVersion(MCExecContext& ctxt, MCStringRef& r_string);
void MCMultimediaEvalQTEffects(MCExecContext& ctxt, MCStringRef& r_result);
void MCMultimediaEvalRecordCompressionTypes(MCExecContext& ctxt, MCStringRef& r_string);
void MCMultimediaEvalRecordFormats(MCExecContext& ctxt, MCStringRef& r_string);
void MCMultimediaEvalRecordLoudness(MCExecContext& ctxt, integer_t& r_loudness);
void MCMultimediaEvalMovie(MCExecContext& ctxt, MCStringRef& r_string);
void MCMultimediaEvalMCISendString(MCExecContext& ctxt, MCStringRef p_command, MCStringRef& r_result);
void MCMultimediaEvalSound(MCExecContext& ctxt, MCStringRef& r_sound);

void MCMultimediaExecRecord(MCExecContext& ctxt, MCStringRef p_filename);
void MCMultimediaExecRecordPause(MCExecContext& ctxt);
void MCMultimediaExecRecordResume(MCExecContext& ctxt);

void MCMultimediaExecStartPlayer(MCExecContext& ctxt, MCPlayer *p_target);

void MCMultimediaExecStopPlaying(MCExecContext& ctxt);
void MCMultimediaExecStopPlayingObject(MCExecContext& ctxt, MCObject *p_object);
void MCMultimediaExecStopRecording(MCExecContext& ctxt);

void MCMultimediaExecPrepareVideoClip(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_clip, bool p_looping, MCPoint *p_at, MCStringRef p_options);
void MCMultimediaExecPlayAudioClip(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_clip, bool p_looping);
void MCMultimediaExecPlayVideoClip(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_clip, bool p_looping, MCPoint *p_at, MCStringRef p_options);
void MCMultimediaExecPlayStopAudio(MCExecContext& ctxt);
void MCMultimediaExecPlayPlayerOperation(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_player, int p_player_chunk_type, int p_operation);
void MCMultimediaExecPlayVideoOperation(MCExecContext& ctxt, MCStack *p_target, int p_chunk_type, MCStringRef p_clip, int p_operation);
void MCMultimediaExecPlayLastVideoOperation(MCExecContext& ctxt, int p_operation);

void MCMultimediaGetRecordFormat(MCExecContext& ctxt, MCMultimediaRecordFormat& r_value);
void MCMultimediaSetRecordFormat(MCExecContext& ctxt, const MCMultimediaRecordFormat& p_value);
void MCMultimediaGetRecordCompression(MCExecContext& ctxt, MCStringRef& r_value);
void MCMultimediaSetRecordCompression(MCExecContext& ctxt, MCStringRef p_value);
void MCMultimediaGetRecordInput(MCExecContext& ctxt, MCStringRef& r_value);
void MCMultimediaSetRecordInput(MCExecContext& ctxt, MCStringRef p_value);
void MCMultimediaGetRecordSampleSize(MCExecContext& ctxt, uinteger_t& r_value);
void MCMultimediaSetRecordSampleSize(MCExecContext& ctxt, uinteger_t p_value);
void MCMultimediaGetRecordChannels(MCExecContext& ctxt, uinteger_t& r_value);
void MCMultimediaSetRecordChannels(MCExecContext& ctxt, uinteger_t p_value);
void MCMultimediaGetRecordRate(MCExecContext& ctxt, double& r_value);
void MCMultimediaSetRecordRate(MCExecContext& ctxt, double p_value);

void MCMultimediaGetPlayDestination(MCExecContext& ctxt, intenum_t& r_dest);
void MCMultimediaSetPlayDestination(MCExecContext& ctxt, intenum_t dest);
void MCMultimediaGetPlayLoudness(MCExecContext& ctxt, uinteger_t& r_loudness);
void MCMultimediaSetPlayLoudness(MCExecContext& ctxt, uinteger_t loudness);

void MCMultimediaGetQtIdleRate(MCExecContext& ctxt, uinteger_t& r_value);
void MCMultimediaSetQtIdleRate(MCExecContext& ctxt, uinteger_t p_value);
void MCMultimediaGetDontUseQt(MCExecContext& ctxt, bool& r_value);
void MCMultimediaSetDontUseQt(MCExecContext& ctxt, bool p_value);
void MCMultimediaGetDontUseQtEffects(MCExecContext& ctxt, bool& r_value);
void MCMultimediaSetDontUseQtEffects(MCExecContext& ctxt, bool p_value);

void MCMultimediaGetRecording(MCExecContext& ctxt, bool& r_value);
void MCMultimediaSetRecording(MCExecContext& ctxt, bool p_value);

///////////

void MCTextEvalFontNames(MCExecContext& ctxt, MCStringRef p_type, MCStringRef& r_names);
void MCTextEvalFontLanguage(MCExecContext& ctxt, MCStringRef p_font, MCNameRef& r_lang);
void MCTextEvalFontSizes(MCExecContext& ctxt, MCStringRef p_font, MCStringRef& r_sizes);
void MCTextEvalFontStyles(MCExecContext& ctxt, MCStringRef p_font, integer_t p_size, MCStringRef& r_styles);
void MCTextEvalMeasureText(MCExecContext& ctxt, MCObject *p_obj, MCStringRef p_text, MCStringRef p_mode, bool p_unicode, MCStringRef& r_result);
void MCTextGetFontfilesInUse(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_list);
void MCTextExecStartUsingFont(MCExecContext& ctxt, MCStringRef p_path, bool p_is_globally);
void MCTextExecStopUsingFont(MCExecContext& ctxt, MCStringRef p_path);

///////////

void MCNetworkEvalDNSServers(MCExecContext& ctxt, MCStringRef& r_servers);
void MCNetworkEvalCachedUrls(MCExecContext& ctxt, MCStringRef& r_string);
void MCNetworkEvalUrlStatus(MCExecContext& ctxt, MCStringRef p_url, MCStringRef& r_status);
void MCNetworkEvalHostAddress(MCExecContext& ctxt, MCNameRef p_socket, MCStringRef& r_address);
void MCNetworkEvalPeerAddress(MCExecContext& ctxt, MCNameRef p_socket, MCStringRef& r_address);
void MCNetworkEvalHostAddressToName(MCExecContext& ctxt, MCStringRef p_address, MCStringRef &r_name);
void MCNetworkEvalHostNameToAddress(MCExecContext& ctxt, MCStringRef p_hostname, MCNameRef p_message, MCStringRef& r_string);
void MCNetworkEvalHostName(MCExecContext& ctxt, MCStringRef& r_string);
void MCNetworkEvalOpenSockets(MCExecContext& ctxt, MCStringRef& r_string);

void MCNetworkEvalHTTPProxyForURL(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_host, MCStringRef& r_proxy);
void MCNetworkEvalHTTPProxyForURLWithPAC(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_host, MCStringRef p_pac, MCStringRef& r_proxy);

void MCNetworkExecCloseSocket(MCExecContext& ctxt, MCNameRef p_socket);

void MCNetworkExecDeleteUrl(MCExecContext& ctxt, MCStringRef p_target);

void MCNetworkExecLoadUrl(MCExecContext& ctxt, MCStringRef p_url, MCNameRef p_message);
void MCNetworkExecUnloadUrl(MCExecContext& ctxt, MCStringRef p_url);

void MCNetworkExecOpenSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_from_address, MCNameRef p_message, MCNameRef p_end_hostname);
void MCNetworkExecOpenSecureSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_from_address, MCNameRef p_message, MCNameRef p_end_hostname, bool p_with_verification);
void MCNetworkExecOpenDatagramSocket(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_from_address, MCNameRef p_message, MCNameRef p_end_hostname);

void MCNetworkExecPostToUrl(MCExecContext& ctxt, MCValueRef p_data, MCStringRef p_url);

void MCNetworkExecAcceptConnectionsOnPort(MCExecContext& ctxt, uint2 p_port, MCNameRef p_message);
void MCNetworkExecAcceptDatagramConnectionsOnPort(MCExecContext& ctxt, uint2 p_port, MCNameRef p_message);
void MCNetworkExecAcceptSecureConnectionsOnPort(MCExecContext& ctxt, uint2 p_port, MCNameRef p_message, bool p_with_verification);

void MCNetworkExecReadFromSocketFor(MCExecContext& ctxt, MCNameRef p_socket, uint4 p_count, int p_unit_type, MCNameRef p_message);
void MCNetworkExecReadFromSocketUntil(MCExecContext& ctxt, MCNameRef p_socket, MCStringRef p_sentinel, MCNameRef p_message);

void MCNetworkExecWriteToSocket(MCExecContext& ctxt, MCNameRef p_socket, MCStringRef p_data, MCNameRef p_message);

void MCNetworkExecPutIntoUrl(MCExecContext& ctxt, MCValueRef value, int prep, MCUrlChunkPtr url);

void MCNetworkExecReturnValueAndUrlResult(MCExecContext& ctxt, MCValueRef value, MCValueRef url_result);

void MCNetworkGetUrlResponse(MCExecContext& ctxt, MCStringRef& r_value);

void MCNetworkGetFtpProxy(MCExecContext& ctxt, MCStringRef& r_value);
void MCNetworkSetFtpProxy(MCExecContext& ctxt, MCStringRef p_value);
void MCNetworkGetHttpProxy(MCExecContext& ctxt, MCStringRef& r_value);
void MCNetworkSetHttpProxy(MCExecContext& ctxt, MCStringRef p_value);
void MCNetworkGetHttpHeaders(MCExecContext& ctxt, MCStringRef& r_value);
void MCNetworkSetHttpHeaders(MCExecContext& ctxt, MCStringRef p_value);
void MCNetworkGetSocketTimeout(MCExecContext& ctxt, double& r_value);
void MCNetworkSetSocketTimeout(MCExecContext& ctxt, double p_value);

void MCNetworkGetDefaultNetworkInterface(MCExecContext& ctxt, MCStringRef& r_value);
void MCNetworkSetDefaultNetworkInterface(MCExecContext& ctxt, MCStringRef p_value);
void MCNetworkGetNetworkInterfaces(MCExecContext& ctxt, MCStringRef& r_value);

void MCNetworkGetAllowDatagramBroadcasts(MCExecContext& ctxt, bool& r_value);
void MCNetworkSetAllowDatagramBroadcasts(MCExecContext& ctxt, bool p_value);

void MCNetworkExecSetUrl(MCExecContext& ctxt, MCValueRef p_value, MCStringRef p_url);
void MCNetworkExecPutIntoUrl(MCExecContext& ctxt, MCStringRef p_value, int p_where, MCStringRef p_url);

void MCNetworkMarkUrl(MCExecContext& ctxt, MCStringRef p_url, MCMarkedText& r_mark);

///////////

void MCDateTimeEvalMilliseconds(MCExecContext& ctxt, real64_t& r_real);
void MCDateTimeEvalSeconds(MCExecContext& ctxt, real64_t& r_seconds);
void MCDateTimeEvalTicks(MCExecContext& ctxt, real64_t& r_ticks);

void MCDateTimeEvalDate(MCExecContext& ctxt, MCStringRef& r_string);
void MCDateTimeEvalTime(MCExecContext& ctxt, MCStringRef& r_string);

void MCDateTimeEvalDateFormat(MCExecContext& ctxt, MCStringRef& r_string);
void MCDateTimeEvalMonthNames(MCExecContext& ctxt, MCStringRef& r_string);
void MCDateTimeEvalWeekDayNames(MCExecContext& ctxt, MCStringRef& r_string);

void MCDateTimeEvalIsADate(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCDateTimeEvalIsNotADate(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);

void MCDateTimeExecConvert(MCExecContext &ctxt, MCStringRef p_input, int p_from_first, int p_from_second, int p_to_first, int p_to_second, MCStringRef &r_output);
void MCDateTimeExecConvertIntoIt(MCExecContext &ctxt, MCStringRef p_input, int p_from_first, int p_from_second, int p_to_first, int p_to_second);

void MCDateTimeGetTwelveTime(MCExecContext &ctxt, bool& r_value);
void MCDateTimeSetTwelveTime(MCExecContext &ctxt, bool p_value);

void MCDateTimeGetDate(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value);
void MCDateTimeGetTime(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value);
void MCDateTimeGetMilliseconds(MCExecContext &ctxt, double& r_value);
void MCDateTimeGetLongMilliseconds(MCExecContext &ctxt, double& r_value);
void MCDateTimeGetSeconds(MCExecContext &ctxt, double& r_value);
void MCDateTimeGetLongSeconds(MCExecContext &ctxt, double& r_value);
void MCDateTimeGetTicks(MCExecContext &ctxt, double& r_value);
void MCDateTimeGetLongTicks(MCExecContext &ctxt, double& r_value);

void MCDateTimeGetMonthNames(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value);
void MCDateTimeGetWeekDayNames(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value);
void MCDateTimeGetDateFormat(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value);

///////////

void MCScriptingEvalAlternateLanguages(MCExecContext& ctxt, MCStringRef& r_list);
void MCScriptingExecDoAsAlternateLanguage(MCExecContext& ctxt, MCStringRef p_script, MCStringRef p_language);

void MCScriptingExecSendToProgram(MCExecContext& ctxt, MCStringRef message, MCStringRef program, MCStringRef event_type, bool wait_for_reply);
void MCScriptingExecReplyError(MCExecContext& ctxt, MCStringRef message);
void MCScriptingExecReply(MCExecContext& ctxt, MCStringRef message, MCStringRef keyword);
void MCScriptingExecRequestAppleEvent(MCExecContext& ctxt, int type, MCStringRef program);
void MCScriptingExecRequestFromProgram(MCExecContext& ctxt, MCStringRef message, MCStringRef program);

///////////

void MCSecurityEvalEncrypt(MCExecContext& ctxt, MCStringRef p_source, MCStringRef& r_dest);
void MCSecurityEvalCipherNames(MCExecContext& ctxt, MCStringRef& r_names);
void MCSecurityEvalRandomBytes(MCExecContext& ctxt, uinteger_t p_byte_count, MCDataRef& r_bytes);

void MCSecurityExecRsaEncrypt(MCExecContext& ctxt, MCStringRef p_data, bool p_is_public, MCStringRef p_key, MCStringRef p_passphrase);
void MCSecurityExecRsaDecrypt(MCExecContext& ctxt, MCStringRef p_data, bool p_is_public, MCStringRef p_key, MCStringRef p_passphrase);
void MCSecurityExecBlockEncryptWithPassword(MCExecContext& ctxt, MCStringRef p_data, MCNameRef p_cipher, MCStringRef p_password, MCStringRef p_salt, MCStringRef p_iv, uint2 p_bit_rate);
void MCSecurityExecBlockEncryptWithKey(MCExecContext& ctxt, MCStringRef p_data, MCNameRef p_cipher, MCStringRef p_key, MCStringRef p_iv, uint2 p_bit_rate);
void MCSecurityExecBlockDecryptWithPassword(MCExecContext& ctxt, MCStringRef p_data, MCNameRef p_cipher, MCStringRef p_password, MCStringRef p_salt, MCStringRef p_iv, uint2 p_bit_rate);
void MCSecurityExecBlockDecryptWithKey(MCExecContext& ctxt, MCStringRef p_data, MCNameRef p_cipher, MCStringRef p_key, MCStringRef p_iv, uint2 p_bit_rate);

void MCSecurityGetSslCertificates(MCExecContext& ctxt, MCStringRef& r_value);
void MCSecuritySetSslCertificates(MCExecContext& ctxt, MCStringRef p_value);

void MCSecurityExecSecureSocket(MCExecContext& ctxt, MCNameRef p_socket, bool p_secure_verify, MCNameRef p_end_hostname);

///////////

void MCGraphicsEvalIsAColor(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCGraphicsEvalIsNotAColor(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCGraphicsEvalIsAPoint(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCGraphicsEvalIsNotAPoint(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCGraphicsEvalIsARectangle(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);
void MCGraphicsEvalIsNotARectangle(MCExecContext& ctxt, MCValueRef p_value, bool& r_result);

void MCGraphicsEvalIsWithin(MCExecContext& ctxt, MCPoint point, MCRectangle rectangle, bool& r_result);
void MCGraphicsEvalIsNotWithin(MCExecContext& ctxt, MCPoint point, MCRectangle rectangle, bool& r_result);

void MCGraphicsExecFlipSelection(MCExecContext& ctxt, bool horizontal);
void MCGraphicsExecFlipImage(MCExecContext& ctxt, MCImage *image, bool horizontal);

void MCGraphicsExecResetPaint(MCExecContext& ctxt);

void MCGraphicsExecCropImage(MCExecContext& ctxt, MCImage *image, MCRectangle bounds);

void MCGraphicsExecRotateSelection(MCExecContext& ctxt, integer_t angle);
void MCGraphicsExecRotateImage(MCExecContext& ctxt, MCImage *image, integer_t angle);

void MCGraphicsExecPrepareImage(MCExecContext& ctxt, MCImage *image);
void MCGraphicsExecPrepareImageFile(MCExecContext& ctxt, MCStringRef filename);

void MCGraphicsGetImageCacheLimit(MCExecContext &ctxt, uinteger_t &r_value);
void MCGraphicsSetImageCacheLimit(MCExecContext &ctxt, uinteger_t p_value);
void MCGraphicsGetImageCacheUsage(MCExecContext &ctxt, uinteger_t &r_value);

///////////

void MCLegacyEvalHasMemory(MCExecContext& ctxt, uinteger_t p_bytes, bool& r_bool);
void MCLegacyEvalHeapSpace(MCExecContext& ctxt, integer_t& r_bytes);
void MCLegacyEvalStackSpace(MCExecContext& ctxt, integer_t& r_bytes);
void MCLegacyEvalIsNumber(MCExecContext& ctxt, MCStringRef p_string, bool& r_bool);
void MCLegacyEvalLicensed(MCExecContext& ctxt, bool& r_licensed);
void MCLegacyEvalMenus(MCExecContext& ctxt, MCStringRef& r_string);
void MCLegacyEvalScreenType(MCExecContext& ctxt, MCNameRef& r_name);
void MCLegacyEvalScreenVendor(MCExecContext& ctxt, MCNameRef& r_name);
void MCLegacyEvalSelectedButtonOf(MCExecContext& ctxt, bool p_background, integer_t p_family, MCObjectPtr p_object, MCStringRef& r_string);
void MCLegacyEvalSelectedButton(MCExecContext& ctxt, bool p_background, integer_t p_family, MCStringRef& r_string);
void MCLegacyEvalTextHeightSum(MCExecContext& ctxt, MCObjectPtr p_object, integer_t& r_sum);
void MCLegacyEvalMenuObject(MCExecContext& ctxt, MCStringRef& r_object);

void MCLegacyExecDoInBrowser(MCExecContext& ctxt, MCStringRef p_script);
void MCLegacyExecCompactStack(MCExecContext& ctxt, MCStack *stack);

void MCLegacyExecDoMenu(MCExecContext& ctxt, MCStringRef p_option);

void MCLegacyExecLockColormap(MCExecContext& ctxt);
void MCLegacyExecUnlockColormap(MCExecContext& ctxt);

void MCLegacyExecImportEps(MCExecContext& ctxt, MCStringRef p_filename);
void MCLegacyExecImportHypercardStack(MCExecContext& ctxt, MCStringRef p_filename);

void MCLegacyGetRevRuntimeBehaviour(MCExecContext& ctxt, uinteger_t &r_value);
void MCLegacySetRevRuntimeBehaviour(MCExecContext& ctxt, uint4 p_value);
void MCLegacyGetHcImportStat(MCExecContext& ctxt, MCStringRef& r_value);
void MCLegacySetHcImportStat(MCExecContext& ctxt, MCStringRef p_value);
void MCLegacyGetScriptTextFont(MCExecContext& ctxt, MCStringRef& r_value);
void MCLegacySetScriptTextFont(MCExecContext& ctxt, MCStringRef p_value);
void MCLegacyGetScriptTextSize(MCExecContext& ctxt, uinteger_t &r_value);
void MCLegacySetScriptTextSize(MCExecContext& ctxt, uinteger_t p_value);

void MCLegacyGetStackFiles(MCExecContext& ctxt, MCStringRef& r_value);
void MCLegacySetStackFiles(MCExecContext& ctxt, MCStringRef value);

void MCLegacyGetMenuBar(MCExecContext& ctxt, MCStringRef& r_value);
// SN-2014-09-01: [[ Bug 13300 ]] Updated 'set the menubar' to have a (useless) setter at the global scope
void MCLegacySetMenuBar(MCExecContext& ctxt, MCStringRef p_value);

void MCLegacyGetEditMenus(MCExecContext& ctxt, bool& r_value);
void MCLegacySetEditMenus(MCExecContext& ctxt, bool value);

void MCLegacyGetTextAlign(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetTextAlign(MCExecContext& ctxt, MCValueRef value);
void MCLegacyGetTextFont(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetTextFont(MCExecContext& ctxt, MCValueRef value);
void MCLegacyGetTextHeight(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetTextHeight(MCExecContext& ctxt, MCValueRef value);
void MCLegacyGetTextSize(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetTextSize(MCExecContext& ctxt, MCValueRef value);
void MCLegacyGetTextStyle(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetTextStyle(MCExecContext& ctxt, MCValueRef value);

void MCLegacyGetBufferMode(MCExecContext& ctxt, MCStringRef& r_value);
void MCLegacySetBufferMode(MCExecContext& ctxt, MCStringRef p_value);
void MCLegacyGetMultiEffect(MCExecContext& ctxt, bool& r_value);
void MCLegacySetMultiEffect(MCExecContext& ctxt, bool p_value);

void MCLegacyGetPrintTextAlign(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetPrintTextAlign(MCExecContext& ctxt, MCValueRef value);
void MCLegacyGetPrintTextFont(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetPrintTextFont(MCExecContext& ctxt, MCValueRef value);
void MCLegacyGetPrintTextHeight(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetPrintTextHeight(MCExecContext& ctxt, MCValueRef value);
void MCLegacyGetPrintTextSize(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetPrintTextSize(MCExecContext& ctxt, MCValueRef value);
void MCLegacyGetPrintTextStyle(MCExecContext& ctxt, MCValueRef& r_value);
void MCLegacySetPrintTextStyle(MCExecContext& ctxt, MCValueRef value);

void MCLegacyGetEditScripts(MCExecContext& ctxt, bool& r_value);
void MCLegacySetEditScripts(MCExecContext& ctxt, bool p_value);
void MCLegacyGetColorWorld(MCExecContext& ctxt, bool& r_value);
void MCLegacySetColorWorld(MCExecContext& ctxt, bool p_value);
void MCLegacyGetAllowKeyInField(MCExecContext& ctxt, bool& r_value);
void MCLegacySetAllowKeyInField(MCExecContext& ctxt, bool p_value);
void MCLegacyGetAllowFieldRedraw(MCExecContext& ctxt, bool& r_value);
void MCLegacySetAllowFieldRedraw(MCExecContext& ctxt, bool p_value);
void MCLegacyGetRemapColor(MCExecContext& ctxt, bool& r_value);
void MCLegacySetRemapColor(MCExecContext& ctxt, bool p_value);

void MCLegacyGetUserLevel(MCExecContext& ctxt, uinteger_t& r_value);
void MCLegacySetUserLevel(MCExecContext& ctxt, uinteger_t p_value);
void MCLegacyGetUserModify(MCExecContext& ctxt, bool& r_value);
void MCLegacySetUserModify(MCExecContext& ctxt, bool p_value);

void MCLegacyGetLockColormap(MCExecContext& ctxt, bool& r_value);
void MCLegacySetLockColormap(MCExecContext& ctxt, bool p_value);
void MCLegacyGetPrivateColors(MCExecContext& ctxt, bool& r_value);
void MCLegacySetPrivateColors(MCExecContext& ctxt, bool p_value);

void MCLegacyGetLongWindowTitles(MCExecContext& ctxt, bool& r_value);
void MCLegacySetLongWindowTitles(MCExecContext& ctxt, bool p_value);
void MCLegacyGetBlindTyping(MCExecContext& ctxt, bool& r_value);
void MCLegacySetBlindTyping(MCExecContext& ctxt, bool p_value);
void MCLegacyGetPowerKeys(MCExecContext& ctxt, bool& r_value);
void MCLegacySetPowerKeys(MCExecContext& ctxt, bool p_value);
void MCLegacyGetTextArrows(MCExecContext& ctxt, bool& r_value);
void MCLegacySetTextArrows(MCExecContext& ctxt, bool p_value);
void MCLegacyGetColormap(MCExecContext& ctxt, MCStringRef& r_value);
void MCLegacySetColormap(MCExecContext& ctxt, MCStringRef p_value);
void MCLegacyGetNoPixmaps(MCExecContext& ctxt, bool& r_value);
void MCLegacySetNoPixmaps(MCExecContext& ctxt, bool p_value);
void MCLegacyGetLowResolutionTimers(MCExecContext& ctxt, bool& r_value);
void MCLegacySetLowResolutionTimers(MCExecContext& ctxt, bool p_value);

void MCLegacyGetVcSharedMemory(MCExecContext& ctxt, bool& r_value);
void MCLegacySetVcSharedMemory(MCExecContext& ctxt, bool p_value);
void MCLegacyGetVcPlayer(MCExecContext& ctxt, MCStringRef& r_value);
void MCLegacySetVcPlayer(MCExecContext& ctxt, MCStringRef p_value);
void MCLegacyGetSoundChannel(MCExecContext& ctxt, uinteger_t& r_value);
void MCLegacySetSoundChannel(MCExecContext& ctxt, uinteger_t p_value);
void MCLegacyGetLzwKey(MCExecContext& ctxt, MCStringRef& r_value);
void MCLegacySetLzwKey(MCExecContext& ctxt, MCStringRef p_value);

void MCLegacyGetMultiple(MCExecContext& ctxt, bool& r_value);
void MCLegacySetMultiple(MCExecContext& ctxt, bool p_value);
void MCLegacyGetMultiSpace(MCExecContext& ctxt, uinteger_t& r_value);
void MCLegacySetMultiSpace(MCExecContext& ctxt, uinteger_t p_value);

///////////

void MCIdeExecEditScriptOfObject(MCExecContext& ctxt, MCObject *p_object, MCStringRef p_at);
void MCIdeExecHideMessageBox(MCExecContext& ctxt);
void MCIdeExecShowMessageBox(MCExecContext& ctxt);

///////////

struct MCPrintingPrintDeviceOutput;
struct MCPrintingPrinterPageRange;

extern MCExecSetTypeInfo *kMCPrintingPrinterFeaturesTypeInfo;
extern MCExecEnumTypeInfo *kMCPrintingPrinterOrientationTypeInfo;
extern MCExecCustomTypeInfo *kMCPrintingPrintDeviceOutputTypeInfo;
extern MCExecCustomTypeInfo *kMCPrintingPrinterPageRangeTypeInfo;
extern MCExecEnumTypeInfo *kMCPrintingPrinterLinkTypeInfo;
extern MCExecEnumTypeInfo *kMCPrintingPrinterBookmarkInitialStateTypeInfo;
extern MCExecEnumTypeInfo *kMCPrintingPrintJobDuplexTypeInfo;

void MCPrintingExecAnswerPageSetup(MCExecContext &ctxt, bool p_is_sheet);
void MCPrintingExecAnswerPrinter(MCExecContext &ctxt, bool p_is_sheet);

void MCPrintingExecCancelPrinting(MCExecContext& ctxt);
void MCPrintingExecResetPrinting(MCExecContext& ctxt);
void MCPrintingExecPrintAnchor(MCExecContext& ctxt, MCStringRef name, MCPoint location);
void MCPrintingExecPrintLink(MCExecContext& ctxt, int type, MCStringRef target, MCRectangle area);
void MCPrintingExecPrintBookmark(MCExecContext& ctxt, MCStringRef title, MCPoint location, integer_t level, bool initially_closed);
void MCPrintingExecPrintUnicodeBookmark(MCExecContext& ctxt, MCDataRef title, MCPoint location, integer_t level, bool initially_closed);
void MCPrintingExecPrintBreak(MCExecContext& ctxt);
void MCPrintingExecPrintAllCards(MCExecContext& ctxt, MCStack *stack, bool only_marked);
void MCPrintingExecPrintRectOfAllCards(MCExecContext& ctxt, MCStack *stack, bool p_only_marked, MCPoint from, MCPoint to);
void MCPrintingExecPrintCard(MCExecContext& ctxt, MCCard *target);
void MCPrintingExecPrintRectOfCard(MCExecContext& ctxt, MCCard *target, MCPoint from, MCPoint to);
void MCPrintingExecPrintSomeCards(MCExecContext& ctxt, integer_t count);
void MCPrintingExecPrintRectOfSomeCards(MCExecContext& ctxt, integer_t count, MCPoint from, MCPoint to);
void MCPrintingExecPrintCardIntoRect(MCExecContext& ctxt, MCCard *card, MCRectangle destination);
void MCPrintingExecPrintRectOfCardIntoRect(MCExecContext& ctxt, MCCard *card, MCPoint from, MCPoint to, MCRectangle destination);

void MCPrintingExecClosePrinting(MCExecContext& ctxt);

void MCPrintingExecOpenPrintingToDestination(MCExecContext& ctxt, MCStringRef p_destination, MCStringRef p_filename, MCArrayRef p_options);
void MCPrintingExecOpenPrinting(MCExecContext& ctxt);
void MCPrintingExecOpenPrintingWithDialog(MCExecContext& ctxt, bool p_as_sheet);

void MCPrintingGetPrinterNames(MCExecContext& ctxt, MCStringRef& r_printers);

void MCPrintingGetPrintDeviceFeatures(MCExecContext& ctxt, unsigned int& r_features);
void MCPrintingSetPrintDeviceOutput(MCExecContext& ctxt, const MCPrintingPrintDeviceOutput& output);
void MCPrintingGetPrintDeviceOutput(MCExecContext& ctxt, MCPrintingPrintDeviceOutput& r_output);
void MCPrintingGetPrintDeviceRectangle(MCExecContext& ctxt, MCRectangle &r_rectangle);
void MCPrintingGetPrintDeviceRectangle(MCExecContext& ctxt, MCRectangle &r_rectangle);
void MCPrintingGetPrintDeviceSettings(MCExecContext& ctxt, MCDataRef &r_settings);
void MCPrintingSetPrintDeviceSettings(MCExecContext& ctxt, MCDataRef p_settings);
void MCPrintingGetPrintDeviceName(MCExecContext& ctxt, MCStringRef &r_name);
void MCPrintingSetPrintDeviceName(MCExecContext& ctxt, MCStringRef p_name);

void MCPrintingGetPrintPageOrientation(MCExecContext& ctxt, int& r_orientation);
void MCPrintingSetPrintPageOrientation(MCExecContext& ctxt, int orientation);

void MCPrintingSetPrintJobRanges(MCExecContext& ctxt, const MCPrintingPrinterPageRange& p_ranges);
void MCPrintingGetPrintJobRanges(MCExecContext& ctxt, MCPrintingPrinterPageRange& r_ranges);

void MCPrintingSetPrintPageSize(MCExecContext& ctxt, integer_t p_value[2]);
void MCPrintingGetPrintPageSize(MCExecContext& ctxt, integer_t r_value[2]);
void MCPrintingSetPrintPageScale(MCExecContext& ctxt, double p_value);
void MCPrintingGetPrintPageScale(MCExecContext& ctxt, double &r_value);
void MCPrintingGetPrintPageRectangle(MCExecContext& ctxt, MCRectangle &r_value);

void MCPrintingGetPrintJobName(MCExecContext& ctxt, MCStringRef &r_value);
void MCPrintingSetPrintJobName(MCExecContext& ctxt, MCStringRef p_value);
void MCPrintingGetPrintJobCopies(MCExecContext& ctxt, integer_t &r_value);
void MCPrintingSetPrintJobCopies(MCExecContext& ctxt, integer_t p_value);
void MCPrintingGetPrintJobDuplex(MCExecContext& ctxt, intenum_t &r_value);
void MCPrintingSetPrintJobDuplex(MCExecContext& ctxt, intenum_t p_value);
void MCPrintingGetPrintJobCollate(MCExecContext& ctxt, bool &r_value);
void MCPrintingSetPrintJobCollate(MCExecContext& ctxt, bool p_value);
void MCPrintingGetPrintJobColor(MCExecContext& ctxt, bool &r_value);
void MCPrintingSetPrintJobColor(MCExecContext& ctxt, bool p_value);
// SN-2014-09-17: [[ Bug 13467 ]] PrintPageNumber may return empty
void MCPrintingGetPrintJobPage(MCExecContext& ctxt, integer_t *&r_value);

void MCPrintingGetPrintCardBorders(MCExecContext& ctxt, bool &r_card_borders);
void MCPrintingSetPrintCardBorders(MCExecContext& ctxt, bool p_card_borders);
void MCPrintingGetPrintGutters(MCExecContext& ctxt, integer_t r_gutters[2]);
void MCPrintingSetPrintGutters(MCExecContext& ctxt, integer_t p_gutters[2]);
void MCPrintingGetPrintMargins(MCExecContext& ctxt, integer_t r_margins[4]);
void MCPrintingSetPrintMargins(MCExecContext& ctxt, integer_t p_margins[4]);
void MCPrintingGetPrintRowsFirst(MCExecContext& ctxt, bool &r_rows_first);
void MCPrintingSetPrintRowsFirst(MCExecContext& ctxt, bool p_rows_first);
void MCPrintingGetPrintScale(MCExecContext& ctxt, double &r_scale);
void MCPrintingSetPrintScale(MCExecContext& ctxt, double p_scale);
void MCPrintingGetPrintRotated(MCExecContext& ctxt, bool &r_rotated);
void MCPrintingSetPrintRotated(MCExecContext& ctxt, bool p_rotated);
void MCPrintingGetPrintCommand(MCExecContext& ctxt, MCStringRef &r_command);
void MCPrintingSetPrintCommand(MCExecContext& ctxt, MCStringRef p_command);
void MCPrintingGetPrintFontTable(MCExecContext& ctxt, MCStringRef &r_table);
void MCPrintingSetPrintFontTable(MCExecContext& ctxt, MCStringRef p_table);

///////////

extern MCExecEnumTypeInfo *kMCServerErrorModeTypeInfo;
extern MCExecEnumTypeInfo *kMCServerOutputLineEndingsTypeInfo;
extern MCExecEnumTypeInfo *kMCServerOutputTextEncodingTypeInfo;

void MCServerExecPutHeader(MCExecContext& ctxt, MCStringRef value, bool as_new);
void MCServerExecPutBinaryOutput(MCExecContext& ctxt, MCDataRef value);
void MCServerExecPutContent(MCExecContext& ctxt, MCStringRef value);
void MCServerExecPutContentUnicode(MCExecContext& ctxt, MCDataRef value);
void MCServerExecPutMarkup(MCExecContext& ctxt, MCStringRef value);
void MCServerExecPutMarkupUnicode(MCExecContext& ctxt, MCDataRef value);
void MCServerExecPutCookie(MCExecContext& ctxt, MCStringRef name, MCStringRef value, uinteger_t expires, MCStringRef path, MCStringRef domain, bool is_secure, bool http_only);

void MCServerExecDeleteSession(MCExecContext& ctxt);
void MCServerExecStartSession(MCExecContext& ctxt);
void MCServerExecStopSession(MCExecContext& ctxt);

void MCServerExecInclude(MCExecContext& ctxt, MCStringRef filename, bool is_require);
void MCServerExecEcho(MCExecContext& ctxt, MCStringRef data);

void MCServerGetErrorMode(MCExecContext& ctxt, intenum_t& r_value);
void MCServerSetErrorMode(MCExecContext& ctxt, intenum_t p_value);
void MCServerGetOutputLineEnding(MCExecContext& ctxt, intenum_t& r_value);
void MCServerSetOutputLineEnding(MCExecContext& ctxt, intenum_t p_value);
void MCServerGetOutputTextEncoding(MCExecContext& ctxt, intenum_t& r_value);
void MCServerSetOutputTextEncoding(MCExecContext& ctxt, intenum_t p_value);
void MCServerGetSessionSavePath(MCExecContext& ctxt, MCStringRef &r_value);
void MCServerSetSessionSavePath(MCExecContext& ctxt, MCStringRef p_value);
void MCServerGetSessionLifetime(MCExecContext& ctxt, uinteger_t& r_value);
void MCServerSetSessionLifetime(MCExecContext& ctxt, uinteger_t p_value);
void MCServerGetSessionCookieName(MCExecContext& ctxt, MCStringRef &r_value);
void MCServerSetSessionCookieName(MCExecContext& ctxt, MCStringRef p_value);
void MCServerGetSessionId(MCExecContext& ctxt, MCStringRef &r_value);
void MCServerSetSessionId(MCExecContext& ctxt, MCStringRef p_value);

///////////

void MCDebuggingExecBreakpoint(MCExecContext& ctxt, uinteger_t p_line, uinteger_t p_pos);
void MCDebuggingExecDebugDo(MCExecContext& ctxt, MCStringRef p_script, uinteger_t p_line, uinteger_t p_pos);
void MCDebuggingExecAssert(MCExecContext& ctxt, int type, bool p_eval_success, bool p_result);

void MCDebuggingGetTraceAbort(MCExecContext& ctxtm, bool& r_value);
void MCDebuggingSetTraceAbort(MCExecContext& ctxtm, bool p_value);
void MCDebuggingGetTraceDelay(MCExecContext& ctxt, uinteger_t& r_value);
void MCDebuggingSetTraceDelay(MCExecContext& ctxt, uinteger_t p_value);
void MCDebuggingGetTraceReturn(MCExecContext& ctxtm, bool& r_value);
void MCDebuggingSetTraceReturn(MCExecContext& ctxtm, bool p_value);
void MCDebuggingGetTraceStack(MCExecContext& ctxt, MCStringRef& r_value);
void MCDebuggingSetTraceStack(MCExecContext& ctxt, MCStringRef p_value);
void MCDebuggingGetTraceUntil(MCExecContext& ctxt, uinteger_t& r_value);
void MCDebuggingSetTraceUntil(MCExecContext& ctxt, uinteger_t p_value);
void MCDebuggingGetMessageMessages(MCExecContext& ctxtm, bool& r_value);
void MCDebuggingSetMessageMessages(MCExecContext& ctxtm, bool p_value);

void MCDebuggingGetBreakpoints(MCExecContext& ctxt, MCStringRef& r_value);
void MCDebuggingSetBreakpoints(MCExecContext& ctxt, MCStringRef p_value);
void MCDebuggingGetDebugContext(MCExecContext& ctxt, MCStringRef& r_value);
void MCDebuggingSetDebugContext(MCExecContext& ctxt, MCStringRef p_value);
void MCDebuggingGetExecutionContexts(MCExecContext& ctxt, MCStringRef& r_value);
void MCDebuggingGetWatchedVariables(MCExecContext& ctxt, MCStringRef& r_value);
void MCDebuggingSetWatchedVariables(MCExecContext& ctxt, MCStringRef p_value);
void MCDebuggingExecPutIntoMessage(MCExecContext& ctxt, MCStringRef value, int where);
void MCDebuggingGetLogMessage(MCExecContext& ctxt, MCStringRef& r_value);
void MCDebuggingSetLogMessage(MCExecContext& ctxt, MCStringRef p_value);

///////////

void MCTextMessagingGetCanComposeTextMessage(MCExecContext& ctxt, bool& r_result);
void MCTextMessagingExecComposeTextMessage(MCExecContext& ctxt, MCStringRef p_recipients, MCStringRef p_body);

///////////

void MCIdleTimerExecLockIdleTimer(MCExecContext& ctxt);
void MCIdleTimerExecUnlockIdleTimer(MCExecContext& ctxt);
void MCIdleTimerGetIdleTimerLocked(MCExecContext& ctxt, bool& r_result);

//////////

void MCStoreGetCanMakePurchase(MCExecContext& ctxt, bool& r_result);
void MCStoreExecEnablePurchaseUpdates(MCExecContext& ctxt);
void MCStoreExecDisablePurchaseUpdates(MCExecContext& ctxt);
void MCStoreExecRestorePurchases(MCExecContext& ctxt);
void MCStoreGetPurchaseList(MCExecContext& ctxt, MCStringRef& r_list);
void MCStoreExecCreatePurchase(MCExecContext& ctxt, MCStringRef p_product_id, uint32_t& r_id);
void MCStoreGetPurchaseState(MCExecContext& ctxt, int p_id, MCStringRef& r_state);
void MCStoreGetPurchaseError(MCExecContext& ctxt, int p_id, MCStringRef& r_error);
void MCStoreGetPurchaseProperty(MCExecContext& ctxt, MCStringRef p_product_id, MCStringRef p_prop_name, MCStringRef& r_property_value);
void MCStoreSetPurchaseProperty(MCExecContext& ctxt, MCStringRef p_product_id, MCStringRef p_prop_name, MCStringRef p_value);
void MCStoreExecSendPurchaseRequest(MCExecContext& ctxt, uint32_t p_id);
void MCStoreExecConfirmPurchaseDelivery(MCExecContext& ctxt, uint32_t p_id);
void MCStoreExecRequestProductDetails(MCExecContext& ctxt, MCStringRef p_product_id);
void MCStoreExecPurchaseVerify(MCExecContext& ctxt, uint32_t p_id, bool p_verified);
void MCStoreExecConfirmPurchase(MCExecContext& ctxt, MCStringRef p_product_id);
void MCStoreExecMakePurchase(MCExecContext& ctxt, MCStringRef p_product_id, MCStringRef p_quantity, MCStringRef p_payload);
void MCStoreExecReceiveProductDetails(MCExecContext &ctxt, MCStringRef p_product_id, MCStringRef &r_details);
void MCStoreExecConsumePurchase(MCExecContext &ctxt, MCStringRef p_product_id);
void MCStoreExecProductSetType(MCExecContext &ctxt, MCStringRef p_product_id, MCStringRef p_product_type);

///////////

extern MCExecSetTypeInfo *kMCOrientationOrientationsTypeInfo;
extern MCExecEnumTypeInfo *kMCOrientationOrientationTypeInfo;

void MCOrientationGetDeviceOrientation(MCExecContext& ctxt, intenum_t& r_orientation);
void MCOrientationGetOrientation(MCExecContext& ctxt, intenum_t& r_orientation);
void MCOrientationGetAllowedOrientations(MCExecContext& ctxt, intset_t& r_orientation);
void MCOrientationSetAllowedOrientations(MCExecContext& ctxt, intset_t p_orientations);
void MCOrientationGetOrientationLocked(MCExecContext& ctxt, bool& r_locked);
void MCOrientationExecLockOrientation(MCExecContext& ctxt);
void MCOrientationExecUnlockOrientation(MCExecContext& ctxt);
void MCOrientationSetRectForOrientations(MCExecContext& ctxt, intset_t p_orientations, MCRectangle *p_rect);
bool MCOrientationGetRectForOrientation(intenum_t p_orientation, MCRectangle& r_rect);

///////////

void MCMailExecSendEmail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_subject, MCStringRef p_body);
void MCMailExecComposeMail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCArrayRef p_attachments);
void MCMailExecComposeUnicodeMail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCArrayRef p_attachments);
void MCMailExecComposeHtmlMail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCArrayRef p_attachments);
void MCMailGetCanSendMail(MCExecContext& ctxt, bool& r_result);

///////////

void MCAddressBookExecPickContact(MCExecContext& ctxt);
void MCAddressBookExecShowContact(MCExecContext& ctxt, int32_t p_contact_id);
void MCAddressBookExecCreateContact(MCExecContext& ctxt);
void MCAddressBookExecUpdateContact(MCExecContext& ctxt, MCArrayRef p_contact, MCStringRef p_title, MCStringRef p_message, MCStringRef p_alternate_name);
void MCAddressBookGetContactData(MCExecContext& ctxt, int32_t p_contact_id, MCArrayRef& r_contact_data);
void MCAddressBookExecRemoveContact(MCExecContext& ctxt, int32_t p_contact_id);
void MCAddressBookExecAddContact(MCExecContext &ctxt, MCArrayRef p_contact);
void MCAddressBookExecFindContact(MCExecContext& ctxt, MCStringRef p_contact_name);

///////////

struct MCAdTopLeft;

extern MCExecCustomTypeInfo* kMCAdTopLeftTypeInfo;

void MCAdExecRegisterWithInneractive(MCExecContext& ctxt, MCStringRef p_key);
void MCAdExecCreateAd(MCExecContext& ctxt, MCStringRef p_name, MCStringRef p_type, MCAdTopLeft p_topleft, MCArrayRef p_metadata);
void MCAdExecDeleteAd(MCExecContext& ctxt, MCStringRef p_name);
void MCAdSetVisibleOfAd(MCExecContext& ctxt, MCStringRef p_name, bool p_visible);
void MCAdGetVisibleOfAd(MCExecContext& ctxt, MCStringRef p_name, bool& r_visible);
void MCAdGetTopLeftOfAd(MCExecContext& ctxt, MCStringRef p_name, MCAdTopLeft& r_topleft);
void MCAdSetTopLeftOfAd(MCExecContext& ctxt, MCStringRef p_name, const MCAdTopLeft& p_topleft);
void MCAdGetAds(MCExecContext& ctxt, MCStringRef& r_ads);

///////////

struct MCNativeControlIdentifier;

extern MCExecCustomTypeInfo *kMCNativeControlColorTypeInfo;
extern MCExecCustomTypeInfo *kMCNativeControlRangeTypeInfo;
extern MCExecCustomTypeInfo *kMCNativeControlIdentifierTypeInfo;
extern MCExecCustomTypeInfo *kMCNativeControlDecelerationRateTypeInfo;
extern MCExecCustomTypeInfo *kMCNativeControlIndicatorInsetsTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlIndicatorStyleTypeInfo;

extern MCExecEnumTypeInfo *kMCNativeControlPlaybackStateTypeInfo;
extern MCExecSetTypeInfo *kMCNativeControlLoadStateTypeInfo;

extern MCExecEnumTypeInfo *kMCNativeControlInputCapitalizationTypeTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlInputAutocorrectionTypeTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlInputKeyboardTypeTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlInputKeyboardStyleTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlInputReturnKeyTypeTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlInputContentTypeTypeInfo;
extern MCExecSetTypeInfo *kMCNativeControlInputDataDetectorTypeTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlInputTextAlignTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlInputVerticalAlignTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlClearButtonModeTypeInfo;
extern MCExecEnumTypeInfo *kMCNativeControlBorderStyleTypeInfo;

void MCNativeControlExecCreateControl(MCExecContext& ctxt, MCStringRef p_type_name, MCStringRef p_control_name);
void MCNativeControlExecDeleteControl(MCExecContext& ctxt, MCStringRef p_control_name);
void MCNativeControlExecGet(MCExecContext& ctxt, MCStringRef p_control_name, MCStringRef p_property_name, MCValueRef& r_result);
void MCNativeControlExecSet(MCExecContext& ctxt, MCStringRef p_control_name, MCStringRef p_property_name, MCValueRef p_value);
void MCNativeControlExecDo(MCExecContext& ctxt, MCStringRef p_control_name, MCStringRef p_action_name, MCValueRef *p_arguments, uindex_t p_argument_count);
void MCNativeControlGetTarget(MCExecContext& ctxt, MCNativeControlIdentifier& r_target);
void MCNativeControlGetControlList(MCExecContext& ctxt, MCStringRef& r_list);

//////////

extern MCExecEnumTypeInfo *kMCSensorTypeTypeInfo;

void MCSensorExecStartTrackingSensor(MCExecContext& ctxt, intenum_t p_sensor, bool p_loosely);
void MCSensorExecStopTrackingSensor(MCExecContext& ctxt, intenum_t p_sensor);
void MCSensorGetSensorAvailable(MCExecContext& ctxt, intenum_t p_sensor, bool& r_available);

void MCSensorGetDetailedLocationOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_location);
void MCSensorGetLocationOfDevice(MCExecContext& ctxt, MCStringRef &r_location);
void MCSensorGetLocationHistoryOfDevice(MCExecContext& ctxt, MCArrayRef& r_location_history);

void MCSensorGetDetailedHeadingOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_heading);
void MCSensorGetHeadingOfDevice(MCExecContext& ctxt, MCStringRef &r_heading);

void MCSensorGetDetailedAccelerationOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_acceleration);
void MCSensorGetAccelerationOfDevice(MCExecContext& ctxt, MCStringRef &r_acceleration);

void MCSensorGetDetailedRotationRateOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_rotation_rate);
void MCSensorGetRotationRateOfDevice(MCExecContext& ctxt, MCStringRef &r_rotation_rate);

void MCSensorSetLocationCalibrationTimeout(MCExecContext& ctxt, int32_t p_timeout);
void MCSensorGetLocationCalibrationTimeout(MCExecContext& ctxt, int32_t& r_timeout);
// SN-2014-10-15: [[ Merge-6.7.0-rc-3 ]]
void MCSensorGetLocationAuthorizationStatus(MCExecContext& ctxt, MCStringRef &r_status);

//////////

extern MCExecEnumTypeInfo *kMCPickButtonTypeTypeInfo;
extern MCExecEnumTypeInfo *kMCPickCameraSourceTypeTypeInfo;
extern MCExecSetTypeInfo *kMCPickCameraFeaturesTypeInfo;
extern MCExecSetTypeInfo *kMCPickCamerasFeaturesTypeInfo;
extern MCExecSetTypeInfo *kMCPickMediaTypesTypeInfo;
extern MCExecEnumTypeInfo *kMCPickPhotoSourceTypeTypeInfo;

void MCPickExecPickDate(MCExecContext& ctxt, MCStringRef p_current, MCStringRef p_start, MCStringRef p_end, intenum_t p_buttons, MCRectangle p_button_rect);
void MCPickExecPickTime(MCExecContext &ctxt, MCStringRef p_current, MCStringRef p_start, MCStringRef p_end, int32_t *p_step, intenum_t p_buttons, MCRectangle p_button_rect);
void MCPickExecPickDateAndTime(MCExecContext &ctxt, MCStringRef p_current, MCStringRef p_start, MCStringRef p_end, int32_t *p_step, intenum_t p_buttons, MCRectangle p_button_rect);
void MCPickExecPickOptionByIndex(MCExecContext &ctxt, int p_chunk_type, MCStringRef *p_option_lists, uindex_t p_option_list_count, uindex_t *p_initial_indices, uindex_t p_indices_count, bool p_use_hilite_type, bool p_use_picker, bool p_use_cancel, bool p_use_done, MCRectangle p_button_rect);
void MCPickGetSpecificCameraFeatures(MCExecContext& ctxt, intenum_t p_source, intset_t& r_features);
void MCPickGetCameraFeatures(MCExecContext& ctxt, intset_t& r_features);
void MCPickExecPickMedia(MCExecContext &ctxt, intset_t p_allowed_types, bool p_multiple);
void MCPickExecPickPhotoAndResize(MCExecContext& ctxt, intenum_t p_source, uinteger_t p_width, uinteger_t p_height);
void MCPickExecPickPhoto(MCExecContext& ctxt, intenum_t p_source);

///////////

void MCCalendarExecShowEvent(MCExecContext& ctxt, MCStringRef p_id);
void MCCalendarGetEventData(MCExecContext& ctxt, MCStringRef p_id, MCArrayRef& r_data);
void MCCalendarExecCreateEvent(MCExecContext& ctxt);
void MCCalendarExecUpdateEvent(MCExecContext& ctxt, MCStringRef p_id);
void MCCalendarExecRemoveEvent(MCExecContext& ctxt, MCStringRef p_id);
void MCCalendarExecAddEvent(MCExecContext& ctxt, MCArrayRef p_data);
void MCCalendarGetCalendars(MCExecContext& ctxt);
void MCCalendarExecFindEvent(MCExecContext& ctxt, MCStringRef p_id, bool& r_found);

///////////

extern MCExecEnumTypeInfo* kMCStorePurchasePropertyTypeInfo;

void MCNotificationExecCreateLocalNotification(MCExecContext& ctxt, MCStringRef p_alert_body, MCStringRef p_alert_action, MCStringRef p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value);
void MCNotificationGetRegisteredNotifications(MCExecContext& ctxt);
void MCNotificationGetDetails(MCExecContext& ctxt, int32_t p_id, MCArrayRef& r_details);
void MCNotificationExecCancelLocalNotification(MCExecContext& ctxt, int32_t p_id);
void MCNotificationExecCancelAllLocalNotifications(MCExecContext& ctxt);
void MCNotificationGetNotificationBadgeValue(MCExecContext& ctxt);
void MCNotificationSetNotificationBadgeValue(MCExecContext& ctxt, uint32_t p_badge_value);

///////////

extern MCExecEnumTypeInfo* kMCBusyIndicatorTypeInfo;
extern MCExecEnumTypeInfo* kMCActivityIndicatorTypeInfo;

void MCBusyIndicatorExecStartActivityIndicator(MCExecContext& ctxt, intenum_t p_indicator, integer_t* p_location_x, integer_t* p_location_y);
void MCBusyIndicatorExecStopActivityIndicator(MCExecContext& ctxt);
void MCBusyIndicatorExecStartBusyIndicator(MCExecContext& ctxt, intenum_t p_indicator, MCStringRef p_label, int32_t p_opacity);
void MCBusyIndicatorExecStopBusyIndicator(MCExecContext& ctxt);

////////////

extern MCExecEnumTypeInfo* kMCSoundAudioCategoryTypeInfo;

void MCSoundExecPlaySoundOnChannel(MCExecContext& ctxt, MCStringRef p_channel, MCStringRef p_file, intenum_t p_type);
void MCSoundExecStopPlayingOnChannel(MCExecContext& ctxt, MCStringRef p_channel);
void MCSoundExecPausePlayingOnChannel(MCExecContext& ctxt, MCStringRef p_channel);
void MCSoundExecResumePlayingOnChannel(MCExecContext& ctxt, MCStringRef p_channel);
void MCSoundExecDeleteSoundChannel(MCExecContext& ctxt, MCStringRef p_channel);
void MCSoundSetVolumeOfChannel(MCExecContext& ctxt, MCStringRef p_channel, int32_t p_volume);
void MCSoundGetVolumeOfChannel(MCExecContext& ctxt, MCStringRef p_channel, int32_t& r_volume);
void MCSoundGetStatusOfChannel(MCExecContext& ctxt, MCStringRef p_channel, intenum_t& r_status);
void MCSoundGetSoundOfChannel(MCExecContext& ctxt, MCStringRef p_channel, MCStringRef &r_sound);
void MCSoundGetNextSoundOfChannel(MCExecContext& ctxt, MCStringRef p_channel, MCStringRef &r_sound);
void MCSoundGetSoundChannels(MCExecContext& ctxt, MCStringRef &r_channels);
void MCSoundSetAudioCategory(MCExecContext &ctxt, intenum_t p_category);

/////////////

extern MCExecEnumTypeInfo* kMCMiscStatusBarStyleTypeInfo;

void MCMiscGetDeviceToken(MCExecContext& ctxt, MCStringRef& r_token);
void MCMiscGetLaunchUrl(MCExecContext& ctxt, MCStringRef& r_url);

void MCMiscGetLaunchData(MCExecContext &ctxt, MCArrayRef &r_data);

void MCMiscExecBeep(MCExecContext& ctxt, int32_t* p_number_of_times);
void MCMiscExecVibrate(MCExecContext& ctxt, int32_t* p_number_of_times);

void MCMiscGetDeviceResolution(MCExecContext& ctxt, MCStringRef& r_resolution);
void MCMiscSetUseDeviceResolution(MCExecContext& ctxt, bool p_use_device_res, bool p_use_control_device_res);
void MCMiscGetDeviceScale(MCExecContext& ctxt, real64_t& r_scale);
void MCMiscGetPixelDensity(MCExecContext& ctxt, real64_t& r_density);

void MCMiscSetStatusBarStyle(MCExecContext& ctxt, intenum_t p_style);
void MCMiscExecShowStatusBar(MCExecContext& ctxt);
void MCMiscExecHideStatusBar(MCExecContext& ctxt);

void MCMiscSetKeyboardType(MCExecContext& ctxt, intenum_t p_keyboard_type);
void MCMiscSetKeyboardReturnKey(MCExecContext& ctxt, intenum_t p_keyboard_return_key);
void MCMiscExecSetKeyboardDisplay(MCExecContext& ctxt, intenum_t p_mode);
void MCMiscExecGetKeyboardDisplay(MCExecContext& ctxt, intenum_t& r_mode);

void MCMiscGetPreferredLanguages(MCExecContext& ctxt, MCStringRef& r_preferred_languages);
void MCMiscGetCurrentLocale(MCExecContext& ctxt, MCStringRef& r_current_locale);

void MCMiscExecClearTouches(MCExecContext& ctxt);

void MCMiscGetSystemIdentifier(MCExecContext& ctxt, MCStringRef& r_identifier);
void MCMiscGetApplicationIdentifier(MCExecContext& ctxt, MCStringRef& r_identifier);
void MCMiscGetIdentifierForVendor(MCExecContext& ctxt, MCStringRef& r_identifier);

void MCMiscSetReachabilityTarget(MCExecContext& ctxt, MCStringRef p_hostname);
void MCMiscGetReachabilityTarget(MCExecContext& ctxt, MCStringRef& r_hostname);

void MCMiscExecExportImageToAlbum(MCExecContext& ctxt, MCStringRef p_data, MCStringRef p_file_name);

void MCMiscSetRedrawInterval(MCExecContext& ctxt, int32_t p_interval);
void MCMiscSetAnimateAutorotation(MCExecContext& ctxt, bool p_enabled);

void MCMiscGetDoNotBackupFile(MCExecContext& ctxt, MCStringRef p_path, bool& r_no_backup);
void MCMiscSetDoNotBackupFile(MCExecContext& ctxt, MCStringRef p_path, bool p_no_backup);
void MCMiscGetFileDataProtection(MCExecContext& ctxt, MCStringRef p_path, MCStringRef& p_protection_string);
void MCMiscSetFileDataProtection(MCExecContext& ctxt, MCStringRef p_path, MCStringRef p_protection_string);

void MCMiscExecLibUrlDownloadToFile(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_filename);
void MCMiscExecLibUrlSetSSLVerification(MCExecContext& ctxt, bool p_enabled);

void MCMiscGetBuildInfo(MCExecContext& ctxt, MCStringRef p_key, MCStringRef& r_value);
void MCMiscExecRequestPermission(MCExecContext& ctxt, MCStringRef p_permission, bool& r_granted);
void MCMiscExecPermissionExists(MCExecContext& ctxt, MCStringRef p_permission, bool& r_exists);
void MCMiscExecHasPermission(MCExecContext& ctxt, MCStringRef p_permission, bool& r_permission_granted);

void MCMiscExecEnableRemoteControl(MCExecContext& ctxt);
void MCMiscExecDisableRemoteControl(MCExecContext& ctxt);
void MCMiscGetRemoteControlEnabled(MCExecContext& ctxt, bool& r_enabled);
void MCMiscSetRemoteControlDisplayProperties(MCExecContext& ctxt, MCArrayRef p_props);

void MCMiscGetIsVoiceOverRunning(MCExecContext& ctxt, bool& r_is_vo_running);

////////////////////////////////////////////////////////////////////////////////

void MCNFCGetIsNFCAvailable(MCExecContext& ctxt);
void MCNFCGetIsNFCEnabled(MCExecContext& ctxt);
void MCNFCExecEnableNFCDispatch(MCExecContext& ctxt);
void MCNFCExecDisableNFCDispatch(MCExecContext& ctxt);

////////////////////////////////////////////////////////////////////////////////

bool MCExtensionInitialize(void);
void MCExtensionFinalize(void);

bool MCExtensionConvertToScriptType(MCExecContext& ctxt, MCValueRef& x_value);
bool MCExtensionConvertFromScriptType(MCExecContext& ctxt, MCTypeInfoRef p_type, MCValueRef& x_value);

Exec_stat MCExtensionCatchError(MCExecContext& ctxt);

////////////////////////////////////////////////////////////////////////////////

template<typename T> struct MCExecValueTraits
{
};

template<> struct MCExecValueTraits<MCValueRef>
{
    typedef MCValueRef in_type;
    typedef MCValueRef& out_type;
	
	static const MCExecValueType type_enum = kMCExecValueTypeValueRef;

    inline static void set(MCExecValue& self, MCValueRef p_value)
    {
        MCExecTypeSetValueRef(self, p_value);
    }

    inline static void release(MCValueRef& self) { MCValueRelease(self); }
    inline static MCValueRef retain(MCValueRef& self) {	return MCValueRetain(self); }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCValueRef& r_value)
    {
        return ctxt . EvalExprAsValueRef(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<MCBooleanRef>
{
    typedef MCBooleanRef in_type;
    typedef MCBooleanRef& out_type;
	
	static const MCExecValueType type_enum = kMCExecValueTypeBooleanRef;

    inline static void set(MCExecValue& self, MCBooleanRef p_value)
    {
        self . type = type_enum;
        self . booleanref_value = p_value;
    }

    inline static void release(MCBooleanRef& self) { MCValueRelease(self); }
    inline static MCBooleanRef retain(MCBooleanRef self) { return MCValueRetain(self); }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCBooleanRef& r_value)
    {
        return ctxt . EvalExprAsBooleanRef(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<MCNameRef>
{
    typedef MCNameRef in_type;
    typedef MCNameRef& out_type;
	
	static const MCExecValueType type_enum = kMCExecValueTypeNameRef;

    inline static void set(MCExecValue& self, MCNameRef p_value)
    {
        self . type = type_enum;
        self . nameref_value = p_value;
    }

    inline static void release(MCNameRef& self) { MCValueRelease(self); }
    inline static MCNameRef retain(MCNameRef& self) { return MCValueRetain(self); } 

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCNameRef& r_value)
    {
        return ctxt . EvalExprAsNameRef(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<MCDataRef>
{
    typedef MCDataRef in_type;
    typedef MCDataRef& out_type;
	
	static const MCExecValueType type_enum = kMCExecValueTypeDataRef;

    inline static void set(MCExecValue& self, MCDataRef p_value)
    {
        self . type = type_enum;
        self . dataref_value = p_value;
    }

    inline static void release(MCDataRef& self) { MCValueRelease(self); }
    inline static MCDataRef retain(MCDataRef& self) { return MCValueRetain(self); }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCDataRef& r_value)
    {
        return ctxt . EvalExprAsDataRef(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<MCArrayRef>
{
    typedef MCArrayRef in_type;
    typedef MCArrayRef& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeArrayRef;

    inline static void set(MCExecValue& self, MCArrayRef p_value)
    {
        self . type = type_enum;
        self . arrayref_value = p_value;
    }

    inline static void release(MCArrayRef& self) { MCValueRelease(self); }
    inline static MCArrayRef retain(MCArrayRef& self) { return MCValueRetain(self); }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCArrayRef& r_value)
    {
        return ctxt . EvalExprAsArrayRef(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<MCNumberRef>
{
    typedef MCNumberRef in_type;
    typedef MCNumberRef& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeNumberRef;

    inline static void set(MCExecValue& self, MCNumberRef p_value)
    {
        self . type = type_enum;
        self . numberref_value = p_value;
    }

    inline static void release(MCNumberRef& self) { MCValueRelease(self); }
    inline static MCNumberRef retain(MCNumberRef& self) { return MCValueRetain(self); }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCNumberRef& r_value)
    {
        return ctxt . EvalExprAsNumberRef(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<MCStringRef>
{
    typedef MCStringRef in_type;
    typedef MCStringRef& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeStringRef;

	inline static void set(MCExecValue& self, MCStringRef p_value)
    {
        self . type = type_enum;
        self . stringref_value = p_value;
    }

    inline static void release(MCStringRef& self) { MCValueRelease(self); }
    inline static MCStringRef retain(MCStringRef& self) { return MCValueRetain(self); }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCStringRef& r_value)
    {
        return ctxt . EvalExprAsStringRef(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<integer_t>
{
    typedef integer_t in_type;
    typedef integer_t& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeInt;
	
    inline static void set(MCExecValue& self, integer_t p_value)
    {
        self . type = type_enum;
        self . int_value = p_value;
    }

    inline static void release(integer_t& self){}
    inline static integer_t retain(integer_t& self) { return self; }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, integer_t& r_value)
    {
        return ctxt . EvalExprAsInt(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<uinteger_t>
{
    typedef uinteger_t in_type;
    typedef uinteger_t& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeUInt;
	
    inline static void set(MCExecValue& self, uinteger_t p_value)
    {
        self . type = type_enum;
        self . uint_value = p_value;
    }

    inline static void release(uinteger_t& self){}
    inline static uinteger_t retain(uinteger_t& self) { return self; }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, uinteger_t& r_value)
    {
        return ctxt . EvalExprAsUInt(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<bool>
{
    typedef bool in_type;
    typedef bool& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeBool;
	
    inline static void set(MCExecValue& self, bool p_value)
    {
        self . type = type_enum;
        self . bool_value = p_value;
    }

    inline static void release(bool& self){}
	inline static bool retain(bool& self) { return self; }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, bool& r_value)
    {
        return ctxt . EvalExprAsBool(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<double>
{
    typedef double in_type;
    typedef double& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeDouble;

	inline static void set(MCExecValue& self, double p_value)
    {
        self . type = type_enum;
        self . double_value = p_value;
    }

    inline static void release(double& self){}
    inline static double retain(double& self) { return self; }
    
    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, double& r_value)
    {
        return ctxt . EvalExprAsDouble(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<char_t>
{
    typedef char_t in_type;
    typedef char_t& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeChar;

	inline static void set(MCExecValue& self, char_t p_value)
    {
        self . type = type_enum;
        self . char_value = p_value;
    }

    inline static void release(char_t& self){}
	inline static char_t retain(char_t& self) { return self; }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, char_t& r_value)
    {
        return ctxt . EvalExprAsChar(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<MCPoint>
{
    typedef MCPoint in_type;
    typedef MCPoint& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypePoint;

	inline static void set(MCExecValue& self, MCPoint p_value)
    {
        self . type = type_enum;
        self . point_value = p_value;
    }

    inline static void release(MCPoint& self){}
    inline static MCPoint retain(MCPoint& self) { return self; }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCPoint& r_value)
    {
        return ctxt . EvalExprAsPoint(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<MCColor>
{
    typedef MCColor in_type;
    typedef MCColor& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeColor;

	inline static void set(MCExecValue& self, MCColor p_value)
    {
        self . type = type_enum;
        self . color_value = p_value;
    }

    inline static void release(MCColor& self){}
	inline static MCColor retain(MCColor& self) { return self; }


    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCColor& r_value)
    {
        return ctxt . EvalExprAsColor(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<MCRectangle>
{
    typedef MCRectangle in_type;
    typedef MCRectangle& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeRectangle;

	inline static void set(MCExecValue& self, MCRectangle p_value)
    {
        self . type = type_enum;
        self . rectangle_value = p_value;
    }

    inline static void release(MCRectangle& self){}
	inline static MCRectangle retain(MCRectangle& self) { return self; }

    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, MCRectangle& r_value)
    {
        return ctxt . EvalExprAsRectangle(p_expr, p_error, r_value);
    }
};

template<> struct MCExecValueTraits<float>
{
    typedef float in_type;
    typedef float& out_type;

	static const MCExecValueType type_enum = kMCExecValueTypeFloat;

	inline static void set(MCExecValue& self, float p_value)
    {
        self . type = type_enum;
        self . float_value = p_value;
    }

    inline static void release(float& self){}
    inline static float retain(float& self) { return self; }


    inline static bool eval(MCExecContext &ctxt, MCExpression* p_expr, Exec_errors p_error, float& r_value)
    {
        double t_double;
        if (!ctxt . EvalExprAsDouble(p_expr, p_error, t_double))
            return false;
        r_value = (float)t_double;
        return true;
    }
};

#endif
