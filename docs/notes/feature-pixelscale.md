# Control the screen pixel scaling.

The following global properties have been added to the engine:
* **systemPixelScale**
* **pixelScale**

The pixel scale determines the mapping from logical screen pixels to physical device pixels.

# The 'systemPixelScale' property

This property is read-only. It returns the pixel scale reported by the operating system as being appropriate for the display device.

For example, an iOS with a retina display has twice the pixel density of previous generations, so the following will return '2':
	get the systemPixelScale

Currently the **systemPixelScale** returns the correct value for mobile devices, returning '1' on desktop platforms.

We will soon be adding support for high density displays on the desktop, at which point this property will return a meaningful value on all supported platforms.

# The 'pixelScale' property

This property determines the scaling factor between logical and device pixels and can be set to any positive real number.
The **pixelScale** is initialised to the **systemPixelScale** on startup but can be modified at any point.

For example, if you want to turn off automatic scaling on a high-density display you can set the **pixelScale** to '1.0':
	set the pixelScale to 1.0

