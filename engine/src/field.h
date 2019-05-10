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
// MCField class declarations
//
#ifndef	FIELD_H
#define	FIELD_H

#include "mccontrol.h"
#include "exec.h"
#include "exec-interface.h"

#define SCROLL_RATE 100
#define MAX_PASTE_MESSAGES 32


// Type used to index into fields, paragraphs, blocks, etc
typedef int32_t findex_t;

////////////////////////////////////////////////////////////////////////////////

// MW-2012-02-20: [[ FieldExport ]] Structure representing a (flattened) para-
//   graph style used by the export visitor.
struct MCFieldParagraphStyle
{
	bool has_text_align : 1;
	bool has_list_style : 1;
	bool has_first_indent : 1;
	bool has_left_indent : 1;
	bool has_right_indent : 1;
	bool has_space_above : 1;
	bool has_space_below : 1;
	bool has_tabs : 1;
	bool has_tab_alignments : 1;
	bool has_background_color : 1;
	bool has_border_width : 1;
	bool has_list_indent : 1;
	bool has_hgrid : 1;
	bool has_vgrid : 1;
	bool has_border_color : 1;
	bool has_dont_wrap : 1;
	bool has_padding : 1;
	bool has_metadata : 1;
	bool has_list_index : 1;
	
	unsigned text_align : 2;
	unsigned list_style : 4;
	unsigned list_depth : 4;
	bool vgrid : 1;
	bool hgrid : 1;
	bool dont_wrap : 1;
	// MW-2012-03-05: [[ HiddenText ]] Whether the paragraph is currently hidden.
	bool hidden : 1;
	
	uint8_t border_width;
	uint8_t padding;
	int16_t list_indent;
	int16_t first_indent;
	int16_t left_indent;
	int16_t right_indent;
	int16_t space_above;
	int16_t space_below;
	uint16_t tab_count;
	uint16_t *tabs;
	uindex_t tab_alignment_count;
	intenum_t *tab_alignments;
	uint32_t background_color;
	uint32_t border_color;
    MCStringRef metadata;
	uint16_t list_index;
};

// MW-2012-02-20: [[ FieldExport ]] Structure representing a (flattened) char-
//   acter style used by the export visitor.
struct MCFieldCharacterStyle
{
	bool has_text_color : 1;
	bool has_background_color : 1;
	bool has_link_text : 1;
	bool has_image_source : 1;
	bool has_metadata : 1;
	bool has_text_font : 1;
	bool has_text_style : 1;
	bool has_text_size : 1;
	bool has_text_shift : 1;
	
	uint32_t text_color;
	uint32_t background_color;
	MCStringRef link_text;
	MCStringRef image_source;
	MCStringRef metadata;
	MCNameRef text_font;
	uint2 text_style;
	uint2 text_size;
	int2 text_shift;
};

// MW-2012-02-20: [[ FieldExport ]] Flags describing what kind of export to
//   do.
typedef uint32_t MCFieldExportFlags;
enum
{
	// If set, the callback will be invoked for paragraphs
	kMCFieldExportParagraphs = 1 << 0,
	// If set, the callback will be invoked lines (only possible if the
	// field is open).
	kMCFieldExportLines = 1 << 1,
	// If set, the callback will be invoked for runs
	kMCFieldExportRuns = 1 << 2,
	// If set, the callback will be invoked with paragraph numbering information.
	kMCFieldExportNumbering = 1 << 3,
	// If set, the callback will be invoked with the style information for
	// paragraphs.
	kMCFieldExportParagraphStyles = 1 << 4,
	// If set, the callback will be invoked with the style information for
	// characters.
	kMCFieldExportCharacterStyles = 1 << 5,
	// If set, the callback will be invoked with the flattened style infor-
	// mation taking into account inheritence.
	kMCFieldExportFlattenStyles = 1 << 6,
};

enum MCInterfaceFieldCursorMovement
{
    kMCFieldCursorMovementDefault,
    kMCFieldCursorMovementVisual,
    kMCFieldCursorMovementLogical,
};

// MW-2012-02-20: [[ FieldExport ]] The event that occurred to cause the callback.
enum MCFieldExportEventType
{
	kMCFieldExportEventBeginParagraph,
	kMCFieldExportEventEndParagraph,
	kMCFieldExportEventLineBreak,
	kMCFieldExportEventNativeRun,
	kMCFieldExportEventUnicodeRun
};

// MW-2012-02-20: [[ FieldExport ]] The event data.
struct MCFieldExportEventData
{
	// The current paragraph style (if requested).
	bool has_paragraph_style;
	MCFieldParagraphStyle paragraph_style;
	// The current character style (if requested).
	bool has_character_style;
	MCFieldCharacterStyle character_style;
	// The bytes comprising the current run (if a run event).
	MCStringRef m_text;
	MCRange m_range;
	// Whether this event is occuring on the first or last paragraph (or both).
	bool is_first_paragraph, is_last_paragraph;
	// The number of the paragraph (if requested).
	uint32_t paragraph_number;
};

// MW-2012-02-20: [[ FieldExport ]] Callback invoked for each paragraph and run
//   in a field. A paragraph is indicated by 'bytes' being nil; otherwise it is
//   a run. The callback should return 'false' if it wants to terminate.
typedef bool (*MCFieldExportCallback)(void *context, MCFieldExportEventType event_type, const MCFieldExportEventData& event_data);

struct MCInterfaceFieldRanges;
struct MCInterfaceFieldRange;
// SN-2014-11-04: [[ Bug 13934 ]] Add forward declaration for the friends function of MCField
struct MCFieldLayoutSettings;

// Specifies how styling should be applied to replaced text.
enum MCFieldStylingMode
{
	// The new text will have no style.
	kMCFieldStylingNone,
	
	// The new text will take the style from the character before the start
	// of the insertion range.
	kMCFieldStylingFromBefore,
	
	// The new text will take the style from the character after the start
	// of the insertion range.
	kMCFieldStylingFromAfter,
};

////////////////////////////////////////////////////////////////////////////////

typedef MCObjectProxy<MCField>::Handle MCFieldHandle;

class MCField : public MCControl, public MCMixinObjectHandle<MCField>
{
public:
    
    enum { kObjectType = CT_FIELD };
    using MCMixinObjectHandle<MCField>::GetHandle;
    
private:
    
	friend class MCHcfield;
	MCCdata *fdata;
	MCCdata *oldfdata;
	MCCdata *scrolls;
	MCParagraph *paragraphs;
	MCParagraph *oldparagraphs;
	int4 textx;
	int4 texty;
	uint4 textheight;
	uint2 textwidth;
	int2 indent;
	uint2 fixeda;
	uint2 fixedd;
	uint2 fixedheight;
	findex_t foundlength;
	uint2 scrollbarwidth;
	uint2 ntabs;
	uint2 *tabs;
    uint2 nalignments;
    intenum_t *alignments;
	MCParagraph *curparagraph;
	int4 cury;
	MCParagraph *focusedparagraph;
	int4 focusedy;
	MCParagraph *firstparagraph;
	int4 firsty;
	MCParagraph *lastparagraph;
	findex_t foundoffset;
	MCScrollbar *vscrollbar;
	MCScrollbar *hscrollbar;
	MCStringRef label;
    MCTextDirection text_direction;
    MCInterfaceFieldCursorMovement cursor_movement;
    MCInterfaceKeyboardType keyboard_type : 4;
    MCInterfaceReturnKeyType return_key_type : 4;

    // MM-2014-08-11: [[ Bug 13149 ]] Used to flag if a recompute is required during the next draw.
    bool m_recompute : 1;
	
	static int2 clickx;
	static int2 clicky;
	static int2 goalx;

	static MCRectangle cursorrectp;
    static MCRectangle cursorrects;
	static Boolean cursoron;
	static MCField *cursorfield;

	static Boolean extend;
	static Boolean extendwords;
	static Boolean extendlines;
	static Boolean contiguous;
	static int2 narrowmargin;
	static int2 widemargin;
	static const Keytranslations emacs_keys[];
	static const Keytranslations std_keys[];
	static MCRectangle linkrect;
	static MCBlock *linkstart;
	static MCBlock *linkend;
	static findex_t linksi;
	static findex_t linkei;
	static findex_t composeoffset;
	static findex_t composelength;
	static Boolean composing;
	static findex_t composecursorindex;
	static findex_t composeconvertingsi;
	static findex_t composeconvertingei;

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
public:
    
    // SN-2014-11-04: [[ Bug 13934 ]] Refactor the laying out the field when setting properties
    friend MCParagraph* PrepareLayoutSettings(bool all, MCField *p_field, uint32_t p_part_id, findex_t &si, findex_t &ei, MCFieldLayoutSettings &r_layout_settings);
    // SN-2014-12-18: [[ Bug 14161 ]] Add a parameter to force the re-layout of a paragraph
    friend void LayoutParagraph(MCParagraph* p_paragraph, MCFieldLayoutSettings &x_layout_settings, bool p_force);
    friend void FinishLayout(MCFieldLayoutSettings &x_settings);

	MCField();
	MCField(const MCField &fref);
	// virtual functions from MCObject
	virtual ~MCField();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }

	virtual bool visit_self(MCObjectVisitor *p_visitor);
	virtual bool visit_children(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor* p_visitor);

	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual void kunfocus();
	virtual Boolean kdown(MCStringRef p_string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual void mdrag(void);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual void select();
	virtual uint2 gettransient() const;
	virtual void applyrect(const MCRectangle &nrect);

	virtual void undo(Ustruct *us);
	virtual void recompute();

	// MW-2012-02-14: [[ FontRefs ]] Method called to recompute concrete fonts in the
	//   field.
	virtual bool recomputefonts(MCFontRef parent_font, bool force);

	// virtual functions from MCControl
	virtual IO_stat load(IO_handle stream, uint32_t version);
	virtual IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	virtual IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);

	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
	
	virtual MCCdata *getdata(uint4 cardid, Boolean clone);
	virtual void replacedata(MCCdata *&data, uint4 newid);
	virtual void compactdata();
	virtual void resetfontindex(MCStack *oldstack);
	virtual Exec_stat hscroll(int4 offset, Boolean doredraw);
	virtual Exec_stat vscroll(int4 offset, Boolean doredraw);
	virtual void readscrollbars();
	virtual void setsbrects();
	virtual void resetscrollbars(Boolean move);
	virtual void getwidgetthemeinfo(MCWidgetInfo &widgetinfo);
	virtual void unlink(MCControl *p_control);

	// MCField functions in fieldf.cc
	static Field_translations trans_lookup(const Keytranslations table[], KeySym key, uint2 modifiers);
	static Field_translations lookup_mac_keybinding(KeySym key, uint32_t modifiers);
	
    void do_recompute(bool p_force_layout);
    
	void redrawcheck(MCRectangle &drect);
	void resetparagraphs();

	MCCdata *getcarddata(MCCdata *&list, uint4 parid, Boolean create);
	MCCdata* getcarddata(uint4 parid) {return getcarddata(fdata, parid, False);}
	
	void openparagraphs();
	void closeparagraphs(MCParagraph *pgptr);
	MCParagraph *getparagraphs(void) {return paragraphs;}
	void gettabs(uint2 *&t, uint2 &n, Boolean &fixed);
    void gettabaligns(intenum_t *&t, uint16_t &n);
	void getlisttabs(int32_t& r_first, int32_t& r_second);

	uint2 getfwidth() const;
	uint2 getfheight() const;
	MCRectangle getfrect() const;

	int32_t getlayoutwidth(void) const;
	int32_t getcontentx(void) const;
	int32_t getcontenty(void) const;
	int32_t gettexty(void) const;
	int32_t getfirstindent(void) const;
	int32_t getfixedheight(void) const { return fixedheight; }
    
    MCTextDirection gettextdirection() const { return text_direction; }

	bool getshowlines(void) const;

	void removecursor();
	void drawcursor(MCContext *context, const MCRectangle &drect);
	void positioncursor(Boolean force, Boolean goal, MCRectangle &drect, int4 y, bool primary);
	void replacecursor(Boolean force, Boolean goal);
	void dragtext();
	void computedrag();
	void drawrect(MCDC *dc, const MCRectangle &dirty);
	void draw3dhilite(MCDC *dc, const MCRectangle &trect);
	void setfocus(int2 x, int2 y);
	uint2 clearhilites();
	void reverse();
	void startselection(int2 x, int2 y, Boolean words);
	void extendselection(int2 x, int2 y);
	void endselection();
	void unselect(Boolean clear, Boolean internal);
	Boolean deleteselection(Boolean force);
	void centerfound();
	void clearfound();
	void updateparagraph(Boolean flow, Boolean redrawall, Boolean dodraw = True);
	void joinparagraphs();
	void fnop(Field_translations function, MCStringRef p_string, KeySym key);
	void finsertnew(Field_translations function, MCStringRef p_string, KeySym key);
	void fdel(Field_translations function, MCStringRef p_string, KeySym key);
	void fhelp(Field_translations function, MCStringRef p_string, KeySym key);
	void fundo(Field_translations function, MCStringRef p_string, KeySym key);
	void fcut(Field_translations function, MCStringRef p_string, KeySym key);
	void fcutline(Field_translations function, MCStringRef p_string, KeySym key);
	void fcopy(Field_translations function, MCStringRef p_string, KeySym key);
	void fpaste(Field_translations function, MCStringRef p_string, KeySym key);
	void ftab(Field_translations function, MCStringRef p_string, KeySym key);
	void ffocus(Field_translations function, MCStringRef p_string, KeySym key);
	void freturn(Field_translations function, MCStringRef p_string, KeySym key);
	void fcenter(Field_translations function, MCStringRef p_string, KeySym key);
	void fmove(Field_translations function, MCStringRef p_string, KeySym key);
	void fscroll(Field_translations function, MCStringRef p_string, KeySym key);
	void setupmenu(MCStringRef p_string, uint2 fheight, Boolean scrolling);
	void setupentry(MCButton *bptr, MCStringRef p_string);
	void typetext(MCStringRef newtext);
	void startcomposition();
	void stopcomposition(Boolean del, Boolean force);
	void setcompositioncursoroffset(findex_t coffset);
	void setcompositionconvertingrange(findex_t si, findex_t ei);
    bool getcompositionrange(findex_t& si, findex_t& ei);
	void deletecomposition();
	Boolean getcompositionrect(MCRectangle &r, findex_t offset);
	void syncfonttokeyboard();
	void verifyindex(MCParagraph *top, findex_t &si, bool p_is_end);
	
	MCParagraph *verifyindices(MCParagraph *top, findex_t& si, findex_t& ei);
	
	void insertparagraph(MCParagraph *newtext);
	// MCField selection functions in fields.cc
	Boolean find(MCExecContext &ctxt, uint4 cardid,
	             Find_mode mode, MCStringRef, Boolean first);
	Exec_stat sort(MCExecContext &ctxt, uint4 parid, Chunk_term type,
	               Sort_type dir, Sort_type form, MCExpression *by);
	// MW-2012-02-08: [[ Field Indices ]] The 'index' parameter, if non-nil, will contain
	//   the 1-based index of the returned paragraph (i.e. the one si resides in).
	MCParagraph *indextoparagraph(MCParagraph *top, findex_t &si, findex_t &ei, findex_t* index = nil);
	//void indextocharacter(int4 &findex_t);
	findex_t ytooffset(int4 y);
	int4 paragraphtoy(MCParagraph *target);
	findex_t getpgsize(MCParagraph *pgptr);

	// MW-2011-02-03: This method returns the 'correct' set of paragraphs for the field given
	//   the specified 'parid'.
	MCParagraph *resolveparagraphs(uint4 parid);

    void setparagraphs(MCParagraph *newpgptr, uint4 parid, bool p_preserv_zero_length_styles = false);
    void setparagraphs(MCParagraph *newpgptr, uint4 parid, findex_t p_start, findex_t p_end, bool p_preserv_zero_length_styles = false);
    // SN-2014-01-17: [[ Unicodification ]] Suppressed old string version of settext and settextindex
    Exec_stat settext(uint4 parid, MCStringRef p_text, Boolean p_formatted);
	
	// If 'preserve_first_style' is true, then the style of s will be the same as the style
	// immediately following si.
	Exec_stat settextindex(uint4 parid, findex_t si, findex_t ei, MCStringRef s, Boolean undoing, MCFieldStylingMode styling_mode = kMCFieldStylingFromBefore);
	
	void getlinkdata(MCRectangle &r, MCBlock *&sb, MCBlock *&eb);
    
    
	Exec_stat seltext(findex_t si, findex_t ei, Boolean focus, Boolean update = False);
	uint2 hilitedline();
    void hilitedlines(MCAutoArray<uint32_t>& r_lines);
	Exec_stat sethilitedlines(const uint32_t *p_lines, uint32_t p_line_count, Boolean forcescroll = True);
	void hiliteline(int2 x, int2 y);

	bool locchar(Boolean click, MCStringRef& r_string);
	bool loccharchunk(Boolean click, MCStringRef& r_string);
	bool locchunk(Boolean click, MCStringRef& r_string);
	bool locline(Boolean click, MCStringRef& r_string);
	bool loctext(Boolean click, MCStringRef& r_string);
	Boolean locmark(Boolean wholeline, Boolean wholeword,
	                Boolean click, Boolean chunk, Boolean inc_cr, findex_t &si, findex_t &ei);
    Boolean locmarkpoint(MCPoint p_location, Boolean wholeline, Boolean wholeword, Boolean chunk, Boolean inc_cr, findex_t &si, findex_t &ei);

	bool foundchunk(MCStringRef& r_string);
	bool foundline(MCStringRef& r_string);
	bool foundloc(MCStringRef& r_string);
	bool foundtext(MCStringRef& r_string);
	Boolean foundmark(Boolean wholeline, Boolean inc_cr, findex_t &si, findex_t &ei);

	bool selectedchunk(MCStringRef& r_string);
	bool selectedline(MCStringRef& r_string);
	bool selectedloc(MCStringRef& r_string);
	bool selectedtext(MCStringRef& r_string);
	Boolean selectedmark(Boolean wholeline, findex_t &si, findex_t &ei,
	                     Boolean force, bool p_char_indices = false);

	bool returnchunk(findex_t si, findex_t ei, MCStringRef& r_string, bool p_char_indices = false);
	bool returnline(findex_t si, findex_t ei, MCStringRef& r_string);
	bool returnloc(findex_t si, MCStringRef& r_string);
	bool returntext(findex_t si, findex_t ei, MCStringRef& r_string);

	void charstoparagraphs(findex_t si, findex_t ei, MCParagraph*& sp, MCParagraph*& ep, uint4& sl, uint4& el);
	void linestoparagraphs(findex_t si, findex_t ei, MCParagraph*& sp, MCParagraph*& ep);

	MCParagraph *cloneselection();
	bool pickleselection(MCDataRef& r_string);

	void cuttext();
	void copytext();
	void cuttextindex(uint4 parid, findex_t si, findex_t ei);
	void copytextindex(uint4 parid, findex_t si, findex_t ei);
	void pastetext(MCParagraph *newtext, Boolean dodel);
	void movetext(MCParagraph *p_new_text, findex_t p_index);
	void deletetext(findex_t si, findex_t ei);
	MCParagraph *clonetext(findex_t si, findex_t ei);

	Boolean isautoarm()
	{
		return (flags & F_F_AUTO_ARM) != 0;
	}

	// MCField HTML functions in fieldh.cc
	Exec_stat sethtml(uint4 parid, MCValueRef data);
	Exec_stat setrtf(uint4 parid, MCStringRef data);
	void setstyledtext(uint32_t part_id, MCArrayRef p_text);
	Exec_stat setpartialtext(uint4 parid, MCStringRef p_text);
#ifdef _MACOSX
	Exec_stat getparagraphmacunicodestyles(MCParagraph *p_start, MCParagraph *p_finish, MCDataRef& r_data);
	MCParagraph *macstyletexttoparagraphs(const MCString &textdata, const MCString &styledata, Boolean isunicode);
	MCParagraph *macunicodestyletexttoparagraphs(MCDataRef p_text, MCDataRef p_styles);
	static bool macmatchfontname(const char *p_font_name, char p_derived_font_name[]);
#endif

    MCParagraph *rtftoparagraphs(MCStringRef p_data);
	MCParagraph *styledtexttoparagraphs(MCArrayRef p_array);
	MCParagraph *texttoparagraphs(MCStringRef p_text);
	
    MCParagraph *parsestyledtextappendparagraph(MCArrayRef p_style, MCStringRef metadata, bool p_split, MCParagraph*& x_paragraphs);
	void parsestyledtextappendblock(MCParagraph *p_paragraph, MCArrayRef p_style, MCStringRef p_string, MCStringRef p_metadata);
	void parsestyledtextblockarray(MCArrayRef p_block_value, MCParagraph*& x_paragraphs);
	void parsestyledtextarray(MCArrayRef p_styled_text, bool p_paragraph_break, MCParagraph*& x_paragraphs);
	
	MCCdata *getcdata(void) {return fdata;}
	
	// MW-2012-01-27: [[ UnicodeChunks ]] Return the contents of the field in a native
	//   compatible way.
	//bool nativizetext(uint4 parid, MCExecPoint& ep, bool p_ascii_only);
	
	// MW-2012-02-08: [[ TextChanged ]] This causes a 'textChanged' message to be sent.
	void textchanged(void);
	
	// MW-2012-02-20: [[ FieldExport ]] Iterates over paragraphs and runs in the field between the
	//   start and end indices, invoking the callback based on the provided flags.
	bool doexport(MCFieldExportFlags flags, uint32_t p_part_id, int32_t start_index, int32_t end_index, MCFieldExportCallback callback, void *context);
	bool doexport(MCFieldExportFlags flags, MCParagraph *p_paragraphs, int32_t start_index, int32_t end_index, MCFieldExportCallback callback, void *context);
	// MW-2012-02-20: [[ FieldExport ]] Convert the content of the field to text, either as unicode
	//   or native encoding.
	bool exportastext(uint32_t p_part_id, int32_t start_index, int32_t finish_index, MCStringRef& r_string);

	// MW-2012-02-20: [[ FieldExport ]] Convert the content of the field to text, including any list
	//   indices. The output is encoded in either unicode or native.
	bool exportasplaintext(MCParagraph *p_paragraphs, int32_t p_start_index, int32_t p_finish_index, MCStringRef& r_string);
	bool exportasplaintext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, MCStringRef& r_string);

	// MW-2012-02-20: [[ FieldExport ]] Convert the content of the field to text, including any list
	//   indices and line breaks.
	bool exportasformattedtext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, MCStringRef& r_string);

	// MW-2012-02-20: [[ FieldExport ]] Convert the content of the field to rtf.
	bool exportasrtftext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, MCStringRef& r_string);
	bool exportasrtftext(MCParagraph *p_paragraphs, int32_t p_start_index, int32_t p_finish_index, MCStringRef& r_string);

	// MW-2012-02-20: [[ FieldExport ]] Convert the content of the field to (livecode) html.
	bool exportashtmltext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, bool p_effective, MCDataRef& r_text);
	bool exportashtmltext(MCParagraph *p_paragraphs, int32_t p_start_index, int32_t p_finish_index, bool p_effective, MCDataRef& r_text);

	// MW-2012-02-20: [[ FieldExport ]] Convert the content of the field to styled text arrays.
	bool exportasstyledtext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, bool p_formatted, bool p_effective, MCArrayRef &r_array);
    bool exportasstyledtext(MCParagraph* p_paragraphs, int32_t p_start_index, int32_t p_finish_index, bool p_formatted, bool p_effective, MCArrayRef &r_array);\

	// MW-2012-03-07: [[ FieldImport ]] Conver the htmlText string to a list of paragraphs.
    MCParagraph *importhtmltext(MCValueRef p_data);

	// MW-2012-03-05: [[ FieldImport ]] Add a paragraph with the given styling to the end of the supplied
	//   paragraphs list.
	bool importparagraph(MCParagraph*& x_paragraphs, const MCFieldParagraphStyle *style);
	// MW-2012-03-05: [[ FieldImport ]] Add a block with the given text and styling to the end of the given
	//   paragraph.
	bool importblock(MCParagraph *p_paragraph, const MCFieldCharacterStyle& style, const void *bytes, uint32_t byte_count, bool is_unicode);

	// MW-2012-03-14: [[ RtfParaStyles ]] Modified to take a paragraph style and block style. If block is nil
	//   then it marks a new paragraph (in which case paragraph can be nil to indicate no style).
	static bool converttoparagraphs(void *p_context, const MCTextParagraph *p_paragraph, const MCTextBlock *p_block);

	// MW-2012-02-11: [[ TabWidths ]] The 'which' parameter allows the parsing/formatting
	//   routine to do both stops and widths.
	static bool parsetabstops(Properties which, MCStringRef data, uint16_t*& r_tabs, uint16_t& r_tab_count);
	static void formattabstops(Properties which, uint16_t *tabs, uint16_t tab_count, MCStringRef& r_result);

	// IM-2016-09-22: [[ Bug 14645 ]] Convert tab alignments array to / from string
	static bool parsetabalignments(MCStringRef p_data, intenum_t *&r_alignments, uindex_t &r_alignment_count);
	static bool formattabalignments(const intenum_t *p_alignments, uindex_t p_alignment_count, MCStringRef &r_result);
	
	// MW-2012-02-22: [[ FieldChars ]] Count the number of characters (not bytes) between
	//   start and end in the given field.
	findex_t countchars(uint32_t parid, findex_t si, findex_t ei);
	// MW-2012-02-22: [[ FieldChars ]] Resolve the indices of the char range [start,start + count)
	//   within the given range.
	void resolvechars(uint32_t parid, findex_t& si, findex_t& ei, findex_t start, findex_t count);
	// MW-2012-02-22: [[ FieldChars ]] Convert the field indices to char indices.
	void unresolvechars(uint32_t parid, findex_t& si, findex_t& ei);
	
	// MW-2012-09-04: [[ Bug 9759 ]] This method adjusts the pixmap offsets so that patterns
	//   scroll with the text.
	// MW-2012-11-22: [[ Bug 9759 ]] 'dy' added to allow adjustment for hilite patterns in list
	//   behavior fields.
	void adjustpixmapoffset(MCDC *dc, uint2 index, int4 dy = 0);

	bool imagechanged(MCImage *p_image, bool p_deleting);
    
    MCRectangle firstRectForCharacterRange(int32_t& si, int32_t& ei);
    
    MCInterfaceKeyboardType getkeyboardtype() { return keyboard_type; }
    MCInterfaceReturnKeyType getreturnkeytype() { return return_key_type; }

    ////////// BIDIRECTIONAL SUPPORT
    
    MCTextDirection getbasetextdirection() { return text_direction; }
    bool IsCursorMovementVisual();

    ////////// PROPERTY SUPPORT METHODS

    void Relayout(bool reset, int4 xoffset, int4 yoffset);
	void Redraw(bool reset = false, int4 xoffset = 0, int4 yoffset = 0);
    void UpdateScrollbars(void);
    
    void DoSetInputControl(MCExecContext& ctxt, Properties which, bool setting);
    void DoSetTabStops(MCExecContext& ctxt, bool is_relative, uindex_t p_count, uinteger_t *p_tabs);
    
	////////// PROPERTY ACCESSORS

	void GetAutoTab(MCExecContext& ctxt, bool& r_flag);
	void SetAutoTab(MCExecContext& ctxt, bool flag);
	void GetDontSearch(MCExecContext& ctxt, bool& r_flag);
	void SetDontSearch(MCExecContext& ctxt, bool flag);
	void GetDontWrap(MCExecContext& ctxt, bool& r_flag);
	void SetDontWrap(MCExecContext& ctxt, bool flag);
	void GetFixedHeight(MCExecContext& ctxt, bool& r_flag);
	void SetFixedHeight(MCExecContext& ctxt, bool flag);
	void GetLockText(MCExecContext& ctxt, bool& r_flag);
	void SetLockText(MCExecContext& ctxt, bool flag);
    virtual void SetTraversalOn(MCExecContext& ctxt, bool setting);
	void GetSharedText(MCExecContext& ctxt, bool& r_flag);
	void SetSharedText(MCExecContext& ctxt, bool flag);
	void GetShowLines(MCExecContext& ctxt, bool& r_flag);
	void SetShowLines(MCExecContext& ctxt, bool flag);
	void GetHGrid(MCExecContext& ctxt, bool& r_flag);
	void SetHGrid(MCExecContext& ctxt, bool flag);
	void GetVGrid(MCExecContext& ctxt, bool& r_flag);
	void SetVGrid(MCExecContext& ctxt, bool flag);
	void GetStyle(MCExecContext& ctxt, intenum_t& r_style);
	void SetStyle(MCExecContext& ctxt, intenum_t p_style);
	void GetAutoHilite(MCExecContext& ctxt, bool& r_setting);
	void SetAutoHilite(MCExecContext& ctxt, bool setting);
	void GetAutoArm(MCExecContext& ctxt, bool& r_setting);
	void SetAutoArm(MCExecContext& ctxt, bool setting);
	void GetFirstIndent(MCExecContext& ctxt, integer_t& r_indent);
	void SetFirstIndent(MCExecContext& ctxt, integer_t indent);
	void GetWideMargins(MCExecContext& ctxt, bool& r_setting);
	void SetWideMargins(MCExecContext& ctxt, bool setting);
	void GetHScroll(MCExecContext& ctxt, integer_t& r_scroll);
	void SetHScroll(MCExecContext& ctxt, integer_t scroll);
	void GetVScroll(MCExecContext& ctxt, integer_t& r_scroll);
	void SetVScroll(MCExecContext& ctxt, integer_t scroll);
	void GetHScrollbar(MCExecContext& ctxt, bool& r_setting);
	void SetHScrollbar(MCExecContext& ctxt, bool setting);
	void GetVScrollbar(MCExecContext& ctxt, bool& r_setting);
	void SetVScrollbar(MCExecContext& ctxt, bool setting);
	void GetScrollbarWidth(MCExecContext& ctxt, uinteger_t& r_width);
	void SetScrollbarWidth(MCExecContext& ctxt, uinteger_t p_width);
	void GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width);
	void GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height);
	void GetListBehavior(MCExecContext& ctxt, bool& r_setting);
	void SetListBehavior(MCExecContext& ctxt, bool setting);
	void GetMultipleHilites(MCExecContext& ctxt, bool& r_setting);
	void SetMultipleHilites(MCExecContext& ctxt, bool setting);
	void GetNoncontiguousHilites(MCExecContext& ctxt, bool& r_setting);
	void SetNoncontiguousHilites(MCExecContext& ctxt, bool setting);
	void GetText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_text);
	void SetText(MCExecContext& ctxt, uint32_t part, MCStringRef p_text);
	void GetUnicodeText(MCExecContext& ctxt, uint32_t part, MCDataRef& r_text);
	void SetUnicodeText(MCExecContext& ctxt, uint32_t part, MCDataRef p_text);
	void GetHtmlText(MCExecContext& ctxt, uint32_t part, MCValueRef& r_text);
	void SetHtmlText(MCExecContext& ctxt, uint32_t part, MCValueRef p_text);
	void GetEffectiveHtmlText(MCExecContext& ctxt, uint32_t part, MCValueRef& r_text);
	void GetRtfText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_text);
	void SetRtfText(MCExecContext& ctxt, uint32_t part, MCStringRef p_text);
	void GetStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_array);
	void SetStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef p_array);
	void GetEffectiveStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_array);
	void GetFormattedStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_array);
	void GetEffectiveFormattedStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_array);
	void GetPlainText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_string);
	void GetUnicodePlainText(MCExecContext& ctxt, uint32_t part, MCDataRef& r_string);
	void GetFormattedText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_string);
	void SetFormattedText(MCExecContext& ctxt, uint32_t part, MCStringRef p_string);
	void GetUnicodeFormattedText(MCExecContext& ctxt, uint32_t part, MCDataRef& r_string);
	void SetUnicodeFormattedText(MCExecContext& ctxt, uint32_t part, MCDataRef p_string);
	void GetLabel(MCExecContext& ctxt, MCStringRef& r_string);
	void SetLabel(MCExecContext& ctxt, MCStringRef p_string);
	void GetToggleHilite(MCExecContext& ctxt, bool& r_setting);
	void SetToggleHilite(MCExecContext& ctxt, bool setting);
	void GetThreeDHilite(MCExecContext& ctxt, bool& r_setting);
	void SetThreeDHilite(MCExecContext& ctxt, bool setting);
	void GetEncoding(MCExecContext& ctxt, uint32_t part, intenum_t& r_encoding);
    void SetCursorMovement(MCExecContext&, intenum_t);
    void GetCursorMovement(MCExecContext&, intenum_t&);
    void SetTextDirection(MCExecContext&, intenum_t);
    void GetTextDirection(MCExecContext&, intenum_t&);
    
    void SetTextAlign(MCExecContext&, intenum_t*);
    void GetTextAlign(MCExecContext&, intenum_t*&);
    void GetEffectiveTextAlign(MCExecContext& ctxt, intenum_t& r_align);
    
    void GetHilitedLines(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_lines);
    void SetHilitedLines(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_lines);
    void GetFlaggedRanges(MCExecContext& ctxt, uint32_t p_part, MCInterfaceFieldRanges& r_ranges);
    void SetFlaggedRanges(MCExecContext& ctxt, uint32_t p_part, const MCInterfaceFieldRanges& p_ranges); 
    void SetTabStops(MCExecContext& ctxt, uindex_t p_count, uinteger_t *p_tabs);
    void GetTabStops(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tabs);
    void SetTabWidths(MCExecContext& ctxt, uindex_t p_count, uinteger_t *p_tabs);
    void GetTabWidths(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tabs);
    void SetTabAlignments(MCExecContext& ctxt, const MCInterfaceFieldTabAlignments &t_alignments);
    void GetTabAlignments(MCExecContext& ctxt, MCInterfaceFieldTabAlignments &r_alignments);
    void GetPageHeights(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_heights);
    void GetPageRanges(MCExecContext& ctxt, MCInterfaceFieldRanges& r_ranges);
    
    void GetKeyboardType(MCExecContext& ctxt, intenum_t& r_type);
    void SetKeyboardType(MCExecContext& ctxt, intenum_t p_type);
    void GetReturnKeyType(MCExecContext& ctxt, intenum_t& r_type);
    void SetReturnKeyType(MCExecContext& ctxt, intenum_t p_type);
    
    virtual void SetShadow(MCExecContext& ctxt, const MCInterfaceShadow& p_shadow);
    virtual void SetShowBorder(MCExecContext& ctxt, bool setting);
    virtual void SetTextHeight(MCExecContext& ctxt, uinteger_t* height);
    virtual void SetTextFont(MCExecContext& ctxt, MCStringRef font);
    virtual void SetTextSize(MCExecContext& ctxt, uinteger_t* size);
    virtual void SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style);
    virtual void SetBorderWidth(MCExecContext& ctxt, uinteger_t width);
    virtual void Set3D(MCExecContext& ctxt, bool setting);
    virtual void SetOpaque(MCExecContext& ctxt, bool setting);
    virtual void SetEnabled(MCExecContext& ctxt, uint32_t part, bool setting);
	virtual void SetDisabled(MCExecContext& ctxt, uint32_t part, bool setting);
    
	virtual void SetLeftMargin(MCExecContext& ctxt, integer_t p_margin);
	virtual void SetRightMargin(MCExecContext& ctxt, integer_t p_margin);
	virtual void SetTopMargin(MCExecContext& ctxt, integer_t p_margin);
	virtual void SetBottomMargin(MCExecContext& ctxt, integer_t p_margin);
    virtual void SetMargins(MCExecContext& ctxt, const MCInterfaceMargins& p_margins);
	virtual void SetWidth(MCExecContext& ctxt, uinteger_t value);
	virtual void SetHeight(MCExecContext& ctxt, uinteger_t value);
    virtual void SetEffectiveWidth(MCExecContext& ctxt, uinteger_t value);
	virtual void SetEffectiveHeight(MCExecContext& ctxt, uinteger_t value);
    virtual void SetRectangle(MCExecContext& ctxt, MCRectangle p_rect);
    virtual void SetEffectiveRectangle(MCExecContext& ctxt, MCRectangle p_rect);

    //////////
	
    void GetTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value);
    void SetTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef value);
    void GetUnicodeTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCDataRef& r_value);
    void SetUnicodeTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCDataRef r_value);
    void GetPlainTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value);
    void GetUnicodePlainTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCDataRef& r_value);
    void GetFormattedTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value);
    void GetUnicodeFormattedTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCDataRef& r_value);
    void GetRtfTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef& r_value);
    void SetRtfTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCStringRef value);
    void GetHtmlTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCValueRef& r_value);
    void GetEffectiveHtmlTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCValueRef& r_value);
    void SetHtmlTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCValueRef value);
    void GetStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef& r_value);
    void GetEffectiveStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef& r_value);
    void SetStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef value);
    void GetFormattedStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef& r_value);
    void GetEffectiveFormattedStyledTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t p_start, int32_t p_finish, MCArrayRef& r_value);
	
    void GetCharIndexOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t& r_value);
    void GetLineIndexOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t& r_value);
    void GetFormattedTopOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t& r_value);
    void GetFormattedLeftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t& r_value);
    void GetFormattedWidthOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t& r_value);
    void GetFormattedHeightOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t& r_value);
    void GetFormattedRectOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCRectangle32& r_value);
	
    void GetLinkTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value);
	void SetLinkTextOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value);
    void GetMetadataOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value);
	void SetMetadataOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value);
    void GetMetadataOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value);
	void SetMetadataOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value);
    void GetImageSourceOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef& r_value);
	void SetImageSourceOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef value);
    void GetVisitedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_value);
	void SetVisitedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool p_value);
	void GetEncodingOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, intenum_t& r_encoding);
    void GetFlaggedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value);
    void SetFlaggedOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool value);
    void GetFlaggedRangesOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCInterfaceFieldRanges& r_value);
    void SetFlaggedRangesOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceFieldRanges& value);
	void GetListStyleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t& r_value);
    void SetListStyleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, intenum_t value);
	void GetListDepthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value);
	void SetListDepthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *value);
	void GetListIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value);
	void SetListIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t *value);
	void GetListIndexOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value);
	void GetEffectiveListIndexOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value);
	void SetListIndexOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *value);
	
	void GetFirstIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value);
	void GetEffectiveFirstIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t& r_value);
    void SetFirstIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t *p_indent);
	void GetLeftIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value);
	void GetEffectiveLeftIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t& r_value);
    void SetLeftIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t *p_indent);
	void GetRightIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value);
	void GetEffectiveRightIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t& r_value);
    void SetRightIndentOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t *p_indent);
    void GetTextAlignOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t*& r_value);
    void GetEffectiveTextAlignOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, intenum_t& r_value);
    void SetTextAlignOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, intenum_t* value);
	void GetSpaceAboveOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value);
	void GetEffectiveSpaceAboveOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value);
    void SetSpaceAboveOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *p_space);
	void GetSpaceBelowOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value);
	void GetEffectiveSpaceBelowOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value);
    void SetSpaceBelowOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *p_space);
    
	void GetTabStopsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values);
	void SetTabStopsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uindex_t count, uinteger_t *values);
	void GetEffectiveTabStopsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values);
	void GetTabWidthsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values);
	void SetTabWidthsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uindex_t count, uinteger_t *values);
	void GetEffectiveTabWidthsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uindex_t& r_count, uinteger_t*& r_values);
    void GetTabAlignmentsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceFieldTabAlignments& r_values);
    void SetTabAlignmentsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceFieldTabAlignments& r_values);
    void GetEffectiveTabAlignmentsOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceFieldTabAlignments& r_values);
	
	void GetBorderWidthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value);
    void SetBorderWidthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *p_width);
	void GetEffectiveBorderWidthOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value);
	
	void GetBackColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color);
    void SetBackColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& p_color);
	void GetEffectiveBackColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color);
	void GetBorderColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color);
	void SetBorderColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& color);
	void GetEffectiveBorderColorOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color);
	
	void GetHGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool*& r_value);
    void SetHGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool *p_has_hgrid);
    void GetEffectiveHGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value);
	void GetVGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool*& r_value);
    void SetVGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool *p_has_vgrid);
	void GetEffectiveVGridOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& value);
	void GetDontWrapOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool*& r_value);
    void SetDontWrapOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool *p_has_dont_wrap);
	void GetEffectiveDontWrapOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& value);
	
	void GetPaddingOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value);
    void SetPaddingOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t *p_padding);
	void GetEffectivePaddingOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value);

	void GetInvisibleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value);
    void SetInvisibleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool p_invisible);
    
    void GetVisibleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value);
    void SetVisibleOfLineChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool p_invisible);
	
	void GetForeColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color);
	void SetForeColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& color);
	void GetEffectiveForeColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color);
	void GetBackColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color);
	void SetBackColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceNamedColor& color);
	void GetEffectiveBackColorOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceNamedColor& r_color);
	
	void GetTextFontOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCStringRef& r_value);
    void SetTextFontOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, MCStringRef p_value);
    void GetEffectiveTextFontOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCStringRef& r_value);
	void GetTextStyleOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceTextStyle& r_value);
    void SetTextStyleOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, const MCInterfaceTextStyle& p_value);
    void GetEffectiveTextStyleOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, MCInterfaceTextStyle& r_value);
	void GetTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t*& r_value);
    void SetTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, uinteger_t* p_value);
    void GetEffectiveTextSizeOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, uinteger_t& r_value);
	void GetTextShiftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t*& r_value);
    void SetTextShiftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, integer_t* p_value);
    void GetEffectiveTextShiftOfCharChunk(MCExecContext& ctxt, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, integer_t& r_value);
    
    void GetTextStyleElementOfCharChunk(MCExecContext& ctxt, MCNameRef p_index, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool*& r_value);
    void GetEffectiveTextStyleElementOfCharChunk(MCExecContext& ctxt, MCNameRef p_index, uint32_t p_part_id, int32_t si, int32_t ei, bool& r_mixed, bool& r_value);
    void SetTextStyleElementOfCharChunk(MCExecContext& ctxt, MCNameRef p_index, uint32_t p_part_id, int32_t si, int32_t ei, bool *p_value);

    // Invalidates the given rect of this field *unless* this field is the entry
    // box in a combo-box. Because the text is vertically-centred in those
    // fields, the y offsets calculated during dirty calculations are wrong and
    // the rect cannot be trusted so a full-field invalidation is needed.
    //
    // This override is non-virtual as the method is never called in a dynamic
    // context.
    void layer_redrawrect(const MCRectangle& m_dirty_rect);
    
protected:
    
    // FG-2014-11-11: [[ Better theming ]] Fetch the control type/state for theming purposes
    virtual MCPlatformControlType getcontroltype();
    virtual MCPlatformControlPart getcontrolsubpart();
    virtual MCPlatformControlState getcontrolstate();
};
#endif
