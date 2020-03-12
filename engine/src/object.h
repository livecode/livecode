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

//
// Base class for all display objects
//
#ifndef	OBJECT_H
#define	OBJECT_H

#include "dllst.h"
#include "imagebitmap.h"
#include "objdefs.h"
#include "parsedef.h"
#include "platform.h"
#include "context.h"

#include "native-layer.h"

#include <utility>

enum {
    MAC_SHADOW,
    MAC_THUMB_TOP,
    MAC_THUMB_BACK,
    MAC_THUMB_BOTTOM,
    MAC_THUMB_GRIP,
    MAC_THUMB_HILITE,
    MAC_DISABLED,
    MAC_NCOLORS
};

typedef bool (*MCObjectListFontsCallback)(void *p_context, unsigned int p_index);
typedef bool (*MCObjectResetFontsCallback)(void *p_context, unsigned int p_index, unsigned int p_new_index);

// IM-2015-02-12: [[ ScriptObjectProps ]] Add object visitor flags
// Descend recursively to the children of an object
#define kMCObjectVisitorRecursive (1 << 0)

// Visit the children of an object before itself
#define kMCObjectVisitorDepthFirst (1 << 1)

// Only visit children who are direct descendants of an object.
#define kMCObjectVisitorHeirarchical (1 << 2)

typedef uint32_t MCObjectVisitorOptions;

inline bool MCObjectVisitorIsDepthFirst(MCObjectVisitorOptions p_options)
{
	return (p_options & kMCObjectVisitorDepthFirst) != 0;
}

inline bool MCObjectVisitorIsDepthLast(MCObjectVisitorOptions p_options)
{
	return !MCObjectVisitorIsDepthFirst(p_options);
}

inline bool MCObjectVisitorIsRecursive(MCObjectVisitorOptions p_options)
{
	return (p_options & kMCObjectVisitorRecursive) != 0;
}

inline bool MCObjectVisitorIsHeirarchical(MCObjectVisitorOptions p_options)
{
	return (p_options & kMCObjectVisitorHeirarchical) != 0;
}

/* LEGACY - Set flags for visit styles to perform previously defined behaviour */
enum MCVisitStyle
{
	VISIT_STYLE_DEPTH_FIRST = (kMCObjectVisitorRecursive | kMCObjectVisitorDepthFirst),
	VISIT_STYLE_DEPTH_LAST = (kMCObjectVisitorRecursive)
};

enum MCObjectIntersectType
{
	kMCObjectIntersectBounds,
	kMCObjectIntersectPixels,
	kMCObjectIntersectPixelsWithEffects,
};

enum MCObjectShapeType
{
	kMCObjectShapeEmpty,
	kMCObjectShapeRectangle,
	kMCObjectShapeMask,
	kMCObjectShapeComplex
};

enum MCPropertyChangedMessageType 
{
	kMCPropertyChangedMessageTypeNone = 0,
	kMCPropertyChangedMessageTypePropertyChanged = 1 << 0,
	kMCPropertyChangedMessageTypeResizeControlStarted = 1 << 1,
	kMCPropertyChangedMessageTypeResizeControlEnded = 1 << 2,
	kMCPropertyChangedMessageTypeGradientEditStarted = 1 << 3,
	kMCPropertyChangedMessageTypeGradientEditEnded = 1 << 4
};

enum MCInterfaceTheme
{
    kMCInterfaceThemeEmpty = 0,         // Inherit the theme
    kMCInterfaceThemeNative = 1,        // Native appearance
    kMCInterfaceThemeLegacy = 2         // Backwards-compatibility theming
};

enum MCInterfaceScriptStatus
{
    kMCInterfaceScriptStatusCompiled,
    kMCInterfaceScriptStatusUncompiled,
    kMCInterfaceScriptStatusWarning,
    kMCInterfaceScriptStatusError,
};


struct MCObjectShape
{
	// The type of shape.
	MCObjectShapeType type;
	// The bounds of the shape - this clips the content.
	MCRectangle bounds;
	struct
	{
		// The rectangle of the shape's content (clipped to bounds).
		MCRectangle rectangle;
		
		// The mask of the shape's content (clipped to bounds).
		struct
		{
			MCPoint origin;
			MCImageBitmap *bits;			
			// MM-2012-10-03: [[ ResIndependence ]] The scale of the mask. Used when computing intersect of images with a scale factor.
			MCGFloat scale;
		} mask;
	};
};

struct MCObjectRef
{
	MCObject *object;
	uint4 part;
};

struct MCObjectVisitor
{
	virtual ~MCObjectVisitor(void);
	
	virtual bool OnObject(MCObject *p_object);
	virtual bool OnControl(MCControl *p_control);

	virtual bool OnStack(MCStack *p_stack);
	virtual bool OnAudioClip(MCAudioClip *p_audio_clip);
	virtual bool OnVideoClip(MCVideoClip *p_video_clip);
	virtual bool OnCard(MCCard *p_card);
	virtual bool OnGroup(MCGroup *p_group);
	virtual bool OnField(MCField *p_field);
	virtual bool OnButton(MCButton *p_button);
	virtual bool OnImage(MCImage *p_image);
	virtual bool OnScrollbar(MCScrollbar *p_scrollbar);
	virtual bool OnPlayer(MCPlayer *p_player);
    virtual bool OnGraphic(MCGraphic *p_graphic);
    virtual bool OnEps(MCEPS *p_eps);
	virtual bool OnParagraph(MCParagraph *p_paragraph);
	virtual bool OnBlock(MCBlock *p_block);
	virtual bool OnStyledText(MCStyledText *p_styled_text);
    virtual bool OnWidget(MCWidget *p_widget);
};

#define OBJECT_EXTRA_ARRAYPROPS		(1U << 0)
#define OBJECT_EXTRA_PARENTSCRIPT	(1U << 1)
#define OBJECT_EXTRA_BITMAPEFFECTS  (1U << 2)
#define OBJECT_EXTRA_LAYERMODE		(1U << 3)
// MW-2012-02-19: [[ SplitTextAttrs ]] If this flag is set, then there is a font-flags
//   byte in the extended data section.
#define OBJECT_EXTRA_FONTFLAGS		(1U << 4)
#define OBJECT_EXTRA_THEME_INFO     (1U << 5)       // "theme" and/or "themeClass" properties are present


// Forward declaration of MCObjectCast safe-casting utility function
template <typename T>
inline T* MCObjectCast(MCObject*);


// Proxy allowing for weak references to MCObjects
class MCObjectProxyBase
{
public:
    
    // Create a proxy for the given object
    MCObjectProxyBase(MCObject* p_proxy_form);
    
    // Inform the proxy that the bound object no longer exists
    void Clear();
    
protected:
    
    uint32_t    m_refcount;
    MCObject*   m_object;
    
    // The proxy can only be deleted by itself
    ~MCObjectProxyBase();
    
    void Retain();
    void Release();
    
    MCObject* Get();
    
    bool ObjectExists() const;
};


template <class T = MCObject>
class MCObjectProxy : public MCObjectProxyBase
{
public:
    
    // RAII handle class
    class Handle;
    
private:
    
    T* Get()
    {
        return static_cast<T*>(MCObjectProxyBase::Get());
    }
};


// Mixin class for adding MC<...>Handle support to classes
template <class T>
class MCMixinObjectHandle
{
public:

    // Returns a handle of the appropriate type
    typename MCObjectProxy<T>::Handle GetHandle() const
    {
        // Note: m_weak_proxy is in MCObject therefore always refers to MCObject
        // and casting is required when returning a non-nil handle.
        //
        // The casting here looks evil but is actually well-defined and safe.
        MCObjectProxyBase*& t_weak_proxy = static_cast<const T*>(this)->MCObject::m_weak_proxy;
	MCAssert(t_weak_proxy != nil);
        return typename MCObjectProxy<T>::Handle(static_cast<MCObjectProxy<T>*>(t_weak_proxy));
    }
};

// Slightly more readable name for handles to MCObjects
typedef MCObjectProxy<MCObject>::Handle MCObjectHandle;

// Automatic (RAII) class for storage of object handles
template <class T>
class MCObjectProxy<T>::Handle
{
public:
    
	constexpr Handle() = default;
    
    Handle(MCObjectProxy* p_proxy)
    {
        Set(p_proxy);
    }
    
    explicit Handle(const T* p_object)
    {
        Set(p_object);
    }
    
    Handle(const Handle& p_handle)
    {
        Set(p_handle.m_proxy);
    }
    
    Handle(Handle&& p_handle)
    {
        std::swap(m_proxy, p_handle.m_proxy);
    }

    constexpr Handle(decltype(nullptr)) {}
    
    Handle& operator= (MCObjectProxy* p_proxy)
    {
        Set(p_proxy);
        return *this;
    }
    
    Handle& operator= (T* p_object)
    {
        Set(p_object);
        return *this;
    }
    
    Handle& operator= (const Handle& p_handle)
    {
        Set(p_handle.m_proxy);
        return *this;
    }
    
    Handle& operator=(Handle&& p_handle)
    {
        Set(nullptr);
        std::swap(m_proxy, p_handle.m_proxy);
        return *this;
    }

    Handle& operator= (decltype(nullptr))
    {
        Set(nullptr);
        return *this;
    }
    
    ~Handle()
    {
        if (m_proxy != nil)
            m_proxy->Release();
    }
    
    // Returns true if this handle refers to an object (even if that object has
    // since been deleted)
    bool IsBound() const
    {
        return m_proxy != NULL;
    }
    
    // Returns true if this handle refers to a valid object
    bool IsValid() const
    {
        return m_proxy != NULL && m_proxy->ObjectExists();
    }
    operator bool() const
    {
        return IsValid();
    }
    
    // Pointer-like access
    T* Get() const
    {
        MCAssert(!IsBound() || IsValid());
        
        if (!IsBound())
            return nullptr;
        
        return m_proxy->Get();
    }
    T* operator-> () const
    {
        MCAssert(IsValid());
        return m_proxy->Get();
    }
    operator T* () const
    {
        return Get();
    }
    
    // Unsafe get (the returned pointer is *not* safe to dereference!)
    void* UnsafeGet() const
    {
        if (m_proxy != nullptr)
            return m_proxy->m_object;
        return nullptr;
    }
    
    bool IsBoundTo(const T* ptr) const
    {
        return IsBound() && m_proxy->m_object == ptr;
    }
    
    // Checked fetch as the given type
    template <typename U>
    U* GetAs() const
    {
        return MCObjectCast<U> (Get());
    }
    
    // Two Handles are the same if they use the same proxy
    bool operator==(const Handle& other) const
    {
        return m_proxy == other.m_proxy;
    }
    bool operator!=(const Handle& other) const
    {
        return !(operator==(other));
    }
    
    template <class U>
    bool operator==(const typename MCObjectProxy<U>::Handle& other) const
    {
        return m_proxy == other.m_proxy;
    }
    template <class U>
    bool operator!=(const typename MCObjectProxy<U>::Handle& other) const
    {
        return !(operator==(other));
    }
    
    // Performs a retain on the underlying proxy without an RAII wrapper. This
    // is only provided for the use of the EventQueue and the ExternalV1
    // interface.
    // **DANGEROUS** Will lead to memory leaks if not balanced
    MCObjectProxy* ExternalRetain()
    {
        MCAssert(IsBound());
        m_proxy->Retain();
        return m_proxy;
    }
    
    // Performs a release on the underlying proxy without an RAII wrapper. This
    // is only provided for the use of the EventQueue and the ExternalV1
    // interface.
    // **DANGEROUS** Will cause memory corruption if not balanced
    void ExternalRelease()
    {
        MCAssert(IsBound());
        m_proxy->Release();
    }
    
    // Conversion to handles to compatible types
    // [[ C++11 ]] Currently disabled due to lack of required C++11 support on all platforms
#if NEEDS_CPP_11 && __cplusplus >= 201103L
    template <class U, class = typename std::enable_if<std::is_convertible<T*, U*>::value, void>::type>
    operator typename MCObjectProxy<U>::Handle () const
    {
        return MCObjectProxy<U>::Handle(static_cast<MCObjectProxy<U>*>(m_proxy));
    }
#endif

    /* Swap the contents of two object handles. Allows std::swap() to be
     * used on MCObjectHandle instances of exactly the same type. */
    void swap(Handle& x_other)
    {
        using std::swap;
        swap(m_proxy, x_other.m_proxy);
    }

    friend void swap(Handle& x_left, Handle& x_right)
    {
	    x_left.swap(x_right);
    }

private:
    
    // The proxy for the object this is a handle to
    MCObjectProxy*  m_proxy = nullptr;
    
    void Set(MCObjectProxy* p_proxy)
    {
        if (m_proxy != p_proxy)
        {
            if (m_proxy != nil)
                m_proxy->Release();
            m_proxy = p_proxy;
            if (m_proxy != nil)
                m_proxy->Retain();
        }
    }
    
    void Set(decltype(nullptr))
    {
        Set(static_cast<MCObjectProxy*>(nullptr));
    }
    
    // For MCObject and derivatives
    template <class U>
    void Set(const U* p_object)
    {
        Set(p_object != nullptr ? (p_object->GetHandle().m_proxy) : nullptr);
    }
};


// MW-2012-02-17: [[ LogFonts ]] The font attrs struct for the object.
struct MCObjectFontAttrs
{
	MCNameRef name;
	uint2 style;
	uint2 size;
};

struct MCPropertyInfo;
struct MCObjectPropertyTable
{
	MCObjectPropertyTable *parent;
	uindex_t size;
	MCPropertyInfo *table;
};

struct MCInterfaceNamedColor;
struct MCInterfaceLayer;
struct MCInterfaceShadow;
struct MCInterfaceTextStyle;
struct MCInterfaceTriState;
struct MCExecValue;

struct MCDeletedObjectPool;
void MCDeletedObjectsSetup(void);
void MCDeletedObjectsTeardown(void);
void MCDeletedObjectsFreezePool(void);
void MCDeletedObjectsThawPool(void);
void MCDeletedObjectsEnterWait(bool p_dispatching);
void MCDeletedObjectsLeaveWait(bool p_dispatching);
void MCDeletedObjectsOnObjectCreated(MCObject *object);
void MCDeletedObjectsOnObjectDeleted(MCObject *object);
void MCDeletedObjectsOnObjectDestroyed(MCObject *object);
void MCDeletedObjectsOnObjectSuspendDeletion(MCObject *object, void*& r_cookie);
void MCDeletedObjectsOnObjectResumeDeletion(MCObject *object, void *cookie);

void MCDeletedObjectsDoDrain(void);

struct MCPatternInfo
{
	uint32_t id;
	MCPatternRef pattern;
};

class MCObject : 
  public MCDLlist, 
  public MCMixinObjectHandle<MCObject>
{
protected:
	uint4 obj_id;
	MCObjectHandle parent;
	MCNewAutoNameRef _name;
	uint4 flags;
	MCRectangle rect;
	MCColor *colors;
	MCStringRef *colornames;
	MCStringRef _script;
	MCPatternInfo *patterns;
	MCHandlerlist *hlist;
	MCObjectPropertySet *props;
	uint4 state;
	uint4 scriptdepth;
	uint2 fontheight;
	uint2 dflags;
	uint2 ncolors;
	uint2 npatterns;
	uint2 altid;
	uint1 hashandlers;
	uint1 borderwidth;
	int1 shadowoffset;
	uint1 ink;
	uint1 extraflags;

	// Note: Although this is called blendlevel, it is actually '100 - the blendlevel'
	//   i.e. we store as 0 for trans, 100 for opaque.
	uint1 blendlevel;
	
	// MW-2012-02-16: [[ LogFonts ]] The logical font flags - whether the object's
	//   labels are unicode and what font attrs should be considered as set.
	uint1 m_font_flags;

	// MM-2012-09-05: [[ Property Listener ]]
	bool m_listening : 1;
	uint8_t m_properties_changed : 6;
	
	// MW-2012-10-10: [[ IdCache ]]
	bool m_in_id_cache : 1;
	
	// IM-2013-04-16: [[ BZ 10848 ]] // flag to record encrypted state of object script
	bool m_script_encrypted : 1;
    
    // If this is true, then this object is in the parentScript resolution table.
    bool m_is_parent_script : 1;
	
	// If this is true, then the object is currently suspended from deletion.
	bool m_deletion_is_suspended : 1;
	
    // Whether to use legacy theming (or native-like theming)
    MCInterfaceTheme m_theme;
    
    // Override the type of the control for theming purposes
    MCPlatformControlType m_theme_type;
	
	MCStringRef tooltip;
	
	// MW-2008-10-20: Pointer to the parent script's weak object reference.
	MCParentScriptUse *parent_script;

	// Pointer to the object's weak reference (if any).
	// [[ C++11 ]] The various compilers we support disagree about what this friend declaration should look like
#if defined(__clang__) || defined(_MSC_VER)
	template <class T> friend class MCMixinObjectHandle;
#else
	template <class T> friend typename MCObjectProxy<T>::Handle MCMixinObjectHandle<T>::GetHandle() const;
#endif
	mutable MCObjectProxyBase* m_weak_proxy;

	// MW-2011-07-21: For now, make this a uint4 as imageSrc references can make
	//   it exceed 255 and wrap causing much much badness for the likes of rTree.
	uint32_t opened;

	// MW-2012-02-14: [[ FontRefs ]] The object's concrete font.
	MCFontRef m_font;

	// MW-2012-02-16: [[ LogFonts ]] The object's logical font attrs.
	MCObjectFontAttrs *m_font_attrs;
    
    MCDeletedObjectPool *m_pool;

	static uint1 dashlist[2];
	static uint1 dotlist[2];
	static int1 dashoffset;
	static MCRectangle selrect;
	static int2 startx;
	static int2 starty;
	static uint1 menudepth;
	static MCStack *attachedmenu;
	static MCColor maccolors[MAC_NCOLORS];

	// MW-2012-02-17: [[ LogFonts ]] We store the last loaded font index for
	//   the object being serialized. This is because the fonttable comes
	//   after the stack's object struct.
	static uint2 s_last_font_index;

	// MW-2008-10-28: [[ ParentScripts ]] This boolean flag is set to true if
	//   at some point since it was last unset an object has been loaded that
	//   requires parentScript resolution.
	static bool s_loaded_parent_script_reference;

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCPropertyInfo kModeProperties[];
	static MCObjectPropertyTable kModePropertyTable;

	// The native layer associated with this object
	MCNativeLayer* m_native_layer;
	
public:
    
	MCObject();
	MCObject(const MCObject &oref);
	virtual ~MCObject();
    
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCObjectPropertyTable *getmodepropertytable(void) const { return &kModePropertyTable; }
	
	virtual bool visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor *p_visitor);
    virtual bool visit_self(MCObjectVisitor *p_visitor) = 0;
	virtual bool visit_children(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor *p_visitor);

	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	virtual IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	virtual IO_stat load(IO_handle stream, uint32_t version);
	virtual IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	
	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(MCStringRef p_string, KeySym key);
	virtual Boolean kup(MCStringRef p_string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void mfocustake(MCControl *target);
	virtual void munfocus();
	virtual void mdrag(void);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual uint2 gettransient() const;
	virtual void applyrect(const MCRectangle &nrect);
	void setrect(const MCRectangle &p_rect);

	// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
	virtual void select();
	virtual void deselect();
	virtual Boolean del(bool p_check_flag);
	virtual void paste(void);
	virtual void undo(Ustruct *us);
	virtual void freeundo(Ustruct *us);

    virtual void removereferences(void);
    
	// [[ C++11 ]] MSVC doesn't support typename here while other compilers require it
#ifdef _MSC_VER
	virtual MCObjectProxy<MCStack>::Handle getstack();
#else
	virtual typename MCObjectProxy<MCStack>::Handle getstack();
#endif

	// MW-2011-02-27: [[ Bug 9412 ]] If pass_from is non-nil it means the message is passing from that
    //   object and should continue to be passed if not handled. If it is nil, then the message should
    //   not be passed on.
	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);
	virtual void closemenu(Boolean kfocus, Boolean disarm);
	virtual void recompute();
	
    // FG-2014-10-10: [[ Native Widgets ]] Informs the object that the selected tool has changed
    virtual void toolchanged(Tool p_new_tool);
    
    // FG-2014-10-14: [[ Native Widgets ]] Informs the object that its layer has changed
    virtual void layerchanged();
    
    // AL-2015-09-23: [[ Native Widgets ]] Informs the object that its visibility has changed
    virtual void visibilitychanged(bool p_visible);
	
	// IM-2015-12-11: [[ Native Widgets ]] Informs the object that its rect has changed
	virtual void geometrychanged(const MCRectangle &p_rect);

	// IM-2016-01-19: [[ NativeWidgets ]] Informs the object that its visible area has changed.
	virtual void viewportgeometrychanged(const MCRectangle &p_rect);
	
	// IM-2016-09-20: [[ Bug 16965 ]] Inform the object that the stack view transform has changed.
	virtual void OnViewTransformChanged();
	
	// IM-2015-12-16: [[ NativeWidgets ]] Informs the object that it has been opened.
	virtual void OnOpen();
	// IM-2015-12-16: [[ NativeWidgets ]] Informs the object that it will be closed.
	virtual void OnClose();
	
	// IM-2016-08-16: [[ Bug 18153 ]] Inform the object that the stack has been attached to the window
	virtual void OnAttach();
	
	// IM-2016-08-16: [[ Bug 18153 ]] Inform the object that the stack will be removed from the window
	virtual void OnDetach();
    
	// MW-2011-09-20: [[ Collision ]] Compute the shape of the object's mask.
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	// MW-2012-02-14: [[ FontRefs ]] Remap any fonts this object uses as font related property has changed.
    // Set force to true to recompute even if it looks like nothing needs updated
	virtual bool recomputefonts(MCFontRef parent_font, bool force = false);

	// MW-2012-02-14: [[ FontRefs ]] Returns the current concrete fontref of the object.
	MCFontRef getfontref(void) const { return m_font; }

    
    Exec_stat sendgetprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCValueRef& r_value);
    Exec_stat sendsetprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCValueRef p_value);
    
    virtual bool getprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue& r_value);
	virtual bool setprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue p_value);
	virtual bool getcustomprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCProperListRef p_path, MCExecValue& r_value);
	virtual bool setcustomprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCProperListRef p_path, MCExecValue p_value);
    
	// MW-2012-05-28: [[ Value Prop Accessors ]] These methods allow access to object props
	//   via direct types. Appropriate type coercion will be performed, with errors thrown as
	//   necessary.
	//   The methods return no value since they will 'Throw()' if an error occurs - check
	//   ctxt.HasError() on return to ensure things are okay.
	//   Note that these functions will clobber the ctxt's EP.
	void getboolprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, bool& r_value);
	void getintprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, integer_t& r_value);
	void getuintprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, uinteger_t& r_value);
	void getdoubleprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, double& r_value);
	void getnumberprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCNumberRef& r_value);
	void getstringprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCStringRef& r_value);
	void getarrayprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCArrayRef& r_value);
	void getvariantprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCValueRef& r_value);
    void getdataprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCDataRef& r_value);
    void getpointprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCPoint& r_point);
	//
	void setboolprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, bool r_value);
	void setintprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, integer_t r_value);
	void setuintprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, uinteger_t r_value);
	void setdoubleprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, double r_value);
	void setnumberprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCNumberRef r_value);
	void setstringprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCStringRef r_value);
	void setarrayprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCArrayRef r_value);
    void setvariantprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCValueRef r_value);
    void setdataprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, MCDataRef r_value);
    void setpointprop(MCExecContext& ctxt, uint32_t parid, Properties which, Boolean effective, const MCPoint& p_point);

	const MCRectangle &getrect() const;
    virtual MCRectangle getrectangle(bool p_effective) const;

	// MW-2012-06-08: [[ Relayer ]] Move a control within this container (if card or group) so that
	//   'source' is before 'target'.
	virtual void relayercontrol(MCControl *source, MCControl *target);
	virtual void relayercontrol_remove(MCControl *control);
	virtual void relayercontrol_insert(MCControl *control, MCControl *target);

	bool GetNativeView(void *&r_view);
	bool SetNativeView(void *p_view);
	
	// Gets the current native layer (if any) associated with this object
	MCNativeLayer* getNativeLayer() const;
	
	// IM-2016-04-26: [[ WindowsPlayer ]] Retrieve the rect of the native view attached to the object.
	virtual MCRectangle GetNativeViewRect(const MCRectangle &p_object_rect);
	
	MCNameRef getdefaultpropsetname(void);

    
	uint4 getid() const
	{
		return obj_id;
	}
	uint4 getaltid() const
	{
		return altid;
	}
	void setid(uint4 inid)
	{
		obj_id = inid;
	}

	// Returns true if the object has not been named.
	bool isunnamed(void) const
	{
		return MCNameIsEmpty(getname());
	}

	// Returns the name of the object.
	MCNameRef getname(void) const
	{
		return *_name;
	}

	// Tests to see if the object has the given name, interpreting unnamed as
	// the empty string.
	bool hasname(MCNameRef p_other_name);

	// Set the object's name, interpreting the empty string as unnamed.
	void setname(MCNameRef new_name);
	void setname_cstring(const char *p_new_name);

	uint32_t getopened() const
	{
		return opened;
	}
	uint1 gethashandlers() const
	{
		return hashandlers;
	}
	bool getflag(uint4 flag) const
	{
		return (flags & flag) != 0;
	}
	bool getextraflag(uint4 flag) const
	{
		return (extraflags & flag) != 0;
	}
	uint4 getflags(void) const
	{
		return flags;
	}
	MCStringRef _getscript(void)
	{
		return _script;
	}
	MCHandlerlist *gethandlers(void)
	{
		return hlist;
	}

	uint32_t getopacity(void) { return blendlevel * 255 / 100; }
	uint32_t getink(void) { return ink; }

	bool changeflag(bool setting, uint32_t mask);
	bool changeextraflag(bool setting, uint32_t mask);
	bool changestate(bool setting, uint32_t mask);

	void setflag(uint4 on, uint4 flag);
	void setextraflag(uint4 on, uint4 flag);

	bool getstate(uint4 flag) const
	{
		return (state & flag) != 0;
	}
	void setstate(Boolean on, uint4 newstate);

	Boolean isdisabled() const
	{
		return (flags & F_DISABLED) != 0;
	}

	Exec_stat setsprop(Properties which, MCStringRef p_string);

	void help();

	Boolean getselected() const
	{
		return (state & CS_SELECTED) != 0;
	}

	bool isselectable(bool p_only_object = false) const;
	uint1 getborderwidth(void) {return borderwidth;}

	MCObject *getparent() const
	{
        if (parent)
            return parent;
        else
            return NULL;
	}
	
	uint4 getscriptdepth() const
	{
		return scriptdepth;
	}
	
	void lockforexecution(void)
	{
		scriptdepth += 1;
	}
	
	void unlockforexecution(void)
	{
		scriptdepth -= 1;
	}
	
	void setparent(MCObject *newparent)
	{
		parent = newparent;
	}
	MCCard *getcard(uint4 cid = 0);
	Window getw();

	// IMAGE OVERHAUL
	uint16_t getdashes(uint8_t *&r_dashlist, int8_t &r_dashoffset)
	{
		r_dashlist = dashlist;
		r_dashoffset = dashoffset;
		return 2;
	}
	// MW-2009-01-29: [[ Inherited parentScripts ]]
	// Returns the current parentScript attached to this stack. If there
	// is no parentScript, it returns NULL - note that this is an MCParentScript,
	// not an MCParentScriptUse.
	MCParentScript *getparentscript(void) const;
    
    // Set the parentScript of the object at load time of it (used by the script
    // only stack loader).
    bool setparentscript_onload(uint32_t p_id, MCNameRef p_stack);

	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// This method returns false if there was not enough memory to complete the
	// resolution.
	virtual bool resolveparentscript(void);
	
	// MW-2009-02-02: [[ Improved image search ]]
	// This method searches for the image with the given id, taking into account
	// the containment and behavior hierarchy of this object.
	MCImage *resolveimageid(uint4 image_id);
	MCImage *resolveimagename(MCStringRef name);
	
	// IM-2015-12-07: [[ ObjectVisibility ]]
	// Returns the visibility of the object. If effective is true then only returns true if the parent (and its parent, etc.) is also visible.
	bool isvisible(bool p_effective = true);
	
	// IM-2016-02-26: [[ Bug 16244 ]]
	// Returns whether to show this object when its 'visible' property is false
	bool showinvisible();
	
	Boolean resizeparent();
	Boolean getforecolor(uint2 di, Boolean reversed, Boolean hilite, MCColor &c,
	                     MCPatternRef &r_pattern, int2 &x, int2 &y, MCContextType dc_type,
                         MCObject *o, bool selected = false);
	void setforeground(MCDC *dc, uint2 di, Boolean rev, Boolean hilite = False, bool selected = false);
	Boolean setcolor(uint2 index, const MCString &eptr);
	Boolean setcolors(const MCString &data);
	Boolean setpattern(uint2 newpixmap, MCStringRef);
	Boolean setpatterns(MCStringRef data);
	Boolean getcindex(uint2 di, uint2 &i);
	uint2 createcindex(uint2 di);
	void destroycindex(uint2 di, uint2 i);
	Boolean getpindex(uint2 di, uint2 &i);
	uint2 createpindex(uint2 di);
	void destroypindex(uint2 di, uint2 i);
	MCColor *getcolors(void) {return colors;}
	void allowmessages(Boolean allow);

	// MW-2012-02-21: [[ FieldExport ]] This method returns the effective pixel
	//   value for the given color index.
	uint32_t getcoloraspixel(uint2 di);
	
	// MW-2012-02-17: [[ LogFonts ]] Method to set the font-attrs - used to
	//   set the dispatcher attrs and by the HC import code.
	void setfontattrs(MCStringRef textfont, uint2 textsize, uint2 textstyle);

	// MW-2012-02-17: [[ LogFonts ]] Fetch the (effective) font attrs for the
	//   object.
	uint32_t getfontattsnew(MCNameRef& fname, uint2& fsize, uint2& fstyle);
	//void getfontattsnew(MCStringRef& fname, uint2& fsize, uint2& fstyle);

	// MW-2012-02-16: [[ LogFonts ]] Return the (effective) textFont setting. 
	MCNameRef gettextfont(void);
	// MW-2012-02-16: [[ LogFonts ]] Return the (effective) textHeight setting.
	uint2 gettextheight(void);
	// MW-2012-02-16: [[ LogFonts ]] Return the (effective) textSize setting.
	uint2 gettextsize(void);
	// MW-2012-02-16: [[ LogFonts ]] Return the (effective) textStyle setting.
	uint2 gettextstyle(void);

	// MW-2012-02-16: [[ LogFonts ]] Return whether the object's text is unicode.
	bool hasunicode(void) const { return (m_font_flags & FF_HAS_UNICODE) != 0; }

	// MW-2008-10-31: [[ ParentScripts ]] This method will send <p_message> to this
	//   but only if <p_flag> is among the bits of hashandlers for this object or
	//   its parent object.
	Exec_stat conditionalmessage(uint32_t p_flag, MCNameRef name);
	
	// MW-2009-08-26: Refactored method for dispatch - used by externals API.
	Exec_stat dispatch(Handler_type type, MCNameRef name, MCParameter *params);

	Exec_stat message(MCNameRef name, MCParameter *p = NULL, Boolean changedefault = True, Boolean send = False, Boolean p_force = False);
	Exec_stat message_with_valueref_args(MCNameRef name, MCValueRef v1);
	Exec_stat message_with_valueref_args(MCNameRef name, MCValueRef v1, MCValueRef v2);
    Exec_stat message_with_valueref_args(MCNameRef name, MCValueRef v1, MCValueRef v2, MCValueRef v3);
    Exec_stat message_with_valueref_args(MCNameRef name, MCValueRef v1, MCValueRef v2, MCValueRef v3, MCValueRef v4);
	Exec_stat message_with_args(MCNameRef name, int4 v1);
	Exec_stat message_with_args(MCNameRef name, int4 v1, int4 v2);
	Exec_stat message_with_args(MCNameRef name, int4 v1, int4 v2, int4 v3);
	Exec_stat message_with_args(MCNameRef name, int4 v1, int4 v2, int4 v3, int4 v4);
	
	void senderror();
	void sendmessage(Handler_type htype, MCNameRef mess, Boolean handled);
	
	// New method for returning the various 'names' of an object. This should really
	// return an 'MCValueRef' at some point, but as it stands that causes issues.
	bool names(Properties which, MCValueRef& r_name);
    bool getnameproperty(Properties which, uint32_t p_part_id, MCValueRef& r_name_val);
    
	Boolean parsescript(Boolean report, Boolean force = False);
	void drawshadow(MCDC *dc, const MCRectangle &drect, int2 soffset);
	void draw3d(MCDC *dc, const MCRectangle &drect,
	            Etch style, uint2 bwidth);
	void drawborder(MCDC *dc, const MCRectangle &drect, uint2 bwidth);
	void positionrel(const MCRectangle &dptr, Object_pos xpos, Object_pos ypos);
    void drawmarquee(MCContext *p_context, const MCRectangle &p_rect);
    
    // SN-2014-04-16 [[ Bug 12078 ]] Buttons and tooltip label are not drawn in the text direction
    void drawdirectionaltext(MCDC *dc, int2 sx, int2 sy, MCStringRef p_string, MCFontRef font);

	Exec_stat domess(MCStringRef sptr, MCParameter *args = nil, bool p_ignore_errors = true);
    
	void eval(MCExecContext& ctxt, MCStringRef p_script, MCValueRef& r_value);
	// MERG 2013-9-13: [[ EditScriptChunk ]] Added at expression that's passed through as a second parameter to editScript
    void editscript(MCStringRef p_at = nil);

	void removefrom(MCObjectList *l);
	Boolean attachmenu(MCStack *sptr);
	void alloccolors();

    bool handlesmessage(MCNameRef p_message);
	Bool hashandler(Handler_type p_type, MCNameRef name);
	MCHandler *findhandler(Handler_type p_type, MCNameRef name);

	Exec_stat exechandler(MCHandler *p_handler, MCParameter *p_params);

	// MW-2008-10-28: [[ ParentScripts ]] This method executes the given handler
	//   in 'parent' execution context.
	// MW-2009-01-28: [[ Inherited parentScripts ]] The context parameter indicates
	//   which parentScript we are currently executing inside.
	Exec_stat execparenthandler(MCHandler *p_handler, MCParameter *p_params, MCParentScriptUse *p_parentscript);

	// MW-2009-01-29: [[ Bug ]] Cards and stack parentScripts don't work.
	// This method attempts to handle the given message in this object, invoking
	// parentScripts as appropriate.
	Exec_stat handleself(Handler_type type, MCNameRef message, MCParameter* parameters);

	// MW-2012-08-08: [[ BeforeAfter ]] Execute a handler in a parentscript of the given
	//   type.
	Exec_stat handleparent(Handler_type type, MCNameRef message, MCParameter* parameters);

	// IM-2013-07-24: [[ ResIndependence ]] Add scale factor to allow taking high-res snapshots
	MCImageBitmap *snapshot(const MCRectangle *rect, const MCPoint *size, MCGFloat p_scale_factor, bool with_effects);

	// MW-2011-09-20: [[ Collision ]] Check to see if the two objects touch (exactly).
	bool intersects(MCObject *other, uint32_t threshold);
	
	// MW-2012-02-19: [[ SplitTextAttrs ]] Returns true if the object needs to save a
	//   font record. (Called by the font table visitor - so public).
	bool needtosavefontrecord(void) const;

	// AL-2014-02-14: [[ UnicodeFileFormat ]] If 'include_legacy' is set then 2.7, 5.5 and 7.0 versions
	//   of the objects will be included.
	static MCPickleContext *startpickling(bool include_legacy);
	static void continuepickling(MCPickleContext *p_context, MCObject *p_object, uint4 p_part);
	static void pickle(MCObject *p_object, uint4 p_part, MCDataRef& r_string);
	static void stoppickling(MCPickleContext *p_context, MCDataRef& r_string);
	static MCObject *unpickle(MCDataRef p_data, MCStack *p_stack);
	
	// in DLList overrides
	MCObject *next()
	{
		return (MCObject *)MCDLlist::next();
	}
	MCObject *prev()
	{
		return (MCObject *)MCDLlist::prev();
	}
	void totop(MCObject *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCObject *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCObject *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCObject *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCObject *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCObject *remove(MCObject *&list)
	{
		return (MCObject *)MCDLlist::remove((MCDLlist *&)list);
	}

	// MM-2012-09-05: [[ Property Listener ]]
	void listen(void)
	{
		m_listening = true;
	}
	
	void unlisten(void)
	{
		m_listening = false;
		m_properties_changed = kMCPropertyChangedMessageTypeNone;
	}
	
	void signallisteners(Properties which);
	
	void signallistenerswithmessage(uint8_t p_message);
    
	uint8_t propertieschanged(void)
	{
		if (m_properties_changed != kMCPropertyChangedMessageTypeNone)
		{
			uint8_t t_properties_changed;
			t_properties_changed = m_properties_changed;
			m_properties_changed = kMCPropertyChangedMessageTypeNone;
			return t_properties_changed;
		}
		return kMCPropertyChangedMessageTypeNone;
	}	

	virtual void scheduledelete(bool p_is_child = false);

    virtual bool isdeletable(bool p_check_flag);
    
    // MW-2012-10-10: [[ IdCache ]]
	void setinidcache(bool p_value)
	{
		m_in_id_cache = p_value;
	}
	
	bool getinidcache(void)
	{
		return m_in_id_cache;
    }

	// IM-2013-02-11 image change notification (used by button icons, field images, etc.)
	// returns true if the referenced image is still in use by this object
	virtual bool imagechanged(MCImage *p_image, bool p_deleting)
	{
        return false;
    }
    
    void setisparentscript(bool p_value)
    {
        m_is_parent_script = p_value;
    }
    
    bool getisparentscript(void)
    {
        return m_is_parent_script;
    }
    
    MCRectangle measuretext(MCStringRef p_text, bool p_is_unicode);
    
    // Copy the font which should be used for this object. This will map/unmap
    // as appropriate.
    void copyfont(MCFontRef& r_font);
    
	// IM-2016-07-06: [[ Bug 17690 ]] Return the minimum stack file version that can fully
	//   encode this object and its properties.
	virtual uint32_t getminimumstackfileversion(void);
	
	// IM-2016-07-06: [[ Bug 17690 ]] Returns the minimum stack file version of this object
	//   and any child objects.
	uint32_t geteffectiveminimumstackfileversion(void);
	
    // Currently non-functional: always returns false
    bool is_rtl() const { return false; }
    
    // AL-2015-06-30: [[ Bug 15556 ]] Refactored function to sync mouse focus
	void sync_mfocus(bool p_visiblility_changed, bool p_resize_parent);
    
    // This accessor is used by the widget event manager to trigger tooltip
    // display for widgets.
    MCStringRef gettooltip(void) {return tooltip;}
    
    // Returns true if this object is an ancestor *control* of p_object
    //  in the parent chain.
    bool isancestorof(MCObject *p_object);
    
    // Reinstate the weak proxy object (used after an object is deleted, but is
    // in the undo queue).
    void ensure_weak_proxy(void) { if (m_weak_proxy == nullptr) m_weak_proxy = new MCObjectProxyBase(this); }
    
    ////////// PROPERTY SUPPORT METHODS

	void Redraw(void);

	bool GetPixel(MCExecContext& ctxt, Properties which, bool effective, uinteger_t& r_pixel);
	void SetPixel(MCExecContext& ctxt, Properties which, uinteger_t pixel);

	bool GetColor(MCExecContext& ctxt, Properties which, bool effective, MCInterfaceNamedColor& r_color, bool recursive = false);
	void SetColor(MCExecContext& ctxt, int index, const MCInterfaceNamedColor& p_color);
	bool GetColors(MCExecContext& ctxt, bool effective, MCStringRef& r_colors);

	bool GetPattern(MCExecContext& ctxt, Properties which, bool effective, uint4*& r_pattern);
	void SetPattern(MCExecContext& ctxt, uint2 p_new_pixmap, uint4* p_new_id);
	bool GetPatterns(MCExecContext& ctxt, bool effective, MCStringRef& r_patterns);

	void SetRectProp(MCExecContext& ctxt, bool effective, MCRectangle p_rect);
	void GetRectPoint(MCExecContext& ctxt, bool effective, Properties which, MCPoint &r_point);
	void SetRectPoint(MCExecContext& ctxt, bool effective, Properties which, MCPoint point);
	void GetRectValue(MCExecContext& ctxt, bool effective, Properties which, integer_t& r_value);
	void SetRectValue(MCExecContext& ctxt, bool effective, Properties which, integer_t value);

	bool TextPropertyMapFont();
	void TextPropertyUnmapFont(bool p_unmap);
	
    void GetCardIds(MCExecContext& ctxt, MCCard *p_cards, bool p_all, uint32_t p_id, uindex_t& r_count, uinteger_t*& r_ids);
    void GetCardNames(MCExecContext& ctxt, MCCard *p_cards, bool p_all, uint32_t p_id, uindex_t& r_count, MCStringRef*& r_names);
    
    void DoGetProperties(MCExecContext& ctxt, uint32_t part, bool p_effective, MCArrayRef& r_props);
    
	////////// PROPERTY ACCESSORS
	
	void GetId(MCExecContext& ctxt, uint32_t& r_id);
	virtual void SetId(MCExecContext& ctxt, uint32_t id);
	void GetAbbrevId(MCExecContext& ctxt, MCStringRef& r_abbrev_id);
	void GetLongName(MCExecContext& ctxt, uint32_t p_part_id, MCStringRef& r_long_name);
	void GetLongId(MCExecContext& ctxt, uint32_t p_part_id, MCStringRef& r_long_id);
	void GetName(MCExecContext& ctxt, MCStringRef& r_name);
	virtual void SetName(MCExecContext& ctxt, MCStringRef name);
	void GetAbbrevName(MCExecContext& ctxt, MCStringRef& r_abbrev_name);
	void GetShortName(MCExecContext& ctxt, MCStringRef& r_short_name);
	
	void GetAltId(MCExecContext& ctxt, uinteger_t& r_alt_id);
	void SetAltId(MCExecContext& ctxt, uinteger_t alt_id);

	void GetLayer(MCExecContext& ctxt, uint32_t part, MCInterfaceLayer& r_layer);
	void SetLayer(MCExecContext& ctxt, uint32_t part, const MCInterfaceLayer& p_layer);

	void GetScript(MCExecContext& ctxt, MCStringRef& r_script);
	void SetScript(MCExecContext& ctxt, MCStringRef new_script);
	void GetParentScript(MCExecContext& ctxt, MCStringRef& r_p_script);
	void SetParentScript(MCExecContext& ctxt, MCStringRef new_parent_script);
	void GetNumber(MCExecContext& ctxt, uint32_t part, uinteger_t& r_number);
	void GetEffectiveForePixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetForePixel(MCExecContext& ctxt, uinteger_t*& r_pixel);
	virtual void SetForePixel(MCExecContext& ctxt, uinteger_t* pixel);
	void GetEffectiveBackPixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetBackPixel(MCExecContext& ctxt, uinteger_t*& r_pixel);
	virtual void SetBackPixel(MCExecContext& ctxt, uinteger_t* pixel);
	void GetEffectiveHilitePixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetHilitePixel(MCExecContext& ctxt, uinteger_t*& r_pixel);
	virtual void SetHilitePixel(MCExecContext& ctxt, uinteger_t* pixel);
	void GetEffectiveBorderPixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetBorderPixel(MCExecContext& ctxt, uinteger_t*& r_pixel);
	virtual void SetBorderPixel(MCExecContext& ctxt, uinteger_t* pixel);
	void GetEffectiveTopPixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetTopPixel(MCExecContext& ctxt, uinteger_t*& r_pixel);
	virtual void SetTopPixel(MCExecContext& ctxt, uinteger_t* pixel);
	void GetEffectiveBottomPixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetBottomPixel(MCExecContext& ctxt, uinteger_t*& r_pixel);
	virtual void SetBottomPixel(MCExecContext& ctxt, uinteger_t* pixel);
	void GetEffectiveShadowPixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetShadowPixel(MCExecContext& ctxt, uinteger_t*& r_pixel);
	virtual void SetShadowPixel(MCExecContext& ctxt, uinteger_t* pixel);
	void GetEffectiveFocusPixel(MCExecContext& ctxt, uinteger_t& r_pixel);
	void GetFocusPixel(MCExecContext& ctxt, uinteger_t*& r_pixel);
	virtual void SetFocusPixel(MCExecContext& ctxt, uinteger_t* pixel);

	void GetPenBackColor(MCExecContext& ctxt, MCValueRef& r_value);
	void SetPenBackColor(MCExecContext& ctxt, MCValueRef r_value);
	void GetBrushBackColor(MCExecContext& ctxt, MCValueRef& r_value);
	void SetBrushBackColor(MCExecContext& ctxt, MCValueRef r_value);
	void GetPenPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	void SetPenPattern(MCExecContext& ctxt, uinteger_t* pattern);
	void GetBrushPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	void SetBrushPattern(MCExecContext& ctxt, uinteger_t* pattern);

	void GetForeColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	virtual void SetForeColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	void GetEffectiveForeColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetBackColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	virtual void SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	void GetEffectiveBackColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	virtual void SetHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	void GetEffectiveHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetBorderColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	virtual void SetBorderColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	void GetEffectiveBorderColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetTopColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	virtual void SetTopColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	void GetEffectiveTopColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetBottomColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	virtual void SetBottomColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	void GetEffectiveBottomColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetShadowColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	virtual void SetShadowColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	void GetEffectiveShadowColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetFocusColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	virtual void SetFocusColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	void GetEffectiveFocusColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);

	void GetColors(MCExecContext& ctxt, MCStringRef& r_colors);
	void SetColors(MCExecContext& ctxt, MCStringRef colors);
	void GetEffectiveColors(MCExecContext& ctxt, MCStringRef& r_colors);

	void GetForePattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	virtual void SetForePattern(MCExecContext& ctxt, uinteger_t* pattern);
	void GetEffectiveForePattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	void GetBackPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	virtual void SetBackPattern(MCExecContext& ctxt, uinteger_t* pattern);
	void GetEffectiveBackPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	void GetHilitePattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	virtual void SetHilitePattern(MCExecContext& ctxt, uinteger_t* pattern);
	void GetEffectiveHilitePattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	void GetBorderPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	virtual void SetBorderPattern(MCExecContext& ctxt, uinteger_t* pattern);
	void GetEffectiveBorderPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	void GetTopPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	virtual void SetTopPattern(MCExecContext& ctxt, uinteger_t* pattern);
	void GetEffectiveTopPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	void GetBottomPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	virtual void SetBottomPattern(MCExecContext& ctxt, uinteger_t* pattern);
	void GetEffectiveBottomPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	void GetShadowPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	virtual void SetShadowPattern(MCExecContext& ctxt, uinteger_t* pattern);
	void GetEffectiveShadowPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	void GetFocusPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);
	virtual void SetFocusPattern(MCExecContext& ctxt, uinteger_t* pattern);
	void GetEffectiveFocusPattern(MCExecContext& ctxt, uinteger_t*& r_pattern);

	void GetPatterns(MCExecContext& ctxt, MCStringRef& r_patterns);
	void SetPatterns(MCExecContext& ctxt, MCStringRef patterns);
	void GetEffectivePatterns(MCExecContext& ctxt, MCStringRef& r_patterns);

	void GetLockLocation(MCExecContext& ctxt, bool& r_setting);
	void SetLockLocation(MCExecContext& ctxt, bool setting);

	void GetTextHeight(MCExecContext& ctxt, uinteger_t*& r_height);
	virtual void SetTextHeight(MCExecContext& ctxt, uinteger_t* height);
	void GetEffectiveTextHeight(MCExecContext& ctxt, uinteger_t& r_height);
	void GetTextAlign(MCExecContext& ctxt, intenum_t*& r_align);
	void SetTextAlign(MCExecContext& ctxt, intenum_t* align);
	void GetEffectiveTextAlign(MCExecContext& ctxt, intenum_t& r_align);
	void GetTextFont(MCExecContext& ctxt, MCStringRef& r_font);
	virtual void SetTextFont(MCExecContext& ctxt, MCStringRef font);
	void GetEffectiveTextFont(MCExecContext& ctxt, MCStringRef& r_font);
	void GetTextSize(MCExecContext& ctxt, uinteger_t*& r_size);
	virtual void SetTextSize(MCExecContext& ctxt, uinteger_t* size);
	void GetEffectiveTextSize(MCExecContext& ctxt, uinteger_t& r_size);
	void GetTextStyle(MCExecContext& ctxt, MCInterfaceTextStyle& r_style);
	virtual void SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style);
	void GetEffectiveTextStyle(MCExecContext& ctxt, MCInterfaceTextStyle& r_style);

	void GetShowBorder(MCExecContext& ctxt, bool& r_setting);
	virtual void SetShowBorder(MCExecContext& ctxt, bool setting);
	void GetShowFocusBorder(MCExecContext& ctxt, bool& r_setting);
	virtual void SetShowFocusBorder(MCExecContext& ctxt, bool setting);

	void GetBorderWidth(MCExecContext& ctxt, uinteger_t& r_width);
	virtual void SetBorderWidth(MCExecContext& ctxt, uinteger_t width);
	void GetOpaque(MCExecContext& ctxt, bool& r_setting);
	virtual void SetOpaque(MCExecContext& ctxt, bool setting);
	void GetShadow(MCExecContext& ctxt, MCInterfaceShadow& r_shadow);
	virtual void SetShadow(MCExecContext& ctxt, const MCInterfaceShadow& p_shadow);
	void GetShadowOffset(MCExecContext& ctxt, integer_t& r_offset);
	void SetShadowOffset(MCExecContext& ctxt, integer_t offset);
	void Get3D(MCExecContext& ctxt, bool& r_setting);
	virtual void Set3D(MCExecContext& ctxt, bool setting);

	void GetVisible(MCExecContext& ctxt, uint32_t part, bool& r_setting);
	virtual void SetVisible(MCExecContext& ctxt, uint32_t part, bool setting);
    void GetEffectiveVisible(MCExecContext& ctxt, uint32_t part, bool& r_setting);
	void GetInvisible(MCExecContext& ctxt, uint32_t part, bool& r_setting);
	virtual void SetInvisible(MCExecContext& ctxt, uint32_t part, bool setting);
    void GetEffectiveInvisible(MCExecContext& ctxt, uint32_t part, bool& r_setting);
	void GetEnabled(MCExecContext& ctxt, uint32_t part, bool& r_setting);
	virtual void SetEnabled(MCExecContext& ctxt, uint32_t part, bool setting);
	void GetDisabled(MCExecContext& ctxt, uint32_t part, bool& r_setting);
	virtual void SetDisabled(MCExecContext& ctxt, uint32_t part, bool setting);
	void GetSelected(MCExecContext& ctxt, bool& r_setting);
	void SetSelected(MCExecContext& ctxt, bool setting);
	void GetTraversalOn(MCExecContext& ctxt, bool& r_setting);
	void SetTraversalOn(MCExecContext& ctxt, bool setting);

	void GetOwner(MCExecContext& ctxt, MCStringRef& r_owner);
	void GetShortOwner(MCExecContext& ctxt, MCStringRef& r_owner);
	void GetAbbrevOwner(MCExecContext& ctxt, MCStringRef& r_owner);
	void GetLongOwner(MCExecContext& ctxt, uint32_t p_part_id, MCStringRef& r_owner);

	void GetProperties(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_props);
	void SetProperties(MCExecContext& ctxt, uint32_t part, MCArrayRef props);
    void GetEffectiveProperties(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_props);
	void GetCustomPropertySet(MCExecContext& ctxt, MCStringRef& r_propset);
	void SetCustomPropertySet(MCExecContext& ctxt, MCStringRef propset);
	void GetCustomPropertySets(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_propsets);
	void SetCustomPropertySets(MCExecContext& ctxt, uindex_t p_count, MCStringRef* propsets);
    
	void GetInk(MCExecContext& ctxt, intenum_t& r_ink);
	virtual void SetInk(MCExecContext& ctxt, intenum_t ink);
	void GetCantSelect(MCExecContext& ctxt, bool& r_setting);
	virtual void SetCantSelect(MCExecContext& ctxt, bool setting);
	void GetEffectiveCantSelect(MCExecContext& ctxt, bool& r_setting);
	void GetBlendLevel(MCExecContext& ctxt, uinteger_t& r_level);
	virtual void SetBlendLevel(MCExecContext& ctxt, uinteger_t level);

	void GetLocation(MCExecContext& ctxt, MCPoint& r_location);
	void SetLocation(MCExecContext& ctxt, MCPoint location);
	void GetEffectiveLocation(MCExecContext& ctxt, MCPoint& r_location);
	void SetEffectiveLocation(MCExecContext& ctxt, MCPoint location);

	void GetLeft(MCExecContext& ctxt, integer_t& r_value);
	void SetLeft(MCExecContext& ctxt, integer_t value);
	void GetEffectiveLeft(MCExecContext& ctxt, integer_t& r_value);
	void SetEffectiveLeft(MCExecContext& ctxt, integer_t value);
	void GetRight(MCExecContext& ctxt, integer_t& r_value);
	void SetRight(MCExecContext& ctxt, integer_t value);
	void GetEffectiveRight(MCExecContext& ctxt, integer_t& r_value);
	void SetEffectiveRight(MCExecContext& ctxt, integer_t value);
	void GetTop(MCExecContext& ctxt, integer_t& r_value);
	void SetTop(MCExecContext& ctxt, integer_t value);
	void GetEffectiveTop(MCExecContext& ctxt, integer_t& r_value);
	void SetEffectiveTop(MCExecContext& ctxt, integer_t value);
	void GetBottom(MCExecContext& ctxt, integer_t& r_value);
	void SetBottom(MCExecContext& ctxt, integer_t value);
	void GetEffectiveBottom(MCExecContext& ctxt, integer_t& r_value);
	void SetEffectiveBottom(MCExecContext& ctxt, integer_t value);

	void GetTopLeft(MCExecContext& ctxt, MCPoint& r_value);
	void SetTopLeft(MCExecContext& ctxt, MCPoint value);
	void GetEffectiveTopLeft(MCExecContext& ctxt, MCPoint& r_value);
	void SetEffectiveTopLeft(MCExecContext& ctxt, MCPoint value);
	void GetTopRight(MCExecContext& ctxt, MCPoint& r_value);
	void SetTopRight(MCExecContext& ctxt, MCPoint value);
	void GetEffectiveTopRight(MCExecContext& ctxt, MCPoint& r_value);
	void SetEffectiveTopRight(MCExecContext& ctxt, MCPoint value);
	void GetBottomLeft(MCExecContext& ctxt, MCPoint& r_value);
	void SetBottomLeft(MCExecContext& ctxt, MCPoint value);
	void GetEffectiveBottomLeft(MCExecContext& ctxt, MCPoint& r_value);
	void SetEffectiveBottomLeft(MCExecContext& ctxt, MCPoint value);
	void GetBottomRight(MCExecContext& ctxt, MCPoint& r_value);
	void SetBottomRight(MCExecContext& ctxt, MCPoint value);
	void GetEffectiveBottomRight(MCExecContext& ctxt, MCPoint& r_value);
	void SetEffectiveBottomRight(MCExecContext& ctxt, MCPoint value);

	void GetWidth(MCExecContext& ctxt, uinteger_t& r_value);
	virtual void SetWidth(MCExecContext& ctxt, uinteger_t value);
	void GetEffectiveWidth(MCExecContext& ctxt, uinteger_t& r_value);
	virtual void SetEffectiveWidth(MCExecContext& ctxt, uinteger_t value);
	void GetHeight(MCExecContext& ctxt, uinteger_t& r_value);
	virtual void SetHeight(MCExecContext& ctxt, uinteger_t value);
	void GetEffectiveHeight(MCExecContext& ctxt, uinteger_t& r_value);
	virtual void SetEffectiveHeight(MCExecContext& ctxt, uinteger_t value);

	void GetRectangle(MCExecContext& ctxt, MCRectangle& r_rect);
	virtual void SetRectangle(MCExecContext& ctxt, MCRectangle p_rect);
	void GetEffectiveRectangle(MCExecContext& ctxt, MCRectangle& r_rect);
	virtual void SetEffectiveRectangle(MCExecContext& ctxt, MCRectangle p_rect);

	void GetEncoding(MCExecContext& ctxt, intenum_t& r_encoding);

    void GetCustomKeys(MCExecContext& ctxt, MCStringRef& r_string);
    void SetCustomKeys(MCExecContext& ctxt, MCStringRef p_string);
    void GetCustomProperties(MCExecContext& ctxt, MCValueRef &r_array);
    void SetCustomProperties(MCExecContext& ctxt, MCValueRef p_array);
    
    void GetTheme(MCExecContext& ctxt, intenum_t& r_theme);
    virtual void SetTheme(MCExecContext& ctxt, intenum_t  p_theme);
    void GetEffectiveTheme(MCExecContext& ctxt, intenum_t& r_theme);
    
    void GetThemeControlType(MCExecContext& ctxt, intenum_t& r_type);
    void SetThemeControlType(MCExecContext& ctxt, intenum_t  p_type);
    void GetEffectiveThemeControlType(MCExecContext& ctxt, intenum_t& r_type);
    
    void GetScriptStatus(MCExecContext& ctxt, intenum_t& r_status);
    
	////////// ARRAY PROPS
    
    void GetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool& r_element);
    void SetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool p_element);
    void GetCustomKeysElement(MCExecContext& ctxt, MCNameRef p_index, MCStringRef& r_string);
    void GetCustomPropertiesElement(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_array);
    void SetCustomKeysElement(MCExecContext& ctxt, MCNameRef p_index, MCStringRef p_string);
    void SetCustomPropertiesElement(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_array);
    
    ////////// REFLECTIVE PROPS
    
    void GetRevAvailableHandlers(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_handlers);
    void GetEffectiveRevAvailableHandlers(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_handlers);
    void GetRevAvailableVariables(MCExecContext& ctxt, MCNameRef p_key, MCStringRef& r_variables);
    void GetRevAvailableVariablesNonArray(MCExecContext& ctxt, MCStringRef& r_variables);
    void GetRevScriptDescription(MCExecContext& ctxt, MCValueRef& r_status);
    void GetEffectiveRevScriptDescription(MCExecContext& ctxt, MCValueRef& r_handlers);
    void GetRevBehaviorUses(MCExecContext& ctxt, MCArrayRef& r_objects);
    
//////////
			
    MCRectangle measuretext(const MCString& p_text, bool p_is_unicode);
    
    // Object pool instance variable manipulation
    MCDeletedObjectPool *getdeletedobjectpool(void) const { return m_pool; }
    void setdeletedobjectpool(MCDeletedObjectPool *pool) { m_pool = pool; }
	bool getdeletionissuspended(void) const { return m_deletion_is_suspended; }
	void setdeletionissuspended(bool value) { m_deletion_is_suspended = value; }

    // Because C++11 support is not universal amongst our supported platforms,
    // auto-conversion between MC???Handle types is not possible so there needs
    // to be an easy mechanism to get an object handle as a specific type.
    template <class T>
    typename MCObjectProxy<T>::Handle GetHandleAs()
    {
        // Checked cast to the requested type
        T* t_object = MCObjectCast<T> (this);
        return t_object->MCMixinObjectHandle<T>::GetHandle();
    }

protected:
    IO_stat defaultextendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);
	IO_stat defaultextendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_remaining);
	
	// MW-2013-12-05: [[ UnicodeFileFormat ]] These are the new propset pickle routines. If
	//   sfv < 7000 then the legacy ones are used; otherwise new ones are.
	IO_stat loadpropsets(IO_handle stream, uint32_t version);
	IO_stat savepropsets(IO_handle stream, uint32_t p_version);
	
	// MW-2012-02-16: [[ LogFonts ]] Load the font attrs with the given index.
	//   This method is protected as MCStack needs to call it to resolve its
	//   font attrs after the font table loads.
	void loadfontattrs(uint2 index);
	
	void setscript(MCStringRef);
	void setscript_cstring(const char *script)
	{
		MCAutoStringRef t_script;
		/* UNCHECKED */ MCStringCreateWithCString(script, &t_script);
		setscript(*t_script);
	}

    // FG-2014-11-11: [[ Better theming ]] Fetch the control type/state for theming purposes
    virtual MCPlatformControlType getcontroltype();
    virtual MCPlatformControlPart getcontrolsubpart();
    virtual MCPlatformControlState getcontrolstate();
    bool getthemeselectorsforprop(Properties, MCPlatformControlType&, MCPlatformControlPart&, MCPlatformControlState&, MCPlatformThemeProperty&, MCPlatformThemePropertyType&);
    
    // Returns the effective theme for this object
    MCInterfaceTheme gettheme() const;
    
private:
	bool clonepropsets(MCObjectPropertySet*& r_new_props) const;
	void deletepropsets(void);

	// Find the propset with the given name.
	bool findpropset(MCNameRef name, bool p_empty_is_default, MCObjectPropertySet*& r_set);
	// Find the propset with the given name, creating it if necessary.
	/* CAN FAIL */ bool ensurepropset(MCNameRef name, bool p_empty_is_default, MCObjectPropertySet*& r_set);
	
	// MW-2013-12-05: [[ UnicodeFileFormat ]] These are all the legacy propset pickle routines.
	//   If sfv >= 7000, then they are written out more directly.
	bool hasarraypropsets_legacy(void);
	uint32_t measurearraypropsets_legacy(void);
	IO_stat loadpropsets_legacy(IO_handle stream);
	IO_stat savepropsets_legacy(IO_handle stream);
	IO_stat loadunnamedpropset_legacy(IO_handle stream);
	IO_stat saveunnamedpropset_legacy(IO_handle stream);
	IO_stat loadarraypropsets_legacy(MCObjectInputStream& stream);
	IO_stat savearraypropsets_legacy(MCObjectOutputStream& stream);

	// MW-2012-02-16: [[ LogFonts ]] Copy the font attrs from the other object.
	void copyfontattrs(const MCObject& other);
	// MW-2012-02-16: [[ LogFonts ]] Clear the object's font attrs.
	void clearfontattrs(void);
	// MW-2012-02-17: [[ LogFonts ]] Fetch the index for the object's logical font.
	uint2 savefontattrs(void);
	// MW-2012-02-16: [[ LogFonts ]] Set the font attrs to the given values.
	void setfontattrs(uint32_t which, MCNameRef textfont, uint2 textsize, uint2 textstyle);

	// MW-2012-02-19: [[ SplitTextAttrs ]] Returns true if the object has any one of
	//   textFont/textSize/textStyle set.
	bool hasfontattrs(void) const;
	// MW-2012-02-19: [[ SplitTextAttrs ]] Returns true if the object needs to save
	//   the font-flags.
	bool needtosavefontflags(void) const;
    // Returns false if hasfontattrs() or a theme or theming type is set
    bool inheritfont() const;

	// MW-2013-03-06: [[ Bug 10695 ]] New method used by resolveimage* - if name is nil, then id search.
	MCImage *resolveimage(MCStringRef name, uint4 image_id);

	// MW-2012-02-14: [[ FontRefs ]] Called by open/close to map/unmap the concrete font.
	// MW-2013-08-23: [[ MeasureText ]] Made private as external uses of them can be
	//   done via measuretext() in a safe way.
	bool mapfont(bool recursive = false);
	void unmapfont(void);

	friend class MCObjectProxyBase;
	friend class MCEncryptedStack;
};

class MCObjectList : public MCDLlist
{
protected:
	MCObject *object;
	Boolean removed;
public:
	MCObjectList(MCObject *optr)
	{
		object = optr;
		removed = False;
	}
	MCObject *getobject()
	{
		return object;
	}
	Boolean getremoved()
	{
		return removed;
	}
	void setremoved(Boolean r)
	{
		removed = r;
	}
	MCObjectList *next()
	{
		return (MCObjectList *)MCDLlist::next();
	}
	MCObjectList *prev()
	{
		return (MCObjectList *)MCDLlist::prev();
	}
	void insertto(MCObjectList *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCObjectList *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	MCObjectList *remove(MCObjectList *&list)
	{
		return (MCObjectList *)MCDLlist::remove((MCDLlist *&)list);
	}
};


// Utility function for safe(-ish) casting from an MCObject to a derived class
// without using RTTI.
template <typename T>
inline T* MCObjectCast(MCObject* p_object)
{
    if (p_object == nil)
        return nil;
    
    // Check that the object's type matches the (static) type of the desired
    // type. This will break horribly if the desired type has derived types...
    MCAssert(p_object->gettype() == Chunk_term(T::kObjectType));
    return static_cast<T*> (p_object);
}

// Casting to an MCObject* is always safe
template <>
inline MCObject* MCObjectCast<MCObject>(MCObject* p_object)
{
    return p_object;
}

/* Helper class for locking an object for execution during a block.
 * Always allocate this on the stack, e.g.:
 *
 *    {
 *        MCObjectExecutionLock obj_lock {obj};
 *        // ... do stuff
 *    }
 */
class MCObjectExecutionLock
{
    MCObjectHandle m_handle;
public:
    MCObjectExecutionLock(const MCObjectHandle& obj) : m_handle(obj)
    {
        if (m_handle.IsValid())
            m_handle->lockforexecution();
    }

    MCObjectExecutionLock(MCObject* obj) : m_handle(obj)
    {
        if (m_handle.IsValid())
            m_handle->lockforexecution();
    }
    ~MCObjectExecutionLock()
    {
        if (m_handle.IsValid())
            m_handle->unlockforexecution();
    }
};

/* Object handle class that incorporates a part ID, allowing objects
 * to be disambiguated with respect to which card they are on.
 * Basically the same as an MCObjectPtr except for weakly referencing
 * its target object. */
class MCObjectPartHandle: public MCObjectHandle
{
    /* TODO[C++11] uint32_t m_part_id = 0; */
    uint32_t m_part_id;
public:
    /* TODO[2017-04-27] These constructors should be constexpr */
    /* TODO[C++11] constexpr MCObjectPartHandle() = default; */
    MCObjectPartHandle() : MCObjectHandle(), m_part_id(0) {}
    MCObjectPartHandle(decltype(nullptr))
      : MCObjectHandle(nullptr), m_part_id(0) {}

    /* TODO[C++11] MCObjectPartHandle(const MCObjectPartHandle&) = default; */
    MCObjectPartHandle(const MCObjectPartHandle& other)
      : MCObjectHandle(other), m_part_id(other.m_part_id) {}
    /* TODO[C++11] MCObjectPartHandle(MCObjectPartHandle&& other) = deafult; */
    MCObjectPartHandle(MCObjectPartHandle&& other)
      : MCObjectHandle(nullptr), m_part_id(0)
    {
        using std::swap;
        swap(*this, other);
    }

    MCObjectPartHandle(MCObject* p_object, uint32_t p_part_id = 0);
    MCObjectPartHandle(const MCObjectPtr&);

    /* TODO[C++11] MCObjectPartHandle& operator=(const MCObjectPartHandle&) = default; */
    MCObjectPartHandle& operator=(const MCObjectPartHandle& other)
    {
        MCObjectHandle::operator=(other);
        m_part_id = other.m_part_id;
        return *this;
    }
    /* TODO[C++11] MCObjectPartHandle& operator=(MCObjectPartHandle&&) = default; */
    MCObjectPartHandle& operator=(MCObjectPartHandle&& other)
    {
        MCObjectHandle::operator=(std::move(other));
        m_part_id = other.m_part_id;
        return *this;
    }

    MCObjectPartHandle& operator=(decltype(nullptr));
    MCObjectPartHandle& operator=(const MCObjectPtr&);

    uint32_t getPart() const { return m_part_id; }
    MCObjectPtr getObjectPtr() const;

    friend void swap(MCObjectPartHandle&, MCObjectPartHandle&);
};

#endif
