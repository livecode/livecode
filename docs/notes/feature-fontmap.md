# Font mapping feature
From 6.7.5 RC 1, a support for font name mapping had been added in the standalone builder settings for iOS.
Updating to iOS 8.1 caused some stacks to startup extremely slowly, and that is due to the fact that setting a font using a name different from its actual PostScript name has gotten dramatically slower from iOS 8.1 (that slowdown was already present earlier, but not that noticeable).

We internally added a mapping file for all the default iOS fonts, so that the displayed names will not change.

As for the custom fonts any user would add in an application, we are not able to provide such a mapping file. Whence adding a custom font mapping file to a project is now possible from the iOS standalone settings.

# Font mapping file
Each font name is mapped to its PostScript name in this way, one mapping per line:
`<font name>=<PostScript name>`
`<font name 2>=<PostScript name 2>`

* `font name` is the name used in the call `set the font of <object> to <font name>`
* `PostScript Name` is the PostScript name as referenced from the Font Book.
