script "com.livecode.library.dropbox"
/*
Copyright (c) 2016 Gerard McCarthy
The MIT License (MIT)
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

on extensionInitialize
   if the target is not me then
      pass extensionInitialize
   end if
   
   insert the script of me into back
   
   if the environment contains "development" then
      set the _ideoverride of me to true
      local tName
      put the short name of me into tName
      revSBAddDependencyForInclusion "extensions", tName, "scriptLibraries", "Internet"
   end if
   
end extensionInitialize

on extensionFinalize
   if the target is not me then
      pass extensionFinalize
   end if
   
   remove the script of me from back
end extensionFinalize

/**
Title: Dropbox Library

Author: LiveCode

Version: 2.0.0

SVGIcon: M402 717L896 1022 554 1307 64 988ZM1388 1272L1388 1380 898 1673 898 1674 897 1673 896 1674 896 1673 407 1380 407 1272 554 1368 896 1084 896 1082 897 1083 898 1082 898 1084 1241 1368ZM554 128L896 413 402 717 64 447ZM1390 717L1728 988 1239 1307 896 1022ZM1239 128L1728 447 1390 717 896 413Z

Description:
Implements the Dropbox v2 API.
Doesn't include business pro API.
*/

/**

Get information about a user's account.

Syntax: dropboxGetAccount <pAccessToken>, <pUserAccountID>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pUserAccountID: The user's unique Dropbox ID

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"account_id": string,
"name": {
"given_name": string,
"surname": string,
"familiar_name": string,
"display_name": string,
"abbreviated_name": string
},
"email": string,
"email_verified": boolean,
"disabled": boolean,
"is_teammate": boolean,
"profile_photo_url": string
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/

command dropboxGetAccount pAccessToken, pUserAccountID, pCallback
   local tHeader, tPost, tURL
   put __get_account_HEADER(pAccessToken) into tHeader
   put __get_account_POST(pUserAccountID) into tPost
   put __get_account_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetAccount

/**

Get information about multiple user accounts. At most 300 accounts may be queried per request.

Syntax: dropboxGetAccountBatch <pAccessToken>, <pUserAccountIDs>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pUserAccountIDs: A return delimited list of unique Dropbox IDs.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
[
{
"account_id": string,
"name": {
"given_name": string,
"surname": string,
"familiar_name": string,
"display_name": string,
"abbreviated_name": string
},
"email": string,
"email_verified": boolean,
"disabled": boolean,
"is_teammate": boolean,
"profile_photo_url": string
}
]
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/

command dropboxGetAccountBatch pAccessToken, pUserAccountIDs, pCallback
   local tHeader, tPost, tURL
   put __get_account_batch_HEADER(pAccessToken) into tHeader
   put __get_account_batch_POST(pUserAccountIDs) into tPost
   put __get_account_batch_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetAccountBatch

/**

Get information about the current user's account.

Syntax: dropboxGetCurrentAccount <pAccessToken>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"account_id": string,
"name": {
"given_name": string,
"surname": string,
"familiar_name": string,
"display_name": string,
"abbreviated_name": string
},
"email": string,
"email_verified": boolean,
"disabled": boolean,
"is_teammate": boolean,
"profile_photo_url": string
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxGetCurrentAccount pAccessToken, pCallback
   local tHeader, tPost, tURL
   put __get_current_account_HEADER(pAccessToken) into tHeader
   put __get_current_account_POST() into tPost
   put __get_current_account_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetCurrentAccount

/**

Get the space usage information for the current user's account.

Syntax: dropboxGetSpaceUsage <pAccessToken>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"used": 314159265,
"allocation": {
".tag": "individual",
"allocated": 10000000000
}
}
````
*/

command dropboxGetSpaceUsage pAccessToken, pCallback
   local tHeader, tPost, tURL
   put __get_space_usage_HEADER(pAccessToken) into tHeader
   put __get_space_usage_POST() into tPost
   put __get_space_usage_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetSpaceUsage

/**

Copy a file or folder to a different location in the user's Dropbox.
If the source path is a folder all its contents will be copied.

Syntax: dropboxCopy <pAccessToken>, <pPathFrom>, <pPathTo>, <pAllowSharedFolder>, <pAutoRename>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPathFrom: Path in the user's Dropbox to be copied.

pPathTo: Path in the user's Dropbox that is the destination.

pAllowSharedFolder (boolean): If true, copy will copy contents in shared folder, otherwise RelocationError.cant_copy_shared_folder will be returned if pFromPath contains shared folder. The default for this field is false.

pAutoRename (boolean): If there's a conflict, have the Dropbox server try to autorename the file to avoid the conflict. The default for this field is false.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": string,
"name": string,
"id": string,
"client_modified": string,
"server_modified": string,
"rev": string,
"size": integer,
"path_lower": string,
"path_display": string,
"sharing_info": {
"read_only": boolean,
"parent_shared_folder_id": string,
"modified_by": string
},
"property_groups": [
{
"template_id": string,
"fields": [
{
"name": string,
"value": string
}
]
}
],
"has_explicit_shared_members": boolean
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxCopy pAccessToken, pPathFrom, pPathTo, pAllowSharedFolder, pAutoRename, pCallback
   local tHeader, tPost, tURL
   put __copy_HEADER(pAccessToken) into tHeader
   put __copy_POST(pPathFrom, pPathTo, pAllowSharedFolder, pAutoRename) into tPost
   put __copy_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxCopy

/**

Create a folder at a given path.

Syntax: dropboxCreateFolder <pAccessToken>, <pPath>, <pAutoRename>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to be created.

pAutoRename (boolean): If there's a conflict, have the Dropbox server try to autorename the file to avoid the conflict. The default for this field is false.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"name": string,
"id": string,
"path_lower": string,
"path_display": string,
"sharing_info": {
"read_only": boolean,
"parent_shared_folder_id": string,
"traverse_only": boolean,
"no_access": boolean
},
"property_groups": [
{
"template_id": string,
"fields": [
{
"name": string,
"value": string
}
]
}
]
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxCreateFolder pAccessToken, pPath, pAutoRename, pCallback
   local tHeader, tPost, tURL
   put __create_folder_HEADER(pAccessToken) into tHeader
   put __create_folder_POST(pPath, pAutoRename) into tPost
   put __create_folder_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxCreateFolder

/**

Delete the file or folder at a given path.

Syntax: dropboxDelete <pAccessToken>, <pPath>, [<pCallback>]

Description:
If the path is a folder, all its contents will be deleted too.
A successful response indicates that the file or folder was deleted. The returned
metadata will be the corresponding FileMetadata or FolderMetadata for the item at
time of deletion, and not a DeletedMetadata object.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to be deleted.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": string,
"name": string,
"id": string,
"client_modified": string,
"server_modified": string,
"rev": string,
"size": integer,
"path_lower": string,
"path_display": string,
"sharing_info": {
"read_only": boolean,
"parent_shared_folder_id": string,
"modified_by": string
},
"property_groups": [
{
"template_id": string,
"fields": [
{
"name": string,
"value": string
}
]
}
],
"has_explicit_shared_members": boolean
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxDelete pAccessToken, pPath, pCallback
   local tHeader, tPost, tURL
   put __delete_HEADER(pAccessToken) into tHeader
   put __delete_POST(pPath) into tPost
   put __delete_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxDelete

/**

Download a file from a user's Dropbox.

Syntax: dropboxDownload <pAccessToken>, <pPath>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to be download.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be the binary data of the file.

If an error occurred it will be a JSON string in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxDownload pAccessToken, pPath, pCallback
   /*
   DownloadArg
   path String The path of the file to download.
   {"path": "/apps/MyApp/Prime_Numbers.txt"}
   Example: id {"path": "id:a4ayc_80_OEAAAAAAAAAYa"}
   Example: rev  {"path": "rev:a1c10ce0dd78"}
   */
   local tHeader, tPost, tURL
   put __download_HEADER(pAccessToken,pPath) into tHeader
   put __download_POST() into tPost
   put __download_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxDownload

/**

Returns the metadata for a file or folder.

Syntax: dropboxGetMetadata <pAccessToken>, <pPath>, <pIncludeMediaInfo>, [<pCallback>]

Description:

>*Note:* Metadata for the root folder is unsupported.

If the path is a folder, then a folderMetadata json is returned.
If the path is a file then a fileMetadata json is returned.
If the path is a file or folder that has been deleted, then a deletedMetadata json is returned.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path to a file or folder in the user's Dropbox to be queried

pIncludeMediaInfo (boolean): If true, FileMetadata.media_info is set for photo and
video. The default for this field is False.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": string,
"name": string,
"id": string,
"client_modified": string,
"server_modified": string,
"rev": string,
"size": integer,
"path_lower": string,
"path_display": string,
"sharing_info": {
"read_only": boolean,
"parent_shared_folder_id": string,
"modified_by": string
},
"property_groups": [
{
"template_id": string,
"fields": [
{
"name": string,
"value": string
}
]
}
],
"has_explicit_shared_members": boolean
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxGetMetadata pAccessToken, pPath, pIncludeMediaInfo, pCallback
   local tHeader, tPost, tURL
   put __get_metadata_HEADER(pAccessToken) into tHeader
   put __get_metadata_POST(pPath,pIncludeMediaInfo) into tPost
   put __get_metadata_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetMetadata

/**

Get a preview for a file.

Syntax: dropboxGetPreview <pAccessToken>, <pPath>, [<pCallback>]

Description:
Currently previews are only generated for the files with the following extensions: 
.doc, .docx, .docm, .ppt, .pps, .ppsx, .ppsm, .pptx, .pptm, .xls, .xlsx, .xlsm, .rtf.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to be deleted.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will contain the data of the preview PDF.

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxGetPreview pAccessToken, pPath, pCallback
   local tHeader, tPost, tURL
   put __get_preview_HEADER(pAccessToken,pPath) into tHeader
   put __get_preview_POST() into tPost
   put __get_preview_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetPreview

/**

Get a thumbnail for an image.

Syntax: dropboxGetThumbnail <pAccessToken>, <pPath>, <pFormat>, <pSize>, [<pCallback>]

Description:
This method currently supports files with the following file extensions:
jpg, jpeg, png, tiff, tif, gif and bmp. Photos that are larger than 20MB in
size won't be converted to a thumbnail.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to be deleted.

pFormat (enum): The format for the thumbnail image

- "jpeg"
- "png"

pSize (enum): The value will be one of the following datatypes:

- "w32h32": 32 by 32 px.
- "w64h64": 64 by 64 px.
- "w128h128": 128 by 128 px.
- "w640h480": 640 by 480 px.
- "w1024h768": 1024 by 768 px.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will contain the data of the thumbnail image.

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxGetThumbnail pAccessToken, pPath, pFormat, pSize, pCallback
   local tHeader, tPost, tURL
   put __get_thumbnail_HEADER(pAccessToken,pPath,pFormat,pSize) into tHeader
   put __get_thumbnail_POST() into tPost
   put __get_thumbnail_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetThumbnail

/**

Starts returning the contents of a folder. 

Syntax: dropboxListFolder <pAccessToken>, <pPath>, <pRecursive>, <pIncludeMediaInfo>, [<pCallback>]

Description:
If the result's ListFolderResult.has_more field is true, call list_folder/continue with 
the returned ListFolderResult.cursor to retrieve more entries.

If you're using ListFolderArg.recursive set to true to keep a local cache of the contents
of a Dropbox account, iterate through each entry in order and process them as follows to
keep your local state in sync:

- For each FileMetadata, store the new entry at the given path in your local state. 
If the required parent folders don't exist yet, create them. If there's already something
else at the given path, replace it and remove all its children.
- For each FolderMetadata, store the new entry at the given path in your local state. If the
required parent folders don't exist yet, create them. If there's already something else at 
the given path, replace it but leave the children as they are. Check the new entry's 
FolderSharingInfo.read_only and set all its children's read-only statuses to match.
- For each DeletedMetadata, if your local state has something at the given path, remove it and 
all its children. If there's nothing at the given path, ignore this entry.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to be deleted.

pRecursive (boolean): If true, the list folder operation will be applied recursively to all subfolders and
the response will contain contents of all subfolders. The default for this field is False.

pIncludeMediaInfo (boolean): If true, FileMetadata.media_info is set for photo and
video. The default for this field is False.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"entries": [
{
".tag": "file",
"name": "Prime_Numbers.txt",
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"path_lower": "/homework/math/prime_numbers.txt",
"path_display": "/Homework/math/Prime_Numbers.txt",
"sharing_info": {
"read_only": true,
"parent_shared_folder_id": "84528192421",
"modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
],
"has_explicit_shared_members": false
},
{
".tag": "folder",
"name": "math",
"id": "id:a4ayc_80_OEAAAAAAAAAXz",
"path_lower": "/homework/math",
"path_display": "/Homework/math",
"sharing_info": {
"read_only": false,
"parent_shared_folder_id": "84528192421",
"traverse_only": false,
"no_access": false
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
]
}
],
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu",
"has_more": false
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListFolder pAccessToken, pPath, pRecursive, pIncludeMediaInfo, pCallback
   local tHeader, tPost, tURL
   put __list_folder_HEADER(pAccessToken) into tHeader
   put __list_folder_POST(pPath,pRecursive,pIncludeMediaInfo) into tPost
   put __list_folder_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListFolder

/**

Once a cursor has been retrieved from list_folder, use this to paginate through all files and
retrieve updates to the folder, following the same rules as documented for list folder.

Syntax: dropboxListFolderContinue <pAccessToken>, <pCursor>, [<pCallback>]

Description:
If the result's ListFolderResult.has_more field is true, call list_folder/continue with 
the returned ListFolderResult.cursor to retrieve more entries.

If you're using ListFolderArg.recursive set to true to keep a local cache of the contents
of a Dropbox account, iterate through each entry in order and process them as follows to
keep your local state in sync:

- For each FileMetadata, store the new entry at the given path in your local state. 
If the required parent folders don't exist yet, create them. If there's already something
else at the given path, replace it and remove all its children.
- For each FolderMetadata, store the new entry at the given path in your local state. If the
required parent folders don't exist yet, create them. If there's already something else at 
the given path, replace it but leave the children as they are. Check the new entry's 
FolderSharingInfo.read_only and set all its children's read-only statuses to match.
- For each DeletedMetadata, if your local state has something at the given path, remove it and 
all its children. If there's nothing at the given path, ignore this entry.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pCursor: The cursor returned by your last call to list folder or list folder continue.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"entries": [
{
".tag": "file",
"name": "Prime_Numbers.txt",
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"path_lower": "/homework/math/prime_numbers.txt",
"path_display": "/Homework/math/Prime_Numbers.txt",
"sharing_info": {
"read_only": true,
"parent_shared_folder_id": "84528192421",
"modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
],
"has_explicit_shared_members": false
},
{
".tag": "folder",
"name": "math",
"id": "id:a4ayc_80_OEAAAAAAAAAXz",
"path_lower": "/homework/math",
"path_display": "/Homework/math",
"sharing_info": {
"read_only": false,
"parent_shared_folder_id": "84528192421",
"traverse_only": false,
"no_access": false
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
]
}
],
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu",
"has_more": false
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListFolderContinue pAccessToken, pCursor, pCallback
   local tHeader, tPost, tURL
   put __list_folder_continue_HEADER(pAccessToken) into tHeader
   put __list_folder_continue_POST(pCursor) into tPost
   put __list_folder_continue_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListFolderContinue

/**

A way to get a cursor for the folder's state.

Syntax: dropboxListFolderGetLatestCursor <pAccessToken>, <pPath>, <pRecursive>, <pIncludeMediaInfo>, [<pCallback>]

Description:
Unlike dropboxListFolder, dropboxListFolderGetLatestCursor doesn't return any entries. This endpoint is 
for app which only needs to know about new files and modifications and doesn't need to know
about files that already exist in Dropbox. Use the returned cursor with dropboxListFolderContinue to
obtain the entries.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: The path to the folder you want to see the contents of.

pRecursive (boolean): If true, the list folder operation will be applied recursively to all subfolders and
the response will contain contents of all subfolders. The default for this field is False.

pIncludeMediaInfo (boolean): If true, FileMetadata.media_info is set for photo and
video. The default for this field is False.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListFolderGetLatestCursor pAccessToken, pPath, pRecursive, pIncludeMediaInfo, pCallback
   local tHeader, tPost, tURL
   put __list_folder_get_latest_cursor_HEADER(pAccessToken) into tHeader
   put __list_folder_get_latest_cursor_POST(pPath,pRecursive,pIncludeMediaInfo) into tPost
   put __list_folder_get_latest_cursor_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListFolderGetLatestCursor

/**

A longpoll endpoint to wait for changes on an account.

Syntax: dropboxListFolderLongPoll <pCursor>, <pTimeout>, [<pCallback>]

Description:
In conjunction with dropboxListFolderContinue, this call gives you a low-latency way to monitor an account
for file changes. The connection will block until there are changes available or a timeout occurs.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pCursor: A cursor as returned by dropboxListFolder or dropboxListFolderContinue. Cursors retrieved 
by setting ListFolderArg.include_media_info to true are not supported.

pTimeOut: A timeout in seconds. The request will block for at most this length of time, plus up to
90 seconds of random jitter added to avoid the thundering herd problem. Care should be taken when
using this parameter, as some network infrastructure does not support long timeouts. The default for
this field is 30.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"changes": true
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListFolderLongPoll pCursor, pTimeout, pCallback
   local tHeader, tPost, tURL
   put __list_folder_longpoll_HEADER() into tHeader
   put __list_folder_longpoll_POST(pCursor,pTimeout) into tPost
   put __list_folder_longpoll_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListFolderLongPoll

/**

Return revisions of a file.

Syntax: dropboxListRevisions <pAccessToken>, <pPath>, <pLimit>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: The path to the file you want to see the revisions of.

pLimit: The maximum number of revision entries returned. The default for this field is 10.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"is_deleted": false,
"entries": [
{
"name": "Prime_Numbers.txt",
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"path_lower": "/homework/math/prime_numbers.txt",
"path_display": "/Homework/math/Prime_Numbers.txt",
"sharing_info": {
"read_only": true,
"parent_shared_folder_id": "84528192421",
"modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
],
"has_explicit_shared_members": false
}
]
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListRevisions pAccessToken, pPath, pLimit, pCallback
   local tHeader, tPost, tURL
   put __list_revisions_HEADER(pAccessToken) into tHeader
   put __list_revisions_POST(pPath,pLimit) into tPost
   put __list_revisions_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListRevisions

/**

Move a file or folder to a different location in the user's Dropbox.

Syntax: dropboxMove <pAccessToken>, <pPathFrom>, <pPathTo>, <pAllowSharedFolder>, <pAutoRename>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPathFrom: Path in the user's Dropbox to be moved.

pPathTo: Path in the user's Dropbox that is the destination.

pAllowSharedFolder (boolean): If true, copy will copy contents in shared folder, otherwise RelocationError.cant_copy_shared_folder will be returned if pFromPath contains shared folder. The default for this field is false.

pAutoRename (boolean): If there's a conflict, have the Dropbox server try to autorename the file to avoid the conflict. The default for this field is false.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "file",
"name": "Prime_Numbers.txt",
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"path_lower": "/homework/math/prime_numbers.txt",
"path_display": "/Homework/math/Prime_Numbers.txt",
"sharing_info": {
"read_only": true,
"parent_shared_folder_id": "84528192421",
"modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
],
"has_explicit_shared_members": false
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxMove pAccessToken, pPathFrom, pPathTo, pCallback
   local tHeader, tPost, tURL
   put __move_HEADER(pAccessToken) into tHeader
   put __move_POST(pPathFrom,pPathTo) into tPost
   put __move_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxMove

/**

Permanently delete the file or folder at a given path.

Syntax: dropboxPermanentlyDelete <pAccessToken>, <pPath>, [<pCallback>]

Description:
> *Note:* This endpoint is only available for Dropbox Indy apps.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to be deleted.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function.

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxPermanentlyDelete pAccessToken, pPath, pCallback
   local tHeader, tPost, tURL
   put __permanently_delete_HEADER(pAccessToken) into tHeader
   put __permanently_delete_POST(pPath) into tPost
   put __permanently_delete_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxPermanentlyDelete

/**

Restore a file to a specific revision.

Syntax: dropboxRestore <pAccessToken>, <pPath>, <pRevision>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to be restored.

pRevision: The revision to restore for the file.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"name": "Prime_Numbers.txt",
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"path_lower": "/homework/math/prime_numbers.txt",
"path_display": "/Homework/math/Prime_Numbers.txt",
"sharing_info": {
"read_only": true,
"parent_shared_folder_id": "84528192421",
"modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
],
"has_explicit_shared_members": false
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxRestore pAccessToken, pPath, pRevision, pCallback
   local tHeader, tPost, tURL
   put __restore_HEADER(pAccessToken) into tHeader
   put __restore_POST(pPath,pRevision) into tPost
   put __restore_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxRestore

/**

Searches for files and folders.

Syntax: dropboxSearch <pAccessToken>, <pPath>, <pQuery>, <pStart>, <pMaxResults>, <pMode>, [<pCallback>]

Description:
> *Note:* Recent changes may not immediately be reflected in search results due
> to a short delay in indexing.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to be searched.

pQuery: The string to search for. The search string is split on spaces into multiple tokens.
For file name searching, the last token is used for prefix matching (i.e. "bat c" matches 
"bat cave" but not "batman car").

pStart: The starting index within the search results (used for paging). The default 
for this field is 0.

pMaxResults: The maximum number of search results to return. The default for this field is 100.

pMode (enum): The search mode. Note that searching file content is only available for Dropbox Business accounts.

- "filename"
- "filename_and_content"
- "deleted_filename"

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"matches": [
{
"match_type": {
".tag": "content"
},
"metadata": {
".tag": "file",
"name": "Prime_Numbers.txt",
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"path_lower": "/homework/math/prime_numbers.txt",
"path_display": "/Homework/math/Prime_Numbers.txt",
"sharing_info": {
"read_only": true,
"parent_shared_folder_id": "84528192421",
"modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
],
"has_explicit_shared_members": false
}
}
],
"more": false,
"start": 1
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxSearch pAccessToken, pPath, pQuery, pStart, pMaxResults, pMode, pCallback
   local tHeader, tPost, tURL
   put __search_HEADER(pAccessToken) into tHeader
   put __search_POST(pPath,pQuery,pStart,pMaxResults,pMode) into tPost
   put __search_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxSearch

/**

Create a new file with the contents provided in the request.

Syntax: dropboxUpload <pAccessToken>, <pPath>, <pMode>, <pAutorename>, <pMute>, <pData>, [<pCallback>]

Description:
> *Note:* Do not use this to upload a file larger than 150 MB. Instead, create an upload 
> session with dropboxUploadSessionStart.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: Path in the user's Dropbox to upload to.

pMode (enum): Selects what to do if the file already exists. The default for this union is add.

- "add": Do not overwrite an existing file if there is a conflict. The autorename strategy is 
to append a number to the file name. For example, "document.txt" might become "document (2).txt".
- "overwrite": Always overwrite the existing file. The autorename strategy is the same as it is for add.
- "update": Overwrite if the given "rev" matches the existing file's "rev". The autorename 
strategy is to append the string "conflicted copy" to the file name. For example, "document.txt" 
might become "document (conflicted copy).txt" or "document (Panda's conflicted copy).txt".

pAutoRename: If there's a conflict, as determined by mode, have the Dropbox server try to autorename
the file to avoid conflict. The default for this field is False.

pMute: Normally, users are made aware of any file modifications in their Dropbox account via
notifications in the client software. If true, this tells the clients that this modification 
shouldn't result in a user notification. The default for this field is False

pData: The binary file data

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"name": "Prime_Numbers.txt",
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"path_lower": "/homework/math/prime_numbers.txt",
"path_display": "/Homework/math/Prime_Numbers.txt",
"sharing_info": {
"read_only": true,
"parent_shared_folder_id": "84528192421",
"modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
],
"has_explicit_shared_members": false
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxUpload pAccessToken, pPath, pMode, pAutorename, pMute, pData, pCallback
   local tHeader, tPost, tURL
   put __upload_HEADER(pAccessToken,pPath,pMode,pAutorename,pMute) into tHeader
   put __upload_POST(pData) into tPost
   put __upload_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxUpload

/**

Append more data to an upload session.

Syntax: dropboxUploadSessionAppend <pAccessToken>, <pSession>, <pOffset>, <pData>, [<pCallback>]

Description:
> *Note:* A single request should not upload more than 150 MB of file contents.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSession: A session ID as returned by dropboxUploadSessionStart.

pOffset: The amount of data that has been uploaded so far. We use this to
make sure upload data isn't lost or duplicated in the event of a network error.

pData: The binary file data

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. 

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxUploadSessionAppend pAccessToken, pSession, pOffset, pData, pCallback
   local tHeader, tPost, tURL
   put __upload_session_append_HEADER(pAccessToken,pSession,pOffset) into tHeader
   put __upload_session_append_POST(pData) into tPost
   put __upload_session_append_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxUploadSessionAppend

/**

Finish an upload session and save the uploaded data to the given file path.

Syntax: dropboxUploadSessionFinish <pAccessToken>, <pSession>, <pOffset>, <pPath>, <pMode>, <pAutorename>, <pMute>, <pData>, [<pCallback>]

Description:
> *Note:* A single request should not upload more than 150 MB of file contents.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSession: A session ID as returned by dropboxUploadSessionStart.

pOffset: The amount of data that has been uploaded so far. We use this to
make sure upload data isn't lost or duplicated in the event of a network error.

pPath: Path in the user's Dropbox to upload to.

pMode (enum): Selects what to do if the file already exists. The default for this union is add.

- "add": Do not overwrite an existing file if there is a conflict. The autorename strategy is 
to append a number to the file name. For example, "document.txt" might become "document (2).txt".
- "overwrite": Always overwrite the existing file. The autorename strategy is the same as it is for add.
- "update": Overwrite if the given "rev" matches the existing file's "rev". The autorename 
strategy is to append the string "conflicted copy" to the file name. For example, "document.txt" 
might become "document (conflicted copy).txt" or "document (Panda's conflicted copy).txt".

pAutoRename: If there's a conflict, as determined by mode, have the Dropbox server try to autorename
the file to avoid conflict. The default for this field is False.

pMute: Normally, users are made aware of any file modifications in their Dropbox account via
notifications in the client software. If true, this tells the clients that this modification 
shouldn't result in a user notification. The default for this field is False

pData: The binary file data

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"name": "Prime_Numbers.txt",
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"path_lower": "/homework/math/prime_numbers.txt",
"path_display": "/Homework/math/Prime_Numbers.txt",
"sharing_info": {
"read_only": true,
"parent_shared_folder_id": "84528192421",
"modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
},
"property_groups": [
{
"template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
"fields": [
{
"name": "Security Policy",
"value": "Confidential"
}
]
}
],
"has_explicit_shared_members": false
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxUploadSessionFinish pAccessToken, pSession, pOffset, pPath, pMode, pAutorename, pMute, pData, pCallback
   local tHeader, tPost, tURL
   put __upload_session_finish_HEADER(pAccessToken,pSession,pOffset,pPath,pMode,pAutorename,pMute) into tHeader
   put __upload_session_finish_POST(pData) into tPost
   put __upload_session_finish_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxUploadSessionFinish

/**

Upload sessions allow you to upload a single file in one or more requests, for example where the size of the
file is greater than 150 MB.

Syntax: dropboxUploadSessionStart <pAccessToken>, <pData>, [<pCallback>]

Description:
This call starts a new upload session with the given data. You can then use 
dropboxUploadSessionAppend with the returned session id to add more data and 
dropboxUploadSessionFinish to save all the data to a file in Dropbox. 

> *Note:* A single request should not upload more than 150 MB of file contents.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pData: The binary file data

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"session_id": "1234faaf0678bcde"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxUploadSessionStart pAccessToken, pData, pCallback
   local tHeader, tPost, tURL
   put __upload_session_start_HEADER(pAccessToken) into tHeader
   put __upload_session_start_POST(pData) into tPost
   put __upload_session_start_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxUploadSessionStart

/**

Allows an owner or editor (if the ACL update policy allows) of a shared folder to add another member.

Syntax: dropboxAddFolderMember <pAccessToken>, <pSharedFolderId>, <pMembers>, <pQuiet>, <pCustomMessage>, [<pCallback>]

Description:
For the new member to get access to all the functionality for this folder, you will need 
to call dropboxMountFolder on their behalf. Apps must have full Dropbox access to use 
this endpoint.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderId: The ID for the shared folder.

pMembers (array): The intended list of members to add. Added members will
receive invites to join the shared folder.

````
[
{
"email" : email address of member (optional if dropbox_id is set)
"drobox_id" : a unique dropbox user id
"access_level" (enum): 
- "owner" : The collaborator is the owner of the shared folder. Owners can 
view and edit the shared folder as well as set the folder's policies using 
update_folder_policy.
- "editor: The collaborator can both view and edit the shared folder.
- "viewer": The collaborator can only view the shared folder.
- "viewer_no_comment": The collaborator can only view the shared folder 
and does not have any access to comments.
}
]
````

pQuiet (boolean): Whether added members should be notified via email and 
device notifications of their invite. The default for this field is False.

pCustomMessage: Optional message to display to added members in their invitation.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If an error occurred the JSON string will be in
the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxAddFolderMember pAccessToken, pSharedFolderId, pMembers, pQuiet, pCustomMessage, pCallback
   --members as an array
   local tHeader, tPost, tURL
   put __add_folder_member_HEADER(pAccessToken) into tHeader
   put __add_folder_member_POST(pSharedFolderId,pMembers,pQuiet,pCustomMessage) into tPost
   put __add_folder_member_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxAddFolderMember

/**

Returns the status of an asynchronous job.

Syntax: dropboxCheckJobStatus <pAccessToken>, <pAsyncJobId>, [<pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pAsyncJobID:  Id of the asynchronous job. This is the value of a response returned
from the method that launched the job.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "in_progress"
}
````

The tag will be one of:

- in_progress - The asynchronous job is still in progress.
- complete - The asynchronous job has finished.
- failed - The asynchronous job returned an error.

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxCheckJobStatus pAccessToken, pAsyncJobId, pCallback
   local tHeader, tPost, tURL
   put __check_job_status_HEADER(pAccessToken) into tHeader
   put __check_job_status_POST(pAsyncJobId) into tPost
   put __check_job_status_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxCheckJobStatus

/**

Returns the status of an asynchronous job for sharing a folder.

Syntax: dropboxCheckShareJobStatus <pAccessToken>, <pAsyncJobId>, [<pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pAsyncJobID:  Id of the asynchronous job. This is the value of a response returned
from the method that launched the job.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "in_progress"
}
````

The tag will be one of:

- in_progress - The asynchronous job is still in progress.
- complete - The asynchronous job has finished.
- failed - The asynchronous job returned an error.

If complete the JSON includes the metadata of the shared folder:

````
{
".tag": "complete",
"access_type": {
".tag": "owner"
},
"is_team_folder": false,
"policy": {
"acl_update_policy": {
".tag": "owner"
},
"shared_link_policy": {
".tag": "anyone"
},
"member_policy": {
".tag": "anyone"
},
"resolved_member_policy": {
".tag": "team"
}
},
"name": "dir",
"shared_folder_id": "84528192421",
"time_invited": "2016-01-20T00:00:00Z",
"preview_url": "https://www.dropbox.com/scl/fo/fir9vjelf",
"path_lower": "/dir",
"permissions": []
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxCheckShareJobStatus pAccessToken, pAsyncJobId, pCallback
   local tHeader, tPost, tURL
   put __check_share_job_status_HEADER(pAccessToken) into tHeader
   put __check_share_job_status_POST(pAsyncJobId) into tPost
   put __check_share_job_status_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxCheckShareJobStatus

/**

Create a shared link with custom settings

Syntax: dropboxCreateSharedLinkWithSettings <pAccessToken>, <pPath>, [<pSettings>, <pCallback>]

Description:
If no settings are given then the default visibility is RequestedVisibility.public 
(The resolved visibility, though, may depend on other aspects such as team and 
shared folder settings).

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: The path to be shared by the shared link

pSettings (optional array): 
````
{
"requested_visibility" (enum): The requested access for this shared link. This field is optional.

- "public": Anyone who has received the link can access it. No login required.
- "team_only": Only members of the same team can access the link. Login is required.
- "password": A link-specific password is required to access the link. Login is not required.
"link_password": If requested_visibility is password this is needed to specify the password to access the link. This field is optional.
"expires": format="%Y-%m-%dT%H:%M:%SZ" - Expiration time of the shared link. By default the link won't expire. This field is optional.
}
````

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "file",
"url": "https://www.dropbox.com/s/2sn712vy1ovegw8/Prime_Numbers.txt?dl=0",
"name": "Prime_Numbers.txt",
"link_permissions": {
"can_revoke": false,
"resolved_visibility": {
".tag": "public"
},
"revoke_failure_reason": {
".tag": "owner_only"
}
},
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"path_lower": "/homework/math/prime_numbers.txt",
"team_member_info": {
"team_info": {
"id": "dbtid:AAFdgehTzw7WlXhZJsbGCLePe8RvQGYDr-I",
"name": "Acme, Inc."
},
"display_name": "Roger Rabbit",
"member_id": "dbmid:abcd1234"
}
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxCreateSharedLinkWithSettings pAccessToken, pPath, pSettings, pCallback
   local tHeader, tPost, tURL
   put __create_shared_link_with_settings_HEADER(pAccessToken) into tHeader
   put __create_shared_link_with_settings_POST(pPath,pSettings) into tPost
   put __create_shared_link_with_settings_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxCreateSharedLinkWithSettings

/**

Returns shared folder metadata by its folder ID.

Syntax: dropboxGetFolderMetadata <pAccessToken>, <pSharedFolderId>, [<pActions>, <pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderID: The ID for the shared folder.

pActions: This is a return delimited list indicating whether the returned folder data will
include a boolean value FolderPermission.allow that describes whether the current user can 
perform the folder action on the folder. This field is optional.

Actions that may be taken on shared folders. The value will be one of the following datatypes.
New values may be introduced as our API evolves.

- "change_options": Change folder options, such as who can be invited to join the folder.
- "edit_contents": Change or edit contents of the folder.
- "invite_editor": Invite a user or group to join the folder with read and write permission.
- "invite_viewer": Invite a user or group to join the folder with read permission.
- "invite_viewer_no_comment": Invite a user or group to join the folder with read permission but no comment permissions.
- "relinquish_membership": Relinquish one's own membership in the folder.
- "unmount": Unmount the folder.
- "unshare": Stop sharing this folder.
- "leave_a_copy": Keep a copy of the contents upon leaving or being kicked from the folder.
- "share_link": This action is deprecated. Use create_link instead.
- "create_link": Create a shared link for folder.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"access_type": {
".tag": "owner"
},
"is_team_folder": false,
"policy": {
"acl_update_policy": {
".tag": "owner"
},
"shared_link_policy": {
".tag": "anyone"
},
"member_policy": {
".tag": "anyone"
},
"resolved_member_policy": {
".tag": "team"
}
},
"name": "dir",
"shared_folder_id": "84528192421",
"time_invited": "2016-01-20T00:00:00Z",
"preview_url": "https://www.dropbox.com/scl/fo/fir9vjelf",
"path_lower": "/dir",
"permissions": []
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxGetFolderMetadata pAccessToken, pSharedFolderId, pActions, pCallback
   local tHeader, tPost, tURL
   put __get_folder_metadata_HEADER(pAccessToken) into tHeader
   put __get_folder_metadata_POST(pSharedFolderId,pActions) into tPost
   put __get_folder_metadata_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetFolderMetadata

/**

Download the shared link's file from a user's Dropbox.

Syntax: dropboxGetSharedLinkFile <pAccessToken>, <pUrl>, [<pPath>, <pLinkPassword>, <pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pURL: URL of the shared link

pPath: If the shared link is to a folder, this parameter can be used to retrieve the metadata
for a specific file or sub-folder in this folder. A relative path should be used. This field is optional.

pLinkPassword: If the shared link has a password, this parameter can be used. This field is optional.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be the binary data of the file.

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxGetSharedLinkFile pAccessToken, pUrl, pPath, pLinkPassword, pCallback
   local tHeader, tPost, tURL
   put __get_shared_link_file_HEADER(pAccessToken,pUrl,pPath,pLinkPassword) into tHeader
   put __get_shared_link_file_POST(pPath) into tPost
   put __get_shared_link_file_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetSharedLinkFile

/**

Get the shared link's metadata.

Syntax: dropboxGetSharedLinkMetadata <pAccessToken>, <pUrl>, [<pPath>, <pLinkPassword>, <pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pURL: URL of the shared link

pPath: If the shared link is to a folder, this parameter can be used to retrieve the metadata
for a specific file or sub-folder in this folder. A relative path should be used. This field is optional.

pLinkPassword: If the shared link has a password, this parameter can be used. This field is optional.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "file",
"url": "https://www.dropbox.com/s/2sn712vy1ovegw8/Prime_Numbers.txt?dl=0",
"name": "Prime_Numbers.txt",
"link_permissions": {
"can_revoke": false,
"resolved_visibility": {
".tag": "public"
},
"revoke_failure_reason": {
".tag": "owner_only"
}
},
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"path_lower": "/homework/math/prime_numbers.txt",
"team_member_info": {
"team_info": {
"id": "dbtid:AAFdgehTzw7WlXhZJsbGCLePe8RvQGYDr-I",
"name": "Acme, Inc."
},
"display_name": "Roger Rabbit",
"member_id": "dbmid:abcd1234"
}
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxGetSharedLinkMetadata pAccessToken, pUrl, pPath, pLinkPassword, pCallback
   local tHeader, tPost, tURL
   put __get_shared_link_metadata_HEADER(pAccessToken) into tHeader
   put __get_shared_link_metadata_POST(pUrl,pPath,pLinkPassword) into tPost
   put __get_shared_link_metadata_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxGetSharedLinkMetadata

/**

Returns shared folder membership by its folder ID.

Syntax: dropboxListFolderMembers <pAccessToken>, <pSharedFolderId>, [<pActions>, <pLimit>, <pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If a cursor is present then use the value with dropboxListFolderMembersContinue to obtain
more results.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderId: The ID for the shared folder.

pActions: This is a list indicating whether each returned member will include a boolean value 
MemberPermission.allow that describes whether the current user can perform the member Aaction on
the member. This field is optional.

Actions that may be taken on members of a shared folder. The value will be one of the following 
datatypes. New values may be introduced as our API evolves.

- "leave_a_copy": Allow the member to keep a copy of the folder when removing.
- "make_editor": Make the member an editor of the folder.
- "make_owner": Make the member an owner of the folder.
- "make_viewer": Make the member a viewer of the folder.
- "make_viewer_no_comment": Make the member a viewer of the folder without commenting permissions.
- "remove": Remove the member from the folder.

pLimit: The maximum number of results that include members, groups and invitees to return per request. 
The default for this field is 1000.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"users": [
{
"access_type": {
".tag": "owner"
},
"user": {
"account_id": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc",
"same_team": true,
"team_member_id": "dbmid:abcd1234"
},
"permissions": [],
"is_inherited": false
}
],
"groups": [
{
"access_type": {
".tag": "editor"
},
"group": {
"group_name": "Test group",
"group_id": "g:e2db7665347abcd600000000001a2b3c",
"group_management_type": {
".tag": "user_managed"
},
"group_type": {
".tag": "user_managed"
},
"is_member": false,
"is_owner": false,
"same_team": true,
"member_count": 10
},
"permissions": [],
"is_inherited": false
}
],
"invitees": [
{
"access_type": {
".tag": "viewer"
},
"invitee": {
".tag": "email",
"email": "jessica@example.com"
},
"permissions": [],
"is_inherited": false
}
],
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListFolderMembers pAccessToken, pSharedFolderId, pActions, pLimit, pCallback
   local tHeader, tPost, tURL
   put __list_folder_members_HEADER(pAccessToken) into tHeader
   put __list_folder_members_POST(pSharedFolderId,pActions,pLimit) into tPost
   put __list_folder_members_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListFolderMembers

/**

Once a cursor has been retrieved from dropboxListFolderMembers, use this to paginate through all shared folder members.

Syntax: dropboxListFolderMembersContinue <pAccessToken>, <pCursor>, [<pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If a cursor is present then use the value with dropboxListFolderMembersContinue to obtain
more results.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pCursor: The cursor returned by your last call to dropboxListFolderMembers or dropboxListFolderMembersContinue.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"users": [
{
"access_type": {
".tag": "owner"
},
"user": {
"account_id": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc",
"same_team": true,
"team_member_id": "dbmid:abcd1234"
},
"permissions": [],
"is_inherited": false
}
],
"groups": [
{
"access_type": {
".tag": "editor"
},
"group": {
"group_name": "Test group",
"group_id": "g:e2db7665347abcd600000000001a2b3c",
"group_management_type": {
".tag": "user_managed"
},
"group_type": {
".tag": "user_managed"
},
"is_member": false,
"is_owner": false,
"same_team": true,
"member_count": 10
},
"permissions": [],
"is_inherited": false
}
],
"invitees": [
{
"access_type": {
".tag": "viewer"
},
"invitee": {
".tag": "email",
"email": "jessica@example.com"
},
"permissions": [],
"is_inherited": false
}
],
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListFolderMembersContinue pAccessToken, pCursor, pCallback
   local tHeader, tPost, tURL
   put __list_folder_members_continue_HEADER(pAccessToken) into tHeader
   put __list_folder_members_continue_POST(pCursor) into tPost
   put __list_folder_members_continue_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListFolderMembersContinue

/**

Return the list of all shared folders the current user has access to.

Syntax: dropboxListFolders <pAccessToken>, [<pActions>, <pLimit>, <pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If a cursor is present then use the value with dropboxListFolderMembersContinue to obtain
more results.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pActions: This is a return delimited list indicating whether the returned folder data will
include a boolean value FolderPermission.allow that describes whether the current user can 
perform the folder action on the folder. This field is optional.

Actions that may be taken on shared folders. The value will be one of the following datatypes.
New values may be introduced as our API evolves.

- "change_options": Change folder options, such as who can be invited to join the folder.
- "edit_contents": Change or edit contents of the folder.
- "invite_editor": Invite a user or group to join the folder with read and write permission.
- "invite_viewer": Invite a user or group to join the folder with read permission.
- "invite_viewer_no_comment": Invite a user or group to join the folder with read permission but no comment permissions.
- "relinquish_membership": Relinquish one's own membership in the folder.
- "unmount": Unmount the folder.
- "unshare": Stop sharing this folder.
- "leave_a_copy": Keep a copy of the contents upon leaving or being kicked from the folder.
- "share_link": This action is deprecated. Use create_link instead.
- "create_link": Create a shared link for folder.

pLimit: The maximum number of results that include members, groups and invitees to return per request. 
The default for this field is 1000.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"entries": [
{
"access_type": {
".tag": "owner"
},
"is_team_folder": false,
"policy": {
"acl_update_policy": {
".tag": "owner"
},
"shared_link_policy": {
".tag": "anyone"
},
"member_policy": {
".tag": "anyone"
},
"resolved_member_policy": {
".tag": "team"
}
},
"name": "dir",
"shared_folder_id": "84528192421",
"time_invited": "2016-01-20T00:00:00Z",
"preview_url": "https://www.dropbox.com/scl/fo/fir9vjelf",
"path_lower": "/dir",
"permissions": []
}
],
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/

command dropboxListFolders pAccessToken, pLimit, pActions, pCallback
   local tHeader, tPost, tURL
   put __list_folders_HEADER(pAccessToken) into tHeader
   put __list_folders_POST(pLimit,pActions) into tPost
   put __list_folders_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListFolders

/**

Once a cursor has been retrieved from dropboxListFolders, use this to paginate through all shared folder members.

Syntax: dropboxListFoldersContinue <pAccessToken>, <pCursor>, [<pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If a cursor is present then use the value with dropboxListFolderMembersContinue to obtain
more results.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pCursor: The cursor returned by your last call to dropboxListFolders or dropboxListFoldersContinue.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"entries": [
{
"access_type": {
".tag": "owner"
},
"is_team_folder": false,
"policy": {
"acl_update_policy": {
".tag": "owner"
},
"shared_link_policy": {
".tag": "anyone"
},
"member_policy": {
".tag": "anyone"
},
"resolved_member_policy": {
".tag": "team"
}
},
"name": "dir",
"shared_folder_id": "84528192421",
"time_invited": "2016-01-20T00:00:00Z",
"preview_url": "https://www.dropbox.com/scl/fo/fir9vjelf",
"path_lower": "/dir",
"permissions": []
}
],
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListFoldersContinue pAccessToken, pCursor, pCallback
   local tHeader, tPost, tURL
   put __list_folders_continue_HEADER(pAccessToken) into tHeader
   put __list_folders_continue_POST(pCursor) into tPost
   put __list_folders_continue_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListFoldersContinue

/**

Return the list of all shared folders the current user can mount or unmount.

Syntax: dropboxListMountableFolders <pAccessToken>, [<pActions>, <pLimit>, <pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If a cursor is present then use the value with dropboxListMountableFoldersContinue to obtain
more results.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pActions: This is a return delimited list indicating whether the returned folder data will
include a boolean value FolderPermission.allow that describes whether the current user can 
perform the folder action on the folder. This field is optional.

Actions that may be taken on shared folders. The value will be one of the following datatypes.
New values may be introduced as our API evolves.

- "change_options": Change folder options, such as who can be invited to join the folder.
- "edit_contents": Change or edit contents of the folder.
- "invite_editor": Invite a user or group to join the folder with read and write permission.
- "invite_viewer": Invite a user or group to join the folder with read permission.
- "invite_viewer_no_comment": Invite a user or group to join the folder with read permission but no comment permissions.
- "relinquish_membership": Relinquish one's own membership in the folder.
- "unmount": Unmount the folder.
- "unshare": Stop sharing this folder.
- "leave_a_copy": Keep a copy of the contents upon leaving or being kicked from the folder.
- "share_link": This action is deprecated. Use create_link instead.
- "create_link": Create a shared link for folder.

pLimit: The maximum number of results that include members, groups and invitees to return per request. 
The default for this field is 1000.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"entries": [
{
"access_type": {
".tag": "owner"
},
"is_team_folder": false,
"policy": {
"acl_update_policy": {
".tag": "owner"
},
"shared_link_policy": {
".tag": "anyone"
},
"member_policy": {
".tag": "anyone"
},
"resolved_member_policy": {
".tag": "team"
}
},
"name": "dir",
"shared_folder_id": "84528192421",
"time_invited": "2016-01-20T00:00:00Z",
"preview_url": "https://www.dropbox.com/scl/fo/fir9vjelf",
"path_lower": "/dir",
"permissions": []
}
],
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListMountableFolders pAccessToken, pActions, pLimit, pCallback
   local tHeader, tPost, tURL
   put __list_mountable_folders_HEADER(pAccessToken) into tHeader
   put __list_mountable_folders_POST(pLimit,pActions) into tPost
   put __list_mountable_folders_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListMountableFolders

/**

Once a cursor has been retrieved from dropboxListMountableFolders, use this to paginate through all shared folder members.

Syntax: dropboxListMountableFoldersContinue <pAccessToken>, <pCursor>, [<pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If a cursor is present then use the value with dropboxListFolderMembersContinue to obtain
more results.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pCursor: The cursor returned by your last call to dropboxListMountableFolders or dropboxListMountableFoldersContinue.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"entries": [
{
"access_type": {
".tag": "owner"
},
"is_team_folder": false,
"policy": {
"acl_update_policy": {
".tag": "owner"
},
"shared_link_policy": {
".tag": "anyone"
},
"member_policy": {
".tag": "anyone"
},
"resolved_member_policy": {
".tag": "team"
}
},
"name": "dir",
"shared_folder_id": "84528192421",
"time_invited": "2016-01-20T00:00:00Z",
"preview_url": "https://www.dropbox.com/scl/fo/fir9vjelf",
"path_lower": "/dir",
"permissions": []
}
],
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListMountableFoldersContinue pAccessToken, pPath, pCallback
   local tHeader, tPost, tURL
   put __list_mountable_folders_continue_HEADER(pAccessToken) into tHeader
   put __list_mountable_folders_continue_POST(pPath) into tPost
   put __list_mountable_folders_continue_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListMountableFoldersContinue

/**

List shared links of this user.

Syntax: dropboxListSharedLinks <pAccessToken>, [<pPath>, <pDirectOnly>, <pCursor>, <pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath (optional string): If no path is given, returns a list of all shared links for the current user.
If a non-empty path is given, returns a list of all shared links that allow access 
to the given path - direct links to the given path and links to parent folders of 
the given path.

pDirectOnly (optional boolean): Links to parent folders can be suppressed by setting direct_only to true.

pCursor: The cursor returned by your last call to list_shared_links. This field is optional.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"links": [
{
".tag": "file",
"url": "https://www.dropbox.com/s/2sn712vy1ovegw8/Prime_Numbers.txt?dl=0",
"name": "Prime_Numbers.txt",
"link_permissions": {
"can_revoke": false,
"resolved_visibility": {
".tag": "public"
},
"revoke_failure_reason": {
".tag": "owner_only"
}
},
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"path_lower": "/homework/math/prime_numbers.txt",
"team_member_info": {
"team_info": {
"id": "dbtid:AAFdgehTzw7WlXhZJsbGCLePe8RvQGYDr-I",
"name": "Acme, Inc."
},
"display_name": "Roger Rabbit",
"member_id": "dbmid:abcd1234"
}
}
],
"has_more": true,
"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxListSharedLinks pAccessToken, pPath, pDirectOnly, pCursor, pCallback
   local tHeader, tPost, tURL
   put __list_shared_links_HEADER(pAccessToken) into tHeader
   put __list_shared_links_POST(pPath,pDirectOnly,pCursor) into tPost
   put __list_shared_links_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxListSharedLinks

/**

Modify the shared link's settings.

Syntax: dropboxModifySharedLinkSettings <pAccessToken>, <pUrl>, [<pSettings>, <pRemoveExpiration>, <pCallback>]

Description:
If the requested visibility conflict with the shared links policy of the team or the shared folder 
(in case the linked file is part of a shared folder) then the LinkPermissions.resolved_visibility 
of the returned SharedLinkMetadata will reflect the actual visibility of the shared link and the 
LinkPermissions.requested_visibility will reflect the requested visibility.

If no settings are given then the default visibility is RequestedVisibility.public 
(The resolved visibility, though, may depend on other aspects such as team and 
shared folder settings).

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pURL: URL of the shared link to change its settings

pSettings (optional array): 
````
{
"requested_visibility" (enum): The requested access for this shared link. This field is optional.

- "public": Anyone who has received the link can access it. No login required.
- "team_only": Only members of the same team can access the link. Login is required.
- "password": A link-specific password is required to access the link. Login is not required.
"link_password": If requested_visibility is password this is needed to specify the password to access the link. This field is optional.
"expires": format="%Y-%m-%dT%H:%M:%SZ" - Expiration time of the shared link. By default the link won't expire. This field is optional.
}
````

pRemoveExpiration (optional boolean): f set to true, removes the expiration of the shared link. 
The default for this field is False.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "file",
"url": "https://www.dropbox.com/s/2sn712vy1ovegw8/Prime_Numbers.txt?dl=0",
"name": "Prime_Numbers.txt",
"link_permissions": {
"can_revoke": false,
"resolved_visibility": {
".tag": "public"
},
"revoke_failure_reason": {
".tag": "owner_only"
}
},
"client_modified": "2015-05-12T15:50:38Z",
"server_modified": "2015-05-12T15:50:38Z",
"rev": "a1c10ce0dd78",
"size": 7212,
"id": "id:a4ayc_80_OEAAAAAAAAAXw",
"path_lower": "/homework/math/prime_numbers.txt",
"team_member_info": {
"team_info": {
"id": "dbtid:AAFdgehTzw7WlXhZJsbGCLePe8RvQGYDr-I",
"name": "Acme, Inc."
},
"display_name": "Roger Rabbit",
"member_id": "dbmid:abcd1234"
}
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxModifySharedLinkSettings pAccessToken, pUrl, pSettings, pRemoveExpiration, pCallback
   local tHeader, tPost, tURL
   put __modify_shared_link_settings_HEADER(pAccessToken) into tHeader
   put __modify_shared_link_settings_POST(pUrl,pSettings, pRemoveExpiration) into tPost
   put __modify_shared_link_settings_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxModifySharedLinkSettings

/**

The current user mounts the designated folder.

Syntax: dropboxMountFolder <pAccessToken>, <pSharedFolderId>, [<pCallback>]

Description:
Mount a shared folder for a user after they have been added as a member. Once 
mounted, the shared folder will appear in their Dropbox.
Apps must have full Dropbox access to use this endpoint.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderId: The ID of the shared folder to mount.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"access_type": {
".tag": "owner"
},
"is_team_folder": false,
"policy": {
"acl_update_policy": {
".tag": "owner"
},
"shared_link_policy": {
".tag": "anyone"
},
"member_policy": {
".tag": "anyone"
},
"resolved_member_policy": {
".tag": "team"
}
},
"name": "dir",
"shared_folder_id": "84528192421",
"time_invited": "2016-01-20T00:00:00Z",
"preview_url": "https://www.dropbox.com/scl/fo/fir9vjelf",
"path_lower": "/dir",
"permissions": []
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxMountFolder pAccessToken, pSharedFolderId, pCallback
   local tHeader, tPost, tURL
   put __mount_folder_HEADER(pAccessToken) into tHeader
   put __mount_folder_POST(pSharedFolderId) into tPost
   put __mount_folder_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxMountFolder

/**

The current user relinquishes their membership in the designated shared folder and will no longer have access to the folder.

Syntax: dropboxRelinquishFolderMembership <pAccessToken>, <pSharedFolderId>, <pLeaveACopy>, [<pCallback>]

Description:
A folder owner cannot relinquish membership in their own folder.
This will run synchronously if leave_a_copy is false, and asynchronously if leave_a_copy 
is true. Apps must have full Dropbox access to use this endpoint.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderId: The ID for the shared folder.

pLeaveACopy: Keep a copy of the folder's contents upon relinquishing membership. The default 
for this field is False.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "complete"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxRelinquishFolderMembership pAccessToken, pSharedFolderId, pLeaveACopy, pCallback
   local tHeader, tPost, tURL
   put __relinquish_folder_membership_HEADER(pAccessToken) into tHeader
   put __relinquish_folder_membership_POST(pSharedFolderId, pLeaveACopy) into tPost
   put __relinquish_folder_membership_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxRelinquishFolderMembership

/**

Allows an owner or editor (if the ACL update policy allows) of a shared folder to remove another member.

Syntax: dropboxRemoveFolderMember <pAccessToken>, <pSharedFolderId>, <pMember>, <pLeaveACopy>, [<pCallback>]

Description:
Apps must have full Dropbox access to use this endpoint.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderId: The ID for the shared folder.

pMember (array): An array with either an email key or a dropbox_id key to identify the
member to remove

pLeaveACopy: Keep a copy of the folder's contents upon relinquishing membership. The default 
for this field is False.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "complete"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxRemoveFolderMember pAccessToken, pSharedFolderId, pMember, pLeaveACopy, pCallback
   local tHeader, tPost, tURL
   put __remove_folder_member_HEADER(pAccessToken) into tHeader
   put __remove_folder_member_POST(pSharedFolderId,pMember,pLeaveACopy) into tPost
   put __remove_folder_member_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxRemoveFolderMember

/**

Revoke a shared link.

Syntax: dropboxRevokeSharedLink <pAccessToken>, <pURL>, [<pCallback>]

Description:
Note that even after revoking a shared link to a file, the file may be accessible if there are shared 
links leading to any of the file parent folders. To list all shared links that enable access to a specific 
file, you can use the list_shared_links with the file as the ListSharedLinksArg.path argument.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pURL: URL of the shared link.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If an error occurred the JSON string will be in
the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxRevokeSharedLink pAccessToken, pURL, pCallback
   local tHeader, tPost, tURL
   put __revoke_shared_link_HEADER(pAccessToken) into tHeader
   put __revoke_shared_link_POST(pURL) into tPost
   put __revoke_shared_link_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxRevokeSharedLink

/**

Share a folder with collaborators.

Syntax: dropboxShareFolder <pAccessToken>, <pPath>, <pMemberPolicy>, <pAclUpdatePolicy>, <pSharedLinkPolicy>, <pForceAsync>, [<pCallback>]

Description:
Most sharing will be completed synchronously. Large folders will be completed asynchronously. To make
testing the async case repeatable, set `ShareFolderArg.force_async`.
If a ShareFolderLaunch.async_job_id is returned, you'll need to call check_share_job_status until the 
action completes to get the metadata for the folder.
Apps must have full Dropbox access to use this endpoint.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pPath: The path to the folder to share. If it does not exist, then a new one is created.

pMemberPolicy (enum): Who can be a member of this shared folder. Only applicable if the current user is on a team. The default for this union is anyone.

- "anyone": Anyone can become a member.
- "team": Only a teammate can become a member.

pAclUpdatePolicy (enum): Who can add and remove members of this shared folder. The default for this union is owner.
Policy governing who can change a shared folder's access control list (ACL). In other words, who can add, remove, or change the privileges of members. The value will be one of the following datatypes. New values may be introduced as our API evolves.

- "owner": Only the owner can update the ACL.
- "editors": Any editor can update the ACL. This may be further restricted to editors on the same team.

pSharedLinkPolicy (enum): The policy to apply to shared links created for content inside this shared folder. The current 
user must be on a team to set this policy to SharedLinkPolicy.members. The default for this union is anyone.
Policy governing who can view shared links. The value will be one of the following datatypes. New values may be 
introduced as our API evolves.

- "anyone": Links can be shared with anyone.
- "members": Links can only be shared among members of the shared folder.

pForceAsync (boolean): Whether to force the share to happen asynchronously. The default for this field is False.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "complete"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxShareFolder pAccessToken, pPath, pMemberPolicy, pAclUpdatePolicy, pSharedLinkPolicy, pForceAsync, pCallback
   local tHeader, tPost, tURL 
   put __share_folder_HEADER(pAccessToken) into tHeader
   put __share_folder_POST(pPath,pMemberPolicy,pAclUpdatePolicy,pSharedLinkPolicy,pForceAsync) into tPost
   put __share_folder_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxShareFolder

/**

Transfer ownership of a shared folder to a member of the shared folder.

Syntax: dropboxTransferFolder <pAccessToken>, <pSharedFolderId>, <pToDropboxId>, [<pCallback>]

Description:
User must have AccessLevel.owner access to the shared folder to perform a transfer.
Apps must have full Dropbox access to use this endpoint.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderID: The ID for the shared folder.

pToDropboxId: A account or team member ID to transfer ownership to.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxTransferFolder pAccessToken, pSharedFolderId, pToDropboxId, pCallback
   local tHeader, tPost, tURL
   put __transfer_folder_HEADER(pAccessToken) into tHeader
   put __transfer_folder_POST(pSharedFolderId,pToDropboxId) into tPost
   put __transfer_folder_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxTransferFolder


/**

The current user unmounts the designated folder. 

Syntax: dropboxUnmountFolder <pAccessToken>, <pSharedFolderId>, [<pCallback>]

Description:
The user can re-mount the folder at a later time using dropboxMountFolder.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderID: The ID for the shared folder.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function.

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxUnmountFolder pAccessToken, pSharedFolderId, pCallback
   local tHeader, tPost, tURL
   put __unmount_folder_HEADER(pAccessToken) into tHeader
   put __unmount_folder_POST(pSharedFolderId) into tPost
   put __unmount_folder_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxUnmountFolder

/**

Allows a shared folder owner to unshare the folder.

Syntax: dropboxUnshareFolder <pAccessToken>, <pSharedFolderId>, <pLeaveACopy>, [<pCallback>]

Description:
You'll need to call droboxCheckJobStatus to determine if the action has completed successfully.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderID: The ID for the shared folder.

pLeaveACopy (boolean): If true, members of this shared folder will get a copy of this folder after 
it's unshared. Otherwise, it will be removed from their Dropbox. The current user, who is an owner, 
will always retain their copy. The default for this field is False.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
".tag": "complete"
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxUnshareFolder pAccessToken, pSharedFolderId, pLeaveACopy, pCallback
   local tHeader, tPost, tURL
   put __unshare_folder_HEADER(pAccessToken) into tHeader
   put __unshare_folder_POST(pSharedFolderId,pLeaveACopy) into tPost
   put __unshare_folder_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxUnshareFolder

/**

Allows an owner or editor of a shared folder to update another member's permissions.

Syntax: dropboxUpdateFolderMember <pAccessToken>, <pSharedFolderId>, <pMember>, <pAccessLevel>, [<pCallback>]

Description:
If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderID: The ID for the shared folder.

pMember (array): An array with either an email key or a dropbox_id key to identify the
member to remove

pAccessLevel (enum): The new access level for member. Defines the access levels for collaborators.
The value will be one of the following datatypes. New values may be introduced as our API evolves.

- "editor": The collaborator can both view and edit the shared folder.
- "viewer": The collaborator can only view the shared folder.
- "viewer_no_comment": The collaborator can only view the shared folder and does not have any access to comments.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string that contains information about a member's 
access level to content after an operation.

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxUpdateFolderMember pAccessToken, pSharedFolderId, pMember, pAccessLevel, pCallback
   local tHeader, tPost, tURL
   put __update_folder_member_HEADER(pAccessToken) into tHeader
   put __update_folder_member_POST(pSharedFolderId,pMember,pAccessLevel) into tPost
   put __update_folder_member_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxUpdateFolderMember

/**

Update the sharing policies for a shared folder.

Syntax: dropboxUpdateFolderPolicy <pAccessToken>, <pSharedFolderId>, <pMemberPolicy>, <pAclUpdatePolicy>, <pSharedLinkPolicy>, [<pCallback>]

Description:
User must have AccessLevel.owner access to the shared folder to update its policies.

If the callback parameter is not empty the request will be asynchronus and
when complete the callback will be sent to the object that accessed the API.
The callback will be sent with three parameters:

- The request ID which will be the value of the it variable after calling the command
- The HTTP response code
- The data returned which will be the same data as documented for the it variable in a synchronous request.


Parameters:
pAccessToken: An OAuth2 Access token to access the user's account

pSharedFolderID: The ID for the shared folder.

pMemberPolicy (enum): Who can be a member of this shared folder. Only applicable if the current user is on a team. The default for this union is anyone.

- "anyone": Anyone can become a member.
- "team": Only a teammate can become a member.

pAclUpdatePolicy (enum): Who can add and remove members of this shared folder. The default for this union is owner.
Policy governing who can change a shared folder's access control list (ACL). In other words, who can add, remove, or change the privileges of members. The value will be one of the following datatypes. New values may be introduced as our API evolves.

- "owner": Only the owner can update the ACL.
- "editors": Any editor can update the ACL. This may be further restricted to editors on the same team.

pSharedLinkPolicy (enum): The policy to apply to shared links created for content inside this shared folder. The current 
user must be on a team to set this policy to SharedLinkPolicy.members. The default for this union is anyone.
Policy governing who can view shared links. The value will be one of the following datatypes. New values may be 
introduced as our API evolves.

- "anyone": Links can be shared with anyone.
- "members": Links can only be shared among members of the shared folder.

pCallback: The handler to call when the request is complete. If empty the command
will block until complete. Requires tsNet with LiveCode Indy License.

The result: An error if the HTTP request failed

It: For asynchronous requests it will contain a request ID. This may be used with
tsNet's tsNetGetStatus function. If the request is synchronous and successful the it
variable will be a UTF-8 encoded JSON string in the form:

````
{
"access_type": {
".tag": "owner"
},
"is_team_folder": false,
"policy": {
"acl_update_policy": {
".tag": "owner"
},
"shared_link_policy": {
".tag": "anyone"
},
"member_policy": {
".tag": "anyone"
},
"resolved_member_policy": {
".tag": "team"
}
},
"name": "dir",
"shared_folder_id": "84528192421",
"time_invited": "2016-01-20T00:00:00Z",
"preview_url": "https://www.dropbox.com/scl/fo/fir9vjelf",
"path_lower": "/dir",
"permissions": []
}
````

If an error occurred the JSON string will be in the following form:

````
{
"error_summary": string,
"error": {
".tag": string
}
}
````
*/
command dropboxUpdateFolderPolicy pAccessToken, pSharedFolderId, pMemberPolicy, pAclUpdatePolicy, pSharedLinkPolicy, pCallback
   local tHeader, tPost, tURL
   put __update_folder_policy_HEADER(pAccessToken) into tHeader
   put __update_folder_policy_POST(pSharedFolderId,pMemberPolicy,pAclUpdatePolicy,pSharedLinkPolicy) into tPost
   put __update_folder_policy_URL() into tUrl
   
   __API tHeader, tPost, tURL, pCallback
   if the result is empty then
      return it for value
   else
      return the result for error
   end if
end dropboxUpdateFolderPolicy

-----------------------------------------------------------------------------
-----------------------------------------------------------------------------

local sAsyncRequestA

private command __API pHeader, pPost, pURL, pCallback
   set socketTimeoutInterval to 30000  --30 seconds
   
   local tError
   if pCallback is not empty then #asynchronous call
      local tRequestID
      put uuid() into tRequestID
      put __GetCaller() into sAsyncRequestA[tRequestID]["caller"]
      put pCallback into sAsyncRequestA[tRequestID]["callback"]
      put tsNetPost(tRequestID, pURL, pHeader, pPost, "__dropboxAPIAsyncCallback") into tError
      
      if tError is empty then
         return tRequestID for value
      end if #tError is empty
   else #pCallback is empty
      local tOldHeaders, tPostReturn
      put the httpHeaders into tOldHeaders
      set the httpHeaders to pHeader
      post pPost to URL pURL
      put it into tPostReturn
      put the result into tError
      set the httpHeaders to tOldHeaders

      if tError is empty then
         return tPostReturn for value
      end if #tError is empty
   end if #pCallback is not empty


   #if we get to here, then tError is not empty
   if tPostReturn is not empty then 
      put CR&tPostReturn after tError
   end if #tPostReturn is not empty

   return tError for error
end __API

on __dropboxAPIAsyncCallback pRequestID, pStatusCode, pBytes, pCurlResultCode
   if exists(sAsyncRequestA[pRequestID]["caller"]) then
      local tData, tError
      put tsNetRetrData(pRequestID, tError) into tData

      dispatch sAsyncRequestA[pRequestID]["callback"] to sAsyncRequestA[pRequestID]["caller"] with pRequestID, pStatusCode, tData, tError
   end if

   tsNetCloseConn pRequestID
end __dropboxAPIAsyncCallback

private function __GetCaller
   get item 1 to -3 of line -4 of the executionContexts
   if there is not an it then
      delete item -1 of it
   end if
   return it
end __GetCaller

-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
------------header building blocks---------------------------------------
-----------------------------------------------------------------------------

private function __Header_Standard pAccessToken
   return __Header_Auth(pAccessToken) & cr & __Header_Json()
end __Header_Standard

private function __Header_AuthOctet pAccessToken
   return __Header_Auth(pAccessToken) & cr & __Header_Octet()
end __Header_AuthOctet

private function __Header_Auth pAccessToken
   return "Authorization: Bearer " & pAccessToken
end __Header_Auth

private function __Header_Json
   return "Content-Type: application/json; charset=UTF-8"
end __Header_Json

private function __Header_Octet
   return "Content-Type: application/octet-stream"
end __Header_Octet

private function __Header_EmptyContent
   /* 
   needed for get_thumbnail and get_preview (which dont set a Content Type) to clear standard value in headers
   */
   return "Content-Type:"
end __Header_EmptyContent

private function __Header_APIDownload pPath
   /* 
   Dropbox-API-Arg: {"path":"/apps/MyApp/seasons.txt"}
   path can be a path string, an id or a revision
   "path": "/apps/MyApp/seasons.txt"
   "path": "id:a4ayc_80_OEAAAAAAAAAYa"
   "path": "rev:a1c10ce0dd78"
   */
   return "Dropbox-API-Arg: " & __CB(__Q("path") & ":" & __Q(pPath))
end __Header_APIDownload

private function __Header_APISessionAppend pSession,pOffset
   /* 
   Dropbox-API-Arg: 
   {"session_id":"AAAAAAAAAYSEAI0qRJ7GMg","offset":327}
   */
   return "Dropbox-API-Arg: " & __CB(__Q("session_id") & ":" & __Q(pSession)& "," &__Q("offset") & ":" & pOffset)
end __Header_APISessionAppend

private function __Header_APISessionFinish pSession,pOffset,pPath,pMode,pAutorename,pMute
   /* 
   Dropbox-API-Arg: 
   {"cursor":{"session_id":"AAAAAAAAAYSEAI0qRJ7GMg","offset":358},"commit":{"path":"/apps/MyApp/seasons.txt","mode":"add"}}
   {"cursor": {"session_id": "1234faaf0678bcde","offset": 0},"commit": {"path": "/apps/MyApp/seasons.txt","mode": "add","autorename": true,"mute": false}}
   */
   return "Dropbox-API-Arg: " & __CB(__Q("cursor") & ":" &  __CB(__Q("session_id") & ":" & __Q(pSession) & "," &__Q("offset") & ":" & pOffset) & "," &  __Q("commit") & ":" & __CB(__Q("path") & ":" & __Q(pPath) & "," & __Q("mode") & ":" & __Q(pMode) & "," & __Q("autorename") & ":" & pAutorename & "," &__Q("mute") & ":" & pMute))
end __Header_APISessionFinish

private function __Header_APIUpload pPath,pMode,pAutorename,pMute
   /* 
   Dropbox-API-Arg: {"path":"/Apps/MyApp/season/NewRCPStestfile.txt","mode":"overwrite","autorename": true,"mute": false}
   */
   return "Dropbox-API-Arg: " & __CB(__Q("path") & ":" & __Q(pPath) & "," & __Q("mode") & ":" & __Q(pMode) & "," & __Q("autorename") & ":" & pAutorename & "," &__Q("mute") & ":" & pMute)
end __Header_APIUpload

private function __Header_APIThumb pPath,pFormat,pSize
   /* 
   Dropbox-API-Arg: {"path":"/apps/MyApp/RCsubmitter.jpg","format": "jpeg","size": "w64h64"}
   */
   return "Dropbox-API-Arg: " & __CB(__Q("path") & ":" & __Q(pPath) & "," &__Q("format") & ":" & __Q(pFormat)& "," &__Q("size") & ":" & __Q(pSize))
end __Header_APIThumb

private function __Header_APIPreview pPath
   /* 
   Dropbox-API-Arg: {"path":"/apps/MyApp/seasons.txt"}
   */
   return "Dropbox-API-Arg: " & __CB(__Q("path") & ":" & __Q(pPath))
end __Header_APIPreview

private function __Header_APISharedLinkFile pUrl,pPath,pLinkPassword
   /* 
   Dropbox-API-Arg: {"url": "https://www.dropbox.com/s/2sn712vy1ovegw8/Prime_Numbers.txt?dl=0","path": "/Prime_Numbers.txt"}"
   */
   if pPath is not empty then
      if pLinkPassword is not empty then
         return "Dropbox-API-Arg: " & __CB(__Q("url") & ":" &  __Q(pUrl) & "," &__Q("path") & ":" & __Q(pPath) & "," &__Q("link_password") & ":" & __Q(pLinkPassword))
      else
         return "Dropbox-API-Arg: " & __CB(__Q("url") & ":" &  __Q(pUrl) & "," &__Q("path") & ":" & __Q(pPath))
      end if
   else
      return "Dropbox-API-Arg: " & __CB(__Q("url") & ":" &  __Q(pUrl))
   end if
end __Header_APISharedLinkFile

-----------------------------------------------------------------------------
------------individual headers for all API---------------------------------------
-------users--------------------------------------------------------------

private function __get_account_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __get_account_HEADER

private function __get_account_batch_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __get_account_batch_HEADER

private function __get_current_account_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __get_current_account_HEADER

private function __get_space_usage_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __get_space_usage_HEADER

-------files--------------------------------------------------------------

private function __copy_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __copy_HEADER

private function __create_folder_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __create_folder_HEADER

private function __delete_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __delete_HEADER

private function __download_HEADER pAccessToken,pPath
   /*
   DownloadArg
   path String The path of the file to download.
   {"path": "/Homework/math/Prime_Numbers.txt"}
   Example: id {"path": "id:a4ayc_80_OEAAAAAAAAAYa"}
   Example: rev  {"path": "rev:a1c10ce0dd78"}
   */
   return __Header_Auth(pAccessToken) & cr & __Header_EmptyContent() & cr & __Header_APIDownload(pPath)
end __download_HEADER

private function __get_metadata_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __get_metadata_HEADER

private function __get_preview_HEADER pAccessToken,pPath
   /*
   {"path": "/word.docx"}
   Example: id  {"path": "id:a4ayc_80_OEAAAAAAAAAYa"}
   Example: rev  {"path": "rev:a1c10ce0dd78"}
   PreviewArg
   #--path String The path of the file to preview.
   */
   return __Header_Auth(pAccessToken) & cr & __Header_EmptyContent() & cr & __Header_APIPreview(pPath)
end __get_preview_HEADER

private function __get_thumbnail_HEADER pAccessToken,pPath,pFormat,pSize
   /*
   {"path": "/image.jpg","format": "jpeg","size": "w64h64"}
   Example: id  {"path": "id:a4ayc_80_OEAAAAAAAAAYa","format": "jpeg","size": "w64h64"}
   Example: rev  {"path": "rev:a1c10ce0dd78","format": "jpeg","size": "w64h64"}
   ThumbnailArg
   #--path String The path to the image file you want to thumbnail.
   #--format ThumbnailFormat The format for the thumbnail image, jpeg (default) or png. For images that are photos, jpeg should be preferred, while png is better for screenshots and digital arts.
   ThumbnailFormat (union)
   The value will be one of the following datatypes:
   ##----jpeg Void
   ##----png Void
   #--size ThumbnailSize The size for the thumbnail image.
   ThumbnailSize (union)
   The value will be one of the following datatypes:
   ##----w32h32 Void 32 by 32 px.
   ##----w64h64 Void 64 by 64 px.
   ##----w128h128 Void 128 by 128 px.
   ##----w640h480 Void 640 by 480 px.
   ##----w1024h768 Void 1024 by 768
   */
   if pFormat is empty then put "jpeg" into pFormat
   if pSize is empty then put "w32h32" into pSize
   return __Header_Auth(pAccessToken) & cr & __Header_EmptyContent() & cr & __Header_APIThumb(pPath,pFormat,pSize)
end __get_thumbnail_HEADER

private function __list_folder_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_folder_HEADER 

private function __list_folder_continue_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_folder_continue_HEADER

private function __list_folder_get_latest_cursor_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_folder_get_latest_cursor_HEADER

private function __list_folder_longpoll_HEADER
   return __Header_Json()
end __list_folder_longpoll_HEADER

private function __list_revisions_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_revisions_HEADER

private function __move_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __move_HEADER

private function __permanently_delete_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __permanently_delete_HEADER

private function __restore_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __restore_HEADER

private function __search_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __search_HEADER

private function __upload_session_append_HEADER pAccessToken,pSession,pOffset
   /*
   {"session_id": "1234faaf0678bcde","offset": 0}
   UploadSessionCursor
   #--session_id String The upload session ID (returned by upload_session/start).
   #--offset UInt64 The amount of data that has been uploaded so far to make sure upload data isn't lost or duplicated in the event of a network error.
   */
   return __Header_AuthOctet(pAccessToken) & cr & __Header_APISessionAppend(pSession,pOffset)
end __upload_session_append_HEADER

private function __upload_session_finish_HEADER pAccessToken,pSession,pOffset,pPath,pMode,pAutorename,pMute
   /*
   {"cursor": {"session_id": "1234faaf0678bcde","offset": 0},
   "commit": {"path": "/apps/MyApp/seasons.txt","mode": "add","autorename": true,"mute": false}}
   UploadSessionFinishArg
   #--cursor UploadSessionCursor Contains the upload session ID and the offset.
   UploadSessionCursor
   ##--session_id String The upload session ID (returned by upload_session/start).
   ##--offset UInt64 The amount of data that has been uploaded so far to make sure upload data isn't lost or duplicated in the event of a network error.
   #--commit CommitInfo Contains the path and other optional modifiers for the commit.
   CommitInfo
   ##--path String Path in the user's Dropbox to save the file.
   ##--mode WriteMode Selects what to do if the file already exists.
   WriteMode (union)
   Your intent when writing a file to some path. This is used to determine what constitutes a conflict and what the autorename strategy is.
   In some situations, the conflict behavior is identical: (a) If the target path doesn't contain anything, the file is always written; no conflict. (b) If the target path contains a folder, it's always a conflict. (c) If the target path contains a file with identical contents, nothing gets written; no conflict.
   The conflict checking differs in the case where there's a file at the target path with contents different from the contents you're trying to write.The value will be one of the following datatypes:
   ##--#--add Void Never overwrite the existing file. The autorename strategy is to append a number to the file name. For example, "document.txt" might become "document (2).txt".
   ##--#--overwrite Void Always overwrite the existing file. The autorename strategy is the same as it is for add.
   ##--#--update String Overwrite if the given "rev" matches the existing file's "rev". The autorename strategy is to append the string "conflicted copy" to the file name. For example, "document.txt" might become "document (conflicted copy).txt" or "document (Panda's conflicted copy).txt".
   ##--autorename Boolean If there's a conflict, as determined by mode, have the Dropbox server try to autorename the file to avoid conflict. The default for this field is False.
   ##--client_modified Timestamp? The value to store as the client_modified timestamp. Dropbox automatically records the time at which the file was written to the Dropbox servers. It can also record an additional timestamp, provided by Dropbox desktop clients, mobile clients, and API apps of when the file was actually created or modified. This field is optional.
   ##--mute Boolean Normally, users are made aware of any file modifications in their Dropbox account via notifications in the client software. If True, this tells the clients that this modification shouldn't result in a user notification. The default for this field is False.
   */
   if pMode is empty then put "add" into pMode
   if pMute is empty then put "False" into pMute
   if pAutorename is empty then put "False" into pAutorename
   return __Header_AuthOctet(pAccessToken) & cr & __Header_APISessionFinish(pSession,pOffset,pPath,pMode,pAutorename,pMute)
end __upload_session_finish_HEADER

private function __upload_HEADER pAccessToken,pPath,pMode,pAutorename,pMute
   /*
   {"path": "/apps/MyApp/seasons.txt","mode": "add","autorename": true,"mute": false}
   CommitInfo
   #--path String Path in the user's Dropbox to save the file.
   #--mode WriteMode Selects what to do if the file already exists.
   WriteMode (union)
   Your intent when writing a file to some path. This is used to determine what constitutes a conflict and what the autorename strategy is.
   In some situations, the conflict behavior is identical: (a) If the target path doesn't contain anything, the file is always written; no conflict. (b) If the target path contains a folder, it's always a conflict. (c) If the target path contains a file with identical contents, nothing gets written; no conflict.
   The conflict checking differs in the case where there's a file at the target path with contents different from the contents you're trying to write.The value will be one of the following datatypes:
   ##--add Void Never overwrite the existing file. The autorename strategy is to append a number to the file name. For example, "document.txt" might become "document (2).txt".
   ##--overwrite Void Always overwrite the existing file. The autorename strategy is the same as it is for add.
   ##--update String Overwrite if the given "rev" matches the existing file's "rev". The autorename strategy is to append the string "conflicted copy" to the file name. For example, "document.txt" might become "document (conflicted copy).txt" or "document (Panda's conflicted copy).txt".
   #--autorename Boolean If there's a conflict, as determined by mode, have the Dropbox server try to autorename the file to avoid conflict. The default for this field is False.
   client_modified Timestamp? The value to store as the client_modified timestamp. Dropbox automatically records the time at which the file was written to the Dropbox servers. It can also record an additional timestamp, provided by Dropbox desktop clients, mobile clients, and API apps of when the file was actually created or modified. This field is optional.
   #--mute Boolean Normally, users are made aware of any file modifications in their Dropbox account via notifications in the client software. If True, this tells the clients that this modification shouldn't result in a user notification. The default for this field is False.
   */
   if pMode is empty then put "add" into pMode
   if pAutorename is empty then put "false" into pAutorename
   if pMute is empty then put "false" into pMute
   return __Header_AuthOctet(pAccessToken) & cr & __Header_APIUpload(pPath,pMode,pAutorename,pMute)
end __upload_HEADER

private function __upload_session_start_HEADER pAccessToken
   return __Header_AuthOctet(pAccessToken)
end __upload_session_start_HEADER

----------------------------------------------------------------
------sharing-----------------------------------------------

private function __add_folder_member_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __add_folder_member_HEADER

private function __check_job_status_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __check_job_status_HEADER

private function __check_share_job_status_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __check_share_job_status_HEADER

private function __create_shared_link_with_settings_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __create_shared_link_with_settings_HEADER

private function __get_folder_metadata_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __get_folder_metadata_HEADER

private function __get_shared_link_file_HEADER pAccessToken,pUrl,pPath,pLinkPassword
   /*
   {"path": "/word.docx"}
   Example: id  {"path": "id:a4ayc_80_OEAAAAAAAAAYa"}
   Example: rev  {"path": "rev:a1c10ce0dd78"}
   GetSharedLinkMetadataArg
   url String URL of the shared link.
   path String? If the shared link is to a folder, this parameter can be used to retrieve the metadata for a specific file or sub-folder in this folder. 
   A relative path should be used. This field is optional.
   link_password String? If the shared link has a password, this parameter can be used. This field is optional.
   */
   return __Header_Auth(pAccessToken) & cr & __Header_EmptyContent() & cr & __Header_APISharedLinkFile(pUrl,pPath,pLinkPassword)
end __get_shared_link_file_HEADER

private function __get_shared_link_metadata_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __get_shared_link_metadata_HEADER

private function __list_folder_members_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_folder_members_HEADER

private function __list_folder_members_continue_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_folder_members_continue_HEADER

private function __list_folders_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_folders_HEADER

private function __list_folders_continue_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_folders_continue_HEADER

private function __list_mountable_folders_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_mountable_folders_HEADER

private function __list_mountable_folders_continue_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_mountable_folders_continue_HEADER

private function __list_shared_links_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __list_shared_links_HEADER

private function __modify_shared_link_settings_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __modify_shared_link_settings_HEADER

private function __mount_folder_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __mount_folder_HEADER

private function __relinquish_folder_membership_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __relinquish_folder_membership_HEADER

private function __remove_folder_member_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __remove_folder_member_HEADER

private function __revoke_shared_link_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __revoke_shared_link_HEADER

private function __share_folder_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __share_folder_HEADER

private function __transfer_folder_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __transfer_folder_HEADER

private function __unmount_folder_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __unmount_folder_HEADER

private function __unshare_folder_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __unshare_folder_HEADER

private function __update_folder_member_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __update_folder_member_HEADER

private function __update_folder_policy_HEADER pAccessToken
   return __Header_Standard(pAccessToken)
end __update_folder_policy_HEADER

------------------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------
------------individual URLs for all API---------------------------------------
----------------------------------------------------------------
---------users------------------------------------------------

private function __get_account_URL
   return "https://api.dropboxapi.com/2/users/get_account?"
end __get_account_URL 

private function __get_account_batch_URL
   return "https://api.dropboxapi.com/2/users/get_account_batch?"
end __get_account_batch_URL 

private function __get_current_account_URL
   return "https://api.dropboxapi.com/2/users/get_current_account?"
end __get_current_account_URL 

private function __get_space_usage_URL
   return "https://api.dropboxapi.com/2/users/get_space_usage?"
end __get_space_usage_URL 

----------------------------------------------------------------
---------files------------------------------------------------

private function __copy_URL
   return "https://api.dropboxapi.com/2/files/copy?"
end __copy_URL 

private function __create_folder_URL
   return "https://api.dropboxapi.com/2/files/create_folder?"
end __create_folder_URL 

private function __delete_URL
   return "https://api.dropboxapi.com/2/files/delete?"
end __delete_URL 

private function __download_URL
   return "https://content.dropboxapi.com/2/files/download?"
end __download_URL 

private function __get_metadata_URL
   return "https://api.dropboxapi.com/2/files/get_metadata?"
end __get_metadata_URL 

private function __get_preview_URL
   return "https://content.dropboxapi.com/2/files/get_preview?"
end __get_preview_URL

private function __get_thumbnail_URL
   return "https://content.dropboxapi.com/2/files/get_thumbnail?"
end __get_thumbnail_URL

private function __list_folder_URL
   return "https://api.dropboxapi.com/2/files/list_folder?"
end __list_folder_URL 

private function __list_folder_continue_URL
   return "https://api.dropboxapi.com/2/files/list_folder/continue?"
end __list_folder_continue_URL 

private function __list_folder_get_latest_cursor_URL
   return "https://api.dropboxapi.com/2/files/list_folder/get_latest_cursor?"
end __list_folder_get_latest_cursor_URL 

private function __list_folder_longpoll_URL
   return "https://notify.dropboxapi.com/2-beta-2/files/list_folder/longpoll?"
end __list_folder_longpoll_URL

private function __list_revisions_URL
   return "https://api.dropboxapi.com/2/files/list_revisions?"
end __list_revisions_URL

private function __move_URL
   return "https://api.dropboxapi.com/2/files/move?"
end __move_URL

private function __permanently_delete_URL
   return "https://api.dropboxapi.com/2/files/permanently_delete?"
end __permanently_delete_URL 

private function __restore_URL
   return "https://api.dropboxapi.com/2/files/restore?"
end __restore_URL 

private function __search_URL
   return "https://api.dropboxapi.com/2/files/search?"
end __search_URL 

private function __upload_URL
   return "https://content.dropboxapi.com/2/files/upload?"
end __upload_URL 

private function __upload_session_append_URL
   return "https://content.dropboxapi.com/2/files/upload_session/append?"
end __upload_session_append_URL 

private function __upload_session_finish_URL
   return "https://content.dropboxapi.com/2/files/upload_session/finish?"
end __upload_session_finish_URL 

private function __upload_session_start_URL
   return "https://content.dropboxapi.com/2/files/upload_session/start?"
end __upload_session_start_URL 

----------------------------------------------------------------
---------sharing---------------------------------------------

private function __add_folder_member_URL
   return "https://api.dropboxapi.com/2/sharing/add_folder_member?"
end __add_folder_member_URL 

private function __check_job_status_URL
   return "https://api.dropboxapi.com/2/sharing/check_job_status?"
end __check_job_status_URL

private function __check_share_job_status_URL
   return "https://api.dropboxapi.com/2/sharing/check_share_job_status?"
end __check_share_job_status_URL 

private function __create_shared_link_with_settings_URL
   return "https://api.dropboxapi.com/2/sharing/create_shared_link_with_settings?"
end __create_shared_link_with_settings_URL 

private function __get_folder_metadata_URL
   return "https://api.dropboxapi.com/2/sharing/get_folder_metadata?"
end __get_folder_metadata_URL 

private function __get_shared_link_file_URL
   return "https://content.dropboxapi.com/2/sharing/get_shared_link_file?"
end __get_shared_link_file_URL 

private function __get_shared_link_metadata_URL
   return "https://api.dropboxapi.com/2/sharing/get_shared_link_metadata?"
end __get_shared_link_metadata_URL 

private function __list_folder_members_URL
   return "https://api.dropboxapi.com/2/sharing/list_folder_members?"
end __list_folder_members_URL 

private function __list_folder_members_continue_URL
   return "https://api.dropboxapi.com/2/sharing/list_folder_members/continue?"
end __list_folder_members_continue_URL 

private function __list_folders_URL
   return "https://api.dropboxapi.com/2/sharing/list_folders?"
end __list_folders_URL 

private function __list_folders_continue_URL
   return "https://api.dropboxapi.com/2/sharing/list_folders/continue?"
end __list_folders_continue_URL 

private function __list_mountable_folders_URL
   return "https://api.dropboxapi.com/2/sharing/list_mountable_folders?"
end __list_mountable_folders_URL 

private function __list_mountable_folders_continue_URL
   return "https://api.dropboxapi.com/2/sharing/list_mountable_folders/continue?"
end __list_mountable_folders_continue_URL 

private function __list_shared_links_URL
   return "https://api.dropboxapi.com/2/sharing/list_shared_links?"
end __list_shared_links_URL 

private function __modify_shared_link_settings_URL
   return "https://api.dropboxapi.com/2/sharing/modify_shared_link_settings?"
end __modify_shared_link_settings_URL 

private function __mount_folder_URL
   return "https://api.dropboxapi.com/2/sharing/mount_folder?"
end __mount_folder_URL 

private function __relinquish_folder_membership_URL
   return "https://api.dropboxapi.com/2/sharing/relinquish_folder_membership?"
end __relinquish_folder_membership_URL 

private function __remove_folder_member_URL
   return "https://api.dropboxapi.com/2/sharing/remove_folder_member?"
end __remove_folder_member_URL 

private function __revoke_shared_link_URL
   return "https://api.dropboxapi.com/2/sharing/revoke_shared_link?"
end __revoke_shared_link_URL 

private function __share_folder_URL
   return "https://api.dropboxapi.com/2/sharing/share_folder?"
end __share_folder_URL 

private function __transfer_folder_URL
   return "https://api.dropboxapi.com/2/sharing/transfer_folder?"
end __transfer_folder_URL 

private function __unmount_folder_URL
   return "https://api.dropboxapi.com/2/sharing/unmount_folder?"
end __unmount_folder_URL 

private function __unshare_folder_URL
   return "https://api.dropboxapi.com/2/sharing/unshare_folder?"
end __unshare_folder_URL 

private function __update_folder_member_URL
   return "https://api.dropboxapi.com/2/sharing/update_folder_member?"
end __update_folder_member_URL 

private function __update_folder_policy_URL
   return "https://api.dropboxapi.com/2/sharing/update_folder_policy?"
end __update_folder_policy_URL 


------------------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------
------------individual Posts for all API---------------------------------------
------------------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------------------

private function __get_account_POST pUserAccountID
   /*
   account_id String A user's account identifier.
   {"account_id":"pUserAccountID"}
   */
   return __CB(__Q("account_id") & ":" &  __Q(pUserAccountID))
end __get_account_POST

private function __get_current_account_POST
   /*
   No parameters.
   */
   return "null"
end __get_current_account_POST

private function __get_space_usage_POST
   /*
   No parameters.
   */
   return ""
end __get_space_usage_POST

private function __get_account_batch_POST pUserAccountIDs
   //pUserAccountIDs on separate lines
   /*
   GetAccountBatchArg
   account_ids List of (String) List of user account identifiers. Should not contain any duplicate account IDs.
   {"account_ids":["dbid:________________","dbid:_____________________"]}
   */
   if the number of lines of pUserAccountIDs =  1 then
      return __CB(__Q("account_ids") & ":" &  __SB(__Q(pUserAccountIDs)))
   else
      local tgabString
      put "{" & __Q("account_ids") & ":" &  "["  into tgabString
      repeat for each line tAccountID in pUserAccountIDs
         put __Q(tAccountID) & comma after tgabString
      end repeat
      delete last char of tgabString
      put "]"  after tgabString
      return tgabString
   end if
end __get_account_batch_POST

----------------------------------------------------------------

private function __get_metadata_POST pPath,pIncludeMediaInfo
   /*
   GetMetadataArg
   #--path String The path of a file or folder on Dropbox
   #--include_media_info Boolean If true, :field:'FileMetadata.media_info' is set for photo and video. The default for this field is False.
   path can be file or folder
   {"path":"/apps/MyApp/seasons.txt"}
   */
   if pIncludeMediaInfo is empty then put "false" into pIncludeMediaInfo
   return __CB(__Q("path") & ":" &  __Q(pPath) & "," &__Q("include_media_info") & ":" & pIncludeMediaInfo)
end __get_metadata_POST

private function __list_folder_longpoll_POST pCursor,pTimeout
   /*
   ListFolderLongpollArg
   #--cursor String A cursor as returned by list_folder or list_folder/continue
   #--timeout UInt64 A timeout in seconds. The request will block for at most this length of time, 
   plus up to 90 seconds of random jitter added to avoid the thundering herd problem. 
   Care should be taken when using this parameter, as some network infrastructure does not support long timeouts. 
   The default for this field is 30.
   timeout in range [30, 480]
   {"cursor":"1AAGnnWDvEOO2b9xOCXEfhAdJVQeR8jk43Bn2GlNIamvCI2gqCP_Qa129jmJSlbVyIyH6XCEuTjRXUjO-j1jgbk9Lytw0PgEfiq1wartuO9M-kqlfHhRqmu08N-3WD-qNNYs","timeout":30}
   */
   return __CB(__Q("cursor") & ":" &  __Q(pCursor) & "," &__Q("timeout") & ":" & pTimeout)
end __list_folder_longpoll_POST

private function __list_folder_POST pPath,pRecursive,pIncludeMediaInfo
   /*
   ListFolderArg
   #--path String The path to the folder you want to see the contents of.
   #--recursive Boolean If true, the list folder operation will be applied recursively to all subfolders 
   and the response will contain contents of all subfolders. The default for this field is False.
   #--include_media_info Boolean If true, :field:'FileMetadata.media_info' is set for photo and video. The default for this field is False.
   {"path":"/Apps","recursive":true,"include_media_info":false}
   */
   return __CB(__Q("path") & ":" &  __Q(pPath) & "," &__Q("recursive") & ":" & pRecursive & "," &__Q("include_media_info") & ":" & pIncludeMediaInfo)
end __list_folder_POST

private function __list_folder_continue_POST pCursor
   /*
   ListFolderContinueArg
   cursor String The cursor returned by your last call to list_folder or list_folder/continue.
   {"cursor":"1AAEb-y66nJOFj6oqkqCRg01Big_P2chM_2BQle73ZKEIiF6vhScKQQoTpilthdbcu1ML4gqGt6sCZTTYbxwESLhIbIMMxN3z-rElwaoH3v0Y-AYNV6oQm7n7MjkdfOrRm151PeJhHJMZN1Pw_NItgO8UvFnHhLBEsRTa8TGg4DaYtAIpj2pqFKX7w4X6RW7dnUs3BOJUYRsdaLF9Rem0JShPLT_DlRei9VYKDrPJzEliDg"}
   */
   return __CB(__Q("cursor") & ":" &  __Q(pCursor))
end __list_folder_continue_POST

private function __list_folder_get_latest_cursor_POST pPath,pRecursive,pIncludeMediaInfo
   /*
   ListFolderArg
   #--path String The path to the folder you want to see the contents of.
   #--recursive Boolean If true, the list folder operation will be applied recursively to all subfolders and the response 
   will contain contents of all subfolders. The default for this field is False.
   #--include_media_info Boolean If true, :field:'FileMetadata.media_info' is set for photo and video. The default for this field is False.
   {"path":"/Apps/MyApp","recursive":false,"include_media_info":false}
   */
   return __CB(__Q("path") & ":" &  __Q(pPath) & "," &__Q("recursive") & ":" & pRecursive & "," &__Q("include_media_info") & ":" & pIncludeMediaInfo)
end __list_folder_get_latest_cursor_POST

private function __download_POST
   /*
   No parameters. Path in header instead.
   */
   return ""
end __download_POST

private function __upload_session_start_POST pData
   /*
   data-binary @local_file.png
   */
   return pData
end __upload_session_start_POST

private function __upload_session_append_POST pData
   /*
   data-binary @local_file.png
   */
   return pData
end __upload_session_append_POST

private function __upload_session_finish_POST pData
   /*
   data-binary @local_file.png
   */
   return pData
end __upload_session_finish_POST

private function __upload_POST pData
   /*
   data-binary @local_file.png
   */
   return pData
end __upload_POST

private function __search_POST pPath,pQuery,pStart,pMaxResults,pMode
   /*
   SearchArg
   #--path String The path in the user's Dropbox to search. Should probably be a folder.
   #--query String The string to search for. The search string is split on spaces into multiple tokens. For file name searching, the last token is used for prefix matching (i.e. "bat c" matches "bat cave" but not "batman car").
   #--start UInt64 The starting index within the search results (used for paging). The default for this field is 0.
   #--max_results UInt64 The maximum number of search results to return. The default for this field is 100.
   #--mode SearchMode The search mode (filename, filename_and_content, or deleted_filename).
   {"path": "/apps/MyApp","query": "mytext","start": 0,"max_results": 100,"mode": "filename"}
   */
   return __CB(__Q("path") & ":" &  __Q(pPath) & "," &__Q("query") & ":" & __Q(pQuery) & "," &__Q("start") & ":" & pStart & "," &__Q("max_results") & ":" & pMaxResults & "," &__Q("mode") & ":" & __Q(pMode))
end __search_POST

private function __create_folder_POST pPath, pAutoRename
   /*
   CreateFolderArg
   #--path String Path in the user's Dropbox to create.
   {"path": "/Apps/MyApp/new1/season7"}
   */
   return __CB(__Q("path") & ":" &  __Q(pPath), __Q("autorename") & ":" & pAutoRename)
end __create_folder_POST

private function __delete_POST pPath
   /*
   DeleteArg
   #--path String Path in the user's Dropbox to delete.
   {"path": "/apps/MyApp/newFolder31"}
   */
   return __CB(__Q("path") & ":" &  __Q(pPath))
end __delete_POST

private function __permanently_delete_POST pPath
   /*
   DeleteArg
   #--path String Path in the user's Dropbox to delete.
   {"path": "/apps/MyApp/testfile.txt"}
   */
   return __CB(__Q("path") & ":" &  __Q(pPath))
end __permanently_delete_POST

private function __copy_POST pPathFrom, pPathTo, pAllowSharedFolder, pAutoRename
   /*
   RelocationArg
   #--from_path String Path in the user's Dropbox to be copied or moved.
   #--to_path String Path in the user's Dropbox that is the destination.
   {"from_path":"/apps/MyApp/seasons.txt","to_path":"/apps/MyApp/New1/seasons.txt"}
   */
   return __CB(__Q("from_path") & ":" &  __Q(pPathFrom) \
      & "," &__Q("to_path") & ":" & __Q(pPathTo) \
      & "," &__Q("allow_shared_folder") & ":" & pAllowSharedFolder \
      & "," &__Q("autorename") & ":" & pAutoRename)
end __copy_POST

private function __move_POST pPathFrom,pPathTo
   /*
   RelocationArg
   #--from_path String Path in the user's Dropbox to be copied or moved.
   #--to_path String Path in the user's Dropbox that is the destination.
   {"from_path":"/apps/MyApp/seasons.txt","to_path":"/apps/MyApp/New2/seasons.txt"}
   */
   return __CB(__Q("from_path") & ":" &  __Q(pPathFrom) & "," &__Q("to_path") & ":" & __Q(pPathTo))
end __move_POST

private function __get_thumbnail_POST
   /*
   No parameters. Parameters in header instead.
   */
   return ""
end __get_thumbnail_POST

private function __get_preview_POST
   /*
   No parameters. Parameters in header instead.
   */
   return ""
end __get_preview_POST

private function __list_revisions_POST pPath,pLimit
   /*
   ListRevisionsArg
   #--path String The path to the file you want to see the revisions of.
   #--limit UInt64 The maximum number of revision entries returned. The default for this field is 10.
   {"path":"/apps/MyApp/seasons.docx","limit": 10}
   */
   if pLimit is not an integer then 
      put 1000 into pLimit
   end if
   return __CB(__Q("path") & ":" &  __Q(pPath) & "," &__Q("limit") & ":" & pLimit)
end __list_revisions_POST

private function __restore_POST pPath,pRevision
   /*
   RestoreArg
   #--path String The path to the file you want to restore.
   #--rev String The revision to restore for the file.
   {"path":"/apps/MyApp/seasons.docx","rev":"403e457377"}
   */
   return __CB(__Q("path") & ":" &  __Q(pPath) & "," &__Q("rev") & ":" & __Q(pRevision))
end __restore_POST

----------------------------------------------------------------
----------------------------------------------------------------
---------sharing-----------------------------------------

private function __add_folder_member_POST pSharedFolderId,pMembers,pQuiet,pCustomMessage
   /*
   AddFolderMemberArg
   #--shared_folder_id String The ID for the shared folder.
   #--members List of (AddMember) The intended list of members to add. Added members will receive invites to join the shared folder.
   AddMember
   The member and type of access the member should have when added to a shared folder.
   ##----member MemberSelector The member to add to the shared folder.
   MemberSelector (open union)
   Includes different ways to identify a member to add to a shared folder.The value will be one of the following datatypes. New values may be introduced as our API evolves.
   ###------dropbox_id String Dropbox account, team member, or group ID of member to add.
   ###------email String E-mail address of member to add.
   ##----access_level AccessLevel The access level to grant member to the shared folder. AccessLevel. owner is disallowed.
   AccessLevel (open union)
   Defines the access levels for collaborators.The value will be one of the following datatypes. New values may be introduced as our API evolves.
   ###------owner Void The collaborator is the owner of the shared folder. Owners can view and edit the shared folder as well as set the folder's policies using update_folder_policy.
   ###------editor Void The collaborator can both view and edit the shared folder.
   ###------viewer Void The collaborator can only view the shared folder.
   #--quiet Boolean Whether added members should be notified via email and device notifications of their invite. The default for this field is False.
   #--custom_message String? Optional message to display to added members in their invitation. This field is optional.
   
   --{"shared_folder_id":"1172266522","members": [{"member":{".tag":"email","email": "gmctt@tpg.com.au"},"access_level": {".tag":"editor"} }]}
   */
   if pQuiet is empty then put "False" into pQuiet
   if pCustomMessage is empty then put "" into pCustomMessage
   return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId) & "," &__Q("members") & ":" & __FormatAddFolderMembers(pMembers) & "," &__Q("quiet") & ":" & pQuiet & "," &__Q("custom_message") & ":" & __Q(pCustomMessage))
end __add_folder_member_POST

private function __FormatAddFolderMembers pMembers
   -- pMembers is an array  pMembers[1][email] or [1][dropbox_id], [1][accesslevel]
   /*"members": 
   [
   { "member": {".tag": "email","email": "justin@example.com"},"access_level": {".tag": "editor"}},
   {"member": {".tag": "dropbox_id","dropbox_id": "dbid:AAEufNrMPSPe0dMQijRP0N_aZtBJRm26W4Q"},"access_level": {".tag": "editor"}}
   ]
   -- [{"member":{".tag":"email","email": "gmctt@tpg.com.au"},"access_level": {".tag":"editor"} }]
   */
   if pMembers is empty then return "[]" 
   
   local tString
   put "["  into tString
   repeat for each key rKey in pMembers
      if pMembers[rKey]["email"] is not empty then 
         put __Q("member") & ":" after tString
         put __CB(__Q(".tag") & ":" & __Q("email") & "," & __Q("email") & ":" & __Q(pMembers[rKey]["email"])) after tString
         put "," & __Q("access_level") & ":" & __CB(__Q(".tag") & ":" & __Q(pMembers[rKey]["accesslevel"])) after tString
      else
         if pMembers[rKey]["dropbox_id"] is not empty then 
            put __Q("member") & ":" after tString
            put __CB(__Q(".tag") & ":" & __Q("dropbox_id") & "," & __Q("dropbox_id") & ":" & __Q(pMembers[rKey]["dropbox_id"])) after tString
            put "," & __Q("access_level") & ":" & __CB(__Q(".tag") & ":" & __Q(pMembers[rKey]["accesslevel"])) after tString
         end if
      end if
   end repeat
   put "]"  after tString
   return tString
   --
   return pMembers
end __FormatAddFolderMembers

private function __check_job_status_POST pAsyncJobId
   /*
   PollArg
   Arguments for methods that poll the status of an asynchronous job.
   #--async_job_id String Id of the asynchronous job. This is the value of a response returned from the method that launched the job.
   */
   return __CB(__Q("async_job_id") & ":" &  __Q(pAsyncJobId))
end __check_job_status_POST

private function __check_share_job_status_POST pAsyncJobId
   /*
   PollArg
   Arguments for methods that poll the status of an asynchronous job.
   #--async_job_id String Id of the asynchronous job. This is the value of a response returned from the method
   that launched the job. 
   pAsyncJobId from remove_folder_member, share_folder, unshare_folder
   */
   return __CB(__Q("async_job_id") & ":" &  __Q(pAsyncJobId))
end __check_share_job_status_POST

private function __create_shared_link_with_settings_POST pPath,pSettings
   /*
   CreateSharedLinkArg
   #--path String The path to be shared by the shared link 
   #--settings SharedLinkSettings? The requested settings for the newly created shared link This field is optional.
   SharedLinkSettings
   ##--requested_visibility RequestedVisibility? The requested access for this shared link. This field is optional.
   RequestedVisibility (union)
   The access permission that can be requested by the caller for the shared link. 
   Note that the final resolved visibility of the shared link takes into account other aspects, such as team and shared folder settings. 
   Check the ResolvedVisibility for more info on the possible resolved visibility values of shared links. 
   The value will be one of the following datatypes:
   ###--public Void Anyone who has received the link can access it. No login required.
   ###--team_only Void Only members of the same team can access the link. Login is required.
   ###--password Void A link-specific password is required to access the link. Login is not required.
   ##--link_password String? If requested_visibility is RequestedVisibility.password this is needed to specify the password to access the link. This field is optional.
   ##--expires Timestamp? Expiration time of the shared link. By default the link won't expire. This field is optional.
   {"path":"/apps/MyApp/seasons.txt"," "settings": {"requested_visibility": "public"}}
   */
   local tSettings
   repeat for each word tKey in "requested_visibility link_password expires"
      if pSettings[tKey] is not empty then
         if tSettings is not empty then
            put comma after tSettings
         end if
         put __Q(tKey) & ":" & pSettings[tKey] after tSettings
      end if
   end repeat
   
   if tSettings is not empty then
      put comma & __Q("settings") & ":" & __CB(tSettings) into tSettings
   end if
   
   return __CB(__Q("path") & ":" &  __Q(pPath) & tSettings)
end __create_shared_link_with_settings_POST

private function __get_folder_metadata_POST pSharedFolderId,pActions
   /*
   GetMetadataArgs
   shared_folder_id String The ID for the shared folder.
   actions List of (FolderAction)? Folder actions to query. This field is optional.
   FolderAction (open union)
   Actions that may be taken on shared folders. The value will be one of the following datatypes. New values may be introduced as our API evolves.
   change_options Void Change folder options, such as who can be invited to join the folder.
   edit_contents Void Change or edit contents of the folder.
   invite_editor Void Invite a user or group to join the folder with read and write permission.
   invite_viewer Void Invite a user or group to join the folder with read permission.
   relinquish_membership Void Relinquish one's own membership in the folder.
   unmount Void Unmount the folder.
   unshare Void Stop sharing this folder.
   */
   if pActions is not empty then
      return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId) & "," &__Q("actions") & ":" & __List(pActions))
   else
      return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId))
   end if
end __get_folder_metadata_POST

private function __get_shared_link_file_POST pPath
   /*
   No parameters. Parameters in header instead.
   */
   return ""
end __get_shared_link_file_POST

private function __get_shared_link_metadata_POST pUrl,pPath,pLinkPassword
   /*
   GetSharedLinkMetadataArg
   #--url String URL of the shared link.
   #--path String? If the shared link is to a folder, this parameter can be used to retrieve the metadata for a specific file or sub-folder in this folder. A relative path should be used. This field is optional.
   #--link_password String? If the shared link has a password, this parameter can be used. This field is optional.
   */
   if pPath is not empty then
      if pLinkPassword is not empty then
         return __CB(__Q("url") & ":" &  __Q(pUrl) & "," &__Q("path") & ":" & __Q(pPath) & "," &__Q("link_password") & ":" & __Q(pLinkPassword))
      else
         return __CB(__Q("url") & ":" &  __Q(pUrl) & "," &__Q("path") & ":" & __Q(pPath))
      end if
   else
      return __CB(__Q("url") & ":" &  __Q(pUrl))
   end if
end __get_shared_link_metadata_POST

private function __list_folder_members_POST pSharedFolderId,pActions,pLimit
   /*
   ListFolderMembersArgs
   #--shared_folder_id String The ID for the shared folder.
   #--actions List of (MemberAction)? Member actions to query. This field is optional.
   MemberAction (open union)
   Actions that may be taken on members of a shared folder. The value will be one of the following datatypes. New values may be introduced as our API evolves.
   ##----make_editor Void Make the member an editor of the folder.
   ##----make_owner Void Make the member an owner of the folder.
   ##----make_viewer Void Make the member a viewer of the folder.
   ##----remove Void Remove the member from the folder.
   #--limit UInt32 The maximum number of results that include members, groups and invitees to return per request. The default for this field is 1000.
   --
   { "shared_folder_id": "84528192421","actions": [], "limit": 1000}
   */
   if pLimit is not an integer then
      put 1000 into pLimit
   end if
   return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId) & "," & __Q("actions") & ":" & __List(pActions)& "," & __Q("limit") & ":" &  pLimit )
end __list_folder_members_POST

private function __list_folder_members_continue_POST pCursor
   /*
   ListFolderMembersContinueArg
   cursor String The cursor returned by your last call to list_folder_members or list_folder_members/continue.
   */
   return __CB(__Q("cursor") & ":" &  __Q(pCursor))
end __list_folder_members_continue_POST

private function __list_folders_POST pLimit,pActions
   /*
   ListFoldersArgs
   limit UInt32 The maximum number of results to return per request. The default for this field is 1000.
   actions List of (FolderAction)? Folder actions to query. This field is optional.
   FolderAction (open union)
   Actions that may be taken on shared folders. The value will be one of the following datatypes. New values may be introduced as our API evolves.
   change_options Void Change folder options, such as who can be invited to join the folder.
   edit_contents Void Change or edit contents of the folder.
   invite_editor Void Invite a user or group to join the folder with read and write permission.
   invite_viewer Void Invite a user or group to join the folder with read permission.
   relinquish_membership Void Relinquish one's own membership in the folder.
   unmount Void Unmount the folder.
   unshare Void Stop sharing this folder.
   {"limit": 100, "actions": []}
   */
   if pLimit is not an integer then
      put 1000 into pLimit
   end if
   return __CB(__Q("limit") & ":" &  pLimit & "," & __Q("actions") & ":" & __List(pActions))
end __list_folders_POST

private function __list_folders_continue_POST pCursor
   /*
   ListFoldersContinueArg
   cursor String The cursor returned by your last call to list_folder_members or list_folder_members/continue.
   */
   return __CB(__Q("cursor") & ":" &  __Q(pCursor))
end __list_folders_continue_POST

private function __list_mountable_folders_POST pLimit,pActions
   /*
   ListFoldersArgs
   limit UInt32 The maximum number of results to return per request. The default for this field is 1000.
   actions List of (FolderAction)? Folder actions to query. This field is optional.
   FolderAction (open union) 
   */
   if pLimit is not an integer then
      put 1000 into pLimit
   end if
   return __CB(__Q("limit") & ":" &  pLimit & "," & __Q("actions") & ":" & __List(pActions))
end __list_mountable_folders_POST

private function __list_mountable_folders_continue_POST pCursor
   /*
   ListFoldersContinueArg
   cursor String The cursor returned by the previous API call specified in the endpoint description.
   Apps must have full Dropbox access to use this endpoint. 
   {"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"}
   */
   return __CB(__Q("cursor") & ":" &  __Q(pCursor))
end __list_mountable_folders_continue_POST

private function __list_shared_links_POST pPath,pDirectOnly,pCursor
   /*
   ListSharedLinksArg
   path String? See list_shared_links description. This field is optional.
   cursor String? The cursor returned by your last call to list_shared_links. This field is optional.
   direct_only Boolean? See list_shared_links description. This field is optional.
   {"path": "/MySharedFolder1"}
   {"path": "id:a4ayc_80_OEAAAAAAAAAYa"}
   {"path": "rev:a1c10ce0dd78"}
   {"path": "id:a4ayc_80_OEAAAAAAAAAYa","direct_only": true}
   {"cursor": "ZtkX9_EHj3x7PMkVuFIhwKYXEpwpLwyxp9vMKomUhllil9q7eWiAu"}  -from last call to list_shared_links
   {} returns all for user
   */
   if pCursor is not empty then
      if pDirectOnly is not empty then
         --for display in help stack only
         return __CB(__Q("cursor") & ":" &  __Q(pCursor)) & " OR " & __CB(__Q("path") & ":" & __Q(pPath) & "," &__Q("direct_only") & ":" & __Q(pDirectOnly))
      else
         return __CB(__Q("cursor") & ":" &  __Q(pCursor))
      end if
   else
      if pDirectOnly is not empty then
         return __CB(__Q("path") & ":" & __Q(pPath) & "," &__Q("direct_only") & ":" & __Q(pDirectOnly))
      else
         return __CB(__Q("path") & ":" &  __Q(pPath))
      end if
   end if
end __list_shared_links_POST

private function __modify_shared_link_settings_POST pUrl,pSettings, pRemoveExpiration
   /*
   ModifySharedLinkSettingsArgs
   #--url String URL of the shared link to change its settings
   #--settings SharedLinkSettings Set of settings for the shared link.
   SharedLinkSettings
   ##----requested_visibility RequestedVisibility? The requested access for this shared link. This field is optional.
   RequestedVisibility (union)
   The access permission that can be requested by the caller for the shared link. Note that the final resolved visibility of the shared link takes into account other aspects, such as team and shared folder settings. Check the ResolvedVisibility for more info on the possible resolved visibility values of shared links. The value will be one of the following datatypes:
   ###------public Void Anyone who has received the link can access it. No login required.
   ###------team_only Void Only members of the same team can access the link. Login is required.
   ###------password Void A link-specific password is required to access the link. Login is not required.
   ##----link_password String? If requested_visibility is RequestedVisibility.password this is needed to specify the password to access the link. This field is optional.
   ##----expires Timestamp? Expiration time of the shared link. By default the link won't expire. This field is optional.
   */
   local tPost
   put __Q("url") & ":" &  __Q(pUrl) into tPost
   local tSettings
   repeat for each word tKey in "requested_visibility link_password expires"
      if pSettings[tKey] is not empty then
         if tSettings is not empty then
            put comma after tSettings
         end if
         put __Q(tKey) & ":" & pSettings[tKey] after tSettings
      end if
   end repeat
   
   if tSettings is not empty then
      put comma & __Q("settings") & ":" & __CB(tSettings) after tPost
   end if
   
   if pRemoveExpiration is not empty then
      put comma & __Q("remove_expiration") & ":" & pRemoveExpiration after tPost
   end if
   
   return __CB(tPost)
end __modify_shared_link_settings_POST

private function __mount_folder_POST pSharedFolderId
   /*
   MountFolderArg
   #--shared_folder_id String The ID of the shared folder to mount.
   */
   return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId))
end __mount_folder_POST

private function __relinquish_folder_membership_POST pSharedFolderId, pKeepCopy
   /*
   RelinquishFolderMembershipArg
   #--shared_folder_id String The ID for the shared folder.
   */
   
   if pKeepCopy is not true then
      put false into pKeepCopy
   end if
   
   return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId), __Q("leave_a_copy") & ":" & pKeepCopy)
end __relinquish_folder_membership_POST

private function __remove_folder_member_POST pSharedFolderId,pMember,pLeaveACopy
   /*
   RemoveFolderMemberArg
   #--shared_folder_id String The ID for the shared folder.
   #--member MemberSelector The member to remove from the folder. Only the MemberSelector.dropbox_id may be set at this time.
   MemberSelector (open union)
   Includes different ways to identify a member to add to a shared folder.The value will be one of the following datatypes. 
   New values may be introduced as our API evolves.
   ##----dropbox_id String Dropbox account, team member, or group ID of member to add.
   ##----email String E-mail address of member to add.
   #--leave_a_copy Boolean If true, the removed user will keep their copy of the folder after it's unshared, assuming it was mounted. 
   Otherwise, it will be removed from their Dropbox. Also, this must be set to false when kicking a group.
   */
   
   local tMember
   if pMember["dropbox_id"] is not empty then
      put __Q(".tag") & ":" & __Q("dropbox_id"),__Q("dropbox_id") & ":" & __Q(pMember["dropbox_id"]) into tMember
   else
      put __Q(".tag") & ":" & __Q("email"),__Q("email") & ":" & __Q(pMember["email"]) into tMember
   end if
   return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId) & "," &__Q("member") & ":" & __CB(tMember) & "," &__Q("leave_a_copy") & ":" & pLeaveACopy)
end __remove_folder_member_POST

private function __revoke_shared_link_POST pURL
   /*
   RevokeSharedLinkArg
   #--url String URL of the shared link.
   --needs to be full url not short url
   {"url":"https://www.dropbox.com/s/xm2rco8624tno9g/seasons.txt?dl=0"}
   */
   return __CB(__Q("url") & ":" &  __Q(pURL))
end __revoke_shared_link_POST

private function __share_folder_POST pPath,pMemberPolicy,pAclUpdatePolicy,pSharedLinkPolicy,pForceAsync
   /*
   ShareFolderArg
   #--path String The path to the folder to share. If it does not exist, then a new one is created.
   #--member_policy MemberPolicy Who can be a member of this shared folder.
   MemberPolicy (open union)
   Policy governing who can be a member of a shared folder. Only applicable to folders owned by a user on a team.The value will be one of the following datatypes. 
   New values may be introduced as our API evolves.
   ##----team Void Only a teammate can become a member.
   ##----anyone Void Anyone can become a member.
   #--acl_update_policy AclUpdatePolicy Who can add and remove members of this shared folder.
   AclUpdatePolicy (open union)
   Policy governing who can change a shared folder's access control list (ACL). In other words, who can add, remove, or change the privileges of members.
   The value will be one of the following datatypes. New values may be introduced as our API evolves.
   ##----owner Void Only the owner can update the ACL.
   ##----editors Void Any editor can update the ACL. This may be further restricted to editors on the same team.
   #--shared_link_policy SharedLinkPolicy The policy to apply to shared links created for content inside this shared folder.
   SharedLinkPolicy (open union)
   Policy governing who can view shared links.The value will be one of the following datatypes. New values may be introduced as our API evolves.
   ##----anyone Void Links can be shared with anyone.
   ##----members Void Links can only be shared among members of the shared folder.
   #--force_async Boolean Whether to force the share to happen asynchronously. The default for this field is False.
   */
   local tPost
   put __Q("path") & ":" &  __Q(pPath) into tPost
   if pMemberPolicy is not empty then
      put "," &__Q("member_policy") & ":" & __Q(pMemberPolicy) after tPost
   end if
   if pAclUpdatePolicy is not empty then
      put "," &__Q("acl_update_policy") & ":" & __Q(pAclUpdatePolicy) after tPost
   end if
   if pSharedLinkPolicy is not empty then
      put "," &__Q("shared_link_policy") & ":" & __Q(pSharedLinkPolicy) after tPost
   end if
   if pForceAsync is not empty then
      --default false
      put "," &__Q("force_async") & ":" & __Q(pForceAsync) after tPost
   end if
   return __CB(tPost)
end __share_folder_POST

private function __transfer_folder_POST pSharedFolderId,pToDropboxId
   /*
   TransferFolderArg
   #--shared_folder_id String The ID for the shared folder.
   #--to_dropbox_id String A account or team member ID to transfer ownership to.
   */
   return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId) & "," &__Q("to_dropbox_id") & ":" & __Q(pToDropboxId))
end __transfer_folder_POST

private function __unmount_folder_POST pSharedFolderId
   /*
   UnmountFolderArg
   #--shared_folder_id String The ID for the shared folder.
   */
   return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId))
end __unmount_folder_POST

private function __unshare_folder_POST pSharedFolderId,pLeaveACopy
   /*
   UnshareFolderArg
   #--shared_folder_id String The ID for the shared folder.
   #--leave_a_copy Boolean If true, members of this shared folder will get a copy of this folder after it's unshared. 
   Otherwise, it will be removed from their Dropbox. The current user, who is an owner, will always retain their copy.
   */
   if pLeaveACopy is empty then put "false" into pLeaveACopy
   return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId) & "," &__Q("leave_a_copy") & ":" & pLeaveACopy)
end __unshare_folder_POST

private function __update_folder_member_POST pSharedFolderId,pMember,pAccessLevel
   /*
   UpdateFolderMemberArg
   #--shared_folder_id String The ID for the shared folder.
   #--member MemberSelector The member of the shared folder to update. Only the MemberSelector.dropbox_id may be set at this time.
   MemberSelector (open union)
   Includes different ways to identify a member to add to a shared folder.The value will be one of the following datatypes. 
   New values may be introduced as our API evolves.
   ##----dropbox_id String Dropbox account, team member, or group ID of member to add.
   ##----email String E-mail address of member to add.
   #--access_level AccessLevel The new access level for member. AccessLevel.owner is disallowed.
   AccessLevel (open union)
   Defines the access levels for collaborators.The value will be one of the following datatypes. New values may be introduced as our API evolves.
   ##----owner Void The collaborator is the owner of the shared folder. Owners can view and edit the shared folder as well as set the folder's policies using update_folder_policy.
   ##----editor Void The collaborator can both view and edit the shared folder.
   ##----viewer Void The collaborator can only view the shared folder.
   */
   return __CB(__Q("shared_folder_id") & ":" &  __Q(pSharedFolderId) & "," &__Q("member") & ":" & __FormatUpdateFolderMember(pMember) & "," &__Q("access_level") & ":" & __Q(pAccessLevel))
end __update_folder_member_POST

private function __FormatUpdateFolderMember pMember
   -- pMember is an array  pMember["email"] or pMember["dropbox_id"]
   /*
   --{".tag":"email","email":"myemailaddress"}
   --{".tag": "dropbox_id","dropbox_id": "dbid:mydropboxid"}
   */
   if pMember["email"] is not empty then 
      return __CB(__Q(".tag") & ":" & __Q("email") & "," & __Q("email") & ":" & __Q(pMember["email"]))
   end if
   if pMember["dropbox_id"] is not empty then 
      return __CB(__Q(".tag") & ":" & __Q("dropbox_id") & "," & __Q("dropbox_id") & ":" & __Q(pMember["dropbox_id"]))
   end if
   return pMember  --this is useful for syntax display and also allows preformatted json to be passed eg. {".tag":"email","email":"myemailaddress"}
end __FormatUpdateFolderMember

private function __update_folder_policy_POST pSharedFolderId,pMemberPolicy,pAclUpdatePolicy,pSharedLinkPolicy
   /*
   UpdateFolderPolicyArg
   If any of the policy's are unset, then they retain their current setting.
   #--shared_folder_id String The ID for the shared folder.
   #--member_policy MemberPolicy? Who can be a member of this shared folder. Only set this if the current user is on a team. This field is optional.
   MemberPolicy (open union)
   Policy governing who can be a member of a shared folder. Only applicable to folders owned by a user on a team.
   The value will be one of the following datatypes. New values may be introduced as our API evolves.
   ##----team Void Only a teammate can become a member.
   ##----anyone Void Anyone can become a member.
   #--acl_update_policy AclUpdatePolicy? Who can add and remove members of this shared folder. This field is optional.
   AclUpdatePolicy (open union)
   Policy governing who can change a shared folder's access control list (ACL). In other words, who can add, remove, or change the privileges of members.
   The value will be one of the following datatypes. New values may be introduced as our API evolves.
   ##----owner Void Only the owner can update the ACL.
   ##----editors Void Any editor can update the ACL. This may be further restricted to editors on the same team.
   #--shared_link_policy SharedLinkPolicy? The policy to apply to shared links created for content inside this shared folder. This field is optional.
   SharedLinkPolicy (open union)
   Policy governing who can view shared links.The value will be one of the following datatypes. New values may be introduced as our API evolves.
   ##----anyone Void Links can be shared with anyone.
   ##----members Void Links can only be shared among members of the shared folder.
   */
   local tPost
   put __Q("shared_folder_id") & ":" &  __Q(pSharedFolderId) into tPost
   if pMemberPolicy is not empty then
      put "," &__Q("member_policy") & ":" & __Q(pMemberPolicy) after tPost
   end if
   if pAclUpdatePolicy is not empty then
      put "," &__Q("acl_update_policy") & ":" & __Q(pAclUpdatePolicy) after tPost
   end if
   if pSharedLinkPolicy is not empty then
      put "," &__Q("shared_link_policy") & ":" & __Q(pSharedLinkPolicy) after tPost
   end if
   return __CB(tPost)
end __update_folder_policy_POST

--------------------------------------------------------------------
private function __QInStr pString,pAltChar
   if pAltChar = empty then put "^" into pAltChar
   replace pAltChar with quote in pString
   return pString
end __QInStr

private function __Q pString
   return quote & pString & quote
end __Q

private function __CB pString
   return "{" & pString & "}"
end __CB

private function __SB pString
   return "[" & pString & "]"
end __SB

private function __List pString
   local tList
   repeat for each line tLine in pString
      if tList is not empty then 
         put comma after tList
      end if
      put __Q(tLine) after tList
   end repeat
   return __SB(tList)
end __List
