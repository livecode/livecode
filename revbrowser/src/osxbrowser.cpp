/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "osxbrowser.h"

////////////////////////////////////////////////////////////////////////////////

enum
{
	kEventClassRevBrowser = 'REVB'
};

enum
{
	kEventRevBrowser = 0
};

////////////////////////////////////////////////////////////////////////////////

inline int min(int a, int b)
{
	return a < b ? a : b;
}

inline int max(int a, int b)
{
	return a > b ? a : b;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

@interface printDelegate : NSObject 
{
}

- (void)printingFinished: (NSPrintOperation *)printOperation
				 success:(BOOL)success    
			 contextInfo:(void *)info;
@end

@implementation printDelegate

- (void)printingFinished: (NSPrintOperation *)printOperation
				 success:(BOOL)success
			 contextInfo:(void *)info
{
    if( success )
		[printOperation cleanUpOperation];
}
@end

////////////////////////////////////////////////////////////////////////////////


void
OpenDialogEventProc( const NavEventCallbackMessage callbackSelector, 
									   NavCBRecPtr callbackParms, 
									   NavCallBackUserData callbackUD );

@interface WebBrowserAdapter : NSObject
{
	HIObjectRef	_object;
	TAltBrowser *m_browser;
	DOMHTMLElement *m_previous_element;
}

- initWithHIObject: (HIObjectRef)inObject;
- dealloc;

- (void)setBrowser: (TAltBrowser *)inBrowser;
- (HIObjectRef)hiobject;
- (void)webView:(WebView *)sender runJavaScriptAlertPanelWithMessage:(NSString *)message;
@end

@implementation WebBrowserAdapter

- initWithHIObject: (HIObjectRef)inObject
{
    self = [super init];
	if ( self )
	{
		_object = inObject;	// non retained
		m_previous_element = NULL;
	}
    return self;
}

- dealloc
{
	if (m_previous_element != NULL)
		[m_previous_element release];
	[super dealloc];
}

- (void)setBrowser: (TAltBrowser *)inBrowser
{
	m_browser = inBrowser;
}

- (HIObjectRef)hiobject
{
	return _object;
}

- (void)webView: (WebView *)sender decidePolicyForNavigationAction:
  (NSDictionary *)actionInformation request:(NSURLRequest *)request frame:
  (WebFrame *)frame decisionListener:(id<WebPolicyDecisionListener>)listener
{
    NSString * urlKey = [[actionInformation objectForKey:WebActionOriginalURLKey] absoluteString];
	
	int i = [[actionInformation objectForKey:@"WebActionNavigationTypeKey"] intValue];
	
	if ( [urlKey compare:@"about:blank"] != NSOrderedSame )
	{
		bool t_cancel = false;
		if (frame == [sender mainFrame])
			CB_NavigateRequest(m_browser -> GetInst(), [urlKey cString], &t_cancel);
		else
			CB_NavigateFrameRequest(m_browser -> GetInst(), [urlKey cString], &t_cancel);
			
		if (!t_cancel)
			[listener use];
		else
			[listener ignore];

	}
	else
		[listener use];
}

- (void)webView: (WebView *)sender didCommitLoadForFrame:(WebFrame *)frame
{
	NSString *strUrl;
	strUrl = [[[[frame dataSource] request] URL] absoluteString];
	
	if (frame == [sender mainFrame])
		CB_NavigateComplete(m_browser -> GetInst(), [strUrl cString]);
	else
		CB_NavigateFrameComplete(m_browser -> GetInst(), [strUrl cString]);
}

- (void)webView: (WebView *)sender decidePolicyForMIMEType:(NSString *)ptype request:(NSURLRequest *)request frame:
    (WebFrame *)frame decisionListener:(id<WebPolicyDecisionListener>)listener
{
	NSURL * urlKey = [request URL];
	NSString * strUrl = [urlKey absoluteString];
	
	if ( [WebView canShowMIMEType:ptype] )
		[listener use];
	else
	{
		[listener ignore];
		
		bool t_cancel = false;
		CB_DownloadRequest(m_browser -> GetInst(), [strUrl cString], &t_cancel);
	}
}

- (WebView *)webView:(WebView *)sender createWebViewWithRequest:(NSURLRequest *)request
{
	if (!m_browser -> GetNewWindow())
	{
		CB_NewWindow(m_browser -> GetInst(), [[[request URL] absoluteString] cString]);
		return NULL;
	}
	
	id myDocument = [[NSDocumentController sharedDocumentController] openUntitledDocumentOfType:@"DocumentType" display:YES];
    [[[myDocument webView] mainFrame] loadRequest:request];
    return [myDocument webView];
}

- (void)webViewShow:(WebView *)sender
{
    id myDocument = [[NSDocumentController sharedDocumentController] documentForWindow:[sender window]];
    [myDocument showWindows];
}

- (void)webView:(WebView *)sender
		decidePolicyForNewWindowAction:(NSDictionary *) actionInformation
		request:(NSURLRequest *) request
		newFrameName:(NSString *) frameName
		decisionListener:(id<WebPolicyDecisionListener>) listener
{
	NSURL * urlKey = [request URL];
	NSString * strUrl = [urlKey absoluteString];
	   
	if ( m_browser -> GetNewWindow() )
		[listener use];
	else
	{
		[listener ignore];
		CB_NewWindow(m_browser -> GetInst(), [strUrl cString]);
	}
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    WebDataSource *  ldatasource;
	NSURL *		 urlkey;
	
	ldatasource = [frame dataSource];
	urlkey = [[ldatasource initialRequest] URL];
	NSString * strUrl = [urlkey absoluteString];


	if( frame == [sender mainFrame] )
		CB_DocumentComplete(m_browser -> GetInst(), [strUrl cString]);
	else
		CB_DocumentFrameComplete(m_browser -> GetInst(), [strUrl cString]);
} 

- (NSArray *)webView:(WebView *)sender contextMenuItemsForElement:(NSDictionary *)element 
	defaultMenuItems:(NSArray *)defaultMenuItems
{
	   if( m_browser -> GetContextMenu() )
		   return defaultMenuItems;
	   else
		   return nil;
}

- (void)webView:(WebView *)sender unableToImplementPolicyWithError:(NSError *)error frame:(WebFrame *)frame
{
	void *t_foo = NULL;
}


- (void)webView:(WebView *)sender mouseDidMoveOverElement:(NSDictionary *)elementInformation modifierFlags:(unsigned int)modifierFlags
{
	if (m_browser -> GetMessages())
	{
		DOMHTMLElement *t_element;
		t_element = [elementInformation objectForKey: @"WebElementDOMNode"];
		
		if ([t_element nodeType] == DOM_TEXT_NODE)
			t_element = [t_element parentNode];
			
		if (t_element != NULL && [t_element nodeType] == DOM_ELEMENT_NODE && t_element != m_previous_element)
		{
			if (m_previous_element != NULL)
			{
				NSString *t_previous_id;
				t_previous_id = [m_previous_element idName];
				if (t_previous_id != NULL && ![t_previous_id isEqualToString: @""])
					CB_ElementLeave(m_browser -> GetInst(), [t_previous_id cString]);
				
				[m_previous_element release];
				
				m_previous_element = NULL;
			}
			
			NSString *t_id;
			t_id = [t_element idName];
			if (t_id != NULL && ![t_id isEqualToString: @""])
				CB_ElementEnter(m_browser -> GetInst(), [t_id cString]);
			
			m_previous_element = [t_element retain];
		}
	}
}

@end
@implementation NSObject (WebUIDelegate)

- (void)webView:(WebView *)sender runJavaScriptAlertPanelWithMessage:(NSString *)message
{
	AlertStdCFStringAlertParamRec		param;
	DialogRef							alert;
	DialogItemIndex						itemHit;

	param.version 		= kStdCFStringAlertVersionOne;
	param.movable 		= true;
	param.helpButton 	= false;
	param.defaultText 	= (CFStringRef)kAlertDefaultOKText;
	param.cancelText 	= NULL;
	param.otherText 	= NULL;
	param.defaultButton = kAlertStdAlertOKButton;
	param.cancelButton 	= 0;
	param.position 		= kWindowDefaultPosition;
	param.flags 		= 0;
	
	CreateStandardAlert( 0, (CFStringRef)message, NULL, NULL, &alert );
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	RunStandardAlert( alert, NULL, &itemHit );
        [pool release];
}

- (BOOL)webView:(WebView *)sender runJavaScriptConfirmPanelWithMessage:(NSString *)message
{
	AlertStdCFStringAlertParamRec		param;
	DialogRef							alert;
	DialogItemIndex						itemHit;

	param.version 		= kStdCFStringAlertVersionOne;
	param.movable 		= true;
	param.helpButton 	= false;
	param.defaultText 	= (CFStringRef)kAlertDefaultOKText;
	param.cancelText 	= (CFStringRef)kAlertDefaultCancelText;
	param.otherText 	= NULL;
	param.defaultButton = kAlertStdAlertOKButton;
	param.cancelButton 	= kAlertStdAlertCancelButton;
	param.position 		= kWindowDefaultPosition;
	param.flags 		= 0;
	
	CreateStandardAlert( 0, (CFStringRef)message, NULL, &param, &alert );
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	RunStandardAlert( alert, NULL, &itemHit );
        [pool release];
	
	return (itemHit == kAlertStdAlertOKButton );
}

- (void)webView:(WebView *)sender runOpenPanelForFileButtonWithResultListener:(id<WebOpenPanelResultListener>)resultListener
{
	NavDialogCreationOptions	dialogOptions;
	NavDialogRef				dialog;
	OSStatus 					theErr = noErr;

	NavGetDefaultDialogCreationOptions( &dialogOptions );

	dialogOptions.modality = kWindowModalityAppModal;
	dialogOptions.clientName = CFStringCreateWithPascalString( NULL, LMGetCurApName(), GetApplicationTextEncoding());
	dialogOptions.optionFlags &= ~kNavAllowMultipleFiles;

	theErr = NavCreateChooseFileDialog( &dialogOptions, NULL, OpenDialogEventProc, NULL, NULL, resultListener, &dialog );
	if ( theErr == noErr )
	{
		theErr = NavDialogRun( dialog );
	}
	
	if ( theErr != noErr )
	{
		NavDialogDispose( dialog );
		[resultListener cancel];
	}
}

@end

void
OpenDialogEventProc( const NavEventCallbackMessage callbackSelector, 
									   NavCBRecPtr callbackParms, 
									   NavCallBackUserData callbackUD )
{
	id<WebOpenPanelResultListener> 	resultListener = (id<WebOpenPanelResultListener>)callbackUD;

	switch ( callbackSelector )
	{	
		case kNavCBUserAction:
			if ( callbackParms->userAction == kNavUserActionChoose )
			{
				NavReplyRecord	reply;
				OSStatus		status;
				
				status = NavDialogGetReply( callbackParms->context, &reply );
				if ( status == noErr )
				{
					OSStatus		anErr;
					AEKeyword		keywd;
					DescType		returnedType;
					Size			actualSize;
					FSRef 			fileRef;
					FSCatalogInfo	theCatInfo;
					UInt8			path[1024];
					CFStringRef		filename;
					
					anErr = AEGetNthPtr( &reply.selection, 1, typeFSRef, &keywd, &returnedType,
									(Ptr)(&fileRef), sizeof( fileRef ), &actualSize );
					require_noerr(anErr, AEGetNthPtr);
			
					anErr = FSGetCatalogInfo( &fileRef, kFSCatInfoFinderInfo, &theCatInfo, NULL, NULL, NULL );
					require_noerr(anErr, FSGetCatalogInfo);
					
					FSRefMakePath( &fileRef, path, sizeof( path ) );
					filename = CFStringCreateWithCString( NULL, (char *)path, kCFStringEncodingUTF8 );
					[resultListener chooseFilename:(NSString*)filename];
					CFRelease( filename );

				AEGetNthPtr:
				FSGetCatalogInfo:

					NavDisposeReply( &reply );
				}
			}
			else if ( callbackParms->userAction == kNavUserActionCancel )
			{
				[resultListener cancel];
			}
			break;

		case kNavCBTerminate:
			NavDialogDispose( callbackParms->context );
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

TAltBrowser::TAltBrowser()
{
	isvisible = true;
	scaleenabled = true;
	allownewwindow = false;
	scrollbarsenabled = true;
	borderenabled = true;
	contextmenus = true;
	messages = false;
	
	m_container = NULL;
	m_group = NULL;
	m_parent = NULL;
	
	m_parent_handler = NULL;
	m_container_handler = NULL;
	m_webview_handler = NULL;
	
	m_web_browser = NULL;
	m_web_adapter = NULL;
	
	m_lock_update = false;
	
	::SetRect(&m_bounds, 0, 0, 0, 0);
}

TAltBrowser::~TAltBrowser()
{
	DetachFromParent();
	
	RemoveEventHandler(m_webview_handler);
	DisposeEventHandlerUPP(m_webview_handler_upp);
	
	HideWindow(m_container);
	
	WebView *t_view;
	t_view = HIWebViewGetWebView(m_web_browser);
	
	[t_view setPolicyDelegate: nil];
	[t_view setFrameLoadDelegate: nil];
	[t_view setUIDelegate: nil];
	[m_web_adapter release];
	
	[[t_view mainFrame] stopLoading];
	HIViewRemoveFromSuperview(m_web_browser);
	
	DisposeWindow(m_container);
	
	[t_view release];
}

CWebBrowserBase::~CWebBrowserBase(void)
{
}

OSStatus TAltBrowser::ParentEventHandler(EventHandlerCallRef p_call_chain, EventRef p_event, void *p_context)
{
	if (GetEventClass(p_event) == 'revo' && GetEventKind(p_event) == 'sync')
	{
		((TAltBrowser *)p_context) -> Synchronize();
		return noErr;
	}
	
	switch(GetEventKind(p_event))
	{
		case kEventWindowBoundsChanged:
		case kEventWindowShown:
		case kEventWindowHidden:
		case kEventWindowCollapsing:
		case kEventWindowExpanded:
			((TAltBrowser *)p_context) -> Synchronize();
		break;

		case kEventWindowClosed:
		break;
	}
	
	return eventNotHandledErr;
}

static UInt32 key_to_command_id(UInt16 p_key)
{
	UInt32 t_id;
	switch(p_key)
	{	
		case 'c':
		case 'C':
			t_id = kHICommandCopy;
		break;
		
		case 'v':
		case 'V':
			t_id = kHICommandPaste;
		break;
		
		case 'x':
		case 'X':
			t_id = kHICommandCut;
		break;
		
		case 'a':
		case 'A':
			t_id = kHICommandSelectAll;
		break;
		
		case 'Z':
			t_id = kHICommandUndo;
		break;
		
		case 'z':
			t_id = kHICommandRedo;
		break;
			
		default:
			t_id = 0;
		break;
	}
	return t_id;
}

OSStatus TAltBrowser::WebViewEventHandler(EventHandlerCallRef p_call_chain, EventRef p_event, void *p_context)
{
	switch(GetEventKind(p_event))
	{
		case kEventCommandUpdateStatus:
		{
			HICommand t_command;
			GetEventParameter(p_event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &t_command);
			
			UInt16 t_key;
			GetMenuItemCommandKey(t_command . menu . menuRef, t_command . menu . menuItemIndex, FALSE, &t_key);
			if (key_to_command_id(t_key) != 0)
				EnableMenuItem(t_command . menu . menuRef, t_command . menu . menuItemIndex);
		}
		break;
		
		//MH-2007-05-21 [[Bug 4968 ]]: mousewheel activates scrollbars, even if disabled
		case kEventMouseWheelMoved:
		{
			if (! ((TAltBrowser *)p_context) -> scrollbarsenabled)
				return noErr;
		}
		break;
	
		case kEventCommandProcess:
		{
			OSStatus t_err;
			
			HICommand t_command;
			t_err = GetEventParameter(p_event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &t_command);
			
			UInt16 t_key;
			GetMenuItemCommandKey(t_command . menu . menuRef, t_command . menu . menuItemIndex, FALSE, &t_key);
			t_command . commandID = key_to_command_id(t_key);
			
			// MW-2011-01-31: [[ Bug 9359 ]] Make sure we return 'eventNotHandled' if we don't
			//   recognize the key sequence... Otherwise we end up not passing on things like Cmd-Q!
			if (t_command . commandID != 0)
				SetEventParameter(p_event, kEventParamDirectObject, typeHICommand, sizeof(HICommand), &t_command);
			else
				return eventNotHandledErr;
		}
		break;
	
		case kEventControlDraw:
			((TAltBrowser *)p_context) -> Redraw();
		break;
	}

	return eventNotHandledErr;
}

void TAltBrowser::Synchronize(void)
{
	// Do nothing if there is currently no parent.
	if (m_parent == NULL)
		return;

	Rect t_parent_bounds;
	GetWindowBounds(m_parent, kWindowContentRgn, &t_parent_bounds);

	// MW-2012-10-08: [[ Bug 10442 ] Get the window scroll so the browser is placed properly
	//   when parent stack is scrolled.
	int t_scroll;
	if (GetWindowProperty(m_parent, 'revo', 'scrl', 4, NULL, &t_scroll) != noErr)
		t_scroll = 0;
	
	HIRect t_view_bounds;
	t_view_bounds . origin . x = 0;
	t_view_bounds . origin . y = 0;
	t_view_bounds . size . width = m_bounds . right - m_bounds . left;
	t_view_bounds . size . height = m_bounds . bottom - m_bounds . top;
	
	Rect t_container_bounds;
	t_container_bounds . left = max(t_parent_bounds . left, t_parent_bounds . left + m_bounds . left);
	t_container_bounds . top = max(t_parent_bounds . top, t_parent_bounds . top + m_bounds . top) - t_scroll;
	t_container_bounds . right = min(t_parent_bounds . right, t_parent_bounds . left + m_bounds . right);
	t_container_bounds . bottom = min(t_parent_bounds . bottom, t_parent_bounds . top + m_bounds . bottom) - t_scroll;
	
	bool t_is_null;
	if (t_container_bounds . left >= t_container_bounds . right || t_container_bounds . top >= t_container_bounds . bottom)
		t_is_null = true;
	else
		t_is_null = false;
	
	if (!t_is_null)
	{
		ChangeWindowGroupAttributes(m_group,0, kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation);
		SetWindowBounds(m_container, kWindowContentRgn, &t_container_bounds);
		HIViewSetFrame(m_web_browser, &t_view_bounds);
		ChangeWindowGroupAttributes(m_group, kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation, 0);
	}

	bool t_parent_visible;
	t_parent_visible = IsWindowVisible(m_parent) && !IsWindowCollapsed(m_parent);

	if (t_parent_visible && isvisible && !t_is_null)
		ShowWindow(m_container);
	else
		HideWindow(m_container);
}

void TAltBrowser::init(unsigned int p_window)
{
	WebInitForCarbon();

	m_parent = (WindowRef)p_window;

	HIWebViewCreate(&m_web_browser);

	Rect t_content_rect;
	GetWindowBounds(m_parent, kWindowContentRgn, &t_content_rect);
	t_content_rect . right = t_content_rect . left + 32;
	t_content_rect . bottom = t_content_rect . top + 32;
	CreateNewWindow(kSheetWindowClass, kWindowStandardHandlerAttribute | kWindowCompositingAttribute | kWindowNoShadowAttribute, &t_content_rect, &m_container);
	
	HIViewRef t_content_view;
	HIViewFindByID(HIViewGetRoot(m_container), kHIViewWindowContentID, &t_content_view);
	
	HIRect t_bounds_rect;
	HIViewGetBounds(t_content_view, &t_bounds_rect);
	HIViewSetFrame(m_web_browser, &t_bounds_rect);
	
	HIViewAddSubview(t_content_view, m_web_browser);

	WebView *t_webview;
	t_webview = HIWebViewGetWebView(m_web_browser);
	
	m_web_adapter = [[WebBrowserAdapter alloc] initWithHIObject: (HIObjectRef)m_container];
	
	[m_web_adapter setBrowser: this];
	[t_webview setPolicyDelegate: m_web_adapter];
	[t_webview setFrameLoadDelegate: m_web_adapter];
	[t_webview setUIDelegate: m_web_adapter];
	
	HIViewSetVisible(m_web_browser, true);
	
	static EventTypeSpec s_webview_events[] =
	{
		{ kEventClassControl, kEventControlDraw },
		{ kEventClassCommand, kEventCommandProcess },
		{ kEventClassCommand, kEventCommandUpdateStatus },
		{ kEventClassMouse, kEventMouseWheelMoved }
	};
	
	m_webview_handler_upp = NewEventHandlerUPP(WebViewEventHandler);
	InstallEventHandler(GetControlEventTarget(m_web_browser), m_webview_handler_upp, sizeof(s_webview_events) / sizeof(EventTypeSpec), s_webview_events, this, &m_webview_handler);

	AttachToParent(m_parent);
}

void TAltBrowser::AttachToParent(WindowRef p_parent)
{
	// Make sure the parent is in a window group with us.
	m_parent = p_parent;

	WindowGroupRef t_current_group;
	t_current_group = GetWindowGroup(m_parent);
	
	m_group = NULL;
	if (t_current_group != NULL)
	{
		CFStringRef t_group_name;
		t_group_name = NULL;
		CopyWindowGroupName(t_current_group, &t_group_name);
		if (t_group_name != NULL)
		{
			if (CFStringCompare(t_group_name, CFSTR("MCCONTROLGROUP"), 0) == 0)
				m_group = t_current_group;
			CFRelease(t_group_name);
		}
	}
	
	if (m_group != NULL)
	{
		if (GetWindowGroup(m_parent) != m_group)
		{
			ChangeWindowGroupAttributes(m_group, 0, kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation);
			SetWindowGroupParent(m_group, GetWindowGroup(m_parent));
		}
		SetWindowGroup(m_container, m_group);
	}
	else
	{
		CreateWindowGroup(0, &m_group);
		SetWindowGroupName(m_group, CFSTR("MCCONTROLGROUP"));
		SetWindowGroupOwner(m_group, m_parent);
		SetWindowGroupParent(m_group, GetWindowGroup(m_parent));
		SetWindowGroup(m_parent, m_group);
		SetWindowGroup(m_container, m_group);
	}
	
	WindowGroupAttributes fwinAttributes = kWindowGroupAttrSelectAsLayer | kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation;
	ChangeWindowGroupAttributes(m_group, fwinAttributes, 0); 
	SetWindowGroupLevel(m_group, 4);

	static EventTypeSpec s_parent_events[] =
	{
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassWindow, kEventWindowShown },
		{ kEventClassWindow, kEventWindowHidden },
		{ kEventClassWindow, kEventWindowClosed },
		{ kEventClassWindow, kEventWindowExpanded },
		{ kEventClassWindow, kEventWindowCollapsing },
		{ 'revo', 'sync' },
	};

	m_parent_handler_upp = NewEventHandlerUPP(ParentEventHandler);
	InstallEventHandler(GetWindowEventTarget(m_parent), m_parent_handler_upp, sizeof(s_parent_events) / sizeof(EventTypeSpec), s_parent_events, this, &m_parent_handler);	
}

void TAltBrowser::DetachFromParent(void)
{
	RemoveEventHandler(m_parent_handler);
	DisposeEventHandlerUPP(m_parent_handler_upp);
	m_parent_handler = NULL;
	m_parent_handler_upp = NULL;

	HideWindow(m_container);
	SetWindowGroup(m_container, GetWindowGroupOfClass(kDocumentWindowClass));
	m_parent = NULL;
}



void TAltBrowser::GoURL(const char * myurl, const char *p_target_frame)
{
	WebView*            nativeView;
	NSURLRequest*       request;
	WebFrame*           mainFrame;
	NSString	*		  lurlstr = [[NSString alloc] initWithCString:myurl];
	NSURL *			  lurl = [[NSURL alloc] initWithString:lurlstr];
	
	nativeView = HIWebViewGetWebView( m_web_browser ); // get the Cocoa view
												   // Use Objective-C calls to load the actual content
	
    request = [NSURLRequest requestWithURL:lurl];

	
	if (p_target_frame != NULL)
	{
		NSString *t_target_frame;
		t_target_frame = [[NSString alloc] initWithCString:p_target_frame];
		mainFrame = [[nativeView mainFrame] findFrameNamed: t_target_frame];
		[t_target_frame release];
	}
	else
		mainFrame = [nativeView mainFrame];

	if (mainFrame == NULL)
		return;
	
	[mainFrame loadRequest:request];
	
	[lurl release];
	[lurlstr release];
}

void TAltBrowser::SetSource( const char * myhtml )
{
	WebView*		nativeView;
	WebFrame*    mainFrame;
	
	// MW-2012-09-17: [[ Bug 9658 ]] Use loadData: so WebKit infers encoding type and such from html.
	NSData *t_data;
	t_data = [[NSData alloc] initWithBytes: myhtml length: strlen(myhtml)];
	
	nativeView = HIWebViewGetWebView( m_web_browser );
	mainFrame = [nativeView mainFrame];
	
	[mainFrame loadData: t_data MIMEType: nil textEncodingName: nil baseURL: nil];
	
	[t_data release];
}

void TAltBrowser::SetVScroll(int p_vscroll_pixels)
{
	WebView *t_native_view;
	t_native_view = HIWebViewGetWebView(m_web_browser);
	
	NSView *t_document_view;
	t_document_view = [[[t_native_view mainFrame] frameView] documentView];
	
	NSPoint t_new_scroll_origin;
	t_new_scroll_origin = [t_document_view visibleRect] . origin;
	t_new_scroll_origin . y = p_vscroll_pixels * [[t_native_view window] userSpaceScaleFactor];
	
	[t_document_view scrollPoint:t_new_scroll_origin];
}

void TAltBrowser::SetHScroll(int p_hscroll_pixels)
{
	WebView *t_native_view;
	t_native_view = HIWebViewGetWebView(m_web_browser);
	
	NSView *t_document_view;
	t_document_view = [[[t_native_view mainFrame] frameView] documentView];
	
	NSPoint t_new_scroll_origin;
	t_new_scroll_origin = [t_document_view visibleRect] . origin;
	t_new_scroll_origin . x = p_hscroll_pixels * [[t_native_view window] userSpaceScaleFactor];
	
	[t_document_view scrollPoint:t_new_scroll_origin];
}

int TAltBrowser::GetVScroll(void)
{
	WebView *t_native_view;
	t_native_view = HIWebViewGetWebView(m_web_browser);
	
	NSView *t_document_view;
	t_document_view = [[[t_native_view mainFrame] frameView] documentView];

	return [[t_native_view window] userSpaceScaleFactor] * [t_document_view visibleRect] . origin . y;
}

int TAltBrowser::GetHScroll(void)
{
	WebView *t_native_view;
	t_native_view = HIWebViewGetWebView(m_web_browser);
	
	NSView *t_document_view;
	t_document_view = [[[t_native_view mainFrame] frameView] documentView];

	return [[t_native_view window] userSpaceScaleFactor] * [t_document_view visibleRect] . origin . x;
}

int TAltBrowser::GetFormattedHeight(void)
{
	WebView *t_native_view;
	t_native_view = HIWebViewGetWebView(m_web_browser);
	return (int) [[t_native_view window] userSpaceScaleFactor] * NSMaxY([[[[t_native_view mainFrame] frameView] documentView] bounds]);
}

int TAltBrowser::GetFormattedWidth(void)
{
	WebView *t_native_view;
	t_native_view = HIWebViewGetWebView(m_web_browser);
	return (int) [[t_native_view window] userSpaceScaleFactor] * NSMaxX([[[[t_native_view mainFrame] frameView] documentView] bounds]);
}

void TAltBrowser::GetFormattedRect(int& r_left, int& r_top, int& r_right, int& r_bottom)
{
	r_left = m_bounds . left;
	r_top = m_bounds . top;
	r_right = r_left + GetFormattedWidth();
	r_bottom = r_top + GetFormattedHeight();
}

char *TAltBrowser::ExecuteScript(const char *p_javascript_string)
{
		WebView *t_native_view;
		t_native_view = HIWebViewGetWebView(m_web_browser);
		
		// To be consistent with the Windows implementation, we return the value of the "result" global variable.
		// In order to do this, we simply put "result;" at the end of the string to execute, and it will be evaluated
		// as an expression and returned.
		char *t_script;
		t_script = (char *)malloc(strlen(p_javascript_string) + 7 + 1);
		sprintf(t_script, "%s\n%s", p_javascript_string, "result;");
		
		NSString *t_javascript;
		t_javascript = [[NSString alloc] initWithCString:t_script];
		
		NSString *t_execution_result;
		t_execution_result = [t_native_view stringByEvaluatingJavaScriptFromString:t_javascript];
		
		char *t_result;
		if (t_execution_result == nil)
			t_result = NULL;
		else
			t_result = (char *)[t_execution_result cStringUsingEncoding: NSMacOSRomanStringEncoding];
			
		if (t_result != NULL)
			t_result = strdup(t_result);
												
		[t_javascript release];
		free(t_script);
		
		return t_result;
}

char *TAltBrowser::CallScript(const char *p_function_name, char **p_arguments, unsigned int p_argument_count)
{
	WebView *t_native_view;
	t_native_view = HIWebViewGetWebView(m_web_browser);
	
	id t_script_object;
	t_script_object = [t_native_view windowScriptObject];

	// Allocate a C array of NSObjects, populate this with NSString conversions of the parameters
	NSString **t_arguments;
	t_arguments = (NSString **)malloc(p_argument_count * sizeof(NSObject *));

	for (unsigned int i = 0; i < p_argument_count; i++)
	{
		NSString *t_argument;
		t_argument = [[NSString alloc] initWithCString:p_arguments[i]  encoding:NSMacOSRomanStringEncoding];
		t_arguments[i] = t_argument;
	}

	// Create an NSArray from the C array
	NSArray *t_array;
	t_array = [NSArray arrayWithObjects:t_arguments count:p_argument_count];

	// Now convert the function name into an NSString
	NSString *t_method_name;
	t_method_name = [[NSString alloc] initWithCString:p_function_name encoding:NSMacOSRomanStringEncoding];
	
	// We should now be able to use the callWebScriptMethod function...
	NSString *t_execution_result;
	t_execution_result = [t_script_object callWebScriptMethod:t_method_name withArguments:t_array];
    
    // MM-2012-02-10: [[Bug 9659]] JS calls via OS X revBrowser throw errors
	if (t_execution_result != nil && ![t_execution_result isKindOfClass: [NSString class]])
		t_execution_result = [t_execution_result description];
	
	[t_method_name release];
	for (unsigned int i = 0; i < p_argument_count; i++)
		[t_arguments[i] release];

	free(t_arguments);

	const char *t_result;
	t_result = [t_execution_result cStringUsingEncoding:NSMacOSRomanStringEncoding];
	if (t_result != NULL)
		t_result = strdup(t_result);
		
	return (char *)t_result;
}

void TAltBrowser::SetMessages( bool mstate )
{
	messages = mstate;
}

bool TAltBrowser::GetMessages()
{
	return messages;
}

void TAltBrowser::SetBrowser(const char *p_browser)
{
}

char *TAltBrowser::GetBrowser(void)
{
	return strdup("Safari");
}

void TAltBrowser::SetBorder(bool p_enabled)
{
	WebView*            nativeView;
	WebFrameView *		   theframe;
	NSScrollView *	   sview;
	
	nativeView = HIWebViewGetWebView( m_web_browser );
	theframe = [[nativeView mainFrame] frameView];
	sview = (NSScrollView *)[[theframe documentView] enclosingScrollView];
	
	if (p_enabled)
		[sview setBorderType:NSGrooveBorder];
	else
		[sview setBorderType:NSNoBorder];
	
	borderenabled = p_enabled;
}

char * TAltBrowser::GetURL()
{
	WebView*            nativeView;
	NSURLRequest*       request;
	WebFrame*           mainFrame;
	WebDataSource *     datasource;
	NSURL *				  urlkey;
	
	nativeView = HIWebViewGetWebView( m_web_browser ); // get the Cocoa view
	
	mainFrame = [nativeView mainFrame];
	datasource = [mainFrame dataSource];
	
	request = [datasource initialRequest];
	
	urlkey = [request URL];
	
	// OK-2007-11-30 : Bug 5445. It is possible that a browser may receieve a GetURL request when it is in the process of loading a url
	// in this case urlkey will be nil, resulting in a crash when we try and duplicate the curl string. We just return empty for now in this case.
	if (urlkey == nil)
	  return strdup("");
	
	NSString * strUrl = [urlkey absoluteString];
	
	const char * curl;
	curl =  [strUrl UTF8String];
	
	return strdup(curl);
	
}

void TAltBrowser::GoBack(void)
{
	WebView*            nativeView;
	WebFrame*           mainFrame;
	
	nativeView = HIWebViewGetWebView( m_web_browser ); // get the Cocoa view
	
	[nativeView goBack];
}

void TAltBrowser::GoForward(void)
{
	WebView*            nativeView;
	WebFrame*           mainFrame;
	
	nativeView = HIWebViewGetWebView( m_web_browser ); // get the Cocoa view
	
	[nativeView goForward];
}

void TAltBrowser::Refresh(void)
{
	WebView*            nativeView;
	WebFrame*           mainFrame;
	
	nativeView = HIWebViewGetWebView( m_web_browser ); // get the Cocoa view
	
    mainFrame = [nativeView mainFrame];
    [mainFrame reload];
}

void TAltBrowser::Stop(void)
{
	WebView*            nativeView;
	WebFrame*           mainFrame;
	
	nativeView = HIWebViewGetWebView( m_web_browser ); // get the Cocoa view
	
    mainFrame = [nativeView mainFrame];
    [mainFrame stopLoading];
}

void TAltBrowser::SetScrollbars(bool p_enabled)
{
	WebView*            nativeView;
	WebFrameView *		   theframe;
	NSScrollView *	   sview;
	
	nativeView = HIWebViewGetWebView( m_web_browser );
	
	theframe = [[nativeView mainFrame] frameView];
	
	[theframe setAllowsScrolling: p_enabled];
	
	scrollbarsenabled = p_enabled;
}


void TAltBrowser::SetOffline(bool p_state)
{
}

bool TAltBrowser::GetOffline(void)
{
	return false;
}

void TAltBrowser::SetVisible(bool p_state)
{
	WebView*            nativeView;
	
	nativeView = HIWebViewGetWebView( m_web_browser ); // get the Cocoa view
	
	isvisible = p_state;

	Synchronize();
}

bool TAltBrowser::GetVisible()
{
	return isvisible;
}

bool TAltBrowser::GetBorder()
{
	return borderenabled;
}

bool TAltBrowser::GetBusy()
{
	WebView*            nativeView;
	WebDataSource *     datasource;
	
	nativeView = HIWebViewGetWebView( m_web_browser );
	datasource = [[nativeView mainFrame] dataSource];
	return [datasource isLoading];
	
}

bool TAltBrowser::GetScrollbars()
{
	return scrollbarsenabled;
}

bool TAltBrowser::GetNewWindow()
{
	return allownewwindow;
}

void TAltBrowser::SetNewWindow(bool newwindow)
{
	allownewwindow = newwindow;
}

bool TAltBrowser::GetScale()
{
	return scaleenabled;
}

void TAltBrowser::SetScale(bool willscale)
{
	scaleenabled = willscale;
}

bool TAltBrowser::GetContextMenu()
{
	return contextmenus;
}

void TAltBrowser::SetContextMenu(bool menustate)
{
	contextmenus = menustate;
}

// MW-2010-10-04: [[ Bug 9032 ]] Looks like a crash was occuring with previous
//   code. Rewritten to be more careful about nil pointers.
char *TAltBrowser::GetSource()
{
	WebView*            nativeview;
	WebDataSource *     datasource;
	NSString *		   websource;
	const char * curl=NULL;
	
	nativeview = HIWebViewGetWebView( m_web_browser );

    datasource = [[nativeview mainFrame] dataSource];
	
	id t_repn;
	t_repn = nil;
	if (datasource != nil)
		t_repn = [datasource representation];

	NSString *t_source;
	if (t_repn != nil)
		t_source = [t_repn documentSource];

	const char *t_utf8_source;
	t_utf8_source = "";
	if (t_source != nil)
		t_utf8_source = [t_source UTF8String];

	return strdup(t_utf8_source);
}

void TAltBrowser::Focus(void)
{
}

void TAltBrowser::Unfocus(void)
{
}

void TAltBrowser::GetRect(int& r_left, int& r_top, int& r_right, int& r_bottom)
{
	r_left = m_bounds . left;
	r_top = m_bounds . top;
	r_right = m_bounds . right;
	r_bottom = m_bounds . bottom;
	}

void TAltBrowser::SetRect(int p_left, int p_top, int p_right, int p_bottom)
{
	::SetRect(&m_bounds, p_left, p_top, p_right, p_bottom);
	Synchronize();
}

char * TAltBrowser::GetSelectedText()
{
	WebView*            nativeView;
	long				SystemMinorVersion;
	long				SystemBugFixVersion;
	
	//Domranges only showed up in 10.3.9 and later
	Gestalt( gestaltSystemVersionMinor, &SystemMinorVersion);
	Gestalt( gestaltSystemVersionBugFix, &SystemBugFixVersion);
	
	if( (SystemMinorVersion > 3) || ((SystemMinorVersion==3) && (SystemBugFixVersion>8)) )
	{
		nativeView = HIWebViewGetWebView( m_web_browser );
		DOMRange * mrange = [nativeView selectedDOMRange];
		NSString * txtStr;
		
		if( mrange != NULL )
		{
			txtStr = [mrange markupString];
			return strdup([txtStr cString]);
		}
		else
			return strdup("no selection");
	   }
	else
		return strdup("not supported in this version");
}

int TAltBrowser::GetInst()
{
	return instance_id;
}

char * TAltBrowser::GetTitle()
{
	WebView * nativeView;
	nativeView = HIWebViewGetWebView( m_web_browser );
	char * ltitle = (char *)[[[[nativeView mainFrame] dataSource] pageTitle] cString];
	return strdup(ltitle != NULL ? ltitle : "");
}

void TAltBrowser::SetInst( int linst )
{
	instance_id = linst;
}

void TAltBrowser::SetSelectedText(const char * selText )
{
	WebView*            nativeView;
	
	nativeView = HIWebViewGetWebView( m_web_browser );
	// call searchFor with an NSString and go to town
	NSString * mystring = [[NSString alloc] initWithCString:selText];
	
	[nativeView searchFor:mystring direction:YES caseSensitive:FALSE wrap:NO];
}

bool TAltBrowser::FindString(const char *p_string, bool p_search_up)
{
	WebView *t_native_view;
	
	BOOL t_search_forward;
	if (p_search_up)
		t_search_forward = NO;
	else
		t_search_forward = YES;
	
	t_native_view = HIWebViewGetWebView( m_web_browser );
	
	NSString * t_search_string;
	t_search_string = [[NSString alloc] initWithCString:p_string];
	
	if([t_native_view searchFor:t_search_string direction:t_search_forward caseSensitive:FALSE wrap:NO])
		return true;
	else
		return false;
}

void TAltBrowser::Redraw()
{
	WebView *t_native_view;
	t_native_view = HIWebViewGetWebView(m_web_browser);
	[t_native_view setNeedsDisplay: TRUE];
}

void TAltBrowser::MakeTextBigger(void)
{
    WebView*            nativeView;
		
	nativeView = HIWebViewGetWebView( m_web_browser );
	if( [nativeView canMakeTextLarger] )
		[nativeView makeTextLarger: NULL];
}

void TAltBrowser::MakeTextSmaller(void)
{
	WebView*            nativeView;
	
	nativeView = HIWebViewGetWebView( m_web_browser );
	if( [nativeView canMakeTextSmaller] )
		[nativeView makeTextSmaller: NULL];
}

void TAltBrowser::Print()
{
	WebView*            nativeView;
	WebView *		   printView;
	NSView *			  pView;
	WebFrameView *     pFrame;
	WebPreferences *   thePrefs;
	long				 SystemMinorVersion;
	
	//disconnect the group briefly
	ChangeWindowGroupAttributes(m_group,0,
								kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation);
	
	
	nativeView = ::HIWebViewGetWebView( m_web_browser );
	
	NSPrintInfo * info = [NSPrintInfo sharedPrintInfo];
    [info setHorizontalPagination: NSAutoPagination];
	[info setVerticalPagination:NSAutoPagination];
	[info setHorizontallyCentered:NO];
	[info setVerticallyCentered:NO];
	[info setOrientation:NSPortraitOrientation];
	
	
	pView = [[[nativeView mainFrame] frameView] documentView];
	printDelegate * pd = [[printDelegate alloc] init];
	
	NSPrintOperation * op = [NSPrintOperation printOperationWithView:(NSView *)pView printInfo:info];
	[NSPrintOperation setCurrentOperation:op];
	
	[op setCanSpawnSeparateThread:YES];
	Gestalt( gestaltSystemVersionMinor, &SystemMinorVersion);
	
	//in place for panel selector bugs in when used with a compsited webview -- cb
	
	if( SystemMinorVersion > 3)
		[op setShowsProgressPanel:false];
	else
	    [op setShowPanels:NO];
	
	[op runOperation];

	//reconnect the group
	ChangeWindowGroupAttributes(m_group, kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrSharedActivation, 0);
	
}

// MW-2012-11-14: [[ Bug 10509 ]] Reimplemented to fix image corruption.
bool TAltBrowser::GetImage(void*& r_data, int& r_length)
{	
	bool t_success;
	t_success = true;
	
	CGImageRef t_image;
	t_image = nil;
	if (t_success)
	{
		HIRect t_bounds;
		OSStatus t_err;
		t_err = HIViewCreateOffscreenImage(m_web_browser, 0, &t_bounds, &t_image);
		t_success = t_err == noErr;
	}
	
	size_t t_width, t_height;
	void *t_bits;
	t_bits = nil;
	if (t_success)
	{
		t_width = CGImageGetWidth(t_image);
		t_height = CGImageGetHeight(t_image);
		t_bits = malloc(t_width * t_height * 4);
		if (t_bits == nil)
			t_success = false;
	}
	
	CGColorSpaceRef t_colorspace;
	t_colorspace = nil;
	if (t_success)
	{
		t_colorspace = CGColorSpaceCreateDeviceRGB();
		if (t_colorspace == nil)
			t_success = false;
	}
	
	CGContextRef t_context;
	t_context = nil;
	if (t_success)
	{
		CGBitmapInfo t_bitmap_info;
		t_bitmap_info = kCGBitmapByteOrder32Big | kCGImageAlphaNoneSkipFirst;
		t_context = CGBitmapContextCreate(t_bits, t_width, t_height, 8, t_width * 4, t_colorspace, t_bitmap_info);
		if (t_context == nil)
			t_success = false;
	}
	
	if (t_success)
	{
		CGContextDrawImage(t_context, CGRectMake(0, 0, t_width, t_height), t_image);

		r_data = t_bits;
		r_length = t_width * t_height * 4;
	}
	else
		free(t_bits);
	
	if (t_context != nil)
		CGContextRelease(t_context);
		
	if (t_colorspace != nil)
		CGColorSpaceRelease(t_colorspace);
	
	if (t_image != nil)
		CGImageRelease(t_image);
	
	return t_success;
}

int TAltBrowser::GetWindowId(void)
{
	return (int)m_parent;
}

void TAltBrowser::SetWindowId(int p_new_id)
{
	WindowRef t_new_window;
	t_new_window = (WindowRef)p_new_id;
	if (p_new_id == 0 || !IsValidWindowPtr((WindowRef)t_new_window))
		DetachFromParent();
	else
	{
		AttachToParent(t_new_window);
		Synchronize();
	}
}

char *TAltBrowser::GetUserAgent(void)
{
	WebView *t_web_view;
	t_web_view = HIWebViewGetWebView(m_web_browser);
	
	NSString *t_ns_user_agent;
	t_ns_user_agent = [t_web_view customUserAgent];
	
	return strdup(t_ns_user_agent != nil ? [t_ns_user_agent cStringUsingEncoding: NSMacOSRomanStringEncoding] : "");
}

void TAltBrowser::SetUserAgent(const char *p_user_agent)
{
	NSString *t_ns_user_agent;
	if (strcmp(p_user_agent, "") == 0)
		t_ns_user_agent = nil;
	else
		t_ns_user_agent = [NSString stringWithCString: p_user_agent encoding: NSMacOSRomanStringEncoding];
	
	WebView *t_web_view;
	t_web_view = HIWebViewGetWebView(m_web_browser);
	[t_web_view setCustomUserAgent: t_ns_user_agent];
}

////////////////////////////////////////////////////////////////////////////////

CWebBrowserBase *InstantiateBrowser(int p_window_id)
{
	TAltBrowser *t_browser;
	t_browser = new TAltBrowser;
	t_browser -> init(p_window_id);
	return t_browser;
}

