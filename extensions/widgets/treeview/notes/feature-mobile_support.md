# Properties

Enhance the Tree View Widget to support use on the mobile platform.

* Adjust row height when font name or size changes
* Trimming improvements
    * When font name or size changes, re-evaluate ellipsis width to properly trim contents of keys/values
    * In `readOnly` mode, only save space for a single icon on the right
    * Prevent last character from being turned into an ellipsis
    * Width was being reported with extra space for an ellipsis when not needed
    * Remove padding from ellipsis width, only add padding when needed
* Save `alternate row backgrounds` property
* Save `show border` property

New properties:

* `charsToTrimFromKey` - allow a sorting value to be added to the front of the key that is trimmed for display
* `hilitedElementIsFolded` - adjust the fold state of the selected element
* `formattedHeight` - content height for scroller support
* `scroll` and `vScroll` - scroll position
* `textHeight` - custom row height
* `vScrollBar` - control visibility of scroll bar
* `showHover` - allow hover to be disabled, useful on mobile
* `iconHeight` - allow configuration of icon size
* `showValues` - allow the values to be hidden

# Signals

* `formattedHeightChanged` - message to report content height change to support scrollers