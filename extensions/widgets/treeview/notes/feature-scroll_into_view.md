# Properties

The tree view widget now has the ability to scroll the selected row into 
view. If `true`, this will happen when setting the **arrayData**, 
setting the **hilitedElement**, and when adding a new row 
(when **hiliteNewElement** is `true`).

One property has been added to achieve this:
* **scrollHilitedElementIntoView**: either `true` or `false`

The default value is `false` to match the behavior of previous versions 
of the widget.

When selecting a row that is partially visible, the view will be 
adjusted so that the full row is visible.

When changing the **readOnly** property, the view will only change if
the value is being set to `true` and the `Add new element` row is 
currently visible.