# Properties

The tree view widget now will automatically expand to reveal a row when 
it is selected. If **scrollHilitedElementIntoView** is not `true`
then the scroll position will be adjusted to maintain the currently visible
top row.

The tree view widget now has the ability to get and set the fold state
of the selected element via the **hilitedElementFoldState** property.
Values are `folded`, `unfolded`, and `leaf`.

Setting a value other than `folded` or `unfolded` will throw an error.

Setting a value when nothing is selected or setting a value on a leaf
node will have no effect.

The **autoFoldStateReset** boolean property is added to allow the fold 
state to be reset when the array data is set.

Default value is `false` to match existing behavior.