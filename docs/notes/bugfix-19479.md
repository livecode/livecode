# Cmd-. does not affect modal dialogs

Previously using the abort-script keyboard combination (Cmd-. on Mac) would
cause an abort error to be thrown. However, this would be silently swallowed
by any modal command (or equivalent) meaning that unusable modal dialogs
would be uncloseable, requiring the need to restart the IDE / engine.

This has been fixed by making Cmd-. cause an automatic 'close this stack'
when it occurs in a modal loop and allowInterrupts is true, and the current
stack has cantAbort set to false.

