/*                                                              -*-Javascript-*-

Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

	// This class is used to emulate a synchronous LiveCode main loop
	// using emscripten's emterpreter
	$LiveCodeAsync__deps: ['$EmterpreterAsync'],
	$LiveCodeAsync: {
		// true if ensureInit() has ever been run
		_initialised: false,

		// A function that's called to resume the LiveCode main
		// loop.
		_continuation: null,

		// The handle for the setTimeout() handler for the current
		// yield state.
		_timeoutHandle: null,

		// True if the last resume was from a timeout
		_fromTimeout: false,

		_ensureInit: function() {
			// Make sure this only ever gets run once.
			if (LiveCodeAsync._initialised) {
				return;
			}
			LiveCodeAsync._initialised = true;
		},

		_resumeMainLoopTimeout: function() {
			LiveCodeAsync._fromTimeout = true;
			LiveCodeAsync._resumeMainLoop();
		},

		_resumeMainLoop: function() {

			// Make sure we're actually yielding
			assert(LiveCodeAsync._continuation);

			// Cancel the timeout for this yield state
			if (LiveCodeAsync._timeoutHandle) {
				clearTimeout(LiveCodeAsync._timeoutHandle);
			}

			var resume = LiveCodeAsync._continuation;

			// Clear state
			LiveCodeAsync._timeoutHandle = null;
			LiveCodeAsync._continuation = null;

			resume();
		},

		// Yield the main loop; save the execution state and return to
		// the browser.  The main loop will be resumed the next time
		// the browser sends the engine an event.
		//
		// If no event has occurred by the time <timeout> milliseconds
		// elapse, yieldMainLoop() returns false.  Otherwise,
		// yieldMainLoop() returns true.
		yieldMainLoop: function(timeout) {
			LiveCodeAsync._ensureInit();

			// Can't yield recursively
			assert(!LiveCodeAsync._continuation);
			assert(!LiveCodeAsync._timeoutHandle);

			// Suspend execution
			EmterpreterAsync.handle(function(resume) {
				LiveCodeAsync._continuation = resume;

				// Make sure that we get restarted in time; if the
				// timout is negative, never timeout
				LiveCodeAsync._fromTimeout = false;
				if (timeout >= 0) {
					var event = setTimeout(LiveCodeAsync._resumeMainLoopTimeout,
										   timeout);
					LiveCodeAsync._timeoutHandle = event;
				}
			});

			return LiveCodeAsync._fromTimeout;
		},

		isTimedOut: function() {
			LiveCodeAsync._ensureInit()
			return LiveCodeAsync._fromTimeout
		},
	},

	// Yield for up to <timeout> seconds
	MCEmscriptenAsyncYield__deps: ['$LiveCodeAsync'],
	MCEmscriptenAsyncYield: function(timeout) {
		if (!isFinite(timeout)) {
			timeout = -1;
		}
		return LiveCodeAsync.yieldMainLoop(timeout*1000);
	},
})

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
