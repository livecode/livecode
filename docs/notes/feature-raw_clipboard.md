---
version: 8.0.0-dp-8
---
# Improved clipboard and drag-and-drop support

It is now possible to put multiple types of data on the clipboard at
the same time. For example, you can put an image on the clipboard
along with text describing the image and some private data associated
with the image.

For more information, see the **fullClipboardData** and
**fullDragData** entries in the dictionary.

For apps requiring more sophisticated clipboard functionality, new
**rawClipboardData** and **rawDragData** properties have also been
added. These provide low-level access to the system clipboard.

**This feature was sponsored by
[FMProMigrator](https://www.fmpromigrator.com)**.
