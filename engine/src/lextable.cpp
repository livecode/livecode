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

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "scriptpt.h"

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
#define ST_INB ST_ID
#define ST_MNB ST_SPC
#else
#define ST_INB ST_SPC
#define ST_MNB ST_ID
#endif


// Some of these tables need to be accessed from other compilation units and C++
// mandates that variables declared as "const" have internal linkage unless also
// declared as "extern".
extern const LT command_table[];
extern const Cvalue *constant_table;
extern const LT factor_table[];
extern const LT * const table_pointers[];
extern const uint2 table_sizes[];
extern const uint8_t type_table[];
extern const uint8_t unicode_type_table[];


// MW-2011-06-22: [[ SERVER ]] We mark '?' as ST_TAG so we can parse server
//   style scripts. If the SP's tagged property is false, it reverts to ST_ID.
//   Also, cr is marked as ST_EOL rather than ST_SPC. This will make little
//   difference to object scripts but means we can be newline agnostic in server
//   scripts.
const uint8_t type_table[256] =
{
    ST_EOF,  ST_ID,   ST_ID,   ST_ID,   //     ^@      ^A      ^B      ^C
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //     ^D      ^E      ^F      ^G
    ST_ID,   ST_SPC,  ST_EOL,  ST_ID,   //     ^H      ^I      ^J      ^K
    ST_ID,   ST_EOL,  ST_ID,   ST_ID,   //     ^L      ^M      ^N      ^O
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //     ^P      ^Q      ^R      ^S
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //     ^T      ^U      ^V      ^W
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //     ^X      ^Y      ^Z      ^[
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //     ^\      ^]      ^^      ^_
    ST_SPC,  ST_OP,   ST_LIT,  ST_COM,  //              !       "       #
    ST_ID,   ST_OP,   ST_OP,   ST_ID,   //      $       %       &       '
    ST_LP,   ST_RP,   ST_OP,   ST_OP,   //      (       )       *       +
    ST_SEP,  ST_MIN,  ST_NUM,  ST_OP,   //      ,       -       .       /
    ST_NUM,  ST_NUM,  ST_NUM,  ST_NUM,  //      0       1       2       3
    ST_NUM,  ST_NUM,  ST_NUM,  ST_NUM,  //      4       5       6       7
    ST_NUM,  ST_NUM,  ST_OP,   ST_SEMI, //      8       9       :       ;
    ST_OP,   ST_OP,   ST_OP,   ST_TAG,   //      <       =       >       ?
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      @       A       B       C
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      D       E       F       G
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      H       I       J       K
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      L       M       N       O
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      P       Q       R       S
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      T       U       V       W
    ST_ID,   ST_ID,   ST_ID,   ST_LB,   //      X       Y       Z       [
    ST_ESC,  ST_RB,   ST_OP,   ST_ID,   //      \       ]       ^       _
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      `       a       b       c
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      d       e       f       g
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      h       i       j       k
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      l       m       n       o
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      p       q       r       s
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      t       u       v       w
    ST_ID,   ST_ID,   ST_ID,   ST_LC,   //      x       y       z       {
    ST_OP,   ST_RC,   ST_OP,   ST_ID,   //      |       }       ~       DEL
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0x80    0x81    0x82    0x83
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0x84    0x85    0x86    0x87
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0x88    0x89    0x8A    0x8B
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0x8C    0x8D    0x8E    0x8F
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0x90    0x91    0x92    0x93
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0x94    0x95    0x96    0x97
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0x98    0x99    0x9A    0x9B
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0x9C    0x9D    0x9E    0x9F
    ST_INB,  ST_ID,   ST_ID,   ST_ID,   //      0xA0    0xA1    0xA2    0xA3
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xA4    0xA5    0xA6    0xA7
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xA8    0xA9    0xAA    0xAB
    ST_ESC,  ST_OP,   ST_ID,   ST_ID,   //      0xAC    0xAD    0xAE    0xAF
    ST_ID,   ST_ID,   ST_OP,   ST_OP,   //      0xB0    0xB1    0xB2    0xB3
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xB4    0xB5    0xB6    0xB7
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xB8    0xB9    0xBA    0xBB
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xBC    0xBD    0xBE    0xBF
    ST_ID,   ST_ID,   ST_ESC,  ST_ID,   //      0xC0    0xC1    0xC2    0xC3
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xC4    0xC5    0xC6    0xC7
    ST_ID,   ST_ID,   ST_MNB,  ST_ID,   //      0xC8    0xC9    0xCA    0xCB
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xCC    0xCD    0xCE    0xCF
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xD0    0xD1    0xD2    0xD3
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xD4    0xD5    0xD6    0xD7
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xD8    0xD9    0xDA    0xDB
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xDC    0xDD    0xDE    0xDF
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xE0    0xE1    0xE2    0xE3
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xE4    0xE5    0xE6    0xE7
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xE8    0xE9    0xEA    0xEB
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xEC    0xED    0xEE    0xEF
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xF0    0xF1    0xF2    0xF3
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xF4    0xF5    0xF6    0xF7
    ST_ID,   ST_ID,   ST_ID,   ST_ID,   //      0xF8    0xF9    0xFA    0xFB
    ST_ID,   ST_ID,   ST_ID,   ST_ID    //      0xFC    0xFD    0xFE    0xFF
};

const uint8_t unicode_type_table[256] =
{
    ST_EOF,         ST_ID,          ST_ID,          ST_ID,          //     ^@      ^A      ^B      ^C
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //     ^D      ^E      ^F      ^G
    ST_ID,          ST_SPC,         ST_EOL,         ST_ID,          //     ^H      ^I      ^J      ^K
    ST_ID,          ST_EOL,         ST_ID,          ST_ID,          //     ^L      ^M      ^N      ^O
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //     ^P      ^Q      ^R      ^S
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //     ^T      ^U      ^V      ^W
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //     ^X      ^Y      ^Z      ^[
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //     ^\      ^]      ^^      ^_
    ST_SPC,         ST_OP,          ST_LIT,         ST_COM,         //              !       "       #
    ST_ID,          ST_OP,          ST_OP,          ST_ID,          //      $       %       &       '
    ST_LP,          ST_RP,          ST_OP,          ST_OP,          //      (       )       *       +
    ST_SEP,         ST_MIN,         ST_NUM,         ST_OP,          //      ,       -       .       /
    ST_NUM,         ST_NUM,         ST_NUM,         ST_NUM,         //      0       1       2       3
    ST_NUM,         ST_NUM,         ST_NUM,         ST_NUM,         //      4       5       6       7
    ST_NUM,         ST_NUM,         ST_OP,          ST_SEMI,        //      8       9       :       ;
    ST_OP,          ST_OP,          ST_OP,          ST_TAG,         //      <       =       >       ?
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      @       A       B       C
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      D       E       F       G
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      H       I       J       K
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      L       M       N       O
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      P       Q       R       S
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      T       U       V       W
    ST_ID,          ST_ID,          ST_ID,          ST_LB,          //      X       Y       Z       [
    ST_ESC,         ST_RB,          ST_OP,          ST_ID,          //      \       ]       ^       _
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      `       a       b       c
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      d       e       f       g
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      h       i       j       k
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      l       m       n       o
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      p       q       r       s
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      t       u       v       w
    ST_ID,          ST_ID,          ST_ID,          ST_LC,          //      x       y       z       {
    ST_OP,          ST_RC,          ST_OP,          ST_ID,          //      |       }       ~       DEL
    ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,	//      0x80    0x81    0x82    0x83
    ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,	//      0x84    0x85    0x86    0x87
    ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,	//      0x88    0x89    0x8A    0x8B
    ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,	//      0x8C    0x8D    0x8E    0x8F
    ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,	//      0x90    0x91    0x92    0x93
    ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,	//      0x94    0x95    0x96    0x97
    ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,	//      0x98    0x99    0x9A    0x9B
    ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,	//      0x9C    0x9D    0x9E    0x9F
    ST_SPC,         ST_ID,          ST_ID,          ST_ID,          //      0xA0    0xA1    0xA2    0xA3
    ST_UNDEFINED,   ST_ID,          ST_UNDEFINED,   ST_ID,          //      0xA4    0xA5    0xA6    0xA7
    ST_ESC,         ST_ID,          ST_ID,          ST_ID,          //      0xA8    0xA9    0xAA    0xAB
    ST_ESC,         ST_UNDEFINED,   ST_ID,          ST_ID,          //      0xAC    0xAD    0xAE    0xAF
    ST_ID,          ST_ID,          ST_UNDEFINED,   ST_UNDEFINED,	//      0xB0    0xB1    0xB2    0xB3
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xB4    0xB5    0xB6    0xB7
    ST_ID,          ST_UNDEFINED,   ST_ID,          ST_ID,          //      0xB8    0xB9    0xBA    0xBB
    ST_UNDEFINED,   ST_UNDEFINED,   ST_UNDEFINED,   ST_ID,          //      0xBC    0xBD    0xBE    0xBF
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xC0    0xC1    0xC2    0xC3
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xC4    0xC5    0xC6    0xC7
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xC8    0xC9    0xCA    0xCB
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xCC    0xCD    0xCE    0xCF
    ST_UNDEFINED,   ST_ID,          ST_ID,          ST_ID,          //      0xD0    0xD1    0xD2    0xD3
    ST_ID,          ST_ID,          ST_ID,          ST_UNDEFINED,	//      0xD4    0xD5    0xD6    0xD7
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xD8    0xD9    0xDA    0xDB
    ST_ID,          ST_UNDEFINED,   ST_UNDEFINED,   ST_ID,          //      0xDC    0xDD    0xDE    0xDF
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xE0    0xE1    0xE2    0xE3
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xE4    0xE5    0xE6    0xE7
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xE8    0xE9    0xEA    0xEB
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xEC    0xED    0xEE    0xEF
    ST_UNDEFINED,   ST_ID,          ST_ID,          ST_ID,          //      0xF0    0xF1    0xF2    0xF3
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xF4    0xF5    0xF6    0xF7
    ST_ID,          ST_ID,          ST_ID,          ST_ID,          //      0xF8    0xF9    0xFA    0xFB
    ST_ID,          ST_UNDEFINED,   ST_UNDEFINED,   ST_ID,          //      0xFC    0xFD    0xFE    0xFF
};

/* The constant_table must be constexpr to force the compiler to generate
 * it at compile time (the rule is that constexpr expressions may be evaluated
 * at compile-time or runtime *unless* they are used as a subexpression of a
 * constexpr in which case they must be evaluated at compile-time - MSVC
 * needs constexpr here to force compile-time eval; other compilers are happy
 * with just const - go figure!). */
static constexpr const Cvalue constant_table_values[] =
{
    /* Initializer lists are used as (in C++11) they map to constructors. */
    {"arrow", 29},
    {"backslash", "\\"},
    {"busy", 6},
    {"clock", 14},
    {"colon", ":"},
    {"comma", ","},
    {"cr", "\n"},
    {"crlf", "\r\n"},
    {"cross", 7},
    {"done", "done"},
    {"down", "down"},
    {"eight", 8},
    {"empty", kCValueTypeEmpty},
    {"end", "\004"},
    {"endoffile", "\004"},
    {"eof", "\004"},
    {"false", kCValueTypeFalse},
    {"five", 5},
    {"formfeed", "\014"},
    {"four", 4},
    {"hand", 28},
    {"help", 15},
    {"ibeam", 9},
    {"infinity", kCValueTypeInfinity},
    {"left", "left"},
    {"lf", "\n"},
    {"linefeed", "\n"},
    {"nine", 9},
    {"none", 0},
    {"null", kCValueTypeNull},
    {"one", 1},
    {"pi", 3.14159265358979323846},
    {"plus", 13},
    {"quote", "\""},
    {"return", "\n"},
    {"right", "right"},
    {"scrollbarfactor", 65535},
    {"seven", 7},
    {"six", 6},
    {"slash", "/"},
    {"space", " "},
    {"tab", "\t"},
    {"ten", 10},
    {"three", 3},
    {"true", kCValueTypeTrue},
    {"two", 2},
    {"up", "up"},
    {"watch", 14},
    {"zero", 0},
};
const Cvalue *constant_table = constant_table_values;
extern const uint4 constant_table_size = ELEMENTS(constant_table_values);

const static LT accept_table[] =
    {
        {"connections", TT_UNDEFINED, AC_CONNECTIONS},
        {"datagram", TT_UNDEFINED, AC_DATAGRAM},
        {"datagrams", TT_UNDEFINED, AC_DATAGRAM},
        {"on", TT_UNDEFINED, AC_ON},
        {"port", TT_UNDEFINED, AC_PORT},
        {"secure", TT_UNDEFINED, AC_SECURE}
    };

const static LT ae_table[] =
    {
        {"ae", TT_UNDEFINED, AE_AE},
        {"appleevent", TT_UNDEFINED, AE_AE},
        {"class", TT_UNDEFINED, AE_CLASS},
        {"data", TT_UNDEFINED, AE_DATA},
        {"id", TT_UNDEFINED, AE_ID},
        {"returnid", TT_UNDEFINED, AE_RETURN_ID},
        {"sender", TT_UNDEFINED, AE_SENDER}
    };

const static LT ask_table[] =
    {
        {"application", TT_UNDEFINED, AT_PROGRAM},
        {"clear", TT_UNDEFINED, AT_CLEAR},
        {"color", TT_UNDEFINED, AT_COLOR},
        {"directory", TT_UNDEFINED, AT_FOLDER},
        {"effect", TT_UNDEFINED, AT_EFFECT},
        {"error", TT_UNDEFINED, AT_ERROR},
        {"file", TT_UNDEFINED, AT_FILE},
		{"files", TT_UNDEFINED, AT_FILES},
        {"folder", TT_UNDEFINED, AT_FOLDER},
		{"hint", TT_UNDEFINED, AT_HINT},
        {"info", TT_UNDEFINED, AT_INFORMATION},
        {"information", TT_UNDEFINED, AT_INFORMATION},
		{"page", TT_UNDEFINED, AT_PAGE},
        {"password", TT_UNDEFINED, AT_PASSWORD},
        {"plain", TT_UNDEFINED, AT_UNDEFINED},
        {"printer", TT_UNDEFINED, AT_PRINTER},
        {"process", TT_UNDEFINED, AT_PROGRAM},
        {"program", TT_UNDEFINED, AT_PROGRAM},
        {"question", TT_UNDEFINED, AT_QUESTION},
        {"record", TT_UNDEFINED, AT_RECORD},
		{"setup", TT_UNDEFINED, AT_SETUP},
        {"sheet", TT_UNDEFINED, AT_SHEET},
        {"titled", TT_UNDEFINED, AT_TITLED},
		{"types", TT_UNDEFINED, AT_TYPES},
        {"warning", TT_UNDEFINED, AT_WARNING}
    };

const LT command_table[] =
    {
		{"_internal", TT_STATEMENT, S_INTERNAL},
        {"accept", TT_STATEMENT, S_ACCEPT},
        {"add", TT_STATEMENT, S_ADD},
        {"answer", TT_STATEMENT, S_ANSWER},
        {"ask", TT_STATEMENT, S_ASK},
		{"assert", TT_STATEMENT, S_ASSERT},
        {"beep", TT_STATEMENT, S_BEEP},
        {"break", TT_STATEMENT, S_BREAK},
        {"breakpoint", TT_STATEMENT, S_BREAKPOINT},
        {"call", TT_STATEMENT, S_CALL},
        {"cancel", TT_STATEMENT, S_CANCEL},
        {"case", TT_CASE, S_UNDEFINED},
        {"catch", TT_CATCH, S_UNDEFINED},
        {"choose", TT_STATEMENT, S_CHOOSE},
        {"clear", TT_STATEMENT, S_DELETE},
        {"click", TT_STATEMENT, S_CLICK},
        {"clone", TT_STATEMENT, S_CLONE},
        {"close", TT_STATEMENT, S_CLOSE},
        {"combine", TT_STATEMENT, S_COMBINE},
		{"command", TT_STATEMENT, S_SCRIPT_ERROR},
        {"compact", TT_STATEMENT, S_COMPACT},
        {"constant", TT_STATEMENT, S_CONSTANT},
        {"convert", TT_STATEMENT, S_CONVERT},
        {"copy", TT_STATEMENT, S_COPY},
        {"create", TT_STATEMENT, S_CREATE},
        {"crop", TT_STATEMENT, S_CROP},
        {"cut", TT_STATEMENT, S_CUT},
        {"debugdo", TT_STATEMENT, S_DEBUGDO},
        {"decrypt", TT_STATEMENT, S_DECRYPT},
        {"default", TT_DEFAULT, S_UNDEFINED},
        {"define", TT_STATEMENT, S_DEFINE},
        {"dehilite", TT_STATEMENT, S_UNHILITE},
        {"delete", TT_STATEMENT, S_DELETE},
        {"difference", TT_STATEMENT, S_DIFFERENCE},
		// MW-2008-11-05: [[ Dispatch Command ]] 'dispatch' is a statement keyword
        {"disable", TT_STATEMENT, S_DISABLE},
		{"dispatch", TT_STATEMENT, S_DISPATCH},
        {"divide", TT_STATEMENT, S_DIVIDE},
        {"do", TT_STATEMENT, S_DO},
        {"domenu", TT_STATEMENT, S_DOMENU},
        {"drag", TT_STATEMENT, S_DRAG},
        {"drawer", TT_STATEMENT, S_DRAWER},
        {"edit", TT_STATEMENT, S_EDIT},
        {"else", TT_ELSE, S_UNDEFINED},
        {"enable", TT_STATEMENT, S_ENABLE},
        {"encrypt", TT_STATEMENT, S_ENCRYPT},
        {"end", TT_END, S_UNDEFINED},
        {"exit", TT_STATEMENT, S_EXIT},
        {"export", TT_STATEMENT, S_EXPORT},
        {"filter", TT_STATEMENT, S_FILTER},
        {"finally", TT_FINALLY, S_UNDEFINED},
        {"find", TT_STATEMENT, S_FIND},
        {"flip", TT_STATEMENT, S_FLIP},
        {"focus", TT_STATEMENT, S_FOCUS},
        {"function", TT_STATEMENT, S_SCRIPT_ERROR},
        {"get", TT_STATEMENT, S_GET},
        {"getprop", TT_STATEMENT, S_SCRIPT_ERROR},
        {"global", TT_STATEMENT, S_GLOBAL},
        {"go", TT_STATEMENT, S_GO},
        {"grab", TT_STATEMENT, S_GRAB},
        {"group", TT_STATEMENT, S_GROUP},
        {"hide", TT_STATEMENT, S_HIDE},
        {"hilite", TT_STATEMENT, S_HILITE},
        {"if", TT_STATEMENT, S_IF},
        {"import", TT_STATEMENT, S_IMPORT},
		{"include", TT_STATEMENT, S_INCLUDE},
        {"insert", TT_STATEMENT, S_INSERT},
        {"intersect", TT_STATEMENT, S_INTERSECT},
        {"kill", TT_STATEMENT, S_KILL},
        {"launch", TT_STATEMENT, S_LAUNCH},
        {"library", TT_STATEMENT, S_LIBRARY},
        {"load", TT_STATEMENT, S_LOAD},
        {"local", TT_STATEMENT, S_LOCAL},
        {"lock", TT_STATEMENT, S_LOCK},
        {"log", TT_STATEMENT, S_LOG},
        {"mark", TT_STATEMENT, S_MARK},
        {"modal", TT_STATEMENT, S_MODAL},
        {"modeless", TT_STATEMENT, S_MODELESS},
        {"move", TT_STATEMENT, S_MOVE},
        {"multiply", TT_STATEMENT, S_MULTIPLY},
        {"new", TT_STATEMENT, S_CREATE},
        {"next", TT_STATEMENT, S_NEXT},
        {"on", TT_STATEMENT, S_SCRIPT_ERROR},
        {"open", TT_STATEMENT, S_OPEN},
        {"option", TT_STATEMENT, S_OPTION},
        {"palette", TT_STATEMENT, S_PALETTE},
        {"pass", TT_STATEMENT, S_PASS},
        {"paste", TT_STATEMENT, S_PASTE},
        {"place", TT_STATEMENT, S_PLACE},
        {"play", TT_STATEMENT, S_PLAY},
        {"pop", TT_STATEMENT, S_POP},
        {"popup", TT_STATEMENT, S_POPUP},
        {"post", TT_STATEMENT, S_POST},
        {"prepare", TT_STATEMENT, S_PREPARE},
        {"print", TT_STATEMENT, S_PRINT},
		{"private", TT_STATEMENT, S_SCRIPT_ERROR},
        {"pulldown", TT_STATEMENT, S_PULLDOWN},
        {"push", TT_STATEMENT, S_PUSH},
        {"put", TT_STATEMENT, S_PUT},
        {"quit", TT_STATEMENT, S_QUIT},
        {"read", TT_STATEMENT, S_READ},
        {"record", TT_STATEMENT, S_RECORD},
        {"redo", TT_STATEMENT, S_REDO},
		{"relayer", TT_STATEMENT, S_RELAYER},
        {"release", TT_STATEMENT, S_STOP},
        {"remove", TT_STATEMENT, S_REMOVE},
        {"rename", TT_STATEMENT, S_RENAME},
        {"repeat", TT_STATEMENT, S_REPEAT},
        {"replace", TT_STATEMENT, S_REPLACE},
        {"reply", TT_STATEMENT, S_REPLY},
        {"request", TT_STATEMENT, S_REQUEST},
		{"require", TT_STATEMENT, S_REQUIRE},
        {"reset", TT_STATEMENT, S_RESET},
        {"resolve", TT_STATEMENT, S_RESOLVE},
        {"return", TT_STATEMENT, S_RETURN},
        {"revert", TT_STATEMENT, S_REVERT},
#ifdef MODE_DEVELOPMENT
		{"revrelicense", TT_STATEMENT, S_REV_RELICENSE},
#endif
        {"rotate", TT_STATEMENT, S_ROTATE},
		{"save", TT_STATEMENT, S_SAVE},
		// MM-2014-02-12: [[ SecureSocket ]] 'secure' statement used by secure socket
		{"secure", TT_STATEMENT, S_SECURE},
		{"seek", TT_STATEMENT, S_SEEK},
        {"select", TT_STATEMENT, S_SELECT},
        {"send", TT_STATEMENT, S_SEND},
        {"set", TT_STATEMENT, S_SET},
        {"setprop", TT_STATEMENT, S_SCRIPT_ERROR},
        {"sheet", TT_STATEMENT, S_SHEET},
        {"show", TT_STATEMENT, S_SHOW},
        {"sort", TT_STATEMENT, S_SORT},
        {"split", TT_STATEMENT, S_SPLIT},
        {"start", TT_STATEMENT, S_START},
        {"stop", TT_STATEMENT, S_STOP},
        {"subtract", TT_STATEMENT, S_SUBTRACT},
        {"switch", TT_STATEMENT, S_SWITCH},
        {"symmetric", TT_STATEMENT, S_SYMMETRIC},
        {"then", TT_THEN, S_UNDEFINED},
        {"throw", TT_STATEMENT, S_THROW},
        {"toplevel", TT_STATEMENT, S_TOP_LEVEL},
        {"try", TT_STATEMENT, S_TRY},
        {"type", TT_STATEMENT, S_TYPE},
        {"undefine", TT_STATEMENT, S_UNDEFINE},
        {"undo", TT_STATEMENT, S_UNDO},
        {"ungroup", TT_STATEMENT, S_UNGROUP},
        {"unhilite", TT_STATEMENT, S_UNHILITE},
        {"union", TT_STATEMENT, S_UNION},
        {"unload", TT_STATEMENT, S_UNLOAD},
        {"unlock", TT_STATEMENT, S_UNLOCK},
        {"unmark", TT_STATEMENT, S_UNMARK},
        {"visual", TT_STATEMENT, S_VISUAL},
        {"wait", TT_STATEMENT, S_WAIT},
        {"write", TT_STATEMENT, S_WRITE}
    };
extern const uint4 command_table_size = ELEMENTS(command_table);

const static LT convert_table[] =
    {
        {"abbr", TT_CHUNK, CF_ABBREVIATED},
        {"abbrev", TT_CHUNK, CF_ABBREVIATED},
        {"abbreviated", TT_CHUNK, CF_ABBREVIATED},
        {"date", TT_CHUNK, CF_DATE},
        {"dateitems", TT_CHUNK, CF_DATEITEMS},
        {"detailed", TT_CHUNK, CF_LONG},
        {"english", TT_CHUNK, CF_ENGLISH},
        {"internet", TT_CHUNK, CF_INTERNET},
        {"long", TT_CHUNK, CF_LONG},
        {"seconds", TT_CHUNK, CF_SECONDS},
        {"secs", TT_CHUNK, CF_SECONDS},
        {"short", TT_CHUNK, CF_SHORT},
        {"system", TT_CHUNK, CF_SYSTEM},
        {"time", TT_CHUNK, CF_TIME}
    };

const static LT encryption_table[] =
    {
        {"bit", TT_UNDEFINED, ENCRT_BIT},
        {"cert", TT_UNDEFINED, RSA_CERT},
        {"iv", TT_UNDEFINED, ENCRT_IV},
        {"key", TT_UNDEFINED, ENCRT_KEY},
		{"passphrase", TT_UNDEFINED, ENCRT_PASSPHRASE},
        {"password", TT_UNDEFINED, ENCRT_PASSWORD},
		{"private", TT_UNDEFINED, ENCRT_PRIVATE},
        {"privatekey", TT_UNDEFINED, RSA_PRIVKEY},
		{"public", TT_UNDEFINED, ENCRT_PUBLIC},
        {"publickey", TT_UNDEFINED, RSA_PUBKEY},
		{"rsa", TT_UNDEFINED, ENCRT_RSA},
        {"salt", TT_UNDEFINED, ENCRT_SALT},
        {"using", TT_UNDEFINED, ENCRT_USING},
    };

const static LT exit_table[] =
    {
        {"hypercard", TT_UNDEFINED, ET_ALL},
        {"metacard", TT_UNDEFINED, ET_ALL},
        {"repeat", TT_UNDEFINED, ET_REPEAT},
        {"supercard", TT_UNDEFINED, ET_ALL},
        {"switch", TT_UNDEFINED, ET_SWITCH},
        {"to", TT_UNDEFINED, ET_TO},
        {"top", TT_UNDEFINED, ET_ALL}
    };

const static LT export_table[] =
    {
		{"abgr", TT_UNDEFINED, EX_RAW_ABGR},
        {"ac", TT_UNDEFINED, EX_AUDIO_CLIP},
        {"aiff", TT_UNDEFINED, EX_AIFF},
		{"argb", TT_UNDEFINED, EX_RAW_ARGB},
        {"audioclip", TT_UNDEFINED, EX_AUDIO_CLIP},
		{"bgra", TT_UNDEFINED, EX_RAW_BGRA},
		{"bmp", TT_UNDEFINED, EX_BMP},
        {"display", TT_UNDEFINED, EX_DISPLAY},
        {"eps", TT_UNDEFINED, EX_EPS},
        {"flc", TT_UNDEFINED, EX_VIDEO_CLIP},
        {"fli", TT_UNDEFINED, EX_VIDEO_CLIP},
        {"gif", TT_UNDEFINED, EX_GIF},
        {"jpeg", TT_UNDEFINED, EX_JPEG},
        {"mask", TT_UNDEFINED, EX_PBM},
        {"paint", TT_UNDEFINED, EX_PBM},
        {"pbm", TT_UNDEFINED, EX_PBM},
        {"pgm", TT_UNDEFINED, EX_PBM},
        {"png", TT_UNDEFINED, EX_PNG},
        {"pnm", TT_UNDEFINED, EX_PBM},
        {"ppm", TT_UNDEFINED, EX_PBM},
		{"raw", TT_UNDEFINED, EX_RAW},
		{"rgba", TT_UNDEFINED, EX_RAW_RGBA},
        {"snapshot", TT_UNDEFINED, EX_SNAPSHOT},
        {"stack", TT_UNDEFINED, EX_STACK},
        {"ulaw", TT_UNDEFINED, EX_ULAW},
        {"vc", TT_UNDEFINED, EX_VIDEO_CLIP},
        {"videoclip", TT_UNDEFINED, EX_VIDEO_CLIP},
        {"wave", TT_UNDEFINED, EX_WAVE},
        {"xbm", TT_UNDEFINED, EX_XBM},
        {"xwd", TT_UNDEFINED, EX_XWD}
    };

const LT factor_table[] =
    {
        {"&", TT_BINOP, O_CONCAT},
        {"&&", TT_BINOP, O_CONCAT_SPACE},
        {"(", TT_LPAREN, O_GROUPING},
        {")", TT_RPAREN, O_UNDEFINED},
        {"*", TT_BINOP, O_TIMES},
        {"+", TT_BIN_OR_UNOP, O_PLUS},
        {"-", TT_BIN_OR_UNOP, O_MINUS},
        {"/", TT_BINOP, O_OVER},
        {"<", TT_BINOP, O_LT},
        {"<=", TT_BINOP, O_LE},
        {"<>", TT_BINOP, O_NE},
        {"=", TT_BINOP, O_EQ},
        {">", TT_BINOP, O_GT},
        {">=", TT_BINOP, O_GE},
        {"^", TT_BINOP, O_POW},	
#ifdef MODE_DEVELOPMENT
		{"_hscrollbarid", TT_PROPERTY, P_HSCROLLBARID},
		{"_ideoverride", TT_PROPERTY, P_IDE_OVERRIDE},
		{"_unplacedgroupids", TT_PROPERTY, P_UNPLACED_GROUP_IDS},
		{"_vscrollbarid", TT_PROPERTY, P_VSCROLLBARID},
#endif
        {"abbr", TT_PROPERTY, P_ABBREVIATE},
        {"abbrev", TT_PROPERTY, P_ABBREVIATE},
        {"abbreviated", TT_PROPERTY, P_ABBREVIATE},
        {"abs", TT_FUNCTION, F_ABS},
        {"ac", TT_CHUNK, CT_AUDIO_CLIP},
		{"acceleratedrendering", TT_PROPERTY, P_ACCELERATED_RENDERING},
        {"acceleratorkey", TT_PROPERTY, P_ACCELERATOR_KEY},
        {"acceleratormodifiers", TT_PROPERTY, P_ACCELERATOR_MODIFIERS},
        {"acceleratortext", TT_PROPERTY, P_ACCELERATOR_TEXT},
        {"accelkey", TT_PROPERTY, P_ACCELERATOR_KEY},
        {"accelmods", TT_PROPERTY, P_ACCELERATOR_MODIFIERS},
        {"acceltext", TT_PROPERTY, P_ACCELERATOR_TEXT},
        {"accentcolor", TT_PROPERTY, P_ACCENT_COLOR},
        {"acceptdrop", TT_PROPERTY, P_ACCEPT_DROP},
        {"acos", TT_FUNCTION, F_ACOS},
        {"acs", TT_CLASS, CT_AUDIO_CLIP},
        {"activatepalettes", TT_PROPERTY, P_ACTIVATE_PALETTES},
        {"address", TT_PROPERTY, P_ADDRESS},
        {"after", TT_PREP, PT_AFTER},
        {"alias", TT_CHUNK, CT_ALIAS},
        {"aliasreference", TT_FUNCTION, F_ALIAS_REFERENCE},
        {"aligned", TT_PREP, PT_ALIGNED},
		{"allowabledragactions", TT_PROPERTY, P_ALLOWABLE_DRAG_ACTIONS},
		{"allowdatagrambroadcasts", TT_PROPERTY, P_ALLOW_DATAGRAM_BROADCASTS},
        {"allowfieldredraw", TT_PROPERTY, P_ALLOW_FIELD_REDRAW},
        {"allowinlineinput", TT_PROPERTY, P_ALLOW_INLINE_INPUT},
        {"allowinterrupts", TT_PROPERTY, P_ALLOW_INTERRUPTS},
        {"allowkeyinfield", TT_PROPERTY, P_ALLOW_KEY_IN_FIELD},
        {"alphadata", TT_PROPERTY, P_ALPHA_DATA},
        {"alternatelanguages", TT_FUNCTION, F_ALTERNATE_LANGUAGES},
        {"altid", TT_PROPERTY, P_ALT_ID},
        {"altkey", TT_FUNCTION, F_OPTION_KEY},
        {"alwaysbuffer", TT_PROPERTY, P_ALWAYS_BUFFER},
        {"and", TT_BINOP, O_AND},
        {"angle", TT_PROPERTY, P_ANGLE},
        {"annuity", TT_FUNCTION, F_ANNUITY},
		{"antialiased", TT_PROPERTY, P_ANTI_ALIASED},
        {"any", TT_CHUNK, CT_ANY},
        {"arcangle", TT_PROPERTY, P_ARC_ANGLE},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'arithmeticMean' (aka mean / average / avg)
        {"arithmeticmean", TT_FUNCTION, F_ARI_MEAN},
        {"arm", TT_PROPERTY, P_ARM},
        {"armborder", TT_PROPERTY, P_ARM_BORDER},
        {"armed", TT_PROPERTY, P_ARM},
        {"armedicon", TT_PROPERTY, P_ARMED_ICON},
        {"armfill", TT_PROPERTY, P_ARM_FILL},
		{"arraydecode", TT_FUNCTION, F_ARRAY_DECODE},
		{"arrayencode", TT_FUNCTION, F_ARRAY_ENCODE},
        {"arrowsize", TT_PROPERTY, P_ARROW_SIZE},
        {"as", TT_PREP, PT_AS},
        {"asin", TT_FUNCTION, F_ASIN},
        {"at", TT_PREP, PT_AT},
        {"atan", TT_FUNCTION, F_ATAN},
        {"atan2", TT_FUNCTION, F_ATAN2},
        {"audioclip", TT_CHUNK, CT_AUDIO_CLIP},
        {"audioclips", TT_CLASS, CT_AUDIO_CLIP},
        {"audiopan", TT_PROPERTY, P_AUDIO_PAN},
        {"autoarm", TT_PROPERTY, P_AUTO_ARM},
        {"autohilight", TT_PROPERTY, P_AUTO_HILITE},
        {"autohilite", TT_PROPERTY, P_AUTO_HILITE},
        {"autoselect", TT_PROPERTY, P_LIST_BEHAVIOR},
        {"autotab", TT_PROPERTY, P_AUTO_TAB},
		{"availableprinters", TT_PROPERTY, P_PRINTER_NAMES},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'arithmeticMean' (aka mean / average / avg)
        {"average", TT_FUNCTION, F_ARI_MEAN},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'averageDeviation' (aka avgDev)
        {"averagedeviation", TT_FUNCTION, F_AVG_DEV},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'arithmeticMean' (aka mean / average / avg)
        {"avg", TT_FUNCTION, F_ARI_MEAN},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'arithmeticMean' (aka mean / average / avg)
        {"avgdev", TT_FUNCTION, F_AVG_DEV},
        {"backcolor", TT_PROPERTY, P_BACK_COLOR},
        {"backdrop", TT_PROPERTY, P_BACK_DROP},
        {"background", TT_CHUNK, CT_BACKGROUND},
        {"backgroundbehavior", TT_PROPERTY, P_BACKGROUND_BEHAVIOR},
        {"backgroundcolor", TT_PROPERTY, P_BACK_COLOR},
        {"backgroundids", TT_PROPERTY, P_BACKGROUND_IDS},
        {"backgroundnames", TT_PROPERTY, P_BACKGROUND_NAMES},
        {"backgroundpattern", TT_PROPERTY, P_BACK_PATTERN},
        {"backgroundpixel", TT_PROPERTY, P_BACK_PIXEL},
        {"backgrounds", TT_CLASS, CT_BACKGROUND},
        {"backlist", TT_PROPERTY, P_RECENT_NAMES},
        {"backpattern", TT_PROPERTY, P_BACK_PATTERN},
        {"backpixel", TT_PROPERTY, P_BACK_PIXEL},
        {"backscripts", TT_FUNCTION, F_BACK_SCRIPTS},
        {"backsize", TT_PROPERTY, P_BACK_SIZE},
        {"base64decode", TT_FUNCTION, F_BASE64_DECODE},
        {"base64encode", TT_FUNCTION, F_BASE64_ENCODE},
        {"baseconvert", TT_FUNCTION, F_BASE_CONVERT},
        {"beepduration", TT_PROPERTY, P_BEEP_DURATION},
        {"beeploudness", TT_PROPERTY, P_BEEP_LOUDNESS},
        {"beeppitch", TT_PROPERTY, P_BEEP_PITCH},
		{"beepsound", TT_PROPERTY, P_BEEP_SOUND},
        {"before", TT_PREP, PT_BEFORE},
		{"begins", TT_BINOP, O_BEGINS_WITH},
		{"behavior", TT_PROPERTY, P_PARENT_SCRIPT},
        {"bg", TT_CHUNK, CT_BACKGROUND},
        {"bgbehavior", TT_PROPERTY, P_BACKGROUND_BEHAVIOR},
        {"bgs", TT_CLASS, CT_BACKGROUND},
        // AL-2014-10-17: [[ BiDi ]] Returns the result of applying the bi-directional algorithm to text
        {"bididirection", TT_FUNCTION, F_BIDI_DIRECTION},
        {"binarydecode", TT_FUNCTION, F_BINARY_DECODE},
        {"binaryencode", TT_FUNCTION, F_BINARY_ENCODE},
        {"bitand", TT_BINOP, O_AND_BITS},
        {"bitnot", TT_UNOP, O_NOT_BITS},
        {"bitor", TT_BINOP, O_OR_BITS},
        {"bitxor", TT_BINOP, O_XOR_BITS},
        {"bkgnd", TT_CHUNK, CT_BACKGROUND},
        {"bkgnds", TT_CLASS, CT_BACKGROUND},
        {"blendlevel", TT_PROPERTY, P_BLEND_LEVEL},
        {"blindtyping", TT_PROPERTY, P_BLIND_TYPING},
        {"blinkrate", TT_PROPERTY, P_BLINK_RATE},
        {"bordercolor", TT_PROPERTY, P_BORDER_COLOR},
        {"borderpattern", TT_PROPERTY, P_BORDER_PATTERN},
        {"borderwidth", TT_PROPERTY, P_BORDER_WIDTH},
        {"botleft", TT_PROPERTY, P_BOTTOM_LEFT},
        {"botright", TT_PROPERTY, P_BOTTOM_RIGHT},
        {"bottom", TT_PROPERTY, P_BOTTOM},
        {"bottomcolor", TT_PROPERTY, P_BOTTOM_COLOR},
        {"bottomleft", TT_PROPERTY, P_BOTTOM_LEFT},
        {"bottommargin", TT_PROPERTY, P_BOTTOM_MARGIN},
        {"bottompattern", TT_PROPERTY, P_BOTTOM_PATTERN},
        {"bottompixel", TT_PROPERTY, P_BOTTOM_PIXEL},
        {"bottomright", TT_PROPERTY, P_BOTTOM_RIGHT},
        {"boundingbox", TT_PROPERTY, P_BOUNDING_RECT},
        {"boundingrect", TT_PROPERTY, P_BOUNDING_RECT},
        {"breakpoints", TT_PROPERTY, P_BREAK_POINTS},
        {"brush", TT_PROPERTY, P_BRUSH},
        {"brushcolor", TT_PROPERTY, P_BRUSH_COLOR},
        {"brushpattern", TT_PROPERTY, P_BRUSH_PATTERN},
        {"btn", TT_CHUNK, CT_BUTTON},
        {"btns", TT_CLASS, CT_BUTTON},
        {"bufferhiddenimages", TT_PROPERTY, P_BUFFER_IMAGES},
        {"buffermode", TT_PROPERTY, P_BUFFER_MODE},
        {"buildnumber", TT_FUNCTION, F_BUILD_NUMBER},
        {"button", TT_CHUNK, CT_BUTTON},
        {"buttons", TT_CLASS, CT_BUTTON},
        {"by", TT_PREP, PT_BY},
		{"byte", TT_CHUNK, CT_BYTE},
        {"byteoffset", TT_FUNCTION, F_BYTE_OFFSET},
		{"bytes", TT_CLASS, CT_BYTE},
		{"bytetonum", TT_FUNCTION, F_BYTE_TO_NUM},
        {"cachedurl", TT_FUNCTION, F_CACHED_URLS},
        {"cachedurls", TT_FUNCTION, F_CACHED_URLS},
        {"callbacks", TT_PROPERTY, P_CALLBACKS},
        {"cantabort", TT_PROPERTY, P_CANT_ABORT},
        {"cantdelete", TT_PROPERTY, P_CANT_DELETE},
        {"cantmodify", TT_PROPERTY, P_CANT_MODIFY},
        {"cantselect", TT_PROPERTY, P_CANT_SELECT},
        {"capslockkey", TT_FUNCTION, F_CAPS_LOCK_KEY},
		{"capstyle", TT_PROPERTY, P_CAP_STYLE},
        {"card", TT_CHUNK, CT_CARD},
        {"cardids", TT_PROPERTY, P_CARD_IDS},
        {"cardnames", TT_PROPERTY, P_CARD_NAMES},
        {"cards", TT_CLASS, CT_CARD},
        {"casesensitive", TT_PROPERTY, P_CASE_SENSITIVE},
        {"cd", TT_CHUNK, CT_CARD},
        {"cds", TT_CLASS, CT_CARD},
		// MDW-2014-08-23 : [[ feature_floor ]]
        {"ceil", TT_FUNCTION, F_CEIL},
        {"ceiling", TT_FUNCTION, F_CEIL},
        {"centered", TT_PROPERTY, P_CENTERED},
        {"centerrect", TT_PROPERTY, P_CENTER_RECTANGLE},
        {"centerrectangle", TT_PROPERTY, P_CENTER_RECTANGLE},
        {"centurycutoff", TT_PROPERTY, P_CENTURY_CUTOFF},
        {"char", TT_CHUNK, CT_CHARACTER},
        {"character", TT_CHUNK, CT_CHARACTER},
        {"characters", TT_CLASS, CT_CHARACTER},
		{"charindex", TT_PROPERTY, P_CHAR_INDEX},
        {"chars", TT_CLASS, CT_CHARACTER},
        {"charset", TT_PROPERTY, P_CHARSET},
        {"chartonum", TT_FUNCTION, F_CHAR_TO_NUM},
        {"checkmark", TT_PROPERTY, P_CHECK_MARK},
        {"childcontrolids", TT_PROPERTY, P_CHILD_CONTROL_IDS},
        {"childcontrolnames", TT_PROPERTY, P_CHILD_CONTROL_NAMES},
        {"ciphernames", TT_FUNCTION, F_CIPHER_NAMES},
        {"clickchar", TT_FUNCTION, F_CLICK_CHAR},
        {"clickcharchunk", TT_FUNCTION, F_CLICK_CHAR_CHUNK},
        {"clickchunk", TT_FUNCTION, F_CLICK_CHUNK},
        {"clickfield", TT_FUNCTION, F_CLICK_FIELD},
        {"clickh", TT_FUNCTION, F_CLICK_H},
        {"clickline", TT_FUNCTION, F_CLICK_LINE},
        {"clickloc", TT_FUNCTION, F_CLICK_LOC},
        {"clickstack", TT_FUNCTION, F_CLICK_STACK},
        {"clicktext", TT_FUNCTION, F_CLICK_TEXT},
        {"clickv", TT_FUNCTION, F_CLICK_V},
        {"clipboard", TT_FUNCTION, F_CLIPBOARD},
        {"clipboarddata", TT_PROPERTY, P_CLIPBOARD_DATA},
        // MERG-2013-08-12: [[ ClipsToRect ]] If true group clips to the set rect rather than the rect of children
        {"clipstorect", TT_PROPERTY, P_CLIPS_TO_RECT},
        {"closebox", TT_PROPERTY, P_CLOSE_BOX},
        {"cmdargs", TT_FUNCTION, F_COMMAND_ARGUMENTS},
        {"cmdkey", TT_FUNCTION, F_COMMAND_KEY},
        {"cmdname", TT_FUNCTION, F_COMMAND_NAME},
        {"codepoint", TT_CHUNK, CT_CODEPOINT},
        {"codepointoffset", TT_FUNCTION, F_CODEPOINT_OFFSET},
        {"codepointproperty", TT_FUNCTION, F_CODEPOINT_PROPERTY},
		{"codepoints", TT_CLASS, CT_CODEPOINT},
        {"codepointtonum", TT_FUNCTION, F_UNICODE_CHAR_TO_NUM},
        {"codeunit", TT_CHUNK, CT_CODEUNIT},
        {"codeunitoffset", TT_FUNCTION, F_CODEUNIT_OFFSET},
        {"codeunits", TT_CLASS, CT_CODEUNIT},
        {"collapsebox", TT_PROPERTY, P_COLLAPSE_BOX},
		// MERG-2013-08-17: [[ ColorDialogColors ]] Custom color management for the windows color dialog
		{"colordialogcolors", TT_PROPERTY, P_COLOR_DIALOG_COLORS},
        {"colormap", TT_PROPERTY, P_COLORMAP},
        {"colornames", TT_FUNCTION, F_COLOR_NAMES},
		{"coloroverlay", TT_PROPERTY, P_BITMAP_EFFECT_COLOR_OVERLAY},
        {"colorpalette", TT_CHUNK, CT_COLOR_PALETTE},
        {"colors", TT_PROPERTY, P_COLORS},
        {"colorworld", TT_PROPERTY, P_COLOR_WORLD},
        {"columndel", TT_PROPERTY, P_COLUMN_DELIMITER},
        {"columndelimiter", TT_PROPERTY, P_COLUMN_DELIMITER},
        // SN-2015-07-18: [[ CommandFunctions ]] Added keywords for
        //  commandName and commandArguments
        {"commandarguments", TT_FUNCTION, F_COMMAND_ARGUMENTS},
        {"commandchar", TT_PROPERTY, P_COMMAND_CHAR},
        {"commandkey", TT_FUNCTION, F_COMMAND_KEY},
        {"commandname", TT_FUNCTION, F_COMMAND_NAME},
        {"commandnames", TT_FUNCTION, F_COMMAND_NAMES},
        // MW-2011-09-10: [[ TileCache ]] The maximum number of bytes to use for the tile cache
		{"compositorcachelimit", TT_PROPERTY, P_COMPOSITOR_CACHE_LIMIT},
		// MW-2011-09-10: [[ TileCache ]] Read-only statistics about recent composites
		{"compositorstatistics", TT_PROPERTY, P_COMPOSITOR_STATISTICS},
		// MW-2011-09-10: [[ TileCache ]] The size of tile to use
		{"compositortilesize", TT_PROPERTY, P_COMPOSITOR_TILE_SIZE},
		// MW-2011-09-10: [[ TileCache ]] The type of compositor to use
		{"compositortype", TT_PROPERTY, P_COMPOSITOR_TYPE},
        {"compound", TT_FUNCTION, F_COMPOUND},
        {"compress", TT_FUNCTION, F_COMPRESS},
        {"constantmask", TT_PROPERTY, P_CONSTANT_MASK},
        {"constantnames", TT_FUNCTION, F_CONSTANT_NAMES},
        {"constraints", TT_PROPERTY, P_CONSTRAINTS},
        {"contains", TT_BINOP, O_CONTAINS},
        {"control", TT_CHUNK, CT_LAYER},
		// MW-2012-10-08: [[ HitTest ]] New functions for determining the control at a point.
		{"controlatloc", TT_FUNCTION, F_CONTROL_AT_LOC},
		{"controlatscreenloc", TT_FUNCTION, F_CONTROL_AT_SCREEN_LOC},
        {"controlids", TT_PROPERTY, P_CONTROL_IDS},
        {"controlkey", TT_FUNCTION, F_CONTROL_KEY},
        {"controlnames", TT_PROPERTY, P_CONTROL_NAMES},
        {"controls", TT_CLASS, CT_LAYER},
        {"convertoctals", TT_PROPERTY, P_CONVERT_OCTALS},
        {"copyresource", TT_FUNCTION, F_COPY_RESOURCE},
        {"cos", TT_FUNCTION, F_COS},
        {"ctrlkey", TT_FUNCTION, F_CONTROL_KEY},
        {"current", TT_CHUNK, CT_THIS},
		{"currentcard", TT_PROPERTY, P_CURRENT_CARD},
        {"currentframe", TT_PROPERTY, P_CURRENT_FRAME},
        {"currentnode", TT_PROPERTY, P_CURRENT_NODE},
        {"currentpage", TT_PROPERTY, P_CURRENT_PAGE},
        {"currenttime", TT_PROPERTY, P_CURRENT_TIME},
        {"currentwindow", TT_FUNCTION, F_TOP_STACK},
        {"cursor", TT_PROPERTY, P_CURSOR},
        {"cursorMovement", TT_PROPERTY, P_CURSORMOVEMENT},
        {"customkeys", TT_PROPERTY, P_CUSTOM_KEYS},
        {"customproperties", TT_PROPERTY, P_CUSTOM_PROPERTIES},
        {"custompropertyset", TT_PROPERTY, P_CUSTOM_PROPERTY_SET},
        {"custompropertysets", TT_PROPERTY, P_CUSTOM_PROPERTY_SETS},
        {"dashes", TT_PROPERTY, P_DASHES},
        {"date", TT_FUNCTION, F_DATE},
        {"dateformat", TT_FUNCTION, F_DATE_FORMAT},
        {"debugcontext", TT_PROPERTY, P_DEBUG_CONTEXT},
        {"decompress", TT_FUNCTION, F_DECOMPRESS},
        {"decorations", TT_PROPERTY, P_DECORATIONS},
        {"default", TT_PROPERTY, P_DEFAULT},
        {"defaultbutton", TT_PROPERTY, P_DEFAULT_BUTTON},
        {"defaultcursor", TT_PROPERTY, P_DEFAULT_CURSOR},
        {"defaultfolder", TT_PROPERTY, P_DIRECTORY},
        {"defaultmenubar", TT_PROPERTY, P_DEFAULT_MENU_BAR},
		{"defaultnetworkinterface", TT_PROPERTY, P_DEFAULT_NETWORK_INTERFACE},
        {"defaultstack", TT_PROPERTY, P_DEFAULT_STACK},
		// MW-2011-11-24: [[ UpdateScreen ]] Property controlling whether screen updates are coalesced.
		{"deferscreenupdates", TT_PROPERTY, P_DEFER_SCREEN_UPDATES},
        {"deleteregistry", TT_FUNCTION, F_DELETE_REGISTRY},
        {"deleteresource", TT_FUNCTION, F_DELETE_RESOURCE},
		// MW-2011-11-24: [[ Nice Folders ]] The adjective for 'the desktop folder'.
		{"desktop", TT_PROPERTY, P_DESKTOP_FOLDER},
        {"destroystack", TT_PROPERTY, P_DESTROY_STACK},
        {"destroywindow", TT_PROPERTY, P_DESTROY_WINDOW},
        {"detailed", TT_PROPERTY, P_LONG},
        {"dialogdata", TT_PROPERTY, P_DIALOG_DATA},
        {"directories", TT_FUNCTION, F_DIRECTORIES},
        {"directory", TT_PROPERTY, P_DIRECTORY},
        {"disabled", TT_PROPERTY, P_DISABLED},
        {"disabledicon", TT_PROPERTY, P_DISABLED_ICON},
        {"diskspace", TT_FUNCTION, F_DISK_SPACE},
        {"div", TT_BINOP, O_DIV},
        {"dnsservers", TT_FUNCTION, F_DNS_SERVERS},
		{"document", TT_CHUNK, CT_DOCUMENT},
        // MERG-2015-10-11: [[ DocumentFilename ]] Property tag for documentFilename
        {"documentfilename", TT_PROPERTY, P_DOCUMENT_FILENAME},
        // MW-2011-11-24: [[ Nice Folders ]] The adjective for 'the documents folder'.
		{"documents", TT_PROPERTY, P_DOCUMENTS_FOLDER},
        {"dontdither", TT_PROPERTY, P_DONT_DITHER},
        {"dontrefresh", TT_PROPERTY, P_DONT_REFRESH},
        {"dontresize", TT_PROPERTY, P_DONT_RESIZE},
        {"dontsearch", TT_PROPERTY, P_DONT_SEARCH},
        {"dontusens", TT_PROPERTY, P_DONT_USE_NS},
        {"dontuseqt", TT_PROPERTY, P_DONT_USE_QT},
        {"dontuseqteffects", TT_PROPERTY, P_DONT_USE_QT_EFFECTS},
        {"dontwrap", TT_PROPERTY, P_DONT_WRAP},
        {"doubleclickdelta", TT_PROPERTY, P_DOUBLE_DELTA},
        {"doubleclickinterval", TT_PROPERTY, P_DOUBLE_TIME},
		{"dragaction", TT_PROPERTY, P_DRAG_ACTION},
        {"dragdata", TT_PROPERTY, P_DRAG_DATA},
		{"dragdelta", TT_PROPERTY, P_DRAG_DELTA},
        {"dragdestination", TT_FUNCTION, F_DRAG_DESTINATION},
        {"draggable", TT_PROPERTY, P_DRAGGABLE},
		{"dragimage", TT_PROPERTY, P_DRAG_IMAGE},
		{"dragimageoffset", TT_PROPERTY, P_DRAG_IMAGE_OFFSET},
        {"dragsource", TT_FUNCTION, F_DRAG_SOURCE},
        {"dragspeed", TT_PROPERTY, P_DRAG_SPEED},
        {"drivernames", TT_FUNCTION, F_DRIVER_NAMES},
        {"drives", TT_FUNCTION, F_DRIVES},
        {"dropchunk", TT_FUNCTION, F_DROP_CHUNK},
		{"dropshadow", TT_PROPERTY, P_BITMAP_EFFECT_DROP_SHADOW},
        {"duration", TT_PROPERTY, P_DURATION},
        {"dynamicpaths", TT_PROPERTY, P_DYNAMIC_PATHS},
        {"editbackground", TT_PROPERTY, P_EDIT_BACKGROUND},
        {"editbg", TT_PROPERTY, P_EDIT_BACKGROUND},
        {"editbkgnd", TT_PROPERTY, P_EDIT_BACKGROUND},
        // MW-2014-08-12: [[ EditionType ]] New read-only property describing the engine's edition.
        {"editiontype", TT_PROPERTY, P_EDITION_TYPE},
        {"editmenus", TT_PROPERTY, P_EDIT_MENUS},
		{"editmode", TT_PROPERTY, P_EDIT_MODE},
        {"editscripts", TT_PROPERTY, P_EDIT_SCRIPTS},
        {"effective", TT_PROPERTY, P_EFFECTIVE},
        {"effectrate", TT_PROPERTY, P_EFFECT_RATE},
        {"eighth", TT_CHUNK, CT_EIGHTH},
        {"eighthcolor", TT_PROPERTY, P_FOCUS_COLOR},
        {"eighthpixel", TT_PROPERTY, P_FOCUS_PIXEL},
		{"element", TT_CHUNK, CT_ELEMENT},
		{"elements", TT_CLASS, CT_ELEMENT},
        {"emacskeybindings", TT_PROPERTY, P_EMACS_KEY_BINDINGS},
        {"enabled", TT_PROPERTY, P_ENABLED},
        {"enabledtracks", TT_PROPERTY, P_ENABLED_TRACKS},
		// MW-2012-02-12: [[ Encoding ]] New read-only property describing a run's text encoding.
		{"encoding", TT_PROPERTY, P_ENCODING},
        {"endarrow", TT_PROPERTY, P_END_ARROW},
        {"endframe", TT_PROPERTY, P_END_FRAME},
		{"ends", TT_BINOP, O_ENDS_WITH},
        {"endtime", TT_PROPERTY, P_END_TIME},
        {"endvalue", TT_PROPERTY, P_END_VALUE},
		// MW-2011-11-24: [[ Nice Folders ]] The adjective for 'the engine folder'.
		{"engine", TT_PROPERTY, P_ENGINE_FOLDER},
        {"english", TT_PROPERTY, P_ENGLISH},
        {"environment", TT_FUNCTION, F_ENVIRONMENT},
        {"eps", TT_CHUNK, CT_EPS},
        {"epss", TT_CLASS, CT_EPS},
        {"eraser", TT_PROPERTY, P_ERASER},
		{"errormode", TT_PROPERTY, P_ERROR_MODE},
        {"eventaltkey", TT_FUNCTION, F_EVENT_OPTION_KEY},
        {"eventcapslockkey", TT_FUNCTION, F_EVENT_CAPSLOCK_KEY},
        {"eventcommandkey", TT_FUNCTION, F_EVENT_COMMAND_KEY},
        {"eventcontrolkey", TT_FUNCTION, F_EVENT_CONTROL_KEY},
        {"eventoptionkey", TT_FUNCTION, F_EVENT_OPTION_KEY},
        {"eventshiftkey", TT_FUNCTION, F_EVENT_SHIFT_KEY},
        {"executioncontexts", TT_PROPERTY, P_EXECUTION_CONTEXTS},
        {"existence", TT_FUNCTION, F_EXISTS},
        {"exists", TT_FUNCTION, F_EXISTS},
        {"exp", TT_FUNCTION, F_EXP},
        {"exp1", TT_FUNCTION, F_EXP1},
        {"exp10", TT_FUNCTION, F_EXP10},
        {"exp2", TT_FUNCTION, F_EXP2},
        {"explicitvariables", TT_PROPERTY, P_EXPLICIT_VARIABLES},
        {"explicitvars", TT_PROPERTY, P_EXPLICIT_VARIABLES},
        {"extendkey", TT_PROPERTY, P_EXTEND_KEY},
        {"extents", TT_FUNCTION, F_EXTENTS},
        {"externalcommands", TT_PROPERTY, P_EXTERNAL_COMMANDS},
        {"externalfunctions", TT_PROPERTY, P_EXTERNAL_FUNCTIONS},
        {"externalpackages", TT_PROPERTY, P_EXTERNAL_PACKAGES},
        {"externals", TT_PROPERTY, P_EXTERNALS},
        {"family", TT_PROPERTY, P_FAMILY},
        {"field", TT_CHUNK, CT_FIELD},
        {"fields", TT_CLASS, CT_FIELD},
        {"fifth", TT_CHUNK, CT_FIFTH},
        {"fifthcolor", TT_PROPERTY, P_TOP_COLOR},
        {"fifthpixel", TT_PROPERTY, P_TOP_PIXEL},
        {"filename", TT_PROPERTY, P_FILE_NAME},
        {"files", TT_FUNCTION, F_FILES},
        {"filetype", TT_PROPERTY, P_FILE_TYPE},
        {"fillback", TT_PROPERTY, P_BRUSH_BACK_COLOR},
        {"filled", TT_PROPERTY, P_FILLED},
        {"fillfore", TT_PROPERTY, P_BRUSH_COLOR},
		{"fillgradient", TT_PROPERTY, P_GRADIENT_FILL},
        {"fillpat", TT_PROPERTY, P_BRUSH_PATTERN},
		{"fillrule", TT_PROPERTY, P_FILL_RULE}, // PROPERTY - FILL RULE
        {"first", TT_CHUNK, CT_FIRST},
        {"firstcolor", TT_PROPERTY, P_FORE_COLOR},
        {"firstindent", TT_PROPERTY, P_FIRST_INDENT},
        {"firstpixel", TT_PROPERTY, P_FORE_PIXEL},
        {"fixedlineheight", TT_PROPERTY, P_FIXED_HEIGHT},
		// MW-2012-01-26: [[ FlaggedField ]] New property for char-level 'flagged'.
		{"flagged", TT_PROPERTY, P_FLAGGED},
		// MW-2012-02-08: [[ FlaggedField ]] New property for 'flaggedRanges'.
		{"flaggedranges", TT_PROPERTY, P_FLAGGED_RANGES},
        {"fld", TT_CHUNK, CT_FIELD},
        {"flds", TT_CLASS, CT_FIELD},
		// MDW-2014-08-23 : [[ feature_floor ]]
        {"floor", TT_FUNCTION, F_FLOOR},
        {"flushevents", TT_FUNCTION, F_FLUSH_EVENTS},
        {"focuscolor", TT_PROPERTY, P_FOCUS_COLOR},
        {"focusedobject", TT_FUNCTION, F_FOCUSED_OBJECT},
        {"focuspattern", TT_PROPERTY, P_FOCUS_PATTERN},
        {"focuspixel", TT_PROPERTY, P_FOCUS_PIXEL},
        {"folder", TT_PROPERTY, P_DIRECTORY},
        {"folders", TT_FUNCTION, F_DIRECTORIES},
        // TD-2013-06-20: [[ DynamicFonts ]] global property for list of font files
        {"fontfilesinuse", TT_PROPERTY, P_FONTFILES_IN_USE},
        {"fontlanguage", TT_FUNCTION, F_FONT_LANGUAGE},
        {"fontnames", TT_FUNCTION, F_FONT_NAMES},
        {"fontsizes", TT_FUNCTION, F_FONT_SIZES},
        {"fontstyles", TT_FUNCTION, F_FONT_STYLES},		
        {"forecolor", TT_PROPERTY, P_FORE_COLOR},
        {"foregroundcolor", TT_PROPERTY, P_FORE_COLOR},
        {"foregroundpattern", TT_PROPERTY, P_FORE_PATTERN},
        {"foregroundpixel", TT_PROPERTY, P_FORE_PIXEL},
        {"forepattern", TT_PROPERTY, P_FORE_PATTERN},
        {"forepixel", TT_PROPERTY, P_FORE_PIXEL},
        {"format", TT_FUNCTION, F_FORMAT},
        {"formatforprinting", TT_PROPERTY, P_FORMAT_FOR_PRINTING},
        {"formattedheight", TT_PROPERTY, P_FORMATTED_HEIGHT},
        {"formattedleft", TT_PROPERTY, P_FORMATTED_LEFT},
        {"formattedrect", TT_PROPERTY, P_FORMATTED_RECT},
		// MW-2012-02-21: [[ LineBreak ]] New property for formattedStyledText
		{"formattedstyledtext", TT_PROPERTY, P_FORMATTED_STYLED_TEXT},
        {"formattedtext", TT_PROPERTY, P_FORMATTED_TEXT},
        {"formattedtop", TT_PROPERTY, P_FORMATTED_TOP},
        {"formattedwidth", TT_PROPERTY, P_FORMATTED_WIDTH},
        {"formsensitive", TT_PROPERTY, P_FORM_SENSITIVE},
        {"foundchunk", TT_FUNCTION, F_FOUND_CHUNK},
        {"foundfield", TT_FUNCTION, F_FOUND_FIELD},
        {"foundline", TT_FUNCTION, F_FOUND_LINE},
        {"foundloc", TT_FUNCTION, F_FOUND_LOC},
        {"foundtext", TT_FUNCTION, F_FOUND_TEXT},
        {"fourth", TT_CHUNK, CT_FOURTH},
        {"fourthcolor", TT_PROPERTY, P_BORDER_COLOR},
        {"fourthpixel", TT_PROPERTY, P_BORDER_PIXEL},
        {"framecount", TT_PROPERTY, P_FRAME_COUNT},
        {"framerate", TT_PROPERTY, P_FRAME_RATE},
        {"freesize", TT_PROPERTY, P_FREE_SIZE},
        {"from", TT_FROM, PT_FROM},
        {"frontscripts", TT_FUNCTION, F_FRONT_SCRIPTS},
        {"ftpproxy", TT_PROPERTY, P_FTP_PROXY},
        {"fullclipboarddata", TT_PROPERTY, P_FULL_CLIPBOARD_DATA},
        {"fulldragdata", TT_PROPERTY, P_FULL_DRAGBOARD_DATA},
		{"fullscreen", TT_PROPERTY, P_FULLSCREEN},
		// IM-2013-09-23: [[ FullscreenMode ]] New property for 'fullscreenmode'
		{"fullscreenmode", TT_PROPERTY, P_FULLSCREENMODE},
        {"functionnames", TT_FUNCTION, F_FUNCTION_NAMES},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'geometricMean'
        {"geometricmean", TT_FUNCTION, F_GEO_MEAN},
        {"getresource", TT_FUNCTION, F_GET_RESOURCE},
        {"getresources", TT_FUNCTION, F_GET_RESOURCES},
        {"globalloc", TT_FUNCTION, F_GLOBAL_LOC},
        {"globalnames", TT_FUNCTION, F_GLOBALS},
        {"globals", TT_FUNCTION, F_GLOBALS},
		{"graphic", TT_CHUNK, CT_GRAPHIC},
        {"graphics", TT_CLASS, CT_GRAPHIC},
        {"grc", TT_CHUNK, CT_GRAPHIC},
        {"grcs", TT_CLASS, CT_GRAPHIC},
        {"grid", TT_PROPERTY, P_GRID},
        {"gridsize", TT_PROPERTY, P_GRID_SIZE},
        {"group", TT_CHUNK, CT_GROUP},
        {"groupids", TT_PROPERTY, P_GROUP_IDS},
        {"groupnames", TT_PROPERTY, P_GROUP_NAMES},
        {"groups", TT_CLASS, CT_GROUP},
        {"grp", TT_CHUNK, CT_GROUP},
        {"grps", TT_CLASS, CT_GROUP},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'harmonicMean'
        {"harmonicmean", TT_FUNCTION, F_HAR_MEAN},
        {"hasmemory", TT_FUNCTION, F_HAS_MEMORY},
        {"hcaddressing", TT_PROPERTY, P_HC_ADDRESSING},
        {"hcimportstat", TT_PROPERTY, P_HC_IMPORT_STAT},
        {"hcstack", TT_PROPERTY, P_HC_STACK},
        {"heapspace", TT_FUNCTION, F_HEAP_SPACE},
        {"height", TT_PROPERTY, P_HEIGHT},
        {"hgrid", TT_PROPERTY, P_HGRID},
		// MW-2012-03-05: [[ HiddenText ]] Synonym for 'invisible' - preferred for the hidden
		//   paragraph property.
		{"hidden", TT_PROPERTY, P_INVISIBLE},
		{"hidebackdrop", TT_PROPERTY, P_HIDE_BACKDROP},
        {"hideconsolewindows", TT_PROPERTY, P_HIDE_CONSOLE_WINDOWS},
        {"hidepalettes", TT_PROPERTY, P_HIDE_PALETTES},
        {"highlight", TT_PROPERTY, P_HILITE},
        {"highlighted", TT_PROPERTY, P_HILITE},
        {"highlite", TT_PROPERTY, P_HILITE},
        {"highlited", TT_PROPERTY, P_HILITE},
        {"hilight", TT_PROPERTY, P_HILITE},
        {"hilighted", TT_PROPERTY, P_HILITE},
        {"hilite", TT_PROPERTY, P_HILITE},
        {"hiliteborder", TT_PROPERTY, P_HILITE_BORDER},
        {"hilitecolor", TT_PROPERTY, P_HILITE_COLOR},
        {"hilited", TT_PROPERTY, P_HILITE},
        {"hilitedbutton", TT_PROPERTY, P_HILITED_BUTTON},
        {"hilitedbuttonid", TT_PROPERTY, P_HILITED_BUTTON_ID},
        {"hilitedbuttonname", TT_PROPERTY, P_HILITED_BUTTON_NAME},
        {"hilitedicon", TT_PROPERTY, P_HILITED_ICON},
        {"hilitedline", TT_PROPERTY, P_HILITED_LINES},
        {"hilitedlines", TT_PROPERTY, P_HILITED_LINES},
        {"hilitedtext", TT_FUNCTION, F_SELECTED_TEXT},
        {"hilitefill", TT_PROPERTY, P_HILITE_FILL},
        {"hiliteicon", TT_PROPERTY, P_HILITED_ICON},
        {"hilitepattern", TT_PROPERTY, P_HILITE_PATTERN},
        {"hilitepixel", TT_PROPERTY, P_HILITE_PIXEL},
		// MW-2011-11-24: [[ Nice Folders ]] The adjective for 'the home folder'.
		{"home", TT_PROPERTY, P_HOME_FOLDER},
        {"hostaddress", TT_FUNCTION, F_HA},
        {"hostaddresstoname", TT_FUNCTION, F_HATON},
        {"hostname", TT_FUNCTION, F_HN},
        {"hostnametoaddress", TT_FUNCTION, F_HNTOA},
        {"hotspot", TT_PROPERTY, P_HOT_SPOT},
        {"hotspots", TT_PROPERTY, P_HOT_SPOTS},
		{"hovericon", TT_PROPERTY, P_HOVER_ICON},
        {"hscroll", TT_PROPERTY, P_HSCROLL},
        {"hscrollbar", TT_PROPERTY, P_HSCROLLBAR},
        {"htmltext", TT_PROPERTY, P_HTML_TEXT},
        {"httpheaders", TT_PROPERTY, P_HTTP_HEADERS},
        {"httpproxy", TT_PROPERTY, P_HTTP_PROXY},
		{"httpproxyforurl", TT_FUNCTION, F_HTTP_PROXY_FOR_URL},
        {"icon", TT_PROPERTY, P_ICON},
        // MW-2014-06-19: [[ IconGravity ]] Button 'iconGravity' property.
        {"icongravity", TT_PROPERTY, P_ICON_GRAVITY},
        {"iconic", TT_PROPERTY, P_ICONIC},
		{"iconmenu", TT_PROPERTY, P_ICON_MENU},
        {"id", TT_PROPERTY, P_ID},
        {"idlerate", TT_PROPERTY, P_IDLE_RATE},
        {"idleticks", TT_PROPERTY, P_IDLE_TICKS},
        // MERG-2014-06-02: [[ IgnoreMouseEvents ]] ignoreMouseEvents stack property
        {"ignoremouseevents", TT_PROPERTY, P_IGNORE_MOUSE_EVENTS},
        {"image", TT_CHUNK, CT_IMAGE},
		{"imagecachelimit", TT_PROPERTY, P_IMAGE_CACHE_LIMIT},
		{"imagecacheusage", TT_PROPERTY, P_IMAGE_CACHE_USAGE},
        {"imagedata", TT_PROPERTY, P_IMAGE_DATA},
        {"imagepixmapid", TT_PROPERTY, P_IMAGE_PIXMAP_ID},
        {"images", TT_CLASS, CT_IMAGE},
        {"imagesource", TT_PROPERTY, P_IMAGE_SOURCE},
        {"img", TT_CHUNK, CT_IMAGE},
        {"imgs", TT_CLASS, CT_IMAGE},
        {"in", TT_IN, PT_IN},
        {"ink", TT_PROPERTY, P_INK},
		{"innerglow", TT_PROPERTY, P_BITMAP_EFFECT_INNER_GLOW},
		{"innershadow", TT_PROPERTY, P_BITMAP_EFFECT_INNER_SHADOW},
        {"internet", TT_PROPERTY, P_INTERNET},
        {"interrupt", TT_FUNCTION, F_INTERRUPT},
        {"intersect", TT_FUNCTION, F_INTERSECT},
        {"into", TT_PREP, PT_INTO},
        {"inv", TT_PROPERTY, P_INVISIBLE},
        {"invisible", TT_PROPERTY, P_INVISIBLE},
        {"is", TT_BINOP, O_IS},
        {"isnumber", TT_FUNCTION, F_IS_NUMBER},
        {"isotomac", TT_FUNCTION, F_ISO_TO_MAC},
        {"item", TT_CHUNK, CT_ITEM},
        {"itemdel", TT_PROPERTY, P_ITEM_DELIMITER},
        {"itemdelimiter", TT_PROPERTY, P_ITEM_DELIMITER},
        {"itemoffset", TT_FUNCTION, F_ITEM_OFFSET},
        {"items", TT_CLASS, CT_ITEM},
		{"joinstyle", TT_PROPERTY, P_JOIN_STYLE},
        {"jpegquality", TT_PROPERTY, P_JPEG_QUALITY},
        {"keyboardtype", TT_PROPERTY, P_KEYBOARD_TYPE},
        {"keys", TT_FUNCTION, F_KEYS},
        {"keysdown", TT_FUNCTION, F_KEYS_DOWN},
        {"kind", TT_PROPERTY, P_KIND},
        {"label", TT_PROPERTY, P_LABEL},
        {"labelwidth", TT_PROPERTY, P_LABEL_WIDTH},
        {"last", TT_CHUNK, CT_LAST},
        {"layer", TT_PROPERTY, P_LAYER},
        {"layercliprect", TT_PROPERTY, P_LAYER_CLIP_RECT},
		// MW-2011-08-25: [[ TileCache ]] The layerMode property token.
		{"layermode", TT_PROPERTY, P_LAYER_MODE},
        {"layers", TT_CLASS, CT_LAYER},
        {"left", TT_PROPERTY, P_LEFT},
		{"leftbalance", TT_PROPERTY, P_LEFT_BALANCE},
		// MW-2011-01-25: [[ ParaStyles ]] The leftIndent paragraph property.
		{"leftindent", TT_PROPERTY, P_LEFT_INDENT},
        {"leftmargin", TT_PROPERTY, P_LEFT_MARGIN},
        {"len", TT_FUNCTION, F_LENGTH},
        {"length", TT_FUNCTION, F_LENGTH},
        {"libraries", TT_PROPERTY, P_STACKS_IN_USE},
        {"licensed", TT_FUNCTION, F_LICENSED},
        {"line", TT_CHUNK, CT_LINE},
        {"linedel", TT_PROPERTY, P_LINE_DELIMITER},
        {"linedelimiter", TT_PROPERTY, P_LINE_DELIMITER},
        {"lineinc", TT_PROPERTY, P_LINE_INC},
        {"lineincrement", TT_PROPERTY, P_LINE_INC},
		{"lineindex", TT_PROPERTY, P_LINE_INDEX},
        {"lineoffset", TT_FUNCTION, F_LINE_OFFSET},
        {"lines", TT_CLASS, CT_LINE},
        {"linesize", TT_PROPERTY, P_LINE_SIZE},
        {"linkcolor", TT_PROPERTY, P_LINK_COLOR},
        {"linkhilitecolor", TT_PROPERTY, P_LINK_HILITE_COLOR},
        {"linktext", TT_PROPERTY, P_LINK_TEXT},
        {"linkvisitedcolor", TT_PROPERTY, P_LINK_VISITED_COLOR},
        {"listbehavior", TT_PROPERTY, P_LIST_BEHAVIOR},
		// MW-2011-01-25: [[ ParaStyles ]] The listDepth paragraph property.
		{"listdepth", TT_PROPERTY, P_LIST_DEPTH},
		// MW-2011-01-25: [[ ParaStyles ]] The listIndent paragraph property.
		{"listindent", TT_PROPERTY, P_LIST_INDENT},
		// MW-2012-11-13: [[ ParaListIndex ]] The listIndex paragraph property.
		{"listindex", TT_PROPERTY, P_LIST_INDEX},
		{"listregistry", TT_FUNCTION, F_LIST_REGISTRY},
		{"liststyle", TT_PROPERTY, P_LIST_STYLE},
        {"liveresizing", TT_PROPERTY, P_LIVE_RESIZING},
        {"ln", TT_FUNCTION, F_LN},
        {"ln1", TT_FUNCTION, F_LN1},
        {"loadedextensions", TT_PROPERTY, P_LOADED_EXTENSIONS},
        {"loadedtime", TT_PROPERTY, P_MOVIE_LOADED_TIME},
        {"loc", TT_PROPERTY, P_LOCATION},
        {"localloc", TT_FUNCTION, F_LOCAL_LOC},
        {"localnames", TT_FUNCTION, F_LOCALS},
        {"location", TT_PROPERTY, P_LOCATION},
        {"lockcolormap", TT_PROPERTY, P_LOCK_COLORMAP},
        {"lockcursor", TT_PROPERTY, P_LOCK_CURSOR},
        {"locked", TT_PROPERTY, P_LOCK_LOCATION},
        {"lockerrordialogs", TT_PROPERTY, P_LOCK_ERRORS},
        {"lockloc", TT_PROPERTY, P_LOCK_LOCATION},
        {"locklocation", TT_PROPERTY, P_LOCK_LOCATION},
        {"lockmenus", TT_PROPERTY, P_LOCK_MENUS},
        {"lockmessages", TT_PROPERTY, P_LOCK_MESSAGES},
        {"lockmoves", TT_PROPERTY, P_LOCK_MOVES},
        {"lockrecent", TT_PROPERTY, P_LOCK_RECENT},
        {"lockscreen", TT_PROPERTY, P_LOCK_SCREEN},
        {"locktext", TT_PROPERTY, P_LOCK_TEXT},
		// MERG-2013-06-02: [[ GrpLckUpdates ]] The lockUpdates group property.
        {"lockupdates", TT_PROPERTY, P_LOCK_UPDATES},
        {"log10", TT_FUNCTION, F_LOG10},
        {"log2", TT_FUNCTION, F_LOG2},
        {"logmessage", TT_PROPERTY, P_LOG_MESSAGE},
        {"long", TT_PROPERTY, P_LONG},
        {"longfilepath", TT_FUNCTION, F_LONG_FILE_PATH},
        {"longwindowtitles", TT_PROPERTY, P_LONG_WINDOW_TITLES},
        {"lookandfeel", TT_PROPERTY, P_LOOK_AND_FEEL},
        {"looping", TT_PROPERTY, P_LOOPING},
        {"lower", TT_FUNCTION, F_TO_LOWER},
        {"lowresolutiontimers", TT_PROPERTY, P_LOW_RESOLUTION_TIMERS},
        {"lzwkey", TT_PROPERTY, P_LZW_KEY},
        {"machine", TT_FUNCTION, F_MACHINE},
        {"mactoiso", TT_FUNCTION, F_MAC_TO_ISO},
        {"magnifier", TT_CHUNK, CT_MAGNIFY},
        {"magnify", TT_PROPERTY, P_MAGNIFY},
        {"mainstack", TT_PROPERTY, P_MAIN_STACK},
        {"mainstacks", TT_FUNCTION, F_MAIN_STACKS},
        {"margins", TT_PROPERTY, P_MARGINS},
        {"mark", TT_PROPERTY, P_MARKED},
        {"markchar", TT_PROPERTY, P_MARK_CHAR},
        {"marked", TT_CLASS, CT_MARKED},
        {"markercolor", TT_PROPERTY, P_HILITE_COLOR},
        {"markerdrawn", TT_PROPERTY, P_MARKER_DRAWN},
        {"markerfillcolor", TT_PROPERTY, P_BORDER_COLOR},
        {"markerfilled", TT_PROPERTY, P_MARKER_OPAQUE},
        {"markerlinesize", TT_PROPERTY, P_MARKER_LSIZE},
        {"markerpattern", TT_PROPERTY, P_HILITE_PATTERN},
        {"markerpoints", TT_PROPERTY, P_MARKER_POINTS},
        {"maskdata", TT_PROPERTY, P_MASK_DATA},
        {"maskpixmapid", TT_PROPERTY, P_MASK_PIXMAP_ID},
        {"matchchunk", TT_FUNCTION, F_MATCH_CHUNK},
        {"matchtext", TT_FUNCTION, F_MATCH_TEXT},
        {"matrixmultiply", TT_FUNCTION, F_MATRIX_MULTIPLY},
        {"max", TT_FUNCTION, F_MAX},
        {"maxheight", TT_PROPERTY, P_MAX_HEIGHT},
        {"maximizebox", TT_PROPERTY, P_MAXIMIZE_BOX},
        {"maxwidth", TT_PROPERTY, P_MAX_WIDTH},
        {"mcencrypt", TT_FUNCTION, F_ENCRYPT},
        {"mcisendstring", TT_FUNCTION, F_MCI_SEND_STRING},
        {"md5digest", TT_FUNCTION, F_MD5_DIGEST},
        {"me", TT_FUNCTION, F_ME},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'arithmeticMean' (aka mean / average / avg)
        {"mean", TT_FUNCTION, F_ARI_MEAN},
        // MERG-2013-08-14: [[ MeasureText ]] Measure text relative to the effective font on an object
        {"measuretext", TT_FUNCTION, F_MEASURE_TEXT},
        {"measureunicodetext", TT_FUNCTION, F_MEASURE_UNICODE_TEXT},
        {"median", TT_FUNCTION, F_MEDIAN},
        {"mediatypes", TT_PROPERTY, P_MEDIA_TYPES},
        {"menu", TT_CHUNK, CT_MENU},
        {"menubar", TT_PROPERTY, P_MENU_BAR},
        {"menubutton", TT_FUNCTION, F_MENU_OBJECT},
        {"menuhistory", TT_PROPERTY, P_MENU_HISTORY},
        {"menuitem", TT_CHUNK, CT_LINE},
        {"menuitems", TT_CLASS, CT_LINE},
        {"menulines", TT_PROPERTY, P_MENU_LINES},
        {"menumessage", TT_PROPERTY, P_MENU_MESSAGE},
        {"menumode", TT_PROPERTY, P_MENU_MODE},
        {"menumousebutton", TT_PROPERTY, P_MENU_BUTTON},
        {"menuname", TT_PROPERTY, P_MENU_NAME},
        {"menuobject", TT_FUNCTION, F_MENU_OBJECT},
        {"menus", TT_FUNCTION, F_MENUS},
        {"merge", TT_FUNCTION, F_MERGE},
        {"messagedigest", TT_FUNCTION, F_MESSAGE_DIGEST},
		{"messagemessages", TT_PROPERTY, P_MESSAGE_MESSAGES},
		{"metadata", TT_PROPERTY, P_METADATA},
        {"metal", TT_PROPERTY, P_METAL},
        {"mid", TT_CHUNK, CT_MIDDLE},
        {"middle", TT_CHUNK, CT_MIDDLE},
        {"millisec", TT_FUNCTION, F_MILLISECS},
        {"millisecond", TT_FUNCTION, F_MILLISECS},
        {"milliseconds", TT_FUNCTION, F_MILLISECS},
        {"millisecs", TT_FUNCTION, F_MILLISECS},
        {"min", TT_FUNCTION, F_MIN},
        {"minheight", TT_PROPERTY, P_MIN_HEIGHT},
        {"minimizebox", TT_PROPERTY, P_MINIMIZE_BOX},
        {"minstackfileversion", TT_PROPERTY, P_MIN_STACK_FILE_VERSION},
        {"minwidth", TT_PROPERTY, P_MIN_WIDTH},
        {"mirrored", TT_PROPERTY, P_MIRRORED},
		{"miterlimit", TT_PROPERTY, P_MITER_LIMIT},
        {"mnemonic", TT_PROPERTY, P_MNEMONIC},
        {"mod", TT_BINOP, O_MOD},
        {"mode", TT_PROPERTY, P_MODE},
		{"modifiedmark", TT_PROPERTY, P_MODIFIED_MARK},
        {"monthnames", TT_FUNCTION, F_MONTH_NAMES},
        {"mouse", TT_FUNCTION, F_MOUSE},
        {"mousechar", TT_FUNCTION, F_MOUSE_CHAR},
        {"mousecharchunk", TT_FUNCTION, F_MOUSE_CHAR_CHUNK},
        {"mousechunk", TT_FUNCTION, F_MOUSE_CHUNK},
        {"mouseclick", TT_FUNCTION, F_MOUSE_CLICK},
        {"mousecolor", TT_FUNCTION, F_MOUSE_COLOR},
        {"mousecontrol", TT_FUNCTION, F_MOUSE_CONTROL},
        {"mouseh", TT_FUNCTION, F_MOUSE_H},
        {"mouseline", TT_FUNCTION, F_MOUSE_LINE},
        {"mouseloc", TT_FUNCTION, F_MOUSE_LOC},
        {"mousestack", TT_FUNCTION, F_MOUSE_STACK},
        {"mousetext", TT_FUNCTION, F_MOUSE_TEXT},
        {"mousev", TT_FUNCTION, F_MOUSE_V},
        {"movespeed", TT_PROPERTY, P_MOVE_SPEED},
        {"movie", TT_FUNCTION, F_MOVIE},
        {"moviecontrollerid", TT_PROPERTY, P_MOVIE_CONTROLLER_ID},
        {"movies", TT_FUNCTION, F_MOVIE},
        {"movingcontrols", TT_FUNCTION, F_MOVING_CONTROLS},
        {"multieffect", TT_PROPERTY, P_MULTI_EFFECT},
        {"multiple", TT_PROPERTY, P_MULTIPLE},
        {"multiplehilites", TT_PROPERTY, P_MULTIPLE_HILITES},
        {"multiplelines", TT_PROPERTY, P_MULTIPLE_HILITES},
        {"multispace", TT_PROPERTY, P_MULTI_SPACE},
        {"name", TT_PROPERTY, P_NAME},
        {"nativechartonum", TT_FUNCTION, F_NATIVE_CHAR_TO_NUM},
        {"navigationarrows", TT_PROPERTY, P_NAVIGATION_ARROWS},
		{"networkinterfaces", TT_PROPERTY, P_NETWORK_INTERFACES},
        {"newest", TT_PREP, PT_NEWEST},
        {"next", TT_CHUNK, CT_NEXT},
        {"ninth", TT_CHUNK, CT_NINTH},
        {"no", TT_UNOP, O_NOT},
        {"nodes", TT_PROPERTY, P_NODES},
        {"noncontiguoushilites", TT_PROPERTY, P_NONCONTIGUOUS_HILITES},
        {"normalizetext", TT_FUNCTION, F_NORMALIZE_TEXT},
        {"not", TT_UNOP, O_NOT},
        {"num", TT_PROPERTY, P_NUMBER},
        {"number", TT_PROPERTY, P_NUMBER},
        {"numberformat", TT_PROPERTY, P_NUMBER_FORMAT},
		{"numtobyte", TT_FUNCTION, F_NUM_TO_BYTE},
        {"numtochar", TT_FUNCTION, F_NUM_TO_CHAR},
        {"numtocodepoint", TT_FUNCTION, F_NUM_TO_UNICODE_CHAR},
        {"numtonativechar", TT_FUNCTION, F_NUM_TO_NATIVE_CHAR},
        {"of", TT_OF, PT_OF},
        {"offset", TT_FUNCTION, F_OFFSET},
        {"on", TT_OF, PT_ON},
        {"onto", TT_PREP, PT_INTO},
        {"opaque", TT_PROPERTY, P_OPAQUE},
        {"openfiles", TT_FUNCTION, F_OPEN_FILES},
        {"openprocesses", TT_FUNCTION, F_OPEN_PROCESSES},
        {"openprocessids", TT_FUNCTION, F_OPEN_PROCESS_IDS},
        {"opensockets", TT_FUNCTION, F_OPEN_SOCKETS},
        {"openstacks", TT_FUNCTION, F_OPEN_STACKS},
        {"optionkey", TT_FUNCTION, F_OPTION_KEY},
        {"or", TT_BINOP, O_OR},
        {"orientation", TT_PROPERTY, P_ORIENTATION},
		{"outerglow", TT_PROPERTY, P_BITMAP_EFFECT_OUTER_GLOW},
		{"outputlineendings", TT_PROPERTY, P_OUTPUT_LINE_ENDINGS},
		{"outputtextencoding", TT_PROPERTY, P_OUTPUT_TEXT_ENCODING},
		// MW-2008-03-05: [[ Owner Reference ]] 'the owner' is now a function so it works more correctly
		//   in chunk context.
        {"owner", TT_FUNCTION, F_OWNER},
		// MW-2012-02-09: [[ ParaStyles ]] New 'padding' paragraph property.
		{"padding", TT_PROPERTY, P_PADDING},
		{"pagecount", TT_PROPERTY, P_PAGE_COUNT},
        {"pageheights", TT_PROPERTY, P_PAGE_HEIGHTS},
        {"pageinc", TT_PROPERTY, P_PAGE_INC},
        {"pageincrement", TT_PROPERTY, P_PAGE_INC},
        // JS-2013-05-15: [[ PageRanges ]] New 'pageRanges' field property.
        {"pageranges", TT_PROPERTY, P_PAGE_RANGES},
        {"paintcompression", TT_PROPERTY, P_PAINT_COMPRESSION},
        {"palindromeframes", TT_PROPERTY, P_PALINDROME_FRAMES},
        {"pan", TT_PROPERTY, P_PAN},
        {"paragraph", TT_CHUNK, CT_PARAGRAPH},
        {"paragraphoffset", TT_FUNCTION, F_PARAGRAPH_OFFSET},
        {"paragraphs", TT_CLASS, CT_PARAGRAPH},
        {"param", TT_FUNCTION, F_PARAM},
        {"paramcount", TT_FUNCTION, F_PARAM_COUNT},
        {"params", TT_FUNCTION, F_PARAMS},
		{"parentscript", TT_PROPERTY, P_PARENT_SCRIPT},
        {"part", TT_CHUNK, CT_LAYER},
        {"partnumber", TT_PROPERTY, P_LAYER},
        {"parts", TT_CLASS, CT_LAYER},
        {"passkey", TT_PROPERTY, P_KEY},
        {"password", TT_PROPERTY, P_PASSWORD},
        {"pattern", TT_PROPERTY, P_BRUSH_PATTERN},
        {"patterns", TT_PROPERTY, P_PATTERNS},
        {"paused", TT_PROPERTY, P_PAUSED},
        {"peeraddress", TT_FUNCTION, F_PA},
        {"penback", TT_PROPERTY, P_PEN_BACK_COLOR},
        {"pencolor", TT_PROPERTY, P_PEN_COLOR},
        {"pendingmessages", TT_FUNCTION, F_PENDING_MESSAGES},
        {"penfore", TT_PROPERTY, P_PEN_COLOR},
        {"penheight", TT_PROPERTY, P_PEN_HEIGHT},
        {"penpat", TT_PROPERTY, P_PEN_PATTERN},
        {"penpattern", TT_PROPERTY, P_PEN_PATTERN},
        {"penwidth", TT_PROPERTY, P_PEN_WIDTH},
		// IM-2013-12-04: [[ PixelScale ]] The "pixelScale" token
		{"pixelscale", TT_PROPERTY, P_PIXEL_SCALE},
        {"pixmapid", TT_PROPERTY, P_PIXMAP_ID},
		{"plaintext", TT_PROPERTY, P_PLAIN_TEXT},
        {"platform", TT_FUNCTION, F_PLATFORM},
        {"playdestination", TT_PROPERTY, P_PLAY_DESTINATION},
        {"player", TT_CHUNK, CT_PLAYER},
        {"players", TT_CLASS, CT_PLAYER},
        {"playloudness", TT_PROPERTY, P_PLAY_LOUDNESS},
        {"playrate", TT_PROPERTY, P_PLAY_RATE},
        {"playselection", TT_PROPERTY, P_PLAY_SELECTION},
        {"pointerfocus", TT_PROPERTY, P_POINTER_FOCUS},
        {"points", TT_PROPERTY, P_POINTS},
        {"polysides", TT_PROPERTY, P_POLY_SIDES},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'populationStandarddDeviation' (aka popStdDev)
        {"popstddev", TT_FUNCTION, F_POP_STD_DEV},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'populationStandarddDeviation' (aka popStdDev)
        {"populationstandarddeviation", TT_FUNCTION, F_POP_STD_DEV},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'populationVariance' (aka popVariance)
        {"populationvariance", TT_FUNCTION, F_POP_VARIANCE},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'populationStandarddDeviation' (aka popStdDev)
        {"popvariance", TT_FUNCTION, F_POP_VARIANCE},
        {"postscript", TT_PROPERTY, P_POSTSCRIPT},
        {"powerkeys", TT_PROPERTY, P_POWER_KEYS},
		{"preservevariables", TT_PROPERTY, P_PRESERVE_VARIABLES},
		{"preservevars", TT_PROPERTY, P_PRESERVE_VARIABLES},
        {"prev", TT_CHUNK, CT_PREV},
        {"previous", TT_CHUNK, CT_PREV},
		
        {"printcardborders", TT_PROPERTY, P_PRINT_CARD_BORDERS},
		{"printcollate", TT_PROPERTY, P_PRINT_JOB_COLLATE},
        {"printcolors", TT_PROPERTY, P_PRINT_JOB_COLOR},
        {"printcommand", TT_PROPERTY, P_PRINT_COMMAND},
		{"printcopies", TT_PROPERTY, P_PRINT_JOB_COPIES},
		{"printduplex", TT_PROPERTY, P_PRINT_JOB_DUPLEX}, 

		{"printerfeatures", TT_PROPERTY, P_PRINT_DEVICE_FEATURES},
		{"printername", TT_PROPERTY, P_PRINT_DEVICE_NAME},
		{"printeroutput", TT_PROPERTY, P_PRINT_DEVICE_OUTPUT},
		{"printersettings", TT_PROPERTY, P_PRINT_DEVICE_SETTINGS},

        {"printfonttable", TT_PROPERTY, P_PRINT_FONT_TABLE},
        {"printgutters", TT_PROPERTY, P_PRINT_GUTTERS},
	
		{"printmargins", TT_PROPERTY, P_PRINT_MARGINS},
        
		{"printpagenumber", TT_PROPERTY, P_PRINT_JOB_PAGE},
		{"printpaperorientation", TT_PROPERTY, P_PRINT_PAGE_ORIENTATION},
		{"printpaperrect", TT_PROPERTY, P_PRINT_PAGE_RECTANGLE},
		{"printpaperrectangle", TT_PROPERTY, P_PRINT_PAGE_RECTANGLE},
		{"printpaperscale", TT_PROPERTY, P_PRINT_PAGE_SCALE},
		{"printpapersize", TT_PROPERTY, P_PRINT_PAGE_SIZE},

		{"printranges", TT_PROPERTY, P_PRINT_JOB_RANGES},
		{"printrect", TT_PROPERTY, P_PRINT_DEVICE_RECTANGLE},
		{"printrectangle", TT_PROPERTY, P_PRINT_DEVICE_RECTANGLE},
        {"printrotated", TT_PROPERTY, P_PRINT_ROTATED},
        {"printrowsfirst", TT_PROPERTY, P_PRINT_ROWS_FIRST},
        {"printscale", TT_PROPERTY, P_PRINT_SCALE},
		
        {"printtextalign", TT_PROPERTY, P_PRINT_TEXT_ALIGN},
        {"printtextfont", TT_PROPERTY, P_PRINT_TEXT_FONT},
        {"printtextheight", TT_PROPERTY, P_PRINT_TEXT_HEIGHT},
        {"printtextsize", TT_PROPERTY, P_PRINT_TEXT_SIZE},
        {"printtextstyle", TT_PROPERTY, P_PRINT_TEXT_STYLE},

		{"printtitle", TT_PROPERTY, P_PRINT_JOB_NAME},
		
        {"privatecolors", TT_PROPERTY, P_PRIVATE_COLORS},
        {"processid", TT_FUNCTION, F_PROCESS_ID},
        {"processor", TT_FUNCTION, F_PROCESSOR},
		{"processtype", TT_PROPERTY, P_PROCESS_TYPE},
        {"proj", TT_CHUNK, CT_STACK},
        {"project", TT_CHUNK, CT_STACK},
        {"prolog", TT_PROPERTY, P_PROLOG},
        {"properties", TT_PROPERTY, P_PROPERTIES},
        {"propertynames", TT_FUNCTION, F_PROPERTY_NAMES},
        {"proportionalthumbs", TT_PROPERTY, P_PROPORTIONAL_THUMBS},
        {"qteffects", TT_FUNCTION, F_QT_EFFECTS},
		{"qtidlerate", TT_PROPERTY, P_QT_IDLE_RATE},
        {"qtversion", TT_FUNCTION, F_QT_VERSION},
        {"queryregistry", TT_FUNCTION, F_QUERY_REGISTRY},
        {"radiobehavior", TT_PROPERTY, P_RADIO_BEHAVIOR},
        {"raisemenus", TT_PROPERTY, P_RAISE_MENUS},
        {"raisepalettes", TT_PROPERTY, P_RAISE_PALETTES},
		{"raisewindows", TT_PROPERTY, P_RAISE_WINDOWS},
        {"random", TT_FUNCTION, F_RANDOM},
		{"randombytes", TT_FUNCTION, F_RANDOM_BYTES},
        {"randomseed", TT_PROPERTY, P_RANDOM_SEED},
        {"rawclipboarddata", TT_PROPERTY, P_RAW_CLIPBOARD_DATA},
        {"rawdragdata", TT_PROPERTY, P_RAW_DRAGBOARD_DATA},
        {"recent", TT_CHUNK, CT_RECENT},
        {"recentcards", TT_PROPERTY, P_RECENT_CARDS},
        {"recentnames", TT_PROPERTY, P_RECENT_NAMES},
        {"recordchannels", TT_PROPERTY, P_RECORD_CHANNELS},
        {"recordcompression", TT_PROPERTY, P_RECORD_COMPRESSION},
        {"recordcompressiontypes", TT_FUNCTION, F_RECORD_COMPRESSION_TYPES},
        {"recordformat", TT_PROPERTY, P_RECORD_FORMAT},
        {"recordformats", TT_FUNCTION, F_RECORD_FORMATS},
        {"recording", TT_PROPERTY, P_RECORDING},
        {"recordinput", TT_PROPERTY, P_RECORD_INPUT},
        {"recordloudness", TT_FUNCTION, F_RECORD_LOUDNESS},
        {"recordrate", TT_PROPERTY, P_RECORD_RATE},
        {"recordsamplesize", TT_PROPERTY, P_RECORD_SAMPLESIZE},
        {"rect", TT_PROPERTY, P_RECTANGLE},
        {"rectangle", TT_PROPERTY, P_RECTANGLE},
        {"recursionlimit", TT_PROPERTY, P_RECURSION_LIMIT},
#ifdef MODE_DEVELOPMENT
		{"referringstack", TT_PROPERTY, P_REFERRING_STACK},
#endif
        {"rel", TT_TO, PT_RELATIVE},
        {"relative", TT_TO, PT_RELATIVE},
        {"relativepoints", TT_PROPERTY, P_RELATIVE_POINTS},
        {"relayergroupedcontrols", TT_PROPERTY, P_RELAYER_GROUPED_CONTROLS},
        {"remapcolor", TT_PROPERTY, P_REMAP_COLOR},
        {"repeatcount", TT_PROPERTY, P_REPEAT_COUNT},
        {"repeatdelay", TT_PROPERTY, P_REPEAT_DELAY},
        {"repeatrate", TT_PROPERTY, P_REPEAT_RATE},
        {"replacetext", TT_FUNCTION, F_REPLACE_TEXT},
        {"resizable", TT_PROPERTY, P_RESIZABLE},
        {"resizequality", TT_PROPERTY, P_RESIZE_QUALITY},
        {"result", TT_FUNCTION, F_RESULT},
        {"retainimage", TT_PROPERTY, P_RETAIN_IMAGE},
        {"retainpostscript", TT_PROPERTY, P_RETAIN_POSTSCRIPT},
        {"returnkeytype", TT_PROPERTY, P_RETURN_KEY_TYPE},
        {"revavailablehandlers", TT_PROPERTY, P_REV_AVAILABLE_HANDLERS},
		{"revavailablevariables", TT_PROPERTY, P_REV_AVAILABLE_VARIABLES},
        {"revbehavioruses", TT_PROPERTY, P_REV_BEHAVIOR_USES},
#ifdef MODE_DEVELOPMENT
		{"revcrashreportsettings", TT_PROPERTY, P_REV_CRASH_REPORT_SETTINGS},
#endif
        {"revlibrarymapping", TT_PROPERTY, P_REV_LIBRARY_MAPPING},
        {"revlicenseinfo", TT_PROPERTY, P_REV_LICENSE_INFO},
        {"revlicenselimits",TT_PROPERTY,P_REV_LICENSE_LIMITS},
#if defined(MODE_DEVELOPMENT)
#ifdef FEATURE_PROPERTY_LISTENER
		// MM-2012-09-05: [[ Property Listener ]] Returns the list of all active object property listeners
		{"revobjectlisteners", TT_PROPERTY, P_REV_OBJECT_LISTENERS},
		// MM-2012-11-06: [[ Property Listener ]] Minimum number of milliseconds between propertyChanged messages.
		{"revpropertylistenerthrottle", TT_PROPERTY, P_REV_PROPERTY_LISTENER_THROTTLE_TIME},
#endif
#endif
		{"revruntimebehaviour", TT_PROPERTY, P_REV_RUNTIME_BEHAVIOUR},
        {"revscriptdescription", TT_PROPERTY, P_REV_SCRIPT_DESCRIPTION},
#ifdef MODE_DEVELOPMENT
		{"revunplacedgroupids", TT_PROPERTY, P_UNPLACED_GROUP_IDS},
#endif
        {"right", TT_PROPERTY, P_RIGHT},
        {"rightbalance", TT_PROPERTY, P_RIGHT_BALANCE},
		// MW-2011-01-25: [[ ParaStyles ]] The rightIndent paragraph property.
		{"rightindent", TT_PROPERTY, P_RIGHT_INDENT},
        {"rightmargin", TT_PROPERTY, P_RIGHT_MARGIN},
        {"round", TT_FUNCTION, F_ROUND},
        {"roundends", TT_PROPERTY, P_ROUND_ENDS},
        {"roundheight", TT_PROPERTY, P_ROUND_RADIUS},
        {"roundradius", TT_PROPERTY, P_ROUND_RADIUS},
        {"roundwidth", TT_PROPERTY, P_ROUND_RADIUS},
		{"rowdel", TT_PROPERTY, P_ROW_DELIMITER},
		{"rowdelimiter", TT_PROPERTY, P_ROW_DELIMITER},
        {"rtftext", TT_PROPERTY, P_RTF_TEXT},
        {"samplestandarddeviation", TT_FUNCTION, F_SMP_STD_DEV},
        {"samplevariance", TT_FUNCTION, F_SMP_VARIANCE},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'sampleStandardDeviation' (aka stdDev / standardDeviation)
        {"sampstddev", TT_FUNCTION, F_SMP_STD_DEV},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'sampleVariance' (aka sampVariance)
        {"sampvariance", TT_FUNCTION, F_SMP_VARIANCE},
        {"savecompressed", TT_PROPERTY, P_SAVE_COMPRESSED},
        {"sb", TT_CHUNK, CT_SCROLLBAR},
        {"sbs", TT_CLASS, CT_SCROLLBAR},
        {"scale", TT_PROPERTY, P_SCALE},
		// IM-2014-01-07: [[ StackScale ]] New property for 'scalefactor'
		{"scalefactor", TT_PROPERTY, P_SCALE_FACTOR},
        {"scaleindependently", TT_PROPERTY, P_SCALE_INDEPENDENTLY},
		{"screen", TT_PROPERTY, P_SCREEN},
        {"screencolors", TT_FUNCTION, F_SCREEN_COLORS},
        {"screendepth", TT_FUNCTION, F_SCREEN_DEPTH},
        {"screengamma", TT_PROPERTY, P_SCREEN_GAMMA},
        {"screenloc", TT_FUNCTION, F_SCREEN_LOC},
        {"screenmouseloc", TT_PROPERTY, P_SCREEN_MOUSE_LOC},
        {"screenname", TT_FUNCTION, F_SCREEN_NAME},
        {"screennopixmaps", TT_PROPERTY, P_NO_PIXMAPS},
		// IM-2014-01-24: [[ HiDPI ]] The screenPixelScale and screenPixelScales properties
		{"screenpixelscale", TT_PROPERTY, P_SCREEN_PIXEL_SCALE},
		{"screenpixelscales", TT_PROPERTY, P_SCREEN_PIXEL_SCALES},
        {"screenrect", TT_FUNCTION, F_SCREEN_RECT},
		{"screenrects", TT_FUNCTION, F_SCREEN_RECTS},
        {"screensharedmemory", TT_PROPERTY, P_SHARED_MEMORY},
        {"screentype", TT_FUNCTION, F_SCREEN_TYPE},
        {"screenvcsharedmemory", TT_PROPERTY, P_VC_SHARED_MEMORY},
        {"screenvendor", TT_FUNCTION, F_SCREEN_VENDOR},
        {"script", TT_PROPERTY, P_SCRIPT},
		{"scriptexecutionerrors", TT_PROPERTY, P_SCRIPT_EXECUTION_ERRORS},
        {"scriptlimits", TT_FUNCTION, F_SCRIPT_LIMITS},
        {"scriptonly", TT_PROPERTY, P_SCRIPT_ONLY},
        {"scriptparsingerrors", TT_PROPERTY, P_SCRIPT_PARSING_ERRORS},
        {"scriptstatus", TT_PROPERTY, P_SCRIPT_STATUS},
        {"scripttextfont", TT_PROPERTY, P_SCRIPT_TEXT_FONT},
        {"scripttextsize", TT_PROPERTY, P_SCRIPT_TEXT_SIZE},		
        {"scroll", TT_PROPERTY, P_VSCROLL},
        {"scrollbar", TT_CHUNK, CT_SCROLLBAR},
        {"scrollbars", TT_CLASS, CT_SCROLLBAR},
        {"scrollbarwidth", TT_PROPERTY, P_SCROLLBAR_WIDTH},
        {"sec", TT_FUNCTION, F_SECONDS},
        {"second", TT_CHUNK, CT_SECOND},
        {"secondcolor", TT_PROPERTY, P_BACK_COLOR},
        {"secondpixel", TT_PROPERTY, P_BACK_PIXEL},
        {"seconds", TT_FUNCTION, F_SECONDS},
        {"secs", TT_FUNCTION, F_SECONDS},
        {"securemode", TT_PROPERTY, P_SECURE_MODE},
		{"securitycategories", TT_PROPERTY, P_SECURITY_CATEGORIES},
		{"securitypermissions", TT_PROPERTY, P_SECURITY_PERMISSIONS},
        {"segment", TT_CHUNK, CT_WORD},
        {"segmentoffset", TT_FUNCTION, F_WORD_OFFSET},
        {"segments", TT_CLASS, CT_WORD},
        {"selected", TT_PROPERTY, P_SELECTED},
        {"selectedbutton", TT_FUNCTION, F_SELECTED_BUTTON},
        {"selectedchunk", TT_FUNCTION, F_SELECTED_CHUNK},
        {"selectedcolor", TT_PROPERTY, P_SELECTED_COLOR},
        {"selectedfield", TT_FUNCTION, F_SELECTED_FIELD},
        {"selectedgraphic", TT_FUNCTION, F_SELECTED_OBJECT},
        {"selectedimage", TT_FUNCTION, F_SELECTED_IMAGE},
        {"selectedline", TT_FUNCTION, F_SELECTED_LINE},
        {"selectedlines", TT_FUNCTION, F_SELECTED_LINE},
        {"selectedloc", TT_FUNCTION, F_SELECTED_LOC},
        {"selectedobject", TT_FUNCTION, F_SELECTED_OBJECT},
        {"selectedobjects", TT_FUNCTION, F_SELECTED_OBJECT},
        {"selectedtext", TT_FUNCTION, F_SELECTED_TEXT},
        {"selectgroupedcontrols", TT_PROPERTY, P_SELECT_GROUPED_CONTROLS},
        {"selection", TT_FUNCTION, F_SELECTED_TEXT},
        {"selectionhandlecolor", TT_PROPERTY, P_SELECTION_HANDLE_COLOR},
        {"selectionmode", TT_PROPERTY, P_SELECTION_MODE},
        {"selobj", TT_FUNCTION, F_SELECTED_OBJECT},
        {"selobjs", TT_FUNCTION, F_SELECTED_OBJECT},
        {"sentence", TT_CHUNK, CT_SENTENCE},
        {"sentenceoffset", TT_FUNCTION, F_SENTENCE_OFFSET},
        {"sentences", TT_CLASS, CT_SENTENCE},
        {"serialcontrolstring", TT_PROPERTY, P_SERIAL_CONTROL_STRING},
		{"sessioncookiename", TT_PROPERTY, P_SESSION_COOKIE_NAME},
		{"sessionid", TT_PROPERTY, P_SESSION_ID},
		{"sessionlifetime", TT_PROPERTY, P_SESSION_LIFETIME},
		{"sessionsavepath", TT_PROPERTY, P_SESSION_SAVE_PATH},
        {"setregistry", TT_FUNCTION, F_SET_REGISTRY},
        {"setresource", TT_FUNCTION, F_SET_RESOURCE},
        {"seventh", TT_CHUNK, CT_SEVENTH},
        {"seventhcolor", TT_PROPERTY, P_SHADOW_COLOR},
        {"seventhpixel", TT_PROPERTY, P_SHADOW_PIXEL},
		{"sha1digest", TT_FUNCTION, F_SHA1_DIGEST},
        {"shadow", TT_PROPERTY, P_SHADOW},
        {"shadowcolor", TT_PROPERTY, P_SHADOW_COLOR},
        {"shadowoffset", TT_PROPERTY, P_SHADOW_OFFSET},
        {"shadowpattern", TT_PROPERTY, P_SHADOW_PATTERN},
        {"shadowpixel", TT_PROPERTY, P_SHADOW_PIXEL},
		{"sharedbehavior", TT_PROPERTY, P_SHARED_BEHAVIOR},
		{"sharedgroupids", TT_PROPERTY, P_SHARED_GROUP_IDS},
		{"sharedgroupnames", TT_PROPERTY, P_SHARED_GROUP_NAMES},
        {"sharedhilite", TT_PROPERTY, P_SHARED_HILITE},
        {"sharedtext", TT_PROPERTY, P_SHARED_TEXT},
        {"shell", TT_FUNCTION, F_SHELL},
        {"shellcommand", TT_PROPERTY, P_SHELL_COMMAND},
        {"shiftkey", TT_FUNCTION, F_SHIFT_KEY},
        {"short", TT_PROPERTY, P_SHORT},
        {"shortfilepath", TT_FUNCTION, F_SHORT_FILE_PATH},
        {"showbadge", TT_PROPERTY, P_SHOW_BADGE},
        {"showborder", TT_PROPERTY, P_SHOW_BORDER},
        {"showcontroller", TT_PROPERTY, P_SHOW_CONTROLLER},
        {"showfill", TT_PROPERTY, P_FILLED},
        {"showfocusborder", TT_PROPERTY, P_SHOW_FOCUS_BORDER},
        {"showgroups", TT_PROPERTY, P_UNDERLINE_LINKS},
        {"showhilite", TT_PROPERTY, P_SHOW_HILITE},
        {"showicon", TT_PROPERTY, P_SHOW_ICON},
        {"showinvisibles", TT_PROPERTY, P_SHOW_INVISIBLES},
        {"showlines", TT_PROPERTY, P_SHOW_LINES},
        {"showname", TT_PROPERTY, P_SHOW_NAME},
        {"showpen", TT_PROPERTY, P_SHOW_BORDER},
        {"showpict", TT_PROPERTY, P_SHOW_PICT},
        {"showselection", TT_PROPERTY, P_SHOW_SELECTION},
        {"showvalue", TT_PROPERTY, P_SHOW_VALUE},
        {"sin", TT_FUNCTION, F_SIN},
        {"sixth", TT_CHUNK, CT_SIXTH},
        {"sixthcolor", TT_PROPERTY, P_BOTTOM_COLOR},
        {"sixthpixel", TT_PROPERTY, P_BOTTOM_PIXEL},
        {"size", TT_PROPERTY, P_SIZE},
        {"slices", TT_PROPERTY, P_SLICES},
        {"sockettimeoutinterval", TT_PROPERTY, P_SOCKET_TIMEOUT},
        {"sound", TT_FUNCTION, F_SOUND},
        {"soundchannel", TT_PROPERTY, P_SOUND_CHANNEL},
		// MW-2011-01-25: [[ ParaStyles ]] The spaceAbove paragraph property.
		{"spaceabove", TT_PROPERTY, P_SPACE_ABOVE},
		// MW-2011-01-25: [[ ParaStyles ]] The spaceBelow paragraph property.
		{"spacebelow", TT_PROPERTY, P_SPACE_BELOW},
        {"specialfolderpath", TT_FUNCTION, F_SPECIAL_FOLDER_PATH},
        {"spray", TT_PROPERTY, P_SPRAY},
        {"sqrt", TT_FUNCTION, F_SQRT},
        {"sslcertificates",TT_PROPERTY,P_SSL_CERTIFICATES},
        {"stack", TT_CHUNK, CT_STACK},
        {"stackfiles", TT_PROPERTY, P_STACK_FILES},
        {"stackfiletype", TT_PROPERTY, P_STACK_FILE_TYPE},
		{"stackfileversion", TT_PROPERTY, P_STACK_FILE_VERSION},
		{"stacklimit", TT_PROPERTY, P_STACK_LIMIT},
        {"stacks", TT_FUNCTION, F_STACKS},
        {"stacksinuse", TT_PROPERTY, P_STACKS_IN_USE},
        {"stackspace", TT_FUNCTION, F_STACK_SPACE},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'sampleStandardDeviation' (aka stdDev / standardDeviation)
        {"standarddeviation", TT_FUNCTION, F_SMP_STD_DEV},
        {"startangle", TT_PROPERTY, P_START_ANGLE},
        {"startarrow", TT_PROPERTY, P_START_ARROW},
        {"startframe", TT_PROPERTY, P_START_FRAME},
        {"starttime", TT_PROPERTY, P_START_TIME},
        {"startupiconic", TT_PROPERTY, P_START_UP_ICONIC},
        {"startvalue", TT_PROPERTY, P_START_VALUE},
        {"statround", TT_FUNCTION, F_STAT_ROUND},
        // PM-2014-09-02: [[ Bug 13092 ]] Added status property to the player object
        {"status", TT_PROPERTY, P_STATUS},
		{"statusicon", TT_PROPERTY, P_STATUS_ICON},
		{"statusiconmenu", TT_PROPERTY, P_STATUS_ICON_MENU},
		{"statusicontooltip", TT_PROPERTY, P_STATUS_ICON_TOOLTIP},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'sampleStandardDeviation' (aka stdDev / standardDeviation)
        {"stddev", TT_FUNCTION, F_SMP_STD_DEV},
		{"strokegradient", TT_PROPERTY, P_GRADIENT_STROKE},
        {"style", TT_PROPERTY, P_STYLE},
		// MW-2011-12-08: [[ StyledText ]] Keyword for 'styledText' reserved word.
		{"styledtext", TT_PROPERTY, P_STYLED_TEXT},
        {"substacks", TT_PROPERTY, P_SUBSTACKS},
        {"sum", TT_FUNCTION, F_SUM},
        {"syncrate", TT_PROPERTY, P_SYNC_RATE},
        {"syserror", TT_FUNCTION, F_SYS_ERROR},
        {"system", TT_PROPERTY, P_SYSTEM},
		{"systemappearance", TT_PROPERTY, P_SYSTEM_APPEARANCE},
		{"systemcolorselector", TT_PROPERTY, P_SYSTEM_CS},
        {"systemfileselector", TT_PROPERTY, P_SYSTEM_FS},
		// IM-2013-12-04: [[ PixelScale ]] The "systemPixelScale" token
		{"systempixelscale", TT_PROPERTY, P_SYSTEM_PIXEL_SCALE},
		{"systemprintselector", TT_PROPERTY, P_SYSTEM_PS},
        {"systemversion", TT_FUNCTION, F_SYSTEM_VERSION},
        {"systemwindow", TT_PROPERTY, P_SYSTEM_WINDOW},
		// MW-2012-02-10: [[ TabAlign ]] New property allowing configuration of tab alignment.
		{"tabalign", TT_PROPERTY, P_TAB_ALIGN},
        {"tabgroupbehavior", TT_PROPERTY, P_TAB_GROUP_BEHAVIOR},
        {"tabstops", TT_PROPERTY, P_TAB_STOPS},
		// MW-2012-02-11: [[ TabWidths ]] New property setting tabStops by a sequence of widths.
		{"tabwidths", TT_PROPERTY, P_TAB_WIDTHS},
        {"tan", TT_FUNCTION, F_TAN},
        {"target", TT_FUNCTION, F_TARGET},
        {"templateaudioclip", TT_FUNCTION, F_TEMPLATE_AUDIO_CLIP},
        {"templatebutton", TT_FUNCTION, F_TEMPLATE_BUTTON},
        {"templatecard", TT_FUNCTION, F_TEMPLATE_CARD},
        {"templateeps", TT_FUNCTION, F_TEMPLATE_EPS},
        {"templatefield", TT_FUNCTION, F_TEMPLATE_FIELD},
        {"templategraphic", TT_FUNCTION, F_TEMPLATE_GRAPHIC},
        {"templategroup", TT_FUNCTION, F_TEMPLATE_GROUP},
        {"templateimage", TT_FUNCTION, F_TEMPLATE_IMAGE},
        {"templateplayer", TT_FUNCTION, F_TEMPLATE_PLAYER},
        {"templatescrollbar", TT_FUNCTION, F_TEMPLATE_SCROLLBAR},
        {"templatestack", TT_FUNCTION, F_TEMPLATE_STACK},
        {"templatevideoclip", TT_FUNCTION, F_TEMPLATE_VIDEO_CLIP},
        {"tempname", TT_FUNCTION, F_TEMP_NAME},
		// MW-2011-11-24: [[ Nice Folders ]] The adjective for 'the temporary folder'.
		{"temporary", TT_PROPERTY, P_TEMPORARY_FOLDER},
        {"tenth", TT_CHUNK, CT_TENTH},
        {"text", TT_PROPERTY, P_TEXT},
        {"textalign", TT_PROPERTY, P_TEXT_ALIGN},
        {"textarrows", TT_PROPERTY, P_TEXT_ARROWS},
        {"textcolor", TT_PROPERTY, P_FORE_COLOR},
        {"textdata", TT_PROPERTY, P_TEXT},
        {"textdecode", TT_FUNCTION, F_TEXT_DECODE},
        {"textdirection", TT_PROPERTY, P_TEXTDIRECTION},
        {"textencode", TT_FUNCTION, F_TEXT_ENCODE},
        {"textfont", TT_PROPERTY, P_TEXT_FONT},
        {"textheight", TT_PROPERTY, P_TEXT_HEIGHT},
        {"textheightsum", TT_FUNCTION, F_TEXT_HEIGHT_SUM},
        {"textpattern", TT_PROPERTY, P_FORE_PATTERN},
        {"textshift", TT_PROPERTY, P_TEXT_SHIFT},
        {"textsize", TT_PROPERTY, P_TEXT_SIZE},
        {"textstyle", TT_PROPERTY, P_TEXT_STYLE},
        {"the", TT_THE, F_UNDEFINED},
        {"theme", TT_PROPERTY, P_THEME},
        {"themeclass", TT_PROPERTY, P_THEME_CONTROL_TYPE},
        {"there", TT_UNOP, O_THERE},
        {"third", TT_CHUNK, CT_THIRD},
        {"thirdcolor", TT_PROPERTY, P_HILITE_COLOR},
        {"thirdpixel", TT_PROPERTY, P_HILITE_PIXEL},
        {"this", TT_CHUNK, CT_THIS},
        {"threed", TT_PROPERTY, P_3D},
        {"threedhilite", TT_PROPERTY, P_3D_HILITE},
        {"thumbcolor", TT_PROPERTY, P_FORE_COLOR},
        {"thumbpattern", TT_PROPERTY, P_FORE_PATTERN},
        {"thumbpos", TT_PROPERTY, P_THUMB_POS},
        {"thumbposition", TT_PROPERTY, P_THUMB_POS},
        {"thumbsize", TT_PROPERTY, P_THUMB_SIZE},
        {"tick", TT_FUNCTION, F_TICKS},
        {"ticks", TT_FUNCTION, F_TICKS},
        {"tilt", TT_PROPERTY, P_TILT},
        {"time", TT_FUNCTION, F_TIME},
        {"timescale", TT_PROPERTY, P_TIME_SCALE},
        {"title", TT_PROPERTY, P_LABEL},
        {"titlewidth", TT_PROPERTY, P_LABEL_WIDTH},
        {"to", TT_TO, PT_TO},
        {"togglehilites", TT_PROPERTY, P_TOGGLE_HILITE},
        {"token", TT_CHUNK, CT_TOKEN},
        {"tokenoffset", TT_FUNCTION, F_TOKEN_OFFSET},
        {"tokens", TT_CLASS, CT_TOKEN},
        {"tolower", TT_FUNCTION, F_TO_LOWER},
        {"tool", TT_PROPERTY, P_TOOL},
        {"tooltip", TT_PROPERTY, P_TOOL_TIP},
        {"tooltipdelay", TT_PROPERTY, P_TOOL_TIP_DELAY},
        {"top", TT_PROPERTY, P_TOP},
        {"topcolor", TT_PROPERTY, P_TOP_COLOR},
        {"topleft", TT_PROPERTY, P_TOP_LEFT},
        {"topmargin", TT_PROPERTY, P_TOP_MARGIN},
        {"toppattern", TT_PROPERTY, P_TOP_PATTERN},
        {"toppixel", TT_PROPERTY, P_TOP_PIXEL},
        {"topright", TT_PROPERTY, P_TOP_RIGHT},
        {"topstack", TT_FUNCTION, F_TOP_STACK},
        {"topwindow", TT_FUNCTION, F_TOP_STACK},
        {"toupper", TT_FUNCTION, F_TO_UPPER},
        {"traceabort", TT_PROPERTY, P_TRACE_ABORT},
        {"tracedelay", TT_PROPERTY, P_TRACE_DELAY},
        {"tracereturn", TT_PROPERTY, P_TRACE_RETURN},
        {"tracestack", TT_PROPERTY, P_TRACE_STACK},
		{"traceuntil", TT_PROPERTY, P_TRACE_UNTIL},
        {"trackcount", TT_PROPERTY, P_TRACK_COUNT},
        {"tracks", TT_PROPERTY, P_TRACKS},
        {"transpose", TT_FUNCTION, F_TRANSPOSE},
        {"traversalon", TT_PROPERTY, P_TRAVERSAL_ON},
        {"trueword", TT_CHUNK, CT_TRUEWORD},
        {"truewordoffset", TT_FUNCTION, F_TRUEWORD_OFFSET},
        {"truewords", TT_CLASS, CT_TRUEWORD},
        {"trunc", TT_FUNCTION, F_TRUNC},
        {"twelvehourtime", TT_PROPERTY, P_TWELVE_TIME},
        {"typingrate", TT_PROPERTY, P_TYPE_RATE},
        {"umask", TT_PROPERTY, P_UMASK},
		{"unboundedhscroll", TT_PROPERTY, P_UNBOUNDED_HSCROLL},
		{"unboundedvscroll", TT_PROPERTY, P_UNBOUNDED_VSCROLL},
        {"underlinelinks", TT_PROPERTY, P_UNDERLINE_LINKS},
		{"unicodeacceleratortext", TT_PROPERTY, P_UNICODE_ACCELERATOR_TEXT},
		{"unicodeformattedtext", TT_PROPERTY, P_UNICODE_FORMATTED_TEXT},
		{"unicodelabel", TT_PROPERTY, P_UNICODE_LABEL},
		{"unicodeplaintext", TT_PROPERTY, P_UNICODE_PLAIN_TEXT},
        {"unicodetext", TT_PROPERTY, P_UNICODE_TEXT},
		{"unicodetitle", TT_PROPERTY, P_UNICODE_LABEL},
		{"unicodetooltip", TT_PROPERTY, P_UNICODE_TOOL_TIP},
        {"unidecode", TT_FUNCTION, F_UNI_DECODE},
        {"uniencode", TT_FUNCTION, F_UNI_ENCODE},
        {"upper", TT_FUNCTION, F_TO_UPPER},
        {"url", TT_CHUNK, CT_URL},
        {"urldecode", TT_FUNCTION, F_URL_DECODE},
        {"urlencode", TT_FUNCTION, F_URL_ENCODE},
        {"urlheader", TT_CHUNK, CT_URL_HEADER},
		{"urlresponse", TT_PROPERTY, P_URL_RESPONSE},
        {"urlstatus", TT_FUNCTION, F_URL_STATUS},
		// IM-2014-01-24: [[ HiDPI ]] The "usePixelScaling" token
		{"usepixelscaling", TT_PROPERTY, P_USE_PIXEL_SCALING},
        {"userlevel", TT_PROPERTY, P_USER_LEVEL},
        {"usermodify", TT_PROPERTY, P_USER_MODIFY},
        {"userproperties", TT_PROPERTY, P_CUSTOM_KEYS},
        {"usesystemdate", TT_PROPERTY, P_USE_SYSTEM_DATE},
        {"useunicode", TT_PROPERTY, P_USE_UNICODE},
		// MW-2013-05-08: [[ Uuid ]] The uuid function token.
		{"uuid", TT_FUNCTION, F_UUID},
        {"value", TT_FUNCTION, F_VALUE},
        {"variablenames", TT_FUNCTION, F_VARIABLES},
		// JS-2013-06-19: [[ StatsFunctions ]] Token for 'sampleVariance' (aka variance)
        {"variance", TT_FUNCTION, F_SMP_VARIANCE},
        {"vc", TT_CHUNK, CT_VIDEO_CLIP},
        {"vcplayer", TT_PROPERTY, P_VC_PLAYER},
        {"vcs", TT_CLASS, CT_VIDEO_CLIP},
        {"vectordotproduct", TT_FUNCTION, F_VECTOR_DOT_PRODUCT},
        {"version", TT_FUNCTION, F_VERSION},
        {"vgrid", TT_PROPERTY, P_VGRID},
        {"videoclip", TT_CHUNK, CT_VIDEO_CLIP},
        {"videoclipplayer", TT_PROPERTY, P_VC_PLAYER},
        {"videoclips", TT_CLASS, CT_VIDEO_CLIP},
        {"vis", TT_PROPERTY, P_VISIBLE},
        {"visible", TT_PROPERTY, P_VISIBLE},
        {"visited", TT_PROPERTY, P_VISITED},
        {"visitedicon", TT_PROPERTY, P_VISITED_ICON},
        {"volumes", TT_FUNCTION, F_DRIVES},
        {"vscroll", TT_PROPERTY, P_VSCROLL},
        {"vscrollbar", TT_PROPERTY, P_VSCROLLBAR},
        {"waitdepth", TT_FUNCTION, F_WAIT_DEPTH},
        {"watchedvariables", TT_PROPERTY, P_WATCHED_VARIABLES},
        {"wd", TT_CHUNK, CT_STACK},
        {"weekdaynames", TT_FUNCTION, F_WEEK_DAY_NAMES},
        {"wholematches", TT_PROPERTY, P_WHOLE_MATCHES},
        {"widemargins", TT_PROPERTY, P_WIDE_MARGINS},
        {"widget", TT_CHUNK, CT_WIDGET},
        {"widgets", TT_CLASS, CT_WIDGET},
        {"width", TT_PROPERTY, P_WIDTH},
        {"window", TT_CHUNK, CT_STACK},
        {"windowboundingrect", TT_PROPERTY, P_WINDOW_BOUNDING_RECT},
        {"windowid", TT_PROPERTY, P_WINDOW_ID},
        {"windowmanagerplace", TT_PROPERTY, P_WM_PLACE},
        {"windows", TT_FUNCTION, F_OPEN_STACKS},
        {"windowshape", TT_PROPERTY, P_WINDOW_SHAPE},
        {"within", TT_FUNCTION, F_WITHIN},
        {"wmplace", TT_PROPERTY, P_WM_PLACE},
        {"word", TT_CHUNK, CT_WORD},
        {"wordoffset", TT_FUNCTION, F_WORD_OFFSET},
        {"words", TT_CLASS, CT_WORD},
		{"working", TT_PROPERTY, P_WORKING},
		{"wrap", TT_BINOP, O_WRAP},
        {"xextent", TT_PROPERTY, P_X_EXTENT},
        {"xhot", TT_PROPERTY, P_XHOT},
        {"xoffset", TT_PROPERTY, P_X_OFFSET},
        {"xscale", TT_PROPERTY, P_X_SCALE},
        {"yextent", TT_PROPERTY, P_Y_EXTENT},
        {"yhot", TT_PROPERTY, P_YHOT},
        {"yoffset", TT_PROPERTY, P_Y_OFFSET},
        {"yscale", TT_PROPERTY, P_Y_SCALE},
        {"zoom", TT_PROPERTY, P_ZOOM},
        {"zoombox", TT_PROPERTY, P_ZOOM_BOX},
        {"\255", TT_BINOP, O_NE},
        {"\262", TT_BINOP, O_LE},
        {"\263", TT_BINOP, O_GE}
    };

extern const uint4 factor_table_size = ELEMENTS(factor_table);

const static LT find_table[] =
    {
        {"characters", TT_CHUNK, FM_CHARACTERS},
        {"chars", TT_CHUNK, FM_CHARACTERS},
        {"normal", TT_CHUNK, FM_NORMAL},
        {"string", TT_CHUNK, FM_STRING},
        {"whole", TT_CHUNK, FM_WHOLE},
        {"word", TT_CHUNK, FM_WORD},
        {"words", TT_CHUNK, FM_WORD}
    };

const static LT flip_table[] =
    {
        {"down", TT_UNDEFINED, FL_VERTICAL},
        {"horizontal", TT_UNDEFINED, FL_HORIZONTAL},
        {"left", TT_UNDEFINED, FL_HORIZONTAL},
        {"right", TT_UNDEFINED, FL_HORIZONTAL},
        {"up", TT_UNDEFINED, FL_VERTICAL},
        {"vertical", TT_UNDEFINED, FL_VERTICAL}
    };

const static LT go_table[] =
    {
        {"back", TT_CHUNK, CT_BACKWARD},
        {"backward", TT_CHUNK, CT_BACKWARD},
        {"finish", TT_CHUNK, CT_FINISH},
        {"forth", TT_CHUNK, CT_FORWARD},
        {"forward", TT_CHUNK, CT_FORWARD},
        {"help", TT_CHUNK, CT_HELP},
        {"home", TT_CHUNK, CT_HOME},
        {"recent", TT_CHUNK, CT_RECENT},
        {"start", TT_CHUNK, CT_START}
    };

const static LT handler_table[] =
    {
		{"after", TT_HANDLER, HT_AFTER},
		{"before", TT_HANDLER, HT_BEFORE},
		{"command", TT_HANDLER, HT_MESSAGE},
        {"constant", TT_VARIABLE, S_CONSTANT},
        {"function", TT_HANDLER, HT_FUNCTION},
        {"getprop", TT_HANDLER, HT_GETPROP},
        {"global", TT_VARIABLE, S_GLOBAL},
        {"local", TT_VARIABLE, S_LOCAL},
        {"on", TT_HANDLER, HT_MESSAGE},
		{"private", TT_HANDLER, HT_PRIVATE},
        {"setprop", TT_HANDLER, HT_SETPROP},
        {"var", TT_VARIABLE, S_LOCAL},
        {"variable", TT_VARIABLE, S_LOCAL}
    };

const static LT insert_table[] =
    {
        {"back", TT_UNDEFINED, IP_BACK},
        {"front", TT_UNDEFINED, IP_FRONT}
    };

const static LT lock_table[] =
    {
        {"clipboard", TT_UNDEFINED, LC_CLIPBOARD},
        {"colormap", TT_UNDEFINED, LC_COLORMAP},
        {"cursor", TT_UNDEFINED, LC_CURSOR},
        {"dialog", TT_UNDEFINED, LC_ERRORS},
        {"dialogs", TT_UNDEFINED, LC_ERRORS},
        {"error", TT_UNDEFINED, LC_ERRORS},
        {"menus", TT_UNDEFINED, LC_MENUS},
        {"messages", TT_UNDEFINED, LC_MSGS},
        {"moves", TT_UNDEFINED, LC_MOVES},
        {"recent", TT_UNDEFINED, LC_RECENT},
        {"screen", TT_UNDEFINED, LC_SCREEN}
    };

const static LT mark_table[] =
    {
        {"all", TT_UNDEFINED, MC_ALL},
        {"by", TT_UNDEFINED, MC_BY},
        {"cards", TT_UNDEFINED, MC_CARDS},
        {"cds", TT_UNDEFINED, MC_CARDS},
        {"finding", TT_UNDEFINED, MC_FINDING},
        {"where", TT_UNDEFINED, MC_WHERE}
    };

const static LT mode_table[] =
    {
        {"append", TT_UNDEFINED, OM_APPEND},
        {"appending", TT_UNDEFINED, OM_APPEND},
        {"binary", TT_UNDEFINED, OM_BINARY},
        {"neither", TT_UNDEFINED, OM_NEITHER},
        {"read", TT_UNDEFINED, OM_READ},
        {"reading", TT_UNDEFINED, OM_READ},
        {"text", TT_UNDEFINED, OM_TEXT},
        {"update", TT_UNDEFINED, OM_UPDATE},
        {"updating", TT_UNDEFINED, OM_UPDATE},
        {"write", TT_UNDEFINED, OM_WRITE},
        {"writing", TT_UNDEFINED, OM_WRITE}
    };

const static LT move_table[] =
    {
        {"messages", TT_UNDEFINED, MM_MESSAGES},
        {"waiting", TT_UNDEFINED, MM_WAITING}
    };

const static LT open_table[] =
    {
        {"directory", TT_UNDEFINED, OA_DIRECTORY},
        {"driver", TT_UNDEFINED, OA_DRIVER},
        {"file", TT_UNDEFINED, OA_FILE},
        {"folder", TT_UNDEFINED, OA_DIRECTORY},
        {"printing", TT_UNDEFINED, OA_PRINTING},
        {"process", TT_UNDEFINED, OA_PROCESS},
        {"socket", TT_UNDEFINED, OA_SOCKET},
        {"standarderror", TT_UNDEFINED, OA_STDERR},

        {"standardinput", TT_UNDEFINED, OA_STDIN},
        {"standardoutput", TT_UNDEFINED, OA_STDOUT},
        {"stderr", TT_UNDEFINED, OA_STDERR},
        {"stdin", TT_UNDEFINED, OA_STDIN},
        {"stdout", TT_UNDEFINED, OA_STDOUT}
    };

const static LT play_table[] =
    {
        {"ac", TT_UNDEFINED, PP_AUDIO_CLIP},
        {"audioclip", TT_UNDEFINED, PP_AUDIO_CLIP},
        {"back", TT_UNDEFINED, PP_BACK},
        {"forward", TT_UNDEFINED, PP_FORWARD},
        {"looping", TT_UNDEFINED, PP_LOOPING},
        {"options", TT_UNDEFINED, PP_OPTIONS},
        {"pause", TT_UNDEFINED, PP_PAUSE},
        {"player", TT_UNDEFINED, PP_PLAYER},
        {"resume", TT_UNDEFINED, PP_RESUME},
        {"step", TT_UNDEFINED, PP_STEP},
        {"stop", TT_UNDEFINED, PP_STOP},
        {"tempo", TT_UNDEFINED, PP_TEMPO},
        {"vc", TT_UNDEFINED, PP_VIDEO_CLIP},
		{"video", TT_UNDEFINED, PP_VIDEO},
        {"videoclip", TT_UNDEFINED, PP_VIDEO_CLIP}
    };

const static LT record_table[] =
    {
        {"best", TT_UNDEFINED, RC_BEST},
        {"better", TT_UNDEFINED, RC_BETTER},
        {"good", TT_UNDEFINED, RC_GOOD},
        {"pause", TT_UNDEFINED, RC_PAUSE},
        {"quality", TT_UNDEFINED, RC_QUALITY},
        {"resume", TT_UNDEFINED, RC_RESUME},
        {"sound", TT_UNDEFINED, RC_SOUND}
    };

const static LT repeat_table[] =
    {
        {"each", TT_UNDEFINED, RF_EACH},
        {"for", TT_UNDEFINED, RF_FOR},
        {"forever", TT_UNDEFINED, RF_FOREVER},
        {"step", TT_UNDEFINED, RF_STEP},
        // SN-2015-06-18: [[ Bug 15509 ]] Parse 'times' in 'repeat for x times'
        {"times", TT_UNDEFINED, RF_TIMES},
        {"until", TT_UNDEFINED, RF_UNTIL},
        {"while", TT_UNDEFINED, RF_WHILE},
        {"with", TT_UNDEFINED, RF_WITH}
    };

const static LT reset_table[] =
    {
        {"cursors", TT_UNDEFINED, RT_CURSORS},
        {"paint", TT_UNDEFINED, RT_PAINT},
        {"printing", TT_UNDEFINED, RT_PRINTING},
        {"templateaudioclip", TT_UNDEFINED, RT_TEMPLATE_AUDIO_CLIP},
        {"templatebutton", TT_UNDEFINED, RT_TEMPLATE_BUTTON},
        {"templatecard", TT_UNDEFINED, RT_TEMPLATE_CARD},
        {"templateeps", TT_UNDEFINED, RT_TEMPLATE_EPS},
        {"templatefield", TT_UNDEFINED, RT_TEMPLATE_FIELD},
        {"templategraphic", TT_UNDEFINED, RT_TEMPLATE_GRAPHIC},
        {"templategroup", TT_UNDEFINED, RT_TEMPLATE_GROUP},
        {"templateimage", TT_UNDEFINED, RT_TEMPLATE_IMAGE},
        {"templateplayer", TT_UNDEFINED, RT_TEMPLATE_PLAYER},
        {"templatescrollbar", TT_UNDEFINED, RT_TEMPLATE_SCROLLBAR},
        {"templatestack", TT_UNDEFINED, RT_TEMPLATE_STACK},
        {"templatevideoclip", TT_UNDEFINED, RT_TEMPLATE_VIDEO_CLIP},
    };

const static LT show_table[] =
    {
        {"all", TT_UNDEFINED, SO_ALL},
        {"background", TT_UNDEFINED, SO_BACKGROUND},
        {"bg", TT_UNDEFINED, SO_BACKGROUND},
        {"bkgnd", TT_UNDEFINED, SO_BACKGROUND},
        {"box", TT_UNDEFINED, SO_MESSAGE},
        {"break", TT_UNDEFINED, SO_BREAK},
        {"card", TT_UNDEFINED, SO_CARD},
        {"cards", TT_UNDEFINED, SO_CARD},
        {"cd", TT_UNDEFINED, SO_CARD},
        {"cds", TT_UNDEFINED, SO_CARD},
        {"groups", TT_UNDEFINED, SO_GROUPS},
        {"marked", TT_UNDEFINED, SO_MARKED},
        {"menubar", TT_UNDEFINED, SO_MENU},
        {"message", TT_UNDEFINED, SO_MESSAGE},
        {"msg", TT_UNDEFINED, SO_MESSAGE},
        {"page", TT_UNDEFINED, SO_BREAK},
        {"pattern", TT_UNDEFINED, SO_PALETTE},
        {"picture", TT_UNDEFINED, SO_PICTURE},
        {"taskbar", TT_UNDEFINED, SO_TASKBAR},
        {"titlebar", TT_UNDEFINED, SO_TITLEBAR},
        {"tool", TT_UNDEFINED, SO_PALETTE},
        {"window", TT_UNDEFINED, SO_WINDOW}
    };

const static LT sort_table[] =
    {
        {"ascending", TT_UNDEFINED, ST_ASCENDING},
        {"binary", TT_UNDEFINED, ST_BINARY},
        {"by", TT_UNDEFINED, ST_BY},
        {"cards", TT_UNDEFINED, ST_CARDS},
        {"cds", TT_UNDEFINED, ST_CARDS},
        {"datetime", TT_UNDEFINED, ST_DATETIME},
        {"descending", TT_UNDEFINED, ST_DESCENDING},
		{"in", TT_UNDEFINED, ST_OF},
        {"international", TT_UNDEFINED, ST_INTERNATIONAL},
        {"items", TT_UNDEFINED, ST_ITEMS},
        {"lines", TT_UNDEFINED, ST_LINES},
        {"marked", TT_UNDEFINED, ST_MARKED},
        {"numeric", TT_UNDEFINED, ST_NUMERIC},
        {"of", TT_UNDEFINED, ST_OF},
        {"text", TT_UNDEFINED, ST_TEXT}
    };


const static LT ssl_table[] =
    {
        {"certificate", TT_UNDEFINED, SSL_CERTIFICATE},
        {"verification", TT_UNDEFINED, SSL_VERIFICATION}
    };


const static LT start_table[] =
    {
        {"drag", TT_UNDEFINED, SC_DRAG},
        {"editing", TT_UNDEFINED, SC_EDITING},
        {"library", TT_UNDEFINED, SC_USING},
        {"me", TT_UNDEFINED, SC_PLAYER},
        {"moving", TT_UNDEFINED, SC_MOVING},
        {"player", TT_UNDEFINED, SC_PLAYER},
        {"playing", TT_UNDEFINED, SC_PLAYING},
        {"recording", TT_UNDEFINED, SC_RECORDING},
		{"session", TT_UNDEFINED, SC_SESSION},
        {"using", TT_UNDEFINED, SC_USING}
    };

const static LT sugar_table[] =
    {
		{"anchor", TT_UNDEFINED, SG_ANCHOR},
		{"bookmark", TT_UNDEFINED, SG_BOOKMARK},
		{"browser", TT_UNDEFINED, SG_BROWSER},
        {"callback", TT_CHUNK, CT_UNDEFINED},
		{"caller", TT_UNDEFINED, SG_CALLER},
		{"closed", TT_UNDEFINED, SG_CLOSED},
        {"data", TT_UNDEFINED, SG_DATA},
		{"effects", TT_UNDEFINED, SG_EFFECTS},
		{"elevated", TT_UNDEFINED, SG_ELEVATED},
        {"empty", TT_CHUNK, CT_UNDEFINED},
        {"error", TT_UNDEFINED, SG_ERROR},
        {"extension", TT_UNDEFINED, SG_EXTENSION},
		// MW-2013-11-14: [[ AssertCmd ]] Token for 'failure'
		{"failure", TT_UNDEFINED, SG_FAILURE},
		// MW-2013-11-14: [[ AssertCmd ]] Token for 'false'
		{"false", TT_UNDEFINED, SG_FALSE},
        // TD-2013-06-14: [[ DynamicFonts ]] start using font theFont [globally]
        {"file", TT_UNDEFINED, SG_FILE},
        {"font", TT_UNDEFINED, SG_FONT},
        {"globally", TT_UNDEFINED, SG_GLOBALLY},
		
        // MM-2014-06-13: [[ Bug 12567 ]] Added host. Used in 'with verification for host <host>'
		{"host", TT_UNDEFINED, SG_HOST},		
		{"initially", TT_UNDEFINED, SG_INITIALLY},
        {"keyword", TT_CHUNK, CT_UNDEFINED},
		{"level", TT_UNDEFINED, SG_LEVEL},
		{"link", TT_UNDEFINED, SG_LINK},
#ifdef FEATURE_PROPERTY_LISTENER
		// MM-2012-09-05: [[ Property Listener ]] Syntax: cancel listener for [object]
		{"listener", TT_UNDEFINED, SG_LISTENER},
#endif
		// JS-2013-07-01: [[ EnhancedFilter ]] Token for 'matching'.
        {"matching", TT_UNDEFINED, SG_MATCHING},
        {"message", TT_CHUNK, CT_UNDEFINED},
        {"new", TT_CHUNK, CT_UNDEFINED},
		{"nothing", TT_UNDEFINED, SG_NOTHING},
        // MW-2014-09-30: [[ ScriptOnlyStack ]] Token for 'only'.
        {"only", TT_UNDEFINED, SG_ONLY},
		{"open", TT_UNDEFINED, SG_OPEN},
		{"optimized", TT_UNDEFINED, SG_OPTIMIZED},
		{"options", TT_UNDEFINED, SG_OPTIONS},
		{"path", TT_UNDEFINED, SG_PATH},
		// JS-2013-07-01: [[ EnhancedFilter ]] Token for 'pattern'.
		{"pattern", TT_UNDEFINED, SG_PATTERN},
		{"preserving", TT_UNDEFINED, SG_PRESERVING},
        {"real", TT_UNDEFINED, SG_REAL},
		// MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
		// AL-2013-10-30: [[ Bug 11351 ]] Ensure table is in alphabetical order.
        {"recursively", TT_UNDEFINED, SG_RECURSIVELY},
		// JS-2013-07-01: [[ EnhancedFilter ]] Token for 'regex'.
		{"regex", TT_UNDEFINED, SG_REGEX},
		{"replacing", TT_UNDEFINED, SG_REPLACING},
		{"resource", TT_UNDEFINED, SG_RESOURCE},
		{"standard", TT_UNDEFINED, SG_STANDARD},
        {"strictly", TT_UNDEFINED, SG_STRICTLY},
		// MERG-2013-06-24: [[ IsAnAsciiString ]] Token for 'string'.
        {"string", TT_UNDEFINED, SG_STRING},
		{"styles", TT_UNDEFINED, SG_STYLES},
		// MW-2013-11-14: [[ AssertCmd ]] Token for 'success'
		{"success", TT_UNDEFINED, SG_SUCCESS},
		// MW-2013-11-14: [[ AssertCmd ]] Token for 'true'
		{"true", TT_UNDEFINED, SG_TRUE},
		{"unicode", TT_UNDEFINED, SG_UNICODE},
		{"url", TT_UNDEFINED, SG_URL},
        {"urlresult", TT_UNDEFINED, SG_URL_RESULT},
        {"value", TT_UNDEFINED, SG_VALUE},
		// JS-2013-07-01: [[ EnhancedFilter ]] Token for 'wildcard'.
		{"wildcard", TT_UNDEFINED, SG_WILDCARD},
		{"without", TT_PREP, PT_WITHOUT},
    };

const static LT there_table[] =
    {
        {"directory", TT_UNDEFINED, TM_DIRECTORY},
        {"file", TT_UNDEFINED, TM_FILE},
        {"folder", TT_UNDEFINED, TM_DIRECTORY},
        {"process", TT_UNDEFINED, TM_PROCESS},
        {"url", TT_UNDEFINED, TM_URL}
    };

const static LT tool_table[] =
    {
        {"browse", TT_TOOL, T_BROWSE},
        {"brush", TT_TOOL, T_BRUSH},
        {"bucket", TT_TOOL, T_BUCKET},
        {"button", TT_TOOL, T_BUTTON},
        {"can", TT_TOOL, T_UNDEFINED},
        {"curve", TT_TOOL, T_CURVE},
        {"dropper", TT_TOOL, T_DROPPER},
        {"eraser", TT_TOOL, T_ERASER},
        {"eye", TT_TOOL, T_DROPPER},
        {"field", TT_TOOL, T_FIELD},
        {"graphic", TT_TOOL, T_GRAPHIC},
        {"help", TT_TOOL, T_HELP},
        {"image", TT_TOOL, T_IMAGE},
        {"lasso", TT_TOOL, T_LASSO},
        {"line", TT_TOOL, T_LINE},
        {"oval", TT_TOOL, T_OVAL},
        {"pencil", TT_TOOL, T_PENCIL},
        {"player", TT_TOOL, T_PLAYER},
        {"pointer", TT_TOOL, T_POINTER},
        {"poly", TT_TOOL, T_POLYGON},
        {"polygon", TT_TOOL, T_POLYGON},
        {"rect", TT_TOOL, T_RECTANGLE},
        {"rectangle", TT_TOOL, T_RECTANGLE},
        {"reg", TT_TOOL, T_REGULAR_POLYGON},
        {"regular", TT_TOOL, T_REGULAR_POLYGON},
        {"round", TT_TOOL, T_ROUND_RECT},
        {"rounded", TT_TOOL, T_ROUND_RECT},
        {"scrollbar", TT_TOOL, T_SCROLLBAR},
        {"select", TT_TOOL, T_SELECT},
        {"spray", TT_TOOL, T_SPRAY},
        {"text", TT_TOOL, T_TEXT},
        {"tool", TT_END, T_UNDEFINED}
    };

const static LT unit_table[] =
    {
		{"byte", TT_UNDEFINED, FU_BYTE},
		{"bytes", TT_UNDEFINED, FU_BYTE},
        {"char", TT_UNDEFINED, FU_CHARACTER},
        {"character", TT_UNDEFINED, FU_CHARACTER},
        {"characters", TT_UNDEFINED, FU_CHARACTER},
        {"chars", TT_UNDEFINED, FU_CHARACTER},
        {"codepoint", TT_UNDEFINED, FU_CODEPOINT},
        {"codepoints", TT_UNDEFINED, FU_CODEPOINT},
        {"codeunit", TT_UNDEFINED, FU_CODEUNIT},
        {"codeunits", TT_UNDEFINED, FU_CODEUNIT},
        {"element", TT_UNDEFINED, FU_ELEMENT},
        {"int1", TT_UNDEFINED, FU_INT1},
        {"int1s", TT_UNDEFINED, FU_INT1},
        {"int2", TT_UNDEFINED, FU_INT2},
        {"int2s", TT_UNDEFINED, FU_INT2},
        {"int4", TT_UNDEFINED, FU_INT4},
        {"int4s", TT_UNDEFINED, FU_INT4},
        {"int8", TT_UNDEFINED, FU_INT8},
        {"int8s", TT_UNDEFINED, FU_INT8},
        {"item", TT_UNDEFINED, FU_ITEM},
        {"items", TT_UNDEFINED, FU_ITEM},
		{"key", TT_UNDEFINED, FU_KEY},
        {"line", TT_UNDEFINED, FU_LINE},
        {"lines", TT_UNDEFINED, FU_LINE},
        {"paragraph", TT_UNDEFINED, FU_PARAGRAPH},
        {"paragraphs", TT_UNDEFINED, FU_PARAGRAPH},
        {"real4", TT_UNDEFINED, FU_REAL4},
        {"real4s", TT_UNDEFINED, FU_REAL4},
        {"real8", TT_UNDEFINED, FU_REAL8},
        {"real8s", TT_UNDEFINED, FU_REAL8},
        {"segment", TT_UNDEFINED, FU_WORD},
        {"segments", TT_UNDEFINED, FU_WORD},
        {"sentence", TT_UNDEFINED, FU_SENTENCE},
        {"sentences", TT_UNDEFINED, FU_SENTENCE},
        {"token", TT_UNDEFINED, FU_TOKEN},
        {"tokens", TT_UNDEFINED, FU_TOKEN},
        {"trueword", TT_UNDEFINED, FU_TRUEWORD},
        {"truewords", TT_UNDEFINED, FU_TRUEWORD},
        {"uint1", TT_UNDEFINED, FU_UINT1},
        {"uint1s", TT_UNDEFINED, FU_UINT1},
        {"uint2", TT_UNDEFINED, FU_UINT2},
        {"uint2s", TT_UNDEFINED, FU_UINT2},
        {"uint4", TT_UNDEFINED, FU_UINT4},
        {"uint4s", TT_UNDEFINED, FU_UINT4},
        {"uint8", TT_UNDEFINED, FU_UINT8},
        {"uint8s", TT_UNDEFINED, FU_UINT8},
        {"word", TT_UNDEFINED, FU_WORD},
        {"words", TT_UNDEFINED, FU_WORD}
    };

const static LT validation_table[] =
{
    {"a", TT_UNDEFINED, IV_UNDEFINED},
    {"among", TT_UNDEFINED, IV_AMONG},
    {"an", TT_UNDEFINED, IV_UNDEFINED},
    {"array", TT_UNDEFINED, IV_ARRAY},
	// MERG-2013-06-24: [[ IsAnAsciiString ]] Token for 'ascii'.
    {"ascii", TT_UNDEFINED, IV_ASCII},
	{"boolean", TT_UNDEFINED, IV_LOGICAL},
    {"color", TT_UNDEFINED, IV_COLOR},
    {"date", TT_UNDEFINED, IV_DATE},
    {"integer", TT_UNDEFINED, IV_INTEGER},
    {"logical", TT_UNDEFINED, IV_LOGICAL},
    {"number", TT_UNDEFINED, IV_NUMBER},
    {"point", TT_UNDEFINED, IV_POINT},
    {"rect", TT_UNDEFINED, IV_RECT},
    {"rectangle", TT_UNDEFINED, IV_RECT},
};

const static LT visual_table[] =
{
    {"barn", TT_VISUAL, VE_BARN},
    {"black", TT_VISUAL, VE_BLACK},
    {"blinds", TT_VISUAL, VE_BLINDS},
    {"bottom", TT_VISUAL, VE_BOTTOM},
    {"card", TT_VISUAL, VE_CARD},
    {"center", TT_VISUAL, VE_CENTER},
    {"checkerboard", TT_VISUAL, VE_CHECKERBOARD},
    {"close", TT_VISUAL, VE_CLOSE},
	{"curl", TT_VISUAL, VE_CURL},
    {"dissolve", TT_VISUAL, VE_DISSOLVE},
    {"door", TT_VISUAL, VE_DOOR},
    {"down", TT_VISUAL, VE_DOWN},
    {"effect", TT_VISUAL, VE_EFFECT},
    {"fast", TT_VISUAL, VE_FAST},
	{"flip", TT_VISUAL, VE_FLIP},
    {"from", TT_VISUAL, VE_FROM},
    {"gray", TT_VISUAL, VE_GRAY},
    {"grey", TT_VISUAL, VE_GRAY},
    {"in", TT_VISUAL, VE_IN},
    {"inverse", TT_VISUAL, VE_INVERSE},
    {"iris", TT_VISUAL, VE_IRIS},
    {"left", TT_VISUAL, VE_LEFT},
    {"normal", TT_VISUAL, VE_NORMAL},
    {"open", TT_VISUAL, VE_OPEN},
    {"option", TT_VISUAL, VE_OPTION},
    {"out", TT_VISUAL, VE_OUT},
    {"plain", TT_VISUAL, VE_PLAIN},
    {"push", TT_VISUAL, VE_PUSH},
    {"reveal", TT_VISUAL, VE_REVEAL},
    {"right", TT_VISUAL, VE_RIGHT},
    {"scroll", TT_VISUAL, VE_SCROLL},
    {"shrink", TT_VISUAL, VE_SHRINK},
    {"slow", TT_VISUAL, VE_SLOW},
    {"slowly", TT_VISUAL, VE_SLOW},
    {"sound", TT_VISUAL, VE_SOUND},
    {"stretch", TT_VISUAL, VE_STRETCH},
    {"to", TT_VISUAL, VE_TO},
    {"top", TT_VISUAL, VE_TOP},
    {"up", TT_VISUAL, VE_UP},
    {"venetian", TT_VISUAL, VE_VENETIAN},
    {"very", TT_VISUAL, VE_VERY},
    {"white", TT_VISUAL, VE_WHITE},
    {"wipe", TT_VISUAL, VE_WIPE},
    {"with", TT_VISUAL, VE_WITH},
    {"zoom", TT_VISUAL, VE_ZOOM}
};

const static LT server_table[] =
{
	{"binary", TT_PREP, PT_BINARY},
	{"content", TT_PREP, PT_CONTENT},
	{"cookie", TT_PREP, PT_COOKIE},
	{"header", TT_PREP, PT_HEADER},
	{"httponly", TT_SERVER, SK_HTTPONLY},
	{"markup", TT_PREP, PT_MARKUP},
	{"new", TT_SERVER, SK_NEW},
	{"secure", TT_SERVER, SK_SECURE},
	{"unicode", TT_SERVER, SK_UNICODE},
};

const LT * const table_pointers[] =
{
    accept_table,
    ae_table,
    ask_table,
    command_table,
    convert_table,
    encryption_table,
    exit_table,
    export_table,
    factor_table,
    find_table,
    flip_table,
    go_table,
    handler_table,
    insert_table,
    lock_table,
    mark_table,
    mode_table,
    move_table,
    open_table,
    play_table,
    record_table,
    repeat_table,
    reset_table,
    show_table,
    sort_table,
    ssl_table,
    start_table,
    sugar_table,
    there_table,
    tool_table,
    unit_table,
    validation_table,
    visual_table,
	server_table
};
extern const uint4 table_pointers_size = ELEMENTS(table_pointers);

const uint2 table_sizes[] =
{
    ELEMENTS(accept_table),
    ELEMENTS(ae_table),
    ELEMENTS(ask_table),
    ELEMENTS(command_table),
    ELEMENTS(convert_table),
    ELEMENTS(encryption_table),
    ELEMENTS(exit_table),
    ELEMENTS(export_table),
    ELEMENTS(factor_table),
    ELEMENTS(find_table),
    ELEMENTS(flip_table),
    ELEMENTS(go_table),
    ELEMENTS(handler_table),
    ELEMENTS(insert_table),
    ELEMENTS(lock_table),
    ELEMENTS(mark_table),
    ELEMENTS(mode_table),
    ELEMENTS(move_table),
    ELEMENTS(open_table),
    ELEMENTS(play_table),
    ELEMENTS(record_table),
    ELEMENTS(repeat_table),
    ELEMENTS(reset_table),
    ELEMENTS(show_table),
    ELEMENTS(sort_table),
    ELEMENTS(ssl_table),
    ELEMENTS(start_table),
    ELEMENTS(sugar_table),
    ELEMENTS(there_table),
    ELEMENTS(tool_table),
    ELEMENTS(unit_table),
    ELEMENTS(validation_table),
    ELEMENTS(visual_table),
	ELEMENTS(server_table),
};
extern const uint4 table_sizes_size = ELEMENTS(table_sizes);
