# Add functions for getting synchronous modifier key state

LiveCode currently provides functions for checking the state of so-called
"modifier" keys: Caps Lock, Control, Command, Shift, Alt/Option. These
functions return either "up" or "down", reflecting the state of the key at the
time the function was called. However, it is often desireable to check the
state of the key at the time the event was generated and this is not possible
using these functions.

New functions called "eventAltKey", "eventShiftKey", etc have been added; these
return the state of the key at the time the event began processing. This is
useful in keyDown and rawKeyDown handlers to check whether a modifier was
pressed at the time the key the event relates to was pressed (if the non-event
forms are used instead, there is a chance the modifier key has been released
and the wrong result will be generated).

Note that the "eventXXXKey" functions should *not* be called after a wait;
their value is undefined after any form of wait has occurred.
