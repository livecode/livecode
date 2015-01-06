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

#include "dllst.h"
#include "parsedef.h"
#include "objdefs.h"
#include "globals.h"
#include "imagebitmap.h"
#include "platform.h"

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
    virtual bool OnWidget(MCWidget *p_widget);
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
	MCStringRef *colornames;
	MCStringRef _script;
	MCPatternInfo *patterns;
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
	
	MCStringRef tooltip;
	
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
public:
	MCObject();
	MCObject(const MCObject &oref);
	virtual ~MCObject();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCObjectPropertyTable *getmodepropertytable(void) const { return &kModePropertyTable; }
	
	virtual bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor *p_visitor);

	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	virtual IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

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
	virtual void setrect(const MCRectangle &nrect);

	// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
#ifdef LEGACY_EXEC
	virtual Exec_stat getprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective, bool recursive = false);
	virtual Exec_stat getarrayprop_legacy(uint4 parid, Properties which, MCExecPoint &, MCNameRef key, Boolean effective);
	virtual Exec_stat setprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setarrayprop_legacy(uint4 parid, Properties which, MCExecPoint&, MCNameRef key, Boolean effective);

    // FG-2014-11-07: [[ Better theming ]] Gets a propery according to the native UI theme
    virtual Exec_stat getsystemthemeprop(Properties which, MCExecPoint&);
#endif

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
	
    // FG-2014-10-10: [[ Native Widgets ]] Informs the object that the selected tool has changed
    virtual void toolchanged(Tool p_new_tool);
    
    // FG-2014-10-14: [[ Native Widgets ]] Informs the object that its layer has changed
    virtual void layerchanged();
    
	// MW-2011-09-20: [[ Collision ]] Compute the shape of the object's mask.
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	// MW-2012-02-14: [[ FontRefs ]] Remap any fonts this object uses as font related property has changed.
	virtual bool recomputefonts(MCFontRef parent_font);

	// MW-2012-02-14: [[ FontRefs ]] Returns the current concrete fontref of the object.
	MCFontRef getfontref(void) const { return m_font; }

#ifdef LEGACY_EXEC
    Exec_stat getarrayprop(uint32_t p_part_id, Properties p_which, MCExecPoint& ep, MCNameRef p_index, Boolean p_effective);
    Exec_stat setarrayprop(uint32_t p_part_id, Properties p_which, MCExecPoint& ep, MCNameRef p_index, Boolean p_effective);
#endif
    
    Exec_stat sendgetprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCValueRef& r_value);
    Exec_stat sendsetprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCValueRef p_value);
    
    virtual bool getprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue& r_value);
	virtual bool setprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue p_value);
	virtual bool getcustomprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCExecValue& r_value);
	virtual bool setcustomprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCExecValue p_value);
    
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

	MCNameRef getdefaultpropsetname(void);

#ifdef LEGACY_EXEC
	Exec_stat sendgetprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);
	Exec_stat getcustomprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);

	Exec_stat sendsetprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);
	Exec_stat setcustomprop(MCExecPoint& ep, MCNameRef set_name, MCNameRef prop_name);
#endif
    
#ifdef OLD_EXEC
	Exec_stat setprops(uint32_t parid, MCExecPoint& ep);
	Exec_stat changeid(uint32_t new_id);
#endif

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
        char *t_name;
        /* UNCHECKED */ MCStringConvertToCString(MCNameGetString(_name), t_name);
		return t_name;
	}

    /*
	MCString getname_oldstring(void) const
	{
		return MCNameGetOldString(_name);
	}
    */

	// Tests to see if the object has the given name, interpreting unnamed as
	// the empty string.
	bool hasname(MCNameRef p_other_name);

	// Set the object's name, interpreting the empty string as unnamed.
	void setname(MCNameRef new_name);
	void setname_cstring(const char *p_new_name);

	uint1 getopened() const
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
	MCImage *resolveimagename(MCStringRef name);
	
	Boolean isvisible();
	Boolean resizeparent();
	Boolean getforecolor(uint2 di, Boolean reversed, Boolean hilite, MCColor &c,
	                     MCPatternRef &r_pattern, int2 &x, int2 &y, MCDC *dc, MCObject *o, bool selected = false);
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
#ifdef LEGACY_EXEC
	// Wrapper for 'names()' working in the old way (for convenience).
	Exec_stat names_old(Properties which, MCExecPoint& ep, uint32_t parid);
#endif

	Boolean parsescript(Boolean report, Boolean force = False);
	void drawshadow(MCDC *dc, const MCRectangle &drect, int2 soffset);
	void draw3d(MCDC *dc, const MCRectangle &drect,
	            Etch style, uint2 bwidth);
	void drawborder(MCDC *dc, const MCRectangle &drect, uint2 bwidth);
	void positionrel(const MCRectangle &dptr, Object_pos xpos, Object_pos ypos);
    
    // SN-2014-04-16 [[ Bug 12078 ]] Buttons and tooltip label are not drawn in the text direction
    void drawdirectionaltext(MCDC *dc, int2 sx, int2 sy, MCStringRef p_string, MCFontRef font);

	Exec_stat domess(MCStringRef sptr);
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

#ifdef OLD_EXEC
	// MW-2011-01-14: [[ Bug 9288 ]] Added 'parid' to make sure 'the properties of card id ...' returns
	//   the correct result.
	// MERG-2013-05-07: [[ RevisedPropsProp ]] Add 'effective' option to enable 'the effective
	//   properties of object ...'.
	Exec_stat getproparray(MCExecPoint &ep, uint4 parid, bool effective);
#endif

	MCObjectHandle *gethandle(void);

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

	void scheduledelete(void);
	
	// IM-2013-02-11 image change notification (used by button icons, field images, etc.)
	// returns true if the referenced image is still in use by this object
	virtual bool imagechanged(MCImage *p_image, bool p_deleting)
	{
		return false;
	}

	// MW-2012-10-10: [[ IdCache ]]
	void setinidcache(bool p_value)
	{
		m_in_id_cache = p_value;
	}
	
	bool getinidcache(void)
	{
		return m_in_id_cache;
	}
    
    MCRectangle measuretext(MCStringRef p_text, bool p_is_unicode);
    
    // MW-2014-12-17: [[ Widgets ]] Returns true if the object is a widget or contains
    //   a widget.
    bool haswidgets(void);
    
    // Currently non-functional: always returns false
    bool is_rtl() const { return false; }
    
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

	void SetVisibility(MCExecContext& ctxt, uint32_t part, bool flag, bool visible);
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
	void GetLongName(MCExecContext& ctxt, MCStringRef& r_long_name);
	void GetLongId(MCExecContext& ctxt, MCStringRef& r_long_id);
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
	void GetLongOwner(MCExecContext& ctxt, MCStringRef& r_owner);

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
    
	////////// ARRAY PROPS
    
    void GetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool& r_element);
    void SetTextStyleElement(MCExecContext& ctxt, MCNameRef p_index, bool p_element);
    void GetCustomKeysElement(MCExecContext& ctxt, MCNameRef p_index, MCStringRef& r_string);
    void GetCustomPropertiesElement(MCExecContext& ctxt, MCNameRef p_index, MCValueRef& r_array);
    void SetCustomKeysElement(MCExecContext& ctxt, MCNameRef p_index, MCStringRef p_string);
    void SetCustomPropertiesElement(MCExecContext& ctxt, MCNameRef p_index, MCValueRef p_array);
    
    ////////// MODE SPECIFIC PROPS
    
#ifdef MODE_DEVELOPMENT    
    void GetRevAvailableHandlers(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_handlers);
    void GetEffectiveRevAvailableHandlers(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_handlers);
    void GetRevAvailableVariables(MCExecContext& ctxt, MCNameRef p_key, MCStringRef& r_variables);
    void GetRevAvailableVariablesNonArray(MCExecContext& ctxt, MCStringRef& r_variables);
#endif
    
//////////
				
protected:
	IO_stat defaultextendedsave(MCObjectOutputStream& p_stream, uint4 p_part);
	IO_stat defaultextendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_remaining);
	
	// MW-2013-12-05: [[ UnicodeFileFormat ]] These are the new propset pickle routines. If
	//   sfv < 7000 then the legacy ones are used; otherwise new ones are.
	IO_stat loadpropsets(IO_handle stream, uint32_t version);
	IO_stat savepropsets(IO_handle stream);
	
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

#ifdef OLD_EXEC
    // MW-2014-09-30: [[ ScriptStack ]] Used by MCStack::setasscriptonly.
    Exec_stat setscriptprop(MCExecPoint& ep);
#endif

    // FG-2014-11-11: [[ Better theming ]] Fetch the control type/state for theming purposes
    virtual MCPlatformControlType getcontroltype();
    virtual MCPlatformControlPart getcontrolsubpart();
    virtual MCPlatformControlState getcontrolstate();
    bool getthemeselectorsforprop(Properties, MCPlatformControlType&, MCPlatformControlPart&, MCPlatformControlState&, MCPlatformThemeProperty&, MCPlatformThemePropertyType&);
    
private:
#ifdef OLD_EXEC
	Exec_stat setvisibleprop(uint4 parid, Properties which, MCExecPoint& ep);
	Exec_stat setrectprop(Properties which, MCExecPoint& ep, Boolean effective);
	Exec_stat getrectprop(Properties which, MCExecPoint& ep, Boolean effective);

	Exec_stat setparentscriptprop(MCExecPoint& ep);
	Exec_stat setshowfocusborderprop(MCExecPoint& ep);
#endif
	bool clonepropsets(MCObjectPropertySet*& r_new_props) const;
	void deletepropsets(void);

	// Find the propset with the given name.
	bool findpropset(MCNameRef name, bool p_empty_is_default, MCObjectPropertySet*& r_set);
	// Find the propset with the given name, creating it if necessary.
	/* CAN FAIL */ bool ensurepropset(MCNameRef name, bool p_empty_is_default, MCObjectPropertySet*& r_set);
#ifdef OLD_EXEC
	// Set propset to the one corresponding to name, creating it if it does not exist.
	/* CAN FAIL */ bool setpropset(MCNameRef name);

	// List the available propsets into ep.
	void listpropsets(MCExecPoint& ep);
	// Change the available propsets to those listed in ep.
	/* CAN FAIL */ bool changepropsets(MCExecPoint& ep);
#endif
	
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

	// MW-2013-03-06: [[ Bug 10695 ]] New method used by resolveimage* - if name is nil, then id search.
	MCImage *resolveimage(MCStringRef name, uint4 image_id);

#ifdef LEGACY_EXEC
	Exec_stat mode_getprop(uint4 parid, Properties which, MCExecPoint &, MCStringRef carray, Boolean effective);
#endif

	// MW-2012-02-14: [[ FontRefs ]] Called by open/close to map/unmap the concrete font.
	// MW-2013-08-23: [[ MeasureText ]] Made private as external uses of them can be
	//   done via measuretext() in a safe way.
	bool mapfont(bool recursive = false);
	void unmapfont(void);

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
