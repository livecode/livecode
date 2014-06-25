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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "transfer.h"
//#include "execpt.h" 
#include "dispatch.h"
#include "image.h"
#include "globals.h"

#include "lnxdc.h"

#include "lnxtransfer.h"
#include "lnxpasteboard.h"
#include "lnxdnd.h" 

bool g_dnd_init = false;
GdkCursor *g_dnd_cursor_drop_okay = NULL;
GdkCursor *g_dnd_cursor_drop_fail = NULL;
MCGdkTransferStore *MCtransferstore = NULL;
GdkWindow *g_last_drag_window;

// Do all the setup of the xDnD protocol
void init_dnd()
{
	if (!g_dnd_init)
	{
		// Create cursors for indicating drop acceptability
        g_dnd_cursor_drop_okay = gdk_cursor_new(GDK_CROSS);
        g_dnd_cursor_drop_fail = gdk_cursor_new(GDK_PIRATE);

		MCtransferstore = new MCGdkTransferStore(MCdpy);
		
        // Initialisation done
        g_dnd_init = true;
	}
}

// Nothing ever calls this but somebody might, one day...
void shutdown_xdnd()
{
	g_object_unref(g_dnd_cursor_drop_okay);
    g_object_unref(g_dnd_cursor_drop_fail);
    delete MCtransferstore;
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

void DnDClientEvent(GdkEvent*);

MCDragAction MCScreenDC::dodragdrop(MCPasteboard* p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint *p_image_offset)
{
    // The source window for the drag and drop operation
    GdkWindow *t_source;
    t_source = g_last_drag_window;
    
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
    
    // Get the list of supported transfer types
    MCTransferType *t_transfer_types;
    uint32_t t_transfer_types_count;
    MCtransferstore->cleartypes();
    if (!p_pasteboard->Query(t_transfer_types, t_transfer_types_count))
    {
        // No types supported therefore nothing to drop
        return DRAG_ACTION_NONE;
    }
    
    // Get the data for each transfer type
    for (uint32_t i = 0; i < t_transfer_types_count; i++)
    {
        // Ignore the type if we can't convert the data to the required form
        MCAutoDataRef t_data;
        if (p_pasteboard->Fetch(t_transfer_types[i], &t_data))
            MCtransferstore->addRevType(t_transfer_types[i], *t_data);
    }
    
    // Create a drag-and-drop context for this operation
    GdkDragContext *t_context = MCtransferstore->CreateDragContext(t_source);
    
    // Take ownership of the mouse so that nothing interferes with the drag
    GdkScreen *t_screen;
    t_screen = gdk_display_get_default_screen(dpy);
    gdk_pointer_grab(gdk_screen_get_root_window(t_screen), FALSE,
                     GdkEventMask(GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK),
                     NULL, NULL, GDK_CURRENT_TIME);
    
    // We need to know what action was selected so we know whether to delete
    // the data afterwards (as done for move actions)
    MCDragAction t_action = DRAG_ACTION_NONE;
    
    // Whether the target accepted the drop or not
    bool t_accepted = true;
    
    // The drag-and-drop loop
    bool t_dnd_done = false;
    while (!t_dnd_done)
    {
        // Get the next event (remember to free it at the end!)
        GdkEvent *t_event = gdk_event_get();
        
        switch (t_event->type)
        {
            case GDK_KEY_PRESS:
            case GDK_KEY_RELEASE:
            {
                // Update the modifier state with the asynchronous state
                MCmodifierstate = MCscreen->querymods();
                break;
            }
                
            case GDK_MOTION_NOTIFY:
            {
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
                gdk_drag_drop(t_event->dnd.context, t_event->button.time);
                break;
            }
                
            case GDK_SELECTION_REQUEST:
            {
                // The data from the drag-and-drop selection has been requested
                if (t_event->selection.selection == gdk_drag_get_selection(t_context))
                {
                    // What transfer type ended up being requested?
                    MCTransferType t_type;
                    MCMIMEtype *t_mime_type;
                    t_mime_type = new MCMIMEtype(dpy, t_event->selection.target);
                    t_type = t_mime_type->asRev();
                    
                    GdkWindow *t_requestor;
                    t_requestor = x11::gdk_x11_window_foreign_new_for_display(dpy, t_event->selection.requestor);
                    
                    // Send the requested data, if at all possible
                    MCAutoDataRef t_data;
                    if (MCtransferstore->Fetch(t_mime_type, &t_data, t_event->selection.property, t_event->selection.window, t_requestor, t_event->selection.time))
                    {
                        // Send the data to the requestor window
                        gdk_property_change(t_requestor,
                                            t_event->selection.property,
                                            t_event->selection.target,
                                            8, GDK_PROP_MODE_REPLACE,
                                            MCDataGetBytePtr(*t_data),
                                            MCDataGetLength(*t_data));
                        
                        // Tell the requestor that the data is ready
                        gdk_selection_send_notify(t_event->selection.requestor,
                                                  t_event->selection.selection,
                                                  t_event->selection.target,
                                                  t_event->selection.property,
                                                  t_event->selection.time);
                        
                    }
                    
                    g_object_unref(t_requestor);
                }
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
                // Which action did the destination request?
                GdkDragAction t_gdk_action;
                t_gdk_action = gdk_drag_context_get_actions(t_event->dnd.context);
                
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
                gdk_pointer_ungrab(t_event->dnd.time);
                DnDClientEvent(t_event);
                break;
                
            case GDK_DROP_FINISHED:
            {
                // Did the drop succeed?
                bool t_success;
                t_success = gdk_drag_drop_succeeded(t_event->dnd.context);
                
                // If we failed, there was no action
                if (!t_success)
                    t_action = DRAG_ACTION_NONE;
                
                // All done
                t_dnd_done = true;
                break;
            }
        }
        
        gdk_event_free(t_event);
    }
    
    // Other people can now use the pointer
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    
    // Clean up allocated memory
    //if (t_pasteboard != NULL)
    //    t_pasteboard->Release();
    
    // Restore the original modifier key state
    MCmodifierstate = t_old_modstate;
    
    return t_action;
}
