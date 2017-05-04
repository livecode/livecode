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

//
// globals for MetaCard
//
#ifndef __MC_GLOBALS__
#define __MC_GLOBALS__

#include "clipboard.h"
#include "mcstring.h"
#include "imagelist.h"
#include "parsedef.h"
#include "sysdefs.h"
#include "mcsemaphore.h"

#include "object.h"
#include "card.h"
#include "group.h"
#include "aclip.h"
#include "stack.h"
#include "player.h"
#include "image.h"
#include "magnify.h"
#include "field.h"
#include "tooltip.h"

#include "foundation-locale.h"

typedef struct _Streamnode Streamnode;
typedef struct _Linkatts Linkatts;

////////////////////////////////////////////////////////////////////////////////
//
//  GLOBAL VARIABLES
//

extern Bool MCquit;

// MW-2013-04-01: [[ Bug 10799 ]] If this is true, it means we must do an 'exit' after the
//   event loop ends. This is to distinguish an implicit quit from, say, the home button on
//   iOS.
extern Bool MCquitisexplicit;

extern int MCidleRate;

extern Boolean MCaqua;
extern MCStringRef MCcmd;

/* The app code path is the folder relative to which the engine can find all
 * its code resources */
extern MCStringRef MCappcodepath;

extern MCStringRef MCfiletype;
extern MCStringRef MCstackfiletype;

extern Boolean MCuseXft ;
extern Boolean MCuselibgnome ;
extern Boolean MCuseESD ;

#ifdef TARGET_PLATFORM_LINUX
class MCXImageCache;
extern MCXImageCache *MCimagecache ;

extern Boolean MCutf8 ;
extern Boolean MCXVideo ;

extern Window MClastvideowindow ;
#endif

extern MCStringRef *MCstacknames;

extern int2 MCnstacks;
extern Boolean MCnofiles;
extern Boolean MCmmap;
extern Boolean MCnopixmaps;
extern Boolean MCpointerfocus;
extern Boolean MCemacskeys;
extern Boolean MClowrestimers;
extern Boolean MCraisemenus;
extern Boolean MCsystemmodals;
extern Boolean MCactivatepalettes;
extern Boolean MChidepalettes;
extern Boolean MCraisepalettes;
extern Boolean MCproportionalthumbs;
extern Boolean MCdontuseNS;
extern Boolean MCdontuseQT;
extern Boolean MCdontuseQTeffects;
extern uint4 MCeventtime;
extern uint2 MCbuttonstate;
extern uint2 MCmodifierstate;
extern uint2 MCextendkey;
extern int2 MCclicklocx;
extern int2 MCclicklocy;
extern int2 MCmousex;
extern int2 MCmousey;
extern uint2 MCsiguser1;
extern uint2 MCsiguser2;
extern int4 MCinputfd;
extern int4 MCshellfd;

extern MCCursorRef MCcursors[];
extern Boolean MCshm;
extern Boolean MCshmpix ;
extern Boolean MCvcshm;
extern Boolean MCmmap;
extern Boolean MCnoui;
extern char *MCdisplayname;
extern Boolean MCshmoff;
extern Boolean MCshmon;
extern uint4 MCvisualid;
extern real8 MCgamma;

extern MCColor MCzerocolor;
extern MCColor MConecolor;
extern MCColor MCpencolor;
extern MCStringRef MCpencolorname;
extern MCColor MCbrushcolor;
extern MCColor MChilitecolor;
extern MCColor MCgraycolor;
extern MCStringRef MCbrushcolorname;
extern uint4 MCpenpmid;
extern MCPatternRef MCpenpattern;
extern uint4 MCbrushpmid;
extern MCPatternRef MCbrushpattern;
extern uint4 MCbackdroppmid;
extern MCPatternRef MCbackdroppattern;
extern MCImageList *MCpatternlist;
extern MCColor MCaccentcolor;
extern MCStringRef MCaccentcolorname;
extern MCColor MChilitecolor;
extern MCStringRef MChilitecolorname;
extern MCColor MCselectioncolor;
extern MCStringRef MCselectioncolorname;
extern Linkatts MClinkatts;
extern Boolean MCrelayergrouped;
extern Boolean MCselectgrouped;
extern Boolean MCselectintersect;
extern MCRectangle MCwbr;
extern uint2 MCjpegquality;
extern Export_format MCpaintcompression;
extern intenum_t MCrecordformat;
extern uint2 MCsoundchannel;
extern uint2 MCrecordsamplesize;
extern uint2 MCrecordchannels;
extern real8 MCrecordrate;
extern char MCrecordcompression[5];
extern char MCrecordinput[5];
extern Boolean MCuselzw;

extern real8 MCinfinity;
extern char* MCstackbottom;
extern Boolean MCcheckstack;
extern Boolean MCswapbytes;
extern Boolean MCtranslatechars;
extern Boolean MCdragging;
extern Streamnode *MCfiles;
extern Streamnode *MCprocesses;
extern MCSocket **MCsockets;
extern real8 MCsockettimeout;
extern real8 MCmaxwait;
extern uint2 MCnfiles;
extern uint2 MCnprocesses;
extern uint2 MCnsockets;
extern MCStack **MCusing;
extern uint2 MCnusing;
extern uint2 MCiconicstacks;
extern MCSemaphore MCwaitdepth;
extern uint4 MCrecursionlimit;


extern Boolean MCownselection;
extern MCUndolist *MCundos;
extern MCSellist *MCselected;
extern MCStacklist *MCstacks;
extern MCStacklist *MCtodestroy;
extern MCCardlist *MCrecent;
extern MCCardlist *MCcstack;
extern MCDispatch *MCdispatcher;
extern MCStackHandle MCtopstackptr;
extern MCStackHandle MCdefaultstackptr;
extern MCStackHandle MCstaticdefaultstackptr;
extern MCStackHandle MCmousestackptr;
extern MCStackHandle MCclickstackptr;
extern MCStackHandle MCfocusedstackptr;
extern MCObjectPartHandle MCtargetptr;
extern MCObjectHandle MCmenuobjectptr;
extern MCCardHandle MCdynamiccard;
extern Boolean MCdynamicpath;
extern MCGroup *MCsavegroupptr;
extern MCObjectHandle MCerrorptr;
extern MCObjectHandle MCerrorlockptr;
extern MCGroupHandle MCdefaultmenubar;
extern MCGroupHandle MCmenubar;
extern MCAudioClipHandle MCacptr;
extern MCPlayerHandle MCplayers;

extern MCStack *MCtemplatestack;
extern MCAudioClip *MCtemplateaudio;
extern MCVideoClip *MCtemplatevideo;
extern MCGroup *MCtemplategroup;
extern MCCard *MCtemplatecard;
extern MCButton *MCtemplatebutton;
extern MCGraphic *MCtemplategraphic;
extern MCEPS *MCtemplateeps;
extern MCScrollbar *MCtemplatescrollbar;
extern MCPlayer *MCtemplateplayer;
extern MCImage *MCtemplateimage;
extern MCField *MCtemplatefield;

extern MCImageHandle MCmagimage;
extern MCMagnifyHandle MCmagnifier;
extern MCObjectHandle MCdragsource;
extern MCObjectHandle MCdragdest;
extern MCFieldHandle MCactivefield;
extern MCFieldHandle MCclickfield;
extern MCFieldHandle MCfoundfield;
extern MCFieldHandle MCdropfield;
extern int4 MCdropchar;
extern MCImageHandle MCactiveimage;
extern MCImageHandle MCeditingimage;
extern MCTooltipHandle MCtooltip;
extern MCStackHandle MCmbstackptr;

extern MCUIDC *MCscreen;
extern MCPrinter *MCprinter;
extern MCPrinter *MCsystemprinter;

extern MCStringRef MCscriptfont;
extern uint2 MCscriptsize;
extern uint2 MCfocuswidth;
extern uint2 MCsizewidth;
extern uint2 MCminsize;
extern uint2 MCcloneoffset;
extern uint2 MCtitlebarheight;
extern uint2 MCdoubledelta;
extern uint2 MCdoubletime;
extern uint2 MCdragdelta;
extern uint2 MCblinkrate;
extern uint2 MCrepeatrate;
extern uint2 MCrepeatdelay;
extern uint2 MCtyperate;
extern uint2 MCrefreshrate;
extern uint2 MCsyncrate;
extern uint2 MCeffectrate;
extern uint2 MCminstackwidth;
extern uint2 MCminstackheight;
extern uint2 MCerrorlimit;
extern uint2 MCscrollbarwidth;
extern uint2 MCwmwidth;
extern uint2 MCwmheight;
extern uint2 MCcharset;
extern uint2 MCet;
extern Boolean MCabortscript;
extern Boolean MCalarm;
extern Boolean MCallowinterrupts;
extern Boolean MCinterrupt;
extern Boolean MCexplicitvariables;
extern Boolean MCpreservevariables;
extern Boolean MCsystemFS;
extern Boolean MCsystemCS;
extern Boolean MCsystemPS;
extern Boolean MChidewindows;
extern Boolean MCbufferimages;
extern MCStringRef MCserialcontrolsettings;
extern MCStringRef MCshellcmd;
extern MCStringRef MCvcplayer;

extern MCStringRef MCftpproxyhost;
extern uint2 MCftpproxyport;

extern MCStringRef MChttpproxy;

extern MCStringRef MChttpheaders;
extern int4 MCrandomseed;
extern Boolean MCshowinvisibles;
extern MCObjectList *MCbackscripts;
extern MCObjectList *MCfrontscripts;

// MW-2011-09-24: [[ Effects ]] Add support for rect restriction on lock/unlock screen with effects.
extern MCRectangle MCcur_effects_rect;
extern MCEffectList *MCcur_effects;
extern MCError *MCperror;
extern MCError *MCeerror;
extern MCVariable *MCmb;
extern MCVariable *MCeach;
extern MCVariable *MCresult;
extern MCExecResultMode MCresultmode;
extern MCVariable *MCurlresult;
extern MCVariable *MCglobals;
extern MCVariable *MCdialogdata;
extern MCStringRef MChcstat;
extern Boolean MCexitall;
extern int4 MCretcode;
extern Boolean MCrecording;

// MM-2012-09-05: [[ Property Listener ]] True if any listened objects have had any of thier props changed since last message loop.
//  Saves time parsing through the list of object listeners if no properties have changed.
extern Boolean MCobjectpropertieschanged;
// MM-2012-11-06: Allow the throttling of the propertyChanged message to be set by the user (minimum number of milliseconds between messages). 
extern uint32_t MCpropertylistenerthrottletime;

// MW-2013-03-20: [[ MainStacksChanged ]] Set to true if the list of mainStacks has changed.
extern Boolean MCmainstackschanged;

// global properties

extern uint2 MClook;
extern MCStringRef MCttbgcolor;
extern MCStringRef MCttfont;
extern uint2 MCttsize;
extern MCSemaphore MCtrylock;
extern MCSemaphore MCerrorlock;
extern Boolean MCwatchcursor;
extern Boolean MClockcursor;
extern MCCursorRef MCcursor;
extern uint4 MCcursorid;
extern MCCursorRef MCdefaultcursor;
extern uint4 MCdefaultcursorid;
extern uint2 MCbusycount;
extern uint2 MClockscreen;
extern Boolean MClockcolormap;
extern Boolean MClockerrors;
extern Boolean MClockmenus;
extern Boolean MClockmessages;
extern Boolean MClockrecent;
extern Boolean MCtwelvetime;
extern Boolean MCuseprivatecmap;
extern Tool MCcurtool;
extern Tool MColdtool;
extern uint4 MCbrush;
extern uint4 MCspray;
extern uint4 MCeraser;
extern Boolean MCcentered;
extern Boolean MCfilled;
extern Boolean MCgrid;
extern uint2 MCgridsize;
extern uint2 MClinesize;
extern uint2 MCstartangle;
extern uint2 MCarcangle;
extern uint1 *MCdashes;
extern uint2 MCndashes;
extern uint2 MCroundradius;
extern Boolean MCmultiple;
extern uint2 MCmultispace;
extern uint2 MCpolysides;
extern Boolean MCroundends;
extern uint2 MCslices;
extern uint2 MCmagnification;
extern uint2 MCdragspeed;
extern uint2 MCmovespeed;
extern uint2 MCtooltipdelay;
extern uint2 MCtooltime;
extern Boolean MClongwindowtitles;
extern Boolean MCblindtyping;
extern Boolean MCpowerkeys;
extern Boolean MCnavigationarrows;
extern Boolean MCtextarrows;
extern uint2 MCuserlevel;
extern Boolean MCusermodify;
extern Boolean MCinlineinput;
extern MCTheme *MCcurtheme;
extern Boolean MChidebackdrop;
extern Boolean MCraisewindows;
extern char *MCsslcertificates;
extern char *MCdefaultnetworkinterface;
extern uint4 MCstackfileversion;
extern uint4 MCmajorosversion;
extern Boolean MCignorevoiceoversensitivity;
extern uint4 MCqtidlerate;

extern MCStringRef MCcommandname;
extern MCArrayRef MCcommandarguments;

extern MCArrayRef MCenvironmentvariables;

#ifdef _LINUX_DESKTOP
extern Window MCgtkthemewindow;
#endif

#define RTB_ACCURATE_UNICODE_CONVERSIONS (1 << 0)
#define RTB_ACCURATE_UNICODE_INPUT (1 << 1)
#define RTB_NO_UNICODE_WINDOWS (1 << 2)
extern uint4 MCruntimebehaviour;

extern MCDragAction MCdragaction;
extern MCObjectHandle MCdragtargetptr;
extern MCDragActionSet MCallowabledragactions;
extern uint4 MCdragimageid;
extern MCPoint MCdragimageoffset;

extern MCClipboard* MCclipboard;
extern MCClipboard* MCselection;
extern MCClipboard* MCdragboard;
extern uindex_t MCclipboardlockcount;

extern uint4 MCsecuremode;

// MW-2009-02-02: Ideally we would eliminate globals like this for 
//   the cursor stuff

extern Boolean MCcursorcanbealpha;
extern Boolean MCcursorcanbecolor;
extern Boolean MCcursorbwonly;
extern int32_t MCcursormaxsize;

// MW-2010-10-01: Stack limits - MCpendingstacklimit is the uncommitted
//   limit. MCstacklimit is the current limit.
extern uint32_t MCpendingstacklimit;
extern uint32_t MCstacklimit;

extern Boolean MCappisactive;

// MW-2012-02-22: [[ NoScrollSave ]] This point stores the offset to apply to
//   objects when saving to account for them being in a scrolled group.
extern MCPoint MCgroupedobjectoffset;

// MW-2012-11-14: [[ Bug 10516 ]] When true, sending packets to broadcast
//   addresses will work.
extern Boolean MCallowdatagrambroadcasts;

// Character encoding used by the system
extern char *MCsysencoding;

// Locales
extern MCLocaleRef kMCBasicLocale;
extern MCLocaleRef kMCSystemLocale;

// A callback to invoke to fetch the current mainwindow to use for modal dialog
// parenting.
typedef void *(*MCMainWindowCallback)(void);
extern MCMainWindowCallback MCmainwindowcallback;

////////////////////////////////////////////////////////////////////////////////
//
//  LIFECYCLE
//

struct X_init_options
{
    /* Standard argc, argv and envp */
    int argc = 0;
    MCStringRef *argv = nullptr;
    MCStringRef *envp = nullptr;
    
    /* Specifies the base folder to use to resolve relative library paths */
    MCStringRef app_code_path = nullptr;

    /* Specifies the root main window of the application */
    MCMainWindowCallback main_window_callback = nullptr;
};

/* These are the main lifecycle functions. They are implemented separately for
 * desktop, server, mobile and emscripten engines. */
bool X_init(const X_init_options& p_options);
bool X_main_loop_iteration(void);
int X_close(void);

bool X_open(int argc, MCStringRef argv[], MCStringRef envp[]);
void X_clear_globals(void);
void X_initialize_names(void);

////////////////////////////////////////////////////////////////////////////////
//
//  HOOK REGISTRATION
//

struct MCHookGlobalHandlersDescriptor
{
    bool (*can_handle)(MCNameRef message_name);
    bool (*handle)(MCNameRef message, MCParameter *parameters, Exec_stat& r_result);
};

struct MCHookNativeControlsDescriptor
{
    bool (*lookup_type)(MCStringRef name, intenum_t& r_type);
    bool (*lookup_property)(MCStringRef name, intenum_t& r_type);
    bool (*lookup_action)(MCStringRef name, intenum_t& r_type);
    bool (*create)(intenum_t type, void*& r_control);
    bool (*action)(intenum_t action, void *control, MCValueRef *arguments, uindex_t argument_count);
};

enum MCHookType
{
    kMCHookGlobalHandlers,
    kMCHookNativeControls,
};

struct MCHook;
extern MCHook *MChooks;

typedef bool (*MCHookForEachCallback)(void *context, void *descriptor);

bool MCHookRegister(MCHookType type, void *descriptor);
void MCHookUnregister(MCHookType type, void *descriptor);
bool MCHookForEach(MCHookType type, MCHookForEachCallback callback, void *context);

////////////////////////////////////////////////////////////////////////////////
//
//  GLOBAL HANDLERS
//

bool MCIsGlobalHandler(MCNameRef name);
bool MCRunGlobalHandler(MCNameRef message, MCParameter *parameters, Exec_stat& r_result);

////////////////////////////////////////////////////////////////////////////////
//
//  NATIVE CONTROLS
//

bool MCLookupNativeControlType(MCStringRef p_type_name, intenum_t& r_type);
bool MCLookupNativeControlProperty(MCStringRef p_name, intenum_t& r_prop);
bool MCLookupNativeControlAction(MCStringRef p_name, intenum_t& r_action);
bool MCCreateNativeControl(intenum_t type, void*& r_control);
bool MCPerformNativeControlAction(intenum_t action, void *control, MCValueRef *arguments, uindex_t argument_count);

////////////////////////////////////////////////////////////////////////////////
//
//  POST EXECUTION ACTIONS
//

enum
{
    kMCActionsUpdateScreen = 1 << 0,
    kMCActionsDrainDeletedObjects = 1 << 2,
};

extern uint32_t MCactionsrequired;
extern void MCActionsDoRunSome(uint32_t mask);

inline void MCActionsSchedule(uint32_t mask)
{
    MCactionsrequired |= mask;
}

inline void MCActionsRunAll(void)
{
    if (MCactionsrequired != 0)
        MCActionsDoRunSome(UINT32_MAX);
}

inline void MCActionsRunSome(uint32_t mask)
{
    if ((MCactionsrequired & mask) != 0)
        MCActionsDoRunSome(mask);
}

inline void MCRedrawUpdateScreen(void)
{
    MCActionsRunSome(kMCActionsUpdateScreen);
}

inline void MCDeletedObjectsDrain(void)
{
    MCActionsRunSome(kMCActionsDrainDeletedObjects);
}

///////////////////////////////////////////////////////////////////////////////

#endif
