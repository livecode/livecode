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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "util.h"
#include "date.h"
#include "param.h"
#include "execpt.h"
#include "player.h"
#include "group.h"
#include "button.h"
#include "globals.h"
#include "mode.h"
#include "eventqueue.h"
#include "osspec.h"
#include "redraw.h"
#include "field.h"
#include "unicode.h"

#include "osxdc.h"
#include "osxprinter.h"
#include "resolution.h"

#include <Cocoa/Cocoa.h>

////////////////////////////////////////////////////////////////////////////////

TSMDocumentID MCScreenDC::tsmdocument = 0;
AEEventHandlerUPP MCScreenDC::TSMPositionToOffsetUPP;
AEEventHandlerUPP MCScreenDC::TSMOffsetToPositionUPP;
AEEventHandlerUPP MCScreenDC::TSMUpdateHandlerUPP;
AEEventHandlerUPP MCScreenDC::TSMUnicodeNotFromInputUPP;

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::open_textinput(void)
{
#ifdef OLD_MAC
	//TSM - INIT TSM APPLICATION AND INSTALL REQUIRED APPLEVENT HANDLERS
	TSMPositionToOffsetUPP = NewAEEventHandlerUPP(TSMPositionToOffset);
	TSMOffsetToPositionUPP = NewAEEventHandlerUPP(TSMOffsetToPosition);
	TSMUpdateHandlerUPP = NewAEEventHandlerUPP(TSMUpdateHandler);
	TSMUnicodeNotFromInputUPP
	= NewAEEventHandlerUPP(TSMUnicodeNotFromInputHandler);
	AEInstallEventHandler(kTextServiceClass, kPos2Offset,
	                      TSMPositionToOffsetUPP, 0L , False);
	AEInstallEventHandler(kTextServiceClass, kOffset2Pos,
	                      TSMOffsetToPositionUPP, 0L , False);
	AEInstallEventHandler(kTextServiceClass, kUpdateActiveInputArea,
	                      TSMUpdateHandlerUPP, 0L , False);
	AEInstallEventHandler(kTextServiceClass, kUnicodeNotFromInputMethod,
	                      TSMUnicodeNotFromInputUPP, 0L , False);
	openIME();
#endif
}

void MCScreenDC::close_textinput(void)
{
#ifdef OLD_MAC
	//TSM - closes down TSM for this app and removes appleevents
	AERemoveEventHandler(kTextServiceClass, kPos2Offset,
	                     TSMPositionToOffsetUPP, False);
	AERemoveEventHandler(kTextServiceClass, kOffset2Pos,
	                     TSMOffsetToPositionUPP, False);
	AERemoveEventHandler(kTextServiceClass, kUpdateActiveInputArea,
	                     TSMUpdateHandlerUPP, False);
	AERemoveEventHandler(kTextServiceClass, kUnicodeNotFromInputMethod,
	                     TSMUnicodeNotFromInputUPP, False);
	DisposeAEEventHandlerUPP(TSMPositionToOffsetUPP);
	DisposeAEEventHandlerUPP(TSMOffsetToPositionUPP);
	DisposeAEEventHandlerUPP(TSMUpdateHandlerUPP);
	DisposeAEEventHandlerUPP(TSMUnicodeNotFromInputUPP);
	closeIME();
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::openIME()
{
#ifdef OLD_MAC
	if (tsmdocument)
		return;
	InterfaceTypeList supportedTypes;
	if (MCS_imeisunicode())
		supportedTypes[0] = kUnicodeDocument;
	else
		supportedTypes[0] = kTextService;
	NewTSMDocument(1, supportedTypes, &tsmdocument, NULL);
#endif
}

void MCScreenDC::activateIME(Boolean activate)
{
#ifdef OLD_MAC
	if (tsmdocument)
	{
		if (activate)
		{
			ActivateTSMDocument(tsmdocument);
			UseInputWindow(tsmdocument, !MCinlineinput);
		}
		else
			DeactivateTSMDocument(tsmdocument);
	}
#endif
	[[(NSWindow *)(MCactivefield -> getstack() -> getwindow()) contentView] setUseInputMethod: activate == True];
}

void MCScreenDC::clearIME(Window w)
{
#ifdef OLD_MAC
	if (tsmdocument)
		FixTSMDocument(tsmdocument);
#endif
	[[[(NSWindow *)(MCactivefield -> getstack() -> getwindow()) contentView] inputContext] discardMarkedText];
}

void MCScreenDC::closeIME()
{
#ifdef OLD_MAC
	if (!tsmdocument)
		return;
	DeleteTSMDocument(tsmdocument);
	tsmdocument = 0;
#endif
}

pascal  OSErr TSMOffsetToPosition(const AppleEvent *theAppleEvent,
                                  AppleEvent *reply, long handlerRefcon)
{
	OSErr err;
	DescType returnedType;
	Point thePoint = {0,0};
	uint2 offset;
	long actualsize;
	OSErr rterr;
	err = AEGetParamPtr(theAppleEvent, keyAEOffset, typeLongInteger,
	                    &returnedType, &offset, sizeof(offset), &actualsize);
	if (err != noErr)
		return err;
	rterr = noErr;
	if (MCactivefield)
	{
		MCRectangle r;
		Boolean incomposition = MCactivefield->getcompositionrect(r,offset);
		thePoint.h = r.x;
		thePoint.v = r.y+(r.height >> 1);
		if (!incomposition)
			rterr = errOffsetInvalid;
		else
		{
			Window w = MCactivefield->getstack()->getw();
			// COCOA-TODO: TSMOffsetToPosition
#ifdef OLD_MAC
			CGrafPtr oldport;
			GDHandle olddevice;
			
			GetGWorld(&oldport, &olddevice);
			SetGWorld(GetWindowPort((WindowPtr)w->handle.window), GetMainDevice());
			LocalToGlobal(&thePoint);
			SetGWorld(oldport,olddevice);
#endif
		}
	}
	else
		rterr = errOffsetIsOutsideOfView;
	err = AEPutParamPtr(reply, keyAEPoint, typeQDPoint, &thePoint, sizeof(Point));
	err = AEPutParamPtr(reply, keyErrorNumber, typeShortInteger, &rterr, sizeof(rterr));
	return err;
}

pascal OSErr TSMPositionToOffset(const AppleEvent *theAppleEvent,
                                 AppleEvent *reply, long handlerRefcon)
{
	OSErr err;
	DescType returnedType;
	Point thePoint;
	uint4 offset = 0;
	long actualsize;
	short where = kTSMOutsideOfBody;
	//extract position
	err = AEGetParamPtr(theAppleEvent, keyAECurrentPoint, typeQDPoint,
	                    &returnedType, &thePoint, sizeof(thePoint), &actualsize);
	if (err != noErr)
		return err;
	
	err = AEPutParamPtr(reply, keyAEOffset, typeLongInteger,
	                    &offset, sizeof(offset));
	if (err != noErr)
		return err;
	err = AEPutParamPtr(reply, keyAERegionClass, typeShortInteger,
	                    &where, sizeof(where));
	if (err != noErr)
		return err;
	return noErr;
}


pascal OSErr TSMUpdateHandler(const AppleEvent *theAppleEvent,
                              AppleEvent *reply, long handlerRefcon)
{
	OSErr err;
	DescType returnedType;
	AEDesc text,hiliterange;
	long actualsize;
	TextRangeArray *hiliterangeptr = NULL;
	long fixLength;
	AEDesc slr;
	if (MCactivefield == NULL)
		return paramErr;
	ScriptLanguageRecord scriptLangRec;
	err = AEGetParamDesc(theAppleEvent, keyAETSMScriptTag,
	                     typeIntlWritingCode, &slr);
	err = AEGetDescData(&slr, (void *)&scriptLangRec,
	                    sizeof(ScriptLanguageRecord));
	if (err != noErr)
		return err;
	ScriptCode scriptcode = scriptLangRec.fScript;
	AEDisposeDesc(&slr);
	
	//  if (!charset)
	//  return paramErr; //pass event if script is english
	if (!MCactivefield)
		return noErr;
	//get updated IME text
	if (MCS_imeisunicode())
		err = AEGetParamDesc(theAppleEvent, keyAETheData, typeUnicodeText, &text);
	else
		err = AEGetParamDesc(theAppleEvent, keyAETheData, typeChar, &text);
	if (err != noErr)
		return err;
	err = AEGetParamPtr(theAppleEvent, keyAEFixLength, typeLongInteger,
	                    &returnedType, &fixLength,
	                    sizeof(fixLength), &actualsize);
	//if fixedLength == -1 then text is final
	//if fixedLength == 0 no comfirmed text
	//> 0 number of characters confirmed
	if (err != noErr)
		return err;
	int4 imetextsize;
	imetextsize = AEGetDescDataSize(&text);
	char *imetext = new char[imetextsize + 1];
	AEGetDescData(&text, (void *) imetext, imetextsize);
	imetext[imetextsize] = '\0';
	uint2 commitedLen = (fixLength == -1) ?imetextsize: fixLength;
	//get hilite range
	err = AEGetParamDesc(theAppleEvent, keyAEHiliteRange,
	                     typeTextRangeArray, &hiliterange);
	if (err == noErr)
	{
		uint4 hiliterangesize = AEGetDescDataSize(&hiliterange);
		hiliterangeptr = (TextRangeArray *)new char[hiliterangesize];
		AEGetDescData(&hiliterange, (void *) hiliterangeptr, hiliterangesize);
	}
	if (!MCS_imeisunicode())
	{
		uint4 unicodelen;
		char *unicodeimetext = new char[imetextsize << 1];
		uint2 charset = MCS_langidtocharset(GetScriptManagerVariable(smKeyScript));;
		if (commitedLen != 0)
		{
			if (!charset)
			{
				MCactivefield->stopcomposition(False, True);
				//user switched keyboard to english so we end composition and clear IME
				fixLength = -1;
			}
			MCactivefield->stopcomposition(True,False);
			MCU_multibytetounicode(imetext, commitedLen, unicodeimetext,
			                       imetextsize << 1, unicodelen, charset);
			MCString unicodestr(unicodeimetext, unicodelen);
			MCactivefield->finsertnew( FT_IMEINSERT, unicodestr, 0, true);
		}
		if (fixLength != -1)
			if (imetextsize != fixLength)
			{
				MCactivefield->startcomposition();
				MCU_multibytetounicode(&imetext[commitedLen], imetextsize-commitedLen,
				                       unicodeimetext, imetextsize << 1,
				                       unicodelen, charset);
				MCString unicodestr(unicodeimetext, unicodelen);
				MCactivefield->finsertnew(FT_IMEINSERT, unicodestr, 0, true);
			}
			else if (imetextsize == 0 && fixLength == 0)
				MCactivefield->stopcomposition(True,False);
		if (hiliterangeptr)
		{
			uint2 i;
			for (i = 0; i < hiliterangeptr->fNumOfRanges; i++)
				if (hiliterangeptr->fRange[i].fHiliteStyle == kTSMHiliteCaretPosition)
				{
					MCU_multibytetounicode(&imetext[hiliterangeptr->fRange[i].fStart],
					                       imetextsize-hiliterangeptr->fRange[i].fStart,
					                       unicodeimetext, imetextsize << 1,
					                       unicodelen, charset);
					MCactivefield->setcompositioncursoroffset(unicodelen);
				}
			MCactivefield->setcompositionconvertingrange(0,0);
		}
		delete unicodeimetext;
	}
	else
	{
		// printf("fixlength %d, imetextsize %d, commitedlen %d\n",fixLength,imetextsize,commitedLen);
		if (hiliterangeptr)
		{
			uint2 i;
			for (i = 0; i < hiliterangeptr->fNumOfRanges; i++)
				if (hiliterangeptr->fRange[i].fHiliteStyle == kTSMHiliteCaretPosition)
					MCactivefield->setcompositioncursoroffset(hiliterangeptr
															  ->fRange[i].fStart);
			for (i = 0; i < hiliterangeptr->fNumOfRanges; i++)
				if (hiliterangeptr->fRange[i].fHiliteStyle == kTSMHiliteSelectedConvertedText)
					MCactivefield->setcompositionconvertingrange(hiliterangeptr->fRange[i].fStart,hiliterangeptr->fRange[i].fEnd);
		}
		
		// MW-2012-10-01: [[ Bug 10425 ]] Make sure we only use down/up messages if the input
		//   text is 1 Unicode char long, and maps to MacRoman.
		uint1 t_char;
		if (commitedLen == 2 && MCUnicodeMapToNative((const uint2 *)imetext, 1, t_char))
		{
			// MW-2012-10-30: [[ Bug 10501 ]] Make sure we stop composing
			MCactivefield -> stopcomposition(True, False);
			
			// MW-2008-08-21: [[ Bug 6700 ]] Make sure we generate synthentic keyUp/keyDown events
			//   for MacRoman characters entered using the default IME.
			char tbuf[2];
			tbuf[0] = t_char;
			tbuf[1] = 0;
			MCdispatcher->wkdown(MCactivefield -> getstack() -> getwindow(), tbuf, ((unsigned char *)tbuf)[0]);
			MCdispatcher->wkup(MCactivefield -> getstack() -> getwindow(), tbuf, ((unsigned char *)tbuf)[0]);
		}
		else
		{
			if (commitedLen != 0)
			{
				MCactivefield->stopcomposition(True,False);
				MCString unicodestr(imetext, commitedLen);
				MCactivefield->finsertnew( FT_IMEINSERT, unicodestr, 0, true);
			}
		}
		
		if (fixLength != -1)
			if (imetextsize != fixLength)
			{
				MCactivefield->startcomposition();
				MCString unicodestr(&imetext[commitedLen], imetextsize-commitedLen);
				MCactivefield->finsertnew( FT_IMEINSERT, unicodestr, 0, true);
			}
			else if (imetextsize == 0 && fixLength == 0)
				MCactivefield->stopcomposition(True,False);
	}
	if (hiliterangeptr)
		delete hiliterangeptr;
	delete imetext;
	AEDisposeDesc(&text);
	AEDisposeDesc(&hiliterange);
	return noErr;
}

pascal OSErr TSMUnicodeNotFromInputHandler(const AppleEvent *theAppleEvent,
										   AppleEvent *reply,
										   long handlerRefcon)
{
	OSErr err;
	AEDesc text,slr;
	if (!MCactivefield)
		return paramErr;
	ScriptLanguageRecord scriptLangRec;
	err = AEGetParamDesc(theAppleEvent, keyAETSMScriptTag,
	                     typeIntlWritingCode, &slr);
	err = AEGetDescData( &slr, (void *)&scriptLangRec,
						sizeof(ScriptLanguageRecord));
	if (err != noErr)
		return err;
	ScriptCode scriptcode = scriptLangRec.fScript;
	uint1 charset = MCS_langidtocharset(scriptcode);
	AEDisposeDesc(&slr);
	err = AEGetParamDesc(theAppleEvent, keyAETheData, typeUnicodeText, &text);
	if (err != noErr)
		return err;
	int4 imetextsize;
	imetextsize = AEGetDescDataSize(&text);
	if (!imetextsize)
		return paramErr;
	char *imetext = new char[imetextsize + 1];
	AEGetDescData(&text, (void *) imetext, imetextsize);
	imetext[imetextsize] = '\0';
	if (imetextsize != 0)
	{
		if (imetextsize == 2)
		{
			uint2 *ukey = (uint2 *)imetext;
			//allow ansi characters to be passed as low level keyevent
			if (*ukey <= MAXINT1)
			{
				delete imetext;
				AEDisposeDesc(&text);
				return paramErr;
			}
			else if (!charset)
			{
				uint1 tcharset = MCU_unicodetocharset(*ukey);
				if (tcharset == LCH_ENGLISH || tcharset == LCH_GREEK)
				{
					delete imetext;
					AEDisposeDesc(&text);
					return paramErr;
				}
				else
				{
					uint2 tmods = MCscreen->querymods();
					tmods &= ~MS_CAPS_LOCK;
					if (tmods != 0)
					{
						delete imetext;
						AEDisposeDesc(&text);
						return paramErr;
					}
					else
						charset = tcharset;
				}
			}
		}
		MCactivefield->stopcomposition(True,False);
		MCString unicodestr(imetext, imetextsize);
		// we pass charset as keysym to avoid changing keyboards
		MCactivefield->finsertnew(FT_IMEINSERT, unicodestr, charset, true);
	}
	delete imetext;
	AEDisposeDesc(&text);
	return  noErr;
}

////////////////////////////////////////////////////////////////////////////////
