# Proxy automatic configuration support

LibURL has been updated to add support for proxy auto configuration files (PAC). If no global proxy server has been set using the **HTTPProxy** property, when fetching a URL, LibURL will attempt to parse the systems .pac file in order to extract the proxy server (if any) to use for the given URL.