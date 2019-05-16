---
version: 9.0.0-rc-1
---
# LiveCode Builder Host Library

## Widget Touch Handlers

Widgets can now handle the following touch messages:

- `OnTouchStart` - sent when the touch first starts
- `OnTouchMove` - sent when the touch moves
- `OnTouchFinish` - sent when the touch ends without being canceled
- `OnTouchCancel` - sent when the touch is canceled

When a widget hit tests as the target for the first of a sequence of
touches and implements one of the touch event handlers it will receive all
events for the sequence otherwise it will not receive any. A sequence of
touches begins with the start of the first touch until there are no more
active touches.

While a widget is the target for touch events the touch messages will not
be sent to the LCS object unless the widget posts them. If the widget
becomes invisible, or is closed whilst active touches are present
`OnTouchCancel` is sent for all of them.

The currently active touch (the one which caused the event to
bubble) can be found by using `the touch id`, and its
position using `the touch position`. The id is an integer beginning with
1 and incremented for each touch in the sequence. It is unrelated to the
id parameter for LCS touch messages.

The number of touches currently in progress can be found by using
`the number of touches` and the position of a given touch using
`the position of touch tID`. A list of currently active touch IDs can
be found using `the touch ids`.

Note: Due to the way touch messages are currently processed above
the widget event system, widgets will receive mouse messages and
then touch messages. To make a widget which works on both Desktop
and Mobile, and wants touches on Mobile, the widget must have both
mouse and touch handlers, and check for platform in the mouse handlers.
