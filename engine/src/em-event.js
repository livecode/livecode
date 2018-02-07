/*                                                              -*-Javascript-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

	$LiveCodeEvents__deps: ['$LiveCodeAsync', '$LiveCodeDC'],
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
				['keyup', LiveCodeEvents._handleKeyboardEvent],
				['keydown', LiveCodeEvents._handleKeyboardEvent],

				['input', LiveCodeEvents._handleInput],
				['beforeinput', LiveCodeEvents._handleInput],

				['compositionstart', LiveCodeEvents._handleComposition],
				['compositionupdate', LiveCodeEvents._handleComposition],
				['compositionend', LiveCodeEvents._handleComposition],
				
				['contextmenu', LiveCodeEvents._handleContextMenu],
			];

			var mapLength = mapping.length;
			for (var i = 0; i < mapLength; i++) {
				func(mapping[i][0], mapping[i][1]);
			}
		},

		addEventListeners: function(pElement)
		{
			LiveCodeEvents._eventForEach(function(type, handler) {
				pElement.addEventListener(type, handler, true);
			});

			// Make sure the canvas is treated as focusable...
			pElement.tabIndex = 0;

			// Make it a target for text input events
			pElement.setAttribute('contentEditable', 'true');

			// Force the canvas to use a normal mouse cursor by
			// default
			pElement.style.cursor = 'default';
		},
		
		removeEventListeners: function(pElement)
		{
			// Remove all of the event handlers
			LiveCodeEvents._eventForEach(function (type, handler) {
				pElement.removeEventListener(type, handler, true);
			});
		},
		
		initialize: function() {
			// Make sure this only ever gets run once
			if (LiveCodeEvents._initialised) {
				return;
			}

			// Add document event listeners to track mouse events outside canvas
			document.addEventListener("mouseup", LiveCodeEvents._handleDocumentMouseEvent);
			document.addEventListener("mousemove", LiveCodeEvents._handleDocumentMouseEvent);
			
			LiveCodeEvents._initialised = true;
		},

		finalize: function() {
			if (!LiveCodeEvents._initialised) {
				return;
			}

			LiveCodeEvents._initialised = false;;
		},
		
		_getStackForWindow: function(pWindow) {
			return Module.ccall('MCEmscriptenGetStackForWindow', 'number',
								['number'],
								[pWindow]);
		},
		
		_getStackForCanvas: function(pCanvas) {
			var window = LiveCodeDC.getWindowIDForCanvas(pCanvas);
			if (window == 0)
			{
				console.log('failed to find window for canvas');
				return null;
			}
			return LiveCodeEvents._getStackForWindow(window);
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
				var stack = LiveCodeEvents._getStackForCanvas(e.target);
				// ignore events for non-lc elements
				if (!stack)
					return;

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

		// Converts a string in the form 'U+xxxx' into a codepoint number
		_parseCodepointString: function(string) {
			var codepointString = string.match(/^U\+([0-9a-fA-F]+)$/);
			if (codepointString) {
				var codepointNum = parseInt(codepointString[1], 16);
				if (codepointNum > 0xff) {
					return 0x01000000 | codepointNum;
				} else if (codepointNum === 0x08 || codepointNum === 0x09 || codepointNum < 0x80) {
					return codepointNum;
				} else {
					// Values outside the range HT,BS,0x20-0x7f tend to depend
					// on the browser and aren't reliable
					return 0;
				}
			}
			return 0;
		},

		// Converts ASCII control characters to their X11 key equivalents
		// or returns zero if not a control
		_controlToX11Key: function(codepoint) {
			switch (codepoint) {
				case 0x0008:	// Horizontal tab
				case 0x0009:	// Backspace
					return 0xff00 + codepoint;

				case 0x007f:	// Delete
					return 0xffff;

				default:		// Unrecognised
					return 0;
			}
		},

		// Generate the char_code argument needed by
		// MCEventQueuePostKeyPress() from JavaScript's
		// KeyboardEvent.key.
		_encodeKeyboardCharCode: function(keyboardEvent) {
			// Not all browsers implement the KeyboardEvent interface fully;
			// we may have to fall back to an older interface if not defined
			var key, high, low;
			if ('key' in keyboardEvent) {
				key = keyboardEvent.key;
				high = key.charCodeAt(0);
				low = key.charCodeAt(1);
			} else {
				// Browser uses the old-style key events. Just take whatever
				// charCode it has supplied (if any!)
				if (keyboardEvent.charCode !== 0) {
					return keyboardEvent.charCode;
				} else {
					// Try parsing the keyIdentifier
					return LiveCodeEvents._parseCodepointString(keyboardEvent.keyIdentifier);
				}
			}

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

			// Non-control Unicode codepoints in the ISO/IEC 8859-1 range
			// U+0020..U+00FF are passed directly as keycodes.
			// Otherwise, the codepoint is returned with bit 21 set.
			if (char_code >= 0x20) {
				if ((char_code >= 0x20 && char_code < 0x7f) || (char_code >= 0xa0 && char_code <= 0xff)) {
					return char_code;
				} else if (char_code > 0xff) {
					return char_code | 0x1000000;
				} else {
					// Control character -- handled below
				}
			}

			// If the 'key' property isn't defined, the old-style keyboard events
			// are being used and we need to convert any ASCII control characters
			// into the corresponding XK_ code
			if (!('key' in keyboardEvent) && char_code !== 0) {
				return LiveCodeEvents._controlToX11Key(char_code);
			}

			// String describing the key. This is stored in the 'key' property for
			// new-style key events and 'keyIdentifier' for old-style events.
			var keyName;
			if ('key' in keyboardEvent) {
				keyName = keyboardEvent.key;
			} else {
				keyName = keyboardEvent.keyIdentifier;
			}

			// Otherwise, decode to an X11 keycode
			// See also DOM Level 3 KeyboardEvent key Values
			// <https://w3c.github.io/DOM-Level-3-Events-key/#key-value-tables>
			//
			// Conforms to W3C Editor's Draft 27 April 2015
			//
			// Not all of these keycodes are actually understood by
			// the engine, but they're all included for completeness.
			switch (keyName) {

				// Special Key Values
			case 'Unidentified':	return 0;
			case 'Dead':			return 0; // Dead keys for IME composition

				// Whitespace Keys
			case 'Enter':     return 0xff0d; // XK_Return
			case 'Separator': return 0xffac; // XK_KP_Separator
			case 'Tab':       return 0xff09; // XK_Tab

				// Navigation Keys
			case 'Down':
			case 'ArrowDown':  return 0xff54; // XK_Down
			case 'Left':
			case 'ArrowLeft':  return 0xff51; // XK_Left
			case 'Right':
			case 'ArrowRight': return 0xff53; // XK_Right
			case 'Up':
			case 'ArrowUp':    return 0xff52; // XK_Up
			case 'End':        return 0xff57; // XK_End
			case 'Home':       return 0xff50; // XK_Home
			case 'PageDown':   return 0xff56; // XK_Page_Down
			case 'PageUp':     return 0xff55; // XK_Page_Up

				// Editing Keys
			case 'Backspace': return 0xff08; // XK_BackSpace
			case 'Clear':     return 0xff0b; // XK_Clear
			case 'Copy':      return 0x1008ff57; // GDK_KEY_Copy
			case 'CrSel':     break;
			case 'Cut':       return 0x1008ff58; // GDK_KEY_Cut
			case 'Delete':    return 0xffff; // XK_Delete
			case 'EraseEof':  break;
			case 'ExSel':     break;
			case 'Insert':    return 0xff63; // XK_Insert
			case 'Paste':     return 0x1008ff6d; // GDK_KEY_Paste
			case 'Redo':      return 0xff66; // XK_Redo
			case 'Undo':      return 0xff65; // XK_Undo

				// UI Keys
			case 'Accept':      break;
			case 'Again':       return 0xff66; // XK_Redo
			case 'Attn':        break;
			case 'Cancel':      return 0xff69; // XK_Cancel
			case 'ContextMenu': return 0xff67; // XK_Menu
			case 'Escape':      return 0xff1b; // XK_Escape
			case 'Execute':     return 0xff62; // XK_Execute
			case 'Find':        return 0xff68; // XK_Find
			case 'Help':        return 0xff6a; // XK_Help
			case 'Pause':       return 0xff13; // XK_Pause
			case 'Play':        break;
			case 'Props':       break;
			case 'Select':      return 0xff60; // XK_Select
			case 'ZoomIn':      return 0x1008ff8b; // GDK_KEY_ZoomIn
			case 'ZoomOut':     return 0x1008ff8c; // GDK_KEY_ZoomOut

				// Device Keys
			case 'BrightnessDown': return 0x1008ff03; // GDK_KEY_MonBrightnessDown
			case 'BrightnessUp':   return 0x1008ff02; // GDK_KEY_MonBrightnessUp
			case 'Camera':         break;
			case 'Eject':          return 0x1008ff2c; // GDK_KEY_Eject
			case 'LogOff':         return 0x1008ff61; // GDK_KEY_LogOff
			case 'Power':          break;
			case 'PowerOff':       return 0x1008ff2a; // GDK_KEY_PowerOff
			case 'PrintScreen':    return 0xff15; // XK_Sys_Req
			case 'Hibernate':      return 0x1008ffa8; // GDK_KEY_Hibernate
			case 'Standby':        return 0x1008ff10; // GDK_KEY_Standby
			case 'WakeUp':         return 0x1008ff2b; // GDK_KEY_WakeUp

				// IME and Composition Keys
			case 'AllCandidates':     return 0xff3d; // XK_MultipleCandidate
			case 'Alphanumeric':      return 0xff2f; // ?? XK_Eisu_Shift
			case 'CodeInput':         return 0xff37; // XK_Codeinput
			case 'Compose':           return 0xff20; // XK_Multi_key
			case 'Convert':           return 0xff23; // ?? XK_Henkan
			case 'Dead':              break;
			case 'FinalMode':         break;
			case 'GroupFirst':        return 0xfe0c; // XK_ISO_First_Group
			case 'GroupLast':         return 0xfe0e; // XK_ISO_Last_Group
			case 'GroupNext':         return 0xfe08; // XK_ISO_Next_Group
			case 'GroupPrevious':     return 0xfe0a; // XK_ISO_Prev_Group
			case 'ModeChange':        break;
			case 'NextCandidate':     break;
			case 'PreviousCandidate': return 0xff3c; // XK_PreviousCandidate
			case 'Process':           break;
			case 'SingleCandidate':   return 0xff3c; // XK_SingleCandidate

			case 'HangulMode':        return 0xff31; // XK_Hangul
			case 'HanjaMode':         break;
			case 'JunjaMode':         break;

			case 'Eisu':             return 0xff2f; // XK_Eisu_toggle
			case 'Hankaku':          return 0xff29; // XK_Hankaku
			case 'Hiragana':         return 0xff25; // XK_Hiragana
			case 'HiraganaKatakana': return 0xff27; // XK_Hiragana_Katakana
			case 'KanaMode':         return 0xff2d; // XK_Kana_Lock
			case 'KanjiMode':        return 0xff21; // XK_Kanji
			case 'Katakana':         return 0xff26; // XK_Katakana
			case 'Romaji':           return 0xff24; // XK_Romaji
			case 'Zenkaku':          return 0xff28; // XK_Zenkaku
			case 'ZenkakuHankaku':   return 0xff2a; // XK_Zenkaku_Hankaku

				// General-Purpose Function Keys
				//
				// F1..F12 etc. are handled separately, after this
				// switch statement.

				// Multimedia Keys
			case 'Close':              return 0x1008ff56; // GDK_KEY_Close
			case 'MailForward':        return 0x1008ff90; // GDK_KEY_MailForward
			case 'MailReply':          return 0x1008ff72; // GDK_KEY_Reply
			case 'MailSend':           return 0x1008ff7b; // GDK_KEY_Send
			case 'MediaPlayPause':     return 0x1008ff31; // GDK_KEY_AudioPause
			case 'MediaSelect':        return 0x1008ff32; // GDK_KEY_AudioMedia
			case 'MediaStop':          return 0x1008ff15; // GDK_KEY_AudioStop
			case 'MediaTrackNext':     return 0x1008ff17; // GDK_KEY_AudioNext
			case 'MediaTrackPrevious': return 0x1008ff16; // GDK_KEY_AudioPrev
			case 'New':                return 0x1008ff68; // GDK_KEY_New
			case 'Open':               return 0x1008ff6b; // GDK_KEY_Open
			case 'Print':              break;
			case 'Save':               return 0x1008ff77; // GDK_KEY_Save
			case 'SpellCheck':         return 0x1008ff7c; // GDK_KEY_Spell
			case 'VolumeDown':         return 0x1008ff13; // GDK_KEY_AudioRaiseVolume
			case 'VolumeUp':            return 0x1008ff11; // GDK_KEY_AudioLowerVolume
			case 'VolumeMute':          return 0x1008ff12; // GDK_KEY_AudioMute

				// Application Keys
			case 'LaunchCalculator':    return 0x1008ff1d; // GDK_KEY_Calculator
			case 'LaunchCalendar':      return 0x1008ff20; // GDK_KEY_Calendar
			case 'LaunchMail':          return 0x1008ff19; // GDK_KEY_Mail
			case 'LaunchMediaPlayer':   return 0x1008ff53; // GDK_KEY_CD
			case 'LaunchMusicPlayer':   return 0x1008ff92; // GDK_KEY_Music
			case 'LaunchMyComputer':    return 0x1008ff33; // GDK_KEY_MyComputer
			case 'LaunchScreenSaver':   return 0x1008ff2d; // GDK_KEY_ScreenSaver
			case 'LaunchSpreadsheet':   return 0x1008ff5c; // GDK_KEY_Excel
			case 'LaunchWebBrowser':    return 0x1008ff2e; // GDK_KEY_WWW
			case 'LaunchWebCam':        return 0x1008ff8f; // GDK_KEY_WebCam
			case 'LaunchWordProcessor': return 0x1008ff89; // GDK_KEY_Word

				// Browser Keys
			case 'BrowserBack':      return 0x1008ff26; // GDK_KEY_Back
			case 'BrowserFavorites': return 0x1008ff30; // GDK_KEY_Favorites
			case 'BrowserForward':   return 0x1008ff27; // GDK_KEY_Forward
			case 'BrowserHome':      return 0x1008ff18; // GDK_KEY_HomePage
			case 'BrowserRefresh':   return 0x1008ff29; // GDK_KEY_Refresh
			case 'BrowserSearch':    return 0x1008ff1b; // GDK_KEY_Search
			case 'BrowserStop':      return 0x1008ff28; // GDK_KEY_Stop

				// Media Controller keys
				//
				// FIXME not supported

			default: break;
			}

			// General-Purpose Function keys
			var functionKey = keyName.match(/^F(\d+)$/);
			if (functionKey) {
				var functionNum = parseInt(functionKey[1]);
				if (functionNum >= 1 && functionNum <= 35) {
					return 0xffbd + functionNum; // XK_F...
				}
			}

			// Keys with Unicode codepoint names (U+xxxx)
			var codepoint = LiveCodeEvents._parseCodepointString(keyName);
			if (codepoint !== 0) {
				// Is this a control character?
				if (codepoint < 0x20 || codepoint === 0x7f) {
					return LiveCodeEvents._controlToX11Key(codepoint);
				} else {
					return codepoint;
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

				var stack = LiveCodeEvents._getStackForCanvas(e.target);
				var mods = LiveCodeEvents._encodeModifiers(e);

				// ignore events for non-lc elements
				if (!stack)
					return;

				switch (e.type) {
				case 'keypress':
					var char_code = LiveCodeEvents._encodeKeyboardCharCode(e);
					var key_code = LiveCodeEvents._encodeKeyboardKeyCode(e);
					LiveCodeEvents._postKeyPress(stack, mods, char_code, key_code);
					break;
				case 'keyup':
				case 'keydown':
					char_code = LiveCodeEvents._encodeKeyboardCharCode(e);
					key_code = LiveCodeEvents._encodeKeyboardKeyCode(e);

					// If this is a browser using old-style keyboard events, we won't get
					// a 'keypress' message for special keys
					if (!('key' in e) && e.type === 'keydown' && 0xFE00 <= key_code && key_code <= 0xFFFF) {
						// Dispatch the keypress to the engine
						LiveCodeEvents._postKeyPress(stack, mods, char_code, key_code);

						// Suppress the default behaviour for this key
						e.preventDefault();
					}

					break;
				default:
					console.debug('Unexpected keyboard event type: ' + e.type);
					return;
				}
			});
			LiveCodeAsync.resume();

			return false;
		},

		// ----------------------------------------------------------------
		// Input events
		// ----------------------------------------------------------------

		_postImeCompose: function(stack, enabled, offset, chars, length) {
			Module.ccall('MCEventQueuePostImeCompose',
						 'number',	/* bool */
						 ['number',	/* MCStack* stack */
						 'number',	/* bool enabled */
						 'number',	/* uindex_t offset */
						 'number',	/* unichar_t* chars */
						 'number'],	/* uindex_t char_count */
						 [stack, enabled, offset, chars, length]);
		},

		_handleInput: function(inputEvent) {
			console.debug('Input event: ' + inputEvent.type + ' ' + inputEvent.data);
		},

		_stringToUTF16: function(string) {
			var buffer = _malloc(2 * string.length + 2);
			Module.stringToUTF16(string, buffer, 2*string.length + 2);
			return [buffer, string.length];
		},

		_handleComposition: function(compositionEvent) {
			LiveCodeAsync.delay(function() {
				// Stack that we're targeting
				var stack = LiveCodeEvents._getStackForCanvas(compositionEvent.target);

				// ignore events for non-lc elements
				if (!stack)
					return;

				var encodedString;
				var chars, length;
				switch (compositionEvent.type) {
					case 'compositionstart':
					case 'compositionupdate':
						encodedString = LiveCodeEvents._stringToUTF16(compositionEvent.data);
						chars = encodedString[0];
						length = encodedString[1];
						console.debug('Composition event: ' + compositionEvent.type + ' ' + Module.UTF16ToString(chars));
						LiveCodeEvents._postImeCompose(stack, true, 0, chars, length);
						_free(chars);
						break;
					case 'compositionend':
						encodedString = LiveCodeEvents._stringToUTF16(compositionEvent.data);
						chars = encodedString[0];
						length = encodedString[1];
						console.debug('Composition event: ' + compositionEvent.type + ' ' + Module.UTF16ToString(chars));
						LiveCodeEvents._postImeCompose(stack, false, 0, chars, length);
						_free(chars);
						break;
					default:
						console.debug('Unexpected composition event type: ' + compositionEvent.type)
						return;
				}
			});
			LiveCodeAsync.resume();

			// Preventing the IME event from propogating cancels the IME, so don't do that
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
			var target = LiveCodeEvents._eventTarget(mouseEvent);
			var x = mouseEvent.clientX - target.getBoundingClientRect().left -
				target.clientLeft + target.scrollLeft;
			var y = mouseEvent.clientY - target.getBoundingClientRect().top -
				target.clientTop + target.scrollTop;

			return [x, y];
		},

		// Wrapper for MCEmscriptenHandleMousePosition
		_postMousePosition: function(stack, time, modifiers, x, y) {
			Module.ccall('MCEmscriptenHandleMousePosition',
						 'number', /* bool */
						 ['number', /* MCStack *stack */
						  'number', /* uint32_t time */
						  'number', /* uint32_t modifiers */
						  'number', 'number'], /* uint32_t x, y */
						 [stack, time, modifiers, x, y]);
		},

		// Wrapper for MCEmscriptenHandleMousePress
		_postMousePress: function(stack, time, modifiers, state, button)
		{
			Module.ccall('MCEmscriptenHandleMousePress',
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

		// target for redirected mouse events
		_captureTarget: null,
		
		// Redirect mouse events to the specified target element
		_captureFocus: function(element) {
			LiveCodeEvents._captureTarget = element;
		},
		
		// End mouse event redirection
		_releaseFocus: function() {
			LiveCodeEvents._captureTarget = null;
		},
		
		// Return the target to which mouse events are dispatched
		_eventTarget: function(event) {
			if (event.type == "mousedown" || LiveCodeEvents._captureTarget == null)
				return event.target;
			else
				return LiveCodeEvents._captureTarget;
		},
		
		_handleMouseEvent: function(e) {
			LiveCodeAsync.delay(function () {

				var target = LiveCodeEvents._eventTarget(e);
				var stack = LiveCodeEvents._getStackForCanvas(target);
				var mods = LiveCodeEvents._encodeModifiers(e);
				var pos = LiveCodeEvents._encodeMouseCoordinates(e);

				// ignore events for non-lc elements
				if (!stack)
					return;

				switch (e.type) {
				case 'mousemove':
					LiveCodeEvents._postMousePosition(stack, e.timeStamp, mods, pos[0], pos[1]);
					return;

				case 'mousedown':
					// In the case of mouse down, specifically request
					// keyboard focus
					e.target.focus();
					LiveCodeEvents._postMousePosition(stack, e.timeStamp, mods, pos[0], pos[1]);
					var state = LiveCodeEvents._encodeMouseState(e.type);
					LiveCodeEvents._postMousePress(stack, e.timeStamp, mods,
												   state, e.button);
					
					// Redirect mouse events to this canvas while the mouse is down
					LiveCodeEvents._captureFocus(e.target);
					
					break;

				case 'mouseup':
					LiveCodeEvents._postMousePosition(stack, e.timeStamp, mods, pos[0], pos[1]);
					var state = LiveCodeEvents._encodeMouseState(e.type);
					LiveCodeEvents._postMousePress(stack, e.timeStamp, mods,
												   state, e.button);
					
					// change mouse focus if event target is different from captured target
					var refocus = target != e.target;
					if (refocus)
						LiveCodeEvents._postMouseFocus(stack, e.timeStamp, false);
					
					LiveCodeEvents._releaseFocus();
					
					if (refocus)
					{
						var stack = LiveCodeEvents._getStackForCanvas(e.target);
						var pos = LiveCodeEvents._encodeMouseCoordinates(e);
						if (stack)
						{
							LiveCodeEvents._postMouseFocus(stack, e.timeStamp, true);
							LiveCodeEvents._postMousePosition(stack, e.timeStamp, mods, pos[0], pos[1]);
						}
					}
					
					break;

				case 'mouseenter':
					// Don't send window focus events while capturing mouse events
					if (LiveCodeEvents._captureTarget == null)
					{
						LiveCodeEvents._postMouseFocus(stack, e.timeStamp, true);
						LiveCodeEvents._postMousePosition(stack, e.timeStamp, mods, pos[0], pos[1]);
					}
					break;

				case 'mouseleave':
					// Don't send window focus events while capturing mouse events
					if (LiveCodeEvents._captureTarget == null)
					{
						LiveCodeEvents._postMousePosition(stack, e.timeStamp, mods, pos[0], pos[1]);
						LiveCodeEvents._postMouseFocus(stack, e.timeStamp, false);
					}
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
		
		// Document mouse event handler - redirects to target element when capturing mouse events
		_handleDocumentMouseEvent: function(e) {
			if (LiveCodeEvents._captureTarget) {
				LiveCodeEvents._handleMouseEvent(e);
			}
		},

		// ----------------------------------------------------------------
		// UI events
		// ----------------------------------------------------------------
		
		// prevent context menu popup on right-click
		_handleContextMenu: function(e) {
			e.preventDefault()
		},

		// ----------------------------------------------------------------
		// Window events
		// ----------------------------------------------------------------
		
		_postWindowReshape: function(stack, backingScale)
		{
			Module.ccall('MCEventQueuePostWindowReshape',
							'number', /* bool */
							['number', /* MCStack *stack */
							 'number'], /* MCGFloat backing_scale */
							[stack, backingScale]);
		},
		
		postWindowReshape: function(window)
		{
			LiveCodeAsync.delay(function () {
				var stack = LiveCodeEvents._getStackForWindow(window);
				if (stack == 0)
				{
					console.log('could not find stack for window ' + window);
					return
				}
				LiveCodeEvents._postWindowReshape(stack, 1.0);
			});
			LiveCodeAsync.resume();
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
