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
// globals for MetaCard
//
#ifndef __MC_GLOBALS__
#define __MC_GLOBALS__

#include "mcstring.h"
#include "imagelist.h"

#include "foundation-locale.h"

typedef struct _Streamnode Streamnode;
typedef struct _Linkatts Linkatts;

extern Bool MCquit;

// MW-2013-04-01: [[ Bug 10799 ]] If this is true, it means we must do an 'exit' after the
//   event loop ends. This is to distinguish an implicit quit from, say, the home button on
//   iOS.
extern Bool MCquitisexplicit;

extern int MCidleRate;


extern Boolean MCaqua;
extern MCStringRef MCcmd;
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
extern Boolean MCfreescripts;
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
extern uint2 MCpaintcompression;
extern uint2 MCrecordformat;
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
extern uint2 MCwaitdepth;
extern uint4 MCrecursionlimit;


extern Boolean MCownselection;
extern MCUndolist *MCundos;
extern MCSellist *MCselected;
extern MCStacklist *MCstacks;
extern MCStacklist *MCtodestroy;
extern MCObject *MCtodelete;
extern MCCardlist *MCrecent;
extern MCCardlist *MCcstack;
extern MCDispatch *MCdispatcher;
extern MCStack *MCtopstackptr;
extern MCStack *MCdefaultstackptr;
extern MCStack *MCstaticdefaultstackptr;
extern MCStack *MCmousestackptr;
extern MCStack *MCclickstackptr;
extern MCStack *MCfocusedstackptr;
extern MCObject *MCtargetptr;
extern MCObject *MCmenuobjectptr;
extern MCCard *MCdynamiccard;
extern Boolean MCdynamicpath;
extern MCObject *MCerrorptr;
extern MCObject *MCerrorlockptr;
extern MCGroup *MCsavegroupptr;
extern MCGroup *MCdefaultmenubar;
extern MCGroup *MCmenubar;
extern MCAudioClip *MCacptr;
extern MCPlayer *MCplayers;

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

extern MCImage *MCmagimage;
extern MCMagnify *MCmagnifier;
extern MCObject *MCdragsource;
extern MCObject *MCdragdest;
extern MCField *MCactivefield;
extern MCField *MCclickfield;
extern MCField *MCfoundfield;
extern MCField *MCdropfield;
extern int4 MCdropchar;
extern MCImage *MCactiveimage;
extern MCImage *MCeditingimage;
extern MCTooltip *MCtooltip;
extern MCStack *MCmbstackptr;

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
extern uint2 MCtrylock;
extern uint2 MCerrorlock;
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
extern uint4 MCpattern;
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

#ifdef _LINUX_DESKTOP
extern Window MCgtkthemewindow;
#endif

#define RTB_ACCURATE_UNICODE_CONVERSIONS (1 << 0)
#define RTB_ACCURATE_UNICODE_INPUT (1 << 1)
#define RTB_NO_UNICODE_WINDOWS (1 << 2)
extern uint4 MCruntimebehaviour;

extern MCDragData *MCdragdata;
extern MCDragAction MCdragaction;
extern MCObject *MCdragtargetptr;
extern MCDragActionSet MCallowabledragactions;
extern uint4 MCdragimageid;
extern MCPoint MCdragimageoffset;

extern MCClipboardData *MCclipboarddata;
extern MCSelectionData *MCselectiondata;

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

// MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure only a single animation message is sent per redraw
extern MCThreadMutexRef MCanimationmutex;
extern MCThreadMutexRef MCpatternmutex;
extern MCThreadMutexRef MCimagerepmutex;
extern MCThreadMutexRef MCfieldmutex;
extern MCThreadMutexRef MCthememutex;
extern MCThreadMutexRef MCgraphicmutex;

///////////////////////////////////////////////////////////////////////////////

#endif
