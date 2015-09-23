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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
//#include "execpt.h" 
#include "dispatch.h"
#include "image.h"
#include "globals.h"
#include "resolution.h"
#include "redraw.h"
#include "util.h"

#include "lnxdc.h"

#include "lnxdnd.h" 


static bool g_dnd_init = false;
static GdkCursor *g_dnd_cursor_drop_okay = NULL;
static GdkCursor *g_dnd_cursor_drop_fail = NULL;


// Do all the setup of the xDnD protocol
void init_dnd()
{
	if (!g_dnd_init)
	{
		// Create cursors for indicating drop acceptability
        g_dnd_cursor_drop_okay = gdk_cursor_new(GDK_CROSS);
        g_dnd_cursor_drop_fail = gdk_cursor_new(GDK_PIRATE);
        
        // Initialisation done
        g_dnd_init = true;
	}
}

// Nothing ever calls this but somebody might, one day...
void shutdown_xdnd()
{
    //gdk_cursor_unref(g_dnd_cursor_drop_okay);
    //gdk_cursor_unref(g_dnd_cursor_drop_fail);
    g_dnd_init = false;
}

void set_dnd_cursor(GdkWindow *w, bool p_okay, MCImage *p_image)
{
    // Images are not supported at the moment (though GDK does provide some very
    // basic support for doing so via the find_window_for_screen function)
    if (p_okay)
        gdk_window_set_cursor(w, g_dnd_cursor_drop_okay);
    else
        gdk_window_set_cursor(w, g_dnd_cursor_drop_fail);
}


struct dnd_modal_loop_context
{
    GdkDragContext* drag_context;
    GdkDisplay* display;
};

static void break_dnd_modal_loop(void* context)
{
    dnd_modal_loop_context* t_context = (dnd_modal_loop_context*)context;
    gdk_drag_abort(t_context->drag_context, GDK_CURRENT_TIME);
    gdk_display_pointer_ungrab(t_context->display, GDK_CURRENT_TIME);
}

// SN-2014-07-11: [[ Bug 12769 ]] Update the signature - the non-implemented UIDC dodragdrop was called otherwise
MCDragAction MCScreenDC::dodragdrop(Window w, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint* p_image_offset)
{
    //fprintf(stderr, "DND: dodragdrop\n");
    // The source window for the drag and drop operation
    GdkWindow *t_source;
    t_source = last_window;
    
    // Preserve the modifier state
    uint16_t t_old_modstate = MCmodifierstate;
    
    // Translate the allowed actions into a set of GDK actions
    gint t_possible_actions = 0;
    gint t_suggested_action = 0;
    if (p_allowed_actions & DRAG_ACTION_COPY)
        t_possible_actions |= GDK_ACTION_COPY;
    if (p_allowed_actions & DRAG_ACTION_MOVE)
        t_possible_actions |= GDK_ACTION_MOVE;
    if (p_allowed_actions & DRAG_ACTION_LINK)
        t_possible_actions |= GDK_ACTION_LINK;
    
    // Which is the "best" action that we support?
    if (t_possible_actions & GDK_ACTION_LINK)
        t_suggested_action = GDK_ACTION_LINK;
    else if (t_possible_actions & GDK_ACTION_MOVE)
        t_suggested_action = GDK_ACTION_MOVE;
    else if (t_possible_actions & GDK_ACTION_COPY)
        t_suggested_action = GDK_ACTION_COPY;
    
    // Get the list of supported targets
    const MCLinuxRawClipboard* t_dragboard = static_cast<const MCLinuxRawClipboard*> (MCdragboard->GetRawClipboard());
    MCAutoDataRef t_targets(t_dragboard->CopyTargets());
    if (*t_targets == NULL)
        return DRAG_ACTION_NONE;
    
    // Turn it into a GList
    GList* t_target_list = NULL;
    for (uindex_t i = 0; i < MCDataGetLength(*t_targets)/sizeof(uint32_t); i++)
    {
        uint32_t t_atom = reinterpret_cast<const uint32_t*>(MCDataGetBytePtr(*t_targets))[i];
        t_target_list = g_list_append(t_target_list, gpointer(t_atom));
    }
    if (t_target_list == NULL)
        return DRAG_ACTION_NONE;
    
    // Create a drag-and-drop context for this operation
    GdkDragContext *t_context = gdk_drag_begin(MCLinuxRawClipboard::GetClipboardWindow(), t_target_list);
    
    // Take ownership of the mouse so that nothing interferes with the drag
    GdkScreen *t_screen;
    t_screen = gdk_display_get_default_screen(dpy);
    gdk_pointer_grab(gdk_screen_get_root_window(t_screen), FALSE,
                     GdkEventMask(GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK),
                     NULL, NULL, MCeventtime);
    
    // We need to know what action was selected so we know whether to delete
    // the data afterwards (as done for move actions)
    MCDragAction t_action = DRAG_ACTION_NONE;
    
    // Whether the target accepted the drop or not
    bool t_accepted = true;
    
    // Context for breaking out of the modal loop, if required
    dnd_modal_loop_context t_loop_context;
    modal_loop t_modal_loop;
    t_loop_context.drag_context = t_context;
    t_loop_context.display = dpy;
    t_modal_loop.break_function = break_dnd_modal_loop;
    t_modal_loop.context = &t_loop_context;
    modalLoopStart(t_modal_loop);
    
    // The drag-and-drop loop
    bool t_dnd_done = false;
    while (!t_dnd_done)
    {
        if (t_modal_loop.broken)
            break;
        
        // Run the GLib event loop to exhaustion
        while (g_main_context_iteration(NULL, FALSE))
            ;
        
        GdkEvent *t_event;
        if (pendingevents != NULL)
        {
            // Get the next event from the queue
            t_event = gdk_event_copy(pendingevents->event);
            MCEventnode *tptr = (MCEventnode *)pendingevents->remove(pendingevents);
            delete tptr;
        }
        else
        {
            // In theory, all events should have already been queued as pending
            // through the GLib main loop. However, that only applies to those
            // that the server has already sent - this function call prompts the
            // server to send any events queued on its end.
            t_event = gdk_event_get();
        }
        
        // If there is still no event, actively wait for one
        if (t_event == NULL)
        {
            g_main_context_iteration(NULL, TRUE);
            continue;
        }
        
        switch (t_event->type)
        {
            case GDK_KEY_PRESS:
            case GDK_KEY_RELEASE:
            {
                //fprintf(stderr, "DND: key event\n");
                // Update the modifier state with the asynchronous state
                MCmodifierstate = MCscreen->querymods();
                break;
            }
                
            case GDK_MOTION_NOTIFY:
            {
                //fprintf(stderr, "DND: motion notify\n");
                // Find the window that the motion has moved us into
                GdkWindow *t_dest_window;
                GdkDragProtocol t_protocol;
                gdk_drag_find_window_for_screen(t_context, NULL, t_screen,
                                                t_event->motion.x,
                                                t_event->motion.y,
                                                &t_dest_window,
                                                &t_protocol);
                
                // TODO: set the cursor appropriately
                
                // Send a drag motion event
                gdk_drag_motion(t_context, t_dest_window, t_protocol,
                                t_event->motion.x, t_event->motion.y,
                                GdkDragAction(t_suggested_action),
                                GdkDragAction(t_possible_actions),
                                t_event->motion.time);
                
                break;
            }
                
            case GDK_BUTTON_RELEASE:
            {
                // Drop the item that was being dragged
                //fprintf(stderr, "DND: button release\n");
                
                // Take ownership of the drag-and-drop selection
                gdk_selection_owner_set_for_display(dpy, t_source, gdk_drag_get_selection(t_context), t_event->motion.time, TRUE);
                
                gdk_drag_drop(t_context, t_event->button.time);
                break;
            }
                
            case GDK_SELECTION_REQUEST:
            {
                //fprintf(stderr, "DND: selection request\n");
                // We are using the dragboard
                MCLinuxRawClipboard* t_clipboard = static_cast<MCLinuxRawClipboard*> (MCdragboard->GetRawClipboard());
                
                // Convert the requestor window XID into a GdkWindow
                GdkWindow *t_requestor;
                t_requestor = x11::gdk_x11_window_foreign_new_for_display(dpy, t_event->selection.requestor);
                
                // There is a backwards-compatibility issue with the way the
                // ICCCM deals with selections: older clients can request a
                // selection but not supply a property name. In that case,
                // the property set should be equal to the target name.
                //
                // The GDK manual does not say whether it works around this
                // wrinkle so we might as well check ourselves.
                GdkAtom t_property;
                if (t_event->selection.property != GDK_NONE)
                    t_property = t_event->selection.property;
                else
                    t_property = t_event->selection.target;
                
                // What type should the selection be converted to?
                static GdkAtom s_targets = gdk_atom_intern_static_string("TARGETS");
                if (t_event->selection.target == s_targets)
                {
                    // Get the list of types we can convert to
                    MCAutoDataRef t_targets(t_clipboard->CopyTargets());
                    
                    if (*t_targets != NULL)
                    {
                        // Set a property on the requestor containing the
                        // list of targets we can convert to.
                        gdk_property_change(t_requestor, t_property,
                                            s_targets, 32, GDK_PROP_MODE_REPLACE,
                                            (const guchar*)MCDataGetBytePtr(*t_targets),
                                            MCDataGetLength(*t_targets)/sizeof(uint32_t));
                        
                        // Notify the requestor that we have replied
                        gdk_selection_send_notify(t_event->selection.requestor,
                                                  t_event->selection.selection,
                                                  t_event->selection.target,
                                                  t_property,
                                                  t_event->selection.time);
                    }
                    else
                    {
                        // We don't actually have anything to supply so
                        // reject the request without supplying any data
                        gdk_selection_send_notify(t_event->selection.requestor,
                                                  t_event->selection.selection,
                                                  t_event->selection.target,
                                                  GDK_NONE,
                                                  t_event->selection.time);
                    }
                }
                else
                {
                    // Turn the requested selection into a string
                    MCAutoStringRef t_atom_string(MCLinuxRawClipboard::CopyTypeForAtom(t_event->selection.target));
                    
                    // Get the requested representation of the data
                    const MCRawClipboardItemRep* t_rep = NULL;
                    MCAutoRefcounted<const MCLinuxRawClipboardItem> t_item = t_clipboard->GetSelectionItem();
                    if (t_item != NULL)
                        t_rep = t_item->FetchRepresentationByType(*t_atom_string);
                    
                    // Get the data in the requested form
                    MCAutoDataRef t_data;
                    if (t_rep != NULL)
                        t_data.Give(t_rep->CopyData());
                    
                    if (*t_data != NULL)
                    {
                        // Transfer the data to the requestor via the
                        // property that it specified
                        gdk_property_change(t_requestor, t_property,
                                            t_event->selection.target,
                                            8,
                                            GDK_PROP_MODE_REPLACE,
                                            (const guchar*)MCDataGetBytePtr(*t_data),
                                            MCDataGetLength(*t_data));
                        
                        // Notify the requestor that we have replied
                        gdk_selection_send_notify(t_event->selection.requestor,
                                                  t_event->selection.selection,
                                                  t_event->selection.target,
                                                  t_property,
                                                  t_event->selection.time);
                    }
                    else
                    {
                        // Could not convert the data to the format that was
                        // requested - reject the request.
                        gdk_selection_send_notify(t_event->selection.requestor,
                                                  t_event->selection.selection,
                                                  t_event->selection.target,
                                                  GDK_NONE,
                                                  t_event->selection.time);
                    }
                }
                
                // We don't need the requestor window handle any longer
                g_object_unref(t_requestor);
                break;
            }
                
            case GDK_DRAG_ENTER:
                // This is a D&D client event
                DnDClientEvent(t_event);
                break;
                
            case GDK_DRAG_LEAVE:
                // This is a D&D client event
                DnDClientEvent(t_event);
                break;
                
            case GDK_DRAG_MOTION:
                // This is a D&D client event
                DnDClientEvent(t_event);
                break;
                
            case GDK_DRAG_STATUS:
            {
                //fprintf(stderr, "DND: drag status\n");
                // Which action did the destination request?
                GdkDragAction t_gdk_action;
                t_gdk_action = gdk_drag_context_get_selected_action(t_context);
                
                // Convert to the engine's drag actions
                if (t_gdk_action == GDK_ACTION_LINK)
                    t_action = DRAG_ACTION_LINK;
                if (t_gdk_action == GDK_ACTION_MOVE)
                    t_action = DRAG_ACTION_MOVE;
                if (t_gdk_action == GDK_ACTION_COPY)
                    t_action = DRAG_ACTION_COPY;
                
                break;
            }
                
            case GDK_DROP_START:
                // This is a D&D client event. Note the need to ungrab the
                // pointer, however (just in case the stack needs it)
                gdk_display_pointer_ungrab(dpy, t_event->dnd.time);
                DnDClientEvent(t_event);
                break;
                
            case GDK_DROP_FINISHED:
            {
                //fprintf(stderr, "DND: drop finished\n");
                // Did the drop succeed?
                bool t_success;
                t_success = gdk_drag_drop_succeeded(t_context);
                
                // If we failed, there was no action
                if (!t_success)
                    t_action = DRAG_ACTION_NONE;
                
                // All done
                t_dnd_done = true;
                break;
            }
        }
        
        gdk_event_free(t_event);
        
        // Unlock the screen, perform redraw and other cleanup tasks
        MCU_resetprops(True);
        MCRedrawUpdateScreen();
        siguser();
    }
    
    modalLoopEnd();
    
    // Other people can now use the pointer
    g_object_unref(t_context);
    gdk_display_pointer_ungrab(dpy, GDK_CURRENT_TIME);
    
    // Restore the original modifier key state
    MCmodifierstate = t_old_modstate;
    
    return t_action;
}
