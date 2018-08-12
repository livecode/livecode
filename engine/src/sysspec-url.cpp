/*                                                                     -*-C++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "uidc.h"
#include "param.h"
#include "object.h"
#include "globals.h"
#include "system.h"
#include "eventqueue.h"

////////////////////////////////////////////////////////////////////////////////

#if defined(FEATURE_PLATFORM_URL)

static bool is_whitespace(char p_char)
{
	return p_char == ' ' || p_char == '\n';
}

bool MCSystemStripUrl(MCStringRef p_url, MCStringRef &r_stripped)
{
	uindex_t t_start = 0;
	uindex_t t_end = MCStringGetLength(p_url);
	while (t_start < t_end && is_whitespace(MCStringGetNativeCharAtIndex(p_url, t_start)))
		t_start++;
	while (t_end > t_start && is_whitespace(MCStringGetNativeCharAtIndex(p_url, t_end - 1)))
		t_end--;
	
	return MCStringCopySubstring(p_url, MCRangeMakeMinMax(t_start, t_end), r_stripped);
}

bool MCSystemProcessUrl(MCStringRef p_url, MCSystemUrlOperation p_operations, MCStringRef &r_processed_url)
{
	bool t_success = true;
	
	MCStringRef t_processed;
    t_processed = nil;
	
	if (t_success && (p_operations & kMCSystemUrlOperationStrip))
	{
		MCAutoStringRef t_stripped;
		t_success = MCSystemStripUrl(t_processed != nil ? t_processed : p_url, &t_stripped);
		if (t_success)
		{
            if (t_processed != nil)
                MCValueRelease(t_processed);
            
			t_processed = MCValueRetain(*t_stripped);
		}
	}
	
	// if no processing, just return a copy of the input url
	if (t_success && t_processed == nil)
		t_success = MCStringCopy(p_url, t_processed);
	
	if (t_success)
		r_processed_url = t_processed;
	else
		MCValueRelease(t_processed);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

class MCUrlProgressEvent : public MCCustomEvent
{
public:
	static MCUrlProgressEvent *CreateUrlProgressEvent(MCObjectHandle object, MCStringRef url, MCSystemUrlStatus status, uint32_t amount, uint32_t total, MCStringRef error);

	void Destroy(void);
	void Dispatch(void);
	
private:
	MCStringRef m_url;
	MCObjectHandle m_object;
	MCSystemUrlStatus m_status;
	union
	{
		struct
		{
			int32_t amount;
			int32_t total;
		} m_transferred;
		MCStringRef m_error;
	};
};

MCUrlProgressEvent *MCUrlProgressEvent::CreateUrlProgressEvent(MCObjectHandle p_object, MCStringRef p_url, MCSystemUrlStatus p_status, uint32_t p_amount, uint32_t p_total, MCStringRef p_error)
{
	MCUrlProgressEvent *t_event;
	t_event = new (nothrow) MCUrlProgressEvent();
	if (t_event == nil)
		return nil;
    
	t_event->m_status = kMCSystemUrlStatusNone;
	t_event->m_error = nil;
    t_event -> m_url = MCValueRetain(p_url);
		
    t_event->m_status = p_status;
    t_event->m_object = p_object;

    if (p_status == kMCSystemUrlStatusError)
    {
        t_event -> m_error = MCValueRetain(p_error);
    }
    else
    {
        t_event->m_transferred.amount = p_amount;
        t_event->m_transferred.total = p_total;
    }
	return t_event;
}

void MCUrlProgressEvent::Destroy(void)
{
	MCValueRelease(m_url);
	if (m_status == kMCSystemUrlStatusError)
		MCValueRelease(m_error);
	delete this;
}

void MCUrlProgressEvent::Dispatch(void)
{
	if (m_object.IsValid())
	{
		switch (m_status)
		{
			case kMCSystemUrlStatusStarted:
				m_object -> message_with_valueref_args(MCM_url_progress, m_url, MCSTR("contacted"));
				break;
			case kMCSystemUrlStatusNegotiated:
				m_object -> message_with_valueref_args(MCM_url_progress, m_url, MCSTR("requested"));
				break;
			case kMCSystemUrlStatusUploading:
			{
				MCAutoStringRef t_amount, t_total;
				/* UNCHECKED */ MCStringFormat(&t_amount, "%u", m_transferred.amount);
				/* UNCHECKED */ MCStringFormat(&t_total, "%u", m_transferred.total);
				m_object -> message_with_valueref_args(MCM_url_progress, m_url, MCSTR("uploading"), *t_amount, *t_total);
			}
				break;
			case kMCSystemUrlStatusUploaded:
				m_object -> message_with_valueref_args(MCM_url_progress, m_url, MCSTR("uploaded"));
				break;
			case kMCSystemUrlStatusLoading:
			case kMCSystemUrlStatusLoadingProgress:
			{
				MCAutoStringRef t_amount, t_total;
				/* UNCHECKED */ MCStringFormat(&t_amount, "%u", m_transferred.amount);
				if (m_transferred.total != -1)
					/* UNCHECKED */ MCStringFormat(&t_total, "%u", m_transferred.total);
				else
					/* UNCHECKED */ MCStringFormat(&t_total, "%u", 0);
				m_object -> message_with_valueref_args(MCM_url_progress, m_url, MCSTR("loading"), *t_amount, *t_total);
			}
				break;
			case kMCSystemUrlStatusFinished:
				m_object -> message_with_valueref_args(MCM_url_progress, m_url, MCSTR("downloaded"));
				break;
			case kMCSystemUrlStatusError:
				m_object -> message_with_valueref_args(MCM_url_progress, m_url, MCSTR("error"), m_error);
				break;
			case kMCSystemUrlStatusNone: /* Do nothing */
				break;
		}
	}
}

static void send_url_progress(MCObjectHandle p_object, MCSystemUrlStatus p_status, MCStringRef p_url, int32_t p_amount, int32_t& x_total, const void *p_data)
{
	if (p_status == kMCSystemUrlStatusNegotiated)
        x_total = *(int32_t *)p_data;
    
	MCUrlProgressEvent *t_event;
    t_event = nil;
    
    MCStringRef t_error;
    if (p_status == kMCSystemUrlStatusError)
        t_error = (MCStringRef)p_data;
    else
        t_error = kMCEmptyString;
    
    t_event = MCUrlProgressEvent::CreateUrlProgressEvent(p_object, p_url, p_status, p_amount, x_total, t_error);
	if (t_event)
		MCEventQueuePostCustom(t_event);
}

/* ================================================================
 * MCS_geturl() and supporting functions
 * ================================================================ */

struct MCSGetUrlState
{
	MCStringRef url;
	MCSystemUrlStatus status;
	MCDataRef data;
	MCObjectHandle object;
	uindex_t loaded_size;
	index_t total_size;
    MCStringRef error;
};

// AL-2014-07-15: [[ Bug 12478 ]] Rewritten to take downloaded data as a DataRef
static bool MCS_geturl_callback(void *p_context, MCSystemUrlStatus p_status, const void *p_data)
{
	MCSGetUrlState *context = static_cast<MCSGetUrlState *>(p_context);

	/* Update request status */
	context->status = p_status;

	/* Handle the different possible interpretations of the data
	 * argument. */
	if (p_data != nil)
	{
		switch(p_status)
		{
		case kMCSystemUrlStatusError:
			/* The data is an error message */
			MCValueAssign(context->error, (MCStringRef) p_data);
			break;

		case kMCSystemUrlStatusNegotiated:
			/* The data is the total download size */
			context->total_size = *(int32_t *) p_data;
			break;

		case kMCSystemUrlStatusLoading:
			/* The data is a chunk of the URL's content.  Append it to
			 * the current download buffer */
			if (context->data == nil)
			{
				if (!MCDataCopy((MCDataRef) p_data, context->data))
				{
					return false;
				}
			}
			else if (!MCDataIsEmpty((MCDataRef) p_data))
			{
				if (!MCDataMutableCopyAndRelease(context->data, context->data))
				{
					return false;
				}

				if (!MCDataAppend(context->data, (MCDataRef) p_data))
				{
					return false;
				}

				if (!MCDataCopyAndRelease(context->data, context->data))
				{
					return false;
				}
			}

			/* Update the total amount loaded */
			context->loaded_size = MCMax(context->loaded_size,
			                             MCDataGetLength(context->data));
			break;

		case kMCSystemUrlStatusLoadingProgress:
			/* The data is the total amount loaded so far */
			context->loaded_size = MCMax(context->loaded_size,
			                             *(uint32_t *) p_data);
			break;

		default:
			/* Ignore any attached data */
			break;
		}
	}

	/* Send an message with the latest loading progress */
	MCUrlProgressEvent *t_event;
	t_event = MCUrlProgressEvent::CreateUrlProgressEvent(
	            context->object, context->url, context->status,
	            context->loaded_size, context->total_size,
	            context->error);
	if (t_event == nil)
	{
		return false;
	}

	MCEventQueuePostCustom(t_event);
	return true;

}

void MCS_geturl(MCObject *p_target, MCStringRef p_url)
{
	// Preprocess the URL by removing leading and trailing whitespace
	MCAutoStringRef t_processed_url;
	
	// IM-2013-07-30: [[ Bug 10800 ]] strip whitespace chars from url before attempting to fetch
	if (!MCSystemProcessUrl(p_url, kMCSystemUrlOperationStrip, &t_processed_url))
		return;

	// State structure used as callback context
	MCSGetUrlState t_state;
	t_state . url = *t_processed_url;
	t_state . status = kMCSystemUrlStatusNone;
	t_state . data = MCValueRetain(kMCEmptyData);
	t_state . object = p_target->GetHandle();
	t_state . loaded_size = 0;
	t_state . total_size = -1;
	t_state . error = MCValueRetain(kMCEmptyString);

	bool t_success = true;

	/* Start loading the URL */
	if (t_success)
	{
		t_success = MCSystemLoadUrl(*t_processed_url,
		                            MCS_geturl_callback, &t_state);
	}

	/* Wait until the URL has finished loading */
	if (t_success)
	{
		while (t_success &&
		       t_state.status != kMCSystemUrlStatusFinished &&
		       t_state.status != kMCSystemUrlStatusError)
		{
			/* FIXME if wait() returns non-zero, cancel the URL
			 * operation! */
			MCscreen->wait(MCmaxwait,true,true);
		}
	}

	/* Process the results */
	if (t_success)
	{
		/* Save the data */
		MCurlresult->clear();
		if (t_state.data != nil)
		{
			MCurlresult->setvalueref(t_state.data);
		}

		/* Save the error (if necessary) */
		MCresult->clear();
		if (t_state.status != kMCSystemUrlStatusFinished &&
		    t_state.error != nil)
		{
			MCresult->setvalueref(t_state.error);
		}
	}

	if (t_state.data != nil)
	{
		MCValueRelease(t_state.data);
	}

	if (t_state.error != nil)
	{
		MCValueRelease(t_state.error);
	}
}

//////////

class MCUrlLoadEvent : public MCCustomEvent
{
public:
	static MCUrlLoadEvent *CreateUrlLoadEvent(MCObjectHandle object, MCNameRef p_message, MCStringRef url, MCSystemUrlStatus status, MCDataRef data, MCStringRef error);
	
	void Destroy(void);
	void Dispatch(void);
	
private:
	MCStringRef m_url;
	MCObjectHandle m_object;
	MCSystemUrlStatus m_status;
	MCNameRef m_message;
	union
	{
		MCDataRef m_data;
		MCStringRef m_error;
	};
};

MCUrlLoadEvent *MCUrlLoadEvent::CreateUrlLoadEvent(MCObjectHandle p_object, MCNameRef p_message, MCStringRef p_url, MCSystemUrlStatus p_status, MCDataRef p_data, MCStringRef p_error)
{
	MCUrlLoadEvent *t_event;
	t_event = new (nothrow) MCUrlLoadEvent();
	if (t_event == nil)
		return nil;
	
	t_event->m_status = kMCSystemUrlStatusNone;
	t_event->m_message = nil;
	
	bool t_success;
	t_success = true;
	
	t_event->m_url = MCValueRetain(p_url);
	if (t_success)
        t_event->m_message = MCValueRetain(p_message);
    
    if (p_status == kMCSystemUrlStatusError)
        t_event -> m_error = MCValueRetain(p_error);
	
	if (t_success)
	{
		t_event->m_status = p_status;
		t_event->m_object = p_object;
		if (t_event->m_status == kMCSystemUrlStatusFinished)
			t_event->m_data = MCValueRetain(p_data);
	}
	else
	{
		MCValueRelease(t_event->m_url);
		MCValueRelease(t_event->m_message);
		MCValueRelease(t_event->m_error);
		delete t_event;
		return nil;
	}
	
	return t_event;
    
}

void MCUrlLoadEvent::Destroy(void)
{
	MCValueRelease(m_url);
	MCValueRelease(m_message);
	if (m_status == kMCSystemUrlStatusFinished)
		MCValueRelease(m_data);
	else if (m_status == kMCSystemUrlStatusError)
		MCValueRelease(m_error);
	delete this;
}

void MCUrlLoadEvent::Dispatch(void)
{
	if (m_object.IsValid())
	{
        switch (m_status)
		{
			case kMCSystemUrlStatusFinished:
            {
                MCAutoNumberRef t_num;
                /* UNCHECKED */ MCNumberCreateWithUnsignedInteger(MCDataGetLength(m_data), &t_num);
                m_object -> message_with_valueref_args(m_message, m_url, MCSTR("downloaded"), m_data, *t_num);
				break;
            }
			case kMCSystemUrlStatusError:
				m_object -> message_with_valueref_args(m_message, m_url, MCSTR("error"), m_error);
				break;
		default: /* Do nothing */
				break;
		}
	}
}

struct MCSLoadUrlState
{
	MCStringRef url;
	MCSystemUrlStatus status;
    MCDataRef data;
	MCObjectHandle object;
	int32_t total;
	MCNameRef message;
};

// AL-2014-07-15: [[ Bug 12478 ]] Rewritten to take downloaded data as a DataRef
static bool MCS_loadurl_callback(void *p_context, MCSystemUrlStatus p_status, const void *p_data)
{
	MCSLoadUrlState *context;
	context = static_cast<MCSLoadUrlState *>(p_context);
	
	context -> status = p_status;
    
	if (p_status == kMCSystemUrlStatusLoading)
        /* UNCHECKED */ MCDataAppend(context -> data, (MCDataRef)p_data);

    send_url_progress(context -> object, p_status, context -> url, MCDataGetLength(context -> data), context -> total, p_data);
	
	if (p_status == kMCSystemUrlStatusError || p_status == kMCSystemUrlStatusFinished)
	{
		MCUrlLoadEvent *t_event;
		t_event = MCUrlLoadEvent::CreateUrlLoadEvent(context->object, context->message, context -> url, p_status, context->data, (MCStringRef)p_data);
		if (t_event)
			MCEventQueuePostCustom(t_event);
	}
	return true;
}

void MCS_loadurl(MCObject *p_object, MCStringRef p_url, MCNameRef p_message)
{
	bool t_success = true;
	MCSLoadUrlState *t_state;
	MCStringRef t_processed;
	t_success = MCMemoryCreate(t_state) && MCSystemProcessUrl(p_url, kMCSystemUrlOperationStrip, t_processed);
	
	if (t_success)
	{
        t_state -> url = t_processed;
		t_state -> message = p_message;
		t_state -> status = kMCSystemUrlStatusNone;
		t_state -> object = p_object->GetHandle();
		MCDataCreateMutable(0, t_state -> data);
		
		t_success = MCSystemLoadUrl(t_processed, MCS_loadurl_callback, t_state);
	}
	
	if (t_success)
		MCresult->clear();
	else
	{
        MCValueRelease(t_state -> data);
        MCValueRelease(t_state -> url);
        MCValueRelease(t_state -> message);
		MCurlresult -> clear();
		MCresult->sets("error: load URL failed");
        MCMemoryDestroy(t_state);
	}
}

//////////

struct MCSPostUrlState
{
	MCStringRef url;
	MCSystemUrlStatus status;
	MCDataRef data;
	MCObjectHandle object;
	int32_t post_sent;
	int32_t post_length;
	int32_t total;
};

// AL-2014-07-15: [[ Bug 12478 ]] Rewritten to use value refs returned by platform url methods.
static bool MCS_posturl_callback(void *p_context, MCSystemUrlStatus p_status, const void *p_data)
{
	MCSPostUrlState *context;
	context = static_cast<MCSPostUrlState *>(p_context);
	
	context -> status = p_status;
    
	if (p_status == kMCSystemUrlStatusError)
    {
        MCAutoDataRef t_err;
        if (!MCStringEncode(static_cast<MCStringRef>(const_cast<void*>(p_data)),
                            kMCStringEncodingNative,
                            false,
                            &t_err))
            return false;
		MCValueAssign(context -> data, *t_err);
    }
	else if (p_status == kMCSystemUrlStatusLoading)
    {
		/* UNCHECKED */ MCDataAppend(context -> data, (MCDataRef)p_data);
    }

	if (p_status == kMCSystemUrlStatusUploading || p_status == kMCSystemUrlStatusUploaded)
	{
		context -> post_sent = *(uint32_t*)p_data;
		send_url_progress(context -> object, p_status, context -> url, context -> post_sent, context -> post_length, nil);
	}
	else
		send_url_progress(context -> object, p_status, context -> url, MCDataGetLength(context -> data), context -> total, context -> data);
	
	return true;
}

void MCS_posttourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url)
{
	bool t_success = true;
	
	MCAutoStringRef t_processed;
	MCObjectHandle t_obj = nil;
	MCSPostUrlState t_state;
	
	t_success = MCSystemProcessUrl(p_url, kMCSystemUrlOperationStrip, &t_processed);
	if (t_success)
    {
		t_obj = p_target->GetHandle();
        t_success = t_obj.IsValid();
    }
	
	if (t_success)
	{
		t_state . url = *t_processed;
		t_state . status = kMCSystemUrlStatusNone;
		t_state . object = t_obj;
		t_state . post_sent = 0;
		t_state . post_length = MCDataGetLength(p_data);
        MCDataCreateMutable(0, t_state . data);
        
		t_success = MCSystemPostUrl(*t_processed, p_data, MCDataGetLength(p_data), MCS_posturl_callback, &t_state);
	}
	
	if (t_success)
	{
		MCurlresult -> clear();
        MCresult -> clear();
		
		while(t_state . status != kMCSystemUrlStatusFinished && t_state . status != kMCSystemUrlStatusError)
			MCscreen -> wait(60.0, True, True);
		
		if (t_state . status == kMCSystemUrlStatusFinished)
		{
			MCresult -> clear();
            if (t_state . data != nil)
                MCurlresult -> setvalueref(t_state . data);
		}
		else
		{
			MCurlresult -> clear();
            if (t_state . data != nil)
                MCresult -> setvalueref(t_state . data);
		}
	}
	
    if (t_state . data != nil)
        MCValueRelease(t_state . data);
}

//////////

struct MCSPutUrlState
{
	MCStringRef url;
	MCSystemUrlStatus status;
	MCObjectHandle object;
	int32_t put_sent;
	int32_t put_length;
	MCStringRef error;
};

static bool MCS_puturl_callback(void *p_context, MCSystemUrlStatus p_status, const void *p_data)
{
	MCSPutUrlState *context;
	context = static_cast<MCSPutUrlState*>(p_context);
	
	context->status = p_status;
	
	if (p_status == kMCSystemUrlStatusError)
		MCValueAssign(context->error, (MCStringRef)p_data);
	
	if (p_status == kMCSystemUrlStatusUploading || p_status == kMCSystemUrlStatusUploaded)
	{
		context->put_sent = *(uint32_t*)p_data;
		send_url_progress(context->object, p_status, context->url, context->put_sent, context->put_length, nil);
	}
	else
        send_url_progress(context->object, p_status, context->url, context->put_sent, context->put_length, context->error);

	return true;
}

void MCS_putintourl(MCObject *p_target, MCDataRef p_data, MCStringRef p_url)
{
	bool t_success = true;
	
	MCAutoStringRef t_processed;
	MCObjectHandle t_obj = nil;
	MCSPutUrlState t_state;
	
	t_success = MCSystemProcessUrl(p_url, kMCSystemUrlOperationStrip, &t_processed);
	if (t_success)
    {
		t_obj = p_target->GetHandle();
        t_success = t_obj.IsValid();
    }
	
	if (t_success)
	{
        t_state.url = *t_processed;
		t_state.status = kMCSystemUrlStatusNone;
		t_state.object = t_obj;
		t_state.put_sent = 0;
		t_state.put_length = MCDataGetLength(p_data);
        
		t_success = MCSystemPutUrl(*t_processed, p_data, MCDataGetLength(p_data), MCS_puturl_callback, &t_state);
	}
	
	if (t_success)
	{
		MCurlresult->clear();
		
		while (t_state.status != kMCSystemUrlStatusUploaded && t_state.status != kMCSystemUrlStatusError)
			MCscreen->wait(60.0, True, True);
		
		if (t_state.status == kMCSystemUrlStatusUploaded)
			MCresult->clear();
		else
			MCresult->setvalueref(t_state.error);
	}
}

//////////

struct MCSDownloadUrlState
{
	MCStringRef url;
	MCSystemUrlStatus status;
	IO_handle output;
	MCObjectHandle object;
	int32_t length;
	int32_t total;
};

// AL-2014-07-15: [[ Bug 12478 ]] Rewritten to use value refs returned by platform url methods.
static bool MCS_downloadurl_callback(void *p_context, MCSystemUrlStatus p_status, const void *p_data)
{
	MCSDownloadUrlState *context;
	context = static_cast<MCSDownloadUrlState *>(p_context);
	
	context -> status = p_status;
	
	if (p_status == kMCSystemUrlStatusError)
		MCresult -> setvalueref((MCStringRef)p_data);
	else if (p_status == kMCSystemUrlStatusLoading)
	{
        MCDataRef t_data;
        t_data = (MCDataRef)p_data;
        uindex_t t_length = MCDataGetLength(t_data);
        const char *t_bytes = (const char *)MCDataGetBytePtr(t_data);
        
        context -> length += t_length;
        // PM-2014-11-03: [[ Bug 13877 ]] Make sure we write bytes
        MCS_write(t_bytes, t_length, 1, context -> output);
	}
	
	send_url_progress(context -> object, p_status, context -> url, context -> length, context -> total, p_data);
	
	return true;
}

void MCS_downloadurl(MCObjectHandle p_target, MCStringRef p_url, MCStringRef p_file)
{
	bool t_success = true;
	
	MCAutoStringRef t_processed;
	MCObjectHandle t_obj = nil;
	IO_handle t_output = nil;
	MCSDownloadUrlState t_state;
	
	t_output = MCS_open(p_file, kMCOpenFileModeWrite, False, False, 0);
	if (t_output == nil)
	{
		MCresult -> sets("can't open that file");
		return;
	}
	
	t_success = MCSystemProcessUrl(p_url, kMCSystemUrlOperationStrip, &t_processed);
	if (t_success)
    {
		t_obj = p_target->GetHandle();
        t_success = t_obj.IsValid();
    }
	
	if (t_success)
	{
		t_state . url = *t_processed;
		t_state . status = kMCSystemUrlStatusNone;
		t_state . object = t_obj;
		t_state . output = t_output;
		t_state . length = 0;
		t_state . total = 0;
		
		t_success = MCSystemLoadUrl(*t_processed, MCS_downloadurl_callback, &t_state);
	}
	
	if (t_success)
	{
		while(t_state . status != kMCSystemUrlStatusFinished && t_state . status != kMCSystemUrlStatusError)
			MCscreen -> wait(60.0, True, True);
		
		if (t_state . status == kMCSystemUrlStatusFinished)
			MCresult -> clear();
	}

	if (t_output != nil)
		MCS_close(t_output);
}

//////////

void MCS_deleteurl(MCObject *p_object, MCStringRef p_url)
{
	MCurlresult -> clear();
	MCresult -> sets("not implemented");
}

void MCS_unloadurl(MCObject *p_object, MCStringRef p_url)
{
	MCurlresult -> clear();
	MCresult -> sets("not implemented");
}

//////////

void MCS_seturlsslverification(bool p_enabled)
{
	MCSystemSetUrlSSLVerification(p_enabled);
}

#endif /* FEATURE_PLATFORM_URL */

////////////////////////////////////////////////////////////////////////////////
