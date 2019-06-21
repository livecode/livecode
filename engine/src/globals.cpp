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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "undolst.h"
#include "sellst.h"
#include "stacklst.h"
#include "dispatch.h"
#include "hndlrlst.h"
#include "cardlst.h"

#include "stack.h"
#include "tooltip.h"
#include "aclip.h"
#include "vclip.h"
#include "group.h"
#include "card.h"
#include "button.h"
#include "graphic.h"
#include "eps.h"
#include "scrolbar.h"
#include "player.h"
#include "image.h"
#include "field.h"
#include "mcerror.h"
#include "util.h"
#include "date.h"
#include "hc.h"
#include "external.h"
#include "parentscript.h"
#include "osspec.h"
#include "variable.h"
#include "widget.h"

#include "printer.h"

#include "regex.h"
#include "debug.h"
#include "visual.h"
#include "font.h"

#include "globals.h"
#include "license.h"
#include "mode.h"
#include "securemode.h"
#include "socket.h"
#include "mctheme.h"
#include "mcssl.h"
#include "stacksecurity.h"
#include "resolution.h"
#include "redraw.h"

#include "date.h"
#include "stacktile.h"

#include "widget-events.h"

#include "stackfileformat.h"

#define HOLD_SIZE1 65535
#define HOLD_SIZE2 16384

#ifdef TARGET_PLATFORM_MACOS_X
//#include <Foundation/NSAutoreleasePool.h>
#endif

#ifdef _ANDROID_MOBILE
#include "mblad.h"
#include "mblcontrol.h"
#endif

#ifdef _MOBILE
#include "mblsyntax.h"
#include "mblsensor.h"
#endif

#include "exec.h"
#include "chunk.h"

////////////////////////////////////////////////////////////////////////////////

Bool MCquit;
Bool MCquitisexplicit;
int MCidleRate = 200;

Boolean MCaqua;

/* The path to the base folder which contains the application's code resources.
 * This is used to resolve references to code resources. This is added as a
 * prefix to the (relative) paths computed in MCU_library_load. */
MCStringRef MCappcodepath = nullptr;

MCStringRef MCcmd = nullptr;
MCStringRef MCfiletype = nullptr;
MCStringRef MCstackfiletype = nullptr;


#ifdef TARGET_PLATFORM_LINUX
MCXImageCache *MCimagecache ;

Boolean MCutf8 ;
Boolean MCXVideo ;

Window MClastvideowindow = DNULL ;
#endif

Boolean MCuseXft = True;
Boolean MCuselibgnome = False ;
Boolean MCuseESD = False ;

MCStringRef *MCstacknames = NULL;
int2 MCnstacks = 0;

Boolean MCnofiles = False;
uint4 MCsecuremode = 0;
Boolean MCnopixmaps;
Boolean MClowrestimers;
#if defined X11 && !defined LINUX && !defined DARWIN
Boolean MCpointerfocus;
#else
Boolean MCpointerfocus = True;
#endif
Boolean MCemacskeys;
#if defined LINUX || defined DARWIN || defined BSD
Boolean MCraisemenus;
Boolean MCraisepalettes;
#else
#ifdef _MACOSX
Boolean MCraisemenus;
#else
Boolean MCraisemenus = True;
#endif
Boolean MCraisepalettes = True;
#endif
Boolean MCsystemmodals;
Boolean MCactivatepalettes = True;

// MW-2007-07-05: [[ Bug 2288 ]] Default for hidePalettes is not system-standard
#ifdef _MACOSX
Boolean MChidepalettes = True;
#else
Boolean MChidepalettes = False;
#endif

Boolean MCdontuseNS;
Boolean MCdontuseQT;
Boolean MCdontuseQTeffects;
uint4 MCeventtime;
uint2 MCbuttonstate;
uint2 MCmodifierstate;
uint2 MCextendkey = 5;
int2 MCclicklocx;
int2 MCclicklocy;
int2 MCmousex;
int2 MCmousey;
uint2 MCsiguser1;
uint2 MCsiguser2;
int4 MCinputfd = -1;
int4 MCshellfd = -1;

MCCursorRef MCcursors[PI_NCURSORS];
Boolean MCshm;
Boolean MCvcshm;
Boolean MCmmap = True;
Boolean MCshmpix;
Boolean MCnoui;
char *MCdisplayname = NULL;
Boolean MCshmoff;
Boolean MCshmon;
uint4 MCvisualid;
#ifdef _MACOSX
real8 MCgamma = 1.7;
#else
real8 MCgamma = 2.2;
#endif
Boolean MCproportionalthumbs = True;
MCColor MCzerocolor;
MCColor MConecolor;
MCColor MCpencolor;
MCStringRef MCpencolorname;
MCColor MCbrushcolor;
MCStringRef MCbrushcolorname;
uint4 MCpenpmid = PI_PATTERNS;
MCPatternRef MCpenpattern;
uint4 MCbrushpmid = PI_PATTERNS;
MCPatternRef MCbrushpattern;
uint4 MCbackdroppmid;
MCPatternRef MCbackdroppattern;
MCImageList *MCpatternlist;
MCColor MCaccentcolor;
MCStringRef MCaccentcolorname;
MCColor MChilitecolor = { 0, 0, 0x8080 };
MCStringRef MChilitecolorname;
MCColor MCselectioncolor;
MCStringRef MCselectioncolorname;
Linkatts MClinkatts = { { 0, 0, 0xEFBE }, NULL,
                        { 0xFFFF, 0, 0 }, NULL,
                        { 0x5144, 0x1861, 0x8038 }, NULL, True };
Boolean MCrelayergrouped;
Boolean MCselectgrouped;
Boolean MCselectintersect = True;
MCRectangle MCwbr;
uint2 MCjpegquality = 100;
Export_format MCpaintcompression = EX_PBM;
uint2 MCrecordchannels = 1;
uint2 MCrecordsamplesize = 8;
real8 MCrecordrate = 22.050;
intenum_t MCrecordformat;
char MCrecordcompression[5] = "raw ";
char MCrecordinput[5] = "dflt";
Boolean MCuselzw;

real8 MCinfinity = 0.0;
char *MCstackbottom;
Boolean MCcheckstack = True;
Boolean MCswapbytes;
Boolean MCtranslatechars;
Boolean MCdragging;
Streamnode *MCfiles;
Streamnode *MCprocesses;
MCSocket **MCsockets;
real8 MCsockettimeout = 10.0;
real8 MCmaxwait = 60.0;
uint2 MCnfiles;
uint2 MCnprocesses;
uint2 MCnsockets;
MCStack **MCusing;
uint2 MCnusing;
uint2 MCiconicstacks;
MCSemaphore MCwaitdepth;
uint4 MCrecursionlimit = 400000; // actual max is about 480K on OSX

MCClipboard* MCclipboard;
MCClipboard* MCselection;
MCClipboard* MCdragboard;
uindex_t MCclipboardlockcount;

MCDragAction MCdragaction;
MCDragActionSet MCallowabledragactions;
uint4 MCdragimageid;
MCPoint MCdragimageoffset;
MCObjectHandle MCdragtargetptr;
uint2 MCdragdelta = 4;

MCUndolist *MCundos;
MCSellist *MCselected;
MCStacklist *MCstacks;
MCStacklist *MCtodestroy;
MCCardlist *MCrecent;
MCCardlist *MCcstack;
MCDispatch *MCdispatcher;
MCStackHandle MCtopstackptr;
MCStackHandle MCdefaultstackptr;
MCStackHandle MCstaticdefaultstackptr;
MCStackHandle MCmousestackptr;
MCStackHandle MCclickstackptr;
MCStackHandle MCfocusedstackptr;
MCCardHandle MCdynamiccard;
Boolean MCdynamicpath;
MCObjectHandle MCerrorptr;
MCObjectHandle MCerrorlockptr;
MCObjectPartHandle MCtargetptr;
MCGroup *MCsavegroupptr;
MCObjectHandle MCmenuobjectptr;
MCGroupHandle MCdefaultmenubar;
MCGroupHandle MCmenubar;
MCPlayerHandle MCplayers;
MCAudioClipHandle MCacptr;

MCStack *MCtemplatestack;
MCAudioClip *MCtemplateaudio;
MCVideoClip *MCtemplatevideo;
MCGroup *MCtemplategroup;
MCCard *MCtemplatecard;
MCButton *MCtemplatebutton;
MCGraphic *MCtemplategraphic;
MCEPS *MCtemplateeps;
MCScrollbar *MCtemplatescrollbar;
MCPlayer *MCtemplateplayer;
MCImage *MCtemplateimage;
MCField *MCtemplatefield;

MCImageHandle MCmagimage;
MCMagnifyHandle MCmagnifier;
MCObjectHandle MCdragsource;
MCObjectHandle MCdragdest;
MCFieldHandle MCactivefield;
MCFieldHandle MCclickfield;
MCFieldHandle MCfoundfield;
MCFieldHandle MCdropfield;
int4 MCdropchar;
MCImageHandle MCactiveimage;
MCImageHandle MCeditingimage;
MCTooltipHandle MCtooltip;

MCUIDC *MCscreen;
MCPrinter *MCprinter;
MCPrinter *MCsystemprinter;

MCStringRef MCscriptfont;
uint2 MCscriptsize;
uint2 MCscrollbarwidth = DEFAULT_SB_WIDTH;
uint2 MCfocuswidth = 2;
uint2 MCsizewidth = 8;
uint2 MCminsize = 8;
uint2 MCcloneoffset = 32;
uint2 MCtitlebarheight = 26;
uint2 MCdoubledelta = 4;
uint2 MCdoubletime = 250;
uint2 MCblinkrate = 600;
uint2 MCrepeatrate = 50;
uint2 MCrepeatdelay = 250;
uint2 MCtyperate = 100;
uint2 MCsyncrate = 20;
uint2 MCeffectrate = 2000;
uint2 MCdragspeed = 0;
uint2 MCmovespeed = 200;
uint2 MCtooltipdelay = 500;
uint2 MCtooltime = 5000;
uint2 MCminstackwidth = 32;
uint2 MCminstackheight = 32;
uint2 MCerrorlimit = 1024;
uint2 MCwmwidth = 20;
uint2 MCwmheight = 32;
uint2 MCcharset = 1;
Boolean MCabortscript;
Boolean MCalarm;
Boolean MCallowinterrupts = True;
Boolean MCinterrupt;
Boolean MCexplicitvariables = False;
Boolean MCpreservevariables = False;
Boolean MCsystemFS = True;
Boolean MCsystemCS = True;
Boolean MCsystemPS = True;
Boolean MChidewindows;
Boolean MCbufferimages;
MCStringRef MCserialcontrolsettings;
MCStringRef MCshellcmd;
MCStringRef MCvcplayer;

MCStringRef MCftpproxyhost;
uint2 MCftpproxyport;

MCStringRef MChttpproxy;

MCStringRef MChttpheaders;
int4 MCrandomseed;
Boolean MCshowinvisibles;
MCObjectList *MCbackscripts;
MCObjectList *MCfrontscripts;

// MW-2011-09-24: [[ Effects ]] Add support for rect restriction on lock/unlock screen with effects.
MCRectangle MCcur_effects_rect;
MCEffectList *MCcur_effects;
MCError *MCperror;
MCError *MCeerror;
MCVariable *MCglobals;
MCVariable *MCmb;
MCVariable *MCeach;
MCVariable *MCdialogdata;
MCStringRef MChcstat;

MCVariable *MCresult;
MCExecResultMode MCresultmode;
MCVariable *MCurlresult;
Boolean MCexitall;
int4 MCretcode;
Boolean MCrecording;
#ifdef FEATURE_PLATFORM_RECORDER
MCPlatformSoundRecorderRef MCrecorder;
#endif

// AL-2014-18-02: [[ UnicodeFileFormat ]] Make current stackfile version the default.
uint4 MCstackfileversion = kMCStackFileFormatCurrentVersion;
uint2 MClook;
MCStringRef MCttbgcolor;
MCStringRef MCttfont;
uint2 MCttsize;
MCSemaphore MCtrylock;
MCSemaphore MCerrorlock;
Boolean MCwatchcursor;
Boolean MClockcursor;
MCCursorRef MCcursor;
uint4 MCcursorid;
MCCursorRef MCdefaultcursor = NULL;
uint4 MCdefaultcursorid;
uint2 MCbusycount;
uint2 MClockscreen;
Boolean MClockcolormap;
Boolean MClockerrors;
Boolean MClockmenus;
Boolean MClockmessages;
Boolean MClockrecent;
Boolean MCtwelvetime = True;
Boolean MCuseprivatecmap;
Tool MCcurtool = T_BROWSE;
Tool MColdtool = T_BROWSE;
uint4 MCbrush = 108;
uint4 MCspray = 134;
uint4 MCeraser = 102;
Boolean MCcentered;
Boolean MCfilled;
Boolean MCgrid = True;
uint2 MCgridsize = 4;
uint2 MClinesize = 0;
uint1 *MCdashes = NULL;
uint2 MCndashes = 0;
uint2 MCroundradius = DEFAULT_RADIUS;
uint2 MCstartangle = 0;
uint2 MCarcangle = 360;
Boolean MCmultiple;
uint2 MCmultispace = 1;
uint2 MCpolysides = 4;
Boolean MCroundends;
uint2 MCslices = 16;
uint2 MCmagnification = 8;
Boolean MClongwindowtitles;
Boolean MCblindtyping = True;
Boolean MCpowerkeys = True;
Boolean MCnavigationarrows = True;
Boolean MCtextarrows = True;
uint2 MCuserlevel = 8;
Boolean MCusermodify = True;
uint2 MCsoundchannel;
MCString MCtruemcstring;
MCString MCfalsemcstring;
MCString MCnullmcstring;
Boolean MCinlineinput = True;

uint4 MCqtidlerate = 50;

Boolean MChidebackdrop = False;
Boolean MCraisewindows = False;

uint4 MCmajorosversion = 0;
// PM-2014-12-08: [[ Bug 13659 ]] Toggle the value of ignoreVoiceOverSensitivity property of iOS native controls
Boolean MCignorevoiceoversensitivity = False;

MCTheme *MCcurtheme = NULL;

#ifdef _LINUX_DESKTOP
Window MCgtkthemewindow = DNULL;
#endif

char *MCsslcertificates = NULL;
char *MCdefaultnetworkinterface = NULL;

uint4 MCruntimebehaviour = 0;

Boolean MCcursorcanbealpha = False;
Boolean MCcursorcanbecolor = False;
Boolean MCcursorbwonly = True;
int32_t MCcursormaxsize = 32;

#define kMCStackLimit 8 * 1024 * 1024
uint32_t MCstacklimit = kMCStackLimit;
uint32_t MCpendingstacklimit = kMCStackLimit;

Boolean MCappisactive = False;

// MW-2012-02-22: [[ NoScrollSave ]] The point containing the offset to apply
//   to objects when saving.
MCPoint MCgroupedobjectoffset;

IO_handle IO_stdin;
IO_handle IO_stdout;
IO_handle IO_stderr;

// MM-2012-09-05: [[ Property Listener ]]
Boolean MCobjectpropertieschanged = False;
uint32_t MCpropertylistenerthrottletime = 250;

// MW-2013-03-20: [[ MainStacksChanged ]]
Boolean MCmainstackschanged = False;

// MW-2012-11-13: [[ Bug 10516 ]] Flag to determine whether we allow broadcast
//   UDP sockets.
Boolean MCallowdatagrambroadcasts = False;

char *MCsysencoding = nil;

MCLocaleRef kMCBasicLocale = nil;
MCLocaleRef kMCSystemLocale = nil;

uint32_t MCactionsrequired = 0;

MCArrayRef MCenvironmentvariables;

// SN-2015-07-17: [[ CommandArguments ]] Add global array for the arguments.
MCStringRef MCcommandname;
MCArrayRef MCcommandarguments;

MCHook *MChooks = nil;

// The main window callback to compute the window to parent root modal dialogs to (if any)
MCMainWindowCallback MCmainwindowcallback = nullptr;

////////////////////////////////////////////////////////////////////////////////

extern MCUIDC *MCCreateScreenDC(void);
extern void MCU_finalize_names();

#ifdef _IOS_MOBILE
extern void MCReachabilityEventInitialize();
extern void MCReachabilityEventFinalize();
#endif

#ifdef _ANDROID_MOBILE
void MCAndroidCustomFontsInitialize();
void MCAndroidCustomFontsFinalize();
void MCAndroidFinalizeBuildInfo();
#endif
	
// Reset all global variables to their on-load defaults. This is required on
// Android, as the shared library the engine is compiled as is not reloaded on
// every app start.
void X_clear_globals(void)
{
	MCquit = False;
	MCquitisexplicit = False;
	MCidleRate = 200;
	/* FRAGILE */ MCcmd = nullptr;
    MCappcodepath = nullptr;
	MCfiletype = MCValueRetain(kMCEmptyString);
	MCstackfiletype = MCValueRetain(kMCEmptyString);
	MCstacknames = nil;
	MCnstacks = 0;
	MCnofiles = False;
	MCsecuremode = 0;
	MCnopixmaps = False;
	MClowrestimers = False;
	MCpointerfocus = True;
	MCemacskeys = False;
	MCraisemenus = True;
	MCraisepalettes = True;
	MCsystemmodals = True;
    MCactivatepalettes = True;
    MCdontuseQT = True;
    // SN-2014-10-2-: [[ Bug 13684 ]] Bugfix brought in 7.0 initialisation
    // MW-2007-07-05: [[ Bug 2288 ]] Default for hidePalettes is not system-standard
#ifdef _MACOSX
	MChidepalettes = True;
#else
    MChidepalettes = False;
#endif
	MCdontuseNS = False;
	MCdontuseQTeffects = True;
	MCeventtime = 0;
	MCbuttonstate = 0;
	MCmodifierstate = 0;
	MCextendkey = 5;
	MCclicklocx = 0;
	MCclicklocy = 0;
	MCmousex = 0;
	MCmousey = 0;
	MCsiguser1 = 0;
	MCsiguser2 = 0;
	MCinputfd = -1;
	MCshellfd = -1;
	MCshm = False;
	MCvcshm = False;
	MCmmap = True;
	MCshmpix = False;
	MCnoui = False;
	MCdisplayname = NULL;
	MCshmoff = False;
	MCshmon = False;
	MCvisualid = 0;
	MCgamma = 2.2;
	MCproportionalthumbs = True;
	memset(&MCzerocolor, 0, sizeof(MCColor));
	memset(&MConecolor, 0, sizeof(MCColor));
	memset(&MCpencolor, 0, sizeof(MCColor));
	MCpencolorname = nil;
	memset(&MCbrushcolor, 0, sizeof(MCColor));
	MCbrushcolorname = nil;
	MCpenpmid = PI_PATTERNS;
	MCpenpattern = nil;
	MCbrushpmid = PI_PATTERNS;
	MCbrushpattern = nil;
	MCbackdroppmid = 0;
	MCbackdroppattern = nil;
	MCpatternlist = nil;
	memset(&MCaccentcolor, 0, sizeof(MCColor));
	MCaccentcolorname = nil;
	memset(&MChilitecolor, 0, sizeof(MCColor));
	MChilitecolor . blue = 0x8080;
	MChilitecolorname = nil;
	memset(&MCselectioncolor, 0, sizeof(MCColor));
	MCselectioncolorname = nil;
	memset(&MClinkatts, 0, sizeof(Linkatts));
	MClinkatts . color . blue = 0xEFBE;
	MClinkatts . hilitecolor . red = 0xFFFF;
	MClinkatts . visitedcolor . red = 0x5144;
	MClinkatts . visitedcolor . green = 0x1861;
	MClinkatts . visitedcolor . blue = 0x8038;
	MClinkatts . underline = True;
	MCrelayergrouped = False;
	MCselectgrouped = False;
	MCselectintersect = True;
	memset(&MCwbr, 0, sizeof(MCRectangle));
	MCjpegquality = 100;
	MCpaintcompression = EX_PBM;
	MCrecordformat = 0;
	MCrecordchannels = 1;
	MCrecordsamplesize = 8;
	MCrecordrate = 22.050;
	strcpy(MCrecordcompression, "raw ");
	strcpy(MCrecordinput, "dflt");
	MCuselzw = False;
	MCinfinity = 0.0;
	MCcheckstack = True;
	MCswapbytes = False;
	MCtranslatechars = False;
	MCdragging = False;
	MCfiles = nil;
	MCprocesses = nil;
	MCsockets = nil;
	MCsockettimeout = 10.0;
	MCmaxwait = 60.0;
	MCnfiles = 0;
	MCnprocesses = 0;
	MCnsockets = 0;
	MCusing = nil;
	MCnusing = 0;
	MCiconicstacks = 0;
	MCwaitdepth = MCSemaphore("wait-depth");
	MCrecursionlimit = 400000;
	MCclipboard = NULL;
	MCselection = NULL;
    MCdragboard = NULL;
    MCclipboardlockcount = 0;
	MCdragaction = 0;
	MCallowabledragactions = 0;
	MCdragimageid = 0;
	memset(&MCdragimageoffset, 0, sizeof(MCPoint));
	MCdragtargetptr = nil;
	MCdragdelta = 4;
	MCundos = nil;
	MCselected = nil;
	MCstacks = nil;
    MCtodestroy = nil;
	MCrecent = nil;
	MCcstack = nil;
	MCdispatcher = nil;
	MCtopstackptr = nil;
	MCdefaultstackptr = nil;
	MCstaticdefaultstackptr = nil;
	MCmousestackptr = nil;
	MCclickstackptr = nil;
	MCfocusedstackptr = nil;
	MCdynamiccard = nil;
	MCdynamicpath = False;
	MCerrorptr = nil;
	MCerrorlockptr = nil;
	MCtargetptr = nullptr;
	MCmenuobjectptr = nil;
	MCsavegroupptr = nil;
	MCdefaultmenubar = nil;
	MCmenubar = nil;
	MCplayers = nil;
	MCacptr = nil;
	MCtemplatestack = nil;
	MCtemplateaudio = nil;
	MCtemplatevideo = nil;
	MCtemplategroup = nil;
	MCtemplatecard = nil;
	MCtemplatebutton = nil;
	MCtemplategraphic = nil;
	MCtemplateeps = nil;
	MCtemplatescrollbar = nil;
	MCtemplateplayer = nil;
	MCtemplateimage = nil;
	MCtemplatefield = nil;
	MCmagimage = nil;
	MCmagnifier = nil;
	MCdragsource = nil;
	MCdragdest = nil;
	MCactivefield = nil;
	MCclickfield = nil;
	MCfoundfield = nil;
	MCdropfield = nil;
	MCdropchar = 0;
	MCactiveimage = nil;
	MCeditingimage = nil;
	MCtooltip = nil;
	MCscreen = nil;
	MCsystemprinter = nil;
	MCprinter = nil;
	MCscriptfont = MCValueRetain(kMCEmptyString);
	MCscriptsize = 0;
	MCscrollbarwidth = DEFAULT_SB_WIDTH;
	MCfocuswidth = 2;
	MCsizewidth = 8;
	MCminsize = 8;
	MCcloneoffset = 32;
	MCtitlebarheight = 26;
	MCdoubledelta = 4;
	MCdoubletime = 250;
	MCblinkrate = 600;
	MCrepeatrate = 50;
	MCrepeatdelay = 250;
	MCtyperate = 100;
	MCsyncrate = 20;
	MCeffectrate = 2000;
	MCdragspeed = 0;
	MCmovespeed = 200;
	MCtooltipdelay = 500;
	MCtooltime = 5000;
	MCminstackwidth = 32;
	MCminstackheight = 32;
	MCerrorlimit = 1024;
	MCwmwidth = 20;
	MCwmheight = 32;
	MCcharset = 1;
	MCabortscript = False;
	MCalarm = False;
	MCallowinterrupts = True;
	MCinterrupt = False;
	MCexplicitvariables = False;
	MCpreservevariables = False;
	MCsystemFS = True;
	MCsystemCS = True;
	MCsystemPS = True;
	MChidewindows = False;
	MCbufferimages = False;
	MCserialcontrolsettings = MCValueRetain(kMCEmptyString);
	MCshellcmd = MCValueRetain(kMCEmptyString);
	MCvcplayer = MCValueRetain(kMCEmptyString);
	MCftpproxyhost = MCValueRetain(kMCEmptyString);
	MCftpproxyport = 0;
	MChttpproxy = MCValueRetain(kMCEmptyString);
	MChttpheaders = MCValueRetain(kMCEmptyString);
	MCrandomseed = 0;
	MCshowinvisibles = False;
	MCbackscripts = nil;
	MCfrontscripts = nil;
	MCcur_effects = nil;
	MCperror = nil;
	MCeerror = nil;
	MCglobals = nil;
	MCmb = nil;
	MCeach = nil;
	MCdialogdata = nil;
	MChcstat = nil;
	
	MCresult = nil;
	MCurlresult = nil;
	MCexitall = False;
	MCretcode = 0;
	MCrecording = False;
#ifdef FEATURE_PLATFORM_RECORDER
    MCrecorder = nil;
#endif
    
	// AL-2014-18-02: [[ UnicodeFileFormat ]] Make current stackfile version the default.
	MCstackfileversion = kMCStackFileFormatCurrentVersion;

    MClook = LF_MOTIF;
    MCttbgcolor = MCSTR("255,255,207");
    MCttfont = MCSTR(DEFAULT_TEXT_FONT);
    MCttsize = 12;

    MCtrylock = MCSemaphore("try");
    MCerrorlock = MCSemaphore("error");
	MCwatchcursor = False;
	MClockcursor = False;
	MCcursor = nil;
	MCcursorid = 0;
	MCdefaultcursor = nil;
	MCdefaultcursorid = 0;
	MCbusycount = 0;
	MClockscreen = False;
	MClockerrors = False;
	MClockmenus = False;
	MClockmessages = False;
	MClockrecent = False;
	MCtwelvetime = True;
	MCuseprivatecmap = False;
	MCcurtool = T_BROWSE;
	MColdtool = T_BROWSE;
	MCbrush = 108;
	MCspray = 134;
	MCeraser = 102;
	MCcentered = False;
	MCfilled = False;
	MCgrid = True;
	MCgridsize = 4;
	MClinesize = 0;
	MCdashes = nil;
	MCndashes = 0;
	MCroundradius = DEFAULT_RADIUS;
	MCstartangle = 0;
	MCarcangle = 360;
	MCmultiple = False;
	MCmultispace = 1;
	MCpolysides = 4;
	MCroundends = False;
	MCslices = 16;
	MCmagnification = 8;
	MClongwindowtitles = False;
	MCblindtyping = True;
	MCpowerkeys = True;
	MCnavigationarrows = True;
	MCtextarrows = True;
	MCuserlevel = 8;
	MCusermodify = True;
	MCsoundchannel = 0;
	MCinlineinput = True;
	MCqtidlerate = 50;
	MChidebackdrop = False;
	MCraisewindows = False;
	MCcurtheme = nil;
	MCsslcertificates = nil;
	MCdefaultnetworkinterface = nil;

	MCtruemcstring = MCtruestring;
	MCfalsemcstring = MCfalsestring;
	MCnullmcstring = NULL;

	// MW-2013-03-11: [[ Bug 10713 ]] Make sure we reset the regex cache globals to nil.
	// JS-2013-07-01: [[ EnhancedFilter ]] Refactored regex caching mechanism.
	MCR_clearcache();

	for(uint32_t i = 0; i < PI_NCURSORS; i++)
		MCcursors[i] = nil;
    
	// MM-2012-09-05: [[ Property Listener ]]
	MCobjectpropertieschanged = False;
	MCpropertylistenerthrottletime = 250;
    
	// MW-2013-03-20: [[ MainStacksChanged ]]
	MCmainstackschanged = False;
    
    MCactionsrequired = 0;
    
    MChooks = nil;

    memset(&MClicenseparameters, 0, sizeof(MCLicenseParameters));
    
#if defined(MCSSL)
    MCSocketsInitialize();
#endif
    
    MCenvironmentvariables = nil;

    MCcommandarguments = NULL;
    MCcommandname = NULL;

#ifdef _MOBILE
    MCSensorInitialize();
    MCSystemSoundInitialize();
#endif
    
#ifdef _ANDROID_MOBILE
    extern void MCAndroidMediaPickInitialize();
    // MM-2012-02-22: Initialize up any static variables as Android static vars are preserved between sessions
    MCAdInitialize();
    MCNativeControlInitialize();
    MCAndroidCustomFontsInitialize();
	MCAndroidMediaPickInitialize();
#endif
	
#ifdef _IOS_MOBILE
    MCReachabilityEventInitialize();
#endif
	
	MCDateTimeInitialize();
    
    MClogmessage = MCNAME("log");
}

/* ---------------------------------------------------------------- */

/* Helper function for X_open_environment_variables() */
static bool
X_open_environment_variables_store(MCArrayRef x_array,
                                   MCStringRef p_name_str,
                                   MCStringRef p_value,
                                   bool p_make_global)
{
	MCNewAutoNameRef t_name;
	if (!MCNameCreate(p_name_str, &t_name))
	{
		return false;
	}

	/* Store the environment variable in the array of
	 * environment variables.  Note that if there are
	 * duplicates, we always use the *first* value found in
	 * the environment.  This matches the behaviour of most
	 * shells. See also bug 13622.
	 *
	 * The global array of environment variables is *case
	 * sensitive*, because "path" and "PATH" are distinct.
	 */
	MCValueRef t_current_value;
	if (MCArrayFetchValue(x_array, true, *t_name, t_current_value))
	{
		return true; /* We already have a value for this variable */
	}

	if (!MCArrayStoreValue(x_array, true, *t_name, p_value))
	{
		return false;
	}

	// SN-2014-06-12 [[ RefactorServer ]] We don't want to duplicate
	// the environment variables on server, as they have been copied
	// to $_SERVER
#ifndef _SERVER
	/* Create a global variable for the environment variable, but only
	 * if the variable name is a valid token that doesn't start with
	 * "#" or "0".  These rules are to match the way MCVariable
	 * detects whether a variable proxies an environment variable. */
	unichar_t t_first = MCStringGetCharAtIndex(p_name_str, 0);
	if (p_make_global &&
	    '#' != t_first &&
	    !isdigit(t_first) &&
	    MCU_is_token(p_name_str))
	{
		MCAutoStringRef t_global_str;
		MCNewAutoNameRef t_global;
		if (!MCStringFormat(&t_global_str, "$%@", p_name_str))
		{
			return false;
		}
		if (!MCNameCreate(*t_global_str, &t_global))
		{
			return false;
		}

		MCVariable *t_var;
		MCVariable::ensureglobal(*t_global, t_var);
		t_var->setvalueref(p_value);
	}
#endif
	return true;
}

/* Parse environment variables.  All environment variables are placed
 * into the MCenvironmentvariables global array.  Some environment
 * variables are turned into special "$<name>" global LiveCode
 * variables, but only if they are well-formed (i.e. in the
 * format "name=value"). */
static bool
X_open_environment_variables(MCStringRef envp[])
{
	if (nil == envp || !MCModeHasEnvironmentVariables())
	{
		MCenvironmentvariables = nil;
		return true;
	}

	/* Create the array of raw environment variables */
	MCAutoArrayRef t_env_array;
	if (!MCArrayCreateMutable(&t_env_array))
	{
		return false;
	}

	/* Create an list for degenerate environment variables (ones which
	 * are not in the "name=value" format.  These are considered after
	 * all other variables have been processed. */
	/* FIXME use MCProperListRef */
	MCAutoArrayRef t_degenerate;
	if (!MCArrayCreateMutable(&t_degenerate))
	{
		return false;
	}

	for (uint32_t i = 0 ; envp[i] != nil ; i++)
	{
		MCStringRef t_env_var = envp[i];
		MCAutoStringRef t_env_namestr;
		MCNewAutoNameRef t_env_name;
		MCAutoStringRef t_env_value;
		uindex_t t_equal;

		/* Split the environment variable into name and value.  If the
		 * environment string doesn't contain an '=' character, treat
		 * the whole string as a name, and delay processing. */
		if (MCStringFirstIndexOfChar(t_env_var, '=', 0,
		                             kMCStringOptionCompareExact, t_equal))
		{
			if (!MCStringCopySubstring(t_env_var, MCRangeMake(0, t_equal),
			                           &t_env_namestr))
			{
				return false;
			}

			if (!MCStringCopySubstring(t_env_var,
			                           MCRangeMake(t_equal + 1, UINDEX_MAX),
			                           &t_env_value))
			{
				return false;
			}

			if (!X_open_environment_variables_store(*t_env_array,
			                                        *t_env_namestr,
			                                        *t_env_value,
			                                        true))
			{
				return false;
			}
		}
		else
		{
			/* Delay for processing later. */
			if (!MCArrayStoreValueAtIndex(*t_degenerate,
			                              MCArrayGetCount(*t_degenerate) + 1,
			                              t_env_var))
			{
				return false;
			}
		}
	}

	/* Process degenerate environment variables */
	for (uindex_t i = 1; i <= MCArrayGetCount(*t_degenerate); ++i)
	{
		MCValueRef t_env_var;
		if (!MCArrayFetchValueAtIndex(*t_degenerate, i, t_env_var))
		{
			return false;
		}

		if (!X_open_environment_variables_store(*t_env_array,
		                                        static_cast<MCStringRef>(t_env_var),
		                                        kMCEmptyString, false))
		{
			return false;
		}
	}

	return MCArrayCopy(*t_env_array, MCenvironmentvariables);
}

/* ---------------------------------------------------------------- */

// Important: This function is on the emterpreter whitelist. If its
// signature function changes, the mangled name must be updated in
// em-whitelist.json
bool X_open(int argc, MCStringRef argv[], MCStringRef envp[])
{
	MCperror = new (nothrow) MCError();
	MCeerror = new (nothrow) MCError();
	/* UNCHECKED */ MCVariable::createwithname(MCNAME("MCresult"), MCresult);
    MCresultmode = kMCExecResultModeReturn;

	/* UNCHECKED */ MCVariable::createwithname(MCNAME("MCurlresult"), MCurlresult);
	/* UNCHECKED */ MCVariable::createwithname(MCNAME("MCdialogdata"), MCdialogdata);
	
	////

	MCU_init();
	
	////
    
    // MW-2014-04-15: [[ Bug 12185 ]] Initialize graphics and openssl before anything
    //   that might use them.
    
	// MM-2013-09-03: [[ RefactorGraphics ]] Initialize graphics library.
	MCGraphicsInitialize();
	
	// MM-2014-02-14: [[ LibOpenSSL 1.0.1e ]] Initialise the openlSSL module.
#ifdef MCSSL
	InitialiseSSL();
#endif
    
    ////
    
#if defined(_MACOSX) && defined(FEATURE_QUICKTIME)
    // MW-2014-07-21: Make AVFoundation the default on 10.8 and above.
    if (MCmajorosversion < 0x1080)
    {
        MCdontuseQT = False;
        MCdontuseQTeffects = False;
    }
#endif
    
    ////
    
	MCpatternlist = new (nothrow) MCImageList();

	/* UNCHECKED */ MCVariable::ensureglobal(MCN_msg, MCmb);
	MCmb -> setmsg();

	/* UNCHECKED */ MCVariable::ensureglobal(MCN_each, MCeach);

	/* Environment variables */
	if (!X_open_environment_variables(envp))
	{
		return false;
	}

	/* Handle special "MCNOFILES" environment variable */
	if (nil != MCenvironmentvariables)
	{
		MCValueRef t_env_value;
		if (MCArrayFetchValue(MCenvironmentvariables, true, MCNAME("MCNOFILES"), t_env_value) &&
		    MCStringGetCharAtIndex(static_cast<MCStringRef>(t_env_value), 0) != '0')
		{
			MCnofiles = True;
			MCsecuremode = MC_SECUREMODE_ALL;
		}
	}

    // SN-2015-07-17: [[ CommandArguments ]] Initialise the commandName and
    //  commandArguments properties.
    MCcommandname = NULL;
    MCcommandarguments = NULL;
    if (MCModeHasCommandLineArguments())
    {
        MCcommandname = MCValueRetain(argv[0]);

        bool t_success;
        t_success = MCArrayCreateMutable(MCcommandarguments);

        // We build a 1-based numeric array.
        for (index_t i = 1; t_success && i < argc; i++)
            t_success = MCArrayStoreValueAtIndex(MCcommandarguments, i, argv[i]);

        if (!t_success)
            return false;
    }
    else
    {
        MCcommandname = MCValueRetain(kMCEmptyString);
        MCcommandarguments = MCValueRetain(kMCEmptyArray);
    }
    
    MCDeletedObjectsSetup();

	/* UNCHECKED */ MCStackSecurityCreateStack(MCtemplatestack);
	MCtemplateaudio = new (nothrow) MCAudioClip;
	MCtemplateaudio->init();
	MCtemplatevideo = new (nothrow) MCVideoClip;
	MCtemplategroup = new (nothrow) MCGroup;
	MCtemplatecard = new (nothrow) MCCard;
	MCtemplatebutton = new (nothrow) MCButton;
	MCtemplategraphic = new (nothrow) MCGraphic;
	MCtemplatescrollbar = new (nothrow) MCScrollbar;
	MCtemplateplayer = new (nothrow) MCPlayer;
	MCtemplateimage = new (nothrow) MCImage;
	MCtemplatefield = new (nothrow) MCField;
	
	MCtooltip = new (nothrow) MCTooltip;

    MCclipboard = MCClipboard::CreateSystemClipboard();
    MCdragboard = MCClipboard::CreateSystemDragboard();
    MCselection = MCClipboard::CreateSystemSelectionClipboard();
    MCclipboardlockcount = 0;
	
	MCundos = new (nothrow) MCUndolist;
	MCselected = new (nothrow) MCSellist;
	MCstacks = new (nothrow) MCStacklist(true);
	// IM-2016-11-22: [[ Bug 18852 ]] Changes to MCtodestroy shouldn't affect MCtopstack
    MCtodestroy = new (nothrow) MCStacklist(false);
	MCrecent = new (nothrow) MCCardlist;
	MCcstack = new (nothrow) MCCardlist;

#ifdef _LINUX_DESKTOP
	MCValueAssign(MCvcplayer, MCSTR("xanim"));
#else
	MCValueAssign(MCvcplayer, MCSTR(""));
#endif

	MCValueAssign(MCfiletype, MCSTR("ttxtTEXT"));
    uindex_t t_last_path_separator;
    // find the actual command to execute
     if (!MCStringLastIndexOfChar(MCcmd, PATH_SEPARATOR, MCStringGetLength(MCcmd), kMCStringOptionCompareExact, t_last_path_separator))
		t_last_path_separator = 0;
	else
		t_last_path_separator++;
    
	if (MCStringFind(MCcmd, MCRangeMake(t_last_path_separator, 3), MCSTR("rev"), kMCStringOptionCompareExact, nil))
		MCValueAssign(MCstackfiletype, MCSTR("MCRDMSTK"));
	else
		MCValueAssign(MCstackfiletype, MCSTR("RevoRSTK"));
	MCValueAssign(MCserialcontrolsettings, MCSTR("baud=9600 parity=N data=8 stop=1"));

	MCdispatcher = new (nothrow) MCDispatch;
    MCdispatcher -> add_transient_stack(MCtooltip);

	// IM-2014-08-14: [[ Bug 12372 ]] Pixel scale setup needs to happen before the
	// creation of MCscreen to ensure screen rects are scaled/unscaled as appropriate.
	// IM-2014-01-27: [[ HiDPI ]] Initialize pixel scale settings
	MCResInitPixelScaling();
	
	if (MCnoui)
		MCscreen = new (nothrow) MCUIDC;
	else
	{
		MCscreen = MCCreateScreenDC();
				
		if (!MCscreen->open())
			return false;
	}

    MCExecContext ctxt(nil, nil, nil);
	MCInterfaceInitialize(ctxt);

	// IM-2014-01-27: [[ HiDPI ]] Initialize pixel scale settings
	MCResInitPixelScaling();
    
    // Set up fonts
	MCdispatcher -> open();

	// This is here because it relies on MCscreen being initialized.
	MCtemplateeps = new (nothrow) MCEPS;

	MCsystemFS = MCscreen -> hasfeature(PLATFORM_FEATURE_OS_FILE_DIALOGS);
	MCsystemCS = MCscreen -> hasfeature(PLATFORM_FEATURE_OS_COLOR_DIALOGS);
	MCsystemPS = MCscreen -> hasfeature(PLATFORM_FEATURE_OS_PRINT_DIALOGS);

	MCTheme *newtheme = MCThemeCreateNative();
	if (newtheme != nil)
	{
		if (newtheme->load())
		{
			MCcurtheme = newtheme;
			MClook = MCcurtheme->getthemefamilyid();
		}
		else
        {
            // Fall back to the Win95 theme if the Windows theme failed to load
            if (newtheme->getthemefamilyid() == LF_WIN95)
                MClook = LF_WIN95;
            delete newtheme;
        }
    }

	MCsystemprinter = MCprinter = MCscreen -> createprinter();
	MCprinter -> Initialize();
	
    MCwidgeteventmanager = new (nothrow) MCWidgetEventManager;
    
    /* Now that the script engine state has been initialized, we can load all
     * builtin extensions. */
    if (!MCExtensionInitialize())
    {
        return false;
    }
    
	// MW-2009-07-02: Clear the result as a startup failure will be indicated
	//   there.
	MCresult -> clear();
	if (MCdispatcher->startup() != IO_NORMAL)
		return false;

	return true;
}

int X_close(void)
{
    // MW-2012-02-23: [[ FontRefs ]] Finalize the font module.
    MCFontFinalize();
    
	// MW-2008-01-18: [[ Bug 5711 ]] Make sure we disable the backdrop here otherwise we
	//   get crashiness on Windows due to hiding the backdrop calling WindowProc which
	//   attempts to access stacks that have been deleted...
	if (!MCnoui)
	{
		MCscreen -> disablebackdrop(false);
		MCscreen -> disablebackdrop(true);
	}

    MCExecContext ctxt(nil, nil, nil);
	MCInterfaceFinalize(ctxt);

	MCstacks->closeall();
    
    MCscreen->DoRunloopActions();
    
    /* Finalize all builtin extensions */
    MCExtensionFinalize();

	MCselected->clear(False);
    MCundos->freestate();
    
	MCU_play_stop();
#ifdef FEATURE_PLATFORM_RECORDER
    if (MCrecorder != nil)
    {
        MCPlatformSoundRecorderStop(MCrecorder);
        MCPlatformSoundRecorderRelease(MCrecorder);
    }
#else
	if (MCrecording)
	{
		extern void MCQTStopRecording(void);
		MCQTStopRecording();
	}
#endif
	MClockmessages = True;
	MCS_killall();

    // Flush all engine clipboards to the system clipboards
    MCclipboard->FlushData();
    MCselection->FlushData();
    MCdragboard->FlushData();

    MCdispatcher -> remove_transient_stack(MCtooltip);
	delete MCtooltip;
	MCtooltip = nil;

	MCValueRelease(MChttpproxy);
	MCValueRelease(MCpencolorname);
	MCValueRelease(MCbrushcolorname);
	MCValueRelease(MChilitecolorname);
	MCValueRelease(MCaccentcolorname);
	MCValueRelease(MCselectioncolorname);

	while (MCnfiles)
		IO_closefile(MCfiles[0].name);
	if (MCfiles != NULL)
		delete[] MCfiles; /* Allocated with new[] */
	if (MCprocesses != NULL)
		delete[] MCprocesses; /* Allocated with new[] */
	if (MCnsockets)
		while (MCnsockets)
			delete MCsockets[--MCnsockets];
	if (MCsockets != NULL)
		delete MCsockets;

	while (MCsavegroupptr != NULL)
	{
		MCControl *gptr = MCsavegroupptr->remove(MCsavegroupptr);
        gptr -> removereferences();
		delete gptr;
	}

	// MW-2012-02-14: [[ FontRefs ]] Close the dispatcher before deleting it.
	MCdispatcher -> close();
	delete MCdispatcher;

	delete MCtemplatestack;
	delete MCtemplateaudio;
	delete MCtemplatevideo;
	delete MCtemplategroup;
	delete MCtemplatecard;
	delete MCtemplatebutton;
	delete MCtemplategraphic;
	delete MCtemplateeps;
	delete MCtemplatescrollbar;
	delete MCtemplateplayer;

	MCImage::shutdown();

#ifndef _LINUX_DESKTOP
	MCscreen->close(True);
#endif

	delete MCtemplateimage;
	delete MCtemplatefield;
	delete MCselected;
    delete MCtodestroy;
	delete MCstacks;
    
    MCModeFinalize();
    
    MCDeletedObjectsTeardown();
    
    // These card lists must be deleted *after* draining the deleted
    // objects pool, as they are used in stack destructors
    delete MCcstack;
    delete MCrecent;
    
	// Temporary workaround for a crash
    //MCS_close(IO_stdin);
	//MCS_close(IO_stdout);
	//MCS_close(IO_stderr);

	delete MCpatternlist;
	delete MCresult;
	delete MCurlresult;
	delete MCdialogdata;
	MCValueRelease(MChcstat);

	delete[] MCusing; /* Allocated with new[] */
	MCValueRelease(MChttpheaders);
	MCValueRelease(MCscriptfont);
	MCValueRelease(MClinkatts . colorname);
	MCValueRelease(MClinkatts . hilitecolorname);
	MCValueRelease(MClinkatts . visitedcolorname);
	MCB_clearwatches();
	MCB_clearbreaks(nil);
	MCU_cleaninserted();
	while (MCglobals != NULL)
	{
		MCVariable *tvar = MCglobals;
		MCglobals = MCglobals->getnext();
		delete tvar;
	}

	// JS-2013-06-21: [[ EnhancedFilter ]] refactored regex caching mechanism
    MCR_clearcache();

	delete MCperror;
	delete MCeerror;

    MCclipboard->Release();
    MCselection->Release();
    MCdragboard->Release();

	MCValueRelease(MCshellcmd);
	MCValueRelease(MCvcplayer);
	MCValueRelease(MCfiletype);
	MCValueRelease(MCstackfiletype);
	MCValueRelease(MCserialcontrolsettings);
	
	MCprinter -> Finalize();
	delete MCprinter;
	
    delete MCwidgeteventmanager;
    
	delete MCsslcertificates;
	delete MCdefaultnetworkinterface;
	
#if defined(MCSSL) && !defined(_MOBILE)
	ShutdownSSL();
#endif

#if defined(_MOBILE)
    // SN-2015-02-24: [[ Merge 6.7.4-rc-1 ]] Need to clean-up the completed
    //  purchase list
    extern void MCPurchaseClearPurchaseList();
    
    MCPurchaseClearPurchaseList();
#endif

	MCS_shutdown();
	delete MCundos;
	while (MCcur_effects != NULL)
	{
		MCEffectList *veptr = MCcur_effects;
        MCcur_effects = MCcur_effects->getnext();
		delete veptr;
	}

	if (MCcurtheme != NULL)
		MCcurtheme -> unload();
	delete MCcurtheme;

	MCValueRelease(MCenvironmentvariables);
	MCenvironmentvariables = nil;

    // SN-2015-07-17: [[ CommandArguments ]] Clean up the memory
    MCValueRelease(MCcommandname);
    MCcommandname = NULL;
    MCValueRelease(MCcommandarguments);
    MCcommandarguments = NULL;

	// Cleanup the cursors array - *before* we close the screen!!
	if (MCModeMakeLocalWindows())
		for(uint32_t i = 0; i < PI_NCURSORS; i++)
		{
			MCscreen -> freecursor(MCcursors[i]);
			MCcursors[i] = nil;
		}

#ifdef _LINUX_DESKTOP
	// VALGRIND: Still causing a single invalid memory
	MCscreen->close(True);
#endif

	delete MCscreen;

	MCExternal::Cleanup();

	// Cleanup the MClicenseparameters block
    MCValueRelease(MClicenseparameters . license_token);
    MCValueRelease(MClicenseparameters . license_name);
    MCValueRelease(MClicenseparameters . license_organization);
	MCValueRelease(MClicenseparameters . addons);



	// Cleanup the startup stacks list
	for(uint4 i = 0; i < MCnstacks; ++i)
		MCValueRelease(MCstacknames[i]);

	delete[] MCstacknames; /* allocated with new[] */

	// Cleanup the parentscript stuff
	MCParentScript::Cleanup();
	
	// Finalize the event queue
	MCEventQueueFinalize();
	
	// MW-2012-02-23: [[ LogFonts ]] Finalize the font table module.
	MCLogicalFontTableFinalize();
	
	// MM-2013-09-03: [[ RefactorGraphics ]] Initialize graphics library.
	MCGraphicsFinalize();

#ifdef MCSSL
    MCSocketsFinalize();
#endif /* MCSSL */

#ifdef _ANDROID_MOBILE
    // MM-2012-02-22: Clean up any static variables as Android static vars are preserved between sessions
    MCAdFinalize();
    MCNativeControlFinalize();
    MCSensorFinalize();
    MCAndroidFinalizeBuildInfo();
    MCAndroidCustomFontsFinalize();
	MCSystemSoundFinalize();
#endif
	
#ifdef _IOS_MOBILE
	MCSystemSoundFinalize();
    MCReachabilityEventFinalize();
#endif
	
	MCDateTimeFinalize();
	
	MCU_finalize_names();
	
	if (MCsysencoding != nil)
		delete[] MCsysencoding; /* Allocated with new[] */

    if (kMCSystemLocale != nil)
        MCLocaleRelease(kMCSystemLocale);
    if (kMCBasicLocale != nil)
        MCLocaleRelease(kMCBasicLocale);
    
    if (MCappcodepath != nullptr)
        MCValueRelease(MCappcodepath);
    if (MCcmd != nullptr)
        MCValueRelease(MCcmd);
    
    MCValueRelease(MClogmessage);
    
	return MCretcode;
}

////////////////////////////////////////////////////////////////////////////////

void MCActionsDoRunSome(uint32_t p_mask)
{
    uint32_t t_actions;
    t_actions = MCactionsrequired & p_mask;
    
    if ((t_actions & kMCActionsDrainDeletedObjects) != 0)
    {
        MCactionsrequired &= ~kMCActionsDrainDeletedObjects;
        MCDeletedObjectsDoDrain();
    }
    
    if ((t_actions & kMCActionsUpdateScreen) != 0)
    {
        MCactionsrequired &= ~kMCActionsUpdateScreen;
        MCRedrawDoUpdateScreen();
    }
}

////////////////////////////////////////////////////////////////////////////////

struct MCHook
{
    MCHook *next;
    MCHookType type;
    void *descriptor;
};

bool MCHookRegister(MCHookType p_type, void *p_descriptor)
{
    MCHook *t_hook;
    if (!MCMemoryNew(t_hook))
        return false;
    
    t_hook -> next = MChooks;
    t_hook -> type = p_type;
    t_hook -> descriptor = p_descriptor;
    
    MChooks = t_hook;
    
    return true;
}

void MCHookUnregister(MCHookType p_type, void *p_descriptor)
{
    MCHook *t_hook, *t_previous;
    for(t_previous = nil, t_hook = MChooks; t_hook != nil; t_previous = t_hook, t_hook = t_hook -> next)
        if (t_hook -> type == p_type &&
            t_hook -> descriptor == p_descriptor)
            break;
    
    if (t_hook != nil)
    {
        if (t_previous != nil)
            t_previous -> next = t_hook -> next;
        else
            MChooks = t_hook -> next;
        
        MCMemoryDelete(t_hook);
    }
}

bool MCHookForEach(MCHookType p_type, MCHookForEachCallback p_callback, void *p_context)
{
    for(MCHook *t_hook = MChooks; t_hook != nil; t_hook = t_hook -> next)
        if (t_hook -> type == p_type)
            if (p_callback(p_context, t_hook -> descriptor))
                return true;
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

struct __MCIsGlobalHandlerContext
{
    MCNameRef name;
};

static bool __MCIsGlobalHandlerCallback(void *p_context, void *p_descriptor)
{
    __MCIsGlobalHandlerContext *ctxt;
    ctxt = (__MCIsGlobalHandlerContext *)p_context;
    
    MCHookGlobalHandlersDescriptor *t_desc;
    t_desc = (MCHookGlobalHandlersDescriptor *)p_descriptor;
    
    return t_desc -> can_handle(ctxt -> name);
}

bool MCIsGlobalHandler(MCNameRef message)
{
    __MCIsGlobalHandlerContext ctxt;
    ctxt . name = message;
    return MCHookForEach(kMCHookGlobalHandlers, __MCIsGlobalHandlerCallback, &ctxt);
}

//////////

struct __MCRunGlobalHandlerContext
{
    MCNameRef name;
    MCParameter *parameters;
    Exec_stat *result;
};

static bool __MCRunGlobalHandlerCallback(void *p_context, void *p_descriptor)
{
    __MCRunGlobalHandlerContext *ctxt;
    ctxt = (__MCRunGlobalHandlerContext *)p_context;
    
    MCHookGlobalHandlersDescriptor *t_desc;
    t_desc = (MCHookGlobalHandlersDescriptor *)p_descriptor;
 
    return t_desc -> handle(ctxt -> name, ctxt -> parameters, *(ctxt -> result));
}

bool MCRunGlobalHandler(MCNameRef message, MCParameter *parameters, Exec_stat& r_result)
{
    __MCRunGlobalHandlerContext ctxt;
    ctxt . name = message;
    ctxt . parameters = parameters;
    ctxt . result = &r_result;
    return MCHookForEach(kMCHookGlobalHandlers, __MCRunGlobalHandlerCallback, &ctxt);
}

////////////////////////////////////////////////////////////////////////////////

struct __MCLookupNativeControlContext
{
    MCStringRef name;
    intenum_t *type;
};

static bool __MCLookupNativeControlTypeCallback(void *p_context, void *p_descriptor)
{
    __MCLookupNativeControlContext *ctxt;
    ctxt = (__MCLookupNativeControlContext *)p_context;
    
    MCHookNativeControlsDescriptor *t_desc;
    t_desc = (MCHookNativeControlsDescriptor *)p_descriptor;
    
    return t_desc -> lookup_type(ctxt -> name, *(ctxt -> type));
}

bool MCLookupNativeControlType(MCStringRef p_name, intenum_t& r_type)
{
    __MCLookupNativeControlContext ctxt;
    ctxt . name = p_name;
    ctxt . type = &r_type;
    return MCHookForEach(kMCHookNativeControls, __MCLookupNativeControlTypeCallback, &ctxt);
}

////

static bool __MCLookupNativeControlPropertyCallback(void *p_context, void *p_descriptor)
{
    __MCLookupNativeControlContext *ctxt;
    ctxt = (__MCLookupNativeControlContext *)p_context;
    
    MCHookNativeControlsDescriptor *t_desc;
    t_desc = (MCHookNativeControlsDescriptor *)p_descriptor;
    
    return t_desc -> lookup_property(ctxt -> name, *(ctxt -> type));
}

bool MCLookupNativeControlProperty(MCStringRef p_name, intenum_t& r_type)
{
    __MCLookupNativeControlContext ctxt;
    ctxt . name = p_name;
    ctxt . type = &r_type;
    return MCHookForEach(kMCHookNativeControls, __MCLookupNativeControlPropertyCallback, &ctxt);
}

////

static bool __MCLookupNativeControlActionCallback(void *p_context, void *p_descriptor)
{
    __MCLookupNativeControlContext *ctxt;
    ctxt = (__MCLookupNativeControlContext *)p_context;
    
    MCHookNativeControlsDescriptor *t_desc;
    t_desc = (MCHookNativeControlsDescriptor *)p_descriptor;
    
    return t_desc -> lookup_action(ctxt -> name, *(ctxt -> type));
}

bool MCLookupNativeControlAction(MCStringRef p_name, intenum_t& r_type)
{
    __MCLookupNativeControlContext ctxt;
    ctxt . name = p_name;
    ctxt . type = &r_type;
    return MCHookForEach(kMCHookNativeControls, __MCLookupNativeControlActionCallback, &ctxt);
}

////

struct __MCCreateNativeControlContext
{
    intenum_t type;
    void **control;
};

static bool __MCCreateNativeControlCallback(void *p_context, void *p_descriptor)
{
    __MCCreateNativeControlContext *ctxt;
    ctxt = (__MCCreateNativeControlContext *)p_context;
    
    MCHookNativeControlsDescriptor *t_desc;
    t_desc = (MCHookNativeControlsDescriptor *)p_descriptor;
    
    return t_desc -> create(ctxt -> type, *(ctxt -> control));
}

bool MCCreateNativeControl(intenum_t p_type, void *& r_control)
{
    __MCCreateNativeControlContext ctxt;
    ctxt . type = p_type;
    ctxt . control = &r_control;
    return MCHookForEach(kMCHookNativeControls, __MCCreateNativeControlCallback, &ctxt);
}

////

struct __MCPerformNativeControlActionContext
{
    intenum_t action;
    void *control;
    MCValueRef *arguments;
    uindex_t argument_count;
};

static bool __MCPerformNativeControlActionCallback(void *p_context, void *p_descriptor)
{
    __MCPerformNativeControlActionContext *ctxt;
    ctxt = (__MCPerformNativeControlActionContext *)p_context;
    
    MCHookNativeControlsDescriptor *t_desc;
    t_desc = (MCHookNativeControlsDescriptor *)p_descriptor;
    
    if (t_desc -> action == nil)
        return false;
    
    return t_desc -> action(ctxt -> action, ctxt -> control, ctxt -> arguments, ctxt -> argument_count);
}

bool MCPerformNativeControlAction(intenum_t p_action, void *p_control, MCValueRef *p_arguments, uindex_t p_argument_count)
{
    __MCPerformNativeControlActionContext ctxt;
    ctxt . action = p_action;
    ctxt . control = p_control;
    ctxt . arguments = p_arguments;
    ctxt . argument_count = p_argument_count;
    return MCHookForEach(kMCHookNativeControls, __MCPerformNativeControlActionCallback, &ctxt);
}

////////////////////////////////////////////////////////////////////////////////

// MW-2013-10-08: [[ Bug 11259 ]] Make sure the Linux specific case tables are
//   in a global place so it works for server and desktop.
#if defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER) || defined(__EMSCRIPTEN__)
// MW-2013-10-01: [[ Bug 11160 ]] Use our own lowercasing table (ISO8859-1)
uint1 MClowercasingtable[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xd7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf, 
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, 
};

// MW-2013-10-01: [[ Bug 11160 ]] Use our own uppercasing table (ISO8859-1)
uint1 MCuppercasingtable[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
	0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xf7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xff, 
};

// MW-2013-10-01: [[ Bug 11160 ]] Use our own ctype table (ISO8859-1)
uint2 MCctypetable[] =
{
	0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0410, 0x0410, 0x0410, 0x0410, 0x0410, 0x0400, 0x0400, 
	0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 
	0x0110, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 
	0x03a8, 0x03a8, 0x03a8, 0x03a8, 0x03a8, 0x03a8, 0x03a8, 0x03a8, 0x03a8, 0x03a8, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 
	0x0340, 0x03a3, 0x03a3, 0x03a3, 0x03a3, 0x03a3, 0x03a3, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 
	0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 
	0x0340, 0x03a5, 0x03a5, 0x03a5, 0x03a5, 0x03a5, 0x03a5, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 
	0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0340, 0x0340, 0x0340, 0x0340, 0x0400, 
	0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 
	0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 
	0x0110, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 
	0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 
	0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 
	0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0340, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0383, 0x0385, 
	0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 
	0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0340, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 0x0385, 
};
#endif

////////////////////////////////////////////////////////////////////////////////
