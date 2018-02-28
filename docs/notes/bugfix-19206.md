# clipboard always converts plain text to styled text

Whenever `text` was placed on the clipboard, it was first converted to
LiveCode styled text and then put on the clipboard as styled text, RTF,
HTML, and plain text.  This introduced errors when pasting to other
applications since they would prefer the HTML version which made the
text appear double spaced.