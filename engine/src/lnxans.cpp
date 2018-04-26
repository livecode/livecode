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
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"

#include "exec.h"
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

#include <locale.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtkpagesetupunixdialog.h>
#include <gtk/gtkprintunixdialog.h>

////////////////////////////////////////////////////////////////////////////////

struct MCLinuxPageSetup
{
	int32_t paper_width;
	int32_t paper_height;
	int32_t left_margin, top_margin, right_margin, bottom_margin;
	uint32_t orientation;
};

bool MCLinuxPageSetupEncode(const MCLinuxPageSetup& setup, MCDataRef& r_data);
bool MCLinuxPageSetupDecode(MCDataRef p_data, MCLinuxPageSetup& setup);

////////////////////////////////////////////////////////////////////////////////

struct MCLinuxPrintSetup
{
};

bool MCLinuxPrintSetupEncode(const MCLinuxPrintSetup& setup, MCDataRef& r_data);
bool MCLinuxPrintSetupDecode(MCDataRef p_data, MCLinuxPrintSetup& setup);

////////////////////////////////////////////////////////////////////////////////

typedef GtkWidget* (*gtk_file_chooser_dialog_newPTR )  (const gchar *title,  GtkWindow *parent, GtkFileChooserAction action, const gchar *first_button_text, GtkResponseType first_button_response, ...) ; //, const gchar *second_button_text, GtkResponseType second_button_response, void *EOF );
extern gtk_file_chooser_dialog_newPTR gtk_file_chooser_dialog_new_ptr;

extern void MCRemoteFileDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint32_t p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file, bool p_save, bool p_files, MCStringRef &r_value);
extern void MCRemoteFolderDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef &r_value);
extern void MCRemoteColorDialog(MCStringRef p_title, uint32_t p_r, uint32_t p_g, uint32_t p_b, bool& r_chosen, MCColor& r_chosen_color);
extern void MCRemotePrintSetupDialog(MCDataRef p_config_data, MCDataRef &r_reply_data, uint32_t &r_result);
extern void MCRemotePageSetupDialog(MCDataRef p_config_data, MCDataRef &r_reply_data, uint32_t &r_result);

bool G_init = false ;

char * G_last_opened_path ;
char * G_last_saved_path ;


void gtk_file_tidy_up ( void ) 
{
	g_free ( G_last_opened_path );
	g_free ( G_last_saved_path ) ;
}

extern void gdk_event_fn(GdkEvent*, gpointer);
extern void gdk_event_fn_lost(void*);

// Initilize the GTK library, if we have not already done so.
void gtk_init(void)
{
	
	if ( G_init == false ) 
	{
		G_init = True ;
		
		// If we're not careful, GTK will steal GDK events from us
        gtk_init(NULL, NULL);
        gdk_event_handler_set(&gdk_event_fn, MCscreen, &gdk_event_fn_lost);
		gdk_error_trap_push(); 		// Disable all x-error trapping ...
		
		
		// These are free'd in gtk_file_tidy_up that is called from MCscreen::close()
		G_last_opened_path =(char*)g_malloc(1);
		G_last_saved_path = (char*)g_malloc(1);
		
		memset ( G_last_opened_path, 0, sizeof(char) ) ;
		memset ( G_last_saved_path, 0, sizeof(char) ) ;
		
		// TS 2007-31-10 Bug  5408 
		// We need to reset the locale to the C portable one here, as GTK
		// breaks numbers for French systems.
		
		/* TODO */
		// Revisit this as it is not ideal in a Unicode world...
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
char * get_filter_name (const char * p_type )
{
	uint4 a;
    const char * t_ptr ;

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
char * get_filter_masks (const char * p_type )
{
    const char *t_ptr ;
	uint4 a ;

	a = 0 ;
	t_ptr = p_type ;
	
	// Find first delimiter
	while ( *t_ptr != '|' && *t_ptr != '\0' )
		t_ptr ++ ;
	
	if ( *t_ptr == '\0')
		return (NULL);	// Something went wrong!
	t_ptr++; // Move over the breaking char
	
    // MW-2010-10-14: Make sure we have enough room to do this.
    char *ret;
    ret = strdup(p_type);
    memset(ret, 0, strlen(p_type));
    
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
	uint4 t_count ;
	char *t_ptr ;

	t_count = 0 ;
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
	if (t_window == DNULL && MCtopstackptr)
		t_window = MCtopstackptr -> getwindow();

	gtk_widget_realize( GTK_WIDGET( p_widget )) ;
	
	if ( t_window != NULL)
	{
		GdkWindow * gdk_window = NULL ;
		gdk_window = GTK_WIDGET ( p_widget ) -> window ;
		if ( gdk_window != NULL )
            gdk_window_set_transient_for(gdk_window, t_window);
		else 
			gtk_window_set_keep_above ( GTK_WINDOW ( p_widget ) , True ) ;
	}
}
	


GtkWidget * create_open_dialog (MCStringRef p_title , GtkFileChooserAction action)
{
	
	gtk_init();
	
	GtkWidget *dialog;

    MCAutoStringRefAsSysString t_title;
    t_title.Lock(p_title);
    dialog = gtk_file_chooser_dialog_new_ptr(*t_title, NULL, action,
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
 

void run_dialog(GtkWidget *dialog, MCStringRef &r_value)
{
	// TODO : This needs to be changed to a proper callback function : gdk_event_handler_set()
	g_timeout_add(100, gtk_idle_callback, NULL);

	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        MCAutoStringRef t_filename;
		if ( gtk_file_chooser_get_select_multiple ( GTK_FILE_CHOOSER ( dialog ) ) )
		{
            MCAutoListRef t_filenames;
             /* UNCHECKED */ MCListCreateMutable('\n', &t_filenames);

            GSList * t_filename_list ;
			
			t_filename_list = gtk_file_chooser_get_filenames ( GTK_FILE_CHOOSER ( dialog )) ;
			while ( t_filename_list != NULL )
            {
                MCAutoStringRef t_item;
                /* UNCHECKED */ MCStringCreateWithSysString((char*)t_filename_list -> data, &t_item);
                /* UNCHECKED */ MCListAppend(*t_filenames, *t_item);

				t_filename_list = t_filename_list -> next ;
			}

            /* UNCHECKED */ MCListCopyAsString(*t_filenames, &t_filename);
		}
		else
        {
            gchar *t_cstr_filename;
            t_cstr_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
            /* UNCHECKED */ MCStringCreateWithSysString((char*)t_cstr_filename, &t_filename);
            g_free(t_cstr_filename);
		}

        if (*t_filename != nil)
            r_value = MCValueRetain(*t_filename);
	}
}



void close_dialog ( GtkWidget *dialog ) 
{
	
	gtk_widget_destroy(dialog);

	while (gtk_events_pending())
		gtk_main_iteration();
	
}


void add_dialog_filters(GtkWidget *dialog, MCStringRef *p_types, uint4 p_type_count)
{
	GtkFileFilter *filter ;
	
    if (p_type_count >= 1 )
	{
		for (uint4 a=0; a < p_type_count; a++)
		{
            MCAutoStringRefAsSysString t_type_str;
            t_type_str.Lock(p_types[a]);

			char *t_filter_name;
            t_filter_name = get_filter_name(*t_type_str);
			if (t_filter_name == nil)
				continue;

			char * t_filter_masks;
            t_filter_masks = get_filter_masks(*t_type_str);
            if (t_filter_masks == nil)
			{
				free (t_filter_name);
                continue;
			}

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

			free (t_filter_name);
			free (t_filter_masks);
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




bool file_has_path(MCStringRef p_file)
{
    return ((MCStringGetNativeCharAtIndex(p_file, 0) == '/' ) ||
            (MCStringGetNativeCharAtIndex(p_file, 0) == '~' ) ||
            (MCStringGetNativeCharAtIndex(p_file, 0) == '.' ) );
}



void set_initial_file ( GtkWidget *dialog, MCStringRef p_initial, char * p_last_path)
{
    MCAutoStringRef t_resolved;

	if ( p_initial != NULL )
    {
        if (file_has_path(p_initial))
            MCS_resolvepath(p_initial, &t_resolved);
		else
        {
            MCAutoStringRef t_current_dir;
            MCAutoStringRef t_folder;
            /* UNCHECKED */ MCS_getcurdir(&t_current_dir);
            /* UNCHECKED */ MCStringFormat(&t_folder, "%@/%@", *t_current_dir, p_initial);
            /* UNCHECKED */ MCS_resolvepath(*t_folder, &t_resolved);
        }
			
        MCAutoStringRefAsSysString t_resolved_sys;
        t_resolved_sys.Lock(*t_resolved);
        if (MCS_exists(*t_resolved, True)) // file
            gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( dialog ), *t_resolved_sys);
        else // folder
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), *t_resolved_sys);

	}
	else
		if ( p_last_path != nil )
          gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ) , p_last_path);
}








// ---===================================================---
//
//           MCA_* functions start here
//
// ---===================================================---

extern bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count);


static bool types_to_remote_types(MCStringRef *p_types, uint4 p_type_count, MCStringRef *&r_rtypes, uint32_t &r_count)
{
    MCAutoStringRefArray t_rtypes;
    if (!t_rtypes.New(p_type_count * 2))
        return false;

    index_t t_count;
    t_count = 0;

	for(uint32_t i = 0; i < p_type_count; i++)
    {
        MCAutoStringRefArray t_parts;
        if (MCStringsSplit(p_types[i], '|', t_parts.PtrRef(), t_parts.CountRef()))
        {
            if (t_parts.Count() > 0)
            {
                t_rtypes[t_count++] = MCValueRetain(t_parts[0]);
                if (t_parts.Count() > 1)
                    t_rtypes[t_count++] = MCValueRetain(t_parts[1]);
            }
        }
	}

    t_rtypes.Shrink(t_count);
    t_rtypes.Take(r_rtypes, r_count);

	return true;
}

int MCA_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{	
    MCA_file_with_types(p_title, p_prompt, NULL, 0, p_initial, p_options, r_value, r_result);
    return(1);
}




int MCA_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)

{
	if (!MCModeMakeLocalWindows())
	{
		bool t_plural = (p_options & MCA_OPTION_PLURAL) != 0;

        MCAutoStringRef t_resolved_path;
        /* UNCHECKED */ MCS_resolvepath(p_initial, &t_resolved_path);

        MCStringRef *t_rtypes;
        uindex_t t_count;
        if (types_to_remote_types(p_types, p_type_count, t_rtypes, t_count))
		{
            MCRemoteFileDialog(p_title, p_prompt, t_rtypes, t_count, NULL, *t_resolved_path, false, t_plural, r_result);
            for (uint32_t i = 0; i < t_count; ++i)
                MCValueRelease(t_rtypes[i]);

            MCMemoryDeleteArray(t_rtypes);
        }
		return 1;
	}

	//////////

	GtkWidget *dialog ;
	
	
    // Create the file dialog with the correct prompt
    dialog = create_open_dialog ( p_title == NULL || MCStringIsEmpty(p_title) ? p_prompt : p_title, GTK_FILE_CHOOSER_ACTION_OPEN );
	
	// If we have any filters, add them.
	if ( p_type_count > 0 ) 
        add_dialog_filters ( dialog, p_types , p_type_count );

	
	if ( p_options & MCA_OPTION_PLURAL ) 
		gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ) ,true );
	
    // If we have an initial file/folder then set it.
    set_initial_file ( dialog, p_initial, G_last_opened_path ) ;

	// Run the dialog ... this will be replaced with our own loop which will call the REV event handler too.

    run_dialog(dialog, r_value);

    if (r_value == nil)
        /* UNCHECKED */ MCStringCreateWithCString(MCcancelstring, r_result);
    else if ((p_options & MCA_OPTION_RETURN_FILTER) != 0)
        /* UNCHECKED */ MCStringCreateWithSysString(get_current_filter_name(dialog), r_result);

	
	if (G_last_opened_path != nil)
		g_free(G_last_opened_path);
	G_last_opened_path = gtk_file_chooser_get_current_folder ( GTK_FILE_CHOOSER ( dialog ) ) ;
	
	// All done, close the dialog.
	close_dialog ( dialog ) ;
        return(1);
}




int MCA_ask_file(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_filter, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
	//TODO : This still needs to pass over the p_filter.
    MCA_ask_file_with_types ( p_title, p_prompt, NULL, 0, p_initial, p_options, r_value, r_result);
	return(1);
}


int MCA_ask_file_with_types(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
    if (!MCModeMakeLocalWindows())
    {
        bool t_plural = (p_options & MCA_OPTION_PLURAL) != 0;
        MCAutoStringRef t_resolved_path;
        /* UNCHECKED */ MCS_resolvepath(p_initial, &t_resolved_path);
        MCStringRef *t_rtypes;
        uindex_t t_count;

        if (types_to_remote_types(p_types, p_type_count, t_rtypes, t_count))
		{
            MCRemoteFileDialog(p_title, p_prompt, t_rtypes, t_count, NULL, *t_resolved_path, true, t_plural, r_result);
            for (uint32_t i = 0; i < t_count; ++i)
                MCValueRelease(t_rtypes[i]);

            MCMemoryDeleteArray(t_rtypes);
        }
		return 1;
	}

	GtkWidget *dialog ;
	
    dialog = create_open_dialog(p_title == NULL ? p_prompt : p_title, GTK_FILE_CHOOSER_ACTION_SAVE );

	if ( p_type_count > 0 ) 
		add_dialog_filters ( dialog, p_types , p_type_count );

	// If we are given an initial
    if (p_initial != nil)
	{
		if (MCS_exists(p_initial, True))
        {
            MCAutoStringRef t_resolved;
            /* UNCHECKED */ MCS_resolvepath(p_initial, &t_resolved);
            MCAutoStringRefAsSysString t_resolved_sys;
            t_resolved_sys.Lock(*t_resolved);
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), *t_resolved_sys);
		}
		else
		{
            MCAutoStringRef t_folder;
            MCAutoStringRef t_name;
            uindex_t t_last_slash;
            bool t_folder_exists;

            if (!MCStringLastIndexOfChar(p_initial, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_slash))
            {
                /* UNCHECKED */ MCStringCopy(p_initial, &t_name);
                t_folder_exists = false;
            }
            else
            {
                MCAutoStringRef t_tmp_folder;
                MCStringCopySubstring(p_initial, MCRangeMakeMinMax(t_last_slash + 1, MCStringGetLength(p_initial)), &t_name);
                MCStringCopySubstring(p_initial, MCRangeMake(0, t_last_slash), &t_tmp_folder);

                if (MCS_exists(*t_tmp_folder, False))
                    /* UNCHECKED */ MCS_resolvepath(*t_tmp_folder, &t_folder);
                else
                    /* UNCHECKED */ MCStringCopy(*t_tmp_folder, &t_folder);

                t_folder_exists = true;
            }

            MCAutoStringRefAsSysString t_folder_sys, t_name_sys;

            if (t_folder_exists)
            {
                /* UNCHECKED */ t_folder_sys.Lock(*t_folder);
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), *t_folder_sys);
            }
            else
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), G_last_saved_path);

            /* UNCHECKED */ t_name_sys.Lock(*t_name);
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), *t_name_sys);
		}
	}
	else
	{
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), G_last_saved_path);
	}
	
    run_dialog(dialog, r_value) ;

    MCStringCreateWithSysString(get_current_filter_name(dialog), r_result);

	if (G_last_saved_path != NULL)
		g_free(G_last_saved_path);
	G_last_saved_path = gtk_file_chooser_get_current_folder ( GTK_FILE_CHOOSER ( dialog ) ) ;

	close_dialog ( dialog ) ;
        
        return(1);
	
}




int MCA_folder(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, unsigned int p_options, MCStringRef &r_value, MCStringRef &r_result)
{
    MCAutoStringRef t_resolved;
    if (p_initial != nil)
        /* UNCHECKED */ MCS_resolvepath(p_initial, &t_resolved);
    else
        MCS_getcurdir(&t_resolved);

    if (!MCModeMakeLocalWindows())
    {
        MCRemoteFolderDialog(p_title, p_prompt, *t_resolved, r_value);
		return 0;
	}
	
	//////////

	GtkWidget *dialog ;
	
	
    dialog = create_open_dialog(p_title == nil || MCStringGetLength(p_title) == 0  ? p_prompt : p_title, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );

    MCAutoStringRefAsSysString t_resolved_sys;
    /* UNCHECKED */ t_resolved_sys.Lock(*t_resolved);
    if (p_initial != NULL)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), *t_resolved_sys);

	
    run_dialog(dialog, r_value);
    close_dialog(dialog);
        
        return (1);
	
}





// ---===================================================---
//
//           Colour routines
//
// ---===================================================---


bool MCA_color(MCStringRef p_title, MCColor p_initial_color, bool p_as_sheet, bool& r_chosen, MCColor& r_chosen_color)
{
	GtkWidget * dialog ;
	GtkColorSelection *colorsel;
    GdkColor gdk_color;
	
	gtk_init();
	
    gdk_color . red = p_initial_color . red;
    gdk_color . green = p_initial_color . green;
    gdk_color . blue = p_initial_color . blue;
		
    MCAutoStringRefAsSysString t_title;
    /* UNCHECKED */ t_title.Lock(p_title);
    dialog = gtk_color_selection_dialog_new  (*t_title);
	make_front_widget ( dialog ) ;
	colorsel = GTK_COLOR_SELECTION ( GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel );

	gtk_color_selection_set_current_color  ( colorsel, &gdk_color ) ;

	g_timeout_add(100, gtk_idle_callback, NULL);
	
	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
        gtk_color_selection_get_current_color(colorsel, &gdk_color);

		r_chosen_color . red = gdk_color . red;
		r_chosen_color . blue = gdk_color . blue;
		r_chosen_color . green = gdk_color . green;
		r_chosen = true;
	}
	else
	{
		r_chosen = false;
	}
	
	
	gtk_widget_destroy(dialog);

	while (gtk_events_pending())
		gtk_main_iteration();
	
	return true;
}

// MERG-2013-08-18: Stubs for colorDialogColors.
void MCA_setcolordialogcolors(MCColor* p_colors, uindex_t p_count)
{

}

void MCA_getcolordialogcolors(MCColor*& r_colors, uindex_t& r_count)
{
	r_count = 0;
}

// ---===================================================---
//
//           Printer dialog routines
//
// ---===================================================---

MCPrinterDialogResult MCA_gtk_printer_setup ( PSPrinterSettings &p_settings )
{
	GtkPrintUnixDialog * dialog ;
	gint ret_code ;
	MCPrinterDialogResult result  = PRINTER_DIALOG_RESULT_CANCEL ;

	gtk_init();
	
	dialog = (GtkPrintUnixDialog *)gtk_print_unix_dialog_new  ( "Printer setup", NULL );
	make_front_widget ( (GtkWidget *)dialog ) ;
	
	gtk_print_unix_dialog_set_manual_capabilities ( GTK_PRINT_UNIX_DIALOG(dialog), GTK_PRINT_CAPABILITY_GENERATE_PDF);

#if NOT_WORKING
    // Capture existing settings and ensure they are presented in the dialog.
    GtkPrintSettings *t_settings;
    t_settings = gtk_print_settings_new();
    //gtk_print_settings_set_printer(t_settings, p_settings, printername);
    gtk_print_settings_set_n_copies(t_settings, p_settings . copies);
    gtk_print_settings_set_collate(t_settings, p_settings . collate);
    gtk_print_settings_set_orientation(t_settings,
            p_settings . orientation == PRINTER_ORIENTATION_PORTRAIT ? GTK_PAGE_ORIENTATION_PORTRAIT :
            p_settings . orientation == PRINTER_ORIENTATION_LANDSCAPE ? GTK_PAGE_ORIENTATION_LANDSCAPE :
            p_settings . orientation == PRINTER_ORIENTATION_REVERSE_PORTRAIT ? GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT :
            /*p_settings . orientation == PRINTER_ORIENTATION_REVERSE_LANDSCAPE ?*/ GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE);
    gtk_print_settings_set_duplex(t_settings,
            p_settings . duplex_mode == PRINTER_DUPLEX_MODE_SIMPLEX ? GTK_PRINT_DUPLEX_SIMPLEX :
            p_settings . duplex_mode == PRINTER_DUPLEX_MODE_LONG_EDGE ? GTK_PRINT_DUPLEX_HORIZONTAL :
            /* p_settings . duplex_mode == PRINTER_DUPLEX_MODE_SHORT_EDGE ? */ GTK_PRINT_DUPLEX_VERTICAL);
    gtk_print_settings_set_paper_width(t_settings, p_settings . paper_size_width, GTK_UNIT_POINTS);
    gtk_print_settings_set_paper_height(t_settings, p_settings . paper_size_height, GTK_UNIT_POINTS);
    gtk_print_unix_dialog_set_settings(dialog, t_settings);
    g_object_unref(t_settings);
#endif
    
	g_timeout_add(100, gtk_idle_callback, NULL);
	ret_code = gtk_dialog_run(GTK_DIALOG (dialog)) ;

	if ( ret_code == GTK_RESPONSE_OK ) 
	{
		result = PRINTER_DIALOG_RESULT_OKAY ;
		
		GtkPrinter*     t_printer ;
		t_printer = gtk_print_unix_dialog_get_selected_printer  ( GTK_PRINT_UNIX_DIALOG ( dialog ) ) ;
		p_settings . printername = strdup ( gtk_printer_get_name ( t_printer ) ) ;

		if ( p_settings . outputfilename != NULL ) 
			delete (p_settings . outputfilename - 7 );
		p_settings . outputfilename = NULL ;
		p_settings . printertype = PRINTER_OUTPUT_DEVICE ;
		
        GtkPrintSettings* t_printer_settings ;
		t_printer_settings = gtk_print_unix_dialog_get_settings  ( GTK_PRINT_UNIX_DIALOG ( dialog )) ;
        
		if ( strcmp( p_settings . printername, "Print to File") == 0 ) 
		{
			p_settings . printertype = PRINTER_OUTPUT_FILE ;
			p_settings . outputfilename = strdup ( gtk_print_settings_get ( t_printer_settings, GTK_PRINT_SETTINGS_OUTPUT_URI) ) ;
			p_settings . outputfilename += 7 ;
		}

#ifdef NOT_WORKING

		
/*		GtkPageRange* t_ranges ;
		MCInterval * t_rev_ranges ;
		
		int4 t_range_count;
		t_ranges = gtk_print_settings_get_page_ranges  ( t_printer_settings , &t_range_count ) ;
		if ( t_range_count > 0 ) 
		{
			p_settings . page_ranges = (MCInterval*)t_ranges ; // This is OK as the structures are the same - just different member names.
			p_settings . page_range_count = t_range_count ;
			
			// We need to adjust these as GTK starts pages at 0 and we start pages at 1
			for (int4 a = 0; a < t_range_count; a++)
			{
				p_settings . page_ranges[a] . from++;
				p_settings . page_ranges[a] . to++;
			}
		}*/

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
        p_settings . paper_size_width = (uint4)ceil(gtk_print_settings_get_paper_width(t_printer_settings, GTK_UNIT_POINTS));
        p_settings . paper_size_height = (uint4)ceil(gtk_print_settings_get_paper_height(t_printer_settings, GTK_UNIT_POINTS));
#endif
        
        g_object_unref(t_printer_settings);
	}
	
	gtk_widget_destroy((GtkWidget *)dialog);

	while (gtk_events_pending())
		gtk_main_iteration();
	
	return (result);
}







MCPrinterDialogResult MCA_gtk_page_setup (PSPrinterSettings &p_settings)
{
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

////////////////////////////////////////////////////////////////////////////////

bool MCLinuxPageSetupEncode(const MCLinuxPageSetup& setup, MCDataRef &r_data)
{
	return false;
}

bool MCLinuxPageSetupDecode(MCDataRef p_data, MCLinuxPageSetup& setup)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCLinuxPrintSetupEncode(const MCLinuxPrintSetup& setup, MCDataRef &r_data)
{
	return false;
}

bool MCLinuxPrintSetupDecode(MCDataRef p_data, MCLinuxPrintSetup& setup)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////
