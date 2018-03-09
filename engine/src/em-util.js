/*                                                              -*-Javascript-*-

Copyright (C) 2015 LiveCode Ltd.

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

mergeInto(LibraryManager.library, {

	$LiveCodeUtil: {
		
		/*** Object References ***/
		
		// Object reference map
		_objMap: undefined,
		_objMapIndex: 1,

		// Search the object map for the given object, returning the index if found
		_findObjectIndex: function(jsObj) {
			var iter = this._objMap.entries();
			var n = iter.next();
			while (!n.done)
			{
				var keyvalue = n.value;
				n = iter.next();
				
				if (keyvalue[1]['object'] === jsObj)
					return keyvalue[0];
			}
		},

		// create an integer reference for a JavaScript object, suitable
		// for passing to C++ code
		storeObject: function(jsObj) {
			if (!this._objMap)
				this._objMap = new Map();
			
			var index = this._findObjectIndex(jsObj);
			if (index !== undefined)
			{
				this._objMap.get(index)['refCount']++;
				return index;
			}
			
			var index = this._objMapIndex++;
			this._objMap.set(index, {'object':jsObj, 'refCount':1});
			return index;
		},
		
		// return the object referenced by the index
		fetchObject: function(ref) {
			var value = this._objMap.get(ref);
			if (!value)
				return;
			return value['object'];
		},

		// remove the object reference
		releaseObject: function(ref) {
			var value = this._objMap.get(ref);
			if (!value)
				return;
				
			if (--value['refCount'] == 0)
				this._objMap.delete(ref);
		},
		
		/*** ValueRef Support ***/
		
		// Release MCValueRef
		valueRelease: function(valueref)
		{
			Module.ccall('MCValueRelease', null, ['number'], [valueref]);
		},
		
		/*** String Conversion ***/
		
		// Convert a C++ array with UTF-16 encoding to a JavaScript
		// string.
		stringFromUTF16: function(ptr, length) {

			var result = '';

			for (var i = 0; i < length; i++) {
				var codeUnit = {{{ makeGetValue('ptr', 'i*2', 'i16') }}};
				result += String.fromCharCode(codeUnit);
			}

			return result;
		},

		// Convert a JavaScript string to a C++ heap-allocated array
		// with UTF-16 encoding.  The length of the result array is
		// the same as the length of the input string.  The result
		// must be eventually freed with Module._free().
		stringToUTF16: function(str) {
			var length = str.length;
			var resultPtr = Module._malloc(length * 2);

			for (var i = 0; i < length; i++) {
				var codeUnit = str.charCodeAt(i);
				{{{ makeSetValue('resultPtr', 'i*2', 'codeUnit', 'i16') }}};
			}

			return resultPtr;
		},
		
		// Convert an MCStringRef to a JavaScript string
		stringFromMCStringRef: function(stringref)
		{
			var result = '';
			var t_length = Module.ccall('MCStringGetLength', 'number', ['number'], [stringref]);
			for (var i = 0; i < t_length; i++)
			{
				var codeUnit = Module.ccall('MCStringGetCharAtIndex', 'number', ['number', 'number'] , [stringref, i]);
				result += String.fromCharCode(codeUnit);
			}

			return result;
		},
		
		// Convert a JavaScript string to an MCStringRef
		stringToMCStringRef: function(str)
		{
			var charPtr = LiveCodeUtil.stringToUTF16(str);
			return Module.ccall('MCEmscriptenUtilCreateStringWithCharsAndRelease', 'number', ['number', 'number'], [charPtr, str.length]);
		},
		
		/*** Number conversion ***/
		
		numberToJSValue: function(numberref)
		{
			return Module.ccall('MCNumberFetchAsReal', 'number', ['number'], [numberref]);
		},
		
		numberFromJSValue: function(number)
		{
			return Module.ccall('MCEmscriptenUtilCreateNumberWithReal', 'number', ['number'], [number]);
		},

		/*** Boolean conversion ***/
		booleanToJSValue: function(booleanref)
		{
			return Module.ccall('MCEmscriptenUtilGetBooleanValue', 'number', ['number'], [booleanref]) ? true : false;
		},
		
		booleanFromJSValue: function(boolean)
		{
			return Module.ccall('MCEmscriptenUtilCreateBoolean', 'number', ['number'], [boolean]);
		},

		/*** ProperList Support ***/
		
		// Create mutable (proper) list
		properListCreateMutable: function()
		{
			return Module.ccall('MCEmscriptenUtilCreateMutableProperList', 'number', [], []);
		},
		
		properListPushElementOntoBack: function(listref, valueref)
		{
			return Module.ccall('MCProperListPushElementOntoBack', 'number', ['number', 'number'], [listref, valueref]);
		},
		  
		properListGetLength: function(listref)
		{
			return Module.ccall('MCProperListGetLength', 'number', ['number'], [listref]);
		},
		  
		properListFetchElementAtIndex: function(listref, index)
		{
			return Module.ccall('MCProperListFetchElementAtIndex', 'number', ['number', 'number'], [listref, index]);
		},

		properListToJSArray: function(listref)
		{
			var array = [];
			var len = this.properListGetLength(listref);
			
			for (var i = 0; i < len; i++)
			{
				var value = this.properListFetchElementAtIndex(listref, i);
				var jsValue = this.valueToJSValue(value);
				array.push(jsValue);
			}
			
			return array;
		},
		
		properListFromJSArray: function(array)
		{
			var listref = this.properListCreateMutable();
			var len = array.length;
			for (var i = 0; i < len; i++)
			{
				var value = this.valueFromJSValue(array[i]);
				this.properListPushElementOntoBack(listref, value);
				this.valueRelease(value);
			}
			
			return listref;
		},
		
		/*** Byte array conversion ***/
		
		dataFromJSUint8Array: function(array)
		{
			var memPtr = Module._malloc(array.length);
			Module.writeArrayToMemory(array, memPtr);
			return Module.ccall('MCEmscriptenUtilCreateDataWithBytesAndRelease', 'number', ['number', 'number'], [memPtr, array.length]);
		},
		
		dataToJSUint8Array: function(dataRef)
		{
			var bytePtr = Module.ccall('MCDataGetBytePtr', 'number', ['number'], [dataRef]);
			var byteCount = Module.ccall('MCDataGetLength', 'number', ['number'], [dataRef]);
			return Module.HEAPU8.slice(bytePtr, bytePtr + byteCount);
		},
		
		/*** JavaScript object conversion ***/
		
		objectRefGetID: function(objRef)
		{
			return Module.ccall('MCEmscriptenJSObjectGetID', 'number', ['number'], [objRef]);
		},
		
		objectRefToJSObject: function(objRef)
		{
			var id = this.objectRefGetID(objRef);
			return this.fetchObject(id);
		},
		
		objectRefFromJSObjectID: function(jsObjID)
		{
			return Module.ccall('MCEmscriptenJSObjectFromID', 'number', ['number'], [jsObjID]);
		},
		
		objectRefFromJSObject: function(jsObj)
		{
			var id = this.storeObject(jsObj);
			return this.objectRefFromJSObjectID(id);
		},
		
		valueIsObjectRef: function(valueRef)
		{
			return Module.ccall('MCEmscriptenIsJSObject', 'number', ['number'], [valueRef]);
		},
		
		/*** Type conversion ***/
		
		kMCValueTypeCodeNull:0,
		kMCValueTypeCodeBoolean:1,
		kMCValueTypeCodeNumber:2,
		kMCValueTypeCodeName:3,
		kMCValueTypeCodeString:4,
		kMCValueTypeCodeData:5,
		kMCValueTypeCodeArray:6,
		kMCValueTypeCodeList:7,
		kMCValueTypeCodeSet:8,
		kMCValueTypeCodeProperList:9,
		kMCValueTypeCodeCustom:10,
		kMCValueTypeCodeRecord:11,
		kMCValueTypeCodeHandler:12,
		kMCValueTypeCodeTypeInfo:13,
		kMCValueTypeCodeError:14,
		kMCValueTypeCodeForeignValue:15,
		
		valueGetTypeCode: function(valueref)
		{
			return Module.ccall('MCValueGetTypeCode', 'number', ['number'], [valueref]);
		},
		
		valueToJSValue: function(valueref)
		{
			if (this.valueIsObjectRef(valueref))
				return this.objectRefToJSObject(valueref);
				
			var typecode = this.valueGetTypeCode(valueref);
			switch (typecode)
			{
				case this.kMCValueTypeCodeNull:
					return null;
				case this.kMCValueTypeCodeBoolean:
					return this.booleanToJSValue(valueref);
				case this.kMCValueTypeCodeNumber:
					return this.numberToJSValue(valueref);
				case this.kMCValueTypeCodeString:
					return this.stringFromMCStringRef(valueref);
				case this.kMCValueTypeCodeData:
					return this.dataToJSUint8Array(valueref);
				case this.kMCValueTypeCodeProperList:
					return this.properListToJSArray(valueref);
				/* TODO - support more value types */
				default:
					return this.valueToString(valueref);
			}
		},
		
		valueFromJSValue: function(jsValue)
		{
			if (jsValue === undefined) // Return null value if undefined
				return Module.ccall('MCEmscriptenUtilCreateNull', 'number', [], []);
			else if (jsValue === null)
				return Module.ccall('MCEmscriptenUtilCreateNull', 'number', [], []);
			else if (Array.isArray(jsValue))
				return this.properListFromJSArray(jsValue);
			else if (jsValue instanceof Uint8Array)
				return this.dataFromJSUint8Array(jsValue);
			else
			{
				switch (typeof jsValue)
				{
					case "boolean":
						return this.booleanFromJSValue(jsValue);
					case "number":
						return this.numberFromJSValue(jsValue);
					case "string":
						return this.stringToMCStringRef(jsValue);
					case "function":
					case "object":
						/* TODO - for now, treat functions as objects but we may wish to differentiate them later */
						return this.objectRefFromJSObject(jsValue);
					default:
						return this.stringToMCStringRef(String(jsValue));
				}
			}
		},
		
		valueToMCStringRef: function(valueref)
		{
			return Module.ccall('MCEmscriptenUtilFormatAsString', 'number', ['number'], [valueref]);
		},
		
		valueToString: function(valueref)
		{
			var value = null;
			var string = this.valueToMCStringRef(valueref);
			if (string)
			{
				value = this.stringFromMCStringRef(string);
				this.valueRelease(string);
			}
			return value;
		},
	},
	
	MCEmscriptenUtilReleaseObject__deps: ['$LiveCodeUtil'],
	MCEmscriptenUtilReleaseObject: function(pObjectID)
	{
		LiveCodeUtil.releaseObject(pObjectID);
	},
});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
