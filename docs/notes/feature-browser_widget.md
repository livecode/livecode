# New Browser Widget

This feature introduces a new embeddable browser implemented as a widget, making it much easier to use than the old revbrowser external.

To add a browser to your application, simply drag and drop the browser widget onto your stack and set the properties you need.

## Properties

* *browserUrl* - The URL of the page displayed in the browser.
* *browserHtmltext* - The HTML source of the content displayed in the browser
* *browserScrollbars* - Whether or not the browser displays scrollbars
* *browserUserAgent* - The identifier sent by the browser when fetching web content.
* *browserJavascriptHandlers* - A list of object script handlers that can be called by javascript code in the page loaded in the browser.

## Messages

* *browserDocumentLoadBegin pUrl* - sent when a new document has completed loading in the browser.
* *browserDocumentLoadComplete pUrl* - sent when a new document has completed loading in the browser.
* *browserDocumentLoadFailed pUrl, pError* - sent when a new document has failed to load in the browser.
* *browserFrameDocumentLoadBegin pUrl* - sent when a new document has completed loading in a frame of the browser.
* *browserFrameDocumentLoadComplete pUrl* - sent when a new document has completed loading in a frame of the browser.
* *browserFrameDocumentLoadFailed pUrl, pError* - sent when a new document has failed to load in a frame of the browser.
* *browserNavigateBegin pUrl* - sent when the browser begins navigation to a new page.
* *browserNavigateComplete pUrl* - sent when the browser successfully navigates to a new page.
* *browserNavigateFailed pUrl, pError* - sent when the browser has failed to navigate to a new page.

## Notes

Browser widget properties are not currently saved, so will need to be set in a startup handler in order to initialize the browser as required. Property loading / saving will be implemented in a future release.
