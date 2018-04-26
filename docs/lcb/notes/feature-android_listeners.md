# LiveCode Builder Language
## Android Listener support
An binding string variant has been added which allows the user to 
create an interface proxy - that is an instance of a generic 
Proxy class for a given interface. 

This effectively allows LCB handlers to be registered as the targets 
for java interface callbacks, such as event listeners.

The syntax is as follows:

	foreign handler _JNI_CreateListener(in pMapping) returns JObject binds to "java:listener.class.path>interface()"

The foreign handler binding to such a function takes a value that should
either be a `Handler` or an `Array` - if it is a `Handler`, the specified 
listener should only have one available callback. If the listener has 
multiple callbacks, an array can be used to assign handlers to each. Each 
key in the array must match the name of a callback in the listener. 
Overloaded methods in the interface are not currently supported.

For example:

	handler type ClickCallback(in pView as JObject)

	foreign handler _JNI_OnClickListener(in pHandler as ClickCallback) returns JObject binds to "java:android.view.View$OnClickListener>interface()"

	foreign handler _JNI_SetOnClickListener(in pButton as JObject, in pListener as JObject) returns nothing binds to "java:android.view.View>setOnClickListener(Landroid/view/View$OnClickListener;)V"	
	
	public handler ButtonClicked(in pView as JObject)
		-- do something on button click
	end handler
	
	public handler SetOnClickListenerCallback(in pButton as JObject)
		unsafe
			variable tListener as JObject
			put _JNI_OnClickListener(ButtonClicked) into tListener
			_JNI_SetOnClickListener(pButton, tListener)
		end unsafe
	end handler
	
or

	handler type MouseEventCallback(in pMouseEvent as JObject)

	foreign handler _JNI_MouseListener(in pCallbacks as Array) returns JObject binds to "java:java.awt.event.MouseListener>interface()"

	foreign handler _JNI_SetMouseListener(in pJButton as JObject, in pListener as JObject) returns nothing binds to "java:java.awt.Component>addMouseListener(Ljava/awt/event/MouseListener;)V"	
	
	public handler MouseEntered(in pEvent as JObject)
		-- do something on mouse entter
	end handler
	
	public handler MouseExited(in pEvent as JObject)
		-- do something on mouse entter
	end handler
	
	public handler SetMouseListenerCallbacks(in pJButton as JObject)
		variable tArray as Array
		put MouseEntered into tArray["mouseEntered"]
		put MouseExited into tArray["mouseExited"]
		unsafe
			variable tListener as JObject
			put _JNI_MouseListener(tArray) into tListener
			_JNI_SetMouseListener(pJButton, tListener)
		end unsafe
	end handler
