---
version: 8.0.0-dp-3
---
# LiveCode Builder Host Library

## Composed widgets

The ability to compose widget objects has been added. Widgets can
either be 'host' widgets, created when a widget is directly embedded
in a stack, or 'child' widgets which are created when a widget is used
as a child widget within another widget.

* A `Widget` type has been added, so that variables can contain
  references to child widget objects.  Variables that hold widget
  references can be defined in the usual way, e.g.  `variable tWidget
  as Widget`.

* New widget syntax has been added to create, place, unplace and
  manipulate child widgets.

  * `a new widget <kind>`: Create a widget object of the specified kind.
  * `place <widget> [at (bottom|top) | (below|above) <other widget>]`: Add a child widget to this widget on the specified layer.
  * `unplace <widget>`: Remove a child widget from this widget.
  * `the target`: Return the child widget that started the current execution.
  * `my children`: Return a list of the currently placed child widgets of this widget.
  * `property <property> of <widget>`: Get or set a property implemented by a child widget.
  * `the rectangle of <widget>`: Get or set the **rectangle** property of a child widget.
  * `the width of <widget>`: Get or set the **width** property of a child widget.
  * `the height of <widget>`: Get or set the **height** property of a child widget.
  * `the location of <widget>`: Get or set the **location** property of a child widget.
  * `the enabled of <widget>`: Get or set the **enabled** property of a child widget.
  * `the disabled of <widget>`: Get or set the **disabled** property of a child widget.
  * `annotation <name> of <widget>`: Tag child widgets with named values.

* Events triggered on child widgets (such as `OnMouseUp`) are
  automatically passed up to the parent, as long as the child's event
  handler returns nothing. If any event handler returns something, the
  event is considered handled and is not passed to the parent.

* Messages posted by the child widget can be handled by the parent in
  an `On<message name>` handler.

  For example, if the child executes the statement: `post
  "dataChanged" with [mDataArray]`, this can be handled in the parent
  by adding a handler `public handler OnDataChanged(in pArray as
  Array)`.  Posted messages can only be handled by a direct parent,
  and a widget's script object will only receive messages posted by
  host widget, i.e. the topmost parent.

* The [Simple Composed Widget](https://github.com/livecode/livecode/blob/develop/extensions/widgets/simplecomposed/simplecomposed.lcb)
  provides an example of how the host/child relationship can be used.

