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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "execpt.h"
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

#include "systhreads.h"
#include "stacktile.h"

#define HOLD_SIZE1 65535
#define HOLD_SIZE2 16384

#ifdef TARGET_PLATFORM_MACOS_X
#include <Foundation/NSAutoreleasePool.h>
#endif

#ifdef _ANDROID_MOBILE
#include "mblad.h"
#include "mblcontrol.h"
#include "mblsensor.h"
#endif

////////////////////////////////////////////////////////////////////////////////

Bool MCquit;
Bool MCquitisexplicit;
int MCidleRate = 200;

Boolean MCaqua;
char *MCcmd;
char *MCfiletype;
char *MCstackfiletype;


#ifdef TARGET_PLATFORM_LINUX
MCXImageCache *MCimagecache ;

Boolean MCutf8 ;
Boolean MCXVideo ;

Window MClastvideowindow = DNULL ;
#endif

Boolean MCuseXft = True;
Boolean MCuselibgnome = False ;
Boolean MCuseESD = False ;

char **MCstacknames = NULL;
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
Boolean MCfreescripts = True;
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
char *MCpencolorname;
MCColor MCbrushcolor;
char *MCbrushcolorname;
uint4 MCpenpmid = PI_PATTERNS;
MCPatternRef MCpenpattern;
uint4 MCbrushpmid = PI_PATTERNS;
MCPatternRef MCbrushpattern;
uint4 MCbackdroppmid;
MCPatternRef MCbackdroppattern;
MCImageList *MCpatternlist;
MCColor MCaccentcolor;
char *MCaccentcolorname;
MCColor MChilitecolor = { 0, 0, 0, 0x8080, 0, 0 };
char *MChilitecolorname;
MCColor MCselectioncolor;
char *MCselectioncolorname;
Linkatts MClinkatts = { { 0, 0, 0, 0xEFBE, 0, 0 }, NULL,
                        { 0, 0xFFFF, 0, 0, 0, 0 }, NULL,
                        { 0, 0x5144, 0x1861, 0x8038, 0, 0 }, NULL, True };
Boolean MCrelayergrouped;
Boolean MCselectgrouped;
Boolean MCselectintersect = True;
MCRectangle MCwbr;
uint2 MCjpegquality = 100;
uint2 MCpaintcompression = EX_PBM;
uint2 MCrecordformat = EX_AIFF;
uint2 MCrecordchannels = 1;
uint2 MCrecordsamplesize = 8;
real8 MCrecordrate = 22.050;
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
uint2 MCwaitdepth;
uint4 MCrecursionlimit = 400000; // actual max is about 480K on OSX

MCClipboardData *MCclipboarddata;
MCSelectionData *MCselectiondata;

MCDragData *MCdragdata;
MCDragAction MCdragaction;
MCDragActionSet MCallowabledragactions;
uint4 MCdragimageid;
MCPoint MCdragimageoffset;
MCObject *MCdragtargetptr;
uint2 MCdragdelta = 4;

MCUndolist *MCundos;
MCSellist *MCselected;
MCStacklist *MCstacks;
MCStacklist *MCtodestroy;
MCObject *MCtodelete;
MCCardlist *MCrecent;
MCCardlist *MCcstack;
MCDispatch *MCdispatcher;
MCStack *MCtopstackptr;
MCStack *MCdefaultstackptr;
MCStack *MCstaticdefaultstackptr;
MCStack *MCmousestackptr;
MCStack *MCclickstackptr;
MCStack *MCfocusedstackptr;
MCCard *MCdynamiccard;
Boolean MCdynamicpath;
MCObject *MCerrorptr;
MCObject *MCerrorlockptr;
MCObject *MCtargetptr;
MCObject *MCmenuobjectptr;
MCGroup *MCsavegroupptr;
MCGroup *MCdefaultmenubar;
MCGroup *MCmenubar;
MCPlayer *MCplayers;
MCAudioClip *MCacptr;

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

MCImage *MCmagimage;
MCMagnify *MCmagnifier;
MCObject *MCdragsource;
MCObject *MCdragdest;
MCField *MCactivefield;
MCField *MCclickfield;
MCField *MCfoundfield;
MCField *MCdropfield;
int4 MCdropchar;
MCImage *MCactiveimage;
MCImage *MCeditingimage;
MCTooltip *MCtooltip;

MCUIDC *MCscreen;
MCPrinter *MCprinter;
MCPrinter *MCsystemprinter;

char *MCscriptfont;
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
char *MCserialcontrolsettings;
char *MCshellcmd;
char *MCvcplayer;
char *MCbackdropcolor;

char *MCftpproxyhost;
uint2 MCftpproxyport;

char *MChttpproxy;

char *MClongdateformat;
char *MCshortdateformat;
char *MChttpheaders;
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
char *MChcstat;
char *MCcurdir;
MCVariable *MCresult;
MCVariable *MCurlresult;
Boolean MCexitall;
int4 MCretcode;
Boolean MCrecording;
#ifdef FEATURE_PLATFORM_RECORDER
MCPlatformSoundRecorderRef MCrecorder;
#endif

// MW-2012-03-08: [[ StackFile5500 ]] Make stackfile version 5.5 the default.
uint4 MCstackfileversion = 5500;

#if defined(_WINDOWS)
uint2 MClook = LF_WIN95;
const char *MCttbgcolor = "255,255,231";
const char *MCttfont = "MS Sans Serif";
uint2 MCttsize = 12;
#elif defined(_MACOSX)
uint2 MClook = LF_MAC;
const char *MCttbgcolor = "255,255,207";
const char *MCttfont = "Lucida Grande";
uint2 MCttsize = 11;
#else
uint2 MClook = LF_MOTIF;
const char *MCttbgcolor = "255,255,207";
const char *MCttfont = "Helvetica";
uint2 MCttsize = 12;
#endif
uint2 MCtrylock;
uint2 MCerrorlock;
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
uint4 MCpattern = 1;
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

uint4 MCiconid = 0;
char *MCiconmenu = NULL;
uint4 MCstatusiconid = 0;
char *MCstatusiconmenu = NULL;
char *MCstatusicontooltip = NULL;

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

uint32_t MCstacklimit = 1024 * 1024;
uint32_t MCpendingstacklimit = 1024 * 1024;

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

// MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure only a single animation message is sent per redraw
MCThreadMutexRef MCanimationmutex = NULL;
MCThreadMutexRef MCpatternmutex = NULL;
MCThreadMutexRef MCimagerepmutex = NULL;
MCThreadMutexRef MCfieldmutex = NULL;
MCThreadMutexRef MCthememutex = NULL;
MCThreadMutexRef MCgraphicmutex = NULL;

////////////////////////////////////////////////////////////////////////////////

extern MCUIDC *MCCreateScreenDC(void);
extern void MCU_finalize_names();

#ifdef _ANDROID_MOBILE
void MCAndroidCustomFontsInitialize();
void MCAndroidCustomFontsFinalize();
#endif
	
// Reset all global variables to their on-load defaults. This is required on
// Android, as the shared library the engine is compiled as is not reloaded on
// every app start.
void X_clear_globals(void)
{
	MCquit = False;
	MCquitisexplicit = False;
	MCidleRate = 200;
	MCcmd = nil;
	MCfiletype = nil;
	MCstackfiletype = nil;
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
	MChidepalettes = False;
	MCdontuseNS = False;
	MCdontuseQT = False;
	MCdontuseQTeffects = False;
	MCfreescripts = True;
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
	MCdisplayname = nil;
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
	MCrecordformat = EX_AIFF;
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
	MCwaitdepth = 0;
	MCrecursionlimit = 400000;
	MCclipboarddata = nil;
	MCselectiondata = nil;
	MCdragdata = nil;
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
	MCtodelete = nil;
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
	MCtargetptr = nil;
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
	MCscriptfont = nil;
	MCscriptsize = 0;
	MCscrollbarwidth = DEFAULT_SB_WIDTH;
	uint2 MCfocuswidth = 2;
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
	MCserialcontrolsettings = nil;
	MCshellcmd = nil;
	MCvcplayer = nil;
	MCbackdropcolor = nil;
	MCftpproxyhost = nil;
	MCftpproxyport = 0;
	MChttpproxy = nil;
	MClongdateformat = nil;
	MCshortdateformat = nil;
	MChttpheaders = nil;
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
	MCcurdir = nil;
	MCresult = nil;
	MCurlresult = nil;
	MCexitall = False;
	MCretcode = 0;
	MCrecording = False;
#ifdef FEATURE_PLATFORM_RECORDER
    MCrecorder = nil;
#endif
	// MW-2012-03-08: [[ StackFile5500 ]] Make 5.5 stackfile version the default.
	MCstackfileversion = 5500;
	MClook = LF_MOTIF;
	MCttbgcolor = "255,255,207";
	MCttfont = "Helvetica";
	MCttsize = 12;
	MCtrylock = 0;
	MCerrorlock = 0;
	MCwatchcursor = False;
	MClockcursor = False;
	MCcursor = nil;
	MCcursorid = nil;
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
	MCpattern = 1;
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
    
    // MM-2014-07-31: [[ ThreadedRendering ]]
    MCanimationmutex = NULL;
    MCpatternmutex = NULL;
    MCimagerepmutex = NULL;
    MCfieldmutex = NULL;
    MCthememutex = NULL;
    MCgraphicmutex = NULL;

#ifdef _ANDROID_MOBILE
    // MM-2012-02-22: Initialize up any static variables as Android static vars are preserved between sessions
    MCAdInitialize();
    MCNativeControlInitialize();
    MCSensorInitialize();
    MCAndroidCustomFontsInitialize();
#endif
}

bool X_open(int argc, char *argv[], char *envp[])
{
	MCperror = new MCError();
	MCeerror = new MCError();
	/* UNCHECKED */ MCVariable::createwithname_cstring("MCresult", MCresult);

	/* UNCHECKED */ MCVariable::createwithname_cstring("MCurlresult", MCurlresult);
	/* UNCHECKED */ MCVariable::createwithname_cstring("MCdialogdata", MCdialogdata);
	
	////

	MCU_init();
	
	////
    
    // MW-2014-04-15: [[ Bug 12185 ]] Initialize graphics and openssl before anything
    //   that might use them.
    
	// MM-2013-09-03: [[ RefactorGraphics ]] Initialize graphics library.
	MCGraphicsInitialize();
	
	// MM-2014-02-14: [[ LibOpenSSL 1.0.1e ]] Initialise the openlSSL module.
	InitialiseSSL();
    
    // MM-2014-07-31: [[ ThreadedRendering ]]
    /* UNCHECKED */ MCThreadPoolInitialize();
    /* UNCHECKED */ MCStackTileInitialize();
    /* UNCHECKED */ MCThreadMutexCreate(MCanimationmutex);
    /* UNCHECKED */ MCThreadMutexCreate(MCpatternmutex);
    /* UNCHECKED */ MCThreadMutexCreate(MCimagerepmutex);
    /* UNCHECKED */ MCThreadMutexCreate(MCfieldmutex);
    /* UNCHECKED */ MCThreadMutexCreate(MCthememutex);
    /* UNCHECKED */ MCThreadMutexCreate(MCgraphicmutex);
    
    ////
    
#ifdef _MACOSX
    // MW-2014-07-21: Make AVFoundation the default on 10.8 and above.
    if (MCmajorosversion >= 0x1080)
        MCdontuseQT = True;
#endif
    
    ////
    
	MCpatternlist = new MCImageList();

	/* UNCHECKED */ MCVariable::ensureglobal(MCN_msg, MCmb);
	MCmb -> setmsg();

	/* UNCHECKED */ MCVariable::ensureglobal(MCN_each, MCeach);

	if (envp != nil)
		for (uint32_t i = 0 ; envp[i] != NULL ; i++)
		{
			char *sptr = envp[i];
			if (isupper((uint1)sptr[0]))
			{
				sptr = strchr(sptr, '=');
				uint2 length = sptr - envp[i];
				char *vname = new char[length + 2];
				vname[0] = '$';
				strncpy(&vname[1], envp[i], length);
				vname[length + 1] = '\0';

				MCVariable *tvar;
				/* UNCHECKED */ MCVariable::ensureglobal_cstring(vname, tvar);
				if (strequal(vname, "$MCNOFILES") && *(sptr + 1) != '0')
				{
					MCnofiles = True;
					MCsecuremode = MC_SECUREMODE_ALL;
				}
				tvar->copysvalue(sptr + 1);

				delete vname;
			}
		}

	/* UNCHECKED */ MCStackSecurityCreateStack(MCtemplatestack);
	MCtemplateaudio = new MCAudioClip;
	MCtemplateaudio->init();
	MCtemplatevideo = new MCVideoClip;
	MCtemplategroup = new MCGroup;
	MCtemplatecard = new MCCard;
	MCtemplatebutton = new MCButton;
	MCtemplategraphic = new MCGraphic;
	MCtemplatescrollbar = new MCScrollbar;
	MCtemplateplayer = new MCPlayer;
	MCtemplateimage = new MCImage;
	MCtemplatefield = new MCField;
	
	MCtooltip = new MCTooltip;

	MCclipboarddata = new MCClipboardData;
	MCdragdata = new MCDragData;
	MCselectiondata = new MCSelectionData;
	
	MCundos = new MCUndolist;
	MCselected = new MCSellist;
	MCstacks = new MCStacklist;
	MCtodestroy = new MCStacklist;
	MCrecent = new MCCardlist;
	MCcstack = new MCCardlist;

#ifdef _LINUX_DESKTOP
	MCvcplayer = strclone("xanim");
#else
	MCvcplayer = strclone("");
#endif

	MCfiletype = strclone("ttxtTEXT");
	const char *tname = strrchr(MCcmd, PATH_SEPARATOR);
	if (tname == NULL)
		tname = MCcmd;
	else
		tname++;
	if (MCU_strncasecmp(tname, "rev", 3))
		MCstackfiletype = strclone("MCRDMSTK");
	else
		MCstackfiletype = strclone("RevoRSTK");
	MCserialcontrolsettings = strclone("baud=9600 parity=N data=8 stop=1");

	MCdispatcher = new MCDispatch;
    MCdispatcher -> add_transient_stack(MCtooltip);

	// IM-2014-08-14: [[ Bug 12372 ]] Pixel scale setup needs to happen before the
	// creation of MCscreen to ensure screen rects are scaled/unscaled as appropriate.
	// IM-2014-01-27: [[ HiDPI ]] Initialize pixel scale settings
	MCResInitPixelScaling();
	
	if (MCnoui)
		MCscreen = new MCUIDC;
	else
	{
		MCscreen = MCCreateScreenDC();
				
		if (!MCscreen->open())
			return False;

		MCscreen->alloccolor(MClinkatts.color);
		MCscreen->alloccolor(MClinkatts.hilitecolor);
		MCscreen->alloccolor(MClinkatts.visitedcolor);
	}
	
	// MW-2012-02-14: [[ FontRefs ]] Open the dispatcher after we have an open
	//   screen, otherwise we don't have a root fontref!
	// MW-2013-08-07: [[ Bug 10995 ]] Configure fonts based on platform.
#if defined(TARGET_PLATFORM_WINDOWS)
	if (MCmajorosversion >= 0x0600)
	{
		// Vista onwards
		MCdispatcher -> setfontattrs("Segoe UI", 12, FA_DEFAULT_STYLE);
	}
	else
	{
		// Pre-Vista
		MCdispatcher -> setfontattrs("Tahoma", 11, FA_DEFAULT_STYLE);
	}
#elif defined(TARGET_PLATFORM_MACOS_X)
    if (MCmajorosversion < 0x10A0)
        MCdispatcher -> setfontattrs("Lucida Grande", 11, FA_DEFAULT_STYLE);
    else
    {
        MCdispatcher -> setfontattrs("Helvetica Neue", 11, FA_DEFAULT_STYLE);
        MCttfont = "Helvetica Neue";
    }
#elif defined(TARGET_PLATFORM_LINUX)
	MCdispatcher -> setfontattrs("Helvetica", 12, FA_DEFAULT_STYLE);
#else
	MCdispatcher -> setfontattrs(DEFAULT_TEXT_FONT, DEFAULT_TEXT_SIZE, FA_DEFAULT_STYLE);
#endif
	MCdispatcher -> open();

	// This is here because it relies on MCscreen being initialized.
	MCtemplateeps = new MCEPS;

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
			delete newtheme;
	}

	MCsystemprinter = MCprinter = MCscreen -> createprinter();
	MCprinter -> Initialize();
	
	// MW-2009-07-02: Clear the result as a startup failure will be indicated
	//   there.
	MCresult -> clear();
	if (MCdispatcher->startup() != IO_NORMAL)
		return false;

	return true;
}

int X_close(void)
{
	// MW-2008-01-18: [[ Bug 5711 ]] Make sure we disable the backdrop here otherwise we
	//   get crashiness on Windows due to hiding the backdrop calling WindowProc which
	//   attempts to access stacks that have been deleted...
	if (!MCnoui)
	{
		MCscreen -> disablebackdrop(false);
		MCscreen -> disablebackdrop(true);
	}

	MCstacks->closeall();
	MCselected->clear(False);

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

	MCscreen -> flushclipboard();

	while (MCtodelete != NULL)
	{
		MCObject *optr = MCtodelete->remove(MCtodelete);
		delete optr;
	}

    MCdispatcher -> remove_transient_stack(MCtooltip);
	delete MCtooltip;
	MCtooltip = NULL;

	delete MChttpproxy;

	delete MCbackdropcolor;
	delete MCpencolorname;
	delete MCbrushcolorname;
	delete MChilitecolorname;
	delete MCaccentcolorname;
	delete MCselectioncolorname;
	delete MClongdateformat;
	delete MCshortdateformat;

	while (MCnfiles)
		IO_closefile(MCfiles[0].name);
	if (MCfiles != NULL)
		delete MCfiles;
	if (MCprocesses != NULL)
		delete MCprocesses;
	if (MCnsockets)
		while (MCnsockets)
			delete MCsockets[--MCnsockets];
	if (MCsockets != NULL)
		delete MCsockets;

	while (MCsavegroupptr != NULL)
	{
		MCControl *gptr = MCsavegroupptr->remove(MCsavegroupptr);
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
	delete MCcstack;
	delete MCrecent;
	delete IO_stdin;
	delete IO_stdout;
	delete IO_stderr;
	delete MCpatternlist;
	delete MCresult;
	delete MCurlresult;
	delete MCdialogdata;
	delete MChcstat;
	delete MCcurdir;
	delete MCusing;
	delete MChttpheaders;
	delete MCscriptfont;
	delete MClinkatts.colorname;
	delete MClinkatts.hilitecolorname;
	delete MClinkatts.visitedcolorname;
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

	delete MCclipboarddata;
	delete MCdragdata;
	delete MCselectiondata;

	delete MCshellcmd;
	delete MCvcplayer;
	delete MCfiletype;
	delete MCstackfiletype;
	delete MCserialcontrolsettings;
	
	MCprinter -> Finalize();
	delete MCprinter;
	
	delete MCsslcertificates;
	delete MCdefaultnetworkinterface;
	
	ShutdownSSL();
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

	delete MCiconmenu;
	delete MCstatusiconmenu;
	delete MCstatusicontooltip;

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
	delete MClicenseparameters . license_token;
	delete MClicenseparameters . license_name;
	delete MClicenseparameters . license_organization;
	delete MClicenseparameters . addons;

	// Cleanup the startup stacks list
	delete MCstacknames;

	// Cleanup the parentscript stuff
	MCParentScript::Cleanup();
	
	// MW-2012-02-23: [[ LogFonts ]] Finalize the font table module.
	MCLogicalFontTableFinalize();
	// MW-2012-02-23: [[ FontRefs ]] Finalize the font module.
	MCFontFinalize();
	
	MCU_finalize_names();
	MCNameFinalize();
	
	// MM-2013-09-03: [[ RefactorGraphics ]] Initialize graphics library.
	MCGraphicsFinalize();
    
    // MM-2014-07-31: [[ ThreadedRendering ]]
    MCThreadPoolFinalize();
    MCStackTileFinalize();
    MCThreadMutexRelease(MCanimationmutex);
    MCThreadMutexRelease(MCpatternmutex);
    MCThreadMutexRelease(MCimagerepmutex);
    MCThreadMutexRelease(MCfieldmutex);
    MCThreadMutexRelease(MCthememutex);
    MCThreadMutexRelease(MCgraphicmutex);
    
#ifdef _ANDROID_MOBILE
    // MM-2012-02-22: Clean up any static variables as Android static vars are preserved between sessions
    MCAdFinalize();
    MCNativeControlFinalize();
    MCSensorFinalize();
    MCAndroidCustomFontsFinalize();
#endif
	
	return MCretcode;
}

// MW-2013-10-08: [[ Bug 11259 ]] Make sure the Linux specific case tables are
//   in a global place so it works for server and desktop.
#if defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER)
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
