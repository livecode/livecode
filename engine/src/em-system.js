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
          
          evaluateJavaScript: function(script_buffer, script_length, return_buffer, return_length) {
			  var script = LiveCodeUtil.stringFromUTF16(script_buffer, script_length);

			  // Evaluate javascript and convert result to a string.
			  var success = true;
			  var result;
			  try
			  {
				  result = eval(script);
				
				  // Return empty string for Undefined
				  if (typeof result === "undefined")
					  result = "";
				  // Fail if return value isn't supported
				  if (["boolean", "number", "string"] . indexOf(typeof result) < 0)
				      success = false;
				      
				  // Convert to string for return
				  result = String(result);
			  }
			  catch (e)
			  {
				  // Return the exception message on failure
				  result = e.message;
				  success = false;
			  }
			  
              // Allocate a buffer and store encode the string into it
              var buffer = LiveCodeUtil.stringToUTF16(result);
              var buffer_length = result.length*2;
              {{{ makeSetValue('return_buffer', '0', 'buffer', 'i32') }}};
              {{{ makeSetValue('return_length', '0', 'buffer_length', 'i32') }}};
          
              return success;
          },
		
		//////////
		
		_callStackHandler: function(stack, handler, paramList)
		{
			Module.ccall('MCEmscriptenSystemPostStackHandlerCall', null, ['number', 'number', 'number'], [stack, handler, paramList]);
		},
		
		_getStackHandlerList: function(stack)
		{
			return Module.ccall('MCEmscriptenSystemGetJavascriptHandlersOfStack', 'number', ['number'], [stack]);
		},
		
		// Convert from javascript list to MCProperListRef
		_convertParams: function(paramList)
		{
			var listRef = LiveCodeUtil.properListCreateMutable();
			var count = paramList.length;
			
			for (var i = 0; i < count; i++)
			{
				// For now, convert parameters to strings
				// TODO - support other types
				stringRef = LiveCodeUtil.stringToMCStringRef(String(paramList[i]));
				LiveCodeUtil.properListPushElementOntoBack(listRef, stringRef);
				LiveCodeUtil.valueRelease(stringRef)
			}
			
			return listRef
		},
		  
		_makeHandlerProxy: function(stack, handler)
		{
			return function()
			{
				var tParams = Array.prototype.slice.call(arguments);
				LiveCodeAsync.resume(function() {
					var tHandlerName = LiveCodeUtil.stringToMCStringRef(handler);
					var tConvertedParams = LiveCodeSystem._convertParams(tParams);
					LiveCodeSystem._callStackHandler(stack, tHandlerName, tConvertedParams);
					LiveCodeUtil.valueRelease(tHandlerName);
					LiveCodeUtil.valueRelease(tConvertedParams);
				});
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
    },
    
    MCEmscriptenSystemEvaluateJavaScript__deps: ['$LiveCodeSystem'],
    MCEmscriptenSystemEvaluateJavaScript: function(script_buffer, script_length, return_buffer, return_length) {
		return LiveCodeSystem.evaluateJavaScript(script_buffer, script_length, return_buffer, return_length);
    },
});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
