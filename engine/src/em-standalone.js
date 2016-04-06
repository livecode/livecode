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

	// Return the standalone capsule data.  This should have been
	// downloaded before the engine started (see em-preamble.js).
	//
	// bufferPtr -> pointer at which to store the data buffer
	// lengthPtr -> pointer at which to store the data length
	//
	// Returns false if for some reason the standalone capsule is
	// missing.
	MCEmscriptenStandaloneGetDataJS: function(bufferPtr, lengthPtr) {

		// Check that the data was downloaded
		var xhr = Module['livecodeStandaloneRequest'];

		if (!xhr ||
		    !xhr.response ||
		    typeof xhr.response !== 'object' ||
		    !xhr.response.byteLength) {
			console.error('LiveCode standalone data is missing!');
			return false;
		}

		// Copy the data into the C++ heap
		// FIXME maybe this needs a helper function in LiveCodeUtil
		var dataStack = new Uint8Array(xhr.response);
		var dataLength = dataStack.length;
		var dataPtr = Module._malloc(dataLength *
		                             dataStack.BYTES_PER_ELEMENT);
		var dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr,
		                              dataLength);
		dataHeap.set(dataStack);

		// Set the return parameters
		{{{ makeSetValue('bufferPtr', '0', 'dataPtr', 'i32') }}};
		{{{ makeSetValue('lengthPtr', '0', 'dataLength', 'i32') }}};

		return true;
	},
});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
