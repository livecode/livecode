#Full screen scaling mode.

There are multiple ways in which a stack can be resized or scaled to take full advantage of the available screen space. The full screen scaling mode allows the developer to choose the most appropriate for their application.

The fullscreen scaling mode of a stack can be changed by setting its **fullscreenmode** property to one of the following values:

* empty (default) - the existing behaviour - the stack is resized (not scaled) to fit the screen.
* "exact fit" - scale the stack to fill the screen. This will stretch the stack if the aspect ratio of the screen does not match that of the stack.
* "show all" - scale the stack preserving aspect ratio so all content is visible. Some blank space may remain if the screen & stack aspect ratios do not match.
* "no border" - scale the stack to fill the screen preserving aspect ratio. If the stack & screen aspect ratios do not match, the left / right or top / bottom extremes of the stack will not be visible.
* "no scale" - the stack will not be scaled, being centered on the screen instead.


Note that the **fullscreenmode** only takes effect when a stack is fullscreen. This will be the case on mobile platforms where stacks are always fullscreen, or on the desktop when the **fullscreen** property of the stack is set to true.

The fullscreen scaling mode is available on all desktop and mobile platforms and operates independently from Hi-DPI support.

