# Removal of CEF support from OS X version of revBrowser external and 8.0 browser widget.

Due to the lack of support for 32-bit Chromium on Mac OS X (and thus Chromium Embedded Framework) we will no longer be using libCEF to provide an embedded browser on OSX, either with the revBrowser external, or the new browser widget.

If you currently use revBrowser and the **revBrowserOpenCEF** command to create the browser, this will no longer function. We advise you to switch to using **revBrowserOpen** when your application is running on OSX.

The browser widget will continue to function as before, as the implementation has already been changed to use the OSX WebView class by default. 
