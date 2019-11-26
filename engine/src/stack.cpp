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
    
    DEFINE_RW_OBJ_PROPERTY(P_KEY, Any, MCStack, Key)
    DEFINE_RW_OBJ_PROPERTY(P_PASSWORD, Any, MCStack, Password)
    
    // IM-2014-01-07: [[ StackScale ]] Add stack scalefactor property
    DEFINE_RW_OBJ_PROPERTY(P_SCALE_FACTOR, Double, MCStack, ScaleFactor)
    
    // MERG-2015-08-31: [[ ScriptOnly ]] Add stack scriptOnly property
    DEFINE_RW_OBJ_PROPERTY(P_SCRIPT_ONLY, Bool, MCStack, ScriptOnly)
    
    // MERG-2015-10-11: [[ DocumentFilename ]] Add stack documentFilename property
    DEFINE_RW_OBJ_PROPERTY(P_DOCUMENT_FILENAME, String, MCStack, DocumentFilename)
	
	// IM-2016-02-26: [[ Bug 16244 ]] Add stack showInvisibles property
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_SHOW_INVISIBLES, OptionalBool, MCStack, ShowInvisibleObjects)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_SHOW_INVISIBLES, Bool, MCStack, ShowInvisibleObjects)
    
    DEFINE_RO_OBJ_PROPERTY(P_MIN_STACK_FILE_VERSION, String, MCStack, MinStackFileVersion)
};

MCObjectPropertyTable MCStack::kPropertyTable =
{
	&MCObject::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCStack::MCStack()
    : savecard(),
      savecontrols(),
      wposition(WP_DEFAULT),
      walignment(OP_NONE),
      scrollmode()
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
    _menubar = MCValueRetain(kMCEmptyName);
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
    
    // BWM-2017-08-16: [[ Bug 17810 ]] Script-only-stack line endings default to LF.
    m_line_encoding_style = kMCStringLineEndingStyleLF;

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
    
    m_is_ide_stack = false;
}

MCStack::MCStack(const MCStack &sref)
    : MCObject(sref),
      savecard(),
      savecontrols(),
      wposition(WP_DEFAULT),
      walignment(OP_NONE),
      scrollmode(),
      old_rect(kMCEmptyRectangle)
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
			MCAudioClip *newaclip = new (nothrow) MCAudioClip(*aptr);
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
			MCVideoClip *newvclip = new (nothrow) MCVideoClip(*vptr);
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
		stackfiles = new (nothrow) MCStackfile[ts];
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
		linkatts = new (nothrow) Linkatts;
		memcpy(linkatts, sref.linkatts, sizeof(Linkatts));
        
		linkatts->colorname = linkatts->colorname == nil ? nil : (MCStringRef)MCValueRetain(sref.linkatts->colorname);
		linkatts->hilitecolorname = linkatts->hilitecolorname == nil ? nil : (MCStringRef)MCValueRetain(sref.linkatts->hilitecolorname);
		linkatts->visitedcolorname = linkatts->hilitecolorname == nil ? nil : (MCStringRef)MCValueRetain(sref.linkatts->visitedcolorname);
	}
	else
		linkatts = NULL;
	filename = MCValueRetain(kMCEmptyString);
    _menubar = MCValueRetain(sref._menubar);
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
    
    m_is_ide_stack = sref.m_is_ide_stack;
}

MCStack::~MCStack()
{
	flags &= ~F_DESTROY_STACK;
	state |= CS_DELETE_STACK;
	while (opened)
		close();
	extraclose(false);

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
	
	if (m_parent_stack.IsBound())
		setparentstack(nil);

	delete[] mnemonics; /* Allocated with new[] */
	MCValueRelease(title);
	MCValueRelease(titlestring);

	// Clear and free the id cache before removing any controls
	freeobjectidcache();
	
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

	// MW-2004-11-17: If this is the current Message Box, set to NULL
	if (MCmbstackptr.IsBoundTo(this))
		MCmbstackptr = nil;
    if (MCtopstackptr.IsBoundTo(this))
    {
        MCtopstackptr = nil;
        MCstacks->top(nil);
    }
    if (MCstaticdefaultstackptr.IsBoundTo(this))
		MCstaticdefaultstackptr = MCtopstackptr;
	if (MCdefaultstackptr.IsBoundTo(this))
		MCdefaultstackptr = MCstaticdefaultstackptr;
    delete[] needs; /* Allocated with new[] */
	if (stackfiles != NULL)
	{
		while (nstackfiles--)
		{
			MCValueRelease(stackfiles[nstackfiles].stackname);
			MCValueRelease(stackfiles[nstackfiles].filename);
		}
		delete[] stackfiles; /* Allocated with new[] */
	}
	if (linkatts != NULL)
	{
		MCValueRelease(linkatts->colorname);
		MCValueRelease(linkatts->hilitecolorname);
		MCValueRelease(linkatts->visitedcolorname);
		delete linkatts;
	}
	MCValueRelease(filename);

	MCValueRelease(_menubar);

	unloadexternals();

	// COCOA-TODO: Remove dependence on ifdef
#if !defined(_MAC_DESKTOP)
	MCEventQueueFlush(this);
#endif
	
	// MW-2011-09-13: [[ Redraw ]] If there is snapshot, get rid of it.
	MCGImageRelease(m_snapshot);
	m_snapshot = nil;
	
	view_destroy();

	release_window_buffer();

    // Make sure we destroy the window
    if (window != nullptr)
        MCscreen->destroywindow(window);
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
	
	if (MCacptr && MCacptr->getmessagestack() == this)
		MCacptr->setmessagestack(NULL);
	if (state & CS_ICONIC)
	{
		seticonic(false);
	}
	if (MCmousestackptr.IsBoundTo(this))
	{
		MCmousestackptr = nil;
		int2 x, y;
		MCscreen->querymouse(x, y);
		if (MCU_point_in_rect(curcard->getrect(), x, y))
		{
			ibeam = 0;
		}
	}
	if (MCclickstackptr == this)
		MCclickstackptr = nil;
	if (MCfocusedstackptr == this)
		MCfocusedstackptr = nil;
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
			destroywindow();
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
	if ((mode == WM_TOP_LEVEL || mode == WM_TOP_LEVEL_LOCKED) && editing == NULL && !m_is_ide_stack)
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
		MCfocusedstackptr = nil;
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
        {
			if (MCactiveimage)
			{
				MCactiveimage->cutimage();
				return True;
			}
			else
				return MCselected->usercut();
        }

		if (MCactiveimage)
		{
			MCactiveimage->delimage();
			return True;
		}
		return MCselected->userdel();
	case XK_BackSpace:
		if (MCmodifierstate & MS_MOD1)
			return MCundos->undo();
		if (MCactiveimage)
		{
			MCactiveimage->delimage();
			return True;
		}
		return MCselected->userdel();
	case XK_osfUndo:
		return MCundos->undo();
	case XK_osfCut:
		if (MCactiveimage)
		{
			MCactiveimage->cutimage();
			return True;
		}
		else
			return MCselected->usercut();
	case XK_osfCopy:
		if (MCactiveimage)
		{
			MCactiveimage->copyimage();
			return True;
		}
		else
			return MCselected->usercopy();
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
			if (MCactiveimage)
			{
				MCactiveimage->copyimage();
				return True;
			}
		return MCselected->usercopy();
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
			if (MCactiveimage)
			{
				MCactiveimage->copyimage();
				return True;
			}
			return MCselected->usercopy();
		case XK_V:
		case XK_v:
			MCdefaultstackptr = MCtopstackptr;
			return MCdispatcher -> dopaste(optr);
		case XK_X:

		case XK_x:

			if (MCactiveimage)
			{
				MCactiveimage->cutimage();
				return True;
			}
			return MCselected->usercut();
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
		if (x < rect.width || (cptr != NULL && !cptr->getstate(CS_SUBMENU)))
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
	if (MCmousestackptr.IsBoundTo(this))
		MCmousestackptr = nil;
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
	if (opened && mode < WM_PULLDOWN && MCmousestackptr.IsBoundTo(this))
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
	if (MCNameIsEqualToCaseless(mptr, MCM_internal))
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
	else if (MCNameIsEqualToCaseless(mptr, MCM_idle))
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

bool MCStack::isdeletable(bool p_check_flag)
{
    // MW-2012-10-26: [[ Bug 9918 ]] If 'cantDelete' is set, then don't delete the stack. Also
    //   make sure we throw an error.
    if (!parent || scriptdepth != 0 ||
        (p_check_flag && getflag(F_S_CANT_DELETE)) ||
        MCdispatcher->gethome() == this || this -> isediting() ||
        MCdispatcher->getmenu() == this || MCmenuobjectptr == this)
    {
        MCAutoValueRef t_long_name;
        getnameproperty(P_LONG_NAME, 0, &t_long_name);
        MCeerror->add(EE_OBJECT_CANTREMOVE, 0, 0, *t_long_name);
        return false;
    }
    
    if (cards != nil)
    {
        MCCard *t_card;
        t_card = cards;
        do
        {
            // we should be able to remove a stack
            // that has cantDelete cards
            
            // ensure we don't iterate controls twice
            // by calling isdeletable on the card
            if (!t_card->MCObject::isdeletable(false))
                return false;
            
            t_card = t_card -> next();
        }
        while(t_card != cards);
    }
    
    if (controls != nil)
    {
        MCControl *t_control;
        t_control = controls;
        do
        {
            // we should be able to remove a stack
            // that has cantDelete groups
            if (!t_control->isdeletable(false))
                return false;
            
            t_control = t_control -> next();
        }
        while(t_control != controls);
    }
    
    if (substacks != nil)
    {
        MCStack *t_stack = substacks;
        do
        {
            // we should be able to delete a mainstack from
            // memory that has cantDelete substacks
            if (!t_stack->isdeletable(false))
                return false;
            
            t_stack = t_stack->next();
        }
        while (t_stack != substacks);
    }

    return true;
}

Boolean MCStack::dodel()
{
    MCscreen->ungrabpointer();
    MCdispatcher->removemenu();
    
    setstate(True, CS_DELETE_STACK);
    
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
    
    if (window != NULL && !(state & CS_FOREIGN_WINDOW))
    {
        stop_externals();
        destroywindow();
    }
    
    removereferences();
        
    // MCObject now does things on del(), so we must make sure we finish by
    // calling its implementation.
    return MCObject::del(true);
}

Boolean MCStack::del(bool p_check_flag)
{
    if (!isdeletable(p_check_flag))
	   return False;

    while (substacks)
    {
        /* When a substack is deleted it removes itself from its mainstack,
         * however it isn't actually destroyed - it must be explicitly
         * scheduled for deletion. */
        MCStack *t_substack = substacks;
        if (!substacks -> del(false))
            return False;
        
        /* Schedule the substack for deletion - unlike a main stack we don't
         * need to check for it being in the MCtodestroy list as only mainstacks
         * can ever be in that list. */
        t_substack->scheduledelete();
    }
    
    return dodel();
}

void MCStack::removereferences()
{
    if (controls != NULL)
    {
        MCControl *t_control = controls;
        do
        {   t_control -> removereferences();
            t_control = t_control -> next();
        }
        while(t_control != controls);
    }
    
    if (aclips != NULL)
    {
        MCAudioClip *t_aclip = aclips;
        do
        {   t_aclip -> removereferences();
            t_aclip = t_aclip -> next();
        }
        while(t_aclip != aclips);
    }
    
    if (vclips != NULL)
    {
        MCVideoClip *t_vclip = vclips;
        do
        {   t_vclip -> removereferences();
            t_vclip = t_vclip -> next();
        }
        while(t_vclip != vclips);
    }
    
    if (cards != NULL)
    {
        MCCard *t_card = cards;
        do
        {   t_card -> removereferences();
            t_card = t_card -> next();
        }
        while(t_card != cards);
    }
    
    if (MCdispatcher->ismainstack(this))
    {
        MCdispatcher->removestack(this);
    }
    else if (MCdispatcher->is_transient_stack(this))
    {
        MCdispatcher->remove_transient_stack(this);
    }
    else if (parent->gettype() == CT_STACK)
    {
        remove(parent.GetAs<MCStack>()->substacks);
        // MW-2012-09-07: [[ Bug 10372 ]] If the stack no longer has substacks then make sure we
        //   undo the extraopen.
        if (parent.GetAs<MCStack>()->substacks == nil)
            parent.GetAs<MCStack>()->extraclose(true);
    }
    else
    {
        // One of the above conditions should always be true
        MCUnreachable();
    }
    
    if (MCtopstackptr == this)
    {
        MCtopstackptr = nil;
        MCstacks->top(nil);
    }
    if (MCstaticdefaultstackptr == this)
        MCstaticdefaultstackptr = MCtopstackptr;
    if (MCdefaultstackptr == this)
        MCdefaultstackptr = MCstaticdefaultstackptr;
    
    // MW-2008-10-28: [[ ParentScripts ]] If this stack has its 'has parentscripts'
    //   flag set, flush the parentscripts table.
    if (getextendedstate(ECS_HAS_PARENTSCRIPTS))
        MCParentScript::FlushStack(this);
    
    if (needs != NULL)
    {
        while (nneeds--)
            needs[nneeds]->removelink(this);
        
        delete needs;
        needs = NULL;
    }

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
    
    // COCOA-TODO: Remove dependence on ifdef
#if !defined(_MAC_DESKTOP)
    MCEventQueueFlush(this);
#endif
    
    MCObject::removereferences();
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

MCStackHandle MCStack::getstack()
{
	return GetHandle();
}

Exec_stat MCStack::handle(Handler_type htype, MCNameRef message, MCParameter *params, MCObject *passing_object)
{
	if (!opened)
	{
		if (window == NULL && !MCNameIsEqualToCaseless(message, MCM_start_up)
#ifdef _MACOSX
		        && !(state & CS_DELETE_STACK))
#else
				&& !MCStringIsEmpty(externalfiles) && !(state & CS_DELETE_STACK))
#endif
		{
			// IM-2014-01-16: [[ StackScale ]] Ensure view has the current stack rect
			view_setstackviewport(rect);
			createwindow();
		}
	}

	// MW-2009-01-28: [[ Bug ]] Card and stack parentScripts don't work.
	// First attempt to handle the message with the script and parentScript of this
	// object.
	Exec_stat stat;
	stat = ES_NOT_HANDLED;
	if (!MCdynamicpath || MCdynamiccard->getparent() == this)
		stat = handleself(htype, message, params);
	else if (passing_object != nil)
	{
		// MW-2011-06-20:  If dynamic path is enabled, and this stack is not the parent
		//   of the dynamic card then instead of passing through this stack, we pass
		//   through the dynamic card (which will then pass through this stack).
		MCdynamicpath = False;
		stat = MCdynamiccard->handle(htype, message, params, this);
		return stat;
	}

	if (((passing_object != nil && stat == ES_PASS) || stat == ES_NOT_HANDLED) && m_externals != nil)
	{
        // TODO[19681]: This can be removed when all engine messages are sent with
        // target.
        bool t_target_was_valid = MCtargetptr.IsValid();
        
		Exec_stat oldstat = stat;
		stat = m_externals -> Handle(this, htype, message, params);
		if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
        if (stat == ES_PASS || stat == ES_NOT_HANDLED)
        {
            if (t_target_was_valid && !MCtargetptr.IsValid())
            {
                stat = ES_NORMAL;
            }
        }
	}

	// MW-2011-06-30: Cleanup of message path - this clause handles the transition
	//   through the home stack to dispatcher.
	if (passing_object != nil && (stat == ES_PASS || stat == ES_NOT_HANDLED) && parent)
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

	if (stat == ES_ERROR && !MCerrorptr)
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

void MCStack::OnViewTransformChanged()
{
	if (curcard != nil)
		curcard->OnViewTransformChanged();
}

void MCStack::OnAttach()
{
	if (curcard != nil)
		curcard->OnAttach();
}

void MCStack::OnDetach()
{
	if (curcard != nil)
		curcard->OnDetach();
}

///////////////////////////////////////////////////////////////////////////////

void MCStack::loadexternals(void)
{
    notifyattachments(kMCStackAttachmentEventRealizing);
    
	if (MCStringIsEmpty(externalfiles) || m_externals != NULL || !MCSecureModeCanAccessExternal())
		return;

	m_externals = new (nothrow) MCExternalHandlerList;

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
				/* UNCHECKED */ MCStringAppendSubstring(*t_new_filename, p_path, MCRangeMakeMinMax(2, MCStringGetLength(p_path)));
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
    t_range = MCRangeMakeMinMax(t_start_index, MCStringGetLength(p_path));
    return MCStringFormat(r_resolved, "%@/%*@", *t_cur_dir, &t_range, p_path);
}

// OK-2009-01-09: [[Bug 1161]]
// This function will attempt to resolve the specified filename relative to the stack
// and will either return an absolute path if the filename was found relative to the stack,
// or a copy of the original buffer. The returned buffer should be freed by the caller.
bool MCStack::resolve_filename(MCStringRef p_filename, MCStringRef& r_resolved)
{
    MCAutoStringRef t_filename;
    if (resolve_relative_path(p_filename, &t_filename))
    {
        if (MCS_exists(*t_filename, True))
        {
            r_resolved = MCValueRetain(*t_filename);
            return true;
        }
    }
    
	r_resolved = MCValueRetain(p_filename);
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
		dirtyall();
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

void MCStack::startparsingscript(MCObject *p_object, MCDataRef& r_data)
{
    unichar_t *t_unicode_string;
    uint32_t t_length;
    /* UNCHECKED */ MCStringConvertToUnicode(p_object->_getscript(), t_unicode_string, t_length);
    /* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t *)t_unicode_string, (t_length + 1) * 2, r_data);
}

void MCStack::stopparsingscript(MCObject *p_object, MCDataRef p_data)
{
    MCValueRelease(p_data);
}
