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

#include "cmds.h"
#include "visual.h"
#include "keywords.h"
#include "funcs.h"
#include "operator.h"
#include "newobj.h"
#include "answer.h"
#include "ask.h"
#include "internal.h"

#include "mode.h"

MCStatement *MCN_new_statement(int2 which)
{
	switch (which)
	{
    case S_INTERNAL:
        return new MCInternal;
	case S_ACCEPT:
		return new MCAccept;
	case S_ADD:
		return new MCAdd;
	case S_ANSWER:
		return new MCAnswer;
	case S_ASK:
		return new MCAsk;
	// MW-2013-11-14: [[ AssertCmd ]] Constructor for assert command.
	case S_ASSERT:
		return new MCAssertCmd;
	case S_BEEP:
		return new MCBeep;
	case S_BREAK:
		return new MCBreak;
	case S_BREAKPOINT:
		return new MCBreakPoint;
	case S_CALL:
		return new MCCall;
	case S_CANCEL:
		return new MCCancel;
	case S_CHOOSE:
		return new MCChoose;
	case S_CLICK:
		return new MCClickCmd;
	case S_CLONE:
		return new MCClone;
	case S_CLOSE:
		return new MCClose;
	case S_COMBINE:
		return new MCCombine;
	case S_COMPACT:
		return new MCCompact;
	case S_CONSTANT:
		return new MCLocalConstant;
	case S_CONVERT:
		return new MCConvert;
	case S_COPY:
		return new MCCopyCmd;
	case S_CREATE:
		return new MCCreate;
	case S_CROP:
		return new MCCrop;
	case S_CUT:
		return new MCCutCmd;
	case S_DEBUGDO:
		return new MCDebugDo;
	case S_DECRYPT:
		return new MCCipherDecrypt;
	case S_DEFINE:
		return new MCDefine;
	case S_DELETE:
		return new MCDelete;
    case S_DIFFERENCE:
        return new MCSetOp(MCSetOp::kOpDifference);
	case S_DISABLE:
		return new MCDisable;
	// MW-2008-11-05: [[ Dispatch Command ]] Create a dispatch statement object
	case S_DISPATCH:
		return new MCDispatchCmd;
	case S_DIVIDE:
		return new MCDivide;
	case S_DO:
		return new MCRegularDo;
	case S_DOMENU:
		return new MCDoMenu;
	case S_DRAG:
		return new MCDrag;
	case S_DRAWER:
		return new MCDrawer;
	case S_ECHO:
		return new MCEcho;
	case S_EDIT:
		return new MCEdit;
	case S_ENABLE:
		return new MCEnable;
	case S_ENCRYPT:
		return new MCCipherEncrypt;
	case S_EXPORT:
		return new MCExport;
	case S_EXIT:
		return new MCExit;
	case S_FILTER:
		return new MCFilter;
	case S_FIND:
		return new MCFind;
	case S_FLIP:
		return new MCFlip;
	case S_FOCUS:
		return new MCFocus;
	case S_GET:
		return new MCGet;
	case S_GLOBAL:
		return new MCGlobal;
	case S_GO:
		return new MCGo;
	case S_GRAB:
		return new MCGrab;
	case S_GROUP:
		return new MCMakeGroup;
	case S_HIDE:
		return new MCHide;
	case S_HILITE:
		return new MCHilite;
	case S_IF:
		return new MCIf;
	case S_INCLUDE:
		return new MCInclude(false);
	case S_IMPORT:
		return new MCImport;
	case S_INSERT:
		return new MCInsert;
	case S_INTERSECT:
		return new MCSetOp(MCSetOp::kOpIntersect);
	case S_KILL:
		return new MCKill;
	case S_LAUNCH:
		return new MCLaunch;
	case S_LIBRARY:
		return new MCLibrary;
	case S_LOAD:
		return new MCLoad;
	case S_LOCAL:
		return new MCLocalVariable;
	case S_LOCK:
		return new MCLock;
    case S_LOG:
        return new MCLogCmd;
    case S_MARK:
		return new MCMarkCommand;
	case S_MODAL:
		return new MCModal;
	case S_MODELESS:
		return new MCModeless;
	case S_MOVE:
		return new MCMove;
	case S_MULTIPLY:
		return new MCMultiply;
	case S_NEXT:
		return new MCNext;
	case S_OPEN:
		return new MCOpen;
	case S_OPTION:
		return new MCOption;
	case S_PALETTE:
		return new MCPalette;
	case S_PASS:
		return new MCPass;
	case S_PASTE:
		return new MCPasteCmd;
	case S_PLACE:
		return new MCPlace;
	case S_PLAY:
		return new MCPlay;
	case S_POP:
		return new MCPop;
	case S_POPUP:
		return new MCPopup;
	case S_POST:
		return new MCPost;
	case S_PREPARE:
		return new MCPrepare;
	case S_PRINT:
		return new MCPrint;
	case S_PULLDOWN:
		return new MCPulldown;
	case S_PUSH:
		return new MCPush;
	case S_PUT:
		return new MCPut;
	case S_QUIT:
		return new MCQuit;
	case S_READ:
		return new MCRead;
	case S_RECORD:
		return new MCRecord;
	case S_REDO:
		return new MCRedo;
	case S_RELAYER:
		return new MCRelayer;
	case S_REMOVE:
		return new MCRemove;
	case S_RENAME:
		return new MCRename;
	case S_REPEAT:
		return new MCRepeat;
	case S_REPLACE:
		return new MCReplace;
	case S_REPLY:
		return new MCReply;
	case S_REQUEST:
		return new MCRequest;
	case S_REQUIRE:
		return new MCInclude(true);
	case S_RESET:
		return new MCReset;
    case S_RESOLVE:
        return new MCResolveImage;
    case S_RETURN:
		return new MCReturn;
	case S_REVERT:
		return new MCRevert;
	case S_ROTATE:
		return new MCRotate;
	case S_SAVE:
		return new MCSave;
	case S_SCRIPT_ERROR:
		return new MCScriptError;
	// MM-2014-02-12: [[ SecureSocket ]] Create secure statement object
	case S_SECURE:
		return new MCSecure;
	case S_SEEK:
		return new MCSeek;
	case S_SELECT:
		return new MCSelect;
	case S_SEND:
		return new MCSend;
	case S_SET:
		return new MCSet;
	case S_SHEET:
		return new MCSheet;
	case S_SHOW:
		return new MCShow;
	case S_SORT:
		return new MCSort;
	case S_SPLIT:
		return new MCSplit;
	case S_START:
		return new MCStart;
	case S_STOP:
		return new MCStop;
	case S_SUBTRACT:
		return new MCSubtract;
	case S_SWITCH:
		return new MCSwitch;
    case S_SYMMETRIC:
        return new MCSetOp(MCSetOp::kOpSymmetricDifference);
	case S_THROW:
		return new MCThrowKeyword;
	case S_TOP_LEVEL:
		return new MCTopLevel;
	case S_TRY:
		return new MCTry;
	case S_TYPE:
		return new MCType;
	case S_UNDEFINE:
		return new MCUndefine;
	case S_UNDO:
		return new MCUndoCmd;
	case S_UNGROUP:
		return new MCUngroup;
	case S_UNHILITE:
		return new MCUnhilite;
	case S_UNION:
		return new MCSetOp(MCSetOp::kOpUnion);
	case S_UNLOAD:
		return new MCUnload;
	case S_UNLOCK:
		return new MCUnlock;
	case S_UNMARK:
		return new MCUnmark;
	case S_VISUAL:
		return new MCVisualEffect;
	case S_WAIT:
		return new MCWait;
	case S_WRITE:
		return new MCWrite;
	default:
		break;
	}

	MCStatement *t_new_statement;
	t_new_statement = MCModeNewCommand(which);
	if (t_new_statement != NULL)
		return t_new_statement;

	return new MCStatement;
}

MCExpression *MCN_new_function(int2 which)
{
	switch (which)
	{
	case F_ABS:
		return new MCAbsFunction;
	case F_ACOS:
		return new MCAcos;
	case F_ALIAS_REFERENCE:
		return new MCAliasReference;
	case F_ALTERNATE_LANGUAGES:
		return new MCAlternateLanguages;
	case F_ANNUITY:
		return new MCAnnuity;
	// JS-2013-06-19: [[ StatsFunctions ]] Constructor for 'arithmeticMean' (was average)
	case F_ARI_MEAN:
		return new MCArithmeticMean;
	case F_ARRAY_DECODE:
		return new MCArrayDecode;
	case F_ARRAY_ENCODE:
		return new MCArrayEncode;
	case F_ASIN:
		return new MCAsin;
	case F_ATAN2:
		return new MCAtan2;
	case F_ATAN:
		return new MCAtan;
	// JS-2013-06-19: [[ StatsFunctions ]] Constructor for 'averageDeviation'
	case F_AVG_DEV:
		return new MCAvgDev;
	case F_BACK_SCRIPTS:
		return new MCBackScripts;
	case F_BASE64_DECODE:
		return new MCBase64Decode;
	case F_BASE64_ENCODE:
		return new MCBase64Encode;
	case F_BASE_CONVERT:
		return new MCBaseConvert;
    // AL-2014-10-17: [[ BiDi ]] Returns the result of applying the bi-directional algorithm to text
    case F_BIDI_DIRECTION:
        return new MCBidiDirection;
	case F_BINARY_ENCODE:
		return new MCBinaryEncode;
	case F_BINARY_DECODE:
		return new MCBinaryDecode;
	case F_BUILD_NUMBER:
		return new MCBuildNumber;
    case F_BYTE_OFFSET:
        return new MCByteOffset;
	case F_CACHED_URLS:
		return new MCCachedUrls;
	case F_CAPS_LOCK_KEY:
		return new MCCapsLockKey;
	case F_CHAR_TO_NUM:
		return new MCCharToNum;
	case F_BYTE_TO_NUM:
		return new MCByteToNum;
	case F_CIPHER_NAMES:
		return new MCCipherNames;
	case F_CLICK_CHAR:
		return new MCClickChar;
	case F_CLICK_CHAR_CHUNK:
		return new MCClickCharChunk;
	case F_CLICK_CHUNK:
		return new MCClickChunk;
	case F_CLICK_FIELD:
		return new MCClickField;
	case F_CLICK_H:
		return new MCClickH;
	case F_CLICK_LINE:
		return new MCClickLine;
	case F_CLICK_LOC:
		return new MCClickLoc;
	case F_CLICK_STACK:
		return new MCClickStack;
	case F_CLICK_TEXT:
		return new MCClickText;
	case F_CLICK_V:
		return new MCClickV;
	case F_CLIPBOARD:
		return new MCClipboardFunc;
    case F_CODEPOINT_OFFSET:
        return new MCCodepointOffset;
    case F_CODEUNIT_OFFSET:
        return new MCCodeunitOffset;
	case F_COLOR_NAMES:
		return new MCColorNames;
    case F_COMMAND_ARGUMENTS:
        return new MCCommandArguments;
	case F_COMMAND_KEY:
		return new MCCommandKey;
    case F_COMMAND_NAME:
        return new MCCommandName;
	case F_COMMAND_NAMES:
		return new MCCommandNames;
	case F_COMPOUND:
		return new MCCompound;
	case F_COMPRESS:
		return new MCCompress;
	case F_CONSTANT_NAMES:
		return new MCConstantNames;
	case F_CONTROL_KEY:
		return new MCControlKey;
	case F_COPY_RESOURCE:
		return new MCCopyResource;
	case F_COS:
		return new MCCos;
	case F_DATE:
		return new MCDate;
	case F_DATE_FORMAT:
		return new MCDateFormat;
	case F_DECOMPRESS:
		return new MCDecompress;
	case F_DELETE_REGISTRY:
		return new MCDeleteRegistry;
	case F_DELETE_RESOURCE:
		return new MCDeleteResource;
	case F_DIRECTORIES:
		return new MCFileItems(false);
	case F_DISK_SPACE:
		return new MCDiskSpace;
	case F_DNS_SERVERS:
		return new MCDNSServers;
	case F_DRAG_DESTINATION:
		return new MCDragDestination;
	case F_DRAG_SOURCE:
		return new MCDragSource;
	case F_DRIVER_NAMES:
		return new MCDriverNames;
	case F_DRIVES:
		return new MCDrives;
	case F_DROP_CHUNK:
		return new MCDropChunk;
	case F_ENCRYPT:
		return new MCEncrypt;
	case F_ENVIRONMENT:
		return new MCEnvironment;
    case F_EVENT_CAPSLOCK_KEY:
        return new MCEventCapsLockKey;
    case F_EVENT_COMMAND_KEY:
        return new MCEventCommandKey;
    case F_EVENT_CONTROL_KEY:
        return new MCEventControlKey;
    case F_EVENT_OPTION_KEY:
        return new MCEventOptionKey;
    case F_EVENT_SHIFT_KEY:
        return new MCEventShiftKey;
	case F_EXISTS:
		return new MCExists;
	case F_EXP:
		return new MCExp;
	case F_EXP1:
		return new MCExp1;
	case F_EXP10:
		return new MCExp10;
	case F_EXP2:
		return new MCExp2;
	case F_EXTENTS:
		return new MCExtents;
	case F_FILES:
		return new MCFileItems(true);
	case F_FLUSH_EVENTS:
		return new MCFlushEvents;
	case F_FOCUSED_OBJECT:
		return new MCFocusedObject;
	case F_FONT_NAMES:
		return new MCFontNames;
	case F_FONT_LANGUAGE:
		return new MCFontLanguage;
	case F_FONT_SIZES:
		return new MCFontSizes;
	case F_FONT_STYLES:
		return new MCFontStyles;
	case F_FORMAT:
		return new MCFormat;
	case F_FOUND_CHUNK:
		return new MCFoundChunk;
	case F_FOUND_FIELD:
		return new MCFoundField;
	case F_FOUND_LINE:
		return new MCFoundLine;
	case F_FOUND_LOC:
		return new MCFoundLoc;
	case F_FOUND_TEXT:
		return new MCFoundText;
	case F_FRONT_SCRIPTS:
		return new MCFrontScripts;
	case F_FUNCTION_NAMES:
		return new MCFunctionNames;
	case F_GET_RESOURCE:
		return new MCGetResource;
	case F_GET_RESOURCES:
		return new MCGetResources;
	// JS-2013-06-19: [[ StatsFunctions ]] Constructor for 'geometricMean'
	case F_GEO_MEAN:
		return new MCGeometricMean;
	case F_GLOBAL_LOC:
		return new MCGlobalLoc;
	case F_GLOBALS:
		return new MCGlobals;
	// JS-2013-06-19: [[ StatsFunctions ]] Constructor for 'harmonicMean'
	case F_HAR_MEAN:
		return new MCHarmonicMean;
	case F_HAS_MEMORY:
		return new MCHasMemory;
	case F_HEAP_SPACE:
		return new MCHeapSpace;
	case F_HTTP_PROXY_FOR_URL:
		return new MCHTTPProxyForURL;
	case F_INTERRUPT:
		return new MCInterrupt;
	case F_HA:
		return new MCHostAddress;
	case F_HATON:
		return new MCHostAtoN;
	case F_HN:
		return new MCHostName;
	case F_HNTOA:
		return new MCHostNtoA;
	case F_INTERSECT:
		return new MCIntersect;
	case F_IS_NUMBER:
		return new MCIsNumber;
	case F_ISO_TO_MAC:
		return new MCIsoToMac;
	case F_ITEM_OFFSET:
		return new MCItemOffset;
	case F_KEYS:
		return new MCKeys;
	case F_KEYS_DOWN:
		return new MCKeysDown;
	case F_LENGTH:
		return new MCLength;
	case F_LICENSED:
		return new MCLicensed;
	case F_LINE_OFFSET:
		return new MCLineOffset;
	case F_LIST_REGISTRY:
		return new MCListRegistry;
	case F_LN1:
		return new MCLn1;
	case F_LN:
		return new MCLn;
	case F_LOCAL_LOC:
		return new MCLocalLoc;
	case F_LOCALS:
		return new MCLocals;
	case F_LONG_FILE_PATH:
		return new MCLongFilePath;
	case F_LOG10:
		return new MCLog10;
	case F_LOG2:
		return new MCLog2;
	case F_MACHINE:
		return new MCMachine;
	case F_MAC_TO_ISO:
		return new MCMacToIso;
	case F_MAIN_STACKS:
		return new MCMainStacks;
	case F_MATCH_CHUNK:
		return new MCMatchChunk;
	case F_MATCH_TEXT:
		return new MCMatchText;
	case F_MATRIX_MULTIPLY:
		return new MCMatrixMultiply;
	case F_MAX:
		return new MCMaxFunction;
	case F_MCI_SEND_STRING:
		return new MCMCISendString;
	case F_MD5_DIGEST:
		return new MCMD5Digest;
	case F_ME:
		return new MCMe;
	case F_MEDIAN:
		return new MCMedian;
	case F_MENUS:
		return new MCMenus;
	case F_MENU_OBJECT:
		return new MCMenuObject;
	case F_MERGE:
		return new MCMerge;
    case F_MESSAGE_DIGEST:
        return new MCMessageDigestFunc;
	case F_MILLISECS:
		return new MCMillisecs;
	case F_MIN:
		return new MCMinFunction;
	case F_MONTH_NAMES:
		return new MCMonthNames;
	case F_MOUSE:
		return new MCMouse;
	case F_MOUSE_CHAR:
		return new MCMouseChar;
	case F_MOUSE_CHAR_CHUNK:
		return new MCMouseCharChunk;
	case F_MOUSE_CHUNK:
		return new MCMouseChunk;
	case F_MOUSE_CLICK:
		return new MCMouseClick;
	case F_MOUSE_COLOR:
		return new MCMouseColor;
	case F_MOUSE_CONTROL:
		return new MCMouseControl;
	case F_MOUSE_H:
		return new MCMouseH;
	case F_MOUSE_LINE:
		return new MCMouseLine;
	case F_MOUSE_LOC:
		return new MCMouseLoc;
	case F_MOUSE_STACK:
		return new MCMouseStack;
	case F_MOUSE_TEXT:
		return new MCMouseText;
	case F_MOUSE_V:
		return new MCMouseV;
	case F_MOVIE:
		return new MCMovie;
	case F_MOVING_CONTROLS:
		return new MCMovingControls;
    case F_NATIVE_CHAR_TO_NUM:
        return new MCNativeCharToNum;
	case F_NUM_TO_CHAR:
		return new MCNumToChar;
    case F_NUM_TO_NATIVE_CHAR:
        return new MCNumToNativeChar;
    case F_NUM_TO_UNICODE_CHAR:
        return new MCNumToUnicodeChar;
	case F_NUM_TO_BYTE:
		return new MCNumToByte;
	case F_OFFSET:
		return new MCOffset;
	case F_OPEN_FILES:
		return new MCOpenFiles;
	case F_OPEN_PROCESSES:
		return new MCOpenProcesses;
	case F_OPEN_PROCESS_IDS:
		return new MCOpenProcessIds;
	case F_OPEN_SOCKETS:
		return new MCOpenSockets;
	case F_OPEN_STACKS:
		return new MCOpenStacks;
	case F_OPTION_KEY:
		return new MCOptionKey;
	// MW-2008-11-05: [[ Owner Reference ]] Create a new MCOwner function object for syntax of
	//   the form 'the owner of ...'. 
	case F_OWNER:
		return new MCOwner;
	case F_PA:
		return new MCPeerAddress;
    case F_PARAGRAPH_OFFSET:
        return new MCParagraphOffset;
	case F_PARAM:
		return new MCParam;
	case F_PARAMS:
		return new MCParams;
	case F_PARAM_COUNT:
		return new MCParamCount;
	case F_PENDING_MESSAGES:
		return new MCPendingMessages;
	case F_PLATFORM:
		return new MCPlatform;
	// JS-2013-06-19: [[ StatsFunctions ]] Constructor for 'populationStdDev'
	case F_POP_STD_DEV:
			return new MCPopulationStdDev;
	// JS-2013-06-19: [[ StatsFunctions ]] Constructor for 'populationVariance'
	case F_POP_VARIANCE:
		return new MCPopulationVariance;
	case F_PROCESSOR:
		return new MCProcessor;
	case F_PROCESS_ID:
		return new MCPid;
	case F_PROPERTY_NAMES:
		return new MCPropertyNames;
	case F_QT_EFFECTS:
		return new MCQTEffects;
	case F_QT_VERSION:
		return new MCQTVersion;
	case F_QUERY_REGISTRY:
		return new MCQueryRegistry;
	case F_RANDOM:
		return new MCRandom;
	case F_RECORD_COMPRESSION_TYPES:
		return new MCRecordCompressionTypes;
    case F_RECORD_FORMATS:
        return new MCRecordFormats;
	case F_RECORD_LOUDNESS:
		return new MCRecordLoudness;
	case F_REPLACE_TEXT:
		return new MCReplaceText;
	case F_RESULT:
		return new MCTheResult;
	case F_ROUND:
		return new MCRound;
	case F_SCREEN_COLORS:
		return new MCScreenColors;
	case F_SCREEN_DEPTH:
		return new MCScreenDepth;
	case F_SCREEN_LOC:
		return new MCScreenLoc;
	case F_SCREEN_NAME:
		return new MCScreenName;
	case F_SCREEN_RECT:
	case F_SCREEN_RECTS:
		return new MCScreenRect(which == F_SCREEN_RECTS);
	case F_SCREEN_TYPE:
		return new MCScreenType;
	case F_SCREEN_VENDOR:
		return new MCScreenVendor;
	case F_SCRIPT_LIMITS:
		return new MCScriptLimits;
	case F_SECONDS:
		return new MCSeconds;
	case F_SELECTED_BUTTON:
		return new MCSelectedButton;
	case F_SELECTED_CHUNK:
		return new MCSelectedChunk;
	case F_SELECTED_FIELD:
		return new MCSelectedField;
	case F_SELECTED_IMAGE:
		return new MCSelectedImage;
	case F_SELECTED_LINE:
		return new MCSelectedLine;
	case F_SELECTED_LOC:
		return new MCSelectedLoc;
	case F_SELECTED_OBJECT:
		return new MCSelectedObject;
	case F_SELECTED_TEXT:
		return new MCSelectedText;
    case F_SENTENCE_OFFSET:
        return new MCSentenceOffset;
	case F_SET_REGISTRY:
		return new MCSetRegistry;
	case F_SET_RESOURCE:
		return new MCSetResource;
	case F_SHA1_DIGEST:
		return new MCSHA1Digest;
	case F_SHELL:
		return new MCShell;
	case F_SHIFT_KEY:
		return new MCShiftKey;
	case F_SHORT_FILE_PATH:
		return new MCShortFilePath;
	case F_SIN:
		return new MCSin;
	// JS-2013-06-19: [[ StatsFunctions ]] Constructor for 'sampleStdDev' (was stdDev)
	case F_SMP_STD_DEV:
		return new MCSampleStdDev;
	// JS-2013-06-19: [[ StatsFunctions ]] Constructor for 'sampleVariance'
	case F_SMP_VARIANCE:
		return new MCSampleVariance;
	case F_SOUND:
		return new MCSound;
	case F_SPECIAL_FOLDER_PATH:
		return new MCSpecialFolderPath;
	case F_SQRT:
		return new MCSqrt;
	case F_STACKS:
		return new MCStacks;
	case F_STACK_SPACE:
		return new MCStackSpace;
	case F_STAT_ROUND:
		return new MCStatRound;
	case F_SUM:
		return new MCSum;
	case F_SYS_ERROR:
		return new MCSysError;
	case F_SYSTEM_VERSION:
		return new MCSystemVersion;
	case F_TAN:
		return new MCTan;
	case F_TARGET:
		return new MCTarget;
	case F_TEMP_NAME:
		return new MCTempName;
    case F_TEXT_DECODE:
        return new MCTextDecode;
    case F_TEXT_ENCODE:
        return new MCTextEncode;
	case F_TEXT_HEIGHT_SUM:
		return new MCTextHeightSum;
	case F_TICKS:
		return new MCTicks;
	case F_TIME:
		return new MCTheTime;
    case F_TOKEN_OFFSET:
        return new MCTokenOffset;
    case F_TOP_STACK:
		return new MCTopStack;
	case F_TO_LOWER:
		return new MCToLower;
	case F_TO_UPPER:
		return new MCToUpper;
	case F_TRANSPOSE:
		return new MCTranspose;
    case F_TRUEWORD_OFFSET:
        return new MCTrueWordOffset;
	case F_TRUNC:
		return new MCTrunc;
    case F_UNICODE_CHAR_TO_NUM:
        return new MCUnicodeCharToNum;
	// MDW-2014-08-23 : [[ feature_floor ]]
	case F_FLOOR:
		return new MCFloor;
	// MDW-2014-08-23 : [[ feature_floor ]]
	case F_CEIL:
		return new MCCeil;
	case F_VALUE:
		return new MCValue;
	case F_VARIABLES:
		return new MCVariables;
    case F_VECTOR_DOT_PRODUCT:
        return new MCVectorDotProduct;
	case F_VERSION:
		return new MCVersion;
	case F_WAIT_DEPTH:
		return new MCWaitDepth;
	case F_WEEK_DAY_NAMES:
		return new MCWeekDayNames;
	case F_WITHIN:
		return new MCWithin;
	case F_WORD_OFFSET:
		return new MCWordOffset;
	case F_UNI_DECODE:
		return new MCUniDecode;
	case F_UNI_ENCODE:
		return new MCUniEncode;
	case F_URL_DECODE:
		return new MCUrlDecode;
	case F_URL_ENCODE:
		return new MCUrlEncode;
	case F_URL_STATUS:
		return new MCUrlStatus;
	case F_RANDOM_BYTES:
		return new MCRandomBytes;
	case F_CONTROL_AT_LOC:
		return new MCControlAtLoc(false);
	case F_CONTROL_AT_SCREEN_LOC:
		return new MCControlAtLoc(true);
	// MW-2013-05-08: [[ Uuid ]] Constructor for uuid function.
	case F_UUID:
		return new MCUuidFunc;
    // MERG-2013-08-14: [[ MeasureText ]] Measure text relative to the effective font on an object
    case F_MEASURE_TEXT:
        return new MCMeasureText(false);
    case F_MEASURE_UNICODE_TEXT:
        return new MCMeasureText(true);
    case F_NORMALIZE_TEXT:
        return new MCNormalizeText;
    case F_CODEPOINT_PROPERTY:
        return new MCCodepointProperty;
    default:
		break;
	}

	MCExpression *t_new_function;
	t_new_function = MCModeNewFunction(which);

    // SN-2014-11-25: [[ Bug 14088 ]] A NULL pointer is returned if no function exists.
    //  (that avoids to get a MCFunction which does not implement eval_ctxt).
	return t_new_function;
}

MCExpression *MCN_new_operator(int2 which)
{
	switch (which)
	{
	case O_AND:
		return new MCAnd;
	case O_AND_BITS:
		return new MCAndBits;
	case O_CONCAT:
		return new MCConcat;
	case O_CONCAT_SPACE:
		return new MCConcatSpace;
	case O_CONTAINS:
		return new MCContains;
	case O_DIV:
		return new MCDiv;
	case O_EQ:
		return new MCEqual;
	case O_GE:
		return new MCGreaterThanEqual;
	case O_GROUPING:
		return new MCGrouping;
	case O_GT:
		return new MCGreaterThan;
	case O_IS:
		return new MCIs;
	case O_LE:
		return new MCLessThanEqual;
	case O_LT:
		return new MCLessThan;
	case O_MINUS:
		return new MCMinus;
	case O_MOD:
		return new MCMod;
	case O_WRAP:
		return new MCWrap;
	case O_NE:
		return new MCNotEqual;
	case O_NOT:
		return new MCNot;
	case O_NOT_BITS:
		return new MCNotBits;
	case O_OR:
		return new MCOr;
	case O_OR_BITS:
		return new MCOrBits;
	case O_OVER:
		return new MCOver;
	case O_PLUS:
		return new MCPlus;
	case O_POW:
		return new MCPow;
	case O_THERE:
		return new MCThere;
	case O_TIMES:
		return new MCTimes;
	case O_XOR_BITS:
		return new MCXorBits;
	case O_BEGINS_WITH:
		return new MCBeginsWith;
	case O_ENDS_WITH:
		return new MCEndsWith;
	default:
		return new MCExpression;
	}
}
