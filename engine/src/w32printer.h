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

#ifndef __MC_WIN32PRINTER__
#define __MC_WIN32PRINTER__

#ifndef __MC_PRINTER__
#include "printer.h"
#endif

class MCWindowsPrinterDevice: public MCPrinterDevice
{
public:
	MCWindowsPrinterDevice(void);
	~MCWindowsPrinterDevice(void);

	//

	MCPrinterResult Start(HDC p_dc, const wchar_t *p_document_name, const wchar_t *p_output_file);
	MCPrinterResult Finish(void);

	//

	const char *Error(void) const;

	MCPrinterResult Cancel(void);

	MCPrinterResult Show(void);

	MCPrinterResult Begin(const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, MCContext*& r_context);
	MCPrinterResult End(MCContext *p_context);

	MCPrinterResult Anchor(const char *name, double x, double y);
	MCPrinterResult Link(const char *dest, const MCPrinterRectangle& area, MCPrinterLinkType type);
	MCPrinterResult Bookmark(const char *title, double x, double y, int depth, bool closed);

private:
	void Default(void);
	void SetError(const char *p_message);

	HDC m_dc;
	char *m_error;
	int m_page_started;

	MCPrinterRectangle m_src_rect, m_dst_rect;
};

class MCWindowsPrinter: public MCPrinter
{
public:
	HDC GetDC(bool p_synchronize = false);

	MCWindowsPrinter()
	{
		m_name = MCValueRetain(kMCEmptyString);
	}

	~MCWindowsPrinter()
	{
		MCValueRelease(m_name);
	}

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
	void Synchronize(void);
	void Reset(MCStringRef p_name, DEVMODEW *p_devmode);
	
	bool FetchDialogData(HGLOBAL& r_devmode_handle, HGLOBAL& r_devnames_handle);
	void StoreDialogData(HGLOBAL p_devmode_handle, HGLOBAL p_devnames_handle);

	bool DecodeSettings(MCDataRef p_settings, MCStringRef &r_name, DEVMODEW* &r_devmode);
	void EncodeSettings(MCStringRef p_name, DEVMODEW* p_devmode, MCDataRef &r_buffer);

	HDC LockDC(void);
	void UnlockDC(void);
	void ChangeDC(void);

	bool m_valid;
	MCStringRef m_name;
	DEVMODEW *m_devmode;

	HDC m_dc;
	bool m_dc_locked;
	bool m_dc_changed;
};

#endif
