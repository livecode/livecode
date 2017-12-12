# LiveCode Builder Language
## Objective-C dynamic instance method binding
It is now possible to bind to instance methods dynamically, by leaving
empty the name of the class in the binding string. In particular this 
enables calling protocol-defined instance methods on class instances,
which was not previously possible. 

For example, to directly call the `NSAlertDelegate` method 
`alertShowHelp:` on an `NSAlert` instance:

	-- bind to delegate protocol method
	foreign handler NSAlertDelegateShowHelp(in pTarget as ObjcId, in pAlert as ObjcId) binds to "objc:.-alertShowHelp:"
	...
	-- call alertShowHelp on an NSAlert, passing the alert itself as the
	-- first parameter.
	NSAlertDelegateShowHelp(tAlert, tAlert)