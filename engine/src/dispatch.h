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

////////////////////////////////////////////////////////////////////////////////
//
//  Private Header File:
//    dispatch.h
//
//  Description:
//    This file contains the definition of the MCDispatch object.
//
//  Changes:
//    2009-06-25 MW Added MCDispatch::readstartupstack method to support project
//                  info loading.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef	DISPATCH_H
#define	DISPATCH_H

#include "object.h"

typedef bool (*MCForEachStackCallback)(void *state, MCStack *stack);

class MCDispatch : public MCObject
{
	char *license;
	MCStack *stacks;
	MCFontlist *fonts;
	Boolean handling;
	real8 starttime;
	MCObject *menu;
	MCStack *panels;
	char *startdir;
	char *enginedir;

	bool m_drag_source;
	bool m_drag_target;
	bool m_drag_end_sent;

	MCExternalHandlerList *m_externals;

	static MCImage *imagecache;

public:
	MCDispatch();
	// virtual functions from MCObject
	virtual ~MCDispatch();
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	// dummy cut function for checking licensing
	virtual Boolean cut(Boolean home);
	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *params, MCObject *pass_from);
	// MCDispatch functions
	const char *getl(const MCString &s);
	void getmainstacknames(MCExecPoint &);
	Boolean cl(const MCString &, const MCString &, Boolean doit);
	void appendstack(MCStack *sptr);
	void removestack(MCStack *sptr);
	void destroystack(MCStack *sptr, Boolean needremove);
	Boolean openstartup(const char *name, char **outpath, IO_handle &stream);
	Boolean openenv(const char *name, const char *env,
	                char **outpath, IO_handle &stream, uint4 offset);

	IO_stat readfile(const char *openpath, const char *inname, IO_handle &stream, MCStack *&sptr);
	IO_stat loadfile(const char *fname, MCStack *&sptr);

	// MW-2009-06-25: This method should be used to read stacks used from startup.
	//   Specifically, embedded stacks and ones contained in deployed project info.
	IO_stat readstartupstack(IO_handle stream, MCStack*& r_stack);
	
	// Load the given external from within the app bundle
	bool loadexternal(const char *p_external);

	void cleanup(IO_handle stream, char *lname, char *bname);
	IO_stat savestack(MCStack *sptr, const MCString &);
	IO_stat startup(void);

	void wclose(Window w);
	void wkfocus(Window w);
	void wkunfocus(Window w);
	Boolean wkdown(Window w, const char *string, KeySym key);
	void wkup(Window w, const char *string, KeySym key);

	void wmfocus(Window w, int2 x, int2 y);
	void wmunfocus(Window w);

	void wmdown(Window w, uint2 which);
	void wmdrag(Window w);
	void wmup(Window w, uint2 which);
	void wdoubledown(Window w, uint2 which);
	void wdoubleup(Window w, uint2 which);

	void wmfocus_stack(MCStack *stack, int2 x, int2 y);
	void wmdown_stack(MCStack *stack, uint2 which);
	void wmup_stack(MCStack *stack, uint2 which);
	
	// This method is invoked when this application is acting as a drag-drop
	// target and the mouse pointer has entered the given window.
	//
	void wmdragenter(Window w, MCPasteboard* p_data);

	// This method is invoked when this application is acting as a drag-drop
	// target and the mouse pointer has moved within the given window to the
	// specified location.
	//
	// The return value is the action that would be performed should a drop
	// occur at this point - DRAG_ACTION_NONE indicates the drop would not
	// be accepted.
	//
	MCDragAction wmdragmove(Window w, int2 x, int2 y);

	// This method is invoked when this application is acting as a drag-drop
	// target and the mouse pointer has left the given window.
	//
	void wmdragleave(Window w);

	// This method is invoked when this application is acting as a drag-drop
	// target and the operation has terminated over the given window with an
	// indication that a drop should occur (i.e. the operation has not been
	// cancelled and the user has released the mouse pointer).
	//
	// The return value is the action that has been performed.
	//
	MCDragAction wmdragdrop(Window w);

	// This method returns true when this application is acting as a drag-drop
	// source.
	//
	bool isdragsource(void);

	// This method returns true when this application is acting as a drag-drop
	// target.
	//
	bool isdragtarget(void);

	// Actually perform a paste - if p_explicit is true, then it means its the
	// result of a script command rather than short-cut.
	bool dopaste(MCObject*& r_object, bool p_explicit = false);
	void dodrop(bool p_source);

	void kfocusset(Window w);
	void property(Window w, Atom atom);
	void configure(Window w);
	void enter(Window w);
	void redraw(Window w, MCRegionRef dirty_region);
	MCFontStruct *loadfont(const MCString &fname, uint2 &size, uint2 style, Boolean printer);

	// This method iterates through all stacks and ensures none have a reference
	// to one of the ones in MCcursors.
	void clearcursors(void);

	// This method installs the given stack as the new home stack
	void changehome(MCStack *stack);

	// This method executes the given message in the given encoded stack in an isolated
	// environment.
	bool isolatedsend(const char *p_stack_data, uint32_t p_stack_data_length, const char *p_message, MCParameter *p_parameters);

#ifdef _WINDOWS_DESKTOP
	void freeprinterfonts();
#endif

	MCFontlist *getfontlist()
	{
		return fonts;
	}
	
	MCStack *findstackname(const MCString &);
	MCStack *findstackid(uint4 fid);
	MCStack *findstackd(Window w);
	MCStack *findchildstackd(Window w,uint2 index);
	MCObject *getobjid(Chunk_term type, uint4 inid);
	MCObject *getobjname(Chunk_term type, const MCString &);
	MCStack *gethome();
	Boolean ismainstack(MCStack *sptr);
	void addmenu(MCObject *target);
	void removemenu();
	void closemenus();
	MCObject *getmenu()
	{
		return menu;
	}
	void appendpanel(MCStack *sptr);
	void removepanel(MCStack *sptr);

	// Iterate through all stacks and substacks, invoking the callback for each one.
	bool foreachstack(MCForEachStackCallback p_callback, void *p_state);

	// Recreate the fontlist.
	void flushfonts(void);
	
	MCStack *getstacks(void)
	{
		return stacks;
	}

private:
	// MW-2012-02-17: [[ LogFonts ]] Actual method which performs a load stack. This
	//   is wrapped by readfile to handle logical font table.
	IO_stat doreadfile(const char *openpath, const char *inname, IO_handle &stream, MCStack *&sptr);
	// MW-2012-02-17: [[ LogFonts ]] Actual method which performs a save stack. This
	//   is wrapped by savestack to handle logical font table.
	IO_stat dosavestack(MCStack *sptr, const MCString &);
};
#endif
