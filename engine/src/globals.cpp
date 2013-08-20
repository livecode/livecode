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
#include "pxmaplst.h"
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

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

Bool MCquit;
Bool MCquitisexplicit;
int MCidleRate = 200;

Boolean MCaqua;
char *MCcmd;
MCStringRef MCfiletype;
MCStringRef MCstackfiletype;


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
MCStringRef MCdisplayname = NULL;
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
Pixmap MCpenpm;
uint4 MCbrushpmid = PI_PATTERNS;
Pixmap MCbrushpm;
MCPixmaplist *MCpatterns;
MCColor MCaccentcolor;
MCStringRef MCaccentcolorname;
MCColor MChilitecolor = { 0, 0, 0, 0x8080, 0, 0 };
MCStringRef MChilitecolorname;
MCColor MCselectioncolor;
MCStringRef MCselectioncolorname;
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
MCStringRef MCstackbottom;
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
MCVariable *MCurlresult;
Boolean MCexitall;
int4 MCretcode;
Boolean MCrecording;

Boolean MCantialiasedtextworkaround = False;

// MW-2012-03-08: [[ StackFile5500 ]] Make stackfile version 5.5 the default.
uint4 MCstackfileversion = 5500;

uint1 MCleftmasks[8] = {0xFF, 0x7F, 0x3f, 0x1F, 0x0F, 0x07, 0x03, 0x01};
uint1 MCrightmasks[8] = {0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF};

#if defined(_WINDOWS)
uint2 MClook = LF_WIN95;
MCStringRef MCttbgcolor = MCSTR("255,255,231");
MCStringRef MCttfont = MCSTR("MS Sans Serif");
uint2 MCttsize = 12;
#elif defined(_MACOSX)
uint2 MClook = LF_MAC;
StringRef MCttbgcolor = MCSTR("255,255,207");
MCStringRef MCttfont = MCSTR("Lucida Grande");
uint2 MCttsize = 11;
#else
uint2 MClook = LF_MOTIF;
MCStringRef MCttbgcolor = MCSTR("255,255,207");
MCStringRef MCttfont = MCSTR("Helvetica");
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
Boolean MClockmoves;
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

uint4 MCqtidlerate = 50;

Boolean MChidebackdrop = False;
Boolean MCraisewindows = False;

uint4 MCmajorosversion = 0;

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
	MCpenpm = nil;
	MCbrushpmid = PI_PATTERNS;
	MCbrushpm = nil;
	MCpatterns = nil;
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
	MCftpproxyhost = nil;
	MCftpproxyport = 0;
	MChttpproxy = nil;
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
	
	MCresult = nil;
	MCurlresult = nil;
	MCexitall = False;
	MCretcode = 0;
	MCrecording = False;
	MCantialiasedtextworkaround = False;
	// MW-2012-03-08: [[ StackFile5500 ]] Make 5.5 stackfile version the default.
	MCstackfileversion = 5500;
	MClook = LF_MOTIF;
	MCttbgcolor = MCSTR("255,255,207");
	MCttfont = MCSTR("Helvetica");
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
	MClockmoves = False;
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
	for(uindex_t i = 0; i < PATTERN_CACHE_SIZE; i++)
	{
		MCregexpatterns[i] = nil;
		MCregexcache[i] = nil;
	}

	for(uint32_t i = 0; i < PI_NCURSORS; i++)
		MCcursors[i] = nil;
    
	// MM-2012-09-05: [[ Property Listener ]]
	MCobjectpropertieschanged = False;
	MCpropertylistenerthrottletime = 250;
    
	// MW-2013-03-20: [[ MainStacksChanged ]]
	MCmainstackschanged = False;

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

	MCpatterns = new MCPixmaplist;

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
	//MCvcplayer = strclone("xanim");
	MCvplayer = MCSTR("xanim");
#else
	MCvcplayer = MCSTR("");
#endif

	MCfiletype = MCSTR("ttxtTEXT");
	const char *tname = strrchr(MCcmd, PATH_SEPARATOR);
	if (tname == NULL)
		tname = MCcmd;
	else
		tname++;
	if (MCU_strncasecmp(tname, "rev", 3))
		MCstackfiletype = MCSTR("MCRDMSTK");
	else
		MCstackfiletype = MCSTR("RevoRSTK");
	MCserialcontrolsettings = MCSTR("baud=9600 parity=N data=8 stop=1");

	MCdispatcher = new MCDispatch;

	if (MCnoui)
		MCscreen = new MCUIDC;
	else
	{
		MCscreen = MCCreateScreenDC();
				
		if (!MCscreen->open())
			return false;

		MCscreen->alloccolor(MClinkatts.color);
		MCscreen->alloccolor(MClinkatts.hilitecolor);
		MCscreen->alloccolor(MClinkatts.visitedcolor);
	}
	
	MCExecPoint ep;
	MCExecContext ctxt(ep);
	MCInterfaceInitialize(ctxt);
	
	// MW-2012-02-14: [[ FontRefs ]] Open the dispatcher after we have an open
	//   screen, otherwise we don't have a root fontref!
	MCdispatcher -> setfontattrs(DEFAULT_TEXT_FONT, DEFAULT_TEXT_SIZE, FA_DEFAULT_STYLE);
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

	MCExecPoint ep;
	MCExecContext ctxt(ep);
	MCInterfaceFinalize(ctxt);

	MCstacks->closeall();
	MCselected->clear(False);

	MCU_play_stop();
	if (MCrecording)
		MCtemplateplayer->stoprecording();
	MClockmessages = True;
	MCS_killall();

	MCscreen -> flushclipboard();

	while (MCtodelete != NULL)
	{
		MCObject *optr = MCtodelete->remove(MCtodelete);
		delete optr;
	}

	delete MCtooltip;
	MCtooltip = NULL;

	MCValueRelease(MChttpproxy);
	MCValueRelease(MCpencolorname);
	MCValueRelease(MCbrushcolorname);
	MCValueRelease(MChilitecolorname);
	MCValueRelease(MCaccentcolorname);
	MCValueRelease(MCselectioncolorname);

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
	delete MCpatterns;
	delete MCresult;
	delete MCurlresult;
	delete MCdialogdata;
	MCValueRelease(MChcstat);

	delete MCusing;
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
	uint2 i;
	for (i = 0 ; i < PATTERN_CACHE_SIZE ; i++)
	{
        MCValueRelease(MCregexpatterns[i]);
		MCR_free(MCregexcache[i]);
	}
	delete MCperror;
	delete MCeerror;

	delete MCclipboarddata;
	delete MCdragdata;
	delete MCselectiondata;

	MCValueRelease(MCshellcmd);
	MCValueRelease(MCvcplayer);
	MCValueRelease(MCfiletype);
	MCValueRelease(MCstackfiletype);
	MCValueRelease(MCserialcontrolsettings);
	
	MCprinter -> Finalize();
	delete MCprinter;
	
	delete MCsslcertificates;
	delete MCdefaultnetworkinterface;
	
#ifndef _MOBILE
	ShutdownSSL();
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
	MCValueRelease(MClicenseparameters . addons);

	// Cleanup the startup stacks list
	for(uint4 i = 0; i < MCnstacks; ++i)
		MCValueRelease(MCstacknames[i]);

	// Cleanup the parentscript stuff
	MCParentScript::Cleanup();
	
	// MW-2012-02-23: [[ LogFonts ]] Finalize the font table module.
	MCLogicalFontTableFinalize();
	// MW-2012-02-23: [[ FontRefs ]] Finalize the font module.
	MCFontFinalize();
    
#ifdef _ANDROID_MOBILE
    // MM-2012-02-22: Clean up any static variables as Android static vars are preserved between sessions
    MCAdFinalize();
    MCNativeControlFinalize();
    MCSensorFinalize();
    MCAndroidCustomFontsFinalize();
#endif
	
	MCU_finalize_names();

	return MCretcode;
}
