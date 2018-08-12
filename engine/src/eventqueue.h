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

#ifndef __MC_EVENT_QUEUE__
#define __MC_EVENT_QUEUE__

#if defined(__cplusplus)
extern "C" {
#endif

enum MCMousePressState
{
	kMCMousePressStateUp,
	kMCMousePressStateDown,
	kMCMousePressStateRelease
};

bool MCEventQueueInitialize(void);
void MCEventQueueFinalize(void);

bool MCEventQueueDispatch(void);

void MCEventQueueFlush(MCStack *p_stack);

bool MCEventQueueGetMouseClick(uint32_t p_button);

typedef void (*MCEventQueueNotifyCallback)(void *state, bool dispatch);
bool MCEventQueuePostNotify(MCEventQueueNotifyCallback callback, void *state);

// IM-2014-02-14: [[ HiDPI ]] Post backing scale changes with window reshape message
MC_DLLEXPORT bool MCEventQueuePostWindowReshape(MCStack *stack, MCGFloat backing_scale);

MC_DLLEXPORT bool MCEventQueuePostMouseFocus(MCStack *stack, uint32_t time, bool inside);
MC_DLLEXPORT bool MCEventQueuePostMousePress(MCStack *stack, uint32_t time, uint32_t modifiers, MCMousePressState state, int32_t button);
bool MCEventQueuePostMouseWheel(MCStack *stack, uint32_t time, uint32_t modifiers, int32_t dh, int32_t dv);
MC_DLLEXPORT bool MCEventQueuePostMousePosition(MCStack *stack, uint32_t time, uint32_t modifiers, int32_t x, int32_t y);

MC_DLLEXPORT bool MCEventQueuePostKeyFocus(MCStack *stack, bool owner);
MC_DLLEXPORT bool MCEventQueuePostKeyPress(MCStack *stack, uint32_t modifiers, uint32_t char_code, uint32_t key_code);

MC_DLLEXPORT bool MCEventQueuePostImeCompose(MCStack *stack, bool enabled, uint32_t offset, const uint16_t *chars, uint32_t char_count);

enum MCEventTouchPhase
{
	kMCEventTouchPhaseBegan,
	kMCEventTouchPhaseMoved,
	kMCEventTouchPhaseEnded,
	kMCEventTouchPhaseCancelled
};

bool MCEventQueuePostTouch(MCStack *stack, MCEventTouchPhase phase, uint32_t ident, uint32_t taps, int32_t x, int32_t y);

enum MCEventMotionType
{
	kMCEventMotionShakeBegan,
	kMCEventMotionShakeCancelled,
	kMCEventMotionShakeEnded
};

bool MCEventQueuePostMotion(MCStack *stack, MCEventMotionType type, uint32_t timestamp);

bool MCEventQueuePostAccelerationChanged(double x, double y, double z, double timestamp);

bool MCEventQueuePostOrientationChanged(void);

bool MCEventQueuePostLocationChanged(void);
bool MCEventQueuePostLocationError(void);

bool MCEventQueuePostHeadingChanged(void);
bool MCEventQueuePostHeadingError(void);

void MCEventQueueClearTouches(void);

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)

class MCCustomEvent
{
public:
	virtual ~MCCustomEvent() {};
	virtual void Destroy(void) = 0;
	virtual void Dispatch(void) = 0;
};

bool MCEventQueuePostCustom(MCCustomEvent *event);
bool MCEventQueuePostCustomAtFront(MCCustomEvent *event);

#endif /* __cplusplus */


#endif
