# Clipboard data 'styledText' array accessor.
A new clipboard format has been added 'styledText'. This format returns (or sets) the clipboard to a styled text array - the same format as the 'styledText' property of field chunks. All text formats can convert to and from the 'styledText' key.
For example, you can now do:
   set the clipboardData["styledText"] to the styledText of line 5 of field 3
   set the styledText of line 6 of field 3 to the clipboardData["styledText"]

Note that the dragData can now also be used with this new format in exactly the same way.
