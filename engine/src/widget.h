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

	virtual Boolean doubledown(uint2 p_which);
	virtual Boolean doubleup(uint2 p_which);
	
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
    
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
	
private:
	void OnOpen(void);
	void OnClose(void);
	
	void OnReshape(const MCRectangle& old_rect);
	
	void OnFocus(void);
	void OnUnfocus(void);
	
	void OnMouseEnter(void);
	void OnMouseMove(int32_t x, int32_t y);
	void OnMouseLeave(void);
	
	void OnMouseDown(uint32_t button);
	void OnMouseUp(uint32_t button);
	void OnMouseRelease(uint32_t button);
	
	bool OnKeyPress(MCStringRef key, uint32_t modifiers);
	
	bool OnHitTest(const MCRectangle& region);
	
	void OnPaint(void);

	//////////
	
	bool CallEvent(const char *name, MCParameter *parameters);
	bool CallGetProp(MCNameRef p_property_name, MCNameRef p_key, MCValueRef& r_value);
	bool CallSetProp(MCNameRef p_property_name, MCNameRef p_key, MCValueRef value);
	
	//////////
	
	int32_t m_modifier_state;
	int32_t m_button_state;
	bool m_mouse_over;
	int32_t m_mouse_x, m_mouse_y;
};

#endif
