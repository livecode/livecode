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

    // Blits the contents of the given RGBA buffer to the specified
    // location on the main HTML5 canvas
    MCEmscriptenBlitToWindowCanvas__deps: ['$LiveCodeDC'],
    MCEmscriptenBlitToWindowCanvas: function(p_window_id, p_rgba_buffer, p_x, p_y, p_w, p_h) {
        // Get the window canvas
        var canvas = LiveCodeDC.getWindowCanvas(p_window_id);

        // Create and get a 2D rendering context for the canvas
        var context = canvas.getContext('2d');
        if (!context) {
            context = Browser.createContext(canvas, false, true);
        }

        // Turn the buffer into an ImageData object
        var byteCount = p_w * p_h * 4;
        var dataArray = new Uint8ClampedArray(Module.HEAPU8.buffer, p_rgba_buffer, byteCount);
        var imageData = new ImageData(dataArray, p_w, p_h);

        // And now draw that image data
        context.putImageData(imageData, p_x, p_y);
    }

})
