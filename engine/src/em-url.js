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

	$LiveCodeUrl__deps: ['$LiveCodeAsync', '$LiveCodeUtil'],
	$LiveCodeUrl: {
		// true if _ensureInit() has ever been run
		_initialised: false,

		_ensureInit: function() {
			// Make sure this only ever gets run once.
			if (LiveCodeUrl._initialised) {
				return;
			}
			LiveCodeUrl._initialised = true;
		},

		// ----------------------------------------------------------------
		// Wrappers for C++ functions
		// ----------------------------------------------------------------

		_postStatusStarted: function(callbackPtr, contextPtr) {
			Module.ccall('MCEmscriptenUrlCallbackStarted',
			             null,
			             ['number', // MCSystemUrlCallback p_callback,
			              'number'], // void *p_context
			             [callbackPtr, contextPtr]);
		},

		_postStatusProgress: function(loadedLength, totalLength,
		                              callbackPtr, contextPtr) {
			Module.ccall('MCEmscriptenUrlCallbackProgress',
			             null,
			             ['number', // uint32_t p_loaded_length
			              'number', // int32_t p_total_length
			              'number', // MCSystemUrlCallback p_callback
			              'number'], // void *p_context
			             [loadedLength, totalLength, callbackPtr, contextPtr]);
		},

		_postStatusFinished: function(data, callbackPtr, contextPtr) {
			// Copy the request data into the C++ heap as a byte array
			// FIXME maybe this needs a helper function in LiveCodeUtil
			var dataStack = new Uint8Array(data);
			var dataLength = dataStack.length;
			var dataPtr = Module._malloc(dataLength *
			                             dataStack.BYTES_PER_ELEMENT);
			var dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr,
			                              dataLength);
			dataHeap.set(dataStack);

			Module.ccall('MCEmscriptenUrlCallbackFinished',
			             null,
			             ['number', // const byte_t *p_data
			              'number', // uint32_t p_data_length
			              'number', // MCSystemUrlCallback p_callback
			              'number'], // void *p_context
			             [dataPtr, dataLength, callbackPtr, contextPtr]);

			Module._free(dataPtr);
		},

		_postStatusError: function(errorString, callbackPtr, contextPtr) {
			var errorPtr = LiveCodeUtil.stringToUTF16(errorString);
			var errorLength = errorString.length;

			Module.ccall('MCEmscriptenUrlCallbackError',
			             null,
			             ['number', // const unichar_t *p_error
			              'number', // uint32_t p_error_length
			              'number', // MCSystemUrlCallback p_callback
			              'number'], // void *p_context
			             [errorPtr, errorLength, callbackPtr, contextPtr]);

			Module._free(errorPtr);
		},

		// ----------------------------------------------------------------
		// Loading
		// ----------------------------------------------------------------

		load: function(urlPtr, urlLength,
		               headersPtr, headersLength,
		               timeout,
		               callbackPtr, contextPtr) {

			// Turn the url & headers into JavaScript strings
			var url = LiveCodeUtil.stringFromUTF16(urlPtr, urlLength);
			var headers = LiveCodeUtil.stringFromUTF16(headersPtr, headersLength);

			// ---------- 1. Create request instance
			var xhr = new XMLHttpRequest();

			// ---------- 2. Install callbacks

			// There are some browsers and conditions in which neither
			// the "error", "abort" nor "load" handlers will be
			// dispatched.  Use the loadend handler as a fallback to
			// detect this case.
			var loadendHandler = function(e) {
				LiveCodeAsync.resume(function() {
					LiveCodeUrl._postStatusError("Request failed",
					                             callbackPtr,
					                             contextPtr);
				});
			};
			var clearLoadendHandler = function() {
				xhr.removeEventListener("loadend", loadendHandler, false);
			};
			xhr.addEventListener("loadend", loadendHandler, false);

			xhr.addEventListener("abort", function(e) {
				clearLoadendHandler();
				LiveCodeAsync.resume(function() {
					LiveCodeUrl._postStatusError("Request aborted",
					                             callbackPtr,
					                             contextPtr);
				});
			}, false);

			xhr.addEventListener("error", function(e) {
				clearLoadendHandler();
				LiveCodeAsync.resume(function() {
					LiveCodeUrl._postStatusError("Request failed",
					                             callbackPtr,
					                             contextPtr);
				});
			}, false);

			xhr.addEventListener("load", function(e) {
				clearLoadendHandler();
				LiveCodeAsync.resume(function() {
					LiveCodeUrl._postStatusFinished(xhr.response,
					                                callbackPtr,
					                                contextPtr);
				});
			}, false);

			xhr.addEventListener("loadstart", function(e) {
				LiveCodeAsync.resume(function() {
					LiveCodeUrl._postStatusStarted(callbackPtr,
					                               contextPtr);
				});
			}, false);

			xhr.addEventListener("progress", function(e) {
				var totalLength = -1;
				if (e.lengthComputable) {
					totalLength = e.total;
				}
				LiveCodeAsync.resume(function() {
					LiveCodeUrl._postStatusProgress(e.loaded,
					                                totalLength,
					                                callbackPtr,
					                                contextPtr);
				});
			}, false);

			// ---------- 3. Set request parameters

			// Target URL
			xhr.open("GET", url);

			// Headers
			//
			// Split each line in the request headers into a header
			// name and value.
			//
			// FIXME this isn't quite correct; we don't correctly
			// handle continuation lines in the headers text.
			var headerLines = headers.match(/[^\r\n]+/g);
			if (headerLines != null) {
				var numHeaders = headerLines.length;
				for (var i = 0; i < numHeaders; ++i) {
					var re = /^([^:])*:(.*)$/;
					var headerFields = re.exec(headerLines[i]);
					if (headerFields.length != 3) {
						console.log("Ignoring bad HTTP header '" +
						            headerLines[i] + "'");
						continue;
					}

					xhr.setRequestHeader(headerFields[1], headerFields[2]);
				}
			}

			// Timeout
			xhr.timeout = timeout * 1000;

			// We want an array buffer response (no string decoding
			// etc.)
			xhr.responseType = "arraybuffer";

			// ---------- 4. Execute request
			xhr.send();

			return true;
		},
	},

	// Start loading a URL
	//
	//   urlPtr, urlLength:         URL to be loaded
	//   headersPtr, headersLength: Custom request headers
	//   timeout:                   Socket timeout in milliseconds
	//   callbackPtr, contextPtr:   Callback for request status updates
	MCEmscriptenUrlLoad__deps: ['$LiveCodeUrl'],
	MCEmscriptenUrlLoad: function(urlPtr, urlLength,
	                              headersPtr, headersLength,
	                              timeout,
	                              callbackPtr, contextPtr) {

		return LiveCodeUrl.load(urlPtr, urlLength,
		                        headersPtr, headersLength,
		                        timeout,
		                        callbackPtr, contextPtr);
	},
});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
