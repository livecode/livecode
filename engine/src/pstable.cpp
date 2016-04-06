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

//
// MCPrinterDC tables
//
#define NDEFAULT_FONTS     12
#define DEFAULT_FONT_INDEX 3
#define PRINTER_FONT_LEN   200

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


#define PS_NFUNCS 6

//Define PostScript function definitions
static const char *PSfuncs[PS_NFUNCS] =
    {
        "%%BeginProlog\n\
        /isolatin1encoding [ /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /space /exclam\n\
        /quotedbl /numbersign /dollar /percent /ampersand /quoteright\n\
        /parenleft /parenright /asterisk /plus /comma /minus /period /slash\n\
        /zero /one /two /three /four /five /six /seven /eight /nine /colon\n\
        /semicolon /less /equal /greater /question /at /A /B /C /D /E /F /G /H\n\
        /I /J /K /L /M /N /O /P /Q /R /S /T /U /V /W /X /Y /Z /bracketleft\n\
        /backslash /bracketright /asciicircum /underscore /quoteleft /a /b /c\n\
        /d /e /f /g /h /i /j /k /l /m /n /o /p /q /r /s /t /u /v /w /x /y /z\n\
        /braceleft /bar /braceright /asciitilde /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /space /exclamdown /cent /sterling /currency /yen\n\
        /brokenbar /section /dieresis /copyright /ordfeminine /guillemotleft\n\
        /logicalnot /hyphen /registered /macron /degree /plusminus\n\
        /twosuperior /threesuperior /acute /mu /paragraph /periodcentered\n\
        /cedilla /onesuperior /ordmasculine /guillemotright /onequarter\n\
        /onehalf /threequarters /questiondown /Agrave /Aacute /Acircumflex\n\
        /Atilde /Adieresis /Aring /AE /Ccedilla /Egrave /Eacute /Ecircumflex\n\
        /Edieresis /Igrave /Iacute /Icircumflex /Idieresis /Eth /Ntilde\n\
        /Ograve /Oacute /Ocircumflex /Otilde /Odieresis /multiply /Oslash\n\
        /Ugrave /Uacute /Ucircumflex /Udieresis /Yacute /Thorn /germandbls\n\
        /agrave /aacute /acircumflex /atilde /adieresis /aring /ae /ccedilla\n" ,


        "/egrave /eacute /ecircumflex /edieresis /igrave /iacute /icircumflex\n\
        /idieresis /eth /ntilde /ograve /oacute /ocircumflex /otilde\n\
        /odieresis /divide /oslash /ugrave /uacute /ucircumflex /udieresis\n\
        /yacute /thorn /ydieresis ] def\n\
        \n\
        /symbolencoding [ /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /space /exclam\n\
        /universal /numbersign /existential /percent /ampersand /suchthat\n\
        /parenleft /parenright /asteriskmath /plus /comma /minus /period\n\
        /slash /zero /one /two /three /four /five /six /seven /eight /nine\n\
        /colon /semicolon /less /equal /greater /question /congruent /Alpha\n\
        /Beta /Chi /Delta /Epsilon /Phi /Gamma /Eta /Iota /theta1 /Kappa\n\
        /Lambda /Mu /Nu /Omicron /Pi /Theta /Rho /Sigma /Tau /Upsilon /sigma1\n\
        /Omega /Xi /Psi /Zeta /bracketleft /therefore /bracketright\n\
        /perpendicular /underscore /radicalex /alpha /beta /chi /delta\n\
        /epsilon /phi /gamma /eta /iota /phi1 /kappa /lambda /mu /nu /omicron\n\
        /pi /theta /rho /sigma /tau /upsilon /omega1 /omega /xi /psi /zeta\n\
        /braceleft /bar /braceright /similar /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n\
        /.notdef /.notdef /.notdef /Upsilon1 /minute /lessequal /fraction\n\
        /infinity /florin /club /diamond /heart /spade /arrowboth /arrowleft\n\
        /arrowup /arrowright /arrowdown /degree /plusminus /second\n\
        /greaterequal /multiply /proportional /partialdiff /bullet /divide\n\
        /notequal /equivalence /approxequal /ellipsis /arrowvertex\n\
        /arrowhorizex /carriagereturn /aleph /Ifraktur /Rfraktur /weierstrass\n" ,


        "/circlemultiply /circleplus /emptyset /intersection /union\n\
        /propersuperset /reflexsuperset /notsubset /propersubset /reflexsubset\n\
        /element /notelement /angle /gradient /registerserif /copyrightserif\n\
        /trademarkserif /product /radical /dotmath /logicalnot /logicaland\n\
        /logicalor /arrowdblboth /arrowdblleft /arrowdblup /arrowdblright\n\
        /arrowdbldown /lozenge /angleleft /registersans /copyrightsans\n\
        /trademarksans /summation /parenlefttp /parenleftex /parenleftbt\n\
        /bracketlefttp /bracketleftex /bracketleftbt /bracelefttp\n\
        /braceleftmid /braceleftbt /braceex /apple /angleright /integral\n\
        /integraltp /integralex /integralbt /parenrighttp /parenrightex\n\
        /parenrightbt /bracketrighttp /bracketrightex /bracketrightbt\n\
        /bracerighttp /bracerightmid /bracerightbt /.notdef ] def\n\
        \n\
        %ReEncoding, for reencoding character set of a font family\n\
        %It expects three objects(input) on the stack:\n\
        %  [array] /NewName /OldName\n\
        %The array should contain pairs of\n\
        % <numbers> <name>\n\
        /RE {%def\n\
        findfont begin\n\
        currentdict dup length dict begin\n\
        { %forall\n\
        1 index /FID ne {def} {pop pop} ifelse\n\
        } forall\n\
        /FontName exch def dup length 0 ne { %if\n\
        /Encoding Encoding 256 array copy def\n\
        0 exch { %forall\n\
        dup type /nametype eq { %ifelse\n\
        Encoding 2 index 2 index put\n\
        pop 1 add\n\
        } { %else\n\
        exch pop\n\
        } ifelse\n\
        } forall\n\
        } if pop\n\
        currentdict dup end end\n\
        /FontName get exch definefont pop\n\
        } bind def\n\
        \n\
        %usage: width height LLx LLy FR  Fill-Rectangle\n\
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
        } bind def\n\
        \n\
        %usage: topLeftx, topLefty, width, height, radius  FRR\n\
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
        } bind def\n\
        \n\
        %usage: topLeftx, topLefty, width, height, radius  RR\n\
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
        } bind def\n\
        \n\
        %usage: width height LLx LLy CLP  set clipping rectangle\n\
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
        } bind def\n\
        \n\
        %usage: pn.x pn.y ... p1.x p1.y Count  LS %%print a line w/points \n\
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
        } bind def\n\
        \n\
        %usage: (Hello!) 72 512 T\n\
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
        \n\
        %usage: 10 /Times-Roman setFont\n\
        %if findfont command failed, we substitue with Helvetica default font\n\
        %if findfont command does not failed, it may substitute the font specified with\n\
        %system own default, we want to replace it with Helvetica too.\n\
        /setFont { %def\n\
        dup\n\
        { findfont } stopped \n\
        { %execution stopped if can't find specified font\n\
        pop pop /_Helvetica findfont exch scalefont setfont %replace with Helvetica\n\
        }\n\
        { %found the font specified or replace with a system font\n\
        dup                      %dup the font dictionary\n\
        4 1 roll \n\
        /FontName get eq         %compare font name with the font name specified\n\
        {                    %if they are the same, go ahead and set the font\n\
        scalefont setfont\n\
        }\n\
        {      %not the same, system substitued with a font, set our own font\n\
        /_Helvetica findfont exch scalefont setfont pop\n\
        }ifelse\n\
        } ifelse\n\
        } bind def\n\
        \n\
        %usage: 10 /Times-Roman F\n\
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
        } bind def\n\
        \n\
        %usage: invyscale 0 0 radius starAngle arcAngle yScale centerX centerY DA \n\
        %this routine draws and an Arc\n\
        /DA { %def\n\
        gsave newpath\n\
        translate 1 scale\n\
        arc 1 scale stroke grestore\n\
        } bind def\n\
        \n\
        /BeginEPSF { %def\n\
        /b4_Inc_state save def\n\
        /dict_count countdictstack def\n\
        /op_count count 1 sub def\n\
        userdict begin\n\
        /showpage {} def\n\
        /exitserver {} def\n\
        /product (none) def\n\
        /jobname (none) def\n\
        /revision 1 def\n\
        /version 1 def\n\
        /languagelevel 1 def\n\
        /serialnumber 1 def\n\
        /manualfeedtimeout 0 def\n\
        /setjobtimeout {pop} bind def\n\
        /waittimeout 0  def\n\
        /setdefaulttimeouts {pop pop pop} bind def\n\
        /setrealdevice {pop pop} bind def\n\
        /checkpassword {pop true} bind def\n\
        /manualfeed false def\n\
        0 setgray 0 setlinecap\n\
        1 setlinewidth 0 setlinejoin\n\
        10 setmiterlimit [] 0 setdash newpath\n\
        /languagelevel where\n\
        {pop languagelevel\n\
        1 ne\n\
        {false setstrokeadjust false setoverprint\n\
        } if\n\
        } if\n\
        } bind def\n\
        \n\
        /EndEPSF { %def\n\
        count op_count sub {pop} repeat\n\
        countdictstack dict_count sub {end} repeat\n\
        b4_Inc_state restore\n\
        } bind def\n\
        \n\
        %usage: width, height, 8 rlecmapimage \n\
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
        } bind def\n\
        %%EndProlog\n"
    };
