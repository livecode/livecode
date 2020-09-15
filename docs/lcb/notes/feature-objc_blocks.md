# LiveCode Builder Language

## Objective-C block support

The handler `CreateObjcBlockPointerFromHandler` has been added the `objc` module
which facilitates the calling of Objective-C methods that require blocks.
Objective-C blocks are objects defining fragments of code which can be called at
a later date, typically used by methods that require progress callbacks or
completion handlers.

`CreateObjcBlockPointerFromHandler` wraps a LCB handler in an `ObjcBlockPointer`
and returns true on success and false otherwise. In order to call the
`CreateObjcBlockPointerFromHandler`, pass the handler to be wrapped as the first
argument and the variable into which the `ObjcBlockPointer` should be placed as
the second.

	private variable sRequestPermissionsCompletionHandler as optional ObjcBlockPointer
	private variable sTarget as ScriptObject

	public handler AudioLibraryInitialize() returns Boolean
		if not CreateObjcBlockPointerFromHandler(RequestPermissionsCompletionHandler, \
													sRequestPermissionsCompletionHandler) then
			put nothing into sRequestPermissionsCompletionHandler
			return false
		end if
		put the caller into sTarget
		return true
	end handler

	public handler RequestPermissionsCompletionHandler(in pBlock as ObjcBlockPointer, in pGranted as CBool)
		post "AudioLibraryRequestPermissionsCallback" to sTarget with [pGranted]
	end handler

In the example above, the handler `RequestPermissionsCompletionHandler` is
wrapped by the `ObjcBlockPointer` `sRequestPermissionsCompletionHandler`.

The first parameter of the wrapped handler should be an `ObjcBlockPointer`. The
remaining parameters should match those of the Objective-C block the
`ObjcBlockPointer` will be used with.

Once created, an `ObjcBlockPointer` can be used to call Objective-C methods that
require blocks, passing the created `ObjcBlockPointer` where an Objective-C
block would be expected.

	private foreign handler ObjC_AVCaptureDeviceRequestAccessForMediaType(in pMediaType as ObjcId, in pCompletionHandler as ObjcBlockPointer) \
		returns nothing \
		binds to "objc:AVCaptureDevice.+requestAccessForMediaType:completionHandler:"

	public handler AudioLibraryRequestPermissions()
		unsafe
			ObjC_AVCaptureDeviceRequestAccessForMediaType(StringToNSString("soun"), sRequestPermissionsCompletionHandler)
		end unsafe
	end handler

In the example above, the handler `AudioLibraryRequestPermissions` uses the
previously created block pointer, `sRequestPermissionsCompletionHandler`, when
calling the `requestAccessForMediaType:completionHandler:` method of the
`AVCaptureDevice` type. The method expects the second argument,
`completionHandler:`, to be a block that takes a single `BOOL` parameter, which
in this case matched the signature of the handler
`RequestPermissionsCompletionHandler`.

The wrapped handler will be called whenever the block is invoked, with the first
parameter being the `ObjcBlockPointer` used to wrap the handler.

In the above example, the `RequestPermissionsCompletionHandler` handler will be
called when the `completionHandler:` block is invoked, which in this case will
be when the request permissions process has completed.

The lifetime of a created `ObjcBlockPointer` is not automatically managed. When
such a value has no more references to it and it is no longer going to be used,
`DeleteObjcBlockPointer` should be used to free the resources used by it.

	public handler AudioLibraryFinalize()
		if sRequestPermissionsCompletionHandler is not nothing then
			DeleteObjcBlockPointer(sRequestPermissionsCompletionHandler)
			put nothing into sRequestPermissionsCompletionHandler
		end if
	end handler
