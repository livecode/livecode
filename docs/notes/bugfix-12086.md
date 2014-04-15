# rawKeyDown sends incorrect code for shifted keys on Mac.
The rawKeyDown (and Up) messages will now send identical codes for key presses as previous engines. (Note that these are the codes for the ASCII characters you get for a US English keyboard layout, taking into account shift state).
