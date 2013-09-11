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

//
// MAC plaform specific routines  MACSPEC.CPP
//

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "typedefs.h"
#include "mcio.h"

#include "mcerror.h"
#include "execpt.h"
#include "handler.h"
#include "util.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "control.h"
#include "param.h"
#include "securemode.h"
#include "license.h"
#include "mode.h"
#include "core.h"
#include "socket.h"
#include "osspec.h"
#include "osxdc.h"
#include "mcssl.h"

#include "resolution.h"

#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>

extern "C"
{
	extern UInt32 SwapQDTextFlags(UInt32 newFlags);
	typedef UInt32 (*SwapQDTextFlagsPtr)(UInt32 newFlags);
}

#include "ports.cpp"

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

static Boolean hasPPCToolbox = False;
static Boolean hasAppleEvents = False;

#define MINIMUM_FAKE_PID (1 << 29)

static int4 curpid = MINIMUM_FAKE_PID;
static char *replymessage;       //used in DoSpecial() & other routines
static uint4 replylength;
static AEKeyword replykeyword;   // Use in DoSpecial & other routines
static char *AEanswerData;// used by DoAEAnswer() & MCS_send()
static char *AEanswerErr; //the reply error from an AE send by MC.
static const AppleEvent *aePtr; //current apple event for mcs_request_ae()


static UnicodeToTextInfo unicodeconvertors[32];
static TextToUnicodeInfo texttounicodeinfo;
static TextToUnicodeInfo *texttounicodeconvertor = NULL;
static UnicodeToTextInfo utf8totextinfo;
static TextToUnicodeInfo texttoutf8info;

/***************************************************************************
 * utility functions used by this module only		                   *
 ***************************************************************************/

static OSErr getDescFromAddress(const char *address, AEDesc *retDesc);
static OSErr getDesc(short locKind, StringPtr zone, StringPtr machine, StringPtr app, AEDesc *retDesc);
static OSErr getAEAttributes(const AppleEvent *ae, AEKeyword key, char *&result);
static OSErr getAEParams(const AppleEvent *ae, AEKeyword key, char *&result);
static OSErr getAddressFromDesc(AEAddressDesc targetDesc, char *address);

static void getosacomponents();
static OSErr osacompile(MCString &s, ComponentInstance compinstance, OSAID &id);
static OSErr osaexecute(MCString &s,ComponentInstance compinstance, OSAID id);
/***************************************************************************/

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
			// IM-2013-08-01: [[ ResIndependence ]] scale to device pixels
			t_rect . top = -sptr -> getscroll() * MCResGetDeviceScale();
			
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
	// MW-2013-08-07: [[ Bug 10865 ]] If AppleScript is disabled (secureMode) then
	//   don't handle the event.
	if (!MCSecureModeCanAccessAppleScript())
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
				if (aeid == kAEDoScript)
				{
					MCdefaultstackptr->getcard()->domess(sptr);
					MCresult->fetch(ep);
					AEPutParamPtr(reply, '----', typeChar, ep.getsvalue().getstring(), ep.getsvalue().getlength());
				}
				else
				{
					MCdefaultstackptr->getcard()->eval(sptr, ep);
					AEPutParamPtr(reply, '----', typeChar, ep.getsvalue().getstring(), ep.getsvalue().getlength());
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
{
	//Apple Event for opening documnets, in our use is to open stacks when user
	//double clicked on a MC stack icon
	
	// MW-2013-08-07: [[ Bug 10865 ]] If AppleScript is disabled (secureMode) then
	//   don't handle the event.
	if (!MCSecureModeCanAccessAppleScript())
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
		char *fullPathName = MCS_fsref_to_path(t_doc_fsref);

		if (MCModeShouldQueueOpeningStacks())
		{
			MCU_realloc((char **)&MCstacknames, MCnstacks, MCnstacks + 1, sizeof(char *));
			MCstacknames[MCnstacks++] = strclone(fullPathName);
		}
		else
		{
			MCStack *stkptr;  //stack pointer
			if (MCdispatcher->loadfile(fullPathName, stkptr) == IO_NORMAL)
				stkptr->open();
		}
		delete fullPathName;
	}
	AEDisposeDesc(&docList);
	return noErr;
}

static pascal OSErr DoPrintDoc(const AppleEvent *theAppleEvent, AppleEvent *reply, long refCon)
{
	// MW-2013-08-07: [[ Bug 10865 ]] If AppleScript is disabled (secureMode) then
	//   don't handle the event.
	if (!MCSecureModeCanAccessAppleScript())
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
	// MW-2013-08-07: [[ Bug 10865 ]] Even if AppleScript is disabled we still need
	//   to handle the 'quit' message.
	// if (!MCSecureModeCanAccessAppleScript())
	//	return errAEEventNotHandled;

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
	// MW-2013-08-07: [[ Bug 10865 ]] Even if AppleScript is disabled we still need
	//   to handle the 'preferences' message.
	//if (!MCSecureModeCanAccessAppleScript())
	//	return errAEEventNotHandled;

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
	// MW-2013-08-07: [[ Bug 10865 ]] If AppleScript is disabled (secureMode) then
	//   don't handle the event.
	if (!MCSecureModeCanAccessAppleScript())
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
		fprintf(stderr, "%s exiting on signal %d\n", MCcmd, sig);
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

void MCS_utf8tonative(const char *s, uint4 len, char *d, uint4 &destlen)
{
	init_utf8_converters();
	
	ByteCount processedbytes, outlength;
	uint4 destbufferlength;
	destbufferlength = destlen;
	ConvertFromUnicodeToText(utf8totextinfo, len, (UniChar *)s,kUnicodeUseFallbacksMask | kUnicodeLooseMappingsMask, 0, NULL, 0, NULL, destbufferlength, &processedbytes, &outlength, (LogicalAddress)d);
	destlen = outlength;
}

void MCS_nativetoutf8(const char *s, uint4 len, char *d, uint4 &destlen)
{
	init_utf8_converters();
	
	ByteCount processedbytes, outlength;
	uint4 destbufferlength;
	destbufferlength = destlen;
	ConvertFromTextToUnicode(texttoutf8info, len, (LogicalAddress)s,
	                         kUnicodeLooseMappingsMask, 0, NULL, 0, NULL,
	                         destbufferlength, &processedbytes,
	                         &outlength, (UniChar *)d);
	destlen = outlength;
}


// MW-2005-02-22: Make available to opensslsocket.cpp
char *path2utf(char *path)
{
	char *tutfpath = new char[(PATH_MAX + 2) * 2];
	uint4 destlen;
	destlen = PATH_MAX + 2;
	MCS_nativetoutf8(path, strlen(path), tutfpath, destlen);
	tutfpath[destlen] = 0;
	delete path;
	return tutfpath;
}

void MCS_init()
{
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
	if (!MCS_isatty(0))
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
	
	char *dptr = MCS_getcurdir();
	if (strlen(dptr) <= 1)
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
		MCS_setcurdir(newpath);
		delete tpath;
		delete newpath;
	}
	delete dptr;

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
}

void MCS_shutdown()
{
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
}

void MCS_seterrno(int value)
{
	errno = value;
}

int MCS_geterrno()
{
	return errno;
}

void MCS_alarm(real8 seconds)
{ //is used for checking event loop periodically
	// InsTime() or
	//PrimeTime(pass handle to a function, in the function set MCalarm to True)
}

// MW-2005-08-15: We have two types of process starting in MacOS X it seems:
//   MCS_startprocess is called by MCLaunch with a docname
//   MCS_startprocess is called by MCOpen without a docname
// Thus, we will fork two methods - and dispatch from MCS_startprocess
void MCS_startprocess_unix(char *name, char *doc, Open_mode mode, Boolean elevated);
void MCS_startprocess_launch(char *name, char *docname, Open_mode mode);

void MCS_startprocess(char *name, char *docname, Open_mode mode, Boolean elevated)
{
	uint4 t_length = strlen(name);
	if (t_length > 4 && strcmp(name + t_length - 4, ".app") == 0 || docname != NULL)
	  MCS_startprocess_launch(name, docname, mode);
	else
	  MCS_startprocess_unix(name, NULL, mode, elevated);
}

void MCS_startprocess_launch(char *name, char *docname, Open_mode mode)
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
	errno = MCS_pathtoref(name, &t_app_fsref);
	errno = MCS_fsref_to_fsspec(&t_app_fsref, &t_app_fsspec);
	
	uint2 i;

	if (docname != NULL && *docname == '\0')
		docname = NULL;

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
		if (docname)
		{
			if (MCS_pathtoref(docname, &t_doc_fsref) != noErr)
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
	if (docname != NULL && *docname != '\0')
	{
		for (i = 0 ; i < MCnprocesses ; i++)
			if (strequal(name, MCprocesses[i].name))
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
		if (MCS_pathtoref(docname, &t_tmp_fsref) == noErr && MCS_fsref_to_fsspec(&t_tmp_fsref, &fspec) == noErr)
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
		delete docname;
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
			delete name;
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
			else
				delete name;
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

static IO_handle MCS_dopen(int4 fd, const char *mode)
{
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
}

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

void MCS_launch_document(char *p_document)
{
	int t_error = 0;
	
	FSRef t_document_ref;
	if (t_error == 0)
	{
		errno = MCS_pathtoref(p_document, &t_document_ref);
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
	
	delete p_document;
}

void MCS_launch_url(char *p_document)
{
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

	delete p_document;
}

void MCS_checkprocesses()
{
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

}

void MCS_closeprocess(uint2 index)
{
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
}

void MCS_kill(int4 pid, int4 sig)
{
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
}

void MCS_killall()
{
	struct sigaction action;
	memset((char *)&action, 0, sizeof(action));
	action.sa_handler = (void (*)(int))SIG_IGN;
	sigaction(SIGCHLD, &action, NULL);
	while (MCnprocesses--)
	{
		delete MCprocesses[MCnprocesses].name;
		if (MCprocesses[MCnprocesses].pid != 0
		        && (MCprocesses[MCnprocesses].ihandle != NULL
		            || MCprocesses[MCnprocesses].ohandle != NULL))
		{
			kill(MCprocesses[MCnprocesses].pid, SIGKILL);
			waitpid(MCprocesses[MCnprocesses].pid, NULL, 0);
		}
	}
}

// MW-2005-02-22: Make this global scope for now to enable opensslsocket.cpp
//   to access it.
real8 curtime;

real8 MCS_time()
{
	struct timezone tz;
	struct timeval tv;

	gettimeofday(&tv, &tz);
	curtime = tv.tv_sec + (real8)tv.tv_usec / 1000000.0;

	return curtime;
}

void MCS_reset_time()
{
}

void MCS_sleep(real8 duration)
{
	unsigned long finalTicks;
	Delay((unsigned long)duration * 60, &finalTicks);
}

char *MCS_getenv(const char *name)
{
	return getenv(name); //always returns NULL under CodeWarrier env.
}

extern void MCS_setenv(const char *name, const char *value)
{
	setenv(name, value, True);

}

extern void MCS_unsetenv(const char *name)
{
	unsetenv(name);

}

int4 MCS_rawopen(const char *path, int flags)
{//this call is for unix audio device only.
	return 0;
}

int4 MCS_rawclose(int4 fd)
{//this call is for unix audio device only.
	return 0;
}

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


void MCS_getDNSservers(MCExecPoint &ep)
{
#define DNS_SCRIPT "repeat for each line l in url \"binfile:/etc/resolv.conf\";if word 1 of l is \"nameserver\" then put word 2 of l & cr after it; end repeat;delete last char of it; return it"
	ep . clear();
	MCresult->store(ep, False);
	MCdefaultstackptr->domess(DNS_SCRIPT);
	MCresult->fetch(ep);
}

Boolean MCS_getdevices(MCExecPoint &ep)
{
	ep.clear();


	io_iterator_t SerialPortIterator = NULL;
	mach_port_t masterPort = NULL;
	io_object_t thePort;
	if (FindSerialPortDevices(&SerialPortIterator, &masterPort) != KERN_SUCCESS)
	{
		char *buffer = new char[6 + I2L];
		sprintf(buffer, "error %d", errno);
		MCresult->copysvalue(buffer);
		delete buffer;
		return False;
	}
	uint2 portCount = 0;
	if (SerialPortIterator != 0)
	{
		while ((thePort = IOIteratorNext(SerialPortIterator)) != 0)
		{
			char ioresultbuffer[256];
			getIOKitProp(thePort, kIOTTYDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
			ep.concatcstring(ioresultbuffer, EC_RETURN, portCount == 0);//name
			getIOKitProp(thePort, kIODialinDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
			ep.concatcstring(ioresultbuffer, EC_COMMA, false);//TTY file
			getIOKitProp(thePort, kIOCalloutDeviceKey, ioresultbuffer, sizeof(ioresultbuffer));
			ep.concatcstring(ioresultbuffer, EC_COMMA, false);//CU file
			IOObjectRelease(thePort);
			portCount++;
		}
		IOObjectRelease(SerialPortIterator);
	}
	
	return True;
}


Boolean MCS_nodelay(int4 fd)
{
	return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & O_APPEND | O_NONBLOCK)
				 >= 0;
}

IO_stat MCS_shellread(int fd, char *&buffer, uint4 &buffersize, uint4 &size)
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


IO_stat MCS_runcmd(MCExecPoint &ep)
{

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
			MCprocesses[MCnprocesses].name = strclone("shell");
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
	char *buffer = ep.getbuffer(0);
	uint4 buffersize = ep.getbuffersize();
	uint4 size = 0;
	if (MCS_shellread(toparent[0], buffer, buffersize, size) != IO_NORMAL)
	{
		MCeerror->add(EE_SHELL_ABORT, 0, 0);
		close(toparent[0]);
		if (MCprocesses[index].pid != 0)
			MCS_kill(MCprocesses[index].pid, SIGKILL);
		ep.setbuffer(buffer, buffersize);
		return IO_ERROR;
	}
	ep.setbuffer(buffer, buffersize);
	ep.setlength(size);
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
		MCresult->store(ep2, False);
	}
	else
		MCresult->clear(False);


	return IO_NORMAL;
}


/**************** Socket Code  *********************************/

// MW-2005-02-22: Moved to opensslsocket.cpp


uint4 MCS_getpid()
{
	return getpid();
}

const char *MCS_getaddress()
{
	static struct utsname u;
	static char *buffer;
	uname(&u);
	if (buffer == NULL)
		buffer = new char[strlen(u.nodename) + strlen(MCcmd) + 4];
	sprintf(buffer, "%s:%s", u.nodename, MCcmd);
	return buffer;
}

const char *MCS_getmachine()
{
	static Str255 machineName;
	long response;
	if ((errno = Gestalt(gestaltMachineType, &response)) == noErr)
	{
		GetIndString(machineName, kMachineNameStrID, response);
		if (machineName != nil)
		{
			p2cstr(machineName);
			return (const char*)machineName;
		}
	}
	return "unknown";
}

// MW-2006-05-03: [[ Bug 3524 ]] - Make sure processor returns something appropriate in Intel
const char *MCS_getprocessor()
{ //get machine processor
#ifdef __LITTLE_ENDIAN__
	return "x86";
#else
  return "Motorola PowerPC";
#endif
}

real8 MCS_getfreediskspace(void)
{
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

const char *MCS_getsystemversion()
{
	static char versioninfo[12];
	
	long response;
	
	// MW-2007-10-30: [[ Bug 5406 ]] On OS X 10.4 and above we need to use a different method to fetch the version
	if (MCmajorosversion >= 0x1040)
	{
		long t_major, t_minor, t_bugfix;
		Gestalt(gestaltSystemVersionMajor, &t_major);
		Gestalt(gestaltSystemVersionMinor, &t_minor);
		Gestalt(gestaltSystemVersionBugFix, &t_bugfix);
		sprintf(versioninfo, "%d.%d.%d", t_major, t_minor, t_bugfix);
		return versioninfo;
	}
	else if ((errno = Gestalt(gestaltSystemVersion, &response)) == noErr)
	{
		uint2 i = 0;
		if (response & 0xF000)
			versioninfo[i++] = ((response & 0xF000) >> 12) + '0';
		versioninfo[i++] = ((response & 0xF00) >> 8) + '0';
		versioninfo[i++] = '.';
		versioninfo[i++] = ((response & 0xF0) >> 4) + '0';
		versioninfo[i++] = '.';
		versioninfo[i++] = (response & 0xF) + '0';
		versioninfo[i] = '\0';
		return versioninfo;
	}
	
	return NULL;
}

void MCS_query_registry(MCExecPoint &dest, const char** type)
{
	MCresult->sets("not supported");
	dest.clear();
}

void MCS_set_registry(const char *key, MCExecPoint &dest, char *type)
{
	MCresult->sets("not supported");
	dest.setboolean(False);
}

void MCS_delete_registry(const char *key, MCExecPoint &dest)
{
	MCresult->sets("not supported");
	dest.setboolean(False);
}

void MCS_list_registry(MCExecPoint& p_dest)
{
	MCresult -> sets("not supported");
	p_dest . setboolean(False);
}

Boolean MCS_poll(real8 delay, int fd)
{
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
}

/*****************************************************************************
 *  Apple events handler	 			 	             *
 *****************************************************************************/
 
// MW-2006-08-05: Vetted for Endian issues
void MCS_send(const MCString &message, const char *program,
              const char *eventtype, Boolean needReply)
{ //send "" to program "" with/without reply
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
		eventtype = "miscdosc";
		
	AEEventClass ac;
	AEEventID aid;
	
	ac = FourCharCodeFromString(eventtype);
	aid = FourCharCodeFromString(&eventtype[4]);
	
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
		
		char *doc = message.clone();
		if (MCS_pathtoref(doc, &t_fsref) == noErr && MCS_fsref_to_fsspec(&t_fsref, &fspec) == noErr)
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
		delete doc;
	}
	//non document related massge, assume it's typeChar message
	if (!docmessage && message.getlength())
		AEPutParamPtr(&ae, keyDirectObject, typeChar,
		              message.getstring(), message.getlength());

	//Send the Apple event
	AppleEvent answer;
	if (needReply)
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
	if (needReply)
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
void MCS_reply(const MCString &message, const char *keyword, Boolean error)
{
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
}

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
			char *fullPathName = MCS_fsref_to_path(t_doc_fsref);
			uint2 newlength = strlen(fullPathName) + 1;
			MCU_realloc(&string, length, length + newlength, 1);
			if (length)
				string[length - 1] = '\n';
			memcpy(&string[length], fullPathName, newlength);
			length += newlength;
			delete fullPathName;
		}
		string[length - 1] = '\0';
		AEDisposeDesc(&docList);
	}
	return true;
}

// MW-2006-08-05: Vetted for Endian issues
char *MCS_request_ae(const MCString &message, uint2 ae)
{
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
}

// MW-2006-08-05: Vetted for Endian issues
char *MCS_request_program(const MCString &message, const char *program)
{
	AEAddressDesc receiver;
	errno = getDescFromAddress(program, &receiver);
	if (errno != noErr)
	{
		AEDisposeDesc(&receiver);
		MCresult->sets("no such program");
		return MCU_empty();
	}
	AppleEvent ae;
	errno = AECreateAppleEvent('misc', 'eval', &receiver,
	                           kAutoGenerateReturnID, kAnyTransactionID, &ae);
	AEDisposeDesc(&receiver); //dispose of the receiver description record
	//add parameters to the Apple event
	AEPutParamPtr(&ae, keyDirectObject, typeChar,
	              message.getstring(), message.getlength());
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
		return MCU_empty();
	}
	real8 endtime = curtime + AETIMEOUT;
	while (True)
	{
		if (MCscreen->wait(READ_INTERVAL, False, True))
		{
			MCresult->sets("user interrupt");
			return MCU_empty();
		}
		if (curtime > endtime)
		{
			MCresult->sets("timeout");
			return MCU_empty();
		}
		if (AEanswerErr != NULL || AEanswerData != NULL)
			break;
	}
	if (AEanswerErr != NULL)
	{
		MCresult->copysvalue(AEanswerErr);
		delete AEanswerErr;
		AEanswerErr = NULL;
		return MCU_empty();
	}
	else
	{
		MCresult->clear(False);
		char *retval = AEanswerData;
		AEanswerData = NULL;
		return retval;
	}
}

char *MCS_FSSpec2path(FSSpec *fSpec)
{
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
}

char *MCS_fsref_to_path(FSRef& p_ref)
{
	char *t_path;
	t_path = new char[PATH_MAX + 1];
	
	FSRefMakePath(&p_ref, (UInt8 *)t_path, PATH_MAX);
	
	char *t_macroman_path;
	uint4 t_length;
	t_macroman_path = new char[PATH_MAX + 1];
	t_length = PATH_MAX;
	MCS_utf8tonative(t_path, strlen(t_path), t_macroman_path, t_length);
	t_macroman_path[t_length] = '\0';
	
	delete t_path;
	
	return t_macroman_path;
}

OSErr MCS_pathtoref_and_leaf(const char *p_path, FSRef& r_ref, UniChar*& r_leaf, UniCharCount& r_leaf_length)
{
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
}

OSErr MCS_fsspec_to_fsref(const FSSpec *p_fsspec, FSRef *r_fsref)
{
	return FSpMakeFSRef(p_fsspec, r_fsref);
}

OSErr MCS_fsref_to_fsspec(const FSRef *p_fsref, FSSpec *r_fsspec)
{
	return FSGetCatalogInfo(p_fsref, 0, NULL, NULL, r_fsspec, NULL);
}

OSErr MCS_pathtoref(const MCString& p_path, FSRef *r_ref)
{
	char *t_cstring_path;
	t_cstring_path = p_path . clone();
	
	OSErr t_error;
	t_error = MCS_pathtoref(t_cstring_path, r_ref);
	
	delete t_cstring_path;
	
	return t_error;
}

OSErr MCS_pathtoref(const char *p_path, FSRef *r_ref)
{
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
}

// based on MoreFiles (Apple DTS)
OSErr MCS_path2FSSpec(const char *fname, FSSpec *fspec)
{
	char *path = MCS_resolvepath(fname);
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
}

/**************************************************************************
 * Utility functions used by this module only
 **************************************************************************/

static OSErr getDescFromAddress(const char *address, AEDesc *retDesc)
{
	/* return an address descriptor based on the target address passed in
	  * * There are 3 possible forms of target string: *
	  * 1. ZONE:MACHINE:APP NAME 
	  * 2. MACHINE:APP NAME(sender & receiver are in the same zone but on different machine) 
	  * 3. APP NAME 
	  */
	errno = noErr;
	retDesc->dataHandle = NULL;  /* So caller can dispose always. */
	char *ptr = strchr(address, ':');

	if (ptr == NULL)
	{ //address contains application name only. Form # 3
		char *appname = new char[strlen(address) +1];
		strcpy(appname, address);
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
				result = MCS_FSSpec2path(&fs);
			}
			break;
		case typeFSRef:
			{
				FSRef t_fs_ref;
				errno = AEGetAttributePtr(ae, key, dt, &rType, &t_fs_ref, s, &rSize);
				result = MCS_fsref_to_path(t_fs_ref);
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
				result = MCS_FSSpec2path(&fs);
			}
			break;
		case typeFSRef:
			{
				FSRef t_fs_ref;
				errno = AEGetParamPtr(ae, key, dt, &rType, &t_fs_ref, s, &rSize);
				result = MCS_fsref_to_path(t_fs_ref);
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

void MCS_alternatelanguages(MCExecPoint &ep)
{
	ep.clear();
	getosacomponents();
	uint2 i;
	for (i = 0; i < osancomponents; i++)
		ep.concatcstring(osacomponents[i].compname, EC_RETURN, i == 0);
}

static OSErr osacompile(MCString &s, ComponentInstance compinstance,
                        OSAID &scriptid)
{
	AEDesc aedscript;
	char *buffer = s.clone();
	char *cptr = buffer;
	while (*cptr)
	{
		if (*cptr == '\n')
			*cptr = '\r';
		cptr++;
	}
	scriptid = kOSANullScript;
	AECreateDesc(typeChar, buffer, s.getlength(), &aedscript);
	OSErr err = OSACompile(compinstance, &aedscript, kOSAModeNull, &scriptid);
	AEDisposeDesc(&aedscript);
	delete buffer;
	return err;
}

static OSErr osaexecute(MCString &s, ComponentInstance compinstance,
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
	s.set(buffer,tsize);
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

void MCS_doalternatelanguage(MCString &s, const char *langname)
{
	getosacomponents();
	OSAcomponent *posacomp = NULL;
	uint2 i;
	uint4 l = strlen(langname);
	for (i = 0; i < osancomponents; i++)
	{
		if (l == strlen(osacomponents[i].compname)
		        && !MCU_strncasecmp(osacomponents[i].compname, langname, l))
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
	if (osacompile(s, posacomp->compinstance, scriptid) != noErr)
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

void MCS_nativetoutf16(const char *p_native, uint4 p_native_length, unsigned short *p_utf16, uint4& x_utf16_length)
{
	uint4 t_byte_length;
	t_byte_length = x_utf16_length * sizeof(unsigned short);
	MCS_multibytetounicode(p_native, p_native_length, (char *)p_utf16, t_byte_length, t_byte_length, LCH_ROMAN);
	x_utf16_length = t_byte_length / sizeof(unsigned short);
}

void MCS_utf16tonative(const unsigned short *p_utf16, uint4 p_utf16_length, char *p_native, uint4& p_native_length)
{
	MCS_unicodetomultibyte((const char *)p_utf16, p_utf16_length * 2, p_native, p_native_length, p_native_length, LCH_ROMAN);
}

Boolean MCS_isleadbyte(uint1 charset, char *s)
{
	if (!charset)
		return False;
	return CharacterByteType(s, 0, MCS_charsettolangid(charset)) == smFirstByte;
}

Boolean MCS_imeisunicode()
{
	OSErr err;
	long version;
	err = Gestalt(gestaltTSMgrVersion, &version);
	//only enable if os 9.0 or greater and TSM > 1.5
	if ((err == noErr) && (version >= gestaltTSMgr15))
		if (Gestalt(gestaltSystemVersion, &version) == noErr)
			return version >= 0x900;
	return False;
}

MCSysModuleHandle MCS_loadmodule(const char *p_filename)
{
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
}

void MCS_unloadmodule(MCSysModuleHandle p_module)
{
	CFRelease((CFBundleRef)p_module);
}

void *MCS_resolvemodulesymbol(MCSysModuleHandle p_module, const char *p_symbol)
{
	CFStringRef t_cf_symbol;
	t_cf_symbol = CFStringCreateWithCString(NULL, p_symbol, CFStringGetSystemEncoding());
	if (t_cf_symbol == NULL)
		return NULL;
	
	void *t_symbol_ptr;
	t_symbol_ptr = CFBundleGetFunctionPointerForName((CFBundleRef)p_module, t_cf_symbol);
	
	CFRelease(t_cf_symbol);
	
	return t_symbol_ptr;
}

bool MCS_processtypeisforeground(void)
{
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

bool MCS_changeprocesstype(bool to_foreground)
{
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
}

bool MCS_isatty(int fd)
{
	return isatty(fd) != 0;
}

bool MCS_isnan(double v)
{
	return isnan(v);
}

uint32_t MCS_getsyserror(void)
{
	return errno;
}

bool MCS_mcisendstring(const char *command, char buffer[256])
{
	strcpy(buffer, "not supported");
	return true;
}

void MCS_system_alert(const char *p_title, const char *p_message)
{
	CFStringRef t_cf_title, t_cf_message;
	t_cf_title = CFStringCreateWithCString(NULL, p_title, kCFStringEncodingMacRoman);
	t_cf_message = CFStringCreateWithCString(NULL, p_message, kCFStringEncodingMacRoman);
	DialogRef t_alert;
	CreateStandardAlert(kAlertStopAlert, t_cf_title, t_cf_message, NULL, &t_alert);
	
	DialogItemIndex t_result;
	RunStandardAlert(t_alert, NULL, &t_result);
	CFRelease(t_cf_title);
	CFRelease(t_cf_message);
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

void MCS_startprocess_unix(char *name, char *doc, Open_mode mode, Boolean elevated)
{
	Boolean noerror = True;
	Boolean reading = mode == OM_READ || mode == OM_UPDATE;
	Boolean writing = mode == OM_APPEND || mode == OM_WRITE || mode == OM_UPDATE;
	MCU_realloc((char **)&MCprocesses, MCnprocesses, MCnprocesses + 1, sizeof(Streamnode));
				
	// Store process information.
	uint2 index = MCnprocesses;
	MCprocesses[MCnprocesses].name = name;
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
				
				// The pid is 0, so here we are in the child process.
				// Construct the argument string to pass to the process..
				char **argv = NULL;
				uint32_t argc = 0;
				startprocess_create_argv(name, doc, argc, argv);
				
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
				MCS_nodelay(toparent[0]);
				// Store the in handle for the "process".
				MCprocesses[index].ihandle = MCS_dopen(toparent[0], IO_READ_MODE); 
			}
			if (writing)
			{
				close(tochild[0]);
				// Store the out handle for the "process".
				MCprocesses[index].ohandle = MCS_dopen(tochild[1], IO_WRITE_MODE); 
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
			t_status = AuthorizationExecuteWithPrivileges(t_auth, MCcmd, kAuthorizationFlagDefaults, t_arguments, &t_stream);
		}
		
		uint32_t t_pid;
		t_pid = 0;
		if (t_status == noErr)
		{
			char *t_name_dup;
			t_name_dup = strdup(name);
			
			// Split the arguments
			uint32_t t_argc;
			char **t_argv;
			startprocess_create_argv(t_name_dup, doc, t_argc, t_argv);
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
				MCS_nodelay(t_fd);
				MCprocesses[index].ihandle = MCS_dopen(t_fd, IO_READ_MODE);
			}
			if (writing)
				MCprocesses[index].ohandle = MCS_dopen(dup(fileno(t_stream)), IO_WRITE_MODE);
			
			noerror = True;
		}
		else
			noerror = False;
		
		if (t_stream != nil)
			fclose(t_stream);
		
		if (t_auth != nil)
			AuthorizationFree(t_auth, kAuthorizationFlagDefaults);
	}
	
	delete doc;
	if (!noerror || MCprocesses[index].pid == -1 || MCprocesses[index].pid == 0)
	{
		if (noerror)
			MCprocesses[index].pid = 0;
		else
			delete name;
		MCresult->sets("not opened");
	}
	else
		MCresult->clear(False);
}

////////////////////////////////////////////////////////////////////////////////
