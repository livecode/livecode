/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#ifndef __MC_PLATFORM_LEGACY__
#define __MC_PLATFORM_LEGACY__

#ifndef __MC_PLATFORM__
#include "platform.h"
#endif

#ifndef __MC_GLOBALS__
#include "globals.h"
#endif

////////////////////////////////////////////////////////////////////////////////


inline bool MCPlatformLoadFont(MCStringRef p_utf8_path, bool p_globally, MCPlatformLoadedFontRef& r_loaded_font)
{
    MCPlatformLoadedFontRef t_font = MCplatform -> CreateLoadedFont();
    if (!t_font)
        return false;
    if (!t_font->CreateWithPath(p_utf8_path, p_globally))
    {
        t_font -> Release();
        return false;
    }
    r_loaded_font = t_font;
    return true;
}

inline bool MCPlatformUnloadFont(MCPlatformLoadedFontRef p_loaded_font)
{
    p_loaded_font->Release();
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformCreateColorTransform(const MCColorSpaceInfo& p_info, MCPlatformColorTransformRef& r_transform)
{
    MCPlatformColorTransformRef t_transform = MCplatform -> CreateColorTransform();
    if (t_transform &&
        t_transform->CreateWithColorSpace(p_info))
    {
        r_transform = t_transform;
    }
    else
    {
        t_transform -> Release();
        r_transform = nullptr;
    }
}

inline void MCPlatformCreateMenu(MCPlatformMenuRef& r_menu)
{
    r_menu = MCplatform -> CreateMenu();
}

inline void MCPlatformRetainMenu(MCPlatformMenuRef p_menu)
{
    p_menu -> Retain();
}

inline void MCPlatformReleaseMenu(MCPlatformMenuRef p_menu)
{
    p_menu -> Release();
}

inline void MCPlatformSetMenuTitle(MCPlatformMenuRef p_menu, MCStringRef p_title)
{
    p_menu -> SetTitle(p_title);
}

inline void MCPlatformCountMenuItems(MCPlatformMenuRef p_menu, uindex_t& r_count)
{
    r_count = p_menu -> CountItems();
}

inline void MCPlatformAddMenuItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
    p_menu -> AddItem(p_where);
}

inline void MCPlatformAddMenuSeparatorItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
    p_menu -> AddSeparatorItem(p_where);
}

inline void MCPlatformRemoveMenuItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
    p_menu -> RemoveItem(p_where);
}

inline void MCPlatformRemoveAllMenuItems(MCPlatformMenuRef p_menu)
{
    p_menu -> RemoveAllItems();
}

inline void MCPlatformGetMenuParent(MCPlatformMenuRef p_menu, MCPlatformMenuRef& r_parent, uindex_t& r_index)
{
    p_menu -> GetParent(r_parent, r_index);
}

inline void MCPlatformGetMenuItemProperty(MCPlatformMenuRef p_menu, uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    p_menu -> GetItemProperty(p_index, p_property, p_type, r_value);
}

inline void MCPlatformSetMenuItemProperty(MCPlatformMenuRef p_menu, uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, const void *p_value)
{
    p_menu -> SetItemProperty(p_index, p_property, p_type, p_value);
}

inline bool MCPlatformPopUpMenu(MCPlatformMenuRef p_menu, MCPlatformWindowRef p_window, MCPoint p_location, uindex_t p_item)
{
    return p_menu -> PopUp(p_window, p_location, p_item);
}

inline void MCPlatformShowMenubar(void)
{
    MCplatform -> ShowMenubar();
}

inline void MCPlatformHideMenubar(void)
{
    MCplatform -> HideMenubar();
}

inline void MCPlatformSetMenubar(MCPlatformMenuRef p_menu)
{
    MCplatform -> SetMenubar(p_menu);
}

inline void MCPlatformGetMenubar(MCPlatformMenuRef& r_menu)
{
    r_menu = MCplatform -> GetMenubar();
}

inline void MCPlatformSetIconMenu(MCPlatformMenuRef p_menu)
{
    MCplatform -> SetIconMenu(p_menu);
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformCreateStandardCursor(MCPlatformStandardCursor p_standard_cursor, MCPlatformCursorRef& r_cursor)
{
    MCPlatformCursorRef t_cursor = MCplatform -> CreateCursor();
    t_cursor -> CreateStandard(p_standard_cursor);
    r_cursor = t_cursor;
}

inline void MCPlatformCreateCustomCursor(MCImageBitmap *p_image, MCPoint p_hotspot, MCPlatformCursorRef& r_cursor)
{
    MCPlatformCursorRef t_cursor = MCplatform -> CreateCursor();
    t_cursor -> CreateCustom(p_image, p_hotspot);
    r_cursor = t_cursor;
}

inline void MCPlatformRetainCursor(MCPlatformCursorRef p_cursor)
{
    p_cursor -> Retain();
}

inline void MCPlatformReleaseCursor(MCPlatformCursorRef p_cursor)
{
    p_cursor -> Release();
}

inline void MCPlatformSetCursor(MCPlatformCursorRef p_cursor)
{
    p_cursor -> Set();
}

inline void MCPlatformHideCursorUntilMouseMoves(void)
{
    MCplatform -> HideCursorUntilMouseMoves();
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformDoDragDrop(MCPlatformWindowRef p_window, MCPlatformAllowedDragOperations p_allowed_operations, MCImageBitmap *p_image, const MCPoint *p_image_loc, MCPlatformDragOperation& r_operation)
{
    MCplatform -> DoDragDrop(p_window, p_allowed_operations, p_image, p_image_loc, r_operation);
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformCreateWindow(MCPlatformWindowRef& r_window)
{
    r_window = MCplatform -> CreateWindow();
}

inline void MCPlatformRetainWindow(MCPlatformWindowRef p_window)
{
    p_window -> Retain();
}

inline void MCPlatformReleaseWindow(MCPlatformWindowRef p_window)
{
    p_window -> Release();
}

inline void MCPlatformUpdateWindow(MCPlatformWindowRef p_window)
{
    p_window -> Update();
}

inline void MCPlatformInvalidateWindow(MCPlatformWindowRef p_window, MCGRegionRef p_region)
{
    p_window -> Invalidate(p_region);
}

inline void MCPlatformShowWindow(MCPlatformWindowRef p_window)
{
    p_window -> Show();
}

inline void MCPlatformShowWindowAsSheet(MCPlatformWindowRef p_window, MCPlatformWindowRef p_parent_window)
{
    p_window -> ShowAsSheet(p_parent_window);
}

inline void MCPlatformHideWindow(MCPlatformWindowRef p_window)
{
    p_window -> Hide();
}

inline void MCPlatformFocusWindow(MCPlatformWindowRef p_window)
{
    p_window -> Focus();
}

inline void MCPlatformRaiseWindow(MCPlatformWindowRef p_window)
{
    p_window -> Raise();
}

inline void MCPlatformIconifyWindow(MCPlatformWindowRef p_window)
{
    p_window -> Iconify();
}

inline void MCPlatformUniconifyWindow(MCPlatformWindowRef p_window)
{
    p_window -> Uniconify();
}

inline void MCPlatformConfigureTextInputInWindow(MCPlatformWindowRef p_window, bool p_activate)
{
    p_window -> ConfigureTextInput(p_activate);
}

inline void MCPlatformResetTextInputInWindow(MCPlatformWindowRef p_window)
{
    p_window -> ResetTextInput();
}

inline bool MCPlatformIsWindowVisible(MCPlatformWindowRef p_window)
{
    return p_window -> IsVisible();
}

//////////

inline void MCPlatformMapPointFromScreenToWindow(MCPlatformWindowRef p_window, MCPoint p_screen_point, MCPoint& r_window_point)
{
    p_window -> MapPointFromScreenToWindow(p_screen_point, r_window_point);
}

inline void MCPlatformMapPointFromWindowToScreen(MCPlatformWindowRef p_window, MCPoint p_window_point, MCPoint& r_screen_point)
{
    p_window -> MapPointFromWindowToScreen(p_window_point, r_screen_point);
}

//////////

inline void MCPlatformSetWindowProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, const void *p_value)
{
    p_window -> SetProperty(p_property, p_type, p_value);
}

inline void MCPlatformGetWindowProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    p_window -> GetProperty(p_property, p_type, r_value);
}

inline void MCPlatformSetWindowContentRect(MCPlatformWindowRef p_window, MCRectangle p_content_rect)
{
    p_window -> SetProperty(kMCPlatformWindowPropertyContentRect, kMCPlatformPropertyTypeRectangle, &p_content_rect);
}

inline void MCPlatformGetWindowContentRect(MCPlatformWindowRef p_window, MCRectangle& r_content_rect)
{
    p_window -> GetProperty(kMCPlatformWindowPropertyContentRect, kMCPlatformPropertyTypeRectangle, &r_content_rect);
}

inline void MCPlatformSetWindowFrameRect(MCPlatformWindowRef p_window, MCRectangle p_frame_rect)
{
    p_window -> SetProperty(kMCPlatformWindowPropertyFrameRect, kMCPlatformPropertyTypeRectangle, &p_frame_rect);
}

inline void MCPlatformGetWindowFrameRect(MCPlatformWindowRef p_window, MCRectangle& r_frame_rect)
{
    p_window -> GetProperty(kMCPlatformWindowPropertyFrameRect, kMCPlatformPropertyTypeRectangle, &r_frame_rect);
}

inline void MCPlatformSetWindowBoolProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, bool p_value)
{
    p_window -> SetProperty(p_property, kMCPlatformPropertyTypeBool, &p_value);
}

inline void MCPlatformSetWindowFloatProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, float p_value)
{
    p_window -> SetProperty(p_property, kMCPlatformPropertyTypeFloat, &p_value);
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformSwitchFocusToView(MCPlatformWindowRef p_window, uint32_t p_id)
{
    p_window -> SwitchFocusToView(p_id);
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformWindowDeathGrip(MCPlatformWindowRef p_window)
{
    MCplatform -> DeathGrip(p_window);
}

inline bool MCPlatformGetWindowWithId(uint32_t p_id, MCPlatformWindowRef& r_window)
{
    return MCplatform -> GetWindowWithId(p_id, r_window);
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformWindowMaskCreateWithAlphaAndRelease(int32_t width, int32_t height, int32_t stride, void *bits, MCPlatformWindowMaskRef& r_mask)
{
    MCPlatformWindowMaskRef t_mask = MCplatform -> CreateWindowMask();
    if (t_mask && !t_mask->CreateWithAlphaAndRelease(width, height, stride, bits))
    {
        t_mask -> Release();
        t_mask = nil;
    }
    r_mask = t_mask;
}

inline void MCPlatformWindowMaskRetain(MCPlatformWindowMaskRef p_mask)
{
    p_mask->Retain();
}

inline void MCPlatformWindowMaskRelease(MCPlatformWindowMaskRef p_mask)
{
    p_mask->Release();
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformConfigureBackdrop(MCPlatformWindowRef p_backdrop_window)
{
    MCplatform -> ConfigureBackdrop(p_backdrop_window);
}

////////////////////////////////////////////////////////////////////////////////

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use new platform surface API.
inline bool MCPlatformSurfaceLockGraphics(MCPlatformSurfaceRef p_surface, MCGIntegerRectangle p_region, MCGContextRef& r_context, MCGRaster& r_raster)
{
    return p_surface -> LockGraphics(p_region, r_context, r_raster);
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use new platform surface API.
inline void MCPlatformSurfaceUnlockGraphics(MCPlatformSurfaceRef p_surface, MCGIntegerRectangle p_region, MCGContextRef p_context, MCGRaster& p_raster)
{
    p_surface -> UnlockGraphics(p_region, p_context, p_raster);
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use new platform surface API.
inline bool MCPlatformSurfaceLockPixels(MCPlatformSurfaceRef p_surface, MCGIntegerRectangle p_region, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area)
{
    return p_surface -> LockPixels(p_region, r_raster, r_locked_area);
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use new platform surface API.
inline void MCPlatformSurfaceUnlockPixels(MCPlatformSurfaceRef p_surface, MCGIntegerRectangle p_region, MCGRaster& p_raster)
{
    p_surface -> UnlockPixels(p_region, p_raster);
}

inline bool MCPlatformSurfaceLockSystemContext(MCPlatformSurfaceRef p_surface, void*& r_context)
{
    // MW-2014-04-18: [[ Bug 12230 ]] Make sure we return the result.
    return p_surface -> LockSystemContext(r_context);
}

inline void MCPlatformSurfaceUnlockSystemContext(MCPlatformSurfaceRef p_surface)
{
    p_surface -> UnlockSystemContext();
}

inline bool MCPlatformSurfaceComposite(MCPlatformSurfaceRef p_surface, MCGRectangle p_dst_rect, MCGImageRef p_src_image, MCGRectangle p_src_rect, MCGFloat p_opacity, MCGBlendMode p_blend_mode)
{
    return p_surface -> Composite(p_dst_rect, p_src_image, p_src_rect, p_opacity, p_blend_mode);
}

inline MCGFloat MCPlatformSurfaceGetBackingScaleFactor(MCPlatformSurfaceRef p_surface)
{
    return p_surface -> GetBackingScaleFactor();
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformBeginPageSetupDialog(MCPlatformWindowRef p_owner, void *p_session, void *p_settings, void *p_page_format, MCPlatformPrintDialogSessionRef &r_dialog_session)
{
    MCPlatformPrintDialogSessionRef t_dialog_session = MCplatform -> CreatePrintDialogSession();
    t_dialog_session -> BeginPageSetup(p_owner, p_session, p_settings, p_page_format);
    
    r_dialog_session = t_dialog_session;
}

inline void MCPlatformBeginPrintSettingsDialog(MCPlatformWindowRef p_owner, void *p_session, void *p_settings, void *p_page_format, MCPlatformPrintDialogSessionRef &r_dialog_session)
{
    MCPlatformPrintDialogSessionRef t_dialog_session = MCplatform -> CreatePrintDialogSession();
    t_dialog_session -> BeginSettings(p_owner, p_session, p_settings, p_page_format);
    
    r_dialog_session = t_dialog_session;
}

inline MCPlatformPrintDialogResult MCPlatformPrintDialogSessionResult(MCPlatformPrintDialogSessionRef p_dialog_session)
{
    return p_dialog_session -> GetResult();
}

inline void MCPlatformPrintDialogSessionCopyInfo(MCPlatformPrintDialogSessionRef p_dialog_session, void *&x_session, void *&x_settings, void *&x_page_format)
{
    p_dialog_session -> CopyInfo(x_session, x_settings, x_page_format);
}

inline void MCPlatformPrintDialogSessionRelease(MCPlatformPrintDialogSessionRef p_dialog_session)
{
    p_dialog_session -> Release();
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformBeginFileDialog(MCPlatformFileDialogKind p_kind, MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file)
{
    MCplatform -> BeginFileDialog(p_kind, p_owner, p_title, p_prompt, p_types, p_type_count, p_initial_folder, p_initial_file);
}

inline MCPlatformDialogResult MCPlatformEndFileDialog(MCPlatformFileDialogKind p_kind, MCStringRef &r_paths, MCStringRef &r_type)
{
    return MCplatform -> EndFileDialog(p_kind, r_paths, r_type);
}

inline void MCPlatformBeginFolderDialog(MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial)
{
    MCplatform -> BeginFolderDialog(p_owner, p_title, p_prompt, p_initial);
}

inline MCPlatformDialogResult MCPlatformEndFolderDialog(MCStringRef& r_selected_folder)
{
    return MCplatform -> EndFolderDialog(r_selected_folder);
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformBeginColorDialog(MCStringRef p_title, const MCColor& p_color)
{
    return MCplatform -> BeginColorDialog(p_title, p_color);
}

inline MCPlatformDialogResult MCPlatformEndColorDialog(MCColor& r_new_color)
{
    return MCplatform -> EndColorDialog(r_new_color);
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformPlayerRetain(MCPlatformPlayerRef player)
{
    player -> Retain();
}

inline void MCPlatformPlayerRelease(MCPlatformPlayerRef player)
{
    player -> Release();
}

inline void *MCPlatformPlayerGetNativeView(MCPlatformPlayerRef player)
{
    void *t_view;
    if (!player -> GetNativeView(t_view))
        return nil;
    
    return t_view;
}

inline bool MCPlatformPlayerSetNativeParentView(MCPlatformPlayerRef p_player, void *p_parent_view)
{
    return p_player -> SetNativeParentView(p_parent_view);
}


inline bool MCPlatformPlayerIsPlaying(MCPlatformPlayerRef player)
{
    return player -> IsPlaying();
}

inline void MCPlatformStepPlayer(MCPlatformPlayerRef player, int amount)
{
    player -> Step(amount);
}

inline void MCPlatformStartPlayer(MCPlatformPlayerRef player, double rate)
{
    player -> Start(rate);
}

inline void MCPlatformStopPlayer(MCPlatformPlayerRef player)
{
    player -> Stop();
}

inline bool MCPlatformLockPlayerBitmap(MCPlatformPlayerRef player, const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap)
{
    return player -> LockBitmap(p_size, r_bitmap);
}

inline void MCPlatformUnlockPlayerBitmap(MCPlatformPlayerRef player, MCImageBitmap *bitmap)
{
    player -> UnlockBitmap(bitmap);
}

inline void MCPlatformSetPlayerProperty(MCPlatformPlayerRef player, MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value)
{
    player -> SetProperty(property, type, value);
}

inline void MCPlatformGetPlayerProperty(MCPlatformPlayerRef player, MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value)
{
    player -> GetProperty(property, type, value);
}

inline void MCPlatformCountPlayerTracks(MCPlatformPlayerRef player, uindex_t& r_track_count)
{
    player -> CountTracks(r_track_count);
}

inline void MCPlatformGetPlayerTrackProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value)
{
    player -> GetTrackProperty(index, property, type, value);
}

inline void MCPlatformSetPlayerTrackProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value)
{
    player -> SetTrackProperty(index, property, type, value);
}

inline bool MCPlatformFindPlayerTrackWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index)
{
    return player -> FindTrackWithId(id, r_index);
}

inline void MCPlatformCountPlayerNodes(MCPlatformPlayerRef player, uindex_t& r_node_count)
{
}

inline void MCPlatformGetPlayerNodeProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerNodeProperty property, MCPlatformPropertyType type, void *value)
{
}

inline void MCPlatformSetPlayerNodeProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerNodeProperty property, MCPlatformPropertyType type, void *value)
{
}

inline void MCPlatformFindPlayerNodeWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index)
{
}

inline void MCPlatformCountPlayerHotSpots(MCPlatformPlayerRef player, uindex_t& r_node_count)
{
}

inline void MCPlatformGetPlayerHotSpotProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerHotSpotProperty property, MCPlatformPropertyType type, void *value)
{
}

inline void MCPlatformSetPlayerHotSpotProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerHotSpotProperty property, MCPlatformPropertyType type, void *value)
{
}

inline void MCPlatformFindPlayerHotSpotWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index)
{
}

////////////////////////////////////////////////////////////////////////////////

#ifdef TARGET_PLATFORM_MACOS_X

inline void MCPlatformCreatePlayer(bool dontuseqt, MCPlatformPlayerRef& r_player)
{
    r_player = MCplatform -> CreatePlayer();
}

#endif

#ifdef TARGET_PLATFORM_WINDOWS
class MCWin32DSPlayer;
extern MCWin32DSPlayer *MCWin32DSPlayerCreate(void);
inline void MCPlatformCreatePlayer(bool dontuseqt, MCPlatformPlayerRef &r_player)
{
    r_player = (MCPlatformPlayerRef)MCWin32DSPlayerCreate();
}
#endif

////////////////////////////////////////////////////////////////////////////////

// SN-2014-07-23: [[ Bug 12907 ]]
//  Update as well MCSreenDC::createscriptenvironment (and callees)
inline void MCPlatformScriptEnvironmentCreate(MCStringRef language, MCPlatformScriptEnvironmentRef& r_env)
{
    r_env = MCplatform -> CreateScriptEnvironment();
}

inline void MCPlatformScriptEnvironmentRetain(MCPlatformScriptEnvironmentRef env)
{
    env -> Retain();
}

inline void MCPlatformScriptEnvironmentRelease(MCPlatformScriptEnvironmentRef env)
{
    env -> Release();
}

inline bool MCPlatformScriptEnvironmentDefine(MCPlatformScriptEnvironmentRef env, const char *function, MCPlatformScriptEnvironmentCallback callback)
{
    return env -> Define(function, callback);
}

inline void MCPlatformScriptEnvironmentRun(MCPlatformScriptEnvironmentRef env, MCStringRef script, MCStringRef& r_result)
{
    env -> Run(script, r_result);
}

inline void MCPlatformScriptEnvironmentCall(MCPlatformScriptEnvironmentRef env, const char *method, const char **arguments, uindex_t argument_count, char*& r_result)
{
    r_result = env -> Call(method, arguments, argument_count);
}


////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformSoundCreateWithData(const void *p_data, size_t p_data_size, MCPlatformSoundRef& r_sound)
{
    MCPlatformSoundRef t_sound = MCplatform -> CreateSound();
    if (t_sound &&
        !t_sound->CreateWithData(p_data, p_data_size))
    {
        t_sound -> Release();
        t_sound = nil;
    }
    r_sound = t_sound;
}

inline void MCPlatformSoundRetain(MCPlatformSoundRef p_sound)
{
    p_sound->Retain();
}

inline void MCPlatformSoundRelease(MCPlatformSoundRef p_sound)
{
    p_sound->Release();
}

inline bool MCPlatformSoundIsPlaying(MCPlatformSoundRef p_sound)
{
    return p_sound->IsPlaying();
}

inline void MCPlatformSoundPlay(MCPlatformSoundRef p_sound)
{
    p_sound->Play();
}

inline void MCPlatformSoundPause(MCPlatformSoundRef p_sound)
{
    p_sound->Pause();
}

inline void MCPlatformSoundResume(MCPlatformSoundRef p_sound)
{
    p_sound->Resume();
}

inline void MCPlatformSoundStop(MCPlatformSoundRef p_sound)
{
    p_sound->Stop();
}

inline void MCPlatformSoundSetProperty(MCPlatformSoundRef p_sound, MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value)
{
    p_sound->SetProperty(property, type, value);
}

inline void MCPlatformSoundGetProperty(MCPlatformSoundRef p_sound, MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value)
{
    p_sound->GetProperty(property, type, value);
}

////////////////////////////////////////////////////////////////////////////////

// Implementation guidence:
//
// This is basic abstraction of the sound recording functionality which the
// engine currently has.
//
// The platform sound recorder opaque type should be a procedural wrapper around
// a class which implements an abstract interface (just like the PlatformPlayer).
//
// Initially we need an implementation using the QT Sequence Grabber which needs
// to use the SGAudioMediaType (essentially this means that the code in the engine
// already for this is not usable - its heavily tied to SoundMediaType which
// crashes on modern Mac's).
//
// There is a sample code project 'WhackedTV' which should be useful to base the
// implementation on. This project goes a bit further than we need though - our
// dialog only needs to be the QT one provided by SCRequestImageSettings so that
// should hopefully simplify things.
//
// The SoundRecorder object should hold all the necessary system state to operate
// and whilst recording should ensure that things are idled appropriately using
// a Cocoa timer rather than requiring the engine to call an idle-proc (which is
// what is currently required).
//

inline void MCPlatformSoundRecorderCreate(MCPlatformSoundRecorderRef& r_recorder)
{
    //MCresult -> sets("could not initialize quicktime");
}

inline void MCPlatformSoundRecorderRetain(MCPlatformSoundRecorderRef p_recorder)
{
    p_recorder -> Retain();
}

inline void MCPlatformSoundRecorderRelease(MCPlatformSoundRecorderRef p_recorder)
{
    p_recorder -> Release();
}

// Return true if the recorder is recording.
inline bool MCPlatformSoundRecorderIsRecording(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> IsRecording();
}

// Return the current volume of the recorded input - if not recording, return 0.
inline double MCPlatformSoundRecorderGetLoudness(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> GetLoudness();
}

// Start sound recording to the given file. If the sound recorder is already recording then
// the existing recording should be cancelled (stop and delete output file).
inline bool MCPlatformSoundRecorderStart(MCPlatformSoundRecorderRef p_recorder, MCStringRef p_filename)
{
    return p_recorder -> StartRecording(p_filename);
}

// Stop the sound recording.
inline void MCPlatformSoundRecorderStop(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> StopRecording();
}

inline void MCPlatformSoundRecorderPause(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> PauseRecording();
}

inline void MCPlatformSoundRecorderResume(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> ResumeRecording();
}

// Call callback for each possible input device available - if the callback returns 'false' at any point
// enumeration is cancelled, and the false will be returned.
inline bool MCPlatformSoundRecorderListInputs(MCPlatformSoundRecorderRef p_recorder, MCPlatformSoundRecorderListInputsCallback callback, void *context)
{
    return p_recorder -> ListInputs(callback, context);
}

// Call callback for each possible compressor available - if the callback returns 'false' at any point
// enumeration is cancelled, and the false will be returned.
inline bool MCPlatformSoundRecorderListCompressors(MCPlatformSoundRecorderRef p_recorder, MCPlatformSoundRecorderListCompressorsCallback callback, void *context)
{
    return p_recorder -> ListCompressors(callback, context);
}

// Get the current sound recording configuration. The caller is responsible for freeing 'extra_info'.
inline void MCPlatformSoundRecorderGetConfiguration(MCPlatformSoundRecorderRef p_recorder, MCPlatformSoundRecorderConfiguration& r_config)
{
    p_recorder -> GetConfiguration(r_config);
}

// Set the current sound recording configuration.
inline void MCPlatformSoundRecorderSetConfiguration(MCPlatformSoundRecorderRef p_recorder, const MCPlatformSoundRecorderConfiguration& p_config)
{
    p_recorder -> SetConfiguration(p_config);
}

// Popup a configuration dialog for the compressors. If the dialog is not cancelled the
// sound recorders config will have been updated.
inline void MCPlatformSoundRecorderBeginConfigurationDialog(MCPlatformSoundRecorderRef p_recorder)
{
    p_recorder -> BeginDialog();
}

// End the configuration dialog.
inline MCPlatformDialogResult MCPlatformSoundRecorderEndConfigurationDialog(MCPlatformSoundRecorderRef p_recorder)
{
    return p_recorder -> EndDialog();
}

////////////////////////////////////////////////////////////////////////////////


#include "mac-extern.h"

inline bool MCPlatformInitialize(void)
{
    MCPlatform::CoreRef t_platform = MCMacPlatformCreateCore();
    MCplatform = t_platform.unsafeTake();
    MCPlatform::Ref<MCPlatformCallback> t_callback = MCPlatform::makeRef<MCPlatformCallback>();
    MCplatform -> SetCallback(t_callback.unsafeTake());
    
    return true;
}

inline void MCPlatformFinalize(void)
{
    MCplatform -> Release();
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformShowMessageDialog(MCStringRef p_title, MCStringRef p_message)
{
    MCplatform -> ShowMessageDialog(p_title, p_message);
}

////////////////////////////////////////////////////////////////////////////////

// System properties are settings that can be queried from the system.
//
// TODO-REVIEW: Perhaps these would be better classed as metrics?
//   Or perhaps MCPlatformQuery(...)

inline void MCPlatformGetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    MCplatform -> GetSystemProperty(p_property, p_type, r_value);
}

inline void MCPlatformSetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
    MCplatform -> SetSystemProperty(p_property, p_type, p_value);
}

////////////////////////////////////////////////////////////////////////////////

// Wait for any event for at most duration seconds. If blocking is true then
// no events which cause a dispatch should be processed. If an event is processed
// during duration, true is returned; otherwise false is.
inline bool MCPlatformWaitForEvent(double p_duration, bool p_blocking)
{
    return MCplatform -> WaitForEvent(p_duration, p_blocking);
}

// Break the current WaitForEvent which is progress.
inline void MCPlatformBreakWait(void)
{
    MCplatform -> BreakWait();
}

#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
// Apple platforms only
inline void MCPlatformRunBlockOnMainFiber(void (^block)(void))
{
    MCplatform -> RunBlockOnMainFiber(block);
}
#endif

////////////////////////////////////////////////////////////////////////////////

// Return true if the abort key has been pressed since the last check.
inline bool MCPlatformGetAbortKeyPressed(void)
{
    return MCplatform -> GetAbortKeyPressed();
}

// Get the current (right now!) state of the mouse button.
inline bool MCPlatformGetMouseButtonState(uindex_t button)
{
    return MCplatform -> GetMouseButtonState(button);
}

// Returns an array of all the currently pressed keys.
inline bool MCPlatformGetKeyState(MCPlatformKeyCode*& r_codes, uindex_t& r_code_count)
{
    return MCplatform -> GetKeyState(r_codes, r_code_count);
}

// Get the current (right now!) state of the modifier keys.
inline MCPlatformModifiers MCPlatformGetModifiersState(void)
{
    return MCplatform -> GetModifiersState();
}

// Peek into the event queue and pull out a mouse click event (down then up)
// for the given button. If button is 0, then any button click will do.
inline bool MCPlatformGetMouseClick(uindex_t button, MCPoint& r_location)
{
    return MCplatform -> GetMouseClick(button, r_location);
}

// Get the position of the mouse in global coords.
inline void MCPlatformGetMousePosition(MCPoint& r_location)
{
    MCplatform -> GetMousePosition(r_location);
}

// Set the position of the mouse in global coords.
inline void MCPlatformSetMousePosition(MCPoint location)
{
    MCplatform -> SetMousePosition(location);
}

// Get the window (that we know about) at the given co-ords.
inline void MCPlatformGetWindowAtPoint(MCPoint location, MCPlatformWindowRef& r_window)
{
    MCplatform -> GetWindowAtPoint(location, r_window);
}

// Make the given window grab the pointer (if possible).
inline void MCPlatformGrabPointer(MCPlatformWindowRef p_window)
{
    MCplatform -> GrabPointer(p_window);
}

// Release the pointer from a grab.
inline void MCPlatformUngrabPointer(void)
{
    MCplatform -> UngrabPointer();
}

////////////////////////////////////////////////////////////////////////////////

// Flush events of the specified types in mask.
inline void MCPlatformFlushEvents(MCPlatformEventMask p_mask)
{
    MCplatform -> FlushEvents(p_mask);
}

// Return the 'time' of the last event.
inline uint32_t MCPlatformGetEventTime(void)
{
    return MCplatform -> GetEventTime();
}

////////////////////////////////////////////////////////////////////////////////

// Produce a system beep.
inline void MCPlatformBeep(void)
{
    MCplatform -> Beep();
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformGetScreenCount(uindex_t& r_count)
{
    MCplatform -> GetScreenCount(r_count);
}

inline void MCPlatformGetScreenViewport(uindex_t p_index, MCRectangle& r_viewport)
{
    MCplatform -> GetScreenViewport(p_index, r_viewport);
}

inline void MCPlatformGetScreenWorkarea(uindex_t p_index, MCRectangle& r_workarea)
{
    MCplatform -> GetScreenWorkarea(p_index, r_workarea);
}

inline void MCPlatformGetScreenPixelScale(uindex_t p_index, MCGFloat& r_scale)
{
    MCplatform -> GetScreenPixelScale(p_index, r_scale);
}

inline void MCPlatformDisableScreenUpdates(void)
{
    MCplatform -> DisableScreenUpdates();
}

inline void MCPlatformEnableScreenUpdates(void)
{
    MCplatform -> EnableScreenUpdates();
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformScreenSnapshotOfUserArea(MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
    MCplatform -> ScreenSnapshotOfUserArea(p_size, r_bitmap);
}

inline void MCPlatformScreenSnapshot(MCRectangle p_area, MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
    MCplatform -> ScreenSnapshot(p_area, p_size, r_bitmap);
}

inline void MCPlatformScreenSnapshotOfWindow(uint32_t window_id, MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
    MCplatform -> ScreenSnapshotOfWindow(window_id, p_size, r_bitmap);
}

inline void MCPlatformScreenSnapshotOfWindowArea(uint32_t window_id, MCRectangle p_area, MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
    MCplatform -> ScreenSnapshotOfWindowArea(window_id, p_area, p_size, r_bitmap);
}

////////////////////////////////////////////////////////////////////////////////

inline bool MCPlatformGetControlThemePropBool(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, bool& r_bool)
{
    return MCplatform -> GetControlThemePropBool(p_type, p_part, p_state, p_which, r_bool);
}

inline bool MCPlatformGetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, int& r_int)
{
    return MCplatform -> GetControlThemePropInteger(p_type, p_part, p_state, p_which, r_int);
}

inline bool MCPlatformGetControlThemePropColor(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCColor& r_color)
{
    return MCplatform -> GetControlThemePropColor(p_type, p_part, p_state, p_which, r_color);
}

inline bool MCPlatformGetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCFontRef& r_font)
{
    return MCplatform -> GetControlThemePropFont(p_type, p_part, p_state, p_which, r_font);
}

inline bool MCPlatformGetControlThemePropString(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCStringRef& r_string)
{
    return MCplatform -> GetControlThemePropString(p_type, p_part, p_state, p_which, r_string);
}

////////////////////////////////////////////////////////////////////////////////

inline void MCPlatformRetainColorTransform(MCPlatformColorTransformRef p_transform)
{
    p_transform->Retain();
}

inline void MCPlatformReleaseColorTransform(MCPlatformColorTransformRef p_transform)
{
    p_transform->Release();
}

inline bool MCPlatformApplyColorTransform(MCPlatformColorTransformRef p_transform, MCImageBitmap *p_image)
{
    if (p_transform == nil)
        return false;
    
    return p_transform->Apply(p_image);
}

////////////////////////////////////////////////////////////////////////////////

#endif

