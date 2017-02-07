# Message box properties aded to all engines

Previously global properties `revMessageBoxRedirect` and
`revMessageBoxLastObject` were only availbe in the IDE engine. These
properties have now been renamed to `messageBoxRedirect` and 
`messageBoxLastObject` and they are now available in all engines.

Set the `messageBoxRedirect` to the long id of the object that should
receive `msgChanged` messages whenever the `msg` global is modified.
