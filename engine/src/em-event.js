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

	$LiveCodeEvents__deps: ['$LiveCodeAsync'],
	$LiveCodeEvents: {

		// true if ensureInit() has ever been run
		_initialised: false,

		// contains mouse event data
		_mouseEvent: null,

		// This function is used to call a function for each event
		// type to handler mapping defined for LiveCode on Emscripten.
		//
		// The <func> must take two arguments: the event type, and the
		// handler function used to handle that event type.
		_eventForEach: function(func) {

			// Master mapping from event types to handler functions.
			var mapping = [
				['focus', LiveCodeEvents._handleFocusEvent],
				['blur', LiveCodeEvents._handleFocusEvent],

				['mousemove', LiveCodeEvents._handleMouseEvent],
				['mousedown', LiveCodeEvents._handleMouseEvent],
				['mouseup', LiveCodeEvents._handleMouseEvent],
				['mouseenter', LiveCodeEvents._handleMouseEvent],
				['mouseleave', LiveCodeEvents._handleMouseEvent],

				// FIXME "keypress" events are deprecated
				['keypress', LiveCodeEvents._handleKeyboardEvent],

				['input', LiveCodeEvents._handleKeyboardEvent],
			];

			var mapLength = mapping.length;
			for (var i = 0; i < mapLength; i++) {
				func(mapping[i][0], mapping[i][1]);
			}
		},

		initialize: function() {
			// Make sure this only ever gets run once
			if (LiveCodeEvents._initialised) {
				return;
			}

			console.log('LiveCodeEvents.initialize()');

			var target = LiveCodeEvents._getTarget();

			// Add all of the event handlers
			LiveCodeEvents._eventForEach(function (type, handler) {
				console.log('    - ' + type);
				target.addEventListener(type, handler, true);
			});

			// Make sure the canvas is treated as focusable...
			target.tabIndex = 0;

			LiveCodeEvents._initialised = false;
		},

		finalize: function() {
			if (!LiveCodeEvents._initialised) {
				return;
			}

			console.log('LiveCodeEvents.finalize()');

			var target = LiveCodeEvents._getTarget();

			// Remove all of the event handlers
			LiveCodeEvents._eventForEach(function (type, handler) {
				console.log('    - ' + type);
				target.removeEventListener(type, handler, true);
			});

			LiveCodeEvents._initialised = false;;
		},

		_getTarget: function() {
			// Handlers are attached to the default canvas
			return Module['canvas'];
		},

		_getStack: function() {
			return Module.ccall('MCEmscriptenGetCurrentStack', 'number', [], []);
		},

		_encodeModifiers: function(uiEvent) {
			return Module.ccall('MCEmscriptenEventEncodeModifiers', 'number',
								['number', 'number', 'number', 'number'],
								[uiEvent.shiftKey, uiEvent.altKey,
								 uiEvent.ctrlKey, uiEvent.metaKey]);
		},

		// ----------------------------------------------------------------
		// Focus events
		// ----------------------------------------------------------------

		// Wrapper for MCEventQueuePostKeyFocus()
		_postKeyFocus: function(stack, owner) {
			Module.ccall('MCEventQueuePostKeyFocus',
						 'number', // bool
						 ['number',  // MCStack *stack
						  'number'], // bool owner
						 [stack, owner]);
		},

		_handleFocusEvent: function(e) {
			LiveCodeAsync.delay(function() {
				var stack = LiveCodeEvents._getStack();

				switch (e.type) {
				case 'focus':
				case 'focusin':
					LiveCodeEvents._postKeyFocus(stack, true);
					break;
				case 'blur':
				case 'focusout':
					LiveCodeEvents._postKeyFocus(stack, false);
					break;
				default:
					console.debug('Unexpected focus event type: ' + e.type);
					return;
				}
			});
			LiveCodeAsync.resume();

			// Prevent event from propagating
			return false;
		},

		// ----------------------------------------------------------------
		// Keyboard events
		// ----------------------------------------------------------------

		// Generate the char_code argument needed by
		// MCEventQueuePostKeyPress() from JavaScript's
		// KeyboardEvent.key.
		_encodeKeyboardCharCode: function(keyboardEvent) {
			var key = keyboardEvent.key;
			var high = key.charCodeAt(0); // High surrogate
			var low = key.charCodeAt(1); // Low surrogate

			// Check if there's actually a key code at all
			if (isNaN(high)) {
				return 0;
			}

			// If the first and only character in the key is a BMP
			// character, then use it as the char code.  If it's not
			// the only character, then the key must be a named key
			// (i.e. non-printing).
			if (0xD800 > high)
			{
				if (key.length == 1) {
					return high;
				} else {
					return 0;
				}
			}

			// If the first character is a high surrogate, and the
			// subsequent character is a low surrogate, then combine
			// them into a character code.  If the low surrogate is
			// missing, then assume the key must be a named key.
			if (0xD800 <= high && high <= 0xDBFF) {
				if (isNan(low)) {
					console.debug('High surrogate not followed by low surrogate');
					return 0;
				}
				if (key.length == 2) {
					return ((high - 0xD800) * 0x400) + (low - 0xDC00) + 0x10000;
				} else {
					return 0;
				}
			}

			return 0;
		},

		// Generate the key_code argument needed by
		// MCEventQueuePostKeyPress() from JavaScript's
		// KeyboardEvent.key.
		_encodeKeyboardKeyCode: function(keyboardEvent) {
			// Synthesize a keycode from the char code, if the key event has a
			// corresponding char code.
			// FIXME Maybe this should be done in the engine?
			var char_code = LiveCodeEvents._encodeKeyboardCharCode(keyboardEvent);

			// Unicode codepoints in the ISO/IEC 8859-1 range
			// U+0020..U+00FF are passed directly as keycodes.
			// Otherwise, the codepoint is returned with bit 21 set.
			if (char_code > 0) {
				if (char_code >= 0x20 && char_code <= 0xff) {
					return char_code;
				} else {
					return char_code & 0x1000000;
				}
			}

			console.debug('Don\'t know how to decode key: ' + keyboardEvent.key);
			return 0;
		},

		// Wrapper for MCEventQueuePostKeyPress()
		_postKeyPress: function(stack, modifiers, char_code, key_code) {
			Module.ccall('MCEventQueuePostKeyPress',
						 'number', // bool
						 ['number',  // MCStack *stack
						  'number',  // uint32_t modifiers
						  'number',  // uint32_t char_code
						  'number'], // uint32_t key_code
						 [stack, modifiers, char_code, key_code]);
		},

		_handleKeyboardEvent: function(e) {
			LiveCodeAsync.delay(function() {

				var stack = LiveCodeEvents._getStack();
				var mods = LiveCodeEvents._encodeModifiers(e);

				switch (e.type) {
				case 'keypress':
				case 'keyup':
					var char_code = LiveCodeEvents._encodeKeyboardCharCode(e);
					var key_code = LiveCodeEvents._encodeKeyboardKeyCode(e);
					LiveCodeEvents._postKeyPress(stack, mods, char_code, key_code);
					console.debug(e.type + ' ' + e.key + ': ' + char_code + '/' + key_code);
					break;
				default:
					console.debug('Unexpected keyboard event type: ' + e.type);
					return;
				}
			});
			LiveCodeAsync.resume();

			// Prevent event from propagating
			e.preventDefault();
			return false;
		},

		// ----------------------------------------------------------------
		// Mouse events
		// ----------------------------------------------------------------

		// Encode a mouse state using enum MCMousePressState.  You
		// need to update this code if you change the enumeration...
		_encodeMouseState: function(state) {
			switch (state) {
			case 'mouseup':
				return 0; /* kMCMousePressStateUp */
			case 'mousedown':
				return 1; /* kMCMousePressStateDown */
			default:
				return 2; /* kMCMousePressStateRelease */
			}
		},

		// Convert event coordinates to logical coordinates -- these
		// are in units of CSS pixels relative to the top left of the
		// target
		_encodeMouseCoordinates: function(mouseEvent) {
			var target = mouseEvent.target;
			var x = mouseEvent.clientX - target.getBoundingClientRect().left -
				target.clientLeft + target.scrollLeft;
			var y = mouseEvent.clientY - target.getBoundingClientRect().top -
				target.clientTop + target.scrollTop;

			return [x, y];
		},

		// Wrapper for MCEventQueuePostMousePosition
		_postMousePosition: function(stack, time, modifiers, x, y) {
			Module.ccall('MCEventQueuePostMousePosition',
						 'number', /* bool */
						 ['number', /* MCStack *stack */
						  'number', /* uint32_t time */
						  'number', /* uint32_t modifiers */
						  'number', 'number'], /* uint32_t x, y */
						 [stack, time, modifiers, x, y]);
		},

		// Wrapper for MCEventQueuePostMousePress
		_postMousePress: function(stack, time, modifiers, state, button)
		{
			Module.ccall('MCEventQueuePostMousePress',
						 'number', /* bool */
						 ['number', /* MCStack *stack */
						  'number', /* uint32_t time */
						  'number', /* uint32_t modifiers */
						  'number', /* MCMousePressState state */
						  'number'], /* int32_t button */
						 [stack, time, modifiers, state, button]);
		},

		// Wrapper for MCEventQueuePostMouseFocuse
		_postMouseFocus: function(stack, time, inside)
		{
			Module.ccall('MCEventQueuePostMouseFocus',
						 'number', /* bool */
						 ['number', /* MCStack *stack */
						  'number', /* uint32_t time */
						  'number'], /* bool inside */
						 [stack, time, inside]);
		},

		_handleMouseEvent: function(e) {
			LiveCodeAsync.delay(function () {

				var stack = LiveCodeEvents._getStack();
				var mods = LiveCodeEvents._encodeModifiers(e);
				var pos = LiveCodeEvents._encodeMouseCoordinates(e);

				// Always post the mouse position
				LiveCodeEvents._postMousePosition(stack, e.timestamp, mods,
												  pos[0], pos[1]);

				switch (e.type) {
				case 'mousemove':
					return;

				case 'mousedown':
					// In the case of mouse down, specifically request
					// keyboard focus
					LiveCodeEvents._getTarget().focus();

					// Intentionally fall through to 'mouseup' case.
				case 'mouseup':
					var state = LiveCodeEvents._encodeMouseState(e.type);
					LiveCodeEvents._postMousePress(stack, e.timestamp, mods,
												   state, e.button);
					break;

				case 'mouseenter':
					LiveCodeEvents._postMouseFocus(stack, e.timestamp, true);
					break;

				case 'mouseleave':
					LiveCodeEvents._postMouseFocus(stack, e.timestamp, false);
					break;

				default:
					console.debug('Unexpected mouse event type: ' + e.type);
					return;
				}

			});
			LiveCodeAsync.resume();

			// Prevent event from propagating
			e.preventDefault();
			return false;
		},
	},

	MCEmscriptenEventInitializeJS__deps: ['$LiveCodeEvents'],
	MCEmscriptenEventInitializeJS: function() {
		LiveCodeEvents.initialize();
		return true;
	},

	MCEmscriptenEventFinalizeJS__deps: ['$LiveCodeEvents'],
	MCEmscriptenEventFinalizeJS: function() {
		LiveCodeEvents.finalize();
	},

});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
