/*                                                                     -*-c++-*-
Copyright (C) 2015 LiveCode Ltd.

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

#include <emscripten.h>
#include <ffi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* Alignment of Emscripten function pointers in its function pointer
 * array. */
#define FUNCTION_POINTER_ALIGNMENT 2

/* Alignment of Emscripten data pointers */
#define DATA_POINTER_ALIGNMENT 4

/* ================================================================
 * Helper functions
 * ================================================================ */

/* Map libffi type descriptors to LLVM IR type descriptors. */
static const char *
ffi_map_to_llvm_ir(ffi_type *p_type) {
	if (p_type == &ffi_type_void)
		return NULL;
	if (p_type == &ffi_type_sint8 || p_type == &ffi_type_uint8)
		return "i8";
	if (p_type == &ffi_type_uint16 || p_type == &ffi_type_sint16)
		return "i16";
	if (p_type == &ffi_type_uint32 || p_type ==  &ffi_type_sint32)
		return "i32";
	if (p_type == &ffi_type_uint64 || p_type == &ffi_type_sint64)
		return "i64";
	if (p_type == &ffi_type_float)
		return "float";
	if (p_type == &ffi_type_double)
		return "double";
	if (p_type == &ffi_type_pointer)
		return "*";

	abort(); /* Should never be reached */
	return NULL;
};

/* ================================================================
 * Emscripten-specific implementations of libffi functions
 * ================================================================ */

extern "C" ffi_status
ffi_prep_cif_machdep(ffi_cif *cif)
{
	return FFI_OK;
}

extern "C" void
ffi_call(ffi_cif *p_cif,
         void (*p_fn)(void),
         void *x_rvalue,
         void **x_avalues)
{
	/* Only pay any attention to the "default ABI", which we'll treat as the
	 * Emscripten ABI (even if it isn't). */
	assert(p_cif->abi == FFI_DEFAULT_ABI);

	/* Convert the data type information stored in the cif data to
	 * values that are more easily understood by getValue() and
	 * setValue(). */
	const char *t_rvalue_type_ir = ffi_map_to_llvm_ir(p_cif->rtype);

	const char *t_avalue_types_ir[p_cif->nargs];
	for (unsigned i = 0; i < p_cif->nargs; ++i)
	{
		t_avalue_types_ir[i] = ffi_map_to_llvm_ir(p_cif->arg_types[i]);
	}

	/* Marshal the function and dispatch it, in JavaScript */
	EM_ASM_({
			// ---------- Sensible names for arguments
			var fn = $0;
			var nargs = $1;
			var rvalue_ptr = $2;
			var avalue_ptr = $3;
			var rvalue_type_ptr = $4;
			var avalue_types_ptr = $5;

			var DATA_POINTER_ALIGNMENT = $6;
			var FUNCTION_POINTER_ALIGNMENT = $7;

			var pointer_stringify = Module.Pointer_stringify;
			var getValue = Module.getValue;
			var setValue = Module.setValue;

			var stack = 0;

			// ---------- Get JavaScript function
			// Compute the correct index into the Emscripten runtime
			// function pointer array.
			var index = fn / FUNCTION_POINTER_ALIGNMENT - 1;

			// Get the actual JavaScript function
			var func = Module.Runtime.functionPointers[index];

			/*DEBUG // (no way to hide this behind an #ifdef, unfortunately)
			// Get the function name
			var DBG_func_name;
			for (var key in Module) {
				if (Module.hasOwnProperty(key) && Module[key] == func) {
					DBG_func_name = key;
				}
			}
			var DBG_func_args = [];
			*/

			// ---------- Marshal arguments
			var func_args = [];
			for (var i = 0; i < nargs; ++i) {
				var arg_type_ptr = avalue_types_ptr + i * DATA_POINTER_ALIGNMENT;
				var arg_val_ptr = avalue_ptr + i * DATA_POINTER_ALIGNMENT;
				var arg_type = pointer_stringify(getValue(arg_type_ptr, "*"));

				// Each value in the argument values array is a
				// pointer to the location where the actual argument
				// value is stored.
				var arg_val = getValue(getValue(arg_val_ptr, "*"), arg_type);

				func_args.push(arg_val);

				/*DEBUG
				DBG_func_args.push('(' + arg_type + ')' + arg_val.toString(16));
				*/
			}

			var ret_type = null;
			if (rvalue_type_ptr != 0) {
				ret_type = pointer_stringify(rvalue_type_ptr);
			}

			/*DEBUG
			console.error('ffi_call: ' + fn.toString(16) + '=' + DBG_func_name +
			              ' ' + DBG_func_args);
			*/

			// ---------- Invoke
			stack = Module.Runtime.stackSave();
			var func_retval = func.apply(null, func_args);
			Module.Runtime.stackRestore(stack);

			// ---------- Store return value
			if (ret_type) {
				// the rvalue_ptr is a pointer to a location in which
				// the return value should be saved
				setValue(getValue(rvalue_ptr, "*"), func_retval, ret_type);
			}
		},
		/* 0 */ int(p_fn),
		/* 1 */ int(p_cif->nargs),
		/* 2 */ int(x_rvalue),
		/* 3 */ int(x_avalues),
		/* 4 */ t_rvalue_type_ir,
		/* 5 */ t_avalue_types_ir,
		/* 6 */ DATA_POINTER_ALIGNMENT,
		/* 7 */ FUNCTION_POINTER_ALIGNMENT);
}
