# Mouse release/touch cancel events incorrectly sending mouseUp message

Previously, mouseUp was being sent when a touch action turned into a scroll (i.e a non-flick scroll). This was in addition to the normal mouseRelease message and only the latter should have been sent. The more responsive touches in iOS7 made this more apparent but it is present in all platforms where mouse release or touch cancel events are generated.

