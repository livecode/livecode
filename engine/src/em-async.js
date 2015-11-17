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

	// This class is used to emulate a synchronous LiveCode main loop
	// using emscripten's emterpreter
	$LiveCodeAsync__deps: ['$EmterpreterAsync'],
	$LiveCodeAsync: {
		// true if ensureInit() has ever been run
		_initialised: false,

		// A function that's called to resume the LiveCode main
		// loop.
		_continuation: null,

		// List of hooks to be run before every resume
		_hooks: [],

		// List of callbacks to be run just before resuming the main
		// loop.
		_preResume: [],

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

			LiveCodeAsync.addResumeHook(function () {
				// Run pre-resume callbacks
				LiveCodeAsync._preResume.forEach(function (c) { c(); });
				// Reset pre-resume callback list
				LiveCodeAsync._preResume = [];
			});
		},

		_resumeTimeout: function() {
			LiveCodeAsync._fromTimeout = true;
			LiveCodeAsync.resume();
		},

		// Resume the main loop
		resume: function(delayed) {
			LiveCodeAsync._ensureInit();

			// For convenience, simplify delay+resume
			if (delayed) {
				LiveCodeAsync.delay(delayed);
			}

			// Don't allow calls to resume() when not actually paused.
			if (!LiveCodeAsync.isPaused()) {
				return;
			}

			// Cancel the timeout for this yield state
			if (LiveCodeAsync._timeoutHandle) {
				clearTimeout(LiveCodeAsync._timeoutHandle);
			}

			var resume = LiveCodeAsync._continuation;

			// Clear state
			LiveCodeAsync._timeoutHandle = null;
			LiveCodeAsync._continuation = null;

			// Resume the state, calling the resume callbacks.  The
			// closure passed to the resume() continuation runs after
			// the emterpreter stack has been restored, but before
			// EmterpreterAsync.handle() "returns".  The return value
			// of the closure determines the apparent return value of
			// EmterpreterAsync.handle().
			resume(function (){
				// Run pre-resume hooks
				LiveCodeAsync._runHooks()

				return !LiveCodeAsync.isTimedOut();
			});
		},

		// Yield the main loop; save the execution state and return to
		// the browser.  The main loop will be resumed the next time
		// the browser sends the engine an event.
		//
		// If no event has occurred by the time <timeout> milliseconds
		// elapse, pause() returns false.  Otherwise,
		// pause() returns true.
		pause: function(timeout) {
			LiveCodeAsync._ensureInit();

			// Suspend execution.  Note that pause() might be called
			// multiple times for a single yield from the engine, but
			// the closure passed to EmterpreterAsync.handle() will
			// only be called once.  This means that all the work
			// needs to be done in the closure.
			return EmterpreterAsync.handle(function(resume) {

				// Can't yield recursively
				assert(!LiveCodeAsync._continuation);
				assert(!LiveCodeAsync._timeoutHandle);
				assert(!LiveCodeAsync._inPreResume);

				LiveCodeAsync._continuation = resume;

				// Make sure that we get restarted in time; if the
				// timout is negative, never timeout
				LiveCodeAsync._fromTimeout = false;
				if (timeout >= 0) {
					var event = setTimeout(LiveCodeAsync._resumeTimeout,
										   timeout);
					LiveCodeAsync._timeoutHandle = event;
				}
			});
		},

		// Test whether the engine is currently paused
		isPaused: function() {
			return (!!LiveCodeAsync._continuation);
		},

		// Test whether the engine is currently being resumed from a
		// timeout.  Returns false if the resume is due to an event.
		isTimedOut: function() {
			LiveCodeAsync._ensureInit()
			return LiveCodeAsync._fromTimeout
		},

		// Add a closure to be run before the engine next resumes
		delay: function(delayed) {
			LiveCodeAsync._ensureInit();

			if (!LiveCodeAsync.isPaused()) {
				// Engine is live, just run the closure
				delayed();
			} else {
				LiveCodeAsync._preResume.push(delayed);
			}
		},

		// Run pre-resume hooks.
		_runHooks: function() {
			LiveCodeAsync._hooks.forEach(function (h) { h(); });
		},

		// Register a closure to be run before every resume.  If
		// <callback> is already in the list of hooks, does nothing.
		//
		// callback: closure taking no arguments.
		addResumeHook: function(callback) {
			LiveCodeAsync._ensureInit();

			// Make sure the same hook doesn't get registered twice
			if (LiveCodeAsync._hooks.some(function (h) {
				return (h === callback);
			})) {
				return;
			}

			LiveCodeAsync._hooks.push(callback);
		},

		// Remove a closure from the list of pre-resume hooks.  If
		// <callback> is not in the list of hooks, does nothing.
		removeResumeHook: function(callback) {
			LiveCodeAsync._ensureInit();

			// Find and remove the specified hook
			var numHooks = LiveCodeAsync._hooks.length;
			for (var i = 0; i < numHooks; i++) {
				if (LiveCodeAsync._hooks[i] === callback) {
					LiveCodeAsync._hooks.splice(i, 1);
				}
			}
		},
	},

	// Yield for up to <timeout> seconds
	MCEmscriptenAsyncYield__deps: ['$LiveCodeAsync'],
	MCEmscriptenAsyncYield: function(timeout) {
		if (!isFinite(timeout)) {
			timeout = -1;
		}
		return LiveCodeAsync.pause(timeout*1000);
	},

	// Resume the engine on event
	MCEmscriptenAsyncResume__deps: ['$LiveCodeAsync'],
	MCEmscriptenAsyncResume: function() {
		LiveCodeAsync.resume();
	},

	// Delay a closure until the next time the engine resumes
	MCEmscriptenAsyncDelay__deps: ['$LiveCodeAsync'],
	MCEmscriptenAsyncDelay: function(delayed) {
		LiveCodeAsync.delay(delayed);
	},
});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
