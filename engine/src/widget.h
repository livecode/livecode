#ifndef __MC_WIDGET__
#define __MC_WIDGET__

#ifndef __MC_CONTROL__
#include "control.h"
#endif


class MCWidget: public MCControl
{
public:
	MCWidget(void);
	MCWidget(const MCWidget& p_other);
	virtual ~MCWidget(void);

	virtual Chunk_term gettype(void) const;
	virtual const char *gettypestring(void);

	virtual const MCObjectPropertyTable *getpropertytable(void) const;

	virtual void open(void);
	virtual void close(void);

	virtual void kfocus(void);
	virtual void kunfocus(void);
	virtual Boolean kdown(MCStringRef p_key_string, KeySym p_key);
	virtual Boolean kup(MCStringRef p_key_string, KeySym p_key);

	virtual Boolean mdown(uint2 p_which);
	virtual Boolean mup(uint2 p_which, bool p_release);
	virtual Boolean mfocus(int2 p_x, int2 p_y);
	virtual void munfocus(void);

    virtual void mdrag(void);
    
	virtual Boolean doubledown(uint2 p_which);
	virtual Boolean doubleup(uint2 p_which);
	
    virtual MCObject* hittest(int32_t x, int32_t y);
    
	virtual void timer(MCNameRef p_message, MCParameter *p_parameters);

	virtual void setrect(const MCRectangle& p_rectangle);
	virtual void recompute(void);

	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);

	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	virtual IO_stat load(IO_handle stream, const char *version);

	virtual MCControl *clone(Boolean p_attach, Object_pos p_position, bool invisible);

	virtual void draw(MCDC *p_dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite);
	virtual Boolean maskrect(const MCRectangle& p_rect);
	
    virtual bool getprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue& r_value);
	virtual bool setprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue p_value);
    virtual bool getcustomprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCExecValue& r_value);
	virtual bool setcustomprop(MCExecContext& ctxt, MCNameRef set_name, MCNameRef prop_name, MCExecValue p_value);
    
    
    void SetKind(MCExecContext& ctxt, MCNameRef p_kind);
    void GetKind(MCExecContext& ctxt, MCNameRef& r_kind);
    
    ////////// Functions used by the event manager for event processing
    bool handlesMouseDown() const;
    bool handlesMouseUp() const;
    bool handlesMouseRelease() const;
    bool handlesKeyPress() const;
    bool handlesActionKeyPress() const;
    bool handlesTouches() const;
    
    ////////// Functions used by the event manager for gesture processing
    bool wantsClicks() const;       // Does this widget want click events?
    bool wantsTouches() const;      // Does this widget want touch events?
    bool wantsDoubleClicks() const; // Does this widget want double-clicks?
    bool waitForDoubleClick() const;// Don't send click straight away
    bool isDragSource() const;      // Widget is source for drag-drop operations
    
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
	
private:

    //////////
    ////////// Widget messages
    //////////
    friend class MCWidgetEventManager;

    ////////// Lifecycle events
    
    void OnOpen();
    void OnClose();
    void OnPaint(class MCPaintContext& p_context);
    void OnGeometryChanged(const MCRectangle& p_old_rect);
    void OnVisibilityChanged(bool p_visible);
    void OnHitTest(const MCRectangle& p_intersect, bool& r_inside);
    void OnBoundsTest(const MCRectangle& r_bounds_rect, bool& r_inside);
    void OnSave(class MCWidgetSerializer& p_stream);
    void OnLoad(class MCWidgetSerializer& p_stream);
    void OnCreate();
    void OnDestroy();
    void OnParentPropChanged();
    
    ////////// Basic mouse events
    
    void OnMouseEnter();
    void OnMouseLeave();
    void OnMouseMove(coord_t p_x, coord_t p_y);
    void OnMouseCancel(uinteger_t p_button);
    void OnMouseDown(coord_t p_x, coord_t p_y, uinteger_t p_button);
    void OnMouseUp(coord_t p_x, coord_t p_y, uinteger_t p_button);
    void OnMouseScroll(coord_t p_delta_x, coord_t p_delta_y);
    
    ////////// Derived mouse events
    
    void OnMouseStillDown(uinteger_t p_button, real32_t p_duration);
    void OnMouseHover(coord_t p_x, coord_t p_y, real32_t p_duration);
    void OnMouseStillHover(coord_t p_x, coord_t p_y, real32_t p_duration);
    void OnMouseCancelHover(real32_t p_duration);
    
    ////////// Touch events
    
    void OnTouchStart(uinteger_t p_id, coord_t p_x, coord_t p_y, real32_t p_pressure, real32_t p_radius);
    void OnTouchMove(uinteger_t p_id, coord_t p_x, coord_t p_y, real32_t p_pressure, real32_t p_radius);
    void OnTouchEnter(uinteger_t p_id);
    void OnTouchLeave(uinteger_t p_id);
    void OnTouchFinish(uinteger_t p_id, coord_t p_x, coord_t p_y);
    void OnTouchCancel(uinteger_t p_id);
    
    ////////// Keyboard events
    
    void OnFocusEnter();
    void OnFocusLeave();
    void OnKeyPress(MCStringRef p_keytext);
    void OnModifiersChanged(uinteger_t p_modifier_mask);
    void OnActionKeyPress(MCStringRef p_keyname);
    
    ////////// Raw input events
    
    void OnRawKeyDown(uinteger_t p_keycode);
    void OnRawKeyUp(uinteger_t p_keycode);
    void OnRawMouseDown(uinteger_t p_button);
    void OnRawMouseUp(uinteger_t p_button);
    
    ////////// Drag-drop events
    
    // These take a drag context because touch controls can cause multiple drag
    // operations to be in effect simultaneously.
    //
    // Note: widget script should also have a way of triggering a drag and drop
    // operation itself, if desired; e.g. startDrag(in pTouchID as integer)
    void OnDragEnter(bool& r_accept);
    void OnDragLeave();
    void OnDragMove(coord_t p_x, coord_t p_y);
    void OnDragDrop();
    void OnDragStart(bool& r_accept);
    void OnDragFinish();
    
    ////////// Gestures
    
    // Mouse may have moved during/after the click so position is important
    void OnClick(coord_t p_x, coord_t p_y, uinteger_t p_button, uinteger_t p_count);
    void OnDoubleClick(coord_t p_x, coord_t p_y, uinteger_t p_button);

    ////////// IME
    
    // ????
    
    //////////
	//////////
    //////////
	
	bool CallHandler(MCNameRef p_name, MCParameter *parameters);
	bool CallGetProp(MCNameRef p_property_name, MCNameRef p_key, MCValueRef& r_value);
	bool CallSetProp(MCNameRef p_property_name, MCNameRef p_key, MCValueRef value);
};

#endif
