# Fix iOS mobile browser controls not resizing after setting certain properties.

Setting the mobile browser control 'dataDetectorTypes',
'allowsInlineMediaPlayback', and 'mediaPlaybackRequiresUserAction' properties
will no longer prevent the underlying view from resizing to fit the configured
rect of the browser control.
