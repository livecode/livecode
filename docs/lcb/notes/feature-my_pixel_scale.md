# LiveCode Builder Host Library
## Widget library

A new expression `my pixel scale` has been implemented. Use the widget's
pixel scale to calculate the size of an image to draw. For example,
when drawing an image to `my bounds` create an image sized using
`my width * my pixel scale, my height * my pixel scale` otherwise the image will be
stretched to match the pixel scale. The pixel scale is a per-window/screen property
so may change if the user moves a window to a new screen.
