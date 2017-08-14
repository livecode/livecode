# Synthesize an MS Paint compatible clipboard format for images

The engine will (once again) synthesize a DIBV5 format when an image is
copied to the clipboard. This will be a 32-bit RGBA DIB. Windows then
automatically synthesizes a 24-bit RGB DIB format.
