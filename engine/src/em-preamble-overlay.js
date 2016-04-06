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

// The code in this file is run before the Emscripten engine starts.

// ----------------------------------------------------------------
// Add "download this app" overlay
// ----------------------------------------------------------------

// We display an overlay on the canvas that allows users to download
// the stack.  Note that we *don't* let the overlay parameters be
// overridden by the web page, unlike the standalone parameters above.

// FIXME Massive amounts of hardcoded styling that can't be customized
// in any way

// Enable the overlay by default.
Module['livecodeOverlay'] = true;

// LiveCode community icon, as SVG.
Module['livecodeOverlayIcon'] = '<?xml version="1.0" encoding="utf-8"?><!-- Generator: Adobe Illustrator 18.1.1, SVG Export Plug-In . SVG Version: 6.00 Build 0)  --><svg version="1.1" id="Layer_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px" viewBox="0 0 579 594" enable-background="new 0 0 579 594" xml:space="preserve"><path fill="#AED036" d="M546.3,40.4c-0.1-0.5-0.3-0.9-0.4-1.4c0-0.1-0.1-0.3-0.1-0.5l0,0c-7.3-21.5-28.3-37.1-53-37.1H87 c-25.2,0-46.5,16.1-53.4,38.3l-0.1-0.1C13.9,101,3.1,170.7,3.1,246c0,73.3,10.7,141.4,29.3,201.4c0.5,2.5,1.3,5,2.2,7.3 c0.1,0.3,0.2,0.6,0.3,0.8l0,0c8,20.3,28.3,34.7,52.1,34.7h108.7l-28,102.9l167.1-102.9h157.9c24,0,44.4-14.6,52.3-35.1l0.1,0 c0.1-0.3,0.2-0.6,0.3-0.9c0.7-1.9,1.3-3.9,1.8-5.9c18.9-60.4,29.3-128.7,29.3-202.6C576.4,171.3,565.5,101.3,546.3,40.4z M351.5,319.6c27.7,27.7,72.6,27.7,100.3,0l46.8,46.8c-50,50.1-129.2,53.3-183,9.7c-53.8,43.6-133,40.3-183-9.7 c-28.3-28.3-41.7-66-40-103.1V94.8h66.3v52.8c0,0.4,0,0.9,0,1.3v114.3c-1.7,20.1,5.1,40.9,20.5,56.3c26.4,26.4,68.4,27.6,96.3,3.7 c-21.3-50-11.6-110,29.1-150.8c53.5-53.5,140.3-53.5,193.8,0l-46.8,46.8c-27.7-27.7-72.6-27.7-100.3,0 C323.8,246.9,323.8,291.9,351.5,319.6z"/></svg>';

// Text displayed in the overlay.
Module['livecodeOverlayText'] = 'Download this app!';

// Initial URL that's launched by the overlay (until the standalone
// has been downloaded)
Module['livecodeOverlayUrl'] = 'https://livecode.com/';

// Before the engine starts, insert the overlay on top of the canvas.
Module['preRun'].push(function() {

	// Check if the overlay should be installed
	var mode = Module['livecodeOverlay'];
	if (mode === false) {
		return;
	}

	// If the overlay is already present, don't recreate it
	if (Module['livecodeOverlayContainer']) {
		return;
	}

	// Get the canvas.  If there's no canvas, there's no overlay
	// either.
	var canvas = Module['canvas'];
	if (!canvas) {
		return;
	}

	// Insert a new <span> around the canvas.  This will be used as a
	// common parent for the canvas and the overlay.  This is needed
	// in order to position the overlay correctly relative to the
	// canvas, without making the overlay a child element of the
	// canvas (unfortunately, the latter isn't permitted by HTML).
	//
	// The container needs to have 0 padding, 0 border, 0 margin, and
	// position relative (i.e. it should pretend to not exist).
	var container = document.createElement('div');
	container.classList.add('emscripten');
	container.style.position = 'relative';
	Module['livecodeOverlayContainer'] = container;

	// Move the canvas into the container element.
	canvas.parentNode.appendChild(container);
	container.appendChild(canvas);


	var svg = document.createElement('div');
	svg.style.height = '1em';
	svg.style.width = '1em';
	svg.style.display = 'inline-block';
	svg.innerHTML = Module['livecodeOverlayIcon'];

	// FIXME internationalise this text
	var text = document.createElement('span');
	text.appendChild(document.createTextNode(Module['livecodeOverlayText']));
	text.style.display = 'none'; // Initially invisible
	text.style.marginLeft = '1ex';
	text.style.marginRight = '1ex';

	var overlay = document.createElement('a');
	overlay.appendChild(text);
	overlay.appendChild(svg);
	overlay.style.position = 'absolute';
	overlay.style.right = '1px';
	overlay.style.bottom = '1px';
	overlay.style.color = '#000000';
	overlay.style.backgroundColor = '#FFFFFF';
	overlay.style.border = '1px solid #AED036';
	overlay.style.borderRadius = '5px';
	overlay.style.padding = '2px';
	overlay.href = Module['livecodeOverlayUrl'];
	overlay.target = '_top';

	container.appendChild(overlay);

	// Show the text whenever the mouse is over the overlay, as long
	// as the standalone has finished downloading.
	overlay.addEventListener('mouseover', function () {
		// Update the link target with the standalone's URL, if
		// available
		if (Module['livecodeStandaloneUrl']) {
			overlay.href = Module['livecodeStandaloneUrl'];
		}

		text.style.display = 'inline';
	});

	// Hide the text when the mouse leaves the overlay
	overlay.addEventListener('mouseout', function () {
		text.style.display = 'none';
	});

});
