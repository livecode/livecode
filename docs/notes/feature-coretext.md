# Updated text rendering for iOS and OS X

In order to improve performance, the text rendering routines for iOS and OS X and been updated to use the latest APIs. This has had a significant improvement in the text rendering speed, particularly on OS X.

It's worth noting that the previous OS X routines used synthesised font styles. That is, bold and italic styles were emulated (by slanting or thickening) if the font being rendered was not bold or italic. This is not the case for the new routines. If there is not a font present on the system with the given style, the plain alternative will be used.

The is the case with the default font - Lucida Grande. By default, systems only come with the bold variant. Thus, if you have a field with the default font and italic style, it will be rendered in plain style.
