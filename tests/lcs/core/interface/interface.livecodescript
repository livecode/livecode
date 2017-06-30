script "CoreInterface"
/*
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

constant kCreatedField = "TestField"

// Creates a stack with a field, and put the text pText into it
private command __createFieldWithText pText
   create stack "Test"
   set the defaultStack to "Test"
   
   create field kCreatedField
   set the text of field kCreatedField to pText
end __createFieldWithText

on TestCapsLockKey
   TestAssert "CapsLock", the capsLockKey is among the items of "down,up"
end TestCapsLockKey


on TestColorNames
   TestAssert "the colorNames return 552 lines", the number of lines of the colornames is 552
end TestColorNames


on TestCommandKey
   TestAssert "CommandKey", the commandKey is among the items of "down,up"
end TestCommandKey


on TestFoundChunk
   __createFieldWithText "abcd"
   
   find characters "b" in field kCreatedField
   TestAssert "foundChunk for found chunk", the foundChunk is "char 2 to 2 of field 1" or the foundChunk is "char 2 to 2 of field 2"
   
   find characters "x" in field kCreatedField
   TestAssert "foundChunk for non-found chunk", the foundChunk is empty
end TestFoundChunk


on TestFoundField
   __createFieldWithText "abcd"
   
   find characters "b" in field kCreatedField
   TestAssert "foundField for found chunk", the foundField is "field 1" or the foundField is "field 2"
   
   find characters "x" in field kCreatedField
   TestAssert "foundField for non-found chunk", the foundField is empty
end TestFoundField


on TestFoundLine
   __createFieldWithText "abcd" & return & "efgh"
   
   find "efgh" in field kCreatedField
   TestAssertBroken "foundLine for found chunk", false /*the foundLine is "line 2 of field 1" or the foundLine is "line 2 of field 2"*/, "16178"
   
   find characters "x" in field kCreatedField
   TestAssertBroken "foundLine for non-found chunk", false /*the foundLine is empty*/, "16178"
end TestFoundLine


on TestFoundLoc
   __createFieldWithText "abcd"
   select char 2 to 2 of field kCreatedField
   
   local tCharLoc
   put the selectedloc of field kCreatedField into tCharLoc
   
   find characters "b" in field kCreatedField
   TestAssert "foundLoc for found chunk", the foundLoc is tCharLoc
   
   find characters "x" in field kCreatedField
   TestAssert "foundLoc for non-found chunk", the foundLoc is empty
end TestFoundLoc


on TestFoundText
   __createFieldWithText "abc def"
   
   find "a" in field kCreatedField
   TestAssert "foundText for found chunk", the foundText is "abc"
   
   find "x" in field kCreatedField
   TestAssert "foundText for non-found chunk", the foundText is empty
end TestFoundText


on TestGlobalLoc
   TestAssert "gLobalLoc is topLeft of the stack", globalloc((0,0)) is (the topleft of this stack)
end TestGlobalLoc


private command __testIntersect pImage1, pImage2, pIntersectionType, pShouldIntersect, pExpectedFailure
   local tMess
   local tAlpha1, tAlpha2
   
   put byteToNum(byte 1 of the alphaData of pImage1) into tAlpha1
   put byteToNum(byte 1 of the alphaData of pImage2) into tAlpha2
   
   local tIntersectionMess
   if pIntersectionType is an integer then
      put "alpha >= " & pIntersectionType into tIntersectionMess
   else
      put pIntersectionType into tIntersectionMess
   end if
   
   put merge("Image 1: alpha [[tAlpha1]], image 2: alpha [[tAlpha2]] - [[tIntersectionMess]]") into tMess
   
   local tRes
   put intersect(pImage1, pImage2, pIntersectionType) into tRes
   
   if not pShouldIntersect then
      put not tRes into tRes
   end if
   
   if pExpectedFailure then
      TestAssertBroken tMess, tRes
   else
      TestAssert tMess, tRes
   end if
end __testIntersect

on TestImageIntersect
   local tImage1, tImage2
   create image
   put the long id of the last image into tImage1
   create image
   put the long id of the last image into tImage2
   
   set the rect of tImage1 to 0,0,10,10
   set the rect of tImage2 to 1,1,10,10
   
   local tZeroAlpha, tHalfAlpha, tFullAlpha, tBlackImage
   
   repeat with i = 1 to 100
      put numToByte(0) after tZeroAlpha
      put numToByte(127) after tHalfAlpha
      put numToByte(255) after tFullAlpha
      	
      put numToByte(0)&numToByte(0)&numToByte(0)&numToByte(0) after tBlackImage
   end repeat
   
   set the imagedata of tImage1 to tBlackImage
   set the imagedata of tImage2 to tBlackImage
   
   set the alphadata of tImage1 to tZeroAlpha
   set the alphadata of tImage2 to tZeroAlpha
   
   __testIntersect tImage1, tImage2, "bounds", true
   __testIntersect tImage1, tImage2, "pixels", false
   __testIntersect tImage1, tImage2, "opaque pixels", false
   
   set the alphadata of tImage2 to tHalfAlpha
   __testIntersect tImage1, tImage2, "bounds", true
   __testIntersect tImage1, tImage2, "pixels", false
   __testIntersect tImage1, tImage2, "opaque pixels", false
   
   set the alphadata of tImage2 to tFullAlpha
   __testIntersect tImage1, tImage2, "bounds", true
   __testIntersect tImage1, tImage2, "pixels", false
   __testIntersect tImage1, tImage2, "opaque pixels", false
   
   set the alphadata of tImage1 to tHalfAlpha
   set the alphadata of tImage2 to tHalfAlpha
   __testIntersect tImage1, tImage2, "bounds", true
   __testIntersect tImage1, tImage2, "pixels", true
   __testIntersect tImage1, tImage2, "opaque pixels", false
   __testIntersect tImage1, tImage2, 127, true
   __testIntersect tImage1, tImage2, 128, false
   
   set the alphadata of tImage1 to tHalfAlpha
   set the alphadata of tImage2 to tFullAlpha
   __testIntersect tImage1, tImage2, "bounds", true
   __testIntersect tImage1, tImage2, "pixels", true
   __testIntersect tImage1, tImage2, "opaque pixels", false
   __testIntersect tImage1, tImage2, 127, true, true // Broken
   __testIntersect tImage1, tImage2, 128, false
   
   set the alphadata of tImage1 to tFullAlpha
   set the alphadata of tImage2 to tHalfAlpha
   __testIntersect tImage1, tImage2, "bounds", true
   __testIntersect tImage1, tImage2, "pixels", true
   __testIntersect tImage1, tImage2, "opaque pixels", false
   __testIntersect tImage1, tImage2, 127, true, true // Broken
   __testIntersect tImage1, tImage2, 128, false
   
   set the alphadata of tImage1 to tFullAlpha
   set the alphadata of tImage2 to tFullAlpha
   __testIntersect tImage1, tImage2, "bounds", true
   __testIntersect tImage1, tImage2, "pixels", true
   __testIntersect tImage1, tImage2, "opaque pixels", true
   __testIntersect tImage1, tImage2, 127, true
   
   set the topleft of tImage2 to the topright of tImage1
   TestAssert "Images no overlapping", not intersect(tImage1, tImage2, "bounds")
end TestImageIntersect


on TestLocalLoc
   TestAssert "LocalLoc", localloc(the topleft of this stack) is (0,0)
end TestLocalLoc


on TestMainstack
   TestAssert "Mainstack of this stack in the mainstacks", the mainstack of this stack is among the lines of the mainstacks
end TestMainstack


on TestMouse
   TestAssert "the mouse", the mouse is among the items of "up,down"
end TestMouse


on TestMovingControls
   TestSkipIfNot "wait"

   TestAssert "No moving controls", the movingControls is empty
   create button
   move button 1 relative 50,50 in 100 milliseconds without waiting
   TestAssert "Moving control is referenced", the long id of button 1 is among the lines of the movingControls
   wait 150 milliseconds
   
   TestAssert "Control removed from the movingControls list", the movingControls is empty
end TestMovingControls


on TestOpenStacks
   open this stack
   TestAssert "test", the short name of this stack is among the lines of the openStacks
end TestOpenStacks


on TestOptionKey
TestAssert "optionKey", the optionKey is among the items of "up,down"
end TestOptionKey


on TestScreenColors
TestAssert "the screenColors is the screndepth ^ 2", the screencolors is the exp2 of the screendepth
end TestScreenColors


on TestScreenDepth
   TestAssert "test", the screendepth is among the items of "0,1,2,4,8,16,24,32"
end TestScreenDepth


on TestScreenLoc
   local tRect, tLoc
   put the screenrect into tRect
   put the screenloc into tLoc
   
   TestAssert "the screenloc is 2 integers", the number of items in tLoc is 2 and \
         item 1 of tLoc is an integer and \
         item 2 of tLoc is an integer
   
   TestAssert "screenLoc is in the middle of the screen", item 1 of tLoc is ((item 1 of tRect + item 3 of tRect) div 2) and \
         item 2 of tLoc is ((item 2 of tRect + item 4 of tRect) div 2)
end TestScreenLoc


on TestScreenRect
   TestAssert "the screenRect is a rectangle", the screenrect is a rect
   
   repeat for each line trect in the screenrects
      TestAssert "Each item of the screenRects is a rectangle", trect is a rect
   end repeat
end TestScreenRect


on TestSelectedChunk
  __createFieldWithText "abcd"
   
   select char 1 to -1 of field kCreatedField
   TestAssert "selectedChunk with selected chunk", the selectedChunk is "char 1 to 4 of field 1" or the selectedChunk is "char 1 to 4 of field 2"
   
   select empty
   TestAssert "selectedChunk without selected chunk", the selectedChunk is empty
end TestSelectedChunk


on TestSelectedField
   __createFieldWithText "abcdef"
   
   select char 1 of field kCreatedField
   TestAssert "selectedField with selected chunk", the selectedField is "field 1" or the selectedField is "field 2"
   
   select empty
   TestAssert "selectedField without selected chunk", the selectedField is empty
end TestSelectedField


on TestSelectedImage
   create stack "Test"
   set the defaultstack to "Test"
   
   TestAssert "selectedImage is empty", the selectedimage is empty
   
   create image
   choose select tool
   drag from the loc of the last image to the bottomright of the last image
   
   TestAssert "image has been selected by 'drag'", the selectedimage is "image 1"
end TestSelectedImage


on TestSelectedLine
  __createFieldWithText "abc" & return & "def"
   
   select char 1 to -1 of field kCreatedField
   TestAssert "selectedLine with selected chunk", the selectedLine is "line 1 to 2 of field 1" or the selectedLine is "line 1 to 2 of field 2"
   
   select empty
   TestAssert "selectedLine without selected chunk", the selectedLine is empty
end TestSelectedLine


on TestSelectedLoc
  __createFieldWithText "abc" & return & "def"
   
   select line 1 of field kCreatedField
   TestAssert "selectedLoc with selected text", the selectedLoc is item 1 to 2 of (the formattedrect of line 1 of field kCreatedField)
   
   select empty
   TestAssert "selectedLoc without selected text", the selectedLoc is empty
end TestSelectedLoc


on TestSelectedObject
   select empty
   
   TestAssert "selectedObject is empty", the selectedobject is empty
   
   create stack "Test"
   set the defaultstack to "Test"
   
   create button
   create field
   
   select button 1 and field 1
   
   TestAssert "Two objects are selected", the number of lines in (the selectedobject) is 2
   TestAssert "The field is in the selectedObjects", the long id of button 1 is among the lines of the selectedobject
   TestAssert "The button is in the selectedObjects", the long id of field 1 is among the lines of the selectedobject
   
   set the selected of field 1 to false
   
   TestAssert "One object is selected", the selectedobject is the long id of button 1
   
   select empty
   
   TestAssert "No object is selected anymore", the selectedObject is empty
end TestSelectedObject


on TestSelectedText
  __createFieldWithText "abc"
   
   select char 1 to -1 of field kCreatedField
   TestAssert "selectedText with selected text", the selectedText is "abc"
   
   select empty
   TestAssert "selectedText without selected text", the selectedText is empty
end TestSelectedText

on TestUncontiguousSelectedText
   __createFieldWithText "abc" & return & "def" & return & "ghi"
   set the listbehavior of field kCreatedField to true
   set the multiplehilites of field kCreatedField to true
   set the noncontiguoushilites of field kCreatedField to true
   
   set the hilitedlines of field kCreatedField to 1,3
   
   TestAssert "selectedText of a list field with selected lines", the selectedText of field kCreatedField is ("abc" & return & "ghi")
   
   set the hilitedLines of field kCreatedField to empty
   TestAssert "selectedText of a list field without selected lines", the selectedText of field kCreatedField is empty
end TestUncontiguousSelectedText



on TestShiftKey
   TestAssert "ShifKey", the shiftKey is among the items of "down,up"
end TestShiftKey


on TestStacks
   create stack "Test"
   set the defaultstack to "Test"
   TestAssert "this stack among among the stacks", the filename of this stack is among the lines of the stacks
end TestStacks


on TestTopStack
   TestAssert "topstack", word 1 of the topStack is "stack"
end TestTopStack


on TestWaitDepth
   TestSkipIfNot "wait"
   
   TestAssert "Waitdepth is an integer", the waitdepth is an integer
   
   global gWaitDepthTestResult
   create button
   set the script of button 1 to "on waitDepthTest" & cr & \
         "global gWaitDepthTestResult" & cr & \
         "put the waitdepth into gWaitDepthTestResult" & cr & \
         "end waitDepthTest"
   
   send "waitDepthTest" to button 1 in 0 seconds
   wait 0 seconds with messages
   
   TestAssert "Waitdepth increases", gWaitDepthTestResult is (1 + the waitdepth)
end TestWaitDepth


on TestWithinGraphics
   create graphic
   set the style of it to "oval"
   set the rect of it to 0,0,200,200
   set the opaque of it to true
   
   TestAssert "test", within(graphic 1, (50, 50))
   TestAssert "test", within(graphic 1, (150, 50))
   TestAssert "test", within(graphic 1, (150, 150))
   TestAssert "test", within(graphic 1, (50, 150))
   
   TestAssert "test", not within(graphic 1, (25, 25))
   TestAssert "test", not within(graphic 1, (175, 25))
   TestAssert "test", not within(graphic 1, (175, 175))
   TestAssert "test", not within(graphic 1, (25, 175))
end TestWithinGraphics

on TestWithinImage
   // create image mask with alternating (checkerboard) opaque / transparent pixels
   // and black imagedata
   
   local tMaskdata, tImageData
   repeat with y = 1 to 100
      repeat with x = 1 to 100
         if (x + y) mod 2 is 0 then
            put numtobyte(255) after tmaskdata
         else
            put null after tmaskdata
         end if
         put null&null&null&null after tImagedata
      end repeat
   end repeat
   
   create image
   set the rect of it to 0, 0, 100, 100
   set the imagedata of it to tImagedata
   set the maskdata of it to tMaskdata
   
   TestAssert "test", within(image 1, (0, 0))
   TestAssert "test", not within(image 1, (1, 0))
   TestAssert "test", within(image 1, (2, 0))
   TestAssert "test", not within(image 1, (2, 1))
   TestAssert "test", within(image 1, (3, 1))
   TestAssert "test", not within(image 1, (3, 2))
end TestWithinImage


on TestCloneInvisible
   create button
   
   clone invisible button 1 as "foo"
   
   TestAssert "Clone is invisible", the short name of it is "foo" and the visible of it is false
end TestCloneInvisible


on TestCloseStackMessage
   global tClosed
   
   put false into tClosed
   
   create stack "testStack"
   
   set the script of stack "testStack" to "global tClosed;on closeStack;put true into tClosed; end closeStack"
   
   set the defaultStack to "testStack"
   
   close the defaultStack
   
   TestAssert "closeStack message sent", tClosed
end TestCloseStackMessage


on TestCopyToGroup
   create button "test button"
   create group "test group"
   
   copy button "test button" to group "test group"

   TestAssert "Button has been copied to the group", "test button" is the childControlNames of group "test group"
end TestCopyToGroup


on TestCopyToCard
   create stack "Test"
   set the defaultStack to "Test"
   
   local tID
   put the long id of the current card into tID
   
   create card "new card"
   
   create button "test button"
   
   copy button "test button" to tID
   
   go to tID
   
   TestAssert "Button has been copied to the card", there is a button "test button"
end TestCopyToCard


on TestCopyToStack
   local tStack
   put the long id of this stack into tStack
   
   create stack "Test2"
   set the defaultStack to "Test2"
   
   create card "test card"
   
   copy card "test card" to stack tStack
   
   set the defaultStack to tStack
   
   TestAssert "Card has been copied to the stack", there is a card "test card"
end TestCopyToStack


on TestCreateCard
   create card "test"
   
   TestAssert "Card created", the short name of it is "test" and there is a card 2
end TestCreateCard


on TestCreateStack
   create stack "test"
   
   TestAssert "Stack created", there is a stack "test"
end TestCreateStack


on TestDeleteCharOfField
   __createFieldWithText "fooosfsdgsdglj"
   
   delete char 4 to -1 of field kCreatedField
   
   TestAssert "chars deleted from field", field 1 is "foo"
end TestDeleteCharOfField


on TestDisable
   create button
   
   disable button 1
   
   TestAssert "disable changes the disabled", the disabled of button 1
end TestDisable


on TestEnable
   create button
   
   set the disabled of button 1 to true
   
   enable button 1
   
   TestAssert "enable changes the disabled", not the disabled of button 1
end TestEnable


on TestFind
   create stack "Test"
   set the defaultstack to "Test"
   
   local tId
   
   put the long id of the current card into tId
   
   create field
   
   put "foo" into field 1
   
   create card
   
   find "foo"
   
   TestAssert "find changes to the card having the searched text", the long id of the current card is tId
end TestFind

on TestFindInNamedField
   create stack "Test"
   set the defaultstack to "Test"
   
   local tId
   
   put the long id of the current card into tId
   
   create field "TestField1"
   
   put "foo" into field "TestField1"
   
   create card
   create field "TestField2"
   
   put "bar" into field "TestField2"
   
   find "foo" in field "TestField1"
   
   TestAssert "find changes to the card having the named text field", the long id of the current card is tId
end TestFindInNamedField

on TestHilite
   create button
   
   hilite button 1
   
   TestAssert "hilite changes the hilited", the hilited of button 1
end TestHilite


on TestPushPopCard
   create stack "Test"
   set the defaultstack to "Test"
   
   create card
   
   create field
   
   put the long id of the current card into field 1
   
   push card
   
   create card
   
   pop card
   
   TestAssert "Popped card is now the current card", (the long id of the current card is field 1)
end TestPushPopCard


on TestPopInContainer
   create stack "Test"
   set the defaultstack to "Test"
   
   create card
   
   create card
   
   push card 1
   
   create field
   
   pop card into field 1
   
   TestAssert "field contains the id of the card", (the long id of card 1 is field 1)
end TestPopInContainer


on TestResetTemplateStack
   set the ID of the templatestack to 2000
   
   reset the templatestack
   
   TestAssert "templateStack id reset", the ID of the templatestack is 1001
end TestResetTemplateStack


on TestSelectText
   __createFieldWithText "foo"
   
   select text of field kCreatedField
   
   TestAssert "selection is the text of the field", the selection is "foo"
end TestSelectText


on TestSelectWord
   __createFieldWithText "foo bar"
   
   select word 1 of field kCreatedField
   
   TestAssert "selection is word 1 of the field", the selection is "foo"
end TestSelectWord




on TestSortField
   __createFieldWithText "b" & CR & "d" & CR & "a" & CR & "c"
   
   sort field kCreatedField
   
   TestAssert "Field has been sorted", field kCreatedField is "a" & LF & "b" & LF & "c" & LF & "d"
end TestSortField


on TestMoveControl
   TestSkipIfNot "wait"

   create button
   
   set the loc of button 1 to 0,0
   
   move button 1 from the loc of button 1 to 10, 10 in 10 without waiting
   
   wait 20
   
   stop moving button 1
   
   wait 8
   
   TestAssert "button has moved", not (the loc of button 1 is 0,0)
end TestMoveControl

-- no key options working properly
on TestUndo
   if the platform is "HTML5" then
      TestSkip "key options", "bug 16544"
      exit TestUndo
   end if

   __createFieldWithText "bar"
   
set the script of field kCreatedField to "on keyDown;select text of me;delete;undo;end keyDown"
   
   type "a"
   
   TestAssert "text deletion is undone", the text of field kCreatedField is "bar"
end TestUndo
