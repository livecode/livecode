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


#include "platform.h"
#include "platform-legacy.h"

////////////////////////////////////////////////////////////////////////////////

void MCPlatformHandleApplicationStartup(int p_argc, MCStringRef *p_argv, MCStringRef *p_envp, int& r_error_code, MCStringRef& r_error_message);
void MCPlatformHandleApplicationShutdown(int& r_exit_code);
void MCPlatformHandleApplicationShutdownRequest(bool& r_terminate);
void MCPlatformHandleApplicationSuspend(void);
void MCPlatformHandleApplicationResume(void);
void MCPlatformHandleApplicationRun(bool& r_continue);

void MCPlatformHandleScreenParametersChanged(void);

void MCPlatformHandleWindowCloseRequest(MCPlatformWindowRef window);
void MCPlatformHandleWindowClose(MCPlatformWindowRef window);
void MCPlatformHandleWindowCancel(MCPlatformWindowRef window);
void MCPlatformHandleWindowIconify(MCPlatformWindowRef window);
void MCPlatformHandleWindowUniconify(MCPlatformWindowRef window);
void MCPlatformHandleWindowReshape(MCPlatformWindowRef window);
void MCPlatformHandleWindowFocus(MCPlatformWindowRef window);
void MCPlatformHandleWindowUnfocus(MCPlatformWindowRef window);
void MCPlatformHandleWindowRedraw(MCPlatformWindowRef window, MCPlatformSurfaceRef surface, MCGRegionRef dirty_rgn);
void MCPlatformHandleWindowConstrain(MCPlatformWindowRef window, MCPoint proposed_size, MCPoint& r_wanted_size);

void MCPlatformHandleModifiersChanged(MCPlatformModifiers modifiers);

void MCPlatformHandleRawKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
void MCPlatformHandleKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
void MCPlatformHandleKeyUp(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);

void MCPlatformHandleTextInputQueryTextRanges(MCPlatformWindowRef window, MCRange& r_marked_range, MCRange& r_selected_range);
void MCPlatformHandleTextInputQueryTextIndex(MCPlatformWindowRef window, MCPoint location, uindex_t& r_index);
void MCPlatformHandleTextInputQueryTextRect(MCPlatformWindowRef window, MCRange range, MCRectangle& first_line_rect, MCRange& r_actual_range);
void MCPlatformHandleTextInputQueryText(MCPlatformWindowRef window, MCRange range, unichar_t*& r_chars, uindex_t& r_char_count, MCRange& r_actual_range);
void MCPlatformHandleTextInputInsertText(MCPlatformWindowRef window, unichar_t *chars, uindex_t char_count, MCRange replace_range, MCRange selection_range, bool mark);
void MCPlatformHandleTextInputAction(MCPlatformWindowRef window, MCPlatformTextInputAction action);

void MCPlatformHandleMouseEnter(MCPlatformWindowRef window);
void MCPlatformHandleMouseLeave(MCPlatformWindowRef window);
void MCPlatformHandleMouseDown(MCPlatformWindowRef window, uint32_t button, uint32_t count);
void MCPlatformHandleMouseUp(MCPlatformWindowRef window, uint32_t button, uint32_t count);
void MCPlatformHandleMouseDrag(MCPlatformWindowRef window, uint32_t button);
void MCPlatformHandleMouseRelease(MCPlatformWindowRef window, uint32_t button, bool was_menu);
void MCPlatformHandleMouseMove(MCPlatformWindowRef window, MCPoint location);
void MCPlatformHandleMouseScroll(MCPlatformWindowRef window, int dx, int dy);

void MCPlatformHandleDragEnter(MCPlatformWindowRef window, class MCRawClipboard* p_clipboard, MCPlatformDragOperation& r_operation);
void MCPlatformHandleDragMove(MCPlatformWindowRef window, MCPoint location, MCPlatformDragOperation& r_operation);
void MCPlatformHandleDragLeave(MCPlatformWindowRef window);
void MCPlatformHandleDragDrop(MCPlatformWindowRef window, bool& r_accepted);

void MCPlatformHandleMenuUpdate(MCPlatformMenuRef menu);
void MCPlatformHandleMenuSelect(MCPlatformMenuRef menu, uindex_t index);

void MCPlatformHandleViewFocusSwitched(MCPlatformWindowRef window, uint32_t id);

void MCPlatformHandlePlayerFrameChanged(MCPlatformPlayerRef player);
void MCPlatformHandlePlayerMarkerChanged(MCPlatformPlayerRef player, MCPlatformPlayerDuration time);
void MCPlatformHandlePlayerCurrentTimeChanged(MCPlatformPlayerRef player);
void MCPlatformHandlePlayerFinished(MCPlatformPlayerRef player);
void MCPlatformHandlePlayerBufferUpdated(MCPlatformPlayerRef player);

void MCPlatformHandleSoundFinished(MCPlatformSoundRef sound);

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    void Callback::SendApplicationStartup(int p_argc, MCStringRef *p_argv, MCStringRef *p_envp, int& r_error_code, MCStringRef & r_error_message)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        MCPlatformHandleApplicationStartup(p_argc, p_argv, p_envp, r_error_code, r_error_message);
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::SendApplicationShutdown(int& r_exit_code)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
       MCPlatformHandleApplicationShutdown(r_exit_code);
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::SendApplicationShutdownRequest(bool& r_terminate)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        MCPlatformHandleApplicationShutdownRequest(r_terminate);
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::SendApplicationRun(bool& r_continue)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        MCPlatformHandleApplicationRun(r_continue);
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::SendApplicationSuspend(void)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        //MCLog("Application -> Suspend()", 0);
        MCPlatformHandleApplicationSuspend();
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::SendApplicationResume(void)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        //MCLog("Application -> Resume()", 0);
        MCPlatformHandleApplicationResume();
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    
    //////////
    
    void Callback::SendScreenParametersChanged(void)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        //MCLog("Application -> ScreenParametersChanged()", 0);
        MCPlatformHandleScreenParametersChanged();
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    //////////
    
    void Callback::SendWindowCloseRequest(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> CloseRequest()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowCloseRequest(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendWindowClose(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> Close()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowClose(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendWindowCancel(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> Cancel()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowCancel(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendWindowReshape(MCPlatformWindowRef p_window, MCRectangle p_new_content)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowReshape([%d, %d, %d, %d])", p_window, p_new_content . x, p_new_content . y, p_new_content . width, p_new_content . height);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowReshape(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendWindowConstrain(MCPlatformWindowRef p_window, MCPoint p_proposed_size, MCPoint& r_wanted_size)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowConstrain(p_window, p_proposed_size, r_wanted_size);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendWindowRedraw(MCPlatformWindowRef p_window, MCPlatformSurfaceRef p_surface, MCGRegionRef p_dirty_rgn)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowRedraw(%p, %p)", p_window, p_surface);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowRedraw(p_window, p_surface, p_dirty_rgn);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendWindowIconify(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowIconify()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowIconify(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendWindowUniconify(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowUniconify()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowUniconify(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendWindowFocus(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowFocus()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowFocus(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendWindowUnfocus(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowUnfocus()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowUnfocus(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::SendModifiersChanged(MCPlatformModifiers p_modifiers)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("ModifiersChanged()", 0);
        MCPlatformHandleModifiersChanged(p_modifiers);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::SendMouseEnter(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseEnter()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseEnter(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendMouseLeave(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseLeave()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseLeave(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendMouseDown(MCPlatformWindowRef p_window, uint32_t p_button, uint32_t p_count)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
       //MCLog("Window(%p) -> MouseDown(%d, %d)", p_window, p_button, p_count);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseDown(p_window, p_button, p_count);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendMouseUp(MCPlatformWindowRef p_window, uint32_t p_button, uint32_t p_count)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseUp(%d, %d)", p_window, p_button, p_count);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseUp(p_window, p_button, p_count);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendMouseDrag(MCPlatformWindowRef p_window, uint32_t p_button)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseDrag(%d)", p_window, p_button);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseDrag(p_window, p_button);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendMouseRelease(MCPlatformWindowRef p_window, uint32_t p_button, bool p_was_menu)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseRelease(%d, %d)", p_window, p_button, p_was_menu);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseRelease(p_window, p_button, p_was_menu);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendMouseMove(MCPlatformWindowRef p_window, MCPoint p_location)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseMove([%d, %d])", p_window, p_location . x, p_location . y);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseMove(p_window, p_location);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendMouseScroll(MCPlatformWindowRef p_window, int dx, int dy)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseScroll(%d, %d)", p_window, dx, dy);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseScroll(p_window, dx, dy);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::SendDragEnter(MCPlatformWindowRef p_window, MCRawClipboard* p_dragboard, MCPlatformDragOperation& r_operation)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> DragEnter(%p)", p_window, p_dragboard);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleDragEnter(p_window, p_dragboard, r_operation);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendDragLeave(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> DragLeave()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleDragLeave(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendDragMove(MCPlatformWindowRef p_window, MCPoint p_location, MCPlatformDragOperation& r_operation)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> DragMove([%d, %d])", p_window, p_location . x, p_location . y);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleDragMove(p_window, p_location, r_operation);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendDragDrop(MCPlatformWindowRef p_window, bool& r_accepted)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> DragDrop()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleDragDrop(p_window, r_accepted);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::SendRawKeyDown(MCPlatformWindowRef p_window, MCPlatformKeyCode p_key_code, codepoint_t p_mapped_codepoint, codepoint_t p_unmapped_codepoint)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> RawKeyDown(%d)", p_window, p_key_code);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleRawKeyDown(p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendKeyDown(MCPlatformWindowRef p_window, MCPlatformKeyCode p_key_code, codepoint_t p_mapped_codepoint, codepoint_t p_unmapped_codepoint)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> KeyDown(%04x, %06x, %06x)", p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleKeyDown(p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendKeyUp(MCPlatformWindowRef p_window, MCPlatformKeyCode p_key_code, codepoint_t p_mapped_codepoint, codepoint_t p_unmapped_codepoint)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> KeyUp(%04x, %06x, %06x)", p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleKeyUp(p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::SendTextInputQueryTextRanges(MCPlatformWindowRef p_window, MCRange& r_marked_range, MCRange& r_selected_range)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformHandleTextInputQueryTextRanges(p_window, r_marked_range, r_selected_range);
        MCPlatformWindowDeathGrip(p_window);
        //MCLog("Window(%p) -> QueryTextRanges(-> [%u, %u], [%u, %u])", p_window, r_marked_range . offset, r_marked_range . length, r_selected_range . offset, r_selected_range . length);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendTextInputQueryTextIndex(MCPlatformWindowRef p_window, MCPoint p_location, uindex_t& r_index)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformHandleTextInputQueryTextIndex(p_window, p_location, r_index);
        MCPlatformWindowDeathGrip(p_window);
        //MCLog("Window(%p) -> QueryTextIndex([%d, %d] -> %d)", p_window, p_location . x, p_location . y, r_index);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendTextInputQueryTextRect(MCPlatformWindowRef p_window, MCRange p_range, MCRectangle& r_first_line_rect, MCRange& r_actual_range)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleTextInputQueryTextRect(p_window, p_range, r_first_line_rect, r_actual_range);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendTextInputQueryText(MCPlatformWindowRef p_window, MCRange p_range, unichar_t*& r_chars, uindex_t& r_char_count, MCRange& r_actual_range)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleTextInputQueryText(p_window, p_range, r_chars, r_char_count, r_actual_range);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendTextInputInsertText(MCPlatformWindowRef p_window, unichar_t *p_chars, uindex_t p_char_count, MCRange p_replace_range, MCRange p_selection_range, bool p_mark)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> InsertText('', [%u, %u], [%u, %u], %d)", p_window, p_replace_range . offset, p_replace_range . length, p_selection_range . offset, p_selection_range . length, p_mark);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleTextInputInsertText(p_window, p_chars, p_char_count, p_replace_range, p_selection_range, p_mark);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendTextInputAction(MCPlatformWindowRef p_window, MCPlatformTextInputAction p_action)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> Action(%d)", p_window, p_action);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleTextInputAction(p_window, p_action);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::SendMenuUpdate(MCPlatformMenuRef p_menu)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Menu(%p) -> Update()", p_menu);
        MCPlatformHandleMenuUpdate(p_menu);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::SendMenuSelect(MCPlatformMenuRef p_menu, uindex_t p_index)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Menu(%p) -> Select(%d)", p_menu, p_index);
        MCPlatformHandleMenuSelect(p_menu, p_index);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::SendViewFocusSwitched(MCPlatformWindowRef p_window, uint32_t p_view_id)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> ViewFocusSwitched(%d)", p_window, p_view_id);
        MCPlatformHandleViewFocusSwitched(p_window, p_view_id);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::SendPlayerFrameChanged(MCPlatformPlayerRef p_player)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        MCPlatformHandlePlayerFrameChanged(p_player);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    void Callback::SendPlayerMarkerChanged(MCPlatformPlayerRef p_player, MCPlatformPlayerDuration p_time)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        //MCLog("Player(%p) -> MarkerChanged(%d)", p_player, p_time);
        MCPlatformHandlePlayerMarkerChanged(p_player, p_time);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    void Callback::SendPlayerCurrentTimeChanged(MCPlatformPlayerRef p_player)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        //MCLog("Player(%p) -> CurrentTimeChanged()", p_player);
        MCPlatformHandlePlayerCurrentTimeChanged(p_player);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    void Callback::SendPlayerFinished(MCPlatformPlayerRef p_player)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        //MCLog("Player(%p) -> Finished()", p_player);
        MCPlatformHandlePlayerFinished(p_player);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    void Callback::SendPlayerBufferUpdated(MCPlatformPlayerRef p_player)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        // MCLog("Player(%p) -> BufferUpdated()", p_player);
        MCPlatformHandlePlayerBufferUpdated(p_player);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    //////////
    
    void Callback::SendSoundFinished(MCPlatformSoundRef p_sound)
    {
#if defined(FEATURE_PLATFORM_AUDIO)
        //MCLog("Sound(%p) -> Finished()", p_sound);
        MCPlatformHandleSoundFinished(p_sound);
#endif // FEATURE_PLATFORM_AUDIO
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    
}  /* namespace MCPlatform */

