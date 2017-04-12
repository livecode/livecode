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


#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "image.h"
#include "field.h"
#include "scrolbar.h"
#include "graphic.h"
#include "player.h"
#include "eps.h"
#include "magnify.h"
#include "aclip.h"
#include "cpalette.h"
#include "vclip.h"
#include "MCBlock.h"
#include "cdata.h"
#include "styledtext.h"
#include "globals.h"
#include "font.h"
#include "stacksecurity.h"
#include "widget.h"

#include "stackfileformat.h"

///////////////////////////////////////////////////////////////////////////////
//
//  The MCPickleContext structure is an opaque type used by the state-based
//  pickling operations. It records the current output stream and error state.
//
struct MCPickleContext
{
	IO_handle stream;
	bool error;
    
    // AL-2014-02-14: [[ UnicodeFileFormat ]] If 'include_legacy' is true then
	// 2.7, 5.5 and 7.0 versions will be pickled.
	bool include_legacy;
};

///////////////////////////////////////////////////////////////////////////////
//
//  startpickling is used to start a pickling stream. It returns a context that
//  can be used successively by calls to continuepickling to serialize a
//  sequence of objects.
//
//  If 'include_legacy' is true then each object will be serialized as first a
//  2.7 version, then a 5.5 version, then a 7.0 version. This works with older
//  versions of the engine which will only read as far as the correct version.
//

MCPickleContext *MCObject::startpickling(bool p_include_legacy)
{
	bool t_success;
	t_success = true;

	MCPickleContext *t_context;
	t_context = NULL;
	if (t_success)
	{
		t_context = new (nothrow) MCPickleContext;
		if (t_context == NULL)
			t_success = false;
	}

	IO_handle t_stream;
	t_stream = NULL;
	if (t_success)
	{
		t_stream = MCS_fakeopenwrite();
		if (t_stream == NULL)
			t_success = false;
	}

	if (t_success)
	{
		t_context -> stream = t_stream;
		t_context -> error = false;
		
		// MW-2012-03-04: [[ UnicodeFileFormat ]] Pass the 'include_legacy' flag
		//   through.
		t_context -> include_legacy = p_include_legacy;
	}
	else
	{
		delete t_context;
		t_context = NULL;
	}

	return t_context;
}

///////////////////////////////////////////////////////////////////////////////
//
//  stoppickling is used to end a sequence of pickling operations. It returns
//  an MCSharedString containing the serialized object data.
//
void MCObject::stoppickling(MCPickleContext *p_context, MCDataRef& r_string)
{
	// Since we can use the start, continue and stop pickling methods without
	// checking for errors, exit if the context is NULL.
	if (p_context == NULL)
		return;

	bool t_success;
	t_success = true;

    // FG-2014-08-07: [[ Bugfix 13070 ]]
    // Don't reinterpret_cast a uint32_t& to a size_t& - it doesn't work on 64-bit
	void* t_buffer;
	size_t t_size;
	t_buffer = NULL;
	t_size = 0;
	if (t_success)
	{
		if (MCS_closetakingbuffer(p_context -> stream, t_buffer, t_size) != IO_NORMAL)
			t_success = false;
	}

	if (t_success)
		t_success = MCDataCreateWithBytesAndRelease((char_t *)t_buffer, t_size, r_string);

	if (!t_success)
	{
		if (t_buffer != NULL)
			free(t_buffer);
	}

	delete p_context;
}

// AL-2014-02-14: [[ UnicodeFileFormat ]] Write an object to the given stream
static IO_stat pickle_object_to_stream(IO_handle p_stream, uint32_t p_version, MCObject* p_object, uint4 p_part)
{
    IO_stat t_stat;
    t_stat = IO_NORMAL;
    
	// Write out the charset byte
	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint1(CHARSET, p_stream);
    
	// MW-2012-02-17: [[ LogFonts ]] Build the logical font table for the
	//   object and its children and serialize it.
	if (t_stat == IO_NORMAL)
	{
		// Build the logical font table for the object.
		MCLogicalFontTableBuild(p_object, p_part);
        
		// Write out the font table to the stream.
		MCLogicalFontTableSave(p_stream, p_version);
	}
    
	// Write the object
	if (t_stat == IO_NORMAL)
		t_stat = p_object -> save(p_stream, p_part, false, p_version);
    
	// If the object is a card, pickle all the objects it references
	// immediately after the main object.
	if (t_stat == IO_NORMAL && p_object -> gettype() == CT_CARD)
		t_stat = static_cast<MCCard *>(p_object) -> saveobjects(p_stream, p_part, p_version);
    
	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint1(OT_END, p_stream);
    
    return t_stat;
}

// Convert the given object to a byte-stream that can be read at a later date
// to recreate it exactly.
//
// This routine uses the 'save' method to store the on-disk form of the object
// into a shared string.
//
// The format is:
//   Header: 'REVO'
//   Version: '2700'
//   Size: uint4 in network byte-order
//   Font Table Size: uint2 in network byte-order
//   Array of Fonts:
//     Index: uint2 in network byte-order
//     Size: uint2 in network byte-order
//     Style: uint2 in network byte-order
//     Name: medium string
//   <object byte stream>
//
//   A medium string consists of:
//     Length: uint2 in network byte-order (length of string + 1)
//     Bytes: 'Length' bytes the last is '\0' if the string
//       was not truncated.
//
void MCObject::continuepickling(MCPickleContext *p_context, MCObject *p_object, uint4 p_part)
{
	if (p_context == NULL)
		return;

	IO_stat t_stat;
	t_stat = IO_NORMAL;

	IO_handle t_stream;
	t_stream = p_context -> stream;
	
	// MW-2012-02-22; [[ NoScrollSave ]] Initialize the grouped object offset
	//   to zero.
	MCgroupedobjectoffset . x = 0;
	MCgroupedobjectoffset . y = 0;
	
    // MW-2014-12-17: [[ Widgets ]] If the object is or contains widgets, we can
    //   only produce 8.0 version data.
    uint4 t_chunk_start;
	
	bool t_include_legacy;
	t_include_legacy = p_context->include_legacy;
	
	uint32_t t_min_version;
	t_min_version = p_object->geteffectiveminimumstackfileversion();
	
	// Cut off point for legacy versions
	if (t_min_version > kMCStackFileFormatVersion_7_0)
		t_include_legacy = false;
	
    if (!t_include_legacy)
    {
		const char *t_header;
		uint32_t t_header_size;
		
		MCStackFileGetHeaderForVersion(kMCStackFileFormatCurrentVersion, t_header, t_header_size);
		
        if (t_stat == IO_NORMAL)
            t_stat = IO_write(t_header, t_header_size, 1, t_stream);
        
        // Write the space for the chunk size field
        if (t_stat == IO_NORMAL)
            t_stat = IO_write_uint4(0, t_stream);
        
        // Record where we are now in the stream
        if (t_stat == IO_NORMAL)
            t_chunk_start = MCS_tell(t_stream);
        
        if (t_stat == IO_NORMAL)
            t_stat = pickle_object_to_stream(t_stream, kMCStackFileFormatCurrentVersion, p_object, p_part);
    }
    else
    {
		// Legacy - write version header for 2.7
        if (t_stat == IO_NORMAL)
            t_stat = IO_write(kMCStackFileVersionString_2_7, kMCStackFileVersionStringLength, 1, t_stream);

        // Write the space for the chunk size field
        if (t_stat == IO_NORMAL)
            t_stat = IO_write_uint4(0, t_stream);

        // Record where we are now in the stream
        if (t_stat == IO_NORMAL)
            t_chunk_start = MCS_tell(t_stream);

        if (t_stat == IO_NORMAL)
            t_stat = pickle_object_to_stream(t_stream, kMCStackFileFormatVersion_2_7, p_object, p_part);

        // MW-2012-03-04: [[ UnicodeFileFormat ]] If we are including 2.7, 5.5 and 7.0, now write
        //   out the 5.5 and 7.0 versions.
        if (t_stat == IO_NORMAL)
        {
            t_stat = pickle_object_to_stream(t_stream, kMCStackFileFormatVersion_5_5, p_object, p_part);
            
            if (t_stat == IO_NORMAL)
                pickle_object_to_stream(t_stream, kMCStackFileFormatVersion_7_0, p_object, p_part);
        }
    }
    
	// Write back the length of the chunk at the recorded position
	if (t_stat == IO_NORMAL)
	{
		uint4 t_chunk_size;
		t_chunk_size = MCS_tell(t_stream) - t_chunk_start;
		swap_uint4(&t_chunk_size);
		MCS_writeat(&t_chunk_size, 4, t_chunk_start - 4, t_stream);
	}

	if (t_stat != IO_NORMAL)
		p_context -> error = true;

	// MW-2012-02-17: [[ LogFonts ]] Clean up the font table.
	MCLogicalFontTableFinish();
}

// Do a start/continue/stop pickling call all in one.
void MCObject::pickle(MCObject *p_object, uint4 p_part, MCDataRef& r_data)
{
	MCPickleContext *t_context;
	// AL-2014-02-14: [[ UnicodeFileFormat ]] The pickle method is only used for
	//   internal purposes, and we don't want to include 2.7 or 5.5 versions in this
	//   case - so pass 'false' for include_legacy.
	t_context = startpickling(false);
	continuepickling(t_context, p_object, p_part);
	stoppickling(t_context, r_data);
}

// This visitor iterates over all objects and does the follows:
//   - ensures that non-shared data has the right id
//
struct UnpickleVisitor: public MCObjectVisitor
{
	unsigned int card;

	UnpickleVisitor(unsigned int p_card)
		: card(p_card)
	{
	}

	bool OnField(MCField *p_field)
	{
		if (!p_field -> getflag(F_SHARED_TEXT))
		{
			MCCdata *t_data;
			t_data = p_field -> getcarddata(0);
			if (t_data != NULL)
				t_data -> setid(card);
		}

		return true;
	}

	bool OnButton(MCButton *p_button)
	{
		if (!p_button -> getflag(F_SHARED_HILITE))
		{
			MCCdata *t_data;
			t_data = p_button -> getdata(0, False);
			if (t_data != NULL)
				t_data -> setid(card);
		}

		return true;
	}
};

// MW-2012-03-04: [[ StackFile5500 ]] Read in an object from the given stream - it
//   will be prepared to be attached to the given stack.
static bool unpickle_object_from_stream(IO_handle p_stream, uint32_t p_version, MCStack *p_stack, MCObject*& r_object)
{
	bool t_success;
	t_success = true;

	uint1 t_charset;
	t_charset = 0;
	if (t_success)
	{
		if (IO_read_uint1(&t_charset, p_stream) != IO_NORMAL)
			t_success = false;
		else
			MCtranslatechars = CHARSET != t_charset;
	}

	// MW-2012-02-17: [[ LogFonts ]] Load the font table for the object.
	if (t_success)
	{
		if (MCLogicalFontTableLoad(p_stream, p_version) != IO_NORMAL)
			t_success = false;
	}

	uint1 t_object_type;
	if (t_success)
	{
		if (IO_read_uint1(&t_object_type, p_stream) != IO_NORMAL)
			t_success = false;
	}

	MCObject *t_object;
	t_object = NULL;
	if (t_success)
	{
		switch(t_object_type)
		{
		case OT_STACK:
		case OT_ENCRYPT_STACK:
			/* UNCHECKED */ MCStackSecurityCreateStack((MCStack*&)t_object);
		break;
		case OT_CARD:
			t_object = new (nothrow) MCCard;
		break;
		case OT_GROUP:
			t_object = new (nothrow) MCGroup;
		break;
		case OT_BUTTON:
			t_object = new (nothrow) MCButton;
		break;
		case OT_FIELD:
			t_object = new (nothrow) MCField;
		break;
		case OT_IMAGE:
			t_object = new (nothrow) MCImage;
		break;
		case OT_SCROLLBAR:
			t_object = new (nothrow) MCScrollbar;
		break;
		case OT_GRAPHIC:
			t_object = new (nothrow) MCGraphic;
		break;
		case OT_PLAYER:
			t_object = new (nothrow) MCPlayer;
		break;
		case OT_MCEPS:
			t_object = new (nothrow) MCEPS;
        break;
        case OT_WIDGET:
            t_object = new (nothrow) MCWidget;
        break;
		case OT_MAGNIFY:
			t_object = new (nothrow) MCMagnify;
		break;
		case OT_COLORS:
			t_object = new (nothrow) MCColors;
		break;
		case OT_AUDIO_CLIP:
			t_object = new (nothrow) MCAudioClip;
		break;
		case OT_VIDEO_CLIP:
			t_object = new (nothrow) MCVideoClip;
		break;
		case OT_STYLED_TEXT:
			t_object = new (nothrow) MCStyledText;
		break;
		default:
			t_success = false;
		break;
		}
	}

	if (t_success)
	{
		// MW-2007-12-11: [[ Bug 5671 ]] Make sure stack objects are unpickled correctly
		if (t_object -> gettype() == CT_STACK)
		{
			if (static_cast<MCStack *>(t_object) -> load(p_stream, p_version, t_object_type) != IO_NORMAL)
				t_success = false;
		}
		else if (t_object -> load(p_stream, p_version) != IO_NORMAL)
			t_success = false;
	}

	if (t_success)
	{
		if (t_object -> gettype() == CT_CARD)
		{
			if (static_cast<MCCard *>(t_object) -> loadobjects(p_stream, p_version) != IO_NORMAL)
				t_success = false;
		}
	}
	
	if (t_success)
	{
		uint1 t_end_tag;
		if (IO_read_uint1(&t_end_tag, p_stream) != IO_NORMAL ||
			t_end_tag != OT_END)
			t_success = false;
	}

	if (t_success)
	{
		// MW-2008-03-17: Make sure we can update the data attached to non-shared objects on cards
		//   correctly.
		uint4 t_id;
		if (t_object -> gettype() == CT_CARD)
			t_id = 0;
		else
			t_id = p_stack -> getcard() -> getid();

		UnpickleVisitor t_visitor(t_id);
		if (!t_object -> visit(VISIT_STYLE_DEPTH_LAST, 0, &t_visitor))
			t_success = false;
	}
	
	if (t_success)
		r_object = t_object;
	else
		delete t_object;
	
	return t_success;
}

// Unpickle a byte-stream into a linked-list of objects.
//
MCObject *MCObject::unpickle(MCDataRef p_data, MCStack *p_stack)
{
	bool t_success;
	t_success = true;

	const char *t_buffer;
	uint4 t_length;
	t_buffer = (const char *)MCDataGetBytePtr(p_data);
	t_length = MCDataGetLength(p_data);

	MCObject *t_result;
	t_result = NULL;

	while(t_length > 0 && t_success)
	{
		if (t_success)
			t_success = t_length >= 12;
		
		uint32_t t_version;
		t_version = 0;
		if (t_success)
			t_success = MCStackFileParseVersionNumber(t_buffer, t_version);
		
		// version should be 2.7 or greater
		if (t_success)
		{
			t_success = t_version >= kMCStackFileFormatVersion_2_7 && t_version <= kMCStackFileFormatCurrentVersion;
			
			t_buffer += kMCStackFileVersionStringLength;
			t_length -= kMCStackFileVersionStringLength;
		}
		
		uint4 t_chunk_length;
		t_chunk_length = 0;
		if (t_success)
		{
			memcpy(&t_chunk_length, t_buffer, 4);
			swap_uint4(&t_chunk_length);
			t_buffer += 4;
			t_length -= 4;
			if (t_chunk_length > t_length)
				t_success = false;
		}

		IO_handle t_stream;
		t_stream = NULL;
        
		if (t_success)
		{
            t_stream = MCS_fakeopen((const char *)t_buffer, t_chunk_length);
			if (t_stream == NULL)
				t_success = false;
		}

        // AL-2014-02-14: [[ UnicodeFileFormat ]] Unpickle the first version in the chunk.
		//   If there is no 2.7, then there will be a single version; otherwise it is the
		//   2.7 version preceeding the 5.5 and 7.0 ones.
		MCObject *t_object;
		t_object = nil;
		if (t_success)
			t_success = unpickle_object_from_stream(t_stream, t_version, p_stack, t_object);
			
        // AL-2014-02-14: [[ UnicodeFileFormat ]] If the header was 2.7, then there could
		//   be 5.5 and 7.0 versions following it. So attempt to unpickle second and third
		//   versions, and use them if present.
		if (t_success && t_version == kMCStackFileFormatVersion_2_7)
		{
			MCObject *t_5500_object;
			if (unpickle_object_from_stream(t_stream, kMCStackFileFormatVersion_5_5, p_stack, t_5500_object))
			{
				delete t_object;
				t_object = t_5500_object;
			}
            MCObject *t_7000_object;
			if (unpickle_object_from_stream(t_stream, kMCStackFileFormatVersion_7_0, p_stack, t_7000_object))
			{
				delete t_object;
				t_object = t_7000_object;
			}
		}

		if (t_success)
		{
			t_object -> appendto(t_result);

			t_length -= t_chunk_length;
			t_buffer += t_chunk_length;
		}

		// MW-2012-02-17: [[ LogFonts ]] Cleanup the font table.
		MCLogicalFontTableFinish();

		if (t_stream != NULL)
			MCS_close(t_stream);
	}

	if (!t_success)
	{
		while(t_result != NULL)
		{
			MCObject *t_object;
			t_object = t_result -> remove(t_result);
			delete t_object;
		}
	}

	return t_result;
}

///////////////////////////////////////////////////////////////////////////////
