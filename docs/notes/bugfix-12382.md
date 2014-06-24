# Normal resizeQuality is slow

As part of the update to image filters in the 6.6 release, we improved the quality of the resizing and rotating images when the resizeQuality was set to "normal". This brought all platforms into line with the way things were on Mac pre 6.5.

However, this change in image filter meant that resizing of images was more processor intensive and the resulting output was much smoother. As developers using the "normal" resizeQuality relied on the time and output of the resize operation, we've decided to temporarily revert the "normal" behavior back to how things were in 6.5.

This change is only temorary, with there being plans to fully address the issue in a future release where the resizeQulaity options will undergo and overhaul in order to provide the developer with greater flexibility.
