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
// utility functions
//

#ifndef UTIL_H
#define UTIL_H

#include "parsedef.h"
#include "sysdefs.h"
#include "typedefs.h"
#include "mcsemaphore.h"

typedef struct
{
	uint2 lockscreen;
	MCSemaphore errorlock;
	Boolean watchcursor;
	Boolean lockerrors;
	Boolean lockmessages;
	Boolean lockmoves;
	Boolean lockrecent;
	Boolean lockmenus;
	Boolean interrupt;
	uint2 dragspeed;
	MCCard *dynamiccard;
	MCObject *errorptr;
	MCObject *errorlockptr;
	Boolean exitall;
}
MCSaveprops;

struct MCSortnode
{
	union
	{
		MCStringRef svalue;
		MCNumberRef nvalue;
        MCDataRef dvalue;
	};
	
	const void *data;
	
	MCSortnode()
	: svalue(nil), data(nullptr) {}
	
	~MCSortnode()
	{
		if (svalue != nil)
			MCValueRelease(svalue);
	}
	
	MCSortnode& operator= (const MCSortnode& s)
	{
		MCValueAssign(svalue, s.svalue);
		data = s.data;
		return *this;
	}
};

extern void MCU_play();
extern void MCU_play_stop();
extern void MCU_init();
extern void MCU_watchcursor(MCStack *sptr, Boolean force);
extern void MCU_unwatchcursor(MCStack *sptr, Boolean force);
extern void MCU_resetprops(Boolean update);
extern void MCU_saveprops(MCSaveprops &sp);
extern void MCU_restoreprops(MCSaveprops &sp);
extern int4 MCU_any(int4 max);
extern bool MCU_getnumberformat(uint2 fw, uint2 trail, uint2 force, MCStringRef& r_string);
extern void MCU_setnumberformat(MCStringRef p_input, uint2 &fw, uint2 &trailing, uint2 &force);
extern real8 MCU_stoIEEE(const char *bytes);
extern real8 MCU_i4tor8(int4 in);
extern real8 MCU_fwrap(real8 p_x, real8 p_y);
extern int4 MCU_r8toi4(real8 in);
extern void MCU_srand();
extern real8 MCU_drand();
extern Boolean MCU_comparechar(const char *sptr, char target,
	                               Boolean isunicode = False);
extern Boolean MCU_strchr(const char *&, uint4 &, char,
	                          Boolean isunicode = False);
inline uint1 MCU_charsize(Boolean isunicode = False);
extern char *MCU_strtok(char *, const char *);
/* WRAPPER */ extern bool MCU_strtol(MCStringRef p_string, int4& r_l);
extern int4 MCU_strtol(const char *&, uint4 &, int1, Boolean &done,
	                       Boolean reals = False, Boolean octals = False);
extern real8 MCU_strtor8(const char *&, uint4 &, int1, Boolean &r_done,
						Boolean convertoctals = False);
extern void MCU_strip(char *sptr, uint2 trailing, uint2 force);
extern uint4 MCU_r8tos(char *&sptr, uint4 &s, real8 n,uint2 fw, uint2 trailing, uint2 force);
extern bool MCU_r8tos(real8 n, uint2 fw, uint2 trailing, uint2 force, MCStringRef &r_string);
extern bool MCU_stor8(MCStringRef, real8& r_d, bool co = false);
extern Boolean MCU_stor8(const MCString&, real8& d, Boolean co = False);
extern bool MCU_stoi2(MCStringRef, int2 &r_d);
extern Boolean MCU_stoi2(const MCString&, int2 &d);
extern bool MCU_stoui2(MCStringRef p_string, uint2 &r_d);
extern Boolean MCU_stoui2(const MCString&, uint2 &d);
extern bool MCU_stoi2x2(MCStringRef p_string, int16_t& r_d1, int16_t& r_d2);
extern Boolean MCU_stoi2x2(const MCString&, int2 &d1, int2 &d2);
extern bool MCU_stoi2x4(MCStringRef p_string, int16_t& r_d1, int16_t& r_d2, int16_t& r_d3, int16_t& r_d4);
extern Boolean MCU_stoi2x4(const MCString&, int2 &d1, int2 &d2, int2 &d3, int2 &d4);
extern bool MCU_stoi4x4(MCStringRef p_string, int32_t& r_d1, int32_t& r_d2, int32_t& r_d3, int32_t& r_d4);
extern Boolean MCU_stoi4x4(const MCString&, int32_t &d1, int32_t &d2, int32_t &d3, int32_t &d4);
extern Boolean MCU_stoi4x2(const MCString &s, int32_t &d1, int32_t &d2);
extern Boolean MCU_stobxb(const MCString& p_string, Boolean &r_left, Boolean& r_right);
extern bool MCU_stoi4(MCStringRef p_string, int4& r_d);
extern Boolean MCU_stoi4(const MCString&, int4& d);
extern bool MCU_stoui4(MCStringRef p_string, uint4 &r_d);
extern Boolean MCU_stoui4(const MCString&, uint4& d);
extern bool MCU_stoui4x2(MCStringRef p_string, uint4 &r_d1, uint4 &r_d2);
extern bool MCU_stob(MCStringRef p_string, bool& r_condition);
extern Boolean MCU_stob(const MCString&, Boolean& condition);
extern void MCU_lower(char *sptr, const MCString& s);
extern int4 MCU_strncasecmp(const char *one, const char *two, size_t n);
extern Boolean MCU_offset(const MCString &p, const MCString &w,
	                          uint4 &offset, Boolean casesensitive = False);
extern void MCU_additem(char *&dptr, const char *sptr, Boolean first);
extern void MCU_addline(char *&dptr, const char *sptr, Boolean first);
extern void MCU_break_string(const MCString &s, MCString *&ptrs, uint2 &nptrs,
	                             Boolean isunicode = False);

#ifndef _DEBUG_MEMORY
extern void MCU_realloc(char **data, uint4 osize, uint4 nsize, uint4 csize);
#endif
extern bool MCU_matchname(MCNameRef p_name, Chunk_term type, MCNameRef name);
extern void MCU_snap(int2 &p);

// MDW-2014-07-06: [[ oval_points ]]
extern bool MCU_roundrect(MCPoint *&r_points, uindex_t &r_point_count,
                   const MCRectangle &rect, uint2 radius, uint2 startAngle, uint2 arcAngle, uint2 flags);
extern Boolean MCU_parsepoints(MCPoint *&oldpoints, uindex_t &n, MCStringRef p_data);
extern Boolean MCU_parsepoint(MCPoint &r_point, MCStringRef);
extern void MCU_querymouse(int2 &x, int2 &y);
extern void MCU_resetcursors();
extern void MCU_set_rect(MCRectangle &rect, int2 x, int2 y, uint2 w, uint2 h);
extern void MCU_set_rect(MCRectangle32 &rect, int32_t x, int32_t y, int32_t w, int32_t h);
extern Boolean MCU_point_in_rect(const MCRectangle &srect, int2 x, int2 y);
extern Boolean MCU_rect_in_rect(const MCRectangle &p, const MCRectangle &w);
extern bool MCU_line_intersect_rect(const MCRectangle& srect, const MCRectangle& line);
extern Boolean MCU_point_on_line(MCPoint *points, uint2 npoints,
	                                 int2 x, int2 y, uint2 linesize);
extern Boolean MCU_point_in_polygon(MCPoint *points, uint2 npoints,
	                                    int2 x, int2 y);
extern void MCU_offset_points(MCPoint *points, uint2 npoints,
	                              int2 xoff, int2 yoff);

extern MCRectangle MCU_compute_rect(int2 x1, int2 y1, int2 x2, int2 y2);
extern MCRectangle MCU_center_rect(const MCRectangle &, const MCRectangle &);
extern MCRectangle MCU_bound_rect(const MCRectangle &, int2, int2, uint2, uint2);
extern MCRectangle MCU_clip_rect(const MCRectangle &, int2, int2, uint2, uint2);
extern MCRectangle MCU_intersect_rect(const MCRectangle &one, const MCRectangle &two);
extern MCRectangle MCU_union_rect(const MCRectangle &one, const MCRectangle &two);
extern MCRectangle MCU_subtract_rect(const MCRectangle &one, const MCRectangle &two);
extern MCRectangle MCU_reduce_rect(const MCRectangle &rect, int2 amount);
extern MCRectangle MCU_scale_rect(const MCRectangle &rect, int2 factor);
extern MCRectangle MCU_offset_rect(const MCRectangle& r, int2 ox, int2 oy);

extern MCRectangle MCU_recttoroot(MCStack *sptr, const MCRectangle &o);
extern void MCU_getshift(uint4 mask, uint2 &shift, uint2 &outmask);
extern void MCU_choose_tool(MCExecContext& ctxt, MCStringRef p_string, Tool p_tool);
extern Exec_stat MCU_dofrontscripts(Handler_type htype, MCNameRef message, MCParameter *params);
//extern bool MCU_path2std(MCStringRef p_path, MCStringRef& r_std_path);
//extern void MCU_path2std(char *dptr);
//extern bool MCU_path2native(MCStringRef p_path, MCStringRef& r_native_path);
//extern void MCU_path2native(char *dptr);
extern void MCU_fix_path(MCStringRef in, MCStringRef& r_out);
extern void MCU_base64encode(MCDataRef in, MCStringRef &out);
extern void MCU_base64decode(MCStringRef in, MCDataRef &out);
extern void MCU_urldecode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result);
extern bool MCU_urlencode(MCStringRef p_url, bool p_use_utf8, MCStringRef &r_encoded);
extern Boolean MCU_freeinserted(MCObjectList *&l);
extern void MCU_cleaninserted();
//extern void MCU_get_color(MCExecPoint &ep, const char *name, MCColor &c);
extern void MCU_geturl(MCExecContext& ctxt, MCStringRef p_target, MCValueRef &r_output);

// MW-2013-07-01: [[ Bug 10975 ]] This method returns true if the given string could be a url
//   (as used by MCU_geturl, to determine whether to try and fetch via libUrl).
extern bool MCU_couldbeurl(MCStringRef potential_url);
extern void MCU_puturl(MCExecContext& ctxt, MCStringRef p_target, MCValueRef p_data);
extern uint1 MCU_unicodetocharset(uint2 uchar);
extern uint1 MCU_languagetocharset(MCNameRef langname);
extern MCNameRef MCU_charsettolanguage(uint1 charset);
/* LEGACY */ extern uint1 MCU_languagetocharset(MCStringRef langname);
extern uint1 MCU_wincharsettocharset(uint2 wincharset);
extern uint1 MCU_charsettowincharset(uint1 charset);

extern bool MCU_multibytetounicode(MCDataRef p_input, uinteger_t p_charset, MCDataRef &r_output);
extern bool MCU_unicodetomultibyte(MCDataRef p_input, uinteger_t p_charset, MCDataRef &r_output);
extern void MCU_multibytetounicode(const char *s, uint4 len, char *d,
	                                   uint4 destbufferlength, uint4 &destlen,
	                                   uint1 charset);
extern void MCU_unicodetomultibyte(const char *s, uint4 len, char *d,
	                                   uint4 destbufferlength, uint4 &destlen,
	                                   uint1 charset);

extern double MCU_squared_distance_from_line(int4 sx, int4 sy, int4 ex, int4 ey, int4 x, int4 y);

// 

struct MCInterval
{
	int from;
	int to;
};

extern bool MCU_disjointrangeinclude(MCInterval*& p_ranges, int& p_count, int p_from, int p_to);
extern bool MCU_disjointrangecontains(MCInterval* p_ranges, int p_count, int p_element);

//

// MW-2013-05-02: [[ x64 ]] The 'x_length' parameter is always IO_header::len
//   which is now size_t, so match it.
IO_stat MCU_dofakewrite(char*& x_buffer, size_t& x_length, const void *p_data, uint4 p_size, uint4 p_count);

//

inline bool MCU_empty_rect(const MCRectangle& a)
{
	return a . width == 0 || a . height == 0;
}

inline bool MCU_equal_rect(const MCRectangle& a, const MCRectangle& b)
{
	return a . x == b . x && a . y == b . y &&  a . width == b . width && a . height == b . height;
}

inline MCRectangle MCU_make_rect(int2 x, int2 y, uint2 w, uint2 h)
{
	MCRectangle r;
	r . x = x;
	r . y = y;
	r . width = w;
	r . height = h;
	return r;
}

// Test whether p_string is a valid LiveCode script token
extern bool MCU_is_token(MCStringRef p_string);

// Load a library. If loading succeeds, then a non-nullptr value is returned;
// otherwise nullptr is returned.
//
// If the library parameter does not have an extension, then:
//    - mac/ios: tries framework, bundle or dylib
//    - linux/android: uses so
//    - windows: uses dll
//
// If the library parameter is absolute, then that exact location is used.
//
// If the library parameter has the prefix './', then the mapped location
// relative to the engine is used.
//
// Otherwise, the path is passed through to the system to use its search
// order.
//
MCSLibraryRef
MCU_library_load(MCStringRef p_library);

void
MCU_library_unload(MCSLibraryRef handle);

void*
MCU_library_lookup(MCSLibraryRef handle,
                   MCStringRef p_symbol);

extern "C" void *MCSupportLibraryLoad(const char *name);
extern "C" void MCSupportLibraryUnload(void *handle);
extern "C" char *MCSupportLibraryCopyNativePath(void *handle);
extern "C" void *MCSupportLibraryLookupSymbol(void *handle,
                                              const char *symbol);

/* Split a LiveCode path into dirname and basename, using the current platform's
 * rules. Any unnecessary trailing slashes will be trimmed from dir. */
bool
MCU_path_split(MCStringRef p_path,
               MCStringRef* r_dir,
               MCStringRef* r_base);

/* Split a LiveCode path into dirname and basename, using unix rules.
 * In this case, the string is split at the last '/' into prefix and suffix.
 * If the prefix is '/', then dir is '/' and base is the rest of the path;
 * Otherwise, dir is prefix and base is suffix - in this case dir will not end
 * with '/'. */
bool
MCU_path_split_unix(MCStringRef p_path,
                    MCStringRef* r_dir,
                    MCStringRef* r_base);

/* Split a LiveCode path into dirname and basename, using win32 rules.
 * Win32 paths can have the following forms in addition to unix forms.
 *   //[Share]/[Folder][/Base]
 *   Drive:[Folder]/[Base]
 * In the first case, dir is //Share/Folder and base is Base
 * In the second case, if Folder is not present then
 *   dir is Drive:/
 *   base is Base
 * If Folder is present then
 *   dir is Drive:Folder
 *   base is Base
 * The addition of a trailing '/' in the case of Folder not being present is
 * necessary to distinguish between drive relative and drive absolute paths.
 */
bool
MCU_path_split_win32(MCStringRef p_path,
                     MCStringRef* r_dir,
                     MCStringRef* r_base);

// Format color as string
extern bool MCU_format_color(const MCColor p_color, MCStringRef& r_string);

#endif
