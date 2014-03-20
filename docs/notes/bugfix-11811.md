# Cannot take screen snapshot at high resolution.
The import and export snapshot of screen commands have been updated to allow an 'at size' clause along the same lines as import and export snapshot of object.

If the 'at size' clause is specified, the snapshot of the screen will be taken at device pixel size and then scaled to match the size specified in the at size clause.

For example, to take a snapshot of the screen on a retina display at retina resolution you can use:

    import snapshot from rect 0, 0, 400, 400 at size 800, 800

This will take a snapshot of 400x400 logical pixels, which is 800x800 device pixels and then scale to 800x800 pixels - i.e. the result is you get the all the pixels that have been rendered to the retina display.
