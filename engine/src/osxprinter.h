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

#ifndef __MC_OSXPRINTER__
#define __MC_OSXPRINTER__

#ifndef __MC_PRINTER__
#include "printer.h"
#endif

class MCMacOSXPrinterDevice: public MCPrinterDevice
{
public:
	MCMacOSXPrinterDevice(void);
	~MCMacOSXPrinterDevice(void);

	MCPrinterResult Start(PMPrintSession p_session, PMPrintSettings p_settings, PMPageFormat p_page_format, bool p_color);
	MCPrinterResult Finish(void);

	const char *Error(void) const;

	MCPrinterResult Cancel(void);

	MCPrinterResult Show(void);

	MCPrinterResult Begin(const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, MCContext*& r_context);
	MCPrinterResult End(MCContext *p_context);

	MCPrinterResult Anchor(const char *name, double x, double y);
	MCPrinterResult Link(const char *dest, const MCPrinterRectangle& area, MCPrinterLinkType type);
	MCPrinterResult Bookmark(const char *title, double x, double y, int depth, bool closed);

private:
	MCPrinterResult BeginPage(void);

	MCPrinterResult HandleError(OSStatus p_error, const char *p_message);

	PMPrintSession m_session;
	PMPrintSettings m_settings;
	PMPageFormat m_page_format;
	char *m_error;
	bool m_page_started;
	bool m_color;

	MCPrinterRectangle m_src_rect, m_dst_rect;
};

class MCMacOSXPrinter: public MCPrinter
{
public:

protected:
	void DoInitialize(void);
	void DoFinalize(void);

	bool DoReset(MCStringRef p_name);
	bool DoResetSettings(MCDataRef p_settings);

	void DoFetchSettings(void*& r_buffer, uint4& r_length);
	const char *DoFetchName(void);

	void DoResync(void);

	MCPrinterDialogResult DoPrinterSetup(bool p_window_modal, Window p_owner);
	MCPrinterDialogResult DoPageSetup(bool p_window_modal, Window p_owner);

	MCPrinterResult DoBeginPrint(MCStringRef p_document, MCPrinterDevice*& r_device);
	MCPrinterResult DoEndPrint(MCPrinterDevice* p_device);

private:
	bool Reset(MCStringRef p_name, PMPrintSettings p_settings, PMPageFormat p_page_format);

	void ResetSession(void);

	void GetProperties(bool p_include_output = false);
	void SetProperties(bool p_include_output = false);

	// This call updates the device rect and device features props.
	void SetDerivedProperties(void);
	
	MCPrinterDialogResult DoDialog(bool p_window_modal, Window p_owner, bool p_is_settings);

	//

	bool m_valid;
	char *m_printer_name;
	PMPrinter m_printer;
	PMPaper m_paper;
	PMPrintSettings m_settings;
	PMPageFormat m_page_format;
	PMPrintSession m_session;
};

#endif
