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
#include "globals.h"
#include "font.h"
#include "util.h"
#include "system.h"
#include "graphics_util.h"
#include "securemode.h"
#include "mode.h"
#include "dispatch.h"
#include "variable.h"
#include "param.h"

extern bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count);

#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
#ifdef TARGET_SUBPLATFORM_IPHONE
#include <CoreGraphics/CoreGraphics.h>
#else
#include <ApplicationServices/ApplicationServices.h>
#endif

extern CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
extern bool MCImageGetCGColorSpace(CGColorSpaceRef &r_colorspace);
extern bool MCGImageToCGImage(MCGImageRef p_src, const MCGIntegerRectangle &p_src_rect, bool p_invert, CGImageRef &r_image);
extern bool MCGRasterToCGImage(const MCGRaster &p_raster, const MCGIntegerRectangle &p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image);
#endif

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
    
    bool Callback::Callback_GetGlobalProperty(MCPlatformGlobalProperty p_property, MCPlatformPropertyType p_type, void *r_value)
    {
        switch (p_property)
        {
            case kMCPlatformGlobalPropertyDoubleDelta:
                *(uint16_t *)r_value = MCdoubledelta;
                break;
            case kMCPlatformGlobalPropertyDoubleTime:
                *(uint16_t *)r_value = MCdoubletime;
                break;
            case kMCPlatformGlobalPropertyDragDelta:
                *(uint16_t *)r_value = MCdragdelta;
                break;
            case kMCPlatformGlobalPropertyMajorOSVersion:
                *(uint32_t *)r_value = MCmajorosversion;
                break;
            case kMCPlatformGlobalPropertyAppIsActive:
                *(bool *)r_value = MCappisactive;
                break;
        }
        
        return true;
    }
    
    void Callback::Callback_SendApplicationStartup(int p_argc, MCStringRef *p_argv, MCStringRef *p_envp, int& r_error_code, MCStringRef & r_error_message)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        MCPlatformHandleApplicationStartup(p_argc, p_argv, p_envp, r_error_code, r_error_message);
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::Callback_SendApplicationShutdown(int& r_exit_code)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
       MCPlatformHandleApplicationShutdown(r_exit_code);
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::Callback_SendApplicationShutdownRequest(bool& r_terminate)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        MCPlatformHandleApplicationShutdownRequest(r_terminate);
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::Callback_SendApplicationRun(bool& r_continue)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        MCPlatformHandleApplicationRun(r_continue);
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::Callback_SendApplicationSuspend(void)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        //MCLog("Application -> Suspend()", 0);
        MCPlatformHandleApplicationSuspend();
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    void Callback::Callback_SendApplicationResume(void)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        //MCLog("Application -> Resume()", 0);
        MCPlatformHandleApplicationResume();
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    
    //////////
    
    void Callback::Callback_SendScreenParametersChanged(void)
    {
#if defined(FEATURE_PLATFORM_APPLICATION)
        //MCLog("Application -> ScreenParametersChanged()", 0);
        MCPlatformHandleScreenParametersChanged();
#endif // FEATURE_PLATFORM_APPLICATION
    }
    
    //////////
    
    void Callback::Callback_SendWindowCloseRequest(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> CloseRequest()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowCloseRequest(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendWindowClose(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> Close()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowClose(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendWindowCancel(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> Cancel()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowCancel(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendWindowReshape(MCPlatformWindowRef p_window, MCRectangle p_new_content)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowReshape([%d, %d, %d, %d])", p_window, p_new_content . x, p_new_content . y, p_new_content . width, p_new_content . height);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowReshape(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendWindowConstrain(MCPlatformWindowRef p_window, MCPoint p_proposed_size, MCPoint& r_wanted_size)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowConstrain(p_window, p_proposed_size, r_wanted_size);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendWindowRedraw(MCPlatformWindowRef p_window, MCPlatformSurfaceRef p_surface, MCGRegionRef p_dirty_rgn)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowRedraw(%p, %p)", p_window, p_surface);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowRedraw(p_window, p_surface, p_dirty_rgn);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendWindowIconify(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowIconify()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowIconify(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendWindowUniconify(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowUniconify()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowUniconify(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendWindowFocus(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowFocus()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowFocus(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendWindowUnfocus(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> WindowUnfocus()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleWindowUnfocus(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::Callback_SendModifiersChanged(MCPlatformModifiers p_modifiers)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("ModifiersChanged()", 0);
        MCPlatformHandleModifiersChanged(p_modifiers);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::Callback_SendMouseEnter(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseEnter()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseEnter(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendMouseLeave(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseLeave()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseLeave(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendMouseDown(MCPlatformWindowRef p_window, uint32_t p_button, uint32_t p_count)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
       //MCLog("Window(%p) -> MouseDown(%d, %d)", p_window, p_button, p_count);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseDown(p_window, p_button, p_count);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendMouseUp(MCPlatformWindowRef p_window, uint32_t p_button, uint32_t p_count)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseUp(%d, %d)", p_window, p_button, p_count);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseUp(p_window, p_button, p_count);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendMouseDrag(MCPlatformWindowRef p_window, uint32_t p_button)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseDrag(%d)", p_window, p_button);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseDrag(p_window, p_button);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendMouseRelease(MCPlatformWindowRef p_window, uint32_t p_button, bool p_was_menu)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseRelease(%d, %d)", p_window, p_button, p_was_menu);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseRelease(p_window, p_button, p_was_menu);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendMouseMove(MCPlatformWindowRef p_window, MCPoint p_location)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseMove([%d, %d])", p_window, p_location . x, p_location . y);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseMove(p_window, p_location);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendMouseScroll(MCPlatformWindowRef p_window, int dx, int dy)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> MouseScroll(%d, %d)", p_window, dx, dy);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleMouseScroll(p_window, dx, dy);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::Callback_SendDragEnter(MCPlatformWindowRef p_window, MCRawClipboard* p_dragboard, MCPlatformDragOperation& r_operation)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> DragEnter(%p)", p_window, p_dragboard);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleDragEnter(p_window, p_dragboard, r_operation);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendDragLeave(MCPlatformWindowRef p_window)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> DragLeave()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleDragLeave(p_window);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendDragMove(MCPlatformWindowRef p_window, MCPoint p_location, MCPlatformDragOperation& r_operation)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> DragMove([%d, %d])", p_window, p_location . x, p_location . y);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleDragMove(p_window, p_location, r_operation);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendDragDrop(MCPlatformWindowRef p_window, bool& r_accepted)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> DragDrop()", p_window);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleDragDrop(p_window, r_accepted);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::Callback_SendRawKeyDown(MCPlatformWindowRef p_window, MCPlatformKeyCode p_key_code, codepoint_t p_mapped_codepoint, codepoint_t p_unmapped_codepoint)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> RawKeyDown(%d)", p_window, p_key_code);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleRawKeyDown(p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendKeyDown(MCPlatformWindowRef p_window, MCPlatformKeyCode p_key_code, codepoint_t p_mapped_codepoint, codepoint_t p_unmapped_codepoint)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> KeyDown(%04x, %06x, %06x)", p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleKeyDown(p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendKeyUp(MCPlatformWindowRef p_window, MCPlatformKeyCode p_key_code, codepoint_t p_mapped_codepoint, codepoint_t p_unmapped_codepoint)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> KeyUp(%04x, %06x, %06x)", p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleKeyUp(p_window, p_key_code, p_mapped_codepoint, p_unmapped_codepoint);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::Callback_SendTextInputQueryTextRanges(MCPlatformWindowRef p_window, MCRange& r_marked_range, MCRange& r_selected_range)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformHandleTextInputQueryTextRanges(p_window, r_marked_range, r_selected_range);
        MCPlatformWindowDeathGrip(p_window);
        //MCLog("Window(%p) -> QueryTextRanges(-> [%u, %u], [%u, %u])", p_window, r_marked_range . offset, r_marked_range . length, r_selected_range . offset, r_selected_range . length);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendTextInputQueryTextIndex(MCPlatformWindowRef p_window, MCPoint p_location, uindex_t& r_index)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformHandleTextInputQueryTextIndex(p_window, p_location, r_index);
        MCPlatformWindowDeathGrip(p_window);
        //MCLog("Window(%p) -> QueryTextIndex([%d, %d] -> %d)", p_window, p_location . x, p_location . y, r_index);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendTextInputQueryTextRect(MCPlatformWindowRef p_window, MCRange p_range, MCRectangle& r_first_line_rect, MCRange& r_actual_range)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleTextInputQueryTextRect(p_window, p_range, r_first_line_rect, r_actual_range);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendTextInputQueryText(MCPlatformWindowRef p_window, MCRange p_range, unichar_t*& r_chars, uindex_t& r_char_count, MCRange& r_actual_range)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleTextInputQueryText(p_window, p_range, r_chars, r_char_count, r_actual_range);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendTextInputInsertText(MCPlatformWindowRef p_window, unichar_t *p_chars, uindex_t p_char_count, MCRange p_replace_range, MCRange p_selection_range, bool p_mark)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> InsertText('', [%u, %u], [%u, %u], %d)", p_window, p_replace_range . offset, p_replace_range . length, p_selection_range . offset, p_selection_range . length, p_mark);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleTextInputInsertText(p_window, p_chars, p_char_count, p_replace_range, p_selection_range, p_mark);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendTextInputAction(MCPlatformWindowRef p_window, MCPlatformTextInputAction p_action)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> Action(%d)", p_window, p_action);
        MCPlatformWindowDeathGrip(p_window);
        MCPlatformHandleTextInputAction(p_window, p_action);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::Callback_SendMenuUpdate(MCPlatformMenuRef p_menu)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Menu(%p) -> Update()", p_menu);
        MCPlatformHandleMenuUpdate(p_menu);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    void Callback::Callback_SendMenuSelect(MCPlatformMenuRef p_menu, uindex_t p_index)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Menu(%p) -> Select(%d)", p_menu, p_index);
        MCPlatformHandleMenuSelect(p_menu, p_index);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::Callback_SendViewFocusSwitched(MCPlatformWindowRef p_window, uint32_t p_view_id)
    {
#if defined(FEATURE_PLATFORM_WINDOW)
        //MCLog("Window(%p) -> ViewFocusSwitched(%d)", p_window, p_view_id);
        MCPlatformHandleViewFocusSwitched(p_window, p_view_id);
#endif // FEATURE_PLATFORM_WINDOW
    }
    
    //////////
    
    void Callback::Callback_SendPlayerFrameChanged(MCPlatformPlayerRef p_player)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        MCPlatformHandlePlayerFrameChanged(p_player);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    void Callback::Callback_SendPlayerMarkerChanged(MCPlatformPlayerRef p_player, MCPlatformPlayerDuration p_time)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        //MCLog("Player(%p) -> MarkerChanged(%d)", p_player, p_time);
        MCPlatformHandlePlayerMarkerChanged(p_player, p_time);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    void Callback::Callback_SendPlayerCurrentTimeChanged(MCPlatformPlayerRef p_player)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        //MCLog("Player(%p) -> CurrentTimeChanged()", p_player);
        MCPlatformHandlePlayerCurrentTimeChanged(p_player);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    void Callback::Callback_SendPlayerFinished(MCPlatformPlayerRef p_player)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        //MCLog("Player(%p) -> Finished()", p_player);
        MCPlatformHandlePlayerFinished(p_player);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    void Callback::Callback_SendPlayerBufferUpdated(MCPlatformPlayerRef p_player)
    {
#if defined(FEATURE_PLATFORM_PLAYER)
        // MCLog("Player(%p) -> BufferUpdated()", p_player);
        MCPlatformHandlePlayerBufferUpdated(p_player);
#endif // FEATURE_PLATFORM_PLAYER
    }
    
    //////////
    
    void Callback::Callback_SendSoundFinished(MCPlatformSoundRef p_sound)
    {
#if defined(FEATURE_PLATFORM_AUDIO)
        //MCLog("Sound(%p) -> Finished()", p_sound);
        MCPlatformHandleSoundFinished(p_sound);
#endif // FEATURE_PLATFORM_AUDIO
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    
    void *Callback::Callback_MCListPopFront(void*& x_list)
    {
        return MCListPopFront(x_list);
    }
    
    void Callback::Callback_MCListPushBack(void*& x_list, void *p_element)
    {
        MCListPushBack(x_list, p_element);
    }
    
    bool Callback::Callback_MCFontCreateWithHandle(MCSysFontHandle p_handle, MCNameRef p_name, MCFontRef& r_font)
    {
        return MCFontCreateWithHandle(p_handle, p_name, r_font);
    }
    MCRectangle Callback::Callback_MCU_reduce_rect(const MCRectangle &srect, int2 amount)
    {
        return MCU_reduce_rect(srect, amount);
    }
    
    MCRectangle Callback::Callback_MCU_intersect_rect(const MCRectangle &one, const MCRectangle &two)
    {
        return MCU_intersect_rect(one, two);
    }
    Boolean Callback::Callback_MCU_point_in_rect(const MCRectangle &srect, int2 x, int2 y)
    {
        return MCU_point_in_rect(srect, x, y);
    }
    void Callback::Callback_MCU_urldecode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result) const
    {
        MCU_urldecode(p_source, p_use_utf8, r_result);
    }
    bool Callback::Callback_MCU_urlencode(MCStringRef p_url, bool p_use_utf8, MCStringRef &r_encoded) const
    {
        return MCU_urlencode(p_url, p_use_utf8, r_encoded);
    }
    bool Callback::Callback_MCS_pathtonative(MCStringRef p_livecode_path, MCStringRef& r_native_path)
    {
        return MCS_pathtonative(p_livecode_path, r_native_path);
    }
    bool Callback::Callback_MCS_resolvepath(MCStringRef p_path, MCStringRef& r_resolved_path)
    {
        return MCS_resolvepath(p_path, r_resolved_path);
    }
    real8 Callback::Callback_MCS_time(void)
    {
        return MCS_time();
    }
    void Callback::Callback_MCGRasterApplyAlpha(MCGRaster &x_raster, const MCGRaster &p_alpha, const MCGIntegerPoint &p_offset)
    {
        MCGRasterApplyAlpha(x_raster, p_alpha, p_offset);
    }
    bool Callback::Callback_MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count)
    {
        return MCStringsSplit(p_string, p_separator, r_strings, r_count);
    }
    void Callback::Callback_MCImageFreeBitmap(MCImageBitmap *p_bitmap)
    {
        return MCImageFreeBitmap(p_bitmap);
    }
    void Callback::Callback_MCImageBitmapClear(MCImageBitmap *p_bitmap)
    {
        return MCImageBitmapClear(p_bitmap);
    }
    bool Callback::Callback_MCImageBitmapCreate(uindex_t p_width, uindex_t p_height, MCImageBitmap *&r_bitmap)
    {
        return MCImageBitmapCreate(p_width, p_height, r_bitmap);
    }
    bool Callback::Callback_MCImageBitmapHasTransparency(MCImageBitmap *p_bitmap)
    {
        return MCImageBitmapHasTransparency(p_bitmap);
    }
    MCGRaster Callback::Callback_MCImageBitmapGetMCGRaster(MCImageBitmap *p_bitmap, bool p_is_premultiplied)
    {
        return MCImageBitmapGetMCGRaster(p_bitmap, p_is_premultiplied);
    }
    bool Callback::Callback_MCSecureModeCanAccessAppleScript(void)
    {
        return MCSecureModeCanAccessAppleScript();
    }
    
    void Callback::Callback_SendOpenDoc(MCStringRef p_string)
    {
        if (MCModeShouldQueueOpeningStacks())
        {
            MCU_realloc((char **)&MCstacknames, MCnstacks, MCnstacks + 1, sizeof(MCStringRef));
            MCstacknames[MCnstacks++] = MCValueRetain(p_string);
        }
        else
        {
            MCStack *stkptr;  //stack pointer
            if (MCdispatcher->loadfile(p_string, stkptr) == IO_NORMAL)
                stkptr->open();
        }
    }
    
    void Callback::Callback_DoScript(MCStringRef p_script, MCStringRef &r_result)
    {
        MCExecContext ctxt(MCdefaultstackptr -> getcard(), nil, nil);
        MCdefaultstackptr->getcard()->domess(p_script);
        MCAutoValueRef t_value;
        /* UNCHECKED */ MCresult->eval(ctxt, &t_value);
        /* UNCHECKED */ ctxt . ConvertToString(*t_value, r_result);
    }
    
    void Callback::Callback_Eval(MCStringRef p_script, MCStringRef &r_result)
    {
        MCExecContext ctxt(MCdefaultstackptr -> getcard(), nil, nil);
        MCAutoValueRef t_value;
        MCdefaultstackptr->getcard()->eval(ctxt, p_script, &t_value);
        /* UNCHECKED */ ctxt.ConvertToString(*t_value, r_result);
    }
    
    Exec_stat Callback::Callback_SendMessage(MCNameRef p_message, MCValueRef p_val1, MCValueRef p_val2, MCValueRef p_val3)
    {
        MCParameter p1, p2, p3;
        p1.setvalueref_argument(p_val1);
        p1.setnext(&p2);
        p2.setvalueref_argument(p_val1);
        p2.setnext(&p3);
        p3.setvalueref_argument(p_val3);
        
        return MCdefaultstackptr->getcard()->message(p_message, &p1);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    // libgraphics
    
    MCGImageRef Callback::Callback_MCGImageRetain(MCGImageRef image)
    {
        return MCGImageRetain(image);
    }
    void Callback::Callback_MCGImageRelease(MCGImageRef image)
    {
        MCGImageRelease(image);
    }
    bool Callback::Callback_MCGRegionCreate(MCGRegionRef &r_region)
    {
        return MCGRegionCreate(r_region);
    }
    int32_t Callback::Callback_MCGImageGetWidth(MCGImageRef image)
    {
        return MCGImageGetWidth(image);
    }
    bool Callback::Callback_MCGImageIsOpaque(MCGImageRef image)
    {
        return MCGImageIsOpaque(image);
    }
    bool Callback::Callback_MCGRegionAddRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect)
    {
        return MCGRegionAddRect(p_region, p_rect);
    }
    void Callback::Callback_MCGRegionDestroy(MCGRegionRef p_region)
    {
        MCGRegionDestroy(p_region);
    }
    bool Callback::Callback_MCGRegionIsEmpty(MCGRegionRef p_region)
    {
        return MCGRegionIsEmpty(p_region);
    }
    bool Callback::Callback_MCGRegionIterate(MCGRegionRef p_region, MCGRegionIterateCallback p_callback, void *p_context)
    {
        return MCGRegionIterate(p_region, p_callback, p_context);
    }
    void Callback::Callback_MCGContextRelease(MCGContextRef context)
    {
        MCGContextRelease(context);
    }
    int32_t Callback::Callback_MCGImageGetHeight(MCGImageRef image)
    {
        return MCGImageGetHeight(image);
    }
    bool Callback::Callback_MCGImageGetRaster(MCGImageRef image, MCGRaster &r_raster)
    {
        return MCGImageGetRaster(image, r_raster);
    }
    bool Callback::Callback_MCGRegionSetEmpty(MCGRegionRef p_region)
    {
        return MCGRegionSetEmpty(p_region);
    }
    void Callback::Callback_MCGContextScaleCTM(MCGContextRef context, MCGFloat xscale, MCGFloat yscale)
    {
        MCGContextScaleCTM(context, xscale, yscale);
    }
    bool Callback::Callback_MCGRegionAddRegion(MCGRegionRef p_region, MCGRegionRef p_other)
    {
        return MCGRegionAddRegion(p_region, p_other);
    }
    MCGIntegerRectangle Callback::Callback_MCGRegionGetBounds(MCGRegionRef p_region)
    {
        return MCGRegionGetBounds(p_region);
    }
    void Callback::Callback_MCGContextDrawImage(MCGContextRef context, MCGImageRef image, MCGRectangle dst_rect, MCGImageFilter filter)
    {
        MCGContextDrawImage(context, image, dst_rect, filter);
    }
    void Callback::Callback_MCGContextClipToRect(MCGContextRef context, MCGRectangle rect)
    {
        MCGContextClipToRect(context, rect);
    }
    void Callback::Callback_MCGContextDrawPixels(MCGContextRef context, const MCGRaster& raster, MCGRectangle dst_rect, MCGImageFilter filter)
    {
        MCGContextDrawPixels(context, raster, dst_rect, filter);
    }
    MCGIntegerRectangle Callback::Callback_MCGRectangleGetBounds(const MCGRectangle &p_rect)
    {
        return MCGRectangleGetBounds(p_rect);
    }
    void Callback::Callback_MCGContextClipToRegion(MCGContextRef self, MCGRegionRef p_region)
    {
        MCGContextClipToRegion(self, p_region);
    }
    void Callback::Callback_MCGContextTranslateCTM(MCGContextRef context, MCGFloat xoffset, MCGFloat yoffset)
    {
        MCGContextTranslateCTM(context, xoffset, yoffset);
    }
    bool Callback::Callback_MCGContextCreateWithRaster(const MCGRaster& raster, MCGContextRef& r_context)
    {
        return MCGContextCreateWithRaster(raster, r_context);
    }
    MCGAffineTransform Callback::Callback_MCGContextGetDeviceTransform(MCGContextRef context)
    {
        return MCGContextGetDeviceTransform(context);
    }
    bool Callback::Callback_MCGImageCreateWithRasterNoCopy(const MCGRaster &raster, MCGImageRef &r_image)
    {
        return MCGImageCreateWithRasterNoCopy(raster, r_image);
    }
    MCGIntegerRectangle Callback::Callback_MCGIntegerRectangleIntersection(const MCGIntegerRectangle &rect_1, const MCGIntegerRectangle &rect_2)
    {
        return MCGIntegerRectangleIntersection(rect_1, rect_2);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
    // cgimageutil.cpp
    CGBitmapInfo Callback::Callback_MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha)
    {
        return MCGPixelFormatToCGBitmapInfo(p_pixel_format, p_alpha);
    }
    bool Callback::Callback_MCImageGetCGColorSpace(CGColorSpaceRef &r_colorspace)
    {
        return MCImageGetCGColorSpace(r_colorspace);
    }
    bool Callback::Callback_MCGImageToCGImage(MCGImageRef p_src, const MCGIntegerRectangle &p_src_rect, bool p_invert, CGImageRef &r_image)
    {
        return MCGImageToCGImage(p_src, p_src_rect, p_invert, r_image);
    }
    bool Callback::Callback_MCGRasterToCGImage(const MCGRaster &p_raster, const MCGIntegerRectangle &p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image)
    {
        return MCGRasterToCGImage(p_raster, p_src_rect, p_colorspace, p_copy, p_invert, r_image);
    }
#endif
    
    ////////////////////////////////////////////////////////////////////////////////
    
    
    // libfoundation
    bool Callback::Callback_MCDataCreateWithBytes(const byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data)
    {
        return MCDataCreateWithBytes(p_bytes, p_byte_count, r_data);
    }
    const byte_t *Callback::Callback_MCDataGetBytePtr(MCDataRef p_data)
    {
        return MCDataGetBytePtr(p_data);
    }
    uindex_t Callback::Callback_MCDataGetLength(MCDataRef p_data)
    {
        return MCDataGetLength(p_data);
    }
    bool Callback::Callback_MCListAppend(MCListRef list, MCValueRef value)
    {
        return MCListAppend(list, value);
    }
    bool Callback::Callback_MCListCopy(MCListRef list, MCListRef& r_new_list)
    {
        return MCListCopy(list, r_new_list);
    }
    bool Callback::Callback_MCListCopyAsString(MCListRef list, MCStringRef& r_string)
    {
        return MCListCopyAsString(list, r_string);
    }
    bool Callback::Callback_MCMemoryAllocate(size_t size, void*& r_block)
    {
        return MCMemoryAllocate(size, r_block);
    }
    bool Callback::Callback_MCMemoryReallocate(void *block, size_t new_size, void*& r_new_block)
    {
        return MCMemoryReallocate(block, new_size, r_new_block);
    }
    void Callback::Callback_MCMemoryDeallocate(void *block)
    {
        MCMemoryDeallocate(block);
    }
    bool Callback::Callback_MCMemoryNew(size_t size, void*& r_record)
    {
        return MCMemoryNew(size, r_record);
    }
    void Callback::Callback_MCMemoryDelete(void *p_record)
    {
        return MCMemoryDelete(p_record);
    }
    MCNameRef Callback::Callback_MCNAME(const char *p_string)
    {
        return MCNAME(p_string);
    }
    MCStringRef Callback::Callback_MCSTR(const char *string)
    {
        return MCSTR(string);
    }
    bool Callback::Callback_MCStringAppendFormatV(MCStringRef string, const char *p_format, va_list p_args)
    {
        return MCStringAppendFormatV(string, p_format, p_args);
    }
    bool Callback::Callback_MCStringBeginsWithCString(MCStringRef string, const char_t *prefix_cstring, MCStringOptions options)
    {
        return MCStringBeginsWithCString(string, prefix_cstring, options);
    }
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
    bool Callback::Callback_MCStringConvertToCFStringRef(MCStringRef string, CFStringRef& r_cfstring)
    {
        return MCStringConvertToCFStringRef(string, r_cfstring);
    }
    bool Callback::Callback_MCStringCreateWithCFString(CFStringRef cf_string, MCStringRef& r_string)
    {
        return MCStringCreateWithCFString(cf_string, r_string);
    }
#endif
    bool Callback::Callback_MCStringConvertToCString(MCStringRef string, char*& r_cstring)
    {
        return MCStringConvertToCString(string, r_cstring);
    }
    bool Callback::Callback_MCStringConvertToUTF8(MCStringRef string, char*& r_chars, uindex_t& r_char_count)
    {
        return MCStringConvertToUTF8(string, r_chars, r_char_count);
    }
    bool Callback::Callback_MCStringCopySubstring(MCStringRef string, MCRange range, MCStringRef& r_substring)
    {
        return MCStringCopySubstring(string, range, r_substring);
    }
    bool Callback::Callback_MCStringCreateMutable(uindex_t initial_capacity, MCStringRef& r_string)
    {
        return MCStringCreateMutable(initial_capacity, r_string);
    }
    bool Callback::Callback_MCStringCreateWithBytes(const byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string)
    {
        return MCStringCreateWithBytes(bytes, byte_count, encoding, is_external_rep, r_string);
    }
    bool Callback::Callback_MCStringCreateWithBytesAndRelease(byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string)
    {
        return MCStringCreateWithBytesAndRelease(bytes, byte_count, encoding, is_external_rep, r_string);
    }
    bool Callback::Callback_MCStringCreateWithCString(const char *cstring, MCStringRef& r_string)
    {
        return MCStringCreateWithCString(cstring, r_string);
    }
    bool Callback::Callback_MCStringCreateWithCStringAndRelease(char *cstring /*delete[]*/, MCStringRef& r_string)
    {
        return MCStringCreateWithCStringAndRelease(cstring , r_string);
    }
    bool Callback::Callback_MCStringCreateWithNativeChars(const char_t *chars, uindex_t char_count, MCStringRef& r_string)
    {
        return MCStringCreateWithNativeChars(chars, char_count, r_string);
    }
    bool Callback::Callback_MCStringCreateWithPascalString(const unsigned char* pascal_string, MCStringRef& r_string)
    {
        return MCStringCreateWithPascalString(pascal_string, r_string);
    }
    bool Callback::Callback_MCStringEncode(MCStringRef string, MCStringEncoding encoding, bool is_external_rep, MCDataRef& r_data)
    {
        return MCStringEncode(string, encoding, is_external_rep, r_data);
    }
    bool Callback::Callback_MCStringDecode(MCDataRef data, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string)
    {
        return MCStringDecode(data, encoding, is_external_rep, r_string);
    }
    bool Callback::Callback_MCStringFindAndReplace(MCStringRef string, MCStringRef pattern, MCStringRef replacement, MCStringOptions options)
    {
        return MCStringFindAndReplace(string, pattern, replacement, options);
    }
    bool Callback::Callback_MCStringFirstIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t after, MCStringOptions options, uindex_t& r_offset)
    {
        return MCStringFirstIndexOfChar(string, needle, after, options, r_offset);
    }
    bool Callback::Callback_MCStringFormatV(MCStringRef& r_string, const char *p_format, va_list p_args)
    {
        return MCStringFormatV(r_string, p_format, p_args);
    }
    uindex_t Callback::Callback_MCStringGetLength(const MCStringRef string)
    {
        return MCStringGetLength(string);
    }
    bool Callback::Callback_MCStringIsEmpty(MCStringRef string)
    {
        return MCStringIsEmpty(string);
    }
    bool Callback::Callback_MCStringIsEqualTo(MCStringRef string, MCStringRef other, MCStringOptions options)
    {
        return MCStringIsEqualTo(string, other, options);
    }
    bool Callback::Callback_MCStringIsEqualToCString(MCStringRef string, const char *cstring, MCStringOptions options)
    {
        return MCStringIsEqualToCString(string, cstring, options);
    }
    bool Callback::Callback_MCStringLastIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t before, MCStringOptions options, uindex_t& r_offset)
    {
        return MCStringLastIndexOfChar(string, needle, before, options, r_offset);
    }
    bool Callback::Callback_MCStringMutableCopy(MCStringRef string, MCStringRef& r_new_string)
    {
        return MCStringMutableCopy(string, r_new_string);
    }
    bool Callback::Callback_MCStringPrepend(MCStringRef string, MCStringRef prefix)
    {
        return MCStringPrepend(string, prefix);
    }
    bool Callback::Callback_MCStringSubstringIsEqualTo(MCStringRef string, MCRange range, MCStringRef p_other, MCStringOptions p_options)
    {
        return MCStringSubstringIsEqualTo(string, range, p_other, p_options);
    }
    void Callback::Callback_MCValueRelease(MCValueRef value)
    {
        return MCValueRelease(value);
    }
    MCValueRef Callback::Callback_MCValueRetain(MCValueRef value)
    {
        return MCValueRetain(value);
    }
    bool Callback::Callback_MCMemoryNewArray(uindex_t count, size_t size, void*& r_array, uindex_t& r_count)
    {
        return MCMemoryNewArray(count, size, r_array, r_count);
    }
    bool Callback::Callback_MCMemoryNewArray(uindex_t count, size_t size, void*& r_array)
    {
        return MCMemoryNewArray(count, size, r_array);
    }
    bool Callback::Callback_MCListCreateMutable(char_t delimiter, MCListRef& r_list)
    {
        return MCListCreateMutable(delimiter, r_list);
    }
    void Callback::Callback_MCMemoryDeleteArray(void *array)
    {
        return MCMemoryDeleteArray(array);
    }
    bool Callback::Callback_MCMemoryResizeArray(uindex_t new_count, size_t size, void*& x_array, uindex_t& x_count)
    {
        return MCMemoryResizeArray(new_count, size, x_array, x_count);
    }
    void Callback::Callback___MCAssert(const char *file, uint32_t line, const char *message)
    {
#if defined(DEBUG_LOG)
        __MCAssert(file, line, message);
#endif
    }
    void Callback::Callback___MCUnreachable(void)
    {
#if defined(DEBUG_LOG)
        __MCUnreachable();
#endif
    }
}  /* namespace MCPlatform */

