# LiveCode Builder Host Library
## Widget Library
New syntax has been added to obtain a reference to the widget, and to use that
reference when notifying the engine of property changes via the `trigger all`
command.

This is most useful when LCB handlers within a widget module are used as
asynchronous callback functions passed to foreign functions, as these may be
called at a time when the widget is not the currently active widget. Using the
reference prevents updates being seen as coming from the wrong widget.
