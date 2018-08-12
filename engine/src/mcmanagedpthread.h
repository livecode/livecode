/*                                                                     -*-c++-*-

Copyright (C) 2016 LiveCode Ltd.

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

#ifndef __MC_MANAGED_PTHREAD_H__
#define __MC_MANAGED_PTHREAD_H__

#include <pthread.h>

/* This class encapsulates a pthread_t value, and assists in tracking
 * its state, comparing it with other threads, and some other
 * stuff. Note that this structure is not itself thread-safe, and
 * should normally be protected with a mutex.  (You'll probably need a
 * mutex anyway). */
class MCManagedPThread
{
 public:
	MCManagedPThread() : m_live(false) {}
	MCManagedPThread(pthread_t p_thread)
		: m_live(true), m_thread(p_thread) {}

	operator bool() const { return m_live; }

	MCManagedPThread & operator=(pthread_t p_thread)
	{
		MCAssert(!m_live);
		m_thread = p_thread;
		m_live = true;
		return *this;
	}

	bool operator==(const MCManagedPThread & other) const
	{
		return (0 != pthread_equal(m_thread, other.m_thread));
	}
	bool operator==(const pthread_t & other) const
	{
		return (0 != pthread_equal(m_thread, other));
	}

	bool IsCurrent() const
	{
		return operator==(pthread_self());
	}

	int Create(const pthread_attr_t *attr,
	           void *(*start_routine) (void *),
	           void *arg)
	{
		if (m_live)
			return EDEADLK;

		int status = pthread_create(&m_thread, attr, start_routine, arg);
		m_live = (status == 0);
		return status;
	}

	int Join(void **retval)
	{
		if (!m_live)
			return EINVAL;

		int status = pthread_join(m_thread, retval);

		switch (status)
		{
		case EINVAL:
		case ESRCH:
		case 0:
			m_live = false;
			break;
		case EDEADLK:
			break;
		}

		return status;
	}

 private:
	bool m_live;
	pthread_t m_thread;
};

#endif /* !__MC_MANAGED_PTHREAD_H__ */
