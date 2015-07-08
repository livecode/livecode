# Add revBrowser error callback messages.

Added two new callbacks sent by revBrowser.
Note that these messages are only sent from browsers opened with revBrowserOpenCEF.

## Callback messages:
* browserDocumentFailed
*	sent to the current card when the browser has encountered an error while loading a URL.
* browserDocumentFailedFrame
*	sent to the current card when the browser has encountered an error while loading a URL into a frame.
