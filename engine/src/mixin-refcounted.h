/* Copyright (C) 2015 LiveCode Ltd.
 
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


#ifndef MIXIN_REFCOUNTED_H
#define MIXIN_REFCOUNTED_H


#include "foundation.h"


// class MCMixinRefcounted
//
//      Provides basic reference counting facilities without using virtual
//      methods.
//
// To use this class with a class MCFoo, declare MCFoo as:
//
//      class MCFoo :
//        public MCMixinRefcounted<MCFoo>
//      { ... };
//
// Despite its strange appearance, this is perfectly legal C++. It is required
// because we are deliberately avoiding using dynamic dispatch (using virtual
// methods) so static dispatch is used and that requires knowing the derived
// class at compile-time.
//
// This class will work correctly with derived classes that use custom delete
// operators.
//
template <class Derived>
class MCMixinRefcounted
{
public:
    
    // Constructor. Refcount is initialised to 1.
    inline MCMixinRefcounted() :
      m_refcount(1) {}
    
    // Increments the reference count.
    // TODO: atomic ops
    inline const Derived* Retain() const
    {
        ++m_refcount;
        return static_cast<const Derived*> (this);
    }
    
    // Increments the reference count.
    // TODO: atomic ops
    inline Derived* Retain()
    {
        ++m_refcount;
        return static_cast<Derived*> (this);
    }
    
    // Decreases the reference count
    // TODO: atomic ops
    inline void Release() const
    {
        if (--m_refcount == 0)
            const_cast<MCMixinRefcounted*>(this)->destroy();
    }
    
protected:
    
    // Protected destructor to prevent direct destruction. Use the reference-
    // counting mechanisms instead!
    inline ~MCMixinRefcounted() {}
    
private:
    
    // Reference count. Mutable to allow refcounting on const objects
    mutable uindex_t m_refcount;
    
    
    // Destroys the object when the refcount == 0 by deleting this as an
    // instance of the derived class. (This is so that the destructor of the
    // derived class will run -- deleting as an instance of MCMixinRefcounted
    // won't work as the destructor is non-virtual to reduce overheads).
    inline void destroy()
    {
        MCAssert(m_refcount == 0);
        delete static_cast<Derived*>(this);
    }
};


// class MCAutoRefcounted
//
template <class T>
class MCAutoRefcounted
{
public:
    
    MCAutoRefcounted() :
      m_object(NULL) {}
    
    MCAutoRefcounted(T* p_object) :
      m_object(p_object)
    {
        ;
    }
    
    ~MCAutoRefcounted()
    {
        if (m_object != NULL)
            m_object->Release();
    }
    
    MCAutoRefcounted& operator= (T* p_object)
    {
        if (p_object != m_object)
        {
            if (m_object)
                m_object->Release();
            m_object = p_object;
        }
        
        return *this;
    }
    
    operator T* () const
    {
        return m_object;
    }
    
    T& operator* () const
    {
        return *m_object;
    }
    
    T* operator-> () const
    {
        return m_object;
    }
    
private:
    
    T* m_object;
};


#endif  /* ifndef MIXIN_REFCOUNTED_H */
