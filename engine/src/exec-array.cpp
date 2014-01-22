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
#include "mcio.h"

#include "globals.h"
#include "objectstream.h"
#include "util.h"
#include "variable.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EVAL_METHOD(Arrays, Keys, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Arrays, Extents, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, Combine, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, CombineByRow, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, CombineByColumn, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, CombineAsSet, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, Split, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, SplitByRow, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, SplitByColumn, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, SplitAsSet, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, Union, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Arrays, Intersect, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Arrays, ArrayEncode, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Arrays, ArrayDecode, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Arrays, MatrixMultiply, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Arrays, TransposeMatrix, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Arrays, IsAnArray, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Arrays, IsNotAnArray, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Arrays, IsAmongTheKeysOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Arrays, IsNotAmongTheKeysOf, 3)

////////////////////////////////////////////////////////////////////////////////

void MCArraysEvalKeys(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef& r_string)
{
	if (MCArrayListKeys(p_array, '\n', r_string))
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

struct array_element_t
{
	MCNameRef key;
	MCValueRef value;
};

struct combine_array_t
{
	uindex_t index;
	array_element_t *elements;
};

static bool list_array_elements(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	combine_array_t *ctxt;
	ctxt = (combine_array_t *)p_context;
	ctxt -> elements[ctxt -> index] . key = p_key;
	ctxt -> elements[ctxt -> index] . value = p_value;
	ctxt -> index++;
	return true;
}

static int compare_array_element(const void *a, const void *b)
{
	const array_element_t *t_left, *t_right;
	t_left = (const array_element_t *)a;
	t_right = (const array_element_t *)b;
	return MCStringCompareTo(MCNameGetString(t_left -> key), MCNameGetString(t_right -> key), kMCStringOptionCompareExact);
}

void MCArraysExecCombine(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef p_element_delimiter, MCStringRef p_key_delimiter, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;

	uindex_t t_count;
	t_count = MCArrayGetCount(p_array);

	MCAutoStringRef t_string;
	if (t_success)
		t_success = MCStringCreateMutable(0, &t_string);

	combine_array_t t_lisctxt;
	t_lisctxt . elements = nil;
	if (t_success)
		t_success = MCMemoryNewArray(t_count, t_lisctxt . elements);

	if (t_success)
	{
		t_lisctxt . index = 0;
		MCArrayApply(p_array, list_array_elements, &t_lisctxt);
		qsort(t_lisctxt . elements, t_count, sizeof(array_element_t), compare_array_element);
		for(uindex_t i = 0; i < t_count; i++)
		{
			MCAutoStringRef t_value_as_string;
			if (p_key_delimiter != nil)
			{
				t_success = ctxt . ConvertToString(t_lisctxt . elements[i] . value, &t_value_as_string);
				if (!t_success)
					break;
			}

			t_success =
				MCStringAppend(*t_string, MCNameGetString(t_lisctxt . elements[i] . key)) &&
				(p_key_delimiter == nil ||
					MCStringAppend(*t_string, p_key_delimiter) &&
					MCStringAppend(*t_string, *t_value_as_string)) &&
				(i == t_count - 1 ||
					MCStringAppend(*t_string, p_element_delimiter));

			if (!t_success)
				break;
		}
	}
	
	if (t_success)
		t_success = MCStringCopy(*t_string, r_string);

	MCMemoryDeleteArray(t_lisctxt . elements);

	if (t_success)
		return;

	// Throw the current error code (since last library call returned false).
	ctxt . Throw();
}

void MCArraysExecCombineByColumn(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef& r_string)
{
	ctxt . Unimplemented();
}

void MCArraysExecCombineAsSet(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef p_element_delimiter, MCStringRef& r_string)
{
	// String into which the combined keys are accumulated
    MCAutoStringRef t_string;
    
    // The array keys are not added in any particular order
    MCNameRef t_key;
    MCValueRef t_value_ignored;
    uintptr_t t_iterator = 0;
    while (MCArrayIterate(p_array, t_iterator, t_key, t_value_ignored))
    {
        bool t_success;
        t_success = true;
        if (*t_string == nil)
            t_success = MCStringMutableCopy(MCNameGetString(t_key), &t_string);
        else
            t_success = MCStringAppendFormat(*t_string, "%@%@", p_element_delimiter, t_key);
        
        if (!t_success)
        {
            ctxt . Throw();
            return;
        }
    }
    
    MCStringCopy(*t_string, r_string);
}

//////////

void MCArraysExecSplit(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_element_delimiter, MCStringRef p_key_delimiter, MCArrayRef& r_array)
{
	if (MCStringSplit(p_string, p_element_delimiter, p_key_delimiter, ctxt . GetCaseSensitive() ? kMCStringOptionCompareExact : kMCStringOptionCompareCaseless, r_array))
		return;

	ctxt . Throw();
}

void MCArraysExecSplitByColumn(MCExecContext& ctxt, MCStringRef p_string, MCArrayRef& r_array)
{
    codepoint_t t_row_delim, t_column_delim;
    t_row_delim = ctxt . GetRowDelimiter();
    t_column_delim = ctxt . GetColumnDelimiter();
    
    // Output array
    MCAutoArrayRef t_array;
    if (!MCArrayCreateMutable(&t_array))
    {
        ctxt . Throw();
        return;
    }
    
    // Temporary array for storing columns
    MCAutoArray<MCStringRef> t_temp_array;
    
    // Iterate over the rows of the input string
    uindex_t t_offset, t_length;
    t_offset = 0;
    t_length = MCStringGetLength(p_string);
    while (t_offset < t_length)
    {
        // Find the end of this row
        uindex_t t_row_end;
        if (!MCStringFirstIndexOfChar(p_string, t_row_delim, t_offset, ctxt . GetCaseSensitive(), t_row_end))
            t_row_end = t_length;
        
        // Iterate over the cells of this row
        uindex_t t_cell_offset, t_column_index;
        t_cell_offset = t_offset;
        t_column_index = 0;
        while (t_cell_offset < t_row_end)
        {
            // Find the end of this cell
            uindex_t t_cell_end;
            if (!MCStringFirstIndexOfChar(p_string, t_column_delim, t_cell_offset, ctxt . GetCaseSensitive(), t_cell_end) || t_cell_end > t_row_end)
                t_cell_end = t_row_end;
            
            // Check that the output array has a slot for this column
            if (t_temp_array.Size() <= t_column_index)
                t_temp_array.Extend(t_column_index + 1);
            
            // Check that a string has been created to store this column
            bool t_success;
            MCRange t_range;
            t_range = MCRangeMake(t_cell_offset, t_cell_end - t_cell_offset);
            if (t_temp_array[t_column_index] == nil)
                t_success = MCStringMutableCopySubstring(p_string, t_range, t_temp_array[t_column_index]);
            else
            {
                t_success = MCStringAppendChar(t_temp_array[t_column_index], t_row_delim);
                if (t_success)
                    t_success = MCStringAppendFormat(t_temp_array[t_column_delim], "%*@", t_range, p_string);
            }
            
            if (!t_success)
            {
                ctxt . Throw();
                return;
            }
            
            // Next cell
            t_column_index++;
            t_cell_offset = t_cell_end;
        }
        
        // Next row
        t_offset = t_row_end;
    }
    
    // Convert the temporary array into a "proper" array
    for (uindex_t i = 0; i < t_temp_array.Size(); i++)
    {
        if (!MCArrayStoreValueAtIndex(*t_array, i, t_temp_array[i]))
        {
            ctxt . Throw();
            return;
        }
    }
    
    MCArrayCopy(*t_array, r_array);
}

void MCArraysExecSplitAsSet(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_element_delimiter, MCArrayRef& r_array)
{
	// Split the incoming string into its components
    MCAutoArrayRef t_keys;
    if (!MCStringSplit(p_string, p_element_delimiter, nil, ctxt . GetCaseSensitive() ? kMCStringOptionCompareExact : kMCStringOptionCompareCaseless, &t_keys))
    {
        ctxt . Throw();
        return;
    }
    
    // The new array, with keys equal to the components of the string
    MCAutoArrayRef t_array;
    if (!MCArrayCreateMutable(&t_array))
    {
        ctxt . Throw();
        return;
    }
    
    MCNameRef t_key_ignored;
    MCValueRef t_value;
    uintptr_t t_iterator = 0;
    while (MCArrayIterate(*t_keys, t_iterator, t_key_ignored, t_value))
    {
        // The value stored at each key is a boolean "true"
        MCNewAutoNameRef t_keyname;
        if (!ctxt . ConvertToName(t_value, &t_keyname)
            || !MCArrayStoreValue(*t_array, ctxt . GetCaseSensitive(), *t_keyname, kMCTrue))
        {
            ctxt . Throw();
            return;
        }
        
    }
    
    MCArrayCopy(*t_array, r_array);
}

////////////////////////////////////////////////////////////////////////////////

void MCArraysDoUnion(MCExecContext& ctxt, MCArrayRef p_dst_array, MCArrayRef p_src_array, bool p_recursive)
{
	MCNameRef t_key;
	MCValueRef t_src_value;
    MCValueRef t_dst_value;
	uintptr_t t_iterator;
	t_iterator = 0;
    
    bool t_is_array;
    
	while(MCArrayIterate(p_src_array, t_iterator, t_key, t_src_value))
	{
		if (MCArrayFetchValue(p_dst_array, ctxt . GetCaseSensitive(), t_key, t_dst_value))
        {
            if (p_recursive && MCValueIsArray(t_dst_value) && MCValueIsArray(t_src_value))
            {
                MCArraysExecUnionRecursive(ctxt, (MCArrayRef)t_dst_value, (MCArrayRef)t_src_value);
                if (ctxt . HasError())
                    return;
            }
			continue;
        }
        
		if (!MCArrayStoreValue(p_dst_array, ctxt . GetCaseSensitive(), t_key, MCValueRetain(t_src_value)))
		{
			ctxt . Throw();
			return;
		}
	}
}

void MCArraysDoIntersect(MCExecContext& ctxt, MCArrayRef p_dst_array, MCArrayRef p_src_array, bool p_recursive)
{
	MCNameRef t_key;
	MCValueRef t_src_value;
    MCValueRef t_dst_value;
	uintptr_t t_iterator;
	t_iterator = 0;
    
    bool t_is_array;
    
	while(MCArrayIterate(p_src_array, t_iterator, t_key, t_src_value))
	{
		if (MCArrayFetchValue(p_dst_array, ctxt . GetCaseSensitive(), t_key, t_dst_value))
        {
            if (p_recursive && MCValueIsArray(t_dst_value) && MCValueIsArray(t_src_value))
            {
                MCArraysExecIntersectRecursive(ctxt, (MCArrayRef)t_dst_value, (MCArrayRef)t_src_value);
                if (ctxt . HasError())
                    return;
            }
			continue;
        }
        
		if (!MCArrayRemoveValue(p_dst_array, ctxt . GetCaseSensitive(), t_key))
		{
			ctxt . Throw();
			return;
		}
	}
}

void MCArraysExecUnion(MCExecContext& ctxt, MCArrayRef p_dst_array, MCArrayRef p_src_array)
{
    MCArraysDoUnion(ctxt, p_dst_array, p_src_array, false);
}

void MCArraysExecIntersect(MCExecContext& ctxt, MCArrayRef p_dst_array, MCArrayRef p_src_array)
{
    MCArraysDoIntersect(ctxt, p_dst_array, p_src_array, false);
}

void MCArraysExecUnionRecursive(MCExecContext& ctxt, MCArrayRef p_dst_array, MCArrayRef p_src_array)
{
    MCArraysDoUnion(ctxt, p_dst_array, p_src_array, true);
}

void MCArraysExecIntersectRecursive(MCExecContext& ctxt, MCArrayRef p_dst_array, MCArrayRef p_src_array)
{
    MCArraysDoIntersect(ctxt, p_dst_array, p_src_array, true);
}

////////////////////////////////////////////////////////////////////////////////

void MCArraysEvalArrayEncode(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef& r_encoding)
{
	bool t_success;
	t_success = true;

	IO_handle t_stream_handle;
	t_stream_handle = nil;
	if (t_success)
	{
		t_stream_handle = MCS_fakeopenwrite();
		if (t_stream_handle == nil)
			t_success = false;
	}

	MCObjectOutputStream *t_stream;
	t_stream = nil;
	if (t_success)
	{
		t_stream = new MCObjectOutputStream(t_stream_handle);
		if (t_stream == nil)
			t_success = false;
	}

	if (t_success)
		if (t_stream -> WriteU8(kMCEncodedValueTypeArray) != IO_NORMAL)
			t_success = false;
	
	if (t_success)
		if (MCArraySaveToStreamLegacy(p_array, false, *t_stream) != IO_NORMAL)
			t_success = false;

	if (t_success)
		t_stream -> Flush(true);

	delete t_stream;

	//////////

	void *t_buffer;
	size_t t_length;
	t_buffer = nil;
	t_length = 0;
	if (MCS_closetakingbuffer(t_stream_handle, t_buffer, t_length) != IO_NORMAL)
		t_success = false;

	if (t_success)
		t_success = MCStringCreateWithNativeChars((const char_t *)t_buffer, t_length, r_encoding);

	free(t_buffer);

	if (t_success)
		return;

	ctxt . Throw();
}

void MCArraysEvalArrayDecode(MCExecContext& ctxt, MCStringRef p_encoding, MCArrayRef& r_array)
{
	bool t_success;
	t_success = true;

	IO_handle t_stream_handle;
	t_stream_handle = nil;
    if (t_success)
    {
		t_stream_handle = MCS_fakeopen(MCStringGetOldString(p_encoding));
		if (t_stream_handle == nil)
			t_success = false;
	}

	MCObjectInputStream *t_stream;
	t_stream = nil;
	if (t_success)
	{
		t_stream = new MCObjectInputStream(t_stream_handle, MCStringGetLength(p_encoding));
		if (t_stream == nil)
			t_success = false;
	}

	uint8_t t_type;
	if (t_success)
		if (t_stream -> ReadU8(t_type) != IO_NORMAL)
			t_success = false;

	MCArrayRef t_array;
	t_array = nil;
	if (t_success)
		t_success = MCArrayCreateMutable(t_array);

	if (t_success)
		if (MCArrayLoadFromStreamLegacy(t_array, *t_stream) != IO_NORMAL)
			t_success = false;

	delete t_stream;
	MCS_close(t_stream_handle);

	if (t_success)
	{
		r_array = t_array;
		return;
	}

	MCValueRelease(t_array);

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCArraysSplitIndexes(MCNameRef p_key, integer_t*& r_indexes, uindex_t& r_count, bool& r_all_integers)
{
	r_indexes = nil;
	r_count = 0;

	MCStringRef t_string = MCNameGetString(p_key);
	uindex_t t_string_len = MCStringGetLength(t_string);
	if (t_string_len == 0)
		return true;

	r_all_integers = true;
    
    uindex_t t_start, t_finish;    
    t_start = 0;
    t_finish = 0;
    
	for(;;)
	{
        if (!MCStringFirstIndexOfChar(t_string, ',', t_start, kMCCompareExact, t_finish))
            t_finish = t_string_len;
        		
		if (!MCMemoryResizeArray(r_count + 1, r_indexes, r_count))
			return false;
        
        MCAutoStringRef t_substring;
        MCAutoNumberRef t_number;
        MCStringCopySubstring(t_string, MCRangeMake(t_start, t_finish - t_start), &t_substring);
        
        if (!MCNumberParse(*t_substring, &t_number))
        {
            r_indexes[r_count - 1] = 0;
            r_all_integers = false;
            break;
        }
        else
            r_indexes[r_count - 1] = MCNumberFetchAsInteger(*t_number);

		if (t_finish >= t_string_len)
			break;

		t_start = t_finish + 1;
	}

	return true;
}

struct array_extent_t
{
	integer_t min;
	integer_t max;
};

bool MCArraysCopyExtents(MCArrayRef self, array_extent_t*& r_extents, uindex_t& r_count)
{
	uintptr_t t_index = 0;
	MCNameRef t_key;
	MCValueRef t_value;

	bool t_has_extents = false;
	uindex_t t_dimensions = 0;
	MCAutoArray<array_extent_t> t_extents;

	while (MCArrayIterate(self, t_index, t_key, t_value))
	{
		MCAutoPointer<index_t> t_indexes;
		uindex_t t_index_count;
		bool t_all_integers;
		if (!MCArraysSplitIndexes(t_key, &t_indexes, t_index_count, t_all_integers))
			return false;

		if (!t_all_integers || t_index_count == 0)
		{
			t_has_extents = false;
			break;
		}

		if (!t_has_extents)
		{
			t_dimensions = t_index_count;

			t_has_extents = true;
			
			if (!t_extents.New(t_dimensions))
				return false;
			for (uindex_t i = 0; i < t_dimensions; i++)
				t_extents[i].min = t_extents[i].max = (*t_indexes)[i];
		}
		else
		{
			if (t_dimensions != t_index_count)
			{
				t_has_extents = false;
				break;
			}
			for (uindex_t i = 0; i < t_dimensions; i++)
			{
				integer_t t_number = (*t_indexes)[i];
				if (t_number > t_extents[i].max)
					t_extents[i].max = t_number;
				if (t_number < t_extents[i].min)
					t_extents[i].min = t_number;
			}
		}
	}

	if (!t_has_extents)
	{
		r_extents = nil;
		r_count = 0;
		return true;
	}

	t_extents.Take(r_extents, r_count);
	return true;
}

bool MCArraysCopyExtents(MCArrayRef self, MCListRef& r_list)
{
	MCAutoArray<array_extent_t> t_extents;
	if (!MCArraysCopyExtents(self, t_extents.PtrRef(), t_extents.SizeRef()))
		return false;

	uindex_t t_dimensions = t_extents.Size();

	if (t_dimensions == 0)
	{
		r_list = MCValueRetain(kMCEmptyList);
		return true;
	}

	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	for (uindex_t i = 0; i < t_dimensions; i++)
	{
		MCAutoStringRef t_string;
		if (!MCStringFormat(&t_string, "%d,%d", t_extents[i].min, t_extents[i].max))
			return false;
		if (!MCListAppend(*t_list, *t_string))
			return false;
	}

	return MCListCopy(*t_list, r_list);
}

void MCArraysEvalExtents(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCArraysCopyExtents(p_array, &t_list) &&
		MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

struct matrix_t
{
	integer_t rows, row_offset;
	integer_t columns, column_offset;
	real64_t values[0];
};

inline bool MCMatrixNew(integer_t p_rows, integer_t p_cols, integer_t p_row_offset, integer_t p_col_offset, matrix_t*& r_matrix)
{
	if (!MCMemoryNew(sizeof(matrix_t) + sizeof(real64_t) * p_rows * p_cols, (void*&)r_matrix))
		return false;

	r_matrix->columns = p_cols;
	r_matrix->column_offset = p_col_offset;

	r_matrix->rows = p_rows;
	r_matrix->row_offset = p_row_offset;

	return true;
}

inline real64_t& MCMatrixEntry(matrix_t *m, integer_t r, integer_t c)
{
	return m->values[r * m->columns + c];
}


#define extent_size(x) (x.max - x.min + 1)
bool MCArraysCopyMatrix(MCExecContext& ctxt, MCArrayRef self, matrix_t*& r_matrix)
{
	MCAutoArray<array_extent_t> t_extents;
	if (!MCArraysCopyExtents(self, t_extents.PtrRef(), t_extents.SizeRef()) ||
		t_extents.Size() != 2)
		return false;

	integer_t t_rows = extent_size(t_extents[0]);
	integer_t t_cols = extent_size(t_extents[1]);

	integer_t t_row_offset = t_extents[0].min;
	integer_t t_col_offset = t_extents[1].min;

	if (MCArrayGetCount(self) != t_rows * t_cols)
		return false;

	MCAutoPointer<matrix_t> t_matrix;
	if (!MCMatrixNew(t_rows, t_cols, t_row_offset, t_col_offset, &t_matrix))
		return false;

	for (integer_t row = 0; row < t_rows; row++)
	{
		for (integer_t col = 0; col < t_cols; col++)
		{
			MCAutoStringRef t_string;
			MCNewAutoNameRef t_name;
			MCValueRef t_value;
			if (!MCStringFormat(&t_string, "%d,%d", row + t_row_offset, col + t_col_offset) ||
				!MCNameCreate(*t_string, &t_name) ||
				!MCArrayFetchValue(self, true, *t_name, t_value) ||
				!ctxt.ConvertToReal(t_value, MCMatrixEntry(*t_matrix, row, col)))
				return false;
		}
	}

	t_matrix.Take(r_matrix);
	return true;
}

bool MCArraysCreateWithMatrix(matrix_t *p_matrix, MCArrayRef& r_array)
{
	MCAutoArrayRef t_array;
	if (!MCArrayCreateMutable(&t_array))
		return false;

	for (integer_t r = 0; r < p_matrix->rows; r++)
	{
		for (integer_t c = 0; c < p_matrix->columns; c++)
		{
			MCAutoStringRef t_string;
			MCNewAutoNameRef t_key;
			MCAutoNumberRef t_value;
			if (!MCStringFormat(&t_string, "%d,%d", r + p_matrix->row_offset, c + p_matrix->column_offset) ||
				!MCNameCreate(*t_string, &t_key) ||
				!MCNumberCreateWithReal(MCMatrixEntry(p_matrix, r, c), &t_value) ||
				!MCArrayStoreValue(*t_array, true, *t_key, *t_value))
				return false;
		}
	}

	return MCArrayCopy(*t_array, r_array);
}

bool MCMatrixMultiply(matrix_t *p_a, matrix_t *p_b, matrix_t*& r_c)
{
	if (p_a->columns != p_b->rows ||
		p_a->column_offset != p_b->column_offset ||
		p_a->row_offset != p_b->row_offset)
		return false;

	MCAutoPointer<matrix_t> t_c;
	if (!MCMatrixNew(p_a->rows, p_b->columns, p_a->row_offset, p_a->column_offset, &t_c))
		return false;

	for (index_t i = 0; i < p_a->rows; i++)
	{
		for (index_t j = 0; j < p_b->columns; j++)
		{
			real64_t t_total = 0;
			for (index_t k = 0; k < p_a->columns; k++)
				t_total += MCMatrixEntry(p_a, i, k) * MCMatrixEntry(p_b, k, j);
			MCMatrixEntry(*t_c, i, j) = t_total;
		}
	}

	t_c.Take(r_c);
	return true;
}

void MCArraysEvalMatrixMultiply(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, MCArrayRef& r_result)
{
	MCAutoPointer<matrix_t> t_left, t_right, t_product;
	if (!MCArraysCopyMatrix(ctxt, p_left, &t_left) || !MCArraysCopyMatrix(ctxt, p_right, &t_right) ||
		!MCMatrixMultiply(*t_left, *t_right, &t_product))
	{
		ctxt.LegacyThrow(EE_MATRIXMULT_MISMATCH);
		return;
	}

	if (MCArraysCreateWithMatrix(*t_product, r_result))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCArraysCopyTransposed(MCArrayRef self, MCArrayRef& r_transposed)
{
	MCAutoArray<array_extent_t> t_extents;
	if (!MCArraysCopyExtents(self, t_extents.PtrRef(), t_extents.SizeRef()) ||
		t_extents.Size() != 2)
		return false;

	integer_t t_rows = extent_size(t_extents[0]);
	integer_t t_cols = extent_size(t_extents[1]);

	integer_t t_row_end = t_extents[0].min + t_rows;
	integer_t t_col_end = t_extents[1].min + t_cols;

	if (MCArrayGetCount(self) != t_rows * t_cols)
		return false;

	MCAutoArrayRef t_transposed;
	if (!MCArrayCreateMutable(&t_transposed))
		return false;

	for (integer_t r = t_extents[0].min; r < t_row_end; r++)
	{
		for (integer_t c = t_extents[1].min; c < t_col_end; c++)
		{
			MCAutoStringRef t_src_string, t_dst_string;
			MCNewAutoNameRef t_src_name, t_dst_name;
			MCValueRef t_value;
			if (!MCStringFormat(&t_src_string, "%d,%d", r, c) ||
				!MCStringFormat(&t_dst_string, "%d,%d", c, r))
				return false;
			if (!MCNameCreate(*t_src_string, &t_src_name) ||
				!MCNameCreate(*t_dst_string, &t_dst_name))
				return false;
			if (!MCArrayFetchValue(self, true, *t_src_name, t_value) ||
				!MCArrayStoreValue(*t_transposed, true, *t_dst_name, t_value))
				return false;
		}
	}

	return MCArrayCopy(*t_transposed, r_transposed);
}

void MCArraysEvalTransposeMatrix(MCExecContext& ctxt, MCArrayRef p_matrix, MCArrayRef& r_result)
{
	if (!MCArraysCopyTransposed(p_matrix, r_result))
		ctxt.LegacyThrow(EE_TRANSPOSE_MISMATCH);
}

////////////////////////////////////////////////////////////////////////////////

void MCArraysEvalIsAnArray(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	r_result = MCValueGetTypeCode(p_value) == kMCValueTypeCodeArray;
}

void MCArraysEvalIsNotAnArray(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	r_result = MCValueGetTypeCode(p_value) != kMCValueTypeCodeArray;
}

////////////////////////////////////////////////////////////////////////////////

void MCArraysEvalIsAmongTheKeysOf(MCExecContext& ctxt, MCNameRef p_key, MCArrayRef p_array, bool& r_result)
{
	MCValueRef t_value;
	r_result = MCArrayFetchValue(p_array, ctxt.GetCaseSensitive(), p_key, t_value);
}

void MCArraysEvalIsNotAmongTheKeysOf(MCExecContext& ctxt, MCNameRef p_key, MCArrayRef p_array, bool& r_result)
{
	MCArraysEvalIsAmongTheKeysOf(ctxt, p_key, p_array, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////
