# LiveCode Builder Host Library
## Canvas library

New syntax has been added to the Canvas library to allow specifying the drawing boundary
when beginning a new layer for an effect. This should be a rectangle signifying the bounds
of any drawing performed on the layer.

This information allows the canvas library to limit the amount of memory allocated to the
new layer by restricting the size of its backing buffer to only that area required to
correctly render effects based on the given drawing bounds.

*Note:* The drawing bounds provided should enclose only the area drawn to, excluding the
extended area required to draw the effect as this will be automatically accounted for by
the canvas library when creating the new layer. For example, When drawing a shape with a
shadow effect, you need only specify the bounds of the shape rather than include the area
covered by the shadow.

Example:
public handler OnPaint
	// initialize effect
	variable tProps as Array

	put color [0,0,0,0.5] into tProps["color"]
	put "source over" into tProps["blend mode"]
	put 0 into tProps["spread"]
	put 3 into tProps["size"]
	put 25 into tProps["distance"]
	put 45 into tProps["angle"]

	variable tShadow as Effect
	put outer shadow effect with properties tProps into tShadow
	
	// initialize shape bounds
	variable tBounds as Rectangle
	put rectangle [25, 25, 75, 75] into tBounds

	// draw rounded rectangle with shadow
	begin layer with tShadow for tBounds on this canvas
	fill rounded rectangle path of tBounds on this canvas
	end layer on this canvas	
end handler