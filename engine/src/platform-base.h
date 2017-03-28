/*                                                                     -*-c++-*-
Copyright (C) 2017 LiveCode Ltd.

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

#if !defined(MC_PLATFORM_INSIDE)
#	error "Only <platform.h> can be included directly"
#endif

#include <type_traits>
#include <utility>

class MCPlatformStubs;

namespace MCPlatform {

class Base
{
public:
    Base() = default;
    virtual ~Base() {}

    virtual void Retain() const
    {
        /* If there are too many references, make the object permanent. */
        if (m_references == INT64_MAX)
            m_references = -1;
        else if (m_references >= 0)
            ++m_references;
    }
    virtual void Release() const
    {
        /* Only decrement reference count if object hasn't been made permanent. */
        if (m_references > 0)
            --m_references;
        /* If reference count has reached zero, delete the object and prevent
         * further reference-counting operations via this pointer. */
        if (m_references == 0)
        {
            m_references = -1;
            delete this;
        }
    }
    virtual bool QueryInterface(const char * p_interface_id, MCPlatform::Base *&r_interface)
    {
        return false;
    }

private:
    mutable int64_t m_references = 1;
};
    
class CoreReference: public Base
{
public:
    virtual void SetPlatform(MCPlatformCoreRef p_platform) const
    {
        m_platform = p_platform;
    }
    virtual MCPlatformCoreRef GetPlatform(void) const
    {
        return m_platform;
    }
protected:
    mutable MCPlatformCoreRef m_platform = nil;
};

/* MCPlatform::Ref is an automatic reference counting smart pointer template.
 * It's only enabled for types that derive from MCPlatform::Base. */
template <typename T,
          typename std::enable_if<std::is_base_of<Base, T>::value,
                                  int>::type = 0>
class Ref
{
public:
    using element_type = T;
public:
    /* ---------- Constructors */
    /* Construct an empty Ref, either by default construction or with a null
     * pointer. */
    constexpr Ref() {}
    constexpr Ref(decltype(nullptr)) {}

    /* Construct a Ref around any pointer that converts to T.  This makes
     * it easy to construct a Ref<T> where T is an abstract platform interface
     * class using a pointer to a Y where Y is a concrete platform
     * implementation, without needing a cast.  Example:
     *
     *     Ref<Base> t_value = new MCMacPlatformMenu();
     *
     * Warning: This constructor _takes ownership_ of the pointer that you pass
     * to it.
     */
    template <typename Y, typename std::enable_if<
    std::is_convertible<Y*, element_type*>::value,int>::type = 0>
    explicit Ref(Y* ptr) : m_ptr(ptr) {}

    /* Share ownership of the value managed by another Ref of any type
     * convertable to T. */
    template <typename Y, typename std::enable_if<
                  std::is_convertible<Y*, element_type*>::value,int>::type = 0>
    Ref(const Ref<Y>& ref) : m_ptr(ref.get())
    {
        if (m_ptr)
            m_ptr->Retain();
    }

    /* Take ownership of the value managed by another Ref of any type that
     * converts to T.  This is particularly useful in scenarios where you
     * return a concrete platform implementation to a continuation that
     * expects an abstract platform interface.
     */
    template <typename Y, typename std::enable_if<
                  std::is_convertible<Y*, element_type*>::value,int>::type = 0>
    Ref(Ref<Y>&& ref) : m_ptr(ref.unsafeTake()) {}

    /* ---------- Destructors */
    /* When the Ref is destroyed, unreference the value it contains. */
    ~Ref()
    {
        if (m_ptr != nullptr)
            m_ptr->Release();
    }

    /* ---------- Assignment */
    /* Share ownership of the value managed by another Ref of type convertible
     * to T.  If the other Ref is empty, empty this Ref. */
    template <typename Y>
    auto operator=(const Ref<Y>& ref) -> typename std::enable_if<
        std::is_convertible<Y*, element_type*>::value,Ref&>::type
    {
        Ref{ref}.swap(*this);
        return *this;
    }

    /* Transfer ownership of the value managed by another Ref of type
     * convertible to T.  If the other Ref is empty, empty this Ref. */
    template <typename Y>
    auto operator=(Ref<Y>&& ref) -> typename std::enable_if<
        std::is_convertible<Y*, element_type*>::value,Ref&>::type
    {
        Ref{std::move(ref)}.swap(*this);
        return *this;
    }

    /* ---------- Modifiers */
    /* Release ownership of the managed value, if any. */
    void reset()
    {
        Ref{}.swap(*this);
    }
    /* Replace the managed value with another.  If the Ref manages a value,
     * unreference it.
     *
     * Warning: This function _takes ownership_ of the pointer that you pass to
     * it.
     */
    template <typename Y>
    auto reset(Y* ptr) -> typename std::enable_if<
        std::is_convertible<Y*, element_type*>::value,void>::type
    {
        Ref{ptr}.swap(*this);
    }

    /* Exchange the contents of this Ref and another. */
    void swap(Ref& ref)
    {
        std::swap(m_ptr, ref.m_ptr);
    }

    /* Move the contents of this Ref out of it, leaving it empty.
     *
     * Warning: In order to prevent memory leaks, only use this if unavoidable.
     */
    element_type* unsafeTake()
    {
        element_type* t_ptr = m_ptr;
        m_ptr = nullptr;
        return t_ptr;
    }

    /* ---------- Observers */
    operator bool() const { return m_ptr != nullptr; }
    /* Return the value managed by this Ref.  If this Ref is empty, returns
     * nullptr.
     *
     * Warning: To avoid use-after-free errors, avoid using this if possible.
     * If you do use it, try to ensure that you are holding the Ref in a scope
     * that is guaranteed to outlive the use of the value returned by get().
     * For example, use get() to pass the pointer to a function call that
     * doesn't retain the pointer by storing it in a data structure.  If you are
     * passing the pointer to something that will retain it without increasing
     * its reference count, use unsafeTake().
     */
    element_type* get() const { return m_ptr; }

    /* Deference the stored pointer. */
    element_type* operator->() const
    {
        return m_ptr;
    }

    /* Deference the stored pointer. */
    typename std::add_lvalue_reference<element_type>::type operator*() const
    {
        return *m_ptr;
    }

private:

    element_type* m_ptr = nullptr;
};

template <typename T, typename... Args>
Ref<T> makeRef(Args&&... args)
{
    return Ref<T>(new (nothrow) T(std::forward<Args>(args)...));
}

} /* namespace MCPlatform */
