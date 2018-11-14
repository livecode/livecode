/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#import <AppKit/AppKit.h>

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

static NSRect RectToNSRect(NSWindow *p_window, Rect p_rect)
{
	CGFloat t_height;
	t_height = [[p_window contentView] frame] . size . height;
	return NSMakeRect(p_rect . left, t_height - p_rect . bottom, p_rect . right - p_rect . left, p_rect . bottom - p_rect . top);
}

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


@interface WebBrowserAdapter : NSObject
{
	TAltBrowser *m_browser;
	DOMHTMLElement *m_previous_element;
}

- (id)init;
- (void)dealloc;

- (void)setBrowser: (TAltBrowser *)inBrowser;
- (HIObjectRef)hiobject;
- (void)webView:(WebView *)sender runJavaScriptAlertPanelWithMessage:(NSString *)message;
@end

@implementation WebBrowserAdapter

- (id)init
{
    self = [super init];
	if ( self )
	{
		m_previous_element = NULL;
	}
    return self;
}

- (void)dealloc
{
	if (m_previous_element != NULL)
		[m_previous_element release];
	[super dealloc];
}

- (void)setBrowser: (TAltBrowser *)inBrowser
{
	m_browser = inBrowser;
}

- (void)webView: (WebView *)sender decidePolicyForNavigationAction:
  (NSDictionary *)actionInformation request:(NSURLRequest *)request frame:
  (WebFrame *)frame decisionListener:(id<WebPolicyDecisionListener>)listener
{
    NSString * urlKey = [[actionInformation objectForKey:WebActionOriginalURLKey] absoluteString];
	
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
    return (WebView*)[myDocument webView];
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

- (void)webView:(WebView *)sender runJavaScriptAlertPanelWithMessage:(NSString *)message
{
    // Create and display an alert panel
    NSAutoreleasePool* t_pool = [[NSAutoreleasePool alloc] init];
    NSAlert* t_alert = [[[NSAlert alloc] init] autorelease];
    [t_alert setMessageText:message];
    [t_alert runModal];
    [t_pool release];
}

- (BOOL)webView:(WebView *)sender runJavaScriptConfirmPanelWithMessage:(NSString *)message
{
    // Create and display an alert panel with OK/Cancel buttons
    NSAutoreleasePool* t_pool = [[NSAutoreleasePool alloc] init];
    NSAlert* t_alert = [[[NSAlert alloc] init] autorelease];
    NSInteger t_response;
    [t_alert setMessageText:message];
    [t_alert addButtonWithTitle:@"Cancel"];
    t_response = [t_alert runModal];
    [t_pool release];
    return (t_response == NSAlertFirstButtonReturn);
}

- (void)webView:(WebView *)sender runOpenPanelForFileButtonWithResultListener:(id<WebOpenPanelResultListener>)resultListener
{
    NSAutoreleasePool* t_pool = [[NSAutoreleasePool alloc] init];
    
    // Create an open-file panel that allows a single file to be chosen
    NSOpenPanel* t_dialog = [NSOpenPanel openPanel];
    [t_dialog setCanChooseDirectories:NO];
    [t_dialog setCanChooseFiles:YES];
    [t_dialog setAllowsMultipleSelection:NO];
    
    // Run the dialogue
    NSInteger t_result = [t_dialog runModal];
    
    // If the user didn't cancel it, get the selection
    if (t_result == NSFileHandlingPanelOKButton)
    {
        // Get the URL that was selected and convert to a file path
        NSURL* t_url = [[t_dialog URL] filePathURL];
        NSString* t_path = [t_url path];
        
        // Send it to the listener
        [resultListener chooseFilename:t_path];
    }
    else
    {
        // The dialogue was cancelled and no selection was made
        [resultListener cancel];
    }
    
    [t_pool release];
}

@end


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
	
	m_web_browser = NULL;
	m_web_adapter = NULL;
	
	m_lock_update = false;
	
    // MM-2014-07-01: SetRect no longer part of 10.8 SDK
	//::SetRect(&m_bounds, 0, 0, 0, 0);
    m_bounds . left = 0;
    m_bounds . top = 0;
    m_bounds . right = 0;
    m_bounds . bottom = 0;

}

TAltBrowser::~TAltBrowser()
{
	WebView *t_view;
	t_view = m_web_browser; //HIWebViewGetWebView(m_web_browser);
	
	[t_view setPolicyDelegate: nil];
	[t_view setFrameLoadDelegate: nil];
	[t_view setUIDelegate: nil];
	[m_web_adapter release];
	
	[[t_view mainFrame] stopLoading];
	[t_view removeFromSuperview];
	
	[t_view release];
}

@interface NativeWebView: WebView
{
	unsigned int native_id;
}

- (void)resizeWithOldSuperviewSize: (NSSize)oldsize;

- (unsigned int)com_runrev_livecode_nativeViewId;
- (void)com_runrev_livecode_setNativeViewId:(int)new_id;
@end

@implementation NativeWebView

- (void)resizeWithOldSuperviewSize: (NSSize)oldsize
{
	NSRect t_frame;
	t_frame = [self frame];
	
	NSRect t_new_frame;
	t_new_frame = t_frame;
	t_new_frame . origin . y -= oldsize . height - [[self superview] frame] . size . height;
	
	[self setFrame: t_new_frame];
}

- (unsigned int)com_runrev_livecode_nativeViewId
{
	return 0xffffffff;
}

- (void)com_runrev_livecode_setNativeViewId:(int)new_id
{
	native_id = new_id;
}

@end

void TAltBrowser::init(uintptr_t p_window)
{
	m_parent = p_window;
	
	NSWindow *t_window;
	t_window = [NSApp windowWithWindowNumber: p_window];
	if (t_window == nil)
		m_parent = 0;
	
	m_web_browser = [[NativeWebView alloc] initWithFrame: RectToNSRect(t_window, m_bounds) frameName: nil groupName: nil];
	
	[m_web_browser setAutoresizingMask: NSViewWidthSizable];
	
	m_web_adapter = [[WebBrowserAdapter alloc] init];
	
	[m_web_adapter setBrowser: this];
	[m_web_browser setPolicyDelegate: m_web_adapter];
	[m_web_browser setFrameLoadDelegate: m_web_adapter];
	[m_web_browser setUIDelegate: m_web_adapter];
	
	[m_web_browser setHidden: false];
	
	if (t_window != nil)
		[[t_window contentView] addSubview: m_web_browser];
}

void TAltBrowser::GoURL(const char * myurl, const char *p_target_frame)
{
	WebView*            nativeView;
	NSURLRequest*       request;
	WebFrame*           mainFrame;
	NSString	*		  lurlstr = [[NSString alloc] initWithCString:myurl];
	NSURL *			  lurl = [[NSURL alloc] initWithString:lurlstr];
	
	nativeView = m_web_browser; // get the Cocoa view
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
	
	nativeView = m_web_browser;
	mainFrame = [nativeView mainFrame];
	
	[mainFrame loadData: t_data MIMEType: nil textEncodingName: nil baseURL: nil];
	
	[t_data release];
}

void TAltBrowser::SetVScroll(int p_vscroll_pixels)
{
	WebView *t_native_view;
	t_native_view = m_web_browser;
	
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
	t_native_view = m_web_browser;
	
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
	t_native_view = m_web_browser;
	
	NSView *t_document_view;
	t_document_view = [[[t_native_view mainFrame] frameView] documentView];

	return [[t_native_view window] userSpaceScaleFactor] * [t_document_view visibleRect] . origin . y;
}

int TAltBrowser::GetHScroll(void)
{
	WebView *t_native_view;
	t_native_view = m_web_browser;
	
	NSView *t_document_view;
	t_document_view = [[[t_native_view mainFrame] frameView] documentView];

	return [[t_native_view window] userSpaceScaleFactor] * [t_document_view visibleRect] . origin . x;
}

int TAltBrowser::GetFormattedHeight(void)
{
	WebView *t_native_view;
	t_native_view = m_web_browser;
	return (int) [[t_native_view window] userSpaceScaleFactor] * NSMaxY([[[[t_native_view mainFrame] frameView] documentView] bounds]);
}

int TAltBrowser::GetFormattedWidth(void)
{
	WebView *t_native_view;
	t_native_view = m_web_browser;
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
		t_native_view = m_web_browser;
		
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
	t_native_view = m_web_browser;
	
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
	
	nativeView = m_web_browser;
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
	
	nativeView = m_web_browser; // get the Cocoa view
	
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
	
	nativeView = m_web_browser; // get the Cocoa view
	
	[nativeView goBack];
}

void TAltBrowser::GoForward(void)
{
	WebView*            nativeView;
	
	nativeView = m_web_browser; // get the Cocoa view
	
	[nativeView goForward];
}

void TAltBrowser::Refresh(void)
{
	WebView*            nativeView;
	WebFrame*           mainFrame;
	
	nativeView = m_web_browser; // get the Cocoa view
	
    mainFrame = [nativeView mainFrame];
    [mainFrame reload];
}

void TAltBrowser::Stop(void)
{
	WebView*            nativeView;
	WebFrame*           mainFrame;
	
	nativeView = m_web_browser; // get the Cocoa view
	
    mainFrame = [nativeView mainFrame];
    [mainFrame stopLoading];
}

void TAltBrowser::SetScrollbars(bool p_enabled)
{
	WebView*            nativeView;
	WebFrameView *		   theframe;
	
	nativeView = m_web_browser;
	
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
	
	nativeView = m_web_browser; // get the Cocoa view
	
	isvisible = p_state;

	[nativeView setHidden: !isvisible];
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
	
	nativeView = m_web_browser;
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
	
	nativeview = m_web_browser;

    datasource = [[nativeview mainFrame] dataSource];
	
	id t_repn;
	t_repn = nil;
	if (datasource != nil)
		t_repn = [datasource representation];

    // AL-2013-12-03 [[ Bug 11507 ]] Initialise t_source to nil to prevent crash.
	NSString *t_source = nil;
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
    // MM-2014-07-01: SetRect no longer part of 10.8 SDK
	//::SetRect(&m_bounds, p_left, p_top, p_right, p_bottom);
    m_bounds . left = p_left;
    m_bounds . top = p_top;
    m_bounds . right = p_right;
    m_bounds . bottom = p_bottom;

	[m_web_browser setFrame: RectToNSRect([m_web_browser window], m_bounds)];
}

char * TAltBrowser::GetSelectedText()
{
	WebView*            nativeView;
	SInt32              SystemMinorVersion;
	SInt32              SystemBugFixVersion;
	
	//Domranges only showed up in 10.3.9 and later
	Gestalt( gestaltSystemVersionMinor, &SystemMinorVersion);
	Gestalt( gestaltSystemVersionBugFix, &SystemBugFixVersion);
	
	if( (SystemMinorVersion > 3) || ((SystemMinorVersion==3) && (SystemBugFixVersion>8)) )
	{
		nativeView = m_web_browser;
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
	nativeView = m_web_browser;
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
	
	nativeView = m_web_browser;
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
	
	t_native_view = m_web_browser;
	
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
	t_native_view = m_web_browser;
	[t_native_view setNeedsDisplay: TRUE];
}

void TAltBrowser::MakeTextBigger(void)
{
    WebView*            nativeView;
		
	nativeView = m_web_browser;
	if( [nativeView canMakeTextLarger] )
		[nativeView makeTextLarger: NULL];
}

void TAltBrowser::MakeTextSmaller(void)
{
	WebView*            nativeView;
	
	nativeView = m_web_browser;
	if( [nativeView canMakeTextSmaller] )
		[nativeView makeTextSmaller: NULL];
}

void TAltBrowser::Print()
{
	WebView*            nativeView;
	SInt32				 SystemMinorVersion;
	
	
	nativeView = m_web_browser;
	
	NSPrintInfo * info = [NSPrintInfo sharedPrintInfo];
    [info setHorizontalPagination: NSAutoPagination];
	[info setVerticalPagination:NSAutoPagination];
	[info setHorizontallyCentered:NO];
	[info setVerticallyCentered:NO];
	[info setOrientation:NSPaperOrientationPortrait];
	
	
	NSPrintOperation * op = [NSPrintOperation printOperationWithView:(NSView *)nativeView printInfo:info];
	[NSPrintOperation setCurrentOperation:op];
	
	[op setCanSpawnSeparateThread:YES];
	Gestalt( gestaltSystemVersionMinor, &SystemMinorVersion);
	
	//in place for panel selector bugs in when used with a compsited webview -- cb
	
	if( SystemMinorVersion > 3)
		[op setShowsProgressPanel:false];
	else
	    [op setShowPanels:NO];
	
	[op runOperation];
}

// IM-2014-07-17: [[ Bug 12401 ]] Implement snapshot function for Cocoa-based revBrowser
extern bool MCOSXSnapshotNSView(NSView *p_view, void *&r_data, uint32_t &r_length)
{
	bool t_success;
	t_success = true;
	
	uint32_t t_width, t_height;
	
	void *t_data;
	t_data = nil;
	
	uint32_t t_length;
	t_length = 0;
	
	CGContextRef t_context;
	t_context = nil;
	
	if (t_success)
	{
		NSRect t_rect;
		t_rect = [p_view bounds];
		
		t_width = t_rect.size.width;
		t_height = t_rect.size.height;
		
		t_length = t_width * t_height * sizeof(uint32_t);
		t_success = nil != (t_data = malloc(t_length));
	}
	
	CGColorSpace *t_colorspace;
	t_colorspace = nil;
	
	if (t_success)
		t_success = nil != (t_colorspace = CGColorSpaceCreateDeviceRGB());
	
	if (t_success)
	{
		CGBitmapInfo t_bitmapinfo;
		t_bitmapinfo = kCGBitmapByteOrder32Big | kCGImageAlphaNoneSkipFirst;
		t_success = nil != (t_context = CGBitmapContextCreate(t_data, t_width, t_height, 8, t_width * sizeof(uint32_t), t_colorspace, t_bitmapinfo));
	}
	
	NSGraphicsContext *t_ns_context;
	t_ns_context = nil;
	
	if (t_success)
		t_success = nil != (t_ns_context = [NSGraphicsContext graphicsContextWithGraphicsPort:t_context flipped:false]);
	
	if (t_success)
	{
		[p_view displayRectIgnoringOpacity:[p_view bounds] inContext:t_ns_context];
		
		[t_ns_context flushGraphics];
		CGContextFlush(t_context);
	}
	
	if (t_context != nil)
		CGContextRelease(t_context);
	
	if (t_success)
	{
		r_data = t_data;
		r_length = t_length;
	}
	else
		free(t_data);
	
	return t_success;
}

// MW-2012-11-14: [[ Bug 10509 ]] Reimplemented to fix image corruption.
bool TAltBrowser::GetImage(void*& r_data, int& r_length)
{	
	bool t_success;
	t_success = true;
	
	void *t_data;
	t_data = nil;
	
	uint32_t t_length;
	t_length = 0;
	
#ifdef PRE_COCOA
	CGImageRef t_image;
	t_image = nil;
	if (t_success)
	{
		HIRect t_bounds;
		OSStatus t_err;
		t_err = HIViewCreateOffscreenImage(m_web_browser, 0, &t_bounds, &t_image);
		t_success = t_err == noErr;
	}
	
	if (t_success)
	{
		t_width = CGImageGetWidth(t_image);
		t_height = CGImageGetHeight(t_image);
		t_length = t_width * t_height * sizeof(uint32_t);
		t_bits = malloc(t_length);
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
	}
	
	if (t_context != nil)
		CGContextRelease(t_context);
		
	if (t_colorspace != nil)
		CGColorSpaceRelease(t_colorspace);
	
	if (t_image != nil)
		CGImageRelease(t_image);
#else
	t_success = MCOSXSnapshotNSView(m_web_browser, t_data, t_length);
#endif
	
	if (t_success)
	{
		r_data = t_data;
		r_length = t_length;
	}
	
	return t_success;
}

uintptr_t TAltBrowser::GetWindowId(void)
{
	return uintptr_t(m_parent);
}

void TAltBrowser::SetWindowId(uintptr_t p_new_id)
{
	NSWindow *t_window;
	t_window = [NSApp windowWithWindowNumber: p_new_id];
	if (p_new_id == 0 || t_window == nil)
	{
		[m_web_browser removeFromSuperview];
		m_parent = 0;
	}
	else
	{
		[m_web_browser removeFromSuperview];
		[[t_window contentView] addSubview: m_web_browser];
		[m_web_browser setFrame: RectToNSRect(t_window, m_bounds)];
	}
}

char *TAltBrowser::GetUserAgent(void)
{
	WebView *t_web_view;
	t_web_view = m_web_browser;
	
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
	t_web_view = m_web_browser;
	[t_web_view setCustomUserAgent: t_ns_user_agent];
}

void TAltBrowser::AddJavaScriptHandler(const char *p_handler)
{
}

void TAltBrowser::RemoveJavaScriptHandler(const char *p_handler)
{
}

////////////////////////////////////////////////////////////////////////////////

CWebBrowserBase *InstantiateBrowser(int p_window_id)
{
	TAltBrowser *t_browser;
	t_browser = new TAltBrowser;
	t_browser -> init(p_window_id);
	return t_browser;
}

////////////////////////////////////////////////////////////////////////////////

// IM-2016-03-10: [[ RemoveCefOSX ]] CEF browser no longer supported on OSX
CWebBrowserBase *MCCefBrowserInstantiate(int p_window_id)
{
	return nil;
}

void MCCefFinalise(void)
{
}

////////////////////////////////////////////////////////////////////////////////
