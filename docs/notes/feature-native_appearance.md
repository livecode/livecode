# More native-looking stack theming

The default appearance of stacks on desktop platforms has been updated to more
closely match the appearance of native apps. This particularly affects OSX
and Linux apps.

One effect of this is that the default text size is now different for different
types of controls. As this may cause layout issues for existing stacks, it is
possible to set the theme property of a stack, card or control to "legacy" to
restore the old behavior. The new behavior can be explicitly requested for an
object by setting the theme to "native".
