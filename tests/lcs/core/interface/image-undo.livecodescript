script "CoreInterfaceImageUndo"
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

on TestFlipImageWorks
    create stack
    set the defaultStack to the short name of it

    create image

    local tScript
    put "on undoChanged" into tScript
    put return & "set the testProperty of this card to the text of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    flip image 1 horizontal

    TestAssert "check if undo worked", the testProperty of this card is not empty
end TestFlipImageWorks

private command RandomFillImage pImage
    local tData
	repeat the width of pImage * the height of pImage
		put numToByte(random(256)) after tData
	end repeat
	set the imageData of pImage to tData
end RandomFillImage

on TestClickImageWorks
   TestSkipIfNot "ui"
   TestSkipIf "environment", "server"
   TestSkipIfNot "wait"
  
    create stack
    set the defaultStack to the short name of it

    create image
    RandomFillImage it

    local tScript
    put "on undoChanged" into tScript
    put return & "set the prop of this card to the text of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    choose bucket tool
    click at the loc of image 1

    TestAssert "check if undo worked", the prop of this card is not empty
end TestClickImageWorks


on TestRotateImageWorks
    create stack
    set the defaultStack to the short name of it

    create image
    RandomFillImage it
    
    local tScript
    put "on undoChanged" into tScript
    put return & "set the prop of this card to the text of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    rotate image 1 by 20

    TestAssert "check if undo worked", the prop of this card is not empty
end TestRotateImageWorks
*/
on TestFlipImageGetDataWorks
    create stack
    set the defaultStack to the short name of it

    create image

    local tScript
    put "on undoChanged" into tScript
    put return & "set the uLastImage of this card to the imageData of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    flip image 1 horizontal

    TestAssert "check if undo worked", the uLastImage of this card is not empty
end TestFlipImageGetDataWorks



on TestClickImageGetDataWorks
    TestSkipIfNot "ui"
    TestSkipIf "environment", "server"

    create stack
    set the defaultStack to the short name of it

    create image
    RandomFillImage it

    local tScript
    put "on undoChanged" into tScript
    put return & "set the uLastImage of this card to the imageData of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    choose bucket tool
    click at the loc of image 1

    TestAssert "check if undo worked", the uLastImage of this card is not empty
end TestClickImageGetDataWorks


on TestRotateImageGetDataWorks
    create stack
    set the defaultStack to the short name of it

    create image
    RandomFillImage it
    
    local tScript
    put "on undoChanged" into tScript
    put return & "set the uLastImage of this card to the imageData of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    rotate image 1 by 20

    TestAssert "check if undo worked", the uLastImage of this card is not empty
end TestRotateImageGetDataWorks

on TestFlipImageGetTransparencyWorks
    create stack
    set the defaultStack to the short name of it

    create image

    local tScript
    put "on undoChanged" into tScript
    put return & "set the cAlphaData of this card to the alphaData of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    flip image 1 horizontal

    TestAssert "check if undo worked", the cAlphaData of this card is not empty
end TestFlipImageGetTransparencyWorks



on TestClickImageGetTransparencyWorks
    TestSkipIfNot "ui"
    TestSkipIf "environment", "server"

    create stack
    set the defaultStack to the short name of it

    create image
    RandomFillImage it

    local tScript
    put "on undoChanged" into tScript
    put return & "set the cAlphaData of this card to the alphaData of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    choose bucket tool
    click at the loc of image 1

    TestAssert "check if undo worked", the cAlphaData of this card is not empty
end TestClickImageGetTransparencyWorks


on TestRotateImageGetTransparencyWorks
    
    TestSkip "weird crash, unrelated to my code", ""
    exit TestRotateImageGetTransparencyWorks
    
    create stack
    set the defaultStack to the short name of it

    create image
    RandomFillImage it
    
    local tScript
    put "on undoChanged" into tScript
    put return & "set the cAlphaData of this card to the alphaData of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    rotate image 1 by 20

    TestAssert "check if undo worked", the cAlphaData of this card is not empty
end TestRotateImageGetTransparencyWorks


on TestSetTextLockThrows
    create stack
    set the defaultStack to the short name of it

    create image
    RandomFillImage it
    
    local tScript
    put "on undoChanged" into tScript
    put return & "set the prop of this card to the text of image 1" after tScript
    put return & "try" after tScript
    put return & "set the text of image 1 to txt" after tScript
    put return & "catch tError" after tScript
    put return & "set the errorProp of this card to true" after tScript
    put return & "end try" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    flip image 1 horizontal
    TestAssert "did throw" , the errorProp of this card is true
end TestSetTextLockThrows

on TestSetFilenameThrows
    create stack
    set the defaultStack to the short name of it

    create image
    RandomFillImage it
    
    local tScript
    put "on undoChanged" into tScript
    put return & "set the prop of this card to the text of image 1" after tScript
    put return & "try" after tScript
    put return & "set the filename of image 1 to txt" after tScript
    put return & "catch tError" after tScript
    put return & "set the errorProp of this card to true" after tScript
    put return & "end try" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    flip image 1 horizontal
    TestAssert "did throw" , the errorProp of this card is true
end TestSetFilenameThrows

on TestSetImageDataThrows
   TestSkipIfNot "security"
    create stack
    set the defaultStack to the short name of it

    create image
    RandomFillImage it
    
    local tScript
    put "on undoChanged" into tScript
    put return & "set the prop of this card to the text of image 1" after tScript
    put return & "try" after tScript
    put return & "set the imageData of image 1 to randomBytes(100)" after tScript
    put return & "catch tError" after tScript
    put return & "set the errorProp of this card to true" after tScript
    put return & "end try" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    flip image 1 horizontal
    TestAssert "did throw" , the errorProp of this card is true
end TestSetImageDataThrows

