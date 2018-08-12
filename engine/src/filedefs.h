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
// Structures used for reading/writing
//
#ifndef	FILEDEFS_H
#define	FILEDEFS_H

#include "typedefs.h"

#define READ_PIPE_SIZE 16384

#define SAVE_ENCRYPTED 0x80000000
#define SAVE_LARGE     0x40000000

enum Open_mode {
    OM_APPEND,
    OM_NEITHER,
    OM_VCLIP,
    OM_READ,
    OM_WRITE,
    OM_UPDATE,
    OM_TEXT,
    OM_BINARY
};

enum Encoding_type
{
    EN_BOM_BASED,
    EN_NATIVE,
    EN_UTF8,
    EN_UTF16,
    EN_UTF16LE,
    EN_UTF16BE,
    EN_UTF32,
    EN_UTF32LE,
    EN_UTF32BE,
    EN_BINARY
};

enum Object_type {
    OT_END,
    OT_HOME,
    OT_NOTHOME,
    OT_STACK,
    OT_CARD,
    OT_GROUP,
    OT_GROUPEND,
    OT_PTR,
    OT_BUTTON,
    OT_BDATA,
    OT_FIELD,
    OT_FDATA,
    OT_PARAGRAPH,
    OT_BLOCK,
    OT_IMAGE,
    OT_SCROLLBAR,
    OT_MAGNIFY,
    OT_COLORS,
    OT_GRAPHIC,
    OT_MCEPS,
    OT_AUDIO_CLIP,
    OT_VIDEO_CLIP,
    OT_ENCRYPT_STACK,
    OT_PLAYER,
    OT_CUSTOM,
	OT_STYLED_TEXT,
	// MW-2012-03-03: [[ StackFile5500 ]] The extended paragraph tag.
	OT_PARAGRAPH_EXT,
	// MW-2012-03-04: [[ StackFile5500 ]] The extended block tag.
	OT_BLOCK_EXT,
    // MW-2014-12-16: [[ Widgets ]] The widget object tag.
    OT_WIDGET,
};

#define IO_WRITTEN    (1UL << 0)
#define IO_SEEKED     (1UL << 1)
#define IO_ATEOF      (1UL << 2)
#define IO_FAKE       (1UL << 3)
#define IO_FAKEWRITE	(IO_FAKE | (1UL << 4))
#define IO_FAKECUSTOM (IO_FAKE | (1UL << 5))

#define BIONB_CLEAR
#define BIONB_TESTREAD    (1UL << 1)
#define BIONB_TESTWRITE    (1UL << 2)
#define BOONB_TESTCONNECT    (1UL << 3)

struct MCSystemFileHandle;

typedef MCSystemFileHandle * IO_handle;
typedef struct _Streamnode
{
	MCNameRef name;
	Open_mode mode;
    Encoding_type encoding;
	IO_handle ihandle;
	IO_handle ohandle;
	int4 pid;
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
	MCWinSysHandle phandle;  //process handle
	MCWinSysHandle thandle;  //process handle
#elif defined(_MAC_DESKTOP) || defined(_MAC_SERVER)
	MCMacProcessSerialNumber sn;
#endif
    int4 retcode;
}
Streamnode;

extern IO_handle IO_stdin;
extern IO_handle IO_stdout;
extern IO_handle IO_stderr;

#endif
