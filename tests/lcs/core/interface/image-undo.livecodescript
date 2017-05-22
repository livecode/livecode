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

    TestAssert "didn't throw", true
end TestFlipImageWorks

on TestClickImageWorks
    create stack
    set the defaultStack to the short name of it

    create image
    set the imageData of it to randomBytes(10000)

    local tScript
    put "on undoChanged" into tScript
    put return & "set the prop of this card to the text of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    choose bucket tool
    click at the loc of image 1

    TestAssert "check if there is an image", there is an image 1

    TestAssert "check if undo worked", the prop of this card is not empty

    TestAssert "didn't throw", true
end TestClickImageWorks


on TestRotateImageWorks
    create stack
    set the defaultStack to the short name of it

    create image
    set the imageData of it to randomBytes(10000)
    
    local tScript
    put "on undoChanged" into tScript
    put return & "set the prop of this card to the text of image 1" after tScript
    put return & "end undoChanged" after tScript
    set the script of this card to tScript

    rotate image 1 by 20

    TestAssert "check if there is an image", there is an image 1

    TestAssert "check if undo worked", the prop of this card is not empty

    TestAssert "didn't throw", true
end TestRotateImageWorks
