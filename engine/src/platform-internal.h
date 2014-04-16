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

#ifndef __MC_PLATFORM_INTERNAL__
#define __MC_PLATFORM_INTERNAL__

////////////////////////////////////////////////////////////////////////////////

inline MCRectangle MCRectangleMake(int x, int y, int w, int h)
{
	MCRectangle r;
	r . x = x;
	r . y = y;
	r . width = w;
	r . height = h;
	return r;
}

inline MCPoint MCPointMake(int x, int y)
{
	MCPoint p;
	p . x = x;
	p . y = y;
	return p;
}

////////////////////////////////////////////////////////////////////////////////

class MCPlatformSurface
{
public:
	MCPlatformSurface(void);
	virtual ~MCPlatformSurface(void);
	
	virtual bool LockGraphics(MCRegionRef region, MCGContextRef& r_context) = 0;
	virtual void UnlockGraphics(void) = 0;
	
	virtual bool LockPixels(MCRegionRef region, MCGRaster& r_raster) = 0;
	virtual void UnlockPixels(void) = 0;
	
	virtual bool LockSystemContext(void*& r_context) = 0;
	virtual void UnlockSystemContext(void) = 0;
	
	virtual bool Composite(MCGRectangle dst_rect, MCGImageRef src_image, MCGRectangle src_rect, MCGFloat opacity, MCGBlendMode blend) = 0;

	virtual MCGFloat GetBackingScaleFactor(void) = 0;
};

////////////////////////////////////////////////////////////////////////////////

typedef void (*MCPlatformWindowAttachmentCallback)(void *object, bool realized);

struct MCPlatformWindowAttachment
{
	void *object;
	MCPlatformWindowAttachmentCallback callback;
};

class MCPlatformWindow
{
public:
	MCPlatformWindow(void);
	virtual ~MCPlatformWindow(void);
	
	// Increase the reference count on the window.
	void Retain(void);
	
	// Decrease the reference count on the window.
	void Release(void);
	
	// Returns true if the window is being shown.
	bool IsVisible(void);
	
	// Force an immediate update of the window's dirty region. Assuming there
	// is updates to be made, this will cause an immediate redraw callback.
	void Update(void);
	
	// Add the given region to the window's dirty region.
	void Invalidate(MCRegionRef region);
	
	// Make the window visible as the given class.
	void Show(void);
	void ShowAsSheet(MCPlatformWindowRef parent);
	
	// Make the window invisible.
	void Hide(void);
	
	// Set input focus to the window.
	void Focus(void);
	
	// Bring the window to front.
	void Raise(void);
	
	// Minimize / miniturize the window.
	void Iconify(void);
	
	// Deminimize / deminiturize the window.
	void Uniconify(void);
	
	// Manage text input sessions
	void ConfigureTextInput(bool enabled);
	void ResetTextInput(void);
	bool IsTextInputActive(void);
	
	// Set the given window property.
	void SetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, const void *value);
	
	// Get the given window property.
	void GetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, void *r_value);
	
	// Map co-ords from window to screen and vice-versa.
	void MapPointFromScreenToWindow(MCPoint screen_point, MCPoint& r_window_point);
	void MapPointFromWindowToScreen(MCPoint window_point, MCPoint& r_screen_point);
	
	// Attach an object that needs to be notified when the window is realized / unrealized.
	void AttachObject(void *object, MCPlatformWindowAttachmentCallback callback);
	void DetachObject(void *object);
	
public:
	
	void HandleCloseRequest(void);
	
	void HandleRedraw(MCPlatformSurfaceRef surface, MCRegionRef update_rgn);
	void HandleReshape(MCRectangle new_content);
	void HandleIconify(void);
	void HandleUniconify(void);
	void HandleFocus(void);
	void HandleUnfocus(void);
	
	void HandleKeyDown(MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
	void HandleKeyUp(MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
	
	void HandleDragEnter(MCPlatformPasteboardRef pasteboard, MCPlatformDragOperation& r_operation);
	void HandleDragMove(MCPoint location, MCPlatformDragOperation& r_operation);
	void HandleDragLeave(void);
	void HandleDragDrop(bool& r_accepted);
	
	//////////
	
	virtual void DoRealize(void) = 0;
	virtual void DoSynchronize(void) = 0;
	
	virtual bool DoSetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, const void *value) = 0;
	virtual bool DoGetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, void *r_value) = 0;
	
	virtual void DoShow(void) = 0;
	virtual void DoShowAsSheet(MCPlatformWindowRef parent) = 0;
	virtual void DoHide(void) = 0;
	virtual void DoFocus(void) = 0;
	virtual void DoRaise(void) = 0;
	virtual void DoUpdate(void) = 0;
	virtual void DoIconify(void) = 0;
	virtual void DoUniconify(void) = 0;
	virtual void DoConfigureTextInput(void) = 0;
	virtual void DoResetTextInput(void) = 0;
	
	virtual void DoMapContentRectToFrameRect(MCRectangle content, MCRectangle& r_frame) = 0;
	virtual void DoMapFrameRectToContentRect(MCRectangle frame, MCRectangle& r_content) = 0;
	
protected:
	// Called to tell attachments there is a handle.
	void RealizeAndNotify(void);
	
	// The window's reference count.
	uint32_t m_references;
	
	// Any attachments the window has.
	MCPlatformWindowAttachment *m_attachments;
	uindex_t m_attachment_count;
	
	// Universal property values.
	struct 
	{
		bool style_changed : 1;
		bool opacity_changed : 1;
		bool mask_changed : 1;
		bool content_changed : 1;
		bool title_changed : 1;
		bool has_title_widget_changed : 1;
		bool has_close_widget_changed : 1;
		bool has_collapse_widget_changed : 1;
		bool has_zoom_widget_changed : 1;
		bool has_size_widget_changed : 1;
		bool has_shadow_changed : 1;
		bool has_modified_mark_changed : 1;
		bool use_live_resizing_changed : 1;
        
        // MW-2014-04-08: [[ Bug 12073 ]] Changed flag for mouse cursor.
        bool cursor_changed : 1;
	} m_changes;
	MCPlatformWindowStyle m_style;
	char *m_title;
	MCPlatformWindowMaskRef m_mask;
	float m_opacity;
	MCRectangle m_content;
	MCCursorRef m_cursor;
	struct
	{
		bool m_has_title_widget : 1;
		bool m_has_close_widget : 1;
		bool m_has_collapse_widget : 1;
		bool m_has_zoom_widget : 1;
		bool m_has_size_widget : 1;
		bool m_has_shadow : 1;
		bool m_has_modified_mark : 1;
		bool m_use_live_resizing : 1;
	};
	
	// Universal state.
	MCRegionRef m_dirty_region;
	struct
	{
		bool m_is_visible : 1;
		bool m_is_focused : 1;
		bool m_is_iconified : 1;
		bool m_use_text_input : 1;
	};
};

////////////////////////////////////////////////////////////////////////////////

void MCPlatformWindowDeathGrip(MCPlatformWindowRef window);

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCallbackSendApplicationStartup(int argc, char **argv, char **envp, int& r_error_code, char*& r_error_message);
void MCPlatformCallbackSendApplicationShutdown(int& r_exit_code);
void MCPlatformCallbackSendApplicationShutdownRequest(bool& r_terminate);
void MCPlatformCallbackSendApplicationRun(void);
void MCPlatformCallbackSendApplicationSuspend(void);
void MCPlatformCallbackSendApplicationResume(void);

void MCPlatformCallbackSendScreenParametersChanged(void);

void MCPlatformCallbackSendWindowCloseRequest(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowClose(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowReshape(MCPlatformWindowRef window, MCRectangle new_content);
void MCPlatformCallbackSendWindowConstrain(MCPlatformWindowRef window, MCPoint proposed_size, MCPoint& r_wanted_size);
void MCPlatformCallbackSendWindowRedraw(MCPlatformWindowRef window, MCPlatformSurfaceRef surface, MCRegionRef dirty_rgn);
void MCPlatformCallbackSendWindowIconify(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowUniconify(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowFocus(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowUnfocus(MCPlatformWindowRef window);

void MCPlatformCallbackSendModifiersChanged(MCPlatformModifiers modifiers);
											
void MCPlatformCallbackSendMouseDown(MCPlatformWindowRef window, uint32_t button, uint32_t count);
void MCPlatformCallbackSendMouseUp(MCPlatformWindowRef window, uint32_t button, uint32_t count);
void MCPlatformCallbackSendMouseDrag(MCPlatformWindowRef window, uint32_t button);
void MCPlatformCallbackSendMouseRelease(MCPlatformWindowRef window, uint32_t button);
void MCPlatformCallbackSendMouseEnter(MCPlatformWindowRef window);
void MCPlatformCallbackSendMouseLeave(MCPlatformWindowRef window);
void MCPlatformCallbackSendMouseMove(MCPlatformWindowRef window, MCPoint location);
void MCPlatformCallbackSendMouseScroll(MCPlatformWindowRef window, int32_t dx, int32_t dy);

void MCPlatformCallbackSendDragEnter(MCPlatformWindowRef window, MCPlatformPasteboardRef pasteboard, MCPlatformDragOperation& r_operation);
void MCPlatformCallbackSendDragLeave(MCPlatformWindowRef window);
void MCPlatformCallbackSendDragMove(MCPlatformWindowRef window, MCPoint location, MCPlatformDragOperation& r_operation);
void MCPlatformCallbackSendDragDrop(MCPlatformWindowRef window, bool& r_accepted);

void MCPlatformCallbackSendRawKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code);

void MCPlatformCallbackSendKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
void MCPlatformCallbackSendKeyUp(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);

void MCPlatformCallbackSendTextInputQueryTextRanges(MCPlatformWindowRef window, MCRange& r_marked_range, MCRange& r_selected_range);
void MCPlatformCallbackSendTextInputQueryTextIndex(MCPlatformWindowRef window, MCPoint location, uindex_t& r_index);
void MCPlatformCallbackSendTextInputQueryTextRect(MCPlatformWindowRef window, MCRange range, MCRectangle& first_line_rect, MCRange& r_actual_range);
void MCPlatformCallbackSendTextInputQueryText(MCPlatformWindowRef window, MCRange range, unichar_t*& r_chars, uindex_t& r_char_count, MCRange& r_actual_range);
void MCPlatformCallbackSendTextInputInsertText(MCPlatformWindowRef window, unichar_t *chars, uindex_t char_count, MCRange replace_range, MCRange selection_range, bool mark);
void MCPlatformCallbackSendTextInputAction(MCPlatformWindowRef window, MCPlatformTextInputAction action);

void MCPlatformCallbackSendMenuUpdate(MCPlatformMenuRef menu);
void MCPlatformCallbackSendMenuSelect(MCPlatformMenuRef menu, uindex_t item);

void MCPlatformCallbackSendPasteboardResolve(MCPlatformPasteboardRef pasteboard, MCPlatformPasteboardFlavor flavor, void *handle, void*& r_data, size_t& r_data_size);

void MCPlatformCallbackSendViewFocusSwitched(MCPlatformWindowRef window, uint32_t view_id);

void MCPlatformCallbackSendPlayerFrameChanged(MCPlatformPlayerRef player);
void MCPlatformCallbackSendPlayerMarkerChanged(MCPlatformPlayerRef player, uint32_t time);
void MCPlatformCallbackSendPlayerSelectionChanged(MCPlatformPlayerRef player);
void MCPlatformCallbackSendPlayerCurrentTimeChanged(MCPlatformPlayerRef player);
void MCPlatformCallbackSendPlayerStarted(MCPlatformPlayerRef player);
void MCPlatformCallbackSendPlayerPaused(MCPlatformPlayerRef player);
void MCPlatformCallbackSendPlayerStopped(MCPlatformPlayerRef player);

void MCPlatformCallbackSendSoundFinished(MCPlatformSoundRef sound);

////////////////////////////////////////////////////////////////////////////////

#endif
