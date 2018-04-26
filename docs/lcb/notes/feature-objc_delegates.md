# LiveCode Builder Language
## Objective-C delegate support
Handlers `CreateObjcDelegate` and `CreateObjcDelegateWithContext` have 
been added to the objc module which allow the creation of custom 
delegate objects with LCB implementations of protocol methods.
   
In order to create a delegate to handle a particular protocol method, 
pass in the protocol name as the first argument and the mapping from 
method names to LCB handlers as the second argument. For example, to 
create a selectionChanged message for an `NSTextView`, we need to create 
a handler

	private handler DidChangeSelection(in pNotification as ObjcObject) returns nothing
		post "selectionChanged"
	end handler

and create a `NSTextViewDelegate`:

	variable tDelegate as optional ObjcObject
	put CreateObjcDelegate( \
		"NSTextViewDelegate", \ 	
		{"textViewDidChangeSelection:": DidChangeSelection}, \
		) into tDelegate
	if tDelegate is not nothing then
		put tDelegate into mTextViewDelegate
	end if
	
Optionally, a context parameter can be passed in at delegate creation
time:

	put CreateObjcDelegateWithContext( \
		"NSTextViewDelegate", \ 	
		{"textViewDidChangeSelection:": DidChangeSelectionContext}, \
		tContext) into tDelegate
		
	if tDelegate is not nothing then
		put tDelegate into mTextViewDelegate
	end if

In this case the context variable will be passed as first argument of 
the corresponding LCB callback:

	private handler DidChangeSelectionContext(in pContext, in pNotification as ObjcObject) returns nothing
		post "selectionChanged" with [pContext]
	end handler

Some protocols consist of purely optional methods. In this case the 
information about the protocol's methods are not available from the 	
objective-c runtime API. For this eventuality there are also handlers
`CreateObjcInformalDelegate` and `CreateObjcInformalDelegateWithContext`.

These handlers take a list of foreign handlers as their first argument
instead of a protocol name. The foreign handlers' information is used to
resolve incoming selectors so that the desired LCB callback is called. 
For example the `NSSoundDelegate` protocol has only one method, and it 
is optional, 

	- (void)sound:(NSSound *)sound didFinishPlaying:(BOOL)aBool;

So in order to create an `NSSoundDelegate`, we need to create a list of
foreign handlers, in this case just the following:

	foreign handler NSSoundDidFinishPlaying(in pSound as ObjcId, in pDidFinish as CSChar) binds to "objc:.-sound:didFinishPlaying:"
	
and create the informal delegate
	
	handler DidSoundFinish(in pSound as ObjcId, in pDidFinish as Boolean) returns nothing
		if pDidFinish then
			post "soundFinished"
		end if
	end handler
	
	foreign handler Objc_SetSoundDelegate(in pSound as ObjcId, in pDelegate as ObjcId) returns nothing \
		binds to "objc:NSSound.-setDelegate:"
	...
	
	variable tDelegate as optional ObjcObject
	put CreateObjcInformalDelegate( \
		[NSSoundDidFinishPlaying], \ 	
		{"textViewDidChangeSelection:": DidChangeSelection}) \
		into tDelegate
	end if
	if tDelegate is not nothing then
		put tDelegate into mSoundDelegate
		Objc_SetSoundDelegate(tSound, tDelegate)
	end if

> *Note:* Delegate properties are usually 'assigned' rather than 
> 'retained', so it is necessary to store them in module variables 
> until they are no longer needed. Generally the pattern required is
> as follows:

	handler OnOpen()
		-- Create native view and set native layer
		-- Set native view delegate property
		-- Store view and delegate in module vars
	end handler

	handler OnClose()
		-- Set native view delegate property to nothing
		-- Put nothing into view and delegate module vars
		-- Set native layer to nothing
	end handler