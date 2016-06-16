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

#ifndef MBLEVENTS_H
#define MBLEVENTS_H

#include "eventqueue.h"

class MCMovieTouchedEvent: public MCCustomEvent
{
public:
	MCMovieTouchedEvent(MCObject *p_object) :
      m_object(p_object->GetHandle())
	{
	}
    
    virtual ~MCMovieTouchedEvent()
    {
    }
	
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		if (m_object.IsValid())
			m_object->message(MCM_movie_touched);
	}
	
private:
	MCObjectHandle m_object;
};

#endif // MBLEVENTS_H
