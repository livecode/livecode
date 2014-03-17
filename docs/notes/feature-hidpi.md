# Hi-DPI support for Windows 7/8 and OSX.

Hi-DPI support has been added for Windows 7/8 and OSX.

The global **pixelScale** property is no longer settable on desktop environments.

Desktop systems may have multiple displays attached, each with their own density value. On platforms that support different scale values for each display, LiveCode will now automatically render each stack at the correct scale for the screen on which it is displayed.

A new global property has been added to allow this feature to be turned on or off if required:
* The **usePixelScaling** property controls whether or not LiveCode will automatically apply pixel scaling. If set to false, pixel scaling will be disabled and LiveCode will draw at a 1:1 scale, allowing the operating system to perform any scaling required.

When **usePixelScaling** is false, the **pixelScale** will be set to 1 and will not be modifiable.

Due to limitations of the platform, applications on Windows will not be able to enable or disable pixel scaling. On Android there is no equivalent OS-level scaling provided, so pixel scaling will always be used.


## Handling multiple displays

As there may be multiple displays with differing scale values, The global **systemPixelScale** property will now return the maximum screen density for all connected displays.

Two additional global properties have been added to provide the pixel scale values when multiple displays are available:
* The **screenPixelScale** returns the pixel scale of the main screen
* The **screenPixelScales** returns a return-delimited list of the pixel scale of each connected display



