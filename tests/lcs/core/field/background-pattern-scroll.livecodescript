script "CoreFieldBackgroundPatternScroll"
/*
Copyright (C) 2017 LiveCode Ltd.

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

on TestBackgroundPatternScroll
   create stack "Test"
   set the defaultStack to "Test"

   -- Create black and white stripes (3 pixels then 4 pixels).
   local tImageData
   put empty into tImageData
   repeat 3 times
      put numToChar(0) & numToChar(0) & numToChar(0) & numToChar(0) after tImageData
   end repeat
   repeat 4 times
      put numToChar(0) & numToChar(255) & numToChar(255) & numToChar(255) after tImageData
   end repeat
   set the lockLocation of the templateImage to true
   create image "BgPattern"
   set the rect of image "BgPattern" to 0,0,1,7
   set the imageData of image "BgPattern" to tImageData
   hide image "BgPattern"

   -- Create the test field and make it have formattedHeight of more than
   -- 64k pixels
   create field "TestField"
   set the lockLocation of the last field to true
   set the rect of the last field to 0,0,1,7
   set the backgroundPattern of the last field to the id of image "BgPattern"
   set the showBorder of the last field to false
   set the fixedLineHeight of the last field to true
   set the textHeight of the last field to 1024
   set the vScollbar of the last field to false
   repeat with i = 0 to 127
      put return after the last field
   end repeat

   -- Set the vScroll to multiples of the height of the background pattern
   -- and ensure it remains showing the entire pattern as originally
   -- described.
   lock messages
   lock screen
   set the caseSensitive to true
   repeat with i = 0 to the formattedHeight of field "TestField" - 15 step 7
      set the vScroll of field "TestField" to i
      import snapshot from field "TestField"
      set the rect of the last image to the rect of the last field
      if the imageData of the last image is not the imageData of image "BgPattern" then
         TestAssert "background pattern drift detected at vScroll " & i, false
         exit repeat
      end if
      delete the last image
   end repeat
   unlock screen
   unlock messages

   delete stack "Test"
end TestBackgroundPatternScroll
