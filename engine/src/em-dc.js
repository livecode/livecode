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
	$LiveCodeDC__deps: ['$LiveCodeEvents', '$LiveCodeUtil'],
	$LiveCodeDC: {
		// true if ensureInit() has ever been run
		_initialised: false,

		initialize: function() {
			// Make sure this only ever gets run once
			if (LiveCodeDC._initialised) {
				return;
			}
			
			LiveCodeDC._windowList = this.containerCreate(document.body, 1);
			LiveCodeDC._assignedMainWindow = false;

			LiveCodeDC._initialised = true;
		},

		finalize: function() {
			if (!LiveCodeDC._initialised) {
				return;
			}

			LiveCodeDC._initialised = false;
		},

		containerCreate: function(containerElement, maxZIndex) {
			return {'element':containerElement, 'maxZIndex':maxZIndex, 'contents':[]};
		},
		
		containerAddElement: function(container, element) {
			element.style.position = 'absolute';
			element.style.zIndex = container['maxZIndex'];
			container['maxZIndex']++;
			container['element'].appendChild(element);
			container['contents'].push(element);
		},
		
		containerRemoveElement: function(container, element) {
			this.containerRaiseElement(container, element);
			container['element'].removeChild(element);
			container['maxZIndex']--;
			var index = container['contents'].indexOf(element);
			if (index !== -1) {
				container['contents'].splice(index, 1);
			}
		},
		
		containerRaiseElement: function(container, element) {
			var zIndex = element.style.zIndex;
			for (var i = 0, l = container['contents'].length; i < l; i++)
			{
				var otherElement = container['contents'][i];
				if (otherElement.style.zIndex > zIndex)
				{
					otherElement.style.zIndex--;
				}
			}
			element.style.zIndex = container['maxZIndex'] - 1;
		},
		
		// Windows are backed by an html canvas within a floating div
		createWindow: function() {
			var windowElement;
			var canvas;
			var rect;
			var mainWindow = false;
			if (!LiveCodeDC._assignedMainWindow)
			{
				mainWindow = true;
				canvas = Module['canvas'];
				rect = canvas.getBoundingClientRect();
				LiveCodeDC._assignedMainWindow = true;
				windowElement = canvas.parentElement;
				windowElement.style.setProperty('position', 'relative');
				windowElement.style.setProperty('width', rect['right'] - rect['left']);
				windowElement.style.setProperty('height', rect['bottom'] - rect['top']);
				canvas.style.setProperty('position', 'absolute');
				canvas.style.setProperty('left', '0px');
				canvas.style.setProperty('right', '0px');
				canvas.style.setProperty('width', '100%');
				canvas.style.setProperty('height', '100%');
			}
			else
			{
				windowElement = document.createElement('div');
				windowElement.style.display = 'none';
				windowElement.style.position = 'fixed';
				canvas = document.createElement('canvas');
				canvas.style.setProperty('position', 'absolute');
				canvas.style.setProperty('left', '0px');
				canvas.style.setProperty('right', '0px');
				canvas.style.setProperty('width', '100%');
				canvas.style.setProperty('height', '100%');
				windowElement.appendChild(canvas);
				this.containerAddElement(this._windowList, windowElement);
		  
				rect = {'left':0, 'top':0, 'right':0, 'bottom':0};
			}
			
			var tWindowID = LiveCodeUtil.storeObject({
				'mainWindow': mainWindow,
				'element': windowElement,
				'canvas': canvas,
				'container': this.containerCreate(windowElement, 1),
				'rect': rect,
				'visible':false,
			});
			
			canvas.dataset.lcWindowId = tWindowID;
			LiveCodeEvents.addEventListeners(canvas);
			LiveCodeDC._monitorResize(canvas, function() {
				LiveCodeEvents.postWindowReshape(tWindowID);
			});
			
			return tWindowID;
		},
		
		destroyWindow: function(pID) {
			var window = LiveCodeUtil.fetchObject(pID);
			if (window)
			{
				LiveCodeDC._removeResizeMonitor(window.canvas);
				LiveCodeDC.setWindowVisible(pID, false);
				
				if (window.mainWindow)
				{
					// TODO - handle cleanup of embedded canvas
					LiveCodeDC._assignedMainWindow = false;
				}
				else
				{
					this.containerRemoveElement(this._windowList, window.element);
				}
				LiveCodeUtil.releaseObject(pID);
			}
		},
		
		raiseWindow: function(pID) {
			var window = LiveCodeUtil.fetchObject(pID);
			if (window && !window.mainWindow)
			{
				this.containerRaiseElement(this._windowList, window.element);
			}
		},
		
		setWindowRect: function(pID, pLeft, pTop, pRight, pBottom) {
			var window = LiveCodeUtil.fetchObject(pID);
			if (window)
			{
				var width = pRight - pLeft;
				var height = pBottom - pTop;
				window.rect = {'left':pLeft, 'top':pTop, 'right':pRight, 'bottom':pBottom};
				if (!window.mainWindow)
				{
					window.element.style.setProperty('left', pLeft + 'px', 'important');
					window.element.style.setProperty('top', pTop + 'px', 'important');
				}
				window.element.style.setProperty('width', width + 'px', 'important');
				window.element.style.setProperty('height', height + 'px', 'important');
			}
		},
		
		getWindowRect: function(pID) {
			var window = LiveCodeUtil.fetchObject(pID);
			if (window)
				return window.canvas.getBoundingClientRect();
		  
			return null;
		},
		
		setWindowVisible: function(pID, pVisible) {
			var window = LiveCodeUtil.fetchObject(pID);
			if (window)
			{
				window.visible = pVisible;
				if (!window.mainWindow)
				{
					if (pVisible)
						window.element.style.display = 'block';
					else
						window.element.style.display = 'none';
				}
				else
				{
					if (pVisible)
						window.element.style.visibility = 'visible';
					else
						window.element.style.visibility = 'hidden';
				}
			}
		},
		
		getWindowVisible: function(pID) {
			var window = LiveCodeUtil.fetchObject(pID);
			if (window)
			{
				return window.visible;
			}
			
			return false;
		},
		
		getWindowCanvas: function(pID) {
			var window = LiveCodeUtil.fetchObject(pID);
			if (window)
			{
				return window.canvas;
			}
			
			return null;
		},
		
		getWindowIDForCanvas: function(pCanvas) {
			if (pCanvas.dataset.lcWindowId)
				return Number(pCanvas.dataset.lcWindowId);
			console.debug("canvas not found");
			return 0;
		},
		
		getDisplayRect: function() {
			return {'left':0, 'top':0, 'right':document.body.clientWidth, 'bottom':document.body.clientHeight};
		},
		
		/*** Native Layer API ***/
		nativeElementSetRect: function(pElementID, pLeft, pTop, pRight, pBottom) {
			var tElement = LiveCodeUtil.fetchObject(pElementID);
			if (tElement)
			{
				console.log('set rect of element ' + pElementID + ' to {' + pLeft + ',' + pTop + ',' + pRight + ',' + pBottom + ')');
				tElement.style.setProperty('left', pLeft + 'px', 'important');
				tElement.style.setProperty('top', pTop + 'px', 'important');
				tElement.style.setProperty('width', pRight - pLeft + 'px', 'important');
				tElement.style.setProperty('height', pBottom - pTop + 'px', 'important');
			}
		},
		
		nativeElementSetClip: function(pElementID, pLeft, pTop, pRight, pBottom) {
			var tElement = LiveCodeUtil.fetchObject(pElementID);
			if (tElement)
			{
				tElement.style.setProperty('clip', 'rect(' + pTop + 'px,' + pRight + 'px,' + pBottom + 'px,' + pLeft + 'px)', 'important');
			}
		},
		
		nativeElementSetVisible: function(pElementID, pVisible) {
			var tElement = LiveCodeUtil.fetchObject(pElementID);
			if (tElement)
			{
				if (pVisible)
					tElement.style.display = 'block';
				else
					tElement.style.display = 'none';
			}
		},
		
		nativeElementAddToWindow: function(pElementID, pWindowID) {
			var tWindow = LiveCodeUtil.fetchObject(pWindowID);
			var tElement = LiveCodeUtil.fetchObject(pElementID);
			if (tWindow && tElement)
			{
				this.containerAddElement(tWindow.container, tElement);
			}
		},
		
		nativeElementRemoveFromWindow: function(pElementID, pWindowID) {
			var tWindow = LiveCodeUtil.fetchObject(pWindowID);
			var tElement = LiveCodeUtil.fetchObject(pElementID);
			if (tWindow && tElement)
			{
				this.containerRemoveElement(tWindow.container, tElement);
			}
		},
		
		nativeElementPlaceAbove: function(pElementID, pAboveElementID, pWindowID) {
			var tWindow = LiveCodeUtil.fetchObject(pWindowID);
			var tElement = LiveCodeUtil.fetchObject(pElementID);
			var tAboveElement = LiveCodeUtil.fetchObject(pAboveElementID);
			/* TODO - implement */
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
				var index = 0;
				while (index < LiveCodeDC._monitorResize.watched.length)
				{
					if (LiveCodeDC._monitorResize.watched[index].element === element)
						LiveCodeDC._monitorResize.watched.splice(index, 1);
					else
						index++;
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
	MCEmscriptenElementSetRect__deps: ['$LiveCodeDC'],
	MCEmscriptenElementSetRect: function(p_element, p_left, p_top, p_right, p_bottom) {
		LiveCodeDC.nativeElementSetRect(p_element, p_left, p_top, p_right, p_bottom);
	},
	MCEmscriptenElementSetClip__deps: ['$LiveCodeDC'],
	MCEmscriptenElementSetClip: function(p_element, p_left, p_top, p_right, p_bottom) {
		LiveCodeDC.nativeElementSetClip(p_element, p_left, p_top, p_right, p_bottom);
	},
	MCEmscriptenElementSetVisible__deps: ['$LiveCodeDC'],
	MCEmscriptenElementSetVisible: function(p_element, p_visible) {
		LiveCodeDC.nativeElementSetVisible(p_element, p_visible);
	},
	MCEmscriptenElementAddToWindow__deps: ['$LiveCodeDC'],
	MCEmscriptenElementAddToWindow: function(p_element, p_container) {
		LiveCodeDC.nativeElementAddToWindow(p_element, p_container);
	},
	MCEmscriptenElementRemoveFromWindow__deps: ['$LiveCodeDC'],
	MCEmscriptenElementRemoveFromWindow: function(p_element, p_container) {
		LiveCodeDC.nativeElementRemoveFromWindow(p_element, p_container);
	},
	MCEmscriptenElementPlaceAbove__deps: ['$LiveCodeDC'],
	MCEmscriptenElementPlaceAbove: function(p_element, p_above, p_container) {
		LiveCodeDC.nativeElementPlaceAbove(p_element, p_above, p_container);
	},
});
