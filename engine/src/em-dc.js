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
	$LiveCodeDC__deps: ['$LiveCodeEvents'],
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
			LiveCodeDC._maxZIndex = 1;

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
			var div;
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
				div = document.createElement('div');
				div.style.display = 'none';
				div.style.position = 'fixed';
				div.style.zIndex = LiveCodeDC._maxZIndex;
				LiveCodeDC._maxZIndex++;
				canvas = document.createElement('canvas');
				canvas.style.setProperty('width', '100%');
				canvas.style.setProperty('height', '100%');
				div.appendChild(canvas);
				document.body.appendChild(div);
		  
				rect = {'left':0, 'top':0, 'right':0, 'bottom':0};
			}
			
			var tWindowID = ++LiveCodeDC._maxWindowID;
			
			LiveCodeEvents.addEventListeners(canvas);
			LiveCodeDC._monitorResize(canvas, function() { LiveCodeEvents.postWindowReshape(tWindowID); });
			LiveCodeDC._windowList[tWindowID] = {
				'div': div,
				'canvas': canvas,
				'rect': rect,
				'visible':false,
			};
			
			return tWindowID;
		},
		
		destroyWindow: function(pID) {
			LiveCodeDC.setWindowVisible(pID, false);
			LiveCodeDC.raiseWindow(pID);
			var window = LiveCodeDC._windowList[pID];
			if (window)
			{
				LiveCodeDC._removeResizeMonitor(window.canvas);
		  
				if (window.canvas == Module['canvas'])
				{
					// TODO - handle cleanup of embedded canvas
					LiveCodeDC._assignedMainWindow = false;
				}
				else
				{
					document.body.removeChild(window.div);
					LiveCodeDC._maxZIndex--;
				}
				delete LiveCodeDC._windowList[pID];
			}
		},
		
		raiseWindow: function(pID) {
			var window = LiveCodeDC._windowList[pID];
			if (window && window.div)
			{
				var zIndex = window.div.style.zIndex;
				for (var tID in LiveCodeDC._windowList)
				{
					if (LiveCodeDC._windowList[tID].div && LiveCodeDC._windowList[tID].div.style.zIndex > zIndex)
					{
						LiveCodeDC._windowList[tID].div.style.zIndex--;
					}
				}
				window.div.style.zIndex = LiveCodeDC._maxZIndex - 1;
			}
		},
		
		setWindowRect: function(pID, pLeft, pTop, pRight, pBottom) {
			var window = LiveCodeDC._windowList[pID];
			if (window)
			{
				var width = pRight - pLeft;
				var height = pBottom - pTop;
				window.rect = {'left':pLeft, 'top':pTop, 'right':pRight, 'bottom':pBottom};
				if (window.div)
				{
					window.div.style.setProperty('left', pLeft + 'px', 'important');
					window.div.style.setProperty('top', pTop + 'px', 'important');
					window.div.style.setProperty('width', width + 'px', 'important');
					window.div.style.setProperty('height', height + 'px', 'important');
				}
				else
				{
					window.canvas.style.setProperty('width', width + 'px', 'important');
					window.canvas.style.setProperty('height', height + 'px', 'important');
				}
			}
		},
		
		getWindowRect: function(pID) {
			var window = LiveCodeDC._windowList[pID];
			if (window)
				return window.canvas.getBoundingClientRect();
		  
			return null;
		},
		
		setWindowVisible: function(pID, pVisible) {
			var window = LiveCodeDC._windowList[pID];
			if (window)
			{
				window.visible = pVisible;
				if (window.div)
				{
					if (pVisible)
						window.div.style.display = 'block';
					else
						window.div.style.display = 'none';
				}
				else
				{
					if (pVisible)
						window.canvas.style.visibility = 'visible';
					else
						window.canvas.style.visibility = 'hidden';
				}
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
		
		getWindowIDForCanvas: function(pCanvas) {
			for (var tID in LiveCodeDC._windowList) {
				if (LiveCodeDC._windowList.hasOwnProperty(tID)) {
					if (pCanvas == LiveCodeDC._windowList[tID].canvas) {
						return tID;
					}
				}
			}
			console.debug("canvas not found");
			return 0;
		},
		
		getDisplayRect: function() {
			return {'left':0, 'top':0, 'right':document.body.clientWidth, 'bottom':document.body.clientHeight};
		},
		
		_monitorResize: function(element, callback) {
			// initial setup on first call
			if (!LiveCodeDC._monitorResize.watched) {
				LiveCodeDC._monitorResize.watched = [];
		  
				var resizeCheck = function() {
					LiveCodeDC._monitorResize.watched.forEach(function(data) {
						var rect = data.element.getBoundingClientRect();
						if (rect.left != data.rect.left || rect.top != data.rect.top || rect.right != data.rect.right || rect.bottom != data.rect.bottom) {
							data.rect = rect;
							data.callback();
						}
					});
				};

				// Listen to the window's size changes
				window.addEventListener('resize', resizeCheck);
		  
				// Listen to changes on the elements in the page that affect layout
				var observer = new MutationObserver(resizeCheck);
				observer.observe(document.body, {
					attributes: true,
					childList: true,
					characterData: true,
					subtree: true
				});
			}
			
			LiveCodeDC._monitorResize.watched.push({'element':element, 'callback':callback, 'rect':element.getBoundingClientRect()});
		},
		
		_removeResizeMonitor: function(element) {
			if (LiveCodeDC._monitorResize.watched) {
				var index = LiveCodeDC._monitorResize.watched.indexOf(element);
				if (index !== -1) {
					LiveCodeDC._monitorResize.watched.splice(index, 1);
				}
			}
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
	
	MCEmscriptenRaiseWindow__deps: ['$LiveCodeDC'],
	MCEmscriptenRaiseWindow: function(pWindowID) {
		LiveCodeDC.raiseWindow(pWindowID);
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
	
	MCEmscriptenGetDisplayRect__deps: ['$LiveCodeDC'],
	MCEmscriptenGetDisplayRect: function(rLeft, rTop, rRight, rBottom) {
		var rect = LiveCodeDC.getDisplayRect();
		{{{ makeSetValue('rLeft', '0', 'rect.left', 'i32') }}};
		{{{ makeSetValue('rTop', '0', 'rect.top', 'i32') }}};
		{{{ makeSetValue('rRight', '0', 'rect.right', 'i32') }}};
		{{{ makeSetValue('rBottom', '0', 'rect.bottom', 'i32') }}};
	},
	
	MCEmscriptenSyncCanvasSize__deps: ['$LiveCodeDC'],
	MCEmscriptenSyncCanvasSize: function(p_window_id, p_width, p_height)
	{
		var canvas = LiveCodeDC.getWindowCanvas(p_window_id);
		if (!canvas)
			return;
		
		if (canvas.width != p_width || canvas.height != p_height)
		{
			canvas.width = p_width;
			canvas.height = p_height;
		}
	},
});
