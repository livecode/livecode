# Image Filtering Updates

Due to a degrade in the image resizing quality in 6.5, the image filtering algorithms have been updated for 6.6.

For LiveCode versions prior to 6.5, the image filtering algorithms were not universal across all platforms. "Good" resize quality used bilinear filtering and "best" used bicubic filtering for all platforms. However, "normal" differed. On Mac, box filtering was used. All other platforms applied no filtering.

For LiveCode versions prior to 6.5, all resize operations were cached (i.e. moving a resized image around did not cause the resize to be recalculated).

For LiveCode 6.5, the image filtering was united across all platforms, with no filtering being applied in "normal" mode, bilinear filtering being used in "good" mode and bicubic filtering being used in "best" mode.

For LiveCode 6.5, only "best" resize operations were cached (the acceleratedRendering mode should be used for cacheing in other modes). All others were calculated on the fly.

The bilinear filter used in 6.5 was of poorer quality when compared to pre 6.5. Additionally, the "normal" mode on Mac was poorer (due to the loss of the box filter). We've addressed this in LiveCode 6.6 by improving the image filtering algorithms across all platforms. "Normal" and "good" mode now use updated algorithms. "Best" mode remains as before.

It should be noted that the improvements to the filters used may cause a slight drop in performance. The final image filters and resize modes used has not been finalized and is open to user input.