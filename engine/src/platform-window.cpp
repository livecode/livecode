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
#include "platform-internal.h"

#include "region.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Platform Window Class Implementation
//

MCPlatformWindow::MCPlatformWindow(void)
{
    //MCLog("Create window %p", this);
	
	m_references = 1;
	
	m_attachments = nil;
	m_attachment_count = 0;
	
	MCMemoryClear(&m_changes, sizeof(m_changes));
	m_style = kMCPlatformWindowStyleDocument;
    // SN-2014-06-23: Title updated to StringRef
	m_title = nil;
	m_opacity = 1.0f;
	m_content = MCRectangleMake(0, 0, 0, 0);
	m_mask = nil;
	m_cursor = nil;
	m_has_title_widget = false;
	m_has_close_widget = false;
	m_has_collapse_widget = false;
	m_has_zoom_widget = false;
	m_has_size_widget = false;
	m_has_shadow = false;
	m_has_modified_mark = false;
	m_use_live_resizing = false;
    m_hides_on_suspend = false;
    // MERG-2014-06-02: [[ IgnoreMouseEvents ]] Default ignoreMouseEvents to false
    m_ignore_mouse_events = false;
	
    // MW-2014-05-02: [[ Bug 12348 ]] Make sure we initialize this value appropriately.
    m_use_text_input = false;
    
	/* UNCHECKED */ MCRegionCreate(m_dirty_region);
	m_is_visible = false;
	m_is_focused = false;
	m_is_iconified = false;
    m_is_realized = false;
	
	m_is_opaque = true;
    
    // MERG-2015-10-11: [[ DocumentFilename ]] documentFilename property
    m_document_filename = MCValueRetain(kMCEmptyString);
}

MCPlatformWindow::~MCPlatformWindow(void)
{
    //MCLog("Destroy window %p", this);
	
    if (m_cursor != nullptr)
    {
        MCPlatformReleaseCursor(m_cursor);
    }
    
	MCRegionDestroy(m_dirty_region);
	
	MCPlatformWindowMaskRelease(m_mask);
    // SN-2014-06-23: Title updated to StringRef
	MCValueRelease(m_title);
	
    // MERG-2015-10-11: [[ DocumentFilename ]] documentFilename property
    MCValueRelease(m_document_filename);
    
	free(m_attachments);
}

void MCPlatformWindow::Retain(void)
{
	m_references += 1;
}

void MCPlatformWindow::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}

void MCPlatformWindow::Update(void)
{
	// If the window is not visible, there is nothing to do.
	if (!m_is_visible)
		return;
	
	// If the window dirty region is empty, there is nothing to do.
	if (MCRegionIsEmpty(m_dirty_region))
		return;
	
	DoUpdate();

	// Now that we have updated, make sure we empty the dirty region!
	MCRegionSetEmpty(m_dirty_region);
}

void MCPlatformWindow::Invalidate(MCRegionRef p_region)
{
	// If the window is not visible, there is nothing to do.
	if (!m_is_visible)
		return;
    
	// Union the dirty region.
    if (p_region == nil)
        MCRegionIncludeRect(m_dirty_region, m_content);
    else
        MCRegionAddRegion(m_dirty_region, p_region);
}

void MCPlatformWindow::Show(void)
{
	// If the window is already visible, do nothing.
	if (m_is_visible)
		return;
	
	// Make sure the window has been created.
    if (!m_is_realized)
        RealizeAndNotify();
	
	// Update the state.
	m_is_visible = true;
	
	// Show the window.
	DoShow();
}

void MCPlatformWindow::ShowAsSheet(MCPlatformWindowRef p_parent)
{
	// If the window is already visible, do nothing.
	if (m_is_visible)
		return;
	
	// If the parent isn't visible, or isn't a suitable parent then
	// dispatch a close event.
	if (!p_parent -> m_is_visible ||
		(p_parent -> m_style != kMCPlatformWindowStyleDocument &&
		 p_parent -> m_style != kMCPlatformWindowStyleDialog &&
		 p_parent -> m_style != kMCPlatformWindowStylePalette &&
		 p_parent -> m_style != kMCPlatformWindowStyleUtility))
	{
		MCPlatformCallbackSendWindowClose(this);
		return;
	}
		
	// Make sure the window has been created.
	RealizeAndNotify();
	
	// Update the state.
	m_is_visible = true;
	
	// Show the window.
	DoShowAsSheet(p_parent);
}

void MCPlatformWindow::Hide(void)
{
	// Do nothing if the window is not visible.
	if (!m_is_visible)
		return;
	
	// Update the state.
	m_is_visible = false;
    
	// CW-2015-19-22: [[ Bug 15979 ]] Reset m_is_iconified, otherwise if the window is reopened, it cannot be iconified.
	m_is_iconified = false;
	
	// Hide the window.
	DoHide();
}

void MCPlatformWindow::Focus(void)
{
	// Do nothing if the window is not visible.
	if (!m_is_visible)
		return;
	
	// Do nothing if the window is currently focused.
	if (m_is_focused)
		return;
	
	// Focus the window.
	DoFocus();
	
	// Update the state.
	m_is_focused = true;
}

void MCPlatformWindow::Raise(void)
{
	// Do nothing if the window is not visible.
	if (!m_is_visible)
		return;
	
	// Raise the window.
	DoRaise();
}

void MCPlatformWindow::Iconify(void)
{
	// Do nothing if the window is not visible.
	if (!m_is_visible)
		return;
	
	// Do nothing if the window is iconified.
	if (m_is_iconified)
		return;
	
	// Iconify the window.
	DoIconify();
	
	// Update the state.
	m_is_iconified = true;
}

void MCPlatformWindow::Uniconify(void)
{
	// Do nothing if the window is not visible.
	if (!m_is_visible)
		return;
	
	// Do nothing if the window is not iconified.
	if (!m_is_iconified)
		return;
	
	// Iconify the window.
	DoUniconify();
	
	// Update the state.
	m_is_iconified = false;
}

bool MCPlatformWindow::IsTextInputActive(void)
{
	return m_use_text_input;
}

void MCPlatformWindow::ConfigureTextInput(bool p_activate)
{
	if (p_activate == m_use_text_input)
		return;
	
	m_use_text_input = p_activate;
	
	DoConfigureTextInput();
}

void MCPlatformWindow::ResetTextInput(void)
{
	if (!m_use_text_input)
		return;
	
	DoResetTextInput();
}

bool MCPlatformWindow::IsVisible(void)
{
	return m_is_visible;
}

void MCPlatformWindow::SetProperty(MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, const void *p_value)
{
	// First see if a descendent wants to manage the property.
	if (DoSetProperty(p_property, p_type, p_value))
		return;
	
	// Otherwise perform default handling for the known properties.
	switch(p_property)
	{
		case kMCPlatformWindowPropertyStyle:
			assert(p_type == kMCPlatformPropertyTypeWindowStyle);
			m_style = *(MCPlatformWindowStyle *)p_value;
			m_changes . style_changed = true;
			break;
		case kMCPlatformWindowPropertyTitle:
			assert(p_type == kMCPlatformPropertyTypeMCString);
            // SN-2014-06-23: Title updated to StringRef
            MCValueAssign(m_title, *(MCStringRef*)p_value);
			m_changes . title_changed = true;
			break;
		case kMCPlatformWindowPropertyOpacity:
			assert(p_type == kMCPlatformPropertyTypeFloat);
			m_opacity = *(float *)p_value;
			m_changes . opacity_changed = true;
			break;
		case kMCPlatformWindowPropertyMask:
			assert(p_type == kMCPlatformPropertyTypeWindowMask);
			if (m_mask != nil)
				MCPlatformWindowMaskRelease(m_mask);
			m_mask = *(MCPlatformWindowMaskRef *)p_value;
			if (m_mask != nil)
				MCPlatformWindowMaskRetain(m_mask);
			m_changes . mask_changed = true;
			break;
		case kMCPlatformWindowPropertyIsOpaque:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_is_opaque = *(bool *)p_value;
			m_changes . is_opaque_changed = true;
			break;
		case kMCPlatformWindowPropertyContentRect:
			assert(p_type == kMCPlatformPropertyTypeRectangle);
			m_content = *(MCRectangle *)p_value;
			m_changes . content_changed = true;
			break;
        // MW-2014-06-11: [[ Bug 12593 ]] No need to be platform-specific as uses
        //   virtual method to compute.
		case kMCPlatformWindowPropertyFrameRect:
			assert(p_type == kMCPlatformPropertyTypeRectangle);
			DoMapFrameRectToContentRect(*(MCRectangle *)p_value, m_content);
			m_changes . content_changed = true;
			break;
		case kMCPlatformWindowPropertyHasTitleWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_has_title_widget = *(bool *)p_value;
			m_changes . has_title_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasCloseWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_has_close_widget = *(bool *)p_value;
			m_changes . has_close_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasCollapseWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_has_collapse_widget = *(bool *)p_value;
			m_changes . has_collapse_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasZoomWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_has_zoom_widget = *(bool *)p_value;
			m_changes . has_zoom_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasSizeWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_has_size_widget = *(bool *)p_value;
			m_changes . has_size_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasShadow:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_has_shadow = *(bool *)p_value;
			m_changes . has_shadow_changed = true;
			break;
		case kMCPlatformWindowPropertyHasModifiedMark:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_has_modified_mark = *(bool *)p_value;
			m_changes . has_modified_mark_changed = true;
			break;
		case kMCPlatformWindowPropertyUseLiveResizing:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_use_live_resizing = *(bool *)p_value;
			m_changes . use_live_resizing_changed = true;
			break;
		case kMCPlatformWindowPropertyCursor:
			if (m_cursor != nil)
				MCPlatformReleaseCursor(m_cursor);
			m_cursor = *(MCPlatformCursorRef *)p_value;
			if (m_cursor != nil)
				MCPlatformRetainCursor(m_cursor);
            
            // MW-2014-04-08: [[ Bug 12073 ]] Mark the cursor as changed.
            m_changes . cursor_changed = true;
			break;
		case kMCPlatformWindowPropertyHideOnSuspend:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_hides_on_suspend = *(bool *)p_value;
			m_changes . hides_on_suspend_changed = true;
			break;
        // MERG-2014-06-02: [[ IgnoreMouseEvents ]] Handle ignoreMouseEvents.
        case kMCPlatformWindowPropertyIgnoreMouseEvents:
			assert(p_type == kMCPlatformPropertyTypeBool);
			m_ignore_mouse_events = *(bool *)p_value;
			m_changes . ignore_mouse_events_changed = true;
			break;
        // MERG-2015-10-11: [[ DocumentFilename ]] Handle document filename
        case kMCPlatformWindowPropertyDocumentFilename:
            assert(p_type == kMCPlatformPropertyTypeMCString);
            MCValueAssign(m_document_filename, *(MCStringRef*)p_value);
            m_changes . document_filename_changed = true;
            break;
		default:
			assert(false);
			break;
	}
	
	DoSynchronize();
	MCMemoryClear(&m_changes, sizeof(m_changes));
}

void MCPlatformWindow::GetProperty(MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	if (DoGetProperty(p_property, p_type, r_value))
		return;
	
	switch(p_property)
	{
		case kMCPlatformWindowPropertyTitle:
			assert(p_type == kMCPlatformPropertyTypeMCString);
            // SN-2014-06-23: Title updated to StringRef
            *(MCStringRef*)r_value = MCValueRetain(m_title);
			break;
		case kMCPlatformWindowPropertyStyle:
			assert(p_type == kMCPlatformPropertyTypeWindowStyle);
			*(MCPlatformWindowStyle *)r_value = m_style;
			break;
		case kMCPlatformWindowPropertyOpacity:
			assert(p_type == kMCPlatformPropertyTypeFloat);
			break;
		case kMCPlatformWindowPropertyMask:
			assert(p_type == kMCPlatformPropertyTypeWindowMask);
			break;
		case kMCPlatformWindowPropertyIsOpaque:
			assert(p_type == kMCPlatformPropertyTypeBool);
			*(bool *)r_value = m_is_opaque;
			break;
		case kMCPlatformWindowPropertyFrameRect:
			assert(p_type == kMCPlatformPropertyTypeRectangle);
            DoMapContentRectToFrameRect(m_content, *(MCRectangle *)r_value);
			break;
		case kMCPlatformWindowPropertyContentRect:
			assert(p_type == kMCPlatformPropertyTypeRectangle);
			*(MCRectangle *)r_value = m_content;
			break;
		case kMCPlatformWindowPropertyHasTitleWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasCloseWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasCollapseWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasZoomWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasSizeWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
            *(bool *)r_value = m_has_size_widget;
			break;
		case kMCPlatformWindowPropertyHasShadow:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasModifiedMark:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyUseLiveResizing:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyCursor:
			*(MCPlatformCursorRef *)r_value = m_cursor;
			break;
        // MERG-2015-10-11: [[ DocumentFilename ]] Handle document filename
        case kMCPlatformWindowPropertyDocumentFilename:
            assert(p_type == kMCPlatformPropertyTypeMCString);
            *(MCStringRef*)r_value = MCValueRetain(m_document_filename);
            break;
		default:
			assert(false);
			break;
	}
}

//////////

void MCPlatformWindow::MapPointFromScreenToWindow(MCPoint p_screen_point, MCPoint& r_window_point)
{
	r_window_point . x = p_screen_point . x - m_content . x;
	r_window_point . y = p_screen_point . y - m_content . y;
}

void MCPlatformWindow::MapPointFromWindowToScreen(MCPoint p_window_point, MCPoint& r_screen_point)
{
	r_screen_point . x = p_window_point . x + m_content . x;
	r_screen_point . y = p_window_point . y + m_content . y;
}

//////////

void MCPlatformWindow::AttachObject(void *p_object, MCPlatformWindowAttachmentCallback p_callback)
{
	/* UNCHECKED */ MCMemoryResizeArray(m_attachment_count + 1, m_attachments, m_attachment_count);
	m_attachments[m_attachment_count - 1] . object = p_object;
	m_attachments[m_attachment_count - 1] . callback = p_callback;
	
	if (m_is_visible)
		p_callback(p_object, true);
}

void MCPlatformWindow::DetachObject(void *p_object)
{
	for(uindex_t i = 0; i < m_attachment_count; i++)
		if (m_attachments[i] . object == p_object)
		{
			if (m_is_realized)
				m_attachments[i] . callback(m_attachments[i] . object, false);
			MCMemoryMove(m_attachments + i, m_attachments + i + 1, (m_attachment_count - i - 1) * sizeof(m_attachments[0]));
			m_attachment_count -= 1;
			return;
		}
}

void MCPlatformWindow::RealizeAndNotify(void)
{
    m_is_realized = true;
	
    DoRealize();
	
	for(uindex_t i = 0; i < m_attachment_count; i++)
		m_attachments[i] . callback(m_attachments[i] . object, true);
}

//////////

void MCPlatformWindow::HandleCloseRequest(void)
{
	MCPlatformCallbackSendWindowCloseRequest(this);
}

void MCPlatformWindow::HandleRedraw(MCPlatformSurfaceRef p_surface, MCGRegionRef p_region)
{
	MCPlatformCallbackSendWindowRedraw(this, p_surface, p_region);
}

void MCPlatformWindow::HandleReshape(MCRectangle p_new_content)
{
	// Update the content rect field and mark it as unchanged.
	m_content = p_new_content;
	m_changes . content_changed = false;
	
	// Dispatch the callback.
	MCPlatformCallbackSendWindowReshape(this, m_content);
}

void MCPlatformWindow::HandleIconify(void)
{
	// Update the iconified state.
	m_is_iconified = true;
	
	// Dispatch the callback.
	MCPlatformCallbackSendWindowIconify(this);
}

void MCPlatformWindow::HandleUniconify(void)
{
	// Update the iconified state.
	m_is_iconified = false;
	
	// Dispatch the callback.
	MCPlatformCallbackSendWindowUniconify(this);
}

void MCPlatformWindow::HandleFocus(void)
{
	// Update the focused state.
	m_is_focused = true;
	
	// Dispatch the callback.
	MCPlatformCallbackSendWindowFocus(this);
}

void MCPlatformWindow::HandleUnfocus(void)
{
	// Update the focused state.
	m_is_focused = false;
	
	// Dispatch the callback.
	MCPlatformCallbackSendWindowUnfocus(this);
}

void MCPlatformWindow::HandleKeyDown(MCPlatformKeyCode p_key_code, codepoint_t p_mapped_char, codepoint_t p_unmapped_char)
{
	MCPlatformCallbackSendKeyDown(this, p_key_code, p_mapped_char, p_unmapped_char);
}

void MCPlatformWindow::HandleKeyUp(MCPlatformKeyCode p_key_code, codepoint_t p_mapped_char, codepoint_t p_unmapped_char)
{
	MCPlatformCallbackSendKeyUp(this, p_key_code, p_mapped_char, p_unmapped_char);
}

void MCPlatformWindow::HandleDragEnter(MCRawClipboard* p_dragboard, MCPlatformDragOperation& r_operation)
{
	MCPlatformCallbackSendDragEnter(this, p_dragboard, r_operation);
}

void MCPlatformWindow::HandleDragMove(MCPoint p_location, MCPlatformDragOperation& r_operation)
{
	MCPlatformCallbackSendDragMove(this, p_location, r_operation);
}

void MCPlatformWindow::HandleDragLeave(void)
{
	MCPlatformCallbackSendDragLeave(this);
}

void MCPlatformWindow::HandleDragDrop(bool& r_accepted)
{
	MCPlatformCallbackSendDragDrop(this, r_accepted);
}

////////////////////////////////////////////////////////////////////////////////
//
//  Platform Window Procedural Wrappers
//

extern void MCMacPlatformCreateWindow(MCPlatformWindowRef& r_window);

void MCPlatformCreateWindow(MCPlatformWindowRef& r_window)
{
	MCMacPlatformCreateWindow(r_window);
}

void MCPlatformRetainWindow(MCPlatformWindowRef p_window)
{
	p_window -> Retain();
}

void MCPlatformReleaseWindow(MCPlatformWindowRef p_window)
{
	p_window -> Release();
}

void MCPlatformUpdateWindow(MCPlatformWindowRef p_window)
{
	p_window -> Update();
}

void MCPlatformInvalidateWindow(MCPlatformWindowRef p_window, MCRegionRef p_region)
{
	p_window -> Invalidate(p_region);
}

void MCPlatformShowWindow(MCPlatformWindowRef p_window)
{
	p_window -> Show();
}

void MCPlatformShowWindowAsSheet(MCPlatformWindowRef p_window, MCPlatformWindowRef p_parent_window)
{
	p_window -> ShowAsSheet(p_parent_window);
}

void MCPlatformHideWindow(MCPlatformWindowRef p_window)
{
	p_window -> Hide();
}

void MCPlatformFocusWindow(MCPlatformWindowRef p_window)
{
	p_window -> Focus();
}

void MCPlatformRaiseWindow(MCPlatformWindowRef p_window)
{
	p_window -> Raise();
}

void MCPlatformIconifyWindow(MCPlatformWindowRef p_window)
{
	p_window -> Iconify();
}

void MCPlatformUniconifyWindow(MCPlatformWindowRef p_window)
{
	p_window -> Uniconify();
}

void MCPlatformConfigureTextInputInWindow(MCPlatformWindowRef p_window, bool p_activate)
{
	p_window -> ConfigureTextInput(p_activate);
}

void MCPlatformResetTextInputInWindow(MCPlatformWindowRef p_window)
{
	p_window -> ResetTextInput();
}

bool MCPlatformIsWindowVisible(MCPlatformWindowRef p_window)
{
	return p_window -> IsVisible();
}

//////////

void MCPlatformMapPointFromScreenToWindow(MCPlatformWindowRef p_window, MCPoint p_screen_point, MCPoint& r_window_point)
{
	p_window -> MapPointFromScreenToWindow(p_screen_point, r_window_point);
}

void MCPlatformMapPointFromWindowToScreen(MCPlatformWindowRef p_window, MCPoint p_window_point, MCPoint& r_screen_point)
{
	p_window -> MapPointFromWindowToScreen(p_window_point, r_screen_point);
}

//////////

void MCPlatformSetWindowProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, const void *p_value)
{
	p_window -> SetProperty(p_property, p_type, p_value);
}

void MCPlatformGetWindowProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	p_window -> GetProperty(p_property, p_type, r_value);
}

void MCPlatformSetWindowContentRect(MCPlatformWindowRef p_window, MCRectangle p_content_rect)
{
	p_window -> SetProperty(kMCPlatformWindowPropertyContentRect, kMCPlatformPropertyTypeRectangle, &p_content_rect);
}

void MCPlatformGetWindowContentRect(MCPlatformWindowRef p_window, MCRectangle& r_content_rect)
{
	p_window -> GetProperty(kMCPlatformWindowPropertyContentRect, kMCPlatformPropertyTypeRectangle, &r_content_rect);
}

void MCPlatformSetWindowFrameRect(MCPlatformWindowRef p_window, MCRectangle p_frame_rect)
{
	p_window -> SetProperty(kMCPlatformWindowPropertyFrameRect, kMCPlatformPropertyTypeRectangle, &p_frame_rect);
}

void MCPlatformGetWindowFrameRect(MCPlatformWindowRef p_window, MCRectangle& r_frame_rect)
{
	p_window -> GetProperty(kMCPlatformWindowPropertyFrameRect, kMCPlatformPropertyTypeRectangle, &r_frame_rect);
}

void MCPlatformSetWindowBoolProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, bool p_value)
{
	p_window -> SetProperty(p_property, kMCPlatformPropertyTypeBool, &p_value);
}

void MCPlatformSetWindowFloatProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, float p_value)
{
	p_window -> SetProperty(p_property, kMCPlatformPropertyTypeFloat, &p_value);
}

////////////////////////////////////////////////////////////////////////////////
