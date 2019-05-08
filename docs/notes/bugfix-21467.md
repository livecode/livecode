# New mobileSetKeyboardDisplay and mobileGetKeyboardDisplay handlers

A new command `mobileSetKeyboardDisplay` has been added to support a `pan` mode
where the view is panned up if the currently focused field control is not visible
when the keyboard is shown. Use `mobileGetKeyboardDisplay` to get the current
mode. There are two modes supported:

- `over` - the default where the keyboard displays over the stack
- `pan` - the view is panned up the minimum amount required to ensure the
foucused field is visible.