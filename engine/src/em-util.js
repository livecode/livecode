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
	},
});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
