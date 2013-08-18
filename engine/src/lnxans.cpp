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

#include "lnxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "execpt.h"
#include "mcerror.h"
#include "ans.h"
#include "stack.h"
#include "stacklst.h"
#include "dispatch.h"
#include "globals.h"
#include "util.h"
#include "printer.h"
#include "osspec.h"
#include "mode.h"

#include "lnxdc.h"
#include "lnxpsprinter.h"
#include "lnxans.h"
#include "sserialize_lnx.h"

#include <locale.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtkpagesetupunixdialog.h>
#include <gtk/gtkprintunixdialog.h>

typedef GtkWidget* (*gtk_file_chooser_dialog_newPTR )  (const gchar *title,  GtkWindow *parent, GtkFileChooserAction action, const gchar *first_button_text, GtkResponseType first_button_response, ...) ; //, const gchar *second_button_text, GtkResponseType second_button_response, void *EOF );
extern gtk_file_chooser_dialog_newPTR gtk_file_chooser_dialog_new_ptr;

extern void MCRemoteFileDialog(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char * const p_types[], uint32_t p_type_count, const char *p_initial_folder, const char *p_initial_file, bool p_save, bool p_files);
extern void MCRemoteFolderDialog(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_initial);
extern void MCRemoteColorDialog(MCExecPoint& ep, const char *p_title, uint32_t p_r, uint32_t p_g, uint32_t p_b);
extern void MCRemotePrintSetupDialog(char *&r_reply_data, uint32_t &r_reply_data_size, uint32_t &r_result, const char *p_config_data, uint32_t p_config_data_size);
extern void MCRemotePageSetupDialog(char *&r_reply_data, uint32_t &r_reply_data_size, uint32_t &r_result, const char *p_config_data, uint32_t p_config_data_size);

bool G_init = false ;

char * G_last_opened_path ;
char * G_last_saved_path ;


void gtk_file_tidy_up ( void ) 
{
	g_free ( G_last_opened_path );
	g_free ( G_last_saved_path ) ;
}


// Initilize the GTK library, if we have not already done so.
void gtk_init(void)
{
	
	if ( G_init == false ) 
	{
		G_init = True ;
		
		gtk_init(NULL, NULL);
		gdk_error_trap_push(); 		// Disable all x-error trapping ...
		
		
		// These are free'd in gtk_file_tidy_up that is called from MCscreen::close()
		G_last_opened_path =(char*)g_malloc(1);
		G_last_saved_path = (char*)g_malloc(1);
		
		memset ( G_last_opened_path, 0, sizeof(char) ) ;
		memset ( G_last_saved_path, 0, sizeof(char) ) ;
		
		// TS 2007-31-10 Bug  5408 
		// We need to reset the locale to the C portable one here, as GTK
		// breaks numbers for French systems.
		
		setlocale(LC_ALL, "C");
		//NOTE: Should this be :
		//		setlocale(LC_NUMERIC, "C")
		// 	so that only the numeric part of the locale is set to C?
	}
}	


// ---===================================================---
//
//           Mask manipulation routines
//
// ---===================================================---

//NOTE: These routines may need to be converted to take care of UniCode later on.

// Return the name of the filter
char * get_filter_name ( char * p_type ) 
{
	uint4 a;
	char * t_ptr ;

	// MW-2010-10-14: Make sure we have enough room to do this.
	char *ret;
	ret = strdup(p_type);
	memset(ret, 0, strlen(p_type));

	t_ptr = p_type ;
	a = 0 ;
	do
	{
		if ( *t_ptr == '|' )
			break; // We have got to the end....
		
		ret[a] = *t_ptr ;
		t_ptr++; a++;
	}
	while (*t_ptr != '\0' );

	return ret;

}

// Return ALL the masks from the filter
char * get_filter_masks ( char * p_type ) 
{
	char *t_ptr ;
	uint4 a ;

	// MW-2010-10-14: Make sure we have enough room to do this.
	char *ret;
	ret = strdup(p_type);
	memset(ret, 0, strlen(p_type));

	a = 0 ;
	t_ptr = p_type ;
	
	// Find first delimiter
	while ( *t_ptr != '|' && *t_ptr != '\0' )
		t_ptr ++ ;
	
	if ( *t_ptr == '\0')
		return (NULL);	// Something went wrong!
	t_ptr++; // Move over the breaking char
	
	while ( *t_ptr != '|' && *t_ptr != '\0' )
		ret[a++] = *t_ptr++;
	
	return ret;
}

	
	
// Count how many masks we have in the filter
uint4 get_filter_count ( char * p_masks ) 
{
	uint4 t_count ;
	char *t_ptr ;
	
	t_count = 0 ;
	t_ptr = p_masks ;
	while (*t_ptr != '\0' )
	{
		if ( *t_ptr == ',' ) t_count++;
		t_ptr++;
	}
	return(t_count+1);  // We add 1 as the last (or only) filter has no comma assosiated with it ...
}



// Return the next mask from the comma delimited list of filters.
char * get_next_mask ( char *p_masks ) 
{
	uint4 a ;
	char *t_ptr ;

	// MW-2010-10-14: Make sure we have enough room to do this.
	char *ret;
	ret = strdup(p_masks);
	memset(ret, 0, strlen(p_masks));

	a = 0 ;
	t_ptr = p_masks ;
	while ( *t_ptr != '\0' && *t_ptr != ',' )
		ret[a++] = *t_ptr++;
	return ret;
}	



// Get mask number <p_mask_id> from the comma delimited list of masks.
char * get_filter_mask ( uint4 p_mask_id, char * p_masks ) 
{
	uint4 a ;
	uint4 t_count ;
	char *t_ptr ;

	t_count = 0 ;
	a = 0 ;
	t_ptr = p_masks ;
	
	
	// if we want mask number 0 (the first mask) we will just return the "next" mask
	if ( p_mask_id == 0 )
		return (get_next_mask (p_masks));
	
	
	// loop through the masks until we reach the end OR we have counted enought mask delimiters
	while (*t_ptr != '\0' && t_count < p_mask_id )
		if ( *t_ptr++ == ',' ) t_count++;
	
	// now we are pointing at the start of the correct mask, so we return the "next" one.
	return (get_next_mask ( t_ptr ));
	
}






// ---===================================================---
//
//           Base DIALOG routines
//
// ---===================================================---


// Make the GTK dialog passed in (p_widget) transient for the toplevel or
// default stack -- i.e. it will float above it.
void make_front_widget ( GtkWidget *p_widget)
{
	Window t_window = MCdefaultstackptr -> getwindow();
	if (t_window == DNULL && MCtopstackptr != DNULL)
		t_window = MCtopstackptr -> getwindow();

	gtk_widget_realize( GTK_WIDGET( p_widget )) ;
	
	if ( t_window != NULL)
	{
		GdkWindow * gdk_window = NULL ;
		gdk_window = GTK_WIDGET ( p_widget ) -> window ;
		if ( gdk_window != NULL ) 
			XSetTransientForHint ( ((MCScreenDC*)MCscreen) -> getDisplay(), GDK_WINDOW_XWINDOW (  ( gdk_window ) ),  t_window  ) ;
		else 
			gtk_window_set_keep_above ( GTK_WINDOW ( p_widget ) , True ) ;
	}
}
	


GtkWidget * create_open_dialog ( const char *p_title , GtkFileChooserAction action) 
{
	
	gtk_init();
	
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new_ptr(p_title, NULL, action,
											 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
											 (action == GTK_FILE_CHOOSER_ACTION_SAVE) ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
											 NULL);
	make_front_widget( dialog ) ;
	return ( dialog ) ;
}





static gboolean gtk_idle_callback (gpointer data)
{
	MCscreen -> expose();
	return (true); 
}
 

void run_dialog ( GtkWidget *dialog, MCExecPoint &ep ) 
{
	char *t_filename;
	
	ep . clear();

	// TODO : This needs to be changed to a proper callback function : gdk_event_handler_set()
	g_timeout_add(100, gtk_idle_callback, NULL);

	bool foo = false ;
	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{

		if ( gtk_file_chooser_get_select_multiple ( GTK_FILE_CHOOSER ( dialog ) ) )
		{
			GSList * t_filename_list ;
			bool t_firstfile = true ;
			
			t_filename_list = gtk_file_chooser_get_filenames ( GTK_FILE_CHOOSER ( dialog )) ;
			while ( t_filename_list != NULL )
			{
				ep . concatcstring( (char*)t_filename_list -> data , EC_RETURN, t_firstfile) ;				
				if (t_firstfile ) t_firstfile = false ;
			
				t_filename_list = t_filename_list -> next ;
			}
		}
		else
		{
			t_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
			ep . copysvalue(t_filename, strlen(t_filename));
		}
		
		
	}
}



void close_dialog ( GtkWidget *dialog ) 
{
	
	gtk_widget_destroy(dialog);

	while (gtk_events_pending())
		gtk_main_iteration();
	
}


void add_dialog_filters(GtkWidget *dialog, char * const p_types[], uint4 p_type_count )
{
	GtkFileFilter *filter ;
	
	if ( p_type_count >= 1 ) 
	{
		for (uint4 a=0; a < p_type_count; a++)
		{
			char *t_filter_name, *t_filter_masks;
			t_filter_name = get_filter_name(p_types[a]);
			t_filter_masks = get_filter_masks(p_types[a]);

			filter = gtk_file_filter_new();
			gtk_file_filter_set_name(filter, t_filter_name);
			
			if (strcasecmp (t_filter_name, "All files") != 0)
			{
				for ( uint4 m = 0 ; m < get_filter_count(t_filter_masks); m++)
				{
					char *t_filter_mask;
					t_filter_mask = get_filter_mask(m, t_filter_masks);
					
					char *filter_mask;
					filter_mask = nil;
					MCCStringFormat(filter_mask, "*.%s", t_filter_mask);

					gtk_file_filter_add_pattern(filter, filter_mask);

					MCCStringFree(filter_mask);
					delete t_filter_mask;
				}
			}
			else
				gtk_file_filter_add_pattern ( filter, "*" );

			gtk_file_chooser_add_filter ( GTK_FILE_CHOOSER ( dialog ) , filter ) ;

			delete t_filter_name;
			delete t_filter_masks;
		}
	}
		
}



const char * get_current_filter_name ( GtkWidget * dialog ) 
{
	GtkFileFilter *filter ;
	filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER ( dialog ) );
	if ( filter != NULL )
		return ( gtk_file_filter_get_name ( filter ) );
	else
		return "\0" ;
}


// ---===================================================---
//
//           File/Path processing routines
//
// ---===================================================---




bool file_has_path ( const char * p_file) 
{
	return (  ( *p_file == '/' ) || ( *p_file == '~' ) || ( *p_file == '.' ) );
}



void set_initial_file ( GtkWidget *dialog, const char * p_initial, char * p_last_path ) 
{
	char *t_filename ;

	if ( p_initial != NULL ) 
	{
		if ( file_has_path (p_initial)  )
			t_filename =  MCS_resolvepath ( p_initial );
		else
			t_filename = strcat ( strcat(MCS_resolvepath ( NULL ), "/") , p_initial );
			
		gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( dialog ) , MCS_resolvepath ( t_filename ) );
	}
	else
		if ( p_last_path != NULL )
		  gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ) , p_last_path );
}








// ---===================================================---
//
//           MCA_* functions start here
//
// ---===================================================---


static bool types_to_remote_types(char * const p_types[], uint4 p_type_count, char**& r_rtypes)
{
	char **t_rtypes;
	if (!MCMemoryNewArray(p_type_count * 2, t_rtypes))
		return false;

	for(uint32_t i = 0; i < p_type_count; i++)
	{
		uint32_t t_part_count;
		char **t_parts;
		if (MCCStringSplit(p_types[i], '|', t_parts, t_part_count))
		{
			if (t_part_count > 0)
			{
				MCCStringClone(t_parts[0], t_rtypes[i * 2]);
				if (t_part_count > 1)
					MCCStringClone(t_parts[1], t_rtypes[i * 2 + 1]);
			}
			MCCStringArrayFree(t_parts, t_part_count);
		}
	}

	r_rtypes = t_rtypes;

	return true;
}

int MCA_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{

	
  	MCA_file_with_types ( ep, p_title, p_prompt, NULL, 0, p_initial, p_options ) ;	
        return(1);
}




int MCA_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)

{
	if (!MCModeMakeLocalWindows())
	{
		bool t_plural = (p_options & MCA_OPTION_PLURAL) != 0;
		char *t_resolved_path = MCS_resolvepath(p_initial);
		char **t_rtypes;
		if (types_to_remote_types(p_types, p_type_count, t_rtypes))
		{
			MCRemoteFileDialog(ep, p_title, p_prompt, t_rtypes, p_type_count * 2, NULL, t_resolved_path, false, t_plural);
			MCCStringArrayFree(t_rtypes, p_type_count * 2);
		}
		delete t_resolved_path;
		return 1;
	}

	//////////

	GtkWidget *dialog ;
	
	
	// Create the file dialog with the correct prompt
	dialog = create_open_dialog ( p_title == NULL  ? p_prompt : p_title, GTK_FILE_CHOOSER_ACTION_OPEN );
	
	// If we have any filters, add them.
	if ( p_type_count > 0 ) 
		add_dialog_filters ( dialog, p_types , p_type_count );

	
	if ( p_options & MCA_OPTION_PLURAL ) 
		gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ) ,true );
	
	// If we have an initial file/folder then set it.
	set_initial_file ( dialog, p_initial, G_last_opened_path ) ;

	// Run the dialog ... this will be replaced with our own loop which will call the REV event handler too.

	run_dialog ( dialog, ep) ;
	
	
	MCresult -> clear();
	MCresult -> copysvalue(get_current_filter_name ( dialog ) );
	
	if (G_last_opened_path != nil)
		g_free(G_last_opened_path);
	G_last_opened_path = gtk_file_chooser_get_current_folder ( GTK_FILE_CHOOSER ( dialog ) ) ;
	
	// All done, close the dialog.
	close_dialog ( dialog ) ;
        return(1);
}




int MCA_ask_file(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_filter, const char *p_initial, unsigned int p_options)
{
	//TODO : This still needs to pass over the p_filter.
	MCA_ask_file_with_types ( ep, p_title, p_prompt, NULL, 0, p_initial, p_options ) ;	
	return(1);
}


int MCA_ask_file_with_types(MCExecPoint& ep, const char *p_title, const char *p_prompt, char * const p_types[], uint4 p_type_count, const char *p_initial, unsigned int p_options)
{
	if (!MCModeMakeLocalWindows())
	{
		bool t_plural = (p_options & MCA_OPTION_PLURAL) != 0;
		char *t_resolved_path = MCS_resolvepath(p_initial);
		char **t_rtypes;
		if (types_to_remote_types(p_types, p_type_count, t_rtypes))
		{
			MCRemoteFileDialog(ep, p_title, p_prompt, t_rtypes, p_type_count * 2, NULL, t_resolved_path, true, t_plural);
			MCCStringArrayFree(t_rtypes, p_type_count * 2);
		}
		delete t_resolved_path;
		return 1;
	}

	GtkWidget *dialog ;
	
	dialog = create_open_dialog( p_title == NULL  ? p_prompt : p_title, GTK_FILE_CHOOSER_ACTION_SAVE );

	if ( p_type_count > 0 ) 
		add_dialog_filters ( dialog, p_types , p_type_count );

	// If we are given an initial
	if (p_initial != nil)
	{
		if (MCS_exists(p_initial, True))
		{
			char *t_path;
			t_path = MCS_resolvepath(p_initial);
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), t_path);
			delete t_path;
		}
		else
		{
			char *t_folder;
			const char *t_name;
			if (strchr(p_initial, '/') == NULL)
			{
				t_folder = NULL;
				t_name = p_initial;
			}
			else
			{
				t_folder = strdup(p_initial);
				strrchr(t_folder, '/')[0] = '\0';
				t_name = strrchr(p_initial, '/') + 1;
				if (MCS_exists(t_folder, False))
				{
					char *t_new_folder;
					t_new_folder = MCS_resolvepath(t_folder);
					delete t_folder;
					t_folder = t_new_folder;
				}
				else
					t_folder = NULL;
			}
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), t_folder == NULL ? G_last_saved_path : t_folder);
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), t_name);
			delete t_folder;
		}
	}
	else
	{
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), G_last_saved_path);
	}
	
	run_dialog ( dialog, ep) ;

	MCresult -> clear();
	MCresult -> copysvalue(get_current_filter_name ( dialog ) );
	

	if (G_last_saved_path != NULL)
		g_free(G_last_saved_path);
	G_last_saved_path = gtk_file_chooser_get_current_folder ( GTK_FILE_CHOOSER ( dialog ) ) ;

	close_dialog ( dialog ) ;
        
        return(1);
	
}




int MCA_folder(MCExecPoint& ep, const char *p_title, const char *p_prompt, const char *p_initial, unsigned int p_options)
{
	if (!MCModeMakeLocalWindows())
	{
		char *t_resolved_initial_path = MCS_resolvepath(p_initial);
		
		MCRemoteFolderDialog(ep, p_title, p_prompt, t_resolved_initial_path);
		if (t_resolved_initial_path != NULL)
			free(t_resolved_initial_path);
		return 0;
	}
	
	//////////

	GtkWidget *dialog ;
	
	
	dialog = create_open_dialog( p_title == NULL  ? p_prompt : p_title, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );

	if ( p_initial != NULL ) 
		gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ) , MCS_resolvepath(p_initial) );
	
	run_dialog ( dialog, ep) ;
	close_dialog ( dialog ) ;
        
        return (1);
	
}





// ---===================================================---
//
//           Colour routines
//
// ---===================================================---




char * rgbtostr ( uint4 red, uint4 green, uint4 blue ) 
{
	char ret[15];
		
	memset(ret, 0, (sizeof(char))*15);
	sprintf( ret, "%d,%d,%d", red >> 8,  green >> 8 , blue >> 8 );
	return ( strdup ( ret ) ) ;
}




int MCA_color(MCExecPoint& ep, const char *p_title, const char *p_initial, Boolean sheet)
{
	uint32_t t_red, t_green, t_blue;
	if (p_initial == NULL)
	{
		t_red = MCpencolor.red;
		t_green = MCpencolor.green;
		t_blue = MCpencolor.blue;
	}
	else
	{
		char *cname = NULL;
		MCColor c;
		MCscreen->parsecolor(p_initial, &c, &cname);
		delete cname;
		t_red = c.red;
		t_green = c.green;
		t_blue = c.blue;
	}

	if (!MCModeMakeLocalWindows())
	{
		MCRemoteColorDialog(ep, p_title, t_red >> 8, t_green >> 8, t_blue >> 8);
		return 0;
	}

	//////////

	GtkWidget * dialog ;
	GtkColorSelection *colorsel;
	GdkColor gdk_color ;
	char *colorstr;
	
	gtk_init();
	
	gdk_color . red = t_red;
	gdk_color . green = t_green;
	gdk_color . blue = t_blue;
		
	dialog = gtk_color_selection_dialog_new  ( p_title);
	make_front_widget ( dialog ) ;
	colorsel = GTK_COLOR_SELECTION ( GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel );

	gtk_color_selection_set_current_color  ( colorsel, &gdk_color ) ;

	g_timeout_add(100, gtk_idle_callback, NULL);
	
	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		gtk_color_selection_get_current_color  (  colorsel , &gdk_color ) ;
		colorstr = rgbtostr ( gdk_color.red, gdk_color.green, gdk_color.blue);
		ep . clear();
		ep . copysvalue(colorstr, strlen(colorstr));
		delete colorstr ;
	}
	else
	{
		ep . clear();
	}
	
	
	gtk_widget_destroy(dialog);

	while (gtk_events_pending())
		gtk_main_iteration();
	
	return (1);
}

// MERG-2013-08-18: Stubs for colorDialogColors.
void MCA_setcolordialogcolors(MCExecPoint& p_ep)
{

}

void MCA_getcolordialogcolors(MCExecPoint& p_ep)
{
	p_ep.clear();
}

// ---===================================================---
//
//           Printer dialog routines
//
// ---===================================================---

MCPrinterDialogResult MCA_gtk_printer_setup ( PSPrinterSettings &p_settings )
{
	if (!MCModeMakeLocalWindows())
	{
		bool t_success;
		t_success = true;

		MCLinuxPrintSetup t_setup;

		void *t_data_in;
		uint32_t t_data_in_size;

		if (t_success)
			t_success = MCLinuxPrintSetupEncode(t_setup, t_data_in, t_data_in_size);

		uint32_t t_result;
		t_result = PRINTER_DIALOG_RESULT_ERROR;
		if (t_success)
		{
			char *t_data_out;
			uint32_t t_data_out_size;
			t_data_out = nil;
			t_data_out_size = 0;

			MCRemotePrintSetupDialog(t_data_out, t_data_out_size, t_result, (const char *)t_data_in, t_data_in_size);

			if (t_result == PRINTER_DIALOG_RESULT_OKAY)
			{
				if (MCLinuxPrintSetupDecode(t_data_out, t_data_out_size, t_setup))
				{
				}
				else
					t_result = PRINTER_DIALOG_RESULT_ERROR;

			}

			free(t_data_out);
		}

		MCMemoryDeallocate(t_data_in);

		return (MCPrinterDialogResult)t_result;
	}

	GtkWidget * dialog ;
	gint ret_code ;
	MCPrinterDialogResult result  = PRINTER_DIALOG_RESULT_CANCEL ;

	gtk_init();
	
	dialog = gtk_print_unix_dialog_new  ( "Printer setup", NULL );
	make_front_widget ( dialog ) ;
	
	gtk_print_unix_dialog_set_manual_capabilities ( GTK_PRINT_UNIX_DIALOG(dialog), GTK_PRINT_CAPABILITY_GENERATE_PS);

	g_timeout_add(100, gtk_idle_callback, NULL);
	ret_code = gtk_dialog_run(GTK_DIALOG (dialog)) ;
	
	
	if ( ret_code == GTK_RESPONSE_OK ) 
	{
		result = PRINTER_DIALOG_RESULT_OKAY ;
		
		GtkPrinter*     t_printer ;
		t_printer = gtk_print_unix_dialog_get_selected_printer  ( GTK_PRINT_UNIX_DIALOG ( dialog ) ) ;
		p_settings . printername = strdup ( gtk_printer_get_name ( t_printer ) ) ;

		
		GtkPrintSettings* t_printer_settings ;
		t_printer_settings = gtk_print_unix_dialog_get_settings  ( GTK_PRINT_UNIX_DIALOG ( dialog )) ;
			

		if ( p_settings . outputfilename != NULL ) 
			delete (p_settings . outputfilename - 7 );
		p_settings . outputfilename = NULL ;
		p_settings . printertype = PRINTER_OUTPUT_DEVICE ;
		
		if ( strcmp( p_settings . printername, "Print to File") == 0 ) 
		{
			p_settings . printertype = PRINTER_OUTPUT_FILE ;
			p_settings . outputfilename = strdup ( gtk_print_settings_get ( t_printer_settings, GTK_PRINT_SETTINGS_OUTPUT_URI) ) ;
			p_settings . outputfilename += 7 ;
		}
		
		
		GtkPageRange* t_ranges ;
		MCRange * t_rev_ranges ;
		
		int4 t_range_count ; 
		
		t_ranges = gtk_print_settings_get_page_ranges  ( t_printer_settings , &t_range_count ) ;
		if ( t_range_count > 0 ) 
		{
			p_settings . page_ranges = (MCRange*)t_ranges ; // This is OK as the structures are the same - just different member names.
			p_settings . page_range_count = t_range_count ;
			
			// We need to adjust these as GTK starts pages at 0 and we start pages at 1
			for ( uint4 a=0; a<t_range_count; a++)
			{
				p_settings . page_ranges[a] . from++;
				p_settings . page_ranges[a] . to++;
			}
		}
		
		
		
		p_settings . copies =  gtk_print_settings_get_n_copies  ( t_printer_settings ) ;
		p_settings . collate = gtk_print_settings_get_collate ( t_printer_settings ) ;
		
		GtkPageOrientation tGtkOr = gtk_print_settings_get_orientation  ( t_printer_settings ) ;
		switch(tGtkOr)
		{
			case GTK_PAGE_ORIENTATION_PORTRAIT:
				p_settings . orientation = PRINTER_ORIENTATION_PORTRAIT ;
			break ;
				
			case GTK_PAGE_ORIENTATION_LANDSCAPE:
				p_settings . orientation = PRINTER_ORIENTATION_LANDSCAPE ;
			break ;
				
			case GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
				p_settings . orientation = PRINTER_ORIENTATION_REVERSE_PORTRAIT ;
			break ;
				
			case GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
				p_settings . orientation = PRINTER_ORIENTATION_REVERSE_LANDSCAPE ;
			break ;
		}
		
		
		switch ( gtk_print_settings_get_duplex  ( t_printer_settings ) )
		{
			case GTK_PRINT_DUPLEX_SIMPLEX:
				p_settings . duplex_mode = PRINTER_DUPLEX_MODE_SIMPLEX ;
			break ;
			
			case GTK_PRINT_DUPLEX_HORIZONTAL:
				p_settings . duplex_mode = PRINTER_DUPLEX_MODE_SHORT_EDGE ;
			break ;
			
			case GTK_PRINT_DUPLEX_VERTICAL:
				p_settings . duplex_mode = PRINTER_DUPLEX_MODE_LONG_EDGE ;
			break ;
			
		}
		
	}
	
	gtk_widget_destroy(dialog);

	while (gtk_events_pending())
		gtk_main_iteration();
	
	return (result);
}







MCPrinterDialogResult MCA_gtk_page_setup (PSPrinterSettings &p_settings)
{
	if (!MCModeMakeLocalWindows())
	{
		bool t_success;
		t_success = true;

		MCLinuxPageSetup t_setup;
		t_setup . paper_width = p_settings . paper_size_width;
		t_setup . paper_height = p_settings . paper_size_height;
		t_setup . orientation = p_settings . orientation;
		t_setup . left_margin = t_setup . top_margin = t_setup . right_margin = t_setup . bottom_margin = 0;

		void *t_data_in;
		uint32_t t_data_in_size;

		if (t_success)
			t_success = MCLinuxPageSetupEncode(t_setup, t_data_in, t_data_in_size);

		uint32_t t_result;
		t_result = PRINTER_DIALOG_RESULT_ERROR;
		if (t_success)
		{
			char *t_data_out;
			uint32_t t_data_out_size;
			t_data_out = nil;
			t_data_out_size = 0;

			MCRemotePageSetupDialog(t_data_out, t_data_out_size, t_result, (const char *)t_data_in, t_data_in_size);

			if (t_result == PRINTER_DIALOG_RESULT_OKAY)
			{
				if (MCLinuxPageSetupDecode(t_data_out, t_data_out_size, t_setup))
				{
					p_settings . paper_size_width = t_setup . paper_width;
					p_settings . paper_size_height = t_setup . paper_height;
					p_settings . orientation = (MCPrinterOrientation)t_setup . orientation;
				}
				else
					t_result = PRINTER_DIALOG_RESULT_ERROR;

			}

			free(t_data_out);
		}

		MCMemoryDeallocate(t_data_in);

		return (MCPrinterDialogResult)t_result;
	}

	GtkWidget * dialog ;
	
	gint ret_code ;
	MCPrinterDialogResult result  = PRINTER_DIALOG_RESULT_CANCEL ;
	
	
	gtk_init();
	
	dialog = gtk_page_setup_unix_dialog_new  ( "Page setup dialog", NULL );
	make_front_widget ( dialog ) ;

	g_timeout_add(100, gtk_idle_callback, NULL);
	
	ret_code = gtk_dialog_run(GTK_DIALOG (dialog)) ;
	
	if ( ret_code == GTK_RESPONSE_OK ) 
	{
	
		result = PRINTER_DIALOG_RESULT_OKAY ;
		
		GtkPageSetup* t_page_setup ;
		t_page_setup = gtk_page_setup_unix_dialog_get_page_setup  ( GTK_PAGE_SETUP_UNIX_DIALOG ( dialog ) ) ;
		
			
		p_settings . paper_size_width = gtk_page_setup_get_paper_width  ( t_page_setup, GTK_UNIT_POINTS ) ;
		p_settings . paper_size_height = gtk_page_setup_get_paper_height  ( t_page_setup, GTK_UNIT_POINTS ) ; 
		
		GtkPageOrientation tGtkOr = gtk_page_setup_get_orientation  ( t_page_setup ) ;
		switch(tGtkOr)
		{
			case GTK_PAGE_ORIENTATION_PORTRAIT:
				p_settings . orientation = PRINTER_ORIENTATION_PORTRAIT ;
			break ;
				
			case GTK_PAGE_ORIENTATION_LANDSCAPE:
				p_settings . orientation = PRINTER_ORIENTATION_LANDSCAPE ;
				uint4 tmp ;
				tmp = p_settings . paper_size_height  ;
				p_settings . paper_size_height = p_settings . paper_size_width ;
				p_settings . paper_size_width = tmp ;
			break ;
				
			case GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
				p_settings . orientation = PRINTER_ORIENTATION_REVERSE_PORTRAIT ;
			break ;
				
			case GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
				p_settings . orientation = PRINTER_ORIENTATION_REVERSE_LANDSCAPE ;
				tmp = p_settings . paper_size_height  ;
				p_settings . paper_size_height = p_settings . paper_size_width ;
				p_settings . paper_size_width = tmp ;
			break ;
		}
		
		
	}
	
	gtk_widget_destroy(dialog);

	while (gtk_events_pending())
		gtk_main_iteration();
	
	return (result );
}
