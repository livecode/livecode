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
// global parsing definitions for MetaCard
//
#ifndef	PARSEDEFS_H
#define	PARSEDEFS_H

#include "mcutility.h"
#include "sysdefs.h"

typedef struct _constant
{
	MCString name;
	MCString value;
}
constant;

#define ELEMENTS(table) (sizeof(table) / sizeof(table[0]))

enum Accept_constants {
    AC_CONNECTIONS,
	AC_ON,
	AC_PORT,
	AC_DATAGRAM,
    AC_SECURE
};

enum Apple_event {
    AE_UNDEFINED,
    AE_AE,
    AE_CLASS,
    AE_DATA,
    AE_ID,
    AE_RETURN,
    AE_RETURN_ID,
    AE_SENDER
};

enum Ask_type {
    AT_UNDEFINED,
    AT_CLEAR,
    AT_COLOR,
    AT_EFFECT,
    AT_ERROR,
    AT_FILE,
    AT_FOLDER,
    AT_INFORMATION,
    AT_PASSWORD,
    AT_PRINTER,
    AT_PROGRAM,
    AT_QUESTION,
    AT_RECORD,
    AT_TITLED,
    AT_WARNING,
    AT_SHEET,
	AT_FILES,
	AT_TYPES,
	AT_FOLDERS,
	// Adding page setup, for "answer pagesetup" syntax
	AT_PAGE,
	AT_SETUP,
	AT_PAGESETUP,
	AT_HINT,
};

enum Assert_type {
    ASSERT_TYPE_NONE,
    ASSERT_TYPE_TRUE,
    ASSERT_TYPE_FALSE,
    ASSERT_TYPE_SUCCESS,
    ASSERT_TYPE_FAILURE,
};

inline Chunk_term ct_class(Chunk_term src)
{
	if (src == CT_UNDEFINED)
		return src;
	if (src < CT_DIRECT)
		return CT_DIRECT;
	if (src < CT_ORDINAL)
		return CT_ORDINAL;
	if (src == CT_ID)
		return CT_ID;
	if (src == CT_EXPRESSION)
		return CT_EXPRESSION;
	return CT_TYPES;
}

enum Convert_form {
    CF_UNDEFINED,
    CF_SHORT,
    CF_ABBREVIATED,
    CF_LONG,
    CF_INTERNET,
    CF_SECONDS,
    CF_DATEITEMS,
    CF_TIME,
    CF_SHORT_TIME,
    CF_ABBREV_TIME,
    CF_LONG_TIME,
    CF_DATE,
    CF_SHORT_DATE,
    CF_ABBREV_DATE,
    CF_LONG_DATE,
    CF_INTERNET_DATE,
    CF_ENGLISH = 1000,
    CF_SYSTEM = 2000
};

// destination container for chunk expressions
enum Dest_type {
    DT_UNDEFINED,
    DT_ISDEST,
    DT_VARIABLE,
    DT_EXPRESSION,
    DT_ME,
    DT_MENU_OBJECT,
    DT_TARGET,
	DT_FIRST_OBJECT,
    DT_BUTTON = DT_FIRST_OBJECT,
    DT_CARD,
    DT_FIELD,
    DT_GROUP,
    DT_IMAGE,
    DT_GRAPHIC,
    DT_EPS,
    DT_SCROLLBAR,
    DT_AUDIO_CLIP,
    DT_VIDEO_CLIP,
    DT_PLAYER,
    DT_STACK,
	DT_LAST_OBJECT,
    DT_SELECTED,
    DT_ERROR,
    DT_TOP_STACK,
    DT_CLICK_STACK,
    DT_MOUSE_STACK,
    DT_FUNCTION,
	
	// MW-2008-11-05: [[ Owner Reference ]] This desttype is used for chunks of the form:
	//   ... of the owner of ...
	DT_OWNER,
	// MW-2013-08-05: [[ ThisMe ]] Access to the behavior object (this me).
	DT_THIS_ME,
};

enum Encryption_constants
{
    ENCRT_BIT,
    ENCRT_IV,
    ENCRT_KEY,
    ENCRT_PASSWORD,
    ENCRT_SALT,
    ENCRT_USING,
    RSA_CERT,
    RSA_PRIVKEY,
    RSA_PUBKEY,
	ENCRT_RSA,
	ENCRT_PUBLIC,
	ENCRT_PRIVATE,
	ENCRT_PASSPHRASE,
};

enum Exec_concat {
    EC_NONE,
    EC_SPACE,
    EC_COMMA,
    EC_NULL,
    EC_RETURN,
	EC_TAB
};

// return codes from statements, handlers (exec and eval)
enum Exec_stat {
    ES_ERROR,
    ES_NORMAL,
    ES_NEXT_REPEAT,
    ES_EXIT_REPEAT,
    ES_EXIT_HANDLER,
    ES_EXIT_SWITCH,
    ES_EXIT_ALL,
	ES_RETURN_HANDLER,
    ES_PASS,
    ES_PASS_ALL,
    ES_NOT_HANDLED,
    ES_NOT_FOUND
};

enum Exit_to {
    ET_UNDEFINED,
    ET_ALL,
    ET_REPEAT,
    ET_SWITCH,
    ET_TO
};

enum Export_format {
    EX_UNDEFINED,
    EX_AUDIO_CLIP,
    EX_DISPLAY,
    EX_EPS,
    EX_GIF,
    EX_JPEG,
    EX_PBM,
    EX_PNG,
    EX_SNAPSHOT,
    EX_STACK,
    EX_VIDEO_CLIP,
    EX_XBM,
    EX_XWD,
    EX_AIFF,
    EX_WAVE,
    EX_ULAW,
    EX_MOVIE,
	EX_RAW,
	EX_RAW_ARGB,
	EX_RAW_ABGR,
	EX_RAW_RGBA,
	EX_RAW_BGRA,
	EX_RAW_RGB,
	EX_RAW_BGR,
	EX_RAW_BGRX,
	EX_RAW_GRAY,
	EX_RAW_INDEXED,
	EX_BMP,
    EX_OBJECT,
};

enum Factor_rank {
    FR_UNDEFINED,
    FR_GROUPING,
    FR_OR,
    FR_AND,
    FR_OR_BITS,
    FR_XOR_BITS,
    FR_AND_BITS,
    FR_EQUAL,
    FR_COMPARISON,
    FR_CONCAT,
    FR_ADDSUB,
    FR_MULDIV,
    FR_POW,
    FR_UNARY,
    FR_VALUE
};

enum File_unit {
    FU_UNDEFINED,
    FU_BYTE,
    FU_CHARACTER,
    FU_CODEPOINT,
    FU_CODEUNIT,
    FU_INT1,
    FU_INT2,
    FU_INT4,
    FU_INT8,
    FU_ITEM,
    FU_LINE,
    FU_PARAGRAPH,
    FU_REAL4,
    FU_REAL8,
    FU_TOKEN,
    FU_SENTENCE,
    FU_WORD,
    FU_TRUEWORD,
    FU_UINT1,
    FU_UINT2,
    FU_UINT4,
    FU_UINT8,
    FU_ELEMENT,
	FU_KEY
};

enum Find_mode {
    FM_UNDEFINED,
    FM_NORMAL,
    FM_WHOLE,
    FM_WORD,
    FM_CHARACTERS,
    FM_STRING
};

enum Flip_dir {
    FL_UNDEFINED,
    FL_HORIZONTAL,
    FL_VERTICAL
};

// built in functions
enum Functions {
    F_UNDEFINED,
    F_ABS,
    F_ACOS,
    F_ALIAS_REFERENCE,
    F_ALTERNATE_LANGUAGES,
    F_ANNUITY,
	// JS-2013-06-19: [[ StatsFunctions ]] Tag for 'arithmeticMean' (was average)
    F_ARI_MEAN,
    F_ASIN,
    F_ATAN,
    F_ATAN2,
	// JS-2013-06-19: [[ StatsFunctions ]] Tag for 'averageDeviation'
    F_AVG_DEV,
    F_BACK_SCRIPTS,
    F_BASE_CONVERT,
    F_BASE64_DECODE,
    F_BASE64_ENCODE,
    // AL-2014-10-17: [[ BiDi ]] Returns the result of applying the bi-directional algorithm to text
    F_BIDI_DIRECTION,
    F_BINARY_DECODE,
    F_BINARY_ENCODE,
    F_BUILD_NUMBER,
    F_BYTE_OFFSET,
    F_CACHED_URLS,
    F_CAPS_LOCK_KEY,
	F_BYTE_TO_NUM,
	// MDW-2014-08-23 : [[ feature_floor ]]
	F_CEIL,
    F_CHAR_TO_NUM,
    F_CIPHER_NAMES,
    F_CLICK_CHAR,
    F_CLICK_CHAR_CHUNK,
    F_CLICK_CHUNK,
    F_CLICK_FIELD,
    F_CLICK_H,
    F_CLICK_LINE,
    F_CLICK_LOC,
    F_CLICK_STACK,
    F_CLICK_TEXT,
    F_CLICK_V,
    F_CLIPBOARD,
    F_CODEPOINT_OFFSET,
    F_CODEUNIT_OFFSET,
    F_COLOR_NAMES,
    F_COMMAND_ARGUMENTS,
    F_COMMAND_KEY,
    F_COMMAND_NAME,
    F_COMMAND_NAMES,
    F_COMPOUND,
    F_COMPRESS,
    F_CONSTANT_NAMES,
    F_CONTROL_KEY,
    F_COPY_RESOURCE,
    F_COS,
    F_DATE,
    F_DATE_FORMAT,
    F_DECOMPRESS,
    F_DELETE_RESOURCE,
    F_DIRECTORIES,
    F_DISK_SPACE,
    F_DNS_SERVERS,
    F_DRAG_DESTINATION,
    F_DRAG_SOURCE,
    F_DRIVER_NAMES,
    F_DRIVES,
    F_DROP_CHUNK,
    F_ENCRYPT,
    F_ENVIRONMENT,
    F_EXISTS,
    F_EXP,
    F_EXP1,
    F_EXP10,
    F_EXP2,
    F_EXTENTS,
    F_FILES,
    F_FLUSH_EVENTS,
	// MDW-2014-08-23 : [[ feature_floor ]]
	F_FLOOR,
    F_FOCUSED_OBJECT,
    F_FONT_LANGUAGE,
    F_FONT_NAMES,
    F_FONT_SIZES,
    F_FONT_STYLES,
    F_FORMAT,
    F_FOUND_CHUNK,
    F_FOUND_FIELD,
    F_FOUND_LINE,
    F_FOUND_LOC,
    F_FOUND_TEXT,
    F_FRONT_SCRIPTS,
    F_FUNCTION_NAMES,
    F_GET_RESOURCE,
    F_GET_RESOURCES,
	// JS-2013-06-19: [[ StatsFunctions ]] Tag for 'geometricMean'
    F_GEO_MEAN,
    F_GLOBAL_LOC,
    F_GLOBALS,
	// JS-2013-06-19: [[ StatsFunctions ]] Tag for 'harmonicMean'
    F_HAR_MEAN,
    F_HAS_MEMORY,
    F_HEAP_SPACE,
    F_HA,
    F_HATON,
    F_HN,
    F_HNTOA,
    F_INTERRUPT,
    F_INTERSECT,
    F_IS_NUMBER,
    F_ISO_TO_MAC,
    F_ITEM_OFFSET,
    F_KEYS,
    F_KEYS_DOWN,
    F_LENGTH,
    F_LICENSED,
    F_LINE_OFFSET,
    F_LN,
    F_LN1,
    F_LOCAL_LOC,
    F_LOCALS,
    F_LOG10,
    F_LOG2,
    F_LONG_FILE_PATH,
    F_MAC_TO_ISO,
    F_MACHINE,
    F_MAIN_STACKS,
    F_MATCH_CHUNK,
    F_MATCH_TEXT,
    F_MATRIX_MULTIPLY,
    F_MAX,
    F_MCI_SEND_STRING,
    F_MD5_DIGEST,
    F_ME,
    F_MEDIAN,
    F_MENU_OBJECT,
    F_MENUS,
    F_MERGE,
    F_MILLISECS,
    F_MIN,
    F_MONTH_NAMES,
    F_MOUSE,
    F_MOUSE_CHAR,
    F_MOUSE_CHAR_CHUNK,
    F_MOUSE_CHUNK,
    F_MOUSE_CLICK,
    F_MOUSE_COLOR,
    F_MOUSE_CONTROL,
    F_MOUSE_H,
    F_MOUSE_LINE,
    F_MOUSE_LOC,
    F_MOUSE_STACK,
    F_MOUSE_TEXT,
    F_MOUSE_V,
    F_MOVIE,
    F_MOVING_CONTROLS,
    F_NATIVE_CHAR_TO_NUM,
    F_NUM_TO_CHAR,
    F_NUM_TO_NATIVE_CHAR,
    F_NUM_TO_UNICODE_CHAR,
	F_NUM_TO_BYTE,
    F_OFFSET,
    F_OPEN_FILES,
    F_OPEN_PROCESSES,
    F_OPEN_PROCESS_IDS,
    F_OPEN_SOCKETS,
    F_OPEN_STACKS,
    F_OPTION_KEY,
	// MW-2008-03-05: [[ Owner Reference ]] The 'owner' token is now a function - this is its
	//   tag.
	F_OWNER,
    F_PA,
    F_PARAGRAPH_OFFSET,
    F_PARAM,
    F_PARAMS,
    F_PARAM_COUNT,
    F_PENDING_MESSAGES,
    F_PLATFORM,
	// JS-2013-06-19: [[ StatsFunctions ]] Tag for 'populationStdDev'
    F_POP_STD_DEV,
	// JS-2013-06-19: [[ StatsFunctions ]] Tag for 'populationVariance'
    F_POP_VARIANCE,
    F_PROCESS_ID,
    F_PROCESSOR,
    F_PROPERTY_NAMES,
    F_QT_EFFECTS,
    F_QT_VERSION,
    F_QUERY_REGISTRY,
    F_RANDOM,
    F_RECORD_COMPRESSION_TYPES,
    F_RECORD_FORMATS,
    F_RECORD_LOUDNESS,
    F_REPLACE_TEXT,
    F_RESULT,
    F_ROUND,
	F_RUNTIME_ENVIRONMENTS, // RUNTIME ONLY
    F_SCREEN_COLORS,
    F_SCREEN_DEPTH,
    F_SCREEN_LOC,
    F_SCREEN_NAME,
    F_SCREEN_RECT,
		F_SCREEN_RECTS,
    F_SCREEN_TYPE,
    F_SCREEN_VENDOR,
    F_SCRIPT_LIMITS,
    F_SECONDS,
    F_SELECTED_BUTTON,
    F_SELECTED_CHUNK,
    F_SELECTED_FIELD,
    F_SELECTED_IMAGE,
    F_SELECTED_LINE,
    F_SELECTED_LOC,
    F_SELECTED_TEXT,
    F_SELECTED_OBJECT,
    F_SENTENCE_OFFSET,
    F_SET_REGISTRY,
    F_SET_RESOURCE,
    F_SHELL,
    F_SHIFT_KEY,
    F_SHORT_FILE_PATH,
    F_SIN,
	// JS-2013-06-19: [[ StatsFunctions ]] Tag for 'sampleStdDev' (was stdDev)
    F_SMP_STD_DEV,
	// JS-2013-06-19: [[ StatsFunctions ]] Tag for 'sampleVariance'
    F_SMP_VARIANCE,
    F_SOUND,
    F_SPECIAL_FOLDER_PATH,
    F_SQRT,
    F_STACKS,
    F_STACK_SPACE,
    F_STAT_ROUND,
    F_SUM,
    F_SYS_ERROR,
    F_SYSTEM_VERSION,
    F_TAN,
    F_TARGET,
    F_TEMP_NAME,
    F_TEMPLATE_BUTTON,
    F_TEMPLATE_CARD,
    F_TEMPLATE_FIELD,
    F_TEMPLATE_GROUP,
    F_TEMPLATE_IMAGE,
    F_TEMPLATE_GRAPHIC,
    F_TEMPLATE_EPS,
    F_TEMPLATE_SCROLLBAR,
    F_TEMPLATE_AUDIO_CLIP,
    F_TEMPLATE_VIDEO_CLIP,
    F_TEMPLATE_PLAYER,
    F_TEMPLATE_STACK,
    F_TEXT_DECODE,
    F_TEXT_ENCODE,
    F_TEXT_HEIGHT_SUM,
    F_TICKS,
    F_TIME,
    F_TO_LOWER,
    F_TO_UPPER,
    F_TOKEN_OFFSET,
    F_TOP_STACK,
    F_TRANSPOSE,
    F_TRUEWORD_OFFSET,
    F_TRUNC,
    F_UNICODE_CHAR_TO_NUM,
    F_UNI_DECODE,
    F_UNI_ENCODE,
    F_URL_DECODE,
    F_URL_ENCODE,
    F_URL_STATUS,
    F_VALUE,
    F_VARIABLES,
    F_VERSION,
    F_WAIT_DEPTH,
    F_WEEK_DAY_NAMES,
    F_WINDOWS,
    F_WITHIN,
    F_WORD_OFFSET,
    F_DELETE_REGISTRY,
	F_LIST_REGISTRY,
	F_HTTP_PROXY_FOR_URL,
	F_ARRAY_ENCODE,
	F_ARRAY_DECODE,
	F_RANDOM_BYTES,
	F_SHA1_DIGEST,
    F_MESSAGE_DIGEST,
	
	// MW-2012-10-08: [[ HitTest ]] New functions for returning control at a point.
	F_CONTROL_AT_LOC,
	F_CONTROL_AT_SCREEN_LOC,
	
	// MW-2013-05-08: [[ Uuid ]] New function for generating uuids.
	F_UUID,
    
    // MERG-2013-08-14: [[ MeasureText ]] Measure text relative to the effective font on an object
    F_MEASURE_TEXT,
    F_MEASURE_UNICODE_TEXT,
    
    F_NORMALIZE_TEXT,
    
    F_CODEPOINT_PROPERTY,
    
    F_VECTOR_DOT_PRODUCT,
    
    F_EVENT_CAPSLOCK_KEY,
    F_EVENT_COMMAND_KEY,
    F_EVENT_CONTROL_KEY,
    F_EVENT_OPTION_KEY,
    F_EVENT_SHIFT_KEY,
};

/* The HT_MIN and HT_MAX elements of the enum delimit the range of the handler
 * arrays in MCHandlerlst so iteration over the type should be
 * HT_MIN <= i <= HT_MAX */
enum Handler_type {
    
    HT_UNDEFINED = 0,

    HT_MIN,
    
    HT_MESSAGE = HT_MIN,
    HT_FUNCTION,
    HT_GETPROP,
    HT_SETPROP,
	
	// MW-2012-08-08: [[ BeforeAfter ]] New handler types - stored in separate
	//   lists in MCHandlerArray.
	HT_BEFORE,
	HT_AFTER,

    HT_PRIVATE,

    HT_MAX = HT_PRIVATE
};

enum If_format {
    IF_UNDEFINED,
    IF_ONELINE,
    IF_SINGLE,
    IF_MULTIPLE,
    IF_ELSEMULTIPLE
};

enum If_state {
    IS_UNDEFINED,
    IS_THEN,
    IS_ELSE
};

enum Try_state {
    TS_TRY,
    TS_CATCH,
    TS_FINALLY
};

enum Insert_point {
    IP_BACK,
    IP_FRONT
};

enum Is_type {
    IT_UNDEFINED,
    IT_AMONG,
    IT_NOT_AMONG,
    IT_NORMAL,
    IT_IN,
    IT_NOT,
    IT_NOT_IN,
    IT_WITHIN,
    IT_NOT_WITHIN,
	IT_AMONG_THE_DRAG_DATA,
	IT_NOT_AMONG_THE_DRAG_DATA,
	IT_AMONG_THE_CLIPBOARD_DATA,
	IT_NOT_AMONG_THE_CLIPBOARD_DATA,
    IT_AMONG_THE_RAW_CLIPBOARD_DATA,
    IT_NOT_AMONG_THE_RAW_CLIPBOARD_DATA,
    IT_AMONG_THE_RAW_DRAGBOARD_DATA,
    IT_NOT_AMONG_THE_RAW_DRAGBOARD_DATA,
    IT_AMONG_THE_FULL_CLIPBOARD_DATA,
    IT_NOT_AMONG_THE_FULL_CLIPBOARD_DATA,
    IT_AMONG_THE_FULL_DRAGBOARD_DATA,
    IT_NOT_AMONG_THE_FULL_DRAGBOARD_DATA,
    IT_STRICTLY,
    IT_NOT_STRICTLY,
};

enum Is_validation {
    IV_UNDEFINED,
    IV_AMONG,
    IV_COLOR,
    IV_DATE,
    IV_INTEGER,
    IV_LOGICAL,
    IV_NUMBER,
    IV_POINT,
    IV_RECT,
	IV_ARRAY,
	// MERG-2013-06-24: [[ IsAnAsciiString ]] Tag for 'ascii'.
    IV_ASCII,
    
    IV_STRING,
    IV_BINARY_STRING,
    IV_REAL,
};

enum Lock_constants {
    LC_UNDEFINED,
    LC_COLORMAP,
    LC_CURSOR,
    LC_ERRORS,
    LC_MENUS,
    LC_MSGS,
    LC_MOVES,
    LC_RECENT,
    LC_SCREEN,
	LC_SCREEN_FOR_EFFECT,
    LC_CLIPBOARD,
};

enum Mark_constants {
    MC_UNDEFINED,
    MC_ALL,
    MC_BY,
    MC_CARDS,
    MC_FINDING,
    MC_WHERE
};

// JS-2013-07-01: [[ EnhancedFilter ]] Tags for the type of pattern matcher to use.
enum Match_mode {
    MA_UNDEFINED,
    MA_WILDCARD,
    MA_REGEX,
    MA_EXPRESSION
};

enum Move_mode {
    MM_UNDEFINED,
    MM_MESSAGES,
    MM_WAITING
};

enum Open_argument {
    OA_UNDEFINED,
    OA_DIRECTORY,
    OA_DRIVER,
    OA_FILE,
    OA_OBJECT,
    OA_PRINTING,
    OA_PROCESS,
    OA_SOCKET,
    OA_STDERR,
    OA_STDIN,
    OA_STDOUT
};

enum Operators {
    O_UNDEFINED,
    O_GROUPING,
    O_NOT,
    O_NOT_BITS,
    O_POW,
    O_THERE,
    O_TIMES,
    O_OVER,
    O_DIV,
    O_MOD,
    O_PLUS,
    O_MINUS,
    O_CONCAT,
    O_CONCAT_SPACE,
    O_LT,
    O_LE,
    O_GE,
    O_GT,
    O_CONTAINS,
    O_NE,
    O_EQ,
    O_IS,
    O_AND_BITS,
    O_XOR_BITS,
    O_OR_BITS,
    O_AND,
    O_OR,
	O_WRAP,
	O_BEGINS_WITH,
	O_ENDS_WITH
};

// return codes from parsers
enum Parse_stat {
    PS_ERROR,
    PS_NORMAL,
    PS_EOL,
    PS_EOF,
    PS_NO_MATCH,
    PS_BREAK
};

enum Pixmap_ids {
    PI_NONE,
    PI_ARROW,
    PI_BRUSH,
    PI_SPRAY,
    PI_ERASER,
    PI_BUCKET,
    PI_BUSY,
    PI_CROSS,
    PI_HAND,
    PI_IBEAM,
    PI_LR,
    PI_PENCIL,
    PI_DROPPER,
    PI_PLUS,
    PI_WATCH,
    PI_HELP,
    PI_BUSY1,
    PI_BUSY2,
    PI_BUSY3,
    PI_BUSY4,
    PI_BUSY5,
    PI_BUSY6,
    PI_BUSY7,
    PI_BUSY8,
    PI_DRAGTEXT,
    PI_DRAGCLONE,
    PI_DRAGREFUSE,
    PI_SIZEV,
    PI_SIZEH,
    PI_NCURSORS,
    PI_BRUSHES = 100,
    PI_PATTERNS = 136,
    PI_END = 300
};

enum Play_params {
    PP_UNDEFINED,
    PP_AUDIO_CLIP,
    PP_BACK,
    PP_FORWARD,
    PP_LOOPING,
    PP_OPTIONS,
    PP_PAUSE,
    PP_PLAYER,
    PP_RESUME,
    PP_STEP,
    PP_STOP,
    PP_TEMPO,
    PP_VIDEO_CLIP,
	PP_VIDEO,
};

enum Record_params {
    RC_BEST,
    RC_BETTER,
    RC_GOOD,
    RC_PAUSE,
    RC_QUALITY,
    RC_RESUME,
    RC_SOUND
};

enum Preposition_type {
    PT_UNDEFINED,
    PT_AFTER,
    PT_AS,
    PT_AT,
    PT_BEFORE,
    PT_FROM,
    PT_IN,
    PT_INTO,
    PT_OF,
    PT_ON,
    PT_RELATIVE,
    PT_TO,
    PT_WITHOUT,
    PT_BY,
    PT_ALIGNED,
	PT_HEADER,
	PT_NEW_HEADER,
	PT_CONTENT,
	PT_MARKUP,
	PT_BINARY,
	PT_COOKIE,
	PT_NEWEST,
};

enum Print_mode {
    PM_UNDEFINED,
    PM_ALL,
    PM_BREAK,
    PM_CARD,
    PM_MARKED,
    PM_SOME,
	PM_ANCHOR,
	PM_LINK,
	PM_BOOKMARK,
	PM_UNICODE_BOOKMARK,
	PM_LINK_ANCHOR,
	PM_LINK_URL,
};

enum Properties {
    // not really properties
    P_UNDEFINED,
    P_SHORT,
    P_ABBREVIATE,
    P_LONG,
    P_INTERNET,
    P_EFFECTIVE,
    P_ENGLISH,
    P_SYSTEM,
	P_WORKING,
    // local properties
    P_CASE_SENSITIVE,
    P_FORM_SENSITIVE,
    P_CENTURY_CUTOFF,
    P_CONVERT_OCTALS,
    P_ITEM_DELIMITER,
    P_COLUMN_DELIMITER,
    P_LINE_DELIMITER,
	P_ROW_DELIMITER,
    P_NUMBER_FORMAT,
    P_WHOLE_MATCHES,
    P_USE_SYSTEM_DATE,
    P_USE_UNICODE,
    // global properties
    P_CURSOR,
    P_DEFAULT_CURSOR,
    P_DRAG_SPEED,
    P_MOVE_SPEED,
    P_EDIT_BACKGROUND,
    P_DEFAULT_STACK,
    P_STACK_FILES,
    P_LOCK_COLORMAP,
    P_LOCK_CURSOR,
    P_LOCK_ERRORS,
    P_LOCK_MENUS,
    P_LOCK_MESSAGES,
    P_LOCK_MOVES,
    P_LOCK_RECENT,
    P_LOCK_SCREEN,
	// MERG-2013-06-02: [[ GrpLckUpdates ]] Property tag for 'the lockUpdates' of groups.
    P_LOCK_UPDATES,
    P_BEEP_LOUDNESS,
    P_BEEP_PITCH,
    P_BEEP_DURATION,
	P_BEEP_SOUND, 
    P_PLAY_LOUDNESS,
    P_PLAY_DESTINATION,
    P_DIRECTORY,
    P_TWELVE_TIME,
    P_PRIVATE_COLORS,
    P_IDLE_RATE,
    P_IDLE_TICKS,
    // MERG-2014-06-02: [[ IgnoreMouseEvents ]] Property tag for 'the ignoreMouseEvents' of stacks.
    P_IGNORE_MOUSE_EVENTS,
    P_BLINK_RATE,
    P_RECURSION_LIMIT,
    P_REPEAT_RATE,
    P_REPEAT_DELAY,
    P_TYPE_RATE,
    P_SYNC_RATE,
    P_EFFECT_RATE,
    P_DONT_USE_NS,
    P_DONT_USE_QT,
    P_DONT_USE_QT_EFFECTS,
    P_DOUBLE_TIME,
    P_DOUBLE_DELTA,
    P_LONG_WINDOW_TITLES,
    P_BLIND_TYPING,
    P_POWER_KEYS,
    P_NAVIGATION_ARROWS,
    P_TEXT_ARROWS,
    P_EXTEND_KEY,
    P_COLORMAP,
    P_NO_PIXMAPS,
    P_POINTER_FOCUS,
    P_LOW_RESOLUTION_TIMERS,
    P_RAISE_MENUS,
    P_ACTIVATE_PALETTES,
    P_HIDE_PALETTES,
    P_RAISE_PALETTES,
    P_PROPORTIONAL_THUMBS,
    P_SHARED_MEMORY,
    P_VC_SHARED_MEMORY,
    P_VC_PLAYER,
    P_TRACE_ABORT,
    P_TRACE_DELAY,
    P_TRACE_RETURN,
    P_TRACE_STACK,
	P_TRACE_UNTIL,
    P_SHELL_COMMAND,
    
	P_PRINTER_NAMES,

	P_PRINT_COMMAND,
    P_PRINT_FONT_TABLE,
    
	P_PRINT_CARD_BORDERS,
    P_PRINT_GUTTERS,
    P_PRINT_MARGINS,
    P_PRINT_ROTATED,
    P_PRINT_ROWS_FIRST,
    P_PRINT_SCALE,

	P_PRINT_DEVICE_NAME, 
	P_PRINT_DEVICE_SETTINGS,
	P_PRINT_DEVICE_OUTPUT,
	P_PRINT_DEVICE_FEATURES,
	P_PRINT_DEVICE_RECTANGLE,
	
	P_PRINT_PAGE_SIZE,
    P_PRINT_PAGE_ORIENTATION,
	P_PRINT_PAGE_SCALE,
	P_PRINT_PAGE_RECTANGLE,
	
	P_PRINT_JOB_NAME,
	P_PRINT_JOB_COLOR,
	P_PRINT_JOB_COPIES,
	P_PRINT_JOB_COLLATE,
	P_PRINT_JOB_DUPLEX,
	P_PRINT_JOB_RANGES,
	P_PRINT_JOB_PAGE,
	
	P_PRINT_TEXT_ALIGN,
    P_PRINT_TEXT_FONT,
    P_PRINT_TEXT_HEIGHT,
    P_PRINT_TEXT_SIZE,
    P_PRINT_TEXT_STYLE,
	
	P_DIALOG_DATA,

    P_ACCEPT_DROP,
	P_ALLOWABLE_DRAG_ACTIONS,
    P_DRAG_DATA,
	P_DRAG_DELTA,
    P_DRAG_ACTION,
	P_DRAG_IMAGE,
	P_DRAG_IMAGE_OFFSET,
	
	P_CLIPBOARD_DATA,
    P_HC_IMPORT_STAT,
    P_SCRIPT_TEXT_FONT,
    P_SCRIPT_TEXT_SIZE,
    P_LOOK_AND_FEEL,
    P_SCREEN_MOUSE_LOC,
    P_SCREEN_GAMMA,
    P_UMASK,
    P_BUFFER_MODE,
    P_BUFFER_IMAGES,
    P_BACK_DROP,
    P_MULTI_EFFECT,
    P_ALLOW_INTERRUPTS,
    P_EXPLICIT_VARIABLES,
		P_PRESERVE_VARIABLES,
    P_SYSTEM_FS,
    P_SYSTEM_CS,
	P_SYSTEM_PS,
    P_FILE_TYPE,
    P_STACK_FILE_TYPE,
		P_STACK_FILE_VERSION,
    P_MIN_STACK_FILE_VERSION,
    P_SECURE_MODE,
    P_SERIAL_CONTROL_STRING,
    P_TOOL,
    P_EDIT_MENUS,
    P_EDIT_SCRIPTS,
    P_COLOR_WORLD,
    P_PEN_WIDTH,
    P_PEN_HEIGHT,
    P_ALLOW_KEY_IN_FIELD,
    P_REMAP_COLOR,
    P_HIDE_CONSOLE_WINDOWS,
    P_FTP_PROXY,
    P_HTTP_HEADERS,
    P_HTTP_PROXY,
    P_SHOW_INVISIBLES,
    P_SOCKET_TIMEOUT,
    P_RANDOM_SEED,
    P_DEFAULT_MENU_BAR,
    P_ACCENT_COLOR,
    P_JPEG_QUALITY,
    P_LZW_KEY,
    P_PAINT_COMPRESSION,
    P_EMACS_KEY_BINDINGS,
    P_SOUND_CHANNEL,
    P_RELAYER_GROUPED_CONTROLS,
    P_SELECT_GROUPED_CONTROLS,
    P_SELECTION_HANDLE_COLOR,
    P_SELECTION_MODE,
    P_WINDOW_BOUNDING_RECT,
    P_LINK_COLOR,
    P_LINK_HILITE_COLOR,
    P_LINK_VISITED_COLOR,
    P_UNDERLINE_LINKS,
    P_RECORDING,
    P_RECORD_RATE,
    P_RECORD_CHANNELS,
    P_RECORD_SAMPLESIZE,
    P_RECORD_COMPRESSION,
    P_RECORD_FORMAT,
    P_RECORD_INPUT,
    P_BREAK_POINTS,
    P_DEBUG_CONTEXT,
    P_EXECUTION_CONTEXTS,
    P_MESSAGE_MESSAGES,
    P_WATCHED_VARIABLES,
    P_LOG_MESSAGE,
    P_ALLOW_INLINE_INPUT,
    P_SSL_CERTIFICATES,
	P_HIDE_BACKDROP,
	P_QT_IDLE_RATE,
	P_RAISE_WINDOWS,
	P_PROCESS_TYPE,
    P_ERROR_MODE,
	P_ICON_MENU,
	P_STATUS_ICON,
	P_STATUS_ICON_MENU,
	P_STATUS_ICON_TOOLTIP,
	P_OUTPUT_TEXT_ENCODING,
	P_OUTPUT_LINE_ENDINGS,
	P_SESSION_SAVE_PATH,
	P_SESSION_LIFETIME,
	P_SESSION_COOKIE_NAME,
	P_SESSION_ID,

	P_SCRIPT_EXECUTION_ERRORS,
	P_SCRIPT_PARSING_ERRORS,
	P_DEFAULT_NETWORK_INTERFACE,

	/* 2013-01-07-IM global property to control image cache limit */
	P_IMAGE_CACHE_LIMIT,
	P_IMAGE_CACHE_USAGE,
	
    // read only globals
    P_ADDRESS,
    P_STACKS_IN_USE,
	P_NETWORK_INTERFACES,
    
  	// TD-2013-06-20: [[ DynamicFonts ]] global property for list of font files
    P_FONTFILES_IN_USE,
	
    // window properties
    P_NAME,
    P_SHORT_NAME,
    P_ABBREV_NAME,
    P_LONG_NAME,
    P_ID,
    P_SHORT_ID,
    P_ABBREV_ID,
    P_LONG_ID,
    P_ALT_ID,
    P_NUMBER,
    P_SHOW_BORDER,
    P_BOTTOM,
    P_BOTTOM_LEFT,
    P_BOTTOM_RIGHT,
    P_FORE_PIXEL,
    P_BACK_PIXEL,
    P_HILITE_PIXEL,
    P_BORDER_PIXEL,
    P_TOP_PIXEL,
    P_BOTTOM_PIXEL,
    P_SHADOW_PIXEL,
    P_FOCUS_PIXEL,
    P_FORE_COLOR,
    P_BACK_COLOR,
    P_HILITE_COLOR,
    P_BORDER_COLOR,
    P_TOP_COLOR,
    P_BOTTOM_COLOR,
    P_SHADOW_COLOR,
    P_FOCUS_COLOR,
    P_COLORS,
    P_FORE_PATTERN,
    P_BACK_PATTERN,
    P_HILITE_PATTERN,
    P_BORDER_PATTERN,
    P_TOP_PATTERN,
    P_BOTTOM_PATTERN,
    P_SHADOW_PATTERN,
    P_FOCUS_PATTERN,
    P_PATTERNS,
    P_HEIGHT,
    P_LEFT,
    P_LOCATION,
    P_LOCK_LOCATION,
    P_DISABLED,
    P_ENABLED,
    P_OPAQUE,
    P_RECTANGLE,
    P_RIGHT,
    P_SHADOW,
    P_SHADOW_OFFSET,
    P_3D,
    P_TOP,
    P_TOP_LEFT,
    P_TOP_RIGHT,
    P_INVISIBLE,
    P_VISIBLE,
    P_WIDTH,
    P_BORDER_WIDTH,
    P_SELECTED,
    // painting properties
    P_BRUSH,
    P_BRUSH_COLOR,
    P_BRUSH_PATTERN,
    P_BRUSH_BACK_COLOR,
    P_CENTERED,
    P_ERASER,
    P_FILLED,
    P_GRID,
    P_GRID_SIZE,
    P_LINE_SIZE,
    P_MULTIPLE,
    P_MULTI_SPACE,
    P_PEN_COLOR,
    P_PEN_PATTERN,
    P_PEN_BACK_COLOR,
    P_INK,
    P_POLY_SIDES,
    P_SLICES,
    P_SPRAY,
    P_ROUND_ENDS,
    P_TEXT_ALIGN,
    P_TEXT_FONT,
    P_TEXT_HEIGHT,
    P_TEXT_SIZE,
    P_TEXT_SHIFT,
	// MW-2011-11-23: [[ Array TextStyle ]] These are pseudo-properties
	//   used to express setting / unsetting of specific styles in field
	//   chunks.
	P_TEXT_STYLE_ADD,
	P_TEXT_STYLE_REMOVE,
    // stack properties
	P_REMOTEABLE, // RUNTIME only
	P_STACK_URL, // RUNTIME only
	P_FULLSCREEN, 
	// IM-2013-09-23: [[ FullscreenMode ]] Property tag for the fullscreenMode
	P_FULLSCREENMODE,
	// IM-2014-01-07: [[ StackScale ]] Property tag for the scalefactor
	P_SCALE_FACTOR,
    // MERG-2015-08-31: [[ ScriptOnly ]] Property tag for scriptOnly
    P_SCRIPT_ONLY,
    P_FILE_NAME,
    P_SAVE_COMPRESSED,
    P_USER_LEVEL,
    P_CANT_ABORT,
    P_CANT_PEEK,
    P_CANT_DELETE,
    P_CANT_SELECT,
    P_CANT_MODIFY,
    P_USER_MODIFY,
    P_SCRIPT,
    P_CLOSE_BOX,
    P_DRAGGABLE,
    P_RESIZABLE,
    P_ZOOM_BOX,
    P_MIN_WIDTH,
    P_MIN_HEIGHT,
    P_MAX_WIDTH,
    P_MAX_HEIGHT,
    P_ICONIC,
    P_START_UP_ICONIC,
    P_RECENT_CARDS,
    P_RECENT_NAMES,
    P_MAIN_STACK,
    P_SUBSTACKS,
    P_BACKGROUND_IDS,
    P_BACKGROUND_NAMES,
    P_CARD_IDS,
    P_CARD_NAMES,
    P_EXTERNALS,
    P_EXTERNAL_PACKAGES,
    P_EXTERNAL_COMMANDS,
    P_EXTERNAL_FUNCTIONS,
    P_DESTROY_STACK,
    P_DESTROY_WINDOW,
    P_ALWAYS_BUFFER,
    P_PASSWORD,
    P_KEY,
    P_MODE,
    P_WM_PLACE,
    P_PIXMAP_ID,
    P_WINDOW_ID,
    P_HC_ADDRESSING,
    P_HC_STACK,
    P_DYNAMIC_PATHS,
    P_DECORATIONS,
    P_SIZE,
    P_FREE_SIZE,
	P_OWNER,
    P_SHORT_OWNER,
    P_ABBREV_OWNER,
    P_LONG_OWNER,
    P_CUSTOM,
    P_CUSTOM_VAR,
    P_CUSTOM_PROPERTY_SET,
    P_CUSTOM_PROPERTY_SETS,
    P_PROPERTIES,
    P_TOOL_TIP,
	P_UNICODE_TOOL_TIP,
    P_TOOL_TIP_DELAY,
    P_MENU_BAR,
    P_CHARSET,
    P_FORMAT_FOR_PRINTING,
    P_WINDOW_SHAPE,
    P_METAL,
    P_SYSTEM_WINDOW,
    P_LIVE_RESIZING,
    P_MINIMIZE_BOX,
    P_MAXIMIZE_BOX,
    P_COLLAPSE_BOX,
	P_SCREEN,
	P_REFERRING_STACK, // DEVELOPMENT only
	P_UNPLACED_GROUP_IDS, // DEVELOPMENT only
	P_IDE_OVERRIDE, // DEVELOPMENT only
	P_CURRENT_CARD,
	P_MODIFIED_MARK,
	P_COMPOSITOR_TYPE,
	P_COMPOSITOR_CACHE_LIMIT,
	P_COMPOSITOR_TILE_SIZE,
	P_COMPOSITOR_STATISTICS,
	P_ACCELERATED_RENDERING,
    // group properties
    P_TAB_GROUP_BEHAVIOR,
    P_RADIO_BEHAVIOR,
    P_HILITED_BUTTON,
    P_HILITED_BUTTON_ID,
    P_HILITED_BUTTON_NAME,
    P_SHOW_PICT,
    P_BACK_SIZE,
    P_BACKGROUND_BEHAVIOR,
    P_BOUNDING_RECT,
	P_UNICODE_LABEL,
	P_UNBOUNDED_VSCROLL,
	P_UNBOUNDED_HSCROLL,
	P_SHARED_BEHAVIOR,
    // card properties
    P_MARKED,
    P_DEFAULT_BUTTON,
    P_GROUP_IDS,
    P_GROUP_NAMES,
	P_SHARED_GROUP_IDS,
	P_SHARED_GROUP_NAMES,
    // audioClip/videoClip/player properties
    P_DONT_REFRESH,
    P_FRAME_RATE,
    P_CALLBACKS,
    P_CURRENT_TIME,
    P_DURATION,
    P_LOOPING,
    P_MIRRORED,
    P_PLAY_RATE,
    P_SHOW_BADGE,
    P_SHOW_CONTROLLER,
    P_START_FRAME,
    P_END_FRAME,
    P_START_TIME,
    P_END_TIME,
    P_TIME_SCALE,
    P_PLAY_SELECTION,
    P_SHOW_SELECTION,
    P_PAUSED,
    P_STATUS,
    P_MOVIE_CONTROLLER_ID,
    P_MOVIE_LOADED_TIME,
    P_TRACK_COUNT,
    P_TRACKS,
    P_ENABLED_TRACKS,
    P_MEDIA_TYPES,
    P_CURRENT_NODE,
    P_NODES,
    P_ZOOM,
    P_TILT,
    P_PAN,
    P_CONSTRAINTS,
    P_HOT_SPOTS,
    P_LEFT_BALANCE,
    P_RIGHT_BALANCE,
    P_AUDIO_PAN,
    // EPS properties
    P_POSTSCRIPT,
    P_ANGLE,
    P_PROLOG,
    P_RETAIN_IMAGE,
    P_RETAIN_POSTSCRIPT,
    P_SCALE,
    P_SCALE_INDEPENDENTLY,
    P_X_SCALE,
    P_Y_SCALE,
    P_X_OFFSET,
    P_Y_OFFSET,
    P_X_EXTENT,
    P_Y_EXTENT,
    P_CURRENT_PAGE,
    P_PAGE_COUNT,
    // button properties
    P_LAYER,
    P_STYLE,
    P_TRAVERSAL_ON,
    P_SHOW_FOCUS_BORDER,
    P_AUTO_ARM,
    P_AUTO_HILITE,
    P_ARM_BORDER,
    P_ARM_FILL,
    P_HILITE_BORDER,
    P_HILITE_FILL,
    P_ARM,
    P_HILITE,
    P_ARMED_ICON,
    P_DISABLED_ICON,
    P_HILITED_ICON,
    P_ICON,
    P_VISITED_ICON,
	P_HOVER_ICON,
    P_SHARED_HILITE,
    P_SHOW_HILITE,
    P_SHOW_ICON,
    P_SHOW_NAME,
    P_MENU_BUTTON,
    P_MENU_HISTORY,
    P_MENU_LINES,
    P_MENU_MODE,
    P_MENU_NAME,
    P_ACCELERATOR_TEXT,
	P_UNICODE_ACCELERATOR_TEXT,
    P_ACCELERATOR_KEY,
    P_ACCELERATOR_MODIFIERS,
    P_MNEMONIC,
    P_MARGINS,
    P_LEFT_MARGIN,
    P_RIGHT_MARGIN,
    P_TOP_MARGIN,
    P_BOTTOM_MARGIN,
    P_DEFAULT,
    P_LABEL,
    P_LABEL_WIDTH,
    P_FAMILY,
    P_VISITED,
    // button menu item properties
    P_CHECK_MARK,
    P_COMMAND_CHAR,
    P_MARK_CHAR,
    P_MENU_MESSAGE,
    // image properties
    P_MAGNIFY,
    P_HOT_SPOT,
    P_XHOT,
    P_YHOT,
    P_IMAGE_PIXMAP_ID,
    P_MASK_PIXMAP_ID,
    P_ALPHA_DATA,
    P_IMAGE_DATA,
    P_MASK_DATA,
    P_DONT_DITHER,
    P_CURRENT_FRAME,
    P_FRAME_COUNT,
    P_REPEAT_COUNT,
    P_PALINDROME_FRAMES,
    P_CONSTANT_MASK,
    P_BLEND_LEVEL,
    P_RESIZE_QUALITY,
    // graphic properties
    P_DONT_RESIZE,
    P_POINTS,
    P_RELATIVE_POINTS,
    P_START_ANGLE,
    P_ARC_ANGLE,
    P_ROUND_RADIUS,
    P_ARROW_SIZE,
    P_MARKER_DRAWN,
    P_MARKER_OPAQUE,
    P_MARKER_LSIZE,
    P_MARKER_POINTS,
    P_DASHES,
    P_START_ARROW,
    P_END_ARROW,
	P_ANTI_ALIASED,
	P_FILL_RULE, // PROPERTY - FILL RULE
	P_EDIT_MODE,
	P_CAP_STYLE,
	P_JOIN_STYLE,
	P_MITER_LIMIT,
	// scrollbar properties
    P_THUMB_SIZE,
    P_THUMB_POS,
    P_LINE_INC,
    P_PAGE_INC,
    P_ORIENTATION,
    P_START_VALUE,
    P_END_VALUE,
    P_SHOW_VALUE,
    // field properties
    P_AUTO_TAB,
    P_DONT_SEARCH,
    P_DONT_WRAP,
    P_FIXED_HEIGHT,
    P_WIDE_MARGINS,
    P_FIRST_INDENT,
    P_LOCK_TEXT,
    P_SHARED_TEXT,
    P_SHOW_LINES,
    P_FORMATTED_LEFT,
    P_FORMATTED_TOP,
    P_FORMATTED_WIDTH,
    P_FORMATTED_HEIGHT,
    P_FORMATTED_RECT,
    P_HSCROLL,
    P_VSCROLL,
    P_HSCROLLBAR,
    P_VSCROLLBAR,
	P_HSCROLLBARID, // DEVELOPMENT only
	P_VSCROLLBARID, // DEVELOPMENT only
    P_SCROLLBAR_WIDTH,
    P_LIST_BEHAVIOR,
    P_TEXT,
    P_UNICODE_TEXT,
    P_HTML_TEXT,
    P_RTF_TEXT,
	// MW-2011-12-08: [[ StyledText ]] Property tag for the styledText
	P_STYLED_TEXT,
	// MW-2012-02-21: [[ LineBreaks ]] Property tag for the formattedStyledText
	P_FORMATTED_STYLED_TEXT,
	P_PLAIN_TEXT,
	P_UNICODE_PLAIN_TEXT,
    P_FORMATTED_TEXT,
	P_UNICODE_FORMATTED_TEXT,
    P_MULTIPLE_HILITES,
    P_NONCONTIGUOUS_HILITES,
    P_HILITED_LINES,
    P_TAB_STOPS,
    P_TOGGLE_HILITE,
    P_3D_HILITE,
    P_ALLOW_FIELD_REDRAW, //(sc compatibility)
    P_HGRID,
    P_VGRID,
    P_PAGE_HEIGHTS,
	// JS-2013-05-15: [[ PageRanges ]] Property tag for the pageranges property.
    P_PAGE_RANGES,
    P_LINK_TEXT,
    P_IMAGE_SOURCE,
	// MW-2012-01-06: [[ Block Metadata ]] Property tag for the metadata block property.
	P_METADATA,
	// MW-2012-01-09: [[ Field Indices ]] Property tag for chunk index properties.
	P_CHAR_INDEX,
	P_LINE_INDEX,
	P_LIST_STYLE,
	// MW-2012-01-25: [[ ParaStyles ]] New paragraph-level style properties.
	P_LIST_DEPTH,
	P_LIST_INDENT,
	// MW-2012-11-13: [[ ParaListIndex ]] New property allowing direct setting of the list index
	P_LIST_INDEX,
	P_SPACE_ABOVE,
	P_SPACE_BELOW,
	P_LEFT_INDENT,
	P_RIGHT_INDENT,
	// MW-2012-02-08: [[ ParaStyles ]] New 'padding' property for paragraphs.
	P_PADDING,
	// MW-2012-01-26: [[ FlaggedField ]] New property for char-level 'flagged'.
	P_FLAGGED,
	// MW-2012-02-08: [[ FlaggedField ]] New property for getting/setting lists of flagged
	//   ranges in fields.
	P_FLAGGED_RANGES,
	// MW-2012-02-10: [[ TabAlign ]] New property for setting alignment of tabs.
	P_TAB_ALIGN,
	// MW-2012-02-10: [[ TabWidths ]] New synthetic property to help manage setting of column
	//   widths.
	P_TAB_WIDTHS,
	// MW-2012-02-12: [[ Encoding ]] New read-only property describing a run's text encoding.
	P_ENCODING,
    // color palette properties
    P_SELECTED_COLOR,

    P_REV_LICENSE_LIMITS,
	P_REV_CRASH_REPORT_SETTINGS, // DEVELOPMENT only
	P_REV_AVAILABLE_HANDLERS, // DEVELOPMENT only
	P_MESSAGE_BOX_LAST_OBJECT,
	P_REV_LICENSE_INFO,

	P_REV_RUNTIME_BEHAVIOUR,
	
	P_URL_RESPONSE,
	P_PARENT_SCRIPT,

	P_SECURITY_PERMISSIONS, // RUNTIME only
	P_SECURITY_CATEGORIES, // RUNTIME only

	P_STACK_LIMIT,
	
	// MW-2011-08-25: [[ TileCache ]] The 'layerMode' property index.
	P_LAYER_MODE,
	
	// MW-2011-11-24: [[ Nice Folders ]] The (pseudo) properties for folder variants.
	P_ENGINE_FOLDER,
	P_TEMPORARY_FOLDER,
	P_DOCUMENTS_FOLDER,
	P_DESKTOP_FOLDER,
	P_HOME_FOLDER,

	// MW-2011-11-24: [[ UpdateScreen ]] Property controlling whether stack updates should
	//   occur after every command.
	P_DEFER_SCREEN_UPDATES,

	// MM-2012-09-05: [[ Property Listener ]] Property listing all the currently active object property listeners.
	P_REV_OBJECT_LISTENERS, // DEVELOPMENT only
	P_REV_PROPERTY_LISTENER_THROTTLE_TIME, // DEVELOPMENT only
	
	// MW-2012-11-13: [[ Bug 10516 ]] Tag for allowDatagramBroadcasts property.
	P_ALLOW_DATAGRAM_BROADCASTS,
    
    P_CONTROL_IDS,
    P_CONTROL_NAMES,
	P_CHILD_CONTROL_IDS,
    P_CHILD_CONTROL_NAMES,

	// MERG-2013-08-17: [[ ColorDialogColors ]] Custom color management for the windows color dialog
	P_COLOR_DIALOG_COLORS,
	
	// IM-2013-12-04: [[ PixelScale ]] Tags for the pixelScale and systemPixelScale properties
	P_PIXEL_SCALE,
	P_SYSTEM_PIXEL_SCALE,
	
	// IM-2014-01-24: [[ HiDPI ]] Tags for the usePixelScaling, screenPixelScale, and screenPixelScales properties
	P_USE_PIXEL_SCALING,
	P_SCREEN_PIXEL_SCALE,
	P_SCREEN_PIXEL_SCALES,
	
    // RTL/Bidirectional properties
    P_CURSORMOVEMENT,
    P_TEXTDIRECTION,
    
    // MW-2014-06-19: [[ ImageCenterRect ]] Tag for the centerRect property.
    P_CENTER_RECTANGLE,
    // MW-2014-06-19: [[ IconGravity ]] Tag for the button iconGravity property.
    P_ICON_GRAVITY,
    
    // MERG-2013-08-12: [[ ClipsToRect ]] If true group clips to the set rect rather than the rect of children
    P_CLIPS_TO_RECT,

    // MW-2014-08-12: [[ EditionType ]] Returns whether the engine is commercial or community
    P_EDITION_TYPE,
    
    // MERG-2015-10-11: [[ DocumentFilename ]] Property tag for documentFilename
    P_DOCUMENT_FILENAME,
    
    // ARRAY STYLE PROPERTIES
	P_FIRST_ARRAY_PROP,
    P_CUSTOM_KEYS = P_FIRST_ARRAY_PROP,
    P_CUSTOM_PROPERTIES,
	P_REV_AVAILABLE_VARIABLES, // DEVELOPMENT only
	P_GRADIENT_FILL,
	P_GRADIENT_STROKE,
	P_BITMAP_EFFECT_DROP_SHADOW,
	P_BITMAP_EFFECT_INNER_SHADOW,
	P_BITMAP_EFFECT_OUTER_GLOW,
	P_BITMAP_EFFECT_INNER_GLOW,
	P_BITMAP_EFFECT_COLOR_OVERLAY,
    P_TEXT_STYLE,
    
    // NATIVE CONTROL PROPERTIES
    P_URL,
    P_CAN_BOUNCE,
    P_SCROLLING_ENABLED,
    P_CAN_ADVANCE,
    P_CAN_RETREAT,
    P_ALPHA,
    P_BACKGROUND_COLOR,
    // SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]]
    P_IGNORE_VOICE_OVER_SENSITIVITY,
    P_MULTI_LINE,
    P_TEXT_COLOR,
    P_FONT_SIZE,
    P_FONT_NAME,
    P_EDITABLE,
    P_EDITING,
    P_AUTO_CAPITALIZATION_TYPE,
    P_AUTOCORRECTION_TYPE,
    P_KEYBOARD_TYPE,
    P_KEYBOARD_STYLE,
    P_RETURN_KEY_TYPE,
    P_CONTENT,
    P_CONTENT_TYPE,
    P_DATA_DETECTOR_TYPES,
    P_SELECTED_RANGE,
    P_NATURAL_SIZE,
    P_CONTENT_RECT,
    P_SHOW_HORIZONTAL_INDICATOR,
    P_SHOW_VERTICAL_INDICATOR,
    P_TRACKING,
    P_DRAGGING,
    P_AUTO_FIT,
    P_DELAY_REQUESTS,
    P_ALLOWS_INLINE_MEDIA_PLAYBACK,
    P_MEDIA_PLAYBACK_REQUIRES_USER_ACTION,
    P_MANAGE_RETURN_KEY,
    P_MINIMUM_FONT_SIZE,
    P_MAXIMUM_TEXT_LENGTH,
    P_AUTO_CLEAR,
    P_CLEAR_BUTTON_MODE,
    P_BORDER_STYLE,
    P_VERTICAL_TEXT_ALIGN,
    P_CAN_SCROLL_TO_TOP,
    P_CAN_CANCEL_TOUCHES,
    P_DELAY_TOUCHES,
    P_PAGING_ENABLED,
    P_DECELERATION_RATE,
    P_INDICATOR_STYLE,
    P_INDICATOR_INSETS,
    P_LOCK_DIRECTION,
    P_DECELERATING,
    P_PRESERVE_ASPECT,
    P_USE_APPLICATION_AUDIO_SESSION,
    P_SHOULD_AUTOPLAY,
    P_ALLOWS_AIR_PLAY,
    P_PLAYABLE_DURATION,
    P_IS_PREPARED_TO_PLAY,
    P_LOAD_STATE,
    P_PLAYBACK_STATE,
    // SN-2015-09-04: [[ Bug 9744 ]] readyForDisplay property added for players
    P_READY_FOR_DISPLAY,
    
    // MOBILE STORE PROPERTIES
    P_PRODUCT_IDENTIFIER,
    P_PURCHASE_QUANTITY,
    P_PURCHASE_DATE,
    P_TRANSACTION_IDENTIFIER,
    P_RECEIPT,
    P_ORIGINAL_PURCHASE_DATE,
    P_ORIGINAL_TRANSACTION_IDENTIFIER,
    P_ORIGINAL_RECEIPT,
    P_DEVELOPER_PAYLOAD,
    P_SIGNED_DATA,
    P_SIGNATURE,
    P_LOCALIZED_TITLE,
    P_LOCALIZED_DESCRIPTION,
    P_LOCALIZED_PRICE,
    P_KIND,

    // MW-2014-12-10: [[ Extensions ]] 'loadedExtensions' global property
    P_LOADED_EXTENSIONS,
    
    P_RAW_CLIPBOARD_DATA,
    P_RAW_DRAGBOARD_DATA,
    P_FULL_CLIPBOARD_DATA,
    P_FULL_DRAGBOARD_DATA,
    
    P_THEME,
    P_THEME_CONTROL_TYPE,
    
    P_SCRIPT_STATUS,
    
    P_LONG_NAME_NO_FILENAME,
    P_REV_SCRIPT_DESCRIPTION,
    P_REV_BEHAVIOR_USES,
    
    P_REV_LIBRARY_MAPPING,
    
    P_LAYER_CLIP_RECT,
	
	P_SYSTEM_APPEARANCE,
    
    __P_LAST,
};

enum Look_and_feel {
    LF_UNDEFINED,
    LF_AM,
    LF_MOTIF,
    LF_PM,
    LF_MAC,
    LF_WIN95,
    LF_NATIVEWIN,
    LF_NATIVEMAC,
    LF_NATIVEGTK
};

enum Relayer_relation
{
	RR_NONE,
	RR_BEFORE,
	RR_AFTER,
	RR_FRONT,
	RR_BACK
};

enum Repeat_form {
    RF_UNDEFINED,
    RF_EACH,
    RF_FOR,
    RF_FOREVER,
    RF_STEP,
    RF_UNTIL,
    RF_WHILE,
    RF_WITH,
    // SN-2015-06-18: [[ Bug 15509 ]] Parse 'times' in 'repeat for x times'
    RF_TIMES
};

enum Reset_type {
    RT_UNDEFINED,
    RT_CURSORS,
    RT_PAINT,
    RT_PRINTING,
    RT_TEMPLATE_AUDIO_CLIP,
    RT_TEMPLATE_BUTTON,
    RT_TEMPLATE_CARD,
    RT_TEMPLATE_EPS,
    RT_TEMPLATE_FIELD,
    RT_TEMPLATE_GRAPHIC,
    RT_TEMPLATE_GROUP,
    RT_TEMPLATE_IMAGE,
    RT_TEMPLATE_PLAYER,
    RT_TEMPLATE_SCROLLBAR,
    RT_TEMPLATE_STACK,
    RT_TEMPLATE_VIDEO_CLIP,
};


enum RSA_MODE
{
    RSA_DECRYPT,
    RSA_ENCRYPT,
    RSA_SIGN,
    RSA_VERIFY
};

enum RSA_KEYTYPE
{
    RSAKEY_PUBKEY,
    RSAKEY_CERT,
    RSAKEY_PRIVKEY
};

enum Script_point {
    SP_ACCEPT,
    SP_AE,
    SP_ASK,
    SP_COMMAND,
    SP_CONVERT,
    SP_ENCRYPTION,
    SP_EXIT,
    SP_EXPORT,
    SP_FACTOR,
    SP_FIND,
    SP_FLIP,
    SP_GO,
    SP_HANDLER,
    SP_INSERT,
    SP_LOCK,
    SP_MARK,
    SP_MODE,
    SP_MOVE,
    SP_OPEN,
    SP_PLAY,
    SP_RECORD,
    SP_REPEAT,
    SP_RESET,
    SP_SHOW,
    SP_SORT,
    SP_SSL,
    SP_START,
    SP_SUGAR,
    SP_THERE,
    SP_TOOL,
    SP_UNIT,
    SP_VALIDATION,
    SP_VISUAL,
	SP_SERVER
};

enum Show_object {
    SO_UNDEFINED,
    SO_ALL,
    SO_OBJECT,
    SO_BACKGROUND,
    SO_BREAK,
    SO_CARD,
    SO_GROUPS,
    SO_MARKED,
    SO_MENU,
    SO_MESSAGE,
    SO_PALETTE,
    SO_PICTURE,
    SO_TASKBAR,
    SO_TITLEBAR,
    SO_WINDOW
};

enum Sort_type {
    ST_UNDEF,
    ST_OF,
    ST_BY,
    ST_LINES,
    ST_ITEMS,
    ST_MARKED,
    ST_CARDS,
    ST_TEXT,
    ST_BINARY,
    ST_NUMERIC,
    ST_INTERNATIONAL,
    ST_DATETIME,
    ST_ASCENDING,
    ST_DESCENDING
};


enum SSL_constants {
    SSL_UNDEFINED,
    SSL_CERTIFICATE,
    SSL_VERIFICATION
};


enum Start_constants {
    SC_UNDEFINED,
    SC_DRAG,
    SC_EDITING,
    SC_MOVING,
    SC_PLAYER,
    SC_PLAYING,
    SC_RECORDING,
	SC_SESSION,
    SC_USING,
};

enum Sugar_constants {
	SG_UNDEFINED,
	SG_NOTHING,
	SG_BROWSER,
	SG_STANDARD,
	SG_OPTIMIZED,
	SG_OPTIONS,
	SG_ANCHOR,
	SG_LINK,
	
	// MM-2012-09-05: [[ Property Listener ]] Used by cancel listener for object
	SG_LISTENER,
	
	SG_ELEVATED,
	SG_BOOKMARK,
	SG_LEVEL,
	SG_EFFECTS,
	SG_UNICODE,
	SG_URL,
	SG_INITIALLY,
	SG_OPEN,
	SG_CLOSED,
	SG_CALLER,
	
	// MERG-2013-06-24: [[ IsAnAsciiString ]] Tag for 'string'.
    SG_STRING,
	
	// JS-2013-07-01: [[ EnhancedFilter ]] Tag for 'pattern'.
    SG_PATTERN,
	// JS-2013-07-01: [[ EnhancedFilter ]] Tag for 'regex'.
    SG_REGEX,
	// JS-2013-07-01: [[ EnhancedFilter ]] Tag for 'wildcard'.
    SG_WILDCARD,
	// JS-2013-07-01: [[ EnhancedFilter ]] Tag for 'matching'.
	SG_MATCHING,
    
    // MERG-2013-08-26: [[ RecursiveArrayOp ]] Support nested arrays in union and intersect
    SG_RECURSIVELY,
    
    // TD-2013-06-14: [[ DynamicFonts ]] start using font theFont [globally]
    SG_FONT,
    SG_GLOBALLY,
    SG_FILE,
	
	// MW-2013-11-14: [[ AssertCmd ]] Tags for sugar used in assert command.
	SG_TRUE,
	SG_FALSE,
	SG_SUCCESS,
	SG_FAILURE,
    
    // MW-2014-09-30: [[ ScriptOnlyStack ]] Tag for 'only' keyword in create command.
    SG_ONLY,
	
    // MM-2014-06-13: [[ Bug 12567 ]] Added host. Used in 'with verification for host <host>'
	SG_HOST,
    
    SG_EXTENSION,
	SG_RESOURCE,
	SG_PATH,
    
    // AL-2015-06-11: [[ Load Extension From Var ]] Add 'data' syntactic sugar
    SG_DATA,
    
    SG_STRICTLY,
    SG_REAL,
	
	SG_REPLACING,
	SG_PRESERVING,
	SG_STYLES,
    
    SG_URL_RESULT,
    SG_ERROR,
    SG_VALUE,
};

enum Statements {
    S_UNDEFINED,
    S_ACCEPT,
    S_ADD,
    S_ANSWER,
    S_ASK,
	// MW-2013-11-14: [[ AssertCmd ]] 'assert' command tag.
	S_ASSERT,
    S_BEEP,
    S_BREAK,
    S_BREAKPOINT,
    S_CALL,
    S_CANCEL,
    S_CHOOSE,
    S_CLICK,
    S_CLONE,
    S_CLOSE,
    S_COMBINE,
    S_COMPACT,
    S_CONSTANT,
    S_CONVERT,
    S_COPY,
    S_CREATE,
    S_CROP,
    S_CUT,
    S_DEBUGDO,
    S_DECRYPT,
    S_DEFINE,
    S_DELETE,
    S_DIFFERENCE,
    S_DISABLE,
	// MW-2008-11-05: [[ Dispatch Command ]] This is the 'dispatch' token's tag
	S_DISPATCH,
    S_DIVIDE,
    S_DO,
    S_DOMENU,
    S_DRAG,
    S_DRAWER,
	S_ECHO,
    S_EDIT,
    S_ENABLE,
    S_ENCRYPT,
    S_EXIT,
    S_EXPORT,
    S_FILTER,
    S_FIND,
    S_FLIP,
    S_FOCUS,
    S_GET,
    S_GLOBAL,
    S_GO,
    S_GRAB,
    S_GROUP,
    S_HIDE,
    S_HILITE,
    S_IF,
    S_IMPORT,
	S_INCLUDE,
    S_INSERT,
	S_INTERNAL, // DEVELOPMENT only
    S_INTERSECT,
    S_KILL,
    S_LAUNCH,
    S_LIBRARY,
    S_LOCAL,
    S_LOAD,
    S_LOCK,
    S_LOG,
    S_MARK,
    S_MODAL,
    S_MODELESS,
    S_MOVE,
    S_MULTIPLY,
    S_NEXT,
    S_OPEN,
    S_OPTION,
    S_PALETTE,
    S_PASS,
    S_PASTE,
    S_PLACE,
    S_PLAY,
    S_POP,
    S_POPUP,
    S_POST,
    S_PREPARE,
    S_PRINT,
    S_PULLDOWN,
    S_PUSH,
    S_PUT,
    S_QUIT,
    S_READ,
    S_RECORD,
    S_REDO,
	S_RELAYER,
    S_RELEASE,
    S_REMOVE,
    S_RENAME,
    S_REPEAT,
    S_REPLACE,
    S_REPLY,
    S_REQUEST,
	S_REQUIRE,
    S_RESET,
    // MERG-2013-09-23: [[ ResolveImage ]] resolve image [id] relative to <object>
	S_RESOLVE,
    S_RETURN,
    S_REVERT,
	S_REV_RELICENSE, // DEVELOPMENT only
    S_ROTATE,
    S_SAVE,
    S_SCRIPT_ERROR,
	// MM-2014-02-12: [[ SecureSocket ]] secure socket <socket> [with|without verification]
	S_SECURE,
    S_SEEK,
    S_SELECT,
    S_SEND,
    S_SET,
    S_SHEET,
    S_SHOW,
    S_SORT,
    S_SPLIT,
    S_START,
    S_STOP,
    S_SUBTRACT,
    S_SYMMETRIC,
    S_SWITCH,
    S_THROW,
    S_TOP_LEVEL,
    S_TRANSPOSE,
    S_TRY,
    S_TYPE,
    S_UNDEFINE,
    S_UNDO,
    S_UNGROUP,
    S_UNHILITE,
    S_UNION,
    S_UNLOAD,
    S_UNLOCK,
    S_UNMARK,
    S_VISUAL,
    S_WAIT,
    S_WRITE
};

// types of characters found by lex
// MW-2009-03-03: By making Symbol_type an enum many compilers will use a uint32_t
//   which is wasteful in the case of the ScriptPoint's type_table. Thus we make it
//   an anonymous enum, and use a uint8_t for that table. (Note we keep Symbol_type
//   a uint32_t since as a independent var, it makes more sense for it to be that).
typedef uint32_t Symbol_type;
enum {
    ST_UNDEFINED,
    ST_ERR,
    ST_EOF,
    ST_EOL,
    ST_SPC,
    ST_COM,
    ST_OP,
    ST_MIN,
    ST_NUM,
    ST_LP,
    ST_RP,
    ST_LB,
    ST_RB,
    ST_SEP,
    ST_SEMI,
    ST_ID,
    ST_ESC,
    ST_LIT,
	ST_LC,
    ST_RC,
    
	// MW-2009-03-03: The ST_DATA symbol type is a string that shoudl be treated
	//   as an echoed literal - it represents the data between <?rev ?> blocks in
	//   SERVER mode. ST_TAG is the type of '?' so we can identify '?>'.
	ST_DATA,
	ST_TAG
};

enum There_mode {
    TM_UNDEFINED,
    TM_DIRECTORY,
    TM_FILE,
    TM_PROCESS,
    TM_URL
};

// types returned from lex
enum Token_type {
    TT_UNDEFINED,
    TT_EOF,
    TT_NO_MATCH,
    TT_HANDLER,
    TT_STATEMENT,
    TT_CASE,
    TT_CATCH,
    TT_DEFAULT,
    TT_THEN,
    TT_ELSE,
    TT_END,
    TT_IT,
    TT_THE,
    TT_CHUNK,
    TT_FINALLY,
    TT_FUNCTION,
    TT_TOOL,
    TT_UNOP,
    TT_BINOP,
    TT_BIN_OR_UNOP,
    TT_LPAREN,
    TT_RPAREN,
    TT_PROPERTY,
    TT_PREP,
    TT_OF,
    TT_IN,
    TT_TO,
    TT_FROM,
    TT_CLASS,
    TT_VARIABLE,
    TT_VISUAL,
	TT_SERVER
};

enum Tool {
    T_UNDEFINED,
    T_BROWSE,
    T_BRUSH,
    T_BUCKET,
    T_BUTTON,
    T_CURVE,
    T_DROPPER,
    T_ERASER,
    T_FIELD,
    T_GRAPHIC,
    T_HELP,
    T_IMAGE,
    T_LASSO,
    T_LINE,
    T_OVAL,
    T_PENCIL,
    T_POINTER,
    T_POLYGON,
    T_RECTANGLE,
    T_REGULAR_POLYGON,
    T_ROUND_RECT,
    T_SCROLLBAR,
    T_PLAYER,
    T_BROWSER,
    T_SELECT,
    T_SPRAY,
    T_TEXT
};

// format of the value in a container
enum Value_format {
    VF_UNDEFINED,
    VF_STRING,
    VF_NUMBER,
    VF_BOTH,
    VF_ARRAY
};

enum Url_type {
    UT_UNDEFINED,
    UT_FILE,
    UT_BINFILE,
    UT_RESFILE,
    UT_HTTP,
    UT_MAIL,
    UT_FTP
};

enum Visual_effects {
    VE_UNDEFINED,
    VE_EFFECT,
    VE_EXTRA,
    VE_CLOSE,
    VE_OPEN,
    VE_IN,
    VE_OUT,
    VE_DOWN,
    VE_LEFT,
    VE_RIGHT,
    VE_UP,
    VE_BOTTOM,
    VE_CENTER,
    VE_TOP,
    VE_BARN,
    VE_DOOR,
    VE_CHECKERBOARD,
    VE_DISSOLVE,
    VE_IRIS,
    VE_PLAIN,
    VE_PUSH,
    VE_REVEAL,
    VE_SCROLL,
    VE_SHRINK,
    VE_STRETCH,
    VE_VENETIAN,
    VE_BLINDS,
    VE_WIPE,
    VE_ZOOM,
    VE_VERY,
    VE_VSLOW,
    VE_SLOW,
    VE_NORMAL,
    VE_FAST,
    VE_VFAST,
    VE_TO,
    VE_FROM,
    VE_BLACK,
    VE_CARD,
    VE_GRAY,
    VE_INVERSE,
    VE_WHITE,
    VE_WITH,
    VE_SOUND,
    VE_QTEFFECT,
    VE_OPTION,
	VE_CIEFFECT,
	VE_CURL,
	VE_FLIP,
};

enum Server_keywords
{
	SK_HEADER,
	SK_CONTENT,
	SK_MARKUP,
	SK_NEW,
	SK_UNICODE,
	SK_SECURE,
	SK_HTTPONLY,
};

enum MCExecResultMode
{
    kMCExecResultModeReturn,
    kMCExecResultModeReturnValue,
    kMCExecResultModeReturnError,
};

#include "parseerrors.h"
#include "executionerrors.h"

#endif
