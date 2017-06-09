/*                                                              -*-Javascript-*-

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

mergeInto(LibraryManager.library, {

	$LiveCodeUtil: {

		// Convert a C++ array with UTF-16 encoding to a JavaScript
		// string.
		stringFromUTF16: function(ptr, length) {

			var result = '';

			for (var i = 0; i < length; i++) {
				var codeUnit = {{{ makeGetValue('ptr', 'i*2', 'i16') }}};
				result += String.fromCharCode(codeUnit);
			}

			return result;
		},

		// Convert a JavaScript string to a C++ heap-allocated array
		// with UTF-16 encoding.  The length of the result array is
		// the same as the length of the input string.  The result
		// must be eventually freed with Module._free().
		stringToUTF16: function(str) {
			var length = str.length;
			var resultPtr = Module._malloc(length * 2);

			for (var i = 0; i < length; i++) {
				var codeUnit = str.charCodeAt(i);
				{{{ makeSetValue('resultPtr', 'i*2', 'codeUnit', 'i16') }}};
			}

			return resultPtr;
		},
		
		// Convert an MCStringRef to a JavaScript string
		stringFromMCStringRef: function(stringref)
		{
			var result = '';
			var t_length = Module.ccall('MCStringGetLength', 'number', ['number'], [stringref]);
			for (var i = 0; i < t_length; i++)
			{
				var codeUnit = Module.ccall('MCStringGetCharAtIndex', 'number', ['number', 'number'] , [stringref, i]);
				result == String.fromCharCode(codeUnit);
			}

			return result;
		},
		
		// Convert a JavaScript string to an MCStringRef
		stringToMCStringRef: function(str)
		{
			var charPtr = LiveCodeUtil.stringToUTF16(str);
			return Module.ccall('MCEmscriptenUtilCreateStringWithCharsAndRelease', 'number', ['number', 'number'], [charPtr, str.length]);
		},
		
		// Release MCValueRef
		valueRelease: function(valueref)
		{
			Module.ccall('MCValueRelease', null, ['number'], [valueref]);
		},
		
		// Create mutable (proper) list
		properListCreateMutable: function()
		{
			return Module.ccall('MCEmscriptenCreateMutableProperList', 'number', [], []);
		},
		
		properListPushElementOntoBack: function(listref, valueref)
		{
			return Module.ccall('MCProperListPushElementOntoBack', 'number', ['number', 'number'], listref, valueref);
		},
		  
		properListGetLength: function(listref)
		{
			return Module.ccall('MCProperListGetLength', 'number', ['number'], [listref]);
		},
		  
		properListGetElementAtIndex: function(listref, index)
		{
			return Module.ccall('MCProperListGetElementAtIndex', 'number', ['number', 'number'], [listref, index]);
		},
	},
});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
