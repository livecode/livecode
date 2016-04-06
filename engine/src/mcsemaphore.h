/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#ifndef __MC_SEMAPHORE_H__
#define __MC_SEMAPHORE_H__

/* This is a semaphore with an internal lock count.  It's a drop-in
 * replacement for a lock counter integer. */

class MCSemaphore
{
 public:

	/* Default constructor starts with 0 count */
	MCSemaphore(const char *p_desc = nil) : m_counter(0)
	{
#if defined(DEBUG_LOCKS)
		m_desc = p_desc;
#endif /* DEBUG_LOCKS */
	}

	/* We need a copy constructor to make postfix increment/decrement work
	 * correctly. */
	MCSemaphore(const MCSemaphore &p_copy) : m_counter(p_copy.m_counter)
	{
#if defined(DEBUG_LOCKS)
		m_desc = p_copy.m_desc;
#endif /* DEBUG_LOCKS */
	}

	~MCSemaphore() {}

	/* ---------- Reset */
	void Reset()
	{
		m_counter = 0;
	}

	/* ---------- Testing */
	bool IsLocked() const
	{
		return (m_counter > 0);
	}

	/* Allow use in "if" statements; returns true if locked */
	/* FIXME mark as explicit */
	operator bool() const
	{
		return IsLocked();
	}

	/* Automatic extraction of the wait depth */
	operator int() const
	{
		return m_counter;
	}

	/* ---------- Locking ops */
	void Lock()
	{
		MCAssert(m_counter < INT_MAX);
		++m_counter;

#if defined(DEBUG_LOCKS)
		MCLog("LOCK %p -> %i (%s)", this, m_counter,
		      m_desc ? m_desc : "unnamed");
#endif /* DEBUG_LOCKS */
	}

	MCSemaphore & /*prefix*/ operator++()
	{
		Lock();
		return *this;
	}

	MCSemaphore /*postfix*/ operator++(int)
	{
		MCSemaphore tmp(*this); /* copy */
		Lock();
		return tmp;             /* return old value */
	}

	/* ---------- Unlocking ops */
	void Unlock()
	{
		MCAssert(m_counter > 0);
		--m_counter;

#if defined(DEBUG_LOCKS)
		MCLog("UNLOCK %p -> %i (%s)", this, m_counter,
		      m_desc ? m_desc : "unnamed");
#endif /* DEBUG_LOCKS */
	}

	MCSemaphore & /*prefix*/ operator--()
	{
		Unlock();
		return *this;
	}

	MCSemaphore /*postfix*/ operator--(int)
	{
		MCSemaphore tmp(*this); /* copy */
		Unlock();
		return tmp;             /* return old value */
	}

 private:
	int m_counter;

#if defined(DEBUG_LOCKS)
	const char *m_desc;
#endif /* DEBUG_LOCKS */
};

#endif /* !__MC_SEMAPHORE_H__ */
