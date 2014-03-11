# Stack scaling

The new **scaleFactor** stack property allows you to set a custom scale factor for a stack.
The **scaleFactor** property accepts a non-zero real number value which represents the scale multiplier.

This can be used when developing stacks that are larger than the available screen space - for example developing a stack to be used on an iPad with a retina display.
You can also preview the appearance of your stack on displays with differing display densities.


## Examples:

Scaling a stack to half size:
    set the scaleFactor of stack "myLargeStack" to 0.5

Preview the stack appearance on a high-density display:
    set the scaleFactor of stack "myApp" to 1.5
