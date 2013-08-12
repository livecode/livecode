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

#include "parsedef.h"
#include "filedefs.h"
#include "globdefs.h"
#include "objdefs.h"

#include "execpt.h"
#include "globals.h"
#include "system.h"
#include "osspec.h"
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

#include "osxdc.h"

#include <sys/stat.h>

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
static OSErr osacompile(MCString &s, ComponentInstance compinstance, OSAID &id);
static OSErr osaexecute(MCString &s,ComponentInstance compinstance, OSAID id);
/***************************************************************************/

///////////////////////////////////////////////////////////////////////////////

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
				if (aeid == kAEDoScript)
				{
					MCdefaultstackptr->getcard()->domess(sptr);
					MCresult->eval(ep);
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

bool MCS_specialfolder_to_mac_folder(MCStringRef p_type, uint32_t& r_folder, OSType& r_domain)
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
    
	char *controlptr = strclone(MCserialcontrolsettings);
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

void MCS_setfiletype(MCStringRef p_new_path)
{
	FSRef t_fsref;
    // TODO Check whether the double path resolution is an issue
	if (MCS_pathtoref(p_new_path, t_fsref) != noErr)
		return; // ignore errors
    
	FSCatalogInfo t_catalog;
	if (FSGetCatalogInfo(&t_fsref, kFSCatInfoFinderInfo, &t_catalog, NULL, NULL, NULL) == noErr)
	{
		// Set the creator and filetype of the catalog.
		memcpy(&((FileInfo *) t_catalog . finderInfo) -> fileType, &MCfiletype[4], 4);
		memcpy(&((FileInfo *) t_catalog . finderInfo) -> fileCreator, MCfiletype, 4);
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

///////////////////////////////////////////////////////////////////////////////

/* LEGACY */
extern char *path2utf(const char *);

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

///////////////////////////////////////////////////////////////////////////////

void MCS_startprocess_unix(MCNameRef name, MCStringRef doc, Open_mode mode, Boolean elevated);

///////////////////////////////////////////////////////////////////////////////

static Boolean hasPPCToolbox = False;
static Boolean hasAppleEvents = False;

///////////////////////////////////////////////////////////////////////////////

bool MCS_is_link(MCStringRef p_path)
{
	struct stat buf;
	return (lstat(MCStringGetCString(p_path), &buf) == 0 && S_ISLNK(buf.st_mode));
}

bool MCS_readlink(MCStringRef p_path, MCStringRef& r_link)
{
	struct stat t_stat;
	ssize_t t_size;
	MCAutoNativeCharArray t_buffer;
    
	if (lstat(MCStringGetCString(p_path), &t_stat) == -1 ||
		!t_buffer.New(t_stat.st_size))
		return false;
    
	t_size = readlink(MCStringGetCString(p_path), (char*)t_buffer.Chars(), t_stat.st_size);
    
	return (t_size == t_stat.st_size) && t_buffer.CreateStringAndRelease(r_link);
}

///////////////////////////////////////////////////////////////////////////////

class MCStdioFileHandle: public MCSystemFileHandle
{
public:    
	static MCStdioFileHandle *OpenFile(MCStringRef p_path, intenum_t p_mode)
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
        MCStdioFileHandle *t_handle;
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
            if (p_mode == kMCSystemFileModeRead)
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
        
        if (fptr == NULL && p_mode == kMCSystemFileModeRead)
            fptr = fopen(*t_path_utf, IO_CREATE_MODE);
        
        if (fptr != NULL && created)
            MCS_setfiletype(p_path);
        
		if (fptr != NULL)
        {
            t_handle = new MCStdioFileHandle;
            t_handle -> m_stream = fptr;
            t_handle -> m_hserialInputBuff = NULL;
            t_handle -> m_ioptr = NULL;
            t_handle -> m_serialIn = 0;
            t_handle -> m_serialOut = 0;
        }
        
        return t_handle;
	}
	
	static MCStdioFileHandle *OpenFd(uint32_t fd, intenum_t p_mode)
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
        
        switch (p_mode)
        {
            case kMCSystemFileModeAppend:
                t_stream = fdopen(fd, IO_APPEND_MODE);
                break;
            case kMCSystemFileModeRead:
                t_stream = fdopen(fd, IO_READ_MODE);
                break;
            case kMCSystemFileModeUpdate:
                t_stream = fdopen(fd, IO_UPDATE_MODE);
                break;
            case kMCSystemFileModeWrite:
                t_stream - fdopen(fd, IO_WRITE_MODE);
                break;
            default:
                break;
        }
		if (t_stream == NULL)
			return NULL;
		
		// MH-2007-05-17: [[Bug 3196]] Opening the write pipe to a process should not be buffered.
		if (p_mode == kMCSystemFileModeWrite)
			setvbuf(t_stream, NULL, _IONBF, 0);
		
		MCStdioFileHandle *t_handle;
		t_handle = new MCStdioFileHandle;
		t_handle -> m_stream = t_stream;
        t_handle -> m_hserialInputBuff = NULL;
        t_handle -> m_ioptr = NULL;
        t_handle -> m_serialIn = 0;
        t_handle -> m_serialOut = 0;
		
		return t_handle;
	}
    
    static MCStdioFileHandle *OpenDevice(MCStringRef p_path, const char *serialcontrolsettings)
    {
		FILE *fptr;
        MCStdioFileHandle *t_handle;
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
            val = fcntl(fileno(fptr), F_GETFL, val);
            val |= O_NONBLOCK |  O_NOCTTY;
            fcntl(fileno(fptr), F_SETFL, val);
            configureSerialPort((short)fileno(fptr));
            
            t_handle = new MCStdioFileHandle;
            t_handle -> m_stream = fptr;
            t_handle -> m_hserialInputBuff = NULL;
            t_handle -> m_ioptr = NULL;
            t_handle -> m_serialIn = 0;
            t_handle -> m_serialOut = 0;
        }
        
        return t_handle;
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
        if (m_serialIn != 0 || m_serialOut != 0)
        {//close the serial port
            
        }
        else if (m_stream != NULL)
            fclose(m_stream);
        
        delete m_stream;
        m_stream = NULL;
	}
	
	virtual IO_stat Read(void *p_ptr, uint32_t p_size, uint32_t& r_count)
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
        IO_stat stat = IO_NORMAL;
        uint4 nread;
        int32_t toread = GetFileSize();
        if (m_serialIn != 0)
        {//read from serial port
            SInt32 sint_toread = (SInt32) toread;
            long count = 0;  // n group of size data to be read
            
            count = MCU_min(count, p_size * r_count);
            if (count > 0)
                if ((errno = FSRead(m_serialIn, &sint_toread, p_ptr)) != noErr)
                    stat = IO_ERROR;
            if ((uint4)count < p_size * r_count)
                stat = IO_EOF;
            r_count = toread / p_size;
        }
        else
        {
            // MW-2010-08-26: Taken from the Linux source, this changes the previous code
            //   to take into account pipes and such.
            char *sptr = (char *)p_ptr;
            uint4 nread;
            uint4 toread = r_count * p_size;
            uint4 offset = 0;
            errno = 0;
            while ((nread = fread(&sptr[offset], 1, toread, m_stream)) != toread)
            {
                offset += nread;
                r_count = offset / p_size;
                if (ferror(m_stream))
                {
                    clearerr(m_stream);
                    
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
                if (feof(m_stream))
                {
                    return IO_NORMAL;
                }
                return IO_NONE;
            }
        }
        return stat;
	}
    
	virtual bool Write(const void *p_ptr, uint32_t p_size, uint32_t& r_count)
	{
#ifdef /* MCS_write */ LEGACY_SYSTEM
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
#endif /* MCS_write */
        uint32_t count;
        if (m_serialOut != 0)
        {//write to serial port
            uint4 count = p_size * r_count;
            errno = FSWrite(m_serialOut, (long*)&count, p_ptr);
            if (errno == noErr && count == p_size * r_count)
                return IO_NORMAL;
            return IO_ERROR;
        }
        count = p_size * r_count;
        if ((r_count = fwrite(p_ptr, p_size, r_count, m_stream)) != count)
            return IO_ERROR;
        return IO_NORMAL;
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
		int64_t t_pos;
		t_pos = ftello(m_stream);
		return fseeko(m_stream, t_pos, SEEK_SET) == 0;
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
		return fflush(m_stream) == 0;
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
		return ungetc(p_char, m_stream) != EOF;
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
		return NULL;
	}
	
	FILE *GetStream(void)
	{
		return m_stream;
	}
    
    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        
    }
    
	short m_serialIn;  //serial port Input reference number
	short m_serialOut; //serial port output reference number
	MCMacSysHandle m_hserialInputBuff; //handle to serial input buffer
	char *m_ioptr;
	
private:
	FILE *m_stream;
};

struct MCMacDesktop: public MCSystemInterface
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
        IO_stdin = new IO_header(MCStdioFileHandle::OpenFd(0, kMCSystemFileModeRead), 0);
        IO_stdout = new IO_header(MCStdioFileHandle::OpenFd(1, kMCSystemFileModeWrite), 0);
        IO_stderr = new IO_header(MCStdioFileHandle::OpenFd(2, kMCSystemFileModeWrite), 0);
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
    }
	virtual void Finalize(void)
    {
        
    }
	
	virtual void Debug(MCStringRef p_string)
    {
        
    }
    
	virtual real64_t GetCurrentTime(void)
    {
        
    }
    
	virtual bool GetVersion(MCStringRef& r_string)
    {
        
    }
	virtual bool GetMachine(MCStringRef& r_string)
    {
        
    }
	virtual MCNameRef GetProcessor(void)
    {
        
    }
	virtual void GetAddress(MCStringRef& r_string)
    {
        
    }
    
	virtual uint32_t GetProcessId(void)
    {
        
    }
	
	virtual void Alarm(real64_t p_when)
    {
        
    }
	virtual void Sleep(real64_t p_when)
    {
        
    }
	
	virtual void SetEnv(MCStringRef p_name, MCStringRef p_value)
    {
        
    }
    
	virtual bool GetEnv(MCStringRef p_name, MCStringRef& r_value)
    {
        return false;
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
    
    /* LEGACY */
    virtual bool DeleteFile(const char *p_path)
    {
        char *newpath = path2utf(MCS_resolvepath(p_path));
        Boolean done = remove(newpath) == 0;
        delete newpath;
        return done;
    }
	
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
            t_os_error = MCS_pathtoref(p_old_name, t_src_ref);
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
            t_os_error = MCS_pathtoref(p_new_name, t_dst_ref);
            if (t_os_error == noErr)
                FSDeleteObject(&t_dst_ref);
			
            // Get the information to create the file
            t_os_error = MCS_pathtoref_and_leaf(p_new_name, t_dst_parent_ref, t_dst_leaf, t_dst_leaf_length);
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
            t_os_error = MCS_pathtoref(p_old_name, t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSRef t_dst_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_new_name, t_dst_ref);
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
            t_error = !MCS_rename(p_old_name, p_new_name);
		
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
            t_os_error = MCS_pathtoref(p_alias, t_dst_ref);
            if (t_os_error == noErr)
                return False; // we expect an error
        }
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_target, t_src_ref);
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
            t_os_error = MCS_pathtoref_and_leaf(p_alias, t_dst_parent_ref, t_dst_leaf_name, t_dst_leaf_name_length);
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
        t_os_error = MCS_pathtoref(p_target, t_fsref);
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
        
        if (!MCS_fsref_to_path(t_fsref, r_resolved_path))
        {
            if (!MCStringCreateWithCString("can't get alias path", r_resolved_path))
                return False;
            
            return True;
        }
        
        return True;
    }
	
	virtual void GetCurrentFolder(MCStringRef& r_path)
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
        bool t_success;
        char namebuf[PATH_MAX + 2];
        if (NULL == getcwd(namebuf, PATH_MAX))
            t_success = false;
        
        MCAutoNativeCharArray t_buffer;
        if (t_success)
            t_success = t_buffer.New(PATH_MAX + 1);
        
        uint4 outlen;
        outlen = PATH_MAX + 1;
        MCS_utf8tonative(namebuf, strlen(namebuf), (char*)t_buffer.Chars(), outlen);
        t_buffer.Shrink(outlen);
        if (t_success)
            t_success = t_buffer.CreateStringAndRelease(r_path);
        
        if (!t_success)
            r_path = MCValueRetain(kMCEmptyString);            
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
        t_mac_folder = MCSwapInt32NetworkToHost(*((uint32_t*)MCStringGetBytePtr(p_type)));
        
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
        
        if (MCS_specialfolder_to_mac_folder(MCNameGetString(p_type), t_mac_folder, t_domain))
            t_found_folder = true;
        else if (MCStringGetLength(MCNameGetString(p_type)) == 4)
        {
            t_mac_folder = MCSwapInt32NetworkToHost(*((uint32_t*)MCStringGetBytePtr(MCNameGetString(p_type))));
			
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
		
        if (!MCS_fsref_to_path(t_folder_ref, r_folder))
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
            t_found = (buf.st_mode & S_IFDIR);
        
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
            t_found = (buf.st_mode & S_IFDIR) == 0;
        
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
#ifdef /* MCS_chmodMacDsk_dsk_mac */ LEGACY_SYSTEM
    return IO_NORMAL;
#endif /* MCS_chmodMacDsk_dsk_mac */
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
	virtual void GetTemporaryFileName(MCStringRef& r_tmp_name)
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
        bool t_error = false;
        MCAutoStringRef t_temp_file_auto;
        FSRef t_folder_ref;
        char* t_temp_file_chars;
        
        t_temp_file_chars = nil;        
        t_error = !MCStringCreateMutable(0, &t_temp_file_auto);
        
        if (!t_error && FSFindFolder(kOnSystemDisk, kTemporaryFolderType, TRUE, &t_folder_ref) == noErr)
        {
            int t_fd;
            t_error = !MCS_fsref_to_path(t_folder_ref, &t_temp_file_auto);
            
            if (!t_error)
                t_error = MCStringAppendFormat(&t_temp_file_auto, "/tmp.%d.XXXXXXXX", getpid());
            
            t_error = MCMemoryAllocateCopy(MCStringGetCString(*t_temp_file_auto), MCStringGetLength(*t_temp_file_auto) + 1, t_temp_file_chars);
            
            if (!t_error)
            {
                t_fd = mkstemp(t_temp_file_chars);
                t_error = t_fd != -1;
            }
            
            if (!t_error)
            {
                close(t_fd);
                t_error = unlink(t_temp_file_chars) != 0;
            }
        }
        
        if (!t_error)
            t_error = !MCStringCreateWithCString(t_temp_file_chars, r_tmp_name);
        
        if (t_error)
            r_tmp_name = MCValueRetain(kMCEmptyString);
        
        MCMemoryDeallocate(t_temp_file_chars);
    }
    
#define CATALOG_MAX_ENTRIES 16
	virtual bool ListFolderEntries(bool p_files, bool p_detailed, MCListRef& r_list)
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
#endif /* MCS_getentries_dsk_mac_dsk_mac */
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
#ifdef /* MCS_getdevices */ LEGACY_SYSTEM
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
#endif /* MCS_getdevices */
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
        r_drives = MCValueRetain(kMCEmptyString);
        return True;
    }
    
	virtual bool PathToNative(MCStringRef p_path, MCStringRef& r_native)
    {
        MCAutoStringRef t_resolved_path;
        
        if (!ResolvePath(p_path, &t_resolved_path))
            return false;
        
        return MCStringCopyAndRelease(*t_resolved_path, r_native);
    }
    
	virtual bool PathFromNative(MCStringRef p_native, MCStringRef& r_path)
    {
        MCAutoStringRef t_resolved_path;
        
        if (!ResolvePath(p_native, &t_resolved_path))
            return false;
        
        return MCStringCopyAndRelease(*t_resolved_path, r_path);
    }
    
	virtual bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path)
    {
#ifdef /* MCS_resolvepath */ LEGACY_SYSTEM
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
#endif /* MCS_resolvepath */
        if (MCStringGetLength(p_path) == 0)
        {
            MCS_getcurdir(r_resolved_path);
            return true;
        }
        
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
            MCS_getcurdir(&t_folder);
            
            MCAutoStringRef t_resolved;
            if (!MCStringMutableCopy(*t_folder, &t_fullpath) ||
                !MCStringAppendChar(*t_fullpath, '/') ||
                !MCStringAppend(*t_fullpath, *t_tilde_path))
                return false;
        }
        else
            t_fullpath = *t_tilde_path;
        
        if (!MCS_is_link(*t_fullpath))
            return MCStringCopy(*t_fullpath, r_resolved_path);
        
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
            
            return MCStringCopy(*t_resolved, r_resolved_path);
        }
        else
            return MCStringCopy(*t_newname, r_resolved_path);
    }
    
	virtual bool ResolveNativePath(MCStringRef p_path, MCStringRef& r_resolved_path)
    {
        
    }
	
	virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map, uint32_t p_offset)
    {
        MCStdioFileHandle *t_handle;
        t_handle = MCStdioFileHandle::OpenFile(p_path, p_mode);
        
        if (p_offset > 0)
            t_handle -> Seek(p_offset, SEEK_SET);
        
        return new IO_header(t_handle, 0);
    }
    
	virtual IO_handle OpenStdFile(uint32_t fd, intenum_t mode)
    {
        return new IO_header(MCStdioFileHandle::OpenFd(fd, mode), 0);
    }
    
	virtual IO_handle OpenDevice(MCStringRef p_path, const char *p_control_string, uint32_t p_offset)
    {
        MCStdioFileHandle *t_handle;
        t_handle = MCStdioFileHandle::OpenDevice(p_path, p_control_string);
        
        if (p_offset > 0)
            t_handle -> Seek(p_offset, SEEK_SET);
        
        return new IO_header(t_handle, 0);
    }
	
	virtual void *LoadModule(MCStringRef p_path)
    {
        
    }
    
	virtual void *ResolveModuleSymbol(void *p_module, const char *p_symbol)
    {
        
    }
	virtual void UnloadModule(void *p_module)
    {
        
    }
	
	virtual bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
    {
        
    }
	virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
    {
        
    }
	
	virtual bool Shell(MCStringRef filename, MCDataRef& r_data, int& r_retcode)
    {
        
    }
    
	virtual char *GetHostName(void)
    {
        
    }
	virtual bool HostNameToAddress(MCStringRef p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
    {
        
    }
	virtual bool AddressToHostName(MCStringRef p_address, MCSystemHostResolveCallback p_callback, void *p_context)
    {
        
    }
    
	virtual uint32_t TextConvert(const void *string, uint32_t string_length, void *buffer, uint32_t buffer_length, uint32_t from_charset, uint32_t to_charset)
    {
        
    }
	virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
    {
        
    }
    
    virtual void CheckProcesses()
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
                    // TODO Can the handler be a MCMemoryFileHandle?
                    MCStdioFileHandle *t_handle;
                    t_handle = static_cast<MCStdioFileHandle *>(MCprocesses[i].ihandle->handle);
                    clearerr(t_handle -> GetStream());
                }
                MCprocesses[i].pid = 0;
                MCprocesses[i].retcode = WEXITSTATUS(wstat);
            }
    }
};

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCDesktopCreateMacSystem(void)
{
	return new MCMacDesktop;
}


// MW-2005-02-22: Make this global scope for now to enable opensslsocket.cpp
//   to access it.
real8 curtime;

/*****************************************************************************
 *  Apple events handler	 			 	             *
 *****************************************************************************/

// MW-2006-08-05: Vetted for Endian issues
void MCS_send(MCStringRef message, MCStringRef program, MCStringRef eventtype, Boolean reply)
{
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
		
		if (MCS_pathtoref(message, t_fsref) == noErr && MCS_fsref_to_fsspec(&t_fsref, &fspec) == noErr)
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
void MCS_request_program(MCStringRef message, MCStringRef program, MCStringRef& r_value)
{
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
	char *ptr = strchr(MCStringGetCString(address), ':');
    
	if (ptr == NULL)
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

bool MCS_alternatelanguages(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
	
	getosacomponents();
	for (uindex_t i = 0; i < osancomponents; i++)
		if (!MCListAppendCString(*t_list, osacomponents[i].compname))
			return false;
	
	return MCListCopy(*t_list, r_list);
}

static OSErr osacompile(const MCString &s, ComponentInstance compinstance,
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

void MCS_doalternatelanguage(MCStringRef p_script, MCStringRef p_language)
{
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

void MCS_startprocess_launch(MCNameRef name, MCStringRef docname, Open_mode mode)
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
	errno = MCS_pathtoref(MCNameGetString(name), t_app_fsref);
	errno = MCS_fsref_to_fsspec(&t_app_fsref, &t_app_fsspec);
	
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
			if (MCS_pathtoref(docname, t_doc_fsref) != noErr)
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
		if (MCS_pathtoref(docname, t_tmp_fsref) == noErr && MCS_fsref_to_fsspec(&t_tmp_fsref, &fspec) == noErr)
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


void MCS_startprocess_unix(MCNameRef name, MCStringRef doc, Open_mode mode, Boolean elevated)
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
				MCS_nodelay(toparent[0]);
				// Store the in handle for the "process".
				MCprocesses[index].ihandle = MCsystem -> OpenStdFile(toparent[0], kMCSystemFileModeRead);
			}
			if (writing)
			{
				close(tochild[0]);
				// Store the out handle for the "process".
				MCprocesses[index].ohandle = MCsystem -> OpenStdFile(tochild[1], kMCSystemFileModeWrite);
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
				MCS_nodelay(t_fd);
				MCprocesses[index].ihandle = MCsystem -> OpenStdFile(t_fd, kMCSystemFileModeRead);
			}
			if (writing)
				MCprocesses[index].ohandle = MCsystem -> OpenStdFile(dup(fileno(t_stream)), kMCSystemFileModeWrite);
			
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