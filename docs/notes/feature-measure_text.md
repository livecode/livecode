# Text Measurement
There are two new functions **measureText()** and **measureUnicodeText()** which can be used to measure the area required to draw the text using the effective font attributes of a given object.

	**measureText(*text*,*object reference*,[*mode*])**
	**measureUnicodeText(*unicodeText*,*object reference*,[*mode*])**

The mode may be one of:
* width - the functions return the width of the text.
* size - the functions return the width,height of the text.
* bounds - the functions return a rectangle of 0, - ascent, width, descent.

If no mode is specified the functions default to *width* mode. Ascent and descent are relative to a 0 baseline that the text would be drawn on.