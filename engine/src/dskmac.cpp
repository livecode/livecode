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

#include "osxprefix.h"
#include "osxprefix-legacy.h"

#include "parsedef.h"
#include "filedefs.h"
#include "globdefs.h"
#include "objdefs.h"

#include "execpt.h"
#include "exec.h"
#include "globals.h"
#include "system.h"
#include "osspec.h"
#include "mcerror.h"
#include "util.h"
#include "mcio.h"
#include "stack.h"
#include "handler.h"
#include "dispatch.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "param.h"
#include "mode.h"
#include "securemode.h"
#include "text.h"
#include "socket.h"

#include "osxdc.h"

#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "foundation.h"

#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>


#define ENTRIES_CHUNK 1024

#define SERIAL_PORT_BUFFER_SIZE  16384 //set new buffer size for serial input port
#include <termios.h>
#define B16600 16600

#include <pwd.h>

#define USE_FSCATALOGINFO

///////////////////////////////////////////////////////////////////////////////

#define keyReplyErr 'errn'
#define keyMCScript 'mcsc'  //reply from apple event

#define AETIMEOUT                60.0

uint1 *MClowercasingtable = NULL;
uint1 *MCuppercasingtable = NULL;

inline FourCharCode FourCharCodeFromString(const char *p_string)
{
	return MCSwapInt32HostToNetwork(*(FourCharCode *)p_string);
}

inline char *FourCharCodeToString(FourCharCode p_code)
{
	char *t_result;
	t_result = new char[5];
	*(FourCharCode *)t_result = MCSwapInt32NetworkToHost(p_code);
	t_result[4] = '\0';
	return t_result;
}

struct triplets
{
	AEEventClass		theEventClass;
	AEEventID		theEventID;
	AEEventHandlerProcPtr	theHandler;
	AEEventHandlerUPP	theUPP;
};

typedef struct triplets triplets;

typedef struct
{
	char compname[255];
	OSType compsubtype;
	ComponentInstance compinstance;
}
OSAcomponent;

static OSAcomponent *osacomponents = NULL;
static uint2 osancomponents = 0;

//required apple event handler
static pascal OSErr DoOpenApp(const AppleEvent *mess, AppleEvent *rep, long refcon);
static pascal OSErr DoOpenDoc(const AppleEvent *mess, AppleEvent *rep, long refcon);
static pascal OSErr DoPrintDoc(const AppleEvent *m, AppleEvent *rep, long refcon);
static pascal OSErr DoQuitApp(const AppleEvent *mess, AppleEvent *rep, long refcon);
static pascal OSErr DoAppDied(const AppleEvent *mess, AppleEvent *rep, long refcon);
static pascal OSErr DoAEAnswer(const AppleEvent *m, AppleEvent *rep, long refcon);
static pascal OSErr DoAppPreferences(const AppleEvent *m, AppleEvent *rep, long refcon);

#define kAEReopenApplication FOUR_CHAR_CODE('rapp')
static triplets ourkeys[] =
{
	/* The following are the four required AppleEvents. */
	{ kCoreEventClass, kAEReopenApplication, DoOpenApp,  nil },
    { kCoreEventClass, kAEOpenApplication, DoOpenApp,  nil },
    { kCoreEventClass, kAEOpenDocuments, DoOpenDoc,  nil },
    { kCoreEventClass, kAEPrintDocuments, DoPrintDoc, nil },
    { kCoreEventClass, kAEQuitApplication, DoQuitApp,  nil },
    { kCoreEventClass, kAEApplicationDied, DoAppDied,  nil },
    
    { kCoreEventClass, kAEShowPreferences, DoAppPreferences,  nil },
    
    /*if MCS_send() command requires an reply(answer) back from the
     *server app the following handler is used to process the
     *reply(answer) descirption record */
    { kCoreEventClass, kAEAnswer, DoAEAnswer, nil}
};

//static Boolean hasPPCToolbox = False;
//static Boolean hasAppleEvents = False;

#define MINIMUM_FAKE_PID (1 << 29)

static int4 curpid = MINIMUM_FAKE_PID;
static char *replymessage;       //used in DoSpecial() & other routines
static uint4 replylength;
static AEKeyword replykeyword;   // Use in DoSpecial & other routines
static char *AEanswerData;// used by DoAEAnswer() & MCS_send()
static char *AEanswerErr; //the reply error from an AE send by MC.
static const AppleEvent *aePtr; //current apple event for mcs_request_ae()

/***************************************************************************
 * utility functions used by this module only		                   *
 ***************************************************************************/

static OSErr getDescFromAddress(MCStringRef address, AEDesc *retDesc);
static OSErr getDesc(short locKind, StringPtr zone, StringPtr machine, StringPtr app, AEDesc *retDesc);
static OSErr getAEAttributes(const AppleEvent *ae, AEKeyword key, char *&result);
static OSErr getAEParams(const AppleEvent *ae, AEKeyword key, char *&result);
static OSErr getAddressFromDesc(AEAddressDesc targetDesc, char *address);

static void getosacomponents();
static OSErr osacompile(MCStringRef s, ComponentInstance compinstance, OSAID &id);
static OSErr osaexecute(MCStringRef& r_string,ComponentInstance compinstance, OSAID id);

static bool fetch_ae_as_fsref_list(char*& string, uint4& length);

/***************************************************************************/

///////////////////////////////////////////////////////////////////////////////

// MOVE TO MCScreenDC

EventHandlerUPP MCS_weh;

bool WindowIsInControlGroup(WindowRef p_window)
{
	WindowGroupRef t_current_group;
	t_current_group = GetWindowGroup(p_window);
	if (t_current_group != NULL)
	{
		CFStringRef t_group_name;
		t_group_name = NULL;
		CopyWindowGroupName(t_current_group, &t_group_name);
		if (t_group_name != NULL)
		{
			if (CFStringCompare(t_group_name, CFSTR("MCCONTROLGROUP"), 0) != 0)
				t_current_group = NULL;
			CFRelease(t_group_name);
		}
	}
    
	return t_current_group != NULL;
}

static pascal OSStatus WinEvtHndlr(EventHandlerCallRef ehcf, EventRef event, void *userData)
{
    // MW-2005-09-06: userData is now the window handle, so we search for the stack
	//   the previous method of passing the stack caused problems with takewindow
	_Drawable t_window;
	t_window . handle . window = (MCSysWindowHandle)userData;
	MCStack *sptr = MCdispatcher -> findstackd(&t_window);
	
	if (GetEventKind(event) == kEventMouseWheelMoved)
	{
		if (MCmousestackptr != NULL)
		{
			MCObject *mfocused = MCmousestackptr->getcard()->getmfocused();
			if (mfocused == NULL)
				mfocused = MCmousestackptr -> getcard();
			if (mfocused != NULL)
			{
				uint2 t_axis;
				GetEventParameter(event, kEventParamMouseWheelAxis, typeMouseWheelAxis, NULL, sizeof(t_axis), NULL, &t_axis);
				if (t_axis ==  kEventMouseWheelAxisY)
				{
					int4 val;
					GetEventParameter(event, kEventParamMouseWheelDelta, typeLongInteger, NULL, sizeof(val), NULL, &val);
					if (val < 0)
						mfocused->kdown("", XK_WheelUp);
					else
						mfocused->kdown("", XK_WheelDown);
				}
				else if (t_axis ==  kEventMouseWheelAxisX)
				{
					int4 val;
					GetEventParameter(event, kEventParamMouseWheelDelta, typeLongInteger, NULL, sizeof(val), NULL, &val);
					if (val < 0)
						mfocused->kdown("", XK_WheelLeft);
					else
						mfocused->kdown("", XK_WheelRight);
				}
			}
		}
	}
	else   if (GetEventClass(event) == kEventClassWindow)
	{
		if (GetEventKind(event) == kEventWindowConstrain && sptr != NULL)
		{
			if (sptr == MCdispatcher -> gethome())
			{
				const MCDisplay *t_monitors = NULL;
				MCDisplay *t_old_monitors = NULL;
				
				uint4 t_monitor_count, t_old_monitor_count;
				bool t_changed;
				t_old_monitor_count = ((MCScreenDC *)MCscreen) -> getdisplays(t_monitors, false);
				t_old_monitors = new MCDisplay[t_old_monitor_count];
				if (t_old_monitors != NULL)
				{
					memcpy(t_old_monitors, t_monitors, sizeof(MCDisplay) * t_old_monitor_count);
					((MCScreenDC *)MCscreen) -> s_monitor_count = 0;
					delete[] ((MCScreenDC *)MCscreen) -> s_monitor_displays;
					((MCScreenDC *)MCscreen) -> s_monitor_displays = NULL;
					t_monitor_count = ((MCScreenDC *)MCscreen) -> getdisplays(t_monitors, false);
					t_changed = t_monitor_count != t_old_monitor_count || memcmp(t_old_monitors, t_monitors, sizeof(MCDisplay) * t_monitor_count) != 0;
					delete t_old_monitors;
				}
				else
					t_changed = true;
				if (t_changed)
					MCscreen -> delaymessage(MCdefaultstackptr -> getcurcard(), MCM_desktop_changed);
			}
		}
		else if (GetEventKind(event) == kEventWindowCollapsed && sptr != NULL)
			sptr->iconify();
		else if (GetEventKind(event) == kEventWindowExpanded && sptr != NULL)
			sptr->uniconify();
		else if (GetEventKind(event) == kEventWindowBoundsChanged && sptr != NULL)
		{
			UInt32 attributes;
			GetEventParameter( event, kEventParamAttributes, typeUInt32, NULL, sizeof( UInt32) , NULL, &attributes);
			
			Rect t_rect;
			GetWindowPortBounds((WindowPtr)t_window . handle . window, &t_rect);
			
			t_rect . right -= t_rect . left;
			t_rect . bottom -= t_rect . top;
			t_rect . left = 0;
			
			// MW-2011-09-12: [[ MacScroll ]] Make sure the top of the HIView takes into
			//   account the scroll.
			t_rect . top = -sptr -> getscroll();
			
			ControlRef t_root_control;
			GetRootControl((WindowPtr)t_window . handle . window, &t_root_control);
			
			ControlRef t_subcontrol;
			if (GetIndexedSubControl(t_root_control, 1, &t_subcontrol) == noErr)
				SetControlBounds(t_subcontrol, &t_rect);
			
			// MW-2007-08-29: [[ Bug 4846 ]] Ensure a moveStack message is sent whenever the window moves
			if ((attributes & kWindowBoundsChangeSizeChanged) != 0 || ((attributes & kWindowBoundsChangeUserDrag) != 0 && (attributes & kWindowBoundsChangeOriginChanged) != 0))
				sptr->configure(True);//causes a redraw and recalculation
		}
		else if (GetEventKind(event) == kEventWindowInit && sptr != NULL)
		{
			UInt32 t_value;
			t_value = 0;
			SetEventParameter(event, kEventParamWindowFeatures, typeUInt32, 4, &t_value);
		}
		else if (GetEventKind(event) == kEventWindowDrawContent)
		{
			EventRecord t_record;
			t_record . message = (UInt32)userData;
			((MCScreenDC *)MCscreen) -> doredraw(t_record, true);
			return noErr;
		}
		else if (GetEventKind(event) == kEventWindowUpdate)
		{
			EventRecord t_record;
			t_record . message = (UInt32)userData;
			((MCScreenDC *)MCscreen) -> doredraw(t_record);
			return noErr;
		}
		else if (GetEventKind(event) == kEventWindowFocusAcquired)
		{
			// OK-2009-02-17: [[Bug 3576]]
			// OK-2009-03-12: Fixed crash where getwindow() could return null.
			// OK-2009-03-23: Reverted as was causing at least two bugs.
			// if (sptr != NULL && sptr -> getwindow() != NULL)
			//	((MCScreenDC *)MCscreen) -> activatewindow(sptr -> getwindow());
		}
		else if (GetEventKind(event) == kEventWindowFocusRelinquish)
		{
			WindowRef t_window_handle;
			t_window_handle = (WindowRef)userData;
			
			if (sptr != NULL)
				sptr -> getcurcard() -> kunfocus();
		}
		else if (GetEventKind(event) == kEventWindowActivated)
		{
			if ( sptr != NULL )
				if ( sptr -> is_fullscreen() )
					if (!((MCScreenDC*)MCscreen)->getmenubarhidden())
						SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
		}
		else if (GetEventKind(event) == kEventWindowDeactivated)
		{
			if ( sptr != NULL)
				if ( sptr -> is_fullscreen() )
					if (!((MCScreenDC*)MCscreen)->getmenubarhidden())
						SetSystemUIMode(kUIModeNormal, NULL);
		}
		else if (GetEventKind(event) == kEventWindowClose)
		{
			// MW-2008-02-28: [[ Bug 5934 ]] It seems that composited windows don't send WNE type
			//   events when their close button is clicked in the background, therefore we intercept
			//   the high-level carbon event.
			MCdispatcher->wclose(&t_window);
			return noErr;
		}
		else if (GetEventKind(event) == kEventWindowGetRegion)
		{
		}
	}
	return eventNotHandledErr;
}

static pascal OSErr DoSpecial(const AppleEvent *ae, AppleEvent *reply, long refCon)
{
	if (!MCSecureModeCheckAppleScript())
		return errAEEventNotHandled;
    
	OSErr err = errAEEventNotHandled;  //class, id, sender
	DescType rType;
	Size rSize;
	AEEventClass aeclass;
	AEGetAttributePtr(ae, keyEventClassAttr, typeType, &rType, &aeclass, sizeof(AEEventClass), &rSize);
    
	AEEventID aeid;
	AEGetAttributePtr(ae, keyEventIDAttr, typeType, &rType, &aeid, sizeof(AEEventID), &rSize);
    
	if (aeclass == kTextServiceClass)
	{
		err = errAEEventNotHandled;
		return err;
	}
	//trap for the AEAnswer event, let DoAEAnswer() to handle this event
	if (aeclass == kCoreEventClass)
	{
		if (aeid == kAEAnswer)
			return errAEEventNotHandled;
	}
	AEAddressDesc senderDesc;
	//
	char *p3val = new char[128];
	//char *p3val = new char[kNBPEntityBufferSize + 1]; //sender's address 105 + 1
	if (AEGetAttributeDesc(ae, keyOriginalAddressAttr,
	                       typeWildCard, &senderDesc) == noErr)
	{
		getAddressFromDesc(senderDesc, p3val);
		AEDisposeDesc(&senderDesc);
	}
	else
		p3val = '\0';
    
	aePtr = ae; //saving the current AE pointer for use in mcs_request_ae()
	MCParameter p1, p2, p3;
	MCString s;
	
	char *t_temp;
	
	char t_aeclass_string[4];
	t_temp = FourCharCodeToString(aeclass);
	memcpy(t_aeclass_string, t_temp, 4);
	s.set(t_aeclass_string, 4);
	delete t_temp;
	
	p1.sets_argument(s);
	p1.setnext(&p2);
	
	char t_aeid_string[4];
	t_temp = FourCharCodeToString(aeid);
	memcpy(t_aeid_string, t_temp, 4);
	s.set(t_aeid_string, 4);
	delete t_temp;
	
	p2.sets_argument(s);
	p2.setnext(&p3);
	p3.sets_argument(p3val);
	/*for "appleEvent class, id, sender" message to inform script that
     there is an AE arrived */
	Exec_stat stat = MCdefaultstackptr->getcard()->message(MCM_apple_event, &p1);
	if (stat != ES_PASS && stat != ES_NOT_HANDLED)
	{ //if AE is handled by MC
		if (stat == ES_ERROR)
		{ //error in handling AE in MC
			err = errAECorruptData;
			if (reply->dataHandle != NULL)
			{
				short e = err;
				AEPutParamPtr(reply, keyReplyErr, typeShortInteger, (Ptr)&e, sizeof(short));
			}
		}
		else
		{ //ES_NORMAL
			if (replymessage == NULL) //no reply, will return no error code
				err = noErr;
			else
			{
				if (reply->descriptorType != typeNull && reply->dataHandle != NULL)
				{
					err = AEPutParamPtr(reply, replykeyword, typeChar, replymessage, replylength);
					if (err != noErr)
					{
						short e = err;
						AEPutParamPtr(reply, keyReplyErr, typeShortInteger, (Ptr)&e, sizeof(short));
					}
				}
			}
		}
		delete replymessage;
		replymessage = NULL;
	}
	else
		if (aeclass == kAEMiscStandards
            && (aeid == kAEDoScript || aeid == 'eval'))
		{
			DescType rType;
			Size rSize;  //actual size returned
			if ((err = AEGetParamPtr(aePtr, keyDirectObject, typeChar, &rType, NULL, 0, &rSize)) == noErr)
			{
				char *sptr = new char[rSize + 1];
				sptr[rSize] = '\0';
				AEGetParamPtr(aePtr, keyDirectObject, typeChar, &rType, sptr, rSize, &rSize);
				MCExecPoint ep(MCdefaultstackptr->getcard(), NULL, NULL);
                MCAutoStringRef t_sptr;
                /* UNCHECKED */ MCStringCreateWithCString(sptr, &t_sptr);
				if (aeid == kAEDoScript)
				{
					MCdefaultstackptr->getcard()->domess(*t_sptr);
					MCresult->eval(ep);
					AEPutParamPtr(reply, '----', typeChar, ep.getsvalue().getstring(), ep.getsvalue().getlength());
				}
				else
				{
					MCExecContext ctxt(ep);
					MCAutoValueRef t_val;
					MCAutoStringRef t_string;
					MCdefaultstackptr->getcard()->eval(ctxt, *t_sptr, &t_val);
					/* UNCHECKED */ ctxt.ConvertToString(*t_val, &t_string);
					AEPutParamPtr(reply, '----', typeChar, MCStringGetNativeCharPtr(*t_string), MCStringGetLength(*t_string));
				}
				delete sptr;
			}
		}
		else
			err = errAEEventNotHandled;
	// do nothing if the AE is not handled,
	// let the standard AE dispacher to dispatch this AE
	delete p3val;
	return err;
}

//Apple event handlers table is installed in MCS_init() macspec.cpp file
static pascal OSErr DoOpenApp(const AppleEvent *theAppleEvent, AppleEvent *reply, long refCon)
{
	return errAEEventNotHandled;
}

static pascal OSErr DoOpenDoc(const AppleEvent *theAppleEvent, AppleEvent *reply, long refCon)
{ //Apple Event for opening documnets, in our use is to open stacks when user
	//double clicked on a MC stack icon
	if (!MCSecureModeCheckAppleScript())
		return errAEEventNotHandled;
	
	AEDescList docList; //get a list of alias records for the documents
	errno = AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &docList);
	if (errno != noErr)
		return errno;
	long count;
	//get the number of docs descriptors in the list
	AECountItems(&docList, &count);
	if (count < 1)     //if there is no doc to be opened
		return errno;
	AEKeyword rKeyword; //returned keyword
	DescType rType;    //returned type
	
	FSRef t_doc_fsref;
	
	Size rSize;        //returned size, atual size of the docName
	long item;
	// get a FSSpec record, starts from count==1
	for (item = 1; item <= count; item++)
	{
		errno = AEGetNthPtr(&docList, item, typeFSRef, &rKeyword, &rType, &t_doc_fsref, sizeof(FSRef), &rSize);
		if (errno != noErr)
			return errno;
		// extract FSSpec record's info & form a file name for MC to use
        MCAutoStringRef t_full_path_name;
		MCS_mac_fsref_to_path(t_doc_fsref, &t_full_path_name);
        
		if (MCModeShouldQueueOpeningStacks())
		{
			MCU_realloc((char **)&MCstacknames, MCnstacks, MCnstacks + 1, sizeof(MCStringRef));
			MCValueAssign(MCstacknames[MCnstacks++], *t_full_path_name);
		}
		else
		{
			MCStack *stkptr;  //stack pointer
			if (MCdispatcher->loadfile(*t_full_path_name, stkptr) == IO_NORMAL)
				stkptr->open();
		}
	}
	AEDisposeDesc(&docList);
	return noErr;
}

static pascal OSErr DoPrintDoc(const AppleEvent *theAppleEvent, AppleEvent *reply, long refCon)
{
	if (!MCSecureModeCheckAppleScript())
		return errAEEventNotHandled;
    
	errno = errAEEventNotHandled;
	if (reply != NULL)
	{
		short e = errno;
		AEPutParamPtr(reply, keyReplyErr, typeShortInteger, &e, sizeof(short));
	}
	return errno;
}

static pascal OSErr DoQuitApp(const AppleEvent *theAppleEvent, AppleEvent *reply, long refCon)
{
	if (!MCSecureModeCheckAppleScript())
		return errAEEventNotHandled;
    
	errno = errAEEventNotHandled;
	switch (MCdefaultstackptr->getcard()->message(MCM_shut_down_request))
	{
        case ES_PASS:
        case ES_NOT_HANDLED:
            MCdefaultstackptr->getcard()->message(MCM_shut_down);
            MCquit = True; //set MC quit flag, to invoke quitting
            errno = noErr;
            break;
        default:
            errno = userCanceledErr;
            break;
	}
	if (reply != NULL)
	{
		short e = errno;
		AEPutParamPtr(reply, keyReplyErr, typeShortInteger, &e, sizeof(short));
	}
	return errno;
}

static pascal OSErr DoAppPreferences(const AppleEvent *theAppleEvent, AppleEvent *reply, long refCon)
{
	if (!MCSecureModeCheckAppleScript())
		return errAEEventNotHandled;
    
	MCGroup *mb = MCmenubar != NULL ? MCmenubar : MCdefaultmenubar;
	if (mb == NULL)
		return errAEEventNotHandled;
	MCButton *bptr = (MCButton *)mb->findname(CT_MENU, "Edit");
	if (bptr == NULL)
		return errAEEventNotHandled;
	if (bptr != NULL)
	{
		bptr->message_with_args(MCM_menu_pick, "Preferences");
	}
	return noErr;
}


static pascal OSErr DoAppDied(const AppleEvent *theAppleEvent, AppleEvent *reply, long refCon)
{
	errno = errAEEventNotHandled;
	DescType rType;
	Size rSize;
	ProcessSerialNumber sn;
	AEGetParamPtr(theAppleEvent, keyProcessSerialNumber, typeProcessSerialNumber, &rType,	&sn, sizeof(ProcessSerialNumber), &rSize);
	uint2 i;
	for (i = 0 ; i < MCnprocesses ; i++)
	{
		Boolean result;
		SameProcess((ProcessSerialNumber *)&MCprocesses[i].sn, &sn, &result);
		if (result)
		{
			MCprocesses[i].pid = 0;
			IO_cleanprocesses();
			return noErr;
		}
	}
	return errno;
}

static pascal OSErr DoAEAnswer(const AppleEvent *ae, AppleEvent *reply, long refCon)
{
	if (!MCSecureModeCheckAppleScript())
		return errAEEventNotHandled;
    
    //process the repy(answer) returned from a server app. When MCS_send() with
	// a reply, the reply is handled in this routine.
	// This is different from MCS_reply()
	//check if there is an error code
	DescType rType; //returned type
	Size rSize;
    
	/*If the handler returns a result code other than noErr, and if the
     client is waiting for a reply, it is returned in the keyErrorNumber
     parameter of the reply Apple event. */
	if (AEGetParamPtr(ae, keyErrorString, typeChar, &rType, NULL, 0, &rSize) == noErr)
	{
		AEanswerErr = new char[rSize + 1];
		AEGetParamPtr(ae, keyErrorString, typeChar, &rType, AEanswerErr, rSize, &rSize);
		AEanswerErr[rSize] = '\0';
	}
	else
	{
		short e;
		if (AEGetParamPtr(ae, keyErrorNumber, typeSMInt, &rType, (Ptr)&e, sizeof(short), &rSize) == noErr
            && e != noErr)
		{
			AEanswerErr = new char[35 + I2L];
			sprintf(AEanswerErr, "Got error %d when sending Apple event", e);
		}
		else
		{
			delete AEanswerData;
			if ((errno = AEGetParamPtr(ae, keyDirectObject, typeChar, &rType, NULL, 0, &rSize)) != noErr)
			{
				if (errno == errAEDescNotFound)
				{
					AEanswerData = MCU_empty();
					return noErr;
				}
				AEanswerErr = new char[37 + I2L];
				sprintf(AEanswerErr, "Got error %d when receiving Apple event", errno);
				return errno;
			}
			AEanswerData = new char[rSize + 1];
			AEGetParamPtr(ae, keyDirectObject, typeChar, &rType, AEanswerData, rSize, &rSize);
			AEanswerData[rSize] = '\0';
		}
	}
	return noErr;
}

/// END HERE

///////////////////////////////////////////////////////////////////////////////

static void MCS_launch_set_result_from_lsstatus(void)
{
	int t_error;
	t_error = 0;
    
	switch(errno)
	{
		case kLSUnknownErr:
		case kLSNotAnApplicationErr:
		case kLSLaunchInProgressErr:
		case kLSServerCommunicationErr:
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1040
		case kLSAppInTrashErr:
		case kLSIncompatibleSystemVersionErr:
		case kLSNoLaunchPermissionErr:
		case kLSNoExecutableErr:
		case kLSNoClassicEnvironmentErr:
		case kLSMultipleSessionsNotSupportedErr:
#endif
			t_error = 2;
            break;
            
		case kLSDataUnavailableErr:
		case kLSApplicationNotFoundErr:
		case kLSDataErr:
			t_error = 3;
            break;
	}
	
	switch(t_error)
	{
        case 0:
            MCresult -> clear();
            break;
            
        case 1:
            MCresult -> sets("can't open file");
            break;
            
        case 2:
            MCresult -> sets("request failed");
            break;
            
        case 3:
            MCresult -> sets("no association");
            break;
	}
    
}

///////////////////////////////////////////////////////////////////////////////

IO_stat MCS_mac_shellread(int fd, char *&buffer, uint4 &buffersize, uint4 &size)
{
	MCshellfd = fd;
	size = 0;
	while (True)
	{
		int readsize = 0;
		ioctl(fd, FIONREAD, (char *)&readsize);
		readsize += READ_PIPE_SIZE;
		if (size + readsize > buffersize)
		{
			MCU_realloc((char **)&buffer, buffersize,
                        buffersize + readsize + 1, sizeof(char));
			buffersize += readsize;
		}
		errno = 0;
		int4 amount = read(fd, &buffer[size], readsize);
		if (amount <= 0)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
				break;
			if (!MCS_poll(SHELL_INTERVAL, 0))
				if (!MCnoui && MCscreen->wait(SHELL_INTERVAL, False, True))
				{
					MCshellfd = -1;
					return IO_ERROR;
				}
		}
		else
			size += amount;
	}
	MCshellfd = -1;
	return IO_NORMAL;
}

///////////////////////////////////////////////////////////////////////////////

// Special Folders

// MW-2012-10-10: [[ Bug 10453 ]] Added 'mactag' field which is the tag to use in FSFindFolder.
//   This allows macfolder to be 0, which means don't alias the tag to the specified disk.
typedef struct
{
	MCNameRef *token;
	unsigned long macfolder;
	OSType domain;
	unsigned long mactag;
}
sysfolders;

// MW-2008-01-18: [[ Bug 5799 ]] It seems that we are requesting things in the
//   wrong domain - particularly for 'temp'. See:
// http://lists.apple.com/archives/carbon-development/2003/Oct/msg00318.html

static sysfolders sysfolderlist[] = {
    {&MCN_apple, 'amnu', kOnAppropriateDisk, 'amnu'},
    {&MCN_desktop, 'desk', kOnAppropriateDisk, 'desk'},
    {&MCN_control, 'ctrl', kOnAppropriateDisk, 'ctrl'},
    {&MCN_extension,'extn', kOnAppropriateDisk, 'extn'},
    {&MCN_fonts,'font', kOnAppropriateDisk, 'font'},
    {&MCN_preferences,'pref', kUserDomain, 'pref'},
    {&MCN_temporary,'temp', kUserDomain, 'temp'},
    {&MCN_system, 'macs', kOnAppropriateDisk, 'macs'},
    // TS-2007-08-20: Added to allow a common notion of "home" between all platforms
    {&MCN_home, 'cusr', kUserDomain, 'cusr'},
    // MW-2007-09-11: Added for uniformity across platforms
    {&MCN_documents, 'docs', kUserDomain, 'docs'},
    // MW-2007-10-08: [[ Bug 10277 ] Add support for the 'application support' at user level.
    {&MCN_support, 0, kUserDomain, 'asup'},
};

static bool MCS_mac_specialfolder_to_mac_folder(MCStringRef p_type, uint32_t& r_folder, OSType& r_domain)
{
	for (uindex_t i = 0; i < ELEMENTS(sysfolderlist); i++)
	{
		if (MCStringIsEqualTo(p_type, MCNameGetString(*(sysfolderlist[i].token)), kMCStringOptionCompareCaseless))
		{
			r_folder = sysfolderlist[i].macfolder;
			r_domain = sysfolderlist[i].domain;
            return true;
		}
	}
    return false;
}

/********************************************************************/
/*                        Serial Handling                           */
/********************************************************************/

// Utilities

static void parseSerialControlStr(char *setting, struct termios *theTermios)
{
	int baud = 0;
	char *type = setting;
	char *value = NULL;
	if ((value = strchr(type, '=')) != NULL)
	{
		*value++ = '\0';
		if (MCU_strncasecmp(type, "baud", strlen(type)) == 0)
		{
			long baudrate = strtol(value, NULL, 10);
			if (baudrate == 57600)
				baud = B57600;
			else if (baudrate == 38400)
				baud = B38400;
			else if (baudrate == 28800)
				baud = B28800;
			else if (baudrate == 19200)
				baud = B19200;
			else if (baudrate == 16600)
				baud = B16600;
			else if (baudrate == 14400)
				baud = B14400;
			else if (baudrate == 9600)
				baud = B9600;
			else if (baudrate == 7200)
				baud = B7200;
			else if (baudrate == 4800)
				baud = B4800;
			else if (baudrate == 3600)
				baud = B4800;
			else if (baudrate == 2400)
				baud = B2400;
			else if (baudrate == 1800)
				baud = B1800;
			else if (baudrate == 1200)
				baud = B1200;
			else if (baudrate == 600)
				baud = B600;
			else if (baudrate == 300)
				baud = B300;
			cfsetispeed(theTermios, baud);
			cfsetospeed(theTermios, baud);
		}
		else if (MCU_strncasecmp(type, "parity", strlen(type)) == 0)
		{
			if (value[0] == 'N' || value[0] == 'n')
				theTermios->c_cflag &= ~(PARENB | PARODD);
			else if (value[0] == 'O' || value[0] == 'o')
				theTermios->c_cflag |= PARENB | PARODD;
			else if (value[0] == 'E' || value[0] == 'e')
				theTermios->c_cflag |= PARENB;
		}
		else if (MCU_strncasecmp(type, "data", strlen(type)) == 0)
		{
			short data = atoi(value);
			switch (data)
			{
                case 5:
                    theTermios->c_cflag |= CS5;
                    break;
                case 6:
                    theTermios->c_cflag |= CS6;
                    break;
                case 7:
                    theTermios->c_cflag |= CS7;
                    break;
                case 8:
                    theTermios->c_cflag |= CS8;
                    break;
			}
		}
		else if (MCU_strncasecmp(type, "stop", strlen(type)) == 0)
		{
			double stopbit = strtol(value, NULL, 10);
			if (stopbit == 1.0)
				theTermios->c_cflag &= ~CSTOPB;
			else if (stopbit == 1.5)
				theTermios->c_cflag &= ~CSTOPB;
			else if (stopbit == 2.0)
				theTermios->c_cflag |= CSTOPB;
		}
	}
}

static void configureSerialPort(int sRefNum)
{/****************************************************************************
  *parse MCserialcontrolstring and set the serial output port to the settings*
  *defined by MCserialcontrolstring accordingly                              *
  ****************************************************************************/
	//initialize to the default setting
	struct termios	theTermios;
	if (tcgetattr(sRefNum, &theTermios) < 0)
	{
		// TODO: handle error appropriately
	}
	cfsetispeed(&theTermios,  B9600);
	theTermios.c_cflag = CS8;
    
	char *controlptr = strclone(MCStringGetCString(MCserialcontrolsettings));
	char *str = controlptr;
	char *each = NULL;
	while ((each = strchr(str, ' ')) != NULL)
	{
		*each = '\0';
		each++;
		if (str != NULL)
			parseSerialControlStr(str, &theTermios);
		str = each;
	}
	delete controlptr;
	//configure the serial output device
	parseSerialControlStr(str,&theTermios);
	if (tcsetattr(sRefNum, TCSANOW, &theTermios) < 0)
	{
		// TODO: handle error appropriately
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM TEXT.CPP
//

struct UnicodeInfoRecord
{
	UnicodeInfoRecord *next;
	TextEncoding encoding;
	TextToUnicodeInfo info;
};

static TextToUnicodeInfo fetch_unicode_info(TextEncoding p_encoding)
{
	static UnicodeInfoRecord *s_records = NULL;
	
	UnicodeInfoRecord *t_previous, *t_current;
	for(t_previous = NULL, t_current = s_records; t_current != NULL; t_previous = t_current, t_current = t_current -> next)
		if (t_current -> encoding == p_encoding)
			break;
    
	if (t_current == NULL)
	{
		UnicodeMapping t_mapping;
		t_mapping . unicodeEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault, kUnicodeNoSubset, kUnicode16BitFormat);
		t_mapping . otherEncoding = CreateTextEncoding(p_encoding, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat);
		t_mapping . mappingVersion = kUnicodeUseLatestMapping;
		
		TextToUnicodeInfo t_info;
		OSErr t_err;
		t_err = CreateTextToUnicodeInfo(&t_mapping, &t_info);
		if (t_err != noErr)
			t_info = NULL;
		
		UnicodeInfoRecord *t_record;
		t_record = new UnicodeInfoRecord;
		t_record -> next = s_records;
		t_record -> encoding = p_encoding;
		t_record -> info = t_info;
		s_records = t_record;
		
		return t_record -> info;
	}
	
	if (t_previous != NULL)
	{
		t_previous -> next = t_current -> next;
		t_current -> next = s_records;
		s_records = t_current;
	}
	
	return s_records -> info;
}

///////////////////////////////////////////////////////////////////////////////

static void MCS_mac_setfiletype(MCStringRef p_new_path)
{
	FSRef t_fsref;
    // TODO Check whether the double path resolution is an issue
	if (MCS_mac_pathtoref(p_new_path, t_fsref) != noErr)
		return; // ignore errors
    
	FSCatalogInfo t_catalog;
	if (FSGetCatalogInfo(&t_fsref, kFSCatInfoFinderInfo, &t_catalog, NULL, NULL, NULL) == noErr)
	{
		// Set the creator and filetype of the catalog.
        const char *t_char_ptr;
        t_char_ptr = MCStringGetCString(MCfiletype);
		memcpy(&((FileInfo *) t_catalog . finderInfo) -> fileType, t_char_ptr + 4, 4);
		memcpy(&((FileInfo *) t_catalog . finderInfo) -> fileCreator, t_char_ptr, 4);
		((FileInfo *) t_catalog . finderInfo) -> fileType = MCSwapInt32NetworkToHost(((FileInfo *) t_catalog . finderInfo) -> fileType);
		((FileInfo *) t_catalog . finderInfo) -> fileCreator = MCSwapInt32NetworkToHost(((FileInfo *) t_catalog . finderInfo) -> fileCreator);
        
		FSSetCatalogInfo(&t_fsref, kFSCatInfoFinderInfo, &t_catalog);
	}
}

///////////////////////////////////////////////////////////////////////////////

extern "C"
{
#include	<CoreFoundation/CoreFoundation.h>
#include	<IOKit/IOKitLib.h>
#include	<IOKit/serial/IOSerialKeys.h>
#include	<IOKit/IOBSD.h>
}

static kern_return_t FindSerialPortDevices(io_iterator_t *serialIterator, mach_port_t *masterPort)
{
    kern_return_t	kernResult;
    CFMutableDictionaryRef classesToMatch;
    if ((kernResult = IOMasterPort(NULL, masterPort)) != KERN_SUCCESS)
        return kernResult;
    if ((classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue)) == NULL)
        return kernResult;
    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey),
                         CFSTR(kIOSerialBSDRS232Type));
    //kIOSerialBSDRS232Type filters KeySpan USB modems use
    //kIOSerialBSDModemType to get 'real' serial modems for OSX
    //computers with real serial ports - if there are any!
    kernResult = IOServiceGetMatchingServices(*masterPort, classesToMatch,
                                              serialIterator);
    return kernResult;
}

static void getIOKitProp(io_object_t sObj, const char *propName,
                         char *dest, uint2 destlen)
{
	CFTypeRef nameCFstring;
	dest[0] = 0;
	nameCFstring = IORegistryEntryCreateCFProperty(sObj,
                                                   CFStringCreateWithCString(kCFAllocatorDefault, propName,
                                                                             kCFStringEncodingASCII),
                                                   kCFAllocatorDefault, 0);
	if (nameCFstring)
	{
		CFStringGetCString((CFStringRef)nameCFstring, (char *)dest, (long)destlen,
                           (unsigned long)kCFStringEncodingASCII);
		CFRelease(nameCFstring);
	}
}

///////////////////////////////////////////////////////////////////////////////

//for setting serial port use
typedef struct
{
	short baudrate;
	short parity;
	short stop;
	short data;
}
SerialControl;

//struct
SerialControl portconfig; //serial port configuration structure

extern "C"
{
	extern UInt32 SwapQDTextFlags(UInt32 newFlags);
	typedef UInt32 (*SwapQDTextFlagsPtr)(UInt32 newFlags);
}

static void configureSerialPort(int sRefNum);
static bool getResourceInfo(MCListRef p_list, ResType p_type);
static void parseSerialControlStr(char *set, struct termios *theTermios);


static UnicodeToTextInfo unicodeconvertors[32];
static TextToUnicodeInfo texttounicodeinfo;
static TextToUnicodeInfo *texttounicodeconvertor = NULL;
static UnicodeToTextInfo utf8totextinfo;
static TextToUnicodeInfo texttoutf8info;

///////////////////////////////////////////////////////////////////////////////

static void init_utf8_converters(void)
{
	if (texttoutf8info != nil)
		return;
	
	memset(unicodeconvertors, 0, sizeof(unicodeconvertors));
	UnicodeMapping ucmapping;
	ucmapping.unicodeEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault,
												   kTextEncodingDefaultVariant,
												   kUnicodeUTF8Format);
	ucmapping.otherEncoding = kTextEncodingMacRoman;
	ucmapping.mappingVersion = -1;
	CreateTextToUnicodeInfo(&ucmapping, &texttoutf8info);
	CreateUnicodeToTextInfo(&ucmapping, &utf8totextinfo);
}
///////////////////////////////////////////////////////////////////////////////

/* LEGACY */
extern char *path2utf(char *);

static void handle_signal(int sig)
{
	MCHandler handler(HT_MESSAGE);
	switch (sig)
	{
        case SIGUSR1:
            MCsiguser1++;
            break;
        case SIGUSR2:
            MCsiguser2++;
            break;
        case SIGTERM:
            switch (MCdefaultstackptr->getcard()->message(MCM_shut_down_request))
		{
            case ES_NORMAL:
                return;
            case ES_PASS:
            case ES_NOT_HANDLED:
                MCdefaultstackptr->getcard()->message(MCM_shut_down);
                MCquit = True; //set MC quit flag, to invoke quitting
                return;
            default:
                break;
		}
            MCS_killall();
            exit(-1);
            
            // MW-2009-01-29: [[ Bug 6410 ]] If one of these signals occurs, we need
            //   to return, so that the OS can CrashReport away.
        case SIGILL:
        case SIGBUS:
        case SIGSEGV:
            fprintf(stderr, "%s exiting on signal %d\n", MCStringGetCString(MCcmd), sig);
            MCS_killall();
            return;
            
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGIOT:
            if (MCnoui)
                exit(1);
            MCabortscript = True;
            break;
        case SIGFPE:
            errno = EDOM;
            break;
        case SIGCHLD:
            MCS_checkprocesses();
            break;
        case SIGALRM:
            MCalarm = True;
            break;
        case SIGPIPE:
        default:
            break;
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////

// MW-2005-08-15: We have two types of process starting in MacOS X it seems:
//   MCS_startprocess is called by MCLaunch with a docname
//   MCS_startprocess is called by MCOpen without a docname
// Thus, we will fork two methods - and dispatch from MCS_startprocess
static void MCS_startprocess_unix(MCNameRef name, MCStringRef doc, Open_mode mode, Boolean elevated);
static void MCS_startprocess_launch(MCNameRef name, MCStringRef docname, Open_mode mode);

///////////////////////////////////////////////////////////////////////////////

static Boolean hasPPCToolbox = False;
static Boolean hasAppleEvents = False;

///////////////////////////////////////////////////////////////////////////////

// MW-2005-02-22: Make this global scope for now to enable opensslsocket.cpp
//   to access it.
real8 curtime;

///////////////////////////////////////////////////////////////////////////////

bool MCS_mac_is_link(MCStringRef p_path)
{
#ifdef /* MCS_is_link_mac_dsk */ LEGACY_SYSTEM
	struct stat buf;
	return (lstat(MCStringGetCString(p_path), &buf) == 0 && S_ISLNK(buf.st_mode));
#endif /* MCS_is_link_mac_dsk */
	struct stat buf;
	return (lstat(MCStringGetCString(p_path), &buf) == 0 && S_ISLNK(buf.st_mode));
}

bool MCS_mac_readlink(MCStringRef p_path, MCStringRef& r_link)
{
#ifdef /* MCS_readlink_mac_dsk */ LEGACY_SYSTEM
	struct stat t_stat;
	ssize_t t_size;
	MCAutoNativeCharArray t_buffer;
    
	if (lstat(MCStringGetCString(p_path), &t_stat) == -1 ||
		!t_buffer.New(t_stat.st_size))
		return false;
    
	t_size = readlink(MCStringGetCString(p_path), (char*)t_buffer.Chars(), t_stat.st_size);
    
	return (t_size == t_stat.st_size) && t_buffer.CreateStringAndRelease(r_link);
#endif /* MCS_readlink_mac_dsk */
	struct stat t_stat;
	ssize_t t_size;
	MCAutoNativeCharArray t_buffer;
    
	if (lstat(MCStringGetCString(p_path), &t_stat) == -1 ||
		!t_buffer.New(t_stat.st_size))
		return false;
    
	t_size = readlink(MCStringGetCString(p_path), (char*)t_buffer.Chars(), t_stat.st_size);
    
	return (t_size == t_stat.st_size) && t_buffer.CreateStringAndRelease(r_link);
}

Boolean MCS_mac_nodelay(int4 p_fd)
{
#ifdef /* MCS_nodelay_dsk_mac */ LEGACY_SYSTEM
	return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & O_APPEND | O_NONBLOCK)
    >= 0;
#endif /* MCS_nodelay_dsk_mac */
	return fcntl(p_fd, F_SETFL, (fcntl(p_fd, F_GETFL, 0) & O_APPEND) | O_NONBLOCK)
    >= 0;
}

///////////////////////////////////////////////////////////////////////////////

static bool MCS_mac_path2std(MCStringRef p_path, MCStringRef& r_stdpath)
{
	uindex_t t_length = MCStringGetLength(p_path);
	if (t_length == 0)
		return MCStringCopy(p_path, r_stdpath);
    
	MCAutoNativeCharArray t_path;
	if (!t_path.New(t_length))
		return false;
    
	const char_t *t_src = MCStringGetNativeCharPtr(p_path);
	char_t *t_dst = t_path.Chars();
    
	for (uindex_t i = 0; i < t_length; i++)
	{
		if (t_src[i] == '/')
			t_dst[i] = ':';
		else if (t_src[i] == ':')
			t_dst[i] = '/';
		else
			t_dst[i] = t_src[i];
	}
    
	return t_path.CreateStringAndRelease(r_stdpath);
}

OSErr MCS_mac_pathtoref(MCStringRef p_path, FSRef& r_ref)
{
#ifdef /* MCS_pathtoref_dsk_mac */ LEGACY_SYSTEM
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(p_path);
	
	char *t_utf8_path;
	t_utf8_path = path2utf(t_resolved_path);
	
	OSErr t_error;
	t_error = FSPathMakeRef((const UInt8 *)t_utf8_path, r_ref, NULL);
	
	delete t_utf8_path;
	
	// path2utf deletes t_resolved_path
	// delete t_resolved_path;
	
	return t_error;
#endif /* MCS_pathtoref_dsk_mac */
    MCAutoStringRef t_auto_path;
    MCAutoStringRefAsUTF8String t_path;
    
    if (!MCS_resolvepath(p_path, &t_auto_path))
        // TODO assign relevant error code
        return memFullErr;
    
    if (!t_path.Lock(*t_auto_path))
        return memFullErr;
    
	return FSPathMakeRef((const UInt8 *)(*t_path), &r_ref, NULL);
}

static OSErr MCS_mac_pathtoref_and_leaf(MCStringRef p_path, FSRef& r_ref, UniChar*& r_leaf, UniCharCount& r_leaf_length)
{
#ifdef /* MCS_pathtoref_and_leaf */ LEGACY_SYSTEM
	OSErr t_error;
	t_error = noErr;
    
	char *t_resolved_path;
	t_resolved_path = NULL;
	if (t_error == noErr)
		t_resolved_path = MCS_resolvepath(p_path);
	
	char *t_resolved_path_leaf;
	t_resolved_path_leaf = NULL;
	if (t_error == noErr)
	{
		t_resolved_path_leaf = strrchr(t_resolved_path, '/');
		if (t_resolved_path_leaf != NULL)
		{
			t_resolved_path_leaf[0] = '\0';
			t_resolved_path_leaf += 1;
		}
		else
			t_error = fnfErr;
	}
    
	char *t_utf8_path;
	t_utf8_path = NULL;
	
	// OK-2010-04-06: [[Bug]] - path2utf frees the buffer passed into it, so we have to clone t_resolved_path
	// here, as otherwise we are using it after its been freed.
	char *t_resolved_path_clone;
	t_resolved_path_clone = strdup(t_resolved_path);
	
	if (t_error == noErr)
		t_utf8_path = path2utf(t_resolved_path_clone);
    
	if (t_error == noErr)
		t_error = FSPathMakeRef((const UInt8 *)t_utf8_path, &r_ref, NULL);
	
	// Convert the leaf from MacRoman to UTF16.
	if (t_error == noErr)
	{
		unsigned short *t_utf16_leaf;
		uint4 t_utf16_leaf_length;
		
		t_utf16_leaf = new unsigned short[256];
		t_utf16_leaf_length = 256;
		MCS_nativetoutf16(t_resolved_path_leaf, strlen(t_resolved_path_leaf), t_utf16_leaf, t_utf16_leaf_length);
        
		r_leaf = (UniChar *)t_utf16_leaf;
		r_leaf_length = (UniCharCount)t_utf16_leaf_length;
	}
	
	if (t_utf8_path != NULL)
		delete t_utf8_path;
    
	return t_error;
#endif /* MCS_pathtoref_and_leaf */
	OSErr t_error;
	t_error = noErr;
    
	MCAutoStringRef t_resolved_path;
    
    if (!MCS_resolvepath(p_path, &t_resolved_path))
        // TODO assign relevant error code
        t_error = fnfErr;
    
    MCAutoStringRef t_folder, t_leaf;
    uindex_t t_leaf_index;
    if (MCStringLastIndexOfChar(*t_resolved_path, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_leaf_index))
    {
        if (!MCStringDivideAtIndex(*t_resolved_path, t_leaf_index, &t_folder, &t_leaf))
            t_error = memFullErr;
    }
    else
        t_error = fnfErr;
    
	MCAutoStringRefAsUTF8String t_utf8_auto;
    
    if (t_error == noErr)
        if (!t_utf8_auto.Lock(*t_folder))
            t_error = fnfErr;
    
	if (t_error == noErr)
		t_error = FSPathMakeRef((const UInt8 *)*t_utf8_auto, &r_ref, NULL);
	
	// Convert the leaf from MacRoman to UTF16.
	if (t_error == noErr)
	{
		unichar_t *t_utf16_leaf;
		uint4 t_utf16_leaf_length;
        /* UNCHECKED */ MCStringConvertToUnicode(*t_leaf, t_utf16_leaf, t_utf16_leaf_length);
		r_leaf = (UniChar *)t_utf16_leaf;
		r_leaf_length = (UniCharCount)t_utf16_leaf_length;
	}
    
	return t_error;
}

static OSErr MCS_mac_fsspec_to_fsref(const FSSpec *p_fsspec, FSRef *r_fsref)
{
	return FSpMakeFSRef(p_fsspec, r_fsref);
}

static OSErr MCS_mac_fsref_to_fsspec(const FSRef *p_fsref, FSSpec *r_fsspec)
{
	return FSGetCatalogInfo(p_fsref, 0, NULL, NULL, r_fsspec, NULL);
}

void MCS_mac_closeresourcefile(SInt16 p_ref) // TODO: remove?
{
	OSErr t_err;
	CloseResFile(p_ref);
	t_err = ResError();
}

bool MCS_mac_fsref_to_path(FSRef& p_ref, MCStringRef& r_path)
{
	MCAutoNativeCharArray t_buffer;
	if (!t_buffer.New(PATH_MAX))
		return false;
	FSRefMakePath(&p_ref, (UInt8*)t_buffer.Chars(), PATH_MAX);
    
	t_buffer.Shrink(MCCStringLength((char*)t_buffer.Chars()));
    
	MCAutoStringRef t_utf8_path;
	return t_buffer.CreateStringAndRelease(&t_utf8_path) &&
    MCU_utf8tonative(*t_utf8_path, r_path);
}

bool MCS_mac_FSSpec2path(FSSpec *fSpec, MCStringRef& r_path)
{
#ifdef /* MCS_mac_FSSpec2path_dsk_mac */ LEGACY_SYSTEM
	char *path = new char[PATH_MAX + 1];
    
    
	char *fname = new char[PATH_MAX + 1];
    
	CopyPascalStringToC(fSpec->name, fname);
	MCU_path2std(fname);
    
	char oldchar = fSpec->name[0];
	Boolean dontappendname = False;
	fSpec->name[0] = '\0';
    
	FSRef ref;
    
	// MW-2005-01-21: Removed the following two lines - function would not work if file did not already exist
    
	/* fSpec->name[0] = oldchar;
     dontappendname = True;*/
    
	if ((errno = FSpMakeFSRef(fSpec, &ref)) != noErr)
	{
		if (errno == nsvErr)
		{
			fSpec->name[0] = oldchar;
			if ((errno = FSpMakeFSRef(fSpec, &ref)) == noErr)
			{
				errno = FSRefMakePath(&ref, (unsigned char *)path, PATH_MAX);
				dontappendname = True;
			}
			else
				path[0] = '\0';
		}
		else
			path[0] = '\0';
	}
	else
		errno = FSRefMakePath(&ref, (unsigned char *)path, PATH_MAX);
	uint4 destlen;
	char *tutfpath = new char[PATH_MAX + 1];
	destlen = PATH_MAX;
	MCS_utf8tonative(path, strlen(path), tutfpath, destlen);
	tutfpath[destlen] = '\0';
	if (!dontappendname)
	{
		if (tutfpath[destlen - 1] != '/')
			strcat(tutfpath, "/");
		strcat(tutfpath, fname);
	}
	delete fname;
	delete path;
	return tutfpath;
#endif /* MCS_mac_FSSpec2path_dsk_mac */
    MCAutoNativeCharArray t_path, t_name;
    MCAutoStringRef t_filename;
    MCAutoStringRef t_filename_std;
    char *t_char_ptr;
    
    t_path.New(PATH_MAX + 1);
    t_name.New(PATH_MAX + 1);
    
    t_char_ptr = (char*)t_path.Chars();
    
	CopyPascalStringToC(fSpec->name, (char*)t_name.Chars());
    
    /* UNCHECKED */ t_name.CreateStringAndRelease(&t_filename_std);
    
	/* UNCHECKED */ MCS_pathfromnative(*t_filename_std, &t_filename);
    
    t_char_ptr = strclone(MCStringGetCString(*t_filename));
    
	char oldchar = fSpec->name[0];
	Boolean dontappendname = False;
	fSpec->name[0] = '\0';
    
	FSRef ref;
    
	// MW-2005-01-21: Removed the following two lines - function would not work if file did not already exist
    
	/* fSpec->name[0] = oldchar;
     dontappendname = True;*/
    
	if ((errno = FSpMakeFSRef(fSpec, &ref)) != noErr)
	{
		if (errno == nsvErr)
		{
			fSpec->name[0] = oldchar;
			if ((errno = FSpMakeFSRef(fSpec, &ref)) == noErr)
			{
				errno = FSRefMakePath(&ref, (unsigned char *)t_char_ptr, PATH_MAX);
				dontappendname = True;
			}
			else
				t_char_ptr[0] = '\0';
		}
		else
			t_char_ptr[0] = '\0';
	}
	else
		errno = FSRefMakePath(&ref, (unsigned char *)t_char_ptr, PATH_MAX);
	uint4 destlen;
	char *tutfpath = new char[PATH_MAX + 1];
	destlen = PATH_MAX;
	MCS_utf8tonative(t_char_ptr, strlen(t_char_ptr), tutfpath, destlen);
	tutfpath[destlen] = '\0';
	if (!dontappendname)
	{
		if (tutfpath[destlen - 1] != '/')
			strcat(tutfpath, "/");
		strcat(tutfpath, (char*)t_name.Chars());
	}
	return tutfpath;
}

static void MCS_openresourcefork_with_fsref(FSRef *p_ref, SInt8 p_permission, bool p_create, SInt16 *r_fork_ref, MCStringRef& r_error)
{
    bool t_success;
    t_success = true;
	
	HFSUniStr255 t_resource_fork_name;
	if (t_success)
	{
		OSErr t_os_error;
		t_os_error = FSGetResourceForkName(&t_resource_fork_name);
		if (t_os_error != noErr)
        {
            t_success = false;
            /* UNCHECKED */ MCStringCreateWithCString("couldn't get resource fork name", r_error);
        }
	}
	
	// Attempt to create a resource fork if required.
	if (t_success && p_create)
	{
		OSErr t_os_error;
		t_os_error = FSCreateResourceFork(p_ref, (UniCharCount)t_resource_fork_name . length, t_resource_fork_name . unicode, 0);
		if (t_os_error != noErr && t_os_error != errFSForkExists)
        {
            t_success = false;
            /* UNCHECKED */ MCStringCreateWithCString("can't create resource fork", r_error);
        }
	}
	
	// Open it..
	SInt16 t_fork_ref;
	bool t_fork_opened;
	t_fork_opened = false;
	if (t_success)
	{
		OSErr t_os_error;
		t_os_error = FSOpenFork(p_ref, (UniCharCount)t_resource_fork_name . length, t_resource_fork_name . unicode, p_permission, &t_fork_ref);
		if (t_os_error == noErr)
			t_fork_opened = true;
		else
        {
            t_success = false;
            /* UNCHECKED */ MCStringCreateWithCString("can't open resource fork", r_error);
        }
	}
	
	*r_fork_ref = t_fork_ref;
}

static void MCS_mac_openresourcefork_with_path(MCStringRef p_path, SInt8 p_permission, bool p_create, SInt16*r_fork_ref, MCStringRef& r_error)
{
	FSRef t_ref;
	OSErr t_os_error;
	t_os_error = MCS_mac_pathtoref(p_path, t_ref);
	if (t_os_error != noErr)
    {
		/* UNCHECKED */ MCStringCreateWithCString("can't open file", r_error);
        return;
    }
    
	MCS_openresourcefork_with_fsref(&t_ref, p_permission, p_create, r_fork_ref, r_error);
}

static bool MCS_mac_openresourcefile_with_fsref(FSRef& p_ref, SInt8 p_permissions, bool p_create, SInt16& r_fileref_num, MCStringRef& r_error)
{
	FSSpec fspec;
	
	if (FSGetCatalogInfo(&p_ref, 0, NULL, NULL, &fspec, NULL) != noErr)
		return MCStringCreateWithCString("file not found", r_error);
	
	r_fileref_num = FSOpenResFile(&p_ref, p_permissions);
	if (p_create && r_fileref_num < 0)
	{
		OSType t_creator, t_ftype;
		CInfoPBRec t_cpb;
		MCMemoryClear(&t_cpb, sizeof(t_cpb));
		t_cpb.hFileInfo.ioNamePtr = fspec.name;
		t_cpb.hFileInfo.ioVRefNum = fspec.vRefNum;
		t_cpb.hFileInfo.ioDirID = fspec.parID;
		/* DEPRECATED */ if (PBGetCatInfoSync(&t_cpb) == noErr)
		{
			t_creator = t_cpb.hFileInfo.ioFlFndrInfo.fdCreator;
			t_ftype = t_cpb.hFileInfo.ioFlFndrInfo.fdType;
		}
		else
		{
            const char *t_char_ptr;
            t_char_ptr = (char*)MCStringGetCString(MCfiletype);
			t_creator = MCSwapInt32NetworkToHost(*(OSType*)t_char_ptr);
			t_ftype = MCSwapInt32NetworkToHost(*(OSType*)(t_char_ptr + 4));
		}
		/* DEPRECATED */ FSpCreateResFile(&fspec, t_creator, t_ftype, smRoman);
		
		if ((errno = ResError()) != noErr)
			return MCStringCreateWithCString("can't create resource fork", r_error);
		
		/* DEPRECATED */ r_fileref_num = FSpOpenResFile(&fspec, p_permissions);
	}
	
	if (r_fileref_num < 0)
	{
		errno = fnfErr;
		return MCStringCreateWithCString("Can't open resource fork", r_error);
	}
	
	if ((errno = ResError()) != noErr)
		return MCStringCreateWithCString("Error opening resource fork", r_error);
	
	return true;
}

bool MCS_mac_openresourcefile_with_path(MCStringRef p_path, SInt8 p_permission, bool p_create, SInt16& r_fork_ref, MCStringRef& r_error)
{
    //	MCAutoStringRef t_utf8_path;
    //	if (!MCU_nativetoutf8(p_path, &t_utf8_path))
    //		return false;`
	
	FSRef t_ref;
	OSErr t_os_error;
	
	t_os_error = MCS_mac_pathtoref(p_path, t_ref);
	if (t_os_error != noErr)
		return MCStringCreateWithCString("can't open file", r_error);
	
	return MCS_mac_openresourcefile_with_fsref(t_ref, p_permission, p_create, r_fork_ref, r_error);
}

static const char *MCS_mac_openresourcefile_with_fsref(FSRef *p_ref, SInt8 permission, bool create, SInt16 *fileRefNum)
{
	FSSpec fspec;
	
	if (FSGetCatalogInfo(p_ref, 0, NULL, NULL, &fspec, NULL) != noErr)
		return "file not found";
	
	if ((*fileRefNum = FSpOpenResFile(&fspec, permission)) < 0)
	{
		if (create)
		{
			OSType creator, ftype;
			CInfoPBRec cpb;
			memset(&cpb, 0, sizeof(CInfoPBRec));
			cpb.hFileInfo.ioNamePtr = fspec.name;
			cpb.hFileInfo.ioVRefNum = fspec.vRefNum;
			cpb.hFileInfo.ioDirID = fspec.parID;
			if (PBGetCatInfoSync(&cpb) == noErr)
			{
				memcpy(&creator, &cpb.hFileInfo.ioFlFndrInfo.fdCreator, 4);
				memcpy(&ftype, &cpb.hFileInfo.ioFlFndrInfo.fdType, 4);
			}
			else
			{
                const char *t_char_ptr;
                t_char_ptr = (char*)MCStringGetCString(MCfiletype);
				memcpy((char*)&creator, t_char_ptr, 4);
				memcpy((char*)&ftype, t_char_ptr + 4, 4);
				creator = MCSwapInt32NetworkToHost(creator);
				ftype = MCSwapInt32NetworkToHost(ftype);
			}
			FSpCreateResFile(&fspec, creator, ftype, smRoman);
			
			if ((errno = ResError()) != noErr)
				return "can't create resource fork";
            
			*fileRefNum = FSpOpenResFile(&fspec, permission);
		}
		
		if (*fileRefNum < 0)
		{
			errno = fnfErr;
			return "Can't open resource fork";
		}
		
		if ((errno = ResError()) != noErr)
			return "Error opening resource fork";
	}
	
	return NULL;
}

// based on MoreFiles (Apple DTS)
OSErr MCS_path2FSSpec(MCStringRef p_filename, FSSpec *fspec)
{
#ifdef /* MCS_path2FSSpec_dsk_mac */ LEGACY_SYSTEM
	char *path = MCS_resolvepath(p_filename));
	memset(fspec, 0, sizeof(FSSpec));
    
	char *f2 = strrchr(path, '/');
	if (f2 != NULL && f2 != path)
		*f2++ = '\0';
	char *fspecname = strclone(f2);
	path = path2utf(path);
	FSRef ref;
	if ((errno = FSPathMakeRef((unsigned char *)path, &ref, NULL)) == noErr)
	{
		if ((errno = FSGetCatalogInfo(&ref, kFSCatInfoNone,
		                              NULL, NULL, fspec, NULL)) == noErr)
		{
			CInfoPBRec cpb;
			memset(&cpb, 0, sizeof(CInfoPBRec));
			cpb.dirInfo.ioNamePtr = fspec->name;
			cpb.dirInfo.ioVRefNum = fspec->vRefNum;
			cpb.dirInfo.ioDrDirID = fspec->parID;
			if ((errno = PBGetCatInfoSync(&cpb)) != noErr)
			{
				delete path;
				return errno;
			}
			c2pstr((char *)fspecname);
			errno = FSMakeFSSpec(cpb.dirInfo.ioVRefNum, cpb.dirInfo.ioDrDirID,
			                     (unsigned char *)fspecname, fspec);
		}
	}
	delete fspecname;
	delete path;
	return errno;
#endif /* MCS_path2FSSpec_dsk_mac */
    MCAutoStringRef t_resolved_path;
    MCAutoStringRefAsUTF8String t_utf_path;
    
	if (!MCS_resolvepath(p_filename, &t_resolved_path))
        return memFullErr;
    
	memset(fspec, 0, sizeof(FSSpec));
    
	char *f2 = strrchr(MCStringGetCString(*t_resolved_path), '/');
	if (f2 != NULL && f2 != (const char*)MCStringGetCString(*t_resolved_path))
		*f2++ = '\0';
	char *fspecname = strclone(f2);
    if (!t_utf_path.Lock(*t_resolved_path))
        return memFullErr;
    
	FSRef ref;
	if ((errno = FSPathMakeRef((unsigned char*)*t_utf_path, &ref, NULL)) == noErr)
	{
		if ((errno = FSGetCatalogInfo(&ref, kFSCatInfoNone,
		                              NULL, NULL, fspec, NULL)) == noErr)
		{
			CInfoPBRec cpb;
			memset(&cpb, 0, sizeof(CInfoPBRec));
			cpb.dirInfo.ioNamePtr = fspec->name;
			cpb.dirInfo.ioVRefNum = fspec->vRefNum;
			cpb.dirInfo.ioDrDirID = fspec->parID;
			if ((errno = PBGetCatInfoSync(&cpb)) != noErr)
			{
				return errno;
			}
			c2pstr((char *)fspecname);
			errno = FSMakeFSSpec(cpb.dirInfo.ioVRefNum, cpb.dirInfo.ioDrDirID,
			                     (unsigned char *)fspecname, fspec);
		}
	}
	delete fspecname;
	return errno;
}

OSErr MCS_path2FSSpec(const char *fname, FSSpec *fspec)
{
	MCAutoStringRef t_filename;
	/* UNCHECKED */ MCStringCreateWithCString(fname, &t_filename);
	return MCS_path2FSSpec(*t_filename, fspec);
}

///////////////////////////////////////////////////////////////////////////////

// Resource utility functions

static bool getResourceInfo(MCListRef p_list, ResType searchType)
{ /* get info of resources of a resource type and build a list of
   * <resName>, <resID>, <resType>, <resSize> on each line */
	Handle rh;
	short rid;
	ResType rtype;
	Str255 rname;  //Pascal string
	char cstr[256];  //C string
	char typetmp[5]; //buffer for storing type string in c format
	short total = Count1Resources(searchType);
	if (ResError() != noErr)
	{
		errno = ResError();
		return false;
	}
	char buffer[4 + U2L + 255 + U4L + 6];
	for (uindex_t i = 1 ; i <= total ; i++)
	{
		if ((rh = Get1IndResource(searchType, i)) == NULL)
			continue;
		GetResInfo(rh, &rid, &rtype, rname);
		p2cstrcpy(cstr, rname); //convert to C string
		// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
		rtype = (ResType)MCSwapInt32NetworkToHost(rtype);
		memcpy(typetmp, (char*)&rtype, 4);
		typetmp[4] = '\0';
		//format res info into "type, id, name, size, attributes" string --
		short flags = GetResAttrs(rh);
		char fstring[7];
		char *sptr = fstring;
		if (flags & resSysHeap)
			*sptr++ = 'S';
		if (flags & resPurgeable)
			*sptr++ = 'U';
		if (flags & resLocked)
			*sptr++ = 'L';
		if (flags & resProtected)
			*sptr++ = 'P';
		if (flags & resPreload)
			*sptr++ = 'R';
		if (flags & resChanged)
			*sptr++ = 'C';
		*sptr = '\0';
		
		MCAutoStringRef t_string;
		if (!MCStringFormat(&t_string, "%4s,%d,%s,%ld,%s\n", typetmp, rid, cstr,
                            GetMaxResourceSize(rh), fstring))
			return false;
		if (!MCListAppend(p_list, *t_string))
			return false;
	}
	
	return true;
}

/********************************************************************/
/*                       Resource Handling                          */
/********************************************************************/

class MCAutoResourceFileHandle
{
public:
	MCAutoResourceFileHandle()
	{
		m_res_file = 0;
	}
	
	~MCAutoResourceFileHandle()
	{
		if (m_res_file != 0)
			MCS_mac_closeresourcefile(m_res_file);
	}
	
	short operator = (short p_res_file)
	{
		MCAssert(m_res_file == 0);
		m_res_file = p_res_file;
		return m_res_file;
	}
	
	short operator * (void)
	{
		return m_res_file;
	}
	
	short& operator & (void)
	{
		MCAssert(m_res_file == 0);
		return m_res_file;
	}
	
private:
	short m_res_file;
};

//////////////////////////////////////////////////////////////////text/////////////

class MCStdioFileHandle: public MCSystemFileHandle
{
public:
    
    MCStdioFileHandle(FILE *p_fptr)
    {
        m_stream = p_fptr;
    }
	
	virtual void Close(void)
	{
#ifdef /* MCS_close_dsk_mac */ LEGACY_SYSTEM
	IO_stat stat = IO_NORMAL;
	if (stream->serialIn != 0 || stream->serialOut != 0)
	{//close the serial port

	}
	else
		if (stream->fptr == NULL)
		{
			if (!(stream->flags & IO_FAKE))
				delete stream->buffer;
		}
		else
			fclose(stream->fptr);
	delete stream;
	stream = NULL;
	return stat;
#endif /* MCS_close_dsk_mac */
       if (m_stream != NULL)
            fclose(m_stream);
        
        delete this;
	}
	
	virtual bool Read(void *p_ptr, uint32_t p_length, uint32_t& r_read)
	{
#ifdef /* MCS_read_dsk_mac */ LEGACY_SYSTEM
	if (MCabortscript || stream == NULL)
		return IO_ERROR;

	if ((stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
		return IO_ERROR;

	// MW-2009-06-25: If this is a custom stream, call the appropriate callback.
	// MW-2009-06-30: Refactored to common (platform-independent) implementation
	//   in mcio.cpp
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_read(ptr, size, n, stream);

	IO_stat stat = IO_NORMAL;
	uint4 nread;
	if (stream-> serialIn != 0)
	{//read from serial port
		long count = 0;  // n group of size data to be read

		count = MCU_min(count, size * n);
		if (count > 0)
			if ((errno = FSRead(stream->serialIn, &count, ptr)) != noErr)
				stat = IO_ERROR;
		if ((uint4)count < size * n)
			stat = IO_EOF;
		n = count / size;
	}
	else
		if (stream->fptr == NULL)
		{ //read from an IO_handle's buffer
			nread = size * n;
			if (nread > stream->len - (stream->ioptr - stream->buffer))
			{
				n = stream->len - (stream->ioptr - stream->buffer) / size;
				nread = size * n;
				stat = IO_EOF;
			}
			if (nread == 1)
			{
				char *tptr = (char *)ptr;
				*tptr = *stream->ioptr++;
			}
			else
			{
				memcpy(ptr, stream->ioptr, nread);
				stream->ioptr += nread;
			}
		}
		else
		{
			// MW-2010-08-26: Taken from the Linux source, this changes the previous code
			//   to take into account pipes and such.
			char *sptr = (char *)ptr;
			uint4 nread;
			uint4 toread = n * size;
			uint4 offset = 0;
			errno = 0;
			while ((nread = fread(&sptr[offset], 1, toread, stream->fptr)) != toread)
			{
				offset += nread;
				n = offset / size;
				if (ferror(stream->fptr))
				{
					clearerr(stream->fptr);
					
					if (errno == EAGAIN)
						return IO_NORMAL;
					
					if (errno == EINTR)
					{
						toread -= nread;
						continue;
					}
					else
						return IO_ERROR;
				}
				if (MCS_eof(stream))
				{
					return IO_EOF;
				}
				return IO_NONE;
			}
		}
	return stat;
#endif /* MCS_read_dsk_mac */
        uint4 nread;
        
        // MW-2010-08-26: Taken from the Linux source, this changes the previous code
        //   to take into account pipes and such.
        char *sptr = (char *)p_ptr;
        uint4 toread = p_length;
        uint4 offset = 0;
        errno = 0;
        while ((nread = fread(&sptr[offset], 1, toread, m_stream)) != toread)
        {
            offset += nread;
            r_read = offset;
            if (ferror(m_stream))
            {
                clearerr(m_stream);
                
                if (errno == EAGAIN)
                    return false;
                
                if (errno == EINTR)
                {
                    toread -= nread;
                    continue;
                }
                else
                    return false;
            }
            if (feof(m_stream))
            {
                m_is_eof = true;
                return true;
            }
            
            m_is_eof = false;
            return false;
        }
        r_read = nread;
        return true;
	}
    
	virtual bool Write(const void *p_buffer, uint32_t p_length)
	{
#ifdef /* MCS_write_dsk_mac */ LEGACY_SYSTEM
        if (stream == NULL)
            return IO_ERROR;
        if (stream->serialOut != 0)
        {//write to serial port
            uint4 count = size * n;
            errno = FSWrite(stream->serialOut, (long*)&count, ptr);
            if (errno == noErr && count == size * n)
                return IO_NORMAL;
            return IO_ERROR;
        }
        
        if ((stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
            return MCU_dofakewrite(stream -> buffer, stream -> len, ptr, size, n);
        
        if (fwrite(ptr, size, n, stream->fptr) != n)
            return IO_ERROR;
        return IO_NORMAL;
#endif /* MCS_write_dsk_mac */
        if (fwrite(p_buffer, 1, p_length, m_stream) != p_length)            
            return false;
        
        return true;
	}
    
    virtual bool IsExhausted(void)
    {
        if (m_is_eof)
            return true;
        
        return false;
    }
	
	virtual bool Seek(int64_t offset, int p_dir)
	{
#ifdef /* MCS_seek_cur_dsk_mac */ LEGACY_SYSTEM
	// MW-2009-06-25: If this is a custom stream, call the appropriate callback.
	// MW-2009-06-30: Refactored to common implementation in mcio.cpp.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_seek_cur(stream, offset);

	/* seek to offset from the current file mark */
	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->ioptr + offset);
	else
		if (fseeko(stream->fptr, offset, SEEK_CUR) != 0)
			return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_seek_cur_dsk_mac */
#ifdef /* MCS_seek_set_dsk_mac */ LEGACY_SYSTEM
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_seek_set(stream, offset);
	
	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->buffer + offset);
	else
		if (fseeko(stream->fptr, offset, SEEK_SET) != 0)
			return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_seek_set_dsk_mac */
#ifdef /* MCS_seek_end_dsk_mac */ LEGACY_SYSTEM
    /* seek to offset from the end of the file */
	if (stream->fptr == NULL)
		IO_set_stream(stream, stream->buffer + stream->len + offset);
	else
		if (fseeko(stream->fptr, offset, SEEK_END) != 0)
			return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_seek_end_dsk_mac */
        // TODO Add MCSystemFileHandle::SetStream(char *newptr) ?
		return fseeko(m_stream, offset, p_dir < 0 ? SEEK_END : (p_dir > 0 ? SEEK_SET : SEEK_CUR)) == 0;
	}
	
	virtual bool Truncate(void)
	{
#ifdef /* MCS_trunc_dsk_mac */ LEGACY_SYSTEM
    
	if (ftruncate(fileno(stream->fptr), ftell(stream->fptr)))
		return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_trunc_dsk_mac */
		return ftruncate(fileno(m_stream), ftell(m_stream)) == 0;
	}
	
	virtual bool Sync(void)
	{
#ifdef /* MCS_sync_dsk_mac */ LEGACY_SYSTEM
	if (stream->fptr != NULL)
	{
		int4 pos = ftello(stream->fptr);
		if (fseek(stream->fptr, pos, SEEK_SET) != 0)
			return IO_ERROR;
	}
	return IO_NORMAL;
#endif /* MCS_sync_dsk_mac */
        if (m_stream != NULL)
        {
            int64_t t_pos;
            t_pos = ftello(m_stream);
            return fseeko(m_stream, t_pos, SEEK_SET) == 0;
        }
        return true;
	}
	
	virtual bool Flush(void)
	{
#ifdef /* MCS_flush_dsk_mac */ LEGACY_SYSTEM
    //flush file buffer
	if (stream->fptr != NULL)
		if (fflush(stream->fptr))
			return IO_ERROR;
	return IO_NORMAL;
#endif /* MCS_flush_dsk_mac */
        if (m_stream != NULL)
            return fflush(m_stream) == 0;
        
        return true;
	}
	
	virtual bool PutBack(char p_char)
	{
#ifdef /* MCS_putback_dsk_mac */ LEGACY_SYSTEM
	if (stream -> serialIn != 0 || stream -> fptr == NULL)
		return MCS_seek_cur(stream, -1);
	
	if (ungetc(c, stream -> fptr) != c)
		return IO_ERROR;
		
	return IO_NORMAL;
#endif /* MCS_putback_dsk_mac */
        if (m_stream == NULL)
            return Seek(-1, 0);
        
        if (ungetc(p_char, m_stream) != p_char)
            return false;
        
        return true;
	}
	
	virtual int64_t Tell(void)
	{
#ifdef /* MCS_tell_dsk_mac */ LEGACY_SYSTEM
	// MW-2009-06-30: If this is a custom stream, call the appropriate callback.
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_tell(stream);

	if (stream->fptr != NULL)
		return ftello(stream->fptr);
	else
		return stream->ioptr - stream->buffer;
#endif /* MCS_tell_dsk_mac */
		return ftello(m_stream);
	}
	
	virtual int64_t GetFileSize(void)
	{
#ifdef /* MCS_fsize_dsk_mac */ LEGACY_SYSTEM
	if ((stream -> flags & IO_FAKECUSTOM) == IO_FAKECUSTOM)
		return MCS_fake_fsize(stream);

	if (stream->flags & IO_FAKE)
		return stream->len;

	// get file size of an Opened file
	struct stat buf;
	if (stream->fptr == NULL)
		return stream->len;
	int fd = fileno(stream->fptr);
	if (fstat(fd, (struct stat *)&buf))
		return 0;
	return buf.st_size;
#endif /* MCS_fsize_dsk_mac */
		struct stat t_info;
		if (fstat(fileno(m_stream), &t_info) != 0)
			return 0;
		return t_info . st_size;
	}
	
	virtual void *GetFilePointer(void)
	{
		return m_stream;
	}
	
	FILE *GetStream(void)
	{
		return m_stream;
	}
    
    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        return false;
    }
	
private:
	FILE *m_stream;
    bool m_is_eof;
};

class MCSerialPortFileHandle: public MCSystemFileHandle
{
public:
    
    MCSerialPortFileHandle(int p_serial_port)
    {
        m_serial_port = p_serial_port;
        m_cur_pos = 0;
        m_is_eof = false;
    }
    
	virtual void Close(void)
    {
        close(m_serial_port);
        delete this;
    }
    
    // Returns true if an attempt has been made to read past the end of the
    // stream.
    virtual bool IsExhausted(void)
    {
        return m_is_eof;
    }
    
    virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
    {
        SInt32 sint_toread = (SInt32) p_length;
        
        if ((errno = FSRead(m_serial_port, &sint_toread, p_buffer)) != noErr)
            return false;
        
        if (sint_toread < p_length)
        {
            m_is_eof = true;
        }
        
        r_read = sint_toread;
        return stat;
#if 0
        // FSRead is deprecated since OSX 10.4, FSReaFork since OSX 10.8, update to read(int fd, size_t size, void* ptr) on the same fashion
        // it was done for MCStdioFileHandle? This would allow interruption handling and such as well
        uint4 nread;
        
        // MW-2010-08-26: Taken from the Linux source, this changes the previous code
        //   to take into account pipes and such.
        char *sptr = (char *)p_buffer;
        uint4 toread = p_length;
        uint4 offset = 0;
        errno = 0;
        r_read = 0;
        while ((nread = read(m_serial_port, &sptr[offset], toread)) != toread)
        {
            if (nread == -1) //error
            {                
                if (errno == EAGAIN)
                    return false;
                
                if (errno == EINTR)
                {
                    // Do we really want to try again?
                    // When an error occurs, whether the file pointer has moved is unspecified...
                    continue;
                }
                else
                    return false;
            }
            else if (nread == 0) // EOF encountered
            {
                m_is_eof = true;
                return true;
            }
            
            m_is_eof = false;
            offset += nread;
            r_read = offset;
            m_cur_pos += offset;
        }
        
        m_is_eof = false;
        r_read = offset + nread;
        
        m_cur_pos += nread;
        return true;
#endif // 0
    }
    
	virtual bool Write(const void *p_buffer, uint32_t p_length)
    {
        uint4 t_count = p_length;
        errno = FSWrite(m_serial_port, (long*)&t_count, p_buffer);
        
#if 0
        // Same here, FSWrite is deprecated since OSX 10.4, FSWriteFork deprecated since OSX 10.8, update to write() as suggested on developer.apple.com?
        uint32_t t_count;
        t_count = write(m_serial_port, p_buffer, p_length);
#endif // 0
        
        if (errno != noErr || t_count != p_length)
            return false;
        
        m_cur_pos += t_count;
        return true;
    }
    
	virtual bool Seek(int64_t p_offset, int p_dir)
    {
        off_t t_new_pos;
        t_new_pos = lseek(m_serial_port, p_offset, p_dir < 0 ? SEEK_END : (p_dir > 0 ? SEEK_SET : SEEK_CUR));
        
        if (t_new_pos == (off_t)-1)
            return false;
        
        m_cur_pos = t_new_pos;
        return true;
    }
	
	virtual bool Truncate(void)
    {
        return ftruncate(m_serial_port, m_cur_pos) != -1;
    }
    
	virtual bool Sync(void)
    {
        return fsync(m_serial_port) == 0;
    }
    
	virtual bool Flush(void)
    {
        return fsync(m_serial_port) == 0; // fsync flushes the data, same as sync?
    }
	
	virtual bool PutBack(char p_char)
    {
        return Seek(-1, 0);
    }
	
	virtual int64_t Tell(void)
    {
        return m_cur_pos;
    }
	
	virtual void *GetFilePointer(void)
    {
        return NULL;
    }
    
	virtual int64_t GetFileSize(void)
    {
        struct stat64 t_stat;
        
        if (fstat64(m_serial_port, &t_stat) != 0)
            return 0;
        
        return t_stat.st_size;
    }
    
    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        return false;
    }
    
private:
	int m_serial_port;  //serial port Input reference number
    bool m_is_eof;
    off_t m_cur_pos; // current position
};

struct MCMacSystemService: public MCMacSystemServiceInterface//, public MCMacDesktop
{
    virtual bool SetResource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_id, MCStringRef p_name, MCStringRef p_flags, MCStringRef p_value, MCStringRef& r_error)
    {
#ifdef /* MCS_setresource_dsk_mac */ LEGACY_SYSTEM
        short newflags = 0; // parse up the attributes
        if (strlen(attrib) != 0)
        {
            const char *sptr = attrib;
            do
            {
                switch (*sptr++)
                {
                    case 'S':
                    case 's':
                        newflags |= resSysHeap;
                        break;
                    case 'U':
                    case 'u':
                        newflags |= resPurgeable;
                        break;
                    case 'L':
                    case 'l':
                        newflags |= resLocked;
                        break;
                    case 'P':
                    case 'p':
                        newflags |= resProtected;
                        break;
                    case 'R':
                    case 'r':
                        newflags |= resPreload;
                        break;
                    case 'C':
                    case 'c':
                        newflags |= resChanged;
                        break;
                }
            }
            while (*sptr);
        }
        else
            newflags |= resChanged;
        
        ResType rtype;
        memcpy((char *)&rtype, type, 4);
        // MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
        rtype = (ResType)MCSwapInt32HostToNetwork(rtype);
        short rid = 0;
        if (strlen(id) != 0)
        {
            const char *eptr;
            rid = (short)strtol(id, (char **)&eptr, 10);
        }
        short resFileRefNum; //open resource fork for read and write permission
        const char *t_open_res_error;
        t_open_res_error = MCS_openresourcefile_with_path(resourcefile, fsRdWrPerm, true, &resFileRefNum); // RESFILE
        if (t_open_res_error != NULL)
        {
            MCresult -> sets(t_open_res_error);
            return;
        }
        
        Handle rh = NULL;
        if (rid != 0)
            rh = Get1Resource(rtype, rid);
        else
        {
            char *whichres = strclone(name);
            unsigned char *rname = c2pstr(whichres); //resource name in Pascal
            rh = Get1NamedResource(rtype, rname);
            delete whichres;
        }
        
        Str255 newname;
        strcpy((char *)newname, name);
        c2pstr((char *)newname);
        if (rh != NULL)
        {
            SInt16 tid;
            ResType ttype;
            Str255 tname;
            GetResInfo(rh, &tid, &ttype, tname);
            if (strlen(name) == 0)
                pStrcpy(newname, tname);
            else
                if (strlen(id) == 0)
                    rid = tid;
            if (strlen(attrib) == 0)
                newflags = GetResAttrs(rh) | resChanged;
            SetResAttrs(rh, 0); // override protect flag
            RemoveResource(rh);
            DisposeHandle(rh);
        }
        if (rid == 0)
            rid = UniqueID(rtype);
        
        uint4 len = s.getlength();
        rh = NewHandle(len);
        
        if (rh == NULL)
            MCresult->sets("can't create resource handle");
        else
        {
            memcpy(*rh, s.getstring(), len);
            AddResource(rh, rtype, rid, newname);
            if ((errno = ResError()) != noErr)
            {
                DisposeHandle(rh);
                MCresult->sets("can't add resource");
            }
            else
                SetResAttrs(rh, newflags);
        }
        MCS_closeresourcefile(resFileRefNum);
#endif /* MCS_setresource_dsk_mac */
        short newflags = 0; // parse up the attributes
        if (MCStringGetLength(p_flags) != 0)
        {
            const char *sptr = MCStringGetCString(p_flags);
            do
            {
                switch (*sptr++)
                {
                    case 'S':
                    case 's':
                        newflags |= resSysHeap;
                        break;
                    case 'U':
                    case 'u':
                        newflags |= resPurgeable;
                        break;
                    case 'L':
                    case 'l':
                        newflags |= resLocked;
                        break;
                    case 'P':
                    case 'p':
                        newflags |= resProtected;
                        break;
                    case 'R':
                    case 'r':
                        newflags |= resPreload;
                        break;
                    case 'C':
                    case 'c':
                        newflags |= resChanged;
                        break;
                }
            }
            while (*sptr);
        }
        else
            newflags |= resChanged;
        
        ResType rtype;
        memcpy((char *)&rtype, MCStringGetCString(p_type), 4);
        // MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
        rtype = (ResType)MCSwapInt32HostToNetwork(rtype);
        short rid = 0;
        if (MCStringGetLength(p_id) != 0)
        {
            const char *eptr;
            rid = (short)strtol(MCStringGetCString(p_id), (char **)&eptr, 10);
        }
        SInt16 resFileRefNum; //open resource fork for read and write permission
        if (!MCS_mac_openresourcefile_with_path(p_source, fsRdWrPerm, true, resFileRefNum, r_error))
        {
            return MCresult -> setvalueref(r_error);
        }
        
        
        Handle rh = NULL;
        if (rid != 0)
            rh = Get1Resource(rtype, rid);
        else
        {
            char *whichres = strclone(MCStringGetCString(p_name));
            unsigned char *rname = c2pstr(whichres); //resource name in Pascal
            rh = Get1NamedResource(rtype, rname);
            delete whichres;
        }
        
        Str255 newname;
        strcpy((char *)newname, MCStringGetCString(p_name));
        c2pstr((char *)newname);
        if (rh != NULL)
        {
            SInt16 tid;
            ResType ttype;
            Str255 tname;
            GetResInfo(rh, &tid, &ttype, tname);
            if (MCStringGetLength(p_name) == 0)
                pStrcpy(newname, tname);
            else
                if (MCStringGetLength(p_id) == 0)
                    rid = tid;
            if (MCStringGetLength(p_flags) == 0)
                newflags = GetResAttrs(rh) | resChanged;
            SetResAttrs(rh, 0); // override protect flag
            RemoveResource(rh);
            DisposeHandle(rh);
        }
        if (rid == 0)
            rid = UniqueID(rtype);
        
        uint4 len = MCStringGetLength(p_value);
        
        rh = NewHandle(len);
        if (rh == NULL)
        {
            MCresult -> sets("can't create resource handle");
        }
        else
        {
            memcpy(*rh, MCStringGetCString(p_value), len);
            AddResource(rh, rtype, rid, newname);
            if ((errno = ResError()) != noErr)
            {
                DisposeHandle(rh);
                MCresult -> sets("can't add resource");
            }
            else
                SetResAttrs(rh, newflags);
        }
        CloseResFile(resFileRefNum);
        
        if (!MCresult -> isclear())
        {
            return MCStringCopy((MCStringRef)MCresult -> getvalueref(), r_error);
        }
        
        return true;
    }
    
    virtual bool GetResource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_value, MCStringRef& r_error)
    {
#ifdef /* MCS_getresource_dsk_mac */ LEGACY_SYSTEM
	short resFileRefNum;
	const char *t_open_res_error;
	t_open_res_error = MCS_openresourcefile_with_path(resourcefile, fsRdPerm, true, &resFileRefNum); // RESFILE
	if (t_open_res_error != NULL)
	{	
		MCresult -> sets(t_open_res_error);
		return;
	}

	ResType rtype;
	memcpy((char *)&rtype, restype, 4);
	// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
	rtype = (ResType)MCSwapInt32HostToNetwork(rtype);

	/* test to see if "name" is a resource name or an id */
	char *whichres = strclone(name);
	const char *eptr = (char *)name;
	long rid = strtol(whichres, (char **)&eptr, 10);

	unsigned char *rname;
	Handle rh = NULL; //resource handle
	if (eptr == whichres)
	{  /* conversion failed, so 'name' is resource name*/
		rname = c2pstr((char *)whichres); //resource name in Pascal
		rh = Get1NamedResource(rtype, rname);
	}
	else //we got an resrouce id, the 'name' specifies an resource id
		rh = Get1Resource(rtype, rid);
	delete whichres;

	if (rh == NULL)
	{
		errno = ResError();
		MCresult->sets("can't find specified resource");
		MCS_closeresourcefile(resFileRefNum);
		return;
	}
	//getting the the resource's size throuth the resource handle
	int4 resLength = GetHandleSize(rh);
	if (resLength <= 0)
	{
		MCresult->sets("can't get resouce length.");
		MCS_closeresourcefile(resFileRefNum);
		return;
	}
	// store the resource into ep and return
	ep.copysvalue((const char *)*rh, resLength);
	MCresult->clear();
	MCS_closeresourcefile(resFileRefNum);

#endif /* MCS_getresource_dsk_mac */
        SInt16 resFileRefNum;
        Handle rh = NULL; //resource handle
        
        if (!MCS_mac_openresourcefile_with_path(p_source, fsRdPerm, true, resFileRefNum, r_error))
        {
            return MCresult -> setvalueref(r_error);
        }
        
        ResType rtype;
        memcpy((char *)&rtype, MCStringGetCString(p_type), 4);
        // MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
        rtype = (ResType)MCSwapInt32HostToNetwork(rtype);
        
        /* test to see if "name" is a resource name or an id */
        char *whichres = strclone(MCStringGetCString(p_name));
        const char *eptr = MCStringGetCString(p_name);
        long rid = strtol(whichres, (char **)&eptr, 10);
        
        if (eptr == whichres)
        {  /* conversion failed, so 'name' is resource name*/
            unsigned char *rname;
            rname = c2pstr((char *)whichres); //resource name in Pascal
            rh = Get1NamedResource(rtype, rname);
        }
        else //we got an resrouce id, the 'name' specifies an resource id
            rh = Get1Resource(rtype, rid);
        delete[] whichres;
        
        if (rh == NULL)
        {
            errno = ResError();
            CloseResFile(resFileRefNum);
            MCresult -> sets("can't find specified resource");
        }
        
        bool t_success = true;
        if (MCresult -> isclear())
        {
            //getting the the resource's size throuth the resource handle
            int4 resLength = GetHandleSize(rh);
            if (resLength <= 0)
            {
                MCresult -> sets("can't get resouce length.");
            }
            else
                t_success = MCStringCreateWithNativeChars((char_t*)(*rh), resLength, r_value);
            
            CloseResFile(resFileRefNum);
        }
        
        if (!MCresult -> isclear())
        {
            return MCStringCopy((MCStringRef)MCresult->getvalueref(), r_error);
        }
        
        return t_success;
    }
    
    virtual bool GetResources(MCStringRef p_source, MCStringRef p_type, MCListRef& r_list, MCStringRef& r_error)
    {
#ifdef /* MCS_getresources_dsk_mac */ LEGACY_SYSTEM
    /* get resources from the resource fork of file 'path',
	   * if resource type is not empty, only resources of the specified type
	   * are listed. otherwise lists all resources from the
	   * resource fork.					    */

	MCAutoResourceFileHandle resFileRefNum;
	if (!MCS_openresourcefile_with_path(p_source, fsRdPerm, true, &resFileRefNum, r_error))
		return false;
	if (r_error != nil)
		return true;
	
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	SetResLoad(False);
	//since most recently opened file is place on the top of the search
	//path, no need to call UseResFile() to set this resource file as
	//the current file
	
	bool t_success = true;
	errno = noErr;
	
	ResType rtype, type;
	if (p_type != nil)
	{ //get the resource info specified by the resource type
		// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
		rtype = MCSwapInt32HostToNetwork(*(uint32_t*)MCStringGetNativeCharPtr(p_type));
		t_success = getResourceInfo(*t_list, rtype);
	}
	else
	{               //rtype is NULL, return All the resources
		short typeCount = Count1Types(); //find out how many resource type there is
		if (ResError() != noErr)
		{
			errno = ResError();
			t_success = false;
		}
		for (uindex_t i = 1; t_success && i <= typeCount; i++)
		{
			Get1IndType(&type, i);
			if (ResError() != noErr || type == 0)
				continue;
			t_success = getResourceInfo(*t_list, type);
		}
	}
	SetResLoad(True);
	
	if (t_success)
	{
		return MCListCopy(*t_list, r_resources);
	}
	else if (errno != noErr)
	{
		r_resources = MCValueRetain(kMCEmptyList);
		r_error = MCValueRetain(kMCEmptyString);
		return true;
	}
	
	return false;
#endif /* MCS_getresources_dsk_mac */
        /* get resources from the resource fork of file 'path',
         * if resource type is not empty, only resources of the specified type
         * are listed. otherwise lists all resources from the
         * resource fork.					    */
        
        MCAutoResourceFileHandle resFileRefNum;
        if (!MCS_mac_openresourcefile_with_path(p_source, fsRdPerm, true, &resFileRefNum, r_error))
            return false;
        if (r_error != nil)
            return true;
        
        MCAutoListRef t_list;
        if (!MCListCreateMutable('\n', &t_list))
            return false;
        
        SetResLoad(False);
        //since most recently opened file is place on the top of the search
        //path, no need to call UseResFile() to set this resource file as
        //the current file
        
        bool t_success = true;
        errno = noErr;
        
        ResType rtype, type;
        if (p_type != nil)
        { //get the resource info specified by the resource type
            // MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
            rtype = MCSwapInt32HostToNetwork(*(uint32_t*)MCStringGetNativeCharPtr(p_type));
            t_success = getResourceInfo(*t_list, rtype);
        }
        else
        {               //rtype is NULL, return All the resources
            short typeCount = Count1Types(); //find out how many resource type there is
            if (ResError() != noErr)
            {
                errno = ResError();
                t_success = false;
            }
            for (uindex_t i = 1; t_success && i <= typeCount; i++)
            {
                Get1IndType(&type, i);
                if (ResError() != noErr || type == 0)
                    continue;
                t_success = getResourceInfo(*t_list, type);
            }
        }
        SetResLoad(True);
        
        if (t_success)
        {
            return MCListCopy(*t_list, r_list);
        }
        else if (errno != noErr)
        {
            r_list = MCValueRetain(kMCEmptyList);
            r_error = MCValueRetain(kMCEmptyString);
            return true;
        }
        
        return false;
    }
    
    
    /*************************************************************************
     * 'which' param can be an id or a name of a resource. If the dest       *
     * file does not have a  resource fork we will create one for it         *
     *************************************************************************/
    virtual bool CopyResource(MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type, MCStringRef p_name, MCStringRef p_newid, MCStringRef& r_error)
    {
#ifdef /* MCS_copyresource_dsk_mac */ LEGACY_SYSTEM
	short prev_res_file = CurResFile(); //save the current resource fork
	MCAutoResourceFileHandle srcFileRefNum, destFileRefNum;
	
	if (!MCS_openresourcefile_with_path(p_source, fsRdPerm, false, &srcFileRefNum, r_error)) // RESFILE
		return false;
	if (r_error != nil)
		return true;
		
	// exclusive read/write permission for the dest file
	// and do not set result if there is error
	if (!MCS_openresourcefile_with_path(p_dest, fsRdWrPerm, true, &destFileRefNum, r_error)) // RESFILE
		return false;
	if (r_error != nil)
		return true;
	
	UseResFile(*destFileRefNum);
	
	if (MCStringGetLength(p_type) != 4)
	{ 
		//copying the entire resource file
		short resTypeCount = Count1Types();
		short resCount;
		ResType resourceType;
		Handle hres;
		for (uindex_t i = 1; i <= resTypeCount; i++)
		{
			UseResFile(*srcFileRefNum);
			Get1IndType(&resourceType, i);
			resCount = Count1Resources(resourceType);
			Str255 rname;
			short id;
			ResType type;
			for (uindex_t j = 1; j <= resCount; j++)
			{
				UseResFile(*srcFileRefNum);
				hres = Get1IndResource(resourceType, j);
				if (hres != NULL)
				{
					GetResInfo(hres, &id, &type, rname);
					DetachResource(hres);
					UseResFile(*destFileRefNum);
					AddResource(hres, type, id, rname);
				}
			}	//loop through each res within each res type
		} //loop through each res type
		
		UseResFile(prev_res_file); //restore the original state
		return true;
	}
	
	//copy only one resource, specified either by id or name
	UseResFile(*srcFileRefNum); //set the source resource file as the current file
	
	ResType restype;
	// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
	restype = MCSwapInt32HostToNetwork(*(uint32_t*)MCStringGetNativeCharPtr(p_type));

	Str255 t_resname;
	
	const char *whichres = MCStringGetCString(p_name);
	const char *eptr = whichres;    /* find out whichres is a name or an id */
	
	long rid = strtol(whichres, (char **)&eptr, 10); // if can't convert, then the value
	// passed in is a resource name
	Boolean hasResName = False;
	Handle rh = NULL;
	
	if (eptr == whichres)
	{  /*did not do the conversion, use resource name */
		c2pstrcpy(t_resname, MCStringGetCString(p_name));
		rh = Get1NamedResource(restype, t_resname);
		hasResName = True;
	}
	else //we got an resource id
		rh = Get1Resource(restype, rid);
	if (rh == NULL || *rh == 0)
	{//bail out if resource handle is bad
		errno = ResError();
		
		UseResFile(prev_res_file); //restore to the original state
		return MCStringCreateWithCString("can't find the resource specified", r_error);
	}
	
	unsigned char resourceName[255];
	short srcID;        //let's get it's resource name.
	ResType srcType;
	if (!hasResName) //No name specified for the resource to be copied
		GetResInfo(rh, &srcID, &srcType, resourceName);
	
	//detach the src res file, and select the dest res file
	DetachResource(rh);
	UseResFile(*destFileRefNum);
	unsigned long newResID;
	if (p_newid == NULL)
		newResID = srcID; //use the resource id of the src file's resource
	else
		newResID = strtoul(MCStringGetCString(p_newid), (char **)&eptr, 10); //use the id passed in
	
	//delete the resource by id to be copied in the destination file, if it existed
	Handle rhandle = Get1Resource(restype, newResID);
	if (rhandle != NULL && ResError() != resNotFound)
		RemoveResource(rhandle);
	
	//now, let's copy the resource to the dest file
	if (!hasResName)
		AddResource(rh, restype, (short)newResID, (unsigned char*)resourceName);
	else
		AddResource(rh, restype, (short)newResID, t_resname);
	//errno = ResError();//if errno == 0 means O.K.
	OSErr t_os_error = ResError();
	
	UseResFile(prev_res_file); //restore to the original state
	
	return true;
#endif /* MCS_copyresource_dsk_mac */
        short prev_res_file = CurResFile(); //save the current resource fork
        MCAutoResourceFileHandle srcFileRefNum, destFileRefNum;
        
        if (!MCS_mac_openresourcefile_with_path(p_source, fsRdPerm, false, &srcFileRefNum, r_error)) // RESFILE
            return false;
        if (r_error != nil)
            return true;
		
        // exclusive read/write permission for the dest file
        // and do not set result if there is error
        if (!MCS_mac_openresourcefile_with_path(p_dest, fsRdWrPerm, true, &destFileRefNum, r_error)) // RESFILE
            return false;
        if (r_error != nil)
            return true;
        
        UseResFile(*destFileRefNum);
        
        if (MCStringGetLength(p_type) != 4)
        {
            //copying the entire resource file
            short resTypeCount = Count1Types();
            short resCount;
            ResType resourceType;
            Handle hres;
            for (uindex_t i = 1; i <= resTypeCount; i++)
            {
                UseResFile(*srcFileRefNum);
                Get1IndType(&resourceType, i);
                resCount = Count1Resources(resourceType);
                Str255 rname;
                short id;
                ResType type;
                for (uindex_t j = 1; j <= resCount; j++)
                {
                    UseResFile(*srcFileRefNum);
                    hres = Get1IndResource(resourceType, j);
                    if (hres != NULL)
                    {
                        GetResInfo(hres, &id, &type, rname);
                        DetachResource(hres);
                        UseResFile(*destFileRefNum);
                        AddResource(hres, type, id, rname);
                    }
                }	//loop through each res within each res type
            } //loop through each res type
            
            UseResFile(prev_res_file); //restore the original state
            return true;
        }
        
        //copy only one resource, specified either by id or name
        UseResFile(*srcFileRefNum); //set the source resource file as the current file
        
        ResType restype;
        // MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
        restype = MCSwapInt32HostToNetwork(*(uint32_t*)MCStringGetNativeCharPtr(p_type));
        
        Str255 t_resname;
        
        const char *whichres = MCStringGetCString(p_name);
        const char *eptr = whichres;    /* find out whichres is a name or an id */
        
        long rid = strtol(whichres, (char **)&eptr, 10); // if can't convert, then the value
        // passed in is a resource name
        Boolean hasResName = False;
        Handle rh = NULL;
        
        if (eptr == whichres)
        {  /*did not do the conversion, use resource name */
            c2pstrcpy(t_resname, MCStringGetCString(p_name));
            rh = Get1NamedResource(restype, t_resname);
            hasResName = True;
        }
        else //we got an resource id
            rh = Get1Resource(restype, rid);
        if (rh == NULL || *rh == 0)
        {//bail out if resource handle is bad
            errno = ResError();
            
            UseResFile(prev_res_file); //restore to the original state
            return MCStringCreateWithCString("can't find the resource specified", r_error);
        }
        
        unsigned char resourceName[255];
        short srcID;        //let's get it's resource name.
        ResType srcType;
        if (!hasResName) //No name specified for the resource to be copied
            GetResInfo(rh, &srcID, &srcType, resourceName);
        
        //detach the src res file, and select the dest res file
        DetachResource(rh);
        UseResFile(*destFileRefNum);
        unsigned long newResID;
        if (p_newid == NULL)
            newResID = srcID; //use the resource id of the src file's resource
        else
            newResID = strtoul(MCStringGetCString(p_newid), (char **)&eptr, 10); //use the id passed in
        
        //delete the resource by id to be copied in the destination file, if it existed
        Handle rhandle = Get1Resource(restype, newResID);
        if (rhandle != NULL && ResError() != resNotFound)
            RemoveResource(rhandle);
        
        //now, let's copy the resource to the dest file
        if (!hasResName)
            AddResource(rh, restype, (short)newResID, (unsigned char*)resourceName);
        else
            AddResource(rh, restype, (short)newResID, t_resname);
        //errno = ResError();//if errno == 0 means O.K.
        OSErr t_os_error = ResError();
        
        UseResFile(prev_res_file); //restore to the original state
        
        return true;
    }
    
    virtual bool DeleteResource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_error)
    {
#ifdef /* MCS_deleteresource_dsk_mac */ LEGACY_SYSTEM
	ResType restype;
	short rfRefNum;
	memcpy((char *)&restype, rtype, 4); /* let's get the resource type first */
	// MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
	restype = (ResType)MCSwapInt32HostToNetwork(restype);

	const char *t_open_res_error;
	t_open_res_error = MCS_openresourcefile_with_path(resourcefile, fsRdWrPerm, true, &rfRefNum); // RESFILE
	if (t_open_res_error != NULL)
	{
		MCresult -> sets(t_open_res_error);
		return;
	}

	Handle rh = NULL;
	const char *eptr = (char *)which;     /* find out if we got a name or an id */
	long rid = strtol(which, (char **)&eptr, 10);
	if (eptr == which)
	{     /* did not do conversion, so use resource name */
		unsigned char *pname = c2pstr((char *)which);
		rh = Get1NamedResource(restype, pname);
	}
	else                  /* 'which' param is an resrouce id */
		rh = Get1Resource(restype, rid);
	if (rh == NULL)
		MCresult->sets("can't find the resource specified");
	else
	{
		SetResAttrs(rh, 0); // override protect flag
		RemoveResource(rh);
		if ((errno = ResError()) != noErr)
			MCresult->sets("can't remove the resource specified");
		DisposeHandle(rh);
	}
	
	MCS_closeresourcefile(rfRefNum);
#endif /* MCS_deleteresource_dsk_mac */
        ResType restype;
        short rfRefNum;
        memcpy((char *)&restype, MCStringGetCString(p_type), 4); /* let's get the resource type first */
        // MH-2007-03-22: [[ Bug 4267 ]] Endianness not dealt with correctly in Mac OS resource handling functions.
        restype = (ResType)MCSwapInt32HostToNetwork(restype);
        
        MCAutoStringRef t_error;
        if (!MCS_mac_openresourcefile_with_path(p_source, fsRdWrPerm, true, rfRefNum, &t_error))
        {
            return MCresult -> setvalueref(*t_error);
        }
        
        Handle rh = NULL;
        const char *eptr = MCStringGetCString(p_name);     /* find out if we got a name or an id */
        long rid = strtol(MCStringGetCString(p_name), (char **)&eptr, 10);
        if (eptr == MCStringGetCString(p_name))
        {     /* did not do conversion, so use resource name */
            char* tname = strclone(MCStringGetCString(p_name));
            unsigned char *pname = c2pstr(tname);
            rh = Get1NamedResource(restype, pname);
            delete[] tname;
        }
        else                  /* 'which' param is an resrouce id */
            rh = Get1Resource(restype, rid);
        if (rh == NULL)
            MCresult->sets("can't find the resource specified");
        else
        {
            SetResAttrs(rh, 0); // override protect flag
            RemoveResource(rh);
            if ((errno = ResError()) != noErr)
                MCresult->sets("can't remove the resource specified");
            DisposeHandle(rh);
        }
        
        CloseResFile(rfRefNum);
        if (MCresult->isclear())
            return true;
        
        MCAssert(MCValueGetTypeCode(MCresult->getvalueref()) == kMCValueTypeCodeString);
        
        return MCStringCopy((MCStringRef)MCresult->getvalueref(), r_error);
    }
    
    virtual void CopyResourceFork(MCStringRef p_source, MCStringRef p_destination)
    {
#ifdef /* MCS_copyresourcefork_dsk_mac */ LEGACY_SYSTEM
	MCAutoStringRef t_error;
	
	SInt16 t_source_ref;
	bool t_source_fork_opened;
	t_source_fork_opened = false;

	MCS_openresourcefork_with_path(p_source, fsRdPerm, false, &t_source_ref, &t_error);
    if (MCStringGetLength(*t_error) == 0)
		t_source_fork_opened = true;
	
	SInt16 t_dest_ref;
	bool t_dest_fork_opened;
	t_dest_fork_opened = false;

	if (t_source_fork_opened)
    {
        MCS_openresourcefork_with_path(p_destination, fsWrPerm, true, &t_dest_ref, &t_error);
        if (MCStringGetLength(*t_error))
            t_dest_fork_opened = true;
    }

	// In block sizes of 1k, copy over the data from source to destination..
	char *t_buffer = new char[65536];
	if (t_source_fork_opened && t_dest_fork_opened)
	{
		OSErr t_os_read_error, t_os_write_error;
		do {
			ByteCount t_actual_read, t_actual_write;
			t_os_read_error = FSReadFork(t_source_ref, fsFromMark, 0, 65536, t_buffer, &t_actual_read);
			if (t_os_read_error == noErr || t_os_read_error == eofErr)
			{
				t_os_write_error = FSWriteFork(t_dest_ref, fsFromMark, 0, t_actual_read, t_buffer, &t_actual_write);
				if (t_os_write_error != noErr || t_actual_write != t_actual_read)
					/* UNCHECKED */ MCStringCreateWithCString("can't copy resource", &t_error);
			}
		} while(MCStringGetLength(*t_error) && t_os_read_error == noErr);
	}
	
	delete t_buffer;
	if (t_source_fork_opened)
		FSCloseFork(t_source_ref);
	if (t_dest_fork_opened)
		FSCloseFork(t_dest_ref);
#endif /* MCS_copyresourcefork_dsk_mac */
        MCAutoStringRef t_error;
        
        SInt16 t_source_ref;
        bool t_source_fork_opened;
        t_source_fork_opened = false;
        
        MCS_mac_openresourcefork_with_path(p_source, fsRdPerm, false, &t_source_ref, &t_error);
        if (*t_error != nil)
            t_source_fork_opened = true;
        
        SInt16 t_dest_ref;
        bool t_dest_fork_opened;
        t_dest_fork_opened = false;
        
        if (t_source_fork_opened)
        {
            MCS_mac_openresourcefork_with_path(p_destination, fsWrPerm, true, &t_dest_ref, &t_error);
            if (MCStringGetLength(*t_error))
                t_dest_fork_opened = true;
        }
        
        // In block sizes of 1k, copy over the data from source to destination..
        char *t_buffer = new char[65536];
        if (t_source_fork_opened && t_dest_fork_opened)
        {
            OSErr t_os_read_error, t_os_write_error;
            do {
                ByteCount t_actual_read, t_actual_write;
                t_os_read_error = FSReadFork(t_source_ref, fsFromMark, 0, 65536, t_buffer, &t_actual_read);
                if (t_os_read_error == noErr || t_os_read_error == eofErr)
                {
                    t_os_write_error = FSWriteFork(t_dest_ref, fsFromMark, 0, t_actual_read, t_buffer, &t_actual_write);
                    if (t_os_write_error != noErr || t_actual_write != t_actual_read)
					/* UNCHECKED */ MCStringCreateWithCString("can't copy resource", &t_error);
                }
            } while(MCStringGetLength(*t_error) && t_os_read_error == noErr);
        }
        
        delete[] t_buffer;
        if (t_source_fork_opened)
            FSCloseFork(t_source_ref);
        if (t_dest_fork_opened)
            FSCloseFork(t_dest_ref);
    }
    
    virtual void LoadResFile(MCStringRef p_filename, MCStringRef& r_data)
    {
#ifdef /* MCS_loadresfile_dsk_mac */ LEGACY_SYSTEM
	if (!MCSecureModeCanAccessDisk())
	{
		r_data = MCValueRetain(kMCEmptyString);
		MCresult->sets("can't open file");
		return;
	}
    
	MCAutoStringRef t_open_res_error_string;

	short fRefNum;
	MCS_openresourcefork_with_path(p_filename, fsRdPerm, false, &fRefNum, &t_open_res_error_string); // RESFORK

	const char *t_open_res_error = nil;
	if (MCStringGetLength(*t_open_res_error_string) != 0)
		t_open_res_error =  MCStringGetCString(*t_open_res_error_string);
	
	if (t_open_res_error != NULL)
	{
		MCresult -> sets(t_open_res_error);
		
		return;
	}
		
	//file mark should be pointing to 0 which is the begining of the file
	//let's get the end of file mark to determine the file size
	long fsize, toread;
	if ((errno = GetEOF(fRefNum, &fsize)) != noErr)
		MCresult->sets("can't get file size");
	else
	{
		toread = fsize;
		char *buffer;
		buffer = new char[fsize];
		if (buffer == NULL)
			MCresult->sets("can't create data buffer");
		else
		{
			errno = FSRead(fRefNum, &toread, buffer);
			if (toread != fsize) //did not read bytes as specified
				MCresult->sets("error reading file");
			else
			{
				/* UNCHECKED */ MCStringCreateWithNativeCharsAndRelease((char_t*)buffer, toread, r_data);
				MCresult->clear(False);
			}
		}
	}
	
	FSCloseFork(fRefNum);

#endif /* MCS_loadresfile_dsk_mac */
        if (!MCSecureModeCanAccessDisk())
        {
            r_data = MCValueRetain(kMCEmptyString);
            MCresult->sets("can't open file");
            return;
        }
        
        MCAutoStringRef t_open_res_error_string;
        
        short fRefNum;
        MCS_mac_openresourcefork_with_path(p_filename, fsRdPerm, false, &fRefNum, &t_open_res_error_string); // RESFORK
        
        const char *t_open_res_error = nil;
        if (MCStringGetLength(*t_open_res_error_string) != 0)
            t_open_res_error =  MCStringGetCString(*t_open_res_error_string);
        
        if (t_open_res_error != NULL)
        {
            MCresult -> sets(t_open_res_error);
            
            return;
        }
		
        //file mark should be pointing to 0 which is the begining of the file
        //let's get the end of file mark to determine the file size
        long fsize, toread;
        if ((errno = GetEOF(fRefNum, &fsize)) != noErr)
            MCresult->sets("can't get file size");
        else
        {
            toread = fsize;
            char *buffer;
            buffer = new char[fsize];
            if (buffer == NULL)
                MCresult->sets("can't create data buffer");
            else
            {
                errno = FSRead(fRefNum, &toread, buffer);
                if (toread != fsize) //did not read bytes as specified
                    MCresult->sets("error reading file");
                else
                {
                    /* UNCHECKED */ MCStringCreateWithNativeCharsAndRelease((char_t*)buffer, toread, r_data);
                    MCresult->clear(False);
                }
            }
        }
        
        FSCloseFork(fRefNum);
    }
    
    // MH-2007-04-02: [[ Bug 705 ]] resfile: URLs do not work with long filenames...
    virtual void SaveResFile(MCStringRef p_path, MCDataRef p_data)
    {
#ifdef /* MCS_saveresfile_dsk_mac */ LEGACY_SYSTEM
	MCAutoStringRef t_error;

	if (!MCSecureModeCanAccessDisk())
		/* UNCHECKED */ MCStringCreateWithCString("can't open file", &t_error);
	
	SInt16 t_fork_ref;
	bool t_fork_opened;
	t_fork_opened = false;
	
	if (MCStringGetLength(*t_error) == 0)
	{
		MCS_openresourcefork_with_path(p_path, fsWrPerm, true, &t_fork_ref, &t_error); // RESFORK
		if (MCStringGetLength(*t_error))
			t_fork_opened = true;
	}
	
	if (MCStringGetLength(*t_error) == 0)
	{
		OSErr t_os_error;
		ByteCount t_actual_count;
		t_os_error = FSWriteFork(t_fork_ref, fsFromStart, 0, MCDataGetLength(p_data), (const void *)MCDataGetBytePtr(p_data), &t_actual_count);
		if (t_os_error == noErr && t_actual_count == (ByteCount)MCDataGetLength(p_data))
			FSSetForkSize(t_fork_ref, fsFromStart, t_actual_count);
		else
			/* UNCHECKED */ MCStringCreateWithCString("error writing file", &t_error);
	}
	
	if (t_fork_opened)
		FSCloseFork(t_fork_ref);
	
	if (MCStringGetLength(*t_error) != 0)
		/* UNCHECKED */ MCresult -> setvalueref(*t_error);
	else
		MCresult -> clear(False);
#endif /* MCS_saveresfile_dsk_mac */
        MCAutoStringRef t_error;
        
        if (!MCSecureModeCanAccessDisk())
		/* UNCHECKED */ MCStringCreateWithCString("can't open file", &t_error);
        
        SInt16 t_fork_ref;
        bool t_fork_opened;
        t_fork_opened = false;
        
        if (*t_error != nil)
        {
            MCS_mac_openresourcefork_with_path(p_path, fsWrPerm, true, &t_fork_ref, &t_error); // RESFORK
            if (MCStringGetLength(*t_error))
                t_fork_opened = true;
        }
        
        if (*t_error != nil)
        {
            OSErr t_os_error;
            ByteCount t_actual_count;
            t_os_error = FSWriteFork(t_fork_ref, fsFromStart, 0, MCDataGetLength(p_data), (const void *)MCDataGetBytePtr(p_data), &t_actual_count);
            if (t_os_error == noErr && t_actual_count == (ByteCount)MCDataGetLength(p_data))
                FSSetForkSize(t_fork_ref, fsFromStart, t_actual_count);
            else
			/* UNCHECKED */ MCStringCreateWithCString("error writing file", &t_error);
        }
        
        if (t_fork_opened)
            FSCloseFork(t_fork_ref);
        
        if (MCStringGetLength(*t_error) != 0)
		/* UNCHECKED */ MCresult -> setvalueref(*t_error);
        else
            MCresult -> clear(False);
    }
    
    // MW-2006-08-05: Vetted for Endian issues
    virtual void Send(MCStringRef p_message, MCStringRef p_program, MCStringRef p_eventtype, Boolean p_reply)
    {
#ifdef /* MCS_send_dsk_mac */ LEGACY_SYSTEM
    //send "" to program "" with/without reply
	if (!MCSecureModeCheckAppleScript())
		return;
    
    
	AEAddressDesc receiver;
	errno = getDescFromAddress(program, &receiver);
	if (errno != noErr)
	{
		AEDisposeDesc(&receiver);
		MCresult->sets("no such program");
		return;
	}
	AppleEvent ae;
	if (eventtype == NULL)
		MCStringCreateWithCString("miscdosc", eventtype);
    
	AEEventClass ac;
	AEEventID aid;
	
	ac = FourCharCodeFromString(MCStringGetCString(eventtype));
	aid = FourCharCodeFromString(&MCStringGetCString(eventtype)[4]);
	
	AECreateAppleEvent(ac, aid, &receiver, kAutoGenerateReturnID,
	                   kAnyTransactionID, &ae);
	AEDisposeDesc(&receiver); //dispose of the receiver description record
	// if the ae message we are sending is 'odoc', 'pdoc' then
	// create a document descriptor of type fypeFSS for the document
	
	Boolean docmessage = False; //Is this message contains a document descriptor?
	AEDescList files_list, file_desc;
	AliasHandle the_alias;
    
	if (aid == 'odoc' || aid == 'pdoc')
	{
		FSSpec fspec;
		FSRef t_fsref;
		
		if (MCS_pathtoref(message, t_fsref) == noErr && MCS_mac_fsref_to_fsspec(&t_fsref, &fspec) == noErr)
		{
			AECreateList(NULL, 0, false, &files_list);
			NewAlias(NULL, &fspec, &the_alias);
			HLock((Handle)the_alias);
			AECreateDesc(typeAlias, (Ptr)(*the_alias),
			             GetHandleSize((Handle)the_alias), &file_desc);
			HUnlock((Handle) the_alias);
			AEPutDesc(&files_list, 0, &file_desc);
			AEPutParamDesc(&ae, keyDirectObject, &files_list);
			docmessage = True;
		}
	}
	//non document related massge, assume it's typeChar message
	if (!docmessage && MCStringGetLength(message))
		AEPutParamPtr(&ae, keyDirectObject, typeChar,
		              MCStringGetCString(message), MCStringGetLength(message));
    
	//Send the Apple event
	AppleEvent answer;
	if (reply == True)
		errno = AESend(&ae, &answer, kAEQueueReply, kAENormalPriority,
		               kAEDefaultTimeout, NULL, NULL); //no reply
	else
		errno = AESend(&ae, &answer, kAENoReply, kAENormalPriority,
		               kAEDefaultTimeout, NULL, NULL); //reply comes in event queue
	if (docmessage)
	{
		DisposeHandle((Handle)the_alias);
		AEDisposeDesc(&file_desc);
		AEDisposeDesc(&files_list);
		AEDisposeDesc(&file_desc);
	}
	AEDisposeDesc(&ae);
	if (errno != noErr)
	{
		char *buffer = new char[6 + I2L];
		sprintf(buffer, "error %d", errno);
		MCresult->copysvalue(buffer);
		delete buffer;
		return;
	}
	if (reply == True)
	{ /* wait for a reply in a loop.  The reply comes in
       from regular event handling loop
       and is handled by an Apple event handler*/
		real8 endtime = curtime + AETIMEOUT;
		while (True)
		{
			if (MCscreen->wait(READ_INTERVAL, False, True))
			{
				MCresult->sets("user interrupt");
				return;
			}
			if (curtime > endtime)
			{
				MCresult->sets("timeout");
				return;
			}
			if (AEanswerErr != NULL || AEanswerData != NULL)
				break;
		}
		if (AEanswerErr != NULL)
		{
			MCresult->copysvalue(AEanswerErr);
			delete AEanswerErr;
			AEanswerErr = NULL;
		}
		else
		{
			MCresult->copysvalue(AEanswerData);
			delete AEanswerData;
			AEanswerData = NULL;
		}
		AEDisposeDesc(&answer);
	}
	else
		MCresult->clear(False);
#endif /* MCS_send_dsk_mac */
        //send "" to program "" with/without reply
        if (!MCSecureModeCheckAppleScript())
            return;
        
        
        AEAddressDesc receiver;
        errno = getDescFromAddress(p_program, &receiver);
        if (errno != noErr)
        {
            AEDisposeDesc(&receiver);
            MCresult->sets("no such program");
            return;
        }
        AppleEvent ae;
        if (p_eventtype == NULL)
            MCStringCreateWithCString("miscdosc", p_eventtype);
        
        AEEventClass ac;
        AEEventID aid;
        
        ac = FourCharCodeFromString(MCStringGetCString(p_eventtype));
        aid = FourCharCodeFromString(&MCStringGetCString(p_eventtype)[4]);
        
        AECreateAppleEvent(ac, aid, &receiver, kAutoGenerateReturnID,
                           kAnyTransactionID, &ae);
        AEDisposeDesc(&receiver); //dispose of the receiver description record
        // if the ae message we are sending is 'odoc', 'pdoc' then
        // create a document descriptor of type fypeFSS for the document
        
        Boolean docmessage = False; //Is this message contains a document descriptor?
        AEDescList files_list, file_desc;
        AliasHandle the_alias;
        
        if (aid == 'odoc' || aid == 'pdoc')
        {
            FSSpec fspec;
            FSRef t_fsref;
            
            if (MCS_mac_pathtoref(p_message, t_fsref) == noErr && MCS_mac_fsref_to_fsspec(&t_fsref, &fspec) == noErr)
            {
                AECreateList(NULL, 0, false, &files_list);
                NewAlias(NULL, &fspec, &the_alias);
                HLock((Handle)the_alias);
                AECreateDesc(typeAlias, (Ptr)(*the_alias),
                             GetHandleSize((Handle)the_alias), &file_desc);
                HUnlock((Handle) the_alias);
                AEPutDesc(&files_list, 0, &file_desc);
                AEPutParamDesc(&ae, keyDirectObject, &files_list);
                docmessage = True;
            }
        }
        //non document related massge, assume it's typeChar message
        if (!docmessage && MCStringGetLength(p_message))
            AEPutParamPtr(&ae, keyDirectObject, typeChar,
                          MCStringGetCString(p_message), MCStringGetLength(p_message));
        
        //Send the Apple event
        AppleEvent answer;
        if (p_reply == True)
            errno = AESend(&ae, &answer, kAEQueueReply, kAENormalPriority,
                           kAEDefaultTimeout, NULL, NULL); //no reply
        else
            errno = AESend(&ae, &answer, kAENoReply, kAENormalPriority,
                           kAEDefaultTimeout, NULL, NULL); //reply comes in event queue
        if (docmessage)
        {
            DisposeHandle((Handle)the_alias);
            AEDisposeDesc(&file_desc);
            AEDisposeDesc(&files_list);
            AEDisposeDesc(&file_desc);
        }
        AEDisposeDesc(&ae);
        if (errno != noErr)
        {
            char *buffer = new char[6 + I2L];
            sprintf(buffer, "error %d", errno);
            MCresult->copysvalue(buffer);
            delete buffer;
            return;
        }
        if (p_reply == True)
        { /* wait for a reply in a loop.  The reply comes in
           from regular event handling loop
           and is handled by an Apple event handler*/
            real8 endtime = curtime + AETIMEOUT;
            while (True)
            {
                if (MCscreen->wait(READ_INTERVAL, False, True))
                {
                    MCresult->sets("user interrupt");
                    return;
                }
                if (curtime > endtime)
                {
                    MCresult->sets("timeout");
                    return;
                }
                if (AEanswerErr != NULL || AEanswerData != NULL)
                    break;
            }
            if (AEanswerErr != NULL)
            {
                MCresult->copysvalue(AEanswerErr);
                delete AEanswerErr;
                AEanswerErr = NULL;
            }
            else
            {
                MCresult->copysvalue(AEanswerData);
                delete AEanswerData;
                AEanswerData = NULL;
            }
            AEDisposeDesc(&answer);
        }
        else
            MCresult->clear(False);
    }
    
    // MW-2006-08-05: Vetted for Endian issues
    virtual void Reply(MCStringRef p_message, MCStringRef p_keyword, Boolean p_error)
    {
#ifdef /* MCS_reply_dsk_mac */ LEGACY_SYSTEM
	delete replymessage;
	replylength = message.getlength();
	replymessage = new char[replylength];
	memcpy(replymessage, message.getstring(), replylength);
    
	//at any one time only either keyword or error is set
	if (keyword != NULL)
	{
		replykeyword = FourCharCodeFromString(keyword);
	}
	else
	{
		if (error)
			replykeyword = 'errs';
		else
			replykeyword = '----';
	}
#endif /* MCS_reply_dsk_mac */
        delete[] replymessage;
        replylength = MCStringGetLength(p_message);
        replymessage = new char[replylength];
        memcpy(replymessage, MCStringGetCString(p_message), replylength);
        
        //at any one time only either keyword or error is set
        if (p_keyword != NULL)
        {
            replykeyword = FourCharCodeFromString(MCStringGetCString(p_keyword));
        }
        else
        {
            if (p_error)
                replykeyword = 'errs';
            else
                replykeyword = '----';
        }
    }
    
    // MW-2006-08-05: Vetted for Endian issues
    virtual void RequestAE(MCStringRef p_message, uint2 p_ae, MCStringRef& r_value)
    {
#ifdef /* MCS_request_ae_dsk_mac */ LEGACY_SYSTEM
	if (aePtr == NULL)
		return strdup("No current Apple event"); //as specified in HyperTalk
	errno = noErr;
    
	switch (ae)
	{
        case AE_CLASS:
		{
			char *aeclass;
			if ((errno = getAEAttributes(aePtr, keyEventClassAttr, aeclass)) == noErr)
				return aeclass;
			break;
		}
        case AE_DATA:
		{
			if (message.getlength() == 0)
			{ //no keyword, get event parameter(data)
				DescType rType;
				Size rSize;  //actual size returned
				/*first let's find out the size of incoming event data */
				
				// On Snow Leopard check for a coercion to a file list first as otherwise
				// we get a bad URL!
				if (MCmajorosversion >= 0x1060)
				{
					char *string = nil;
					uint4 length = 0;
					if (fetch_ae_as_fsref_list(string, length))
						return string;
				}
                
				if ((errno = AEGetParamPtr(aePtr, keyDirectObject, typeChar,
				                           &rType, NULL, 0, &rSize)) == noErr)
				{
					char *info = new char[rSize + 1]; //allocate enough buffer for data
					AEGetParamPtr(aePtr, keyDirectObject, typeChar,
					              &rType, info, rSize, &rSize); //retrive data now
					info[rSize] = '\0';
					return info;
				}
				else
				{
					char *string = nil;
					uint4 length = 0;
					if (fetch_ae_as_fsref_list(string, length))
						return string;
					return strdup("file list error");
				}
			}
			else
			{
				AEKeyword key;
				const char *keystring = message.getstring()
                + message.getlength() - sizeof(AEKeyword);
				key = FourCharCodeFromString(keystring);
				char *info;
                
				if (key == keyAddressAttr || key == keyEventClassAttr
                    || key == keyEventIDAttr || key == keyEventSourceAttr
                    || key == keyInteractLevelAttr || key == keyMissedKeywordAttr
                    || key == keyOptionalKeywordAttr || key == keyOriginalAddressAttr
                    || key == keyReturnIDAttr || key == keyTimeoutAttr
                    || key == keyTransactionIDAttr)
				{
					if ((errno = getAEAttributes(aePtr, key, info)) == noErr)
						return info;
				}
				else
				{
					if ((errno = getAEParams(aePtr, key, info)) == noErr)
						return info;
				}
			}
		}
            break;
        case AE_ID:
		{
			char *aeid;
			if ((errno = getAEAttributes(aePtr, keyEventIDAttr, aeid)) == noErr)
				return aeid;
			break;
		}
        case AE_RETURN_ID:
		{
			char *aerid;
			if ((errno = getAEAttributes(aePtr, keyReturnIDAttr, aerid)) == noErr)
				return aerid;
			break;
		}
        case AE_SENDER:
		{
			AEAddressDesc senderDesc;
			char *sender = new char[128];
			
			if ((errno = AEGetAttributeDesc(aePtr, keyOriginalAddressAttr,
			                                typeWildCard, &senderDesc)) == noErr)
			{
				errno = getAddressFromDesc(senderDesc, sender);
				AEDisposeDesc(&senderDesc);
				return sender;
			}
			delete sender;
			break;
		}
	}  /* end switch */
	if (errno == errAECoercionFail) //data could not display as text
		return strclone("unknown type");
	return strclone("not found");
#endif /* MCS_request_ae_dsk_mac */
        if (aePtr == NULL)
            /* UNCHECKED */ MCStringCreateWithCString("No current Apple event", r_value); //as specified in HyperTalk
        errno = noErr;
        
        switch (p_ae)
        {
            case AE_CLASS:
            {
                char *aeclass;
                if ((errno = getAEAttributes(aePtr, keyEventClassAttr, aeclass)) == noErr)
                    /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)aeclass, r_value);
                break;
            }
            case AE_DATA:
            {
                if (MCStringGetLength(p_message) == 0)
                { //no keyword, get event parameter(data)
                    DescType rType;
                    Size rSize;  //actual size returned
                    /*first let's find out the size of incoming event data */
                    
                    // On Snow Leopard check for a coercion to a file list first as otherwise
                    // we get a bad URL!
                    if (MCmajorosversion >= 0x1060)
                    {
                        char *string = nil;
                        uint4 length = 0;
                        if (fetch_ae_as_fsref_list(string, length))
                            /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)string, r_value);
                    }
                    
                    if ((errno = AEGetParamPtr(aePtr, keyDirectObject, typeChar,
                                               &rType, NULL, 0, &rSize)) == noErr)
                    {
                        char *info = new char[rSize + 1]; //allocate enough buffer for data
                        AEGetParamPtr(aePtr, keyDirectObject, typeChar,
                                      &rType, info, rSize, &rSize); //retrive data now
                        info[rSize] = '\0';
                        
                        /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)info, r_value);
                    }
                    else
                    {
                        char *string = nil;
                        uint4 length = 0;
                        if (fetch_ae_as_fsref_list(string, length))
                            /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)string, r_value);
                        /* UNCHECKED */ MCStringCreateWithCString("file list error", r_value);
                    }
                }
                else
                {
                    AEKeyword key;
                    const char *keystring = MCStringGetCString(p_message)
                        + MCStringGetLength(p_message) - sizeof(AEKeyword);
                    key = FourCharCodeFromString(keystring);
                    char *info;
                    
                    if (key == keyAddressAttr || key == keyEventClassAttr
                        || key == keyEventIDAttr || key == keyEventSourceAttr
                        || key == keyInteractLevelAttr || key == keyMissedKeywordAttr
                        || key == keyOptionalKeywordAttr || key == keyOriginalAddressAttr
                        || key == keyReturnIDAttr || key == keyTimeoutAttr
                        || key == keyTransactionIDAttr)
                    {
                        if ((errno = getAEAttributes(aePtr, key, info)) == noErr)
                            /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)info, r_value);
                    }
                    else
                    {
                        if ((errno = getAEParams(aePtr, key, info)) == noErr)
                            /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)info, r_value);
                    }
                }
            }
                break;
            case AE_ID:
            {
                char *aeid;
                if ((errno = getAEAttributes(aePtr, keyEventIDAttr, aeid)) == noErr)
                    /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)aeid, r_value);
                break;
            }
            case AE_RETURN_ID:
            {
                char *aerid;
                if ((errno = getAEAttributes(aePtr, keyReturnIDAttr, aerid)) == noErr)
                    /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)aerid, r_value);
                
                break;
            }
            case AE_SENDER:
            {
                AEAddressDesc senderDesc;
                char *sender = new char[128];
                
                if ((errno = AEGetAttributeDesc(aePtr, keyOriginalAddressAttr,
                                                typeWildCard, &senderDesc)) == noErr)
                {
                    errno = getAddressFromDesc(senderDesc, sender);
                    AEDisposeDesc(&senderDesc);
                    /* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t*)sender, r_value);
                }
                delete[] sender;
                break;
            }
        }  /* end switch */
        if (errno == errAECoercionFail) //data could not display as text
            /* UNCHECKED */ MCStringCreateWithCString("unknown type", r_value);
        
        /* UNCHECKED */ MCStringCreateWithCString("not found", r_value);
    }
    
    // MW-2006-08-05: Vetted for Endian issues
    virtual bool RequestProgram(MCStringRef p_message, MCStringRef p_program, MCStringRef& r_value)
    {
#ifdef /* MCS_request_program_dsk_mac */ LEGACY_SYSTEM
	AEAddressDesc receiver;
	errno = getDescFromAddress(program, &receiver);
	if (errno != noErr)
	{
		AEDisposeDesc(&receiver);
		MCresult->sets("no such program");
		r_value = MCValueRetain(kMCEmptyString);
        return;
	}
	AppleEvent ae;
	errno = AECreateAppleEvent('misc', 'eval', &receiver,
	                           kAutoGenerateReturnID, kAnyTransactionID, &ae);
	AEDisposeDesc(&receiver); //dispose of the receiver description record
	//add parameters to the Apple event
	AEPutParamPtr(&ae, keyDirectObject, typeChar,
	              MCStringGetCString(message), MCStringGetLength(message));
	//Send the Apple event
	AppleEvent answer;
	errno = AESend(&ae, &answer, kAEQueueReply, kAENormalPriority,
	               kAEDefaultTimeout, NULL, NULL); //no reply
	AEDisposeDesc(&ae);
	AEDisposeDesc(&answer);
	if (errno != noErr)
	{
		char *buffer = new char[6 + I2L];
		sprintf(buffer, "error %d", errno);
		MCresult->copysvalue(buffer);
		delete buffer;
        
        r_value = MCValueRetain(kMCEmptyString);
        return;
	}
	real8 endtime = curtime + AETIMEOUT;
	while (True)
	{
		if (MCscreen->wait(READ_INTERVAL, False, True))
		{
			MCresult->sets("user interrupt");
            r_value = MCValueRetain(kMCEmptyString);
            return;
		}
		if (curtime > endtime)
		{
			MCresult->sets("timeout");
            r_value = MCValueRetain(kMCEmptyString);
            return;
		}
		if (AEanswerErr != NULL || AEanswerData != NULL)
			break;
	}
	if (AEanswerErr != NULL)
	{
		MCresult->copysvalue(AEanswerErr);
		delete AEanswerErr;
		AEanswerErr = NULL;
        r_value = MCValueRetain(kMCEmptyString);
        return;
	}
	else
	{
		MCresult->clear(False);
		if (!MCStringCreateWithCString(AEanswerData, r_value)) // Set empty string if the allocation fails
            r_value = MCValueRetain(kMCEmptyString);
        
		AEanswerData = NULL;
	}
#endif /* MCS_request_program_dsk_mac */
        AEAddressDesc receiver;
        errno = getDescFromAddress(p_program, &receiver);
        if (errno != noErr)
        {
            AEDisposeDesc(&receiver);
            MCresult->sets("no such program");
            r_value = MCValueRetain(kMCEmptyString);
            return false;
        }
        AppleEvent ae;
        errno = AECreateAppleEvent('misc', 'eval', &receiver,
                                   kAutoGenerateReturnID, kAnyTransactionID, &ae);
        AEDisposeDesc(&receiver); //dispose of the receiver description record
        //add parameters to the Apple event
        AEPutParamPtr(&ae, keyDirectObject, typeChar,
                      MCStringGetCString(p_message), MCStringGetLength(p_message));
        //Send the Apple event
        AppleEvent answer;
        errno = AESend(&ae, &answer, kAEQueueReply, kAENormalPriority,
                       kAEDefaultTimeout, NULL, NULL); //no reply
        AEDisposeDesc(&ae);
        AEDisposeDesc(&answer);
        if (errno != noErr)
        {
            char *buffer = new char[6 + I2L];
            sprintf(buffer, "error %d", errno);
            MCresult->copysvalue(buffer);
            delete buffer;
            
            r_value = MCValueRetain(kMCEmptyString);
            return false;
        }
        real8 endtime = curtime + AETIMEOUT;
        while (True)
        {
            if (MCscreen->wait(READ_INTERVAL, False, True))
            {
                MCresult->sets("user interrupt");
                r_value = MCValueRetain(kMCEmptyString);
                return false;
            }
            if (curtime > endtime)
            {
                MCresult->sets("timeout");
                r_value = MCValueRetain(kMCEmptyString);
                return false;
            }
            if (AEanswerErr != NULL || AEanswerData != NULL)
                break;
        }
        if (AEanswerErr != NULL)
        {
            MCresult->copysvalue(AEanswerErr);
            delete AEanswerErr;
            AEanswerErr = NULL;
            r_value = MCValueRetain(kMCEmptyString);
            return true;
        }
        else
        {
            MCresult->clear(False);
            if (!MCStringCreateWithCString(AEanswerData, r_value)) // Set empty string if the allocation fails
                r_value = MCValueRetain(kMCEmptyString);
            
            AEanswerData = NULL;
            return false;
        }
    }
};

struct MCMacDesktop: public MCSystemInterface, public MCMacSystemService
{
	virtual bool Initialize(void)
    {
#ifdef /* MCS_init_dsk_mac */ LEGACY_SYSTEM
        IO_stdin = new IO_header(stdin, 0, 0, 0, NULL, 0, 0);
        IO_stdout = new IO_header(stdout, 0, 0, 0, NULL, 0, 0);
        IO_stderr = new IO_header(stderr, 0, 0, 0, NULL, 0, 0);
        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = handle_signal;
        action.sa_flags = SA_RESTART;
        sigaction(SIGHUP, &action, NULL);
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGQUIT, &action, NULL);
        sigaction(SIGIOT, &action, NULL);
        sigaction(SIGPIPE, &action, NULL);
        sigaction(SIGALRM, &action, NULL);
        sigaction(SIGTERM, &action, NULL);
        sigaction(SIGUSR1, &action, NULL);
        sigaction(SIGUSR2, &action, NULL);
        sigaction(SIGFPE, &action, NULL);
        action.sa_flags |= SA_NOCLDSTOP;
        sigaction(SIGCHLD, &action, NULL);
        
        // MW-2009-01-29: [[ Bug 6410 ]] Make sure we cause the handlers to be reset to
        //   the OS default so CrashReporter will kick in.
        action.sa_flags = SA_RESETHAND;
        sigaction(SIGSEGV, &action, NULL);
        sigaction(SIGILL, &action, NULL);
        sigaction(SIGBUS, &action, NULL);
        
        // MW-2010-05-11: Make sure if stdin is not a tty, then we set non-blocking.
        //   Without this you can't poll read when a slave process.
        if (!IsInteractiveConsole(0))
            MCS_nodelay(0);
        
        setlocale(LC_ALL, MCnullstring);
        
        _CurrentRuneLocale->__runetype[202] = _CurrentRuneLocale->__runetype[201];
        
        // Initialize our case mapping tables
        
        MCuppercasingtable = new uint1[256];
        for(uint4 i = 0; i < 256; ++i)
            MCuppercasingtable[i] = (uint1)i;
        UppercaseText((char *)MCuppercasingtable, 256, smRoman);
        
        MClowercasingtable = new uint1[256];
        for(uint4 i = 0; i < 256; ++i)
            MClowercasingtable[i] = (uint1)i;
        LowercaseText((char *)MClowercasingtable, 256, smRoman);
        
        //
        
        // MW-2013-03-22: [[ Bug 10772 ]] Make sure we initialize the shellCommand
        //   property here (otherwise it is nil in -ui mode).
        MCshellcmd = strclone("/bin/sh");
        
        //
        
        MoreMasters();
        InitCursor();
        MCinfinity = HUGE_VAL;
        
        long response;
        if (Gestalt(gestaltSystemVersion, &response) == noErr)
            MCmajorosversion = response;
		
        MCaqua = True;
        
        init_utf8_converters();
        
        CFBundleRef theBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.ApplicationServices"));
        if (theBundle != NULL)
        {
            if (CFBundleLoadExecutable(theBundle))
            {
                SwapQDTextFlagsPtr stfptr = (SwapQDTextFlagsPtr)CFBundleGetFunctionPointerForName(theBundle, CFSTR("SwapQDTextFlags"));
                if (stfptr != NULL)
                    stfptr(kQDSupportedFlags);
                CFBundleUnloadExecutable(theBundle);
            }
            CFRelease(theBundle);
        }
        
        MCAutoStringRef dptr;
        MCS_getcurdir(&dptr);
        if (MCStringGetLength(*dptr) <= 1)
        { // if root, then started from Finder
            SInt16 vRefNum;
            SInt32 dirID;
            HGetVol(NULL, &vRefNum, &dirID);
            FSSpec fspec;
            FSMakeFSSpec(vRefNum, dirID, NULL, &fspec);
            char *tpath = MCS_FSSpec2path(&fspec);
            char *newpath = new char[strlen(tpath) + 11];
            strcpy(newpath, tpath);
            strcat(newpath, "/../../../");
            MCAutoStringRef t_new_path_auto;
            /* UNCHECKED */ MCStringCreateWithCString(newpath, &t_new_path_auto);
            MCS_setcurdir(*t_new_path_auto);
            delete tpath;
            delete newpath;
        }
        
        // MW-2007-12-10: [[ Bug 5667 ]] Small font sizes have the wrong metrics
        //   Make sure we always use outlines - then everything looks pretty :o)
        SetOutlinePreferred(TRUE);
        
        MCS_reset_time();
        //do toolbox checking
        long result;
        hasPPCToolbox = (Gestalt(gestaltPPCToolboxAttr, &result)
                         ? False : result != 0);
        hasAppleEvents = (Gestalt(gestaltAppleEventsAttr, &result)
                          ? False : result != 0);
        uint1 i;
        if (hasAppleEvents)
        { //install required AE event handler
            for (i = 0; i < (sizeof(ourkeys) / sizeof(triplets)); ++i)
            {
                if (!ourkeys[i].theUPP)
                {
                    ourkeys[i].theUPP = NewAEEventHandlerUPP(ourkeys[i].theHandler);
                    AEInstallEventHandler(ourkeys[i].theEventClass,
                                          ourkeys[i].theEventID,
                                          ourkeys[i].theUPP, 0L, False);
                }
            }
        }
        
        // ** MODE CHOICE
        if (MCModeShouldPreprocessOpeningStacks())
        {
            EventRecord event;
            i = 2;
            // predispatch any openapp or opendoc events so that stacks[] array
            // can be properly initialized
            while (i--)
                while (WaitNextEvent(highLevelEventMask, &event,
                                     (unsigned long)0, (RgnHandle)NULL))
                    AEProcessAppleEvent(&event);
        }
        //install special handler
        AEEventHandlerUPP specialUPP = NewAEEventHandlerUPP(DoSpecial);
        AEInstallSpecialHandler(keyPreDispatch, specialUPP, False);
        
        if (Gestalt('ICAp', &response) == noErr)
        {
            OSErr err;
            ICInstance icinst;
            ICAttr icattr;
            err = ICStart(&icinst, 'MCRD');
            if (err == noErr)
            {
                Str255 proxystr;
                Boolean useproxy;
                
                long icsize = sizeof(useproxy);
                err = ICGetPref(icinst,  kICUseHTTPProxy, &icattr, &useproxy, &icsize);
                if (err == noErr && useproxy == True)
                {
                    icsize = sizeof(proxystr);
                    err = ICGetPref(icinst, kICHTTPProxyHost ,&icattr, proxystr, &icsize);
                    if (err == noErr)
                    {
                        p2cstr(proxystr);
                        MChttpproxy = strclone((char *)proxystr);
                    }
                }
                ICStop(icinst);
            }
        }
        
        
        MCS_weh = NewEventHandlerUPP(WinEvtHndlr);
        
        // MW-2005-04-04: [[CoreImage]] Load in CoreImage extension
        extern void MCCoreImageRegister(void);
        if (MCmajorosversion >= 0x1040)
            MCCoreImageRegister();
		
        if (!MCnoui)
        {
            setlinebuf(stdout);
            setlinebuf(stderr);
        }
#endif /* MCS_init_dsk_mac */
        IO_stdin = MCsystem -> OpenFd(0, kMCSystemFileModeRead);
        IO_stdout = MCsystem -> OpenFd(1, kMCSystemFileModeWrite);
        IO_stderr = MCsystem -> OpenFd(2, kMCSystemFileModeWrite);
        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = handle_signal;
        action.sa_flags = SA_RESTART;
        sigaction(SIGHUP, &action, NULL);
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGQUIT, &action, NULL);
        sigaction(SIGIOT, &action, NULL);
        sigaction(SIGPIPE, &action, NULL);
        sigaction(SIGALRM, &action, NULL);
        sigaction(SIGTERM, &action, NULL);
        sigaction(SIGUSR1, &action, NULL);
        sigaction(SIGUSR2, &action, NULL);
        sigaction(SIGFPE, &action, NULL);
        action.sa_flags |= SA_NOCLDSTOP;
        sigaction(SIGCHLD, &action, NULL);
        
        // MW-2009-01-29: [[ Bug 6410 ]] Make sure we cause the handlers to be reset to
        //   the OS default so CrashReporter will kick in.
        action.sa_flags = SA_RESETHAND;
        sigaction(SIGSEGV, &action, NULL);
        sigaction(SIGILL, &action, NULL);
        sigaction(SIGBUS, &action, NULL);
        
        // MW-2010-05-11: Make sure if stdin is not a tty, then we set non-blocking.
        //   Without this you can't poll read when a slave process.
        if (!IsInteractiveConsole(0))
            MCS_mac_nodelay(0);
        
        setlocale(LC_ALL, MCnullstring);
        
        _CurrentRuneLocale->__runetype[202] = _CurrentRuneLocale->__runetype[201];
        
        // Initialize our case mapping tables
        
        MCuppercasingtable = new uint1[256];
        for(uint4 i = 0; i < 256; ++i)
            MCuppercasingtable[i] = (uint1)i;
        UppercaseText((char *)MCuppercasingtable, 256, smRoman);
        
        MClowercasingtable = new uint1[256];
        for(uint4 i = 0; i < 256; ++i)
            MClowercasingtable[i] = (uint1)i;
        LowercaseText((char *)MClowercasingtable, 256, smRoman);
        
        // MW-2013-03-22: [[ Bug 10772 ]] Make sure we initialize the shellCommand
        //   property here (otherwise it is nil in -ui mode).
        MCValueAssign(MCshellcmd, MCSTR("/bin/sh"));
        
        //
        
        MoreMasters();
        InitCursor();
        MCinfinity = HUGE_VAL;
        
        long response;
        if (Gestalt(gestaltSystemVersion, &response) == noErr)
            MCmajorosversion = response;
		
        MCaqua = True; // Move to MCScreenDC
        
        init_utf8_converters();
        
        // Move to MCScreenDC
        CFBundleRef theBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.ApplicationServices")); // Move to MCScreenDC
        if (theBundle != NULL)
        {
            if (CFBundleLoadExecutable(theBundle))
            {
                SwapQDTextFlagsPtr stfptr = (SwapQDTextFlagsPtr)CFBundleGetFunctionPointerForName(theBundle, CFSTR("SwapQDTextFlags"));
                if (stfptr != NULL)
                    stfptr(kQDSupportedFlags);
                CFBundleUnloadExecutable(theBundle);
            }
            CFRelease(theBundle);
        }
        //
        
        MCAutoStringRef dptr; // Check to see if this can ever happen anymore - if not, remove
        /* UNCHECKED */ GetCurrentFolder(&dptr);
        if (MCStringGetLength(*dptr) <= 1)
        { // if root, then started from Finder
            SInt16 vRefNum;
            SInt32 dirID;
            HGetVol(NULL, &vRefNum, &dirID);
            FSSpec fspec;
            FSMakeFSSpec(vRefNum, dirID, NULL, &fspec);
            MCAutoStringRef t_path;
            MCAutoStringRef t_new_path;
            /* UNCHECKED */ MCS_mac_FSSpec2path(&fspec, &t_path);
            /* UNCHECKED */ MCStringFormat(&t_new_path, "%s%s", MCStringGetCString(*t_path), "/../../../");
            /* UNCHECKED */ SetCurrentFolder(*t_new_path);
        }
        
        // MW-2007-12-10: [[ Bug 5667 ]] Small font sizes have the wrong metrics
        //   Make sure we always use outlines - then everything looks pretty :o)
        SetOutlinePreferred(TRUE); // Move to MCScreenDC
        
        MCS_reset_time();
        //do toolbox checking
        long result;
        hasPPCToolbox = (Gestalt(gestaltPPCToolboxAttr, &result) // This can be removed (along with hasPPCToolbox)
                         ? False : result != 0);
        hasAppleEvents = (Gestalt(gestaltAppleEventsAttr, &result) // This can be removed (always True)
                          ? False : result != 0);
        
        // Move TO MCscreenDC
        uint1 i;
        if (hasAppleEvents)
        { //install required AE event handler
            for (i = 0; i < (sizeof(ourkeys) / sizeof(triplets)); ++i)
            {
                if (!ourkeys[i].theUPP)
                {
                    ourkeys[i].theUPP = NewAEEventHandlerUPP(ourkeys[i].theHandler);
                    AEInstallEventHandler(ourkeys[i].theEventClass,
                                          ourkeys[i].theEventID,
                                          ourkeys[i].theUPP, 0L, False);
                }
            }
        }
        
        // ** MODE CHOICE
        if (MCModeShouldPreprocessOpeningStacks())
        {
            EventRecord event;
            i = 2;
            // predispatch any openapp or opendoc events so that stacks[] array
            // can be properly initialized
            while (i--)
                while (WaitNextEvent(highLevelEventMask, &event,
                                     (unsigned long)0, (RgnHandle)NULL))
                    AEProcessAppleEvent(&event);
        }
        //install special handler
        AEEventHandlerUPP specialUPP = NewAEEventHandlerUPP(DoSpecial);
        AEInstallSpecialHandler(keyPreDispatch, specialUPP, False);
        // END HERE
        
        if (Gestalt('ICAp', &response) == noErr)
        {
            OSErr err;
            ICInstance icinst;
            ICAttr icattr;
            err = ICStart(&icinst, 'MCRD');
            if (err == noErr)
            {
                Str255 proxystr;
                Boolean useproxy;
                
                long icsize = sizeof(useproxy);
                err = ICGetPref(icinst,  kICUseHTTPProxy, &icattr, &useproxy, &icsize);
                if (err == noErr && useproxy == True)
                {
                    icsize = sizeof(proxystr);
                    err = ICGetPref(icinst, kICHTTPProxyHost ,&icattr, proxystr, &icsize);
                    if (err == noErr)
                    {
                        p2cstr(proxystr);
                        /* UNCHECKED */ MCStringCreateWithCString((char *)proxystr, MChttpproxy);
                    }
                }
                ICStop(icinst);
            }
        }
        
        // Move to MCScreenDC
        MCS_weh = NewEventHandlerUPP(WinEvtHndlr);
        
        // MW-2005-04-04: [[CoreImage]] Load in CoreImage extension
        extern void MCCoreImageRegister(void);
        if (MCmajorosversion >= 0x1040)
            MCCoreImageRegister();
        // END HERE
		
        if (!MCnoui)
        {
            setlinebuf(stdout);
            setlinebuf(stderr);
        }
    }
    
	virtual void Finalize(void)
    {
#ifdef /* MCS_shutdown_dsk_mac */ LEGACY_SYSTEM
	uint2 i;

	// MW-2005-04-04: [[CoreImage]] Unload CoreImage extension
	extern void MCCoreImageUnregister(void);
	MCCoreImageUnregister();

	for (i = 0; i < 32; i++)
		if (unicodeconvertors[i])
			DisposeUnicodeToTextInfo(&unicodeconvertors[i]);
	if (texttounicodeconvertor)
		DisposeTextToUnicodeInfo(texttounicodeconvertor);
	DisposeTextToUnicodeInfo(&texttoutf8info);
	DisposeUnicodeToTextInfo(&utf8totextinfo);

	for (i = 0; i< osancomponents; i++)
		CloseComponent(osacomponents[i].compinstance);
	delete osacomponents;


	DisposeEventHandlerUPP(MCS_weh);
#endif /* MCS_shutdown_dsk_mac */
        uint2 i;
        
        // Move To MCScreenDC
        // MW-2005-04-04: [[CoreImage]] Unload CoreImage extension
        extern void MCCoreImageUnregister(void);
        MCCoreImageUnregister();
        // End
        
        for (i = 0; i < 32; i++)
            if (unicodeconvertors[i])
                DisposeUnicodeToTextInfo(&unicodeconvertors[i]);
        if (texttounicodeconvertor)
            DisposeTextToUnicodeInfo(texttounicodeconvertor);
        DisposeTextToUnicodeInfo(&texttoutf8info);
        DisposeUnicodeToTextInfo(&utf8totextinfo);
        
        for (i = 0; i< osancomponents; i++)
            CloseComponent(osacomponents[i].compinstance);
        delete osacomponents;
        
        // Move To MCScreenDC
        DisposeEventHandlerUPP(MCS_weh);
        // End
    }
	
    virtual MCServiceInterface *QueryService(MCServiceType p_type)
    {
        if (p_type == kMCServiceTypeMacSystem)
            return (MCMacSystemServiceInterface *)this;
        return nil;
    }
    
	virtual void Debug(MCStringRef p_string)
    {
        
    }
    
	virtual real64_t GetCurrentTime(void)
    {
#ifdef /* MCS_time_dsk_mac */ LEGACY_SYSTEM
	struct timezone tz;
	struct timeval tv;

	gettimeofday(&tv, &tz);
	curtime = tv.tv_sec + (real8)tv.tv_usec / 1000000.0;

	return curtime;
#endif /* MCS_time_dsk_mac */
        struct timezone tz;
        struct timeval tv;
        
        gettimeofday(&tv, &tz);
        curtime = tv.tv_sec + (real8)tv.tv_usec / 1000000.0;
        
        return curtime;
    }
    
    virtual void ResetTime(void)
    {
#ifdef /* MCS_reset_time_dsk_mac */ LEGACY_SYSTEM
        
#endif /* MCS_reset_time_dsk_mac */
        // Nothing
    }
    
	virtual bool GetVersion(MCStringRef& r_version)
    {
#ifdef /* MCS_getsystemversion_dsk_mac */ LEGACY_SYSTEM
        long t_major, t_minor, t_bugfix;
        Gestalt(gestaltSystemVersionMajor, &t_major);
        Gestalt(gestaltSystemVersionMinor, &t_minor);
        Gestalt(gestaltSystemVersionBugFix, &t_bugfix);
        return MCStringFormat(r_string, "%d.%d.%d", t_major, t_minor, t_bugfix);
#endif /* MCS_getsystemversion_dsk_mac */
        long t_major, t_minor, t_bugfix;
        Gestalt(gestaltSystemVersionMajor, &t_major);
        Gestalt(gestaltSystemVersionMinor, &t_minor);
        Gestalt(gestaltSystemVersionBugFix, &t_bugfix);
        return MCStringFormat(r_version, "%d.%d.%d", t_major, t_minor, t_bugfix);
    }
	virtual bool GetMachine(MCStringRef& r_string)
    {
#ifdef /* MCS_getmachine_dsk_mac */ LEGACY_SYSTEM
	static Str255 machineName;
	long response;
	if ((errno = Gestalt(gestaltMachineType, &response)) == noErr)
	{
		GetIndString(machineName, kMachineNameStrID, response);
		if (machineName != nil)
		{
			p2cstr(machineName);
			return MCStringCreateWithNativeChars((const char_t *)machineName, MCCStringLength((const char *)machineName), r_string);
		}
	}
	return MCStringCopy(MCNameGetString(MCN_unknown), r_string);
#endif /* MCS_getmachine_dsk_mac */
        static Str255 machineName;
        long response;
        if ((errno = Gestalt(gestaltMachineType, &response)) == noErr)
        {
            GetIndString(machineName, kMachineNameStrID, response);
            if (machineName != nil)
            {
                p2cstr(machineName);
                return MCStringCreateWithNativeChars((const char_t *)machineName, MCCStringLength((const char *)machineName), r_string);
            }
        }
        return MCStringCopy(MCNameGetString(MCN_unknown), r_string);
    }
	virtual MCNameRef GetProcessor(void)
    {
#ifdef /* MCS_getprocessor_dsk_mac */ LEGACY_SYSTEM
//get machine processor
#ifdef __LITTLE_ENDIAN__
	return MCN_x86;
#else
    return MCN_motorola_powerpc;
#endif
#endif /* MCS_getprocessor_dsk_mac */
//get machine processor
#ifdef __LITTLE_ENDIAN__
        return MCN_x86;
#else
        return MCN_motorola_powerpc;
#endif
    }
	virtual bool GetAddress(MCStringRef& r_address)
    {
#ifdef /* MCS_getaddress_dsk_mac */ LEGACY_SYSTEM
	static struct utsname u;
	uname(&u);
	return MCStringFormat(r_address, "%s:%s", u.nodename, MCcmd);
#endif /* MCS_getaddress_dsk_mac */
        static struct utsname u;
        uname(&u);
        return MCStringFormat(r_address, "%s:%s", u.nodename, MCStringGetCString(MCcmd));
    }
    
	virtual uint32_t GetProcessId(void)
    {
#ifdef /* MCS_getpid_dsk_mac */ LEGACY_SYSTEM
	return getpid();
#endif /* MCS_getpid_dsk_mac */
        return getpid();
    }
	
	virtual void Alarm(real64_t p_when)
    {
#ifdef /* MCS_alarm_dsk_mac */ LEGACY_SYSTEM
    //is used for checking event loop periodically
	// InsTime() or
	//PrimeTime(pass handle to a function, in the function set MCalarm to True)
#endif /* MCS_alarm_dsk_mac */
    }
    
	virtual void Sleep(real64_t p_duration)
    {
#ifdef /* MCS_sleep_dsk_mac */ LEGACY_SYSTEM
	unsigned long finalTicks;
	Delay((unsigned long)duration * 60, &finalTicks);
#endif /* MCS_sleep_dsk_mac */
        unsigned long finalTicks;
        Delay((unsigned long)p_duration * 60, &finalTicks);
    }
	
	virtual void SetEnv(MCStringRef p_name, MCStringRef p_value)
    {
#ifdef /* MCS_setenv_dsk_mac */ LEGACY_SYSTEM
	setenv(name, value, True);

#endif /* MCS_setenv_dsk_mac */
#ifdef /* MCS_unsetenv_dsk_mac */ LEGACY_SYSTEM
	unsetenv(name);

#endif /* MCS_unsetenv_dsk_mac */
        setenv(MCStringGetCString(p_name), MCStringGetCString(p_value), True);
    }
    
	virtual bool GetEnv(MCStringRef p_name, MCStringRef& r_value)
    {
#ifdef /* MCS_getenv_dsk_mac */ LEGACY_SYSTEM
	return getenv(name); //always returns NULL under CodeWarrier env.
#endif /* MCS_getenv_dsk_mac */
        return MCStringCreateWithCString(getenv(MCStringGetCString(p_name)), r_value); //always returns NULL under CodeWarrier env.
    }
	
	virtual Boolean CreateFolder(MCStringRef p_path)
    {
#ifdef /* MCS_mkdir_dsk_mac */ LEGACY_SYSTEM
    
	char *newpath = path2utf(MCS_resolvepath(path));
	Boolean done = mkdir(newpath, 0777) == 0;
	delete newpath;
	return done;
#endif /* MCS_mkdir_dsk_mac */
        MCAutoStringRefAsUTF8String t_path;
        if (!t_path.Lock(p_path))
            return False;
        
        if (mkdir(*t_path, 0777) != 0)
            return False;
            
        return True;
    }
    
	virtual Boolean DeleteFolder(MCStringRef p_path)
    {
#ifdef /* MCS_rmdir_dsk_mac */ LEGACY_SYSTEM
    
    char *newpath = path2utf(MCS_resolvepath(path));
    Boolean done = rmdir(newpath) == 0;
    delete newpath;
    return done;
#endif /* MCS_rmdir_dsk_mac */
        MCAutoStringRefAsUTF8String t_path;
        if (!t_path.Lock(p_path))
            return False;
        
        if (rmdir(*t_path) != 0)
            return False;
        
        return True;
    }
    
//    /* LEGACY */
//    virtual bool DeleteFile(const char *p_path)
//    {
//        char *newpath = path2utf(MCS_resolvepath(p_path));
//        Boolean done = remove(newpath) == 0;
//        delete newpath;
//        return done;
//    }
	
	virtual Boolean DeleteFile(MCStringRef p_path)
    {
#ifdef /* MCS_unlink_dsk_mac */ LEGACY_SYSTEM
    char *newpath = path2utf(MCS_resolvepath(path));
    Boolean done = remove(newpath) == 0;
    delete newpath;
    return done;
#endif /* MCS_unlink_dsk_mac */
        MCAutoStringRefAsUTF8String t_path;
        if (!t_path.Lock(p_path))
            return False;
        
        if (remove(*t_path) != 0)
            return False;
        
        return True;
    }
	
	virtual Boolean RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_rename_dsk_mac */ LEGACY_SYSTEM
    //rename a file or directory
    
	char *oldpath = path2utf(MCS_resolvepath(oname));
	char *newpath = path2utf(MCS_resolvepath(nname));
	Boolean done = rename(oldpath, newpath) == 0;
    
	delete oldpath;
	delete newpath;
	return done;
#endif /* MCS_rename_dsk_mac */
        MCAutoStringRefAsUTF8String t_old_name, t_new_name;
        
        if (!t_old_name.Lock(p_old_name) || !t_new_name.Lock(p_new_name))
            return False;
        
        if (rename(*t_old_name, *t_new_name) != 0)
            return False;
        
        return True;
    }
	
    // MW-2007-07-16: [[ Bug 5214 ]] Use rename instead of FSExchangeObjects since
    //   the latter isn't supported on all FS's.
    // MW-2007-12-12: [[ Bug 5674 ]] Unfortunately, just renaming the current stack
    //   causes all Finder meta-data to be lost, so what we will do is first try
    //   to FSExchangeObjects and if that fails, do a rename.
	virtual Boolean BackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_backup_dsk_mac */ LEGACY_SYSTEM
	bool t_error;
	t_error = false;
	
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_src_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	FSRef t_dst_parent_ref;
	FSRef t_dst_ref;
	UniChar *t_dst_leaf;
	t_dst_leaf = NULL;
	UniCharCount t_dst_leaf_length;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_dst_path, &t_dst_ref);
		if (t_os_error == noErr)
			FSDeleteObject(&t_dst_ref);
        
		// Get the information to create the file
		t_os_error = MCS_pathtoref_and_leaf(p_dst_path, t_dst_parent_ref, t_dst_leaf, t_dst_leaf_length);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSCatalogInfo t_dst_catalog;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSGetCatalogInfo(&t_src_ref, kFSCatInfoFinderInfo, &t_dst_catalog, NULL, NULL, NULL);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (!t_error)
	{
		memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileType, &MCfiletype[4], 4);
		memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator, MCfiletype, 4);
		((FileInfo *) t_dst_catalog . finderInfo) -> fileType = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileType);
		((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator);
	}
	
	bool t_created_dst;
	t_created_dst = false;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSCreateFileUnicode(&t_dst_parent_ref, t_dst_leaf_length, t_dst_leaf, kFSCatInfoFinderInfo, &t_dst_catalog, &t_dst_ref, NULL);
		if (t_os_error == noErr)
			t_created_dst = true;
		else
			t_error = true;
	}
	
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (t_error && t_created_dst)
		FSDeleteObject(&t_dst_ref);
	
	if (t_dst_leaf != NULL)
		delete t_dst_leaf;
    
	if (t_error)
		t_error = !MCS_rename(p_src_path, p_dst_path);
    
	return !t_error;
#endif /* MCS_backup_dsk_mac */
        bool t_error;
        t_error = false;
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_mac_pathtoref(p_old_name, t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSRef t_dst_parent_ref;
        FSRef t_dst_ref;
        UniChar *t_dst_leaf;
        t_dst_leaf = NULL;
        UniCharCount t_dst_leaf_length;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_mac_pathtoref(p_new_name, t_dst_ref);
            if (t_os_error == noErr)
                FSDeleteObject(&t_dst_ref);
			
            // Get the information to create the file
            t_os_error = MCS_mac_pathtoref_and_leaf(p_new_name, t_dst_parent_ref, t_dst_leaf, t_dst_leaf_length);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSCatalogInfo t_dst_catalog;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSGetCatalogInfo(&t_src_ref, kFSCatInfoFinderInfo, &t_dst_catalog, NULL, NULL, NULL);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        if (!t_error)
        {
            const char *t_char_ptr;
            t_char_ptr = (char*)MCStringGetCString(MCfiletype);
            memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileType, t_char_ptr + 4, 4);
            memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator, t_char_ptr, 4);
            ((FileInfo *) t_dst_catalog . finderInfo) -> fileType = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileType);
            ((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator);
        }
        
        bool t_created_dst;
        t_created_dst = false;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSCreateFileUnicode(&t_dst_parent_ref, t_dst_leaf_length, t_dst_leaf, kFSCatInfoFinderInfo, &t_dst_catalog, &t_dst_ref, NULL);
            if (t_os_error == noErr)
                t_created_dst = true;
            else
                t_error = true;
        }
        
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        if (t_error && t_created_dst)
            FSDeleteObject(&t_dst_ref);
        
        if (t_dst_leaf != NULL)
            delete t_dst_leaf;
		
        if (t_error)
            t_error = !RenameFileOrFolder(p_old_name, p_new_name);
		
        if (t_error)
            return False;
        
        return True;
    }
    
	virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_unbackup_dsk_mac */ LEGACY_SYSTEM
	bool t_error;
	t_error = false;
	
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_src_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSRef t_dst_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_dst_path, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	// It appears that the source file here is the ~file, the backup file.
	// So copy it over to p_dst_path, and delete it.
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSDeleteObject(&t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	if (t_error)
		t_error = !MCS_rename(p_src_path, p_dst_path);
    
	return !t_error;
#endif /* MCS_unbackup_dsk_mac */
        bool t_error;
        t_error = false;
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_mac_pathtoref(p_old_name, t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSRef t_dst_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_mac_pathtoref(p_new_name, t_dst_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        // It appears that the source file here is the ~file, the backup file.
        // So copy it over to p_dst_path, and delete it.
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSDeleteObject(&t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        if (t_error)
            t_error = !RenameFileOrFolder(p_old_name, p_new_name);
		
        if (t_error)
            return False;
        
        return True;
    }
	
	virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias)
    {
#ifdef /* MCS_createalias_dsk_mac */ LEGACY_SYSTEM
	bool t_error;
	t_error = false;
	
	// Check if the destination exists already and return an error if it does
	if (!t_error)
	{
		FSRef t_dst_ref;
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_dest_path, &t_dst_ref);
		if (t_os_error == noErr)
			return False; // we expect an error
	}
    
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_source_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	FSRef t_dst_parent_ref;
	UniChar *t_dst_leaf_name;
	UniCharCount t_dst_leaf_name_length;
	t_dst_leaf_name = NULL;
	t_dst_leaf_name_length = 0;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref_and_leaf(p_dest_path, t_dst_parent_ref, t_dst_leaf_name, t_dst_leaf_name_length);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	AliasHandle t_alias;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSNewAlias(NULL, &t_src_ref, &t_alias);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	IconRef t_src_icon;
	t_src_icon = NULL;
	if (!t_error)
	{
		OSErr t_os_error;
		SInt16 t_unused_label;
		t_os_error = GetIconRefFromFileInfo(&t_src_ref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, &t_src_icon, &t_unused_label);
		if (t_os_error != noErr)
			t_src_icon = NULL;
	}
	
	IconFamilyHandle t_icon_family;
	t_icon_family = NULL;
	if (!t_error && t_src_icon != NULL)
	{
		OSErr t_os_error;
		IconRefToIconFamily(t_src_icon, kSelectorAllAvailableData, &t_icon_family);
	}
	
	HFSUniStr255 t_fork_name;
	if (!t_error)
		FSGetResourceForkName(&t_fork_name);
    
	FSRef t_dst_ref;
	FSSpec t_dst_spec;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSCreateResourceFile(&t_dst_parent_ref, t_dst_leaf_name_length, t_dst_leaf_name,
                                          kFSCatInfoNone, NULL, t_fork_name . length, t_fork_name . unicode, &t_dst_ref, &t_dst_spec);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	ResFileRefNum t_res_file;
	bool t_res_file_opened;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSOpenResourceFile(&t_dst_ref, t_fork_name . length, t_fork_name . unicode, 3, &t_res_file);
		if (t_os_error != noErr)
			t_error = true;
		else
			t_res_file_opened = true;
	}
    
	if (!t_error)
	{
		AddResource((Handle)t_alias, rAliasType, 0, (ConstStr255Param)"");
		if (ResError() != noErr)
			t_error = true;
	}
	
	if (!t_error && t_icon_family != NULL)
		AddResource((Handle)t_icon_family, 'icns', -16496, NULL);
    
	if (t_res_file_opened)
		CloseResFile(t_res_file);
	
	if (!t_error)
	{
		FSCatalogInfo t_info;
		FSGetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info, NULL, NULL, NULL);
		((FileInfo *)&t_info . finderInfo) -> finderFlags |= kIsAlias;
		if (t_icon_family != NULL)
			((FileInfo *)&t_info . finderInfo) -> finderFlags |= kHasCustomIcon;
		FSSetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info);
	}
    
	if (t_src_icon != NULL)
		ReleaseIconRef(t_src_icon);
    
	if (t_dst_leaf_name != NULL)
		delete t_dst_leaf_name;
	
	if (t_error)
	{
		if (t_icon_family != NULL)
			DisposeHandle((Handle)t_icon_family);
		FSDeleteObject(&t_dst_ref);
	}
    
	return !t_error;       
#endif /* MCS_createalias_dsk_mac */
        bool t_error;
        t_error = false;
        
        // Check if the destination exists already and return an error if it does
        if (!t_error)
        {
            FSRef t_dst_ref;
            OSErr t_os_error;
            t_os_error = MCS_mac_pathtoref(p_alias, t_dst_ref);
            if (t_os_error == noErr)
                return False; // we expect an error
        }
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_mac_pathtoref(p_target, t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSRef t_dst_parent_ref;
        UniChar *t_dst_leaf_name;
        UniCharCount t_dst_leaf_name_length;
        t_dst_leaf_name = NULL;
        t_dst_leaf_name_length = 0;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_mac_pathtoref_and_leaf(p_alias, t_dst_parent_ref, t_dst_leaf_name, t_dst_leaf_name_length);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        AliasHandle t_alias;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSNewAlias(NULL, &t_src_ref, &t_alias);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        IconRef t_src_icon;
        t_src_icon = NULL;
        if (!t_error)
        {
            OSErr t_os_error;
            SInt16 t_unused_label;
            t_os_error = GetIconRefFromFileInfo(&t_src_ref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, &t_src_icon, &t_unused_label);
            if (t_os_error != noErr)
                t_src_icon = NULL;
        }
        
        IconFamilyHandle t_icon_family;
        t_icon_family = NULL;
        if (!t_error && t_src_icon != NULL)
        {
            OSErr t_os_error;
            IconRefToIconFamily(t_src_icon, kSelectorAllAvailableData, &t_icon_family);
        }
        
        HFSUniStr255 t_fork_name;
        if (!t_error)
            FSGetResourceForkName(&t_fork_name);
        
        FSRef t_dst_ref;
        FSSpec t_dst_spec;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSCreateResourceFile(&t_dst_parent_ref, t_dst_leaf_name_length, t_dst_leaf_name,
                                              kFSCatInfoNone, NULL, t_fork_name . length, t_fork_name . unicode, &t_dst_ref, &t_dst_spec);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        ResFileRefNum t_res_file;
        bool t_res_file_opened;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSOpenResourceFile(&t_dst_ref, t_fork_name . length, t_fork_name . unicode, 3, &t_res_file);
            if (t_os_error != noErr)
                t_error = true;
            else
                t_res_file_opened = true;
        }
        
        if (!t_error)
        {
            AddResource((Handle)t_alias, rAliasType, 0, (ConstStr255Param)"");
            if (ResError() != noErr)
                t_error = true;
        }
        
        if (!t_error && t_icon_family != NULL)
            AddResource((Handle)t_icon_family, 'icns', -16496, NULL);
        
        if (t_res_file_opened)
            CloseResFile(t_res_file);
        
        if (!t_error)
        {
            FSCatalogInfo t_info;
            FSGetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info, NULL, NULL, NULL);
            ((FileInfo *)&t_info . finderInfo) -> finderFlags |= kIsAlias;
            if (t_icon_family != NULL)
                ((FileInfo *)&t_info . finderInfo) -> finderFlags |= kHasCustomIcon;
            FSSetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info);		
        }
        
        if (t_src_icon != NULL)
            ReleaseIconRef(t_src_icon);
        
        if (t_dst_leaf_name != NULL)
            delete t_dst_leaf_name;
        
        if (t_error)
        {
            if (t_icon_family != NULL)
                DisposeHandle((Handle)t_icon_family);
            FSDeleteObject(&t_dst_ref);
        }
		
        if (t_error)
            return False;
        
        return True;
    }
    
	// NOTE: 'ResolveAlias' returns a standard (not native) path.
	virtual Boolean ResolveAlias(MCStringRef p_target, MCStringRef& r_resolved_path)
    {
#ifdef /* MCS_resolvealias_dsk_mac */ LEGACY_SYSTEM
    FSRef t_fsref;
    
    OSErr t_os_error;
    t_os_error = MCS_pathtoref(p_path, t_fsref);
    if (t_os_error != noErr)
        return MCStringCreateWithCString("file not found", r_error);
    
    Boolean t_is_folder;
    Boolean t_is_alias;
    
    t_os_error = FSResolveAliasFile(&t_fsref, TRUE, &t_is_folder, &t_is_alias);
    if (t_os_error != noErr || !t_is_alias) // this always seems to be false
        return MCStringCreateWithCString("can't get alias", r_error);
    
    if (!MCS_fsref_to_path(t_fsref, r_resolved))
        return MCStringCreateWithCString("can't get alias path", r_error);
    
    return true;
#endif /* MCS_resolvealias_dsk_mac */
        FSRef t_fsref;
        
        OSErr t_os_error;
        t_os_error = MCS_mac_pathtoref(p_target, t_fsref);
        if (t_os_error != noErr)
        {
            if (!MCStringCreateWithCString("file not found", r_resolved_path))
                return False;
            else
                return True;
        }
        
        Boolean t_is_folder;
        Boolean t_is_alias;
        
        t_os_error = FSResolveAliasFile(&t_fsref, TRUE, &t_is_folder, &t_is_alias);
        if (t_os_error != noErr || !t_is_alias) // this always seems to be false
        {
            if (!MCStringCreateWithCString("can't get alias", r_resolved_path))
                return False;
            else
                return True;
        }
        
        if (!MCS_mac_fsref_to_path(t_fsref, r_resolved_path))
        {
            if (!MCStringCreateWithCString("can't get alias path", r_resolved_path))
                return False;
            
            return True;
        }
        
        return True;
    }
	
	virtual bool GetCurrentFolder(MCStringRef& r_path)
    {
#ifdef /* MCS_getcurdir_dsk_mac */ LEGACY_SYSTEM
    char namebuf[PATH_MAX + 2];
    if (NULL == getcwd(namebuf, PATH_MAX))
        return false;
        
    MCAutoNativeCharArray t_buffer;
    if (!t_buffer.New(PATH_MAX + 1))
        return false;
        
    uint4 outlen;
    outlen = PATH_MAX + 1;
    MCS_utf8tonative(namebuf, strlen(namebuf), (char*)t_buffer.Chars(), outlen);
    t_buffer.Shrink(outlen);
    return t_buffer.CreateStringAndRelease(r_path);
#endif /* MCS_getcurdir_dsk_mac */
        char namebuf[PATH_MAX + 2];
        if (NULL == getcwd(namebuf, PATH_MAX))
            return false;
        
        if (!MCStringCreateWithBytes((byte_t*)namebuf, strlen(namebuf), kMCStringEncodingUTF8, false, r_path))
        {
            r_path = MCValueRetain(kMCEmptyString);
            return false;
        }
        return true;
    }
    
    // MW-2006-04-07: Bug 3201 - MCS_resolvepath returns NULL if unable to find a ~<username> folder.
	virtual Boolean SetCurrentFolder(MCStringRef p_path)
    {
#ifdef /* MCS_setcurdir_dsk_mac */ LEGACY_SYSTEM
    char *t_resolved_path;
    t_resolved_path = MCS_resolvepath(path);
    if (t_resolved_path == NULL)
        return False;
        
    char *newpath = NULL;
    newpath = path2utf(t_resolved_path);
    
    Boolean done = chdir(newpath) == 0;
    delete newpath;
    if (!done)
        return False;
    
    return True;
#endif /* MCS_setcurdir_dsk_mac */
        bool t_success;
        MCAutoStringRefAsUTF8String t_utf8_string;
        if (!t_utf8_string.Lock(p_path))
            return False;
        
        if (chdir(*t_utf8_string) != 0)
            return False;
        
        return True;
    }
	
	// NOTE: 'GetStandardFolder' returns a standard (not native) path.
	virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder)
    {
#ifdef /* MCS_getspecialfolder_dsk_mac */ LEGACY_SYSTEM
    uint32_t t_mac_folder = 0;
    OSType t_domain = kOnAppropriateDisk;
    bool t_found_folder = false;
    
    if (MCS_specialfolder_to_mac_folder(p_type, t_mac_folder, t_domain))
        t_found_folder = true;
    else if (MCStringGetLength(p_type) == 4)
    {
        t_mac_folder = MCSwapInt32NetworkToHost(*((uint32_t*)MCStringGetNativeCharPtr(p_type)));
        
        uindex_t t_i;
        for (t_i = 0 ; t_i < ELEMENTS(sysfolderlist); t_i++)
            if (t_mac_folder == sysfolderlist[t_i] . macfolder)
            {
                t_domain = sysfolderlist[t_i] . domain;
                t_mac_folder = sysfolderlist[t_i] . mactag;
                t_found_folder = true;
                break;
            }
    }
    
    FSRef t_folder_ref;
    if (t_found_folder)
    {
        OSErr t_os_error;
        Boolean t_create_folder;
        t_create_folder = t_domain == kUserDomain ? kCreateFolder : kDontCreateFolder;
        t_os_error = FSFindFolder(t_domain, t_mac_folder, t_create_folder, &t_folder_ref);
        t_found_folder = t_os_error == noErr;
    }
    
    if (!t_found_folder)
    {
        r_path = MCValueRetain(kMCEmptyString);
        return true;
    }
    
    return MCS_fsref_to_path(t_folder_ref, r_path);
#endif /* MCS_getspecialfolder_dsk_mac */
        uint32_t t_mac_folder = 0;
        OSType t_domain = kOnAppropriateDisk;
        bool t_found_folder = false;
        
        if (MCS_mac_specialfolder_to_mac_folder(MCNameGetString(p_type), t_mac_folder, t_domain))
            t_found_folder = true;
        else if (MCStringGetLength(MCNameGetString(p_type)) == 4)
        {
            t_mac_folder = MCSwapInt32NetworkToHost(*((uint32_t*)MCStringGetNativeCharPtr(MCNameGetString(p_type))));
			
            uindex_t t_i;
            for (t_i = 0 ; t_i < ELEMENTS(sysfolderlist); t_i++)
                if (t_mac_folder == sysfolderlist[t_i] . macfolder)
                {
                    t_domain = sysfolderlist[t_i] . domain;
                    t_mac_folder = sysfolderlist[t_i] . mactag;
                    t_found_folder = true;
                    break;
                }
        }
        
        FSRef t_folder_ref;
        if (t_found_folder)
        {
            OSErr t_os_error;
            Boolean t_create_folder;
            t_create_folder = t_domain == kUserDomain ? kCreateFolder : kDontCreateFolder;
            t_os_error = FSFindFolder(t_domain, t_mac_folder, t_create_folder, &t_folder_ref);
            t_found_folder = t_os_error == noErr;
        }
        
        if (!t_found_folder)
        {
            r_folder = MCValueRetain(kMCEmptyString);
            return True;
        }
		
        if (!MCS_mac_fsref_to_path(t_folder_ref, r_folder))
            return False;
        
        return True;
    }
	
	virtual Boolean FileExists(MCStringRef p_path)
    {
#ifdef /* MCS_exists_dsk_mac */ LEGACY_SYSTEM
    if (MCStringGetLength(p_path) == 0)
        return false;
    
    MCAutoStringRef t_resolved, t_utf8_path;
    if (!MCS_resolvepath(p_path, &t_resolved) ||
        !MCU_nativetoutf8(*t_resolved, &t_utf8_path))
        return false;
        
    bool t_found;
    struct stat buf;
    t_found = stat(MCStringGetCString(*t_utf8_path), (struct stat *)&buf) == 0;
    if (t_found)
        t_found = (p_is_file == ((buf.st_mode & S_IFDIR) == 0));
    
    return t_found;
#endif /* MCS_exists_dsk_mac */
        if (MCStringGetLength(p_path) == 0)
            return False;
        
        MCAutoStringRefAsUTF8String t_utf8_path;
        if (!t_utf8_path.Lock(p_path))
            return False;
        
        bool t_found;
        struct stat buf;
        t_found = stat(*t_utf8_path, (struct stat *)&buf) == 0;
        if (t_found)
            t_found = ((buf.st_mode & S_IFDIR) == 0);
        
        if (!t_found)
            return False;
        
        return True;
    }
    
	virtual Boolean FolderExists(MCStringRef p_path)
    {
        if (MCStringGetLength(p_path) == 0)
            return False;
        
        MCAutoStringRefAsUTF8String t_utf8_path;
        if (!t_utf8_path.Lock(p_path))
            return False;
        
        bool t_found;
        struct stat buf;
        t_found = stat(*t_utf8_path, (struct stat *)&buf) == 0;
        if (t_found)
            t_found = (buf.st_mode & S_IFDIR) != 0;
        
        if (!t_found)
            return False;
        
        return True;
    }
    
	virtual Boolean FileNotAccessible(MCStringRef p_path)
    {
#ifdef /* MCS_noperm_dsk_mac */ LEGACY_SYSTEM
    return False;
#endif /* MCS_noperm_dsk_mac */
        return False;
    }
	
	virtual Boolean ChangePermissions(MCStringRef p_path, uint2 p_mask)
    {
#ifdef /* MCS_chmod_dsk_mac */ LEGACY_SYSTEM
    return IO_NORMAL;
#endif /* MCS_chmod_dsk_mac */
        return True;
    }
    
	virtual uint2 UMask(uint2 p_mask)
    {
#ifdef /* MCS_umask_dsk_mac */ LEGACY_SYSTEM
	return 0;
#endif /* MCS_umask_dsk_mac */
        return 0;
    }
	
	// NOTE: 'GetTemporaryFileName' returns a standard (not native) path.
	virtual bool GetTemporaryFileName(MCStringRef& r_tmp_name)
    {
#ifdef /* MCS_tmpnam_dsk_mac */ LEGACY_SYSTEM
	char *t_temp_file = nil;
	FSRef t_folder_ref;
	if (FSFindFolder(kOnSystemDisk, kTemporaryFolderType, TRUE, &t_folder_ref) == noErr)
	{
		t_temp_file = MCS_fsref_to_path(t_folder_ref);
		MCCStringAppendFormat(t_temp_file, "/tmp.%d.XXXXXXXX", getpid());
		
		int t_fd;
		t_fd = mkstemp(t_temp_file);
		if (t_fd == -1)
		{
			delete t_temp_file;
			return false;
		}

		close(t_fd);
		unlink(t_temp_file);
	}
	
	if (t_temp_file == nil)
	{
		r_path = MCValueRetain(kMCEmptyString);
		return true;
	}
	
	bool t_success = MCStringCreateWithCString(t_temp_file, r_path);
	delete t_temp_file;
        return t_success;
#endif /* MCS_tmpnam_dsk_mac */
        bool t_success = false;
        MCAutoStringRef t_temp_file_auto;
        FSRef t_folder_ref;
        char* t_temp_file_chars;
        
        t_temp_file_chars = nil;        
        t_success = MCStringCreateMutable(0, &t_temp_file_auto);
        
        if (t_success && FSFindFolder(kOnSystemDisk, kTemporaryFolderType, TRUE, &t_folder_ref) == noErr)
        {
            int t_fd;
            t_success = MCS_mac_fsref_to_path(t_folder_ref, &t_temp_file_auto);
            
            if (t_success)
                t_success = MCStringAppendFormat(&t_temp_file_auto, "/tmp.%d.XXXXXXXX", getpid());
            
            if (t_success)
                t_success = MCMemoryAllocateCopy(MCStringGetCString(*t_temp_file_auto), MCStringGetLength(*t_temp_file_auto) + 1, t_temp_file_chars);
            
            if (t_success)
            {
                t_fd = mkstemp(t_temp_file_chars);
                t_success = t_fd != -1;
            }
            
            if (t_success)
            {
                close(t_fd);
                t_success = unlink(t_temp_file_chars) == 0;
            }
        }
        
        if (t_success)
            t_success = MCStringCreateWithCString(t_temp_file_chars, r_tmp_name);
        
        if (!t_success)
            r_tmp_name = MCValueRetain(kMCEmptyString);
        
        MCMemoryDeallocate(t_temp_file_chars);
        
        return t_success;
    }
    
#define CATALOG_MAX_ENTRIES 16
	virtual bool ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *x_context)
    {
#ifdef /* MCS_getentries_dsk_mac */ LEGACY_SYSTEM
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	OSStatus t_os_status;
	
	Boolean t_is_folder;
	FSRef t_current_fsref;
	
	t_os_status = FSPathMakeRef((const UInt8 *)".", &t_current_fsref, &t_is_folder);
	if (t_os_status != noErr || !t_is_folder)
		return false;

	// Create the iterator, pass kFSIterateFlat to iterate over the current subtree only
	FSIterator t_catalog_iterator;
	t_os_status = FSOpenIterator(&t_current_fsref, kFSIterateFlat, &t_catalog_iterator);
	if (t_os_status != noErr)
		return false;
	
	uint4 t_entry_count;
	t_entry_count = 0;
	
	if (!p_files)
	{
		t_entry_count++;
		/* UNCHECKED */ MCListAppendCString(*t_list, "..");
	}
	
	ItemCount t_max_objects, t_actual_objects;
	t_max_objects = CATALOG_MAX_ENTRIES;
	t_actual_objects = 0;
	FSCatalogInfo t_catalog_infos[CATALOG_MAX_ENTRIES];
	HFSUniStr255 t_names[CATALOG_MAX_ENTRIES];
	
	FSCatalogInfoBitmap t_info_bitmap;
	t_info_bitmap = kFSCatInfoAllDates |
					kFSCatInfoPermissions |
					kFSCatInfoUserAccess |
					kFSCatInfoFinderInfo | 
					kFSCatInfoDataSizes |
					kFSCatInfoRsrcSizes |
					kFSCatInfoNodeFlags;

	MCExecPoint t_tmp_context(NULL, NULL, NULL);	
	OSErr t_oserror;
	do
	{
		t_oserror = FSGetCatalogInfoBulk(t_catalog_iterator, t_max_objects, &t_actual_objects, NULL, t_info_bitmap, t_catalog_infos, NULL, NULL, t_names);
		if (t_oserror != noErr && t_oserror != errFSNoMoreItems)
		{	// clean up and exit
			FSCloseIterator(t_catalog_iterator);
			return false;
		}
		
		for(uint4 t_i = 0; t_i < (uint4)t_actual_objects; t_i++)
		{
			// folders
			UInt16 t_is_folder;
			t_is_folder = t_catalog_infos[t_i] . nodeFlags & kFSNodeIsDirectoryMask;
			if ( (!p_files && t_is_folder) || (p_files && !t_is_folder))
			{
				char t_native_name[256];
				uint4 t_native_length;
				t_native_length = 256;
				MCS_utf16tonative((const unsigned short *)t_names[t_i] . unicode, t_names[t_i] . length, t_native_name, t_native_length);
				
				// MW-2008-02-27: [[ Bug 5920 ]] Make sure we convert Finder to POSIX style paths
				for(uint4 i = 0; i < t_native_length; ++i)
					if (t_native_name[i] == '/')
						t_native_name[i] = ':';
				
				char t_buffer[512];
				if (p_detailed)
				{ // the detailed|long files
					FSPermissionInfo *t_permissions;
					t_permissions = (FSPermissionInfo *)&(t_catalog_infos[t_i] . permissions);
				
					t_tmp_context . copysvalue(t_native_name, t_native_length);
					MCU_urlencode(t_tmp_context);
				
					char t_filetype[9];
					if (!t_is_folder)
					{
						FileInfo *t_file_info;
						t_file_info = (FileInfo *) &t_catalog_infos[t_i] . finderInfo;
						uint4 t_creator;
						t_creator = MCSwapInt32NetworkToHost(t_file_info -> fileCreator);
						uint4 t_type;
						t_type = MCSwapInt32NetworkToHost(t_file_info -> fileType);
						
						if (t_file_info != NULL)
						{
							memcpy(t_filetype, (char*)&t_creator, 4);
							memcpy(&t_filetype[4], (char *)&t_type, 4);
							t_filetype[8] = '\0';
						}
						else
							t_filetype[0] = '\0';
					} else
						strcpy(t_filetype, "????????"); // this is what the "old" getentries did	

					CFAbsoluteTime t_creation_time;
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . createDate, &t_creation_time);
					t_creation_time += kCFAbsoluteTimeIntervalSince1970;

					CFAbsoluteTime t_modification_time;
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . contentModDate, &t_modification_time);
					t_modification_time += kCFAbsoluteTimeIntervalSince1970;

					CFAbsoluteTime t_access_time;
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . accessDate, &t_access_time);
					t_access_time += kCFAbsoluteTimeIntervalSince1970;

					CFAbsoluteTime t_backup_time;
					if (t_catalog_infos[t_i] . backupDate . highSeconds == 0 && t_catalog_infos[t_i] . backupDate . lowSeconds == 0 && t_catalog_infos[t_i] . backupDate . fraction == 0)
						t_backup_time = 0;
					else
					{
						UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . backupDate, &t_backup_time);
						t_backup_time += kCFAbsoluteTimeIntervalSince1970;
					}

					sprintf(t_buffer, "%*.*s,%llu,%llu,%.0lf,%.0lf,%.0lf,%.0lf,%d,%d,%03o,%.8s",
						t_tmp_context . getsvalue() . getlength(),  
					    t_tmp_context . getsvalue() . getlength(),  
					   	t_tmp_context . getsvalue() . getstring(),
						t_catalog_infos[t_i] . dataLogicalSize,
						t_catalog_infos[t_i] . rsrcLogicalSize,
						t_creation_time,
						t_modification_time,
						t_access_time,
						t_backup_time,
						t_permissions -> userID,
						t_permissions -> groupID,
						t_permissions -> mode & 0777,
						t_filetype);
						
					/* UNCHECKED */ MCListAppendCString(*t_list, t_buffer);
				}
				else
					/* UNCHECKED */ MCListAppendNativeChars(*t_list, (const char_t *)t_native_name, t_native_length);
					
				t_entry_count += 1;		
			}
		}	
	} while(t_oserror != errFSNoMoreItems);
	
	FSCloseIterator(t_catalog_iterator);
	return MCListCopy(*t_list, r_list);
#endif /* MCS_getentries_dsk_mac */  
        OSStatus t_os_status;
        
        Boolean t_is_folder;
        FSRef t_current_fsref;
        
        t_os_status = FSPathMakeRef((const UInt8 *)".", &t_current_fsref, &t_is_folder);
        if (t_os_status != noErr || !t_is_folder)
            return false;
        
        // Create the iterator, pass kFSIterateFlat to iterate over the current subtree only
        FSIterator t_catalog_iterator;
        t_os_status = FSOpenIterator(&t_current_fsref, kFSIterateFlat, &t_catalog_iterator);
        if (t_os_status != noErr)
            return false;
        
        uint4 t_entry_count;
        t_entry_count = 0;
        
        // Add ".." folder on Mac, for now impossible since there is no direct access to MCS_getentries_state type
//        if (!p_files)
//        {
//            t_entry_count++;
//            /* UNCHECKED */ MCListAppendCString(*t_list, "..");
//        }
        
        ItemCount t_max_objects, t_actual_objects;
        t_max_objects = CATALOG_MAX_ENTRIES;
        t_actual_objects = 0;
        FSCatalogInfo t_catalog_infos[CATALOG_MAX_ENTRIES];
        HFSUniStr255 t_names[CATALOG_MAX_ENTRIES];
        
        FSCatalogInfoBitmap t_info_bitmap;
        t_info_bitmap = kFSCatInfoAllDates |
        kFSCatInfoPermissions |
        kFSCatInfoUserAccess |
        kFSCatInfoFinderInfo |
        kFSCatInfoDataSizes |
        kFSCatInfoRsrcSizes |
        kFSCatInfoNodeFlags;
        
        MCExecPoint t_tmp_context(NULL, NULL, NULL);
        OSErr t_oserror;
        do
        {
            t_oserror = FSGetCatalogInfoBulk(t_catalog_iterator, t_max_objects, &t_actual_objects, NULL, t_info_bitmap, t_catalog_infos, NULL, NULL, t_names);
            if (t_oserror != noErr && t_oserror != errFSNoMoreItems)
            {	// clean up and exit
                FSCloseIterator(t_catalog_iterator);
                return false;
            }
            
            for(uint4 t_i = 0; t_i < (uint4)t_actual_objects; t_i++)
            {
                MCSystemFolderEntry t_entry;
                
                char t_native_name[256];
                uint4 t_native_length;
                t_native_length = 256;
                MCS_utf16tonative((const unsigned short *)t_names[t_i] . unicode, t_names[t_i] . length, t_native_name, t_native_length);
                // MCS_utf16tonative return a non nul-terminated string
                t_native_name[t_native_length] = '\0';
                
                // MW-2008-02-27: [[ Bug 5920 ]] Make sure we convert Finder to POSIX style paths
                for(uint4 i = 0; i < t_native_length; ++i)
                    if (t_native_name[i] == '/')
                        t_native_name[i] = ':';                

                FSPermissionInfo *t_permissions;
                t_permissions = (FSPermissionInfo *)&(t_catalog_infos[t_i] . permissions);
                
                uint32_t t_creator;
                uint32_t t_type;
                
                t_creator = 0;
                t_type = 0;
                
                if (!t_is_folder)
                {
                    FileInfo *t_file_info;
                    t_file_info = (FileInfo *) &t_catalog_infos[t_i] . finderInfo;
                    t_creator = MCSwapInt32NetworkToHost(t_file_info -> fileCreator);
                    t_type = MCSwapInt32NetworkToHost(t_file_info -> fileType);
                }
                
                CFAbsoluteTime t_creation_time;
                UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . createDate, &t_creation_time);
                t_creation_time += kCFAbsoluteTimeIntervalSince1970;
                
                CFAbsoluteTime t_modification_time;
                UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . contentModDate, &t_modification_time);
                t_modification_time += kCFAbsoluteTimeIntervalSince1970;
                
                CFAbsoluteTime t_access_time;
                UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . accessDate, &t_access_time);
                t_access_time += kCFAbsoluteTimeIntervalSince1970;
                
                CFAbsoluteTime t_backup_time;
                if (t_catalog_infos[t_i] . backupDate . highSeconds == 0 && t_catalog_infos[t_i] . backupDate . lowSeconds == 0 && t_catalog_infos[t_i] . backupDate . fraction == 0)
                    t_backup_time = 0;
                else
                {
                    UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . backupDate, &t_backup_time);
                    t_backup_time += kCFAbsoluteTimeIntervalSince1970;
                }
                
                t_entry.name = t_native_name;
                t_entry.data_size = t_catalog_infos[t_i] . dataLogicalSize;
                t_entry.resource_size = t_catalog_infos[t_i] . rsrcLogicalSize;
                t_entry.creation_time = (uint32_t)t_creation_time;
                t_entry.modification_time = (uint32_t) t_modification_time;
                t_entry.access_time = (uint32_t) t_access_time;
                t_entry.backup_time = (uint32_t) t_backup_time;
                t_entry.user_id = (uint32_t) t_permissions -> userID;
                t_entry.group_id = (uint32_t) t_permissions -> groupID;
                t_entry.permissions = (uint32_t) t_permissions->mode & 0777;
                t_entry.file_creator = t_creator;
                t_entry.file_type = t_type;
                t_entry.is_folder = t_catalog_infos[t_i] . nodeFlags & kFSNodeIsDirectoryMask;
            
                p_callback(x_context, &t_entry);
            }
        } while(t_oserror != errFSNoMoreItems);
        
        FSCloseIterator(t_catalog_iterator);
        
        return true;
    }
    
    virtual real8 GetFreeDiskSpace()
    {
#ifdef /* MCS_getfreediskspace_dsk_mac */ LEGACY_SYSTEM
	char t_defaultfolder[PATH_MAX + 1];
	getcwd(t_defaultfolder, PATH_MAX);
	
	FSRef t_defaultfolder_fsref;
	OSErr t_os_error;
	if (t_defaultfolder != NULL)
		t_os_error = FSPathMakeRef((const UInt8 *)t_defaultfolder, &t_defaultfolder_fsref, NULL);
		
	FSCatalogInfo t_catalog_info;
	if (t_os_error == noErr)
		t_os_error = FSGetCatalogInfo(&t_defaultfolder_fsref, kFSCatInfoVolume, &t_catalog_info, NULL, NULL, NULL);
	
	FSVolumeInfo t_volume_info;
	if (t_os_error == noErr)
		t_os_error = FSGetVolumeInfo(t_catalog_info . volume, 0, NULL, kFSVolInfoSizes, &t_volume_info, NULL, NULL);
		
	real8 t_free_space;
	t_free_space = 0.;
	
	// MH: freeBytes is a 64bit unsigned int, I follow previous functionality, and simply cast to real8.
	if (t_os_error == noErr)
		t_free_space = (real8) t_volume_info . freeBytes;
		
	return t_free_space;
#endif /* MCS_getfreediskspace_dsk_mac */
        char t_defaultfolder[PATH_MAX + 1];
        getcwd(t_defaultfolder, PATH_MAX);
        
        FSRef t_defaultfolder_fsref;
        OSErr t_os_error;
        if (t_defaultfolder != NULL)
            t_os_error = FSPathMakeRef((const UInt8 *)t_defaultfolder, &t_defaultfolder_fsref, NULL);
		
        FSCatalogInfo t_catalog_info;
        if (t_os_error == noErr)
            t_os_error = FSGetCatalogInfo(&t_defaultfolder_fsref, kFSCatInfoVolume, &t_catalog_info, NULL, NULL, NULL);
        
        FSVolumeInfo t_volume_info;
        if (t_os_error == noErr)
            t_os_error = FSGetVolumeInfo(t_catalog_info . volume, 0, NULL, kFSVolInfoSizes, &t_volume_info, NULL, NULL);
		
        real8 t_free_space;
        t_free_space = 0.;
        
        // MH: freeBytes is a 64bit unsigned int, I follow previous functionality, and simply cast to real8.
        if (t_os_error == noErr)
            t_free_space = (real8) t_volume_info . freeBytes;
		
        return t_free_space;
    }
    
    virtual Boolean GetDevices(MCStringRef& r_devices)
    {
#ifdef /* MCS_getdevices_dsk_mac */ LEGACY_SYSTEM
	MCAutoListRef t_list;
	io_iterator_t SerialPortIterator = NULL;
	mach_port_t masterPort = NULL;
	io_object_t thePort;
	if (FindSerialPortDevices(&SerialPortIterator, &masterPort) != KERN_SUCCESS)
	{
		char *buffer = new char[6 + I2L];
		sprintf(buffer, "error %d", errno);
		MCresult->copysvalue(buffer);
		delete buffer;
		return false;
	}
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	uint2 portCount = 0;

	bool t_success = true;
	if (SerialPortIterator != 0)
	{
		while (t_success && (thePort = IOIteratorNext(SerialPortIterator)) != 0)
		{
			char ioresultbuffer[256];

			MCAutoListRef t_result_list;
			MCAutoStringRef t_result_string;

			t_success = MCListCreateMutable(',', &t_result_list);

			if (t_success)
			{
				getIOKitProp(thePort, kIOTTYDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
				t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//name
			}
			if (t_success)
			{
				getIOKitProp(thePort, kIODialinDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
				t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//TTY file
			}
			if (t_success)
			{
				getIOKitProp(thePort, kIOCalloutDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
				t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//TTY file
			}

			if (t_success)
				t_success = MCListCopyAsStringAndRelease(*t_result_list, &t_result_string);

			if (t_success)
				t_success = MCListAppend(*t_list, *t_result_string);

			IOObjectRelease(thePort);
			portCount++;
		}
		IOObjectRelease(SerialPortIterator);
	}
	
	return t_success && MCListCopy(*t_list, r_list);
#endif /* MCS_getdevices_dsk_mac */
        MCAutoListRef t_list;
        io_iterator_t SerialPortIterator = NULL;
        mach_port_t masterPort = NULL;
        io_object_t thePort;
        if (FindSerialPortDevices(&SerialPortIterator, &masterPort) != KERN_SUCCESS)
        {
            char *buffer = new char[6 + I2L];
            sprintf(buffer, "error %d", errno);
            MCresult->copysvalue(buffer);
            delete buffer;
            return false;
        }
        if (!MCListCreateMutable('\n', &t_list))
            return false;
        
        uint2 portCount = 0;
        
        bool t_success = true;
        if (SerialPortIterator != 0)
        {
            while (t_success && (thePort = IOIteratorNext(SerialPortIterator)) != 0)
            {
                char ioresultbuffer[256];
                
                MCAutoListRef t_result_list;
                MCAutoStringRef t_result_string;
                
                t_success = MCListCreateMutable(',', &t_result_list);
                
                if (t_success)
                {
                    getIOKitProp(thePort, kIOTTYDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
                    t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//name
                }
                if (t_success)
                {
                    getIOKitProp(thePort, kIODialinDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
                    t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//TTY file
                }
                if (t_success)
                {
                    getIOKitProp(thePort, kIOCalloutDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
                    t_success = MCListAppendCString(*t_result_list, ioresultbuffer);//TTY file
                }
                
                if (t_success)
                    t_success = MCListCopyAsStringAndRelease(*t_result_list, &t_result_string);
                
                if (t_success)
                    t_success = MCListAppend(*t_list, *t_result_string);
                
                IOObjectRelease(thePort);
                portCount++;
            }
            IOObjectRelease(SerialPortIterator);
        }
        
        if (t_success && MCListCopyAsString(*t_list, r_devices))
            return True;
        
        return False;
    }
    
    virtual Boolean GetDrives(MCStringRef& r_drives)
    {
#ifdef /* MCS_getdrives_dsk_mac */ LEGACY_SYSTEM
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	OSErr t_err;
	ItemCount t_index;
	bool t_first;
	
	t_index = 1;
	t_err = noErr;
	t_first = true;
	
	// To list all the mounted volumes on the system we use the FSGetVolumeInfo
	// API with first parameter kFSInvalidVolumeRefNum and an index in the
	// second parameter.
	// This call will return nsvErr when it reaches the end of the list of
	// volumes, other errors being returned if there's a problem getting the
	// information.
	// Due to this, it is perfectly possible that the first index will not be
	// the first volume we put into the list - so we need a boolean flag (t_first)
	while(t_err != nsvErr)
	{
		HFSUniStr255 t_unicode_name;
		t_err = FSGetVolumeInfo(kFSInvalidVolumeRefNum, t_index, NULL, kFSVolInfoNone, NULL, &t_unicode_name, NULL);
		if (t_err == noErr)
		{
			MCAutoStringRef t_volume_name;
			if (!MCStringCreateWithChars(t_unicode_name . unicode, t_unicode_name . length, &t_volume_name))
				return false;
			if (!MCListAppend(*t_list, *t_volume_name))
				return false;
		}
		t_index += 1;
	}
	
	return MCListCopy(*t_list, r_list);
#endif /* MCS_getdrives_dsk_mac */
        MCAutoListRef t_list;
        if (!MCListCreateMutable('\n', &t_list))
            return false;
        
        OSErr t_err;
        ItemCount t_index;
        bool t_first;
        
        t_index = 1;
        t_err = noErr;
        t_first = true;
        
        // To list all the mounted volumes on the system we use the FSGetVolumeInfo
        // API with first parameter kFSInvalidVolumeRefNum and an index in the
        // second parameter.
        // This call will return nsvErr when it reaches the end of the list of
        // volumes, other errors being returned if there's a problem getting the
        // information.
        // Due to this, it is perfectly possible that the first index will not be
        // the first volume we put into the list - so we need a boolean flag (t_first)
        while(t_err != nsvErr)
        {
            HFSUniStr255 t_unicode_name;
            t_err = FSGetVolumeInfo(kFSInvalidVolumeRefNum, t_index, NULL, kFSVolInfoNone, NULL, &t_unicode_name, NULL);
            if (t_err == noErr)
            {
                MCAutoStringRef t_volume_name;
                if (!MCStringCreateWithChars(t_unicode_name . unicode, t_unicode_name . length, &t_volume_name))
                    return false;
                if (!MCListAppend(*t_list, *t_volume_name))
                    return false;
            }
            t_index += 1;
        }
        
        return MCListCopyAsString(*t_list, r_drives) ? True : False;
    }
	
	bool PathToNative(MCStringRef p_path, MCStringRef& r_native)
	{
        return MCStringCopy(p_path, r_native);
	}
	
	bool PathFromNative(MCStringRef p_native, MCStringRef& r_path)
	{
        return MCStringCopy(p_native, r_path);
	}
    
	virtual bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path)
    {
#ifdef /* MCS_resolvepath_dsk_mac */ LEGACY_SYSTEM
	if (MCStringGetLength(p_path) == 0)
		return MCS_getcurdir(r_resolved);

	MCAutoStringRef t_tilde_path;
	if (MCStringGetCharAtIndex(p_path, 0) == '~')
	{
		uindex_t t_user_end;
		if (!MCStringFirstIndexOfChar(p_path, '/', 0, kMCStringOptionCompareExact, t_user_end))
			t_user_end = MCStringGetLength(p_path);
		
		// Prepend user name
		struct passwd *t_password;
		if (t_user_end == 1)
			t_password = getpwuid(getuid());
		else
		{
			MCAutoStringRef t_username;
			if (!MCStringCopySubstring(p_path, MCRangeMake(1, t_user_end - 1), &t_username))
				return false;

			t_password = getpwnam(MCStringGetCString(*t_username));
		}
		
		if (t_password != NULL)
		{
			if (!MCStringCreateMutable(0, &t_tilde_path) ||
				!MCStringAppendNativeChars(*t_tilde_path, (char_t*)t_password->pw_dir, MCCStringLength(t_password->pw_dir)) ||
				!MCStringAppendSubstring(*t_tilde_path, p_path, MCRangeMake(t_user_end, MCStringGetLength(p_path) - t_user_end)))
				return false;
		}
		else
			t_tilde_path = p_path;
	}
	else
		t_tilde_path = p_path;

	MCAutoStringRef t_fullpath;
	if (MCStringGetCharAtIndex(*t_tilde_path, 0) != '/')
	{
		MCAutoStringRef t_folder;
		if (!MCS_getcurdir(&t_folder))
			return false;

		MCAutoStringRef t_resolved;
		if (!MCStringMutableCopy(*t_folder, &t_fullpath) ||
			!MCStringAppendChar(*t_fullpath, '/') ||
			!MCStringAppend(*t_fullpath, *t_tilde_path))
			return false;
	}
	else
		t_fullpath = *t_tilde_path;

	if (!MCS_is_link(*t_fullpath))
		return MCStringCopy(*t_fullpath, r_resolved);

	MCAutoStringRef t_newname;
	if (!MCS_readlink(*t_fullpath, &t_newname))
		return false;

	// IM - Should we really be using the original p_path parameter here?
	// seems like we should use the computed t_fullpath value.
	if (MCStringGetCharAtIndex(*t_newname, 0) != '/')
	{
		MCAutoStringRef t_resolved;

		uindex_t t_last_component;
		uindex_t t_path_length;

		if (MCStringLastIndexOfChar(p_path, '/', MCStringGetLength(p_path), kMCStringOptionCompareExact, t_last_component))
			t_last_component++;
		else
			t_last_component = 0;

		if (!MCStringMutableCopySubstring(p_path, MCRangeMake(0, t_last_component), &t_resolved) ||
			!MCStringAppend(*t_resolved, *t_newname))
			return false;

		return MCStringCopy(*t_resolved, r_resolved);
	}
	else
		return MCStringCopy(*t_newname, r_resolved);
#endif /* MCS_resolvepath_dsk_mac */
        if (MCStringGetLength(p_path) == 0)
            return GetCurrentFolder(r_resolved_path);
        
        MCAutoStringRef t_tilde_path;
        if (MCStringGetCharAtIndex(p_path, 0) == '~')
        {
            uindex_t t_user_end;
            if (!MCStringFirstIndexOfChar(p_path, '/', 0, kMCStringOptionCompareExact, t_user_end))
                t_user_end = MCStringGetLength(p_path);
            
            // Prepend user name
            struct passwd *t_password;
            if (t_user_end == 1)
                t_password = getpwuid(getuid());
            else
            {
                MCAutoStringRef t_username;
                if (!MCStringCopySubstring(p_path, MCRangeMake(1, t_user_end - 1), &t_username))
                    return false;
                
                t_password = getpwnam(MCStringGetCString(*t_username));
            }
            
            if (t_password != NULL)
            {
                if (!MCStringCreateMutable(0, &t_tilde_path) ||
                    !MCStringAppendNativeChars(*t_tilde_path, (char_t*)t_password->pw_dir, MCCStringLength(t_password->pw_dir)) ||
                    !MCStringAppendSubstring(*t_tilde_path, p_path, MCRangeMake(t_user_end, MCStringGetLength(p_path) - t_user_end)))
                    return false;
            }
            else
                t_tilde_path = p_path;
        }
        else
            t_tilde_path = p_path;
        
        MCAutoStringRef t_fullpath;
        if (MCStringGetCharAtIndex(*t_tilde_path, 0) != '/')
        {
            MCAutoStringRef t_folder;
            /* UNCHECKED */ GetCurrentFolder(&t_folder);
            
            MCAutoStringRef t_resolved;
            if (!MCStringMutableCopy(*t_folder, &t_fullpath) ||
                !MCStringAppendChar(*t_fullpath, '/') ||
                !MCStringAppend(*t_fullpath, *t_tilde_path))
                return false;
        }
        else
            t_fullpath = *t_tilde_path;
        
        if (!MCS_mac_is_link(*t_fullpath))
            return MCStringCopy(*t_fullpath, r_resolved_path);
        
        MCAutoStringRef t_newname;
        if (!MCS_mac_readlink(*t_fullpath, &t_newname))
            return false;
        
        // IM - Should we really be using the original p_path parameter here?
        // seems like we should use the computed t_fullpath value.
        if (MCStringGetCharAtIndex(*t_newname, 0) != '/')
        {
            MCAutoStringRef t_resolved;
            
            uindex_t t_last_component;
            uindex_t t_path_length;
            
            if (MCStringLastIndexOfChar(p_path, '/', MCStringGetLength(p_path), kMCStringOptionCompareExact, t_last_component))
                t_last_component++;
            else
                t_last_component = 0;
            
            if (!MCStringMutableCopySubstring(p_path, MCRangeMake(0, t_last_component), &t_resolved) ||
                !MCStringAppend(*t_resolved, *t_newname))
                return false;
            
            return MCStringCopy(*t_resolved, r_resolved_path);
        }
        else
            return MCStringCopy(*t_newname, r_resolved_path);
    }
	
	virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map)
    {
#ifdef /* MCS_open_dsk_mac */ LEGACY_SYSTEM
        IO_handle handle = NULL;
		//opening regular files
		//set the file type and it's creator. These are 2 global variables
		char *oldpath = strclone(path);
		
		// OK-2008-01-10 : Bug 5764. Check here that MCS_resolvepath does not return NULL
		char *t_resolved_path;
		t_resolved_path = MCS_resolvepath(path);
		if (t_resolved_path == NULL)
			return NULL;
		
		char *newpath = path2utf(t_resolved_path);
		FILE *fptr;
        
		if (driver)
		{
			fptr = fopen(newpath,  mode );
			if (fptr != NULL)
			{
				int val;
				val = fcntl(fileno(fptr), F_GETFL, val);
				val |= O_NONBLOCK |  O_NOCTTY;
				fcntl(fileno(fptr), F_SETFL, val);
				configureSerialPort((short)fileno(fptr));
			}
		}
		else
		{
			fptr = fopen(newpath, IO_READ_MODE);
			if (fptr == NULL)
				fptr = fopen(oldpath, IO_READ_MODE);
			Boolean created = True;
			if (fptr != NULL)
			{
				created = False;
				if (mode != IO_READ_MODE)
				{
					fclose(fptr);
					fptr = NULL;
				}
			}
			if (fptr == NULL)
				fptr = fopen(newpath, mode);
            
			if (fptr == NULL && !strequal(mode, IO_READ_MODE))
				fptr = fopen(newpath, IO_CREATE_MODE);
			if (fptr != NULL && created)
				MCS_setfiletype(oldpath);
		}
        
		delete newpath;
		delete oldpath;
		if (fptr != NULL)
		{
			handle = new IO_header(fptr, 0, 0, 0, NULL, 0, 0);
			if (offset > 0)
				fseek(handle->fptr, offset, SEEK_SET);
            
			if (strequal(mode, IO_APPEND_MODE))
				handle->flags |= IO_SEEKED;
		}
        
        return handle;
#endif /* MCS_open_dsk_mac */
		FILE *fptr;
        IO_handle t_handle;
        t_handle = NULL;
		//opening regular files
		//set the file type and it's creator. These are 2 global variables
        
        MCAutoStringRefAsUTF8String t_path_utf;
        if (!t_path_utf.Lock(p_path))
            return NULL;
        
        fptr = fopen(*t_path_utf, IO_READ_MODE);
        
        Boolean created = True;
        
        if (fptr != NULL)
        {
            created = False;
            if (p_mode != kMCSystemFileModeRead)
            {
                fclose(fptr);
                fptr = NULL;
            }
        }
        
        if (fptr == NULL)
        {
            switch(p_mode)
            {
                case kMCSystemFileModeRead:
                    fptr = fopen(*t_path_utf, IO_READ_MODE);
                    break;
                case kMCSystemFileModeUpdate:
                    fptr = fopen(*t_path_utf, IO_UPDATE_MODE);
                    break;
                case kMCSystemFileModeAppend:
                    fptr = fopen(*t_path_utf, IO_APPEND_MODE);
                    break;
                case kMCSystemFileModeWrite:
                    fptr = fopen(*t_path_utf, IO_WRITE_MODE);
                    break;
                default:
                    fptr = NULL;
            }
        }
        
        if (fptr == NULL && p_mode != kMCSystemFileModeRead)
            fptr = fopen(*t_path_utf, IO_CREATE_MODE);
        
        if (fptr != NULL && created)
            MCS_mac_setfiletype(p_path);
        
		if (fptr != NULL)
            t_handle = new MCStdioFileHandle(fptr);
        
        return t_handle;
    }
    
	virtual IO_handle OpenFd(uint32_t p_fd, intenum_t p_mode)
    {
#ifdef /* MCS_dopen_dsk_mac */ LEGACY_SYSTEM
        IO_handle handle = NULL;
        FILE *fptr = fdopen(fd, mode);
        
        if (fptr != NULL)
        {
            // MH-2007-05-17: [[Bug 3196]] Opening the write pipe to a process should not be buffered.
            if (mode[0] == 'w')
                setvbuf(fptr, NULL, _IONBF, 0);
            
            handle = new IO_header(fptr, 0, 0, NULL, NULL, 0, 0);
        }
        return handle;
#endif /* MCS_dopen_dsk_mac */
		FILE *t_stream;
        t_stream = NULL;
        
        switch (p_mode)
        {
            case kMCSystemFileModeAppend:
                t_stream = fdopen(p_fd, IO_APPEND_MODE);
                break;
            case kMCSystemFileModeRead:
                t_stream = fdopen(p_fd, IO_READ_MODE);
                break;
            case kMCSystemFileModeUpdate:
                t_stream = fdopen(p_fd, IO_UPDATE_MODE);
                break;
            case kMCSystemFileModeWrite:
                t_stream = fdopen(p_fd, IO_WRITE_MODE);
                break;
            default:
                break;
        }
        
		if (t_stream == NULL)
			return NULL;
		
		// MH-2007-05-17: [[Bug 3196]] Opening the write pipe to a process should not be buffered.
		if (p_mode == kMCSystemFileModeWrite)
			setvbuf(t_stream, NULL, _IONBF, 0);
		
		IO_handle t_handle;
		t_handle = new MCStdioFileHandle(t_stream);
		
		return t_handle;
    }
    
	virtual IO_handle OpenDevice(MCStringRef p_path, intenum_t p_mode)
    {
		FILE *fptr;
        
        IO_handle t_handle;
        t_handle = NULL;
		//opening regular files
		//set the file type and it's creator. These are 2 global variables
        
        MCAutoStringRefAsUTF8String t_path_utf;
        if (!t_path_utf.Lock(p_path))
            return NULL;
        
        fptr = fopen(*t_path_utf, IO_READ_MODE);
        
		if (fptr != NULL)
        {
            int val;
            int t_serial_in;
            
            t_serial_in = fileno(fptr);
            val = fcntl(t_serial_in, F_GETFL, val);
            val |= O_NONBLOCK |  O_NOCTTY;
            fcntl(t_serial_in, F_SETFL, val);
            configureSerialPort((short)t_serial_in);
            
            t_handle = new MCSerialPortFileHandle(t_serial_in);
        }
        
        return t_handle;
    }
	
	virtual MCSysModuleHandle LoadModule(MCStringRef p_filename)
    {
#ifdef /* MCS_loadmodule_dsk_mac */ LEGACY_SYSTEM
	char *t_native_path;
	t_native_path = path2utf(MCS_resolvepath(p_filename));
	
	CFURLRef t_url;
	t_url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)t_native_path, strlen(t_native_path), False);
	delete t_native_path;
	
	if (t_url == NULL)
		return NULL;
    
	MCSysModuleHandle t_result;
	t_result = (MCSysModuleHandle)CFBundleCreate(NULL, t_url);
	
	CFRelease(t_url);
	
	return (MCSysModuleHandle)t_result;
#endif /* MCS_loadmodule_dsk_mac */
        MCAutoStringRefAsUTF8String t_utf_path;
        
        if (!t_utf_path.Lock(p_filename))
            return NULL;
        
        CFURLRef t_url;
        t_url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)*t_utf_path, strlen(*t_utf_path), False);
        
        if (t_url == NULL)
            return NULL;
		
        MCSysModuleHandle t_result;
        t_result = (MCSysModuleHandle)CFBundleCreate(NULL, t_url);
        
        CFRelease(t_url);
        
        return (MCSysModuleHandle)t_result;
    }
    
	virtual MCSysModuleHandle ResolveModuleSymbol(MCSysModuleHandle p_module, MCStringRef p_symbol)
    {
#ifdef /* MCS_resolvemodulesymbol_dsk_mac */ LEGACY_SYSTEM
	CFStringRef t_cf_symbol;
	t_cf_symbol = CFStringCreateWithCString(NULL, p_symbol, CFStringGetSystemEncoding());
	if (t_cf_symbol == NULL)
		return NULL;
	
	void *t_symbol_ptr;
	t_symbol_ptr = CFBundleGetFunctionPointerForName((CFBundleRef)p_module, t_cf_symbol);
	
	CFRelease(t_cf_symbol);
	
	return t_symbol_ptr;
#endif /* MCS_resolvemodulesymbol_dsk_mac */
        CFStringRef t_cf_symbol;
        t_cf_symbol = CFStringCreateWithCString(NULL, MCStringGetCString(p_symbol), CFStringGetSystemEncoding());
        if (t_cf_symbol == NULL)
            return NULL;
        
        void *t_symbol_ptr;
        t_symbol_ptr = CFBundleGetFunctionPointerForName((CFBundleRef)p_module, t_cf_symbol);
        
        CFRelease(t_cf_symbol);
        
        return (MCSysModuleHandle) t_symbol_ptr;
    }
    
	virtual void UnloadModule(MCSysModuleHandle p_module)
    {
#ifdef /* MCS_unloadmodule_dsk_mac */ LEGACY_SYSTEM
	CFRelease((CFBundleRef)p_module);
#endif /* MCS_unloadmodule_dsk_mac */
        CFRelease((CFBundleRef)p_module);        
    }
	
	virtual bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
    {
#ifdef /* MCS_longfilepath_dsk_mac */ LEGACY_SYSTEM
	return MCStringCopy(p_path, r_long_path);
#endif /* MCS_longfilepath_dsk_mac */
        return MCStringCopy(p_path, r_long_path);
    }
    
	virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
    {
#ifdef /* MCS_shortfilepath_dsk_mac */ LEGACY_SYSTEM
	return MCStringCopy(p_path, r_short_path);
#endif /* MCS_shortfilepath_dsk_mac */
        return MCStringCopy(p_path, r_short_path);
    }
    
	virtual uint32_t TextConvert(const void *p_string, uint32_t p_string_length, void *r_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset)
    {
#ifdef /* MCS_multibytetounicode_dsk_mac */ LEGACY_SYSTEM
        // MW-2012-06-14: [[ Bug ]] If used for charset 0 before any other, causes a crash.
        static int oldcharset = -1;
        if (!destbufferlength)
        {
            destlen = len << 1;
            return;
        }
        if (charset != oldcharset)
        {
            if (texttounicodeconvertor)
                DisposeTextToUnicodeInfo(texttounicodeconvertor);
            texttounicodeconvertor = NULL;
            ScriptCode fscript = MCS_charsettolangid(charset);
            TextEncoding scriptEncoding;
            UpgradeScriptInfoToTextEncoding(fscript, kTextLanguageDontCare,
                                            kTextRegionDontCare, NULL,
                                            &scriptEncoding);
            texttounicodeconvertor = &texttounicodeinfo;
            CreateTextToUnicodeInfoByEncoding(scriptEncoding, texttounicodeconvertor);
        }
        ByteCount processedbytes, outlength;
        ConvertFromTextToUnicode(*texttounicodeconvertor, len, (LogicalAddress) s,
                                 kUnicodeLooseMappingsMask
                                 | kUnicodeUseFallbacksMask, 0, NULL, 0, NULL,
                                 destbufferlength, &processedbytes,
                                 &outlength, (UniChar *)d);
        destlen = outlength;
        oldcharset = charset;
#endif /* MCS_multibytetounicode_dsk_mac */
#ifdef /* MCS_unicodetomultibyte_dsk_mac */ LEGACY_SYSTEM
	ScriptCode fscript = MCS_charsettolangid(charset);
	//we cache unicode convertors for speed
	if (!destbufferlength)
	{
		if (charset)
			destlen = len << 1;
		else
			destlen = len >> 1;
		return;
	}
	if (unicodeconvertors[fscript] == NULL)
	{
		TextEncoding scriptEncoding;
		UpgradeScriptInfoToTextEncoding(fscript, kTextLanguageDontCare,
		                                kTextRegionDontCare, NULL,
		                                &scriptEncoding);
		CreateUnicodeToTextInfoByEncoding(scriptEncoding,
		                                  &unicodeconvertors[fscript]);
	}
	ByteCount processedbytes, outlength;
	destlen = 0;
	
	// MW-2008-06-12: [[ Bug 6313 ]] Loop through all input characters, replacing unknown
	//   ones with ? - this mimics Windows behaviour.
	// MW-2008-06-12: Make sure we loop until we have no pairs of bytes left otherwise
	//   we go into an infinite loop when doing things like uniDecode("abc")
	while(len > 1)
	{
		ConvertFromUnicodeToText(unicodeconvertors[fscript], len, (UniChar *)s,
								 kUnicodeLooseMappingsMask
								 | kUnicodeStringUnterminatedBit
								 | kUnicodeUseFallbacksBit, 0, NULL, 0, NULL,
								 destbufferlength, &processedbytes,
								 &outlength, (LogicalAddress)d);
		if (processedbytes == 0)
		{
			*d = '?';
			processedbytes = 2;
			outlength = 1;
		}
        
		len -= processedbytes;
		destlen += outlength;
		s += processedbytes;
		d += outlength;
	}
#endif /* MCS_unicodetomultibyte_dsk_mac */
        uint32_t t_return_size;
        if (p_from_charset == LCH_UNICODE) // Unicode to multibyte
        {
//            TextConvert(const void *p_string, uint32_t p_string_length, void *r_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset)
            //            (const char *s, uint4 len, char *d, uint4 destbufferlength, uint4 &destlen, uint1 charset)
            char* t_dest_ptr = (char*) r_buffer;
            char* t_src_ptr = (char*)p_string;
            ScriptCode fscript = MCS_charsettolangid(p_to_charset);
            //we cache unicode convertors for speed
            if (!p_buffer_length)
            {
                if (p_to_charset)
                    t_return_size = p_string_length << 1;
                else
                    t_return_size = p_string_length >> 1;
                return t_return_size;
            }
            if (unicodeconvertors[fscript] == NULL)
            {
                TextEncoding scriptEncoding;
                UpgradeScriptInfoToTextEncoding(fscript, kTextLanguageDontCare,
                                                kTextRegionDontCare, NULL,
                                                &scriptEncoding);
                CreateUnicodeToTextInfoByEncoding(scriptEncoding,
                                                  &unicodeconvertors[fscript]);
            }
            ByteCount processedbytes, outlength;
            t_return_size = 0;
            
            // MW-2008-06-12: [[ Bug 6313 ]] Loop through all input characters, replacing unknown
            //   ones with ? - this mimics Windows behaviour.
            // MW-2008-06-12: Make sure we loop until we have no pairs of bytes left otherwise
            //   we go into an infinite loop when doing things like uniDecode("abc")
            while(p_string_length > 1)
            {
                ConvertFromUnicodeToText(unicodeconvertors[fscript], p_string_length, (UniChar *)t_src_ptr,
                                         kUnicodeLooseMappingsMask
                                         | kUnicodeStringUnterminatedBit
                                         | kUnicodeUseFallbacksBit, 0, NULL, 0, NULL,
                                         p_buffer_length, &processedbytes,
                                         &outlength, (LogicalAddress)r_buffer);
                if (processedbytes == 0)
                {
                    *t_dest_ptr = '?';
                    processedbytes = 2;
                    outlength = 1;
                }
                
                p_string_length -= processedbytes;
                t_return_size += outlength;
                t_src_ptr += processedbytes;
                t_dest_ptr += outlength;
            }
        }
        else if (p_to_charset == LCH_UNICODE) // Multibyte to unicode
        {
//            (const char *s, uint4 len, char *d, uint4 destbufferlength, uint4 &destlen, uint1 charset)
            // MW-2012-06-14: [[ Bug ]] If used for charset 0 before any other, causes a crash.
            static int oldcharset = -1;
            if (!p_buffer_length)
            {
                return p_string_length << 1;
            }
            if (p_from_charset != oldcharset)
            {
                if (texttounicodeconvertor)
                    DisposeTextToUnicodeInfo(texttounicodeconvertor);
                texttounicodeconvertor = NULL;
                ScriptCode fscript = MCS_charsettolangid(p_from_charset);
                TextEncoding scriptEncoding;
                UpgradeScriptInfoToTextEncoding(fscript, kTextLanguageDontCare,
                                                kTextRegionDontCare, NULL,
                                                &scriptEncoding);
                texttounicodeconvertor = &texttounicodeinfo;
                CreateTextToUnicodeInfoByEncoding(scriptEncoding, texttounicodeconvertor);
            }
            ByteCount processedbytes, outlength;
            ConvertFromTextToUnicode(*texttounicodeconvertor, p_string_length, (LogicalAddress) p_string,
                                     kUnicodeLooseMappingsMask
                                     | kUnicodeUseFallbacksMask, 0, NULL, 0, NULL,
                                     p_buffer_length, &processedbytes,
                                     &outlength, (UniChar *)r_buffer);
            t_return_size = outlength;
            oldcharset = p_from_charset;
        }
        return t_return_size;
    }
    
	virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4& p_output_length, uint4& r_used)
    {
#ifdef /* MCSTextConvertToUnicode_dsk_mac */ LEGACY_SYSTEM
	if (p_input_length == 0)
	{
		r_used = 0;
		return true;
	}

	int4 t_encoding;
	t_encoding = -1;
	
	if (p_input_encoding >= kMCTextEncodingWindowsNative)
	{
		struct { uint4 codepage; int4 encoding; } s_codepage_map[] =
		{
			{437, kTextEncodingDOSLatinUS },
			{850, kTextEncodingDOSLatinUS },
			{932, kTextEncodingDOSJapanese },
			{949, kTextEncodingDOSKorean },
			{1361, kTextEncodingWindowsKoreanJohab },
			{936, kTextEncodingDOSChineseSimplif },
			{950, kTextEncodingDOSChineseTrad },
			{1253, kTextEncodingWindowsGreek },
			{1254, kTextEncodingWindowsLatin5 },
			{1258, kTextEncodingWindowsVietnamese },
			{1255, kTextEncodingWindowsHebrew },
			{1256, kTextEncodingWindowsArabic },
			{1257, kTextEncodingWindowsBalticRim },
			{1251, kTextEncodingWindowsCyrillic },
			{874, kTextEncodingDOSThai },
			{1250, kTextEncodingWindowsLatin2 },
			{1252, kTextEncodingWindowsLatin1 }
		};
		
		for(uint4 i = 0; i < sizeof(s_codepage_map) / sizeof(s_codepage_map[0]); ++i)
			if (s_codepage_map[i] . codepage == p_input_encoding - kMCTextEncodingWindowsNative)
			{
				t_encoding = s_codepage_map[i] . encoding;
				break;
			}
			
		// MW-2008-03-24: [[ Bug 6187 ]] RTF parser doesn't like ansicpg1000
		if (t_encoding == -1 && (p_input_encoding - kMCTextEncodingWindowsNative >= 10000))
			t_encoding = p_input_encoding - kMCTextEncodingWindowsNative - 10000;
			
	}
	else if (p_input_encoding >= kMCTextEncodingMacNative)
		t_encoding = p_input_encoding - kMCTextEncodingMacNative;
	
	TextToUnicodeInfo t_info;
	t_info = fetch_unicode_info(t_encoding);
	
	if (t_info == NULL)
	{
		r_used = 0;
		return true;
	}
	
	ByteCount t_source_read, t_unicode_length;
	if (ConvertFromTextToUnicode(t_info, p_input_length, p_input, 0, 0, (ByteOffset *)NULL, (ItemCount *)NULL, NULL, p_output_length, &t_source_read, &t_unicode_length, (UniChar *)p_output) != noErr)
	{
		r_used = 4 * p_input_length;
		return false;
	}

	r_used = t_unicode_length;
	
	return true;
#endif /* MCSTextConvertToUnicode_dsk_mac */
        if (p_input_length == 0)
        {
            r_used = 0;
            return true;
        }
        
        int4 t_encoding;
        t_encoding = -1;
        
        if (p_input_encoding >= kMCTextEncodingWindowsNative)
        {
            struct { uint4 codepage; int4 encoding; } s_codepage_map[] =
            {
                {437, kTextEncodingDOSLatinUS },
                {850, kTextEncodingDOSLatinUS },
                {932, kTextEncodingDOSJapanese },
                {949, kTextEncodingDOSKorean },
                {1361, kTextEncodingWindowsKoreanJohab },
                {936, kTextEncodingDOSChineseSimplif },
                {950, kTextEncodingDOSChineseTrad },
                {1253, kTextEncodingWindowsGreek },
                {1254, kTextEncodingWindowsLatin5 },
                {1258, kTextEncodingWindowsVietnamese },
                {1255, kTextEncodingWindowsHebrew },
                {1256, kTextEncodingWindowsArabic },
                {1257, kTextEncodingWindowsBalticRim },
                {1251, kTextEncodingWindowsCyrillic },
                {874, kTextEncodingDOSThai },
                {1250, kTextEncodingWindowsLatin2 },
                {1252, kTextEncodingWindowsLatin1 }
            };
            
            for(uint4 i = 0; i < sizeof(s_codepage_map) / sizeof(s_codepage_map[0]); ++i)
                if (s_codepage_map[i] . codepage == p_input_encoding - kMCTextEncodingWindowsNative)
                {
                    t_encoding = s_codepage_map[i] . encoding;
                    break;
                }
			
            // MW-2008-03-24: [[ Bug 6187 ]] RTF parser doesn't like ansicpg1000
            if (t_encoding == -1 && (p_input_encoding - kMCTextEncodingWindowsNative >= 10000))
                t_encoding = p_input_encoding - kMCTextEncodingWindowsNative - 10000;
			
        }
        else if (p_input_encoding >= kMCTextEncodingMacNative)
            t_encoding = p_input_encoding - kMCTextEncodingMacNative;
        
        TextToUnicodeInfo t_info;
        t_info = fetch_unicode_info(t_encoding);
        
        if (t_info == NULL)
        {
            r_used = 0;
            return true;
        }
        
        ByteCount t_source_read, t_unicode_length;
        if (ConvertFromTextToUnicode(t_info, p_input_length, p_input, 0, 0, (ByteOffset *)NULL, (ItemCount *)NULL, NULL, p_output_length, &t_source_read, &t_unicode_length, (UniChar *)p_output) != noErr)
        {
            r_used = 4 * p_input_length;
            return false;
        }
        
        r_used = t_unicode_length;
        
        return true;
    }
    
    virtual void CheckProcesses(void)
    {
#ifdef /* MCS_checkprocesses_dsk_mac */ LEGACY_SYSTEM
	uint2 i;
	int wstat;
	for (i = 0 ; i < MCnprocesses ; i++)
		if (MCprocesses[i].pid != 0 && MCprocesses[i].pid != -1
		        && waitpid(MCprocesses[i].pid, &wstat, WNOHANG) > 0)
		{
			if (MCprocesses[i].ihandle != NULL)
				clearerr(MCprocesses[i].ihandle->fptr);
			MCprocesses[i].pid = 0;
			MCprocesses[i].retcode = WEXITSTATUS(wstat);
		}


#endif /* MCS_checkprocesses_dsk_mac */
        uint2 i;
        int wstat;
        for (i = 0 ; i < MCnprocesses ; i++)
            if (MCprocesses[i].pid != 0 && MCprocesses[i].pid != -1
		        && waitpid(MCprocesses[i].pid, &wstat, WNOHANG) > 0)
            {
                if (MCprocesses[i].ihandle != NULL)
                {
                    MCStdioFileHandle *t_handle;
                    t_handle = static_cast<MCStdioFileHandle *>(MCprocesses[i].ihandle);
                    clearerr(t_handle -> GetStream());
                }
                MCprocesses[i].pid = 0;
                MCprocesses[i].retcode = WEXITSTATUS(wstat);
            }
    }
    
    virtual uint32_t GetSystemError(void)
    {
#ifdef /* MCS_getsyserror_dsk_mac */ LEGACY_SYSTEM
	return errno;
#endif /* MCS_getsyserror_dsk_mac */
        return errno;
    }

    virtual bool Shell(MCStringRef p_command, MCDataRef& r_data, int& r_retcode)
    {
#ifdef /* MCS_runcmd_dsk_mac */ LEGACY_SYSTEM

	IO_cleanprocesses();
	int tochild[2];
	int toparent[2];
	int4 index = MCnprocesses;
	if (pipe(tochild) == 0)
	{
		if (pipe(toparent) == 0)
		{
			MCU_realloc((char **)&MCprocesses, MCnprocesses,
									MCnprocesses + 1, sizeof(Streamnode));
			MCprocesses[MCnprocesses].name = (MCNameRef)MCValueRetain(MCM_shell);
			MCprocesses[MCnprocesses].mode = OM_NEITHER;
			MCprocesses[MCnprocesses].ohandle = NULL;
			MCprocesses[MCnprocesses].ihandle = NULL;
			if ((MCprocesses[MCnprocesses++].pid = fork()) == 0)
			{
				close(tochild[1]);
				close(0);
				dup(tochild[0]);
				close(tochild[0]);
				close(toparent[0]);
				close(1);
				dup(toparent[1]);
				close(2);
				dup(toparent[1]);
				close(toparent[1]);
				execl(MCshellcmd, MCshellcmd, "-s", NULL);
				_exit(-1);
			}
			MCS_checkprocesses();
			close(tochild[0]);
			char *str = path2utf(ep.getsvalue().clone());
			write(tochild[1], str, strlen(str));
			delete str;
			write(tochild[1], "\n", 1);
			close(tochild[1]);
			close(toparent[1]);
			MCS_nodelay(toparent[0]);
			if (MCprocesses[index].pid == -1)
			{
				if (MCprocesses[index].pid > 0)
					MCS_kill(MCprocesses[index].pid, SIGKILL);
				MCprocesses[index].pid = 0;
				MCeerror->add
				(EE_SHELL_BADCOMMAND, 0, 0, ep.getsvalue());
				return IO_ERROR;
			}
		}
		else
		{
			close(tochild[0]);
			close(tochild[1]);
			MCeerror->add
			(EE_SHELL_BADCOMMAND, 0, 0, ep.getsvalue());
			return IO_ERROR;
		}
	}
	else
	{
		MCeerror->add
		(EE_SHELL_BADCOMMAND, 0, 0, ep.getsvalue());
		return IO_ERROR;
	}
	char *buffer;
	uint4 buffersize;
	buffer = (char *)malloc(4096);
	buffersize = 4096;
	uint4 size = 0;
	if (MCS_shellread(toparent[0], buffer, buffersize, size) != IO_NORMAL)
	{
		MCeerror->add(EE_SHELL_ABORT, 0, 0);
		close(toparent[0]);
		if (MCprocesses[index].pid != 0)
			MCS_kill(MCprocesses[index].pid, SIGKILL);
		ep.grabbuffer(buffer, size);
		return IO_ERROR;
	}
	ep.grabbuffer(buffer, size);
	close(toparent[0]);
	MCS_checkprocesses();
	if (MCprocesses[index].pid != 0)
	{
		uint2 count = SHELL_COUNT;
		while (count--)
		{
			if (MCscreen->wait(SHELL_INTERVAL, False, False))
			{
				if (MCprocesses[index].pid != 0)
					MCS_kill(MCprocesses[index].pid, SIGKILL);
				return IO_ERROR;
			}
			if (MCprocesses[index].pid == 0)
				break;
		}
		if (MCprocesses[index].pid != 0)
		{
			MCprocesses[index].retcode = -1;
			MCS_kill(MCprocesses[index].pid, SIGKILL);
		}
	}
	if (MCprocesses[index].retcode)
	{
		MCExecPoint ep2(ep);
		ep2.setint(MCprocesses[index].retcode);
		MCresult->set(ep2);
	}
	else
		MCresult->clear(False);


	return IO_NORMAL;
#endif /* MCS_runcmd_dsk_mac */
        IO_cleanprocesses();
        int tochild[2];
        int toparent[2];
        int4 index = MCnprocesses;
        if (pipe(tochild) == 0)
        {
            if (pipe(toparent) == 0)
            {
                MCU_realloc((char **)&MCprocesses, MCnprocesses,
                            MCnprocesses + 1, sizeof(Streamnode));
                MCprocesses[MCnprocesses].name = (MCNameRef)MCValueRetain(MCM_shell);
                MCprocesses[MCnprocesses].mode = OM_NEITHER;
                MCprocesses[MCnprocesses].ohandle = NULL;
                MCprocesses[MCnprocesses].ihandle = NULL;
                if ((MCprocesses[MCnprocesses++].pid = fork()) == 0)
                {
                    close(tochild[1]);
                    close(0);
                    dup(tochild[0]);
                    close(tochild[0]);
                    close(toparent[0]);
                    close(1);
                    dup(toparent[1]);
                    close(2);
                    dup(toparent[1]);
                    close(toparent[1]);
                    execl(MCStringGetCString(MCshellcmd), MCStringGetCString(MCshellcmd), "-s", NULL);
                    _exit(-1);
                }
                CheckProcesses();
                close(tochild[0]);
                
                MCAutoStringRefAsUTF8String t_utf_path;
                /* UNCHECKED */ t_utf_path . Lock(p_command);
                write(tochild[1], *t_utf_path, strlen(*t_utf_path));

                write(tochild[1], "\n", 1);
                close(tochild[1]);
                close(toparent[1]);
                MCS_mac_nodelay(toparent[0]);
                if (MCprocesses[index].pid == -1)
                {
                    if (MCprocesses[index].pid > 0)
                        Kill(MCprocesses[index].pid, SIGKILL);
                    MCprocesses[index].pid = 0;
                    MCeerror->add
                    (EE_SHELL_BADCOMMAND, 0, 0, MCStringGetCString(p_command));
                    return IO_ERROR;
                }
            }
            else
            {
                close(tochild[0]);
                close(tochild[1]);
                MCeerror->add
                (EE_SHELL_BADCOMMAND, 0, 0, MCStringGetCString(p_command));
                return IO_ERROR;
            }
        }
        else
        {
            MCeerror->add
            (EE_SHELL_BADCOMMAND, 0, 0, MCStringGetCString(p_command));
            return IO_ERROR;
        }
        char *buffer;
        uint4 buffersize;
        buffer = (char *)malloc(4096);
        buffersize = 4096;
        uint4 size = 0;
        if (MCS_mac_shellread(toparent[0], buffer, buffersize, size) != IO_NORMAL)
        {
            MCeerror->add(EE_SHELL_ABORT, 0, 0);
            close(toparent[0]);
            if (MCprocesses[index].pid != 0)
                Kill(MCprocesses[index].pid, SIGKILL);
            /* UNCHECKED */ MCDataCreateWithBytes((char_t*)buffer, size, r_data);
            return false;
        }
        /* UNCHECKED */ MCDataCreateWithBytes((char_t*)buffer, size, r_data);
        close(toparent[0]);
        CheckProcesses();
        if (MCprocesses[index].pid != 0)
        {
            uint2 count = SHELL_COUNT;
            while (count--)
            {
                if (MCscreen->wait(SHELL_INTERVAL, False, False))
                {
                    if (MCprocesses[index].pid != 0)
                        Kill(MCprocesses[index].pid, SIGKILL);
                    return IO_ERROR;
                }
                if (MCprocesses[index].pid == 0)
                    break;
            }
            if (MCprocesses[index].pid != 0)
            {
                MCprocesses[index].retcode = -1;
                Kill(MCprocesses[index].pid, SIGKILL);
            }
        }
        
        r_retcode = MCprocesses[index].retcode;        
        
        return IO_NORMAL;
    }
    
    virtual bool StartProcess(MCNameRef p_name, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated)
    {
#ifdef /* MCS_startprocess_dsk_mac */ LEGACY_SYSTEM
	if (MCStringEndsWithCString(MCNameGetString(name), (const char_t *)".app", kMCStringOptionCompareCaseless) || MCStringGetLength(docname) != 0))
	  MCS_startprocess_launch(name, docname, mode);
	else
	  MCS_startprocess_unix(name, kMCEmptyString, mode, elevated);
#endif /* MCS_startprocess_dsk_mac */
        if (MCStringEndsWithCString(MCNameGetString(p_name), (const char_t *)".app", kMCStringOptionCompareCaseless) || (p_doc != nil && MCStringGetLength(p_doc) != 0))
            MCS_startprocess_launch(p_name, p_doc, (Open_mode)p_mode);
        else
            MCS_startprocess_unix(p_name, kMCEmptyString, (Open_mode)p_mode, p_elevated);
    }
    
    virtual bool ProcessTypeIsForeground(void)
    {
#ifdef /* MCS_processtypeisforeground_dsk_mac */ LEGACY_SYSTEM
	ProcessSerialNumber t_psn = { 0, kCurrentProcess };
	
	CFDictionaryRef t_info;
	t_info = ProcessInformationCopyDictionary(&t_psn, kProcessDictionaryIncludeAllInformationMask);
	
	bool t_result;
	t_result = true;
	if (t_info != NULL)
	{
		CFBooleanRef t_value;
		t_value = (CFBooleanRef)CFDictionaryGetValue(t_info, CFSTR("LSBackgroundOnly"));
		if (t_value != NULL && CFBooleanGetValue(t_value) == TRUE)
			t_result = false;
		CFRelease(t_info);
	}
	
	return t_result;
#endif /* MCS_processtypeisforeground_dsk_mac */
        ProcessSerialNumber t_psn = { 0, kCurrentProcess };
        
        CFDictionaryRef t_info;
        t_info = ProcessInformationCopyDictionary(&t_psn, kProcessDictionaryIncludeAllInformationMask);
        
        bool t_result;
        t_result = true;
        if (t_info != NULL)
        {
            CFBooleanRef t_value;
            t_value = (CFBooleanRef)CFDictionaryGetValue(t_info, CFSTR("LSBackgroundOnly"));
            if (t_value != NULL && CFBooleanGetValue(t_value) == TRUE)
                t_result = false;
            CFRelease(t_info);
        }
        
        return t_result;
    }
    
    virtual bool ChangeProcessType(bool p_to_foreground)
    {
#ifdef /* MCS_changeprocesstype_dsk_mac*/ LEGACY_SYSTEM
	// We can only switch from background to foreground. So check to see if
	// we are foreground already, we are only asking to go to foreground.
	if (MCS_processtypeisforeground())
	{
		if (to_foreground)
			return true;
		return false;
	}
	
	// Actually switch to foreground.
	ProcessSerialNumber t_psn = { 0, kCurrentProcess };
	TransformProcessType(&t_psn, kProcessTransformToForegroundApplication);
	SetFrontProcess(&t_psn);
	
	return true;
#endif /* MCS_changeprocesstype_dsk_mac */
        // We can only switch from background to foreground. So check to see if
        // we are foreground already, we are only asking to go to foreground.
        if (ProcessTypeIsForeground())
        {
            if (p_to_foreground)
                return true;
            return false;
        }
        
        // Actually switch to foreground.
        ProcessSerialNumber t_psn = { 0, kCurrentProcess };
        TransformProcessType(&t_psn, kProcessTransformToForegroundApplication);
        SetFrontProcess(&t_psn);
        
        return true;
    }
    
    virtual void CloseProcess(uint2 p_index)
    {
#ifdef /* MCS_closeprocess_dsk_mac */ LEGACY_SYSTEM
	if (MCprocesses[index].ihandle != NULL)
	{
		MCS_close(MCprocesses[index].ihandle);
		MCprocesses[index].ihandle = NULL;
	}
	if (MCprocesses[index].ohandle != NULL)
	{
		MCS_close(MCprocesses[index].ohandle);
		MCprocesses[index].ohandle = NULL;
	}
	MCprocesses[index].mode = OM_NEITHER;
#endif /* MCS_closeprocess_dsk_mac */
        if (MCprocesses[p_index].ihandle != NULL)
        {
            MCprocesses[p_index].ihandle -> Close();
            MCprocesses[p_index].ihandle = NULL;
        }
        if (MCprocesses[p_index].ohandle != NULL)
        {
            MCprocesses[p_index].ohandle -> Close();
            MCprocesses[p_index].ohandle = NULL;
        }
        MCprocesses[p_index].mode = OM_NEITHER;
    }
    
    virtual void Kill(int4 p_pid, int4 p_sig)
    {
#ifdef /* MCS_kill_dsk_mac */ LEGACY_SYSTEM
	if (pid == 0)
		return;

	uint2 i;
  for (i = 0 ; i < MCnprocesses ; i++)
    if (pid == MCprocesses[i].pid && (MCprocesses[i].sn.highLongOfPSN != 0 || MCprocesses[i].sn.lowLongOfPSN != 0))
		{
      AppleEvent ae, answer;
      AEDesc pdesc;
      AECreateDesc(typeProcessSerialNumber, &MCprocesses[i].sn,
		   sizeof(ProcessSerialNumber), &pdesc);
      AECreateAppleEvent('aevt', 'quit', &pdesc, kAutoGenerateReturnID,
			 kAnyTransactionID, &ae);
      AESend(&ae, &answer, kAEQueueReply, kAENormalPriority,
	     kAEDefaultTimeout, NULL, NULL);
      AEDisposeDesc(&ae);
      AEDisposeDesc(&answer);
      return;
    }

	kill(pid, sig);
#endif /* MCS_kill_dsk_mac */
        if (p_pid == 0)
            return;
        
        uint2 i;
        for (i = 0 ; i < MCnprocesses ; i++)
            if (p_pid == MCprocesses[i].pid && (MCprocesses[i].sn.highLongOfPSN != 0 || MCprocesses[i].sn.lowLongOfPSN != 0))
            {
                AppleEvent ae, answer;
                AEDesc pdesc;
                AECreateDesc(typeProcessSerialNumber, &MCprocesses[i].sn,
                             sizeof(ProcessSerialNumber), &pdesc);
                AECreateAppleEvent('aevt', 'quit', &pdesc, kAutoGenerateReturnID,
                                   kAnyTransactionID, &ae);
                AESend(&ae, &answer, kAEQueueReply, kAENormalPriority,
                       kAEDefaultTimeout, NULL, NULL);
                AEDisposeDesc(&ae);
                AEDisposeDesc(&answer);
                return;
            }
        
        kill(p_pid, p_sig);
    }
    
    virtual void KillAll(void)
    {
#ifdef /* MCS_killall_dsk_mac */ LEGACY_SYSTEM
	struct sigaction action;
	memset((char *)&action, 0, sizeof(action));
	action.sa_handler = (void (*)(int))SIG_IGN;
	sigaction(SIGCHLD, &action, NULL);
	while (MCnprocesses--)
	{
		MCNameDelete(MCprocesses[MCnprocesses] . name);
		MCprocesses[MCnprocesses] . name = nil;
		if (MCprocesses[MCnprocesses].pid != 0
		        && (MCprocesses[MCnprocesses].ihandle != NULL
		            || MCprocesses[MCnprocesses].ohandle != NULL))
		{
			kill(MCprocesses[MCnprocesses].pid, SIGKILL);
			waitpid(MCprocesses[MCnprocesses].pid, NULL, 0);
		}
	}
#endif /* MCS_killall_dsk_mac */
        struct sigaction action;
        memset((char *)&action, 0, sizeof(action));
        action.sa_handler = (void (*)(int))SIG_IGN;
        sigaction(SIGCHLD, &action, NULL);
        while (MCnprocesses--)
        {
            MCNameDelete(MCprocesses[MCnprocesses] . name);
            MCprocesses[MCnprocesses] . name = nil;
            if (MCprocesses[MCnprocesses].pid != 0
		        && (MCprocesses[MCnprocesses].ihandle != NULL
		            || MCprocesses[MCnprocesses].ohandle != NULL))
            {
                kill(MCprocesses[MCnprocesses].pid, SIGKILL);
                waitpid(MCprocesses[MCnprocesses].pid, NULL, 0);
            }
        }
    }
    
    virtual Boolean Poll(real8 p_delay, int p_fd)
    {
#ifdef /* MCS_poll_dsk_mac */ LEGACY_SYSTEM
	Boolean handled = False;
	fd_set rmaskfd, wmaskfd, emaskfd;
	FD_ZERO(&rmaskfd);
	FD_ZERO(&wmaskfd);
	FD_ZERO(&emaskfd);
	int4 maxfd = 0;
	if (!MCnoui)
	{
		if (fd != 0)
			FD_SET(fd, &rmaskfd);
		maxfd = fd;
	}
	if (MCshellfd != -1)
	{
		FD_SET(MCshellfd, &rmaskfd);
		if (MCshellfd > maxfd)
			maxfd = MCshellfd;
	}

	uint2 i;
	for (i = 0 ; i < MCnsockets ; i++)
	{
		int fd = MCsockets[i]->fd;
		if (!fd || MCsockets[i]->resolve_state == kMCSocketStateResolving ||
				MCsockets[i]->resolve_state == kMCSocketStateError)
			continue;
		if (MCsockets[i]->connected && !MCsockets[i]->closing
		        && !MCsockets[i]->shared || MCsockets[i]->accepting)
			FD_SET(fd, &rmaskfd);
		if (!MCsockets[i]->connected || MCsockets[i]->wevents != NULL)
			FD_SET(fd, &wmaskfd);
		FD_SET(fd, &emaskfd);
		if (fd > maxfd)
			maxfd = fd;
		if (MCsockets[i]->added)
		{
			delay = 0.0;
			MCsockets[i]->added = False;
			handled = True;
		}
	}

	struct timeval timeoutval;
	timeoutval.tv_sec = (long)delay;
	timeoutval.tv_usec = (long)((delay - floor(delay)) * 1000000.0);
	int n = 0;
	
	n = select(maxfd + 1, &rmaskfd, &wmaskfd, &emaskfd, &timeoutval);

	if (n <= 0)
		return handled;

	if (MCshellfd != -1 && FD_ISSET(MCshellfd, &rmaskfd))
		return True;

	for (i = 0 ; i < MCnsockets ; i++)
	{
		int fd = MCsockets[i]->fd;
		if (FD_ISSET(fd, &emaskfd) && fd != 0)
		{

			if (!MCsockets[i]->waiting)
			{
				MCsockets[i]->error = strclone("select error");
				MCsockets[i]->doclose();
			}

		}
		else
		{
			if (FD_ISSET(fd, &rmaskfd) && !MCsockets[i]->shared)
			{
				MCsockets[i]->readsome();
			}
			if (FD_ISSET(fd, &wmaskfd))
			{
				MCsockets[i]->writesome();
			}
		}
		MCsockets[i]->setselect();
	}

	return True;
#endif /* MCS_poll_dsk_mac */
        Boolean handled = False;
        fd_set rmaskfd, wmaskfd, emaskfd;
        FD_ZERO(&rmaskfd);
        FD_ZERO(&wmaskfd);
        FD_ZERO(&emaskfd);
        int4 maxfd = 0;
        if (!MCnoui)
        {
            if (p_fd != 0)
                FD_SET(p_fd, &rmaskfd);
            maxfd = p_fd;
        }
        if (MCshellfd != -1)
        {
            FD_SET(MCshellfd, &rmaskfd);
            if (MCshellfd > maxfd)
                maxfd = MCshellfd;
        }
        
        uint2 i;
        for (i = 0 ; i < MCnsockets ; i++)
        {
            int fd = MCsockets[i]->fd;
            if (!fd || MCsockets[i]->resolve_state == kMCSocketStateResolving ||
				MCsockets[i]->resolve_state == kMCSocketStateError)
                continue;
            if (MCsockets[i]->connected && !MCsockets[i]->closing
		        && !MCsockets[i]->shared || MCsockets[i]->accepting)
                FD_SET(fd, &rmaskfd);
            if (!MCsockets[i]->connected || MCsockets[i]->wevents != NULL)
                FD_SET(fd, &wmaskfd);
            FD_SET(fd, &emaskfd);
            if (fd > maxfd)
                maxfd = fd;
            if (MCsockets[i]->added)
            {
                p_delay = 0.0;
                MCsockets[i]->added = False;
                handled = True;
            }
        }
        
        struct timeval timeoutval;
        timeoutval.tv_sec = (long)p_delay;
        timeoutval.tv_usec = (long)((p_delay - floor(p_delay)) * 1000000.0);
        int n = 0;
        
        n = select(maxfd + 1, &rmaskfd, &wmaskfd, &emaskfd, &timeoutval);
        
        if (n <= 0)
            return handled;
        
        if (MCshellfd != -1 && FD_ISSET(MCshellfd, &rmaskfd))
            return True;
        
        for (i = 0 ; i < MCnsockets ; i++)
        {
            int fd = MCsockets[i]->fd;
            if (FD_ISSET(fd, &emaskfd) && fd != 0)
            {
                
                if (!MCsockets[i]->waiting)
                {
                    MCsockets[i]->error = strclone("select error");
                    MCsockets[i]->doclose();
                }
                
            }
            else
            {
                if (FD_ISSET(fd, &rmaskfd) && !MCsockets[i]->shared)
                {
                    MCsockets[i]->readsome();
                }
                if (FD_ISSET(fd, &wmaskfd))
                {
                    MCsockets[i]->writesome();
                }
            }
            MCsockets[i]->setselect();
        }
        
        return True;
    }
    
    virtual Boolean IsInteractiveConsole(int p_fd)
    {
#ifdef /* MCS_isatty_dsk_mac */ LEGACY_SYSTEM
	return isatty(fd) != 0;
#endif /* MCS_isatty_dsk_mac */
        return isatty(p_fd) != 0;
    }
    
    virtual int GetErrno()
    {
#ifdef /* MCS_geterrno_dsk_mac */ LEGACY_SYSTEM
	return errno;
#endif /* MCS_geterrno_dsk_mac */
        return errno;
    }
    virtual void SetErrno(int p_errno)
    {
#ifdef /* MCS_seterrno_dsk_mac */ LEGACY_SYSTEM
	errno = value;
#endif /* MCS_seterrno_dsk_mac */
        errno = p_errno;
    }
    
    virtual void LaunchDocument(MCStringRef p_document)
    {
#ifdef /* MCS_launch_document_dsk_mac */ LEGACY_SYSTEM
	int t_error = 0;
	
	FSRef t_document_ref;
	if (t_error == 0)
	{
		errno = MCS_pathtoref(p_document, t_document_ref);
		if (errno != noErr)
		{
			// MW-2008-06-12: [[ Bug 6336 ]] No result set if file not found on OS X
			MCresult -> sets("can't open file");
			t_error = 1;
		}
	}

	if (t_error == 0)
	{
		errno = LSOpenFSRef(&t_document_ref, NULL);
		MCS_launch_set_result_from_lsstatus();
	}
#endif /* MCS_launch_document_dsk_mac */
        int t_error = 0;
        
        FSRef t_document_ref;
        if (t_error == 0)
        {
            errno = MCS_mac_pathtoref(p_document, t_document_ref);
            if (errno != noErr)
            {
                // MW-2008-06-12: [[ Bug 6336 ]] No result set if file not found on OS X
                MCresult -> sets("can't open file");
                t_error = 1;
            }
        }
        
        if (t_error == 0)
        {
            errno = LSOpenFSRef(&t_document_ref, NULL);
            MCS_launch_set_result_from_lsstatus();
        }
    }
    
    virtual void LaunchUrl(MCStringRef p_document)
    {
#ifdef /* MCS_launch_url_dsk_mac */ LEGACY_SYSTEM
	bool t_success;
	t_success = true;

	CFStringRef t_cf_document;
	t_cf_document = NULL;
	if (t_success)
	{
		t_cf_document = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, p_document, kCFStringEncodingMacRoman, kCFAllocatorNull);
		if (t_cf_document == NULL)
			t_success = false;
	}
	
	CFURLRef t_cf_url;
	t_cf_url = NULL;
	if (t_success)
	{
		t_cf_url = CFURLCreateWithString(kCFAllocatorDefault, t_cf_document, NULL);
		if (t_cf_url == NULL)
			t_success = false;
	}

	if (t_success)
	{
		errno = LSOpenCFURLRef(t_cf_url, NULL);
		MCS_launch_set_result_from_lsstatus();
	}
	
	if (t_cf_url != NULL)
		CFRelease(t_cf_url);
		
	if (t_cf_document != NULL)
		CFRelease(t_cf_document);
#endif /* MCS_launch_url_dsk_mac */
        bool t_success;
        t_success = true;
        
        CFStringRef t_cf_document;
        t_cf_document = NULL;
        if (t_success)
        {
            t_cf_document = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, MCStringGetCString(p_document), kCFStringEncodingMacRoman, kCFAllocatorNull);
            if (t_cf_document == NULL)
                t_success = false;
        }
        
        CFURLRef t_cf_url;
        t_cf_url = NULL;
        if (t_success)
        {
            t_cf_url = CFURLCreateWithString(kCFAllocatorDefault, t_cf_document, NULL);
            if (t_cf_url == NULL)
                t_success = false;
        }
        
        if (t_success)
        {
            errno = LSOpenCFURLRef(t_cf_url, NULL);
            MCS_launch_set_result_from_lsstatus();
        }
        
        if (t_cf_url != NULL)
            CFRelease(t_cf_url);
		
        if (t_cf_document != NULL)
            CFRelease(t_cf_document);
    }
    
    virtual void DoAlternateLanguage(MCStringRef p_script, MCStringRef p_language)
    {
#ifdef /* MCS_doalternatelanguage_dsk_mac */ LEGACY_SYSTEM
	getosacomponents();
	OSAcomponent *posacomp = NULL;
	uint2 i;
	uint4 l = strlen(MCStringGetCString(p_language));
	for (i = 0; i < osancomponents; i++)
	{
		if (l == strlen(osacomponents[i].compname)
            && !MCU_strncasecmp(osacomponents[i].compname, MCStringGetCString(p_language), l))
		{
			posacomp = &osacomponents[i];
			break;
		}
	}
	if (posacomp == NULL)
	{
		MCresult->sets("alternate language not found");
		return;
	}
	if (posacomp->compinstance == NULL)
		posacomp->compinstance = OpenDefaultComponent(kOSAComponentType,
                                                      posacomp->compsubtype);
	//self check if returns error
	OSAID scriptid;
	if (osacompile(MCStringGetOldString(p_script), posacomp->compinstance, scriptid) != noErr)
	{
		MCresult->sets("compiler error");
		return;
	}
	MCString rvalue;
	OSErr err;
	err = osaexecute(rvalue, posacomp->compinstance, scriptid);
	if (err == noErr)
	{
		MCresult->copysvalue(rvalue);
		delete rvalue.getstring();
	}
	else if (err == errOSAScriptError)
	{
        /*		MCExecPoint ep(nil, nil, nil);
         
         AEDesc err_str;
         OSAScriptError(posacomp -> compinstance, kOSAErrorMessage, typeChar, &err_str);
         
         ep . setstring("execution error,%s",);
         AEDisposeDesc(&err_str);*/
		
		MCresult->sets("execution error");
	}
	else
		MCresult->sets("execution error");
	
	OSADispose(posacomp->compinstance, scriptid);
#endif /* MCS_doalternatelanguage_dsk_mac */
        getosacomponents();
        OSAcomponent *posacomp = NULL;
        uint2 i;
        uint4 l = strlen(MCStringGetCString(p_language));
        for (i = 0; i < osancomponents; i++)
        {
            if (l == strlen(osacomponents[i].compname)
                && !MCU_strncasecmp(osacomponents[i].compname, MCStringGetCString(p_language), l))
            {
                posacomp = &osacomponents[i];
                break;
            }
        }
        if (posacomp == NULL)
        {
            MCresult->sets("alternate language not found");
            return;
        }
        if (posacomp->compinstance == NULL)
            posacomp->compinstance = OpenDefaultComponent(kOSAComponentType,
                                                          posacomp->compsubtype);
        //self check if returns error
        OSAID scriptid;
        if (osacompile(p_script, posacomp->compinstance, scriptid) != noErr)
        {
            MCresult->sets("compiler error");
            return;
        }
        MCAutoStringRef rvalue;
        OSErr err;
        err = osaexecute(&rvalue, posacomp->compinstance, scriptid);
        if (err == noErr)
        {
            /* UNCHECKED */ MCresult->setvalueref(*rvalue);
        }
        else if (err == errOSAScriptError)
        {
            /*		MCExecPoint ep(nil, nil, nil);
             
             AEDesc err_str;
             OSAScriptError(posacomp -> compinstance, kOSAErrorMessage, typeChar, &err_str);
             
             ep . setstring("execution error,%s",);
             AEDisposeDesc(&err_str);*/
            
            MCresult->sets("execution error");
        }
        else
            MCresult->sets("execution error");
        
        OSADispose(posacomp->compinstance, scriptid);
    }
    
    virtual bool AlternateLanguages(MCListRef& r_list)
    {
#ifdef /* MCS_alternatelanguages_dsk_mac */ LEGACY_SYSTEM
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
	
	getosacomponents();
	for (uindex_t i = 0; i < osancomponents; i++)
		if (!MCListAppendCString(*t_list, osacomponents[i].compname))
			return false;
	
	return MCListCopy(*t_list, r_list);
#endif /* MCS_alternatelanguages_dsk_mac */
        MCAutoListRef t_list;
        if (!MCListCreateMutable('\n', &t_list))
            return false;
        
        getosacomponents();
        for (uindex_t i = 0; i < osancomponents; i++)
            if (!MCListAppendCString(*t_list, osacomponents[i].compname))
                return false;
        
        return MCListCopy(*t_list, r_list);
    }
    
#define DNS_SCRIPT "repeat for each line l in url \"binfile:/etc/resolv.conf\";\
if word 1 of l is \"nameserver\" then put word 2 of l & cr after it; end repeat;\
delete last char of it; return it"
    virtual bool GetDNSservers(MCListRef& r_list)
    {
#ifdef /* MCS_getDNSservers_dsk_mac */ LEGACY_SYSTEM
	MCAutoListRef t_list;

	MCresult->clear();
	MCdefaultstackptr->domess(DNS_SCRIPT);

	return MCListCreateMutable('\n', &t_list) &&
		MCListAppend(*t_list, MCresult->getvalueref()) &&
		MCListCopy(*t_list, r_list);
#endif /* MCS_getDNSservers_dsk_mac */
        MCAutoListRef t_list;
        
        MCresult->clear();
        MCdefaultstackptr->domess(MCSTR(DNS_SCRIPT));
        
        return MCListCreateMutable('\n', &t_list) &&
            MCListAppend(*t_list, MCresult->getvalueref()) &&
            MCListCopy(*t_list, r_list);
    }
};

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCDesktopCreateMacSystem(void)
{
	return new MCMacDesktop;
}


/*****************************************************************************
 *  Apple events handler	 			 	             *
 *****************************************************************************/


static bool fetch_ae_as_fsref_list(char*& string, uint4& length)
{
	AEDescList docList; //get a list of alias records for the documents
	long count;
	if (AEGetParamDesc(aePtr, keyDirectObject,
					   typeAEList, &docList) == noErr
		&& AECountItems(&docList, &count) == noErr && count > 0)
	{
		AEKeyword rKeyword; //returned keyword
		DescType rType;    //returned type
		
		FSRef t_doc_fsref;
		
		Size rSize;      //returned size, atual size of the docName
		long item;
		// get a FSSpec record, starts from count==1
		for (item = 1; item <= count; item++)
		{
			if (AEGetNthPtr(&docList, item, typeFSRef, &rKeyword, &rType,
							&t_doc_fsref, sizeof(FSRef), &rSize) != noErr)
			{
				AEDisposeDesc(&docList);
				return false;
			}
            
            MCAutoStringRef t_fullpathname;
			/* UNCHECKED */ MCS_mac_fsref_to_path(t_doc_fsref, &t_fullpathname);
			uint2 newlength = MCStringGetLength(*t_fullpathname) + 1;
			MCU_realloc(&string, length, length + newlength, 1);
			if (length)
				string[length - 1] = '\n';
			memcpy(&string[length], MCStringGetCString(*t_fullpathname), newlength);
			length += newlength;
		}
		string[length - 1] = '\0';
		AEDisposeDesc(&docList);
	}
	return true;
}

OSErr MCS_fsspec_to_fsref(const FSSpec *p_fsspec, FSRef *r_fsref)
{
	return FSpMakeFSRef(p_fsspec, r_fsref);
}

OSErr MCS_fsref_to_fsspec(const FSRef *p_fsref, FSSpec *r_fsspec)
{
	return FSGetCatalogInfo(p_fsref, 0, NULL, NULL, r_fsspec, NULL);
}

///////////////////////////////////////////////////////////////////////////////
//**************************************************************************
// * Utility functions used by this module only
// **************************************************************************/

static OSErr getDescFromAddress(MCStringRef address, AEDesc *retDesc)
{
	/* return an address descriptor based on the target address passed in
     * * There are 3 possible forms of target string: *
     * 1. ZONE:MACHINE:APP NAME
     * 2. MACHINE:APP NAME(sender & receiver are in the same zone but on different machine)
     * 3. APP NAME
     */
	errno = noErr;
	retDesc->dataHandle = NULL;  /* So caller can dispose always. */
    
    uindex_t t_index;
	/* UNCHECKED */ MCStringFirstIndexOfChar(address, ':', 0, kMCStringOptionCompareExact, t_index);
    
	if (t_index == 0)
	{ //address contains application name only. Form # 3
		char *appname = new char[MCStringGetLength(address) +1];
		strcpy(appname, MCStringGetCString(address));
		c2pstr(appname);  //convert c string to pascal string
		errno = getDesc(0, NULL, NULL, (unsigned char*)appname, retDesc);
		delete appname;
	}
    
	/* CARBON doesn't support the seding apple events between systmes. Therefore no
	 need to do the complicated location/zone searching                       */
    
	return errno;
}

// MW-2006-08-05: Vetted for Endian issues
static OSErr getDesc(short locKind, StringPtr zone, StringPtr machine,
                     StringPtr app, AEDesc *retDesc)
{
    
	/* Carbon doesn't support the seding apple events between different
	 * machines, plus CARBON does not support PPC Toolbox routines,
	 * Therefore no need to do the complicated location/zone
	 * searching. Use Process Manager's routine to get target program's
	 * process id for the address descriptor */
	ProcessSerialNumber psn;
	psn.highLongOfPSN = 0;
	psn.lowLongOfPSN = kNoProcess; //start at the beginning to do the search
	ProcessInfoRec pInfoRec;
	Str32 pname;
	/* need to specify values for the processInfoLength,processName, and
	 * processAppSpec fields of the process information record to get
	 * info returned in those fields. Since we only want the application
	 * name info returned, we allocate a string of 32 length as buffer
	 * to store the process name */
	pInfoRec.processInfoLength = sizeof(ProcessInfoRec);
	pInfoRec.processName = pname;
	pInfoRec.processAppSpec = NULL;
	Boolean processFound = False;
    
	while (GetNextProcess(&psn) == noErr)
	{
		if (GetProcessInformation(&psn, &pInfoRec) == noErr)
			if (EqualString(pInfoRec.processName, app, False, True))
			{
				processFound = True;
				break;
			}
	}
	if (processFound)
		return AECreateDesc(typeProcessSerialNumber, (Ptr)&pInfoRec.processNumber,
		                    sizeof(ProcessSerialNumber), retDesc);
	else
		return procNotFound; //can't find the program/process to create a descriptor
}

// MW-2006-08-05: Vetted for Endian issues
static OSErr getAEAttributes(const AppleEvent *ae, AEKeyword key, char *&result)
{
	DescType rType;
	Size rSize;
	DescType dt;
	Size s;
	if ((errno = AESizeOfAttribute(ae, key, &dt, &s)) == noErr)
	{
		switch (dt)
		{
            case typeBoolean:
			{
				Boolean b;
				AEGetAttributePtr(ae, key, dt, &rType, &b, s, &rSize);
				result = strclone(b ? MCtruestring : MCfalsestring);
			}
                break;
            case typeChar:
                result = new char[s + 1];
                AEGetAttributePtr(ae, key, dt, &rType, result, s, &rSize);
                result[s] = '\0';
                break;
            case typeType:
            {
                FourCharCode t_type;
                AEGetAttributePtr(ae, key, dt, &rType, &t_type, s, &rSize);
				result = FourCharCodeToString(t_type);
			}
                break;
            case typeShortInteger:
			{
				int2 i;
				AEGetAttributePtr(ae, key, dt, &rType, &i, s, &rSize);
				result = new char[I2L];
				sprintf(result, "%d", i);
				break;
			}
            case typeLongInteger:
			{
				int4 i;
				AEGetAttributePtr(ae, key, dt, &rType, &i, s, &rSize);
				result = new char[I4L];
				sprintf(result, "%d", i);
				break;
			}
            case typeShortFloat:
			{
				real4 f;
				AEGetAttributePtr(ae, key, dt, &rType, &f, s, &rSize);
				result = new char[R4L];
				sprintf(result, "%12.12g", f);
				break;
			}
            case typeLongFloat:
			{
				real8 f;
				AEGetAttributePtr(ae, key, dt, &rType, &f, s, &rSize);
				result = new char[R8L];
				sprintf(result, "%12.12g", f);
				break;
			}
            case typeMagnitude:
			{
				uint4 i;
				AEGetAttributePtr(ae, key, dt, &rType, &i, s, &rSize);
				result = new char[U4L];
				sprintf(result, "%u", i);
				break;
			}
            case typeNull:
                result = MCU_empty();
                break;
            case typeFSS:
			{
				FSSpec fs;
				errno = AEGetAttributePtr(ae, key, dt, &rType, &fs, s, &rSize);
                MCAutoStringRef t_result;
                
				/* UNCHECKED */ MCS_mac_FSSpec2path(&fs, &t_result);
                result = strclone(MCStringGetCString(*t_result));
			}
                break;
            case typeFSRef:
			{
				FSRef t_fs_ref;
				errno = AEGetAttributePtr(ae, key, dt, &rType, &t_fs_ref, s, &rSize);
                MCAutoStringRef t_result;
                /* UNCHECKED */ MCS_mac_fsref_to_path(t_fs_ref, &t_result);
                
                result = strclone(MCStringGetCString(*t_result));
			}
                break;
            default:
                result = new char[18];
                sprintf(result, "unknown type %4.4s", (char *)&dt);
                break;
		}
	}
	return errno;
}

// MW-2006-08-05: Vetted for Endian issues
static OSErr getAEParams(const AppleEvent *ae, AEKeyword key, char *&result)
{
	DescType rType;
	Size rSize;
	DescType dt;
	Size s;
	if ((errno = AESizeOfParam(ae, key, &dt, &s)) == noErr)
	{
		switch (dt)
		{
            case typeBoolean:
			{
				Boolean b;
				AEGetParamPtr(ae, key, dt, &rType, &b, s, &rSize);
				result = strclone(b ? MCtruestring : MCfalsestring);
			}
                break;
            case typeChar:
                result = new char[s + 1];
                AEGetParamPtr(ae, key, dt, &rType, result, s, &rSize);
                result[s] = '\0';
                break;
            case typeType:
            {
                FourCharCode t_type;
                AEGetParamPtr(ae, key, dt, &rType, &t_type, s, &rSize);
				result = FourCharCodeToString(t_type);
			}
                break;
            case typeShortInteger:
			{
				int2 i;
				AEGetParamPtr(ae, key, dt, &rType, &i, s, &rSize);
				result = new char[I2L];
				sprintf(result, "%d", i);
				break;
			}
            case typeLongInteger:
			{
				int4 i;
				AEGetParamPtr(ae, key, dt, &rType, &i, s, &rSize);
				result = new char[I4L];
				sprintf(result, "%d", i);
				break;
			}
            case typeShortFloat:
			{
				real4 f;
				AEGetParamPtr(ae, key, dt, &rType, &f, s, &rSize);
				result = new char[R4L];
				sprintf(result, "%12.12g", f);
				break;
			}
            case typeLongFloat:
			{
				real8 f;
				AEGetParamPtr(ae, key, dt, &rType, &f, s, &rSize);
				result = new char[R8L];
				sprintf(result, "%12.12g", f);
				break;
			}
            case typeMagnitude:
			{
				uint4 i;
				AEGetParamPtr(ae, key, dt, &rType, &i, s, &rSize);
				result = new char[U4L];
				sprintf(result, "%u", i);
				break;
			}
            case typeNull:
                result = MCU_empty();
                break;
            case typeFSS:
			{
				FSSpec fs;
				errno = AEGetParamPtr(ae, key, dt, &rType, &fs, s, &rSize);
                MCAutoStringRef t_result;
                
				/* UNCHECKED */ MCS_mac_FSSpec2path(&fs, &t_result);
                result = strclone(MCStringGetCString(*t_result));
			}
                break;
            case typeFSRef:
			{
				FSRef t_fs_ref;
				errno = AEGetParamPtr(ae, key, dt, &rType, &t_fs_ref, s, &rSize);
                MCAutoStringRef t_result;
                /* UNCHECKED */ MCS_mac_fsref_to_path(t_fs_ref, &t_result);
                
                result = strclone(MCStringGetCString(*t_result));
			}
                break;
            default:
                result = new char[18];
                sprintf(result, "unknown type %4.4s", (char *)&dt);
                break;
		}
	}
	return errno;
}

static OSErr getAddressFromDesc(AEAddressDesc targetDesc, char *address)
{/* This function returns the zone, machine, and application name for the
  indicated target descriptor.  */
    
    
	address[0] = '\0';
	return noErr;
    
}

static OSErr osacompile(MCStringRef s, ComponentInstance compinstance,
                        OSAID &scriptid)
{
	AEDesc aedscript;
	char *buffer = strclone(MCStringGetCString(s));
	char *cptr = buffer;
	while (*cptr)
	{
		if (*cptr == '\n')
			*cptr = '\r';
		cptr++;
	}
	scriptid = kOSANullScript;
	AECreateDesc(typeChar, buffer, MCStringGetLength(s), &aedscript);
	OSErr err = OSACompile(compinstance, &aedscript, kOSAModeNull, &scriptid);
	AEDisposeDesc(&aedscript);
	delete[] buffer;
	return err;
}

static OSErr osaexecute(MCStringRef& r_string, ComponentInstance compinstance,
                        OSAID scriptid)
{
	OSAID scriptresult;
	OSErr err;
	err = OSAExecute(compinstance, scriptid, kOSANullScript,
	                 kOSAModeNull, &scriptresult);
	if (err != noErr)
		return err;
	AEDesc aedresult;
	OSADisplay(compinstance, scriptresult, typeChar, kOSAModeNull, &aedresult);
	uint4 tsize = AEGetDescDataSize(&aedresult);
	char *buffer = new char[tsize];
	err = AEGetDescData(&aedresult,buffer,tsize);
    /* UNCHECKED */ MCStringCreateWithNativeCharsAndRelease((char_t*)buffer, tsize, r_string);
	AEDisposeDesc(&aedresult);
	OSADispose(compinstance, scriptresult);
	return err;
}

static void getosacomponents()
{
	if (osacomponents != NULL)
		return;
	ComponentDescription compdesc;
	Component tcomponent = 0L;
	Handle compname = NewHandle(0);
	compdesc.componentType = kOSAComponentType; //scripting component
	compdesc.componentSubType = 0L;//any
	compdesc.componentManufacturer = 0L;
	compdesc.componentFlags = kOSASupportsCompiling; // compile and execute
	compdesc.componentFlagsMask = kOSASupportsCompiling;
	uint2 compnumber = CountComponents(&compdesc);
	if (compnumber - 1) //don't include the generic script comp
		osacomponents = new OSAcomponent[compnumber - 1];
	while ((tcomponent = FindNextComponent(tcomponent,&compdesc)) != NULL)
	{
		ComponentDescription founddesc;
		OSErr err = GetComponentInfo(tcomponent, &founddesc, compname, NULL, NULL);
		if (founddesc.componentSubType == kOSAGenericScriptingComponentSubtype)
			continue;
		if (err != noErr)
			break;
		//self check if return error
		HLock(compname);
		p2cstr((unsigned char *)*compname);
		MCString s = *compname;
		MCU_lower(osacomponents[osancomponents].compname, s);
		osacomponents[osancomponents].compname[s.getlength()] = '\0';
		osacomponents[osancomponents].compsubtype = founddesc.componentSubType;
		osacomponents[osancomponents].compinstance = NULL;
		HUnlock(compname);
		osancomponents++;
	}
	DisposeHandle(compname);
}

struct  LangID2Charset
{
	Lang_charset charset;
	ScriptCode scriptcode;
};

static LangID2Charset scriptcodetocharsets[] = {
    { LCH_ENGLISH, smRoman },
    { LCH_ROMAN, smRoman },
    { LCH_JAPANESE, smJapanese },
    { LCH_CHINESE, smTradChinese },
    { LCH_RUSSIAN, smCyrillic },
    { LCH_TURKISH, smCyrillic },
    { LCH_BULGARIAN, smCyrillic },
    { LCH_UKRAINIAN, smCyrillic },
    { LCH_ARABIC, smArabic },
    { LCH_HEBREW, smHebrew },
    { LCH_GREEK, smGreek },
    { LCH_KOREAN, smKorean },
    { LCH_POLISH, smCentralEuroRoman },
    { LCH_VIETNAMESE, smVietnamese },
    { LCH_LITHUANIAN, smCentralEuroRoman },
    { LCH_THAI, smThai },
    { LCH_SIMPLE_CHINESE, smSimpChinese },
    { LCH_UNICODE, smUnicodeScript }
};

uint1 MCS_langidtocharset(uint2 scode)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(scriptcodetocharsets); i++)
		if (scriptcodetocharsets[i].scriptcode == scode)
			return scriptcodetocharsets[i].charset;
	return 0;
}

uint2 MCS_charsettolangid(uint1 charset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(scriptcodetocharsets); i++)
		if (scriptcodetocharsets[i].charset == charset)
			return scriptcodetocharsets[i].scriptcode;
	return 0;
}

void MCS_unicodetomultibyte(const char *s, uint4 len, char *d,
                            uint4 destbufferlength, uint4 &destlen,
                            uint1 charset)
{
	ScriptCode fscript = MCS_charsettolangid(charset);
	//we cache unicode convertors for speed
	if (!destbufferlength)
	{
		if (charset)
			destlen = len << 1;
		else
			destlen = len >> 1;
		return;
	}
	if (unicodeconvertors[fscript] == NULL)
	{
		TextEncoding scriptEncoding;
		UpgradeScriptInfoToTextEncoding(fscript, kTextLanguageDontCare,
		                                kTextRegionDontCare, NULL,
		                                &scriptEncoding);
		CreateUnicodeToTextInfoByEncoding(scriptEncoding,
		                                  &unicodeconvertors[fscript]);
	}
	ByteCount processedbytes, outlength;
	destlen = 0;

	// MW-2008-06-12: [[ Bug 6313 ]] Loop through all input characters, replacing unknown
	//   ones with ? - this mimics Windows behaviour.
	// MW-2008-06-12: Make sure we loop until we have no pairs of bytes left otherwise
	//   we go into an infinite loop when doing things like uniDecode("abc")
	while(len > 1)
	{
		ConvertFromUnicodeToText(unicodeconvertors[fscript], len, (UniChar *)s,
								 kUnicodeLooseMappingsMask
								 | kUnicodeStringUnterminatedBit
								 | kUnicodeUseFallbacksBit, 0, NULL, 0, NULL,
								 destbufferlength, &processedbytes,
								 &outlength, (LogicalAddress)d);
		if (processedbytes == 0)
		{
			*d = '?';
			processedbytes = 2;
			outlength = 1;
		}

		len -= processedbytes;
		destlen += outlength;
		s += processedbytes;
		d += outlength;
	}
}

void MCS_multibytetounicode(const char *s, uint4 len, char *d,
                            uint4 destbufferlength,
                            uint4 &destlen, uint1 charset)
{
	// MW-2012-06-14: [[ Bug ]] If used for charset 0 before any other, causes a crash.
	static int oldcharset = -1;
	if (!destbufferlength)
	{
		destlen = len << 1;
		return;
	}
	if (charset != oldcharset)
	{
		if (texttounicodeconvertor)
			DisposeTextToUnicodeInfo(texttounicodeconvertor);
		texttounicodeconvertor = NULL;
		ScriptCode fscript = MCS_charsettolangid(charset);
		TextEncoding scriptEncoding;
		UpgradeScriptInfoToTextEncoding(fscript, kTextLanguageDontCare,
		                                kTextRegionDontCare, NULL,
		                                &scriptEncoding);
		texttounicodeconvertor = &texttounicodeinfo;
		CreateTextToUnicodeInfoByEncoding(scriptEncoding, texttounicodeconvertor);
	}
	ByteCount processedbytes, outlength;
	ConvertFromTextToUnicode(*texttounicodeconvertor, len, (LogicalAddress) s,
	                         kUnicodeLooseMappingsMask
	                         | kUnicodeUseFallbacksMask, 0, NULL, 0, NULL,
	                         destbufferlength, &processedbytes,
	                         &outlength, (UniChar *)d);
	destlen = outlength;
	oldcharset = charset;
}

////////////////////////////////////////////////////////////////////////////////

static bool startprocess_create_argv(char *name, char *doc, uint32_t & r_argc, char**& r_argv)
{
	uint32_t argc;
	char **argv;
	argc = 0;
	argv = nil;
	if (doc == NULL)
	{
		char *sptr = name;
		while (*sptr)
		{
			while (isspace(*sptr))
				sptr++;
			MCU_realloc((char **)&argv, argc, argc + 2, sizeof(char *));
			if (*sptr == '"')
			{
				argv[argc++] = ++sptr;
				while (*sptr && *sptr != '"')
					sptr++;
			}
			else
			{
				argv[argc++] = sptr;
				while (*sptr && !isspace(*sptr))
					sptr++;
			}
			if (*sptr)
				*sptr++ = '\0';
		}
	}
	else
	{
		argv = new char *[3];
		argv[0] = name;
		argv[1] = doc;
		argc = 2;
	}
	
	argv[argc] = NULL;
	
	r_argc = argc;
	r_argv = argv;
	
	return true;
}

static bool startprocess_write_uint32_to_fd(int fd, uint32_t value)
{
	value = MCSwapInt32HostToNetwork(value);
	if (write(fd, &value, sizeof(uint32_t)) != sizeof(uint32_t))
		return false;
	return true;
}

static bool startprocess_write_cstring_to_fd(int fd, char *string)
{
	if (!startprocess_write_uint32_to_fd(fd, strlen(string) + 1))
		return false;
	if (write(fd, string, strlen(string) + 1) != strlen(string) + 1)
		return false;
	return true;
}

static bool startprocess_read_uint32_from_fd(int fd, uint32_t& r_value)
{
	uint32_t value;
	if (read(fd, &value, sizeof(uint32_t)) != sizeof(uint32_t))
		return false;
	r_value = MCSwapInt32NetworkToHost(value);
	return true;
}

static bool startprocess_read_cstring_from_fd(int fd, char*& r_cstring)
{
	uint32_t t_length;
	if (!startprocess_read_uint32_from_fd(fd, t_length))
		return false;
	
	char *t_string;
	t_string = (char *)malloc(t_length);
	if (read(fd, t_string, t_length) != t_length)
	{
		free(t_string);
		return false;
	}
	
	r_cstring = t_string;
	
	return true;
}

bool MCS_mac_elevation_bootstrap_main(int argc, char *argv[])
{
	// Read the argument count
	uint32_t t_arg_count;
	startprocess_read_uint32_from_fd(fileno(stdin), t_arg_count);
	
	// Read the arguments
	char **t_args;
	t_args = (char **)malloc(sizeof(char *) * (t_arg_count + 1));
	for(uint32_t i = 0; i < t_arg_count; i++)
		startprocess_read_cstring_from_fd(fileno(stdin), t_args[i]);
    
	t_args[t_arg_count] = nil;
	
	// Now send back our PID to the parent.
	startprocess_write_uint32_to_fd(fileno(stdout), getpid());
	
	// Make sure stderr is also sent to stdout
	close(2);
	dup(1);
	
	// And finally exec to the new process (this does not return if successful).
	execvp(t_args[0], t_args);
	
	// If we get this far then an error has occured :o(
	return false;
}

static void MCS_startprocess_launch(MCNameRef name, MCStringRef docname, Open_mode mode)
{
	LaunchParamBlockRec launchParms;
	launchParms.launchBlockID = extendedBlock;
	launchParms.launchEPBLength = extendedBlockLen;
	launchParms.launchFileFlags = 0;
	launchParms.launchControlFlags = launchContinue + launchNoFileFlags;
	launchParms.launchAppParameters = NULL;
	AEDesc pd, target;
	AEDescList files_list, file_desc;
	AliasHandle the_alias;
	AppleEvent ae;
	
	FSRef t_app_fsref;
	FSSpec t_app_fsspec;
	errno = MCS_mac_pathtoref(MCNameGetString(name), t_app_fsref);
	errno = MCS_mac_fsref_to_fsspec(&t_app_fsref, &t_app_fsspec);
	
	uint2 i;
    
	if (mode == OM_NEITHER)
	{
		if (errno != noErr)
		{
			MCresult->sets("no such program");
			return;
		}
        
		FSRef t_doc_fsref;
		
		LSLaunchFSRefSpec inLaunchSpec;
		FSRef launchedapp;
		inLaunchSpec.numDocs = 0;
		inLaunchSpec.itemRefs = NULL;
		if (MCStringGetLength(docname))
		{
			if (MCS_mac_pathtoref(docname, t_doc_fsref) != noErr)
			{
				MCresult->sets("no such document");
				return;
			}
			inLaunchSpec.numDocs = 1;
			inLaunchSpec.itemRefs = &t_doc_fsref;
		}
		inLaunchSpec.appRef = &t_app_fsref;
		inLaunchSpec.passThruParams = NULL;
		inLaunchSpec.launchFlags = kLSLaunchDefaults;
		inLaunchSpec.asyncRefCon = NULL;
		errno = LSOpenFromRefSpec(&inLaunchSpec, &launchedapp);
		return;
	}
    
	errno = connectionInvalid;
	if (MCStringGetLength(docname))
	{
		for (i = 0 ; i < MCnprocesses ; i++)
			if (MCNameIsEqualTo(name, MCprocesses[i].name, kMCCompareExact))
				break;
		if (i == MCnprocesses)
		{
			FInfo fndrInfo;
			if ((errno = FSpGetFInfo(&t_app_fsspec, &fndrInfo)) != noErr)
			{
				MCresult->sets("no such program");
				return;
			}
			OSType creator = fndrInfo.fdCreator;
			AECreateDesc(typeApplSignature, (Ptr)&creator, sizeof(OSType), &target);
		}
		else
			AECreateDesc(typeProcessSerialNumber, &MCprocesses[i].sn,
			             sizeof(ProcessSerialNumber), &target);
		AECreateAppleEvent('aevt', 'odoc', &target, kAutoGenerateReturnID,
		                   kAnyTransactionID, &ae);
		FSSpec fspec;
		FSRef t_tmp_fsref;
		if (MCS_mac_pathtoref(docname, t_tmp_fsref) == noErr && MCS_mac_fsref_to_fsspec(&t_tmp_fsref, &fspec) == noErr)
		{
			AECreateList(NULL, 0, false, &files_list);
			NewAlias(NULL, &fspec, &the_alias);
			HLock((Handle)the_alias);
			AECreateDesc(typeAlias, (Ptr)(*the_alias),
			             GetHandleSize((Handle)the_alias), &file_desc);
			HUnlock((Handle)the_alias);
			AEPutDesc(&files_list, 0, &file_desc);
			AEPutParamDesc(&ae, keyDirectObject, &files_list);
		}
		AppleEvent the_reply;
		AECreateDesc(typeNull, NULL, 0, &the_reply);
		errno = AESend(&ae, &the_reply, kAENoReply,
		               kAENormalPriority, kNoTimeOut, NULL, NULL);
		AEDisposeDesc(&the_reply);
		AECoerceDesc(&ae, typeAppParameters, &pd);
		launchParms.launchAppParameters = (AppParametersPtr)*(Handle)pd.dataHandle;
		HLock((Handle)pd.dataHandle);
        
	}
	
	if (errno != noErr)
	{
		launchParms.launchAppSpec = &t_app_fsspec;
		errno = LaunchApplication(&launchParms);
		if (errno != noErr)
		{
			char buffer[7 + I2L];
			sprintf(buffer, "error %d", errno);
			MCresult->copysvalue(buffer);
		}
		else
		{
			MCresult->clear(False);
			for (i = 0 ; i < MCnprocesses ; i++)
			{
				Boolean result;
				SameProcess((ProcessSerialNumber *)&MCprocesses[i].sn, &launchParms.launchProcessSN, &result);
				if (result)
					break;
			}
			if (i == MCnprocesses)
			{
				MCU_realloc((char **)&MCprocesses, MCnprocesses, MCnprocesses + 1,
				            sizeof(Streamnode));
				MCprocesses[MCnprocesses].name = name;
				MCprocesses[MCnprocesses].mode = OM_NEITHER;
				MCprocesses[MCnprocesses].ihandle = NULL;
				MCprocesses[MCnprocesses].ohandle = NULL;
				MCprocesses[MCnprocesses].pid = ++curpid;
				memcpy(&MCprocesses[MCnprocesses++].sn, &launchParms.launchProcessSN, sizeof(MCMacProcessSerialNumber));
			}
		}
	}
	if (launchParms.launchAppParameters != NULL)
	{
		HUnlock((Handle)pd.dataHandle);
		AEDisposeDesc(&target);
		AEDisposeDesc(&pd);
		DisposeHandle((Handle)the_alias);
		AEDisposeDesc(&file_desc);
		AEDisposeDesc(&files_list);
		AEDisposeDesc(&file_desc);
		AEDisposeDesc(&ae);
	}
}


static void MCS_startprocess_unix(MCNameRef name, MCStringRef doc, Open_mode mode, Boolean elevated)
{
	Boolean noerror = True;
	Boolean reading = mode == OM_READ || mode == OM_UPDATE;
	Boolean writing = mode == OM_APPEND || mode == OM_WRITE || mode == OM_UPDATE;
	MCU_realloc((char **)&MCprocesses, MCnprocesses, MCnprocesses + 1, sizeof(Streamnode));
    
	// Store process information.
	uint2 index = MCnprocesses;
	MCprocesses[MCnprocesses].name = (MCNameRef)MCValueRetain(name);
	MCprocesses[MCnprocesses].mode = mode;
	MCprocesses[MCnprocesses].ihandle = NULL;
	MCprocesses[MCnprocesses].ohandle = NULL;
	MCprocesses[MCnprocesses].sn.highLongOfPSN = 0;
	MCprocesses[MCnprocesses].sn.lowLongOfPSN = 0;
	
	if (!elevated)
	{
		int tochild[2]; // pipe to child
		int toparent[2]; // pipe to parent
		
		// If we are reading, create the pipe to parent.
		// Parent reads, child writes.
		if (reading)
			if (pipe(toparent) != 0)
				noerror = False;
		
		// If we are writing, create the pipe to child.
		// Parent writes, child reads.
		if (noerror && writing)
			if (pipe(tochild) != 0)
			{
				noerror = False;
				if (reading)
				{
					// error, get rid of these fds
					close(toparent[0]);
					close(toparent[1]);
				}
			}
        
		if (noerror)
		{
			// Fork
			if ((MCprocesses[MCnprocesses++].pid = fork()) == 0)
			{
				char *t_name_dup;
				t_name_dup = strdup(MCNameGetCString(name));
				
				// The pid is 0, so here we are in the child process.
				// Construct the argument string to pass to the process..
				char **argv = NULL;
				uint32_t argc = 0;
				startprocess_create_argv(t_name_dup, const_cast<char*>(MCStringGetCString(doc)), argc, argv);
				
				// The parent is reading, so we (we are child) are writing.
				if (reading)
				{
					// Don't need to read
					close(toparent[0]);
					
					// Close the current stdout, and duplicate the out descriptor of toparent to stdout.
					close(1);
					dup(toparent[1]);
					
					// Redirect stderr of this child to toparent-out.
					close(2);
					dup(toparent[1]);
					
					// We no longer need this pipe, so close the output descriptor.
					close(toparent[1]);
				}
				else
				{
					// Not reading, so close stdout and stderr.
					close(1);
					close(2);
				}
				if (writing)
				{
					// Parent is writing, so child needs to read. Close tochild[1], we dont need it.
					close(tochild[1]);
					
					// Attach stdin to tochild[0].
					close(0);
					dup(tochild[0]);
					
					// Close, as we no longer need it.
					close(tochild[0]);
				}
				else // not writing, so close stdin
					close(0);
                
				// Execute a new process in a new process image.
				execvp(argv[0], argv);
				
				// If we get here, an error occurred
				_exit(-1);
			}
			
			// If we get here, we are in the parent process, as the child has exited.
			
			MCS_checkprocesses();
			
			if (reading)
			{
				close(toparent[1]);
				MCS_mac_nodelay(toparent[0]);
				// Store the in handle for the "process".
				MCprocesses[index].ihandle = MCsystem -> OpenFd(toparent[0], kMCSystemFileModeRead);
			}
			if (writing)
			{
				close(tochild[0]);
				// Store the out handle for the "process".
				MCprocesses[index].ohandle = MCsystem -> OpenFd(tochild[1], kMCSystemFileModeWrite);
			}
		}
	}
	else
	{
		OSStatus t_status;
		t_status = noErr;
		
		AuthorizationRef t_auth;
		t_auth = nil;
		if (t_status == noErr)
			t_status = AuthorizationCreate(nil, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &t_auth);
		
		if (t_status == noErr)
		{
			AuthorizationItem t_items =
			{
				kAuthorizationRightExecute, 0,
				NULL, 0
			};
			AuthorizationRights t_rights =
			{
				1, &t_items
			};
			AuthorizationFlags t_flags =
            kAuthorizationFlagDefaults |
            kAuthorizationFlagInteractionAllowed |
            kAuthorizationFlagPreAuthorize |
            kAuthorizationFlagExtendRights;
			t_status = AuthorizationCopyRights(t_auth, &t_rights, nil, t_flags, nil);
		}
		
		FILE *t_stream;
		t_stream = nil;
		if (t_status == noErr)
		{
			char *t_arguments[] =
			{
				"-elevated-slave",
				nil
			};
			t_status = AuthorizationExecuteWithPrivileges(t_auth, MCStringGetCString(MCcmd), kAuthorizationFlagDefaults, t_arguments, &t_stream);
		}
		
		uint32_t t_pid;
		t_pid = 0;
		if (t_status == noErr)
		{
			char *t_name_dup;
			t_name_dup = strdup(MCNameGetCString(name));
			
			// Split the arguments
			uint32_t t_argc;
			char **t_argv;
			startprocess_create_argv(t_name_dup, const_cast<char *>(MCStringGetCString(doc)), t_argc, t_argv);
			startprocess_write_uint32_to_fd(fileno(t_stream), t_argc);
			for(uint32_t i = 0; i < t_argc; i++)
				startprocess_write_cstring_to_fd(fileno(t_stream), t_argv[i]);
			if (!startprocess_read_uint32_from_fd(fileno(t_stream), t_pid))
				t_status = errAuthorizationToolExecuteFailure;
			
			delete t_name_dup;
			delete[] t_argv;
		}
		
		if (t_status == noErr)
		{
			MCprocesses[MCnprocesses++].pid = t_pid;
			MCS_checkprocesses();
			
			if (reading)
			{
				int t_fd;
				t_fd = dup(fileno(t_stream));
				MCS_mac_nodelay(t_fd);
				MCprocesses[index].ihandle = MCsystem -> OpenFd(t_fd, kMCSystemFileModeRead);
			}
			if (writing)
				MCprocesses[index].ohandle = MCsystem -> OpenFd(dup(fileno(t_stream)), kMCSystemFileModeWrite);
			
			noerror = True;
		}
		else
			noerror = False;
		
		if (t_stream != nil)
			fclose(t_stream);
		
		if (t_auth != nil)
			AuthorizationFree(t_auth, kAuthorizationFlagDefaults);
	}
	
	if (!noerror || MCprocesses[index].pid == -1 || MCprocesses[index].pid == 0)
	{
		if (noerror)
			MCprocesses[index].pid = 0;
		MCresult->sets("not opened");
	}
	else
		MCresult->clear(False);
}

bool MCS_generate_uuid(char p_buffer[128])
{
	CFUUIDRef t_uuid;
	t_uuid = CFUUIDCreate(kCFAllocatorDefault);
	if (t_uuid != NULL)
	{
		CFStringRef t_uuid_string;
		
		t_uuid_string = CFUUIDCreateString(kCFAllocatorDefault, t_uuid);
		if (t_uuid_string != NULL)
		{
			CFStringGetCString(t_uuid_string, p_buffer, 127, kCFStringEncodingMacRoman);
			CFRelease(t_uuid_string);
		}
		
		CFRelease(t_uuid);
        
		return true;
	}
    
	return false;
}
