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

#ifndef __PSPRINTER_H__
#define __PSPRINTER_H__

// This structure will hold all our printer settings.
struct PSPrinterSettings
{
  	char * printername ;
	
	uint4 copies ;
	bool collate ;
	
	MCPrinterOrientation orientation ;
	
	MCPrinterDuplexMode duplex_mode ;
	
	uint4 paper_size_width ;
	uint4 paper_size_height ;
	
	MCPrinterOutputType printertype ;
	char * outputfilename ;
	MCInterval * page_ranges ;
	int4	page_range_count ;
	
};

class MCPSPrinter : public MCPrinter
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

	MCPrinterResult DoBeginPrint(MCStringRef p_document_name, MCPrinterDevice*& r_device);
	MCPrinterResult DoEndPrint(MCPrinterDevice* p_device);
	
private:
	PSPrinterSettings m_printersettings;
    MCCustomPrinter *m_pdf_printer;
    
	// This flushes the settings from m_printersettings into the Object's knowledge of them
	void FlushSettings(void);
	void SyncSettings(void);
};



#endif
