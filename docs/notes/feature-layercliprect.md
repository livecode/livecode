# New layerClipRect control property

A new property 'layerClipRect' has been added to all controls.

Use the layerClipRect property to clip an object's display to a rectangle.
The clipping rectangle only changes what part of the object is rendered,
it has no effect on interaction; in particular, mouse events will still
occur as they would without it being set.
