# Remove legacy mergExt externals

The following mergExt deprecated externals are no longer included in
LiveCode.

- mergAES - we have revsecurity based encryption for mobile
- mergDropbox & mergDropboxSync - these use the now abandoned by Dropbox
(v1 API). We have a script library available for v2 API.
- mergSocket - we have sockets in the engine for mobile
- mergZXing - no longer supportable as the ZXing project no longer
supports iOS. Use mergAVCam for barcode capture instead.
