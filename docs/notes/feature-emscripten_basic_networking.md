# HTML5 Basic Networking Support

Basic networking support has been added to the HTML5 engine by way of the **load** command. The HTML5 **load** command functions in the same way as the **load** command on mobile platforms, with a completion message being sent on download, containing the contents of the URL (or any error) and the **urlProgress** message being sent periodically during the request.

** Note: The current HTML5 support for 'load url' is experimental, and will be replaced by a libUrl-like API in a subsequent DP ensuring it has parity with other platforms when 'Internet Library' is used.**

**Note - only HTTP and HTTPS protocols are supported and URLs can only be fetched from the domain hosting the web page running the HTML5 engine.**

```
command fetchURL pURL
   load URL pURL with "loadComplete"
end fetchURL

on loadComplete pURL, pStatus, pData, pTotal
   -- pURL - The URL being fetched.
   --
   -- pStatus - The status of the URL: One of:
   --  * downloaded
   --  * error
   --
   -- pData - This will be:
   --  * the content of the URL, if pStatus is downloaded
   --  * the error string, if pStatus is error
   --
   -- pTotal - The total size of the URL, in bytes.
end loadComplete

on urlProgress pURL, pStatus, pData, pTotal
   -- pURL - The URL being fetched.
   --
   -- pStatus - The current status of the operation. One of:
   --  * contacted
   --  * requested
   --  * loading
   --  * downloaded
   --  * error
   --
   -- pData - This will be:
   --  * the number of bytes fetched, if pStatus is loading
   --  * the error string, if pStatus is error
   --  * empty in all other cases
   --
   -- pTotal - The total size of the URL, in bytes.
end urlProgress
```
