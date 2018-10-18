# Properties

The tree view widget now has the ability to automatically expand to reveal
a row when it is selected. If **scrollHilitedElementIntoView** is not `true`
then the scroll position will be adjusted to maintain the currently visible
top row.

The tree view fold state can now be reset in two ways. To set the array
data in a folded state then use the **foldedArrayData** property. To
collapse the tree without changing the data, get the **foldedArrayData**.
