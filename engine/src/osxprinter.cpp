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

#include "osxprefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "execpt.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"
#include "util.h"
#include "font.h"
#include "globals.h"
#include "metacontext.h"
#include "printer.h"
#include "mode.h"
#include "region.h"

#include "osxdc.h"
#include "osxprinter.h"
#include "sserialize_osx.h"

#include "graphicscontext.h"

#include <cups/ppd.h>
#include <pwd.h>

///////////////////////////////////////////////////////////////////////////////

#define PDEBUG(a, b)

///////////////////////////////////////////////////////////////////////////////

extern void MCRemotePrintSetupDialog(char *&r_reply_data, uint32_t &r_reply_data_size, uint32_t &r_result, const char *p_config_data, uint32_t p_config_data_size);
extern void MCRemotePageSetupDialog(char *&r_reply_data, uint32_t &r_reply_data_size, uint32_t &r_result, const char *p_config_data, uint32_t p_config_data_size);

///////////////////////////////////////////////////////////////////////////////

extern char *osx_cfstring_to_cstring(CFStringRef p_string, bool p_release = true);
extern bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, bool p_copy, bool p_invert, CGImageRef &r_image);
extern bool MCGImageToCGImage(MCGImageRef p_src, MCGRectangle p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image);
extern bool MCGImageToCGImage(MCGImageRef p_src, MCGRectangle p_src_rect, bool p_copy, bool p_invert, CGImageRef &r_image);

///////////////////////////////////////////////////////////////////////////////

bool MCMacOSXPrinter::c_sheet_pending = false;
bool MCMacOSXPrinter::c_sheet_accepted = false;

///////////////////////////////////////////////////////////////////////////////

CGAffineTransform MCGAffineTransformToCGAffineTransform(const MCGAffineTransform &p_transform)
{
	CGAffineTransform t_transform;
	t_transform.a = p_transform.a;
	t_transform.b = p_transform.b;
	t_transform.c = p_transform.c;
	t_transform.d = p_transform.d;
	t_transform.tx = p_transform.tx;
	t_transform.ty = p_transform.ty;
	
	return t_transform;
}

///////////////////////////////////////////////////////////////////////////////

void MCMacOSXPrinter::DoInitialize(void)
{
	m_valid = false;

	m_printer = NULL;
	m_printer_name = NULL;
	m_session = NULL;
	m_page_format = NULL;
	m_settings = NULL;
	m_paper = NULL;
	
	Reset(NULL, NULL, NULL);
}

void MCMacOSXPrinter::DoFinalize(void)
{
	delete m_printer_name;
	m_printer_name = NULL;

	if (m_printer != NULL)
	{
		PMRelease(m_printer);
		m_printer = NULL;
	}

	if (m_settings != NULL)
	{
		PMRelease(m_settings);
		m_settings = NULL;
	}

	if (m_page_format != NULL)
	{
		PMRelease(m_page_format);
		m_page_format = NULL;
	}

	if (m_session != NULL)
	{
		PMRelease(m_session);
		m_session = NULL;
	}
	
	if (m_paper != NULL)
	{
		PMRelease(m_paper);
		m_paper = NULL;
	}

	m_valid = false;
}

bool MCMacOSXPrinter::DoReset(const char *p_name)
{
	return Reset(p_name, NULL, NULL);
}

bool MCMacOSXPrinter::DoResetSettings(const MCString& p_settings)
{
	if (p_settings . getlength() == 0)
		return Reset(NULL, NULL, NULL);

	bool t_success;
	t_success = true;

	MCDictionary t_dictionary;
	if (t_success)	
		t_success = t_dictionary . Unpickle(p_settings . getstring(), p_settings . getlength());

	MCString t_name;
	if (t_success)
		t_success = t_dictionary . Get('NMEA', t_name);

	MCString t_settings_data;
	if (t_success)
		t_success = t_dictionary . Get('OSXS', t_settings_data);

	MCString t_page_format_data;
	if (t_success)
		t_success = t_dictionary . Get('OSXP', t_page_format_data);

	PMPrintSettings t_settings;
	t_settings = NULL;
	if (t_success)
	{
		Handle t_handle;
		if (PtrToHand(t_settings_data . getstring(), &t_handle, t_settings_data . getlength()) == noErr)
		{
			if (PMUnflattenPrintSettings(t_handle, &t_settings) != noErr)
				t_success = false;
			DisposeHandle(t_handle);
		}
		else
			t_success = false;
	}

	PMPageFormat t_page_format;
	t_page_format = NULL;
	if (t_success)
	{
		Handle t_handle;
		if (PtrToHand(t_page_format_data . getstring(), &t_handle, t_page_format_data . getlength()) == noErr)
		{
			if (PMUnflattenPageFormat(t_handle, &t_page_format) != noErr)
				t_success = false;
			DisposeHandle(t_handle);
		}
		else
			t_success = false;
	}

	if (t_success)
		t_success = Reset(t_name . getstring(), t_settings, t_page_format);
	else
	{
		PMRelease(t_settings);
		PMRelease(t_page_format);
	}

	return t_success;
}

void MCMacOSXPrinter::DoFetchSettings(void*& r_buffer, uint4& r_length)
{
	MCDictionary t_dictionary;

	bool t_success;
	t_success = m_valid;

	GetProperties(false);

	if (t_success)
	{
		Handle t_handle;
		t_handle = NULL;
		if (PMFlattenPageFormat(m_page_format, &t_handle) == noErr)
		{
			HLock(t_handle);
			t_dictionary . Set('OSXP', MCString((char *)*t_handle, GetHandleSize(t_handle)));
			HUnlock(t_handle);
			DisposeHandle(t_handle);
		}
		else
			t_success = false;
	}

	if (t_success)
	{
		Handle t_handle;
		t_handle = NULL;
		if (PMFlattenPrintSettings(m_settings, &t_handle) == noErr)
		{
			HLock(t_handle);
			t_dictionary . Set('OSXS', MCString((char *)*t_handle, GetHandleSize(t_handle)));
			HUnlock(t_handle);
			DisposeHandle(t_handle);
		}
		else
			t_success = false;
	}

	if (t_success)
	{
		const char *t_name;
		t_name = DoFetchName();
		t_dictionary . Set('NMEA', MCString(t_name, strlen(t_name) + 1));
	}

	if (t_success)
		t_dictionary . Pickle(r_buffer, r_length);
	else
	{
		r_buffer = NULL;
		r_length = 0;
	}
}

const char *MCMacOSXPrinter::DoFetchName(void)
{
	if (!m_valid)
		return NULL;

	// MW-2007-10-30: [[ Bug 5346 ]] PMPrinterGetName has copy semantics so
	//   osx_cfstring_to_cstring shouldn't release it.
	if (m_printer_name == NULL)
	{
		if (m_printer == NULL)
			m_printer_name = strdup("");
		else
			m_printer_name = osx_cfstring_to_cstring(PMPrinterGetName(m_printer), false);
	}

	return m_printer_name;
}

void MCMacOSXPrinter::DoResync(void)
{
	if (!m_resync)
		return;

	GetProperties(false);
}

MCPrinterDialogResult MCMacOSXPrinter::DoPrinterSetup(bool p_window_modal, Window p_owner)
{
	return DoDialog(p_window_modal, p_owner, true);
}

MCPrinterDialogResult MCMacOSXPrinter::DoPageSetup(bool p_window_modal, Window p_owner)
{
	return DoDialog(p_window_modal, p_owner, false);
}

MCPrinterResult MCMacOSXPrinter::DoBeginPrint(const char *p_document_name, MCPrinterDevice*& r_device)
{
	if (!m_valid)
		return PRINTER_RESULT_ERROR;

	GetProperties(true);

	MCMacOSXPrinterDevice *t_device;
	t_device = new MCMacOSXPrinterDevice;

	MCPrinterResult t_result;
	t_result = t_device -> Start(m_session, m_settings, m_page_format, GetJobColor());
	r_device = t_device;

	return t_result;
}

MCPrinterResult MCMacOSXPrinter::DoEndPrint(MCPrinterDevice* p_device)
{
	MCPrinterResult t_result;
	t_result = static_cast<MCMacOSXPrinterDevice *>(p_device) -> Finish();

	delete static_cast<MCMacOSXPrinterDevice *>(p_device);

	return t_result;
}

///////////////////////////////////////////////////////////////////////////////

bool MCMacOSXPrinter::Reset(const char *p_name, PMPrintSettings p_settings, PMPageFormat p_page_format)
{
	bool t_success;
	t_success = true;

	if (p_name != NULL && *p_name == '\0')
		p_name = NULL;

	// Convert the name to a core-foundation string - this is because the
	// PMPrinter object returns names in this form.
	//
	CFStringRef t_name;
	t_name = NULL;
	if (t_success && p_name != NULL)
	{
		t_name = CFStringCreateWithCString(kCFAllocatorDefault, p_name, kCFStringEncodingMacRoman);
		if (t_name == NULL)
			t_success = false;
	}

	// Fetch the list of available PMPrinters on the current system.
	//
	CFArrayRef t_printers;
	t_printers = NULL;
	if (t_success)
	{
		OSErr t_err;
		t_err = PMServerCreatePrinterList(kPMServerLocal, &t_printers);
		if (t_err != noErr)
			t_success = false;
	}

	// Search the list of printers for either:
	//   (a) The default printer if p_name == NULL
	//   (b) The printer with the given name
	//
	PMPrinter t_printer;
	t_printer = NULL;
	if (t_success)
	{
		OSErr t_err;
		for(CFIndex i = 0; i < CFArrayGetCount(t_printers); ++i)
		{
			PMPrinter t_printer_to_test;
			t_printer_to_test = (PMPrinter)CFArrayGetValueAtIndex(t_printers, i);
			if (t_name == NULL && PMPrinterIsDefault(t_printer_to_test))
			{
				t_printer = t_printer_to_test;
				break;
			}
			else if (t_name != NULL && CFStringCompare(t_name, PMPrinterGetName(t_printer_to_test), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
			{
				t_printer = t_printer_to_test;
				break;
			}
		}
	}

	// Create the print session object
	//
	PMPrintSession t_session;
	t_session = NULL;
	if (t_success)
	{
		OSErr t_err;
		t_err = PMCreateSession(&t_session);
		if (t_err != noErr)
			t_success = false;
	}

	// Create the print settings object
	//
	PMPrintSettings t_settings;
	t_settings = p_settings;
	if (t_success && t_settings == NULL)
	{
		OSErr t_err;
		t_err = PMCreatePrintSettings(&t_settings);
		if (t_err != noErr)
			t_success = false;
	}

	// Create the page format object
	//
	PMPageFormat t_page_format;
	t_page_format = p_page_format;
	if (t_success && t_page_format == NULL)
	{
		OSErr t_err;
		t_err = PMCreatePageFormat(&t_page_format);
		if (t_err != noErr)
			t_success = false;
	}

	// Set the session printer
	//   On <  10.3 we use PMSessionSetCurrentPrinter with the printer name
	//   On >= 10.3 we use PMSessionSetCurrentPMPrinter
	if (t_success && t_printer != NULL)
	{
		OSErr t_err;
		if (PMSessionSetCurrentPMPrinter != NULL)
			t_err = PMSessionSetCurrentPMPrinter(t_session, t_printer);
		else
			t_err = PMSessionSetCurrentPrinter(t_session, PMPrinterGetName(t_printer));

		if (t_err != noErr)
			t_success = false;
	}

	// Default the print settings
	//
	if (t_success)
	{
		OSErr t_err;
		t_err = p_settings != NULL ? PMSessionValidatePrintSettings(t_session, t_settings, NULL) : PMSessionDefaultPrintSettings(t_session, t_settings);
		if (t_err != noErr)
			t_success = false;
	}

	// Default the page setup
	//
	if (t_success)
	{
		OSErr t_err;
		t_err = p_page_format != NULL ? PMSessionValidatePageFormat(t_session, t_page_format, NULL) : PMSessionDefaultPageFormat(t_session, t_page_format);
		if (t_err != noErr)
			t_success = false;
	}

	PMPaper t_paper;
	t_paper = NULL;
	if (t_success && MCmajorosversion >= 0x1040 && PMGetPageFormatPaper != NULL)
	{
		OSErr t_err;
		t_err = PMGetPageFormatPaper(t_page_format, &t_paper);
		if (t_err != noErr)
			t_success = false;
		else
			PMRetain(t_paper);
	}

	if (t_success)
	{
		delete m_printer_name;
		m_printer_name = NULL;
		
		if (m_printer != NULL)
			PMRelease(m_printer);
		if (m_session != NULL)
			PMRelease(m_session);
		if (m_settings != NULL)
			PMRelease(m_settings);
		if (m_page_format != NULL)
			PMRelease(m_page_format);
		if (m_paper != NULL)
			PMRelease(m_paper);

		if (t_printer != NULL)
			PMRetain(t_printer);

		m_printer = t_printer;
		m_session = t_session;
		m_settings = t_settings;
		m_page_format = t_page_format;
		m_paper = t_paper;
		m_valid = true;

		SetProperties();
	}
	else
	{
		if (t_page_format != NULL)
			PMRelease(t_page_format);
		if (t_settings != NULL)
			PMRelease(t_settings);
		if (t_session != NULL)
			PMRelease(t_session);
		if (t_paper != NULL)
			PMRelease(t_paper);
	}

	if (t_printers != NULL)
		CFRelease(t_printers);

	if (t_name != NULL)
		CFRelease(t_name);

	return t_success;
}

void MCMacOSXPrinter::SetProperties(bool p_include_output)
{
	double t_page_scale;
	int32_t t_page_width;
	int32_t t_page_height;
	int32_t t_job_copies;
	MCPrinterDuplexMode t_job_duplex;
	bool t_job_collate;

	MCPrinterOrientation t_page_orientation;
	PMOrientation t_pm_orientation;
	PDEBUG(stderr, "SetProperties: PMGetOrientation\n");
	if (PMGetOrientation(m_page_format, &t_pm_orientation) == noErr)
	{
		switch(t_pm_orientation)
		{
		case kPMPortrait:
			t_page_orientation = PRINTER_ORIENTATION_PORTRAIT;
		break;
		case kPMReversePortrait:
			t_page_orientation = PRINTER_ORIENTATION_REVERSE_PORTRAIT;
		break;
		case kPMLandscape:
			t_page_orientation = PRINTER_ORIENTATION_LANDSCAPE;
		break;
		case kPMReverseLandscape:
			t_page_orientation = PRINTER_ORIENTATION_REVERSE_LANDSCAPE;
		break;
		}
	}
	else
		t_page_orientation = PRINTER_DEFAULT_PAGE_ORIENTATION;

	PDEBUG(stderr, "SetProperties: PMGetScale\n");
	if (PMGetScale(m_page_format, &t_page_scale) != noErr)
		t_page_scale = 1.0f;

	PDEBUG(stderr, "SetProperties: PMGetPageFormatPaper\n");
	PMRect t_pm_paper_rect;
	PMPaper t_pm_paper;
	if (PMGetPageFormatPaper != NULL && PMGetPageFormatPaper(m_page_format, &t_pm_paper) == noErr)
	{
		PDEBUG(stderr, "SetProperties: PMPaperGetWidth\n");
		double t_width;
		PMPaperGetWidth(t_pm_paper, &t_width);
		
		PDEBUG(stderr, "SetProperties: PMPaperGetHeight\n");
		double t_height;
		PMPaperGetHeight(t_pm_paper, &t_height);

		t_page_width = ceil(t_width);
		t_page_height = ceil(t_height);

		if (MCmajorosversion >= 0x1040)
		{
			if (m_paper != NULL)
				PMRelease(m_paper);
			
			m_paper = t_pm_paper;
			PMRetain(t_pm_paper);
		}
	}
	else
	{
		PDEBUG(stderr, "SetProperties: PMGetUnadjustedPaperRect\n");
		if (PMGetUnadjustedPaperRect(m_page_format, &t_pm_paper_rect) == noErr)
		{
			t_page_width = ceil(t_pm_paper_rect . right - t_pm_paper_rect . left);
			t_page_height = ceil(t_pm_paper_rect . bottom - t_pm_paper_rect . top);
		}
		else
		{
			t_page_width = PRINTER_DEFAULT_PAGE_WIDTH;
			t_page_height = PRINTER_DEFAULT_PAGE_HEIGHT;
		}
	}

	PDEBUG(stderr, "SetProperties: PMGetCopies\n");
	UInt32 t_pm_copies;
	if (PMGetCopies(m_settings, &t_pm_copies) == noErr)
		t_job_copies = (int32_t)t_pm_copies;
	else
		t_job_copies = PRINTER_DEFAULT_JOB_COPIES;

	PDEBUG(stderr, "SetProperties: PMGetCollate\n");
	Boolean t_pm_collate;
	if (PMGetCollate(m_settings, &t_pm_collate) == noErr)
		t_job_collate = t_pm_collate == true;
	else
		t_job_collate = PRINTER_DEFAULT_JOB_COLLATE;

	PDEBUG(stderr, "SetProperties: PMGetDuplex\n");
	PMDuplexMode t_pm_duplex;
	if (PMGetDuplex != NULL && PMGetDuplex(m_settings, &t_pm_duplex) == noErr)
		t_job_duplex = t_pm_duplex == kPMDuplexNone ? PRINTER_DUPLEX_MODE_SIMPLEX : (t_pm_duplex == kPMDuplexNoTumble ? PRINTER_DUPLEX_MODE_LONG_EDGE : PRINTER_DUPLEX_MODE_SHORT_EDGE);
	else
		t_job_duplex = PRINTER_DEFAULT_JOB_DUPLEX;

	PDEBUG(stderr, "SetProperties: PMGetPageRange\n");
	UInt32 t_pm_first_page_range, t_pm_last_page_range;
	if (PMGetPageRange(m_settings, &t_pm_first_page_range, &t_pm_last_page_range) != noErr)
		t_pm_first_page_range = 1, t_pm_last_page_range = 1;

	PDEBUG(stderr, "SetProperties: PMGetFirstPage\n");
	UInt32 t_pm_first_page;
	if (PMGetFirstPage(m_settings, &t_pm_first_page) != noErr)
		t_pm_first_page = 1;

	PDEBUG(stderr, "SetProperties: PMGetLastPage\n");
	UInt32 t_pm_last_page;
	if (PMGetLastPage(m_settings, &t_pm_last_page) != noErr)
		t_pm_last_page = 0xFFFFFFFF;

	PDEBUG(stderr, "SetProperties: SetJobRanges\n");
	if (t_pm_last_page < 65536)
	{
		MCRange t_range;
		t_range . from = t_pm_first_page;
		t_range . to = t_pm_last_page;
		SetJobRanges(1, &t_range);
	}
	else
		SetJobRanges(PRINTER_PAGE_RANGE_ALL, NULL);	

	SetPageSize(t_page_width, t_page_height);
	SetPageOrientation(t_page_orientation);
	
	// MW-2008-03-15: [[ Bug 6080 ]] The MCPrinter objects' page scale is a scale factor,
	//   not a percentage.
	SetPageScale(t_page_scale / 100.0);
	
	SetJobCopies(t_job_copies);
	SetJobColor(PRINTER_DEFAULT_JOB_COLOR);
	SetJobDuplex(t_job_duplex);
	SetJobCollate(t_job_collate);

	if (p_include_output)
	{
		PMDestinationType t_type;
		CFURLRef t_output_location_url;
		CFStringRef t_output_format;
		
		t_output_location_url = NULL;
		t_output_format = NULL;
		
		MCPrinterOutputType t_output_type;
		char *t_output_location;
		if (PMSessionGetDestinationType(m_session, m_settings, &t_type) != noErr ||
			PMSessionCopyDestinationLocation(m_session, m_settings, &t_output_location_url) != noErr ||
			PMSessionCopyDestinationFormat(m_session, m_settings, &t_output_format) != noErr)
		{
			PDEBUG(stderr, "SetProperties: Output location unknown\n");
			t_output_type = PRINTER_OUTPUT_DEVICE;
			t_output_location = NULL;
		}
		else if (t_type == kPMDestinationFile && (t_output_format == NULL || CFStringCompare(t_output_format, kPMDocumentFormatPDF, 0) == 0))
		{
			PDEBUG(stderr, "SetProperties: Output location is file\n");
			CFStringRef t_output_format;
			t_output_type = PRINTER_OUTPUT_FILE;
			t_output_location = osx_cfstring_to_cstring(CFURLCopyFileSystemPath(t_output_location_url, kCFURLPOSIXPathStyle), true);
		}
		else if (t_type == kPMDestinationPrinter)
		{
			PDEBUG(stderr, "SetProperties: Output location is device\n");
			t_output_type = PRINTER_OUTPUT_DEVICE;
			t_output_location = NULL;
		}
		else
		{
			PDEBUG(stderr, "SetProperties: Output location is system\n");
			t_output_type = PRINTER_OUTPUT_SYSTEM;
			t_output_location = NULL;
		}

		SetDeviceOutput(t_output_type, t_output_location);

		delete t_output_location;

		if (t_output_location_url != NULL)
			CFRelease(t_output_location_url);
		if (t_output_format != NULL)
			CFRelease(t_output_format);
	}

	SetDerivedProperties();

	delete m_printer_name;
	m_printer_name = NULL;
	
	if (m_printer != NULL)
	{
		PMRelease(m_printer);
		m_printer = NULL;
	}

	PDEBUG(stderr, "SetProperties: PMSessionGetCurrentPrinter\n");
	PMSessionGetCurrentPrinter(m_session, &m_printer);
	if (m_printer != NULL)
		PMRetain(m_printer);
		
	PDEBUG(stderr, "SetProperties: END\n");
}

void MCMacOSXPrinter::SetDerivedProperties(void)
{
	PMRect t_pm_page_rect, t_pm_paper_rect;
	PDEBUG(stderr, "SetProperties: PMGetAdjustedPaperRect\n");
	if (PMGetAdjustedPaperRect(m_page_format, &t_pm_paper_rect) == noErr && PMGetAdjustedPageRect(m_page_format, &t_pm_page_rect) == noErr)
	{
		int4 t_left;
		t_left = (int4)floor(t_pm_page_rect . left - t_pm_paper_rect . left);
		
		int4 t_top;
		t_top = (int4)floor(t_pm_page_rect . top - t_pm_paper_rect . top);
		
		int4 t_right;
		t_right = (int4)ceil(t_pm_page_rect . right - t_pm_paper_rect . left);
		
		int4 t_bottom;
		t_bottom = (int4)ceil(t_pm_page_rect . bottom - t_pm_paper_rect . top);

		MCRectangle t_rect;
		MCU_set_rect(t_rect, t_left, t_top, t_right - t_left, t_bottom - t_top);
		
		SetDeviceRectangle(t_rect);
	}

	MCPrinterFeatureSet t_features;
	t_features = PRINTER_FEATURE_COPIES | PRINTER_FEATURE_COLLATE;
	
	PDEBUG(stderr, "SetProperties: PMPrinterGetDescriptionURL\n");
	CFURLRef t_ppd_url;
	t_ppd_url = NULL;
	if (PMPrinterGetDescriptionURL(m_printer, kPMPPDDescriptionType, &t_ppd_url) == noErr)
	{
		char *t_ppd_file;
		t_ppd_file = osx_cfstring_to_cstring(CFURLCopyFileSystemPath(t_ppd_url, kCFURLPOSIXPathStyle), true);
		if (t_ppd_file != NULL)
		{
			ppd_file_t *t_ppd;
			t_ppd = ppdOpenFile(t_ppd_file);
			if (t_ppd != NULL)
			{
				if (t_ppd -> color_device != 0)
					t_features |= PRINTER_FEATURE_COLOR;
				
				ppd_option_t *t_duplex;
				t_duplex = ppdFindOption(t_ppd, "Duplex");
				if (t_duplex == NULL)
					t_duplex = ppdFindOption(t_ppd, "EFDuplex");
				if (t_duplex == NULL)
					t_duplex = ppdFindOption(t_ppd, "EFDuplexing");
				if (t_duplex == NULL)
					t_duplex = ppdFindOption(t_ppd, "KD03Duplex");
				if (t_duplex == NULL)
					t_duplex = ppdFindOption(t_ppd, "JCLDuplex");
					
				if (t_duplex != NULL && t_duplex -> num_choices > 1)
					t_features |= PRINTER_FEATURE_DUPLEX;
				
				ppdClose(t_ppd);
			}
			free(t_ppd_file);
		}
		CFRelease(t_ppd_url);
	}
	
	SetDeviceFeatures(t_features);
}

void MCMacOSXPrinter::GetProperties(bool p_include_output)
{
	if (!m_valid)
		return;

	OSStatus t_err;

	t_err = PMSetPageRange(m_settings, 1, 0xFFFFFFFF);

	t_err = PMSetCollate(m_settings, GetJobCollate());
	t_err = PMSetCopies(m_settings, GetJobCopies(), False);
	if (&PMSetDuplex != NULL)
		t_err = PMSetDuplex(m_settings, GetJobDuplex() == PRINTER_DUPLEX_MODE_SIMPLEX ? kPMDuplexNone : (GetJobDuplex() == PRINTER_DUPLEX_MODE_LONG_EDGE ? kPMDuplexNoTumble : kPMDuplexTumble));
	
	if (MCmajorosversion >= 0x1040)
	{
		if (m_page_format != NULL)
		{
			PMRelease(m_page_format);
			m_page_format = NULL;
		}
		
		PMPaper t_paper;
		t_paper = NULL;
		
		if (m_paper != NULL)
		{
			double t_width;
			PMPaperGetWidth(m_paper, &t_width);
			
			double t_height;
			PMPaperGetHeight(m_paper, &t_height);

			if (ceil(t_width) == GetPageWidth() && ceil(t_height) == GetPageHeight())
			{
				t_paper = m_paper;
				PMRetain(t_paper);
			}
		}
		
		if (t_paper == NULL)
		{
			CFArrayRef t_array;
			PMPrinterGetPaperList(m_printer, &t_array);
			for(uint4 i = 0; i < CFArrayGetCount(t_array); ++i)
			{
				PMPaper t_paper_to_test;
				t_paper_to_test = (PMPaper)CFArrayGetValueAtIndex(t_array, i);
				
				double t_width;
				PMPaperGetWidth(t_paper_to_test, &t_width);
				
				double t_height;
				PMPaperGetHeight(t_paper_to_test, &t_height);
				
				if (ceil(t_width) == GetPageWidth() && ceil(t_height) == GetPageHeight())
				{
					t_paper = t_paper_to_test;
					PMRetain(t_paper);
					break;
				}
			}
		}
		
		if (t_paper == NULL)
		{
			PMPaperMargins t_margins;
			t_margins . left = 0.0;
			t_margins . top = 0.0;
			t_margins . right = 0.0;
			t_margins . bottom = 0.0;
					
			PMPaperCreate(m_printer, CFSTR("Revolution"), CFSTR("Revolution Custom Paper"), GetPageWidth(), GetPageHeight(), &t_margins, &t_paper);
		}
		
		PMCreatePageFormatWithPMPaper(&m_page_format, t_paper);
		PMRelease(t_paper);
	}
	else
	{
		PMRect t_paper_rect;
		t_paper_rect . left = 0.0;
		t_paper_rect . top = 0.0;
		t_paper_rect . right = GetPageWidth();
		t_paper_rect . bottom = GetPageHeight();
		t_err = PMSetPhysicalPaperSize(m_page_format, &t_paper_rect);
	}
	
	t_err = PMSetScale(m_page_format, GetPageScale() * 100.0);
	switch(GetPageOrientation())
	{
	case PRINTER_ORIENTATION_PORTRAIT:
		t_err = PMSetOrientation(m_page_format, kPMPortrait, False);
		break;
	case PRINTER_ORIENTATION_REVERSE_PORTRAIT:
		t_err = PMSetOrientation(m_page_format, kPMReversePortrait, False);
		break;
	case PRINTER_ORIENTATION_LANDSCAPE:
		t_err = PMSetOrientation(m_page_format, kPMLandscape, False);
		break;
	case PRINTER_ORIENTATION_REVERSE_LANDSCAPE:
		t_err = PMSetOrientation(m_page_format, kPMReverseLandscape, False);
		break;
	}

	// Configure the output mode
	if (p_include_output && GetDeviceOutputType() != PRINTER_OUTPUT_SYSTEM)
	{
		PMDestinationType t_output_type;
		CFURLRef t_output_url;
		switch(GetDeviceOutputType())
		{
		case PRINTER_OUTPUT_DEVICE:
			t_output_type = kPMDestinationPrinter;
			t_output_url = NULL;
			break;
		case PRINTER_OUTPUT_PREVIEW:
			t_output_type = kPMDestinationPreview;
			t_output_url = NULL;
			break;
		case PRINTER_OUTPUT_FILE:
		{
			CFStringRef t_output_file;
			t_output_file = CFStringCreateWithCString(kCFAllocatorDefault, GetDeviceOutputLocation(), kCFStringEncodingMacRoman);
			t_output_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, t_output_file, kCFURLPOSIXPathStyle, false);
			CFRelease(t_output_file);
			t_output_type = kPMDestinationFile;
		}
		break;
		}

		t_err = PMSessionSetDestination(m_session, m_settings, t_output_type, kPMDocumentFormatPDF, t_output_url);
		
		if (t_output_url != NULL)
			CFRelease(t_output_url);
	}
	
	const char *t_document_name;
	t_document_name = GetJobName();
	if (t_document_name == NULL)
		t_document_name = MCdefaultstackptr -> gettitletext();
	
	CFStringRef t_cf_document;
	t_cf_document = CFStringCreateWithCString(kCFAllocatorDefault, t_document_name, kCFStringEncodingMacRoman);
	if (t_cf_document != NULL)
	{
		PMSetJobNameCFString(m_settings, t_cf_document);
		CFRelease(t_cf_document);
	}
	
	t_err = PMSessionValidatePrintSettings(m_session, m_settings, kPMDontWantBoolean);
	t_err = PMSessionValidatePageFormat(m_session, m_page_format, kPMDontWantBoolean);

	SetDerivedProperties();
	m_resync = false;
}

void MCMacOSXPrinter::ResetSession(void)
{
	PMPrintSession t_session;
	PMCreateSession(&t_session);
	if (PMSessionSetCurrentPMPrinter != NULL)
		PMSessionSetCurrentPMPrinter(t_session, m_printer);
	else
		PMSessionSetCurrentPrinter(t_session, PMPrinterGetName(m_printer));
	
	PMSessionValidatePrintSettings(t_session, m_settings, kPMDontWantBoolean);
	PMSessionValidatePageFormat(t_session, m_page_format, kPMDontWantBoolean);
	
	PMDestinationType t_type;
	
	CFURLRef t_output_location_url;
	t_output_location_url = NULL;
	
	CFStringRef t_output_format;
	t_output_format = NULL;
			
	if (PMSessionGetDestinationType(m_session, m_settings, &t_type) == noErr &&
		PMSessionCopyDestinationLocation(m_session, m_settings, &t_output_location_url) == noErr &&
		PMSessionCopyDestinationFormat(m_session, m_settings, &t_output_format) == noErr)
		PMSessionSetDestination(t_session, m_settings, t_type, t_output_format, t_output_location_url);
	
	if (t_output_format != NULL)
		CFRelease(t_output_format);
		
	if (t_output_location_url != NULL)
		CFRelease(t_output_location_url);
		
	PMRelease(m_session);
	
	m_session = t_session;

}

MCPrinterDialogResult MCMacOSXPrinter::DoDialog(bool p_window_modal, Window p_owner, bool p_is_settings)
{
	if (!m_valid)
		return PRINTER_DIALOG_RESULT_ERROR;

	GetProperties();
	
	ResetSession();

	PMSheetDoneUPP t_sheet_done_callback;
	t_sheet_done_callback = NULL;

	Boolean t_accepted;
	t_accepted = false;
	OSErr t_err;

	if (!MCModeMakeLocalWindows())
	{
		bool t_success = true;
		// serialize printer + print settings + page format, display remotely then deserialize returned data
		char *t_data = NULL;
		uint32_t t_data_size = 0;
		char *t_reply_data = NULL;
		uint32_t t_reply_data_size = 0;
		uint32_t t_result;
		t_success = serialize_printer_settings(t_data, t_data_size, m_session, m_printer, m_settings, m_page_format);
		PMPrinter t_printer = NULL;
		if (t_success)
		{
			if (p_is_settings)
				MCRemotePrintSetupDialog(t_reply_data, t_reply_data_size, t_result, t_data, t_data_size);
			else
				MCRemotePageSetupDialog(t_reply_data, t_reply_data_size, t_result, t_data, t_data_size);
			t_success = deserialize_printer_settings(t_reply_data, t_reply_data_size, m_session, t_printer, m_settings, m_page_format);
			free(t_reply_data);
		}
		if (t_success)
		{
			PMSessionSetCurrentPMPrinter(m_session, t_printer);
			PMRelease(m_printer);
			m_printer = t_printer;
		}
		if (t_success)
		{
			t_err = noErr;
			t_accepted = (Boolean)t_result;
		}
		else
			t_err = errAborted;
	}
	else
	{
		if (p_window_modal && p_owner != NULL)
		{
			t_sheet_done_callback = NewPMSheetDoneUPP(SheetDoneCallback);
			PMSessionUseSheets(m_session, (WindowPtr)p_owner -> handle . window, t_sheet_done_callback);
			
			c_sheet_pending = true;
			c_sheet_accepted = false;
		}

		Cursor t_cursor;
		SetCursor(GetQDGlobalsArrow(&t_cursor)); // bug in OS X doesn't reset this
		ShowCursor();
		
		PDEBUG(stderr, "DoDialog: Showing dialog\n");
		
		if (p_is_settings)
			t_err = PMSessionPrintDialog(m_session, m_settings, m_page_format, &t_accepted);
		else
			t_err = PMSessionPageSetupDialog(m_session, m_page_format, &t_accepted);

		PDEBUG(stderr, "DoDialog: Dialog shown\n");
	}

	if (t_err == noErr && c_sheet_pending)
	{
		while(c_sheet_pending)
			MCscreen -> wait(REFRESH_INTERVAL, True, True);
				
		t_accepted = c_sheet_accepted;
	}

	if (t_sheet_done_callback != NULL)
	{
		DisposePMSheetDoneUPP(t_sheet_done_callback);

		// MW-2009-01-22: [[ Bug 3509 ]] Make sure we force an update to the menubar, just in case
		//   the dialog has messed with our menu item enabled state!
		MCscreen -> updatemenubar(True);
	}

	if (t_err != noErr)
	{
		PDEBUG(stderr, "DoDialog: Error occured\n");
		return PRINTER_DIALOG_RESULT_ERROR;
	}
	else if (t_accepted)
	{
		PDEBUG(stderr, "DoDialog: SetProperties\n");
		SetProperties(p_is_settings);
		PDEBUG(stderr, "DoDialog: Returning OKAY\n");
		return PRINTER_DIALOG_RESULT_OKAY;
	}
	else
	{
		PDEBUG(stderr, "DoDialog: Returning Cancel\n");
		return PRINTER_DIALOG_RESULT_CANCEL;
	}
}

void MCMacOSXPrinter::SheetDoneCallback(PMPrintSession p_session, WindowRef p_window, Boolean p_accepted)
{
	c_sheet_pending = false;
	c_sheet_accepted = p_accepted;
}

///////////////////////////////////////////////////////////////////////////////

#define GRAYSCALE(R,G,B) (0.3 * R + 0.59 * G + 0.11 * B)

class MCQuartzMetaContext: public MCMetaContext
{
public:
	MCQuartzMetaContext(const MCRectangle& p_rect, int p_width, int p_height);
	~MCQuartzMetaContext(void);
	
	void render(PMPrintSession p_session, const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, bool p_color);

protected:
	void domark(MCMark *p_mark);
	bool candomark(MCMark *p_mark);
	
	bool begincomposite(const MCRectangle &p_region, MCGContextRef &r_context);
	void endcomposite(MCRegionRef p_clip_region);

private:
	bool m_color;
	
	int m_page_width, m_page_height;
	
	CGContextRef m_context;

	MCGContextRef m_composite_context;
	MCRectangle m_composite_rect;
};

//

static void MCPrinterRectangleToCGRect(const MCPrinterRectangle& mc, CGRect& cg, int h)
{
	cg . origin . x = mc . left;
	cg . origin . y = h - mc . bottom;
	cg . size . width = mc . right - mc . left;
	cg . size . height = mc . bottom - mc . top;
}

static void MCRectangleToCGRect(const MCRectangle& mc, CGRect& cg, int h)
{
	cg . origin . x = mc . x;
	cg . origin . y = h - (mc . y + mc . height);
	cg . size . width = mc . width;
	cg . size . height = mc . height;
}

static void CGImagePatternDrawCallback(void *p_info, CGContextRef p_context)
{
	CGImageRef t_image;
	t_image = (CGImageRef)p_info;
	CGContextDrawImage(p_context, CGRectMake(0, 0, CGImageGetWidth(t_image), CGImageGetHeight(t_image)), t_image);
}

static void FreeData(void *info, const void *data, size_t size)
{
	free(info);
}

static CGColorSpaceRef OSX_CGColorSpaceCreateWithProfile(const char *p_profile_path)
{
	OSStatus t_err;
	t_err = noErr;

	CMProfileLocation t_location;
	t_location . locType = cmPathBasedProfile;
	strcpy(t_location . u . pathLoc . path, p_profile_path);
	
	CMProfileRef t_profile;
	t_profile = NULL;
	
	t_err = CMOpenProfile(&t_profile, &t_location);
	if (t_err == noErr)
	{
		CGColorSpaceRef t_colorspace;
		t_colorspace = CGColorSpaceCreateWithPlatformColorSpace(t_profile);
		
		CMCloseProfile(t_profile);
		
		return t_colorspace;
	}
	
	return NULL;
}

static CGColorSpaceRef OSX_CGColorSpaceCreateGenericGray(void)
{
	static CGColorSpaceRef s_colorspace = NULL;
	if (s_colorspace == NULL)
	{
		s_colorspace = OSX_CGColorSpaceCreateWithProfile("/System/Library/ColorSync/Profiles/Generic Gray Profile.icc");
		if (s_colorspace == NULL)
			s_colorspace = CGColorSpaceCreateDeviceGray();
	}
	
	CGColorSpaceRetain(s_colorspace);
	
	return s_colorspace;
}

CGColorSpaceRef OSX_CGColorSpaceCreateGenericRGB(void)
{
	static CGColorSpaceRef s_colorspace = NULL;
	if (s_colorspace == NULL)
	{
		s_colorspace = OSX_CGColorSpaceCreateWithProfile("/System/Library/ColorSync/Profiles/Generic RGB Profile.icc");
		if (s_colorspace == NULL)
			s_colorspace = CGColorSpaceCreateDeviceRGB();
	}
	
	CGColorSpaceRetain(s_colorspace);
	
	return s_colorspace;
}

bool MCGImageToCGImage(MCGImageRef p_src, CGImageRef &r_image)
{
	bool t_success = true;
	
	/* OVERHAUL - REVISIT: for a grayscale image this should be a grayscale colorspace */
	CGColorSpaceRef t_colorspace = nil;
	if (t_success)
		t_success = nil != (t_colorspace = OSX_CGColorSpaceCreateGenericRGB());
	
	MCGRectangle t_src_rect = MCGRectangleMake(0, 0, MCGImageGetWidth(p_src), MCGImageGetHeight(p_src));
	if (t_success)
		t_success = MCGImageToCGImage(p_src, t_src_rect, t_colorspace, true, true, r_image);
	
	if (t_colorspace != nil)
		CGColorSpaceRelease(t_colorspace);
	
	return t_success;
}

static OSStatus OSX_PMSessionGetCGGraphicsContext(PMPrintSession p_session, CGContextRef* r_context)
{
	if (&PMSessionGetCGGraphicsContext != NULL)
		return PMSessionGetCGGraphicsContext(p_session, r_context);
		
	return PMSessionGetGraphicsContext(p_session, kPMGraphicsContextCoreGraphics, reinterpret_cast<void **>(r_context));
}

static OSStatus OSX_PMSessionBeginCGDocument(PMPrintSession p_session, PMPrintSettings p_settings, PMPageFormat p_format)
{
	if (&PMSessionBeginCGDocument != NULL)
		return PMSessionBeginCGDocument(p_session, p_settings, p_format);
	
	CFStringRef t_context_strings[1];
	t_context_strings[0] = kPMGraphicsContextCoreGraphics;
	
	CFArrayRef t_context_array;
	t_context_array = CFArrayCreate(kCFAllocatorDefault, (const void **)t_context_strings, 1, &kCFTypeArrayCallBacks);
	
	OSStatus t_err;
	t_err = PMSessionSetDocumentFormatGeneration(p_session, kPMDocumentFormatPDF, t_context_array, NULL);
	CFRelease(t_context_array);
	
	if (t_err == noErr)
		return PMSessionBeginDocument(p_session, p_settings, p_format);
	
	return t_err;
}

static void OSX_CGContextAddRoundedRect(CGContextRef p_context, CGRect p_rect, float p_radius)
{
	float hr;
	hr = MCU_fmin(p_radius / 2.0f, p_rect . size . width / 2.0f);
	
	float vr;
	vr = MCU_fmin(p_radius / 2.0f, p_rect . size . height / 2.0f);
	
	float l;
	l = p_rect . origin . x + hr;
	
	float t;
	t = p_rect . origin . y + vr;
	
	float r;
	r = p_rect . origin . x + p_rect . size . width - hr;
	
	float b;
	b = p_rect . origin . y + p_rect . size . height - vr;
	
	float hk;
	hk = hr * 36195.0f / 65536.0f;
	
	float vk;
	vk = hr * 36195.0f / 65536.0f;
	
	CGContextMoveToPoint(p_context, r, t - vr);
	CGContextAddCurveToPoint(p_context, r + hk, t - vr, r + hr, t - vk, r + hr, t);
	CGContextAddLineToPoint(p_context, r + hr, b);
	CGContextAddCurveToPoint(p_context, r + hr, b + vk, r + hk, b + vr, r, b + vr);
	CGContextAddLineToPoint(p_context, l, b + vr);
	CGContextAddCurveToPoint(p_context, l - hk, b + vr, l - hr, b + vk, l - hr, b);
	CGContextAddLineToPoint(p_context, l - hr, t);
	CGContextAddCurveToPoint(p_context, l - hr, t - vk, l - hk, t - vr, l, t - vr);
	CGContextClosePath(p_context);
}

static void OSX_CGContextAddArcSection(CGContextRef p_context, float cx, float cy, float hr, float vr, float s, float e, bool first)
{
	float hk, vk;

	if (e - s == 90.0f)
	{
		hk = hr * 36195.0f / 65536.0f;
		vk = vr * 36195.0f / 65536.0f;
	}
	else
	{
		float h = tan(M_PI * (e - s) / 720.0f);
		hk = 4.0f * h * hr / 3.0f;
		vk = 4.0f * h * vr / 3.0f;
	}

	float ca, sa;
	float cb, sb;

	ca = cos(M_PI * s / 180.0f);
	sa = sin(M_PI * s / 180.0f);

	cb = cos(M_PI * e / 180.0f);
	sb = sin(M_PI * e / 180.0f);

	if (first)
		CGContextMoveToPoint(p_context, cx + hr * ca, cy - vr * sa);

	CGContextAddCurveToPoint(p_context,
		cx + hr * ca - hk * sa, cy - vr * sa - vk * ca,
		cx + hr * cb + hk * sb, cy - vr * sb + vk * cb,
		cx + hr * cb, cy - vr * sb);
}

static void OSX_CGContextAddArc(CGContextRef p_context, CGRect p_rect, float p_start, float p_angle)
{
	if (p_angle > 360.0f)
		p_angle = 360.0f;
		
	p_start = fmod(p_start, 360.0f);
	
	float cx;
	cx = p_rect . origin . x + p_rect . size . width / 2.0f;
	
	float cy;
	cy = p_rect . origin . y + p_rect . size . height / 2.0f;
	
	float hr;
	hr = p_rect . size . width / 2.0f;
	
	float vr;
	vr = p_rect . size . height / 2.0f;
	
	bool t_first;
	t_first = true;
	
	float t_delta;
	
	while(p_angle > 0.0f)
	{
		t_delta = MCU_fmin(90.0f - fmod(p_start, 90.0f), p_angle);
		p_angle -= t_delta;
		
		OSX_CGContextAddArcSection(p_context, cx, cy, hr, vr, p_start, p_start + t_delta, t_first);
		
		p_start += t_delta;
		t_first = false;
	}
	
	CGContextClosePath(p_context);
}

static void OSX_CGContextAddSegment(CGContextRef p_context, CGRect p_rect, float p_start, float p_angle)
{
	if (p_angle > 360.0f)
		p_angle = 360.0f;
		
	p_start = fmod(p_start, 360.0f);
	
	float cx;
	cx = p_rect . origin . x + p_rect . size . width / 2.0f;
	
	float cy;
	cy = p_rect . origin . y + p_rect . size . height / 2.0f;
	
	float hr;
	hr = p_rect . size . width / 2.0f;
	
	float vr;
	vr = p_rect . size . height / 2.0f;
	
	bool t_first;
	t_first = true;
	
	float t_delta;
	
	while(p_angle > 0.0f)
	{
		t_delta = MCU_fmin(90.0f - fmod(p_start, 90.0f), p_angle);
		p_angle -= t_delta;
		
		OSX_CGContextAddArcSection(p_context, cx, cy, hr, vr, p_start, p_start + t_delta, t_first);
		
		p_start += t_delta;
		t_first = false;
	}
	
	CGContextAddLineToPoint(p_context, cx, cy);
	CGContextClosePath(p_context);
}

struct RegionToRectsInfo
{
	unsigned int count;
	CGRect *rectangles;
};

static OSStatus RegionToRectsCallback(UInt16 p_message, RgnHandle p_region, const Rect *p_rect, void *p_context)
{
	RegionToRectsInfo *t_info;
	t_info = (RegionToRectsInfo *)p_context;
	
	if (p_message == kQDRegionToRectsMsgParse)
	{
		t_info -> count += 1;
		t_info -> rectangles = (CGRect *)realloc(t_info -> rectangles, t_info -> count * sizeof(CGRect));
		t_info -> rectangles[t_info -> count - 1] = CGRectMake(p_rect -> left, p_rect -> top, p_rect -> right - p_rect -> left, p_rect -> bottom - p_rect -> top);
	}
	
	return noErr;
}

static void OSX_CGContextClipToRegion(CGContextRef p_context, RgnHandle p_region)
{
	RegionToRectsInfo t_info;
	t_info . count = 0;
	t_info . rectangles = NULL;
	QDRegionToRects(p_region, 0, (RegionToRectsUPP)RegionToRectsCallback, &t_info);
	CGContextClipToRects(p_context, t_info . rectangles, t_info . count);
	free(t_info . rectangles);
}

//

MCQuartzMetaContext::MCQuartzMetaContext(const MCRectangle& p_rect, int p_page_width, int p_page_height)
	: MCMetaContext(p_rect)
{
	m_color = true;
	m_page_height = p_page_height;
	m_page_width = p_page_width;
	m_context = NULL;
}

MCQuartzMetaContext::~MCQuartzMetaContext(void)
{
}

void MCQuartzMetaContext::render(PMPrintSession p_session, const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, bool p_color)
{
	m_color = p_color;
	
	if (OSX_PMSessionGetCGGraphicsContext(p_session, &m_context) == noErr)
	{
		CGContextSaveGState(m_context);

		CGContextTranslateCTM(m_context, 0.0f, m_page_height);
		CGContextScaleCTM(m_context, 1.0f, -1.0f);
		CGContextTranslateCTM(m_context, p_dst_rect . left, p_dst_rect . top);
		CGContextScaleCTM(m_context,
			(p_dst_rect . right - p_dst_rect . left) / (p_src_rect . right - p_src_rect . left),
			(p_dst_rect . bottom - p_dst_rect . top) / (p_src_rect . bottom - p_src_rect . top));
		CGContextTranslateCTM(m_context, -p_src_rect . left, -p_src_rect . top);
		
		execute();
		
		CGContextRestoreGState(m_context);
	}
}

bool MCQuartzMetaContext::candomark(MCMark *p_mark)
{
	// As currently implemented, the system contexts can do all marks unless it
	// is a group. Group marks are only generated precisely when the (most feeble)
	// system context cannot render it.
	return p_mark -> type != MARK_TYPE_GROUP;
}

void MCQuartzMetaContext::domark(MCMark *p_mark)
{
	CGContextSaveGState(m_context);
	
	CGContextClipToRect(m_context, CGRectMake(p_mark -> clip . x, p_mark -> clip . y, p_mark -> clip . width, p_mark -> clip . height));
	
	bool t_is_stroke;
	if (p_mark -> stroke != NULL)
	{
		float t_line_width;
		t_line_width = p_mark -> stroke -> width == 0 ? 1.0f : p_mark -> stroke -> width;
		
		CGLineCap t_line_cap;
		switch(p_mark -> stroke -> cap)
		{
			case CapButt:
				t_line_cap = kCGLineCapButt;
			break;
			
			case CapRound:
				t_line_cap = kCGLineCapRound;
			break;
			
			case CapProjecting:
				t_line_cap = kCGLineCapSquare;
			break;
		}
	
		CGLineJoin t_line_join;
		switch(p_mark -> stroke -> join)
		{
			case JoinRound:
				t_line_join = kCGLineJoinRound;
			break;
			
			case JoinMiter:
				t_line_join = kCGLineJoinMiter;
			break;
			
			case JoinBevel:
				t_line_join = kCGLineJoinBevel;
			break;
		}
		
		CGContextSetLineWidth(m_context, t_line_width);
		CGContextSetLineCap(m_context, t_line_cap);
		CGContextSetLineJoin(m_context, t_line_join);
		
		if (p_mark -> stroke -> dash . length > 1)
		{
			float *t_lengths;
			t_lengths = new float[p_mark -> stroke -> dash . length];
			for(uint4 i = 0; i < p_mark -> stroke -> dash . length; i++)
				t_lengths[i] = p_mark -> stroke -> dash . data[i];
			CGContextSetLineDash(m_context, p_mark -> stroke -> dash . offset, t_lengths, p_mark -> stroke -> dash . length);
			delete[] t_lengths;
		}
		
		t_is_stroke = true;
	}
	else
		t_is_stroke = false;
	
	if (p_mark -> fill != NULL)
	{
		if (p_mark -> fill -> style == FillTiled)
		{
			static CGPatternCallbacks s_pattern_callbacks =
			{
				0,
				CGImagePatternDrawCallback,
				(CGPatternReleaseInfoCallback)CGImageRelease
			};
		
			CGImageRef t_tile_image;
			/* UNCHECKED */ MCGImageToCGImage(p_mark -> fill -> pattern -> image, t_tile_image);
			
			// IM-2013-08-14: [[ ResIndependence ]] Append pattern image scale to pattern transform
			MCGFloat t_scale;
			t_scale = 1.0 / p_mark -> fill -> pattern -> scale;
			
			CGAffineTransform t_transform;
			t_transform = CGContextGetCTM(m_context);
			t_transform = CGAffineTransformConcat(CGAffineTransformMakeTranslation(p_mark -> fill -> origin . x, p_mark -> fill -> origin . y), t_transform);
			t_transform = CGAffineTransformConcat(CGAffineTransformMakeScale(t_scale, t_scale), t_transform);
			
			CGPatternRef t_pattern;
			t_pattern = CGPatternCreate(
				t_tile_image,
				CGRectMake(0, 0, CGImageGetWidth(t_tile_image), CGImageGetHeight(t_tile_image)),
				t_transform,
				CGImageGetWidth(t_tile_image), CGImageGetHeight(t_tile_image),
				kCGPatternTilingNoDistortion,
				true,
				&s_pattern_callbacks);
			
			CGColorSpaceRef t_pattern_space;
			t_pattern_space = CGColorSpaceCreatePattern(NULL);
			
			float t_components[1];
			t_components[0] = 1.0f;
			
			if (t_is_stroke)
			{
				CGContextSetStrokeColorSpace(m_context, t_pattern_space);
				CGContextSetStrokePattern(m_context, t_pattern, t_components);
			}
			else
			{
				CGContextSetFillColorSpace(m_context, t_pattern_space);
				CGContextSetFillPattern(m_context, t_pattern, t_components);
			}
				
			CGColorSpaceRelease(t_pattern_space);
			CGPatternRelease(t_pattern);
		}
		else
		{
			float t_red;
			t_red = p_mark -> fill -> colour . red / 65535.0f;
			
			float t_green;
			t_green = p_mark -> fill -> colour . green / 65535.0f;
			
			float t_blue;
			t_blue = p_mark -> fill -> colour . blue / 65535.0f;
			
			if (m_color)
			{
				if (t_is_stroke)
					CGContextSetRGBStrokeColor(m_context, t_red, t_green, t_blue, 1.0f);
				else
					CGContextSetRGBFillColor(m_context, t_red, t_green, t_blue, 1.0f);
			}
			else
			{
				float t_level;
				t_level = GRAYSCALE(t_red, t_green, t_blue);
				
				if (t_is_stroke)
					CGContextSetGrayStrokeColor(m_context, t_level, 1.0f);
				else
					CGContextSetGrayFillColor(m_context, t_level, 1.0f);
			}
		}
	}
	
	switch(p_mark -> type)
	{
		case MARK_TYPE_LINE:
			CGContextMoveToPoint(m_context, p_mark -> line . start . x + 0.5f, p_mark -> line . start . y + 0.5f);
			CGContextAddLineToPoint(m_context, p_mark -> line . end . x + 0.5f, p_mark -> line . end . y + 0.5f);
			CGContextDrawPath(m_context, kCGPathStroke);
		break;
		
		case MARK_TYPE_POLYGON:
			CGContextMoveToPoint(m_context, p_mark -> polygon . vertices[0] . x + 0.5f, p_mark -> polygon . vertices[0] . y + 0.5f);
			for(uint2 i = 1; i < p_mark -> polygon . count; ++i)
				CGContextAddLineToPoint(m_context, p_mark -> polygon . vertices[i] . x + 0.5f, p_mark -> polygon . vertices[i] . y + 0.5f);
			if (!t_is_stroke)
				CGContextClosePath(m_context);
			CGContextDrawPath(m_context, t_is_stroke ? kCGPathStroke : kCGPathFill);	
		break;
		
		case MARK_TYPE_TEXT:
		{
			int2 x;
			x = p_mark -> text . position . x;
			
			int2 y;
			y = p_mark -> text . position . y;
		
			MCFontStruct *f;
			f = p_mark -> text . font;
			
			void *s;
			s = p_mark -> text . data;
			
			uint4 len;
			len = p_mark -> text . length;
			
			if (p_mark -> text . background != NULL)
			{
				CGContextSaveGState(m_context);
				if (m_color)
					CGContextSetRGBFillColor(m_context,
						p_mark -> text . background -> colour . red / 65535.0f,
						p_mark -> text . background -> colour . green / 65535.0f,
						p_mark -> text . background -> colour . blue / 65535.0f,
						1.0f);
				else
					CGContextSetGrayFillColor(m_context,
						GRAYSCALE(p_mark -> text . background -> colour . red / 65535.0f,
							p_mark -> text . background -> colour . green / 65535.0f,
							p_mark -> text . background -> colour . blue / 65535.0f),
						1.0f);

				CGContextFillRect(m_context,
					CGRectMake(x, y - f -> ascent,
						MCscreen -> textwidth(f, (const char *)s, len), f -> ascent + f -> descent));

				CGContextRestoreGState(m_context);
			}
		
			bool t_is_unicode;
			t_is_unicode = p_mark -> text . font -> unicode || p_mark -> text . unicode_override;
							
			MCExecPoint text_ep(NULL, NULL, NULL);
			text_ep . setsvalue(MCString((const char *)s, len));
			if (!t_is_unicode)
			{
				text_ep . nativetoutf16();
				s = (void *)text_ep . getsvalue() . getstring();
				len = text_ep . getsvalue() . getlength();
			}
			
			OSStatus t_err;
			ATSUTextLayout t_layout;
			ATSUStyle t_style;
			
			ATSUFontID t_font_id;
			Fixed t_font_size;
			Boolean t_font_is_bold;
			Boolean t_font_is_italic;
			Boolean t_font_is_underline;
			Boolean t_font_is_condensed;
			Boolean t_font_is_extended;
			ATSLineLayoutOptions t_layout_options;
			
			ATSUAttributeTag t_tags[] =
			{
				kATSUFontTag,
				kATSUSizeTag
			};
			ByteCount t_sizes[] =
			{
				sizeof(ATSUFontID),
				sizeof(Fixed)
			};
			ATSUAttributeValuePtr t_attrs[] =
			{
				&t_font_id,
				&t_font_size
			};
			
			ATSUAttributeTag t_layout_tags[] =
			{
				kATSUCGContextTag,
				kATSULineLayoutOptionsTag,
			};
			ByteCount t_layout_sizes[] =
			{
				sizeof(CGContextRef),
				sizeof(ATSLineLayoutOptions)
			};
			ATSUAttributeValuePtr t_layout_attrs[] =
			{
				&m_context,
				&t_layout_options
			};
			
			UniCharCount t_run = len / 2;
			Rect t_bounds;
			
			FMFontStyle t_intrinsic_style;
			t_err = ATSUFONDtoFontID((short)(intptr_t)f -> fid, f -> style, &t_font_id);

			t_font_size = f -> size << 16;
			t_err = ATSUCreateStyle(&t_style);
			t_err = ATSUSetAttributes(t_style, sizeof(t_tags) / sizeof(ATSUAttributeTag), t_tags, t_sizes, t_attrs);
			t_err = ATSUCreateTextLayout(&t_layout);
			t_err = ATSUSetTransientFontMatching(t_layout, true);
			
			if (t_is_unicode)
			{
				t_layout_options = kATSLineUseDeviceMetrics | kATSLineFractDisable;
			}
			else
				t_layout_options = 0; //kATSLineDisableAllLayoutOperations | kATSLineUseDeviceMetrics | kATSLineFractDisable; //kATSLineUseQDRendering | kATSLineUseDeviceMetrics | kATSLineDisableAllLayoutOperations | kATSLineUseDeviceMetrics | kATSLineFractDisable; /*| kATSLineDisableAutoAdjustDisplayPos | kATSLineUseQDRendering*/;
			
			t_err = ATSUSetLayoutControls(t_layout, sizeof(t_layout_tags) / sizeof(ATSUAttributeTag), t_layout_tags, t_layout_sizes, t_layout_attrs);
			
			CGContextSaveGState(m_context);
			CGContextConcatCTM(m_context, CGAffineTransformMake(1, 0, 0, -1, 0, m_page_height));
			if (t_is_unicode)
			{
				t_err = ATSUSetTextPointerLocation(t_layout, (const UniChar *)s, 0, len / 2, len / 2);
				t_err = ATSUSetRunStyle(t_layout, t_style, 0, len / 2);
				t_err = ATSUSetTransientFontMatching(t_layout, true);
				t_err = ATSUDrawText(t_layout, 0, len / 2, x << 16, (m_page_height - y) << 16);
			}
			else
			{
				int4 t_screen_x;
				t_screen_x = x;
				for(uint4 i = 0; i < p_mark -> text . length; i++)
				{
					t_err = ATSUSetTextPointerLocation(t_layout, (const UniChar *)s + i, 0, 1, 1);
					t_err = ATSUSetRunStyle(t_layout, t_style, 0, 1);
					t_err = ATSUSetTransientFontMatching(t_layout, true);
					t_err = ATSUDrawText(t_layout, 0, 1, t_screen_x << 16, (m_page_height - y) << 16);
					t_screen_x += f -> widths[((uint1 *)p_mark -> text . data)[i]];
				}
			}
			CGContextRestoreGState(m_context);
			
			t_err = ATSUDisposeTextLayout(t_layout);
			t_err = ATSUDisposeStyle(t_style);
		}
		break;
		
		case MARK_TYPE_RECTANGLE:
			if (t_is_stroke)
			{
				CGRect t_rect;
				CGContextAddRect(m_context,
					CGRectMake(p_mark -> rectangle . bounds . x + 0.5f, p_mark -> rectangle . bounds . y + 0.5f,
						p_mark -> rectangle . bounds . width - 1.0f, p_mark -> rectangle . bounds . height - 1.0f));
				CGContextDrawPath(m_context, kCGPathStroke);
			}
			else
			{
				CGRect t_rect;
				CGContextAddRect(m_context,
					CGRectMake(p_mark -> rectangle . bounds . x, p_mark -> rectangle . bounds . y,
						p_mark -> rectangle . bounds . width, p_mark -> rectangle . bounds . height));
				CGContextDrawPath(m_context, kCGPathFill);
			}
		break;
		
		case MARK_TYPE_ROUND_RECTANGLE:
			if (t_is_stroke)
			{
				OSX_CGContextAddRoundedRect(m_context,
					CGRectMake(p_mark -> rectangle . bounds . x + 0.5f, p_mark -> rectangle . bounds . y + 0.5f,
						p_mark -> rectangle . bounds . width - 1.0f, p_mark -> rectangle . bounds . height - 1.0f),
						p_mark -> round_rectangle . radius - 1.0f);
				CGContextDrawPath(m_context, kCGPathStroke);
			}
			else
			{
				OSX_CGContextAddRoundedRect(m_context,
					CGRectMake(p_mark -> rectangle . bounds . x, p_mark -> rectangle . bounds . y,
						p_mark -> rectangle . bounds . width, p_mark -> rectangle . bounds . height),
						p_mark -> round_rectangle . radius);
				CGContextDrawPath(m_context, kCGPathFill);
			}
		break;
		
		case MARK_TYPE_ARC:
			if (t_is_stroke)
			{
				CGRect t_bounds;
				t_bounds = CGRectMake(p_mark -> rectangle . bounds . x + 0.5f, p_mark -> rectangle . bounds . y + 0.5f,
						p_mark -> rectangle . bounds . width - 1.0f, p_mark -> rectangle . bounds . height - 1.0f);
				if (p_mark -> arc . complete)
					OSX_CGContextAddSegment(m_context, t_bounds, p_mark -> arc . start, p_mark -> arc . angle);
				else
					OSX_CGContextAddArc(m_context, t_bounds, p_mark -> arc . start, p_mark -> arc . angle);
				CGContextDrawPath(m_context, kCGPathStroke);
			}
			else
			{
				OSX_CGContextAddSegment(m_context,
					CGRectMake(p_mark -> arc . bounds . x, p_mark -> arc . bounds . y,
						p_mark -> arc . bounds . width, p_mark -> arc . bounds . height),
						p_mark -> arc . start, p_mark -> arc . angle);
				CGContextDrawPath(m_context, kCGPathFill);
			}
		break;
		
		case MARK_TYPE_IMAGE:
		{
			MCImageBitmap *t_src_bitmap = nil;
			
			int32_t t_dst_x, t_dst_y;
			uint32_t t_dst_width, t_dst_height;

			t_dst_x = p_mark->image.dx - p_mark->image.sx;
			t_dst_y = p_mark->image.dy - p_mark->image.sy;
			
			t_dst_width = p_mark->image.descriptor.bitmap->width;
			t_dst_height = p_mark->image.descriptor.bitmap->height;
			
			t_src_bitmap = p_mark->image.descriptor.bitmap;

			CGContextSaveGState(m_context);

			CGContextClipToRect(m_context, CGRectMake(p_mark -> image . dx, p_mark -> image . dy, p_mark -> image . sw, p_mark -> image . sh));
			
			if (p_mark->image.descriptor.has_transform)
			{
				CGAffineTransform t_transform = MCGAffineTransformToCGAffineTransform(p_mark->image.descriptor.transform);
				CGContextTranslateCTM(m_context, t_dst_x, t_dst_y);
				CGContextConcatCTM(m_context, t_transform);
				CGContextTranslateCTM(m_context, -t_dst_x, -t_dst_y);
			}
			
			CGImageRef t_image = nil;
			/* UNCHECKED */ MCImageBitmapToCGImage(t_src_bitmap, false, true, t_image);
			
			CGRect t_dst_rect;
			t_dst_rect = CGRectMake(t_dst_x, t_dst_y, t_dst_width, t_dst_height);
			
			CGContextDrawImage(m_context, t_dst_rect, t_image);
			
			CGContextRestoreGState(m_context);
			
			CGImageRelease(t_image);
		}
		break;
	}
	
	CGContextRestoreGState(m_context);
}

#define SCALE 4

bool MCQuartzMetaContext::begincomposite(const MCRectangle &p_region, MCGContextRef &r_context)
{
	bool t_success = true;
	
	uint4 t_scale = SCALE;
	
	MCGContextRef t_context = nil;
	
	uint32_t t_width = p_region.width * t_scale;
	uint32_t t_height = p_region.height * t_scale;
	
	if (t_success)
		t_success = MCGContextCreate(t_width, t_height, true, t_context);
	
	if (t_success)
	{
		MCGContextScaleCTM(t_context, t_scale, t_scale);
		MCGContextTranslateCTM(t_context, -(MCGFloat)p_region.x, -(MCGFloat)p_region.y);
		
		m_composite_context = t_context;
		m_composite_rect = p_region;
		
		r_context = m_composite_context;
	}
	else
	{
		MCGContextRelease(t_context);
	}
	
	return t_success;
}


void MCQuartzMetaContext::endcomposite(MCRegionRef p_clip_region)
{
	OSX_CGContextClipToRegion(m_context, (RgnHandle)p_clip_region);
	
	MCGImageRef t_image;
	t_image = nil;
	
	/* UNCHECKED */ MCGContextCopyImage(m_composite_context, t_image);
	MCGContextRelease(m_composite_context);
	m_composite_context = nil;
	
	CGImageRef t_cgimage = nil;
	/* UNCHECKED */ MCGImageToCGImage(t_image, MCGRectangleMake(0, 0, MCGImageGetWidth(t_image), MCGImageGetHeight(t_image)), false, true, t_cgimage);
	
	CGContextDrawImage(m_context, CGRectMake(m_composite_rect . x, m_composite_rect . y, m_composite_rect . width, m_composite_rect . height), t_cgimage);
	CGImageRelease(t_cgimage);
	
	MCGImageRelease(t_image);
	MCRegionDestroy(p_clip_region);
}

///////////////////////////////////////////////////////////////////////////////

MCMacOSXPrinterDevice::MCMacOSXPrinterDevice(void)
{
	m_session = NULL;
	m_settings = NULL;
	m_page_format = NULL;

	m_error = NULL;
	m_page_started = false;
}

MCMacOSXPrinterDevice::~MCMacOSXPrinterDevice(void)
{
	delete m_error;
}

const char *MCMacOSXPrinterDevice::Error(void) const
{
	return m_error;
}

MCPrinterResult MCMacOSXPrinterDevice::Start(PMPrintSession p_session, PMPrintSettings p_settings, PMPageFormat p_page_format, bool p_color)
{
	OSErr t_err;
	t_err = OSX_PMSessionBeginCGDocument(p_session, p_settings, p_page_format);
	if (t_err != noErr)
		return HandleError(t_err, "unable to begin document");

	m_session = p_session;
	m_settings = p_settings;
	m_page_format = p_page_format;
	m_color = p_color;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCMacOSXPrinterDevice::Finish(void)
{
	if (m_session == NULL)
		return PRINTER_RESULT_SUCCESS;

	if (m_page_started)
	{
		OSErr t_err;
		t_err = PMSessionEndPage(m_session);
		if (t_err != noErr)
			return HandleError(t_err, "unable to end page");
	}

	OSErr t_err;
	t_err = PMSessionEndDocument(m_session);
	if (t_err != noErr)
		return HandleError(t_err, "unable to end document");

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCMacOSXPrinterDevice::Cancel(void)
{
	if (m_session == NULL)
		return PRINTER_RESULT_SUCCESS;

	return HandleError(kPMCancel, NULL);
}

MCPrinterResult MCMacOSXPrinterDevice::Show(void)
{
	if (m_session == NULL)
		return PRINTER_RESULT_SUCCESS;

	if (!m_page_started)
	{
		OSErr t_err;
		t_err = PMSessionBeginPage(m_session, m_page_format, NULL);
		if (t_err != noErr)
			return HandleError(t_err, "unable to begin page");
	}

	OSErr t_err;
	t_err = PMSessionEndPage(m_session);
	if (t_err != noErr)
		return HandleError(t_err, "unable to end page");

	m_page_started = false;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCMacOSXPrinterDevice::Begin(const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, MCContext*& r_context)
{
	if (m_session == NULL)
		return PRINTER_RESULT_SUCCESS;

	if (!m_page_started)
	{
		OSErr t_err;
		t_err = PMSessionBeginPage(m_session, m_page_format, NULL);
		if (t_err != noErr)
			return HandleError(t_err, "unable to begin page");

		m_page_started = true;
	}

	// Calculate the convex integer hull of the source rectangle.
	//
	// MW-2008-03-18: [[ Bug 5940 ]] Made sure the left and top are the right way round
	//   otherwise strangeness results!
	MCRectangle t_src_rect_hull;
	t_src_rect_hull . x = (int4)floor(p_src_rect . left);
	t_src_rect_hull . y = (int4)floor(p_src_rect . top);
	t_src_rect_hull . width = (int4)(ceil(p_src_rect . right) - floor(p_src_rect . left));
	t_src_rect_hull . height = (int4)(ceil(p_src_rect . bottom) - floor(p_src_rect . top));

	// Create a QD-based meta context targetting the previously computed
	// rectangle
	//
	PMRect t_paper_rect;
	PMGetAdjustedPaperRect(m_page_format, &t_paper_rect);
	
	int t_page_width, t_page_height;
	t_page_width = ceil(t_paper_rect . right - t_paper_rect . left);
	t_page_height = ceil(t_paper_rect . bottom - t_paper_rect . top);
	
	MCQuartzMetaContext *t_context;
	t_context = new MCQuartzMetaContext(t_src_rect_hull, t_page_width, t_page_height);
	r_context = t_context;

	m_src_rect = p_src_rect;
	m_dst_rect = p_dst_rect;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCMacOSXPrinterDevice::End(MCContext *p_raw_context)
{
	if (m_session == NULL)
		return PRINTER_RESULT_SUCCESS;

	MCQuartzMetaContext *t_context;
	t_context = static_cast<MCQuartzMetaContext *>(p_raw_context);
	t_context -> render(m_session, m_src_rect, m_dst_rect, m_color);

	delete t_context;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCMacOSXPrinterDevice::Anchor(const char *name, double x, double y)
{
	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCMacOSXPrinterDevice::Link(const char *name, const MCPrinterRectangle& area, MCPrinterLinkType type)
{
	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCMacOSXPrinterDevice::Bookmark(const char *title, double x, double y, int depth, bool closed)
{
	return PRINTER_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

MCPrinterResult MCMacOSXPrinterDevice::HandleError(OSErr p_error, const char *p_message)
{
	if (m_session != NULL)
	{
		if (p_error == kPMCancel)
			PMSessionSetError(m_session, p_error);

		PMSessionEndDocument(m_session);

		m_session = NULL;
		m_page_format = NULL;
		m_settings = NULL;
	}

	if (p_message != NULL)
	{
		delete m_error;
		m_error = strdup(p_message);
	}

	return p_error == kPMCancel ? PRINTER_RESULT_CANCEL : PRINTER_RESULT_ERROR;
}

///////////////////////////////////////////////////////////////////////////////

