#Resolution Independence

The primary feature included in LiveCode 6.5 is support for resolution independence. Resolution independence means that an app can be designed using an abstract pixel density and then have that automatically map to the pixel density of the display with no loss of quality.

For example, iPhones have retina displays which are twice the density of previous generations. This greater density isn't intended to provide more screen real-estate, but instead provide a crisper display. An app should be designed at the standard density (about 163dpi) leaving it to the OS to scale the rendering to enable the app to benefit from the greater density.

##New Graphics Layer

The first step to supporting resolution independence was to completely refactor LiveCode's graphics layer. This involved writing and integrating an entirely new 2D graphics library that allows for scaled drawing. In addition to 2D graphic rendering, the library also handles text and image rendering. As such, nearly all aspects of LiveCode's drawing routines have been touched.

Since all of the updates are internal, the end LiveCode developer should see no major changes: Where possible, we've tried to match previous behaviors as closely as possible. However, in the early DPs, we do expect some rendering irregularities.

Though the primary reason for the updates is to support resolution independence, we do get the side benefits of having a modern 2D graphics library. These include a clean developer API allowing for easy integration into other modules, potential performance improvements and support for graphic rendering on the server platforms (planned for a future release).

##Multiple Density Support

Most OSs support multiple pixel densities, with there being a "natural density" and a notion of Hi-DPI. DP1 of LiveCode 6.5 has support for multiple densities on Android and iOS.

For iOS, the screen is either retina or non-retina. The natural density is the non-retina resolutions. Retina screens are considered to be 2x the density of non-retina.

For Android, screens usually fall in to one of four density categories - low (0.75x), medium (1x), high (1.5x), extra-high (2x). Medium is considered to be the natural density. Some devices have a "TV" screen density, at 1.33x scale.

When coding for multiple densities, we take the notion of a "pixel" from a stack's point of view to be "a pixel at the natural density of the OS", what we refer to as a point. A scale factor is then applied on output to map to that of the screen density of the device.

This is an entirely invisible process. From the point of view of the app everything appears as if the screen was the "natural" density. In particular:

* import / export snapshot work in points, not pixels - i.e they produce images at point resolution
* imageData (maskData/alphaData) of images work in points, not pixels
* intersections work in points
* hit detection works in points
* the screenRect properties and stack rect are given in points

Android screen sizes prior to the introduction of resolution independence were given in pixels, rather than points. As a result of this many Android devices with high density displays will now report an apparently smaller screen size. This may require modifications to existing stacks that were created with the larger pixel size in mind.

##Density Mapped Images

Whilst text and vector operations scale naturally, this is not true of images. To take full advantage of Hi-DPI, images must be provided at appropriate sizes for different densities. The developer can do this by having multiple image files in the same location named appropriately (with the image object referencing the image file at the natural density). The naming convention is as follows:

* &lt;image&gt;@ultra-low.&lt;ex&gt; - 0.25x
* &lt;image&gt;@extra-low.&lt;ext&gt; - 0.5x
* &lt;image&gt;@low.&lt;ext&gt; - 0.75x
* &lt;image&gt;@medium.&lt;ext&gt; / &lt;image&gt;.&lt;ext&gt; - 1x
* &lt;image&gt;@high.&lt;ext&gt; - 1.5x
* &lt;image&gt;@extra-high.&lt;ext&gt; / &lt;image&gt;@2x - 2x
* &lt;image&gt;@ultra-high.&lt;ext&gt; - 4x

When an image is required, the current scale factor is rounded up to the nearest standard density (one of 0.25, 0.5, 0.75, 1, 1.5, 2 and 4). The image with the lowest scale factor that is greater or equal to the nearest standard density is then selected. For example, if the scale factor is 1.75 and there is an ‘extra-high’ image available that is used.

Images should be created at appropriately scaled pixel sizes - if the natural size of an image is x points wide and y points high, the scaled versions should be x * scale pixels wide and y * scale pixels high in order to have the same natural size. Developers should take care that the natural size of the image scales nicely to integer pixel sizes, as the logical point size of the image will be rounded up to the nearest integer. If this is not practical for a given image, the developer may need to design their apps to handle slightly different resulting image sizes.
A useful rule of thumb is for images to have natural width and height values that are multipes of four, this way all the supported densities will result in integer pixel sizes for each version of the image, and the resulting image will have the same logical point size as all display densities.

##Future Plans

###More control over automatic scaling

Currently, on Android and iOS the scale factor is automatically applied. On iOS, this can be overridden by calling the existing command "iphoneUseDeviceResolution true", which will turn off scaling so one point is equal to one pixel. This capability will be generalized to all platforms supporting resolution independence, and extended to allow configuration of the display scale.

###Full screen scaling mode.

There are multiple ways in which a stack can be resized or scaled to take full advantage of the available screen space. The full screen scaling mode will allow the developer to choose the most appropraite for their application:

* empty (default) - the existing behaviour - the stack is resized (not scaled) to fit the screen.
* "exact fit" - scale the stack to fill the screen. This will stretch the stack if the aspect ratio of the screen does not match that of the stack.
* "show all" - scale the stack preserving aspect ratio so all content is visible. Some blank space may remain if the screen & stack aspect ratios do not match.
* "no border" - scale the stack to fill the screen preserving aspect ratio. If the stack & screen aspect ratios do not match, the left / right or top / bottom extremes of the stack will not be visible.
* "no scale" - the stack will not be scaled, being centered on the screen instead.

This will be available on all desktop platforms and operates independently from Hi-DPI support.

###Hi-DPI support on desktop platforms.

Automatically scale stacks on desktop systems with high resolution displays. This will function in the same way as the current support for mobile devices.
Support for automatic scaling will be added for desktop operating systems that support high resolution displays. This currently includes Windows 7 & 8 and OSX Mountain Lion.

###Server graphics.

Support for creating images for output on a web server, for example by composing a set of objects and using "export snapshot ... " to generate the image data.
