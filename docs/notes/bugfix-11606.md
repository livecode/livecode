# Text wrapping and metrics issues.
A problem with text measurement meant that in some cases fields were not wrapping text in a consistent nor identical manner across platforms - this has been resolved.
Additionally, metrics on Windows when formatForPrinting is set to true have been adjusted to match Mac. This means that for a stack with formatForPrinting set to true, the text will be rendered with almost identical metrics on Mac and Windows whether on screen or hardcopy.
[ Note that setting formatForPrinting to true on a stack no longer causes display issues on screen on Windows thus - although slower to render - it is a viable property to set all the time if WYSIWYG display is needed ].
