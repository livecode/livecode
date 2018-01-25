# Return Typeface object from MCCanvasFontGetHandle on Android

The canvas module's font handle is intended to return a pointer
that can be used on the given platform to set the font of native
views on that platform. In the case of android, the handle was
a Skia Typeface pointer rather than a Typeface Java object.

Now the returned pointer can be wrapped using PointerToJObject
and used directly as the Typeface parameter in android java ffi
calls (such as setTypeface).

Note, the pointer returned by MCCanvasFontGetHandle is an 
unwrapped jobject local ref, so can only be expected to live
while the calling handler is running. If the Typeface object
is required elsewhere, it must be wrapped using PointerToJObject
and put in a module variable. 
