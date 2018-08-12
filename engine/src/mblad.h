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

#ifndef __MC_MOBILE_AD__
#define __MC_MOBILE_AD__

#include "mblsyntax.h"

enum MCAdEventType {
    kMCAdEventTypeReceive,
	kMCAdEventTypeReceiveDefault,
	kMCAdEventTypeReceiveFailed,
	kMCAdEventTypeClick,
    kMCAdEventTypeResizeStart,
    kMCAdEventTypeResizeEnd,
    kMCAdEventTypeExpandStart,
    kMCAdEventTypeExpandEnd
};

class MCAd
{
public:
	// Increment/decrement reference count. This count is used to control the
	// lifetime of the ad instance. This prevents an instance
	// being deleted while it is being referenced on the stack.
	void Retain(void);
	void Release(void);
	
	// Make sure the instance has a view and is active.
	virtual bool Create(void) = 0;
    
	// Delete the view associated with the control. This is usually called
	// in response to a 'delete control' call.
	virtual void Delete(void) = 0;
    
	// Get the native control id of the instance.
	uint32_t GetId(void);
	
	// Get the ad's name (if any)
	MCStringRef GetName();
	
	// Set the native ad's name
	bool SetName(MCStringRef name);
	
	// Get the owning object of the instance
	MCObjectHandle GetOwner(void);
	
	// Set the owning object of the instance
	void SetOwner(MCObjectHandle owner);
        
    // Return a pointer to the next ad
    MCAd *GetNext();
    
    // Set the next ad
    void SetNext(MCAd *p_next);
    
	// Look for an instance either by name or id
	static bool FindByNameOrId(MCStringRef name_or_id, MCAd *&r_ad);
    
	// Look for an instance with a given id
	static bool FindById(uint32_t p_id, MCAd *&r_ad);

    // Return first add in list
    static MCAd *GetFirst();
    
    // Clean up all ads
    static void Finalize(void);
    
    virtual bool GetVisible(void) = 0;
    virtual void SetVisible(bool p_visible) = 0;
    virtual MCAdTopLeft GetTopLeft() = 0;
    virtual void SetTopLeft(MCAdTopLeft p_top_left) = 0;
    
protected:
	// Constructor is not available to the outside.
	MCAd(void);
	// Destructor is not available to the outside.
	virtual ~MCAd(void);
    
private:
	// The chain of controls
	MCAd *m_next;
	// The reference count for the instance
	uint32_t m_references;
	// The id of the instance
	uint32_t m_id;
	// The name of the instance
	MCStringRef m_name;
	// The instance's owning object (handle)
	MCObjectHandle m_object;
};

void MCAdInitialize(void);
void MCAdFinalize(void);

MCStringRef MCAdGetInneractiveKey(void);
bool MCAdInneractiveKeyIsNil(void);
bool MCAdSetInneractiveKey(MCStringRef p_new_key);

MCAd* MCAdGetStaticAdsPtr();
void MCAdSetStaticAdsPtr(MCAd* p_ads_ptr);

void MCAdPostMessage(MCAd *, MCAdEventType);
MCAdType MCAdTypeFromString(MCStringRef p_string);

#endif //__MC_MOBILE_AD__
