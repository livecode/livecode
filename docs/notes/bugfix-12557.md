# Objects which are adjacent don't necessary appear so at non integral scale factors.
At non-integral scale factors (such as 150% Hi-DPI mode on Windows), objects which should appear next to each other can have a visible channel.
This issue isn't completely fixable due to the nature of approximations used when compositing to the screen. However, this problem has been mitigated in a couple of ways - firstly antialiasing is forced on whenever the scale factor is non-integral; secondly clipping rectangles always fall on device pixel boundaries.
