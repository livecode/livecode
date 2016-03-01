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

//#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "undolst.h"
#include "card.h"
#include "aclip.h"
#include "vclip.h"
#include "mccontrol.h"
#include "image.h"
#include "button.h"
#include "mcerror.h"
#include "handler.h"
#include "hndlrlst.h"
#include "player.h"
#include "param.h"
#include "debug.h"
#include "util.h"
#include "date.h"
#include "parentscript.h"
#include "group.h"
#include "eventqueue.h"
#include "securemode.h"
#include "osspec.h"
#include "region.h"
#include "redraw.h"
#include "globals.h"
#include "license.h"
#include "mode.h"
#include "tilecache.h"
#include "font.h"
#include "external.h"

#include "exec.h"

#ifdef _MOBILE
#include "mbldc.h"
#endif

#include "resolution.h"

static int4 s_last_stack_time = 0;
static int4 s_last_stack_index = 0;

uint2 MCStack::ibeam;

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCStack::kProperties[] =
{
	DEFINE_RW_OBJ_PROPERTY(P_FULLSCREEN, Bool, MCStack, Fullscreen)
	DEFINE_RO_OBJ_PROPERTY(P_NUMBER, UInt16, MCStack, Number)
	DEFINE_RO_OBJ_PROPERTY(P_LAYER, Int16, MCStack, Layer)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_FILE_NAME, OptionalString, MCStack, FileName)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_FILE_NAME, OptionalString, MCStack, FileName)
	DEFINE_RW_OBJ_PROPERTY(P_SAVE_COMPRESSED, Bool, MCStack, SaveCompressed)
	DEFINE_RW_OBJ_PROPERTY(P_CANT_ABORT, Bool, MCStack, CantAbort)
	DEFINE_RW_OBJ_PROPERTY(P_CANT_DELETE, Bool, MCStack, CantDelete)
	DEFINE_RW_OBJ_ENUM_PROPERTY(P_STYLE, InterfaceStackStyle, MCStack, Style)
	DEFINE_RW_OBJ_PROPERTY(P_CANT_MODIFY, Bool, MCStack, CantModify)
	DEFINE_RW_OBJ_PROPERTY(P_CANT_PEEK, Bool, MCStack, CantPeek)
	DEFINE_RW_OBJ_PROPERTY(P_DESTROY_STACK, Bool, MCStack, DestroyStack)
	DEFINE_RW_OBJ_PROPERTY(P_DESTROY_WINDOW, Bool, MCStack, DestroyWindow)
    DEFINE_RW_OBJ_PROPERTY(P_DYNAMIC_PATHS, Bool, MCStack, DynamicPaths)
	DEFINE_RW_OBJ_PROPERTY(P_ALWAYS_BUFFER, Bool, MCStack, AlwaysBuffer)
	DEFINE_RW_OBJ_PROPERTY(P_LABEL, String, MCStack, Label)
	DEFINE_RW_OBJ_PROPERTY(P_UNICODE_LABEL, BinaryString, MCStack, UnicodeLabel)

	DEFINE_RW_OBJ_PROPERTY(P_CLOSE_BOX, Bool, MCStack, CloseBox)
	DEFINE_RW_OBJ_PROPERTY(P_ZOOM_BOX, Bool, MCStack, ZoomBox)
	DEFINE_RW_OBJ_PROPERTY(P_DRAGGABLE, Bool, MCStack, Draggable)
	DEFINE_RW_OBJ_PROPERTY(P_COLLAPSE_BOX, Bool, MCStack, CollapseBox)
	DEFINE_RW_OBJ_PROPERTY(P_LIVE_RESIZING, Bool, MCStack, LiveResizing)
	DEFINE_RW_OBJ_PROPERTY(P_SYSTEM_WINDOW, Bool, MCStack, SystemWindow)
	DEFINE_RW_OBJ_PROPERTY(P_METAL, Bool, MCStack, Metal)
	DEFINE_RW_OBJ_PROPERTY(P_SHADOW, Bool, MCStack, WindowShadow)
	DEFINE_RW_OBJ_PROPERTY(P_RESIZABLE, Bool, MCStack, Resizable)

    // AL-2014-05-26: [[ Bug 12510 ]] Stack decoration synonyms not implemented in 7.0
    DEFINE_RW_OBJ_PROPERTY(P_MAXIMIZE_BOX, Bool, MCStack, ZoomBox)
	DEFINE_RW_OBJ_PROPERTY(P_MINIMIZE_BOX, Bool, MCStack, CollapseBox)
    
	DEFINE_RW_OBJ_PROPERTY(P_MIN_WIDTH, UInt16, MCStack, MinWidth)
	DEFINE_RW_OBJ_PROPERTY(P_MAX_WIDTH, UInt16, MCStack, MaxWidth)
	DEFINE_RW_OBJ_PROPERTY(P_MIN_HEIGHT, UInt16, MCStack, MinHeight)
	DEFINE_RW_OBJ_PROPERTY(P_MAX_HEIGHT, UInt16, MCStack, MaxHeight)

	DEFINE_RO_OBJ_PROPERTY(P_RECENT_NAMES, String, MCStack, RecentNames)
	DEFINE_RO_OBJ_PROPERTY(P_RECENT_CARDS, String, MCStack, RecentCards)

	DEFINE_RW_OBJ_PROPERTY(P_ICONIC, Bool, MCStack, Iconic)
	DEFINE_RW_OBJ_PROPERTY(P_START_UP_ICONIC, Bool, MCStack, StartUpIconic)
	DEFINE_RW_OBJ_PROPERTY(P_ICON, UInt16, MCStack, Icon)

	DEFINE_RO_OBJ_PROPERTY(P_OWNER, OptionalString, MCStack, Owner)
	DEFINE_RW_OBJ_PROPERTY(P_MAIN_STACK, String, MCStack, MainStack)
	DEFINE_RW_OBJ_PROPERTY(P_SUBSTACKS, OptionalString, MCStack, Substacks)

	DEFINE_RO_OBJ_PROPERTY(P_BACKGROUND_NAMES, String, MCStack, BackgroundNames)
	DEFINE_RO_OBJ_PROPERTY(P_BACKGROUND_IDS, String, MCStack, BackgroundIds)
	DEFINE_RO_OBJ_PROPERTY(P_SHARED_GROUP_NAMES, String, MCStack, SharedGroupNames)
	DEFINE_RO_OBJ_PROPERTY(P_SHARED_GROUP_IDS, String, MCStack, SharedGroupIds)
	DEFINE_RO_OBJ_LIST_PROPERTY(P_CARD_IDS, LinesOfLooseUInt, MCStack, CardIds)
	DEFINE_RO_OBJ_LIST_PROPERTY(P_CARD_NAMES, LinesOfString, MCStack, CardNames)

	DEFINE_RW_OBJ_PROPERTY(P_EDIT_BACKGROUND, Bool, MCStack, EditBackground)
	DEFINE_RW_OBJ_PROPERTY(P_EXTERNALS, String, MCStack, Externals)
	DEFINE_RO_OBJ_PROPERTY(P_EXTERNAL_COMMANDS, OptionalString, MCStack, ExternalCommands)
	DEFINE_RO_OBJ_PROPERTY(P_EXTERNAL_FUNCTIONS, OptionalString, MCStack, ExternalFunctions)
	DEFINE_RO_OBJ_PROPERTY(P_EXTERNAL_PACKAGES, OptionalString, MCStack, ExternalPackages)

	DEFINE_RO_OBJ_PROPERTY(P_MODE, Int16, MCStack, Mode)
	DEFINE_RW_OBJ_PROPERTY(P_WM_PLACE, Bool, MCStack, WmPlace)
	DEFINE_RO_OBJ_PROPERTY(P_WINDOW_ID, UInt16, MCStack, WindowId)
	DEFINE_RO_OBJ_PROPERTY(P_PIXMAP_ID, UInt16, MCStack, PixmapId)
	DEFINE_RW_OBJ_PROPERTY(P_HC_ADDRESSING, Bool, MCStack, HcAddressing)
	DEFINE_RO_OBJ_PROPERTY(P_HC_STACK, Bool, MCStack, HcStack)
	DEFINE_RO_OBJ_PROPERTY(P_SIZE, UInt16, MCStack, Size)
	DEFINE_RO_OBJ_PROPERTY(P_FREE_SIZE, UInt16, MCStack, FreeSize)
	DEFINE_RW_OBJ_PROPERTY(P_LOCK_SCREEN, Bool, MCStack, LockScreen)

	DEFINE_RW_OBJ_PROPERTY(P_STACK_FILES, String, MCStack, StackFiles)
	DEFINE_RW_OBJ_PROPERTY(P_MENU_BAR, String, MCStack, MenuBar)
	DEFINE_RW_OBJ_PROPERTY(P_EDIT_MENUS, Bool, MCStack, EditMenus)
	DEFINE_RO_OBJ_PROPERTY(P_VSCROLL, Int32, MCStack, VScroll)
	DEFINE_RO_OBJ_ENUM_PROPERTY(P_CHARSET, InterfaceCharset, MCStack, Charset)
	DEFINE_RW_OBJ_PROPERTY(P_FORMAT_FOR_PRINTING, Bool, MCStack, FormatForPrinting)

	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_LINK_COLOR, InterfaceNamedColor, MCStack, LinkColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_LINK_COLOR, InterfaceNamedColor, MCStack, LinkColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_LINK_HILITE_COLOR, InterfaceNamedColor, MCStack, LinkHiliteColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_LINK_HILITE_COLOR, InterfaceNamedColor, MCStack, LinkHiliteColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_LINK_VISITED_COLOR, InterfaceNamedColor, MCStack, LinkVisitedColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_LINK_VISITED_COLOR, InterfaceNamedColor, MCStack, LinkVisitedColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_UNDERLINE_LINKS, OptionalBool, MCStack, UnderlineLinks)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_UNDERLINE_LINKS, Bool, MCStack, UnderlineLinks)

	DEFINE_RW_OBJ_PROPERTY(P_WINDOW_SHAPE, UInt32, MCStack, WindowShape)
	DEFINE_RO_OBJ_PROPERTY(P_SCREEN, Int16, MCStack, Screen)
	DEFINE_RW_OBJ_PROPERTY(P_CURRENT_CARD, OptionalString, MCStack, CurrentCard)
	DEFINE_RW_OBJ_PROPERTY(P_MODIFIED_MARK, Bool, MCStack, ModifiedMark)
	DEFINE_RW_OBJ_PROPERTY(P_ACCELERATED_RENDERING, Bool, MCStack, AcceleratedRendering)

	DEFINE_RW_OBJ_ENUM_PROPERTY(P_COMPOSITOR_TYPE, InterfaceCompositorType, MCStack, CompositorType)
	DEFINE_RW_OBJ_PROPERTY(P_COMPOSITOR_CACHE_LIMIT, OptionalUInt32, MCStack, CompositorCacheLimit)
    DEFINE_RW_OBJ_PROPERTY(P_COMPOSITOR_TILE_SIZE, OptionalUInt32, MCStack, CompositorTileSize)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_DEFER_SCREEN_UPDATES, Bool, MCStack, DeferScreenUpdates)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_DEFER_SCREEN_UPDATES, Bool, MCStack, DeferScreenUpdates)
    
    DEFINE_RW_OBJ_PROPERTY(P_IGNORE_MOUSE_EVENTS, Bool, MCStack, IgnoreMouseEvents)
    
    DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_DECORATIONS, InterfaceDecoration, MCStack, Decorations)

	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_SHOW_BORDER)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_BORDER_WIDTH)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_ENABLED)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_DISABLED)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_3D)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_LOCK_LOCATION)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_TOOL_TIP)
	// MW-2012-03-13: [[ UnicodeToolTip ]] Stacks don't have tooltips.
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_UNICODE_TOOL_TIP)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_LAYER)
    
   	// IM-2013-09-23: [[ FullscreenMode ]] Add stack fullscreenMode property
    DEFINE_RW_OBJ_ENUM_PROPERTY(P_FULLSCREENMODE, InterfaceStackFullscreenMode, MCStack, FullscreenMode)
    
    DEFINE_RO_OBJ_PROPERTY(P_KEY, Bool, MCStack, Key)
    DEFINE_RO_OBJ_PROPERTY(P_PASSWORD, BinaryString, MCStack, Password)
    
    // IM-2014-01-07: [[ StackScale ]] Add stack scalefactor property
    DEFINE_RW_OBJ_PROPERTY(P_SCALE_FACTOR, Double, MCStack, ScaleFactor)
    
    // MERG-2015-08-31: [[ ScriptOnly ]] Add stack scriptOnly property
    DEFINE_RW_OBJ_PROPERTY(P_SCRIPT_ONLY, Bool, MCStack, ScriptOnly)
    
    // MERG-2015-10-11: [[ DocumentFilename ]] Add stack documentFilename property
    DEFINE_RW_OBJ_PROPERTY(P_DOCUMENT_FILENAME, String, MCStack, DocumentFilename)
	
	// IM-2016-02-26: [[ Bug 16244 ]] Add stack showInvisibles property
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_SHOW_INVISIBLES, OptionalBool, MCStack, ShowInvisibleObjects)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_SHOW_INVISIBLES, Bool, MCStack, ShowInvisibleObjects)
};

MCObjectPropertyTable MCStack::kPropertyTable =
{
	&MCObject::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCStack::MCStack()
{
	obj_id = START_ID;
	flags = F_VISIBLE | F_RESIZABLE | F_OPAQUE;
	window = NULL;
	
	// IM-2014-07-23: [[ Bug 12930 ]] No parent window to start with
	m_parent_stack = nil;
	
	cursor = None;
	substacks = NULL;
	cards = curcard = savecards = NULL;
	controls = NULL;
	editing = NULL;
	aclips = NULL;
	vclips = NULL;
	backgroundid = 0;
	state = 0;
	iconid = 0;
	rect.x = rect.y = 0;
	rect.width = MCminsize << 5;
	rect.height = MCminsize << 5;
	title = MCValueRetain(kMCEmptyString);
	titlestring = MCValueRetain(kMCEmptyString);
	minwidth = MCminstackwidth;
	minheight = MCminstackheight;
	maxwidth = MAXUINT2;
	maxheight = MAXUINT2;
	externalfiles = MCValueRetain(kMCEmptyString);
	idlefunc = NULL;
	windowshapeid = 0;

	old_blendlevel = 100;

	nfuncs = 0;
	nmnemonics = 0;
	lasty = 0;
	mnemonics = NULL;
	nneeds = 0;
	needs = NULL;
	mode = WM_CLOSED;
	
	// MW-2014-01-30: [[ Bug 5331 ]] Make liveResizing on by default
	flags |= F_DECORATIONS;
	decorations = WD_MENU | WD_TITLE | WD_MINIMIZE | WD_MAXIMIZE | WD_CLOSE | WD_LIVERESIZING;
	
	nstackfiles = 0;
	stackfiles = NULL;
	linkatts = NULL;
	filename = MCValueRetain(kMCEmptyString);
	/* UNCHECKED */ MCNameClone(kMCEmptyName, _menubar);
	menuy = menuheight = 0;
	menuwindow = False;

	f_extended_state = 0;
	m_externals = NULL;

	// MW-2011-09-12: [[ MacScroll ]] There is no scroll to start with.
	m_scroll = 0;

	// MW-2011-09-13: [[ Effects ]] No snapshot to begin with.
	m_snapshot = nil;
	
	// MW-2011-09-13: [[ Masks ]] The window mask starts off as nil.
	m_window_shape = nil;

	m_window_buffer = nil;
	
	// MW-2011-11-24: [[ UpdateScreen ]] Start off with defer updates false.
	m_defer_updates = false;

	// MW-2012-10-10: [[ IdCache ]]
	m_id_cache = nil;

	// MW-2014-03-12: [[ Bug 11914 ]] Stacks are not engine menus by default.
	m_is_menu = false;
	
    // MW-2014-09-30: [[ ScriptOnlyStack ]] Stacks are not script-only by default.
    m_is_script_only = false;
    
	// IM-2014-05-27: [[ Bug 12321 ]] No fonts to purge yet
	m_purge_fonts = false;
    
    // MERG-2015-10-11: [[ DocumentFilename ]] The filename the stack represnts
    m_document_filename = MCValueRetain(kMCEmptyString);

	// IM-2016-02-29: [[ Bug 16244 ]] by default, visibility is determined by global 'showInvisibles' property.
	m_hidden_object_visibility = kMCStackObjectVisibilityDefault;
	
	m_view_need_redraw = false;
	m_view_need_resize = false;

	cursoroverride = false ;
	old_rect.x = old_rect.y = old_rect.width = old_rect.height = 0 ;

    m_attachments = nil;
    
	view_init();
}

MCStack::MCStack(const MCStack &sref) : MCObject(sref)
{
	obj_id = sref.obj_id;

	// MW-2007-07-05: [[ Bug 3491 ]] Creating unnamed stacks in quick succession gives them the same name
	if (isunnamed())
	{
		int4 t_time;
		t_time = (int4)MCS_time();
		if (t_time == s_last_stack_time)
		{
			char t_name[U4L * 2 + 7];
			sprintf(t_name, "Stack %d.%d", t_time, s_last_stack_index);
			setname_cstring(t_name);
			s_last_stack_index += 1;
		}
		else
		{
			char t_name[U4L + 7];
			sprintf(t_name, "Stack %d", t_time);
			setname_cstring(t_name);
			s_last_stack_time = t_time;
			s_last_stack_index = 2;
		}
	}
	window = NULL;
	
	// IM-2014-07-23: [[ Bug 12930 ]] No parent window to start with
	m_parent_stack = nil;
	
	cursor = None;
	substacks = NULL;
	cards = curcard = savecards = NULL;
	controls = NULL;
	editing = NULL;
	aclips = NULL;
	vclips = NULL;
	backgroundid = 0;
	iconid = 0;
	title = MCValueRetain(sref.title);
	titlestring = MCValueRetain(kMCEmptyString);
	minwidth = sref.minwidth;
	minheight = sref.minheight;
	maxwidth = sref.maxwidth;
	maxheight = sref.maxheight;
	externalfiles = MCValueRetain(sref.externalfiles);
	idlefunc = NULL;
	windowshapeid = sref.windowshapeid;

	old_blendlevel = sref . blendlevel;

	menuwindow = sref . menuwindow;

	// MW-2011-09-12: [[ MacScroll ]] Make sure the rect refers to the unscrolled size.
	rect . height += sref . getscroll();
	
	// MW-2012-10-10: [[ IdCache ]]
	m_id_cache = nil;
    
	mnemonics = NULL;
	nfuncs = 0;
	nmnemonics = 0;
	lasty = sref.lasty;
	if (sref.controls != NULL)
	{
		MCControl *optr = sref.controls;
		do
		{
			// MW-2011-08-08: We are cloning a stack so want to copy ids from
			//   the original objects, thus use 'doclone' if we are cloning
			//   a group.
			MCControl *newcontrol;
			if (optr -> gettype() == CT_GROUP)
			{
				MCGroup *t_group;
				t_group = (MCGroup *)optr;
				newcontrol = t_group -> doclone(False, OP_NONE, true, false);
			}
			else
				newcontrol = optr -> clone(False, OP_NONE, false);

			newcontrol->setid(optr->getid());
			newcontrol->appendto(controls);
			newcontrol->setparent(this);
			optr = optr->next();
		}
		while (optr != sref.controls);
	}
	if (sref.cards != NULL)
	{
		MCCard *cptr = sref.cards;
		do
		{
			MCCard *newcard = cptr->clone(False, False);
			newcard->setid(cptr->getid());
			newcard->appendto(cards);
			newcard->setparent(this);
			newcard->clonedata(cptr);
			newcard->replacedata(NULL);
			cptr = cptr->next();
		}
		while (cptr != sref.cards);
		curcard = cards;
	}
	if (sref.aclips != NULL)
	{
		MCAudioClip *aptr = sref.aclips;
		do
		{
			MCAudioClip *newaclip = new MCAudioClip(*aptr);
			newaclip->setid(aptr->getid());
			newaclip->appendto(aclips);
			newaclip->setparent(this);
			aptr = aptr->next();
		}
		while (aptr != sref.aclips);
	}
	if (sref.vclips != NULL)
	{
		MCVideoClip *vptr = sref.vclips;
		do
		{
			MCVideoClip *newvclip = new MCVideoClip(*vptr);
			newvclip->setid(vptr->getid());
			newvclip->appendto(vclips);
			newvclip->setparent(this);
			vptr = vptr->next();
		}
		while (vptr != sref.vclips);
	}
	nneeds = 0;
	needs = NULL;
	mode = WM_CLOSED;
	decorations = sref.decorations;
	nstackfiles = sref.nstackfiles;
	if (nstackfiles != 0)
	{
		uint2 ts = nstackfiles;
		stackfiles = new MCStackfile[ts];
		while (ts--)
		{
			stackfiles[ts].stackname = MCValueRetain(sref.stackfiles[ts].stackname);
			stackfiles[ts].filename = MCValueRetain(sref.stackfiles[ts].filename);
		}
	}
	else
		stackfiles = NULL;
	if (sref.linkatts != NULL)
	{
		linkatts = new Linkatts;
		memcpy(linkatts, sref.linkatts, sizeof(Linkatts));
        
		linkatts->colorname = linkatts->colorname == nil ? nil : (MCStringRef)MCValueRetain(sref.linkatts->colorname);
		linkatts->hilitecolorname = linkatts->hilitecolorname == nil ? nil : (MCStringRef)MCValueRetain(sref.linkatts->hilitecolorname);
		linkatts->visitedcolorname = linkatts->hilitecolorname == nil ? nil : (MCStringRef)MCValueRetain(sref.linkatts->visitedcolorname);
	}
	else
		linkatts = NULL;
	filename = MCValueRetain(kMCEmptyString);
	/* UNCHECKED */ MCNameClone(sref._menubar, _menubar);
	menuy = menuheight = 0;

	f_extended_state = 0;

	m_externals = NULL;

	// MW-2011-09-12: [[ MacScroll ]] There is no scroll to start with.
	m_scroll = 0;
	
	// MW-2011-09-13: [[ Effects ]] No snapshot to begin with.
	m_snapshot = nil;
	
	// MW-2011-09-13: [[ Masks ]] The windowmask starts off as nil.
	m_window_shape = nil;

	m_window_buffer = nil;

	// MW-2011-11-24: [[ UpdateScreen ]] Start off with defer updates false.
	m_defer_updates = false;
	
	// MW-2010-11-17: [[ Valgrind ]] Uninitialized value.
	cursoroverride = false;
	
	// MW-2014-03-12: [[ Bug 11914 ]] Stacks are not engine menus by default.
	m_is_menu = false;
	
    // MW-2014-09-30: [[ ScriptOnlyStack ]] Stacks copy the source script-onlyness.
    m_is_script_only = sref.m_is_script_only;
    
	// IM-2014-05-27: [[ Bug 12321 ]] No fonts to purge yet
	m_purge_fonts = false;

	// IM-2016-02-29: [[ Bug 16244 ]] by default, visibility is determined by global 'showInvisibles' property.
	m_hidden_object_visibility = kMCStackObjectVisibilityDefault;

	m_view_need_redraw = sref.m_view_need_redraw;
	m_view_need_resize = sref.m_view_need_resize;

    m_attachments = nil;
    
    // MERG-2015-10-12: [[ DocumentFilename ]] No document filename to begin with
    m_document_filename = MCValueRetain(kMCEmptyString);
    
	view_copy(sref);
}

MCStack::~MCStack()
{
	flags &= ~F_DESTROY_STACK;
	state |= CS_DELETE_STACK;
	while (opened)
		close();
	extraclose(false);

	if (needs != NULL)
	{
		while (nneeds--)

			needs[nneeds]->removelink(this);
		delete needs;
	}
	if (substacks != NULL)
	{
		while (substacks != NULL)
		{
			MCStack *sptr = substacks->remove
			                (substacks);
			delete sptr;
		}
		opened++;
		MCObject::close();
	}
	
	if (m_parent_stack != nil)
		setparentstack(nil);

	delete mnemonics;
	MCValueRelease(title);
	MCValueRelease(titlestring);

	if (window != NULL && !(state & CS_FOREIGN_WINDOW))
	{
		stop_externals();
		MCscreen->destroywindow(window);
	}

	while (controls != NULL)
	{
		MCControl *cptr = controls->remove
		                  (controls);
		delete cptr;
	}
	while (aclips != NULL)
	{
		MCAudioClip *aptr = aclips->remove
		                    (aclips);
		delete aptr;
	}
	while (vclips != NULL)
	{
		MCVideoClip *vptr = vclips->remove
		                    (vclips);
		delete vptr;
	}
	MCrecent->deletestack(this);
	MCcstack->deletestack(this);
	while (cards != NULL)
	{
		MCCard *cptr = cards->remove
		               (cards);
		delete cptr;
	}
	MCValueRelease(externalfiles);

	uint2 i = 0;
	while (i < MCnusing)
		if (MCusing[i] == this)
		{
			MCnusing--;
			uint2 j;
			for (j = i ; j < MCnusing ; j++)
				MCusing[j] = MCusing[j + 1];
		}
		else
			i++;
	// MW-2004-11-17: If this is the current Message Box, set to NULL
	if (MCmbstackptr == this)
		MCmbstackptr = NULL;
	if (MCstaticdefaultstackptr == this)
		MCstaticdefaultstackptr = MCtopstackptr;
	if (MCdefaultstackptr == this)
		MCdefaultstackptr = MCstaticdefaultstackptr;
	if (stackfiles != NULL)
	{
		while (nstackfiles--)
		{
			MCValueRelease(stackfiles[nstackfiles].stackname);
			MCValueRelease(stackfiles[nstackfiles].filename);
		}
		delete stackfiles;
	}
	if (linkatts != NULL)
	{
		MCValueRelease(linkatts->colorname);
		MCValueRelease(linkatts->hilitecolorname);
		MCValueRelease(linkatts->visitedcolorname);
		delete linkatts;
	}
	MCValueRelease(filename);

	MCNameDelete(_menubar);

	unloadexternals();

	// COCOA-TODO: Remove dependence on ifdef
#if !defined(_MAC_DESKTOP)
	MCEventQueueFlush(this);
#endif
	
	// MW-2011-09-13: [[ Redraw ]] If there is snapshot, get rid of it.
	MCGImageRelease(m_snapshot);
	m_snapshot = nil;
	
	// MW-2012-10-10: [[ IdCache ]] Free the idcache.
	freeobjectidcache();
	
	view_destroy();

	release_window_buffer();
}

Chunk_term MCStack::gettype() const
{
	return CT_STACK;
}

const char *MCStack::gettypestring()
{
	return MCstackstring;
}

bool MCStack::visit_self(MCObjectVisitor* p_visitor)
{
	return p_visitor -> OnStack(this);
}

bool MCStack::visit_children(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor* p_visitor)
{
	bool t_continue;
	t_continue = true;

	if (t_continue && cards != nil)
	{
		MCCard *t_card;
		t_card = cards;
		do
		{
			if (MCObjectVisitorIsHeirarchical(p_options))
				t_continue = t_card->visit(p_options, p_part, p_visitor);
			else
				t_continue = t_card->visit_self(p_visitor);
			t_card = t_card -> next();
		}
		while(t_continue && t_card != cards);
	}

	if (t_continue && controls != nil)
	{
		MCControl *t_control;
		t_control = controls;
		do
		{
			if (!MCObjectVisitorIsHeirarchical(p_options) || t_control->getparent() == this)
				t_continue = t_control -> visit(p_options, 0, p_visitor);
			t_control = t_control -> next();
		}
		while(t_continue && t_control != controls);
	}

	if (t_continue && MCObjectVisitorIsHeirarchical(p_options) && substacks != nil)
	{
		MCStack *t_stack = substacks;
		do
		{
			t_continue = t_stack->visit(p_options, 0, p_visitor);
			t_stack = t_stack->next();
		}
		while (t_continue && t_stack != substacks);
	}
	
	return true;
}

void MCStack::open()
{
	openrect(rect, WM_LAST, NULL, WP_DEFAULT,OP_NONE);
}

void MCStack::close()
{
	if (!opened)
		return;
	
	// MW-2014-02-25: [[ Platform ]] Make sure we lock the screen when closing
	//   so nothing is seen.
	MCRedrawLockScreen();
				
	// MW-2014-03-12: [[ Bug 11914 ]] Only fiddle with scrolling and such
	//   if this is an engine menu.
	if (m_is_menu && menuheight && (rect.height != menuheight || menuy != 0))
	{
		if (menuy != 0)
			scrollmenu(-menuy, False);
		minheight = maxheight = rect.height = menuheight;
	}
	
	if (state & CS_IGNORE_CLOSE)
	{
		state &= ~(CS_IGNORE_CLOSE);

		return;
	}
	if (editing != NULL)
		stopedit();
	state |= CS_IGNORE_CLOSE;
	
	// MW-2008-10-31: [[ ParentScripts ]] Send closeControl messages as appropriate.
	curcard -> closecontrols();
	curcard->message(MCM_close_card);
	curcard -> closebackgrounds(NULL);
	curcard->message(MCM_close_stack);
	
	state &= ~CS_SUSPENDED;
	
	curcard->close();
	
	// MW-2011-09-12: [[ MacScroll ]] Clear the current scroll setting from the stack.
	clearscroll();
	
	if (MCacptr != NULL && MCacptr->getmessagestack() == this)
		MCacptr->setmessagestack(NULL);
	if (state & CS_ICONIC)
	{
		seticonic(false);
	}
	if (MCmousestackptr == this)
	{
		MCmousestackptr = NULL;
		int2 x, y;
		MCscreen->querymouse(x, y);
		if (MCU_point_in_rect(curcard->getrect(), x, y))
		{
			ibeam = 0;
		}
	}
	if (MCclickstackptr == this)
		MCclickstackptr = NULL;
	if (MCfocusedstackptr == this)
		MCfocusedstackptr = NULL;
	if (!(state & CS_ICONIC))
		MCstacks->remove(this);
	if (window != NULL && !(state & CS_FOREIGN_WINDOW))
	{
		MCscreen->closewindow(window);
		if (mode == WM_MODAL || mode == WM_SHEET)
			MCscreen->closemodal();
		if (iconid != 0)
		{
			MCImage *iptr = (MCImage *)getobjid(CT_IMAGE, iconid);
			if (iptr != NULL)
				iptr->close();
		}

		MCscreen->flush(window);

		if (flags & F_DESTROY_WINDOW && MCdispatcher -> gethome() != this)
		{
			stop_externals();
			MCscreen->destroywindow(window);
			window = NULL;
			cursor = None;
			MCValueAssign(titlestring, kMCEmptyString);
			state &= ~CS_BEEN_MOVED;
		}
	}
	if (substacks == NULL)
		MCObject::close();
	else if (opened != 0)
		opened--;

	// MW-2011-09-13: [[ Effects ]] Free any snapshot that we have.
	MCGImageRelease(m_snapshot);
	m_snapshot = nil;
	
	state &= ~(CS_IGNORE_CLOSE | CS_KFOCUSED | CS_ISOPENING);
	
	MCRedrawUnlockScreen();
}

void MCStack::kfocus()
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return;
	MCstacks->top(this);
	MCfocusedstackptr = this;

	// MW-2007-09-11: [[ Bug 5139 ]] Don't add activity to recent cards if the stack is an
	//   IDE stack.
	if ((mode == WM_TOP_LEVEL || mode == WM_TOP_LEVEL_LOCKED) && editing == NULL && !getextendedstate(ECS_IDE))
		MCrecent->addcard(curcard);

	if (state & CS_SUSPENDED)
	{
		curcard->message(MCM_resume_stack);
        
        // We have just invoked script, so it is entirely possible that focus is now
        // 'elsewhere' - if the focused stack is not us, then do nothing.
        if (MCfocusedstackptr != this)
            return;

		state &= ~CS_SUSPENDED;
	}
    
	if (!(state & CS_KFOCUSED))
	{
		state |= CS_KFOCUSED;
		if (gettool(this) == T_BROWSE)
			curcard->kfocus();
		updatemenubar();
		if (hashandlers & HH_IDLE)
			MCscreen->addtimer(this, MCM_idle, MCidleRate);
		if (curcard->gethashandlers() & HH_IDLE)
			MCscreen->addtimer(curcard, MCM_idle, MCidleRate);
	}
}

Boolean MCStack::kfocusnext(Boolean top)
{
	if (!opened || flags & F_CANT_MODIFY || gettool(this) != T_BROWSE)
		return False;
	Boolean done = curcard->kfocusnext(top);
	
	// MW-2014-03-12: [[ Bug 11914 ]] Only fiddle with scrolling and such
	//   if this is an engine menu.
	if (m_is_menu && menuheight && (rect.height != menuheight || menuy != 0))
		scrollintoview();
	return done;
}

Boolean MCStack::kfocusprev(Boolean bottom)
{
	if (!opened || flags & F_CANT_MODIFY || gettool(this) != T_BROWSE)
		return False;
	Boolean done = curcard->kfocusprev(bottom);
	
	// MW-2014-03-12: [[ Bug 11914 ]] Only fiddle with scrolling and such
	//   if this is an engine menu.
	if (m_is_menu && menuheight && (rect.height != menuheight || menuy != 0))
		scrollintoview();
	return done;
}


void MCStack::kunfocus()
{
	if (!(state & CS_KFOCUSED))
		return;
	if (MCfocusedstackptr == this)
		MCfocusedstackptr = NULL;
	state &= ~CS_KFOCUSED;
	if (!opened)
		return;
	curcard->message(MCM_suspend_stack);
    
    // We have just invoked script, so we might be the focused stack again. So
    // if focused stack is us, then do nothing.
    if (MCfocusedstackptr == this)
        return;
    
	state |= CS_SUSPENDED;
	curcard->kunfocus();
}

Boolean MCStack::kdown(MCStringRef p_string, KeySym key)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	if (curcard->kdown(p_string, key))
		return True;
	MCObject *optr;
	switch (key)
	{
	case XK_Delete:
		if (MCmodifierstate & MS_MOD1)
			return MCundos->undo();
		if (MCmodifierstate & MS_SHIFT)
			if (MCactiveimage != NULL)
			{
				MCactiveimage->cutimage();
				return True;
			}
			else
				return MCselected->cut();
		if (MCactiveimage != NULL)
		{
			MCactiveimage->delimage();
			return True;
		}
		return MCselected->del();
	case XK_BackSpace:
		if (MCmodifierstate & MS_MOD1)
			return MCundos->undo();
		if (MCactiveimage != NULL)
		{
			MCactiveimage->delimage();
			return True;
		}
		return MCselected->del();
	case XK_osfUndo:
		return MCundos->undo();
	case XK_osfCut:
		if (MCactiveimage != NULL)
		{
			MCactiveimage->cutimage();
			return True;
		}
		else
			return MCselected->cut();
	case XK_osfCopy:
		if (MCactiveimage != NULL)
		{
			MCactiveimage->copyimage();
			return True;
		}
		else
			return MCselected->copy();
	case XK_osfPaste:
		MCdefaultstackptr = MCtopstackptr;
		return MCdispatcher -> dopaste(optr);
	case XK_Insert:
		if (MCmodifierstate & MS_SHIFT)
		{
			MCdefaultstackptr = MCtopstackptr;
			return MCdispatcher -> dopaste(optr);
		}
		if (MCmodifierstate & MS_CONTROL)
			if (MCactiveimage != NULL)
			{
				MCactiveimage->copyimage();
				return True;
			}
		return MCselected->copy();
	case XK_Left:
		if (mode >= WM_PULLDOWN || !MCnavigationarrows)
			return False;
		if (MCmodifierstate & MS_CONTROL)
			setcard(getstack()->getchild(CT_FIRST, kMCEmptyString, CT_CARD), True, False);
		else
			setcard(getstack()->getchild(CT_PREV, kMCEmptyString, CT_CARD), True, False);
		return True;
	case XK_Right:
		if (mode >= WM_PULLDOWN || !MCnavigationarrows)
			return False;
		if (MCmodifierstate & MS_CONTROL)
			setcard(getstack()->getchild(CT_LAST, kMCEmptyString, CT_CARD), True, False);
		else
			setcard(getstack()->getchild(CT_NEXT, kMCEmptyString, CT_CARD), True, False);
		return True;
	case XK_Up:
		if (mode >= WM_PULLDOWN || !MCnavigationarrows)
			return False;
		if (MCmodifierstate & MS_CONTROL)
			MCrecent->godirect(True);
		else
			MCrecent->gorel(-1);
		return True;
	case XK_Down:
		if (mode >= WM_PULLDOWN || !MCnavigationarrows)
			return False;
		if (MCmodifierstate & MS_CONTROL)
			MCrecent->godirect(False);
		else
			MCrecent->gorel(1);
		return True;
	case XK_Return:
	case XK_KP_Enter:
		if (!(MCmodifierstate & (MS_MOD1 | MS_CONTROL)))
		{
			MCButton *bptr = curcard->getdefbutton();

			if (bptr != NULL)
			{
				bptr->activate(False, 0);
				return True;
			}
		}
		break;
	default:
		break;
	}
	if (MClook != LF_MOTIF && MCmodifierstate & MS_CONTROL)
		switch (key)
		{
		case XK_C:
		case XK_c:
			if (MCactiveimage != NULL)
			{
				MCactiveimage->copyimage();
				return True;
			}
			return MCselected->copy();
		case XK_V:
		case XK_v:
			MCdefaultstackptr = MCtopstackptr;
			return MCdispatcher -> dopaste(optr);
		case XK_X:

		case XK_x:

			if (MCactiveimage != NULL)
			{
				MCactiveimage->cutimage();
				return True;
			}
			return MCselected->cut();
		case XK_Z:
		case XK_z:
			return MCundos->undo();
		}
	uint2 i;
	
	// Does this keypress correspond to a mnemonic letter?
	// The comparison should be case-insensitive for letters
	KeySym t_key;
	t_key = MCKeySymToLower(key);
	for (i = 0 ; i < nmnemonics ; i++)
	{
		if (mnemonics[i].key == t_key
		        && mnemonics[i].button->isvisible()
		        && !mnemonics[i].button->isdisabled())
		{
			mnemonics[i].button->activate(True, key);
			return True;
		}
	}

	return False;
}

Boolean MCStack::kup(MCStringRef p_string, KeySym key)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	Boolean done = curcard->kup(p_string, key);
    
	// MW-2014-03-12: [[ Bug 11914 ]] Only fiddle with scrolling and such
	//   if this is an engine menu.
	if (m_is_menu && menuheight && (rect.height != menuheight || menuy != 0))
		scrollintoview();
	
	return done;
}

Boolean MCStack::mfocus(int2 x, int2 y)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	//XCURSORS
	if ( !cursoroverride )
		setcursor(getcursor(), False);
	
	// MW-2014-03-12: [[ Bug 11914 ]] Only fiddle with scrolling and such
	//   if this is an engine menu.
	if (m_is_menu && menuheight && (rect.height != menuheight || menuy != 0))
	{
		MCControl *cptr = curcard->getmfocused();
		if (x < rect.width || cptr != NULL && !cptr->getstate(CS_SUBMENU))
		{
			uint1 oldmode = scrollmode;
			if (menuy < 0 && y < MENU_ARROW_SIZE >> 1)
			{
				if (y < 0)
					scrollmode = SM_PAGEDEC;
				else
					scrollmode = SM_LINEDEC;
				if (oldmode == SM_CLEARED)
					timer(MCM_internal, NULL);
				x = y = -MAXINT2;
			}
			else
				if (menuy + menuheight > rect.height
				        && y > rect.height - MENU_ARROW_SIZE)
				{
					if (y > rect.height)
						scrollmode = SM_PAGEINC;

					else
						scrollmode = SM_LINEINC;
					if (oldmode == SM_CLEARED)
						timer(MCM_internal, NULL);
					x = y = -MAXINT2;
				}
				else
					if (scrollmode != SM_CLEARED)
					{
						MCscreen->cancelmessageobject(curcard, MCM_internal);
						scrollmode = SM_CLEARED;
					}
		}
	}
	return curcard->mfocus(x, y);
}

void MCStack::mfocustake(MCControl *target)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return;
	curcard->mfocustake(target);
}

void MCStack::munfocus(void)
{
	if (MCmousestackptr == this)
		MCmousestackptr = NULL;
	if (curcard != 0)
	{
		if (gettool(this) != T_SELECT)
			curcard->munfocus();
		if (ibeam != 0 && !curcard->getgrab())
			ibeam = 0;
	}
}

void MCStack::mdrag(void)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return;
	curcard -> mdrag();
}

Boolean MCStack::mdown(uint2 which)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	return curcard->mdown(which);
}

Boolean MCStack::mup(uint2 which, bool p_release)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	Boolean handled = curcard->mup(which, p_release);
	// MW-2010-07-06: [[ Bug ]] We should probably only mfocus the card if this
	//   stack is still the mouse stack.
	if (opened && mode < WM_PULLDOWN && MCmousestackptr == this)
		curcard->mfocus(MCmousex, MCmousey);
	return handled;
}

Boolean MCStack::doubledown(uint2 which)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	return curcard->doubledown(which);
}

Boolean MCStack::doubleup(uint2 which)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	return curcard->doubleup(which);
}

void MCStack::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualTo(mptr, MCM_internal, kMCCompareCaseless))
	{
		if (scrollmode == SM_PAGEDEC || scrollmode == SM_LINEDEC)
		{
			int2 newoffset = controls->getrect().height;
			if (-menuy < newoffset)
			{
				newoffset = -menuy;
				scrollmode = SM_CLEARED;
			}
			scrollmenu(newoffset, True);
		}
		else if (scrollmode == SM_PAGEINC || scrollmode == SM_LINEINC)
			{
				int2 newoffset = -controls->getrect().height;
				if (menuy + menuheight + newoffset < rect.height)
				{
					newoffset = rect.height - menuy - menuheight;
					scrollmode = SM_CLEARED;
				}
				scrollmenu(newoffset, True);
			}
		if (scrollmode == SM_PAGEINC || scrollmode == SM_PAGEDEC)
			MCscreen->addtimer(this, MCM_internal, MCsyncrate);
		else
			if (scrollmode == SM_LINEINC || scrollmode == SM_LINEDEC)
				MCscreen->addtimer(this, MCM_internal, MCrepeatrate);
	}
	else if (MCNameIsEqualTo(mptr, MCM_idle, kMCCompareCaseless))
		{
			if (opened && hashandlers & HH_IDLE && state & CS_KFOCUSED
			        && getstack()->gettool(this) == T_BROWSE)
			{
				external_idle();
				if (message(mptr, params, True, True) == ES_ERROR)
					senderror();
				else
					MCscreen->addtimer(this, MCM_idle, MCidleRate);
			}
		}
		else
			MCObject::timer(mptr, params);
}

MCRectangle MCStack::getrectangle(bool p_effective) const
{
    if (!p_effective)
        return getrect();
    
    return getwindowrect();
}

void MCStack::applyrect(const MCRectangle &nrect)
{
	// IM-2013-09-30: [[ FullscreenMode ]] allow setrect on mobile,
	// which now has an effect on fullscreen scaled stacks

	// IM-2013-09-23: [[ FullscreenMode ]] Use view to determine adjusted stack size
	MCRectangle t_new_rect;
	MCRectangle t_view_rect;
	MCGAffineTransform t_transform;
	// IM-2014-01-16: [[ StackScale ]] Get new rect size from view
	view_calculate_viewports(nrect, t_new_rect, t_view_rect, t_transform);
	
	if (rect.x != t_new_rect.x || rect.y != t_new_rect.y)
		state |= CS_BEEN_MOVED;

	MCRectangle oldrect = rect;
	// IM-2013-09-30: [[ FullscreenMode ]] update old_rect when modifying the stack rect
	old_rect = rect = t_new_rect;
	
	menuy = menuheight = 0;
	if (opened && haswindow())
	{
		mode_constrain(rect);
		
		// MW-2014-03-12: [[ Bug 11914 ]] Only fiddle with scrolling and such
		//   if this is an engine menu.
		if (m_is_menu && (mode == WM_PULLDOWN || mode == WM_OPTION))
		{
			rect.x = oldrect.x;
			rect.y = oldrect.y;
			menuheight = minheight = maxheight = rect.height;
		}

		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		if (MCRedrawIsScreenLocked())
		{
			state |= CS_NEED_RESIZE;
			MCRedrawScheduleUpdateForStack(this);
		}
		else
			setgeom();
	}
	else
	{
		// IM-2014-01-16: [[ StackView ]] Ensure view is updated with new stack rect
		view_setstackviewport(nrect);
	}
	
	// MW-2012-10-23: [[ Bug 10461 ]] Make sure we do this *after* the stack has been resized
	//   otherwise we get cases where the card size isn't updated.
	// MW-2012-10-04: [[ Bug 10420 ]] If we have a card then make sure we sync the
	//   stack size to it.
	if (curcard != nil)
		resize(oldrect . width, oldrect . height);
}

#ifdef LEGACY_EXEC
Exec_stat MCStack::getprop_legacy(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective, bool recursive)
{
	uint2 j = 0;
	uint2 k = 0;
	MCStack *sptr = this;
	uint2 num;

	switch (which)
	{
#ifdef /* MCStack::getprop */ LEGACY_EXEC		
	case P_FULLSCREEN:
			ep.setsvalue( MCU_btos(getextendedstate(ECS_FULLSCREEN)));
	break;

	// IM-2013-09-23: [[ FullscreenMode ]] Add stack fullscreenMode property
	case P_FULLSCREENMODE:
		ep.setsvalue(MCStackFullscreenModeToString(view_getfullscreenmode()));
		break;
			
	// IM-2014-01-07: [[ StackScale ]] Add stack scalefactor property
	case P_SCALE_FACTOR:
		ep.setnvalue(view_get_content_scale());
		break;
		
	case P_LONG_ID:
	case P_LONG_NAME:
		if (filename == NULL)
		{
			if (MCdispatcher->ismainstack(sptr))
			{
				if (!isunnamed())
					ep.setstringf("stack \"%s\"", getname_cstring());
				else
					ep.clear();
			}
			else if (isunnamed())
				ep.clear();
			else
				return names(P_LONG_NAME, ep, parid);
		}
		else
			ep.setstringf("stack \"%s\"", filename);
		break;
	case P_NUMBER:
		if (parent != NULL && !MCdispatcher->ismainstack(sptr))
		{
			sptr = (MCStack *)parent;
			sptr->count(CT_STACK, CT_UNDEFINED, this, num);
			ep.setint(num);
		}
		else
			ep.setint(0);
		break;
	case P_LAYER:
		ep.setint(0);
		break;
	case P_FILE_NAME:
		if (effective && !MCdispatcher->ismainstack(this))
			return parent->getprop(0, which, ep, effective);
		ep.setsvalue(filename);
		break;
	case P_SAVE_COMPRESSED:
		ep.setboolean(True);
		break;
	case P_CANT_ABORT:
		ep.setboolean(getflag(F_CANT_ABORT));
		break;
	case P_CANT_DELETE:
		ep.setboolean(getflag(F_S_CANT_DELETE));
		break;
	case P_STYLE:
		switch (getstyleint(flags) + WM_TOP_LEVEL_LOCKED)
		{
		case WM_MODELESS:
			ep.setstaticcstring(MCmodelessstring);
			break;
		case WM_PALETTE:
			ep.setstaticcstring(MCpalettestring);
			break;
		case WM_MODAL:
			ep.setstaticcstring(MCmodalstring);
			break;
		case WM_SHEET:
			ep.setstaticcstring(MCsheetstring);
			break;
		case WM_DRAWER:
			ep.setstaticcstring(MCdrawerstring);
			break;
		default:
			ep.setstaticcstring(MCtoplevelstring);
			break;
		}
		break;
	case P_CANT_MODIFY:
		ep.setboolean(getflag(F_CANT_MODIFY));
		break;
	case P_CANT_PEEK:
		ep.setboolean(True);
		break;
	case P_DYNAMIC_PATHS:
		ep.setboolean(getflag(F_DYNAMIC_PATHS));
		break;
	case P_KEY:
		// OK-2010-02-11: [[Bug 8610]] - Passkey property more useful if it returns
		//   whether or not the script is available.
		ep . setboolean(iskeyed());
		
		break;
	case P_PASSWORD:
		ep . clear();
		break;
	case P_DESTROY_STACK:
		ep.setboolean(getflag(F_DESTROY_STACK));
		break;
	case P_DESTROY_WINDOW:
		ep.setboolean(getflag(F_DESTROY_WINDOW));
		break;
	case P_ALWAYS_BUFFER:
		ep.setboolean(getflag(F_ALWAYS_BUFFER));
		break;
	case P_LABEL:
	case P_UNICODE_LABEL:
		// MW-2007-07-06: [[ Bug 3226 ]] Updated to take into account 'title' being
		//   stored as a UTF-8 string.
		if (title == NULL)
			ep.clear();
		else
		{
			ep.setsvalue(title);
			if (which == P_LABEL)
				ep.utf8tonative();
			else
				ep.utf8toutf16();
		}
		break;
	case P_CLOSE_BOX:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_CLOSE);
		break;
	case P_ZOOM_BOX:
	case P_MAXIMIZE_BOX:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_MAXIMIZE);
		break;
	case P_DRAGGABLE:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_TITLE);
		break;
	case P_COLLAPSE_BOX:
	case P_MINIMIZE_BOX:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_TITLE);
		break;
	case P_LIVE_RESIZING:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_LIVERESIZING);
		break;
	case P_SYSTEM_WINDOW:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_UTILITY);
		break;
	case P_METAL:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_METAL);
		break;
	case P_SHADOW:
		ep.setboolean(!(flags & F_DECORATIONS && decorations & WD_NOSHADOW));
		break;
	case P_RESIZABLE:
		ep.setboolean(getflag(F_RESIZABLE));
		break;
	case P_MIN_WIDTH:
		ep.setint(minwidth);
		break;
	case P_MAX_WIDTH:
		ep.setint(maxwidth);
		break;
	case P_MIN_HEIGHT:
		ep.setint(minheight);
		break;
	case P_MAX_HEIGHT:
		ep.setint(maxheight);
		break;
	case P_DECORATIONS:

		ep.clear();
		if (flags & F_DECORATIONS)
		{
			if (decorations & WD_WDEF)
				ep.setint(decorations & ~WD_WDEF);
			else
			{
				if (decorations & WD_TITLE)
					ep.concatcstring(MCtitlestring, EC_COMMA, j++ == 0);
				if (decorations & WD_MENU)
					ep.concatcstring(MCmenustring, EC_COMMA, j++ == 0);
				if (decorations & WD_MINIMIZE)
					ep.concatcstring(MCminimizestring, EC_COMMA, j++ == 0);
				if (decorations & WD_MAXIMIZE)
					ep.concatcstring(MCmaximizestring, EC_COMMA, j++ == 0);
				if (decorations & WD_CLOSE)
					ep.concatcstring(MCclosestring, EC_COMMA, j++ == 0);
				if (decorations & WD_METAL)
					ep.concatcstring(MCmetalstring, EC_COMMA, j++ == 0);
				if (decorations & WD_UTILITY)
					ep.concatcstring(MCutilitystring, EC_COMMA, j++ == 0);
				if (decorations & WD_NOSHADOW)
					ep.concatcstring(MCnoshadowstring, EC_COMMA, j++ == 0);
				if (decorations & WD_FORCETASKBAR)
					ep.concatcstring(MCforcetaskbarstring, EC_COMMA, j++ == 0);
			}
		}
		else
			ep.setstaticcstring(MCdefaultstring);
		break;
	case P_RECENT_NAMES:
		MCrecent->getnames(this, ep);
		break;
	case P_RECENT_CARDS:
		MCrecent->getlongids(this, ep);
		break;
	case P_ICONIC:
		ep.setboolean(getstate(CS_ICONIC));
		break;
	case P_START_UP_ICONIC:
		ep.setboolean(getflag(F_START_UP_ICONIC));
		break;
	case P_ICON:
		ep.setint(iconid);
		break;
	case P_OWNER:
		if (parent == NULL || MCdispatcher->ismainstack(this))
			ep.clear();
		else
			return parent->getprop(0, P_LONG_ID, ep, False);
		break;
	case P_MAIN_STACK:
		if (parent != NULL && !MCdispatcher->ismainstack(sptr))
			sptr = (MCStack *)parent;
		ep.setnameref_unsafe(sptr->getname());
		break;
	case P_SUBSTACKS:
		ep.clear();
		if (substacks != NULL)
		{
			MCStack *sptr = substacks;
			do
			{
				ep.concatnameref(sptr->getname(), EC_RETURN, sptr == substacks);
				sptr = sptr->next();
			}
			while (sptr != substacks);
		}
		break;
	// MW-2011-08-08: [[ Groups ]] Add 'sharedGroupNames' and 'sharedGroupIds' properties to
	//   stack.
	case P_BACKGROUND_NAMES:
	case P_BACKGROUND_IDS:
	case P_SHARED_GROUP_NAMES:
	case P_SHARED_GROUP_IDS:
		{
			ep.clear();
			MCControl *startptr = editing == NULL ? controls : savecontrols;
			MCControl *optr = startptr;
			if (optr != NULL)
			{
				bool t_want_background;
				t_want_background = which == P_BACKGROUND_NAMES || which == P_BACKGROUND_IDS;
				
				bool t_want_shared;
				t_want_shared = which == P_SHARED_GROUP_NAMES || which == P_SHARED_GROUP_IDS;

				MCExecPoint ep2(ep);
				do
				{
					// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
					MCGroup *t_group;
					t_group = nil;
					if (optr->gettype() == CT_GROUP)
						t_group = static_cast<MCGroup *>(optr);

					optr = optr -> next();

					if (t_group == nil)
						continue;

					if (t_want_background && !t_group -> isbackground())
						continue;

					if (t_want_shared && !t_group -> isshared())
						continue;

					Properties t_prop;
					if (which == P_BACKGROUND_NAMES || which == P_SHARED_GROUP_NAMES)
						t_prop = P_SHORT_NAME;
					else
						t_prop = P_SHORT_ID;

					t_group->getprop(0, t_prop, ep2, False);
					
					ep.concatmcstring(ep2.getsvalue(), EC_RETURN, j++ == 0);
				}
				while (optr != startptr);
			}
		}
		break;
	case P_CARD_IDS:
	case P_CARD_NAMES:
		ep.clear();
		if (cards != NULL)
		{
			MCExecPoint ep2(ep);
			MCCard *cptr = cards;
			do
			{
				if (which == P_CARD_NAMES)
					cptr->getprop(0, P_SHORT_NAME, ep2, False);
				else
					cptr->getprop(0, P_SHORT_ID, ep2, False);
				ep.concatmcstring(ep2.getsvalue(), EC_RETURN, j++ == 0);
				cptr = cptr->next();
			}
			while (cptr != cards);
		}
		break;
	case P_EDIT_BACKGROUND:
		ep.setboolean(editing != NULL);
		break;
	case P_EXTERNALS:
		if (externalfiles == NULL)
			ep.clear();
		else
			ep.setsvalue(externalfiles);
		break;
	case P_EXTERNAL_COMMANDS:
	case P_EXTERNAL_FUNCTIONS:
		if (m_externals != nil)
			m_externals -> ListHandlers(ep, which == P_EXTERNAL_COMMANDS ? HT_MESSAGE : HT_FUNCTION);
		else
		ep.clear();
		break;
	case P_EXTERNAL_PACKAGES:
		if (m_externals != nil)
			m_externals -> ListExternals(ep);
		else
		ep . clear();
		break;
	case P_MODE:
		ep.setint(getmode());
		break;
	case P_WM_PLACE:
		ep.setboolean(getflag(F_WM_PLACE));
		break;
	case P_WINDOW_ID:
		ep.setint(MCscreen->dtouint((Drawable)window));
		break;
	case P_PIXMAP_ID:
		ep.setint(0);
		break;
	case P_HC_ADDRESSING:
		ep.setboolean(getflag(F_HC_ADDRESSING));
		break;
	case P_HC_STACK:
		ep.setboolean(getflag(F_HC_STACK));
		break;
	case P_SIZE:
		ep.setstaticcstring(STACK_SIZE);
		break;
	case P_FREE_SIZE:
		ep.setstaticcstring(FREE_SIZE);
		break;
	case P_LOCK_SCREEN:
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		ep.setboolean(MCRedrawIsScreenLocked());
		break;
	case P_SHOW_BORDER:
	case P_BORDER_WIDTH:
	case P_ENABLED:
	case P_DISABLED:
	case P_3D:
	case P_LOCK_LOCATION:
	case P_TOOL_TIP:
	// MW-2012-03-13: [[ UnicodeToolTip ]] Stacks's don't have tooltips.
	case P_UNICODE_TOOL_TIP:
		MCeerror->add(EE_OBJECT_SETNOPROP, 0, 0);
		return ES_ERROR;
	case P_STACK_FILES:
		getstackfiles(ep);
		break;
	case P_MENU_BAR:
		ep.setnameref_unsafe(getmenubar());
		break;
	case P_EDIT_MENUS:
		ep.setboolean(getstate(CS_EDIT_MENUS));
		break;
	case P_VSCROLL:
		ep.setint(getscroll());
		break;
	case P_CHARSET:
#ifdef _MACOSX
		ep.setstaticcstring((state & CS_TRANSLATED) != 0 ? "ISO" : "MacOS");
#else
		ep.setstaticcstring((state & CS_TRANSLATED) != 0 ? "MacOS" : "ISO");
#endif
		break;
	case P_FORMAT_FOR_PRINTING:
		ep.setboolean(getflag(F_FORMAT_FOR_PRINTING));
		break;
	case P_LINK_COLOR:
	case P_LINK_HILITE_COLOR:
	case P_LINK_VISITED_COLOR:
	case P_UNDERLINE_LINKS:
		if (linkatts == NULL && !effective)
			ep.clear();
		else
		{
			Linkatts *la = getlinkatts();
			switch (which)
			{
			case P_LINK_COLOR:
				MCU_get_color(ep, la->colorname, la->color);
				break;
			case P_LINK_HILITE_COLOR:
				MCU_get_color(ep, la->hilitecolorname, la->hilitecolor);
				break;
			case P_LINK_VISITED_COLOR:
				MCU_get_color(ep, la->visitedcolorname, la->visitedcolor);
				break;
			case P_UNDERLINE_LINKS:
				ep.setboolean(la->underline);
				break;
			default:
				break;
			}
		}
		break;
	case P_WINDOW_SHAPE:
		ep.setint(windowshapeid);
		break;
	case P_SCREEN:
	{
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(rect);
		ep . setint(t_display -> index + 1);
	}
	break;
	case P_CURRENT_CARD:
		if (curcard != nil)
			return curcard -> names(P_SHORT_NAME, ep, parid);
		else
			ep . clear();
	break;
	case P_MODIFIED_MARK:
		ep . setboolean(getextendedstate(ECS_MODIFIED_MARK));
		break;
	
	// MW-2011-11-23: [[ AccelRender ]] Return the accelerated rendering state.
	case P_ACCELERATED_RENDERING:
		ep . setboolean(view_getacceleratedrendering());
		break;
	
	case P_COMPOSITOR_TYPE:
	{
		switch(view_getcompositortype())
		{
		case kMCTileCacheCompositorNone:
			ep . clear();
			break;
				
		case kMCTileCacheCompositorSoftware:
			ep . setstaticcstring("Software");
			break;
		case kMCTileCacheCompositorCoreGraphics:
			ep . setstaticcstring("CoreGraphics");
			break;
		case kMCTileCacheCompositorStaticOpenGL:
			ep . setstaticcstring("Static OpenGL");
			break;
		case kMCTileCacheCompositorDynamicOpenGL:
			ep . setstaticcstring("Dynamic OpenGL");
			break;
		default:
			assert(false);
			break;	
		}
	}
	break;
			
	case P_COMPOSITOR_TILE_SIZE:
		if (!view_getacceleratedrendering())
			ep . clear();
		else
			ep . setuint(view_getcompositortilesize());
	break;

	case P_COMPOSITOR_CACHE_LIMIT:
		if (!view_getacceleratedrendering())
			ep . clear();
		else
			ep . setuint(view_getcompositorcachelimit());
	break;
		
	// MW-2011-11-24: [[ UpdateScreen ]] Get the updateScreen properties.
	case P_DEFER_SCREEN_UPDATES:
		ep . setboolean(effective ? m_defer_updates && view_getacceleratedrendering() : m_defer_updates);
		break;
    // MERG-2014-06-02: [[ IgnoreMouseEvents ]] Get the ignoreMouseEvents property
    case P_IGNORE_MOUSE_EVENTS:
        ep.setboolean(getextendedstate(ECS_IGNORE_MOUSE_EVENTS));
        break;
#endif /* MCStack::getprop */
	default:
	{
		Exec_stat t_stat;
		t_stat = mode_getprop(parid, which, ep, kMCEmptyString, effective);
		if (t_stat == ES_NOT_HANDLED)
			return MCObject::getprop_legacy(parid, which, ep, effective, recursive);

		return t_stat;
	}
	break;
	}
	return ES_NORMAL;
}
#endif

#ifdef LEGACY_EXEC
Exec_stat MCStack::setprop_legacy(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty;
	Boolean bval;
	uint4 bflags;
	Boolean newlock;

	uint2 newsize;
	MCString data = ep.getsvalue();

	switch (which)
	{
#ifdef /* MCStack::setprop */ LEGACY_EXEC
	case P_FULLSCREEN:
		{
			if (!MCU_stob(data, bval))
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
				return ES_ERROR;
			}
			
			bool t_bval;
			t_bval = bval == True;

			bool v_changed = (getextendedstate(ECS_FULLSCREEN) != t_bval);
			if ( v_changed)
			{
				// IM-2014-01-16: [[ StackScale ]] Save the old rect here as view_setfullscreen() will update the stack rect
				if (t_bval)
					old_rect = rect;
				
				// IM-2014-05-27: [[ Bug 12321 ]] Move font purging to reopenstack() to avoid multiple redraws.

				setextendedstate(t_bval, ECS_FULLSCREEN);
				view_setfullscreen(t_bval);
			}
		}
	break;
		
	// IM-2013-09-23: [[ FullscreenMode ]] Add stack fullscreenMode property
	case P_FULLSCREENMODE:
		{
			// MW-2013-11-07: [[ Bug 11393 ]] Whether we need to reset fonts depends
			//   on both formatForPrinting and fullscreenmode (on Windows).

			bool t_ideal_layout;
			t_ideal_layout = getuseideallayout();

			MCStackFullscreenMode t_mode;
			if (!MCStackFullscreenModeFromString(ep.getcstring(), t_mode))
			{
				MCeerror->add(EE_STACK_BADFULLSCREENMODE, 0, 0, data);
				return ES_ERROR;
			}

			if (t_mode != view_getfullscreenmode())
				view_setfullscreenmode(t_mode);

			if ((t_ideal_layout != getuseideallayout()) && opened)
				purgefonts();
		}
		break;
			
	// IM-2014-01-07: [[ StackScale ]] Add stack scalefactor property
	case P_SCALE_FACTOR:
		{
			real64_t t_scale;
			
			Exec_stat t_stat;
			t_stat = ep.getreal8(t_scale, 0, 0, EE_PROPERTY_NAN);
			
			if (t_stat != ES_NORMAL)
				return t_stat;
			
			if (t_scale <= 0)
			{
				MCeerror->add(EE_STACK_BADSCALEFACTOR, 0, 0, t_scale);
				return ES_ERROR;
			}
			
			view_set_content_scale(t_scale);
		}
		break;
			
	case P_NAME:
		{
			// MW-2008-10-28: [[ ParentScripts ]] If this stack has its 'has parentscripts'
			//   flag set, temporarily store a copy of the current name.
			MCAutoNameRef t_old_name;
			if (getextendedstate(ECS_HAS_PARENTSCRIPTS))
				t_old_name . Clone(getname());

			// We don't allow ',' in stack names - so coerce to '_'.
			ep . replacechar(',', '_');

			// If the name is going to be empty, coerce to 'Untitled'.
			if (ep . isempty())
				ep . setstaticcstring(MCuntitledstring);

			if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
				return ES_ERROR;

			dirtywindowname();

			// MW-2008-10-28: [[ ParentScripts ]] If there is a copy of the old name, then
			//   it means this stack potentially has parent scripts...
			if (t_old_name != NULL)
			{
				// If the name has changed process...
				if (!hasname(t_old_name))
				{
					bool t_is_mainstack;
					t_is_mainstack = MCdispatcher -> ismainstack(this) == True;

					// First flush any references to parentScripts on this stack
					MCParentScript::FlushStack(this);
					setextendedstate(false, ECS_HAS_PARENTSCRIPTS);
				}
			}
		}
		return ES_NORMAL;
	case P_ID:
		uint4 newid;
		if (!MCU_stoui4(data, newid) || newid < obj_id)
		{
			MCeerror->add(EE_STACK_BADID, 0, 0, data);
			return ES_ERROR;
		}
		if (obj_id != newid)
		{
			uint4 oldid = obj_id;
			obj_id = newid;
			message_with_args(MCM_id_changed, oldid, obj_id);
		}
		break;
	case P_VISIBLE:

		dirty = True;

		if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		if (opened && (!(state & CS_IGNORE_CLOSE)) )
		{
			if (flags & F_VISIBLE)
			{
				dirtywindowname();
				openwindow(mode >= WM_PULLDOWN);
			}
			else
			{
				MCscreen->closewindow(window);
#ifdef X11
				//x11 will send propertynotify event which call ::close
				state |= CS_ISOPENING;
#endif

			}


			MCscreen->sync(getw());
		}
		return ES_NORMAL;
	case P_FORE_PIXEL:
	case P_BACK_PIXEL:
	case P_HILITE_PIXEL:
	case P_BORDER_PIXEL:
	case P_TOP_PIXEL:
	case P_BOTTOM_PIXEL:
	case P_SHADOW_PIXEL:
	case P_FOCUS_PIXEL:
	case P_FORE_COLOR:
	case P_BACK_COLOR:
	case P_HILITE_COLOR:
	case P_BORDER_COLOR:
	case P_TOP_COLOR:
	case P_BOTTOM_COLOR:
	case P_SHADOW_COLOR:
	case P_FOCUS_COLOR:
	case P_COLORS:
	case P_FORE_PATTERN:
	case P_BACK_PATTERN:
	case P_HILITE_PATTERN:
	case P_BORDER_PATTERN:
	case P_TOP_PATTERN:
	case P_BOTTOM_PATTERN:
	case P_SHADOW_PATTERN:
	case P_FOCUS_PATTERN:
	case P_TEXT_FONT:
	case P_TEXT_SIZE:
	case P_TEXT_STYLE:
	case P_TEXT_HEIGHT:
		if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		// MW-2011-08-18: [[ Redraw ]] This could be restricted to just children
		//   of this stack - but for now do the whole screen.
		MCRedrawDirtyScreen();
		return ES_NORMAL;
	case P_FILE_NAME:
		delete filename;
		// MW-2007-03-15: [[ Bug 616 ]] Throw an error if you try and set the filename of a substack
		if (!MCdispatcher->ismainstack(this))
		{
			MCeerror -> add(EE_STACK_NOTMAINSTACK, 0, 0);
			return ES_ERROR;
		}
		
		if (data.getlength() == 0)
			filename = NULL;
		else
			filename = data.clone();
		return ES_NORMAL;
	case P_SAVE_COMPRESSED:
		break;
	case P_CANT_ABORT:
		if (!MCU_matchflags(data, flags, F_CANT_ABORT, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_CANT_DELETE:
		if (!MCU_matchflags(data, flags, F_S_CANT_DELETE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_STYLE:
		flags &= ~F_STYLE;
		if (data == MCsheetstring)
			flags |= WM_MODAL - WM_TOP_LEVEL_LOCKED;
		else
			if (data == MCmodalstring || data == MCdialogstring
			        || data == MCmovablestring)
				flags |= WM_MODAL - WM_TOP_LEVEL_LOCKED;
			else
			{
				if (data == MCmodelessstring)
					flags |= WM_MODELESS - WM_TOP_LEVEL_LOCKED;
				else if (data == MCpalettestring || data == MCshadowstring || data == MCroundrectstring)
						flags |= WM_PALETTE - WM_TOP_LEVEL_LOCKED;
			}

		if (opened)
		{
			mode = WM_TOP_LEVEL;
			reopenwindow();
		}
		break;

	case P_CANT_MODIFY:
		if (!MCU_matchflags(data, flags, F_CANT_MODIFY, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
		{
			if (!iskeyed())
			{
				flags ^= F_CANT_MODIFY;
				MCeerror->add(EE_STACK_NOKEY, 0, 0);
				return ES_ERROR;
			}
			if (mode == WM_TOP_LEVEL || mode == WM_TOP_LEVEL_LOCKED)
				if (flags & F_CANT_MODIFY || !MCdispatcher->cut(True))
					mode = WM_TOP_LEVEL_LOCKED;
				else
					mode = WM_TOP_LEVEL;
			stopedit();
			dirtywindowname();
			resetcursor(True);
			MCstacks->top(this);
		}
		break;
	case P_CANT_PEEK:
		break;
	case P_DYNAMIC_PATHS:
		if (!MCU_matchflags(data, flags, F_DYNAMIC_PATHS, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_DESTROY_STACK:
		if (!MCU_matchflags(data, flags, F_DESTROY_STACK, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_DESTROY_WINDOW:
		if (!MCU_matchflags(data, flags, F_DESTROY_WINDOW, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_ALWAYS_BUFFER:
		if (!MCU_matchflags(data, flags, F_ALWAYS_BUFFER, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_LABEL:
	case P_UNICODE_LABEL:
		// MW-2007-07-06: [[ Bug 3226 ]] Updated to take into account 'title' being
		//   stored as a UTF-8 string.
		delete title;
		title = NULL;
		if (data != MCnullmcstring)
		{
			if (which == P_UNICODE_LABEL)
				ep . utf16toutf8();
			else
				ep . nativetoutf8();

			title = ep.getsvalue().clone();
			flags |= F_TITLE;
		}
		else
			flags &= ~F_TITLE;
		dirtywindowname();
		break;
	case P_CLOSE_BOX:
	case P_ZOOM_BOX:
	case P_DRAGGABLE:
	case P_MAXIMIZE_BOX:
	case P_SHADOW:
	case P_SYSTEM_WINDOW:
	case P_MINIMIZE_BOX:
	case P_METAL:
	case P_LIVE_RESIZING:
	case P_COLLAPSE_BOX:
		if (!MCU_stob(data, bval))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (!(flags & F_DECORATIONS))
			decorations = WD_MENU | WD_TITLE | WD_MINIMIZE | WD_MAXIMIZE | WD_CLOSE;
		flags |= F_DECORATIONS;
		switch (which)
		{
		case P_CLOSE_BOX:
			bflags = WD_CLOSE;
			break;
		case P_COLLAPSE_BOX:
		case P_MINIMIZE_BOX:
			bflags = WD_MINIMIZE;
			break;
		case P_ZOOM_BOX:
		case P_MAXIMIZE_BOX:
			bflags = WD_MAXIMIZE;
			break;
		case P_LIVE_RESIZING:
			bflags = WD_LIVERESIZING;
			break;
		case P_SYSTEM_WINDOW:
			bflags = WD_UTILITY;
			break;
		case P_METAL:
			bflags = WD_METAL;
			break;
		case P_SHADOW:
			bval = !bval;
			bflags = WD_NOSHADOW;
			break;
		default:
			bflags = 0;
			break;
		}
		if (bval)
			decorations |= bflags;
		else
			decorations &= ~bflags;
		if (opened)
			reopenwindow();
		break;
	case P_RESIZABLE:
		if (!MCU_matchflags(data, flags, F_RESIZABLE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
			reopenwindow();
		break;
	case P_MIN_WIDTH:
	case P_MAX_WIDTH:
	case P_MIN_HEIGHT:
	case P_MAX_HEIGHT:
		if (!MCU_stoui2(data, newsize))
		{
			MCeerror->add
			(EE_STACK_MINMAXNAN, 0, 0, data);
			return ES_ERROR;
		}
		switch (which)
		{
		case P_MIN_WIDTH:
			minwidth = newsize;
			if (minwidth > maxwidth)
				maxwidth = minwidth;
			break;
		case P_MAX_WIDTH:
			maxwidth = newsize;
			if (minwidth > maxwidth)
				minwidth = maxwidth;
			break;
		case P_MIN_HEIGHT:
			minheight = newsize;
			if (minheight > maxheight)
				maxheight = minheight;
			break;
		case P_MAX_HEIGHT:
			maxheight = newsize;
			if (minheight > maxheight)
				minheight = maxheight;
			break;
		default:
			break;
		}
		if (opened)
		{
			sethints();
			setgeom();
		}
		break;
	case P_ICON:
		{
			uint4 newiconid;
			if (!MCU_stoui4(data, newiconid))
			{
				MCeerror->add(EE_STACK_ICONNAN, 0, 0, data);
				return ES_ERROR;
			}
			if (opened && iconid != newiconid)
			{
				iconid = newiconid;
				if (state & CS_ICONIC)
					redrawicon();
			}
		}
		break;
	case P_ICONIC:
		{
			uint4 newstate = state;
			if (!MCU_matchflags(data, newstate, CS_ICONIC, dirty))
			{
				MCeerror->add
				(EE_OBJECT_NAB, 0, 0, data);
				return ES_ERROR;
			}
			//SMR 1261 don't set state to allow iconify() to take care of housekeeping
			// need to check X11 to make sure MCStack::iconify() (in stack2.cpp) is called when this prop is set
			if (dirty && opened)
			{
				sethints();
				if (newstate & CS_ICONIC)
					MCscreen->iconifywindow(window);
				else
				{
					MCscreen->uniconifywindow(window);
				}
			}
		}
		break;
	case P_DECORATIONS:
		{
			uint2 olddec = decorations;
			uint4 oldflags = flags;
			decorations = WD_CLEAR;
			if (data == MCdefaultstring)
				flags &= ~F_DECORATIONS;
			else
			{
				flags |= F_DECORATIONS;
				uint2 i1;
				if (MCU_stoui2(data, i1))
					decorations = i1 | WD_WDEF;
				else
				{
					uint4 l = data.getlength();
					const char *sptr = data.getstring();
					MCU_skip_spaces(sptr, l);
					if (decorations & WD_WDEF)
						ep.setint(decorations & ~WD_WDEF);
					else
					{
						while (l != 0)
						{
							const char *startptr = sptr;
							if (!MCU_strchr(sptr, l, ','))
							{
								sptr += l;
								l = 0;
							}
							MCString tdata(startptr, sptr - startptr);
							MCU_skip_char(sptr, l);
							MCU_skip_spaces(sptr, l);
							if (tdata == MCtitlestring)
							{
								decorations |= WD_TITLE;
								continue;
							}
							if (tdata == MCmenustring)
							{
								decorations |= WD_MENU | WD_TITLE;
								continue;
							}
							if (tdata == MCminimizestring)
							{
								decorations |= WD_MINIMIZE | WD_TITLE;
								continue;
							}
							if (tdata == MCmaximizestring)
							{
								decorations |= WD_MAXIMIZE | WD_TITLE;
								continue;
							}
							if (tdata == MCclosestring)
							{
								decorations |= WD_CLOSE | WD_TITLE;
								continue;
							}
							if (tdata == MCmetalstring)
							{
								decorations |= WD_METAL; //metal can not have title
								continue;
							}
							if (tdata == MCutilitystring)
							{
								decorations |= WD_UTILITY;
								continue;
							}
							if (tdata == MCnoshadowstring)
							{
								decorations |= WD_NOSHADOW;
								continue;
							}
							if (tdata == MCforcetaskbarstring)
							{
								decorations |= WD_FORCETASKBAR;
								continue;
							}
							MCeerror->add(EE_STACK_BADDECORATION, 0, 0, data);
							return ES_ERROR;
						}
					}
				}

			}
			if (flags != oldflags || decorations != olddec)
				if (opened)
					reopenwindow();
				else
				{
					if (window != NULL)
					{
						stop_externals();
						MCscreen->destroywindow(window);
						MCValueAssign(titlestring, kMCEmptyString);
					}
				}
		}
		break;
	case P_START_UP_ICONIC:
		if (!MCU_matchflags(data, flags, F_START_UP_ICONIC, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
			sethints();
		break;
	case P_EDIT_BACKGROUND:
		Boolean edit;
		if (!MCU_stob(data, edit) || !opened)
			return ES_ERROR;
		if (edit)
		{
			MCGroup *gptr = (MCGroup *)curcard->getchild(CT_FIRST, MCnullmcstring,
			                CT_GROUP, CT_UNDEFINED);
			if (gptr == NULL)
				gptr = getbackground(CT_FIRST, MCnullmcstring, CT_GROUP);
			if (gptr != NULL)
				startedit(gptr);
		}
		else
			stopedit();
		dirtywindowname();
		break;
	case P_MAIN_STACK:
		{
			MCStack *stackptr = NULL;
			char *sname = data.clone();

			if ((stackptr = MCdispatcher->findstackname(sname)) == NULL)
			{
				MCeerror->add(EE_STACK_NOMAINSTACK, 0, 0, data);
				delete sname;
				return ES_ERROR;
			}
			delete sname;

			if (stackptr != this && !MCdispatcher->ismainstack(stackptr))
			{
				MCeerror->add(EE_STACK_NOTMAINSTACK, 0, 0, data);
				return ES_ERROR;
			}
			
			if (parent != NULL && this != MCdispatcher->gethome() && (substacks == NULL || stackptr == this))
			{
				bool t_this_is_mainstack;
				t_this_is_mainstack = MCdispatcher -> ismainstack(this) == True;

				// OK-2008-04-10 : Added parameters to mainstackChanged message to specify the new
				// and old mainstack names.
				MCObject *t_old_stackptr;
				if (t_this_is_mainstack)
					t_old_stackptr = this;
				else
					t_old_stackptr = parent;

				//   If this was previously a mainstack, then it will be referenced by (name, NULL).
				//   If this was previously a substack, it will have been referenced by (name, old_mainstack).

				if (t_this_is_mainstack)
					MCdispatcher->removestack(this);
				else
				{
					MCStack *pstack = (MCStack *)parent;
					remove(pstack->substacks);
					// MW-2012-09-07: [[ Bug 10372 ]] If the stack no longer has substacks, then 
					//   make sure we undo the extraopen.
					if (pstack->substacks == NULL)
						pstack -> extraclose(true);
				}

				if (stackptr == this)
				{
					MCdispatcher->appendstack(this);
					parent = MCdispatcher->gethome();
				}
				else
				{
					// MW-2012-09-07: [[ Bug 10372 ]] If the stack doesn't have substacks, then
					//   make sure we apply the extraopen (as it's about to have some!).
					if (stackptr -> substacks == NULL)
						stackptr -> extraopen(true);
					appendto(stackptr->substacks);
					parent = stackptr;
				}

				// OK-2008-04-10 : Added parameters to mainstackChanged message to specify the new
				// and old mainstack names.
				message_with_args(MCM_main_stack_changed, t_old_stackptr -> getname(), stackptr -> getname());
			}
			else
			{
				MCeerror->add(EE_STACK_CANTSETMAINSTACK, 0, 0, data);
				return ES_ERROR;
			}
		}
		break;
	case P_SUBSTACKS:
		{
			if (!MCdispatcher->ismainstack(this))
			{
				MCeerror->add
				(EE_STACK_NOTMAINSTACK, 0, 0, data);
				return ES_ERROR;
			}
			char *subs = data.clone();
			if (resubstack(subs) != ES_NORMAL)
			{
				delete subs;
				return ES_ERROR;
			}
			delete subs;
		}
		break;
	case P_EXTERNALS:
		delete externalfiles;
		if (data != MCnullmcstring)
			externalfiles = data.clone();
		else
			externalfiles = NULL;
		break;
	case P_WM_PLACE:
		if (!MCU_matchflags(data, flags, F_WM_PLACE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_HC_ADDRESSING:
		if (!MCU_matchflags(data, flags, F_HC_ADDRESSING, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_LOCK_SCREEN:
		if (!MCU_stob(data, newlock))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		if (newlock)
			MCRedrawLockScreen();
		else
			MCRedrawUnlockScreenWithEffects();
		break;
	case P_SHOW_BORDER:
	case P_BORDER_WIDTH:
	case P_ENABLED:
	case P_DISABLED:
	case P_3D:
	case P_LOCK_LOCATION:
	case P_TOOL_TIP:
	// MW-2012-03-13: [[ UnicodeToolTip ]] Stacks don't have tooltips.
	case P_UNICODE_TOOL_TIP:
	case P_LAYER:
		MCeerror->add
		(EE_OBJECT_SETNOPROP, 0, 0);
		return ES_ERROR;
	case P_STACK_FILES:
		setstackfiles(data);
		if (nstackfiles != 0)
			flags |= F_STACK_FILES;
		else
			flags &= ~F_STACK_FILES;
		break;
	case P_MENU_BAR:
	{
		MCAutoNameRef t_new_menubar;
		/* UNCHECKED */ ep . copyasnameref(t_new_menubar);
		if (!MCNameIsEqualTo(getmenubar(), t_new_menubar, kMCCompareCaseless))
		{
			MCNameDelete(_menubar);
			/* UNCHECKED */ MCNameClone(t_new_menubar, _menubar);
			if (!hasmenubar())
				flags &= ~F_MENU_BAR;
			else
				flags |= F_MENU_BAR;
			if (opened)
			{
				setgeom();
				updatemenubar();

				// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
				dirtyall();
			}
		}
	}
	break;
	case P_EDIT_MENUS:
		if (!MCU_matchflags(data, state, CS_EDIT_MENUS, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
		{
			setgeom();
			updatemenubar();
		}
		break;
	case P_FORMAT_FOR_PRINTING:
		{
			// MW-2013-11-07: [[ Bug 11393 ]] Whether we need to reset fonts depends
			//   on both formatForPrinting and fullscreenmode (on Windows).

			bool t_ideal_layout;
			t_ideal_layout = getuseideallayout();

			if (!MCU_matchflags(data, flags, F_FORMAT_FOR_PRINTING, dirty))
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
				return ES_ERROR;
			}

			if ((getuseideallayout() != t_ideal_layout) && opened)
				purgefonts();
		}
		break;
	case P_LINK_COLOR:
	case P_LINK_HILITE_COLOR:
	case P_LINK_VISITED_COLOR:
	case P_UNDERLINE_LINKS:
		{
			Exec_stat stat = ES_NORMAL;
			if (ep.getsvalue().getlength() == 0)
			{
				if (linkatts != NULL)
				{
					delete linkatts->colorname;
					delete linkatts->hilitecolorname;
					delete linkatts->visitedcolorname;
					delete linkatts;
					linkatts = NULL;
				}
			}
			else
			{
				if (linkatts == NULL)
				{
					linkatts = new Linkatts;
					memcpy(linkatts, &MClinkatts, sizeof(Linkatts));
					linkatts->colorname = strclone(MClinkatts.colorname);
					linkatts->hilitecolorname = strclone(MClinkatts.hilitecolorname);
					linkatts->visitedcolorname = strclone(MClinkatts.visitedcolorname);
					MCscreen->alloccolor(linkatts->color);
					MCscreen->alloccolor(linkatts->hilitecolor);
					MCscreen->alloccolor(linkatts->visitedcolor);
				}
				switch (which)
				{
				case P_LINK_COLOR:
					stat = MCU_change_color(linkatts->color, linkatts->colorname, ep, 0, 0);
					break;
				case P_LINK_HILITE_COLOR:
					stat = MCU_change_color(linkatts->hilitecolor,
					                        linkatts->hilitecolorname, ep, 0, 0);
					break;
				case P_LINK_VISITED_COLOR:
					stat = MCU_change_color(linkatts->visitedcolor,
					                        linkatts->visitedcolorname, ep, 0, 0);
					break;
				case P_UNDERLINE_LINKS:
					stat = ep.getboolean(linkatts->underline, 0, 0, EE_PROPERTY_NAB);
					break;
				default:
					break;
				}
			}
			
			// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
			dirtyall();
			return stat;
		}
		break;
	case P_WINDOW_SHAPE:
		{
			uint4 newwinshapeid;
			// unless we opened the window ourselves, we can't change the window shape
			if (!MCU_stoui4(data, newwinshapeid))
			{
				MCeerror->add(EE_STACK_ICONNAN, 0, 0, data);
				return ES_ERROR;
			}
			windowshapeid = newwinshapeid;
			if (windowshapeid)
			{
				// MW-2011-10-08: [[ Bug 4198 ]] Make sure we preserve the shadow status of the stack.
				decorations = WD_SHAPE | (decorations & WD_NOSHADOW);
				flags |= F_DECORATIONS;
				
#if defined(_DESKTOP)
				// MW-2004-04-27: [[Deep Masks]] If a window already has a mask, replace it now to avoid flicker
				if (m_window_shape != NULL)
				{
					// IM-2014-10-22: [[ Bug 13746 ]] use common loadwindowshape() method
					loadwindowshape();
					break;
				}
#endif
			}
			else
			{
				decorations &= ~WD_SHAPE;
				flags &= ~F_DECORATIONS;
			}
			
			if (opened)
			{
				reopenwindow();
				
#if defined(_DESKTOP)
				// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
				if (m_window_shape != NULL)
					dirtyall();
#endif
			}
		}
		break;
	case P_BLEND_LEVEL:
		old_blendlevel = blendlevel;
		if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		
		// MW-2011-11-03: [[ Bug 9852 ]] Make sure an update is scheduled to sync the
		//   opacity.
		MCRedrawScheduleUpdateForStack(this);
	break;
	case P_CURRENT_CARD:
	{
		MCCard *t_card;
		t_card = getchild(CT_EXPRESSION, ep . getsvalue(), CT_CARD);
		if (t_card != NULL)
			setcard(t_card, False, False);
	}
	break;
	case P_MODIFIED_MARK:
		if (!MCU_matchflags(data, f_extended_state, ECS_MODIFIED_MARK, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
			updatemodifiedmark();
		break;
	
	// MW-2011-11-23: [[ AccelRender ]] Configure accelerated rendering.
	case P_ACCELERATED_RENDERING:
	{
		Boolean t_accel_render;
		if (!MCU_stob(data, t_accel_render))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		view_setacceleratedrendering(t_accel_render == True);
	}
	break;
	
	// MW-2011-09-10: [[ TileCache ]] Configure the compositor to use.
	case P_COMPOSITOR_TYPE:
	{
		MCTileCacheCompositorType t_type;
		if (data == MCnullmcstring || data == "none")
			t_type = kMCTileCacheCompositorNone;
		else if (data == "software")
			t_type = kMCTileCacheCompositorSoftware;
		else if (data == "coregraphics")
			t_type = kMCTileCacheCompositorCoreGraphics;
		else if (data == "opengl")
		{
			if (MCTileCacheSupportsCompositor(kMCTileCacheCompositorDynamicOpenGL))
				t_type = kMCTileCacheCompositorDynamicOpenGL;
			else
				t_type = kMCTileCacheCompositorStaticOpenGL;
		}
		else if (data == "static opengl")
			t_type = kMCTileCacheCompositorStaticOpenGL;
		else if (data == "dynamic opengl")
			t_type = kMCTileCacheCompositorDynamicOpenGL;
		else
		{
			MCeerror -> add(EE_COMPOSITOR_UNKNOWNTYPE, 0, 0, data);
			return ES_ERROR;
		}
		
		if (!MCTileCacheSupportsCompositor(t_type))
		{
			MCeerror -> add(EE_COMPOSITOR_NOTSUPPORTED, 0, 0, data);
			return ES_ERROR;
		}
		
		view_setcompositortype(t_type);
	}
	break;
			
	// MW-2011-09-10: [[ TileCache ]] Set the maximum number of bytes to use for the tile cache.
	case P_COMPOSITOR_CACHE_LIMIT:
	{
		uint32_t t_new_cachelimit;	
		if (!MCU_stoui4(data, t_new_cachelimit))
		{
			MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		view_setcompositorcachelimit(t_new_cachelimit);
	}
	break;

	// MW-2011-09-10: [[ TileCache ]] Set the size of tile to use for the tile cache.
	case P_COMPOSITOR_TILE_SIZE:
	{
		uint32_t t_new_tilesize;	
		if (!MCU_stoui4(data, t_new_tilesize))
		{
			MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		if (!view_isvalidcompositortilesize(t_new_tilesize))
		{
			MCeerror->add(EE_COMPOSITOR_INVALIDTILESIZE, 0, 0, data);
			return ES_ERROR;
		}
		view_setcompositortilesize(t_new_tilesize);
	}
	break;

	// MW-2011-11-24: [[ UpdateScreen ]] Configure the updateScreen properties.
	case P_DEFER_SCREEN_UPDATES:
	{
		Boolean t_defer_updates;
		if (!MCU_stob(data, t_defer_updates))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}

		m_defer_updates = (t_defer_updates == True);
	}
	break;
            
    case P_FORE_PIXEL:
    case P_BACK_PIXEL:
    case P_HILITE_PIXEL:
    case P_BORDER_PIXEL:
    case P_TOP_PIXEL:
    case P_BOTTOM_PIXEL:
    case P_SHADOW_PIXEL:
    case P_FOCUS_PIXEL:
    case P_FORE_COLOR:
    case P_BACK_COLOR:
    case P_HILITE_COLOR:
    case P_BORDER_COLOR:
    case P_TOP_COLOR:
    case P_BOTTOM_COLOR:
    case P_SHADOW_COLOR:
    case P_FOCUS_COLOR:
    case P_COLORS:
    case P_FORE_PATTERN:
    case P_BACK_PATTERN:
    case P_HILITE_PATTERN:
    case P_BORDER_PATTERN:
    case P_TOP_PATTERN:
    case P_BOTTOM_PATTERN:
    case P_SHADOW_PATTERN:
    case P_FOCUS_PATTERN:
    case P_TEXT_FONT:
    case P_TEXT_SIZE:
    case P_TEXT_STYLE:
    case P_TEXT_HEIGHT:
        if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
            return ES_ERROR;
        // MW-2011-08-18: [[ Redraw ]] This could be restricted to just children
        //   of this stack - but for now do the whole screen.
        MCRedrawDirtyScreen();
        return ES_NORMAL;
    
    // MERG-2014-06-02: [[ IgnoreMouseEvents ]] Set the ignoreMouseEvents property
    case P_IGNORE_MOUSE_EVENTS:
    {
        if (!MCU_matchflags(data, f_extended_state, ECS_IGNORE_MOUSE_EVENTS, dirty))
        {
            MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
            return ES_ERROR;
        }
        if (dirty && opened)
            updateignoremouseevents();
    }
    break;
   
#endif /* MCStack::setprop */
	default:
	{
		Exec_stat t_stat;
		t_stat = mode_setprop(parid, which, ep, kMCEmptyString, kMCEmptyString, effective);
		if (t_stat == ES_NOT_HANDLED)
			return MCObject::setprop_legacy(parid, which, ep, effective);

		return t_stat;
	}
	}
	return ES_NORMAL;
}
#endif


Boolean MCStack::del()
{
	// MW-2012-10-26: [[ Bug 9918 ]] If 'cantDelete' is set, then don't delete the stack. Also
	//   make sure we throw an error.
	if (parent == NULL || scriptdepth != 0 || flags & F_S_CANT_DELETE)
	{
		MCeerror->add(EE_OBJECT_CANTREMOVE, 0, 0);
		return False;
	}
	if (MCdispatcher->gethome() == this)
		return False;
	
    setstate(CS_DELETE_STACK, True);
    
	if (opened)
	{
		// MW-2007-04-22: [[ Bug 4203 ]] Coerce the flags to include F_DESTROY_WINDOW to ensure we don't
		//   get system resource accumulation in a tight create/destroy loop.
		flags |= F_DESTROY_WINDOW;
		close();
	}

	if (curcard != NULL)
		curcard->message(MCM_delete_stack);
	else
		if (cards != NULL)
			cards->message(MCM_delete_stack);
	
    notifyattachments(kMCStackAttachmentEventDeleting);
    
	if (MCdispatcher->ismainstack(this))
	{
		MCdispatcher->removestack(this);
	}
	else
	{
		remove(((MCStack *)parent)->substacks);
		// MW-2012-09-07: [[ Bug 10372 ]] If the stack no longer has substacks then make sure we
		//   undo the extraopen.
		if (((MCStack *)parent) -> substacks == NULL)
			((MCStack *)parent) -> extraclose(true);
	}

	if (MCstaticdefaultstackptr == this)
		MCstaticdefaultstackptr = MCtopstackptr;
	if (MCdefaultstackptr == this)
		MCdefaultstackptr = MCstaticdefaultstackptr;

	// MW-2008-10-28: [[ ParentScripts ]] If this stack has its 'has parentscripts'
	//   flag set, flush the parentscripts table.
	if (getextendedstate(ECS_HAS_PARENTSCRIPTS))
		MCParentScript::FlushStack(this);
    
    // MCObject now does things on del(), so we must make sure we finish by
    // calling its implementation.
    return MCObject::del();
}

void MCStack::paste(void)
{
	if (MCdispatcher -> findstackname(getname()) != NULL)
	{
		unsigned int t_index;
		t_index = 1;

		MCStringRef t_old_name;
		t_old_name = !isunnamed() ? MCNameGetString(getname()) : MCSTR("Unnamed");

		MCNameRef t_name;
		t_name = MCValueRetain(kMCEmptyName);
		
		for(;;)
		{
			MCAutoStringRef t_new_name;
			if (t_index == 1)
				/* UNCHECKED */ MCStringFormat(&t_new_name, "Copy of %@", t_old_name);
			else
				/* UNCHECKED */ MCStringFormat(&t_new_name, "Copy (%d) of %@", t_index, t_old_name);
			
			MCValueRelease(t_name);
			/* UNCHECKED */ MCNameCreate(*t_new_name, t_name);
			
			if (MCdispatcher -> findstackname(t_name) == NULL)
				break;
			t_index += 1;
		}
		
		setname(t_name);
		MCValueRelease(t_name);
	}

	// MW-2007-12-11: [[ Bug 5441 ]] When we paste a stack, it should be parented to the home
	//   stack, just like when we create a whole new stack.
	parent = MCdispatcher -> gethome();
	MCdispatcher -> appendstack(this);

	positionrel(MCdefaultstackptr->rect, OP_CENTER, OP_MIDDLE);
	open();
}

MCStack *MCStack::getstack()
{
	return this;
}

Exec_stat MCStack::handle(Handler_type htype, MCNameRef message, MCParameter *params, MCObject *passing_object)
{
	if (!opened)
	{
		if (window == NULL && !MCNameIsEqualTo(message, MCM_start_up, kMCCompareCaseless)
#ifdef _MACOSX
		        && !(state & CS_DELETE_STACK))
#else
				&& !MCStringIsEmpty(externalfiles) && !(state & CS_DELETE_STACK))
#endif
		{
			// IM-2014-01-16: [[ StackScale ]] Ensure view has the current stack rect
			view_setstackviewport(rect);
			realize();
		}
	}

	// MW-2009-01-28: [[ Bug ]] Card and stack parentScripts don't work.
	// First attempt to handle the message with the script and parentScript of this
	// object.
	Exec_stat stat;
	stat = ES_NOT_HANDLED;
	if (!MCdynamicpath || MCdynamiccard->getparent() != this)
		stat = handleself(htype, message, params);
	else if (passing_object != nil)
	{
		// MW-2011-06-20:  If dynamic path is enabled, and this stack is the parent
		//   of the dynamic card then instead of passing through this stack, we pass
		//   through the dynamic card (which will then pass through this stack).
		MCdynamicpath = False;
		stat = MCdynamiccard->handle(htype, message, params, this);
		return stat;
	}

	if (((passing_object != nil && stat == ES_PASS) || stat == ES_NOT_HANDLED) && m_externals != nil)
	{
		Exec_stat oldstat = stat;
		stat = m_externals -> Handle(this, htype, message, params);
		if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
	}

	// MW-2011-06-30: Cleanup of message path - this clause handles the transition
	//   through the home stack to dispatcher.
	if (passing_object != nil && (stat == ES_PASS || stat == ES_NOT_HANDLED) && parent != NULL)
	{
		Exec_stat oldstat = stat;
		if (MCModeHasHomeStack() || parent != MCdispatcher -> gethome() || !MCdispatcher->ismainstack(this))
		{
			// If the engine variant has a home stack (server / development) then we pass
			// to parent.
			// Otherwise we are one of the following:
			//   1) a substack of a stack which is not the home stack (parent != gethome()).
			//   2) the home stack (parent != gethome())
			//   3) a substack of the home stack (parent == gethome(), ismainstack() == false)
			// In all cases we want to pass to parent.
			stat = parent->handle(htype, message, params, this);
		}
		else if (parent->getparent() != NULL)
		{
			// It should be impossible for parent -> getparent() to be NULL. In any case, this
			// should always be MCdispatcher so we want to do this.
			stat = parent->getparent()->handle(htype, message, params, this);
		}
		if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
	}

	if (stat == ES_ERROR && MCerrorptr == NULL)
		MCerrorptr = this;

	return stat;
}

void MCStack::recompute()
{
	if (curcard != NULL)
		curcard->recompute();
}

void MCStack::toolchanged(Tool p_new_tool)
{
    if (curcard != NULL)
        curcard->toolchanged(p_new_tool);
}

///////////////////////////////////////////////////////////////////////////////

void MCStack::loadexternals(void)
{
    notifyattachments(kMCStackAttachmentEventRealizing);
    
	if (MCStringIsEmpty(externalfiles) || m_externals != NULL || !MCSecureModeCanAccessExternal())
		return;

	m_externals = new MCExternalHandlerList;

	MCAutoArrayRef t_array;
	/* UNCHECKED */ MCStringSplit(externalfiles, MCSTR("\n"), nil, kMCStringOptionCompareExact, &t_array);
	uindex_t t_count;
	t_count = MCArrayGetCount(*t_array);
	for (uindex_t i = 0; i < t_count; i++)
	{
		MCValueRef t_val;
		/* UNCHECKED */ MCArrayFetchValueAtIndex(*t_array, i + 1, t_val);
		m_externals->Load((MCStringRef)t_val);
	}

	// If the handler list is empty, then delete the object - thus preventing
	// its use in MCStack::handle.
	if (m_externals -> IsEmpty())
	{
		delete m_externals;
		m_externals = nil;
}
}

void MCStack::unloadexternals(void)
{
    notifyattachments(kMCStackAttachmentEventUnrealizing);
    
	if (m_externals == NULL)
		return;

	delete m_externals;
	m_externals = NULL;
}

bool MCStack::resolve_relative_path(MCStringRef p_path, MCStringRef& r_resolved)
{
    if (MCStringIsEmpty(p_path))
    {
        // PM-2015-01-26: [[ Bug 14437 ]] If we clear the player filename in the property inspector or by script, make sure we resolve empty, to prevent a crash
        r_resolved = MCValueRetain(kMCEmptyString);
		return false;
    }

	MCStringRef t_stack_filename;
	t_stack_filename = getfilename();
	if (MCStringIsEmpty(t_stack_filename))
	{
		MCStack *t_parent_stack;
		t_parent_stack = static_cast<MCStack *>(getparent());
		if (t_parent_stack != NULL)
			t_stack_filename = t_parent_stack -> getfilename();
	}

	if (!MCStringIsEmpty(t_stack_filename))
	{
		uindex_t t_slash;
        if (MCStringLastIndexOfChar(t_stack_filename, '/', UINDEX_MAX, kMCCompareExact, t_slash))
        {
            MCAutoStringRef t_new_filename;
            MCStringCreateMutable(0, &t_new_filename);
            /* UNCHECKED */ MCStringAppendSubstring(*t_new_filename, t_stack_filename, MCRangeMake(0, t_slash + 1));
                
            // If the relative path begins with "./" or ".\", we must remove this, otherwise
			// certain system calls will get confused by the path.
			if (MCStringBeginsWith(p_path, MCSTR("./"), kMCCompareExact) || MCStringBeginsWith(p_path, MCSTR(".\\"), kMCCompareExact))
				/* UNCHECKED */ MCStringAppendSubstring(*t_new_filename, p_path, MCRangeMake(2, MCStringGetLength(p_path) - 2));
			else
				/* UNCHECKED */ MCStringAppend(*t_new_filename, p_path);
            
			r_resolved = MCValueRetain(*t_new_filename);
			return true;
		}
	}
    
    return false;
}

// PM-2015-01-26: [[ Bug 14435 ]] Make possible to set the filename using a relative path to the default folder
bool MCStack::resolve_relative_path_to_default_folder(MCStringRef p_path, MCStringRef &r_resolved)
{
    if (MCStringIsEmpty(p_path))
		return false;
	
	// If the relative path begins with "./" or ".\", we must remove this, otherwise
    // certain system calls will get confused by the path.
    uindex_t t_start_index;
    MCAutoStringRef t_cur_dir;

    if (MCStringBeginsWith(p_path, MCSTR("./"), kMCCompareExact)
            || MCStringBeginsWith(p_path, MCSTR(".\\"), kMCCompareExact))
        t_start_index = 2;
    else
        t_start_index = 0;
	
    MCS_getcurdir(&t_cur_dir);

    MCRange t_range;
    t_range = MCRangeMake(t_start_index, MCStringGetLength(p_path) - t_start_index);
    return MCStringFormat(r_resolved, "%@/%*@", *t_cur_dir, &t_range, p_path);
}

// OK-2009-01-09: [[Bug 1161]]
// This function will attempt to resolve the specified filename relative to the stack
// and will either return an absolute path if the filename was found relative to the stack,
// or a copy of the original buffer. The returned buffer should be freed by the caller.
bool MCStack::resolve_filename(MCStringRef filename, MCStringRef& r_resolved)
{
    MCAutoStringRef t_filename;
    if (resolve_relative_path(filename, &t_filename))
    {
        if (MCS_exists(*t_filename, True))
        {
            r_resolved = MCValueRetain(*t_filename);
            return true;
        }
    }
    
	r_resolved = MCValueRetain(filename);
	return true;
}

MCRectangle MCStack::recttoroot(const MCRectangle& p_rect)
{
	// IM-2014-01-07: [[ StackScale ]] Use stack->root transform to convert coords
	return MCRectangleGetTransformedBounds(p_rect, getroottransform());
}

MCRectangle MCStack::rectfromroot(const MCRectangle& p_rect)
{
	// IM-2014-01-07: [[ StackScale ]] Use root->stack transform to convert coords
	return MCRectangleGetTransformedBounds(p_rect, MCGAffineTransformInvert(getroottransform()));
}

// MW-2011-09-20: [[ Collision ]] The stack's shape is its rect. At some point it
//   might be need to update this to include the windowmask...
bool MCStack::lockshape(MCObjectShape& r_shape)
{
	r_shape . type = kMCObjectShapeRectangle;
	
	// Object shapes are in card-relative co-ords.
	r_shape . bounds = MCRectangleMake(0, 0, rect . width, rect . height);
	
	// IM-2014-01-08: [[ StackScale ]] convert stack coords to card coords
	r_shape . bounds = MCRectangleGetTransformedBounds(r_shape . bounds, MCGAffineTransformInvert(gettransform()));
	
	r_shape . rectangle = r_shape . bounds;
	
	return true;
}

void MCStack::unlockshape(MCObjectShape& p_shape)
{
}

// MW-2012-02-14: [[ FontRefs ]] This method causes recursion throughout all the
//   children (cards, controls and substacks) of the stack updating the font
//   allocations.
bool MCStack::recomputefonts(MCFontRef p_parent_font, bool p_force)
{
	// MW-2012-02-17: [[ FontRefs ]] If the stack has formatForPrinting set,
	//   make sure all its children inherit from a font with printer metrics
	//   set.
	if (getuseideallayout())
	{
		MCNameRef t_textfont;
		uint2 t_textsize;
		uint2 t_textstyle;
		getfontattsnew(t_textfont, t_textsize, t_textstyle);

		MCFontStyle t_fontstyle;
		t_fontstyle = MCFontStyleFromTextStyle(t_textstyle) | kMCFontStylePrinterMetrics;

		MCFontRef t_printer_parent_font;
		/* UNCHECKED */ MCFontCreate(t_textfont, t_fontstyle, t_textsize, t_printer_parent_font);

		bool t_changed;
		t_changed = MCObject::recomputefonts(t_printer_parent_font, p_force);

		MCFontRelease(t_printer_parent_font);
	}
	else
	{
		// A stack's font is determined by the object, so defer there first and
		// only continue if something changes.
		if (!MCObject::recomputefonts(p_parent_font, p_force))
			return false;
	}

	// MW-2012-12-14: [[ Bug ]] Only recompute the card if we are open.
	// Now iterate through the current card, updating that.
	if (opened != 0 && curcard != nil)
		curcard -> recomputefonts(m_font, p_force);
	
	// Now iterate through all the sub-stacks, updating them.
	if (substacks != NULL)
	{
		MCStack *sptr = substacks;
		do
		{
			if (sptr -> getopened() != 0)
				sptr -> recomputefonts(m_font, p_force);
			sptr = sptr->next();
		}
		while (sptr != substacks);
	}
	
	// If we are in the IDE engine and this is the home stack, we must update all
	// stacks.
	if (MCModeHasHomeStack() && MCdispatcher -> gethome() == this)
	{
		MCStack *t_stack;
		t_stack = MCdispatcher -> gethome() -> next();
		if (t_stack != MCdispatcher -> gethome())
		{
			do
			{
				if (t_stack -> getopened() != 0)
					t_stack -> recomputefonts(m_font, p_force);
				t_stack = t_stack -> next();
			}
			while(t_stack != MCdispatcher -> gethome());
		}
	}

	// The return value indicates something changed. This is only used when
	// descending the hierarchy to eliminate unnecessary updates.
	return true;
}

bool MCStack::changeextendedstate(bool setting, uint32_t mask)
{
	if (setting && !(f_extended_state & mask))
	{
		f_extended_state |= mask;
		return true;
	}
	
	if (!setting && (f_extended_state & mask))
	{
		f_extended_state &= ~mask;
		return true;
	}

	return false;
}

void MCStack::purgefonts()
{
	recomputefonts(parent -> getfontref(), true);
	recompute();
	
	// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
	dirtyall();
}

// MW-2013-11-07: [[ Bug 11393 ]] If the stack is formatForPrinting or has fullscreenmode
//   set then ideal layout should be used (on Windows).
bool MCStack::getuseideallayout(void)
{
#if !defined(_WINDOWS)
	// If we are not on Windows, then we are already using ideal layout.
	return false;
#else
	if (getflag(F_FORMAT_FOR_PRINTING))
		return true;

	// IM-2014-02-12: [[ Bug 11783 ]] Only use ideal layout if stack is fullscreen and has a scaling fullscreenmode
	if (view_getfullscreen() && m_view_fullscreenmode != kMCStackFullscreenResize && m_view_fullscreenmode != kMCStackFullscreenNoScale)
		return true;

	return false;
#endif
}

#ifndef _MAC_DESKTOP
// MW-2014-06-11: [[ Bug 12495 ]] Non-platform API version of updating windowshape.
void MCStack::updatewindowshape(MCWindowShape *p_shape)
{
    destroywindowshape();
    m_window_shape = p_shape;
    // MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
    dirtyall();
}
#endif

//////////

MCRectangle MCStack::getwindowrect(void) const
{
	if (window == nil)
		return rect;
		
	MCRectangle t_rect;
	t_rect = view_getwindowrect();
	
	// IM-2014-01-23: [[ HiDPI ]] Use inverse view transform to get stack coords
	// IM-2014-07-14: [[ Bug 12765 ]] Don't use the stack transform as this should be in view coords.
	return MCRectangleGetTransformedBounds(t_rect, MCGAffineTransformInvert(view_getviewtransform()));
}

//////////

void MCStack::constrain(MCPoint p_size, MCPoint& r_new_size)
{
	r_new_size . x = MCMax(minwidth, MCMin(maxwidth, p_size . x));
	r_new_size . y = MCMax(minheight, MCMin(maxheight, p_size . y));
}

//////////

bool MCStack::haswindow(void)
{
	return window != NULL;
}

void MCStack::openwindow(Boolean p_override)
{
	if (MCModeMakeLocalWindows() && window != NULL)
	{
		// IM-2014-09-23: [[ Bug 13349 ]] Sync geometry to window before opening.
		view_update_geometry();
		platform_openwindow(p_override);
	}
}

//////////

// MW-2014-09-30: [[ ScriptOnlyStack ]] Sets the stack as script only with the given script.
void MCStack::setasscriptonly(MCStringRef p_script)
{
    MCExecContext ctxt(nil,nil,nil);
    /* UNCHECKED */ SetScript(ctxt, p_script);
    
    m_is_script_only = true;
    
    // Make sure we have at least one card.
    if (cards == NULL)
    {
        curcard = cards = MCtemplatecard->clone(False, False);
        cards->setparent(this);
    }
}

MCPlatformControlType MCStack::getcontroltype()
{
    MCPlatformControlType t_type;
    t_type = MCObject::getcontroltype();
    
    if (t_type != kMCPlatformControlTypeGeneric)
        return t_type;
    else
        return kMCPlatformControlTypeWindow;
}

MCPlatformControlPart MCStack::getcontrolsubpart()
{
    return kMCPlatformControlPartNone;
}

//////////

bool MCStack::attach(void *p_context, MCStackAttachmentCallback p_callback)
{
    MCStackAttachment *t_attachment;
    for(t_attachment = m_attachments; t_attachment != nil; t_attachment = t_attachment -> next)
        if (t_attachment -> context == p_context && t_attachment -> callback == p_callback)
            return true;
    
    if (!MCMemoryNew(t_attachment))
        return false;
    
    t_attachment -> next = m_attachments;
    t_attachment -> context = p_context;
    t_attachment -> callback = p_callback;
    m_attachments = t_attachment;
    
    // If we are already realized, then notify.
    if (window != nil)
        p_callback(p_context, this, kMCStackAttachmentEventRealizing);
    
    return true;
}

void MCStack::detach(void *p_context, MCStackAttachmentCallback p_callback)
{
    MCStackAttachment *t_attachment, *t_previous;
    for(t_previous = nil, t_attachment = m_attachments; t_attachment != nil; t_attachment = t_attachment -> next)
    {
        if (t_attachment -> context == p_context && t_attachment -> callback == p_callback)
            break;
        t_previous = t_attachment;
    }
    
    if (t_attachment == nil)
        return;
    
    if (t_previous != nil)
        t_previous -> next = t_attachment -> next;
    else
        m_attachments = t_attachment -> next;
    
    MCMemoryDelete(t_attachment);
}

void MCStack::notifyattachments(MCStackAttachmentEvent p_event)
{
    for(MCStackAttachment *t_attachment = m_attachments; t_attachment != nil; t_attachment = t_attachment -> next)
        t_attachment -> callback(t_attachment -> context, this, p_event);
}

MCStackObjectVisibility MCStack::gethiddenobjectvisibility()
{
	return m_hidden_object_visibility;
}

void MCStack::sethiddenobjectvisibility(MCStackObjectVisibility p_visibility)
{
	bool t_visible = showinvisible();
	
	m_hidden_object_visibility = p_visibility;
	
	if (t_visible != showinvisible())
	{
		// Visibility of objects has changed so redraw the stack.
		view_dirty_all();
	}
}

bool MCStack::geteffectiveshowinvisibleobjects()
{
	switch (gethiddenobjectvisibility())
	{
		case kMCStackObjectVisibilityDefault:
			return MCshowinvisibles;
			
		case kMCStackObjectVisibilityShow:
			return true;
			
		case kMCStackObjectVisibilityHide:
			return false;

		default:
			MCUnreachable();
			return false;
	}
}