# Use more accurate measurements when laying out text.

The measurements used by LiveCode to lay out text (the text "metrics") in versions prior to 8 were optimised for low-resolution bitmap fonts. In LiveCode 8, more accurate measurements are now being used by the field and other controls.

For many existing stacks, the most visible change is that lines in the field are now closer together and better match the text display in other programs. If this is not desirable, a fixed line height can be set on the field to emulate the old behaviour.

