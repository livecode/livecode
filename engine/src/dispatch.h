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

typedef bool (*MCStackForEachCallback)(MCStack *p_stack, void *p_context);

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

    MCStack *m_transient_stacks;
    
	static MCImage *imagecache;

    // AL-2015-02-10: [[ Standalone Inclusions ]] Add resource mapping array to MCDispatch object.
    MCArrayRef m_library_mapping;

    static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;

public:
	MCDispatch();
	// virtual functions from MCObject
	virtual ~MCDispatch();
    
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    
#ifdef LEGACY_EXEC
	virtual Exec_stat getprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
    virtual Exec_stat setprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
#endif
	// dummy cut function for checking licensing
	virtual Boolean cut(Boolean home);
	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *params, MCObject *pass_from);
	// MCDispatch functions
	const char *getl(const MCString &s);
	bool getmainstacknames(MCListRef& r_list);
	Boolean cl(const MCString &, const MCString &, Boolean doit);
	void appendstack(MCStack *sptr);
	void removestack(MCStack *sptr);
	void destroystack(MCStack *sptr, Boolean needremove);
	Boolean openstartup(MCStringRef name, MCStringRef& r_outpath, IO_handle &r_stream);
	Boolean openenv(MCStringRef name, MCStringRef env, MCStringRef& r_outpath, IO_handle& r_stream, uint4 offset);

	IO_stat readfile(MCStringRef openpath, MCStringRef inname, IO_handle &stream, MCStack *&sptr);
	IO_stat loadfile(MCStringRef p_name, MCStack *&sptr);

	// MW-2009-06-25: This method should be used to read stacks used from startup.
	//   Specifically, embedded stacks and ones contained in deployed project info.
	IO_stat readstartupstack(IO_handle stream, MCStack*& r_stack);
	
	// Load the given external from within the app bundle
	bool loadexternal(MCStringRef p_external);

	void cleanup(IO_handle stream, MCStringRef lname, MCStringRef bname);
	IO_stat savestack(MCStack *sptr, const MCStringRef);
	IO_stat startup(void);
	
	void wreshape(Window w);
	void wredraw(Window w, MCPlatformSurfaceRef surface, MCGRegionRef region);
	void wiconify(Window w);
	void wuniconify(Window w);
	
	void wclose(Window w);
	void wkfocus(Window w);
	void wkunfocus(Window w);
	Boolean wkdown(Window w, MCStringRef p_string, KeySym key);
	void wkup(Window w, MCStringRef p_string, KeySym key);

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
	MCFontStruct *loadfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer);
	
	// This method iterates through all stacks and ensures none have a reference
	// to one of the ones in MCcursors.
	void clearcursors(void);

	// This method installs the given stack as the new home stack
	void changehome(MCStack *stack);

	// This method executes the given message in the given encoded stack in an isolated
	// environment.
	bool isolatedsend(const char *p_stack_data, uint32_t p_stack_data_length, MCStringRef p_message, MCParameter *p_parameters);

    // MW-2014-06-24: [[ TransientStack ]] Transient stacks are a generalization of MCtooltip
    //   allowing controls to create popup windows temporarily by deriving from MCStack and then
    //   adding to MCdispatch for the time they are in use.
    bool is_transient_stack(MCStack *stack);
    void add_transient_stack(MCStack *stack);
    void remove_transient_stack(MCStack *stack);
    
#ifdef _WINDOWS_DESKTOP
	void freeprinterfonts();
#endif

	MCFontlist *getfontlist()
	{
		return fonts;
	}
	
	MCStack *findstackname(MCNameRef);
	MCStack *findstackid(uint4 fid);
	// IM-2014-07-09: [[ Bug 12225 ]] Find the stack by window ID
	MCStack *findstackwindowid(uintptr_t p_win_id);
	MCStack *findstackd(Window w);
	
	// IM-2014-07-23: [[ Bug 12930 ]] Replace findchildstack method with iterating method
	bool foreachchildstack(MCStack *p_stack, MCStackForEachCallback p_callback, void *p_context);
	
	MCObject *getobjid(Chunk_term type, uint4 inid);
	MCObject *getobjname(Chunk_term type, MCNameRef);
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

	// Recreate the fontlist.
	void flushfonts(void);
	
	MCStack *getstacks(void)
	{
		return stacks;
	}

    // AL-2015-02-10: [[ Standalone Inclusions ]] Add functions to fetch relative paths present
    //  in the resource mapping array of MCdispatcher.
    void addlibrarymapping(MCStringRef p_mapping);
    bool fetchlibrarymapping(MCStringRef p_name, MCStringRef& r_path);


	////////// PROPERTY ACCESSORS

    bool GetColor(MCExecContext& ctxt, Properties which, bool effective, MCInterfaceNamedColor& r_color);
    
	void GetDefaultTextFont(MCExecContext& ctxt, MCStringRef& r_font);
	void GetDefaultTextSize(MCExecContext& ctxt, uinteger_t& r_size);
	void GetDefaultTextStyle(MCExecContext& ctxt, MCInterfaceTextStyle& r_style);
    void GetDefaultTextAlign(MCExecContext& ctxt, intenum_t& r_align);
    void GetDefaultTextHeight(MCExecContext& ctxt, uinteger_t& r_height);
    
    void GetDefaultForePixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetDefaultBackPixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetDefaultTopPixel(MCExecContext& ctxt, uinteger_t& r_pixel);

	void GetDefaultForeColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetDefaultBackColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
    
	void GetDefaultPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
    
private:
	// MW-2012-02-17: [[ LogFonts ]] Actual method which performs a load stack. This
	//   is wrapped by readfile to handle logical font table.
	IO_stat doreadfile(MCStringRef openpath, MCStringRef inname, IO_handle &stream, MCStack *&sptr);
	// MW-2012-02-17: [[ LogFonts ]] Actual method which performs a save stack. This
    //   is wrapped by savestack to handle logical font table.
    IO_stat dosavestack(MCStack *sptr, const MCStringRef);
    // MW-2014-09-30: [[ ScriptOnlyStack ]] Save a stack if it is marked as script-only.
    IO_stat dosavescriptonlystack(MCStack *sptr, const MCStringRef);
};
#endif
