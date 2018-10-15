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
          
    $LiveCodeDialog__deps: ['$LiveCodeUtil'],
    $LiveCodeDialog: {
          
        showAlert: function(message_buffer, message_length) {
            var message = LiveCodeUtil.stringFromUTF16(message_buffer, message_length);
            alert(message);
            return 0;
        },
          
        showConfirm: function(message_buffer, message_length) {
            var message = LiveCodeUtil.stringFromUTF16(message_buffer, message_length);
          
            var result = confirm(message);
            return result;
        },
          
          showPrompt: function(message_buffer, message_length, default_buffer, default_length, return_buffer, return_length) {
              var message = LiveCodeUtil.stringFromUTF16(message_buffer, message_length);
              var default_value = LiveCodeUtil.stringFromUTF16(default_buffer, default_length);
          
              // Prompt the user for some text input. If cancelled, will return null.
              var result = prompt(message, default_value);
              if (result === null) {
                  return false;
              }
          
              // Allocate a buffer and store encode the string into it
              var buffer = LiveCodeUtil.stringToUTF16(result);
              var buffer_length = result.length*2;
              {{{ makeSetValue('return_buffer', '0', 'buffer', 'i32') }}};
              {{{ makeSetValue('return_length', '0', 'buffer_length', 'i32') }}};
          
              return true;
          }
    },
    
    MCEmscriptenDialogShowAlert__deps: ['$LiveCodeDialog'],
    MCEmscriptenDialogShowAlert: function(message, message_length) {
        return LiveCodeDialog.showAlert(message, message_length);
    },
    
    MCEmscriptenDialogShowConfirm__deps: ['$LiveCodeDialog'],
    MCEmscriptenDialogShowConfirm: function(message, message_length) {
        return LiveCodeDialog.showConfirm(message, message_length);
    },
          
    MCEmscriptenDialogShowPrompt__deps: ['$LiveCodeDialog'],
    MCEmscriptenDialogShowPrompt: function(message_buffer, message_length, default_buffer, default_length, return_buffer, return_length) {
        return LiveCodeDialog.showPrompt(message_buffer, message_length, default_buffer, default_length, return_buffer, return_length);
    },
});

/*
 * Local Variables:
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 */
