# Message box refactor

The way the message box functions has been refactored:

- the IDE only global property `revMessageBoxRedirect` has been removed
- the IDE only global property `revMessageBoxLastObject` has been removed
- the legacy message box behavior setting the text of the first field
of a stack named `Message Box` has been removed
- the `msgChanged` message is now sent to the object that changed the
message
- IDE plugin developers should subscribe to `ideMsgChanged` for custom
message box development.
- If the `msgChanged` message is not handled the content of the 
`message box` will be logged to the system log unless the engine is
running in no ui (command line) mode which will write the content to
STDOUT.
