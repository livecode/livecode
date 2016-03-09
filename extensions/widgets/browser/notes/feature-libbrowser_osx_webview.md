# New Browser Widget Implementation on OSX

This feature adds an additional implementation of the browser widget
based on the WebView component provided as part of OSX's WebKit
framework.

Due to a number of stability issues in CEF on OSX, we have decided to
make this new implementation the default for OSX browser widgets.

# New browserType property

A new **browserType** property has been added to the browser widget to
allow switching to an alternative implementation if required. To
revert to using the CEF-based browser on OSX, set this property to
"CEF".
