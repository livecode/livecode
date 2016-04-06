---
version: 8.0.0-dp-9
---
# New "documentFilename" stack property

A new property has been added to specify the file path to the file that a stack
represents.

On Mac OS X, setting the **documentFilename** property will set the
represented filename of the window. The window will show an icon for
the file next to the window title.

On other platforms there is no visual representation of the
association between the stack and the document file, but the property
may still be used to manage the association.
