# Hi-DPI support for Windows 7/8 and OSX.

Hi-DPI support has been added for Windows 7/8 and OSX. This means that OS scaling has been turned off so stacks will no longer appear pixelated.
In addition, the **systemPixelScale** property will now return the current screen density for the supported platforms, so stacks will automatically render at the correct scale for the current screen density.
