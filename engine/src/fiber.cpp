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

#include "prefix.h"


#include "fiber.h"

#include "mcmanagedpthread.h"

////////////////////////////////////////////////////////////////////////////////

struct MCFiber
{
	MCFiber *next;
	MCManagedPThread thread;
	bool owns_thread;
	bool finished;
	uindex_t depth;
	
	MCFiberCallback callback;
	void *context;
	MCFiberRef caller;
};

////////////////////////////////////////////////////////////////////////////////

static MCFiberRef s_fibers = nil;
static MCFiberRef s_fiber_current = nil;
static pthread_mutex_t s_fiber_mutex;
static pthread_cond_t s_fiber_condition;

void MCFiberInitialize(void)
{
	s_fibers = nil;
	s_fiber_current = nil;
	pthread_mutex_init(&s_fiber_mutex, NULL);
	pthread_cond_init(&s_fiber_condition, NULL);
}

void MCFiberFinalize(void)
{
	pthread_cond_destroy(&s_fiber_condition);
	pthread_mutex_destroy(&s_fiber_mutex);
	s_fiber_current = nil;
	s_fibers = nil;
}

static void MCFiberSwitch(MCFiberRef p_target)
{
	// Lock the mutex and update the current fiber to the target.
	pthread_mutex_lock(&s_fiber_mutex);
	s_fiber_current = p_target;
	pthread_mutex_unlock(&s_fiber_mutex);
	
	// Signal the condition to wake up any of the fiber threads.
	// This is where the switch actually happens.
	pthread_cond_signal(&s_fiber_condition);
}

static void MCFiberDispatch(MCFiberRef p_current)
{
	// Increase the nesting depth of the fiber.
	p_current -> depth += 1;
	
	// Loop until there are no more callback requests.
	for(;;)
	{
		// Wait until we are made current.
		pthread_mutex_lock(&s_fiber_mutex);
		while(s_fiber_current != p_current)
			pthread_cond_wait(&s_fiber_condition, &s_fiber_mutex);
		pthread_mutex_unlock(&s_fiber_mutex);
		
		// If there are no callbacks to this fiber then we are done.
		if (p_current -> callback == nil)
			break;
		
		// We have a callback to invoke.
		MCFiberCallback t_callback;
		void *t_context;
		t_callback = p_current -> callback;
		t_context = p_current -> context;
		
		// Get the calling fiber.
		MCFiberRef t_target;
		t_target = p_current -> caller;
		
		// Clear the current fiber's callback fields as we are about to invoke it.
		p_current -> callback = nil;
		p_current -> context = nil;
		p_current -> caller = nil;
		
		// Invoke the callback.
		t_callback(t_context);
		
		// Switch to the calling fiber.
		MCFiberSwitch(t_target);
	}
	
	// Decrease the nesting depth of the fiber.
	p_current -> depth -= 1;
}

static void *MCFiberOwnedThreadRoutine(void *p_context)
{
	MCFiberRef self;
	self = (MCFiberRef)p_context;
	
	MCFiberDispatch(self);
	
	self -> finished = true;

	return nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCFiberConvert(MCFiberRef& r_fiber)
{
	MCFiberRef self;
	if (!MCMemoryCreate(self))
		return false;
	
	// If we have no fibers already, initialize ourselves.
	if (s_fibers == nil)
		MCFiberInitialize();
	
	// Get the thread id of the fiber and link it into the fiber chain.
	self -> thread = pthread_self();
	self -> next = s_fibers;
	s_fibers = self;
	
	// As we are converting the current thread, the current fiber becomes
	// self.
	s_fiber_current = self;
	
	// Return successfully.
	r_fiber = self;
	return true;
}

bool MCFiberCreate(size_t p_stack_size, MCFiberRef& r_fiber)
{
	MCFiberRef self;
	if (!MCMemoryCreate(self))
		return false;
	
	if (s_fibers == nil)
		MCFiberInitialize();

	self -> next = s_fibers;
	s_fibers = self;

	self->thread.Create(nil, MCFiberOwnedThreadRoutine, self);
	if (!self->thread)
	{
		MCFiberDestroy(self);
		return false;
	}
	
	r_fiber = self;
	
	return true;
}

void MCFiberDestroy(MCFiberRef self)
{
	// A fiber can only be destroyed at zero depth.
	MCAssert(self -> depth == 0);
	
	// If the thread is owned by the fiber then wait on it to finish.
	if (self -> owns_thread && self -> thread)
	{
		// A fiber that owns its thread cannot destroy itself.
		MCAssert(self != s_fiber_current);

		// Loop until the thread is finished.
		while(!self -> finished)
			MCFiberMakeCurrent(self);
		
		// Join to the thread.
		self->thread.Join(nil);
	}
	
	// Remove the fiber record from the list.
	if (s_fibers != self)
	{
		MCFiber *t_previous;
		for(t_previous = s_fibers; t_previous -> next != self; t_previous = t_previous -> next)
			;
		t_previous -> next = self -> next;
	}
	else
		s_fibers = self -> next;
	
	// Delete the record.
	MCMemoryDestroy(self);
	
	// If there are now no fibers, finalize our state.
	if (s_fibers == nil)
		MCFiberFinalize();
}

MCFiberRef MCFiberGetCurrent(void)
{
	return s_fiber_current;
}

void MCFiberMakeCurrent(MCFiberRef p_target)
{
	// If we are on the current fiber, then do nothing.
	if (p_target == s_fiber_current)
		return;
	
	// Get the fiber we are currently on.
	MCFiberRef t_current;
	t_current = s_fiber_current;
	
	// Jump to the new fiber.
	MCFiberSwitch(p_target);
	
	// Dispatch callbacks until no more are done, then we return to the caller.
	MCFiberDispatch(t_current);
}

void MCFiberCall(MCFiberRef p_target, MCFiberCallback p_callback, void *p_context)
{
	// If we are on the given fiber, call the callback directly.
	if (p_target == s_fiber_current)
	{
		p_callback(p_context);
		return;
	}
	
	// Otherwise, set the fields in the fiber's structure.
	p_target -> callback = p_callback;
	p_target -> context = p_context;
	p_target -> caller = s_fiber_current;
	
	// And switch to the fiber.
	MCFiberMakeCurrent(p_target);
}

bool MCFiberIsCurrentThread(MCFiberRef self)
{
	return self->thread.IsCurrent();
}

////////////////////////////////////////////////////////////////////////////////
