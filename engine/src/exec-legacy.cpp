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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "util.h"
#include "mode.h"
#include "uidc.h"
#include "stack.h"
#include "card.h"
#include "group.h"

#include "exec.h"
#include "hc.h"
#include "eps.h"
#include "aclip.h"
#include "vclip.h"

////////////////////////////////////////////////////////////////////////////////

#define HEAP_SPACE  1000000
#define STACK_SPACE  1000000

////////////////////////////////////////////////////////////////////////////////

void MCLegacyExecCompactStack(MCExecContext& ctxt, MCStack *p_stack)
{
	p_stack->compact();
}


void MCLegacyEvalHasMemory(MCExecContext& ctxt, uinteger_t p_bytes, bool& r_bool)
{
	char *t_buffer = nil;
	r_bool = nil != (t_buffer = (char*)malloc(p_bytes));
	free(t_buffer);
}

//////////

void MCLegacyEvalHeapSpace(MCExecContext& ctxt, integer_t& r_bytes)
{
	r_bytes = HEAP_SPACE;
}

//////////

void MCLegacyEvalStackSpace(MCExecContext& ctxt, integer_t& r_bytes)
{
	r_bytes = STACK_SPACE;
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyEvalIsNumber(MCExecContext& ctxt, MCStringRef p_string, bool& r_bool)
{
	real64_t t_real;
	r_bool = MCTypeConvertStringToReal(p_string, t_real);
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyEvalLicensed(MCExecContext& ctxt, bool& r_licensed)
{
	r_licensed = MCModeGetLicensed();
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyEvalMenus(MCExecContext& ctxt, MCStringRef& r_string)
{
	r_string = MCValueRetain(kMCEmptyString);
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyEvalScreenType(MCExecContext& ctxt, MCNameRef& r_name)
{
	switch (MCscreen->getvclass())
	{
	case StaticGray:
		r_name = MCValueRetain(MCN_staticgray);
		break;
	case GrayScale:
		r_name = MCValueRetain(MCN_grayscale);
		break;
	case StaticColor:
		r_name = MCValueRetain(MCN_staticcolor);
		break;
	case PseudoColor:
		r_name = MCValueRetain(MCN_pseudocolor);
		break;
	case TrueColor:
		r_name = MCValueRetain(MCN_truecolor);
		break;
	case DirectColor:
		r_name = MCValueRetain(MCN_directcolor);
		break;
	}
}

void MCLegacyEvalScreenVendor(MCExecContext& ctxt, MCNameRef& r_name)
{
	r_name = MCValueRetain(MCscreen->getvendorname());
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyEvalSelectedButtonOf(MCExecContext& ctxt, bool p_background, integer_t p_family, MCObjectPtr p_object, MCStringRef& r_string)
{
	MCCard *cptr;

	switch (p_object . object -> gettype())
	{
	case CT_CARD:
		cptr = (MCCard *)p_object . object;
		break;
	case CT_STACK:
		{
			MCStack *sptr = (MCStack *)p_object . object;
			cptr = sptr->getchild(CT_THIS, kMCEmptyString, CT_CARD);
		}
		break;
	default:
		ctxt.LegacyThrow(EE_SELECTEDBUTTON_BADPARENT);
		return;
	}

	if (cptr->selectedbutton(p_family, p_background, r_string))
		return;

	ctxt.Throw();
}

void MCLegacyEvalSelectedButton(MCExecContext& ctxt, bool p_background, integer_t p_family, MCStringRef& r_string)
{
	MCCard *cptr;
	cptr = MCdefaultstackptr->getchild(CT_THIS, kMCEmptyString, CT_CARD);

	if (cptr->selectedbutton(p_family, p_background, r_string))
		return;

	ctxt . Throw();
}
////////////////////////////////////////////////////////////////////////////////

void MCLegacyEvalTextHeightSum(MCExecContext& ctxt, MCObjectPtr p_object, integer_t& r_sum)
{
	p_object . object -> getintprop(ctxt, 0, P_FORMATTED_HEIGHT, False, r_sum);
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyEvalMenuObject(MCExecContext& ctxt, MCStringRef& r_object)
{
	if (!MCmenuobjectptr)
	{
		r_object = (MCStringRef)MCValueRetain(kMCEmptyString);
		return;
	}

	MCmenuobjectptr -> getstringprop(ctxt, 0, P_NAME, False, r_object);
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyExecDoInBrowser(MCExecContext& ctxt, MCStringRef p_script)
{
	if (MCModeExecuteScriptInBrowser(p_script) == ES_NORMAL)
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	const char *token;
	const char *command;
}
DT;

static const DT domenu_table[] = {
                               {"new stack...", "create stack\n\
                                set the mainStack of the topStack to \"Home\"\n\
                                modal \"Stack Properties\""},
                               {"open stack...", "answer file \"Choose a stack to open...\"\n\
                                if it is not empty then\n\
                                set the cursor to watch\n\
                                topLevel it\n\
                                end if"},
                               {"close stack", "close this stack"},
                               {"compact stack", "compact stack"},
                               {"print card", "print this card"},
                               {"print stack...", "print this stack"},
                               {"quit hypercard", "quit"},
                               {"quit", "quit"},
                               {"undo", "undo"},
                               {"cut", "cut"},
                               {"copy", "copy"},
                               {"paste", "paste"},
                               {"new card", "create card"},
                               {"delete card", "delete this card"},
                               {"cut card", "cut this card"},
                               {"copy card", "copy this card"},
                               {"background", "edit background"},
                               {"back", "go back"},
                               {"home", "go home"},
                               {"help", "help"},
                               {"recent", "go recent"},
                               {"first", "go to first card"},
                               {"prev", "go to prev card"},
                               {"next", "go to next card"},
                               {"last", "go to last card"},
                               {"find...", "modeless \"Find\""},
                               {"message", "modeless \"Message Box\""},
                               {"bring closer", "set the layer of the selobj to the layer of the selobj+1"},
                               {"send farther", "set the layer of the selobj to the layer of the selobj-1"},
                               {"new button", "create button"},
                               {"new field", "create field"},
                               {"new background", "create background"},
                           };

/* doMenu items
About Hypercard..., New Stack..., Open Stack..., Close Stack, Save A
Copy..., Compact Stack, Protect Stack..., Delete Stack..., Page
Setup..., Print Field..., Print Card, Print Stack..., Print Report...,
Quit Hypercard, Undo, Cut, Copy, Paste, New Card, Delete Card, Cut
Card, Copy Card, Text Style..., Background, Icon..., Back, Home, Help,
recent, First, Prev, Next, Last, Find..., Message, Scroll, Next
Window, Button Info..., Field Info..., Card Info..., Bkgnd Info...,
Stack Info..., Bring Closer, Send Farther, New Button, New Field, New
Background, Plain, Bold, Italic, Underline, Outline, Shadow, Condense,
Extend, Group, Other..., Select, Select All, Fill, Invert, Pickup,
Darken, Lighten, Trace Edges, Rotate Left, Rotate Right, Flip
Vertical, Flip Horizontal, Opaque, Transparent, Keep, Revert, grid,
Fatbits, Power Keys, Line Size..., brush Shape..., Edit pattern...,
Polygon Sides..., Draw filled, Draw centered, Draw multiple, Rotate,
Slant, distort, Perspective, Import Paint..., Export Paint..., New
Icon, Close Icon Editor, Duplicate Icon, Cut Icon, Copy Icon, Erase,
Frame, Gray, Mirror Horizontal, Mirror Vertical, Rotate 90, Shadow,
Delete Report, Cut Report, Copy Report, Report Items..., Report
Name..., New Report, New Item, Item Info..., Close Script, Save
Script, Revert To Saved, Print Script, Find Again, Find Selection,
Scroll To Selection, Replace..., Replace Again, Comment, Uncomment,
Set Checkpoint, Step, Step Into, Trace, Go, Trace Delay..., Abort,
Variable Watcher, Message Watcher
*/

void MCLegacyExecDoMenu(MCExecContext& ctxt, MCStringRef p_option)
{
	uint2 size = ELEMENTS(domenu_table);
	bool t_success = false;
	while(size--)
	{
		if (MCStringIsEqualToCString(p_option, domenu_table[size].token, kMCCompareCaseless))
		{
			t_success = true;
			break;
		}
	}
	if (t_success)
    {
        MCAutoStringRef t_command;
        /* UNCHECKED */ MCStringCreateWithCString(domenu_table[size].command, &t_command);
		ctxt . GetObject()->domess(*t_command);
    }
	else
	{
		MCAutoStringRef t_result;
		MCStringFormat(&t_result, "doMenu \"%@\" not implemented", p_option);
		ctxt . SetTheResultToValue(*t_result);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyExecLockColormap(MCExecContext& ctxt)
{
	MClockcolormap = True;
}

void MCLegacyExecUnlockColormap(MCExecContext& ctxt)
{
	MClockcolormap = False;
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyExecImport(MCExecContext& ctxt, MCStringRef p_filename, bool p_is_stack)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;

	if (!p_is_stack && MCdefaultstackptr->islocked())
	{
		ctxt . LegacyThrow(EE_IMPORT_LOCKED);
		return;
	}

	MCU_watchcursor(ctxt.GetObject()->getstack(), True);
    
	IO_handle t_stream;
	if ((t_stream = MCS_open(p_filename, kMCOpenFileModeRead, True, False, 0)) != NULL)
    {
        if (p_is_stack)
        {
            MCStack *t_stack;
            if (hc_import(p_filename, t_stream, t_stack) == IO_NORMAL)
            {
                t_stack->open();
            }
            else
            {
                ctxt . LegacyThrow(EE_IMPORT_CANTREAD);
            }
        }
        else
        {
            MCEPS *eptr = new (nothrow) MCEPS;
            if (eptr->import(p_filename, t_stream))
            {
                eptr->setparent(MCdefaultstackptr);
                eptr->attach(OP_CENTER, false);
            }
            else
            {
                delete eptr;
                ctxt . LegacyThrow(EE_IMPORT_CANTREAD);
            }
        }
        
        MCS_close(t_stream);
    }
    else
    {
		ctxt . LegacyThrow(EE_IMPORT_CANTOPEN);
	}
	// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
	//   return back to the caller.
	MCU_unwatchcursor(ctxt . GetObject()->getstack(), True);
}

void MCLegacyExecImportEps(MCExecContext& ctxt, MCStringRef p_filename)
{
	MCLegacyExecImport(ctxt, p_filename, false);
}
void MCLegacyExecImportHypercardStack(MCExecContext& ctxt, MCStringRef p_filename)
{
	MCLegacyExecImport(ctxt, p_filename, true);
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyGetRevRuntimeBehaviour(MCExecContext& ctxt, uinteger_t &r_value)
{
	r_value = MCruntimebehaviour;
}
void MCLegacySetRevRuntimeBehaviour(MCExecContext& ctxt, uint4 p_value)
{
	MCruntimebehaviour = p_value;		
}

void MCLegacyGetHcImportStat(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(MChcstat != nil ? MChcstat : kMCEmptyString);
}

void MCLegacySetHcImportStat(MCExecContext& ctxt, MCStringRef p_value)
{
	MCValueAssign(MChcstat, p_value);
}

void MCLegacyGetScriptTextFont(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(MCscriptfont != nil ? MCscriptfont : kMCEmptyString);
}

void MCLegacySetScriptTextFont(MCExecContext& ctxt, MCStringRef p_value)
{
	MCValueAssign(MCscriptfont, p_value);
}

void MCLegacyGetScriptTextSize(MCExecContext& ctxt, uinteger_t &r_value)
{
	r_value = MCscriptsize;
}

void MCLegacySetScriptTextSize(MCExecContext& ctxt, uinteger_t p_value)
{
	MCscriptsize = p_value;
}

void MCLegacyGetStackFiles(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetStackFiles(MCExecContext& ctxt, MCStringRef p_value)
{
}

void MCLegacyGetMenuBar(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (!MCmenubar)
		r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
	else
		MCmenubar -> getstringprop(ctxt, 0, P_LONG_NAME, False, r_value);
}

// SN-2014-09-01: [[ Bug 13300 ]] Updated 'set the menubar' to have a (useless) setter at the global scope
void MCLegacySetMenuBar(MCExecContext& ctxt, MCStringRef p_value)
{
}

void MCLegacyGetEditMenus(MCExecContext& ctxt, bool& r_value)
{
	r_value = true;
}

void MCLegacySetEditMenus(MCExecContext& ctxt, bool p_value)
{
}

void MCLegacyGetTextAlign(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetTextAlign(MCExecContext& ctxt, MCValueRef value)
{
}

void MCLegacyGetTextFont(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetTextFont(MCExecContext& ctxt, MCValueRef value)
{
}

void MCLegacyGetTextHeight(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetTextHeight(MCExecContext& ctxt, MCValueRef value)
{
}

void MCLegacyGetTextSize(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetTextSize(MCExecContext& ctxt, MCValueRef value)
{
}

void MCLegacyGetTextStyle(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetTextStyle(MCExecContext& ctxt, MCValueRef value)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyGetBufferMode(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCStringCreateWithCString("millions", r_value))
		return;

	ctxt . Throw();
}

void MCLegacySetBufferMode(MCExecContext& ctxt, MCStringRef p_value)
{
}

void MCLegacyGetMultiEffect(MCExecContext& ctxt, bool& r_value)
{
	r_value = false;
}

void MCLegacySetMultiEffect(MCExecContext& ctxt, bool p_value)
{
}

void MCLegacyGetPrintTextAlign(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetPrintTextAlign(MCExecContext& ctxt, MCValueRef value)
{
}

void MCLegacyGetPrintTextFont(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetPrintTextFont(MCExecContext& ctxt, MCValueRef value)
{
}

void MCLegacyGetPrintTextHeight(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetPrintTextHeight(MCExecContext& ctxt, MCValueRef value)
{
}

void MCLegacyGetPrintTextSize(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetPrintTextSize(MCExecContext& ctxt, MCValueRef value)
{
}

void MCLegacyGetPrintTextStyle(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCLegacySetPrintTextStyle(MCExecContext& ctxt, MCValueRef value)
{
}

void MCLegacyGetEditScripts(MCExecContext& ctxt, bool& r_value)
{
	r_value = true;
}

void MCLegacySetEditScripts(MCExecContext& ctxt, bool p_value)
{
}

void MCLegacyGetColorWorld(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCscreen->getdepth() > 1;
}

void MCLegacySetColorWorld(MCExecContext& ctxt, bool p_value)
{
}

void MCLegacyGetAllowKeyInField(MCExecContext& ctxt, bool& r_value)
{
	r_value = true;
}

void MCLegacySetAllowKeyInField(MCExecContext& ctxt, bool p_value)
{
}

void MCLegacyGetAllowFieldRedraw(MCExecContext& ctxt, bool& r_value)
{
	r_value = false;
}

void MCLegacySetAllowFieldRedraw(MCExecContext& ctxt, bool p_value)
{
}

void MCLegacyGetRemapColor(MCExecContext& ctxt, bool& r_value)
{
	r_value = true;
}

void MCLegacySetRemapColor(MCExecContext& ctxt, bool p_value)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyGetUserLevel(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCuserlevel;
}

void MCLegacySetUserLevel(MCExecContext& ctxt, uinteger_t p_value)
{
	MCuserlevel = p_value;
}

void MCLegacyGetUserModify(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCusermodify == True;
}

void MCLegacySetUserModify(MCExecContext& ctxt, bool p_value)
{
	MCusermodify = True;
}

void MCLegacyGetLockColormap(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClockcolormap == True;
}

void MCLegacySetLockColormap(MCExecContext& ctxt, bool p_value)
{
	MClockcolormap = p_value ? True : False;
}

void MCLegacyGetPrivateColors(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCuseprivatecmap == True;
}

void MCLegacySetPrivateColors(MCExecContext& ctxt, bool p_value)
{
	MCuseprivatecmap = p_value ? True : False;
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyGetLongWindowTitles(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClongwindowtitles == True;
}

void MCLegacySetLongWindowTitles(MCExecContext& ctxt, bool p_value)
{
	MClongwindowtitles = p_value ? True : False;
}

void MCLegacyGetBlindTyping(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCblindtyping == True;
}

void MCLegacySetBlindTyping(MCExecContext& ctxt, bool p_value)
{
	MCblindtyping = p_value ? True : False;
}

void MCLegacyGetPowerKeys(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCpowerkeys == True;
}

void MCLegacySetPowerKeys(MCExecContext& ctxt, bool p_value)
{
	MCpowerkeys = p_value ? True : False;
}

void MCLegacyGetTextArrows(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCtextarrows == True;
}

void MCLegacySetTextArrows(MCExecContext& ctxt, bool p_value)
{
	MCtextarrows = p_value ? True : False;
}

void MCLegacyGetColormap(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCStringCreateWithCString("fixed", r_value))
		return;

	ctxt . Throw();
}
void MCLegacySetColormap(MCExecContext& ctxt, MCStringRef p_value)
{
	// NO OP
}

void MCLegacyGetNoPixmaps(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCnopixmaps == True;
}

void MCLegacySetNoPixmaps(MCExecContext& ctxt, bool p_value)
{
	MCnopixmaps = p_value ? True : False;
}

void MCLegacyGetLowResolutionTimers(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClowrestimers == True;
}

void MCLegacySetLowResolutionTimers(MCExecContext& ctxt, bool p_value)
{
	MClowrestimers = p_value ? True : False;
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyGetVcSharedMemory(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCvcshm == True;
}

void MCLegacySetVcSharedMemory(MCExecContext& ctxt, bool p_value)
{
	MCvcshm = p_value ? True : False;
}

void MCLegacyGetVcPlayer(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(MCvcplayer);
}

void MCLegacySetVcPlayer(MCExecContext& ctxt, MCStringRef p_value)
{
	MCValueAssign(MCvcplayer, p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCLegacyGetSoundChannel(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCsoundchannel;
}

void MCLegacySetSoundChannel(MCExecContext& ctxt, uinteger_t p_value)
{
	MCsoundchannel = p_value;
}

void MCLegacyGetLzwKey(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(kMCEmptyString);
}

void MCLegacySetLzwKey(MCExecContext& ctxt, MCStringRef p_value)
{
	if (MCStringGetLength(p_value) == 8)
	{
		uint32_t p_count = 0;
		for (int i = 0; i < 8; i++)
			p_count += MCStringGetNativeCharAtIndex(p_value, i);
		if (p_count == 800)
			MCuselzw = True;
	}
}

void MCLegacyGetMultiple(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCmultiple == True;
}

void MCLegacySetMultiple(MCExecContext& ctxt, bool p_value)
{
	MCmultiple = p_value ? True : False;
}

void MCLegacyGetMultiSpace(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCmultispace;
}

void MCLegacySetMultiSpace(MCExecContext& ctxt, uinteger_t p_value)
{
	MCmultispace = p_value;
}
