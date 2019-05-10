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
#include "mcio.h"

#include "exec.h"
#include "exec-interface.h"

#include "stackfileformat.h"
#include "patternmatcher.h"

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

////////////////////////////////////////////////////////////////////////////////

// combine by row or column expects an integer-indexed array
struct array_int_indexed_element_t
{
    index_t key;
    MCValueRef value;
};

struct combine_int_indexed_array_t
{
    uindex_t index;
    array_int_indexed_element_t *elements;
    MCExecContext* converter;
};

static bool list_int_indexed_array_elements(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
    combine_int_indexed_array_t *ctxt;
    ctxt = (combine_int_indexed_array_t *)p_context;

    MCAutoNumberRef t_key;
    if (ctxt -> converter -> ConvertToNumber(MCNameGetString(p_key), &t_key))
    {
        index_t t_key_num = MCNumberFetchAsInteger(*t_key);

        if (t_key_num < 1) // Invalid index
            return false;

        ctxt -> elements[ctxt -> index] . key = t_key_num;
        ctxt -> elements[ctxt -> index] . value = p_value;
        ++(ctxt -> index);

        return true;
    }
    else
        return false;
}

static int compare_int_indexed_elements(const void *a, const void* b)
{
    const array_int_indexed_element_t *t_left, *t_right;
    t_left = (const array_int_indexed_element_t *)a;
    t_right = (const array_int_indexed_element_t *)b;

    return (t_left -> key - t_right -> key < 0) ? -1 : (t_left -> key != t_right -> key ? 1 : 0);
}

////////////////////////////////////////////////////////////////////////////////

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
            
            t_success = ctxt . ConvertToString(t_lisctxt . elements[i] . value, &t_value_as_string);
            if (!t_success)
                break;

			t_success =
				(p_key_delimiter == nil ||
                    (MCStringAppend(*t_string, MCNameGetString(t_lisctxt . elements[i] . key)) &&
					MCStringAppend(*t_string, p_key_delimiter)))&&
				MCStringAppend(*t_string, *t_value_as_string) &&
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

// Should be removed when 'combine by row' and 'combine by column' only differs by the delimiter used,
// not by the way the array is handled - any index is fine for combine by row
void MCArraysExecCombineByRow(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef &r_string)
{
    MCAutoListRef t_list;
    bool t_success = MCListCreateMutable(ctxt . GetRowDelimiter(), &t_list);

    uindex_t t_count = MCArrayGetCount(p_array);
    combine_array_t t_lisctxt;
	
	MCAutoArray<array_element_t> t_elements;
	if (t_success)
		t_success = t_elements . New(t_count);
	
	if (t_success)
	{
		t_lisctxt . elements = t_elements.Ptr();
		t_lisctxt . index = 0;
	}
	
    if (t_success)
        t_success = MCArrayApply(p_array, list_array_elements, &t_lisctxt);
	
	if (t_success)
	{
        qsort(t_lisctxt . elements, t_count, sizeof(array_element_t), compare_array_element);

        for (uindex_t i = 0; i < t_count && t_success; ++i)
        {
            MCAutoStringRef t_string;
            if (ctxt . ConvertToString(t_lisctxt . elements[i] . value, &t_string))
                t_success = MCListAppend(*t_list, *t_string);
            else
                t_success = false;
        }
    }

    if (t_success && MCListCopyAsString(*t_list, r_string))
        return;

    ctxt . Throw();
}

// SN-2014-09-01: [[ Bug 13297 ]] Combining by column deserves its own function as it is too
// different from combining by row
void MCArraysExecCombineByColumn(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef &r_string)
{    
    MCStringRef t_row_delimiter, t_col_delimiter;
    t_row_delimiter = ctxt . GetRowDelimiter();
    t_col_delimiter = ctxt . GetColumnDelimiter();
    
    MCAutoListRef t_list;
	bool t_success = MCListCreateMutable(t_row_delimiter, &t_list);
    
    uindex_t t_count = MCArrayGetCount(p_array);
    combine_int_indexed_array_t t_lisctxt;
	
	MCAutoArray<array_int_indexed_element_t> t_elements;
	if (t_success)
		t_success = t_elements . New(t_count);
	
	if (t_success)
	{
		t_lisctxt . elements = t_elements.Ptr();
		t_lisctxt . index = 0;
		t_lisctxt . converter = &ctxt;
	}
	
	if (t_success)
		t_success = MCArrayApply(p_array, list_int_indexed_array_elements, &t_lisctxt);
	
    if (t_success)
    {
		qsort(t_lisctxt . elements, t_count, sizeof(array_element_t), compare_int_indexed_elements);
		
		// Combine by row/column is only valid if all the indices are consecutive numbers
		// Otherwise, an empty string is returned - no error
		index_t t_last_index;
		t_last_index = 0;
		for (uindex_t i = 0; i < t_count; ++i)
		{
			if (!t_last_index)
			{
				t_last_index = t_lisctxt . elements[i] . key;
				continue;
			}
			
			if (++t_last_index != t_lisctxt . elements[i] . key)
			{
				r_string = MCValueRetain(kMCEmptyString);
				return;
			}
		}

		// SN-2014-09-01: [[ Bug 13297 ]]
		// We need to store the converted strings in a array, to be able to iterate through the elements by one row-delimitated
		//  at a time
		MCAutoStringRefArray t_strings;
		t_success = t_strings . New(t_count);
		
		MCAutoArray<uindex_t> t_next_row_indices;
		if (t_success)
			t_next_row_indices . New(t_count);
		
		for (uindex_t i = 0; i < t_count && t_success; ++i)
		{
			if (t_lisctxt . elements[i] . key == 0) // The index 0 is ignored
				continue;
			
			t_success = ctxt . ConvertToString(t_lisctxt . elements[i] . value, t_strings[i]);
		}
		
		// SN-2014-09-01: [[ Bug 13297 ]] Added a missed part in column-combining:
		// only combining row-by-row the array elements.
		if (t_success)
		{
			uindex_t t_elements_over;
			t_elements_over = 0;
			
			// We iterate as long as one element still has uncombined rows
			while (t_success && t_elements_over != t_count)
			{
				MCAutoListRef t_row;
				
				t_success = MCListCreateMutable(t_col_delimiter, &t_row);
				t_elements_over = 0;
				
				// Iterate through all the elements of the array
				for (uindex_t i = 0; i < t_count && t_success; ++i)
				{
					// Only consider this element if it has any uncombined rows remaining
					if (t_next_row_indices[i] < MCStringGetLength(t_strings[i]))
					{
						MCRange t_cell_range;
						if (MCStringFind(t_strings[i], MCRangeMake(t_next_row_indices[i], UINDEX_MAX), t_row_delimiter, ctxt.GetStringComparisonType(), &t_cell_range))
						{
							// We found a row delimiter, so we stop the copy range before it and update the next index from which to look
							t_success = MCListAppendSubstring(*t_row, t_strings[i], MCRangeMakeMinMax(t_next_row_indices[i], t_cell_range . offset));
							t_next_row_indices[i] = t_cell_range . offset + t_cell_range . length;
						}
						else
						{
							// No row delimiter: we copy the remaining part of the string and mark the element
							// as wholly combined by setting the next index to the length of the element
							t_success = MCListAppendSubstring(*t_row, t_strings[i], MCRangeMake(t_next_row_indices[i], UINDEX_MAX));
							t_next_row_indices[i] = MCStringGetLength(t_strings[i]);
						}
					}
					else
					{
						// Everything has been combined in this element
						t_elements_over++;
						t_success = MCListAppend(*t_row, kMCEmptyString);
					}
				}
				
				// One more row has been combined - doing it anyway mimics the previous behaviour of having an empty row
				// added in the end when combining by columns
				if (t_success && t_elements_over != t_count)
					t_success = MCListAppend(*t_list, *t_row);
			}
		}
	}
	
    if (t_success && MCListCopyAsString(*t_list, r_string))
        return;
    
    ctxt . Throw();
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

    if (*t_string == nil)
        r_string = MCValueRetain(kMCEmptyString);
    else
        MCStringCopy(*t_string, r_string);
}

//////////

void MCArraysExecSplit(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_element_delimiter, MCStringRef p_key_delimiter, MCArrayRef& r_array)
{
	if (MCStringSplit(p_string, p_element_delimiter, p_key_delimiter, ctxt . GetStringComparisonType(), r_array))
		return;

	ctxt . Throw();
}

void MCArraysExecSplitByColumn(MCExecContext& ctxt, MCStringRef p_string, MCArrayRef& r_array)
{
    MCStringRef t_row_delim, t_column_delim;
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
    
    bool t_success;
    t_success = true;
    
    uindex_t t_row_index;
    t_row_index = 0;
    
    while (t_success && t_offset < t_length)
    {
        // Find the end of this row
        MCRange t_row_found;
        if (!MCStringFind(p_string, MCRangeMake(t_offset, UINDEX_MAX), t_row_delim, ctxt . GetStringComparisonType(), &t_row_found))
        {
            t_row_found . offset = t_length;
            t_row_found . length = 0;
        }
        
        // Iterate over the cells of this row
        uindex_t t_cell_offset, t_column_index;
        t_cell_offset = t_offset;
        t_column_index = 0;
        while (t_success && t_cell_offset <= t_row_found . offset)
        {
            // Find the end of this cell
            MCRange t_cell_found;
            if (!MCStringFind(p_string, MCRangeMake(t_cell_offset, UINDEX_MAX), t_column_delim, ctxt . GetStringComparisonType(), &t_cell_found) || t_cell_found . offset > t_row_found . offset)
            {
                t_cell_found . offset = t_row_found . offset;
                // AL-2014-08-04: [[ Bug 13090 ]] Make sure cell offset is incremented eventually when the delimiter is not found
                t_cell_found . length = 1;
            }
            
            // Check that the output array has a slot for this column
            if (t_temp_array.Size() <= t_column_index)
                t_temp_array.Extend(t_column_index + 1);
            
            // Check that a string has been created to store this column
            MCRange t_range;
            t_range = MCRangeMakeMinMax(t_cell_offset, t_cell_found . offset);
            if (t_temp_array[t_column_index] == nil)
            {
                t_success = MCStringCreateMutable(0, t_temp_array[t_column_index]);
                
                // AL-2014-08-04: [[ Bug 13090 ]] If we are creating a new column, make sure we pad with empty cells 'above' this one
                uindex_t t_rows = t_row_index;
                while (t_success && t_rows--)
                    t_success = MCStringAppend(t_temp_array[t_column_index], t_row_delim);
                
                if (t_success)
                    t_success = MCStringAppendFormat(t_temp_array[t_column_index], "%*@", &t_range, p_string);
            }
            else
            {
                // AL-2014-06-12: [[ Bug 12610 ]] Range parameter to MCStringFormat must be a pointer to an MCRange
                t_success = MCStringAppendFormat(t_temp_array[t_column_index], "%@%*@", t_row_delim, &t_range, p_string);
            }
            
            // Next cell
            t_column_index++;
            t_cell_offset = t_cell_found . offset + t_cell_found . length;
        }
        
        // AL-2014-08-04: [[ Bug 13090 ]] Pad the rest of this row with empty cells
        index_t t_pad_number;
        t_pad_number = t_temp_array . Size() - t_column_index;
        if (t_success && t_pad_number > 0)
        {
            while (t_success && t_pad_number--)
                t_success = MCStringAppend(t_temp_array[t_column_index++], t_row_delim);
        }
        
        // Next row
        t_row_index++;
        t_offset = t_row_found . offset + t_row_found . length;
    }
    
    // Convert the temporary array into a "proper" array
    for (uindex_t i = 0; i < t_temp_array.Size() && t_success; i++)
    {
        t_success = MCArrayStoreValueAtIndex(*t_array, i + 1, t_temp_array[i]);
        MCValueRelease(t_temp_array[i]);
    }

    if (t_success)
        t_success = MCArrayCopy(*t_array, r_array);
    
    if (!t_success)
        ctxt . Throw();
}

void MCArraysExecSplitAsSet(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_element_delimiter, MCArrayRef& r_array)
{
	// Split the incoming string into its components
    MCAutoArrayRef t_keys;
    if (!MCStringSplit(p_string, p_element_delimiter, nil, ctxt . GetStringComparisonType(), &t_keys))
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

//
// Semantics of 'union tLeft with tRight [recursively]'
// 
// repeat for each key tKey in tRight
//    if tKey is not among the keys of tLeft then
//        put tRight[tKey] into tLeft[tKey]
//    else if tRecursive then
//        union tLeft[tKey] with tRight[tKey] recursively
//    end if
// end repeat
//
// if right is not an array then no-op
// else if left is not an array then left = right
//
// Semantics of 'symmetric difference tLeft with tRight'
// 
// repeat for each key tKey in tRight
//    if tKey is among the keys of tLeft then
//        delete tLeft[tKey]
//    else if tKey is not among the keys of tLeft then
//        put tRight[tKey] into tLeft[tKey]
//    end if
// end repeat
//
// if right is not an array then no-op
// else if left is not an array then left = right
//

enum MCArrayDoUnionOp
{
    kMCArrayDoUnionOpUnion,
    kMCArrayDoUnionOpUnionRecursively,
    kMCArrayDoUnionOpSymmetricDifference,
};

static void MCArraysDoUnion(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCArrayDoUnionOp p_op, MCValueRef& r_result)
{
    if (!MCValueIsArray(p_src))
    {
        r_result = MCValueRetain(p_dst);
        return;
    }
    
    if (!MCValueIsArray(p_dst))
    {
        r_result = MCValueRetain(p_src);
        return;
        
    }
    
    MCArrayRef t_src_array;
    t_src_array = (MCArrayRef)p_src;
    
    MCArrayRef t_dst_array;
    t_dst_array = (MCArrayRef)p_dst;
    
    MCAutoArrayRef t_result;
    if (!MCArrayMutableCopy(t_dst_array, &t_result))
        return;
    
    MCNameRef t_key;
    MCValueRef t_src_value;
    MCValueRef t_dst_value;
    uintptr_t t_iterator;
    t_iterator = 0;
    
	while(MCArrayIterate(t_src_array, t_iterator, t_key, t_src_value))
	{
        bool t_key_exists;
        t_key_exists = MCArrayFetchValue(t_dst_array, ctxt . GetCaseSensitive(), t_key, t_dst_value);

        if (!t_key_exists)
        {
            if (!MCArrayStoreValue(*t_result, ctxt . GetCaseSensitive(), t_key, t_src_value))
            {
                ctxt . Throw();
                return;
            }
        }
        else if (p_op == kMCArrayDoUnionOpUnionRecursively)
        {
            MCAutoValueRef t_recursive_result;
            MCArraysDoUnion(ctxt, t_dst_value, t_src_value, p_op, &t_recursive_result);
            if (ctxt . HasError())
                return;
            
            if (!MCArrayStoreValue(*t_result, ctxt . GetCaseSensitive(), t_key, *t_recursive_result))
            {
                ctxt . Throw();
                return;
            }
        }
        else if (p_op == kMCArrayDoUnionOpSymmetricDifference)
        {
            if (!MCArrayRemoveValue(*t_result, ctxt.GetCaseSensitive(), t_key))
            {
                ctxt.Throw();
                return;
            }
        }
	}
    
    r_result = MCValueRetain(*t_result);
}

//
// Semantics of 'intersect tLeft with tRight [recursively]'
// 
// repeat for each key tKey in tLeft
//    if tKey is not among the keys of tRight then
//        delete variable tLeft[tKey]
//    else if tRecursive then
//        intersect tLeft[tKey] with tRight[tKey] recursively
//    end if
// end repeat
//
// if left is not an array then no-op
// else if right is not an array then left = empty
//
// Semantics of 'difference tLeft with tRight'
// 
// repeat for each key tKey in tLeft
//    if tKey is among the keys of tRight then
//        delete variable tLeft[tKey]
//    end if
// end repeat
//
// if left is not an array then no-op
// else if right is not an array then no-op

enum MCArrayDoIntersectOp
{
    kMCArrayDoIntersectOpIntersect,
    kMCArrayDoIntersectOpIntersectRecursively,
    kMCArrayDoIntersectOpDifference,
};

static void MCArraysDoIntersect(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCArrayDoIntersectOp p_op, MCValueRef& r_result)
{
    if (!MCValueIsArray(p_dst))
    {
        r_result = MCValueRetain(p_dst);
        return;
        
    }

    if (!MCValueIsArray(p_src))
    {
        if (p_op != kMCArrayDoIntersectOpDifference)
        {
            r_result = MCValueRetain(kMCEmptyString);
        }
        else
        {
            r_result = MCValueRetain(p_dst);
        }
        return;
    }
    
    MCArrayRef t_dst_array;
    t_dst_array = (MCArrayRef)p_dst;
    
    MCArrayRef t_src_array;
    t_src_array = (MCArrayRef)p_src;
    
    MCAutoArrayRef t_result;
    if (!MCArrayMutableCopy(t_dst_array, &t_result))
        return;
    
    MCNameRef t_key;
    MCValueRef t_src_value;
    MCValueRef t_dst_value;
    uintptr_t t_iterator;
    t_iterator = 0;
    
    while(MCArrayIterate(t_dst_array, t_iterator, t_key, t_dst_value))
    {
        bool t_key_exists;
        t_key_exists = MCArrayFetchValue(t_src_array, ctxt . GetCaseSensitive(), t_key, t_src_value);

        if (t_key_exists == (p_op == kMCArrayDoIntersectOpDifference))
        {
            if (!MCArrayRemoveValue(*t_result, ctxt . GetCaseSensitive(), t_key))
            {
                ctxt . Throw();
                return;
            }
        }
        else if (p_op == kMCArrayDoIntersectOpIntersectRecursively)
        {
            MCAutoValueRef t_recursive_result;
            MCArraysDoIntersect(ctxt, t_dst_value, t_src_value, p_op, &t_recursive_result);
            
            if (ctxt . HasError())
                return;
            
            if (!MCArrayStoreValue(*t_result, ctxt . GetCaseSensitive(), t_key, *t_recursive_result))
                return;
        }
    }
    
    r_result = MCValueRetain(*t_result);
}

void MCArraysExecUnion(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result)
{
    MCArraysDoUnion(ctxt, p_dst, p_src, kMCArrayDoUnionOpUnion, r_result);
}

void MCArraysExecUnionRecursively(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result)
{
    MCArraysDoUnion(ctxt, p_dst, p_src, kMCArrayDoUnionOpUnionRecursively, r_result);
}

void MCArraysExecSymmetricDifference(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result)
{
    MCArraysDoUnion(ctxt, p_dst, p_src, kMCArrayDoUnionOpSymmetricDifference, r_result);
}

void MCArraysExecIntersect(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result)
{
    MCArraysDoIntersect(ctxt, p_dst, p_src, kMCArrayDoIntersectOpIntersect, r_result);
}

void MCArraysExecIntersectRecursively(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result)
{
    MCArraysDoIntersect(ctxt, p_dst, p_src, kMCArrayDoIntersectOpIntersectRecursively, r_result);
}

void MCArraysExecDifference(MCExecContext& ctxt, MCValueRef p_dst, MCValueRef p_src, MCValueRef& r_result)
{
    MCArraysDoIntersect(ctxt, p_dst, p_src, kMCArrayDoIntersectOpDifference, r_result);
}


////////////////////////////////////////////////////////////////////////////////

void MCArraysEvalArrayEncode(MCExecContext& ctxt, MCArrayRef p_array, MCStringRef p_version, MCDataRef& r_encoding)
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

    // AL-2014-05-15: [[ Bug 12203 ]] Add version parameter to arrayEncode, to allow
    //  version 7.0 variant to preserve unicode. 
    MCInterfaceStackFileVersion t_version;
    if (p_version != nil)
        MCInterfaceStackFileVersionParse(ctxt, p_version, t_version);
    
    // AL-2014-05-22: [[ Bug 12547 ]] Make arrayEncode encode in 7.0 format by default.
    bool t_legacy;
    t_legacy = p_version != nil && t_version . version < kMCStackFileFormatVersion_7_0;
    
    if (t_legacy)
    {
        MCObjectOutputStream *t_stream;
        t_stream = nil;
        if (t_success)
        {
            t_stream = new (nothrow) MCObjectOutputStream(t_stream_handle);
            if (t_stream == nil)
                t_success = false;
        }
        
        if (t_success)
        {
            if (t_stream -> WriteU8(kMCEncodedValueTypeLegacyArray) != IO_NORMAL)
                t_success = false;
        }
        
        if (t_success)
            if (MCArraySaveToStreamLegacy(p_array, false, *t_stream) != IO_NORMAL)
                t_success = false;
        
        if (t_success)
            t_stream -> Flush(true);
        
        delete t_stream;
    }
    else
    {
        if (t_success)
            if (IO_write_uint1(kMCEncodedValueTypeArray, t_stream_handle) != IO_NORMAL)
                t_success = false;
        
        if (t_success)
            if (IO_write_valueref_new(p_array, t_stream_handle) != IO_NORMAL)
                t_success = false;
    }
	
	//////////

	void *t_buffer;
	size_t t_length;
	t_buffer = nil;
	t_length = 0;
	if (MCS_closetakingbuffer(t_stream_handle, t_buffer, t_length) != IO_NORMAL)
		t_success = false;

	if (t_success)
		t_success = MCDataCreateWithBytes((const byte_t *)t_buffer, t_length, r_encoding);

	free(t_buffer);

	if (t_success)
		return;

	ctxt . Throw();
}

void MCArraysEvalArrayDecode(MCExecContext& ctxt, MCDataRef p_encoding, MCArrayRef& r_array)
{
	bool t_success;
	t_success = true;

	IO_handle t_stream_handle;
	t_stream_handle = nil;
    if (t_success)
    {
        t_stream_handle = MCS_fakeopen(MCDataGetBytePtr(p_encoding), MCDataGetLength(p_encoding));
		if (t_stream_handle == nil)
			t_success = false;
	}

    uint8_t t_type;
	if (t_success)
		if (IO_read_uint1(&t_type, t_stream_handle) != IO_NORMAL)
			t_success = false;
        
    // AL-2014-05-01: [[ Bug 11989 ]] If the type is 'empty' then just return the empty array.
	if (t_success && t_type == kMCEncodedValueTypeEmpty)
    {
        r_array = MCValueRetain(kMCEmptyArray);
        return;
    }
    
    // AL-2014-05-15: [[ Bug 12203 ]] Check initial byte for version 7.0 encoded array.
    bool t_legacy;
    t_legacy = t_type < kMCEncodedValueTypeArray;

    MCArrayRef t_array = nil;
	if (t_legacy)
    {
        if (t_success)
            t_success = MCArrayCreateMutable(t_array);
        
        if (t_success)
            if (MCS_putback(t_type, t_stream_handle) != IO_NORMAL)
                t_success = false;
        
        MCObjectInputStream *t_stream;
        t_stream = nil;
        if (t_success)
        {
            t_stream = new (nothrow) MCObjectInputStream(t_stream_handle, MCDataGetLength(p_encoding), false);
            if (t_stream == nil)
                t_success = false;
        }
        
        if (t_success)
            if (t_stream -> ReadU8(t_type) != IO_NORMAL)
                t_success = false;
        
        if (t_success)
            if (MCArrayLoadFromStreamLegacy(t_array, *t_stream) != IO_NORMAL)
                t_success = false;
        
        if (t_success)
        {
            if (!MCArrayCopyAndRelease(t_array, t_array))
            {
                MCValueRelease(t_array);
                t_success = false;
            }
        }
        
        delete t_stream;
    }
    else
    {
        if (t_success)
            if (IO_read_valueref_new((MCValueRef &)t_array, t_stream_handle) != IO_NORMAL)
                t_success = false;
    }
    
	MCS_close(t_stream_handle);

	if (t_success)
	{
		r_array = t_array;
		return;
	}

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCArraysSplitIndexes(MCNameRef p_key, integer_t*& r_indexes, uindex_t& r_count, bool& r_all_integers)
{
	MCStringRef t_string = MCNameGetString(p_key);
	uindex_t t_string_len = MCStringGetLength(t_string);
	if (t_string_len == 0)
	{
		r_indexes = nil;
		r_count = 0;
		return true;
	}
	
	MCAutoArray<integer_t> t_indexes;
	r_all_integers = true;
    
    uindex_t t_start, t_finish;    
    t_start = 0;
    t_finish = 0;
    
	for(;;)
	{
        if (!MCStringFirstIndexOfChar(t_string, ',', t_start, kMCCompareExact, t_finish))
            t_finish = t_string_len;
		
        MCAutoStringRef t_substring;
        if (!MCStringCopySubstring(t_string, MCRangeMakeMinMax(t_start, t_finish), &t_substring))
			return false;
		
        MCAutoNumberRef t_number;
#ifdef FIX_ANOMALY_21476_STRICT_EXTENTS
        if (!MCNumberParseInteger(*t_substring, &t_number))
#endif
        if (!MCNumberParse(*t_substring, &t_number))
        {
            if (!t_indexes . Push(0))
				return false;
			
            r_all_integers = false;
            break;
        }
        
        if (!t_indexes . Push(MCNumberFetchAsInteger(*t_number)))
			return false;
			
		if (t_finish >= t_string_len)
			break;

		t_start = t_finish + 1;
	}
	
	t_indexes . Take(r_indexes, r_count);
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
		MCAutoCustomPointer<index_t,MCMemoryDeleteArray> t_indexes;
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

	if (MCArrayGetCount(self) != (uindex_t) t_rows * t_cols)
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
	// MW-2014-03-14: [[ Bug 11924 ]] If both are empty arrays, then the result
	//   is empty.
    if (MCArrayIsEmpty(p_left) && MCArrayIsEmpty(p_right))
    {
        r_result = MCValueRetain(kMCEmptyArray);
        return;
    }
    
    // MW-2014-03-14: [[ Bug 11924 ]] If either array is empty, then its a mismatch.
	MCAutoPointer<matrix_t> t_left, t_right, t_product;
	if (MCArrayIsEmpty(p_left) || MCArrayIsEmpty(p_right) ||
        !MCArraysCopyMatrix(ctxt, p_left, &t_left) || !MCArraysCopyMatrix(ctxt, p_right, &t_right) ||
		!MCMatrixMultiply(*t_left, *t_right, &t_product))
	{
		ctxt.LegacyThrow(EE_MATRIXMULT_MISMATCH);
		return;
	}

	if (MCArraysCreateWithMatrix(*t_product, r_result))
		return;

	ctxt.Throw();
}

void MCArraysEvalVectorDotProduct(MCExecContext& ctxt, MCArrayRef p_left, MCArrayRef p_right, double& r_result)
{
    double t_sum = 0.0;
    
    MCNameRef t_key;
    MCValueRef t_value;
    uintptr_t t_index = 0;
    while (MCArrayIterate(p_left, t_index, t_key, t_value))
    {
        MCValueRef t_other_value;
        double t_number, t_other_number;
        if (!ctxt . ConvertToReal(t_value, t_number) ||
            !MCArrayFetchValue(p_right, ctxt.GetCaseSensitive(), t_key, t_other_value) ||
            !ctxt . ConvertToReal(t_other_value, t_other_number))
        {
            ctxt . LegacyThrow(EE_VECTORDOT_MISMATCH);
            return;
        }

        t_sum += t_number * t_other_number;
    }
    
    r_result = t_sum;
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

	if (MCArrayGetCount(self) != (uindex_t) t_rows * t_cols)
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
    // FG-2014-10-21: [[ Bugfix 13737 ]] An array is only an array if it has at
    // least one key (i.e the empty array is not an array...)
    r_result = MCValueGetTypeCode(p_value) == kMCValueTypeCodeArray
        && MCArrayGetCount((MCArrayRef)p_value) > 0;
}

void MCArraysEvalIsNotAnArray(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
    MCArraysEvalIsAnArray(ctxt, p_value, r_result);
    r_result = !r_result;
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

void MCArraysExecFilter(MCExecContext& ctxt, MCArrayRef p_source, bool p_without, MCPatternMatcher *p_matcher, bool p_match_key, MCArrayRef &r_result)
{
    MCAutoArrayRef t_result;
    if (!MCArrayMutableCopy(p_source, &t_result))
        return;
    
    MCNameRef t_key;
    MCValueRef t_value;
    uintptr_t t_iterator;
    t_iterator = 0;
    
    while(MCArrayIterate(p_source, t_iterator, t_key, t_value))
    {
        bool t_match = p_matcher -> match(ctxt, t_key, p_match_key);
        
        if ((t_match && p_without) || (!t_match && !p_without))
        {
            if (!MCArrayRemoveValue(*t_result, ctxt . GetCaseSensitive(), t_key))
            {
                ctxt . Throw();
                return;
            }

        }
    }
    
    r_result = MCValueRetain(*t_result);
}

void MCArraysExecFilterWildcard(MCExecContext& ctxt, MCArrayRef p_source, MCStringRef p_pattern, bool p_without, bool p_match_keys, MCArrayRef &r_result)
{
    // Create the pattern matcher
    MCWildcardMatcher t_matcher(p_pattern, p_source, ctxt . GetStringComparisonType());
    
    MCArraysExecFilter(ctxt, p_source, p_without, &t_matcher, p_match_keys, r_result);
}

void MCArraysExecFilterRegex(MCExecContext& ctxt, MCArrayRef p_source, MCStringRef p_pattern, bool p_without, bool p_match_keys, MCArrayRef &r_result)
{
    // Create the pattern matcher
    MCRegexMatcher t_matcher(p_pattern, p_source, ctxt . GetStringComparisonType());
    
    MCAutoStringRef t_regex_error;
    if (!t_matcher.compile(&t_regex_error))
    {
        ctxt . LegacyThrow(EE_MATCH_BADPATTERN);
        return;
    }
    
    MCArraysExecFilter(ctxt, p_source, p_without, &t_matcher, p_match_keys, r_result);
}

void MCArraysExecFilterExpression(MCExecContext& ctxt, MCArrayRef p_source, MCExpression* p_expression, bool p_without, bool p_match_keys, MCArrayRef &r_result)
{
    // Create the pattern matcher
    MCExpressionMatcher t_matcher(p_expression, p_source, ctxt . GetStringComparisonType());
    
    MCArraysExecFilter(ctxt, p_source, p_without, &t_matcher, p_match_keys, r_result);
}

////////////////////////////////////////////////////////////////////////////////
