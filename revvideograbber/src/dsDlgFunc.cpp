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

#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <stdio.h>

//    Globals for dialog
static DLGTEMPLATE          gs_a_stDialogTemplate[20];
static int					gs_nLB;
#define MAX_CONTENT			50
static char					gs_a_strLBContent[MAX_CONTENT][256];


//!  CopyToWideChar: copy char string into multibyte string
static void CopyToWideChar( WCHAR** pstrOut, LPTSTR strIn )
{
    long lLength = lstrlen( strIn );
    WCHAR* strOut = *pstrOut;

    lLength = MultiByteToWideChar( CP_ACP, 0, strIn, lLength, strOut, lLength );
    strOut[lLength++] = L'\0'; // Add the null terminator
 
    *pstrOut += lLength;
}
 
//! AddDialogControl: add a dialog item in a Dialogtemplate
static void AddDialogControl( WORD** pp, DWORD dwStyle, SHORT x, SHORT y, SHORT cx, SHORT cy, WORD id,  LPTSTR strClassName, LPTSTR strTitle )
{
    //
    DLGITEMTEMPLATE* p = (DLGITEMTEMPLATE*)(((((ULONG)(*pp))+3)>>2)<<2);
 
    p->style           = dwStyle | WS_CHILD | WS_VISIBLE;
    p->dwExtendedStyle = 0L;
    p->x               = x;
    p->y               = y;
    p->cx              = cx;
    p->cy              = cy;
    p->id              = id;
 
    *pp = (WORD*)(++p); // Advance ptr
 
	//Set Class name
    CopyToWideChar( (WCHAR**)pp, strClassName ); 
	
	// Set Title
    CopyToWideChar( (WCHAR**)pp, strTitle );     
 
	//Skip Extra Stuff
    (*pp)++;
}
 
 

//!  fn_p_stPrepareDialogBox_LB   : build simple dialog box with one list
DLGTEMPLATE *fn_p_stPrepareDialogBox_LB( char *szTitle )
{
    DLGTEMPLATE *pdt;
    WORD        *pw;
 
    // Allocate ample memory for building the template
    memset( gs_a_stDialogTemplate, 0, 20*sizeof(DLGTEMPLATE) );
    
    // Fill in the DLGTEMPLATE info
    pdt                 = gs_a_stDialogTemplate;
    pdt->style          = DS_MODALFRAME | DS_NOIDLEMSG | DS_SETFOREGROUND | DS_3DLOOK | DS_CENTER | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU ;
    pdt->dwExtendedStyle = 0L;
    pdt->cdit            = 2;
    pdt->x               = 0;
    pdt->y               = 0;
    pdt->cx              = 310;
    pdt->cy              = 110;
 
    // Add menu array, class array, dlg title, font size and font name*/
    pw      = (WORD*)(++pdt);
    *pw++   = 0;                                                            // Set Menu array to nothing
    *pw++   = 0;                                                            // Set Class array to nothing
    CopyToWideChar( (WCHAR**)&pw, szTitle );                                // Dlg title
    /*
    *pw++ = 8;                                                              // Font Size
    CopyToWideChar( (WCHAR**)&pw, "Arial" );                                // Font Name
    */
 
    AddDialogControl( &pw, BS_PUSHBUTTON | WS_TABSTOP, 130, 90, 50, 15, IDOK, "BUTTON", "OK" );
    AddDialogControl( &pw, (LBS_STANDARD - LBS_SORT) | WS_VSCROLL | WS_TABSTOP, 5, 5, 300, 80, 1000, "LISTBOX", "");
 
    return gs_a_stDialogTemplate;
}


 

//!  _DisplayModeSelectProc  : dialog callback
BOOL CALLBACK _DisplayModeSelectProc( HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{
    // Handle the initialization message
    if( uiMsg == WM_INITDIALOG )
    {
        long    lLBItem, lItem;
        char    szText[150];
        
		// fill list box
        SendDlgItemMessage( hDlg, 1000, LB_RESETCONTENT, 0, 0 );

        for (lItem = 0; lItem < gs_nLB; lItem++)
        {
            sprintf( szText, "%s", gs_a_strLBContent[lItem]);

            lLBItem = SendDlgItemMessage( hDlg, 1000, LB_ADDSTRING, 0, (LPARAM) szText );
            if (lItem != LB_ERR)
                SendDlgItemMessage( hDlg, 1000, LB_SETITEMDATA, lLBItem, lItem );
        }

        //Search for resolution to set at first
        for (lItem = 0; lItem < gs_nLB; lItem++)
        {
            if (lParam == SendDlgItemMessage( hDlg, 1000, LB_GETITEMDATA, lLBItem, 0 ) )
                break;
        }
        
        if ( (lParam < 0) || (lParam >= gs_nLB) )
            lParam = 0;

        //select first line
        SendDlgItemMessage( hDlg, 1000, LB_SETCURSEL, lParam, 0 );

        SendMessage( hDlg, WM_SHOWWINDOW, SW_SHOW, 0 );
        return TRUE;
    }

	//handle command message
    if( uiMsg == WM_COMMAND )
    {
        // Handle the case when the user hits the OK button
        if( IDOK == LOWORD(wParam) )
        {
            long lItem, lReturn;

            lItem = SendDlgItemMessage( hDlg, 1000, LB_GETCURSEL, 0, 0 );
            lReturn = SendDlgItemMessage( hDlg, 1000, LB_GETITEMDATA, lItem, 0 );

            EndDialog( hDlg, lReturn );
            return TRUE;
        }
    }
    return FALSE;
}


//select a LB item by dialog
long Dlg_fn_lDoDialogBox_LB(char *strTitle, long nLB, char *buffer, long fixLength, long _lIndex )
{
    DLGTEMPLATE     *p_stDlgTemplate;     
	long select; 

	//copy the string to local
	gs_nLB = min(nLB, MAX_CONTENT);
	for(int i=0; i< gs_nLB; i++)
	{
		strcpy(gs_a_strLBContent[i], buffer);
		buffer+= fixLength;
	}

	//prepare dlg
    p_stDlgTemplate = fn_p_stPrepareDialogBox_LB( strTitle );
	
	//get selection
    select = DialogBoxIndirectParam( NULL, p_stDlgTemplate, NULL, (DLGPROC)_DisplayModeSelectProc, _lIndex );

    return select;
}

 

