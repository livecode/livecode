/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#ifndef DLLIST_H
#include "dllst.h"
#endif

#ifndef __MC_IMAGE_BITMAP_H__
#include "imagebitmap.h"
#endif

#include "globals.h"

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

enum MCVisitStyle
{
	VISIT_STYLE_DEPTH_FIRST,
	VISIT_STYLE_DEPTH_LAST
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
	virtual bool OnParagraph(MCParagraph *p_paragraph);
	virtual bool OnBlock(MCBlock *p_block);
	virtual bool OnStyledText(MCStyledText *p_styled_text);
};

#define OBJECT_EXTRA_ARRAYPROPS		(1U << 0)
#define OBJECT_EXTRA_PARENTSCRIPT	(1U << 1)
#define OBJECT_EXTRA_BITMAPEFFECTS  (1U << 2)
#define OBJECT_EXTRA_LAYERMODE		(1U << 3)
// MW-2012-02-19: [[ SplitTextAttrs ]] If this flag is set, then there is a font-flags
//   byte in the extended data section.
#define OBJECT_EXTRA_FONTFLAGS		(1U << 4)

class MCObjectHandle
{
public:
	MCObjectHandle(MCObject *p_object);
	~MCObjectHandle(void);

	// Increase the reference count.
	void Retain(void);

	// Decrease the reference count, destroying the object when it reaches 0.
	void Release(void);

	MCObject *Get(void);

	// Set the handle to nil - the object has been destroyed.
	void Clear(void);

	// Returns true if the object still exists.
	bool Exists(void);

private:
	uint32_t m_references;
	MCObject *m_object;
};

// MW-2012-02-17: [[ LogFonts ]] The font attrs struct for the object.
struct MCObjectFontAttrs
{
	MCNameRef name;
	uint2 style;
	uint2 size;
};

struct MCDeletedObjectPool;
void MCDeletedObjectsSetup(void);
void MCDeletedObjectsTeardown(void);
void MCDeletedObjectsEnterWait(bool p_dispatching);
void MCDeletedObjectsLeaveWait(bool p_dispatching);
void MCDeletedObjectsOnObjectCreated(MCObject *object);
void MCDeletedObjectsOnObjectDeleted(MCObject *object);
void MCDeletedObjectsOnObjectDestroyed(MCObject *object);

void MCDeletedObjectsDoDrain(void);

struct MCPatternInfo
{
	uint32_t id;
	MCPatternRef pattern;
};

class MCObject : public MCDLlist
{
protected:
	uint4 obj_id;
	MCObject *parent;
	MCNameRef _name;
	uint4 flags;
	MCRectangle rect;
	MCColor *colors;
	char **colornames;
	MCPatternInfo *patterns;
	char *script;
	MCHandlerlist *hlist;
	MCObjectPropertySet *props;
	uint4 state;
	uint2 fontheight;
	uint2 dflags;
	uint2 ncolors;
	uint2 npatterns;
	uint2 altid;
	uint1 hashandlers;
	uint1 scriptdepth;
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
	
	char *tooltip;
	
	// MW-2008-10-20: Pointer to the parent script's weak object reference.
	MCParentScriptUse *parent_script;

	// MW-2009-08-25: Pointer to the object's weak reference (if any).
	MCObjectHandle *m_weak_handle;

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
	
public:
	MCObject();
	MCObject(const MCObject &oref);
	virtual ~MCObject();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor *p_visitor);

	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	virtual IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

	virtual IO_stat load(IO_handle stream, const char *version);
	virtual IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	
	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean kup(const char *string, KeySym key);
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
	virtual void setrect(const MCRectangle &nrect);

	// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat getarrayprop(uint4 parid, Properties which, MCExecPoint &, MCNameRef key, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setarrayprop(uint4 parid, Properties which, MCExecPoint&, MCNameRef key, Boolean effective);

	virtual void select();
	virtual void deselect();
	virtual Boolean del();
	virtual void paste(void);
	virtual void undo(Ustruct *us);
	virtual void freeundo(Ustruct *us);
	virtual MCStack *getstack();

	// MW-2011-02-27: [[ Bug 9412 ]] If pass_from is non-nil it means the message is passing from that
    //   object and should continue to be passed if not handled. If it is nil, then the message should
    //   not be passed on.
	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);
	virtual void closemenu(Boolean kfocus, Boolean disarm);
	virtual void recompute();
	
	// MW-2011-09-20: [[ Collision ]] Compute the shape of the object's mask.
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	// MW-2012-02-14: [[ FontRefs ]] Remap any fonts this object uses as font related property has changed.
	virtual bool recomputefonts(MCFontRef parent_font);

	// MW-2012-02-14: [[ FontRefs ]] Returns the current concrete fontref of the object.
	MCFontRef getfontref(void) const { return m_font; }

	const MCRectangle &getrect() const;
    virtual MCRectangle getrectangle(bool p_effective) const;

	// MW-2012-06-08: [[ Relayer ]] Move a control within this container (if card or group) so that
	//   'source' is before 'target'.
	virtual void relayercontrol(MCControl *source, MCControl *target);
	virtual void relayercontrol_remove(MCControl *control);
	virtual void relayercontrol_insert(MCControl *control, MCControl *target);

	MCNameRef getdefaultpropsetname(void);

	Exec_stat sendgetprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);
	Exec_stat getcustomprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);
	Exec_stat sendsetprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);
	Exec_stat setcustomprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);

	Exec_stat changeid(uint32_t new_id);

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
		return MCNameIsEmpty(_name);
	}

	// Returns the name of the object.
	MCNameRef getname(void) const
	{
		return _name;
	}

	const char *getname_cstring(void) const
	{
		return MCNameGetCString(_name);
	}

	MCString getname_oldstring(void) const
	{
		return MCNameGetOldString(_name);
	}

	// Tests to see if the object has the given name, interpreting unnamed as
	// the empty string.
	bool hasname(MCNameRef p_other_name);

	// Set the object's name, interpreting the empty string as unnamed.
	void setname(MCNameRef new_name);
	void setname_cstring(const char *p_new_name);
	void setname_oldstring(const MCString& p_new_name);

	uint1 getopened() const
	{
		return opened;
	}
	uint1 gethashandlers() const
	{
		return hashandlers;
	}
	Boolean getflag(uint4 flag) const
	{
		return (flags & flag) != 0;
	}
	Boolean getextraflag(uint4 flag) const
	{
		return (extraflags & flag) != 0;
	}
	uint4 getflags(void) const
	{
		return flags;
	}
	char *getscript(void)
	{
		return script;
	}
	MCHandlerlist *gethandlers(void)
	{
		return hlist;
	}

	uint32_t getopacity(void) { return blendlevel * 255 / 100; }
	uint32_t getink(void) { return ink; }

	void setflag(uint4 on, uint4 flag);
	void setextraflag(uint4 on, uint4 flag);

	Boolean getstate(uint4 flag) const
	{
		return (state & flag) != 0;
	}
	void setstate(Boolean on, uint4 newstate);

	Boolean isdisabled() const
	{
		return (flags & F_DISABLED) != 0;
	}

	Exec_stat setsprop(Properties which, const MCString &);
	void help();

	Boolean getselected() const
	{
		return (state & CS_SELECTED) != 0;
	}

	bool isselectable(bool p_only_object = false) const;
	uint1 getborderwidth(void) {return borderwidth;}

	MCObject *getparent() const
	{
		return parent;
	}
	uint1 getscriptdepth() const
	{
		return scriptdepth;
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

	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// This method returns false if there was not enough memory to complete the
	// resolution.
	virtual bool resolveparentscript(void);
	
	// MW-2009-02-02: [[ Improved image search ]]
	// This method searches for the image with the given id, taking into account
	// the containment and behavior hierarchy of this object.
	MCImage *resolveimageid(uint4 image_id);
	MCImage *resolveimagename(const MCString& name);
	
	Boolean isvisible();
	Boolean resizeparent();
	Boolean getforecolor(uint2 di, Boolean reversed, Boolean hilite, MCColor &c,
	                     MCPatternRef &r_pattern, int2 &x, int2 &y, MCDC *dc, MCObject *o);
	void setforeground(MCDC *dc, uint2 di, Boolean rev, Boolean hilite = False);
	Boolean setcolor(uint2 index, const MCString &eptr);
	Boolean setcolors(const MCString &data);
	Boolean setpattern(uint2 newpixmap, const MCString &);
	Boolean setpatterns(const MCString &data);
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
	void setfontattrs(const char *textfont, uint2 textsize, uint2 textstyle);

	// MW-2012-02-17: [[ LogFonts ]] Fetch the (effective) font attrs for the
	//   object.
	void getfontattsnew(MCNameRef& fname, uint2& fsize, uint2& fstyle);
	void getfontattsnew(const char *& fname, uint2& fsize, uint2& fstyle);

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
	Exec_stat message_with_args(MCNameRef name, const MCString &v1);
	Exec_stat message_with_args(MCNameRef name, const MCString &v1, const MCString &v2);
	Exec_stat message_with_args(MCNameRef name, const MCString &v1, const MCString &v2, const MCString& v3);
	Exec_stat message_with_args(MCNameRef name, const MCString &v1, const MCString &v2, const MCString& v3, const MCString& v4);
	Exec_stat message_with_args(MCNameRef name, MCNameRef v1);
	Exec_stat message_with_args(MCNameRef name, MCNameRef v1, MCNameRef v2);
	Exec_stat message_with_args(MCNameRef name, int4 v1);
	Exec_stat message_with_args(MCNameRef name, int4 v1, int4 v2);
	Exec_stat message_with_args(MCNameRef name, int4 v1, int4 v2, int4 v3);
	Exec_stat message_with_args(MCNameRef name, int4 v1, int4 v2, int4 v3, int4 v4);
	
	void senderror();
	void sendmessage(Handler_type htype, MCNameRef mess, Boolean handled);
	Exec_stat names(Properties which, MCExecPoint &ep, uint4 parid);
	Boolean parsescript(Boolean report, Boolean force = False);
	void drawshadow(MCDC *dc, const MCRectangle &drect, int2 soffset);
	void draw3d(MCDC *dc, const MCRectangle &drect,
	            Etch style, uint2 bwidth);
	void drawborder(MCDC *dc, const MCRectangle &drect, uint2 bwidth);
	void positionrel(const MCRectangle &dptr, Object_pos xpos, Object_pos ypos);
	Exec_stat domess(const char *sptr);
	Exec_stat eval(const char *sptr, MCExecPoint &ep);
	// MERG 2013-9-13: [[ EditScriptChunk ]] Added at expression that's passed through as a second parameter to editScript
    void editscript(MCString p_at = NULL);
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

	// MW-2011-01-14: [[ Bug 9288 ]] Added 'parid' to make sure 'the properties of card id ...' returns
	//   the correct result.
	// MERG-2013-05-07: [[ RevisedPropsProp ]] Add 'effective' option to enable 'the effective
	//   properties of object ...'.
	Exec_stat getproparray(MCExecPoint &ep, uint4 parid, bool effective);

	MCObjectHandle *gethandle(void);

	// MW-2011-09-20: [[ Collision ]] Check to see if the two objects touch (exactly).
	bool intersects(MCObject *other, uint32_t threshold);
	
	// MW-2012-02-19: [[ SplitTextAttrs ]] Returns true if the object needs to save a
	//   font record. (Called by the font table visitor - so public).
	bool needtosavefontrecord(void) const;

	// MW-2012-03-04: [[ StackFile5500 ]] If 'include_2700' is set then both 2.7 and 5.5 versions
	//   of the objects will be included.
	static MCPickleContext *startpickling(bool include_2700);
	static void continuepickling(MCPickleContext *p_context, MCObject *p_object, uint4 p_part);
	static MCSharedString *stoppickling(MCPickleContext *p_context);

	static MCSharedString *pickle(MCObject *p_object, uint4 p_part);

	static MCObject *unpickle(MCSharedString *p_object, MCStack *p_stack);

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
	
	inline void signallisteners(Properties which)
	{
		if (m_listening && which != P_CUSTOM_PROPERTY_SET)
		{
			m_properties_changed |= kMCPropertyChangedMessageTypePropertyChanged;
			MCobjectpropertieschanged = True;
		}
	}
	
	inline void signallistenerswithmessage(uint8_t p_message)
	{
		if (m_listening)
		{
			m_properties_changed |= p_message;
			MCobjectpropertieschanged = True;
		}
	}	
	
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
	
	// MW-2012-10-10: [[ IdCache ]]
	void setinidcache(bool p_value)
	{
		m_in_id_cache = p_value;
	}
	
	bool getinidcache(void)
	{
		return m_in_id_cache;
	}
    
    void setisparentscript(bool p_value)
    {
        m_is_parent_script = p_value;
    }
    
    bool getisparentscript(void)
    {
        return m_is_parent_script;
    }

	// IM-2013-02-11 image change notification (used by button icons, field images, etc.)
	// returns true if the referenced image is still in use by this object
	virtual bool imagechanged(MCImage *p_image, bool p_deleting)
	{
		return false;
	}
    
    MCRectangle measuretext(const MCString& p_text, bool p_is_unicode);
    
    // Object pool instance variable manipulation
    MCDeletedObjectPool *getdeletedobjectpool(void) const { return m_pool; }
    void setdeletedobjectpool(MCDeletedObjectPool *pool) { m_pool = pool; }
    
protected:
	IO_stat defaultextendedsave(MCObjectOutputStream& p_stream, uint4 p_part);
	IO_stat defaultextendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_remaining);

	IO_stat loadpropsets(IO_handle stream);
	IO_stat savepropsets(IO_handle stream);

	// MW-2012-02-16: [[ LogFonts ]] Load the font attrs with the given index.
	//   This method is protected as MCStack needs to call it to resolve its
	//   font attrs after the font table loads.
	void loadfontattrs(uint2 index);
	
    // MW-2014-09-30: [[ ScriptStack ]] Used by MCStack::setasscriptonly.
	Exec_stat setscriptprop(MCExecPoint& ep);
    
private:
	Exec_stat getrectprop(Properties which, MCExecPoint& ep, Boolean effective);

	Exec_stat setrectprop(Properties which, MCExecPoint& ep, Boolean effective);
	Exec_stat setparentscriptprop(MCExecPoint& ep);
	Exec_stat setvisibleprop(uint4 parid, Properties which, MCExecPoint& ep);
	Exec_stat setshowfocusborderprop(MCExecPoint& ep);

	bool clonepropsets(MCObjectPropertySet*& r_new_props) const;
	void deletepropsets(void);

	// Find the propset with the given name.
	bool findpropset(MCNameRef name, bool p_empty_is_default, MCVariableValue*& r_value);
	// Find the propset with the given name, creating it if necessary.
	/* CAN FAIL */ bool ensurepropset(MCNameRef name, bool p_empty_is_default, MCVariableValue*& r_value);
	// Set propset to the one corresponding to name, creating it if it does
	// not exist.
	/* CAN FAIL */ bool setpropset(MCNameRef name);

	// List the available propsets into ep.
	void listpropsets(MCExecPoint& ep);
	// Change the available propsets to those listed in ep.
	/* CAN FAIL */ bool changepropsets(MCExecPoint& ep);

	bool hasarraypropsets(void);
	uint32_t measurearraypropsets(void);
	IO_stat loadunnamedpropset(IO_handle stream);
	IO_stat saveunnamedpropset(IO_handle stream);
	IO_stat loadarraypropsets(MCObjectInputStream& stream);
	IO_stat savearraypropsets(MCObjectOutputStream& stream);

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

	// MW-2013-03-06: [[ Bug 10695 ]] New method used by resolveimage* - if name is nil, then id search.
	MCImage *resolveimage(const MCString& name, uint4 image_id);
	
	// MW-2012-02-14: [[ FontRefs ]] Called by open/close to map/unmap the concrete font.
	// MW-2013-08-23: [[ MeasureText ]] Made private as external uses of them can be
	//   done via measuretext() in a safe way.
	void mapfont(void);
	void unmapfont(void);
	
	Exec_stat mode_getprop(uint4 parid, Properties which, MCExecPoint &, const MCString &carray, Boolean effective);

	friend class MCObjectHandle;
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
#endif
