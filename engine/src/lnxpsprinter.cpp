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

#include "lnxprefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "mcstring.h"
#include "globals.h"
#include "mctheme.h"
#include "util.h"
#include "object.h"
#include "context.h"
#include "globals.h"

#include "stack.h"
#include "region.h"
#include "osspec.h"
#include "variable.h"
 
#include "card.h"
#include "mcerror.h"
#include "util.h"
#include "font.h"

#include "metacontext.h"

#include "lnxflst.h"
#include "printer.h"
#include "lnxpsprinter.h"
#include "lnxdc.h"

#include "lnxans.h"

#include "graphicscontext.h"

#define C_FNAME "/tmp/tmpprintfile.ps"

///////////////////////////////////////////////////////////////////////////////////////////////
//
//                                P S P R I N T E R  
//
//////////////////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_PRINTER_SCRIPT "return the last word of shell(\"lpstat -d\")"

char *getdefaultprinter(void)
{
    MCAutoValueRef t_value;
    MCExecContext ctxt(nil, nil, nil);
    MCAutoStringRef t_string;
    char *t_cstring;

    MCtemplatestack->domess(MCSTR(DEFAULT_PRINTER_SCRIPT));

    /* UNCHECKED */ MCresult->eval(ctxt, &t_value);
    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
    /* UNCHECKED */  MCStringConvertToCString(*t_string, t_cstring);
    
    return t_cstring;
}

void MCPSPrinter::DoInitialize(void)
{
    // Set up our settings buffer to some defaults ...
	m_printersettings . orientation = PRINTER_DEFAULT_PAGE_ORIENTATION ;
	m_printersettings . copies = PRINTER_DEFAULT_JOB_COPIES ;
	m_printersettings . paper_size_height = PRINTER_DEFAULT_PAGE_HEIGHT ;
	m_printersettings . paper_size_width  = PRINTER_DEFAULT_PAGE_WIDTH ;
	m_printersettings . collate = false ;
	m_printersettings . duplex_mode = PRINTER_DEFAULT_JOB_DUPLEX ;
	
	m_printersettings . printername = getdefaultprinter() ;
	m_printersettings . outputfilename = NULL ;

	m_printersettings . page_ranges = NULL ;
	m_printersettings . page_range_count = PRINTER_PAGE_RANGE_ALL ;
    
    // We only ever have a (wrapped) PDF Printer temporarily.
    m_pdf_printer = nil;
}

void MCPSPrinter::DoFinalize(void)
{
    delete m_pdf_printer;

    /* Allocated by MCStringConvertToCString() */
	if ( m_printersettings . printername != NULL )
		MCMemoryDeleteArray(m_printersettings . printername);

	if ( m_printersettings . outputfilename != NULL ) 
		delete (m_printersettings . outputfilename -7);	// Need to subtract 7 here as we added 7 to skip the "file://" part
	
	if ( m_printersettings . page_ranges != NULL)
		delete m_printersettings . page_ranges ;
}


bool MCPSPrinter::DoReset(MCStringRef p_name)
{
    if (!MCStringIsEmpty(p_name))
    {
        MCAutoStringRefAsSysString t_name;
        /* UNCHECKED */ t_name.Lock(p_name);
        m_printersettings . printername = strdup(*t_name);
    }
	
	FlushSettings();
	
	// MDW-2013-04-16: [[ x64 ]] DoReset needs to return a bool
	return true;
}


bool MCPSPrinter::DoResetSettings(MCDataRef p_settings)
{
	bool t_success;
	t_success = true;

	MCDictionary t_dictionary;
	if (t_success)	
		t_success = t_dictionary . Unpickle(MCDataGetBytePtr(p_settings), MCDataGetLength(p_settings));

	MCString t_name;
	if (t_success)
		t_success = t_dictionary . Get('NMEA', t_name);
		
	if ( t_success ) 
			m_printersettings . printername = t_name.clone() ;
 
	return t_success;
	
}

void MCPSPrinter::DoFetchSettings(void*& r_buffer, uint4& r_length)
{
	MCDictionary t_dictionary;
	
	if ( m_printersettings . printername != NULL ) 
		t_dictionary . Set('NMEA', MCString(m_printersettings . printername , strlen(m_printersettings . printername ) + 1 ) );

	t_dictionary . Pickle(r_buffer, r_length);
}

const char *MCPSPrinter::DoFetchName(void)
{
	return m_printersettings . printername;
}

void MCPSPrinter::DoResync(void)
{
}

MCPrinterDialogResult MCPSPrinter::DoPrinterSetup(bool p_window_modal, Window p_owner)
{
	MCPrinterDialogResult ret ; 
	ret = MCA_gtk_printer_setup( m_printersettings ) ;
	FlushSettings () ;
		
	return ( ret );
}


MCPrinterDialogResult MCPSPrinter::DoPageSetup(bool p_window_modal, Window p_owner)
{
	MCPrinterDialogResult ret ; 

	ret = MCA_gtk_page_setup( m_printersettings ) ;
	FlushSettings () ;
	
	return ( ret ) ;
}

void MCPSPrinter::FlushSettings ( void ) 
{
	SetPageSize ( m_printersettings . paper_size_width, m_printersettings . paper_size_height ) ;
	SetPageOrientation ( m_printersettings . orientation ) ;
	
	SetJobDuplex ( m_printersettings . duplex_mode ) ;
	SetJobCopies ( m_printersettings . copies ) ;
	SetJobCollate ( m_printersettings . collate ) ;
	
    if ( m_printersettings . outputfilename != NULL )
    {
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCStringCreateWithSysString(m_printersettings.outputfilename, &t_string);
        SetDeviceOutput( m_printersettings . printertype, *t_string);
    }
	
	if ( m_printersettings . page_range_count > 0 )
		SetJobRanges ( m_printersettings . page_range_count, m_printersettings . page_ranges ) ;
	
	MCRectangle aRect ;
	aRect . x = 0 ;
	aRect . y = 0 ;
	bool t_rotated = (( m_printersettings . orientation == PRINTER_ORIENTATION_PORTRAIT ) || ( m_printersettings . orientation == PRINTER_ORIENTATION_REVERSE_PORTRAIT ) ) ;
	aRect . width =  (!t_rotated ) ? m_printersettings . paper_size_width : m_printersettings . paper_size_height ;
	aRect . height = ( t_rotated ) ? m_printersettings . paper_size_width : m_printersettings . paper_size_height ;
	SetDeviceRectangle ( aRect ) ;
	
}

void MCPSPrinter::SyncSettings (void)
{
	
	m_printersettings . orientation = GetPageOrientation ();
	m_printersettings . duplex_mode = GetJobDuplex ();
	m_printersettings . copies = GetJobCopies ();
	m_printersettings . collate = GetJobCollate ();
	
	MCRectangle aRect ;
	aRect . x = 0 ;
	aRect . y = 0 ;
	bool t_rotated = (( m_printersettings . orientation == PRINTER_ORIENTATION_PORTRAIT ) || ( m_printersettings . orientation == PRINTER_ORIENTATION_REVERSE_PORTRAIT ) ) ;

	aRect = GetDeviceRectangle();
	m_printersettings . paper_size_width = t_rotated ? aRect . height : aRect . width ;
	m_printersettings . paper_size_height = t_rotated ? aRect . width : aRect . height ;
}

////////////////////////////////////////////////////////////////////////////////

static void exec_command(char *command);

MCPrinterResult MCPSPrinter::DoBeginPrint(MCStringRef p_document, MCPrinterDevice*& r_device)
{
	const char *t_output_file;
	if (GetDeviceOutputType() == PRINTER_OUTPUT_FILE)
		t_output_file = GetDeviceOutputLocation();
	else
        t_output_file = C_FNAME;
    
    // Create a stringref from the output path.
    MCAutoStringRef t_path;
    /* UNCHECKED */ MCStringCreateWithCString(t_output_file, &t_path);
    
    // Now attempt to create a PDF printer - creation of the custom printer
    // copies all the existing printer state into itself, so we basically get
    // a custom printer configured just like we are.
    if (!MCCustomPrinterCreate(MCSTR("pdf"), *t_path, kMCEmptyArray, m_pdf_printer))
        return PRINTER_RESULT_ERROR;
	
    // Now all we need to do is get the PDF printer to begin!
    return m_pdf_printer -> DoBeginPrint(p_document, r_device);
}

MCPrinterResult MCPSPrinter::DoEndPrint(MCPrinterDevice* p_device)
{
    // If we have no PDF printer, then we can't do anything.
    if (m_pdf_printer == nil)
        return PRINTER_RESULT_ERROR;
    
    // Get the PDF printer to finish.
    if (m_pdf_printer -> DoEndPrint(p_device) == PRINTER_RESULT_SUCCESS)
    {
        // Need to sync the setting between the engine and our copy of the job etc
        SyncSettings ();
	
        if (GetDeviceOutputType() == PRINTER_OUTPUT_DEVICE)
        {
            char buffer[1024];
            
            sprintf(buffer, "lp " ) ;
            
            if ( m_printersettings . printername != NULL )
                sprintf( buffer, "%s -d %s", buffer, m_printersettings . printername ) ;
            
            if ( m_printersettings . copies > 1 )
                sprintf ( buffer, "%s -n %d", buffer, m_printersettings . copies ) ;
            
            if ( m_printersettings . orientation != PRINTER_ORIENTATION_PORTRAIT )
                sprintf(buffer, "%s -o landscape", buffer );
            
            if ( m_printersettings . collate )
                sprintf(buffer, "%s -o collate=true", buffer ) ;
            
            if ( m_printersettings . duplex_mode == PRINTER_DUPLEX_MODE_SHORT_EDGE )
                sprintf(buffer, "%s -sides=two-sided-short-edge", buffer );
            
            if ( m_printersettings . duplex_mode == PRINTER_DUPLEX_MODE_LONG_EDGE )
                sprintf(buffer, "%s -sides=two-sided-long-edge", buffer );
            
            sprintf( buffer, "%s %s\n", buffer, C_FNAME ) ;
            
            if (GetDeviceCommand() != NULL)
                sprintf(buffer, GetDeviceCommand(), C_FNAME ) ;
            
            exec_command(buffer);
        }
    }
    
    delete m_pdf_printer;
    m_pdf_printer = nil;
	
	return PRINTER_RESULT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////

static void exec_command ( char * command )
{
    MCAutoStringRef t_command;
    /* UNCHECKED */ MCStringCreateWithCString(command, &t_command);
    
    MCAutoStringRef t_output;
    if (MCS_runcmd(*t_command, &t_output) != IO_NORMAL)
	{
		MCeerror->add(EE_PRINT_ERROR, 0, 0);
	}
	else
        MCresult->setvalueref(*t_output);
}

////////////////////////////////////////////////////////////////////////////////
