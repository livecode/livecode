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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"


#include "exec.h"
#include "stack.h"
#include "aclip.h"
#include "vclip.h"
#include "dispatch.h"
#include "card.h"
#include "objptr.h"
#include "mccontrol.h"
#include "image.h"
#include "field.h"
#include "button.h"
#include "group.h"
#include "graphic.h"
#include "player.h"
#include "eps.h"
#include "scrolbar.h"
#include "magnify.h"
#include "cpalette.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "visual.h"
#include "chunk.h"
#include "mcerror.h"
#include "util.h"
#include "date.h"
#include "objectstream.h"
#include "mode.h"
#include "redraw.h"
#include "font.h"
#include "variable.h"
#include "widget.h"

#include "globals.h"
#include "mctheme.h"
#include "license.h"
#include "stacksecurity.h"
#include "exec.h"

#include "exec.h"
#include "graphics_util.h"

#include "stackfileformat.h"

#define STACK_EXTRA_ORIGININFO (1U << 0)

IO_stat MCStack::load_substacks(IO_handle stream, uint32_t version)
{
	IO_stat stat;

	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		switch (type)
		{
		case OT_STACK:
		case OT_ENCRYPT_STACK:
			{
				MCStack *newstk = nil;
				/* UNCHECKED */ MCStackSecurityCreateStack(newstk);
				newstk->setparent(this);
				if ((stat = newstk->load(stream, version, type)) != IO_NORMAL)
				{
					delete newstk;
					return checkloadstat(stat);
				}
				newstk->appendto(substacks);
				
				// MW-2011-08-09: [[ Groups ]] Make sure F_GROUP_SHARED is set
				//   appropriately.
				newstk -> checksharedgroups();
			}
			break;
		default:
			MCS_seek_cur(stream, -1);
			return IO_NORMAL;
		}
	}
	return IO_NORMAL;
}

IO_stat MCStack::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_remaining)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	// MW-2013-03-28: Read the legacy origininfo section - this isn't generated in
	//   6.0+ stackfiles.
	if (p_remaining > 0)
	{
		uint4 t_flags, t_length, t_header_length;
		t_stat = checkloadstat(p_stream . ReadTag(t_flags, t_length, t_header_length));

		if (t_stat == IO_NORMAL)
			t_stat = checkloadstat(p_stream . Mark());

		uint32_t t_origin_info;
		if (t_stat == IO_NORMAL && (t_flags & STACK_EXTRA_ORIGININFO))
			t_stat = checkloadstat(p_stream . ReadU32(t_origin_info));

		if (t_stat == IO_NORMAL)
			t_stat = checkloadstat(p_stream . Skip(t_length));

		if (t_stat == IO_NORMAL)
			p_remaining -= t_length + t_header_length;
	}

	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedload(p_stream, p_version, p_remaining);

	return t_stat;
}

IO_stat MCStack::load(IO_handle stream, uint32_t version)
{
	MCUnreachableReturn(IO_ERROR);
}

IO_stat MCStack::load(IO_handle stream, uint32_t version, uint1 type)
{
	IO_stat stat;
	
	// FG-2013-09-20 [[ Bugfix 10846 ]]
	// Community edition cannot read encrypted stacks
	if (type != OT_STACK)
	{
		if (MCresult->isclear() && type == OT_ENCRYPT_STACK)
			MCresult->sets("Encrypted stacks cannot be opened in Community Edition");
		return IO_ERROR;
	}
	
	uint32_t t_reserved = 0;
	
	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_uint4(&t_reserved, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	
	stat = load_stack(stream, version);
	
	return checkloadstat(stat);
}

IO_stat MCStack::load_stack(IO_handle stream, uint32_t version)
{
	IO_stat stat;
	
//---- 2.7+:
//  . F_OPAQUE now valid - default true
	if (version < kMCStackFileFormatVersion_2_7)
	{
		flags |= F_OPAQUE;
	}
//----

	if (MCtranslatechars)
		state |= CS_TRANSLATED;
	if ((stat = IO_read_uint4(&iconid, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if (version > kMCStackFileFormatVersion_1_0)
	{
		if (flags & F_TITLE)
		{
			// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
			if (version >= kMCStackFileFormatVersion_7_0)
			{
				if ((stat = IO_read_stringref_new(title, stream, true)) != IO_NORMAL)
					return checkloadstat(stat);
			}
			else if (version >= kMCStackFileFormatVersion_5_5)
			{
				// MW-2012-03-04: [[ StackFile5500 ]] If the version is 5.5 or above, then the
				//   stack title will be UTF-8 already.
				if ((stat = IO_read_stringref_legacy_utf8(title, stream)) != IO_NORMAL)
					return checkloadstat(stat);
			}
			else
			{
				// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv < 5500, then legacy string
				if ((stat = IO_read_stringref_legacy(title, stream, false)) != IO_NORMAL)
					return checkloadstat(stat);
			}
		}
		if (flags & F_DECORATIONS)
		{
			if ((stat = IO_read_uint2(&decorations, stream)) != IO_NORMAL)
				return checkloadstat(stat);
			if (!(decorations & WD_WDEF) && decorations & WD_SHAPE)
				if ((stat = IO_read_uint4(&windowshapeid, stream)) != IO_NORMAL)
					return checkloadstat(stat);
		}
	}
	else
		flags &= ~(F_TITLE | F_DECORATIONS);
	if (version < kMCStackFileFormatVersion_2_3)
		flags &= ~(F_SHOW_BORDER | F_3D | F_OPAQUE | F_FORMAT_FOR_PRINTING);
	if (flags & F_RESIZABLE)
	{
		if ((stat = IO_read_uint2(&minwidth, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if ((stat = IO_read_uint2(&minheight, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if ((stat = IO_read_uint2(&maxwidth, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if ((stat = IO_read_uint2(&maxheight, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (maxwidth == 1280 && maxheight == 1024)
			maxwidth = maxheight = MAXUINT2;
	}
	
	// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if ((stat = IO_read_stringref_new(externalfiles, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
		return checkloadstat(stat);
	
	if (version > kMCStackFileFormatVersion_1_3)
	{
		if ((stat = MCLogicalFontTableLoad(stream, version)) != IO_NORMAL)
			return checkloadstat(stat);

		// MW-2012-02-17: [[ LogFonts ]] Now we have a fonttable, we can resolve the
		//   stack's font attrs.
		if (getflag(F_FONT))
		{
			// MW-2012-02-19: [[ SplitTextAttrs ]] Clearout the font flag now that we've
			//   used it to determine whether to resolve font attrs or not.
			flags &= ~F_FONT;
			loadfontattrs(s_last_font_index);
		}
	}
	
	// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if (flags & F_STACK_FILES)
	{
		MCAutoStringRef sf;
		if ((stat = IO_read_stringref_new(&sf, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return checkloadstat(stat);
		setstackfiles(*sf);
		
	}
	
	// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if (flags & F_MENU_BAR)
	{
		MCNameRef t_menubar;
		if ((stat = IO_read_nameref_new(t_menubar, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return checkloadstat(stat);
		MCValueRelease(_menubar);
		_menubar = t_menubar;
	}
	
	if (flags & F_LINK_ATTS)
	{
		linkatts = new (nothrow) Linkatts;
		memset(linkatts, 0, sizeof(Linkatts));
		
		// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		if ((stat = IO_read_mccolor(linkatts->color, stream)) != IO_NORMAL
		        || (stat = IO_read_stringref_new(linkatts->colorname, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return checkloadstat(stat);
		
		// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		if ((stat = IO_read_mccolor(linkatts->hilitecolor, stream)) != IO_NORMAL
		        || (stat=IO_read_stringref_new(linkatts->hilitecolorname, stream, version >= kMCStackFileFormatVersion_7_0))!=IO_NORMAL)
			return checkloadstat(stat);
		
		// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		if ((stat = IO_read_mccolor(linkatts->visitedcolor, stream)) != IO_NORMAL
		        || (stat=IO_read_stringref_new(linkatts->visitedcolorname, stream, version >= kMCStackFileFormatVersion_7_0))!=IO_NORMAL)
			return checkloadstat(stat);

        uint1 t_underline = 0;
		if ((stat = IO_read_uint1(&t_underline, stream)) != IO_NORMAL)
			return checkloadstat(stat);
        linkatts->underline = (t_underline != 0);
        
        // for interface colors, empty name means unset whereas nil name means
        // defer to rgb values. Therefore set values to nil if they are empty.
        if (MCStringIsEmpty(linkatts -> colorname))
        {
            MCValueRelease(linkatts -> colorname);
            linkatts -> colorname = nil;
        }
        
        if (MCStringIsEmpty(linkatts -> hilitecolorname))
        {
            MCValueRelease(linkatts -> hilitecolorname);
            linkatts -> hilitecolorname = nil;
        }
        
        if (MCStringIsEmpty(linkatts -> visitedcolorname))
        {
            MCValueRelease(linkatts -> visitedcolorname);
            linkatts -> visitedcolorname = nil;
        }
	}

	if ((stat = loadpropsets(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);

	mode_load();

	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		switch (type)
		{
		case OT_CARD:
			{
				MCCard *newcard = new (nothrow) MCCard;
				newcard->setparent(this);
				if ((stat = newcard->load(stream, version)) != IO_NORMAL)
				{
					delete newcard;
					return checkloadstat(stat);
				}
				newcard->appendto(cards);
				if (curcard == NULL)
					curcard = cards;
			}
			break;
		case OT_GROUP:
			{
				MCGroup *newgroup = new (nothrow) MCGroup;
				newgroup->setparent(this);
				if ((stat = newgroup->load(stream, version)) != IO_NORMAL)
				{
					delete newgroup;
					return checkloadstat(stat);
				}
				MCControl *newcontrol = newgroup;
				newcontrol->appendto(controls);
			}
			break;
		case OT_BUTTON:
			{
				MCButton *newbutton = new (nothrow) MCButton;
				newbutton->setparent(this);
				if ((stat = newbutton->load(stream, version)) != IO_NORMAL)
				{
					delete newbutton;
					return checkloadstat(stat);
				}
				MCControl *cptr = (MCControl *)newbutton;
				cptr->appendto(controls);
			}
			break;
		case OT_FIELD:
			{
				MCField *newfield = new (nothrow) MCField;
				newfield->setparent(this);
				if ((stat = newfield->load(stream, version)) != IO_NORMAL)
				{
					delete newfield;
					return checkloadstat(stat);
				}
				MCControl *cptr = (MCControl *)newfield;
				cptr->appendto(controls);
			}
			break;
		case OT_IMAGE:
			{
				MCImage *newimage = new (nothrow) MCImage;
				newimage->setparent(this);
				if ((stat = newimage->load(stream, version)) != IO_NORMAL)
				{
					delete newimage;
					return checkloadstat(stat);

				}
				MCControl *cptr = (MCControl *)newimage;
				cptr->appendto(controls);
			}
			break;
		case OT_SCROLLBAR:
			{
				MCScrollbar *newscrollbar = new (nothrow) MCScrollbar;
				newscrollbar->setparent(this);
				if ((stat = newscrollbar->load(stream, version)) != IO_NORMAL)
				{
					delete newscrollbar;
					return checkloadstat(stat);
				}
				newscrollbar->appendto(controls);
			}
			break;
		case OT_GRAPHIC:
			{
				MCGraphic *newgraphic = new (nothrow) MCGraphic;
				newgraphic->setparent(this);
				if ((stat = newgraphic->load(stream, version)) != IO_NORMAL)
				{
					delete newgraphic;
					return checkloadstat(stat);
				}
				newgraphic->appendto(controls);
			}
			break;
		case OT_PLAYER:
			{
				MCPlayer *newplayer = new (nothrow) MCPlayer;
				newplayer->setparent(this);
				if ((stat = newplayer->load(stream, version)) != IO_NORMAL)
				{
					delete newplayer;
					return checkloadstat(stat);
				}
				MCControl *cptr = (MCControl *)newplayer;
				cptr->appendto(controls);
			}
			break;
		case OT_MCEPS:
			{
				MCEPS *neweps = new (nothrow) MCEPS;
				neweps->setparent(this);
				if ((stat = neweps->load(stream, version)) != IO_NORMAL)
				{
					delete neweps;
					return checkloadstat(stat);
				}
				neweps->appendto(controls);
			}
			break;
        case OT_WIDGET:
            {
                MCWidget *newwidget = new (nothrow) MCWidget;
                newwidget->setparent(this);
                if ((stat = newwidget->load(stream, version)) != IO_NORMAL)
                {
                    delete newwidget;
                    return checkloadstat(stat);
                }
                newwidget->appendto(controls);
            }
            break;
		case OT_MAGNIFY:
			{
				MCMagnify *newmag = new (nothrow) MCMagnify;
				newmag->setparent(this);
				if ((stat = newmag->load(stream, version)) != IO_NORMAL)
				{
					delete newmag;
					return checkloadstat(stat);
				}
				newmag->appendto(controls);
			}
			break;
		case OT_COLORS:
			{
				MCColors *newcolors = new (nothrow) MCColors;
				newcolors->setparent(this);
				if ((stat = newcolors->load(stream, version)) != IO_NORMAL)
				{
					delete newcolors;
					return checkloadstat(stat);
				}
				newcolors->appendto(controls);
			}
			break;
		case OT_AUDIO_CLIP:
			{
				MCAudioClip *newaclip = new (nothrow) MCAudioClip;
				newaclip->setparent(this);
				if ((stat = newaclip->load(stream, version)) != IO_NORMAL)
				{
					delete newaclip;
					return checkloadstat(stat);
				}
				newaclip->appendto(aclips);
			}
			break;
		case OT_VIDEO_CLIP:
			{
				MCVideoClip *newvclip = new (nothrow) MCVideoClip;
				newvclip->setparent(this);
				if ((stat = newvclip->load(stream, version)) != IO_NORMAL)
				{
					delete newvclip;
					return checkloadstat(stat);
				}
				newvclip->appendto(vclips);
			}
			break;
		default:
            MCS_seek_cur(stream, -1);
                
            // IM-2013-09-30: [[ FullscreenMode ]] ensure old_rect is initialized for fullscreen stacks
            old_rect = rect;
			
            return IO_NORMAL;
		}
	}
}

IO_stat MCStack::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	uint32_t t_size, t_flags;
	t_size = 0;
	t_flags = 0;

	// The extended data area for a stack is:
	//    taginfo tag
	//    if origininfo then
	//      uint32_t origin_info

	// MW-2013-03-28: The origininfo is no longer emitted into the stackfile as it
	//   serves no purpose. (There used to be code emitting it here!)

	IO_stat t_stat;
	t_stat = p_stream . WriteTag(t_flags, t_size);
	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedsave(p_stream, p_part, p_version);

	return t_stat;
}

IO_stat MCStack::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;
	
	// MW-2012-02-17: [[ LogFonts ]] Build the logical font table for the stack and
	//   its children.
	MCLogicalFontTableBuild(this, 0);

	if (editing != NULL)
		stopedit();
	
	if (linkatts != NULL)
		flags |= F_LINK_ATTS;
	else
		flags &= ~F_LINK_ATTS;
	
	if (!MCStringIsEmpty(title))
		flags |= F_TITLE;
	else
		flags &= ~F_TITLE;

	// MW-2013-03-28: Make sure we never save a stack with F_CANT_STANDALONE set.
	flags &= ~F_CANT_STANDALONE;
	
//---- 2.7+:
//  . F_OPAQUE now valid -  previous versions must be true
	uint4 t_old_flags;
	if (p_version < kMCStackFileFormatVersion_2_7)
	{
		t_old_flags = flags;
		flags |= F_OPAQUE;
	}
//----
	
	// Store the current rect, fetching the old_rect if fullscreen.
	MCRectangle t_rect_to_restore;
	t_rect_to_restore = rect;
	if (is_fullscreen())
		rect = old_rect;

	if ((stat = IO_write_uint1(OT_STACK, stream)) != IO_NORMAL)
		return stat;

	// MW-2009-06-30: We always write out an extended data block for the
	//   stack as we always want to store the full origin info.
	// MW-2011-09-12: [[ MacScroll ]] As the scroll is a transient thing,
	//   we don't want to apply it to the rect that we save, so temporarily
	//   adjust around object save. 
	rect . height += getscroll();
	stat = MCObject::save(stream, p_part, true, p_version);
	rect = t_rect_to_restore;
	if (stat != IO_NORMAL)
		return stat;
	
	// MW-2013-11-20: [[ UnicodeFileFormat ]] These are for backwards compatibility
	//   so legacy.
	if ((stat = IO_write_cstring_legacy(NULL, stream, 2)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_cstring_legacy(NULL, stream, 2)) != IO_NORMAL)
		return stat;

//---- 2.7+:
	if (p_version < kMCStackFileFormatVersion_2_7)
	{
		flags = t_old_flags;
	}
//
	
	stat = save_stack(stream, p_part, p_force_ext, p_version);
	
	return stat;
}

IO_stat MCStack::save_stack(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;
	
	if ((stat = IO_write_uint4(iconid, stream)) != IO_NORMAL)
		return stat;
	if (flags & F_TITLE)
	{
		// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		if (p_version >= kMCStackFileFormatVersion_7_0)
		{
			if ((stat = IO_write_stringref_new(title, stream, true)) != IO_NORMAL)
				return stat;
		}
		else if (p_version >= kMCStackFileFormatVersion_5_5)
		{
			// MW-2012-03-04: [[ StackFile5500 ]] If the stackfile version is 5.5, then
			//   write out UTF-8 directly.
			if ((stat = IO_write_stringref_legacy_utf8(title, stream)) != IO_NORMAL)
				return stat;
		}
		else
        {
            if ((stat = IO_write_stringref_legacy(title, stream, false)) != IO_NORMAL)
				return stat;
		}
	}
	if (flags & F_DECORATIONS)
	{
		if ((stat = IO_write_uint2(decorations, stream)) != IO_NORMAL)
			return stat;
		if (!(decorations & WD_WDEF) && decorations & WD_SHAPE)
			if ((stat = IO_write_uint4(windowshapeid, stream)) != IO_NORMAL)
				return stat;
	}
	if (flags & F_RESIZABLE)
	{
		if ((stat = IO_write_uint2(minwidth, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(minheight, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(maxwidth, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(maxheight, stream)) != IO_NORMAL)
			return stat;
    }
	
	// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
    if ((stat = IO_write_stringref_new(externalfiles, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
		return stat;

	// MW-2012-02-17: [[ LogFonts ]] Save the stack's logical font table.
	if ((stat = MCLogicalFontTableSave(stream, p_version)) != IO_NORMAL)
		return stat;
	
	// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if (flags & F_STACK_FILES)
	{
		MCAutoStringRef t_sf;
		if (!getstackfiles(&t_sf))
			return IO_ERROR;
		if ((stat = IO_write_stringref_new(*t_sf, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return stat;
	}
	
	// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if (flags & F_MENU_BAR)
		if ((stat = IO_write_nameref_new(_menubar, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return stat;
	
	if (flags & F_LINK_ATTS)
	{
        // linkatts color names may be nil.
        
		// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
        if ((stat = IO_write_mccolor(linkatts->color, stream)) != IO_NORMAL
            || (stat = IO_write_stringref_new(linkatts->colorname != nil ? linkatts->colorname : kMCEmptyString, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return stat;
		
		// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
        if ((stat = IO_write_mccolor(linkatts->hilitecolor, stream)) != IO_NORMAL
                || (stat=IO_write_stringref_new(linkatts->hilitecolorname != nil ? linkatts->hilitecolorname : kMCEmptyString, stream, p_version >= kMCStackFileFormatVersion_7_0))!=IO_NORMAL)
			return stat;
		
		// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
        if ((stat = IO_write_mccolor(linkatts->visitedcolor, stream)) != IO_NORMAL
                || (stat=IO_write_stringref_new(linkatts->visitedcolorname != nil ? linkatts->visitedcolorname : kMCEmptyString, stream, p_version >= kMCStackFileFormatVersion_7_0))!=IO_NORMAL)
			return stat;
		
		if ((stat = IO_write_uint1(linkatts->underline, stream)) != IO_NORMAL)
			return stat;
	}
	if ((stat = savepropsets(stream, p_version)) != IO_NORMAL)
		return stat;
	if (cards != NULL)
	{
		MCCard *cptr = cards;
		do
		{
			if ((stat = cptr->save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
				return stat;
			cptr = (MCCard *)cptr->next();
		}
		while (cptr != cards);
	}
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if ((stat = cptr->save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
				return stat;
			cptr = (MCControl *)cptr->next();
		}
		while (cptr != controls);
	}
	if (aclips != NULL)
	{
		MCAudioClip *acptr = aclips;
		do
		{
			if ((stat = acptr->save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
				return stat;
			acptr = (MCAudioClip *)acptr->next();
		}
		while (acptr != aclips);
	}
	if (vclips != NULL)
	{
		MCVideoClip *vptr = vclips;
		do
		{
			if ((stat = vptr->save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
				return stat;
			vptr = (MCVideoClip *)vptr->next();
		}
		while (vptr != vclips);
	}
	if (substacks != NULL)
	{
		MCStack *sptr = substacks;
		do
		{
			if ((stat = sptr->save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
				return stat;
			sptr = (MCStack *)sptr->nptr;
		}
		while (sptr != substacks);
	}

	return IO_NORMAL;
}

// I don't think this gets called from anywhere...
Exec_stat MCStack::resubstack(MCStringRef p_data)
{
	Boolean iserror = False;
	MCAutoStringRef t_error;
	
	// MW-2012-09-07: [[ Bug 10372 ]] Record the old stack of substackedness so we
	//   can work out later whether we need to extraopen/close.
	bool t_had_substacks;
	t_had_substacks = substacks != nil;
	
	MCStack *oldsubs = substacks;
	substacks = NULL;
	
	MCAutoArrayRef t_array;
	/* UNCHECKED */ MCStringSplit(p_data, MCSTR("\n"), nil, kMCStringOptionCompareExact, &t_array);
	uindex_t t_count;
	t_count = MCArrayGetCount(*t_array);
	for (uindex_t i = 0; i < t_count; i++)
	{
		MCValueRef t_val;
        /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_array, i + 1, t_val);
		
		// If tsub is one of the existing substacks of the stack, it is set to
		// non-null, as it needs to be removed.
		MCStack *tsub = oldsubs;
		if (tsub != NULL)
		{
			// If t_val doesn't exist as a name, it can't exist as a substack name.
			// t_val is always a stringref (fetched from an MCSplitString array)
			MCNameRef t_name;
			t_name = MCNameLookupCaseless((MCStringRef)t_val);
		
			if (t_name != nil)
				while (tsub -> hasname(t_name))
				{
					tsub = (MCStack *)tsub->nptr;
					if (tsub == oldsubs)
					{
						tsub = NULL;
						break;
					}
				}
		}
		
		// OK-2008-04-10 : Added parameters to mainstackChanged message
		Boolean t_was_mainstack;
		if (tsub == NULL)
		{
			MCNewAutoNameRef t_name;
			/* UNCHECKED */ MCNameCreate((MCStringRef)t_val, &t_name);
			MCStack *toclone = MCdispatcher -> findstackname(*t_name);
			t_was_mainstack = MCdispatcher -> ismainstack(toclone);	
			
			if (toclone != NULL)
			/* UNCHECKED */ MCStackSecurityCopyStack(toclone, tsub);
		}
		else
		{
			// If we are here then it means tsub was found in the current list of
			// substacks of this stack.
			t_was_mainstack = False;
			
			tsub -> remove(oldsubs);
		}
		
		if (tsub != NULL)
		{
			MCObject *t_old_mainstack;
			if (t_was_mainstack)
				t_old_mainstack = tsub;
			else
				t_old_mainstack = tsub -> getparent();
			
			tsub->appendto(substacks);
			tsub->parent = this;
			tsub->message_with_valueref_args(MCM_main_stack_changed, t_old_mainstack -> getname(), getname());
		}
		else
		{
			iserror = True;
			t_error = ((MCStringRef)t_val);
		}
	}
	
	while (oldsubs != NULL)
	{
		MCStack *dsub = (MCStack *)oldsubs->remove(oldsubs);
		delete dsub;
	}

	// MW-2012-09-07: [[ Bug 10372 ]] Make sure we sync the appropriate extraopen/close with
	//   the updated substackness of this stack.
	if (t_had_substacks && substacks == NULL)
		extraclose(true);
	else if (!t_had_substacks && substacks != NULL)
		extraopen(true);

	// MW-2011-08-17: [[ Redraw ]] This seems a little extreme, but leave as is
	//   for now.
	MCRedrawDirtyScreen();

	if (iserror)
	{
		MCeerror->add(EE_STACK_BADSUBSTACK, 0, 0, *t_error);
		return ES_ERROR;
	}

	return ES_NORMAL;
}

MCCard *MCStack::getcardid(uint4 inid)
{
	// MW-2012-10-10: [[ IdCache ]] First check for the object in the cache.
	MCObject *t_object;
	t_object = findobjectbyid(inid);
	if (t_object != nil && t_object -> gettype() == CT_CARD)
		return (MCCard *)t_object;
	
	MCCard *cptr;
	cptr = cards;
	do
	{
		if (cptr->getid() == inid)
		{
			// MW-2012-10-10: [[ IdCache ]] Cache the found object.
			cacheobjectbyid(cptr);
			return cptr;
		}
		cptr = (MCCard *)cptr->next();
	}
	while (cptr != cards);
	return NULL;
}

// OK-2009-03-12: [[Bug 7787]] - Temporary fix. This method is the same as getcardid() except
// that it searches the savecards member, enabling cards to be found in edit group mode.
MCCard *MCStack::findcardbyid(uint4 p_id)
{
	MCCard *t_cards;
	if (editing == NULL)
		t_cards = cards;
	else
		t_cards = savecards;

	MCCard *t_card;
	t_card = t_cards;

	do
	{
		if (t_card -> getid() == p_id)
			return t_card;

		t_card = (MCCard *)t_card -> next();
	}
	while (t_card != t_cards);

	return NULL;
}


MCControl *MCStack::getcontrolid(Chunk_term type, uint4 inid, bool p_recurse)
{
	if (controls == NULL && (editing == NULL || savecontrols == NULL))
		return NULL;
	if (controls != NULL)
	{
		// MW-2012-10-10: [[ IdCache ]] Lookup the object in the cache.
		MCObject *t_object;
		t_object = findobjectbyid(inid);
		if (t_object != nil && ((type == CT_LAYER && t_object -> gettype() > CT_CARD) || t_object -> gettype() == type))
			return (MCControl *)t_object;

		MCControl *tobj = controls;
		do
		{
			MCControl *foundobj;
			if (p_recurse)
			{
				if (tobj -> gettype() == CT_GROUP)
				{
					MCGroup *t_group;
					t_group = (MCGroup *)tobj;
					foundobj = t_group -> findchildwithid(type, inid);
				}
				else
					foundobj = tobj -> findid(type, inid, False);
			}
			else
				foundobj = tobj->findid(type, inid, False);

			if (foundobj != NULL)
			{
				// MW-2012-10-10: [[ IdCache ]] Put the object in the id cache.
				cacheobjectbyid(foundobj);
				return foundobj;
			}

			tobj = (MCControl *)tobj->next();
		}
		while (tobj != controls);
	}
	if (editing != NULL && savecontrols != NULL)
	{
		MCControl *tobj = savecontrols;
		do
		{
			MCControl *foundobj = tobj->findid(type, inid, False);
			if (foundobj != NULL)
			{
				// MW-2012-10-10: [[ IdCache ]] Put the object in the id cache.
				cacheobjectbyid(foundobj);
				return foundobj;
			}
			tobj = (MCControl *)tobj->next();
		}
		while (tobj != savecontrols);
	}
	return NULL;
}

MCControl *MCStack::getcontrolname(Chunk_term type, MCNameRef p_name)
{
	if (controls == NULL)
		return NULL;
	MCControl *tobj = controls;
	do
	{
		MCControl *foundobj = tobj->findname(type, p_name);
		if (foundobj != NULL)
			return foundobj;
		tobj = (MCControl *)tobj->next();
	}
	while (tobj != controls);
	return NULL;
}

MCObject *MCStack::getAVid(Chunk_term type, uint4 inid)
{
	MCObject *objs;
	if (type == CT_AUDIO_CLIP)
		objs = aclips;
	else
		objs = vclips;
	if (objs == NULL)
		return NULL;
	MCObject *tobj = objs;
	do
	{
		if (tobj->getid() == inid)
			return tobj;
		tobj = (MCControl *)tobj->next();
	}
	while (tobj != objs);
	return NULL;
}

bool MCStack::getAVname(Chunk_term type, MCNameRef p_name, MCObject*& r_object)
{
	MCObject *objs;
	if (type == CT_AUDIO_CLIP)
		objs = aclips;
	else
		objs = vclips;
	if (objs == NULL)
		return false;
	MCObject *tobj = objs;

    // AL-2014-05-27: [[ Bug 12530 ]] Expression for audio or video clip can be numerical
    uint2 t_num;
    if (MCU_stoui2(MCNameGetString(p_name), t_num))
    {
        --t_num;
        while (t_num--)
        {
            tobj = tobj->next();
            if (tobj == objs)
                return false;
        }
        r_object = tobj;
        return true;
    }
    
	do
	{
		if (MCU_matchname(p_name, type, tobj->getname()))
        {
			r_object = tobj;
            return true;
        }
		tobj = (MCControl *)tobj->next();
	}
	while (tobj != objs);
	return false;
}

Exec_stat MCStack::setcard(MCCard *card, Boolean recent, Boolean dynamic)
{
	if (state & CS_IGNORE_CLOSE)
		return ES_NORMAL;
	Boolean wasfocused = False;
	Boolean abort = False;

	if (editing != NULL && card != curcard)
		stopedit();
	if (!opened)
	{
		curcard = card;
		return ES_NORMAL;
	}

	// MW-2011-09-13: [[ Effects ]] If the screen isn't locked and we want an effect then
	//   take a snapshot.
	if (!MCRedrawIsScreenLocked() && MCcur_effects != nil)
		snapshotwindow(curcard -> getrect());
	
	// MW-2011-09-14: [[ Redraw ]] We lock the screen between before closeCard and until
	//   after preOpenCard.
	MCRedrawLockScreen();
    
	MCCard *oldcard = curcard;
	Boolean oldlock = MClockmessages;
	if (card != oldcard)
	{
		if (MCfoundfield)
			MCfoundfield->clearfound();
		
		if (MCmousestackptr.IsBoundTo(this))
			curcard->munfocus();
		if (state & CS_KFOCUSED)
		{
			wasfocused = True;
			curcard->kunfocus();
		}

		// MW-2008-10-31: [[ ParentScripts ]] Send closeControl appropriately
		if (curcard -> closecontrols() == ES_ERROR
				|| curcard != oldcard || !opened
				|| curcard->message(MCM_close_card) == ES_ERROR
		        || curcard != oldcard || !opened
				|| curcard -> closebackgrounds(card) == ES_ERROR
		        || curcard != oldcard || !opened)
		{
			// MW-2011-09-14: [[ Redraw ]] Unlock the screen.
			MCRedrawUnlockScreen();

			if (curcard != oldcard || !opened)
				return ES_NORMAL;
			else
				return ES_ERROR;
		}

		if (dynamic && flags & F_DYNAMIC_PATHS)
			MCdynamiccard = card;
		
		MCscreen->cancelmessageobject(curcard, MCM_idle);
		uint2 oldscroll = getscroll();
		curcard = card;

		// MW-2012-02-01: [[ Bug 9966 ]] Make sure we open the new card *then* close
		//   the old card. This prevents any shared groups being closed then opened
		//   again - in particular, players will keep playing.
		curcard->open();
		
        
		// MW-2011-11-23: [[ Bug ]] Close the old card here to ensure no players
		//   linger longer than they should.
		oldcard -> close();
        

		// MW-2011-09-12: [[ MacScroll ]] Use 'getnextscroll()' to see if anything needs
		//   changing on that score.
		if (oldscroll != getnextscroll(false))
		{
			setgeom();
			updatemenubar();
		}
		
		updatecardsize();

		// MW-2008-10-31: [[ ParentScripts ]] Send preOpenControl appropriately
        bool t_error;
        t_error = false;
        
        if (!t_error)
            t_error = curcard -> openbackgrounds(true, oldcard) == ES_ERROR || curcard != card || !opened;

        if (!t_error)
        {
#ifdef FEATURE_PLATFORM_PLAYER
            // PM-2014-10-13: [[ Bug 13569 ]] Detach all players before any messages are sent
            MCPlayer::DetachPlayers(this);
#endif
                
            t_error = curcard->message(MCM_preopen_card) == ES_ERROR || curcard != card || !opened;
        }
        
        if (!t_error)
        {
            t_error = curcard -> opencontrols(true) == ES_ERROR || curcard != card || !opened;
#ifdef FEATURE_PLATFORM_PLAYER
            // PM-2014-10-13: [[ Bug 13569 ]] after any messages are sent, attach all players previously detached
             MCPlayer::AttachPlayers(this);
#endif
        }
        
        if (t_error)
		{
			// MW-2011-08-18: [[ Redraw ]] Use global screen lock
			MCRedrawUnlockScreen();
			if (curcard != card)
				return ES_NORMAL;
			else
			{
				// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
				dirtyall();
				dirtywindowname();
				return ES_ERROR;
			}
		}
        
		MClockmessages = True;

		if (mode == WM_TOP_LEVEL || mode == WM_TOP_LEVEL_LOCKED)
		{
			dirtywindowname();

			// MW-2007-09-11: [[ Bug 5139 ]] Don't add activity to recent cards if the stack is an
			//   IDE stack.
			if (recent && !m_is_ide_stack)
				MCrecent->addcard(curcard);
		}

		// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
        dirtyall();
	}
	
	// MW-2011-09-14: [[ Redraw ]] Unlock the screen so the effect stuff has a chance
	//   to run.
	MCRedrawUnlockScreen();

	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	if (MCRedrawIsScreenLocked())
	{
		while (MCcur_effects != NULL)
		{
			MCEffectList *veptr = MCcur_effects;
			MCcur_effects = MCcur_effects->next;
			delete veptr;
		}

		if (card != oldcard)
		{
			MClockmessages = oldlock;
 
			// MW-2008-10-31: [[ ParentScripts ]] Send openControl appropriately
			if (curcard -> openbackgrounds(false, oldcard) == ES_ERROR
			        || curcard != card
			        || curcard->message(MCM_open_card) == ES_ERROR
					|| curcard != card
					|| curcard -> opencontrols(false) == ES_ERROR)
			{

				if (curcard != card)
					return ES_NORMAL;
				else
					return ES_ERROR;
			}
 
            if (wasfocused)
				curcard->kfocus();
			if (MCmousestackptr.IsBoundTo(this) && !mfocus(MCmousex, MCmousey))
				curcard->message(MCM_mouse_enter);
		}
		return ES_NORMAL;
	}
	else
		effectrect(curcard->getrect(), abort);

	MClockmessages = oldlock;
	if (oldcard != NULL && oldcard != curcard)
	{
		// MW-2008-10-31: [[ ParentScripts ]] Send openControl appropriately
		if (abort
				|| curcard -> openbackgrounds(false, oldcard) == ES_ERROR
		        || curcard != card || !opened
		        || curcard->message(MCM_open_card) == ES_ERROR
		        || curcard != card || !opened
				|| curcard -> opencontrols(false) == ES_ERROR
				|| curcard != card || !opened)
		{
			if (curcard != card || !opened)
				return ES_NORMAL;
			else
				return ES_ERROR;
		}
   		
		if (wasfocused)
			kfocus();
		if (MCmousestackptr.IsBoundTo(this) && !mfocus(MCmousex, MCmousey))
			curcard->message(MCM_mouse_enter);
	}
    
	return ES_NORMAL;
}


MCStack *MCStack::findstackfile(MCNameRef p_name)
{
	MCAutoStringRef t_fname;
	getstackfile(MCNameGetString(p_name), &t_fname);
	if (!MCStringIsEmpty(*t_fname))
	{
		MCU_watchcursor(getstack(), False);
		MCStack *tstk;
		if (MCdispatcher->loadfile(*t_fname, tstk) == IO_NORMAL)
		{
			MCStack *stackptr = tstk->findsubstackname(p_name);
			
			// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
			//   return back to the caller.
			MCU_unwatchcursor(getstack(), False);
			
			if (stackptr == NULL)
				return tstk;
			
			return stackptr;
		}
		
		// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
		//   return back to the caller.
		MCU_unwatchcursor(getstack(), False);
	}
	return NULL;
}

MCStack *MCStack::findstackname(MCNameRef p_name)
{
	MCStack *foundstk;
	if ((foundstk = findsubstackname(p_name)) != NULL)
		return foundstk;
	else
		return MCdispatcher->findstackname(p_name);
}

MCStack *MCStack::findstackname_string(MCStringRef p_string)
{
	MCNewAutoNameRef t_name;
	/* UNCHECKED */ MCNameCreate(p_string, &t_name);
	return findstackname(*t_name);
}

MCStack *MCStack::findsubstackname(MCNameRef p_name)
{
	if (findname(CT_STACK, p_name) != nil)
		return this;
    
	MCStack *sptr = this;
	uint2 num = 0;
	if (!MCdispatcher->ismainstack(this) && !MCU_stoui2(MCNameGetString(p_name), num))
		sptr = parent->getstack();
	if (sptr->substacks != NULL)
	{
		MCStack *tptr = sptr->substacks;
		if (MCU_stoui2(MCNameGetString(p_name), num))
		{
			while (--num)
			{
				tptr = (MCStack *)tptr->next();
				if (tptr == sptr->substacks)
					return NULL;
			}
			return tptr;
		}
		else
			do
			{
				if (tptr->findname(CT_STACK, p_name) != NULL)
					return tptr;
				tptr = (MCStack *)tptr->next();
			}
        while (tptr != sptr->substacks);
	}
	return NULL;
}

MCStack *MCStack::findstackid(uint4 fid)
{
	if (fid == 0)
		return NULL;
	MCStack *foundstk;
	if ((foundstk = findsubstackid(fid)) != NULL)
		return foundstk;
	else
		return MCdispatcher->findstackid(fid);
}

MCStack *MCStack::findsubstackid(uint4 fid)
{
	if (fid == 0)
		return NULL;
	if (altid == fid)
		return this;

	MCStack *sptr = this;
	if (!MCdispatcher->ismainstack(this))
		sptr = parent->getstack();
	if (substacks != NULL)
	{
		MCStack *tptr = sptr->substacks;
		do
		{
			if (tptr->altid == fid)
				return tptr;
			tptr = (MCStack *)tptr->next();
		}
		while (tptr != sptr->substacks);
	}
	return NULL;
}

MCStack *MCStack::findstackwindowid(uintptr_t p_win_id)
{
	if (p_win_id == 0)
		return NULL;
	if (MCscreen->dtouint((Drawable)window) == p_win_id)
		return this;
	if (substacks != NULL)
	{
		MCStack *tptr = substacks;
		do
		{
			if (MCscreen->dtouint((Drawable)tptr->window) == p_win_id)
				return tptr;
			tptr = (MCStack *)tptr->next();
		}
		while (tptr != substacks);
	}
	return NULL;
}

void MCStack::translatecoords(MCStack *dest, int2 &x, int2 &y)
{
	// WEBREV
	// IM-2013-10-09: [[ FullscreenMode ]] Reimplement using MCStack::stacktogloballoc
	MCPoint t_loc;
	t_loc = MCPointMake(x, y);

	t_loc = stacktogloballoc(t_loc);
	t_loc = dest->globaltostackloc(t_loc);

	x = t_loc.x;
	y = t_loc.y;
}

uint4 MCStack::newid()
{
	return ++obj_id;
}

void MCStack::appendaclip(MCAudioClip *aptr)
{
	aptr->appendto(aclips);
	aptr->setid(newid());
	aptr->setparent(this);
    
    // AL-2014-11-27: [[ NewIdeMEssages ]] Send newAudioclip message
    aptr->message(MCM_new_audioclip);
}

void MCStack::removeaclip(MCAudioClip *aptr)
{
    // AL-2014-11-27: [[ NewIdeMEssages ]] Send deleteAudioclip message
    aptr->message(MCM_delete_audioclip);
	aptr->remove(aclips);
}

void MCStack::appendvclip(MCVideoClip *vptr)
{
	vptr->appendto(vclips);
	vptr->setid(newid());
	vptr->setparent(this);
    
    // AL-2014-11-27: [[ NewIdeMEssages ]] Send newVideoclip message
    vptr->message(MCM_new_videoclip);
}

void MCStack::removevclip(MCVideoClip *vptr)
{
    // AL-2014-11-27: [[ NewIdeMEssages ]] Send deleteVideoclip message
    vptr->message(MCM_delete_videoclip);
	vptr->remove(vclips);
}

void MCStack::appendcontrol(MCControl *optr)
{
	optr->appendto(controls);
}

void MCStack::removecontrol(MCControl *optr)
{
	optr->remove(controls);
}

void MCStack::appendcard(MCCard *cptr)
{
	cptr->setparent(this);
	if (cards == NULL)
		curcard = cards = cptr;
	else
	{
		curcard->append(cptr);
		setcard(cptr, True, False);
	}
	cptr->message(MCM_new_card);
}

void MCStack::removecard(MCCard *cptr)
{
	if (state & CS_IGNORE_CLOSE)
	{
		curcard = cptr->next();
		cptr->remove
		(cards);
		if (cards == NULL)
		{
			cards = curcard = MCtemplatecard->clone(False, False);
			cards->setparent(this);
		}
	}
	else
	{
		if (curcard == cptr)
			setcard(cptr->next(), True, False);
		if (curcard == cptr)
		{
			MCCard *newcard = MCtemplatecard->clone(False, False);
			newcard->setparent(this);
			newcard->appendto(cards);
			setcard(newcard, True, False);
		}
		cptr->remove(cards);
		dirtywindowname();
	}
	MCrecent->deletecard(cptr);
	MCcstack->deletecard(cptr);
}

MCObject *MCStack::getsubstackobjid(Chunk_term type, uint4 inid)
{
	MCStack *sptr = this;
	MCObject *optr = NULL;
	if (!MCdispatcher->ismainstack(this))
		sptr = parent->getstack();
	if (type == CT_AUDIO_CLIP || type == CT_VIDEO_CLIP)
		optr = sptr->getAVid(type, inid);
	else
		optr = sptr->getcontrolid(type, inid);
	if (optr != NULL)
		return optr;
	if (sptr->substacks != NULL)
	{
		MCStack *tptr = sptr->substacks;
		do
		{
			if (type == CT_AUDIO_CLIP || type == CT_VIDEO_CLIP)
				optr = tptr->getAVid(type, inid);
			else
				optr = tptr->getcontrolid(type, inid);
			if (optr != NULL)
				return optr;
			tptr = (MCStack *)tptr->next();
		}
		while (tptr != sptr->substacks);
	}
	return NULL;
}

MCObject *MCStack::getobjid(Chunk_term type, uint4 inid)
{
	if (inid == 0)
		return NULL;
	MCObject *optr = NULL;
	if (type == CT_AUDIO_CLIP || type == CT_VIDEO_CLIP)
		optr = getAVid(type, inid);
	else
		optr = getcontrolid(type, inid);
	if (optr != NULL)
		return optr;
	if ((optr = getsubstackobjid(type, inid)) != NULL)
		return optr;
	else
		return MCdispatcher->getobjid(type, inid);
}

MCObject *MCStack::getsubstackobjname(Chunk_term type, MCNameRef p_name)
{
	MCStack *sptr = this;
	MCObject *optr = NULL;
	if (!MCdispatcher->ismainstack(this))
		sptr = parent->getstack();
	if (type == CT_AUDIO_CLIP || type == CT_VIDEO_CLIP)
	{
		/* UNCHECKED */ sptr->getAVname(type, p_name, optr);
	}
	else
		optr = sptr->getcontrolname(type, p_name);
	if (optr != NULL)
		return optr;
	if (sptr->substacks != NULL)
	{
		MCStack *tptr = sptr->substacks;
		do
		{
			if (type == CT_AUDIO_CLIP || type == CT_VIDEO_CLIP)
			{
				/* UNCHECKED */ tptr->getAVname(type, p_name, optr);
			}
			else
				optr = tptr->getcontrolname(type, p_name);
			if (optr != NULL)
				return optr;
			tptr = (MCStack *)tptr->next();
		}
		while (tptr != sptr->substacks);
	}
	return NULL;
}


MCObject *MCStack::getobjname(Chunk_term type, MCNameRef p_name)
{
	MCObject *optr = NULL;
	uint4 iid;
	if (MCU_stoui4(MCNameGetString(p_name), iid))
	{
		optr = getobjid(type, iid);
		if (optr != NULL)
			return optr;
	}
	if (type == CT_AUDIO_CLIP || type == CT_VIDEO_CLIP)
	{
		/* UNCHECKED */ getAVname(type, p_name, optr);
	}
	else
	{
		optr = getcontrolname(type, p_name);
	}
	if (optr != NULL)
		return optr;
	if ((optr = getsubstackobjname(type, p_name)) != NULL)
		return optr;
	else
		return MCdispatcher->getobjname(type, p_name);
}


void MCStack::createmenu(MCControl *nc, uint2 width, uint2 height)
{
	// MW-2014-03-12: [[ Bug 11914 ]] Mark the stack as an engine menu.
	m_is_menu = true;

	allowmessages(False);
	flags &= ~F_RESIZABLE;
	rect.width = minwidth = maxwidth = width;
	rect.height = minheight = maxheight = height;
	controls = nc;
	curcard = cards = MCtemplatecard->clone(False, False);
	curcard->allowmessages(False);
	curcard->setsprop(P_SHOW_BORDER, MCSTR(MCtruestring));
	setsprop(P_COLORS, kMCEmptyString);
	if (nc->gettype() == CT_FIELD && IsMacLFAM() && MCaqua)
	{
		curcard->setsprop(P_BORDER_WIDTH, MCSTR("0"));
		uint2 i;
		MCObject *tparent = getparent();
		if  (!tparent->getcindex(DI_BACK, i) && !tparent->getpindex(DI_BACK,i))
			setsprop(P_BACK_COLOR,  MCSTR("255,255,255"));
	}
	else
		if ((nc->gettype() == CT_FIELD && MClook != LF_MOTIF)
		        || IsMacLF() || (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN))
		{
			curcard->setsprop(P_BORDER_WIDTH, MCSTR("1"));
			if (IsMacLF() || nc->gettype() == CT_FIELD || (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN))
				curcard->setsprop(P_3D, MCSTR(MCfalsestring));
		}
	
	MCWidgetInfo wmenu;
	wmenu.type = WTHEME_TYPE_MENU;
	if ( nc->gettype() != CT_FIELD && (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN))
	{
		uint2 i;
		MCObject *tparent = getparent();

		if  (!tparent->getcindex(DI_BACK, i) && !tparent->getpindex(DI_BACK,i))
        {
            MCAutoStringRef colorbuf;
            MCcurtheme->getthemecolor(wmenu, WCOLOR_BACK, &colorbuf);
			setsprop(P_BACK_COLOR, *colorbuf);
        }
		if  (!tparent->getcindex(DI_BORDER, i) && !tparent->getpindex(DI_BORDER,i))
        {
            MCAutoStringRef colorbuf;
            MCcurtheme->getthemecolor(wmenu, WCOLOR_BORDER, &colorbuf);
			setsprop(P_BORDER_COLOR, *colorbuf);
        }
		if  (!tparent->getcindex(DI_FORE, i) && !tparent->getpindex(DI_FORE,i))
        {
            MCAutoStringRef colorbuf;
            MCcurtheme->getthemecolor(wmenu, WCOLOR_TEXT, &colorbuf);
			setsprop(P_FORE_COLOR, *colorbuf);
        }
		if  (!tparent->getcindex( DI_HILITE, i) && !tparent->getpindex( DI_HILITE,i))
        {
            MCAutoStringRef colorbuf;
            MCcurtheme->getthemecolor(wmenu, WCOLOR_HILIGHT, &colorbuf);
			setsprop(P_HILITE_COLOR, *colorbuf);
        }
	}

	cards->setparent(this);
	updatecardsize();
	
	MCControl *cptr = nc;
	do
	{
		cptr->setid(newid());
		cptr->allowmessages(False);
		cards->newcontrol(cptr, False);
		cptr = (MCControl *)cptr->next();
	}
	while (cptr != nc);
}

void MCStack::menuset(uint2 button, uint2 defy)
{
	MCButton *bptr = (MCButton *)curcard->getnumberedchild(button, CT_BUTTON, CT_UNDEFINED);
	if (bptr == NULL)
	{
		lasty = defy;
		return;
	}
	MCRectangle trect = bptr->getrect();
	lasty = trect.y + (trect.height >> 1);
}

void MCStack::menumup(uint2 which, MCStringRef &r_string, uint2 &selline)
{
	// The original behaviour of this function interprets an empty string and
	// the null string as different things: the empty string means that the
	// function succeeded but there is no text while the null string indicates
	// that no menu handled the key event.
	r_string = nil;
	// IM-2014-08-06: [[ Bug 13059 ]] menumup() can be called on pop-up menu stacks
	// configured through menuname so remove check for m_is_menu
	{
        MCControl *focused = curcard->getmfocused();
        if (focused == NULL)
            focused = curcard->getkfocused();
        MCButton *bptr = (MCButton *)focused;
        if (focused != NULL && (focused->gettype() == CT_FIELD
                                || (focused->gettype() == CT_BUTTON
                                    && bptr->getmenumode() != WM_CASCADE)))
        {
            bool t_has_tags = bptr->getmenuhastags();
            
            MCExecContext ctxt(this, nil, nil);
            MCAutoStringRef t_label;
            MCStringRef t_name = nil;
            if (focused -> gettype() == CT_FIELD)
			/* UNCHECKED */ static_cast<MCField *>(focused) -> selectedtext(&t_label);
            else
                focused->getstringprop(ctxt, 0, P_LABEL, true, &t_label);
            t_name = MCNameGetString(focused->getname());
            if (!MCStringIsEmpty(t_name) && t_has_tags)
                r_string = MCValueRetain(t_name);
            else
                r_string = MCValueRetain(*t_label);
            
            /*		{
             // If the name exists, use it in preference to the label
             if (t_has_tags && !MCStringIsEqualTo(*t_label, t_name, kMCStringOptionCompareExact))
             r_string = MCValueRetain(t_name);
             else
             r_string = MCValueRetain(*t_label);
             }*/
            
            if (focused->gettype() == CT_FIELD)
            {
                MCField *f = (MCField *)focused;
                selline = f->hilitedline();
			}
			else
				curcard->count(CT_LAYER, CT_UNDEFINED, focused, selline, True);
		}
	}
	curcard->mup(which, false);
}


void MCStack::menukdown(MCStringRef p_string, KeySym key, MCStringRef &r_string, uint2 &selline)
{
	r_string = nil;
	// MW-2014-03-12: [[ Bug 11914 ]] Only do internal menu actions if this is an
	//   engine menu.
	if (m_is_menu)
	{
        MCControl *kfocused = curcard->getkfocused();
        if (kfocused != NULL)
        {
            // OK-2010-03-08: [[Bug 8650]] - Check its actually a button before casting,
            // with combo boxes on OS X this will be a field.
            if (kfocused ->gettype() == CT_BUTTON && ((MCButton*)kfocused)->getmenuhastags())
            {
                r_string = MCValueRetain(MCNameGetString(kfocused->getname()));
            }
            else
            {
                MCAutoStringRef t_string;
                MCExecContext ctxt(this, nil, nil);
                kfocused->getstringprop(ctxt, 0, P_LABEL, True, &t_string);
                r_string = MCValueRetain(*t_string);
                
			}
			curcard->count(CT_LAYER, CT_UNDEFINED, kfocused, selline, True);
        }
        
        curcard->kdown(p_string, key);
        curcard->kunfocus();
    }
	else
		curcard -> kdown(p_string, key);
}

void MCStack::findaccel(uint2 key, MCStringRef &r_pick, bool &r_disabled)
{
	if (controls != NULL)
	{
		MCButton *bptr = (MCButton *)controls;
		bool t_menuhastags = bptr->getmenuhastags();
		do
		{
			if (bptr->getaccelkey() == key && bptr->getaccelmods() == MCmodifierstate)
			{
				if (t_menuhastags)
					r_pick = MCValueRetain(MCNameGetString(bptr->getname()));
				else 
					r_pick = MCValueRetain(bptr->getlabeltext());
				r_disabled = bptr->isdisabled() == True;
				return;
			}
			if (bptr->getmenumode() == WM_CASCADE && bptr->getmenu() != NULL)
			{
				MCStringRef t_accel = nil;
				bptr->getmenu()->findaccel(key, t_accel, r_disabled);
				if (!MCStringIsEmpty(t_accel))
				{
					MCAutoStringRef t_label;
					/* UNCHECKED */ MCStringCreateMutable(0, &t_label);
					if (t_menuhastags)
						/* UNCHECKED */ MCStringAppend(*t_label, MCNameGetString(bptr->getname()));
					else
						/* UNCHECKED */ MCStringAppend(*t_label, bptr->getlabeltext());
					
					/* UNCHECKED */ MCStringAppendFormat(*t_label, "|%@", t_accel);

                    // SN-2014-09-11: [[ Bug 13405 ]] The string we just built should be returned.
                    // Changed to a stringCopy, to avoid returning a mutable string.
					MCStringCopy(*t_label, r_pick);
					return;
				}
			}
			bptr = (MCButton *)bptr->next();

		}
		while (bptr != controls);
	}
	r_pick = MCValueRetain(kMCEmptyString);
}

void MCStack::raise()
{
	MCscreen->raisewindow(window);
	MCscreen->waitconfigure(window);
}

void MCStack::enter()
{
	setcursor(getcursor(), False);
	curcard->message(MCM_mouse_enter);
}

void MCStack::flip(uint2 count)
{
	if (count)
	{
		MCCard *cptr = curcard;
		uint4 oldstate = state;
		state &= ~CS_MARKED;
		while (count)
		{
			cptr = (MCCard *)cptr->next();
			if (cptr->countme(0, (oldstate & CS_MARKED) != 0))
			{
				setcard(cptr, True, False);
				count--;
			}
			if (MCscreen->abortkey())
			{
				MCeerror->add(EE_REPEAT_ABORT, 0, 0);
				state = oldstate;
				return;
			}
		}
		state = oldstate;
	}
	else
	{
		// MW-2011-08-17: [[ Redraw ]] This shouldn't be reached anymore.
		abort();
	}
}

bool MCStack::sort(MCExecContext &ctxt, Sort_type dir, Sort_type form,
                        MCExpression *by, Boolean marked)
{
	if (by == NULL)
		return false;
	if (editing != NULL)
		stopedit();

	MCStack *olddefault = MCdefaultstackptr;
	MCdefaultstackptr = this;
	MCCard *cptr = curcard;
	MCAutoArrayZeroedNonPod<MCSortnode> items;
	uint4 nitems = 0;
	MCerrorlock++;
    
    extern void MCStringsSortAddItem(MCExecContext &ctxt, MCSortnode *items, uint4 &nitems, int form, MCValueRef p_input, MCExpression *by);
    extern void MCStringsSort(MCSortnode *p_items, uint4 nitems, Sort_type p_dir, Sort_type p_form, MCStringOptions p_options);
    
	do
	{
		items.Extend(nitems + 1);
		items[nitems].data = (void *)curcard;
        
        if (!marked || curcard->getmark())
            MCStringsSortAddItem(ctxt, items . Ptr(), nitems, form, nil, by);
        else
        {
            if (form == ST_DATETIME || form == ST_NUMERIC)
                /* UNCHECKED */ MCNumberCreateWithReal(-MAXREAL8, items[nitems].nvalue);
            else if (form == ST_BINARY)
                items[nitems] . dvalue = MCValueRetain(kMCEmptyData);
            else
                items[nitems] . svalue = MCValueRetain(kMCEmptyString);
            // AL-2014-05-20: [[ Bug 12457 ]] Increment items when 'default' item is added
            nitems++;
        }
		curcard = (MCCard *)curcard->next();
	}
	while (curcard != cptr);
	MCerrorlock--;
	if (nitems > 1)
		MCStringsSort(items . Ptr(), nitems, dir, form, ctxt . GetStringComparisonType());
	MCCard *newcards = NULL;
	uint4 i;
	for (i = 0 ; i < nitems ; i++)
	{
		const MCCard *tcptr = (const MCCard *)items[i].data;
		cptr = (MCCard *)tcptr;
		cptr->remove(cards);
		cptr->appendto(newcards);
	}
	cards = newcards;
	setcard(cards, True, False);
	dirtywindowname();
	MCdefaultstackptr = olddefault;
    
    // SN-2014-03-21: [[ Bug 11953 ]] sort card of stack crashes
    // This function must discard a bad target expression error, since a -MAXREAL8/empty string  is provided in that case
    ctxt . IgnoreLastError();
	return true;
}

void MCStack::breakstring(MCStringRef source, MCStringRef*& dest, uindex_t &nstrings, Find_mode fmode)
{
    MCAutoArray<MCStringRef> t_strings;
    nstrings = 0;
	switch (fmode)
	{
	case FM_NORMAL:
	case FM_CHARACTERS:
	case FM_WORD:
        {
			// MW-2007-07-05: [[ Bug 110 ]] - Break string only breaks at spaces, rather than space charaters 
			uint4 l;
            l = 0;
            uint4 remaining_chars;
            remaining_chars = MCStringGetLength(source);
			MCU_skip_spaces(source, l);
            remaining_chars -= l;
            uindex_t t_word_start;
            t_word_start = l;
            
			while(remaining_chars > 0)
			{
				while(remaining_chars > 0 && !MCUnicodeIsWhitespace(MCStringGetCharAtIndex(source, l)))
                {
					l++;
                    remaining_chars--;
                }
                
                MCStringRef t_word;
                uindex_t t_word_length;
                t_word_length = l - t_word_start;
                nstrings++;
                /* UNCHECKED */ MCStringCopySubstring(source, MCRangeMake(t_word_start, t_word_length), t_word);
				t_strings . Push(t_word);
        
                uindex_t t_space_start, t_spaces_length;
                t_space_start = l;
				MCU_skip_spaces(source, l);
                t_word_start = l;
                t_spaces_length = t_word_start - t_space_start;
                remaining_chars -= t_spaces_length;
			}
		}
		break;
	case FM_STRING:
	case FM_WHOLE:
	default:
        nstrings = 0;
		break;
    }

    if (nstrings == 0)
        t_strings . Push(MCValueRetain(source));

    t_strings . Take(dest, nstrings);
}

Boolean MCStack::findone(MCExecContext &ctxt, Find_mode fmode,
                         MCStringRef *strings, uint2 nstrings,
                         MCChunk *field, Boolean firstcard)
{
	Boolean firstword = True;
	uint2 i = 0;
	if (field != NULL)
	{
		MCObject *optr;
		uint4 parid;
		MCerrorlock++;
		// IM-2016-11-15: [[ Bug 14080 ]] Create new execcontext so errors in getobj don't affect subsequent evaluations
		MCExecContext t_context(ctxt);
		if (field->getobj(t_context, optr, parid, True))
		{
			// IM-2016-11-14: [[ Bug 18666 ]] If the resolved object is not on the current card then don't search it.
			if (parid != 0 && parid != curcard->getid())
			{
				MCerrorlock--;
				return False;
			}
			
			if (optr->gettype() == CT_FIELD)
			{
				MCField *searchfield = (MCField *)optr;
				while (i < nstrings)
					if (!searchfield->find(t_context, curcard->getid(), fmode, strings[i], firstword))
					{
						MCerrorlock--;
						return False;
					}
					else
					{
						firstword = False;
						i++;
					}
				MCerrorlock--;
				return True;
			}
		}
		MCerrorlock--;
		return False;
	}
	else
	{
		while (i < nstrings)
			if (!curcard->find(ctxt, fmode, strings[i], firstcard, firstword))
				return False;
			else
			{
				firstword = False;
				i++;
			}
		return True;
	}
}

void MCStack::find(MCExecContext &ctxt, Find_mode fmode,
                   MCStringRef tofind, MCChunk *field)
{
	MCStringRef *strings = NULL;
	uindex_t nstrings;
	breakstring(tofind, strings, nstrings, fmode);
	MCCard *ocard = curcard;
	Boolean firstcard = MCfoundfield.IsValid();
	MCField *oldfound = MCfoundfield;
	do
	{
		if (findone(ctxt, fmode, strings, nstrings, field, firstcard))
		{
			for (uindex_t i = 0; i < nstrings; i++)
                MCValueRelease(strings[i]);
            delete strings;
            
			MCField *newfound = MCfoundfield;
			if (curcard != ocard)
			{
				MCCard *newcard = curcard;
				curcard = ocard;
				MCfoundfield = nil;
				setcard(newcard, True, False);
				MCfoundfield = newfound;
			}
			if (opened)
				MCfoundfield->centerfound();
			MCresult->clear(False);
			return;
		}
		firstcard = False;
		if (MCfoundfield)
			MCfoundfield->clearfound();
		if (oldfound != NULL)
		{
			oldfound->clearfound();
			oldfound = NULL;
		}
		curcard = (MCCard *)curcard->next();
	}
	while (curcard != ocard);
	curcard = ocard;
	// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
	dirtyall();
    for (int i = 0 ; i < nstrings ; i++)
        MCValueRelease(strings[i]);
	delete strings;
	MCresult->sets(MCnotfoundstring);
}

void MCStack::markfind(MCExecContext &ctxt, Find_mode fmode,
                       MCStringRef tofind, MCChunk *field, Boolean mark)
{
	if (MCfoundfield)
		MCfoundfield->clearfound();
	MCStringRef *strings = NULL;
	uindex_t nstrings;
	breakstring(tofind, strings, nstrings, fmode);
	MCCard *ocard = curcard;
	do
	{
		if (findone(ctxt, fmode, strings, nstrings, field, False))
		{
			MCfoundfield->clearfound();
			curcard->setmark(mark);
		}
		curcard = (MCCard *)curcard->next();
	}
	while (curcard != ocard);
    for (uint4 i = 0 ; i < nstrings ; i++)
        MCValueRelease(strings[i]);
	delete strings;
	if (MCfoundfield)
		MCfoundfield->clearfound();
}

void MCStack::mark(MCExecContext& ctxt, MCExpression *p_where, bool p_mark)
{
	if (p_where == nil)
	{
		MCCard *cptr = cards;
		do
		{
			cptr->setmark(p_mark);
			cptr = (MCCard *)cptr->next();
		}
		while (cptr != cards);
	}
	else
	{
		MCCard *oldcard = curcard;
		curcard = cards;
		MCerrorlock++;
		do
		{
            MCAutoBooleanRef t_condition;
            if (ctxt . EvalExprAsBooleanRef(p_where, EE_MARK_BADSTRING, &t_condition))
			{
                if (*t_condition == kMCTrue)
					curcard->setmark(p_mark);
			}
			curcard = (MCCard *)curcard->next();
		}
		while (curcard != cards);
		curcard = oldcard;
		MCerrorlock--;
	}
}

Linkatts *MCStack::getlinkatts()
{
	return linkatts != NULL ? linkatts : &MClinkatts;
}

////////////////////////////////////////////////////////////////////////////////

// MW-2009-01-28: [[ Inherited parentScripts ]]
// This method returns false if there was not enough memory to complete the
// resolution.
bool MCStack::resolveparentscripts(void)
{
	// MW-2010-01-08: [[ Bug 8280 ]] Make sure the stack resolves its own behavior!
	resolveparentscript();

	if (cards != NULL)
	{
		MCCard *t_card;
		t_card = cards;
		do
		{
			if (!t_card -> resolveparentscript())
				return false;
			t_card = t_card -> next();
		}
		while(t_card != cards);
	}

	if (controls != NULL)
	{
		MCControl *t_control;
		t_control = controls;
		do
		{
			if (!t_control -> resolveparentscript())
				return false;
			t_control = t_control -> next();
		}
		while(t_control != controls);
	}

	if (substacks != NULL)
	{
		MCStack *t_substack;
		t_substack = substacks;
		do
		{
			if (!t_substack -> resolveparentscripts())
				return false;
			t_substack = t_substack -> next();
		}
		while(t_substack != substacks);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

struct MCGroupPlacementCount
{
	uint4 id;
	uint4 count;
	MCControl *group;
};

static int32_t MCGroupPlacementCountCompare(const void *a, const void *b)
{
	return ((MCGroupPlacementCount *)a) -> id - ((MCGroupPlacementCount *)b) -> id;
}

// MW-2011-08-09: [[ Groups ]] This method ensures that F_GROUP_SHARED is set
//   appropriately on all shared groups. This process is run on every stack
//   after load - note that it only ever *sets* F_GROUP_SHARED, it doesn't
//   ever unset it.
void MCStack::checksharedgroups(void)
{
	// No cards means nothing to do.
	if (cards == nil || controls == nil)
		return;

	MCGroupPlacementCount *t_groups;
	uint32_t t_group_count;
	t_groups = nil;
	t_group_count = 0;
	
	// First make a list of all the non-shared groups on the stack (we don't need
	// to consider shared groups as we might be turning groups into sg's as a result
	// of this method!).
	MCControl *t_control;
	t_control = controls;
	do
	{
		if (t_control -> gettype() == CT_GROUP && !static_cast<MCGroup *>(t_control) -> isshared())
        {
			if (MCMemoryResizeArray(t_group_count + 1, t_groups, t_group_count))
			{
				t_groups[t_group_count - 1] . id = t_control -> getid();
				t_groups[t_group_count - 1] . group = t_control;
			}
			else
			{
				// If resizing the array failed, then we must use a quadratic
				// algorithm for checking.
				MCMemoryDeleteArray(t_groups);
				checksharedgroups_slow();
				return;
			}
        }
		t_control = t_control -> next();
	}
	while(t_control != controls);
	
	// Sort the list of groups by id
	qsort(t_groups, t_group_count, sizeof(MCGroupPlacementCount), MCGroupPlacementCountCompare);
	
	// Next loop through all cards, and their objptrs
	MCCard *t_card;
	t_card = cards;
	do
	{
		MCObjptr *t_refs;
		t_refs = t_card -> getobjptrs();
		if (t_refs != nil)
		{
			MCObjptr *t_ref;
			t_ref = t_refs;
			do
			{
				uint4 t_id;
				t_id = t_ref -> getid();
				
				MCGroupPlacementCount t_key;
				t_key . id = t_id;
				t_key . count = 0;
				t_key . group = nil;
				
				MCGroupPlacementCount *t_group;
				t_group = (MCGroupPlacementCount *)bsearch(&t_key, t_groups, t_group_count, sizeof(MCGroupPlacementCount), MCGroupPlacementCountCompare);
				if (t_group != nil)
					t_group -> count += 1;
				
				t_ref = t_ref -> next();
			}
			while(t_ref != t_refs);
		}
		
		t_card = t_card -> next();
	}
	while(t_card != cards);

	// Loop through all the unshared groups we listed before, applying the shared
	// flag for any that do not have a count of 1.
	for(uint32_t i = 0; i < t_group_count; i++)
		if (t_groups[i] . count != 1)
			t_groups[i] . group -> setflag(True, F_GROUP_SHARED);
	
	MCMemoryDeleteArray(t_groups);
}

// This method performs the same task as 'checksharedgroups' except that it uses
// no extra memory to do so.
void MCStack::checksharedgroups_slow(void)
{
	// Loop over all controls in the stack.
	MCControl *t_control;
	t_control = controls;
	do
	{
		// Check to see if the current control is a group.
		MCGroup *t_group;
		t_group = nil;
		if (t_control -> gettype() == CT_GROUP)
			t_group = static_cast<MCGroup *>(t_control);

		// Advance 't_control' (so we can use continue for fall through).
		t_control = t_control -> next();

		// If this wasn't a group, continue.
		if (t_group == nil)
			continue;

		// If the group is already shared, continue.
		if (t_group -> isshared())
			continue;

		// The number of finds of the group we've had.
		int t_found;
		t_found = 0;

		// Loop through all cards.
		MCCard *t_card;
		t_card = cards;
		do
		{
			// Look to see if the group is on the card.
			if (t_card -> getchildid(t_group -> getid()))
			{
				t_found += 1;

				// If we've found the group more than once, we need search no
				// more.
				if (t_found > 1)
					break;
			}

			// Advance to the next card
			t_card = t_card -> next();
		}
		while(t_card != cards);

		// If the group was not found, or it was found more than once it must
		// be shared.
		if (t_found != 1)
			t_group -> setflag(True, F_GROUP_SHARED);
	}
	while(t_control != controls);
}

////////////////////////////////////////////////////////////////////////////////
