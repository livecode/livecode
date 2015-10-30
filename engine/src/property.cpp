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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "scriptpt.h"
//#include "execpt.h"
#include "handler.h"
#include "chunk.h"
#include "image.h"
#include "aclip.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "field.h"
#include "scrolbar.h"
#include "button.h"
#include "player.h"
#include "stacklst.h"
#include "sellst.h"
#include "cardlst.h"
#include "dispatch.h"
#include "property.h"
#include "mcerror.h"
#include "util.h"
#include "date.h"
#include "printer.h"
#include "debug.h"
#include "funcs.h"
#include "parentscript.h"
#include "securemode.h"
#include "osspec.h"
#include "redraw.h"
#include "variable.h"

#include "ans.h"
#include "font.h"
#include "mctheme.h"

#include "globals.h"
#include "license.h"

#include "iquantization.h"

#include "regex.h"

#include "exec.h"
#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

static MCPropertyInfo kMCPropertyInfoTable[] =
{
	DEFINE_RW_PROPERTY(P_CASE_SENSITIVE, Bool, Engine, CaseSensitive)
    DEFINE_RW_PROPERTY(P_FORM_SENSITIVE, Bool, Engine, FormSensitive)
	DEFINE_RW_PROPERTY(P_CENTURY_CUTOFF, Int16, Engine, CenturyCutOff)
	DEFINE_RW_PROPERTY(P_CONVERT_OCTALS, Bool, Engine, ConvertOctals)
	DEFINE_RW_PROPERTY(P_ITEM_DELIMITER, String, Engine, ItemDelimiter)
	DEFINE_RW_PROPERTY(P_COLUMN_DELIMITER, String, Engine, ColumnDelimiter)
	DEFINE_RW_PROPERTY(P_ROW_DELIMITER, String, Engine, RowDelimiter)
	DEFINE_RW_PROPERTY(P_LINE_DELIMITER, String, Engine, LineDelimiter)
	DEFINE_RW_PROPERTY(P_WHOLE_MATCHES, Bool, Engine, WholeMatches)
	DEFINE_RW_PROPERTY(P_USE_SYSTEM_DATE, Bool, Engine, UseSystemDate)
	DEFINE_RW_PROPERTY(P_USE_UNICODE, Bool, Engine, UseUnicode)
	DEFINE_RW_CUSTOM_PROPERTY(P_NUMBER_FORMAT, EngineNumberFormat, Engine, NumberFormat)
	
	DEFINE_RO_SET_PROPERTY(P_PRINT_DEVICE_FEATURES, PrintingPrinterFeatures, Printing, PrintDeviceFeatures)
	DEFINE_RW_CUSTOM_PROPERTY(P_PRINT_DEVICE_OUTPUT, PrintingPrintDeviceOutput, Printing, PrintDeviceOutput)
	DEFINE_RW_ENUM_PROPERTY(P_PRINT_PAGE_ORIENTATION, PrintingPrinterOrientation, Printing, PrintPageOrientation)
	DEFINE_RW_CUSTOM_PROPERTY(P_PRINT_JOB_RANGES, PrintingPrinterPageRange, Printing, PrintJobRanges)
	
	DEFINE_RW_PROPERTY(P_PRINT_PAGE_SIZE, Int16X2, Printing, PrintPageSize)
	DEFINE_RW_PROPERTY(P_PRINT_PAGE_SCALE, Double, Printing, PrintPageScale)
	DEFINE_RO_PROPERTY(P_PRINT_PAGE_RECTANGLE, Rectangle, Printing, PrintPageRectangle)
	DEFINE_RW_PROPERTY(P_PRINT_JOB_NAME, String, Printing, PrintJobName)
	DEFINE_RW_PROPERTY(P_PRINT_JOB_COPIES, Int16, Printing, PrintJobCopies)
	DEFINE_RW_PROPERTY(P_PRINT_JOB_COLLATE, Bool, Printing, PrintJobCollate)
	DEFINE_RW_PROPERTY(P_PRINT_JOB_COLOR, Bool, Printing, PrintJobColor)
	DEFINE_RW_ENUM_PROPERTY(P_PRINT_JOB_DUPLEX, PrintingPrintJobDuplex, Printing, PrintJobDuplex)
    // SN-2014-09-17: [[ Bug 13467 ]] PrintPageNumber may return empty
	DEFINE_RO_PROPERTY(P_PRINT_JOB_PAGE, OptionalInt16, Printing, PrintJobPage)
	DEFINE_RO_PROPERTY(P_PRINT_DEVICE_RECTANGLE, Rectangle, Printing, PrintDeviceRectangle)
	DEFINE_RW_PROPERTY(P_PRINT_DEVICE_SETTINGS, BinaryString, Printing, PrintDeviceSettings)
	DEFINE_RW_PROPERTY(P_PRINT_DEVICE_NAME, String, Printing, PrintDeviceName)
	DEFINE_RW_PROPERTY(P_PRINT_FONT_TABLE, String, Printing, PrintFontTable)
	DEFINE_RW_PROPERTY(P_PRINT_GUTTERS, Int16X2, Printing, PrintGutters)
	DEFINE_RW_PROPERTY(P_PRINT_MARGINS, Int16X4, Printing, PrintMargins)
	DEFINE_RW_PROPERTY(P_PRINT_ROWS_FIRST, Bool, Printing, PrintRowsFirst)
	DEFINE_RW_PROPERTY(P_PRINT_SCALE, Double, Printing, PrintScale)
	DEFINE_RW_PROPERTY(P_PRINT_COMMAND, String, Printing, PrintCommand)
	DEFINE_RW_PROPERTY(P_PRINT_ROTATED, Bool, Printing, PrintRotated)
	DEFINE_RW_PROPERTY(P_PRINT_CARD_BORDERS, Bool, Printing, PrintCardBorders)
	DEFINE_RO_PROPERTY(P_PRINTER_NAMES, String, Printing, PrinterNames)

	DEFINE_RW_ENUM_PROPERTY(P_ERROR_MODE, ServerErrorMode, Server, ErrorMode)
	DEFINE_RW_ENUM_PROPERTY(P_OUTPUT_LINE_ENDINGS, ServerOutputLineEndings, Server, OutputLineEnding)
	DEFINE_RW_ENUM_PROPERTY(P_OUTPUT_TEXT_ENCODING, ServerOutputTextEncoding, Server, OutputTextEncoding)
	DEFINE_RW_PROPERTY(P_SESSION_SAVE_PATH, String, Server, SessionSavePath)
	DEFINE_RW_PROPERTY(P_SESSION_LIFETIME, UInt32, Server, SessionLifetime)
	DEFINE_RW_PROPERTY(P_SESSION_COOKIE_NAME, String, Server, SessionCookieName)
	DEFINE_RW_PROPERTY(P_SESSION_ID, String, Server, SessionId)

	DEFINE_RO_PROPERTY(P_SCRIPT_EXECUTION_ERRORS, String, Engine, ScriptExecutionErrors)
	DEFINE_RO_PROPERTY(P_SCRIPT_PARSING_ERRORS, String, Engine, ScriptParsingErrors)
	
	DEFINE_RW_PROPERTY(P_REV_RUNTIME_BEHAVIOUR, UInt16, Legacy, RevRuntimeBehaviour)
	DEFINE_RW_PROPERTY(P_HC_IMPORT_STAT, String, Legacy, HcImportStat)
	DEFINE_RW_PROPERTY(P_SCRIPT_TEXT_FONT, String, Legacy, ScriptTextFont)
	DEFINE_RW_PROPERTY(P_SCRIPT_TEXT_SIZE, UInt16, Legacy, ScriptTextSize)

	DEFINE_RW_PROPERTY(P_DIALOG_DATA, Any, Interface, DialogData)

	DEFINE_RW_ENUM_PROPERTY(P_LOOK_AND_FEEL, InterfaceLookAndFeel, Interface, LookAndFeel)
	DEFINE_RW_PROPERTY(P_SCREEN_MOUSE_LOC, Point, Interface, ScreenMouseLoc)
	DEFINE_RW_CUSTOM_PROPERTY(P_BACK_DROP, InterfaceBackdrop, Interface, Backdrop)
	DEFINE_RW_PROPERTY(P_BUFFER_IMAGES, Bool, Interface, BufferImages)
	DEFINE_RW_PROPERTY(P_SYSTEM_FS, Bool, Interface, SystemFileSelector)
	DEFINE_RW_PROPERTY(P_SYSTEM_CS, Bool, Interface, SystemColorSelector)
	DEFINE_RW_PROPERTY(P_SYSTEM_PS, Bool, Interface, SystemPrintSelector)
	DEFINE_RW_PROPERTY(P_UMASK, UInt16, Files, UMask)
	DEFINE_RW_PROPERTY(P_FILE_TYPE, String, Files, FileType)
	DEFINE_RW_PROPERTY(P_ALLOW_INTERRUPTS, Bool, Engine, AllowInterrupts)
	DEFINE_RW_PROPERTY(P_EXPLICIT_VARIABLES, Bool, Engine, ExplicitVariables)
	DEFINE_RW_PROPERTY(P_PRESERVE_VARIABLES, Bool, Engine, PreserveVariables)

	DEFINE_RW_PROPERTY(P_RECORD_SAMPLESIZE, UInt16, Multimedia, RecordSampleSize)
	DEFINE_RW_PROPERTY(P_RECORD_RATE, Double, Multimedia, RecordRate)
	DEFINE_RW_PROPERTY(P_RECORD_CHANNELS, UInt16, Multimedia, RecordChannels)
	DEFINE_RW_ENUM_PROPERTY(P_RECORD_FORMAT, MultimediaRecordFormat, Multimedia, RecordFormat) 
	DEFINE_RW_PROPERTY(P_RECORD_COMPRESSION, String, Multimedia, RecordCompression)
	DEFINE_RW_PROPERTY(P_RECORD_INPUT, String, Multimedia, RecordInput)

	DEFINE_RW_ENUM_PROPERTY(P_DRAG_ACTION, PasteboardDragAction, Pasteboard, DragAction)
	DEFINE_RW_PROPERTY(P_ACCEPT_DROP, Bool, Pasteboard, AcceptDrop)
    // SN-2014-10-07: [[ Bug 13610 ]] The id passed to MCPasteBoardSet/GetDragImage is a uint32_t
	DEFINE_RW_PROPERTY(P_DRAG_IMAGE, UInt32, Pasteboard, DragImage)
	DEFINE_RW_PROPERTY(P_DRAG_IMAGE_OFFSET, OptionalPoint, Pasteboard, DragImageOffset)
	DEFINE_RW_SET_PROPERTY(P_ALLOWABLE_DRAG_ACTIONS, PasteboardAllowableDragActions, Pasteboard, AllowableDragActions)
	DEFINE_RW_PROPERTY(P_ALLOW_INLINE_INPUT, Bool, Interface, AllowInlineInput)
	DEFINE_RW_PROPERTY(P_DRAG_DELTA, UInt16, Interface, DragDelta)
	DEFINE_RW_PROPERTY(P_STACK_FILE_TYPE, String, Interface, StackFileType)
	
	DEFINE_RW_PROPERTY(P_ICON_MENU, String, Interface, IconMenu)
	DEFINE_RW_PROPERTY(P_STATUS_ICON, UInt32, Interface, StatusIcon)
	DEFINE_RW_PROPERTY(P_STATUS_ICON_TOOLTIP, String, Interface, StatusIconToolTip)
	DEFINE_RW_PROPERTY(P_STATUS_ICON_MENU, String, Interface, StatusIconMenu)
	DEFINE_RW_ENUM_PROPERTY(P_PROCESS_TYPE, InterfaceProcessType, Interface, ProcessType)
	DEFINE_RW_PROPERTY(P_STACK_LIMIT, UInt32, Engine, StackLimit)
	DEFINE_RO_EFFECTIVE_PROPERTY(P_STACK_LIMIT, UInt32, Engine, StackLimit)
	DEFINE_RW_PROPERTY(P_IMAGE_CACHE_LIMIT, UInt32, Graphics, ImageCacheLimit)
	DEFINE_RO_PROPERTY(P_IMAGE_CACHE_USAGE, UInt32, Graphics, ImageCacheUsage)
	DEFINE_RW_PROPERTY(P_ALLOW_DATAGRAM_BROADCASTS, Bool, Network, AllowDatagramBroadcasts)
	
	DEFINE_RW_PROPERTY(P_BRUSH_BACK_COLOR, Any, Interface, BrushBackColor)
	DEFINE_RW_PROPERTY(P_PEN_BACK_COLOR, Any, Interface, PenBackColor)
	DEFINE_RW_CUSTOM_PROPERTY(P_BRUSH_COLOR, InterfaceNamedColor, Interface, BrushColor)
	DEFINE_RW_CUSTOM_PROPERTY(P_PEN_COLOR, InterfaceNamedColor, Interface, PenColor)
	DEFINE_RW_PROPERTY(P_BRUSH_PATTERN, UInt16, Interface, BrushPattern)
	DEFINE_RW_PROPERTY(P_PEN_PATTERN, UInt16, Interface, PenPattern)
	DEFINE_RW_PROPERTY(P_FILLED, Bool, Interface, Filled)
	DEFINE_RW_PROPERTY(P_POLY_SIDES, UInt16, Interface, PolySides)
	DEFINE_RW_PROPERTY(P_LINE_SIZE, UInt16, Interface, LineSize)
	DEFINE_RW_PROPERTY(P_PEN_WIDTH, UInt16, Interface, LineSize)
	DEFINE_RW_PROPERTY(P_PEN_HEIGHT, UInt16, Interface, LineSize)
	DEFINE_RW_PROPERTY(P_ROUND_RADIUS, UInt16, Interface, RoundRadius)
	DEFINE_RW_PROPERTY(P_START_ANGLE, UInt16, Interface, StartAngle)
	DEFINE_RW_PROPERTY(P_ARC_ANGLE, UInt16, Interface, ArcAngle)
	DEFINE_RW_PROPERTY(P_ROUND_ENDS, Bool, Interface, RoundEnds)
	DEFINE_RW_PROPERTY(P_DASHES, ItemsOfUInt, Interface, Dashes)
	
	DEFINE_RW_PROPERTY(P_EDIT_BACKGROUND, Bool, Interface, EditBackground)
	
	DEFINE_RW_PROPERTY(P_LOCK_SCREEN, Bool, Interface, LockScreen)
	
	DEFINE_RO_PROPERTY(P_RECENT_CARDS, String, Interface, RecentCards)
	DEFINE_RO_PROPERTY(P_RECENT_NAMES, String, Interface, RecentNames)
	
	DEFINE_RO_PROPERTY(P_TEXT_ALIGN, Any, Legacy, TextAlign)
	DEFINE_RW_PROPERTY(P_TEXT_FONT, Any, Legacy, TextFont)
	DEFINE_RW_PROPERTY(P_TEXT_HEIGHT, Any, Legacy, TextHeight)
	DEFINE_RW_PROPERTY(P_TEXT_SIZE, Any, Legacy, TextSize)
	DEFINE_RW_PROPERTY(P_TEXT_STYLE, Any, Legacy, TextStyle)
	DEFINE_RW_PROPERTY(P_PRINT_TEXT_ALIGN, Any, Legacy, TextAlign)
	DEFINE_RW_PROPERTY(P_PRINT_TEXT_FONT, Any, Legacy, TextFont)
	DEFINE_RW_PROPERTY(P_PRINT_TEXT_HEIGHT, Any, Legacy, TextHeight)
	DEFINE_RW_PROPERTY(P_PRINT_TEXT_SIZE, Any, Legacy, TextSize)
	DEFINE_RW_PROPERTY(P_PRINT_TEXT_STYLE, Any, Legacy, TextStyle)
	
	DEFINE_RW_ENUM_PROPERTY(P_PLAY_DESTINATION, InterfacePlayDestination, Multimedia, PlayDestination)
	DEFINE_RW_PROPERTY(P_PLAY_LOUDNESS, UInt16, Multimedia, PlayLoudness)
	
	DEFINE_RW_PROPERTY(P_STACK_FILES, String, Legacy, StackFiles)
    // SN-2014-09-01: [[ Bug 13300 ]] Updated 'set the menubar' to have a (useless) setter at the global scope 
	DEFINE_RW_PROPERTY(P_MENU_BAR, String, Legacy, MenuBar)
	DEFINE_RW_PROPERTY(P_EDIT_MENUS, Bool, Legacy, EditMenus)
	DEFINE_RW_PROPERTY(P_BUFFER_MODE, String, Legacy, BufferMode)
	DEFINE_RW_PROPERTY(P_MULTI_EFFECT, Bool, Legacy, MultiEffect)
	DEFINE_RW_PROPERTY(P_USER_LEVEL, UInt16, Legacy, UserLevel)
	DEFINE_RW_PROPERTY(P_USER_MODIFY, Bool, Legacy, UserModify)

	DEFINE_RW_CUSTOM_PROPERTY(P_ACCENT_COLOR, InterfaceNamedColor, Interface, AccentColor)
	DEFINE_RW_CUSTOM_PROPERTY(P_HILITE_COLOR, InterfaceNamedColor, Interface, HiliteColor)
	DEFINE_RW_ENUM_PROPERTY(P_PAINT_COMPRESSION, InterfacePaintCompression, Interface, PaintCompression)
	DEFINE_RW_CUSTOM_PROPERTY(P_LINK_COLOR, InterfaceNamedColor, Interface, LinkColor)
	DEFINE_RW_CUSTOM_PROPERTY(P_LINK_HILITE_COLOR, InterfaceNamedColor, Interface, LinkHiliteColor)
	DEFINE_RW_CUSTOM_PROPERTY(P_LINK_VISITED_COLOR, InterfaceNamedColor, Interface, LinkVisitedColor)
	DEFINE_RW_PROPERTY(P_UNDERLINE_LINKS, Bool, Interface, UnderlineLinks)
	
	DEFINE_RW_PROPERTY(P_SELECT_GROUPED_CONTROLS, Bool, Interface, SelectGroupedControls)
	
	DEFINE_RW_PROPERTY(P_ICON, UInt32, Interface, Icon)
	DEFINE_RW_PROPERTY(P_SHOW_INVISIBLES, Bool, Interface, ShowInvisibles)

	DEFINE_RO_PROPERTY(P_URL_RESPONSE, String, Network, UrlResponse)
	DEFINE_RW_PROPERTY(P_FTP_PROXY, String, Network, FtpProxy)
	DEFINE_RW_PROPERTY(P_HTTP_PROXY, String, Network, HttpProxy)
	DEFINE_RW_PROPERTY(P_HTTP_HEADERS, String, Network, HttpHeaders)
	DEFINE_RW_PROPERTY(P_SOCKET_TIMEOUT, Double, Network, SocketTimeout)

	DEFINE_RW_PROPERTY(P_SECURE_MODE, Bool, Engine, SecureMode)
	DEFINE_RO_SET_PROPERTY(P_SECURITY_CATEGORIES, EngineSecurityCategories, Engine, SecurityCategories)
	DEFINE_RW_SET_PROPERTY(P_SECURITY_PERMISSIONS, EngineSecurityCategories, Engine, SecurityPermissions)

	DEFINE_RW_PROPERTY(P_SERIAL_CONTROL_STRING, String, Files, SerialControlString)
	DEFINE_RW_PROPERTY(P_HIDE_CONSOLE_WINDOWS, Bool, Files, HideConsoleWindows)

    // AL-2014-05-23: [[ Bug 12494 ]] Random seed is a 32-bit integer
	DEFINE_RW_PROPERTY(P_RANDOM_SEED, Int32, Math, RandomSeed)

	DEFINE_RW_PROPERTY(P_LOCK_CURSOR, Bool, Interface, LockCursor)
	DEFINE_RW_PROPERTY(P_LOCK_ERRORS, Bool, Interface, LockErrors)
	DEFINE_RW_PROPERTY(P_LOCK_MENUS, Bool, Interface, LockMenus)
	DEFINE_RW_PROPERTY(P_LOCK_MESSAGES, Bool, Interface, LockMessages)
	DEFINE_RW_PROPERTY(P_LOCK_MOVES, Bool, Interface, LockMoves)
	DEFINE_RW_PROPERTY(P_LOCK_RECENT, Bool, Interface, LockRecent)
	DEFINE_RW_PROPERTY(P_DEFAULT_MENU_BAR, Name, Interface, DefaultMenubar)
	DEFINE_RW_CUSTOM_PROPERTY(P_STACK_FILE_VERSION, InterfaceStackFileVersion, Interface, StackFileVersion)
	DEFINE_RW_PROPERTY(P_DEFAULT_STACK, String, Interface, DefaultStack)
    DEFINE_RW_PROPERTY(P_DEFAULT_CURSOR, UInt32, Interface, DefaultCursor)
    DEFINE_RW_PROPERTY(P_CURSOR, OptionalUInt32, Interface, Cursor)

	DEFINE_RW_PROPERTY(P_TWELVE_TIME, Bool, DateTime, TwelveTime)

	DEFINE_RW_PROPERTY(P_QT_IDLE_RATE, UInt16, Multimedia, QtIdleRate)
	DEFINE_RW_PROPERTY(P_DONT_USE_QT, Bool, Multimedia, DontUseQt)
	DEFINE_RW_PROPERTY(P_DONT_USE_QT_EFFECTS, Bool, Multimedia, DontUseQtEffects)
	
	// PM-2015-07-15: [[ Bug 15602 ]] Use 32-bit number for 'recursionLimit' property
	DEFINE_RW_PROPERTY(P_RECURSION_LIMIT, UInt32, Engine, RecursionLimit)

	DEFINE_RW_PROPERTY(P_IDLE_RATE, UInt16, Interface, IdleRate)
	DEFINE_RW_PROPERTY(P_IDLE_TICKS, UInt16, Interface, IdleTicks)
	DEFINE_RW_PROPERTY(P_BLINK_RATE, UInt16, Interface, BlinkRate)
	DEFINE_RW_PROPERTY(P_REPEAT_RATE, UInt16, Interface, RepeatRate)
	DEFINE_RW_PROPERTY(P_REPEAT_DELAY, UInt16, Interface, RepeatDelay)
	DEFINE_RW_PROPERTY(P_TYPE_RATE, UInt16, Interface, TypeRate)
	DEFINE_RW_PROPERTY(P_SYNC_RATE, UInt16, Interface, SyncRate)
	DEFINE_RW_PROPERTY(P_EFFECT_RATE, UInt16, Interface, EffectRate)
	DEFINE_RW_PROPERTY(P_DOUBLE_DELTA, UInt16, Interface, DoubleDelta)
	DEFINE_RW_PROPERTY(P_DOUBLE_TIME, UInt16, Interface, DoubleTime)
	DEFINE_RW_PROPERTY(P_TOOL_TIP_DELAY, UInt16, Interface, TooltipDelay)

	DEFINE_RW_PROPERTY(P_LONG_WINDOW_TITLES, Bool, Legacy, LongWindowTitles)
	DEFINE_RW_PROPERTY(P_BLIND_TYPING, Bool, Legacy, BlindTyping)
	DEFINE_RW_PROPERTY(P_POWER_KEYS, Bool, Legacy, PowerKeys)
	DEFINE_RW_PROPERTY(P_TEXT_ARROWS, Bool, Legacy, TextArrows)
	DEFINE_RW_PROPERTY(P_NO_PIXMAPS, Bool, Legacy, NoPixmaps)
	DEFINE_RW_PROPERTY(P_LOW_RESOLUTION_TIMERS, Bool, Legacy, LowResolutionTimers)
	DEFINE_RW_PROPERTY(P_COLORMAP, String, Legacy, Colormap)

	DEFINE_RW_PROPERTY(P_NAVIGATION_ARROWS, Bool, Interface, NavigationArrows)
	DEFINE_RW_PROPERTY(P_EXTEND_KEY, UInt16, Interface, ExtendKey)
	DEFINE_RW_PROPERTY(P_POINTER_FOCUS, Bool, Interface, PointerFocus)
	DEFINE_RW_PROPERTY(P_EMACS_KEY_BINDINGS, Bool, Interface, EmacsKeyBindings)
	DEFINE_RW_PROPERTY(P_RAISE_MENUS, Bool, Interface, RaiseMenus)
	DEFINE_RW_PROPERTY(P_ACTIVATE_PALETTES, Bool, Interface, ActivatePalettes)
	DEFINE_RW_PROPERTY(P_HIDE_PALETTES, Bool, Interface, HidePalettes)
	DEFINE_RW_PROPERTY(P_RAISE_PALETTES, Bool, Interface, RaisePalettes)
	DEFINE_RW_PROPERTY(P_RAISE_WINDOWS, Bool, Interface, RaiseWindows)
	DEFINE_RW_PROPERTY(P_HIDE_BACKDROP, Bool, Interface, HideBackdrop)
	DEFINE_RW_PROPERTY(P_DONT_USE_NS, Bool, Interface, DontUseNavigationServices)

	DEFINE_RW_PROPERTY(P_PROPORTIONAL_THUMBS, Bool, Interface, ProportionalThumbs)
	DEFINE_RW_PROPERTY(P_SHARED_MEMORY, Bool, Interface, SharedMemory)
	DEFINE_RW_PROPERTY(P_SCREEN_GAMMA, Double, Interface, ScreenGamma)
	DEFINE_RW_ENUM_PROPERTY(P_SELECTION_MODE, InterfaceSelectionMode, Interface, SelectionMode)
	DEFINE_RW_CUSTOM_PROPERTY(P_SELECTION_HANDLE_COLOR, InterfaceNamedColor, Interface, SelectionHandleColor)
	DEFINE_RW_PROPERTY(P_WINDOW_BOUNDING_RECT, Rectangle, Interface, WindowBoundingRect)
	DEFINE_RW_PROPERTY(P_JPEG_QUALITY, UInt16, Interface, JpegQuality)
	DEFINE_RW_PROPERTY(P_RELAYER_GROUPED_CONTROLS, Bool, Interface, RelayerGroupedControls)

	DEFINE_RW_PROPERTY(P_VC_SHARED_MEMORY, Bool, Legacy, VcSharedMemory)
	DEFINE_RW_PROPERTY(P_VC_PLAYER, String, Legacy, VcPlayer)
	DEFINE_RW_PROPERTY(P_SOUND_CHANNEL, UInt16, Legacy, SoundChannel)
	DEFINE_RW_PROPERTY(P_LZW_KEY, String, Legacy, LzwKey)
	DEFINE_RW_PROPERTY(P_MULTIPLE, Bool, Legacy, Multiple)
	DEFINE_RW_PROPERTY(P_MULTI_SPACE, UInt16, Legacy, MultiSpace)

	DEFINE_RO_PROPERTY(P_ADDRESS, String, Engine, Address)
	DEFINE_RO_PROPERTY(P_STACKS_IN_USE, String, Engine, StacksInUse)
    // TD-2013-06-20: [[ DynamicFonts ]] global property for list of font files
    DEFINE_RO_PROPERTY(P_FONTFILES_IN_USE, LinesOfString, Text, FontfilesInUse)

	DEFINE_RW_PROPERTY(P_SHELL_COMMAND, String, Files, ShellCommand)
	DEFINE_RW_PROPERTY(P_DIRECTORY, String, Files, CurrentFolder)
	DEFINE_RO_PROPERTY(P_ENGINE_FOLDER, String, Files, EngineFolder)
	DEFINE_RO_PROPERTY(P_HOME_FOLDER, String, Files, HomeFolder)	
	DEFINE_RO_PROPERTY(P_DOCUMENTS_FOLDER, String, Files, DocumentsFolder)
	DEFINE_RO_PROPERTY(P_DESKTOP_FOLDER, String, Files, DesktopFolder)
	DEFINE_RO_PROPERTY(P_TEMPORARY_FOLDER, String, Files, TemporaryFolder)

	DEFINE_RW_PROPERTY(P_RECORDING, Bool, Multimedia, Recording)

	DEFINE_RW_PROPERTY(P_SSL_CERTIFICATES, String, Security, SslCertificates)

	DEFINE_RW_PROPERTY(P_TRACE_ABORT, Bool, Debugging, TraceAbort)
	DEFINE_RW_PROPERTY(P_TRACE_DELAY, UInt16, Debugging, TraceDelay)
	DEFINE_RW_PROPERTY(P_TRACE_RETURN, Bool, Debugging, TraceReturn)
	DEFINE_RW_PROPERTY(P_TRACE_STACK, String, Debugging, TraceStack)
	DEFINE_RW_PROPERTY(P_TRACE_UNTIL, UInt16, Debugging, TraceUntil)
	DEFINE_RW_PROPERTY(P_MESSAGE_MESSAGES, Bool, Debugging, MessageMessages)

	DEFINE_RW_PROPERTY(P_CENTERED, Bool, Interface, Centered)
	DEFINE_RW_PROPERTY(P_GRID, Bool, Interface, Grid)
	DEFINE_RW_PROPERTY(P_GRID_SIZE, UInt16, Interface, GridSize)
	DEFINE_RW_PROPERTY(P_SLICES, UInt16, Interface, Slices)
	DEFINE_RW_PROPERTY(P_BEEP_LOUDNESS, Int16, Interface, BeepLoudness)
	DEFINE_RW_PROPERTY(P_BEEP_PITCH, Int16, Interface, BeepPitch)
	DEFINE_RW_PROPERTY(P_BEEP_DURATION, Int16, Interface, BeepDuration)
	DEFINE_RW_PROPERTY(P_BEEP_SOUND, String, Interface, BeepSound)
	DEFINE_RW_PROPERTY(P_TOOL, String, Interface, Tool)
	DEFINE_RW_PROPERTY(P_BRUSH, UInt16, Interface, Brush)
	DEFINE_RW_PROPERTY(P_ERASER, UInt16, Interface, Eraser)
	DEFINE_RW_PROPERTY(P_SPRAY, UInt16, Interface, Spray)

	DEFINE_RW_PROPERTY(P_DEFAULT_NETWORK_INTERFACE, String, Network, DefaultNetworkInterface)
	DEFINE_RO_PROPERTY(P_NETWORK_INTERFACES, String, Network, NetworkInterfaces)
	
	DEFINE_RW_PROPERTY(P_DEBUG_CONTEXT, String, Debugging, DebugContext)
	DEFINE_RO_PROPERTY(P_EXECUTION_CONTEXTS, String, Debugging, ExecutionContexts)
	DEFINE_RW_PROPERTY(P_BREAK_POINTS, String, Debugging, Breakpoints)
	DEFINE_RW_PROPERTY(P_WATCHED_VARIABLES, String, Debugging, WatchedVariables)

    DEFINE_RW_ARRAY_PROPERTY(P_CLIPBOARD_DATA, Any, Pasteboard, ClipboardData)
    DEFINE_RW_ARRAY_PROPERTY(P_DRAG_DATA, Any, Pasteboard, DragData)
    DEFINE_RW_PROPERTY(P_CLIPBOARD_DATA, Any, Pasteboard, ClipboardTextData)
    DEFINE_RW_PROPERTY(P_DRAG_DATA, Any, Pasteboard, DragTextData)
    
    DEFINE_RW_ARRAY_PROPERTY(P_RAW_CLIPBOARD_DATA, Any, Pasteboard, RawClipboardData)
    DEFINE_RW_PROPERTY(P_RAW_CLIPBOARD_DATA, Any, Pasteboard, RawClipboardTextData)
    DEFINE_RW_ARRAY_PROPERTY(P_RAW_DRAGBOARD_DATA, Any, Pasteboard, RawDragData)
    DEFINE_RW_PROPERTY(P_RAW_DRAGBOARD_DATA, Any, Pasteboard, RawDragTextData)
    DEFINE_RW_ARRAY_PROPERTY(P_FULL_CLIPBOARD_DATA, Any, Pasteboard, FullClipboardData)
    DEFINE_RW_PROPERTY(P_FULL_CLIPBOARD_DATA, Any, Pasteboard, FullClipboardTextData)
    DEFINE_RW_ARRAY_PROPERTY(P_FULL_DRAGBOARD_DATA, Any, Pasteboard, FullDragData)
    DEFINE_RW_PROPERTY(P_FULL_DRAGBOARD_DATA, Any, Pasteboard, FullDragTextData)

	// MERG-2013-08-17: [[ ColorDialogColors ]] Custom color management for the windows color dialog    
    DEFINE_RW_PROPERTY(P_COLOR_DIALOG_COLORS, LinesOfString, Dialog, ColorDialogColors)
    
    // IM-2013-12-04: [[ PixelScale ]] Add support for global pixelScale and systemPixelScale properties
    // IM-2013-12-04: [[ PixelScale ]] Global property pixelScale returns the current pixel scale
    DEFINE_RW_PROPERTY(P_PIXEL_SCALE, Double, Interface, PixelScale)
    // IM-2013-12-04: [[ PixelScale ]] Global property systemPixelScale returns the pixel scale as determined by the OS
    DEFINE_RO_PROPERTY(P_SYSTEM_PIXEL_SCALE, Double, Interface, SystemPixelScale)
    
    DEFINE_RW_PROPERTY(P_MOVE_SPEED, UInt16, Interface, MoveSpeed)
    DEFINE_RW_PROPERTY(P_DRAG_SPEED, UInt16, Interface, DragSpeed)

    // IM-2014-01-24: [[ HiDPI ]] Global property usePixelScaling returns its configured value (default: true)
    DEFINE_RW_PROPERTY(P_USE_PIXEL_SCALING, Bool, Interface, UsePixelScaling)
    // IM-2014-01-27: [[ HiDPI ]] Global property screenPixelScale returns the pixel scale of the main screen
    DEFINE_RO_PROPERTY(P_SCREEN_PIXEL_SCALE, Double, Interface, ScreenPixelScale)
    // IM-2014-01-27: [[ HiDPI ]] Global property screenPixelScales returns a return-delimited
    // list of the pixel scales of all connected screens
    DEFINE_RO_PROPERTY(P_SCREEN_PIXEL_SCALES, LinesOfDouble, Interface, ScreenPixelScales)
    
    // MW-2014-08-12: [[ EditionType ]] Return whether the engine is community or commercial.
    DEFINE_RO_PROPERTY(P_EDITION_TYPE, String, Engine, EditionType)
    
    // MW-2014-12-10: [[ Extensions ]] Returns a list of loaded extensions.
    DEFINE_RO_PROPERTY(P_LOADED_EXTENSIONS, ProperLinesOfString, Engine, LoadedExtensions)
};

static bool MCPropertyInfoTableLookup(Properties p_which, Boolean p_effective, const MCPropertyInfo*& r_info, bool p_is_array_prop)
{
	for(uindex_t i = 0; i < sizeof(kMCPropertyInfoTable) / sizeof(MCPropertyInfo); i++)
        if (kMCPropertyInfoTable[i] . property == p_which &&
            // SN-2014-08-14: [[ Bug 13204 ]] We want to check for the 'effective property' only if it
            //  is differs from the 'property'.
            (!kMCPropertyInfoTable[i] . has_effective || (kMCPropertyInfoTable[i] . effective == p_effective)) &&
            kMCPropertyInfoTable[i] . is_array_prop == p_is_array_prop)
		{
			r_info = &kMCPropertyInfoTable[i];
			return true;
		}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

struct Property_Override
{
	LT lt;
	Properties property;
};

// List of syntax marks that should be interpreted as properties (if preceded by "the")
Property_Override property_overrides[] =
{
	{{"url", TT_CHUNK, CT_URL}, P_URL},
};
extern const uint32_t property_overrides_size = ELEMENTS(property_overrides);

bool lookup_property_override(const LT&p_lt, Properties &r_property)
{
	for (uint32_t i = 0; i < property_overrides_size; i++)
		if (p_lt.type == property_overrides[i].lt.type && p_lt.which == property_overrides[i].lt.which)
		{
			r_property = property_overrides[i].property;
			return true;
		}
	
	return false;
}

bool lookup_property_override_name(uint16_t p_property, MCNameRef &r_name)
{
	for (uint32_t i = 0; i < property_overrides_size; i++)
		if (property_overrides[i].property == p_property)
			return MCNameCreateWithCString(property_overrides[i].lt.token, r_name);
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

MCProperty::MCProperty()
{
	tocount = CT_UNDEFINED;
	ptype = CT_UNDEFINED;
	which = P_UNDEFINED;
	function = F_UNDEFINED;
	target = NULL;
	destvar = NULL;
	effective = False;
	customindex = NULL;
	customprop = nil;
}

MCProperty::~MCProperty()
{
	delete target;
	delete destvar;
	delete customindex;
	MCNameDelete(customprop);
}

MCObject *MCProperty::getobj(MCExecContext &ctxt)
{
	MCObject *objptr = NULL;
	MCChunk *tchunk = new MCChunk(False);
	MCerrorlock++;
	MCScriptPoint sp(ctxt);
	if (tchunk->parse(sp, False) == PS_NORMAL)
	{
		uint4 parid;
		tchunk->getobj(ctxt, objptr, parid, True);
	}
	MCerrorlock--;
	delete tchunk;
	return objptr;
}

#ifdef LEGACY_EXEC
MCObject *MCProperty::getobj(MCExecPoint &ep)
{
	MCObject *objptr = NULL;
	MCChunk *tchunk = new MCChunk(False);
	MCerrorlock++;
	MCScriptPoint sp(ep);
	if (tchunk->parse(sp, False) == PS_NORMAL)
	{
		uint4 parid;
		tchunk->getobj(ep, objptr, parid, True);
	}
	MCerrorlock--;
	delete tchunk;
	return objptr;
}
#endif

Parse_stat MCProperty::parse(MCScriptPoint &sp, Boolean the)
{
	Symbol_type type;
	const LT *te;
	Boolean lp = False;

	initpoint(sp);
	parent = sp.getobj();

	if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_EFFECTIVE) == PS_NORMAL)
		effective = True;
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_PROPERTY_NOPROP, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_FACTOR, te) != PS_NORMAL)
	{
		// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
		//   execution outwith a handler.
		if (sp . findvar(sp.gettoken_nameref(), &destvar) == PS_NORMAL)
		{
			destvar->parsearray(sp);
			which = P_CUSTOM_VAR;
		}
		else
		{
			which = P_CUSTOM;
			/* UNCHECKED */ MCNameClone(sp . gettoken_nameref(), customprop);
			if (sp.next(type) == PS_NORMAL && type == ST_LB)
			{
				if (sp.parseexp(False, True, &customindex) != PS_NORMAL
				        || sp.next(type) != PS_NORMAL || type != ST_RB)
				{
					MCperror->add(PE_PROPERTY_BADINDEX, sp);
					return PS_ERROR;
				}
			}
			else
				sp.backup();
		}
		if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL || !the)
			return PS_ERROR;
	}
	else
		if (te->type != TT_PROPERTY)
		{
			Properties t_override;
			
			if (te->type == TT_CLASS && te->which == CT_MARKED)
				which = P_MARKED;
			else if (the && lookup_property_override(*te, t_override))
				which = t_override;
			else
			{
				MCperror->add(PE_PROPERTY_NOTAPROP, sp);
				return PS_ERROR;
			}
		}
		else
			which = (Properties)te->which;

	// MW-2011-11-24: [[ Nice Folders ]] Make sure engine/temporary/documents/desktop/home
	//   are all followed by 'folder'/'directory'.
	if (which == P_ENGINE_FOLDER || which == P_TEMPORARY_FOLDER || which == P_DOCUMENTS_FOLDER ||
		which == P_DESKTOP_FOLDER || which == P_HOME_FOLDER)
	{
		if (sp . next(type) != PS_NORMAL)
		{
			MCperror->add(PE_GENERAL_NOMODIFY, sp);
			return PS_ERROR;
		}

		if (sp.lookup(SP_FACTOR, te) != PS_NORMAL ||
			te -> which != P_DIRECTORY)
		{
			MCperror->add(PE_GENERAL_CANTMODIFY, sp);
			return PS_ERROR;
		}
	}

	if (which == P_SHORT || which == P_LONG || which == P_ABBREVIATE ||
			which == P_ENGLISH || which == P_INTERNET || which == P_SYSTEM ||
			which == P_WORKING)
	{
		uint2 dummy;
		if (which == P_SYSTEM
		        || sp.skip_token(SP_FACTOR, TT_PROPERTY, P_SYSTEM) == PS_NORMAL)
		{
			dummy = which;
			dummy += CF_SYSTEM;
			which = (Properties)dummy;
		}
		else if (which == P_ENGLISH
					|| sp.skip_token(SP_FACTOR, TT_PROPERTY, P_ENGLISH) == PS_NORMAL)
		{
			dummy = which;
			dummy += CF_ENGLISH;
			which = (Properties)dummy;
		}
		else if (which == P_WORKING
					|| sp . skip_token(SP_FACTOR, TT_PROPERTY, P_WORKING) == PS_NORMAL)
		{
			dummy = which;
			dummy += 1000;
			which = (Properties)dummy;
		}
		else if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_INTERNET) == PS_NORMAL)
				which = P_INTERNET;

		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add(PE_GENERAL_NOMODIFY, sp);
			return PS_ERROR;
		}

		if (sp.lookup(SP_FACTOR, te) != PS_NORMAL)
		{
			MCperror->add(PE_GENERAL_CANTMODIFY, sp);
			return PS_ERROR;
		}

		switch (te->type)
		{
		case TT_PROPERTY:
			if (te->which == P_ID || te->which == P_NAME || te->which == P_OWNER)
			{
				if (which == P_SHORT || which == P_ABBREVIATE || which == P_LONG)
					which = (Properties)(te->which + which - P_SHORT + 1);
			}
			else
			{
				MCperror->add(PE_PROPERTY_CANTMODIFY, sp);
				return PS_ERROR;
			}
			break;
		case TT_FUNCTION:
			switch (te->which)
			{
			// MW-2008-11-05: [[ Owner Reference ]] In the case of encountering:
			//     the ( short | abbrev | long ) owner
			//   Augment which to be the corresponding property. Note that 'the owner'
			//   is parsed as a function.
			case F_OWNER:
				if (which == P_SHORT || which == P_ABBREVIATE || which == P_LONG)
					which = (Properties)(P_OWNER + which - P_SHORT + 1);
				break;
			case F_TIME:
			case F_DATE:
			case F_MILLISECS:
			case F_SECONDS:
			case F_TICKS:
			case F_FILES:
			case F_DIRECTORIES:
			case F_MONTH_NAMES:
			case F_WEEK_DAY_NAMES:
			case F_DATE_FORMAT:
				function = (Functions)te->which;
				if (which == P_INTERNET && function != F_DATE)
				{
					MCperror->add(PE_FACTOR_BADPARAM, sp);
					return PS_ERROR;
				}
				return PS_NORMAL;
			case F_SCREEN_RECT:
			case F_SCREEN_RECTS:
				if (which != (1000 + P_WORKING))
				{
					MCperror -> add(PE_FACTOR_BADPARAM, sp);
					return PS_ERROR;
				}
				
				if (te -> which == F_SCREEN_RECTS)
					which = (Properties)(1000 + P_LONG);

				function = F_SCREEN_RECT;
				return PS_NORMAL;
			default:
				MCperror->add(PE_FUNCTION_CANTMODIFY, sp);
				return PS_ERROR;
			}
			break;
		default:
			MCperror->add(PE_GENERAL_CANTMODIFY, sp);
			return PS_ERROR;
		}
	}
	switch (which)
	{
	case P_CASE_SENSITIVE:
    case P_FORM_SENSITIVE:
	case P_CENTURY_CUTOFF:
	case P_CONVERT_OCTALS:
	case P_ITEM_DELIMITER:
	case P_COLUMN_DELIMITER:
	case P_ROW_DELIMITER:
	case P_LINE_DELIMITER:
	case P_WHOLE_MATCHES:
	case P_USE_SYSTEM_DATE:
	case P_USE_UNICODE:
	case P_CURSOR:
	case P_DEFAULT_CURSOR:
	case P_DEFAULT_STACK:
	case P_DEFAULT_MENU_BAR:
	case P_DRAG_SPEED:
	case P_MOVE_SPEED:
	case P_LOCK_COLORMAP:



	case P_LOCK_CURSOR:
	case P_LOCK_ERRORS:
	case P_LOCK_MENUS:
	case P_LOCK_MESSAGES:
	case P_LOCK_MOVES:
	case P_LOCK_RECENT:
	case P_USER_LEVEL:
	case P_USER_MODIFY:
	case P_BRUSH:
	case P_CENTERED:
	case P_ERASER:
	case P_GRID:
	case P_GRID_SIZE:
	case P_MULTIPLE:
	case P_MULTI_SPACE:
	case P_SLICES:
	case P_SPRAY:
	case P_BEEP_LOUDNESS:
	case P_BEEP_PITCH:
	case P_BEEP_DURATION:
	case P_BEEP_SOUND:
	case P_DIRECTORY:
	case P_PRIVATE_COLORS:
	case P_TWELVE_TIME:
	case P_IDLE_RATE:
	case P_IDLE_TICKS:
	case P_BLINK_RATE:
	case P_RECURSION_LIMIT:
	case P_REPEAT_RATE:
	case P_REPEAT_DELAY:
	case P_TYPE_RATE:
	case P_SYNC_RATE:
	case P_EFFECT_RATE:
	case P_DOUBLE_DELTA:
	case P_DOUBLE_TIME:
	case P_TOOL_TIP_DELAY:
	case P_LONG_WINDOW_TITLES:
	case P_BLIND_TYPING:
	case P_POWER_KEYS:
	case P_NAVIGATION_ARROWS:
	case P_TEXT_ARROWS:
	case P_EXTEND_KEY:
	case P_COLORMAP:
	case P_NO_PIXMAPS:
	case P_LOW_RESOLUTION_TIMERS:
	case P_POINTER_FOCUS:

	case P_EMACS_KEY_BINDINGS:
	case P_RAISE_MENUS:
	case P_ACTIVATE_PALETTES:
	case P_HIDE_PALETTES:
	case P_RAISE_PALETTES:
	case P_RAISE_WINDOWS:
	case P_DONT_USE_NS:
	case P_DONT_USE_QT_EFFECTS:
	case P_PROPORTIONAL_THUMBS:
	case P_SHARED_MEMORY:
	case P_VC_SHARED_MEMORY:
	case P_VC_PLAYER:
	case P_SCREEN_GAMMA:
	case P_SHELL_COMMAND:
	case P_SOUND_CHANNEL:
	case P_TRACE_ABORT:
	case P_TRACE_DELAY:
	case P_TRACE_RETURN:
	case P_TRACE_UNTIL:
	case P_TRACE_STACK:
	
	case P_PRINTER_NAMES:

	case P_PRINT_CARD_BORDERS:
	case P_PRINT_COMMAND:
	case P_PRINT_FONT_TABLE:
	case P_PRINT_GUTTERS:
	case P_PRINT_MARGINS:
	case P_PRINT_ROTATED:
	case P_PRINT_ROWS_FIRST:
	case P_PRINT_SCALE:
	
	case P_PRINT_DEVICE_NAME:
	case P_PRINT_DEVICE_SETTINGS:
	case P_PRINT_DEVICE_OUTPUT:
	case P_PRINT_DEVICE_RECTANGLE:
	case P_PRINT_DEVICE_FEATURES:
	
	case P_PRINT_PAGE_SIZE:
	case P_PRINT_PAGE_ORIENTATION:
	case P_PRINT_PAGE_SCALE:
	case P_PRINT_PAGE_RECTANGLE:
	
	case P_PRINT_JOB_COPIES:
	case P_PRINT_JOB_COLLATE:
	case P_PRINT_JOB_NAME:
	case P_PRINT_JOB_DUPLEX:
	case P_PRINT_JOB_COLOR:
	case P_PRINT_JOB_RANGES:
	case P_PRINT_JOB_PAGE:

	case P_PRINT_TEXT_ALIGN:
	case P_PRINT_TEXT_FONT:
	case P_PRINT_TEXT_HEIGHT:
	case P_PRINT_TEXT_SIZE:
	case P_PRINT_TEXT_STYLE:
	case P_DIALOG_DATA:
	case P_HC_IMPORT_STAT:
	case P_SCRIPT_TEXT_FONT:
	case P_SCRIPT_TEXT_SIZE:
	case P_LOOK_AND_FEEL:

	case P_SCREEN_MOUSE_LOC:
	case P_UMASK:
	case P_BUFFER_MODE:
	case P_BUFFER_IMAGES:
	case P_BACK_DROP:
	case P_MULTI_EFFECT:
	case P_ALLOW_INTERRUPTS:
	case P_EXPLICIT_VARIABLES:
	case P_PRESERVE_VARIABLES:
	case P_SYSTEM_FS:
	case P_SYSTEM_CS:
	case P_SYSTEM_PS:
	case P_FILE_TYPE:
	case P_STACK_FILE_TYPE:
	case P_STACK_FILE_VERSION:
	case P_SECURE_MODE:
	case P_SECURITY_CATEGORIES:
	case P_SECURITY_PERMISSIONS:
	case P_SERIAL_CONTROL_STRING:
	case P_EDIT_SCRIPTS:
	case P_COLOR_WORLD:
	case P_ALLOW_FIELD_REDRAW:
	case P_ALLOW_KEY_IN_FIELD:
	case P_REMAP_COLOR:
	case P_HIDE_CONSOLE_WINDOWS:
	case P_FTP_PROXY:
	case P_HTTP_HEADERS:
	case P_HTTP_PROXY:
	case P_SHOW_INVISIBLES:
	case P_SOCKET_TIMEOUT:
	case P_RANDOM_SEED:
	case P_ADDRESS:
	case P_STACKS_IN_USE:
    // TD-2013-06-20: [[ DynamicFonts ]] global property for list of font files
    case P_FONTFILES_IN_USE:
	case P_RELAYER_GROUPED_CONTROLS:
	case P_SELECTION_MODE:
	case P_SELECTION_HANDLE_COLOR:
	case P_WINDOW_BOUNDING_RECT:
	case P_JPEG_QUALITY:
	case P_LZW_KEY:
	case P_RECORDING:
	case P_RECORD_FORMAT:
	case P_RECORD_COMPRESSION:
	case P_RECORD_INPUT:
	case P_RECORD_SAMPLESIZE:
	case P_RECORD_CHANNELS:
	case P_RECORD_RATE:
	case P_BREAK_POINTS:
	case P_DEBUG_CONTEXT:
	case P_EXECUTION_CONTEXTS:
	case P_MESSAGE_MESSAGES:
	case P_WATCHED_VARIABLES:
	case P_ALLOW_INLINE_INPUT:
	case P_ACCEPT_DROP:
	case P_ALLOWABLE_DRAG_ACTIONS:
	case P_DRAG_ACTION:
	case P_DRAG_IMAGE:
	case P_DRAG_IMAGE_OFFSET:
	case P_DRAG_DELTA:
	case P_SSL_CERTIFICATES:
	case P_QT_IDLE_RATE:
	case P_HIDE_BACKDROP:
	
	// MW-2008-08-12: Add support for the urlResponse property, giving
	// access to MCurlresult.
	case P_URL_RESPONSE:
	
	// IM-2011-07-07: Add support for specifying the default network interface
	case P_DEFAULT_NETWORK_INTERFACE:
			
	// MM-2011-07-14: Add support for listing avaiable network interfaces
	case P_NETWORK_INTERFACES:

	case P_REV_MESSAGE_BOX_LAST_OBJECT: // DEVELOPMENT only
	case P_REV_MESSAGE_BOX_REDIRECT: // DEVELOPMENT only
	case P_REV_LICENSE_LIMITS: // DEVELOPMENT only

	// MW-2010-06-04: Add support for dock menu and status icon separation.
	case P_ICON_MENU:
	case P_STATUS_ICON:
	case P_STATUS_ICON_MENU:
	case P_STATUS_ICON_TOOLTIP:

	case P_PROCESS_TYPE:
	case P_STACK_LIMIT:
	case P_ALLOW_DATAGRAM_BROADCASTS:

	case P_ERROR_MODE:
	case P_OUTPUT_TEXT_ENCODING:
	case P_OUTPUT_LINE_ENDINGS:
	case P_SESSION_SAVE_PATH:
	case P_SESSION_LIFETIME:
	case P_SESSION_COOKIE_NAME:
	case P_SESSION_ID:
	
	case P_SCRIPT_EXECUTION_ERRORS:
	case P_SCRIPT_PARSING_ERRORS:
			
	case P_REV_RUNTIME_BEHAVIOUR:
	
	//MW-2011-11-24: [[ Nice Folders ]] The the ... folder properties are all
	//  global.
	case P_ENGINE_FOLDER:
	case P_HOME_FOLDER:
	case P_TEMPORARY_FOLDER:
	case P_DOCUMENTS_FOLDER:
	case P_DESKTOP_FOLDER:
	// MM-2012-09-05: [[ Property Listener ]]
	case P_REV_OBJECT_LISTENERS: // DEVELOPMENT only
			
	case P_IMAGE_CACHE_LIMIT:
	case P_IMAGE_CACHE_USAGE:
	case P_REV_PROPERTY_LISTENER_THROTTLE_TIME: // DEVELOPMENT only

	// MERG-2013-08-17: [[ ColorDialogColors ]] Custom color management for the windows color dialog
	case P_COLOR_DIALOG_COLORS:

	// IM-2013-12-04: [[ PixelScale ]] Add support for global pixelScale and systemPixelScale properties
	case P_PIXEL_SCALE:
	case P_SYSTEM_PIXEL_SCALE:

	// IM-2014-01-24: [[ HiDPI ]] Add support for global usePixelScaling, screenPixelScale, screenPixelScales properties
	case P_USE_PIXEL_SCALING:
	case P_SCREEN_PIXEL_SCALE:
    case P_SCREEN_PIXEL_SCALES:
            
    // MW-2014-08-12: [[ EditionType ]] Add support for global editionType property.
    case P_EDITION_TYPE:
            
    // MW-2014-12-10: [[ Extensions ]] Add support for global loadedExtensions property.
    case P_LOADED_EXTENSIONS:
        break;
	        
	case P_REV_CRASH_REPORT_SETTINGS: // DEVELOPMENT only
	case P_REV_LICENSE_INFO:
	case P_DRAG_DATA:
	case P_CLIPBOARD_DATA:
    case P_RAW_CLIPBOARD_DATA:
    case P_RAW_DRAGBOARD_DATA:
    case P_FULL_CLIPBOARD_DATA:
    case P_FULL_DRAGBOARD_DATA:
		if (sp.next(type) != PS_NORMAL)
			return PS_NORMAL;
		if (type != ST_LB)
		{
			sp.backup();
			break;
		}
		if (sp.parseexp(False, True, &customindex) != PS_NORMAL)
		{
			MCperror->add(PE_VARIABLE_BADINDEX, sp);
			return PS_ERROR;
		}
		if (sp.next(type) != PS_NORMAL || type != ST_RB)
		{
			MCperror->add(PE_VARIABLE_NORBRACE, sp);
			return PS_ERROR;
		}
		break;
	case P_TOOL:
		if (sp.skip_token(SP_FACTOR, TT_LPAREN) == PS_NORMAL)
		{
			if (sp.skip_token(SP_FACTOR, TT_RPAREN) != PS_NORMAL)
			{
				MCperror->add(PE_FACTOR_NORPAREN, sp);
				return PS_ERROR;
			}
			break;
		}
	case P_BRUSH_COLOR:
	case P_DONT_USE_QT:
	case P_BRUSH_BACK_COLOR:
	case P_BRUSH_PATTERN:
	case P_PEN_COLOR:
	case P_PEN_BACK_COLOR:
	case P_PEN_PATTERN:
	case P_RECENT_CARDS:
	case P_RECENT_NAMES:
	case P_TEXT_ALIGN:
	case P_TEXT_FONT:
	case P_TEXT_HEIGHT:
	case P_TEXT_SIZE:
	case P_TEXT_STYLE:
	case P_EDIT_BACKGROUND:
	case P_LINE_SIZE:
	case P_PEN_WIDTH:
	case P_PEN_HEIGHT:
	case P_FILLED:
	case P_POLY_SIDES:
	case P_ROUND_ENDS:
	case P_DASHES:
	case P_ROUND_RADIUS:
	case P_START_ANGLE:
	case P_ARC_ANGLE:
	case P_NUMBER_FORMAT:
	case P_PLAY_DESTINATION:
	case P_PLAY_LOUDNESS:
	case P_LOCK_SCREEN:
	case P_STACK_FILES:
	case P_MENU_BAR:
	case P_EDIT_MENUS:
	case P_ACCENT_COLOR:
	case P_HILITE_COLOR:
	case P_PAINT_COMPRESSION:
	case P_LINK_COLOR:
	case P_LINK_HILITE_COLOR:
	case P_LINK_VISITED_COLOR:
	case P_UNDERLINE_LINKS:
	case P_SELECT_GROUPED_CONTROLS:
	case P_ICON:
		if (sp.next(type) != PS_NORMAL)
			break;
		if (which < P_FIRST_ARRAY_PROP)
		{
			if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || te->which != PT_OF)
			{
				sp.backup();
				break;
			}
		}
		sp.backup();
	default:
        if (sp.next(type) == PS_NORMAL)
        {
            if (type == ST_LB)
            {
                if (sp.parseexp(False, True, &customindex) != PS_NORMAL
                        || sp.next(type) != PS_NORMAL || type != ST_RB)
                {
                    MCperror->add(PE_PROPERTY_BADINDEX, sp);
                    return PS_ERROR;
                }
            }
            else
                sp.backup();
        }

		if (sp.skip_token(SP_FACTOR, TT_LPAREN) == PS_NORMAL)
			lp = True;
		else
			if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL && !the)
			{
				sp.backup();
				sp.next(type);
				
				// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
				//   execution outwith a handler.
				if (sp . finduqlvar(sp.gettoken_nameref(), &destvar) != PS_NORMAL)
				{
					MCperror->add(PE_PROPERTY_NOTOF, sp);
					return PS_ERROR;
				}
				return PS_NORMAL;
			}
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add
			(PE_PROPERTY_MISSINGTARGET, sp);
			return PS_ERROR;
		}
		Boolean doingthe = False;
		if (sp.lookup(SP_FACTOR, te) == PS_NORMAL && which == P_NUMBER
		        && (te->type == TT_CLASS || te->type == TT_CHUNK))
		{
			Boolean gotclass = True;
			if (te->type == TT_CHUNK)
			{
				if (te->which == CT_BACKGROUND || te->which == CT_CARD)
				{
					ptype = (Chunk_term)te->which;
					if (sp.next(type) != PS_NORMAL)
					{
						MCperror->add
						(PE_PROPERTY_MISSINGTARGET, sp);
						return PS_ERROR;
					}
					if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || te->type != TT_CLASS
					        || !(te->which > CT_GROUP && tocount < CT_LINE))
					{
						sp.backup();
						sp.backup();
						gotclass = False;
					}
				}
				else
				{
					sp.backup();
					gotclass = False;
				}
			}
			if (gotclass)
			{
				tocount = (Chunk_term)te->which;
				if (tocount == CT_MARKED)
					sp.skip_token(SP_FACTOR, TT_CLASS, CT_CARD);
				if (sp.next(type) != PS_NORMAL)
				{
					if (tocount < CT_LINE)
					{
						target = new MCChunk(False);
						break;
					}
					else
					{
						MCperror->add
						(PE_PROPERTY_MISSINGOFORIN, sp);
						return PS_ERROR;
					}
				}
				if (sp.lookup(SP_FACTOR, te) != PS_NORMAL
				    || (te->type != TT_OF && te->type != TT_IN))
				{
					if (tocount < CT_LINE)
					{
						sp.backup();
						target = new MCChunk(False);
						break;
					}
					else
					{
						MCperror->add
						(PE_PROPERTY_NOTOFORIN, sp);
						return PS_ERROR;
					}
				}
				if (sp.skip_token(SP_FACTOR, TT_THE) == PS_NORMAL)
					doingthe = True;
			}
		}
		else
			sp.backup();
		target = new MCChunk(False);
		if (target->parse(sp, doingthe) != PS_NORMAL)
		{
			MCperror->add
			(PE_PROPERTY_BADCHUNK, sp);
			return PS_ERROR;

		}
	}
	if (lp)
		sp.skip_token(SP_FACTOR, TT_RPAREN);
	return PS_NORMAL;
}

// This method resolves the property name and index depending on what kind of
// property access was compiled. There are three cases:
//   1) A static property (which != P_CUSTOM_VAR, customindex == nil)
//   2) A static property with array index (which != P_CUSTOM_VAR, customindex != nil)
//   3) A dynamic property (which == P_CUSTOM_VAR, destvar holds property)
// On exit, if the access is for a static property r_which will contain the 
// property; otherwise r_which will contain P_CUSTOM and r_prop_name will be
// the name of the custom property to use. In either case r_index_name will be
// non-nil if there is an index to fetch also.
//
// A static property is something like:
//   the width of me ('width' is a keyword so maps to a member of the Properties enum)
// or
//   the unquotedliteral of me ('unqutotedliteral' will map to P_CUSTOM, with the name in customprop)
//
// A dynamic property is something like:
//   the pVar of me
// where pVar is a variable.
//
#ifdef LEGACY_EXEC
Exec_stat MCProperty::resolveprop(MCExecPoint& ep, Properties& r_which, MCNameRef& r_prop_name, MCNameRef& r_index_name)
{
	MCNameRef t_prop_name, t_index_name;
	Properties t_prop;
	t_prop = which;
	t_prop_name = nil;
	t_index_name = nil;
	
	// MW-2011-09-02: If the property is already compiled as 'P_CUSTOM' it means
	//   customprop is non-nil. Handle this case here rather than in the caller to
	//   simplify code.
	if (which == P_CUSTOM)
		/* UNCHECKED */ MCNameClone(customprop, t_prop_name);
	
	// At present, something like the 'pVar[pIndex] of me' is evaluated as 'the pVar of me'
	// this is because any index is extracted from the pVar variable. It might be worth
	// considering altering this behavior slightly, to allow dynamic indices on dynamic
	// props, although this needs to be done sympathetically to the current behavior...
	if (which == P_CUSTOM_VAR)
	{
		MCExecPoint ep2(ep);
		destvar -> eval(ep2);
        MCAutoStringRef t_value;
        /* UNCHECKED */ ep2 . copyasstringref(&t_value);
		MCScriptPoint sp(*t_value);
		Symbol_type type;
		const LT *te;
		if (sp.next(type) && sp.lookup(SP_FACTOR, te) == PS_NORMAL && te->type == TT_PROPERTY && sp.next(type) == PS_EOF)
			t_prop = (Properties)te -> which;
		else
		{
			uint2 i;
			MCString s = ep2.getsvalue();
			MCString icarray;
			for (i = 0 ; i < ep2.getsvalue().getlength() ; i++)
				if (s.getstring()[i] == '[')
				{
					icarray.set(s.getstring() + i + 1, s.getlength() - i - 2);
					s.setlength(i);
					break;
				}

			// MW-2011-09-02: [[ Bug 9698 ]] Always create a name for the property, otherwise
			//   if the var contains empty, this function returns a nil name which causes
			//   customprop to be used incorrectly.
			/* UNCHECKED */ MCNameCreateWithOldString(s, t_prop_name);

			if (icarray . getlength() != 0)
				/* UNCHECKED */ MCNameCreateWithOldString(icarray, t_index_name);

			t_prop = P_CUSTOM;
		}
	}
	else if (customindex != nil)
	{
		MCExecPoint ep2(ep);
		if (customindex -> eval(ep2) != ES_NORMAL)
		{
			MCeerror->add(EE_PROPERTY_BADEXPRESSION, line, pos);
			return ES_ERROR;
		}

		if (ep2 . getsvalue() . getlength() != 0)
			/* UNCHECKED */ ep2 . copyasnameref(t_index_name);
	}

	r_which = t_prop;
	r_prop_name = t_prop_name;
	r_index_name = t_index_name;

	return ES_NORMAL;
}
#endif

bool MCProperty::resolveprop(MCExecContext& ctxt, Properties& r_which, MCNameRef& r_prop_name, MCNameRef& r_index_name)
{
	MCNameRef t_prop_name, t_index_name;
	Properties t_prop;
	t_prop = which;
	t_prop_name = nil;
	t_index_name = nil;
	
	// MW-2011-09-02: If the property is already compiled as 'P_CUSTOM' it means
	//   customprop is non-nil. Handle this case here rather than in the caller to
	//   simplify code.
	if (which == P_CUSTOM)
    {
        if (!MCNameClone(customprop, t_prop_name))
            return false;
    }
	
	// At present, something like the 'pVar[pIndex] of me' is evaluated as 'the pVar of me'
	// this is because any index is extracted from the pVar variable. It might be worth
	// considering altering this behavior slightly, to allow dynamic indices on dynamic
	// props, although this needs to be done sympathetically to the current behavior...
	if (which == P_CUSTOM_VAR)
	{
        MCAutoStringRef t_string;
        if (!ctxt  . EvalExprAsStringRef(destvar, EE_PROPERTY_BADEXPRESSION, &t_string))
            return false;

        // AL-2015-08-27: [[ Bug 15798 ]] Parse into index and property name before determining
        //  if this is a custom property or not (otherwise things like textStyle["bold"] are
        //  interpreted as custom properties).
        MCAutoStringRef t_icarray, t_property_name;
        uindex_t t_offset, t_end_offset;
        // SN-2014-08-08: [[ Bug 13135 ]] Targetting a custom var should resolve the index AND suppress
        //  it from the initial name
        if (MCStringFirstIndexOfChar(*t_string, '[', 0, kMCStringOptionCompareExact, t_offset) &&
            MCStringLastIndexOfChar(*t_string, ']', UINDEX_MAX, kMCStringOptionCompareExact, t_end_offset) &&
            t_end_offset == MCStringGetLength(*t_string) - 1)
        {
            if (!MCStringCopySubstring(*t_string, MCRangeMake(0, t_offset), &t_property_name))
                return false;
            
            // AL-2015-08-27: [[ Bug 15798 ]] If the index is quoted, we don't want to include the
            //  quotes in the index name.
            if (MCStringGetCodepointAtIndex(*t_string, t_offset + 1) == '"' &&
                MCStringGetCodepointAtIndex(*t_string, t_end_offset - 1) == '"')
            {
                t_offset++;
                t_end_offset--;
            }
            
            if (!MCStringCopySubstring(*t_string, MCRangeMake(t_offset + 1, t_end_offset - t_offset - 1), &t_icarray))
                return false;
        }
        else
            t_property_name = *t_string;
        
        if (*t_icarray != nil)
        {
            if (!MCNameCreate(*t_icarray, t_index_name))
                return false;
        }
        
        MCScriptPoint sp(*t_property_name);
		Symbol_type type;
		const LT *te;
		if (sp.next(type) && sp.lookup(SP_FACTOR, te) == PS_NORMAL && te->type == TT_PROPERTY && sp.next(type) == PS_EOF)
			t_prop = (Properties)te -> which;
		else
        {
			// MW-2011-09-02: [[ Bug 9698 ]] Always create a name for the property, otherwise
			//   if the var contains empty, this function returns a nil name which causes
			//   customprop to be used incorrectly.
            //            
            if (!MCNameCreate(*t_property_name, t_prop_name))
                return false;
            
			t_prop = P_CUSTOM;
		}
	}
	else if (customindex != nil)
    {
        if (!ctxt . EvalExprAsNameRef(customindex, EE_PROPERTY_BADEXPRESSION, t_index_name))
            return false;
    }
    
	r_which = t_prop;
	r_prop_name = t_prop_name;
	r_index_name = t_index_name;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCProperty::set */ LEGACY_EXEC
	MCImage *newim;
	int2 mx, my;
	uint2 tir;
	Exec_stat stat;
	char *eptr = NULL;
	Boolean b, newlock;

	if (destvar != NULL && which != P_CUSTOM_VAR)
		return destvar->set(ep);

	switch (which)
	{
	case P_CASE_SENSITIVE:
		return ep.setcasesensitive(line, pos);
	case P_CENTURY_CUTOFF:
		return ep.setcutoff(line, pos);
	case P_CONVERT_OCTALS:
		return ep.setconvertoctals(line, pos);
	case P_ITEM_DELIMITER:
		return ep.setitemdel(line, pos);
	case P_COLUMN_DELIMITER:
		return ep.setcolumndel(line, pos);
	case P_ROW_DELIMITER:
		return ep.setrowdel(line, pos);
	case P_LINE_DELIMITER:
		return ep.setlinedel(line, pos);
	case P_WHOLE_MATCHES:
		return ep.setwholematches(line, pos);
	case P_USE_SYSTEM_DATE:
		if (ep.setusesystemdate(line, pos) != ES_NORMAL)
			return ES_ERROR;
		return ES_NORMAL;
	case P_USE_UNICODE:
		return ep.setuseunicode(line, pos);
		
		
	// BEGIN PRINT DEVICE PROPERTIES
	case P_PRINT_DEVICE_NAME:
		MCprinter -> SetDeviceName(ep . getcstring());
		return ES_NORMAL;

	case P_PRINT_DEVICE_SETTINGS:
		MCprinter -> SetDeviceSettings(ep . getsvalue());
		return ES_NORMAL;
		
	case P_PRINT_DEVICE_OUTPUT:
	{
		MCPrinterOutputType t_output_type;
		const char *t_output_location;
		
		MCString t_value;
		t_value = ep . getsvalue0();
		
		if (t_value == "preview")
		{
			t_output_type = PRINTER_OUTPUT_PREVIEW;
			t_output_location = NULL;
		}
		else if (t_value == "device")
		{
			t_output_type = PRINTER_OUTPUT_DEVICE;
			t_output_location = NULL;
		}
		else
		{
			MCString t_head, t_tail;
			t_value . split(':', t_head, t_tail);
			if (t_head == "file")
			{
				t_output_type = PRINTER_OUTPUT_FILE;
				t_output_location = t_tail . getstring();
			}
			else
			{
				MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, t_value);
				return ES_ERROR;
			}
		}
		
		MCprinter -> SetDeviceOutput(t_output_type, t_output_location);
	}
	return ES_NORMAL;
	// END PRINT DEVICE PROPERTIES
	
	
	// BEGIN PRINT PAGE PROPERTIES
	case P_PRINT_PAGE_SIZE:
	{
		int2 t_width, t_height;
		if (!MCU_stoi2x2(ep . getsvalue(), t_width, t_height))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		MCprinter -> SetPageSize(t_width, t_height);
	}
	return ES_NORMAL;
	
	case P_PRINT_PAGE_ORIENTATION:
	{
		MCString t_value;
		t_value = ep . getsvalue();
		
		MCPrinterOrientation t_orientation;
		if (t_value == "portrait")
			t_orientation = PRINTER_ORIENTATION_PORTRAIT;
		else if (t_value == "reverse portrait")
			t_orientation = PRINTER_ORIENTATION_REVERSE_PORTRAIT;
		else if (t_value == "landscape")
			t_orientation = PRINTER_ORIENTATION_LANDSCAPE;
		else if (t_value == "reverse landscape")
			t_orientation = PRINTER_ORIENTATION_REVERSE_LANDSCAPE;
		else
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, t_value);
			return ES_ERROR;
		}
		
		MCprinter -> SetPageOrientation(t_orientation);
	}
	return ES_NORMAL;
	
	case P_PRINT_PAGE_SCALE:
	{
		real8 t_scale;
		if (!MCU_stor8(ep . getsvalue(), t_scale))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		MCprinter -> SetPageScale(t_scale);
	}
	return ES_NORMAL;
	// END PRINT PAGE PROPERTIES
	
	
	// BEGIN PRINT JOB PROPERTIES
	case P_PRINT_JOB_NAME:
		MCprinter -> SetJobName(ep . getcstring());
		return ES_NORMAL;
	
	case P_PRINT_JOB_COPIES:
	{
		int4 t_copies;
		if (!MCU_stoi4(ep . getsvalue(), t_copies))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		MCprinter -> SetJobCopies(t_copies);
	}
	return ES_NORMAL;
	
	case P_PRINT_JOB_DUPLEX:
	{
		MCString t_value;
		t_value = ep . getsvalue();
		
		MCPrinterDuplexMode t_mode;
		if (t_value == "none")
			t_mode = PRINTER_DUPLEX_MODE_SIMPLEX;
		else if (t_value == "short edge")
			t_mode = PRINTER_DUPLEX_MODE_SHORT_EDGE;
		else if (t_value == "long edge")
			t_mode = PRINTER_DUPLEX_MODE_LONG_EDGE;
		else
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}

		MCprinter -> SetJobDuplex(t_mode);
	}
	return ES_NORMAL;
	
	case P_PRINT_JOB_COLLATE:
	{
		Boolean t_bool;
		if (!MCU_stob(ep . getsvalue(), t_bool))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		
		MCprinter -> SetJobCollate(t_bool == True);
	}
	return ES_NORMAL;

	case P_PRINT_JOB_COLOR:
	{
		Boolean t_bool;
		if (!MCU_stob(ep . getsvalue(), t_bool))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		
		MCprinter -> SetJobColor(t_bool == True);
	}
	return ES_NORMAL;

	case P_PRINT_JOB_RANGES:
	{
		bool t_error;
		t_error = false;

		MCInterval *t_ranges;
		int t_range_count;
		t_ranges = NULL;

		if (ep . getsvalue() == "all" || ep . getsvalue() == "")
			t_range_count = PRINTER_PAGE_RANGE_ALL;
		else if (ep . getsvalue() == "current")
			t_range_count = PRINTER_PAGE_RANGE_CURRENT;
		else if (ep . getsvalue() == "selection")
			t_range_count = PRINTER_PAGE_RANGE_SELECTION;
		else
		{
			const char *t_str;
			t_str = ep . getcstring();
			t_range_count = 0;
			while(*t_str != '\0' && !t_error)
			{
				char *t_end_ptr;
				int t_from, t_to;
				t_from = strtol(t_str, &t_end_ptr, 10);
				if (t_str == t_end_ptr)
					t_error = true;
				else if (*t_end_ptr == ',' || *t_end_ptr == '\0')
					t_to = t_from;
				else if (*t_end_ptr == '-')
				{
					t_str = t_end_ptr + 1;
					t_to = strtol(t_str, &t_end_ptr, 10);
					if (t_str == t_end_ptr)
						t_error = true;
				}

				if (!t_error)
				{
					MCU_disjointrangeinclude(t_ranges, t_range_count, t_from, t_to);

					t_str = t_end_ptr;
					if (*t_str == ',')
						t_str++;
					else if (*t_str != '\0')
						t_error = true;
				}
			}
		}

		if (!t_error)
			MCprinter -> SetJobRanges(t_range_count, t_ranges);

		delete t_ranges;

		if (t_error)
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
	}
	return ES_NORMAL;
	// END PRINT JOB PROPERTIES

	// BEGIN PRINT LAYOUT PROPERTIES
	case P_PRINT_CARD_BORDERS:
	{
		Boolean t_bool;
		if (!MCU_stob(ep . getsvalue(), t_bool))
		{
			MCeerror->add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		MCprinter -> SetLayoutShowBorders(t_bool == True);
	}
	return ES_NORMAL;
	
	case P_PRINT_GUTTERS:
	{
		int2 t_rows, t_columns;
		if (!MCU_stoi2x2(ep . getsvalue(), t_rows, t_columns))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		MCprinter -> SetLayoutSpacing(t_rows, t_columns);
	}
	return ES_NORMAL;
	
	case P_PRINT_MARGINS:
	{
		int2 t_left, t_top, t_right, t_bottom;
		if (!MCU_stoi2x4(ep . getsvalue(), t_left, t_top, t_right, t_bottom))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		MCprinter -> SetPageMargins(t_left, t_top, t_right, t_bottom);
	}
	return ES_NORMAL;
	
	case P_PRINT_ROWS_FIRST:
	{
		Boolean t_bool;
		if (!MCU_stob(ep . getsvalue(), t_bool))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		MCprinter -> SetLayoutRowsFirst(t_bool == True);
	}
	return ES_NORMAL;

	case P_PRINT_SCALE:
	{
		float64_t t_scale;
		if (!MCU_stor8(ep . getsvalue(), t_scale))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		MCprinter -> SetLayoutScale(t_scale);
	}
	return ES_NORMAL;
	// END PRINT LAYOUT PROPERTIES


	// BEGIN LEGACY PRINTING PROPERTIES
	case P_PRINT_ROTATED:
	{
		Boolean t_bool;
		if (!MCU_stob(ep . getsvalue(), t_bool))
		{
			MCeerror -> add(EE_PROPERTY_BADPRINTPROP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		
		MCprinter -> SetPageOrientation(t_bool ? PRINTER_ORIENTATION_LANDSCAPE : PRINTER_ORIENTATION_PORTRAIT);
	}
	return ES_NORMAL;
	// END LEGACY PRINTING PROPERTIES


	case P_PRINT_COMMAND:
		MCprinter -> SetDeviceCommand(ep . getcstring());
	return ES_NORMAL;
	
	case P_PRINT_FONT_TABLE:
		MCprinter -> SetDeviceFontTable(ep . getcstring());
	return ES_NORMAL;
	
	case P_PRINT_TEXT_ALIGN:
	case P_PRINT_TEXT_FONT:
	case P_PRINT_TEXT_HEIGHT:
	case P_PRINT_TEXT_SIZE:
	case P_PRINT_TEXT_STYLE:
		// Ignore these for now
		return ES_NORMAL;

	case P_DIALOG_DATA:
		MCdialogdata->store(ep, True);
		break;

	case P_ERROR_MODE:
	{
		MCString t_value;
		t_value = ep.getsvalue();
		if (t_value == "inline")
			MCS_set_errormode(kMCSErrorModeInline);
		else if (t_value == "stderr")
			MCS_set_errormode(kMCSErrorModeStderr);
		else if (t_value == "quiet")
			MCS_set_errormode(kMCSErrorModeQuiet);
		else
		{
			MCeerror -> add(EE_ERRORMODE_BADVALUE, line, pos);
			return ES_ERROR;
		}
	}
	break;

	case P_OUTPUT_TEXT_ENCODING:			
	{
		MCString t_value;
		t_value = ep.getsvalue();
		
		MCSOutputTextEncoding t_encoding;
		if (t_value == "windows" || t_value == "windows-1252")
			t_encoding = kMCSOutputTextEncodingWindows1252;
		else if (t_value == "mac" || t_value == "macroman" || t_value == "macintosh")
			t_encoding = kMCSOutputTextEncodingMacRoman;
		else if (t_value == "linux" || t_value == "iso-8859-1")
			t_encoding = kMCSOutputTextEncodingISO8859_1;
		else if (t_value == "utf8" || t_value == "utf-8")
			t_encoding = kMCSOutputTextEncodingUTF8;
		else if (t_value == "native")
			t_encoding = kMCSOutputTextEncodingNative;
		else
		{
			MCeerror -> add(EE_OUTPUTENCODING_BADVALUE, line, pos);
			return ES_ERROR;
		}
		
		MCS_set_outputtextencoding(t_encoding);
	}
	break;
	
	case P_OUTPUT_LINE_ENDINGS:
	{
		MCString t_value;
		t_value = ep.getsvalue();
		
		MCSOutputLineEndings t_ending;
		if (t_value == "lf")
			t_ending = kMCSOutputLineEndingsLF;
		else if (t_value == "cr")
			t_ending = kMCSOutputLineEndingsCR;
		else if (t_value == "crlf")
			t_ending = kMCSOutputLineEndingsCRLF;
		else
		{
			MCeerror -> add(EE_OUTPUTLINEENDING_BADVALUE, line, pos);
			return ES_ERROR;
		}
		
		MCS_set_outputlineendings(t_ending);
	}
	break;
	case P_SESSION_SAVE_PATH:
	{
		if (!MCS_set_session_save_path(ep.getcstring()))
		{
			MCeerror->add(EE_SESSION_SAVE_PATH_BADVALUE, line, pos);
			return ES_ERROR;
		}
	}
	break;
	case P_SESSION_LIFETIME:
	{
		uint32_t t_lifetime;
		if (!MCU_stoui4(ep.getsvalue(), t_lifetime))
		{
			MCeerror->add(EE_SESSION_LIFETIME_BADVALUE, line, pos);
			return ES_ERROR;
		}
		MCS_set_session_lifetime(t_lifetime);
	}
	break;
	case P_SESSION_COOKIE_NAME:
	{
		if (!MCS_set_session_name(ep.getcstring()))
		{
			MCeerror->add(EE_SESSION_NAME_BADVALUE, line, pos);
			return ES_ERROR;
		}
	}
	break;
	case P_SESSION_ID:
	{
		if (!MCS_set_session_id(ep.getcstring()))
		{
			MCeerror->add(EE_SESSION_ID_BADVALUE, line, pos);
			return ES_ERROR;
		}
	}
	break;
			
	case P_REV_RUNTIME_BEHAVIOUR:
		return ep.getuint4(MCruntimebehaviour, line, pos, EE_PROPERTY_NAN);
	break;

	case P_CLIPBOARD_DATA:
	case P_DRAG_DATA:
	{
		MCTransferData *t_pasteboard;
		if (which == P_CLIPBOARD_DATA)
			t_pasteboard = MCclipboarddata;
		else
			t_pasteboard = MCdragdata;

		MCTransferType t_type;
		if (customindex == NULL)
			t_type = TRANSFER_TYPE_TEXT;
		else
		{
			MCExecPoint ep2(ep);
			if (customindex -> eval(ep2) != ES_NORMAL)
			{
				MCeerror -> add(EE_PROPERTY_BADEXPRESSION, line, pos);
				return ES_ERROR;
			}

			t_type = MCTransferData::StringToType(ep2 . getsvalue());
		}

		if (t_type != TRANSFER_TYPE_NULL)
		{
			MCSharedString *t_data;
			
			// MW-2014-03-12: [[ ClipboardStyledText ]] If styledText is being requested, then
			//   convert the array to a styles pickle and store that.
			if (t_type == TRANSFER_TYPE_STYLED_TEXT_ARRAY)
			{
				t_type =  TRANSFER_TYPE_STYLED_TEXT;
				t_data = MCConvertStyledTextArrayToStyledText(ep . getarray());
			}
			else
				t_data = MCSharedString::Create(ep . getsvalue());
			
			if (t_data != NULL)
			{
				bool t_success;
				t_success = t_pasteboard -> Store(t_type, t_data);
				t_data -> Release();
				if (!t_success)
					return ES_ERROR;
			}
		}
	}
	break;
	case P_HC_IMPORT_STAT:
		delete MChcstat;
		MChcstat = ep.getsvalue().clone();
		break;
	case P_SCRIPT_TEXT_FONT:
		delete MCscriptfont;
		MCscriptfont = ep.getsvalue();
		break;
	case P_SCRIPT_TEXT_SIZE:
		return ep.getuint2(MCscriptsize, line, pos, EE_PROPERTY_NAN);
	case P_LOOK_AND_FEEL:
		{
			MCTheme *newtheme = NULL;
			MCField *oldactive = MCactivefield;
			if (oldactive != NULL)
				oldactive->kunfocus();
			uint2 oldlook = MClook;
			MCTheme *oldtheme = MCcurtheme;
			MCcurtheme = NULL;
			if (ep.getsvalue() == MClnfwinstring)
				MClook = LF_WIN95;
			else if (ep.getsvalue() == MClnfmacstring)
					MClook = LF_MAC;
			else if (ep.getsvalue() == MClnfamstring)
					{
						if (!oldtheme ||
						        (oldtheme->getthemeid() != LF_NATIVEWIN
						         && oldtheme->getthemeid() != LF_NATIVEMAC &&
						         oldtheme->getthemeid() != LF_NATIVEGTK))
						{
					MCcurtheme = MCThemeCreateNative();
							if (MCcurtheme->load())
							{
								if (oldtheme != NULL)
									oldtheme -> unload();
								delete oldtheme;
								MClook = MCcurtheme->getthemefamilyid();
							}
							else
							{
								delete MCcurtheme;
								MCcurtheme = oldtheme;
								MClook = oldlook;
							}
						}
						else
						{
							MCcurtheme = oldtheme;
							MClook = oldlook;
						}
					}
					else
						MClook = LF_MOTIF;
			if (MClook != oldlook || MCcurtheme != oldtheme)
			{
				if (IsMacEmulatedLF())
				{
					MCtemplatescrollbar->alloccolors();
					MCtemplatebutton->allocicons();
				}
				if (oldtheme)
				{
					oldtheme->unload();
					delete oldtheme;
				}

				// MW-2011-08-17: [[ Redraw ]] Changing theme means we must dirty
				//   everything.
				MCRedrawDirtyScreen();
			}
			if (oldactive != NULL)

				oldactive->kfocus();
		}
		break;
	case P_SCREEN_MOUSE_LOC:
		if (!MCU_stoi2x2(ep.getsvalue(), mx, my))
		{
			MCeerror->add
			(EE_OBJECT_LOCNAP, 0, 0, ep.getsvalue());
			return ES_ERROR;
		}
		MCscreen->setmouse(mx, my);
		break;
	case P_UMASK:
		{
			uint4 newmask;
			if (ep.getuint4(newmask, line, pos, EE_PROPERTY_UMASKNAN) != ES_NORMAL)
				return ES_ERROR;
			MCS_umask(newmask);
			return ES_NORMAL;
		}
		break;
	case P_BACK_DROP:
		if (ep.getsvalue() != MCbackdropcolor)
		{
			delete MCbackdropcolor;
			if (MCbackdroppattern != nil)
				MCpatternlist->freepat(MCbackdroppattern);
			if (ep.getsvalue() != "none" && ep.getsvalue() != "0")
			{
				MCColor t_colour;
				char *t_colour_name = NULL;
				
				MCbackdropcolor = ep.getsvalue().clone();
				if (!strchr(MCbackdropcolor,','))
				{
					uint4 tmp = strtol(MCbackdropcolor, NULL, 10);
					if (tmp)
					{
						MCbackdroppmid = tmp;
						if (MCbackdroppmid < PI_END)
							MCbackdroppmid += PI_PATTERNS;
						MCPatternRef t_new_pattern = MCpatternlist->allocpat(MCbackdroppmid, parent);
						if (t_new_pattern != nil)
							MCbackdroppattern = t_new_pattern;
					}
				}
				if (MCbackdroppattern == nil && MCscreen -> parsecolor(MCbackdropcolor, &t_colour, &t_colour_name))
				{
					delete t_colour_name;
				}
				else
					t_colour . red = t_colour . green = t_colour . blue = 0;
				MCscreen -> configurebackdrop(t_colour, MCbackdroppattern, nil);
				MCscreen -> enablebackdrop();
			}
			else
			{
				MCscreen -> disablebackdrop();
				MCscreen -> configurebackdrop(MCscreen -> getwhite(), nil, nil);
				MCbackdropcolor = NULL;
			}
		}
		break;
	case P_BUFFER_MODE:
	case P_MULTI_EFFECT:
		break;
	case P_BUFFER_IMAGES:
		return ep.getboolean(MCbufferimages, line, pos, EE_PROPERTY_NAB);
		
	case P_ALLOW_INTERRUPTS:
		return ep.getboolean(MCallowinterrupts, line, pos, EE_PROPERTY_NAB);
		
	case P_EXPLICIT_VARIABLES:
		return ep.getboolean(MCexplicitvariables, line, pos, EE_PROPERTY_NAB);
		
	case P_PRESERVE_VARIABLES:
		return ep.getboolean(MCpreservevariables, line, pos, EE_PROPERTY_NAB);
		
	case P_SYSTEM_FS:
		return ep.getboolean(MCsystemFS, line, pos, EE_PROPERTY_NAB);
		
	case P_SYSTEM_CS:
		return ep.getboolean(MCsystemCS, line, pos, EE_PROPERTY_NAB);
		
	case P_SYSTEM_PS:
		return ep.getboolean(MCsystemPS, line, pos, EE_PROPERTY_NAB);
		
	case P_FILE_TYPE:
		delete MCfiletype;
		MCfiletype = ep.getsvalue().clone();
		return ES_NORMAL;
		
	case P_RECORD_FORMAT:
		if (ep.getsvalue() == "aiff")
			MCrecordformat = EX_AIFF;
		else
			if (ep.getsvalue() == "wave")
				MCrecordformat = EX_WAVE;
			else
				if (ep.getsvalue() == "ulaw")
					MCrecordformat = EX_ULAW;
				else
					MCrecordformat = EX_MOVIE;
		break;
		
	case P_RECORD_COMPRESSION:
		if (ep.getsvalue().getlength() != 4)
		{
			MCeerror->add
			(EE_RECORDCOMPRESSION_BADTYPE, 0, 0, ep.getsvalue());

			return ES_ERROR;
		}
		memcpy(MCrecordcompression, ep.getsvalue().getstring(), 4);
		break;
		
	case P_RECORD_INPUT:
		if (ep.getsvalue().getlength() != 4)
		{
			MCeerror->add
			(EE_RECORDINPUT_BADTYPE, 0, 0, ep.getsvalue());
			return ES_ERROR;
		}
		memcpy(MCrecordinput, ep.getsvalue().getstring(), 4);
		break;
		
	case P_RECORD_SAMPLESIZE:
		ep.getuint2(MCrecordsamplesize, line, pos, EE_PROPERTY_NAN);
		if (MCrecordsamplesize <= 8)
			MCrecordsamplesize = 8;
		else
			MCrecordsamplesize = 16;
		break;
		
	case P_RECORD_CHANNELS:
		ep.getuint2(MCrecordchannels, line, pos, EE_PROPERTY_NAN);
		if (MCrecordchannels <= 1)
			MCrecordchannels = 1;
		else
			MCrecordchannels = 2;
		break;
		
	case P_RECORD_RATE:
		ep.getreal8(MCrecordrate, line, pos, EE_PROPERTY_NAN);
		if (MCrecordrate <= (8.000 + 11.025) / 2.0)
			MCrecordrate = 8.000;
		else
			if (MCrecordrate <= (11.025 + 11.127) / 2.0)
				MCrecordrate = 11.025;
			else

				if (MCrecordrate <= (11.127 + 22.050) / 2.0)
					MCrecordrate = 12.000;
				else
					if (MCrecordrate <= (22.050 + 22.255) / 2.0)
						MCrecordrate = 22.050;
					else
						if (MCrecordrate <= (22.255 + 32.000) / 2.0)
							MCrecordrate = 24.000;
						else
							if (MCrecordrate <= (32.000 + 44.100) / 2.0)
								MCrecordrate = 32.000;
							else
								if (MCrecordrate <= (44.100 + 48.000) / 2.0)
									MCrecordrate = 44.100;
								else
									MCrecordrate = 48.000;
		break;
		
	case P_BREAK_POINTS:
		MCB_parsebreaks(ep);
		break; // MW-2004-11-26: Reinstated 'break' (VG)
	
	case P_DEBUG_CONTEXT:
		{
			char *buffer = ep.getsvalue().clone();
			char *cptr = strrchr(buffer, ',');
			if (cptr != NULL)
			{
				*cptr++ = '\0';
				uint4 l = strtol(cptr, NULL, 10);
				ep.setsvalue(buffer);
				MCObject *objptr = getobj(ep);
				uint2 i;
				for (i = 0 ; i < MCnexecutioncontexts ; i++)
				{
					if (MCexecutioncontexts[i]->getobj() == objptr
					        && MCexecutioncontexts[i]->getline() == l)
					{
						MCdebugcontext = i;
						break;
					}
				}
			}
			else
			{
				int4 i;
				char *t_end;
				i = strtol(buffer, &t_end, 10);
				if (t_end != buffer && i <= MCnexecutioncontexts)
					MCdebugcontext = i - 1;
				else
					MCdebugcontext = MAXUINT2;
			}
			delete buffer;
		}
		return ES_NORMAL;
	case P_MESSAGE_MESSAGES:
		return ep.getboolean(MCmessagemessages, line, pos, EE_PROPERTY_NAB);
	case P_WATCHED_VARIABLES:
		return MCB_parsewatches(ep);
	case P_ALLOW_INLINE_INPUT:
		return ep.getboolean(MCinlineinput, line, pos, EE_PROPERTY_NAB);
	case P_ACCEPT_DROP:
		stat = ep.getboolean(b, line, pos, EE_PROPERTY_NAB);
		if (stat == ES_NORMAL)
			MCdragaction = b ? DRAG_ACTION_COPY : DRAG_ACTION_NONE;
		return stat;
	case P_DRAG_ACTION:
		if (ep . getsvalue() == "copy")
			MCdragaction = DRAG_ACTION_COPY;
		else if (ep . getsvalue() == "move")
			MCdragaction = DRAG_ACTION_MOVE;
		else if (ep . getsvalue() == "link")
			MCdragaction = DRAG_ACTION_LINK;
		else if (ep . getsvalue() == "none")
			MCdragaction = DRAG_ACTION_NONE;
		else
		{
			MCeerror -> add(EE_DRAGDROP_BADACTION, line, pos);
			return ES_ERROR;
		}
		return ES_NORMAL;
	case P_ALLOWABLE_DRAG_ACTIONS:
	{
		MCDragActionSet t_action_set;
		t_action_set = 0;

		const char *t_actions;
		t_actions = ep . getcstring();
		while(t_actions[0] != '\0')
		{
			const char *t_next_action;
			t_next_action = strchr(t_actions, ',');
			if (t_next_action == NULL)
				t_next_action = t_actions + strlen(t_actions);

			MCString t_token(t_actions, t_next_action - t_actions);
			if (t_token == "copy")
				t_action_set |= DRAG_ACTION_COPY;
			else if (t_token == "move")
				t_action_set |= DRAG_ACTION_MOVE;
			else if (t_token == "link")
				t_action_set |= DRAG_ACTION_LINK;
			else
			{
				MCeerror -> add(EE_DRAGDROP_BADACTION, line, pos);
				return ES_ERROR;
			}

			if (*t_next_action == ',')
				t_actions = t_next_action + 1;
			else
				t_actions = t_next_action;
		}

		MCallowabledragactions = t_action_set;
	}
	return ES_NORMAL;
	case P_DRAG_IMAGE:
		stat = ep.getuint4(MCdragimageid, line, pos, EE_PROPERTY_NAN);
		return stat;
	case P_DRAG_IMAGE_OFFSET:
	{
		int2 x, y;
		if (!MCU_stoi2x2(ep . getsvalue(), x, y))
		{
			MCeerror -> add(EE_DRAGDROP_BADIMAGEOFFSET, line, pos);
			return ES_ERROR;
		}
		MCdragimageoffset . x = x;
		MCdragimageoffset . y = y;
	}
	return ES_NORMAL;
	case P_DRAG_DELTA:
		return ep.getuint2(MCdragdelta, line, pos, EE_PROPERTY_BADDRAGDELTA);
	case P_STACK_FILE_TYPE:
		delete MCstackfiletype;
        MCstackfiletype = ep.getsvalue().clone();
		return ES_NORMAL;
	case P_STACK_FILE_VERSION:
		{
			uint4 major = 0, minor = 0, revision = 0, version = 0;
			uint4 count;
			// MW-2006-03-24: This should be snscanf - except it doesn't exist on BSD!!
			char *t_version;
			
			t_version = ep . getsvalue() . clone();
			count = sscanf(t_version, "%d.%d.%d", &major, &minor, &revision);
			delete t_version;
			
			version = major * 1000 + minor * 100 + revision * 10;
			
			// MW-2012-03-04: [[ StackFile5500 ]] Allow versions up to 5500 to be set.
			if (count < 2 || version < 2400 || version > 5500)
			{
				MCeerror -> add(EE_PROPERTY_STACKFILEBADVERSION, 0, 0, ep . getsvalue());
				return ES_ERROR;
			}
			MCstackfileversion = version;
		}
		return ES_NORMAL;
	// MW-2013-11-05: [[ Bug 11114 ]] Reinstate ability to set the securityPermissions.
	case P_SECURITY_PERMISSIONS:
		uint4 t_secmode;
		t_secmode = 0;
		
		const char *t_modes;
		t_modes = ep . getcstring();
		while(t_modes[0] != '\0')
		{
			const char *t_next_mode;
			t_next_mode = strchr(t_modes, ',');
			if (t_next_mode == NULL)
				t_next_mode = t_modes + strlen(t_modes);
			
			MCString t_token(t_modes, t_next_mode - t_modes);
			
			// MW-2009-06-29: If the token is empty, ignore it.
			if (t_token . getlength() != 0)
			{
				int i = 0;
				int t_bitmask = 1;
				while (i < MC_SECUREMODE_MODECOUNT)
				{
					if (t_token == MCsecuremode_strings[i])
					{
						t_secmode |= t_bitmask;
						break;
					}
					i++;
					t_bitmask <<= 1;
				}
				
				if (i == MC_SECUREMODE_MODECOUNT)
				{
					// MW-2009-06-29: Pass the token as a hint.
					MCeerror -> add(EE_SECUREMODE_BADCATEGORY, line, pos, t_token);
					return ES_ERROR;
				}
			}
			
			if (*t_next_mode == ',')
				t_modes = t_next_mode + 1;
			else
				t_modes = t_next_mode;
		}
		
		MCsecuremode |= (~t_secmode) & MC_SECUREMODE_ALL;
		break;
	case P_SECURE_MODE:
		MCnofiles = True;
		MCsecuremode = MC_SECUREMODE_ALL;
		return ES_NORMAL;
	case P_SERIAL_CONTROL_STRING:
		delete MCserialcontrolsettings;
		MCserialcontrolsettings = ep.getsvalue().clone();
		return ES_NORMAL;
	case P_EDIT_SCRIPTS:
	case P_COLOR_WORLD:
	case P_ALLOW_KEY_IN_FIELD:
	case P_ALLOW_FIELD_REDRAW:
	case P_REMAP_COLOR:
		return ES_NORMAL;
	case P_HIDE_CONSOLE_WINDOWS:
		return ep.getboolean(MChidewindows, line, pos, EE_PROPERTY_NAB);
	case P_FTP_PROXY:
		delete MCftpproxyhost;
		if (ep.getsvalue().getlength() == 0)
			MCftpproxyhost = NULL;
		else
		{
			MCftpproxyhost = ep.getsvalue().clone();
			if ((eptr = strchr(MCftpproxyhost, ':')) != NULL)
			{
				*eptr++ = '\0';
				MCftpproxyport = (uint2)strtol(eptr, NULL, 10);
			}
			else
				MCftpproxyport = 80;
		}
		break;
	case P_HTTP_HEADERS:
		delete MChttpheaders;
		if (ep.getsvalue().getlength() == 0)
			MChttpheaders = NULL;
		else
			MChttpheaders = ep.getsvalue().clone();
		break;
	case P_HTTP_PROXY:
		delete MChttpproxy;
		if (ep . getsvalue() . getlength() == 0)
			MChttpproxy = NULL;
		else
			MChttpproxy = ep . getsvalue() . clone();
		break;
	case P_SHOW_INVISIBLES:
		stat = ep.getboolean(MCshowinvisibles, line, pos, EE_PROPERTY_NAB);

		// MW-2011-08-17: [[ Redraw ]] Things are now visible that weren't before so dirty the screen.
		MCRedrawDirtyScreen();
		return stat;
	case P_SOCKET_TIMEOUT:
		if (ep.getreal8(MCsockettimeout, line, pos,
		                EE_PROPERTY_SOCKETTIMEOUTNAN) != ES_NORMAL)
			return ES_ERROR;
		if (MCsockettimeout < 1.0)
			MCsockettimeout = 0.001;
		else
			MCsockettimeout /= 1000.0;
		break;
	case P_RANDOM_SEED:
		if (ep.getint4(MCrandomseed, line, pos,
		               EE_PROPERTY_RANDOMSEEDNAN) != ES_NORMAL)
			return ES_ERROR;

		MCU_srand();
		break;
	case P_USER_LEVEL:
		return ep.getuint2(MCuserlevel, line, pos, EE_PROPERTY_BADUSERLEVEL);
	case P_USER_MODIFY:
		return ep.getboolean(MCusermodify, line, pos, EE_PROPERTY_NAB);
	case P_CURSOR:
		{
			uint4 cid;
			if (ep.getuint4(cid, line, pos, EE_PROPERTY_CURSORNAN) != ES_NORMAL)
				return ES_ERROR;
			if (cid < PI_NCURSORS)
			{
				if (cid == PI_BUSY)
				{
					cid = PI_BUSY1 + MCbusycount;
					MCbusycount = MCbusycount + 1 & 0x7;
				}
				MCcursor = MCcursors[cid];
			}
			else
			{
				// MW-2009-02-02: [[ Improved image search ]]
				// Search for the appropriate image object using the standard method - note
				// here we use the object attached to the execution context rather than the
				// 'parent' - this ensures that the search follows the use of behavior.
				newim = ep . getobj() -> resolveimageid(cid);
				if (newim == NULL)
				{
					if (cid == 28)
						MCcursor = MCcursors[8];
					else if (cid == 29)
						MCcursor = MCcursors[1];
					else
					{
						MCeerror->add(EE_PROPERTY_CURSORNOIMAGE, line, pos, ep.getsvalue());
						return ES_ERROR;
					}
				}
				if (newim)
					MCcursor = newim->getcursor();
			}
			MCcursorid = cid;
			if (MCmousestackptr != NULL)
				MCmousestackptr->resetcursor(True);
			else
				MCdefaultstackptr->resetcursor(True);
		}
		break;
	case P_DEFAULT_CURSOR:
		{
			uint4 cid;
			
			if (ep.getuint4(cid, line, pos, EE_PROPERTY_CURSORNAN) != ES_NORMAL)
				return ES_ERROR;
				
			if (cid < PI_NCURSORS)
				MCdefaultcursor = MCcursors[cid];
			else
			{
				// MW-2009-02-02: [[ Improved image search ]]
				// Search for the appropriate image object using the standard method - note
				// here we use the object attached to the execution context rather than the
				// 'parent' - this ensures that the search follows the use of behavior.
				newim = ep . getobj() -> resolveimageid(cid);

				if (newim == NULL)
				{
					if (cid == 28)
						MCdefaultcursor = MCcursors[8];
					else if (cid == 29)
						MCdefaultcursor = MCcursors[1];
					else
					{
						MCeerror->add(EE_PROPERTY_CURSORNOIMAGE, line, pos, ep.getsvalue());
						return ES_ERROR;
					}
				}
				else
					MCdefaultcursor = newim->getcursor(true);
			}
			
			MCdefaultcursorid = cid;
			if (MCmousestackptr != NULL)
				MCmousestackptr->resetcursor(True);
			else
				MCdefaultstackptr->resetcursor(True);
		}

		break;
	case P_DEFAULT_STACK:
		{
			MCStack *sptr = MCdefaultstackptr->findstackname(ep.getsvalue());
			if (sptr == NULL)
			{
				MCeerror->add
				(EE_PROPERTY_NODEFAULTSTACK, line, pos, ep.getsvalue());
				return ES_ERROR;
			}
			MCdefaultstackptr = MCstaticdefaultstackptr = sptr;
		}
		break;
	case P_DEFAULT_MENU_BAR:
		{
            MCGroup *gptr = (MCGroup *)MCdefaultstackptr->getobjname(CT_GROUP, ep.getsvalue());
            
            if (gptr == NULL)
            {
                // AL-2014-10-31: [[ Bug 13884 ]] Resolve chunk properly if the name is not found
                //  so that setting the defaultMenubar by the long id of a group works.
                MCObject *optr = getobj(ep);
                if (optr == NULL || optr -> gettype() != CT_GROUP)
                {
                    MCeerror->add(EE_PROPERTY_NODEFAULTMENUBAR, line, pos, ep.getsvalue());
                    return ES_ERROR;
                }
                gptr = (MCGroup *)optr;
            }

			MCdefaultmenubar = gptr;
			MCscreen->updatemenubar(False);
		}
		break;

	case P_DRAG_SPEED:
		return ep.getuint2(MCdragspeed, line, pos, EE_PROPERTY_BADDRAGSPEED);
	case P_MOVE_SPEED:
		return ep.getuint2(MCmovespeed, line, pos, EE_PROPERTY_BADMOVESPEED);
	case P_LOCK_COLORMAP:
		if (ep.getboolean(MClockcolormap, line, pos, EE_PROPERTY_NAB) != ES_NORMAL)
			return ES_ERROR;
		break;
	case P_LOCK_CURSOR:
		if (ep.getboolean(MClockcursor, line, pos, EE_PROPERTY_NAB) != ES_NORMAL)
			return ES_ERROR;
		if (!MClockcursor)
			ep.getobj()->getstack()->resetcursor(True);
		break;
	case P_LOCK_ERRORS:
		MCerrorlockptr = ep.getobj();
		return ep.getboolean(MClockerrors, line, pos, EE_PROPERTY_NAB);

	case P_LOCK_MENUS:
		if (ep.getboolean(MClockmenus, line, pos, EE_PROPERTY_NAB) != ES_NORMAL)
			return ES_ERROR;
		MCscreen->updatemenubar(True);
		break;
	case P_LOCK_MESSAGES:
		return ep.getboolean(MClockmessages, line, pos, EE_PROPERTY_NAB);
	case P_LOCK_MOVES:
		{
			Boolean t_new_value;
			Exec_stat t_stat = ep.getboolean(t_new_value, line, pos, EE_PROPERTY_NAB);
			if (t_stat == ES_NORMAL) {
				MCscreen->setlockmoves(t_new_value);
			}
			return t_stat;
		}
	case P_LOCK_RECENT:
		return ep.getboolean(MClockrecent, line, pos, EE_PROPERTY_NAB);
	case P_PRIVATE_COLORS:
		return ep.getboolean(MCuseprivatecmap, line, pos, EE_PROPERTY_NAB);
	case P_TWELVE_TIME:
		return ep.getboolean(MCtwelvetime, line, pos, EE_PROPERTY_NAB);
	case P_IDLE_RATE:
		if (ep.getuint2(tir, line, pos, EE_PROPERTY_BADIDLERATE) != ES_NORMAL)
			return ES_ERROR;
		MCidleRate = MCU_max(tir, 1);
		break;
	case P_QT_IDLE_RATE:
		if (ep.getuint2(tir, line, pos, EE_PROPERTY_BADIDLERATE) != ES_NORMAL)
			return ES_ERROR;
		MCqtidlerate = MCU_max(tir, 1);
		break;
	case P_IDLE_TICKS:
		if (ep.getuint2(tir, line, pos, EE_PROPERTY_BADIDLERATE) != ES_NORMAL)
			return ES_ERROR;
		MCidleRate = MCU_max(tir * 1000 / 60, 1);
		break;
	case P_BLINK_RATE:
		if (ep.getuint2(MCblinkrate, line, pos,
		                EE_PROPERTY_BADBLINKRATE) != ES_NORMAL)
			return ES_ERROR;
		MCblinkrate = MCU_max(MCblinkrate, 1);

		break;
	case P_RECURSION_LIMIT:
		if (ep.getuint4(MCrecursionlimit, line, pos, EE_PROPERTY_NAN) != ES_NORMAL)
			return ES_ERROR;
#ifdef _WINDOWS
		MCrecursionlimit = MCU_min(MCstacklimit - MC_UNCHECKED_STACKSIZE, MCU_max(MCrecursionlimit, MCU_max(MC_UNCHECKED_STACKSIZE, MCU_abs(MCstackbottom - (char *)&stat) * 3)));
#else
		MCrecursionlimit = MCU_max(MCrecursionlimit, MCU_abs(MCstackbottom - (char *)&stat) * 3); // fudge to 3x current stack depth
#endif
		break;
	case P_REPEAT_RATE:
		if (ep.getuint2(MCrepeatrate, line, pos,
		                EE_PROPERTY_BADREPEATRATE) != ES_NORMAL)
			return ES_ERROR;
		MCrepeatrate = MCU_max(MCrepeatrate, 1);
		break;
	case P_REPEAT_DELAY:
		return ep.getuint2(MCrepeatdelay, line, pos, EE_PROPERTY_BADREPEATDELAY);
	case P_TYPE_RATE:
		return ep.getuint2(MCtyperate, line, pos, EE_PROPERTY_BADTYPERATE);
	case P_SYNC_RATE:
		if (ep.getuint2(MCsyncrate, line, pos, EE_PROPERTY_BADSYNCRATE) != ES_NORMAL)
			return ES_ERROR;
		MCsyncrate = MCU_max(MCsyncrate, 1);
		break;
	case P_EFFECT_RATE:
		return ep.getuint2(MCeffectrate, line, pos, EE_PROPERTY_BADEFFECTRATE);
	case P_DOUBLE_DELTA:
		return ep.getuint2(MCdoubledelta, line, pos, EE_PROPERTY_BADDOUBLEDELTA);
	case P_DOUBLE_TIME:
		return ep.getuint2(MCdoubletime, line, pos, EE_PROPERTY_BADDOUBLETIME);
	case P_TOOL_TIP_DELAY:
		return ep.getuint2(MCtooltipdelay, line, pos, EE_PROPERTY_BADTOOLDELAY);
	case P_LONG_WINDOW_TITLES:
		return ep.getboolean(MClongwindowtitles, line, pos, EE_PROPERTY_NAB);
	case P_BLIND_TYPING:
		return ep.getboolean(MCblindtyping, line, pos, EE_PROPERTY_NAB);
	case P_POWER_KEYS:
		return ep.getboolean(MCpowerkeys, line, pos, EE_PROPERTY_NAB);
	case P_NAVIGATION_ARROWS:
		return ep.getboolean(MCnavigationarrows, line, pos, EE_PROPERTY_NAB);
	case P_TEXT_ARROWS:
		return ep.getboolean(MCtextarrows, line, pos, EE_PROPERTY_NAB);
	case P_EXTEND_KEY:
		return ep.getuint2(MCextendkey, line, pos, EE_PROPERTY_BADEXTENDKEY);
	case P_COLORMAP:
		if (!MCscreen->setcolors(ep.getsvalue()))
		{
			MCeerror->add
			(EE_PROPERTY_BADCOLORS, line, pos);
			return ES_ERROR;
		}
		break;
	case P_NO_PIXMAPS:
		return ep.getboolean(MCnopixmaps, line, pos, EE_PROPERTY_NAB);
	case P_LOW_RESOLUTION_TIMERS:
		return ep.getboolean(MClowrestimers, line, pos, EE_PROPERTY_NAB);
	case P_POINTER_FOCUS:
		return ep.getboolean(MCpointerfocus, line, pos, EE_PROPERTY_NAB);
	case P_EMACS_KEY_BINDINGS:
		return ep.getboolean(MCemacskeys, line, pos, EE_PROPERTY_NAB);
	case P_RAISE_MENUS:
		return ep.getboolean(MCraisemenus, line, pos, EE_PROPERTY_NAB);
	case P_ACTIVATE_PALETTES:
		return ep.getboolean(MCactivatepalettes, line, pos, EE_PROPERTY_NAB);
	case P_HIDE_PALETTES:
		stat = ep.getboolean(MChidepalettes, line, pos, EE_PROPERTY_NAB);
#ifdef _MACOSX
        // MW-2014-04-23: [[ Bug 12080 ]] Make sure we update the hidesOnSuspend of all palettes.
        MCstacks -> hidepaletteschanged();
#endif
        return stat;
	case P_RAISE_PALETTES:
		// MW-2004-11-17: On Linux, effect a restack if 'raisepalettes' is changed
		// MW-2004-11-24: Altered MCStacklst::restack to find right stack if passed NULL
		stat = ep.getboolean(MCraisepalettes, line, pos, EE_PROPERTY_NAB);
#ifdef LINUX
		MCstacks -> restack(NULL);
#endif
		return stat;
	case P_RAISE_WINDOWS:
	  stat = ep.getboolean(MCraisewindows, line, pos, EE_PROPERTY_NAB);
		if (stat == ES_NORMAL)
			MCscreen -> enactraisewindows();
		return stat;
	case P_HIDE_BACKDROP:
		return ep.getboolean(MChidebackdrop, line, pos, EE_PROPERTY_NAB);
	case P_DONT_USE_NS:
		return ep.getboolean(MCdontuseNS, line, pos, EE_PROPERTY_NAB);
	
	case P_DONT_USE_QT_EFFECTS:
		return ep.getboolean(MCdontuseQTeffects, line, pos, EE_PROPERTY_NAB);
	case P_PROPORTIONAL_THUMBS:
		if (IsMacLFAM())
			return ES_NORMAL;
		else
		{
			stat = ep.getboolean(MCproportionalthumbs, line, pos, EE_PROPERTY_NAB);

			// MW-2011-08-17: [[ Redraw ]] This affects the redraw of any scrollbar so dirty everything.
			MCRedrawDirtyScreen();
		}
		return stat;
	case P_SHARED_MEMORY:
		return ep.getboolean(MCshm, line, pos, EE_PROPERTY_NAB);
	case P_VC_SHARED_MEMORY:
		return ep.getboolean(MCvcshm, line, pos, EE_PROPERTY_NAB);
	case P_VC_PLAYER:
		delete MCvcplayer;
		MCvcplayer = ep.getsvalue().clone();
		break;
	case P_SCREEN_GAMMA:
		return ep.getreal8(MCgamma, line, pos, EE_PROPERTY_NAN);

	// IM-2013-12-04: [[ PixelScale ]] Enable setting of pixelScale to override default system value
	// IM-2013-12-06: [[ PixelScale ]] Remove handling of empty pixelScale - should always have a numeric value
	case P_PIXEL_SCALE:
		{
			real64_t t_scale;
			stat = ep.getreal8(t_scale, line, pos, EE_PROPERTY_NAN);
			
			if (stat != ES_NORMAL)
				return stat;
			
			if (t_scale <= 0)
			{
				MCeerror->add(EE_PROPERTY_BADPIXELSCALE, line, pos, t_scale);
				return ES_ERROR;
			}
			
			// IM-2014-01-30: [[ HiDPI ]] It is an error to set the pixelScale on platforms that do not support this
			if (!MCResPlatformCanSetPixelScale())
			{
				MCeerror->add(EE_PROPERTY_PIXELSCALENOTSUPPORTED, line, pos, t_scale);
				return ES_ERROR;
			}
			
			if (MCResGetUsePixelScaling())
				MCResSetPixelScale(t_scale);
		}
		break;

	// IM-2014-01-24: [[ HiDPI ]] Enable or disable pixel scaling on Hi-DPI displays
	case P_USE_PIXEL_SCALING:
		{
			Boolean t_pixel_scaling;
			stat = ep.getboolean(t_pixel_scaling, line, pos, EE_PROPERTY_NAB);
			if (stat != ES_NORMAL)
				return stat;
			
			// IM-2014-01-30: [[ HiDPI ]] It is an error to set the usePixelScale on platforms that do not support this
			if (!MCResPlatformCanChangePixelScaling())
			{
				MCeerror->add(EE_PROPERTY_USEPIXELSCALENOTSUPPORTED, line, pos, t_pixel_scaling);
				return ES_ERROR;
			}
			
			MCResSetUsePixelScaling(t_pixel_scaling);
		}
		break;

	case P_SHELL_COMMAND:
		delete MCshellcmd;
		MCshellcmd = ep.getsvalue().clone();
		break;
	case P_SOUND_CHANNEL:
		return ep.getuint2(MCsoundchannel, line, pos, EE_PROPERTY_NAN);
	case P_RELAYER_GROUPED_CONTROLS:
		return ep.getboolean(MCrelayergrouped, line, pos, EE_PROPERTY_NAB);
	case P_SELECTION_MODE:
		MCselectintersect = ep.getsvalue() == MCintersectstring;
		break;
	case P_SELECTION_HANDLE_COLOR:
		stat = MCU_change_color(MCselectioncolor, MCselectioncolorname, ep, line, pos);
		MCselected->redraw();
		return stat;
	case P_WINDOW_BOUNDING_RECT:
		{
			int2 i1, i2, i3, i4;
			if (!MCU_stoi2x4(ep.getsvalue(), i1, i2, i3, i4))
			{
				MCeerror->add
				(EE_OBJECT_NAR, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			MCwbr.x = i1;
			MCwbr.y = i2;
			MCwbr.width = MCU_max(i3 - i1, 1);
			MCwbr.height = MCU_max(i4 - i2, 1);
		}
		break;
	case P_JPEG_QUALITY:
		if (ep.getuint2(MCjpegquality, line, pos, EE_PROPERTY_NAN) != ES_NORMAL)
			return ES_ERROR;
		MCjpegquality = MCU_min(MCjpegquality, 100);
		break;
	case P_RECORDING:
		{
			Boolean trecording;
			if (ep.getboolean(trecording, line, pos, EE_PROPERTY_NAB) != ES_NORMAL)
				return ES_ERROR;
			if (!trecording)
			{
#ifdef FEATURE_PLATFORM_RECORDER
                bool t_recording;
                t_recording = false;
                
                extern MCPlatformSoundRecorderRef MCrecorder;
                
                if (MCrecorder != nil)
                    t_recording = MCPlatformSoundRecorderIsRecording(MCrecorder);
#else
				extern void MCQTStopRecording(void);
				MCQTStopRecording();
#endif
			}
		}
		break;
	case P_LZW_KEY:
		if (ep.getsvalue().getlength() == 8 && ep.getsvalue().getstring()[0]
		        + ep.getsvalue().getstring()[1]	+ ep.getsvalue().getstring()[2]
		        + ep.getsvalue().getstring()[3]	+ ep.getsvalue().getstring()[4]
		        + ep.getsvalue().getstring()[5]	+ ep.getsvalue().getstring()[6]
		        + ep.getsvalue().getstring()[7] == 800)
			MCuselzw = True;
		break;
	case P_TRACE_ABORT:
		return ep.getboolean(MCtraceabort, line, pos, EE_PROPERTY_NAB);
	case P_TRACE_DELAY:
		return ep.getuint2(MCtracedelay, line, pos, EE_PROPERTY_BADTRACEDELAY);
	case P_TRACE_RETURN:
		return ep.getboolean(MCtracereturn, line, pos, EE_PROPERTY_NAB);
	case P_TRACE_STACK:
		if (ep.getsvalue().getlength() == 0)
			MCtracestackptr = NULL;
		else
		{
			MCStack *sptr = MCdefaultstackptr->findstackname(ep.getsvalue());
			if (sptr == NULL)
			{
				MCeerror->add
				(EE_PROPERTY_BADTRACESTACK, line, pos, ep.getsvalue());
				return ES_ERROR;
			}
			MCtracestackptr = sptr;
		}
		break;
	case P_TRACE_UNTIL:
		return ep.getuint4(MCtraceuntil, line, pos, EE_PROPERTY_BADTRACEDELAY);
	case P_DIRECTORY:
		{
			if (!MCSecureModeCheckDisk(line, pos))
				return ES_ERROR;

			char *path = ep.getsvalue().clone();
			if (!MCS_setcurdir(path))
				MCresult->sets("can't open directory");
			delete path;
		}
		break;
	case P_SSL_CERTIFICATES:
		{
			delete MCsslcertificates;
			MCsslcertificates = ep.getsvalue().clone();
		}
		break;
	case P_DEFAULT_NETWORK_INTERFACE:
		{
			if (ep.getsvalue().getlength() == 0)
			{
				delete MCdefaultnetworkinterface;
				MCdefaultnetworkinterface = NULL;
			}
			else
			{
				// MW-2013-07-01: [[ EnhancedFilter ]] Removed 'usecache' parameter as there's
				//   no reason not to use the cache.
				regexp *t_net_int_regex;
				t_net_int_regex = MCR_compile("\\b(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b", True /* casesensitive */);
				
                int t_net_int_valid;
				t_net_int_valid = MCR_exec(t_net_int_regex, ep.getsvalue().getstring(), ep.getsvalue().getlength());
				if (t_net_int_valid != 0)
				{
					delete MCdefaultnetworkinterface;
					MCdefaultnetworkinterface = ep.getsvalue().clone();
				}
				else
				{
					MCeerror -> add(EE_PROPERTY_BADNETWORKINTERFACE, line, pos, ep.getsvalue());
					return ES_ERROR;
				}
			}
		}
		break;
	case P_BRUSH:
	case P_ERASER:
	case P_SPRAY:
		{
			uint4 newbrush;
			if (ep.getuint4(newbrush, line, pos, EE_PROPERTY_BRUSHNAN) != ES_NORMAL)
				return ES_ERROR;
			if (newbrush < PI_PATTERNS)
				newbrush += PI_BRUSHES;
			
			// MW-2009-02-02: [[ Improved image search ]]
			// Search for the appropriate image object using the standard method - note
			// here we use the object attached to the execution context rather than the
			// 'parent' - this ensures that the search follows the use of behavior.
			newim = ep . getobj() -> resolveimageid(newbrush);
			
			if (newim == NULL)
			{
				MCeerror->add(EE_PROPERTY_BRUSHNOIMAGE, line, pos, ep.getsvalue());
				return ES_ERROR;
			}
			switch (which)
			{
			case P_BRUSH:
				MCbrush = newbrush;
				break;
			case P_ERASER:
				MCeraser = newbrush;
				break;
			case P_SPRAY:
				MCspray = newbrush;
				break;
			default:
				break;
			}
			newim->createbrush(which);
			ep.getobj()->getstack()->resetcursor(True);
		}
		break;
	case P_CENTERED:
		return ep.getboolean(MCcentered, line, pos, EE_PROPERTY_NAB);
	case P_GRID:
		return ep.getboolean(MCgrid, line, pos, EE_PROPERTY_NAB);
	case P_GRID_SIZE:
		if (ep.getuint2(MCgridsize, line, pos, EE_PROPERTY_BADGRIDSIZE) == ES_ERROR)
			return ES_ERROR;
		MCgridsize = MCU_max(MCgridsize, 1);
		return ES_NORMAL;
	case P_MULTIPLE:
		return ep.getboolean(MCmultiple, line, pos, EE_PROPERTY_NAB);
	case P_MULTI_SPACE:
		return ep.getuint2(MCmultispace, line, pos, EE_PROPERTY_BADMULTISPACE);
	case P_SLICES:
		if (ep.getuint2(MCslices, line, pos, EE_PROPERTY_BADSLICES) != ES_NORMAL)
			return ES_ERROR;
		if (MCslices < 2)
			MCslices = 2;
		break;
	case P_BEEP_LOUDNESS:
	case P_BEEP_PITCH:
	case P_BEEP_DURATION:
		{
			int4 beep;
			if (ep.getint4(beep, line, pos, EE_PROPERTY_BEEPNAN) != ES_NORMAL)
				return ES_ERROR;
			MCscreen->setbeep(which, beep);
		}
		break;
	case P_BEEP_SOUND:
		{
			if (!MCscreen -> setbeepsound( ep.getcstring() ))
				return ES_ERROR ;
		}
		break;
	case P_TOOL:
		return MCU_choose_tool(ep, T_UNDEFINED, line, pos);
	
	case P_ICON_MENU:
		if (MCiconmenu != NULL)
			delete MCiconmenu;
		if (ep.getsvalue().getlength() != 0)
			MCiconmenu = ep . getsvalue() . clone();
		else
			MCiconmenu = NULL;
		MCscreen -> seticonmenu(MCiconmenu);
		return ES_NORMAL;

	case P_STATUS_ICON:
	{
		uint4 t_new_icon_id;
		stat = ep.getuint4(t_new_icon_id, line, pos, EE_STACK_ICONNAN);
		if (stat == ES_NORMAL && t_new_icon_id != MCstatusiconid)
		{
			MCstatusiconid = t_new_icon_id;
			MCscreen -> configurestatusicon(MCstatusiconid, MCstatusiconmenu, MCstatusicontooltip);
		}
	}
	return stat;

	case P_STATUS_ICON_TOOLTIP:
		delete MCstatusicontooltip;
		if (ep.getsvalue().getlength() != 0)
			MCstatusicontooltip = ep . getsvalue() . clone();
		else
			MCstatusicontooltip = NULL;
		MCscreen -> configurestatusicon(MCstatusiconid, MCstatusiconmenu, MCstatusicontooltip);
		break;

	case P_STATUS_ICON_MENU:
		if (MCstatusiconmenu != NULL)
			delete MCstatusiconmenu;
		if (ep.getsvalue().getlength() != 0)
			MCstatusiconmenu = ep . getsvalue() . clone();
		else
			MCstatusiconmenu = NULL;
		MCscreen -> configurestatusicon(MCstatusiconid, MCstatusiconmenu, MCstatusicontooltip);
		return ES_NORMAL;

	case P_PROCESS_TYPE:
	{
		bool t_wants_foreground;
		if (ep . getsvalue() == "foreground")
			t_wants_foreground = true;
		else if (ep . getsvalue() == "background")
			t_wants_foreground = false;
		else
		{
			MCeerror -> add(EE_PROCESSTYPE_BADVALUE, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		
		if (!MCS_changeprocesstype(t_wants_foreground))
		{
			MCeerror -> add(EE_PROCESSTYPE_NOTPOSSIBLE, line, pos);
			return ES_ERROR;
		}
	}
	break;

	case P_STACK_LIMIT:
		return ep . getuint4(MCpendingstacklimit, line, pos, EE_STACKLIMIT_NAN);
	
	case P_IMAGE_CACHE_LIMIT:
	{
		uint32_t t_cache_limit;
		if (ES_ERROR == ep.getuint4(t_cache_limit, line, pos, EE_PROPERTY_BADIMAGECACHELIMIT))
			return ES_ERROR;
		MCCachedImageRep::SetCacheLimit(t_cache_limit);
	}
	break;
	
	
	case P_ALLOW_DATAGRAM_BROADCASTS:
		return ep . getboolean(MCallowdatagrambroadcasts, line, pos, EE_PROPERTY_NAB);
	
		// MERG-2013-08-17: [[ ColorDialogColors ]] Custom color management for the windows color dialog
	case P_COLOR_DIALOG_COLORS:
		MCA_setcolordialogcolors(ep);
		return ES_NORMAL;

	case P_BRUSH_COLOR:
	case P_DONT_USE_QT:
	case P_BRUSH_BACK_COLOR:
	case P_BRUSH_PATTERN:
	case P_PEN_COLOR:
	case P_PEN_BACK_COLOR:
	case P_PEN_PATTERN:
	case P_TEXT_ALIGN:
	case P_TEXT_FONT:
	case P_TEXT_HEIGHT:
	case P_TEXT_SIZE:
	case P_TEXT_STYLE:
	case P_EDIT_BACKGROUND:
	case P_ROUND_ENDS:
	case P_DASHES:
	case P_FILLED:
	case P_POLY_SIDES:
	case P_LINE_SIZE:
	case P_PEN_WIDTH:
	case P_PEN_HEIGHT:
	case P_ROUND_RADIUS:
	case P_START_ANGLE:
	case P_ARC_ANGLE:
	case P_NUMBER_FORMAT:
	case P_PLAY_DESTINATION:
	case P_PLAY_LOUDNESS:
	case P_LOCK_SCREEN:
	case P_STACK_FILES:
	case P_MENU_BAR:
	case P_EDIT_MENUS:
	case P_ACCENT_COLOR:
	case P_HILITE_COLOR:
	case P_PAINT_COMPRESSION:
	case P_LINK_COLOR:
	case P_LINK_HILITE_COLOR:
	case P_LINK_VISITED_COLOR:
	case P_UNDERLINE_LINKS:
	case P_SELECT_GROUPED_CONTROLS:
	case P_ICON:
		if (target == NULL)
		{
			switch (which)
			{
			case P_BRUSH_PATTERN:
				{
					if (ep.getuint4(MCbrushpmid, line, pos, EE_PROPERTY_BRUSHPATNAN) != ES_NORMAL)
						return ES_ERROR;
					if (MCbrushpmid < PI_END)
						MCbrushpmid += PI_PATTERNS;
					MCPatternRef t_new_pattern = MCpatternlist->allocpat(MCbrushpmid, parent);
					if (t_new_pattern == None)
					{
						MCeerror->add(EE_PROPERTY_BRUSHPATNOIMAGE, line, pos, ep.getsvalue());
						return ES_ERROR;
					}
					MCeditingimage = nil;

					MCpatternlist->freepat(MCbrushpattern);
					MCbrushpattern = t_new_pattern;
				}
				break;
			case P_DONT_USE_QT:
				return ep.getboolean(MCdontuseQT, line, pos, EE_PROPERTY_NAB);
			case P_PEN_PATTERN:
				{
					if (ep.getuint4(MCpenpmid, line, pos, EE_PROPERTY_PENPATNAN) != ES_NORMAL)
						return ES_ERROR;
					if (MCpenpmid < PI_END)
						MCpenpmid += PI_PATTERNS;
					MCPatternRef t_new_pattern = MCpatternlist->allocpat(MCpenpmid, parent);
					if (t_new_pattern == None)
					{
						MCeerror->add(EE_PROPERTY_PENPATNOIMAGE, line, pos, ep.getsvalue());
						return ES_ERROR;
					}
					MCeditingimage = nil;

					MCpatternlist->freepat(MCpenpattern);
					MCpenpattern = t_new_pattern;
				}
				break;
			case P_BRUSH_BACK_COLOR:
			case P_PEN_BACK_COLOR:
				break;
			case P_BRUSH_COLOR:
			case P_PEN_COLOR:
				{
					MCColor color;
					char *name = NULL;
					char *cstring = ep.getsvalue().clone();
					if (!MCscreen->parsecolor(cstring, &color, &name))
					{
						MCeerror->add
						(EE_PROPERTY_BADCOLOR, line, pos, ep.getsvalue());
						delete cstring;
						return ES_ERROR;
					}
					delete cstring;
					MCscreen->alloccolor(color);

					MCeditingimage = nil;

					if (which == P_BRUSH_COLOR)
					{
						MCpatternlist->freepat(MCbrushpattern);
						MCbrushcolor = color;
						delete MCbrushcolorname;
						MCbrushcolorname = name;
					}
					else
					{
						MCpatternlist->freepat(MCpenpattern);
						MCpencolor = color;
						delete MCpencolorname;
						MCpencolorname = name;
					}
				}
				break;
			case P_TEXT_ALIGN:
			case P_TEXT_FONT:
			case P_TEXT_HEIGHT:
			case P_TEXT_SIZE:
			case P_TEXT_STYLE:
				break;
			case P_EDIT_BACKGROUND:
				return MCdefaultstackptr->setprop(0, which, ep, False);
			case P_ROUND_ENDS:
				return ep.getboolean(MCroundends, line, pos, EE_PROPERTY_NAB);
			case P_DASHES:
				{
					uint1 *newdashes = NULL;
					uint2 newndashes = 0;
					char *svalue = ep.getsvalue().clone();
					char *eptr = svalue;
					uint4 t_dash_len = 0;
					while ((eptr = strtok(eptr, ",")) != NULL)
					{
						int2 i1;
						MCString e = eptr;
						if ((!MCU_stoi2(e, i1)) || i1 < 0)
						{
							MCeerror->add
							(EE_GRAPHIC_NAN, line, pos, ep.getsvalue());
							delete newdashes;
							delete svalue;
							return ES_ERROR;
						}
						t_dash_len += i1;
						MCU_realloc((char **)&newdashes, newndashes,
						            newndashes + 1, sizeof(uint1));
						newdashes[newndashes++] = (uint1)i1;
						eptr = NULL;
					}
					if (newndashes > 0 && t_dash_len == 0)
					{
						delete newdashes;
						newdashes = NULL;
						newndashes = 0;
					}
					delete svalue;
					delete MCdashes;
					MCdashes = newdashes;
					MCndashes = newndashes;
				}
				break;
			case P_FILLED:
				return ep.getboolean(MCfilled, line, pos, EE_PROPERTY_NAB);
			case P_POLY_SIDES:
				if (ep.getuint2(MCpolysides, line, pos,
				                EE_PROPERTY_BADPOLYSIDES) != ES_NORMAL)
					return ES_ERROR;
				MCpolysides = MCU_max(3, MCU_min(MCpolysides,
				                                 MCscreen->getmaxpoints()));
				break;
			case P_LINE_SIZE:
			case P_PEN_WIDTH:
			case P_PEN_HEIGHT:
				if (ep.getuint2(MClinesize, line, pos,
				                EE_PROPERTY_BADLINESIZE) != ES_NORMAL)
					return ES_ERROR;
				break;
			case P_ROUND_RADIUS:
				return ep.getuint2(MCroundradius, line, pos,
				                   EE_PROPERTY_BADROUNDRADIUS);
			case P_START_ANGLE:
				if (ep.getuint2(MCstartangle, line, pos,
				                EE_PROPERTY_BADSTARTANGLE) != ES_NORMAL)
					return ES_ERROR;
				MCstartangle %= 360;
				break;
			case P_ARC_ANGLE:
				if (ep.getuint2(MCarcangle, line, pos,
				                EE_PROPERTY_BADARCANGLE) != ES_NORMAL)
					return ES_ERROR;
				MCarcangle %= 361;
				break;
			case P_NUMBER_FORMAT:
				ep.setnumberformat();
				break;
            // AL-2014-08-12: [[ Bug 13161 ]] Setting templateAudioClip playLoudness shouldn't set the global playLoudness
			case P_PLAY_LOUDNESS:
                {
                    uint2 t_loudness;
                    if (ep . getuint2(t_loudness, line, pos, EE_ACLIP_LOUDNESSNAN) != ES_NORMAL)
                        return ES_ERROR;
                    
                    t_loudness = MCU_max(MCU_min(t_loudness, 100), 0);
                    
                    extern bool MCSystemSetPlayLoudness(uint2 loudness);
#ifdef _MOBILE
                    if (MCSystemSetPlayLoudness(t_loudness))
                        return ES_NORMAL;
#endif
                    if (MCplayers != NULL)
                    {
                        MCPlayer *tptr = MCplayers;
                        while (tptr != NULL)
                        {
                            tptr->setvolume(t_loudness);
                            tptr = tptr->getnextplayer();
                        }
                    }
                    MCS_setplayloudness(t_loudness);
                }
 
                // fall through to set templateAudioClip property
            case P_PLAY_DESTINATION:
                return MCtemplateaudio->setprop(0, which, ep, False);
			case P_LOCK_SCREEN:
				if (ep.getboolean(newlock, line, pos, EE_PROPERTY_NAB) != ES_NORMAL)
					return ES_ERROR;

				// MW-2011-08-18: [[ Redraw ]] Update to use redraw methods.
				if (newlock)
					MCRedrawLockScreen();
				else
					MCRedrawUnlockScreenWithEffects();

				break;
			case P_STACK_FILES: // obsolete
				break;
			case P_MENU_BAR: // can't set menubar (activate stack)
				break;
			case P_EDIT_MENUS:
				break;
			case P_ACCENT_COLOR:
				return MCU_change_color(MCaccentcolor, MCaccentcolorname,
				                        ep, line, pos);
			case P_HILITE_COLOR:
				return MCU_change_color(MChilitecolor, MChilitecolorname,
				                        ep, line, pos);
			case P_PAINT_COMPRESSION:
				if (ep.getsvalue() == "png")
					MCpaintcompression = EX_PNG;
				else if (ep.getsvalue() == "jpeg")
					MCpaintcompression = EX_JPEG;
				else if (ep.getsvalue() == "gif")
					MCpaintcompression = EX_GIF;
				else
					MCpaintcompression = EX_PBM;
				break;
			case P_LINK_COLOR:
				stat = MCU_change_color(MClinkatts.color, MClinkatts.colorname, ep, line, pos);

				// MW-2011-08-17: [[ Redraw ]] Global property could affect anything so dirty screen.
				MCRedrawDirtyScreen();
				return stat;
			case P_LINK_HILITE_COLOR:
				stat = MCU_change_color(MClinkatts.hilitecolor, MClinkatts.hilitecolorname, ep, line, pos);

				// MW-2011-08-17: [[ Redraw ]] Global property could affect anything so dirty screen.
				MCRedrawDirtyScreen();
				return stat;
			case P_LINK_VISITED_COLOR:
				stat = MCU_change_color(MClinkatts.visitedcolor, MClinkatts.visitedcolorname, ep, line, pos);

				// MW-2011-08-17: [[ Redraw ]] Global property could affect anything so dirty screen.
				MCRedrawDirtyScreen();
				return stat;
			case P_UNDERLINE_LINKS:
				stat = ep.getboolean(MClinkatts.underline, line, pos, EE_PROPERTY_NAB);

				// MW-2011-08-17: [[ Redraw ]] Global property could affect anything so dirty screen.
				MCRedrawDirtyScreen();
				return stat;
			case P_SELECT_GROUPED_CONTROLS:
				return ep.getboolean(MCselectgrouped, line, pos, EE_PROPERTY_NAB);
			case P_ICON:
			{
				uint4 t_new_icon_id;
				stat = ep.getuint4(t_new_icon_id, line, pos, EE_STACK_ICONNAN);
				if (stat == ES_NORMAL && t_new_icon_id != MCiconid)
				{
					MCiconid = t_new_icon_id;
					MCscreen -> seticon(MCiconid);
				}
				return stat;
			}
			default:
				break;
			}
			break;
		}
	default:
		if (target == NULL)
		{
			Exec_stat t_stat;
			t_stat = mode_set(ep);
			if (t_stat == ES_NOT_HANDLED)
			{
				MCeerror->add(EE_PROPERTY_CANTSET, line, pos, ep.getsvalue());
				return ES_ERROR;
			}
			return t_stat;
		}

		Properties t_prop;
		MCNameRef t_prop_name, t_index_name;
		t_prop_name = t_index_name = nil;
		if (resolveprop(ep, t_prop, t_prop_name, t_index_name) != ES_NORMAL)
			return ES_ERROR;

		Exec_stat t_stat;
		t_stat = ES_NORMAL;
		if (t_prop == P_CUSTOM)
		{
			MCObject *t_object;
			uint4 t_parid;
			t_stat = target -> getobjforprop(ep, t_object, t_parid);
			
			// MW-2011-09-02: Moved handling of customprop != nil case into resolveprop,
			//   so t_prop_name is always non-nil if t_prop == P_CUSTOM.
			// MW-2011-11-23: [[ Array Chunk Props ]] Moved handling of arrayprops into
			//   MCChunk::setprop.
			if (t_stat == ES_NORMAL)
			{
				if (t_index_name == nil)
					t_stat = t_object -> setcustomprop(ep, t_object -> getdefaultpropsetname(), t_prop_name);
				else
					t_stat = t_object -> setcustomprop(ep, t_prop_name, t_index_name);
				// MM-2012-09-05: [[ Property Listener ]] Make sure setting a custom property sends propertyChanged message to listeners.
				if (t_stat == ES_NORMAL)
					t_object -> signallisteners(t_prop);
			}
		}
		else
		{
			// MW-2011-11-23: [[ Array Chunk Props ]] If the prop is an array-prop, then
			//   a nil index translates to the empty name (the array[empty] <=> the array).
			MCNameRef t_derived_index_name;
			if (t_prop < P_FIRST_ARRAY_PROP)
				t_derived_index_name = nil;
			else
				t_derived_index_name = t_index_name != nil ? t_index_name : kMCEmptyName;

			t_stat = target -> setprop(t_prop, ep, t_derived_index_name, effective);
		}

		MCNameDelete(t_index_name);
		MCNameDelete(t_prop_name);

		if (t_stat != ES_NORMAL)
		{
			MCeerror->add(EE_PROPERTY_CANTSETOBJECT, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
	}
	return ES_NORMAL;
#endif /* MCProperty::set */

#ifdef LEGACY_EXEC
Exec_stat MCProperty::set_variable(MCExecPoint& ep)
{
	return destvar->set(ep);
}

Exec_stat MCProperty::set_global_property(MCExecPoint& ep)
{
	const MCPropertyInfo *t_info;
	if (MCPropertyInfoTableLookup(which, effective, t_info))
	{
		MCExecContext ctxt(ep);
        MCAutoValueRef t_value;
        /* UNCHECKED */ ep . copyasvalueref(&t_value);
        
        if (t_info -> custom_index)
        {
            MCNewAutoNameRef t_type;
            
            if (customindex != nil)
            {
                if (customindex -> eval(ep) != ES_NORMAL)
                {
                    MCeerror -> add(EE_PROPERTY_BADEXPRESSION, line, pos);
                    return ES_ERROR;
                }
                ep . copyasnameref(&t_type);
            }
            MCExecStoreProperty(ctxt, t_info, *t_type, *t_value);
        }
        else
            MCExecStoreProperty(ctxt, t_info, nil, *t_value);
        
        if (!ctxt . HasError())
            return ES_NORMAL;
        
        return ctxt . Catch(line, pos);
	}

	Exec_stat t_stat;
	t_stat = mode_set(ep);
	if (t_stat != ES_NORMAL)
	{
		MCeerror->add(EE_PROPERTY_CANTSET, line, pos, ep.getsvalue());
		return ES_ERROR;
	}

	return ES_NORMAL;
}

Exec_stat MCProperty::set_object_property(MCExecPoint& ep)
{
	Properties t_prop;
	MCNameRef t_prop_name, t_index_name;
	t_prop_name = t_index_name = nil;
	if (resolveprop(ep, t_prop, t_prop_name, t_index_name) != ES_NORMAL)
		return ES_ERROR;
	
	Exec_stat t_stat;
	t_stat = ES_NORMAL;
	if (t_prop == P_CUSTOM)
	{
		MCObject *t_object;
		uint4 t_parid;
		t_stat = target -> getobjforprop(ep, t_object, t_parid);
		
		// MW-2011-09-02: Moved handling of customprop != nil case into resolveprop,
		//   so t_prop_name is always non-nil if t_prop == P_CUSTOM.
		// MW-2011-11-23: [[ Array Chunk Props ]] Moved handling of arrayprops into
		//   MCChunk::setprop.
		if (t_stat == ES_NORMAL)
		{
			if (t_index_name == nil)
				t_stat = t_object -> setcustomprop(ep, t_object -> getdefaultpropsetname(), t_prop_name);
			else
				t_stat = t_object -> setcustomprop(ep, t_prop_name, t_index_name);
			// MM-2012-09-05: [[ Property Listener ]] Make sure setting a custom property sends propertyChanged message to listeners.
			if (t_stat == ES_NORMAL)
				t_object -> signallisteners(t_prop);
		}
	}
	else
	{
		// MW-2011-11-23: [[ Array Chunk Props ]] If the prop is an array-prop, then
		//   a nil index translates to the empty name (the array[empty] <=> the array).
		MCNameRef t_derived_index_name;
		if (t_prop < P_FIRST_ARRAY_PROP)
			t_derived_index_name = nil;
		else
			t_derived_index_name = t_index_name != nil ? t_index_name : kMCEmptyName;
		
		t_stat = target -> setprop(t_prop, ep, t_derived_index_name, effective);
	}
	
	MCNameDelete(t_index_name);
	MCNameDelete(t_prop_name);
	
	if (t_stat != ES_NORMAL)
	{
		MCeerror->add(EE_PROPERTY_CANTSETOBJECT, line, pos, ep . getsvalue());
		return ES_ERROR;
	}

	return t_stat;
}
#endif
////////////////////////////////////////////////////////////////////////////////

#ifdef /* MCProperty::eval */ LEGACY_EXEC
	uint2 i = 0;
	int2 mx, my;
	MCExecPoint ep2(ep);
    
	if (destvar != NULL && which != P_CUSTOM_VAR)
		return destvar->eval(ep);
	if (function != F_UNDEFINED)
	{
		switch (function)
		{
            case F_DATE:
                MCD_date(which, ep);
                return ES_NORMAL;
            case F_TIME:
                MCD_time(which, ep);
                return ES_NORMAL;
            case F_MILLISECS:
                if (which == P_LONG)
                    ep.setnvalue(MCS_time() * 1000.0);
                else
                    ep.setnvalue(floor(MCS_time() * 1000.0));
                return ES_NORMAL;
            case F_SECONDS:
                if (which == P_LONG)
                    ep.setnvalue(MCS_time());
                else
                    ep.setnvalue(floor(MCS_time()));
                return ES_NORMAL;
            case F_TICKS:
                if (which == P_LONG)
                    ep.setnvalue(MCS_time() * 60.0);
                else
                    ep.setnvalue(floor(MCS_time() * 60.0));
                return ES_NORMAL;
            case F_FILES:
            case F_DIRECTORIES:
                if (MCsecuremode & MC_SECUREMODE_DISK)
                {
                    MCeerror->add(EE_DISK_NOPERM, line, pos);
                    return ES_ERROR;
                }
                MCS_getentries(ep, function == F_FILES, which == P_LONG);
                return ES_NORMAL;
            case F_MONTH_NAMES:
                MCD_monthnames(which, ep);
                return ES_NORMAL;
            case F_WEEK_DAY_NAMES:
                MCD_weekdaynames(which, ep);
                return ES_NORMAL;
            case F_DATE_FORMAT:
                MCD_dateformat(which, ep);
                return ES_NORMAL;
            case F_SCREEN_RECT:
                MCScreenRect::evaluate(ep, which >= 1000, (which % 1000) == P_LONG, effective == True);
                return ES_NORMAL;
            default:
                fprintf(stderr, "MCProperty: ERROR bad function in eval\n");
                return ES_ERROR;
		}
	}
	switch (which)
	{
        case P_CASE_SENSITIVE:
            ep.setboolean(ep.getcasesensitive());
            break;
        case P_CENTURY_CUTOFF:
            ep.setnvalue(ep.getcutoff());
            break;
        case P_CONVERT_OCTALS:
            ep.setboolean(ep.getconvertoctals());
            break;
        case P_ITEM_DELIMITER:
		{
			char id = ep.getitemdel();
			ep.copysvalue(&id, 1);
		}
            break;
        case P_COLUMN_DELIMITER:
		{
			char id = ep.getcolumndel();
			ep.copysvalue(&id, 1);
		}
            break;
        case P_ROW_DELIMITER:
		{
			char id = ep.getrowdel();
			ep.copysvalue(&id, 1);
		}
            break;
        case P_LINE_DELIMITER:
		{
			char id = ep.getlinedel();
			ep.copysvalue(&id, 1);
		}
            break;
        case P_WHOLE_MATCHES:
            ep.setboolean(ep.getwholematches());
            break;
        case P_USE_SYSTEM_DATE:
            ep.setboolean(ep.getusesystemdate());
            break;
        case P_USE_UNICODE:
            ep.setboolean(ep.getuseunicode());
            break;
            
        case P_PRINTER_NAMES:
            if (!MCSecureModeCheckPrinter())
                return ES_ERROR;
            MCscreen -> listprinters(ep);
            break;
            
        case P_PRINT_DEVICE_NAME:
            ep . setsvalue(MCprinter -> GetDeviceName());
            break;
            
        case P_PRINT_DEVICE_SETTINGS:
        {
            MCString t_settings;
            t_settings = MCprinter -> CopyDeviceSettings();
            ep . copysvalue(t_settings . getstring(), t_settings . getlength());
            delete (char *)t_settings . getstring();
        }
            break;
            
        case P_PRINT_DEVICE_OUTPUT:
        {
            switch(MCprinter -> GetDeviceOutputType())
            {
                case PRINTER_OUTPUT_DEVICE:
                    ep . setstaticcstring("device");
                    break;
                    
                case PRINTER_OUTPUT_PREVIEW:
                    ep . setstaticcstring("preview");
                    break;
                    
                case PRINTER_OUTPUT_FILE:
                    ep . setstringf("file:%s", MCprinter -> GetDeviceOutputLocation());
                    break;
                    
                case PRINTER_OUTPUT_SYSTEM:
                    ep . setstaticcstring("system");
                    break;
            }
        }
            break;
            
        case P_PRINT_DEVICE_FEATURES:
        {
            MCPrinterFeatureSet t_features;
            t_features = MCprinter -> GetDeviceFeatures();
            
            ep . clear();
            
            int4 t_count;
            t_count = 0;
            if ((t_features & PRINTER_FEATURE_COLLATE) != 0)
                ep . concatcstring("collate", EC_COMMA, t_count++ == 0);
            if ((t_features & PRINTER_FEATURE_COPIES) != 0)
                ep . concatcstring("copies", EC_COMMA, t_count++ == 0);
            if ((t_features & PRINTER_FEATURE_COLOR) != 0)
                ep . concatcstring("color", EC_COMMA, t_count++ == 0);
            if ((t_features & PRINTER_FEATURE_DUPLEX) != 0)
                ep . concatcstring("duplex", EC_COMMA, t_count++ == 0);
        }
            break;
            
        case P_PRINT_DEVICE_RECTANGLE:
            ep.setrectangle(MCprinter -> GetDeviceRectangle());
            break;
            
        case P_PRINT_PAGE_SIZE:
            ep.setpoint(MCprinter -> GetPageWidth(), MCprinter -> GetPageHeight());
            break;
            
        case P_PRINT_PAGE_RECTANGLE:
            ep.setrectangle(MCprinter -> GetPageRectangle());
            break;
            
        case P_PRINT_PAGE_ORIENTATION:
        {
            switch(MCprinter -> GetPageOrientation())
            {
                case PRINTER_ORIENTATION_PORTRAIT:
                    ep . setstaticcstring("portrait");
                    break;
                    
                case PRINTER_ORIENTATION_REVERSE_PORTRAIT:
                    ep . setstaticcstring("reverse portrait");
                    break;
                    
                case PRINTER_ORIENTATION_LANDSCAPE:
                    ep . setstaticcstring("landscape");
                    break;
                    
                case PRINTER_ORIENTATION_REVERSE_LANDSCAPE:
                    ep . setstaticcstring("reverse landscape");
                    break;
            }
        }
            break;
            
        case P_PRINT_ROTATED:
            switch(MCprinter -> GetPageOrientation())
		{
			case PRINTER_ORIENTATION_PORTRAIT:
			case PRINTER_ORIENTATION_REVERSE_PORTRAIT:
				ep . setboolean(false);
                break;
                
			case PRINTER_ORIENTATION_LANDSCAPE:
			case PRINTER_ORIENTATION_REVERSE_LANDSCAPE:
				ep . setboolean(true);
                break;
		}
            break;
            
        case P_PRINT_PAGE_SCALE:
            ep . setnvalue(MCprinter -> GetPageScale());
            break;
            
        case P_PRINT_JOB_NAME:
            ep . setsvalue(MCprinter -> GetJobName());
            break;
        case P_PRINT_JOB_COPIES:
            ep . setint(MCprinter -> GetJobCopies());
            break;
        case P_PRINT_JOB_COLLATE:
            ep . setboolean(MCprinter -> GetJobCollate());
            break;
        case P_PRINT_JOB_DUPLEX:
            switch(MCprinter -> GetJobDuplex())
		{
			case PRINTER_DUPLEX_MODE_SIMPLEX:
				ep . setstaticcstring("none");
                break;
                
			case PRINTER_DUPLEX_MODE_LONG_EDGE:
				ep . setstaticcstring("long edge");
                break;
                
			case PRINTER_DUPLEX_MODE_SHORT_EDGE:
				ep . setstaticcstring("short edge");
                break;
		}
            break;
        case P_PRINT_JOB_COLOR:
            ep . setboolean(MCprinter -> GetJobColor());
            break;
        case P_PRINT_JOB_RANGES:
        {
            MCPrinterPageRangeCount t_count;
            t_count = MCprinter -> GetJobRangeCount();
            if (t_count == PRINTER_PAGE_RANGE_CURRENT)
                ep . setstaticcstring("current");
            else if (t_count == PRINTER_PAGE_RANGE_SELECTION)
                ep . setstaticcstring("selection");
            else if (t_count == PRINTER_PAGE_RANGE_ALL)
                ep . setstaticcstring("all");
            else
            {
                const MCRange *t_ranges;
                t_ranges = MCprinter -> GetJobRanges();
                
                ep . clear();
                for(int i = 0; i < t_count; ++i)
                {
                    char t_buffer[I4L * 2 + 2];
                    if (t_ranges[i] . from == t_ranges[i] . to)
                        sprintf(t_buffer, "%d", t_ranges[i] . from);
                    else
                        sprintf(t_buffer, "%d-%d", t_ranges[i] . from, t_ranges[i] . to);
                    ep . concatcstring(t_buffer, EC_COMMA, i == 0);
                }
            }
        }
            break;
        case P_PRINT_JOB_PAGE:
            if (MCprinter -> GetJobPageNumber() == -1)
                ep . clear();
            else
                ep . setint(MCprinter -> GetJobPageNumber());
            break;
            
        case P_PRINT_CARD_BORDERS:
            ep . setboolean(MCprinter -> GetLayoutShowBorders());
            break;
            
        case P_PRINT_COMMAND:
            if (MCprinter -> GetDeviceCommand() == NULL)
                ep . clear();
            else
                ep . copysvalue(MCprinter -> GetDeviceCommand());
            break;
            
        case P_PRINT_FONT_TABLE:
            // OK-2008-11-25: [[Bug 7475]] Null check needed to avoid crash
            const char *t_device_font_table;
            t_device_font_table = MCprinter -> GetDeviceFontTable();
            if (t_device_font_table == NULL)
                ep . clear();
            else
                ep . copysvalue(t_device_font_table);
            
            break;
            
        case P_PRINT_GUTTERS:
            ep.setpoint(MCprinter -> GetLayoutRowSpacing(), MCprinter -> GetLayoutColumnSpacing());
            break;
            
        case P_PRINT_MARGINS:
            ep.setrectangle(MCprinter -> GetPageLeftMargin(), MCprinter -> GetPageTopMargin(), MCprinter -> GetPageRightMargin(), MCprinter -> GetPageBottomMargin());
            break;
            
        case P_PRINT_ROWS_FIRST:
            ep . setboolean(MCprinter -> GetLayoutRowsFirst());
            break;
            
            // MW-2007-09-10: [[ Bug 5343 ]] Without this we get a crash :oD
        case P_PRINT_SCALE:
            ep . setnvalue(MCprinter -> GetLayoutScale());
            break;
            
        case P_PRINT_TEXT_ALIGN:
        case P_PRINT_TEXT_FONT:
        case P_PRINT_TEXT_HEIGHT:
        case P_PRINT_TEXT_SIZE:
        case P_PRINT_TEXT_STYLE:
            break;
            
        case P_CLIPBOARD_DATA:
        case P_DRAG_DATA:
        {
            bool t_query_success;
            t_query_success = true;
            
            MCTransferData *t_pasteboard;
            if (which == P_CLIPBOARD_DATA)
                t_pasteboard = MCclipboarddata;
            else
                t_pasteboard = MCdragdata;
            
            if (t_pasteboard -> Lock())
            {
                MCTransferType t_type;
                if (customindex == NULL)
                    t_type = TRANSFER_TYPE_TEXT;
                else
                {
                    if (customindex->eval(ep) != ES_NORMAL)
                    {
                        MCeerror->add(EE_PROPERTY_BADEXPRESSION, line, pos);
                        return ES_ERROR;
                    }
                    t_type = MCTransferData::StringToType(ep . getsvalue());
                }
                
                // MW-2014-03-12: [[ ClipboardStyledText ]] If styledText is being requested, then
                //   convert the styles data to an array and return that.
                if (t_type == TRANSFER_TYPE_STYLED_TEXT_ARRAY &&
                    t_pasteboard -> Contains(TRANSFER_TYPE_STYLED_TEXT, true))
                {
                    MCSharedString *t_data;
                    t_data = t_pasteboard -> Fetch(TRANSFER_TYPE_STYLED_TEXT);
                    if (t_data != NULL)
                    {
                        ep . setarray(MCConvertStyledTextToStyledTextArray(t_data), True);
                        t_data -> Release();
                        t_query_success = true;
                    }
                }
                else if (t_type != TRANSFER_TYPE_NULL && t_pasteboard -> Contains(t_type, true))
                {
                    MCSharedString *t_data;
                    t_data = t_pasteboard -> Fetch(t_type);
                    if (t_data != NULL)
                    {
                        ep . copysvalue(t_data -> Get() . getstring(), t_data -> Get() . getlength());
                        t_data -> Release();
                    }
                    else
                        t_query_success = false;
                }
                else
                {
                    ep . clear();
                    MCresult -> sets("format not available");
                }
                
                t_pasteboard -> Unlock();
            }
            else
                t_query_success = false;
            
            if (!t_query_success)
            {
                ep . clear();
                MCresult -> sets("unable to query clipboard");
            }
        }
            break;
        case P_DIALOG_DATA:
            MCdialogdata->fetch(ep);
            break;
        case P_HC_IMPORT_STAT:
            ep.setsvalue(MChcstat);
            break;
        case P_SCRIPT_TEXT_FONT:
            ep.setsvalue(MCscriptfont);
            break;
        case P_SCRIPT_TEXT_SIZE:
            if (MCscriptsize == 0)
                ep.clear();
            else
                ep.setnvalue(MCscriptsize);
            break;
        case P_LOOK_AND_FEEL:
            if (MCcurtheme)
                ep.setsvalue(MCcurtheme->getname());
            else
            {
                switch(MClook)
                {
                    case LF_MAC:
                        ep.setstaticcstring(MClnfmacstring);
                        break;
                    case LF_WIN95:
                        ep.setstaticcstring(MClnfwinstring);
                        break;
                    default:
                        ep.setstaticcstring(MClnfmotifstring);
                        break;
                }
            }
            break;
        case P_SCREEN_MOUSE_LOC:
            MCscreen->querymouse(mx, my);
            ep.setpoint(mx, my);
            break;
        case P_UMASK:
            ep.setint(MCS_getumask());
            break;
        case P_BUFFER_MODE:
            switch(MCscreen->getdepth())
		{
            case 1:
                ep.setstaticcstring("bw");
                break;
            case 2:
                ep.setuint(4);
                break;
            case 4:
                ep.setuint(16);
                break;
            case 8:
                ep.setuint(256);
                break;
            case 16:
                ep.setstaticcstring("thousands");
                break;
            default:
                ep.setstaticcstring("millions");
                break;
		}
            break;
        case P_BUFFER_IMAGES:
            ep.setboolean(MCbufferimages);
            break;
        case P_BACK_DROP:
            if (MCbackdropcolor == NULL)
                ep.setstaticcstring("none");
            else
                ep.setsvalue(MCbackdropcolor);
            break;
        case P_MULTI_EFFECT:
            ep.setboolean(false);
            break;
        case P_ALLOW_INTERRUPTS:
            ep.setboolean(MCallowinterrupts);
            break;
        case P_EXPLICIT_VARIABLES:
            ep.setboolean(MCexplicitvariables);
            break;
        case P_PRESERVE_VARIABLES:
            ep.setboolean(MCpreservevariables);
            break;
        case P_SYSTEM_FS:
            ep.setboolean(MCsystemFS);
            break;
        case P_SYSTEM_CS:
            ep.setboolean(MCsystemCS);
            break;
        case P_SYSTEM_PS:
            ep.setboolean(MCsystemPS);
            break;
        case P_FILE_TYPE:
            ep.setsvalue(MCfiletype);
            break;
        case P_STACK_FILE_TYPE:
            ep.setsvalue(MCstackfiletype);
            break;
        case P_STACK_FILE_VERSION:
        {
            char t_version[6];
            t_version[0] = '0' + MCstackfileversion / 1000;
            t_version[1] = '.';
            t_version[2] = '0' + (MCstackfileversion % 1000) / 100;
            if (MCstackfileversion % 100 == 0)
                t_version[3] = '\0';
            else
            {
                t_version[3] = '.';
                t_version[4] = '0' + (MCstackfileversion % 100) / 10;
                t_version[5] = '\0';
            }
            ep . copysvalue(t_version, strlen(t_version));
        }
            break;
        case P_SECURE_MODE:
            ep.setboolean(MCsecuremode == MC_SECUREMODE_ALL);
            break;
        case P_SECURITY_CATEGORIES:
        case P_SECURITY_PERMISSIONS:
        {
            int t_count;
            t_count = 0;
            
            ep . clear();
            for (int i=0; i<MC_SECUREMODE_MODECOUNT; i++)
            {
                if ((which == P_SECURITY_CATEGORIES) || ((MCsecuremode & (1 << i)) == 0))
                {
                    ep.concatcstring(MCsecuremode_strings[i], EC_COMMA, t_count == 0);
                    t_count += 1;
                }
            }
        }
            break;
        case P_SERIAL_CONTROL_STRING:
            ep.setsvalue(MCserialcontrolsettings);
            break;
        case P_COLOR_WORLD:
            ep.setboolean(MCscreen->getdepth() > 1);
            break;
        case P_ALLOW_KEY_IN_FIELD:
        case P_REMAP_COLOR:
        case P_EDIT_SCRIPTS:
            ep.setboolean(True);
            break;
        case P_ALLOW_FIELD_REDRAW:
            ep.setboolean(False);
            break;
        case P_HIDE_CONSOLE_WINDOWS:
            ep.setboolean(MChidewindows);
            break;
        case P_FTP_PROXY:
            if (MCftpproxyhost == NULL)
                ep.clear();
            else
                ep.setstringf("%s:%d", MCftpproxyhost, MCftpproxyport);
            break;
        case P_HTTP_HEADERS:
            if (MChttpheaders == NULL)
                ep.clear();
            else
                ep.setsvalue(MChttpheaders);
            break;
        case P_HTTP_PROXY:
            if (MChttpproxy == NULL)
                ep . clear();
            else
                ep . copysvalue(MChttpproxy);
            break;
        case P_SHOW_INVISIBLES:
            ep.setboolean(MCshowinvisibles);
            break;
        case P_SOCKET_TIMEOUT:
            ep.setnvalue(MCsockettimeout * 1000.0);
            break;
        case P_RANDOM_SEED:
            ep.setnvalue(MCrandomseed);
            break;
        case P_DEFAULT_STACK:
            MCdefaultstackptr->getprop(0, P_NAME, ep, effective);
            break;
        case P_DEFAULT_MENU_BAR:
            if (MCdefaultmenubar == NULL)
                ep.clear();
            else
                MCdefaultmenubar->getprop(0, P_LONG_NAME, ep, effective);
            break;
        case P_DRAG_SPEED:
            ep.setint(MCdragspeed);
            break;
        case P_MOVE_SPEED:
            ep.setint(MCmovespeed);
            break;
        case P_LOCK_COLORMAP:
            ep.setboolean(MClockcolormap);
            break;
        case P_LOCK_CURSOR:
            ep.setboolean(MClockcursor);
            break;
        case P_LOCK_ERRORS:
            ep.setboolean(MClockerrors);
            break;
        case P_LOCK_MENUS:
            ep.setboolean(MClockmenus);
            break;
        case P_LOCK_MESSAGES:
            ep.setboolean(MClockmessages);
            break;
        case P_LOCK_MOVES:
            ep.setboolean(MCscreen->getlockmoves());
            break;
        case P_LOCK_RECENT:
            ep.setboolean(MClockrecent);
            break;
        case P_USER_LEVEL:
            ep.setint(MCuserlevel);
            break;
        case P_USER_MODIFY:
            ep.setboolean(True);
            break;
        case P_CURSOR:
            if (MCcursor != None)
                ep.setint(MCcursorid);
            else
                ep.clear();
            break;
        case P_DEFAULT_CURSOR:
            if (MCdefaultcursor != None)
                ep.setint(MCdefaultcursorid);
            else
                ep.clear();
            break;
        case P_TRACE_ABORT:
            ep.setboolean(MCtraceabort);
            break;
        case P_TRACE_DELAY:
            ep.setint(MCtracedelay);
            break;
        case P_TRACE_RETURN:
            ep.setboolean(MCtracereturn);
            break;
        case P_TRACE_STACK:
            if (MCtracestackptr == NULL)
                ep.clear();
            else
                MCtracestackptr->getprop(0, P_NAME, ep, effective);
            break;
        case P_TRACE_UNTIL:
            ep.setuint(MCtraceuntil);
            break;
        case P_DIRECTORY:
		{
			if (!MCSecureModeCheckDisk(line, pos))
				return ES_ERROR;
            
			char *dir = MCS_getcurdir();
			ep.copysvalue(dir, strlen(dir));
			delete dir;
		}
            break;
        case P_SSL_CERTIFICATES:
            ep.setsvalue(MCsslcertificates);
            break;
        case P_DEFAULT_NETWORK_INTERFACE:
            ep.setsvalue(MCdefaultnetworkinterface);
            break;
        case P_NETWORK_INTERFACES:
            MCS_getnetworkinterfaces(ep);
            break;
        case P_ERROR_MODE:
        {
            MCSErrorMode t_mode;
            t_mode = MCS_get_errormode();
            switch (t_mode)
            {
                case kMCSErrorModeNone:
                    ep.clear();
                    break;
                case kMCSErrorModeQuiet:
                    ep.setstaticcstring("quiet");
                    break;
                case kMCSErrorModeStderr:
                    ep.setstaticcstring("stderr");
                    break;
                case kMCSErrorModeInline:
                    ep.setstaticcstring("inline");
                    break;
                case kMCSErrorModeDebugger:
                    ep.setstaticcstring("debugger");
                    break;
                default:
                    break;
            }
        }
            break;
        case P_OUTPUT_TEXT_ENCODING:
        {
            const char *t_encoding;
            switch(MCS_get_outputtextencoding())
            {
                case kMCSOutputTextEncodingWindows1252:
                    t_encoding = "windows-1252";
                    break;
                case kMCSOutputTextEncodingMacRoman:
                    t_encoding = "macintosh";
                    break;
                case kMCSOutputTextEncodingISO8859_1:
                    t_encoding = "iso-8859-1";
                    break;
                case kMCSOutputTextEncodingUTF8:
                    t_encoding = "utf-8";
                    break;
                default:
                    t_encoding = "";
                    break;
            }
            ep.setstaticcstring(t_encoding);
        }
            break;
        case P_OUTPUT_LINE_ENDINGS:
        {
            const char *t_ending;
            switch(MCS_get_outputlineendings())
            {
                case kMCSOutputLineEndingsLF:
                    t_ending = "lf";
                    break;
                case kMCSOutputLineEndingsCR:
                    t_ending = "cr";
                    break;
                case kMCSOutputLineEndingsCRLF:
                    t_ending = "crlf";
                    break;
                default:
                    t_ending = "";
                    break;
            }
            ep . setstaticcstring(t_ending);
        }
            break;
        case P_SESSION_SAVE_PATH:
            ep.setcstring(MCS_get_session_save_path());
            break;
        case P_SESSION_LIFETIME:
            ep.setuint(MCS_get_session_lifetime());
            break;
        case P_SESSION_COOKIE_NAME:
            ep.setcstring(MCS_get_session_name());
            break;
        case P_SESSION_ID:
            ep.setcstring(MCS_get_session_id());
            break;
            
        case P_SCRIPT_EXECUTION_ERRORS:
            ep . setstaticcstring(MCexecutionerrors);
            break;
        case P_SCRIPT_PARSING_ERRORS:
            ep . setstaticcstring(MCparsingerrors);
            break;
        case P_REV_RUNTIME_BEHAVIOUR:
            ep.setint(MCruntimebehaviour);
            break;
        case P_PRIVATE_COLORS:
            ep.setboolean(MCuseprivatecmap);
            break;
        case P_TWELVE_TIME:
            ep.setboolean(MCtwelvetime);
            break;
        case P_IDLE_RATE:
            ep.setint(MCidleRate);
            break;
        case P_QT_IDLE_RATE:
            ep.setint(MCqtidlerate);
            break;
        case P_IDLE_TICKS:
            ep.setint(MCidleRate * 60 / 1000);
            break;
        case P_BLINK_RATE:
            ep.setint(MCblinkrate);
            break;
        case P_RECURSION_LIMIT:
            ep.setuint(MCrecursionlimit);
            break;
        case P_REPEAT_RATE:
            ep.setint(MCrepeatrate);
            break;
        case P_REPEAT_DELAY:
            ep.setint(MCrepeatdelay);
            break;
        case P_TYPE_RATE:
            ep.setint(MCtyperate);
            break;
        case P_SYNC_RATE:
            ep.setint(MCsyncrate);
            break;
        case P_EFFECT_RATE:
            ep.setint(MCeffectrate);
            break;
        case P_DOUBLE_DELTA:
            ep.setint(MCdoubledelta);
            break;
        case P_DRAG_DELTA:
            ep.setint(MCdragdelta);
            break;
        case P_DOUBLE_TIME:
            ep.setint(MCdoubletime);
            break;
        case P_TOOL_TIP_DELAY:
            ep.setint(MCtooltipdelay);
            break;
        case P_LONG_WINDOW_TITLES:
            ep.setboolean(MClongwindowtitles);
            break;
        case P_BLIND_TYPING:
            ep.setboolean(MCblindtyping);
            break;
        case P_POWER_KEYS:
            ep.setboolean(MCpowerkeys);
            break;
        case P_NAVIGATION_ARROWS:
            ep.setboolean(MCnavigationarrows);
            break;
        case P_TEXT_ARROWS:
            ep.setboolean(MCtextarrows);
            break;
        case P_EXTEND_KEY:
            ep.setint(MCextendkey);
            break;
        case P_COLORMAP:
            MCscreen->getcolors(ep);
            break;
        case P_NO_PIXMAPS:
            ep.setboolean(MCnopixmaps);
            break;
        case P_LOW_RESOLUTION_TIMERS:
            ep.setboolean(MClowrestimers);
            break;
        case P_POINTER_FOCUS:
            ep.setboolean(MCpointerfocus);
            break;
        case P_EMACS_KEY_BINDINGS:
            ep.setboolean(MCemacskeys);
            break;
        case P_RAISE_MENUS:
            ep.setboolean(MCraisemenus);
            break;
        case P_ACTIVATE_PALETTES:
            ep.setboolean(MCactivatepalettes);
            break;
        case P_HIDE_PALETTES:
            ep.setboolean(MChidepalettes);
            break;
        case P_RAISE_PALETTES:
            ep.setboolean(MCraisepalettes);
            break;
        case P_RAISE_WINDOWS:
            ep.setboolean(MCraisewindows);
            break;
        case P_DONT_USE_NS:
            ep.setboolean(MCdontuseNS);
            break;
        case P_HIDE_BACKDROP:
            ep.setboolean(MChidebackdrop);
            break;
        case P_DONT_USE_QT:
            ep.setboolean(MCdontuseQT);
            break;
        case P_DONT_USE_QT_EFFECTS:
            ep.setboolean(MCdontuseQTeffects);
            break;
        case P_PROPORTIONAL_THUMBS:
            ep.setboolean(MCproportionalthumbs);
            break;
        case P_SHARED_MEMORY:
            ep.setboolean(MCshm);
            break;
        case P_VC_SHARED_MEMORY:
            ep.setboolean(MCvcshm);
            break;
        case P_VC_PLAYER:
            ep.setsvalue(MCvcplayer);
            break;
        case P_SCREEN_GAMMA:
            ep.setr8(MCgamma, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
            break;
            
            // IM-2013-12-04: [[ PixelScale ]] Global property pixelScale returns the current pixel scale
        case P_PIXEL_SCALE:
            ep.setnvalue(MCResGetPixelScale());
            break;
            // IM-2013-12-04: [[ PixelScale ]] Global property systemPixelScale returns the pixel scale as determined by the OS
        case P_SYSTEM_PIXEL_SCALE:
            // IM-2014-01-24: [[ HiDPI ]] systemPixelScale now returns the maximum scale on all displays
            MCGFloat t_scale;
            t_scale = 1.0;
            /* UNCHECKED */ MCscreen->getmaxdisplayscale(t_scale);
            ep.setnvalue(t_scale);
            break;
            
            // IM-2014-01-24: [[ HiDPI ]] Global property usePixelScaling returns its configured value (default: true)
        case P_USE_PIXEL_SCALING:
            ep.setboolean(MCResGetUsePixelScaling());
            break;
            
            // IM-2014-01-27: [[ HiDPI ]] Global property screenPixelScale returns the pixel scale of the main screen
        case P_SCREEN_PIXEL_SCALE:
            // IM-2014-01-27: [[ HiDPI ]] Global property screenPixelScales returns a return-delimited
            // list of the pixel scales of all connected screens
        case P_SCREEN_PIXEL_SCALES:
        {
            MCResListScreenPixelScales(ep, which == P_SCREEN_PIXEL_SCALES);
            break;
        }
            
            // MW-2014-08-12: [[ EditionType ]] Return whether the engine is community or commercial.
        case P_EDITION_TYPE:
            ep . setstaticcstring(MClicenseparameters . license_class == kMCLicenseClassCommunity ? "community" : "commercial");
            break;
            
        case P_SHELL_COMMAND:
            ep.setsvalue(MCshellcmd);
            break;
        case P_SOUND_CHANNEL:
            ep.setint(MCsoundchannel);
            break;
        case P_ADDRESS:
            ep.setsvalue(MCS_getaddress());
            break;
        case P_STACKS_IN_USE:
            ep.clear();
            i = MCnusing;
            while (i--)
            {
                MCusing[i]->getprop(0, P_SHORT_NAME, ep2, effective);
                ep.concatmcstring(ep2.getsvalue(), EC_RETURN, i == MCnusing - 1);
            }
            break;
            
            // TD-2013-06-20: [[ DynamicFonts ]] global property for list of font files
        case P_FONTFILES_IN_USE:
            // MERG-2013-08-14: [[ DynamicFonts ]] Refactored to use MCFontListLoaded
            return MCFontListLoaded(ep);
            break;
        case P_RELAYER_GROUPED_CONTROLS:
            ep.setboolean(MCrelayergrouped);
            break;
        case P_SELECTION_MODE:
            if (MCselectintersect)
                ep.setstaticcstring(MCintersectstring);
            else
                ep.setstaticcstring(MCsurroundstring);
            break;
        case P_SELECTION_HANDLE_COLOR:
            ep.setcolor(MCselectioncolor, MCselectioncolorname);
            break;
        case P_WINDOW_BOUNDING_RECT:
            ep.setrectangle(MCwbr);
            break;
        case P_JPEG_QUALITY:
            ep.setint(MCjpegquality);
            break;
        case P_RECORDING:
            ep.setboolean(MCrecording);
            break;
        case P_BRUSH:
            if (MCbrush < PI_PATTERNS)
                ep.setint(MCbrush - PI_BRUSHES);
            else
                ep.setint(MCbrush);
            break;
        case P_CENTERED:
            ep.setboolean(MCcentered);
            break;
        case P_ERASER:
            if (MCeraser < PI_PATTERNS)
                ep.setint(MCeraser - PI_BRUSHES);
            else
                ep.setint(MCeraser);
            break;
        case P_GRID:
            ep.setboolean(MCgrid);
            break;
        case P_GRID_SIZE:
            ep.setint(MCgridsize);
            break;
        case P_MULTIPLE:
            ep.setboolean(MCmultiple);
            break;
        case P_MULTI_SPACE:
            ep.setint(MCmultispace);
            break;
        case P_SLICES:
            ep.setint(MCslices);
            break;
        case P_SPRAY:
            if (MCspray < PI_PATTERNS)
                ep.setint(MCspray - PI_BRUSHES);
            else
                ep.setint(MCspray);
            break;
        case P_BEEP_LOUDNESS:
        case P_BEEP_PITCH:
        case P_BEEP_DURATION:
            MCscreen->getbeep(which, ep);
            break;
        case P_BEEP_SOUND:
            ep.setsvalue(MCscreen->getbeepsound());
            break;
        case P_TOOL:
            ep.setstringf("%s tool", MCtoolnames[MCcurtool]);
            break;
        case P_LZW_KEY:
            ep.clear();
            break;
        case P_RECORD_FORMAT:
            switch (MCrecordformat)
		{
            case EX_AIFF:
                ep.setstaticcstring("aiff");
                break;
            case EX_WAVE:
                ep.setstaticcstring("wave");
                break;
            case EX_ULAW:
                ep.setstaticcstring("ulaw");
                break;
            default:
                ep.setstaticcstring("movie");
                break;
		}
            break;
        case P_RECORD_COMPRESSION:
            ep.copysvalue(MCrecordcompression, 4);
            break;
        case P_RECORD_INPUT:
            ep.copysvalue(MCrecordinput, 4);
            break;
        case P_RECORD_CHANNELS:
            ep.setnvalue(MCrecordchannels);
            break;
        case P_RECORD_RATE:
            ep.setnvalue(MCrecordrate);
            break;
        case P_RECORD_SAMPLESIZE:
            ep.setnvalue(MCrecordsamplesize);
            break;
        case P_BREAK_POINTS:
            MCB_unparsebreaks(ep);
            break;
        case P_DEBUG_CONTEXT:
		{
			ep.clear();
			if (MCdebugcontext != MAXUINT2)
			{
				MCexecutioncontexts[MCdebugcontext]->getobj()->getprop(0, P_LONG_ID, ep, False);
				ep.concatnameref(MCexecutioncontexts[MCdebugcontext]->gethandler()->getname(), EC_COMMA, false);
				ep.concatuint(MCexecutioncontexts[MCdebugcontext]->getline(), EC_COMMA, false);
			}
		}
            break;
        case P_EXECUTION_CONTEXTS:
		{
			ep.clear();
			Boolean added = False;
			if (MCnexecutioncontexts < MAX_CONTEXTS)
			{
				ep.setline(line);
				MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
				added = True;
			}
			for (i = 0 ; i < MCnexecutioncontexts ; i++)
			{
				MCExecPoint ep2(ep);
				MCexecutioncontexts[i]->getobj()->getprop(0, P_LONG_ID, ep2, False);
				ep.concatmcstring(ep2.getsvalue(), EC_RETURN, i == 0);
                // PM-2014-04-14: [[Bug 12125]] Do this check to avoid a crash in LC server
                if (MCexecutioncontexts[i]->gethandler() != NULL)
                    ep.concatnameref(MCexecutioncontexts[i]->gethandler()->getname(), EC_COMMA, false);
				ep.concatuint(MCexecutioncontexts[i]->getline(), EC_COMMA, false);
				if (MCexecutioncontexts[i] -> getparentscript() != NULL)
				{
					MCexecutioncontexts[i] -> getparentscript() -> GetParent() -> GetObject() -> getprop(0, P_LONG_ID, ep2, False);
					ep.concatmcstring(ep2.getsvalue(), EC_COMMA, false);
				}
			}
			if (added)
				MCnexecutioncontexts--;
		}
            break;
        case P_MESSAGE_MESSAGES:
            ep.setboolean(MCmessagemessages);
            break;
        case P_WATCHED_VARIABLES:
            MCB_unparsewatches(ep);
            break;
        case P_ALLOW_INLINE_INPUT:
            ep.setboolean(MCinlineinput);
            break;
        case P_ACCEPT_DROP:
            ep.setboolean(MCdragaction != DRAG_ACTION_NONE);
            break;
        case P_ALLOWABLE_DRAG_ACTIONS:
        {
            int t_count;
            t_count = 0;
            
            ep . clear();
            if ((MCallowabledragactions & DRAG_ACTION_COPY) != 0)
                ep . concatcstring("copy", EC_COMMA, t_count++ == 0);
            else if ((MCallowabledragactions & DRAG_ACTION_MOVE) != 0)
                ep . concatcstring("move", EC_COMMA, t_count++ == 0);
            else if ((MCallowabledragactions & DRAG_ACTION_LINK) != 0)
                ep . concatcstring("link", EC_COMMA, t_count++ == 0);
        }
            break;
        case P_DRAG_ACTION:
            switch(MCdragaction)
		{
            case DRAG_ACTION_NONE:
                ep . setstaticcstring("none");
                break;
            case DRAG_ACTION_COPY:
                ep . setstaticcstring("copy");
                break;
            case DRAG_ACTION_MOVE:
                ep . setstaticcstring("move");
                break;
            case DRAG_ACTION_LINK:
                ep . setstaticcstring("link");
                break;
		}
            break;
        case P_DRAG_IMAGE:
            if (MCdragimageid != 0)
                ep . setint(MCdragimageid);
            else
                ep . setint(0);
            break;
        case P_DRAG_IMAGE_OFFSET:
            if (MCdragimageid != 0)
                ep.setpoint(MCdragimageoffset . x, MCdragimageoffset . y);
            else
                ep . clear();
            break;
            
            // MW-2008-08-12: Add access to the MCurlresult internal global variable
            //   this is set by libURL after doing DELETE, POST, PUT or GET type queries.
        case P_URL_RESPONSE:
            MCurlresult -> fetch(ep);
            break;
            
            // MW-2011-11-24: [[ Nice Folders ]] Handle fetching of the special folder types.
        case P_ENGINE_FOLDER:
            ep . setstaticcstring("engine");
            MCS_getspecialfolder(ep);
            break;
        case P_HOME_FOLDER:
            ep . setstaticcstring("home");
            MCS_getspecialfolder(ep);
            break;
        case P_DOCUMENTS_FOLDER:
            ep . setstaticcstring("documents");
            MCS_getspecialfolder(ep);
            break;
        case P_DESKTOP_FOLDER:
            ep . setstaticcstring("desktop");
            MCS_getspecialfolder(ep);
            break;
        case P_TEMPORARY_FOLDER:
            ep . setstaticcstring("temporary");
            MCS_getspecialfolder(ep);
            break;
            
        case P_IMAGE_CACHE_LIMIT:
            ep.setuint(MCCachedImageRep::GetCacheLimit());
            break;
			
        case P_IMAGE_CACHE_USAGE:
            ep.setuint(MCCachedImageRep::GetCacheUsage());
            break;
			
        case P_BRUSH_BACK_COLOR:
        case P_PEN_BACK_COLOR:
        case P_BRUSH_COLOR:
        case P_BRUSH_PATTERN:
        case P_PEN_COLOR:
        case P_PEN_PATTERN:
        case P_RECENT_CARDS:
        case P_RECENT_NAMES:
        case P_TEXT_ALIGN:
        case P_TEXT_FONT:
        case P_TEXT_HEIGHT:
        case P_TEXT_SIZE:
        case P_TEXT_STYLE:
        case P_EDIT_BACKGROUND:
        case P_ROUND_ENDS:
        case P_DASHES:
        case P_FILLED:
            
        case P_POLY_SIDES:
        case P_LINE_SIZE:
        case P_PEN_WIDTH:
        case P_PEN_HEIGHT:
        case P_ROUND_RADIUS:
        case P_START_ANGLE:
        case P_ARC_ANGLE:
        case P_NUMBER_FORMAT:
        case P_PLAY_DESTINATION:
        case P_PLAY_LOUDNESS:
        case P_LOCK_SCREEN:
        case P_STACK_FILES:
        case P_MENU_BAR:
        case P_EDIT_MENUS:
        case P_ACCENT_COLOR:
        case P_HILITE_COLOR:
        case P_PAINT_COMPRESSION:
        case P_LINK_COLOR:
        case P_LINK_HILITE_COLOR:
        case P_LINK_VISITED_COLOR:
        case P_UNDERLINE_LINKS:
        case P_SELECT_GROUPED_CONTROLS:
        case P_ICON:
        case P_ICON_MENU:
        case P_STATUS_ICON:
        case P_STATUS_ICON_MENU:
        case P_STATUS_ICON_TOOLTIP:
        case P_PROCESS_TYPE:
        case P_STACK_LIMIT:
        case P_ALLOW_DATAGRAM_BROADCASTS:
            // MERG-2013-08-17: [[ ColorDialogColors ]] Custom color management for the windows color dialog
        case P_COLOR_DIALOG_COLORS:
            
            if (target == NULL)
            {
                switch (which)
                {
                    case P_BRUSH_BACK_COLOR:
                    case P_PEN_BACK_COLOR:
                        ep.clear();
                        break;
                    case P_BRUSH_COLOR:
                        ep.setcolor(MCbrushcolor, MCbrushcolorname);
                        break;
                    case P_BRUSH_PATTERN:
                        if (MCbrushpmid < PI_END && MCbrushpmid > PI_PATTERNS)
                            ep.setint(MCbrushpmid - PI_PATTERNS);
                        else
                            
                            ep.setint(MCbrushpmid);
                        break;
                    case P_PEN_COLOR:
                        ep.setcolor(MCpencolor, MCpencolorname);
                        break;
                    case P_PEN_PATTERN:
                        if (MCpenpmid < PI_END && MCpenpmid > PI_PATTERNS)
                            ep.setint(MCpenpmid - PI_PATTERNS);
                        else
                            ep.setint(MCpenpmid);
                        break;
                    case P_RECENT_CARDS:
                        MCrecent->getlongids(NULL, ep);
                        break;
                    case P_RECENT_NAMES:
                        MCrecent->getnames(NULL, ep);
                        break;
                    case P_TEXT_ALIGN:
                    case P_TEXT_FONT:
                    case P_TEXT_HEIGHT:
                    case P_TEXT_SIZE:
                    case P_TEXT_STYLE:
                        ep.clear();
                        break;
                    case P_EDIT_BACKGROUND:
                        return MCdefaultstackptr->getprop(0, which, ep, False);
                    case P_ROUND_ENDS:
                        ep.setboolean(MCroundends);
                        break;
                    case P_DASHES:
                        ep.clear();
                        for (i = 0 ; i < MCndashes ; i++)
                            ep.concatuint(MCdashes[i], EC_COMMA, i == 0);
                        break;
                    case P_FILLED:
                        ep.setboolean(MCfilled);
                        break;
                    case P_POLY_SIDES:
                        ep.setint(MCpolysides);
                        break;
                    case P_LINE_SIZE:
                    case P_PEN_WIDTH:
                    case P_PEN_HEIGHT:
                        ep.setint(MClinesize);
                        break;
                    case P_ROUND_RADIUS:
                        ep.setint(MCroundradius);
                        break;
                    case P_START_ANGLE:
                        ep.setint(MCstartangle);
                        break;
                    case P_ARC_ANGLE:
                        ep.setint(MCarcangle);
                        break;
                    case P_NUMBER_FORMAT:
                        MCU_getnumberformat(ep, ep.getnffw(),
                                            ep.getnftrailing(), ep.getnfforce());
                        break;
                    case P_PLAY_DESTINATION:
                        return MCtemplateaudio->getprop(0, which, ep, False);
                        // AL-2014-08-12: [[ Bug 13161 ]] Get the global playLoudness rather than templateAudioClip playLoudness
                    case P_PLAY_LOUDNESS:
                    {
                        uint2 t_loudness;
                        t_loudness = 0;
                        extern bool MCSystemGetPlayLoudness(uint2& r_loudness);
#ifdef _MOBILE
                        if (MCSystemGetPlayLoudness(t_loudness))
#else
                            if (false)
#endif
                                ;
                            else
                                t_loudness = MCS_getplayloudness();
                        ep . setuint(t_loudness);
                    }
                        break;
                    case P_LOCK_SCREEN:
                        // MW-2011-08-18: [[ Redraw ]] Update to use redraw.
                        ep.setboolean(MCRedrawIsScreenLocked());
                        break;
                    case P_STACK_FILES:
                        ep.clear();
                        break;
                    case P_MENU_BAR:
                        if (MCmenubar == NULL)
                            ep.clear();
                        else
                            MCmenubar->getprop(0, P_LONG_NAME, ep, effective);
                        break;
                    case P_EDIT_MENUS:
                        ep.setboolean(True);
                        break;
                    case P_ACCENT_COLOR:
                        ep.setcolor(MCaccentcolor, MCaccentcolorname);
                        break;
                    case P_HILITE_COLOR:
                        ep.setcolor(MChilitecolor, MChilitecolorname);
                        break;
                    case P_PAINT_COMPRESSION:
                        switch (MCpaintcompression)
                    {
                        case EX_PNG:
                            ep.setstaticcstring("png");
                            break;
                        case EX_JPEG:
                            ep.setstaticcstring("jpeg");
                            break;
                        case EX_GIF:
                            ep.setstaticcstring("gif");
                            break;
                        default:
                            ep.setstaticcstring("rle");
                            break;
                    }
                        break;
                    case P_LINK_COLOR:
                        MCU_get_color(ep, MClinkatts.colorname, MClinkatts.color);
                        break;
                    case P_LINK_HILITE_COLOR:
                        MCU_get_color(ep, MClinkatts.hilitecolorname, MClinkatts.hilitecolor);
                        break;
                    case P_LINK_VISITED_COLOR:
                        MCU_get_color(ep, MClinkatts.visitedcolorname, MClinkatts.visitedcolor);
                        break;
                    case P_UNDERLINE_LINKS:
                        ep.setboolean(MClinkatts.underline);
                        break;
                    case P_SELECT_GROUPED_CONTROLS:
                        ep.setboolean(MCselectgrouped);
                        break;
                    case P_ICON:
                        ep.setint(MCiconid);
                        break;
                    case P_ICON_MENU:
                        ep.setsvalue(MCiconmenu);
                        break;
                    case P_STATUS_ICON:
                        ep.setint(MCstatusiconid);
                        break;
                    case P_STATUS_ICON_MENU:
                        ep.setsvalue(MCstatusiconmenu);
                        break;
                    case P_PROCESS_TYPE:
                        ep.setstaticcstring(MCS_processtypeisforeground() ? "foreground" : "background");
                        break;
                    case P_STACK_LIMIT:
                        ep.setuint(effective ? MCstacklimit : MCpendingstacklimit);
                        break;
                    case P_ALLOW_DATAGRAM_BROADCASTS:
                        ep . setboolean(MCallowdatagrambroadcasts);
                        break;
                        // MERG-2013-08-17: [[ ColorDialogColors ]] Custom color management for the windows color dialog
                    case P_COLOR_DIALOG_COLORS:
                        MCA_getcolordialogcolors(ep);
                        break;
                    default:
                        break;
                }
                break;
            }
        default:
            if (target == NULL)
            {
                Exec_stat t_stat;
                t_stat = ES_NORMAL;
                
                if (customindex != nil)
                    t_stat = customindex -> eval(ep);
                else
                    ep . clear();
                
                if (t_stat == ES_NORMAL)
                    t_stat = mode_eval(ep);
                if (t_stat != ES_NOT_HANDLED)
                    return t_stat;
            }
            
            if (tocount == CT_UNDEFINED)
            {
                Properties t_prop;
                MCNameRef t_prop_name, t_index_name;
                t_prop_name = t_index_name = nil;
                if (resolveprop(ep, t_prop, t_prop_name, t_index_name) != ES_NORMAL)
                    return ES_ERROR;
                
                Exec_stat t_stat;
                t_stat = ES_NORMAL;
                if (t_prop == P_CUSTOM)
                {
                    MCObject *t_object;
                    uint4 t_parid;
                    t_stat = target -> getobjforprop(ep, t_object, t_parid);
                    
                    // MW-2011-09-02: Moved handling of customprop != nil case into resolveprop,
                    //   so t_prop_name is always non-nil if t_prop == P_CUSTOM.
                    // MW-2011-11-23: [[ Array Chunk Props ]] Moved handling of arrayprops into
                    //   MCChunk::setprop.
                    if (t_stat == ES_NORMAL)
                    {
                        if (t_index_name == nil)
                            t_stat = t_object -> getcustomprop(ep, t_object -> getdefaultpropsetname(), t_prop_name);
                        else
                            t_stat = t_object -> getcustomprop(ep, t_prop_name, t_index_name);
                    }
                }
                else
                {
                    // MW-2011-11-23: [[ Array Chunk Props ]] If the prop is an array-prop, then
                    //   a nil index translates to the empty name (the array[empty] <=> the array).
                    MCNameRef t_derived_index_name;
                    if (t_prop < P_FIRST_ARRAY_PROP)
                        t_derived_index_name = nil;
                    else
                        t_derived_index_name = t_index_name != nil ? t_index_name : kMCEmptyName;
                    
                    t_stat = target -> getprop(t_prop, ep, t_derived_index_name, effective);
                }
                
                MCNameDelete(t_index_name);
                MCNameDelete(t_prop_name);
                
                if (t_stat != ES_NORMAL)
                {
                    MCeerror->add(EE_PROPERTY_NOPROP, line, pos);
                    return ES_ERROR;
                }
            }
            else
            {
                if (target->count(tocount, ptype, ep) != ES_NORMAL)
                {
                    MCeerror->add(EE_PROPERTY_BADCOUNTS, line, pos);
                    return ES_ERROR;
                }
            }
	}
	return ES_NORMAL;
#endif /* MCProperty::eval */


#ifdef LEGACY_EXEC
Exec_stat MCProperty::eval_variable(MCExecPoint& ep)
{
	return destvar -> eval(ep);
}

Exec_stat MCProperty::eval_function(MCExecPoint& ep)
{
	switch (function)
	{
		case F_DATE:
		{
			MCExecContext ctxt(ep);
			MCAutoStringRef t_date;

			MCDateTimeGetDate(ctxt, which, &t_date);

			if (!ctxt . HasError())
			{
				ep . setvalueref(*t_date);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_TIME:
		{
			MCExecContext ctxt(ep);
			MCAutoStringRef t_time;

			MCDateTimeGetTime(ctxt, which, &t_time);

			if (!ctxt . HasError())
			{
				ep . setvalueref(*t_time);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_MILLISECS:
		{
			MCExecContext ctxt(ep);
			double t_millisecs;

			if (which == P_LONG)
				MCDateTimeGetLongMilliseconds(ctxt, t_millisecs);
			else
				MCDateTimeGetMilliseconds(ctxt, t_millisecs);

			if (!ctxt . HasError())
			{
				ep . setnvalue(t_millisecs);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_SECONDS:
		{
			MCExecContext ctxt(ep);
			double t_secs;

			if (which == P_LONG)
				MCDateTimeGetLongSeconds(ctxt, t_secs);
			else
				MCDateTimeGetSeconds(ctxt, t_secs);

			if (!ctxt . HasError())
			{
				ep . setnvalue(t_secs);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_TICKS:
		{
			MCExecContext ctxt(ep);
			double t_ticks;

			if (which == P_LONG)
				MCDateTimeGetLongTicks(ctxt, t_ticks);
			else
				MCDateTimeGetTicks(ctxt, t_ticks);

			if (!ctxt . HasError())
			{
				ep . setnvalue(t_ticks);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_FILES:
		{
			MCExecContext ctxt(ep);
			MCAutoStringRef t_files;

			if (which == P_LONG)
				MCFilesGetDetailedFiles(ctxt, &t_files);
			else
				MCFilesGetFiles(ctxt, &t_files);

			if (!ctxt . HasError())
			{
				ep . setvalueref(*t_files);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_DIRECTORIES:
		{
			MCExecContext ctxt(ep);
			MCAutoStringRef t_folders;

			if (which == P_LONG)
				MCFilesGetDetailedFolders(ctxt, &t_folders);
			else
				MCFilesGetFolders(ctxt, &t_folders);

			if (!ctxt . HasError())
			{
				ep . setvalueref(*t_folders);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_MONTH_NAMES:
		{
			MCExecContext ctxt(ep);
			MCAutoStringRef t_month_names;

			MCDateTimeGetMonthNames(ctxt, which, &t_month_names);

			if (!ctxt . HasError())
			{
				ep . setvalueref(*t_month_names);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_WEEK_DAY_NAMES:
		{
			MCExecContext ctxt(ep);
			MCAutoStringRef t_week_day_names;

			MCDateTimeGetWeekDayNames(ctxt, which, &t_week_day_names);

			if (!ctxt . HasError())
			{
				ep . setvalueref(*t_week_day_names);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_DATE_FORMAT:
		{
			MCExecContext ctxt(ep);
			MCAutoStringRef t_date_format;

			MCDateTimeGetDateFormat(ctxt, which, &t_date_format);

			if (!ctxt . HasError())
			{
				ep . setvalueref(*t_date_format);
				return ES_NORMAL;
			}

			return ctxt . Catch(line, pos);
		}
		case F_SCREEN_RECT:
		if (target == NULL)
		{
			MCExecContext ctxt(ep);

			if ((which % 1000) == P_LONG)
			{
				MCAutoStringRef t_rects;

				MCInterfaceGetScreenRects(ctxt, which >= 1000, effective == True, &t_rects);

				if (!ctxt . HasError())
				{
					ep . setvalueref(*t_rects);
					return ES_NORMAL;
				}
			}
			else
			{
				MCRectangle t_rect;

				MCInterfaceGetScreenRect(ctxt, which >= 1000, effective == True, t_rect);

				if (!ctxt . HasError())
				{
					ep . setrectangle(t_rect);
					return ES_NORMAL;
				}
			}

			return ctxt . Catch(line, pos);
		}
		default:
			break;
	}
	
	fprintf(stderr, "MCProperty: ERROR bad function in eval\n");
	return ES_ERROR;
}

Exec_stat MCProperty::eval_global_property(MCExecPoint& ep)
{
	const MCPropertyInfo *t_info;
	if (MCPropertyInfoTableLookup(which, effective, t_info))
	{
		MCExecContext ctxt(ep);
        MCAutoValueRef t_value;
        
        if (t_info -> custom_index)
        {
            MCNewAutoNameRef t_type;
            
            if (customindex != nil)
            {
                if (customindex -> eval(ep) != ES_NORMAL)
                {
                    MCeerror -> add(EE_PROPERTY_BADEXPRESSION, line, pos);
                    return ES_ERROR;
                }
                ep . copyasnameref(&t_type);
            }
            MCExecFetchProperty(ctxt, t_info, *t_type, &t_value);
        }
        else
            MCExecFetchProperty(ctxt, t_info, nil, &t_value);
        
        if (!ctxt . HasError())
        {
            ep . setvalueref(*t_value);
            return ES_NORMAL;
        }
		
        return ctxt . Catch(line, pos);
	}

	Exec_stat t_stat;
	t_stat = ES_NORMAL;
	
	if (customindex != nil)
		t_stat = customindex -> eval(ep);
	else
		ep . clear();
	
	if (t_stat == ES_NORMAL)
		t_stat = mode_eval(ep);
	if (t_stat != ES_NORMAL)
	{
		MCeerror->add(EE_PROPERTY_NOPROP, line, pos);
		return ES_ERROR;
	}

	return ES_NORMAL;
}

Exec_stat MCProperty::eval_object_property(MCExecPoint& ep)
{
	Properties t_prop;
	MCNameRef t_prop_name, t_index_name;
	t_prop_name = t_index_name = nil;
	if (resolveprop(ep, t_prop, t_prop_name, t_index_name) != ES_NORMAL)
		return ES_ERROR;
	
	Exec_stat t_stat;
	t_stat = ES_NORMAL;
	if (t_prop == P_CUSTOM)
	{
		MCObject *t_object;
		uint4 t_parid;
		t_stat = target -> getobjforprop(ep, t_object, t_parid);
		
		// MW-2011-09-02: Moved handling of customprop != nil case into resolveprop,
		//   so t_prop_name is always non-nil if t_prop == P_CUSTOM.
		// MW-2011-11-23: [[ Array Chunk Props ]] Moved handling of arrayprops into
		//   MCChunk::setprop.
		if (t_stat == ES_NORMAL)
		{
			ep.clear();
			Boolean added = False;
			if (MCnexecutioncontexts < MAX_CONTEXTS)
			{
				ep.setline(line);
				MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
				added = True;
			}
			for (i = 0 ; i < MCnexecutioncontexts ; i++)
			{
				MCExecPoint ep2(ep);
				MCexecutioncontexts[i]->getobj()->getprop(0, P_LONG_ID, ep2, False);
				ep.concatmcstring(ep2.getsvalue(), EC_RETURN, i == 0);
                // PM-2014-04-14: [[Bug 12125]] Do this check to avoid a crash in LC server
                if (MCexecutioncontexts[i]->gethandler() != NULL)
                    ep.concatnameref(MCexecutioncontexts[i]->gethandler()->getname(), EC_COMMA, false);
				ep.concatuint(MCexecutioncontexts[i]->getline(), EC_COMMA, false);
				if (MCexecutioncontexts[i] -> getparentscript() != NULL)
				{
					MCexecutioncontexts[i] -> getparentscript() -> GetParent() -> GetObject() -> getprop(0, P_LONG_ID, ep2, False);
					ep.concatmcstring(ep2.getsvalue(), EC_COMMA, false);
				}
			}
			if (added)
				MCnexecutioncontexts--;
		}
	}
	else
	{
		// MW-2011-11-23: [[ Array Chunk Props ]] If the prop is an array-prop, then
		//   a nil index translates to the empty name (the array[empty] <=> the array).
		MCNameRef t_derived_index_name;
		if (t_prop < P_FIRST_ARRAY_PROP)
			t_derived_index_name = nil;
		else
			t_derived_index_name = t_index_name != nil ? t_index_name : kMCEmptyName;
		
		t_stat = target -> getprop(t_prop, ep, t_derived_index_name, effective);
	}
	
	MCNameDelete(t_index_name);
	MCNameDelete(t_prop_name);
	
	if (t_stat != ES_NORMAL)
	{
		MCeerror->add(EE_PROPERTY_NOPROP, line, pos);
		return ES_ERROR;
	}

	return ES_NORMAL;
}

Exec_stat MCProperty::eval_count(MCExecPoint& ep)
{
	if (target->count(tocount, ptype, ep) != ES_NORMAL)
	{
		MCeerror->add(EE_PROPERTY_BADCOUNTS, line, pos);
		return ES_ERROR;
	}
	
	return ES_NORMAL;
}
#endif

////////////////////////////////////////////////////////////////////////////////

static MCPropertyInfo *lookup_mode_property(const MCPropertyTable *p_table, Properties p_which, bool p_effective, bool p_is_array_prop)
{
	for(uindex_t i = 0; i < p_table -> size; i++)
		if (p_table -> table[i] . property == p_which && p_table -> table[i] . effective == p_effective &&
            p_table -> table[i] . is_array_prop == p_is_array_prop)
			return &p_table -> table[i];
	
	return nil;
}

void MCProperty::eval_variable_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    destvar -> eval_ctxt(ctxt, r_value);
}

void MCProperty::eval_function_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
	switch (function)
	{
		case F_DATE:
		{
			MCDateTimeGetDate(ctxt, which, r_value . stringref_value);
            
			if (!ctxt . HasError())
				r_value . type = kMCExecValueTypeStringRef;
            
            return;
		}
		case F_TIME:
		{
			MCDateTimeGetTime(ctxt, which, r_value . stringref_value);
            
			if (!ctxt . HasError())
				r_value . type = kMCExecValueTypeStringRef;
            
            return;
		}
		case F_MILLISECS:
		{
			if (which == P_LONG)
				MCDateTimeGetLongMilliseconds(ctxt, r_value . double_value);
			else
				MCDateTimeGetMilliseconds(ctxt, r_value . double_value);
            
			if (!ctxt . HasError())
                r_value . type = kMCExecValueTypeDouble;
            
            return;
		}
		case F_SECONDS:
		{
			if (which == P_LONG)
				MCDateTimeGetLongSeconds(ctxt, r_value . double_value);
			else
				MCDateTimeGetSeconds(ctxt, r_value . double_value);
            
			if (!ctxt . HasError())
                r_value . type = kMCExecValueTypeDouble;
            
            return;
		}
		case F_TICKS:
		{
			if (which == P_LONG)
				MCDateTimeGetLongTicks(ctxt, r_value . double_value);
			else
				MCDateTimeGetTicks(ctxt, r_value . double_value);
            
			if (!ctxt . HasError())
                r_value . type = kMCExecValueTypeDouble;
            
            return;
		}
		case F_FILES:
		{
			if (which == P_LONG)
				MCFilesGetDetailedFiles(ctxt, r_value . stringref_value);
			else
				MCFilesGetFiles(ctxt, r_value . stringref_value);
            
			if (!ctxt . HasError())
				r_value . type = kMCExecValueTypeStringRef;
            
            return;
		}
		case F_DIRECTORIES:
		{
			if (which == P_LONG)
				MCFilesGetDetailedFolders(ctxt, r_value . stringref_value);
			else
				MCFilesGetFolders(ctxt, r_value . stringref_value);
            
			if (!ctxt . HasError())
				r_value . type = kMCExecValueTypeStringRef;
            
            return;
		}
		case F_MONTH_NAMES:
		{
			MCDateTimeGetMonthNames(ctxt, which, r_value . stringref_value);
            
			if (!ctxt . HasError())
				r_value . type = kMCExecValueTypeStringRef;
            
            return;
		}
		case F_WEEK_DAY_NAMES:
		{
			MCDateTimeGetWeekDayNames(ctxt, which, r_value . stringref_value);
            
			if (!ctxt . HasError())
				r_value . type = kMCExecValueTypeStringRef;
            
            return;
		}
		case F_DATE_FORMAT:
		{
			MCDateTimeGetDateFormat(ctxt, which, r_value . stringref_value);
            
			if (!ctxt . HasError())
				r_value . type = kMCExecValueTypeStringRef;
            
            return;
		}
		case F_SCREEN_RECT:
		{
			if ((which % 1000) == P_LONG)
			{
				MCInterfaceGetScreenRects(ctxt, which >= 1000, effective == True, r_value . stringref_value);
                
                if (!ctxt . HasError())
                    r_value . type = kMCExecValueTypeStringRef;
                
                return;
			}
			else
			{
				MCInterfaceGetScreenRect(ctxt, which >= 1000, effective == True, r_value . rectangle_value);
                
                if (!ctxt . HasError())
                    r_value . type = kMCExecValueTypeRectangle;
                
                return;
            }
		}
		default:
			break;
	}
	
	fprintf(stderr, "MCProperty: ERROR bad function in eval\n");
	ctxt . Throw();
}

void MCProperty::eval_global_property_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    bool t_is_array_prop;
    MCNewAutoNameRef t_index;
    
    if (customindex != nil)
        ctxt . EvalExprAsNameRef(customindex, EE_PROPERTY_BADEXPRESSION, &t_index);
    
    t_is_array_prop = (*t_index != nil && !MCNameIsEmpty(*t_index));
    
	const MCPropertyInfo *t_info;
    // AL-2014-09-01: [[ Bug 13312 ]] Initialise t_info to nil to prevent crashes
    t_info = nil;
    
	if (!MCPropertyInfoTableLookup(which, effective, t_info, t_is_array_prop))
        t_info = lookup_mode_property(getmodepropertytable(), which, effective, t_is_array_prop);
        
    if (t_info != nil)
    {
        MCExecFetchProperty(ctxt, t_info, *t_index, r_value);
        return;
	}

    MCeerror->add(EE_PROPERTY_NOPROP, line, pos);
    ctxt . Throw();
}

void MCProperty::eval_object_property_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
	Properties t_prop;
	MCNewAutoNameRef t_prop_name, t_index_name;
    
    bool t_success;
    t_success = resolveprop(ctxt, t_prop, &t_prop_name, &t_index_name);
	
	if (t_prop == P_CUSTOM)
	{
		MCObject *t_object;
		uint4 t_parid;
		if (t_success)
            t_success = target -> getobjforprop(ctxt, t_object, t_parid);
		
		// MW-2011-09-02: Moved handling of customprop != nil case into resolveprop,
		//   so t_prop_name is always non-nil if t_prop == P_CUSTOM.
		// MW-2011-11-23: [[ Array Chunk Props ]] Moved handling of arrayprops into
		//   MCChunk::setprop.
		if (t_success)
		{
			if (*t_index_name == nil)
				t_success = t_object -> getcustomprop(ctxt, t_object -> getdefaultpropsetname(), *t_prop_name, r_value);
			else
				t_success = t_object -> getcustomprop(ctxt, *t_prop_name, *t_index_name, r_value);
		}
	}
	else
	{
		// MW-2011-11-23: [[ Array Chunk Props ]] If the prop is an array-prop, then
		//   a nil index translates to the empty name (the array[empty] <=> the array).
		MCNameRef t_derived_index_name;
        t_derived_index_name = *t_index_name != nil ? *t_index_name : kMCEmptyName;
		
        if (t_success)
            t_success = target -> getprop(ctxt, t_prop, t_derived_index_name, effective, r_value);
	}
	
	if (!t_success)
    {
		MCeerror->add(EE_PROPERTY_NOPROP, line, pos);
        ctxt . Throw();
    }
}

void MCProperty::eval_count_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
	target->count(ctxt, tocount, ptype, r_value . uint_value);
    
    if (!ctxt . HasError())
        r_value . type = kMCExecValueTypeUInt;
}

void MCProperty::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
	ctxt . SetLineAndPos(line, pos);
    
	if (destvar != nil && which != P_CUSTOM_VAR)
		return eval_variable_ctxt(ctxt, r_value);
	
	if (function != F_UNDEFINED)
		return eval_function_ctxt(ctxt, r_value);
	
	if (target == nil)
		return eval_global_property_ctxt(ctxt, r_value);
	
	if (tocount == CT_UNDEFINED)
		return eval_object_property_ctxt(ctxt, r_value);
	
	return eval_count_ctxt(ctxt, r_value);
}

void MCProperty::set_variable(MCExecContext& ctxt, MCExecValue p_value)
{
    destvar -> give_value(ctxt, p_value);
}

void MCProperty::set_global_property(MCExecContext& ctxt, MCExecValue p_value)
{
    bool t_is_array_prop;
    MCNewAutoNameRef t_index;
    
    if (customindex != nil)
        ctxt . EvalExprAsNameRef(customindex, EE_PROPERTY_BADEXPRESSION, &t_index);
    
    t_is_array_prop = (*t_index != nil && !MCNameIsEmpty(*t_index));
    
	const MCPropertyInfo *t_info;
    // AL-2014-09-01: [[ Bug 13312 ]] Initialise t_info to nil to prevent crashes
    t_info = nil;
    
    if (!MCPropertyInfoTableLookup(which, effective, t_info, t_is_array_prop))
        t_info = lookup_mode_property(getmodepropertytable(), which, effective, t_is_array_prop);
    
    if (t_info != nil)
    {
        MCExecStoreProperty(ctxt, t_info, *t_index, p_value);
        return;
	}
    
    MCeerror->add(EE_PROPERTY_CANTSET, line, pos);
    ctxt . Throw();
}

void MCProperty::set_object_property(MCExecContext& ctxt, MCExecValue p_value)
{
	Properties t_prop;
	MCNewAutoNameRef t_prop_name, t_index_name;
	if (!resolveprop(ctxt, t_prop, &t_prop_name, &t_index_name))
        return;
	
    bool t_success;
    t_success = true;
    
	if (t_prop == P_CUSTOM)
	{
		MCObject *t_object;
		uint4 t_parid;
		t_success = target -> getobjforprop(ctxt, t_object, t_parid);
		
		// MW-2011-09-02: Moved handling of customprop != nil case into resolveprop,
		//   so t_prop_name is always non-nil if t_prop == P_CUSTOM.
		// MW-2011-11-23: [[ Array Chunk Props ]] Moved handling of arrayprops into
		//   MCChunk::setprop.
		if (t_success)
		{
			if (*t_index_name == nil)
				t_success = t_object -> setcustomprop(ctxt, t_object -> getdefaultpropsetname(), *t_prop_name, p_value);
			else
				t_success = t_object -> setcustomprop(ctxt, *t_prop_name, *t_index_name, p_value);
			// MM-2012-09-05: [[ Property Listener ]] Make sure setting a custom property sends propertyChanged message to listeners.
			if (t_success)
				t_object -> signallisteners(t_prop);
		}
	}
	else
	{   
		// MW-2011-11-23: [[ Array Chunk Props ]] If the prop is an array-prop, then
		//   a nil index translates to the empty name (the array[empty] <=> the array).
		MCNameRef t_derived_index_name;
        t_derived_index_name = *t_index_name != nil ? *t_index_name : kMCEmptyName;
		
		t_success = target -> setprop(ctxt, t_prop, t_derived_index_name, effective, p_value);
	}
    
	if (!t_success)
    {
		MCeerror->add(EE_PROPERTY_CANTSETOBJECT, line, pos);
        ctxt . Throw();
    }
}

void MCProperty::set(MCExecContext& ctxt, MCExecValue p_value)
{
    if (destvar != NULL && which != P_CUSTOM_VAR)
        set_variable(ctxt, p_value);
    else if (target == nil)
        set_global_property(ctxt, p_value);
    else
        set_object_property(ctxt, p_value);
}
