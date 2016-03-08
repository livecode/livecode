# General rework
The segmented control widget has been reworked to make it more fully
integrated into the IDE and behave more like a classic control.

## Standard property names
The segmented control now uses standard property names for its 
properties (where they exist). 

- `multiSelect` is now called `multipleHilites`
- `showFrameBorder` is now called `showBorder`
- `segmentColor` is now called `backColor`
- `segmentLabelColor` is now called `foreColor`
- `selectedSegmentColor` is now called `hiliteColor`
- `segmentSelectedLabelColor` is now called `textHiliteColor`

Segmented controls saved with the old property names will load 
correctly, but any scripts will need to be updated to use the new
property names.

## Inherited colors
When no colors are set on the segmented control, it will use a set of 
generic native theme colors.

## Style property removed
The `style` property has been removed.

## Messages
The `segmentSelected` and `segmentUnselected` properties have been 
removed. Use the `selectionChanged` message instead.