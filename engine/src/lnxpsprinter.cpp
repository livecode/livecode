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
#include "execpt.h"
#include "stack.h"
#include "region.h"
#include "osspec.h"
 
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

#define A4_PAPER_HEIGHT	210
#define A4_PAPER_WIDTH	297

#define C_FNAME "/tmp/tmpprintfile.ps"


void exec_command ( char * command )  ;


// This is all here as it is needed throughout the whole of the following classes

IO_handle stream;
char buffer[ 200 ] ;

void PSwrite(const char *sptr);
void PSrawwrite(const char *sptr, uint4 length);




#define NDEFAULT_FONTS     12
#define DEFAULT_FONT_INDEX 3
#define PRINTER_FONT_LEN   200
#define MAX_PATTERNS 200 

static ConstFontTable defaultfonts[NDEFAULT_FONTS] =
    {
        { "Charter", "Courier", "", "-Bold", "-Oblique", "-BoldOblique" },
        { "Clean", "Courier", "", "-Bold", "-Oblique", "-BoldOblique" },
        { "Courier", "Courier", "", "-Bold", "-Oblique", "-BoldOblique" },
        { "Helvetica", "Helvetica", "","-Bold", "-Oblique", "-BoldOblique" },
        { "Lucida", "Courier", "", "-Bold", "-Oblique", "-BoldOblique" },
        { "LucidaBright", "Times", "-Roman", "-Bold", "-Italic", "-BoldItalic" },
        { "LucidaTypewriter", "Courier", "", "-Bold", "-Oblique", "-BoldOblique" },
        { "New Century Schoolbook", "NewCenturySchlbk", "-Roman",
          "-Bold", "-Italic", "-BoldItalic" },
        { "Symbol", "Symbol", "", "", "", "" },
        { "Times", "Times", "-Roman", "-Bold", "-Italic", "-BoldItalic" },
        { "fixed", "Courier", "", "-Bold", "-Oblique", "-BoldOblique" },
        { "terminal", "Courier", "", "-Bold", "-Oblique", "-BoldOblique" }
    };

#define PS_NFUNCS 13

//Define PostScript function definitions

/*
	string1 string2 /str-append
	exch
	1 index length
	dup length
	add string
	% (s2 s1 ns)
	dup dup 5 2 roll
	% (ns ns s2 s1 ns)
	1 index length 3 1 roll
	% (ns ns s2 len s1 ns)
	copy pop exch
	% (ns ns len s2)
	putinterval
	% (ns)



	% string1 string2 newstring

	<size> <name-str> /setFont
	dup (-ISO) str-append cvn dup
	{ findfont } stopped
	{
		% (size name-str name-iso)
		% not found so search for original and re-encode
		exch cvn
		% (size name-iso name)
		{ findfont } stopped
		{
			% ( size name-iso )
			% not found so use Helvetica
			pop
			/Helvetica findfont

			% (size font)
		}
		{
			% ( size name-iso font )
			% found so re-encode
			dup length dict begin { 1 index /FID ne { def } { pop pop } ifelse } forall
			/Encoding ISOLatin1Encoding def
			currentdict
			end
			% (size name-iso new-font)
			exch definefont
			% (size font)
		}
	}
	{
		% (size name-str name-iso font-dict)
		% found so continue after popping name
		exch
		pop
	}
	ifelse

	% (size font-dict)
	exch
	scalefont setfont

*/

static const char *PSfuncs[PS_NFUNCS] =
    {
        "%%BeginProlog\n" ,
		
		"%usage: width height LLx LLy FR  Fill-Rectangle\n\
        /FR { %def\n\
        gsave\n\
        newpath\n\
        moveto\n\
        1 index 0 rlineto\n\
        0 exch rlineto\n\
        neg 0 rlineto\n\
        closepath fill\n\
        grestore\n\
        } bind def\n\
        \n" ,


        "%usage: width height LLx LLy R    Rectangle routine\n\
        /R { %def\n\
        gsave\n\
        newpath\n\
        moveto \n\
        1 index 0 rlineto\n\
        0 exch rlineto\n\
        neg 0 rlineto\n\
        closepath\n\
        stroke\n\
        grestore\n\
        } bind def\n",
		
        "%usage: topLeftx, topLefty, width, height, radius  FRR\n\
        /FRR { %def  (tlx tly w h r --)  Fill Rounded Rectangle\n\
        gsave\n\
        matrix currentmatrix\n\
        newpath 6 1 roll 5 -2 roll translate\n\
        dup 0 exch neg moveto\n\
        mark 0 3 index neg 5 index 5 index neg 5 index arcto cleartomark\n\
        mark 3 index 3 index neg 5 index 0 5 index arcto cleartomark\n\
        mark 3 index 0 0 0 5 index arcto cleartomark\n\
        mark 0 0 0 5 index neg 5 index arcto cleartomark\n\
        closepath pop pop pop fill setmatrix\n\
        grestore\n\
        } bind def\n",
		
        "%usage: topLeftx, topLefty, width, height, radius  RR\n\
        /RR { %def  (tlx tly w h r --)  Draw Rounded Rectangle\n\
        gsave\n\
        matrix currentmatrix\n\
        newpath 6 1 roll 5 -2 roll translate\n\
        dup 0 exch neg moveto\n\
        mark 0 3 index neg 5 index 5 index neg 5 index arcto cleartomark\n\
        mark 3 index 3 index neg 5 index 0 5 index arcto cleartomark\n\
        mark 3 index 0 0 0 5 index arcto cleartomark\n\
        mark 0 0 0 5 index neg 5 index arcto cleartomark\n\
        closepath  pop pop pop  stroke setmatrix\n\
        grestore\n\
        } bind def\n",
		
        "%usage: width height LLx LLy CLP  set clipping rectangle\n\
        /CLP { %def \n\
        newpath moveto\n\
        1 index 0 rlineto\n\
        0 exch rlineto\n\
        neg 0 rlineto\n\
        closepath clip\n\
        } bind def\n\
        \n\
        %usage: tx ty fx fy L\n\
        /L { %def\n\
        gsave newpath\n\
        moveto lineto stroke\n\
        grestore\n\
        } bind def\n",
		
        "%usage: pn.x pn.y ... p1.x p1.y Count  LS %%print a line w/points \n\
        /LS { %def Draws connected line segments\n\
        gsave newpath\n\
        3 1 roll\n\
        moveto\n\
        {lineto} repeat stroke\n\
        grestore\n\
        } bind def\n\
        \n" ,

        "%usage: pn.x pn.y ... p1.x p1.y Count  CLS %%print a line w/points \n\
        /CLS { %def Draws connected line segments AND CLOSE path\n\
        gsave newpath\n\
        3 1 roll\n\
        moveto\n\
        {lineto} repeat \n\
        closepath\n\
        stroke\n\
        grestore\n\
        } bind def\n" ,
		
        "%usage: (Hello!) 72 512 T\n\
        /T { %def\n\
        gsave moveto show\n\
        grestore\n\
        } bind def\n\
        \n\
        %parameters: # of char -1, ScreenTextWidth, string, x, y, AT(Ajusted Text)\n\
        %usage: 120 8 (Hello!) 72 512 AT\n\
        /AT { %def\n\
        gsave moveto\n\
        dup                        %duplicate a copy of the String\n\
        4 1 roll                   %move one copy of string to the bottom of stack\n\
        stringwidth pop            %get PS string width and pop the Height out(no need)\n\
        sub \n\
        exch\n\
        div \n\
        0 3 2 roll ashow\n\
        grestore\n\
        } bind def\n\
        \n"

		"/str-append \
		{ %s1 s2 /str-append => (s1s2)\n \
		exch \
		1 index length \
		1 index length \
		add string \
		% (s2 s1 ns) \n\
		dup dup 5 2 roll \
		% (ns ns s2 s1 ns) \n\
		1 index length 3 1 roll \
		% (ns ns s2 len s1 ns) \n\
		copy pop exch \
		% (ns ns len s2) \n\
		putinterval \
		% (ns) \n \
		} bind def\n"

		"/setFont \
		{\
		dup (-ISO) str-append cvn dup % (size name-str name-iso name-iso) \n\
		\
		{ FontDirectory exch get } stopped not \
		\
		{ \
			% (size name-str name-iso font-dict) \n\
			% found so continue after popping name \n\
			3 1 roll \
			pop pop \
		} \
		{ \
			% not found so search for original and re-encode \n\
			pop pop exch cvn \
			% (size name-iso name) \n\
			{ findfont } stopped \
			{ \
				% ( size name-iso ) \n\
				% not found so use Helvetica \n\
				pop \
				/Helvetica findfont \
				% (size font) \n\
			} \
			{\
				% ( size name-iso font ) \n\
				% found so re-encode \n\
				dup length dict begin { 1 index /FID ne { def } { pop pop } ifelse } forall \
				/Encoding ISOLatin1Encoding def \
				currentdict \
				end \
				% (size name-iso new-font) \n\
				definefont \
				% (size font) \n\
			} \
			ifelse \
		} \
		ifelse \
		\
		% (size font-dict) \n\
		exch \
		scalefont setfont \
		} bind def \n"

		"%usage: 10 /Times-Roman F\n\
        /F { %def\n\
        findfont exch scalefont setfont\n\
        } bind def\n\
        \n" ,


        "%usage: radius startAngle arcAngle yScale centerX centerY FA \n\
        %this routine fills an Arc\n\
        /FA { %def\n\
        gsave newpath\n\
        translate 1 scale\n\
        0 0 moveto\n\
        0 0 5 2 roll arc\n\
        closepath fill grestore\n\
        } bind def\n", 
		
        "%usage: invyscale 0 0 radius starAngle arcAngle yScale centerX centerY DA \n\
        %this routine draws and an Arc\n\
        /DA { %def\n\
        gsave newpath\n\
        translate 1 scale\n\
        arc 1 scale stroke grestore\n\
        } bind def\n\
        \n", 

		"%usage: width, height, 8 rlecmapimage \n\
        %un-RLE image, then convert colormap entries to RGB values\n\
        /rlecmapimage {\n\
        /buffer 1 string def\n\
        /rgbval 3 string def\n\
        /block 384 string def\n\
        { currentfile buffer readhexstring pop\n\
        /bcount exch 0 get store\n\
        bcount 128 ge\n\
        {\n\
        0 1 bcount 128 sub\n\
        { currentfile buffer readhexstring pop pop\n\
        /rgbval cmap buffer 0 get 3 mul 3 getinterval store\n\
        block exch 3 mul rgbval putinterval\n\
        } for\n\
        block 0 bcount 127 sub 3 mul getinterval\n\
        }\n\
        {\n\
        currentfile buffer readhexstring pop pop\n\
        /rgbval cmap buffer 0 get 3 mul 3 getinterval store\n\
        0 1 bcount { block exch 3 mul rgbval putinterval} for\n\
        block 0 bcount 1 add 3 mul getinterval\n\
        } ifelse\n\
        }\n\
        false 3 colorimage\n\
        } bind def\n", 
        
		"%%EndProlog\n"
    };


class MCPSMetaContext : public MCMetaContext
{
public:
	MCPSMetaContext(MCRectangle& p_rect);
	~MCPSMetaContext(void);


protected:
	bool candomark(MCMark *p_mark);
	void domark(MCMark *p_mark);
	bool begincomposite(const MCRectangle &p_region, MCGContextRef &r_context);
	void endcomposite(MCRegionRef p_clip_region);

private:

	
	uint4 cardheight, cardwidth ; 
	bool onPattern ;
	
	// Optimizations so we only switch fonts or render new colors when we need to.
	MCFontStruct *oldfont ;
	MCColor oldcolor ;
	
	
	MCGImageRef f_pattern_pixmaps[MAX_PATTERNS];
	uint2 f_pattern_count ;

	void drawtext(MCMark * p_mark );
	void setfont(MCFontStruct *font) ;
	void printpattern(const MCGRaster &image) ;
	void printraster(const MCGRaster &p_raster, int16_t p_dx, int16_t p_dy, real64_t p_xscale, real64_t p_yscale);
	void printimage(MCImageBitmap *image, int2 dx, int2 dy, real8 xscale, real8 yscale) ;
	void write_scaling(const MCRectangle &rect) ;

	bool pattern_created( MCGImageRef p_pattern ) ;
	void fillpattern ( MCPatternRef p_pattern, MCPoint p_origin ) ;
	void create_pattern ( MCGImageRef p_pattern );

	MCRectangle m_composite_rect;
	MCGContextRef m_composite_context;
};


///////////////////////////////////////////////////////////////////////////////////////////////
//
//                            P S P R I N T E R D E V I C E
//
//////////////////////////////////////////////////////////////////////////////////////////////


MCPSPrinterDevice::MCPSPrinterDevice(void)
{
	m_error = NULL;
	m_page_started = false;

	page_count = 1;
}

MCPSPrinterDevice::~MCPSPrinterDevice(void)
{
	delete m_error;
}
 

const char *MCPSPrinterDevice::Error(void) const
{
	return m_error ;
}


MCPrinterResult MCPSPrinterDevice::Cancel(void)
{
	return ( PRINTER_RESULT_CANCEL ) ;
}


MCPrinterResult MCPSPrinterDevice::Show(void)
{
	if (!m_page_started)
		BeginPage();
	
	EndPage() ;
	
	return ( PRINTER_RESULT_SUCCESS ) ;
}




void MCPSPrinterDevice::BeginPage(void)
{

	page_count++;
	sprintf(buffer, "%%%%Page: Revolution %d\n", page_count);
	PSwrite ( buffer ) ;
	PSwrite("1.3 setlinewidth\n");
	m_page_started = true;
}



void MCPSPrinterDevice::EndPage(void)
{
	PSwrite("%%PageTrailer\n");
	PSwrite("showpage\n");
	
	m_page_started = false;
}



MCPrinterResult MCPSPrinterDevice::Begin(const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, MCContext*& r_context)
{
	
	
	MCPSMetaContext *t_context;
	
	real8 trans_x, trans_y ;
	real8 scale_x, scale_y ;
	real8 swidth, sheight, dwidth, dheight ;
	
	uint4 t_pageheight = MCprinter->GetPageHeight() ;
	
	dwidth =  p_dst_rect . right - p_dst_rect . left ;
	dheight = p_dst_rect . bottom - p_dst_rect . top ;
	swidth =  p_src_rect . right - p_src_rect . left ;
	sheight = p_src_rect . bottom - p_src_rect . top ;
	trans_x = p_dst_rect . left - p_src_rect . left ;
 	trans_y = p_dst_rect . top - p_src_rect . top ;
	scale_x = dwidth / swidth ;
	scale_y = dheight / sheight ;
	
	if (!m_page_started)
		BeginPage();

	PSwrite("grestore\ngsave\nmatrix\n");

	sprintf(buffer, "%G %G scale\n", MCprinter->GetPageScale(), MCprinter->GetPageScale());
	PSwrite(buffer);
	
	sprintf(buffer, "%G %G translate\n", p_dst_rect.left, t_pageheight - p_dst_rect.bottom);
	PSwrite(buffer);

	sprintf(buffer, "%G %G scale\n", scale_x, scale_y);
	PSwrite(buffer);
	
	sprintf(buffer, "%G %G translate\n", - p_src_rect.left, - p_src_rect.top);
	PSwrite(buffer);
	

	MCRectangle t_src_mcrect;
	

	
	t_src_mcrect . x = (int2)(p_src_rect . left + 0.5);
	t_src_mcrect . y = (int2)(p_src_rect . top + 0.5);
	t_src_mcrect . width = (int2)((p_src_rect . right - p_src_rect . left) + 0.5);
	t_src_mcrect . height = (int2)((p_src_rect . bottom - p_src_rect . top) + 0.5);

	t_context = new MCPSMetaContext( t_src_mcrect );
	
	r_context = t_context;
	
	return ( PRINTER_RESULT_SUCCESS ) ;
}

MCPrinterResult MCPSPrinterDevice::End(MCContext *p_context)
{
	PSwrite("grestore\n");
	
	MCPSMetaContext *t_metacontext;
	t_metacontext = static_cast<MCPSMetaContext *>(p_context) ;
	t_metacontext -> execute();
	
	delete t_metacontext ;
	
	return ( PRINTER_RESULT_SUCCESS ) ;
}

MCPrinterResult MCPSPrinterDevice::Anchor(const char *name, double x, double y)
{
	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCPSPrinterDevice::Link(const char *name, const MCPrinterRectangle& area, MCPrinterLinkType type)
{
	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCPSPrinterDevice::Bookmark(const char *title, double x, double y, int depth, bool closed)
{
	return PRINTER_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
//                                P S P R I N T E R  
//
//////////////////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_PRINTER_SCRIPT "return the last word of shell(\"lpstat -d\")"


char * getdefaultprinter(void)
{
	MCExecPoint ep ( NULL, NULL, NULL ) ;
	MCdefaultstackptr->domess(DEFAULT_PRINTER_SCRIPT);
	MCresult->fetch(ep);
	return strdup(ep.getcstring());
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
}

void MCPSPrinter::DoFinalize(void)
{
	
	if ( m_printersettings . printername != NULL ) 
		delete m_printersettings . printername ;

	if ( m_printersettings . outputfilename != NULL ) 
		delete (m_printersettings . outputfilename -7);	// Need to subtract 7 here as we added 7 to skip the "file://" part
	
	if ( m_printersettings . page_ranges != NULL)
		delete m_printersettings . page_ranges ;
	
}


bool MCPSPrinter::DoReset(const char *p_name)
{
	if ( p_name != NULL ) 
		m_printersettings . printername = strdup(p_name);
	
	FlushSettings();
	
	// MDW-2013-04-16: [[ x64 ]] DoReset needs to return a bool
	return true;
}


bool MCPSPrinter::DoResetSettings(const MCString& p_settings)
{
	bool t_success;
	t_success = true;

	MCDictionary t_dictionary;
	if (t_success)	
		t_success = t_dictionary . Unpickle(p_settings . getstring(), p_settings . getlength());

	MCString t_name;
	if (t_success)
		t_success = t_dictionary . Get('NMEA', t_name);
		
	if ( t_success ) 
			m_printersettings . printername = t_name.clone() ;
 
	return t_success;
	
}

void MCPSPrinter::DoFetchSettings(void*& r_buffer, uint4& r_length)
{
	bool t_success;
	t_success = true;

	MCDictionary t_dictionary;
	
	if ( m_printersettings . printername != NULL ) 
		t_dictionary . Set('NMEA', MCString(m_printersettings . printername , strlen(m_printersettings . printername ) + 1 ) ) ;
	
	
	if (t_success)
		t_dictionary . Pickle(r_buffer, r_length);
	else
	{
		r_buffer = NULL;
		r_length = 0;
	}
}


const char *MCPSPrinter::DoFetchName(void)
{
	return m_printersettings . printername  ;
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



MCPrinterResult MCPSPrinter::DoBeginPrint(const char *p_document, MCPrinterDevice*& r_device)
{
	MCPSPrinterDevice *t_device = new MCPSPrinterDevice ;
	
	const char *t_output_file;
	if (GetDeviceOutputType() == PRINTER_OUTPUT_FILE)
		t_output_file = GetDeviceOutputLocation();
	else
		t_output_file = C_FNAME;

	stream = MCS_open(t_output_file, IO_CREATE_MODE, False, False, 0);

	PSwrite("%!PS-Adobe-3.0\n");
	sprintf(buffer, "%%%%Creator: Revolution %s\n", MCversionstring); PSwrite(buffer);
	PSwrite("%%DocumentData: Clean8Bit\n");
	sprintf(buffer, "%%%%Title: %s\n", p_document ) ; PSwrite(buffer ) ;
	PSwrite("%%MCOrientation Portrait\n");
	PSwrite("%%EndComments\n");
	
	// Dump out all our PostScript functions for short hand coding.
	for ( int i = 0; i < PS_NFUNCS; i++)
		PSwrite ( PSfuncs [ i ]  );
	
	r_device = t_device ;
	
	return ( PRINTER_RESULT_SUCCESS ) ;
}



MCPrinterResult MCPSPrinter::DoEndPrint(MCPrinterDevice* p_device)
{		

	p_device ->  Show();
	PSwrite("%%EndDocument\n");
	PSwrite("%%EOF\n");
	MCS_close( stream ) ;
	
	// Need to sync the setting between the engine and our copy of the job etc
	SyncSettings ();
	
	if (GetDeviceOutputType() == PRINTER_OUTPUT_DEVICE)
	{
		// OK - lets build up the command line
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
		

		//sprintf(buffer, "ggv %s\n", C_FNAME);
		
		if (GetDeviceCommand() != NULL) 
			sprintf(buffer, GetDeviceCommand(), C_FNAME ) ;
		
		//fprintf(stderr, "Print command : [%s]\n", buffer ) ;
		exec_command ( buffer ) ; 
	}
	
	if ( p_device != NULL)
		delete p_device ;
	
	return ( PRINTER_RESULT_SUCCESS ) ;
}



void MCPSPrinter::FlushSettings ( void ) 
{
	SetPageSize ( m_printersettings . paper_size_width, m_printersettings . paper_size_height ) ;
	SetPageOrientation ( m_printersettings . orientation ) ;
	
	SetJobDuplex ( m_printersettings . duplex_mode ) ;
	SetJobCopies ( m_printersettings . copies ) ;
	SetJobCollate ( m_printersettings . collate ) ;
	
	if ( m_printersettings . outputfilename != NULL ) 
		SetDeviceOutput( m_printersettings . printertype, m_printersettings . outputfilename ) ;
	
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




//{ Folding
///////////////////////////////////////////////////////////////////////////////////////////////
//
//                                M E T A C O N T E X T 
//
//////////////////////////////////////////////////////////////////////////////////////////////




MCPSMetaContext::MCPSMetaContext ( MCRectangle& p_rect ) : MCMetaContext(p_rect)
{
	write_scaling ( p_rect ) ;
	sprintf(buffer, "gsave\n%d %d %d %d CLP\n", p_rect . width  , p_rect . height , p_rect . x,  p_rect . y ) ;
	PSwrite ( buffer ) ;
	
	f_pattern_count = 0 ;
	
	oldcolor.red = 0 ;
	oldcolor.green = 0 ;
	oldcolor.blue = 0 ;
	
	oldfont = NULL ;
	
	onPattern = False ;
		

}


MCPSMetaContext::~MCPSMetaContext(void)
{
	
}

bool MCPSMetaContext::candomark(MCMark *p_mark)
{
	/* OVERHAUL - REVISIT: supporting transformed images requires more work */
	if (p_mark -> type == MARK_TYPE_IMAGE && p_mark->image.descriptor.has_transform)
		return false;
		
	return p_mark -> type != MARK_TYPE_GROUP;
}

void MCPSMetaContext::domark(MCMark *p_mark)
{
	bool isFilled ;
	bool isStroke ;
	MCColor color ;

	isFilled = ( p_mark -> fill != NULL )  ;
	isStroke = ( p_mark -> stroke != NULL ) ; 
	
	if ( isStroke )
	{
		if (p_mark->stroke->width == 0)
		{
			PSwrite("0 setlinejoin\n0 setlinecap\n1 setlinewidth\n");
		}
		else
		{
			switch (p_mark->stroke->join)
			{
			case JoinMiter:
				PSwrite("0 setlinejoin\n");
				break;
			case JoinRound:
				PSwrite("1 setlinejoin\n");
				break;
			case JoinBevel:
				PSwrite("2 setlinejoin\n");
				break;
			}
			switch (p_mark->stroke->cap)
			{
			case CapButt:
				PSwrite("0 setlinecap\n");
				break;
			case CapRound:
				PSwrite("1 setlinecap\n");
				break;
			case CapProjecting:
				PSwrite("2 setlinecap\n");
				break;
			}
			sprintf(buffer, "%d setlinewidth\n", p_mark->stroke->width);
			PSwrite(buffer);
		}
	}
	if ( isFilled ) 
	{
		if (p_mark -> fill -> style == FillTiled)
		{
			fillpattern( p_mark -> fill -> pattern, p_mark -> fill -> origin )  ;
			onPattern = True ;
		}
		else if ( onPattern )
		{
			PSwrite ("0 setgray\n");
			onPattern = False ;
		}
		


		if ( (( p_mark -> fill -> colour . red != oldcolor . red ) ||
			( p_mark -> fill -> colour . green != oldcolor . green ) ||
			( p_mark -> fill -> colour . blue != oldcolor . blue )) && 
			!onPattern )
		{
			real8 r = (real8)p_mark -> fill -> colour.red / (real8)MAXUINT2;
			real8 g = (real8)p_mark -> fill -> colour.green / (real8)MAXUINT2;
			real8 b = (real8)p_mark -> fill -> colour.blue / (real8)MAXUINT2;
			sprintf(buffer, "%G %G %G setrgbcolor\n", r, g, b);
			PSwrite(buffer);
			
			oldcolor . red = p_mark -> fill -> colour . red ;
			oldcolor . green = p_mark -> fill -> colour . green ;
			oldcolor . blue = p_mark -> fill -> colour . blue ;
			
		}
		
	}
	
	
	switch(p_mark -> type)
	{
		case MARK_TYPE_LINE:
			sprintf( buffer, "%d %d %d %d L\n", p_mark -> line . start . x , 
												cardheight - p_mark -> line . start . y ,
												p_mark -> line . end . x ,
												cardheight - p_mark -> line . end . y ) ;
			PSwrite ( buffer ) ;
		break;
		

		case MARK_TYPE_POLYGON:
			PSwrite("gsave newpath\n");

			sprintf(buffer, "%d %d moveto\n", p_mark -> polygon . vertices[0] . x, cardheight - p_mark -> polygon . vertices[0] . y );
			PSwrite(buffer);

			uint2 i;
			for (i = 1 ; i < p_mark -> polygon . count ; i++)
			{
				sprintf(buffer, "%d %d lineto\n", p_mark -> polygon . vertices[i] . x, cardheight - p_mark -> polygon . vertices[i] . y);
				PSwrite(buffer);
			}
		if ( isStroke ) 
			PSwrite("stroke grestore\n");
		else 
			PSwrite("closepath fill grestore\n");

		break;
		
		case MARK_TYPE_TEXT:
			drawtext ( p_mark ) ;
		break;
		
		
		
		case MARK_TYPE_RECTANGLE:
			if (isStroke)
			{
				MCRectangle t_rect;
				t_rect = p_mark->rectangle.bounds;
			
				sprintf( buffer, "%d %d %d %d R \n", t_rect . width, t_rect . height, t_rect . x , cardheight - (t_rect.y + t_rect.height) ) ;
				PSwrite ( buffer ) ;
				
			}
			else
			{
				if ( isFilled ) 
					sprintf( buffer, "%d %d %d %d FR \n", p_mark -> rectangle .bounds. width , p_mark -> rectangle . bounds . height , 
														  p_mark -> rectangle . bounds . x ,
														  cardheight - ( p_mark -> rectangle . bounds . y + p_mark -> rectangle . bounds . height ) );
				else
					sprintf( buffer, "%d %d %d %d R \n", p_mark -> rectangle .bounds. width , p_mark -> rectangle . bounds . height , 
														  p_mark -> rectangle . bounds . x ,
														  cardheight - ( p_mark -> rectangle . bounds . y + p_mark -> rectangle . bounds . height ) );
				PSwrite ( buffer ) ;
			}

		break;
		
		
		
		case MARK_TYPE_ROUND_RECTANGLE:
			
			//%usage: topLeftx, topLefty, width, height, radius  FRR
			if ( !isStroke ) 
				sprintf( buffer, "%d %d %d %d %d FRR \n",  p_mark -> round_rectangle . bounds. x , 
													   	cardheight - ( p_mark -> round_rectangle . bounds . y  ),  //+ p_mark -> round_rectangle . bounds . height
														p_mark -> round_rectangle . bounds . width,
													   	p_mark -> round_rectangle . bounds . height,
														p_mark -> round_rectangle . radius);
			else
				sprintf( buffer, "%d %d %d %d %d RR \n",   p_mark -> round_rectangle . bounds. x , 
													   	cardheight - ( p_mark -> round_rectangle . bounds . y ) , 
														p_mark -> round_rectangle . bounds . width ,
													   	p_mark -> round_rectangle . bounds . height ,
														p_mark -> round_rectangle . radius);
		
			PSwrite ( buffer ) ;

		break;
		
		case MARK_TYPE_ARC:
			uint4 t_x, t_y, t_r, t_rw ;
			uint4 t_width, t_height ;
			
			t_x = p_mark -> arc . bounds . x;
			t_y = p_mark -> arc . bounds . y ;
			t_width = p_mark -> arc . bounds . width ;
			t_height = p_mark -> arc . bounds . height ;
		
			t_r = (t_height / 2.0 );
			t_rw = ( t_width / 2.0 ) ;
		
		
			if ( isStroke ) 
				sprintf(buffer, "%g 0 0 %d %d %d %g %d %d DA\n", (real8)t_height / (real8)t_width ,
																 t_height >> 1,
																 p_mark -> arc . start,
																 p_mark -> arc . angle + p_mark -> arc . start, 
																 (real8)t_width / (real8)t_height, 
																 t_x + t_rw ,
																 cardheight - ( t_y + t_r )  ) ;				
			else			
				sprintf(buffer, "%d %d %d %g %d %d FA\n", t_height >>1 , 
														  p_mark -> arc . start ,
 														  p_mark -> arc . angle + p_mark -> arc . start, 
														  (real8) t_width / (real8)t_height, 
						 								  t_x + t_rw ,
														  cardheight - ( t_y + t_r )  ) ;		
		
			PSwrite ( buffer ) ;
			
			if ( ( p_mark -> arc . complete ) && ( p_mark -> arc . angle < 360 ) )
			{
				int2 cx = t_x + t_rw ;
				int2 cy = cardheight - ( t_y + t_r ) ;
				
				real8 torad = M_PI * 2.0 / 360.0;
				
				real8 tw = (real8)p_mark -> arc . bounds . width;
				real8 th = (real8)p_mark -> arc . bounds . height;
				
				real8 sa = (real8)p_mark -> arc . start * torad;
				
				int2 dx = cx + (int2)(cos(sa) * tw / 2.0);
				int2 dy = cy + (int2)(sin(sa) * th / 2.0);
				
				sprintf(buffer, "%d %d %d %d L\n", cx, cy, dx, dy ) ;
				PSwrite ( buffer ) ;

				sa = (real8)(p_mark -> arc . start + p_mark -> arc . angle) * torad;
				dx = cx + (int2)(cos(sa) * tw / 2.0);
				dy = cy + (int2)(sin(sa) * th / 2.0);
				
				sprintf(buffer, "%d %d %d %d L\n", cx, cy, dx, dy ) ;
				PSwrite ( buffer ) ;
			}
		
		
			
		
		break;
		
		
		
		case MARK_TYPE_IMAGE:
		{
			uint2 sx, sy, dx, dy, sw, sh, dw, dh ;
			sx = p_mark -> image . sx ;
			sy = p_mark -> image . sy ;
			sw = p_mark -> image . sw ;
			sh = p_mark -> image . sh ;
			
			dx = p_mark -> image . dx ;
			dy = p_mark -> image . dy ;
			
			MCRectangle t_src_rect;
			MCU_set_rect(t_src_rect, sx, sy, sw, sh);
			
			MCImageBitmap *t_image = nil;
			/* UNCHECKED */ MCImageCopyBitmapRegion(p_mark->image.descriptor.bitmap, t_src_rect, t_image);
			printimage ( t_image, dx, dy, 1.0, 1.0);
			MCImageFreeBitmap(t_image);
		}
		break;
		
		case MARK_TYPE_METAFILE:
			// UNSUPPORTED
		break;
	}
	
}

#define SCALE 4

bool MCPSMetaContext::begincomposite(const MCRectangle &p_region, MCGContextRef &r_context)
{
	bool t_success = true;
	
	MCGContextRef t_context = nil;

	uint4 t_scale = SCALE;
	
	uint32_t t_width, t_height;
	t_width = p_region . width * t_scale;
	t_height = p_region . height * t_scale;

	if (t_success)
		t_success = MCGContextCreate(t_width, t_height, true, t_context);

	if (t_success)
{
		MCGContextScaleCTM(t_context, t_scale, t_scale);
		MCGContextTranslateCTM(t_context, -(MCGFloat)p_region . x, -(MCGFloat)p_region . y);

		m_composite_context = t_context;
		m_composite_rect = p_region;

		r_context = m_composite_context;
	}
	else
		MCGContextRelease(t_context);
	
	return t_success;
}

void MCPSMetaContext::endcomposite(MCRegionRef p_clip_region)
{
	uint4 t_scale = SCALE;
	
	MCGImageRef t_image;
	t_image = nil;
	
	/* UNCHECKED */ MCGContextCopyImage(m_composite_context, t_image);
	
	MCGRaster t_raster;
	MCGImageGetRaster(t_image, t_raster);
	
	printraster(t_raster, m_composite_rect.x, m_composite_rect.y, t_scale, t_scale);
	
	MCGImageRelease(t_image);

	MCGContextRelease(m_composite_context);
	m_composite_context = nil;
	
	if ( p_clip_region != NULL )
		MCRegionDestroy(p_clip_region);
}





void MCPSMetaContext::drawtext(MCMark * p_mark )
{
	uint4 x = p_mark -> text . position . x ;
	uint4 y = p_mark -> text . position . y ;
	uint2 l = p_mark -> text . length ;
	MCFontStruct *f = p_mark -> text . font ;

	if ( f != oldfont )
	{
		setfont ( f ) ;
		oldfont = f ;
	}
	
	if (l == 0)
		return;

	char *newsptr = NULL;
	char *text = new char[l + 1];
	memcpy(text, p_mark -> text . data , l);
	
	uint2 w = MCscreen->textwidth(f, text, l);
	
	
	text[l] = '\0';
	const char *sptr = text;
	if ((sptr = strpbrk(text, "()\\")) != NULL)
	{
		uint2 count = 0;
		while (sptr != NULL)
		{
			sptr = strpbrk(sptr + 1, "()\\");
			count++;
		}
		newsptr = new char[strlen(text) + count + 1];
		char *dptr = newsptr;
		sptr = text;
		uint2 ilength = l;
		while (ilength--)
		{
			if (*sptr == '(' || *sptr == ')' || *sptr == '\\')
			{
				*dptr++ = '\\';
				l++;
			}
			*dptr++ = *sptr++;
		}
		sptr = newsptr;
	}
	else
		sptr = text;

	
	if (l == 1)
		sprintf(buffer, "(%1.*s) %d %d T\n", (int)l, sptr, x, cardheight - y);
	else
		sprintf(buffer, "%d %d (%1.*s) %d %d AT\n",
		        l - 1, w, (int)l, sptr, x, cardheight - y );
	delete text;
	if (newsptr != NULL)
		delete newsptr;
	PSwrite(buffer);
}




void MCPSMetaContext::write_scaling(const MCRectangle &rect)
{
	cardheight = rect.height ;
	cardwidth = rect.width ;
	
	real8 scale = 0.75 ;
	
	
}

// IM-2013-08-12: [[ ResIndependence ]] refactor bitmap printing to printraster method
// *NOTE* currently assumes raster is xRGB.
void MCPSMetaContext::printraster(const MCGRaster &p_raster, int16_t dx, int16_t dy, real64_t xscale, real64_t yscale)
{
	MCColor c;
	uint2 x, y;
	uint4 charCount = 0;

	bool cmapdone = False ;
	
	sprintf(buffer, "/tmp %d string def\n%d %d %d\n",
			p_raster.width * 3, p_raster.width, p_raster.height, 8);
			
	PSwrite(buffer);
	sprintf(buffer, "[ %g 0 0 -%g %g %g ]\n", xscale, yscale, -dx * xscale,
			(cardheight - dy) * yscale);
	PSwrite(buffer);
	PSwrite("{currentfile tmp readhexstring pop}\nfalse 3\ncolorimage\n");

	uint8_t *t_src_row = (uint8_t*)p_raster.pixels;
	for (y = 0 ; y < p_raster.height ; y++)
	{
		uint32_t *t_src_ptr = (uint32_t*)t_src_row;
		for (x = 0 ; x < p_raster.width ; x++)
		{
			uint8_t r, g, b, a;
			MCGPixelUnpackNative(*t_src_ptr++, r, g, b, a);
			
			sprintf(buffer, "%02X%02X%02X", r, g, b);
			PSwrite(buffer);
			charCount += 6;
			
			if (charCount % 78 == 0)
				PSwrite("\n");
		}
		t_src_row += p_raster.stride;
	}
	PSwrite("\n");
}

void MCPSMetaContext::printimage(MCImageBitmap *p_image, int16_t dx, int16_t dy, real64_t xscale, real64_t yscale)
{
	MCGRaster t_raster;
	t_raster.width = p_image->width;
	t_raster.height = p_image->height;
	t_raster.pixels = p_image->data;
	t_raster.stride = p_image->stride;
	t_raster.format = kMCGRasterFormat_xRGB;
	
	printraster(t_raster, dx, dy, xscale, yscale);
}

void MCPSMetaContext::setfont(MCFontStruct *font)
{
	char psFont[PRINTER_FONT_LEN];
	uint2 i = 0;
	uint4 fontcount = NDEFAULT_FONTS ;

	const char * fontname; 
	uint2 fontstyle;
	uint2 fontsize;
	MCFontlistGetCurrent() -> getfontreqs(font, fontname, fontsize, fontstyle);


	while (i < fontcount)
	{
		if (strcasecmp(fontname, defaultfonts[i].fontname) == 0 )
		{
			strcpy(psFont, defaultfonts[i].printerfontname);
			
			if (((fontstyle & FA_WEIGHT) > MCFW_MEDIUM) && (fontstyle & FA_ITALIC))
				strcat(psFont, defaultfonts[i].bolditalic);
			else
				if ((fontstyle & FA_WEIGHT) > MCFW_MEDIUM)
					strcat(psFont, defaultfonts[i].bold);
				else
					if (fontstyle & FA_ITALIC)
						strcat(psFont, defaultfonts[i].italic);
					else
						strcat(psFont, defaultfonts[i].normal);
			break;
		}
		else
			i++;
	}
	if (i == fontcount)
	{
		strcpy(psFont, defaultfonts[DEFAULT_FONT_INDEX].printerfontname);
		if (((fontstyle & FA_WEIGHT) > MCFW_MEDIUM) && (fontstyle & FA_ITALIC))
			strcat(psFont, defaultfonts[DEFAULT_FONT_INDEX].bolditalic);
		else
			if ((fontstyle & FA_WEIGHT) > MCFW_MEDIUM)
				strcat(psFont, defaultfonts[DEFAULT_FONT_INDEX].bold);
			else
				if (fontstyle & FA_ITALIC)
					strcat(psFont, defaultfonts[DEFAULT_FONT_INDEX].italic);
				else
					strcat(psFont, defaultfonts[DEFAULT_FONT_INDEX].normal);
	}

	
	
	sprintf(buffer, "%d (%s) setFont\n", fontsize, psFont);

	PSwrite(buffer);
}

void MCPSMetaContext::printpattern(const MCGRaster &image)
{
	MCColor c;
	uint2 x, y;
	uint4 charCount = 0;
	
	bool colorprint = True ;

	sprintf(buffer, "%d %d 8\n", image.width, image.height);
	PSwrite ( buffer ) ;

	PSwrite ( "[ 1 0 0 1 0 0 ]\n");
	PSwrite ( "<\n");
	
	// print out pattern image bottom-to-top
	uint8_t *t_src_row = (uint8_t*)image.pixels + (image.height - 1) * image.stride;
	y = image.height;
	while (y--)
	{
		uint32_t *t_src_ptr = (uint32_t*)t_src_row;
		for (x = 0 ; x < image.width ; x++)
		{
			uint8_t r, g, b, a;
			MCGPixelUnpackNative(*t_src_ptr++, r, g, b, a);
			if (colorprint)
			{
				sprintf(buffer, "%02X%02X%02X", r, g, b);
				PSwrite(buffer);
				charCount += 6;
			}
			else
			{
				// MDW-2013-04-16: [[ x64 ]] need to compare unsigned with unsigned
				if ((unsigned)(r + g + b) > (unsigned)(MAXUINT1 * 3 / 2))
					PSwrite("F");
				else
					PSwrite("0");
				charCount += 1;
			}
			if (charCount % 78 == 0)
				PSwrite("\n");
		}
		t_src_row -= image.stride;
	}
	if (colorprint)
		PSwrite(">\nfalse 3\ncolorimage\n");
	else
		PSwrite(">\nfalse 1\ncolorimage\n");

	PSwrite("\n");
}







bool MCPSMetaContext::pattern_created( MCGImageRef p_pattern ) 
{
	for ( uint2 i = 0 ; i < f_pattern_count; i++)
		if ( f_pattern_pixmaps[i] == p_pattern ) 
		  return ( true ) ;
	return ( false ) ;
}

// IM-2013-09-04: [[ ResIndependence ]] update fillpattern to take MCPatternRef & apply scale factor
void MCPSMetaContext::fillpattern(MCPatternRef p_pattern, MCPoint p_origin)
{
	if (!pattern_created(p_pattern->image))
		create_pattern(p_pattern->image);
	// MDW-2013-04-16: [[ x64 ]] p_pattern is an XID (long unsigned int), so need $ld here
	sprintf(buffer, "pattern_id_%ld\n", p_pattern->image);
	PSwrite ( buffer );
	sprintf(buffer, "[%f 0 0 %f %d %d]\n", 1.0 / p_pattern->scale, 1.0 / p_pattern->scale, p_origin.x, cardheight - p_origin.y);
	PSwrite(buffer);
	PSwrite("makepattern\n");
	PSwrite("setpattern\n");
}
		
void MCPSMetaContext::create_pattern ( MCGImageRef p_pattern )
{
	int32_t t_w, t_h;
	t_w = MCGImageGetWidth(p_pattern);
	t_h = MCGImageGetHeight(p_pattern);
	
	MCGRaster t_raster;
	MCGImageGetRaster(p_pattern, t_raster);
	
	// MDW-2013-04-16: [[ x64 ]] p_pattern is an XID (long unsigned int), so need $ld here
	sprintf(buffer, "/pattern_id_%ld\n", p_pattern);
	PSwrite ( buffer  ) ;
	
	PSwrite("<<\n");
	PSwrite("/PaintType 1\n");
	PSwrite("/PatternType 1 /TilingType 1\n");
	sprintf(buffer, "/BBox [0 0 %d %d]\n", t_w, t_h ) ; PSwrite ( buffer ) ;
	sprintf(buffer, "/XStep %d /YStep %d\n", t_w, t_h ) ; PSwrite ( buffer ) ;
	PSwrite("/PaintProc {\n") ;
	PSwrite("begin\n");
	printpattern(t_raster);
	PSwrite("end\n");
	PSwrite("}\n");
	PSwrite(">>\n");
	PSwrite ( "def\n"  ) ;
	
	f_pattern_count ++ ;
	f_pattern_pixmaps [ f_pattern_count ] = p_pattern ; // Mark this pattern as being created already
}




///////////////////////////////////////////////////////////////////////////////////////////////
//
//                            Utility Functions & Misc.
//
//////////////////////////////////////////////////////////////////////////////////////////////



void exec_command ( char * command ) 
{
	MCExecPoint ep;
	sprintf(ep.getbuffer(strlen(command)), command);
	ep.setstrlen();
	if (MCS_runcmd(ep) != IO_NORMAL)
	{
		MCeerror->add(EE_PRINT_ERROR, 0, 0);
	}
	else
		MCresult->sets(ep.getsvalue());
	
}



void PSwrite(const char *sptr)
{
	PSrawwrite(sptr, strlen(sptr));
}


void PSrawwrite(const char *sptr, uint4 length)
{
	IO_write(sptr, 1, length, stream);
}


