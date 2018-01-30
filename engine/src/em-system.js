/*                                                              -*-Javascript-*-
 
 Copyright (C) 2016 LiveCode Ltd.
 
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
          
    $LiveCodeSystem__deps: ['$LiveCodeUtil', '$LiveCodeAsync'],
    $LiveCodeSystem: {
		
		initialize: function() {
			if (!document['liveCode'])
				document['liveCode'] = {}
			document['liveCode']['findStackWithName'] = function(name) {
				return LiveCodeSystem.getStackWithName(name);
			};
		},
		
		finalize: function() {
			delete document['liveCodeGetStackWithName'];
		},
		
		//////////
		
		evaluateJavaScriptWithArguments: function(pScriptRef, pArgsRef, rResultRef) {
			var script = LiveCodeUtil.stringFromMCStringRef(pScriptRef);
			var args = [];
			if (pArgsRef)
				args = LiveCodeUtil.properListToJSArray(pArgsRef);
				
			// Evaluate javascript and convert result to a valueref.
			var result;
			var success;
			try
			{
				var jsResult = function() {
					return eval(script);
				}.apply(null, args);
				
				result = LiveCodeUtil.valueFromJSValue(jsResult);
				success = true;
			}
			catch (e)
			{
				// return the exception message on failure
				result = LiveCodeUtil.stringToMCStringRef(e.message);
				success = false;
			}
			
			{{{ makeSetValue('rResultRef', '0', 'result', '*') }}};
			return success;
		},

		//////////
		
		_callStackHandler: function(stack, handler, paramList)
		{
			return Module.ccall('MCEmscriptenSystemCallStackHandler', 'number', ['number', 'number', 'number'], [stack, handler, paramList]);
		},
		
		_getStackHandlerList: function(stack)
		{
			return Module.ccall('MCEmscriptenSystemGetJavascriptHandlersOfStack', 'number', ['number'], [stack]);
		},
		
		_makeHandlerProxy: function(stack, handler)
		{
			return function()
			{
				var tParams = Array.prototype.slice.call(arguments);
				var tJSResult;
				LiveCodeAsync.resume(function() {
					var tHandlerName = LiveCodeUtil.stringToMCStringRef(handler);
					var tConvertedParams = LiveCodeUtil.properListFromJSArray(tParams);
					var tResult = LiveCodeSystem._callStackHandler(stack, tHandlerName, tConvertedParams);
					LiveCodeUtil.valueRelease(tHandlerName);
					LiveCodeUtil.valueRelease(tConvertedParams);

					if (tResult)
					{
						tJSResult = LiveCodeUtil.valueToJSValue(tResult);
						LiveCodeUtil.valueRelease(tResult);
					}
				});
				
				return tJSResult;
			}
		},
		
		_stackHandle: function(stack)
		{
			var stackHandle = { _stack: stack };
			var handlerList = LiveCodeSystem._getStackHandlerList(stack);
			var count = LiveCodeUtil.properListGetLength(handlerList);
			for (var i = 0; i < count; i++)
			{
				var stringref = LiveCodeUtil.properListFetchElementAtIndex(handlerList, i);
				var handler_name = LiveCodeUtil.stringFromMCStringRef(stringref);
				stackHandle[handler_name] = LiveCodeSystem._makeHandlerProxy(stack, handler_name);
			}
			
			LiveCodeUtil.valueRelease(handlerList);
			
			return stackHandle;
		},
		
		_resolveStack: function(stack_name)
		{
			var stringref = LiveCodeUtil.stringToMCStringRef(stack_name);
			var stack = Module.ccall('MCEmscriptenResolveStack', 'number', ['number'], [stringref]);
			LiveCodeUtil.valueRelease(stringref);
			
			return stack
		},
		
		getStackWithName: function(name)
		{
			var stackHandle = null;
			LiveCodeAsync.resume(function() {
				var stack = LiveCodeSystem._resolveStack(name);
				if (stack != 0)
					stackHandle = LiveCodeSystem._stackHandle(stack);

			});
			return stackHandle;
		},
		
		//////////
		
		callHandlerWithParamList: function(handler, paramList) {
			return Module.ccall('MCEmscriptenSystemCallHandler', 'number', ['number', 'number'], [handler, paramList]);
		},
		
		wrapHandlerRef: function(handlerref) {
			var jsFunction = function() {
				var tParams = Array.prototype.slice.call(arguments);
				var jsResult;
				LiveCodeAsync.resume(function() {
					var paramList = LiveCodeUtil.properListFromJSArray(tParams);
					var result = LiveCodeSystem.callHandlerWithParamList(handlerref, paramList);

					if (paramList)
						LiveCodeUtil.valueRelease(paramList);

					if (result)
					{
						jsResult = LiveCodeUtil.valueToJSValue(result);
						LiveCodeUtil.valueRelease(result);
					}
				});

				return jsResult;
			};
			
			return LiveCodeUtil.objectRefFromJSObject(jsFunction);
		},
    },
    
    MCEmscriptenSystemEvaluateJavaScriptWithArguments__deps: ['$LiveCodeSystem'],
    MCEmscriptenSystemEvaluateJavaScriptWithArguments: function(pScriptRef, pArgsRef, rResultRef) {
		return LiveCodeSystem.evaluateJavaScriptWithArguments(pScriptRef, pArgsRef, rResultRef);
    },
    
    MCEmscriptenSystemWrapHandler__deps: ['$LiveCodeSystem'],
    MCEmscriptenSystemWrapHandler: function(pHandlerRef) {
		return LiveCodeSystem.wrapHandlerRef(pHandlerRef);
	},
	
	MCEmscriptenSystemInitializeJS__deps: ['$LiveCodeSystem'],
	MCEmscriptenSystemInitializeJS: function() {
		LiveCodeSystem.initialize();
		return true;
	},

	MCEmscriptenSystemFinalizeJS__deps: ['$LiveCodeSystem'],
	MCEmscriptenSystemFinalizeJS: function() {
		LiveCodeSystem.finalize();
	},

});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
