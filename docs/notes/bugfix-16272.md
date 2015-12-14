---
version: 8.0.0-dp-8
---
# Changed behaviour of submenu menu items

Previously, clicks to menu items that open submenus would dismiss
the menu and send a **mouseRelease** message. Unfortunately, this
did not match the conventions of modern user interfaces where
these clicks are ignored. This can be seen on OSX where the
system's menus are used instead of being emulated by LiveCode.

Now, clicking on the item that anchors a submenu will send the
**mouseRelease** message as before but, if that message is not handled
or is passed, the click will be ignored and the menu will remain
on-screen.

To restore the old behaviour, handle the **mouseRelease** message
without passing. To add the new behaviour to existing code that
handles the **mouseRelease** message, add a **pass** command to the
end of the handler.

