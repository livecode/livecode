# LiveCode Builder Host Library
## Widget library

New statements have been implemented to manage widget focus.

- `focus in` - Set the keyboard focus to the widget
- `focus out` - Focus on no control
- `focus next` - Focus on the next control with traversal on
- `focus previous` - Focus on the previous control with traversal on

Additionally the messages `OnFocusEnter` and `OnFocusLeave` have been
implemented to allow a widget to handle focus changes.
