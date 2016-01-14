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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "util.h"
#include "date.h"
#include "param.h"
#include "execpt.h"
#include "player.h"
#include "group.h"
#include "button.h"
#include "globals.h"
#include "mode.h"
#include "eventqueue.h"
#include "osspec.h"
#include "redraw.h"

#include "osxdc.h"

#ifdef OLD_MAC
void MCScreenDC::expose()
{
	SetGWorld(GetWindowPort(getinvisiblewin()), GetMainDevice());
	
	EventRecord event;
	while (GetNextEvent(updateMask, &event))
		doredraw(event);
}
#endif

#ifdef OLD_MAC
uint1 MCScreenDC::fontnametocharset(const char *oldfontname)
{
	// MW-2006-06-09: [[ Bug 3670 ]] Fixed length buffer can cause a crash
	char fname[256];
	strncpy(fname, oldfontname, 255);
	fname[255] = '\0';
	
	char *sptr = fname;
	if ((sptr = strchr(fname, ',')) != NULL)
		*sptr = '\0';
	short ffamilyid;		    //font family ID
	StringPtr reqnamePascal = c2pstr(fname);
	GetFNum(reqnamePascal, &ffamilyid);
	return MCS_langidtocharset(FontToScript(ffamilyid));
}

char *MCScreenDC::charsettofontname(uint1 charset, const char *oldfontname)
{
	char *fname = new char[255];
	strcpy(fname, oldfontname);
	char *sptr = fname;
	if ((sptr = strchr(fname, ',')) != NULL)
		*sptr = '\0';
	char *tmpname = strclone(fname);//make a copy of the font name
	short ffamilyid;		    //font family ID
	StringPtr reqnamePascal = c2pstr(tmpname);
	GetFNum(reqnamePascal, &ffamilyid);
	delete tmpname;
	if (FontToScript(ffamilyid) != MCS_charsettolangid(charset))
	{
		GetFontName(GetScriptVariable(MCS_charsettolangid(charset),
		                              smScriptAppFond), (unsigned char *)fname);
		p2cstr((unsigned char *)fname);
	}
	return fname;
}
#endif

#ifdef OLD_MAC
CFStringRef MCScreenDC::convertutf8tocf(const char *p_utf8_string)
{
	return CFStringCreateWithCString(kCFAllocatorDefault, p_utf8_string, kCFStringEncodingUTF8);
}
#endif

#ifdef OLD_MAC
void MCScreenDC::setstatus(const char *status)
{ //No action
}
#endif

#ifdef OLD_MAC
void MCScreenDC::setcmap(MCStack *sptr)
{// no action
}

void MCScreenDC::sync(Window w)
{
	flush(w);
}

void MCScreenDC::flush(Window w)
{
	if (w != DNULL)
	{
		CGrafPtr t_port;
		t_port = GetWindowPort((WindowPtr)w -> handle . window);
		if (t_port != NULL)
			QDFlushPortBuffer(GetWindowPort((WindowPtr)w->handle.window), NULL);
	}
}
#endif

#ifdef OLD_MAC
void MCScreenDC::setgraphicsexposures(Boolean on, MCStack *sptr)
{
	ge = on;
}
#endif

#ifdef OLD_MAC
uint2 MCScreenDC::getpad()
{ //return the boundary each scan line is padded to.
	return 32;
}
#endif

#ifdef OLD_MAC
Window MCScreenDC::getroot()
{
	static Drawable mydrawable;
	GrafPtr wMgrPort = NULL;  //window manager port == desktop port
	if (mydrawable == DNULL)
		mydrawable = new _Drawable;
	mydrawable->type = DC_WINDOW;
	mydrawable->handle.window = (MCSysWindowHandle)wMgrPort; //broken
	return mydrawable;
}
#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef OLD_MAC
void MCScreenDC::copybits(Drawable s, Drawable d, int2 sx, int2 sy,
                          uint2 sw, uint2 sh, int2 dx, int2 dy, uint4 rop)
{
	const BitMap *sbm, *dbm;
	PixMapHandle spm, dpm;
	if (s->type == DC_WINDOW)
		sbm = GetPortBitMapForCopyBits(GetWindowPort((WindowPtr)s->handle.window));
	else
	{
		spm = GetGWorldPixMap((CGrafPtr)s->handle.pixmap);
		LockPixels(spm);
		sbm = (BitMap *)*spm;
	}
	if (d->type == DC_WINDOW)
		dbm = GetPortBitMapForCopyBits(GetWindowPort((WindowPtr)d->handle.window));
	else
	{
		dpm = GetGWorldPixMap((CGrafPtr)d->handle.pixmap);
		LockPixels(dpm);
		dbm = (BitMap *)*dpm;
	}
	Rect srcR;
	Rect destR;
	SetRect(&srcR, sx, sy, sx + sw , sy + sh);
	SetRect(&destR, dx, dy, dx + sw, dy + sh);
	CopyBits(sbm, dbm, &srcR, &destR, rop, NULL);
	if (s->type == DC_BITMAP)
		UnlockPixels(spm);
	if (d->type == DC_BITMAP)
		UnlockPixels(dpm);
}

void MCScreenDC::copyarea(Drawable s, Drawable d, int2 depth,
                          int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx,
                          int2 dy, uint4 rop)
{
	if (s == DNULL || d == DNULL)
		return;
	CGrafPtr oldport;
	GDHandle olddevice;
	GetGWorld(&oldport, &olddevice);
	if (ge && d->type == DC_WINDOW && s == d && rop == GXcopy)
	{
		SetGWorld(GetWindowPort((WindowPtr)d->handle.window), GetMainDevice());
		Rect r;
		r.top = MCU_min(sy, dy);
		r.left = MCU_min(sx, dx);
		r.right = MCU_max(sx + sw, dx + sw);
		r.bottom = MCU_max(sy + sh, dy + sh);
		RgnHandle dirty = NewRgn();
		Rect windRect;
		RectRgn(dirty, GetPortBounds(GetWindowPort((WindowPtr)d->handle.window), &windRect));
		SetClip(dirty);
		ScrollRect(&r, dx - sx, dy - sy, dirty);
		r.left = sx <= dx ? sx : dx + sw;
		r.top = sy <= dy ? sy : dy + sh;
		r.right = sx == dx ? sx + sw : r.left + MCU_abs(sx - dx);
		r.bottom = sy == dy ? sy + sh : r.top + MCU_abs(sy - dy);
		RgnHandle clean = NewRgn();
		RectRgn(clean, &r);
		DiffRgn(dirty, clean, dirty);
		if (!EmptyRgn(dirty))
		{
			InvalWindowRgn((WindowPtr)d->handle.window, dirty);
			expose();
		}
		DisposeRgn(dirty);
		DisposeRgn(clean);
	}
	else
	{
		if (d -> type == DC_WINDOW)
			SetGWorld(GetWindowPort((WindowPtr)d -> handle . window), GetMainDevice());
		else
			SetGWorld((CGrafPtr)d->handle.pixmap, NULL);
		ForeColor(blackColor);
		BackColor(whiteColor);
		copybits(s, d, sx, sy, sw, sh, dx, dy, ink_table_c[rop]);
	}
	SetGWorld(oldport, olddevice);
}

void MCScreenDC::freepixmap(Pixmap &p)
{
	if (p != DNULL)
	{
		DisposeGWorld((CGrafPtr)p->handle.pixmap);
		delete p;
		p = DNULL;
	}
}

Pixmap MCScreenDC::createpixmap(uint2 width, uint2 height,
                                uint2 depth, Boolean purge)
{
	GWorldPtr hpixmap = NULL;
	Rect r;
	r.top = r.left = 0;
	r.bottom = height;
	r.right = width;
	GWorldFlags f = 0;
	if (depth != 1)
		depth = 32;
	if (usetemp && width * height * depth > MAXUINT2 || FreeMem() < (MAXUINT2 * 8))
		f = useTempMem;
	QDErr err = NewGWorld(&hpixmap, depth, &r, NULL, NULL, MCmajorosversion >= 0x1040 ? kNativeEndianPixMap : 0);
	if (hpixmap == NULL || err != noErr)
		return DNULL;
	Pixmap pm = new _Drawable;
	pm->type = DC_BITMAP;
	pm->handle.pixmap = (MCSysBitmapHandle)hpixmap;
	return pm;
}

bool MCScreenDC::lockpixmap(Pixmap p_pixmap, void*& r_bits, uint4& r_stride)
{
	PixMapHandle t_src_pixmap;
	
	t_src_pixmap = GetGWorldPixMap((CGrafPtr)p_pixmap -> handle . pixmap);
	LockPixels(t_src_pixmap);
	HLock((Handle)t_src_pixmap);
	
	r_bits = GetPixBaseAddr(t_src_pixmap);
	r_stride = GetPixRowBytes(t_src_pixmap);
	
	return true;
}

void MCScreenDC::unlockpixmap(Pixmap p_pixmap, void *p_bits, uint4 p_stride)
{
	PixMapHandle t_src_pixmap;
	
	t_src_pixmap = GetGWorldPixMap((CGrafPtr)p_pixmap -> handle . pixmap);
	HUnlock((Handle)t_src_pixmap);
	UnlockPixels(t_src_pixmap);
}

Boolean MCScreenDC::getpixmapgeometry(Pixmap p, uint2 &w, uint2 &h, uint2 &d)
{
	if (p != DNULL)
	{
		PixMapHandle hpm = GetGWorldPixMap((CGrafPtr)p->handle.pixmap);
		Rect r;
		GetPixBounds(hpm, &r);
		w = r.right - r.left;
		h = r.bottom - r.top;
		d = GetPixDepth(hpm);
		return True;
	}
	return False;
}
#endif
