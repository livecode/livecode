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

#include "prefix.h"

#include "system.h"
#include "core.h"

#include "mbliphoneapp.h"

#include <CFNetwork/CFFTPStream.h>

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////

// MW-2013-10-02: [[ MobileSSLVerify ]] When true, SSL verification is turned off.
static bool s_disable_ssl_verification = false;

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCHTTPParseHeaderCallback)(const char *p_key, uint32_t p_key_length, const char *p_value, uint32_t p_value_length, void *p_context);
bool MCHTTPParseHeaders(const char *p_headers, MCHTTPParseHeaderCallback p_callback, void *p_context)
{
	bool t_success = true;
	uint32_t t_index;
	
	const char *t_current_line = p_headers;
	const char *t_next_line;
	t_success = p_headers != nil;
	
	while (t_success && t_current_line != nil && !(t_current_line[0] == '\0' || t_current_line[0] == '\n'))
	{
		uint32_t t_line_length;
		if (MCCStringFirstIndexOf(t_current_line, '\n', t_line_length))
			t_next_line = t_current_line + t_line_length + 1;
		else
		{
			t_next_line = nil;
			t_line_length = MCCStringLength(t_current_line);
		}
		
		t_success = MCCStringFirstIndexOf(t_current_line, ':', t_index);

		if (t_success)
			t_success = t_next_line == nil || t_index < (t_next_line - t_current_line);
		if (t_success)
		{
			uint32_t t_key_length, t_value_length;
			const char *t_key, *t_value;
			
			t_key = t_current_line;
			t_key_length = t_index;
			
			t_value = t_current_line + t_index + 1;
			t_value_length = t_line_length - (t_index + 1);
			
			while (t_value[0] == ' ')
			{
				t_value ++;
				t_value_length --;
			}
			
			t_success = p_callback(t_key, t_key_length, t_value, t_value_length, p_context);
		}
		
		t_current_line = t_next_line;
	}
	
	return t_success;
}

bool UrlRequestSetHTTPHeader(const char *p_key, uint32_t p_key_length, const char *p_value, uint32_t p_value_length, void *p_context)
{
	NSMutableURLRequest *t_request;
	t_request = (NSMutableURLRequest*)p_context;
	bool t_success = true;
	
	NSString *t_key = nil;
	NSString *t_value = nil;
	
	t_key = [[NSString alloc] initWithBytes: p_key length: p_key_length encoding:NSMacOSRomanStringEncoding];
	t_value = [[NSString alloc] initWithBytes: p_value length: p_value_length encoding:NSMacOSRomanStringEncoding];
	
	t_success = (t_key != nil && t_value != nil);
	
	if (t_success)
	{
		[t_request setValue: t_value forHTTPHeaderField: t_key];
	}
	
	if (t_key != nil)
		[t_key release];
	
	if (t_value != nil)
		[t_value release];
	
	return t_success;
}

@class MCSystemUrlTimer;
@interface MCSystemUrlDelegate : NSObject
{
	MCSystemUrlCallback m_callback;
	void *m_context;
	
	NSTimer *m_timer;
	
	// This is true when the actual payload has started to come through
	bool m_uploading;
	bool m_loading;
    bool m_status_error;
	int32_t m_length;
    int32_t m_status_code;
}

- initWithCallback:(MCSystemUrlCallback)callback context: (void*)context;

- (void)setTimer: (NSTimer*)timer;
- (void)cancelTimer;

- (void)connection: (NSURLConnection *)connection didSendBodyData: (NSInteger)bytesWritten totalBytesWritten: (NSInteger)totalBytesWritten totalBytesExpectedToWrite: (NSInteger)totalBytesExpectedToWrite;
- (void)connection: (NSURLConnection *)connection didReceiveResponse: (NSURLResponse *)response;
- (void)connection: (NSURLConnection *)connection didReceiveData: (NSData *)data;
- (void)connection: (NSURLConnection *)connection didFailWithError: (NSError *)error;
- (void)connectionDidFinishLoading: (NSURLConnection *)connection;

- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace;
- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge;

@end

@implementation MCSystemUrlDelegate

- initWithCallback:(MCSystemUrlCallback)callback context: (void*)context
{
	self = [ super init ];
	if (self == nil)
		return nil;
	
	m_timer = nil;
	m_callback = callback;
	m_context = context;
	
	m_loading = false;
	m_uploading = false;
    m_status_error = false;
	
	return self;
}

- (void)setTimer: (NSTimer*)timer
{
	m_timer = timer;
}

- (void)cancelTimer
{
	if (m_timer != nil)
	{
		[m_timer invalidate];
		m_timer = nil;
	}
}

- (void)connection: (NSURLConnection *)connection didSendBodyData: (NSInteger) bytesWritten totalBytesWritten: (NSInteger)totalBytesWritten totalBytesExpectedToWrite: (NSInteger)totalBytesExpectedToWrite
{
	m_uploading = true;
	[self cancelTimer];

	int32_t t_sent = totalBytesWritten;
	m_callback(m_context, (totalBytesWritten < totalBytesExpectedToWrite) ? kMCSystemUrlStatusUploading : kMCSystemUrlStatusUploaded, &t_sent);
}

- (void)connection: (NSURLConnection *)connection didReceiveResponse: (NSURLResponse *)response
{
	[self cancelTimer];
	
	if ([response isKindOfClass: [ NSHTTPURLResponse class]])
	{
        NSHTTPURLResponse *t_http_response = (NSHTTPURLResponse*)response;
        NSInteger t_status_code = [t_http_response statusCode];
		if (t_status_code == 200)
			m_loading = true;
        else if (t_status_code >= 400)
        {
            m_loading = true;
            m_status_error = true;
            m_status_code = t_status_code;
        }
	}
	else
		m_loading = true;
	
	if (m_loading)
	{
		if ([response expectedContentLength] != NSURLResponseUnknownLength)
			m_length = (int32_t)[response expectedContentLength];
		else
			m_length = -1;
		
		m_callback(m_context, kMCSystemUrlStatusNegotiated, &m_length);
	}
}

- (void)connection: (NSURLConnection *)connection didReceiveData: (NSData *)data
{
	[self cancelTimer];
	if (!m_loading)
		return;
	
	MCString t_data;
	t_data . set((const char *)[data bytes], [data length]);
	m_callback(m_context, kMCSystemUrlStatusLoading, &t_data);
}

- (void)connection: (NSURLConnection *)connection didFailWithError: (NSError *)error
{
	[self cancelTimer];
	m_callback(m_context, kMCSystemUrlStatusError, [[error localizedDescription] cStringUsingEncoding: NSMacOSRomanStringEncoding]);
	[connection release];
}

- (void)connectionDidFinishLoading: (NSURLConnection *)connection
{
	[self cancelTimer];
    if (m_status_error)
    {
        NSString *t_err_string = [NSString stringWithFormat:@"%d %@", m_status_code, [NSHTTPURLResponse localizedStringForStatusCode:m_status_code]];
        m_callback(m_context, kMCSystemUrlStatusError, [ t_err_string cStringUsingEncoding: NSMacOSRomanStringEncoding]);
    }
    else
    {
        m_callback(m_context, kMCSystemUrlStatusFinished, nil);
    }
	[connection release];
}

// MW-2013-10-02: [[ MobileSSLVerify ]] Handle the case of server trust authentication if
//   not verifying SSL connections.
- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace
{
	// If we aren't being asked to authenticate ServerTrust, then we don't do anything.
	if (![protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust])
		return NO;
	
	// If we haven't disabled ssl verification, do nothing.
	if (!s_disable_ssl_verification)
		return NO;
	
	// Otherwise we are being asked to verify the integrity of the server, which we don't want
	// to do, so return YES to ensure we get to ignore it.
	return YES;
}

// MW-2013-10-02: [[ MobileSSLVerify ]] Handle the case of server trust authentication if
//   not verifying SSL connections.
- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
	// We should only get here if we are being asked to authenticate server trust
	// so just accept whatever it has sent.
	[challenge.sender useCredential:[NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust] forAuthenticationChallenge:challenge];
}

@end

extern real8 MCsockettimeout;
extern char *MChttpheaders;

struct load_url_t
{
	const char *url;
	MCSystemUrlCallback callback;
	void *context;
	bool success;
};

static void do_system_load_url(void *p_ctxt)
{
	load_url_t *ctxt;
	ctxt = (load_url_t *)p_ctxt;
	
	bool t_success;
	t_success = true;
	
	NSMutableURLRequest *t_request;
	NSURL *t_url;
	t_request = nil;
	if (t_success)
	{
		t_url = [NSURL URLWithString: [NSString stringWithCString: ctxt -> url encoding: NSMacOSRomanStringEncoding]];
		t_request = [NSMutableURLRequest requestWithURL: t_url
											cachePolicy: NSURLRequestUseProtocolCachePolicy
										timeoutInterval: MCsockettimeout];
		if (t_request == nil)
			t_success = false;
	}
	
	if (t_success && MChttpheaders != nil && MChttpheaders[0] != '\0')
	{
		if ([[t_url scheme] isEqualToString: @"http"] || [[t_url scheme] isEqualToString: @"https"])
			t_success = MCHTTPParseHeaders(MChttpheaders, UrlRequestSetHTTPHeader, t_request);
	}
	
	MCSystemUrlDelegate *t_delegate;
	t_delegate = nil;
	if (t_success)
	{
		t_delegate = [[MCSystemUrlDelegate alloc] initWithCallback: ctxt -> callback context: ctxt -> context];
		if (t_delegate == nil)
			t_success = false;
	}
	
	NSURLConnection *t_connection;
	t_connection = nil;
	if (t_success)
	{
		t_connection = [[NSURLConnection alloc] initWithRequest: t_request delegate: t_delegate];
		if (t_connection == nil)
			t_success = false;
	}
	
	if (t_success)
		t_success = ctxt -> callback(ctxt -> context, kMCSystemUrlStatusStarted, nil);
	
	if (!t_success)
		[t_connection release];
	
	[t_delegate release];
	
	ctxt -> success = t_success;
}

bool MCSystemLoadUrl(const char *p_url, MCSystemUrlCallback p_callback, void *p_context)
{
	load_url_t ctxt;
	ctxt . url = p_url;
	ctxt . callback = p_callback;
	ctxt . context = p_context;
	
	MCIPhoneRunOnMainFiber(do_system_load_url, &ctxt);
	
	return ctxt . success;
}

@interface PostUrlTimeoutMonitor : NSObject
{
	NSURLConnection *m_connection;
	NSTimer *m_timer;
	MCSystemUrlDelegate *m_delegate;
}
@end

@implementation PostUrlTimeoutMonitor
- (PostUrlTimeoutMonitor*) initWithTimeInterval:(NSTimeInterval)interval withConnection:(NSURLConnection*)connection withDelegate:(MCSystemUrlDelegate*)delegate
{
	self = [super init];
	if (self)
	{
		m_timer = [NSTimer scheduledTimerWithTimeInterval:interval target:self selector:@selector(timerFireMethod:) userInfo:nil repeats:NO];
		m_connection = connection;
		m_delegate = delegate;
	}
	return self;
}

- (void) timerFireMethod:(NSTimer*)theTimer
{
	NSError *t_error = nil;
	NSDictionary *t_user_info = nil;
	t_user_info = [NSDictionary dictionaryWithObject:@"timed out" forKey:NSLocalizedDescriptionKey];
	t_error = [NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorTimedOut userInfo:t_user_info];
	
	[m_delegate setTimer:nil];
	[m_delegate connection:m_connection didFailWithError: t_error];
	[m_connection cancel];
}

- (NSTimer*) timer
{
	return m_timer;
}
@end

struct post_url_t
{
	const char *url;
	const void *data;
	uint32_t length;
	MCSystemUrlCallback callback;
	void *context;
	bool success;
};

static void do_post_url(void *p_ctxt)
{
	post_url_t *ctxt;
	ctxt = (post_url_t *)p_ctxt;
	
	bool t_success;
	t_success = true;
	NSMutableURLRequest *t_request = nil;
	NSURLConnection *t_connection = nil;
	NSData *t_data = nil;
	MCSystemUrlDelegate *t_delegate = nil;
	PostUrlTimeoutMonitor *t_timeout_monitor = nil;
	
	if (t_success)
	{
		t_request = [NSMutableURLRequest
					 requestWithURL:[NSURL URLWithString: [NSString stringWithCString: ctxt -> url encoding: NSMacOSRomanStringEncoding]]
					 cachePolicy: NSURLRequestUseProtocolCachePolicy
					 timeoutInterval: MCsockettimeout];
		t_success = (t_request != nil);
	}
	
	if (t_success)
	{
		[t_request setHTTPMethod: @"POST"];
		[t_request setValue: @"application/x-www-form-urlencoded" forHTTPHeaderField: @"Content-Type"];
		t_data = [NSData dataWithBytes: ctxt -> data length: ctxt -> length];
		t_success = (t_data != nil);
		if (t_success)
			[t_request setHTTPBody: t_data];
	}
	
	if (t_success && MChttpheaders != nil && MChttpheaders[0] != '\0')
		t_success = MCHTTPParseHeaders(MChttpheaders, UrlRequestSetHTTPHeader, t_request);
	
	if (t_success)
	{
		t_delegate = [[MCSystemUrlDelegate alloc] initWithCallback: ctxt -> callback context: ctxt -> context];
		t_success = (t_delegate != nil);
	}
	
	if (t_success)
	{
		t_connection = [[NSURLConnection alloc] initWithRequest: t_request delegate: t_delegate];
		t_success = (t_connection != nil);
	}
	
	if (t_success)
	{
		t_timeout_monitor = [[PostUrlTimeoutMonitor alloc] initWithTimeInterval: MCsockettimeout withConnection:t_connection
																   withDelegate: t_delegate];
		t_success = t_timeout_monitor != nil;
	}
	if (t_success)
	{
		[t_delegate setTimer: t_timeout_monitor.timer];
		t_success = ctxt -> callback(ctxt -> context, kMCSystemUrlStatusStarted, nil);
	}
	
	if (!t_success)
	{
		if (t_connection)
			[t_connection release];
		if (t_timeout_monitor)
			[t_timeout_monitor.timer invalidate];
	}
	if (t_delegate)
		[t_delegate release];
	if (t_timeout_monitor)
		[t_timeout_monitor release];
	
	ctxt -> success = t_success;
}

bool MCSystemPostUrl(const char *p_url, const void *p_data, uint32_t p_length, MCSystemUrlCallback p_callback, void *p_context)
{
	post_url_t ctxt;
	ctxt . url = p_url;
	ctxt . data = p_data;
	ctxt . length = p_length;
	ctxt . callback = p_callback;
	ctxt . context = p_context;
	
	MCIPhoneRunOnMainFiber(do_post_url, &ctxt);
	
	return ctxt . success;
}

struct launch_url_t
{
	const char *url;
	bool success;
};

static void do_launch_url(void *p_ctxt)
{
	launch_url_t *ctxt;
	ctxt = (launch_url_t *)p_ctxt;
	
	bool t_success = true;
	
	NSURL *t_url;
	if (t_success)
	{
		t_url = [NSURL URLWithString: [NSString stringWithCString: ctxt -> url encoding: NSMacOSRomanStringEncoding]];
		t_success = (t_url != nil);
	}
	
	if (t_success)
	{
		t_success = [[UIApplication sharedApplication] canOpenURL: t_url] &&
		[[UIApplication sharedApplication] openURL: t_url];
	}
	
	ctxt -> success = t_success;
}

bool MCSystemLaunchUrl(const char *p_url)
{
	launch_url_t ctxt;
	ctxt . url = p_url;
	MCIPhoneRunOnMainFiber(do_launch_url, &ctxt);
	return ctxt . success;
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	struct
	{
		const void *bytes;
		uint32_t length;
		uint32_t sent;
	} data;
	NSTimer *timer;
	MCSystemUrlCallback callback;
	void *context;
} FTPClientCallbackData;

void PutFTPUrlClientCallback(CFWriteStreamRef p_stream, CFStreamEventType p_event, void *p_context)
{
	FTPClientCallbackData *t_context = (FTPClientCallbackData*)p_context;
	if (t_context->timer != nil)
	{
		[t_context->timer invalidate];
		t_context->timer = nil;
	}
	bool t_success = true;
	bool t_close_stream = false;
	
	switch (p_event)
	{
		case kCFStreamEventOpenCompleted:
			break;
		case kCFStreamEventCanAcceptBytes:
			int32_t t_sent;
			t_sent = CFWriteStreamWrite(p_stream, (UInt8*)t_context->data.bytes + t_context->data.sent, t_context->data.length - t_context->data.sent);
			t_success = t_sent != -1;
			if (t_success)
			{
				t_context->data.sent += t_sent;
				if (t_context->data.sent == t_context->data.length)
				{
					t_context->callback(t_context->context, kMCSystemUrlStatusUploaded, &t_context->data.sent);
					t_close_stream = true;
				}
				else
					t_context->callback(t_context->context, kMCSystemUrlStatusUploading, &t_context->data.sent);
			}
			break;
		case kCFStreamEventErrorOccurred:
			t_success = false;
			break;
	}
	
	if (!t_success)
	{
		CFErrorRef t_err = nil;
		t_err = CFWriteStreamCopyError(p_stream);
		const char *t_err_str = nil;
		if (t_err == nil) // synthetic timeout error from timer
			t_err_str = "timed out";
		else
		{
			CFStringRef t_description = CFErrorCopyDescription(t_err);
			t_err_str = [(NSString*)t_description cStringUsingEncoding: NSMacOSRomanStringEncoding];
			CFRelease(t_description);
		}
		t_context->callback(t_context->context, kMCSystemUrlStatusError, t_err_str);
		if (t_err)
			CFRelease(t_err);
		t_close_stream = true;
	}
	
	if (t_close_stream)
	{
		CFWriteStreamClose(p_stream);
		CFRelease(p_stream);
		MCMemoryDelete(t_context);
	}
}

@interface PutFTPUrlTimeoutMonitor : NSObject
{
	CFWriteStreamRef m_stream;
	NSTimer *m_timer;
	FTPClientCallbackData *m_context;
}
@end

@implementation PutFTPUrlTimeoutMonitor
- (PutFTPUrlTimeoutMonitor*) initWithTimeInterval:(NSTimeInterval)interval withStream:(CFWriteStreamRef)stream withContext:(FTPClientCallbackData*)context;
{
	self = [super init];
	if (self)
	{
		m_timer = [NSTimer scheduledTimerWithTimeInterval:interval target:self selector:@selector(timerFireMethod:) userInfo:nil repeats:NO];
		m_stream = stream;
		m_context = context;
	}
	return self;
}

- (void) timerFireMethod:(NSTimer*)theTimer
{
	m_context->timer = nil;
	PutFTPUrlClientCallback(m_stream, kCFStreamEventErrorOccurred, m_context);
}

- (NSTimer*) timer
{
	return m_timer;
}
@end

bool MCSystemPutFTPUrl(NSURL *p_url, const void *p_data, uint32_t p_length, MCSystemUrlCallback p_callback, void *p_context)
{
	bool t_success = true;
	
	CFWriteStreamRef t_ftp_stream = nil;
	FTPClientCallbackData *t_context = nil;
	PutFTPUrlTimeoutMonitor *t_monitor = nil;
	
	if (t_success)
	{
		t_ftp_stream = CFWriteStreamCreateWithFTPURL(kCFAllocatorDefault, (CFURLRef)p_url);
		t_success = t_ftp_stream != nil;
	}
	if (t_success)
	{
		t_success = MCMemoryNew(t_context);
	}
	if (t_success)
	{
		t_context->data.bytes = p_data;
		t_context->data.length = p_length;
		t_context->callback = p_callback;
		t_context->context = p_context;
		
		t_success = nil != (t_monitor = [[PutFTPUrlTimeoutMonitor alloc] initWithTimeInterval:MCsockettimeout withStream:t_ftp_stream withContext:t_context]);
	}
	if (t_success)
	{
		t_context->timer = t_monitor.timer;

		uint32_t t_client_flags = kCFStreamEventOpenCompleted | kCFStreamEventCanAcceptBytes | \
		kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered;
		
		CFStreamClientContext t_client_context;
		t_client_context.version = 0;
		t_client_context.info = t_context;
		t_client_context.retain = nil;
		t_client_context.release = nil;
		t_client_context.copyDescription = nil;
		t_success = CFWriteStreamSetClient(t_ftp_stream, t_client_flags, PutFTPUrlClientCallback, &t_client_context);
	}
	if (t_success)
		t_success = CFWriteStreamOpen(t_ftp_stream);
	if (t_success)
	{
		CFWriteStreamScheduleWithRunLoop(t_ftp_stream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
	}
	
	if (!t_success)
	{
		if (t_context)
			MCMemoryDelete(t_context);
		if (t_monitor)
			[t_monitor.timer invalidate];
		if (t_ftp_stream)
			CFRelease(t_ftp_stream);
	}
	if (t_monitor)
		[t_monitor release];
	
	return t_success;
}

struct put_url_t
{
	const char *url;
	const void *data;
	uint32_t length;
	MCSystemUrlCallback callback;
	void *context;
	bool success;
};

static void do_put_url(void *p_ctxt)
{
	put_url_t *ctxt;
	ctxt = (put_url_t *)p_ctxt;
	
	bool t_success = true;
	NSURL *t_url = nil;
	t_url = [NSURL URLWithString: [NSString stringWithCString: ctxt -> url encoding: NSMacOSRomanStringEncoding]];
	t_success = t_url != nil;
	
	if (t_success)
	{
		NSString *t_scheme;
		t_scheme = [t_url scheme];
		if ([[t_url scheme] isEqualToString: @"ftp"])
			t_success = MCSystemPutFTPUrl(t_url, ctxt -> data, ctxt -> length, ctxt -> callback, ctxt -> context);
		else
			t_success = false;
	}
	
	ctxt -> success = t_success;
}

bool MCSystemPutUrl(const char *p_url, const void *p_data, uint32_t p_length, MCSystemUrlCallback p_callback, void *p_context)
{
	put_url_t ctxt;
	ctxt . url = p_url;
	ctxt . data = p_data;
	ctxt . length = p_length;
	ctxt . callback = p_callback;
	ctxt . context = p_context;
	
	MCIPhoneRunOnMainFiber(do_put_url, &ctxt);
	
	return ctxt . success;
}

//////////

// MW-2013-10-02: [[ MobileSSLVerify ]] Enable or disable SSL verification.
void MCSystemSetUrlSSLVerification(bool p_enabled)
{
	s_disable_ssl_verification = !p_enabled;
}
