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

#ifndef __MC_PRINTER__
#define __MC_PRINTER__

#include "parsedef.h"
#include "util.h"

////////////////////////////////////////////////////////////////////////////////

enum MCPrinterDialogResult
{
	PRINTER_DIALOG_RESULT_OKAY,
	PRINTER_DIALOG_RESULT_CANCEL,
	PRINTER_DIALOG_RESULT_ERROR
};

struct MCPrinterRectangle
{
	float64_t left;
	float64_t top;
	float64_t right;
	float64_t bottom;
};

enum MCPrinterResult
{
	PRINTER_RESULT_SUCCESS, ///< The operation succeeded
	PRINTER_RESULT_FAILURE, ///< The operation failured due to an avoidable problem (programmer error)
	PRINTER_RESULT_CANCEL, 	///< A cancellation request occurred during the requested (or since the last) operation
	PRINTER_RESULT_ERROR 	///< A printer error occurred during the requested (or since the last) operation
};

enum MCPrinterLinkType
{
	kMCPrinterLinkUnspecified = 0,
	kMCPrinterLinkAnchor = 1,
	kMCPrinterLinkURI = 2,
};

class MCPrinterDevice
{
public:
	virtual ~MCPrinterDevice(void) {};

	// Return a descriptive string of the last printer error that occurred. This
	// call should return NULL if ERROR has not been returned by any method of
	// the object.
	//
	// The pointer returned remains the property of the object, and only needs
	// to be valid up until the next time a method on the object is called.
	//
	virtual const char *Error(void) const = 0;

	// Cancel the current document.
	//
	// If an error has occurred with printing, ERROR should be returned, else
	// CANCEL should be returned.
	//
	virtual MCPrinterResult Cancel(void) = 0;

	// Show the current page, sending it to the printer.
	//
	// If this call succeeds, SUCCESS should be returned.
	// If the job has been cancelled, CANCEL should be returned.
	// If a printer error has occurred, ERROR should be returned.
	//
	virtual MCPrinterResult Show(void) = 0;

	// Return a graphics context using p_src_rect as the source co-ordinate
	// system.
	// The p_dst_rect uses points, and the source co-ordinate is mapped into
	// it.
	//
	// If the call succeeds, SUCCESS should be returned and r_context will be
	// a valid graphics context.
	// If the job has been cancelld, CANCEL should be returned.
	// If a printer error has occurred, ERROR should be returned.
	//
	virtual MCPrinterResult Begin(const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, MCContext*& r_context) = 0;

	// Finish using the given graphics context for rendering to the printer.
	// After this call, the graphics context ceases to be valid.
	//
	// If the call succeeds, SUCCESS should be returned.
	// If the job has been cancelled, CANCEL should be returned.
	// If a printer error has occurred, ERROR should be returned.
	//
	virtual MCPrinterResult End(MCContext *p_context) = 0;

	// Make a an anchor with the given name on the page. The location coords
	// are in points. If the device does not support anchors, this should be
	// a no-op.
	//
	// If the call succeeds, SUCCESS should be returned.
	// If the job has been cancelled, CANCEL should be returned.
	// If a printer error has occurred, ERROR should be returned.
	//
	virtual MCPrinterResult Anchor(const char *name, double x, double y) = 0;

	// Make a link to the given named anchor or URI. The location coords are
	// in points. If the device does not support anchors, this should be
	// a no-op.
	//
	// If the call succeeds, SUCCESS should be returned.
	// If the job has been cancelled, CANCEL should be returned.
	// If a printer error has occurred, ERROR should be returned.
	virtual MCPrinterResult Link(const char *destination, const MCPrinterRectangle& area, MCPrinterLinkType type) = 0;

	// Make a bookmark with the given title, nested at the given depth.
	// The location coords are in points.  If the device does not support
	// bookmarks, this should be a no-op.
	//
	// If the call succeeds, SUCCESS should be returned.
	// If the job has been cancelled, CANCEL should be returned.
	// If a printer error has occurred, ERROR should be returned.
	//
	virtual MCPrinterResult Bookmark(const char *title, double x, double y, int depth, bool closed) = 0;
};

////////////////////////////////////////////////////////////////////////////////

enum MCPrinterOrientation
{
	PRINTER_ORIENTATION_PORTRAIT,
	PRINTER_ORIENTATION_REVERSE_PORTRAIT,
	PRINTER_ORIENTATION_LANDSCAPE,
	PRINTER_ORIENTATION_REVERSE_LANDSCAPE
};

enum MCPrinterDuplexMode
{
	PRINTER_DUPLEX_MODE_SIMPLEX,
	PRINTER_DUPLEX_MODE_SHORT_EDGE,
	PRINTER_DUPLEX_MODE_LONG_EDGE
};

enum MCPrinterOutputType
{
	PRINTER_OUTPUT_PREVIEW,
	PRINTER_OUTPUT_DEVICE,
	PRINTER_OUTPUT_FILE,
	PRINTER_OUTPUT_WORKFLOW,
	PRINTER_OUTPUT_SYSTEM
};

typedef uint4 MCPrinterFeatureSet;
enum
{
	PRINTER_FEATURE_COLLATE_BIT = 0,
	PRINTER_FEATURE_COLLATE = 1 << PRINTER_FEATURE_COLLATE_BIT,
	PRINTER_FEATURE_COPIES_BIT = 1,
	PRINTER_FEATURE_COPIES = 1 << PRINTER_FEATURE_COPIES_BIT,
	PRINTER_FEATURE_COLOR_BIT = 2,
	PRINTER_FEATURE_COLOR = 1 << PRINTER_FEATURE_COLOR_BIT,
	PRINTER_FEATURE_DUPLEX_BIT = 3,
	PRINTER_FEATURE_DUPLEX = 1 << PRINTER_FEATURE_DUPLEX_BIT
};

typedef int4 MCPrinterPageRangeCount;
enum
{
	PRINTER_PAGE_RANGE_SELECTION = -2,
	PRINTER_PAGE_RANGE_CURRENT = -1,
	PRINTER_PAGE_RANGE_ALL = 0
};

#define PRINTER_DEFAULT_PAGE_WIDTH 612
#define	PRINTER_DEFAULT_PAGE_HEIGHT 792
#define PRINTER_DEFAULT_PAGE_ORIENTATION PRINTER_ORIENTATION_PORTRAIT
#define PRINTER_DEFAULT_PAGE_SCALE 1.0f

#define PRINTER_DEFAULT_JOB_COPIES 1
#define PRINTER_DEFAULT_JOB_COLLATE false
#define PRINTER_DEFAULT_JOB_DUPLEX PRINTER_DUPLEX_MODE_SIMPLEX
#define PRINTER_DEFAULT_JOB_COLOR true

// The MCPrinter object is the interface to the printing subsystem.
//
// It manages all the current print properties, print layout and selection
// of the appropriate printing backend, provided by the screen class.
//
class MCPrinter
{
public:
	MCPrinter(void);
	virtual ~MCPrinter(void);

	void Initialize(void);
	void Finalize(void);

	// Printing loop methods
	void Open(bool p_cancelled = false);
	void Close(void);
	void Cancel(void);
	void Reset(void);
	void Break(void);
	
	void LayoutStack(MCStack *p_stack, bool p_marked, const MCRectangle* p_src_rect);
	void LayoutCardSequence(MCStack *p_stack, uint32_t p_number_cards, const MCRectangle *p_src_rect);
	void LayoutCard(MCCard *p_card, const MCRectangle *p_rect);

	void MakeAnchor(MCStringRef name, int2 x, int2 y);
	void MakeLink(MCStringRef destination, const MCRectangle& area, MCPrinterLinkType type);
	void MakeBookmark(MCStringRef title, int2 x, int2 y, uint32_t level, bool closed);

	void Render(MCCard *p_card, const MCRectangle& p_src, const MCRectangle& p_dst);
	
	bool ChoosePrinter(bool p_window_modal, MCStringRef &r_result);
	bool ChoosePaper(bool p_window_modal, MCStringRef &r_result);

	// Device configuration properties
	//
	void SetDeviceName(MCStringRef p_name);
	const char *GetDeviceName(void);

	void SetDeviceSettings(MCDataRef p_settings);
	bool CopyDeviceSettings(MCDataRef &r_settings);

	void SetDeviceOutput(MCPrinterOutputType p_type, MCStringRef p_location);
	MCPrinterOutputType GetDeviceOutputType(void) const;
	const char *GetDeviceOutputLocation(void) const;
	
	void SetDeviceCommand(MCStringRef p_command);
	const char *GetDeviceCommand(void) const;
	
	void SetDeviceFontTable(MCStringRef p_font_table);
	const char *GetDeviceFontTable(void) const;
	
	void SetDeviceFeatures(MCPrinterFeatureSet p_features);
	MCPrinterFeatureSet GetDeviceFeatures(void);

	void SetDeviceRectangle(const MCRectangle& p_rectangle);
	MCRectangle GetDeviceRectangle(void);

	// Page configuration properties
	//
	void SetPageSize(int32_t p_width, int32_t p_height);
	int32_t GetPageWidth(void) const;
	int32_t GetPageHeight(void) const;
	
	void SetPageOrientation(MCPrinterOrientation p_orientation);
	MCPrinterOrientation GetPageOrientation(void) const;
	
	void SetPageMargins(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
	int32_t GetPageLeftMargin(void) const;
	int32_t GetPageTopMargin(void) const;
	int32_t GetPageRightMargin(void) const;
	int32_t GetPageBottomMargin(void) const;

	void SetPageScale(float64_t p_scale);
	float64_t GetPageScale(void) const;

	MCRectangle GetPageRectangle(void) const;

	// Job configuration properties
	//
	void SetJobCopies(uint32_t p_copies);
	uint32_t GetJobCopies(void) const;
	
	void SetJobCollate(bool p_collate);
	bool GetJobCollate(void) const;
	
	void SetJobName(MCStringRef(p_name));
	const char *GetJobName(void) const;
	
	void SetJobDuplex(MCPrinterDuplexMode p_mode);
	MCPrinterDuplexMode GetJobDuplex(void) const;
	
	void SetJobColor(bool p_color);
	bool GetJobColor(void) const;

	void SetJobRanges(MCPrinterPageRangeCount p_count_type, const MCInterval *p_ranges);
	MCPrinterPageRangeCount GetJobRangeCount(void) const;
	const MCInterval* GetJobRanges(void) const;

	int GetJobPageNumber(void) const;

	// Layout configuration properties
	void SetLayoutShowBorders(bool p_show_borders);
	bool GetLayoutShowBorders(void) const;
	
	void SetLayoutSpacing(int32_t p_row, int32_t p_column);
	int32_t GetLayoutRowSpacing(void) const;
	int32_t GetLayoutColumnSpacing(void) const;
	
	void SetLayoutRowsFirst(bool p_rows_first);
	bool GetLayoutRowsFirst(void) const;
	
	void SetLayoutScale(float64_t p_scale);
	float64_t GetLayoutScale(void) const;

protected:
	// Initialize the platform-specific part of the object.
	//
	// This call should clear the state of the system printer and
	// update the printer properties to reflect those of the user's
	// default printer.
	//
	virtual void DoInitialize(void) = 0;

	// Finalize the platform-specific part of the object.
	//
	virtual void DoFinalize(void) = 0;

	// Reset the printer properties to the defaults for the printer named
	// <p_name>.
	// If <p_name> is NULL, reset the printer properties to the defaults
	// for the default system printer.
	// If the printer is unknown or there is no default printer return false
	//
	virtual bool DoReset(MCStringRef p_name) = 0;

	// Reset the printer properties to those held in the p_settings
	// string.
	// If the printer is unknown, return false.
	//
	virtual bool DoResetSettings(MCDataRef p_settings) = 0;

	// Return the name of the currently selected printer - this should
	// be stored from a previous call to DoReset/DoResetSettings.
	//
	virtual const char *DoFetchName(void) = 0;

	// Return the printer settings string - this should be computed on
	// demand.
	//
	virtual void DoFetchSettings(void*& r_buffer, uint4& r_length) = 0;

	// Ensure that the device rectangle and features properties are correct.
	//
	virtual void DoResync(void) = 0;

	// Setup and display a printer setup dialogue. If cancel was not 
	// chosen and no error was returned, the printer state should
	// be updated to reflect the user's choice.
	//
	virtual MCPrinterDialogResult DoPrinterSetup(bool p_window_modal, Window p_owner) = 0;

	// Setup and display a page setup dialogue. If cancel was not 
	// chosen and no error was returned, the printer state should
	// be updated to reflect the user's choice.
	//
	virtual MCPrinterDialogResult DoPageSetup(bool p_window_modal, Window p_owner) = 0;

	// Start a print job with the given name. The returned device structure will
	// be used for the actual rendering.
	//
	// The return value should reflect the starting state of the device object
	// note that a valid device object must be returned regardless of whether
	// a printer error, or cancellation occurred.
	//
	virtual MCPrinterResult DoBeginPrint(MCStringRef p_document, MCPrinterDevice*& r_device) = 0;

	// End the current print job. This call will be made regardless of the error
	// or cancellation state of the device object.
	//
	// The return value should reflect the ending state of the device object
	// if it had not previously returned a CANCEL or ERROR state.
	//
	virtual MCPrinterResult DoEndPrint(MCPrinterDevice* p_device) = 0;

	// This is true if any of the properties affecting the system
	// device have changed. It is used to recaclulate printRect
	// as needed.
	bool m_resync;
private:
	enum
	{
		STATUS_READY,
		STATUS_CANCELLED,
		STATUS_ERROR
	};

	void SetStatus(uint32_t p_status, MCStringRef p_error = NULL);
	void SetStatusFromResult(MCPrinterResult p_result);
	void SetResult(void);

	// Layout methods

	void DoPageBreak(void);
	void DoLayout(MCCard *p_first_card, uint32_t p_number_of_cards, const MCRectangle& p_src_rect, bool p_marked);
	void DoPrint(MCCard *p_card, const MCRectangle& p_src, const MCRectangle& p_dst);
	void DoMakeAnchor(MCStringRef name, int2 x, int2 y);
	void DoMakeLink(MCStringRef destination, const MCRectangle& area, MCPrinterLinkType type);
	void DoMakeBookmark(MCStringRef title, int2 x, int2 y, uint32_t level, bool closed);
	
	void ResetLayout(void);
	bool CalculateLayout(const MCRectangle& p_src, MCRectangle& p_dst);
	void UpdateLayout(const MCRectangle& p_rect);

	// Property instance variables
	
	MCPrinterOutputType m_device_output_type;
	char *m_device_output_location;
	char *m_device_command;
	char *m_device_font_table;
	MCRectangle m_device_rectangle;
	MCPrinterFeatureSet m_device_features;
	
	int32_t m_page_width;
	int32_t m_page_height;
	MCPrinterOrientation m_page_orientation;
	int32_t m_page_left_margin;
	int32_t m_page_top_margin;
	int32_t m_page_right_margin;
	int32_t m_page_bottom_margin;
	float64_t m_page_scale;
	
	uint32_t m_job_copies;
	bool m_job_collate;
	char *m_job_name;
	MCPrinterDuplexMode m_job_duplex;
	bool m_job_color;
	MCPrinterPageRangeCount m_job_range_count;
	MCInterval *m_job_ranges;
	
	bool m_layout_show_borders;
	int32_t m_layout_row_spacing;
	int32_t m_layout_column_spacing;
	float64_t m_layout_scale;
	bool m_layout_rows_first;
	
	// Printing loop instance variables
	
	uint32_t m_loop_nesting;
	int32_t m_loop_layout_x;
	int32_t m_loop_layout_y;
	int32_t m_loop_layout_delta;
	char *m_loop_error;
	uint32_t m_loop_status;
	int32_t m_loop_page;
	
	// Printer device instance variables
	
	MCPrinterDevice *m_device;
};

inline MCPrinterOutputType MCPrinter::GetDeviceOutputType(void) const
{
	return m_device_output_type;
}

inline const char *MCPrinter::GetDeviceOutputLocation(void) const
{
	return m_device_output_location;
}
	
inline const char *MCPrinter::GetDeviceCommand(void) const
{
	return m_device_command;
}
	
inline const char *MCPrinter::GetDeviceFontTable(void) const
{
	return m_device_font_table;
}

inline int32_t MCPrinter::GetPageWidth(void) const
{
	return m_page_width;
}

inline int32_t MCPrinter::GetPageHeight(void) const
{
	return m_page_height;
}
	
inline MCPrinterOrientation MCPrinter::GetPageOrientation(void) const
{
	return m_page_orientation;
}

inline int32_t MCPrinter::GetPageLeftMargin(void) const
{
	return m_page_left_margin;
}

inline int32_t MCPrinter::GetPageTopMargin(void) const
{
	return m_page_top_margin;
}

inline int32_t MCPrinter::GetPageRightMargin(void) const
{
	return m_page_right_margin;
}

inline int32_t MCPrinter::GetPageBottomMargin(void) const
{
	return m_page_bottom_margin;
}

inline float64_t MCPrinter::GetPageScale(void) const
{
	return m_page_scale;
}
	
inline uint32_t MCPrinter::GetJobCopies(void) const
{
	return m_job_copies;
}
	
inline bool MCPrinter::GetJobCollate(void) const
{
	return m_job_collate;
}

inline const char *MCPrinter::GetJobName(void) const
{
	return m_job_name;
}
	
inline MCPrinterDuplexMode MCPrinter::GetJobDuplex(void) const
{
	return m_job_duplex;
}

inline bool MCPrinter::GetJobColor(void) const
{
	return m_job_color;
}

inline MCPrinterPageRangeCount MCPrinter::GetJobRangeCount(void) const
{
	return m_job_range_count;
}

inline const MCInterval* MCPrinter::GetJobRanges(void) const
{
	return m_job_ranges;
}

inline int MCPrinter::GetJobPageNumber(void) const
{
	return m_loop_nesting > 0 ? m_loop_page : -1;
}

inline bool MCPrinter::GetLayoutShowBorders(void) const
{
	return m_layout_show_borders;
}

inline int32_t MCPrinter::GetLayoutRowSpacing(void) const
{
	return m_layout_row_spacing;
}

inline int32_t MCPrinter::GetLayoutColumnSpacing(void) const
{
	return m_layout_column_spacing;
}

inline bool MCPrinter::GetLayoutRowsFirst(void) const
{
	return m_layout_rows_first;
}

inline float64_t MCPrinter::GetLayoutScale(void) const
{
	return m_layout_scale;
}

////////////////////////////////////////////////////////////////////////////////

class MCCustomPrintingDevice;

class MCCustomPrinter: public MCPrinter
{
public:
	MCCustomPrinter(MCStringRef p_name, MCCustomPrintingDevice *p_device);
	~MCCustomPrinter(void);
    
	void SetDeviceOptions(MCArrayRef p_options);
    
    // We promote these to public in the custom printer so that we can aggregate
    // a normal printer around a custom printer (i.e. on Linux).
	MCPrinterResult DoBeginPrint(MCStringRef p_document, MCPrinterDevice*& r_device);
	MCPrinterResult DoEndPrint(MCPrinterDevice* p_device);
    
protected:
	void DoInitialize(void);
	void DoFinalize(void);
    
	bool DoReset(MCStringRef p_name);
	bool DoResetSettings(MCDataRef p_settings);
    
	const char *DoFetchName(void);
	void DoFetchSettings(void*& r_bufer, uint4& r_length);
    
	void DoResync(void);
    
	MCPrinterDialogResult DoPrinterSetup(bool p_window_modal, Window p_owner);
	MCPrinterDialogResult DoPageSetup(bool p_window_modal, Window p_owner);

private:
	MCStringRef m_device_name;
	MCCustomPrintingDevice *m_device;
	MCArrayRef m_device_options;
};

bool MCCustomPrinterCreate(MCStringRef p_destination, MCStringRef p_filename, MCArrayRef p_options, MCCustomPrinter*& r_printer);

////////////////////////////////////////////////////////////////////////////////

#endif
