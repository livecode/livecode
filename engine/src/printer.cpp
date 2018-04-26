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
#include "parsedef.h"
#include "objdefs.h"

#include "card.h"
#include "globals.h"

#include "stack.h"
#include "mcerror.h"
#include "util.h"

#include "variable.h"
#include "player.h"

#include "context.h"

#include "printer.h"

#include "mode.h"

extern char *strndup(const char *p_string, unsigned int p_length);

MCPrinter::MCPrinter(void)
{
}

MCPrinter::~MCPrinter(void)
{
}

void MCPrinter::Initialize(void)
{
	m_device_output_type = PRINTER_OUTPUT_DEVICE;
	m_device_output_location = NULL;
	m_device_command = NULL;
	m_device_font_table = NULL;
	m_device_features = 0;
	MCU_set_rect(m_device_rectangle, 0, 0, PRINTER_DEFAULT_PAGE_WIDTH, PRINTER_DEFAULT_PAGE_HEIGHT);
	
	m_page_width = PRINTER_DEFAULT_PAGE_WIDTH;
	m_page_height = PRINTER_DEFAULT_PAGE_HEIGHT;
	m_page_left_margin = 72;
	m_page_top_margin = 72;
	m_page_right_margin = 72;
	m_page_bottom_margin = 72;
	m_page_orientation = PRINTER_DEFAULT_PAGE_ORIENTATION;
	m_page_scale = 1.0;
	
	m_job_copies = PRINTER_DEFAULT_JOB_COPIES;
	m_job_collate = PRINTER_DEFAULT_JOB_COLLATE;
	m_job_name = NULL;
	m_job_duplex = PRINTER_DEFAULT_JOB_DUPLEX;
	m_job_color = PRINTER_DEFAULT_JOB_COLOR;
	m_job_range_count = PRINTER_PAGE_RANGE_ALL;
	m_job_ranges = NULL;
	
	m_layout_show_borders = false;
	m_layout_row_spacing = 36;
	m_layout_column_spacing = 36;
	m_layout_rows_first = true;
	m_layout_scale = 1.0;

	m_loop_nesting = 0;
	m_loop_layout_x = 0;
	m_loop_layout_y = 0;
	m_loop_layout_delta = 0;
	m_loop_error = NULL;
	m_loop_status = STATUS_READY;
	m_loop_page = 0;
	
	m_device = NULL;
	
	m_resync = true;

	DoInitialize();
}

void MCPrinter::Finalize(void)
{
	DoFinalize();

	delete m_device_output_location;
	m_device_output_location = NULL;
	
	delete m_device_command;
	m_device_command = NULL;
	
	delete m_device_font_table;
	m_device_font_table = NULL;
	
	delete m_job_name;
	m_job_name = NULL;
	
	delete m_job_ranges;
	m_job_ranges = NULL;

	delete m_loop_error;
	m_loop_error = NULL;
}

////////////////////////////////////////////////////////////////////////////////

bool MCPrinter::ChoosePaper(bool p_window_modal, MCStringRef &r_result)
{
	if (MCsystemFS && MCscreen -> hasfeature(PLATFORM_FEATURE_OS_PRINT_DIALOGS))
	{
		Window t_owner;
		t_owner = MCModeGetParentWindow();

		switch(DoPageSetup(p_window_modal, t_owner))
		{
		case PRINTER_DIALOG_RESULT_CANCEL:
			return MCStringCreateWithCString("cancel", r_result);

		case PRINTER_DIALOG_RESULT_ERROR:
			return MCStringCreateWithCString("unable to open dialog", r_result);

		case PRINTER_DIALOG_RESULT_OKAY:
			r_result = MCValueRetain(kMCEmptyString);
			return true;
		}

		MCAssert(false);
		return false;
	}
	else
		return MCStringCreateWithCString("non-system page setup dialogs not supported", r_result);
}

bool MCPrinter::ChoosePrinter(bool p_window_modal, MCStringRef &r_result)
{
	if (MCsystemFS && MCscreen -> hasfeature(PLATFORM_FEATURE_OS_PRINT_DIALOGS))
	{
		Window t_owner;
		t_owner = MCModeGetParentWindow();

		switch(DoPrinterSetup(p_window_modal, t_owner))
		{
		case PRINTER_DIALOG_RESULT_CANCEL:
			return MCStringCreateWithCString("cancel", r_result);

		case PRINTER_DIALOG_RESULT_ERROR:
			return MCStringCreateWithCString("unable to open dialog", r_result);

		case PRINTER_DIALOG_RESULT_OKAY:
			r_result = MCValueRetain(kMCEmptyString);
			return true;
		}

		MCAssert(false);
		return false;
	}
	else
		return MCStringCreateWithCString("non-system page setup dialogs not supported", r_result);
}

////////////////////////////////////////////////////////////////////////////////

void MCPrinter::SetDeviceName(MCStringRef p_name)
{
	if (!DoReset(p_name))
		MCresult -> sets("unknown printer");
}

const char *MCPrinter::GetDeviceName(void)
{
	return DoFetchName();
}

void MCPrinter::SetDeviceSettings(MCDataRef p_settings)
{
	if (MCDataIsEmpty(p_settings))
		DoReset(kMCEmptyString);
	else if (!DoResetSettings(p_settings))
	{
		MCresult -> sets("unknown printer");
		return;
	}
}

bool MCPrinter::CopyDeviceSettings(MCDataRef &r_settings)
{
	void *t_buffer;
	uint4 t_length;
	DoFetchSettings(t_buffer, t_length);
	return MCDataCreateWithBytesAndRelease((byte_t *)t_buffer, t_length, r_settings);
}

void MCPrinter::SetDeviceOutput(MCPrinterOutputType p_type, MCStringRef p_location)
{
	m_device_output_type = p_type;
	
	delete m_device_output_location;
	m_device_output_location = NULL;
	
	if (p_location != NULL)
    {
        char *t_loc;
        // SN-2014-12-22: [[ Bug 14278 ]] Now store the string as a UTF-8 string.
        /* UNCHECKED */ MCStringConvertToUTF8String(p_location, t_loc);
		m_device_output_location = t_loc;
    }
}

void MCPrinter::SetDeviceCommand(MCStringRef p_command)
{
	delete m_device_command;
	if (MCStringIsEmpty(p_command))
		m_device_command = NULL;
	else
    {
        char *t_command;
        /* UNCHECKED */ MCStringConvertToCString(p_command, t_command);
		m_device_command = t_command;
    }
}

void MCPrinter::SetDeviceFontTable(MCStringRef p_font_table)
{
	delete m_device_font_table;
    char * t_font_table;
    /* UNCHECKED */ MCStringConvertToCString(p_font_table, t_font_table);
	m_device_font_table = t_font_table;
}


MCRectangle MCPrinter::GetDeviceRectangle(void)
{
	if (m_resync)
		DoResync();
	return m_device_rectangle;
}

void MCPrinter::SetDeviceRectangle(const MCRectangle& p_rectangle)
{
	m_device_rectangle = p_rectangle;
}

MCPrinterFeatureSet MCPrinter::GetDeviceFeatures(void)
{
	if (m_resync)
		DoResync();
	return m_device_features;
}

void MCPrinter::SetDeviceFeatures(MCPrinterFeatureSet p_features)
{
	m_device_features = p_features;
}

void MCPrinter::SetPageSize(int32_t p_width, int32_t p_height)
{
	if (m_page_width != p_width || m_page_height != p_height)
		m_resync = true;

	m_page_width = p_width;
	m_page_height = p_height;
}

void MCPrinter::SetPageMargins(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	m_page_left_margin = p_left;
	m_page_top_margin = p_top;
	m_page_right_margin = p_right;
	m_page_bottom_margin = p_bottom;
}

void MCPrinter::SetPageOrientation(MCPrinterOrientation p_orientation)
{
	if (m_page_orientation != p_orientation)
		m_resync = true;

	m_page_orientation = p_orientation;
}

void MCPrinter::SetPageScale(float64_t p_scale)
{
	if (m_page_scale != p_scale)
		m_resync = true;

	m_page_scale = p_scale;
}

void MCPrinter::SetJobCopies(uint32_t p_copies)
{
	if (m_job_copies != p_copies)
		m_resync = true;

	m_job_copies = p_copies;
}

void MCPrinter::SetJobCollate(bool p_collate)
{
	if (m_job_collate != p_collate)
		m_resync = true;

	m_job_collate = p_collate;
}

void MCPrinter::SetJobName(MCStringRef p_name)
{
	delete m_job_name;
    char *t_name;
    /* UNCHECKED */ MCStringConvertToCString(p_name, t_name);
	m_job_name = t_name;
}

void MCPrinter::SetJobDuplex(MCPrinterDuplexMode p_mode)
{
	if (m_job_duplex != p_mode)
		m_resync = true;

	m_job_duplex = p_mode;
}

void MCPrinter::SetJobColor(bool p_color)
{
	m_job_color = p_color;
}

void MCPrinter::SetJobRanges(MCPrinterPageRangeCount p_count, const MCInterval *p_ranges)
{
	// MW-2008-02-28: [[ Bug 5623 ]] Intermittant crash when using printing commands is caused
	//   by this being deleted, but not set to NULL to prevent it being deleted in future.
	delete m_job_ranges;
	m_job_ranges = NULL;

	m_job_range_count = p_count;
	if (p_count > 0)
	{
		m_job_ranges = new (nothrow) MCInterval[p_count];
		memcpy(m_job_ranges, p_ranges, sizeof(MCInterval) * p_count);
	}
}

void MCPrinter::SetLayoutShowBorders(bool p_show_borders)
{
	m_layout_show_borders = p_show_borders;
}

void MCPrinter::SetLayoutRowsFirst(bool p_rows_first)
{
	m_layout_rows_first = p_rows_first;
}

void MCPrinter::SetLayoutSpacing(int32_t p_row, int32_t p_column)
{
	m_layout_row_spacing = p_row;
	m_layout_column_spacing = p_column;
}

void MCPrinter::SetLayoutScale(float64_t p_scale)
{
	if (p_scale < 0.0)
		p_scale = 0.0;
	else
		m_layout_scale = p_scale;
}

//

MCRectangle MCPrinter::GetPageRectangle(void) const
{
	MCRectangle t_page;
	t_page . x = 0;
	t_page . y = 0;

	if (m_page_orientation == PRINTER_ORIENTATION_PORTRAIT || m_page_orientation == PRINTER_ORIENTATION_REVERSE_PORTRAIT)
	{
		t_page . width = (int)ceil(m_page_width / m_page_scale);
		t_page . height = (int)ceil(m_page_height / m_page_scale);
	}
	else
	{
		t_page . width = (int)ceil(m_page_height / m_page_scale);
		t_page . height = (int)ceil(m_page_width / m_page_scale);
	}

	return t_page;
}

////////////////////////////////////////////////////////////////////////////////

void MCPrinter::Open(bool p_cancelled)
{
	if (m_loop_nesting == 0)
	{
		// MW-2007-11-29: [[ Bug 5485 ]] Make sure we can open printing in a cancelled state
		//   (this is used by 'open printing with dialog')
		if (p_cancelled)
			SetStatusFromResult(PRINTER_RESULT_CANCEL);
		else
		{
			if (m_loop_status == STATUS_READY)
			{
				MCAutoStringRef t_job_name;
				if (m_job_name == nil)
					t_job_name = MCdefaultstackptr -> gettitletext();
				else
					/* UNCHECKED */ MCStringCreateWithCString(m_job_name, &t_job_name);

				SetStatusFromResult(DoBeginPrint(*t_job_name, m_device));
			}
	
			ResetLayout();

			m_loop_page = 1;
		}
	}
	
	m_loop_nesting += 1;
	
	SetResult();
}

void MCPrinter::Close(void)
{
	// In the future this should throw an execution error
	if (m_loop_nesting == 0)
		return;

	m_loop_nesting -= 1;
	
	if (m_loop_nesting == 0)
	{
		if (m_loop_status == STATUS_READY)
		{
			MCPrinterResult t_result;
			t_result = DoEndPrint(m_device);
			m_device = NULL;
			SetStatusFromResult(t_result);
		}
	
		// Before we reset status, make sure the result reflects the current status.
		SetResult();
		
		// Reset status
		SetStatus(STATUS_READY);
	}
	else
		SetResult();
}

void MCPrinter::Cancel(void)
{
	// In the future this should throw an execution error
	if (m_loop_nesting == 0)
		return;

	if (m_loop_status == STATUS_READY)
		SetStatusFromResult(m_device -> Cancel());
	
	// If there is an error or we have already cancelled, this is ignored.
	
	SetResult();
}

void MCPrinter::Reset(void)
{
	if (m_loop_nesting > 0)
	{
		// Printing is open, so we cancel and close it first.
		Cancel();
	
		// Regardless of errors, try to close printing.
		while(m_loop_nesting > 0)
			Close();
	}

	Finalize();
	Initialize();

	SetResult();
}

void MCPrinter::Break(void)
{
	Open();
	
	DoPageBreak();
	
	// We've started a new page, thus make sure we rest the layout.
	if (m_loop_nesting > 0 && m_loop_status == STATUS_READY)
		ResetLayout();

	// Close and set the result.
	Close();
}

void MCPrinter::LayoutStack(MCStack *p_stack, bool p_marked, const MCRectangle* p_src_rect)
{
	Open();

	if (m_loop_nesting > 0 && m_loop_status == STATUS_READY)
	{
		MCCard *t_current_card;
		t_current_card = p_stack -> getcurcard();
	
		// Count the number of cards to print, filtering by marked cards if p_marked is true.
		uint2 t_number_of_cards;
		t_number_of_cards = 0;
		if (p_marked)
		{
			p_stack -> setmark();
			p_stack -> count(CT_CARD, CT_CARD, (MCObject *)NULL, t_number_of_cards);
			p_stack -> clearmark();
			if (t_number_of_cards > 0)
				while(!t_current_card -> countme(0, p_marked)) // instead of p_marked, code was : mode == PM_MARKED
					t_current_card = (MCCard *)t_current_card -> next();
		}
		else
			p_stack -> count(CT_CARD, CT_CARD, NULL, t_number_of_cards);
	
		MCRectangle t_rect;
		if (p_src_rect != NULL)
			t_rect = *p_src_rect;
		else
			t_rect = t_current_card -> getrect();
		
		DoLayout(t_current_card, t_number_of_cards, t_rect, p_marked);
	}

	// We rely on Close() to set the result.
	Close();
}

void MCPrinter::LayoutCardSequence(MCStack *p_stack, uint32_t p_number_cards, const MCRectangle *p_src_rect)
{
	Open();
	
	if (m_loop_nesting > 0 && m_loop_status == STATUS_READY)
	{
		MCCard *t_current_card;
		t_current_card = p_stack -> getcurcard();
		
		MCRectangle t_src_rect;
		if (p_src_rect == NULL)
		{
			t_src_rect = t_current_card -> getrect();
			t_src_rect . y += MCdefaultstackptr -> getscroll();
			t_src_rect . height -= MCdefaultstackptr -> getscroll();
		}
		else
			t_src_rect = *p_src_rect;
			
		// Do the layout, passing false to layout all cards, not just marked ones.
		DoLayout(t_current_card, p_number_cards, t_src_rect, false);
	}		
	
	// Close printing (sets the result).
	Close();
}

void MCPrinter::LayoutCard(MCCard *p_card, const MCRectangle *p_rect)
{
	Open();
	
	MCRectangle t_rect;
	if (p_rect == NULL)
	{
		t_rect = p_card -> getrect();
		t_rect . y += MCdefaultstackptr -> getscroll();
		t_rect . height -= MCdefaultstackptr -> getscroll();
	}
	else
		t_rect = *p_rect;

	DoLayout(p_card, 1, t_rect, false);

	// Close, which sets the result.
	Close();
}

void MCPrinter::MakeAnchor(MCStringRef p_name, int16_t x, int16_t y)
{
	Open();

	DoMakeAnchor(p_name, x, y);

	Close();
}

void MCPrinter::MakeLink(MCStringRef p_dest, const MCRectangle& p_area, MCPrinterLinkType p_type)
{
	Open();

	DoMakeLink(p_dest, p_area, p_type);

	Close();
}

void MCPrinter::MakeBookmark(MCStringRef p_title, int2 x, int2 y, uint32_t level, bool closed)
{
	Open();

	DoMakeBookmark(p_title, x, y, level, closed);

	Close();
}

void MCPrinter::Render(MCCard *p_card, const MCRectangle& p_src, const MCRectangle& p_dst)
{
	Open();

	DoPrint(p_card, p_src, p_dst);
	
	Close();
}

void MCPrinter::DoPageBreak(void)
{
	if (m_loop_nesting > 0 && m_loop_status == STATUS_READY)
	{
		if (m_job_range_count <= 0 || MCU_disjointrangecontains(m_job_ranges, m_job_range_count, m_loop_page))
			SetStatusFromResult(m_device -> Show());
		m_loop_page += 1;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCPrinter::DoLayout(MCCard *p_first_card, uint32_t p_number_of_cards, const MCRectangle& p_src_rect, bool p_marked)
{
	if (m_loop_nesting > 0 && m_loop_status == STATUS_READY)
	{
		MCStack *t_card_owner;
		t_card_owner = p_first_card -> getstack();
	
		// MW-2010-10-04: We should back up the *current* card of the stack, to restore after layout.
		MCCard *t_old_card;
		t_old_card = t_card_owner -> getcard(0);
		
		Boolean t_old_lock;
		t_old_lock = MClockmessages;
		
		// Lock messages, as we do not want any opencard messages etc. to be sent when cycling cards.
		MClockmessages = True;
		
		uint32_t t_number_cards_left;
		t_number_cards_left = p_number_of_cards;
		
		MCCard *t_current_card;
		t_current_card = p_first_card;
		
		while(t_number_cards_left > 0 && m_loop_status == STATUS_READY)
		{
			// Set the card of the stack.
			t_card_owner -> setcard(t_current_card, False, False);
			
			/// Calculate the layout for this card, and do a page break if required
			MCRectangle t_dst_rect;
			if (CalculateLayout(p_src_rect, t_dst_rect))
				DoPageBreak();

			// Print the card
			DoPrint(t_current_card, p_src_rect, t_dst_rect);
						
			UpdateLayout(t_dst_rect);
				
			// Advance card
			do
			{
				t_current_card = t_current_card -> next();
			}
			while(!t_current_card -> countme(0, p_marked)); // Skip marked cards if p_marked is true
			
			t_number_cards_left -= 1;
		}

		// Reset old state.
		t_card_owner -> setcard(t_old_card, False, False);
		MClockmessages = t_old_lock;
	}
}

// Interfaces with the printer to do the actual print. This is the only method that does this.
void MCPrinter::DoPrint(MCCard *p_card, const MCRectangle& p_src, const MCRectangle& p_dst)
{
	if (m_loop_nesting > 0 && m_loop_status == STATUS_READY && (m_job_range_count <= 0 || MCU_disjointrangecontains(m_job_ranges, m_job_range_count, m_loop_page)))
	{
		MCContext *t_context;

		// MW-2010-10-04: [[ Bug 9031 ]] Make sure we set the card appropriately if required.
		MCCard *t_old_card;
		t_old_card = p_card -> getstack() -> getcard(0);
		if (t_old_card != p_card)
		{
			Boolean t_old_lock;
			t_old_lock = MClockmessages;
			MClockmessages = True;
			p_card -> getstack() -> setcard(p_card, False, False);
			MClockmessages = t_old_lock;
		}

		// We need to convert rectangle to printer rects here.
		MCPrinterRectangle t_src_printer_rect;
		t_src_printer_rect . left = p_src . x;
		t_src_printer_rect . top = p_src . y;
		t_src_printer_rect . right = p_src . x + p_src . width;
		t_src_printer_rect . bottom = p_src . y + p_src . height;

		MCPrinterRectangle t_dst_printer_rect;
		t_dst_printer_rect . left = p_dst . x;
		t_dst_printer_rect . top = p_dst . y;
		t_dst_printer_rect . right = p_dst . x + p_dst . width;
		t_dst_printer_rect . bottom = p_dst . y + p_dst . height;

		SetStatusFromResult(m_device -> Begin(t_src_printer_rect, t_dst_printer_rect, t_context));
        
        // SN-2014-08-25: [[ Bug 13187 ]] MCplayers' syncbuffering relocated
        MCPlayer::SyncPlayers(p_card->getstack(), t_context);
#ifdef FEATURE_PLATFORM_PLAYER
        MCPlatformWaitForEvent(0.0, true);
#endif
        
		// Draw the card into the context.
		if (m_loop_status == STATUS_READY)
		{
			p_card -> draw(t_context, p_src, false);
			if (m_layout_show_borders)
				p_card -> drawborder(t_context, p_src, 1);
			SetStatusFromResult(m_device -> End(t_context));
		}

		// MW-2010-10-04: [[ Bug 9031 ]] Make sure we reset the card to its previous value if changed.
		if (t_old_card != p_card)
		{
			Boolean t_old_lock;
			t_old_lock = MClockmessages;
			MClockmessages = True;
			p_card -> getstack() -> setcard(t_old_card, False, False);
			MClockmessages = t_old_lock;
		}
	}
}

void MCPrinter::DoMakeAnchor(MCStringRef p_name, int2 x, int2 y)
{
	if (m_loop_nesting > 0 && m_loop_status == STATUS_READY && (m_job_range_count <= 0 || MCU_disjointrangecontains(m_job_ranges, m_job_range_count, m_loop_page)))
    {
        MCAutoPointer<char> t_name;
        /* UNCHECKED */ MCStringConvertToCString(p_name, &t_name);
		SetStatusFromResult(m_device -> Anchor(*t_name, x, y));
    }
}

void MCPrinter::DoMakeLink(MCStringRef p_dest, const MCRectangle& p_area, MCPrinterLinkType p_type)
{
	if (m_loop_nesting > 0 && m_loop_status == STATUS_READY && (m_job_range_count <= 0 || MCU_disjointrangecontains(m_job_ranges, m_job_range_count, m_loop_page)))
	{
		MCPrinterRectangle t_print_area;
		t_print_area . left = p_area . x;
		t_print_area . top = p_area . y;
		t_print_area . right = p_area . x + p_area . width;
		t_print_area . bottom = p_area . y + p_area . height;
        MCAutoPointer<char> t_dest;
        /* UNCHECKED */ MCStringConvertToCString(p_dest, &t_dest);
		SetStatusFromResult(m_device -> Link(*t_dest, t_print_area, p_type));
	}
}

void MCPrinter::DoMakeBookmark(MCStringRef p_title, int2 x, int2 y, uint32_t level, bool closed)
{
	if (m_loop_nesting > 0 && m_loop_status == STATUS_READY && (m_job_range_count <= 0 || MCU_disjointrangecontains(m_job_ranges, m_job_range_count, m_loop_page)))
	{
        MCAutoPointer<char> t_title;
        /* UNCHECKED */ MCStringConvertToCString(p_title, &t_title);
		SetStatusFromResult(m_device -> Bookmark(*t_title, x, y, level, closed));
	}
}

void MCPrinter::ResetLayout(void)
{
	m_loop_layout_x = m_page_left_margin;
	m_loop_layout_y = m_page_top_margin;
	m_loop_layout_delta = 0;
}

/// This is a helper function for the layout methods, and does not check any printing status.
/// Thus, it is assumed that this is done at a higher level.
/// The function returns true if a page break is required.
bool MCPrinter::CalculateLayout(const MCRectangle& p_src_rect, MCRectangle& r_dst_rect)
{
	int32_t t_dst_width;
	t_dst_width = (int32_t)ceil(m_layout_scale * p_src_rect . width);
	
	int32_t t_dst_height;
	t_dst_height = (int32_t)ceil(m_layout_scale * p_src_rect . height);
	
	int32_t t_maximum_x;
	t_maximum_x = m_page_width - m_page_right_margin;
	
	int32_t t_maximum_y;
	t_maximum_y = m_page_height - m_page_bottom_margin;
	
	bool t_require_break;
	t_require_break = false;
	
	if (m_loop_layout_x != m_page_left_margin || m_loop_layout_y != m_page_top_margin)
	{
		if (m_layout_rows_first)
		{
			if (m_loop_layout_x + t_dst_width > t_maximum_x)
			{
				m_loop_layout_x = m_page_left_margin;
				m_loop_layout_y += m_layout_row_spacing + m_loop_layout_delta;
				m_loop_layout_delta = t_dst_height;
				if (m_loop_layout_y + t_dst_height > t_maximum_y)
					t_require_break = true;
			}
		}
		else
		{
			if (m_loop_layout_y + t_dst_height > t_maximum_y)
			{
				m_loop_layout_y = m_page_top_margin;
				m_loop_layout_x += m_layout_column_spacing + m_loop_layout_delta;
				m_loop_layout_delta = t_dst_width;
				if (m_loop_layout_x + t_dst_width > t_maximum_x)
					t_require_break = true;
			}
		}
	}
	
	if (t_require_break)
	{
		m_loop_layout_x = m_page_left_margin;
		m_loop_layout_y = m_page_top_margin;
	}
	
	r_dst_rect . x = m_loop_layout_x;
	r_dst_rect . y = m_loop_layout_y;
	r_dst_rect . width = t_dst_width;
	r_dst_rect . height = t_dst_height;
	
	if (m_layout_rows_first)
	{
		m_loop_layout_delta = MCU_max(m_loop_layout_delta, t_dst_height);
		m_loop_layout_x += m_layout_column_spacing + t_dst_width;
	}
	else
	{
		m_loop_layout_delta = MCU_max(m_loop_layout_delta, t_dst_width);
		m_loop_layout_y += m_layout_row_spacing + t_dst_height;
	}
			
	return t_require_break;
}

// Updates the layout after a rectangle has been printed.
void MCPrinter::UpdateLayout(const MCRectangle& p_rect)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCPrinter::SetStatus(uint32_t p_status, MCStringRef p_error)
{
	m_loop_status = p_status;
	
	if (m_loop_error != NULL)
	{
		delete m_loop_error;
		m_loop_error = NULL;
	}
	
	if (p_error != nil)
    {
        char *t_error;
        /* UNCHECKED */ MCStringConvertToCString(p_error, t_error);
		m_loop_error = t_error;
    }
}

void MCPrinter::SetStatusFromResult(MCPrinterResult p_result)
{
	switch(p_result)
	{
		case PRINTER_RESULT_SUCCESS:
		break;
	
		case PRINTER_RESULT_FAILURE:
			assert(false);
		break;
		
		case PRINTER_RESULT_ERROR:
			SetStatus(STATUS_ERROR, MCSTR("printing failed"));
		break;
		
		case PRINTER_RESULT_CANCEL:
			SetStatus(STATUS_CANCELLED);
		break;
		
		default:
			assert(false);
		break;
	}

	if (p_result != PRINTER_RESULT_SUCCESS && m_device != NULL)
	{
		DoEndPrint(m_device);
		m_device = NULL;
	}
}

void MCPrinter::SetResult(void)
{
	switch (m_loop_status)
	{
	case STATUS_READY:
		MCresult -> clear();
	break;
	
	case STATUS_CANCELLED:
		MCresult -> sets(MCcancelstring);
	break;
	
	case STATUS_ERROR:
		MCresult -> copysvalue(m_loop_error);
	break;
	
	default:
		assert(false);
	break;
	}
}

////////////////////////////////////////////////////////////////////////////////
