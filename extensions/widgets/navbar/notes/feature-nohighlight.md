# No Highlight Option

Add an option to have none of the navigation items highlighted.

## Properties

* The **hilightedItem** property has been enhanced to accept a value of
  0 which indicates that no items will be highlighted.

## Backward Compatibility Note

* If a stack is saved without an item being highlighted, then when
  opened in an earlier version of the widget, a call to
  `getNavSelectedItemName` will throw an error if called before an
  item is selected.

* The widget will render properly (with nothing highlighted) and issue
  the **hiliteChanged** message when an item is selected.