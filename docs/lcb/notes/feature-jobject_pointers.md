# LiveCode Builder Standard Library

## Java Utilities

Syntax for converting between a JObject and Pointer have been added to the 
Java utility library.

* `PointerToJObject` - converts a Pointer to a JObject
* `PointerFromJObject` - converts a JObject to a Pointer

These can be used in APIs where `Pointer` is the type of a platform-agnostic 
parameter whose underlying type is assumed to be `jobject` when used in a 
platform-specific implementation, for example:

	-- pView is assumed to be a JObject with underlying type android.view.View
	set my native layer to PointerFromJObject(pView) 
