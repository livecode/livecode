# Nine-way stretch for images
You can now set 'the centerRect' property of an image. This property should be a rectangle, with co-ordinates relative to the formattedRect of the image.
The property specifies the area of the image that should be stretched when the image is scaled.
For example, if the centerRect of an image which is 16x16 is set to 4,4,12,12 then:
* The 4x4 corner portions of the image will not be stretched
* The top 4x8 and 8x4 side portions of the image will be stretched horizontally or vertically (depending on orientation)
* The middle 8x8 area will stretch to fill the middle.
This property is useful for using images as backgrounds to buttons and interface elements - allowing a non-stretched border with stretched interior to be specified.
