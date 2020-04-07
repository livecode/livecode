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
	
    DEFINE_RW_ARRAY_PROPERTY(P_REV_LIBRARY_MAPPING, String, Engine, RevLibraryMappingByKey)
    DEFINE_RO_ARRAY_PROPERTY(P_REV_LICENSE_INFO, Array, License, RevLicenseInfoByKey)
	DEFINE_RO_PROPERTY(P_REV_LICENSE_INFO, String, License, RevLicenseInfo)
	DEFINE_RW_PROPERTY(P_REV_LICENSE_LIMITS, Array, License, RevLicenseLimits)
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
	DEFINE_RW_CUSTOM_PROPERTY(P_RECORD_FORMAT, MultimediaRecordFormat, Multimedia, RecordFormat)
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
	DEFINE_RW_PROPERTY(P_BRUSH_PATTERN, OptionalUInt16, Interface, BrushPattern)
	DEFINE_RW_PROPERTY(P_PEN_PATTERN, OptionalUInt16, Interface, PenPattern)
	DEFINE_RW_PROPERTY(P_FILLED, Bool, Interface, Filled)
	DEFINE_RW_PROPERTY(P_POLY_SIDES, UInt16, Interface, PolySides)
	DEFINE_RW_PROPERTY(P_LINE_SIZE, UInt16, Interface, LineSize)
	DEFINE_RW_PROPERTY(P_PEN_WIDTH, UInt16, Interface, LineSize)
	DEFINE_RW_PROPERTY(P_PEN_HEIGHT, UInt16, Interface, LineSize)
	DEFINE_RW_PROPERTY(P_ROUND_RADIUS, UInt16, Interface, RoundRadius)
	DEFINE_RW_PROPERTY(P_START_ANGLE, UInt16, Interface, StartAngle)
	DEFINE_RW_PROPERTY(P_ARC_ANGLE, UInt16, Interface, ArcAngle)
	DEFINE_RW_PROPERTY(P_ROUND_ENDS, Bool, Interface, RoundEnds)
	DEFINE_RW_PROPERTY(P_DASHES, ItemsOfLooseUInt, Interface, Dashes)
	
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
	DEFINE_RW_PROPERTY(P_BRUSH, UInt32, Interface, Brush)
	DEFINE_RW_PROPERTY(P_ERASER, UInt32, Interface, Eraser)
	DEFINE_RW_PROPERTY(P_SPRAY, UInt32, Interface, Spray)

	DEFINE_RW_PROPERTY(P_DEFAULT_NETWORK_INTERFACE, String, Network, DefaultNetworkInterface)
	DEFINE_RO_PROPERTY(P_NETWORK_INTERFACES, String, Network, NetworkInterfaces)
	
	DEFINE_RW_PROPERTY(P_DEBUG_CONTEXT, String, Debugging, DebugContext)
	DEFINE_RO_PROPERTY(P_EXECUTION_CONTEXTS, String, Debugging, ExecutionContexts)
	DEFINE_RW_PROPERTY(P_BREAK_POINTS, String, Debugging, Breakpoints)
	DEFINE_RW_PROPERTY(P_WATCHED_VARIABLES, String, Debugging, WatchedVariables)
    DEFINE_RW_PROPERTY(P_LOG_MESSAGE, String, Debugging, LogMessage)

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
    DEFINE_RO_PROPERTY(P_SCREEN_PIXEL_SCALES, LinesOfLooseDouble, Interface, ScreenPixelScales)
    
    // MW-2014-08-12: [[ EditionType ]] Return whether the engine is community or commercial.
    DEFINE_RO_PROPERTY(P_EDITION_TYPE, String, Engine, EditionType)
    
    // MW-2014-12-10: [[ Extensions ]] Returns a list of loaded extensions.
    DEFINE_RO_PROPERTY(P_LOADED_EXTENSIONS, ProperLinesOfString, Engine, LoadedExtensions)
	
	DEFINE_RO_ENUM_PROPERTY(P_SYSTEM_APPEARANCE, InterfaceSystemAppearance, Interface, SystemAppearance)
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
        {
            r_name = MCNAME(property_overrides[i].lt.token);
            return true;
        }
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

MCProperty::MCProperty()
{
	tocount = CT_UNDEFINED;
	ptype = CT_UNDEFINED;
	which = P_UNDEFINED;
	function = F_UNDEFINED;
	effective = False;
}

MCProperty::~MCProperty()
{
}

MCObject *MCProperty::getobj(MCExecContext &ctxt)
{
	MCObject *objptr = NULL;
	MCChunk *tchunk = new (nothrow) MCChunk(False);
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

Parse_stat MCProperty::parse(MCScriptPoint &sp, Boolean the)
{
	Symbol_type type;
	const LT *te;
	Boolean lp = False;

	initpoint(sp);

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
		if (sp . findvar(sp.gettoken_nameref(), &(&destvar)) == PS_NORMAL)
		{
			destvar->parsearray(sp);
			which = P_CUSTOM_VAR;
		}
		else
		{
			which = P_CUSTOM;
            customprop = sp . gettoken_nameref();
			if (sp.next(type) == PS_NORMAL && type == ST_LB)
			{
				if (sp.parseexp(False, True, &(&customindex)) != PS_NORMAL
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
    case P_LOG_MESSAGE:
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

	case P_MESSAGE_BOX_LAST_OBJECT:
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
	case P_SYSTEM_APPEARANCE:
        break;
    
    case P_REV_LIBRARY_MAPPING:
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
		if (sp.parseexp(False, True, &(&customindex)) != PS_NORMAL)
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
	case P_SHOW_INVISIBLES:
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
                if (sp.parseexp(False, True, &(&customindex)) != PS_NORMAL
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
				if (sp . finduqlvar(sp.gettoken_nameref(), &(&destvar)) != PS_NORMAL)
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
						/* UNCHECKED */ target.Reset(new (nothrow) MCChunk(False));
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
						/* UNCHECKED */ target.Reset(new (nothrow) MCChunk(False));
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
		/* UNCHECKED */ target.Reset(new (nothrow) MCChunk(False));
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
        t_prop_name = MCValueRetain(*customprop);
    }
	
	// At present, something like the 'pVar[pIndex] of me' is evaluated as 'the pVar of me'
	// this is because any index is extracted from the pVar variable. It might be worth
	// considering altering this behavior slightly, to allow dynamic indices on dynamic
	// props, although this needs to be done sympathetically to the current behavior...
	if (which == P_CUSTOM_VAR)
	{
        MCAutoStringRef t_string;
        if (!ctxt  . EvalExprAsStringRef(destvar.Get(), EE_PROPERTY_BADEXPRESSION, &t_string))
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
			
			// As we are only looking for an ASCII char, GetCharAtIndex is
			// sufficient (no need for GetCodepointAtIndex).
            if (MCStringGetCharAtIndex(*t_string, t_offset + 1) == '"' &&
                MCStringGetCharAtIndex(*t_string, t_end_offset - 1) == '"')
            {
                t_offset++;
                t_end_offset--;
            }
            
            if (!MCStringCopySubstring(*t_string, MCRangeMakeMinMax(t_offset + 1, t_end_offset), &t_icarray))
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
	else if (customindex)
    {
        if (!ctxt . EvalExprAsNameRef(customindex.Get(), EE_PROPERTY_BADEXPRESSION,
                                      t_index_name))
            return false;
    }
    
	r_which = t_prop;
	r_prop_name = t_prop_name;
	r_index_name = t_index_name;
    return true;
}

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
    
    if (customindex)
        ctxt . EvalExprAsNameRef(customindex.Get(), EE_PROPERTY_BADEXPRESSION,
                                 &t_index);
    
    t_is_array_prop = (*t_index != nil && !MCNameIsEmpty(*t_index));
    
	const MCPropertyInfo *t_info;
    // AL-2014-09-01: [[ Bug 13312 ]] Initialise t_info to nil to prevent crashes
    t_info = nil;
    
	if (!MCPropertyInfoTableLookup(which, effective, t_info, t_is_array_prop))
        t_info = lookup_mode_property(getmodepropertytable(), which, effective, t_is_array_prop);
        
    if (t_info != nil && t_info -> getter != nil)
    {
        MCExecFetchProperty(ctxt, t_info, *t_index, r_value);
        return;
	}

    ctxt . LegacyThrow(EE_PROPERTY_NOPROP);
}

void MCProperty::eval_object_property_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
	Properties t_prop;
	MCNewAutoNameRef t_prop_name, t_index_name;
    
    bool t_success;
    t_success = resolveprop(ctxt, t_prop, &t_prop_name, &t_index_name);
	
	if (t_prop == P_CUSTOM)
	{
        if (t_success)
            t_success = target -> getcustomprop(ctxt, *t_prop_name, *t_index_name, r_value);
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
    
	if (destvar && which != P_CUSTOM_VAR)
		return eval_variable_ctxt(ctxt, r_value);
	
	if (function != F_UNDEFINED)
		return eval_function_ctxt(ctxt, r_value);
	
	if (!target)
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
    
    if (customindex)
        ctxt . EvalExprAsNameRef(customindex.Get(), EE_PROPERTY_BADEXPRESSION,
                                 &t_index);
    
    t_is_array_prop = (*t_index != nil && !MCNameIsEmpty(*t_index));
    
	const MCPropertyInfo *t_info;
    // AL-2014-09-01: [[ Bug 13312 ]] Initialise t_info to nil to prevent crashes
    t_info = nil;
    
    if (!MCPropertyInfoTableLookup(which, effective, t_info, t_is_array_prop))
        t_info = lookup_mode_property(getmodepropertytable(), which, effective, t_is_array_prop);
    
    if (t_info != nil && t_info -> setter != nil)
    {
        MCExecStoreProperty(ctxt, t_info, *t_index, p_value);
        return;
	}
    
    ctxt . LegacyThrow(EE_PROPERTY_CANTSET);
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
        t_success = target -> setcustomprop(ctxt, *t_prop_name, *t_index_name, p_value);
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
    if (destvar && which != P_CUSTOM_VAR)
        set_variable(ctxt, p_value);
    else if (!target)
        set_global_property(ctxt, p_value);
    else
        set_object_property(ctxt, p_value);
}
