# Improve error message when calling LCB library function incorrectly.

If a handler in an LCB library is called with the wrong number of
arguments then the engine will now throw either a "too few arguments"
or "too many arguments" error.
