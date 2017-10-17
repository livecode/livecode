/*                                                              -*-Javascript-*-
 
 Copyright (C) 2017 LiveCode Ltd.
 
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
	// $LiveCodeDC__deps: ['$LiveCodeAsync'],
	$LiveCodeDC: {
		// true if ensureInit() has ever been run
		_initialised: false,

		initialize: function() {
			// Make sure this only ever gets run once
			if (LiveCodeDC._initialised) {
				return;
			}
			
			LiveCodeDC._windowList = {};
			LiveCodeDC._assignedMainWindow = false;
			LiveCodeDC._maxWindowID = 0;

			LiveCodeDC._initialised = true;
		},

		finalize: function() {
			if (!LiveCodeDC._initialised) {
				return;
			}

			LiveCodeDC._initialised = false;
		},

		// Windows are backed by an html canvas within a floating div
		createWindow: function() {
			var canvas;
			var rect;
			if (!LiveCodeDC._assignedMainWindow)
			{
				canvas = Module['canvas'];
				rect = canvas.getBoundingClientRect();
				LiveCodeDC._assignedMainWindow = true;
			}
			else
			{
				canvas = document.createElement('canvas');
				canvas.style.position = 'absolute';
				document.appendChild(canvas);
		  
				rect = {'left':0, 'top':0, 'right':0, 'bottom':0};
			}
			
			LiveCodeDC._maxWindowID += 1;
			LiveCodeDC._windowList[LiveCodeDC._maxWindowID] = {
				'canvas': canvas,
				'rect': rect,
				'visible':false,
			};
			
			return LiveCodeDC._maxWindowID;
		},
		
		destroyWindow: function(pID) {
			var window = LiveCodeDC._windowList[pID];
			if (window)
			{
				if (window.canvas == Module['canvas'])
				{
					// TODO - handle cleanup of embedded canvas
					LiveCodeDC._assignedMainWindow = false;
				}
				else
				{
					document.removeChild(window.canvas);
				}
				LiveCodeDC._windowList[pID] = null;
			}
		},
		
		setWindowRect: function(pID, pLeft, pTop, pRight, pBottom) {
			var window = LiveCodeDC._windowList[pID];
			if (window)
			{
				window.rect = {'left':pLeft, 'top':pTop, 'right':pRight, 'bottom':pBottom};
				window.canvas.style.left = pLeft + 'px';
				window.canvas.style.top = pLeft + 'px';
				window.canvas.style.right = pRight + 'px';
				window.canvas.style.bottom = pBottom + 'px';
			}
		},
		
		getWindowRect: function(pID) {
			var window = LiveCodeDC._windowList[pID];
			if (window)
				return window.rect;
		  
			return null;
		},
		
		setWindowVisible: function(pID, pVisible) {
			var window = LiveCodeDC._windowList[pID];
			if (window)
			{
				window.visible = pVisible;
				if (pVisible)
					window.canvas.style.visibility = 'visible';
				else
					window.canvas.style.visibility = 'hidden';
			}
		},
		
		getWindowVisible: function(pID) {
			var window = LiveCodeDC._windowList[pID];
			if (window)
			{
				return window.visible;
			}
			
			return false;
		},
		
		getWindowCanvas: function(pID) {
			var window = LiveCodeDC._windowList[pID];
			if (window)
			{
				return window.canvas;
			}
			
			return null;
		},
	},
	
	MCEmscriptenDCInitializeJS__deps: ['$LiveCodeDC'],
	MCEmscriptenDCInitializeJS: function() {
		LiveCodeDC.initialize();
		return true;
	},

	MCEmscriptenDCFinalizeJS__deps: ['$LiveCodeDC'],
	MCEmscriptenDCFinalizeJS: function() {
		LiveCodeDC.finalize();
	},

	MCEmscriptenCreateWindow__deps: ['$LiveCodeDC'],
	MCEmscriptenCreateWindow: function() {
		var window = LiveCodeDC.createWindow();
		console.debug("Created window: " + window);
		return window;
	},
	
	MCEmscriptenDestroyWindow__deps: ['$LiveCodeDC'],
	MCEmscriptenDestroyWindow: function(pWindowID) {
		LiveCodeDC.destroyWindow(pWindowID);
	},
	
	MCEmscriptenSetWindowRect__deps: ['$LiveCodeDC'],
	MCEmscriptenSetWindowRect: function(pWindowID, pLeft, pTop, pRight, pBottom) {
		LiveCodeDC.setWindowRect(pWindowID, pLeft, pTop, pRight, pBottom);
	},

	MCEmscriptenGetWindowRect__deps: ['$LiveCodeDC'],
	MCEmscriptenGetWindowRect: function(pWindowID, rLeft, rTop, rRight, rBottom) {
		var rect = LiveCodeDC.getWindowRect(pWindowID);
		{{{ makeSetValue('rLeft', '0', 'rect.left', 'i32') }}};
		{{{ makeSetValue('rTop', '0', 'rect.top', 'i32') }}};
		{{{ makeSetValue('rRight', '0', 'rect.right', 'i32') }}};
		{{{ makeSetValue('rBottom', '0', 'rect.bottom', 'i32') }}};
	},
	
	MCEmscriptenSetWindowVisible__deps: ['$LiveCodeDC'],
	MCEmscriptenSetWindowVisible: function(pWindowID, pVisible) {
		LiveCodeDC.setWindowVisible(pWindowID, pVisible);
	},
	
	MCEmscriptenGetWindowVisible__deps: ['$LiveCodeDC'],
	MCEmscriptenGetWindowVisible: function(pWindowID) {
		return LiveCodeDC.getWindowVisible(pWindowID);
	},
});
