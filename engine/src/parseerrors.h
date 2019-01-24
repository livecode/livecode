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

#ifndef __MC_PARSE_ERRORS__
#define __MC_PARSE_ERRORS__

// This file is processed automatically to produce the error strings that are
// embedded in the IDE and Server engines. Take care when editing the file and
// ensure that new errors are added in precisely this form:
//   <tab>// {PE-iiii} <message>
//   <tab>PE_<tag>
//   <return>

enum Parse_errors
{
	// NOT AN ERROR CODE
	PE_UNDEFINED,
	
	// {PE-0001} Object Name:
	PE_OBJECT_NAME,
	
	// {PE-0002} set: license limit exceeded
	PE_OBJECT_NOTLICENSED,
	
	// {PE-0003} abs: bad parameter
	PE_ABS_BADPARAM,
	
	// {PE-0004} accept: bad expression
	PE_ACCEPT_BADEXP,
	
	// {PE-0005} acos: bad parameter
	PE_ACOS_BADPARAM,
	
	// {PE-0006} add: bad destination
	PE_ADD_BADDEST,
	
	// {PE-0007} add: bad expression
	PE_ADD_BADEXP,
	
	// {PE-0008} add: missing 'to'
	PE_ADD_NOTO,
	
	// {PE-0009} aliasReference: bad expression
	PE_ALIASREFERENCE_BADPARAM,
	
	// {PE-0010} annuity: bad parameter
	PE_ANNUITY_BADPARAM,
	
	// {PE-0011} answer: bad question
	PE_ANSWER_BADQUESTION,
	
	// {PE-0012} answer: bad response
	PE_ANSWER_BADRESPONSE,
	
	// {PE-0013} answer: bad title
	PE_ANSWER_BADTITLE,
	
	// {PE-0014} split: bad variable
	PE_ARRAYOP_BADARRAY,
	
	// {PE-0015} split: missing 'with'
	PE_ARRAYOP_NOWITH,
	
	// {PE-0016} split: bad expression
	PE_ARRAYOP_BADEXP,
	
	// {PE-0017} arrowKey: bad direction expression
	PE_ARROWKEY_BADEXP,
	
	// {PE-0018} asin: bad parameter
	PE_ASIN_BADPARAM,
	
	// {PE-0019} ask: bad question
	PE_ASK_BADQUESTION,
	
	// {PE-0020} ask: bad reply
	PE_ASK_BADREPLY,
	
	// {PE-0021} ask: bad title
	PE_ASK_BADTITLE,
	
	// {PE-0022} atan2: bad parameter
	PE_ATAN2_BADPARAM,
	
	// {PE-0023} atan: bad parameter
	PE_ATAN_BADPARAM,
	
	// {PE-0024} average: bad parameter
	PE_AVERAGE_BADPARAM,
	
	// {PE-0025} base64Decode: bad parameter
	PE_BASE64DECODE_BADPARAM,
	
	// {PE-0026} base64Encode: bad parameter
	PE_BASE64ENCODE_BADPARAM,
	
	// {PE-0027} baseConvert: bad parameter
	PE_BASECONVERT_BADPARAM,
	
	// {PE-0028} binaryDecode: bad expression
	PE_BINARYD_BADPARAM,
	
	// {PE-0029} binaryEncode: bad expression
	PE_BINARYE_BADPARAM,
	
	// {PE-0030} cancel: bad message expression
	PE_CANCEL_BADEXP,
	
	// {PE-0031} charToNum: bad parameter
	PE_CHARTONUM_BADPARAM,
	
	// {PE-0032} choose: bad expression
	PE_CHOOSE_BADEXP,
	
	// {PE-0033} choose: no tool specified
	PE_CHOOSE_NOTOKEN,
	
	// {PE-0034} Chunk: token is not a chunk
	PE_CHUNK_BADCHUNK,
	
	// {PE-0035} Chunk: bad destination
	PE_CHUNK_BADDEST,
	
	// {PE-0036} Chunk: bad non-chunk expression
	PE_CHUNK_BADEXP,
	
	// {PE-0037} Chunk: bad chunk order (must be small to large)
	PE_CHUNK_BADORDER,
	
	// {PE-0038} Chunk: bad preposition
	PE_CHUNK_BADPREP,
	
	// {PE-0039} Chunk: bad range
	PE_CHUNK_BADRANGE,
	
	// {PE-0040} Chunk: bad stack reference
	PE_CHUNK_BADSTACKREF,
	
	// {PE-0041} Chunk: missing chunk
	PE_CHUNK_NOCHUNK,
	
	// {PE-0042} Chunk: bad expression
	PE_CHUNK_NOENDEXP,
	
	// {PE-0043} Chunk: bad range end expression
	PE_CHUNK_NOSTARTEXP,
	
	// {PE-0044} Chunk: can't create a variable with that name (explicitVariables?)
	PE_CHUNK_NOVARIABLE,
	
	// {PE-0045} click: bad "button" expression
	PE_CLICK_BADBUTTONEXP,
	
	// {PE-0046} click: bad location expression
	PE_CLICK_BADLOCATIONEXP,
	
	// {PE-0047} click: missing 'at'
	PE_CLICK_NOAT,
	
	// {PE-0048} clone: bad name expression
	PE_CLONE_BADCHUNK,
	
	// {PE-0049} clone: bad target expression
	PE_CLONE_BADNAME,
	
	// {PE-0050} close: bad stack expression
	PE_CLOSE_BADEXP,
	
	// {PE-0051} close: bad stack name
	PE_CLOSE_BADNAME,
	
	// {PE-0052} close: no stack type
	PE_CLOSE_NOTYPE,
	
	// {PE-0053} compact: error in stack expression
	PE_COMPACT_BADPARAM,
	
	// {PE-0054} compound: bad parameter
	PE_COMPRESS_BADPARAM,
	
	// {PE-0055} compress: bad parameter
	PE_COMPOUND_BADPARAM,
	
	// {PE-0056} constant: error in value string
	PE_CONSTANT_BADINIT,
	
	// {PE-0057} constant: missing value string
	PE_CONSTANT_NOINIT,
	
	// {PE-0058} convert: 'and' in the wrong place
	PE_CONVERT_BADAND,
	
	// {PE-0059} convert: no container specified
	PE_CONVERT_NOCONTAINER,
	
	// {PE-0060} convert: no format supplied
	PE_CONVERT_NOFORMAT,
	
	// {PE-0061} convert: not a valid format
	PE_CONVERT_NOTFORMAT,
	
	// {PE-0062} convert: missing 'to'
	PE_CONVERT_NOTO,
	
	// {PE-0063} copy: bad destination object expression
	PE_COPY_BADDEST,
	
	// {PE-0064} cos: bad parameter
	PE_COS_BADPARAM,
	
	// {PE-0065} create: bad background or card expression
	PE_CREATE_BADBGORCARD,
	
	// {PE-0066} create: error in file name expression
	PE_CREATE_BADFILENAME,
	
	// {PE-0067} create: can't create this type
	PE_CREATE_BADTYPE,
	
	// {PE-0068} create: not an object type
	PE_CREATE_NOFILENAME,
	
	// {PE-0069} create: no file name supplied
	PE_CREATE_NONAME,
	
	// {PE-0070} create: no alias or directory name supplied
	PE_CREATE_NOTTYPE,
	
	// {PE-0071} create: no object type
	PE_CREATE_NOTYPE,
	
	// {PE-0072} crop: bad image expression
	PE_CROP_BADIMAGE,
	
	// {PE-0073} crop: bad rect expression
	PE_CROP_BADRECT,
	
	// {PE-0074} decompress: bad parameter
	PE_DECOMPRESS_BADPARAM,
	
	// {PE-0075} define: bad object expression
	PE_DEFINE_BADOBJECT,
	
	// {PE-0076} define: invalid property name
	PE_DEFINE_INVALIDNAME,
	
	// {PE-0077} delete: bad file expression
	PE_DELETE_BADFILEEXP,
	
	// {PE-0078} delete: bad variable expression
	PE_DELETE_BADVAREXP,
	
	// {PE-0079} disable: no control specified
	PE_DISABLE_BADCHUNK,
	
	// {PE-0080} divide: bad destination
	PE_DIVIDE_BADDEST,
	
	// {PE-0081} divide: bad expression
	PE_DIVIDE_BADEXP,
	
	// {PE-0082} divide: missing 'by'
	PE_DIVIDE_NOBY,
	
	// {PE-0083} do: bad expression
	PE_DO_BADEXP,
	
	// {PE-0084} do: bad language expression
	PE_DO_BADLANG,
	
	// {PE-0085} doMenu: bad expression
	PE_DOMENU_BADEXP,
	
	// {PE-0086} drag: bad "button" expression
	PE_DRAG_BADBUTTONEXP,
	
	// {PE-0087} drag: bad end location expression
	PE_DRAG_BADENDLOCEXP,
	
	// {PE-0088} drag: bad start location expression
	PE_DRAG_BADSTARTLOCEXP,
	
	// {PE-0089} drag: missing 'from'
	PE_DRAG_NOFROM,
	
	// {PE-0090} drag: missing 'to'
	PE_DRAG_NOTO,
	
	// {PE-0091} driverNames: bad in type expression
	PE_DRIVERNAMES_BADPARAM,
	
	// {PE-0092} drives: bad in type expression
	PE_DRIVES_BADPARAM,
	
	// {PE-0093} edit: missing 'of'
	PE_EDIT_NOOF,
	
	// {PE-0094} edit: missing 'script'
	PE_EDIT_NOSCRIPT,
	
	// {PE-0095} edit: no object specified
	PE_EDIT_NOTARGET,
	
	// {PE-0096} encrypt: bad parameter
	PE_ENCRYPT_BADPARAM,
	
	// {PE-0097} exit: bad destination expression
	PE_EXIT_BADDEST,
	
	// {PE-0098} exit: no destination expression
	PE_EXIT_NODEST,
	
	// {PE-0099} exp10: bad parameter
	PE_EXP10_BADPARAM,
	
	// {PE-0100} exp1: bad parameter
	PE_EXP1_BADPARAM,
	
	// {PE-0101} exp2: bad parameter
	PE_EXP2_BADPARAM,
	
	// {PE-0102} export: bad file name
	PE_EXPORT_BADFILENAME,
	
	// {PE-0103} export: bad mask file name
	PE_EXPORT_BADMASKNAME,
	
	// {PE-0104} export: bad image or image type expression
	PE_EXPORT_BADTYPE,
	
	// {PE-0105} export: missing file name or container
	PE_EXPORT_NOFILE,
	
	// {PE-0106} export: missing 'mask'
	PE_EXPORT_NOMASK,
	
	// {PE-0107} export: missing 'to'
	PE_EXPORT_NOTO,
	
	// {PE-0108} export: no image type specified
	PE_EXPORT_NOTYPE,
	
	// {PE-0109} Expression: bad chunk
	PE_EXPRESSION_BADCHUNK,
	
	// {PE-0110} Expression: bad function
	PE_EXPRESSION_BADFUNCTION,
	
	// {PE-0111} Expression: bad property
	PE_EXPRESSION_BADPROPERTY,
	
	// {PE-0112} Expression: double binary operator
	PE_EXPRESSION_DOUBLEBINOP,
	
	// {PE-0113} Expression: no binary operator
	PE_EXPRESSION_NOBINOP,
	
	// {PE-0114} Expression: missing factor
	PE_EXPRESSION_NOFACT,
	
	// {PE-0115} Expression: missing left operand to binary operator
	PE_EXPRESSION_NOLFACT,
	
	// {PE-0116} Expression: missing ')'
	PE_EXPRESSION_NORPAR,
	
	// {PE-0117} Expression: bad factor
	PE_EXPRESSION_NOTFACT,
	
	// {PE-0118} Expression: unquoted literal
	PE_EXPRESSION_NOTLITERAL,
	
	// {PE-0119} Expression: missing ')' before factor
	PE_EXPRESSION_WANTRPAR,
	
	// {PE-0120} exp: bad parameter
	PE_EXP_BADPARAM,
	
	// {PE-0121} extents: bad parameter
	PE_EXTENTS_BADPARAM,
	
	// {PE-0122} Function: bad parameter
	PE_FACTOR_BADPARAM,
	
	// {PE-0123} Function: missing 'of'
	PE_FACTOR_NOOF,
	
	// {PE-0124} Function: missing '('
	PE_FACTOR_NOLPAREN,
	
	// {PE-0125} Function: missing ')'
	PE_FACTOR_NORPAREN,
	
	// {PE-0126} Function: separator is not a ','
	PE_FACTOR_NOTSEP,
	
	// {PE-0127} filter: bad container
	PE_FILTER_BADDEST,
	
	// {PE-0128} filter: bad pattern expression
	PE_FILTER_BADEXP,
	
	// {PE-0129} filter: expected "with"
	PE_FILTER_NOWITH,
	
	// {PE-0130} find: bad field expression
	PE_FIND_BADFIELD,
	
	// {PE-0131} find: bad string expression
	PE_FIND_BADSTRING,
	
	// {PE-0132} find: no string expression
	PE_FIND_NOSTRING,
	
	// {PE-0133} flip: bad image expression
	PE_FLIP_BADIMAGE,
	
	// {PE-0134} flip: missing direction
	PE_FLIP_NODIR,
	
	// {PE-0135} flushEvents: bad event type expression
	PE_FLUSHEVENTS_BADPARAM,
	
	// {PE-0136} focus: bad object expression
	PE_FOCUS_BADOBJECT,
	
	// {PE-0137} fontNames: bad type expression
	PE_FONTNAMES_BADPARAM,
	
	// {PE-0138} fontSizes: bad font name string
	PE_FONTSIZES_BADPARAM,
	
	// {PE-0139} fontStyle: bad font name or size expression
	PE_FONTSTYLES_BADPARAM,
	
	// {PE-0140} format: bad format string
	PE_FORMAT_BADPARAM,
	
	// {PE-0141} Function: bad form
	PE_FUNCTION_BADFORM,
	
	// {PE-0142} Function: bad object expression
	PE_FUNCTION_BADOBJECT,
	
	// {PE-0143} Function: bad parameter
	PE_FUNCTION_BADPARAMS,
	
	// {PE-0144} Function: can't modify this function
	PE_FUNCTION_CANTMODIFY,
	
	// {PE-0145} Function: missing function or property
	PE_GENERAL_CANTMODIFY,
	
	// {PE-0146} Function: can't modify this token
	PE_GENERAL_NOMODIFY,
	
	// {PE-0147} get: bad expression
	PE_GET_BADEXP,
	
	// {PE-0148} global: not a valid variable name
	PE_GLOBAL_BADNAME,
	
	// {PE-0149} globalLoc: bad point expression
	PE_GLOBALLOC_BADPOINT,
	
	// {PE-0150} go: bad destination chunk
	PE_GO_BADCHUNKDEST,
	
	// {PE-0151} go: bad chunk expression
	PE_GO_BADCHUNKEXP,
	
	// {PE-0152} go: bad chunk order (must be small to large)
	PE_GO_BADCHUNKORDER,
	
	// {PE-0153} go: bad destination type
	PE_GO_BADCHUNKTYPE,
	
	// {PE-0154} go: bad destination expression
	PE_GO_BADDESTEXP,
	
	// {PE-0155} go: bad direct destination 
	PE_GO_BADDIRECT,
	
	// {PE-0156} go: bad window expression
	PE_GO_BADPREP,
	
	// {PE-0157} go: preposition in the wrong place
	PE_GO_BADWINDOW,
	
	// {PE-0158} go: duplicate chunk
	PE_GO_DUPCHUNK,
	
	// {PE-0159} go: no destination specified
	PE_GO_NODEST,
	
	// {PE-0160} go: missing id
	PE_GO_NOID,
	
	// {PE-0161} go: no window mode specified
	PE_GO_NOMODE,
	
	// {PE-0162} grab: bad object expression
	PE_GRAB_BADCHUNK,
	
	// {PE-0163} Handler: bad character between handlers
	PE_HANDLERLIST_BADCHAR,
	
	// {PE-0164} Handler: no end of line
	PE_HANDLERLIST_BADEOL,
	
	// {PE-0165} Handler: error in handler
	PE_HANDLERLIST_BADHANDLER,
	
	// {PE-0166} Handler: error in command
	PE_HANDLER_BADCOMMAND,
	
	// {PE-0167} Handler: end doesn't match handler name
	PE_HANDLER_BADEND,
	
	// {PE-0168} Handler: unexpected end of line
	PE_HANDLER_BADLINE,
	
	// {PE-0169} Handler: bad handler name (may be reserved word)
	PE_HANDLER_BADNAME,
	
	// {PE-0170} Handler: not a valid parameter name
	PE_HANDLER_BADPARAM,
	
	// {PE-0171} Handler: unexpected end of line in parameters
	PE_HANDLER_BADPARAMEOL,
	
	// {PE-0172} Handler: error in variable or constant declaration
	PE_HANDLER_BADVAR,
	
	// {PE-0173} Handler: bad command
	PE_HANDLER_NOCOMMAND,
	
	// {PE-0174} Handler: missing handler name after end
	PE_HANDLER_NOEND,
	
	// {PE-0175} Handler: no name specified
	PE_HANDLER_NONAME,
	
	// {PE-0176} Handler: not a command
	PE_HANDLER_NOTCOMMAND,
	
	// {PE-0177} hasMemory: bad amount expression
	PE_HASMEMORY_BADPARAM,
	
	// {PE-0178} hide: error in visual effect expression
	PE_HIDE_BADEFFECT,
	
	// {PE-0179} hide: error in target object expression
	PE_HIDE_BADTARGET,
	
	// {PE-0180} hostAddress: error in socket expression
	PE_HOSTADDRESS_BADSOCKET,
	
	// {PE-0181} hostAddressToName: error in address expression
	PE_HOSTATON_BADADDRESS,
	
	// {PE-0182} hostName: error in name expression
	PE_HOSTNAME_BADNAME,
	
	// {PE-0183} hostNameToAddress: error in name expression
	PE_HOSTNTOA_BADNAME,
	
	// {PE-0184} if: bad condition
	PE_IF_BADCONDITION,
	
	// {PE-0185} if: unexpected end of line
	PE_IF_BADEOL,
	
	// {PE-0186} if: error in command
	PE_IF_BADSTATEMENT,
	
	// {PE-0187} if: garbage where a command should be
	PE_IF_BADTYPE,
	
	// {PE-0188} if: not a command
	PE_IF_NOTCOMMAND,
	
	// {PE-0189} if: missing 'then'
	PE_IF_NOTHEN,
	
	// {PE-0190} if: missing 'end if'
	PE_IF_WANTEDENDIF,
	
	// {PE-0191} import: bad file or display name
	PE_IMPORT_BADFILENAME,
	
	// {PE-0192} import: bad mask file name
	PE_IMPORT_BADMASKNAME,
	
	// {PE-0193} import: bad image type
	PE_IMPORT_BADTYPE,
	
	// {PE-0194} import: missing file or display name
	PE_IMPORT_NOFILE,
	
	// {PE-0195} import: missing 'from'
	PE_IMPORT_NOFROM,
	
	// {PE-0196} import: missing 'mask'
	PE_IMPORT_NOMASK,
	
	// {PE-0197} import: no image type specified
	PE_IMPORT_NOTYPE,
	
	// {PE-0198} insert: bad object specification
	PE_INSERT_BADOBJECT,
	
	// {PE-0199} insert: expected 'into'
	PE_INSERT_NOINTO,
	
	// {PE-0200} insert: missing 'script'
	PE_INSERT_NOPLACE,
	
	// {PE-0201} insert: expected 'front' or 'back'
	PE_INSERT_NOSCRIPT,
	
	// {PE-0202} intersect: two objects are required
	PE_INTERSECT_NOOBJECT,
	
	// {PE-0203} is: bad chunk type
	PE_IS_BADAMONGTYPE,
	
	// {PE-0204} is: bad validation type
	PE_IS_BADVALIDTYPE,
	
	// {PE-0205} is: missing validation type
	PE_IS_NORIGHT,
	
	// {PE-0206} is: no right operand
	PE_IS_NOVALIDTYPE,
	
	// {PE-0207} isNumber: bad expression
	PE_ISNUMBER_BADPARAM,
	
	// {PE-0208} isoToMac: bad source expression
	PE_ISOTOMAC_BADPARAM,
	
	// {PE-0209} keys: bad variable name
	PE_KEYS_BADPARAM,
	
	// {PE-0210} kill: bad process name
	PE_KILL_BADNAME,
	
	// {PE-0211} kill: no process
	PE_KILL_NOPROCESS,
	
	// {PE-0212} launch: error in application expression
	PE_LAUNCH_BADAPPEXP,
	
	// {PE-0213} length: bad parameter
	PE_LENGTH_BADPARAM,
	
	// {PE-0214} ln: bad parameter
	PE_LN1_BADPARAM,
	
	// {PE-0215} ln1: bad parameter
	PE_LN_BADPARAM,
	
	// {PE-0216} load: error in url expression
	PE_LOAD_BADURLEXP,
	
	// {PE-0217} load: error in message expression
	PE_LOAD_BADMESSAGEEXP,
	
	// {PE-0218} local: error in initialization string
	PE_LOCAL_BADINIT,
	
	// {PE-0219} local: not a valid variable or constant name
	PE_LOCAL_BADNAME,
	
	// {PE-0220} local: name shadows another variable or constant
	PE_LOCAL_SHADOW,
	
	// {PE-0221} localLoc: bad point expression
	PE_LOCALLOC_BADPOINT,
	
	// {PE-0222} lock: no target object specified
	PE_LOCK_NOTARGET,
	
	// {PE-0223} lock: not a valid target object
	PE_LOCK_NOTTARGET,
	
	// {PE-0224} log10: bad parameter
	PE_LOG10_BADPARAM,
	
	// {PE-0225} log2: bad parameter
	PE_LOG2_BADPARAM,
	
	// {PE-0226} longFilePath: bad file expression
	PE_LONGFILEPATH_BADPARAM,
	
	// {PE-0227} macToIso: bad source expression
	PE_MACTOISO_BADPARAM,
	
	// {PE-0228} mark: bad 'field' expression
	PE_MARK_BADFIELD,
	
	// {PE-0229} mark: bad 'finding' string expression
	PE_MARK_BADSTRING,
	
	// {PE-0230} mark: bad 'where' expression
	PE_MARK_BADWHEREEXP,
	
	// {PE-0231} mark: missing 'by' or 'where'
	PE_MARK_NOBYORWHERE,
	
	// {PE-0232} mark: missing 'cards'
	PE_MARK_NOCARDS,
	
	// {PE-0233} mark: missing 'finding'
	PE_MARK_NOFINDING,
	
	// {PE-0234} mark: missing 'finding' string
	PE_MARK_NOSTRING,
	
	// {PE-0235} mark: expected 'by' or 'where'
	PE_MARK_NOTBYORWHERE,
	
	// {PE-0236} mark: expected 'cards'
	PE_MARK_NOTCARDS,
	
	// {PE-0237} matchChunk: bad parameter
	PE_MATCH_BADPARAM,
	
	// {PE-0238} matrixMultiply: bad parameter
	PE_MATRIXMULT_BADPARAM,
	
	// {PE-0239} max: bad parameter
	PE_MAX_BADPARAM,
	
	// {PE-0240} MCISendString: bad source expression
	PE_MCISENDSTRING_BADPARAM,
	
	// {PE-0241} MD5digest: bad source expression
	PE_MD5DIGEST_BADPARAM,
	
	// {PE-0242} median: bad source expression
	PE_ME_THE,
	
	// {PE-0243} me: unexpected 'the'
	PE_MEDIAN_BADPARAM,
	
	// {PE-0244} merge: bad source expression
	PE_MERGE_BADPARAM,
	
	// {PE-0245} min: bad parameter
	PE_MIN_BADPARAM,
	
	// {PE-0246} mouse: bad parameter
	PE_MOUSE_BADPARAM,
	
	// {PE-0247} move: bad object expression
	PE_MOVE_BADOBJECT,
	
	// {PE-0248} move: bad end location expression
	PE_MOVE_BADENDLOCEXP,
	
	// {PE-0249} move: bad start location expression
	PE_MOVE_BADSTARTLOCEXP,
	
	// {PE-0250} move: expected 'messages' or 'waiting'
	PE_MOVE_BADWITHOUT,
	
	// {PE-0251} move: missing 'to'
	PE_MOVE_NOTO,
	
	// {PE-0252} multiply: bad destination
	PE_MULTIPLY_BADDEST,
	
	// {PE-0253} multiply: bad expression
	PE_MULTIPLY_BADEXP,
	
	// {PE-0254} multiply: missing 'by'
	PE_MULTIPLY_NOBY,
	
	// {PE-0255} next: missing 'repeat'
	PE_NEXT_NOREPEAT,
	
	// {PE-0256} next: token is not 'repeat'
	PE_NEXT_NOTREPEAT,
	
	// {PE-0257} numToChar: bad parameter
	PE_NUMTOCHAR_BADPARAM,
	
	// {PE-0258} offset: bad parameter
	PE_OFFSET_BADPARAMS,
	
	// {PE-0259} open: error in message expression
	PE_OPEN_BADMESSAGE,
	
	// {PE-0260} open: bad mode
	PE_OPEN_BADMODE,
	
	// {PE-0261} open: bad file name
	PE_OPEN_BADNAME,
	
	// {PE-0262} open: type must be 'file' or 'process'
	PE_OPEN_BADTYPE,
	
	// {PE-0263} open: no mode specified
	PE_OPEN_NOMODE,
	
	// {PE-0264} open: no type specified
	PE_OPEN_NOTYPE,
	
	// {PE-0265} param: bad expression
	PE_PARAM_BADEXP,
	
	// {PE-0266} param: bad parameter
	PE_PARAM_BADPARAM,
	
	// {PE-0267} Script: garbage character in script
	PE_PARSE_BADCHAR,
	
	// {PE-0268} Script: missing '"' after literal
	PE_PARSE_BADLIT,
	
	// {PE-0269} pass: mismatched or missing message to pass
	PE_PASS_NOMESSAGE,
	
	// {PE-0270} pause: bad clip name expression
	PE_PAUSE_BADCLIP,
	
	// {PE-0271} peerAddress: error in socket expression
	PE_PEERADDRESS_BADSOCKET,
	
	// {PE-0272} place: bad background expression
	PE_PLACE_BADBACKGROUND,
	
	// {PE-0273} place: bad card expression
	PE_PLACE_BADCARD,
	
	// {PE-0274} play: bad clip name expression
	PE_PLAY_BADCLIP,
	
	// {PE-0275} play: bad location expression
	PE_PLAY_BADLOC,
	
	// {PE-0276} play: bad options expression
	PE_PLAY_BADOPTIONS,
	
	// {PE-0277} play: bad tempo expression
	PE_PLAY_BADTEMPO,
	
	// {PE-0278} play: bad stack expression
	PE_PLAY_BADSTACK,
	
	// {PE-0279} pop: bad card expression
	PE_POP_BADCHUNK,
	
	// {PE-0280} pop: preposition in the wrong place
	PE_POP_BADPREP,
	
	// {PE-0281} pop: no card specified
	PE_POP_NOCARD,
	
	// {PE-0282} post: bad source expression
	PE_POST_BADSOURCEEXP,
	
	// {PE-0283} post: missing 'to'
	PE_POST_NOTO,
	
	// {PE-0284} post: bad destination expression
	PE_POST_BADDESTEXP,
	
	// {PE-0285} print: bad target card
	PE_PRINT_BADTARGET,
	
	// {PE-0286} print: bad 'from' expression
	PE_PRINT_BADFROMEXP,
	
	// {PE-0287} print: bad rectangle expression
	PE_PRINT_BADRECTEXP,
	
	// {PE-0288} print: missing 'to'
	PE_PRINT_NOTO,
	
	// {PE-0289} print: bad 'to' expression
	PE_PRINT_BADTOEXP,
	
	// {PE-0290} Properties: bad chunk expression
	PE_PROPERTY_BADCHUNK,
	
	// {PE-0291} Properties: bad array index expression
	PE_PROPERTY_BADINDEX,
	
	// {PE-0292} Properties: can't modify this property
	PE_PROPERTY_CANTMODIFY,
	
	// {PE-0293} Properties: missing 'of'
	PE_PROPERTY_MISSINGOFORIN,
	
	// {PE-0294} Properties: missing 'of' or 'in'
	PE_PROPERTY_MISSINGTARGET,
	
	// {PE-0295} Properties: no such property
	PE_PROPERTY_NOPROP,
	
	// {PE-0296} Properties: token is not a property
	PE_PROPERTY_NOTAPROP,
	
	// {PE-0297} Properties: expecting 'of'
	PE_PROPERTY_NOTOF,
	
	// {PE-0298} Properties: expecting 'of' or 'in'
	PE_PROPERTY_NOTOFORIN,
	
	// {PE-0299} push: bad expression
	PE_PUSH_BADEXP,
	
	// {PE-0300} put: bad chunk
	PE_PUT_BADCHUNK,
	
	// {PE-0301} put: bad expression
	PE_PUT_BADEXP,
	
	// {PE-0302} put: preposition must be 'before', 'into' or 'after'
	PE_PUT_BADPREP,
	
	// {PE-0303} queryRegistry: bad parameter expression
	PE_QUERYREGISTRY_BADPARAM,
	
	// {PE-0304} random: bad parameter
	PE_RANDOM_BADPARAM,
	
	// {PE-0305} read: bad 'at' expression
	PE_READ_BADAT,
	
	// {PE-0306} read: bad condition
	PE_READ_BADCOND,
	
	// {PE-0307} read: bad message expression
	PE_READ_BADMESS,
	
	// {PE-0308} read: bad file name expression
	PE_READ_BADNAME,
	
	// {PE-0309} read: bad source type (should be 'file' or 'process')
	PE_READ_BADTYPE,
	
	// {PE-0310} read: no termination condition specified
	PE_READ_NOCOND,
	
	// {PE-0311} read: missing 'from'
	PE_READ_NOFROM,
	
	// {PE-0312} read: bad termination condition
	PE_READ_NOTCOND,
	
	// {PE-0313} read: missing source type ('file' or 'process')
	PE_READ_NOTYPE,
	
	// {PE-0314} record: missing 'file' or file name
	PE_RECORD_BADFILEEXP,
	
	// {PE-0315} remove: bad object expression
	PE_REMOVE_BADOBJECT,
	
	// {PE-0316} remove: expected 'from'
	PE_REMOVE_NOFROM,
	
	// {PE-0317} remove: expected 'front' or 'back'
	PE_REMOVE_NOPLACE,
	
	// {PE-0318} rename: bad expression
	PE_RENAME_BADEXP,
	
	// {PE-0319} repeat: error in command
	PE_REPEAT_BADCOMMAND,
	
	// {PE-0320} repeat: bad termination condition
	PE_REPEAT_BADCOND,
	
	// {PE-0321} repeat: unexpected end of line in condition	
	PE_REPEAT_BADCONDEOL,
	
	// {PE-0322} repeat: missing condition
	PE_REPEAT_BADCONDTYPE,
	
	// {PE-0323} repeat: unexpected end of line
	PE_REPEAT_BADFORMEOL,
	
	// {PE-0324} repeat: error in 'for' condition
	PE_REPEAT_BADFORMTYPE,
	
	// {PE-0325} repeat: garbage in repeat condition
	PE_REPEAT_BADSTATEMENT,
	
	// {PE-0326} repeat: not a command
	PE_REPEAT_BADTOKEN,
	
	// {PE-0327} repeat: bad 'with' end expression
	PE_REPEAT_BADWITHENDEXP,
	
	// {PE-0328} repeat: bad 'with' start expression
	PE_REPEAT_BADWITHSTARTEXP,
	
	// {PE-0329} repeat: bad 'with' step expression
	PE_REPEAT_BADWITHSTEPEXP,
	
	// {PE-0330} repeat: bad 'with' variable
	PE_REPEAT_BADWITHVAR,
	
	// {PE-0331} repeat: missing 'to' after 'down'
	PE_REPEAT_NODOWNTO,
	
	// {PE-0332} repeat: missing '='
	PE_REPEAT_NOEQUALS,
	
	// {PE-0333} repeat: missing 'in'
	PE_REPEAT_NOOF,
	
	// {PE-0334} repeat: garbage where a command should be
	PE_REPEAT_NOTCOMMAND,
	
	// {PE-0335} repeat: expecting '='
	PE_REPEAT_NOTEQUALS,
	
	// {PE-0336} repeat: expecting 'to'
	PE_REPEAT_NOTWITHTO,
	
	// {PE-0337} repeat: missing 'to'
	PE_REPEAT_NOWITHTO,
	
	// {PE-0338} repeat: missing 'with' variable
	PE_REPEAT_NOWITHVAR,
	
	// {PE-0339} repeat: missing 'end repeat'
	PE_REPEAT_WANTEDENDREPEAT,
	
	// {PE-0340} replace: bad container expression
	PE_REPLACE_BADCONTAINER,
	
	// {PE-0341} replace: bad expression
	PE_REPLACE_BADEXP,
	
	// {PE-0342} replaceText: bad parameter
	PE_REPLACETEXT_BADPARAM,
	
	// {PE-0343} reply: error in message expression
	PE_REPLY_BADEXP,
	
	// {PE-0344} reply: error in keyword expression
	PE_REPLY_BADKEYWORD,
	
	// {PE-0345} request: error in message expression
	PE_REQUEST_BADEXP,
	
	// {PE-0346} request: error in program expression
	PE_REQUEST_BADPROGRAM,
	
	// {PE-0347} request: no 'type' expression
	PE_REQUEST_NOTYPE,
	
	// {PE-0348} request: type must be class, ID, sender, returnID, or data
	PE_REQUEST_NOTTYPE,
	
	// {PE-0349} reset: missing 'paint', 'cursors', or template object
	PE_RESET_NOTYPE,
	
	// {PE-0350} getResources: bad parameter
	PE_RESOURCES_BADPARAM,
	
	// {PE-0351} return: error in expression
	PE_RETURN_BADEXP,
	
	// {PE-0352} rotate: error in image expression
	PE_ROTATE_BADIMAGE,
	
	// {PE-0353} rotate: error in angle expression
	PE_ROTATE_BADANGLE,
	
	// {PE-0354} round: bad parameter
	PE_ROUND_BADPARAM,
	
	// {PE-0355} save: error in stack expression
	PE_SAVE_BADEXP,
	
	// {PE-0356} save: error in file name expression
	PE_SAVE_BADFILEEXP,
	
	// {PE-0357} seek: bad mode (should be 'to' or 'relative')
	PE_SEEK_BADMODE,
	
	// {PE-0358} seek: bad file name expression
	PE_SEEK_BADNAME,
	
	// {PE-0359} seek: expected 'file'
	PE_SEEK_BADTYPE,
	
	// {PE-0360} seek: bad offset expression
	PE_SEEK_BADWHERE,
	
	// {PE-0361} seek: missing 'in'
	PE_SEEK_NOIN,
	
	// {PE-0362} seek: no mode ('to' or 'relative')
	PE_SEEK_NOMODE,
	
	// {PE-0363} seek: missing 'file'
	PE_SEEK_NOTYPE,
	
	// {PE-0364} select: missing target chunk
	PE_SELECT_NOTARGET,
	
	// {PE-0365} select: missing 'of'
	PE_SELECT_NOOF,
	
	// {PE-0366} select: bad target chunk
	PE_SELECT_BADTARGET,
	
	// {PE-0367} selectedButton: missing 'family'
	PE_SELECTEDBUTTON_NOFAMILY,
	
	// {PE-0368} selectedButton: bad card expression
	PE_SELECTEDBUTTON_NOOBJECT,
	
	// {PE-0369} send: bad message or expression
	PE_SEND_BADEXP,
	
	// {PE-0370} send: bad event type expression
	PE_SEND_BADEVENTTYPE,
	
	// {PE-0371} send: bad target chunk or program expression
	PE_SEND_BADTARGET,
	
	// {PE-0372} set: error in expression
	PE_SET_BADEXP,
	
	// {PE-0373} set: no property specified
	PE_SET_NOPROP,
	
	// {PE-0374} set: missing 'the'
	PE_SET_NOTHE,
	
	// {PE-0375} set: missing 'to'
	PE_SET_NOTO,
	
	// {PE-0376} setRegistry: bad parameter expression
	PE_SETREGISTRY_BADPARAM,
	
	// {PE-0377} shell: bad parameter
	PE_SHELL_BADPARAM,
	
	// {PE-0378} shortFilePath: bad file expression
	PE_SHORTFILEPATH_BADPARAM,
	
	// {PE-0379} show: error visual effect expression
	PE_SHOW_BADEFFECT,
	
	// {PE-0380} show: error in location expression
	PE_SHOW_BADLOCATION,
	
	// {PE-0381} show: error in object expression
	PE_SHOW_BADTARGET,
	
	// {PE-0382} sin: bad parameter
	PE_SIN_BADPARAM,
	
	// {PE-0383} sort: bad expression
	PE_SORT_BADEXPRESSION,
	
	// {PE-0384} sort: error in object expression
	PE_SORT_BADTARGET,
	
	// {PE-0385} sort: no object specified
	PE_SORT_NOTARGET,
	
	// {PE-0386} specialFolderPath: bad type expression
	PE_SPECIALFOLDERPATH_BADTYPE,
	
	// {PE-0387} sqrt: bad parameter
	PE_SQRT_BADPARAM,
	
	// {PE-0388} start: error in chunk expression
	PE_START_BADCHUNK,
	
	// {PE-0389} start: bad action (should be 'editing' or 'using')
	PE_START_NOTTYPE,
	
	// {PE-0390} start: no action specified ('editing' or 'using')
	PE_START_NOTYPE,
	
	// {PE-0391} Commands: not a valid object expression
	PE_STATEMENT_BADCHUNK,
	
	// {PE-0392} Commands: bad 'in' expression
	PE_STATEMENT_BADINEXP,
	
	// {PE-0393} Commands: not a modifier key
	PE_STATEMENT_BADKEY,
	
	// {PE-0394} Commands: bad parameter
	PE_STATEMENT_BADPARAM,
	
	// {PE-0395} Commands: bad parameters
	PE_STATEMENT_BADPARAMS,
	
	// {PE-0396} Commands: missing ','
	PE_STATEMENT_BADSEP,
	
	// {PE-0397} Commands: missing modifier key 
	PE_STATEMENT_NOKEY,
	
	// {PE-0398} Commands: expected 'and'
	PE_STATEMENT_NOTAND,
	
	// {PE-0399} Commands: expected ','
	PE_STATEMENT_NOTSEP,
	
	// {PE-0400} stdDev: bad parameters
	PE_STDDEV_BADPARAM,
	
	// {PE-0401} stop: error in chunk expression
	PE_STOP_BADCHUNK,
	
	// {PE-0402} stop: bad action (must be editing, moving, playing, or using)
	PE_STOP_NOTTYPE,
	
	// {PE-0403} stop: no action (must be editing, moving, playing, or using)
	PE_STOP_NOTYPE,
	
	// {PE-0404} subtract: bad destination
	PE_SUBTRACT_BADDEST,
	
	// {PE-0405} subtract: bad expression
	PE_SUBTRACT_BADEXP,
	
	// {PE-0406} subtract: missing 'from'
	PE_SUBTRACT_NOFROM,
	
	// {PE-0407} Stack: bad stack expression
	PE_SUBWINDOW_BADEXP,
	
	// {PE-0408} sum: bad parameter
	PE_SUM_BADPARAM,
	
	// {PE-0409} switch: bad 'case' condition
	PE_SWITCH_BADCASECONDITION,
	
	// {PE-0410} switch: bad condition
	PE_SWITCH_BADCONDITION,
	
	// {PE-0411} switch: error in command
	PE_SWITCH_BADSTATEMENT,
	
	// {PE-0412} switch: garbage where a command should be
	PE_SWITCH_BADTYPE,
	
	// {PE-0413} switch: not a command
	PE_SWITCH_NOTCOMMAND,
	
	// {PE-0414} switch: missing 'end switch'
	PE_SWITCH_WANTEDENDSWITCH,
	
	// {PE-0415} tan: bad parameter
	PE_TAN_BADPARAM,
	
	// {PE-0416} Operators there: bad file expression
	PE_THERE_BADFILE,
	
	// {PE-0417} Operators there: missing 'is'
	PE_THERE_NOIS,
	
	// {PE-0418} Operators there: no object specified
	PE_THERE_NOOBJECT,
	
	// {PE-0419} throw: bad error expression
	PE_THROW_BADERROR,
	
	// {PE-0420} toLower: bad parameter
	PE_TOLOWER_BADPARAM,
	
	// {PE-0421} topStack: bad parameter
	PE_TOPSTACK_BADPARAM,
	
	// {PE-0422} toUpper: bad parameter
	PE_TOUPPER_BADPARAM,
	
	// {PE-0423} transpose: bad variable parameter
	PE_TRANSPOSE_BADPARAM,
	
	// {PE-0424} trunc: bad parameter
	PE_TRUNC_BADPARAM,
	
	// {PE-0425} try: error in command
	PE_TRY_BADSTATEMENT,
	
	// {PE-0426} try: garbage where a command should be
	PE_TRY_BADTYPE,
	
	// {PE-0427} try: not a command
	PE_TRY_NOTCOMMAND,
	
	// {PE-0428} try: missing 'end try'
	PE_TRY_WANTEDENDTRY,
	
	// {PE-0429} type: bad string expression
	PE_TYPE_BADEXP,
	
	// {PE-0430} ungroup: bad group expression
	PE_UNGROUP_BADGROUP,
	
	// {PE-0431} uniDecode: bad parameter
	PE_UNIDECODE_BADPARAM,
	
	// {PE-0432} uniEncode: bad parameter
	PE_UNIENCODE_BADPARAM,
	
	// {PE-0433} unload: bad url expression
	PE_UNLOAD_BADURLEXP,
	
	// {PE-0434} unlock: not a visual effect
	PE_UNLOCK_BADEFFECT,
	
	// {PE-0435} unlock: no type specified ('screen', 'recent' or 'messages')
	PE_UNLOCK_NOTARGET,
	
	// {PE-0436} unlock: bad type specified ('screen', 'recent' or 'messages')
	PE_UNLOCK_NOTTARGET,
	
	// {PE-0437} urlDecode: bad parameter
	PE_URLDECODE_BADPARAM,
	
	// {PE-0438} urlEncode: bad parameter
	PE_URLENCODE_BADPARAM,
	
	// {PE-0439} urlStatus: bad url expression
	PE_URLSTATUS_BADPARAM,
	
	// {PE-0440} value: bad parameter
	PE_VALUE_BADPARAM,
	
	// {PE-0441} value: bad object expression
	PE_VALUE_BADOBJECT,
	
	// {PE-0442} Array: bad index
	PE_VARIABLE_BADINDEX,
	
	// {PE-0443} Array: missing ']'
	PE_VARIABLE_NORBRACE,
	
	// {PE-0444} visual: double directions specified
	PE_VISUAL_DUPDIRECTION,
	
	// {PE-0445} visual: double effect specified
	PE_VISUAL_DUPVISUAL,
	
	// {PE-0446} visual: expected visual name
	PE_VISUAL_NOTID,
	
	// {PE-0447} visual: missing visual name
	PE_VISUAL_NOTOKEN,
	
	// {PE-0448} visual: not a visual name
	PE_VISUAL_NOTVISUAL,
	
	// {PE-0449} wait: error in condition expression
	PE_WAIT_BADCOND,
	
	// {PE-0450} wait: no duration expression
	PE_WAIT_NODURATION,
	
	// {PE-0451} within: bad object expression
	PE_WITHIN_NOOBJECT,
	
	// {PE-0452} within: bad point expression
	PE_WITHIN_BADPOINT,
	
	// {PE-0453} write: bad 'at' expression
	PE_WRITE_BADAT,
	
	// {PE-0454} write: error in source expression
	PE_WRITE_BADEXP,
	
	// {PE-0455} write: bad file name
	PE_WRITE_BADNAME,
	
	// {PE-0456} write: bad destination (should be 'file or process')
	PE_WRITE_BADTYPE,
	
	// {PE-0457} write: missing 'to'
	PE_WRITE_NOTO,
	
	// {PE-0458} write: no destination (should be 'file or process')
	PE_WRITE_NOTYPE,
	
	// {PE-0459} encrypt/decrypt: source container expected, but not supplied
	PE_ENCRYPTION_NOSOURCE,
	
	// {PE-0460} encrypt/decrypt: cipher name expected, but not supplied
	PE_ENCRYPTION_NOCIPHER,
	
	// {PE-0461} encrypt/decrypt: key expected, but not supplied
	PE_ENCRYPTION_NOKEY,
	
	// {PE-0462} encrypt/decrypt: 'at' clause supplied, but trailing 'bit' missing
	PE_ENCRYPTION_NOBIT,
	
	// {PE-0463} encrypt/decrypt: error parsing source container
	PE_ENCRYPTION_BADSOURCE,
	
	// {PE-0464} encrypt/decrypt: error parsing cipher expression
	PE_ENCRYPTION_BADCIPHER,
	
	// {PE-0465} encrypt/decrypt: error parsing key expression
	PE_ENCRYPTION_BADKEY,
	
	// {PE-0466} encrypt/decrypt: error parsing salt expression
	PE_ENCRYPTION_BADSALT,
	
	// {PE-0467} encrypt/decrypt: error parsing IV expression
	PE_ENCRYPTION_BADIV,
	
	// {PE-0468} encrypt/decrypt: error parsing bit expression
	PE_ENCRYPTION_BADBIT,
	
	// {PE-0469} encrypt/decrypt: invalid clause supplied in this context
	PE_ENCRYPTION_BADPARAM,
	
	// {PE-0470} visual: no parameter
	PE_VISUAL_NOPARAM,
	
	// {PE-0471} visual: bad parameter
	PE_VISUAL_BADPARAM,
	
	// {PE-0472} private: pass not valid in context
	PE_PRIVATE_BADPASS,
	
	// {PE-0473} begins/ends with: bad parameter
	PE_BEGINSENDS_NOWITH,
	
	// {PE-0474} numtobyte: bad parameter
	PE_NUMTOBYTE_BADPARAM,
	
	// {PE-0475} bytetonum: bad parameter
	PE_BYTETONUM_BADPARAM,
	
	// {PE-0476} arraydecode: bad parameter
	PE_ARRAYDECODE_BADPARAM,
	
	// {PE-0477} arrayencode: bad parameter
	PE_ARRAYENCODE_BADPARAM,
	
	// {PE-0478} dispatch: bad message
	PE_DISPATCH_BADMESSAGE,
	
	// {PE-0479} dispatch: bad parameters
	PE_DISPATCH_BADPARAMS,
	
	// {PE-0480} dispatch: bad target
	PE_DISPATCH_BADTARGET,
	
	// {PE-0481} do: unknown target environment
	PE_DO_BADENV,
	
	// {PE-0482} export: bad palette type
	PE_EXPORT_BADPALETTE,
	
	// {PE-0483} printing: missing destination
	PE_OPENPRINTING_NODST,
	
	// {PE-0484} printing: missing filename
	PE_OPENPRINTING_NOFILENAME,
	
	// {PE-0485} printing: bad options expression
	PE_OPENPRINTING_BADOPTIONS,
	
	// {PE-0486} print: bad anchor name expression
	PE_PRINTANCHOR_BADNAMEEXP,
	
	// {PE-0487} print: no anchor location expression
	PE_PRINTANCHOR_NOATEXP,
	
	// {PE-0488} print: bad anchor target expression
	PE_PRINTANCHOR_BADTOEXP,
	
	// {PE-0489} print: no link location expression
	PE_PRINTLINK_NOTOEXP,
	
	// {PE-0490} print: bad link target expression
	PE_PRINTLINK_BADTOEXP,
	
	// {PE-0491} print: no link area expression
	PE_PRINTLINK_NOAREAEXP,
	
	// {PE-0492} print: bad link area expression
	PE_PRINTLINK_BADAREAEXP,
	
	// {PE-0493} Handler: repeated parameter name
	PE_HANDLER_DUPPARAM,
	
	// {PE-0494} 
	PE_INTERNAL_BADVERB,
	
	// {PE-0495} 
	PE_INTERNAL_BADNOUN,
	
	// {PE-0496} 
	PE_INTERNAL_BADEOS,
	
	// {PE-0497} randomBytes: bad parameter expression
	PE_RANDOMBYTES_BADPARAM,
	
	// {PE-0498} sha1Digest: bad parameter expression
	PE_SHA1DIGEST_BADPARAM,
	
	// {PE-0499} print: bad bookmark title expression
	PE_PRINTBOOKMARK_BADTITLEEXP,
	
	// {PE-0500} print: no bookmark level expression
	PE_PRINTBOOKMARK_NOLEVEL,
	
	// {PE-0501} print: bad bookmark level expression
	PE_PRINTBOOKMARK_BADLEVELEXP,
	
	// {PE-0502} print: bad bookmark location expression
	PE_PRINTBOOKMARK_BADATEXP,
	
	// {PE-0503} print: no bookmark expression
	PE_PRINTBOOKMARK_NOBOOKMARK,
	
	// {PE-0504} print: bad bookmark initial state expression
	PE_PRINTBOOKMARK_BADINITIALEXP,
	
	// {PE-0505} script: bad handler declaration
	PE_SCRIPT_BADHANDLER,
	
	// {PE-0506} script: illegal handler type in global script
	PE_SCRIPT_BADHANDLERTYPE,
	
	// {PE-0507} script: bad variable declaration
	PE_SCRIPT_BADVAR,
	
	// {PE-0508} script: bad statement
	PE_SCRIPT_BADSTATEMENT,
	
	// {PE-0509} script: not a statment
	PE_SCRIPT_NOTSTATEMENT,
	
	// {PE-0510} script: bad command
	PE_SCRIPT_BADCOMMAND,
	
	// {PE-0511} script: not a command
	PE_SCRIPT_NOTCOMMAND,
	
	// {PE-0512} script: unexpected eol
	PE_SCRIPT_BADEOL,
	
	// {PE-0513} script: illegal character
	PE_SCRIPT_BADCHAR,
	
	// {PE-0514} include: bad filename expression
	PE_INCLUDE_BADFILENAME,
	
	// {PE-0515} require: bad filename expression
	PE_REQUIRE_BADFILENAME,
	
	// {PE-0516} echo: malformed implicit put
	PE_SCRIPT_BADECHO,
	
	// {PE-0517} lock: bad rect expression
	PE_LOCK_BADRECT,
	
	// {PE-0518} lock: missing 'rect' keyword
	PE_LOCK_NORECT,

	// {PE-0519} relayer: bad source control
	PE_RELAYER_BADCONTROL,

	// {PE-0520} relayer: invalid relation.
	PE_RELAYER_BADRELATION,

	// {PE-0521} relayer: bad target control
	PE_RELAYER_BADTARGET,
	
	// {PE-0522} controlAtLoc: bad location expression
	PE_CONTROLATLOC_BADPARAM,
	
	// {PE-0523} split/combine: bad form clause
	PE_ARRAYOP_BADFORM,
	
	// {PE-0524} uuid: bad parameter list
	PE_UUID_BADPARAM,

	// {PE-0525} avgDev: bad parameters
	PE_AVGDEV_BADPARAM,

	// {PE-0526} geometricMean: bad parameters
	PE_GEO_MEAN_BADPARAM,

	// {PE-0527} harmonicMean: bad parameters
	PE_HAR_MEAN_BADPARAM,

	// {PE-0528} pStdDev: bad parameters
	PE_POP_STDDEV_BADPARAM,

	// {PE-0529} pVariance: bad parameters
	PE_POP_VARIANCE_BADPARAM,

	// {PE-0530} variance: bad parameters
	PE_VARIANCE_BADPARAM,
	
    // MERG-2013-08-14: [[ MeasureText ]] Measure text relative to the effective font on an object
    // {PE-0531} measureText: no object
    PE_MEASURE_TEXT_NOOBJECT,

    // {PE-0532} measureText: bad text parameter
    PE_MEASURE_TEXT_BADTEXT,

    // {PE-0533} measureText: bad mode parameter
    PE_MEASURE_TEXT_BADMODE,

	// MERG-2013-10-04: [[ ResolveImage ]] resolve image relative to object.
    // {PE-0534} resolve image: bad image reference
	PE_RESOLVE_BADIMAGE,

    // {PE-0535} resolve image: bad object reference
	PE_RESOLVE_BADOBJECT,
	
	// MERG-2013-10-04: [[ EditScriptAt ]] edit script of object at.
    // {PE-0536} edit script: no at expression
	PE_EDIT_NOAT,
	
	// MW-2013-11-14: [[ AssertCmd ]] Parsing errors for assert command.
	// {PE-0537} assert: bad type
	PE_ASSERT_BADTYPE,
	
	// {PE-0538} assert: bad expression
	PE_ASSERT_BADEXPR,
    
	// MM-2014-02-12: [[ SecureSocket ]]
	// {PE-0539} secure: missing 'socket'
	PE_SECURE_NOSOCKET,
	
	// MM-2014-02-12: [[ SecureSocket ]]	
	// {PE-0540} secure: bad socket name
	PE_SECURE_BADNAME,
	
	// MM-2014-02-12: [[ SecureSocket ]]	
	// {PE-0541} secure: expected 'verification'
	PE_SECURE_BADMESSAGE,

    // {PE-0542} textDecode: bad parameters
    PE_TEXTDECODE_BADPARAM,
    
    // {PE-0543} textEncode: bad parameters
    PE_TEXTENCODE_BADPARAM,
    
    // {PE-0544} normalizeText: bad parameters
    PE_NORMALIZETEXT_BADPARAM,
    
    // {PE-0545} codepointProperty: bad parameters
    PE_CODEPOINTPROPERTY_BADPARAM,
    
    // SN-2014-05-06 [[ Bug 12360 ]] Bad encoding expression
    // {PE-0546} open: bad encoding parameter
    PE_OPEN_BADENCODING,
    
    // SN-2014-05-06 [[ Bug 12360 ]]
    // {PE-0547} open: no encoding when opening in binary mode
    PE_OPEN_BADBINARYENCODING,
	
	// MM-2014-06-13: [[ Bug 12567 ]] New variant secure socket <socket> with verification for host <host>
	// {PE-0548} secure: bad host name
	PE_SECURE_BADHOST,

	// MM-2014-06-13: [[ Bug 12567 ]] New variant secure socket <socket> with verification for host <host>
	// {PE-0549} secure: expected 'host'
	PE_SECURE_NOHOST,
	
	// MM-2014-06-13: [[ Bug 12567 ]] New variant open socket <socket> with verification for host <host>
	// {PE-0550} open: bad host name
	PE_OPEN_BADHOST,
	
	// MM-2014-06-13: [[ Bug 12567 ]] New variant open socket <socket> with verification for host <host>
	// {PE-0551} open: expected 'host'
	PE_OPEN_NOHOST,
    
    // AL-2014-10-17: [[ BiDi ]] Returns the result of applying the bi-directional algorithm to text
    // {PE-0552} bidiDirection: bad parameters
	PE_BIDIDIRECTION_BADPARAM,
	
	// MDW-2014-08-23: [[ feature_floor ]] floor: error in source expression
	// {PE-0553} floor: error in source expression
	PE_FLOOR_BADPARAM,
	
	// MDW-2014-08-23: [[ feature_floor ]] ceil: error in source expression
	// {PE-0554} ceil: error in source expression
	PE_CEIL_BADPARAM,
    
    // SN-2015-11-15: [[ Bug 165452 ]] New error, if a global variable shadows
    // a local variable declared beforehand
    // {PE-0555} global: shadowing a local variable
    PE_GLOBAL_SHADOW,
    
	// {PE-0556} load: error in extension expression
	PE_LOAD_BADEXTENSION,
	
	// {PE-0557} load: expected 'resource'
	PE_LOAD_NORESOURCE,
	
	// {PE-0558} load: expected 'path'
	PE_LOAD_NOPATH,
	
	// {PE-0559} load: error in resource path expression
	PE_LOAD_BADRESOURCEPATH,
    
    // {PE-0560} load: missing file or display name
    PE_LOAD_NOFILE,
    
    // {PE-0561} load: missing 'from'
    PE_LOAD_NOFROM,

    // {PE-0562} is strictly: missing 'an' or 'a'
    PE_ISSTRICTLY_NOAN,
    
    // {PE-0563} is strictly: missing 'string'
    PE_ISSTRICTLY_NOSTRING,
    
    // {PE-0564} is strictly: missing type
    PE_ISSTRICTLY_NOTYPE,
    
    // {PE-0565} import: no array expression
    PE_IMPORT_NOARRAY,
    
    // {PE-0566} export: no array expression
    PE_EXPORT_NOARRAY,
	
	// {PE-0567} go: error in widget expression
	PE_GO_BADWIDGETEXP,
	
	// {PE-0568} launch: error in widget expression
	PE_LAUNCH_BADWIDGETEXP,

	// {PE-0569} save: error in format expression
	PE_SAVE_BADFORMATEXP,
	
	// {PE-0570} replace: missing 'styles'
	PE_REPLACE_NOSTYLES,
    
    // {PE-0571} revert: bad stack expression
    PE_REVERT_BADSTACK,
	
    // {PE-0572} vectordot: bad parameter
    PE_VECTORDOT_BADPARAM,
    
    // {PE-0573} return: form not allowed in handler type
    PE_RETURN_BADFORMINCONTEXT,
    
    // {PE-0574} return: form not allowed in handler type
    PE_RETURN_BADFOR,

	// {PE-0575} files: bad folder expression
	PE_FILES_BADPARAM,

	// {PE-0576} folders: bad folder expression
	PE_FOLDERS_BADPARAM,

    // {PE-0577} open: expected 'from' address
    PE_OPEN_NOFROM,

    // {PE-0578} messageDigest: bad parameters
    PE_MESSAGEDIGEST_BADPARAM,
    
    // {PE-0579} setop: missing 'difference'
    PE_ARRAYOP_NODIFFERENCE,
    
    // {PE-0580} setop: 'recursive' only makes sense for union or intersect
    PE_ARRAYOP_BADRECURSIVE,
    
    // {PE-0581} setop: destination is not a container (did you mean to use 'into'?)
    PE_ARRAYOP_DSTNOTCONTAINER,
	
    // {PE-0582} send: can't send script in time
    PE_SEND_SCRIPTINTIME,
    
    // {PE-0583} fontLanguage: bad type expression
    PE_FONTLANGUAGE_BADPARAM,
    
    // {PE-0584} out of memory
    PE_OUTOFMEMORY,
};

extern const char *MCparsingerrors;

#endif

