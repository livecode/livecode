# LiveCode Builder Host Library
## Canvas library

A new statement `clip to <path> on <canvas>` has been implemented to allow drawing on a
canvas to be clipped to the path. Previously the clip region could only be set to a
rectangle via the `clip to <rectangle> on <canvas>` statement.

For example to draw an image into a circle:

    clip to circle path centered at point [my width /2,my height / 2] with radius my width/2 on this canvas
    variable tImage as Image
    put image from resource file "foo.png" into tImage
    draw tImage into rectangle [0, 0, the width of tImage, the height of tImage] of this canvas