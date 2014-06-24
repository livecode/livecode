# Improved revBrowser external

The revBrowser external has been updated to support Cocoa on OSX, and now embeds the browser control properly within the window.
In addition a new browser component based on CEF (Chromium Embedded Framework) has been added.

This new browser allows for a consistent appearance across all platforms with a modern, well supported feature set.

To use the new CEF browser use the *revBrowserOpenCef* command in place of *revBrowserOpen*. This will create a CEF browser instance which can be used with the existing revBrowser commands and functions in exactly the same way as before.

## JavaScript integration

The new chrome browser allows us to add the ability to call LiveCode handlers from within the browser using JavaScript. To make a LiveCode handler visible to JavaScript, use the *revBrowserAddJavaScriptHandler* command, and to remove it use the *revBrowserRemoveJavaScriptHandler* command. LiveCode handlers are added as functions with the same name attached to a global 'liveCode' object. When called, these functions will result in the corresponding LiveCode handler message being sent to the browser card with the browser instance ID and any function arguments as parameters.

### Example:

With the handler "myJSHandler" registered using *revBrowserAddJavaScriptHandler*, it can be called from the browser like so:

liveCode.myJSHandler(tFieldContents, tAction);


the LiveCode handler would then be called with the following parameters:
* pBrowserInstance (the browser instance id, as returned from the *revOpenBrowserCef* function)
* pFieldContents (the first argument of the JavaScript function call)
* pAction (the second argument of the JavaScript function call)

