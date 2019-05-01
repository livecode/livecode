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

#ifndef __MC_EXECUTION_ERRORS__
#define __MC_EXECUTION_ERRORS__

// This file is processed automatically to produce the error strings that are
// embedded in the IDE and Server engines. Take care when editing the file and
// ensure that new errors are added in precisely this form:
//   <tab>// {EE-iiii} <message>
//   <tab>EE_<tag>
//   <return>

enum Exec_errors
{
	// NOT AN ERROR CODE
	EE_UNDEFINED,
	
	// {EE-0001} Handler: Running low on memory, script aborted
	EE_NO_MEMORY,
	
	// {EE-0002} recursionLimit: Recursion limit reached
	EE_RECURSION_LIMIT,
	
	// {EE-0003} abs: error in source expression
	EE_ABS_BADSOURCE,
	
	// {EE-0004} accept: bad expression
	EE_ACCEPT_BADEXP,
	
	// {EE-0005} aclip: playLoudness is not an integer
	EE_ACLIP_LOUDNESSNAN,
	
	// {EE-0006} acos: error in source expression
	EE_ACOS_BADSOURCE,
	
	// {EE-0007} numeric: domain error
	EE_MATH_DOMAIN,
	
	// {EE-0008} add: error in matrix operation
	EE_ADD_BADARRAY,
	
	// {EE-0009} add: destination has a bad format (numeric?)
	EE_ADD_BADDEST,
	
	// {EE-0010} add: error in source expression
	EE_ADD_BADSOURCE,
	
	// {EE-0011} add: can't set destination
	EE_ADD_CANTSET,
	
	// {EE-0012} add: can't add array to scalar
	EE_ADD_MISMATCH,
	
	// {EE-0013} aliasReference: error in file expression
	EE_ALIASREFERENCE_BADSOURCE,
	
	// {EE-0014} Operators and: error in left operand
	EE_AND_BADLEFT,
	
	// {EE-0015} Operators and: error in right operand
	EE_AND_BADRIGHT,
	
	// {EE-0016} Operators bitAnd: error in left operand
	EE_ANDBITS_BADLEFT,
	
	// {EE-0017} Operators bitAnd: error in right operand
	EE_ANDBITS_BADRIGHT,
	
	// {EE-0018} annuity: error in period expression
	EE_ANNUITY_BADPERIODS,
	
	// {EE-0019} annuity: error in rate expression
	EE_ANNUITY_BADRATE,
	
	// {EE-0020} answer: error in question expression
	EE_ANSWER_BADQUESTION,
	
	// {EE-0021} answer: error in response expression
	EE_ANSWER_BADRESPONSE,
	
	// {EE-0022} answer: error in title expression
	EE_ANSWER_BADTITLE,
	
	// {EE-0023} split: error in expression
	EE_ARRAYOP_BADEXP,
	
	// {EE-0024} arrowKey: error in direction expression
	EE_ARROWKEY_BADEXP,
	
	// {EE-0025} arrowKey: not a direction
	EE_ARROWKEY_NOTAKEY,
	
	// {EE-0026} asin: error in source expression
	EE_ASIN_BADSOURCE,
	
	// {EE-0027} UNUSED
	EE_UNUSED_0027,
	
	// {EE-0028} ask: error in question expression
	EE_ASK_BADQUESTION,
	
	// {EE-0029} ask: error in reply expression
	EE_ASK_BADREPLY,
	
	// {EE-0030} ask: error in title expression
	EE_ASK_BADTITLE,
	
	// {EE-0031} atan2: error in first expression
	EE_ATAN2_BADS1,
	
	// {EE-0032} atan2: error in second expression
	EE_ATAN2_BADS2,
	
	// {EE-0033} UNUSED
	EE_UNUSED_0033,
	
	// {EE-0034} atan: error in source expression
	EE_ATAN_BADSOURCE,
	
	// {EE-0035} UNUSED
	EE_UNUSED_0035,
	
	// {EE-0036} average: error in source expression
	EE_AVERAGE_BADSOURCE,
	
	// {EE-0037} base64Decode: error in source expression
	EE_BASE64DECODE_BADSOURCE,
	
	// {EE-0038} base64Encode: error in source expression
	EE_BASE64ENCODE_BADSOURCE,
	
	// {EE-0039} baseConvert: bad destination base
	EE_BASECONVERT_BADDESTBASE,
	
	// {EE-0040} baseConvert: error in source expression
	EE_BASECONVERT_BADSOURCE,
	
	// {EE-0041} baseConvert: bad source base
	EE_BASECONVERT_BADSOURCEBASE,
	
	// {EE-0042} baseConvert: can't convert this number
	EE_BASECONVERT_CANTCONVERT,
	
	// {EE-0043} baseConvert: destination is not base 10
	EE_BASECONVERT_NOTBASE10,
	
	// {EE-0044} beep: error in expression
	EE_BEEP_BADEXP,
	
	// {EE-0045} binaryDecode: destination is not a variable
	EE_BINARYD_BADDEST,
	
	// {EE-0046} binaryDecode: invalid data for parameter
	EE_BINARYD_BADFORMAT,
	
	// {EE-0047} binaryDecode: not enough parameters
	EE_BINARYD_BADPARAM,
	
	// {EE-0048} binaryDecode: error in source expression
	EE_BINARYD_BADSOURCE,
	
	// {EE-0049} binaryEncode: invalid data for parameter
	EE_BINARYE_BADFORMAT,
	
	// {EE-0050} binaryEncode: not enough parameters
	EE_BINARYE_BADPARAM,
	
	// {EE-0051} binaryEncode: error in source expression
	EE_BINARYE_BADSOURCE,
	
	// {EE-0052} Button: bad accelerator modifier
	EE_BUTTON_BADMODIFIER,
	
	// {EE-0053} Button: family is not an integer
	EE_BUTTON_FAMILYNAN,
	
	// {EE-0054} Button: menuButton is not an integer
	EE_BUTTON_MENUBUTTONNAN,
	
	// {EE-0055} Button: menuHistory is not an integer
	EE_BUTTON_MENUHISTORYNAN,
	
	// {EE-0056} Button: menuLines is not an integer
	EE_BUTTON_MENULINESNAN,
	
	// {EE-0057} Button: mnemonic is not an integer
	EE_BUTTON_MNEMONICNAN,
	
	// {EE-0058} cancel: message id is not an integer
	EE_CANCEL_IDNAN,
	
	// {EE-0059} charToNum: error in source expression
	EE_CHARTONUM_BADSOURCE,
	
	// {EE-0060} choose: error in expression
	EE_CHOOSE_BADEXP,
	
	// {EE-0061} choose: not a tool
	EE_CHOOSE_BADTOOL,
	
	// {EE-0062} Chunk: error in background expression
	EE_CHUNK_BADBACKGROUNDEXP,
	
	// {EE-0063} Chunk: error in card expression
	EE_CHUNK_BADCARDEXP,
	
	// {EE-0064} Chunk: error in character range
	EE_CHUNK_BADCHARMARK,
	
	// {EE-0065} Chunk: container is not a button or field
	EE_CHUNK_BADCONTAINER,
	
	// {EE-0066} Chunk: error in chunk expression
	EE_CHUNK_BADEXPRESSION,
	
	// {EE-0067} Chunk: error in item range
	EE_CHUNK_BADITEMMARK,
	
	// {EE-0068} Chunk: error in line range
	EE_CHUNK_BADLINEMARK,
	
	// {EE-0069} Chunk: error in object expression
	EE_CHUNK_BADOBJECTEXP,
	
	// {EE-0070} Chunk: error in range end expression
	EE_CHUNK_BADRANGEEND,
	
	// {EE-0071} Chunk: error in range start expression
	EE_CHUNK_BADRANGESTART,
	
	// {EE-0072} Chunk: error in stack expression
	EE_CHUNK_BADSTACKEXP,
	
	// {EE-0073} Chunk: error in text string
	EE_CHUNK_BADTEXT,
	
	// {EE-0074} Chunk: can't separate tokens
	EE_CHUNK_BADTOKENMARK,
	
	// {EE-0075} Chunk: can't separate words
	EE_CHUNK_BADWORDMARK,
	
	// {EE-0076} Chunk: can't delete object
	EE_CHUNK_CANTDELETEOBJECT,
	
	// {EE-0077} Chunk: can't find object
	EE_CHUNK_CANTFINDOBJECT,
	
	// {EE-0078} Chunk: can't get object attributes
	EE_CHUNK_CANTGETATTS,
	
	// {EE-0079} Chunk: can't get value of destination container
	EE_CHUNK_CANTGETDEST,
	
	// {EE-0080} Chunk: can't get number
	EE_CHUNK_CANTGETNUMBER,
	
	// {EE-0081} Chunk: can't get source string
	EE_CHUNK_CANTGETSOURCE,
	
	// {EE-0082} Chunk: can't get substring
	EE_CHUNK_CANTGETSUBSTRING,
	
	// {EE-0083} Chunk: can't find substring
	EE_CHUNK_CANTMARK,
	
	// {EE-0084} Chunk: can't set attributes
	EE_CHUNK_CANTSETATTS,
	
	// {EE-0085} Chunk: can't store to destination container
	EE_CHUNK_CANTSETDEST,
	
	// {EE-0086} Chunk: can't set as a number
	EE_CHUNK_CANTSETN,
	
	// {EE-0087} Chunk: can't find background
	EE_CHUNK_NOBACKGROUND,
	
	// {EE-0088} Chunk: can't find card
	EE_CHUNK_NOCARD,
	
	// {EE-0089} Chunk: no such object
	EE_CHUNK_NOOBJECT,
	
	// {EE-0090} Chunk: can't set property
	EE_CHUNK_NOPROP,
	
	// {EE-0091} Chunk: can't find stack
	EE_CHUNK_NOSTACK,
	
	// {EE-0092} Chunk: no target found
	EE_CHUNK_NOTARGET,
	
	// {EE-0093} Chunk: can't select object that isn't open
	EE_CHUNK_NOTOPEN,
	
	// {EE-0094} Chunk: source is not a container
	EE_CHUNK_OBJECTNOTCONTAINER,
	
	// {EE-0095} Chunk: can't find object to store into
	EE_CHUNK_SETCANTGETBOJECT,
	
	// {EE-0096} Chunk: can't get source from container
	EE_CHUNK_SETCANTGETDEST,
	
	// {EE-0097} Chunk: destination is not a container
	EE_CHUNK_SETNOTACONTAINER,
	
	// {EE-0098} click: script aborted
	EE_CLICK_ABORT,
	
	// {EE-0099} click: expression is not a button number
	EE_CLICK_BADBUTTON,
	
	// {EE-0100} click: error in point expression
	EE_CLICK_BADLOCATION,
	
	// {EE-0101} click: expression is not a point
	EE_CLICK_NAP,
	
	// {EE-0102} click: stack is not open
	EE_CLICK_STACKNOTOPEN,
	
	// {EE-0103} clone: error in name expression
	EE_CLONE_BADNAME,
	
	// {EE-0104} clone: can't clone this object
	EE_CLONE_CANTCLONE,
	
	// {EE-0105} clone: stack is locked
	EE_CLONE_LOCKED,
	
	// {EE-0106} clone: can't find object to clone
	EE_CLONE_NOTARGET,
	
	// {EE-0107} close: error in name expression
	EE_CLOSE_BADNAME,
	
	// {EE-0108} close: can't find stack
	EE_CLOSE_NOOBJ,
	
	// {EE-0109} color: error setting selectedColor
	EE_COLOR_BADSELECTEDCOLOR,
	
	// {EE-0110} compact: can't find stack to save
	EE_COMPACT_NOTARGET,
	
	// {EE-0111} compact: object is not a stack
	EE_COMPACT_NOTASTACK,
	
	// {EE-0112} compound: error in periods expression
	EE_COMPOUND_BADPERIODS,
	
	// {EE-0113} compound: error in rate expression
	EE_COMPOUND_BADRATE,
	
	// {EE-0114} compress: error in source expression
	EE_COMPRESS_BADSOURCE,
	
	// {EE-0115} compress: error occurred during compression
	EE_COMPRESS_ERROR,
	
	// {EE-0116} Operators &&: error in left operand
	EE_CONCATSPACE_BADLEFT,
	
	// {EE-0117} Operators &&: error in right operand
	EE_CONCATSPACE_BADRIGHT,
	
	// {EE-0118} Operators &: error in left operand
	EE_CONCAT_BADLEFT,
	
	// {EE-0119} Operators &: error in right operand
	EE_CONCAT_BADRIGHT,
	
	// {EE-0120} Operators contains: error in left operand
	EE_CONTAINS_BADLEFT,
	
	// {EE-0121} Operators contains: error in right operand
	EE_CONTAINS_BADRIGHT,
	
	// {EE-0122} convert: can't read from container
	EE_CONVERT_CANTGET,
	
	// {EE-0123} convert: can't set container
	EE_CONVERT_CANTSET,
	
	// {EE-0124} copy: invalid destination object
	EE_CLIPBOARD_BADDEST,
	
	// {EE-0125} copy: can't find destination object
	EE_CLIPBOARD_NODEST,
	
	// {EE-0126} copy: can't copy source object
	EE_CLIPBOARD_BADOBJ,
	
	// {EE-0127} copy: can't find source object
	EE_COPY_NOOBJ,
	
	// {EE-0128} copy: stack is password protected
	EE_COPY_PASSWORD,
	
	// {EE-0129} cos: error in source expression
	EE_COS_BADSOURCE,
	
	// {EE-0130} UNUSED
	EE_UNUSED_0130,
	
	// {EE-0131} create: error in bad parent or background expression
	EE_CREATE_BADBGORCARD,
	
	// {EE-0132} create: error in name expression
	EE_CREATE_BADEXP,
	
	// {EE-0133} create: error in file name expression
	EE_CREATE_BADFILEEXP,
	
	// {EE-0134} create: stack is locked (cantModify)
	EE_CREATE_LOCKED,
	
	// {EE-0135} crop: error in image expression
	EE_CROP_NOIMAGE,
	
	// {EE-0136} crop: object is not an image
	EE_CROP_NOTIMAGE,
	
	// {EE-0137} crop: error in rectangle expression
	EE_CROP_CANTGETRECT,
	
	// {EE-0138} crop: expression is not a rectangle
	EE_CROP_NAR,
	
	// {EE-0139} cut: can't find or copy object
	EE_CUT_NOOBJ,
	
	// {EE-0140} decompress: error in source expression
	EE_DECOMPRESS_BADSOURCE,
	
	// {EE-0141} decompress: string is not compressed data
	EE_DECOMPRESS_NOTCOMPRESSED,
	
	// {EE-0142} decompress: error during decompression
	EE_DECOMPRESS_ERROR,
	
	// {EE-0143} delete: error in file or url name expression
	EE_DELETE_BADFILEEXP,
	
	// {EE-0144} delete: can't find object
	EE_DELETE_NOOBJ,
	
	// {EE-0145} disable: can't find object
	EE_DISABLE_NOOBJ,
	
	// {EE-0146} Stack: stack has not been given a file name
	EE_DISPATCH_NOFILEYET,
	
	// {EE-0147} divide: error in matrix operation
	EE_DIVIDE_BADARRAY,
	
	// {EE-0148} divide: destination has a bad format (numeric?)
	EE_DIVIDE_BADDEST,
	
	// {EE-0149} divide: error in source expression
	EE_DIVIDE_BADSOURCE,
	
	// {EE-0150} divide: can't set destination
	EE_DIVIDE_CANTSET,
	
	// {EE-0151} divide: can't divide scalar by array
	EE_DIVIDE_MISMATCH,
	
	// {EE-0152} numeric: range error (overflow)
	EE_MATH_RANGE,
	
	// {EE-0153} numeric: divide by zero
	EE_MATH_ZERO,
	
	// {EE-0154} Operators div: error in matrix operation
	EE_DIV_BADARRAY,
	
	// {EE-0155} Operators div: error in left operand
	EE_DIV_BADLEFT,
	
	// {EE-0156} Operators div: error in right operand
	EE_DIV_BADRIGHT,
	
	// {EE-0157} Operators div: can't divide scalar by matrix
	EE_DIV_MISMATCH,
	
	// {EE-0158} UNUSED
	EE_UNUSED_0158,
	
	// {EE-0159} UNUSED
	EE_UNUSED_0159,
	
	// {EE-0160} do: aborted
	EE_DO_ABORT,
	
	// {EE-0161} do: error in source expression
	EE_DO_BADCOMMAND,
	
	// {EE-0162} do: error in statement
	EE_DO_BADEXEC,
	
	// {EE-0163} do: error in expression
	EE_DO_BADEXP,
	
	// {EE-0164} do: error in language expression
	EE_DO_BADLANG,
	
	// {EE-0165} do: unexpected end of line in source expression
	EE_DO_BADLINE,
	
	// {EE-0166} do: can't find command
	EE_DO_NOCOMMAND,
	
	// {EE-0167} do: not a command
	EE_DO_NOTCOMMAND,
	
	// {EE-0168} do: license limit exceeded
	EE_DO_NOTLICENSED,
	
	// {EE-0169} doMenu: error in expression
	EE_DOMENU_BADEXP,
	
	// {EE-0170} doMenu: don't know this menu item
	EE_DOMENU_DONTKNOW,
	
	// {EE-0171} drag: script aborted
	EE_DRAG_ABORT,
	
	// {EE-0172} drag: bad "button" number
	EE_DRAG_BADBUTTON,
	
	// {EE-0173} drag: bad end point expression
	EE_DRAG_BADENDLOC,
	
	// {EE-0174} drag: end point is not a point
	EE_DRAG_ENDNAP,
	
	// {EE-0175} drag: bad start point expression
	EE_DRAG_BADSTARTLOC,
	
	// {EE-0176} drag: start point is not a point
	EE_DRAG_STARTNAP,
	
	// {EE-0177} driverNames: error in type expression
	EE_DRIVERNAMES_BADTYPE,
	
	// {EE-0178} drives: error in type expression
	EE_DRIVES_BADTYPE,
	
	// {EE-0179} edit: can't find object
	EE_EDIT_BADTARGET,
	
	// {EE-0180} encrypt: error in source expression
	EE_ENCRYPT_BADSOURCE,
	
	// {EE-0181} Operators =: error in operand
	EE_EQUAL_OPS,
	
	// {EE-0182} exp10: error in source expression
	EE_EXP10_BADSOURCE,
	
	// {EE-0183} UNUSED
	EE_UNUSED_0183,
	
	// {EE-0184} exp1: error in source expression
	EE_EXP1_BADSOURCE,
	
	// {EE-0185} UNUSED
	EE_UNUSED_0185,
	
	// {EE-0186} exp2: error in source expression
	EE_EXP2_BADSOURCE,
	
	// {EE-0187} UNUSED
	EE_UNUSED_0187,
	
	// {EE-0188} export: error in file (or mask file) name expression
	EE_EXPORT_BADNAME,
	
	// {EE-0189} export: can't open file (or mask file)
	EE_EXPORT_CANTOPEN,
	
	// {EE-0190} export: can't write to file, mask file, or container
	EE_EXPORT_CANTWRITE,
	
	// {EE-0191} export: no image selected, or image not open
	EE_EXPORT_NOSELECTED,
	
	// {EE-0192} export: selected object is not an image
	EE_EXPORT_NOTANIMAGE,
	
	// {EE-0193} Expression: error in numeric factor
	EE_EXPRESSION_NFACTOR,
	
	// {EE-0194} Expression: error in string factor
	EE_EXPRESSION_SFACTOR,
	
	// {EE-0195} exp: error in source expression
	EE_EXP_BADSOURCE,
	
	// {EE-0196} UNUSED
	EE_UNUSED_0196,
	
	// {EE-0197} extents: error in variable expression
	EE_EXTENTS_BADSOURCE,
	
	// {EE-0198} Factor: error in left operand
	EE_FACTOR_BADLEFT,
	
	// {EE-0199} Factor: error in right operand
	EE_FACTOR_BADRIGHT,
	
	// {EE-0200} Field: bad text attributes
	EE_FIELD_BADTEXTATTS,
	
	// {EE-0201} Field: hilitedLine is not an integer
	EE_FIELD_HILITEDNAN,
	
	// {EE-0202} Field: scrollbarWidth is not an integer
	EE_FIELD_SCROLLBARWIDTHNAN,
	
	// {EE-0203} Field: shift is not an integer
	EE_FIELD_SHIFTNAN,
	
	// {EE-0204} Field: tabstops is not a positive integer
	EE_FIELD_TABSNAN,
	
	// {EE-0205} files: no permission to list files or directories
	EE_FILES_NOPERM,
	
	// {EE-0206} filter: bad source string
	EE_FILTER_CANTGET,
	
	// {EE-0207} filter: bad pattern string
	EE_FILTER_CANTGETPATTERN,
	
	// {EE-0208} filter: can't set destination
	EE_FILTER_CANTSET,
	
	// {EE-0209} find: bad source string
	EE_FIND_BADSTRING,
	
	// {EE-0210} flip: can't find image
	EE_FLIP_NOIMAGE,
	
	// {EE-0211} flip: object is not an editable image
	EE_FLIP_NOTIMAGE,
	
	// {EE-0212} flushEvents: bad event type
	EE_FLUSHEVENTS_BADTYPE,
	
	// {EE-0213} focus: not a valid control
	EE_FOCUS_BADOBJECT,
	
	// {EE-0214} fontNames: error in type expression
	EE_FONTNAMES_BADTYPE,
	
	// {EE-0215} fontSizes: bad font name
	EE_FONTSIZES_BADFONTNAME,
	
	// {EE-0216} fontStyles: bad font name
	EE_FONTSTYLES_BADFONTNAME,
	
	// {EE-0217} fontStyles: bad font size
	EE_FONTSTYLES_BADFONTSIZE,
	
	// {EE-0218} format: bad format string or parameter mismatch
	EE_FORMAT_BADSOURCE,
	
	// {EE-0219} Function: error in function handler
	EE_FUNCTION_BADFUNCTION,
	
	// {EE-0220} Function: error in source expression
	EE_FUNCTION_BADSOURCE,
	
	// {EE-0221} Function: source is not a number
	EE_FUNCTION_CANTEVALN,
	
	// {EE-0222} Function: is not a number
	EE_FUNCTION_NAN,
	
	// {EE-0223} get: error in expression
	EE_GET_BADEXP,
	
	// {EE-0224} get: can't set destination
	EE_GET_CANTSET,
	
	// {EE-0225} globalLoc: coordinate is not a point
	EE_GLOBALLOC_NAP,
	
	// {EE-0226} go: error in background expression
	EE_GO_BADBACKGROUNDEXP,
	
	// {EE-0227} go: error in card expression
	EE_GO_BADCARDEXP,
	
	// {EE-0228} go: error in stack expression
	EE_GO_BADSTACKEXP,
	
	// {EE-0229} go: error in window expression
	EE_GO_BADWINDOWEXP,
	
	// {EE-0230} go: can't attach menu to this object type
	EE_GO_CANTATTACH,
	
	// {EE-0231} go: can't find destination
	EE_GO_NODEST,
	
	// {EE-0232} grab: can't find object
	EE_GRAB_NOOBJ,
	
	// {EE-0233} graphic: not an integer
	EE_GRAPHIC_NAN,
	
	// {EE-0234} Operators >=: error in operands
	EE_GREATERTHANEQUAL_OPS,
	
	// {EE-0235} Operators >: error in operands
	EE_GREATERTHAN_OPS,
	
	// {EE-0236} Group: backSize is not a point
	EE_GROUP_BACKSIZENAP,
	
	// {EE-0237} Group: hilitedButton is not an integer
	EE_GROUP_HILITEDNAN,
	
	// {EE-0238} Group: bad object type
	EE_GROUP_NOOBJ,
	
	// {EE-0239} Operators (): error in right operand
	EE_GROUPING_BADRIGHT,
	
	// {EE-0240} Handler: aborted
	EE_HANDLER_ABORT,
	
	// {EE-0241} Handler: error in statement
	EE_HANDLER_BADSTATEMENT,
	
	// {EE-0242} Handler: error in parameter expression
	EE_HANDLER_BADPARAM,
	
	// {EE-0243} Handler: not a valid parameter index
	EE_HANDLER_BADPARAMINDEX,
	
	// {EE-0244} hasMemory: bad amount expression
	EE_HASMEMORY_BADAMOUNT,
	
	// {EE-0245} hide: error in visual effect expression
	EE_HIDE_BADEFFECT,
	
	// {EE-0246} hide: can't find object
	EE_HIDE_NOOBJ,
	
	// {EE-0247} hostAddress: error in socket expression
	EE_HOSTADDRESS_BADSOCKET,
	
	// {EE-0248} hostAddressToName: error in address expression
	EE_HOSTATON_BADADDRESS,
	
	// {EE-0249} hostName: error in name expression
	EE_HOSTNAME_BADNAME,
	
	// {EE-0250} hostNameToAddress: error in name expression
	EE_HOSTNTOA_BADNAME,
	
	// {EE-0251} if-then: aborted
	EE_IF_ABORT,
	
	// {EE-0252} if-then: error in condition expression
	EE_IF_BADCOND,
	
	// {EE-0253} if-then: error in statement
	EE_IF_BADSTATEMENT,
	
	// {EE-0254} Image: bad pixmap id
	EE_IMAGE_BADPIXMAP,
	
	// {EE-0255} Image: hotspot is not an integer
	EE_IMAGE_HOTNAP,
	
	// {EE-0256} Image: id is not an integer
	EE_OBJECT_IDNAN,
	
	// {EE-0257} Image: id is already in use by another object
	EE_OBJECT_IDINUSE,
	
	// {EE-0258} Image: image must be open to set id
	EE_IMAGE_NOTOPEN,
	
	// {EE-0259} Image: hotSpot x is not an integer
	EE_IMAGE_XHOTNAN,
	
	// {EE-0260} Image: hotSpot y is not an integer
	EE_IMAGE_YHOTNAN,
	
	// {EE-0261} import: error in expression
	EE_IMPORT_BADNAME,
	
	// {EE-0262} import: can't open file, mask file or display
	EE_IMPORT_CANTOPEN,
	
	// {EE-0263} import: can't read file, mask file or display
	EE_IMPORT_CANTREAD,
	
	// {EE-0264} import: destination stack is locked (cantModify)
	EE_IMPORT_LOCKED,
	
	// {EE-0265} insert: can't find object
	EE_INSERT_BADTARGET,
	
	// {EE-0266} insert: license limit exceeded
	EE_INSERT_NOTLICENSED,
	
	// {EE-0267} intersect: two objects required
	EE_INTERSECT_NOOBJECT,
	
	// {EE-0268} Operators is: error in left operand
	EE_IS_BADLEFT,
	
	// {EE-0269} Operators is: error in right operand
	EE_IS_BADRIGHT,
	
	// {EE-0270} Operators is: can't compare operands
	EE_IS_BADOPS,
	
	// {EE-0271} Operators is: left operand of 'within' is not a point
	EE_IS_WITHINNAP,
	
	// {EE-0272} Operators is: right operand of 'within' is not a rectangle
	EE_IS_WITHINNAR,
	
	// {EE-0273} isNumber: error in source expression
	EE_ISNUMBER_BADSOURCE,
	
	// {EE-0274} isoToMac: error source expression
	EE_ISOTOMAC_BADSOURCE,
	
	// {EE-0275} Operators ,: error in left operand
	EE_ITEM_BADLEFT,
	
	// {EE-0276} Operators ,: error in right operand
	EE_ITEM_BADRIGHT,
	
	// {EE-0277} keys: parameter is not a variable
	EE_KEYS_BADSOURCE,
	
	// {EE-0278} kill: no such process
	EE_KILL_BADNAME,
	
	// {EE-0279} kill: bad number
	EE_KILL_BADNUMBER,
	
	// {EE-0280} launch: error in application expression
	EE_LAUNCH_BADAPPEXP,
	
	// {EE-0281} length: error in source expression
	EE_LENGTH_BADSOURCE,
	
	// {EE-0282} Operators <=: error in operands
	EE_LESSTHANEQUAL_OPS,
	
	// {EE-0283} Operators <: error in operands
	EE_LESSTHAN_OPS,
	
	// {EE-0284} ln1: error in source expression
	EE_LN1_BADSOURCE,
	
	// {EE-0285} UNUSED
	EE_UNUSED_0285,
	
	// {EE-0286} ln: error in source expression
	EE_LN_BADSOURCE,
	
	// {EE-0287} UNUSED
	EE_UNUSED_0287,
	
	// {EE-0288} load: error in url expression
	EE_LOAD_BADURLEXP,
	
	// {EE-0289} load: error in message expression
	EE_LOAD_BADMESSAGEEXP,
	
	// {EE-0290} localLoc: coordinate is not a point
	EE_LOCALLOC_NAP,
	
	// {EE-0291} log10: error in source expression
	EE_LOG10_BADSOURCE,
	
	// {EE-0292} UNUSED
	EE_UNUSED_0292,
	
	// {EE-0293} log2: error in source expression
	EE_LOG2_BADSOURCE,
	
	// {EE-0294} UNUSED
	EE_UNUSED_0294,
	
	// {EE-0295} longFilePath: error in file expression
	EE_LONGFILEPATH_BADSOURCE,
	
	// {EE-0296} macToIso: error source expression
	EE_MACTOISO_BADSOURCE,
	
	// {EE-0297} mark: bad card expression
	EE_MARK_BADCARD,
	
	// {EE-0298} mark: error in find expression
	EE_MARK_BADSTRING,
	
	// {EE-0299} matchChunk: can't set destination variable
	EE_MATCH_BADDEST,
	
	// {EE-0300} matchChunk: bad or missing parameter
	EE_MATCH_BADPARAM,
	
	// {EE-0301} matchChunk: error in pattern expression
	EE_MATCH_BADPATTERN,
	
	// {EE-0302} matchChunk: error in source expression
	EE_MATCH_BADSOURCE,
	
	// {EE-0303} matrix: range error in matrix operation
	EE_MATRIX_RANGE,
	
	// {EE-0304} matrixMultiply: error in source expression
	EE_MATRIXMULT_BADSOURCE,
	
	// {EE-0305} matrixMultiply: can't multiply these arrays
	EE_MATRIXMULT_MISMATCH,
	
	// {EE-0306} max: error in source expression
	EE_MAX_BADSOURCE,
	
	// {EE-0307} MCISendString: error in source expression
	EE_MCISENDSTRING_BADSOURCE,
	
	// {EE-0308} MD5digest: error in source expression
	EE_MD5DIGEST_BADSOURCE,
	
	// {EE-0309} median: error in source expression
	EE_MEDIAN_BADSOURCE,
	
	// {EE-0310} merge: error in source expression
	EE_MERGE_BADSOURCE,
	
	// {EE-0311} Operators -: can't subtract array from scalar
	EE_MINUS_BADARRAY,
	
	// {EE-0312} Operators -: error in left operand
	EE_MINUS_BADLEFT,
	
	// {EE-0313} Operators -: error in right operand
	EE_MINUS_BADRIGHT,
	
	// {EE-0314} Operators -: range error (overflow) in array operation
	EE_MINUS_MISMATCH,
	
	// {EE-0315} UNUSED
	EE_UNUSED_0315,
	
	// {EE-0316} min: error in source expression
	EE_MIN_BADSOURCE,
	
	// {EE-0317} Operators mod: error in matrix operation
	EE_MOD_BADARRAY,
	
	// {EE-0318} Operators mod: error in left operand
	EE_MOD_BADLEFT,
	
	// {EE-0319} Operators mod: error in right operand
	EE_MOD_BADRIGHT,
	
	// {EE-0320} Operators mod: can't divide scalar by matrix
	EE_MOD_MISMATCH,
	
	// {EE-0321} UNUSED
	EE_UNUSED_0321,
	
	// {EE-0322} UNUSED
	EE_UNUSED_0322,
	
	// {EE-0323} mouse: error in source expression
	EE_MOUSE_BADSOURCE,
	
	// {EE-0324} move: script aborted
	EE_MOVE_ABORT,
	
	// {EE-0325} move: can't find object
	EE_MOVE_BADOBJECT,
	
	// {EE-0326} move: bad end point expression
	EE_MOVE_BADENDLOC,
	
	// {EE-0327} move: bad duration expression
	EE_MOVE_BADDURATION,
	
	// {EE-0328} move: duration is not a number
	EE_MOVE_DURATIONNAN,
	
	// {EE-0329} move: end point is not a point
	EE_MOVE_ENDNAP,
	
	// {EE-0330} move: bad start point expression
	EE_MOVE_BADSTARTLOC,
	
	// {EE-0331} move: start point is not a point
	EE_MOVE_STARTNAP,
	
	// {EE-0332} multiply: error in matrix operation
	EE_MULTIPLY_BADARRAY,
	
	// {EE-0333} multiply: destination has a bad format (numeric?)
	EE_MULTIPLY_BADDEST,
	
	// {EE-0334} multiply: error in source expression
	EE_MULTIPLY_BADSOURCE,
	
	// {EE-0335} multiply: can't set destination
	EE_MULTIPLY_CANTSET,
	
	// {EE-0336} multiply: can't multiply scalar by array
	EE_MULTIPLY_MISMATCH,
	
	// {EE-0337} UNUSED
	EE_UNUSED_0337,
	
	// {EE-0338} Operators <>: error in operands
	EE_NOTEQUAL_OPS,
	
	// {EE-0339} Operators not: error in right operand
	EE_NOT_BADRIGHT,
	
	// {EE-0340} Operators bitNot: error in right operand
	EE_NOTBITS_BADRIGHT,
	
	// {EE-0341} numToChar: error in source expression
	EE_NUMTOCHAR_BADSOURCE,
	
	// {EE-0342} Object: bad textAlign expression
	EE_OBJECT_BADALIGN,
	
	// {EE-0343} Object: unknown color
	EE_OBJECT_BADCOLOR,
	
	// {EE-0344} Object: error in colors
	EE_OBJECT_BADCOLORS,
	
	// {EE-0345} Object: can't set layer (card not open, or control in group)
	EE_OBJECT_BADRELAYER,
	
	// {EE-0346} Object: not a textStyle
	EE_OBJECT_BADSTYLE,
	
	// {EE-0347} Object: stack locked, or object's script is executing
	EE_OBJECT_CANTREMOVE,
	
	// {EE-0348} Object: object does not have this property
	EE_OBJECT_GETNOPROP,
	
	// {EE-0349} Object: height is not an integer
	EE_OBJECT_LAYERNAN,
	
	// {EE-0350} Object: layer is not an integer
	EE_OBJECT_LOCNAP,
	
	// {EE-0351} Object: margin is not an integer
	EE_OBJECT_MARGINNAN,
	
	// {EE-0352} Object: value is not a boolean (true or false)
	EE_OBJECT_NAB,
	
	// {EE-0353} Object Name:
	EE_OBJECT_NAME,
	
	// {EE-0354} Object: property is not an integer
	EE_OBJECT_NAN,
	
	// {EE-0355} Object: coordinate is not a point
	EE_OBJECT_NAP,
	
	// {EE-0356} Object: rectangle does not have 4 points
	EE_OBJECT_NAR,
	
	// {EE-0357} Object: no Home stack
	EE_OBJECT_NOHOME,
	
	// {EE-0358} Object: pixel value is not an integer
	EE_OBJECT_PIXELNAN,
	
	// {EE-0359} Object: pixmap is not an integer
	EE_OBJECT_PIXMAPNAN,
	
	// {EE-0360} Object: can't set script while it is executing
	EE_OBJECT_SCRIPTEXECUTING,
	
	// {EE-0361} Object: can't set this property
	EE_OBJECT_SETNOPROP,
	
	// {EE-0362} Object: textheight is not an integer
	EE_OBJECT_TEXTHEIGHTNAN,
	
	// {EE-0363} Object: textsize is not an integer
	EE_OBJECT_TEXTSIZENAN,
	
	// {EE-0364} offset: error in start offset expression
	EE_OFFSET_BADOFFSET,
	
	// {EE-0365} offset: error in part expression
	EE_OFFSET_BADPART,
	
	// {EE-0366} offset: error in whole expression
	EE_OFFSET_BADWHOLE,
	
	// {EE-0367} open: error in message expression
	EE_OPEN_BADMESSAGE,
	
	// {EE-0368} open: error in name expression
	EE_OPEN_BADNAME,
	
	// {EE-0369} open: no permission to open files or processes
	EE_OPEN_NOPERM,
	
	// {EE-0370} Operators or: error in left operand
	EE_OR_BADLEFT,
	
	// {EE-0371} Operators or: error in right operand
	EE_OR_BADRIGHT,
	
	// {EE-0372} Operators bitOr: error in left operand
	EE_ORBITS_BADLEFT,
	
	// {EE-0373} Operators bitOr: error in right operand
	EE_ORBITS_BADRIGHT,
	
	// {EE-0374} Operators /: error in matrix operation
	EE_OVER_BADARRAY,
	
	// {EE-0375} Operators /: error in left operand
	EE_OVER_BADLEFT,
	
	// {EE-0376} Operators /: error in right operand
	EE_OVER_BADRIGHT,
	
	// {EE-0377} Operators /: can't divide scalar by matrix
	EE_OVER_MISMATCH,
	
	// {EE-0378} UNUSED
	EE_UNUSED_0378,
	
	// {EE-0379} UNUSED
	EE_UNUSED_0379,
	
	// {EE-0380} param: error in expression
	EE_PARAM_BADEXP,
	
	// {EE-0381} param: bad parameter index
	EE_PARAM_BADINDEX,
	
	// {EE-0382} param: error in source expression
	EE_PARAM_BADSOURCE,
	
	// {EE-0383} param: is not a number
	EE_PARAM_NAN,
	
	// {EE-0384} paste: stack is locked (cantModify)
	EE_PASTE_LOCKED,
	
	// {EE-0385} peerAddress: error in socket expression
	EE_PEERADDRESS_BADSOCKET,
	
	// {EE-0386} place: group is not in this stack or is already on this card
	EE_PLACE_ALREADY,
	
	// {EE-0387} place: can't find group
	EE_PLACE_NOBACKGROUND,
	
	// {EE-0388} place: can't find card
	EE_PLACE_NOCARD,
	
	// {EE-0389} place: source is not a group, or cannot be used as a background
	EE_PLACE_NOTABACKGROUND,
	
	// {EE-0390} place: destination is not a card
	EE_PLACE_NOTACARD,
	
	// {EE-0391} play: can't get sound or movie name
	EE_PLAY_BADCLIP,
	
	// {EE-0392} play: bad movie location
	EE_PLAY_BADLOC,
	
	// {EE-0393} play: bad movie options
	EE_PLAY_BADOPTIONS,
	
	// {EE-0394} Operators +: error in left operand
	EE_PLUS_BADLEFT,
	
	// {EE-0395} Operators +: error in right operand
	EE_PLUS_BADRIGHT,
	
	// {EE-0396} UNUSED
	EE_UNUSED_0396,
	
	// {EE-0397} pop: can't set destination
	EE_POP_CANTSET,
	
	// {EE-0398} post: can't get source
	EE_POST_BADDESTEXP,
	
	// {EE-0399} post: can't get destination
	EE_POST_BADSOURCEEXP,
	
	// {EE-0400} pow: error in left operand
	EE_POW_BADLEFT,
	
	// {EE-0401} pow: error in right operand
	EE_POW_BADRIGHT,
	
	// {EE-0402} UNUSED
	EE_UNUSED_0402,
	
	// {EE-0403} print: can't get 'from' or 'to' coordinates
	EE_PRINT_CANTGETCOORD,
	
	// {EE-0404} print: can't get number of cards
	EE_PRINT_CANTGETCOUNT,
	
	// {EE-0405} print: can't get rectangle
	EE_PRINT_CANTGETRECT,
	
	// {EE-0406} print: error printing
	EE_PRINT_ERROR,
	
	// {EE-0407} print: error writing file (disk full?)
	EE_PRINT_IOERROR,
	
	// {EE-0408} print: coordinate not a point
	EE_PRINT_NAP,
	
	// {EE-0409} print: expression is not a rectangle
	EE_PRINT_NAR,
	
	// {EE-0410} print: not a card
	EE_PRINT_NOTACARD,
	
	// {EE-0411} print: card or stack must be open to print it
	EE_PRINT_NOTOPEN,
	
	// {EE-0412} print: no card specified
	EE_PRINT_NOTARGET,
	
	// {EE-0413} arcAngle: not an integer
	EE_PROPERTY_BADARCANGLE,
	
	// {EE-0414} blinkRate: not a number
	EE_PROPERTY_BADBLINKRATE,
	
	// {EE-0415} penColor: bad color
	EE_PROPERTY_BADCOLOR,
	
	// {EE-0416} colormap: bad color name or value
	EE_PROPERTY_BADCOLORS,
	
	// {EE-0417} Object: error counting objects as number
	EE_PROPERTY_BADCOUNTN,
	
	// {EE-0418} Object: error counting objects as text
	EE_PROPERTY_BADCOUNTS,
	
	// {EE-0419} dragSpeed: not a number
	EE_PROPERTY_BADDRAGSPEED,
	
	// {EE-0420} effectRate: not a number
	EE_PROPERTY_BADEFFECTRATE,
	
	// {EE-0421} extendKey: not a number
	EE_PROPERTY_BADEXTENDKEY,
	
	// {EE-0422} Property: bad array expression
	EE_PROPERTY_BADEXPRESSION,
	
	// {EE-0423} gridSize: not an integer
	EE_PROPERTY_BADGRIDSIZE,
	
	// {EE-0424} idleRate: not a number
	EE_PROPERTY_BADIDLERATE,
	
	// {EE-0425} lineSize: not an integer
	EE_PROPERTY_BADLINESIZE,
	
	// {EE-0426} moveSpeed: not a number
	EE_PROPERTY_BADMOVESPEED,
	
	// {EE-0427} multiSpace: not a number
	EE_PROPERTY_BADMULTISPACE,
	
	// {EE-0428} polySides: not an integer
	EE_PROPERTY_BADPOLYSIDES,
	
	// {EE-0429} repeatDelay: not a number
	EE_PROPERTY_BADREPEATDELAY,
	
	// {EE-0430} repeatRate: not a number
	EE_PROPERTY_BADREPEATRATE,
	
	// {EE-0431} doubleClickDelta: not an integer
	EE_PROPERTY_BADDOUBLEDELTA,
	
	// {EE-0432} doubleClickTime: not a number
	EE_PROPERTY_BADDOUBLETIME,
	
	// {EE-0433} roundRadius: not an integer
	EE_PROPERTY_BADROUNDRADIUS,
	
	// {EE-0434} slices: not an integer
	EE_PROPERTY_BADSLICES,
	
	// {EE-0435} startAngle: not an integer
	EE_PROPERTY_BADSTARTANGLE,
	
	// {EE-0436} traceDelay: not a number
	EE_PROPERTY_BADTRACEDELAY,
	
	// {EE-0437} traceStack: not a stack name
	EE_PROPERTY_BADTRACESTACK,
	
	// {EE-0438} print: bad property value
	EE_PROPERTY_BADPRINTPROP,
	
	// {EE-0439} syncRate: not a number
	EE_PROPERTY_BADSYNCRATE,
	
	// {EE-0440} tooltipDelay: not a number
	EE_PROPERTY_BADTOOLDELAY,
	
	// {EE-0441} typeRate: not a number
	EE_PROPERTY_BADTYPERATE,
	
	// {EE-0442} userLevel: not an integer
	EE_PROPERTY_BADUSERLEVEL,
	
	// {EE-0443} beep: not an integer
	EE_PROPERTY_BEEPNAN,
	
	// {EE-0444} brush: not an integer
	EE_PROPERTY_BRUSHNAN,
	
	// {EE-0445} brush: can't find image
	EE_PROPERTY_BRUSHNOIMAGE,
	
	// {EE-0446} brushPattern: not a valid image id
	EE_PROPERTY_BRUSHPATNAN,
	
	// {EE-0447} brushPattern: can't find image
	EE_PROPERTY_BRUSHPATNOIMAGE,
	
	// {EE-0448} Object: no object to set property
	EE_PROPERTY_CANTSET,
	
	// {EE-0449} Object: can't set object property
	EE_PROPERTY_CANTSETOBJECT,
	
	// {EE-0450} cursor: not an integer
	EE_PROPERTY_CURSORNAN,
	
	// {EE-0451} cursor: can't find image
	EE_PROPERTY_CURSORNOIMAGE,
	
	// {EE-0452} Property: value is not a boolean ("true" or "false")
	EE_PROPERTY_NAB,
	
	// {EE-0453} Property: value is not a number
	EE_PROPERTY_NAN,
	
	// {EE-0454} defaultStack: can't find stack
	EE_PROPERTY_NODEFAULTSTACK,
	
	// {EE-0455} defaultMenuBar: can't find group
	EE_PROPERTY_NODEFAULTMENUBAR,
	
	// {EE-0456} Object: does not have this property
	EE_PROPERTY_NOPROP,
	
	// {EE-0457} Object: property is not an integer
	EE_PROPERTY_NOTANUM,
	
	// {EE-0458} penPattern: not a valid image id
	EE_PROPERTY_PENPATNAN,
	
	// {EE-0459} penPattern: can't find image
	EE_PROPERTY_PENPATNOIMAGE,
	
	// {EE-0460} randomSeed: property is not an integer
	EE_PROPERTY_RANDOMSEEDNAN,
	
	// {EE-0461} socketTimeout: not a number
	EE_PROPERTY_SOCKETTIMEOUTNAN,
	
	// {EE-0462} umask: property is not an integer
	EE_PROPERTY_UMASKNAN,
	
	// {EE-0463} push: object is not a card
	EE_PUSH_NOTACARD,
	
	// {EE-0464} push: can't find card
	EE_PUSH_NOTARGET,
	
	// {EE-0465} put: error in expression
	EE_PUT_BADEXP,
	
	// {EE-0466} put: can't set destination
	EE_PUT_CANTSET,
	
	// {EE-0467} put: can't put into destination
	EE_PUT_CANTSETINTO,
	
	// {EE-0468} queryRegistry: error in source expression
	EE_QUERYREGISTRY_BADEXP,
	
	// {EE-0469} random: error in source expression
	EE_RANDOM_BADSOURCE,
	
	// {EE-0470} read: aborted
	EE_READ_ABORT,
	
	// {EE-0471} read: error in 'at' expression
	EE_READ_BADAT,
	
	// {EE-0472} read: error in condition expression
	EE_READ_BADCOND,
	
	// {EE-0473} read: error in count expression
	EE_READ_BADEXP,
	
	// {EE-0474} read: error reading
	EE_READ_ERROR,
	
	// {EE-0475} read: count is not an integer
	EE_READ_NAN,
	
	// {EE-0476} read: error in 'until' expression
	EE_READ_NOCHARACTER,
	
	// {EE-0477} read: file is not open
	EE_READ_NOFILE,
	
	// {EE-0478} read: error in 'for' expression
	EE_READ_NONUMBER,
	
	// {EE-0479} read: process is not open
	EE_READ_NOPROCESS,
	
	// {EE-0480} record: error in file expression
	EE_RECORD_BADFILE,
	
	// {EE-0481} recordCompression: type must be 4 characters
	EE_RECORDCOMPRESSION_BADTYPE,
	
	// {EE-0482} recordInput: type must be 4 characters
	EE_RECORDINPUT_BADTYPE,
	
	// {EE-0483} remove: can't find object
	EE_REMOVE_NOOBJECT,
	
	// {EE-0484} remove: object is not a group
	EE_REMOVE_NOTABACKGROUND,
	
	// {EE-0485} remove: object is not a card
	EE_REMOVE_NOTACARD,
	
	// {EE-0486} rename: error in source expression
	EE_RENAME_BADSOURCE,
	
	// {EE-0487} rename: error in destination expression
	EE_RENAME_BADDEST,
	
	// {EE-0488} repeat: aborted
	EE_REPEAT_ABORT,
	
	// {EE-0489} repeat: error in 'for' condition expression
	EE_REPEAT_BADFORCOND,
	
	// {EE-0490} repeat: error in statement
	EE_REPEAT_BADSTATEMENT,
	
	// {EE-0491} repeat: error in 'until' condition expression
	EE_REPEAT_BADUNTILCOND,
	
	// {EE-0492} repeat: error in 'while' condition expression
	EE_REPEAT_BADWHILECOND,
	
	// {EE-0493} repeat: error in 'with' end condition expression
	EE_REPEAT_BADWITHEND,
	
	// {EE-0494} repeat: error in 'with' start condition expression
	EE_REPEAT_BADWITHSTART,
	
	// {EE-0495} repeat: error in 'with' step condition expression
	EE_REPEAT_BADWITHSTEP,
	
	// {EE-0496} repeat: error setting 'with' variable
	EE_REPEAT_BADWITHVAR,
	
	// {EE-0497} replace: can't set container
	EE_REPLACE_CANTSET,
	
	// {EE-0498} replace: error in pattern expression
	EE_REPLACE_BADPATTERN,
	
	// {EE-0499} replace: error in replacement expression
	EE_REPLACE_BADREPLACEMENT,
	
	// {EE-0500} replace: error in container expression
	EE_REPLACE_BADCONTAINER,
	
	// {EE-0501} replaceText: bad parameter
	EE_REPLACETEXT_BADPATTERN,
	
	// {EE-0502} replaceText: bad source string
	EE_REPLACETEXT_BADSOURCE,
	
	// {EE-0503} reply: error in keyword expression
	EE_REPLY_BADKEYWORDEXP,
	
	// {EE-0504} reply: error in message expression
	EE_REPLY_BADMESSAGEEXP,
	
	// {EE-0505} request: error in keyword expression
	EE_REQUEST_BADKEYWORDEXP,
	
	// {EE-0506} request: error in message expression
	EE_REQUEST_BADMESSAGEEXP,
	
	// {EE-0507} request: error in program expression
	EE_REQUEST_BADPROGRAMEXP,
	
	// {EE-0508} request: no permission to request that
	EE_REQUEST_NOPERM,
	
	// {EE-0509} getResources: error in expression
	EE_RESOURCES_BADPARAM,
	
	// {EE-0510} getResources: no permission to get that
	EE_RESOURCES_NOPERM,
	
	// {EE-0511} return: error in expression
	EE_RETURN_BADEXP,
	
	// {EE-0512} revert: can't revert Home stack
	EE_REVERT_HOME,
	
	// {EE-0513} rotate: error in object expression
	EE_ROTATE_NOIMAGE,
	
	// {EE-0514} rotate: object is not an editable image
	EE_ROTATE_NOTIMAGE,
	
	// {EE-0515} rotate: error in angle expression
	EE_ROTATE_BADANGLE,
	
	// {EE-0516} round: error in source or digit expression
	EE_ROUND_BADSOURCE,
	
	// {EE-0517} save: error in file name expression
	EE_SAVE_BADNOFILEEXP,
	
	// {EE-0518} save: saving disabled
	EE_SAVE_NOPERM,
	
	// {EE-0519} save: can't find stack to save
	EE_SAVE_NOTARGET,
	
	// {EE-0520} save: object is not a stack
	EE_SAVE_NOTASTACK,
	
	// {EE-0521} seek: error in file name expression
	EE_SEEK_BADNAME,
	
	// {EE-0522} seek: error in offset expression
	EE_SEEK_BADWHERE,
	
	// {EE-0523} seek: file is not open
	EE_SEEK_NOFILE,
	
	// {EE-0524} select: can't select target
	EE_SELECT_BADTARGET,
	
	// {EE-0525} selectedChunk: error in button or field expression
	EE_SELECTED_BADSOURCE,
	
	// {EE-0526} selectedButton: bad family expression
	EE_SELECTEDBUTTON_BADFAMILY,
	
	// {EE-0527} selectedButton: bad parent object expression
	EE_SELECTEDBUTTON_BADPARENT,
	
	// {EE-0528} send: error in message handler execution
	EE_SEND_BADEXEC,
	
	// {EE-0529} send: error in message expression
	EE_SEND_BADEXP,
	
	// {EE-0530} send: error in 'in' expression
	EE_SEND_BADINEXP,
	
	// {EE-0531} send: error in program expression
	EE_SEND_BADPROGRAMEXP,
	
	// {EE-0532} send: bad target expression
	EE_SEND_BADTARGET,
	
	// {EE-0533} send: no permission to send that
	EE_SEND_NOPERM,
	
	// {EE-0534} set: error in source expression
	EE_SET_BADEXP,
	
	// {EE-0535} set: can't set property
	EE_SET_BADSET,
	
	// {EE-0536} setRegistry: no permission to get or set registry
	EE_SETREGISTRY_NOPERM,
	
	// {EE-0537} setRegistry: error in source expression
	EE_SETREGISTRY_BADEXP,
	
	// {EE-0538} shell: aborted
	EE_SHELL_ABORT,
	
	// {EE-0539} shell: can't run shell command
	EE_SHELL_BADCOMMAND,
	
	// {EE-0540} shell: error in source expression
	EE_SHELL_BADSOURCE,
	
	// {EE-0541} shell: no permission to run commands
	EE_SHELL_NOPERM,
	
	// {EE-0542} shortFilePath: error in file expression
	EE_SHORTFILEPATH_BADSOURCE,
	
	// {EE-0543} show: error in visual effect expression
	EE_SHOW_BADEFFECT,
	
	// {EE-0544} show: error in location expression
	EE_SHOW_BADLOCATION,
	
	// {EE-0545} show: error in number of cards expression
	EE_SHOW_BADNUMBER,
	
	// {EE-0546} show: location is not in proper x,y form
	EE_SHOW_NOLOCATION,
	
	// {EE-0547} show: can't find object
	EE_SHOW_NOOBJ,
	
	// {EE-0548} sin: error in source expression
	EE_SIN_BADSOURCE,
	
	// {EE-0549} UNUSED
	EE_UNUSED_0549,
	
	// {EE-0550} sort: can't find object to sort
	EE_SORT_BADTARGET,
	
	// {EE-0551} sort: error sorting
	EE_SORT_CANTSORT,
	
	// {EE-0552} sort: can't find field
	EE_SORT_NOTARGET,
	
	// {EE-0553} specialFolderPath: error in type expression
	EE_SPECIALFOLDERPATH_BADPARAM,
	
	// {EE-0554} sqrt: error in source expression
	EE_SQRT_BADSOURCE,
	
	// {EE-0555} UNUSED
	EE_UNUSED_0555,
	
	// {EE-0556} Stack: bad decoration
	EE_STACK_BADDECORATION,
	
	// {EE-0557} Stack: invalid id (must be greater than current id)
	EE_STACK_BADID,
	
	// {EE-0558} Stack: invalid key
	EE_STACK_BADKEY,
	
	// {EE-0559} Stack: bad substack name
	EE_STACK_BADSUBSTACK,
	
	// {EE-0560} Stack: can't set mainStack (has substacks?)
	EE_STACK_CANTSETMAINSTACK,
	
	// {EE-0561} Stack: deskIcon is not an integer
	EE_STACK_ICONNAN,
	
	// {EE-0562} Stack: userLevel is not an integer
	EE_STACK_LEVELNAN,
	
	// {EE-0563} Stack: size is not an integer
	EE_STACK_MINMAXNAN,
	
	// {EE-0564} Stack: stack is password protected
	EE_STACK_NOKEY,
	
	// {EE-0565} Stack: can't find mainStack
	EE_STACK_NOMAINSTACK,
	
	// {EE-0566} Stack: stack is not a mainStack
	EE_STACK_NOTMAINSTACK,
	
	// {EE-0567} Stack: error in external function handler
	EE_STACK_EXTERNALERROR,
	
	// {EE-0568} start: can't find object
	EE_START_BADTARGET,
	
	// {EE-0569} start: stack is locked
	EE_START_LOCKED,
	
	// {EE-0570} start: expression is not a group
	EE_START_NOTABACKGROUND,
	
	// {EE-0571} start: license limit exceeded
	EE_START_NOTLICENSED,
	
	// {EE-0572} Handler: error in source expression
	EE_STATEMENT_BADPARAM,
	
	// {EE-0573} Handler: can't find handler
	EE_STATEMENT_BADCOMMAND,
	
	// {EE-0574} stdDev: error in expression
	EE_STDDEV_BADSOURCE,
	
	// {EE-0575} stop: error in expression
	EE_STOP_BADTARGET,
	
	// {EE-0576} stop: target is not a group
	EE_STOP_NOTABACKGROUND,
	
	// {EE-0577} subtract: error in matrix operation
	EE_SUBTRACT_BADARRAY,
	
	// {EE-0578} subtract: destination has a bad format (numeric?)
	EE_SUBTRACT_BADDEST,
	
	// {EE-0579} subtract: error in source expression
	EE_SUBTRACT_BADSOURCE,
	
	// {EE-0580} subtract: can't set destination
	EE_SUBTRACT_CANTSET,
	
	// {EE-0581} subtract: can't subtract array from scalar
	EE_SUBTRACT_MISMATCH,
	
	// {EE-0582} subwindow: error in expression
	EE_SUBWINDOW_BADEXP,
	
	// {EE-0583} subwindow: can't find stack or button
	EE_SUBWINDOW_NOSTACK,
	
	// {EE-0584} sum: error in source expression
	EE_SUM_BADSOURCE,
	
	// {EE-0585} switch: error in condition expression
	EE_SWITCH_BADCOND,
	
	// {EE-0586} switch: error in case expression
	EE_SWITCH_BADCASE,
	
	// {EE-0587} switch: error in statement
	EE_SWITCH_BADSTATEMENT,
	
	// {EE-0588} textHeightSum: can't find object
	EE_TAN_BADSOURCE,
	
	// {EE-0589} UNUSED
	EE_UNUSED_0589,
	
	// {EE-0590} tan: domain error
	EE_TEXT_HEIGHT_SUM_NOOBJECT,
	
	// {EE-0591} there: error in source expression
	EE_THERE_BADSOURCE,
	
	// {EE-0592} throw: error in source expression
	EE_THROW_BADERROR,
	
	// {EE-0593} Operators *: error in matrix operation
	EE_TIMES_BADARRAY,
	
	// {EE-0594} Operators *: error in left operand
	EE_TIMES_BADLEFT,
	
	// {EE-0595} Operators *: error in right operand
	EE_TIMES_BADRIGHT,
	
	// {EE-0596} UNUSED
	EE_UNUSED_0596,
	
	// {EE-0597} toLower: error in source expression
	EE_TOLOWER_BADSOURCE,
	
	// {EE-0598} topStack: error in source expression
	EE_TOPSTACK_BADSOURCE,
	
	// {EE-0599} toUpper: error in source expression
	EE_TOUPPER_BADSOURCE,
	
	// {EE-0600} transpose: source is not a variable
	EE_TRANSPOSE_BADSOURCE,
	
	// {EE-0601} transpose: can't transpose this array
	EE_TRANSPOSE_MISMATCH,
	
	// {EE-0602} trunc: error in source expression
	EE_TRUNC_BADSOURCE,
	
	// {EE-0603} try: error in statement
	EE_TRY_BADSTATEMENT,
	
	// {EE-0604} type: script aborted
	EE_TYPE_ABORT,
	
	// {EE-0605} type: bad string expression
	EE_TYPE_BADSTRINGEXP,
	
	// {EE-0606} ungroup: can't find group
	EE_UNGROUP_NOGROUP,
	
	// {EE-0607} ungroup: target is not a group
	EE_UNGROUP_NOTAGROUP,
	
	// {EE-0608} uniDecode: error in language expression
	EE_UNIDECODE_BADLANGUAGE,
	
	// {EE-0609} uniDecode: error in source expression
	EE_UNIDECODE_BADSOURCE,
	
	// {EE-0610} uniEncode: error in language expression
	EE_UNIENCODE_BADLANGUAGE,
	
	// {EE-0611} uniEncode: error in source expression
	EE_UNIENCODE_BADSOURCE,
	
	// {EE-0612} unload: error in url expression
	EE_UNLOAD_BADURLEXP,
	
	// {EE-0613} unlock: expression is not a visual effect
	EE_UNLOCK_BADEFFECT,
	
	// {EE-0614} urlDecode: error in source expression
	EE_URLDECODE_BADSOURCE,
	
	// {EE-0615} urlEncode: error in source expression
	EE_URLENCODE_BADSOURCE,
	
	// {EE-0616} urlStatus: error in url expression
	EE_URLSTATUS_BADSOURCE,
	
	// {EE-0617} value: error in source expression
	EE_VALUE_BADSOURCE,
	
	// {EE-0618} value: error executing expression
	EE_VALUE_ERROR,
	
	// {EE-0619} value: can't find object
	EE_VALUE_NOOBJ,
	
	// {EE-0620} Array: bad index expression
	EE_VARIABLE_BADINDEX,
	
	// {EE-0621} Chunk: source is not a character
	EE_VARIABLE_NAC,
	
	// {EE-0622} Chunk: source is not a number
	EE_VARIABLE_NAN,
	
	// {EE-0623} visual: bad effect expression
	EE_VISUAL_BADEXP,
	
	// {EE-0624} wait: aborted
	EE_WAIT_ABORT,
	
	// {EE-0625} wait: error in expression
	EE_WAIT_BADEXP,
	
	// {EE-0626} wait: expression is not a number
	EE_WAIT_NAN,
	
	// {EE-0627} within: can't find control
	EE_WITHIN_NOCONTROL,
	
	// {EE-0628} within: not a point
	EE_WITHIN_NAP,
	
	// {EE-0629} write: error in expression
	EE_WRITE_BADEXP,
	
	// {EE-0630} Operators bitXor: error in left operand
	EE_XORBITS_BADLEFT,
	
	// {EE-0631} Operators bitXor: error in right operand
	EE_XORBITS_BADRIGHT,
	
	// {EE-0632} Property stackFileVersion: not a valid version
	EE_PROPERTY_STACKFILEBADVERSION,
	
	// {EE-0633} Property stackFileVersion: version not supported
	EE_PROPERTY_STACKFILEUNSUPPORTEDVERSION,
	
	// {EE-0634} External handler: exception
	EE_EXTERNAL_EXCEPTION,
	
	// {EE-0635} export: Empty rectangle
	EE_EXPORT_EMPTYRECT,
	
	// {EE-0636} import: Empty rectangle
	EE_IMPORT_EMPTYRECT,
	
	// {EE-0637} Property dragAction: Unknown action
	EE_DRAGDROP_BADACTION,
	
	// {EE-0638} Property dragImageOffset: Not a point
	EE_DRAGDROP_BADIMAGEOFFSET,
	
	// {EE-0639} Property dragDelta: Not a number
	EE_PROPERTY_BADDRAGDELTA,
	
	// {EE-0640} clipboard: invalid mix of data types
	EE_CLIPBOARD_BADMIX,
	
	// {EE-0641} clipboard: invalid text
	EE_CLIPBOARD_BADTEXT,
	
	// {EE-0642} wrap: bad array
	EE_WRAP_BADARRAY,
	
	// {EE-0643} wrap: illegal type for left operand
	EE_WRAP_BADLEFT,
	
	// {EE-0644} wrap: illegal type for right operand
	EE_WRAP_BADRIGHT,
	
	// {EE-0645} wrap: type mismatch
	EE_WRAP_MISMATCH,
	
	// {EE-0646} UNUSED
	EE_UNUSED_0646,
	
	// {EE-0647} UNUSED
	EE_UNUSED_0647,
	
	// {EE-0648} begins/ends with: illegal type for right operand
	EE_BEGINSENDS_BADRIGHT,
	
	// {EE-0649} begins/ends with: illegal type for left operand
	EE_BEGINSENDS_BADLEFT,
	
	// {EE-0650} UNUSED
	EE_UNUSED_0650,
	
	// {EE-0651} UNUSED
	EE_UNUSED_0651,
	
	// {EE-0652} UNUSED
	EE_UNUSED_0652,
	
	// {EE-0653} UNUSED
	EE_UNUSED_0653,
	
	// {EE-0654} UNUSED
	EE_UNUSED_0654,
	
	// {EE-0655} UNUSED
	EE_UNUSED_0655,
	
	// {EE-0656} UNUSED
	EE_UNUSED_0656,
	
	// {EE-0657} UNUSED
	EE_UNUSED_0657,
	
	// {EE-0658} graphic: not a boolean
	EE_GRAPHIC_NAB,
	
	// {EE-0659} graphic: bad fill rule
	EE_GRAPHIC_BADFILLRULE,
	
	// {EE-0660} graphic: bad gradient type
	EE_GRAPHIC_BADGRADIENTTYPE,
	
	// {EE-0661} graphic: bad gradient ramp
	EE_GRAPHIC_BADGRADIENTRAMP,
	
	// {EE-0662} graphic: bad gradient point
	EE_GRAPHIC_BADGRADIENTPOINT,
	
	// {EE-0663} graphic: bad gradient quality
	EE_GRAPHIC_BADGRADIENTQUALITY,
	
	// {EE-0664} graphic: bad gradient key
	EE_GRAPHIC_BADGRADIENTKEY,
	
	// {EE-0665} graphic: bad cap style
	EE_GRAPHIC_BADCAPSTYLE,
	
	// {EE-0666} graphic: bad join style
	EE_GRAPHIC_BADJOINSTYLE,
	
	// {EE-0667} byteToNum: bad expression
	EE_BYTETONUM_BADSOURCE,
	
	// {EE-0668} numToByte: bad expression
	EE_NUMTOBYTE_BADSOURCE,
	
	// {EE-0669} arrayEncode: bad expression
	EE_ARRAYENCODE_BADSOURCE,
	
	// {EE-0670} arrayEncode: failure
	EE_ARRAYENCODE_FAILED,
	
	// {EE-0671} arrayDecode: bad expression
	EE_ARRAYDECODE_BADSOURCE,
	
	// {EE-0672} arrayDecode: failure
	EE_ARRAYDECODE_FAILED,
	
	// {EE-0673} parentScript: bad object
	EE_PARENTSCRIPT_BADOBJECT,
	
	// {EE-0674} dispatch: bad message
	EE_DISPATCH_BADMESSAGEEXP,
	
	// {EE-0675} dispatch: bad command
	EE_DISPATCH_BADCOMMAND,
	
	// {EE-0676} dispatch: bad target
	EE_DISPATCH_BADTARGET,
	
	// {EE-0677} queryRegistry: bad destination
	EE_QUERYREGISTRY_BADDEST,
	
	// {EE-0678} parentScript: loop in hierarchy
	EE_PARENTSCRIPT_CYCLICOBJECT,
	
	// {EE-0679} Security: disk access not allowed
	EE_DISK_NOPERM,
	
	// {EE-0680} Security: network access not allowed
	EE_NETWORK_NOPERM,
	
	// {EE-0681} Security: external command access not allowed
	EE_PROCESS_NOPERM,
	
	// {EE-0682} Security: registry access not allowed
	EE_REGISTRY_NOPERM,
	
	// {EE-0683} <not used>
	NOTUSED__EE_STACK_NOPERM,
	
	// {EE-0684} Security: printer access not allowed
	EE_PRINT_NOPERM,
	
	// {EE-0685} Security: audio and camera access not allowed
	EE_PRIVACY_NOPERM,
	
	// {EE-0686} 
	EE_BITMAPEFFECT_BADKEY,
	
	// {EE-0687} 
	EE_BITMAPEFFECT_BADKEYFORTYPE,
	
	// {EE-0688} 
	EE_BITMAPEFFECT_BADNUMBER,
	
	// {EE-0689} 
	EE_BITMAPEFFECT_BADCOLOR,
	
	// {EE-0690} 
	EE_BITMAPEFFECT_BADBLENDMODE,
	
	// {EE-0691} 
	EE_BITMAPEFFECT_BADFILTER,
	
	// {EE-0692} 
	EE_BITMAPEFFECT_BADTECHNIQUE,
	
	// {EE-0693} 
	EE_BITMAPEFFECT_BADSOURCE,
	
	// {EE-0694} 
	EE_BITMAPEFFECT_BADBOOLEAN,
	
	// {EE-0695} 
	EE_SECUREMODE_BADCATEGORY,
	
	// {EE-0696} Security: AppleScript access not allowed
	EE_SECUREMODE_APPLESCRIPT_NOPERM,
	
	// {EE-0697} Security: alternate script execution not allowed
	EE_SECUREMODE_DOALTERNATE_NOPERM,
	
	// {EE-0698} processType: bad value
	EE_PROCESSTYPE_BADVALUE,
	
	// {EE-0699} processType: cannot change
	EE_PROCESSTYPE_NOTPOSSIBLE,
	
	// {EE-0700} 
	NOTUSED__EE_VARIABLE_CONTENTS_LOCKED,
	
	// {EE-0701} 
	NOTUSED__EE_VARIABLE_ELEMENT_LOCKED,
	
	// {EE-0702} do in browser: not supported
	EE_ENVDO_NOTSUPPORTED,
	
	// {EE-0703} do in browser: failed
	EE_ENVDO_FAILED,
	
	// {EE-0704} editMode: Bad value
	EE_GRAPHIC_BADEDITMODE,
	
	// {EE-0705} export: Bad palette
	EE_EXPORT_BADPALETTE,
	
	// {EE-0706} export: Invalid palette size
	EE_EXPORT_BADPALETTESIZE,
	
	// {EE-0707} printing: Bad options array
	EE_OPEN_BADOPTIONS,
	
	// {EE-0708} 
	NOTUSED__EE_FEATURE_NOTSUPPORTED,
	
	// {EE-0709} printing: Unknown destination
	EE_PRINT_UNKNOWNDST,
	
	// {EE-0710} object: cannot change id while in group edit mode
	EE_OBJECT_NOTWHILEEDITING,
	
	// {EE-0711} print: bad anchor name
	EE_PRINTANCHOR_BADNAME,
	
	// {EE-0712} print: bad anchor location
	EE_PRINTANCHOR_BADLOCATION,
	
	// {EE-0713} print: anchor location not a point
	EE_PRINTANCHOR_LOCATIONNAP,
	
	// {EE-0714} print: bad link destination
	EE_PRINTLINK_BADDEST,
	
	// {EE-0715} print: bad link area
	EE_PRINTLINK_BADAREA,
	
	// {EE-0716} print: link area not a rectangle
	EE_PRINTLINK_AREANAR,
	
	// {EE-0717} randomBytes: bad count
	EE_RANDOMBYTES_BADCOUNT,
	
	// {EE-0718} sha1Digest: bad source
	EE_SHA1DIGEST_BADSOURCE,
	
	// {EE-0719} stackLimit: not a number
	EE_STACKLIMIT_NAN,
	
	// {EE-0720} could not load security library
	EE_SECURITY_NOLIBRARY,
	
	// {EE-0721} group: cannot be background or shared (it is nested)
	EE_GROUP_CANNOTBEBGORSHARED,
	
	// {EE-0722} group: must be shared (it is placed on more than one card, or unplaced)
	EE_GROUP_CANNOTBENONSHARED,
	
	// {EE-0723} group: cannot group background or shared groups
	EE_GROUP_NOBG,
	
	// {EE-0724} print: bad bookmark title
	EE_PRINTBOOKMARK_BADTITLE,
	
	// {EE-0725} print: bad bookmark level
	EE_PRINTBOOKMARK_BADLEVEL,
	
	// {EE-0726} print: bad bookmark position
	EE_PRINTBOOKMARK_BADAT,
	
	// {EE-0727} print: bad bookmark initial state
	EE_PRINTBOOKMARK_BADINITIAL,
	
	// {EE-0728} Script File Index:
	EE_SCRIPT_FILEINDEX,
	
	// {EE-0729} include: could not find file
	EE_INCLUDE_FILENOTFOUND,
	
	// {EE-0730} script: parsing error
	EE_SCRIPT_SYNTAXERROR,
	
	// {EE-0731} script: bad statement
	EE_SCRIPT_BADSTATEMENT,
	
	// {EE-0732} include: not supported in this environment
	EE_INCLUDE_BADCONTEXT,
	
	// {EE-0733} include: bad filename
	EE_INCLUDE_BADFILENAME,
	
	// {EE-0734} include: nesting depth limit reached
	EE_INCLUDE_TOOMANY,
	
	// {EE-0735} require: not supported in this environment
	EE_REQUIRE_BADCONTEXT,
	
	// {EE-0736} 
	NOTUSED__EE_REQUIRE_BADFILENAME,
	
	// {EE-0737} 
	NOTUSED__EE_REQUIRE_BADSTACK,
	
	// {EE-0738} errorMode: bad value
	EE_ERRORMODE_BADVALUE,
	
	// {EE-0739} outputEncoding: bad value
	EE_OUTPUTENCODING_BADVALUE,
	
	// {EE-0740} outputLineEnding: bad value
	EE_OUTPUTLINEENDING_BADVALUE,
	
	// {EE-0741} Script Error Position:
	EE_SCRIPT_ERRORPOS,
	
	// {EE-0742} defaultNetworkInterface: network interface must be an IPv4 address
	EE_PROPERTY_BADNETWORKINTERFACE,

	// {EE-0743} layerMode: must be 'static', 'dynamic' or 'scrolling'
	EE_CONTROL_BADLAYERMODE,
	
	// {EE-0744} tileSize: must be a power of two between 16 and 256
	EE_COMPOSITOR_INVALIDTILESIZE,
	
	// {EE-0745} tileCompositor: unknown type
	EE_COMPOSITOR_UNKNOWNTYPE,
	
	// {EE-0746} tileCompositor: not supported on this platform
	EE_COMPOSITOR_NOTSUPPORTED,
	
	// {EE-0747} intersects: error in threshold expression
	EE_INTERSECT_BADTHRESHOLD,
	
	// {EE-0748} intersects: threshold must be a non-negative number, "bounds", "pixels" or "opaque pixels"
	EE_INTERSECT_ILLEGALTHRESHOLD,
	
	// {EE-0749} session: not supported in this environment
	EE_SESSION_BADCONTEXT,
	
	// {EE-0750} sessionSavePath: bad value
	EE_SESSION_SAVE_PATH_BADVALUE,
	
	// {EE-0751} sessionLifetime: bad value
	EE_SESSION_LIFETIME_BADVALUE,
	
	// {EE-0752} sessionName: bad value
	EE_SESSION_NAME_BADVALUE,
	
	// {EE-0753} sessionId: bad value
	EE_SESSION_ID_BADVALUE,
	
	// {EE-0754} lock: bad rect expr
	EE_LOCK_BADRECT,
	
	// {EE-0755} lock: not a rectangle
	EE_LOCK_NAR,

	// {EE-0756} field: bad listStyle value
	EE_FIELD_BADLISTSTYLE,

	// {EE-0757} field: firstIndent must be an integer
	EE_FIELD_FIRSTINDENTNAN,
	
	// {EE-0758} field: leftIndent must be an integer
	EE_FIELD_LEFTINDENTNAN,

	// {EE-0759} field: rightIndent must be an integer
	EE_FIELD_RIGHTINDENTNAN,

	// {EE-0760} field: spaceBefore must be an integer
	EE_FIELD_SPACEBEFORENAN,

	// {EE-0761} field: spaceAfter must be an integer
	EE_FIELD_SPACEAFTERNAN,

	// {EE-0762} field: listDepth must be an integer between 1 and 16.
	EE_FIELD_BADLISTDEPTH,

	// {EE-0763} field: borderWidth must be an integer
	EE_FIELD_BORDERWIDTHNAN,

	// {EE-0764} field: listIndent must be an integer
	EE_FIELD_LISTINDENTNAN,

	// {EE-0765} field: padding must be an integer
	EE_FIELD_PADDINGNAN,
	
	// {EE-0766} Chunk: can't store unicode to destination container
	EE_CHUNK_CANTSETUNICODEDEST,
	
	// {EE-0767} relayer: target not a container
	EE_RELAYER_TARGETNOTCONTAINER,
	
	// {EE-0768} relayer: couldn't resolve target object
	EE_RELAYER_NOTARGET,

	// {EE-0769} relayer: couldn't resolve source control

	EE_RELAYER_NOSOURCE,

	// {EE-0770} relayer: source not a control
	EE_RELAYER_SOURCENOTCONTROL,

	// {EE-0771} relayer: source and target not on the same card
	EE_RELAYER_CARDNOTSAME,

	// {EE-0772} relayer: layer not an integer
	EE_RELAYER_LAYERNAN,

	// {EE-0773} relayer: bad layer expression
	EE_RELAYER_BADLAYER,

	// {EE-0774} relayer: target not a control
	EE_RELAYER_TARGETNOTCONTROL,

	// {EE-0775} relayer: cannot move a control into a descendent
	EE_RELAYER_ILLEGALMOVE,
	
	// {EE-0776} relayer: required objects disappeared while processing
	EE_RELAYER_OBJECTSVANISHED,
	
	// {EE-0777} controlAtLoc: location not a point
	EE_CONTROLATLOC_NAP,
	
	// {EE-0778} do: no caller
	EE_DO_NOCALLER,
	
	// {EE-0779} read: invalid command for datagram socket
	EE_READ_NOTVALIDFORDATAGRAM,

	// {EE-0780} field: listIndex must be an integer
	EE_FIELD_LISTINDEXNAN,
	
	// {EE-0781} image cache limit: not a number
	EE_PROPERTY_BADIMAGECACHELIMIT,
    
    // {EE-0782} controls don't have the same owner
	EE_GROUP_DIFFERENTPARENT,
	
	// {EE-0783} uuid: bad type expression
	EE_UUID_BADTYPE,
	
	// {EE-0784} uuid: wrong number of arguments for specified type
	EE_UUID_TOOMANYPARAMS,
	
	// {EE-0785} uuid: unsupported type
	EE_UUID_UNKNOWNTYPE,
	
	// {EE-0786} uuid: bad namespace expression
	EE_UUID_BADNAMESPACEID,
	
	// {EE-0787} uuid: namespace not a uuid
	EE_UUID_NAMESPACENOTAUUID,
	
	// {EE-0788} uuid: bad name expression
	EE_UUID_BADNAME,
	
	// {EE-0789} uuid: not enough randomness available
	EE_UUID_NORANDOMNESS,
	
	// {EE-0790} avgDev: error in source expression
	EE_AVGDEV_BADSOURCE,
	
	// {EE-0791} geometricMean: error in source expression
	EE_GEO_MEAN_BADSOURCE,

	// {EE-0792} harmonicMean: error in source expression
	EE_HAR_MEAN_BADSOURCE,

	// {EE-0793} pStdDev: error in source expression
	EE_POP_STDDEV_BADSOURCE,
	
	// {EE-0794} pVariance: error in source expression
	EE_POP_VARIANCE_BADSOURCE,
	
	// {EE-0795} variance: error in source expression
	EE_VARIANCE_BADSOURCE,
	
	// {EE-0796} group: object cannot be grouped
	EE_GROUP_NOTGROUPABLE,
    
    // MERG-2013-08-14: [[ MeasureText ]] Measure text relative to the effective font on an object
    // {EE-0797} measureText: no object
    EE_MEASURE_TEXT_NOOBJECT,

    // TD-2013-06-24: [[ DynamicFonts ]]
    // {EE-0798} font: couldn't find font
	EE_FONT_BADFILEEXP,
	
	// MERG-2013-10-04: [[ EditScriptAt ]] edit script of object at.
    // {EE-0799} edit script: bad at expression
	EE_EDIT_BADAT,

	// IM-2013-09-22: [[ FullscreenMode ]]
	// {EE-0800} fullscreenmode: not a valid mode
    EE_STACK_BADFULLSCREENMODE,
	
	// IM-2013-12-04: [[ PixelScale ]]
	// {EE-0801} pixelScale: not a valid scale value
	EE_PROPERTY_BADPIXELSCALE,
	
	// IM-2014-01-07: [[ StackScale ]]
	// {EE-0802} scaleFactor: not a valid scale value
	EE_STACK_BADSCALEFACTOR,
	
	// IM-2014-01-30: [[ HiDPI ]]
	// {EE-0803} pixelScale: the pixelScale property cannot be set on this platform
	EE_PROPERTY_PIXELSCALENOTSUPPORTED,
	
	// IM-2014-01-30: [[ HiDPI ]]
	// {EE-0804} usePixelScaling: the usePixelScaling property cannot be set on this platform
	EE_PROPERTY_USEPIXELSCALENOTSUPPORTED,
	
	// MM-2014-02-12: [[ SecureSocket ]]
	// {EE-0805} secure: error in socket expression
	EE_SECURE_BADNAME,
	
	// PM-2014-04-15: [[Bug 12105]]
	// {EE-0806} paramCount: could not find handler
	EE_PARAMCOUNT_NOHANDLER,
	
	// MW-2015-05-28: [[ Bug 12463 ]]
	// {EE-0807} send: too many pending messages
	EE_SEND_TOOMANYPENDING,
	
	// MM-2014-06-13: [[ Bug 12567 ]] New variant secure socket <socket> with verification for host <host>
	// {EE-0808} secure: error in host name expression
	EE_SECURE_BADHOST,
	
	// MM-2014-06-13: [[ Bug 12567 ]] New variant open socket <socket> with verification for host <host>
	// {EE-0809} open: error in host name expression
	EE_OPEN_BADHOST,
	
	// SN-2014-12-15: [[ Bug 14211 ]] Add an error when using a parsed bad extents (such as 'next')
	// {EE-0810} Chunk: bad extents provided
	EE_CHUNK_BADEXTENTS,
	
	// {EE-0811} external: unlicensed
	EE_EXTERNAL_UNLICENSED,

    // {EE-0812} Import: no image selected, or image not open
    EE_IMPORT_NOSELECTED,

    // {EE-0813} Resolve image: error in source expression
    EE_RESOLVE_IMG_BADEXP,

    // {EE-0814} Internal BSDiff: error in old file expression
    EE_INTERNAL_BSDIFF_BADOLD,

    // {EE-0815} Internal BSDiff: error in new file expression
    EE_INTERNAL_BSDIFF_BADNEW,

    // {EE-0816} Internal BSDiff: error in patch filename expression
    EE_INTERNAL_BSDIFF_BADPATCH,

    // {EE-0817} Internal Bootstrap: error in stack file
    EE_INTERNAL_BOOTSTRAP_BADSTACK,

    // {EE-0818} IDE script configure: error in settings expression
    EE_IDE_BADARRAY,

    // {EE-0819} IDE script replace: error in text expression
    EE_IDE_BADTEXT,

    // {EE-0820} IDE script classify: error in script expression
    EE_IDE_BADSCRIPT,

    // {EE-0821} IDE filter control: error in pattern expression
    EE_IDE_BADPATTERN,

    // {EE-0822} Engine PutInto: error in variable expression
    EE_ENGINE_PUT_BADVARIABLE,

    // {EE-0823} Engine DeleteVariableChunk: error in expression
    EE_ENGINE_DELETE_BADVARCHUNK,

    // {EE-0824} IDE Extract: error in segment expression
    EE_IDE_EXTRACT_BADSEGMENT,

    // {EE-0825} IDE Extract: error in section name expression
    EE_IDE_EXTRACT_BADSECTION,

    // {EE-0826} IDE Extract: error in filename expression
    EE_IDE_EXTRACT_BADFILENAME,
    
    // {EE-0827} MCInternalPayloadPatch: error in output filename expression
    EE_OUTPUT_BADFILENAME,
    
    // {EE-0828} MCInternalListTasksWithModule: error in module expression
    EE_INTERNAL_TASKS_BADMODULE,
    
    // {EE-0829} MCInternalCanDeleteFile: error in filename expression
    EE_INTERNAL_DELETE_BADFILENAME,
    
    // {EE-0830} MCInternalCanDeleteKey: error in key expression
    EE_INTERNAL_DELETE_BADKEY,

    // {EE-0831} MCHandler: error in expression
    EE_HANDLER_BADEXP,
    
    // {EE-0832} textDecode: bad data expression
    EE_TEXTDECODE_BADDATA,
    
    // {EE-0833} textDecode: bad text encoding expression
    EE_TEXTDECODE_BADENCODING,
    
    // {EE-0834} textDecode: could not decode data
    EE_TEXTDECODE_FAILED,
    
    // {EE-0835} textEncode: bad text expression
    EE_TEXTENCODE_BADTEXT,
    
    // {EE-0836} textEncode: bad text encoding expression
    EE_TEXTENCODE_BADENCODING,
    
    // {EE-0837} textEncode: could not encode text
    EE_TEXTENCODE_FAILED,
    
    // {EE-0838} normalizeText: bad normal form
    EE_NORMALIZETEXT_BADFORM,
    
    // {EE-0839} normalizeText: bad text expression
    EE_NORMALIZETEXT_BADTEXT,
    
    // {EE-0840} codepointProperty: bad codepoint
    EE_CODEPOINTPROPERTY_BADCODEPOINT,

    // {EE-0841} codepointProperty: bad property name
    EE_CODEPOINTPROPERTY_BADPROPERTY,
	
	// SN-2014-05-06: [[ Bug 12360 ]]
	// {EE-0842} open: bad text encoding
	EE_OPEN_BADENCODING,
	
	// {EE-0843} open: unsupported encoding
	EE_OPEN_UNSUPPORTED_ENCODING,
	
	// AL-2014-10-17: [[ BiDi ]] Returns the result of applying the bi-directional algorithm to text
	// {EE-0844} bidiDirection: error in source expression
	EE_BIDIDIRECTION_BADSOURCE,
	
	// MW-2014-10-23: Improve the error message you get from 'start using <name>'
	// {EE-0845} start: script of specified stack won't compile
	EE_START_WONTCOMPILE,
	
	// SN-2014-12-16: [[ Bug 14181 ]] hostnameToAddress should have no message on server
	// {EE-0846} hostnameToAddress: callbacks are not allowed on server
	EE_HOSTNAME_BADMESSAGE,

    // {EE-0847} Error evaluating expression
    EE_EXPR_EVALERROR,
    
    // {EE-0848} Property: value is not a character
    EE_PROPERTY_NAC,
    
    // {EE-0849} Property: value is not a string
    EE_PROPERTY_NAS,
    
    // {EE-0850} Property: value is not a color
    EE_PROPERTY_NOTACOLOR,
    
    // {EE-0851} Property: value is not a rectangle
    EE_PROPERTY_NOTARECT,
    
    // {EE-0852} Property: value is not a point
    EE_PROPERTY_NOTAPOINT,
    
    // {EE-0853} Property: value is not a pair of integers
    EE_PROPERTY_NOTAINTPAIR,
    
    // {EE-0854} Property: value is not a quadruple of integers
    EE_PROPERTY_NOTAINTQUAD,
    
    // {EE-0855} Property: invalid enumeration value
    EE_PROPERTY_BADENUMVALUE,
    
    // {EE-0856} Backdrop: invalid value
    EE_BACKDROP_INVALIDVALUE,
    
    // {EE-0857} Property: value is not an array
    EE_PROPERTY_NOTANARRAY,
    
    // {EE-0858} MCInternalPayloadPatch: error in patch item expression
    EE_INTERNAL_PATCH_BADITEM,
    
    // {EE-0859} MCInternalPayloadPatch: error in base item expression
    EE_INTERNAL_BASE_BADITEM,

	// MDW-2014-09-28: [[ feature_floor ]]
	// {EE-0860} floor: bad parameter
	EE_FLOOR_BADSOURCE,

	// MDW-2014-09-28: [[ feature_floor ]]
	// {EE-0861} ceil: bad parameter
    EE_CEIL_BADSOURCE,

    // {EE-0862} commandArguments: bad parameter
    EE_COMMANDARGUMENTS_BADPARAM,
    
    // AL-2015-07-07: The following error codes are 8.0 specific so should have their numbers
    //  incremented whenever new codes are merged up from below.
    // MW-2014-12-10: [[ Extensions ]] The error codes used to indicate an extension error.
    // {EE-0863} extension: error occurred with domain
    EE_EXTENSION_ERROR_DOMAIN,
    // {EE-0864} extension: error occurred with description
    EE_EXTENSION_ERROR_DESCRIPTION,
    // {EE-0865} extension: error occurred with file
    EE_EXTENSION_ERROR_FILE,
    // {EE-0866} extension: error occurred with line
    EE_EXTENSION_ERROR_LINE,
    
    // {EE-0867} load: error in extension expression
    EE_LOAD_BADEXTENSION,
    
    // {EE-0868} load: error in resource path expression
    EE_LOAD_BADRESOURCEPATH,
    
    // {EE-0869} System error: function
    EE_SYSTEM_FUNCTION,

    // {EE-0870} System error: code
    EE_SYSTEM_CODE,

    // {EE-0871} System error: message
    EE_SYSTEM_MESSAGE,
    
    // {EE-0872} Import: bad array
    EE_IMPORT_BADARRAY,
    
    // {EE-0873} Import: not an object array
    EE_IMPORT_NOTANOBJECTARRAY,

    // {EE-0874} clipboard: bad item type or data
    EE_CLIPBOARD_BADREP,
    
    // {EE-0875} clipboard: failed to insert item
    EE_CLIPBOARD_INSERTFAILED,
    
    // {EE-0876} clipboard: clipboard not locked
    EE_CLIPBOARD_NOTLOCKED,
    
    // {EE-0877} clipboard: already locked
    EE_CLIPBOARD_ALREADYLOCKED,
    
    // {EE-0878} clipboard: needs to be cleared (contains external data)
    EE_CLIPBOARD_EXTERNALDATA,
	
	// {EE-0879} go: error in widget expression
	EE_GO_BADWIDGETEXP,
	
	// {EE-0880} launch: error in widget expression
	EE_LAUNCH_BADWIDGETEXP,

	// {EE-0881} do: error in widget expression
	EE_DO_BADWIDGETEXP,

    // {EE-0882} documentFilename: bad filename
    EE_DOCUMENTFILENAME_BADFILENAME, 

	// {EE-0883} save: error in file format expression
	EE_SAVE_BADNOFORMATEXP,
	
	// {EE-0884} replace: not a field chunk
	EE_REPLACE_BADFIELDCHUNK,
	
	// {EE-0885} call: too few arguments
	EE_INVOKE_TOOFEWARGS,
	
	// {EE-0886} call: too many arguments
	EE_INVOKE_TOOMANYARGS,
    
    // {EE-0887} Stack: script only stacks can not be password protected
    EE_SCRIPT_ONLY_STACK_NOPASSWORD,
    
    // {EE-0888} revert: can't find stack
    EE_REVERT_NOSTACK,
    
    // {EE-0889} vectordot: error in first parameter
    EE_VECTORDOT_BADLEFT,
    
    // {EE-0890} vectordot: error in second parameter
    EE_VECTORDOT_BADRIGHT,
    
    // {EE-0891} vectordot: arrays are not key-wise compatible
    EE_VECTORDOT_MISMATCH,

	// {EE-0892} files: error in folder parameter
	EE_FILES_BADFOLDER,

	// {EE-0893} folders: error in folder parameter
	EE_FOLDERS_BADFOLDER,

    // {EE-0894} no target object
    EE_NOTARGET,

    // {EE-0895} image: cannot change image while being edited
    EE_IMAGE_MUTABLELOCK,

    // {EE-0896} graphic : too many points
    EE_GRAPHIC_TOOMANYPOINTS,

	// {EE-0897} extension: error occurred with column
	EE_EXTENSION_ERROR_COLUMN,
	
	// {EE-0898} parentScript: can't change parent while parent script is executing
    EE_PARENTSCRIPT_EXECUTING,
    
    // {EE-0899} call: type conversion error
    EE_INVOKE_TYPEERROR,

    // {EE-0900} open: error in from address expression
    EE_OPEN_BADFROMADDRESS,

    // {EE-0901} messageDigest: error in digest type parameter
    EE_MESSAGEDIGEST_BADTYPE,

    // {EE-0902} messageDigest: error in message data parameter
    EE_MESSAGEDIGEST_BADDATA,

    // {EE-0903} snapshot: no screen
    EE_SNAPSHOT_FAILED,

    // {EE-0904} stack: password protecting stacks not supported in this edition
    EE_STACK_PASSWORD_NOT_SUPPORTED,
    
    // {EE-0905} files: error in kind parameter
    EE_FILES_BADKIND,
    
    // {EE-0906} folders: error in kind parameter
    EE_FOLDERS_BADKIND,

    // {EE-0907} library mapping: bad mapping
    EE_BAD_LIBRARY_MAPPING,

    // {EE-0908} fontLanguage: bad font name
    EE_FONTLANGUAGE_BADFONTNAME,
    
    // {EE-0909} android permission: bad permission name
    EE_BAD_PERMISSION_NAME,
    
    // {EE-0910} Property: value is not a data
    EE_PROPERTY_NOTADATA
    
};

extern const char *MCexecutionerrors;

#endif
