# Properties

The tree view widget now has the ability to automatically select a new
row when it is added.

The tree view widget now has the ability to scroll the selected row into 
view. If `true`, this will happen when setting the **arrayData**, 
setting the **hilitedElement**, and when adding a new row 
(when **hiliteNewElement** is `true`).

Two properties have been added to achieve these options:
* **hiliteNewElement**: either `true` or `false`
* **scrollHilitedElementIntoView**: either `true` or `false`

The default values for both are `false` to match the behavior of previous 
versions of the widget.

When selecting a row that is partially visible, the view will be 
adjusted so that the full row is visible.

When changing the **readOnly** property, the view will only change 
position if the value is being set to `true` and the `Add new element` 
row is currently visible.