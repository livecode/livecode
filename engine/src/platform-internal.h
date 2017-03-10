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

#ifndef __MC_PLATFORM_INTERNAL__
#define __MC_PLATFORM_INTERNAL__

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

#define MC_PLATFORM_INTERNAL_INSIDE
#include "platform-internal-base.h"
#undef MC_PLATFORM_INTERNAL_INSIDE

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to match the new stack surface API.
//  You can now lock/unlock multiple areas of the surface, but need to store the context and raster for those areas locally.
class MCPlatformSurface
{
public:
	MCPlatformSurface(void);
	virtual ~MCPlatformSurface(void);
	
	virtual bool LockGraphics(MCGIntegerRectangle area, MCGContextRef& r_context, MCGRaster &r_raster) = 0;
	virtual void UnlockGraphics(MCGIntegerRectangle area, MCGContextRef context, MCGRaster &raster) = 0;
	
	virtual bool LockPixels(MCGIntegerRectangle area, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area) = 0;
	virtual void UnlockPixels(MCGIntegerRectangle area, MCGRaster& raster) = 0;
	
	virtual bool LockSystemContext(void*& r_context) = 0;
	virtual void UnlockSystemContext(void) = 0;
	
	virtual bool Composite(MCGRectangle dst_rect, MCGImageRef src_image, MCGRectangle src_rect, MCGFloat opacity, MCGBlendMode blend) = 0;

	virtual MCGFloat GetBackingScaleFactor(void) = 0;
};

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {

class WindowMask: public Base
{
public:
    WindowMask(void);
    virtual ~WindowMask(void);
    
    virtual bool IsValid(void) const = 0;

    virtual bool CreateWithAlphaAndRelease(int32_t p_width, int32_t p_height, int32_t p_stride, void *p_bits) = 0;
};

typedef Ref<WindowMask> WindowMaskRef;

} /* namespace MCPlatform */

typedef MCPlatform::WindowMask MCPlatformWindowMask;

////////////////////////////////////////////////////////////////////////////////

typedef void (*MCPlatformWindowAttachmentCallback)(void *object, bool realized);

struct MCPlatformWindowAttachment
{
	void *object;
	MCPlatformWindowAttachmentCallback callback;
};

class MCPlatformWindow: public MCPlatform::CoreReference
{
public:
	MCPlatformWindow(void);
	virtual ~MCPlatformWindow(void);
	
	// Returns true if the window is being shown.
	bool IsVisible(void);
	
	// Force an immediate update of the window's dirty region. Assuming there
	// is updates to be made, this will cause an immediate redraw callback.
	void Update(void);
	
	// Add the given region to the window's dirty region.
	void Invalidate(MCGRegionRef region);
	
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
	
	void HandleRedraw(MCPlatformSurfaceRef surface, MCGRegionRef update_rgn);
	void HandleReshape(MCRectangle new_content);
	void HandleIconify(void);
	void HandleUniconify(void);
	void HandleFocus(void);
	void HandleUnfocus(void);
	
	void HandleKeyDown(MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
	void HandleKeyUp(MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
	
	void HandleDragEnter(class MCRawClipboard* p_dragboard, MCPlatformDragOperation& r_operation);
	void HandleDragMove(MCPoint location, MCPlatformDragOperation& r_operation);
	void HandleDragLeave(void);
	void HandleDragDrop(bool& r_accepted);
    // Called to tell attachments there is a handle.
    void RealizeAndNotify(void);
    
    //////////
	
    virtual void SwitchFocusToView(uint32_t p_id) = 0;
    
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
		bool is_opaque_changed : 1;
		bool has_modified_mark_changed : 1;
		bool use_live_resizing_changed : 1;
        
        // MW-2014-04-08: [[ Bug 12073 ]] Changed flag for mouse cursor.
        bool cursor_changed : 1;
        
        bool hides_on_suspend_changed : 1;
        
        // MERG-2014-06-02: [[ IgnoreMouseEvents ]] Changed flag for ignore mouse events.
        bool ignore_mouse_events_changed : 1;
        
        // MERG-2015-10-11: [[ DocumentFilename ]] Changed flag for docuent filename
        bool document_filename_changed : 1;
	} m_changes;
	MCPlatformWindowStyle m_style;
	MCStringRef m_title;
	MCPlatformWindowMaskRef m_mask;
	float m_opacity;
	MCRectangle m_content;
	MCCursorRef m_cursor;
    // MERG-2015-10-11: [[ DocumentFilename ]] documentFilename property
    MCStringRef m_document_filename;
	struct
	{
		bool m_has_title_widget : 1;
		bool m_has_close_widget : 1;
		bool m_has_collapse_widget : 1;
		bool m_has_zoom_widget : 1;
		bool m_has_size_widget : 1;
		bool m_has_shadow : 1;
		bool m_is_opaque : 1;
		bool m_has_modified_mark : 1;
		bool m_use_live_resizing : 1;
        bool m_hides_on_suspend : 1;
        // MERG-2014-06-02: [[ IgnoreMouseEvents ]] ignoreMouseEvents property
        bool m_ignore_mouse_events : 1;
	};
	
	// Universal state.
	MCGRegionRef m_dirty_region;
	struct
	{
		bool m_is_visible : 1;
		bool m_is_focused : 1;
		bool m_is_iconified : 1;
		bool m_use_text_input : 1;
        bool m_is_realized : 1;
	};
};

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {

class ColorTransform: public virtual Base
{
public:
    virtual bool CreateWithColorSpace(const MCColorSpaceInfo& p_info) = 0;
    virtual bool Apply(MCImageBitmap *p_image) = 0;
};

typedef Ref<ColorTransform> ColorTransformRef;

} /* namespace MCPlatform */

typedef MCPlatform::ColorTransform MCPlatformColorTransform;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {

class LoadedFont: public virtual Base {
public:
    virtual bool CreateWithPath(MCStringRef p_path, bool p_globally) = 0;
};

typedef Ref<LoadedFont> LoadedFontRef;

} /* namespace MCPlatform */

typedef MCPlatform::LoadedFont MCPlatformLoadedFont;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {

class Cursor: public virtual Base
{
public:
    virtual void CreateStandard(MCPlatformStandardCursor p_standard_cursor) = 0;
    virtual void CreateCustom(MCImageBitmap *p_image, MCPoint p_hotspot) = 0;
    virtual void Set(void) = 0;
};

typedef Ref<Cursor> CursorRef;

} /* namespace MCPlatform */

typedef MCPlatform::Cursor MCPlatformCursor;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {

class Menu: public CoreReference
{
public:
    virtual void SetTitle(MCStringRef p_title) = 0;
    virtual uindex_t CountItems(void) = 0;
    virtual void AddItem(uindex_t p_where) = 0;
    virtual void AddSeparatorItem(uindex_t p_where) = 0;
    virtual void RemoveItem(uindex_t p_where) = 0;
    virtual void RemoveAllItems(void) = 0;

    /* Warning: the pointer returned into r_parent is _not_ owned by the
     * caller. */
    /* TODO [2017-02-24] Maybe this should return a
     * std::tuple<Ref<Menu>, uindex_t> */
    virtual void GetParent(MCPlatformMenuRef& r_parent, uindex_t& r_index) = 0;

    virtual void GetItemProperty(uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, void *r_value) = 0;
    virtual void SetItemProperty(uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, const void *p_value) = 0;
    virtual bool PopUp(MCPlatformWindowRef p_window, MCPoint p_location, uindex_t p_item) = 0;
    virtual void StartUsingAsMenubar(void) = 0;
    virtual void StopUsingAsMenubar(void) = 0;
};

typedef Ref<Menu> MenuRef;

} /* namespace MCPlatform */

typedef MCPlatform::Menu MCPlatformMenu;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
class PrintDialogSession: public Base
{
public:
    virtual void BeginPageSetup(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format) = 0;
    virtual void BeginSettings(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format) = 0;
    virtual void CopyInfo(void *&r_session, void *&r_settings, void *&r_page_format) = 0;
    virtual void SetResult(MCPlatformPrintDialogResult p_result) = 0;
    virtual MCPlatformPrintDialogResult GetResult(void) = 0;
};

typedef Ref<PrintDialogSession> PrintDialogSessionRef;
    
} /* namespace MCPlatform */

typedef MCPlatform::PrintDialogSession MCPlatformPrintDialogSession;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {

class Sound: public Base
{
public:
    virtual bool IsValid(void) const = 0;
    
    virtual bool CreateWithData(const void *data, size_t data_size) = 0;
    
    virtual bool IsPlaying(void) const = 0;
    
    virtual void Play(void) = 0;
    virtual void Pause(void) = 0;
    virtual void Resume(void) = 0;
    virtual void Stop(void) = 0;
    
    virtual void SetProperty(MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value) = 0;
    virtual void GetProperty(MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value) = 0;
};

typedef Ref<Sound> SoundRef;
}

typedef MCPlatform::Sound MCPlatformSound;

////////////////////////////////////////////////////////////////////////////////

class MCPlatformPlayer: public MCPlatform::CoreReference
{
public:
	MCPlatformPlayer(void);
	virtual ~MCPlatformPlayer(void);
	
	virtual bool GetNativeView(void *&r_view) = 0;
	virtual bool SetNativeParentView(void *p_parent_view) = 0;
	
	virtual bool IsPlaying(void) = 0;
	// PM-2014-05-28: [[ Bug 12523 ]] Take into account the playRate property
	virtual void Start(double rate) = 0;
	virtual void Stop(void) = 0;
	virtual void Step(int amount) = 0;
	
	virtual bool LockBitmap(const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap) = 0;
	virtual void UnlockBitmap(MCImageBitmap *bitmap) = 0;
	
	virtual void SetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value) = 0;
	virtual void GetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value) = 0;
	
	virtual void CountTracks(uindex_t& r_count) = 0;
	virtual bool FindTrackWithId(uint32_t id, uindex_t& r_index) = 0;
	virtual void SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value) = 0;
	virtual void GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value) = 0;
	
protected:
	virtual void Realize(void) = 0;
	virtual void Unrealize(void) = 0;
	
};

////////////////////////////////////////////////////////////////////////////////

class MCPlatformSoundRecorder: public MCPlatform::Base
{
public:
	MCPlatformSoundRecorder(void);
	virtual ~MCPlatformSoundRecorder(void);
    
	virtual bool IsRecording(void);
    
    virtual void SetProperty(MCPlatformSoundRecorderProperty property, MCPlatformPropertyType type, void *value);
    virtual void GetProperty(MCPlatformSoundRecorderProperty property, MCPlatformPropertyType type, void *value);
    
    virtual void GetConfiguration(MCPlatformSoundRecorderConfiguration &r_config);
    virtual void SetConfiguration(const MCPlatformSoundRecorderConfiguration p_config);
    
    virtual void BeginDialog(void) = 0;
    virtual MCPlatformDialogResult EndDialog(void) = 0;
    virtual bool StartRecording(MCStringRef filename) = 0;
    virtual void StopRecording(void) = 0;
    virtual void PauseRecording(void) = 0;
    virtual void ResumeRecording(void) = 0;
    virtual double GetLoudness(void) = 0;
    
    virtual bool ListInputs(MCPlatformSoundRecorderListInputsCallback callback, void *context) = 0;
    virtual bool ListCompressors(MCPlatformSoundRecorderListCompressorsCallback callback, void *context) = 0;
    
protected:

    bool m_recording;;
    MCStringRef m_filename;
    
    // The recorder's current configuration settings.
     MCPlatformSoundRecorderConfiguration m_configuration;
};

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class Core: public Base
    {
    public:
        virtual int Run(int argc, char *argv[], char *envp[]) = 0;
        
        // Wait
        virtual bool WaitForEvent(double p_duration, bool p_blocking) = 0;
        virtual void BreakWait(void) = 0;
        
        // Callbacks
        virtual void ScheduleCallback(void (*p_callback)(void *), void *p_context) = 0;
    
        // Abort key
        virtual bool InitializeAbortKey(void) = 0;
        virtual void FinalizeAbortKey(void) = 0;
        virtual bool GetAbortKeyPressed(void) = 0;
        
        // Color transform
        virtual bool InitializeColorTransform(void) = 0;
        virtual void FinalizeColorTransform(void) = 0;
        
        // Menus
        virtual MCPlatformMenuRef CreateMenu(void) = 0;
        virtual bool InitializeMenu(void) = 0;
        virtual void FinalizeMenu(void) = 0;
        virtual void ShowMenubar(void) = 0;
        virtual void HideMenubar(void) = 0;
        virtual void SetMenubar(MCPlatformMenuRef p_menu) = 0;
        virtual MCPlatformMenuRef GetMenubar(void) = 0;
        virtual void SetIconMenu(MCPlatformMenuRef p_menu) = 0;
        virtual void SaveQuittingState() = 0;
        virtual void PopQuittingState() = 0;
        virtual bool IsInQuittingState(void) = 0;
        virtual void LockMenuSelect(void) = 0;
        virtual void UnlockMenuSelect(void) = 0;
        
        // Dialogs
        virtual void ShowMessageDialog(MCStringRef p_title, MCStringRef p_message) = 0;
    
        // Windows
        virtual MCPlatformWindowRef CreateWindow(void) = 0;
        virtual bool GetWindowWithId(uint32_t p_id, MCPlatformWindowRef& r_window) = 0;
        
        // Color dialog
        virtual void BeginColorDialog(MCStringRef p_title, const MCColor& p_color) = 0;
        virtual MCPlatformDialogResult EndColorDialog(MCColor& r_color) = 0;
        
        
        // File & folder dialog
        virtual void BeginFileDialog(MCPlatformFileDialogKind p_kind, MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file) = 0;
        virtual MCPlatformDialogResult EndFileDialog(MCPlatformFileDialogKind p_kind, MCStringRef &r_paths, MCStringRef &r_type) = 0;
        virtual void BeginFolderDialog(MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial) = 0;
        virtual MCPlatformDialogResult EndFolderDialog(MCStringRef& r_selected_folder) = 0;
    
        // System Properties
        virtual void GetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *r_value) = 0;
        virtual void SetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *p_value) = 0;
        
        // Player
        virtual MCPlatformPlayerRef CreatePlayer(void) = 0;
        
        // Snapshots
        virtual void ScreenSnapshotOfUserArea(MCPoint *p_size, MCImageBitmap*& r_bitmap) = 0;
        virtual void ScreenSnapshotOfWindow(uint32_t p_window_id, MCPoint *p_size, MCImageBitmap*& r_bitmap) = 0;
        virtual void ScreenSnapshotOfWindowArea(uint32_t p_window_id, MCRectangle p_area, MCPoint *p_size, MCImageBitmap*& r_bitmap) = 0;
        virtual void ScreenSnapshot(MCRectangle p_screen_rect, MCPoint *p_size, MCImageBitmap*& r_bitmap) = 0;
        
        virtual void Beep(void) = 0;
        
        virtual void DeathGrip(MCPlatform::Base * p_window) = 0;
        
        // Events
        virtual void FlushEvents(MCPlatformEventMask p_mask) = 0;
        virtual uint32_t GetEventTime(void) = 0;
        
        // Mice
        virtual void HideCursorUntilMouseMoves(void) = 0;
        virtual bool GetMouseButtonState(uindex_t p_button) = 0;
        virtual bool GetMouseClick(uindex_t p_button, MCPoint& r_location) = 0;
        virtual void GetMousePosition(MCPoint& r_location) = 0;
        virtual void SetMousePosition(MCPoint p_location) = 0;
        virtual void GetWindowAtPoint(MCPoint p_loc, MCPlatformWindowRef& r_window) = 0;
        virtual void GrabPointer(MCPlatformWindowRef p_window) = 0;
        virtual void UngrabPointer(void) = 0;
        
        // Modifier Keys
        virtual MCPlatformModifiers GetModifiersState(void) = 0;
        virtual bool GetKeyState(MCPlatformKeyCode*& r_codes, uindex_t& r_code_count) = 0;
        
        // Drag and drop
        virtual void DoDragDrop(MCPlatformWindowRef p_window, MCPlatformAllowedDragOperations p_allowed_operations, MCImageBitmap *p_image, const MCPoint *p_image_loc, MCPlatformDragOperation& r_operation) = 0;
    
        // Screens
        virtual void GetScreenCount(uindex_t& r_count) = 0;
        virtual void GetScreenViewport(uindex_t p_index, MCRectangle& r_viewport) = 0;
        virtual void GetScreenWorkarea(uindex_t p_index, MCRectangle& r_workarea) = 0;
        virtual void GetScreenPixelScale(uindex_t p_index, MCGFloat& r_scale) = 0;
        virtual void DisableScreenUpdates(void) = 0;
        virtual void EnableScreenUpdates(void) = 0;

        // Backdrop
        virtual void ConfigureBackdrop(MCPlatformWindowRef p_backdrop_window) = 0;
    };
    
    typedef Ref<Core> CoreRef;
    
} /* namespace MCPlatform */

typedef MCPlatform::Core MCPlatformCore;

////////////////////////////////////////////////////////////////////////////////

void MCPlatformWindowDeathGrip(MCPlatformWindowRef window);

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCallbackSendApplicationStartup(int argc, MCStringRef *argv, MCStringRef *envp, int& r_error_code, MCStringRef& r_error_message);
void MCPlatformCallbackSendApplicationShutdown(int& r_exit_code);
void MCPlatformCallbackSendApplicationShutdownRequest(bool& r_terminate);
void MCPlatformCallbackSendApplicationRun(bool& r_continue);
void MCPlatformCallbackSendApplicationSuspend(void);
void MCPlatformCallbackSendApplicationResume(void);

void MCPlatformCallbackSendScreenParametersChanged(void);

void MCPlatformCallbackSendWindowCloseRequest(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowClose(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowCancel(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowReshape(MCPlatformWindowRef window, MCRectangle new_content);
void MCPlatformCallbackSendWindowConstrain(MCPlatformWindowRef window, MCPoint proposed_size, MCPoint& r_wanted_size);
void MCPlatformCallbackSendWindowRedraw(MCPlatformWindowRef window, MCPlatformSurfaceRef surface, MCGRegionRef dirty_rgn);
void MCPlatformCallbackSendWindowIconify(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowUniconify(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowFocus(MCPlatformWindowRef window);
void MCPlatformCallbackSendWindowUnfocus(MCPlatformWindowRef window);

void MCPlatformCallbackSendModifiersChanged(MCPlatformModifiers modifiers);
											
void MCPlatformCallbackSendMouseDown(MCPlatformWindowRef window, uint32_t button, uint32_t count);
void MCPlatformCallbackSendMouseUp(MCPlatformWindowRef window, uint32_t button, uint32_t count);
void MCPlatformCallbackSendMouseDrag(MCPlatformWindowRef window, uint32_t button);
void MCPlatformCallbackSendMouseRelease(MCPlatformWindowRef window, uint32_t button, bool was_menu);
void MCPlatformCallbackSendMouseEnter(MCPlatformWindowRef window);
void MCPlatformCallbackSendMouseLeave(MCPlatformWindowRef window);
void MCPlatformCallbackSendMouseMove(MCPlatformWindowRef window, MCPoint location);
void MCPlatformCallbackSendMouseScroll(MCPlatformWindowRef window, int32_t dx, int32_t dy);

void MCPlatformCallbackSendDragEnter(MCPlatformWindowRef window, class MCRawClipboard* p_dragboard, MCPlatformDragOperation& r_operation);
void MCPlatformCallbackSendDragLeave(MCPlatformWindowRef window);
void MCPlatformCallbackSendDragMove(MCPlatformWindowRef window, MCPoint location, MCPlatformDragOperation& r_operation);
void MCPlatformCallbackSendDragDrop(MCPlatformWindowRef window, bool& r_accepted);

void MCPlatformCallbackSendRawKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);

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

void MCPlatformCallbackSendViewFocusSwitched(MCPlatformWindowRef window, uint32_t view_id);

void MCPlatformCallbackSendPlayerFrameChanged(MCPlatformPlayerRef player);
void MCPlatformCallbackSendPlayerMarkerChanged(MCPlatformPlayerRef player, MCPlatformPlayerDuration time);
void MCPlatformCallbackSendPlayerCurrentTimeChanged(MCPlatformPlayerRef player);
void MCPlatformCallbackSendPlayerFinished(MCPlatformPlayerRef player);
void MCPlatformCallbackSendPlayerBufferUpdated(MCPlatformPlayerRef player);

void MCPlatformCallbackSendSoundFinished(MCPlatformSoundRef sound);

////////////////////////////////////////////////////////////////////////////////

#endif
