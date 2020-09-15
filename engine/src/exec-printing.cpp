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

#include "globals.h"
#include "scriptpt.h"
#include "mode.h"
#include "handler.h"
#include "osspec.h"
#include "uidc.h"
#include "license.h"
#include "debug.h"
#include "field.h"
#include "stack.h"
#include "card.h"
#include "printer.h"
#include "securemode.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

static MCExecSetTypeElementInfo _kMCPrintingPrinterFeaturesElementInfo[] =
{
	{ "collate", PRINTER_FEATURE_COLLATE_BIT },
	{ "copies", PRINTER_FEATURE_COPIES_BIT },
	{ "color", PRINTER_FEATURE_COLOR_BIT },
	{ "duplex", PRINTER_FEATURE_DUPLEX_BIT },
};

static MCExecSetTypeInfo _kMCPrintingPrinterFeaturesTypeInfo =
{
	"Printing.PrinterFeatures",
	sizeof(_kMCPrintingPrinterFeaturesElementInfo) / sizeof(MCExecSetTypeElementInfo),
	_kMCPrintingPrinterFeaturesElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCPrintingPrinterOrientationElementInfo[] =
{
	{ "portrait", PRINTER_ORIENTATION_PORTRAIT, false },
	{ "reverse portrait", PRINTER_ORIENTATION_REVERSE_PORTRAIT, false },
	{ "landscape", PRINTER_ORIENTATION_LANDSCAPE, false },
	{ "reverse landscape", PRINTER_ORIENTATION_REVERSE_LANDSCAPE, false },
};

static MCExecEnumTypeInfo _kMCPrintingPrinterOrientationTypeInfo =
{
	"Printing.PrinterOrientation",
	sizeof(_kMCPrintingPrinterOrientationElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCPrintingPrinterOrientationElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCPrintingDuplexJobElementInfo[] =
{
	{ "none", PRINTER_DUPLEX_MODE_SIMPLEX, false },
	{ "short edge", PRINTER_DUPLEX_MODE_SHORT_EDGE, false },
	{ "long edge", PRINTER_DUPLEX_MODE_LONG_EDGE, false },
};

static MCExecEnumTypeInfo _kMCPrintingPrintJobDuplexTypeInfo =
{
	"Printing.PrintJobDuplex",
	sizeof(_kMCPrintingDuplexJobElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCPrintingDuplexJobElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCPrintingPrinterLinkTypeElementInfo[] =
{
	{ "", kMCPrinterLinkUnspecified, false },
	{ "anchor", kMCPrinterLinkAnchor, false },
	{ "url", kMCPrinterLinkURI, false },
};

static MCExecEnumTypeInfo _kMCPrintingPrinterLinkTypeInfo =
{
	"Printing.PrinterLinkType",
	sizeof(_kMCPrintingPrinterLinkTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCPrintingPrinterLinkTypeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCPrintingPrinterBookmarkInitialStateTypeElementInfo[] =
{
	{ "open", 0, false },
	{ "closed", 1, false },
};

static MCExecEnumTypeInfo _kMCPrintingPrinterBookmarkInitialStateTypeInfo =
{
	"Printing.PrinterBookmarkInitialState",
	sizeof(_kMCPrintingPrinterBookmarkInitialStateTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCPrintingPrinterBookmarkInitialStateTypeElementInfo
};

//////////

struct MCPrintingPrintDeviceOutput
{
	MCPrinterOutputType type;
	MCStringRef location;
};

static void MCPrintingPrintDeviceOutputParse(MCExecContext& ctxt, MCStringRef p_input, MCPrintingPrintDeviceOutput *r_output)
{
	if (MCStringIsEqualToCString(p_input, "preview", kMCCompareCaseless))
	{
		r_output -> type = PRINTER_OUTPUT_PREVIEW;
		r_output -> location = nil;
		return;
	}
	
	if (MCStringIsEqualToCString(p_input, "device", kMCCompareCaseless))
	{
		r_output -> type = PRINTER_OUTPUT_DEVICE;
		r_output -> location = nil;
		return;
	}
	
	MCAutoStringRef t_head, t_tail;
	if (MCStringDivideAtChar(p_input, ':', kMCCompareExact, &t_head, &t_tail))
	{
		if (MCStringIsEqualToCString(*t_head, "file", kMCCompareCaseless))
		{
			r_output -> type = PRINTER_OUTPUT_FILE;
			r_output -> location = MCValueRetain(*t_tail);
			return;
		}
		
		ctxt . LegacyThrow(EE_PROPERTY_BADPRINTPROP, p_input);
		return;
	}
	
	ctxt . Throw();
}

static void MCPrintingPrintDeviceOutputFormat(MCExecContext& ctxt, const MCPrintingPrintDeviceOutput *p_input, MCStringRef& r_output)
{
	switch(p_input -> type)
	{
		case PRINTER_OUTPUT_DEVICE:
			if (MCStringCreateWithCString("device", r_output))
				return;
			break;
		case PRINTER_OUTPUT_PREVIEW:
			if (MCStringCreateWithCString("preview", r_output))
				return;
			break;
		case PRINTER_OUTPUT_SYSTEM:
			if (MCStringCreateWithCString("system", r_output))
				return;
			break;
		case PRINTER_OUTPUT_FILE:
			if (MCStringFormat(r_output, "file:%@", p_input -> location))
				return;
			break;
		default:
			MCAssert(false);
			break;
	}
	
	ctxt . Throw();
}

static void MCPrintingPrintDeviceOutputFree(MCExecContext& ctxt, MCPrintingPrintDeviceOutput *p_input)
{
	MCValueRelease(p_input -> location);
}

static MCExecCustomTypeInfo _kMCPrintingPrintDeviceOutputTypeInfo =
{
	"Printer.PrintDeviceOutput",
	sizeof(MCPrintingPrintDeviceOutput),
	(void *)MCPrintingPrintDeviceOutputParse,
	(void *)MCPrintingPrintDeviceOutputFormat,
	(void *)MCPrintingPrintDeviceOutputFree,
};

//////////

struct MCPrintingPrinterPageRange
{
	MCPrinterPageRangeCount count;
	MCInterval *ranges;
};

static void MCPrintingPrinterPageRangeParse(MCExecContext& ctxt, MCStringRef p_input, MCPrintingPrinterPageRange& r_output)
{
	if (MCStringIsEqualToCString(p_input, "all", kMCCompareCaseless) ||
		MCStringIsEmpty(p_input))
	{
		r_output . count = PRINTER_PAGE_RANGE_ALL;
		r_output . ranges = nil;
		return;
	}
	
	if (MCStringIsEqualToCString(p_input, "current", kMCCompareCaseless))
	{
		r_output . count = PRINTER_PAGE_RANGE_CURRENT;
		r_output . ranges = nil;
		return;
	}
	
	if (MCStringIsEqualToCString(p_input, "selection", kMCCompareCaseless))
	{
		r_output . count = PRINTER_PAGE_RANGE_SELECTION;
		r_output . ranges = nil;
		return;
	}
	
	bool t_error;
	t_error = false;
	
	MCInterval *t_ranges;
	int t_range_count;
	t_ranges = NULL;
	///////////////////////////////////////////
	uindex_t t_pos = 0;
	uindex_t t_comma, t_dash;
	t_range_count = 0;
	while (t_pos != MCStringGetLength(p_input) && !t_error)
	{
		int t_from, t_to;
		bool t_found_comma;
		t_found_comma = MCStringFirstIndexOfChar(p_input, ',', t_pos, kMCCompareExact, t_comma);
		if (t_found_comma)
		{
			if (MCStringSubstringContains(p_input, MCRangeMakeMinMax(t_pos, t_comma), MCSTR("-"), kMCCompareExact))
			{
				/* UNCHECKED */ MCStringFirstIndexOfChar(p_input, '-', t_pos, kMCCompareExact, t_dash);

				MCAutoStringRef t_substring_from;
				/* UNCHECKED */ MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_pos, t_dash), &t_substring_from);
				t_error = !ctxt . ConvertToInteger(*t_substring_from, t_from);
				t_pos = t_dash + 1;

				MCAutoStringRef t_substring_to;
				/* UNCHECKED */ MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_pos, t_comma), &t_substring_to);
				t_error = !ctxt . ConvertToInteger(*t_substring_to, t_to);
				t_pos = t_comma;
			}
			//case of no dash found before comma
			else
			{
				MCAutoStringRef t_substring;
				/* UNCHECKED */ MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_pos, t_comma), &t_substring);
				t_error = !ctxt . ConvertToInteger(*t_substring, t_from);
				t_to = t_from;
				t_pos = t_comma;
			}
		}
		//case no comma exists after t_pos
		else
		{
			//case dash found after t_pos
			if (MCStringSubstringContains(p_input, MCRangeMakeMinMax(t_pos, MCStringGetLength(p_input)), MCSTR("-"), kMCCompareExact))
			{
				/* UNCHECKED */ MCStringFirstIndexOfChar(p_input, '-', t_pos, kMCCompareExact, t_dash);

				MCAutoStringRef t_substring_from;
				/* UNCHECKED */ MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_pos, t_dash), &t_substring_from);
				t_error =  !ctxt . ConvertToInteger(*t_substring_from, t_from);
				t_pos = t_dash + 1;

				MCAutoStringRef t_substring_to;
				/* UNCHECKED */ MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_pos, MCStringGetLength(p_input)), &t_substring_to);
				t_error = !ctxt . ConvertToInteger(*t_substring_to, t_to);
				t_pos = MCStringGetLength(p_input);
			}
			//case no dash after t_pos
			else
			{
				MCAutoStringRef t_substring;
				/* UNCHECKED */ MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_pos, MCStringGetLength(p_input)), &t_substring);
				t_error = !ctxt . ConvertToInteger(*t_substring, t_from);
				t_to = t_from;
				t_pos = t_comma;
			}
		}

		if (!t_error)
		{
			MCU_disjointrangeinclude(t_ranges, t_range_count, t_from, t_to);
			
			//t_pos index should contain a comma, or t_pos equals the length of the p_input string 
			if (MCStringGetNativeCharAtIndex(p_input, t_pos) == ',')
				t_pos = t_comma + 1;
			else if (t_pos != MCStringGetLength(p_input))
				t_error = true;
		}
	}

	if (!t_error)
	{
		r_output . count = t_range_count;
		r_output . ranges = t_ranges;
		return;
	}
	
    if (t_ranges != nil)
        MCMemoryDeallocate(t_ranges);
    
	ctxt . LegacyThrow(EE_PROPERTY_BADPRINTPROP);
}

static void MCPrintingPrinterPageRangeFormat(MCExecContext& ctxt, const MCPrintingPrinterPageRange& p_input, MCStringRef& r_output)
{
	switch(p_input . count)
	{
		case PRINTER_PAGE_RANGE_CURRENT:
			if (MCStringCreateWithCString("current", r_output))
				return;
			break;
		case PRINTER_PAGE_RANGE_SELECTION:
			if (MCStringCreateWithCString("selection", r_output))
				return;
			break;
		case PRINTER_PAGE_RANGE_ALL:
			if (MCStringCreateWithCString("all", r_output))
				return;
			break;
		default:
		{
			const MCInterval *t_ranges;
			t_ranges = MCprinter -> GetJobRanges();

            MCAutoListRef t_list;
            if (!MCListCreateMutable(',', &t_list))
                break;
            for (index_t i = 0; i < p_input . count; ++i)
            {
                if (t_ranges[i].from == t_ranges[i].to)
                {
                    if (!MCListAppendFormat(*t_list, "%d", t_ranges[i].from))
                        break;
                }
                else
                {
                    if (!MCListAppendFormat(*t_list, "%d-%d",
                                            t_ranges[i].from, t_ranges[i].to))
                        break;
                }
            }
            if (MCListCopyAsString(*t_list, r_output))
                return;
		}
		break;
	}
	
	ctxt . Throw();
}

static void MCPrintingPrinterPageRangeFree(MCExecContext& ctxt, MCPrintingPrinterPageRange& p_input)
{
	MCMemoryDeleteArray(p_input . ranges);
}

static MCExecCustomTypeInfo _kMCPrintingPrinterPageRangeTypeInfo =
{
	"Printing.PrinterPageRange",
	sizeof(MCPrintingPrinterPageRange),
	(void *)MCPrintingPrinterPageRangeParse,
	(void *)MCPrintingPrinterPageRangeFormat,
	(void *)MCPrintingPrinterPageRangeFree,
};

//////////

MCExecSetTypeInfo *kMCPrintingPrinterFeaturesTypeInfo = &_kMCPrintingPrinterFeaturesTypeInfo;
MCExecEnumTypeInfo *kMCPrintingPrinterOrientationTypeInfo = &_kMCPrintingPrinterOrientationTypeInfo;
MCExecEnumTypeInfo *kMCPrintingPrintJobDuplexTypeInfo = &_kMCPrintingPrintJobDuplexTypeInfo;
MCExecEnumTypeInfo *kMCPrintingPrinterLinkTypeInfo = &_kMCPrintingPrinterLinkTypeInfo;
MCExecEnumTypeInfo *kMCPrintingPrinterBookmarkInitialStateTypeInfo = &_kMCPrintingPrinterBookmarkInitialStateTypeInfo;
MCExecCustomTypeInfo *kMCPrintingPrintDeviceOutputTypeInfo = &_kMCPrintingPrintDeviceOutputTypeInfo;
MCExecCustomTypeInfo *kMCPrintingPrinterPageRangeTypeInfo = &_kMCPrintingPrinterPageRangeTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCPrintingExecCancelPrinting(MCExecContext& ctxt) 
{
	MCprinter -> Cancel();
	if (MCprinter != MCsystemprinter)
	{
		delete MCprinter;
		MCprinter = MCsystemprinter;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingExecResetPrinting(MCExecContext& ctxt)
{
	MCprinter -> Reset();
	if (MCprinter != MCsystemprinter)
	{
		delete MCprinter;
		MCprinter = MCsystemprinter;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingExecPrintAnchor(MCExecContext& ctxt, MCStringRef p_name, MCPoint p_location)
{
	if (!ctxt . EnsurePrintingIsAllowed())
		return;

	MCprinter -> MakeAnchor(p_name, p_location . x, p_location . y);
}

void MCPrintingExecPrintLink(MCExecContext& ctxt, int p_type, MCStringRef p_target, MCRectangle p_area)
{
	if (!ctxt . EnsurePrintingIsAllowed())
		return;
		
	MCprinter -> MakeLink(p_target, p_area, (MCPrinterLinkType)p_type);
}

void MCPrintingExecPrintBookmark(MCExecContext& ctxt, MCStringRef p_title, MCPoint p_location, integer_t p_level, bool p_initially_closed)
{
	if (!ctxt . EnsurePrintingIsAllowed())
		return;
	
	MCprinter -> MakeBookmark(p_title, p_location . x, p_location . y, p_level, p_initially_closed);
}

void MCPrintingExecPrintUnicodeBookmark(MCExecContext& ctxt, MCDataRef p_title, MCPoint p_location, integer_t p_level, bool p_initially_closed)
{
	MCAutoStringRef t_title_string;
	if (MCStringDecode(p_title, kMCStringEncodingUTF16, false, &t_title_string))
	{
		MCPrintingExecPrintBookmark(ctxt, *t_title_string, p_location, p_level, p_initially_closed);
		return;
	}
	
	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

static bool MCPrintingBeginLayout(MCExecContext& ctxt, MCStack *p_stack)
{
	if (!ctxt . EnsurePrintingIsAllowed())
		return false;
	
	if (p_stack -> getopened() == 0)
	{
		ctxt . LegacyThrow(EE_PRINT_NOTOPEN);
		return false;
	}

	ctxt . SetTheResultToEmpty();

	MCU_watchcursor(ctxt . GetObject() -> getstack(), True);

	return true;
}

static void MCPrintingEndLayout(MCExecContext& ctxt)
{
	// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
	//   return back to the caller.
	MCU_unwatchcursor(ctxt . GetObject() -> getstack(), True);
}

//////////

void MCPrintingExecPrintBreak(MCExecContext& ctxt)
{
	if (!MCPrintingBeginLayout(ctxt, MCdefaultstackptr))
		return;
		
	MCprinter -> Break();

	MCPrintingEndLayout(ctxt);
}

void MCPrintingExecPrintAllCards(MCExecContext& ctxt, MCStack *p_stack, bool p_only_marked)
{
	if (p_stack == nil)
		p_stack = MCdefaultstackptr;

	if (!MCPrintingBeginLayout(ctxt, p_stack))
		return;
		
	MCprinter -> LayoutStack(p_stack, p_only_marked, nil);

	MCPrintingEndLayout(ctxt);
}

void MCPrintingExecPrintRectOfAllCards(MCExecContext& ctxt, MCStack *p_stack, bool p_only_marked, MCPoint p_from, MCPoint p_to)
{
	if (p_stack == nil)
		p_stack = MCdefaultstackptr;
		
	if (!MCPrintingBeginLayout(ctxt, p_stack))
		return;
	
	MCRectangle t_src_rect;
	t_src_rect . x = p_from . x;
	t_src_rect . y = p_from . y;
	t_src_rect . width = p_to . x - p_from . x;
	t_src_rect . height = p_to . y - p_from . y;
	
	MCprinter -> LayoutStack(p_stack, p_only_marked, &t_src_rect);

	MCPrintingEndLayout(ctxt);
}

void MCPrintingExecPrintCard(MCExecContext& ctxt, MCCard *p_card)
{
	if (p_card == nil)
		p_card = MCdefaultstackptr -> getcurcard();

	if (!MCPrintingBeginLayout(ctxt, p_card -> getstack()))
		return;
		
	MCprinter -> LayoutCard(p_card, nil);

	MCPrintingEndLayout(ctxt);
}

void MCPrintingExecPrintRectOfCard(MCExecContext& ctxt, MCCard *p_card, MCPoint p_from, MCPoint p_to)
{
	if (p_card == nil)
		p_card = MCdefaultstackptr -> getcurcard();

	if (!MCPrintingBeginLayout(ctxt, p_card -> getstack()))
		return;
	
	MCRectangle t_src_rect;
	t_src_rect . x = p_from . x;
	t_src_rect . y = p_from . y;
    // SN-2014-11-03: [[ Bug 13913 ]] Use the correct coordinates
    // SN-2014-11-13: [[ Bug 13913 ]] Really, the right coordinates should be used...
    t_src_rect . width = p_to . x - p_from . x;
    t_src_rect . height = p_to . y - p_from . y;
	
	MCprinter -> LayoutCard(p_card, &t_src_rect);

	MCPrintingEndLayout(ctxt);
}

void MCPrintingExecPrintSomeCards(MCExecContext& ctxt, integer_t p_count)
{
	if (!MCPrintingBeginLayout(ctxt, MCdefaultstackptr))
		return;

	MCprinter -> LayoutCardSequence(MCdefaultstackptr, p_count, nil);

	MCPrintingEndLayout(ctxt);
}

void MCPrintingExecPrintRectOfSomeCards(MCExecContext& ctxt, integer_t p_count, MCPoint p_from, MCPoint p_to)
{
	if (!MCPrintingBeginLayout(ctxt, MCdefaultstackptr))
		return;
	
	MCRectangle t_src_rect;
	t_src_rect . x = p_from . x;
	t_src_rect . y = p_from . y;
    // SN-2015-03-10: [[ Bug 14814 ]] Same fix as for
    //  MCPrintingExecPrintRectOfCardIntoRect
	t_src_rect . width = p_to . x - p_from . x;
	t_src_rect . height = p_to . y - p_from . y;
	
	MCprinter -> LayoutCardSequence(MCdefaultstackptr, p_count, &t_src_rect);

	MCPrintingEndLayout(ctxt);
}

void MCPrintingExecPrintRectOfCardIntoRect(MCExecContext& ctxt, MCCard *p_card, MCPoint p_src_from, MCPoint p_src_to, MCRectangle p_dst_rect)
{
	if (p_card == nil)
		p_card = MCdefaultstackptr -> getcurcard();
		
	if (!MCPrintingBeginLayout(ctxt, p_card -> getstack()))
		return;

	MCRectangle t_src_rect;
	t_src_rect . x = p_src_from . x;
	t_src_rect . y = p_src_from . y;
    // SN-2015-03-10: [[ Bug 14814 ]] Use p_src_to - p_src_from coordinates
    //  to compute the width and height (not p_src_to - p_src_to).
	t_src_rect . width = p_src_to . x - p_src_from . x;
	t_src_rect . height = p_src_to. y - p_src_from . y;
	
	MCprinter -> Render(p_card, t_src_rect, p_dst_rect);

	MCPrintingEndLayout(ctxt);
}

void MCPrintingExecPrintCardIntoRect(MCExecContext& ctxt, MCCard *p_card, MCRectangle p_dst_rect)
{
	if (p_card == nil)
		p_card = MCdefaultstackptr -> getcurcard();
		
	if (!MCPrintingBeginLayout(ctxt, p_card -> getstack()))
		return;

	MCprinter -> Render(p_card, p_card -> getrect(), p_dst_rect);

	MCPrintingEndLayout(ctxt);
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingExecOpenPrintingToDestination(MCExecContext& ctxt, MCStringRef p_destination, MCStringRef p_filename, MCArrayRef p_options)
{
	if (MCCustomPrinterCreate(p_destination, p_filename, p_options, (MCCustomPrinter*&)MCprinter))
    {
		MCPrintingExecOpenPrinting(ctxt);
        return;
    }
    
    ctxt . LegacyThrow(EE_PRINT_UNKNOWNDST);
}

void MCPrintingExecOpenPrinting(MCExecContext& ctxt)
{
	if (ctxt . EnsurePrintingIsAllowed())
		MCprinter ->Open(false);
}

void MCPrintingExecOpenPrintingWithDialog(MCExecContext& ctxt, bool p_as_sheet)
{
	MCAutoStringRef t_result;
	if (MCprinter->ChoosePrinter(p_as_sheet, &t_result))
	{
        // PM-2014-11-04: [[ Bug 13915 ]] Make sure we do not print if user cancels        
        if (MCStringGetLength(*t_result) > 0)
            ctxt.SetTheResultToValue(MCN_cancel);
        else
        {
            ctxt.SetTheResultToEmpty();
            MCPrintingExecOpenPrinting(ctxt);
        }

		return;
	}

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingExecClosePrinting(MCExecContext& ctxt)
{
	MCprinter -> Close();
	if (MCsystemprinter != MCprinter)	
	{
		delete MCprinter;
		MCprinter = MCsystemprinter;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingExecAnswerPageSetup(MCExecContext &ctxt, bool p_is_sheet)
{
	if (!MCSecureModeCanAccessPrinter())
	{
		ctxt.LegacyThrow(EE_PRINT_NOPERM);
		return;
	}

	MCAutoStringRef t_result;
	if (MCsystemPS && MCscreen->hasfeature(PLATFORM_FEATURE_OS_PRINT_DIALOGS))
	{
		if (!MCprinter->ChoosePaper(p_is_sheet, &t_result))
		{
			ctxt.Throw();
			return;
		}
	}
	else
	{
		MCDialogExecCustomAnswerDialog(ctxt, MCN_page_setup_dialog, MCN_pagesetup, p_is_sheet, nil, 0, &t_result);
		if (ctxt.HasError())
			return;
	}

	ctxt.SetItToValue(*t_result);

	if (MCStringGetLength(*t_result) > 0)
		ctxt.SetTheResultToValue(MCN_cancel);
	else
		ctxt.SetTheResultToEmpty();
}

//////////

void MCPrintingExecAnswerPrinter(MCExecContext &ctxt, bool p_is_sheet)
{
	if (!MCSecureModeCanAccessPrinter())
	{
		ctxt.LegacyThrow(EE_PRINT_NOPERM);
		return;
	}

	MCAutoStringRef t_result;
	if (MCsystemPS && MCscreen->hasfeature(PLATFORM_FEATURE_OS_PRINT_DIALOGS))
	{
		if (!MCprinter->ChoosePrinter(p_is_sheet, &t_result))
		{
			ctxt.Throw();
			return;
		}
	}
	else
	{
		MCDialogExecCustomAnswerDialog(ctxt, MCN_print_dialog, MCN_printer,
			p_is_sheet, nil, 0, &t_result);
		if (ctxt.HasError())
			return;
	}

	ctxt.SetItToValue(*t_result);

	if (MCStringGetLength(*t_result) > 0)
		ctxt.SetTheResultToValue(MCN_cancel);
	else
		ctxt.SetTheResultToEmpty();
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingGetPrintDeviceFeatures(MCExecContext& ctxt, unsigned int& r_features)
{
	r_features = MCprinter -> GetDeviceFeatures();
}

void MCPrintingSetPrintDeviceOutput(MCExecContext& ctxt, const MCPrintingPrintDeviceOutput& p_output)
{
	MCprinter -> SetDeviceOutput(p_output . type, p_output . location);
}

void MCPrintingGetPrintDeviceOutput(MCExecContext& ctxt, MCPrintingPrintDeviceOutput& r_output)
{
	MCPrinterOutputType t_output_type;
	t_output_type = MCprinter -> GetDeviceOutputType();
	
	if (t_output_type == PRINTER_OUTPUT_FILE)
	{
        // SN-2014-12-22: [[ Bug 14278 ]] Output location now stored as a UTF-8 string.
		if (!MCStringCreateWithBytes((byte_t*)MCprinter -> GetDeviceOutputLocation(), strlen(MCprinter->GetDeviceOutputLocation()), kMCStringEncodingUTF8, false, r_output . location))
		{
			ctxt . Throw();
			return;
		}
	}
	else
		r_output . location = nil;
	
	r_output . type = t_output_type;
}

void MCPrintingSetPrintPageOrientation(MCExecContext& ctxt, int p_orientation)
{
	MCprinter -> SetPageOrientation((MCPrinterOrientation)p_orientation);
}

void MCPrintingGetPrintPageOrientation(MCExecContext& ctxt, int& r_orientation)
{
	r_orientation = (int)MCprinter -> GetPageOrientation();
}

void MCPrintingSetPrintJobRanges(MCExecContext& ctxt, const MCPrintingPrinterPageRange& p_ranges)
{
	MCprinter -> SetJobRanges(p_ranges . count, p_ranges . ranges);
}

void MCPrintingGetPrintJobRanges(MCExecContext& ctxt, MCPrintingPrinterPageRange& r_ranges)
{
	MCPrinterPageRangeCount t_count;
	t_count = MCprinter -> GetJobRangeCount();
	
	if (t_count > 0)
	{
		if (MCMemoryNewArray(t_count, r_ranges . ranges))
		{
			MCMemoryCopy(r_ranges . ranges, MCprinter -> GetJobRanges(), sizeof(MCInterval) * t_count);
			r_ranges . count = t_count;
			return;
		}
	}
	else
	{
		r_ranges . count = t_count;
		r_ranges . ranges = nil;
		return;
	}
	
	ctxt . Throw();

}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingSetPrintPageSize(MCExecContext& ctxt, integer_t p_value[2])
{
	MCprinter -> SetPageSize(p_value[0], p_value[1]);
}

void MCPrintingGetPrintPageSize(MCExecContext& ctxt, integer_t r_value[2])
{
	r_value[0] = MCprinter->GetPageWidth();
	r_value[1] = MCprinter->GetPageHeight();
}

void MCPrintingSetPrintPageScale(MCExecContext& ctxt, double p_value)
{
	MCprinter -> SetPageScale(p_value);
}

void MCPrintingGetPrintPageScale(MCExecContext& ctxt, double &r_value)
{
	r_value = MCprinter -> GetPageScale();
}

void MCPrintingGetPrintPageRectangle(MCExecContext& ctxt, MCRectangle &r_value)
{
	r_value = MCprinter -> GetPageRectangle();
}

void MCPrintingGetPrintJobName(MCExecContext& ctxt, MCStringRef &r_value)
{
	if (!MCStringCreateWithCString(MCprinter -> GetJobName(), r_value))
		ctxt . Throw();
}

void MCPrintingSetPrintJobName(MCExecContext& ctxt, MCStringRef p_value)
{
	MCprinter -> SetJobName(p_value);
}

void MCPrintingGetPrintJobCopies(MCExecContext& ctxt, integer_t &r_value)
{
	r_value = MCprinter -> GetJobCopies();
}

void MCPrintingSetPrintJobCopies(MCExecContext& ctxt, integer_t p_value)
{
	MCprinter -> SetJobCopies(p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingGetPrintJobCollate(MCExecContext& ctxt, bool &r_value)
{
	r_value = MCprinter -> GetJobCollate();
}

void MCPrintingSetPrintJobCollate(MCExecContext& ctxt, bool p_value)
{
	MCprinter -> SetJobCollate(p_value);
}

void MCPrintingGetPrintJobColor(MCExecContext& ctxt, bool &r_value)
{
	r_value = MCprinter -> GetJobColor();
}

void MCPrintingSetPrintJobColor(MCExecContext& ctxt, bool p_value)
{
	MCprinter -> SetJobColor(p_value);
}

// SN-2014-09-17: [[ Bug 13467 ]] When -1 is returned by the printer, we should get empty instead
void MCPrintingGetPrintJobPage(MCExecContext& ctxt, integer_t *&r_value)
{
    integer_t t_value;
	t_value = MCprinter -> GetJobPageNumber();
    if (t_value == -1)
        r_value = nil;
    else
        *r_value = t_value;
}

void MCPrintingGetPrintJobDuplex(MCExecContext& ctxt, intenum_t &r_value)
{
	r_value = (integer_t)MCprinter -> GetJobDuplex();
}

void MCPrintingSetPrintJobDuplex(MCExecContext& ctxt, intenum_t p_value)
{
	MCprinter -> SetJobDuplex((MCPrinterDuplexMode)p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingGetPrintDeviceRectangle(MCExecContext& ctxt, MCRectangle &r_rectangle)
{
	r_rectangle = MCprinter -> GetDeviceRectangle();
}

void MCPrintingGetPrintDeviceSettings(MCExecContext& ctxt, MCDataRef &r_settings)
{
	if (MCprinter -> CopyDeviceSettings(r_settings))
		return;

	ctxt . Throw();
}

void MCPrintingSetPrintDeviceSettings(MCExecContext& ctxt, MCDataRef p_settings)
{
	MCprinter -> SetDeviceSettings(p_settings);
}

void MCPrintingGetPrintDeviceName(MCExecContext& ctxt, MCStringRef &r_name)
{
	if (MCStringCreateWithCString(MCprinter -> GetDeviceName(), r_name))
		return;

	ctxt . Throw();
}

void MCPrintingSetPrintDeviceName(MCExecContext& ctxt, MCStringRef p_name)
{
	MCprinter -> SetDeviceName(p_name);
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingGetPrintCardBorders(MCExecContext& ctxt, bool &r_card_borders)
{
	r_card_borders = MCprinter -> GetLayoutShowBorders();
}

void MCPrintingSetPrintCardBorders(MCExecContext& ctxt, bool p_card_borders)
{
	MCprinter -> SetLayoutShowBorders(p_card_borders);
}

void MCPrintingGetPrintGutters(MCExecContext& ctxt, integer_t r_gutters[2])
{
	r_gutters[0] = MCprinter -> GetLayoutRowSpacing();
	r_gutters[1] = MCprinter -> GetLayoutColumnSpacing();
}

void MCPrintingSetPrintGutters(MCExecContext& ctxt, integer_t p_gutters[2])
{
	MCprinter -> SetLayoutSpacing(p_gutters[0], p_gutters[1]);
}

void MCPrintingGetPrintMargins(MCExecContext& ctxt, integer_t r_margins[4])
{
	r_margins[0] = MCprinter -> GetPageLeftMargin();
	r_margins[1] = MCprinter -> GetPageTopMargin();
	r_margins[2] = MCprinter -> GetPageRightMargin();
	r_margins[3] = MCprinter -> GetPageBottomMargin();
}

void MCPrintingSetPrintMargins(MCExecContext& ctxt, integer_t p_margins[4])
{
	MCprinter -> SetPageMargins(p_margins[0], p_margins[1], p_margins[2], p_margins[3]);
}

void MCPrintingGetPrintRowsFirst(MCExecContext& ctxt, bool &r_rows_first)
{
	r_rows_first = MCprinter -> GetLayoutRowsFirst();
}

void MCPrintingSetPrintRowsFirst(MCExecContext& ctxt, bool p_rows_first)
{
	MCprinter -> SetLayoutRowsFirst(p_rows_first);
}

void MCPrintingGetPrintScale(MCExecContext& ctxt, double &r_scale)
{
	// MW-2007-09-10: [[ Bug 5343 ]] Without this we get a crash :oD
	r_scale = MCprinter -> GetLayoutScale();
}

void MCPrintingSetPrintScale(MCExecContext& ctxt, double p_scale)
{
	MCprinter -> SetLayoutScale(p_scale);
}

void MCPrintingGetPrintRotated(MCExecContext& ctxt, bool &r_rotated)
{
	switch(MCprinter -> GetPageOrientation())
	{
	case PRINTER_ORIENTATION_PORTRAIT:
	case PRINTER_ORIENTATION_REVERSE_PORTRAIT:
		r_rotated = false;
		break;
	case PRINTER_ORIENTATION_LANDSCAPE:
	case PRINTER_ORIENTATION_REVERSE_LANDSCAPE:
		r_rotated = true;
		break;
	}
}

void MCPrintingSetPrintRotated(MCExecContext& ctxt, bool p_rotated)
{
	MCprinter -> SetPageOrientation(p_rotated ? PRINTER_ORIENTATION_LANDSCAPE : PRINTER_ORIENTATION_PORTRAIT);	
}

void MCPrintingGetPrintCommand(MCExecContext& ctxt, MCStringRef &r_command)
{	
	if (MCprinter -> GetDeviceCommand() == NULL)
		r_command = (MCStringRef)MCValueRetain(kMCEmptyString);
	else if (!MCStringCreateWithCString(MCprinter -> GetDeviceCommand(), r_command))
		ctxt . Throw();
}

void MCPrintingSetPrintCommand(MCExecContext& ctxt, MCStringRef p_command)
{
	MCprinter -> SetDeviceCommand(p_command);
}

void MCPrintingGetPrintFontTable(MCExecContext& ctxt, MCStringRef &r_table)
{
	// OK-2008-11-25: [[Bug 7475]] Null check needed to avoid crash
	const char *t_device_font_table;
	t_device_font_table = MCprinter -> GetDeviceFontTable();
	if (t_device_font_table == NULL)
		r_table = (MCStringRef)MCValueRetain(kMCEmptyString);
	else if (!MCStringCreateWithCString(t_device_font_table, r_table))
		ctxt . Throw();
}

void MCPrintingSetPrintFontTable(MCExecContext& ctxt, MCStringRef p_table)
{
	MCprinter -> SetDeviceFontTable(p_table);
}

////////////////////////////////////////////////////////////////////////////////

void MCPrintingGetPrinterNames(MCExecContext& ctxt, MCStringRef &r_names)
{
	if (!ctxt . EnsurePrintingIsAllowed())
		return;

	if (MCscreen -> listprinters(r_names))
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////
