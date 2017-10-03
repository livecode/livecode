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

#ifndef __MC_REDRAW__
#define __MC_REDRAW__

void MCRedrawBeginDeviceSceneryLayer(MCObject* obj, MCGContextRef gcontext, const MCRectangle32& p_rect, MCContext*& r_user_context, MCRectangle& r_user_rect);
void MCRedrawEndDeviceSceneryLayer(MCContext* user_context);

void MCRedrawBeginDeviceSpriteLayer(MCObject* obj, MCGContextRef gcontext, const MCRectangle32& p_rect, MCContext*& r_user_context, MCRectangle& r_user_rect);
void MCRedrawEndDeviceSpriteLayer(MCContext* user_context);

template<typename Object, bool (Object::*Method)(MCContext *target, const MCRectangle& region)>
bool MCRedrawRenderDeviceSceneryLayer(void *p_context, MCGContextRef p_gcontext, const MCRectangle32& p_rectangle)
{
    bool t_success;
    Object *t_this = static_cast<Object *>(p_context);
    MCContext *t_user_context;
    MCRectangle t_user_rect;
    MCRedrawBeginDeviceSceneryLayer(t_this, p_gcontext, p_rectangle, t_user_context, t_user_rect);
    t_success = (t_this->*Method)(t_user_context, t_user_rect);
    MCRedrawEndDeviceSceneryLayer(t_user_context);
    return t_success;
}

template<typename Object, bool (Object::*Method)(MCContext *target, const MCRectangle& region)>
bool MCRedrawRenderDeviceSpriteLayer(void *p_context, MCGContextRef p_gcontext, const MCRectangle32& p_rectangle)
{
    bool t_success;
    Object *t_this = static_cast<Object *>(p_context);
    MCContext *t_user_context;
    MCRectangle t_user_rect;
    MCRedrawBeginDeviceSpriteLayer(t_this, p_gcontext, p_rectangle, t_user_context, t_user_rect);
    t_success = (t_this->*Method)(t_user_context, t_user_rect);
    MCRedrawEndDeviceSpriteLayer(t_user_context);
    return t_success;
}

/****/

bool MCRedrawIsScreenLocked(void);
void MCRedrawSaveLockScreen(uint2& r_lock);
void MCRedrawRestoreLockScreen(uint2 lock);

void MCRedrawLockScreen(void);
void MCRedrawUnlockScreen(void);
void MCRedrawUnlockScreenWithEffects(void);
void MCRedrawForceUnlockScreen(void);

bool MCRedrawIsScreenDirty(void);
void MCRedrawDirtyScreen(void);

void MCRedrawScheduleUpdateForStack(MCStack *stack);
void MCRedrawDoUpdateScreen(void);

bool MCRedrawIsScreenUpdateEnabled(void);
void MCRedrawDisableScreenUpdates(void);
void MCRedrawEnableScreenUpdates(void);

void MCRedrawDoUpdateScreen(void);

#endif
