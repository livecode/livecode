# New Browser Widget

A new embeddable browser browser widget has been added, making it much easier to use than the old revbrowser external.

To add a browser to your application, simply drag and drop the browser widget onto your stack and set the properties you need.

## Properties

* **url** - The URL of the page displayed in the browser.
* **htmltext** - The HTML source of the content displayed in the browser
* **vscrollbar** - Whether or not the browser displays a vertical scrollbar
* **hscrollbar** - Whether or not the browser displays a horizontal scrollbar
* **userAgent** - The identifier sent by the browser when fetching web content.
* **javascriptHandlers** - A list of object script handlers that can be called by javascript code in the page loaded in the browser.

## Messages

* **browserDocumentLoadBegin pUrl** - sent when a new document has completed loading in the browser.
* **browserDocumentLoadComplete pUrl** - sent when a new document has completed loading in the browser.
* **browserDocumentLoadFailed pUrl, pError** - sent when a new document has failed to load in the browser.
* **browserFrameDocumentLoadBegin pUrl** - sent when a new document has completed loading in a frame of the browser.
* **browserFrameDocumentLoadComplete pUrl** - sent when a new document has completed loading in a frame of the browser.
* **browserFrameDocumentLoadFailed pUrl, pError** - sent when a new document has failed to load in a frame of the browser.
* **browserNavigateBegin pUrl** - sent when the browser begins navigation to a new page.
* **browserNavigateComplete pUrl** - sent when the browser successfully navigates to a new page.
* **browserNavigateFailed pUrl, pError** - sent when the browser has failed to navigate to a new page.
