/* Copyright (C) 2014 Runtime Revolution Ltd.
 
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

#ifndef TEXT_API_H
#define TEXT_API_H


#include "foundation.h"
#include "foundation-bidi.h"

extern "C" {


// Opaque reference types for the MCText... classes
typedef class MCTextCell*           MCTextCellRef;
typedef class MCTextPane*           MCTextPaneRef;          // inherits MCTextCell
typedef class MCTextParagraph*      MCTextParagraphRef;     // inherits MCTextCell
typedef class MCTextLine*           MCTextLineRef;          // inherits MCTextCell
typedef class MCTextSegment*        MCTextSegmentRef;       // inherits MCTextCell
typedef class MCTextBlock*          MCTextBlockRef;         // inherits MCTextCell
typedef class MCTextRun*            MCTextRunRef;           // inherits MCTextBlock
typedef class MCTextBreakBlock*     MCTextBreakBlockRef;    // inherits MCTextBlock
typedef class MCTextControlBlock*   MCTextControlBlockRef;  // inherits MCTextBlock



// The different types of text cells supported by the engine
enum MCTextCellType
{
    kMCTextCellTypeBlock,       // Cell comprised of a single run of text, a single image or other primitive
    kMCTextCellTypeSegment,     // Cell delimited by tab characters. Contains blocks.
    kMCTextCellTypeLine,        // Cell delimited by line breaking characters. Contains segments.
    kMCTextCellTypeParagraph,   // Cell delimited by paragraph breaking characters. Contains lines.
    kMCTextCellTypePane,        // Rectangle with text. Contains paragraphs.
};


// Text cell flags
typedef uint32_t MCTextCellFlags;
enum
{
    kMCTextCellNeedsLayout  = 0x00000001,   // Cell has not been laid out
};


// Alignment types
typedef uint8_t MCTextCellAlignment;
enum
{
    kMCTextCellAlignStart,          // Align to the start of the cell
    kMCTextCellAlignEnd,            // Align to the end of the cell
    kMCTextCellAlignCenter,         // Centre within the cell
    kMCTextCellAlignJustify         // Stretch to fill the cell
};


// Layout directions
typedef uint8_t MCTextCellLayoutDirection;
enum
{
    kMCTextCellLayoutLeftThenDown,
    kMCTextCellLayoutLeftThenUp,
    kMCTextCellLayoutRightThenDown,
    kMCTextCellLayoutRightThenUp,
    kMCTextCellLayoutDownThenLeft,
    kMCTextCellLayoutDownThenRight,
    kMCTextCellLayoutUpThenLeft,
    kMCTextCellLayoutUpThenRight,
};


struct MCTextAttributes
{
    
};

////////////////////////////////////////////////////////////////////////////////

// MCTextCell accessors
MCTextCellType              MCTextCellGetType(MCTextCellRef);
MCTextCellRef               MCTextCellGetParent(MCTextCellRef);
MCTextCellRef               MCTextCellGetChildren(MCTextCellRef);
coord_t                     MCTextCellGetX(MCTextCellRef);
coord_t                     MCTextCellGetY(MCTextCellRef);
coord_t                     MCTextCellGetWidth(MCTextCellRef);
coord_t                     MCTextCellGetHeight(MCTextCellRef);
coord_t                     MCTextCellGetMaxWidth(MCTextCellRef);
coord_t                     MCTextCellGetMaxHeight(MCTextCellRef);
MCTextCellAlignment         MCTextCellGetHorizontalAlignment(MCTextCellRef);
MCTextCellAlignment         MCTextCellGetVerticalAlignment(MCTextCellRef);
MCTextCellLayoutDirection   MCTextCellGetLayoutDirection(MCTextCellRef);
MCTextDirection             MCTextCellGetTextDirection(MCTextCellRef);
void                        MCTextCellSetPosition(MCTextCellRef, coord_t p_x, coord_t p_y);
void                        MCTextCellSetMaxSize(MCTextCellRef, coord_t p_width, coord_t p_height);
void                        MCTextCellSetAlignment(MCTextCellRef, MCTextCellAlignment p_horizontal, MCTextCellAlignment p_vertical);
void                        MCTextCellSetLayoutDirection(MCTextCellRef, MCTextCellLayoutDirection p_direction);
void                        MCTextCellSetTextDirection(MCTextCellRef, MCTextDirection p_direction);


////////////////////////////////////////////////////////////////////////////////

// Creates a new, empty text pane. The specified stack is used for scaling and
// theming only - the pane isn't a LiveCode control and isn't a child of the
// stack.
bool MCTextPaneCreate(class MCStack* p_on_stack, MCTextPaneRef& r_pane);

// Deletes the given text pane
bool MCTextPaneDelete(MCTextPaneRef p_pane);

// Sets the contents of the text pane to be the given string. Other than certain
// inline control characters (tabs, newlines, BiDi controls, etc), the string
// is unformatted.
bool MCTextPaneSetContentsPlain(MCTextPaneRef p_pane, MCStringRef p_contents);
    
MCTextPaneRef MCTextPaneGet();
void MCTextPaneSet(MCTextPaneRef p_pane);
void MCTextPanePaintShim(MCTextPaneRef p_pane);

////////////////////////////////////////////////////////////////////////////////

}

#endif  // ifndef TEXT_API_H
